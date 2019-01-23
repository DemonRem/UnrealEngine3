/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
#include "EnginePrivate.h"
#include "EnginePhysicsClasses.h"
#include "EngineSequenceClasses.h"
#include "UnStatChart.h"

/** Physics stats */
DECLARE_STATS_GROUP(TEXT("Physics"),STATGROUP_Physics);
DECLARE_CYCLE_STAT(TEXT("Total Dynamics Time"),STAT_RBTotalDynamicsTime,STATGROUP_Physics);
DECLARE_CYCLE_STAT(TEXT("Start Physics Time"),STAT_PhysicsKickOffDynamicsTime,STATGROUP_Physics);
DECLARE_CYCLE_STAT(TEXT("Fetch Results Time"),STAT_PhysicsFetchDynamicsTime,STATGROUP_Physics);
DECLARE_CYCLE_STAT(TEXT("Physics Events Time"),STAT_PhysicsEventTime,STATGROUP_Physics);
DECLARE_CYCLE_STAT(TEXT("Substep Time"),STAT_RBSubstepTime,STATGROUP_Physics);
DECLARE_CYCLE_STAT(TEXT("Solver Time"),STAT_RBSolver,STATGROUP_Physics);
DECLARE_CYCLE_STAT(TEXT("Nearphase Time"),STAT_RBNearphase,STATGROUP_Physics);
DECLARE_CYCLE_STAT(TEXT("Broadphase Update Time"),STAT_RBBroadphaseUpdate,STATGROUP_Physics);
DECLARE_CYCLE_STAT(TEXT("Broadphase GetPairs Time"),STAT_RBBroadphaseGetPairs,STATGROUP_Physics);

DECLARE_CYCLE_STAT(TEXT("Fluid Mesh Emitter Time"),STAT_PhysicsFluidMeshEmitterUpdate,STATGROUP_Physics);
DECLARE_CYCLE_STAT(TEXT("Physics Stats Time"),STAT_PhysicsOutputStats,STATGROUP_Physics);

DECLARE_DWORD_COUNTER_STAT(TEXT("Total SW Dynamic Bodies"),STAT_TotalSWDynamicBodies,STATGROUP_Physics);
DECLARE_DWORD_COUNTER_STAT(TEXT("Awake SW Dynamic Bodies"),STAT_AwakeSWDynamicBodies,STATGROUP_Physics);

DECLARE_DWORD_COUNTER_STAT(TEXT("Solver Bodies"),STAT_SWSolverBodies,STATGROUP_Physics);
DECLARE_DWORD_COUNTER_STAT(TEXT("Num Pairs"),STAT_SWNumPairs,STATGROUP_Physics);
DECLARE_DWORD_COUNTER_STAT(TEXT("Num Contacts"),STAT_SWNumContacts,STATGROUP_Physics);
DECLARE_DWORD_COUNTER_STAT(TEXT("Num Joints"),STAT_SWNumJoints,STATGROUP_Physics);

DECLARE_DWORD_COUNTER_STAT(TEXT("Num ConvexMesh"),STAT_NumConvexMeshes,STATGROUP_Physics);
DECLARE_DWORD_COUNTER_STAT(TEXT("Num TriMesh"),STAT_NumTriMeshes,STATGROUP_Physics);

DECLARE_MEMORY_STAT(TEXT("Novodex Allocation Size"),STAT_NovodexTotalAllocationSize,STATGROUP_Physics);
DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Novodex Allocation Count"),STAT_NovodexNumAllocations,STATGROUP_Physics);
DECLARE_CYCLE_STAT(TEXT("Novodex Allocator Time"),STAT_NovodexAllocatorTime,STATGROUP_Physics);

/** Physics Fluid Stats */
DECLARE_STATS_GROUP(TEXT("PhysicsFluid"),STATGROUP_PhysicsFluid);

DECLARE_DWORD_COUNTER_STAT(TEXT("Total Fluids"),STAT_TotalFluids,STATGROUP_PhysicsFluid);
DECLARE_DWORD_COUNTER_STAT(TEXT("Total Fluid Emitters"),STAT_TotalFluidEmitters,STATGROUP_PhysicsFluid);
DECLARE_DWORD_COUNTER_STAT(TEXT("Active Fluid Particles"),STAT_ActiveFluidParticles,STATGROUP_PhysicsFluid);
DECLARE_DWORD_COUNTER_STAT(TEXT("Total Fluid Particles"),STAT_TotalFluidParticles,STATGROUP_PhysicsFluid);
DECLARE_DWORD_COUNTER_STAT(TEXT("Total Fluid Packets"),STAT_TotalFluidPackets,STATGROUP_PhysicsFluid);


/** Physics Cloth Stats */
DECLARE_STATS_GROUP(TEXT("PhysicsCloth"),STATGROUP_PhysicsCloth);

DECLARE_DWORD_COUNTER_STAT(TEXT("Total Cloths"),STAT_TotalCloths,STATGROUP_PhysicsCloth);
DECLARE_DWORD_COUNTER_STAT(TEXT("Active Cloths"),STAT_ActiveCloths,STATGROUP_PhysicsCloth);
DECLARE_DWORD_COUNTER_STAT(TEXT("Active Cloth Vertices"),STAT_ActiveClothVertices,STATGROUP_PhysicsCloth);
DECLARE_DWORD_COUNTER_STAT(TEXT("Total Cloth Vertices"),STAT_TotalClothVertices,STATGROUP_PhysicsCloth);
DECLARE_DWORD_COUNTER_STAT(TEXT("Active Attached Cloth Vertices"),STAT_ActiveAttachedClothVertices,STATGROUP_PhysicsCloth);
DECLARE_DWORD_COUNTER_STAT(TEXT("Total Attached Cloth Vertices"),STAT_TotalAttachedClothVertices,STATGROUP_PhysicsCloth);

// Read from ini file in InitGameRBPhys.
FLOAT	NxTIMESTEP		= 0;
FLOAT	NxMAXDELTATIME	= 0;
INT		NxMAXSUBSTEPS	= 0;

UBOOL	NxRIGIDBODYCMPFIXEDTIMESTEP	= 0;
FLOAT	NxRIGIDBODYCMPTIMESTEP		= 0;
FLOAT	NxRIGIDBODYCMPMAXDELTATIME	= 0;
INT		NxRIGIDBODYCMPMAXSUBSTEPS	= 0;

UBOOL	NxFLUIDCMPFIXEDTIMESTEP		= 0;
FLOAT	NxFLUIDCMPTIMESTEP			= 0;
FLOAT	NxFLUIDCMPMAXDELTATIME		= 0;
INT		NxFLUIDCMPMAXSUBSTEPS		= 0;

UBOOL	NxCLOTHCMPFIXEDTIMESTEP		= 0;
FLOAT	NxCLOTHCMPTIMESTEP			= 0;
FLOAT	NxCLOTHCMPMAXDELTATIME		= 0;
INT		NxCLOTHCMPMAXSUBSTEPS		= 0;

/** If set keeps track of memory routed through this allocator. */
//#define KEEP_TRACK_OF_NOVODEX_ALLOCATIONS STATS
#define KEEP_TRACK_OF_NOVODEX_ALLOCATIONS 0

#if WITH_NOVODEX
#include "UnNovodexSupport.h"

// On PC, add support for dumping scenes to XML.
//#if !CONSOLE
//#  include "NXU_helper.h"
//#endif

#  if USE_QUICKLOAD_CONVEX
#  include "NxQuickLoad.h"
#  endif

#if defined(WIN32)
#define SUPPORT_DOUBLE_BUFFERING 0
#else
#define SUPPORT_DOUBLE_BUFFERING 0
#endif
#if SUPPORT_DOUBLE_BUFFERING
#include "NxdScene.h"
#endif

#if PS3
	#include "PS3/NxCellConfiguration.h"
#endif

#endif // WITH_NOVODEX


UBOOL FCollisionNotifyInfo::IsValidForNotify() const
{
	if(Info0.Actor && Info0.Actor->bDeleteMe)
	{
		return FALSE;
	}

	if(Info1.Actor && Info1.Actor->bDeleteMe)
	{
		return FALSE;
	}

	return TRUE;
}


#if WITH_NOVODEX
static INT NxDumpIndex = -1;
static UBOOL bOutputAllStats = FALSE;

static FNxContactReport		nContactReportObject;
static FNxNotify			nNotifyObject;
static FNxModifyContact		nContactModifyObject;

void FRBPhysScene::AddNovodexDebugLines(ULineBatchComponent* LineBatcherToUse)
{
	NxScene* NovodexScene = GetNovodexPrimaryScene();
	if(NovodexScene && LineBatcherToUse)
	{
		const NxDebugRenderable* DebugData = NovodexScene->getDebugRenderable();
		if(DebugData)
		{
			INT NumPoints = DebugData->getNbPoints();
			if(NumPoints > 0)
			{
				const NxDebugPoint* Points = DebugData->getPoints();
				for(INT i=0; i<NumPoints; i++)
				{
					DrawWireStar( LineBatcherToUse, N2UPosition(Points->p), 2.f, FColor((DWORD)Points->color), SDPG_World );
					Points++;
				}
			}

			// Build a list of all the lines we want to draw
			TArray<ULineBatchComponent::FLine> DebugLines;

			// Add all the 'lines' from Novodex
			INT NumLines = DebugData->getNbLines();
			if (NumLines > 0)
			{
				const NxDebugLine* Lines = DebugData->getLines();
				for(INT i = 0; i<NumLines; i++)
				{
					new(DebugLines) ULineBatchComponent::FLine(N2UPosition(Lines->p0), N2UPosition(Lines->p1), FColor((DWORD)Lines->color), 0.f, SDPG_World);
					Lines++;
				}
			}

			// Add all the 'triangles' from Novodex
			INT NumTris = DebugData->getNbTriangles();
			if(NumTris > 0)
			{
				const NxDebugTriangle* Triangles = DebugData->getTriangles();
				for(INT i=0; i<NumTris; i++)
				{
					new(DebugLines) ULineBatchComponent::FLine(N2UPosition(Triangles->p0), N2UPosition(Triangles->p1), FColor((DWORD)Triangles->color), 0.f, SDPG_World);
					new(DebugLines) ULineBatchComponent::FLine(N2UPosition(Triangles->p1), N2UPosition(Triangles->p2), FColor((DWORD)Triangles->color), 0.f, SDPG_World);
					new(DebugLines) ULineBatchComponent::FLine(N2UPosition(Triangles->p2), N2UPosition(Triangles->p0), FColor((DWORD)Triangles->color), 0.f, SDPG_World);
					Triangles++;
				}
			}

			// Draw them all in one call.
			LineBatcherToUse->DrawLines(DebugLines);
		}
	}
}


/** Called by Novodex when a constraint is broken. From here we call SeqAct_ConstraintBroken events. */
bool FNxNotify::onJointBreak(NxReal breakingForce, NxJoint& brokenJoint)
{
	URB_ConstraintInstance* Inst = (URB_ConstraintInstance*)(brokenJoint.userData);

	// Fire any events associated with this constraint actor.
	if(Inst && Inst->Owner)
	{
		for(INT Idx = 0; Idx < Inst->Owner->GeneratedEvents.Num(); Idx++)
		{
			USeqEvent_ConstraintBroken *BreakEvent = Cast<USeqEvent_ConstraintBroken>(Inst->Owner->GeneratedEvents(Idx));
			if (BreakEvent != NULL)
			{
				BreakEvent->CheckActivate(Inst->Owner, Inst->Owner);
			}
		}

		URB_ConstraintSetup* Setup = NULL;

		// Two cases supported here for finding the URB_ConstraintSetup
		USkeletalMeshComponent* SkelComp = Cast<USkeletalMeshComponent>(Inst->OwnerComponent);
		ARB_ConstraintActor* ConAct = Cast<ARB_ConstraintActor>(Inst->Owner);

		// One is the case of a constraint Actor in  the level - just look to the RB_ConstraintActor
		if(ConAct)
		{
			check(ConAct->ConstraintInstance == Inst);
			Setup = ConAct->ConstraintSetup;
		}
		else if(SkelComp)
		{
			check(SkelComp->PhysicsAssetInstance);
			check(SkelComp->PhysicsAsset);
			check(SkelComp->PhysicsAssetInstance->Constraints.Num() == SkelComp->PhysicsAsset->ConstraintSetup.Num());
			check(Inst->ConstraintIndex < SkelComp->PhysicsAsset->ConstraintSetup.Num());
			Setup = SkelComp->PhysicsAsset->ConstraintSetup(Inst->ConstraintIndex);
		}

		// maybe the constraint name  (add to TTP)
		const FVector& ConstrainLocation = Inst->GetConstraintLocation();
		Inst->Owner->eventConstraintBrokenNotify( Inst->Owner, Setup, Inst );
	}

	// We still hold references to this joint, so do not want it released, so we return false here.
	return false;
}

// BSP triangle gathering.
struct FBSPTriIndices
{
	INT v0, v1, v2;
};

static void GatherBspTrisRecursive( UModel* model, INT nodeIndex, TArray<FBSPTriIndices>& tris, TArray<NxMaterialIndex>& materials )
{
	check(GEngine->DefaultPhysMaterial);

	while(nodeIndex != INDEX_NONE)
	{
		FBspNode* curBspNode   = &model->Nodes( nodeIndex );

		INT planeNodeIndex = nodeIndex;
		while(planeNodeIndex != INDEX_NONE)
		{
			FBspNode* curPlaneNode = &model->Nodes( planeNodeIndex );
			FBspSurf* curSurf = &model->Surfs( curPlaneNode->iSurf );

			UPhysicalMaterial* PhysMat = GEngine->DefaultPhysMaterial;
			if(curSurf->Material && curSurf->Material->GetPhysicalMaterial())
			{
				PhysMat = curSurf->Material->GetPhysicalMaterial();
			}

			int vertexOffset = curPlaneNode->iVertPool;

			if( (curPlaneNode->NumVertices > 0) && !(curSurf->PolyFlags & PF_NotSolid)) /* If there are any triangles to add. */
			{
				for( int i = 2; i < curPlaneNode->NumVertices; i++ )
				{
					// Verts, indices added as a fan from v0
					FBSPTriIndices ti;
					ti.v0 = model->Verts( vertexOffset ).pVertex;
					ti.v1 = model->Verts( vertexOffset + i - 1 ).pVertex;
					ti.v2 = model->Verts( vertexOffset + i ).pVertex;

					tris.AddItem( ti );
					materials.AddItem( 0 );
				}
			}

			planeNodeIndex = curPlaneNode->iPlane;
		}

		// recurse back and iterate to front.
		if( curBspNode->iBack != INDEX_NONE )
		{
			GatherBspTrisRecursive(model, curBspNode->iBack, tris, materials );
		}

		nodeIndex = curBspNode->iFront;
	}
}
#endif // WITH_NOVODEX

