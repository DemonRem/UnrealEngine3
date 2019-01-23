/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "UnNet.h"

#include "EngineSequenceClasses.h"
#include "UnPhysicalMaterial.h"
#include "DemoRecording.h"

#if LINE_CHECK_TRACING

#include "StackTracker.h"
FStackTracker* GLineCheckStackTracker = NULL;
FScriptStackTracker* GLineCheckScriptStackTracker = NULL;

/** Dump out the results of all line checks called in the game since the last call to ResetLineChecks() */
void DumpLineChecks()
{
	if( GLineCheckStackTracker )
	{
		FString Filename = FString::Printf(TEXT("%sLineCheckLog-%s.csv"), *appGameLogDir(), *appSystemTimeString());
		FOutputDeviceFile OutputFile(*Filename);
		GLineCheckStackTracker->DumpStackTraces( 100, OutputFile );
		OutputFile.TearDown();
	}

	if( GLineCheckScriptStackTracker )
	{
		FString Filename = FString::Printf(TEXT("%sScriptLineCheckLog-%s.csv"), *appGameLogDir(), *appSystemTimeString());
		FOutputDeviceFile OutputFile(*Filename);
		GLineCheckScriptStackTracker->DumpStackTraces( 100, OutputFile );
		OutputFile.TearDown();
	}
}

/** Reset the line check stack tracker (calls appFree() on all user data pointers)*/
void ResetLineChecks()
{
	if( GLineCheckStackTracker )
	{
		GLineCheckStackTracker->ResetTracking();
	}

	if( GLineCheckScriptStackTracker )
	{
		GLineCheckScriptStackTracker->ResetTracking();
	}
}

/** A struct to store information about individual callstacks during line checks*/
struct FLineCheckData
{
	/** Store the flags used by the line check at the time of the call */
	INT Flags;
	/** Was this a non zero extent line check */
	UBOOL IsNonZeroExtent;
};

/** Updates an existing call stack trace with new data for this particular call*/
static void LineCheckUpdateFn(const FStackTracker::FCallStack& CallStack, void* UserData)
{
	if (UserData)
	{
		//Callstack has been called more than once, aggregate the data
		FLineCheckData* NewLCData = static_cast<FLineCheckData*>(UserData);
		FLineCheckData* OldLCData = static_cast<FLineCheckData*>(CallStack.UserData);

		OldLCData->Flags |= NewLCData->Flags;
		OldLCData->IsNonZeroExtent |= NewLCData->IsNonZeroExtent;
	}
}

/** After the stack tracker reports a given stack trace, it calls this function
 *  which appends data particular to line checks
 */
static void LineCheckReportFn(const FStackTracker::FCallStack& CallStack, FOutputDevice& Ar)
{
	//Output to a csv file any relevant data
	FLineCheckData* LCData = static_cast<FLineCheckData*>(CallStack.UserData);
	if (LCData)
	{
		FString UserOutput = LINE_TERMINATOR TEXT(",,,");
		UserOutput += (LCData->IsNonZeroExtent ? TEXT( "NonZeroExtent") : TEXT("ZeroExtent"));

		if (LCData->Flags & TRACE_StopAtAnyHit)
		{
			UserOutput += TEXT(" TRACE_StopAtAnyHit");
		}
		else if (LCData->Flags & TRACE_SingleResult)
		{
			UserOutput += TEXT(" TRACE_SingleResult");
		}
		else if (LCData->Flags & TRACE_ComplexCollision)
		{
			UserOutput += TEXT(" TRACE_ComplexCollision");
		}   
		else if (LCData->Flags & TRACE_AllComponents)
		{
			UserOutput += TEXT(" TRACE_AllComponents");
		}

		Ar.Log(*UserOutput);
	}
}

/** Turn line check stack traces on and off, does not reset the actual data */
void ToggleLineChecks()
{
	if (GLineCheckStackTracker == NULL)
	{
		GLineCheckStackTracker = new FStackTracker(LineCheckUpdateFn, LineCheckReportFn);
	}

	if (GLineCheckScriptStackTracker == NULL)
	{
		GLineCheckScriptStackTracker = new FScriptStackTracker();
	}

	GLineCheckStackTracker->ToggleTracking();
	GLineCheckScriptStackTracker->ToggleTracking();
}

/** Captures a single stack trace for a line check */
void CaptureLineCheck(INT LineCheckFlags, const FVector* Extent, const FFrame* ScriptStackFrame)
{
	if (GLineCheckStackTracker == NULL || GLineCheckScriptStackTracker == NULL)
	{
		return;
	}

	if (ScriptStackFrame)
	{
		INT EntriesToIgnore = 0;
		GLineCheckScriptStackTracker->CaptureStackTrace(ScriptStackFrame, EntriesToIgnore);
	}
	else
	{		   
		FLineCheckData* LCData = static_cast<FLineCheckData*>(appMalloc(sizeof(FLineCheckData)));
		LCData->Flags = LineCheckFlags;

		LCData->IsNonZeroExtent = (Extent && !Extent->IsZero()) ? TRUE : FALSE;

		INT EntriesToIgnore = 3;
		GLineCheckStackTracker->CaptureStackTrace(EntriesToIgnore, static_cast<void*>(LCData));
	}
}
#endif //LINE_CHECK_TRACING

/*-----------------------------------------------------------------------------
	World/ Level/ Actor GC verification.
-----------------------------------------------------------------------------*/

/**
 * Verifies that there are no unreachable actor references. Registered as post GC callback.
 */
void VerifyNoUnreachableActorsReferenced()
{
#if DO_GUARD_SLOW
	// Don't perform work in the Editor or during the exit purge.
	if( !GIsEditor && !GExitPurge )
	{
		UBOOL bNoUnreachableReferences = TRUE;

		// Some operations can only be performed with a valid GWorld.
		if( GWorld )
		{
			// Iterate over all streaming levels and check them as well. This is a superset of the 
			// actor iterator used above as it contains levels that are loaded but not visible. We
			// can only do this if there actually is a world object.
			AWorldInfo* WorldInfo = GWorld->GetWorldInfo();
			for( INT LevelIndex=0; LevelIndex<WorldInfo->StreamingLevels.Num(); LevelIndex++ )
			{
				ULevelStreaming* StreamingLevel = WorldInfo->StreamingLevels(LevelIndex);
				if( StreamingLevel && StreamingLevel->LoadedLevel )
				{
					ULevel* Level = StreamingLevel->LoadedLevel;
					// Iterate over all actors in the level and make sure neither of them references an
					// unreachable object or is unreachable itself.
					for( INT ActorIndex=0; ActorIndex<Level->Actors.Num(); ActorIndex++ )
					{
						AActor* Actor = Level->Actors(ActorIndex);
						if( Actor )
						{
							bNoUnreachableReferences = Actor->VerifyNoUnreachableReferences() && bNoUnreachableReferences;
						}
					}
				}
			}
		}
		
		// If there is no pending incremental purge it means that all ULevel objects are fair game.
		if( !UObject::IsIncrementalPurgePending() )
		{
			// Iterate over all levels in memory.
			for( TObjectIterator<ULevel> It; It; ++It )
			{
				ULevel* Level = *It;
				check( !Level->HasAnyFlags( RF_Unreachable ) );
				// Iterate over all actors in the level and make sure neither of them references an
				// unreachable object or is unreachable itself.
				for( INT ActorIndex=0; ActorIndex<Level->Actors.Num(); ActorIndex++ )
				{
					AActor* Actor = Level->Actors(ActorIndex);
					if( Actor )
					{
						bNoUnreachableReferences = Actor->VerifyNoUnreachableReferences() && bNoUnreachableReferences;
					}
				}
			}
		}
		check( bNoUnreachableReferences );
	}
#endif
}

IMPLEMENT_POST_GARBAGE_COLLECTION_CALLBACK( DUMMY_VerifyNoUnreachableActorsReferenced, VerifyNoUnreachableActorsReferenced, GCCB_POST_VerifyNoUnreachableActorsReferenced );

/*-----------------------------------------------------------------------------
	Level actor management.
-----------------------------------------------------------------------------*/


/**
 * PERF_ISSUE_FINDER
 *
 * Actors should not have more than one Collision Component. 
 *
 * Turn this on to check if actors have more than one collision component (most should not, 
 * but can easily happen accidentally)
 * Having more than one is going to cause extra collision checks.
 *
 **/
//#define DEBUG_CHECKCOLLISIONCOMPONENTS 1


/**
 * PERF_ISSUE_FINDER
 *
 * Move Actor should not take a long time to execute.  If it is then then there is probably something wrong.
 *
 * Turn this on to have the engine log out when a specific actor is taking longer than 
 * SHOW_MOVEACTOR_TAKING_LONG_TIME_AMOUNT to move.  This is a great way to catch cases where
 * collision has been enabled but it should not have been.  Or if a specific actor is doing something evil
 *
 **/
//#define SHOW_MOVEACTOR_TAKING_LONG_TIME 1
const static FLOAT SHOW_MOVEACTOR_TAKING_LONG_TIME_AMOUNT = 2.0f; // modify this value to look at larger or smaller sets of "bad" actors

