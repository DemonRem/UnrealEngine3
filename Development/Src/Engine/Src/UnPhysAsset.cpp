/*=============================================================================
	UnPhysAsset.cpp: Physics Asset - code for managing articulated assemblies of rigid bodies.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/ 

#include "EnginePrivate.h"
#include "EnginePhysicsClasses.h"

IMPLEMENT_CLASS(UPhysicsAsset);
IMPLEMENT_CLASS(URB_BodySetup);

IMPLEMENT_CLASS(UPhysicsAssetInstance);
IMPLEMENT_CLASS(URB_BodyInstance);

IMPLEMENT_CLASS(UPhysicalMaterial);
IMPLEMENT_CLASS(UPhysicalMaterialPropertyBase);


#if WITH_NOVODEX
	#include "UnNovodexSupport.h"

	static const NxReal SLEEP_ENERGY_THRESHOLD = 0.5f;
	static const NxReal SLEEP_DAMPING_AMOUNT = 0.2f;
	static const NxU32 BODY_SOLVER_ITERATIONS = 8;

#if SHOW_PHYS_INIT_COSTS
extern DOUBLE TotalCreateActorTime;
#endif

#define SHOW_SLOW_RELEASE_TIMES 0
#define SHOW_SLOW_RELEASE_TIMES_AMOUNT 0.1f

#endif // WITH_NOVODEX


///////////////////////////////////////	
//////////// UPhysicsAsset ////////////
///////////////////////////////////////

UBOOL UPhysicsAsset::LineCheck(FCheckResult& Result, class USkeletalMeshComponent* SkelComp, const FVector& Start, const FVector& End, const FVector& Extent)
{
	FVector Scale3D = SkelComp->Scale * SkelComp->Scale3D;
	AActor* SkelOwner = SkelComp->GetOwner();
	if( SkelOwner != NULL )
	{
		Scale3D *= SkelOwner ->DrawScale * SkelOwner->DrawScale3D;
	}

	if( !Scale3D.IsUniform() )
	{
		debugf( TEXT("UPhysicsAsset::LineCheck : Non-uniform scale factor. SkelComp %s Scale %f Scale3D %s Owner %s Scale %f Scale3D %s"), 
			*SkelComp->GetName(), 
			 SkelComp->Scale,
			*SkelComp->Scale3D.ToString(),
			 SkelOwner ? *SkelOwner->GetName() : TEXT("NULL"),
			 SkelOwner ? SkelOwner->DrawScale : 0.f,
			 SkelOwner ? *SkelOwner->DrawScale3D.ToString() : TEXT("N/A") );
		return 1;
	}

	UBOOL bIsZeroExtent = Extent.IsZero();

	Result.Item = INDEX_NONE;
	Result.LevelIndex = INDEX_NONE;
	Result.Time = 1.0f;
	Result.BoneName = NAME_None;
	Result.Component = NULL;
	Result.Material = NULL;
	Result.PhysMaterial = NULL;

	FCheckResult TempResult;

	for(INT i=0; i<BodySetup.Num(); i++)
	{
		URB_BodySetup* bs = BodySetup(i);

		if( (bIsZeroExtent && !bs->bBlockZeroExtent) || (!bIsZeroExtent && !bs->bBlockNonZeroExtent) )
		{
			continue;
		}

		// Find the index for the bone that matches this body setup
		INT BoneIndex = SkelComp->MatchRefBone(bs->BoneName);
		if(BoneIndex != INDEX_NONE)
		{
			FMatrix WorldBoneTM = SkelComp->GetBoneMatrix(BoneIndex);

			// Ignore bones that have been scaled to zero.
			FLOAT Det = WorldBoneTM.Determinant();
			if(Abs(Det) > KINDA_SMALL_NUMBER)
			{
				WorldBoneTM.RemoveScaling();

				TempResult.Time = 1.0f;

				bs->AggGeom.LineCheck(TempResult, WorldBoneTM, Scale3D, End, Start, Extent, FALSE);

				if(TempResult.Time < Result.Time)
				{
					Result = TempResult;
					Result.Item = i;
					Result.BoneName = bs->BoneName;
					Result.Component = SkelComp;
					Result.Actor = SkelComp->GetOwner();


					if( SkelComp->PhysicsAssetInstance != NULL )
					{
						check(SkelComp->PhysicsAssetInstance->Bodies.Num() == BodySetup.Num());
						Result.PhysMaterial = SkelComp->PhysicsAssetInstance->Bodies(Result.Item)->GetPhysicalMaterial();
					}
					else
					{
						// We can not know which material we hit without doing a per poly line check and that is SLOW
						Result.PhysMaterial = BodySetup(Result.Item)->PhysMaterial;
						if( SkelComp->PhysMaterialOverride != NULL )
						{
							Result.PhysMaterial = SkelComp->PhysMaterialOverride;
						}
					}					
				}
			}
		}
	}

	if(Result.Time < 1.0f)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}


UBOOL UPhysicsAsset::PointCheck(FCheckResult& Result, class USkeletalMeshComponent* SkelComp, const FVector& Location, const FVector& Extent)
{
	FVector Scale3D = SkelComp->Scale * SkelComp->Scale3D;
	if (SkelComp->GetOwner() != NULL)
	{
		Scale3D *= SkelComp->GetOwner()->DrawScale * SkelComp->GetOwner()->DrawScale3D;
	}

	Result.Time = 1.0f;

	FCheckResult TempResult;
	UBOOL bHit = FALSE;

	for(INT i=0; i<BodySetup.Num(); i++)
	{
		URB_BodySetup* bs = BodySetup(i);

		// Find the index for the bone that matches this body setup
		INT BoneIndex = SkelComp->MatchRefBone(bs->BoneName);
		if(BoneIndex != INDEX_NONE)
		{
			FMatrix WorldBoneTM = SkelComp->GetBoneMatrix(BoneIndex);
			WorldBoneTM.RemoveScaling();

			bHit = !bs->AggGeom.PointCheck(TempResult, WorldBoneTM, Scale3D, Location, Extent);

			// If we got a hit, fill in the result.
			if(bHit)
			{
				Result = TempResult;
				Result.Item = i;
				Result.BoneName = bs->BoneName;
				Result.Component = SkelComp;
				Result.Actor = SkelComp->GetOwner();
				// Grab physics material from BodySetup.
				Result.PhysMaterial = BodySetup(Result.Item)->PhysMaterial;

				// Currently we just return the first hit we find. Is this bad?
				break;
			}
		}
	}

	return !bHit;
}


FBox UPhysicsAsset::CalcAABB(USkeletalMeshComponent* SkelComp)
{
	FBox Box(0);

	FVector Scale3D = SkelComp->Scale * SkelComp->Scale3D;
	if (SkelComp->GetOwner() != NULL)
	{
		Scale3D *= SkelComp->GetOwner()->DrawScale * SkelComp->GetOwner()->DrawScale3D;
	}

	if( Scale3D.IsUniform() )
	{
		for(INT i=0; i<BodySetup.Num(); i++)
		{
			URB_BodySetup* bs = BodySetup(i);

			INT BoneIndex = SkelComp->MatchRefBone(bs->BoneName);
			if(BoneIndex != INDEX_NONE)
			{
				FMatrix WorldBoneTM = SkelComp->GetBoneMatrix(BoneIndex);
				WorldBoneTM.RemoveScaling();

				Box += bs->AggGeom.CalcAABB( WorldBoneTM, Scale3D );
			}
		}
	}
	else
	{
		debugf( TEXT("UPhysicsAsset::CalcAABB : Non-uniform scale factor.") );
	}

	if(!Box.IsValid)
	{
		Box = FBox( SkelComp->LocalToWorld.GetOrigin(), SkelComp->LocalToWorld.GetOrigin() );
	}

	return Box;
}

// Find the index of the physics bone that is controlling this graphics bone.
INT	UPhysicsAsset::FindControllingBodyIndex(class USkeletalMesh* skelMesh, INT StartBoneIndex)
{
	INT BoneIndex = StartBoneIndex;
	while(1)
	{
		FName BoneName = skelMesh->RefSkeleton(BoneIndex).Name;
		INT BodyIndex = FindBodyIndex(BoneName);

		if(BodyIndex != INDEX_NONE)
			return BodyIndex;

		INT ParentBoneIndex = skelMesh->RefSkeleton(BoneIndex).ParentIndex;

		if(ParentBoneIndex == BoneIndex)
			return INDEX_NONE;

		BoneIndex = ParentBoneIndex;
	}

	return INDEX_NONE; // Shouldn't reach here.
}


// TODO: These do a nasty linear search. Replace with TMap!!!
INT UPhysicsAsset::FindBodyIndex(FName bodyName)
{
	check( BodySetup.Num() == DefaultInstance->Bodies.Num() );

	for(INT i=0; i<BodySetup.Num(); i++)
	{
		if( BodySetup(i)->BoneName == bodyName )
			return i;
	}

	return INDEX_NONE;
}

INT UPhysicsAsset::FindConstraintIndex(FName constraintName)
{
	check( ConstraintSetup.Num() == DefaultInstance->Constraints.Num() );

	for(INT i=0; i<ConstraintSetup.Num(); i++)
	{
		if( ConstraintSetup(i)->JointName == constraintName )
			return i;
	}

	return INDEX_NONE;
}

FName UPhysicsAsset::FindConstraintBoneName(INT ConstraintIndex)
{
	check( ConstraintSetup.Num() == DefaultInstance->Constraints.Num() );

	if ( (ConstraintIndex < 0) || (ConstraintIndex >= ConstraintSetup.Num()) )
		return NAME_None;

	return ConstraintSetup(ConstraintIndex)->JointName;
}

/** Utility for getting indices of all bodies below (and including) the one with the supplied name. */
void UPhysicsAsset::GetBodyIndicesBelow(TArray<INT>& OutBodyIndices, FName InBoneName, USkeletalMesh* SkelMesh)
{
	INT BaseIndex = SkelMesh->MatchRefBone(InBoneName);

	// Iterate over all other bodies, looking for 'children' of this one
	for(INT i=0; i<BodySetup.Num(); i++)
	{
		URB_BodySetup* BS = BodySetup(i);
		FName TestName = BS->BoneName;
		INT TestIndex = SkelMesh->MatchRefBone(TestName);

		// We want to return this body as well.
		if(TestIndex == BaseIndex || SkelMesh->BoneIsChildOf(TestIndex, BaseIndex))
		{
			OutBodyIndices.AddItem(i);
		}
	}
}