void ULevel::BuildPhysBSPData()
{
#if WITH_NOVODEX
	if(Model->Nodes.Num() > 0)
	{
		// Generate a vertex buffer for all BSP verts at physics scale.
		TArray<FVector> PhysScaleTriVerts;
		PhysScaleTriVerts.Add( Model->Points.Num() );
		for(INT i=0; i<Model->Points.Num(); i++)
		{
			PhysScaleTriVerts(i) = Model->Points(i) * U2PScale;
		}

		// Harvest an overall index buffer for the level BSP.
		TArray<FBSPTriIndices> TriInidices;
		TArray<NxMaterialIndex> MaterialIndices;
		GatherBspTrisRecursive( Model, 0, TriInidices, MaterialIndices );
		check(TriInidices.Num() == MaterialIndices.Num());

		// Then create Novodex descriptor
		NxTriangleMeshDesc LevelBSPDesc;

		LevelBSPDesc.numVertices = PhysScaleTriVerts.Num();
		LevelBSPDesc.pointStrideBytes = sizeof(FVector);
		LevelBSPDesc.points = PhysScaleTriVerts.GetData();

		LevelBSPDesc.numTriangles = TriInidices.Num();
		LevelBSPDesc.triangleStrideBytes = sizeof(FBSPTriIndices);
		LevelBSPDesc.triangles = TriInidices.GetData();

		//LevelBSPDesc.materialIndexStride = sizeof(NxMaterialIndex);
		//LevelBSPDesc.materialIndices = MaterialIndices.GetData();

		LevelBSPDesc.flags = 0;

		CachedPhysBSPData.Empty();
		FNxMemoryBuffer Buffer(&CachedPhysBSPData);
		if( NxGetCookingParams().targetPlatform == PLATFORM_PC )
		{
			LevelBSPDesc.flags |= NX_MF_HARDWARE_MESH;
		}
		NxCookTriangleMesh(LevelBSPDesc, Buffer);

		// Log cooked physics size.
		debugf( TEXT("COOKEDPHYSICS: BSP %3.2f KB"), ((FLOAT)CachedPhysBSPData.Num())/1024.f );

		// Update cooked data version number.
		CachedPhysBSPDataVersion = GCurrentCachedPhysDataVersion;
	}
#endif // WITH_NOVODEX
}

void ULevel::InitLevelBSPPhysMesh()
{
#if WITH_NOVODEX
	// Do nothing if we already have an Actor for the level.
	// Just checking software version - that should always be around.
	if(LevelBSPActor)
	{
		return;
	}

	// If we have no physics mesh created yet - do it now
	// Again, just check software version of mesh
	if(!LevelBSPPhysMesh)
	{
		// Create the actor representing the level BSP. Do nothing if no BSP nodes.
		if( GWorld->RBPhysScene && Model->Nodes.Num() > 0 )
		{
			// If we don't have any cached data, or its out of date - cook it now and warn.
			if( CachedPhysBSPData.Num() == 0 || 
				CachedPhysBSPDataVersion != GCurrentCachedPhysDataVersion || 
				!bUsePrecookedPhysData)
			{
				debugf( TEXT("No Cached BSP Physics Data Found Or Out Of Date - Cooking Now.") );
				BuildPhysBSPData();
			}

#if XBOX || PS3
			check( GetCookedPhysDataEndianess(CachedPhysBSPData) != CPDE_LittleEndian );
#endif

			// Still may be no physics data if no structural brushes were present.
			if ( CachedPhysBSPData.Num() > 0 )
			{
				FNxMemoryBuffer Buffer(&CachedPhysBSPData);
				LevelBSPPhysMesh = GNovodexSDK->createTriangleMesh(Buffer);
				SetNxTriMeshRefCount(LevelBSPPhysMesh, DelayNxMeshDestruction);
				GNumPhysXTriMeshes++;
			}
		}

		// We don't need the cached physics data any more, so clear it
		CachedPhysBSPData.Empty();
	}

	// If we have a physics mesh - create NxActor.
	// Just check software version - should always be around.
	if(LevelBSPPhysMesh)
	{
		check(GEngine->DefaultPhysMaterial);

		NxTriangleMeshShapeDesc LevelBSPShapeDesc;
		LevelBSPShapeDesc.meshData = LevelBSPPhysMesh;
		LevelBSPShapeDesc.meshFlags = 0;
		LevelBSPShapeDesc.materialIndex = GWorld->RBPhysScene->FindPhysMaterialIndex( GEngine->DefaultPhysMaterial );
		LevelBSPShapeDesc.groupsMask = CreateGroupsMask(RBCC_Default, NULL);
		
		// Only use Mesh Paging on HW RB compartments
		FRBPhysScene* UseScene = GWorld->RBPhysScene;
		NxCompartment *RBCompartment = UseScene->GetNovodexRigidBodyCompartment();
		if(RBCompartment && RBCompartment->getDeviceCode() != NX_DC_CPU)
		{
			LevelBSPShapeDesc.meshPagingMode = NX_MESH_PAGING_AUTO;
		}

		// Create actor description and instance it.
		NxActorDesc LevelBSPActorDesc;
		LevelBSPActorDesc.shapes.pushBack(&LevelBSPShapeDesc);

		NxScene* NovodexScene = GWorld->RBPhysScene->GetNovodexPrimaryScene();
		check(NovodexScene);

		LevelBSPActor = NovodexScene->createActor(LevelBSPActorDesc);
		
		if( LevelBSPActor )
		{
			// No BodyInstance here.
			LevelBSPActor->userData = NULL;
		}
		else
		{
			// Log failure.
			debugf(TEXT("Couldn't create Novodex BSP actor"));
		}

		// Remember scene index.
		SceneIndex = GWorld->RBPhysScene->NovodexSceneIndex;
	}
#endif // WITH_NOVODEX
}

#if SHOW_PHYS_INIT_COSTS
DOUBLE TotalInstanceGeomTime;
DOUBLE TotalCreateActorTime;
DOUBLE TotalTerrainTime;
DOUBLE TotalPerTriStaticMeshTime;
DOUBLE TotalInitArticulatedTime;
DOUBLE TotalConstructBodyInstanceTime;
DOUBLE TotalInitBodyTime;
INT TotalConvexGeomCount;
#endif

/** Reset stats used for seeing where time goes initializing physics. */
void ULevel::ResetInitRBPhysStats()
{
#if SHOW_PHYS_INIT_COSTS
	TotalInstanceGeomTime = 0;
	TotalCreateActorTime = 0;
	TotalTerrainTime = 0;
	TotalPerTriStaticMeshTime = 0;
	TotalInitArticulatedTime = 0;
	TotalConstructBodyInstanceTime = 0;
	TotalInitBodyTime = 0;
	TotalConvexGeomCount = 0;
#endif
}

/** Output stats for initializing physics. */
void ULevel::OutputInitRBPhysStats()
{
#if SHOW_PHYS_INIT_COSTS
	if( ((TotalInstanceGeomTime * 1000) > 10) 
	||  ((TotalCreateActorTime  * 1000) > 10) )
	{
		debugf( NAME_PerfWarning, TEXT("InstanceGeom: %f ms - %d Convex, Terrain %f ms, PerTriSM %f ms, InitArticulated %f ms, BodyInst Alloc %f ms, InitBody %f ms"), 
			TotalInstanceGeomTime * 1000.f,
			TotalConvexGeomCount,
			TotalTerrainTime * 1000.f,
			TotalPerTriStaticMeshTime * 1000.f,
			TotalInitArticulatedTime * 1000.f,
			TotalConstructBodyInstanceTime * 1000.f,
			TotalInitBodyTime * 1000.f);
		debugf( NAME_PerfWarning, TEXT("NxActor Creation: %f ms"), TotalCreateActorTime * 1000.f );
	}
#endif
}

/**
 *	Iterates over all actors calling InitRBPhys on them.
 */
void ULevel::IncrementalInitActorsRBPhys(INT NumActorsToInit)
{
	// A value of 0 means that we want to update all components.
	if( NumActorsToInit == 0 )
	{
		NumActorsToInit = Actors.Num();
	}
	// Only the game can use incremental update functionality.
	else
	{
		checkMsg(!GIsEditor && GIsGame,TEXT("Cannot call IncrementalInitActorsRBPhys with non 0 argument in the Editor/ commandlets."));
	}

	// Reset creation stats if this is the first time in here.
	if(CurrentActorIndexForInitActorsRBPhys == 0)
	{
		ResetInitRBPhysStats();
	}

	NumActorsToInit = Min( NumActorsToInit, Actors.Num() - CurrentActorIndexForInitActorsRBPhys );

	// Do as many Actor's as we were told.
	for( INT i=0; i<NumActorsToInit; i++ )
	{
		AActor* Actor = Actors(CurrentActorIndexForInitActorsRBPhys++);
		if( Actor )
		{
			Actor->InitRBPhys();
		}
	}

	// See whether we are done.
	if( CurrentActorIndexForInitActorsRBPhys == Actors.Num() )
	{
		// Output stats for creation numbers.
		OutputInitRBPhysStats();

		// We only use the static-mesh physics data cache for startup - clear it now to free up more memory for run-time.
		ClearPhysStaticMeshCache();

		CurrentActorIndexForInitActorsRBPhys	= 0;

		// Set flag to indicate we have initialized all Actors.
		bAlreadyInitializedAllActorRBPhys		= TRUE;
	}
	// Only the game can use incremental update functionality.
	else
	{
		check(!GIsEditor && GIsGame);
	}
}


/**
 *	Destroys the physics engine BSP representation. 
 *	Does not iterate over actors - they are responsible for cleaning themselves up in AActor::Destroy.
 *	We don't free the actual physics mesh here, in case we want to InitLevelRBPhys again (ie unhide the level).
 */
void ULevel::TermLevelRBPhys(FRBPhysScene* Scene)
{
#if WITH_NOVODEX
	// hardware scene support
	if(Scene == NULL || SceneIndex == Scene->NovodexSceneIndex)
	{
		NxScene * NovodexScene = GetNovodexPrimarySceneFromIndex(SceneIndex);
		if( NovodexScene )
		{
			// Free the level BSP actor.
			if( LevelBSPActor )
			{
				NovodexScene->releaseActor(*(LevelBSPActor));
				LevelBSPActor = NULL;
			}
		}
	}
#endif // WITH_NOVODEX
}

void ULevel::FinishDestroy()
{
	TermLevelRBPhys(NULL);

#if WITH_NOVODEX

	// Add mesh to list to clean up.
	if(LevelBSPPhysMesh)
	{
		GNovodexPendingKillTriMesh.AddItem(LevelBSPPhysMesh);
		LevelBSPPhysMesh = NULL;
	}

#endif // WITH_NOVODEX

	Super::FinishDestroy();
}


/** Create the cache of cooked collision data for static meshes used in this level. */
void ULevel::BuildPhysStaticMeshCache()
{
	// Ensure cache is empty
	ClearPhysStaticMeshCache();

	INT TriByteCount = 0;
	INT TriMeshCount = 0;
	INT HullByteCount = 0;
	INT HullCount = 0;
	DOUBLE BuildPhysCacheStart = appSeconds();

	// Iterate over each actor in the level
	for(INT i=0; i<Actors.Num(); i++)
	{
		AActor* Actor = Actors(i);
		if(Actor)
		{
			// Iterate over all components of that actor
			for(INT j=0; j<Actor->AllComponents.Num(); j++)
			{
				// If its a static mesh component, with a static mesh
				UActorComponent* Comp = Actor->AllComponents(j);
				UStaticMeshComponent* SMComp = Cast<UStaticMeshComponent>(Comp);
				if(SMComp && SMComp->StaticMesh)
				{
					// Overall scale factor for this mesh.
					FVector TotalScale3D = SMComp->Scale * Actor->DrawScale * SMComp->Scale3D * Actor->DrawScale3D;

					// If we are doing per-tri collision..
					if(!SMComp->StaticMesh->UseSimpleRigidBodyCollision)
					{
						// See if we already have cached data for this mesh at this scale.
						FKCachedPerTriData* TestData = FindPhysPerTriStaticMeshCachedData(SMComp->StaticMesh, TotalScale3D);
						if(!TestData)
						{
							// If not, cook it now and add to store and map
							INT NewPerTriDataIndex = CachedPhysPerTriSMDataStore.AddZeroed();
							FKCachedPerTriData* NewPerTriData = &CachedPhysPerTriSMDataStore(NewPerTriDataIndex);

							FCachedPerTriPhysSMData NewCachedData;
							NewCachedData.Scale3D = TotalScale3D;
							NewCachedData.CachedDataIndex = NewPerTriDataIndex;

							FString DebugName = FString::Printf(TEXT("%s %s"), *Actor->GetName(), *SMComp->StaticMesh->GetName() );
							MakeCachedPerTriMeshDataForStaticMesh( NewPerTriData, SMComp->StaticMesh, TotalScale3D, *DebugName );

							// Log to memory used total.
							TriByteCount += NewPerTriData->CachedPerTriData.Num();
							TriMeshCount++;

							CachedPhysPerTriSMDataMap.Add( SMComp->StaticMesh, NewCachedData );

							//debugf( TEXT("Added PER-TRI: %s @ [%f %f %f]"), *SMComp->StaticMesh->GetName(), TotalScale3D.X, TotalScale3D.Y, TotalScale3D.Z );
						}
					}
					// If we have simplified collision..
					else if(SMComp->StaticMesh->BodySetup)
					{
						// And it has some convex bits
						URB_BodySetup* BodySetup = SMComp->StaticMesh->BodySetup;
						if(BodySetup->AggGeom.ConvexElems.Num() > 0)
						{
							// First see if its already in the cache
							FKCachedConvexData* TestData = FindPhysStaticMeshCachedData(SMComp->StaticMesh, TotalScale3D);

							// If not, cook it and add it.
							if(!TestData)
							{
								// Create new struct for the cache
								INT NewConvexDataIndex = CachedPhysSMDataStore.AddZeroed();
								FKCachedConvexData* NewConvexData = &CachedPhysSMDataStore(NewConvexDataIndex);

								FCachedPhysSMData NewCachedData;
								NewCachedData.Scale3D = TotalScale3D;
								NewCachedData.CachedDataIndex = NewConvexDataIndex;

								// Cook the collision geometry at the scale its used at in-level.
								FString DebugName = FString::Printf(TEXT("%s %s"), *Actor->GetName(), *SMComp->StaticMesh->GetName() );
								MakeCachedConvexDataForAggGeom( NewConvexData, &(BodySetup->AggGeom), TotalScale3D, *DebugName );

								// Add to memory used total.
								for(INT HullIdx = 0; HullIdx < NewConvexData->CachedConvexElements.Num(); HullIdx++)
								{
									FKCachedConvexDataElement& Hull = NewConvexData->CachedConvexElements(HullIdx);
									HullByteCount += Hull.ConvexElementData.Num();
									HullCount++;
								}

								// And add to the cache.
								CachedPhysSMDataMap.Add( SMComp->StaticMesh, NewCachedData );

								//debugf( TEXT("Added SIMPLE: %d - %s @ [%f %f %f]"), NewConvexDataIndex, *SMComp->StaticMesh->GetName(), TotalScale3D.X, TotalScale3D.Y, TotalScale3D.Z );
							}
						}
					}
				}
			}
		}
	}

#if WITH_NOVODEX
	// Update the version of this data to the current one.
	CachedPhysSMDataVersion = GCurrentCachedPhysDataVersion;
#endif

	debugf( TEXT("Built Phys StaticMesh Cache: %2.3f ms"), (appSeconds() - BuildPhysCacheStart) * 1000.f );
	debugf( TEXT("COOKEDPHYSICS: %d TriMeshes (%f KB), %d Convex Hulls (%f KB) - Total %f KB"), 
		TriMeshCount, ((FLOAT)TriByteCount)/1024.f, 
		HullCount, ((FLOAT)HullByteCount)/1024.f, 
		((FLOAT)(TriByteCount + HullByteCount))/1024.f);
}


