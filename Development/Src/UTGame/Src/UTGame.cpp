//=============================================================================
// Copyright 1998-2007 Epic Games - All Rights Reserved.
// Confidential.
//=============================================================================

#include "UTGame.h"
#include "EngineMaterialClasses.h"
#include "EngineAnimClasses.h"
#include "UnPath.h"

// These are not included in UTGame.h, so included here.
#include "UTGameAnimationClasses.h"
#include "UTGameSequenceClasses.h"
#include "UTGameUIClasses.h"
#include "UTGameAIClasses.h"
#include "UTGameVehicleClasses.h"
#include "UTGameOnslaughtClasses.h"
#include "UTGameUIFrontEndClasses.h"

#define STATIC_LINKING_MOJO 1

// Register things.
#define NAMES_ONLY
#define AUTOGENERATE_NAME(name) FName UTGAME_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name) IMPLEMENT_FUNCTION(cls,idx,name)
#include "UTGameClasses.h"
#include "UTGameAnimationClasses.h"
#include "UTGameSequenceClasses.h"
#include "UTGameUIClasses.h"
#include "UTGameAIClasses.h"
#include "UTGameVehicleClasses.h"
#include "UTGameOnslaughtClasses.h"
#include "UTGameUIFrontEndClasses.h"
#undef AUTOGENERATE_FUNCTION
#undef AUTOGENERATE_NAME
#undef NAMES_ONLY

// Register natives.
#define NATIVES_ONLY
#define NAMES_ONLY
#define AUTOGENERATE_NAME(name)
#define AUTOGENERATE_FUNCTION(cls,idx,name)
#include "UTGameClasses.h"
#include "UTGameAnimationClasses.h"
#include "UTGameSequenceClasses.h"
#include "UTGameUIClasses.h"
#include "UTGameAIClasses.h"
#include "UTGameVehicleClasses.h"
#include "UTGameOnslaughtClasses.h"
#include "UTGameUIFrontEndClasses.h"
#undef AUTOGENERATE_FUNCTION
#undef AUTOGENERATE_NAME
#undef NATIVES_ONLY
#undef NAMES_ONLY

/**
 * Initialize registrants, basically calling StaticClass() to create the class and also 
 * populating the lookup table.
 *
 * @param	Lookup	current index into lookup table
 */
void AutoInitializeRegistrantsUTGame( INT& Lookup )
{
	AUTO_INITIALIZE_REGISTRANTS_UTGAME;
	AUTO_INITIALIZE_REGISTRANTS_UTGAME_ANIMATION;
	AUTO_INITIALIZE_REGISTRANTS_UTGAME_SEQUENCE;
	AUTO_INITIALIZE_REGISTRANTS_UTGAME_UI;
	AUTO_INITIALIZE_REGISTRANTS_UTGAME_AI;
	AUTO_INITIALIZE_REGISTRANTS_UTGAME_VEHICLE;
	AUTO_INITIALIZE_REGISTRANTS_UTGAME_ONSLAUGHT;
	AUTO_INITIALIZE_REGISTRANTS_UTGAME_UIFRONTEND;
}

/**
 * Auto generates names.
 */
void AutoGenerateNamesUTGame()
{
	#define NAMES_ONLY
	#define AUTOGENERATE_FUNCTION(cls,idx,name)
    #define AUTOGENERATE_NAME(name) UTGAME_##name = FName(TEXT(#name));
	#include "UTGameClasses.h"
	#include "UTGameAnimationClasses.h"
	#include "UTGameSequenceClasses.h"
	#include "UTGameUIClasses.h"
	#include "UTGameAIClasses.h"
	#include "UTGameVehicleClasses.h"
	#include "UTGameOnslaughtClasses.h"
	#include "UTGameUIFrontEndClasses.h"
	#undef AUTOGENERATE_FUNCTION
	#undef AUTOGENERATE_NAME
	#undef NAMES_ONLY
}


IMPLEMENT_CLASS(UUTTypes);
IMPLEMENT_CLASS(AUTEmitter);
IMPLEMENT_CLASS(AUTReplicatedEmitter);
IMPLEMENT_CLASS(UUTCheatManager);
IMPLEMENT_CLASS(AUTPickupFactory);
IMPLEMENT_CLASS(AUTAmmoPickupFactory);
IMPLEMENT_CLASS(AUTPowerupPickupFactory);
IMPLEMENT_CLASS(AUTWeaponLocker);
IMPLEMENT_CLASS(AUTGameObjective);
IMPLEMENT_CLASS(UUTActorFactoryVehicle);
IMPLEMENT_CLASS(UUTSkeletalMeshComponent);
IMPLEMENT_CLASS(AUTCarriedObject);
IMPLEMENT_CLASS(AUTGameStats);
IMPLEMENT_CLASS(AUTDeployable);
IMPLEMENT_CLASS(AUTDeployedActor);
IMPLEMENT_CLASS(AUTPlayerReplicationInfo);
IMPLEMENT_CLASS(AUTLinkedReplicationInfo);
IMPLEMENT_CLASS(UUTPhysicalMaterialProperty);
IMPLEMENT_CLASS(UUTActorFactoryMover);
IMPLEMENT_CLASS(UUTSkelControl_CoreSpin);
IMPLEMENT_CLASS(UUTActorFactoryPickup);
IMPLEMENT_CLASS(AUTProj_Grenade);
IMPLEMENT_CLASS(AUTKActor);
IMPLEMENT_CLASS(AUTInventory);
IMPLEMENT_CLASS(AUTTimedPowerup);
IMPLEMENT_CLASS(UUTActorFactoryAI);
IMPLEMENT_CLASS(AUTTeamInfo);
IMPLEMENT_CLASS(UUTMapInfo);
IMPLEMENT_CLASS(AUTDroppedPickup);
IMPLEMENT_CLASS(AUTItemPickupFactory);
IMPLEMENT_CLASS(AUTGame);
IMPLEMENT_CLASS(UUTMapMusicInfo);
IMPLEMENT_CLASS(UUTParticleSystemComponent);
IMPLEMENT_CLASS(AUTHUD);
IMPLEMENT_CLASS(AUTCTFFlag);
IMPLEMENT_CLASS(AUTInventoryManager);
IMPLEMENT_CLASS(AForcedDirVolume);
IMPLEMENT_CLASS(AUTMutator);
IMPLEMENT_CLASS(AUTGameReplicationInfo);
IMPLEMENT_CLASS(AUTEntryPlayerController);
IMPLEMENT_CLASS(AUTSlowVolume);
IMPLEMENT_CLASS(AUTEmitterPool);