void UPhysicsAsset::UpdateBodyIndices()
{
	for(INT i=0; i<DefaultInstance->Bodies.Num(); i++)
	{
		DefaultInstance->Bodies(i)->BodyIndex = i;
	}
}


// Find all the constraints that are connected to a particular body.
void UPhysicsAsset::BodyFindConstraints(INT bodyIndex, TArray<INT>& constraints)
{
	constraints.Empty();
	FName bodyName = BodySetup(bodyIndex)->BoneName;

	for(INT i=0; i<ConstraintSetup.Num(); i++)
	{
		if( ConstraintSetup(i)->ConstraintBone1 == bodyName || ConstraintSetup(i)->ConstraintBone2 == bodyName )
			constraints.AddItem(i);
	}
}

void UPhysicsAsset::ClearShapeCaches()
{
	for(INT i=0; i<BodySetup.Num(); i++)
	{
		BodySetup(i)->ClearShapeCache();
	}
}



///////////////////////////////////////
/////////// URB_BodySetup /////////////
///////////////////////////////////////

void URB_BodySetup::CopyBodyPropertiesFrom(class URB_BodySetup* fromSetup)
{
	CopyMeshPropsFrom(fromSetup);

	PhysMaterial = fromSetup->PhysMaterial;
	COMNudge = fromSetup->COMNudge;
	bFixed = fromSetup->bFixed;
	bNoCollision = fromSetup->bNoCollision;
}

// Cleanup all collision geometries currently in this StaticMesh. Must be called before the StaticMesh is destroyed.
void URB_BodySetup::ClearShapeCache()
{
#if WITH_NOVODEX
	// Clear pre-cooked data.
	PreCachedPhysData.Empty();


	// Clear created shapes.
	for (INT i = 0; i < CollisionGeom.Num(); i++)
	{
		NxActorDesc* ActorDesc = (NxActorDesc*) CollisionGeom(i);
		if (ActorDesc)
		{
			// Add any convex shapes being used to the 'pending destroy' list.
			// We can't destroy these now, because during GC we might hit this while some things are still using it.
			for(UINT j=0; j<ActorDesc->shapes.size(); j++)
			{
				NxShapeDesc* ShapeDesc = ActorDesc->shapes[j];

				// If this shape has a CCD skeleton - add it to the list to kill.
				if(ShapeDesc->ccdSkeleton)
				{
					GNovodexPendingKillCCDSkeletons.AddItem(ShapeDesc->ccdSkeleton);
				}
				
				// If its a convex, add its mesh data to the list to kill.
				if(ShapeDesc->getType() == NX_SHAPE_CONVEX)
				{
					NxConvexShapeDesc* ConvexDesc = (NxConvexShapeDesc*)ShapeDesc;
					GNovodexPendingKillConvex.AddItem( ConvexDesc->meshData );
				}

				// Free memory for the ShapeDesc
				delete ShapeDesc;
			}

			// Delete ActorDesc itself
			delete ActorDesc;
			CollisionGeom(i) = NULL;
		}
	}

	CollisionGeom.Empty();
	CollisionGeomScale3D.Empty();
#endif // WITH_NOVODEX
}

/** Pre-cache this mesh at all desired scales. */
void URB_BodySetup::PreCachePhysicsData()
{
	PreCachedPhysData.Empty();

	// Go over each scale we want to pre-cache data for.
	for(INT i=0; i<PreCachedPhysScale.Num(); i++)
	{
		INT NewDataIndex = PreCachedPhysData.AddZeroed();
		FKCachedConvexData& NewCachedData = PreCachedPhysData(NewDataIndex);

		FVector Scale3D = PreCachedPhysScale(i);
		if(Scale3D.GetMin() > KINDA_SMALL_NUMBER)
		{
			MakeCachedConvexDataForAggGeom( &NewCachedData, &AggGeom, Scale3D, *GetName() );
		}
	}
#if WITH_NOVODEX
	// Save version number
	PreCachedPhysDataVersion = GCurrentCachedPhysDataVersion;
#endif // WITH_NOVODEX
}

void URB_BodySetup::BeginDestroy()
{
	Super::BeginDestroy();

	AggGeom.FreeRenderInfo();
}	

void URB_BodySetup::FinishDestroy()
{
	ClearShapeCache();
	Super::FinishDestroy();
}

void URB_BodySetup::PreSave()
{
	Super::PreSave();

	if(!IsTemplate())
	{
		// Pre-cache physics data at particular scales for this mesh.
		PreCachePhysicsData();
	}
}

void URB_BodySetup::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if(Ar.Ver() >= VER_PRECACHE_STATICMESH_COLLISION)
	{
		Ar << PreCachedPhysData;
	}
}


void URB_BodySetup::PostLoad()
{
	Super::PostLoad();

	// Old version of convex, without some information we need. So calculate now.
	if(!IsTemplate() && GetLinkerVersion() < VER_NEW_SIMPLE_CONVEX_COLLISION)
	{
		INT i=0;
		while(i<AggGeom.ConvexElems.Num())
		{
			FKConvexElem& Convex = AggGeom.ConvexElems(i);
			UBOOL bValidHull = Convex.GenerateHullData();

			// Remove hulls which are not valid.
			if(bValidHull)
			{
				i++;
			}
			else
			{
				AggGeom.ConvexElems.Remove(i);
			}
		}

		MarkPackageDirty();
	}

	if(!IsTemplate() && GetLinkerVersion() < VER_FIX_BAD_INDEX_CONVEX_VERTEX_PERMUTE)
	{
		for (INT Index = 0; Index < AggGeom.ConvexElems.Num(); Index++)
		{
			FKConvexElem& Convex = AggGeom.ConvexElems(Index);
			// Create SIMD data structures for faster tests
			Convex.PermuteVertexData();
		}
	}
}

void URB_BodySetup::PostEditChange(UProperty* PropertyThatChanged)
{
	// If we change the CCD flag, we need to create shapes - so we have to flush the shape cache.
	if( PropertyThatChanged && (PropertyThatChanged->GetFName()  == FName(TEXT("bEnableContinuousCollisionDetection"))) )
	{
		// This is yucky because we are not actually free'ing the memory for the shapes, but if the game is running,
		// we will crash if we try and destroy the meshes while things are actually using them.
		CollisionGeom.Empty();
		CollisionGeomScale3D.Empty();
	}

	Super::PostEditChange(PropertyThatChanged);
}


///////////////////////////////////////
/////////// URB_BodyInstance //////////
///////////////////////////////////////