/**  Clear the static mesh cooked collision data cache. */
void ULevel::ClearPhysStaticMeshCache()
{
	CachedPhysSMDataMap.Empty();
	CachedPhysSMDataStore.Empty();
	CachedPhysPerTriSMDataMap.Empty();
	CachedPhysPerTriSMDataStore.Empty();
}

/** 
 *	Utility for finding if we have cached data for a paricular static mesh at a particular scale.
 *	Returns NULL if there is no cached data. 
 *	The returned pointer will change if anything is added/removed from the cache, so should not be held.
 */
FKCachedConvexData* ULevel::FindPhysStaticMeshCachedData(UStaticMesh* InMesh, const FVector& InScale3D)
{
	// If we are intentionally not using precooked data, or its out of date, dont use it.
#if WITH_NOVODEX
	if( !bUsePrecookedPhysData || CachedPhysSMDataVersion != GCurrentCachedPhysDataVersion )
#endif // WITH_NOVODEX
	{
		return NULL;
	}

	// First look up mesh in map to find all cached data for this mesh
	TArray<FCachedPhysSMData> OutData;
	CachedPhysSMDataMap.MultiFind(InMesh, OutData);

	// Then iterate over results to see if we have one at the right scale.
	for(INT i=0; i<OutData.Num(); i++)
	{
		if( (OutData(i).Scale3D - InScale3D).IsNearlyZero() )
		{
			check( OutData(i).CachedDataIndex < CachedPhysSMDataStore.Num() );
			return &CachedPhysSMDataStore( OutData(i).CachedDataIndex );
		}
	}

	return NULL;
}

/** 
 *	Utility for finding if we have cached per-triangle data for a paricular static mesh at a particular scale.
 *	Returns NULL if there is no cached data. Pointer is to element of a map, so it invalid if anything is added/removed from map.
 */
FKCachedPerTriData* ULevel::FindPhysPerTriStaticMeshCachedData(UStaticMesh* InMesh, const FVector& InScale3D)
{
	// If we are intentionally not using precooked data, or its out of date, dont use it.
#if WITH_NOVODEX
	if( !bUsePrecookedPhysData || CachedPhysSMDataVersion != GCurrentCachedPhysDataVersion )
#endif // WITH_NOVODEX
	{
		return NULL;
	}

	// First look up mesh in map to find all cached data for this mesh
	TArray<FCachedPerTriPhysSMData> OutData;
	CachedPhysPerTriSMDataMap.MultiFind(InMesh, OutData);

	// Then iterate over results to see if we have one at the right scale.
	for(INT i=0; i<OutData.Num(); i++)
	{
		if( (OutData(i).Scale3D - InScale3D).IsNearlyZero() )
		{
			check( OutData(i).CachedDataIndex < CachedPhysPerTriSMDataStore.Num() );
			return &CachedPhysPerTriSMDataStore( OutData(i).CachedDataIndex );
		}
	}

	return NULL;
}


//////////////////////////////////////////////////////////////////////////
// UWORLD
//////////////////////////////////////////////////////////////////////////

/** Create the physics scene for the world. */
void UWorld::InitWorldRBPhys()
{
#if WITH_NOVODEX
	if( !RBPhysScene )
	{
		// JTODO: How do we handle changing gravity on the fly?
		FVector Gravity = FVector( 0.f, 0.f, GWorld->GetRBGravityZ() );

		RBPhysScene			= CreateRBPhysScene(Gravity);
	}
#endif // WITH_NOVODEX
}

/** Destroy the physics engine scene. */
void UWorld::TermWorldRBPhys()
{
#if WITH_NOVODEX
	if(RBPhysScene)
	{
		// Ensure all actors in the world are terminated before we clean up the scene.
		// If we don't do this, we'll have pointers to garbage NxActors and stuff.
		
		// We cannot iterate over the Levels array as we also need to deal with levels that are no longer associated
		// with the world but haven't been GC'ed before we change the world. The problem is TermWorldRBPhys destroying
		// the Novodex scene which is why we need to make sure all ULevel/ AActor objects have their physics terminated
		// before this happens.
		for( TObjectIterator<ULevel> It; It; ++It ) 
		{
			ULevel* Level = *It;
			
			for(INT j=0; j<Level->Actors.Num(); j++)
			{
				AActor* Actor = Level->Actors(j);
				if(Actor)
				{
					Actor->TermRBPhys(RBPhysScene);
				}
			}
			
			// Ensure level is cleaned up too.
			Level->TermLevelRBPhys(RBPhysScene);
		}
		
		// Ensure that all primitive components had a chance to delete their BodyInstance data.
		for( TObjectIterator<UPrimitiveComponent> It; It; ++It )
		{
			UPrimitiveComponent* PrimitiveComponent = *It;
			PrimitiveComponent->TermComponentRBPhys(RBPhysScene);
		}

		// Then release the scene itself.
		DestroyRBPhysScene(RBPhysScene);
		RBPhysScene = NULL;
	}
#endif // WITH_NOVODEX
}

/** Fire off physics engine thread. */
void UWorld::TickWorldRBPhys(FLOAT DeltaSeconds)
{
#if WITH_NOVODEX
	if (!RBPhysScene)
		return;

	FRBPhysScene* UseScene = RBPhysScene;

	// wait for any scenes that may still be running.
	WaitPhysCompartments(UseScene);

	// When ticking the main scene, clean up any physics engine resources (once a frame)
	DeferredRBResourceCleanup();

	FVector DefaultGravity( 0.f, 0.f, GWorld->GetRBGravityZ() );

	NxScene* NovodexScene = UseScene->GetNovodexPrimaryScene();
	if(NovodexScene)
	{
		NovodexScene->setGravity( U2NPosition(DefaultGravity) );
	}

    FLOAT maxSubstep = NxTIMESTEP;
    FLOAT maxDeltaTime = NxMAXDELTATIME;

    // clamp down... if this happens we are simming physics slower than real-time, so be careful with it.
    // it can improve framerate dramatically (really, it is the same as scaling all velocities down and
    // enlarging all timesteps) but at the same time, it will screw with networking (client and server will
    // diverge a lot more.)
    FLOAT physDT = DeltaSeconds > maxDeltaTime ? maxDeltaTime : DeltaSeconds;

	TickRBPhysScene(UseScene, physDT, maxSubstep, FALSE);
#endif // WITH_NOVODEX
}

/**
 * Waits for the physics scene to be done processing - blocking the main game thread if necessary.
 */
void UWorld::WaitWorldRBPhys()
{
#if WITH_NOVODEX
	if (!RBPhysScene)
		return;


	WaitRBPhysScene(RBPhysScene);
	if(!RBPhysScene->UsingBufferedScene)
	{
		RBPhysScene->AddNovodexDebugLines(GWorld->LineBatcher);
	}

#endif
}

/** 
*	Perform any cleanup of physics engine resources. 
*	This is deferred because when closing down the game, you want to make sure you are not destroying a mesh after the physics SDK has been shut down.
*/
void UWorld::DeferredRBResourceCleanup()
{
#if WITH_NOVODEX
	// Clean up any deferred actors.
	for(INT i=0; i<GNovodexPendingKillActor.Num(); i++)
	{
		NxActor* nActor = GNovodexPendingKillActor(i);
		check(nActor);

		NxScene& nScene = nActor->getScene();
		nScene.releaseActor(*nActor);
	}
	GNovodexPendingKillActor.Empty();

	// Clean up any ForceFields in the 'pending kill' array. (Note: to be done before deleting convex meshes)
	NxScene* nxScene = GWorld->RBPhysScene->GetNovodexPrimaryScene();
	// Clean up any Cloths in the 'pending kill' array. (Note: to be done before deleting cloth meshes)
	for(INT i=0; i<GNovodexPendingKillCloths.Num(); i++)
	{
		NxCloth* nCloth = GNovodexPendingKillCloths(i);
		check(nCloth);
		
		NxScene& nxScene = nCloth->getScene();

		nxScene.releaseCloth(*nCloth);
	}
	GNovodexPendingKillCloths.Empty();

	// Clean up any convex meshes in the 'pending kill' array.
	INT ConvexIndex = 0;
	while(ConvexIndex<GNovodexPendingKillConvex.Num())
	{
		NxConvexMesh* ConvexMesh = GNovodexPendingKillConvex(ConvexIndex);
		check(ConvexMesh);

		// We check that nothing is still using this Convex Mesh.
		INT Refs = ConvexMesh->getReferenceCount();
		if(Refs > DelayNxMeshDestruction)
		{
			debugf( TEXT("WARNING: Release aborted - ConvexMesh still in use!") ); // Wish I could give more info...
			GNovodexPendingKillConvex.Remove(ConvexIndex);
		}
		// Not ready to destroy yet - decrement ref count
		else if(Refs > 0)
		{
			SetNxConvexMeshRefCount(ConvexMesh, Refs-1);
			ConvexIndex++;
		}
		// Ref count is zero - release the mesh.
		else
		{
#if USE_QUICKLOAD_CONVEX
			NxReleaseQLConvexMesh(*ConvexMesh);
#else
			GNovodexSDK->releaseConvexMesh(*ConvexMesh);
#endif
			GNumPhysXConvexMeshes--;
			GNovodexPendingKillConvex.Remove(ConvexIndex);
		}
	}

	// Clean up any triangle meshes in the 'pending kill' array.
	INT TriIndex = 0;
	while(TriIndex<GNovodexPendingKillTriMesh.Num())
	{
		NxTriangleMesh* TriMesh = GNovodexPendingKillTriMesh(TriIndex);
		check(TriMesh);

		// We check that nothing is still using this Triangle Mesh.
		INT Refs = TriMesh->getReferenceCount();
		if(Refs > DelayNxMeshDestruction)
		{
			debugf( TEXT("WARNING: Release aborted - TriangleMesh still in use!") ); // Wish I could give more info...
			GNovodexPendingKillTriMesh.Remove(TriIndex);
		}
		// Not ready to destroy yet - decrement ref count
		else if(Refs > 0)
		{
			SetNxTriMeshRefCount(TriMesh, Refs-1);
			TriIndex++;
		}
		// Ref count is zero - release the mesh.
		else
		{
			GNovodexSDK->releaseTriangleMesh(*TriMesh);
			GNumPhysXTriMeshes--;
			GNovodexPendingKillTriMesh.Remove(TriIndex);
		}
	}

	// Clean up any heightfields in the 'pending kill' array.
	for(INT i=0; i<GNovodexPendingKillHeightfield.Num(); i++)
	{
		NxHeightField* HF = GNovodexPendingKillHeightfield(i);
		check(HF);
		GNovodexSDK->releaseHeightField(*HF);
	}
	GNovodexPendingKillHeightfield.Empty();

	// Clean up any CCD skeletons in the 'pending kill' array.
	for(INT i=0; i<GNovodexPendingKillCCDSkeletons.Num(); i++)
	{
		NxCCDSkeleton* CCDSkel = GNovodexPendingKillCCDSkeletons(i);
		check(CCDSkel);

		// Shouldn't need to check ref count - only Shapes use these.
		GNovodexSDK->releaseCCDSkeleton(*CCDSkel);
	}
	GNovodexPendingKillCCDSkeletons.Empty();

#if !NX_DISABLE_CLOTH
	// Clean up any cloth meshes in the 'pending kill' array.
	for(INT i=0; i<GNovodexPendingKillClothMesh.Num(); i++)
	{
		NxClothMesh* ClothMesh = GNovodexPendingKillClothMesh(i);
		check(ClothMesh);

		// We check that nothing is still using this Cloth Mesh.
		INT Refs = ClothMesh->getReferenceCount();
		if(Refs > 0)
		{
			debugf( TEXT("WARNING: Release aborted - ClothMesh still in use!") ); // Wish I could give more info...
		}

		GNovodexSDK->releaseClothMesh(*ClothMesh);
	}
	GNovodexPendingKillClothMesh.Empty();
#endif // !NX_DISABLE_CLOTH

	// Novodex fluids
#ifndef NX_DISABLE_FLUIDS
	for( INT i = 0; i < GNovodexPendingKillFluids.Num(); ++i )
	{
		NxFluid * fluid = GNovodexPendingKillFluids(i);
		if(fluid)
		{
			NxScene &Scene = fluid->getScene();
			Scene.releaseFluid(*fluid);
		}
	}
	GNovodexPendingKillFluids.Empty();
#endif //#ifndef NX_DISABLE_FLUIDS

#endif // WITH_NOVODEX

}

void AWorldInfo::SetLevelRBGravity(FVector NewGrav)
{
#if WITH_NOVODEX
	if(GWorld->RBPhysScene)
	{
		GWorld->RBPhysScene->SetGravity(NewGrav);
	}
#endif // WITH_NOVODEX
}

//////// GAME-LEVEL RIGID BODY PHYSICS STUFF ///////

#if WITH_NOVODEX
class FNxAllocator : public NxUserAllocator
{
#if KEEP_TRACK_OF_NOVODEX_ALLOCATIONS
	/** Sync object for thread safety */
	FCriticalSection* SyncObject;
#endif

public:
	/** Create the synch object if we are tracking allocations */
	FNxAllocator(void)
	{
#if KEEP_TRACK_OF_NOVODEX_ALLOCATIONS
		SyncObject = GSynchronizeFactory->CreateCriticalSection();
		check(SyncObject);
#endif
	}
	/** Create the synch object if we are tracking allocations */
	virtual ~FNxAllocator(void)
	{
#if KEEP_TRACK_OF_NOVODEX_ALLOCATIONS
		GSynchronizeFactory->Destroy(SyncObject);
#endif
	}

	virtual void* mallocDEBUG(size_t Size, const char* Filename, int Line)
	{
		STAT(CallCount++);
		clock(CallCycles);
		SCOPE_CYCLE_COUNTER(STAT_NovodexAllocatorTime);
		void* Pointer = appMalloc(Size);
		AddAllocation( Pointer, Size, Filename, Line );
		unclock(CallCycles);
		return Pointer;
	}

	virtual void* malloc(size_t Size)
	{
		STAT(CallCount++);
		clock(CallCycles);
		void* Pointer = appMalloc(Size);
		AddAllocation( Pointer, Size, "MALLOC NON DEBUG", 0 );
		unclock(CallCycles);
		return Pointer;
	}

	virtual void* realloc(void* Memory, size_t Size)
	{
		STAT(CallCount++);
		clock(CallCycles);
		RemoveAllocation( Memory );
		void* Pointer = appRealloc(Memory, Size);
		AddAllocation( Pointer, Size, "", 0 );
		unclock(CallCycles);
		return Pointer;
	}

	virtual void free(void* Memory)
	{
		STAT(CallCount++);
		clock(CallCycles);
		RemoveAllocation( Memory );
		appFree(Memory);
		unclock(CallCycles);
	}

	/** Number of cycles spent in allocator this frame.			*/
	static DWORD					CallCycles;
	/** Number of times allocator has been invoked this frame.	*/
	static DWORD					CallCount;

#if KEEP_TRACK_OF_NOVODEX_ALLOCATIONS
	struct FPhysXAlloc
	{
		INT Size;
#ifdef _DEBUG
		char Filename[100]; 
		INT	Count;
#endif
		INT Line;
	};

	/** Dynamic map from allocation pointer to size.			*/
	static TDynamicMap<PTRINT,FPhysXAlloc>	AllocationToSizeMap;
	/** Total size of current allocations in bytes.				*/
	static SIZE_T							TotallAllocationSize;
	/** Number of allocations.									*/
	static SIZE_T							NumAllocations;

