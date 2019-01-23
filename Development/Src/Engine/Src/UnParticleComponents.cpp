/*=============================================================================
	UnParticleComponent.cpp: Particle component implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineParticleClasses.h"
#include "EngineMaterialClasses.h"
#include "LevelUtils.h"
#include "UnNovodexSupport.h"
#include "ImageUtils.h"

IMPLEMENT_CLASS(AEmitter);

IMPLEMENT_CLASS(UParticleSystem);
IMPLEMENT_CLASS(UParticleEmitter);
IMPLEMENT_CLASS(UParticleSpriteEmitter);
IMPLEMENT_CLASS(UParticleSystemComponent);
IMPLEMENT_CLASS(UParticleMeshEmitter);
IMPLEMENT_CLASS(UParticleSpriteSubUVEmitter);
IMPLEMENT_CLASS(UParticleLODLevel);

/** Whether to allow particle systems to perform work. */
UBOOL GIsAllowingParticles = TRUE;

// Comment this in to debug empty emitter instance templates...
//#define _PSYSCOMP_DEBUG_INVALID_EMITTER_INSTANCE_TEMPLATES_

/*-----------------------------------------------------------------------------
	FParticlesStatGroup
-----------------------------------------------------------------------------*/
DECLARE_STATS_GROUP(TEXT("Particles"),STATGROUP_Particles);
DECLARE_DWORD_COUNTER_STAT(TEXT("Sprite Particles"),STAT_SpriteParticles,STATGROUP_Particles);
DECLARE_DWORD_COUNTER_STAT(TEXT("Sprite Ptcl Render Calls"),STAT_SpriteParticlesRenderCalls,STATGROUP_Particles);
DECLARE_DWORD_COUNTER_STAT(TEXT("Sprite Ptcl Render Calls Completed"),STAT_SpriteParticlesRenderCallsCompleted,STATGROUP_Particles);
DECLARE_DWORD_COUNTER_STAT(TEXT("Sprite Ptcls Spawned"),STAT_SpriteParticlesSpawned,STATGROUP_Particles);
DECLARE_DWORD_COUNTER_STAT(TEXT("Sprite Ptcls Updated"),STAT_SpriteParticlesUpdated,STATGROUP_Particles);
DECLARE_DWORD_COUNTER_STAT(TEXT("Sprite Ptcls Killed"),STAT_SpriteParticlesKilled,STATGROUP_Particles);
DECLARE_DWORD_COUNTER_STAT(TEXT("Mesh Particles"),STAT_MeshParticles,STATGROUP_Particles);
DECLARE_CYCLE_STAT(TEXT("Sort Time"),STAT_SortingTime,STATGROUP_Particles);
DECLARE_CYCLE_STAT(TEXT("Sprite Render Time"),STAT_SpriteRenderingTime,STATGROUP_Particles);
DECLARE_CYCLE_STAT(TEXT("Sprite Res Update Time"),STAT_SpriteResourceUpdateTime,STATGROUP_Particles);
DECLARE_CYCLE_STAT(TEXT("Sprite Tick Time"),STAT_SpriteTickTime,STATGROUP_Particles);
DECLARE_CYCLE_STAT(TEXT("Sprite Spawn Time"),STAT_SpriteSpawnTime,STATGROUP_Particles);
DECLARE_CYCLE_STAT(TEXT("Sprite Update Time"),STAT_SpriteUpdateTime,STATGROUP_Particles);
DECLARE_CYCLE_STAT(TEXT("PSys Comp Tick Time"),STAT_PSysCompTickTime,STATGROUP_Particles);

/*-----------------------------------------------------------------------------
	Particle scene view
-----------------------------------------------------------------------------*/
FSceneView*			GParticleView = NULL;

/*-----------------------------------------------------------------------------
	Particle data manager
-----------------------------------------------------------------------------*/
FParticleDataManager	GParticleDataManager;


/*-----------------------------------------------------------------------------
	Detailed particle tick stats.
-----------------------------------------------------------------------------*/

#define TRACK_DETAILED_PARTICLE_TICK_STATS (!FINAL_RELEASE)

#if TRACK_DETAILED_PARTICLE_TICK_STATS

/**
 * Tick time stats for a particle system.
 */
struct FParticleTickStats
{
	/** Total accumulative tick time in seconds. */
	FLOAT	AccumTickTime;
	/** Max tick time in seconds. */
	FLOAT	MaxTickTime;
	/** Total accumulative active particles. */
	INT		AccumActiveParticles;
	/** Max active particle count. */
	INT		MaxActiveParticleCount;
	/** Total tick count. */
	INT		TickCount;

	/** Constructor, initializing all member variables. */
	FParticleTickStats()
	:	AccumTickTime(0)
	,	MaxTickTime(0)
	,	AccumActiveParticles(0)
	,	MaxActiveParticleCount(0)
	,	TickCount(0)
	{
	}
};

/**
 * Per particle system (template) tick time stats tracking.
 */
class FParticleTickStatManager : FSelfRegisteringExec
{
public:
	/** Constructor, initializing all member variables. */
	FParticleTickStatManager()
	:	bIsEnabled(FALSE)
	{}

	/**
	 * Exec handler routed via UObject::StaticExec
	 *
	 * @param	Cmd		Command to parse
	 * @param	Ar		Output device to log to
	 *
	 * @return	TRUE if command was handled, FALSE otherwise
	 */
	virtual UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar )
	{
		const TCHAR *Str = Cmd;
		if( ParseCommand(&Str,TEXT("PARTICLETICKSTATS")) )
		{
			// Empty out stats.
			if( ParseCommand(&Str,TEXT("RESET")) )
			{
				ParticleSystemToTickStatsMap.Empty();
				return TRUE;
			}
			// Start tracking.
			else if( ParseCommand(&Str,TEXT("START")) )
			{
				bIsEnabled = TRUE;
				return TRUE;
			}
			// Stop tracking
			else if( ParseCommand(&Str,TEXT("STOP")) )
			{
				bIsEnabled = FALSE;
				return TRUE;
			}
			// Dump stats in CSV file format to log.
			else if( ParseCommand(&Str,TEXT("DUMP")) )
			{
				// Header for CSV format.
				Ar.Logf(TEXT(",avg ms/ tick, max ms/ tick, ticks, avg active/ tick, max active/ tick, particle system"));

				// Iterate over all gathered stats and dump them (unsorted) in CSV file format, pasteable into Excel.
				for( TMap<FString,FParticleTickStats>::TConstIterator It(ParticleSystemToTickStatsMap); It; ++It )
				{
					const FString& ParticleSystemName = It.Key();
					const FParticleTickStats& TickStats = It.Value();
					Ar.Logf(TEXT(", %5.2f, %5.2f, %i, %i, %i, %s"), 
						TickStats.AccumTickTime / TickStats.TickCount * 1000.f,
						TickStats.MaxTickTime * 1000.f,
						TickStats.TickCount,
						TickStats.AccumActiveParticles / TickStats.TickCount,
						TickStats.MaxActiveParticleCount,
						*ParticleSystemName );
				}
				return TRUE;
			}
			return FALSE;
		}
		return FALSE;
	}

	/**
	 * Updates stats with passed in information.
	 *
	 * @param	Component			Particle system component to update associated template's stat for
	 * @param	TickTime			Time it took to tick this component
	 * @param	ActiveParticles		Number of currently active particles
	 */
	void UpdateStats( UParticleSystem* Template, FLOAT TickTime, INT ActiveParticles )
	{
		check(Template);

		// Find existing entry and update it if found.
		FParticleTickStats* ParticleTickStats = ParticleSystemToTickStatsMap.Find( Template->GetPathName() );
		if( ParticleTickStats )
		{
			ParticleTickStats->AccumTickTime += TickTime;
			ParticleTickStats->MaxTickTime = Max( ParticleTickStats->MaxTickTime, TickTime );
			ParticleTickStats->AccumActiveParticles += ActiveParticles;
			ParticleTickStats->MaxActiveParticleCount = Max( ParticleTickStats->MaxActiveParticleCount, ActiveParticles );
			ParticleTickStats->TickCount++;
		}
		// Create new entry.
		else
		{
			// Create new entry...
			FParticleTickStats NewParticleTickStats;
			NewParticleTickStats.AccumTickTime = TickTime;
			NewParticleTickStats.MaxTickTime = TickTime;
			NewParticleTickStats.AccumActiveParticles = ActiveParticles;
			NewParticleTickStats.MaxActiveParticleCount = ActiveParticles;
			NewParticleTickStats.TickCount = 1;
			// ... and set it for subsequent retrieval.
			ParticleSystemToTickStatsMap.Set( *Template->GetPathName(), NewParticleTickStats );
		}
	}

	/** Mapping from particle system to tick stats. */
	TMap<FString,FParticleTickStats> ParticleSystemToTickStatsMap;

	/** Whether tracking is currently enabled. */
	UBOOL bIsEnabled;
};

/** Global tick stat manager object. */
FParticleTickStatManager* GParticleTickStatManager;

#endif

/*-----------------------------------------------------------------------------
	Conversion functions
-----------------------------------------------------------------------------*/
void Particle_ModifyFloatDistribution(UDistributionFloat* pkDistribution, FLOAT fScale)
{
	if (pkDistribution->IsA(UDistributionFloatConstant::StaticClass()))
	{
		UDistributionFloatConstant* pkDistConstant = Cast<UDistributionFloatConstant>(pkDistribution);
		pkDistConstant->Constant *= fScale;
	}
	else
	if (pkDistribution->IsA(UDistributionFloatUniform::StaticClass()))
	{
		UDistributionFloatUniform* pkDistUniform = Cast<UDistributionFloatUniform>(pkDistribution);
		pkDistUniform->Min *= fScale;
		pkDistUniform->Max *= fScale;
	}
	else
	if (pkDistribution->IsA(UDistributionFloatConstantCurve::StaticClass()))
	{
		UDistributionFloatConstantCurve* pkDistCurve = Cast<UDistributionFloatConstantCurve>(pkDistribution);

		INT iKeys = pkDistCurve->GetNumKeys();
		INT iCurves = pkDistCurve->GetNumSubCurves();

		for (INT KeyIndex = 0; KeyIndex < iKeys; KeyIndex++)
		{
			FLOAT fKeyIn = pkDistCurve->GetKeyIn(KeyIndex);
			for (INT SubIndex = 0; SubIndex < iCurves; SubIndex++)
			{
				FLOAT fKeyOut = pkDistCurve->GetKeyOut(SubIndex, KeyIndex);
				FLOAT ArriveTangent;
				FLOAT LeaveTangent;
                pkDistCurve->GetTangents(SubIndex, KeyIndex, ArriveTangent, LeaveTangent);

				pkDistCurve->SetKeyOut(SubIndex, KeyIndex, fKeyOut * fScale);
				pkDistCurve->SetTangents(SubIndex, KeyIndex, ArriveTangent * fScale, LeaveTangent * fScale);
			}
		}
	}
}

void Particle_ModifyVectorDistribution(UDistributionVector* pkDistribution, FVector& vScale)
{
	if (pkDistribution->IsA(UDistributionVectorConstant::StaticClass()))
	{
		UDistributionVectorConstant* pkDistConstant = Cast<UDistributionVectorConstant>(pkDistribution);
		pkDistConstant->Constant *= vScale;
	}
	else
	if (pkDistribution->IsA(UDistributionVectorUniform::StaticClass()))
	{
		UDistributionVectorUniform* pkDistUniform = Cast<UDistributionVectorUniform>(pkDistribution);
		pkDistUniform->Min *= vScale;
		pkDistUniform->Max *= vScale;
	}
	else
	if (pkDistribution->IsA(UDistributionVectorConstantCurve::StaticClass()))
	{
		UDistributionVectorConstantCurve* pkDistCurve = Cast<UDistributionVectorConstantCurve>(pkDistribution);

		INT iKeys = pkDistCurve->GetNumKeys();
		INT iCurves = pkDistCurve->GetNumSubCurves();

		for (INT KeyIndex = 0; KeyIndex < iKeys; KeyIndex++)
		{
			FLOAT fKeyIn = pkDistCurve->GetKeyIn(KeyIndex);
			for (INT SubIndex = 0; SubIndex < iCurves; SubIndex++)
			{
				FLOAT fKeyOut = pkDistCurve->GetKeyOut(SubIndex, KeyIndex);
				FLOAT ArriveTangent;
				FLOAT LeaveTangent;
                pkDistCurve->GetTangents(SubIndex, KeyIndex, ArriveTangent, LeaveTangent);

				switch (SubIndex)
				{
				case 1:
					pkDistCurve->SetKeyOut(SubIndex, KeyIndex, fKeyOut * vScale.Y);
					pkDistCurve->SetTangents(SubIndex, KeyIndex, ArriveTangent * vScale.Y, LeaveTangent * vScale.Y);
					break;
				case 2:
					pkDistCurve->SetKeyOut(SubIndex, KeyIndex, fKeyOut * vScale.Z);
					pkDistCurve->SetTangents(SubIndex, KeyIndex, ArriveTangent * vScale.Z, LeaveTangent * vScale.Z);
					break;
				case 0:
				default:
					pkDistCurve->SetKeyOut(SubIndex, KeyIndex, fKeyOut * vScale.X);
					pkDistCurve->SetTangents(SubIndex, KeyIndex, ArriveTangent * vScale.X, LeaveTangent * vScale.X);
					break;
				}
			}
		}
	}
}

void Particle_ConvertEmitterModules(UParticleEmitter* Emitter)
{
	if (GIsGame == TRUE)
	{
		return;
	}

	if (Emitter->ConvertedModules)
		return;

	if (Emitter->GetLinker() && (Emitter->GetLinker()->Ver() < 183))
	{
		// Convert all rotations to "rotations per second" not "degrees per second"
		UParticleModule* Module;
		for (INT i = 0; i < Emitter->Modules.Num(); i++)
		{
			Module = Emitter->Modules(i);

			if (!Module->IsA(UParticleModuleRotationBase::StaticClass()) &&
				!Module->IsA(UParticleModuleRotationRateBase::StaticClass()))
			{
				continue;
			}

			// Depending on the type, we will need to grab different distributions...
			FVector Scale = FVector(1.0f/360.0f, 1.0f/360.0f, 1.0f/360.0f);

			if (Module->IsA(UParticleModuleRotation::StaticClass()))
			{
				UParticleModuleRotation* PMRotation = CastChecked<UParticleModuleRotation>(Module);
				Particle_ModifyFloatDistribution(PMRotation->StartRotation.Distribution, 1.0f/360.0f);
			}
			else
			if (Module->IsA(UParticleModuleMeshRotation::StaticClass()))
			{
				UParticleModuleMeshRotation* PMMRotation = CastChecked<UParticleModuleMeshRotation>(Module);
				Particle_ModifyVectorDistribution(PMMRotation->StartRotation.Distribution, Scale);
			}
			else
			if (Module->IsA(UParticleModuleRotationRate::StaticClass()))
			{
				UParticleModuleRotationRate* PMRotationRate = CastChecked<UParticleModuleRotationRate>(Module);
				Particle_ModifyFloatDistribution(PMRotationRate->StartRotationRate.Distribution, 1.0f/360.0f);
			}
			else
			if (Module->IsA(UParticleModuleMeshRotationRate::StaticClass()))
			{
				UParticleModuleMeshRotationRate* PMMRotationRate = CastChecked<UParticleModuleMeshRotationRate>(Module);
				Particle_ModifyVectorDistribution(PMMRotationRate->StartRotationRate.Distribution, Scale);
			}
			else
			if (Module->IsA(UParticleModuleRotationOverLifetime::StaticClass()))
			{
				UParticleModuleRotationOverLifetime* PMRotOverLife = CastChecked<UParticleModuleRotationOverLifetime>(Module);
				Particle_ModifyFloatDistribution(PMRotOverLife->RotationOverLife.Distribution, 1.0f/360.0f);
			}
		}
	}

	if (Emitter->GetLinker() && (Emitter->GetLinker()->Ver() < 190))
	{
		// Convert ParticleModuleTypeDataSubUV modules
		if (Emitter->TypeDataModule &&
			(Emitter->TypeDataModule->IsA(UParticleModuleTypeDataSubUV::StaticClass())))
		{
			// Copy the contents to the base emitter class
			UParticleModuleTypeDataSubUV* TDSubUV = CastChecked<UParticleModuleTypeDataSubUV>(Emitter->TypeDataModule);
			switch (TDSubUV->InterpolationMethod)
			{
			case PSSUVIM_Linear:
				Emitter->InterpolationMethod	= PSUVIM_Linear;
				break;
			case PSSUVIM_Linear_Blend:
				Emitter->InterpolationMethod	= PSUVIM_Linear_Blend;
				break;
			case PSSUVIM_Random:
				Emitter->InterpolationMethod	= PSUVIM_Random;
				break;
			case PSSUVIM_Random_Blend:
				Emitter->InterpolationMethod	= PSUVIM_Random_Blend;
				break;
			}
			Emitter->SubImages_Horizontal	= TDSubUV->SubImages_Horizontal;
			Emitter->SubImages_Vertical		= TDSubUV->SubImages_Vertical;
			Emitter->DirectUV				= TDSubUV->DirectUV;

			Emitter->TypeDataModule = 0;
			
			Emitter->MarkPackageDirty();
		}
	}
}


/*-----------------------------------------------------------------------------
	AEmitter implementation.
-----------------------------------------------------------------------------*/

void AEmitter::SetTemplate(UParticleSystem* NewTemplate, UBOOL bDestroyOnFinish)
{
	if (ParticleSystemComponent)
	{
		FComponentReattachContext ReattachContext(ParticleSystemComponent);
		ParticleSystemComponent->SetTemplate(NewTemplate);
	}
	bDestroyOnSystemFinish = bDestroyOnFinish;
}

void AEmitter::AutoPopulateInstanceProperties()
{
	if (ParticleSystemComponent)
	{
		ParticleSystemComponent->AutoPopulateInstanceProperties();
	}
}