// Transform is in Unreal scale.
void URB_BodyInstance::InitBody(URB_BodySetup* setup, const FMatrix& transform, const FVector& Scale3D, UBOOL bFixed, UPrimitiveComponent* PrimComp, FRBPhysScene* InRBScene)
{
	check(PrimComp);
	check(setup);

	OwnerComponent = PrimComp;
	AActor* Owner = OwnerComponent->GetOwner();

#if WITH_NOVODEX
	// If there is already a body instanced, or there is no scene to create it into, do nothing.
	if (BodyData || !InRBScene)	// hardware scene support
	{
		return;
	}

	// Make the debug name for this geometry...
	FString DebugName(TEXT(""));
#if !FINAL_RELEASE
	if(Owner)
	{
		DebugName += FString::Printf( TEXT("Actor: %s "), *Owner->GetName() );
	}

	DebugName += FString::Printf( TEXT("Component: %s "), *PrimComp->GetName() );

	UStaticMeshComponent* SMComp = Cast<UStaticMeshComponent>(PrimComp);
	if(SMComp)
	{
		DebugName += FString::Printf( TEXT("StaticMesh: %s"), *SMComp->StaticMesh->GetPathName() );
	}

	if(setup->BoneName != NAME_None)
	{
		DebugName += FString::Printf( TEXT("Bone: %s "), *setup->BoneName.ToString() );
	}
#endif

	if(Scale3D.IsNearlyZero())
	{
		debugf(TEXT("URB_BodyInstance::InitBody : Scale3D is (nearly) zero: %s"), *DebugName);
		return;
	}

	// Try to find a shape with the matching scale
	NxArray<NxShapeDesc*>* Aggregate = NULL;
	for (INT i=0; i < setup->CollisionGeomScale3D.Num(); i++)
	{
		// Found a shape with the right scale
		if ((setup->CollisionGeomScale3D(i) - Scale3D).IsNearlyZero())
		{
			Aggregate = &((NxActorDesc*) setup->CollisionGeom(i))->shapes;
		}
	}

	// If no shape was found, create a new one and place it in the list
	if (!Aggregate)
	{
		// Instance a Novodex collision shape based on the stored unreal collision data.
		NxActorDesc* GeomActorDesc = NULL;

		// If this is a static mesh, check the cache in the ULevel to see if we have cooked data for it already
		FKCachedConvexData* CachedData = PrimComp->GetCachedPhysConvexData(Scale3D);

#ifdef ENABLE_CCD
		UBOOL bEnableCCD = setup->bEnableContinuousCollisionDetection;
#else
		UBOOL bEnableCCD = FALSE;
#endif

		// Actually instance the novodex geometry from the Unreal description.
		GeomActorDesc = setup->AggGeom.InstanceNovodexGeom(Scale3D, CachedData, bEnableCCD, *DebugName);
		if (GeomActorDesc)
		{
			Aggregate = &GeomActorDesc->shapes;
		}

		if (Aggregate)
		{
			setup->CollisionGeomScale3D.AddItem(Scale3D);
			setup->CollisionGeom.AddItem(GeomActorDesc);
		}
		else
		{
			debugf(TEXT("URB_BodyInstance::InitBody : Could not create new Shape: %s"), *DebugName);
			return;
		}
	}

	// Find the PhysicalMaterial to use for this instance.
	UPhysicalMaterial*	PhysMat  = GetPhysicalMaterial();
	NxMaterialIndex		MatIndex = InRBScene->FindPhysMaterialIndex( PhysMat );

	// Make the collision mask for this instance.
	NxGroupsMask GroupsMask = CreateGroupsMask(OwnerComponent->RBChannel, &(OwnerComponent->RBCollideWithChannels));

	// Create ActorDesc description, and copy list of primitives from stored geometry.
	NxActorDesc ActorDesc;
	for(UINT i=0; i<Aggregate->size(); i++)
	{
#ifndef NX_DISABLE_FLUIDS
		// Novodex fluids
		if( PrimComp->bFluidDrain )
		{
			((*Aggregate)[i])->shapeFlags |= NX_SF_FLUID_DRAIN;
		}
		else
		{
			((*Aggregate)[i])->shapeFlags &= ~(NxU32)NX_SF_FLUID_DRAIN;
		}
		if( PrimComp->bFluidTwoWay )
		{
			((*Aggregate)[i])->shapeFlags |= NX_SF_FLUID_TWOWAY;
		}
		else
		{
			((*Aggregate)[i])->shapeFlags &= ~(NxU32)NX_SF_FLUID_TWOWAY;
		}
#endif

		ActorDesc.shapes.push_back( (*Aggregate)[i] );

		// Set the material to the one specified in the PhysicalMaterial before creating this NxActor instance.
		ActorDesc.shapes[i]->materialIndex = MatIndex;

		// Set the collision filtering mask for this shape.
		ActorDesc.shapes[i]->groupsMask = GroupsMask;
	}

	NxMat34 nTM = U2NTransform(transform);

	ActorDesc.density = PhysMat->Density;
	ActorDesc.globalPose = nTM;

	if(setup->bNoCollision || !PrimComp->BlockRigidBody)
	{
		ActorDesc.flags = NX_AF_DISABLE_COLLISION;
	}

	// force more expensive cone friction that doesn't converge to axis aligned velocity
	if (PhysMat->bForceConeFriction)
	{
		ActorDesc.flags |= NX_AF_FORCE_CONE_FRICTION;
	}

	// caching this expression
	UBOOL bStatic = (Owner && Owner->bStatic);

	// Now fill in dynamics parameters.
	// If an owner and is static, don't create any dynamics properties for this Actor.
	NxBodyDesc BodyDesc;
	if(!bStatic)
	{
		// Set the damping properties from the PhysicalMaterial
		BodyDesc.angularDamping = PhysMat->AngularDamping;
		BodyDesc.linearDamping = PhysMat->LinearDamping;
		BodyDesc.sleepEnergyThreshold = SLEEP_ENERGY_THRESHOLD;
		BodyDesc.sleepDamping = SLEEP_DAMPING_AMOUNT;
		BodyDesc.solverIterationCount = BODY_SOLVER_ITERATIONS;

		// Inherit linear velocity from Unreal Actor.
		FVector uLinVel(0.f);
		if(Owner)
		{	
			uLinVel = Velocity = PreviousVelocity = Owner->Velocity;
		}

		// Set kinematic flag if body is not currently dynamics.
		if(setup->bFixed || bFixed)
		{
			BodyDesc.flags |= NX_BF_KINEMATIC;
		}

#ifdef ENABLE_CCD
		// If we don't want CCD, set the threshold to something really high.
		if(!setup->bEnableContinuousCollisionDetection)
		{
			BodyDesc.CCDMotionThreshold = BIG_NUMBER;
		}
		// If we do want it on, set a threshold velocity for it to work.
		else
		{
			// @todo at the moment its always on - maybe look at bounding box to figure out a good threshold.
			BodyDesc.CCDMotionThreshold = 0.f * U2PScale;
		}
#endif

		// Set linear velocity of body.
		BodyDesc.linearVelocity = U2NPosition(uLinVel);

		// Set the dominance group
		ActorDesc.dominanceGroup = Clamp<BYTE>(PrimComp->RBDominanceGroup, 0, 31);

		// Assign Body Description to Actor Description
		ActorDesc.body = &BodyDesc;
	}

	// Give the actor a chance to modify the NxActorDesc before we create the NxActor with it
	if(Owner)
	{
		Owner->ModifyNxActorDesc(ActorDesc,PrimComp);
	}

	if (!ActorDesc.isValid())
	{
		// check to make certain you have a correct Physic Asset and that your bone
		// names have not been changed
		debugf(TEXT("URB_BodyInstance::InitBody - Error, rigid body description invalid, %s"), *DebugName);
		return;
	}

	BodyData                   = NULL;
	NxScene *NovodexScene      = InRBScene->GetNovodexPrimaryScene();
	NxCompartment *Compartment = InRBScene->GetNovodexRigidBodyCompartment();
	UBOOL bUsingCompartment    = PrimComp->bUseCompartment && !bStatic && Compartment;
	if(bUsingCompartment)
	{
		ActorDesc.compartment = Compartment;
	}
	
#if SHOW_PHYS_INIT_COSTS
	DOUBLE StartCreate = appSeconds();
#endif
	NxActor* Actor = NovodexScene->createActor(ActorDesc);
#if SHOW_PHYS_INIT_COSTS
	TotalCreateActorTime += (appSeconds() - StartCreate);
#endif
	// removed scene loop....

	// If we made the NxActor successfully.
	if(Actor)
	{
		if(!bStatic)
		{
			check( Actor->isDynamic() );

			// Adjust mass scaling to avoid big differences between big and small objects.
			FLOAT OldMass = Actor->getMass();
			FLOAT NewMass = appPow(OldMass, 0.75f);
			//debugf( TEXT("OldMass: %f NewMass: %f"), OldMass, NewMass );

			// Apply user-defined mass scaling.
			NewMass *= Clamp<FLOAT>(setup->MassScale, 0.01f, 100.0f);

			FLOAT MassRatio = NewMass/OldMass;
			NxVec3 InertiaTensor = Actor->getMassSpaceInertiaTensor();
			Actor->setMassSpaceInertiaTensor(InertiaTensor * MassRatio);
			Actor->setMass(NewMass);

			// Apply the COMNudge
			NxVec3 nCOMNudge = U2NPosition(setup->COMNudge);

			NxVec3 nCOMPos = Actor->getCMassLocalPosition();
			Actor->setCMassOffsetLocalPosition(nCOMPos + nCOMNudge);

			if(setup->bFixed || bFixed)
			{
				Actor->clearBodyFlag( NX_BF_KINEMATIC );
				Actor->raiseBodyFlag( NX_BF_KINEMATIC );
			}

			if( Owner && Owner->Velocity.Size() > KINDA_SMALL_NUMBER )
			{
				// Wake up bodies that are part of a moving actor.
				Actor->wakeUp();
			}
			else
			{
				// Bodies should start out sleeping.
				Actor->putToSleep();
			}
		}

		// Put the NxActor into the 'notify on collision' group if desired.
		if(PrimComp->bNotifyRigidBodyCollision)
		{
			Actor->setGroup(UNX_GROUP_NOTIFYCOLLIDE);
		}

		// Store pointer to Novodex data in RB_BodyInstance
		BodyData = (FPointer)Actor;

		// Store pointer to owning bodyinstance.
		Actor->userData = this;
		
		// Store scene index
		SceneIndex = InRBScene->NovodexSceneIndex;

		// After starting up the physics, let the Actor do anything else it might want.
		if(Owner)
		{
			Owner->PostInitRigidBody(Actor, ActorDesc,PrimComp);
		}
	}
	else
	{
		debugf(TEXT("URB_BodyInstance::InitBody : Could not create NxActor: %s"), *DebugName);
	}
#endif // WITH_NOVODEX
}

/**
 *	Clean up the physics engine info for this instance.
 *	If Scene is NULL, it will always shut down physics. If an RBPhysScene is passed in, it will only shut down physics it the physics is in that scene. 
 *	Returns TRUE if physics was shut down, and FALSE otherwise.
 */