	/**
	 * Add allocation to keep track of.
	 *
	 * @param	Pointer		Allocation
	 * @param	Size		Allocation size in bytes
	 */
	void AddAllocation( void* Pointer, SIZE_T Size, const char* Filename, INT Line )
	{
		if(!GExitPurge)
		{
			FScopeLock sl(SyncObject);
			NumAllocations++;
			TotallAllocationSize += Size;

			FPhysXAlloc Alloc;
#ifdef _DEBUG
			strncpy(Alloc.Filename, Filename, 100);
#endif
			Alloc.Size = Size;
			Alloc.Line = Line;

			AllocationToSizeMap.Set( (PTRINT) Pointer, Alloc );
			SET_DWORD_STAT(STAT_NovodexNumAllocations,NumAllocations);
			SET_DWORD_STAT(STAT_NovodexTotalAllocationSize,TotallAllocationSize);
			SET_DWORD_STAT(STAT_MemoryNovodexTotalAllocationSize,TotallAllocationSize);
		}
	}
	/**
	 * Remove allocation from list to track.
	 *
	 * @param	Pointer		Allocation
	 */
	void RemoveAllocation( void* Pointer )
	{
		if(!GExitPurge)
		{
			FScopeLock sl(SyncObject);
			NumAllocations--;
			FPhysXAlloc* AllocPtr = AllocationToSizeMap.Find( (PTRINT) Pointer );
			check(AllocPtr);
			TotallAllocationSize -= AllocPtr->Size;
			AllocationToSizeMap.Remove( (PTRINT) Pointer );
			SET_DWORD_STAT(STAT_NovodexNumAllocations,NumAllocations);
			SET_DWORD_STAT(STAT_NovodexTotalAllocationSize,TotallAllocationSize);
			SET_DWORD_STAT(STAT_MemoryNovodexTotalAllocationSize,TotallAllocationSize);
		}
	}
#else	//KEEP_TRACK_OF_NOVODEX_ALLOCATIONS
	void AddAllocation( void* Pointer, SIZE_T Size, const char* Filename, INT Line ) {}
	void RemoveAllocation( void* Pointer ) {}
#endif	//KEEP_TRACK_OF_NOVODEX_ALLOCATIONS
};

#if KEEP_TRACK_OF_NOVODEX_ALLOCATIONS
/** Dynamic map from allocation pointer to size.			*/
TDynamicMap<PTRINT,FNxAllocator::FPhysXAlloc> FNxAllocator::AllocationToSizeMap;
/** Total size of current allocations in bytes.				*/
SIZE_T FNxAllocator::TotallAllocationSize;
/** Number of allocations.									*/
SIZE_T FNxAllocator::NumAllocations;
#endif	//KEEP_TRACK_OF_NOVODEX_ALLOCATIONS
/** Number of cycles spent in allocator this frame.			*/
DWORD FNxAllocator::CallCycles;
/** Number of times allocator has been invoked this frame.	*/
DWORD FNxAllocator::CallCount;

class FNxOutputStream : public NxUserOutputStream
{
public:
	virtual void reportError(NxErrorCode ErrorCode, const char* Message, const char* Filename, int Line)
	{
		// Hack to suppress warnings about open meshes (get this when cooking terrain) and static compounds.
		if( appStrstr( ANSI_TO_TCHAR(Message), TEXT("Mesh has a negative volume!") ) == NULL && 
			appStrstr( ANSI_TO_TCHAR(Message), TEXT("Creating static compound shape") ) == NULL )
		{
			debugfSuppressed(NAME_DevPhysics, TEXT("Error (%d) in file %s, line %d: %s"), (INT)ErrorCode, ANSI_TO_TCHAR(Filename), Line, ANSI_TO_TCHAR(Message));
		}
	}

	virtual NxAssertResponse reportAssertViolation(const char* Message, const char* Filename, int Line)
	{
		debugfSuppressed(NAME_DevPhysics, TEXT("Assert in file %s, line %d: %s"), ANSI_TO_TCHAR(Filename), Line, ANSI_TO_TCHAR(Message));
		return NX_AR_BREAKPOINT;
	}

	virtual void print(const char* Message)
	{
		debugfSuppressed(NAME_DevPhysics, TEXT("%s"), ANSI_TO_TCHAR(Message));
	}
};

#if XBOX
/**
 * This class manages the scheduling of all physics tasks. It uses the thread
 * pool to parcel out tasks to. Background tasks are queued up and passed out
 * when not explicitly simulating physics.
 */
class FPhysicsScheduler :
	public NxUserScheduler
{
	/** Wrapper class that interfaces between the two async systems */
	class FPhysicsTask :
		public FQueuedWork
	{
		/**
		 * The task that needs to be executed
		 */
		NxTask* Task;
		/**
		 * The integer to decrement when done. Can be NULL
		 */
		volatile INT* TaskDone;
		/**
		 * Used when auto cleaning up (not pooled)
		 */
		UBOOL bShouldAutoDelete;

	public:
		/**
		 * Constructs a wrapper task by copying values and marks this object
		 * to be auto deleted. Assumes it was heap allocated
		 */
		FPhysicsTask(NxTask* InTask) :
			Task(InTask),
			TaskDone(NULL),
			bShouldAutoDelete(TRUE)
		{
		}

		/**
		 * Zeros all values and assumes was allocated from the pool
		 */
		FPhysicsTask(void) :
			Task(NULL),
			TaskDone(NULL),
			bShouldAutoDelete(FALSE)
		{
		}

		/**
		 * Sets the current task that is to be executed
		 *
		 * @param InTask the task to process
		 * @param InTaskDone the counter to change when done
		 */
		void SetTask(NxTask* InTask,volatile INT* InTaskDone)
		{
			// Verify that we didn't run out of pool space
			check(Task == NULL && InTask != NULL);
			Task = InTask;
			TaskDone = InTaskDone;
		}

		/**
		 * Forwards the call to the task object
		 */
		virtual void DoWork(void)
		{
			Task->execute();
		}

		/**
		 * Marks the job as done and cleans up
		 */
		virtual void Abandon(void)
		{
			Dispose();
		}

		/**
		 * Marks the job as done and cleans up
		 */
		virtual void Dispose(void)
		{
			if (TaskDone != NULL)
			{
				appInterlockedDecrement(TaskDone);
				// Zero for the next round
				Task = NULL;
				TaskDone = NULL;
			}
			// Done if heap allocated
			if (bShouldAutoDelete)
			{
				delete this;
			}
		}
	};

	/**
	 * Synch object to use when adding/removing tasks from the list
	 */
	FCriticalSection* TaskSync;
	/**
	 * A set of tasks that were queued and need to happen outside of simulation
	 */
	TArray<FPhysicsTask*> PendingBackgroundTasks;
	/**
	 * A counter indicating the current number of tasks waiting to be executed
	 */
	volatile INT TasksRemaining;
	/**
	 * Pool of tasks to use
	 */
	FPhysicsTask PooledTasks[128];
	/**
	 * Index to the next free task in the pool
	 */
	DWORD NextAvailTask;
	/**
	 * Set of worker threads just for physics
	 */
	FQueuedThreadPoolWin ThreadPool;

public:
	/**
	 * Allocates the sync object and zeros the other members
	 */
	FPhysicsScheduler(void) :
		TaskSync(NULL),
		TasksRemaining(0)
	{
		TaskSync = GSynchronizeFactory->CreateCriticalSection();
		check(TaskSync);
		ThreadPool.Create(2,16 + 32);
	}

	/**
	 * Cleans up any allocated resources
	 */
	virtual ~FPhysicsScheduler(void)
	{
		GSynchronizeFactory->Destroy(TaskSync);
		ThreadPool.Destroy();
	}

	/** Singleton accessor method that on demand allocates */
	static FPhysicsScheduler* GetInstance(void)
	{
		static FPhysicsScheduler* Instance = NULL;
		if (Instance == NULL)
		{
			Instance = new FPhysicsScheduler();
		}
		return Instance;
	}

// NxUserScheduler interface

    /**
	 * Passes the task to our thread pool for processing
	 */
	virtual void addTask(NxTask* Task)
	{
		// Add to the outstanding number first in case it's really fast
		appInterlockedIncrement(&TasksRemaining);
		// This is expected to only be called from one thread
		NextAvailTask = (NextAvailTask + 1) % 128;
		// Grab the object we are going to use
		FPhysicsTask* Wrapper = &PooledTasks[NextAvailTask];
		// Set the task data and fire it off
		Wrapper->SetTask(Task,&TasksRemaining);
		// Create our wrapper object and queue it to be done
		ThreadPool.AddQueuedWork(Wrapper);
	}

	/**
	 * If we aren't simulating, then the task is immediately passed on to the
	 * thread pool for processing. If we are simulating, then the task is placed
	 * on a queue for processing once simulating is complete
	 */
	virtual void addBackgroundTask(NxTask* Task)
	{
		// Only process non-critical tasks if we aren't currently simulating
		if (TasksRemaining > 0)
		{
			FScopeLock sl(TaskSync);
			// Queue the task to be executed later
			PendingBackgroundTasks.AddItem(new FPhysicsTask(Task));
		}
		else
		{
			// Immediately pass this off to the thread pool
			ThreadPool.AddQueuedWork(new FPhysicsTask(Task));
		}
	}

	/**
	 * Called to block until all outstanding simulation tasks are complete
	 */
    virtual void waitTasksComplete(void)
	{
		// On Xe, this is fine to busy loop with a yield
		while (TasksRemaining > 0)
		{
			appSleep(0.f);
		}
	}

	/**
	 * Kicks off any background tasks
	 */
	void ProcessBackgroundTasks(void)
	{
		FScopeLock sl(TaskSync);
		// Kick off any background tasks before returning
		if (PendingBackgroundTasks.Num() > 0)
		{
			for (INT Index = 0; Index < PendingBackgroundTasks.Num(); Index++)
			{
				// Fire and forget on the tasks
				ThreadPool.AddQueuedWork(PendingBackgroundTasks(Index));
			}
			PendingBackgroundTasks.Empty(PendingBackgroundTasks.Num());
		}
	}
};

#endif	// XBOX

#if _MSC_VER && !XBOX // Windows only hack to bypass PhysX installation req
/** Forward decl of the hack function. See bottom of file for impl */
static void DoPhysXRegistryHack(void);
#endif

#endif // WITH_NOVODEX



#if WITH_NOVODEX
	/** Used to access the PhysX version without exposing the define */
	INT GCurrentPhysXVersion = NX_PHYSICS_SDK_VERSION;
	// Touch this value if something in UE3 related to physics changes
	BYTE GCurrentEpicPhysDataVersion = 13;
	// This value will auto-update as the SDK changes
	INT	GCurrentCachedPhysDataVersion = (GCurrentPhysXVersion | GCurrentEpicPhysDataVersion);
#endif // WITH_NOVODEX



void InitGameRBPhys()
{
#if WITH_NOVODEX
	// Read physics time stepping parameters from ini.
	verify( GConfig->GetFloat(	TEXT("Engine.Physics"), TEXT("NxTimeStep"		), NxTIMESTEP,		GEngineIni ) );
	verify( GConfig->GetFloat(	TEXT("Engine.Physics"), TEXT("NxMaxDeltaTime"	), NxMAXDELTATIME,	GEngineIni ) );
	verify( GConfig->GetInt(	TEXT("Engine.Physics"), TEXT("NxMaxSubSteps"	), NxMAXSUBSTEPS,	GEngineIni ) );
	
	verify( GConfig->GetBool(	TEXT("Engine.Physics"), TEXT("NxRigidBodyCmpFixedTimeStep"	), NxRIGIDBODYCMPFIXEDTIMESTEP,	GEngineIni ) );
	verify( GConfig->GetFloat(	TEXT("Engine.Physics"), TEXT("NxRigidBodyCmpTimeStep"		), NxRIGIDBODYCMPTIMESTEP,		GEngineIni ) );
	verify( GConfig->GetFloat(	TEXT("Engine.Physics"), TEXT("NxRigidBodyCmpMaxDeltaTime"	), NxRIGIDBODYCMPMAXDELTATIME,	GEngineIni ) );
	verify( GConfig->GetInt(	TEXT("Engine.Physics"), TEXT("NxRigidBodyCmpMaxSubSteps"	), NxRIGIDBODYCMPMAXSUBSTEPS,	GEngineIni ) );
	
	verify( GConfig->GetBool(	TEXT("Engine.Physics"), TEXT("NxFluidCmpFixedTimeStep"		), NxFLUIDCMPFIXEDTIMESTEP,		GEngineIni ) );
	verify( GConfig->GetFloat(	TEXT("Engine.Physics"), TEXT("NxFluidCmpTimeStep"			), NxFLUIDCMPTIMESTEP,			GEngineIni ) );
	verify( GConfig->GetFloat(	TEXT("Engine.Physics"), TEXT("NxFluidCmpMaxDeltaTime"		), NxFLUIDCMPMAXDELTATIME,		GEngineIni ) );
	verify( GConfig->GetInt(	TEXT("Engine.Physics"), TEXT("NxFluidCmpMaxSubSteps"		), NxFLUIDCMPMAXSUBSTEPS,		GEngineIni ) );
	
	verify( GConfig->GetBool(	TEXT("Engine.Physics"), TEXT("NxClothCmpFixedTimeStep"		), NxCLOTHCMPFIXEDTIMESTEP,		GEngineIni ) );
	verify( GConfig->GetFloat(	TEXT("Engine.Physics"), TEXT("NxClothCmpTimeStep"			), NxCLOTHCMPTIMESTEP,			GEngineIni ) );
	verify( GConfig->GetFloat(	TEXT("Engine.Physics"), TEXT("NxClothCmpMaxDeltaTime"		), NxCLOTHCMPMAXDELTATIME,		GEngineIni ) );
	verify( GConfig->GetInt(	TEXT("Engine.Physics"), TEXT("NxClothCmpMaxSubSteps"		), NxCLOTHCMPMAXSUBSTEPS,		GEngineIni ) );

#if _MSC_VER && !XBOX // Windows only hack to bypass PhysX installation req
	// Bypass the PhysX installation requirement
	DoPhysXRegistryHack();
#endif

	FNxAllocator* Allocator = new FNxAllocator();
	FNxOutputStream* Stream = new FNxOutputStream();

#if USE_QUICKLOAD_CONVEX
	NxQuickLoadAllocator* QLAllocator = new NxQuickLoadAllocator( *Allocator );
	GNovodexSDK = NxCreatePhysicsSDK(GCurrentPhysXVersion, QLAllocator, Stream);
	NxQuickLoadInit( GNovodexSDK );
#else
	GNovodexSDK = NxCreatePhysicsSDK(GCurrentPhysXVersion, Allocator, Stream);
#endif

	check(GNovodexSDK && TEXT("https://udn.epicgames.com/Three/DevelopingOnVista64bit")); //@todo ship

#if PS3
	// Specify 1-15 per SPU (0 to not use that SPU).
	BYTE SPUPriorities[8] = { SPU_PRIO_PHYSX,SPU_PRIO_PHYSX,SPU_PRIO_PHYSX,SPU_PRIO_PHYSX,SPU_PRIO_PHYSX,SPU_PRIO_PHYSX,SPU_PRIO_PHYSX,SPU_PRIO_PHYSX };
	NxCellSpursControl::initWithSpurs( GSPURS, SPU_NUM_PHYSX, SPUPriorities );
#endif

	// Set parameters

	// Set the extra thickness we will use, to make more stable contact generation.
	GNovodexSDK->setParameter(NX_SKIN_WIDTH, PhysSkinWidth);

	// support creating meshes while scenes are running.
	GNovodexSDK->setParameter(NX_ASYNCHRONOUS_MESH_CREATION, 1);

#ifdef ENABLE_CCD
	// Turn on CCD.
	GNovodexSDK->setParameter(NX_CONTINUOUS_CD, 1);
#endif

	// 
	//GNovodexSDK->setParameter(NX_ADAPTIVE_FORCE, 0);

	// Init the cooker. Sets params to defaults.
	NxInitCooking(&GNovodexSDK->getFoundationSDK().getAllocator(), Stream);
	
	// Set skin thickness for cooking to be the same as the one above.
	const NxCookingParams& Params = NxGetCookingParams();
	NxCookingParams NewParams = Params;
	NewParams.skinWidth = PhysSkinWidth;
	NxSetCookingParams(NewParams);

#if STATS
	// Make sure that these stats exist so we can write to them during the physics ticking.
	{
		FCycleStat* RootStat = GStatManager.GetCurrentStat();
		GStatManager.GetChildStat(RootStat, STAT_RBBroadphaseUpdate);
		GStatManager.GetChildStat(RootStat, STAT_RBBroadphaseGetPairs);
		GStatManager.GetChildStat(RootStat, STAT_RBNearphase);
		GStatManager.GetChildStat(RootStat, STAT_RBSolver);
		GStatManager.GetChildStat(RootStat, STAT_NovodexAllocatorTime);
	}
#endif // STATS

#endif // WITH_NOVODEX
}

