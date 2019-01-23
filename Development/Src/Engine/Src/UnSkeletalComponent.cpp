/*=============================================================================
	UnSkeletalComponent.cpp: Actor component implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineAnimClasses.h"
#include "EngineDecalClasses.h"
#include "EnginePhysicsClasses.h"
#include "UnSkeletalRenderCPUSkin.h"
#include "UnSkeletalRenderGPUSkin.h"
#include "UnDecalRenderData.h"
#include "ChartCreation.h"

#if WITH_FACEFX
	#include "UnFaceFXSupport.h"
	#include "UnFaceFXRegMap.h"
	#include "UnFaceFXMaterialParameterProxy.h"
	#include "UnFaceFXMorphTargetProxy.h"

	#include "../FaceFX/UnFaceFXMaterialNode.h"
	#include "../FaceFX/UnFaceFXMorphNode.h"
	#include "../../../External/FaceFX/FxSDK/Inc/FxActorInstance.h"

	using namespace OC3Ent;
	using namespace Face;

	/** FaceFX stats */
	enum EFaceFXStats
	{
		STAT_FaceFX_TickTime = STAT_FaceFXFirstStat,
		STAT_FaceFX_BeginFrameTime,
		STAT_FaceFX_MorphPassTime,
		STAT_FaceFX_MaterialPassTime,
		STAT_FaceFX_BoneBlendingPassTime,
		STAT_FaceFX_EndFrameTime
	};

	/** FaceFX stats objects */
	DECLARE_STATS_GROUP(TEXT("FaceFX"),STATGROUP_FaceFX);
	DECLARE_CYCLE_STAT(TEXT("FaceFX Tick Time"),STAT_FaceFX_TickTime,STATGROUP_FaceFX);
	DECLARE_CYCLE_STAT(TEXT("FaceFX Begin Frame Time"),STAT_FaceFX_BeginFrameTime,STATGROUP_FaceFX);
	DECLARE_CYCLE_STAT(TEXT("FaceFX Morph Pass Time"),STAT_FaceFX_MorphPassTime,STATGROUP_FaceFX);
	DECLARE_CYCLE_STAT(TEXT("FaceFX Material Pass Time"),STAT_FaceFX_MaterialPassTime,STATGROUP_FaceFX);
	DECLARE_CYCLE_STAT(TEXT("FaceFX Bone Blending Time"),STAT_FaceFX_BoneBlendingPassTime,STATGROUP_FaceFX);
	DECLARE_CYCLE_STAT(TEXT("FaceFX End Frame Time"),STAT_FaceFX_EndFrameTime,STATGROUP_FaceFX);
	
	DECLARE_MEMORY_STAT(TEXT("FaceFX Peak Mem"),STAT_FaceFXPeakAllocSize,STATGROUP_Memory);
	DECLARE_MEMORY_STAT(TEXT("FaceFX Cur Mem"),STAT_FaceFXCurrentAllocSize,STATGROUP_Memory);

	// Priority with which to display sounds triggered by FaceFX.
	#define SUBTITLE_PRIORITY_FACEFX	10000

#endif // WITH_FACEFX


IMPLEMENT_CLASS(USkeletalMeshComponent);

extern UBOOL GShouldLogOutAFrameOfSkelCompTick;
#define SHOW_SKELETAL_MESH_COMPONENT_TICK_TIME 0

#define SHOW_ANIMNODE_TICK_TIMES	(0)
#define SHOW_INITANIMTREE_TIME		(0)

/*-----------------------------------------------------------------------------
	USkeletalMeshComponent.
-----------------------------------------------------------------------------*/

void USkeletalMeshComponent::DeleteAnimTree()
{
	// Just release the reference to the existing AnimTree. GC will take care of actually freeing the AnimNodes, 
	// as this should have been the only reference to them!
	Animations = NULL;

	// Also clear refs to nodes in tree.
	AnimTickArray.Empty();
}

void USkeletalMeshComponent::Attach()
{
	if( SkeletalMesh )
	{
		// If the component has morph targets, and the GPU doesn't support morphing, use CPU skinning.
		const UBOOL bHasMorphTargets = MorphSets.Num();
		const UBOOL bSupportsGPUMorphing = (GRHIShaderPlatform != SP_PCD3D_SM2);

		// Also check if skeletal mesh has too many bones/chunk for GPU skinning.
		const UBOOL bIsCPUSkinned = SkeletalMesh->IsCPUSkinned() || (bHasMorphTargets && !bSupportsGPUMorphing);
		if(bIsCPUSkinned)
		{
			//debugf(TEXT("'%s' has too many bones for GPU skinning"), *SkeletalMesh->GetFullName());
			MeshObject = ::new FSkeletalMeshObjectCPUSkin(this);			
		}
		else
		{
			MeshObject = ::new FSkeletalMeshObjectGPUSkin(this);			
		}
	}

	Super::Attach();

	if( GWorld->HasBegunPlay() && !Animations && AnimTreeTemplate )
	{
		// make sure anim tree is instanced from template.
		SetAnimTreeTemplate( AnimTreeTemplate );
	}
	else
	{
		InitAnimTree();
	}

#if WITH_FACEFX
	if( !FaceFXActorInstance && SkeletalMesh )
	{
		if( SkeletalMesh->FaceFXAsset )
		{
			FaceFXActorInstance = new FxActorInstance();
			FaceFXActorInstance->SetActor(SkeletalMesh->FaceFXAsset->GetFxActor());
		}
	}
#endif // WITH_FACEFX

	bRequiredBonesUpToDate = FALSE;

	UpdateParentBoneMap();

	UpdateSkelPose();
	ConditionalUpdateTransform();
}

void USkeletalMeshComponent::BeginPlay()
{
	Super::BeginPlay();

	UBOOL bNewAnim = false;
	if(!Animations && AnimTreeTemplate)
	{
		Animations = AnimTreeTemplate->CopyAnimTree(this);
		bNewAnim = true;
	}

	// If we created a new AnimTree, init it now.
	if(bNewAnim)
	{
		InitAnimTree();

		UpdateSkelPose();
		ConditionalUpdateTransform();
	}

	// Call BeginPlay on any attached components.
	for(UINT AttachmentIndex = 0;AttachmentIndex < (UINT)Attachments.Num();AttachmentIndex++)
	{
		FAttachment& Attachment = Attachments(AttachmentIndex);
		if(Attachment.Component)
		{
			Attachment.Component->ConditionalBeginPlay();
		}
	}
}

/** Calculate the up-to-date transform of the supplied SkeletalMeshComponent, which should be attached to this component. */
FMatrix USkeletalMeshComponent::CalcAttachedSkelCompMatrix(const USkeletalMeshComponent* AttachedComp)
{
	// First, find what our up-to-date transform should be
	FMatrix ParentLocalToWorld;
	if(AttachedToSkelComponent)
	{
		ParentLocalToWorld = AttachedToSkelComponent->CalcAttachedSkelCompMatrix(this);
	}
	else
	{
		ParentLocalToWorld = LocalToWorld;
	}

	// Then find the attachment in the array
	INT AttachmentIndex = INDEX_NONE;
	for(UINT Idx = 0; Idx < (UINT)Attachments.Num(); Idx++)
	{
		FAttachment& Attachment = Attachments(Idx);
		if(Attachment.Component == AttachedComp)
		{
			AttachmentIndex = Idx;
			break;
		}
	}

	// If attachment is not in the array - generate a warning here and return SkelComp's current matrix
	if(AttachmentIndex == INDEX_NONE)
	{
		debugf(TEXT("ERROR: Component '%s' not found as attachment of '%s'"), *AttachedComp->GetPathName(), *GetPathName());
		return AttachedComp->LocalToWorld;
	}

	// Use the attachment info to calculate the new transform.
	FAttachment& Attachment = Attachments(AttachmentIndex);
	INT	BoneIndex = MatchRefBone(Attachment.BoneName);
	if(BoneIndex != INDEX_NONE)
	{
		FVector RelativeScale = Attachment.RelativeScale == FVector(0,0,0) ? FVector(1,1,1) : Attachment.RelativeScale;
		const FMatrix& AttachmentToWorld = FScaleRotationTranslationMatrix( RelativeScale, Attachment.RelativeRotation, Attachment.RelativeLocation ) * SpaceBases(BoneIndex) * ParentLocalToWorld;

		return AttachmentToWorld;
	}
	// If bone not found, return current transform.
	else
	{
		debugf(TEXT("CalcAttachedSkelCompMatrix: Bone '%s' not found in '%s' for attached component '%s'"), *Attachment.BoneName.ToString(), *this->GetPathName(), *AttachedComp->GetPathName());
		return AttachedComp->LocalToWorld;
	}
}

static void AddDistanceFactorToChart(FLOAT DistanceFactor)
{
	if(DistanceFactor < SMALL_NUMBER)
	{
		return;
	}

	if(DistanceFactor >= GDistanceFactorDivision[NUM_DISTANCEFACTOR_BUCKETS-2])
	{
		GDistanceFactorChart[NUM_DISTANCEFACTOR_BUCKETS-1]++;
	}
	else if(DistanceFactor < GDistanceFactorDivision[0])
	{
		GDistanceFactorChart[0]++;
	}
	else
	{
		for(INT i=1; i<NUM_DISTANCEFACTOR_BUCKETS-2; i++)
		{
			if(DistanceFactor < GDistanceFactorDivision[i])
			{
				GDistanceFactorChart[i]++;
				break;
			}
		}
	}
}

void USkeletalMeshComponent::UpdateTransform()
{
	SCOPE_CYCLE_COUNTER(STAT_SkelCompUpdateTransform);

	// for characters we are not trying to animate we don't need to BlendInPhysics!
	const UBOOL bRecentlyRendered = (LastRenderTime > GWorld->GetWorldInfo()->TimeSeconds - 1.0f);

	if(ParentAnimComponent && bTransformFromAnimParent)
	{		
		Super::SetParentToWorld(ParentAnimComponent->LocalToWorld);
	}

	// If we have physics, and physics is done, blend in any physics results.
	// We use the bHasHadPhysicsBlendedIn to ensure we don't try and blend it in more than once!
	// We need to do this before Super::UpdateTransform so the bounding box takes into account bone movement due to physics.
	if(PhysicsAssetInstance && !bHasHadPhysicsBlendedIn && GWorld->InTick && GWorld->bPostTickComponentUpdate)
	{
		if(bRecentlyRendered || bUpdateSkelWhenNotRendered)
		{
			BlendInPhysics();
		}

		bHasHadPhysicsBlendedIn = TRUE;
	}

	Super::UpdateTransform();

	// Start assuming this is the last time we'll call UpdateTransform.
	UBOOL bFinalUpdate = TRUE;
	
	// if we have physics to blend in (or our parent does), and we haven't received physics update yet, this is not final update. 
	if((PhysicsAssetInstance || (ParentAnimComponent && ParentAnimComponent->PhysicsAssetInstance)) && GWorld->InTick && !GWorld->bPostTickComponentUpdate)
	{
		bFinalUpdate = FALSE;
	}

	// We mark ourself as dirty, so we revisit it afterwards. That later visit will update attachments.
	if( !bFinalUpdate )
	{
		BeginDeferredUpdateTransform();
	}

	if(bFinalUpdate || bForceUpdateAttachmentsInTick)
	{
		// Mark all the attachments' transforms as dirty.
		for(UINT AttachmentIndex = 0;AttachmentIndex < (UINT)Attachments.Num();AttachmentIndex++)
		{
			FAttachment& Attachment = Attachments(AttachmentIndex);
			if(Attachment.Component)
			{
				Attachment.Component->BeginDeferredUpdateTransform();
			}
		}

		// Update the attachments.
		UpdateChildComponents();

		if(Owner && GWorld->HasBegunPlay())
		{
			for(INT i=0; i<Owner->Attached.Num(); i++)
			{
				AActor* Other = Owner->Attached(i);
				if(Other && Other->BaseSkelComponent == this)
				{
					// This UpdateTransform might be part of the initial association of components.
					// In that case, we don't want to start calling MoveActor on things, as they might not be fully associated.
					// So we check that the level is not in the process of being made visible, and do nothing if that is the case.
					ULevel* Level = Other->GetLevel();
					if(Level && !Level->bHasVisibilityRequestPending)
					{
						const INT BoneIndex = MatchRefBone(Other->BaseBoneName);
						if(BoneIndex != INDEX_NONE)
						{
							FMatrix BaseTM = GetBoneMatrix(BoneIndex);
							BaseTM.RemoveScaling();

							FRotationTranslationMatrix HardRelMatrix(Other->RelativeRotation,Other->RelativeLocation);

							const FMatrix& NewWorldTM = HardRelMatrix * BaseTM;

							const FVector& NewWorldPos = NewWorldTM.GetOrigin();
							const FRotator& NewWorldRot = NewWorldTM.Rotator();

							FCheckResult Hit(1.f);
							GWorld->MoveActor( Other, NewWorldPos - Other->Location, NewWorldRot, MOVE_IgnoreBases, Hit );
							if (Owner == NULL || Owner->bDeleteMe)
							{
								// MoveActor() resulted in us being detached or Owner's destruction
								break;
							}
						}
						else
						{
							debugf(TEXT("USkeletalMeshComponent::UpdateTransform for %s: BaseBoneName (%s) not found for attached Actor %s!"), *Owner->GetName(), *Other->BaseBoneName.ToString(), *Other->GetName());
						}
					}
				}
			}
		}
	}

	// if we have not updated the transforms then no need to send them to the rendering thread
	if( bFinalUpdate && MeshObject && ( bRecentlyRendered || bUpdateSkelWhenNotRendered || MeshObject->bHasBeenUpdatedAtLeastOnce == FALSE ) )
	{
		SCOPE_CYCLE_COUNTER(STAT_MeshObjectUpdate);

		INT UseLOD = PredictedLODLevel;
		// If we have a ParentAnimComponent - force this component to render at that LOD, so all bones are present for it.
		// Note that this currently relies on the behaviour where this mesh is rendered at the LOD we pass in here for all viewports. We should
		// be able to render it at lower LOD on viewports where it is further away. That will make the ParentAnimComponent case a bit harder to solve.
		if(ParentAnimComponent)
		{
			UseLOD = ParentAnimComponent->PredictedLODLevel;
		}

		MeshObject->Update(UseLOD,this,ActiveMorphs);  // send to rendering thread
		MeshObject->bHasBeenUpdatedAtLeastOnce = TRUE;
	}
}


void USkeletalMeshComponent::UpdateChildComponents()
{
	for(UINT AttachmentIndex = 0;AttachmentIndex < (UINT)Attachments.Num();AttachmentIndex++)
	{
		FAttachment&	Attachment = Attachments(AttachmentIndex);
		const INT		BoneIndex = MatchRefBone(Attachments(AttachmentIndex).BoneName);

		if(Attachment.Component && BoneIndex != INDEX_NONE)
		{
			FVector RelativeScale = Attachment.RelativeScale == FVector(0,0,0) ? FVector(1,1,1) : Attachment.RelativeScale;
			const FMatrix& AttachmentToWorld = FScaleRotationTranslationMatrix( RelativeScale, Attachment.RelativeRotation, Attachment.RelativeLocation ) * SpaceBases(BoneIndex) * LocalToWorld;
			SetAttachmentOwnerVisibility(Attachment.Component);
			Attachment.Component->UpdateComponent(Scene,GetOwner(),AttachmentToWorld);
		}
	}
}

void USkeletalMeshComponent::Detach()
{
	Super::Detach();

	for(INT AttachmentIndex = 0;AttachmentIndex < Attachments.Num();AttachmentIndex++)
	{
		if(Attachments(AttachmentIndex).Component)
		{
			Attachments(AttachmentIndex).Component->ConditionalDetach();
		}
	}

#if WITH_FACEFX
	if( FaceFXActorInstance )
	{
		delete FaceFXActorInstance;
		FaceFXActorInstance = NULL;
	}
#endif // WITH_FACEFX

	if(MeshObject)
	{
		// Begin releasing the RHI resources used by this skeletal mesh component.
		// This doesn't immediately destroy anything, since the rendering thread may still be using the resources.
		MeshObject->ReleaseResources();

		// Begin a deferred delete of MeshObject.  BeginCleanup will call MeshObject->FinishDestroy after the above release resource
		// commands execute in the rendering thread.
		BeginCleanup(MeshObject);
		MeshObject = NULL;
	}
}

void USkeletalMeshComponent::TickAnimNodes(FLOAT DeltaTime)
{
	TickTag++;
	check(Animations->SkelComponent == this);
	Animations->TotalWeightAccumulator = 1.f;

	for(INT i=0; i<AnimTickArray.Num(); i++)
	{
		UAnimNode* Node = AnimTickArray(i);
		check(Node);

		// Ensure NodeTotalWeight is no more than 1.0. This can happen when using AnimNodeBlendPerBone etc.
		Node->NodeTotalWeight = ::Min(Node->TotalWeightAccumulator, 1.f);

		// Reset TotalWeightAccumulator for next frame
		Node->TotalWeightAccumulator	= 0.f;
		// Clear bJustBecameRelevant flag.
		Node->bJustBecameRelevant		= FALSE;

		// Call final blend relevancy notifications
		if( !Node->bRelevant )
		{
			if( Node->NodeTotalWeight > ZERO_ANIMWEIGHT_THRESH )
			{
				Node->OnBecomeRelevant();
				Node->bRelevant				= TRUE;
				Node->bJustBecameRelevant	= TRUE;
			}
		}
		else
		{
			if( Node->NodeTotalWeight <= ZERO_ANIMWEIGHT_THRESH )
			{
				Node->OnCeaseRelevant();
				Node->bRelevant = FALSE;
			}
		}

		// If we are not skipping because of zero weight, call the Tick function.
		if( Node->bRelevant || !Node->bSkipTickWhenZeroWeight )
		{
			Node->NodeTickTag = TickTag;

#if !FINAL_RELEASE && SHOW_ANIMNODE_TICK_TIMES
			DOUBLE Start = 0.f;
			if(GShouldLogOutAFrameOfSkelCompTick)
			{
				Start = appSeconds();
			}
#endif

			Node->TickAnim(DeltaTime, Node->NodeTotalWeight);

#if !FINAL_RELEASE && SHOW_ANIMNODE_TICK_TIMES
			if(GShouldLogOutAFrameOfSkelCompTick)
			{
				DOUBLE End = appSeconds();
				debugf(TEXT("-- %s - %s:\t%fms"), SkeletalMesh?*SkeletalMesh->GetName():TEXT("None"), *Node->GetName(), (End-Start)*1000.f);
			}
#endif
		}
	}

	// After all nodes have been ticked, and weights have been updated, take another pass for AnimNodeSequence groups.
	// (Anim Synchronization, and notification groups).
	UAnimTree* AnimTree = Cast<UAnimTree>(Animations);
	if( AnimTree )
	{
		SCOPE_CYCLE_COUNTER(STAT_AnimSyncGroupTime);
		AnimTree->UpdateAnimNodeSeqGroups(DeltaTime);
	}
}