IMPLEMENT_CLASS(AUTVoteCollector);
IMPLEMENT_CLASS(AUTVoteReplicationInfo);

void AUTDroppedPickup::TickSpecial(FLOAT DeltaSeconds)
{
	Super::TickSpecial(DeltaSeconds);
	if(PickupMesh && WorldInfo->NetMode != NM_DedicatedServer )
	{
		if ( (bFadeOut || bRotatingPickup) && (WorldInfo->TimeSeconds - LastRenderTime < 0.2f) )
		{
			PickupMesh->Rotation.Yaw += appRound(DeltaSeconds * YawRotationRate);
			PickupMesh->BeginDeferredUpdateTransform();
		}
		if ( bFadeOut )
		{
			PickupMesh->Scale = ::Max(0.01f, PickupMesh->Scale - StartScale * DeltaSeconds);
			PickupMesh->BeginDeferredUpdateTransform();
		}
	}
}

void AUTPickupFactory::TickSpecial( FLOAT DeltaSeconds )
{
	Super::TickSpecial(DeltaSeconds);
	
	if(BaseMesh && BaseMaterialInstance && BaseMaterialParamName != NAME_None && WorldInfo->NetMode != NM_DedicatedServer && WorldInfo->TimeSeconds - LastRenderTime < 0.2f ) 
	{
		if(Glow != NULL && BasePulseTime > 0.0)
		{
			Glow->SetFloatParameter(GlowEmissiveParam,BaseEmissive.A);
		}

		// if this pickup wants to fade in when respawning
		if(bIsRespawning && ( bDoVisibilityFadeIn == TRUE ) && ( MIC_Visibility != NULL ) )
		{
			FLOAT ResVal = 0.0f;
			MIC_Visibility->GetScalarParameterValue(VisibilityParamName,ResVal);
			if(ResVal != 0.0f)
			{
				ResVal *= 1.5;
				if(ResVal < 0.0f) //clamp
				{
					ResVal = 0.0f;
				}

				MIC_Visibility->SetScalarParameterValue(VisibilityParamName, (ResVal-DeltaSeconds)/1.5f  );
			}
		}
	}

	if ( bUpdatingPickup && PickupMesh && !PickupMesh->HiddenGame && WorldInfo->TimeSeconds - LastRenderTime < 0.2f )
	{
		if ( bRotatingPickup )
		{
			PickupMesh->Rotation.Yaw += appRound(DeltaSeconds * YawRotationRate);

			// @TODO FIXMESTEVE Better way to change pivot point?
			FRotationMatrix R(PickupMesh->Rotation);
			PickupMesh->Translation = PivotTranslation.X * R.GetAxis(0) + PivotTranslation.Y * R.GetAxis(1) + FVector(0,0,PickupMesh->Translation.Z);
		}

		if ( bFloatingPickup )
		{
			BobTimer += DeltaSeconds;
			const FLOAT Offset = BobOffset * appSin(BobTimer * BobSpeed);
			PickupMesh->Translation.Z = BobBaseOffset + Offset;
		}
		// this is just visual, so we don't need to update it immediately
		PickupMesh->BeginDeferredUpdateTransform();
	}

	// Pulse
	if ( BaseMesh && BaseMaterialInstance && WorldInfo->NetMode != NM_DedicatedServer && BaseMaterialParamName != NAME_None )
	{
		//@note: we don't check LastRenderTime here so that the base doesn't wait to start the pulse until it becomes visible
		// which would defeat the purpose of it indicating when the pickup will respawn
		if (BasePulseTime > 0.0)
		{
			BaseEmissive.R += (BaseTargetEmissive.R - BaseEmissive.R) * (DeltaSeconds / BasePulseTime);
			BaseEmissive.G += (BaseTargetEmissive.G - BaseEmissive.G) * (DeltaSeconds / BasePulseTime);
			BaseEmissive.B += (BaseTargetEmissive.B - BaseEmissive.B) * (DeltaSeconds / BasePulseTime);
			BaseEmissive.A += (BaseTargetEmissive.A - BaseEmissive.A) * (DeltaSeconds / BasePulseTime);
			
			BasePulseTime -= DeltaSeconds;
			if ( bPulseBase && BasePulseTime <= 0 )
			{
				if ( BaseTargetEmissive == BaseDimEmissive )
				{
					BaseTargetEmissive = BaseBrightEmissive;
				}
				else
				{
					BaseTargetEmissive = BaseDimEmissive;
				}

				BasePulseTime = BasePulseRate;
			}
		}
		else
		{
			BaseEmissive = BaseTargetEmissive;
			BasePulseTime = 0.0;
		}

		if (WorldInfo->TimeSeconds - LastRenderTime < 0.2f)
		{
			BaseMaterialInstance->SetVectorParameterValue(BaseMaterialParamName, BaseEmissive);
		}
	}
}