void DestroyGameRBPhys()
{
#if WITH_NOVODEX
	NxCloseCooking();

	if( GNovodexSDK )
	{
		NxReleasePhysicsSDK(GNovodexSDK);
		GNovodexSDK = NULL;
	}

#if !CONSOLE
	// Cleanup mem from XML dumping.
	//NXU::releasePersistentMemory();
#endif // CONSOLE

#endif
}

/** Change the global physics-data cooking mode to cook to Xenon target. */
void SetPhysCookingXenon()
{
#if WITH_NOVODEX
	const NxCookingParams& Params = NxGetCookingParams();
	NxCookingParams NewParams = Params;
	NewParams.targetPlatform = PLATFORM_XENON;
	NxSetCookingParams(NewParams);
#endif
}

/** Change the global physics-data cooking mode to cook to PS3 target. */
void SetPhysCookingPS3()
{
#if WITH_NOVODEX
	const NxCookingParams& Params = NxGetCookingParams();
	NxCookingParams NewParams = Params;
	NewParams.targetPlatform = PLATFORM_PLAYSTATION3;
	NxSetCookingParams(NewParams);
#endif
}

/** Utility to determine the endian-ness of a set of cooked physics data. */
ECookedPhysicsDataEndianess GetCookedPhysDataEndianess(const TArray<BYTE>& InData)
{
	if(InData.Num() < 4)
	{
		return CPDE_Unknown;
	}
	else
	{
		// In the Novodex format, the 4th byte is 1 if data is little endian.
		if(InData(3) & 1)
		{
			return CPDE_LittleEndian;
		}
		else
		{
			return CPDE_BigEndian;
		}
	}
}

#if WITH_NOVODEX
/** Interface for receiving async line check results from the physics engine SceneQuery. */
class FNxSceneQueryReport : public NxSceneQueryReport
{
	virtual	NxQueryReportResult	onBooleanQuery(void* userData, bool result)
	{
		check(userData);
		FAsyncLineCheckResult* Result = (FAsyncLineCheckResult*)userData;
		check(Result->bCheckStarted);
		Result->bHit = result;
		Result->bCheckCompleted = TRUE;
		return NX_SQR_ABORT_QUERY;
	}

	virtual	NxQueryReportResult	onRaycastQuery(void* userData, NxU32 nbHits, const NxRaycastHit* hits)
	{
		appErrorf(TEXT("onRaycastQuery - NOT SUPPORTED"));
		return NX_SQR_CONTINUE;
	}

	virtual	NxQueryReportResult	onShapeQuery(void* userData, NxU32 nbHits, NxShape** hits)
	{
		appErrorf(TEXT("onShapeQuery - NOT SUPPORTED"));
		return NX_SQR_CONTINUE;
	}

	virtual	NxQueryReportResult	onSweepQuery(void* userData, NxU32 nbHits, NxSweepQueryHit* hits)
	{
		appErrorf(TEXT("onSweepQuery - NOT SUPPORTED"));
		return NX_SQR_CONTINUE;
	}
};

/** SceneQuery async line check report object. */
static FNxSceneQueryReport nSceneQueryReportObject;
#endif // WITH_NOVODEX

///////////////// RBPhysScene /////////////////

static INT NovodexSceneCount = 1;

/** Exposes creation of physics-engine scene outside Engine (for use with PhAT for example). */
FRBPhysScene* CreateRBPhysScene(const FVector& Gravity)
{
#if WITH_NOVODEX
	NxVec3 nGravity = U2NPosition(Gravity);

	NxSceneDesc SceneDesc;
	SceneDesc.gravity = nGravity;
	SceneDesc.maxTimestep = NxTIMESTEP;
	SceneDesc.maxIter = NxMAXSUBSTEPS;
	SceneDesc.timeStepMethod = NX_TIMESTEP_FIXED;
#if !XBOX
	// On non-Xbox platforms allow PhysX to handle threading
	SceneDesc.flags = NX_SF_SIMULATE_SEPARATE_THREAD;
#else

	// Indicate we'll use our own thread
	SceneDesc.flags = NX_SF_SIMULATE_SEPARATE_THREAD;
	//On 360 simThreadMask chooses the HW thread to run on.
	SceneDesc.simThreadMask = 1 << PHYSICS_HWTHREAD; 
#endif

	SceneDesc.userContactReport = &nContactReportObject; // See the GNovodexSDK->setActorGroupPairFlags call below for which object will cause notification.
	SceneDesc.userNotify = &nNotifyObject;
	SceneDesc.userContactModify = &nContactModifyObject; // See the GNovodexSDK->setActorGroupPairFlags

	UBOOL bNxPrimarySceneHW = FALSE;
 	verify( GConfig->GetBool( TEXT("Engine.Physics"), TEXT("NxPrimarySceneHW"), bNxPrimarySceneHW, GEngineIni ) );
 	if( bNxPrimarySceneHW && IsPhysXHardwarePresent() )
   	{
 		SceneDesc.simType = NX_SIMULATION_HW;
 		debugf( TEXT("Primary PhysX scene will be in hardware.") );
   	}
   	else
   	{
 		SceneDesc.simType = NX_SIMULATION_SW;
 		debugf( TEXT("Primary PhysX scene will be in software.") );
   	}

	// Create FRBPhysScene container
	FRBPhysScene* NewRBPhysScene = new FRBPhysScene();	// moved this, now done before physics scene creation (so we may loop)
	
	NxScenePair ScenePair;
	
	// Create the primary scene.
	ScenePair.PrimaryScene = 0;
	NewRBPhysScene->UsingBufferedScene = FALSE;
	NewRBPhysScene->CompartmentsRunning = FALSE;
	#if SUPPORT_DOUBLE_BUFFERING
	if(GNumHardwareThreads >= 2 || IsPhysXHardwarePresent())
	{
		debugf( TEXT("Creating Double Buffered Primary PhysX Scene.") );
		ScenePair.PrimaryScene = NxdScene::create(*GNovodexSDK, SceneDesc);
		check(ScenePair.PrimaryScene);
		if(ScenePair.PrimaryScene)
		{
			NewRBPhysScene->UsingBufferedScene = TRUE;
		}
	}
	#endif
	if(!ScenePair.PrimaryScene)
	{
		debugf( TEXT("Creating Primary PhysX Scene.") );
		ScenePair.PrimaryScene = GNovodexSDK->createScene(SceneDesc);
		check(ScenePair.PrimaryScene);
	}
	
	// Create the async compartment.
	if(ScenePair.PrimaryScene)
	{
		NxCompartmentDesc CompartmentDesc;
		if(IsPhysXHardwarePresent())
		{
			CompartmentDesc.deviceCode = NX_DC_PPU_AUTO_ASSIGN;
		}
		else
		{
			CompartmentDesc.deviceCode = NX_DC_CPU;
		}
		// TODO: read this values in from an INI file or something.
		CompartmentDesc.gridHashCellSize = 20.0f;
		CompartmentDesc.gridHashTablePower = 8;
		CompartmentDesc.flags = NX_CF_INHERIT_SETTINGS;
		CompartmentDesc.timeScale = 1.0f;
		
#if defined(WIN32)
		CompartmentDesc.type = NX_SCT_RIGIDBODY;
		// JDHACK: Rigid-body compartments always in software due to it taking 75 megs of EMU memory!
		NxU32 OldAndBusted = CompartmentDesc.deviceCode;
		CompartmentDesc.deviceCode = NX_DC_CPU;
		ScenePair.RigidBodyCompartment = ScenePair.PrimaryScene->createCompartment(CompartmentDesc);
		CompartmentDesc.deviceCode = OldAndBusted;
		check(ScenePair.RigidBodyCompartment);
#endif
		
#if !defined(NX_DISABLE_FLUIDS)
		CompartmentDesc.type = NX_SCT_FLUID;
		ScenePair.FluidCompartment = ScenePair.PrimaryScene->createCompartment(CompartmentDesc);
		check(ScenePair.FluidCompartment);
#endif

#if !XBOX // compartments not supported on xenon yet
		CompartmentDesc.type = NX_SCT_CLOTH;
		CompartmentDesc.isValid();
		ScenePair.ClothCompartment = ScenePair.PrimaryScene->createCompartment(CompartmentDesc);
		//check(ScenePair.ClothCompartment); // No CPU Cloth Compartments yet...
#endif	

#if !CONSOLE // No soft bodies on console
		CompartmentDesc.type = NX_SCT_SOFTBODY;
		ScenePair.SoftBodyCompartment = ScenePair.PrimaryScene->createCompartment(CompartmentDesc);
		//check(ScenePair.SoftBodyCompartment); // No CPU Cloth Compartments yet...
#endif
	}

	NxScene *NovodexScene = ScenePair.PrimaryScene;
	
#if PS3
	// PS3 specific PhysX settings.

	// defaults that can be overridden
	UBOOL bUseSPUBroad = FALSE;
	UBOOL bUseSPUNarrow = TRUE;
	UBOOL bUseSPUMid = FALSE;
	UBOOL bUseSPUIslandGen = TRUE;
	UBOOL bUseSPUDynamics = TRUE;
	UBOOL bUseSPUCloth = TRUE;
	UBOOL bUseSPUHeightField = TRUE;
	UBOOL bUseSPURaycast = TRUE;

	// look for overrides on commandline
	ParseUBOOL(appCmdLine(), TEXT("spubroad="), bUseSPUBroad);
	ParseUBOOL(appCmdLine(), TEXT("spunarrow="), bUseSPUNarrow);
	ParseUBOOL(appCmdLine(), TEXT("spumid="), bUseSPUMid);
	ParseUBOOL(appCmdLine(), TEXT("spuisland="), bUseSPUIslandGen);
	ParseUBOOL(appCmdLine(), TEXT("spudynamics="), bUseSPUDynamics);
	ParseUBOOL(appCmdLine(), TEXT("spucloth="), bUseSPUCloth);
	ParseUBOOL(appCmdLine(), TEXT("spuheightfield="), bUseSPUHeightField);
	ParseUBOOL(appCmdLine(), TEXT("spuraycast="), bUseSPURaycast);

	if (ParseParam(appCmdLine(),TEXT("SPUNoPhysics")))
	{
		bUseSPUBroad = bUseSPUNarrow = bUseSPUMid = bUseSPUIslandGen = bUseSPUDynamics = bUseSPUCloth = bUseSPUHeightField = bUseSPURaycast = FALSE;
	}

	// disable SPU usage if desired
	if (!bUseSPUBroad)
	{
		debugf(TEXT("PhysX: DISABLING Broadphase SPU usage"));
		NxCellConfig::setSceneParamInt(NovodexScene, NxCellConfig::NX_CELL_SCENE_PARAM_SPU_BROADPHASE, 0);
	}

	if (!bUseSPUNarrow)
	{
		debugf(TEXT("PhysX: DISABLING Narrowphase SPU usage"));
		NxCellConfig::setSceneParamInt(NovodexScene, NxCellConfig::NX_CELL_SCENE_PARAM_SPU_NARROWPHASE, 0);
	}

	if (!bUseSPUMid)
	{
		debugf(TEXT("PhysX: DISABLING Midphase SPU usage"));
		NxCellConfig::setSceneParamInt(NovodexScene, NxCellConfig::NX_CELL_SCENE_PARAM_SPU_MIDPHASE, 0);
	}

	if (!bUseSPUIslandGen)
	{
		debugf(TEXT("PhysX: DISABLING Island Gen SPU usage"));
		NxCellConfig::setSceneParamInt(NovodexScene, NxCellConfig::NX_CELL_SCENE_PARAM_SPU_ISLAND_GEN, 0);
	}

	if (!bUseSPUDynamics)
	{
		debugf(TEXT("PhysX: DISABLING Dynamics SPU usage"));
		NxCellConfig::setSceneParamInt(NovodexScene, NxCellConfig::NX_CELL_SCENE_PARAM_SPU_DYNAMICS, 0);
	}

	if (!bUseSPUCloth)
	{
//		debugf(TEXT("PhysX: DISABLING Cloth SPU usage"));
//		NxCellConfig::setSceneParamInt(NovodexScene, NxCellConfig::NX_CELL_SCENE_PARAM_SPU_CLOTH, 0);
	}

	if (!bUseSPUHeightField)
	{
		debugf(TEXT("PhysX: DISABLING HeightField SPU usage"));
		NxCellConfig::setSceneParamInt(NovodexScene, NxCellConfig::NX_CELL_SCENE_PARAM_SPU_HEIGHT_FIELD, 0);
	}

	if (!bUseSPURaycast)
	{
		debugf(TEXT("PhysX: DISABLING Raycast SPU usage"));
		NxCellConfig::setSceneParamInt(NovodexScene, NxCellConfig::NX_CELL_SCENE_PARAM_SPU_RAYCAST, 0);
	}

#endif // PS3

	// Use userData pointer in NovodexScene to store reference to this FRBPhysScene
	NovodexScene->userData = NewRBPhysScene;

	// Notify when anything in the 'notify collide' group touches anything.
	// Fire 'modify contact' callback when anything touches that group.
	NovodexScene->setActorGroupPairFlags(UNX_GROUP_DEFAULT,			UNX_GROUP_NOTIFYCOLLIDE,	NX_NOTIFY_FORCES | NX_NOTIFY_ON_TOUCH | NX_NOTIFY_ON_START_TOUCH);
	NovodexScene->setActorGroupPairFlags(UNX_GROUP_NOTIFYCOLLIDE,	UNX_GROUP_NOTIFYCOLLIDE,	NX_NOTIFY_FORCES | NX_NOTIFY_ON_TOUCH | NX_NOTIFY_ON_START_TOUCH);
	NovodexScene->setActorGroupPairFlags(UNX_GROUP_MODIFYCONTACT,	UNX_GROUP_NOTIFYCOLLIDE,	NX_NOTIFY_FORCES | NX_NOTIFY_ON_TOUCH | NX_NOTIFY_ON_START_TOUCH | NX_NOTIFY_CONTACT_MODIFICATION);
	NovodexScene->setActorGroupPairFlags(UNX_GROUP_MODIFYCONTACT,	UNX_GROUP_DEFAULT,			NX_NOTIFY_CONTACT_MODIFICATION);
	NovodexScene->setActorGroupPairFlags(UNX_GROUP_MODIFYCONTACT,	UNX_GROUP_MODIFYCONTACT,	NX_NOTIFY_CONTACT_MODIFICATION);

	// Set up filtering
	NovodexScene->setFilterOps(NX_FILTEROP_OR, NX_FILTEROP_OR, NX_FILTEROP_SWAP_AND);
	NovodexScene->setFilterBool(true);  

	NxGroupsMask zeroMask;    
	zeroMask.bits0=zeroMask.bits1=zeroMask.bits2=zeroMask.bits3=0;    
	NovodexScene->setFilterConstant0(zeroMask);    
	NovodexScene->setFilterConstant1(zeroMask);

	FLOAT NxSceneInitialTimeStep = 0.0f;
	if( GConfig->GetFloat( TEXT("Engine.Physics"), TEXT("NxSceneInitialTimeStep"), NxSceneInitialTimeStep, GEngineIni ) )
	{
		if( NxSceneInitialTimeStep > 0.0f )
		{
			FLOAT NxPrimaryInitialTimeStep = NxSceneInitialTimeStep;
			FLOAT NxRigidBodyCmpInitialTimeStep = NxSceneInitialTimeStep;
			FLOAT NxFluidCmpInitialTimeStep = NxSceneInitialTimeStep;
			FLOAT NxClothCmpInitialTimeStep = NxSceneInitialTimeStep;
			FLOAT NxSoftBodyCmpInitialTimeStep = NxSceneInitialTimeStep;
			GConfig->GetFloat( TEXT("Engine.Physics"), TEXT("NxPrimaryInitialTimeStep"), NxPrimaryInitialTimeStep, GEngineIni );
			GConfig->GetFloat( TEXT("Engine.Physics"), TEXT("NxRigidBodyCmpInitialTimeStep"), NxRigidBodyCmpInitialTimeStep, GEngineIni );
			GConfig->GetFloat( TEXT("Engine.Physics"), TEXT("NxFluidCmpInitialTimeStep"), NxFluidCmpInitialTimeStep, GEngineIni );
			GConfig->GetFloat( TEXT("Engine.Physics"), TEXT("NxClothCmpInitialTimeStep"), NxClothCmpInitialTimeStep, GEngineIni );
			GConfig->GetFloat( TEXT("Engine.Physics"), TEXT("NxSoftBodyCmpInitialTimeStep"), NxSoftBodyCmpInitialTimeStep, GEngineIni );
			NovodexScene->setTiming( NxPrimaryInitialTimeStep );
			if( ScenePair.RigidBodyCompartment )
			{
				ScenePair.RigidBodyCompartment->setTiming( NxRigidBodyCmpInitialTimeStep );
			}
			if( ScenePair.FluidCompartment )
			{
				ScenePair.FluidCompartment->setTiming( NxFluidCmpInitialTimeStep );
			}
			if( ScenePair.ClothCompartment )
			{
				ScenePair.ClothCompartment->setTiming( NxClothCmpInitialTimeStep );
			}
			if( ScenePair.SoftBodyCompartment )
			{
				ScenePair.SoftBodyCompartment->setTiming( NxSoftBodyCmpInitialTimeStep );
			}
			SCOPE_CYCLE_COUNTER(STAT_RBTotalDynamicsTime);
			NovodexScene->simulate( NxSceneInitialTimeStep );
			NovodexScene->fetchResults( NX_ALL_FINISHED, TRUE );
		}
	}

	// Set up async line check query structure.
	NxSceneQueryDesc QueryDesc;
	QueryDesc.executeMode = NX_SQE_ASYNCHRONOUS;
	QueryDesc.report = &nSceneQueryReportObject;
	ScenePair.SceneQuery = NovodexScene->createSceneQuery(QueryDesc);

	//ScenePair

	// Add to map
	GNovodexSceneMap.Set(NovodexSceneCount, ScenePair);

	// Store index of NovodexScene in FRBPhysScene
	NewRBPhysScene->NovodexSceneIndex = NovodexSceneCount;

	// Increment scene count
	NovodexSceneCount++;

	return NewRBPhysScene;
#else
	return NULL;
#endif // WITH_NOVODEX
}

