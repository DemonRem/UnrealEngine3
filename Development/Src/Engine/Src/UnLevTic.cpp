/*=============================================================================
	UnLevTic.cpp: Level timer tick function
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineDecalClasses.h"
#include "UnNet.h"
#include "UnPath.h"
#include "DemoRecording.h"

#include "EngineUserInterfaceClasses.h"
#include "EngineSequenceClasses.h"
#include "EngineParticleClasses.h"

/** Game stats */
DECLARE_STATS_GROUP(TEXT("Game"),STATGROUP_Game);

DECLARE_CYCLE_STAT(TEXT("DecalMgr Tick Time"),STAT_DecalTime,STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("Async Work Time"),STAT_PhysicsTime,STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("Move Actor Time"),STAT_MoveActorTime,STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("Farmove Actor Time"),STAT_FarMoveActorTime,STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("GC Mark Time"),STAT_GCMarkTime,STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("GC Sweep Time"),STAT_GCSweepTime,STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("Update Components Time"),STAT_UpdateComponentsTime,STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("Script Time"),STAT_UnrealScriptTime,STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("Kismet Time"),STAT_KismetTime,STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("Temp Time"),STAT_TempTime,STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("Post AW Actor Tick"),STAT_PostAsyncTickTime,STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("During AW Actor Tick"),STAT_DuringAsyncTickTime,STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("Pre AW Actor Tick"),STAT_PreAsyncTickTime,STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("Post AW Comp Tick"),STAT_PostAsyncComponentTickTime,STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("During AW Comp Tick"),STAT_DuringAsyncComponentTickTime,STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("Pre AW Comp Tick"),STAT_PreAsyncComponentTickTime,STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("Tick Time"),STAT_TickTime,STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("Volume Streaming Tick"),STAT_VolumeStreamingTickTime,STATGROUP_Streaming);
DECLARE_DWORD_COUNTER_STAT(TEXT("Post AW Actors Ticked"),STAT_PostAsyncActorsTicked,STATGROUP_Game);
DECLARE_DWORD_COUNTER_STAT(TEXT("During AW Actors Ticked"),STAT_DuringAsyncActorsTicked,STATGROUP_Game);
DECLARE_DWORD_COUNTER_STAT(TEXT("Pre AW Actors Ticked"),STAT_PreAsyncActorsTicked,STATGROUP_Game);
DECLARE_DWORD_COUNTER_STAT(TEXT("Post AW Comps Ticked"),STAT_PostAsyncComponentsTicked,STATGROUP_Game);
DECLARE_DWORD_COUNTER_STAT(TEXT("During AW Comps Ticked"),STAT_DuringAsyncComponentsTicked,STATGROUP_Game);
DECLARE_DWORD_COUNTER_STAT(TEXT("Pre AW Comps Ticked"),STAT_PreAsyncComponentsTicked,STATGROUP_Game);
DECLARE_DWORD_COUNTER_STAT(TEXT("Streaming Volumes"),STAT_VolumeStreamingChecks,STATGROUP_Streaming);

/*-----------------------------------------------------------------------------
	ParticleDataManager
-----------------------------------------------------------------------------*/
extern FParticleDataManager	GParticleDataManager;


extern UBOOL GShouldLogOutAFrameOfSkelCompTick;


/*-----------------------------------------------------------------------------
	FTickableObject implementation.
-----------------------------------------------------------------------------*/

/** Static array of tickable objects */
TArray<FTickableObject*> FTickableObject::TickableObjects;

/** Static array of tickable objects that are ticked from rendering thread*/
TArray<FTickableObject*> FTickableObject::RenderingThreadTickableObjects;

/*-----------------------------------------------------------------------------
	Helper classes.
-----------------------------------------------------------------------------*/

FNetViewer::FNetViewer(UNetConnection* InConnection, FLOAT DeltaSeconds)
	: InViewer(InConnection->Actor), Viewer(InConnection->Viewer)
{
	// Get viewer coordinates.
	ViewLocation = Viewer->Location;
	FRotator ViewRotation = InViewer->Rotation;
	InViewer->eventGetPlayerViewPoint(ViewLocation, ViewRotation);
	ViewDir = ViewRotation.Vector();

	// Compute ahead-vectors for prediction.
	FVector Ahead = FVector(0,0,0);
	if (InConnection->TickCount & 1)
	{
		FLOAT PredictSeconds = (InConnection->TickCount & 2) ? 0.4f : 0.9f;
		Ahead = PredictSeconds * Viewer->Velocity;
		if( Viewer->Base )
		{
			Ahead += PredictSeconds * Viewer->Base->Velocity;
		}
		if (!Ahead.IsZero())
		{
			FCheckResult Hit(1.0f);
			Hit.Location = ViewLocation + Ahead;
			GWorld->BSPLineCheck(Hit, NULL, Hit.Location, ViewLocation, FVector(0,0,0), TRACE_Visible);
			ViewLocation = Hit.Location;
		}
	}
}

//
// Priority sortable list.
//
struct FActorPriority
{
	INT			    Priority;	// Update priority, higher = more important.
	AActor*			Actor;		// Actor.
	UActorChannel*	Channel;	// Actor channel.
	FActorPriority()
	{}
	FActorPriority(UNetConnection* InConnection, UActorChannel* InChannel, AActor* InActor, const TArray<FNetViewer>& Viewers, UBOOL bLowBandwidth)
		: Actor(InActor), Channel(InChannel)
	{	
		FLOAT Time  = Channel ? (InConnection->Driver->Time - Channel->LastUpdateTime) : InConnection->Driver->SpawnPrioritySeconds;
		// take the highest priority of the viewers on this connection
		Priority = 0;
		for (INT i = 0; i < Viewers.Num(); i++)
		{
			Priority = Max<INT>(Priority, appRound(65536.0f * Actor->GetNetPriority(Viewers(i).ViewLocation, Viewers(i).ViewDir, Viewers(i).InViewer, InChannel, Time, bLowBandwidth)));
		}
	}
};

IMPLEMENT_COMPARE_POINTER( FActorPriority, UnLevTic, { return B->Priority - A->Priority; } )

/**
 * Class that holds lists of objects that need deferred ticking
 */
class FDeferredTickList
{
	/**
	 * During async operations list for Actors
	 */
	TArray<AActor*> ActorsDuringAsync;
	/**
	 * During async operations list for ActorComponents
	 */
	TArray<UActorComponent*> ComponentsDuringAsync;
	/**
	 * Post async operations list for Actors
	 */
	TArray<AActor*> ActorsPostAsync;
	/**
	 * Post async operations list for ActorComponents
	 */
	TArray<UActorComponent*> ComponentsPostAsync;

public:
	/**
	 * Default ctor, starts by ticking pre async work
	 */
	FDeferredTickList(void)
	{
	}

	/**
	 * Resets the internal state of the object between Tick()s
	 * NOTE: Uses pre-sized arrays to minimize memory allocations
	 */
	void Reset(void)
	{
		const INT NumActorsPreSize = 500;
		const INT NumComponentsPreSize = NumActorsPreSize * 4;
		// Presize each array with the sizes above. Use the max size in case
		// the number of ticked/deferred actor/components is beyond our expected
		// sizes. This prevents memory thrashing when the expected size is exceeded
		ActorsDuringAsync.Empty(Max<INT>(NumActorsPreSize,ActorsDuringAsync.Num()));
		ComponentsDuringAsync.Empty(Max<INT>(NumComponentsPreSize,ComponentsDuringAsync.Num()));
		ActorsPostAsync.Empty(Max<INT>(NumActorsPreSize,ActorsPostAsync.Num()));
		ComponentsPostAsync.Empty(Max<INT>(NumComponentsPreSize,ComponentsPostAsync.Num()));
	}

	/**
	 * Decides whether to defer ticking of this actor. If the actor is
	 * deferred, it is added to the correct list.
	 *
	 * NOTE: This is done externally to actor until we have enough information
	 * about what classes/types of things need to be checked. So for now, it
	 * is here and inlined. If need be, we'll move this to an actor method and
	 * make it virtual, for now speed rules.
	 *
	 * @param Actor the actor to check for deferring ticking
	 *
	 * @return TRUE if the actor was deferred, FALSE if it needs ticking
	 */
	FORCEINLINE UBOOL ConditionalDefer(AActor* Actor)
	{
		UBOOL bDeferred = FALSE;
		// Defer based upon tick group setting
		if (GWorld->TickGroup < TG_DuringAsyncWork &&
			Actor->TickGroup == TG_DuringAsyncWork)
		{
			bDeferred = TRUE;
			ActorsDuringAsync.AddItem(Actor);
		}
		else if (GWorld->TickGroup < TG_PostAsyncWork &&
			Actor->TickGroup == TG_PostAsyncWork)
		{
			bDeferred = TRUE;
			ActorsPostAsync.AddItem(Actor);
		}
		return bDeferred;
	}

	/**
	 * Places a newly spawned actor in the post async work list
	 *
	 * @param Actor the actor to check for deferring ticking
	 */
	FORCEINLINE void AddNewlySpawned(AActor* Actor)
	{
		check(GWorld->TickGroup < TG_PostAsyncWork);
		ActorsPostAsync.AddItem(Actor);
	}

	/**
	 * Decides whether to defer ticking of this component. If the component is
	 * deferred, it is added to the correct list.
	 *
	 * @param Component the component to check for deferring ticking
	 *
	 * @return TRUE if the component was deferred, FALSE if it needs ticking
	 */
	FORCEINLINE UBOOL ConditionalDefer(UActorComponent* Component)
	{
		UBOOL bDeferred = FALSE;
		// Components are only deferred based upon tick group
		if (GWorld->TickGroup < TG_DuringAsyncWork &&
			Component->TickGroup == TG_DuringAsyncWork)
		{
			bDeferred = TRUE;
			ComponentsDuringAsync.AddItem(Component);
		}
		else if (GWorld->TickGroup < TG_PostAsyncWork &&
			Component->TickGroup == TG_PostAsyncWork)
		{
			bDeferred = TRUE;
			ComponentsPostAsync.AddItem(Component);
		}
		return bDeferred;
	}

	/**
	 * Common class for iterating a deferred array of tickable items
	 */
	template<typename CONTAINED_TYPE> class TDeferredArrayIterator
	{
		/**
		 * The current index that we are at
		 */
		INT Index;
		/**
		 * The object array that is being iterated
		 */
		TArray<CONTAINED_TYPE*>& Array;

	protected:
		/**
		 * Used by derived classes to tell it which array to iterate
		 */
		TDeferredArrayIterator(TArray<CONTAINED_TYPE*>& InArray) :
			Index(0),
			Array(InArray)
		{
		}

	public:
		/**
		 * Returns the current object pointed at by the Iterator
		 *
		 * @return	Current object in the array
		 */
		FORCEINLINE CONTAINED_TYPE* operator*(void)
		{
			return Array(Index);
		}

		/**
		 * Returns the current suitable actor pointed at by the Iterator
		 *
		 * @return	Current suitable actor
		 */
		FORCEINLINE CONTAINED_TYPE* operator->(void)
		{
			return Array(Index);
		}

		/**
		 * Returns whether the iterator has reached the end of the array or not
		 *
		 * @return TRUE if iterator points to a suitable item, FALSE if it has reached the end
		 */
		FORCEINLINE operator UBOOL(void)
		{
			return Index < Array.Num();
		}

		/**
		 * Updates the index of the iterator
		 */
		FORCEINLINE	void operator++(void)
		{
			++Index;
		}
	};

	/**
	 * Global actor iterator
	 */
	class FGlobalActorIterator :
		public FDynamicActorIterator
	{
	public:
		/**
		 * Ctor to meet the interface. Ignores the parameter
		 */
		FGlobalActorIterator(FDeferredTickList&)
		{
		}
	};

	/**
	 * Iterator for actors in the during async group
	 */
	class FActorDuringAsyncWorkIterator :
		public TDeferredArrayIterator<AActor>
	{
	public:
		/**
		 * Passes the during async work list to the base iterator
		 */
		FActorDuringAsyncWorkIterator(FDeferredTickList& List) :
			TDeferredArrayIterator<AActor>(List.ActorsDuringAsync)
		{
		}
	};

	/**
	 * Iterator for actors in the post async group
	 */
	class FActorPostAsyncWorkIterator :
		public TDeferredArrayIterator<AActor>
	{
	public:
		/**
		 * Passes the post async work list to the base iterator
		 */
		FActorPostAsyncWorkIterator(FDeferredTickList& List) :
			TDeferredArrayIterator<AActor>(List.ActorsPostAsync)
		{
		}
	};