void AUTPowerupPickupFactory::TickSpecial(FLOAT DeltaSeconds)
{
	Super::TickSpecial(DeltaSeconds);

	if (WorldInfo->NetMode != NM_DedicatedServer && Spinner != NULL && !Spinner->HiddenGame && WorldInfo->TimeSeconds - LastRenderTime < 0.2f)
	{
		Spinner->Rotation.Yaw += appRound(DeltaSeconds * YawRotationRate);
		// this is just visual, so we don't need to update it immediately
		Spinner->BeginDeferredUpdateTransform();
	}

	if ( ParticleEffects != NULL && !ParticleEffects->HiddenGame && WorldInfo->NetMode != NM_DedicatedServer && WorldInfo->TimeSeconds - LastRenderTime < 0.2f)
	{
		if ( PickupMesh )
		{
			ParticleEffects->Rotation = PickupMesh->Rotation;
		}
		// this is just visual, so we don't need to update it immediately
		ParticleEffects->BeginDeferredUpdateTransform();
	}
}

void AUTAmmoPickupFactory::TransformAmmoType(UClass* NewAmmoClass)
{
	if (NewAmmoClass == NULL)
	{
		debugf(NAME_Warning, TEXT("NewAmmoClass of None passed to UTAmmoPickupFactory::TransformAmmoType()"));
	}
	else
	{
		UObject* OldDefault = GetClass()->GetDefaultObject();
		UObject* NewDefault = NewAmmoClass->GetDefaultObject();

		// create the instancing graph for copying components
		FObjectInstancingGraph InstancingGraph;
		InstancingGraph.SetDestinationRoot(this, NewDefault);

		ClearComponents();
		
		// copy non-native, non-duplicatetransient properties from this class and superclasses
		// skip editable properties that have been modified
		// only copy component properties from classes above PickupFactory
		// on client, also don't copy replicated properties (assume server sent correct value)
		for (UProperty* Property = StaticClass()->PropertyLink; Property != NULL; Property = Property->PropertyLinkNext)
		{
			if ( !(Property->PropertyFlags & CPF_Native) && !(Property->PropertyFlags & CPF_DuplicateTransient) &&
				(!(Property->PropertyFlags & CPF_Edit) || Property->Identical((BYTE*)this + Property->Offset, (BYTE*)OldDefault + Property->Offset)) &&
				(Property->GetOwnerClass()->IsChildOf(APickupFactory::StaticClass()) || (Property->PropertyFlags & CPF_Component)) &&
				(Role == ROLE_Authority || !(Property->PropertyFlags & CPF_Net)) )
			{
				Property->CopyCompleteValue((BYTE*)this + Property->Offset, (BYTE*)NewDefault + Property->Offset, NULL, this, &InstancingGraph);
			}
		}

		// instance components we copied references to
		StaticClass()->InstanceComponentTemplates((BYTE*)this, (BYTE*)NewDefault, NewDefault->GetClass()->GetPropertiesSize(), this, &InstancingGraph);

		ForceUpdateComponents(FALSE, FALSE);

		TransformedClass = NewAmmoClass;

		eventInitPickupMeshEffects();
	}
}

void AUTWeaponLocker::TickSpecial(FLOAT DeltaTime)
{
	Super::TickSpecial(DeltaTime);

	if (bPlayerNearby && bIsActive && ScaleRate > 0.f)
	{
		for (INT i = 0; i < Weapons.Num(); i++)
		{
			if (Weapons(i).PickupMesh != NULL && Abs(Weapons(i).PickupMesh->Scale3D.X - 1.0f) > KINDA_SMALL_NUMBER)
			{
				Weapons(i).PickupMesh->Scale3D.X = Min<FLOAT>(1.0f, Weapons(i).PickupMesh->Scale3D.X + (ScaleRate * DeltaTime));
				// this is just visual, so we don't need to update it immediately
				Weapons(i).PickupMesh->BeginDeferredUpdateTransform();
			}
		}
	}
}

void AUTVehicleFactory::TickSpecial( FLOAT DeltaSeconds )
{
	Super::TickSpecial(DeltaSeconds);
	
	if (RespawnProgress > 0.f)
	{
		RespawnProgress -= DeltaSeconds * RespawnRateModifier;
		if (RespawnProgress <= 0.f)
		{
			eventSpawnVehicle();
		}
	}
}

