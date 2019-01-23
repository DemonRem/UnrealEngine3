//=============================================================================
// Copyright 1998-2007 Epic Games - All Rights Reserved.
// Confidential.
//=============================================================================

#include "UTGame.h"
#include "UTGameVehicleClasses.h"
#include "UTGameAIClasses.h"
#include "UTGameOnslaughtClasses.h"
#include "UnNet.h"

static inline UBOOL NEQ(const FTakeHitInfo& A, const FTakeHitInfo& B, UPackageMap* Map, UActorChannel* Channel)
{
	return (A != B);
}

static inline UBOOL NEQ(const FDrivenWeaponPawnInfo& A, const FDrivenWeaponPawnInfo& B, UPackageMap* Map, UActorChannel* Channel)
{
	// it's important that we always check both objects so that Channel->ActorMustStayDirty will be set to true if either is not serializable
	UBOOL bResult = NEQ(A.BaseVehicle, B.BaseVehicle, Map, Channel);
	bResult = (NEQ(A.PRI, B.PRI, Map, Channel) || bResult);
	return (bResult || A.SeatIndex != B.SeatIndex);
}

static inline UBOOL NEQ(const FCustomCharData& A, const FCustomCharData& B, UPackageMap* Map, UActorChannel* Channel)
{
	// should only be replicated once, when the PRI holding it is bNetInitial, so it seems like a waste of CPU to compare all those strings here
	return TRUE;
}

static inline UBOOL NEQ(const FPlayEmoteInfo& A, const FPlayEmoteInfo& B, UPackageMap* Map, UActorChannel* Channel)
{
	return( (A.EmoteTag != B.EmoteTag) ||
			(A.EmoteID != B.EmoteID) ||
			(A.bNewData != B.bNewData) );
}

INT* AUTPawn::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);
	
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);	
	if ( bNetDirty )
	{
		if ( bNetOwner )
		{
			DOREP(UTPawn,ShieldBeltArmor);
			DOREP(UTPawn,HelmetArmor);
			DOREP(UTPawn,VestArmor);
			DOREP(UTPawn,ThighpadArmor);
			DOREP(UTPawn,bHasHoverboard);
		}
		else
		{
			DOREP(UTPawn,DrivenWeaponPawn);
			DOREP(UTPawn,bDualWielding);
			DOREP(UTPawn,bPuttingDownWeapon);
		}
		DOREP(UTPawn,bFeigningDeath);
		DOREP(UTPawn,CurrentWeaponAttachmentClass);
		DOREP(UTPawn,bIsTyping);
		DOREP(UTPawn,BodyMatColor);
		DOREP(UTPawn,ClientBodyMatDuration);
		DOREP(UTPawn,HeadScale);
		DOREP(UTPawn,PawnAmbientSoundCue);
		DOREP(UTPawn,WeaponAmbientSoundCue);
		DOREP(UTPawn,ReplicatedBodyMaterial);
		DOREP(UTPawn,OverlayMaterialInstance);
		DOREP(UTPawn,WeaponOverlayFlags);
		DOREP(UTPawn,CustomGravityScaling);

		if (WorldInfo->TimeSeconds - LastEmoteTime <= MinTimeBetweenEmotes)
		{
			DOREP(UTPawn,EmoteRepInfo);
		}
		if (WorldInfo->TimeSeconds < LastTakeHitTimeout)
		{
			DOREP(UTPawn,LastTakeHitInfo);
		}
		if( bTearOff )
		{
			DOREP(UTPawn,bTearOffGibs);
		}
	}

	return Ptr;
}

INT* AUTWeapon::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);
	
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);	
	if ( bNetDirty )
	{
		DOREP(UTWeapon,AmmoCount);
		DOREP(UTWeapon,bLockedOnTarget);
		DOREP(UTWeapon,LockedTarget);
	}

	return Ptr;
}

INT* AUTWeaponPickupFactory::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);
	
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);	
	if (bNetInitial)
	{
		DOREP(UTWeaponPickupFactory,WeaponPickupClass);
	}
	
	return Ptr;
}

INT* AUTVehicleFactory::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);

	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);	
	if (bNetDirty)
	{
		DOREP(UTVehicleFactory,bHasLockedVehicle);
		if (bReplicateChildVehicle)
		{
			DOREP(UTVehicleFactory,ChildVehicle);
		}
	}

	return Ptr;
}

INT* AUTVehicleWeapon::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);
	
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);	
	if (bNetInitial && bNetOwner)
	{
		DOREP(UTVehicleWeapon,SeatIndex);
		DOREP(UTVehicleWeapon,MyVehicle);
	}

	return Ptr;
}

INT* AUTWeaponPawn::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);

	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);	
	if (bNetDirty)
	{
		DOREP(UTWeaponPawn,MySeatIndex);
		DOREP(UTWeaponPawn,MyVehicle);
		DOREP(UTWeaponPawn,MyVehicleWeapon);
	}
	
	return Ptr;
}

INT* AUTGameObjective::GetOptimizedRepList(BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel)
{
	Ptr = Super::GetOptimizedRepList(Recent, Retire, Ptr, Map, Channel);

	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);
	if (bNetDirty)
	{
		DOREP(UTGameObjective,DefenderTeamIndex);
		DOREP(UTGameObjective,bUnderAttack);
	}

	return Ptr;
}