	/**
	 * Iterator for deferred components. Handles ticking during the async work
	 */
	class FComponentDuringAsyncWorkIterator :
		public TDeferredArrayIterator<UActorComponent>
	{
	public:
		/**
		 * Passes the during async work list to the base iterator
		 */
		FComponentDuringAsyncWorkIterator(FDeferredTickList& List) :
			TDeferredArrayIterator<UActorComponent>(List.ComponentsDuringAsync)
		{
		}
	};

	/**
	 * Iterator for deferred components. Handles ticking after the async work
	 */
	class FComponentPostAsyncWorkIterator :
		public TDeferredArrayIterator<UActorComponent>
	{
	public:
		/**
		 * Passes the during async work list to the base iterator
		 */
		FComponentPostAsyncWorkIterator(FDeferredTickList& List) :
			TDeferredArrayIterator<UActorComponent>(List.ComponentsPostAsync)
		{
		}
	};
	// Friend access since these are internal to FDeferredTickList anyway
	friend class FComponentPostAsyncWorkIterator;
	friend class FComponentDuringAsyncWorkIterator;
	friend class FActorPostAsyncWorkIterator;
	friend class FActorDuringAsyncWorkIterator;
};

/*-----------------------------------------------------------------------------
	Tick a single actor.
-----------------------------------------------------------------------------*/

void AActor::TickAuthoritative( FLOAT DeltaSeconds )
{
	// Tick the nonplayer.
	//clockSlow(GStats.DWORDStats(GEngineStats.STATS_Game_ScriptTickCycles));
	eventTick(DeltaSeconds);
	//unclockSlow(GStats.DWORDStats(GEngineStats.STATS_Game_ScriptTickCycles));

	// Update the actor's script state code.
	ProcessState( DeltaSeconds );

	UpdateTimers(DeltaSeconds );

	// Update LifeSpan.
	if( LifeSpan!=0.f )
	{
		LifeSpan -= DeltaSeconds;
		if( LifeSpan <= 0.0001f )
		{
			// Actor's LifeSpan expired.
			GWorld->DestroyActor( this );
			return;
		}
	}

	// Perform physics.
	if ( !bDeleteMe && (Physics!=PHYS_None) && (Role!=ROLE_AutonomousProxy) )
		performPhysics( DeltaSeconds );
}

void AActor::TickSimulated( FLOAT DeltaSeconds )
{
	TickAuthoritative(DeltaSeconds);
}

void APawn::TickSimulated( FLOAT DeltaSeconds )
{
	// Simulated Physics for pawns
	// simulate gravity

	if ( bHardAttach )
	{
		Acceleration = FVector(0.f,0.f,0.f);
		if (Physics == PHYS_RigidBody)
			setPhysics(PHYS_None);
		else
			Physics = PHYS_None;
	}
	else if (Physics == PHYS_RigidBody || Physics == PHYS_Interpolating)
	{
		performPhysics(DeltaSeconds);
	}
	else if (Physics == PHYS_Spider)
	{
		// never try to detect/simulate other physics or gravity when spidering
		Acceleration = Velocity.SafeNormal();
		moveSmooth(Velocity * DeltaSeconds);
	}
	else
	{
		// make sure we have a valid physicsvolume (level streaming might kill it)
		if (PhysicsVolume == NULL)
		{
			SetZone(FALSE, FALSE);
		}

		Acceleration = Velocity.SafeNormal();
		if ( PhysicsVolume->bWaterVolume )
			Physics = PHYS_Swimming;
		else if ( bCanClimbLadders && PhysicsVolume->IsA(ALadderVolume::StaticClass()) )
			Physics = PHYS_Ladder;
		else if ( bSimulateGravity )
			Physics = PHYS_Walking;	// set physics mode guess for use by animation
		else
			Physics = PHYS_Flying;

		//simulated pawns just predict location, no script execution
		moveSmooth(Velocity * DeltaSeconds);

		// allow touched actors to impact physics
		if( PendingTouch )
		{
			PendingTouch->eventPostTouch(this);
			AActor *OldTouch = PendingTouch;
			PendingTouch = PendingTouch->PendingTouch;
			OldTouch->PendingTouch = NULL;
		}

		// if simulated gravity, check if falling
		if ( bSimulateGravity && !bSimGravityDisabled )
		{
			FVector CollisionCenter = Location + CylinderComponent->Translation;
			FCheckResult Hit(1.f);
			if ( Velocity.Z == 0.f )
			{
				GWorld->SingleLineCheck(Hit, this, CollisionCenter - FVector(0.f,0.f,1.5f * CylinderComponent->CollisionHeight), CollisionCenter, TRACE_AllBlocking, FVector(CylinderComponent->CollisionRadius,CylinderComponent->CollisionRadius,4.f));
			}
			else if ( Velocity.Z < 0.f )
			{
				GWorld->SingleLineCheck(Hit, this, CollisionCenter - FVector(0.f,0.f,8.f), CollisionCenter, TRACE_AllBlocking, GetCylinderExtent());
			}

			if ( (Hit.Time == 1.f) || (Hit.Normal.Z < WalkableFloorZ) )
			{
				if ( Velocity.Z == 0.f )
					Velocity.Z = 0.15f * GetGravityZ();
				Velocity.Z += GetGravityZ() * DeltaSeconds;
				Physics = PHYS_Falling;
			}
			else
				Velocity.Z = 0.f;
		}
	}

	// Tick the nonplayer.
	//clockSlow(GStats.DWORDStats(GEngineStats.STATS_Game_ScriptTickCycles));
	eventTick(DeltaSeconds);
	//unclockSlow(GStats.DWORDStats(GEngineStats.STATS_Game_ScriptTickCycles));

	// Update the actor's script state code.
	ProcessState( DeltaSeconds );

	UpdateTimers(DeltaSeconds );
}

void AActor::TickSpecial( FLOAT DeltaSeconds )
{
}

void APawn::TickSpecial( FLOAT DeltaSeconds )
{
	if( (Role==ROLE_Authority) && (BreathTime > 0.f) )
	{
		BreathTime -= DeltaSeconds;
		if (BreathTime < 0.001f)
		{
			BreathTime = 0.0f;
			eventBreathTimer();
		}
	}
}

void AProjectile::TickSpecial( FLOAT DeltaSeconds )
{
	Super::TickSpecial(DeltaSeconds);

	if ( bRotationFollowsVelocity )
	{
		Rotation = Velocity.Rotation();
	}
}

void AEmitterPool::TickSpecial(FLOAT DeltaTime)
{
	Super::TickSpecial(DeltaTime);

	INT i = 0;
	while (i < RelativePSCs.Num())
	{
		if (RelativePSCs(i).PSC == NULL || RelativePSCs(i).Base == NULL || RelativePSCs(i).Base->bDeleteMe)
		{
			RelativePSCs.Remove(i, 1);
		}
		else
		{
			FVector NewTranslation = RelativePSCs(i).Base->Location + RelativePSCs(i).RelativeLocation;
			FRotator NewRotation = RelativePSCs(i).Base->Rotation + RelativePSCs(i).RelativeRotation;
			if (RelativePSCs(i).PSC->Translation != NewTranslation || RelativePSCs(i).PSC->Rotation != NewRotation)
			{
				RelativePSCs(i).PSC->Translation = NewTranslation;
				RelativePSCs(i).PSC->Rotation = NewRotation;
				RelativePSCs(i).PSC->BeginDeferredUpdateTransform();
			}
			i++;
		}
	}
}

UBOOL AActor::PlayerControlled()
{
	return 0;
}

UBOOL APawn::PlayerControlled()
{
	return ( IsLocallyControlled() && Controller != NULL && Controller->GetAPlayerController() );
}

/* AActor::InStasis()
 * Called from AActor::Tick() if Actor->bStasis==true
 * @return true if this actor ands its components can safely not be ticked.
 */
UBOOL AActor::InStasis()
{
	return ((Physics==PHYS_None) || (Physics == PHYS_Rotating))
			&& (WorldInfo->TimeSeconds - LastRenderTime > 5.0f || WorldInfo->NetMode == NM_DedicatedServer);
}

/** ticks the actor
 * @return TRUE if the actor was ticked, FALSE if it was aborted (e.g. because it's in stasis)
 */
UBOOL AActor::Tick( FLOAT DeltaSeconds, ELevelTick TickType )
{
	bTicked = GWorld->Ticked;

	// Ignore actors in stasis
	if (bStasis && InStasis())
	{
		return FALSE;
	}

	// Non-player update.
	UBOOL bShouldTick = ((TickType!=LEVELTICK_ViewportsOnly) || PlayerControlled());
	if(bShouldTick)
	{
		// This actor is tickable.
		if( RemoteRole == ROLE_AutonomousProxy )
		{
			APlayerController *PC = GetTopPlayerController();
			if ( (PC && PC->LocalPlayerController()) || Physics == PHYS_RigidBody || Physics == PHYS_Interpolating )
			{
				TickAuthoritative(DeltaSeconds);
			}
			else
			{
				eventTick(DeltaSeconds);

				// Update the actor's script state code.
				ProcessState( DeltaSeconds );
				// Server handles timers for autonomous proxy.
				UpdateTimers( DeltaSeconds );
			}
		}
		else if ( Role>ROLE_SimulatedProxy )
		{
			TickAuthoritative(DeltaSeconds);
		}
		else if ( Role == ROLE_SimulatedProxy )
		{
			TickSimulated(DeltaSeconds);
		}
		else if ( !bDeleteMe && ((Physics == PHYS_Falling) || (Physics == PHYS_Rotating) || (Physics == PHYS_Projectile) || (Physics == PHYS_Interpolating)) ) // dumbproxies simulate falling if client side physics set
		{
			performPhysics( DeltaSeconds );
		}

		if (!bDeleteMe)
		{
			TickSpecial(DeltaSeconds);	// perform any tick functions unique to an actor subclass
		}
	}
	
	return TRUE;
}


/* Controller Tick
Controllers are never animated, and do not look for an owner to be ticked before them
Non-player controllers don't support being an autonomous proxy
*/
UBOOL AController::Tick( FLOAT DeltaSeconds, ELevelTick TickType )
{
	bTicked = GWorld->Ticked;

	if (TickType == LEVELTICK_ViewportsOnly)
	{
		return TRUE;
	}

	// Ignore controllers whose pawn is in stasis
	if ( (Pawn && Pawn->bStasis && Pawn->InStasis())
		|| (bStasis && InStasis()) )
	{
		return FALSE;
	}

	if( Role>=ROLE_SimulatedProxy )
	{
		TickAuthoritative(DeltaSeconds);
	}
	
	// Update eyeheight and send visibility updates
	// with PVS, monsters look for other monsters, rather than sending msgs

	if( Role==ROLE_Authority && TickType==LEVELTICK_All )
	{
		if( SightCounter < 0.0f )
		{
			if( IsProbing(NAME_EnemyNotVisible) )
			{
				CheckEnemyVisible();
			}
			SightCounter += 0.15f + 0.1f * appSRand();
		}

		SightCounter = SightCounter - DeltaSeconds;
		// for best performance, players show themselves to players and non-players (e.g. monsters),
		// and monsters show themselves to players
		// but monsters don't show themselves to each other
		// also

		if( Pawn && !Pawn->bHidden && !Pawn->bAmbientCreature )
		{
			ShowSelf();
		}
	}

	if ( Pawn != NULL )
	{
		UpdatePawnRotation();
	}
	return TRUE;
}