void AUTVehicleFactory::CheckForErrors()
{
	Super::CheckForErrors();

	// throw an error if multiple enabled vehicle factories overlap
	UBOOL bFoundThis = FALSE;
	for (FActorIterator It; It; ++It)
	{
		// don't start testing until we've found ourselves - avoids getting two errors for each pair of factories that overlap
		if (*It == this)
		{
			bFoundThis = TRUE;
		}
		else if (bFoundThis)
		{
			AUTVehicleFactory* OtherFactory = Cast<AUTVehicleFactory>(*It);
			// if they overlap and they'll both spawn vehicles for the same team
			if ( OtherFactory != NULL && (OtherFactory->Location - Location).SizeSquared() < Square(CylinderComponent->CollisionRadius + OtherFactory->CylinderComponent->CollisionRadius) &&
					(TeamSpawningControl == TS_All || OtherFactory->TeamSpawningControl == TS_All || TeamSpawningControl == OtherFactory->TeamSpawningControl) )
			{
				// check if they can both be active simultaneously
				UBOOL bActiveSimultaneously = (!bDisabled && !OtherFactory->bDisabled);
				FName ActiveLinkSetup = NAME_None;
				UUTOnslaughtMapInfo* ONSInfo = Cast<UUTOnslaughtMapInfo>(WorldInfo->GetMapInfo());
				if (ONSInfo != NULL)
				{
					bActiveSimultaneously = FALSE;
					for (INT i = 0; i < ONSInfo->LinkSetups.Num(); i++)
					{
						const FLinkSetup& Setup = ONSInfo->LinkSetups(i);
						
						UBOOL bNewThisDisabled = bDisabled;
						UBOOL bNewOtherDisabled = OtherFactory->bDisabled;

						if (Setup.ActivatedActors.ContainsItem(this) || Setup.ActivatedGroups.ContainsItem(Group))
						{
							bNewThisDisabled = FALSE;
						}
						else if (Setup.DeactivatedActors.ContainsItem(this) || Setup.DeactivatedGroups.ContainsItem(Group))
						{
							bNewThisDisabled = TRUE;
						}
						if (Setup.ActivatedActors.ContainsItem(OtherFactory) || Setup.ActivatedGroups.ContainsItem(OtherFactory->Group))
						{
							bNewOtherDisabled = FALSE;
						}
						else if (Setup.DeactivatedActors.ContainsItem(OtherFactory) || Setup.DeactivatedGroups.ContainsItem(OtherFactory->Group))
						{
							bNewOtherDisabled = TRUE;
						}

						if (!bNewThisDisabled && !bNewOtherDisabled)
						{
							bActiveSimultaneously = TRUE;
							ActiveLinkSetup = Setup.SetupName;
							break;
						}
					}
				}

				if (bActiveSimultaneously)
				{
					if (ActiveLinkSetup != NAME_None)
					{
						GWarn->MapCheck_Add(MCTYPE_ERROR, this, *FString::Printf(TEXT("Vehicle factory is too close to other factory '%s' that can be active simultaneously in link setup '%s'"), *OtherFactory->GetName(), *ActiveLinkSetup.ToString()));
					}
					else
					{
						GWarn->MapCheck_Add(MCTYPE_ERROR, this, *FString::Printf(TEXT("Vehicle factory is too close to other factory '%s' that can be active simultaneously"), *OtherFactory->GetName()));
					}
				}
			}
		}
	}
}



//======================================================================
// Commandlet for level error checking
IMPLEMENT_CLASS(UUTLevelCheckCommandlet)

INT UUTLevelCheckCommandlet::Main( const FString& Params )
{
	// Retrieve list of all packages in .ini paths.
	TArray<FString> PackageList = GPackageFileCache->GetPackageFileList();
	if( !PackageList.Num() )
		return 0;

	// Iterate over all level packages.
	FString MapExtension = GConfig->GetStr(TEXT("URL"), TEXT("MapExt"), GEngineIni);
	for( INT PackageIndex = 0; PackageIndex < PackageList.Num(); PackageIndex++ )
	{
		const FFilename& Filename = PackageList(PackageIndex);

		if (Filename.GetExtension() == MapExtension)
		{
			warnf(NAME_Log, TEXT("------------------------------------"));
			warnf(NAME_Log, TEXT("Loading %s"), *Filename);
			warnf(NAME_Log, TEXT("...................................."));

			// Assert if package couldn't be opened so we have no chance of messing up saving later packages.
			UPackage*	Package = CastChecked<UPackage>(UObject::LoadPackage( NULL, *Filename, 0 ));

			// We have to use GetPackageLinker as GetLinker() returns NULL for top level packages.
			BeginLoad();
			ULinkerLoad* Linker	= UObject::GetPackageLinker( Package, NULL, 0, NULL, NULL );
			EndLoad();

			warnf(NAME_Log, TEXT("Checking Map..."), *Filename);

			// Check all actors for Errors

			for( TObjectIterator<AActor> It; It; ++It )
			{
				AActor* Actor = *It;
				if (Actor)
				{
					Actor->CheckForErrors();
				}
					
			}

			const UWorld* World = FindObject<UWorld>( Package, TEXT("TheWorld") );
			if ( World )
			{
				AWorldInfo* WorldInfo = World->GetWorldInfo();
				if ( WorldInfo )
					warnf(NAME_Log,TEXT("Gravity %f"),WorldInfo->GlobalGravityZ);
			}

			warnf(NAME_Log, TEXT("...................................."));
			warnf(NAME_Log, TEXT("Process Completed"));
			warnf(NAME_Log, TEXT("------------------------------------"));
			warnf(NAME_Log, TEXT(""));

			// Garbage collect to restore clean plate.
			UObject::CollectGarbage(RF_Native);
		}
	}
	return 0;
}

//======================================================================
// Commandlet for replacing one kind of actor with another kind of actor, copying changed properties from the most-derived common superclass
IMPLEMENT_CLASS(UUTReplaceActorCommandlet)

