/*=============================================================================
	PhysXParticleSetMesh.cpp: PhysX Emitter Source.
	Copyright 2007-2008 AGEIA Technologies.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineParticleClasses.h"
#include "EnginePhysicsClasses.h"

#if WITH_NOVODEX && !NX_DISABLE_FLUIDS

#include "UnNovodexSupport.h"
#include "PhysXVerticalEmitter.h"
#include "PhysXParticleSystem.h"
#include "PhysXParticleSetMesh.h"

FPhysXParticleSetMesh::FPhysXParticleSetMesh(FPhysXMeshInstance& InMeshInstance) :
	FPhysXParticleSet(sizeof(PhysXRenderParticleMesh), InMeshInstance.PhysXTypeData.VerticalLod, InMeshInstance.PhysXTypeData.PhysXParSys),
	PhysXTypeData(InMeshInstance.PhysXTypeData),
	MeshInstance(InMeshInstance)
{
	check(InMeshInstance.PhysXTypeData.PhysXParSys);
}

FPhysXParticleSetMesh::~FPhysXParticleSetMesh()
{
}

void FPhysXParticleSetMesh::AsyncUpdate(FLOAT DeltaTime, UBOOL bProcessSimulationStep)
{
	FPhysXParticleSystem& PSys = GetPSys();
	FillInVertexBuffer(DeltaTime, PhysXTypeData.PhysXRotationMethod, PhysXTypeData.FluidRotationCoefficient);

	if(bProcessSimulationStep)
	{
		DeathRowManagment();
		AsyncParticleReduction(DeltaTime);
	}
}

void FPhysXParticleSetMesh::RemoveAllParticles()
{
	RemoveAllParticlesInternal();
	MeshInstance.ActiveParticles = 0;
}

void FPhysXParticleSetMesh::ApplyParticleModulesToSingleParticle(
	const PhysXRenderParticleMesh& NParticle,
	FParticleMeshPhysXEmitterInstance* SpawnInstance, 
	const NxVec3& Velocity,
	FLinearColor& Color,
	NxVec3& Size)
{
	UParticleLODLevel* LODLevel = SpawnInstance->SpriteTemplate->GetCurrentLODLevel(SpawnInstance);
	for( INT ModuleIndex = 0; ModuleIndex < LODLevel->UpdateModules.Num(); ModuleIndex++ )
	{
		UParticleModule* HighModule	= LODLevel->UpdateModules(ModuleIndex);
		if( !(HighModule && HighModule->bEnabled) )
			continue;

		// SizeMultiplyLife
		UParticleModuleSizeMultiplyLife* SizeMultiplyLifeModule = Cast<UParticleModuleSizeMultiplyLife>(HighModule);
		if( SizeMultiplyLifeModule )
		{
			FVector SizeScale = SizeMultiplyLifeModule->LifeMultiplier.GetValue(NParticle.RelativeTime, SpawnInstance->Component);

			if (SizeMultiplyLifeModule->MultiplyX)
				Size.x *= SizeScale.X;

			if (SizeMultiplyLifeModule->MultiplyY)
				Size.y *= SizeScale.Y;

			if (SizeMultiplyLifeModule->MultiplyZ)
				Size.z *= SizeScale.Z;

			continue;
		}

		// ColorOverLife
		UParticleModuleColorOverLife* ColorOverLifeModule = Cast<UParticleModuleColorOverLife>(HighModule);
		if( ColorOverLifeModule )
		{
			FVector ColorVec = ColorOverLifeModule->ColorOverLife.GetValue(NParticle.RelativeTime, SpawnInstance->Component);
			FLOAT	fAlpha = ColorOverLifeModule->AlphaOverLife.GetValue(NParticle.RelativeTime, SpawnInstance->Component);
			Color.R = ColorVec.X;
			Color.G = ColorVec.Y;
			Color.B = ColorVec.Z;
			Color.A = fAlpha;

			continue;
		}

		// SizeMultiplyVelocity
		UParticleModuleSizeMultiplyVelocity* SizeMultiplyVelocityModule = Cast<UParticleModuleSizeMultiplyVelocity>(HighModule);
		if( SizeMultiplyVelocityModule )
		{
			FVector SizeScale = SizeMultiplyVelocityModule->VelocityMultiplier.GetValue(NParticle.RelativeTime, SpawnInstance->Component) * Velocity.magnitude();
			if(SizeMultiplyVelocityModule->MultiplyX)
			{
				Size.x *= SizeScale.X;
			}
			if(SizeMultiplyVelocityModule->MultiplyY)
			{
				Size.y *= SizeScale.Y;
			}
			if(SizeMultiplyVelocityModule->MultiplyZ)
			{
				Size.z *= SizeScale.Z;
			}

			continue;
		}
	}

}

/**
Here we assume that the particles of the render instance emitter have been updated.
*/
void FPhysXParticleSetMesh::FillInVertexBuffer(FLOAT DeltaTime, BYTE FluidRotationMethod, FLOAT FluidRotationCoefficient)
{
	FPhysXParticleSystem& PSys = GetPSys();
	MeshInstance.ActiveParticles = GetNumRenderParticles();
	TmpRenderIndices.Empty();

	if(GetNumRenderParticles() == 0)
		return;

	FParticleMeshPhysXEmitterInstance* SpawnInstance = MeshInstance.GetSpawnInstance();
	if(!SpawnInstance)
		return;

	FDynamicMeshEmitterData::FParticleInstancedMeshInstance *InstanceWriteBuffer = MeshInstance.PrepareInstanceWriteBuffer(GetMaxRenderParticles());

	if(!InstanceWriteBuffer)
		return;

	PhysXParticle* SdkParticles = PSys.ParticlesSdk;
	PhysXParticleEx* SdkParticlesEx = PSys.ParticlesEx;
	
	FLOAT Temp = NxMath::clamp(PhysXTypeData.VerticalLod.RelativeFadeoutTime, 1.0f, 0.0f);
	FLOAT TimeUntilFadeout = 1.0f - Temp;

	const NxReal MaxRotPerStep = 2.5f;
	const NxReal Epsilon = 0.001f;
	
	// Retrieve global up axis.
	NxVec3 UpVector(0.0f, 0.0f, 1.0f);
	PSys.GetGravity(UpVector);
	UpVector = -UpVector;
	UpVector.normalize();

	for (INT i=0; i<GetNumRenderParticles(); i++)
	{
		PhysXRenderParticleMesh& NParticle = *(PhysXRenderParticleMesh*)GetRenderParticle(i);
		PhysXParticle& SdkParticle = SdkParticles[NParticle.ParticleIndex];

		NParticle.RelativeTime += NParticle.OneOverMaxLifetime * DeltaTime;
		if(NParticle.RelativeTime >= TimeUntilFadeout && (NParticle.Flags & PhysXRenderParticleMesh::PXRP_DeathQueue) == 0)
			TmpRenderIndices.Push(i);

		// Apply particle modules to update size and color
		NxVec3 Size = *reinterpret_cast<NxVec3*>(&NParticle.Size.X);
		FLinearColor Color(NParticle.Color);
		
		ApplyParticleModulesToSingleParticle(NParticle, SpawnInstance, SdkParticle.Vel, Color, Size);

		FVector	Pos = N2UPosition(SdkParticle.Pos);

		if( SpawnInstance->MeshRotationActive )
		{
			switch(FluidRotationMethod)
			{
				case PMRM_Velocity:
				{
					FVector NewDirection = N2UVectorCopy(SdkParticle.Vel);
					NewDirection.Normalize();
					FVector OldDirection(1.0f, 0.0f, 0.0f);
					NParticle.Rot = FQuatFindBetween(OldDirection, NewDirection);
					break;
				}

				case PMRM_Spherical:
				{
					NxVec3             vel  = SdkParticle.Vel;
					NxU32              id   = NParticle.Id;
					vel.z = 0; // Project onto xy plane.
					NxReal velmag = vel.magnitude();
					if(velmag > Epsilon)
					{
						NxVec3 avel;
						avel.cross(vel, UpVector);
						NxReal avelm = avel.normalize();
						if(avelm > Epsilon)
						{
							// Set magnitude to magnitude of linear motion (later maybe clamp)
							avel *= -velmag;

							NxReal w = velmag;
							NxReal v = (NxReal)DeltaTime*w*FluidRotationCoefficient;
							NxReal q = NxMath::cos(v);
							NxReal s = NxMath::sin(v)/w;

							NxQuat& Rot = *reinterpret_cast<NxQuat*>(&NParticle.Rot.X);
							Rot.multiply(NxQuat(avel*s,q), Rot);
							Rot.normalize();
						}
					}
				}
				break;

				case PMRM_Box:
				case PMRM_LongBox:
				case PMRM_FlatBox:
				{
					const NxVec3& Vel = *reinterpret_cast<NxVec3*>(&SdkParticle.Vel);
					NxQuat& Rot       = *reinterpret_cast<NxQuat*>(&NParticle.Rot.X);
					NxVec3& AngVel    = *reinterpret_cast<NxVec3*>(&NParticle.AngVel.X);

					const NxVec3 &Contact = PSys.ParticleContactsSdk[NParticle.ParticleIndex];

					NxReal VelMagSqr  = Vel.magnitudeSquared();
					NxReal LimitSqr   = VelMagSqr * FluidRotationCoefficient * FluidRotationCoefficient;

					NxVec3 PoseCorrection(0.0f);		// Rest pose correction.

					// Check bounce...
					if(Contact.magnitudeSquared() > Epsilon)
					{
						NxVec3 UpVector = Contact; // So we can rest on a ramp.
						UpVector.normalize();
						NxVec3 t = Vel - UpVector * UpVector.dot(Vel);
						AngVel = -t.cross(UpVector) * FluidRotationCoefficient;

						NxMat33 rot33;
						rot33.fromQuat(Rot);
						NxVec3  Up(0.0f);

						switch(FluidRotationMethod)
						{
							case PMRM_FlatBox:
							{
								Up = rot33.getColumn(2);
								if(Up.z < 0)
								{
									Up = -Up;
								}
								break;
							}
							default:
							{
								NxReal Best = 0;
								for(int j = (FluidRotationMethod == PMRM_LongBox ? 1 : 0); j < 3; j++)
								{
									NxVec3 tmp = rot33.getColumn(j);
									NxReal d   = tmp.dot(UpVector);
									if(d > Best)
									{
										Up   = tmp;
										Best = d;
									}
									if(-d > Best)
									{
										Up   = -tmp;
										Best = -d;
									}
								}
								break;
							}
						}

						PoseCorrection = Up.cross(UpVector);
						NxReal Mag = PoseCorrection.magnitude();
						NxReal MaxMag = 0.5f / (1.0f + NxMath::sqrt(LimitSqr));
						if(Mag > MaxMag)
						{
							PoseCorrection *= MaxMag / Mag;
						}
					}

					// Limit angular velocity.
					NxReal MagSqr = AngVel.magnitudeSquared();
					if(Contact.magnitudeSquared() > Epsilon && MagSqr > LimitSqr)
					{
						AngVel *= NxMath::sqrt(LimitSqr) / NxMath::sqrt(MagSqr);
					}
					
					// Integrate rotation.
					NxVec3 DeltaRot = AngVel * (NxReal)DeltaTime;
					
					// Apply combined rotation.
					NxVec3 Axis  = DeltaRot + PoseCorrection;
					NxReal Angle = Axis.normalize();
					if(Angle > Epsilon)
					{
						if(Angle > MaxRotPerStep)
						{
							Angle = MaxRotPerStep;
						}
						NxQuat TempRot;
						TempRot.fromAngleAxisFast(Angle, Axis);
						Rot = TempRot * Rot;
					}
				}
				break;
			}

			FQuatRotationTranslationMatrix kRotMat(NParticle.Rot, FVector::ZeroVector);

			InstanceWriteBuffer[i].Location = Pos;
			InstanceWriteBuffer[i].XAxis = FVector(Size.x * kRotMat.M[0][0], Size.x * kRotMat.M[0][1], Size.x * kRotMat.M[0][2]);
			InstanceWriteBuffer[i].YAxis = FVector(Size.y * kRotMat.M[1][0], Size.y * kRotMat.M[1][1], Size.y * kRotMat.M[1][2]);
			InstanceWriteBuffer[i].ZAxis = FVector(Size.z * kRotMat.M[2][0], Size.z * kRotMat.M[2][1], Size.z * kRotMat.M[2][2]);
			InstanceWriteBuffer[i].Color = Color;
		}
		else
		{
			InstanceWriteBuffer[i].Location = Pos;
			InstanceWriteBuffer[i].XAxis = FVector(Size.x, 0.0f, 0.0f);
			InstanceWriteBuffer[i].YAxis = FVector(0.0f, Size.y, 0.0f);
			InstanceWriteBuffer[i].ZAxis = FVector(0.0f, 0.0f, Size.z);
			InstanceWriteBuffer[i].Color = Color;
		}
	}
}