//
// Create a new actor. Returns the new actor, or NULL if failure.
//
AActor* UWorld::SpawnActor
(
	UClass*			Class,
	FName			InName,
	const FVector&	Location,
	const FRotator&	Rotation,
	AActor*			Template,
	UBOOL			bNoCollisionFail,
	UBOOL			bRemoteOwned,
	AActor*			Owner,
	APawn*			Instigator,
	UBOOL			bNoFail
)
{
	check(CurrentLevel);
	check(GIsEditor || (CurrentLevel == PersistentLevel));
	check(GWorld == this || GIsCooking);

	// It's not safe to call UWorld accessor functions till the world info has been spawned.
	const UBOOL bBegunPlay = HasBegunPlay();

	// Make sure this class is spawnable.
	if( !Class )
	{
		debugf( NAME_Warning, TEXT("SpawnActor failed because no class was specified") );
		return NULL;
	}
	if( Class->ClassFlags & CLASS_Deprecated )
	{
		debugf( NAME_Warning, TEXT("SpawnActor failed because class %s is deprecated"), *Class->GetName() );
		return NULL;
	}
	if( Class->ClassFlags & CLASS_Abstract )
	{
		debugf( NAME_Warning, TEXT("SpawnActor failed because class %s is abstract"), *Class->GetName() );
		return NULL;
	}
	else if( !Class->IsChildOf(AActor::StaticClass()) )
	{
		debugf( NAME_Warning, TEXT("SpawnActor failed because %s is not an actor class"), *Class->GetName() );
		return NULL;
	}
	else if( bBegunPlay && (Class->GetDefaultActor()->bStatic || Class->GetDefaultActor()->bNoDelete) )
	{
		debugf( NAME_Warning, TEXT("SpawnActor failed because class %s has bStatic or bNoDelete"), *Class->GetName() );
		if ( !bNoFail )
			return NULL;		
	}
	else if (Template != NULL && Template->GetClass() != Class)
	{
		debugf(NAME_Warning, TEXT("SpawnActor failed because template class (%s) does not match spawn class (%s)"), *Template->GetClass()->GetName(), *Class->GetName());
		if (!bNoFail)
		{
			return NULL;
		}
	}

#if !FINAL_RELEASE
	// Check to see if this move is illegal during this tick group
	if (InTick && TickGroup == TG_DuringAsyncWork && Class->GetDefaultActor()->bBlockActors)
	{
		debugf(NAME_Error,TEXT("Can't spawn collidable actor (%s) during async work!"),*Class->GetName());
	}
#endif

	// Use class's default actor as a template.
	if( !Template )
	{
		Template = Class->GetDefaultActor();
	}
	check(Template!=NULL);

	FVector NewLocation = Location;
	// Make sure actor will fit at desired location, and adjust location if necessary.
	if( (Template->bCollideWorld || (Template->bCollideWhenPlacing && (GetNetMode() != NM_Client))) && !bNoCollisionFail )
	{
		if (!FindSpot(Template->GetCylinderExtent(), NewLocation, Template->bCollideComplex))
		{
			debugf( NAME_Warning, TEXT("SpawnActor failed because of collision at the spawn location [%s]"), *Class->GetName() );
			return NULL;
		}
	}

	// Spawn in the same level as the owner if we have one. @warning: this relies on the outer of an actor being the level.
	ULevel* LevelToSpawnIn = Owner ? CastChecked<ULevel>(Owner->GetOuter()) : CurrentLevel;
	AActor* Actor = ConstructObject<AActor>( Class, LevelToSpawnIn, InName, RF_Transactional, Template );
	check(Actor);
	if ( GUndo )
	{
		GWorld->ModifyLevel( LevelToSpawnIn );
	}
	LevelToSpawnIn->Actors.AddItem( Actor );

#if defined(DEBUG_CHECKCOLLISIONCOMPONENTS) || LOOKING_FOR_PERF_ISSUES
	INT numcollisioncomponents = 0;
	for(INT ComponentIndex = 0;ComponentIndex < Actor->Components.Num();ComponentIndex++)
	{
		UActorComponent* ActorComponent = Actor->Components(ComponentIndex);
		if( ActorComponent )
		{
			UPrimitiveComponent *C = Cast<UPrimitiveComponent>( ActorComponent );
			if ( C && C->ShouldCollide() )
			{
				numcollisioncomponents++;
			}
		}

		// most pawns are going to have a CylinderComponent (Unreal Physics) and then they are also
		// going to have a SkeletalMeshComponent (per body hit detection).  So we really care about
		// cases of 3 or more  components with collision.  If we are still lots of spam then we will
		// want to increase the num we are looking for 
		if ( numcollisioncomponents > 2 )
		{
			debugf( NAME_PerfWarning, TEXT("Actor has more 3 or more components which collide: "), *Actor->GetName());

			for(INT ComponentIndex = 0;ComponentIndex < Actor->Components.Num();ComponentIndex++)
			{
				if(Actor->Components(ComponentIndex))
				{
					UPrimitiveComponent *C = Cast<UPrimitiveComponent>(Actor->Components(ComponentIndex));
					if ( C && C->ShouldCollide() )
					{
						debugf( NAME_PerfWarning, TEXT("    -component with collision:  %s"), *C->GetName() );
					}
				}
			}
		}
	}	

	if ( Actor->bCollideActors && !Actor->IsA(AProjectile::StaticClass()) )
	{
		debugf( NAME_PerfWarning, TEXT("%s has bCollideActors set"),*Actor->GetFullName());
	}
#endif

	// Detect if the actor's collision component is not in the components array, which is invalid.
	if(Actor->CollisionComponent && Actor->Components.FindItemIndex(Actor->CollisionComponent) == INDEX_NONE)
	{
		if ( bBegunPlay )
		{
			appErrorf(TEXT("Spawned actor %s with a collision component %s that is not in the Components array."),*Actor->GetFullName(),*Actor->CollisionComponent->GetFullName());
		}
		else
		{
			debugf( NAME_Warning, TEXT("Spawned actor %s with a collision component %s that is not in the Components array."),*Actor->GetFullName(),*Actor->CollisionComponent->GetFullName() );
		}
	}

	// Set base actor properties.
	if (Actor->Tag == NAME_None)
	{
		Actor->Tag = Class->GetFName();
	}
	Actor->bTicked		= !Ticked;
	Actor->CreationTime = GetTimeSeconds();
	Actor->WorldInfo	= GetWorldInfo();

	// Set network role.
	check(Actor->Role==ROLE_Authority);
	if( bRemoteOwned )
	{
		Exchange( Actor->Role, Actor->RemoteRole );
	}

	// Set the actor's location and rotation.
	Actor->Location = NewLocation;
	Actor->Rotation = Rotation;

	// Initialize the actor's components.
	Actor->ConditionalForceUpdateComponents(FALSE,FALSE);

	// init actor's physics volume
	Actor->PhysicsVolume = GetWorldInfo()->PhysicsVolume;

	// Set owner.
	Actor->SetOwner( Owner );

	// Set instigator
	Actor->Instigator = Instigator;

	// Initialise physics if we are in the game.
	if (bBegunPlay)
	{
		Actor->InitRBPhys();
	}

	// Send messages.
	if ( !GIsCooking )
	{
		Actor->InitExecution();
		Actor->Spawned();
	}
	if(bBegunPlay)
	{
		Actor->PreBeginPlay();
		
		if( Actor->bDeleteMe && !bNoFail )
		{
			return NULL;
		}

		for(INT ComponentIndex = 0;ComponentIndex < Actor->Components.Num();ComponentIndex++)
		{
			if(Actor->Components(ComponentIndex))
			{
				Actor->Components(ComponentIndex)->ConditionalBeginPlay();
			}
		}
	}

	// Check for encroachment.
	if( !bNoCollisionFail )
	{
		if( CheckEncroachment( Actor, Actor->Location, Actor->Rotation, 1 ) )
		{
			debugf(NAME_Warning, TEXT("SpawnActor destroyed [%s] after spawning because it was encroaching on another Actor"), *Actor->GetName());
			DestroyActor( Actor );
			return NULL;
		}
	}
	else if ( Actor->bCollideActors )
	{
		Actor->FindTouchingActors();
		
		if( Actor->bDeleteMe && !bNoFail )
		{
			return NULL;
		}
	}

	if(bBegunPlay)
	{
		Actor->PostBeginPlay();
		
		if( Actor->bDeleteMe && !bNoFail )
		{
			return NULL;
		}
	}

	// Success: Return the actor.
	if( InTick )
	{
		NewlySpawned.AddItem( Actor );
	}

	if ( !bBegunPlay )
	{
		// Set bDeleteMe to true so that when the initial undo record is made,
		// the actor will be treated as destroyed, in that undo an add will
		// actually work
		Actor->bDeleteMe = 1;
		Actor->Modify();
		Actor->bDeleteMe = 0;
	}

	return Actor;
}

//
// Spawn a brush.
//
ABrush* UWorld::SpawnBrush()
{
	ABrush* Result = (ABrush*)SpawnActor( ABrush::StaticClass() );
	check(Result);
	return Result;
}

/**
 * Wrapper for DestroyActor() that should be called in the editor.
 *
 * @param	bShouldModifyLevel		If TRUE, Modify() the level before removing the actor.
 */
UBOOL UWorld::EditorDestroyActor( AActor* ThisActor, UBOOL bShouldModifyLevel )
{
	check(ThisActor);
	check(ThisActor->IsValid());

	if ( ThisActor->IsA(ANavigationPoint::StaticClass()) )
	{
		if ( GetWorldInfo()->bPathsRebuilt )
		{
			debugf(TEXT("EditorDestroyActor Clear paths rebuilt"));
		}
		GetWorldInfo()->bPathsRebuilt = FALSE;
	}

	return DestroyActor( ThisActor, FALSE, bShouldModifyLevel );
}

/**
 * Removes the actor from its level's actor list and generally cleans up the engine's internal state.
 * What this function does not do, but is handled via garbage collection instead, is remove references
 * to this actor from all other actors, and kill the actor's resources.  This function is set up so that
 * no problems occur even if the actor is being destroyed inside its recursion stack.
 *
 * @param	ThisActor				Actor to remove.
 * @param	bNetForce				[opt] Ignored unless called during play.  Default is FALSE.
 * @param	bShouldModifyLevel		[opt] If TRUE, Modify() the level before removing the actor.  Default is TRUE.
 * @return							TRUE if destroy, FALSE if actor couldn't be destroyed.
 */
UBOOL UWorld::DestroyActor( AActor* ThisActor, UBOOL bNetForce, UBOOL bShouldModifyLevel )
{
	check(ThisActor);
	check(ThisActor->IsValid());
	//debugf( NAME_Log, "Destroy %s", *ThisActor->GetClass()->GetName() );

#if !FINAL_RELEASE
	// Check to see if this move is illegal during this tick group
	if (InTick && TickGroup == TG_DuringAsyncWork && ThisActor->bBlockActors)
	{
		debugf(NAME_Error,TEXT("Can't destroy collidable actor (%s) during async work!"),*ThisActor->GetName());
	}
#endif

	// In-game deletion rules.
	if( HasBegunPlay() )
	{
		// Can't kill bStatic and bNoDelete actors during play.
		if( ThisActor->bStatic || ThisActor->bNoDelete )
		{
			return 0;
		}

		// If already on list to be deleted, pretend the call was successful.
		if( ThisActor->bDeleteMe )
		{
			return 1;
		}

		// Can't kill if wrong role.
		if( ThisActor->Role!=ROLE_Authority && !bNetForce && !ThisActor->bNetTemporary )
		{
			return 0;
		}

		// Don't destroy player actors.
		APlayerController* PC = ThisActor->GetAPlayerController();
		if ( PC )
		{
			UNetConnection* C = Cast<UNetConnection>(PC->Player);
			if( C )
			{	
				if( C->Channels[0] && C->State!=USOCK_Closed )
				{
					PC->bPendingDestroy = true;
					C->Channels[0]->Close();
				}
				return 0;
			}
		}
	}
	else
	{
		ThisActor->Modify();
	}
	ThisActor->bPendingDelete = true;

	// check for a destroyed sequence event
	for (INT Idx = 0; Idx < ThisActor->GeneratedEvents.Num(); Idx++)
	{
		USeqEvent_Destroyed *Evt = Cast<USeqEvent_Destroyed>(ThisActor->GeneratedEvents(Idx));
		if (Evt != NULL)
		{
			Evt->CheckActivate(ThisActor,ThisActor);
		}
	}

	// Terminate any physics engine stuff for this actor right away.
	ThisActor->TermRBPhys(NULL);

	// Send EndState notification.
	if( ThisActor->GetStateFrame() && ThisActor->GetStateFrame()->StateNode )
	{
		ThisActor->eventEndState(NAME_None);
		if( ThisActor->bDeleteMe )
		{
			return 1;
		}
	}
	// Tell this actor it's about to be destroyed.
	ThisActor->eventDestroyed();
	ThisActor->PostScriptDestroyed();
	// Remove from base.
	if( ThisActor->Base )
	{
		ThisActor->SetBase( NULL );
		if( ThisActor->bDeleteMe )
		{
			return 1;
		}
	}
	
	// Make a copy of the array, as calling SetBase might change the contents of the array.
	TArray<AActor*> AttachedCopy = ThisActor->Attached;
	for( INT AttachmentIndex=0; AttachmentIndex < AttachedCopy.Num(); AttachmentIndex++ )
	{
		AActor* AttachedActor = AttachedCopy(AttachmentIndex);
		if( AttachedActor && AttachedActor->Base == ThisActor && !AttachedActor->bDeleteMe )
		{
			AttachedActor->SetBase( NULL );
		}
	}
	// Then empty the array.
	ThisActor->Attached.Empty();

	if( ThisActor->bDeleteMe )
	{
		return 1;
	}

	// Clean up all touching actors.
	INT iTemp = 0;
	for ( INT i=0; i<ThisActor->Touching.Num(); i++ )
	{
		if ( ThisActor->Touching(i) && ThisActor->Touching(i)->Touching.FindItem(ThisActor, iTemp) )
		{
			ThisActor->EndTouch( ThisActor->Touching(i), 1 );
			i--;
			if( ThisActor->bDeleteMe )
			{
				return 1;
			}
		}
	}

	// If this actor has an owner, notify it that it has lost a child.
	if( ThisActor->Owner )
	{
		ThisActor->SetOwner(NULL);
		if( ThisActor->bDeleteMe )
		{
			return 1;
		}
	}
	// Notify net players that this guy has been destroyed.
	if( NetDriver )
	{
		NetDriver->NotifyActorDestroyed( ThisActor );
	}

	// If demo recording, notify the demo.
	if( DemoRecDriver && !DemoRecDriver->ServerConnection )
	{
		DemoRecDriver->NotifyActorDestroyed( ThisActor );
	}

	// Remove the actor from the actor list.
	RemoveActor( ThisActor, bShouldModifyLevel );
	
	// Mark the actor and its direct components as pending kill.
	ThisActor->bDeleteMe = 1;
	ThisActor->MarkPackageDirty();
	ThisActor->MarkComponentsAsPendingKill();

	// Clean up the actor's components.
	ThisActor->ClearComponents();

	// Invalidate the lighting cache in the Editor.  We need to check for GIsEditor as play has not begun in network game and objects get destroyed on switching levels
	if( GIsEditor && !HasBegunPlay() )
	{
		ThisActor->InvalidateLightingCache();
	}

	// Return success.
	return 1;
}