void AEmitter::CheckForErrors()
{
	Super::CheckForErrors();

	// Emitters placed in a level should have a non-NULL ParticleSystemComponent.
	UObject* Outer = GetOuter();
	if( Cast<ULevel>( Outer ) )
	{
		if ( ParticleSystemComponent == NULL )
		{
			GWarn->MapCheck_Add( MCTYPE_WARNING, this, *FString::Printf(TEXT("%s : Emitter actor has NULL ParticleSystemComponent property - please delete!"), *GetName() ), MCACTION_DELETE, TEXT("ParticleSystemComponentNull") );
		}
	}
}

void AEmitter::execSetTemplate(FFrame& Stack, RESULT_DECL)
{
	P_GET_OBJECT(UParticleSystem,NewTemplate);
	P_GET_UBOOL_OPTX(bDestroyOnFinish, FALSE);
	P_FINISH;

	SetTemplate(NewTemplate, bDestroyOnFinish);
}

//----------------------------------------------------------------------------

/**
 * Try to find a level color for the specified particle system component.
 */
static FColor* GetLevelColor(UParticleSystemComponent* Component)
{
	FColor* LevelColor = NULL;

	AActor* Owner = Component->GetOwner();
	if ( Owner )
	{
		ULevel* Level = Owner->GetLevel();
		ULevelStreaming* LevelStreaming = FLevelUtils::FindStreamingLevel( Level );
		if ( LevelStreaming )
		{
			LevelColor = &LevelStreaming->DrawColor;
		}
	}

	return LevelColor;
}

/*-----------------------------------------------------------------------------
	UParticleLODLevel implementation.
-----------------------------------------------------------------------------*/
void UParticleLODLevel::PostEditChange(UProperty* PropertyThatChanged)
{
	Super::PostEditChange( PropertyThatChanged );
}

void UParticleLODLevel::PostLoad()
{
	Super::PostLoad();
}

void UParticleLODLevel::UpdateModuleLists()
{
	SpawningModules.Empty();
	SpawnModules.Empty();
	UpdateModules.Empty();
	OrbitModules.Empty();

	UParticleModule* Module;
	INT TypeDataModuleIndex = -1;

	for (INT i = 0; i < Modules.Num(); i++)
	{
		Module = Modules(i);
		if (!Module)
		{
			continue;
		}

		if (Module->bSpawnModule)
		{
			SpawnModules.AddItem(Module);
		}
		if (Module->bUpdateModule)
		{
			UpdateModules.AddItem(Module);
		}

		if (Module->IsA(UParticleModuleTypeDataBase::StaticClass()))
		{
			TypeDataModule = Module;
			if (!Module->bSpawnModule && !Module->bUpdateModule)
			{
				// For now, remove it from the list and set it as the TypeDataModule
				TypeDataModuleIndex = i;
			}
		}
		else
		if (Module->IsA(UParticleModuleSpawnBase::StaticClass()))
		{
			UParticleModuleSpawnBase* SpawnBase = Cast<UParticleModuleSpawnBase>(Module);
			SpawningModules.AddItem(SpawnBase);
		}
		else
		if (Module->IsA(UParticleModuleOrbit::StaticClass()))
		{
			UParticleModuleOrbit* Orbit = Cast<UParticleModuleOrbit>(Module);
			OrbitModules.AddItem(Orbit);
		}
	}

	if (TypeDataModuleIndex != -1)
	{
		Modules.Remove(TypeDataModuleIndex);
	}

	if (TypeDataModule && (LevelSetting == 0))
	{
		UParticleModuleTypeDataMesh* MeshTD = Cast<UParticleModuleTypeDataMesh>(TypeDataModule);
		if (MeshTD)
		{
			if (MeshTD->Mesh)
			{
				if (MeshTD->Mesh->LODModels(0).Elements.Num())
				{
					UParticleSpriteEmitter* SpriteEmitter = Cast<UParticleSpriteEmitter>(GetOuter());
					if (SpriteEmitter && (MeshTD->bOverrideMaterial == FALSE))
					{
						FStaticMeshElement&	Element = MeshTD->Mesh->LODModels(0).Elements(0);
						if (Element.Material)
						{
							RequiredModule->Material = Element.Material;
						}
					}
				}
			}
		}
	}
}

/**
 */
UBOOL UParticleLODLevel::GenerateFromLODLevel(UParticleLODLevel* SourceLODLevel, FLOAT Percentage, UBOOL bGenerateModuleData)
{
	// See if there are already modules in place
	if (Modules.Num() > 0)
	{
		debugf(TEXT("ERROR? - GenerateFromLODLevel - modules already present!"));
		return FALSE;
	}

	UBOOL	bResult	= TRUE;

	Modules.InsertZeroed(0, SourceLODLevel->Modules.Num());

	UObject* DupObject = UObject::StaticDuplicateObject(
		SourceLODLevel->RequiredModule, SourceLODLevel->RequiredModule, 
		SourceLODLevel->RequiredModule->GetOuter(), TEXT("None"));
	if (DupObject)
	{
		RequiredModule				= Cast<UParticleModuleRequired>(DupObject);
		RequiredModule->bEditable	= FALSE;

		if (bGenerateModuleData)
		{
			if (RequiredModule->GenerateLODModuleValues(SourceLODLevel->RequiredModule, Percentage, this) == FALSE)
			{
				debugf(TEXT("ERROR - GenerateFromLODLevel - Failed to generate LOD module values for RequiredModule!"));
				bResult	= FALSE;
			}
		}
	}

	// Type data...
	if (SourceLODLevel->TypeDataModule)
	{
		UParticleModule* CopyModule	= SourceLODLevel->TypeDataModule;
		DupObject = UObject::StaticDuplicateObject(CopyModule, CopyModule, CopyModule->GetOuter(), TEXT("None"));
		if (DupObject)
		{
			UParticleModule* NewModule	= Cast<UParticleModule>(DupObject);
			NewModule->bEditable		= FALSE;
			if (bGenerateModuleData)
			{
				if (NewModule->GenerateLODModuleValues(CopyModule, Percentage, this) == FALSE)
				{
					debugf(TEXT("ERROR - GenerateFromLODLevel - Failed to generate LOD module values for %s!"),
						*NewModule->GetName());
					bResult	= FALSE;
				}
			}
			TypeDataModule = NewModule;
		}
	}

	// Modules
	for (INT ModuleIndex = 0; ModuleIndex < SourceLODLevel->Modules.Num(); ModuleIndex++)
	{
		UParticleModule* CopyModule	= SourceLODLevel->Modules(ModuleIndex);
		if (CopyModule)
		{
			DupObject = UObject::StaticDuplicateObject(CopyModule, CopyModule, CopyModule->GetOuter(), TEXT("None"));
			if (DupObject)
			{
				UParticleModule* NewModule	= Cast<UParticleModule>(DupObject);
				NewModule->bEditable		= FALSE;
				if (bGenerateModuleData)
				{
					if (NewModule->GenerateLODModuleValues(CopyModule, Percentage, this) == FALSE)
					{
						debugf(TEXT("ERROR - GenerateFromLODLevel - Failed to generate LOD module values for %s!"),
							*NewModule->GetName());
						bResult	= FALSE;
					}
				}
				Modules(ModuleIndex) = NewModule;
			}
		}
		else
		{
			warnf(TEXT("Emitter has an empty module slot!!!"));
			Modules(ModuleIndex) = NULL;
		}
	}

	return bResult;
}

/**
 *	CalculateMaxActiveParticleCount
 *	Determine the maximum active particles that could occur with this emitter.
 *	This is to avoid reallocation during the life of the emitter.
 *
 *	@return		The maximum active particle count for the LOD level.
 */
INT	UParticleLODLevel::CalculateMaxActiveParticleCount()
{
	check(RequiredModule != NULL);

	// Determine the lifetime for particles coming from the emitter
	FLOAT ParticleLifetime = 0.0f;
	UBOOL bHadZerorParticleLifetime = FALSE;

	TArray<UParticleModuleLifetimeBase*> LifetimeModules;
	LifetimeModules.Empty();
	for (INT ModuleIndex = 0; ModuleIndex < Modules.Num(); ModuleIndex++)
	{
		UParticleModule* Module = Modules(ModuleIndex);
		if (Module)
		{
			UParticleModuleLifetime* LifetimeMod = Cast<UParticleModuleLifetime>(Module);
			if (LifetimeMod)
			{
				LifetimeModules.AddItem(LifetimeMod);
			}
		}
	}

	if (LifetimeModules.Num() > 0)
	{
		for (INT LTMIndex = 0; LTMIndex < LifetimeModules.Num(); LTMIndex++)
		{
			UParticleModuleLifetimeBase* LifetimeBase = LifetimeModules(LTMIndex);
			FLOAT PartLife = LifetimeBase->GetMaxLifetime();

			if (PartLife == 0.0f)
			{
				bHadZerorParticleLifetime = TRUE;
			}
			ParticleLifetime += PartLife;
		}
	}
	else
	{
		warnf(TEXT("LODLevel has no Lifetime module - PSys %s"), *GetOuter()->GetName());
	}

	// Check for an UberRaiDrops module
	for (INT ModuleIndex = 0; ModuleIndex < Modules.Num(); ModuleIndex++)
	{
		UParticleModule* Module = Modules(ModuleIndex);
		if (Module)
		{
			UParticleModuleUberRainDrops* URDMod = Cast<UParticleModuleUberRainDrops>(Module);
			if (URDMod)
			{
				ParticleLifetime += URDMod->LifetimeMax;
			}
		}
	}

	// Calculate the Spawn distribution contribution
	FLOAT MaxSpawnRate, MinSpawnRate;
	RequiredModule->SpawnRate.GetOutRange(MinSpawnRate, MaxSpawnRate);

	// Calculate the BurstList contribution
	INT MaxBurst = 0;
	for (INT BurstIndex = 0; BurstIndex < RequiredModule->BurstList.Num(); BurstIndex++)
	{
		FParticleBurst& BurstEntry = RequiredModule->BurstList(BurstIndex);
		//@todo. Take time into account??
		MaxBurst += BurstEntry.Count;
	}

	// Determine the max
	INT MaxAPC = 0;

	MaxAPC += appFloor(MaxSpawnRate * ParticleLifetime) + 2;
	MaxAPC += MaxBurst;

	PeakActiveParticles = MaxAPC;

	return MaxAPC;
}

/**
 */
UBOOL UParticleLODLevel::GenerateFromBoundingLODLevels(UParticleLODLevel* HighLODLevel, UParticleLODLevel* LowLODLevel, UBOOL bGenerateModuleData)
{
	// See if there are already modules in place
	if (Modules.Num() > 0)
	{
		debugf(TEXT("ERROR? - GenerateFromBoundingLODLevels - modules already present!"));
		return FALSE;
	}

	UBOOL	bResult		= TRUE;
	FLOAT	Percentage	= (LevelSetting / (FLOAT)(100 - HighLODLevel->LevelSetting)) * 100.0f;

	//@todo. Evaluate between the high and low, and set the values appropriately.
	FLOAT	Multiplier	= 1.0f;
	INT		Diff		= LowLODLevel->LevelSetting - HighLODLevel->LevelSetting;
	// The Diff == 0 case is when there is an exact match to the current setting.
	if (Diff > 0)
	{
		Multiplier	= ((FLOAT)(LowLODLevel->LevelSetting - LevelSetting) / (FLOAT)(Diff));
	}

	Modules.InsertZeroed(0, HighLODLevel->Modules.Num());

	UObject* DupObject = UObject::StaticDuplicateObject(
		HighLODLevel->RequiredModule, HighLODLevel->RequiredModule, 
		HighLODLevel->RequiredModule->GetOuter(), TEXT("None"));
	if (DupObject)
	{
		RequiredModule				= Cast<UParticleModuleRequired>(DupObject);
		RequiredModule->bEditable	= FALSE;

		if (RequiredModule->GenerateLODModuleValues(HighLODLevel->RequiredModule, Percentage, this) == FALSE)
		{
			debugf(TEXT("ERROR - GenerateFromLODLevel - Failed to generate LOD module values for RequiredModule!"));
			bResult	= FALSE;
		}
	}

	for (INT ModuleIndex = 0; ModuleIndex < HighLODLevel->Modules.Num(); ModuleIndex++)
	{
		UParticleModule* CopyModule	= HighLODLevel->Modules(ModuleIndex);
		DupObject = UObject::StaticDuplicateObject(CopyModule, CopyModule, CopyModule->GetOuter(), TEXT("None"));
		if (DupObject)
		{
			UParticleModule* NewModule	= Cast<UParticleModule>(DupObject);
			NewModule->bEditable		= FALSE;
			if (NewModule->GenerateLODModuleValues(CopyModule, Percentage, this) == FALSE)
			{
				debugf(TEXT("ERROR - GenerateFromLODLevel - Failed to generate LOD module values for %s!"),
					*NewModule->GetName());
				bResult	= FALSE;
			}
			Modules(ModuleIndex) = NewModule;
		}
	}

	return bResult;
}

/*-----------------------------------------------------------------------------
	UParticleEmitter implementation.
-----------------------------------------------------------------------------*/
FParticleEmitterInstance* UParticleEmitter::CreateInstance(UParticleSystemComponent* InComponent)
{
	appErrorf(TEXT("UParticleEmitter::CreateInstance is pure virtual")); 
	return NULL; 
}

void UParticleEmitter::UpdateModuleLists()
{
	for (INT LODIndex = 0; LODIndex < LODLevels.Num(); LODIndex++)
	{
		UParticleLODLevel* LODLevel = LODLevels(LODIndex);
		if (LODLevel)
		{
			LODLevel->UpdateModuleLists();
		}
	}
}