/** Exposes destruction of physics-engine scene outside Engine. */
void DestroyRBPhysScene(FRBPhysScene* Scene)
{
#if WITH_NOVODEX
	if(Scene && Scene->CompartmentsRunning)
	{
		NxScene *NovodexScene = Scene->GetNovodexPrimaryScene();
		check(NovodexScene);
		if(NovodexScene)
		{
			WaitForNovodexScene(*NovodexScene);
		}
		Scene->CompartmentsRunning = FALSE;
	}
	{
		SCOPE_CYCLE_COUNTER(STAT_PhysicsFluidMeshEmitterUpdate)
		
		// make sure we kill the physical emitters before the scene...
		TArray<FPhysXEmitterInstance*> PhysicalEmitters = Scene->PhysicalEmitters;
		INT NumEmitters = PhysicalEmitters.Num();
		for(INT i=0; i<NumEmitters; i++)
		{
			FPhysXEmitterInstance *EmitterInstance = PhysicalEmitters(i);
			EmitterInstance->RemovedFromScene(); // signal destruction.
			EmitterInstance->SyncPhysicsData();  // perform the actual destruction.
		}
	}

	NxScenePair *ScenePair = GetNovodexScenePairFromIndex(Scene->NovodexSceneIndex);
	if(ScenePair)
	{
		NxScene *PrimaryScene = ScenePair->PrimaryScene;
		
		for(INT i=GNovodexPendingKillCloths.Num()-1; i>=0; i--)
		{
			NxCloth* nCloth = GNovodexPendingKillCloths(i);
			check(nCloth);
			
			NxScene& nxScene = nCloth->getScene();

			if(&nxScene==PrimaryScene)
				GNovodexPendingKillCloths.Remove(i);
		}

#ifndef NX_DISABLE_FLUIDS		
		for(INT i=GNovodexPendingKillFluids.Num()-1; i>=0; i--)
		{
			NxFluid* nFluid = GNovodexPendingKillFluids(i);
			check(nFluid);
			
			NxScene& nxScene = nFluid->getScene();

			if(&nxScene==PrimaryScene)
				GNovodexPendingKillFluids.Remove(i);
		}
#endif //NX_DISABLE_FLUIDS		

		// Release SceneQuery object.
		PrimaryScene->releaseSceneQuery(*ScenePair->SceneQuery);
		ScenePair->SceneQuery = NULL;

		#if SUPPORT_DOUBLE_BUFFERING
		if(PrimaryScene && Scene->UsingBufferedScene)
		{
			NxdScene::release((NxdScene*)PrimaryScene);
			PrimaryScene = 0;
		}
		#endif
		if(PrimaryScene)
		{
			GNovodexSDK->releaseScene(*PrimaryScene);
		}
	}

	// Clear the entry to the scene in the map
	NxScenePair blank;
	GNovodexSceneMap.Set(Scene->NovodexSceneIndex, blank);

	// Delete the FRBPhysScene container object.
	delete Scene;
#endif // WITH_NOVODEX
}

/** Executes performanec statistics code for physics */
static void StatRBPhysScene(FRBPhysScene* Scene)
{
	SCOPE_CYCLE_COUNTER(STAT_PhysicsOutputStats);

#if WITH_NOVODEX
	NxScene *NovodexScene = 0;
	if( Scene )
	{
		NovodexScene = Scene->GetNovodexPrimaryScene();
	}
	if( !NovodexScene )
	{
		return;
   	}

#if STATS
	if( !NovodexScene )
	{
		return;
   	}

	// Only do stats when the game is actually running (this includes PIE though)
	if(GIsGame)
	{
		//GStatChart->AddDataPoint( FString(TEXT("PhysFetch")), (FLOAT)(GPhysicsStats.RBKickOffDynamicsTime.Value * GSecondsPerCycle * 1000.0) );

		// Show the average substep time as well.
		const FCycleStat* Total = GStatManager.GetCycleStat(STAT_RBTotalDynamicsTime);
		DWORD TotalCycles = Total ? Total->Cycles : 0;
		INT NumSubSteps = Scene->NumSubSteps > 0 ? Scene->NumSubSteps : 1;
		SET_CYCLE_COUNTER(STAT_RBSubstepTime, TotalCycles/NumSubSteps, 1);

#if LOOKING_FOR_PERF_ISSUES
		FLOAT PhysTimeMs = TotalCycles * GSecondsPerCycle * 1000;

		// Log detailed stats for any frame with physics times > 10 ms
		if( PhysTimeMs > 10.f )
		{
			debugf(TEXT("SLOW PHYSICS FRAME: %f ms"), PhysTimeMs);
			bOutputAllStats = TRUE;
		}
#endif

		// Get stats for software and, if present, hardware scene.
		NxSceneStats SWStats;
		NovodexScene->getStats(SWStats);

		if( bOutputAllStats )
		{
			debugf(NAME_PerfWarning,TEXT("---------------- Dumping all physics stats ----------------"));
			debugf(TEXT("Num Dynamic Actors: %d"), SWStats.numDynamicActors);
			debugf(TEXT("Num Awake Actors: %d"), SWStats.numDynamicActorsInAwakeGroups);
			debugf(TEXT("Solver Bodies: %d"), SWStats.numSolverBodies);
			debugf(TEXT("Num Pairs: %d"), SWStats.numPairs);
			debugf(TEXT("Num Contacts: %d"), SWStats.numContacts);
			debugf(TEXT("Num Joints: %d"), SWStats.numJoints);
		}

		SET_DWORD_STAT(STAT_TotalSWDynamicBodies,SWStats.numDynamicActors);
		SET_DWORD_STAT(STAT_AwakeSWDynamicBodies,SWStats.numDynamicActorsInAwakeGroups);
		SET_DWORD_STAT(STAT_SWSolverBodies,SWStats.numSolverBodies);
		SET_DWORD_STAT(STAT_SWNumPairs,SWStats.numPairs);
		SET_DWORD_STAT(STAT_SWNumContacts,SWStats.numContacts);
		SET_DWORD_STAT(STAT_SWNumJoints,SWStats.numJoints);
		SET_DWORD_STAT(STAT_NumConvexMeshes,GNumPhysXConvexMeshes);
		SET_DWORD_STAT(STAT_NumTriMeshes,GNumPhysXTriMeshes);
		SET_CYCLE_COUNTER(STAT_RBNearphase, 0, 0);
		SET_CYCLE_COUNTER(STAT_RBBroadphaseUpdate, 0, 0);
		SET_CYCLE_COUNTER(STAT_RBBroadphaseGetPairs, 0, 0);
		SET_CYCLE_COUNTER(STAT_RBSolver, 0, 0);
		SET_CYCLE_COUNTER(STAT_NovodexAllocatorTime, FNxAllocator::CallCycles, FNxAllocator::CallCount);
		FNxAllocator::CallCycles = 0;
		FNxAllocator::CallCount	= 0;

#ifdef NX_ENABLE_SCENE_STATS2		
		SET_DWORD_STAT( STAT_TotalFluids, FindNovodexSceneStat(NovodexScene, TEXT("TotalFluids"), FALSE) );
		SET_DWORD_STAT( STAT_TotalFluidEmitters, FindNovodexSceneStat(NovodexScene, TEXT("TotalFluidEmitters"), FALSE) );
		SET_DWORD_STAT( STAT_ActiveFluidParticles, FindNovodexSceneStat(NovodexScene, TEXT("ActiveFluidParticles"), FALSE) );
		SET_DWORD_STAT( STAT_TotalFluidParticles, FindNovodexSceneStat(NovodexScene, TEXT("TotalFluidParticles"), FALSE) );
		SET_DWORD_STAT( STAT_TotalFluidPackets, FindNovodexSceneStat(NovodexScene, TEXT("TotalFluidPackets"), FALSE) );

		SET_DWORD_STAT( STAT_TotalCloths, FindNovodexSceneStat(NovodexScene, TEXT("TotalCloths"), FALSE) );
		SET_DWORD_STAT( STAT_ActiveCloths, FindNovodexSceneStat(NovodexScene, TEXT("ActiveCloths"), FALSE) );
		SET_DWORD_STAT( STAT_ActiveClothVertices, FindNovodexSceneStat(NovodexScene, TEXT("ActiveClothVertices"), FALSE) );
		SET_DWORD_STAT( STAT_TotalClothVertices, FindNovodexSceneStat(NovodexScene, TEXT("TotalClothVertices"), FALSE) );
		SET_DWORD_STAT( STAT_ActiveAttachedClothVertices, FindNovodexSceneStat(NovodexScene, TEXT("ActiveAttachedClothVertices"), FALSE) );
		SET_DWORD_STAT( STAT_TotalAttachedClothVertices, FindNovodexSceneStat(NovodexScene, TEXT("TotalAttachedClothVertices"), FALSE) );

#endif //NX_ENABLE_SCENE_STATS2

#if !PS3
		const NxProfileData* nProfileData = NovodexScene->readProfileData(true);
		if (nProfileData != NULL)
		{		
			for(UINT i=0; i<nProfileData->numZones; i++)
			{
				NxProfileZone* nZone = nProfileData->profileZones + i;
				FANSIToTCHAR ZoneName(nZone->name);

				FCycleStat* Stat = NULL;
				if( appStrcmp(TEXT("PrBroadphase_NarrowPhase"), ZoneName) == 0 )
				{
					Stat = (FCycleStat*)GStatManager.GetCycleStat(STAT_RBNearphase);
				}
				else if( appStrcmp(TEXT("PrBroadphase_Update"), ZoneName) == 0 )
				{
					Stat = (FCycleStat*)GStatManager.GetCycleStat(STAT_RBBroadphaseUpdate);
				}
				else if( appStrcmp(TEXT("PrBroadphase_GetPairs"), ZoneName) == 0 )
				{
					Stat = (FCycleStat*)GStatManager.GetCycleStat(STAT_RBBroadphaseGetPairs);
				}
				else if( appStrcmp(TEXT("PrSolver"), ZoneName) == 0 )
				{
					Stat = (FCycleStat*)GStatManager.GetCycleStat(STAT_RBSolver);
				}

				if( Stat )
				{
					Stat->NumCallsPerFrame += nZone->callCount;
					Stat->Cycles += (DWORD) (((DOUBLE)nZone->hierTime) / (1000000.0 * GSecondsPerCycle));
				}

				if( bOutputAllStats && (nZone->hierTime > 1.f) )
				{
					// Don't output stats which are zero.
					if(nZone->hierTime > 1.f)
					{
						debugf(NAME_PerfWarning, TEXT("%d %d %s T: %6.3f ms"), i, nZone->recursionLevel, (TCHAR*)ZoneName, nZone->hierTime / 1000.f );
					}
				}
			}
		}
#endif // !CONSOLE

		bOutputAllStats = FALSE;
	}
#endif

#endif // WITH_NOVODEX
}