/**
 * Cleans up anything that needs cleaning up before the Level Transition.
 *
 * Destroys actors marked as bKillDuringLevelTransition. 
 */
void UWorld::CleanUpBeforeLevelTransition()
{
	// Kill actors we are supposed to remove reference to during e.g. seamless map transitions.
	for( INT ActorIndex=0; ActorIndex<PersistentLevel->Actors.Num(); ActorIndex++ )
	{
		AActor* Actor = PersistentLevel->Actors(ActorIndex);
		if( Actor && Actor->bKillDuringLevelTransition )
		{
			DestroyActor( Actor );
		}
	}

	// reset the CrossFadeMusicTrack references that the WorldInfo has so they can be GC'd
	AWorldInfo* WorldInfo = GetWorldInfo();
	if( WorldInfo != NULL )
	{
		if( ( WorldInfo->LastMusicAction != NULL ) && ( WorldInfo->LastMusicAction->CurrPlayingTrack != NULL ) )
		{
			WorldInfo->LastMusicAction->CurrPlayingTrack->FadeOut( 0.1f, 0.0f );
			WorldInfo->LastMusicAction->CurrPlayingTrack = NULL;
			WorldInfo->LastMusicAction = NULL;
		}

		WorldInfo->LastMusicTrack.TheSoundCue = NULL;
	}
	

}


/*-----------------------------------------------------------------------------
	Player spawning.
-----------------------------------------------------------------------------*/

//
// Spawn an actor for gameplay.
//
struct FAcceptInfo
{
	AActor*			Actor;
	FString			Name;
	TArray<FString> Parms;
	FAcceptInfo( AActor* InActor, const TCHAR* InName )
	: Actor( InActor ), Name( InName ), Parms()
	{}
};
APlayerController* UWorld::SpawnPlayActor( UPlayer* Player, ENetRole RemoteRole, const FURL& URL, FString& Error )
{
	Error=TEXT("");

	// Get package map.
	UPackageMap*    PackageMap = NULL;
	UNetConnection* Conn       = Cast<UNetConnection>( Player );
	if( Conn )
		PackageMap = Conn->PackageMap;

	// Make the option string.
	TCHAR Options[1024]=TEXT("");
	for( INT i=0; i<URL.Op.Num(); i++ )
	{
		appStrcat( Options, TEXT("?") );
		appStrcat( Options, *URL.Op(i) );
	}

	// Tell UnrealScript to log in.
	APlayerController* Actor = GetGameInfo()->eventLogin( *URL.Portal, Options, Error );
	if( !Actor )
	{
		debugf( NAME_Warning, TEXT("Login failed: %s"), *Error);
		return NULL;
	}

	// Possess the newly-spawned player.
	Actor->SetPlayer( Player );
	debugf(TEXT("%s got player %s"), *Actor->GetName(), *Player->GetName());
	Actor->Role       = ROLE_Authority;
	Actor->RemoteRole = RemoteRole;
	GetGameInfo()->eventPostLogin( Actor );

	return Actor;
}

/*-----------------------------------------------------------------------------
	Level actor moving/placing.
-----------------------------------------------------------------------------*/

/** CheckSlice() used by FindSpot() */
UBOOL UWorld::CheckSlice(FVector& Location, const FVector& Extent, INT& bKeepTrying)
{
	FCheckResult Hit(1.f);
	FVector SliceExtent = Extent;
	SliceExtent.Z = 1.f;
	bKeepTrying = 0;

	if( !EncroachingWorldGeometry(Hit, Location, SliceExtent) )
	{
		// trace down to find floor
		FVector Down = FVector(0.f,0.f,Extent.Z);
		
		SingleLineCheck(Hit, NULL, Location - 2.f*Down, Location, TRACE_World, SliceExtent);

		FVector FloorNormal = Hit.Normal;
		if( !Hit.Actor || (Hit.Time > 0.5f) )
		{
			// assume ceiling was causing problem
			if( !Hit.Actor )
			{
				Location = Location - Down;
			}
			else
			{
				Location = Location - (2.f*Hit.Time-1.f) * Down + FVector(0.f,0.f,1.f);
			}

			if( !EncroachingWorldGeometry(Hit,Location, Extent) )
			{
				// push back up to ceiling, and return
				SingleLineCheck( Hit, NULL, Location + Down, Location, TRACE_World, Extent );
				if( Hit.Actor )
				{
					Location = Hit.Location;
				}
				return TRUE;
			}
			else
			{
				// push out from floor, try to fit
				FloorNormal.Z = 0.f;
				Location = Location + FloorNormal * Extent.X;
				return ( !EncroachingWorldGeometry( Hit,Location, Extent ) );
			}
		}
		else
		{
			// assume Floor was causing problem
			Location = Location + (0.5f-Hit.Time) * 2.f*Down + FVector(0.f,0.f,1.f);
			if( !EncroachingWorldGeometry(Hit, Location, Extent) )
			{
				return TRUE;
			}
			else
			{
				// push out from floor, try to fit
				FloorNormal.Z = 0.f;
				Location = Location + FloorNormal * Extent.X;
				return ( !EncroachingWorldGeometry(Hit,Location, Extent) );
			}
		}
	}
	bKeepTrying = 1;
	return FALSE;
}


/**
 * Find a suitable nearby location to place a collision box.
 * No suitable location will ever be found if Location is not a valid point inside the level
 */
UBOOL UWorld::FindSpot(const FVector& Extent, FVector& Location, UBOOL bUseComplexCollision)
{
	FCheckResult Hit(1.f);

	// check if fits at desired location
	if( !EncroachingWorldGeometry(Hit, Location, Extent, bUseComplexCollision) )
	{
		return TRUE;
	}

	if( Extent.IsZero() )
	{
		return FALSE;
	}

	FVector StartLoc = Location;

	// Check if slice fits
	INT bKeepTrying = 1;

	if( CheckSlice(Location, Extent, bKeepTrying) )
	{
		return TRUE;
	}
	else if( !bKeepTrying )
	{
		return FALSE;
	}

	// Try to fit half-slices
	Location = StartLoc;
	FVector SliceExtent = 0.5f * Extent;
	SliceExtent.Z = 1.f;
	INT NumFit = 0;
	for (INT i=-1; i<2; i+=2)
	{
		for (INT j=-1; j<2; j+=2)
		{
			if( NumFit < 2 )
			{
				const FVector SliceOffset = FVector(0.55f*Extent.X*i, 0.55f*Extent.Y*j, 0.f);
				if( !EncroachingWorldGeometry(Hit, StartLoc+SliceOffset, SliceExtent, bUseComplexCollision) )
				{
					NumFit++;
					Location += 1.1f * SliceOffset;
				}
			}
		}
	}

	if( NumFit == 0 )
	{
		return FALSE;
	}

	// find full-sized slice to check
	if( NumFit == 1 )
	{
		Location = 2.f * Location - StartLoc;
	}

	SingleLineCheck(Hit, NULL, Location, StartLoc, TRACE_World);
	if( Hit.Actor )
	{
		return FALSE;
	}

	if (!EncroachingWorldGeometry(Hit,Location, Extent, bUseComplexCollision) || CheckSlice(Location, Extent, bKeepTrying))
	{
		// adjust toward center
		SingleLineCheck(Hit, NULL, StartLoc + 0.2f * (StartLoc - Location), Location, TRACE_World, Extent);
		if( Hit.Actor )
		{
			Location = Hit.Location;
		}
		return TRUE;
	}
	return FALSE;
}


//
// Try to place an actor that has moved a long way.  This is for
// moving actors through teleporters, adding them to levels, and
// starting them out in levels.  The results of this function is independent
// of the actor's current location and rotation.
//
// If the actor doesn't fit exactly in the location specified, tries
// to slightly move it out of walls and such.
//
// Returns 1 if the actor has been successfully moved, or 0 if it couldn't fit.
//
// Updates the actor's Zone and PhysicsVolume.
//
UBOOL UWorld::FarMoveActor( AActor* Actor, const FVector& DestLocation, UBOOL test, UBOOL bNoCheck, UBOOL bAttachedMove )
{
	SCOPE_CYCLE_COUNTER(STAT_FarMoveActorTime);

	check(Actor!=NULL);
	if( (Actor->bStatic || !Actor->bMovable) && HasBegunPlay() )
		return 0;
	if ( test && (Actor->Location == DestLocation) )
		return 1;

#if !FINAL_RELEASE
	// Check to see if this move is illegal during this tick group
	if (InTick && TickGroup == TG_DuringAsyncWork && Actor->bBlockActors && !test)
	{
		debugf(NAME_Error,TEXT("Can't move collidable actor (%s) during async work!"),*Actor->GetName());
	}
#endif

	FVector prevLocation = Actor->Location;
	FVector newLocation = DestLocation;
	int result = 1;

	if (!bNoCheck && (Actor->bCollideWorld || (Actor->bCollideWhenPlacing && (GetNetMode() != NM_Client))) )
		result = FindSpot(Actor->GetCylinderExtent(), newLocation, Actor->bCollideComplex);

	if (result && !test && !bNoCheck)
		result = !CheckEncroachment( Actor, newLocation, Actor->Rotation, 0);
	
	if( prevLocation != Actor->Location && !test && !Actor->IsEncroacher() ) // CheckEncroachment moved this actor (teleported), we're done
	{
		// todo: ensure the actor was placed back into the collision hash
		//debugf(TEXT("CheckEncroachment moved this actor, we're done!"));
		//farMoveStackCnt--;
		return result;
	}
	
	if( result )
	{
		//move based actors and remove base unles this farmove was done as a test
		if ( !test )
		{
			Actor->bJustTeleported = true;
			if ( !bAttachedMove )
				Actor->SetBase(NULL);
			for ( INT i=0; i<Actor->Attached.Num(); i++ )
			{
				if ( Actor->Attached(i) )
				{
					FarMoveActor(Actor->Attached(i),
						newLocation + Actor->Attached(i)->Location - Actor->Location,false,bNoCheck,true);
				}
			}
		}
		Actor->Location = newLocation;
	}

	// Update any collision components.  If we are in the Tick phase, only upgrade components with collision.
	Actor->ForceUpdateComponents( GWorld->InTick );

	if ( !test && Actor->bCollideActors )
	{
		// remove actors that are no longer touched
		Actor->UnTouchActors();

		// touch actors
		Actor->FindTouchingActors();
	}

	// Set the zone after moving, so that if a PhysicsVolumeChange or ActorEntered/ActorLeaving message
	// tries to move the actor, the hashing will be correct.
	if( result )
	{
		Actor->SetZone( test,0 );
	}

	return result;
}