UBOOL URB_BodyInstance::TermBody(FRBPhysScene* Scene)
{
#if WITH_NOVODEX
	// If this body is in the scene we want, (or NULL was specified) kill it.
	if(Scene == NULL || SceneIndex == Scene->NovodexSceneIndex)
	{
		AActor* Owner = NULL;
		if(OwnerComponent)
		{
			Owner = OwnerComponent->GetOwner();
		}

#if !FINAL_RELEASE
		// Check to see if this physics call is illegal during this tick group
		if (GWorld && GWorld->InTick && GWorld->TickGroup == TG_DuringAsyncWork)
		{
			debugf(NAME_Error,TEXT("Can't call TermBody() on (%s)->(%s) during async work!"), *Owner->GetName(), *GetName());
		}
#endif

		// Check the software scene is still around. If it isn't - both have been torn down already so 
		// its unsafe to touch any NxActors etc
		NxScene *NovodexScene = GetNovodexPrimarySceneFromIndex( SceneIndex );
		if( NovodexScene )
		{
			NxActor * BoneSpringActor = (NxActor*)BodyData;
			if( BoneSpringActor )
			{
				NxScene * BoneSpringScene = &BoneSpringActor->getScene();
				check(BoneSpringScene);
				NxJoint* Spring = (NxJoint*)BoneSpring;
				if( Spring )
				{
					BoneSpringScene->releaseJoint(*Spring);
				}
			}

			// Clean up kinematic actor for bone spring if there is one.
			NxActor* KinActor = (NxActor*)BoneSpringKinActor;
			if(KinActor)
			{
				DestroyDummyKinActor(KinActor);
				BoneSpringKinActor = NULL;
			}
		}

		BoneSpring = NULL;

		if( NovodexScene )
		{
			NxActor* nActor = (NxActor*)BodyData;
			if( nActor )
			{
				if(Owner && !Owner->IsPendingKill())
				{
					Owner->PreTermRigidBody(nActor);
				}

#if SHOW_SLOW_RELEASE_TIMES || LOOKING_FOR_PERF_ISSUES
				DOUBLE ReleaseStart = appSeconds();
				INT NumShapes  = nActor->getNbShapes();
#endif

				// If physics is running, defer destruction of NxActor
				if (GWorld && GWorld->InTick && GWorld->TickGroup == TG_DuringAsyncWork)
				{
					nActor->userData = 0;
					GNovodexPendingKillActor.AddItem(nActor);
				}
				else
				{
					NovodexScene->releaseActor(*nActor);
				}

#if SHOW_SLOW_RELEASE_TIMES || LOOKING_FOR_PERF_ISSUES
					DOUBLE ReleaseTimeMs = (appSeconds() - ReleaseStart) * 1000.f;
					if(ReleaseTimeMs > SHOW_SLOW_RELEASE_TIMES_AMOUNT)
					{
						debugf(NAME_PerfWarning, TEXT("Novodex releaseActor (%s - %d shapes) took: %f ms"), *Owner->GetName(), NumShapes, ReleaseTimeMs );
					}
#endif
			}
		}

		BodyData = NULL;

		return TRUE;
	}
#endif // WITH_NOVODEX

	return FALSE;
}

void URB_BodyInstance::FinishDestroy()
{
	// Clean up physics engine stuff when the BodyInstance gets GC'd
	TermBody(NULL);

	Super::FinishDestroy();
}

void URB_BodyInstance::SetFixed(UBOOL bNewFixed)
{
#if WITH_NOVODEX

#if !FINAL_RELEASE
	// Check to see if this physics call is illegal during this tick group
	if (GWorld->InTick && GWorld->TickGroup == TG_DuringAsyncWork)
	{
		FString SpecificName;
		USkeletalMeshComponent* Comp = Cast<USkeletalMeshComponent>(OwnerComponent);
		if( Comp != NULL )
		{
			SpecificName = Comp->SkeletalMesh->GetFullName();
		}
		else
		{
			SpecificName = OwnerComponent ? *OwnerComponent->GetName() : TEXT("No Owner");
		}

		debugf(NAME_Error,TEXT("Can't call URB_BodyInstance::SetFixed() in '%s' during async work!"), *SpecificName );
	}
#endif

	NxActor* Actor = (NxActor*)BodyData;
	if(Actor && Actor->isDynamic())
	{
		// If we want it fixed, and it is currently not kinematic
		if(bNewFixed && !Actor->readBodyFlag(NX_BF_KINEMATIC))
		{
			Actor->raiseBodyFlag(NX_BF_KINEMATIC);
		}
		// If want to stop it being fixed, and it is currently kinematic
		else if(!bNewFixed && Actor->readBodyFlag(NX_BF_KINEMATIC))
		{
			Actor->clearBodyFlag(NX_BF_KINEMATIC);

			// Should not need to do this, but moveGlobalPose does not currently update velocity correctly so we are not using it.
			if(OwnerComponent)
			{
				AActor* Owner = OwnerComponent->GetOwner();
				if(Owner)
				{
					AActor* Owner = OwnerComponent->GetOwner();
					if(Owner)
					{
						setLinearVelocity(Actor, U2NPosition(Owner->Velocity) );
					}
				}
			}
		}
	}
#endif // WITH_NOVODEX
}

UBOOL URB_BodyInstance::IsFixed()
{
#if WITH_NOVODEX
	
	NxActor* Actor = (NxActor*)BodyData;
	if( Actor && Actor->isDynamic() )
	{
		if( !Actor->readBodyFlag(NX_BF_KINEMATIC) )
		{
			return FALSE;
		}
	}

	return TRUE;
#else
	return FALSE;
#endif
}

/** Used to disable rigid body collisions for this body. Overrides the bNoCollision flag in the BodySetup for this body. */
void URB_BodyInstance::SetBlockRigidBody(UBOOL bNewBlockRigidBody)
{
#if WITH_NOVODEX

#if !FINAL_RELEASE
	// Check to see if this physics call is illegal during this tick group
	if (GWorld->InTick && GWorld->TickGroup == TG_DuringAsyncWork)
	{
		debugf(NAME_Error,TEXT("Can't call URB_BodyInstance::SetBlockRigidBody() in '%s' during async work!"), OwnerComponent ? *OwnerComponent->GetName() : TEXT("No Owner"));
	}
#endif

	NxActor* Actor = (NxActor*)BodyData;
	if(Actor)
	{
		if(bNewBlockRigidBody)
		{
			Actor->clearActorFlag(NX_AF_DISABLE_COLLISION);
		}
		else
		{
			Actor->raiseActorFlag(NX_AF_DISABLE_COLLISION);
		}
	}
#endif // WITH_NOVODEX
}

UBOOL URB_BodyInstance::IsValidBodyInstance()
{
	UBOOL Retval = false;
#if WITH_NOVODEX
	NxActor* Actor = GetNxActor();

	if(Actor)
	{
		Retval = true;
	}
#endif // WITH_NOVODEX

	return Retval;
}

FMatrix URB_BodyInstance::GetUnrealWorldTM()
{
#if WITH_NOVODEX
	NxActor* Actor = GetNxActor();
	check(Actor);
	check(Actor->getNbShapes() > 0);

	NxMat34 nTM = Actor->getGlobalPose();
	FMatrix uTM = N2UTransform(nTM);

	return uTM;
#endif // WITH_NOVODEX

	return FMatrix::Identity;
}


FVector URB_BodyInstance::GetUnrealWorldVelocity()
{
#if WITH_NOVODEX
	NxActor* Actor = GetNxActor();
	check(Actor);

	FVector uVelocity(0.f);
	if(Actor->isDynamic())
	{
		NxVec3 nVelocity = Actor->getLinearVelocity();
		uVelocity = N2UPosition(nVelocity);
	}

	return uVelocity;
#endif // WITH_NOVODEX

	return FVector(0.f);
}

FVector URB_BodyInstance::GetUnrealWorldAngularVelocity()
{
#if WITH_NOVODEX
	NxActor* Actor = GetNxActor();
	check(Actor);

	FVector uAngVelocity(0.f);
	if(Actor->isDynamic())
	{
		NxVec3 nAngVelocity = Actor->getAngularVelocity();
		uAngVelocity = N2UVectorCopy(nAngVelocity);
	}

	return uAngVelocity;
#endif // WITH_NOVODEX

	return FVector(0.f);
}

FVector URB_BodyInstance::GetCOMPosition()
{
#if WITH_NOVODEX
	NxActor* Actor = GetNxActor();
	if(Actor)
	{
		NxVec3 nCOMPos = Actor->getCMassGlobalPosition();
		return N2UPosition(nCOMPos);
	}
	else
	{
		return FVector(0,0,0);
	}
#endif

	return FVector(0,0,0);
}

FLOAT URB_BodyInstance::GetBodyMass()
{
	FLOAT Retval = 0.f;

#if WITH_NOVODEX
	NxActor* Actor = GetNxActor();
	if(Actor)
	{
		Retval = Actor->getMass();
	}
	else
	{
		Retval = 0.f;
	}
#endif

	return Retval;
}

void URB_BodyInstance::DrawCOMPosition( FPrimitiveDrawInterface* PDI, FLOAT COMRenderSize, const FColor& COMRenderColor )
{
#if WITH_NOVODEX
	NxActor* Actor = GetNxActor();
	if(Actor)
	{
		NxVec3 nCOMPos = Actor->getCMassGlobalPosition();
		FVector COMPos = N2UPosition(nCOMPos);

		DrawWireStar(PDI, COMPos, COMRenderSize, COMRenderColor, SDPG_World);
	}
#endif
}

/** Utility for copying properties from one BodyInstance to another. */
void URB_BodyInstance::CopyBodyInstancePropertiesFrom(URB_BodyInstance* FromInst)
{
	bEnableBoneSpringLinear = FromInst->bEnableBoneSpringLinear;
	bEnableBoneSpringAngular = FromInst->bEnableBoneSpringAngular;
	bDisableOnOverextension = FromInst->bDisableOnOverextension;
	BoneLinearSpring = FromInst->BoneLinearSpring;
	BoneLinearDamping = FromInst->BoneLinearDamping;
	BoneAngularSpring = FromInst->BoneAngularSpring;
	BoneAngularDamping = FromInst->BoneAngularDamping;
	OverextensionThreshold = FromInst->OverextensionThreshold;
}

#if WITH_NOVODEX
class NxActor* URB_BodyInstance::GetNxActor()
{
	return (NxActor*)BodyData;
}
#endif // WITH_NOVODEX