INT UUTReplaceActorCommandlet::Main(const FString& Params)
{
	const TCHAR* Parms = *Params;

	// get the specified filename/wildcard
	FString PackageWildcard;
	if (!ParseToken(Parms, PackageWildcard, 0))
	{
		warnf(TEXT("Syntax: ucc utreplaceactor <file/wildcard> <Package.Class to remove> <Package.Class to replace with>"));
		return 1;
	}
	
	// find all the files matching the specified filename/wildcard
	TArray<FString> FilesInPath;
	GFileManager->FindFiles(FilesInPath, *PackageWildcard, 1, 0);
	if (FilesInPath.Num() == 0)
	{
		warnf(NAME_Error, TEXT("No packages found matching %s!"), *PackageWildcard);
		return 2;
	}
	// get the directory part of the filename
	INT ChopPoint = Max(PackageWildcard.InStr(TEXT("/"), 1) + 1, PackageWildcard.InStr(TEXT("\\"), 1) + 1);
	if (ChopPoint < 0)
	{
		ChopPoint = PackageWildcard.InStr( TEXT("*"), 1 );
	}
	FString PathPrefix = (ChopPoint < 0) ? TEXT("") : PackageWildcard.Left(ChopPoint);

	// get the class to remove and the class to replace it with
	FString ClassName;
	if (!ParseToken(Parms, ClassName, 0))
	{
		warnf(TEXT("Syntax: ucc utreplaceactor <file/wildcard> <Package.Class to remove> <Package.Class to replace with>"));
		return 1;
	}
	UClass* ClassToReplace = (UClass*)StaticLoadObject(UClass::StaticClass(), NULL, *ClassName, NULL, LOAD_NoWarn | LOAD_Quiet, NULL);
	if (ClassToReplace == NULL)
	{
		warnf(NAME_Error, TEXT("Invalid class to remove: %s"), *ClassName);
		return 4;
	}
	else
	{
		ClassToReplace->AddToRoot();
	}
	if (!ParseToken(Parms, ClassName, 0))
	{
		warnf(TEXT("Syntax: ucc utreplaceactor <file/wildcard> <Package.Class to remove> <Package.Class to replace with>"));
		return 1;
	}
	UClass* ReplaceWithClass = (UClass*)StaticLoadObject(UClass::StaticClass(), NULL, *ClassName, NULL, LOAD_NoWarn | LOAD_Quiet, NULL);
	if (ReplaceWithClass == NULL)
	{
		warnf(NAME_Error, TEXT("Invalid class to replace with: %s"), *ClassName);
		return 5;
	}
	else
	{
		ReplaceWithClass->AddToRoot();
	}

	// find the most derived superclass common to both classes
	UClass* CommonSuperclass = NULL;
	for (UClass* BaseClass1 = ClassToReplace; BaseClass1 != NULL && CommonSuperclass == NULL; BaseClass1 = BaseClass1->GetSuperClass())
	{
		for (UClass* BaseClass2 = ReplaceWithClass; BaseClass2 != NULL && CommonSuperclass == NULL; BaseClass2 = BaseClass2->GetSuperClass())
		{
			if (BaseClass1 == BaseClass2)
			{
				CommonSuperclass = BaseClass1;
			}
		}
	}
	checkSlow(CommonSuperclass != NULL);

	for (INT i = 0; i < FilesInPath.Num(); i++)
	{
		const FString& PackageName = FilesInPath(i);
		// get the full path name to the file
		FString FileName = PathPrefix + PackageName;
		// skip if read-only
		if (GFileManager->IsReadOnly(*FileName))
		{
			warnf(TEXT("Skipping %s (read-only)"), *FileName);
		}
		else
		{
			// clean up any previous world
			if (GWorld != NULL)
			{
				GWorld->CleanupWorld();
				GWorld->RemoveFromRoot();
				GWorld = NULL;
			}
			// load the package
			warnf(TEXT("Loading %s..."), *PackageName); 
			UPackage* Package = LoadPackage(NULL, *PackageName, LOAD_None);

			// load the world we're interested in
			GWorld = FindObject<UWorld>(Package, TEXT("TheWorld"));
			if (GWorld == NULL)
			{
				warnf(TEXT("Skipping %s (not a map)"));
			}
			else
			{
				// add the world to the root set so that the garbage collection to delete replaced actors doesn't garbage collect the whole world
				GWorld->AddToRoot();
				// initialize the levels in the world
				GWorld->Init();
				GWorld->GetWorldInfo()->PostEditChange(NULL);
				// iterate through all the actors in the world, looking for matches with the class to replace (must have exact match, not subclass)
				for (FActorIterator It; It; ++It)
				{
					AActor* OldActor = *It;
					if (OldActor->GetClass() == ClassToReplace)
					{
						// replace an instance of the old actor
						warnf(TEXT("Replacing actor %s"), *OldActor->GetName());
						// make sure we spawn the new actor in the same level as the old
						//@warning: this relies on the outer of an actor being the level
						GWorld->CurrentLevel = OldActor->GetLevel();
						checkSlow(GWorld->CurrentLevel != NULL);
						// spawn the new actor
						AActor* NewActor = GWorld->SpawnActor(ReplaceWithClass, NAME_None, OldActor->Location, OldActor->Rotation, NULL, TRUE);
						// copy non-native non-transient properties common to both that were modified in the old actor to the new actor
						for (UProperty* Property = CommonSuperclass->PropertyLink; Property != NULL; Property = Property->PropertyLinkNext)
						{
							//@note: skipping properties containing components - don't have a reasonable way to deal with them and the instancing mess they create
                            // to  hack around this for now you can do:
                           // (!(Property->PropertyFlags & CPF_Component) || Property->GetFName() == FName(TEXT("Weapons"))) &&
							if ( !(Property->PropertyFlags & CPF_Native) && !(Property->PropertyFlags & CPF_Transient) &&
								 !(Property->PropertyFlags & CPF_Component) &&
								 !Property->Identical((BYTE*)OldActor + Property->Offset, (BYTE*)OldActor->GetClass()->GetDefaultObject() + Property->Offset) )
							{
								Property->CopyCompleteValue((BYTE*)NewActor + Property->Offset, (BYTE*)OldActor + Property->Offset);
							}
						}
						// destroy the old actor
						//@warning: must do this before replacement so the new Actor doesn't get the old Actor's entry in the level's actor list (which would cause it to be in there twice)
						GWorld->DestroyActor(OldActor);
						check(OldActor->IsValid()); // make sure DestroyActor() doesn't immediately trigger GC since that'll break the reference replacement
						// check for any references to the old Actor and replace them with the new one
						TMap<AActor*, AActor*> ReplaceMap;
						ReplaceMap.Set(OldActor, NewActor);
						FArchiveReplaceObjectRef<AActor> ReplaceAr(GWorld, ReplaceMap, FALSE, FALSE, FALSE);
					}
					else
					{
						// check for any references to the old class and replace them with the new one
						TMap<UClass*, UClass*> ReplaceMap;
						ReplaceMap.Set(ClassToReplace, ReplaceWithClass);
						FArchiveReplaceObjectRef<UClass> ReplaceAr(*It, ReplaceMap, FALSE, FALSE, FALSE);
						if (ReplaceAr.GetCount() > 0)
						{
							warnf(TEXT("Replaced %i class references in actor %s"), ReplaceAr.GetCount(), *It->GetName());
						}
					}
				}
				// replace Kismet references to the class
				USequence* Sequence = GWorld->GetGameSequence();
				if (Sequence != NULL)
				{
					TMap<UClass*, UClass*> ReplaceMap;
					ReplaceMap.Set(ClassToReplace, ReplaceWithClass);
					FArchiveReplaceObjectRef<UClass> ReplaceAr(Sequence, ReplaceMap, FALSE, FALSE, FALSE);
					if (ReplaceAr.GetCount() > 0)
					{
						warnf(TEXT("Replaced %i class references in Kismet"), ReplaceAr.GetCount());
					}
				}

				// collect garbage to delete replaced actors and any objects only referenced by them (components, etc)
				GWorld->PerformGarbageCollection();

				// save the world
				warnf(TEXT("Saving %s..."), *PackageName);
				SavePackage(Package, GWorld, 0, *FileName, GWarn);
				// clear GWorld by removing it from the root set and replacing it with a new one
				GWorld->CleanupWorld();
				GWorld->RemoveFromRoot();
				GWorld = NULL;
			}
		}
		
		// get rid of the loaded world
		warnf(TEXT("Cleaning up..."));
		CollectGarbage(RF_Native);
	}

	// UEditorEngine::FinishDestroy() expects GWorld to exist
	UWorld::CreateNew();

	return 0;
}