void UParticleEmitter::PostLoad()
{
	Super::PostLoad();

	if (GIsUCCMake == TRUE)
	{
		return;
	}

	if ((GIsEditor == TRUE) && 1)//(GIsUCC == FALSE))
	{
		ConvertedModules = FALSE;
		PeakActiveParticles = 0;

		// Check for improper outers...
		UObject* EmitterOuter = GetOuter();
		UBOOL bWarned = FALSE;
		for (INT LODIndex = 0; (LODIndex < LODLevels.Num()) && !bWarned; LODIndex++)
		{
			UParticleLODLevel* LODLevel = LODLevels(LODIndex);
			if (LODLevel)
			{
				LODLevel->ConditionalPostLoad();

				UParticleModule* Module = LODLevel->TypeDataModule;
				if (Module)
				{
					Module->ConditionalPostLoad();

					UObject* OuterObj = Module->GetOuter();
					check(OuterObj);
					if (OuterObj != EmitterOuter)
					{
						warnf(TEXT("UParticleModule %s has an incorrect outer on %s... run FixupEmitters on package %s (%s)"),
							*(Module->GetPathName()), 
							*(EmitterOuter->GetPathName()),
							*(OuterObj->GetOutermost()->GetPathName()),
							*(GetOutermost()->GetPathName()));
						warnf(TEXT("\tModule Outer..............%s"), *(OuterObj->GetPathName()));
						warnf(TEXT("\tModule Outermost..........%s"), *(Module->GetOutermost()->GetPathName()));
						warnf(TEXT("\tEmitter Outer.............%s"), *(EmitterOuter->GetPathName()));
						warnf(TEXT("\tEmitter Outermost.........%s"), *(GetOutermost()->GetPathName()));
						bWarned = TRUE;
					}
				}

				if (!bWarned)
				{
					for (INT ModuleIndex = 0; (ModuleIndex < LODLevel->Modules.Num()) && !bWarned; ModuleIndex++)
					{
						Module = LODLevel->Modules(ModuleIndex);
						if (Module)
						{
							Module->ConditionalPostLoad();

							UObject* OuterObj = Module->GetOuter();
							check(OuterObj);
							if (OuterObj != EmitterOuter)
							{
								warnf(TEXT("UParticleModule %s has an incorrect outer on %s... run FixupEmitters on package %s (%s)"),
									*(Module->GetPathName()), 
									*(EmitterOuter->GetPathName()),
									*(OuterObj->GetOutermost()->GetPathName()),
									*(GetOutermost()->GetPathName()));
								warnf(TEXT("\tModule Outer..............%s"), *(OuterObj->GetPathName()));
								warnf(TEXT("\tModule Outermost..........%s"), *(Module->GetOutermost()->GetPathName()));
								warnf(TEXT("\tEmitter Outer.............%s"), *(EmitterOuter->GetPathName()));
								warnf(TEXT("\tEmitter Outermost.........%s"), *(GetOutermost()->GetPathName()));
								bWarned = TRUE;
							}
						}
					}
				}
			}
		}
	}
	else
	{
		// Need to force updating to the new LODLevel setup.
		// This won't be necessary once all packages are converted.
		if (GetLinker() && (GetLinker()->Ver() < VER_CHANGE_EMITTER_TO_LODMODEL))
		{
			ConvertedModules = FALSE;
		}
	}
   
	// Move the old ParticleEmitter setups to the new LOD level model.
	if ((GetLinker() && (GetLinker()->Ver() < VER_CHANGE_EMITTER_TO_LODMODEL)) || (LODLevels.Num() == 0))
	{
		// Remove any 'empty' modules from the array
		// This has only been seen on a single system, AW_EphyraStreets.ChiggerTrackParticle
		for (INT ModuleIndex = Modules.Num() - 1; ModuleIndex >= 0 ; ModuleIndex--)
		{
			if (Modules(ModuleIndex) == NULL)
			{
				Modules.Remove(ModuleIndex);
			}
		}

		// Create a ParticleLODLevel
		LODLevels.InsertZeroed(0, 1);
		UParticleLODLevel* LODLevel = ConstructObject<UParticleLODLevel>(UParticleLODLevel::StaticClass(), this);
		check(LODLevel);
		LODLevels(0)			= LODLevel;
		LODLevel->Level					= 0;
		LODLevel->LevelSetting			= 0;
		LODLevel->ConvertedModules		= TRUE;
		LODLevel->PeakActiveParticles	= PeakActiveParticles;
  
		// Convert to the RequiredModule
		UParticleModuleRequired* RequiredModule	= ConstructObject<UParticleModuleRequired>(UParticleModuleRequired::StaticClass(), GetOuter());
		check(RequiredModule);
		LODLevel->RequiredModule	= RequiredModule;
		// make sure the spawnrate object is fully loaded before duping it
		SpawnRate.Distribution->GetLinker()->Preload(SpawnRate.Distribution);
		UObject*	DupObject	= UObject::StaticDuplicateObject(SpawnRate.Distribution, SpawnRate.Distribution, RequiredModule, TEXT("None"));
		check(DupObject);
		RequiredModule->SpawnRate.Distribution			= Cast<UDistributionFloat>(DupObject);
		RequiredModule->bUseLocalSpace		= UseLocalSpace;
		RequiredModule->bKillOnDeactivate	= KillOnDeactivate;
		RequiredModule->bKillOnCompleted	= bKillOnCompleted;
		RequiredModule->EmitterDuration		= EmitterDuration;
		RequiredModule->EmitterLoops		= EmitterLoops;
		RequiredModule->ParticleBurstMethod	= ParticleBurstMethod;
		RequiredModule->BurstList.Empty();
		RequiredModule->BurstList.AddZeroed(BurstList.Num());
		for (INT BurstIndex = 0; BurstIndex < BurstList.Num(); BurstIndex++)
		{
			RequiredModule->BurstList(BurstIndex).Count	= BurstList(BurstIndex).Count;
			RequiredModule->BurstList(BurstIndex).Time	= BurstList(BurstIndex).Time;
		}

		RequiredModule->ModuleEditorColor		= FColor::MakeRandomColor();
		RequiredModule->InterpolationMethod		= InterpolationMethod;
		if (SubImages_Horizontal > 0)
		{
			RequiredModule->SubImages_Horizontal	= SubImages_Horizontal;
		}
		else
		{
			RequiredModule->SubImages_Horizontal	= 1;
		}
		if (SubImages_Vertical > 0)
		{
			RequiredModule->SubImages_Vertical	= SubImages_Vertical;
		}
		else
		{
			RequiredModule->SubImages_Vertical	= 1;
		}
		RequiredModule->bScaleUV				= ScaleUV;
		RequiredModule->RandomImageTime			= RandomImageTime;
		RequiredModule->RandomImageChanges		= RandomImageChanges;
		RequiredModule->bDirectUV				= DirectUV;
		RequiredModule->EmitterRenderMode		= EmitterRenderMode;
		RequiredModule->EmitterEditorColor		= EmitterEditorColor;
		RequiredModule->bEnabled				= TRUE;

		// Copy the TypeData module
		LODLevel->TypeDataModule				= TypeDataModule;

		// Copy the modules
		LODLevel->Modules.InsertZeroed(0, Modules.Num());
		for (INT ModuleIndex = 0; ModuleIndex < Modules.Num(); ModuleIndex++)
		{
			// make sure the modules are fully loaded so AutogenerateLowestLODLevel can use the modules
			Modules(ModuleIndex)->GetLinker()->Preload(Modules(ModuleIndex));

			LODLevel->Modules(ModuleIndex) = Modules(ModuleIndex);
		}

		LODLevel->UpdateModuleLists();

		MarkPackageDirty();
	}

	ConvertedModules = TRUE;

	if ((GIsGame == FALSE) && GetLinker() && (GetLinker()->Ver() < VER_EMITTER_LOWEST_LOD_REGENERATION))
	{
		// Check the LOD levels to see if they are unedited.
		// If so, re-generate the lowest.
		if (LODLevels.Num() == 2)
		{
			// There are only two levels, so it is likely unedited.
			UBOOL bRegenerate = TRUE;
			UParticleLODLevel* LODLevel = LODLevels(1);
			if (LODLevel)
			{
				if (LODLevel->RequiredModule->bEditable == TRUE)
				{
					bRegenerate = FALSE;
				}
				else
				{
					for (INT ModuleIndex = 0; ModuleIndex < LODLevel->Modules.Num(); ModuleIndex++)
					{
						UParticleModule* Module = LODLevel->Modules(ModuleIndex);
						if (Module)
						{
							if (Module->bEditable == TRUE)
							{
								bRegenerate = FALSE;
								break;
							}
						}
					}
				}

				if (bRegenerate == TRUE)
				{
					LODLevels.Remove(1);
					// The re-generation will happen below...
				}
			}
		}
	}


	// this will look at all of the emitters and then remove ones that some how have become NULL (e.g. from a removal of an Emitter where content
	// is still referencing it)
	for (INT LODIndex = 0; LODIndex < LODLevels.Num(); LODIndex++)
	{
		UParticleLODLevel* LODLevel = LODLevels(LODIndex);
		if (LODLevel)
		{
			for (INT ModuleIndex = LODLevel->Modules.Num()-1; ModuleIndex >= 0; ModuleIndex--)
			{
				UParticleModule* ParticleModule = LODLevel->Modules(ModuleIndex);
				if( ParticleModule == NULL )
				{
					LODLevel->Modules.Remove(ModuleIndex);
					MarkPackageDirty( TRUE );
				}
			}
		}
	}


	UObject* MyOuter = GetOuter();
	UParticleSystem* PSysOuter = Cast<UParticleSystem>(MyOuter);
	UBOOL bRegenDup = FALSE;
	if (PSysOuter)
	{
		bRegenDup = PSysOuter->bRegenerateLODDuplicate;
	}
	if (AutogenerateLowestLODLevel(bRegenDup) == FALSE)
	{
		warnf(TEXT("Failed to auto-generate low LOD level!"));
	}
	else
	{
		UpdateModuleLists();
	}

	if (GetLinker() && (GetLinker()->Ver() < VER_CHANGED_LINKACTION_TYPE))
	{
		warnf(TEXT("ParticleSystem - potential enable flag issues - %s"), *GetName());

		UParticleLODLevel* LODLevel;
		for (INT LODIndex = 0; LODIndex < LODLevels.Num(); LODIndex++)
		{
			LODLevel	= LODLevels(LODIndex);
			LODLevel->bEnabled	= bEnabled;
			LODLevel->RequiredModule->bEnabled	= bEnabled;
		}
	}
	else
	{
		UParticleLODLevel* LODLevel;
		for (INT LODIndex = 0; LODIndex < LODLevels.Num(); LODIndex++)
		{
			LODLevel	= LODLevels(LODIndex);
			LODLevel->bEnabled	= LODLevel->RequiredModule->bEnabled;
		}
	}

	if (GetLinker() && (GetLinker()->Ver() < VER_RECALC_PEAK_ACTIVE_PARTICLES))
	{
		CalculateMaxActiveParticleCount();
		MarkPackageDirty();
	}
}

void UParticleEmitter::PreEditChange(UProperty* PropertyThatWillChange)
{
	Super::PreEditChange(PropertyThatWillChange);
}

void UParticleEmitter::PostEditChange(UProperty* PropertyThatChanged)
{
	check(GIsEditor);

	// Reset the peak active particle counts.
	// This could check for changes to SpawnRate and Burst and only reset then,
	// but since we reset the particle system after any edited property, it
	// may as well just autoreset the peak counts.
	for (INT LODIndex = 0; LODIndex < LODLevels.Num(); LODIndex++)
	{
		UParticleLODLevel* LODLevel = LODLevels(LODIndex);
		if (LODLevel)
		{
			LODLevel->PeakActiveParticles	= 1;
		}
	}

	UpdateModuleLists();

	for (TObjectIterator<UParticleSystemComponent> It;It;++It)
	{
		if (It->Template)
		{
			INT i;

			for (i=0; i<It->Template->Emitters.Num(); i++)
			{
				if (It->Template->Emitters(i) == this)
				{
					It->UpdateInstances();
				}
			}
		}
	}

	Super::PostEditChange(PropertyThatChanged);

	if (CalculateMaxActiveParticleCount() == FALSE)
	{
		//
	}
}

void UParticleEmitter::SetEmitterName(FName Name)
{
	EmitterName = Name;
}

FName& UParticleEmitter::GetEmitterName()
{
	return EmitterName;
}

void UParticleEmitter::SetLODCount(INT LODCount)
{
	// 
}

void UParticleEmitter::AddEmitterCurvesToEditor(UInterpCurveEdSetup* EdSetup)
{
	debugf(TEXT("UParticleEmitter::AddEmitterCurvesToEditor> Should no longer be called..."));
	return;
//	UParticleLODLevel* LODLevel = LODLevels(0);
//	EdSetup->AddCurveToCurrentTab(LODLevel->RequiredModule->SpawnRate, FString(TEXT("Spawn Rate")), LODLevel->RequiredModule->EmitterEditorColor);
}

void UParticleEmitter::RemoveEmitterCurvesFromEditor(UInterpCurveEdSetup* EdSetup)
{
	for (INT LODIndex = 0; LODIndex < LODLevels.Num(); LODIndex++)
	{
		UParticleLODLevel* LODLevel = LODLevels(LODIndex);
		if (LODLevel->RequiredModule->IsDisplayedInCurveEd(EdSetup))
		{
			EdSetup->RemoveCurve(LODLevel->RequiredModule->SpawnRate.Distribution);
		}
		// Remove each modules curves as well.
		for (INT ii = 0; ii < LODLevel->Modules.Num(); ii++)
		{
			if (LODLevel->Modules(ii)->IsDisplayedInCurveEd(EdSetup))
			{
				// Remove it from the curve editor!
				LODLevel->Modules(ii)->RemoveModuleCurvesFromEditor(EdSetup);
			}
		}
	}
}

void UParticleEmitter::ChangeEditorColor(FColor& Color, UInterpCurveEdSetup* EdSetup)
{
	UParticleLODLevel* LODLevel = LODLevels(0);
	LODLevel->RequiredModule->EmitterEditorColor	= Color;
	for (INT TabIndex = 0; TabIndex < EdSetup->Tabs.Num(); TabIndex++)
	{
		FCurveEdTab*	Tab = &(EdSetup->Tabs(TabIndex));
		for (INT CurveIndex = 0; CurveIndex < Tab->Curves.Num(); CurveIndex++)
		{
			FCurveEdEntry* Entry	= &(Tab->Curves(CurveIndex));
			if (LODLevel->RequiredModule->SpawnRate.Distribution == Entry->CurveObject)
			{
				Entry->CurveColor	= Color;
			}
		}
	}
}

void UParticleEmitter::AutoPopulateInstanceProperties(UParticleSystemComponent* PSysComp)
{
	for (INT LODIndex = 0; LODIndex < LODLevels.Num(); LODIndex++)
	{
		UParticleLODLevel* LODLevel	= LODLevels(LODIndex);
		for (INT ModuleIndex = 0; ModuleIndex < LODLevel->Modules.Num(); ModuleIndex++)
		{
			UParticleModule* Module = LODLevel->Modules(ModuleIndex);
			Module->AutoPopulateInstanceProperties(PSysComp);
		}
	}
}

/** CreateLODLevel
 *	Creates the given LOD level.
 *	Intended for editor-time usage.
 *	Assumes that the given LODLevel will be in the [0..100] range.
 *	
 *	@return The index of the created LOD level
 */
INT UParticleEmitter::CreateLODLevel(INT LODLevel, UBOOL bGenerateModuleData)
{
	INT					LevelIndex		= -1;
	UParticleLODLevel*	CreatedLODLevel	= NULL;
	UParticleLODLevel*	HighLODLevel	= NULL;
	UParticleLODLevel*	LowLODLevel		= NULL;

	UParticleLODLevel*	NextHighestLODLevel	= NULL;
	UParticleLODLevel*	NextLowestLODLevel	= NULL;

	if (GetClosestLODLevels(LODLevel, HighLODLevel, LowLODLevel))
	{
		if (HighLODLevel && (HighLODLevel->LevelSetting == LODLevel))
		{
			CreatedLODLevel	= HighLODLevel;
		}

		if (LowLODLevel && (LowLODLevel->LevelSetting == LODLevel))
		{
			CreatedLODLevel	= LowLODLevel;
		}
	}

	if (CreatedLODLevel == NULL)
	{
		// Create a ParticleLODLevel
		CreatedLODLevel = ConstructObject<UParticleLODLevel>(UParticleLODLevel::StaticClass(), this);
		check(CreatedLODLevel);

		CreatedLODLevel->LevelSetting			= LODLevel;
		CreatedLODLevel->ConvertedModules		= TRUE;
		CreatedLODLevel->PeakActiveParticles	= 0;

		// Determine where to place it...
		if (LODLevels.Num() == 0)
		{
			LODLevels.InsertZeroed(0, 1);
			LODLevels(0)	= CreatedLODLevel;
			CreatedLODLevel->Level	= 0;
		}
		else
		{
			//@todo. This loop is not needed... We can use HighLODLevel and LowLODLevel obtained above.
			for (INT CheckIndex = 0; CheckIndex < LODLevels.Num(); CheckIndex++)
			{
				UParticleLODLevel* CheckLevel	= LODLevels(CheckIndex);
				if (CheckLevel->LevelSetting < LODLevel)
				{
					NextLowestLODLevel = CheckLevel;
				}
				else
				if (CheckLevel->LevelSetting > LODLevel)
				{
					// Insert prior to this setting...
					LODLevels.InsertZeroed(CheckIndex, 1);
					LODLevels(CheckIndex)					= CreatedLODLevel;
					CreatedLODLevel->Level					= CheckIndex;
					for (INT RemapIndex = CheckIndex + 1; RemapIndex < LODLevels.Num(); RemapIndex++)
					{
						UParticleLODLevel* RemapLevel	= LODLevels(RemapIndex);
						if (RemapLevel)
						{
							RemapLevel->Level++;
						}
					}
					break;
				}
				else
				{
					NextHighestLODLevel	= CheckLevel;
				}
			}
		}

		if (NextLowestLODLevel)
		{
			// Generate from the more detailed LOD level
			FLOAT Multiplier	= ((FLOAT)(100 - LODLevel) / (FLOAT)(100 - NextLowestLODLevel->LevelSetting)) * 100.0f;
			if (CreatedLODLevel->GenerateFromLODLevel(NextLowestLODLevel, Multiplier, bGenerateModuleData) == FALSE)
			{
				warnf(TEXT("Failed to generate LOD level %d from level %d (Multiplier = %f)"),
					LODLevel, NextLowestLODLevel->LevelSetting, Multiplier);
			}
		}
		else
		if (NextHighestLODLevel)
		{
			// Generate from the higher LOD level
#if 1
			FLOAT Multiplier	= ((FLOAT)(100 - NextHighestLODLevel->LevelSetting) / (FLOAT)(100 - LODLevel)) * 100.0f;
			if (CreatedLODLevel->GenerateFromLODLevel(NextHighestLODLevel, Multiplier, bGenerateModuleData) == FALSE)
			{
				warnf(TEXT("Failed to generate LOD level %d from level %d (Multiplier = %f)"),
					LODLevel, NextHighestLODLevel->LevelSetting, Multiplier);
			}
#else
			if (CreatedLODLevel->GenerateFromBoundingLODLevels(HighLODLevel, LowLODLevel) == FALSE)
			{
				warnf(TEXT("Failed to generate LOD level %d from levels %d and %d"),
					LODLevel, HighLODLevel->LevelSetting, LowLODLevel->LevelSetting);
			}
#endif
		}
		else
		{
			// Create the RequiredModule
			UParticleModuleRequired* RequiredModule	= ConstructObject<UParticleModuleRequired>(UParticleModuleRequired::StaticClass(), GetOuter());
			check(RequiredModule);
			CreatedLODLevel->RequiredModule	= RequiredModule;

			// The SpawnRate for the required module
			check(RequiredModule->SpawnRate.Distribution);
			UDistributionFloatConstant* ConstantSpawn	= Cast<UDistributionFloatConstant>(RequiredModule->SpawnRate.Distribution);
			ConstantSpawn->Constant					= 10;
			ConstantSpawn->bIsDirty					= TRUE;
			RequiredModule->bUseLocalSpace			= FALSE;
			RequiredModule->bKillOnDeactivate		= FALSE;
			RequiredModule->bKillOnCompleted		= FALSE;
			RequiredModule->EmitterDuration			= 1.0f;
			RequiredModule->EmitterLoops			= 0;
			RequiredModule->ParticleBurstMethod		= EPBM_Instant;
			RequiredModule->BurstList.Empty();
			RequiredModule->ModuleEditorColor		= FColor::MakeRandomColor();
			RequiredModule->InterpolationMethod		= PSUVIM_None;
			RequiredModule->SubImages_Horizontal	= 1;
			RequiredModule->SubImages_Vertical		= 1;
			RequiredModule->bScaleUV				= FALSE;
			RequiredModule->RandomImageTime			= 0.0f;
			RequiredModule->RandomImageChanges		= 0;
			RequiredModule->bDirectUV				= FALSE;
			RequiredModule->EmitterRenderMode		= ERM_Normal;
			RequiredModule->EmitterEditorColor		= FColor::MakeRandomColor();
			RequiredModule->bEnabled				= TRUE;

			// Copy the TypeData module
			CreatedLODLevel->TypeDataModule			= NULL;
		}

		LevelIndex	= CreatedLODLevel->Level;

		MarkPackageDirty();
	}

	return LevelIndex;
}

/** IsLODLevelValid
 *	Returns TRUE if the given LODLevel is one of the array entries.
 *	Intended for editor-time usage.
 *	Assumes that the given LODLevel will be in the [0..100] range.
 *	
 *	@return FALSE if the requested LODLevel is not valid.
 *			TRUE if the requested LODLevel is valid.
 */
UBOOL UParticleEmitter::IsLODLevelValid(INT LODLevel)
{
	for (INT LODIndex = 0; LODIndex < LODLevels.Num(); LODIndex++)
	{
		UParticleLODLevel* CheckLODLevel	= LODLevels(LODIndex);
		if (CheckLODLevel->LevelSetting == LODLevel)
		{
			return TRUE;
		}
	}

	return FALSE;
}

/**
 * This will update the LOD of the particle in the editor.
 *
 * @see GetCurrentLODLevel(FParticleEmitterInstance* Instance)
 **/