/** Used to turn the angular/linear bone spring on and off. */
void URB_BodyInstance::EnableBoneSpring(UBOOL bInEnableLinear, UBOOL bInEnableAngular, const FMatrix& InBoneTarget)
{
#if WITH_NOVODEX
	// If we don't want any spring, but we have one - get rid of it.
	if(!bInEnableLinear && !bInEnableAngular && BoneSpring)
	{
		// If there is a kinematic actor for this spring, release it.
		NxActor* KinActor = (NxActor*)BoneSpringKinActor;
		if(KinActor)
		{
			DestroyDummyKinActor(KinActor);
		}
		BoneSpringKinActor = NULL;


		NxJoint* Spring = (NxJoint*)BoneSpring;
		// if bone spring was made, this is the scene it was put in.
		NxActor * BoneSpringActor = GetNxActor();
		if( BoneSpringActor )
		{
			BoneSpringActor->getScene().releaseJoint(*Spring);
		}
		BoneSpring = NULL;
	}
	// If we want a spring, but we don't have one, create here.
	else if((bInEnableLinear || bInEnableAngular) && !BoneSpring)
	{
		NxActor* BoneActor = GetNxActor();
		if(BoneActor)
		{
			// hardware scene support - get scene from actor
			NxScene * NovodexScene = &BoneActor->getScene();

			NxD6JointDesc Desc;
			Desc.actor[0] = BoneActor;

			if(bUseKinActorForBoneSpring)
			{
				NxActor* KinActor = CreateDummyKinActor(NovodexScene, InBoneTarget);
				BoneSpringKinActor = KinActor;
				Desc.actor[1] = KinActor;
			}
			// If we want to make spring to base body, find it here.
			else if(bMakeSpringToBaseCollisionComponent)
			{
				if( OwnerComponent && 
					OwnerComponent->GetOwner() && 
					OwnerComponent->GetOwner()->Base && 
					OwnerComponent->GetOwner()->Base->CollisionComponent )
				{
					NxActor* nSpringActor = OwnerComponent->GetOwner()->Base->CollisionComponent->GetNxActor();
					if(nSpringActor)
					{
						Desc.actor[1] = nSpringActor;
					}
				}
			}

			Desc.flags = NX_D6JOINT_SLERP_DRIVE;

			Desc.localAxis[0].set(1,0,0);
			Desc.localNormal[0].set(0,1,0);

			Desc.localAxis[1].set(1,0,0);
			Desc.localNormal[1].set(0,1,0);

			if( NovodexScene )
			{
				NxJoint* Spring = NovodexScene->createJoint(Desc);
				BoneSpring = Spring;
			}

			// Push drive params into the novodex joint now.
			SetBoneSpringParams(BoneLinearSpring, BoneLinearDamping, BoneAngularSpring, BoneAngularDamping);

			// Set the initial bone target to be correct.
			SetBoneSpringTarget(InBoneTarget, TRUE);
		}
	}

	// Turn drives on as required.
	if(BoneSpring)
	{
		NxJoint* Spring = (NxJoint*)BoneSpring;
		NxD6Joint* D6Joint = Spring->isD6Joint();
		check(D6Joint);

		NxD6JointDesc Desc;
		D6Joint->saveToDesc(Desc);

		if(bInEnableAngular)
		{
			Desc.slerpDrive.driveType = NX_D6JOINT_DRIVE_POSITION;
		}

		if(bInEnableLinear)
		{
			Desc.xDrive.driveType = NX_D6JOINT_DRIVE_POSITION;
			Desc.yDrive.driveType = NX_D6JOINT_DRIVE_POSITION;
			Desc.zDrive.driveType = NX_D6JOINT_DRIVE_POSITION;
		}

		D6Joint->loadFromDesc(Desc);
	}
#endif

	bEnableBoneSpringLinear = bInEnableLinear;
	bEnableBoneSpringAngular = bInEnableAngular;
}

/** Used to set the spring stiffness and  damping parameters for the bone spring. */
void URB_BodyInstance::SetBoneSpringParams(FLOAT InLinearSpring, FLOAT InLinearDamping, FLOAT InAngularSpring, FLOAT InAngularDamping)
{
#if WITH_NOVODEX
	if(BoneSpring)
	{
		NxJoint* Spring = (NxJoint*)BoneSpring;
		NxD6Joint* D6Joint = Spring->isD6Joint();
		check(D6Joint);

		NxD6JointDesc Desc;
		D6Joint->saveToDesc(Desc);

		Desc.xDrive.spring = InLinearSpring;
		Desc.xDrive.damping = InLinearDamping;

		Desc.yDrive.spring = InLinearSpring;
		Desc.yDrive.damping = InLinearDamping;

		Desc.zDrive.spring = InLinearSpring;
		Desc.zDrive.damping = InLinearDamping;

		Desc.slerpDrive.spring = InAngularSpring;
		Desc.slerpDrive.damping = InAngularDamping;

		D6Joint->loadFromDesc(Desc);
	}
#endif

	BoneLinearSpring = InLinearSpring;
	BoneLinearDamping = InLinearDamping;
	BoneAngularSpring = InAngularSpring;
	BoneAngularDamping = InAngularDamping;
}

/** Used to set desired target location of this spring. Usually called by UpdateRBBoneKinematics. */
void URB_BodyInstance::SetBoneSpringTarget(const FMatrix& InBoneTarget, UBOOL bTeleport)
{
#if WITH_NOVODEX
	if(BoneSpring)
	{
		FMatrix UseTarget = InBoneTarget;
		UseTarget.RemoveScaling();

		NxJoint* Spring = (NxJoint*)BoneSpring;
		NxD6Joint* D6Joint = Spring->isD6Joint();
		NxActor* KinActor = (NxActor*)BoneSpringKinActor;
		check(D6Joint);

		// If using a kinematic actor, just update its transform.
		if(KinActor)
		{
			NxMat34 nPose = U2NTransform(UseTarget);
			if(bTeleport)
			{
				KinActor->setGlobalPose(nPose);
			}
			else
			{
				KinActor->moveGlobalPose(nPose);
			}
		}
		// If not, update joint attachment to world.
		else
		{
			NxD6JointDesc Desc;
			D6Joint->saveToDesc(Desc);

			Desc.localAnchor[1] = U2NPosition(UseTarget.GetOrigin());
			Desc.localAxis[1] = U2NVectorCopy(UseTarget.GetAxis(0));
			Desc.localNormal[1] = U2NVectorCopy(UseTarget.GetAxis(1));

			D6Joint->loadFromDesc(Desc);
		}

		// @todo Hook to a toggleable parameter.
		// Draw bone springs, if desired.
		static bool bShowBoneSprings = FALSE;
		if (bShowBoneSprings)
		{
			NxActor *nActor0, *nActor1;
			Spring->getActors(&nActor0, &nActor1);
			if (nActor0)
			{
				FLinearColor LineColor(255, 255, 255);
				if( nActor0->readBodyFlag(NX_BF_KINEMATIC) )
				{
					// color red
					LineColor.G = 32; 
					LineColor.B = 32;
				}

				GWorld->LineBatcher->DrawLine(UseTarget.GetOrigin(), N2UPosition(nActor0->getGlobalPosition()), LineColor, SDPG_Foreground);
			}
		}

		// If desired - see if spring length has exceeded OverextensionThreshold
		if(bDisableOnOverextension || bTeleportOnOverextension)
		{
			NxActor *nActor0, *nActor1;
			Spring->getActors(&nActor0, &nActor1);
			check(nActor0);

			// Calculate distance from body to target (ie length of spring)
			FVector BodyPosition = N2UPosition( nActor0->getGlobalPosition() );
			FVector SpringError = UseTarget.GetOrigin() - BodyPosition;
			
			if( SpringError.Size() > OverextensionThreshold )
			{
				// If desired - disable spring.
				if(bDisableOnOverextension)
				{
					FMatrix Dummy = FMatrix::Identity;
					EnableBoneSpring(FALSE, FALSE, Dummy);
				}
				// Teleport entire asset.
				else if(bTeleportOnOverextension)
				{
					UPhysicsAssetInstance* Inst = Cast<UPhysicsAssetInstance>(GetOuter());
					if(Inst)
					{
						// Apply the spring delta to all the bodies (including this one).
						for (INT i=0; i<Inst->Bodies.Num(); i++)
						{
							check(Inst->Bodies(i));
							NxActor* UpdateActor = Inst->Bodies(i)->GetNxActor();
							if (UpdateActor != NULL)
							{
								FVector UpdatePos = N2UPosition(UpdateActor->getGlobalPosition());
								UpdateActor->setGlobalPosition( U2NPosition(UpdatePos + SpringError) );
							}
						}
					}
				}
			}
		}
	}
#endif
}

/** 
*	Changes the current PhysMaterialOverride for this body. 
*	Note that if physics is already running on this component, this will _not_ alter its mass/inertia etc, it will only change its 
*	surface properties like friction and the damping.
*/
void URB_BodyInstance::SetPhysMaterialOverride( UPhysicalMaterial* NewPhysMaterial )
{
#if !FINAL_RELEASE
	// Check to see if this physics call is illegal during this tick group
	if (GWorld->InTick && GWorld->TickGroup == TG_DuringAsyncWork)
	{
		debugf(NAME_Error,TEXT("Can't call SetPhysMaterialOverride() on (%s)->(%s) during async work!"), *OwnerComponent->GetOwner()->GetName(), *GetName());
		return;
	}
#endif

	// Save ref to PhysicalMaterial
	PhysMaterialOverride = NewPhysMaterial;

#if WITH_NOVODEX
	// Go through the chain of physical materials and update the NxActor
	UpdatePhysMaterialOverride();
#endif // WITH_NOVODEX
}

#if WITH_NOVODEX
/**
 *	UpdatePhysMaterialOverride
 *		Update Nx with the physical material that should be applied to this body
 *		Author: superville
 */
void URB_BodyInstance::UpdatePhysMaterialOverride()
{
	NxActor* nActor = GetNxActor();

	// If there is no physics object, nothing to do.
	if( nActor )
	{
		UPhysicalMaterial* PhysMat = GetPhysicalMaterial();

		// Turn PhysicalMaterial into PhysX material index using RBScene.
		NxScene* nScene = &nActor->getScene();
		check(nScene);
		FRBPhysScene* RBScene = (FRBPhysScene*)(nScene->userData);
		check(RBScene);
		NxMaterialIndex MatIndex = RBScene->FindPhysMaterialIndex( PhysMat );

		// Finally assign to all shapes in the physics object.
		SetNxActorMaterial( nActor, MatIndex, PhysMat );	
	}
}
#endif // WITH_NOVODEX