void USkeletalMeshComponent::Tick(FLOAT DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_SkelComponentTickTime);

	DeltaTime *= GetOwner() ? GetOwner()->CustomTimeDilation : 1.f;

	// If in-game, tick all animation channels in our anim nodes tree. Dont want to play animation in level editor.
	const UBOOL bHasBegunPlay = GWorld->HasBegunPlay();

	if( MeshObject && Animations && bHasBegunPlay)
	{
		SCOPE_CYCLE_COUNTER(STAT_AnimTickTime);
		if (!bPauseAnims)
		{
			TickAnimNodes(DeltaTime);
		}
		TickSkelControls(DeltaTime);
	}

	// See if this mesh was rendered recently.
	const UBOOL bRecentlyRendered = (LastRenderTime > GWorld->GetWorldInfo()->TimeSeconds - 1.0f);

	// If we have cloth, apply wind forces to make it flutter, each frame.
	if(ClothSim)
	{
		// Do 'auto freeezing'
		if(bAutoFreezeClothWhenNotRendered)
		{
			// If we have not been rendered for a while, and are not yet frozen, do it now
			if(!bRecentlyRendered && !bClothFrozen)
			{
				SetClothFrozen(TRUE);
			}
			// If we have been rendered recently, and are still frozen, unfreeze.
			else if(bRecentlyRendered && bClothFrozen)
			{
				SetClothFrozen(FALSE);
			}
		}

		// If not frozen - update wind forces.
		if(!bClothFrozen)
		{
			UpdateClothWindForces(DeltaTime);
		}
	}

	// Save this off before we call BeginDeferredUpdateTransform.
	UBOOL bNeedsUpdateTransform = NeedsUpdateTransform();

	// See if we are going to need to update kinematics
	UBOOL bUpdateKinematics = (!bUseSingleBodyPhysics && PhysicsAssetInstance && bUpdateKinematicBonesFromAnimation && !bNotUpdatingKinematicDueToDistance);

	// If we need it, find the up-to-date transform for this component. 
	FMatrix ParentTransform = FMatrix::Identity;
	if((bUpdateKinematics || bForceUpdateAttachmentsInTick) && bNeedsUpdateTransform && (Owner != NULL))
	{
		// We have a special case for when its attached to another SkelComp.
		if(AttachedToSkelComponent)
		{
			ParentTransform = AttachedToSkelComponent->CalcAttachedSkelCompMatrix(this);
		}
		else
		{
			ParentTransform = Owner->LocalToWorld();
		}
	}

	// If we want to move the bones - do it now.
	if((bRecentlyRendered || bUpdateSkelWhenNotRendered) && !bForceRefpose)
	{
#if SHOW_SKELETAL_MESH_COMPONENT_TICK_TIME || LOOKING_FOR_PERF_ISSUES
		const DOUBLE UpdateSkelPoseStart = appSeconds();	
#endif

		// Do not update bones if we are taking bone transforms from another SkelMeshComp
		if(!ParentAnimComponent)
		{
			// Update the mesh-space bone transforms held in SpaceBases array from animation data.
			UpdateSkelPose( DeltaTime ); 
		}

		// If desired, force attachments into the correct position now.
		if(bForceUpdateAttachmentsInTick)
		{
			if(NeedsUpdateTransform() && Owner != NULL)
			{
				ConditionalUpdateTransform(ParentTransform);
			}
			else
			{
				ConditionalUpdateTransform();
			}
		}
		// Otherwise, make sure that we do call UpdateTransform later this frame, as that is where transforms are sent to rendering thread
		else
		{
			BeginDeferredUpdateTransform();
		}

#if SHOW_SKELETAL_MESH_COMPONENT_TICK_TIME || LOOKING_FOR_PERF_ISSUES
		const DOUBLE UpdateSkelPoseStop = (appSeconds() - UpdateSkelPoseStart) * 1000.f;
		if( GShouldLogOutAFrameOfSkelCompTick == TRUE )
		{
			debugf( TEXT( "USkeletalMeshComponent:  %s   SkelMesh:  %s  Owner: %s, UpdateSkelPoseS: %f" ), *this->GetPathName(), *SkeletalMesh->GetPathName(), *GetOwner()->GetName(), UpdateSkelPoseStop );
		}
#endif
	}

	// If desired, update physics bodies associated with skeletal mesh component to match.
	// should not be in the above bUpdateSkelWhenNotRendered block so that physics gets properly updated if the bodies are moving due to other sources (e.g. actor movement)
	if(bUpdateKinematics)
	{
		FMatrix CurrentLocalToWorld;
		if(bNeedsUpdateTransform && Owner != NULL)
		{
			CurrentLocalToWorld = CalcCurrentLocalToWorld(ParentTransform);
		}
		else
		{
			CurrentLocalToWorld = LocalToWorld;
		}

		UpdateRBBonesFromSpaceBases(CurrentLocalToWorld, FALSE, FALSE);
	}
}

INT USkeletalMeshComponent::GetNumElements() const
{
	return SkeletalMesh ? SkeletalMesh->Materials.Num() : 0;
}

//
//	USkeletalMeshComponent::GetMaterial
//

UMaterialInterface* USkeletalMeshComponent::GetMaterial(INT MaterialIndex) const
{
	if(MaterialIndex < Materials.Num() && Materials(MaterialIndex))
	{
		return Materials(MaterialIndex);
	}
	else if(SkeletalMesh && MaterialIndex < SkeletalMesh->Materials.Num() && SkeletalMesh->Materials(MaterialIndex))
	{
		return SkeletalMesh->Materials(MaterialIndex);
	}
	else
	{
		return NULL;
	}
}

/**
 *	Attach a Component to the bone of this SkeletalMeshComponent at the supplied offset.
 *	If you are a attaching to a SkeletalMeshComponent that is using another SkeletalMeshComponent for its bone transforms (via the ParentAnimComponent pointer)
 *	you should attach to that component instead.
 */
void USkeletalMeshComponent::AttachComponent(UActorComponent* Component,FName BoneName,FVector RelativeLocation,FRotator RelativeRotation,FVector RelativeScale)
{
	if( IsPendingKill() )
	{
		debugf( TEXT("USkeletalMeshComponent::AttachComponent: Trying to attach '%s' to '%s' which IsPendingKill. Aborting"), *(Component->GetName()), *(this->GetName()) );
		return;
	}

	Component->DetachFromAny();

	if(ParentAnimComponent)
	{
		debugf( 
			TEXT("SkeletalMeshComponent %s in Actor %s has a ParentAnimComponent - should attach Component %s to that instead."), 
			*GetPathName(),
			Owner ? *Owner->GetPathName() : TEXT("None"),
			*Component->GetPathName()
			);
		return;
	}

	// Add the component to the attachments array.
	new(Attachments) FAttachment(Component,BoneName,RelativeLocation,RelativeRotation,RelativeScale);

	// Set pointer is a skeletal mesh component
	USkeletalMeshComponent* SkelComp = Cast<USkeletalMeshComponent>(Component);
	if(SkelComp)
	{
		SkelComp->AttachedToSkelComponent = this;
	}

	if(IsAttached())
	{
		// Attach the component to the scene.
		INT BoneIndex = MatchRefBone(BoneName);
		if(BoneIndex != INDEX_NONE)
		{
			const FMatrix& AttachmentToWorld = FScaleRotationTranslationMatrix(RelativeScale,RelativeRotation,RelativeLocation) * SpaceBases(BoneIndex) * LocalToWorld;
			SetAttachmentOwnerVisibility(Component);
			Component->ConditionalAttach(Scene,Owner,AttachmentToWorld);
		}
		else
		{
#if !PS3 // hopefully will be found on PC
			debugf( TEXT("USkeletalMeshComponent::AttachComponent : Could not find bone '%s' in %s  attaching: %s"), *BoneName.ToString(), *GetOwner()->GetName(), *Component->TemplateName.ToString() );
#endif
		}
	}
}

/** Version of AttachComponent that uses Socket data stored in the Skeletal mesh to attach a component to a particular bone and offset. */
void USkeletalMeshComponent::AttachComponentToSocket(UActorComponent* Component,FName SocketName)
{
	if( SkeletalMesh != NULL )
	{
		USkeletalMeshSocket* Socket = SkeletalMesh->FindSocket(SocketName);
		if( Socket )
		{
			AttachComponent(Component, Socket->BoneName, Socket->RelativeLocation, Socket->RelativeRotation, Socket->RelativeScale);
		}
		else
		{
#if !PS3 // hopefully will be found on PC
			debugf( TEXT("AttachComponentToSocket : Could not find socket '%s' in %s  attaching: %s"), *SocketName.ToString(), *SkeletalMesh->GetPathName(), *Component->GetName() );
#endif
		}
	}
	else
	{
#if !PS3 // hopefully will be found on PC
		debugf( TEXT("AttachComponentToSocket : no SkeletalMesh could not attach socket '%s'  attaching: %s"), *SocketName.ToString(), *Component->GetName() );
#endif
	}
}

//
//	USkeletalMeshComponent::DetachComponent
//

void USkeletalMeshComponent::DetachComponent(UActorComponent* Component)
{
	if (Component != NULL)
	{
		// Find the specified component in the Attachments array.
		for(INT AttachmentIndex = 0;AttachmentIndex < Attachments.Num();AttachmentIndex++)
		{
			if(Attachments(AttachmentIndex).Component == Component)
			{
				// This attachment is the specified component, detach it from the scene and remove it from the attachments array.
				Component->ConditionalDetach();
				Attachments.Remove(AttachmentIndex--);

				// Unset pointer to a skeletal mesh component
				USkeletalMeshComponent* SkelComp = Cast<USkeletalMeshComponent>(Component);
				if(SkelComp)
				{
					SkelComp->AttachedToSkelComponent = NULL;
				}

				break;
			}
		}
	}
}

/** if bOverrideAttachmentOwnerVisibility is true, overrides the owner visibility values in the specified attachment with our own
 * @param Component the attached primitive whose settings to override
 */
void USkeletalMeshComponent::SetAttachmentOwnerVisibility(UActorComponent* Component)
{
	if (bOverrideAttachmentOwnerVisibility)
	{
		UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component);
		if (Primitive != NULL)
		{
			Primitive->SetOwnerNoSee(bOwnerNoSee);
			Primitive->SetOnlyOwnerSee(bOnlyOwnerSee);
		}
	}
}

//
//	USkeletalMeshComponent::SetParentToWorld
//

void USkeletalMeshComponent::SetParentToWorld(const FMatrix& ParentToWorld)
{
	// If getting transform from ParentAnimComponent - ignore any other calls to modify the transform.
	if(bTransformFromAnimParent && ParentAnimComponent)
	{
		return;
	}
	else
	{
		Super::SetParentToWorld(ParentToWorld);
	}
}

/** Utility for calculating the current LocalToWorld matrix of this SkelMeshComp, given its parent transform. */
FMatrix USkeletalMeshComponent::CalcCurrentLocalToWorld(const FMatrix& ParentMatrix)
{
	FMatrix ResultMatrix = ParentMatrix;

	if(AbsoluteTranslation)
	{
		ResultMatrix.M[3][0] = ResultMatrix.M[3][1] = ResultMatrix.M[3][2] = 0.0f;
	}

	if(AbsoluteRotation || AbsoluteScale)
	{
		FVector	X(ResultMatrix.M[0][0],ResultMatrix.M[1][0],ResultMatrix.M[2][0]),
			Y(ResultMatrix.M[0][1],ResultMatrix.M[1][1],ResultMatrix.M[2][1]),
			Z(ResultMatrix.M[0][2],ResultMatrix.M[1][2],ResultMatrix.M[2][2]);

		if(AbsoluteScale)
		{
			X.Normalize();
			Y.Normalize();
			Z.Normalize();
		}

		if(AbsoluteRotation)
		{
			X = FVector(X.Size(),0,0);
			Y = FVector(0,Y.Size(),0);
			Z = FVector(0,0,Z.Size());
		}

		ResultMatrix.M[0][0] = X.X;
		ResultMatrix.M[1][0] = X.Y;
		ResultMatrix.M[2][0] = X.Z;
		ResultMatrix.M[0][1] = Y.X;
		ResultMatrix.M[1][1] = Y.Y;
		ResultMatrix.M[2][1] = Y.Z;
		ResultMatrix.M[0][2] = Z.X;
		ResultMatrix.M[1][2] = Z.Y;
		ResultMatrix.M[2][2] = Z.Z;
	}

	// If desired, take into account the transform from the Origin/RotOrigin in the SkeletalMesh (if there is one).
	// We don't do this if bTransformFromAnimParent is true and we have a parent - in that case both ResultMatrixs should be the same, including skeletal offset.
	FMatrix SkelCompOffset = FMatrix::Identity;
	if(SkeletalMesh && !bForceRawOffset && !(ParentAnimComponent && bTransformFromAnimParent))
	{
		SkelCompOffset = FTranslationMatrix( SkeletalMesh->Origin ) * FRotationMatrix(SkeletalMesh->RotOrigin);
	}

	ResultMatrix = SkelCompOffset * FScaleRotationTranslationMatrix( Scale * Scale3D, Rotation, Translation ) * ResultMatrix;
	return ResultMatrix;
}

//
//	USkeletalMeshComponent::SetTransformedToWorld
//
void USkeletalMeshComponent::SetTransformedToWorld()
{
	LocalToWorld = CalcCurrentLocalToWorld(CachedParentToWorld);
	LocalToWorldDeterminant = LocalToWorld.Determinant();
}

UBOOL USkeletalMeshComponent::IsValidComponent() const
{
	 return (SkeletalMesh != NULL) && Super::IsValidComponent(); 
}


/** 
 * Build a priority list of branches that should be evaluated first.
 * This is to solve Controllers relying on bones to be updated before them. 
 */
void USkeletalMeshComponent::BuildComposePriorityList(TArray<BYTE>& PriorityList)
{
	if( !SkeletalMesh || !Animations )
	{
		return;
	}

			UAnimTree*	Tree		= Cast<UAnimTree>(Animations);
	const	INT			NumBones	= SkeletalMesh->RefSkeleton.Num();

	// If the first node of the Animation Tree if not a UAnimTree, then skip.
	// This can happen in the AnimTree editor when previewing a node different than the root.
	if( !Tree )
	{
		return;
	}

	// reinitialize list
	PriorityList.Empty();
	PriorityList.AddZeroed(NumBones);

	const BYTE Flag = 1;

	for(INT i=0; i<Tree->PrioritizedSkelBranches.Num(); i++)
	{
		const FName BoneName = Tree->PrioritizedSkelBranches(i);

		if( BoneName != NAME_None )
		{
			const INT BoneIndex = SkeletalMesh->MatchRefBone(BoneName);

			if( BoneIndex != INDEX_NONE )
			{
				// flag selected bone.
				PriorityList(BoneIndex) = Flag;

				// flag all parents up until root node to be evaluated.
				if( BoneIndex > 0 )
				{
					INT TestBoneIndex = SkeletalMesh->RefSkeleton(BoneIndex).ParentIndex;
					PriorityList(TestBoneIndex) = Flag;
					while( TestBoneIndex > 0 )
					{
						TestBoneIndex = SkeletalMesh->RefSkeleton(TestBoneIndex).ParentIndex;
						PriorityList(TestBoneIndex) = Flag;
					}
				}

				// Flag all child bones. We rely on the fact that they are in strictly increasing order
				// so we start at the bone after BoneIndex, up until we reach the end of the list
				// or we find another bone that has a parent before BoneIndex.
				INT	Index = BoneIndex + 1;
				if(Index < NumBones)
				{
					INT ParentIndex	= SkeletalMesh->RefSkeleton(Index).ParentIndex;

					while( Index < NumBones && ParentIndex >= BoneIndex )
					{
						PriorityList(Index)	= Flag;
						ParentIndex			= SkeletalMesh->RefSkeleton(Index).ParentIndex;

						Index++;
					}
				}
			}
		}
	}

#if 0
	debugf(TEXT("USkeletalMeshComponent::BuildComposePriorityList"));
	for(INT i=0; i<PriorityList.Num(); i++)
	{
		debugf(TEXT(" Bone: %3d, Flag: %3d"), i, PriorityList(i));
	}
#endif

}


/**
 * Take an array of relative bone atoms (translation vector, rotation quaternion and scale vector) and turn into array of component-space bone transformation matrices.
 * It will work down hierarchy multiplying the component-space transform of the parent by the relative transform of the child.
 * This code also applies any per-bone rotators etc. as part of the composition process
 *
 * @param	LocalTransforms		Relative bone atom ie. transform of child bone relative to parent bone.
 * @param	RequiredBones		Array of indices of the bones that we actually want to update. Array must be in increasing order.
 */