void UParticleEmitter::EditorUpdateCurrentLOD(FParticleEmitterInstance* Instance)
{
	UParticleLODLevel*	CurrentLODLevel	= NULL;
	UParticleLODLevel*	Higher			= NULL;
	UParticleLODLevel*	Lower			= NULL;

	for (INT LevelIndex = 0; LevelIndex < (LODLevels.Num() - 1); LevelIndex++)
	{
		Higher	= LODLevels(LevelIndex);
		Lower	= LODLevels(LevelIndex + 1);
		if ((Higher->LevelSetting <= Instance->Component->EditorLODLevel) &&
			(Lower->LevelSetting > Instance->Component->EditorLODLevel))
		{
			CurrentLODLevel	= Higher;
			break;
		}
	}

	if (CurrentLODLevel == NULL)
	{
		check(Lower);
		CurrentLODLevel	= Lower;
	}

	Instance->CurrentLODLevelIndex	= CurrentLODLevel->Level;
	Instance->CurrentLODLevel		= CurrentLODLevel;
	Instance->EmitterDuration		= Instance->EmitterDurations(Instance->CurrentLODLevelIndex);
}


/** GetLODLevel
 *	Returns the given LODLevel. Intended for game-time usage.
 *	Assumes that the given LODLevel will be in the [0..# LOD levels] range.
 *	
 *	@param	LODLevel - the requested LOD level in the range [0..# LOD levels].
 *
 *	@return NULL if the requested LODLevel is not valid.
 *			The pointer to the requested UParticleLODLevel if valid.
 */
UParticleLODLevel* UParticleEmitter::GetLODLevel(INT LODLevel)
{
	if (LODLevel >= LODLevels.Num())
	{
		return NULL;
	}

	return LODLevels(LODLevel);
}

/**	GetClosestLODLevel
 *	Returns the closest LODLevel to the requested value.
 *	Intended for game-time usage.
 *	Assumes that the given LODLevel will be in the [0..100] range.
 *
 *	@param	LODLevel - the desired LOD level in the range [0..100].
 *
 *	@return	Pointer to the closed UParticleLODLevel.
 */
UParticleLODLevel* UParticleEmitter::GetClosestLODLevel(INT LODLevel)
{
	UParticleLODLevel* HigherLOD;
	UParticleLODLevel* LowerLOD;

	if (GetClosestLODLevels(LODLevel, HigherLOD, LowerLOD))
	{
		INT	HighDiff	= HigherLOD ? (HigherLOD->Level - LODLevel) : 100;
		INT	LowDiff		= LowerLOD ? (LODLevel - LowerLOD->Level) : 0;

		if (HighDiff < LowDiff)
		{
			return HigherLOD;
		}
		else
		{
			return LowerLOD;
		}
	}
	
	return NULL;
}

/**	GetClosestLODLevels
 *	Returns the two closest UParticleLODLevels to the requested value.
 *	Intended for editor usage.
 *	Assumes that the given LODLevel will be in the [0..100] range.
 *
 *	@param	LODLevel	- The requested LOD level, range [0..100].
 *	@param	HigherLOD	- Filled in with the higher LOD level.
 *	@param	LowerLOD	- Filled in with the lower LOD level.
 *
 *	@return	TRUE if successful.
 *			FALSE if not.
 */
UBOOL UParticleEmitter::GetClosestLODLevels(INT LODLevel, UParticleLODLevel*& HigherLOD, UParticleLODLevel*& LowerLOD)
{
	HigherLOD	= NULL;
	LowerLOD	= NULL;

	INT	CurrentLowDiff	= 101;
	INT	CurrentHighDiff	= 101;

	for (INT LODLevelIndex = 0; LODLevelIndex < LODLevels.Num(); LODLevelIndex++)
	{
		UParticleLODLevel* CheckLevel = LODLevels(LODLevelIndex);
		if (CheckLevel)
		{
			if ((CheckLevel->LevelSetting - LODLevel) < 0)
			{
				INT	Diff = LODLevel - CheckLevel->LevelSetting;
				if (Diff < CurrentHighDiff)
				{
					CurrentHighDiff	= Diff;
					HigherLOD		= CheckLevel;
				}
			}
			else
			if ((CheckLevel->LevelSetting - LODLevel) > 0)
			{
				INT	Diff = CheckLevel->LevelSetting - LODLevel;
				if (Diff < CurrentLowDiff)
				{
					CurrentLowDiff	= Diff;
					LowerLOD		= CheckLevel;
				}
			}
			else
			{
				CurrentHighDiff	= 0;
				CurrentLowDiff	= 0;
				HigherLOD		= CheckLevel;
				LowerLOD		= CheckLevel;
			}
		}
	}

	return TRUE;
}

UBOOL UParticleEmitter::GetLODLevelModules(INT LODLevel, TArray<UParticleModule*>*& LevelModules)
{
	return FALSE;
}

UBOOL UParticleEmitter::GetClosestLODLevelModules(INT LODLevel, TArray<UParticleModule*>*& HigherLODModule, TArray<UParticleModule*>*& LowerLODModules)
{
	return FALSE;
}

UBOOL UParticleEmitter::GetLODLevelSpawnModules(INT LODLevel, TArray<UParticleModule*>*& LevelSpawnModules)
{
	return FALSE;
}

UBOOL UParticleEmitter::GetClosestLODLevelSpawnModules(INT LODLevel, 
													   TArray<UParticleModule*>*& HigherLODSpawnModule, 
													   TArray<UParticleModule*>*& LowerLODSpawnModules
													   )
{
	return FALSE;
}

UBOOL UParticleEmitter::GetLODLevelUpdateModules(INT LODLevel, TArray<UParticleModule*>*& LevelUpdateModules)
{
	return FALSE;
}

UBOOL UParticleEmitter::GetClosestLODLevelUpdateModules(INT LODLevel, 
														TArray<UParticleModule*>*& HigherLODUpdateModule, 
														TArray<UParticleModule*>*& LowerLODUpdateModules
														)
{
	return FALSE;
}

UBOOL UParticleEmitter::AutogenerateLowestLODLevel(UBOOL bDuplicateHighest)
{
	for (INT LODIndex = 0; LODIndex < LODLevels.Num(); LODIndex++)
	{
		UParticleLODLevel* LODLevel	= LODLevels(LODIndex);
		if (LODLevel)
		{
			if (LODLevel->LevelSetting == 100)
			{
				// Make sure it's the last on in the array!
				if (LODIndex != (LODLevels.Num() - 1))
				{
					warnf(TEXT("Lowest LOD level is not the last in the array!"));
				}

				// Check each module...

				return TRUE;
			}
		}
	}

	// Didn't find it?
	if (LODLevels.Num() == 1)
	{
		// We need to generate it...
//		debugf(TEXT("Autogeneration of lowest LOD level commencing..."));

		// 
		LODLevels.InsertZeroed(1, 1);
		UParticleLODLevel* LODLevel = ConstructObject<UParticleLODLevel>(UParticleLODLevel::StaticClass(), this);
		check(LODLevel);
		LODLevels(1)					= LODLevel;
		LODLevel->Level					= 1;
		LODLevel->LevelSetting			= 100;
		LODLevel->ConvertedModules		= TRUE;
		LODLevel->PeakActiveParticles	= 0;

		// Grab LODLevel 0 for creation
		UParticleLODLevel* SourceLODLevel	= LODLevels(0);

		LODLevel->bEnabled				= SourceLODLevel->bEnabled;

		FLOAT Percentage	= 10.0f;
		if (SourceLODLevel->TypeDataModule)
		{
			UParticleModuleTypeDataTrail2*	Trail2TD	= Cast<UParticleModuleTypeDataTrail2>(SourceLODLevel->TypeDataModule);
			UParticleModuleTypeDataBeam2*	Beam2TD		= Cast<UParticleModuleTypeDataBeam2>(SourceLODLevel->TypeDataModule);

			if (Trail2TD || Beam2TD)
			{
				// For now, don't support LOD on beams and trails
				Percentage	= 100.0f;
			}
		}

		if (bDuplicateHighest == TRUE)
		{
			Percentage = 100.0f;
		}

		if (LODLevel->GenerateFromLODLevel(SourceLODLevel, Percentage) == FALSE)
		{
			warnf(TEXT("Failed to generate LOD level %d from LOD level 0"), 1);
			return FALSE;
		}

		MarkPackageDirty();
		return TRUE;
	}
	else
	{
		// There are multiple LOD levels, but none is the lowest.
		warnf(TEXT("No lowest LOD level in multi-entry array!"));
	}

	return FALSE;
}

/**
 *	CalculateMaxActiveParticleCount
 *	Determine the maximum active particles that could occur with this emitter.
 *	This is to avoid reallocation during the life of the emitter.
 *
 *	@return	TRUE	if the number was determined
 *			FALSE	if the number could not be determined
 */
UBOOL UParticleEmitter::CalculateMaxActiveParticleCount()
{
	INT	CurrMaxAPC = 0;

	UBOOL bIsBeamOrTrail = FALSE;
	INT MaxCount = 0;
	
	// Check for beams or trails
	if (TypeDataModule != NULL)
	{
		UParticleModuleTypeDataBeam2* BeamTD = Cast<UParticleModuleTypeDataBeam2>(TypeDataModule);
		UParticleModuleTypeDataTrail2* TrailTD = Cast<UParticleModuleTypeDataTrail2>(TypeDataModule);
		if (BeamTD || TrailTD)
		{
			bIsBeamOrTrail = TRUE;
			if (BeamTD)
			{
				MaxCount = BeamTD->MaxBeamCount + 2;
			}
			if (TrailTD)
			{
				MaxCount = (TrailTD->MaxTrailCount * TrailTD->MaxParticleInTrailCount) + 2;
			}
		}
	}

	for (INT LODIndex = 0; LODIndex < LODLevels.Num(); LODIndex++)
	{
		UParticleLODLevel* LODLevel = LODLevels(LODIndex);
		if (LODLevel)
		{
			INT LODMaxAPC = LODLevel->CalculateMaxActiveParticleCount();
			if (bIsBeamOrTrail == TRUE)
			{
				LODLevel->PeakActiveParticles = MaxCount;
				LODMaxAPC = MaxCount;
			}

			if (LODMaxAPC > CurrMaxAPC)
			{
				if (LODIndex > 0)
				{
					// Check for a ridiculous difference in counts...
					if ((LODMaxAPC / CurrMaxAPC) > 2)
					{
						warnf(TEXT("MaxActiveParticleCount Discrepancy?\n\tLOD %2d, Emitter %16s"), LODIndex, *GetName());
					}
				}
				CurrMaxAPC = LODMaxAPC;
			}
		}
	}

	if ((GIsEditor == TRUE) && (CurrMaxAPC > 500))
	{
		//@todo. Added an option to the emitter to disable this warning - for 
		// the RARE cases where it is really required to render that many.
		warnf(TEXT("MaxCount = %4d for Emitter %s (%s)"),
			CurrMaxAPC, *(GetName()), GetOuter() ? *(GetOuter()->GetPathName()) : TEXT("????"));
	}

	return TRUE;
}

/*-----------------------------------------------------------------------------
	UParticleSpriteEmitter implementation.
-----------------------------------------------------------------------------*/
void UParticleSpriteEmitter::PostLoad()
{
	Super::PostLoad();

	if (GIsUCCMake == TRUE)
	{
		return;
	}

	if (Material != NULL)
	{
		UMaterial* UMat = Material->GetMaterial();
		if (UMat)
		{
			UMat->ConditionalPostLoad();
		}
	}

	// Moving the materials from the emitter to the RequiredModule (for LOD'ing)
	if (GetLinker() && (GetLinker()->Ver() < VER_PARTICLE_MATERIALS_TO_REQUIRED_MODULE))
	{
		// Copy the base material to the required module of each LOD level
		for (INT LODIndex = 0; LODIndex < LODLevels.Num(); LODIndex++)
		{
			UParticleLODLevel* LODLevel = LODLevels(LODIndex);
			if (LODLevel)
			{
				UParticleModuleRequired* RequiredModule = LODLevel->RequiredModule;
				if (RequiredModule)
				{
					RequiredModule->Material = Material;
					RequiredModule->ScreenAlignment = ScreenAlignment;
				}
			}
		}
		MarkPackageDirty();
	}

	// Postload the materials
	for (INT LODIndex = 0; LODIndex < LODLevels.Num(); LODIndex++)
	{
		UParticleLODLevel* LODLevel = LODLevels(LODIndex);
		if (LODLevel)
		{
			UParticleModuleRequired* RequiredModule = LODLevel->RequiredModule;
			if (RequiredModule)
			{
				if (RequiredModule->Material)
				{
					RequiredModule->Material->ConditionalPostLoad();
				}
			}
		}
	}
}

void UParticleSpriteEmitter::PostEditChange(UProperty* PropertyThatChanged)
{
	// Prevent the user from selecting TypeSpecific screen alignment for unsupported cases
	if (ScreenAlignment == PSA_TypeSpecific)
	{
		if (LODLevels.Num() > 0)
		{
			UParticleLODLevel* LODLevel	= LODLevels(0);
            UParticleModuleTypeDataBase* TypeData = Cast<UParticleModuleTypeDataBase>(LODLevel->TypeDataModule);
			if ((TypeData == NULL) || (TypeData->SupportsSpecificScreenAlignmentFlags() == FALSE))
			{
				ScreenAlignment = PSA_Square;
			}
		}
	}
	Super::PostEditChange(PropertyThatChanged);
}

FParticleEmitterInstance* UParticleSpriteEmitter::CreateInstance(UParticleSystemComponent* InComponent)
{
	FParticleEmitterInstance* Instance = 0;

	Particle_ConvertEmitterModules(this);

	UParticleLODLevel* LODLevel	= GetLODLevel(0);
	check(LODLevel);

	if (LODLevel->TypeDataModule)
	{
		//@todo. This will NOT work for trails/beams!
		UParticleModuleTypeDataBase* TypeData = CastChecked<UParticleModuleTypeDataBase>(LODLevel->TypeDataModule);
		if (TypeData)
		{
			Instance = TypeData->CreateInstance(this, InComponent);
		}
	}
	else
	{
		if ((EParticleSubUVInterpMethod)(LODLevel->RequiredModule->InterpolationMethod) != PSUVIM_None)
		{
			check(InComponent);
			Instance = new FParticleSpriteSubUVEmitterInstance();
			check(Instance);
			Instance->InitParameters(this, InComponent);
		}
	}

	if (Instance == NULL)
	{
		check(InComponent);
		Instance = new FParticleSpriteEmitterInstance();
		check(Instance);
		Instance->InitParameters(this, InComponent);
	}

	check(Instance);

	Instance->CurrentLODLevelIndex	= 0;
	Instance->CurrentLODLevel		= LODLevels(Instance->CurrentLODLevelIndex);

	Instance->Init();
	return Instance;
}

// Sets up this sprite emitter with sensible defaults so we can see some particles as soon as its created.
void UParticleSpriteEmitter::SetToSensibleDefaults()
{
	PreEditChange(NULL);

	UParticleLODLevel* LODLevel = LODLevels(0);

	// Spawn rate
	UDistributionFloatConstant* SpawnRateDist = Cast<UDistributionFloatConstant>(LODLevel->RequiredModule->SpawnRate.Distribution);
	if (SpawnRateDist)
	{
		SpawnRateDist->Constant = 20.f;
	}

	// Create basic set of modules

	// Lifetime module
	UParticleModuleLifetime* LifetimeModule = ConstructObject<UParticleModuleLifetime>(UParticleModuleLifetime::StaticClass(), GetOuter());
	UDistributionFloatUniform* LifetimeDist = Cast<UDistributionFloatUniform>(LifetimeModule->Lifetime.Distribution);
	if (LifetimeDist)
	{
		LifetimeDist->Min = 1.0f;
		LifetimeDist->Max = 1.0f;
		LifetimeDist->bIsDirty = TRUE;
	}
	LODLevel->Modules.AddItem(LifetimeModule);

	// Size module
	UParticleModuleSize* SizeModule = ConstructObject<UParticleModuleSize>(UParticleModuleSize::StaticClass(), GetOuter());
	UDistributionVectorUniform* SizeDist = Cast<UDistributionVectorUniform>(SizeModule->StartSize.Distribution);
	if (SizeDist)
	{
		SizeDist->Min = FVector(25.f, 25.f, 25.f);
		SizeDist->Max = FVector(25.f, 25.f, 25.f);
		SizeDist->bIsDirty = TRUE;
	}
	LODLevel->Modules.AddItem(SizeModule);

	// Initial velocity module
	UParticleModuleVelocity* VelModule = ConstructObject<UParticleModuleVelocity>(UParticleModuleVelocity::StaticClass(), GetOuter());
	UDistributionVectorUniform* VelDist = Cast<UDistributionVectorUniform>(VelModule->StartVelocity.Distribution);
	if (VelDist)
	{
		VelDist->Min = FVector(-10.f, -10.f, 50.f);
		VelDist->Max = FVector(10.f, 10.f, 100.f);
		VelDist->bIsDirty = TRUE;
	}
	LODLevel->Modules.AddItem(VelModule);

	PostEditChange(NULL);
}

/*-----------------------------------------------------------------------------
	UParticleSystem implementation.
-----------------------------------------------------------------------------*/
/** 
 *	Return the currently set LOD method
 */
BYTE UParticleSystem::GetCurrentLODMethod()
{
	return LODMethod;
}

/**
 *	Return the number of LOD levels for this particle system
 *
 *	@return	The number of LOD levels in the particle system
 */
INT UParticleSystem::GetLODLevelCount()
{
	return LODDistances.Num();
}

/**
 *	Return the distance for the given LOD level
 *
 *	@param	LODLevelIndex	The LOD level that the distance is being retrieved for
 *
 *	@return	-1.0f			If the index is invalid
 *			Distance		The distance set for the LOD level
 */
FLOAT UParticleSystem::GetLODDistance(INT LODLevelIndex)
{
	if (LODLevelIndex >= LODDistances.Num())
	{
		return -1.0f;
	}

	return LODDistances(LODLevelIndex);
}

/**
 *	Set the LOD method
 *
 *	@param	InMethod		The desired method
 */
void UParticleSystem::SetCurrentLODMethod(BYTE InMethod)
{
	LODMethod = (ParticleSystemLODMethod)InMethod;
}

/**
 *	Set the distance for the given LOD index
 *
 *	@param	LODLevelIndex	The LOD level to set the distance ofr
 *	@param	InDistance		The distance to set
 *
 *	@return	TRUE			If successful
 *			FALSE			Invalid LODLevelIndex
 */