/*
PlayerControllers
Controllers are never animated, and do not look for an owner to be ticked before them
*/
UBOOL APlayerController::Tick( FLOAT DeltaSeconds, ELevelTick TickType )
{
	bTicked = GWorld->Ticked;

	GetViewTarget();
	if( (RemoteRole == ROLE_AutonomousProxy) && !LocalPlayerController() )
	{
		// kick idlers
		if ( PlayerReplicationInfo )
		{
			if ( (Pawn && ( !GWorld->GetWorldInfo()->Game || !GWorld->GetWorldInfo()->Game->bKickLiveIdlers || (Pawn->Physics != PHYS_Walking)) ) || !bShortConnectTimeOut || (PlayerReplicationInfo->bOnlySpectator && (ViewTarget != this)) || PlayerReplicationInfo->bOutOfLives 
			|| GWorld->GetWorldInfo()->Pauser || (GWorld->GetGameInfo() && (GWorld->GetGameInfo()->bWaitingToStartMatch || GWorld->GetGameInfo()->bGameEnded || (GWorld->GetGameInfo()->NumPlayers < 2))) || PlayerReplicationInfo->bAdmin )
			{
				LastActiveTime = GWorld->GetTimeSeconds();
			}
			else if ( (GWorld->GetGameInfo()->MaxIdleTime > 0) && (GWorld->GetTimeSeconds() - LastActiveTime > GWorld->GetGameInfo()->MaxIdleTime - 10) )
			{
				if ( GWorld->GetTimeSeconds() - LastActiveTime > GWorld->GetGameInfo()->MaxIdleTime )
				{
					GWorld->GetGameInfo()->eventKickIdler(this);
					LastActiveTime = GWorld->GetTimeSeconds() - GWorld->GetGameInfo()->MaxIdleTime + 3.f;
				}
				else
					eventKickWarning();
			}
		}

		// force physics update for clients that aren't sending movement updates in a timely manner 
		// this prevents cheats associated with artificially induced ping spikes
		if ( Pawn && !Pawn->bDeleteMe 
			&& (Pawn->Physics!=PHYS_None) && (Pawn->Physics != PHYS_RigidBody)
			&& (GWorld->GetTimeSeconds() - ServerTimeStamp > ::Max<FLOAT>(DeltaSeconds+0.06f,UCONST_MAXCLIENTUPDATEINTERVAL)) 
			&& (ServerTimeStamp != 0.f) )
		{
			// force position update
			if ( !Pawn->Velocity.IsZero() )
			{
				Pawn->performPhysics( GWorld->GetTimeSeconds() - ServerTimeStamp );
			}
			ServerTimeStamp = GWorld->GetTimeSeconds();
			TimeMargin = 0.f;
			MaxTimeMargin = ((AGameInfo *)(AGameInfo::StaticClass()->GetDefaultActor()))->MaxTimeMargin;
		}

		// update viewtarget replicated info
		if( ViewTarget != Pawn )
		{
            APawn* TargetPawn = ViewTarget ? ViewTarget->GetAPawn() : NULL; 
			if ( TargetPawn )
			{
				if ( TargetPawn->Controller && TargetPawn->Controller->GetAPlayerController() )
					TargetViewRotation = TargetPawn->Controller->Rotation;
				else
					TargetViewRotation = TargetPawn->Rotation;
				TargetEyeHeight = TargetPawn->BaseEyeHeight;
			}
		}

		// Update the actor's script state code.
		ProcessState( DeltaSeconds );
		// Server handles timers for autonomous proxy.
		UpdateTimers( DeltaSeconds );

		// send ClientAdjustment if necessary
		if ( PendingAdjustment.TimeStamp > 0.f )
			eventSendClientAdjustment();
	}
	else if( Role>=ROLE_SimulatedProxy )
	{
		// Process PlayerTick with input.
		if ( !PlayerInput )
			eventInitInputSystem();

		for(INT InteractionIndex = 0;InteractionIndex < Interactions.Num();InteractionIndex++)
			if(Interactions(InteractionIndex))
				Interactions(InteractionIndex)->Tick(DeltaSeconds);

		if(PlayerInput)
			eventPlayerTick(DeltaSeconds);

		for(INT InteractionIndex = 0;InteractionIndex < Interactions.Num();InteractionIndex++)
			if(Interactions(InteractionIndex))
				Interactions(InteractionIndex)->Tick(-1.0f);

		// Update the actor's script state code.
		ProcessState( DeltaSeconds );

		UpdateTimers( DeltaSeconds );

		if ( bDeleteMe )
			return 1;

		// Perform physics.
		if( Physics!=PHYS_None && Role!=ROLE_AutonomousProxy )
			performPhysics( DeltaSeconds );

		// update viewtarget replicated info
		if( ViewTarget != Pawn )
		{
            APawn* TargetPawn = ViewTarget ? ViewTarget->GetAPawn() : NULL; 
			if ( TargetPawn )
			{
				SmoothTargetViewRotation(TargetPawn, DeltaSeconds);
			}
		}

	}

	// Update eyeheight and send visibility updates
	// with PVS, monsters look for other monsters, rather than sending msgs
	if( Role==ROLE_Authority && TickType==LEVELTICK_All )
	{
		if( SightCounter < 0.0f )
		{
			SightCounter += 0.2f;
		}
		SightCounter = SightCounter - DeltaSeconds;

		if( Pawn && !Pawn->bHidden )
		{
			ShowSelf();
		}
	}

	return 1;
}

/* Update active timers */
void AActor::UpdateTimers(FLOAT DeltaSeconds)
{
	// split into two loops to avoid infinite loop where
	// the timer is called, causes settimer to be called
	// again with a rate less than our current delta
	// and causing an invalid index to be accessed
	for (INT Idx = 0; Idx < Timers.Num(); Idx++)
	{
		// just increment the counters
		Timers(Idx).Count += DeltaSeconds;
	}

	UBOOL bRemoveTimer = false;
	
	for (INT Idx = 0; Idx < Timers.Num() && !IsPendingKill(); Idx++)
	{
		// check for a cleared timer
		// we check this here instead of the previous loop so that if a timer function that is called clears some other timer, that other timer doesn't get called (since its Rate would then be zero)
		if (Timers(Idx).Rate == 0.f)
		{
			Timers.Remove(Idx--, 1);
		}
		else if (Timers(Idx).Rate < Timers(Idx).Count)
		{
			UObject* TimerObj = Timers(Idx).TimerObj;

			bRemoveTimer = false;

			// calculate how many times the timer may have elapsed
			// (for large delta times on looping timers)
			INT CallCount = Timers(Idx).bLoop == 1 ? appTrunc(Timers(Idx).Count/Timers(Idx).Rate) : 1;
			
			// lookup the function to call
			UFunction *Func = TimerObj->FindFunction(Timers(Idx).FuncName);
			// if we didn't find the function, or it's not looping
			if( Func == NULL ||
				!Timers(Idx).bLoop )
			{
				if( Func == NULL ) 
				{
					debugf(NAME_Warning,
						TEXT("Failed to find function %s for timer in actor %s"),
						*Timers(Idx).FuncName.ToString(), *TimerObj->GetName() );
				}
				// mark the timer for removal
				bRemoveTimer = true;
			}
			else
			{
				// otherwise reset for loop
				Timers(Idx).Count -= CallCount * Timers(Idx).Rate;
			}

			// now call the function
			if( Func != NULL )
			{
				// allocate null func params
				void *FuncParms = appAlloca(Func->ParmsSize);
				while( CallCount > 0 )
				{
					// make sure any params are cleared
					appMemzero(FuncParms, Func->ParmsSize);

					// and call the function
					TimerObj->ProcessEvent(Func,FuncParms);
					CallCount--;
					
					// Make sure Timer is still relevant
					if( !IsPendingKill() )
					{
						// check to see if the timer was cleared from the last call
						if( Timers(Idx).Rate == 0 )
						{
							// mark the timer for removal
							bRemoveTimer = true;
							break;
						}
						else if( Timers(Idx).Count == 0.f )
						{
							// If timer has been re set, then do not flag for removal.
							bRemoveTimer = false;
						}
					}
				}
			}

			//check to see if this timer should be removed
			if( bRemoveTimer && 
				!IsPendingKill() )
			{
				Timers.Remove(Idx--,1);
			}
		}
	}

}

/*-----------------------------------------------------------------------------
	Network client tick.
-----------------------------------------------------------------------------*/

void UWorld::TickNetClient( FLOAT DeltaSeconds )
{
	if( NetDriver->ServerConnection->State==USOCK_Open )
	{
		// Don't replicate any properties from client to server.
	}
	else if( NetDriver->ServerConnection->State==USOCK_Closed )
	{
		// ask local player to display connection lost message
		if (GEngine->GamePlayers.Num() == 0 || GEngine->GamePlayers(0) == NULL || GEngine->GamePlayers(0)->Actor == NULL || !GEngine->GamePlayers(0)->Actor->eventNotifyConnectionLost())
		{
			GEngine->SetClientTravel(TEXT("?failed"), TRAVEL_Absolute);
		}
	}
}

/*-----------------------------------------------------------------------------
	Network server ticking individual client.
-----------------------------------------------------------------------------*/

/** returns whether this Actor should be considered relevant because it is visible through
 * the other side of any the passed in list of PortalTeleporters
 */
UBOOL AActor::IsRelevantThroughPortals(const TArray<APortalTeleporter*>& Portals)
{
	for (INT i = 0; i < Portals.Num(); i++)
	{
		if (Portals(i) != NULL && Portals(i)->SisterPortal != NULL && GWorld->BSPFastLineCheck(Location, Portals(i)->SisterPortal->Location))
		{
			return TRUE;
		}
	}

	return FALSE;
}

UBOOL AActor::IsNetRelevantFor(APlayerController* RealViewer, AActor* Viewer, const FVector& SrcLocation)
{
	if( bAlwaysRelevant || IsOwnedBy(Viewer) || IsOwnedBy(RealViewer) || this==Viewer || Viewer==Instigator )
	{
		return 1;
	}
	else if ( Base && (BaseSkelComponent || ((Base == Owner) && !bOnlyOwnerSee)) )
	{
		return Base->IsNetRelevantFor( RealViewer, Viewer, SrcLocation );
	}
	else if( (bHidden || bOnlyOwnerSee) && !bBlockActors )
	{
		return 0;
	}
	else
	{
		FCheckResult Hit(1.f);
		return ( GWorld->SingleLineCheck( Hit, this, SrcLocation, Location, TRACE_World|TRACE_StopAtAnyHit, FVector(0.f,0.f,0.f) ) || IsRelevantThroughPortals(RealViewer->VisiblePortals));
	}
}

UBOOL AProjectile::IsNetRelevantFor(APlayerController* RealViewer, AActor* Viewer, const FVector& SrcLocation)
{
	if ((SrcLocation - Location).SizeSquared() > NetCullDistanceSquared)
	{
		return false;
	}

	return Super::IsNetRelevantFor(RealViewer, Viewer, SrcLocation);
}

UBOOL APlayerController::IsNetRelevantFor(APlayerController* RealViewer, AActor* Viewer, const FVector& SrcLocation)
{
	return ( this==RealViewer );
}

UBOOL APawn::CacheNetRelevancy(UBOOL bIsRelevant, APlayerController* RealViewer, AActor* Viewer)
{
	bCachedRelevant = bIsRelevant;
	NetRelevancyTime = GWorld->GetTimeSeconds();
	LastRealViewer = RealViewer;
	LastViewer = Viewer;
	return bIsRelevant;
}

UBOOL APawn::IsNetRelevantFor(APlayerController* RealViewer, AActor* Viewer, const FVector& SrcLocation)
{
	if ( (NetRelevancyTime == GWorld->GetTimeSeconds()) && (RealViewer == LastRealViewer) && (Viewer == LastViewer) )
		return bCachedRelevant;
	if( bAlwaysRelevant || IsOwnedBy(Viewer) || IsOwnedBy(RealViewer) || this==Viewer || Viewer==Instigator
		|| IsBasedOn(Viewer) || (Viewer && Viewer->IsBasedOn(this)) || RealViewer->bReplicateAllPawns 
		|| (Controller && ((Location - Viewer->Location).SizeSquared() < AlwaysRelevantDistanceSquared)) || HasAudibleAmbientSound(SrcLocation) )
		return CacheNetRelevancy(true,RealViewer,Viewer);
	else if( (bHidden || bOnlyOwnerSee) && !bBlockActors ) 
		return CacheNetRelevancy(false,RealViewer,Viewer);
	else if ( Base && (BaseSkelComponent || ((Base == Owner) && !bOnlyOwnerSee)) )
		return Base->IsNetRelevantFor( RealViewer, Viewer, SrcLocation );
	else
	{
#ifdef USE_DISTANCE_FOG_OCCLUSION
		// check distance fog
		if ( RealViewer->BeyondFogDistance(SrcLocation, Location) )
			return CacheNetRelevancy(false,RealViewer,Viewer);
#endif
		// check against BSP - check head and center
		//debugf(TEXT("Check relevance of %s"),*(PlayerReplicationInfo->PlayerName));
		FCheckResult Hit(1.f);
		if ( !GWorld->SingleLineCheck( Hit, this, Location + FVector(0.f,0.f,BaseEyeHeight), SrcLocation, TRACE_World|TRACE_StopAtAnyHit, FVector(0.f,0.f,0.f) )
			&& !GWorld->SingleLineCheck( Hit, this, Location, SrcLocation, TRACE_World|TRACE_StopAtAnyHit, FVector(0.f,0.f,0.f) )
			 && !IsRelevantThroughPortals(RealViewer->VisiblePortals) )
		{
			return CacheNetRelevancy(false,RealViewer,Viewer);
		}
		return CacheNetRelevancy(true,RealViewer,Viewer);
	}
}