/**
*	GetPhysicalMaterial
*		Figure out which physical material to apply to the NxActor for this body.
*		Need to walk the chain of potential materials/overrides
*		Author: superville
*/
UPhysicalMaterial* URB_BodyInstance::GetPhysicalMaterial()
{
	check( GEngine->DefaultPhysMaterial != NULL );

	USkeletalMeshComponent* SkelComp = Cast<USkeletalMeshComponent>(OwnerComponent);
	UStaticMeshComponent*	StatComp = Cast<UStaticMeshComponent>(OwnerComponent);
	URB_BodySetup*			Setup	 = NULL;
	if( SkelComp != NULL && SkelComp->PhysicsAsset != NULL )
	{
		Setup = SkelComp->PhysicsAsset->BodySetup(BodyIndex);
	}
	if( StatComp != NULL && StatComp->StaticMesh != NULL )
	{
		Setup = StatComp->StaticMesh->BodySetup;
	}

	// Find the PhysicalMaterial we need to apply to the physics bodies.
	// (LOW priority) Engine Mat, Component Override, Setup Mat, Body Override (HIGH priority)
	UPhysicalMaterial* PhysMat = GEngine->DefaultPhysMaterial;					 // Fallback is engine default.
	if( OwnerComponent != NULL && OwnerComponent->PhysMaterialOverride != NULL ) // Next use component override
	{
		PhysMat = OwnerComponent->PhysMaterialOverride;
	}
	if( Setup != NULL && Setup->PhysMaterial != NULL )							 // Next use setup material
	{
		PhysMat = Setup->PhysMaterial;
	}
	if( PhysMaterialOverride != NULL )											 // Always use body override if it exists
	{
		PhysMat = PhysMaterialOverride;
	}

	return PhysMat;
}


///////////////////////////////////////
//////// UPhysicsAssetInstance ////////
///////////////////////////////////////

void UPhysicsAssetInstance::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar << CollisionDisableTable;
}

void UPhysicsAssetInstance::EnableCollision(class URB_BodyInstance* BodyA, class URB_BodyInstance* BodyB)
{
	if(BodyA == BodyB)
		return;

	FRigidBodyIndexPair Key(BodyA->BodyIndex, BodyB->BodyIndex);

	// If its not in table - do nothing
	if( !CollisionDisableTable.Find(Key) )
		return;

	CollisionDisableTable.Remove(Key);

#if WITH_NOVODEX
	NxActor* ActorA = BodyA->GetNxActor();
	NxActor* ActorB = BodyB->GetNxActor();

	if (ActorA && ActorB)
	{
		NxScene* NovodexScene = &(ActorA->getScene());
		NxU32 CurrentFlags = NovodexScene->getActorPairFlags(*ActorA, *ActorB);
		NovodexScene->setActorPairFlags(*ActorA, *ActorB, CurrentFlags & ~NX_IGNORE_PAIR);
	}
#endif // WITH_NOVODEX
}

void UPhysicsAssetInstance::DisableCollision(class URB_BodyInstance* BodyA, class URB_BodyInstance* BodyB)
{
	if(BodyA == BodyB)
		return;

	FRigidBodyIndexPair Key(BodyA->BodyIndex, BodyB->BodyIndex);

	// If its already in the disable table - do nothing
	if( CollisionDisableTable.Find(Key) )
		return;

	CollisionDisableTable.Set(Key, 0);

#if WITH_NOVODEX
	NxActor* ActorA = BodyA->GetNxActor();
	NxActor* ActorB = BodyB->GetNxActor();

	if (ActorA && ActorB)
	{
		NxScene* NovodexScene = &(ActorA->getScene());
		NxU32 CurrentFlags = NovodexScene->getActorPairFlags(*ActorA, *ActorB);
		NovodexScene->setActorPairFlags(*ActorA, *ActorB, CurrentFlags | NX_IGNORE_PAIR);
	}
#endif // WITH_NOVODEX
}

// Called to actually start up the physics of
void UPhysicsAssetInstance::InitInstance(USkeletalMeshComponent* SkelComp, class UPhysicsAsset* PhysAsset, UBOOL bFixed, FRBPhysScene* InRBScene)
{
	Owner = SkelComp->GetOwner();
	
	FString OwnerName( TEXT("None") );
	if(Owner)
	{
		OwnerName = Owner->GetName();
	}

	if(!InRBScene)
	{
		debugf(TEXT("UPhysicsAssetInstance::InitInstance : No RBPhysScene: %s"), *OwnerName);
		return;
	}

	if(!SkelComp->SkeletalMesh)
	{
		debugf(TEXT("UPhysicsAssetInstance::InitInstance : SkeletalMeshComponent has no SkeletalMesh: %s"), *OwnerName);
		return;
	}

	FVector Scale3D = SkelComp->Scale * SkelComp->Scale3D;
	if (Owner != NULL)
	{
		Scale3D *= Owner->DrawScale * Owner->DrawScale3D;
	}

	if( !Scale3D.IsUniform() )
	{
		debugf(TEXT("UPhysicsAssetInstance::InitInstance : Actor has non-uniform scaling: %s"), *OwnerName);
		return;
	}
	FLOAT Scale = Scale3D.X;

	if( Bodies.Num() != PhysAsset->BodySetup.Num() )
	{
		debugf(TEXT("UPhysicsAssetInstance::InitInstance : Asset/AssetInstance Body Count Mismatch (%d/%d) : %s"), PhysAsset->BodySetup.Num(), Bodies.Num(), *OwnerName);
		return;
	}

	if( Constraints.Num() != PhysAsset->ConstraintSetup.Num() )
	{
		debugf(TEXT("UPhysicsAssetInstance::InitInstance : Asset/AssetInstance Counstraint Count Mismatch (%d/%d) : %s"), PhysAsset->ConstraintSetup.Num(), Constraints.Num(), *OwnerName);
		return;
	}

	// Find root physics body
	USkeletalMesh* SkelMesh = SkelComp->SkeletalMesh;
	RootBodyIndex = INDEX_NONE;
	for(INT i=0; i<SkelMesh->RefSkeleton.Num() && RootBodyIndex == INDEX_NONE; i++)
	{
		INT BodyInstIndex = PhysAsset->FindBodyIndex( SkelMesh->RefSkeleton(i).Name );
		if(BodyInstIndex != INDEX_NONE)
		{
			RootBodyIndex = BodyInstIndex;
		}
	}

	if(RootBodyIndex == INDEX_NONE)
	{
		debugf(TEXT("UPhysicsAssetInstance::InitInstance : Could not find root physics body: %s"), Owner ? *Owner->GetName() : TEXT("NONE") );
		return;
	}

	// Create all the bodies.
	for(INT i=0; i<Bodies.Num(); i++)
	{
		URB_BodyInstance* BodyInst = Bodies(i);
		check(BodyInst);

		// Set the BodyIndex property in the BodyInstance.
		BodyInst->BodyIndex = i;

		// Get transform of bone by name.
		INT BoneIndex = SkelComp->MatchRefBone( PhysAsset->BodySetup(i)->BoneName );
		if(BoneIndex == INDEX_NONE)
		{
			continue;
		}

		FMatrix BoneTM = SkelComp->GetBoneMatrix( BoneIndex );
		BoneTM.RemoveScaling();

		// Create physics body instance.
		BodyInst->InitBody( PhysAsset->BodySetup(i), BoneTM, Scale3D, bFixed, SkelComp, InRBScene);

		// Enable bone springs if desired.
		if(BodyInst->bEnableBoneSpringLinear || BodyInst->bEnableBoneSpringAngular)
		{
			// If bMakeSpringToBaseCollisionComponent is TRUE, transform needs to be relative to base component body.
			if(BodyInst->bMakeSpringToBaseCollisionComponent)
			{
				if( SkelComp->GetOwner() &&
					SkelComp->GetOwner()->Base &&
					SkelComp->GetOwner()->Base->CollisionComponent )
				{
					FMatrix BaseL2W = SkelComp->GetOwner()->Base->CollisionComponent->LocalToWorld;
					FMatrix InvBaseL2W = BaseL2W.Inverse();
					BoneTM = BoneTM * InvBaseL2W;
				}
			}

			BodyInst->EnableBoneSpring(BodyInst->bEnableBoneSpringLinear, BodyInst->bEnableBoneSpringAngular, BoneTM);
		}
	}

	// Create all the constraints.
	for(INT i=0; i<Constraints.Num(); i++)
	{
		URB_ConstraintInstance* ConInst = Constraints(i);
		check( ConInst );

		// Set the ConstraintIndex property in the ConstraintInstance.
		ConInst->ConstraintIndex = i;

		// See if both bodies needed for this constraint actually exist in the skeletal mesh. Don't create constraint if this is not the case.
		UBOOL bConstraintOK = TRUE;

		FName ConBone1 = PhysAsset->ConstraintSetup(i)->ConstraintBone1;
		if( ConBone1 != NAME_None )
		{
			if( SkelComp->MatchRefBone(ConBone1) == INDEX_NONE )
			{
				bConstraintOK = FALSE;
			}
		}

		FName ConBone2 = PhysAsset->ConstraintSetup(i)->ConstraintBone2;
		if( ConBone2 != NAME_None )
		{
			if( SkelComp->MatchRefBone(ConBone2) == INDEX_NONE )
			{
				bConstraintOK = FALSE;
			}
		}

		// Constraint is OK - create physics joint
		if(bConstraintOK)
		{
			Constraints(i)->InitConstraint( SkelComp, SkelComp, PhysAsset->ConstraintSetup(i), Scale, Owner, SkelComp, FALSE );
		}
	}

	// Fill in collision disable table information.
	for(INT i=1; i<Bodies.Num(); i++)
	{
		for(INT j=0; j<i; j++)
		{
			FRigidBodyIndexPair Key(j,i);
			if( CollisionDisableTable.Find(Key) )
			{
#if WITH_NOVODEX
				NxActor* ActorA = Bodies(i)->GetNxActor();
				NxActor* ActorB = Bodies(j)->GetNxActor();

				if (ActorA && ActorB)
				{
					// hardware scene support
					NxScene * NovodexScene = &ActorA->getScene();
					check( &ActorB->getScene() == NovodexScene );

					NxU32 CurrentFlags = NovodexScene->getActorPairFlags(*ActorA, *ActorB);
					NovodexScene->setActorPairFlags(*ActorA, *ActorB, CurrentFlags | NX_IGNORE_PAIR);
				}
#endif // WITH_NOVODEX
			}
		}
	}	

}