UBOOL UParticleSystem::SetLODDistance(INT LODLevelIndex, FLOAT InDistance)
{
	if (LODLevelIndex >= LODDistances.Num())
	{
		return FALSE;
	}

	LODDistances(LODLevelIndex) = InDistance;

	return TRUE;
}

/**
 *	Handle edited properties
 *
 *	@param	PropertyThatChanged		The property that was changed
 */
void UParticleSystem::PostEditChange(UProperty* PropertyThatChanged)
{
	UpdateTime_Delta = 1.0f / UpdateTime_FPS;

	for (TObjectIterator<UParticleSystemComponent> It;It;++It)
	{
		if (It->Template == this)
		{
			It->UpdateInstances();
		}
	}

	if (PropertyThatChanged)
	{
		if (PropertyThatChanged->GetName() == TEXT("LODCount"))
		{
			debugf(TEXT("LODCount changed!"));
			if (LODCount <= 0)
			{
				LODCount	= 1;
			}

			// For each emitter, update the LOD count
			for (INT EmitterIndex = 0; EmitterIndex < Emitters.Num(); EmitterIndex++)
			{
				Emitters(EmitterIndex)->SetLODCount(LODCount);
			}
		}

		if (PropertyThatChanged->GetName() == TEXT("PreviewLODSetting"))
		{
			if (PreviewLODSetting > (LODCount - 1))
			{
				PreviewLODSetting	= LODCount - 1;
			}
			else
			if (PreviewLODSetting < 0)
			{
				PreviewLODSetting	= 0;
			}
		}
	}

	ThumbnailImageOutOfDate = TRUE;

	Super::PostEditChange(PropertyThatChanged);
}

void UParticleSystem::PreSave()
{
	Super::PreSave();
}

void UParticleSystem::PostLoad()
{
	Super::PostLoad();

	//@todo. Put this in a better place??
	bool bHadDeprecatedEmitters = FALSE;

	// Remove any old emitters
	UBOOL IsLit = FALSE;
	for (INT i = Emitters.Num() - 1; i >= 0; i--)
	{
		UParticleEmitter* Emitter = Emitters(i);
		if (Emitter == NULL)
		{
			// Empty emitter slots are ok with cooked content.
			if( !GUseSeekFreeLoading )
			{
				warnf(TEXT("ParticleSystem contains empty emitter slots - %s"), *GetFullName());
			}
			continue;
		}

		Emitter->ConditionalPostLoad();

		if (Emitter->IsA(UParticleSpriteSubUVEmitter::StaticClass()))
		{
			Emitters.Remove(i);
			MarkPackageDirty();
			bHadDeprecatedEmitters = TRUE;
		}
		else
		if (Emitter->IsA(UParticleMeshEmitter::StaticClass()))
		{
			Emitters.Remove(i);
			MarkPackageDirty();
			bHadDeprecatedEmitters = TRUE;
		}

		if (Emitter->IsA(UParticleSpriteEmitter::StaticClass()))
		{
			UBOOL bIsMeshEmitter = FALSE;
			UMaterial* UMat = NULL;
			UParticleSpriteEmitter* SpriteEmitter = Cast<UParticleSpriteEmitter>(Emitter);

			UParticleLODLevel* LODLevel = SpriteEmitter->LODLevels(0);
			check(LODLevel);
			
			LODLevel->ConditionalPostLoad();
			if (LODLevel->TypeDataModule)
			{
				//@todo. Check for lighting on mesh elements!
				UParticleModuleTypeDataMesh* MeshTD = Cast<UParticleModuleTypeDataMesh>(LODLevel->TypeDataModule);
				if (MeshTD)
				{
					bIsMeshEmitter = TRUE;

					UBOOL bQuickOut = FALSE;
					UStaticMesh* Mesh = MeshTD->Mesh;
					if (Mesh)
					{
						Mesh->ConditionalPostLoad();
						for (INT LODIndex = 0; (LODIndex < Mesh->LODModels.Num()) && (bQuickOut == FALSE); LODIndex++)
						{
							FStaticMeshRenderData* LOD = &(Mesh->LODModels(LODIndex));
							for (INT MatIndex = 0; (MatIndex < LOD->Elements.Num()) && (bQuickOut == FALSE); MatIndex++)
							{
								UMaterialInterface* MatInst = LOD->Elements(MatIndex).Material;
								if (MatInst)
								{
									MatInst->ConditionalPostLoad();
									UMat = MatInst->GetMaterial();
								}
							}
						}
					}
				}
			}

			if ((UMat == NULL) && (SpriteEmitter->Material != NULL))
			{
				SpriteEmitter->Material->ConditionalPostLoad();
				UMat = SpriteEmitter->Material->GetMaterial();
			}

			if (UMat && (UMat->LightingModel != MLM_Unlit))
			{
				IsLit = TRUE;

#if 0
					FString TempName = GetPathName();
					TempName.Replace(TEXT("."), TEXT(", "));
					debugf(NAME_ParticleWarn, TEXT("LIT: %s, %s, %s, %d"),*TempName, *UMat->GetName(), bIsMeshEmitter ? TEXT("Y") : TEXT("N"), i);
#endif
			}
#if defined(_PE_CHECK_LODLEVELS_)
			UBOOL bQuickOut = FALSE;
			for (INT LODLevelIndex = 0; (LODLevelIndex < SpriteEmitter->LODLevels.Num()) && (bQuickOut == FALSE); LODLevelIndex++)
			{
				UParticleLODLevel* LODLevel = SpriteEmitter->LODLevels(LODLevelIndex);
				if (LODLevel && LODLevel->TypeDataModule)
				{
					//@todo. Check for lighting on mesh elements!
					UParticleModuleTypeDataMesh* MeshTD = Cast<UParticleModuleTypeDataMesh>(LODLevel->TypeDataModule);
					if (MeshTD)
					{
						UStaticMesh* Mesh = MeshTD->Mesh;
						if (Mesh)
						{
							for (INT LODIndex = 0; (LODIndex < Mesh->LODModels.Num()) && (bQuickOut == FALSE); LODIndex++)
							{
								FStaticMeshRenderData* LOD = &(Mesh->LODModels(LODIndex));
								for (INT MatIndex = 0; (MatIndex < LOD->Elements.Num()) && (bQuickOut == FALSE); MatIndex++)
								{
									UMaterialInterface* MatInst = LOD->Elements(MatIndex).Material;
									if (MatInst)
									{
										MatInst->ConditionalPostLoad();
										UMaterial* Mat = MatInst->GetMaterial();
										if (Mat && Mat->LightingModel != MLM_Unlit)
										{
											IsLit = TRUE;
											if (GIsEditor == TRUE)
											{
												FString TempName = GetPathName();
												TempName.Replace(TEXT("."), TEXT(", "));
												debugf(NAME_ParticleWarn, TEXT("LIT: %s, %s, Y, %d"),*TempName, *Mat->GetName(), i);
											}
											bQuickOut = TRUE;
										}
									}
								}
							}
						}
					}
				}
			}
#endif	//#if defined(_PE_CHECK_LODLEVELS_)

#if !FINAL_RELEASE
#if CONSOLE
			// Do not cook out disabled emitter...
			UBOOL bDisabledEmitter = TRUE;
			for (INT LODLevelIndex = 0; (LODLevelIndex < SpriteEmitter->LODLevels.Num()) && (bDisabledEmitter == TRUE); LODLevelIndex++)
			{
				UParticleLODLevel* DisabledLODLevel = SpriteEmitter->LODLevels(LODLevelIndex);
				if (DisabledLODLevel)
				{
					check(DisabledLODLevel->RequiredModule);
					if (DisabledLODLevel->RequiredModule->bEnabled == TRUE)
					{
						bDisabledEmitter = FALSE;
					}
				}
			}

			if (bDisabledEmitter)
			{
				// We don't actually delete it, we just clear it's spot in the emitter array
				//warnf(NAME_ParticleWarn,  TEXT("Emitter %2d disabled in PSys   %s"), i, *(GetPathName()));
			}
#endif	//#if CONSOLE
#endif	//#if !FINAL_RELEASE

			//@todo. Move this into the editor and serialize?
			bHasPhysics = FALSE;
			for (INT LODIndex = 0; (LODIndex < Emitter->LODLevels.Num()) && (bHasPhysics == FALSE); LODIndex++)
			{
				//@todo. This is a temporary fix for emitters that apply physics.
				// Check for collision modules with bApplyPhysics set to TRUE
				UParticleLODLevel* LODLevel = Emitter->LODLevels(LODIndex);
				if (LODLevel)
				{
					for (INT ModuleIndex = 0; ModuleIndex < LODLevel->Modules.Num(); ModuleIndex++)
					{
						UParticleModuleCollision* CollisionModule = Cast<UParticleModuleCollision>(LODLevel->Modules(ModuleIndex));
						if (CollisionModule)
						{
							if (CollisionModule->bApplyPhysics == TRUE)
							{
								bHasPhysics = TRUE;
								break;
							}
						}
					}
				}
			}
		}
	}
	bLit = IsLit;

	if ((bHadDeprecatedEmitters || (GetLinker() && (GetLinker()->Ver() < 204))) && CurveEdSetup)
	{
		CurveEdSetup->ResetTabs();
	}

	// Add default LOD Distances
	if( ((GetLinker() && (GetLinker()->Ver() < VER_PARTICLESYSTEM_LODGROUP_SUPPORT)) || (LODDistances.Num() == 0)) && 
		(Emitters.Num() > 0) )
	{
		UParticleEmitter* Emitter = Emitters(0);
		if (Emitter)
		{
			LODDistances.Add(Emitter->LODLevels.Num());
			for (INT LODIndex = 0; LODIndex < LODDistances.Num(); LODIndex++)
			{
				LODDistances(LODIndex) = LODIndex * 2500.0f;
			}
		}
	}

#if !CONSOLE
	// Rescale and compress thumbnails.
	if( ThumbnailImage && GetLinker() && (GetLinker()->Ver() < VER_RESCALE_AND_COMPRESS_PARTICLE_THUMBNAILS) )
	{
		ThumbnailImage->ConditionalPostLoad();

		TArray<FColor> SrcImage;
		TArray<FColor> ScaledImage;

		// Retrieve raw data.
		void* RawSrcData = (void*)(ThumbnailImage->Mips(0).Data.Lock(LOCK_READ_WRITE));
		SrcImage.Add( ThumbnailImage->SizeX * ThumbnailImage->SizeY );
		check( ThumbnailImage->Mips(0).Data.GetBulkDataSize() == (SrcImage.Num() * SrcImage.GetTypeSize()) );
		appMemcpy( SrcImage.GetData(), RawSrcData, SrcImage.Num() * SrcImage.GetTypeSize() );
		ThumbnailImage->Mips(0).Data.Unlock();

		// Rescale.
		INT ScaledSizeX = 512;
		INT ScaledSizeY = 512;
		FImageUtils::ImageResize( ThumbnailImage->SizeX, ThumbnailImage->SizeY, SrcImage, ScaledSizeX, ScaledSizeY, ScaledImage );

		// Compress and replace.
		EObjectFlags ObjectFlags = ThumbnailImage->GetFlags();
		ThumbnailImage = FImageUtils::ConstructTexture2D( ScaledSizeX, ScaledSizeY, ScaledImage, this, TEXT("ThumbnailTexture"), ObjectFlags );

		MarkPackageDirty();
	}
#endif
}

void UParticleSystem::UpdateColorModuleClampAlpha(UParticleModuleColorBase* ColorModule)
{
	if (ColorModule)
	{
		ColorModule->RemoveModuleCurvesFromEditor(CurveEdSetup);
		ColorModule->AddModuleCurvesToEditor(CurveEdSetup);
	}
}

/**
 *	CalculateMaxActiveParticleCounts
 *	Determine the maximum active particles that could occur with each emitter.
 *	This is to avoid reallocation during the life of the emitter.
 *
 *	@return	TRUE	if the numbers were determined for each emitter
 *			FALSE	if not be determined
 */
UBOOL UParticleSystem::CalculateMaxActiveParticleCounts()
{
	UBOOL bSuccess = TRUE;

	for (INT EmitterIndex = 0; EmitterIndex < Emitters.Num(); EmitterIndex++)
	{
		UParticleEmitter* Emitter = Emitters(EmitterIndex);
		if (Emitter)
		{
			if (Emitter->CalculateMaxActiveParticleCount() == FALSE)
			{
				bSuccess = FALSE;
			}
		}
	}

	return bSuccess;
}

/*-----------------------------------------------------------------------------
	UParticleSystemComponent implementation.
-----------------------------------------------------------------------------*/
void UParticleSystemComponent::PostLoad()
{
	Super::PostLoad();

	if (Template)
	{
		Template->ConditionalPostLoad();
	}
	bIsViewRelevanceDirty = TRUE;

	// Initialize the system to avoid hitching
	//@todo.SAS. Do we want to make this a flag on the PSys?
	InitializeSystem();
}

void UParticleSystemComponent::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );
	
	// Take instance particle count/ size into account.
	for (INT InstanceIndex = 0; InstanceIndex < EmitterInstances.Num(); InstanceIndex++)
	{
		FParticleEmitterInstance* EmitterInstance = EmitterInstances(InstanceIndex);
		if( EmitterInstance != NULL )
		{
			INT Num, Max;
			EmitterInstance->GetAllocatedSize(Num, Max);
			Ar.CountBytes(Num, Max);
		}
	}
}

void UParticleSystemComponent::BeginDestroy()
{
	Super::BeginDestroy();
	ResetParticles(TRUE);
}

void UParticleSystemComponent::FinishDestroy()
{
	GParticleDataManager.RemoveParticleSystemComponent(this);

	for (INT EmitterIndex = 0; EmitterIndex < EmitterInstances.Num(); EmitterIndex++)
	{
		FParticleEmitterInstance* EmitInst = EmitterInstances(EmitterIndex);
		if (EmitInst)
		{
			delete EmitInst;
			EmitterInstances(EmitterIndex) = NULL;
		}
	}
	Super::FinishDestroy();
}

// Collision Handling...
UBOOL UParticleSystemComponent::SingleLineCheck(FCheckResult& Hit, AActor* SourceActor, const FVector& End, const FVector& Start, DWORD TraceFlags, const FVector& Extent)
{
	check(GWorld);

	return GWorld->SingleLineCheck(Hit, SourceActor, End, Start, TraceFlags, Extent);
}

void UParticleSystemComponent::Attach()
{
	// NULL out template if we're not allowing particles. This is not done in the Editor to avoid clobbering content via PIE.
	if( !GIsAllowingParticles && !GIsEditor )
	{
		Template = NULL;
	}

	if (Template)
	{
		this->bAcceptsLights = Template->bLit;
		if (Template->bHasPhysics)
		{
			TickGroup = TG_PreAsyncWork;
			
			AEmitter* EmitterOwner = Cast<AEmitter>(GetOwner());
			if (EmitterOwner)
			{
				EmitterOwner->TickGroup = TG_PreAsyncWork;
			}
		}

		if (LODLevel == -1)
		{
			// Force it to LODLevel 0
			LODLevel = 0;
		}
	}

	Super::Attach();

	if (Template && bAutoActivate && (EmitterInstances.Num() == 0 || bResetOnDetach))
	{
		InitializeSystem();
	}

	if (Template && (bIsActive == FALSE) && (bAutoActivate == TRUE)&& (EmitterInstances.Num() > 0) && (bWasDeactivated == FALSE))
	{
		SetActive(TRUE);
	}

	if (Template)
	{
//		if (SceneInfo != NULL)
		{
			GParticleDataManager.AddParticleSystemComponent(this);
		}
	}

	bJustAttached = TRUE;
}

void UParticleSystemComponent::UpdateTransform()
{
	if (bIsActive)
	{
		Super::UpdateTransform();
		GParticleDataManager.AddParticleSystemComponent(this);
	}
}

void UParticleSystemComponent::Detach()
{
	if (bResetOnDetach)
	{
		// Empty the EmitterInstance array.
		ResetParticles();
	}
	else
	{
		// tell emitter instances that we were detached, but don't clear them
		for (INT InstanceIndex = 0; InstanceIndex < EmitterInstances.Num(); InstanceIndex++)
		{
			FParticleEmitterInstance* EmitterInstance = EmitterInstances(InstanceIndex);
			if (EmitterInstance != NULL)
			{
				EmitterInstance->RemovedFromScene();
			}
		}
	}

	if (GIsGame == TRUE)
	{
		GParticleDataManager.RemoveParticleSystemComponent(this);
	}

	Super::Detach();
}

void UParticleSystemComponent::UpdateDynamicData()
{
	if (SceneInfo)
	{
		FParticleSystemSceneProxy* SceneProxy = (FParticleSystemSceneProxy*)Scene_GetProxyFromInfo(SceneInfo);
		UpdateDynamicData(SceneProxy);
	}
}

void UParticleSystemComponent::UpdateDynamicData(FParticleSystemSceneProxy* Proxy)
{
	if (Proxy)
	{
		if (EmitterInstances.Num() > 0)
		{
			BYTE CheckLODMethod = PARTICLESYSTEMLODMETHOD_DirectSet;
			if (bOverrideLODMethod)
			{
				CheckLODMethod = LODMethod;
			}
			else
			{
				if (Template)
				{
					CheckLODMethod = Template->LODMethod;
				}
			}

			if (CheckLODMethod == PARTICLESYSTEMLODMETHOD_Automatic)
			{
				INT LODIndex = 0;
				for (INT LODDistIndex = 1; LODDistIndex < Template->LODDistances.Num(); LODDistIndex++)
				{
					if (Template->LODDistances(LODDistIndex) > Proxy->GetPendingLODDistance())
					{
						break;
					}
					LODIndex = LODDistIndex;
				}

				if (LODIndex != LODLevel)
				{
					SetLODLevel(LODIndex);
				}
			}

			INT LiveCount = 0;
			for (INT EmitterIndex = 0; EmitterIndex < EmitterInstances.Num(); EmitterIndex++)
			{
				FParticleEmitterInstance* EmitInst = EmitterInstances(EmitterIndex);
				if (EmitInst)
				{
					if (EmitInst->ActiveParticles > 0)
					{
						LiveCount++;
					}
				}
			}
			if (LiveCount > 0)
			{
				FParticleDynamicData* CachedDynamicData = new FParticleDynamicData(this);
#if !FINAL_RELEASE
				//@todo.SAS. Remove thisline  - it is used for debugging purposes...
				Proxy->SetLastDynamicData(Proxy->GetDynamicData());
				//@todo.SAS. END
#endif // !FINAL_RELEASE
				Proxy->UpdateData(CachedDynamicData);
			}
			else
			{
				Proxy->UpdateData(NULL);
			}
		}
		else
		{
			Proxy->UpdateData(NULL);
		}
	}
}