UBOOL AVehicle::IsNetRelevantFor(APlayerController* RealViewer, AActor* Viewer, const FVector& SrcLocation)
{
	if ( bAlwaysRelevant )
		return true;
	if ( (NetRelevancyTime == GWorld->GetTimeSeconds()) && (RealViewer == LastRealViewer) && (Viewer == LastViewer) )
		return bCachedRelevant;
	if( IsOwnedBy(Viewer) || IsOwnedBy(RealViewer) || this==Viewer || Viewer==Instigator
		|| IsBasedOn(Viewer) || (Viewer && Viewer->IsBasedOn(this))  || RealViewer->bReplicateAllPawns 
		|| (Controller && ((Location - Viewer->Location).SizeSquared() < AlwaysRelevantDistanceSquared)) || HasAudibleAmbientSound(SrcLocation) )
		return CacheNetRelevancy(true,RealViewer,Viewer);
	else if( (bHidden || bOnlyOwnerSee) && !bBlockActors )
		return CacheNetRelevancy(false,RealViewer,Viewer);
	else
	{
#ifdef USE_DISTANCE_FOG_OCCLUSION
		// check distance fog
		if ( RealViewer->BeyondFogDistance(SrcLocation, Location) )
			return CacheNetRelevancy(false,RealViewer,Viewer);
#endif
		if ( !CylinderComponent )
		{
			return CacheNetRelevancy(true,RealViewer,Viewer);
		}
		// check Location and collision bounds
		FCheckResult Hit(1.f);
		if ( GWorld->SingleLineCheck( Hit, this, Location + FVector(0.f,0.f,CylinderComponent->CollisionHeight), SrcLocation, TRACE_World|TRACE_StopAtAnyHit, FVector(0.f,0.f,0.f) )
			|| GWorld->SingleLineCheck( Hit, this, Location, SrcLocation, TRACE_World|TRACE_StopAtAnyHit, FVector(0.f,0.f,0.f) )
			 || IsRelevantThroughPortals(RealViewer->VisiblePortals) )
		{
			return CacheNetRelevancy(true,RealViewer,Viewer);
		}
		if ( bDoExtraNetRelevancyTraces )
		{
			// check at corner points as well before failing
			FVector Y = ((Location - SrcLocation) ^ FVector(0.f, 0.f, 1.f)).SafeNormal();

			// randomize point somewhat so stopped vehicle can't worst case stay not relevant
			if ( GWorld->SingleLineCheck( Hit, this, Location + Y*(0.5f + 0.5*appFrand())*CylinderComponent->CollisionRadius + FVector(0.f,0.f,CylinderComponent->CollisionHeight), SrcLocation, TRACE_World|TRACE_StopAtAnyHit, FVector(0.f,0.f,0.f))
				|| GWorld->SingleLineCheck( Hit, this, Location - Y*(0.5f + 0.5*appFrand())*CylinderComponent->CollisionRadius + FVector(0.f,0.f,CylinderComponent->CollisionHeight), SrcLocation, TRACE_World|TRACE_StopAtAnyHit, FVector(0.f,0.f,0.f)) )
			{
				return CacheNetRelevancy(true,RealViewer,Viewer);
			}
		}				
		return CacheNetRelevancy(false,RealViewer,Viewer);
	}
}

INT UWorld::ServerTickClients( FLOAT DeltaSeconds )
{
	if ( NetDriver->ClientConnections.Num() == 0 )
		return 0;
	INT Updated=0;

	INT NetRelevantActorCount = FActorIteratorBase::GetNetRelevantActorCount() + 2;

	FMemMark Mark(GMem);
	// initialize connections
	for( INT i=NetDriver->ClientConnections.Num()-1; i>=0; i-- )
	{
		UNetConnection* Connection = NetDriver->ClientConnections(i);
		check(Connection);
		check(Connection->State == USOCK_Pending || Connection->State == USOCK_Open || Connection->State == USOCK_Closed);
		checkSlow(Connection->GetUChildConnection() == NULL);

		// Handle not ready channels.
		//@note: we cannot add Connection->IsNetReady(0) here to check for saturation, as if that's the case we still want to figure out the list of relevant actors
		//			to reset their NetUpdateTime so that they will get sent as soon as the connection is no longer saturated
		if (Connection->Actor != NULL && Connection->State == USOCK_Open && (Connection->Driver->Time - Connection->LastReceiveTime < 1.5f))
		{
			Connection->Viewer = Connection->Actor->GetViewTarget();
			Connection->OwnedConsiderList = new(GMem,NetRelevantActorCount)AActor*;
			Connection->OwnedConsiderListSize = 0;

			for (INT j = 0; j < Connection->Children.Num(); j++)
			{
				if (Connection->Children(j)->Actor != NULL)
				{
					Connection->Children(j)->Viewer = Connection->Children(j)->Actor->GetViewTarget();
					Connection->Children(j)->OwnedConsiderList = new(GMem, NetRelevantActorCount) AActor*;
					Connection->Children(j)->OwnedConsiderListSize = 0;
				}
				else
				{
					Connection->Children(j)->Viewer = NULL;
				}
			}
		}
		else
		{
			Connection->Viewer = NULL;
			for (INT j = 0; j < Connection->Children.Num(); j++)
			{
				Connection->Children(j)->Viewer = NULL;
			}
		}
	}

	// make list of actors to consider
	AActor **ConsiderList = new(GMem,NetRelevantActorCount)AActor*;
	INT ConsiderListSize = 0;

	// Add WorldInfo to considerlist
	AWorldInfo* WorldInfo = GetWorldInfo();
	if (WorldInfo->RemoteRole != ROLE_None)
	{
		ConsiderList[0] = WorldInfo;
		ConsiderListSize++;
	}

	UBOOL bCPUSaturated		= FALSE;
	FLOAT ServerTickTime	= GEngine->GetMaxTickRate( DeltaSeconds );
	if ( ServerTickTime == 0.f )
	{
		ServerTickTime = DeltaSeconds;
	}
	else
	{
		ServerTickTime	= 1.f/ServerTickTime;
		bCPUSaturated	= DeltaSeconds > 1.2f * ServerTickTime;
	}

	//debugfSuppressed(NAME_DevNetTraffic, TEXT("UWorld::ServerTickClients, Building ConsiderList %4.2f"), GWorld->GetTimeSeconds());

	for( FNetRelevantActorIterator It; It; ++It )
	{
		AActor* Actor = *It;
		if( (Actor->RemoteRole!=ROLE_None) && (WorldInfo->TimeSeconds > Actor->NetUpdateTime) ) 
		{
			Actor->NetUpdateTime = WorldInfo->TimeSeconds + appSRand() * ServerTickTime + 1.f/Actor->NetUpdateFrequency; // FIXME - cache 1/netupdatefreq
		
			if ( Actor->bAlwaysRelevant || !Actor->bOnlyRelevantToOwner ) 
			{
				ConsiderList[ConsiderListSize] = Actor;
				ConsiderListSize++;
			}
			else
			{
				AActor* ActorOwner = Actor->Owner;
				if ( !ActorOwner && (Actor->GetAPlayerController() || Actor->GetAPawn()) ) 
				{
					ActorOwner = Actor;
				}
				if ( ActorOwner )
				{
					for ( INT j=0; j<NetDriver->ClientConnections.Num(); j++ )
					{
						UNetConnection* ClientConnection = NetDriver->ClientConnections(j);
						UNetConnection* Connection = ClientConnection;
						INT ChildIndex = 0;
						UBOOL bCloseChannel = TRUE;
						while (Connection != NULL)
						{
							if (Connection->Viewer != NULL)
							{
								if (ActorOwner == Connection->Actor || ActorOwner == Connection->Actor->Pawn || Connection->Viewer->IsRelevancyOwnerFor(Actor, ActorOwner))
								{
									Connection->OwnedConsiderList[Connection->OwnedConsiderListSize] = Actor;
									Connection->OwnedConsiderListSize++;
									bCloseChannel = FALSE;
								}
							}
							else
							{
								// don't ever close the channel if one or more child connections don't have a Viewer to check relevancy with
								bCloseChannel = FALSE;
							}

							Connection = (ChildIndex < ClientConnection->Children.Num()) ? ClientConnection->Children(ChildIndex++) : NULL;
						}
						// if it's not being considered, but there is an open channel for this actor already, close it
						if (bCloseChannel)
						{
							UActorChannel* Channel = ClientConnection->ActorChannels.FindRef(Actor);
							if (Channel != NULL && NetDriver->Time - Channel->RelevantTime >= NetDriver->RelevantTimeout)
							{
								Channel->Close();
							}
						}
					}
				}
			}
		}
		/*
		else
		{
			if( Actor->GetAPawn() && (Actor->RemoteRole!=ROLE_None) && (GWorld->GetTimeSeconds() <= Actor->NetUpdateTime) ) 
			{
				debugfSuppressed(NAME_DevNetTraffic, TEXT("%s skipped in considerlist because of NetUpdateTime %f"), *Actor->GetName(), (GWorld->GetTimeSeconds() - Actor->NetUpdateTime) );
			}
		}
		*/
	}

	for( INT i=NetDriver->ClientConnections.Num()-1; i>=0; i-- )
	{
		UNetConnection* Connection = NetDriver->ClientConnections(i);

		if (Connection->Viewer)
		{
			// Get list of visible/relevant actors.
			FLOAT PruneActors = 0.f;
			clock(PruneActors);
			FMemMark RelevantActorMark(GMem);
			NetTag++;
			Connection->TickCount++;

			// Set up to skip all sent temporary actors.
			for( INT j=0; j<Connection->SentTemporaries.Num(); j++ )
			{
				Connection->SentTemporaries(j)->NetTag = NetTag;
			}

			TArray<FNetViewer>& ConnectionViewers = WorldInfo->ReplicationViewers;
			ConnectionViewers.Reset();
			new(ConnectionViewers) FNetViewer(Connection, DeltaSeconds);
			for (INT j = 0; j < Connection->Children.Num(); j++)
			{
				if (Connection->Children(j)->Viewer != NULL)
				{
					new(ConnectionViewers) FNetViewer(Connection->Children(j), DeltaSeconds);
				}
			}

			// Make list of all actors to consider.
			INT					ConsiderCount	= 0;
			INT					NetRelevantCount = FActorIteratorBase::GetNetRelevantActorCount();
			FActorPriority* PriorityList = new(GMem,NetRelevantCount+2)FActorPriority;
			FActorPriority** PriorityActors = new(GMem,NetRelevantCount+2)FActorPriority*;
			UBOOL bLowNetBandwidth = !bCPUSaturated && (Connection->CurrentNetSpeed/FLOAT(WorldInfo->Game->NumPlayers + GWorld->GetGameInfo()->NumBots) < (WorldInfo->Game->bAllowVehicles ? 500.f : 300.f) );
			UBOOL bPrioritySort = FALSE;
			for (INT j = 0; j < ConnectionViewers.Num(); j++)
			{
				bPrioritySort = bPrioritySort || ConnectionViewers(j).InViewer->bWasSaturated;
				ConnectionViewers(j).InViewer->bWasSaturated = ConnectionViewers(j).InViewer->bWasSaturated && bLowNetBandwidth;
			}

			for( INT j=0; j<ConsiderListSize; j++ )
			{
				AActor* Actor = ConsiderList[j];
				if( Actor->NetTag!=NetTag )
				{
					//debugf(TEXT("Consider %s alwaysrelevant %d frequency %f "),*Actor->GetName(), Actor->bAlwaysRelevant, Actor->NetUpdateFrequency);
					UActorChannel* Channel = Connection->ActorChannels.FindRef(Actor);
					if( Actor->bOnlyDirtyReplication 
					&&	Channel 
					&&	!Channel->ActorDirty 
					&&	Channel->Recent.Num() 
					&&	Channel->Dirty.Num() == 0 
					)
					{
						Channel->RelevantTime = NetDriver->Time;
					}
					else
					{
						Actor->NetTag                 = NetTag;
						PriorityList  [ConsiderCount] = FActorPriority(Connection, Channel, Actor, ConnectionViewers, bLowNetBandwidth);
						PriorityActors[ConsiderCount] = PriorityList + ConsiderCount;
						ConsiderCount++;
					}
				}
			}

			UNetConnection* NextConnection = Connection;
			INT ChildIndex = 0;
			while (NextConnection != NULL)
			{
				for (INT j = 0; j < NextConnection->OwnedConsiderListSize; j++)
				{
					AActor* Actor = NextConnection->OwnedConsiderList[j];
					//debugf(TEXT("Consider owned %s always relevant %d frequency %f  "),Actor->GetName(), Actor->bAlwaysRelevant,Actor->NetUpdateFrequency);
					if (Actor->NetTag != NetTag)
					{
						UActorChannel* Channel = Connection->ActorChannels.FindRef(Actor);
						if( Actor->bOnlyDirtyReplication
						&&	Channel
						&&	!Channel->ActorDirty
						&&	Channel->Recent.Num()
						&&	Channel->Dirty.Num() == 0 )
						{
							Channel->RelevantTime = NetDriver->Time;
						}
						else
						{
							Actor->NetTag                 = NetTag;
							PriorityList  [ConsiderCount] = FActorPriority(NextConnection, Channel, Actor, ConnectionViewers, bLowNetBandwidth);
							PriorityActors[ConsiderCount] = PriorityList + ConsiderCount;
							ConsiderCount++;
						}
					}
				}
				NextConnection->OwnedConsiderList = NULL;
				NextConnection->OwnedConsiderListSize = 0;

				NextConnection = (ChildIndex < Connection->Children.Num()) ? Connection->Children(ChildIndex++) : NULL;
			}

			FLOAT RelevantTime = 0.f;

			// Sort by priority, if network connection is saturated
			if ( bPrioritySort )
			{
				Sort<USE_COMPARE_POINTER(FActorPriority,UnLevTic)>( PriorityActors, ConsiderCount );
			}

			// Update all relevant actors in sorted order.
			INT j;
			UBOOL bNewSaturated = false;
			//debugf(TEXT("START"));
			for( j=0; j<ConsiderCount; j++ )
			{
				UActorChannel* Channel     = PriorityActors[j]->Channel;
				//debugf(TEXT(" Maybe Replicate %s"),*PriorityActors[j]->Actor->GetName());
				if ( !Channel || Channel->Actor ) //make sure didn't just close this channel
				{
					AActor*        Actor       = PriorityActors[j]->Actor;
					UBOOL          CanSee      = 0;

					// only check visibility on already visible actors every 1.0 + 0.5R seconds
					// bTearOff actors should never be checked
					if ( !Actor->bTearOff && (!Channel || NetDriver->Time-Channel->RelevantTime>1.f) )
					{
						for (INT k = 0; k < ConnectionViewers.Num(); k++)
						{
							if (Actor->IsNetRelevantFor(ConnectionViewers(k).InViewer, ConnectionViewers(k).Viewer, ConnectionViewers(k).ViewLocation))
							{
								CanSee = TRUE;
								break;
							}
						}
					}
					
					if( CanSee || (Channel && NetDriver->Time-Channel->RelevantTime<NetDriver->RelevantTimeout) )
					{	
						// Find or create the channel for this actor.
						// we can't create the channel if the client is in a different world than we are
						// or the package map doesn't support the actor's class/archetype (or the actor itself in the case of serializable actors)
						// or it's a static actor and the client hasn't initialized the level it's in
						if ( Channel == NULL && Connection->PackageMap->SupportsObject(Actor->GetClass()) &&
								Connection->PackageMap->SupportsObject((Actor->bStatic || Actor->bNoDelete) ? Actor : Actor->GetArchetype()) )
						{
							if ( Connection->ClientWorldPackageName == GWorld->GetOutermost()->GetFName()
								&& ((!Actor->bStatic && !Actor->bNoDelete) || Connection->ClientHasInitializedLevelFor(Actor)) )
							{
								// Create a new channel for this actor.
								Channel = (UActorChannel*)Connection->CreateChannel( CHTYPE_Actor, 1 );
								if( Channel )
								{
									Channel->SetChannelActor( Actor );
								}
							}
							// if we couldn't replicate it for a reason that should be temporary, and this Actor is updated very infrequently, make sure we update it again soon
							else if (Actor->NetUpdateFrequency < 1.0f)
							{
								Actor->NetUpdateTime = WorldInfo->TimeSeconds + 0.2f * appFrand();
							}
						}

						if( Channel )
						{
							if (!Connection->IsNetReady(0))
							{
								bNewSaturated = true;
								break;
							}
							if( CanSee )
							{
								Channel->RelevantTime = NetDriver->Time + 0.5f * appSRand();
							}
							if( Channel->IsNetReady(0) )
							{
								//debugf(TEXT("Replicate %s"),*Actor->GetName());
								Channel->ReplicateActor();
								Updated++;
							}
							else
							{							
								Actor->NetUpdateTime = WorldInfo->TimeSeconds - 1.f;
							}
							if (!Connection->IsNetReady(0))
							{
								bNewSaturated = true;
								break;
							}
						}
					}
					else if( Channel )
					{
						Channel->Close();

						// streamingServer
						///////////////////Actor->Attached can be null
						// attachments may not be relevant anymore either
						for ( INT k=0; k<Actor->Attached.Num(); k++ )
						{
							if ( Actor->Attached(k) && !Actor->Attached(k)->bAlwaysRelevant 
								&& (Actor->Attached(k)->BaseSkelComponent || ((Actor == Actor->Attached(k)->Owner) && !Actor->Attached(k)->bOnlyOwnerSee)) )
							{
								UChannel *AttachedChannel = Connection->ActorChannels.FindRef(Actor->Attached(k));
								if ( AttachedChannel )
									AttachedChannel->Close();
							}
						}
					}
				}
			}
			for (INT k = 0; k < ConnectionViewers.Num(); k++)
			{
				ConnectionViewers(k).InViewer->bWasSaturated = bNewSaturated;
			}

			// relevant actors that could not be processed this frame are marked to be considered for next frame
			for ( INT k=j; k<ConsiderCount; k++ )
			{
				AActor* Actor = PriorityActors[k]->Actor;
				UActorChannel* Channel = PriorityActors[k]->Channel;
				
				debugfSuppressed(NAME_DevNetTraffic, TEXT("Saturated. %s"), *Actor->GetName());
				if (Channel != NULL && NetDriver->Time - Channel->RelevantTime <= 1.f)
				{
					//debugfSuppressed(NAME_DevNetTraffic, TEXT(" Saturated. Mark %s NetUpdateTime to be checked for next tick"), *Actor->GetName());
					Actor->NetUpdateTime = WorldInfo->TimeSeconds - 1.f;
				}
				else
				{
					for (INT h = 0; h < ConnectionViewers.Num(); h++)
					{
						if (Actor->IsNetRelevantFor(ConnectionViewers(h).InViewer, ConnectionViewers(h).Viewer, ConnectionViewers(h).ViewLocation))
						{
							//debugfSuppressed(NAME_DevNetTraffic, TEXT(" Saturated. Mark %s NetUpdateTime to be checked for next tick"), *Actor->GetName());
							Actor->NetUpdateTime = WorldInfo->TimeSeconds - 1.f;
							if (Channel != NULL)
							{
								Channel->RelevantTime = NetDriver->Time + 0.5f * appSRand();
							}
							break;
						}
					}
				}
			}
			RelevantActorMark.Pop();
			unclock(RelevantTime);
			//debugf(TEXT("Potential %04i ConsiderList %03i ConsiderCount %03i Prune=%01.4f Relevance=%01.4f"),NetRelevantCount, 
			//			ConsiderListSize, ConsiderCount, PruneActors * GSecondsPerCycle * 1000.f,RelevantTime * GSecondsPerCycle * 1000.f);
		}
	}
	Mark.Pop();
	return Updated;
}