/*-----------------------------------------------------------------------------
	FPhysXMeshInstance implementation.
-----------------------------------------------------------------------------*/

FParticleMeshPhysXEmitterInstance* FPhysXMeshInstance::GetSpawnInstance()
{
	if(SpawnInstances.Num() == 0)
		return NULL;

	check(SpawnInstances(0));
	FParticleMeshPhysXEmitterInstance* SpawnInstance = SpawnInstances(0);
	return SpawnInstance;
}

FPhysXMeshInstance::FPhysXMeshInstance(class UParticleModuleTypeDataMeshPhysX &TypeData) :
	PhysXTypeData(TypeData),
	ActiveParticles(0),
	DynamicEmitter(NULL)
{
	PSet = new FPhysXParticleSetMesh(*this);
	check(PSet);
}

FPhysXMeshInstance::~FPhysXMeshInstance()
{
	PhysXTypeData.RenderInstance = NULL;
	check(PSet);

	if(PhysXTypeData.PhysXParSys && PhysXTypeData.PhysXParSys->PSys)
		PhysXTypeData.PhysXParSys->PSys->RemoveParticleSet(PSet);

	delete PSet;
	PSet = NULL;

	if( DynamicEmitter )
	{
		delete DynamicEmitter;
		DynamicEmitter = NULL;
	}
}