void UParticleSystemComponent::PreEditChange(UProperty* PropertyThatWillChange)
{
	ResetParticles();
	Super::PreEditChange(PropertyThatWillChange);
}

void UParticleSystemComponent::PostEditChange(UProperty* PropertyThatChanged)
{
	bIsViewRelevanceDirty = TRUE;

	InitializeSystem();
	if (bAutoActivate)
	{
		ActivateSystem();
	}
	Super::PostEditChange(PropertyThatChanged);
}

void UParticleSystemComponent::UpdateBounds()
{
	FBox BoundingBox;
	BoundingBox.Init();

	if( Template && Template->bUseFixedRelativeBoundingBox )
	{
		// Use hardcoded relative bounding box from template.
		FVector BoundingBoxOrigin = LocalToWorld.GetOrigin();
		BoundingBox		 = Template->FixedRelativeBoundingBox;
		BoundingBox.Min += BoundingBoxOrigin;
		BoundingBox.Max += BoundingBoxOrigin;
	}
	else
	{
		BoundingBox += LocalToWorld.GetOrigin();
	
		for (INT i=0; i<EmitterInstances.Num(); i++)
		{
			FParticleEmitterInstance* EmitterInstance = EmitterInstances(i);
			if( EmitterInstance && EmitterInstance->HasActiveParticles() )
			{
				BoundingBox += EmitterInstance->GetBoundingBox();
			}
		}

		// Expand the actual bounding-box slightly so it will be valid longer in the case of expanding particle systems.
		BoundingBox = BoundingBox.ExpandBy(BoundingBox.GetExtent().Size() * 0.1f);
	}

	Bounds = FBoxSphereBounds(BoundingBox);
}

void UParticleSystemComponent::Tick(FLOAT DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_PSysCompTickTime);

#if TRACK_DETAILED_PARTICLE_TICK_STATS
	// Keep track of start time if tracking is enabled.
	DOUBLE StartTime = 0;
	// Create an instance on first use to avoid order of operations with regards to static construction.
	if( !GParticleTickStatManager )
	{
		GParticleTickStatManager = new FParticleTickStatManager();
	}
	// appSeconds can be comparatively costly so we only call it when the stats are enabled.
	if( GParticleTickStatManager->bIsEnabled )
	{
		StartTime = appSeconds();
	}
	// Cache the template as particle finished might reset it.
	UParticleSystem* CachedTemplate = Template;
#endif

	// Bail out if inactive and not AutoActivate
	if ((bIsActive == FALSE) && (bAutoActivate == FALSE))
	{
		return;
	}

	// Bail out if there is no template, there are no instances, or we're running a dedicated server and we don't update on those
	if ((Template == NULL) || (EmitterInstances.Num() == 0) || ((bUpdateOnDedicatedServer == FALSE) && (GWorld->GetNetMode() == NM_DedicatedServer)))
	{
		return;
	}

	// Bail out if MaxSecondsBeforeInactive > 0 and we haven't been rendered the last MaxSecondsBeforeInactive seconds.
	if (bWarmingUp == FALSE)
	{
		const FLOAT MaxSecondsBeforeInactive = Max( SecondsBeforeInactive, Template->SecondsBeforeInactive );
		if( MaxSecondsBeforeInactive > 0 
			&&	AccumTickTime > SecondsBeforeInactive
			&&	GIsGame )
		{
			const FLOAT CurrentTimeSeconds = GWorld->GetTimeSeconds();
			if( CurrentTimeSeconds > (LastRenderTime + MaxSecondsBeforeInactive) )
			{
				bForcedInActive = TRUE;
				return;
			}
		}
	}

	bForcedInActive = FALSE;
	DeltaTime *= GetOwner() ? GetOwner()->CustomTimeDilation : 1.f;

	AccumTickTime += DeltaTime;

	if (bUpdateComponentInTick && GetOwner() != NULL && (NeedsReattach() || NeedsUpdateTransform()))
	{
		if (GetOwner()->Components.ContainsItem(this))
		{
			// directly attached
			UpdateComponent(GetScene(), GetOwner(), GetOwner()->LocalToWorld());
		}
		else if (GWorld->TickGroup != TG_DuringAsyncWork) // we can't safely update everything during async work
		{
			// we have to tell the owner to update everything because we don't know what component we're attached to or how deep it is
			// we could fix this by having a pointer in ActorComponent to the SkeletalMeshComponent that is set in this case
			GetOwner()->ConditionalUpdateComponents(FALSE);
		}
		else
		{
			debugf(NAME_Warning, TEXT("%s (template: %s) needs update but can't be because it's in TG_DuringAsnycWork and indirectly attached!"), *GetName(), *Template->GetPathName());
		}
	}

	if ((GIsEditor == TRUE) && (GIsGame == FALSE))
	{
		return TickEditor(DeltaTime);
	}

	if (Template->SystemUpdateMode == EPSUM_FixedTime)
	{
		// Use the fixed delta time!
		DeltaTime = Template->UpdateTime_Delta;
	}

	// Tick Subemitters.
	INT TotalActiveParticles = 0;
	for (INT i=0; i<EmitterInstances.Num(); i++)
	{
		FParticleEmitterInstance* Instance = EmitterInstances(i);
		if (Instance && Instance->SpriteTemplate)
		{
			UParticleLODLevel* LODLevel = Instance->SpriteTemplate->GetCurrentLODLevel(Instance);
			if (LODLevel && LODLevel->bEnabled)
			{
				for (INT ParticleIndex = 0; ParticleIndex < Instance->ActiveParticles; ParticleIndex++)
				{
					PARTICLE_INSTANCE_PREFETCH(Instance, ParticleIndex);
				}
				Instance->Tick(DeltaTime, bSuppressSpawning);
				TotalActiveParticles += Instance->ActiveParticles;
			}
		}
	}

	// Indicate that we have been ticked since being attached.
	bJustAttached = FALSE;

	// If component has just totally finished, call script event.
	const UBOOL bIsCompleted = HasCompleted(); 
	if (bIsCompleted && !bWasCompleted)
	{
		if ( DELEGATE_IS_SET(OnSystemFinished) )
		{
			delegateOnSystemFinished(this);
		}

		if (bIsCachedInPool)
		{
			ConditionalDetach();
			SetHiddenGame(TRUE);
		}
		else
		{
			// When system is done - destroy all subemitters etc. We don't need them any more.
			ResetParticles();
		}	
	}
	bWasCompleted = bIsCompleted;

	// Update bounding box.
	if (!bWarmingUp && !bIsCompleted && !Template->bUseFixedRelativeBoundingBox)
	{
		// Compute the new system bounding box.
		FBox BoundingBox;
		BoundingBox.Init();
	
		// Calculate combined bounding box by combining them from emitter instances.
		BoundingBox += LocalToWorld.GetOrigin();
		for (INT i=0; i<EmitterInstances.Num(); i++)
		{
			FParticleEmitterInstance* Instance = EmitterInstances(i);
			if (Instance && Instance->SpriteTemplate)
			{
				UParticleLODLevel* LODLevel = Instance->SpriteTemplate->GetCurrentLODLevel(Instance);
	 			if (LODLevel && LODLevel->bEnabled)
				{
					BoundingBox += Instance->GetBoundingBox();
				}
			}
		}

		// Only update the primitive's bounding box in the octree if the system bounding box has gotten larger.
		if(!Bounds.GetBox().IsInside(BoundingBox.Min) || !Bounds.GetBox().IsInside(BoundingBox.Max))
		{
			ConditionalUpdateTransform();
		}
	}

	PartSysVelocity = (LocalToWorld.GetOrigin() - OldPosition) / DeltaTime;
	OldPosition = LocalToWorld.GetOrigin();

	if (bSkipUpdateDynamicDataDuringTick == FALSE)
	{
		GParticleDataManager.AddParticleSystemComponent(this);
	}

#if TRACK_DETAILED_PARTICLE_TICK_STATS
	if( GParticleTickStatManager->bIsEnabled )
	{
		GParticleTickStatManager->UpdateStats( CachedTemplate, appSeconds() - StartTime, TotalActiveParticles );	
	}
#endif
}

void UParticleSystemComponent::TickEditor(FLOAT DeltaTime)
{
	// 
	if (Template->SystemUpdateMode == EPSUM_FixedTime)
	{
		// Use the fixed delta time!
		DeltaTime = Template->UpdateTime_Delta;
	}

	// Tick Subemitters.
	for (INT i = 0; i < EmitterInstances.Num(); i++)
	{
		FParticleEmitterInstance* Instance = EmitterInstances(i);
		if (Instance && Instance->SpriteTemplate)
		{
			UParticleLODLevel* HighLODLevel	= NULL;
			UParticleLODLevel* LowLODLevel	= NULL;

			INT	LODSetting	= Template->EditorLODSetting;
			if (LODSetting < 0)
			{
				LODSetting	= 0;
			}
			Instance->SpriteTemplate->GetClosestLODLevels(LODSetting, HighLODLevel, LowLODLevel);
			if ((HighLODLevel != NULL) && (LowLODLevel != NULL) &&
			   (HighLODLevel->bEnabled || LowLODLevel->bEnabled))
			{
				// Determine the multiplier
				FLOAT	Multiplier	= 1.0f;
				INT		Diff		= LowLODLevel->LevelSetting - HighLODLevel->LevelSetting;
				// The Diff == 0 case is when there is an exact match to the current setting.
				if (Diff > 0)
				{
					Multiplier	= ((FLOAT)(LowLODLevel->LevelSetting - LODSetting) / (FLOAT)(Diff));
				}
				Instance->TickEditor(HighLODLevel, LowLODLevel, Multiplier, DeltaTime, bSuppressSpawning);
			}
		}
	}

	// Indicate that we have been ticked since being attached.
	bJustAttached = FALSE;

	// If component has just totally finished, call script event.
	UBOOL bIsCompleted = HasCompleted(); 
	if (bIsCompleted && !bWasCompleted)
	{
		if ( DELEGATE_IS_SET(OnSystemFinished) )
		{
			delegateOnSystemFinished(this);
		}

		if (bIsCachedInPool)
		{
			SetHiddenGame(TRUE);
		}
		else
		{
			// When system is done - destroy all subemitters etc. We don't need them any more.
			ResetParticles();
		}
	}
	bWasCompleted = bIsCompleted;

	// Update bounding box.
	if (!bIsCompleted && !Template->bUseFixedRelativeBoundingBox)
	{
		// Compute the new system bounding box.
		FBox BoundingBox;
		BoundingBox.Init();
	
		// Calculate combined bounding box by combining them from emitter instances.
		BoundingBox += LocalToWorld.GetOrigin();
		for (INT i=0; i<EmitterInstances.Num(); i++)
		{
			FParticleEmitterInstance* Instance = EmitterInstances(i);
			if (Instance && Instance->SpriteTemplate)
			{
				UParticleLODLevel* LODLevel = Instance->SpriteTemplate->GetCurrentLODLevel(Instance);
	 			if (LODLevel && LODLevel->bEnabled)
				{
					BoundingBox += Instance->GetBoundingBox();
				}
			}
		}

		// Only update the primitive's bounding box in the octree if the system bounding box has gotten larger.
		if(!Bounds.GetBox().IsInside(BoundingBox.Min) || !Bounds.GetBox().IsInside(BoundingBox.Max))
		{
			ConditionalUpdateTransform();
		}
	}


	PartSysVelocity = (LocalToWorld.GetOrigin() - OldPosition) / DeltaTime;
	OldPosition = LocalToWorld.GetOrigin();

	// Do this here to allow for Cascade-caused spikes in PeakCounts
	if (Template->bShouldResetPeakCounts == TRUE)
	{
		for (INT i = 0; i < Template->Emitters.Num(); i++)
		{
			UParticleEmitter* Emitter = Template->Emitters(i);
			for (INT LODIndex = 0; LODIndex < Emitter->LODLevels.Num(); LODIndex++)
			{
				UParticleLODLevel* LODLevel = Emitter->LODLevels(LODIndex);
	 			if (LODLevel && LODLevel->bEnabled)
				{
					LODLevel->PeakActiveParticles = 0;
				}
			}
		}
		Template->CalculateMaxActiveParticleCounts();
		Template->bShouldResetPeakCounts = FALSE;
	}

	GParticleDataManager.AddParticleSystemComponent(this);
}

// If particles have not already been initialised (ie. EmitterInstances created) do it now.
void UParticleSystemComponent::InitParticles()
{
	if (GIsUCC == TRUE)
	{
		// Don't bother creating instances - they won't be used...
		return;
	}

	if (IsTemplate() == TRUE)
	{
		return;
	}

	if (Template != NULL)
	{
		WarmupTime = Template->WarmupTime;

		// If nothing is initialized, create EmitterInstances here.
		if (EmitterInstances.Num() == 0)
		{
			const UBOOL bShowInEditor				= !HiddenEditor && (!Owner || !Owner->bHiddenEd);
			const UBOOL bShowInGame					= !HiddenGame && (!Owner || !Owner->bHidden || bCastHiddenShadow);
			const UBOOL bDetailModeAllowsRendering	= DetailMode <= GSystemSettings.DetailMode;
			if ( bDetailModeAllowsRendering && ((GIsGame && bShowInGame) || (!GIsGame && bShowInEditor)) )
			{
				EmitterInstances.Empty(Template->Emitters.Num());
				for (INT Idx = 0; Idx < Template->Emitters.Num(); Idx++)
				{
					UParticleEmitter* Emitter = Template->Emitters(Idx);
					if (Emitter)
					{
						EmitterInstances.AddItem(Emitter->CreateInstance(this));
					}
					else
					{
						INT NewIndex = EmitterInstances.Add();
						EmitterInstances(NewIndex) = NULL;
					}
				}
				bWasCompleted = FALSE;
			}
		}
		else
		{
			// create new instances as needed
			while (EmitterInstances.Num() < Template->Emitters.Num())
			{
				INT					Index	= EmitterInstances.Num();
				UParticleEmitter*	Emitter	= Template->Emitters(Index);
				if (Emitter)
				{
					FParticleEmitterInstance* Instance = Emitter->CreateInstance(this);
					EmitterInstances.AddItem(Instance);
					if (Instance)
					{
						Instance->InitParameters(Emitter, this);
					}
				}
				else
				{
					INT NewIndex = EmitterInstances.Add();
					EmitterInstances(NewIndex) = NULL;
				}
			}

			INT PreferredLODLevel = LODLevel;
			// re-initialize the instances
			for (INT Idx = 0; Idx < EmitterInstances.Num(); Idx++)
			{
				FParticleEmitterInstance* Instance = EmitterInstances(Idx);
				if (Instance) // @FIXME - prevent crash, but is there a logic problem?
				{
					UParticleEmitter* Emitter = NULL;
					
					if (Idx < Template->Emitters.Num())
					{
						Emitter = Template->Emitters(Idx);
					}
					if (Emitter)
					{
						Instance->InitParameters(Emitter, this, FALSE);
						Instance->Init();
						if (PreferredLODLevel >= Emitter->LODLevels.Num())
						{
							PreferredLODLevel = Emitter->LODLevels.Num() - 1;
						}
					}
					else
					{
						// Get rid of the 'phantom' instance
						Instance->RemovedFromScene();
						delete Instance;
						EmitterInstances(Idx) = NULL;
					}
				}
				else
				{
					UParticleEmitter*	Emitter	= Template->Emitters(Idx);
					if (Emitter)
					{
						Instance = Emitter->CreateInstance(this);
						EmitterInstances(Idx) = Instance;
						Instance->InitParameters(Emitter, this, FALSE);
						Instance->Init();
						if (PreferredLODLevel >= Emitter->LODLevels.Num())
						{
							PreferredLODLevel = Emitter->LODLevels.Num() - 1;
						}
					}
					else
					{
						EmitterInstances(Idx) = NULL;
					}
				}
			}

			if (PreferredLODLevel != LODLevel)
			{
				// This should never be higher...
				check(PreferredLODLevel < LODLevel);
				LODLevel = PreferredLODLevel;
			}

			for (INT Idx = 0; Idx < EmitterInstances.Num(); Idx++)
			{
				FParticleEmitterInstance* Instance = EmitterInstances(Idx);
				// set the LOD levels here
				if (Instance)
				{
					Instance->CurrentLODLevelIndex	= LODLevel;
					Instance->CurrentLODLevel		= Instance->SpriteTemplate->LODLevels(Instance->CurrentLODLevelIndex);
				}
			}
		}
	}
}

void UParticleSystemComponent::ResetParticles(UBOOL bEmptyInstances)
{
	// Remove instances from scene.
	for( INT InstanceIndex=0; InstanceIndex<EmitterInstances.Num(); InstanceIndex++ )
	{
		FParticleEmitterInstance* EmitterInstance = EmitterInstances(InstanceIndex);
		if( EmitterInstance )
		{
			EmitterInstance->RemovedFromScene();
			EmitterInstance->SpriteTemplate	= NULL;
			EmitterInstance->Component	= NULL;
		}
	}

	// Set the system as deactive
	bIsActive	= FALSE;

	// Remove instances if we're not running gameplay.
	if (!GIsGame || bEmptyInstances)
	{
		for (INT EmitterIndex = 0; EmitterIndex < EmitterInstances.Num(); EmitterIndex++)
		{
			FParticleEmitterInstance* EmitInst = EmitterInstances(EmitterIndex);
			if (EmitInst)
			{
				delete EmitInst;
				EmitterInstances(EmitterIndex) = NULL;
			}
		}
		EmitterInstances.Empty();
		SMComponents.Empty();
		SMMaterialInterfaces.Empty();
	}
}

void UParticleSystemComponent::ResetBurstLists()
{
	for (INT i=0; i<EmitterInstances.Num(); i++)
	{
		if (EmitterInstances(i))
		{
			EmitterInstances(i)->ResetBurstList();
		}
	}
}