//
// Tries to move the actor by a movement vector.  If no collision occurs, this function
// just does a Location+=Move.
//
// Assumes that the actor's Location is valid and that the actor
// does fit in its current Location. Assumes that the level's
// Dynamics member is locked, which will always be the case during
// a call to UWorld::Tick; if not locked, no actor-actor collision
// checking is performed.
//
// If bCollideWorld, checks collision with the world.
//
// For every actor-actor collision pair:
//
// If both have bCollideActors and bBlocksActors, performs collision
//    rebound, and dispatches Touch messages to touched-and-rebounded
//    actors.
//
// If both have bCollideActors but either one doesn't have bBlocksActors,
//    checks collision with other actors (but lets this actor
//    interpenetrate), and dispatches Touch and UnTouch messages.
//
// Returns 1 if some movement occured, 0 if no movement occured.
//
// Updates actor's Zone and PhysicsVolume.
//

UBOOL UWorld::MoveActor
(
	AActor*			Actor,
	const FVector&	Delta,
	const FRotator&	NewRotation,
	DWORD			MoveFlags,
	FCheckResult&	Hit
)
{
	SCOPE_CYCLE_COUNTER(STAT_MoveActorTime);

	check(Actor!=NULL);

#if defined(SHOW_MOVEACTOR_TAKING_LONG_TIME) || LOOKING_FOR_PERF_ISSUES
	DWORD MoveActorTakingLongTime=0;
	clock(MoveActorTakingLongTime);
#endif


	if ( Actor->bDeleteMe )
	{
		//debugf(TEXT("%s deleted move physics %d"),*Actor->GetName(),Actor->Physics);
		return 0;
	}
	if( (Actor->bStatic || !Actor->bMovable) && HasBegunPlay() )
		return 0;

	// Init CheckResult
	Hit.Actor = NULL;
	Hit.Time  = 1.f;

#if !FINAL_RELEASE
	// Check to see if this move is illegal during this tick group
	if (InTick && TickGroup == TG_DuringAsyncWork && Actor->bBlockActors)
	{
		debugf(NAME_Error,TEXT("Can't move collidable actor (%s) during async work!"),*Actor->GetName());
	}
#endif

	// Set up.
	FLOAT DeltaSize;
	FVector DeltaDir;
	if( Delta.IsZero() )
	{
		// Skip if no vector or rotation.
		if( NewRotation==Actor->Rotation && !Actor->bAlwaysEncroachCheck )
		{
			return 1;
		}
		DeltaSize = 0.f;
		DeltaDir = Delta;
	}
	else
	{
		DeltaSize = Delta.Size();
		DeltaDir = Delta/DeltaSize;
	}

	UBOOL bNoFail = MoveFlags & MOVE_NoFail;
	UBOOL bIgnoreBases = MoveFlags & MOVE_IgnoreBases;

	FMemMark Mark(GMem);
	INT     MaybeTouched   = 0;
	FCheckResult* FirstHit = NULL;
	FVector FinalDelta = Delta;
	FRotator OldRotation = Actor->Rotation;
	UBOOL bMovementOccurred = TRUE;

	if ( Actor->IsEncroacher() )
	{
		if( Actor->bNoEncroachCheck || !Actor->bCollideActors || bNoFail )
		{
			// Update the location.
			Actor->Location += FinalDelta;
			Actor->Rotation = NewRotation;

			// Update any collision components.  If we are in the Tick phase, only upgrade components with collision.
			// This is done before moving attached actors so they can test for encroachment based on the correct mover position.
			// It is done before touch so that components are correctly up-to-date when we call Touch events. Things like
			// Kismet's touch event do an IsOverlapping - which requires the component to be in the right location.
			// This will not fix all problems with components being out of date though - for example, attachments of an Actor are not 
			// updated at this point.
			Actor->ForceUpdateComponents( GWorld->InTick );
		}
		else if( CheckEncroachment( Actor, Actor->Location + FinalDelta, NewRotation, 1 ) )
		{
			// Abort if encroachment declined.
			Mark.Pop();
			return 0;
		}
		// if checkencroachment() doesn't fail, collision components will have been updated
	}
	else
	{
		// Perform movement collision checking if needed for this actor.
		if( (Actor->bCollideActors || Actor->bCollideWorld) &&
			Actor->CollisionComponent &&
			(DeltaSize != 0.f) )
		{
			// Check collision along the line.
			DWORD TraceFlags = 0;
			if( MoveFlags & MOVE_TraceHitMaterial )
			{
				TraceFlags |= TRACE_Material;
			}
			
			if( Actor->bCollideActors )
			{
				TraceFlags |= TRACE_Pawns | TRACE_Others | TRACE_Volumes;
			}

			if( Actor->bCollideWorld )
			{
				TraceFlags |= TRACE_World;
			}

			if( Actor->bCollideComplex )
			{
				TraceFlags |= TRACE_ComplexCollision;
			}

			FVector ColCenter;

			if( Actor->CollisionComponent->IsValidComponent() )
			{
				if( !Actor->CollisionComponent->IsAttached() )
				{
					appErrorf(TEXT("%s collisioncomponent %s not initialized deleteme %d"),*Actor->GetName(), *Actor->CollisionComponent->GetName(), Actor->bDeleteMe);
				}
				ColCenter = Actor->CollisionComponent->Bounds.Origin;
			}
			else
			{
				ColCenter = Actor->Location;
			}

			FirstHit = MultiLineCheck
			(
				GMem,
				ColCenter + Delta,
				ColCenter,
				Actor->GetCylinderExtent(),
				TraceFlags,
				Actor
			);

			// Handle first blocking actor.
			if( Actor->bCollideWorld || Actor->bBlockActors )
			{
				Hit = FCheckResult(1.f);
				for( FCheckResult* Test=FirstHit; Test; Test=Test->GetNext() )
				{
					if( (!bIgnoreBases || !Actor->IsBasedOn(Test->Actor)) && !Test->Actor->IsBasedOn(Actor) )
					{
						MaybeTouched = 1;
						if( Actor->IsBlockedBy(Test->Actor,Test->Component) )
						{
							Hit = *Test;
							break;
						}
					}
				}
				/* logging for stuck in collision
				if ( Hit.bStartPenetrating && Actor->GetAPawn() )
				{
					if ( Hit.Actor )
						debugf(TEXT("Started penetrating %s time %f dot %f"), *Hit.Actor->GetName(), Hit.Time, (Delta.SafeNormal() | Hit.Normal));
					else
						debugf(TEXT("Started penetrating no actor time %f dot %f"), Hit.Time, (Delta.SafeNormal() | Hit.Normal));
				}
				*/
				FinalDelta = Delta * Hit.Time;
			}
		}

		if ( bMovementOccurred )
		{
			// Update the location.
			Actor->Location += FinalDelta;
			Actor->Rotation = NewRotation;

			// Update any collision components.  If we are in the Tick phase (and not in the final component update phase), only upgrade components with collision.
			// This is done before moving attached actors so they can test for encroachment based on the correct mover position.
			// It is done before touch so that components are correctly up-to-date when we call Touch events. Things like
			// Kismet's touch event do an IsOverlapping - which requires the component to be in the right location.
			// This will not fix all problems with components being out of date though - for example, attachments of an Actor are not 
			// updated at this point.
			Actor->ForceUpdateComponents( GWorld->InTick && !GWorld->bPostTickComponentUpdate );
		}
	}

	// Move the based actors (after encroachment checking).
	if( (Actor->Attached.Num() > 0) && bMovementOccurred )
	{
		// Move base.
		FRotator ReducedRotation(0,0,0);

		FMatrix OldMatrix = FRotationMatrix( OldRotation ).Transpose();
		FMatrix NewMatrix = FRotationMatrix( NewRotation );
		UBOOL bRotationChanged = (OldRotation != Actor->Rotation);
		if( bRotationChanged )
		{
			ReducedRotation = FRotator( ReduceAngle(Actor->Rotation.Pitch)	- ReduceAngle(OldRotation.Pitch),
										ReduceAngle(Actor->Rotation.Yaw)	- ReduceAngle(OldRotation.Yaw)	,
										ReduceAngle(Actor->Rotation.Roll)	- ReduceAngle(OldRotation.Roll) );
		}

		// Calculate new transform matrix of base actor (ignoring scale).
		FMatrix BaseTM = FRotationTranslationMatrix(Actor->Rotation, Actor->Location);

		FSavedPosition* SavedPositions = NULL;

		for( INT i=0; i<Actor->Attached.Num(); i++ )
		{
			// For non-skeletal based actors. Skeletal-based actors are handled inside USkeletalMeshComponent::Update
			AActor* Other = Actor->Attached(i);
			if ( Other && !Other->BaseSkelComponent )
			{
				SavedPositions = new(GMem) FSavedPosition(Other, Other->Location, Other->Rotation, SavedPositions);

				FVector   RotMotion( 0, 0, 0 );
				FCheckResult OtherHit(1.f);
				UBOOL bMoveFailed = FALSE;
				if( Other->Physics == PHYS_Interpolating || (Other->bHardAttach && !Other->bBlockActors) )
				{
					FRotationTranslationMatrix HardRelMatrix(Other->RelativeRotation, Other->RelativeLocation);
					FMatrix NewWorldTM = HardRelMatrix * BaseTM;
					FVector NewWorldPos = NewWorldTM.GetOrigin();
					FRotator NewWorldRot = NewWorldTM.Rotator();
					MoveActor( Other, NewWorldPos - Other->Location, NewWorldRot, MOVE_IgnoreBases, OtherHit );
					bMoveFailed = (OtherHit.Time < 1.f) || (NewWorldRot != Other->Rotation);
				}
				else if ( Other->bIgnoreBaseRotation )
				{
					// move attached actor, ignoring effects of any changes in its base's rotation.
					MoveActor( Other, FinalDelta, Other->Rotation, MOVE_IgnoreBases, OtherHit );
				}
				else
				{
					FRotator finalRotation = Other->Rotation + ReducedRotation;

					if( bRotationChanged )
					{
						Other->UpdateBasedRotation(finalRotation, ReducedRotation);

						// Handle rotation-induced motion.
						RotMotion = NewMatrix.TransformFVector( OldMatrix.TransformFVector( Other->RelativeLocation ) ) - Other->RelativeLocation;
					}
					// move attached actor
					MoveActor( Other, FinalDelta + RotMotion, finalRotation, MOVE_IgnoreBases, OtherHit );
				}

				if( !bNoFail && !bMoveFailed &&
					// If neither actor should check for encroachment, skip overlapping test
				   ((!Actor->bNoEncroachCheck || !Other->bNoEncroachCheck ) &&
					 Other->IsBlockedBy( Actor, Actor->CollisionComponent )) )
				{
					// check if encroached
					// IsOverlapping() returns false for based actors, so temporarily clear the base.
					UBOOL bStillBased = (Other->Base == Actor);
					if ( bStillBased )
						Other->Base = NULL;
					UBOOL bStillEncroaching = Other->IsOverlapping(Actor);
					if ( bStillBased )
						Other->Base = Actor;
					// if encroachment declined, move back to old location
					if ( bStillEncroaching && Actor->eventEncroachingOn(Other) )
					{
						bMoveFailed = TRUE;
					}
				}
				if ( bMoveFailed )
				{
					Actor->Location -= FinalDelta;
					Actor->Rotation = OldRotation;
					Actor->ForceUpdateComponents( GWorld->InTick );
					for( FSavedPosition* Pos = SavedPositions; Pos!=NULL; Pos=Pos->GetNext() )
					{
						if ( Pos->Actor && !Pos->Actor->bDeleteMe )
						{
							MoveActor( Pos->Actor, Pos->OldLocation - Pos->Actor->Location, Pos->OldRotation, MOVE_IgnoreBases, OtherHit );
							if (bRotationChanged)
							{
								Pos->Actor->ReverseBasedRotation();
							}
						}
					}
					Mark.Pop();
					return 0;
				}
			}
		}
	}

	// update relative location of this actor
	if( Actor->Base && !Actor->bHardAttach && Actor->Physics != PHYS_Interpolating && !Actor->BaseSkelComponent )
	{
		Actor->RelativeLocation = Actor->Location - Actor->Base->Location;
		
		if( !Actor->Base->bWorldGeometry && (OldRotation != Actor->Rotation) )
		{
			Actor->UpdateRelativeRotation();
		}
	}

	// Handle bump and touch notifications.
	if( Hit.Actor )
	{
		// Notification that Actor has bumped against the level.
		if( Hit.Actor->bWorldGeometry )
		{
			Actor->NotifyBumpLevel(Hit.Location, Hit.Normal);
		}
		// Notify first bumped actor unless it's the level or the actor's base.
		else if( !Actor->IsBasedOn(Hit.Actor) )
		{
			// Notify both actors of the bump.
			Hit.Actor->NotifyBump(Actor, Actor->CollisionComponent, Hit.Normal);
			Actor->NotifyBump(Hit.Actor, Hit.Component, Hit.Normal);
		}
	}

	// Handle Touch notifications.
	if( MaybeTouched || (!Actor->bBlockActors && !Actor->bCollideWorld && Actor->bCollideActors) )
	{
		for( FCheckResult* Test=FirstHit; Test && Test->Time<Hit.Time; Test=Test->GetNext() )
		{
			if ( (!bIgnoreBases || !Actor->IsBasedOn(Test->Actor)) &&
				(!Actor->IsBlockedBy(Test->Actor,Test->Component)) && Actor != Test->Actor)
			{
				Actor->BeginTouch(Test->Actor, Test->Component, Test->Location, Test->Normal);
			}
		}
	}

	// UnTouch notifications.
	Actor->UnTouchActors();

	// Set actor zone.
	Actor->SetZone(FALSE,FALSE);
	Mark.Pop();

	// Update physics 'pushing' body.
	Actor->UpdatePushBody();

#if defined(SHOW_MOVEACTOR_TAKING_LONG_TIME) || LOOKING_FOR_PERF_ISSUES
	unclock(MoveActorTakingLongTime);
	FLOAT MSec = MoveActorTakingLongTime * GSecondsPerCycle * 1000.f;
	if( MSec > SHOW_MOVEACTOR_TAKING_LONG_TIME_AMOUNT )
	{
		debugf( NAME_PerfWarning, TEXT("%10f executing MoveActor for %s"), MSec, *Actor->GetFullName() );
	}
#endif

	// Return whether we moved at all.
	return Hit.Time>0.f;
}