/** Exposes ticking of physics-engine scene outside Engine. */
void TickRBPhysScene(FRBPhysScene* Scene, FLOAT DeltaTime, FLOAT MaxSubstep, UBOOL bUseFixedTimestep)
{
#if WITH_NOVODEX
	// Render debug info.
	if(Scene->UsingBufferedScene)
	{
		Scene->AddNovodexDebugLines(GWorld->LineBatcher);
	}
	
#if !FINAL_RELEASE
	if( GStatManager.GetGroup(STATGROUP_Physics)->bShowGroup || 
		GStatManager.GetGroup(STATGROUP_PhysicsCloth)->bShowGroup ||
		GStatManager.GetGroup(STATGROUP_PhysicsCloth)->bShowGroup )
	{
		// do async safe statistics.
		StatRBPhysScene(Scene);
	}
#endif
	
	for( INT i = 0; i < Scene->PhysicalEmitters.Num(); ++i )
	{
		FPhysXEmitterInstance *EmitterInstance = Scene->PhysicalEmitters(i);
		EmitterInstance->SyncPhysicsData();
	}
	
	for(INT i = 0; i < Scene->PhysicalEmitters.Num(); i++ )
	{
		FPhysXEmitterInstance* ParticleEmitterInstance = Scene->PhysicalEmitters(i);
		if( ParticleEmitterInstance  != NULL )
		{
			ParticleEmitterInstance->ApplyParticleForces();
		}
	}
	
	INT NumSubSteps;
	if(bUseFixedTimestep)
	{
		NumSubSteps = NxMAXSUBSTEPS;
	}
	else
	{
		NumSubSteps = appCeil(DeltaTime/MaxSubstep);
		NumSubSteps = Clamp<INT>(NumSubSteps, 1, NxMAXSUBSTEPS);
		MaxSubstep = ::Clamp(DeltaTime/NumSubSteps, 0.0025f, MaxSubstep);
	}

	Scene->NumSubSteps = NumSubSteps;

	// Make sure list of queued collision notifications is empty.
	Scene->PendingCollisionNotifies.Empty();

	NxScene *NovodexScene = Scene->GetNovodexPrimaryScene();;
	check(NovodexScene);
	if(NovodexScene)
	{
		SCOPE_CYCLE_COUNTER(STAT_PhysicsKickOffDynamicsTime);

		NovodexScene->setTiming( MaxSubstep, NumSubSteps );
		NxScenePair *ScenePair = GetNovodexScenePairFromIndex(Scene->NovodexSceneIndex);
		if(ScenePair)
		{
			if(ScenePair->RigidBodyCompartment)
			{
				FLOAT MaxTimeStep = NxRIGIDBODYCMPTIMESTEP;
				INT   MaxIters    = NxRIGIDBODYCMPMAXSUBSTEPS;
				if(!NxRIGIDBODYCMPFIXEDTIMESTEP)
				{
					MaxIters    = appCeil(DeltaTime/MaxTimeStep);
					MaxIters    = Clamp<INT>(MaxIters, 1, NxRIGIDBODYCMPMAXSUBSTEPS);
					MaxTimeStep = Clamp(DeltaTime/MaxIters, 0.0025f, MaxTimeStep);
				}
				ScenePair->RigidBodyCompartment->setTiming(MaxTimeStep, MaxIters);
			}
			if(ScenePair->FluidCompartment)
			{
				FLOAT MaxTimeStep = NxFLUIDCMPTIMESTEP;
				INT   MaxIters    = NxFLUIDCMPMAXSUBSTEPS;
				if(!NxFLUIDCMPFIXEDTIMESTEP)
				{
					MaxIters    = appCeil(DeltaTime/MaxTimeStep);
					MaxIters    = Clamp<INT>(MaxIters, 1, NxFLUIDCMPMAXSUBSTEPS);
					MaxTimeStep = Clamp(DeltaTime/MaxIters, 0.0025f, MaxTimeStep);
				}
				ScenePair->FluidCompartment->setTiming(MaxTimeStep, MaxIters);
			}
			if(ScenePair->ClothCompartment)
			{
				FLOAT MaxTimeStep = NxCLOTHCMPTIMESTEP;
				INT   MaxIters    = NxCLOTHCMPMAXSUBSTEPS;
				if(!NxCLOTHCMPFIXEDTIMESTEP)
				{
					MaxIters    = appCeil(DeltaTime/MaxTimeStep);
					MaxIters    = Clamp<INT>(MaxIters, 1, NxCLOTHCMPMAXSUBSTEPS);
					MaxTimeStep = Clamp(DeltaTime/MaxIters, 0.0025f, MaxTimeStep);
				}
				ScenePair->ClothCompartment->setTiming(MaxTimeStep, MaxIters);
			}
			if(ScenePair->SoftBodyCompartment && ScenePair->SoftBodyCompartment != ScenePair->ClothCompartment)
			{
				// TODO: when we actually support soft bodies.
				ScenePair->SoftBodyCompartment->setTiming(MaxSubstep, NumSubSteps);
			}
		}
		SCOPE_CYCLE_COUNTER(STAT_RBTotalDynamicsTime);
		NovodexScene->simulate(DeltaTime);
		Scene->CompartmentsRunning = TRUE;
	}

	//GStatChart->AddDataPoint( FString(TEXT("PhysKickoff")), (FLOAT)(GPhysicsStats.RBKickOffDynamicsTime.Value * GSecondsPerCycle * 1000.0) );
#endif // WITH_NOVODEX
}


/**
 * Waits for the scene to be done processing and fires off any physics events
 */
void WaitRBPhysScene(FRBPhysScene* Scene)
{
#if WITH_NOVODEX
	NxScene *NovodexScene = Scene->GetNovodexPrimaryScene();
	check(NovodexScene);
	if(NovodexScene)
	{
		SCOPE_CYCLE_COUNTER(STAT_RBTotalDynamicsTime);
		SCOPE_CYCLE_COUNTER(STAT_PhysicsFetchDynamicsTime);

		if(Scene->UsingBufferedScene)
		{
			NovodexScene->fetchResults(NX_PRIMARY_FINISHED, true);
		}
		else
		{
			NovodexScene->fetchResults(NX_ALL_FINISHED, true);
			Scene->CompartmentsRunning = FALSE;
		}
	}

#if XBOX
	// Kick off any background physics tasks
	FPhysicsScheduler::GetInstance()->ProcessBackgroundTasks();
#endif

	// TEMP: Fire off and then block on line checks.
	Scene->BeginLineChecks();
	Scene->FinishLineChecks();

#endif // WITH_NOVODEX
}


/**
 * Waits for the scene compartments to be done processing and fires off any physics events
 */
void WaitPhysCompartments(FRBPhysScene* Scene)
{
#if WITH_NOVODEX
	if(Scene->CompartmentsRunning)
	{
		NxScene *NovodexScene = Scene->GetNovodexPrimaryScene();
		check(NovodexScene);
		if(NovodexScene)
		{
			SCOPE_CYCLE_COUNTER(STAT_RBTotalDynamicsTime);
			SCOPE_CYCLE_COUNTER(STAT_PhysicsFetchDynamicsTime);
			NovodexScene->fetchResults(NX_ALL_FINISHED, true);
		}
		Scene->CompartmentsRunning = FALSE;
	}
#endif // WITH_NOVODEX
}


/** 
 *	Call after WaitRBPhysScene to make deferred OnRigidBodyCollision calls. 
 *
 * @param RBPhysScene - the scene to process deferred collision events
 */
void DispatchRBCollisionNotifies(FRBPhysScene* RBPhysScene)
{
	SCOPE_CYCLE_COUNTER(STAT_PhysicsEventTime);

	// Fire any collision notifies in the queue.
	for(INT i=0; i<RBPhysScene->PendingCollisionNotifies.Num(); i++)
	{
		FCollisionNotifyInfo& NotifyInfo = RBPhysScene->PendingCollisionNotifies(i);
		if(NotifyInfo.RigidCollisionData.ContactInfos.Num() > 0)
		{
			if(NotifyInfo.bCallEvent0 && NotifyInfo.Info0.Actor && NotifyInfo.IsValidForNotify())
			{
				NotifyInfo.Info0.Actor->OnRigidBodyCollision(NotifyInfo.Info0, NotifyInfo.Info1, NotifyInfo.RigidCollisionData);
			}

			// Need to check IsValidForNotify again in case first call broke something.
			if(NotifyInfo.bCallEvent1 && NotifyInfo.Info1.Actor && NotifyInfo.IsValidForNotify())
			{
				NotifyInfo.Info1.Actor->OnRigidBodyCollision(NotifyInfo.Info0, NotifyInfo.Info1, NotifyInfo.RigidCollisionData);
			}
		}
	}
	RBPhysScene->PendingCollisionNotifies.Empty();

	// Fire any pushing notifications.
	for(INT i=0; i<RBPhysScene->PendingPushNotifies.Num(); i++)
	{
		FPushNotifyInfo& PushInfo = RBPhysScene->PendingPushNotifies(i);
		if(PushInfo.Pusher && !PushInfo.Pusher->bDeleteMe)
		{
			PushInfo.Pusher->ProcessPushNotify(PushInfo.PushedInfo, PushInfo.ContactInfos);
		}
	}
	RBPhysScene->PendingPushNotifies.Empty();
}

/** 
 *	Begin an async ray cast against static geometry in the level using the physics engine. 
 *	@param Origin		Starting point of ray.
 *	@param Direction	Normalized direction of the ray.
 *	@param Distance		Length of ray
 */
void FRBPhysScene::AsyncRayCheck(const FVector& Origin, const FVector& Direction, FLOAT Distance, UBOOL bDynamicObjects, FAsyncLineCheckResult* Result)
{
#if WITH_NOVODEX
	check(Result);
	if(!Result->IsReady())
	{
		debugf(TEXT("FRBPhysScene::AsyncRayCheck - Result Struct Not Ready."));
		return;
	}

	NxScenePair *ScenePair = GetNovodexScenePairFromIndex(NovodexSceneIndex);
	if(ScenePair)
	{
		check(ScenePair->SceneQuery);
		Result->bCheckStarted = TRUE;
		Result->bCheckCompleted = FALSE;
		NxRay nRay(U2NPosition(Origin), U2NVectorCopy(Direction));

		NxShapesType TestTypes = (bDynamicObjects) ? NX_ALL_SHAPES : NX_STATIC_SHAPES;

		ScenePair->SceneQuery->raycastAnyShape(nRay, TestTypes, -1, Distance*U2PScale, 0, 0, Result);
	}
#endif
}

/** 
 *	Begin an async ray cast against static geometry in the level using the physics engine. 
 *	@param Start	Start point of line
 *	@param End		End point of line
 */
void FRBPhysScene::AsyncLineCheck(const FVector& Start, const FVector& End, UBOOL bDynamicObjects, FAsyncLineCheckResult* Result)
{
	FVector Delta = End-Start;
	FLOAT Distance = Delta.Size();
	Delta /= Distance;
	AsyncRayCheck(Start, Delta, Distance, bDynamicObjects, Result);
}

/** Fires off any batched line checks.*/
void FRBPhysScene::BeginLineChecks()
{
#if WITH_NOVODEX
	NxScenePair *ScenePair = GetNovodexScenePairFromIndex(NovodexSceneIndex);
	if(ScenePair)
	{
		check(ScenePair->SceneQuery);
		// If not finished, force it to finish now.
		if(!ScenePair->SceneQuery->finish(false))
		{
			debugf(TEXT("Calling FRBPhysScene::BeginLineChecks and already running!"));
			ScenePair->SceneQuery->finish(true);
		}

		ScenePair->SceneQuery->execute();
	}
#endif
}

/** Forces all line checks to be processed now (blocking) so no FAsyncLineCheckResults are referenced by the system. */
void FRBPhysScene::FinishLineChecks()
{
#if WITH_NOVODEX
	NxScenePair *ScenePair = GetNovodexScenePairFromIndex(NovodexSceneIndex);
	if(ScenePair)
	{
		check(ScenePair->SceneQuery);
		// If not finished, force it to finish now.
		if(!ScenePair->SceneQuery->finish(false))
		{
			ScenePair->SceneQuery->finish(true);
		}
	}
#endif
}

void FRBPhysScene::SetGravity(const FVector& NewGrav)
{
#if WITH_NOVODEX
	NxVec3 nGravity = U2NPosition(NewGrav);
	NxScene* NovodexScene = GetNovodexPrimaryScene();
	if(NovodexScene)
	{
		NovodexScene->setGravity(nGravity);
	}
#endif
}

UINT FRBPhysScene::FindPhysMaterialIndex(UPhysicalMaterial* PhysMat)
{
#if WITH_NOVODEX
	// Find the name of the PhysicalMaterial
	FName PhysMatName = PhysMat->GetFName();

	// Search for name in map.
	UINT* MatIndexPtr = MaterialMap.Find(PhysMatName);

	// If present, just return it.
	if(MatIndexPtr)
	{
		return *MatIndexPtr;
	}
	// If not, add to mapping.
	else
	{
		NxScene* PrimaryScene = GetNovodexPrimaryScene();
		if(!PrimaryScene)
		{
			return 0;
		}

		UINT NewMaterialIndex = 0;

		// hardware scene support - making sure both scenes have the material
		if(UnusedMaterials.Num() > 0)
		{
			NewMaterialIndex = UnusedMaterials.Pop();
		}
		else
		{
			NxMaterialDesc MatDesc;
			NxMaterial * NewMaterial = PrimaryScene->createMaterial(MatDesc);
			NewMaterialIndex = NewMaterial->getMaterialIndex();
		}

		NxMaterial * NewMaterial = PrimaryScene->getMaterialFromIndex(NewMaterialIndex);

		NewMaterial->setRestitution(PhysMat->Restitution);
		NewMaterial->setStaticFriction(PhysMat->Friction);
		NewMaterial->setDynamicFriction(PhysMat->Friction);
		NewMaterial->setFrictionCombineMode(NX_CM_MULTIPLY);
		NewMaterial->setRestitutionCombineMode(NX_CM_MAX);
		NewMaterial->userData = PhysMat;

		// Add the newly created material to the table.
		MaterialMap.Set(PhysMatName, NewMaterialIndex);

		return NewMaterialIndex;
	}
#else
	return 0;
#endif
}

IMPLEMENT_COMPARE_POINTER(UStaticMesh, UnPhysLevel, { return((B->BodySetup->CollisionGeom.Num() * B->BodySetup->AggGeom.ConvexElems.Num()) - (A->BodySetup->CollisionGeom.Num() * A->BodySetup->AggGeom.ConvexElems.Num())); } );