void USkeletalMeshComponent::ComposeSkeleton(TArray<FBoneAtom>& LocalTransforms, const TArray<BYTE>& RequiredBones)
{
	SCOPE_CYCLE_COUNTER(STAT_SkelComposeTime);

	if( !SkeletalMesh )
	{
		return;
	}

	check( SkeletalMesh->RefSkeleton.Num() == LocalTransforms.Num() );
	check( SkeletalMesh->RefSkeleton.Num() == SpaceBases.Num() );

	const UAnimTree* Tree = Cast<UAnimTree>(Animations);

	TArray<INT>		AffectedBones;
	TArray<FMatrix> NewBoneTransforms;

	// Cache this once
	AWorldInfo* WorldInfo = GWorld->GetWorldInfo();

	// If bIgnoreControllersWhenNotRendered is true, set bForceIgnore if the Owner has not been drawn recently.
	const UBOOL bRenderedRecently	= (WorldInfo->TimeSeconds - LastRenderTime) < 1.0f;
	const UBOOL bForceIgnore		= GIsGame && bIgnoreControllersWhenNotRendered && !bRenderedRecently;

	// Number of passes for composing
	// AnimTree defines a list of branches that should be performed before the rest of the others.
	INT PassNb = Tree && Tree->PrioritizedSkelBranches.Num() > 0 ? 1 : 0;
	while( PassNb >= 0 )
	{
		// Iterate over each bone
		for(INT i=0; i<RequiredBones.Num(); i++)
		{
			const INT BoneIndex = RequiredBones(i);

			// If we should skip the bones for this pass, then do so.
			if( Tree && BoneIndex < Tree->PriorityList.Num() && Tree->PriorityList(BoneIndex) != PassNb )
			{
				continue;
			}

			// For root bone, just read bone atom as component-space transform.
			if( BoneIndex == 0 )
			{
				LocalTransforms(0).ToTransform(SpaceBases(0));
			}
			// For all bones below the root, final component-space transform is relative transform * component-space transform of parent.
			else
			{
				FMatrix LocalBoneTM;
				LocalTransforms(BoneIndex).ToTransform(LocalBoneTM);

				const INT ParentIndex = SkeletalMesh->RefSkeleton(BoneIndex).ParentIndex;

#if DO_GUARD_SLOW
				// Check the precondition that Parents occur before Children in the RequiredBones array.
				const INT ReqBoneParentIndex = RequiredBones.FindItemIndex(ParentIndex);
				check(ReqBoneParentIndex != INDEX_NONE);
				check(ReqBoneParentIndex < i);
#endif
				SpaceBases(BoneIndex) = LocalBoneTM * SpaceBases(ParentIndex);
			}

			// If we have an AnimTree, and we are not ignoring controllers, apply any SkelControls in the tree now.
			if( Tree && !bIgnoreControllers && !bForceIgnore )
			{
				// If the SkelControlIndex is not empty, and we have controllers for this bone, apply it now.
				if( (SkelControlIndex.Num() > 0) && (SkelControlIndex(BoneIndex) != 255) )
				{
					const INT ControlIndex = SkelControlIndex(BoneIndex);
					check( ControlIndex < Tree->SkelControlLists.Num() );

					// Iterate over linked list of controls, calculate desired transforms for each.
					USkelControlBase* Control = Tree->SkelControlLists(ControlIndex).ControlHead;
					while( Control )
					{
						if ((bRenderedRecently || !Control->bIgnoreWhenNotRendered) && (PredictedLODLevel < Control->IgnoreAtOrAboveLOD) && Control->ControlStrength > ZERO_ANIMWEIGHT_THRESH )
						{
							SCOPE_CYCLE_COUNTER(STAT_SkelControl);

							AffectedBones.Reset();
							Control->GetAffectedBones(BoneIndex, this, AffectedBones);

							// Do nothing if we are not going to affect any bones.
							if( AffectedBones.Num() > 0 )
							{
								NewBoneTransforms.Reset();
								Control->CalculateNewBoneTransforms(BoneIndex, this, NewBoneTransforms);

								// Get Alpha for this control. CalculateNewBoneTransforms() may have changed it.
								const FLOAT ControlAlpha = Control->GetControlAlpha();

								// This allows CalculateNewBoneTransforms to do nothing, by returning an empty array.
								// ControlAlpha can also return 0 to skip applying the controller.
								if( ControlAlpha > ZERO_ANIMWEIGHT_THRESH && NewBoneTransforms.Num() > 0 )
								{
									check( AffectedBones.Num() == NewBoneTransforms.Num() );

									// Now handle blending control output into skeleton.
									// We basically have to turn each transform back into a local-space FBoneAtom, interpolate between the current FBoneAtom
									// for this bone, then do the regular 'relative to parent' blend maths.

									for(INT AffectedIdx=0; AffectedIdx<AffectedBones.Num(); AffectedIdx++)
									{
										const INT AffectedBoneIndex	= AffectedBones(AffectedIdx);
										const INT ParentIndex		= SkeletalMesh->RefSkeleton(AffectedBoneIndex).ParentIndex;

										// Calculate transform of parent bone
										FMatrix ParentTM;
										if( AffectedBoneIndex > 0 )
										{
											// If the parent of this bone is another one affected by this controller,
											// we want to use the new parent transform from the controller as the basis for the relative-space animation atom.
											// If not, use the current SpaceBase matrix for the parent.
											const INT NewBoneTransformIndex = AffectedBones.FindItemIndex(ParentIndex);
											if( NewBoneTransformIndex == INDEX_NONE )
											{
												ParentTM = SpaceBases(ParentIndex);
											}
											else
											{
												ParentTM = NewBoneTransforms(NewBoneTransformIndex);
											}
										}
										else
										{
											ParentTM = FMatrix::Identity;
										}

										// Then work out relative transform, and convert to FBoneAtom.
										const FMatrix RelTM = NewBoneTransforms(AffectedIdx) * ParentTM.Inverse();
										const FBoneAtom ControlRelAtom(RelTM);

										// faster version when we don't need to blend. Copy results directly
										if( ControlAlpha >= (1.f - ZERO_ANIMWEIGHT_THRESH) )
										{
											SpaceBases(AffectedBoneIndex)		= NewBoneTransforms(AffectedIdx);
											// Update local transforms, just in case other controllers need it later.
											LocalTransforms(AffectedBoneIndex)	= ControlRelAtom;
										}
										else
										{
											// Set the new FBoneAtom to be a blend between the current one, and the one from the controller.
											const FBoneAtom CurrentAtom = LocalTransforms(AffectedBoneIndex);
											LocalTransforms(AffectedBoneIndex).Blend(CurrentAtom, ControlRelAtom, ControlAlpha);

											// Then do usual hierarchical blending stuff (just like in ComposeSkeleton).
											if( AffectedBoneIndex > 0 )
											{
												FMatrix LocalBoneTM;
												LocalTransforms(AffectedBoneIndex).ToTransform(LocalBoneTM);
												SpaceBases(AffectedBoneIndex) = LocalBoneTM * SpaceBases(ParentIndex);
											}
											else
											{
												LocalTransforms(0).ToTransform(SpaceBases(0));
											}
										}
									}

									// Calculate desired bone scaling for this bone - scaled by the ControlStrength.
									const FLOAT BoneScale = Lerp(1.f, Control->GetBoneScale(BoneIndex, this), ControlAlpha);

									// Apply bone scaling.
									if( BoneScale != 1.f )
									{
										LocalTransforms(BoneIndex).Scale *= BoneScale;
										SpaceBases(BoneIndex) = FScaleMatrix(FVector(BoneScale)) * SpaceBases(BoneIndex);
									}

									// Find the earliest bone in bones array affected by this controller. 
									// Because parents are always before children in AffectedBones, this will always be the first element.
									const INT FirstAffectedBone = AffectedBones(0);

									// For any bones between that bone and the one we updated, that was not affected by the bone controller, we need to refresh it.
									for(INT UpdateBoneIndex = FirstAffectedBone+1; UpdateBoneIndex<BoneIndex; UpdateBoneIndex++)
									{
										// @todo We don't need to do this for any bones that are not in RequiredBones, but we don't want to do another ContainsItem here,
										// so should probably build an array of bools (entry for each bone) which we can quickly look up into.
										if( !AffectedBones.ContainsItem(UpdateBoneIndex) )
										{
											LocalTransforms(UpdateBoneIndex).ToTransform( SpaceBases(UpdateBoneIndex) );
											const INT UpdateParentIndex = SkeletalMesh->RefSkeleton(UpdateBoneIndex).ParentIndex;
											SpaceBases(UpdateBoneIndex) *= SpaceBases(UpdateParentIndex);
										}
									}
								}
							} // if( AffectedBones.Num() > 0 )
						} // if( Control->ControlStrength > KINDA_SMALL_NUMBER )

						Control = Control->NextControl;
					}
				}
			}
		}

		PassNb--;
	} // while( PassNb >= 0 )
}


/** 
 *	Utility for iterating over all SkelControls in the Animations AnimTree of this SkeletalMeshComponent.
 *	Assumes that bTickStatus has already been toggled outside of this function.
 */
void USkeletalMeshComponent::TickSkelControls(FLOAT DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_SkelControlTickTime);

	UAnimTree* AnimTree = Cast<UAnimTree>(Animations);
	if(AnimTree)
	{
		for(INT i=0; i<AnimTree->SkelControlLists.Num(); i++)
		{
			USkelControlBase* Control = AnimTree->SkelControlLists(i).ControlHead;
			while(Control)
			{
				if ( Control->ControlTickTag != TickTag )
				{
					Control->ControlTickTag = TickTag;
					Control->TickSkelControl(DeltaSeconds, this);
				}
				Control = Control->NextControl;
			}
		}
	}
}

IMPLEMENT_COMPARE_CONSTREF( BYTE, UnSkeletalComponent, { return (A - B); } )

/** Uses the current AnimTree to update the ActiveMorphs array of morph targets to apply to the SkeletalMesh. */
void USkeletalMeshComponent::UpdateActiveMorphs()
{
	ActiveMorphs.Empty();

	UAnimTree* AnimTree = Cast<UAnimTree>(Animations);
	if(AnimTree)
	{
		AnimTree->GetTreeActiveMorphs( ActiveMorphs );
	}
}

/** Performs per-frame FaceFX processing. */
void USkeletalMeshComponent::UpdateFaceFX( TArray<FBoneAtom>& LocalTransforms, UBOOL bTickFaceFX )
{
#if WITH_FACEFX
	// Note: SkeletalMesh and SkeletalMesh->FaceFXAsset cannot be NULL when 
	// this is called.
	if( FaceFXActorInstance )
	{
		FxActor* FaceFXActor = FaceFXActorInstance->GetActor();
		if( FaceFXActor )
		{
			// First, tick the current animation.
			// The only time we should be setting bTickFaceFX to FALSE is when we are ForceTicking somewhere else to a specific location.
			// In that case, IsPlaying will not be true, but we do want to evaluate FaceFX to update the face - hence the !bTickFaceFX check here.
			if( FaceFXActorInstance->IsPlayingAnim() || 
				FaceFXActorInstance->GetAllowNonAnimTick() || 
				FaceFXActorInstance->IsOpenInStudio() || 
				!bTickFaceFX ) 
			{

				// Do not do this if we are ForceTicking outside of this function.
				if(bTickFaceFX)
				{
					FxReal AudioOffset = -1.0f;
					if( CachedFaceFXAudioComp )
					{
						//@todo Is there a better way to determine if an audio component
						// is currently playing?
						if( CachedFaceFXAudioComp->SoundCue &&
							CachedFaceFXAudioComp->PlaybackTime != 0.0f &&
							!CachedFaceFXAudioComp->bFinished &&
							!GIsBenchmarking )
						{
							AudioOffset = CachedFaceFXAudioComp->PlaybackTime;
						}
					}

					FxAnimPlaybackState PlaybackState = APS_Stopped;
					{
						SCOPE_CYCLE_COUNTER(STAT_FaceFX_TickTime);
						FxDReal AppTime = 0.0;
						if( Owner && Owner->WorldInfo )
						{
							AppTime = Owner->WorldInfo->TimeSeconds;
						}
						else
						{
							AppTime = appSeconds();
						}

						// In FaceFX 1.7+ the compiled face graph has already been ticked from the FaceFX 
						// Studio code.  Ticking it again here causes the face graph results to be cleared
						// before the skeleton has been updated, so if the FaceFX actor instance is open 
						// inside of FaceFX Studio the actor instance isn't ticked again here.
						if( !FaceFXActorInstance->IsOpenInStudio() )
						{
							PlaybackState = FaceFXActorInstance->Tick(AppTime, AudioOffset);
						}
					}

					if( APS_StartAudio == PlaybackState )
					{
						if( CachedFaceFXAudioComp )
						{
							if( CachedFaceFXAudioComp->SoundCue )
							{
								CachedFaceFXAudioComp->SubtitlePriority = SUBTITLE_PRIORITY_FACEFX;
								CachedFaceFXAudioComp->Play();
							}
						}
					}

					//@todo What about non-anim tick events?
					if( APS_Stopped == PlaybackState )
					{
						StopFaceFXAnim();
					}
				}

				// Next update through the face graph and relink anything requesting a relink.
				FxBool bShouldClientRelink = FaceFXActor->ShouldClientRelink();
				if( bShouldClientRelink )
				{
					// Link the bones.
					FxMasterBoneList& MasterBoneList = FaceFXActor->GetMasterBoneList();
					for( FxSize i = 0; i < MasterBoneList.GetNumRefBones(); ++i )
					{
						FxBool bFoundBone = FxFalse;
						for( INT j = 0; j < SkeletalMesh->RefSkeleton.Num(); ++j )
						{
							if( FName(*FString(MasterBoneList.GetRefBone(i).GetNameAsCstr()), FNAME_Find, TRUE) == SkeletalMesh->RefSkeleton(j).Name )
							{
								MasterBoneList.SetRefBoneClientIndex(i, j);
								bFoundBone = FxTrue;
							}
						}
						if( !bFoundBone )
						{
							debugf(TEXT("FaceFX: WARNING Reference bone %s could not be found in the skeleton."), 
								ANSI_TO_TCHAR(MasterBoneList.GetRefBone(i).GetNameAsCstr()));
							// Make sure any bones not linked up to the skeleton do not try to update
							// the skeleton.
							MasterBoneList.SetRefBoneClientIndex(i, FX_INT32_MAX);
						}
					}
					FaceFXActor->SetShouldClientRelink(FxFalse);
				}

				{
					SCOPE_CYCLE_COUNTER(STAT_FaceFX_BeginFrameTime);
					FaceFXActorInstance->BeginFrame();
				}

				FxCompiledFaceGraph& cg = FaceFXActor->GetCompiledFaceGraph();
				FxSize numNodes = cg.nodes.Length();
				for( FxSize node = 0; node < numNodes; ++node )
				{
					switch( cg.nodes[node].nodeType )
					{
					case NT_MorphTargetUE3:
						{
							SCOPE_CYCLE_COUNTER(STAT_FaceFX_MorphPassTime);

							// Update any morph nodes that may be in the Face Graph.
							FFaceFXMorphTargetProxy* Proxy = reinterpret_cast<FFaceFXMorphTargetProxy*>(cg.nodes[node].pUserData);
							// If the proxy is invalid create one.
							FxBool bShouldLink = FxFalse;
							if( !Proxy )
							{
								FFaceFXMorphTargetProxy* NewMorphTargetProxy = new FFaceFXMorphTargetProxy();
								cg.nodes[node].pUserData = NewMorphTargetProxy;
								bShouldLink = FxTrue;
								Proxy = NewMorphTargetProxy;
							}

							if( Proxy )
							{
								// If the proxy requests to be re-linked, link it up.
								if( bShouldLink || bShouldClientRelink )
								{
									const FxFaceGraphNodeUserProperty& MorphTargetNameProperty = 
										cg.nodes[node].userProperties[FX_MORPH_NODE_TARGET_NAME_INDEX];
									Proxy->Link(FName(ANSI_TO_TCHAR(MorphTargetNameProperty.GetStringProperty().GetData())));
								}
								Proxy->SetSkeletalMeshComponent(this);
								Proxy->Update(cg.nodes[node].finalValue);
							}
						}
						break;
					case NT_MaterialParameterUE3:
						{
							SCOPE_CYCLE_COUNTER(STAT_FaceFX_MaterialPassTime);

							// Update any material parameter nodes that may be in the Face Graph.
							FFaceFXMaterialParameterProxy* Proxy = reinterpret_cast<FFaceFXMaterialParameterProxy*>(cg.nodes[node].pUserData);
							// If the proxy is invalid create one.
							FxBool bShouldLink = FxFalse;
							if( !Proxy )
							{
								FFaceFXMaterialParameterProxy* NewMaterialParameterProxy = new FFaceFXMaterialParameterProxy();
								cg.nodes[node].pUserData = NewMaterialParameterProxy;
								bShouldLink = FxTrue;
								Proxy = NewMaterialParameterProxy;
							}

							if( Proxy )
							{
								// If the proxy requests to be re-linked, link it up.
								if( bShouldLink || bShouldClientRelink )
								{
									const FxFaceGraphNodeUserProperty& MaterialSlotProperty = 
										cg.nodes[node].userProperties[FX_MATERIAL_PARAMETER_NODE_MATERIAL_SLOT_ID_INDEX];
									const FxFaceGraphNodeUserProperty& ParameterNameProperty =
										cg.nodes[node].userProperties[FX_MATERIAL_PARAMETER_NODE_PARAMETER_NAME_INDEX];
									Proxy->Link(MaterialSlotProperty.GetIntegerProperty(), FName(ANSI_TO_TCHAR(ParameterNameProperty.GetStringProperty().GetData())));
								}
								Proxy->SetSkeletalMeshComponent(this);
								Proxy->Update(cg.nodes[node].finalValue);
							}
						}
						break;
					default:
						break;
					}
				}

				{
					SCOPE_CYCLE_COUNTER(STAT_FaceFX_BoneBlendingPassTime);
					FxMasterBoneList& MasterBoneList = FaceFXActor->GetMasterBoneList();
					const FxSize NumBones = FaceFXActorInstance->GetNumBones();
					
					if( FaceFXBlendMode == FXBM_Additive )
					{
						// Additive blending of Face FX transforms in parent bone space

						for( FxSize i = 0; i < NumBones; ++i )
						{
							const FxInt32 ClientIndex = MasterBoneList.GetRefBoneClientIndex(i);
							if( FX_INT32_MAX != ClientIndex )
							{
								// Face FX local bone transform
								FxVec3 pos, scale;
								FxQuat rot;
								FxReal weight;
								FaceFXActorInstance->GetBone(i, pos, rot, scale, weight);
								const FVector		Position(pos.x, -pos.y, pos.z);
								const FQuat			Rotation(rot.x, -rot.y, rot.z, rot.w);

								FBoneAtom			FaceFXAtom	= FBoneAtom(Rotation, Position, 1.f);
								FaceFXAtom.Rotation.Normalize();

								// Face FX reference pose bone transform
								const FxBone&		FFXRefBoneTransform = MasterBoneList.GetRefBone(i);
								const FxVec3&		FFXRefBonePos		= FFXRefBoneTransform.GetPos();
								const FxQuat&		FFXRefBoneRot		= FFXRefBoneTransform.GetRot();

								const FVector		RefPosition(FFXRefBonePos.x, -FFXRefBonePos.y, FFXRefBonePos.z);
								const FQuat			RefRotation(FFXRefBoneRot.x, -FFXRefBoneRot.y, FFXRefBoneRot.z, FFXRefBoneRot.w);

								FBoneAtom	RefSkelAtom	= FBoneAtom(RefRotation, RefPosition, 1.f);
								RefSkelAtom.Rotation.Normalize();

								// Add local delta transform to current local transform.
								// @fixme laurent -- the '+' operator for FBoneAtoms doesn't work properly with the Rotation component.
								// It's the using the blend special case, but that doesn't work for quaterion addition.
								LocalTransforms(ClientIndex).Rotation		= (FaceFXAtom.Rotation * (-RefSkelAtom.Rotation)) * LocalTransforms(ClientIndex).Rotation;
								LocalTransforms(ClientIndex).Translation	= LocalTransforms(ClientIndex).Translation + (FaceFXAtom.Translation - RefSkelAtom.Translation);
								LocalTransforms(ClientIndex).Scale			= LocalTransforms(ClientIndex).Scale + (FaceFXAtom.Scale - RefSkelAtom.Scale);
								
								// make sure rotation is normalized
								LocalTransforms(ClientIndex).Rotation.Normalize();
							}
						}
					}
					else
					{
						// Original path for Face FX, overwrite skeleton local transforms.
						for( FxSize i = 0; i < NumBones; ++i )
						{
							const FxInt32 ClientIndex = MasterBoneList.GetRefBoneClientIndex(i);
							if( FX_INT32_MAX != ClientIndex )
							{
								FxVec3 pos, scale;
								FxQuat rot;
								FxReal weight;
								FaceFXActorInstance->GetBone(i, pos, rot, scale, weight);
								const FVector		Position(pos.x, -pos.y, pos.z);
								const FQuat			Rotation(rot.x, -rot.y, rot.z, rot.w);
								LocalTransforms(ClientIndex) = FBoneAtom(Rotation, Position, 1.f);
							}
						}
					}
				}

				{
					SCOPE_CYCLE_COUNTER(STAT_FaceFX_EndFrameTime);
					FaceFXActorInstance->EndFrame();
				}
			}
		}
	}
#endif
}