/** notification when actor has bumped against the level */
void AActor::NotifyBumpLevel(const FVector &HitLocation, const FVector &HitNormal)
{
}

void AActor::NotifyBump(AActor *Other, UPrimitiveComponent* OtherComp, const FVector &HitNormal)
{
	eventBump(Other, OtherComp, HitNormal);
}

void APawn::NotifyBump(AActor *Other, UPrimitiveComponent* OtherComp, const FVector &HitNormal)
{
	if( !Controller || !Controller->eventNotifyBump(Other, HitNormal) )
	{
		eventBump(Other, OtherComp, HitNormal);
	}
}


/*-----------------------------------------------------------------------------
	Encroachment.
-----------------------------------------------------------------------------*/

//
// Check whether Actor is encroaching other actors after a move, and return
// 0 to ok the move, or 1 to abort it.  If OKed, move is actually completed if Actor->IsEncroacher()
// 
//
UBOOL UWorld::CheckEncroachment
(
	AActor*			Actor,
	FVector			TestLocation,
	FRotator		TestRotation,
	UBOOL			bTouchNotify
)
{
	check(Actor);

	// If this actor doesn't need encroachment checking, allow the move.
	if (!Actor->bCollideActors && !Actor->IsEncroacher())
	{
		return 0;
	}

	// set up matrices for calculating movement caused by mover rotation
	FMatrix WorldToLocal, TestLocalToWorld;
	FVector StartLocation = Actor->Location;
	FRotator StartRotation = Actor->Rotation;
	if ( Actor->IsEncroacher() )
	{
		// get old transform
		WorldToLocal = Actor->WorldToLocal();

		// move the actor 
		if ( (Actor->Location != TestLocation) || (Actor->Rotation != TestRotation) )
		{
			Actor->Location = TestLocation;
			Actor->Rotation = TestRotation;
			Actor->ForceUpdateComponents(GWorld->InTick);
		}
		TestLocalToWorld = Actor->LocalToWorld();
	}

	FLOAT ColRadius, ColHeight;
	Actor->GetBoundingCylinder(ColRadius, ColHeight);
	UBOOL bIsZeroExtent = (ColRadius == 0.f) && (ColHeight == 0.f);

	// Query the mover about what he wants to do with the actors he is encroaching.
	FMemMark Mark(GMem);
	FCheckResult* FirstHit = Hash ? Hash->ActorEncroachmentCheck( GMem, Actor, TestLocation, TestRotation, TRACE_AllColliding ) : NULL;	
	for( FCheckResult* Test = FirstHit; Test!=NULL; Test=Test->GetNext() )
	{
		if
		(	Test->Actor!=Actor
		&&	!Test->Actor->bWorldGeometry
		&&  !Test->Actor->IsBasedOn(Actor)
		&& (Test->Component == NULL || (bIsZeroExtent ? Test->Component->BlockZeroExtent : Test->Component->BlockNonZeroExtent))
		&&	Actor->IsBlockedBy( Test->Actor, Test->Component ) )
		{
			UBOOL bStillEncroaching = true;
			// Actors can be pushed by movers or karma stuff.
			if (Actor->IsEncroacher() && !Test->Actor->IsEncroacher() && Test->Actor->bPushedByEncroachers)
			{
				// check if mover can safely push encroached actor
				// Move test actor away from mover
				FVector MoveDir = TestLocation - StartLocation;
				FVector OldLoc = Test->Actor->Location;
				FVector Dest = Test->Actor->Location + MoveDir;
				if ( TestRotation != StartRotation )
				{
					FVector TestLocalLoc = WorldToLocal.TransformFVector(Test->Actor->Location);
					// multiply X 1.5 to account for max bounding box center to colliding edge dist change
					MoveDir += 1.5f * (TestLocalToWorld.TransformFVector(TestLocalLoc) - Test->Actor->Location);
				}
				// temporarily turn off blocking for encroacher, so it won't affect the movement
				UBOOL bRealBlockActors = Actor->bBlockActors;
				Actor->bBlockActors = FALSE;
				Test->Actor->moveSmooth(MoveDir);

				// see if mover still encroaches test actor
				FCheckResult TestHit(1.f);
				bStillEncroaching = Test->Actor->IsOverlapping(Actor, &TestHit);
				if ( bStillEncroaching && Test->Actor->GetAPawn() )
				{
					// try moving away
					FCheckResult Hit(1.f);
					Actor->ActorLineCheck(Hit, Actor->Location, Test->Actor->Location,FVector(0.f,0.f,0.f), TRACE_AllColliding);
					if ( Hit.Time < 1.f )
					{
						FVector PushOut = (Hit.Normal + FVector(0.f,0.f,0.1f)) * Test->Actor->GetAPawn()->CylinderComponent->CollisionRadius;
						Test->Actor->moveSmooth(PushOut);
						MoveDir += PushOut;
						bStillEncroaching = Test->Actor->IsOverlapping(Actor, &TestHit);
					}
				}
				Actor->bBlockActors = bRealBlockActors;
				if ( !bStillEncroaching ) //push test actor back toward brush
				{
					MoveActor( Test->Actor, -1.f * MoveDir, Test->Actor->Rotation, 0, TestHit );
				}
				Test->Actor->PushedBy(Actor);
			}
			if ( bStillEncroaching && Actor->eventEncroachingOn(Test->Actor) )
			{
				Mark.Pop();
			
				// move the encroacher back
				Actor->Location = StartLocation;
				Actor->Rotation = StartRotation;
				Actor->ConditionalUpdateComponents(GWorld->InTick);
				return 1;
			}
			else 
			{
				Actor->eventRanInto(Test->Actor);
			}
		}
	}

	// If bTouchNotify, send Touch and UnTouch notifies.
	if( bTouchNotify )
	{
		// UnTouch notifications.
		Actor->UnTouchActors();
	}

	// Notify the encroached actors but not the level.
	for( FCheckResult* Test = FirstHit; Test; Test=Test->GetNext() )
		if
		(	Test->Actor!=Actor
		&&	!Test->Actor->bWorldGeometry
		&&  !Test->Actor->IsBasedOn(Actor)
		&&	Test->Actor!=GetWorldInfo()
		&& (Test->Component == NULL || (bIsZeroExtent ? Test->Component->BlockZeroExtent : Test->Component->BlockNonZeroExtent)) )
		{
			if( Actor->IsBlockedBy(Test->Actor,Test->Component) )
			{
				Test->Actor->eventEncroachedBy(Actor);
			}
			else if (bTouchNotify)
			{
				// Make sure Test->Location is not Zero, if that's the case, use TestLocation
				FVector	HitLocation = Test->Location.IsZero() ? TestLocation : Test->Location;

				// Make sure we have a valid Normal
				FVector NormalDir = Test->Normal.IsZero() ? (TestLocation - Actor->Location) : Test->Normal;
				if( !NormalDir.IsZero() )
				{
					NormalDir.Normalize();
				}
				else
				{
					NormalDir = TestLocation - Test->Actor->Location;
					NormalDir.Normalize();
				}
				Actor->BeginTouch( Test->Actor, Test->Component, HitLocation, NormalDir );
			}
		}
							
	Mark.Pop();

	// Ok the move.
	return 0;
}

/*-----------------------------------------------------------------------------
	SinglePointCheck.
-----------------------------------------------------------------------------*/

//
// Check for nearest hit.
// Return 1 if no hit, 0 if hit.
//
UBOOL UWorld::SinglePointCheck( FCheckResult&	Hit, const FVector& Location, const FVector& Extent, DWORD TraceFlags )
{
	FMemMark Mark(GMem);
	FCheckResult* Hits = MultiPointCheck( GMem, Location, Extent, TraceFlags );
	if( !Hits )
	{
		Mark.Pop();
		return 1;
	}
	Hit = *Hits;
	for( Hits = Hits->GetNext(); Hits!=NULL; Hits = Hits->GetNext() )
		if( (Hits->Location-Location).SizeSquared() < (Hit.Location-Location).SizeSquared() )
			Hit = *Hits;
	Mark.Pop();
	return 0;
}