/*-----------------------------------------------------------------------------
	Network server tick.
-----------------------------------------------------------------------------*/

void UNetConnection::SetActorDirty(AActor* DirtyActor )
{
	if( Actor && State==USOCK_Open )
	{
		UActorChannel* Channel = ActorChannels.FindRef(DirtyActor);
		if ( Channel )
			Channel->ActorDirty = true;
	}
}

void UWorld::TickNetServer( FLOAT DeltaSeconds )
{
	// Update all clients.
	INT Updated=0;

	// first, set which channels have dirty actors (need replication)
	AActor* WorldInfoActor = GetWorldInfo();
	if( WorldInfoActor && WorldInfoActor->bNetDirty )
	{
		for( INT j=NetDriver->ClientConnections.Num()-1; j>=0; j-- )
			NetDriver->ClientConnections(j)->SetActorDirty(WorldInfoActor);
		if (DemoRecDriver != NULL && DemoRecDriver->ClientConnections.Num() > 0)
		{
			DemoRecDriver->ClientConnections(0)->SetActorDirty(WorldInfoActor);
		}
		WorldInfoActor->bNetDirty = 0;
	}

	for( FNetRelevantActorIterator It; It; ++It )
	{
		AActor* Actor = *It;
		if( Actor && Actor->bNetDirty )
		{
			if ( Actor->RemoteRole != ROLE_None )
			{
				for( INT j=NetDriver->ClientConnections.Num()-1; j>=0; j-- )
					NetDriver->ClientConnections(j)->SetActorDirty(Actor);
				if (DemoRecDriver != NULL && DemoRecDriver->ClientConnections.Num() > 0)
				{
					DemoRecDriver->ClientConnections(0)->SetActorDirty(Actor);
				}
			}
			Actor->bNetDirty = 0;
		}
	}
	Updated = ServerTickClients( DeltaSeconds );

	// Log message.
	if( appTrunc(GWorld->GetTimeSeconds()-DeltaSeconds)!=appTrunc(GWorld->GetTimeSeconds()) )
	{
		debugf( NAME_Title, *LocalizeProgress(TEXT("RunningNet"),TEXT("Engine")), *GetWorldInfo()->Title, *URL.Map, NetDriver->ClientConnections.Num() );
	}
}

/*-----------------------------------------------------------------------------
Demo Recording tick.
-----------------------------------------------------------------------------*/

static void DemoReplicateActor(AActor* Actor, UNetConnection* Connection, UBOOL IsNetClient)
{
	// All replicatable actors are assumed to be relevant for demo recording.
	if
		(	Actor
		&&	( (IsNetClient && Actor->bTearOff) || Actor->RemoteRole!=ROLE_None || (IsNetClient && Actor->Role!=ROLE_None && Actor->Role != ROLE_Authority))
		&&  (!Actor->bNetTemporary || Connection->SentTemporaries.FindItemIndex(Actor)==INDEX_NONE)
		// @todo: WTF
		&&  (Actor->bStatic || !Actor->GetClass()->GetDefaultActor()->bStatic)
		//@todo: FIXME: UActorChannel::ReceivedBunch() currently doesn't handle receiving non-owned PCs (tries to hook them up to a splitscreen viewport). Do demos need those other PCs in them?
		&& (Actor == Connection->Actor || Actor->GetAPlayerController() == NULL)
		)
	{
		// Create a new channel for this actor.
		UActorChannel* Channel = Connection->ActorChannels.FindRef( Actor );
		if( !Channel && Connection->PackageMap->SupportsObject(Actor->GetClass()) &&
			Connection->PackageMap->SupportsObject((Actor->bStatic || Actor->bNoDelete) ? Actor : Actor->GetArchetype()))
		{
			// Check we haven't run out of actor channels.
			Channel = (UActorChannel*)Connection->CreateChannel( CHTYPE_Actor, 1 );
			check(Channel);
			Channel->SetChannelActor( Actor );
		}
		if( Channel )
		{
			// Send it out!
			check(!Channel->Closing);
			if( Channel->IsNetReady(0) )
			{
				Actor->bDemoRecording = 1;
#if CLIENT_DEMO
				Actor->bClientDemoRecording = IsNetClient;
#endif
				UBOOL TornOff = 0;
				if(IsNetClient)
				{
					if( Actor->bTearOff && Actor->Role==ROLE_Authority && Actor->RemoteRole==ROLE_None )
					{
						TornOff = 1;
						Actor->RemoteRole = ROLE_SimulatedProxy;
					}
					else
						Exchange(Actor->RemoteRole, Actor->Role);
				}
				Channel->ReplicateActor();
				if(IsNetClient)
				{
					if( TornOff )
						Actor->RemoteRole = ROLE_None;
					else
						Exchange(Actor->RemoteRole, Actor->Role);
				}
				Actor->bDemoRecording = 0;
#if CLIENT_DEMO
				Actor->bClientDemoRecording = 0;
#endif
			}
		}
	}
}