//// EXEC
UBOOL ExecRBCommands(const TCHAR* Cmd, FOutputDevice* Ar)
{
#if WITH_NOVODEX
	if( ParseCommand(&Cmd, TEXT("NXSTATS")) )
	{
		bOutputAllStats = TRUE;
		return TRUE;
	}
	else if(ParseCommand(&Cmd, TEXT("MESHSCALES")) )
	{
		// First make array of all USequenceOps
		TArray<UStaticMesh*> AllMeshes;
		for( TObjectIterator<UStaticMesh> It; It; ++It )
		{
			UStaticMesh* Mesh = *It;
			if(Mesh && Mesh->BodySetup)
			{
				AllMeshes.AddItem(Mesh);
			}
		}

		// Then sort them
		Sort<USE_COMPARE_POINTER(UStaticMesh, UnPhysLevel)>(&AllMeshes(0), AllMeshes.Num());

		Ar->Logf( TEXT("----- STATIC MESH SCALES ------") );
		for(INT i=0; i<AllMeshes.Num(); i++)
		{
			Ar->Logf( TEXT("%s (%d) (%d HULLS)"), *(AllMeshes(i)->GetPathName()), AllMeshes(i)->BodySetup->CollisionGeom.Num(), AllMeshes(i)->BodySetup->AggGeom.ConvexElems.Num() );
			for(INT j=0; j<AllMeshes(i)->BodySetup->CollisionGeom.Num(); j++)
			{
				const FVector& Scale3D = AllMeshes(i)->BodySetup->CollisionGeomScale3D(j);
				Ar->Logf( TEXT("  %f,%f,%f"), Scale3D.X, Scale3D.Y, Scale3D.Z);
			}
		}
		return TRUE;
	}
	else if( ParseCommand(&Cmd, TEXT("NXDUMPMEM")) )
	{
#if KEEP_TRACK_OF_NOVODEX_ALLOCATIONS && (defined _DEBUG)
		// Group allocations by file/line
		TArray<FNxAllocator::FPhysXAlloc> GroupedAllocs;

		// Iterate over all allocations
		for ( TDynamicMap<PTRINT,FNxAllocator::FPhysXAlloc>::TIterator AllocIt(FNxAllocator::AllocationToSizeMap); AllocIt; ++AllocIt )
		{
			FNxAllocator::FPhysXAlloc Alloc = AllocIt.Value();

			// See if we have this location already.
			INT EntryIndex = INDEX_NONE;
			for(INT i=0; i<GroupedAllocs.Num(); i++)
			{
				if((strcmp(GroupedAllocs(i).Filename, Alloc.Filename) == 0) && (GroupedAllocs(i).Line == Alloc.Line))
				{
					EntryIndex = i;
					break;
				}
			}

			// If we found it - update size
			if(EntryIndex != INDEX_NONE)
			{
				GroupedAllocs(EntryIndex).Count += 1;
				GroupedAllocs(EntryIndex).Size += Alloc.Size;
			}
			// If we didn't add to array.
			else
			{
				Alloc.Count = 1;
				GroupedAllocs.AddItem(Alloc);
			}
		}

		// Now print out amount allocated for each allocating location.
		for(INT AllocIndex = 0; AllocIndex < GroupedAllocs.Num(); AllocIndex++)
		{
			FNxAllocator::FPhysXAlloc& Alloc = GroupedAllocs(AllocIndex);
			debugf( TEXT("%s:%d,%d,%d"), ANSI_TO_TCHAR(Alloc.Filename), Alloc.Line, Alloc.Size, Alloc.Count );
		}
#endif
		return TRUE;
	}
#if !CONSOLE // On PC, support dumping all physics info to XML.
	else if( ParseCommand(&Cmd, TEXT("NXDUMP")) )
	{
		TCHAR File[MAX_SPRINTF]=TEXT("");
		const TCHAR* FileNameBase = TEXT("UE3_PxCoreDump_");

		if( NxDumpIndex == -1 )
		{
			for( INT TestDumpIndex=0; TestDumpIndex<65536; TestDumpIndex++ )
			{
				appSprintf( File, TEXT("%s%03i.xml"), FileNameBase, TestDumpIndex );

				if( GFileManager->FileSize(File) < 0 )
				{
					NxDumpIndex = TestDumpIndex;
					break;
				}
			}
		}

		appSprintf( File, TEXT("%s%03i"), FileNameBase, NxDumpIndex++ );

		//NXU::coreDump(GNovodexSDK, TCHAR_TO_ANSI(File), NXU::FT_XML, true, false);

		return 1;
	}
#endif // CONSOLE
	else if(!GIsUCC && GNovodexSDK && ParseCommand(&Cmd, TEXT("NXVRD")) )
	{
		NxFoundationSDK  &FoundationSDK  = GNovodexSDK->getFoundationSDK();
		NxRemoteDebugger *RemoteDebugger = FoundationSDK.getRemoteDebugger();
		if(RemoteDebugger && ParseCommand(&Cmd, TEXT("CONNECT")))
		{
			if(RemoteDebugger->isConnected())
			{
				RemoteDebugger->disconnect();
			}
			if(*Cmd)
			{
				FTCHARToANSI_Convert ToAnsi;
				ANSICHAR *AnsiCmd = ToAnsi.Convert(Cmd, 0, 0);
				check(AnsiCmd);
				if(AnsiCmd)
				{
					RemoteDebugger->connect(AnsiCmd);
				}
			}
			else
			{
				RemoteDebugger->connect("localhost");
			}
		}
		else if(RemoteDebugger && ParseCommand(&Cmd, TEXT("DISCONNECT")))
		{
			RemoteDebugger->disconnect();
		}
		return TRUE;
	}
	else if( ParseCommand(&Cmd, TEXT("NXVIS")) )
	{
		struct { const TCHAR* Name; NxParameter Flag; FLOAT Size; } Flags[] =
		{
			// Axes
			{ TEXT("WORLDAXES"),			NX_VISUALIZE_WORLD_AXES,		1.f },
			{ TEXT("BODYAXES"),				NX_VISUALIZE_BODY_AXES,			1.f },
			{ TEXT("MASSAXES"),             NX_VISUALIZE_BODY_MASS_AXES,    1.f },

			// Contacts
			{ TEXT("CONTACTPOINT"),			NX_VISUALIZE_CONTACT_POINT,		1.f },
			{ TEXT("CONTACTS"),				NX_VISUALIZE_CONTACT_NORMAL,	1.f },
			{ TEXT("CONTACTERROR"),			NX_VISUALIZE_CONTACT_ERROR,		100.f },
			{ TEXT("CONTACTFORCE"),			NX_VISUALIZE_CONTACT_FORCE,		1.f },

			// Joints
			{ TEXT("JOINTLIMITS"),			NX_VISUALIZE_JOINT_LIMITS,		1.f },
			{ TEXT("JOINTLOCALAXES"),		NX_VISUALIZE_JOINT_LOCAL_AXES,	1.f },
			{ TEXT("JOINTWORLDAXES"),		NX_VISUALIZE_JOINT_WORLD_AXES,	1.f },

			// Collision
			{ TEXT("CCD"),					NX_VISUALIZE_COLLISION_SKELETONS,	1.f },
			{ TEXT("CCDTESTS"),				NX_VISUALIZE_COLLISION_CCD,			1.f },
			{ TEXT("COLLISIONAABBS"),		NX_VISUALIZE_COLLISION_AABBS,		1.f },
			{ TEXT("COLLISION"),			NX_VISUALIZE_COLLISION_SHAPES,		1.f },
			{ TEXT("COLLISIONAXES"),		NX_VISUALIZE_COLLISION_AXES,		1.f },

			//Cloth
			{ TEXT("CLOTH_MESH"),			NX_VISUALIZE_CLOTH_MESH,			1.f },
			{ TEXT("CLOTH_COLLISIONS"),		NX_VISUALIZE_CLOTH_COLLISIONS,		1.f },
			{ TEXT("CLOTH_ATTACHMENT"),		NX_VISUALIZE_CLOTH_ATTACHMENT,		1.f },
			{ TEXT("CLOTH_SLEEP"),			NX_VISUALIZE_CLOTH_SLEEP,			1.f },

			//Fluid
			{ TEXT("FLUID_PACKETS"),		NX_VISUALIZE_FLUID_PACKETS,			1.f },
			{ TEXT("FLUID_POSITION"),		NX_VISUALIZE_FLUID_POSITION,		1.f },
			{ TEXT("FLUID_MESH_PACKETS"),	NX_VISUALIZE_FLUID_MESH_PACKETS,	1.f },
			{ TEXT("FLUID_PACKET_DATA"),	NX_VISUALIZE_FLUID_PACKET_DATA,		1.f },
			{ TEXT("FLUID_BOUNDS"),			NX_VISUALIZE_FLUID_BOUNDS,			1.f },
			{ TEXT("FLUID_VELOCITY"),		NX_VISUALIZE_FLUID_VELOCITY,		10.f },
			{ TEXT("FLUID_KERNEL_RADIUS"),	NX_VISUALIZE_FLUID_KERNEL_RADIUS,	1.f },
			{ TEXT("FLUID_DRAINS"),			NX_VISUALIZE_FLUID_DRAINS,			1.f },
			{ TEXT("FLUID_EMITTERS"),		NX_VISUALIZE_FLUID_EMITTERS,		1.f },

			// ForceFields
			{ TEXT("FORCEFIELDS"),			NX_VISUALIZE_FORCE_FIELDS,		1.f },
		};

		// stall all scenes to make sure we don't simulate when calling setParameter
		NxU32 NbScenes = GNovodexSDK->getNbScenes();
		for (NxU32 i = 0; i < NbScenes; i++)
		{
			NxScene *nScene = GNovodexSDK->getScene(i);
			WaitForNovodexScene(*nScene);
		}

		UBOOL bDebuggingActive = false;
		UBOOL bFoundFlag = false;
		for (int i = 0; i < ARRAY_COUNT(Flags); i++)
		{
			// Parse out the command sent in and set only those flags
			if (ParseCommand(&Cmd, Flags[i].Name))
			{
				NxReal Result = GNovodexSDK->getParameter(Flags[i].Flag);
				if (Result == 0.0f)
				{
					GNovodexSDK->setParameter(Flags[i].Flag, Flags[i].Size);
					Ar->Logf(TEXT("Flag set."));
				}
				else
				{
					GNovodexSDK->setParameter(Flags[i].Flag, 0.0f);
					Ar->Logf(TEXT("Flag un-set."));
				}

				bFoundFlag = true;
			}

			// See if any flags are true
			NxReal Result = GNovodexSDK->getParameter(Flags[i].Flag);
			if(Result > 0.f)
			{
				bDebuggingActive = true;
			}
		}

		// If no debugging going on - disable it using NX_VISUALIZATION_SCALE
		if(bDebuggingActive)
		{
			GNovodexSDK->setParameter(NX_VISUALIZATION_SCALE, 1.0f);
		}
		else
		{
			GNovodexSDK->setParameter(NX_VISUALIZATION_SCALE, 0.0f);
		}

		if(!bFoundFlag)
		{
			Ar->Logf(TEXT("Unknown Novodex visualization flag specified."));
		}

		return 1;
	}
	else if( ParseCommand(&Cmd, TEXT("DUMPAWAKE")) )
	{
		INT AwakeCount = 0;
		for( TObjectIterator<URB_BodyInstance> It; It; ++It ) 
		{
			URB_BodyInstance* BI = *It;
			if(BI && BI->GetNxActor() && !BI->GetNxActor()->isSleeping())
			{
				debugf(TEXT("-- %s:%d"), BI->OwnerComponent ? *BI->OwnerComponent->GetPathName() : TEXT("None"), BI->BodyIndex);
				AwakeCount++;
			}
		}
		debugf(TEXT("TOTAL: %d AWAKE BODIES FOUND"), AwakeCount);
		return 1;
	}

#endif // WITH_NOVODEX

	return 0;
}

#if WITH_NOVODEX && _MSC_VER && !XBOX // Windows only hack to bypass PhysX installation req

#define BASE_KEY_NAME            TEXT("Software\\AGEIA Technologies\\")
#define LOCAL_DLL_SUBKEY_NAME    TEXT("enableLocalPhysXCore")
#define EPIC_HACK_SUBKEY_NAME    TEXT("EpicLocalDLLHack")
#define NO_NIC_MAC               "AGEIA"

#if 1 //@todo physx: !FINAL_RELEASE

// Only include and link against in non-FR builds
#include "Iphlpapi.h"
#pragma comment(lib, "Iphlpapi.lib")

/**
 * Reads the MAC address the same that PhysX does
 *
 * @param Mac the array to set
 */
void GetMAC(BYTE Mac[6])
{
    IP_ADAPTER_INFO AdapterInfo[16];
    DWORD dwBufLen = sizeof(AdapterInfo);
	// Ask for all of the adapters
    DWORD dwStatus = GetAdaptersInfo(AdapterInfo,&dwBufLen);
    if (dwStatus == ERROR_SUCCESS)
	{
		// Iterate through the list copying the mac for each
		PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
		do
		{
			appMemcpy(Mac,pAdapterInfo->Address,6);
			pAdapterInfo = pAdapterInfo->Next;
		}
		while (pAdapterInfo);
	}
	else
    {
		// Copy default
		appMemcpy(Mac,NO_NIC_MAC,6);
    }
}

/**
 * The PhysX DLLs need to be properly installed. However, we can work around this
 * by providing a registry value to point to our pseudo install. This method writes
 * the MAC address to the registry value that PhysX looks at. Also adds a key that
 * we'll use later to delete the hack
 */
static void DoPhysXRegistryHack(void)
{
	BYTE Mac[6];
	// Read the mac address so we can write it to the key
	GetMAC(Mac);
	HKEY BaseKey;
	// Open the base key
    LONG Result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,BASE_KEY_NAME,0,KEY_WRITE,&BaseKey);
	// Create if missing
	if (Result != ERROR_SUCCESS)
	{
		Result = RegCreateKeyEx(HKEY_LOCAL_MACHINE, BASE_KEY_NAME, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE | KEY_CREATE_SUB_KEY, NULL, &BaseKey, NULL);
		//Result = RegCreateKey(HKEY_LOCAL_MACHINE,BASE_KEY_NAME,&BaseKey);
		// Make sure we can create the key or PhysX won't run later
//		check(Result == ERROR_SUCCESS);
	}
	// Now write out the MAC address
	Result = RegSetValueEx(BaseKey,LOCAL_DLL_SUBKEY_NAME,0,REG_BINARY,(LPBYTE)Mac,6);
//	check(Result == ERROR_SUCCESS);
	UBOOL bHack = TRUE;
	// Write out the hack key
	Result = RegSetValueEx(BaseKey,EPIC_HACK_SUBKEY_NAME,0,REG_BINARY,(LPBYTE)&bHack,sizeof(UBOOL));
//	check(Result == ERROR_SUCCESS);
	RegCloseKey(BaseKey);
	debugf(TEXT(""));
	debugf(TEXT("Locally bypassing PhysX installation requirement. (TODO: Remove bypass before shipping)"));
	debugf(TEXT(""));
}
#else
/**
 * Removes our registry hack so that a proper install is required
 */
static void DoPhysXRegistryHack(void)
{
	HKEY BaseKey;
	// Open the base key
    LONG Result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,BASE_KEY_NAME,0,KEY_WRITE | KEY_READ,&BaseKey);
	if (Result == ERROR_SUCCESS)
	{
		DWORD Type;
		UBOOL bIgnored;
		DWORD Length;
		// Check for Epic's hack key
        Result = RegQueryValueEx(BaseKey,EPIC_HACK_SUBKEY_NAME,0,&Type,
			(LPBYTE)&bIgnored,(LPDWORD)&Length);
		if (Result == ERROR_SUCCESS)
		{
			BYTE Mac[6] = { 0,0,0,0,0,0 };
			// It's there so delete the local flag
			Result = RegSetValueEx(BaseKey,LOCAL_DLL_SUBKEY_NAME,0,REG_BINARY,(LPBYTE)Mac,6);
		}
		RegCloseKey(BaseKey);
	}
}
#endif
#endif