/*
  EncroachingWorldGeometry
  return true if Extent encroaches on level, terrain, or bWorldGeometry actors
*/
UBOOL UWorld::EncroachingWorldGeometry( FCheckResult& Hit, const FVector& Location, const FVector& Extent, UBOOL bUseComplexCollision )
{
	FMemMark Mark(GMem);
	FCheckResult* Hits = MultiPointCheck( GMem, Location, Extent, TRACE_World | TRACE_Blocking | TRACE_StopAtAnyHit | (bUseComplexCollision ? TRACE_ComplexCollision : 0) );
	if ( !Hits )
	{
		Mark.Pop();
		return false;
	}
	Hit = *Hits;
	Mark.Pop();
	return true;
}

/*-----------------------------------------------------------------------------
	MultiPointCheck.
-----------------------------------------------------------------------------*/

FCheckResult* UWorld::MultiPointCheck( FMemStack& Mem, const FVector& Location, const FVector& Extent, DWORD TraceFlags )
{
	check(Hash);
	FCheckResult* Result=NULL;

	if(this->bShowPointChecks)
	{
		// Draw box showing extent of point check.
		DrawWireBox(LineBatcher,FBox(Location-Extent, Location+Extent), FColor(0, 128, 255), SDPG_World);
	}

	// Check with level.
	if( TraceFlags & TRACE_Level )
	{
		FCheckResult TestHit(1.f);
		if( BSPPointCheck( TestHit, NULL, Location, Extent )==0 )
		{
			// Hit.
			TestHit.GetNext() = Result;
			Result            = new(Mem)FCheckResult(TestHit);
			Result->Actor     = GetWorldInfo();
			if( TraceFlags & TRACE_StopAtAnyHit )
			{
				return Result;
			}
		}
	}

	// Check with actors.
	FCheckResult* ActorCheckResult = Hash->ActorPointCheck( Mem, Location, Extent, TraceFlags );
	if (Result != NULL)
	{
		// Link the actor hit in after the world
		Result->Next = ActorCheckResult;
	}
	else
	{
		Result = ActorCheckResult;
	}

	return Result;
}

/*-----------------------------------------------------------------------------
	SingleLineCheck.
-----------------------------------------------------------------------------*/

DECLARE_CYCLE_STAT(TEXT("Single Line Check"),	STAT_SingleLineCheck,	STATGROUP_Collision);
DECLARE_CYCLE_STAT(TEXT("Multi Line Check"),	STAT_MultiLineCheck,	STATGROUP_Collision);
DECLARE_CYCLE_STAT(TEXT("Check Level"),			STAT_Col_Level,			STATGROUP_Collision);
DECLARE_CYCLE_STAT(TEXT("Check Actors"),		STAT_Col_Actors,		STATGROUP_Collision);
DECLARE_CYCLE_STAT(TEXT("Check Sort"),			STAT_Col_Sort,			STATGROUP_Collision);

//
// Trace a line and return the first hit actor (Actor->bWorldGeometry means hit the world geomtry).
//
UBOOL UWorld::SingleLineCheck
(
	FCheckResult&		Hit,
	AActor*				SourceActor,
	const FVector&		End,
	const FVector&		Start,
	DWORD				TraceFlags,
	const FVector&		Extent,
	ULightComponent*	SourceLight
)
{
	SCOPE_CYCLE_COUNTER(STAT_SingleLineCheck);

	// Get list of hit actors.
	FMemMark Mark(GMem);

	//If enabled, capture the callstack that triggered this linecheck
	LINE_CHECK_TRACE(TraceFlags, &Extent);

	TraceFlags = TraceFlags | TRACE_SingleResult;
	FCheckResult* FirstHit = MultiLineCheck
	(
		GMem,
		End,
		Start,
		Extent,
		TraceFlags,
		SourceActor,
		SourceLight
	);

	if( FirstHit != NULL )
	{
		Hit = *FirstHit;

		DetermineCorrectPhysicalMaterial<FCheckResult, FCheckResult>( Hit, Hit );	

		Hit.Material = Hit.Material ? Hit.Material->GetMaterial() : NULL;
	
	}
	else
	{
		Hit.Time = 1.f;
		Hit.Actor = NULL;
	}

	Mark.Pop();
	return FirstHit==NULL;
}

/*-----------------------------------------------------------------------------
	MultiLineCheck.
-----------------------------------------------------------------------------*/

FCheckResult* UWorld::MultiLineCheck
(
	FMemStack&			Mem,
	const FVector&		End,
	const FVector&		Start,
	const FVector&		Extent,
	DWORD				TraceFlags,
	AActor*				SourceActor,
	ULightComponent*	SourceLight
)
{
	SCOPE_CYCLE_COUNTER(STAT_MultiLineCheck);

	INT NumHits=0;
	FCheckResult Hits[64];

	// Draw line that we are checking, and box showing extent at end of line, if non-zero
	if(this->bShowLineChecks && Extent.IsZero())
	{
		LineBatcher->DrawLine(Start, End, FColor(0, 255, 128), SDPG_World);
		
	}
	else if(this->bShowExtentLineChecks && !Extent.IsZero())
	{
		LineBatcher->DrawLine(Start, End, FColor(0, 255, 255), SDPG_World);
		DrawWireBox(LineBatcher,FBox(End-Extent, End+Extent), FColor(0, 255, 255), SDPG_World);
	}

	FLOAT Dilation = 1.f;
	FVector NewEnd = End;

	// Check for collision with the level, and cull by the end point for speed.
	INT WorldNum = 0;
	
	{
		SCOPE_CYCLE_COUNTER(STAT_Col_Level);
		if( (TraceFlags & TRACE_Level) && BSPLineCheck( Hits[NumHits], NULL, End, Start, Extent, TraceFlags )==0 )
		{
			Hits[NumHits].Actor = GetWorldInfo();
			FLOAT Dist = (Hits[NumHits].Location - Start).Size();
			Hits[NumHits].Time *= Dilation;
			Dilation = ::Min(1.f, Hits[NumHits].Time * (Dist + 5)/(Dist+0.0001f));
			NewEnd = Start + (End - Start) * Dilation;
			WorldNum = NumHits;
			NumHits++;
		}
	}

	if(Dilation > SMALL_NUMBER)
	{
		SCOPE_CYCLE_COUNTER(STAT_Col_Actors);
		if( !NumHits || !(TraceFlags & TRACE_StopAtAnyHit) )
		{
			// Check with actors.
			if( (TraceFlags & TRACE_Hash) && Hash )
			{
				for( FCheckResult* Link=Hash->ActorLineCheck( Mem, NewEnd, Start, Extent, TraceFlags, SourceActor, SourceLight ); Link && NumHits<ARRAY_COUNT(Hits); Link=Link->GetNext() )
				{
					Link->Time *= Dilation;
					Hits[NumHits++] = *Link;
				}
			}
		}
	}

	// Sort the list.
	FCheckResult* Result = NULL;
	if( NumHits )
	{
		SCOPE_CYCLE_COUNTER(STAT_Col_Sort);
		appQsort( Hits, NumHits, sizeof(Hits[0]), (QSORT_COMPARE)FCheckResult::CompareHits );
		Result = new(Mem,NumHits)FCheckResult;
		for( INT i=0; i<NumHits; i++ )
		{
			Result[i]      = Hits[i];
			Result[i].Next = (i+1<NumHits) ? &Result[i+1] : NULL;
		}
	}

	return Result;
}


/*-----------------------------------------------------------------------------
	BSP line/ point checking
-----------------------------------------------------------------------------*/

UBOOL UWorld::BSPLineCheck(	FCheckResult& Hit, AActor* Owner, const FVector& End, const FVector& Start, const FVector& Extent, DWORD TraceFlags )
{
	UBOOL bHit = FALSE;
	FCheckResult TotalResult(1.f);

	// Iterate over each level in the World
	for( INT LevelIndex=0; LevelIndex<Levels.Num(); LevelIndex++ )
	{
		ULevel* Level = Levels(LevelIndex);
		FCheckResult TmpResult(1.f);

		// Test against this level's BSP
		Level->Model->LineCheck( TmpResult, Owner, NULL, End, Start, Extent, TraceFlags );
		
		// If this hit is the closest one so far, remember it.
		if(TmpResult.Time < TotalResult.Time)
		{
			TotalResult = TmpResult;
			TotalResult.Level = Level;
			TotalResult.LevelIndex = LevelIndex;
			bHit = TRUE;
		}
	}

	// If we got a valid hit- set the output CheckResult.
	if(bHit)
	{
		Hit = TotalResult;
	}

	// return hit condition
	return !bHit;
}

UBOOL UWorld::BSPFastLineCheck( const FVector& End, const FVector& Start )
{
	UBOOL bBlocked  = FALSE;

	for( INT LevelIndex=0; LevelIndex<Levels.Num() && !bBlocked ; LevelIndex++ )
	{
		ULevel* Level = Levels(LevelIndex);
		bBlocked = !Level->Model->FastLineCheck( End, Start );
	}

	return !bBlocked;
}

UBOOL UWorld::BSPPointCheck( FCheckResult &Result, AActor *Owner, const FVector& Location, const FVector& Extent )
{
	UBOOL bBlocked = FALSE;

	for( INT LevelIndex=0; LevelIndex<Levels.Num(); LevelIndex++ )
	{
		ULevel* Level = Levels(LevelIndex);
		bBlocked = !Level->Model->PointCheck( Result, Owner, NULL, Location, Extent );		
		// if the check hit the level,
		if (bBlocked)
		{
			// update in the check result
			Result.Level = Level;
			Result.LevelIndex = LevelIndex;
			// and sotp furthing searching
			break;
		}
	}

	return !bBlocked;
}

/*-----------------------------------------------------------------------------
	ULevel zone functions.
-----------------------------------------------------------------------------*/

//
// Figure out which zone an actor is in, update the actor's iZone,
// and notify the actor of the zone change.  Skips the zone notification
// if the zone hasn't changed.
//

void AActor::SetZone( UBOOL bTest, UBOOL bForceRefresh )
{
	if( bDeleteMe )
	{
		return;
	}

	// update physics volume
	APhysicsVolume *NewVolume = GWorld->GetWorldInfo()->GetPhysicsVolume(Location,this,bCollideActors && !bTest && !bForceRefresh);
	if( !bTest )
	{
		if( NewVolume != PhysicsVolume )
		{
			if( PhysicsVolume )
			{
				PhysicsVolume->eventActorLeavingVolume(this);
				eventPhysicsVolumeChange(NewVolume);
			}
			PhysicsVolume = NewVolume;
			PhysicsVolume->eventActorEnteredVolume(this);
		}
	}
	else
	{
		PhysicsVolume = NewVolume;
	}
}

void APhysicsVolume::SetZone( UBOOL bTest, UBOOL bForceRefresh )
{
	if( bDeleteMe )
	{
		return;
	}

	PhysicsVolume = this;
}