INT UWorld::TickDemoRecord( FLOAT DeltaSeconds )
{
	UNetConnection* Connection = DemoRecDriver->ClientConnections(0);
	UBOOL IsNetClient = (GetNetMode() == NM_Client);
	DemoReplicateActor(GetWorldInfo(), Connection, IsNetClient);
	for (FNetRelevantActorIterator It; It; ++It)
	{
		DemoReplicateActor(*It, Connection, IsNetClient);
	}
	return 1;
}

INT UWorld::TickDemoPlayback( FLOAT DeltaSeconds )
{
	if (GEngine->TransitionType==TT_Connecting && DemoRecDriver->ServerConnection->State != USOCK_Pending)
	{
		GEngine->TransitionType = TT_None;
		GEngine->SetProgress( TEXT(""),  TEXT(""), 0.0f );
	} 
	if( DemoRecDriver->ServerConnection->State==USOCK_Closed && DemoRecDriver->PlayCount==0 )
	{
		// Demo stopped playing
		GEngine->SetClientTravel( TEXT("?disconnect"), TRAVEL_Absolute );
	}

	// @todo demo: Somewhere handle playback with rendering disabled for timedemo?
	return 1;
}

/*-----------------------------------------------------------------------------
	Main level timer tick handler.
-----------------------------------------------------------------------------*/
UBOOL UWorld::IsPaused()
{
	// pause if specifically set or if we're waiting for the end of the tick to perform streaming level loads (so actors don't fall through the world in the meantime, etc)
	return ( (GetWorldInfo()->Pauser != NULL && GetTimeSeconds() >= GetWorldInfo()->PauseDelay) ||
				GetWorldInfo()->bRequestedBlockOnAsyncLoading ||
				(GEngine->IsA(UGameEngine::StaticClass()) && ((UGameEngine*)GEngine)->bShouldCommitPendingMapChange) );
}

/**
 * Ticks a set of deferred components
 *
 * @param DeltaSeconds - the amount of time that has elapsed since last frame
 * @param DeferredList - the list holding all of the deferred tickable items
 */
template<typename ITER> void TickDeferredComponents(FLOAT DeltaSeconds,
	FDeferredTickList& DeferredList)
{
	// Iterate through the list of components
	for (ITER It(DeferredList); It; ++It)
	{
		// Don't tick a component whose owner is going away
		if (It->IsPendingKill() == FALSE)
		{
			It->ConditionalTick(DeltaSeconds);
			debugfSlow(NAME_DevTick,TEXT("Ticked deferred component (%s) in group (%d)"),
				*It->GetName(),(INT)GWorld->TickGroup);
		}
	}
}

/**
 * Ticks the set of components for a given actor
 *
 * @param Actor - the actor to tick the components of
 * @param DeltaSeconds - time in seconds since last tick
 * @param TickType - type of tick (viewports only, time only, etc)
 * @param DeferredList - The list object that manages deferred ticking
 */
extern UBOOL GShouldLogOutAFrameOfSkelCompTick;

void TickActorComponents(AActor* Actor,FLOAT DeltaSeconds,ELevelTick TickType,
	FDeferredTickList* DeferredList)
{
#if STATS
	const DWORD Counter = (DWORD)STAT_PreAsyncComponentTickTime - GWorld->TickGroup;
	SCOPE_CYCLE_COUNTER(Counter);
#endif
	const UBOOL bShouldTick = ((TickType != LEVELTICK_ViewportsOnly) ||
		Actor->PlayerControlled());
	// Update components. We do this after the position has been updated so 
	// stuff like animation can update using the new position.
	for (INT ComponentIndex = 0; ComponentIndex < Actor->AllComponents.Num();
		ComponentIndex++)
	{
		UActorComponent* ActorComp = Actor->AllComponents(ComponentIndex);
		if (ActorComp != NULL)
		{
			if (bShouldTick ||
				(ActorComp->bTickInEditor && !GWorld->HasBegunPlay()))
			{
				// Don't tick this component if it was deferred until later
				if (DeferredList == NULL ||
					DeferredList->ConditionalDefer(ActorComp) == FALSE)
				{
#if STATS
					const DWORD Counter2 = (DWORD)STAT_PreAsyncComponentsTicked - GWorld->TickGroup;
					INC_DWORD_STAT(Counter2);
#endif
					// Tick the component
					ActorComp->ConditionalTick(DeltaSeconds);
					// Log it for debugging
					debugfSlow(NAME_DevTick,TEXT("Ticked component (%s) in group (%d)"),
						*ActorComp->GetName(),(INT)GWorld->TickGroup);
				}
			}
		}
	}
}

/**
 * Ticks the newly spawned actors. NOTE: Newly spawned actors
 * have all of their components ticked without deferral
 *
 * @param World the world being ticked
 * @param DeltaSeconds - time in seconds since last tick
 * @param TickType - type of tick (viewports only, time only, etc)
 */
static void TickNewlySpawned(UWorld* World,FLOAT DeltaSeconds,
	ELevelTick TickType)
{
	// Tick any actors that were spawned during the ticking. Array might grow during ticking!
	for( INT NewlySpawnedIndex=0; NewlySpawnedIndex<World->NewlySpawned.Num(); NewlySpawnedIndex++ )
	{
		AActor* Actor = World->NewlySpawned(NewlySpawnedIndex);
		if( Actor 
		&&	Actor->bTicked != (DWORD)World->Ticked 
		&&	!Actor->ActorIsPendingKill() )
		{			
			checkf(!Actor->HasAnyFlags(RF_Unreachable), TEXT("%s"), *Actor->GetFullName());
			UBOOL bTicked = Actor->Tick(DeltaSeconds*Actor->CustomTimeDilation,TickType);
			// If this actor actually ticked, ticks it's components
			if (bTicked == TRUE)
			{
				debugfSlow(NAME_DevTick,TEXT("Ticked newly spawned (%s) in group (%d)"),
					*Actor->GetName(),(INT)GWorld->TickGroup);
				TickActorComponents(Actor,DeltaSeconds*Actor->CustomTimeDilation,TickType,NULL);
			}
		}
	}
	World->NewlySpawned.Empty();
}

/**
 * Defers a list of newly spawned actors until post async work
 *
 * @param World the world we're operating on
 * @param DeferredList - The list object that manages deferred ticking
 */
void DeferNewlySpawned(UWorld* World,FDeferredTickList& DeferredList)
{
	// Add each newly spawned/deferred actor to the post async work list
	for( INT NewlySpawnedIndex=0; NewlySpawnedIndex<World->NewlySpawned.Num(); NewlySpawnedIndex++ )
	{
		AActor* Actor = World->NewlySpawned(NewlySpawnedIndex);
		if( Actor
		&&	Actor->bTicked != (DWORD)World->Ticked
		&&	!Actor->ActorIsPendingKill() )
		{
			debugfSlow(NAME_DevTick,TEXT("Deferring newly spawned actor (%s) in group (%d)"),
				*Actor->GetName(),(INT)GWorld->TickGroup);
			DeferredList.AddNewlySpawned(Actor);
		}
	}
	World->NewlySpawned.Empty();
}

/**
 * Ticks the world's dynamic actors based upon their tick group. This function
 * is called once for each ticking group
 *
 * @param World - The being ticked
 * @param DeltaSeconds - time in seconds since last tick
 * @param TickType - type of tick (viewports only, time only, etc)
 * @param DeferredList - The list object that manages deferred ticking
 */
template<typename ITER> void TickActors(UWorld* World,FLOAT DeltaSeconds,
	ELevelTick TickType,FDeferredTickList& DeferredList)
{
#if STATS
	const DWORD Counter = (DWORD)STAT_PreAsyncTickTime - World->TickGroup;
	SCOPE_CYCLE_COUNTER(Counter);
#endif
	World->NewlySpawned.Empty();
	// Use the specified iterator to iterate through the list of actors
	// that should be ticked (ticking group dependent)
	for (ITER It(DeferredList); It; ++It)
	{
		AActor* Actor = *It;
		// Tick this actor if it isn't dead and it isn't being deferred
		if (Actor->ActorIsPendingKill() == FALSE &&
			DeferredList.ConditionalDefer(Actor) == FALSE)
		{
			checkf(!Actor->HasAnyFlags(RF_Unreachable), TEXT("%s"), *Actor->GetFullName());
			const UBOOL bTicked = Actor->Tick(DeltaSeconds*Actor->CustomTimeDilation,TickType);
			// If this actor actually ticked, ticks it's components
			if (bTicked == TRUE)
			{
				debugfSlow(NAME_DevTick,TEXT("Ticked actor (%s) in group (%d)"),
					*Actor->GetName(),(INT)GWorld->TickGroup);
#if STATS
				const DWORD Counter2 = (DWORD)STAT_PreAsyncActorsTicked - World->TickGroup;
				INC_DWORD_STAT(Counter2);
#endif
				TickActorComponents(Actor,DeltaSeconds,TickType,&DeferredList);
			}
		}
	}

	// If an actor was spawned during the async work, tick it in the post
	// async work, so that it doesn't try to interact with the async threads
	if (World->TickGroup == TG_DuringAsyncWork)
	{
		DeferNewlySpawned(World,DeferredList);
	}
	else
	{
		TickNewlySpawned(World,DeltaSeconds,TickType);
	}

}

/**
 * Global instance of our deferred list. It's global so that we can minimize
 * the allocating/freeing of memory per frame
 */
FDeferredTickList GDeferredList;

/**
 * Ticks any of our async worker threads (notifies them of their work to do)
 *
 * @param DeltaSeconds - The elapsed time between frames
 */
void UWorld::TickAsyncWork(FLOAT DeltaSeconds)
{
	// Currently just tick physics
	TickWorldRBPhys(DeltaSeconds);
	//@todo joeg -- Kick off async line/box check thread
	//@todo joeg -- Kick off async pathing thread
}

/**
 * Waits for any async work that needs to be done before continuing
 */
void UWorld::WaitForAsyncWork(void)
{
//	SCOPE_CYCLE_COUNTER(STAT_AsyncWorkWaitTime);
	// Block until physics is done
	WaitWorldRBPhys();
	//@todo joeg -- Figure out if we need to block on other things here
}

/**
 * Streaming settings for levels which are detrmined visible by level streaming volumes.
 */
class FVisibleLevelStreamingSettings
{
public:
	FVisibleLevelStreamingSettings()
	{
		bShouldBeVisible		= FALSE;
		bShouldBlockOnLoad		= FALSE;
		bShouldChangeVisibility	= FALSE;
	}

	FVisibleLevelStreamingSettings( EStreamingVolumeUsage Usage )
	{
		switch( Usage )
		{
		case SVB_Loading:
			bShouldBeVisible		= FALSE;
			bShouldBlockOnLoad		= FALSE;
			bShouldChangeVisibility	= FALSE;
			break;
		case SVB_LoadingNotVisible:
			bShouldBeVisible		= FALSE;
			bShouldBlockOnLoad		= FALSE;
			bShouldChangeVisibility	= TRUE;
			break;
		case SVB_LoadingAndVisibility:
			bShouldBeVisible		= TRUE;
			bShouldBlockOnLoad		= FALSE;
			bShouldChangeVisibility	= TRUE;
			break;
		case SVB_VisibilityBlockingOnLoad:
			bShouldBeVisible		= TRUE;
			bShouldBlockOnLoad		= TRUE;
			bShouldChangeVisibility	= TRUE;
			break;
		case SVB_BlockingOnLoad:
			bShouldBeVisible		= FALSE;
			bShouldBlockOnLoad		= TRUE;
			bShouldChangeVisibility	= FALSE;
			break;
		default:
			appErrorf(TEXT("Unsupported usage %i"),(INT)Usage);
		}
	}

	FVisibleLevelStreamingSettings& operator|=(const FVisibleLevelStreamingSettings& B)
	{
		bShouldBeVisible		|= B.bShouldBeVisible;
		bShouldBlockOnLoad		|= B.bShouldBlockOnLoad;
		bShouldChangeVisibility	|= B.bShouldChangeVisibility;
		return *this;
	}

	UBOOL AllSettingsEnabled() const
	{
		return bShouldBeVisible && bShouldBlockOnLoad;
	}

	UBOOL ShouldBeVisible( UBOOL bCurrentShouldBeVisible ) const
	{
		if( bShouldChangeVisibility )
		{
			return bShouldBeVisible;
		}
		else
		{
			return bCurrentShouldBeVisible;
		}
	}