UBOOL USkeletalMeshComponent::PlayFaceFXAnim(UFaceFXAnimSet* FaceFXAnimSetRef, const FString& AnimName, const FString& GroupName)
{
#if WITH_FACEFX
	//debugf(TEXT("PlayFaceFXAnim on: %s, AnimSet: %s, GroupName: %s, AnimName: %s"), *Owner->GetName(), *FaceFXAnimSetRef->GetFullName(), *GroupName, *AnimName);

	if( FaceFXActorInstance )
	{
		// See if AnimSet needs to be mounted
		if( FaceFXAnimSetRef )
		{
			if( SkeletalMesh && SkeletalMesh->FaceFXAsset )
			{
				// Ensure AnimSet is mounted - this should be fine to call if it is already mounted (will do nothing).
				SkeletalMesh->FaceFXAsset->MountFaceFXAnimSet(FaceFXAnimSetRef);
			}
		}
		
		// Start the FaceFX animation playing
		UBOOL bPlaySuccess = FaceFXActorInstance->PlayAnim(TCHAR_TO_ANSI(*AnimName), TCHAR_TO_ANSI(*GroupName));

		// If that worked, we want to start some audio
		if( bPlaySuccess )
		{
			// If we have an animation playing, and an Owner Actor
			const FxAnim* Anim = FaceFXActorInstance->GetCurrentAnim();
			if( Anim && Owner )
			{
				// Get the audio component even if we do not have a sound to play, so we can clear any existing sound on it.
				// Use the virtual function in Actor to do this.

				// Hackette here - in Matinee we want to use the C++ function, because we can't run events in the editor :(
				if(GIsEditor && !GWorld->HasBegunPlay())
				{
					CachedFaceFXAudioComp = Owner->PreviewGetFaceFXAudioComponent();
				}
				// In real game, use script event.
				else
				{
					CachedFaceFXAudioComp = Owner->eventGetFaceFXAudioComponent();
				}

				USoundCue* Sound = reinterpret_cast<USoundCue*>(Anim->GetSoundCuePointer());
				
				// If we got the sound, get an AudioComponent to play it on.
				if( CachedFaceFXAudioComp )
				{
					CachedFaceFXAudioComp->Stop();
					CachedFaceFXAudioComp->SoundCue = Sound;
				}
			}

			return TRUE;
		}
		else
		{
			FString AnimSetName = FaceFXAnimSetRef ? FaceFXAnimSetRef->GetFullName() : TEXT("None");
			warnf(TEXT("Failed to play FaceFX Animation on: %s, AnimSet: %s, GroupName: %s, AnimName: %s"), *Owner->GetName(), *AnimSetName, *GroupName, *AnimName);

			StopFaceFXAnim();
		}
	}
#endif // WITH_FACEFX

	return FALSE;
}

void USkeletalMeshComponent::StopFaceFXAnim( void )
{
#if WITH_FACEFX
	if( FaceFXActorInstance )
	{
		//debugf(TEXT("StopFaceFXAnim on: %s AnimName: %s"), *Owner->GetName(), FaceFXActorInstance->GetAnimPlayer().GetCurrentAnimName().GetAsCstr());
		// If there is an AudioComponent for this animation, stop it
		if( CachedFaceFXAudioComp )
		{
			CachedFaceFXAudioComp->Stop();
			CachedFaceFXAudioComp->SoundCue = NULL;

			// ..and release ref to it.
			CachedFaceFXAudioComp = NULL;
		}

		// Stop the animation playback.
		FaceFXActorInstance->StopAnim();
	}
#endif // WITH_FACEFX
}


/** Returns TRUE if Face FX is currently playing an animation */
UBOOL USkeletalMeshComponent::IsPlayingFaceFXAnim()
{
#if WITH_FACEFX
	if( !FaceFXActorInstance )
	{
		return FALSE;
	}

	return (FaceFXActorInstance->IsPlayingAnim() == FxTrue);
#else
	return FALSE;
#endif
}


void USkeletalMeshComponent::DeclareFaceFXRegister( const FString& RegName )
{
#if WITH_FACEFX
	// This will not add duplicate entries.
	FFaceFXRegMap::AddRegisterMapping(*RegName);
#endif // WITH_FACEFX
}

FLOAT USkeletalMeshComponent::GetFaceFXRegister( const FString& RegName )
{
#if WITH_FACEFX
	if( FaceFXActorInstance )
	{
		FFaceFXRegMapEntry* pRegMapEntry = FFaceFXRegMap::GetRegisterMapping(*RegName);
		if( pRegMapEntry )
		{
			return FaceFXActorInstance->GetRegister(pRegMapEntry->FaceFXRegName);
		}
		else
		{
			debugf(TEXT("FaceFX: WARNING Attempt to read from undeclared register %s"), *RegName);
		}
	}
#endif // WITH_FACEFX
	return 0.0f;
}

void USkeletalMeshComponent::SetFaceFXRegister( const FString& RegName, FLOAT RegVal, BYTE RegOp, FLOAT InterpDuration )
{
#if WITH_FACEFX
	if( FaceFXActorInstance )
	{
		FFaceFXRegMapEntry* pRegMapEntry = FFaceFXRegMap::GetRegisterMapping(*RegName);
		if( pRegMapEntry )
		{
			FxRegisterOp RegisterOp = RO_Add;
			switch( RegOp )
			{
			case FXRO_Add:
				RegisterOp = RO_Add;
				break;
			case FXRO_Multiply:
				RegisterOp = RO_Multiply;
				break;
			case FXRO_Replace:
				RegisterOp = RO_Replace;
				break;
			default:
				debugf(TEXT("FaceFX: Invalid RegOp in USkeletalMeshComponent::SetFaceFXRegister()!"));
				break;
			}
			FaceFXActorInstance->SetRegister(pRegMapEntry->FaceFXRegName, RegVal, RegisterOp, InterpDuration);
		}
		else
		{
			debugf(TEXT("FaceFX: WARNING Attempt to write to undeclared register %s"), *RegName);
		}
	}
#endif // WITH_FACEFX
}

void USkeletalMeshComponent::SetFaceFXRegisterEx( const FString& RegName, BYTE RegOp, FLOAT FirstValue, FLOAT FirstInterpDuration, FLOAT NextValue, FLOAT NextInterpDuration )
{
#if WITH_FACEFX
	if( FaceFXActorInstance )
	{
		FFaceFXRegMapEntry* pRegMapEntry = FFaceFXRegMap::GetRegisterMapping(*RegName);
		if( pRegMapEntry )
		{
			FxRegisterOp RegisterOp = RO_Add;
			switch( RegOp )
			{
			case FXRO_Add:
				RegisterOp = RO_Add;
				break;
			case FXRO_Multiply:
				RegisterOp = RO_Multiply;
				break;
			case FXRO_Replace:
				RegisterOp = RO_Replace;
				break;
			default:
				debugf(TEXT("FaceFX: Invalid RegOp in USkeletalMeshComponent::SetFaceFXRegisterEx()!"));
				break;
			}
			//@todo FaceFX The second RegisterOp should be the actual NextRegisterOp, so expose it through script and properly use it here.
			FaceFXActorInstance->SetRegisterEx(pRegMapEntry->FaceFXRegName, RegisterOp, FirstValue, FirstInterpDuration, RegisterOp, NextValue, NextInterpDuration);
		}
		else
		{
			debugf(TEXT("FaceFX: WARNING Attempt to write to undeclared register %s"), *RegName);
		}
	}
#endif // WITH_FACEFX
}

/** Takes sorted array Base and then adds any elements from sorted array Insert which is missing from it, preserving order. */
static void MergeInByteArray(TArray<BYTE>& BaseArray, TArray<BYTE>& InsertArray)
{
	// Then we merge them into the array of required bones.
	INT BaseBonePos = 0;
	INT InsertBonePos = 0;

	// Iterate over each of the bones we need.
	while( InsertBonePos < InsertArray.Num() )
	{
		// Find index of physics bone
		BYTE InsertByte = InsertArray(InsertBonePos);

		// If at end of BaseArray array - just append.
		if( BaseBonePos == BaseArray.Num() )
		{
			BaseArray.AddItem(InsertByte);
			BaseBonePos++;
			InsertBonePos++;
		}
		// If in the middle of BaseArray, merge together.
		else
		{
			// Check that the BaseArray array is strictly increasing, otherwise merge code does not work.
			check( BaseBonePos == 0 || BaseArray(BaseBonePos-1) < BaseArray(BaseBonePos) );

			// Get next required bone index.
			BYTE BaseByte = BaseArray(BaseBonePos);

			// We have a bone in BaseArray not required by Insert. Thats ok - skip.
			if( BaseByte < InsertByte )
			{
				BaseBonePos++;
			}
			// Bone required by Insert is in 
			else if(BaseByte == InsertByte )
			{
				BaseBonePos++;
				InsertBonePos++;
			}
			// Bone required by Insert is missing - insert it now.
			else // BaseByte > InsertByte
			{
				BaseArray.Insert(BaseBonePos);
				BaseArray(BaseBonePos) = InsertByte;

				BaseBonePos++;
				InsertBonePos++;
			}
		}
	}
}

/** Recalculates the RequiredBones array in this SkeletalMeshComponent based on current SkeletalMesh, LOD and PhysicsAsset. */
void USkeletalMeshComponent::RecalcRequiredBones(INT LODIndex)
{
	// The list of bones we want is taken from the predicted LOD level.
	FStaticLODModel& LODModel = SkeletalMesh->LODModels(LODIndex);


	// The LODModel.RequiredBones array only includes bones that are desired for that LOD level.
	// They are also in strictly increasing order, which also infers parents-before-children.
	RequiredBones = LODModel.RequiredBones;


	// Add in any bones that may be required when mirroring.
	// JTODO: This is only required if there are mirroring nodes in the tree, but hard to know...
	if(SkeletalMesh->SkelMirrorTable.Num() == LocalAtoms.Num())
	{
		TArray<BYTE> MirroredDesiredBones;
		MirroredDesiredBones.Add(RequiredBones.Num());

		// Look up each bone in the mirroring table.
		for(INT i=0; i<RequiredBones.Num(); i++)
		{
			MirroredDesiredBones(i) = SkeletalMesh->SkelMirrorTable(RequiredBones(i)).SourceIndex;
		}

		// Sort to ensure strictly increasing order.
		Sort<USE_COMPARE_CONSTREF(BYTE, UnSkeletalComponent)>(&MirroredDesiredBones(0), MirroredDesiredBones.Num());

		// Make sure all of these are in RequiredBones, and 
		MergeInByteArray(RequiredBones, MirroredDesiredBones);
	}

	// If we have a PhysicsAsset, we also need to make sure that all the bones used by it are always updated, as its used
	// by line checks etc. We might also want to kick in the physics, which means having valid bone transforms.
	if(PhysicsAsset)
	{
		TArray<BYTE> PhysAssetBones;
		for(INT i=0; i<PhysicsAsset->BodySetup.Num(); i++ )
		{
			INT PhysBoneIndex = SkeletalMesh->MatchRefBone( PhysicsAsset->BodySetup(i)->BoneName );
			if(PhysBoneIndex != INDEX_NONE)
			{
				PhysAssetBones.AddItem(PhysBoneIndex);
			}	
		}

		// Then sort array of required bones in hierarchy order
		Sort<USE_COMPARE_CONSTREF(BYTE, UnSkeletalComponent)>( &PhysAssetBones(0), PhysAssetBones.Num() );

		// Make sure all of these are in RequiredBones.
		MergeInByteArray(RequiredBones, PhysAssetBones);
	}

	// Ensure that we have a complete hierarchy down to those bones.
	UAnimNode::EnsureParentsPresent(RequiredBones, SkeletalMesh);
}

#if ENABLE_GETBONEATOM_STATS
IMPLEMENT_COMPARE_CONSTREF( FAnimNodeTimeStat, UnSkeletalComponent, { return (B.NodeExclusiveTime > A.NodeExclusiveTime) ? -1 : 1; } )
#endif

/**
 * Update the SpaceBases array of component-space bone transforms.
 * This will evaluate any animation blend tree if present (or use the reference pose if not).
 * It will then blend in any physics information from a PhysicsAssetInstance based on the PhysicsWeight value.
 * Then evaluates any root bone options to apply translation to the owning Actor if desired.
 * Finally it composes all the relative transforms to calculate component-space transforms for each bone. 
 * Applying per-bone controllers is done as part of multiplying the relative transforms down the heirarchy.
 *
 * NOTE: DeltaTime is optional and can be zero!!!
 *
 *	@param	bTickFaceFX		Passed to FaceFX to tell it to update facial state based on global clock or wave position.
							Set to false if you are forcing a pose outside of this function and do not want it over-ridden.
 */