void APawn::SetZone( UBOOL bTest, UBOOL bForceRefresh )
{
	if( bDeleteMe )
	{
		return;
	}

	// update physics volume
	APhysicsVolume *NewVolume = GWorld->GetWorldInfo()->GetPhysicsVolume(Location,this,bCollideActors && !bTest && !bForceRefresh);
	APhysicsVolume *NewHeadVolume = GWorld->GetWorldInfo()->GetPhysicsVolume(Location + FVector(0,0,BaseEyeHeight),this,bCollideActors && !bTest && !bForceRefresh);
	if ( NewVolume != PhysicsVolume )
	{
		if ( !bTest )
		{
			if ( PhysicsVolume )
			{
				PhysicsVolume->eventPawnLeavingVolume(this);
				eventPhysicsVolumeChange(NewVolume);
			}
			if ( Controller )
				Controller->eventNotifyPhysicsVolumeChange( NewVolume );
		}
		PhysicsVolume = NewVolume;
		if ( !bTest )
			PhysicsVolume->eventPawnEnteredVolume(this);
	}
	if ( NewHeadVolume != HeadVolume )
	{
		if ( !bTest && (!Controller || !Controller->eventNotifyHeadVolumeChange(NewHeadVolume)) )
		{
			eventHeadVolumeChange(NewHeadVolume);
		}
		HeadVolume = NewHeadVolume;
	}
	checkSlow(PhysicsVolume);
}

void AWorldInfo::SetZone( UBOOL bTest, UBOOL bForceRefresh )
{
}

// init actor volumes
void AVolume::SetVolumes(const TArray<AVolume*>& Volumes)
{
}
void AWorldInfo::SetVolumes(const TArray<AVolume*>& Volumes)
{
}
void AActor::SetVolumes(const TArray<AVolume*>& Volumes)
{
	for( INT i=0; i<Volumes.Num(); i++ )
	{
		AVolume*		V = Volumes(i);
		APhysicsVolume*	P = Cast<APhysicsVolume>(V);

		if( ((bCollideActors && V->bCollideActors) || P || V->bProcessAllActors) && V->Encompasses(Location) )
		{
			if( bCollideActors && V->bCollideActors )
			{
				V->Touching.AddItem(this);
				Touching.AddItem(V);
			}
			if( P && (P->Priority > PhysicsVolume->Priority) )
			{
				PhysicsVolume = P;
			}
			if( V->bProcessAllActors )
			{
				V->eventProcessActorSetVolume( this );
			}
		}
	}
}
void AActor::SetVolumes()
{
	for( FActorIterator It; It; ++ It )
	{
		AVolume*		V = (*It)->GetAVolume();
		APhysicsVolume*	P = Cast<APhysicsVolume>(V);
		if( V && ((bCollideActors && V->bCollideActors) || P || V->bProcessAllActors) && V->Encompasses(Location) )
		{
			if( bCollideActors && V->bCollideActors )
			{
				V->Touching.AddItem(this);
				Touching.AddItem(V);
			}
			if( P && (P->Priority > PhysicsVolume->Priority) )
			{
				PhysicsVolume = P;
			}
			if( V->bProcessAllActors )
			{
				V->eventProcessActorSetVolume( this );
			}
		}
	}
}

/** 
 *	Calculates the relative transform between the actor and its base if it has one.
 *	This transform will then be enforced in the game.
 */
void AActor::EditorUpdateBase()
{
	if( Base )
	{
		// this will only happen for placed actors at level startup
		AActor* NewBase = Base;
		USkeletalMeshComponent* NewSkelComp = BaseSkelComponent;
		FName NewBaseBoneName = BaseBoneName;

		SetBase(NULL);

		// If a bone name is specified, attempt to attach to a skeletalmeshcomponent. 
		if(NewBaseBoneName != NAME_None)
		{
			// First see if we have a Base SkeletalMeshComponent specified
			USkeletalMeshComponent* BaseSkelComp = NewSkelComp;

			// Ignore BaseSkelComponent if its Owner is not the Base.
			if(BaseSkelComp && BaseSkelComp->GetOwner() != NewBase)
			{
				BaseSkelComp = NULL;
			}

			// If not, see if the CollisionComponent in the Base is a SkeletalMesh
			if(!BaseSkelComp)
			{									
				BaseSkelComp = Cast<USkeletalMeshComponent>( NewBase->CollisionComponent );
			}

			// If that failed, see if its a pawn, and use its Mesh pointer.
			APawn* Pawn = Cast<APawn>(NewBase);
			if(!BaseSkelComp && Pawn)
			{
				BaseSkelComp = Pawn->Mesh;
			}

			// If BaseSkelComp is still NULL at this point, SetBase will fail gracefully and just attach it relative to the Actor ref frame as usual.
			SetBase( NewBase, FVector(0,0,1), 0, BaseSkelComp, NewBaseBoneName );
		}
		else // Normal case - just attaching to actor.
		{
			SetBase( NewBase, FVector(0,0,1), 0 );
		}
	}

	// explicitly clear the base when referencing actors in other levels
	if ( Base && Base->GetOuter() != GetOuter() )
	{
		SetBase(NULL);
	}
}

/** 
 *	Utility for updating all actors attached to this one to satisfy their RelativeLocation/RelativeRotation. 
 *	Should only be used in the editor!
 */
void AActor::EditorUpdateAttachedActors(const TArray<AActor*>& IgnoreActors)
{
	FRotationTranslationMatrix ActorTM( Rotation, Location );

	for( INT i=0; i<Attached.Num(); i++ )
	{
		AActor* Other = Attached(i);

		// If we have an attached actor, which isn't deleted, and we are not ignoring, update its location.
		if(Other && !Other->bDeleteMe && !IgnoreActors.ContainsItem(Other))
		{
			// Matrix of parent of this Actor
			FMatrix BaseTM;

			// If its based on a SkeletalMeshComponent, use the bone matrix as the parent transform.
			if( Other->BaseSkelComponent )
			{
				const INT BoneIndex = Other->BaseSkelComponent->MatchRefBone(Other->BaseBoneName);
				if(BoneIndex != INDEX_NONE)
				{
					BaseTM = Other->BaseSkelComponent->GetBoneMatrix(BoneIndex);
					BaseTM.RemoveScaling();
				}
				// If we couldn't find the bone - just use Actor as base.
				else
				{
					BaseTM = ActorTM;
				}
			}
			// If its not attached to a bone, just use the Actor transform.
			else
			{
				BaseTM = ActorTM;
			}

			// CAlculate relative transform, and apply it
			const FRotationTranslationMatrix HardRelMatrix(Other->RelativeRotation, Other->RelativeLocation);
			const FMatrix NewWorldTM = HardRelMatrix * BaseTM;
			const FVector NewWorldPos = NewWorldTM.GetOrigin();
			const FRotator NewWorldRot = NewWorldTM.Rotator();

			// Update actors location and rotatation.
			Other->Location = NewWorldPos;
			Other->Rotation = NewWorldRot;

			Other->InvalidateLightingCache();
			Other->ForceUpdateComponents();

			// Now update anything which are based on _this_ thing.
			// @todo: Are we sure we can't get cycles?
			Other->EditorUpdateAttachedActors(IgnoreActors);
		}
	}
}

/** 
 *	Called before the Actor is saved. 
 */
void AActor::PreSave()
{
	Super::PreSave();
	// update the base of this actor
	EditorUpdateBase();
}

/**
 * Initializes this actor when play begins.  This version marks the actor as ready to execute script, but skips
 * the rest of the stuff that actors normally do in PostBeginPlay().
 */
void AStaticMeshActorBase::PostBeginPlay()
{
	bScriptInitialized = TRUE;
}

// Allow actors to initialize themselves on the C++ side
void AActor::PostBeginPlay()
{
	// Send PostBeginPlay.
	eventPostBeginPlay();

	if( bDeleteMe )
		return;

	// Init scripting.
	eventSetInitialState();

	// Find Base
	if( !Base && bCollideWorld && bShouldBaseAtStartup && ((Physics == PHYS_None) || (Physics == PHYS_Rotating)) )
	{
		FindBase();
	}
}

void AActor::PreBeginPlay()
{
	// Send PostBeginPlay.
	eventPreBeginPlay();

	if (!bDeleteMe)
	{
		// only call zone/volume change events when spawning actors during gameplay
		SetZone(!GWorld->HasBegunPlayAndNotAssociatingLevel(), TRUE);

		// Verify that the physics mode and ticking group are compatible
		if (Physics == PHYS_RigidBody && TickGroup != TG_PostAsyncWork)
		{
			debugf(TEXT("Physics mode for (%s) is not compatible with TickGroup(%d) adjusting"),
				*GetName(),TickGroup);
			SetTickGroup(TG_PostAsyncWork);
		}
	}
}

void AVolume::SetVolumes()
{
}

/**
* Called after this instance has been serialized.
*/
void AWorldInfo::PostLoad()
{
	Super::PostLoad();

	// Force to be blocking, needed for BSP.
	bBlockActors = TRUE;

	// clamp desaturation to 0..1 (fixup for old data)
	DefaultPostProcessSettings.Scene_Desaturation = Clamp(DefaultPostProcessSettings.Scene_Desaturation, 0.f, 1.f);
}

/**
 * Called after GWorld has been set. Used to load, but not associate, all
 * levels in the world in the Editor and at least create linkers in the game.
 * Should only be called against GWorld::PersistentLevel's WorldInfo.
 */
void AWorldInfo::LoadSecondaryLevels()
{
	check( GIsEditor );

	// streamingServer
	// Only load secondary levels in the Editor, and not for commandlets.
	if( !GIsUCC
	// Don't do any work for world info actors that are part of secondary levels being streamed in! 
	&&	!GIsAsyncLoading )
	{
		for( INT LevelIndex=0; LevelIndex<StreamingLevels.Num(); LevelIndex++ )
		{
			UBOOL bLoadedLevelPackage = FALSE;
			ULevelStreaming* StreamingLevel = StreamingLevels(LevelIndex);
			if( StreamingLevel )
			{
				// Load the package and find the world object.
				UPackage* LevelPackage = UObject::LoadPackage( NULL, *StreamingLevel->PackageName.ToString(), LOAD_None );
				if( LevelPackage )
				{
					// Figure out whether main world is a PIE package.
					UBOOL bIsMainWorldAPIEPackage = (GetOutermost()->PackageFlags & PKG_PlayInEditor) ? TRUE : FALSE;
					// Worlds can only refer to PIE packages if they ar a PIE package themselves.
					if( (LevelPackage->PackageFlags & PKG_PlayInEditor) && !bIsMainWorldAPIEPackage )
					{
						appErrorf( *LocalizeUnrealEd("CannotOpenPIEMapsForEditing") );
					}

					bLoadedLevelPackage = TRUE;

					// Find the world object in the loaded package.
					UWorld* LoadedWorld	= FindObjectChecked<UWorld>( LevelPackage, TEXT("TheWorld") );

					// LoadedWorld won't be serialized as there's a BeginLoad on the stack so we manually serialize it here.
					check( LoadedWorld->GetLinker() );
					LoadedWorld->GetLinker()->Preload( LoadedWorld );

					// Keep reference to prevent garbage collection.
					check( LoadedWorld->PersistentLevel );
					StreamingLevel->LoadedLevel	= LoadedWorld->PersistentLevel;
				}
			}

			// Remove this level object if the file couldn't be found.
			if ( !bLoadedLevelPackage )
			{		
				StreamingLevels.Remove( LevelIndex-- );
				MarkPackageDirty();
			}
		}
	}
}

/**
 * Called when a property on this object has been modified externally
 *
 * @param PropertyThatChanged the property that was modified
 */