AActor* UUTActorFactoryVehicle::CreateActor( const FVector* const Location, const FRotator* const Rotation, const USeqAct_ActorFactory* const ActorFactoryData )
{
	AActor* Actor = Super::CreateActor( Location, Rotation, ActorFactoryData );
	AUTVehicle* Vehicle = Cast<AUTVehicle>(Actor);
	if (Vehicle)
	{
		Vehicle->eventSetTeamNum(TeamNum);
		Vehicle->bTeamLocked = bTeamLocked;
		// actor factories could spawn the vehicle anywhere, so make sure it's awake so it doesn't end up floating or something
		if (Vehicle->Mesh != NULL)
		{
			Vehicle->Mesh->WakeRigidBody();
		}
	}
	return Actor;
}

void AUTCarriedObject::PostNetReceiveBase(AActor* NewBase)
{	
	APawn* P = NewBase ? NewBase->GetAPawn() : NULL;
	if ( !P )
	{
		Super::PostNetReceiveBase(NewBase);
		return;
	}
	if ( NewBase != Base )
	{
		if (P->IsA(AUTPawn::StaticClass()))
		{
			((AUTPawn*)P)->eventHoldGameObject(this);
		}
		else if (P->IsA(AUTVehicleBase::StaticClass()))
		{
			((AUTVehicleBase*)P)->eventHoldGameObject(this);
		}
	}
	bJustTeleported = 0;
}

FString AUTGameStats::GetMapFilename()
{
	return GWorld->URL.Map;
}

void AUTGame::AddSupportedGameTypes(AWorldInfo* Info, const TCHAR* WorldFilename) const
{
	// match the map prefix
	FString BaseFilename = FFilename(WorldFilename).GetBaseFilename();
	for (INT i = 0; i < MapPrefixes.Num(); i++)
	{
		if (BaseFilename.StartsWith(MapPrefixes(i).Prefix))
		{
			UClass* GameClass = StaticLoadClass(AGameInfo::StaticClass(), NULL, *MapPrefixes(i).GameType, NULL, LOAD_None, NULL);
			if (GameClass != NULL)
			{
				Info->GameTypesSupportedOnThisMap.AddUniqueItem(GameClass);
			}
			for (INT j = 0; j < MapPrefixes(i).AdditionalGameTypes.Num(); j++)
			{
				GameClass = StaticLoadClass(AGameInfo::StaticClass(), NULL, *MapPrefixes(i).AdditionalGameTypes(j), NULL, LOAD_None, NULL);
				if (GameClass != NULL)
				{
					Info->GameTypesSupportedOnThisMap.AddUniqueItem(GameClass);
				}
			}
			break;
		}
	}
}