void USkeletalMeshComponent::UpdateSkelPose( FLOAT DeltaTime, UBOOL bTickFaceFX )
{
	SCOPE_CYCLE_COUNTER(STAT_UpdateSkelPose);

	// Can't do anything without a SkeletalMesh
	if( !SkeletalMesh )
	{
		return;
	}

	const UBOOL bOldIgnoreControllers = bIgnoreControllers;

	// Allocate transforms if not present.
	if( SpaceBases.Num() != SkeletalMesh->RefSkeleton.Num() )
	{
		SpaceBases.Empty();
		SpaceBases.Add( SkeletalMesh->RefSkeleton.Num() );

		// Controls sometimes use last frames position of a bone. But if that is not valid (ie array is freshly allocated)
		// we need to turn them off.
		bIgnoreControllers = TRUE;
	}

	if( LocalAtoms.Num() != SkeletalMesh->RefSkeleton.Num() )
	{
		LocalAtoms.Empty();
		LocalAtoms.Add( SkeletalMesh->RefSkeleton.Num() );
	}

	// Do nothing more if no bones in skeleton.
	if( SpaceBases.Num() == 0 )
	{
		bIgnoreControllers = bOldIgnoreControllers;
		return;
	}

	// Update bones transform from animations (if present)

	// Predict the best (min) LOD level we are going to need. Basically we use the Min (best) LOD the renderer desired last frame.
	// Because we update bones based on this LOD level, we have to update bones to this LOD before we can allow rendering at it.

	// Support forcing to a particular LOD.
	if(ForcedLodModel > 0)
	{
		PredictedLODLevel = ::Clamp(ForcedLodModel - 1, 0, SkeletalMesh->LODModels.Num()-1);
	}
	else
	{
		// If no MeshObject - just assume lowest LOD.
		if(MeshObject)
		{
			PredictedLODLevel = ::Clamp(MeshObject->MinDesiredLODLevel + GSystemSettings.SkeletalMeshLODBias, 0, SkeletalMesh->LODModels.Num()-1);
		}
		else
		{
			PredictedLODLevel = SkeletalMesh->LODModels.Num()-1;
		}
	}

	// See if LOD has changed. If so, we need to recalc required bones.
	if(PredictedLODLevel != OldPredictedLODLevel)
	{
		bRequiredBonesUpToDate = FALSE;
	}
	OldPredictedLODLevel = PredictedLODLevel;

	const UBOOL bRenderedRecently = (GWorld->GetTimeSeconds() - LastRenderTime) < 1.0f;

	// Read back MaxDistanceFactor from the render object.
	if(MeshObject)
	{
		MaxDistanceFactor = MeshObject->MaxDistanceFactor;

#if !FINAL_RELEASE
		// Only chart DistanceFactor if it was actually rendered recently
		if(bChartDistanceFactor && bRenderedRecently)
		{
			AddDistanceFactorToChart(MaxDistanceFactor);
		}
#endif
	}

	// See if this mesh is far enough from the viewer we should stop updating kinematic rig
	UBOOL bNewNotUpdateKinematic = FALSE;
	if(	MinDistFactorForKinematicUpdate > 0.f && 
		(MaxDistanceFactor < MinDistFactorForKinematicUpdate || !bRenderedRecently) )
	{
		bNewNotUpdateKinematic = TRUE;
	}

	// Flag to indicate this is a frame where we have just turned on kinematic updating of bodies again.  
	UBOOL bJustEnabledKinematicUpdate = FALSE;  
	UBOOL bKinematicUpdateStateChanged = FALSE;  

	// Turn off BlockRigidBody when we stop updating kinematics, and turn it back on when we start again.  
	if(bNotUpdatingKinematicDueToDistance && !bNewNotUpdateKinematic)   
	{   
		bKinematicUpdateStateChanged = TRUE;  
		bJustEnabledKinematicUpdate = TRUE;   
	}  
	else if(!bNotUpdatingKinematicDueToDistance && bNewNotUpdateKinematic)  
	{  
		bKinematicUpdateStateChanged = TRUE;  
	}  
	bNotUpdatingKinematicDueToDistance = bNewNotUpdateKinematic;  

	// This also looks at bNotUpdatingKinematicDueToDistance and sets the collision state accordingly  
	if(bKinematicUpdateStateChanged)
	{
		SetBlockRigidBody(BlockRigidBody);
	}

	// Recalculate the RequiredBones array, if necessary
	if(!bRequiredBonesUpToDate)
	{
		RecalcRequiredBones(PredictedLODLevel);
		bRequiredBonesUpToDate = TRUE;
	}

	// Root motion extracted for this call
	FBoneAtom	ExtractedRootMotionDelta	= FBoneAtom::Identity;
	INT			bHasRootMotion				= 0;
	{
		SCOPE_CYCLE_COUNTER(STAT_AnimBlendTime);
		if( Animations && !bForceRefpose )
		{
			// Check we don't have any nasty AnimTree sharing going on or anything.
			check(Animations->SkelComponent == this);

			// Increment the cache tag, so caches are invalidated on the nodes.
			CachedAtomsTag++;

#if ENABLE_GETBONEATOM_STATS
			BoneAtomBlendStats.Empty();
#endif
			Animations->GetBoneAtoms(LocalAtoms, RequiredBones, ExtractedRootMotionDelta, bHasRootMotion);

#if ENABLE_GETBONEATOM_STATS
			if(GShouldLogOutAFrameOfSkelCompTick)
			{
				// Sort results (slowest first)
				Sort<USE_COMPARE_CONSTREF(FAnimNodeTimeStat,UnSkeletalComponent)>( &BoneAtomBlendStats(0), BoneAtomBlendStats.Num() );

				debugf(TEXT(" ======= GetBoneAtom - TIMING - %s %s"), *GetPathName(), SkeletalMesh?*SkeletalMesh->GetName():TEXT("NONE"));
				for(INT i=0; i<BoneAtomBlendStats.Num(); i++)
				{
					debugf(TEXT("%fms\t%s"), BoneAtomBlendStats(i).NodeExclusiveTime * 1000.f, *BoneAtomBlendStats(i).NodeName.ToString());
				}
				debugf(TEXT(" ======="));
			}
#endif
		}
		else
		{
			UAnimNode::FillWithRefPose(LocalAtoms, RequiredBones, SkeletalMesh->RefSkeleton);
		}
	}

	// Root Motion 

	// Root motion movement is done only once per tick. 
	// Because RootMotionDelta is relative to the last time the animation was ticked. 
	// So we can't just arbitrarily call that function and have root motion work, the same offsets would be applied every time.
	// Because this function can be called at any time with a delta time of 0 to update the skeleton...
	// This is probably going to be a problem with Matinee. In that case, this system would need to refactored to support that.
	if( DeltaTime > 0.f )
	{
		// If PendingRMM has changed, set it
		if( PendingRMM != OldPendingRMM )
		{
			// Already set, do nothing
			if( RootMotionMode == PendingRMM )
			{
				OldPendingRMM = PendingRMM;
			}
			// delay by a frame if setting to RMM_Ignore AND animation extracted root motion on this frame.
			// This is to force physics to process the entire root motion.
			else if( PendingRMM != RMM_Ignore || !bHasRootMotion || bRMMOneFrameDelay == 1 )
			{
				RootMotionMode		= PendingRMM;
				OldPendingRMM		= PendingRMM;
				bRMMOneFrameDelay	= 0;
			}
			else
			{
				bRMMOneFrameDelay	= 1;
			}
		}

		// if root motion is requested, then transform it from mesh space to world space so it can be used.
		if( bHasRootMotion && RootMotionMode != RMM_Ignore )
		{
#if 0 // DEBUG
			debugf(TEXT("%3.2f [%s] Extracted RM, PreProcessing, Translation: %3.3f, vect: %s, RootMotionAccelScale: %s"), 
				GWorld->GetTimeSeconds(), *Owner->GetName(), ExtractedRootMotionDelta.Translation.Size(), *ExtractedRootMotionDelta.Translation.ToString(), *RootMotionAccelScale.ToString());
#endif
			// Transform mesh space root delta translation to world space
			ExtractedRootMotionDelta.Translation = LocalToWorld.TransformNormal(ExtractedRootMotionDelta.Translation);

			// Scale RootMotion translation in Mesh Space.
			if( RootMotionAccelScale != FVector(1.f) )
			{
				ExtractedRootMotionDelta.Translation *= RootMotionAccelScale;
			}

			// If Owner required a Script event forwarded when root motion has been extracted, forward it
			if( Owner && bRootMotionExtractedNotify )
			{
				Owner->eventRootMotionExtracted(this, ExtractedRootMotionDelta);
			}

			// Root Motion delta is accumulated every time it is extracted.
			// This is because on servers using autonomous physics, physics updates and ticking are out of synch.
			// So 2 physics updates can happen in a row, or 2 animation updates, effectively
			// making client and server out of synch.
			// So root motion is accumulated, and reset when used by physics.
			RootMotionDelta.Translation	+= ExtractedRootMotionDelta.Translation;
			RootMotionVelocity			= ExtractedRootMotionDelta.Translation / DeltaTime;
		}
		else
		{
			RootMotionDelta.Translation = FVector(0.f);
			RootMotionVelocity			= FVector(0.f);
		}

#if 0 // DEBUG
		static FVector	AccumulatedRMTranslation = FVector(0.f);
		{
			if( RootMotionMode != RMM_Ignore )
			{
				AccumulatedRMTranslation	+= ExtractedRootMotionDelta.Translation;
			}
			else
			{
				AccumulatedRMTranslation	= FVector(0.f);
			}

			if( RootMotionMode != RMM_Ignore )
			{
				debugf(TEXT("%3.2f [%s] RM Translation: %3.3f, vect: %s"), GWorld->GetTimeSeconds(), *Owner->GetName(), RootMotionDelta.Translation.Size(), *RootMotionDelta.Translation.ToString());
				debugf(TEXT("%3.2f [%s] RM Velocity: %3.3f, vect: %s"), GWorld->GetTimeSeconds(), *Owner->GetName(), RootMotionVelocity.Size(), *RootMotionVelocity.ToString());
				debugf(TEXT("%3.2f [%s] RM AccumulatedRMTranslation: %3.3f, vect: %s"), GWorld->GetTimeSeconds(), *Owner->GetName(), AccumulatedRMTranslation.Size(), *AccumulatedRMTranslation.ToString());
			}
		}
#endif

		if( bHasRootMotion && RootMotionRotationMode != RMRM_Ignore )
		{
			FQuat	MeshToWorldQuat(LocalToWorld);

			// Transform mesh space delta rotation to world space.
			RootMotionDelta.Rotation = MeshToWorldQuat * ExtractedRootMotionDelta.Rotation * (-MeshToWorldQuat);
			RootMotionDelta.Rotation.Normalize();
		}
		else
		{
			RootMotionDelta.Rotation = FQuat::Identity;
		}

#if 0 // DEBUG ROOT ROTATION
		if( RootMotionRotationMode != RMRM_Ignore )
		{
			const FRotator DeltaRotation = FQuatRotationTranslationMatrix(RootMotionDelta.Rotation, FVector(0.f)).Rotator();
			debugf(TEXT("%3.2f Root Rotation: %s"), GWorld->GetTimeSeconds(), *DeltaRotation.ToString());
		}
#endif

		// Motion applied right away
		if( bHasRootMotion && 
			(RootMotionMode == RMM_Translate || RootMotionRotationMode == RMRM_RotateActor || 
			(RootMotionMode == RMM_Ignore && PreviousRMM == RMM_Translate)) )	// If root motion was just turned off, forward remaining root motion.
		{
			/** 
			 * Delay applying instant translation for one frame
			 * So we check for PreviousRMM to be up to date with the current root motion mode.
			 * We need to do this because in-game physics have already been applied for this tick.
			 * So we want root motion to kick in for next frame.
			 */
			const UBOOL		bCanDoTranslation	= (RootMotionMode == RMM_Translate && PreviousRMM == RMM_Translate);
			const FVector	InstantTranslation	= bCanDoTranslation ? RootMotionDelta.Translation : FVector(0.f);

			const UBOOL		bCanDoRotation		= (RootMotionRotationMode == RMRM_RotateActor);
			const FRotator	InstantRotation		=  bCanDoRotation ? FQuatRotationTranslationMatrix(RootMotionDelta.Rotation, FVector(0.f)).Rotator() : FRotator(0,0,0);

			if( Owner && (!InstantRotation.IsZero() || InstantTranslation.SizeSquared() > SMALL_NUMBER) )
			{
#if 0 // DEBUG ROOT MOTION
				debugf(TEXT("%3.2f Root Motion Instant. DeltaRot: %s"), GWorld->GetTimeSeconds(), *InstantRotation.ToString());
#endif
				// Transform mesh directly. Doesn't take in-game physics into account.
				FCheckResult Hit(1.f);
				GWorld->MoveActor(Owner, InstantTranslation, Owner->Rotation + InstantRotation, 0, Hit);

				// If we have used translation, reset the accumulator.
				if( bCanDoTranslation )
				{
					RootMotionDelta.Translation = FVector(0.f);
				}

				if( bCanDoRotation )
				{
					Owner->DesiredRotation = Owner->Rotation;

					// Update DesiredRotation for AI Controlled Pawns
					APawn* PawnOwner = Cast<APawn>(Owner);
					if( PawnOwner && PawnOwner->Controller && PawnOwner->Controller->bForceDesiredRotation )
					{
						PawnOwner->Controller->DesiredRotation = Owner->Rotation;
					}
				}
			}
		}

		// Track root motion mode changes
		if( RootMotionMode != PreviousRMM )
		{
			// notify owner that root motion mode changed. 
			// if RootMotionMode != RMM_Ignore, then on next frame root motion will kick in.
			if( bRootMotionModeChangeNotify && Owner )
			{
				Owner->eventRootMotionModeChanged(this);
			}
			PreviousRMM = RootMotionMode;
		}
	}

#if 0 // DEBUG root Rotation
	if( Owner )
	{
		Owner->DrawDebugLine(Owner->Location, Owner->Location + Owner->Rotation.Vector() * 200.f, 255, 0, 0, FALSE);
	}
#endif

	if( bForceDiscardRootMotion )
	{
		LocalAtoms(0).Translation	= FVector(0.f);
		LocalAtoms(0).Rotation		= FQuat::Identity;
	}

	// Remember the root bone's translation so we can move the bounds.
	RootBoneTranslation = LocalAtoms(0).Translation - SkeletalMesh->RefSkeleton(0).BonePos.Position;

	// Update the ActiveMorphs array.
	UpdateActiveMorphs();

	if( SkeletalMesh->FaceFXAsset )
	{
		SCOPE_CYCLE_COUNTER(STAT_UpdateFaceFX);
		// Do FaceFX processing.
		UpdateFaceFX(LocalAtoms, bTickFaceFX);
	}

	// We need the world space bone transforms now for two reasons:
	// 1) we don't have physics, so we will not be revisiting this skeletal mesh in UpdateTransform.
	// 2) we do have physics, and want to update the physics state from the animation.
	// This will do controllers and the like.

	//const DOUBLE ComposeSkeleton_Start = appSeconds();

	ComposeSkeleton(LocalAtoms, RequiredBones);

// 	if( GShouldLogOutAFrameOfSkelCompTick == TRUE )
// 	{
// 		const DOUBLE ComposeSkeleton_Time = (appSeconds() - ComposeSkeleton_Start) * 1000.f;
// 		debugf( TEXT( "   ComposeSkeleton_Time: %f" ), ComposeSkeleton_Time );
// 	}

	// UpdateRBBonesFromSpaceBases, updating the physical skeleton from the animated one, is done inside UpdateTransform.

	// When we re-enable kinematic update, we teleport bodies to the right place now (handles flappy bits not getting jerked when updated).
	if(bJustEnabledKinematicUpdate)
	{
		UpdateRBBonesFromSpaceBases(LocalToWorld, TRUE, TRUE);
	}

	// If desired, pass the animation data to the physics joints so they can be used by motors.
	if(PhysicsAssetInstance && bUpdateJointsFromAnimation)
	{
		UpdateRBJointMotors();
	}

	bIgnoreControllers = bOldIgnoreControllers;
	bHasHadPhysicsBlendedIn = FALSE;
}

/** Used by the SkelControlFootPlacement to line-check against the world and find the point to place the foot bone. */
UBOOL USkeletalMeshComponent::LegLineCheck(const FVector& Start, const FVector& End, FVector& HitLocation, FVector& HitNormal)
{
	if(Owner)
	{
		FCheckResult Hit(1.f);
		UBOOL bHit = !GWorld->SingleLineCheck( Hit, Owner, End, Start, TRACE_AllBlocking );
		if(bHit)
		{
			HitLocation = Hit.Location;
			HitNormal = Hit.Normal;
			return true;
		}
	}

	return false;
}

//
//	USkeletalMeshComponent::UpdateBounds
//

void USkeletalMeshComponent::UpdateBounds()
{
	SCOPE_CYCLE_COUNTER(STAT_UpdateSkelMeshBounds);

	FVector DrawScale = Scale * Scale3D;
	if (Owner != NULL)
	{
		DrawScale *= Owner->DrawScale * Owner->DrawScale3D;
	}

	// Can only use the PhysicsAsset to calculate the bounding box if we are not non-uniformly scaling the mesh.
	UBOOL bCanUsePhysicsAsset = (DrawScale.IsUniform() && SpaceBases.Num() > 0);

	// For AnimSet Viewer, use 'bounds preview' physics asset if present.
	if(SkeletalMesh && SkeletalMesh->BoundsPreviewAsset && bCanUsePhysicsAsset)
	{
		Bounds = FBoxSphereBounds(SkeletalMesh->BoundsPreviewAsset->CalcAABB(this));
	}
	// If we have a PhysicsAsset, and we can use it, do so to calc bounds.
	else if( PhysicsAsset && bCanUsePhysicsAsset )
	{
		Bounds = FBoxSphereBounds(PhysicsAsset->CalcAABB(this));
	}
	// Use ParentAnimComponent's PhysicsAsset, if we don't have one and it does
	else if( ParentAnimComponent && ParentAnimComponent->PhysicsAsset && bCanUsePhysicsAsset )
	{
		Bounds = FBoxSphereBounds(ParentAnimComponent->PhysicsAsset->CalcAABB(this));
	}
	// Fallback is to use the one from the skeletal mesh. Usually pretty bad.
	else if( SkeletalMesh )
	{
		FBoxSphereBounds RootAdjustedBounds = SkeletalMesh->Bounds;

		// Adjust bounds by root bone translation
		RootAdjustedBounds.Origin += RootBoneTranslation;
		Bounds = RootAdjustedBounds.TransformBy(LocalToWorld);
	}
	else
	{
		Super::UpdateBounds();
		return;
	}

	// Add bounds of any per-poly collision data.
	if(SkeletalMesh && SpaceBases.Num() > 0)
	{
		check(SkeletalMesh->PerPolyCollisionBones.Num() == SkeletalMesh->PerPolyBoneKDOPs.Num());
		for(INT i=0; i<SkeletalMesh->PerPolyBoneKDOPs.Num(); i++)
		{
			FPerPolyBoneCollisionData& Data = SkeletalMesh->PerPolyBoneKDOPs(i);
			INT BoneIndex = SkeletalMesh->MatchRefBone(SkeletalMesh->PerPolyCollisionBones(i));
			if(Data.KDOPTree.Triangles.Num() > 0  && BoneIndex != INDEX_NONE)
			{
				check(Data.KDOPTree.Nodes.Num() > 0);
				FBox PerPolyBoneBox = Data.KDOPTree.Nodes(0).BoundingVolume.ToFBox();
				Bounds = Bounds + PerPolyBoneBox.TransformBy(GetBoneMatrix(BoneIndex));
			}
		}
	}
	UpdateClothBounds();
}