void AWorldInfo::PostEditChange(UProperty* PropertyThatChanged)
{
	// Reassociate levels in case we changed streaming behavior. Editor-only!
	if( GIsEditor )
	{
		// Load and associate levels if necessary.
		GWorld->FlushLevelStreaming();

		// Remove all currently visible levels.
		for( INT LevelIndex=0; LevelIndex<StreamingLevels.Num(); LevelIndex++ )
		{
			ULevelStreaming* StreamingLevel = StreamingLevels(LevelIndex);
			if( StreamingLevel 
			&&	StreamingLevel->LoadedLevel 
			&&	StreamingLevel->bIsVisible )
			{
				GWorld->RemoveFromWorld( StreamingLevel );
			}
		}

		// Load and associate levels if necessary.
		GWorld->FlushLevelStreaming();

		// Update the level browser so it always contains valid data
		if( GCallbackEvent )
		{
			GCallbackEvent->Send( CALLBACK_WorldChange );
		}
	}

	// clamp desaturation to 0..1
	DefaultPostProcessSettings.Scene_Desaturation = Clamp(DefaultPostProcessSettings.Scene_Desaturation, 0.f, 1.f);

	// Ensure texture size is power of two between 512 and 4096.
	PackedLightAndShadowMapTextureSize = Clamp( 1 << appCeilLogTwo( PackedLightAndShadowMapTextureSize ), 512, 4096 );

	Super::PostEditChange(PropertyThatChanged);
}

void AWorldInfo::SetVolumes()
{
}

APhysicsVolume* AWorldInfo::GetDefaultPhysicsVolume()
{
	if ( !PhysicsVolume )
	{
		PhysicsVolume = CastChecked<APhysicsVolume>(GWorld->SpawnActor(ADefaultPhysicsVolume::StaticClass()));
		PhysicsVolume->Priority = -1000000;
		PhysicsVolume->bNoDelete = true;
	}
	return PhysicsVolume;
}

APhysicsVolume* AWorldInfo::GetPhysicsVolume(const FVector& Loc, AActor* A, UBOOL bUseTouch)
{
	APhysicsVolume *NewVolume = GWorld->GetDefaultPhysicsVolume();
	if (A != NULL)
	{
		// use the base physics volume if possible for skeletal attachments
		if (A->Base != NULL && A->BaseSkelComponent != NULL)
		{
			return A->Base->PhysicsVolume ? A->Base->PhysicsVolume : NewVolume;
		}
		// if this actor has no collision
		if ( !A->bCollideActors && !A->bCollideWorld && GIsGame )
		{
			// use base's physics volume if possible
			if (A->Base != NULL)
			{
				return A->Base->PhysicsVolume ? A->Base->PhysicsVolume : NewVolume;
			}
			else
			{
				// otherwise use the default one
				return NewVolume;
			}
		}
		// check touching array for volumes if possible
		if ( bUseTouch )
		{
			for ( INT Idx = 0; Idx < A->Touching.Num(); Idx++ )
			{
				APhysicsVolume *V = Cast<APhysicsVolume>(A->Touching(Idx));
				if ( V && (V->Priority > NewVolume->Priority) && (V->bPhysicsOnContact || V->Encompasses(Loc)) )
				{
					NewVolume = V;
				}
			}
			return NewVolume;
		}
	}
	// check for all volumes at that point
	FMemMark Mark(GMem);
	for( FCheckResult* Link=GWorld->Hash->ActorPointCheck( GMem, Loc, FVector(0.f,0.f,0.f), TRACE_PhysicsVolumes); Link; Link=Link->GetNext() )
	{
		APhysicsVolume *V = (APhysicsVolume*)(Link->Actor);
		if ( V && (V->Priority > NewVolume->Priority) )
		{
			NewVolume = V;
		}
	}
	Mark.Pop();

	return NewVolume;
}

/**
 * Activates LevelStartup and/or LevelBeginning events in the sequences in the world
 *
 * @param bShouldActivateLevelStartupEvents If TRUE, will activate all LevelStartup events
 * @param bShouldActivateLevelBeginningEvents If TRUE, will activate all LevelBeginning events
 * @param bShouldActivateLevelLoadedEvents If TRUE, will activate all LevelLoadedAndVisible events
 */
void AWorldInfo::NotifyMatchStarted(UBOOL bShouldActivateLevelStartupEvents, UBOOL bShouldActivateLevelBeginningEvents, UBOOL bShouldActivateLevelLoadedEvents)
{
	for (INT LevelIdx = 0; LevelIdx < GWorld->Levels.Num(); LevelIdx++)
	{
		ULevel *Level = GWorld->Levels(LevelIdx);
		for (INT SeqIdx = 0; SeqIdx < Level->GameSequences.Num(); SeqIdx++)
		{
			USequence* Seq = Level->GameSequences(SeqIdx);
			if(Seq)
			{
				Seq->NotifyMatchStarted(bShouldActivateLevelStartupEvents, bShouldActivateLevelBeginningEvents, bShouldActivateLevelLoadedEvents);
			}
		}
	}
}

/**
 * Finds the post process settings to use for a given view location, taking into account the world's default
 * settings and the post process volumes in the world.
 * @param	ViewLocation			Current view location.
 * @param	bUseVolumes				Whether to use the world's post process volumes
 * @param	OutPostProcessSettings	Upon return, the post process settings for a camera at ViewLocation.
 * @return	If the settings came from a post process volume, the post process volume is returned.
 */
APostProcessVolume* AWorldInfo::GetPostProcessSettings(const FVector& ViewLocation,UBOOL bUseVolumes,FPostProcessSettings& OutPostProcessSettings)
{
	APostProcessVolume* Volume = NULL;
	if( bUseVolumes )
	{
		// Find the highest priority volume encompassing the current view location. This is made easier by the linked
		// list being sorted by priority. @todo: it remains to be seen whether we should trade off sorting for constant
		// time insertion/ removal time via e.g. TLinkedList.
		Volume = HighestPriorityPostProcessVolume;
		while( Volume )
		{
			// Volume encompasses, break out of loop.
			if( Volume->bEnabled && Volume->Encompasses( ViewLocation ) )
			{
				break;
			}
			// Volume doesn't encompass location, further traverse linked list.
			else
			{
				Volume = Volume->NextLowerPriorityVolume;
			}
		}
	}

	if(Volume)
	{
		OutPostProcessSettings = Volume->Settings;
	}
	else
	{
		// If first level is a FakePersistentLevel (see CommitMapChange for more info)
		// then use its world info for post process settings
		AWorldInfo* CurrentWorldInfo = this;
		if( StreamingLevels.Num() > 0 &&
			StreamingLevels(0) &&
			StreamingLevels(0)->LoadedLevel && 
			StreamingLevels(0)->IsA(ULevelStreamingPersistent::StaticClass()) )
		{
			CurrentWorldInfo = StreamingLevels(0)->LoadedLevel->GetWorldInfo();
		}

		OutPostProcessSettings = CurrentWorldInfo->DefaultPostProcessSettings;
	}

	return Volume;
}

/**
 * Finds the reverb settings to use for a given view location, taking into account the world's default
 * settings and the reverb volumes in the world.
 *
 * @param	ViewLocation			Current view location.
 * @param	bUseVolumes				Whether to use the world's reverb volumes.
 * @param	OutReverbSettings		[out] Upon return, the reverb settings for a camera at ViewLocation.
 * @return							If the settings came from a reverb volume, the reverb volume is returned.
 */
AReverbVolume* AWorldInfo::GetReverbSettings(const FVector& ViewLocation, UBOOL bUseVolumes, FReverbSettings& OutReverbSettings)
{
	AReverbVolume* Volume = NULL;
	if( bUseVolumes )
	{
		// Find the highest priority volume encompassing the current view location. This is made easier by the linked
		// list being sorted by priority. @todo: it remains to be seen whether we should trade off sorting for constant
		// time insertion/ removal time via e.g. TLinkedList.
		Volume = HighestPriorityReverbVolume;
		while( Volume )
		{
			// Volume encompasses, break out of loop.
			if( Volume->Encompasses( ViewLocation ) )
			{
				break;
			}
			// Volume doesn't encompass location, further traverse linked list.
			else
			{
				Volume = Volume->NextLowerPriorityVolume;
			}
		}
	}

	if(Volume)
	{
		OutReverbSettings = Volume->Settings;
	}
	else
	{
		// If first level is a FakePersistentLevel (see CommitMapChange for more info)
		// then use its world info for reverb settings
		AWorldInfo* CurrentWorldInfo = this;
		if( StreamingLevels.Num() > 0 &&
			StreamingLevels(0) &&
			StreamingLevels(0)->LoadedLevel && 
			StreamingLevels(0)->IsA(ULevelStreamingPersistent::StaticClass()) )
		{
			CurrentWorldInfo = StreamingLevels(0)->LoadedLevel->GetWorldInfo();
		}

		OutReverbSettings = CurrentWorldInfo->DefaultReverbSettings;
	}

	return Volume;
}

/**
 * Finds the portal volume actor at a given location
 */
APortalVolume* AWorldInfo::GetPortalVolume( const FVector& Location )
{
	for( INT i = 0; i < PortalVolumes.Num(); i++ )
	{
		APortalVolume* Volume = PortalVolumes( i );

		// Volume encompasses, break out of loop.
		if( Volume->Encompasses( Location ) )
		{
			return( Volume );
		}
	}

	return( NULL );
}

/** 
 * Remap sound locations through portals
 */
FVector AWorldInfo::RemapLocationThroughPortals( const FVector& SourceLocation, const FVector& ListenerLocation )
{
	// Deconst the variable
	FVector ModifiedSourceLocation = SourceLocation;

	// Account for the fact we are in a different portal volume
	APortalVolume* SpeakerVolume = GetPortalVolume( SourceLocation );
	APortalVolume* ListenerVolume = GetPortalVolume( ListenerLocation );
	if( SpeakerVolume && ListenerVolume && ( ListenerVolume != SpeakerVolume ) )
	{
		// Find the destination portal (if any) that is in the same portal volume as the listener
		for( INT i = 0; i < ListenerVolume->Portals.Num(); i++ )
		{
			// Get the teleporter that the listener is next to
			APortalTeleporter* SourceTeleporter = ListenerVolume->Portals( i );
			for( INT j = 0; j < SpeakerVolume->Portals.Num(); j++ )
			{
				// Get the teleporter that is next to the sound
				APortalTeleporter* DestinationTeleporter = SpeakerVolume->Portals( j ); 

				// If they are the same - we can hear the sound through the portal
				if( SourceTeleporter->SisterPortal == DestinationTeleporter )
				{
					// If there is a portal link - map the sound location to the portal location
					ModifiedSourceLocation = SourceLocation - DestinationTeleporter->Location + SourceTeleporter->Location;
					return( ModifiedSourceLocation );
				}
			}
		}
	}

	return( ModifiedSourceLocation );
}

/**
 * Sets bMapNeedsLightingFullyRebuild to the specified value.  Marks the worldinfo package dirty if the value changed.
 *
 * @param	bInMapNeedsLightingFullyRebuild			The new value.
 */
void AWorldInfo::SetMapNeedsLightingFullyRebuilt(UBOOL bInMapNeedsLightingFullyRebuild)
{
	check(IsInGameThread());
	if ( bMapNeedsLightingFullyRebuilt != bInMapNeedsLightingFullyRebuild )
	{
		// Save the lighting invalidation for transactions.
		Modify();

		bMapNeedsLightingFullyRebuilt = bInMapNeedsLightingFullyRebuild;
		MarkPackageDirty();
	}
}