	UBOOL ShouldBlockOnLoad() const
	{
		return bShouldBlockOnLoad;
	}

private:
	/** Whether level should be visible.						*/
	UBOOL bShouldBeVisible;
	/** Whether level should block on load.						*/
	UBOOL bShouldBlockOnLoad;
	/** Whether existing visibility settings should be changed. */
	UBOOL bShouldChangeVisibility;
};

/**
 * Issues level streaming load/unload requests based on whether
 * players are inside/outside level streaming volumes.
 */
void UWorld::ProcessLevelStreamingVolumes(FVector* OverrideViewLocation)
{
	// if we are delaying using streaming volumes, return now
	if( StreamingVolumeUpdateDelay > 0 )
	{
		StreamingVolumeUpdateDelay--;
		return;
	}
	// Option to skip indefinitely.
	else if( StreamingVolumeUpdateDelay == INDEX_NONE )
	{
		return;
	}

	SCOPE_CYCLE_COUNTER( STAT_VolumeStreamingTickTime );

	// Begin by assembling a list of kismet streaming objects that have non-EditorPreVisOnly volumes associated with them.
	// @todo DB: Cache this, e.g. level startup.
	TArray<ULevelStreaming*> LevelStreamingObjectsWithVolumes;
	TMap<ULevelStreaming*,UBOOL> LevelStreamingObjectsWithVolumesOtherThanBlockingLoad;
	AWorldInfo* WorldInfo = GetWorldInfo();
	for( INT LevelIndex = 0 ; LevelIndex < WorldInfo->StreamingLevels.Num() ; ++LevelIndex )
	{
		ULevelStreaming* LevelStreamingObject = WorldInfo->StreamingLevels(LevelIndex);
		if( LevelStreamingObject )
		{
			for ( INT i = 0 ; i < LevelStreamingObject->EditorStreamingVolumes.Num() ; ++i )
			{
				ALevelStreamingVolume* StreamingVolume = LevelStreamingObject->EditorStreamingVolumes(i);
				if( StreamingVolume 
				&& !StreamingVolume->bEditorPreVisOnly 
				&& !StreamingVolume->bDisabled )
				{
					LevelStreamingObjectsWithVolumes.AddItem( LevelStreamingObject );
					if( StreamingVolume->Usage != SVB_BlockingOnLoad )
					{
						LevelStreamingObjectsWithVolumesOtherThanBlockingLoad.Set( LevelStreamingObject, TRUE );
					}
					break;
				}
			}
		}
	}

	// The set of levels with volumes whose volumes current contain player viewpoints.
	TMap<ULevelStreaming*,FVisibleLevelStreamingSettings> VisibleLevelStreamingObjects;

	// Iterate over all players and build a list of level streaming objects with
	// volumes that contain player viewpoints.
	for (AController* C = WorldInfo->ControllerList; C != NULL; C = C->NextController)
	{
		APlayerController* PlayerActor = C->GetAPlayerController();

		if (PlayerActor != NULL && PlayerActor->bIsUsingStreamingVolumes)
		{
			FVector ViewLocation;
			// let the caller override the location to check for volumes
			if (OverrideViewLocation)
			{
				ViewLocation = *OverrideViewLocation;
			}
			else
			{
				FRotator ViewRotation;
				PlayerActor->eventGetPlayerViewPoint( ViewLocation, ViewRotation );
			}

			TMap<ALevelStreamingVolume*,UBOOL> VolumeMap;

			// Iterate over streaming levels with volumes and compute whether the
			// player's ViewLocation is in any of their volumes.
			for( INT LevelIndex = 0 ; LevelIndex < LevelStreamingObjectsWithVolumes.Num() ; ++LevelIndex )
			{
				ULevelStreaming* LevelStreamingObject = LevelStreamingObjectsWithVolumes( LevelIndex );

				// StreamingSettings is an OR of all level streaming settings of volumes containing player viewpoints.
				FVisibleLevelStreamingSettings StreamingSettings;

				// See if level streaming settings were computed for other players.
				FVisibleLevelStreamingSettings* ExistingStreamingSettings = VisibleLevelStreamingObjects.Find( LevelStreamingObject );
				if ( ExistingStreamingSettings )
				{
					// Stop looking for viewpoint-containing volumes once all streaming settings have been enabled for the level.
					if ( ExistingStreamingSettings->AllSettingsEnabled() )
					{
						continue;
					}

					// Initialize the level's streaming settings with settings that were computed for other players.
					StreamingSettings = *ExistingStreamingSettings;
				}

				// For each streaming volume associated with this level . . .
				for ( INT i = 0 ; i < LevelStreamingObject->EditorStreamingVolumes.Num() ; ++i )
				{
					ALevelStreamingVolume* StreamingVolume = LevelStreamingObject->EditorStreamingVolumes(i);
					if ( StreamingVolume && !StreamingVolume->bEditorPreVisOnly && !StreamingVolume->bDisabled )
					{
						UBOOL bViewpointInVolume;
						UBOOL* bResult = VolumeMap.Find(StreamingVolume);
						if ( bResult )
						{
							// This volume has already been considered for another level.
							bViewpointInVolume = *bResult;
						}
						else
						{
							// Compute whether the viewpoint is inside the volume and cache the result.
							bViewpointInVolume = StreamingVolume->Encompasses( ViewLocation );
							VolumeMap.Set( StreamingVolume, bViewpointInVolume );
							INC_DWORD_STAT( STAT_VolumeStreamingChecks );
						}

						if ( bViewpointInVolume )
						{
							// Copy off the streaming settings for this volume.
							StreamingSettings |= FVisibleLevelStreamingSettings( (EStreamingVolumeUsage) StreamingVolume->Usage );

							// Update the streaming settings for the level.
							// This also marks the level as "should be loaded".
							VisibleLevelStreamingObjects.Set( LevelStreamingObject, StreamingSettings );

							// Stop looking for viewpoint-containing volumes once all streaming settings have been enabled.
							if ( StreamingSettings.AllSettingsEnabled() )
							{
								break;
							}
						}
					}
				}
			} // for each streaming level 
		} // if PlayerActor
	} // for each Player

	// Iterate over all streaming levels and set the level's loading status based
	// on whether it was found to be visible by a level streaming volume.
	for( INT LevelIndex = 0 ; LevelIndex < LevelStreamingObjectsWithVolumes.Num() ; ++LevelIndex )
	{
		ULevelStreaming* LevelStreamingObject = LevelStreamingObjectsWithVolumes(LevelIndex);

		// Figure out whether level should be loaded and keep track of original state for notifications on change.
		FVisibleLevelStreamingSettings* NewStreamingSettings= VisibleLevelStreamingObjects.Find( LevelStreamingObject );
		UBOOL bShouldAffectLoading							= LevelStreamingObjectsWithVolumesOtherThanBlockingLoad.Find( LevelStreamingObject ) != NULL;
		UBOOL bShouldBeLoaded								= NewStreamingSettings != NULL;
		UBOOL bOriginalShouldBeLoaded						= LevelStreamingObject->bShouldBeLoaded;
		UBOOL bOriginalShouldBeVisible						= LevelStreamingObject->bShouldBeVisible;
		UBOOL bOriginalShouldBlockOnLoad					= LevelStreamingObject->bShouldBlockOnLoad;

		if( bShouldBeLoaded || bShouldAffectLoading )
		{
			if( bShouldBeLoaded )
			{
				// Loading.
				LevelStreamingObject->bShouldBeLoaded		= TRUE;
				LevelStreamingObject->bShouldBeVisible		= NewStreamingSettings->ShouldBeVisible( bOriginalShouldBeVisible );
				LevelStreamingObject->bShouldBlockOnLoad	= NewStreamingSettings->ShouldBlockOnLoad();
			}
			// Prevent unload request flood.  The additional check ensures that unload requests can still be issued in the first UnloadCooldownTime seconds of play.
			else 
			if( WorldInfo->TimeSeconds - LevelStreamingObject->LastVolumeUnloadRequestTime > LevelStreamingObject->MinTimeBetweenVolumeUnloadRequests 
			||  LevelStreamingObject->LastVolumeUnloadRequestTime < 0.1f )
			{
				// Unloading.
				LevelStreamingObject->LastVolumeUnloadRequestTime	= WorldInfo->TimeSeconds;
				LevelStreamingObject->bShouldBeLoaded				= FALSE;
				LevelStreamingObject->bShouldBeVisible				= FALSE;
			}
		
			// Notify players of the change.
			if( bOriginalShouldBeLoaded		!= LevelStreamingObject->bShouldBeLoaded
			||	bOriginalShouldBeVisible	!= LevelStreamingObject->bShouldBeVisible 
			||	bOriginalShouldBlockOnLoad	!= LevelStreamingObject->bShouldBlockOnLoad )
			{
				for (AController *Controller = GWorld->GetWorldInfo()->ControllerList; Controller != NULL; Controller = Controller->NextController)
				{
					APlayerController *PC = Cast<APlayerController>(Controller);
					if (PC != NULL)
					{
						PC->eventLevelStreamingStatusChanged( 
							LevelStreamingObject, 
							LevelStreamingObject->bShouldBeLoaded, 
							LevelStreamingObject->bShouldBeVisible,
							LevelStreamingObject->bShouldBlockOnLoad );
					}
				}
			}
		}
	}
}

/**
 * Update the level after a variable amount of time, DeltaSeconds, has passed.
 * All child actors are ticked after their owners have been ticked.
 */