void UParticleSystemComponent::SetTemplate(class UParticleSystem* NewTemplate)
{
	if( GIsAllowingParticles || GIsEditor ) 
	{
		bIsViewRelevanceDirty = TRUE;

		UBOOL bIsTemplate = IsTemplate();
		// duplicated in ActivateSystem
		// check to see if we need to update the component
		if (bIsTemplate == FALSE && NewTemplate )
		{
			if (Owner != NULL)
			{
				UpdateComponent(GWorld->Scene,Owner,Owner->LocalToWorld());
			}
		}
		bWasCompleted = FALSE;
		// remember if we were active and therefore should restart after setting up the new template
		UBOOL bWasActive = bIsActive; 
		UBOOL bResetInstances = FALSE;
		if (NewTemplate != Template)
		{
			bResetInstances = TRUE;
		}
		if (bIsTemplate == FALSE)
		{
			ResetParticles(bResetInstances);
		}

		Template = NewTemplate;
		if (Template)
		{
			WarmupTime = Template->WarmupTime;
			bAcceptsLights = Template->bLit;
		}
		else
		{
			WarmupTime = 0.0f;
			bAcceptsLights = FALSE;
		}

		if( NewTemplate )
		{
			if ((bAutoActivate || bWasActive) && (bIsTemplate == FALSE))
			{
				ActivateSystem();
			}
			else
			{
				InitializeSystem();
			}
			if ((SceneInfo == NULL) || bResetInstances)
			{
				BeginDeferredReattach();
			}
		}
	}
	else
	{
		Template = NULL;
	}
}

void UParticleSystemComponent::ActivateSystem()
{
	if (IsTemplate() == TRUE)
	{
		return;
	}

	if( GIsAllowingParticles )
	{
		// Stop suppressing particle spawning.
		bSuppressSpawning = FALSE;
		
		// Set the system as active
		UBOOL bNeedToUpdateTransform = bWasDeactivated;
		bWasCompleted = FALSE;
		bWasDeactivated = FALSE;
		bIsActive = TRUE;

		if (bIsViewRelevanceDirty == TRUE)
		{
			CacheViewRelevanceFlags(NULL);
		}

		if (SceneInfo == NULL)
		{
			BeginDeferredUpdateTransform();
		}

		// if no instances, or recycling
		if (EmitterInstances.Num() == 0 || GIsGame)
		{
			InitializeSystem();
		}
		else
		{
			// If currently running, re-activating rewinds the emitter to the start. Existing particles should stick around.
			for (INT i=0; i<EmitterInstances.Num(); i++)
			{
				if (EmitterInstances(i))
				{
					EmitterInstances(i)->Rewind();
				}
			}
		}

		// duplicated in SetTemplate
		// check to see if we need to update the component
		if (Owner != NULL)
		{
			if( bNeedToUpdateTransform )
			{
				DirtyTransform();
			}
			UpdateComponent(GWorld->Scene,Owner,Owner->LocalToWorld());
		}
		else if (bNeedToUpdateTransform)
		{
			ConditionalUpdateTransform();
		}

		if (WarmupTime != 0.0f)
		{
			UBOOL bSaveSkipUpdate = bSkipUpdateDynamicDataDuringTick;
			bSkipUpdateDynamicDataDuringTick = TRUE;
			bWarmingUp = TRUE;
			ResetBurstLists();

			FLOAT WarmupElapsed = 0.f;
			FLOAT WarmupTimestep = 0.032f;

			while (WarmupElapsed < WarmupTime)
			{
				Tick(WarmupTimestep);
				WarmupElapsed += WarmupTimestep;
			}

			bWarmingUp = FALSE;
			WarmupTime = 0.0f;
			bSkipUpdateDynamicDataDuringTick = bSaveSkipUpdate;
		}
		AccumTickTime = 0.0;
	}

	GParticleDataManager.AddParticleSystemComponent(this);

	LastRenderTime = GWorld->GetTimeSeconds();
}

/**
 *	DeactivateSystem
 *	Called to deactivate the particle system.
 *
 *	@param	bResetParticles		If TRUE, call ResetParticles on each emitter.
 */
void UParticleSystemComponent::DeactivateSystem()
{
	if (IsTemplate() == TRUE)
	{
		return;
	}

	bSuppressSpawning = TRUE;
	bWasDeactivated = TRUE;

	for (INT i = 0; i < EmitterInstances.Num(); i++)
	{
		FParticleEmitterInstance*	Instance = EmitterInstances(i);
		if (Instance)
		{
			if (Instance->KillOnDeactivate)
			{
				//debugf(TEXT("%s killed on deactivate"),EmitterInstances(i)->GetName());
				Instance->RemovedFromScene();
				delete Instance;
				EmitterInstances(i) = NULL;
			}
		}
	}

	LastRenderTime = GWorld->GetTimeSeconds();
}

/** calls ActivateSystem() or DeactivateSystem() only if the component is not already activated/deactivated
 * necessary because ActivateSystem() resets already active emitters so it shouldn't be called multiple times on looping effects
 * @param bNowActive - whether the system should be active
 */
void UParticleSystemComponent::SetActive(UBOOL bNowActive)
{
	if (bNowActive)
	{
		if (!bIsActive || (bWasDeactivated || bWasCompleted))
		{
			ActivateSystem();
		}
	}
	else if (bIsActive && !(bWasDeactivated || bWasCompleted))
	{
		DeactivateSystem();
	}
}

/** stops the emitter, detaches the component, and resets the component's properties to the values of its template */
void UParticleSystemComponent::ResetToDefaults()
{
	if (!IsTemplate())
	{
		// make sure we're fully stopped and detached
		DeactivateSystem();
		SetTemplate(NULL);
		DetachFromAny();

		UParticleSystemComponent* Default = GetArchetype<UParticleSystemComponent>();

		// copy all non-native, non-duplicatetransient, non-Component properties we have from all classes up to and including UActorComponent
		for (UProperty* Property = GetClass()->PropertyLink; Property != NULL; Property = Property->PropertyLinkNext)
		{
			if ( !(Property->PropertyFlags & CPF_Native) && !(Property->PropertyFlags & CPF_DuplicateTransient) && !(Property->PropertyFlags & CPF_Component) &&
				Property->GetOwnerClass()->IsChildOf(UActorComponent::StaticClass()) )
			{
				Property->CopyCompleteValue((BYTE*)this + Property->Offset, (BYTE*)Default + Property->Offset, NULL, this);
			}
		}
	}
}

void UParticleSystemComponent::UpdateInstances()
{
	ResetParticles();

	InitializeSystem();
	if (bAutoActivate)
	{
		ActivateSystem();
	}

	if (Template && Template->bUseFixedRelativeBoundingBox)
	{
		ConditionalUpdateTransform();
	}
}

UBOOL UParticleSystemComponent::HasCompleted()
{
	UBOOL bHasCompleted = TRUE;

	for (INT i=0; i<EmitterInstances.Num(); i++)
	{
		FParticleEmitterInstance* Instance = EmitterInstances(i);
		
		if (Instance && Instance->CurrentLODLevel && Instance->CurrentLODLevel->bEnabled)
		{
			if (bWasDeactivated && bSuppressSpawning)
			{
				if (Instance->ActiveParticles != 0)
				{
					bHasCompleted = FALSE;
				}
			}
			else
			{
				if (Instance->HasCompleted())
				{
					if (Instance->bKillOnCompleted)
					{
						Instance->RemovedFromScene();
						delete Instance;
						EmitterInstances(i) = NULL;
					}
				}
				else
				{
					bHasCompleted = FALSE;
				}
			}
		}
	}

	return bHasCompleted;
}

void UParticleSystemComponent::InitializeSystem()
{
	if( GIsAllowingParticles )
	{
		if (IsTemplate() == TRUE)
		{
			return;
		}
		// Cache the view relevance
		CacheViewRelevanceFlags();

		// Allocate the emitter instances and particle data
		InitParticles();

		if (IsAttached())
		{
			AccumTickTime = 0.0;
			if ((bIsActive == FALSE) && (bAutoActivate == TRUE) && (bWasDeactivated == FALSE))
			{
				SetActive(TRUE);
			}
		}
	}
}

/**
 *	Cache the view-relevance for each emitter at each LOD level.
 *
 *	@param	NewTemplate		The UParticleSystem* to use as the template.
 *							If NULL, use the currently set template.
 */
void UParticleSystemComponent::CacheViewRelevanceFlags(class UParticleSystem* NewTemplate)
{
	if (NewTemplate && (NewTemplate != Template))
	{
		bIsViewRelevanceDirty = TRUE;
	}

	if (bIsViewRelevanceDirty)
	{
		UParticleSystem* TemplateToCache = Template;
		if (NewTemplate)
		{
			TemplateToCache = NewTemplate;
		}

		if (TemplateToCache)
		{
			for (INT EmitterIndex = 0; EmitterIndex < TemplateToCache->Emitters.Num(); EmitterIndex++)
			{
				UParticleSpriteEmitter* Emitter = Cast<UParticleSpriteEmitter>(TemplateToCache->Emitters(EmitterIndex));
				FParticleEmitterInstance* EmitterInst = NULL;
				if (EmitterIndex < EmitterInstances.Num())
				{
					EmitterInst = EmitterInstances(EmitterIndex);
				}

				for (INT LODIndex = 0; LODIndex < Emitter->LODLevels.Num(); LODIndex++)
				{
					UParticleLODLevel* LODLevel = Emitter->LODLevels(LODIndex);

					// Prime the array
					// This code assumes that the particle system emitters all have the same number of LODLevels. 
					if (LODIndex >= CachedViewRelevanceFlags.Num())
					{
						CachedViewRelevanceFlags.AddZeroed(1);
					}
					FMaterialViewRelevance& LODViewRel = CachedViewRelevanceFlags(LODIndex);
					check(LODLevel->RequiredModule);

					FMaterialViewRelevance LocalMaterialViewRelevance;
					if (LODLevel->RequiredModule->bEnabled == TRUE)
					{
						UMaterialInterface* MaterialInst = LODLevel->RequiredModule->Material;
						check(Emitter->LODLevels.Num() >= 1);
						UParticleLODLevel* ZeroLODLevel = Emitter->LODLevels(0);
						if (ZeroLODLevel && ZeroLODLevel->TypeDataModule)
						{
							UParticleModuleTypeDataMesh* MeshTD = Cast<UParticleModuleTypeDataMesh>(ZeroLODLevel->TypeDataModule);
							if (MeshTD && (MeshTD->bOverrideMaterial == FALSE) && MeshTD->Mesh)
							{
								if (MeshTD->Mesh->LODModels(0).Elements.Num())
								{
									FStaticMeshElement&	Element = MeshTD->Mesh->LODModels(0).Elements(0);
									if (Element.Material)
									{
										MaterialInst = Element.Material;
									}
								}
							}
						}

						if (EmitterInst)
						{
							if (EmitterInst->CurrentMaterial)
							{
								MaterialInst = EmitterInst->CurrentMaterial;
							}
						}

						if(MaterialInst)
						{
							LODViewRel |= MaterialInst->GetViewRelevance();
						}
					}
				}
			}
		}
		bIsViewRelevanceDirty = FALSE;
	}
}

/**
 * SetKillOnDeactivate is used to set the KillOnDeactivate flag. If true, when
 * the particle system is deactivated, it will immediately kill the emitter
 * instance. If false, the emitter instance live particles will complete their
 * lifetime.
 *
 * Set this to true for cached ParticleSystems
 *
 *	@param	EmitterIndex		The index of the emitter to set it on
 *	@param	bKill				value to set KillOnDeactivate to
 */
void UParticleSystemComponent::SetKillOnDeactivate(INT EmitterIndex,UBOOL bKill)
{
	if (EmitterInstances.Num() == 0)
	{
		return;
	}

	if ((EmitterIndex >= 0) && EmitterIndex < EmitterInstances.Num())
	{
		FParticleEmitterInstance* EmitterInst = EmitterInstances(EmitterIndex);
		if (EmitterInst)
		{
			EmitterInst->SetKillOnDeactivate(bKill);
		}
	}
}

/**
 * SetKillOnDeactivate is used to set the KillOnCompleted( flag. If true, when
 * the particle system is completed, it will immediately kill the emitter
 * instance.
 *
 * Set this to true for cached ParticleSystems
 *
 *	@param	EmitterIndex		The index of the emitter to set it on
 *	@param	bKill				The value to set it to
 **/
void UParticleSystemComponent::SetKillOnCompleted(INT EmitterIndex,UBOOL bKill)
{
	if (EmitterInstances.Num() == 0)
	{
		return;
	}

	if ((EmitterIndex >= 0) && EmitterIndex < EmitterInstances.Num())
	{
		FParticleEmitterInstance* EmitterInst = EmitterInstances(EmitterIndex);
		if (EmitterInst)
		{
			EmitterInst->SetKillOnCompleted(bKill);
		}
	}
}

/**
 * Rewind emitter instances.
 **/
void UParticleSystemComponent::RewindEmitterInstance(INT EmitterIndex)
{
	if (EmitterInstances.Num() == 0)
	{
		return;
	}

	if ((EmitterIndex >= 0) && EmitterIndex < EmitterInstances.Num())
	{
		FParticleEmitterInstance* EmitterInst = EmitterInstances(EmitterIndex);
		if (EmitterInst)
		{
			EmitterInst->Rewind();
		}
	}
}

void UParticleSystemComponent::RewindEmitterInstances()
{
	for (INT EmitterIndex = 0; EmitterIndex < EmitterInstances.Num(); EmitterIndex++)
	{
		FParticleEmitterInstance* EmitterInst = EmitterInstances(EmitterIndex);
		if (EmitterInst)
		{
			EmitterInst->Rewind();
		}
	}
}

/**
 *	Set the beam type
 *
 *	@param	EmitterIndex		The index of the emitter to set it on
 *	@param	NewMethod			The new method/type of beam to generate
 */
void UParticleSystemComponent::SetBeamType(INT EmitterIndex,INT NewMethod)
{
	if (EmitterInstances.Num() == 0)
	{
		return;
	}

	if ((EmitterIndex >= 0) && EmitterIndex < EmitterInstances.Num())
	{
		FParticleEmitterInstance* EmitterInst = EmitterInstances(EmitterIndex);
		if (EmitterInst)
		{
			FParticleBeam2EmitterInstance* BeamInst = CastEmitterInstance<FParticleBeam2EmitterInstance>(EmitterInst);
			if (BeamInst)
			{
				BeamInst->SetBeamType(NewMethod);
			}
		}
	}
}

/**
 *	Set the beam tessellation factor
 *
 *	@param	EmitterIndex		The index of the emitter to set it on
 *	@param	NewFactor			The value to set it to
 */
void UParticleSystemComponent::SetBeamTessellationFactor(INT EmitterIndex,FLOAT NewFactor)
{
	if (EmitterInstances.Num() == 0)
	{
		return;
	}

	if ((EmitterIndex >= 0) && EmitterIndex < EmitterInstances.Num())
	{
		FParticleEmitterInstance* EmitterInst = EmitterInstances(EmitterIndex);
		if (EmitterInst)
		{
			FParticleBeam2EmitterInstance* BeamInst = CastEmitterInstance<FParticleBeam2EmitterInstance>(EmitterInst);
			if (BeamInst)
			{
				BeamInst->SetTessellationFactor(NewFactor);
			}
		}
	}
}

/**
 *	Set the beam end point
 *
 *	@param	EmitterIndex		The index of the emitter to set it on
 *	@param	NewEndPoint			The value to set it to
 */
void UParticleSystemComponent::SetBeamEndPoint(INT EmitterIndex,FVector NewEndPoint)
{
	if (EmitterInstances.Num() == 0)
	{
		return;
	}

	if ((EmitterIndex >= 0) && EmitterIndex < EmitterInstances.Num())
	{
		FParticleEmitterInstance* EmitterInst = EmitterInstances(EmitterIndex);
		if (EmitterInst)
		{
			FParticleBeam2EmitterInstance* BeamInst = CastEmitterInstance<FParticleBeam2EmitterInstance>(EmitterInst);
			if (BeamInst)
			{
				BeamInst->SetEndPoint(NewEndPoint);
			}
		}
	}
}

/**
 *	Set the beam distance
 *
 *	@param	EmitterIndex		The index of the emitter to set it on
 *	@param	Distance			The value to set it to
 */
void UParticleSystemComponent::SetBeamDistance(INT EmitterIndex,FLOAT Distance)
{
	if (EmitterInstances.Num() == 0)
	{
		return;
	}

	if ((EmitterIndex >= 0) && EmitterIndex < EmitterInstances.Num())
	{
		FParticleEmitterInstance* EmitterInst = EmitterInstances(EmitterIndex);
		if (EmitterInst)
		{
			FParticleBeam2EmitterInstance* BeamInst = CastEmitterInstance<FParticleBeam2EmitterInstance>(EmitterInst);
			if (BeamInst)
			{
				BeamInst->SetDistance(Distance);
			}
		}
	}
}

/**
 *	Set the beam source point
 *
 *	@param	EmitterIndex		The index of the emitter to set it on
 *	@param	NewSourcePoint		The value to set it to
 *	@param	SourceIndex			Which beam within the emitter to set it on
 */
void UParticleSystemComponent::SetBeamSourcePoint(INT EmitterIndex,FVector NewSourcePoint,INT SourceIndex)
{
	if (EmitterInstances.Num() == 0)
	{
		return;
	}

	if ((EmitterIndex >= 0) && EmitterIndex < EmitterInstances.Num())
	{
		FParticleEmitterInstance* EmitterInst = EmitterInstances(EmitterIndex);
		if (EmitterInst)
		{
			FParticleBeam2EmitterInstance* BeamInst = CastEmitterInstance<FParticleBeam2EmitterInstance>(EmitterInst);
			if (BeamInst)
			{
				BeamInst->SetSourcePoint(NewSourcePoint, SourceIndex);
			}
		}
	}
}

/**
 *	Set the beam source tangent
 *
 *	@param	EmitterIndex		The index of the emitter to set it on
 *	@param	NewTangentPoint		The value to set it to
 *	@param	SourceIndex			Which beam within the emitter to set it on
 */