FMatrix USkeletalMeshComponent::GetBoneMatrix(DWORD BoneIdx) const
{
	// Handle case of use a ParentAnimComponent - get bone matrix from there.
	if(ParentAnimComponent)
	{
		if(BoneIdx < (DWORD)ParentBoneMap.Num())
		{
			INT ParentBoneIndex = ParentBoneMap(BoneIdx);

			// If ParentBoneIndex is valid, grab matrix from ParentAnimComponent.
			if(	ParentBoneIndex != INDEX_NONE && 
				ParentBoneIndex < ParentAnimComponent->SpaceBases.Num())
			{
				return ParentAnimComponent->SpaceBases(ParentBoneIndex) * LocalToWorld;
			}
			else
			{
#if !PS3 // will be caught on PC, hopefully
				debugf( NAME_Warning, TEXT("GetBoneMatrix : ParentBoneIndex(%d) out of range of ParentAnimComponent->SpaceBases for %s"), BoneIdx, *this->GetFName().ToString() );
#endif
				return FMatrix::Identity;
			}
		}
		else
		{
#if !PS3 // will be caught on PC, hopefully
			debugf( NAME_Warning, TEXT("GetBoneMatrix : BoneIndex(%d) out of range of ParentBoneMap for %s"), BoneIdx, *this->GetFName().ToString() );
#endif
			return FMatrix::Identity;
		}
	}
	else
	{
		if( SpaceBases.Num() && BoneIdx < (DWORD)SpaceBases.Num() )
		{
			return SpaceBases(BoneIdx) * LocalToWorld;
		}
		else
		{
#if !PS3 // will be caught on PC, hopefully
			debugf( NAME_Warning, TEXT("GetBoneMatrix : BoneIndex(%d) out of range of SpaceBases for %s owned by %s"), BoneIdx, *this->GetFName().ToString(),this->Owner?*this->Owner->GetFName().ToString():TEXT("NULL") );
#endif
			return FMatrix::Identity;
		}
	}
}

void USkeletalMeshComponent::execGetBoneMatrix( FFrame& Stack, RESULT_DECL )
{
	P_GET_INT(BoneIdx);
	P_FINISH;

	*(FMatrix*)Result = GetBoneMatrix(BoneIdx);
}

/**
 * Find the index of bone by name. Looks in the current SkeletalMesh being used by this SkeletalMeshComponent.
 * 
 * @param BoneName Name of bone to look up
 * 
 * @return Index of the named bone in the current SkeletalMesh. Will return INDEX_NONE if bone not found.
 *
 * @see USkeletalMesh::MatchRefBone.
 */
INT USkeletalMeshComponent::MatchRefBone( FName BoneName) const
{
	INT BoneIndex = INDEX_NONE;
	if ( BoneName != NAME_None && SkeletalMesh )
	{
		BoneIndex = SkeletalMesh->MatchRefBone( BoneName );
	}

	return BoneIndex;
}

void USkeletalMeshComponent::execMatchRefBone( FFrame& Stack, RESULT_DECL )
{
	P_GET_NAME(BoneName);
	P_FINISH;

	*(INT*)Result = MatchRefBone(BoneName);
}

void USkeletalMeshComponent::execGetParentBone(FFrame& Stack, RESULT_DECL)
{
	P_GET_NAME(BoneName);
	P_FINISH;

	INT BoneIndex = MatchRefBone(BoneName);
	if (BoneIndex != INDEX_NONE && SkeletalMesh->RefSkeleton(BoneIndex).ParentIndex > 0)
	{
		*(FName*)Result = SkeletalMesh->RefSkeleton(SkeletalMesh->RefSkeleton(BoneIndex).ParentIndex).Name;
	}
	else
	{
		*(FName*)Result = NAME_None;
	}
}

void USkeletalMeshComponent::execGetBoneNames(FFrame& Stack, RESULT_DECL)
{
	P_GET_TARRAY_REF(FName, BoneNames);
	P_FINISH;

	if (SkeletalMesh == NULL)
	{
		// no mesh, so no bones
		BoneNames.Empty();
	}
	else
	{
		// pre-size the array to avoid unnecessary reallocation
		BoneNames.Empty(SkeletalMesh->RefSkeleton.Num());
		BoneNames.Add(SkeletalMesh->RefSkeleton.Num());
		for (INT i = 0; i < SkeletalMesh->RefSkeleton.Num(); i++)
		{
			BoneNames(i) = SkeletalMesh->RefSkeleton(i).Name;
		}
	}
}

/**
 *	Sets the SkeletalMesh used by this SkeletalMeshComponent.
 *	Will also update ParentBoneMap, if we are using a ParentAnimComponent. 
 *	NB: If other things are using THIS component as a ParenAnimComponent, you must manually call UpdateParentBoneMap on them after making this call.
 * 
 * @param InSkelMesh		New SkeletalMesh to use for SkeletalMeshComponent.
 * @param bKeepSpaceBases	If true, when changing the skeletal mesh, keep the SpaceBases array around.
 */
void USkeletalMeshComponent::SetSkeletalMesh(USkeletalMesh* InSkelMesh, UBOOL bKeepSpaceBases)
{
	if (InSkelMesh == NULL || InSkelMesh == SkeletalMesh)
	{
		// do nothing if the input mesh is invalid or it's the same mesh we're already using
		return;
	}

	USkeletalMesh* OldSkelMesh = SkeletalMesh;

	// If desired - back up SpaceBases array now
	TArray<FMatrix> OldSpaceBases;
	if( bKeepSpaceBases && SkeletalMesh)
	{
		OldSpaceBases = SpaceBases;
	}

	// Use recreate context to 
	{
		FComponentReattachContext	ReattachContext(this);

		FMatrix OldSkelMatrix;
		if(SkeletalMesh)
		{
			OldSkelMatrix =  ( bForceRawOffset ?  FMatrix::Identity : FTranslationMatrix( SkeletalMesh->Origin ) ) * FRotationMatrix(SkeletalMesh->RotOrigin);
		}
		else
		{
			OldSkelMatrix = FMatrix::Identity;
		}

		SkeletalMesh = InSkelMesh;

		// Reset the animation stuff when changing mesh.
		SpaceBases.Empty();	

		InitAnimTree();

		// We want to just replace the bit of the transform due to the actual skeletal mesh.
		FMatrix OldOffset = OldSkelMatrix.Inverse() * LocalToWorld;

		if(SkeletalMesh)
		{
			LocalToWorld = ( bForceRawOffset ? FMatrix::Identity : FTranslationMatrix(SkeletalMesh->Origin) )  * FRotationMatrix(SkeletalMesh->RotOrigin) * OldOffset;
		}
		else
		{
			LocalToWorld = OldOffset;
		}

		// If this component refers to some parent, make sure it is up to date.
		// No way to tell if other things refer to this - that has to be done manually by user unfortuntalely.
		UpdateParentBoneMap();

		// Indicate that 'required bones' array will need to be recalculated.
		bRequiredBonesUpToDate = FALSE;
	}

	// If we backed up the SpaceBases array - find bones by name that are in both meshes and copy matrix.
	// TODO: This leaves any _new_ bones in their animated pose - is that bad? What else should we do?
	if(OldSpaceBases.Num() > 0 && SpaceBases.Num() > 0)
	{
		check(OldSpaceBases.Num() == OldSkelMesh->RefSkeleton.Num());
		check(SpaceBases.Num() == SkeletalMesh->RefSkeleton.Num());

		for(INT i=0; i<OldSkelMesh->RefSkeleton.Num(); i++)
		{
			FName BoneName = OldSkelMesh->RefSkeleton(i).Name;
			INT BoneIndex = SkeletalMesh->MatchRefBone(BoneName);
			if(BoneIndex != INDEX_NONE)
			{
				SpaceBases(BoneIndex) = OldSpaceBases(i);
			}
		}
	}
}


/**
 *	Sets the PhysicsAsset used by this SkeletalMeshComponent.
 * 
 * @param InPhysicsAsset	New PhysicsAsset to use for SkeletalMeshComponent.
 * @param bForceReInit		Force asset to be re-initialised.
 */
void USkeletalMeshComponent::SetPhysicsAsset(UPhysicsAsset* InPhysicsAsset, UBOOL bForceReInit)
{
	// If this is different from what we have now, or we should have an instance but for whatever reason it failed last time, teardown/recreate now.
	if(bForceReInit || (InPhysicsAsset != PhysicsAsset) || (bHasPhysicsAssetInstance && !PhysicsAssetInstance) )
	{
		// SkelComp had a physics instance, then terminate it.
		if( bHasPhysicsAssetInstance )
		{
			TermArticulated(NULL);

			{
				// Need to update scene proxy, because it keeps a ref to the PhysicsAsset.
				FPrimitiveSceneAttachmentContext ReattachContext(this);
				PhysicsAsset = InPhysicsAsset;
			}

			// Component should be re-attached here, so create physics.
			if( PhysicsAsset )
			{
				// Initialize new Physics Asset
				InitArticulated(bSkelCompFixed);
			}
		}
		else
		{
			// If PhysicsAsset hasn't been instanced yet, just update the template.
			PhysicsAsset = InPhysicsAsset;
		}

		// Indicate that 'required bones' array will need to be recalculated.
		bRequiredBonesUpToDate = FALSE;
	}
}

/** Turn on and off cloth simulation for this skeletal mesh. */
void USkeletalMeshComponent::SetEnableClothSimulation(UBOOL bInEnable)
{
	if(!ClothSim && bInEnable)
	{
		FRBPhysScene* UseScene = GWorld->RBPhysScene;
		InitClothSim(UseScene);
	}
	else if(ClothSim && !bInEnable)
	{
		TermClothSim(NULL);
	}

	bEnableClothSimulation = bInEnable;
}

void USkeletalMeshComponent::execSetEnableClothSimulation( FFrame& Stack, RESULT_DECL )
{
	P_GET_UBOOL(bInEnable);
	P_FINISH;

	SetEnableClothSimulation(bInEnable);
}

/** Change the ParentAnimComponent that this component is using as the source for its bone transforms. */
void USkeletalMeshComponent::SetParentAnimComponent(USkeletalMeshComponent* NewParentAnimComp)
{
	ParentAnimComponent = NewParentAnimComp;

	UpdateParentBoneMap();
}

/**
 *	Sets the value of the bForceWireframe flag and reattaches the component as necessary.
 *
 *	@param	InForceWireframe		New value of bForceWireframe.
 */
void USkeletalMeshComponent::SetForceWireframe(UBOOL InForceWireframe)
{
	if(bForceWireframe != InForceWireframe)
	{
		bForceWireframe = InForceWireframe;
		FComponentReattachContext ReattachContext(this);
	}
}

/**
 * Find a named AnimSequence from the AnimSets array in the SkeletalMeshComponent. 
 * This searches array from end to start, so specific sequence can be replaced by putting a set containing a sequence with the same name later in the array.
 * 
 * @param AnimSeqName Name of AnimSequence to look for.
 * 
 * @return Pointer to found AnimSequence. Returns NULL if could not find sequence with that name.
 */
UAnimSequence* USkeletalMeshComponent::FindAnimSequence(FName AnimSeqName)
{
	if( AnimSeqName == NAME_None )
	{
		return NULL;
	}

	// Work from last element in list backwards, so you can replace a specific sequence by adding a set later in the array.
	for(INT i=AnimSets.Num()-1; i>=0; i--)
	{
		if( AnimSets(i) )
		{
			UAnimSequence* FoundSeq = AnimSets(i)->FindAnimSequence(AnimSeqName);
			if( FoundSeq )
			{
#if 0 // DEBUG
				debugf(TEXT("Found %s in AnimSet %s"), *AnimSeqName, AnimSets(i)->GetName());
#endif
				return FoundSeq;
			}
		}
	}

	return NULL;
}

/**
 * Find a named MorphTarget from the MorphSets array in the SkeletalMeshComponent.
 * This searches the array in the same way as FindAnimSequence
 *
 * @param AnimSeqName Name of MorphTarget to look for.
 *
 * @return Pointer to found MorphTarget. Returns NULL if could not find target with that name.
 */
UMorphTarget* USkeletalMeshComponent::FindMorphTarget( FName MorphTargetName )
{
	if(MorphTargetName == NAME_None)
	{
		return NULL;
	}

	// Work from last element in list backwards, so you can replace a specific target by adding a set later in the array.
	for(INT i=MorphSets.Num()-1; i>=0; i--)
	{
		if( MorphSets(i) )
		{
			UMorphTarget* FoundTarget = MorphSets(i)->FindMorphTarget(MorphTargetName);
			if(FoundTarget)
			{
				return FoundTarget;
			}
		}
	}

	return NULL;
}


/**
 *	Set the AnimTree, residing in a package, to use as the template for this SkeletalMeshComponent.
 *	The AnimTree that is passed in is copied and assigned to the Animations pointer in the SkeletalMeshComponent.
 *	NB. This will destroy the currently instanced AnimTree, so it important you don't have any references to it or nodes within it!
 */
void USkeletalMeshComponent::SetAnimTreeTemplate(UAnimTree* NewTemplate)
{
	// If there is a tree instanced at the moment - destroy it.
	DeleteAnimTree();
	checkSlow(Animations == NULL);

	if (NewTemplate != NULL)
	{
		// Copy template and assign to Animations pointer.
		Animations = NewTemplate->CopyAnimTree(this);

		if (Animations != NULL)
		{
			// If successful, initialise the new tree.
			InitAnimTree();

			// Remember the new template
			AnimTreeTemplate = NewTemplate;
		}
		else
		{
			debugf( TEXT("Failed to instance AnimTree Template: %s"), *NewTemplate->GetName() );
			AnimTreeTemplate = NULL;
		}
	}
	else
	{
		AnimTreeTemplate = NULL;
	}
}

/** Update mapping table between. Call whenever you change this or the ParentAnimComponent skeletal mesh. */
void USkeletalMeshComponent::UpdateParentBoneMap()
{
	ParentBoneMap.Empty();

	if(SkeletalMesh && ParentAnimComponent && ParentAnimComponent->SkeletalMesh)
	{
		USkeletalMesh* ParentMesh = ParentAnimComponent->SkeletalMesh;

		ParentBoneMap.Add( SkeletalMesh->RefSkeleton.Num() );
		if (SkeletalMesh == ParentMesh)
		{
			// if the meshes are the same, the indices must match exactly so we don't need to look them up
			for (INT i = 0; i < ParentBoneMap.Num(); i++)
			{
				ParentBoneMap(i) = i;
			}
		}
		else
		{
			for(INT i=0; i<ParentBoneMap.Num(); i++)
			{
				FName BoneName = SkeletalMesh->RefSkeleton(i).Name;
				ParentBoneMap(i) = ParentMesh->MatchRefBone( BoneName );
			}
		}
	}
}

void USkeletalMeshComponent::InitAnimTree()
{
#if SHOW_INITANIMTREE_TIME
	DOUBLE Start = appSeconds();
#endif

	if(Animations)
	{
		// Try a cast to a tree.
		UAnimTree* Tree = Cast<UAnimTree>(Animations);

		// Reset all nodes
		TArray<UAnimNode*> TreeNodes;
		Animations->GetNodes( TreeNodes );

		for(INT i=0; i<TreeNodes.Num(); i++)
		{
			TreeNodes(i)->ParentNodes.Empty();
			TreeNodes(i)->SkelComponent		= NULL;
			TreeNodes(i)->NodeTickTag		= TickTag;
			TreeNodes(i)->NodeTotalWeight	= 0.f;
		}

		// Initialise all nodes in tree
		Animations->InitAnim(this, NULL);

		// Build array in tick order. Start by adding root node and call from there.
		TickTag++;
		AnimTickArray.Reset();
		AnimTickArray.Reserve(TreeNodes.Num());
		AnimTickArray.AddItem(Animations);
		Animations->NodeTickTag = TickTag;
		Animations->BuildTickArray(AnimTickArray);

		// If its an AnimTree, initialise the MorphNodes.
		if(Tree)
		{
			TArray<UMorphNodeBase*> MorphNodes;
			Tree->GetMorphNodes(MorphNodes);

			for(INT i=0; i<MorphNodes.Num(); i++)
			{
				MorphNodes(i)->SkelComponent = NULL;
			}

			Tree->InitTreeMorphNodes(this);
		}
		
		// Initialise the skeletal controls on the tree.
		InitSkelControls();

		// if there's an Owner, notify it that our AnimTree was initialized so it can cache references to controllers and such
		if (Tree != NULL && Owner != NULL)
		{
			Owner->eventPostInitAnimTree(this);
		}
	}

#if SHOW_INITANIMTREE_TIME
	debugf(TEXT("InitAnimTree: %f"), (appSeconds() - Start) * 1000.f);
#endif
}