/**
 *	Clean up all the physics engine info for this asset instance.
 *	If Scene is NULL, it will always shut down physics. If an RBPhysScene is passed in, it will only shut down physics if the asset is in that scene. 
 *	Returns TRUE if physics was shut down, and FALSE otherwise.
 */
UBOOL UPhysicsAssetInstance::TermInstance(FRBPhysScene* Scene)
{
	UBOOL bTerminating = FALSE;

	// We shut down the physics for each body and constraint here. 
	// The actual UObjects will get GC'd

	for(INT i=0; i<Constraints.Num(); i++)
	{
		check( Constraints(i) );
		UBOOL bTerminated = Constraints(i)->TermConstraint(Scene, FALSE);
		if(bTerminated)
		{
			Constraints(i) = NULL;
			bTerminating = TRUE;
		}
	}

	for(INT i=0; i<Bodies.Num(); i++)
	{
		check( Bodies(i) );
		UBOOL bTerminated = Bodies(i)->TermBody(Scene);
		if(bTerminated)
		{
			Bodies(i) = NULL;
			bTerminating = TRUE;
		}
	}

	return bTerminating;
}

/** Function to scale strength of all linear motors in the constraint instance. */
void UPhysicsAssetInstance::SetLinearDriveScale(FLOAT InLinearSpringScale, FLOAT InLinearDampingScale, FLOAT InLinearForceLimitScale)
{
	// Update params
	LinearSpringScale = InLinearSpringScale;
	LinearDampingScale = InLinearDampingScale;
	LinearForceLimitScale = InLinearForceLimitScale;

	// Iterate over each joint calling SetLinearDriveParams. This will then update motors taking into account the new drive scaling in the owning instance.
	for(INT i=0; i<Constraints.Num(); i++)
	{
		URB_ConstraintInstance* ConInst = Constraints(i);
		check(ConInst);

		ConInst->SetLinearDriveParams(ConInst->LinearDriveSpring, ConInst->LinearDriveDamping, ConInst->LinearDriveForceLimit);
	}
}

/** Function to scale strength of all angular motors in the constraint instance. */
void UPhysicsAssetInstance::SetAngularDriveScale(FLOAT InAngularSpringScale, FLOAT InAngularDampingScale, FLOAT InAngularForceLimitScale)
{
	// Update params
	AngularSpringScale		= InAngularSpringScale;
	AngularDampingScale		= InAngularDampingScale;
	AngularForceLimitScale	= InAngularForceLimitScale;

	// Iterate over each joint calling SetAngularDriveParams. This will then update motors taking into account the new drive scaling in the owning instance.
	for(INT i=0; i<Constraints.Num(); i++)
	{
		URB_ConstraintInstance* ConInst = Constraints(i);
		check(ConInst);

		ConInst->SetAngularDriveParams(ConInst->AngularDriveSpring, ConInst->AngularDriveDamping, ConInst->AngularDriveForceLimit);
	}
}

/** Set angular motors strength */
void UPhysicsAssetInstance::SetAllMotorsAngularDriveStrength(FLOAT InAngularSpringStrength, FLOAT InAngularDampingStrength, FLOAT InAngularForceLimitStrength, USkeletalMeshComponent* SkelMeshComp)
{
	if( !SkelMeshComp )
	{
		return;
	}

	const FLOAT DefSpringScale	= SkelMeshComp->PhysicsAsset->DefaultInstance->AngularSpringScale;
	const FLOAT DefDampScale	= SkelMeshComp->PhysicsAsset->DefaultInstance->AngularDampingScale;
	const FLOAT DefForceScale	= SkelMeshComp->PhysicsAsset->DefaultInstance->AngularForceLimitScale;

	SetAngularDriveScale
	(
		DefSpringScale * InAngularSpringStrength, 
		DefDampScale * InAngularDampingStrength, 
		DefForceScale * InAngularForceLimitStrength
	);
}

/** Utility which returns total mass of all bones below the supplied one in the hierarchy (including this one). */
FLOAT UPhysicsAssetInstance::GetTotalMassBelowBone(FName InBoneName, UPhysicsAsset* InAsset, USkeletalMesh* InSkelMesh)
{
	if(!InAsset || !InSkelMesh)
	{
		return 0.f;
	}

	TArray<INT> BodyIndices;
	InAsset->GetBodyIndicesBelow(BodyIndices, InBoneName, InSkelMesh);

	FLOAT TotalMass = 0.f;
	for(INT i=0; i<BodyIndices.Num(); i++)
	{
		TotalMass += Bodies(i)->GetBodyMass();
	}

	return TotalMass;
}


/** Fix or unfix all bodies */
void UPhysicsAssetInstance::SetAllBodiesFixed(UBOOL bNewFixed)
{
	for(INT i=0; i<Bodies.Num(); i++)
	{
		Bodies(i)->SetFixed(bNewFixed);
	}
}

/** Fix or unfix a list of bodies, by name */
void UPhysicsAssetInstance::SetNamedBodiesFixed(UBOOL bNewFixed, const TArray<FName>& BoneNames, USkeletalMeshComponent* SkelMesh, UBOOL bSetOtherBodiesToComplement)
{
	if( !SkelMesh || !SkelMesh->PhysicsAsset || !SkelMesh->PhysicsAssetInstance )
	{
		debugf(TEXT("UPhysicsAssetInstance::SetNamedBodiesFixed No SkeletalMesh or PhysicsAssetInstance for %s"), *SkelMesh->GetName());
		return;
	}

	// Fix / Unfix bones
	for(INT i=0; i<SkelMesh->PhysicsAsset->BodySetup.Num(); i++)
	{
		URB_BodyInstance*	BodyInst	= SkelMesh->PhysicsAssetInstance->Bodies(i);
		URB_BodySetup*		BodySetup	= SkelMesh->PhysicsAsset->BodySetup(i);

		// Update Bodies contained within given list
		if( BoneNames.ContainsItem(BodySetup->BoneName) )
		{
			BodyInst->SetFixed(bNewFixed);
		}
		else if( bSetOtherBodiesToComplement )
		// Set others to complement if bSetOtherBodiesToComplement is TRUE
		{
			BodyInst->SetFixed(!bNewFixed);
		}
	}
}

/** Enable or Disable AngularPositionDrive */
void UPhysicsAssetInstance::SetAllMotorsAngularPositionDrive(UBOOL bEnableSwingDrive, UBOOL bEnableTwistDrive)
{
	for(INT i=0; i<Constraints.Num(); i++)
	{
		Constraints(i)->SetAngularPositionDrive(bEnableSwingDrive, bEnableTwistDrive);
	}
}

void UPhysicsAssetInstance::SetNamedMotorsAngularPositionDrive(UBOOL bEnableSwingDrive, UBOOL bEnableTwistDrive, const TArray<FName>& BoneNames, USkeletalMeshComponent* SkelMeshComp, UBOOL bSetOtherBodiesToComplement)
{
	if( !SkelMeshComp || !SkelMeshComp->PhysicsAsset || SkelMeshComp->PhysicsAssetInstance != this )
	{
		return;
	}

	for(INT i=0; i<Constraints.Num(); i++)
	{
		URB_ConstraintSetup* CS = SkelMeshComp->PhysicsAsset->ConstraintSetup(Constraints(i)->ConstraintIndex);
		if( CS )
		{
			if( BoneNames.ContainsItem(CS->JointName) )
			{
				Constraints(i)->SetAngularPositionDrive(bEnableSwingDrive, bEnableTwistDrive);
			}
			else if( bSetOtherBodiesToComplement )
			{
				Constraints(i)->SetAngularPositionDrive(!bEnableSwingDrive, !bEnableTwistDrive);
			}
		}
	}
}

/** Set Angular Drive motors params for all constraint instance */
void UPhysicsAssetInstance::SetAllMotorsAngularDriveParams(FLOAT InSpring, FLOAT InDamping, FLOAT InForceLimit)
{
	for(INT i=0; i<Constraints.Num(); i++)
	{
		Constraints(i)->SetAngularDriveParams(InSpring, InDamping, InForceLimit);
	}
}


/** Use to toggle and set RigidBody angular and linear bone springs (see RB_BodyInstance). */
void UPhysicsAssetInstance::SetNamedRBBoneSprings(UBOOL bEnable, const TArray<FName>& BoneNames, FLOAT InBoneLinearSpring, FLOAT InBoneAngularSpring, USkeletalMeshComponent* SkelMeshComp)
{
	if( !SkelMeshComp )
	{
		return;
	}

	// Set up springs
	for(INT i=0; i<BoneNames.Num(); i++)
	{
		// Find the body instance to turn on spring for
		URB_BodyInstance* BoneBI	= SkelMeshComp->FindBodyInstanceNamed( BoneNames(i) );
		if( BoneBI && BoneBI->IsValidBodyInstance())
		{
			// Find current matrix of bone
			FMatrix BoneMatrix	= BoneBI->GetUnrealWorldTM();
			if( bEnable )
			{
				// If making bone spring to base body, transform needs to be relative to that body.
				if(BoneBI->bMakeSpringToBaseCollisionComponent)
				{
					if( BoneBI->OwnerComponent && 
						BoneBI->OwnerComponent->GetOwner() && 
						BoneBI->OwnerComponent->GetOwner()->Base && 
						BoneBI->OwnerComponent->GetOwner()->Base->CollisionComponent )
					{
						URB_BodyInstance* SpringBI = BoneBI->OwnerComponent->GetOwner()->Base->CollisionComponent->BodyInstance;
						if(SpringBI && SpringBI->IsValidBodyInstance())
						{
							FMatrix InvSpringBodyTM = SpringBI->GetUnrealWorldTM().Inverse();
							BoneMatrix = BoneMatrix * InvSpringBodyTM;
						}
					}
				}

				BoneBI->BoneLinearSpring	= InBoneLinearSpring;
				BoneBI->BoneAngularSpring	= InBoneAngularSpring;
			}
			BoneBI->EnableBoneSpring(bEnable, bEnable, BoneMatrix);
		}
	}
}