FLOAT HACK_DelayAfterSkip = 0.0f;
void UWorld::Tick( ELevelTick TickType, FLOAT DeltaSeconds )
{
#if PS3
	// These are for an automated system to detect whether the PS3 has crashed or not.
	static DWORD PS3ProgressCounter = 0;
	if ( PS3ProgressCounter == 0 )
	{
		printf( "PS3Progress_FirstFrame\n" );
		PS3ProgressCounter++;
	}
	else if ( PS3ProgressCounter < 3 )
	{
		printf( "PS3Progress_Frame_%d\n", PS3ProgressCounter );
		PS3ProgressCounter++;
	}
	else if ( PS3ProgressCounter == 3 )
	{
		printf( "PS3Progress_Running\n" );
		PS3ProgressCounter++;
	}
#endif

	AWorldInfo* Info = GetWorldInfo();

	FMemMark Mark(GMem);
	FMemMark EngineMark(GEngineMem);
	GInitRunaway();
	GParticleDataManager.Clear();
	InTick=1;

	UBOOL RecordDemoFrame = FALSE;
	if (DemoRecDriver && !IsPaused())
	{
		// @todo demo default TimeDilation?
		RecordDemoFrame = DemoRecDriver->UpdateDemoTime(&DeltaSeconds, Info->TimeDilation/1.1f); // 1.1f == default TimeDilation
		DemoRecDriver->TickDispatch( DeltaSeconds );

		// Fetch demo playback packets from demo file.
		if (DemoRecDriver->ServerConnection)
		{
			TickDemoPlayback(DeltaSeconds);
		}
	}

	// Update the net code and fetch all incoming packets.
	if( NetDriver )
	{
		NetDriver->TickDispatch( DeltaSeconds );
		if( NetDriver->ServerConnection )
		{
			TickNetClient( DeltaSeconds );
		}
	}
	// Update collision.
	if( Hash )
	{
		Hash->Tick();
	}

	// Update batched lines.
	PersistentLineBatcher->Tick(DeltaSeconds);

	// Update time.
	Info->RealTimeSeconds += DeltaSeconds;

	DeltaSeconds *= Info->TimeDilation;
	// Clamp time between 2000 fps and 2.5 fps.
	DeltaSeconds = Clamp(DeltaSeconds,0.0005f,0.40f);
	Info->DeltaSeconds = DeltaSeconds;

	if ( !IsPaused() )
	{
		Info->TimeSeconds += DeltaSeconds;
	}

	if( Info->bPlayersOnly )
	{
		TickType = LEVELTICK_ViewportsOnly;
	}

	// give the async loading code more time if we're performing a high priority load
	if (Info->bHighPriorityLoading || Info->bHighPriorityLoadingLocal)
	{
		UObject::ProcessAsyncLoading(TRUE, 0.02f);
	}

	// If caller wants time update only, or we are paused, skip the rest.
	if
	(	(TickType!=LEVELTICK_TimeOnly)
	&&	!IsPaused()
	&&	(!NetDriver || !NetDriver->ServerConnection || NetDriver->ServerConnection->State==USOCK_Open) )
	{
		// update the base sequence
		if (!Info->bPlayersOnly && GIsGame)
		{
			SCOPE_CYCLE_COUNTER(STAT_KismetTime);

			for (INT SeqIdx = 0; SeqIdx < CurrentLevel->GameSequences.Num(); SeqIdx++)
			{
				if (CurrentLevel->GameSequences(SeqIdx) != NULL)
				{
					CurrentLevel->GameSequences(SeqIdx)->UpdateOp( DeltaSeconds );
				}
			}
		}

		extern FLOAT HACK_DelayAfterSkip;
		if (HACK_DelayAfterSkip > 0.0f)
		{
			// countdown the N second load savegame delay after skipping a matinee
			HACK_DelayAfterSkip -= DeltaSeconds;
		}

		SCOPE_CYCLE_COUNTER(STAT_TickTime);
		TickGroup = TG_PreAsyncWork;
		// Clear out our old state and empty our arrays (without memory changes)
		GDeferredList.Reset();
		// Tick all actors/components that need to do their work first
		TickActors<FDeferredTickList::FGlobalActorIterator>(this,DeltaSeconds,
			TickType,GDeferredList);

		{
			SCOPE_CYCLE_COUNTER(STAT_PhysicsTime);
			// Tick our async work (physics, etc.) and tick with no elapsed time for
			// playersonly
			TickAsyncWork(Info->bPlayersOnly == FALSE ? DeltaSeconds : 0.f);
		}
		TickGroup = TG_DuringAsyncWork;
		// Tick all actors that can be done during async work
		TickActors<FDeferredTickList::FActorDuringAsyncWorkIterator>(this,
			DeltaSeconds,TickType,GDeferredList);
		// Tick all of the deferred components
		TickDeferredComponents<FDeferredTickList::FComponentDuringAsyncWorkIterator>(DeltaSeconds,GDeferredList);
		// Wait for async work to come back
		WaitForAsyncWork();
		TickGroup = TG_PostAsyncWork;
		// Fire collision notifies (OnRigidBodyCollision events)
		if(RBPhysScene)
		{
			DispatchRBCollisionNotifies(RBPhysScene);
		}
		// Now do the final ticking of actors
		TickActors<FDeferredTickList::FActorPostAsyncWorkIterator>(this,
			DeltaSeconds,TickType,GDeferredList);
		// And the final ticking of components
		TickDeferredComponents<FDeferredTickList::FComponentPostAsyncWorkIterator>(DeltaSeconds,GDeferredList);

		// Tick the decal manager
		DecalManager->Tick(DeltaSeconds);
	}
	else if( IsPaused() )
	{
		// Absorb input if paused.
		NewlySpawned.Empty();
		for (FDynamicActorIterator It; It; ++It)
		{
			AActor*				Actor	= *It;
			APlayerController*	PC		= Cast<APlayerController>(Actor);
			if( ( PC != NULL ) && ( PC->PlayerInput != NULL ) )
			{
				PC->PlayerInput->eventPlayerInput( DeltaSeconds );
				for( TFieldIterator<UFloatProperty> Jtr(PC->PlayerInput->GetClass()); Jtr; ++Jtr )
				{
					if( Jtr->PropertyFlags & CPF_Input )
					{
						*(FLOAT*)((BYTE*)PC->PlayerInput + Jtr->Offset) = 0.f;
					}
				}
				PC->bTicked = (DWORD)Ticked;
			}
			else
			{
				if( Actor->bAlwaysTick && !Actor->ActorIsPendingKill() )
				{
					checkf(!Actor->HasAnyFlags(RF_Unreachable), TEXT("%s"), *Actor->GetFullName());
					Actor->Tick(DeltaSeconds,TickType);
				}
				else
				{
					Actor->bTicked = (DWORD)Ticked;
				}
			}
		}
		// And anything that was spawned
		TickNewlySpawned(this,DeltaSeconds,TickType);

		// See if RealTimeToUnPause is non-zero, and if it is, see if it is time to unpause.
		if((Info->RealTimeToUnPause != 0.f) && (Info->RealTimeSeconds > Info->RealTimeToUnPause))
		{
			ULocalPlayer* LP = GEngine->GamePlayers(0);
			if(LP && LP->Actor)
			{
				LP->Actor->eventConditionalPause(FALSE);
			}
			Info->RealTimeToUnPause = 0.f;
		}
	}

	// Handle ticking FTickableObjects based upon paused state and/or editorness
	if (TickType != LEVELTICK_TimeOnly && !IsPaused())
	{
		// Tick all objects inheriting from FTickableObjects.
		for( INT i=0; i<FTickableObject::TickableObjects.Num(); i++ )
		{
			FTickableObject* TickableObject = FTickableObject::TickableObjects(i);
			if( TickableObject->IsTickable() )
			{
				TickableObject->Tick(DeltaSeconds);
			}
		}
	}
	else if (IsPaused())
	{
		// Tick all objects that require ticking even when paused
		for( INT i=0; i<FTickableObject::TickableObjects.Num(); i++ )
		{
			FTickableObject* TickableObject = FTickableObject::TickableObjects(i);
			if( TickableObject->IsTickable() && TickableObject->IsTickableWhenPaused() )
			{
				TickableObject->Tick(DeltaSeconds);
			}
		}
	}
	else if ( GIsEditor && !GIsPlayInEditorWorld )
	{
		// otherwise, if we are running the editor and we aren't ticking the PIE world,
		// tick all FTickableObjects require ticking in the editor.
		// Tick all objects that require ticking even when paused
		for( INT i=0; i<FTickableObject::TickableObjects.Num(); i++ )
		{
			FTickableObject* TickableObject = FTickableObject::TickableObjects(i);
			if( TickableObject->IsTickable() && TickableObject->IsTickableInEditor() )
			{
				TickableObject->Tick(DeltaSeconds);
			}
		}
	}

	// update all dirty components
	bPostTickComponentUpdate = TRUE;
	for (FDynamicActorIterator It; It; ++It)
	{
		AActor* Actor = *It;
		Actor->ConditionalUpdateComponents();
	}
	LineBatcher->UpdateComponent(Scene,NULL,FMatrix::Identity);
	PersistentLineBatcher->UpdateComponent(Scene,NULL,FMatrix::Identity);
	bPostTickComponentUpdate = FALSE;

	if( !IsPaused() )
	{
		// Update cameras last. This needs to be done before NetUpdates, and after all actors have been ticked.
		for( AController *C = this->GetFirstController(); C != NULL; C = C->NextController)
		{
			APlayerController* PC = C->GetAPlayerController();

			// if it is a player, update the camra.
			if( PC && PC->PlayerCamera )
			{
				PC->PlayerCamera->eventUpdateCamera(DeltaSeconds);
			}
		}

		// Issues level streaming load/unload requests based on local players being inside/outside level streaming volumes.
		if( GIsGame && GWorld->GetNetMode() != NM_Client)
		{
			ProcessLevelStreamingVolumes();
		}
	}

	GParticleDataManager.UpdateDynamicData();

	// Update net and flush networking.
	if (NetDriver != NULL)
	{
		if (NetDriver->ServerConnection != NULL)
		{
			// attempt to verify any packages that are pending
			// it's important to verify packages in order so that we're not reshuffling replicated indices during gameplay, so abort as soon as one fails
			while (NetDriver->ServerConnection->PendingPackageInfos.Num() > 0 && VerifyPackageInfo(NetDriver->ServerConnection->PendingPackageInfos(0)))
			{
				NetDriver->ServerConnection->PendingPackageInfos.Remove(0);
			}
		}
		else
		{
			TickNetServer(DeltaSeconds);
		}
		NetDriver->TickFlush();
	}

	// Update and flush demo Recording.
	if (DemoRecDriver)
	{
		if (!DemoRecDriver->ServerConnection && RecordDemoFrame)
		{
			TickDemoRecord( DeltaSeconds );
		}
		DemoRecDriver->TickFlush();
	}

	// Update the object propagator (this is mostly for a networked propagator to poll)
	GObjectPropagator->Tick( DeltaSeconds );

	if( GetWorldInfo()->DeferredExecs.Num() > 0 )
	{
		// for each of the strings in the DeferredExec array we add them to the Engine's 
		// DeferredCommands which will be executed when it is safe :-)
		for( INT Idx = 0; Idx < GetWorldInfo()->DeferredExecs.Num(); Idx++ )
		{
			new(GEngine->DeferredCommands) FString( *GetWorldInfo()->DeferredExecs(Idx) );
		}

		GetWorldInfo()->DeferredExecs.Empty(); 
	}

	// Finish up.
	Ticked = !Ticked;
	InTick = 0;
	Mark.Pop();
	EngineMark.Pop();

	// Send end of world tick callback.
	GCallbackEvent->Send(CALLBACK_WorldTickFinished);

	if( HasBegunPlay() )
	{
		TimeSinceLastPendingKillPurge += DeltaSeconds;
		// Perform incremental purge update if it's pending or in progress.
		if( !IsIncrementalPurgePending() 
		// Purge reference to pending kill objects every now and so often.
		&&	(TimeSinceLastPendingKillPurge > GEngine->TimeBetweenPurgingPendingKillObjects) && GEngine->TimeBetweenPurgingPendingKillObjects > 0 )
		{
			SCOPE_CYCLE_COUNTER(STAT_GCMarkTime);
			PerformGarbageCollection();
		}
		else
		{
			SCOPE_CYCLE_COUNTER(STAT_GCSweepTime);
			IncrementalPurgeGarbage( TRUE );
		}
	}

	// Don't collect stats during the first 1.5 seconds of gameplay in order to not distort numbers.
	if( !HasBegunPlay() || GetTimeSeconds() < 1.5f )
	{
		GEngine->ResetFPSChart();
	}

#if !FINAL_RELEASE
    // This will show all of the SkeletalMeshComponents that were ticked for one frame 
	if( GShouldLogOutAFrameOfSkelCompTick == TRUE )
	{
		GShouldLogOutAFrameOfSkelCompTick = FALSE;
	}
#endif // !FINAL_RELEASE

}

/**
 *  Interface to allow WorldInfo to request immediate garbage collection
 */
void UWorld::PerformGarbageCollection()
{
	// Iterate over level collection to see whether there are any outstanding load requests.
	AWorldInfo* WorldInfo							= GetWorldInfo();
	UBOOL		bHasOutstandingLevelLoadRequests	= FALSE;
	for( INT LevelIndex=0; LevelIndex<WorldInfo->StreamingLevels.Num(); LevelIndex++ )
	{
		ULevelStreaming* StreamingLevel	= WorldInfo->StreamingLevels(LevelIndex);
		if( StreamingLevel && StreamingLevel->bHasLoadRequestPending )
		{
			bHasOutstandingLevelLoadRequests = TRUE;
			break;
		}
	}

	// We don't collect garbage while there are outstanding async load requests as we would need
	// to block on loading the remaining data.
	if( !UObject::IsAsyncLoading() && !bHasOutstandingLevelLoadRequests )
	{
		// Perform housekeeping.
		UObject::CollectGarbage( GARBAGE_COLLECTION_KEEPFLAGS, FALSE );

		// Remove NULL entries from actor list. Only does so for dynamic actors to avoid resorting; in theory static 
		// actors shouldn't be deleted during gameplay.
		for( INT LevelIndex=0; LevelIndex<Levels.Num(); LevelIndex++ )
		{
			ULevel* Level = Levels(LevelIndex);
			// Don't compact actors array for levels that are currently in the process of being made visible as the
			// code that spreads this work across several frames relies on the actor count not changing as it keeps
			// an index into the array.
			if( !Level->bHasVisibilityRequestPending )
			{
				// Correctly handle never removing the first two entries in the case of iFirstDynamicActor not being set
				INT	FirstDynamicIndex = Max(2,Level->iFirstDynamicActor);
				// Remove NULL entries from array, we're iterating backwards to avoid unnecessary memcpys during removal.
				for( INT ActorIndex=Level->Actors.Num()-1; ActorIndex>=FirstDynamicIndex; ActorIndex-- )
				{
					if( Level->Actors(ActorIndex) == NULL )
					{
						Level->Actors.Remove( ActorIndex );
					}
				}
			}
		}

		// Reset counter.
		TimeSinceLastPendingKillPurge = 0;
	}
}

void AWorldInfo::ForceGarbageCollection()
{
	GWorld->TimeSinceLastPendingKillPurge = 1.f + GEngine->TimeBetweenPurgingPendingKillObjects;
}

void AWorldInfo::VerifyNavList()
{
#if !FINAL_RELEASE
	UWorld::VerifyNavList( *FString::Printf(TEXT("From script...")) );
#endif
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