/** 
 *	Iterate over all SkelControls in the AnimTree (Animations) calling InitSkelControl. 
 *	Also sets up the SkelControlIndex array indicating which SkelControl to apply when we reach certain bones.
 */
void USkeletalMeshComponent::InitSkelControls()
{
	// Initialise the SkelControls and the SkelControlIndex array.
	SkelControlIndex.Reset();

	UAnimTree* Tree = Cast<UAnimTree>(Animations);
	if(SkeletalMesh && Tree && Tree->SkelControlLists.Num() > 0)
	{
		INT NumBones = SkeletalMesh->RefSkeleton.Num();

		// Allocate SkelControlIndex array and initialize all elements to '255' - which indicates 'no control'.
		SkelControlIndex.Add(NumBones);
		appMemset( &SkelControlIndex(0), 0xFF, sizeof(BYTE) * NumBones );

		INT NumControls = Tree->SkelControlLists.Num();
		check(NumControls < 255);

		// For each list, store index of head struct at the bone where it should be applied.
		for(INT ControlIndex = 0; ControlIndex < NumControls; ControlIndex++)
		{
				INT BoneIndex = SkeletalMesh->MatchRefBone(Tree->SkelControlLists(ControlIndex).BoneName);
				if(BoneIndex != INDEX_NONE)
				{
					if(SkelControlIndex(BoneIndex) != 255)
					{
						debugf( TEXT("SkelControl: Trying To Control Bone Which Already Has Control List.") );
					}
					else
					{
						// Save index of SkelControl.
						SkelControlIndex(BoneIndex) = ControlIndex;
					}
				}
		}
	}
}

/** 
 *	Utility for find a USkelControl by name from the AnimTree currently being used by this SkelelalMeshComponent.
 *	Do not hold on to pointer- will become invalid when tree changes.
 */
USkelControlBase* USkeletalMeshComponent::FindSkelControl(FName InControlName)
{
	UAnimTree* AnimTree = Cast<UAnimTree>(Animations);
	if(AnimTree)
	{
		return AnimTree->FindSkelControl(InControlName);
	}

	return NULL;
}


/** 
 * Find an Animation Node in the Animation Tree whose NodeName matches InNodeName. 
 * Warning: The search is O(n^2), so for large AnimTrees, cache result.
 */
UAnimNode* USkeletalMeshComponent::FindAnimNode(FName InNodeName)
{
	if( Animations )
	{
		return Animations->FindAnimNode(InNodeName);
	}

	return NULL;
}


/** 
 *	Utility for find a UMorphNode by name from the AnimTree currently being used by this SkelelalMeshComponent.
 *	Do not hold on to pointer- will become invalid when tree changes.
 */
UMorphNodeBase* USkeletalMeshComponent::FindMorphNode(FName InNodeName)
{
	UAnimTree* AnimTree = Cast<UAnimTree>(Animations);
	if(AnimTree)
	{
		return AnimTree->FindMorphNode(InNodeName);
	}

	return NULL;
}


/** 
 *	Find the current world space location and rotation of a named socket on the skeletal mesh component.
 *	If the socket is not found, then it returns false and does not change the OutLocation/OutRotation variables.
 *	@param InSocketName the name of the socket to find
 *	@param OutLocation (out) set to the world space location of the socket
 *	@param OutRotation (out) if not NULL, the rotator pointed to is set to the world space rotation of the socket
 *	@return whether or not the socket was found
 */
UBOOL USkeletalMeshComponent::GetSocketWorldLocationAndRotation(FName InSocketName, FVector& OutLocation, FRotator* OutRotation)
{
	if( SkeletalMesh )
	{
		USkeletalMeshSocket* Socket = SkeletalMesh->FindSocket( InSocketName );
		if( Socket )
		{
			INT BoneIndex = MatchRefBone(Socket->BoneName);
			if( BoneIndex != INDEX_NONE )
			{
				FMatrix BoneMatrix = GetBoneMatrix(BoneIndex);
				FRotationTranslationMatrix SocketMatrix(Socket->RelativeRotation,Socket->RelativeLocation);
				FMatrix WorldSocketMatrix = SocketMatrix * BoneMatrix;

				OutLocation = WorldSocketMatrix.GetOrigin();
				if (OutRotation != NULL)
				{
					*OutRotation = WorldSocketMatrix.Rotator();
				}

				return true;
			}
			else
			{
#if !PS3 // hopefully will be found on PC
				debugf( TEXT("GetSocketWorldLocationAndRotation : Could not find bone '%s'"), *Socket->BoneName.ToString() );
#endif
			}
		}
		else
		{
#if !PS3 // hopefully will be found on PC
			debugf( TEXT("GetSocketWorldLocationAndRotation : Could not find socket '%s' in '%s'"), *InSocketName.ToString(), SkeletalMesh?*SkeletalMesh->GetName():TEXT("None") );
#endif
		}
	}
	else
	{
#if !PS3 // hopefully will be found on PC
		debugf( TEXT("GetSocketWorldLocationAndRotation : Could not find SkeletalMesh (SkeletalMesh == NULL :-( )") );
#endif
	}

	return false;
}


///////////////////////////////////////////////////////
// Script function implementations


/** @see USkeletalMeshComponent::FindAnimSequence */
void USkeletalMeshComponent::execFindAnimSequence( FFrame& Stack, RESULT_DECL )
{
	P_GET_NAME(AnimSeqName);
	P_FINISH;

	*(UAnimSequence**)Result = FindAnimSequence( AnimSeqName );
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent,INDEX_NONE,execFindAnimSequence);

/** @see USkeletalMeshComponent::FindMorphTarget */
void USkeletalMeshComponent::execFindMorphTarget( FFrame& Stack, RESULT_DECL )
{
	P_GET_NAME(MorphTargetName);
	P_FINISH;

	*(UMorphTarget**)Result = FindMorphTarget( MorphTargetName );
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent,INDEX_NONE,execFindMorphTarget);

void USkeletalMeshComponent::execAttachComponent(FFrame& Stack,RESULT_DECL)
{
	P_GET_OBJECT(UActorComponent,Component);
	P_GET_NAME(BoneName);
	P_GET_VECTOR_OPTX(RelativeLocation,FVector(0,0,0));
	P_GET_ROTATOR_OPTX(RelativeRotation,FRotator(0,0,0));
	P_GET_VECTOR_OPTX(RelativeScale,FVector(1,1,1));
	P_FINISH;

	if (Component == NULL)
	{
		debugf(NAME_Warning,TEXT("Attempting to attach NULL component to %s"),*GetName());
	}
	else
	{
		AttachComponent(Component,BoneName,RelativeLocation,RelativeRotation,RelativeScale);
	}
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent,INDEX_NONE,execAttachComponent);

void USkeletalMeshComponent::execDetachComponent(FFrame& Stack,RESULT_DECL)
{
	P_GET_OBJECT(UActorComponent,Component);
	P_FINISH;

	if (Component == NULL)
	{
		debugf(NAME_Warning,TEXT("Attempting to detach NULL component from %s"),*GetName());
	}
	else
	{
		DetachComponent(Component);
	}
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent,INDEX_NONE,execDetachComponent);

void USkeletalMeshComponent::execAttachComponentToSocket(FFrame& Stack, RESULT_DECL)
{
	P_GET_OBJECT(UActorComponent,Component);
	P_GET_NAME(SocketName);
	P_FINISH;

	if (Component == NULL)
	{
		debugf(NAME_Warning,TEXT("Attempting to attach NULL component to %s, socket %s"),*GetName(),*SocketName.ToString());
	}
	else
	{
		AttachComponentToSocket(Component, SocketName);
	}
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent,INDEX_NONE,execAttachComponentToSocket);

void USkeletalMeshComponent::execGetSocketWorldLocationAndRotation(FFrame& Stack, RESULT_DECL)
{
	P_GET_NAME(InSocketName);
	P_GET_VECTOR_REF(OutLocation);
	P_GET_ROTATOR_REF(OutRotation); // optional
	P_FINISH;

	*(UBOOL*)Result = GetSocketWorldLocationAndRotation(InSocketName, OutLocation, pOutRotation);
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent,INDEX_NONE,execGetSocketWorldLocationAndRotation);

void USkeletalMeshComponent::execGetSocketByName(FFrame& Stack, RESULT_DECL)
{
	P_GET_NAME(InSocketName);
	P_FINISH;

	USkeletalMeshSocket* Socket = NULL;

	if( SkeletalMesh )
	{
		Socket = SkeletalMesh->FindSocket( InSocketName );
	}
	else
	{
		debugf(NAME_Warning,TEXT("Socket not found in %s, with name %s"), *GetName(), *InSocketName.ToString());
	}

	*(USkeletalMeshSocket**)Result = Socket;
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent,INDEX_NONE,execGetSocketByName);


void USkeletalMeshComponent::execFindComponentAttachedToBone(FFrame& Stack, RESULT_DECL)
{
	P_GET_NAME(InBoneName);
	P_FINISH;

	UActorComponent*	FoundComponent = NULL;

	if( InBoneName != NAME_None )
	{
		for(INT idx=0; idx<Attachments.Num(); idx++)
		{
			if( Attachments(idx).BoneName == InBoneName )
			{
				FoundComponent = Attachments(idx).Component;
				break;
			}
		}
	}
	*(UActorComponent**)Result = FoundComponent;
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent,INDEX_NONE,execFindComponentAttachedToBone);

void USkeletalMeshComponent::execIsComponentAttached(FFrame& Stack, RESULT_DECL)
{
	P_GET_OBJECT(UActorComponent,Component)
	P_GET_NAME_OPTX(BoneName,NAME_None)
	P_FINISH;

	UBOOL	bFound = false;

	for(INT idx=0; idx<Attachments.Num(); idx++)
	{
		if( Attachments(idx).Component == Component &&
			(BoneName == NAME_None || Attachments(idx).BoneName == BoneName) )
		{
			bFound = true;
			break;
		}
	}

	*(UBOOL*)Result = bFound;
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent,INDEX_NONE,execIsComponentAttached);

void USkeletalMeshComponent::execAttachedComponents(FFrame& Stack, RESULT_DECL)
{
	P_GET_OBJECT(UClass,BaseClass);
	P_GET_OBJECT_REF(UActorComponent, OutComponent);
	P_FINISH;

	if (BaseClass == NULL)
	{
		debugf(NAME_Error, TEXT("(%s:%04X) AttachedComponents() called with no class"), *Stack.Node->GetFullName(), Stack.Code - &Stack.Node->Script(0));
		// skip the iterator
		INT wEndOffset = Stack.ReadWord();
		Stack.Code = &Stack.Node->Script(wEndOffset + 1);
	}
	else
	{
		INT Index = 0;
		PRE_ITERATOR;
			// Fetch next component in the iteration.
			OutComponent = NULL;
			while (Index < Attachments.Num() && OutComponent == NULL)
			{
				UActorComponent* TestComponent = Attachments(Index).Component;
				Index++;
				if (TestComponent != NULL && !TestComponent->IsPendingKill() && TestComponent->IsA(BaseClass))
				{
					OutComponent = TestComponent;
				}
			}
			if (OutComponent == NULL)
			{
				Stack.Code = &Stack.Node->Script(wEndOffset + 1);
				break;
			}
		POST_ITERATOR;
	}
}

void USkeletalMeshComponent::execSetSkeletalMesh( FFrame& Stack, RESULT_DECL )
{
	P_GET_OBJECT(USkeletalMesh, NewMesh);
	P_GET_UBOOL_OPTX(bKeepSpaceBases, FALSE);
	P_FINISH;	

	SetSkeletalMesh( NewMesh, bKeepSpaceBases ); 
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent,INDEX_NONE,execSetSkeletalMesh);


void USkeletalMeshComponent::execSetPhysicsAsset( FFrame& Stack, RESULT_DECL )
{
	P_GET_OBJECT(UPhysicsAsset, NewPhysicsAsset);
	P_GET_UBOOL_OPTX(bForceReInit, FALSE);
	P_FINISH;	

	SetPhysicsAsset(NewPhysicsAsset, bForceReInit); 
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent,INDEX_NONE,execSetPhysicsAsset);

//
//	USkeletalMeshComponent::execSetParentAnimComponent
//

void USkeletalMeshComponent::execSetParentAnimComponent( FFrame& Stack, RESULT_DECL )
{
	P_GET_OBJECT(USkeletalMeshComponent, NewParentAnimComp);
	P_FINISH;

	SetParentAnimComponent( NewParentAnimComp );
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent,INDEX_NONE,execSetParentAnimComponent);

//
//	USkeletalMeshComponent::execGetBoneQuaternion
//

void USkeletalMeshComponent::execGetBoneQuaternion( FFrame& Stack, RESULT_DECL )
{
	P_GET_NAME(BoneName);
	P_GET_INT_OPTX(Space,0);
	P_FINISH;

	*(FQuat*)Result = GetBoneQuaternion(BoneName, Space);
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent,INDEX_NONE,execGetBoneQuaternion);

FQuat USkeletalMeshComponent::GetBoneQuaternion(FName BoneName, INT Space)
{
	INT BoneIndex = MatchRefBone(BoneName);

	if( BoneIndex == INDEX_NONE )
	{
		debugf(NAME_Warning, TEXT("USkeletalMeshComponent::execGetBoneQuaternion : Could not find bone: %s"), *BoneName.ToString());
		return FQuat::Identity;
	}

	// If local space...
	FMatrix BoneMatrix = (Space == 1) ? SpaceBases(BoneIndex) : GetBoneMatrix(BoneIndex);
	BoneMatrix.RemoveScaling();

	return FQuat(BoneMatrix);
}

//
//	USkeletalMeshComponent::execGetBoneLocation
//

void USkeletalMeshComponent::execGetBoneLocation( FFrame& Stack, RESULT_DECL )
{
	P_GET_NAME(BoneName);
	P_GET_INT_OPTX(Space,0);
	P_FINISH;

	*(FVector*)Result = GetBoneLocation(BoneName,Space);
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent,INDEX_NONE,execGetBoneLocation);

FVector USkeletalMeshComponent::GetBoneLocation( FName BoneName, INT Space )
{
	INT BoneIndex = MatchRefBone(BoneName);
	if( BoneIndex == INDEX_NONE )
	{
		debugf( TEXT("USkeletalMeshComponent::GetBoneLocation : Could not find bone: %s"), *BoneName.ToString() );
		return FVector(0,0,0);
	}

	// If space == Local
	if( Space == 1 )
	{
		return SpaceBases(BoneIndex).GetOrigin();
	}
	else
	{
		return GetBoneMatrix(BoneIndex).GetOrigin();
	}
}

void USkeletalMeshComponent::execGetBoneAxis(FFrame& Stack, RESULT_DECL)
{
	P_GET_NAME(BoneName);
	P_GET_BYTE(Axis);
	P_FINISH;

	*(FVector*)Result = GetBoneAxis(BoneName,Axis);
}

FVector USkeletalMeshComponent::GetBoneAxis( FName BoneName, BYTE Axis )
{
	INT BoneIndex = MatchRefBone(BoneName);
	if (BoneIndex == INDEX_NONE)
	{
		debugf(NAME_Warning, TEXT("USkeletalMeshComponent::execGetBoneAxis : Could not find bone: %s"), *BoneName.ToString());
		return FVector(0.f, 0.f, 0.f);
	}
	else if (Axis == AXIS_None || Axis == 3 || Axis > 4)
	{
		debugf(NAME_Warning, TEXT("USkeletalMeshComponent::execGetBoneAxis: Invalid axis specified"));
		return FVector(0.f, 0.f, 0.f);
	}
	else
	{
		INT MatrixAxis;
		if (Axis == AXIS_X)
		{
			MatrixAxis = 0;
		}
		else if (Axis == AXIS_Y)
		{
			MatrixAxis = 1;
		}
		else
		{
			MatrixAxis = 2;
		}
		return GetBoneMatrix(BoneIndex).GetAxis(MatrixAxis).SafeNormal();
	}
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent, INDEX_NONE, execGetBoneAxis);

/** 
 *	Transform a location/rotation from world space to bone relative space.
 *	This is handy if you know the location in world space for a bone attachment, as AttachComponent takes location/rotation in bone-relative space.
 */
void USkeletalMeshComponent::execTransformToBoneSpace(FFrame& Stack, RESULT_DECL)
{
	P_GET_NAME(BoneName);
	P_GET_VECTOR(InPosition);
	P_GET_ROTATOR(InRotation);
	P_GET_VECTOR_REF(OutPosition);
	P_GET_ROTATOR_REF(OutRotation);
	P_FINISH;

	INT BoneIndex = MatchRefBone(BoneName);
	if(BoneIndex != INDEX_NONE)
	{
		FMatrix BoneToWorldTM = GetBoneMatrix(BoneIndex);

		FMatrix WorldTM = FRotationTranslationMatrix(InRotation, InPosition);
		FMatrix LocalTM = WorldTM * BoneToWorldTM.Inverse();

		OutPosition = LocalTM.GetOrigin();
		OutRotation = LocalTM.Rotator();
	}
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent, INDEX_NONE, execTransformToBoneSpace);

/**
 *	Transfor a location/rotation in bone relative space to world space.
 */
void USkeletalMeshComponent::execTransformFromBoneSpace(FFrame& Stack, RESULT_DECL)
{
	P_GET_NAME(BoneName);
	P_GET_VECTOR(InPosition);
	P_GET_ROTATOR(InRotation);
	P_GET_VECTOR_REF(OutPosition);
	P_GET_ROTATOR_REF(OutRotation);
	P_FINISH;

	INT BoneIndex = MatchRefBone(BoneName);
	if(BoneIndex != INDEX_NONE)
	{
		FMatrix BoneToWorldTM = GetBoneMatrix(BoneIndex);

		FMatrix LocalTM = FRotationTranslationMatrix(InRotation, InPosition);
		FMatrix WorldTM = LocalTM * BoneToWorldTM;

		OutPosition = WorldTM.GetOrigin();
		OutRotation = WorldTM.Rotator();
	}
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent, INDEX_NONE, execTransformFromBoneSpace);