void UParticleSystemComponent::SetBeamSourceTangent(INT EmitterIndex,FVector NewTangentPoint,INT SourceIndex)
{
	if (EmitterInstances.Num() == 0)
	{
		return;
	}

	if ((EmitterIndex >= 0) && EmitterIndex < EmitterInstances.Num())
	{
		FParticleEmitterInstance* EmitterInst = EmitterInstances(EmitterIndex);
		if (EmitterInst)
		{
			FParticleBeam2EmitterInstance* BeamInst = CastEmitterInstance<FParticleBeam2EmitterInstance>(EmitterInst);
			if (BeamInst)
			{
				BeamInst->SetSourceTangent(NewTangentPoint, SourceIndex);
			}
		}
	}
}

/**
 *	Set the beam source strength
 *
 *	@param	EmitterIndex		The index of the emitter to set it on
 *	@param	NewSourceStrength	The value to set it to
 *	@param	SourceIndex			Which beam within the emitter to set it on
 */
void UParticleSystemComponent::SetBeamSourceStrength(INT EmitterIndex,FLOAT NewSourceStrength,INT SourceIndex)
{
	if (EmitterInstances.Num() == 0)
	{
		return;
	}

	if ((EmitterIndex >= 0) && EmitterIndex < EmitterInstances.Num())
	{
		FParticleEmitterInstance* EmitterInst = EmitterInstances(EmitterIndex);
		if (EmitterInst)
		{
			FParticleBeam2EmitterInstance* BeamInst = CastEmitterInstance<FParticleBeam2EmitterInstance>(EmitterInst);
			if (BeamInst)
			{
				BeamInst->SetSourceStrength(NewSourceStrength, SourceIndex);
			}
		}
	}
}

/**
 *	Set the beam target point
 *
 *	@param	EmitterIndex		The index of the emitter to set it on
 *	@param	NewTargetPoint		The value to set it to
 *	@param	TargetIndex			Which beam within the emitter to set it on
 */
void UParticleSystemComponent::SetBeamTargetPoint(INT EmitterIndex,FVector NewTargetPoint,INT TargetIndex)
{
	if (EmitterInstances.Num() == 0)
	{
		return;
	}

	if ((EmitterIndex >= 0) && EmitterIndex < EmitterInstances.Num())
	{
		FParticleEmitterInstance* EmitterInst = EmitterInstances(EmitterIndex);
		if (EmitterInst)
		{
			FParticleBeam2EmitterInstance* BeamInst = CastEmitterInstance<FParticleBeam2EmitterInstance>(EmitterInst);
			if (BeamInst)
			{
				BeamInst->SetTargetPoint(NewTargetPoint, TargetIndex);
			}
		}
	}
}

/**
 *	Set the beam target tangent
 *
 *	@param	EmitterIndex		The index of the emitter to set it on
 *	@param	NewTangentPoint		The value to set it to
 *	@param	TargetIndex			Which beam within the emitter to set it on
 */
void UParticleSystemComponent::SetBeamTargetTangent(INT EmitterIndex,FVector NewTangentPoint,INT TargetIndex)
{
	if (EmitterInstances.Num() == 0)
	{
		return;
	}

	if ((EmitterIndex >= 0) && EmitterIndex < EmitterInstances.Num())
	{
		FParticleEmitterInstance* EmitterInst = EmitterInstances(EmitterIndex);
		if (EmitterInst)
		{
			FParticleBeam2EmitterInstance* BeamInst = CastEmitterInstance<FParticleBeam2EmitterInstance>(EmitterInst);
			if (BeamInst)
			{
				BeamInst->SetTargetTangent(NewTangentPoint, TargetIndex);
			}
		}
	}
}

/**
 *	Set the beam target strength
 *
 *	@param	EmitterIndex		The index of the emitter to set it on
 *	@param	NewTargetStrength	The value to set it to
 *	@param	TargetIndex			Which beam within the emitter to set it on
 */
void UParticleSystemComponent::SetBeamTargetStrength(INT EmitterIndex,FLOAT NewTargetStrength,INT TargetIndex)
{
	if (EmitterInstances.Num() == 0)
	{
		return;
	}

	if ((EmitterIndex >= 0) && EmitterIndex < EmitterInstances.Num())
	{
		FParticleEmitterInstance* EmitterInst = EmitterInstances(EmitterIndex);
		if (EmitterInst)
		{
			FParticleBeam2EmitterInstance* BeamInst = CastEmitterInstance<FParticleBeam2EmitterInstance>(EmitterInst);
			if (BeamInst)
			{
				BeamInst->SetTargetStrength(NewTargetStrength, TargetIndex);
			}
		}
	}
}

/** Set the LOD level of the particle system									*/
void UParticleSystemComponent::SetLODLevel(int InLODLevel)
{
	if (Template == NULL)
	{
		return;
	}

	LODLevel = Clamp(InLODLevel + GSystemSettings.ParticleLODBias,0,Template->GetLODLevelCount()-1);

	for (INT i=0; i<EmitterInstances.Num(); i++)
	{
		FParticleEmitterInstance* Instance = EmitterInstances(i);
		if (Instance)
		{
			if (Instance->SpriteTemplate != NULL)
			{
				Instance->CurrentLODLevelIndex	= LODLevel;
				// check to make certain the data in the content actually represents what we are being asked to render
				if( Instance->SpriteTemplate->LODLevels.Num() > Instance->CurrentLODLevelIndex )
				{
					Instance->CurrentLODLevel		= Instance->SpriteTemplate->LODLevels(Instance->CurrentLODLevelIndex);
				}
				// set to the LOD which is guaranteed to exist
				else
				{
					Instance->CurrentLODLevelIndex = 0;
					Instance->CurrentLODLevel = Instance->SpriteTemplate->LODLevels(Instance->CurrentLODLevelIndex);
				}

				check(Instance->CurrentLODLevel);
				check(Instance->CurrentLODLevel->RequiredModule);
				Instance->bKillOnCompleted = Instance->CurrentLODLevel->RequiredModule->bKillOnCompleted;
				Instance->KillOnDeactivate = Instance->CurrentLODLevel->RequiredModule->bKillOnDeactivate;

				Instance->CurrentLODLevel->bEnabled = Instance->CurrentLODLevel->RequiredModule->bEnabled;
			}
			else
			{
				// This is a legitimate case when PSysComponents are cached...
				// However, with the addition of the bIsActive flag to that class, this should 
				// never be called when the component has not had it's instances initialized/activated.
#if defined(_PSYSCOMP_DEBUG_INVALID_EMITTER_INSTANCE_TEMPLATES_)
				// need better debugging here
				warnf(TEXT("Template of emitter instance %d (%s) a ParticleSystemComponent (%s) was NULL: %s" ), 
					i, *Instance->GetName(), *Template->GetName(), *this->GetFullName());
#endif	//#if defined(_PSYSCOMP_DEBUG_INVALID_EMITTER_INSTANCE_TEMPLATES_)
			}
		}
	}
}

/** Set the editor LOD level of the particle system								*/
void UParticleSystemComponent::SetEditorLODLevel(INT InLODLevel)
{
	if (InLODLevel < 0)
	{
		InLODLevel	= 0;
	}

	if (InLODLevel >= 100)
	{
		warnf(TEXT("Setting LOD level on %s to %d - max LOD 100."),
			*Template->GetName(), InLODLevel);
		EditorLODLevel	= 100;
	}
	else
	{
		EditorLODLevel	= InLODLevel;
	}
}

/** Get the LOD level of the particle system									*/
INT UParticleSystemComponent::GetLODLevel()
{
	return LODLevel;
}

/** Get the editor LOD level of the particle system								*/
INT UParticleSystemComponent::GetEditorLODLevel()
{
	return EditorLODLevel;
}

/**
 *	Determine the appropriate LOD level to utilize
 */
INT UParticleSystemComponent::DetermineLODLevel(const FSceneView* View)
{
	INT	LODIndex = -1;

	BYTE CheckLODMethod = PARTICLESYSTEMLODMETHOD_DirectSet;
	if (bOverrideLODMethod)
	{
		CheckLODMethod = LODMethod;
	}
	else
	{
		if (Template)
		{
			CheckLODMethod = Template->LODMethod;
		}
	}

	if (CheckLODMethod == PARTICLESYSTEMLODMETHOD_Automatic)
	{
		// Default to the highest LOD level
		LODIndex = 0;

		FVector	CameraPosition	= View->ViewOrigin;
		FVector	CompPosition	= LocalToWorld.GetOrigin();
		FVector	DistDiff		= CompPosition - CameraPosition;
		FLOAT	Distance		= DistDiff.Size();
		//debugf(TEXT("Perform LOD Determination Check - Dist = %8.5f, PSys %s"), Distance, *GetFullName());

		for (INT LODDistIndex = 1; LODDistIndex < Template->LODDistances.Num(); LODDistIndex++)
		{
			if (Template->LODDistances(LODDistIndex) > Distance)
			{
				break;
			}
			LODIndex = LODDistIndex;
		}
	}

	return LODIndex;
}

/** 
 *	Set a named float instance parameter on this ParticleSystemComponent. 
 *	Updates the parameter if it already exists, or creates a new entry if not. 
 */
void UParticleSystemComponent::SetFloatParameter(FName Name, FLOAT Param)
{
	if(Name == NAME_None)
	{
		return;
	}

	// First see if an entry for this name already exists
	for (INT i = 0; i < InstanceParameters.Num(); i++)
	{
		FParticleSysParam& P = InstanceParameters(i);
		if (P.Name == Name && P.ParamType == PSPT_Scalar)
		{
			P.Scalar = Param;
			return;
		}
	}

	// We didn't find one, so create a new one.
	INT NewParamIndex = InstanceParameters.AddZeroed();
	InstanceParameters(NewParamIndex).Name = Name;
	InstanceParameters(NewParamIndex).ParamType = PSPT_Scalar;
	InstanceParameters(NewParamIndex).Scalar = Param;
}

/** 
 *	Set a named vector instance parameter on this ParticleSystemComponent. 
 *	Updates the parameter if it already exists, or creates a new entry if not. 
 */
void UParticleSystemComponent::SetVectorParameter(FName Name, FVector Param)
{
	if(Name == NAME_None)
	{
		return;
	}

	// First see if an entry for this name already exists
	for (INT i = 0; i < InstanceParameters.Num(); i++)
	{
		FParticleSysParam& P = InstanceParameters(i);
		if (P.Name == Name && P.ParamType == PSPT_Vector)
		{
			P.Vector = Param;
			return;
		}
	}

	// We didn't find one, so create a new one.
	INT NewParamIndex = InstanceParameters.AddZeroed();
	InstanceParameters(NewParamIndex).Name = Name;
	InstanceParameters(NewParamIndex).ParamType = PSPT_Vector;
	InstanceParameters(NewParamIndex).Vector = Param;
}

/** 
 *	Set a named color instance parameter on this ParticleSystemComponent. 
 *	Updates the parameter if it already exists, or creates a new entry if not. 
 */
void UParticleSystemComponent::SetColorParameter(FName Name, FColor Param)
{
	if(Name == NAME_None)
	{
		return;
	}

	// First see if an entry for this name already exists
	for (INT i = 0; i < InstanceParameters.Num(); i++)
	{
		FParticleSysParam& P = InstanceParameters(i);
		if (P.Name == Name && P.ParamType == PSPT_Color)
		{
			P.Color = Param;
			return;
		}
	}

	// We didn't find one, so create a new one.
	INT NewParamIndex = InstanceParameters.AddZeroed();
	InstanceParameters(NewParamIndex).Name = Name;
	InstanceParameters(NewParamIndex).ParamType = PSPT_Color;
	InstanceParameters(NewParamIndex).Color = Param;
}

/** 
 *	Set a named actor instance parameter on this ParticleSystemComponent. 
 *	Updates the parameter if it already exists, or creates a new entry if not. 
 */
void UParticleSystemComponent::SetActorParameter(FName Name, AActor* Param)
{
	if(Name == NAME_None)
	{
		return;
	}

	// First see if an entry for this name already exists
	for (INT i = 0; i < InstanceParameters.Num(); i++)
	{
		FParticleSysParam& P = InstanceParameters(i);
		if (P.Name == Name && P.ParamType == PSPT_Actor)
		{
			P.Actor = Param;
			return;
		}
	}

	// We didn't find one, so create a new one.
	INT NewParamIndex = InstanceParameters.AddZeroed();
	InstanceParameters(NewParamIndex).Name = Name;
	InstanceParameters(NewParamIndex).ParamType = PSPT_Actor;
	InstanceParameters(NewParamIndex).Actor = Param;
}

/** 
 *	Set a named material instance parameter on this ParticleSystemComponent. 
 *	Updates the parameter if it already exists, or creates a new entry if not. 
 */
void UParticleSystemComponent::SetMaterialParameter(FName Name, UMaterialInterface* Param)
{
	if(Name == NAME_None)
	{
		return;
	}

	// First see if an entry for this name already exists
	for (INT i = 0; i < InstanceParameters.Num(); i++)
	{
		FParticleSysParam& P = InstanceParameters(i);
		if (P.Name == Name && P.ParamType == PSPT_Material)
		{
			P.Material = Param;
			return;
		}
	}

	// We didn't find one, so create a new one.
	INT NewParamIndex = InstanceParameters.AddZeroed();
	InstanceParameters(NewParamIndex).Name = Name;
	InstanceParameters(NewParamIndex).ParamType = PSPT_Material;
	InstanceParameters(NewParamIndex).Material = Param;
}

/** 
 *	Try and find an Instance Parameter with the given name in this ParticleSystemComponent, and if we find it return the float value. 
 *	If we fail to find the parameter, return FALSE.
 */
UBOOL UParticleSystemComponent::GetFloatParameter(const FName InName, FLOAT& OutFloat)
{
	// Always fail if we pass in no name.
	if(InName == NAME_None)
	{
		return FALSE;
	}

	for (INT i = 0; i < InstanceParameters.Num(); i++)
	{
		FParticleSysParam& Param = InstanceParameters(i);
		if (Param.Name == InName && Param.ParamType == PSPT_Scalar)
		{
			OutFloat = Param.Scalar;
			return TRUE;
		}
	}

	return FALSE;
}

/** 
 *	Try and find an Instance Parameter with the given name in this ParticleSystemComponent, and if we find it return the vector value. 
 *	If we fail to find the parameter, return FALSE.
 */
UBOOL UParticleSystemComponent::GetVectorParameter(const FName InName, FVector& OutVector)
{
	// Always fail if we pass in no name.
	if(InName == NAME_None)
	{
		return FALSE;
	}

	for (INT i = 0; i < InstanceParameters.Num(); i++)
	{
		FParticleSysParam& Param = InstanceParameters(i);
		if (Param.Name == InName && Param.ParamType == PSPT_Vector)
		{
			OutVector = Param.Vector;
			return TRUE;
		}
	}

	return FALSE;
}

/** 
 *	Try and find an Instance Parameter with the given name in this ParticleSystemComponent, and if we find it return the material value. 
 *	If we fail to find the parameter, return FALSE.
 */
UBOOL UParticleSystemComponent::GetMaterialParameter(const FName InName, UMaterialInterface*& OutMaterial)
{
	// Always fail if we pass in no name.
	if (InName == NAME_None)
	{
		return FALSE;
	}

	for (INT i = 0; i < InstanceParameters.Num(); i++)
	{
		FParticleSysParam& Param = InstanceParameters(i);
		if (Param.Name == InName && Param.ParamType == PSPT_Material)
		{
			OutMaterial = Param.Material;
			return TRUE;
		}
	}

	return FALSE;
}

/** clears the specified parameter, returning it to the default value set in the template
 * @param ParameterName name of parameter to remove
 * @param ParameterType type of parameter to remove; if omitted or PSPT_None is specified, all parameters with the given name are removed
 */
void UParticleSystemComponent::ClearParameter(FName ParameterName, BYTE ParameterType)
{
	for (INT i = 0; i < InstanceParameters.Num(); i++)
	{
		if (InstanceParameters(i).Name == ParameterName && (ParameterType == PSPT_None || InstanceParameters(i).ParamType == ParameterType))
		{
			InstanceParameters.Remove(i--);
		}
	}
}

/** 
 *	Auto-populate the instance parameters based on contained modules.
 */
void UParticleSystemComponent::AutoPopulateInstanceProperties()
{
	if (Template)
	{
		for (INT EmitterIndex = 0; EmitterIndex < Template->Emitters.Num(); EmitterIndex++)
		{
			UParticleEmitter* Emitter = Template->Emitters(EmitterIndex);
			Emitter->AutoPopulateInstanceProperties(this);
		}
	}
}

void UParticleSystemComponent::KillParticlesForced()
{
	for (INT EmitterIndex=0;EmitterIndex<EmitterInstances.Num();EmitterIndex++)
	{
		if (EmitterInstances(EmitterIndex))
		{
			EmitterInstances(EmitterIndex)->KillParticlesForced();
		}
	}
}

/**
 *	Function for setting the bSkipUpdateDynamicDataDuringTick flag.
 */
void UParticleSystemComponent::SetSkipUpdateDynamicDataDuringTick(UBOOL bInSkipUpdateDynamicDataDuringTick)
{
	bSkipUpdateDynamicDataDuringTick = bInSkipUpdateDynamicDataDuringTick;
}

/**
 *	Function for retrieving the bSkipUpdateDynamicDataDuringTick flag.
 */
UBOOL UParticleSystemComponent::GetSkipUpdateDynamicDataDuringTick()
{
	return bSkipUpdateDynamicDataDuringTick;
}

IMPLEMENT_CLASS(UParticleEmitterInstance);
IMPLEMENT_CLASS(UParticleSpriteEmitterInstance);
IMPLEMENT_CLASS(UParticleSpriteSubUVEmitterInstance);
IMPLEMENT_CLASS(UParticleMeshEmitterInstance);
IMPLEMENT_CLASS(UParticleMeshNxFluidEmitterInstance);
IMPLEMENT_CLASS(UParticleTrailEmitterInstance);
IMPLEMENT_CLASS(UParticleBeamEmitterInstance);
IMPLEMENT_CLASS(UParticleSpriteNxFluidEmitterInstance);