INT* AUTOnslaughtObjective::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);
	
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);
	if (bNetDirty)
	{
		DOREP(UTOnslaughtObjective,Health);
	}

	return Ptr;
}

INT* AUTOnslaughtPanelNode::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	Ptr = Super::GetOptimizedRepList(Recent, Retire, Ptr, Map, Channel);

	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);
	if (bNetDirty)
	{
		DOREP(UTOnslaughtPanelNode,ReplicatedNumPanelsBlownOff);
	}

	return Ptr;
}

INT* AUTOnslaughtPowerCore::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);
	
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);	
	if (bNetDirty)
	{
		DOREP(UTOnslaughtPowerCore,ProcessedTarydium);
	}

	return Ptr;
}

INT* AUTPlayerReplicationInfo::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);
	
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);
	if (bNetDirty)
	{
		if ( bNetOwner )
		{
			DOREP(UTPlayerReplicationInfo,StartObjective);
		}
		DOREP(UTPlayerReplicationInfo,CustomReplicationInfo);
		DOREP(UTPlayerReplicationInfo,bHolding);
		DOREP(UTPlayerReplicationInfo,Squad);
		DOREP(UTPlayerReplicationInfo,SecondaryPlayerLocationHint);
		if (CharacterDataChangeCount != ((AUTPlayerReplicationInfo*)Recent)->CharacterDataChangeCount)
		{
			DOREP(UTPlayerReplicationInfo, CharacterData);
			((AUTPlayerReplicationInfo*)Recent)->CharacterDataChangeCount = CharacterDataChangeCount;
		}
	}
	if ( !bNetOwner )
	{
		for ( INT i=0; i<WorldInfo->ReplicationViewers.Num(); i++ )
		{
			// @TODO FIXMESTEVE - is this slow? - instead, set this once at start of replication!
			AUTPlayerController *ViewerPC = Cast<AUTPlayerController>(WorldInfo->ReplicationViewers(i).InViewer);
			if ( ViewerPC && ViewerPC->bViewingMap ) 
			{
				AController* C = (Owner != NULL) ? Owner->GetAController() : NULL;
				if (C != NULL && C->Pawn != NULL && !Map->CanSerializeObject(C->Pawn))
				{
					HUDPawnClass = C->Pawn->GetClass();
					HUDPawnLocation = C->Pawn->Location;
					DOREP(UTPlayerReplicationInfo,HUDPawnClass);
					DOREP(UTPlayerReplicationInfo,HUDPawnLocation);
				}
				break;
			}
		}
	}

	return Ptr;
}

INT* AUTLinkedReplicationInfo::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);
	
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);	
	if (bNetDirty)
	{
		DOREP(UTLinkedReplicationInfo,NextReplicationInfo);
	}

	return Ptr;
}

INT* AUTPickupFactory::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);	

	if ( bNetDirty )
	{
		DOREP(UTPickupFactory,bPulseBase);
		DOREP(UTPickupFactory,bIsRespawning);
	}

	return Ptr;
}

INT* AUTVehicle::GetOptimizedRepList( BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel )
{
	Ptr = Super::GetOptimizedRepList(Recent,Retire,Ptr,Map,Channel);
	
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);	
	if (bNetDirty)
	{
		if (!bNetOwner)
		{
			DOREP(UTVehicle,WeaponRotation);
		}
		DOREP(UTVehicle,bTeamLocked);
		DOREP(UTVehicle,Team);
		DOREP(UTVehicle,bDeadVehicle);
		DOREP(UTVehicle,bPlayingSpawnEffect);
		DOREP(UTVehicle,bIsDisabled);
		DOREP(UTVehicle,SeatMask);
		DOREP(UTVehicle,PassengerPRI);
		DOREP(UTVehicle,ForceMovementDirection);
		if (WorldInfo->TimeSeconds < LastTakeHitTimeout)
		{
			DOREP(UTVehicle,LastTakeHitInfo);
		}
		if (KillerController == NULL || Map->CanSerializeObject(KillerController))
		{
			DOREP(UTVehicle,KillerController);
		}
	}
	return Ptr;
}

INT* AUTProjectile::GetOptimizedRepList(BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel)
{
	checkSlow(StaticClass()->ClassFlags & CLASS_NativeReplication);

	if (bNetDirty)
	{
		if (bReplicateInstigator)
		{
			if (!bNetTemporary || (InstigatorBaseVehicle != NULL && Map->CanSerializeObject(InstigatorBaseVehicle)))
			{
				DOREP(UTProjectile,InstigatorBaseVehicle);
			}
		
			// don't ever stay dirty for unserializable Instigator if we have InstigatorBaseVehicle
			if (Instigator == NULL || (!bNetTemporary && InstigatorBaseVehicle == NULL && ((AUTProjectile*)Recent)->InstigatorBaseVehicle == NULL) || Map->CanSerializeObject(Instigator))
			{
				DOREP(Actor,Instigator);
			}
		}

		if (bNetInitial)
		{
			DOREP(UTProjectile,bWideCheck);
		}
	}

	// we already handled Instigator replication, so temporarily disable bReplicateInstigator for AActor version
	UBOOL bOldReplicateInstigator = bReplicateInstigator;
	bReplicateInstigator = FALSE;
	Ptr = Super::GetOptimizedRepList(Recent, Retire, Ptr, Map, Channel);
	bReplicateInstigator = bOldReplicateInstigator;

	return Ptr;
}

//--------------------------------------------------------------