/** Use to toggle collision on particular bodies in the asset. */
void UPhysicsAssetInstance::SetNamedBodiesBlockRigidBody(UBOOL bNewBlockRigidBody, const TArray<FName>& BoneNames, USkeletalMeshComponent* SkelMesh)
{
	if( !SkelMesh || !SkelMesh->PhysicsAsset || !SkelMesh->PhysicsAssetInstance )
	{
		debugf(TEXT("UPhysicsAssetInstance::SetNamedBodiesBlockRigidBody No SkeletalMesh or PhysicsAssetInstance for %s"), *SkelMesh->GetName());
		return;
	}

	// Fix / Unfix bones
	for(INT i=0; i<SkelMesh->PhysicsAsset->BodySetup.Num(); i++)
	{
		URB_BodyInstance*	BodyInst	= SkelMesh->PhysicsAssetInstance->Bodies(i);
		URB_BodySetup*		BodySetup	= SkelMesh->PhysicsAsset->BodySetup(i);

		// Update Bodies contained within given list
		if( BoneNames.ContainsItem(BodySetup->BoneName) )
		{
			BodyInst->SetBlockRigidBody(bNewBlockRigidBody);
		}
	}
}

/** Allows you to fix/unfix bodies where bAlwaysFullAnimWeight is set to TRUE in the BodySetup. */
void UPhysicsAssetInstance::SetFullAnimWeightBonesFixed(UBOOL bNewFixed, USkeletalMeshComponent* SkelMesh)
{
	if( !SkelMesh || !SkelMesh->PhysicsAsset || !SkelMesh->PhysicsAssetInstance )
	{
		debugf(TEXT("UPhysicsAssetInstance::SetFullAnimWeightBonesFixed No SkeletalMesh or PhysicsAssetInstance for %s"), SkelMesh ? *SkelMesh->GetName() : TEXT("None"));
		return;
	}

	// Fix / Unfix bones
	for(INT i=0; i<SkelMesh->PhysicsAsset->BodySetup.Num(); i++)
	{
		URB_BodyInstance*	BodyInst	= SkelMesh->PhysicsAssetInstance->Bodies(i);
		URB_BodySetup*		BodySetup	= SkelMesh->PhysicsAsset->BodySetup(i);

		// Set fixed on any bodies with bAlwaysFullAnimWeight set to TRUE
		if( BodySetup->bAlwaysFullAnimWeight )
		{
			BodyInst->SetFixed(bNewFixed);
		}
	}
}

/** Find instance of the body that matches the name supplied. */
URB_BodyInstance* UPhysicsAssetInstance::FindBodyInstance(FName BodyName, UPhysicsAsset* InAsset)
{
	if(InAsset && InAsset->BodySetup.Num() == Bodies.Num())
	{
		INT BodyIndex = InAsset->FindBodyIndex(BodyName);
		if(BodyIndex != INDEX_NONE)
		{
			return Bodies(BodyIndex);
		}
	}

	return NULL;
}

/** Find instance of the constraint that matches the name supplied. */
URB_ConstraintInstance* UPhysicsAssetInstance::FindConstraintInstance(FName ConName, UPhysicsAsset* InAsset)
{
	if(InAsset && InAsset->ConstraintSetup.Num() == Constraints.Num())
	{
		INT ConIndex = InAsset->FindConstraintIndex(ConName);
		if(ConIndex != INDEX_NONE)
		{
			return Constraints(ConIndex);
		}
	}

	return NULL;
}


///////////////////////////////////////
////////// UPhysicalMaterial //////////
///////////////////////////////////////

void UPhysicalMaterial::PostEditChange(UProperty* PropertyThatChanged)
{
#if WITH_NOVODEX
	if(GWorld->RBPhysScene)
	{
		UINT NovodexMatIndex = GWorld->RBPhysScene->FindPhysMaterialIndex(this);
		NxScene* NovodexScene = GWorld->RBPhysScene->GetNovodexPrimaryScene();
		if(NovodexScene)
		{
			// We need to update the Novodex material corresponding to this Unreal one.
			NxMaterial* Material = NovodexScene->getMaterialFromIndex(NovodexMatIndex);
			check(Material);

			Material->setDynamicFriction(Friction);
			Material->setStaticFriction(Friction);
			Material->setRestitution(Restitution);
		}
	}
#endif

	//here we need to check to see if setting the parent to this PhysicalMaterial will
	// make a circular linked list of physical materials

	// we use the age old tortoise and the hare solution to finding a cycle in a linked list
	UPhysicalMaterial* Hare = this;
	UPhysicalMaterial* Turtoise = this;


	// move the tortoise one link forward and the hare two links forward
	// if they ever are the same we have a cycle
	// if the hare makes it to the end of the linked list (NULL) then no cycle
	do
	{	
		// move the tortoise once
		Turtoise = Turtoise->Parent;

		// move the hare forward twice
		Hare = Hare->Parent;
		if( NULL != Hare )
		{
			Hare = Hare->Parent;
		}

	}
	while( ( NULL != Hare )
		&& ( Hare != Turtoise ) 
		);

	// we have reached an end condition need to check which one
	if( NULL == Hare )
	{
		// all good
	}
	else if( Hare == Turtoise )
	{
		// need to send a warning and not allow the setting of the parent node
		appMsgf( AMT_OK, *LocalizeUnrealEd("Error_PhysicalMaterialCycleInHierarchy") );
		Parent = NULL;
	}

	Super::PostEditChange(PropertyThatChanged);
}



/**
 * This will fix any old PhysicalMaterials that were created in the PhysMaterial's outer instead
 * of correctly inside the PhysMaterial.  This will allow "broken" PhysMaterials to be renamed.
 **/
UBOOL UPhysicalMaterial::Rename( const TCHAR* InName, UObject* NewOuter, ERenameFlags Flags )
{
	// A long time ago in a galaxy far away, the properties were created in the physmaterial's outer, not in the physmaterial, so Rename won't work (since it's not in
	// what is being renamed... So, if this is the case, then we move the property into the physmaterial (this)
	if( PhysicalMaterialProperty && PhysicalMaterialProperty->GetOuter() == GetOuter() )
	{
		// the physmatproperty probably has other properties inside it that are also outside of the physmaterial, so we need to fix those up,
		// but they aren't native, so we have to loop through all objectprops looking for the evil ones
		// note: this will only happen for broken physmaterials, so this can't make it any worse
		for (TFieldIterator<UObjectProperty> It(PhysicalMaterialProperty->GetClass()); It; ++It)
		{
			UObject* PropVal = *((UObject**)((BYTE*)PhysicalMaterialProperty + It->Offset));

			if( PropVal != NULL )
			{
				check(PropVal->HasAnyFlags(RF_Public) == FALSE && TEXT("A PhysicalMaterial has properties that are marked public.  Run Fixup redirects to correct this issue."));

				if( PropVal->GetOuter() == GetOuter() )
				{
					PropVal->Rename(NULL, PhysicalMaterialProperty);
				}
			}
		}

		if( PhysicalMaterialProperty->Rename( *MakeUniqueObjectName(this, PhysicalMaterialProperty->GetClass()).ToString(), this ) == FALSE )
		{
			return FALSE;
		}
	}

	// Rename the physical material
	return Super::Rename( InName, NewOuter, Flags );
}



///////////////////////////////////////
////////// AKAsset //////////
///////////////////////////////////////


void AKAsset::CheckForErrors()
{
	Super::CheckForErrors();
	if( SkeletalMeshComponent == NULL )
	{
		GWarn->MapCheck_Add( MCTYPE_WARNING, this, *FString::Printf(TEXT("%s : KAsset actor has NULL SkeletalMeshComponent property - please delete!"), *GetName() ), MCACTION_DELETE, TEXT("KAssetSkeletalComponentNull") );
	}
	else
	{
		if( SkeletalMeshComponent->SkeletalMesh == NULL )
		{
			GWarn->MapCheck_Add( MCTYPE_WARNING, this, *FString::Printf(TEXT("%s : KAsset actor has a SkeletalMeshComponent with a NULL skeletal mesh"), *GetName() ), MCACTION_NONE, TEXT("KAssetSkeletalMeshNull") );
		}
		if( SkeletalMeshComponent->PhysicsAsset == NULL )
		{
			GWarn->MapCheck_Add( MCTYPE_WARNING, this, *FString::Printf(TEXT("%s : KAsset actor has a SkeletalMeshComponent with a NULL physics asset"), *GetName() ), MCACTION_NONE, TEXT("KAssetPhysicsAssetNull") );
		}
	}
}

/** Make KAssets be hit when using the TRACE_Mover flag. */
UBOOL AKAsset::ShouldTrace(UPrimitiveComponent *Primitive,AActor *SourceActor, DWORD TraceFlags)
{
	return (TraceFlags & TRACE_Movers) 
			&& ((TraceFlags & TRACE_OnlyProjActor) 
				? (bProjTarget || (bBlockActors && Primitive->BlockActors)) 
				: (!(TraceFlags & TRACE_Blocking) || (SourceActor && SourceActor->IsBlockedBy(this,Primitive))));
}

/** Allow specific control control over pawns being blocked by this KAsset. */
UBOOL AKAsset::IgnoreBlockingBy(const AActor *Other)
{
	if(!bBlockPawns && Other->GetAPawn())
		return TRUE;

	return Super::IgnoreBlockingBy(Other);
}