/** finds the closest bone to the given location
 * @param TestLocation the location to test against
 * @param BoneLocation (optional, out) if specified, set to the world space location of the bone that was found, or (0,0,0) if no bone was found
 * @param IgnoreScale (optional) if specified, only bones with scaling larger than the specified factor are considered
 * @return the name of the bone that was found, or 'None' if no bone was found
 */
FName USkeletalMeshComponent::FindClosestBone(FVector TestLocation, FVector* BoneLocation, FLOAT IgnoreScale)
{
	if (SkeletalMesh == NULL)
	{
		if (BoneLocation != NULL)
		{
			*BoneLocation = FVector(0.f, 0.f, 0.f);
		}
		return NAME_None;
	}
	else
	{
		// transform the TestLocation into mesh local space so we don't have to transform the (mesh local) bone locations
		TestLocation = LocalToWorld.Inverse().TransformFVector(TestLocation);
		
		FLOAT IgnoreScaleSquared = Square(IgnoreScale);
		FLOAT BestDistSquared = BIG_NUMBER;
		INT BestIndex = -1;
		for (INT i = 0; i < SpaceBases.Num(); i++)
		{
			if (IgnoreScale < 0.f || SpaceBases(i).GetAxis(0).SizeSquared() > IgnoreScaleSquared)
			{
				FLOAT DistSquared = (TestLocation - SpaceBases(i).GetOrigin()).SizeSquared();
				if (DistSquared < BestDistSquared)
				{
					BestIndex = i;
					BestDistSquared = DistSquared;
				}
			}
		}

		if (BestIndex == -1)
		{
			if (BoneLocation != NULL)
			{
				*BoneLocation = FVector(0.f, 0.f, 0.f);
			}
			return NAME_None;
		}
		else
		{
			// transform the bone location into world space
			if (BoneLocation != NULL)
			{
				*BoneLocation = (SpaceBases(BestIndex) * LocalToWorld).GetOrigin();
			}
			return SkeletalMesh->RefSkeleton(BestIndex).Name;
		}
	}
}

void USkeletalMeshComponent::execFindClosestBone(FFrame& Stack, RESULT_DECL)
{
	P_GET_VECTOR(TestLocation);
	P_GET_VECTOR_REF(BoneLocation); // optional
	P_GET_FLOAT_OPTX(IgnoreScale, -1.0f);
	P_FINISH;

	*(FName*)Result = FindClosestBone(TestLocation, pBoneLocation, IgnoreScale);
}

/** Script-exposed SetAnimTreeTemplate function. */
void USkeletalMeshComponent::execSetAnimTreeTemplate( FFrame& Stack, RESULT_DECL )
{
	P_GET_OBJECT(UAnimTree, NewTemplate);
	P_FINISH;

	SetAnimTreeTemplate(NewTemplate);
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent,INDEX_NONE,execSetAnimTreeTemplate);

/** Script-exposed UpdateParentBoneMap function. */
void USkeletalMeshComponent::execUpdateParentBoneMap( FFrame& Stack, RESULT_DECL )
{
	P_FINISH;

	UpdateParentBoneMap();
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent,INDEX_NONE,execUpdateParentBoneMap);

/** Script-exposed InitSkelControls function. */
void USkeletalMeshComponent::execInitSkelControls( FFrame& Stack, RESULT_DECL )
{
	P_FINISH;

	InitSkelControls();
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent,INDEX_NONE,execInitSkelControls);


void USkeletalMeshComponent::execFindSkelControl( FFrame& Stack, RESULT_DECL )
{
	P_GET_NAME(InControlName);
	P_FINISH;

	*(USkelControlBase**)Result = FindSkelControl(InControlName);
}

void USkeletalMeshComponent::execFindAnimNode( FFrame& Stack, RESULT_DECL )
{
	P_GET_NAME(InNodeName);
	P_FINISH;

	*(UAnimNode**)Result = FindAnimNode(InNodeName);
}

/** returns all AnimNodes in the animation tree that are the specfied class or a subclass
 * @param BaseClass base class to return
 * @param Node (out) the returned AnimNode for each iteration
 */
void USkeletalMeshComponent::execAllAnimNodes(FFrame& Stack, RESULT_DECL)
{
	P_GET_OBJECT(UClass, BaseClass);
	P_GET_OBJECT_REF(UAnimNode, Node);
	P_FINISH;

	if (Animations == NULL)
	{
		debugf(NAME_ScriptWarning, TEXT("(%s:%04X) AllAnimNodes() called with no Animations"), *Stack.Node->GetFullName(), Stack.Code - &Stack.Node->Script(0));
		// skip the iterator
		INT wEndOffset = Stack.ReadWord();
		Stack.Code = &Stack.Node->Script(wEndOffset + 1);
	}
	else
	{
		TArray<UAnimNode*> AllNodes;
		// if we have a valid subclass of AnimNode
		if (BaseClass != NULL && BaseClass != UAnimNode::StaticClass())
		{
			// get only nodes of that class
			Animations->GetNodesByClass(AllNodes, BaseClass);
		}
		else
		{
			// otherwise get all of them
			Animations->GetNodes(AllNodes);
		}
		INT CurrentIndex = 0;
		PRE_ITERATOR;
			// get the next node in the iteration
			if (CurrentIndex < AllNodes.Num())
			{
				Node = AllNodes(CurrentIndex++);
			}
			else
			{
				// we're out of nodes
				Node = NULL;
				Stack.Code = &Stack.Node->Script(wEndOffset + 1);
				break;
			}
		POST_ITERATOR;
	}
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent, INDEX_NONE, execAllAnimNodes);

void USkeletalMeshComponent::execFindMorphNode( FFrame& Stack, RESULT_DECL )
{
	P_GET_NAME(InNodeName);
	P_FINISH;

	*(UMorphNodeBase**)Result = FindMorphNode(InNodeName);
}

/** 
 *	Utility function for calculating transform from the component reference frame to the desired reference frame specified by the enum. 
 *	Scaling is removed from matrix before being returned.
 */
FMatrix USkeletalMeshComponent::CalcComponentToFrameMatrix(INT BoneIndex, BYTE Space, FName OtherBoneName)
{
	FMatrix ComponentToFrame;
	if(Space == BCS_WorldSpace)
	{
		ComponentToFrame = LocalToWorld;
	}
	else if(Space == BCS_ActorSpace)
	{
		if(Owner)
		{
			ComponentToFrame = LocalToWorld * Owner->LocalToWorld().Inverse();
		}
		else
		{
			//ActorToWorld = FMatrix::Identity;
			ComponentToFrame = LocalToWorld;
		}
	}
	else if(Space == BCS_ComponentSpace)
	{
		ComponentToFrame = FMatrix::Identity;
	}
	else if(Space == BCS_ParentBoneSpace)
	{
		if(BoneIndex == 0)
		{
			ComponentToFrame = FMatrix::Identity;
		}
		else
		{
			const INT ParentIndex = SkeletalMesh->RefSkeleton(BoneIndex).ParentIndex;
			ComponentToFrame =SpaceBases(ParentIndex).Inverse();
		}
	}
	else if(Space == BCS_BoneSpace)
	{
		ComponentToFrame = SpaceBases(BoneIndex).Inverse();
	}
	else if(Space == BCS_OtherBoneSpace)
	{
		const INT OtherBoneIndex = MatchRefBone(OtherBoneName);
		if(OtherBoneIndex != INDEX_NONE)
		{
			ComponentToFrame = SpaceBases(OtherBoneIndex).Inverse();
		}
		else
		{
			debugf( TEXT("GetFrameMatrix: Invalid BoneName: %s  for Mesh: %s"), *OtherBoneName.ToString(), *SkeletalMesh->GetFName().ToString() );
			ComponentToFrame = FMatrix::Identity;
		}
	}
	else
	{
		debugf( TEXT("GetFrameMatrix: Unknown Frame %d  for Mesh: %s"), Space, *SkeletalMesh->GetFName().ToString() );
		ComponentToFrame = FMatrix::Identity;
	}

	ComponentToFrame.RemoveScaling();
	return ComponentToFrame;
}

/** Script-exposed execFindConstraintBoneName function. */
void USkeletalMeshComponent::execFindConstraintIndex( FFrame& Stack, RESULT_DECL )
{
	P_GET_NAME(constraintName);
	P_FINISH;

	*(INT*)Result = PhysicsAsset ? PhysicsAsset->FindConstraintIndex(constraintName) : -1;
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent,INDEX_NONE,execFindConstraintIndex);

/** Script-exposed execFindConstraintBoneName function. */
void USkeletalMeshComponent::execFindConstraintBoneName( FFrame& Stack, RESULT_DECL )
{
	P_GET_INT(ConstraintIndex);
	P_FINISH;

	*(FName*)Result = PhysicsAsset ? PhysicsAsset->FindConstraintBoneName(ConstraintIndex) : NAME_None;
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent,INDEX_NONE,execFindConstraintBoneName);


/** Script-exposed execFindBodyInstanceNamed function. */
void USkeletalMeshComponent::execFindBodyInstanceNamed( FFrame& Stack, RESULT_DECL )
{
	P_GET_NAME(BoneName);
	P_FINISH;

	*(URB_BodyInstance**)Result = FindBodyInstanceNamed(BoneName);
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent, INDEX_NONE, execFindBodyInstanceNamed);


/** Find a BodyInstance by BoneName */
URB_BodyInstance* USkeletalMeshComponent::FindBodyInstanceNamed(FName BoneName)
{
	if( !PhysicsAsset || !PhysicsAssetInstance )
	{
		return NULL;
	}

	// Find the specified body instance
	for(INT i=0; i<PhysicsAsset->BodySetup.Num(); i++)
	{
		URB_BodyInstance*	BodyInst	= PhysicsAssetInstance->Bodies(i);
		URB_BodySetup*		BodySetup	= PhysicsAsset->BodySetup(i);

		if( BodySetup->BoneName == BoneName )
		{
			return BodyInst;
		}
	}

	return NULL;
}



void USkeletalMeshComponent::ForceSkelUpdate()
{
	// easiest way to make sure everything works is to temporarily pretend we've been recently rendered
	FLOAT OldRenderTime = LastRenderTime;
	LastRenderTime = GWorld->GetWorldInfo()->TimeSeconds;

	UpdateSkelPose();
	ConditionalUpdateTransform(); 

	LastRenderTime = OldRenderTime;
}

void USkeletalMeshComponent::execForceSkelUpdate(FFrame& Stack, RESULT_DECL)
{
	P_FINISH;
	
	ForceSkelUpdate();
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent,INDEX_NONE,execForceSkelUpdate);


/** 
 * Force AnimTree to recache all animations.
 * Call this when the AnimSets array has been changed.
 */
void USkeletalMeshComponent::UpdateAnimations()
{
	if( Animations )
	{
		// Force all nodes in the AnimTree to re-look up their animations.
		TickTag++;
		Animations->AnimSetsUpdated();
	}
	else
	{
		debugf(TEXT("UpdateAnimations, Animations == NULL"));
	}
}


void USkeletalMeshComponent::execUpdateAnimations(FFrame& Stack, RESULT_DECL)
{
	P_FINISH;

	UpdateAnimations();
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent,INDEX_NONE,execUpdateAnimations);


void USkeletalMeshComponent::execPlayFaceFXAnim( FFrame& Stack, RESULT_DECL )
{
	P_GET_OBJECT_REF(UFaceFXAnimSet, FaceFXAnimSetRef);
	P_GET_STR(AnimName);
	P_GET_STR(GroupName);
	P_FINISH;

	*(UBOOL*)Result = PlayFaceFXAnim(FaceFXAnimSetRef, AnimName, GroupName);
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent, INDEX_NONE, execPlayFaceFXAnim);

void USkeletalMeshComponent::execStopFaceFXAnim( FFrame& Stack, RESULT_DECL )
{
	P_FINISH;

	StopFaceFXAnim();
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent, INDEX_NONE, execStopFaceFXAnim);


void USkeletalMeshComponent::execIsPlayingFaceFXAnim( FFrame& Stack, RESULT_DECL )
{
	P_FINISH;

	*(UBOOL*)Result = IsPlayingFaceFXAnim();
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent, INDEX_NONE, execIsPlayingFaceFXAnim);


void USkeletalMeshComponent::execDeclareFaceFXRegister( FFrame& Stack, RESULT_DECL )
{
	P_GET_STR(RegName);
	P_FINISH;

	DeclareFaceFXRegister(RegName);
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent, INDEX_NONE, execDeclareFaceFXRegister);

void USkeletalMeshComponent::execGetFaceFXRegister( FFrame& Stack, RESULT_DECL )
{
	P_GET_STR(RegName);
	P_FINISH;

	*(FLOAT*)Result = GetFaceFXRegister(RegName);
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent, INDEX_NONE, execGetFaceFXRegister);

void USkeletalMeshComponent::execSetFaceFXRegister( FFrame& Stack, RESULT_DECL )
{
	P_GET_STR(RegName);
	P_GET_FLOAT(RegVal);
	P_GET_BYTE(RegOp);
	P_GET_FLOAT_OPTX(InterpDuration,0.0f);
	P_FINISH;

	SetFaceFXRegister(RegName, RegVal, RegOp, InterpDuration);
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent, INDEX_NONE, execSetFaceFXRegister);

void USkeletalMeshComponent::execSetFaceFXRegisterEx( FFrame& Stack, RESULT_DECL )
{
	P_GET_STR(RegName);
	P_GET_BYTE(RegOp);
	P_GET_FLOAT(FirstValue);
	P_GET_FLOAT(FirstInterpDuration);
	P_GET_FLOAT(NextValue);
	P_GET_FLOAT(NextInterpDuration);
	P_FINISH;

	SetFaceFXRegisterEx(RegName, RegOp, FirstValue, FirstInterpDuration, NextValue, NextInterpDuration);
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent, INDEX_NONE, execSetFaceFXRegisterEx);

void USkeletalMeshComponent::execGetBonesWithinRadius(FFrame& Stack, RESULT_DECL)
{
	P_GET_VECTOR(Origin);
	P_GET_FLOAT(Radius);
	P_GET_INT(TraceFlags);
	P_GET_TARRAY_REF(FName,out_Bones);
	P_FINISH;

	*(UBOOL*)Result = GetBonesWithinRadius( Origin, Radius, TraceFlags, out_Bones );
}
IMPLEMENT_FUNCTION(USkeletalMeshComponent,INDEX_NONE,execGetBonesWithinRadius);


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Decals on skeletal meshes.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Transforms the specified decal info into reference pose space.
 *
 * @param	Decal			Info of decal to transform.
 * @param	BoneIndex		The index of the bone hit by the decal.
 * @return					A reference to the transformed decal info, or NULL if the operation failed.
 */
FDecalState* USkeletalMeshComponent::TransformDecalToRefPoseSpace(FDecalState* Decal, INT BoneIndex) const
{
	FDecalState* NewDecalState = NULL;

	FMatrix ComponentSpaceToWorldTM = GetBoneMatrix( BoneIndex );
	ComponentSpaceToWorldTM.RemoveScaling();
	const FMatrix RefToWorld = SkeletalMesh->RefBasesInvMatrix(BoneIndex) * ComponentSpaceToWorldTM;
	const FMatrix WorldToRef = RefToWorld.Inverse();

	NewDecalState = Decal;

	NewDecalState->HitLocation = WorldToRef.TransformFVector4( Decal->HitLocation );
	NewDecalState->HitTangent = WorldToRef.TransformNormal( Decal->HitTangent );
	NewDecalState->HitBinormal = WorldToRef.TransformNormal( Decal->HitBinormal );
	NewDecalState->HitNormal = WorldToRef.TransformNormal( Decal->HitNormal );
	NewDecalState->WorldTexCoordMtx = FMatrix( /*TileX**/NewDecalState->HitTangent/Decal->Width,
									/*TileY**/NewDecalState->HitBinormal/Decal->Height,
									NewDecalState->HitNormal,
									FVector(0.f,0.f,0.f) ).Transpose();

	return NewDecalState;
}

namespace{
class FIndexRemap
{
public:
	INT NewIndex;
	UBOOL bIsRigid;
	FIndexRemap(INT InNewIndex,UBOOL bInIsRigid)
		: NewIndex( InNewIndex )
		, bIsRigid( bInIsRigid )
	{}
};
} // namespace

FDecalRenderData* USkeletalMeshComponent::GenerateDecalRenderData(FDecalState* Decal) const
{
	SCOPE_CYCLE_COUNTER(STAT_DecalSkeletalMeshAttachTime);

	// Do nothing if the specified decal doesn't project on skeletal meshes.
	if ( !Decal->bProjectOnSkeletalMeshes )
	{
		return NULL;
	}

	// Only decals spawned at runtime can affect skeletal meshes.
	if( Decal->DecalComponent->bStaticDecal )
	{
		return FALSE;
	}

	// Can't do anything without a SkeletalMesh
	if( !SkeletalMesh )
	{
		return NULL;
	}

	FDecalRenderData* DecalRenderData = NULL;

	// Find the named bone.
	Decal->HitBoneIndex = MatchRefBone( Decal->HitBone );
	if ( Decal->HitBoneIndex != INDEX_NONE )
	{
		// Transform the decal to reference pose space.
		TransformDecalToRefPoseSpace( Decal, Decal->HitBoneIndex );

		DecalRenderData = new FDecalRenderData( NULL, FALSE, FALSE );
		DecalRenderData->NumTriangles = DecalRenderData->GetNumIndices()/3;
	}

	return DecalRenderData;
}