AActor* UUTActorFactoryMover::CreateActor(const FVector* const Location, const FRotator* const Rotation, const USeqAct_ActorFactory* const ActorFactoryData)
{
	AActor* Actor = Super::CreateActor(Location, Rotation, ActorFactoryData);

	if (bCreateKismetEvent && Actor != NULL && Actor->SupportedEvents.FindItemIndex(EventClass) != INDEX_NONE)
	{
		USequence* GameSequence = GWorld->GetGameSequence();
		if (GameSequence != NULL)
		{
			GameSequence->Modify();

			// create a new sequence to put the new event in
			USequence* NewSequence = ConstructObject<USequence>(USequence::StaticClass(), GameSequence, FName(*Actor->GetName()), RF_Transactional);
			NewSequence->ParentSequence = GameSequence;
			GameSequence->SequenceObjects.AddItem(NewSequence);
			NewSequence->ObjName = NewSequence->GetName();
			NewSequence->OnCreated();
			NewSequence->Modify();

			// now create the event
			USequenceEvent* NewEvent = ConstructObject<USequenceEvent>(EventClass, NewSequence, NAME_None, RF_Transactional);
			NewEvent->ObjName = FString::Printf(TEXT("%s %s"), *Actor->GetName(), *NewEvent->ObjName);
			NewEvent->Originator = Actor;
			NewEvent->ParentSequence = NewSequence;
			NewSequence->SequenceObjects.AddItem(NewEvent);
			NewEvent->OnCreated();
			NewEvent->Modify();
		}
	}

	return Actor;
}

void UUTSkelControl_CoreSpin::TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp)
{
	AUTOnslaughtObjective* OwnerObjective = Cast<AUTOnslaughtObjective>(SkelComp->GetOwner());
	FLOAT DPS = 0.0f;
	if (OwnerObjective != NULL)
	{
			const FLOAT HealthPerc = 1.0f - FLOAT(OwnerObjective->Health) / FLOAT(OwnerObjective->DamageCapacity);
			DPS = DegreesPerSecondMin + ((DegreesPerSecondMax - DegreesPerSecondMin) * HealthPerc);
	}
	else
	{
		DPS = DegreesPerSecondMin;
	}
	
	const FLOAT UnrealDegrees = appTrunc(DPS * 182.0444 * DeltaSeconds);

	BoneRotation.Yaw += appTrunc(UnrealDegrees * (bCounterClockwise ? -1 : 1));
	Super::TickSkelControl(DeltaSeconds, SkelComp);
}

AActor* UUTActorFactoryPickup::CreateActor(const FVector* const Location, const FRotator* const Rotation, const USeqAct_ActorFactory* const ActorFactoryData)
{
	ADroppedPickup* Pickup = NULL;
	// spawn the inventory actor
	AInventory* NewInventory = Cast<AInventory>(GWorld->SpawnActor(InventoryClass, NAME_None, *Location, *Rotation));
	if (NewInventory != NULL)
	{
		// if it has its own DroppedPickupClass, use it, otherwise stick with the default
		if (NewInventory->DroppedPickupClass != NULL)
		{
			NewActorClass = NewInventory->DroppedPickupClass;
		}
		Pickup = Cast<ADroppedPickup>(Super::CreateActor(Location, Rotation, ActorFactoryData));
		if (Pickup == NULL)
		{
			GWorld->DestroyActor(NewInventory);
		}
		else
		{
			Pickup->setPhysics(PHYS_Falling);
			Pickup->Inventory = NewInventory;
			Pickup->InventoryClass = InventoryClass;
			Pickup->eventSetPickupMesh(Pickup->Inventory->DroppedPickupMesh);
		}
	}
	return Pickup;
}
		  
/** 
 * @See AActor::physicRotation
 */
void AUTProj_Grenade::physicsRotation(FLOAT deltaTime)
{
	// Accumulate a desired new rotation.
	FRotator NewRotation = Rotation + (RotationRate * deltaTime);

	// Set the new rotation.
	// fixedTurn() returns denormalized results so we must convert Rotation to prevent negative values in Rotation from causing unnecessary MoveActor() calls
	if (NewRotation != Rotation.Denormalize())
	{
		FCheckResult Hit(1.f);
		GWorld->MoveActor( this, FVector(0,0,0), NewRotation, 0, Hit );
	}
}

void AUTKActor::physRigidBody(FLOAT DeltaTime)
{
	Super::physRigidBody(DeltaTime);
	FRigidBodyState OutState;

	if ( bDamageOnEncroachment && bResetDOEWhenAsleep && ( !GetCurrentRBState(OutState) || ( DOEResetThreshold>0 && Velocity.Size() < DOEResetThreshold ) ) )
	{
		bDamageOnEncroachment = FALSE;
	}
}

void AUTTimedPowerup::TickSpecial(FLOAT DeltaTime)
{
	Super::TickSpecial(DeltaTime);

	if (Owner != NULL && TimeRemaining > 0.0f)
	{
		TimeRemaining -= DeltaTime;
		if (TimeRemaining <= 0.0f)
		{
			eventTimeExpired();
		}
	}
}

AActor* UUTActorFactoryAI::CreateActor(const FVector* const Location, const FRotator* const Rotation, const USeqAct_ActorFactory* const ActorFactoryData)
{
	APawn* NewPawn = (APawn*)Super::CreateActor(Location, Rotation, ActorFactoryData);
	if (bForceDeathmatchAI && NewPawn != NULL)
	{
		AUTBot* Bot = Cast<AUTBot>(NewPawn->Controller);
		if (Bot != NULL)
		{
			Bot->Squad = (AUTSquadAI*)GWorld->SpawnActor(AUTSquadAI::StaticClass());
			if (Bot->Squad != NULL)
			{
				if (Bot->PlayerReplicationInfo != NULL)
				{
					Bot->Squad->Team = Cast<AUTTeamInfo>(Bot->PlayerReplicationInfo->Team);
				}
				Bot->Squad->eventSetLeader(Bot);
			}
		}
	}

	// Set the 'use hardware for physics' flag as desired on the spawned Pawn.
	if(bUseCompartment && NewPawn && NewPawn->Mesh)
	{
		NewPawn->Mesh->bUseCompartment = TRUE;
	}

	return NewPawn;
}