FDynamicMeshEmitterData::FParticleInstancedMeshInstance *FPhysXMeshInstance::PrepareInstanceWriteBuffer(INT CurrentMaxParticles)
{
	FDynamicMeshEmitterData::FParticleInstancedMeshInstance *InstanceWriteBuffer = NULL;
	AllocateDynamicEmitter(CurrentMaxParticles);

	if (!DynamicEmitter)
		return NULL;

	DynamicEmitter->PhysXParticleBuf = 
		(FDynamicMeshEmitterData::FParticleInstancedMeshInstance *)appMalloc(ActiveParticles * sizeof(FDynamicMeshEmitterData::FParticleInstancedMeshInstance));
	InstanceWriteBuffer = DynamicEmitter->PhysXParticleBuf;

	return InstanceWriteBuffer;
}

void FPhysXMeshInstance::AllocateDynamicEmitter(INT CurrentMaxParticles)
{
	// (JPB) The value of bSelected (selected in the editor) was provided by GetDynamicData(),
	// but since we are allocating the emitter earlier, we don't have this information.
	// For now, just set to FALSE.
	UBOOL bSelected = FALSE;

	if( DynamicEmitter )
	{
		delete DynamicEmitter;
		DynamicEmitter = NULL;
	}

	FParticleMeshPhysXEmitterInstance* SpawnInstance = GetSpawnInstance();
	check(SpawnInstance);

	// Make sure the template is valid
	if (!SpawnInstance->SpriteTemplate)
	{
		return;
	}

	// If the template is disabled, don't return data.
	UParticleLODLevel* LODLevel = SpawnInstance->SpriteTemplate->GetCurrentLODLevel(SpawnInstance);
	if ((LODLevel == NULL) || (LODLevel->bEnabled == FALSE))
	{
		return;
	}
	SpawnInstance->CurrentMaterial = LODLevel->RequiredModule->Material;

	if ((SpawnInstance->MeshComponentIndex == -1) || (SpawnInstance->MeshComponentIndex >= SpawnInstance->Component->SMComponents.Num()))
	{
		// Not initialized?
		return;
	}

	UStaticMeshComponent* MeshComponent = SpawnInstance->Component->SMComponents(SpawnInstance->MeshComponentIndex);
	if (MeshComponent == NULL)
	{
		// The mesh component has been GC'd?
		return;
	}

	// Allocate it for now, but we will want to change this to do some form
	// of caching
	if (ActiveParticles > 0)
	{
		// Get the material instance. If none is present, use the DefaultMaterial
		FMaterialRenderProxy* MatResource = NULL;
		if (LODLevel->TypeDataModule)
		{
			UParticleModuleTypeDataMesh* MeshTD = CastChecked<UParticleModuleTypeDataMesh>(LODLevel->TypeDataModule);
			if (MeshTD->bOverrideMaterial == TRUE)
			{
				if (SpawnInstance->CurrentMaterial)
				{
					MatResource = SpawnInstance->CurrentMaterial->GetRenderProxy(bSelected);
				}
			}
		}

		// Allocate the dynamic data
		FDynamicMeshEmitterData* NewEmitterData = ::new FDynamicMeshEmitterData(LODLevel->RequiredModule);

		NewEmitterData->Source.eEmitterType = DET_Mesh;

		// Take scale into account
		NewEmitterData->Source.Scale = FVector(1.0f, 1.0f, 1.0f);
		if (SpawnInstance->Component)
		{
			check(SpawnInstance->SpriteTemplate);
			UParticleLODLevel* LODLevel2 = SpawnInstance->SpriteTemplate->GetCurrentLODLevel(SpawnInstance);
			check(LODLevel2);
			check(LODLevel2->RequiredModule);
			// Take scale into account
			NewEmitterData->Source.Scale *= SpawnInstance->Component->Scale * SpawnInstance->Component->Scale3D;
			AActor* Actor = SpawnInstance->Component->GetOwner();
			if (Actor && !SpawnInstance->Component->AbsoluteScale)
			{
				NewEmitterData->Source.Scale *= Actor->DrawScale * Actor->DrawScale3D;
			}
		}

		// Fill it in...
		INT		ParticleCount	= ActiveParticles;
		UBOOL	bSorted			= FALSE;

		NewEmitterData->Source.ActiveParticleCount = ActiveParticles;
		NewEmitterData->Source.ScreenAlignment = LODLevel->RequiredModule->ScreenAlignment;
		NewEmitterData->Source.bUseLocalSpace = FALSE;
		NewEmitterData->Source.MeshAlignment = SpawnInstance->MeshTypeData->MeshAlignment;
		NewEmitterData->Source.bMeshRotationActive = SpawnInstance->MeshRotationActive;
		NewEmitterData->Source.MeshRotationOffset = SpawnInstance->MeshRotationOffset;
		NewEmitterData->Source.bScaleUV = LODLevel->RequiredModule->bScaleUV;
		NewEmitterData->Source.SubUVInterpMethod = LODLevel->RequiredModule->InterpolationMethod;
		NewEmitterData->Source.SubImages_Horizontal = LODLevel->RequiredModule->SubImages_Horizontal;
		NewEmitterData->Source.SubImages_Vertical = LODLevel->RequiredModule->SubImages_Vertical;
		NewEmitterData->Source.SubUVDataOffset = SpawnInstance->SubUVDataOffset;
		NewEmitterData->Source.EmitterRenderMode = SpawnInstance->SpriteTemplate->EmitterRenderMode;
		NewEmitterData->bSelected = bSelected;

		if (SpawnInstance->Module_AxisLock && (SpawnInstance->Module_AxisLock->bEnabled == TRUE))
		{
			NewEmitterData->Source.LockAxisFlag = SpawnInstance->Module_AxisLock->LockAxisFlags;
			if (SpawnInstance->Module_AxisLock->LockAxisFlags != EPAL_NONE)
			{
				NewEmitterData->Source.bLockAxis = TRUE;
			}
		}

		// If there are orbit modules, add the orbit module data
		if (LODLevel->OrbitModules.Num() > 0)
		{
			UParticleLODLevel* HighestLODLevel = SpawnInstance->SpriteTemplate->LODLevels(0);
			UParticleModuleOrbit* LastOrbit = HighestLODLevel->OrbitModules(LODLevel->OrbitModules.Num() - 1);
			check(LastOrbit);

			UINT* LastOrbitOffset = SpawnInstance->ModuleOffsetMap.Find(LastOrbit);
			NewEmitterData->Source.OrbitModuleOffset = *LastOrbitOffset;
		}

		if (SpawnInstance->Module_AxisLock && (SpawnInstance->Module_AxisLock->bEnabled == TRUE))
		{
			NewEmitterData->Source.LockAxisFlag = SpawnInstance->Module_AxisLock->LockAxisFlags;
			if (SpawnInstance->Module_AxisLock->LockAxisFlags != EPAL_NONE)
			{
				NewEmitterData->Source.bLockAxis = TRUE;
				switch (SpawnInstance->Module_AxisLock->LockAxisFlags)
				{
				case EPAL_X:
					NewEmitterData->Source.LockedAxis = FVector(1,0,0);
					break;
				case EPAL_Y:
					NewEmitterData->Source.LockedAxis = FVector(0,1,0);
					break;
				case EPAL_NEGATIVE_X:
					NewEmitterData->Source.LockedAxis = FVector(-1,0,0);
					break;
				case EPAL_NEGATIVE_Y:
					NewEmitterData->Source.LockedAxis = FVector(0,-1,0);
					break;
				case EPAL_NEGATIVE_Z:
					NewEmitterData->Source.LockedAxis = FVector(0,0,-1);
					break;
				case EPAL_Z:
				case EPAL_NONE:
				default:
					NewEmitterData->Source.LockedAxis = FVector(0,0,1);
					break;
				}
			}
		}

		// Setup dynamic render data.  Only call this AFTER filling in source data for the emitter.
		NewEmitterData->Init(
			bSelected, 
			SpawnInstance, 
			SpawnInstance->MeshTypeData->Mesh, 
			MeshComponent, 
			TRUE);		// Use Nx fluid?

		DynamicEmitter = NewEmitterData;
	}
}


/**
 *	Retrieves the dynamic data for the emitter
 *	
 *	@param	bSelected					Whether the emitter is selected in the editor
 *
 *	@return	FDynamicEmitterDataBase*	The dynamic data, or NULL if it shouldn't be rendered
 */
FDynamicEmitterDataBase* FPhysXMeshInstance::GetDynamicData(UBOOL bSelected)
{
	FDynamicMeshEmitterData* NewEmitterData = DynamicEmitter;
	DynamicEmitter = NULL;

	return NewEmitterData;
}

#endif	//#if WITH_NOVODEX && !NX_DISABLE_FLUIDS