void AUTCTFFlag::PostNetReceiveLocation()
{
	Super::PostNetReceiveLocation();
	//debugf(TEXT("POST RECEIVE FLAG"));
	SkelMesh->ResetClothVertsToRefPose();
}

void AForcedDirVolume::PostEditChange(UProperty* Property)
{
	ArrowDirection = Arrow->Rotation.Vector();
}

UBOOL AForcedDirVolume::IgnoreBlockingBy( const AActor *Other ) const
{
	return (!bBlockPawns 
			|| (Other->GetAProjectile() != NULL)
			|| (Other->IsA(TypeToForce))); // ignore everything if we're not blocking pawns; if we are blocking them ignore projectiles and whatever we force in a direction.
}

/** Helper class to sort online player scores used in SortPlayerScores. Must be outside of the function or gcc can't compile it */
class OnlinePlayerScoreSorter
{
public:
	static inline INT Compare(const FOnlinePlayerScore& A,const FOnlinePlayerScore& B)
	{
		// Sort descending...
		return B.Score - A.Score;
	}
};

/**
 * Sorts the scores and assigns relative positions to the players
 *
 * @param PlayerScores the raw scores before sorting and relative position setting
 */
void AUTGame::SortPlayerScores(TArray<FOnlinePlayerScore>& PlayerScores)
{
	Sort<FOnlinePlayerScore,OnlinePlayerScoreSorter>(PlayerScores.GetTypedData(),PlayerScores.Num());
	// Now that the items are sorted, assign relative positions
	for (INT Idx = 0; Idx < PlayerScores.Num(); Idx++)
	{
		// Total players minus position equals their relative position, ie 1st is highest
		PlayerScores(Idx).Score = PlayerScores.Num() - Idx;
	}
}


UBOOL AUTDeployedActor::IgnoreBlockingBy( const AActor *Other ) const
{
	return ( Other == Instigator && Physics == PHYS_Falling );
}


void UUTMapInfo::CheckForErrors()
{
	Super::CheckForErrors();

	if( MapMusicInfo == NULL )
	{
		GWarn->MapCheck_Add(MCTYPE_WARNING, NULL, TEXT("Map does not have a MapMusicInfo set."));
	}
	for (FActorIterator It; It; ++It)
	{
		if (It->GetClass() == ATeleporter::StaticClass())
		{
			GWarn->MapCheck_Add(MCTYPE_WARNING, *It, TEXT("Maps should use UTTeleporters instead of Teleporters"), MCACTION_DELETE);
		}
	}
}


/**
 * Returns the index of a given map in the array or INDEX_None if it doesn't exist
 *
 * @Param MapId		The ID to look up.
 */
INT AUTVoteReplicationInfo::GetMapIndex(INT MapID)
{
	for (INT i=0;i<Maps.Num(); i++)
	{
		if (Maps(i).MapID == MapID)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

/**
 * Look for the setting of the owner 
 */
void AUTVoteReplicationInfo::TickSpecial(FLOAT DeltaTime)
{
	Super::TickSpecial(DeltaTime);
	if (OldOwner != Owner && Owner && GWorld->GetWorldInfo()->NetMode == NM_Client)
	{
		eventClientHasOwner();
	}
	OldOwner = Owner;
}


/**
 * Returns the index of a given map in the array or INDEX_None if it doesn't exist
 *
 */
INT AUTVoteCollector::GetMapIndex(INT MapID)
{
	for (INT i=0;i<Votes.Num(); i++)
	{
		if (Votes(i).MapID == MapID)
		{
			return i;
		}
	}
	return INDEX_NONE;
}


/**
 * Game-specific code to handle DLC being added or removed
 * 
 * @param bContentWasInstalled TRUE if DLC was installed, FALSE if it was removed
 */
void appOnDownloadableContentChanged(UBOOL bContentWasInstalled)
{
	// Refresh the custom character data in case character data was added/removed
	UUTCustomChar_Data::StaticClass()->GetDefaultObject()->ReloadConfig(NULL, NULL, UE3::LCPF_PropagateToInstances);

	// refresh the data provider
	UGameUISceneClient* Client = UUIRoot::GetSceneClient();
	if (Client)
	{
		UUIDataStore_GameResource* DataStore = (UUIDataStore_GameResource*)Client->DataStoreManager->FindDataStore(FName(TEXT("UTGameResources")));
		if (DataStore)
		{
			// reparse the .ini files for the PerObjectConfig objects
			DataStore->InitializeListElementProviders();
			// make ay currently active widgets using the data get refreshed
			DataStore->eventRefreshSubscribers();
		}
	}
}

/** Function that disables streaming of world textures for NumFrames. */
void AUTGameReplicationInfo::SetNoStreamWorldTextureForFrames(INT NumFrames)
{
	if(GStreamingManager)
	{
		GStreamingManager->SetDisregardWorldResourcesForFrames(NumFrames);
	}
}

void AUTEmitterPool::TickSpecial(FLOAT DeltaTime)
{
	Super::TickSpecial(DeltaTime);

	INT i = 0;
	while (i < RelativeExplosionLights.Num())
	{
		if (RelativeExplosionLights(i).Light == NULL || RelativeExplosionLights(i).Base == NULL || RelativeExplosionLights(i).Base->bDeleteMe)
		{
			RelativeExplosionLights.Remove(i, 1);
		}
		else
		{
			FVector NewTranslation = RelativeExplosionLights(i).Base->Location + RelativeExplosionLights(i).RelativeLocation;
			if (RelativeExplosionLights(i).Light->Translation != NewTranslation)
			{
				RelativeExplosionLights(i).Light->Translation = NewTranslation;
				RelativeExplosionLights(i).Light->BeginDeferredUpdateTransform();
			}
			i++;
		}
	}
}

