/*=============================================================================
	UnAnimTree.cpp: Blend tree implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/ 

#include "EnginePrivate.h"
#include "EngineAnimClasses.h"

IMPLEMENT_CLASS(UAnimNode);

IMPLEMENT_CLASS(UAnimNodeBlendBase);
IMPLEMENT_CLASS(UAnimNodeBlend);
IMPLEMENT_CLASS(UAnimNodeCrossfader);
IMPLEMENT_CLASS(UAnimNodeBlendPerBone);
IMPLEMENT_CLASS(UAnimNodeBlendList);
IMPLEMENT_CLASS(UAnimNodeRandom);
IMPLEMENT_CLASS(UAnimNodeBlendDirectional);
IMPLEMENT_CLASS(UAnimNodeBlendByPosture);
IMPLEMENT_CLASS(UAnimNodeBlendByPhysics);
IMPLEMENT_CLASS(UAnimNodeBlendBySpeed);
IMPLEMENT_CLASS(UAnimNodeBlendByBase);
IMPLEMENT_CLASS(UAnimNodeMirror);
IMPLEMENT_CLASS(UAnimNodeBlendMultiBone);
IMPLEMENT_CLASS(UAnimNodeAimOffset);
IMPLEMENT_CLASS(UAnimNodeBlendByAim);
IMPLEMENT_CLASS(UAnimNodeSynch);
IMPLEMENT_CLASS(UAnimNodeScalePlayRate);
IMPLEMENT_CLASS(UAnimNodeScaleRateBySpeed);
IMPLEMENT_CLASS(UAnimNodePlayCustomAnim);
IMPLEMENT_CLASS(UAnimNodeSlot);
IMPLEMENT_CLASS(UAnimTree);

UBOOL UAnimNode::bNodeSearching = FALSE;
INT UAnimNode::CurrentSearchTag = 0;

static const FLOAT	AnimDebugHozSpace = 40.0f;
static const FLOAT  AnimDebugVertSpace = 20.0f;

static const FColor FullWeightColor(255,255,255,255);
static const FColor ZeroWeightColor(0,0,255,255);
static const FColor InactiveColor(128,128,128,255);

/** Anim stats */
DECLARE_STATS_GROUP(TEXT("Anim"),STATGROUP_Anim);

/****
SeklCompTick

   Anim Tick

   UpdatSkelPose
      GetBoneAtoms
          Anim Decompression
          Blend (the remainder)  (aim offsets)
      ComposeSkeleton  (local to component space)  
          Skel Control

   Update RBBones

   -->  Physics Engine here <--

   Update Transform (for anything that is dirty (e.g. it was moved above))
       BlendinPhysics 
       MeshObject Update  (vertex math and copying!)  (send all bones to render thread)
****/

DECLARE_CYCLE_STAT(TEXT("  Update SkelMesh Bounds"),STAT_UpdateSkelMeshBounds,STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("  MeshObject Update"),STAT_MeshObjectUpdate,STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("  BlendInPhysics"),STAT_BlendInPhysics,STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("SkelComp UpdateTransform"),STAT_SkelCompUpdateTransform,STATGROUP_Anim);
//                         -->  Physics Engine here <--
DECLARE_CYCLE_STAT(TEXT("  Update RBBones"),STAT_UpdateRBBones,STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("      SkelControl"),STAT_SkelControl,STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("    Compose Skeleton"),STAT_SkelComposeTime,STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("    UpdateFaceFX"),STAT_UpdateFaceFX,STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("      Anim Decompression"),STAT_GetAnimationPose,STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("      Mirror BoneAtoms"),STAT_MirrorBoneAtoms,STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("    GetBoneAtoms"),STAT_AnimBlendTime,STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("  UpdateSkelPose"),STAT_UpdateSkelPose,STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("    Tick SkelControls"),STAT_SkelControlTickTime,STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("    Sync Groups"),STAT_AnimSyncGroupTime,STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("  Anim Tick Time"),STAT_AnimTickTime,STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("SkelComp Tick Time"),STAT_SkelComponentTickTime,STATGROUP_Anim);



#if ENABLE_GETBONEATOM_STATS
TArray<FAnimNodeTimeStat> BoneAtomBlendStats;
#endif


#define SHOW_COPYANIMTREE_TIMES (0)

/**
 * Used when drawing in-game animation tree.
 * 
 * @param Weight Weight of Atom between 0.0 and 1.0
 * 
 * @return Color to draw atom.
 */
FColor AnimDebugColorForWeight(FLOAT Weight)
{
	FColor out;

	if( Weight <= ZERO_ANIMWEIGHT_THRESH )
	{
		out = InactiveColor;
	}
	else
	{
		out.R = ZeroWeightColor.R + (BYTE)(Weight * (FLOAT)(FullWeightColor.R - ZeroWeightColor.R));
		out.G = ZeroWeightColor.G + (BYTE)(Weight * (FLOAT)(FullWeightColor.G - ZeroWeightColor.G));
		out.B = ZeroWeightColor.B + (BYTE)(Weight * (FLOAT)(FullWeightColor.B - ZeroWeightColor.B));
		out.A = 128;
	}

	return out;
}

///////////////////////////////////////
//////////// FBoneAtom ////////////////
///////////////////////////////////////

// BoneAtom identity
const FBoneAtom FBoneAtom::Identity(FQuat(0.f,0.f,0.f,1.f), FVector(0.f, 0.f, 0.f), 1.f);

/**
* Does a debugf of the contents of this BoneAtom.
*/
void FBoneAtom::DebugPrint() const
{
	debugf(TEXT("Rotation: %f %f %f %f"), Rotation.X, Rotation.Y, Rotation.Z, Rotation.W);
	debugf(TEXT("Translation: %f %f %f"), Translation.X, Translation.Y, Translation.Z);
	debugf(TEXT("Scale: %f"), Scale);
}

///////////////////////////////////////
//////////// UAnimNode ////////////////
///////////////////////////////////////

/** Get notification that this node has become relevant for the final blend. ie TotalWeight is now > 0 */
void UAnimNode::OnBecomeRelevant()
{
	eventOnBecomeRelevant();
}

/** Get notification that this node is no longer relevant for the final blend. ie TotalWeight is now == 0 */
void UAnimNode::OnCeaseRelevant()
{
	eventOnCeaseRelevant();
}

/**
 * Do any initialisation. Should not reset any state of the animation tree so should be safe to call multiple times.
 * However, the SkeletalMesh may have changed on the SkeletalMeshComponent, so should update any data that relates to that.
 * 
 * @param meshComp SkeletalMeshComponent to which this node of the tree belongs.
 * @param Parent Parent blend node (will be NULL for root note).
 */
void UAnimNode::InitAnim( USkeletalMeshComponent* meshComp, UAnimNodeBlendBase* Parent )
{
	SkelComponent = meshComp;

	eventOnInit();
}


/** AnimSets have been updated, update all animations */
void UAnimNode::AnimSetsUpdated()
{
	// Update bNodeTickStatus flag to avoid updating several times the same nodes.
	if( SkelComponent )
	{
		NodeTickTag = SkelComponent->TickTag;
	}
}


/**
 * Fills the Atoms array with the specified skeletal mesh reference pose.
 * 
 * @param OutAtoms			[out] Output array of relative bone transforms. Must be the same length as RefSkel when calling function.
 * @param DesiredBones		Indices of bones we want to modify. Parents must occur before children.
 * @param RefSkel			Input reference skeleton to create atoms from.
 */
void UAnimNode::FillWithRefPose(TArray<FBoneAtom>& OutAtoms, const TArray<BYTE>& DesiredBones, const TArray<struct FMeshBone>& RefSkel)
{
	check( OutAtoms.Num() == RefSkel.Num() );

	for(INT i=0; i<DesiredBones.Num(); i++)
	{
		const INT BoneIndex				= DesiredBones(i);
		const FMeshBone& RefSkelBone	= RefSkel(BoneIndex);
		FBoneAtom& OutAtom				= OutAtoms(BoneIndex);

		OutAtom.Rotation	= RefSkelBone.BonePos.Orientation;
		OutAtom.Translation	= RefSkelBone.BonePos.Position;
		OutAtom.Scale		= 1.f;
		//Atoms(BoneIndex) = FBoneAtom( RefSkel(BoneIndex).BonePos.Orientation, RefSkel(BoneIndex).BonePos.Position, 1.f );		
	}
}

/** 
 *	Utility for taking an array of bone indices and ensuring that all parents are present 
 *	(ie. all bones between those in the array and the root are present). 
 *	Note that this must ensure the invariant that parent occur before children in BoneIndices.
 */
void UAnimNode::EnsureParentsPresent(TArray<BYTE>& BoneIndices, USkeletalMesh* SkelMesh)
{
	// Iterate through existing array.
	INT i=0;
	while( i<BoneIndices.Num() )
	{
		const BYTE BoneIndex = BoneIndices(i);

		// For the root bone, just move on.
		if( BoneIndex > 0 )
		{
			// Warn if we're getting bad data.
			// Bones are matched as INT, and a non found bone will be set to INDEX_NONE == -1
			// This will be turned into a 255 BYTE
			// This should never happen, so if it does, something is wrong!
			if( BoneIndex >= SkelMesh->RefSkeleton.Num() )
			{
				debugf(TEXT("UAnimNode::EnsureParentsPresent, BoneIndex >= SkelMesh->RefSkeleton.Num()."));
				i++;
				continue;
			}

			const BYTE ParentIndex = SkelMesh->RefSkeleton(BoneIndex).ParentIndex;

			// If we do not have this parent in the array, we add it in this location, and leave 'i' where it is.
			if( !BoneIndices.ContainsItem(ParentIndex) )
			{
				BoneIndices.Insert(i);
				BoneIndices(i) = ParentIndex;
			}
			// If parent was in array, just move on.
			else
			{
				i++;
			}
		}
		else
		{
			i++;
		}
	}
}


/**
 * Get the set of bone 'atoms' (ie. transform of bone relative to parent bone) generated by the blend subtree starting at this node.
 * 
 * @param	Atoms			Output array of bone transforms. Must be correct size when calling function - that is one entry for each bone. Contents will be erased by function though.
 * @param	DesiredBones	Indices of bones that we want to return. Note that bones not in this array will not be modified, so are not safe to access! Parents must occur before children.
 */
void UAnimNode::GetBoneAtoms(TArray<FBoneAtom>& Atoms, const TArray<BYTE>& DesiredBones, FBoneAtom& RootMotionDelta, INT& bHasRootMotion)
{
	START_GETBONEATOM_TIMER

	// No root motion here, move along, nothing to see...
	RootMotionDelta = FBoneAtom::Identity;
	bHasRootMotion	= 0;

	const INT NumAtoms = SkelComponent->SkeletalMesh->RefSkeleton.Num();
	check(NumAtoms == Atoms.Num());
	FillWithRefPose(Atoms, DesiredBones, SkelComponent->SkeletalMesh->RefSkeleton);
}

/** 
 *	Will copy the cached results into the OutAtoms array if they are up to date and return TRUE 
 *	If cache is not up to date, does nothing and returns FALSE.
 */
UBOOL UAnimNode::GetCachedResults(TArray<FBoneAtom>& OutAtoms, FBoneAtom& OutRootMotionDelta, INT& bOutHasRootMotion)
{
	check(SkelComponent);

	// See if results are cached, and cached array is the same size as the target array.
	if( NodeCachedAtomsTag == SkelComponent->CachedAtomsTag && 
		CachedBoneAtoms.Num() == OutAtoms.Num() )
	{
		OutAtoms = CachedBoneAtoms;
		OutRootMotionDelta = CachedRootMotionDelta;
		bOutHasRootMotion = bCachedHasRootMotion;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/** Save the supplied array of BoneAtoms in the CachedBoneAtoms. */
void UAnimNode::SaveCachedResults(const TArray<FBoneAtom>& NewAtoms, const FBoneAtom& NewRootMotionDelta, INT bNewHasRootMotion)
{
	check(SkelComponent);

	// Do not cache results for nodes which only have one parent- it is unnecessary.
	// We make sure the cache is empty.
	if(ParentNodes.Num() < 2)
	{
		CachedBoneAtoms.Empty();
	}
	else
	{
		CachedBoneAtoms = NewAtoms;
		CachedRootMotionDelta = NewRootMotionDelta;
		bCachedHasRootMotion = bNewHasRootMotion;
	}

	// Change flag to indicate cache is up to date
	NodeCachedAtomsTag = SkelComponent->CachedAtomsTag;
}

void UAnimNode::GetNodes(TArray<UAnimNode*>& Nodes)
{
	// we can't start another search while we're already in one as it would invalidate SearchTags on the original search
	check(!UAnimNode::bNodeSearching);

	// set flag allowing GetNodesInternal()
	UAnimNode::bNodeSearching = TRUE;
	// increment search tag so all nodes in the tree will consider themselves once
	UAnimNode::CurrentSearchTag++;
	GetNodesInternal(Nodes);
	// reset flag
	UAnimNode::bNodeSearching = FALSE;
}

/**
 * Find all AnimNodes including and below this one in the tree. Results are arranged so that parents are before children in the array.
 * 
 * @param Nodes Output array of AnimNode pointers.
 */
void UAnimNode::GetNodesInternal(TArray<UAnimNode*>& Nodes)
{
	// make sure we're only called from inside GetNodes() or another GetNodesInternal()
	check(UAnimNode::bNodeSearching);

	if (SearchTag != UAnimNode::CurrentSearchTag)
	{
		SearchTag = UAnimNode::CurrentSearchTag;
		Nodes.AddItem(this);
	}
}

/** Add this node and all children of the specified class to array. Node are added so a parent is always before its children in the array. */
void UAnimNode::GetNodesByClass(TArray<UAnimNode*>& Nodes, UClass* BaseClass)
{
	TArray<UAnimNode*> AllNodes;
	GetNodes(AllNodes);

	// preallocate enough for all nodes to avoid repeated realloc
	Nodes.Reserve(AllNodes.Num());
	for (INT i = 0; i < AllNodes.Num(); i++)
	{
		checkSlow( AllNodes(i) );
		if (AllNodes(i)->IsA(BaseClass))
		{
			Nodes.AddItem(AllNodes(i));
		}
	}
}

/** Return an array with all UAnimNodeSequence childs, including this node. */
void UAnimNode::GetAnimSeqNodes(TArray<UAnimNodeSequence*>& Nodes, FName InSynchGroupName)
{
	TArray<UAnimNode*> AllNodes;
	GetNodes(AllNodes);

	Nodes.Reserve(AllNodes.Num());
	for (INT i = 0; i < AllNodes.Num(); i++)
	{
		UAnimNodeSequence* SeqNode = Cast<UAnimNodeSequence>(AllNodes(i));
		if (SeqNode != NULL && (InSynchGroupName == NAME_None || InSynchGroupName == SeqNode->SynchGroupName))
		{
			Nodes.AddItem(SeqNode);
		}
	}
}

/** Find a node whose NodeName matches InNodeName. Will search this node and all below it. */
UAnimNode* UAnimNode::FindAnimNode(FName InNodeName)
{
	TArray<UAnimNode*> Nodes;
	this->GetNodes( Nodes );

	for(INT i=0; i<Nodes.Num(); i++)
	{
		checkSlow( Nodes(i) );
		if( Nodes(i)->NodeName == InNodeName )
		{
			return Nodes(i);
		}
	}

	return NULL;
}

/** Utility for counting the number of parents of this node that have been ticked. */
INT UAnimNode::CountNumParentsTicked()
{
	INT NumParentsTicked = 0;
	for(INT i=0; i<ParentNodes.Num(); i++)
	{
		if(ParentNodes(i)->NodeTickTag == SkelComponent->TickTag)
		{
			NumParentsTicked++;
		}
	}
	return NumParentsTicked;
}


/** Returns TRUE if this node is a child of given Node */
UBOOL UAnimNode::IsChildOf(UAnimNode* Node)
{
	if( Node == NULL )
	{
		return FALSE;
	}

	if( this == Node )
	{
		return TRUE;
	}

	for(INT i=0; i<ParentNodes.Num(); i++)
	{
		if( ParentNodes(i)->IsChildOf(Node) )
		{
			return TRUE;
		}
	}

	return FALSE;
}

void UAnimNode::PlayAnim(UBOOL bLoop, FLOAT Rate, FLOAT StartTime)
{}

void UAnimNode::StopAnim()
{}

///////////////////////////////////////
///////// UAnimNodeBlendBase //////////
///////////////////////////////////////

/**
 * Do any initialisation. Should not reset any state of the animation tree so should be safe to call multiple times.
 * For a blend, will call InitAnim on any child nodes.
 * 
 * @param meshComp SkeletalMeshComponent to which this node of the tree belongs.
 * @param Parent Parent blend node (will be NULL for root note).
 */
void UAnimNodeBlendBase::InitAnim( USkeletalMeshComponent* meshComp, UAnimNodeBlendBase* Parent)
{
	Super::InitAnim(meshComp, Parent);

	for(INT i=0; i<Children.Num(); i++)
	{
		if( Children(i).Anim )
		{
			// If this is the first time this node has been encountered, call InitAnim on it.
			if(Children(i).Anim->ParentNodes.Num() == 0)
			{
				Children(i).Anim->InitAnim( meshComp, this );
			}

			// Add this node as a parent of the child.
			Children(i).Anim->ParentNodes.AddItem(this);
		}
	}
}

/** Used for building array of AnimNodes in 'tick' order - that is, all parents of a node are added to array before it. */
void UAnimNodeBlendBase::BuildTickArray(TArray<UAnimNode*>& OutTickArray)
{
	// Then iterate over calling BuildTickArray on any children who have had all parents updated.
	for(INT i=0; i<Children.Num(); i++)
	{
		UAnimNode* ChildNode = Children(i).Anim;
		if( ChildNode )
		{
			UBOOL bAllParentsWereTicked = (ChildNode->CountNumParentsTicked() == ChildNode->ParentNodes.Num());
			if( bAllParentsWereTicked && ChildNode->NodeTickTag != SkelComponent->TickTag )
			{
				// Add to array
				OutTickArray.AddItem(ChildNode);
				// Use tick tag to indicate it was added
				ChildNode->NodeTickTag = SkelComponent->TickTag;
				// Call to add its children.
				ChildNode->BuildTickArray(OutTickArray);
			}
		}
	}
}


/** AnimSets have been updated, update all animations */
void UAnimNodeBlendBase::AnimSetsUpdated()
{
	Super::AnimSetsUpdated();

	for(INT i=0; i<Children.Num(); i++)
	{
		// Forward update to nodes who haven't received it yet.
		if( Children(i).Anim && SkelComponent && Children(i).Anim->NodeTickTag != SkelComponent->TickTag )
		{
			Children(i).Anim->AnimSetsUpdated();
		}
	}
}


/** Calculates total weight of children */
void UAnimNodeBlendBase::SetChildrenTotalWeightAccumulator(const INT Index, const FLOAT NodeTotalWeight)
{
	// Update the weight of this connection
	Children(Index).TotalWeight = NodeTotalWeight * Children(Index).Weight;

	// Update the accumulator to find out the combined weight of the child node
	Children(Index).Anim->TotalWeightAccumulator += NodeTotalWeight * Children(Index).Weight;
}


/**
 *	Do any time-dependent work in the blend, changing blend weights etc.
 *	We have code here to ensure that a child node is not ticked until all of its parents have been.
 * 
 * @param DeltaSeconds Size of timestep we are advancing animation tree by. In seconds.
 */
void UAnimNodeBlendBase::TickAnim(FLOAT DeltaSeconds, FLOAT TotalWeight)
{
	// Iterate over each child, updating this nodes contribution to its weight.
	if( SkelComponent )
	{
		for(INT i=0; i<Children.Num(); i++)
		{
			UAnimNode* ChildNode = Children(i).Anim;
			if( ChildNode )
			{
				// Calculate the 'global weight' of the connection to this child.
				SetChildrenTotalWeightAccumulator(i, TotalWeight);
			}
		}
	}
}


/**
 * Find all AnimNodes including and below this one in the tree. Results are arranged so that parents are before children in the array.
 * For a blend node, will recursively call GetNodes on all child nodes.
 * 
 * @param Nodes Output array of AnimNode pointers.
 */
void UAnimNodeBlendBase::GetNodesInternal(TArray<UAnimNode*>& Nodes)
{
	// make sure we're only called from inside GetNodes() or another GetNodesInternal()
	check(UAnimNode::bNodeSearching);

	if (SearchTag != UAnimNode::CurrentSearchTag)
	{
		SearchTag = UAnimNode::CurrentSearchTag;
		Nodes.AddItem(this);
		for (INT i = 0; i < Children.Num(); i++)
		{
			if (Children(i).Anim != NULL)
			{
				Children(i).Anim->GetNodesInternal(Nodes);
			}
		}
	}
}

/**
 * Blends together the Children AnimNodes of this blend based on the Weight in each element of the Children array.
 * Instead of using SLERPs, the blend is done by taking a weighted sum of each atom, and renormalising the quaternion part at the end.
 * This allows n-way blends, and makes the code much faster, though the angular velocity will not be constant across the blend.
 *
 * @param	Atoms			Output array of relative bone transforms.
 * @param	DesiredBones	Indices of bones that we want to return. Note that bones not in this array will not be modified, so are not safe to access! 
 *							This array must be in strictly increasing order.
 */
void UAnimNodeBlendBase::GetBoneAtoms(TArray<FBoneAtom>& Atoms, const TArray<BYTE>& DesiredBones, FBoneAtom& RootMotionDelta, INT& bHasRootMotion)
{
	START_GETBONEATOM_TIMER

	// See if results are cached.
	if( GetCachedResults(Atoms, RootMotionDelta, bHasRootMotion) )
	{
		return;
	}

	// Handle case of a blend with no children.
	if( Children.Num() == 0 )
	{
		RootMotionDelta = FBoneAtom::Identity;
		bHasRootMotion	= 0;
		FillWithRefPose(Atoms, DesiredBones, SkelComponent->SkeletalMesh->RefSkeleton);
		return;
	}

#ifdef _DEBUG
	// Check all children weights sum to 1.0
	if( Abs(GetChildWeightTotal() - 1.f) > ZERO_ANIMWEIGHT_THRESH )
	{
		debugf( TEXT("WARNING: AnimNodeBlendBase (%s) has Children weights which do not sum to 1.0."), *GetName() );

		FLOAT TotalWeight = 0.f;
		for(INT i=0; i<Children.Num(); i++)
		{
			debugf( TEXT("Child: %d Weight: %f"), i, Children(i).Weight );

			TotalWeight += Children(i).Weight;
		}

		debugf( TEXT("Total Weight: %f"), TotalWeight );
		//@todo - adjust first node weight to 

		RootMotionDelta = FBoneAtom::Identity;
		bHasRootMotion	= 0;
		FillWithRefPose(Atoms, DesiredBones, SkelComponent->SkeletalMesh->RefSkeleton);
		return;
	}
#endif

	const INT NumAtoms = SkelComponent->SkeletalMesh->RefSkeleton.Num();
	check( NumAtoms == Atoms.Num() );

	// Find index of the last child with a non-zero weight.
	INT LastChildIndex = INDEX_NONE;
	for(INT i=0; i<Children.Num(); i++)
	{
		if( Children(i).Weight > ZERO_ANIMWEIGHT_THRESH )
		{
			// If this is the only child with any weight, pass Atoms array into it directly.
			if( Children(i).Weight >= (1.f - ZERO_ANIMWEIGHT_THRESH) )
			{
				if( Children(i).Anim )
				{
					EXCLUDE_CHILD_TIME
					if( Children(i).bMirrorSkeleton )
					{
						GetMirroredBoneAtoms(Atoms, i, DesiredBones, RootMotionDelta, bHasRootMotion);
					}
					else
					{
						Children(i).Anim->GetBoneAtoms(Atoms, DesiredBones, RootMotionDelta, bHasRootMotion);
					}
				}
				else
				{
					RootMotionDelta = FBoneAtom::Identity;
					bHasRootMotion	= 0;
					FillWithRefPose(Atoms, DesiredBones, SkelComponent->SkeletalMesh->RefSkeleton);
				}

				SaveCachedResults(Atoms, RootMotionDelta, bHasRootMotion);

				return;
			}
			LastChildIndex = i;
		}
	}
	check(LastChildIndex != INDEX_NONE);

	// We don't allocate this array until we need it.
	TArray<FBoneAtom> ChildAtoms;
	UBOOL bNoChildrenYet = TRUE;

	bHasRootMotion						= 0;
	INT		LastRootMotionChildIndex	= INDEX_NONE;
	FLOAT	AccumulatedRootMotionWeight	= 0.f;

	// Iterate over each child getting its atoms, scaling them and adding them to output (Atoms array)
	for(INT i=0; i<=LastChildIndex; i++)
	{
		// If this child has non-zero weight, blend it into accumulator.
		if( Children(i).Weight > ZERO_ANIMWEIGHT_THRESH )
		{
			// Do need to request atoms, so allocate array here.
			if( ChildAtoms.Num() == 0 )
			{
				ChildAtoms.Add(NumAtoms);
			}

			// Get bone atoms from child node (if no child - use ref pose).
			if( Children(i).Anim )
			{
				EXCLUDE_CHILD_TIME
				if( Children(i).bMirrorSkeleton )
				{
					GetMirroredBoneAtoms(ChildAtoms, i, DesiredBones, Children(i).RootMotion, Children(i).bHasRootMotion);
				}
				else
				{
					Children(i).Anim->GetBoneAtoms(ChildAtoms, DesiredBones, Children(i).RootMotion, Children(i).bHasRootMotion);
				}


				// If this children received root motion information, accumulate its weight
				if( Children(i).bHasRootMotion )
				{
					bHasRootMotion				= 1;
					LastRootMotionChildIndex	= i;
					AccumulatedRootMotionWeight += Children(i).Weight;
				}
			}
			else
			{	
				Children(i).RootMotion		= FBoneAtom::Identity;
				Children(i).bHasRootMotion	= 0;
				FillWithRefPose(ChildAtoms, DesiredBones, SkelComponent->SkeletalMesh->RefSkeleton);
			}

			for(INT j=0; j<DesiredBones.Num(); j++)
			{
				const INT BoneIndex = DesiredBones(j);

				// We just write the first childrens atoms into the output array. Avoids zero-ing it out.
				if( bNoChildrenYet )
				{
					Atoms(BoneIndex) = ChildAtoms(BoneIndex) * Children(i).Weight;
				}
				else
				{
					// @fixme laurent, this is causing issues with AnimNodeMirror :(
					// To ensure the 'shortest route', we make sure the dot product between the accumulator and the incoming child atom is positive.
					if( (Atoms(BoneIndex).Rotation | ChildAtoms(BoneIndex).Rotation) < 0.f )
					{
						ChildAtoms(BoneIndex).Rotation = ChildAtoms(BoneIndex).Rotation * -1.f;
					}

					Atoms(BoneIndex) += ChildAtoms(BoneIndex) * Children(i).Weight;
				}

				// If last child - normalize the rotation quaternion now.
				if( i == LastChildIndex )
				{
					Atoms(BoneIndex).Rotation.Normalize();
				}
			}

			bNoChildrenYet = FALSE;
		}
	}

	// 2nd pass, iterate over all childs who received root motion
	// And blend root motion only between these childs.
	if( bHasRootMotion )
	{
		bNoChildrenYet = TRUE;
		for(INT i=0; i<=LastRootMotionChildIndex; i++)
		{
			if( Children(i).Weight > ZERO_ANIMWEIGHT_THRESH && Children(i).bHasRootMotion )
			{
				const FLOAT	Weight				= Children(i).Weight / AccumulatedRootMotionWeight;
				FBoneAtom	WeightedRootMotion	= Children(i).RootMotion * Weight;


#if 0 // Debug Root Motion
				if( !WeightedRootMotion.Translation.IsZero() )
				{
					debugf(TEXT("%3.2f [%s]  Adding weighted (%3.2f) root motion trans: %3.2f, vect: %s. ChildWeight: %3.3f"), GWorld->GetTimeSeconds(), *SkelComponent->GetOwner()->GetName(), Weight, WeightedRootMotion.Translation.Size(), *WeightedRootMotion.Translation.ToString(), Children(i).Weight);
				}
#endif

				// Accumulate Root Motion
				if( bNoChildrenYet )
				{
					RootMotionDelta = WeightedRootMotion;
					bNoChildrenYet	= FALSE;
				}
				else
				{
					// To ensure the 'shortest route', we make sure the dot product between the accumulator and the incoming child atom is positive.
					if( (RootMotionDelta.Rotation | WeightedRootMotion.Rotation) < 0.f )
					{
						WeightedRootMotion.Rotation = WeightedRootMotion.Rotation * -1.f;
					}

					RootMotionDelta += WeightedRootMotion;
				}
			}
		}

		// Normalize rotation quaternion at the end.
		RootMotionDelta.Rotation.Normalize();
	}

#if 0 // Debug Root Motion
	if( !RootMotionDelta.Translation.IsZero() )
	{
		debugf(TEXT("%3.2f [%s] Root Motion Trans: %3.2f, vect: %s"), GWorld->GetTimeSeconds(), *SkelComponent->GetOwner()->GetName(), RootMotionDelta.Translation.Size(), *RootMotionDelta.Translation.ToString());
	}
#endif

	SaveCachedResults(Atoms, RootMotionDelta, bHasRootMotion);
}


IMPLEMENT_COMPARE_CONSTREF( BYTE, UnAnimTree, { return (A - B); } )

/** 
* Get mirrored bone atoms from desired child index. 
 * Bones are mirrored using the SkelMirrorTable.
 */
void UAnimNodeBlendBase::GetMirroredBoneAtoms(TArray<FBoneAtom>& Atoms, INT ChildIndex, const TArray<BYTE>& DesiredBones, FBoneAtom& RootMotionDelta, INT& bHasRootMotion)
{
	USkeletalMesh* SkelMesh = SkelComponent->SkeletalMesh;
	check(SkelMesh);

	// If mirroring is enabled, and the mirror info array is initialized correctly.
	if( SkelMesh->SkelMirrorTable.Num() == Atoms.Num() )
	{
		// Get atoms from SourceNode.
		TArray<FBoneAtom> ChildAtoms;
		ChildAtoms.Add(Atoms.Num());

		FBoneAtom RMD;
		if( Children(ChildIndex).Anim )
		{
			Children(ChildIndex).Anim->GetBoneAtoms(ChildAtoms, DesiredBones, RMD, bHasRootMotion);
		}
		else
		{
			RMD				= FBoneAtom::Identity;
			bHasRootMotion	= 0;
			FillWithRefPose(ChildAtoms, DesiredBones, SkelMesh->RefSkeleton);
		}

		{
			SCOPE_CYCLE_COUNTER(STAT_MirrorBoneAtoms);


			// We build the mesh-space matrices of the source bones.
			TArray<FMatrix> BoneTM;
			BoneTM.Add(SkelMesh->RefSkeleton.Num());

			for(INT i=0; i<DesiredBones.Num(); i++)
			{	
				const INT BoneIndex = DesiredBones(i);

				if( BoneIndex == 0 )
				{
					ChildAtoms(0).ToTransform(BoneTM(0));
				}
				else
				{
					FMatrix LocalBoneTM;
					ChildAtoms(BoneIndex).ToTransform(LocalBoneTM);

					const INT ParentIndex	= SkelMesh->RefSkeleton(BoneIndex).ParentIndex;
					BoneTM(BoneIndex)		= LocalBoneTM * BoneTM(ParentIndex);
				}
			}

			// Then we do the mirroring.

			// Make array of flags to track which bones have already been mirrored.
			TArray<UBOOL> BoneMirrored;
			BoneMirrored.InsertZeroed(0, Atoms.Num());

			for(INT i=0; i<DesiredBones.Num(); i++)
			{
				const INT BoneIndex = DesiredBones(i);
				if( BoneMirrored(BoneIndex) )
				{
					continue;
				}

				const INT SourceIndex = SkelMesh->SkelMirrorTable(BoneIndex).SourceIndex;

				// Get 'flip axis' from SkeletalMesh, unless we have specified an override for that bone.
				BYTE FlipAxis = SkelMesh->SkelMirrorFlipAxis;
				if( SkelMesh->SkelMirrorTable(BoneIndex).BoneFlipAxis != AXIS_None )
				{
					FlipAxis = SkelMesh->SkelMirrorTable(BoneIndex).BoneFlipAxis;
				}

				// Mirror the root motion delta
				// @fixme laurent -- add support for root rotation and mirroring
				if( BoneIndex == 0 )
				{
					RootMotionDelta = FBoneAtom::Identity;

					if( bHasRootMotion && !RMD.Translation.IsZero() )
					{
						FTranslationMatrix RootTM(RMD.Translation);
						RootTM.Mirror(SkelMesh->SkelMirrorAxis, FlipAxis);
						RootMotionDelta.Translation = RootTM.GetOrigin();
					}
				}

				if( BoneIndex == SourceIndex )
				{
					BoneTM(BoneIndex).Mirror(SkelMesh->SkelMirrorAxis, FlipAxis);
					BoneMirrored(BoneIndex) = TRUE;
				}
				else
				{
					// get source flip axis
					BYTE SourceFlipAxis = SkelMesh->SkelMirrorFlipAxis;
					if( SkelMesh->SkelMirrorTable(SourceIndex).BoneFlipAxis != AXIS_None )
					{
						SourceFlipAxis = SkelMesh->SkelMirrorTable(SourceIndex).BoneFlipAxis;
					}

					FMatrix BoneTransform0 = BoneTM(BoneIndex);
					FMatrix BoneTransform1 = BoneTM(SourceIndex);
					BoneTransform0.Mirror(SkelMesh->SkelMirrorAxis, SourceFlipAxis);
					BoneTransform1.Mirror(SkelMesh->SkelMirrorAxis, FlipAxis);
					BoneTM(BoneIndex)			= BoneTransform1;
					BoneTM(SourceIndex)			= BoneTransform0;
					BoneMirrored(BoneIndex)		= TRUE;
					BoneMirrored(SourceIndex)	= TRUE;
				}
			}

			// Now we need to convert this back into local space transforms.
			for(INT i=0; i<DesiredBones.Num(); i++)
			{
				const INT BoneIndex = DesiredBones(i);

				if( BoneIndex == 0 )
				{
					Atoms(BoneIndex) = FBoneAtom(BoneTM(BoneIndex));
				}
				else
				{
					Atoms(BoneIndex) = FBoneAtom(BoneTM(BoneIndex) * BoneTM(SkelMesh->RefSkeleton(BoneIndex).ParentIndex).Inverse());
				}
			}
		}
	}
	// Otherwise, just pass right through.
	else
	{
		if( Children(ChildIndex).Anim )
		{
			Children(ChildIndex).Anim->GetBoneAtoms(Atoms, DesiredBones, RootMotionDelta, bHasRootMotion);
		}
		else
		{
			RootMotionDelta	= FBoneAtom::Identity;
			bHasRootMotion	= 0;
			FillWithRefPose(Atoms, DesiredBones, SkelComponent->SkeletalMesh->RefSkeleton);
		}
	}
}

/**
 * For debugging.
 * 
 * @return Sum weight of all children weights. Should always be 1.0
 */
FLOAT UAnimNodeBlendBase::GetChildWeightTotal()
{
	FLOAT TotalWeight = 0.f;

	for(INT i=0; i<Children.Num(); i++)
	{
		TotalWeight += Children(i).Weight;
	}

	return TotalWeight;
}

/**
 * Make sure to relay OnChildAnimEnd to Parent AnimNodeBlendBase as long as it exists 
 */ 
void UAnimNodeBlendBase::OnChildAnimEnd(UAnimNodeSequence* Child, FLOAT PlayedTime, FLOAT ExcessTime) 
{ 
	for(INT i=0; i<ParentNodes.Num(); i++)
	{
		ParentNodes(i)->OnChildAnimEnd(Child, PlayedTime, ExcessTime); 
	}
} 

void UAnimNodeBlendBase::PlayAnim(UBOOL bLoop, FLOAT Rate, FLOAT StartTime)
{
	// pass on the call to our children
	for (INT i = 0; i < Children.Num(); i++)
	{
		if (Children(i).Anim != NULL)
		{
			Children(i).Anim->PlayAnim(bLoop, Rate, StartTime);
		}
	}
}

void UAnimNodeBlendBase::StopAnim()
{
	// pass on the call to our children
	for (INT i = 0; i < Children.Num(); i++)
	{
		if (Children(i).Anim != NULL)
		{
			Children(i).Anim->StopAnim();
		}
	}
}

/** Rename all child nodes upon Add/Remove, so they match their position in the array. */
void UAnimNodeBlendBase::RenameChildConnectors()
{
	for(INT i=0; i<Children.Num(); i++)
	{
		FString NewChildName = FString::Printf( TEXT("Child%d"), i + 1 );
		Children(i).Name = FName( *NewChildName );
	}
}

/** A child connector has been added */
void UAnimNodeBlendBase::OnAddChild(INT ChildNum)
{
	// Make sure name matches position in array.
	RenameChildConnectors();
}

/** A child connector has been removed */
void UAnimNodeBlendBase::OnRemoveChild(INT ChildNum)
{
	// Make sure name matches position in array.
	RenameChildConnectors();
}


///////////////////////////////////////
//////////// UAnimNodeBlend ///////////
///////////////////////////////////////

/** @see UAnimNode::TickAnim */
void UAnimNodeBlend::TickAnim(FLOAT DeltaSeconds, FLOAT TotalWeight)
{
	//@todo: Fix me.  Asserting should not occur here.  We should pop up an error or
	//       fix the asset in CheckErrors / 
	//check(Children.Num() == 2);
	if( Children.Num() != 2 )
	{
		return;
	}
	   
	if( BlendTimeToGo != 0.f )
	{
		// Amount we want to change Child2Weight by.
		const FLOAT BlendDelta = Child2WeightTarget - Child2Weight; 

		if( Abs(BlendDelta) > KINDA_SMALL_NUMBER && BlendTimeToGo > DeltaSeconds )
		{
			Child2Weight	+= (BlendDelta / BlendTimeToGo) * DeltaSeconds;
			BlendTimeToGo	-= DeltaSeconds;
		}
		else
		{
			Child2Weight	= Child2WeightTarget;
			BlendTimeToGo	= 0.f;
		}
	}

	// debugf(TEXT("Blender: %s (%x) Child2Weight: %f BlendTimeToGo: %f"), *GetPathName(), (INT)this, Child2BoneWeights.ChannelWeight, BlendTimeToGo);
	Children(0).Weight = 1.f - Child2Weight;
	Children(1).Weight = Child2Weight;

	// Tick children
	Super::TickAnim(DeltaSeconds, TotalWeight);
}


/**
 * Set desired balance of this blend.
 * 
 * @param BlendTarget Target amount of weight to put on Children(1) (second child). Between 0.0 and 1.0. 1.0 means take all animation from second child.
 * @param BlendTime How long to take to get to BlendTarget.
 */
void UAnimNodeBlend::SetBlendTarget(FLOAT BlendTarget, FLOAT BlendTime)
{
	Child2WeightTarget = Clamp<FLOAT>(BlendTarget, 0.f, 1.f);
	
	// If we want this weight NOW - update weights straight away (dont wait for TickAnim).
	if( BlendTime <= 0.0f )
	{
		Child2Weight		= Child2WeightTarget;
		Children(0).Weight	= 1.f - Child2Weight;
		Children(1).Weight	= Child2Weight;
	}

	BlendTimeToGo = BlendTime;
}


///////////////////////////////////////
///////////// AnimNodeCrossfader //////
///////////////////////////////////////

/** @see UAnimNode::InitAnim */
void UAnimNodeCrossfader::InitAnim( USkeletalMeshComponent* meshComp, UAnimNodeBlendBase* Parent )
{
	Super::InitAnim( meshComp, Parent );
	
	UAnimNodeSequence	*ActiveChild = GetActiveChild();
	if( ActiveChild && ActiveChild->AnimSeqName == NAME_None )
	{
		SetAnim( DefaultAnimSeqName );
	}
}

/** @see UAnimNode::TickAnim */
void UAnimNodeCrossfader::TickAnim( FLOAT DeltaSeconds, FLOAT TotalWeight )
{
	if( !bDontBlendOutOneShot && PendingBlendOutTimeOneShot > 0.f )
	{
		UAnimNodeSequence	*ActiveChild = GetActiveChild();

		if( ActiveChild && ActiveChild->AnimSeq )
		{
			FLOAT	fCountDown = ActiveChild->AnimSeq->SequenceLength - ActiveChild->CurrentTime;
			
			// if playing a one shot anim, and it's time to blend back to previous animation, do so.
			if( fCountDown <= PendingBlendOutTimeOneShot )
			{
				SetBlendTarget( 1.f - Child2WeightTarget, PendingBlendOutTimeOneShot );
				PendingBlendOutTimeOneShot = 0.f;
			}
		}
	}

	// Tick children
	Super::TickAnim( DeltaSeconds, TotalWeight );
}

/** @see AnimCrossfader::GetAnimName */
FName UAnimNodeCrossfader::GetAnimName()
{
	UAnimNodeSequence	*ActiveChild = GetActiveChild();
	if( ActiveChild )
	{
		return ActiveChild->AnimSeqName;
	}
	else
	{
		return NAME_None;
	}
}

/**
 * Get active AnimNodeSequence child. To access animation properties and control functions.
 *
 * @return	AnimNodeSequence currently playing.
 */
UAnimNodeSequence *UAnimNodeCrossfader::GetActiveChild()
{
	// requirements for the crossfader. Just exit if not met, do not crash.
	if( Children.Num() != 2 ||	// needs 2 childs
		!Children(0).Anim ||	// properly connected
		!Children(1).Anim )
	{
		return NULL;
	}

	return Cast<UAnimNodeSequence>(Child2WeightTarget < 0.5f ? Children(0).Anim : Children(1).Anim);
}

/** @see AnimNodeCrossFader::PlayOneShotAnim */
void UAnimNodeCrossfader::execPlayOneShotAnim( FFrame& Stack, RESULT_DECL )
{
	P_GET_NAME(AnimSeqName);
	P_GET_FLOAT_OPTX(BlendInTime,0.f); 
	P_GET_FLOAT_OPTX(BlendOutTime,0.f);
	P_GET_UBOOL_OPTX(bDontBlendOut,false);
	P_GET_FLOAT_OPTX(Rate,1.f);
	P_FINISH;

	// requirements for the crossfader. Just exit if not met, do not crash.
	if( Children.Num() != 2 ||	// needs 2 childs
		!Children(0).Anim ||	// properly connected
		!Children(1).Anim ||
		SkelComponent == NULL )
	{
		return;
	}

	// Make sure AnimSeqName exists
	if( SkelComponent->FindAnimSequence( AnimSeqName ) == NULL )
	{
		debugf( NAME_Warning,TEXT("%s - Failed to find animsequence '%s' on SkeletalMeshComponent: '%s' whose owner is: '%s' and is of type %s" ),
			*GetName(),
			*AnimSeqName.ToString(),
			*SkelComponent->GetName(), 
			*SkelComponent->GetOwner()->GetName(),
			*SkelComponent->TemplateName.ToString()
			);
		return;
	}

	UAnimNodeSequence*	Child = Cast<UAnimNodeSequence>(Child2WeightTarget < 0.5f ? Children(1).Anim : Children(0).Anim);

	if( Child )
	{
		FLOAT	BlendTarget			= Child2WeightTarget < 0.5f ? 1.f : 0.f;

		bDontBlendOutOneShot		= bDontBlendOut;
		PendingBlendOutTimeOneShot	= BlendOutTime;

		Child->SetAnim(AnimSeqName);
		Child->PlayAnim(false, Rate);
		SetBlendTarget( BlendTarget, BlendInTime );
	}
}

/** @see AnimNodeCrossFader::BlendToLoopingAnim */
void UAnimNodeCrossfader::execBlendToLoopingAnim( FFrame& Stack, RESULT_DECL )
{
	P_GET_NAME(AnimSeqName);
	P_GET_FLOAT_OPTX(BlendInTime,0.f);
	P_GET_FLOAT_OPTX(Rate,1.f);
	P_FINISH;

	// requirements for the crossfader. Just exit if not met, do not crash.
	if( Children.Num() != 2 ||	// needs 2 childs
		!Children(0).Anim ||	// properly connected
		!Children(1).Anim ||
		SkelComponent == NULL )
	{
		return;
	}

	// Make sure AnimSeqName exists
	if( SkelComponent->FindAnimSequence( AnimSeqName ) == NULL )
	{
		debugf( NAME_Warning,TEXT("%s - Failed to find animsequence '%s' on SkeletalMeshComponent: '%s' whose owner is: '%s' and is of type %s" ),
			*GetName(),
			*AnimSeqName.ToString(),
			*SkelComponent->GetName(), 
			*SkelComponent->GetOwner()->GetName(),
			*SkelComponent->TemplateName.ToString()
			);
		return;
	}

	UAnimNodeSequence*	Child = Cast<UAnimNodeSequence>(Child2WeightTarget < 0.5f ? Children(1).Anim : Children(0).Anim);

	if( Child )
	{
		FLOAT	BlendTarget			= Child2WeightTarget < 0.5f ? 1.f : 0.f;

		// One shot variables..
		bDontBlendOutOneShot		= true;
		PendingBlendOutTimeOneShot	= 0.f;

		Child->SetAnim(AnimSeqName);
		Child->PlayAnim(true, Rate);
		SetBlendTarget( BlendTarget, BlendInTime );
	}
}


/************************************************************************************
 * AnimNodeBlendPerBone
 ***********************************************************************************/

/** 
 * Calculates total weight of children. 
 * Set a full weight on source, because it's potentially always feeding animations into the final blend.
 */
void UAnimNodeBlendPerBone::SetChildrenTotalWeightAccumulator(const INT Index, const FLOAT NodeTotalWeight)
{
	if( Index == 0 )
	{
		// Update the weight of this connection
		Children(Index).TotalWeight = NodeTotalWeight;
		// Update the accumulator to find out the combined weight of the child node
		Children(Index).Anim->TotalWeightAccumulator += NodeTotalWeight;
	}
	else
	{
		// Update the weight of this connection
		Children(Index).TotalWeight = NodeTotalWeight * Children(Index).Weight;
		// Update the accumulator to find out the combined weight of the child node
		Children(Index).Anim->TotalWeightAccumulator += NodeTotalWeight * Children(Index).Weight;
	}
}

void UAnimNodeBlendPerBone::PostEditChange(UProperty* PropertyThatChanged)
{
	if( PropertyThatChanged && 
		(PropertyThatChanged->GetFName() == FName(TEXT("BranchStartBoneName"))) )
	{
		BuildWeightList();
	}

	Super::PostEditChange(PropertyThatChanged);
}

void UAnimNodeBlendPerBone::BuildWeightList()
{
	if( !SkelComponent || !SkelComponent->SkeletalMesh )
	{
		return;
	}

	TArray<FMeshBone>& RefSkel	= SkelComponent->SkeletalMesh->RefSkeleton;
	const INT NumAtoms			= RefSkel.Num();

	Child2PerBoneWeight.Reset();
	Child2PerBoneWeight.AddZeroed(NumAtoms);

	for(INT i=0; i<NumAtoms; i++)
	{
		const	INT		BoneIndex	= i;
		UBOOL	bBranchBone = FALSE;

		for(INT NameIndex=0; NameIndex<BranchStartBoneName.Num(); NameIndex++)
		{
			if( BranchStartBoneName(NameIndex) != NAME_None )
			{
				const INT BranchBoneIndex = SkelComponent->MatchRefBone(BranchStartBoneName(NameIndex));
				if( BranchBoneIndex != INDEX_NONE && BranchBoneIndex == BoneIndex )
				{
					bBranchBone = TRUE;
					break;
				}
			}
		}

		if( bBranchBone )
		{
			Child2PerBoneWeight(BoneIndex) = 1.f;
		}
		else
		{
			if( BoneIndex > 0 )
			{
				Child2PerBoneWeight(BoneIndex) = Child2PerBoneWeight(RefSkel(BoneIndex).ParentIndex);
			}
		}
	}

	// build list of required bones
	LocalToCompReqBones.Empty();

	FLOAT LastWeight = Child2PerBoneWeight(0);
	for(INT i=0; i<NumAtoms; i++)
	{
		// if weight different than previous one, then this bone needs to be blended in component space
		if( Child2PerBoneWeight(i) != LastWeight )
		{
			LocalToCompReqBones.AddItem(i);
			LastWeight = Child2PerBoneWeight(i);
		}
	}
	EnsureParentsPresent(LocalToCompReqBones, SkelComponent->SkeletalMesh);
}


/** 
 *	Utility for taking two arrays of bytes, which must be strictly increasing, and finding the intersection between them.
 *	That is - any item in the output should be present in both A and B. Output is strictly increasing as well.
 */
static void IntersectByteArrays(TArray<BYTE>& Output, const TArray<BYTE>& A, const TArray<BYTE>& B)
{
	INT APos = 0;
	INT BPos = 0;
	while(	APos < A.Num() && BPos < B.Num() )
	{
		// If value at APos is lower, increment APos.
		if( A(APos) < B(BPos) )
		{
			APos++;
		}
		// If value at BPos is lower, increment APos.
		else if( B(BPos) < A(APos) )
		{
			BPos++;
		}
		// If they are the same, put value into output, and increment both.
		else
		{
			Output.AddItem( A(APos) );
			APos++;
			BPos++;
		}
	}
}



void UAnimNodeBlendPerBone::InitAnim(USkeletalMeshComponent* meshComp, UAnimNodeBlendBase* Parent)
{
	Super::InitAnim(meshComp, Parent);

	BuildWeightList();
}

// Arrays used for temporary bone blending
static TArray<FMatrix> Child1CompSpace;
static TArray<FMatrix> Child2CompSpace;
static TArray<FMatrix> ResultCompSpace;

/** @see UAnimNode::GetBoneAtoms. */
void UAnimNodeBlendPerBone::GetBoneAtoms(TArray<FBoneAtom>& Atoms, const TArray<BYTE>& DesiredBones, FBoneAtom& RootMotionDelta, INT& bHasRootMotion)
{
	START_GETBONEATOM_TIMER

	// See if results are cached.
	if( GetCachedResults(Atoms, RootMotionDelta, bHasRootMotion) )
	{
		return;
	}

	// If drawing all from Children(0), no need to look at Children(1). Pass Atoms array directly to Children(0).
	if( Children(0).Weight >= (1.f - ZERO_ANIMWEIGHT_THRESH) )
	{
		if( Children(0).Anim )
		{
			EXCLUDE_CHILD_TIME
			Children(0).Anim->GetBoneAtoms(Atoms, DesiredBones, RootMotionDelta, bHasRootMotion);
		}
		else
		{
			RootMotionDelta = FBoneAtom::Identity;
			bHasRootMotion	= 0;
			FillWithRefPose(Atoms, DesiredBones, SkelComponent->SkeletalMesh->RefSkeleton);
		}

		SaveCachedResults(Atoms, RootMotionDelta, bHasRootMotion);
		return;
	}

	TArray<FMeshBone>& RefSkel = SkelComponent->SkeletalMesh->RefSkeleton;
	const INT NumAtoms = RefSkel.Num();

	TArray<FBoneAtom> Child1Atoms, Child2Atoms;

	// Get bone atoms from each child (if no child - use ref pose).
	Child1Atoms.Add(NumAtoms);
	FBoneAtom	Child1RMD				= FBoneAtom::Identity;
	INT  		bChild1HasRootMotion	= FALSE;
	if( Children(0).Anim )
	{
		EXCLUDE_CHILD_TIME
		Children(0).Anim->GetBoneAtoms(Child1Atoms, DesiredBones, Child1RMD, bChild1HasRootMotion);
	}
	else
	{
		FillWithRefPose(Child1Atoms, DesiredBones, SkelComponent->SkeletalMesh->RefSkeleton);
	}

	// Get only the necessary bones from child2. The ones that have a Child2PerBoneWeight(BoneIndex) > 0
	Child2Atoms.Add(NumAtoms);
	FBoneAtom	Child2RMD				= FBoneAtom::Identity;
	INT		bChild2HasRootMotion	= FALSE;

	//debugf(TEXT("child2 went from %d bones to %d bones."), DesiredBones.Num(), Child2DesiredBones.Num() );
	if( Children(1).Anim )
	{
		EXCLUDE_CHILD_TIME
		Children(1).Anim->GetBoneAtoms(Child2Atoms, DesiredBones, Child2RMD, bChild2HasRootMotion);
	}
	else
	{
		FillWithRefPose(Child2Atoms, DesiredBones, SkelComponent->SkeletalMesh->RefSkeleton);
	}

	// If we are doing component-space blend, ensure working buffers are big enough
	if(!bForceLocalSpaceBlend)
	{
		Child1CompSpace.Reset();
		Child1CompSpace.Add(NumAtoms);

		Child2CompSpace.Reset();
		Child2CompSpace.Add(NumAtoms);

		ResultCompSpace.Reset();
		ResultCompSpace.Add(NumAtoms);
	}

	INT LocalToCompReqIndex = 0;

	// do blend
	for(INT i=0; i<DesiredBones.Num(); i++)
	{
		const INT	BoneIndex			= DesiredBones(i);
		const FLOAT Child2BoneWeight	= Children(1).Weight * Child2PerBoneWeight(BoneIndex);

		//debugf(TEXT("  (%2d) %1.1f %s"), BoneIndex, Child2PerBoneWeight(BoneIndex), *RefSkel(BoneIndex).Name);
		const UBOOL bDoComponentSpaceBlend =	LocalToCompReqIndex < LocalToCompReqBones.Num() && 
												BoneIndex == LocalToCompReqBones(LocalToCompReqIndex);

		if( !bForceLocalSpaceBlend && bDoComponentSpaceBlend )
		{
			//debugf(TEXT("  (%2d) %1.1f %s"), BoneIndex, Child2PerBoneWeight(BoneIndex), *RefSkel(BoneIndex).Name);
			LocalToCompReqIndex++;

			const INT ParentIndex	= RefSkel(BoneIndex).ParentIndex;

			// turn bone atoms to matrices
			Child1Atoms(BoneIndex).ToTransform(Child1CompSpace(BoneIndex));
			Child2Atoms(BoneIndex).ToTransform(Child2CompSpace(BoneIndex));

			// transform to component space
			if( BoneIndex > 0 )
			{
				Child1CompSpace(BoneIndex) *= Child1CompSpace(ParentIndex);
				Child2CompSpace(BoneIndex) *= Child2CompSpace(ParentIndex);
			}

			// everything comes from child1 copy directly
			if( Child2BoneWeight <= ZERO_ANIMWEIGHT_THRESH )
			{
				ResultCompSpace(BoneIndex) = Child1CompSpace(BoneIndex);
			}
			// everything comes from child2, copy directly
			else if( Child2BoneWeight >= (1.f - ZERO_ANIMWEIGHT_THRESH) )
			{
				ResultCompSpace(BoneIndex) = Child2CompSpace(BoneIndex);
			}
			// blend rotation in component space of both childs
			else
			{
				// convert matrices to FBoneAtoms
				FBoneAtom Child1CompSpaceAtom(Child1CompSpace(BoneIndex));
				FBoneAtom Child2CompSpaceAtom(Child2CompSpace(BoneIndex));

				// blend FBoneAtom rotation. We store everything in Child1

				// We use a linear interpolation and a re-normalize for the rotation.
				// Treating Rotation as an accumulator, initialise to a scaled version of Atom1.
				Child1CompSpaceAtom.Rotation = Child1CompSpaceAtom.Rotation * (1.f - Child2BoneWeight);

				// To ensure the 'shortest route', we make sure the dot product between the accumulator and the incoming rotation is positive.
				if( (Child1CompSpaceAtom.Rotation | Child2CompSpaceAtom.Rotation) < 0.f )
				{
					Child1CompSpaceAtom.Rotation = Child1CompSpaceAtom.Rotation * -1.f;
				}

				// Then add on the second rotation..
				Child1CompSpaceAtom.Rotation = Child1CompSpaceAtom.Rotation + (Child2CompSpaceAtom.Rotation * Child2BoneWeight);

				// ..and renormalize
				Child1CompSpaceAtom.Rotation.Normalize();

				// convert back FBoneAtom to FMatrix
				Child1CompSpaceAtom.ToTransform(ResultCompSpace(BoneIndex));
			}

			// Blend Translation and Scale in local space
			if( Child2BoneWeight <= ZERO_ANIMWEIGHT_THRESH )
			{
				// if blend is all the way for child1, then just copy its bone atoms
				Atoms(BoneIndex) = Child1Atoms(BoneIndex);
			}
			else if( Child2BoneWeight >= (1.f - ZERO_ANIMWEIGHT_THRESH) )
			{
				// if blend is all the way for child2, then just copy its bone atoms
				Atoms(BoneIndex) = Child2Atoms(BoneIndex);
			}
			else
			{
				// Simple linear interpolation for translation and scale.
				Atoms(BoneIndex).Translation	= Lerp(Child1Atoms(BoneIndex).Translation, Child2Atoms(BoneIndex).Translation, Child2BoneWeight);
				Atoms(BoneIndex).Scale			= Lerp(Child1Atoms(BoneIndex).Scale, Child2Atoms(BoneIndex).Scale, Child2BoneWeight);
			}

			// and rotation was blended in component space...
			// convert bone atom back to local space
			FMatrix ParentTM = FMatrix::Identity;
			if( BoneIndex > 0 )
			{
				ParentTM = ResultCompSpace(ParentIndex);
			}

			// Then work out relative transform, and convert to FBoneAtom.
			const FMatrix RelTM = ResultCompSpace(BoneIndex) * ParentTM.Inverse();				
			Atoms(BoneIndex).Rotation = FBoneAtom(RelTM).Rotation;
		}	
		else
		{
			// otherwise do faster local space blending.
			Atoms(BoneIndex).Blend(Child1Atoms(BoneIndex), Child2Atoms(BoneIndex), Child2BoneWeight);
		}

		// Blend root motion
		if( BoneIndex == 0 )
		{
			bHasRootMotion = bChild1HasRootMotion || bChild2HasRootMotion;

			if( bChild1HasRootMotion && bChild2HasRootMotion )
			{
				RootMotionDelta.Blend(Child1RMD, Child2RMD, Child2BoneWeight);
			}
			else if( bChild1HasRootMotion )
			{
				RootMotionDelta = Child1RMD;
			}
			else if( bChild2HasRootMotion )
			{
				RootMotionDelta = Child2RMD;
			}
		}
	}

	SaveCachedResults(Atoms, RootMotionDelta, bHasRootMotion);
}


///////////////////////////////////////
/////// AnimNodeBlendDirectional //////
///////////////////////////////////////


/**
 * Updates weight of the 4 directional animation children by looking at the current velocity and heading of actor.
 * 
 * @see UAnimNode::TickAnim
 */
void UAnimNodeBlendDirectional::TickAnim( FLOAT DeltaSeconds, FLOAT TotalWeight )
{
	check(Children.Num() == 4);

	// Calculate DirAngle based on player velocity.
	AActor* actor = SkelComponent->GetOwner(); // Get the actor to use for acceleration/look direction etc.
	if( actor )
	{
		FLOAT TargetDirAngle = 0.f;
		FVector	VelDir = actor->Velocity;
		VelDir.Z = 0.0f;

		if( VelDir.IsNearlyZero() )
		{
			TargetDirAngle = 0.f;
		}
		else
		{
			VelDir = VelDir.SafeNormal();

			FVector LookDir = actor->Rotation.Vector();
			LookDir.Z = 0.f;
			LookDir = LookDir.SafeNormal();

			FVector LeftDir = LookDir ^ FVector(0.f,0.f,1.f);
			LeftDir = LeftDir.SafeNormal();

			FLOAT ForwardPct = (LookDir | VelDir);
			FLOAT LeftPct = (LeftDir | VelDir);

			TargetDirAngle = appAcos( Clamp<FLOAT>(ForwardPct, -1.f, 1.f) );
			if( LeftPct > 0.f )
			{
				TargetDirAngle *= -1.f;
			}
		}
		// Move DirAngle towards TargetDirAngle as fast as DirRadsPerSecond allows
		FLOAT DeltaDir = FindDeltaAngle(DirAngle, TargetDirAngle);
		if( DeltaDir != 0.f )
		{
			FLOAT MaxDelta = DeltaSeconds * DirDegreesPerSecond * (PI/180.f);
			DeltaDir = Clamp<FLOAT>(DeltaDir, -MaxDelta, MaxDelta);
			DirAngle = UnwindHeading( DirAngle + DeltaDir );
		}
	}

	// Option to only choose one animation
	if(SkelComponent->PredictedLODLevel < SingleAnimAtOrAboveLOD)
	{
		// Work out child weights based on DirAngle.
		if(DirAngle < -0.5f*PI) // Back and left
		{
			Children(2).Weight = (DirAngle/(0.5f*PI)) + 2.f;
			Children(3).Weight = 0.f;

			Children(0).Weight = 0.f;
			Children(1).Weight = 1.f - Children(2).Weight;
		}
		else if(DirAngle < 0.f) // Forward and left
		{
			Children(2).Weight = -DirAngle/(0.5f*PI);
			Children(3).Weight = 0.f;

			Children(0).Weight = 1.f - Children(2).Weight;
			Children(1).Weight = 0.f;
		}
		else if(DirAngle < 0.5f*PI) // Forward and right
		{
			Children(2).Weight = 0.f;
			Children(3).Weight = DirAngle/(0.5f*PI);

			Children(0).Weight = 1.f - Children(3).Weight;
			Children(1).Weight = 0.f;
		}
		else // Back and right
		{
			Children(2).Weight = 0.f;
			Children(3).Weight = (-DirAngle/(0.5f*PI)) + 2.f;

			Children(0).Weight = 0.f;
			Children(1).Weight = 1.f - Children(3).Weight;
		}
	}
	else
	{
		Children(0).Weight = 0.f;
		Children(1).Weight = 0.f;
		Children(2).Weight = 0.f;
		Children(3).Weight = 0.f;

		if(DirAngle < -0.75f*PI) // Back
		{
			Children(1).Weight = 1.f;
		}
		else if(DirAngle < -0.25f*PI) // Left
		{
			Children(2).Weight = 1.f;
		}
		else if(DirAngle < 0.25f*PI) // Forward
		{
			Children(0).Weight = 1.f;
		}
		else if(DirAngle < 0.75f*PI) // Right
		{
			Children(3).Weight = 1.f;
		}
		else // Back
		{
			Children(1).Weight = 1.f;
		}
	}
	
	// Update children weights
	Super::TickAnim(DeltaSeconds, TotalWeight);
}


///////////////////////////////////////
/////////// AnimNodeBlendList /////////
///////////////////////////////////////

/**
 * Will ensure TargetWeight array is right size. If it has to resize it, will set Chidlren(0) to have a target of 1.0.
 * Also, if all Children weights are zero, will set Children(0) as the active child.
 * 
 * @see UAnimNode::InitAnim
 */
void UAnimNodeBlendList::InitAnim( USkeletalMeshComponent* meshComp, UAnimNodeBlendBase* Parent )
{
	Super::InitAnim(meshComp, Parent);

	if( TargetWeight.Num() != Children.Num() )
	{
		TargetWeight.Reset();
		TargetWeight.AddZeroed( Children.Num() );

		if( TargetWeight.Num() > 0 )
		{
			TargetWeight(0) = 1.f;
		}
	}

	// If all child weights are zero - set the first one to the active child.
	const FLOAT ChildWeightSum = GetChildWeightTotal();
	if( ChildWeightSum <= ZERO_ANIMWEIGHT_THRESH )
	{
		SetActiveChild(ActiveChildIndex, 0.f);
	}
}


/** @see UAnimNode::TickAnim */
void UAnimNodeBlendList::TickAnim(FLOAT DeltaSeconds, FLOAT TotalWeight)
{
	check(Children.Num() == TargetWeight.Num());

	// Do nothing if BlendTimeToGo is zero.
	if( BlendTimeToGo > 0.f )
	{
		// So we don't overshoot.
		if( BlendTimeToGo <= DeltaSeconds )
		{
			BlendTimeToGo = 0.f; 

			for(INT i=0; i<Children.Num(); i++)
			{
				Children(i).Weight = TargetWeight(i);
			}
		}
		else
		{
			for(INT i=0; i<Children.Num(); i++)
			{
				// Amount left to blend.
				const FLOAT BlendDelta = TargetWeight(i) - Children(i).Weight;
				Children(i).Weight += (BlendDelta / BlendTimeToGo) * DeltaSeconds;
			}

			BlendTimeToGo -= DeltaSeconds;
		}
	}

	// Update child node weights
	Super::TickAnim(DeltaSeconds, TotalWeight);
}


/**
 * The the child to blend up to full Weight (1.0)
 * 
 * @param ChildIndex Index of child node to ramp in. If outside range of children, will set child 0 to active.
 * @param BlendTime How long for child to take to reach weight of 1.0.
 */
void UAnimNodeBlendList::SetActiveChild( INT ChildIndex, FLOAT BlendTime )
{
	check(Children.Num() == TargetWeight.Num());
	
	if( ChildIndex < 0 || ChildIndex >= Children.Num() )
	{
		debugf( TEXT("UAnimNodeBlendList::SetActiveChild : %s ChildIndex (%d) outside number of Children (%d)."), *GetName(), ChildIndex, Children.Num() );
		ChildIndex = 0;
	}

	for(INT i=0; i<Children.Num(); i++)
	{
		if(i == ChildIndex)
		{
			TargetWeight(i) = 1.0f;

			// If we basically want this straight away - dont wait until a tick to update weights.
			if(BlendTime == 0.0f)
			{
				Children(i).Weight = 1.0f;
			}
		}
		else
		{
			TargetWeight(i) = 0.0f;

			if(BlendTime == 0.0f)
			{
				Children(i).Weight = 0.0f;
			}
		}
	}

	BlendTimeToGo = BlendTime;
	ActiveChildIndex = ChildIndex;

	if( bPlayActiveChild )
	{
		// Play the animation if this child is a sequence
		UAnimNodeSequence* AnimSeq = Cast<UAnimNodeSequence>(Children(ActiveChildIndex).Anim);
		if( AnimSeq )
		{
			AnimSeq->PlayAnim( AnimSeq->bLooping, AnimSeq->Rate );
		}
	}
}

/**
 * Reset the specified TargetWeight array to the given number of children.
 */
static void ResetTargetWeightArray(TArrayNoInit<FLOAT>& TargetWeight, INT ChildNum)
{
	TargetWeight.Empty();
	if( ChildNum > 0 )
	{
		TargetWeight.AddZeroed( ChildNum );
		TargetWeight(0) = 1.f;
	}
}

/** Called when we add a child to this node. We reset the TargetWeight array when this happens. */
void UAnimNodeBlendList::OnAddChild(INT ChildNum)
{
	Super::OnAddChild(ChildNum);

	ResetTargetWeightArray( TargetWeight, Children.Num() );
}

/** Called when we remove a child to this node. We reset the TargetWeight array when this happens. */
void UAnimNodeBlendList::OnRemoveChild(INT ChildNum)
{
	Super::OnRemoveChild(ChildNum);

	ResetTargetWeightArray( TargetWeight, Children.Num() );
}

/**
 * Blend animations based on an Owner's posture.
 *
 * @param DeltaSeconds	Time since last tick in seconds.
 */
void UAnimNodeBlendByPosture::TickAnim( FLOAT DeltaSeconds, FLOAT TotalWeight )
{
	APawn* Owner = SkelComponent ? Cast<APawn>(SkelComponent->GetOwner()) : NULL;

	if ( Owner )
	{
		if ( Owner->bIsCrouched )
		{
			if ( ActiveChildIndex != 1 )
			{
				SetActiveChild( 1, 0.1f );
			}
		}
		else if ( ActiveChildIndex != 0 )
		{
			SetActiveChild( 0 , 0.1f );
		}
	}

	Super::TickAnim(DeltaSeconds, TotalWeight);
}


/************************************************************************************
 * UAnimNodeBlendBySpeed
 ***********************************************************************************/

/**
 * Resets the last channel on becoming active.	
 */
void UAnimNodeBlendBySpeed::OnBecomeRelevant()
{
	Super::OnBecomeRelevant();
	LastChannel = -1;
}

/**
 * Blend animations based on an Owner's velocity.
 *
 * @param DeltaSeconds	Time since last tick in seconds.
 */
void UAnimNodeBlendBySpeed::TickAnim( FLOAT DeltaSeconds, FLOAT TotalWeight )
{
	INT		NumChannels				= Children.Num();
	UBOOL	SufficientChannels		= NumChannels >= 2,
			SufficientConstraints	= Constraints.Num() >= NumChannels;

	if( SufficientChannels && SufficientConstraints )
	{
		INT		TargetChannel	= 0;

		// Get the speed we should use for the blend.
		Speed = CalcSpeed();

		// Find appropriate channel for current speed with "Constraints" containing an upper speed bound.
		while( (TargetChannel < NumChannels-1) && (Speed > Constraints(TargetChannel)) )
		{
			TargetChannel++;
		}

		// See if we need to blend down.
		if( TargetChannel > 0 )
		{
			FLOAT SpeedRatio = (Speed - Constraints(TargetChannel-1)) / (Constraints(TargetChannel) - Constraints(TargetChannel-1));
			if( SpeedRatio <= BlendDownPerc )
			{
				TargetChannel--;
			}
		}

		if( TargetChannel != LastChannel )
		{
			if( TargetChannel < LastChannel )
			{
				SetActiveChild( TargetChannel, BlendDownTime );
			}
			else
			{
				SetActiveChild( TargetChannel, BlendUpTime );
			}
			LastChannel = TargetChannel;
		}
	}
	else if( !SufficientChannels )
	{
		debugf(TEXT("UAnimNodeBlendBySpeed::TickAnim - Need at least two children"));
	}
	else if( !SufficientConstraints )
	{
		debugf(TEXT("UAnimNodeBlendBySpeed::TickAnim - Number of constraints (%i) is lower than number of children! (%i)"), Constraints.Num(), NumChannels);
	}
	
	Super::TickAnim(DeltaSeconds, TotalWeight);				
}

/** 
 *	Function called to calculate the speed that should be used for this node. 
 *	Allows subclasses to easily modify the speed used.
 */
FLOAT UAnimNodeBlendBySpeed::CalcSpeed()
{
	AActor* Owner = SkelComponent ? SkelComponent->GetOwner() : NULL;
	if(Owner)
	{
		if( bUseAcceleration )
		{
			return Owner->Acceleration.Size();
		}
		else
		{
			return Owner->Velocity.Size();
		}
	}
	else
	{
		return Speed;
	}
}

///////////////////////////////////////
////////// UAnimNodeMirror ////////////
///////////////////////////////////////


void UAnimNodeMirror::GetBoneAtoms(TArray<FBoneAtom>& Atoms, const TArray<BYTE>& DesiredBones, FBoneAtom& RootMotionDelta, INT& bHasRootMotion)
{
	START_GETBONEATOM_TIMER

	if( GetCachedResults(Atoms, RootMotionDelta, bHasRootMotion) )
	{
		return;
	}

	check(Children.Num() == 1);

	// If mirroring is enabled, and the mirror info array is initialized correctly.
	if( bEnableMirroring )
	{
		EXCLUDE_CHILD_TIME
		GetMirroredBoneAtoms(Atoms, 0, DesiredBones, RootMotionDelta, bHasRootMotion);
	}
	// If no mirroring is going on, just pass right through.
	else
	{
		EXCLUDE_CHILD_TIME
		if( Children(0).Anim )
		{
			Children(0).Anim->GetBoneAtoms(Atoms, DesiredBones, RootMotionDelta, bHasRootMotion);
		}
		else
		{
			RootMotionDelta = FBoneAtom::Identity;
			bHasRootMotion	= 0;
			FillWithRefPose(Atoms, DesiredBones, SkelComponent->SkeletalMesh->RefSkeleton);
		}
	}

	SaveCachedResults(Atoms, RootMotionDelta, bHasRootMotion);
}


///////////////////////////////////////
//////////// UAnimTree ////////////////
///////////////////////////////////////

void UAnimTree::InitAnim(USkeletalMeshComponent* meshComp, UAnimNodeBlendBase* Parent)
{
	Super::InitAnim(meshComp, Parent);

	if( meshComp )
	{
		// update list of bone priorities
		meshComp->BuildComposePriorityList(PriorityList);
	}

	// Rebuild cached list of nodes belonging to each group
	RepopulateAnimGroups();
}

void UAnimTree::PostEditChange(UProperty* PropertyThatChanged)
{
	if( PropertyThatChanged && (PropertyThatChanged->GetFName() == FName(TEXT("PrioritizedSkelBranches"))) )
	{
		if( SkelComponent )
		{
			SkelComponent->BuildComposePriorityList(PriorityList);
		}
	}

	if( PropertyThatChanged && 
		(	PropertyThatChanged->GetFName() == FName(TEXT("PreviewSkelMesh")) ||
			PropertyThatChanged->GetFName() == FName(TEXT("PreviewAnimSets"))
		) )
	{
		if( SkelComponent )
		{
			// SkeletalMesh changed, force InitAnim() to be called on all nodes.
			SkelComponent->InitAnimTree();
		}
	}

	Super::PostEditChange(PropertyThatChanged);
}


/** 
 * Get relative position of a synchronized node. 
 * Taking into account node's relative offset.
 */
static inline FLOAT GetNodeRelativePosition(UAnimNodeSequence* SeqNode)
{
	if( SeqNode && SeqNode->AnimSeq && SeqNode->AnimSeq->SequenceLength > 0.f )
	{
		// Find it's relative position on its time line.
		FLOAT RelativePosition = appFmod((SeqNode->CurrentTime / SeqNode->AnimSeq->SequenceLength) - SeqNode->SynchPosOffset, 1.f);
		if( RelativePosition < 0.f )
		{
			RelativePosition += 1.f;
		}

		return RelativePosition;
	}

	return 0.f;
}


/** 
 * Find out position of a synchronized node given a relative position. 
 * Taking into account node's relative offset.
 */
static inline FLOAT FindNodePositionFromRelative(UAnimNodeSequence* SeqNode, FLOAT RelativePosition)
{
	if( SeqNode && SeqNode->AnimSeq )
	{
		FLOAT SynchedRelPosition = appFmod(RelativePosition + SeqNode->SynchPosOffset, 1.f);
		if( SynchedRelPosition < 0.f )
		{
			SynchedRelPosition += 1.f;
		}

		return SynchedRelPosition * SeqNode->AnimSeq->SequenceLength;
	}

	return 0.f;
}


/** Look up AnimNodeSequences and cache Group lists */
void UAnimTree::RepopulateAnimGroups()
{
	if( Children(0).Anim )
	{
		// get all UAnimNodeSequence children and cache them to avoid over casting
		TArray<UAnimNodeSequence*> Nodes;
		Children(0).Anim->GetAnimSeqNodes(Nodes);

		for( INT GroupIdx=0; GroupIdx<AnimGroups.Num(); GroupIdx++ )
		{
			FAnimGroup& AnimGroup = AnimGroups(GroupIdx);

			// Clear cached nodes
			AnimGroup.SeqNodes.Empty();

			// Clear Master Nodes
			AnimGroup.SynchMaster	= NULL;
			AnimGroup.NotifyMaster	= NULL;

			// Caches nodes belonging to group
			if( AnimGroup.GroupName != NAME_None )
			{
				for( INT i=0; i<Nodes.Num(); i++ )
				{
					UAnimNodeSequence* SeqNode = Nodes(i);
					if( SeqNode->SynchGroupName == AnimGroup.GroupName )	// need to be from the same group name
					{
						AnimGroup.SeqNodes.AddItem(SeqNode);
					}
				}

				// Update Master Nodes
				UpdateMasterNodesForGroup(AnimGroup);
			}
		}
	}
}

/** Utility function, to check if a node is relevant for synchronization group. */
static inline UBOOL IsAnimNodeRelevantForSynchGroup(const UAnimNodeSequence* SeqNode)
{
	return SeqNode && SeqNode->AnimSeq && SeqNode->bSynchronize;
}

/** Utility function to check if a node is relevant for notification group. */
static inline UBOOL IsAnimNodeRelevantForNotifyGroup(const UAnimNodeSequence* SeqNode)
{
	return SeqNode && SeqNode->AnimSeq && !SeqNode->bNoNotifies;
}

/** 
 * Updates the Master Nodes of a given group.
 * Makes sure Synchronization and Notification masters are still relevant, and the highest weight of the group.
 */
void UAnimTree::UpdateMasterNodesForGroup(FAnimGroup& AnimGroup)
{
	// If we have a SynchMaster and it is not relevant anymore, clear it.
	if( AnimGroup.SynchMaster && (!AnimGroup.SynchMaster->bRelevant || !AnimGroup.SynchMaster->bForceAlwaysSlave || !IsAnimNodeRelevantForSynchGroup(AnimGroup.SynchMaster)) )
	{
		AnimGroup.SynchMaster = NULL;
	}

	// Again, make sure our notification master is still relevant. Otherwise, clear it.
	if( AnimGroup.NotifyMaster && (!AnimGroup.NotifyMaster->bRelevant || !IsAnimNodeRelevantForNotifyGroup(AnimGroup.NotifyMaster)) )
	{
		AnimGroup.NotifyMaster = NULL;
	}
	
	// If master nodes have full weight, then don't bother looking for new ones.
	if( (!AnimGroup.SynchMaster || (AnimGroup.SynchMaster->NodeTotalWeight < (1.f - ZERO_ANIMWEIGHT_THRESH))) ||
		(!AnimGroup.NotifyMaster || (AnimGroup.NotifyMaster->NodeTotalWeight < (1.f - ZERO_ANIMWEIGHT_THRESH))) )
	{
		// Find the node which has the highest weight... that's our master node
		FLOAT				SynchWeight		= AnimGroup.SynchMaster ? AnimGroup.SynchMaster->NodeTotalWeight : 0.f;
		FLOAT				NotifyWeight	= AnimGroup.NotifyMaster ? AnimGroup.NotifyMaster->NodeTotalWeight : 0.f;

		for(INT i=0; i<AnimGroup.SeqNodes.Num(); i++)
		{
			UAnimNodeSequence* Candidate = AnimGroup.SeqNodes(i);
			if( Candidate && Candidate->bRelevant )
			{
				// See if this node is a candidate for synchronization master
				if( IsAnimNodeRelevantForSynchGroup(Candidate) && !Candidate->bForceAlwaysSlave && Candidate->NodeTotalWeight > SynchWeight )
				{
					AnimGroup.SynchMaster	= Candidate;
					SynchWeight				= Candidate->NodeTotalWeight;
				}

				// See if this node is a candidate for notification master
				if( IsAnimNodeRelevantForNotifyGroup(Candidate) && Candidate->NodeTotalWeight > NotifyWeight )
				{
					AnimGroup.NotifyMaster	= Candidate;
					NotifyWeight			= Candidate->NodeTotalWeight;
				}
			}
		}
	}
}

/** The main synchronization code... */
void UAnimTree::UpdateAnimNodeSeqGroups(FLOAT DeltaSeconds)
{
	// Force continuous update if within the editor (because we can add or remove nodes)
	// This can be improved by only doing that when a node has been added a removed from the tree.
	if( GIsEditor && !GIsGame )
	{
		RepopulateAnimGroups();
	}

	// Go through all groups, and update all nodes.
	for(INT GroupIdx=0; GroupIdx<AnimGroups.Num(); GroupIdx++)
	{
		FAnimGroup& AnimGroup = AnimGroups(GroupIdx);

		// Update Master Nodes
		UpdateMasterNodesForGroup(AnimGroup);

		// Calculate GroupDelta here, as this is common to all nodes in this group.
		const FLOAT GroupDelta = AnimGroup.RateScale * SkelComponent->GlobalAnimRateScale * DeltaSeconds;

		// If we don't have a valid synchronization master for this group, update all nodes without synchronization.
		if( !IsAnimNodeRelevantForSynchGroup(AnimGroup.SynchMaster) || !AnimGroup.SynchMaster->bRelevant )
		{
			for(INT i=0; i<AnimGroup.SeqNodes.Num(); i++)
			{
				UAnimNodeSequence* SeqNode = AnimGroup.SeqNodes(i);
				if( SeqNode )
				{
					// Keep track of PreviousTime before any update. This is used by Root Motion.
					SeqNode->PreviousTime = SeqNode->CurrentTime;
					if( SeqNode->AnimSeq && SeqNode->bPlaying )
					{
						const FLOAT MoveDelta		= GroupDelta * SeqNode->Rate * SeqNode->AnimSeq->RateScale * SkelComponent->GlobalAnimRateScale;
						// Fire notifies if node is notification master
						const UBOOL	bFireNotifies	= (SeqNode == AnimGroup.NotifyMaster);

						// Advance animation.
						SeqNode->AdvanceBy(MoveDelta, DeltaSeconds, bFireNotifies);
					}
				}
			}
		}
		// Now that we have the master node, have it update all the others
		else 
		{
			UAnimNodeSequence* SynchMaster	= AnimGroup.SynchMaster;
			const FLOAT MasterMoveDelta		= GroupDelta * SynchMaster->Rate * SynchMaster->AnimSeq->RateScale * SkelComponent->GlobalAnimRateScale;

			// Keep track of PreviousTime before any update. This is used by Root Motion.
			SynchMaster->PreviousTime = SynchMaster->CurrentTime;

			if( SynchMaster->bPlaying )
			{
				// Fire notifies if node is notification master
				const UBOOL	bFireNotifies = (SynchMaster == AnimGroup.NotifyMaster);

				// Advance Synch Master Node
				SynchMaster->AdvanceBy(MasterMoveDelta, DeltaSeconds, bFireNotifies);
			}

			// SynchMaster node was changed during the tick?
			// Skip this round of updates...
			if( AnimGroup.SynchMaster != SynchMaster )
			{
				continue;
			}

			// Find it's relative position on its time line.
			const FLOAT MasterRelativePosition = GetNodeRelativePosition(SynchMaster);

			// Update slaves to match relative position of master node.
			for(INT i=0; i<AnimGroup.SeqNodes.Num(); i++)
			{
				UAnimNodeSequence* SeqNode = AnimGroup.SeqNodes(i);
				if( SeqNode && SeqNode != SynchMaster )
				{
					// Keep track of PreviousTime before any update. This is used by Root Motion.
					SeqNode->PreviousTime = SeqNode->CurrentTime;

					// If node should be synchronized
					if( IsAnimNodeRelevantForSynchGroup(SeqNode) && SeqNode->AnimSeq->SequenceLength > 0.f )
					{
						// Slave's new time
						const FLOAT NewTime		= FindNodePositionFromRelative(SeqNode, MasterRelativePosition);
						FLOAT SlaveMoveDelta	= appFmod(NewTime - SeqNode->CurrentTime, SeqNode->AnimSeq->SequenceLength);

						// Make sure SlaveMoveDelta And MasterMoveDelta are the same sign, so they both move in the same direction.
						if( SlaveMoveDelta * MasterMoveDelta < 0.f )
						{
							if( SlaveMoveDelta >= 0.f )
							{
								SlaveMoveDelta -= SeqNode->AnimSeq->SequenceLength;
							}
							else
							{
								SlaveMoveDelta += SeqNode->AnimSeq->SequenceLength;
							}
						}

						// Fire notifies if node is master of notification group
						const UBOOL	bFireNotifies = (SeqNode == AnimGroup.NotifyMaster);

						// Move slave node to correct position
						SeqNode->AdvanceBy(SlaveMoveDelta, DeltaSeconds, bFireNotifies);
					}
					// If node shouldn't be synchronized, update it normally
					else if( SeqNode->AnimSeq && SeqNode->bPlaying )
					{
						const FLOAT MoveDelta		= GroupDelta * SeqNode->Rate * SeqNode->AnimSeq->RateScale * SkelComponent->GlobalAnimRateScale;
						// Fire notifies if node is notification master
						const UBOOL	bFireNotifies	= (SeqNode == AnimGroup.NotifyMaster);

						// Advance animation.
						SeqNode->AdvanceBy(MoveDelta, DeltaSeconds, bFireNotifies);
					}
				}
			}
		}
	}
}

/** Add a node to an existing group */
UBOOL UAnimTree::SetAnimGroupForNode(class UAnimNodeSequence* SeqNode, FName GroupName, UBOOL bCreateIfNotFound)
{
	if( !SeqNode )
	{	
		return FALSE;
	}

	// If node is already in this group, then do nothing
	if( SeqNode->SynchGroupName == GroupName )
	{
		return TRUE;
	}

	// If node is already part of a group, remove it from there first.
	if( SeqNode->SynchGroupName != NAME_None )
	{
		const INT GroupIndex = GetGroupIndex(SeqNode->SynchGroupName);
		if( GroupIndex != INDEX_NONE )
		{
			FAnimGroup& AnimGroup	= AnimGroups(GroupIndex);
			SeqNode->SynchGroupName = NAME_None;
			AnimGroup.SeqNodes.RemoveItem(SeqNode);

			UBOOL bUpdateMasterNodes = FALSE;

			// If we're removing a Master Node, clear reference to it.
			if( AnimGroup.SynchMaster == SeqNode )
			{
				AnimGroup.SynchMaster	= NULL;
				bUpdateMasterNodes		= TRUE;
			}
			// If we're removing a Master Node, clear reference to it.
			if( AnimGroup.NotifyMaster == SeqNode )
			{
				AnimGroup.NotifyMaster	= NULL;
				bUpdateMasterNodes		= TRUE;
			}
			if( bUpdateMasterNodes )
			{
				UpdateMasterNodesForGroup(AnimGroup);
			}
		}
	}

	// See if we can move the node to the new group
	if( GroupName != NAME_None )
	{
		INT GroupIndex = GetGroupIndex(GroupName);
		
		// If group wasn't found, maybe we should create it
		if( GroupIndex == INDEX_NONE && bCreateIfNotFound )
		{
			GroupIndex = AnimGroups.AddZeroed();
			AnimGroups(GroupIndex).RateScale = 1.f;
			AnimGroups(GroupIndex).GroupName = GroupName;
		}

		if( GroupIndex != INDEX_NONE )
		{
			FAnimGroup& AnimGroup	= AnimGroups(GroupIndex);
			// Set group name
			SeqNode->SynchGroupName = GroupName;
			AnimGroup.SeqNodes.AddUniqueItem(SeqNode);

			// If node can be synchronized, then have it start at the current group position.
			// In case this node becomes the new SynchMaster, we don't want it to mess up the other nodes.
			if( IsAnimNodeRelevantForSynchGroup(SeqNode) )
			{
				SeqNode->SetPosition(FindNodePositionFromRelative(SeqNode, GetGroupRelativePosition(GroupName)), FALSE);
			}
		}
	}

	// return TRUE if change was successful
	return (SeqNode->SynchGroupName == GroupName);
}

/** Force a group at a relative position. */
void UAnimTree::ForceGroupRelativePosition(FName GroupName, FLOAT RelativePosition)
{
	for( INT GroupIdx=0; GroupIdx<AnimGroups.Num(); GroupIdx++ )
	{
		const FAnimGroup& AnimGroup = AnimGroups(GroupIdx);
		if( AnimGroup.GroupName == GroupName )
		{
			for( INT i=0; i<AnimGroup.SeqNodes.Num(); i++ )
			{
				UAnimNodeSequence* SeqNode = AnimGroup.SeqNodes(i);
				if( IsAnimNodeRelevantForSynchGroup(SeqNode) )
				{
					SeqNode->SetPosition(FindNodePositionFromRelative(SeqNode, RelativePosition), FALSE);
				}
			}
			break;
		}
	}
}

/** Get the relative position of a group. */
FLOAT UAnimTree::GetGroupRelativePosition(FName GroupName)
{
	const INT GroupIndex = GetGroupIndex(GroupName);
	if( GroupIndex != INDEX_NONE && AnimGroups(GroupIndex).SynchMaster )
	{
		return GetNodeRelativePosition(AnimGroups(GroupIndex).SynchMaster);
	}

	return 0.f;
}

/** Returns the master node driving synchronization for this group. */
UAnimNodeSequence* UAnimTree::GetGroupSynchMaster(FName GroupName)
{
	const INT GroupIndex = GetGroupIndex(GroupName);
	if( GroupIndex != INDEX_NONE )
	{
		return AnimGroups(GroupIndex).SynchMaster;
	}

	return NULL;
}

/** Returns the master node driving notifications for this group. */
UAnimNodeSequence* UAnimTree::GetGroupNotifyMaster(FName GroupName)
{
	const INT GroupIndex = GetGroupIndex(GroupName);
	if( GroupIndex != INDEX_NONE )
	{
		return AnimGroups(GroupIndex).NotifyMaster;
	}

	return NULL;
}

/** Adjust the Rate Scale of a group */
void UAnimTree::SetGroupRateScale(FName GroupName, FLOAT NewRateScale)
{
	const INT GroupIndex = GetGroupIndex(GroupName);
	if( GroupIndex != INDEX_NONE )
	{
		AnimGroups(GroupIndex).RateScale = NewRateScale;
	}
}

/** 
 * Returns the index in the AnimGroups list of a given GroupName.
 * If group cannot be found, then INDEX_NONE will be returned.
 */
INT UAnimTree::GetGroupIndex(FName GroupName)
{
	if( GroupName != NAME_None )
	{
		for(INT GroupIdx=0; GroupIdx<AnimGroups.Num(); GroupIdx++ )
		{
			if( AnimGroups(GroupIdx).GroupName == GroupName )
			{
				return GroupIdx;
			}
		}
	}

	return INDEX_NONE;
}

/** Get all SkelControls within this AnimTree. */
void UAnimTree::GetSkelControls(TArray<USkelControlBase*>& OutControls)
{
	OutControls.Empty();

	// Iterate over array of list-head structs.
	for(INT i=0; i<SkelControlLists.Num(); i++)
	{
		// Then walk down each list, adding the SkelControl if its not already in the array.
		USkelControlBase* Control = SkelControlLists(i).ControlHead;
		while(Control)
		{
			OutControls.AddUniqueItem(Control);
			Control = Control->NextControl;
		}
	}
}

/** Get all MorphNodes within this AnimTree. */
void UAnimTree::GetMorphNodes(TArray<UMorphNodeBase*>& OutNodes)
{
	// Firest empty the array we will put nodes into.
	OutNodes.Empty();

	// Iterate over each node connected to the root.
	for(INT i=0; i<RootMorphNodes.Num(); i++)
	{
		// If non-NULL, call GetNodes. This will add itself and any children.
		if( RootMorphNodes(i) )
		{
			RootMorphNodes(i)->GetNodes(OutNodes);
		}
	}
}

/** Calls GetActiveMorphs on each element of the RootMorphNodes array. */
void UAnimTree::GetTreeActiveMorphs(TArray<FActiveMorph>& OutMorphs)
{
	// Iterate over each node connected to the root.
	for(INT i=0; i<RootMorphNodes.Num(); i++)
	{
		// If non-NULL, call GetNodes. This will add itself and any children.
		if( RootMorphNodes(i) )
		{
			RootMorphNodes(i)->GetActiveMorphs(OutMorphs);
		}
	}
}

/** Call InitMorph on all morph nodes attached to the tree. */
void UAnimTree::InitTreeMorphNodes(USkeletalMeshComponent* InSkelComp)
{
	TArray<UMorphNodeBase*>	AllNodes;
	GetMorphNodes(AllNodes);

	for(INT i=0; i<AllNodes.Num(); i++)
	{
		if(AllNodes(i))
		{
			AllNodes(i)->InitMorphNode(InSkelComp);
		}
	}
}

/** Utility for find a SkelControl in an AnimTree by name. */
USkelControlBase* UAnimTree::FindSkelControl(FName InControlName)
{
	// Always return NULL if we did not pass in a name.
	if(InControlName == NAME_None)
	{
		return NULL;
	}

	// Iterate over array of list-head structs.
	for(INT i=0; i<SkelControlLists.Num(); i++)
	{
		// Then walk down each list, adding the SkelControl if its not already in the array.
		USkelControlBase* Control = SkelControlLists(i).ControlHead;
		while(Control)
		{
			if( Control->ControlName == InControlName )
			{
				return Control;
			}
			Control = Control->NextControl;
		}
	}

	return NULL;
}

/** Utility for find a MorphNode in an AnimTree by name. */
UMorphNodeBase* UAnimTree::FindMorphNode(FName InNodeName)
{
	// Always return NULL if we did not pass in a name.
	if(InNodeName == NAME_None)
	{
		return NULL;
	}

	TArray<UMorphNodeBase*>	MorphNodes;
	GetMorphNodes(MorphNodes);

	for(INT i=0; i<MorphNodes.Num(); i++)
	{
		if(MorphNodes(i)->NodeName == InNodeName)
		{
			return MorphNodes(i);
		}
	}

	return NULL;
}

void UAnimTree::CopyAnimNodes(const TArray<UAnimNode*>& SrcNodes, UObject* NewOuter, TArray<UAnimNode*>& DestNodes, TMap<UAnimNode*,UAnimNode*>& SrcToDestNodeMap)
{
	DWORD OldHackFlags = GUglyHackFlags;
	GUglyHackFlags |= HACK_DisableSubobjectInstancing; // Disable subobject instancing. Will not be needed when we can remove 'editinline export' from AnimTree pointers.

	// Duplicate each AnimNode in tree.
	for(INT i=0; i<SrcNodes.Num(); i++)
	{
		UAnimNode* NewNode = ConstructObject<UAnimNode>( SrcNodes(i)->GetClass(), NewOuter, NAME_None, 0, SrcNodes(i) );
		NewNode->SetArchetype( SrcNodes(i)->GetClass()->GetDefaultObject() );
		DestNodes.AddItem(NewNode);
		SrcToDestNodeMap.Set( SrcNodes(i), NewNode );
	}

	// Then fix up pointers.
	for(INT i=0; i<DestNodes.Num(); i++)
	{
		// Only UAnimNodeBlendBase classes have references to other AnimNodes.
		UAnimNodeBlendBase* NewBlend = Cast<UAnimNodeBlendBase>( DestNodes(i) );
		if(NewBlend)
		{
			for(INT j=0; j<NewBlend->Children.Num(); j++)
			{
				if(NewBlend->Children(j).Anim)
				{
					UAnimNode** NewNode = SrcToDestNodeMap.Find(NewBlend->Children(j).Anim);
					if(NewNode)
					{
						check(*NewNode);
						NewBlend->Children(j).Anim = *NewNode;
					}
				}
			}
		}

		DestNodes(i)->ParentNodes.Empty(); // Just in case...

		// Allow node to do work post-instance
		DestNodes(i)->PostAnimNodeInstance( SrcNodes(i) );
	}

	GUglyHackFlags = OldHackFlags;
}

void UAnimTree::CopySkelControls(const TArray<USkelControlBase*>& SrcControls, UObject* NewOuter, TArray<USkelControlBase*>& DestControls, TMap<USkelControlBase*,USkelControlBase*>& SrcToDestControlMap)
{
	DWORD OldHackFlags = GUglyHackFlags;
	GUglyHackFlags |= HACK_DisableSubobjectInstancing; // Disable subobject instancing. Will not be needed when we can remove 'editinline export' from AnimTree pointers.

	for(INT i=0; i<SrcControls.Num(); i++)
	{
		USkelControlBase* NewControl = ConstructObject<USkelControlBase>( SrcControls(i)->GetClass(), NewOuter, NAME_None, 0, SrcControls(i) );
		NewControl->SetArchetype( SrcControls(i)->GetClass()->GetDefaultObject() );
		DestControls.AddItem(NewControl);
		SrcToDestControlMap.Set( SrcControls(i), NewControl );
	}

	// Then we fix up 'NextControl' pointers.
	for(INT i=0; i<DestControls.Num(); i++)
	{
		if(DestControls(i)->NextControl)
		{
			USkelControlBase** NewControl = SrcToDestControlMap.Find(DestControls(i)->NextControl);
			if(NewControl)
			{
				check(*NewControl);
				DestControls(i)->NextControl = *NewControl;
			}
		}
	}

	GUglyHackFlags = OldHackFlags;
}

void UAnimTree::CopyMorphNodes(const TArray<class UMorphNodeBase*>& SrcNodes, UObject* NewOuter, TArray<class UMorphNodeBase*>& DestNodes, TMap<class UMorphNodeBase*,class UMorphNodeBase*>& SrcToDestNodeMap)
{
	DWORD OldHackFlags = GUglyHackFlags;
	GUglyHackFlags |= HACK_DisableSubobjectInstancing; // Disable subobject instancing. Will not be needed when we can remove 'editinline export' from AnimTree pointers.

	for(INT i=0; i<SrcNodes.Num(); i++)
	{
		UMorphNodeBase* NewNode = ConstructObject<UMorphNodeBase>( SrcNodes(i)->GetClass(), NewOuter, NAME_None, 0, SrcNodes(i) );
		NewNode->SetArchetype( SrcNodes(i)->GetClass()->GetDefaultObject() );
		DestNodes.AddItem(NewNode);
		SrcToDestNodeMap.Set( SrcNodes(i), NewNode );
	}

	// Then we fix up links in the NodeConns array.
	for(INT i=0; i<DestNodes.Num(); i++)
	{
		UMorphNodeWeightBase* WeightNode = Cast<UMorphNodeWeightBase>( DestNodes(i) );
		if(WeightNode)
		{
			// Iterate over each connector
			for(INT j=0; j<WeightNode->NodeConns.Num(); j++)
			{
				FMorphNodeConn& Conn = WeightNode->NodeConns(j);

				// Iterate over each link from this connector.
				for(INT k=0; k<Conn.ChildNodes.Num(); k++)
				{
					// If this is a pointer to another node, look it up in the SrcToDestNodeMap and replace the reference.
					if( Conn.ChildNodes(k) )
					{
						UMorphNodeBase** NewNode = SrcToDestNodeMap.Find( Conn.ChildNodes(k) );
						if(NewNode)
						{
							check(*NewNode);
							Conn.ChildNodes(k) = *NewNode;
						}
					}
				}
			}
		}
	}

	GUglyHackFlags = OldHackFlags;
}

UAnimTree* UAnimTree::CopyAnimTree(UObject* NewTreeOuter)
{
#if SHOW_COPYANIMTREE_TIMES
	DOUBLE Start = appSeconds();	
#endif

	// Construct the new AnimTree object.
	DWORD OldHackFlags = GUglyHackFlags;
	GUglyHackFlags |= HACK_DisableSubobjectInstancing; // Disable subobject instancing. Will not be needed when we can remove 'editinline export' from AnimTree pointers.
	UAnimTree* NewTree = ConstructObject<UAnimTree>( UAnimTree::StaticClass(), NewTreeOuter, NAME_None, 0, this );
	GUglyHackFlags = OldHackFlags;


	// Then get all the nodes in the source tree, excluding the tree itself (ie this object).
	TArray<UAnimNode*> SrcNodes;
	GetNodes(SrcNodes);
	verify(SrcNodes.RemoveItem(this) > 0);

	// Duplicate all the AnimNodes	
	TArray<UAnimNode*> DestNodes; // Array of newly created nodes.
	TMap<UAnimNode*,UAnimNode*> SrcToDestNodeMap; // Mapping table from src node to newly created node.
	CopyAnimNodes(SrcNodes, NewTree, DestNodes, SrcToDestNodeMap);


	// Now we get all the SkelControls in this tree
	TArray<USkelControlBase*> SrcControls;
	GetSkelControls(SrcControls);

	// Duplicate all the SkelControls
	TArray<USkelControlBase*> DestControls; // Array of new skel controls.
	TMap<USkelControlBase*, USkelControlBase*> SrcToDestControlMap; // Map from src control to newly created one.
	CopySkelControls(SrcControls, NewTree, DestControls, SrcToDestControlMap);

	// Now we get all the MorphNodes in this tree
	TArray<UMorphNodeBase*> SrcMorphNodes;
	GetMorphNodes(SrcMorphNodes);

	// Duplicate all the MorphNodes
	TArray<UMorphNodeBase*> DestMorphNodes; // Array of new morph nodes.
	TMap<UMorphNodeBase*, UMorphNodeBase*> SrcToDestMorphNodeMap; // Map from src node to newly created one.
	CopyMorphNodes(SrcMorphNodes, NewTree, DestMorphNodes, SrcToDestMorphNodeMap);

	// Finally we fix up references in the new AnimTree itself (root AnimNode, head of each SkelControl chain, and root MorphNodes).
	check(NewTree->Children.Num() == 1);
	if(NewTree->Children(0).Anim)
	{
		UAnimNode** NewNode = SrcToDestNodeMap.Find(NewTree->Children(0).Anim);
		check(NewNode && *NewNode); // When we copy the entire tree, we should always find the node we want.
		NewTree->Children(0).Anim = *NewNode;
	}

	for(INT i=0; i<NewTree->SkelControlLists.Num(); i++)
	{
		if(NewTree->SkelControlLists(i).ControlHead)
		{
			USkelControlBase** NewControl = SrcToDestControlMap.Find(NewTree->SkelControlLists(i).ControlHead);
			check(NewControl && *NewControl);
			NewTree->SkelControlLists(i).ControlHead = *NewControl;
		}
	}

	for(INT i=0; i<NewTree->RootMorphNodes.Num(); i++)
	{
		if( NewTree->RootMorphNodes(i) )
		{
			UMorphNodeBase** NewNode = SrcToDestMorphNodeMap.Find( NewTree->RootMorphNodes(i) );
			check(*NewNode);
			NewTree->RootMorphNodes(i) = *NewNode;
		}
	}

#if SHOW_COPYANIMTREE_TIMES
	debugf(TEXT("CopyAnimTree: %f ms"), (appSeconds() - Start) * 1000.f);
#endif

	return NewTree;
}


///////////////////////
// UAnimNodeBlendMultiBone
//////////////////////////

/** 
 * Calculates total weight of children. 
 * Set a full weight on source, because it's potentially always feeding animations into the final blend.
 */
void UAnimNodeBlendMultiBone::SetChildrenTotalWeightAccumulator(const INT Index, const FLOAT NodeTotalWeight)
{
	if( Index == 0 )
	{
		// Update the weight of this connection
		Children(Index).TotalWeight = NodeTotalWeight;
		// Update the accumulator to find out the combined weight of the child node
		Children(Index).Anim->TotalWeightAccumulator += NodeTotalWeight * 1.f;
	}
	else
	{
		// Update the weight of this connection
		Children(Index).TotalWeight = NodeTotalWeight * Children(Index).Weight;
		// Update the accumulator to find out the combined weight of the child node
		Children(Index).Anim->TotalWeightAccumulator += NodeTotalWeight * Children(Index).Weight;
	}
}


/** @see UAnimNode::InitAnim */
void UAnimNodeBlendMultiBone::InitAnim( USkeletalMeshComponent* meshComp, UAnimNodeBlendBase* Parent )
{
	Super::InitAnim(meshComp, Parent);

	for( INT Idx = 0; Idx < BlendTargetList.Num(); Idx++ )
	{
		if( BlendTargetList(Idx).InitTargetStartBone != NAME_None )
		{
			SetTargetStartBone( Idx, BlendTargetList(Idx).InitTargetStartBone, BlendTargetList(Idx).InitPerBoneIncrease );
		}
	}
}

void UAnimNodeBlendMultiBone::PostEditChange(UProperty* PropertyThatChanged)
{
	for( INT Idx = 0; Idx < BlendTargetList.Num(); Idx++ )
	{
		if( PropertyThatChanged && 
			(PropertyThatChanged->GetFName() == FName(TEXT("InitTargetStartBone")) || 
			 PropertyThatChanged->GetFName() == FName(TEXT("InitPerBoneIncrease"))) )
		{
			SetTargetStartBone( Idx, BlendTargetList(Idx).InitTargetStartBone, BlendTargetList(Idx).InitPerBoneIncrease );
		}
	}

	Super::PostEditChange(PropertyThatChanged);
}


/**
 * Utility for creating the Child2PerBoneWeight array. Walks down the heirarchy increasing the weight by PerBoneIncrease each step.
 * The per-bone weight is multiplied by the overall blend weight to calculate how much animation data to pull from each child.
 * 
 * @param StartBoneName Start blending in animation from Children(1) (ie second child) from this bone down.
 * @param PerBoneIncrease Amount to increase bone weight by for each bone down heirarchy.
 */
void UAnimNodeBlendMultiBone::SetTargetStartBone( INT TargetIdx, FName StartBoneName, FLOAT PerBoneIncrease )
{
	FChildBoneBlendInfo& Info = BlendTargetList(TargetIdx);

	if( !SkelComponent || 
		(Info.OldStartBone	  == StartBoneName && 
		 Info.OldBoneIncrease == PerBoneIncrease &&
		 Info.TargetRequiredBones.Num() > 0 &&
		 SourceRequiredBones.Num() > 0) )
	{
		return;
	}

	Info.OldBoneIncrease		= PerBoneIncrease;
	Info.InitPerBoneIncrease	= PerBoneIncrease;
	Info.OldStartBone			= StartBoneName;
	Info.InitTargetStartBone	= StartBoneName;

	if( StartBoneName == NAME_None )
	{
		Info.TargetPerBoneWeight.Empty();
	}
	else
	{
		INT StartBoneIndex = SkelComponent->MatchRefBone( StartBoneName );
		if( StartBoneIndex == INDEX_NONE )
		{
			debugf( TEXT("UAnimNodeBlendPerBone::SetTargetStartBone : StartBoneName (%s) not found."), *StartBoneName.ToString() );
			return;
		}

		TArray<FMeshBone>& RefSkel = SkelComponent->SkeletalMesh->RefSkeleton;
		Info.TargetRequiredBones.Empty();
		Info.TargetPerBoneWeight.Empty();
		Info.TargetPerBoneWeight.AddZeroed( RefSkel.Num() );
		SourceRequiredBones.Empty();

		check(PerBoneIncrease >= 0.0f && PerBoneIncrease <= 1.0f); // rather aggressive...

		// Allocate bone weights by walking heirarchy, increasing bone weight for bones below the start bone.
		Info.TargetPerBoneWeight(StartBoneIndex) = PerBoneIncrease;
		for( INT i = 0; i < Info.TargetPerBoneWeight.Num(); i++ )
		{
			if( i != StartBoneIndex )
			{
				FLOAT ParentWeight = Info.TargetPerBoneWeight( RefSkel(i).ParentIndex );
				FLOAT BoneWeight   = (ParentWeight == 0.0f) ? 0.0f : Min( ParentWeight + PerBoneIncrease, 1.0f );

				Info.TargetPerBoneWeight(i) = BoneWeight;
			}

			if( Info.TargetPerBoneWeight(i) > ZERO_ANIMWEIGHT_THRESH )
			{
				Info.TargetRequiredBones.AddItem(i);
			}
			else if( Info.TargetPerBoneWeight(i) <=(1.f - ZERO_ANIMWEIGHT_THRESH) )
			{
				SourceRequiredBones.AddItem( i );
			}
		}
	}
}

void UAnimNodeBlendMultiBone::execSetTargetStartBone( FFrame& Stack, RESULT_DECL )
{
	P_GET_INT(TargetIdx);
	P_GET_NAME(StartBoneName);
	P_GET_FLOAT_OPTX(PerBoneIncrease, 1.0f);
	P_FINISH;
	
	SetTargetStartBone( TargetIdx, StartBoneName, PerBoneIncrease );
}


/** @see UAnimNode::GetBoneAtoms. */
void UAnimNodeBlendMultiBone::GetBoneAtoms(TArray<FBoneAtom>& Atoms, const TArray<BYTE>& DesiredBones, FBoneAtom& RootMotionDelta, INT& bHasRootMotion)
{
	START_GETBONEATOM_TIMER

	if( GetCachedResults(Atoms, RootMotionDelta, bHasRootMotion) )
	{
		return;
	}

	// Handle case of a blend with no children.
	if( Children.Num() == 0 )
	{
		RootMotionDelta = FBoneAtom::Identity;
		bHasRootMotion	= 0;
		FillWithRefPose(Atoms, DesiredBones, SkelComponent->SkeletalMesh->RefSkeleton);
		return;
	}

	INT NumAtoms = SkelComponent->SkeletalMesh->RefSkeleton.Num();
	check( NumAtoms == Atoms.Num() );

	// Find index of the last child with a non-zero weight.
	INT LastChildIndex = INDEX_NONE;
	for(INT i=0; i<Children.Num(); i++)
	{
		if( Children(i).Weight > ZERO_ANIMWEIGHT_THRESH )
		{
			LastChildIndex = i;
		}
	}
	check(LastChildIndex != INDEX_NONE);

	// We don't allocate this array until we need it.
	TArray<FBoneAtom> ChildAtoms;
	if( LastChildIndex == 0 )
	{
		if( Children(0).Anim )
		{
			EXCLUDE_CHILD_TIME
			Children(0).Anim->GetBoneAtoms(Atoms, DesiredBones, RootMotionDelta, bHasRootMotion);
		}
		else
		{
			RootMotionDelta = FBoneAtom::Identity;
			bHasRootMotion	= 0;
			FillWithRefPose(Atoms, DesiredBones, SkelComponent->SkeletalMesh->RefSkeleton);
		}

		SaveCachedResults(Atoms, RootMotionDelta, bHasRootMotion);

		return;
	}

	for( INT j = 0; j < DesiredBones.Num(); j++ )
	{
		FLOAT AccWeight			= 0.f;
		UBOOL bNoChildrenYet	= TRUE;
		
		// Iterate over each child getting its atoms, scaling them and adding them to output (Atoms array)
		for( INT i = LastChildIndex; i >= 0; i-- )
		{
			if( Children(i).Weight > ZERO_ANIMWEIGHT_THRESH )
			{
				INT		BoneIndex	= DesiredBones(j);
				FLOAT	BoneWeight;
				if( i > 0 )
				{
					BoneWeight	= Children(i).Weight * BlendTargetList(i).TargetPerBoneWeight(BoneIndex);
				}
				else
				{
					BoneWeight = 1.f - AccWeight;
				}


				// Do need to request atoms, so allocate array here.
				if( ChildAtoms.Num() == 0 )
				{
					ChildAtoms.Add(NumAtoms);
				}

				// Get bone atoms from child node (if no child - use ref pose).
				if( Children(i).Anim )
				{
					EXCLUDE_CHILD_TIME
					Children(i).Anim->GetBoneAtoms(ChildAtoms, DesiredBones, Children(i).RootMotion, Children(i).bHasRootMotion);

					bHasRootMotion = bHasRootMotion || Children(i).bHasRootMotion;

					if( bNoChildrenYet )
					{
						RootMotionDelta = Children(i).RootMotion * Children(i).Weight;
					}
					else
					{
						RootMotionDelta += Children(i).RootMotion * Children(i).Weight;
					}
				}
				else
				{
					FillWithRefPose(ChildAtoms, DesiredBones, SkelComponent->SkeletalMesh->RefSkeleton);
				}

				// To ensure the 'shortest route', we make sure the dot product between the accumulator and the incoming child atom is positive.
				if( (Atoms(BoneIndex).Rotation | ChildAtoms(BoneIndex).Rotation) < 0.f )
				{
					ChildAtoms(BoneIndex).Rotation = ChildAtoms(BoneIndex).Rotation * -1.f;
				}

				// We just write the first childrens atoms into the output array. Avoids zero-ing it out.
				if( bNoChildrenYet )
				{
					Atoms(BoneIndex) = ChildAtoms(BoneIndex) * BoneWeight;
				}
				else
				{
					Atoms(BoneIndex) += ChildAtoms(BoneIndex) * BoneWeight;
				}

				// If last child - normalize the rotaion quaternion now.
				if( i == 0 )
				{
					Atoms(BoneIndex).Rotation.Normalize();
				}

				if( i > 0 )
				{
					AccWeight += BoneWeight;
				}
				
				bNoChildrenYet = FALSE;
			}
		}
	}

	SaveCachedResults(Atoms, RootMotionDelta, bHasRootMotion);
}


/************************************************************************************
 * UAnimNodeAimOffset
 ***********************************************************************************/

/** Used to save pointer to AimOffset node in package, to avoid duplicating profile data. */
void UAnimNodeAimOffset::PostAnimNodeInstance(UAnimNode* SourceNode)
{
	TemplateNode = CastChecked<UAnimNodeAimOffset>(SourceNode);

	// We are going to use data from the TemplateNode, rather than keeping a copy for each instance of the node.
	Profiles.Empty();
}

void UAnimNodeAimOffset::PostLoad()
{
	Super::PostLoad();

	// If a version before profiles - update here.
	if ( GetLinkerVersion() < VER_AIMOFFSET_PROFILES )
	{
		check(Profiles.Num() == 0);

		// Add new profile struct
		Profiles.AddZeroed();
		FAimOffsetProfile& NewProfile = Profiles(0);

		// Call if 'default'
		NewProfile.ProfileName = FName(TEXT("Default"));

		// Copy properties from node into this profile, and clear.
		NewProfile.HorizontalRange = HorizontalRange;
		HorizontalRange = FVector2D(0,0);

		NewProfile.VerticalRange = VerticalRange;
		VerticalRange = FVector2D(0,0);

		NewProfile.AimComponents = AimComponents;
		AimComponents.Empty();

		NewProfile.AnimName_LU = AnimName_LU;
		AnimName_LU = NAME_None;
		NewProfile.AnimName_LC = AnimName_LC;
		AnimName_LC = NAME_None;
		NewProfile.AnimName_LD = AnimName_LD;
		AnimName_LD = NAME_None;
		NewProfile.AnimName_CU = AnimName_CU;
		AnimName_CU = NAME_None;
		NewProfile.AnimName_CC = AnimName_CC;
		AnimName_CC = NAME_None;
		NewProfile.AnimName_CD = AnimName_CD;
		AnimName_CD = NAME_None;
		NewProfile.AnimName_RU = AnimName_RU;
		AnimName_RU = NAME_None;
		NewProfile.AnimName_RC = AnimName_RC;
		AnimName_RC = NAME_None;
		NewProfile.AnimName_RD = AnimName_RD;
		AnimName_RD = NAME_None;
	}

	// If a version before Quaterions, update here
	if( GetLinkerVersion() < VER_AIMOFFSET_ROT2QUAT )
	{
		// The old versioning system may have done the conversion already.
		// So in that case, don't do it.
		if( InstanceVersionNumber < 1 )
		{
			debugf(TEXT("Converting Rotators to Quaternions on NodeName: %s for SkelComp: %s, Outer: %s"), *NodeName.ToString(), SkelComponent ? *SkelComponent->GetOwner()->GetName() : TEXT("None"), *GetOuter()->GetName());

			INT OldProfileIndex = CurrentProfileIndex;

			for(INT PIndex = 0; PIndex < Profiles.Num(); PIndex++)
			{
				FAimOffsetProfile& P = Profiles(PIndex);

				// A bit unpleasant- but ConvertRotToQuat works on the 'current' profile, so we make each one in turn 'current'.
				CurrentProfileIndex = PIndex;

				for(INT i=0; i<P.AimComponents.Num(); i++)
				{
					ConvertRotToQuat(i, ANIMAIM_LEFTUP);
					ConvertRotToQuat(i, ANIMAIM_CENTERUP);
					ConvertRotToQuat(i, ANIMAIM_RIGHTUP);

					ConvertRotToQuat(i, ANIMAIM_LEFTCENTER);
					ConvertRotToQuat(i, ANIMAIM_CENTERCENTER);
					ConvertRotToQuat(i, ANIMAIM_RIGHTCENTER);

					ConvertRotToQuat(i, ANIMAIM_LEFTDOWN);
					ConvertRotToQuat(i, ANIMAIM_CENTERDOWN);
					ConvertRotToQuat(i, ANIMAIM_RIGHTDOWN);
				}
			}

			CurrentProfileIndex = OldProfileIndex;
		}

		// That system is outdated now...
		InstanceVersionNumber = 0;
	}
}

void UAnimNodeAimOffset::PostEditChange(UProperty* PropertyThatChanged)
{
	Super::PostEditChange(PropertyThatChanged);

	if( PropertyThatChanged->GetFName() == FName(TEXT("bBakeFromAnimations")) )
	{
		bBakeFromAnimations = FALSE;
		BakeOffsetsFromAnimations();
	}
}

void UAnimNodeAimOffset::InitAnim(USkeletalMeshComponent* meshComp, UAnimNodeBlendBase* Parent)
{
	Super::InitAnim(meshComp, Parent);

	// Update list of required bones
	// this is the list of bones needed for transformation and their parents.
	UpdateListOfRequiredBones();
}

/** Utility function to convert rotator to Quaternion */
void UAnimNodeAimOffset::ConvertRotToQuat(INT CompIndex, EAnimAimDir InAimDir)
{
	// Extract component rotation
	FRotator	Rotation =	GetBoneAimRotation(CompIndex, InAimDir);

	// Transform it to Quaternion and set it.
	SetBoneAimQuaternion(CompIndex, InAimDir, Rotation.Quaternion());

	// Clear rotation component
	SetBoneAimRotation(CompIndex, InAimDir, FRotator(0,0,0));
}


void UAnimNodeAimOffset::UpdateListOfRequiredBones()
{
	// Empty required bones list
	RequiredBones.Reset();
	BoneToAimCpnt.Reset();

	FAimOffsetProfile* P = GetCurrentProfile();
	if( !P || !SkelComponent || !SkelComponent->SkeletalMesh )
	{
		return;
	}

	const TArray<FMeshBone>&	RefSkel		= SkelComponent->SkeletalMesh->RefSkeleton;
	const INT					NumBones	= RefSkel.Num();

	// Size look up table
	BoneToAimCpnt.Add(NumBones);

	// Iterate through all bones in the skeleton and see if we have an AimComponent defined for these
	// We do this to update AimComponent bone indices
	// but also build the RequiredBones list in increasing order.
	for(INT BoneIndex=0; BoneIndex<NumBones; BoneIndex++)
	{
		// See if we can find an AimComponent for this bone..
		INT AimCompIndex = INDEX_NONE;
		for(INT i=0; i<P->AimComponents.Num(); i++)
		{
			if( P->AimComponents(i).BoneName == RefSkel(BoneIndex).Name )
			{
				AimCompIndex = i;
				break;
			}
		}

		// Update look up table to find AimComponent Index from BoneIndex.
		// AimComponents are based on Bone names, because similar skeletons can be exported in
		// different bone orders, so it's not safe to refer by Index. Only Bones Names.
		// So the look up table needs to be updated when node is instanced.
		BoneToAimCpnt(BoneIndex) = AimCompIndex;

		// Build RequiredBones array in increasing order
		if( AimCompIndex != INDEX_NONE )
		{
			RequiredBones.AddItem(BoneIndex);
		}
	}

	// Make sure parents are present in the array. Since we need to get the relevant bones in component space.
	// And that require the parent bones...
	UAnimNode::EnsureParentsPresent(RequiredBones, SkelComponent->SkeletalMesh);
}

static inline FLOAT UnWindNormalizedAimAngle(FLOAT Angle)
{
	while( Angle > 2.f )
	{
		Angle -= 4.f;
	}

	while( Angle < -2.f )
	{
		Angle += 4.f;
	}

	return Angle;
}

FAimOffsetProfile* UAnimNodeAimOffset::GetCurrentProfile()
{
	// Check profile index is not outside range.
	FAimOffsetProfile* P = NULL;
	if(TemplateNode)
	{
		if(CurrentProfileIndex < TemplateNode->Profiles.Num())
		{
			P = &TemplateNode->Profiles(CurrentProfileIndex);
		}
	}
	else
	{
		if(CurrentProfileIndex < Profiles.Num())
		{
			P = &Profiles(CurrentProfileIndex);
		}
	}
	return P;
}

/** Temporary working space used by AimOffset node. Grows to max size required by any node. */
static TArray<FMatrix> AimOffsetBoneTM;

void UAnimNodeAimOffset::GetBoneAtoms(TArray<FBoneAtom>& Atoms, const TArray<BYTE>& DesiredBones, FBoneAtom& RootMotionDelta, INT& bHasRootMotion)
{
	START_GETBONEATOM_TIMER

	if( GetCachedResults(Atoms, RootMotionDelta, bHasRootMotion) )
	{
		return;
	}

	// Get local space atoms from child
	if( Children(0).Anim )
	{
		EXCLUDE_CHILD_TIME
		Children(0).Anim->GetBoneAtoms(Atoms, DesiredBones, RootMotionDelta, bHasRootMotion);
	}
	else
	{
		RootMotionDelta = FBoneAtom::Identity;
		bHasRootMotion	= 0;
		FillWithRefPose(Atoms, DesiredBones, SkelComponent->SkeletalMesh->RefSkeleton);
	}

	// Have the option of doing nothing if at a low LOD.
	if( SkelComponent->PredictedLODLevel >= PassThroughAtOrAboveLOD )
	{
		return;
	}

	const USkeletalMesh*	SkelMesh = SkelComponent->SkeletalMesh;
	const INT				NumBones = SkelMesh->RefSkeleton.Num();

	// Make sure we have a valid setup
	FAimOffsetProfile* P = GetCurrentProfile();
	if( !SkelComponent || RequiredBones.Num() == 0 || BoneToAimCpnt.Num() != NumBones || !P )
	{
		return;
	}

	FVector2D SafeAim = GetAim();
	
	// Add in rotation offset, but not in the editor
	if( !GIsEditor || GIsGame )
	{
		if( AngleOffset.X != 0.f )
		{
			SafeAim.X = UnWindNormalizedAimAngle(SafeAim.X - AngleOffset.X);
		}
		if( AngleOffset.Y != 0.f )
		{
			SafeAim.Y = UnWindNormalizedAimAngle(SafeAim.Y - AngleOffset.Y);
		}
	}

	// Scale by range
	if( P->HorizontalRange.X != 0.f && SafeAim.X < 0.f )
	{
		SafeAim.X = SafeAim.X / Abs(P->HorizontalRange.X);
	}
	if( P->HorizontalRange.Y != 0.f && SafeAim.X > 0.f )
	{
		SafeAim.X = SafeAim.X / P->HorizontalRange.Y;
	}
	if( P->VerticalRange.X != 0.f && SafeAim.Y < 0.f )
	{
		SafeAim.Y = SafeAim.Y / Abs(P->VerticalRange.X);
	}
	if( P->VerticalRange.Y != 0.f && SafeAim.Y > 0.f )
	{
		SafeAim.Y = SafeAim.Y / P->VerticalRange.Y;
	}

	// Make sure we're using correct values within legal range.
	SafeAim.X = Clamp<FLOAT>(SafeAim.X, -1.f, +1.f);
	SafeAim.Y = Clamp<FLOAT>(SafeAim.Y, -1.f, +1.f);

	// Post process final Aim.
	PostAimProcessing(SafeAim);

	// We build the mesh-space matrices of the source bones.
	AimOffsetBoneTM.Reset();
	AimOffsetBoneTM.Add(NumBones);

	TArray<BYTE> IntersectedList;
	IntersectByteArrays(IntersectedList, RequiredBones, DesiredBones);

	const INT	NumRelevantBones	= IntersectedList.Num();
	const INT	NumAimComp			= P->AimComponents.Num();

	for(INT i=0; i<NumRelevantBones; i++)
	{
		const INT BoneIndex = IntersectedList(i);

		// transform required bones into component space
		Atoms(BoneIndex).ToTransform(AimOffsetBoneTM(BoneIndex));
		if( BoneIndex > 0 )
		{
			AimOffsetBoneTM(BoneIndex) *= AimOffsetBoneTM(SkelMesh->RefSkeleton(BoneIndex).ParentIndex);
		}

		// See if this bone should be transformed. ie there is an AimComponent defined for it
		const INT AimCompIndex = BoneToAimCpnt(BoneIndex);
		if( AimCompIndex != INDEX_NONE )
		{
					FQuat			QuaternionOffset;
					FVector			TranslationOffset;
			const	FAimComponent&	AimCpnt = P->AimComponents(AimCompIndex);

			// If bForceAimDir - just use whatever ForcedAimDir is set to - ignore Aim.
			if( bForceAimDir )
			{
				switch( ForcedAimDir )
				{
					case ANIMAIM_LEFTUP			:	QuaternionOffset	= AimCpnt.LU.Quaternion; 
													TranslationOffset	= AimCpnt.LU.Translation;	
													break;
					case ANIMAIM_CENTERUP		:	QuaternionOffset	= AimCpnt.CU.Quaternion; 
													TranslationOffset	= AimCpnt.CU.Translation;	
													break;
					case ANIMAIM_RIGHTUP		:	QuaternionOffset	= AimCpnt.RU.Quaternion; 
													TranslationOffset	= AimCpnt.RU.Translation; 
													break;
					case ANIMAIM_LEFTCENTER		:	QuaternionOffset	= AimCpnt.LC.Quaternion; 
													TranslationOffset	= AimCpnt.LC.Translation; 
													break;
					case ANIMAIM_CENTERCENTER	:	QuaternionOffset	= AimCpnt.CC.Quaternion; 
													TranslationOffset	= AimCpnt.CC.Translation; 
													break;
					case ANIMAIM_RIGHTCENTER	:	QuaternionOffset	= AimCpnt.RC.Quaternion; 
													TranslationOffset	= AimCpnt.RC.Translation; 
													break;
					case ANIMAIM_LEFTDOWN		:	QuaternionOffset	= AimCpnt.LD.Quaternion; 
													TranslationOffset	= AimCpnt.LD.Translation; 
													break;
					case ANIMAIM_CENTERDOWN		:	QuaternionOffset	= AimCpnt.CD.Quaternion; 
													TranslationOffset	= AimCpnt.CD.Translation; 
													break;
					case ANIMAIM_RIGHTDOWN		:	QuaternionOffset	= AimCpnt.RD.Quaternion; 
													TranslationOffset	= AimCpnt.RD.Translation; 
													break;
				}
			}
			else
			{
				if( SafeAim.X >= 0.f && SafeAim.Y >= 0.f ) // Up Right
				{
					QuaternionOffset	= BiLerp(AimCpnt.CC.Quaternion, AimCpnt.RC.Quaternion, AimCpnt.CU.Quaternion, AimCpnt.RU.Quaternion, SafeAim.X, SafeAim.Y);
					TranslationOffset	= BiLerp(AimCpnt.CC.Translation, AimCpnt.RC.Translation, AimCpnt.CU.Translation, AimCpnt.RU.Translation, SafeAim.X, SafeAim.Y);
				}
				else if( SafeAim.X >= 0.f && SafeAim.Y < 0.f ) // Bottom Right
				{
					QuaternionOffset	= BiLerp(AimCpnt.CD.Quaternion, AimCpnt.RD.Quaternion, AimCpnt.CC.Quaternion, AimCpnt.RC.Quaternion, SafeAim.X, SafeAim.Y+1.f);
					TranslationOffset	= BiLerp(AimCpnt.CD.Translation, AimCpnt.RD.Translation, AimCpnt.CC.Translation, AimCpnt.RC.Translation, SafeAim.X, SafeAim.Y+1.f);
				}
				else if( SafeAim.X < 0.f && SafeAim.Y >= 0.f ) // Up Left
				{
					QuaternionOffset	= BiLerp(AimCpnt.LC.Quaternion, AimCpnt.CC.Quaternion, AimCpnt.LU.Quaternion, AimCpnt.CU.Quaternion, SafeAim.X+1.f, SafeAim.Y);
					TranslationOffset	= BiLerp(AimCpnt.LC.Translation, AimCpnt.CC.Translation, AimCpnt.LU.Translation, AimCpnt.CU.Translation, SafeAim.X+1.f, SafeAim.Y);
				}
				else if( SafeAim.X < 0.f && SafeAim.Y < 0.f ) // Bottom Left
				{
					QuaternionOffset	= BiLerp(AimCpnt.LD.Quaternion, AimCpnt.CD.Quaternion, AimCpnt.LC.Quaternion, AimCpnt.CC.Quaternion, SafeAim.X+1.f, SafeAim.Y+1.f);
					TranslationOffset	= BiLerp(AimCpnt.LD.Translation, AimCpnt.CD.Translation, AimCpnt.LC.Translation, AimCpnt.CC.Translation, SafeAim.X+1.f, SafeAim.Y+1.f);
				}
			}

			// only perform a transformation if it is significant
			// (Since it's something expensive to do)

			FMatrix		QuaterionMatrix		= FQuatRotationTranslationMatrix(QuaternionOffset,FVector(0.f));
			FRotator	RotationOffset		= QuaterionMatrix.Rotator();
			const UBOOL	bDoTranslation		= !TranslationOffset.IsNearlyZero();
			const UBOOL	bDoRotation			= !RotationOffset.IsZero();
			const UBOOL bDoTransformation	= bDoTranslation || bDoRotation;
			
			if( bDoTransformation )
			{
				// Find bone translation
				const FVector BoneOrigin = bDoTranslation ? (AimOffsetBoneTM(BoneIndex) * FTranslationMatrix(TranslationOffset)).GetOrigin() : AimOffsetBoneTM(BoneIndex).GetOrigin();

				// Apply bone rotation
				if( bDoRotation )
				{
					AimOffsetBoneTM(BoneIndex) *= QuaterionMatrix;
				}

				// Apply bone translation
				AimOffsetBoneTM(BoneIndex).SetOrigin(BoneOrigin);

				// Transform back to parent bone space
				FMatrix RelTM;
				if( BoneIndex == 0 )
				{
					RelTM = AimOffsetBoneTM(BoneIndex);
				}
				else
				{
					const INT ParentIndex = SkelMesh->RefSkeleton(BoneIndex).ParentIndex;
					RelTM = AimOffsetBoneTM(BoneIndex) * AimOffsetBoneTM(ParentIndex).Inverse();
				}

				const FBoneAtom	TransformedAtom(RelTM);
				Atoms(BoneIndex).Rotation		= TransformedAtom.Rotation;
				Atoms(BoneIndex).Translation	= TransformedAtom.Translation;
			}
		}
	}

	SaveCachedResults(Atoms, RootMotionDelta, bHasRootMotion);
}


FLOAT UAnimNodeAimOffset::GetSliderPosition(INT SliderIndex, INT ValueIndex)
{
	check(SliderIndex == 0);
	check(ValueIndex == 0 || ValueIndex == 1);

	if( ValueIndex == 0 )
	{
		return (0.5f * Aim.X) + 0.5f;
	}
	else
	{
		return ((-0.5f * Aim.Y) + 0.5f);
	}
}

void UAnimNodeAimOffset::HandleSliderMove(INT SliderIndex, INT ValueIndex, FLOAT NewSliderValue)
{
	check(SliderIndex == 0);
	check(ValueIndex == 0 || ValueIndex == 1);

	if( ValueIndex == 0 )
	{
		Aim.X = (NewSliderValue - 0.5f) * 2.f;
	}
	else
	{
		Aim.Y = -1.f * (NewSliderValue - 0.5f) * 2.f;
	}
}

FString UAnimNodeAimOffset::GetSliderDrawValue(INT SliderIndex)
{
	check(SliderIndex == 0);
	return FString::Printf( TEXT("%0.2f,%0.2f"), Aim.X, Aim.Y );
}


/** Util for finding the rotation that corresponds to a particular direction. */
static FRotator* GetAimRotPtr(FAimOffsetProfile* P, INT CompIndex, EAnimAimDir InAimDir)
{
	if( CompIndex < 0 || CompIndex >= P->AimComponents.Num() )
	{
		return NULL;
	}

	FRotator		*RotPtr		= NULL;
	FAimComponent	&AimCpnt	= P->AimComponents(CompIndex);

	switch( InAimDir )
	{
		case ANIMAIM_LEFTUP			: RotPtr = &(AimCpnt.LU.Rotation); break;
		case ANIMAIM_CENTERUP		: RotPtr = &(AimCpnt.CU.Rotation); break;
		case ANIMAIM_RIGHTUP		: RotPtr = &(AimCpnt.RU.Rotation); break;

		case ANIMAIM_LEFTCENTER		: RotPtr = &(AimCpnt.LC.Rotation); break;
		case ANIMAIM_CENTERCENTER	: RotPtr = &(AimCpnt.CC.Rotation); break;
		case ANIMAIM_RIGHTCENTER	: RotPtr = &(AimCpnt.RC.Rotation); break;

		case ANIMAIM_LEFTDOWN		: RotPtr = &(AimCpnt.LD.Rotation); break;
		case ANIMAIM_CENTERDOWN		: RotPtr = &(AimCpnt.CD.Rotation); break;
		case ANIMAIM_RIGHTDOWN		: RotPtr = &(AimCpnt.RD.Rotation); break;
	}

	return RotPtr;
}

/** Util for finding the quaternion that corresponds to a particular direction. */
static FQuat* GetAimQuatPtr(FAimOffsetProfile* P, INT CompIndex, EAnimAimDir InAimDir)
{
	if( CompIndex < 0 || CompIndex >= P->AimComponents.Num() )
	{
		return NULL;
	}

	FQuat			*QuatPtr	= NULL;
	FAimComponent	&AimCpnt	= P->AimComponents(CompIndex);

	switch( InAimDir )
	{
		case ANIMAIM_LEFTUP			: QuatPtr = &(AimCpnt.LU.Quaternion); break;
		case ANIMAIM_CENTERUP		: QuatPtr = &(AimCpnt.CU.Quaternion); break;
		case ANIMAIM_RIGHTUP		: QuatPtr = &(AimCpnt.RU.Quaternion); break;

		case ANIMAIM_LEFTCENTER		: QuatPtr = &(AimCpnt.LC.Quaternion); break;
		case ANIMAIM_CENTERCENTER	: QuatPtr = &(AimCpnt.CC.Quaternion); break;
		case ANIMAIM_RIGHTCENTER	: QuatPtr = &(AimCpnt.RC.Quaternion); break;

		case ANIMAIM_LEFTDOWN		: QuatPtr = &(AimCpnt.LD.Quaternion); break;
		case ANIMAIM_CENTERDOWN		: QuatPtr = &(AimCpnt.CD.Quaternion); break;
		case ANIMAIM_RIGHTDOWN		: QuatPtr = &(AimCpnt.RD.Quaternion); break;
	}

	return QuatPtr;
}


/** Util for finding the translation that corresponds to a particular direction. */
static FVector* GetAimTransPtr(FAimOffsetProfile* P, INT CompIndex, EAnimAimDir InAimDir)
{
	if( CompIndex < 0 || CompIndex >= P->AimComponents.Num() )
	{
		return NULL;
	}

	FVector			*TransPtr	= NULL;
	FAimComponent	&AimCpnt	= P->AimComponents(CompIndex);

	switch( InAimDir )
	{
		case ANIMAIM_LEFTUP			: TransPtr = &(AimCpnt.LU.Translation); break;
		case ANIMAIM_CENTERUP		: TransPtr = &(AimCpnt.CU.Translation); break;
		case ANIMAIM_RIGHTUP		: TransPtr = &(AimCpnt.RU.Translation); break;

		case ANIMAIM_LEFTCENTER		: TransPtr = &(AimCpnt.LC.Translation); break;
		case ANIMAIM_CENTERCENTER	: TransPtr = &(AimCpnt.CC.Translation); break;
		case ANIMAIM_RIGHTCENTER	: TransPtr = &(AimCpnt.RC.Translation); break;

		case ANIMAIM_LEFTDOWN		: TransPtr = &(AimCpnt.LD.Translation); break;
		case ANIMAIM_CENTERDOWN		: TransPtr = &(AimCpnt.CD.Translation); break;
		case ANIMAIM_RIGHTDOWN		: TransPtr = &(AimCpnt.RD.Translation); break;
	}

	return TransPtr;
}


/** Util for grabbing the rotation on a specific bone in a specific direction. */
FRotator UAnimNodeAimOffset::GetBoneAimRotation(INT CompIndex, EAnimAimDir InAimDir)
{
	FAimOffsetProfile* P = GetCurrentProfile();
	if(!P)
	{
		return FRotator(0,0,0);
	}

	// Get the array for this pose.
	FRotator *RotPtr = GetAimRotPtr(P, CompIndex, InAimDir);

	// And return the Rotator (if its in range).
	if( RotPtr )
	{
		return (*RotPtr);
	}
	else
	{
		return FRotator(0,0,0);
	}
}

/** Util for grabbing the quaternion on a specific bone in a specific direction. */
FQuat UAnimNodeAimOffset::GetBoneAimQuaternion(INT CompIndex, EAnimAimDir InAimDir)
{
	FAimOffsetProfile* P = GetCurrentProfile();
	if(!P)
	{
		return FQuat::Identity;
	}

	// Get the array for this pose.
	FQuat *QuatPtr = GetAimQuatPtr(P, CompIndex, InAimDir);

	// And return the Quaternion (if its in range).
	if( QuatPtr )
	{
		return (*QuatPtr);
	}
	else
	{
		return FQuat::Identity;
	}
}


/** Util for grabbing the translation on a specific bone in a specific direction. */
FVector UAnimNodeAimOffset::GetBoneAimTranslation(INT CompIndex, EAnimAimDir InAimDir)
{
	FAimOffsetProfile* P = GetCurrentProfile();
	if(!P)
	{
		return FVector(0,0,0);
	}

	// Get the translation for this pose.
	FVector *TransPtr = GetAimTransPtr(P, CompIndex, InAimDir);

	// And return the Rotator (if its in range).
	if( TransPtr )
	{
		return (*TransPtr);
	}
	else
	{
		return FVector(0,0,0);
	}
}

/** Util for setting the rotation on a specific bone in a specific direction. */
void UAnimNodeAimOffset::SetBoneAimRotation(INT CompIndex, EAnimAimDir InAimDir, FRotator InRot)
{
	FAimOffsetProfile* P = GetCurrentProfile();
	if(!P)
	{
		return;
	}

	// Get the array for this pose.
	FRotator *RotPtr = GetAimRotPtr(P, CompIndex, InAimDir);

	// Set the Rotator (if BoneIndex is in range).
	if( RotPtr )
	{
		(*RotPtr) = InRot;
	}
}

/** Util for setting the quaternion on a specific bone in a specific direction. */
void UAnimNodeAimOffset::SetBoneAimQuaternion(INT CompIndex, EAnimAimDir InAimDir, FQuat InQuat)
{
	FAimOffsetProfile* P = GetCurrentProfile();
	if(!P)
	{
		return;
	}

	// Get the array for this pose.
	FQuat *QuatPtr = GetAimQuatPtr(P, CompIndex, InAimDir);

	// Set the Rotator (if BoneIndex is in range).
	if( QuatPtr )
	{
		(*QuatPtr) = InQuat;
	}
}


/** Util for setting the translation on a specific bone in a specific direction. */
void UAnimNodeAimOffset::SetBoneAimTranslation(INT CompIndex, EAnimAimDir InAimDir, FVector InTrans)
{
	FAimOffsetProfile* P = GetCurrentProfile();
	if(!P)
	{
		return;
	}

	// Get the array for this pose.
	FVector *TransPtr = GetAimTransPtr(P, CompIndex, InAimDir);

	// Set the Rotator (if BoneIndex is in range).
	if( TransPtr )
	{
		(*TransPtr) = InTrans;
	}
}

/** Returns TRUE if AimComponents contains specified bone */
UBOOL UAnimNodeAimOffset::ContainsBone(const FName &BoneName)
{
	FAimOffsetProfile* P = GetCurrentProfile();
	if(!P)
	{
		return FALSE;
	}

	for( INT i=0; i<P->AimComponents.Num(); i++ )
	{
		if( P->AimComponents(i).BoneName == BoneName )
		{
			return TRUE;
		}
	}
	return FALSE;
}


/** Bake in Offsets from supplied Animations. */
void UAnimNodeAimOffset::BakeOffsetsFromAnimations()
{
	if( !SkelComponent || !SkelComponent->SkeletalMesh )
	{
		appMsgf(AMT_OK, TEXT(" No SkeletalMesh to import animations from. Aborting."));
		return;
	}

	// Check profile index is not outside range.
	FAimOffsetProfile* P = GetCurrentProfile();
	if(!P)
	{
		return;
	}

	// Clear current setup
	P->AimComponents.Empty();
	BoneToAimCpnt.Empty();

	// AnimNodeSequence used to extract animation data.
	UAnimNodeSequence*	SeqNode = ConstructObject<UAnimNodeSequence>(UAnimNodeSequence::StaticClass());
	SeqNode->SkelComponent = SkelComponent;

	// Extract Center/Center (reference pose)
	TArray<FBoneAtom>	BoneAtoms_CC;
	if( ExtractAnimationData(SeqNode, P->AnimName_CC, BoneAtoms_CC) == FALSE )
	{
		appMsgf(AMT_OK, TEXT(" Couldn't get CenterCenter pose, this is necessary. Aborting."));
		return;;
	}

	TArray<FBoneAtom>	BoneAtoms;

	// Extract Left/Up
	if( ExtractAnimationData(SeqNode, P->AnimName_LU, BoneAtoms) == TRUE )
	{
		debugf(TEXT(" Converting Animation: %s"), *P->AnimName_LU.ToString());
		ExtractOffsets(BoneAtoms_CC, BoneAtoms, ANIMAIM_LEFTUP);
	}

	// Extract Left/Center
	if( ExtractAnimationData(SeqNode, P->AnimName_LC, BoneAtoms) == TRUE )
	{
		debugf(TEXT(" Converting Animation: %s"), *P->AnimName_LC.ToString());
		ExtractOffsets(BoneAtoms_CC, BoneAtoms, ANIMAIM_LEFTCENTER);
	}

	// Extract Left/Down
	if( ExtractAnimationData(SeqNode, P->AnimName_LD, BoneAtoms) == TRUE )
	{
		debugf(TEXT(" Converting Animation: %s"), *P->AnimName_LD.ToString());
		ExtractOffsets(BoneAtoms_CC, BoneAtoms, ANIMAIM_LEFTDOWN);
	}

	// Extract Center/Up
	if( ExtractAnimationData(SeqNode, P->AnimName_CU, BoneAtoms) == TRUE )
	{
		debugf(TEXT(" Converting Animation: %s"), *P->AnimName_CU.ToString());
		ExtractOffsets(BoneAtoms_CC, BoneAtoms, ANIMAIM_CENTERUP);
	}

	// Extract Center/Down
	if( ExtractAnimationData(SeqNode, P->AnimName_CD, BoneAtoms) == TRUE )
	{
		debugf(TEXT(" Converting Animation: %s"), *P->AnimName_CD.ToString());
		ExtractOffsets(BoneAtoms_CC, BoneAtoms, ANIMAIM_CENTERDOWN);
	}

	// Extract Right/Up
	if( ExtractAnimationData(SeqNode, P->AnimName_RU, BoneAtoms) == TRUE )
	{
		debugf(TEXT(" Converting Animation: %s"), *P->AnimName_RU.ToString());
		ExtractOffsets(BoneAtoms_CC, BoneAtoms, ANIMAIM_RIGHTUP);
	}

	// Extract Right/Center
	if( ExtractAnimationData(SeqNode, P->AnimName_RC, BoneAtoms) == TRUE )
	{
		debugf(TEXT(" Converting Animation: %s"), *P->AnimName_RC.ToString());
		ExtractOffsets(BoneAtoms_CC, BoneAtoms, ANIMAIM_RIGHTCENTER);
	}

	// Extract Right/Down
	if( ExtractAnimationData(SeqNode, P->AnimName_RD, BoneAtoms) == TRUE )
	{
		debugf(TEXT(" Converting Animation: %s"), *P->AnimName_RD.ToString());
		ExtractOffsets(BoneAtoms_CC, BoneAtoms, ANIMAIM_RIGHTDOWN);
	}

	// Done, cache required bones
	UpdateListOfRequiredBones();

	// Clean up.
	SeqNode->SkelComponent	= NULL;
	SeqNode					= NULL;

	appMsgf(AMT_OK, TEXT(" Export finished, check log for details."));
}


void UAnimNodeAimOffset::ExtractOffsets(TArray<FBoneAtom>& RefBoneAtoms, TArray<FBoneAtom>& BoneAtoms, EAnimAimDir InAimDir)
{
	TArray<FMatrix>	TargetTM;
	TargetTM.Add(BoneAtoms.Num());

	for(INT i=0; i<BoneAtoms.Num(); i++)
	{
		// Transform target pose into mesh space
		BoneAtoms(i).ToTransform(TargetTM(i));
		if( i > 0 )
		{
			TargetTM(i) *= TargetTM(SkelComponent->SkeletalMesh->RefSkeleton(i).ParentIndex);
		}

		// Now get Source transform on this bone
		// But from Target parent bone in mesh space.
		FMatrix SourceTM;
		RefBoneAtoms(i).ToTransform(SourceTM);
		if( i > 0 )
		{
			// Transform Target Skeleton by reference transform for this bone
			SourceTM *= TargetTM(SkelComponent->SkeletalMesh->RefSkeleton(i).ParentIndex);
		}

		FMatrix TargetTranslationM = FMatrix::Identity;
		TargetTranslationM.SetOrigin( TargetTM(i).GetOrigin() );

		FMatrix SourceTranslationM = FMatrix::Identity;
		SourceTranslationM.SetOrigin( SourceTM.GetOrigin() );
		
		const FMatrix	TranslationTM		= SourceTranslationM.Inverse() * TargetTranslationM;
		const FVector	TranslationOffset	= TranslationTM.GetOrigin();

		if( !TranslationOffset.IsNearlyZero() )
		{
			const INT CompIdx = GetComponentIdxFromBoneIdx(i, TRUE);

			if( CompIdx != INDEX_NONE )
			{
				SetBoneAimTranslation(CompIdx, InAimDir, TranslationOffset);
			}
		}

		FMatrix TargetRotationM = TargetTM(i);
		TargetRotationM.RemoveScaling();
		TargetRotationM.SetOrigin(FVector(0.f));

		FMatrix SourceRotationM = SourceTM;
		SourceRotationM.RemoveScaling();
		SourceRotationM.SetOrigin(FVector(0.f));

		// Convert delta rotation to FRotator.
		const FMatrix	RotationTM		= SourceRotationM.Inverse() * TargetRotationM;
		const FRotator	RotationOffset	= RotationTM.Rotator();
		const FQuat		QuaterionOffset(RotationTM); 

		if( !RotationOffset.IsZero() )
		{
			const INT CompIdx = GetComponentIdxFromBoneIdx(i, TRUE);

			if( CompIdx != INDEX_NONE )
			{
				SetBoneAimQuaternion(CompIdx, InAimDir, QuaterionOffset);
			}
		}

	}
}


INT UAnimNodeAimOffset::GetComponentIdxFromBoneIdx(const INT BoneIndex, UBOOL bCreateIfNotFound)
{
	if( BoneIndex == INDEX_NONE )
	{
		return INDEX_NONE;
	}

	FAimOffsetProfile* P = GetCurrentProfile();
	if( !P )
	{
		return INDEX_NONE;
	}

	// If AimComponent exists, return it's index from the look up table
	if( BoneIndex < BoneToAimCpnt.Num() && BoneToAimCpnt(BoneIndex) != INDEX_NONE )
	{ 
		return BoneToAimCpnt(BoneIndex);
	}

	if( bCreateIfNotFound )
	{
		const FName	BoneName = SkelComponent->SkeletalMesh->RefSkeleton(BoneIndex).Name;

		// If its a valid bone we want to add..
		if( BoneName != NAME_None && BoneIndex != INDEX_NONE )
		{
			INT InsertPos = INDEX_NONE;

			// Iterate through current array, to find place to insert this new Bone so they stay in Bone index order.
			for(INT i=0; i<P->AimComponents.Num() && InsertPos == INDEX_NONE; i++)
			{
				const FName	TestName	= P->AimComponents(i).BoneName;
				const INT	TestIndex	= SkelComponent->SkeletalMesh->MatchRefBone(TestName);

				if( TestIndex != INDEX_NONE && TestIndex > BoneIndex )
				{
					InsertPos = i;
				}
			}

			// If we didn't find insert position - insert at end.
			// This also handles case of and empty BoneNames array.
			if( InsertPos == INDEX_NONE )
			{
				InsertPos = P->AimComponents.Num();
			}

			// Add a new component.
			P->AimComponents.InsertZeroed(InsertPos);

			// Set correct name and index.
			P->AimComponents(InsertPos).BoneName = BoneName;

			// Initialize Quaternions - InsertZeroed doesn't set them to Identity
			SetBoneAimQuaternion(InsertPos, ANIMAIM_LEFTUP,			FQuat::Identity);
			SetBoneAimQuaternion(InsertPos, ANIMAIM_CENTERUP,		FQuat::Identity);
			SetBoneAimQuaternion(InsertPos, ANIMAIM_RIGHTUP,		FQuat::Identity);

			SetBoneAimQuaternion(InsertPos, ANIMAIM_LEFTCENTER,		FQuat::Identity);
			SetBoneAimQuaternion(InsertPos, ANIMAIM_CENTERCENTER,	FQuat::Identity);
			SetBoneAimQuaternion(InsertPos, ANIMAIM_RIGHTCENTER,	FQuat::Identity);

			SetBoneAimQuaternion(InsertPos, ANIMAIM_LEFTDOWN,		FQuat::Identity);
			SetBoneAimQuaternion(InsertPos, ANIMAIM_CENTERDOWN,		FQuat::Identity);
			SetBoneAimQuaternion(InsertPos, ANIMAIM_RIGHTDOWN,		FQuat::Identity);

			// Update RequiredBoneIndex, this will update the LookUp Table
			UpdateListOfRequiredBones();

			return InsertPos;
		}
	}

	return INDEX_NONE;
}


/** 
 * Extract Parent Space Bone Atoms from Animation Data specified by Name. 
 * Returns TRUE if successful.
 */
UBOOL UAnimNodeAimOffset::ExtractAnimationData(UAnimNodeSequence *SeqNode, FName AnimationName, TArray<FBoneAtom>& BoneAtoms)
{
	//Set Animation
	SeqNode->SetAnim(AnimationName);

	if( SeqNode->AnimSeq == NULL )
	{
		debugf(TEXT(" ExtractAnimationData: Animation not found: %s, Skipping..."), *AnimationName.ToString());
		return FALSE;
	}

	const USkeletalMesh*	SkelMesh = SkelComponent->SkeletalMesh;
	const INT				NumBones = SkelMesh->RefSkeleton.Num();

	// initialize Bone Atoms array
	if( BoneAtoms.Num() != NumBones )
	{
		BoneAtoms.Empty();
		BoneAtoms.Add(NumBones);
	}

	// Initialize Desired bones array. We take all.
	TArray<BYTE> DesiredBones;
	DesiredBones.Empty();
	DesiredBones.Add(NumBones);
	for(INT i=0; i<DesiredBones.Num(); i++)
	{
		DesiredBones(i) = i;
	}

	// Extract bone atoms from animation data
	FBoneAtom	RootMotionDelta;
	INT			bHasRootMotion;
	SeqNode->GetBoneAtoms(BoneAtoms, DesiredBones, RootMotionDelta, bHasRootMotion);

	return TRUE;
}

/** 
 *	Change the currently active profile to the one with the supplied name. 
 *	If a profile with that name does not exist, this does nothing.
 */
void UAnimNodeAimOffset::SetActiveProfileByName(FName ProfileName)
{
	if(TemplateNode)
	{
		// Look through profiles to find a name that matches.
		for(INT i=0; i<TemplateNode->Profiles.Num(); i++)
		{
			// Found it - change to it.
			if(TemplateNode->Profiles(i).ProfileName == ProfileName)
			{
				SetActiveProfileByIndex(i);
				return;
			}
		}
	}
	else
	{
		// Look through profiles to find a name that matches.
		for(INT i=0; i<Profiles.Num(); i++)
		{
			// Found it - change to it.
			if(Profiles(i).ProfileName == ProfileName)
			{
				SetActiveProfileByIndex(i);
				return;
			}
		}
	}
}

/** 
 *	Change the currently active profile to the one with the supplied index. 
 *	If ProfileIndex is outside range, this does nothing.
 */
void UAnimNodeAimOffset::SetActiveProfileByIndex(INT ProfileIndex)
{
	// Check new index is in range.
	// Don't update if selecting the current profile again.
	if( TemplateNode )
	{
		if( ProfileIndex == CurrentProfileIndex || ProfileIndex < 0 || ProfileIndex >= TemplateNode->Profiles.Num() )
		{
			return;
		}
	}
	else
	{
		if( ProfileIndex == CurrentProfileIndex || ProfileIndex < 0 || ProfileIndex >= Profiles.Num() )
		{
			return;
		}
	}

	// Update profile index
	CurrentProfileIndex = ProfileIndex;

	// We need to recalculate the bone indices modified by the new profile.
	UpdateListOfRequiredBones();
}


/************************************************************************************
 * UAnimNodeBlendByAim
 ***********************************************************************************/

void UAnimNodeBlendByAim::TickAnim(FLOAT DeltaSeconds, FLOAT TotalWeight)
{
	// if not relevant enough for final blend, pass right through.
	if( bInitialized && !bRelevant )
	{
		Super::TickAnim(DeltaSeconds, TotalWeight);
		return;
	}

	if( !bInitialized || (GIsEditor && !GIsGame) )
	{
		if( SeqNode1 != Children(0).Anim && Children(0).Anim )
		{
			SeqNode1 = Cast<UAnimNodeSequence>(Children(0).Anim);
		}

		if( SeqNode2 != Children(1).Anim && Children(1).Anim )
		{
			SeqNode2 = Cast<UAnimNodeSequence>(Children(1).Anim);
		}

		if( SeqNode3 != Children(2).Anim && Children(2).Anim )
		{
			SeqNode3 = Cast<UAnimNodeSequence>(Children(2).Anim);
		}

		if( SeqNode4 != Children(3).Anim && Children(3).Anim )
		{
			SeqNode4 = Cast<UAnimNodeSequence>(Children(3).Anim);
		}

		// require at least 4 animnode sequence connected to work...
		if( !SeqNode1 || !SeqNode2 || !SeqNode3 || !SeqNode4 )
		{
			Super::TickAnim(DeltaSeconds, TotalWeight);
			return;
		}
	}

	// Get Normalized Aim.
	FVector2D SafeAim = GetAim();

	// Add in rotation offset, but not in the editor
	if( !GIsEditor || GIsGame )
	{
		if( AngleOffset.X != 0.f )
		{
			SafeAim.X = UnWindNormalizedAimAngle(SafeAim.X - AngleOffset.X);
		}
		if( AngleOffset.Y != 0.f )
		{
			SafeAim.Y = UnWindNormalizedAimAngle(SafeAim.Y - AngleOffset.Y);
		}
	}

	// Scale by range
	if( HorizontalRange.X != 0.f && SafeAim.X < 0.f )
	{
		SafeAim.X = SafeAim.X / Abs(HorizontalRange.X);
	}
	if( HorizontalRange.Y != 0.f && SafeAim.X > 0.f )
	{
		SafeAim.X = SafeAim.X / HorizontalRange.Y;
	}

	if( VerticalRange.X != 0.f && SafeAim.Y < 0.f )
	{
		SafeAim.Y = SafeAim.Y / Abs(VerticalRange.X);
	}
	if( VerticalRange.Y != 0.f && SafeAim.Y > 0.f )
	{
		SafeAim.Y = SafeAim.Y / VerticalRange.Y;
	}

	// Make sure we're using correct values within legal range.
	SafeAim.X = Clamp<FLOAT>(SafeAim.X, -1.f, +1.f);
	SafeAim.Y = Clamp<FLOAT>(SafeAim.Y, -1.f, +1.f);

	// did aim changed since previous frame?
	const UBOOL	bAimIsIdentical	=	Abs(SafeAim.X - PreviousAim.X) < KINDA_SMALL_NUMBER && 
									Abs(SafeAim.Y - PreviousAim.Y) < KINDA_SMALL_NUMBER;

	// Test if aim changed significantly since last time. If not, don't bother updating weights.
	if( bInitialized && bAimIsIdentical )
	{
		Super::TickAnim(DeltaSeconds, TotalWeight);
		return;
	}

	bInitialized	= TRUE;
	PreviousAim		= SafeAim;

	if( SafeAim.X >= 0.f && SafeAim.Y >= 0.f ) // Up Right
	{
		if( SeqNode1->AnimSeqName != AnimName_CC )
		{
			SeqNode1->SetAnim(AnimName_CC);
			SeqNode2->SetAnim(AnimName_RC);
			SeqNode3->SetAnim(AnimName_CU);
			SeqNode4->SetAnim(AnimName_RU);
		}
	}
	else if( SafeAim.X >= 0.f && SafeAim.Y < 0.f ) // Bottom Right
	{
		if( SeqNode1->AnimSeqName != AnimName_CD )
		{
			SeqNode1->SetAnim(AnimName_CD);
			SeqNode2->SetAnim(AnimName_RD);
			SeqNode3->SetAnim(AnimName_CC);
			SeqNode4->SetAnim(AnimName_RC);
		}

		SafeAim.Y += 1.f;
	}
	else if( SafeAim.X < 0.f && SafeAim.Y >= 0.f ) // Up Left
	{
		if( SeqNode1->AnimSeqName != AnimName_LC )
		{
			SeqNode1->SetAnim(AnimName_LC);
			SeqNode2->SetAnim(AnimName_CC);
			SeqNode3->SetAnim(AnimName_LU);
			SeqNode4->SetAnim(AnimName_CU);
		}

		SafeAim.X += 1.f;
	}
	else if( SafeAim.X < 0.f && SafeAim.Y < 0.f ) // Bottom Left
	{
		if( SeqNode1->AnimSeqName != AnimName_LD )
		{
			SeqNode1->SetAnim(AnimName_LD);
			SeqNode2->SetAnim(AnimName_CD);
			SeqNode3->SetAnim(AnimName_LC);
			SeqNode4->SetAnim(AnimName_CC);
		}

		SafeAim.X += 1.f;
		SafeAim.Y += 1.f;
	}

	Children(0).Weight = BiLerp(1.f, 0.f, 0.f, 0.f, SafeAim.X, SafeAim.Y);
	Children(1).Weight = BiLerp(0.f, 1.f, 0.f, 0.f, SafeAim.X, SafeAim.Y);
	Children(2).Weight = BiLerp(0.f, 0.f, 1.f, 0.f, SafeAim.X, SafeAim.Y);
	Children(3).Weight = BiLerp(0.f, 0.f, 0.f, 1.f, SafeAim.X, SafeAim.Y);

	Super::TickAnim(DeltaSeconds, TotalWeight);
}


FLOAT UAnimNodeBlendByAim::GetSliderPosition(INT SliderIndex, INT ValueIndex)
{
	check(SliderIndex == 0);
	check(ValueIndex == 0 || ValueIndex == 1);

	if( ValueIndex == 0 )
	{
		return (0.5f * Aim.X) + 0.5f;
	}
	else
	{
		return ((-0.5f * Aim.Y) + 0.5f);
	}
}

void UAnimNodeBlendByAim::HandleSliderMove(INT SliderIndex, INT ValueIndex, FLOAT NewSliderValue)
{
	check(SliderIndex == 0);
	check(ValueIndex == 0 || ValueIndex == 1);

	if( ValueIndex == 0 )
	{
		Aim.X = (NewSliderValue - 0.5f) * 2.f;
	}
	else
	{
		Aim.Y = -1.f * (NewSliderValue - 0.5f) * 2.f;
	}
}

FString UAnimNodeBlendByAim::GetSliderDrawValue(INT SliderIndex)
{
	check(SliderIndex == 0);
	return FString::Printf(TEXT("%0.2f,%0.2f"), Aim.X, Aim.Y);
}


/************************************************************************************
 * UAnimNodeSynch
 ***********************************************************************************/

void UAnimNodeSynch::PostLoad()
{
	Super::PostLoad();

	// If a version before VER_ANIMNODESYNCH_RATESCALE, set RateScale to default value
	// Because StructDefaultProperties don't propagate through DynamicArrays
	if ( GetLinkerVersion() < VER_ANIMNODESYNCH_RATESCALE )
	{
		for( INT GroupIdx=0; GroupIdx<Groups.Num(); GroupIdx++ )
		{
			FSynchGroup& SynchGroup = Groups(GroupIdx);
			SynchGroup.RateScale = 1.f;
		}
	}
}


/** Add a node to an existing group */
void UAnimNodeSynch::AddNodeToGroup(class UAnimNodeSequence* SeqNode, FName GroupName)
{
	if( !SeqNode || GroupName == NAME_None )
	{
		return;
	}

	for( INT GroupIdx=0; GroupIdx<Groups.Num(); GroupIdx++ )
	{
		FSynchGroup& SynchGroup = Groups(GroupIdx);
		if( SynchGroup.GroupName == GroupName )
		{
			// Set group name
			SeqNode->SynchGroupName = GroupName;
			SynchGroup.SeqNodes.AddUniqueItem(SeqNode);

			break;
		}
	}
}


/** Remove a node from an existing group */
void UAnimNodeSynch::RemoveNodeFromGroup(class UAnimNodeSequence* SeqNode, FName GroupName)
{
	if( !SeqNode || GroupName == NAME_None )
	{
		return;
	}

	for( INT GroupIdx=0; GroupIdx<Groups.Num(); GroupIdx++ )
	{
		FSynchGroup& SynchGroup = Groups(GroupIdx);
		if( SynchGroup.GroupName == GroupName )
		{
			SeqNode->SynchGroupName = NAME_None;
			SynchGroup.SeqNodes.RemoveItem(SeqNode);

			// If we're removing the Master Node, clear reference to it.
			if( SynchGroup.MasterNode == SeqNode )
			{
				SynchGroup.MasterNode = NULL;
				UpdateMasterNodeForGroup(SynchGroup);
			}

			break;
		}
	}
}


/** Force a group at a relative position. */
void UAnimNodeSynch::ForceRelativePosition(FName GroupName, FLOAT RelativePosition)
{
	for( INT GroupIdx=0; GroupIdx<Groups.Num(); GroupIdx++ )
	{
		FSynchGroup& SynchGroup = Groups(GroupIdx);
		if( SynchGroup.GroupName == GroupName )
		{
			for( INT i=0; i < SynchGroup.SeqNodes.Num(); i++ )
			{
				UAnimNodeSequence* SeqNode = SynchGroup.SeqNodes(i);
				if( SeqNode && SeqNode->AnimSeq )
				{
					SeqNode->SetPosition(FindNodePositionFromRelative(SeqNode, RelativePosition), FALSE);
				}
			}
		}
	}
}


/** Get the relative position of a group. */
FLOAT UAnimNodeSynch::GetRelativePosition(FName GroupName)
{
	for( INT GroupIdx=0; GroupIdx<Groups.Num(); GroupIdx++ )
	{
		FSynchGroup& SynchGroup = Groups(GroupIdx);
		if( SynchGroup.GroupName == GroupName )
		{
			if( SynchGroup.MasterNode )
			{
				return GetNodeRelativePosition(SynchGroup.MasterNode);
			}
			else
			{
				debugf(TEXT("UAnimNodeSynch::GetRelativePosition, no master node for group %s"), *SynchGroup.GroupName.ToString());
			}
		}
	}

	return 0.f;
}

/** Accesses the Master Node driving a given group */
UAnimNodeSequence* UAnimNodeSynch::GetMasterNodeOfGroup(FName GroupName)
{
	for(INT GroupIdx=0; GroupIdx<Groups.Num(); GroupIdx++ )
	{
		if( Groups(GroupIdx).GroupName == GroupName )
		{
			return Groups(GroupIdx).MasterNode;
		}
	}

	return NULL;
}

/** Force a group at a relative position. */
void UAnimNodeSynch::SetGroupRateScale(FName GroupName, FLOAT NewRateScale)
{
	for(INT GroupIdx=0; GroupIdx<Groups.Num(); GroupIdx++ )
	{
		if( Groups(GroupIdx).GroupName == GroupName )
		{
			Groups(GroupIdx).RateScale = NewRateScale;
		}
	}
}

void UAnimNodeSynch::InitAnim(USkeletalMeshComponent* MeshComp, UAnimNodeBlendBase* Parent)
{
	// Rebuild cached list of nodes belonging to each group
	RepopulateGroups();

	Super::InitAnim(MeshComp, Parent);
}

/** Look up AnimNodeSequences and cache Group lists */
void UAnimNodeSynch::RepopulateGroups()
{
	if( Children(0).Anim )
	{
		// get all UAnimNodeSequence children and cache them to avoid over casting
		TArray<UAnimNodeSequence*> Nodes;
		Children(0).Anim->GetAnimSeqNodes(Nodes);

		for( INT GroupIdx=0; GroupIdx<Groups.Num(); GroupIdx++ )
		{
			FSynchGroup& SynchGroup = Groups(GroupIdx);

			// Clear cached nodes
			SynchGroup.SeqNodes.Empty();

			// Caches nodes belonging to group
			if( SynchGroup.GroupName != NAME_None )
			{
				for( INT i=0; i < Nodes.Num(); i++ )
				{
					UAnimNodeSequence* SeqNode = Nodes(i);
					if( SeqNode->SynchGroupName == SynchGroup.GroupName )	// need to be from the same group name
					{
						SynchGroup.SeqNodes.AddItem(SeqNode);
					}
				}
			}

			// Clear Master Node
			SynchGroup.MasterNode = NULL;

			// Update Master Node
			UpdateMasterNodeForGroup(SynchGroup);
		}
	}
}

/** Updates the Master Node of a given group */
void UAnimNodeSynch::UpdateMasterNodeForGroup(FSynchGroup& SynchGroup)
{
	// start with our previous node. see if we can find better!
	UAnimNodeSequence*	MasterNode		= SynchGroup.MasterNode;	
	// Find the node which has the highest weight... that's our master node
	FLOAT				HighestWeight	= MasterNode ? MasterNode->NodeTotalWeight : 0.f;

		// if the previous master node has a full weight, then don't bother looking for another one.
	if( !SynchGroup.MasterNode || (SynchGroup.MasterNode->NodeTotalWeight < (1.f - ZERO_ANIMWEIGHT_THRESH)) )
	{
		for(INT i=0; i < SynchGroup.SeqNodes.Num(); i++)
		{
			UAnimNodeSequence* SeqNode = SynchGroup.SeqNodes(i);
			if( SeqNode &&									
				!SeqNode->bForceAlwaysSlave && 
				SeqNode->NodeTotalWeight >= HighestWeight )  // Must be the most relevant to the final blend
			{
				MasterNode		= SeqNode;
				HighestWeight	= SeqNode->NodeTotalWeight;
			}
		}

		SynchGroup.MasterNode = MasterNode;
	}
}

/** The main synchronization code... */
void UAnimNodeSynch::TickAnim(FLOAT DeltaSeconds, FLOAT TotalWeight)
{
	// perform tick... This updates the weight of all the childs
	// And ticks all sequence players including these controlled by this node.
	// A sequence player belonging to a group will be ticked, but its CurrentTime position
	// will NOT be updated. The Synchronization node will take care of that.
	Super::TickAnim(DeltaSeconds, TotalWeight);

	// Force continuous update if within the editor (because we can add or remove nodes)
	// This can be improved by only doing that when a node has been added a removed from the tree.
	if( GIsEditor && !GIsGame )
	{
		RepopulateGroups();
	}

	// Update groups
	for(INT GroupIdx=0; GroupIdx<Groups.Num(); GroupIdx++)
	{
		FSynchGroup& SynchGroup = Groups(GroupIdx);

		// Update Master Node
		UpdateMasterNodeForGroup(SynchGroup);

		// Now that we have the master node, have it update all the others
		if( SynchGroup.MasterNode && SynchGroup.MasterNode->AnimSeq )
		{
			UAnimNodeSequence* MasterNode	= SynchGroup.MasterNode;
			const FLOAT	OldPosition			= MasterNode->CurrentTime;
			const FLOAT MasterMoveDelta		= SynchGroup.RateScale * MasterNode->Rate * MasterNode->AnimSeq->RateScale * DeltaSeconds;

			if( MasterNode->bPlaying == TRUE )
			{
				// Keep track of PreviousTime before any update. This is used by Root Motion.
				MasterNode->PreviousTime = MasterNode->CurrentTime;

				// Update Master Node
				// Master Node has already been ticked, so now we advance its CurrentTime position...
				// Time to move forward by - DeltaSeconds scaled by various factors.
				MasterNode->AdvanceBy(MasterMoveDelta, DeltaSeconds, TRUE);
			}

			// Master node was changed during the tick?
			// Skip this round of updates...
			if( SynchGroup.MasterNode != MasterNode )
			{
				continue;
			}

			// Update slave nodes only if master node has changed.
			if( MasterNode->CurrentTime != OldPosition &&
				MasterNode->AnimSeq &&
				MasterNode->AnimSeq->SequenceLength > 0.f )
			{
				// Find it's relative position on its time line.
				const FLOAT MasterRelativePosition = GetNodeRelativePosition(MasterNode);

				// Update slaves to match relative position of master node.
				for(INT i=0; i<SynchGroup.SeqNodes.Num(); i++)
				{
					UAnimNodeSequence *SlaveNode = SynchGroup.SeqNodes(i);

					if( SlaveNode && 
						SlaveNode != MasterNode && 
						SlaveNode->AnimSeq &&
						SlaveNode->AnimSeq->SequenceLength > 0.f )
					{
						// Slave's new time
						const FLOAT NewTime		= FindNodePositionFromRelative(SlaveNode, MasterRelativePosition);
						FLOAT SlaveMoveDelta	= appFmod(NewTime - SlaveNode->CurrentTime, SlaveNode->AnimSeq->SequenceLength);

						// Make sure SlaveMoveDelta And MasterMoveDelta are the same sign, so they both move in the same direction.
						if( SlaveMoveDelta * MasterMoveDelta < 0.f )
						{
							if( SlaveMoveDelta >= 0.f )
							{
								SlaveMoveDelta -= SlaveNode->AnimSeq->SequenceLength;
							}
							else
							{
								SlaveMoveDelta += SlaveNode->AnimSeq->SequenceLength;
							}
						}

						// Keep track of PreviousTime before any update. This is used by Root Motion.
						SlaveNode->PreviousTime = SlaveNode->CurrentTime;

						// Move slave node to correct position
						SlaveNode->AdvanceBy(SlaveMoveDelta, DeltaSeconds, SynchGroup.bFireSlaveNotifies);
					}
				}
			}
		}
	}
}


/************************************************************************************
 * UAnimNodeRandom
 ***********************************************************************************/

#define DEBUG_ANIMNODERANDOM 0

void UAnimNodeRandom::InitAnim(USkeletalMeshComponent* meshComp, UAnimNodeBlendBase* Parent)
{
#if DEBUG_ANIMNODERANDOM
	debugf(TEXT("%3.2f UAnimNodeRandom::InitAnim"), GWorld->GetTimeSeconds());
#endif

	Super::InitAnim(meshComp, Parent);	

	// Verify that Info array is in synch with child number.
	if( RandomInfo.Num() != Children.Num() )
	{
		const INT Diff = Children.Num() - RandomInfo.Num();
		if( Diff > 0 )
		{
			RandomInfo.AddZeroed(Diff);
		}
		else
		{
			RandomInfo.Remove(RandomInfo.Num() + Diff, -Diff);
		}
	}

	// Only trigger animation is none is playing currently.
	// We don't want to override an animation that is currently playing.
	UBOOL bTriggerAnim = ActiveChildIndex < 0 || ActiveChildIndex >= Children.Num() || !Children(ActiveChildIndex).Anim;

	if( !bTriggerAnim )
	{
		bTriggerAnim = !PlayingSeqNode || !PlayingSeqNode->bPlaying || !PlayingSeqNode->IsChildOf(Children(ActiveChildIndex).Anim);
	}

	if( bTriggerAnim )
	{
		PlayPendingAnimation(0.f);
	}
}

/** A child has been added, update RandomInfo accordingly */
void UAnimNodeRandom::OnAddChild(INT ChildNum)
{
	Super::OnAddChild(ChildNum);

	// Update RandomInfo to match Children array
	if( ChildNum > 0 )
	{
		if( ChildNum < RandomInfo.Num() )
		{
			RandomInfo.InsertZeroed(ChildNum, 1);
		}
		else
		{
			RandomInfo.AddZeroed(1);
		}

		// Set up new addition w/ defaults
		FRandomAnimInfo& Info = RandomInfo(ChildNum);
		Info.Chance			= 1.f;
		Info.BlendInTime	= 0.25f;
		Info.PlayRateRange	= FVector2D(1.f,1.f);
	}
}


/** A child has been removed, update RandomInfo accordingly */
void UAnimNodeRandom::OnRemoveChild(INT ChildNum)
{
	Super::OnRemoveChild(ChildNum);

	if( ChildNum < RandomInfo.Num() )
	{
		// Update Mask to match Children array
		RandomInfo.Remove(ChildNum);
	}
}

void UAnimNodeRandom::OnChildAnimEnd(UAnimNodeSequence* Child, FLOAT PlayedTime, FLOAT ExcessTime)
{
#if DEBUG_ANIMNODERANDOM
	debugf(TEXT("%3.2f UAnimNodeRandom::OnChildAnimEnd %s"), GWorld->GetTimeSeconds(), *Child->AnimSeqName.ToString());
#endif

	Super::OnChildAnimEnd(Child, PlayedTime, ExcessTime);

	// Node playing current animation is done playing? For a loop certainly... So transition to pending animation!
	if( Child && Child == PlayingSeqNode )
	{
		PlayPendingAnimation(0.f, ExcessTime);
	}
}

INT UAnimNodeRandom::PickNextAnimIndex()
{
#if DEBUG_ANIMNODERANDOM
	debugf(TEXT("%3.2f UAnimNodeRandom::PickNextAnimIndex"), GWorld->GetTimeSeconds());
#endif

	if( !Children.Num() )
	{
		return INDEX_NONE;
	}

	// If active child is in the list
	if( PlayingSeqNode && ActiveChildIndex >= 0 && ActiveChildIndex < RandomInfo.Num() )
	{
		FRandomAnimInfo& Info = RandomInfo(ActiveChildIndex);

		// If we need to loop, loop!
		if( Info.LoopCount > 0 )
		{
			Info.LoopCount--;
			return ActiveChildIndex;
		}
	}

	// Build a list of valid indices to choose from
	TArray<INT> IndexList;
	FLOAT TotalWeight = 0.f;
	for(INT Idx=0; Idx<Children.Num(); Idx++)
	{
		if( Idx != ActiveChildIndex && Idx < RandomInfo.Num() && Children(Idx).Weight <= ZERO_ANIMWEIGHT_THRESH && Children(Idx).Anim )
		{
			IndexList.AddItem(Idx);
			TotalWeight += RandomInfo(Idx).Chance;
		}
	}

	// If we have valid children to choose from
	// (Fall through will just replay current active child)
	if( IndexList.Num() > 0 )
	{
		TArray<FLOAT> Weights;
		Weights.Add(IndexList.Num());
	
		/** Value used to normalize weights so all childs add up to 1.f */
		for(INT i=0; i<IndexList.Num(); i++)
		{
			Weights(i) = RandomInfo(IndexList(i)).Chance / TotalWeight;
		}	

		FLOAT	RandomWeight	= appFrand();
		INT		Index			= 0;
		INT		DesiredChildIdx	= IndexList(Index);

		// This child has too much weight, so skip to next.
		while( Index < IndexList.Num() - 1 && RandomWeight > Weights(Index) )
		{
			RandomWeight -= Weights(Index);
			Index++;
			DesiredChildIdx	= IndexList(Index);
		}

		FRandomAnimInfo& Info = RandomInfo(DesiredChildIdx);

		// Reset loop counter
		if( Info.LoopCountMax > Info.LoopCountMin )
		{
			Info.LoopCount = Info.LoopCountMin + appRand() % (Info.LoopCountMax - Info.LoopCountMin + 1);
		}

		return DesiredChildIdx;
	}

	// Fall back to using current one again.
	return ActiveChildIndex;
}

void UAnimNodeRandom::PlayPendingAnimation(FLOAT BlendTime, FLOAT StartTime)
{
#if DEBUG_ANIMNODERANDOM
	debugf(TEXT("%3.2f UAnimNodeRandom::PlayPendingAnimation. PendingChildIndex: %d, BlendTime: %f, StartTime: %f"), GWorld->GetTimeSeconds(), PendingChildIndex, BlendTime, StartTime);
#endif

	// if our pending child index is not valid, pick one
	if( !(PendingChildIndex >= 0 && PendingChildIndex < Children.Num() && PendingChildIndex < RandomInfo.Num() && Children(PendingChildIndex).Anim) )
	{
		PendingChildIndex = PickNextAnimIndex();
#if DEBUG_ANIMNODERANDOM
		debugf(TEXT("%3.2f UAnimNodeRandom::PlayPendingAnimation. PendingChildIndex not valid, picked: %d"), GWorld->GetTimeSeconds(), PendingChildIndex);
#endif
		// if our pending child index is STILL not valid, abort
		if( !(PendingChildIndex >= 0 && PendingChildIndex < Children.Num() && PendingChildIndex < RandomInfo.Num() && Children(PendingChildIndex).Anim) )
		{
#if DEBUG_ANIMNODERANDOM
			debugf(TEXT("%3.2f UAnimNodeRandom::PlayPendingAnimation. PendingChildIndex still not valid, abort"), GWorld->GetTimeSeconds());
#endif
			return;
		}
	}

	// Set new active child w/ blend
	SetActiveChild(PendingChildIndex, BlendTime);

	// Play the animation if this child is a sequence
	PlayingSeqNode = Cast<UAnimNodeSequence>(Children(ActiveChildIndex).Anim);
	if( PlayingSeqNode )
	{
		FRandomAnimInfo& Info = RandomInfo(ActiveChildIndex);

		FLOAT PlayRate = Lerp(Info.PlayRateRange.X, Info.PlayRateRange.Y, appFrand());
		if( PlayRate < KINDA_SMALL_NUMBER )
		{
			PlayRate = 1.f;
		}
		PlayingSeqNode->PlayAnim(FALSE, PlayRate, 0.f);

		// Advance to proper position if needed, useful for looping animations.
		if( StartTime > 0.f )
		{
			PlayingSeqNode->SetPosition(StartTime * PlayingSeqNode->GetGlobalPlayRate(), TRUE);
		}
	}

	// Pick PendingChildIndex for next time...
	PendingChildIndex = PickNextAnimIndex();
#if DEBUG_ANIMNODERANDOM
	debugf(TEXT("%3.2f UAnimNodeRandom::PlayPendingAnimation. Picked PendingChildIndex: %d"), GWorld->GetTimeSeconds(), PendingChildIndex);
#endif
}


/** Ticking, updates weights... */
void UAnimNodeRandom::TickAnim(FLOAT DeltaSeconds, FLOAT TotalWeight)
{
	// Check for transition when a new animation is playing
	// in order to trigger blend early for smooth transitions.
	if( ActiveChildIndex >=0 && ActiveChildIndex < RandomInfo.Num() )
	{
		// Do this only when transitioning from one animation to another. not looping.
		if( ActiveChildIndex != PendingChildIndex )
		{
			FRandomAnimInfo& Info = RandomInfo(ActiveChildIndex);

			if( Info.BlendInTime > 0.f && PlayingSeqNode && PlayingSeqNode->AnimSeq )
			{
				const FLOAT TimeLeft = PlayingSeqNode->GetTimeLeft();

				if( TimeLeft <= Info.BlendInTime )
				{
#if DEBUG_ANIMNODERANDOM
					debugf(TEXT("%3.2f UAnimNodeRandom::TickAnim. Blending to pending animation. TimeLeft: %f"), GWorld->GetTimeSeconds(), TimeLeft);
#endif
					PlayPendingAnimation(TimeLeft);
				}
			}
		}
	}
	else
	{
		// we're not doing anything??
		PlayPendingAnimation();
	}

	Super::TickAnim(DeltaSeconds, TotalWeight);
}


///////////////////////////////////////
/////// AnimNodeBlendByPhysics ////////
///////////////////////////////////////

/**
 * Blend animations based on an Owner's physics.
 *
 * @param DeltaSeconds	Time since last tick in seconds.
 */
void UAnimNodeBlendByPhysics::TickAnim( FLOAT DeltaSeconds, FLOAT TotalWeight )
{
	APawn* Owner = SkelComponent ? Cast<APawn>(SkelComponent->GetOwner()) : NULL;

	if ( Owner )
	{
		if( ActiveChildIndex != Owner->Physics )
		{
			SetActiveChild( Owner->Physics , 0.1f );
		}
	}

	Super::TickAnim(DeltaSeconds, TotalWeight);
}


///////////////////////////////////////
/////// AnimNodeBlendByBase ///////////
///////////////////////////////////////

void UAnimNodeBlendByBase::TickAnim( FLOAT DeltaSeconds, FLOAT TotalWeight )
{
	APawn* Owner = SkelComponent ? Cast<APawn>(SkelComponent->GetOwner()) : NULL;

	if( Owner )
	{
		INT DesiredChildIdx = 0;

		if( Owner->Base )
		{
			switch(Type)
			{
				case BBT_ByActorTag:
					if( Owner->Base->Tag == ActorTag )
					{
						DesiredChildIdx = 1;
					}
					break;
				case BBT_ByActorClass:
					if( Owner->Base->GetClass() == ActorClass )
					{
						DesiredChildIdx = 1;
					}
					break;
				default:
					break;
			}
		}
		
		if( DesiredChildIdx != ActiveChildIndex )
		{
			SetActiveChild( DesiredChildIdx, 0.1f );
		}
	}

	Super::TickAnim( DeltaSeconds, TotalWeight );
}


/************************************************************************************
 * UAnimNodeScalePlayRate
 ***********************************************************************************/

void UAnimNodeScalePlayRate::TickAnim(FLOAT DeltaSeconds, FLOAT TotalWeight)
{
	if( Children(0).Anim )
	{
		TArray<UAnimNodeSequence*> SeqNodes;
		Children(0).Anim->GetAnimSeqNodes(SeqNodes);

		const FLOAT Rate = GetScaleValue();

		for( INT i=0; i<SeqNodes.Num(); i++ )
		{
			SeqNodes(i)->Rate = Rate;
		}
	}

	Super::TickAnim(DeltaSeconds, TotalWeight);
}


FLOAT UAnimNodeScalePlayRate::GetScaleValue()
{
	return ScaleByValue;
}


/************************************************************************************
 * UAnimNodeScaleRateBySpeed
 ***********************************************************************************/

FLOAT UAnimNodeScaleRateBySpeed::GetScaleValue()
{
	AActor* Owner = SkelComponent ? SkelComponent->GetOwner() : NULL;
	if( Owner && BaseSpeed > KINDA_SMALL_NUMBER )
	{
		return Owner->Velocity.Size() / BaseSpeed;
	}
	else
	{
		return ScaleByValue;
	}
}


/************************************************************************************
 * UAnimNodePlayCustomAnim
 ***********************************************************************************/

/**
 * Main tick
 */
void UAnimNodePlayCustomAnim::TickAnim(FLOAT DeltaSeconds, FLOAT TotalWeight)
{
	// check for custom animation timing, to blend out before it is over.
	if( bIsPlayingCustomAnim && CustomPendingBlendOutTime >= 0.f )
	{
		UAnimNodeSequence *ActiveChild = Cast<UAnimNodeSequence>(Children(1).Anim);
		if( ActiveChild && ActiveChild->AnimSeq )
		{
			const FLOAT TimeLeft = ActiveChild->AnimSeq->SequenceLength - ActiveChild->CurrentTime;

			// if it's time to blend back to previous animation, do so.
			if( TimeLeft <= CustomPendingBlendOutTime )
			{
				bIsPlayingCustomAnim = FALSE;
			}
		}
	}

	const FLOAT DesiredChild2Weight = bIsPlayingCustomAnim ? 1.f : 0.f;

	// Child needs to be updated
	if( DesiredChild2Weight != Child2WeightTarget )
	{
		FLOAT BlendInTime = 0.f;

		// if blending out from Custom animation node, use CustomPendingBlendOutTime
		if( Child2WeightTarget == 1.f && CustomPendingBlendOutTime >= 0 )
		{
			BlendInTime					= CustomPendingBlendOutTime;
			CustomPendingBlendOutTime	= -1;
		}
		SetBlendTarget(DesiredChild2Weight, BlendInTime);
	}

	// and call the parent version to perform the actual interpolation
	Super::TickAnim(DeltaSeconds, TotalWeight);
}


/**
 * Play a custom animation.
 * Supports many features, including blending in and out. *
 * @param	AnimName		Name of animation to play.
 * @param	Rate			Rate the animation should be played at.
 * @param	BlendInTime		Blend duration to play anim.
 * @param	BlendOutTime	Time before animation ends (in seconds) to blend out.
 *							-1.f means no blend out. 
 *							0.f = instant switch, no blend. 
 *							otherwise it's starting to blend out at AnimDuration - BlendOutTime seconds.
 * @param	bLooping		Should the anim loop? (and play forever until told to stop)
 * @param	bOverride		play same animation over again only if bOverride is set to true.
 */
FLOAT UAnimNodePlayCustomAnim::PlayCustomAnim(FName AnimName, FLOAT Rate, FLOAT BlendInTime, FLOAT BlendOutTime, UBOOL bLooping, UBOOL bOverride)
{
	// Pre requisites
	if( AnimName == NAME_None || Rate <= 0.f )
	{
		return 0.f;
	}

	UAnimNodeSequence* Child = Cast<UAnimNodeSequence>(Children(1).Anim);
	if( Child )
	{
		SetBlendTarget(1.f, BlendInTime);
		bIsPlayingCustomAnim = TRUE;

		// when looping an animation, blend out time is not supported
		CustomPendingBlendOutTime = bLooping ? -1.f : BlendOutTime;

		// if already playing the same anim, replay it again only if bOverride is set to true.
		if( Child->AnimSeqName == AnimName && Child->bPlaying && !bOverride && Child->bLooping == bLooping )
		{
			return 0.f;
		}

		if( Child->AnimSeqName != AnimName )
		{
			Child->SetAnim(AnimName);
		}
		Child->PlayAnim(bLooping, Rate, 0.f);
		return Child->GetAnimPlaybackLength();
	}
	return 0.f;
}


/**
 * Play a custom animation. 
 * Auto adjusts the animation's rate to match a given duration in seconds.
 * Supports many features, including blending in and out.
 *
 * @param	AnimName		Name of animation to play.
 * @param	Duration		duration in seconds the animation should be played.
 * @param	BlendInTime		Blend duration to play anim.
 * @param	BlendOutTime	Time before animation ends (in seconds) to blend out.
 *							-1.f means no blend out. 
 *							0.f = instant switch, no blend. 
 *							otherwise it's starting to blend out at AnimDuration - BlendOutTime seconds.
 * @param	bLooping		Should the anim loop? (and play forever until told to stop)
 * @param	bOverride		play same animation over again only if bOverride is set to true.
 */
void UAnimNodePlayCustomAnim::PlayCustomAnimByDuration(FName AnimName, FLOAT Duration, FLOAT BlendInTime, FLOAT BlendOutTime, UBOOL bLooping, UBOOL bOverride)
{
	// Pre requisites
	if( AnimName == NAME_None || Duration <= 0.f )
	{
		return;
	}

	UAnimSequence* AnimSeq = SkelComponent->FindAnimSequence(AnimName);
	if( AnimSeq )
	{
		const FLOAT NewRate = AnimSeq->SequenceLength / Duration;
		PlayCustomAnim(AnimName, NewRate, BlendInTime, BlendOutTime, bLooping, bOverride);
	}
	else
	{
		debugf(TEXT("UWarAnim_PlayCustomAnim::PlayAnim - AnimSequence for %s not found"), *AnimName.ToString());
	}
}


/** 
 * Stop playing a custom animation. 
 * Used for blending out of a looping custom animation.
 */
void UAnimNodePlayCustomAnim::StopCustomAnim(FLOAT BlendOutTime)
{
	if( bIsPlayingCustomAnim )
	{
		bIsPlayingCustomAnim		= FALSE;
		CustomPendingBlendOutTime	= BlendOutTime;
	}
}


/************************************************************************************
 * UAnimNodeSlot
 ***********************************************************************************/

/**
 * Will ensure TargetWeight array is right size. If it has to resize it, will set Chidlren(0) to have a target of 1.0.
 * Also, if all Children weights are zero, will set Children(0) as the active child.
 * 
 * @see UAnimNode::InitAnim
 */
void UAnimNodeSlot::InitAnim(USkeletalMeshComponent* MeshComp, UAnimNodeBlendBase* Parent)
{
	Super::InitAnim(MeshComp, Parent);

	if( Children.Num() > 0 )
	{
		// If all child weights are zero - set the first one to the active child.
		const FLOAT ChildWeightSum = GetChildWeightTotal();
		if( ChildWeightSum <= ZERO_ANIMWEIGHT_THRESH )
		{
			Children(0).Weight = 1.f;
		}
	}

	if( TargetWeight.Num() != Children.Num() )
	{
		TargetWeight.Empty();
		TargetWeight.AddZeroed(Children.Num());

		if( TargetWeight.Num() > 0 )
		{
			TargetWeight(0) = 1.f;
		}
	}

	// If all child weights are zero - set the first one to the active child.
	const FLOAT ChildWeightSum = GetChildWeightTotal();
	if( ChildWeightSum <= ZERO_ANIMWEIGHT_THRESH )
	{
		SetActiveChild(TargetChildIndex, 0.f);
	}

	// clear SynchNode; we find it on-demand in AddToSynchGroup() to avoid large initialization times when there are many copies of this object in the tree
	SynchNode = NULL;
}

/** Update position of given channel */
void UAnimNodeSlot::MAT_SetAnimPosition(INT ChannelIndex, FName InAnimSeqName, FLOAT InPosition, UBOOL bFireNotifies, UBOOL bLooping)
{
	const INT ChildNum = ChannelIndex + 1;

	// laurent -- figure out a way to handle that gracefully
	if( ChildNum >= Children.Num() )
	{
		debugf(TEXT("UAnimNodeSlot::MAT_SetAnimPosition, invalid ChannelIndex: %d"), ChannelIndex);
		return;
	}

	UAnimNodeSequence* SeqNode = Cast<UAnimNodeSequence>(Children(ChildNum).Anim);
	if( SeqNode )
	{
		// Update Animation if needed
		if( SeqNode->AnimSeqName != InAnimSeqName )
		{
			SeqNode->SetAnim(InAnimSeqName);
		}

		SeqNode->bLooping = bLooping;

		// Set new position
		SeqNode->SetPosition(InPosition, bFireNotifies);
	}
}

/** Update weight of channels */
void UAnimNodeSlot::MAT_SetAnimWeights(const FAnimSlotInfo& SlotInfo)
{
	const INT NumChilds = Children.Num();

	if( NumChilds == 1 )
	{
		// Only one child, not much choice here!
		Children(0).Weight = 1.f;
	}
	else if( NumChilds >= 2 )
	{
		// number of channels from Matinee
		const INT NumChannels	= SlotInfo.ChannelWeights.Num();
		FLOAT AccumulatedWeight = 0.f;

		// Set blend weight to each child, from Matinee channels alpha.
		// Start from last to first, as we want bottom channels to have precedence over top ones.
		for(INT i=Children.Num()-1; i>0; i--)
		{
			const INT	ChannelIndex	= i - 1;
			const FLOAT ChannelWeight	= ChannelIndex < NumChannels ? Clamp<FLOAT>(SlotInfo.ChannelWeights(ChannelIndex), 0.f, 1.f) : 0.f;
			Children(i).Weight			= ChannelWeight * (1.f - AccumulatedWeight);
			AccumulatedWeight			+= Children(i).Weight;
		}
		
		// Set remaining weight to "normal" / animtree animation.
		Children(0).Weight = 1.f - AccumulatedWeight;
	}
}


/** Rename all child nodes upon Add/Remove, so they match their position in the array. */
void UAnimNodeSlot::RenameChildConnectors()
{
	const INT NumChildren = Children.Num();

	if( NumChildren > 0 )
	{
		Children(0).Name = FName(TEXT("Source"));

		for(INT i=1; i<NumChildren; i++)
		{
			Children(i).Name = FName(*FString::Printf(TEXT("Channel %2d"), i-1));
		}
	}
}

/**
 * When requested to play a new animation, we need to find a new child.
 * We'd like to find one that is unused for smooth blending, 
 * but that may be a luxury that is not available.
 */
INT UAnimNodeSlot::FindBestChildToPlayAnim(FName AnimToPlay)
{
	FLOAT	BestWeight	= 1.f;
	INT		BestIndex	= INDEX_NONE;

	for(INT i=1; i<Children.Num(); i++)
	{
		if( BestIndex == INDEX_NONE || Children(i).Weight < BestWeight )
		{
			BestIndex	= i;
			BestWeight	= Children(i).Weight;
		}
	}

	// Send a warning if node we've picked is relevant. This is going to result in a visible pop.
	if( BestWeight > ZERO_ANIMWEIGHT_THRESH )
	{
		AActor* AOwner = SkelComponent ? SkelComponent->GetOwner() : NULL;
		debugf(TEXT("UAnimNodeSlot::FindBestChildToPlayAnim - Best Index %d with a weight of %f, for Anim: %s and Owner: %s"), 
			BestIndex, BestWeight, *AnimToPlay.ToString(), *AOwner->GetName());
	}

	return BestIndex;
}


/**
 * Play a custom animation.
 * Supports many features, including blending in and out.
 *
 * @param	AnimName		Name of animation to play.
 * @param	Rate			Rate the animation should be played at.
 * @param	BlendInTime		Blend duration to play anim.
 * @param	BlendOutTime	Time before animation ends (in seconds) to blend out.
 *							-1.f means no blend out. 
 *							0.f = instant switch, no blend. 
 *							otherwise it's starting to blend out at AnimDuration - BlendOutTime seconds.
 * @param	bLooping		Should the anim loop? (and play forever until told to stop)
 * @param	bOverride		play same animation over again only if bOverride is set to true.
 */
FLOAT UAnimNodeSlot::PlayCustomAnim(FName AnimName, FLOAT Rate, FLOAT BlendInTime, FLOAT BlendOutTime, UBOOL bLooping, UBOOL bOverride)
{
	// Pre requisites
	if( AnimName == NAME_None || Rate <= 0.f )
	{
		return 0.f;
	}

	// Figure out on which child to the play the animation on.
	// If BlendInTime == 0.f, then no need to look for another child to play the animation on,
	// It's fine to use the same
	if( CustomChildIndex < 1 || BlendInTime > 0.f )
	{
		CustomChildIndex = FindBestChildToPlayAnim(AnimName);
	}

	if( CustomChildIndex < 1 || CustomChildIndex >= Children.Num() )
	{
		debugf(TEXT("UAnimNodeSlot::PlayCustomAnim, CustomChildIndex %d is out of bounds."), CustomChildIndex);
		return 0.f;
	}

	UAnimNodeSequence* SeqNode = Cast<UAnimNodeSequence>(Children(CustomChildIndex).Anim);
	if( SeqNode )
	{
		UBOOL bSetAnim = TRUE;

		// if already playing the same anim, replay it again only if bOverride is set to true.
		if( !bOverride && 
			SeqNode->bPlaying && SeqNode->bLooping == bLooping && 
			SeqNode->AnimSeqName == AnimName && SeqNode->AnimSeq != NULL )
		{
			bSetAnim = FALSE;
		}

		if( bSetAnim )
		{
			if( SeqNode->AnimSeqName != AnimName || SeqNode->AnimSeq == NULL )
			{
				SeqNode->SetAnim(AnimName);
				if( SeqNode->AnimSeq == NULL )
				{
					// Animation was not found, so abort. because we wouldn't be able to blend out.
					return 0.f;
				}
			}

			// Play the animation
			SeqNode->PlayAnim(bLooping, Rate, 0.f);
		}

		SetActiveChild(CustomChildIndex, BlendInTime);
		bIsPlayingCustomAnim = TRUE;

		// when looping an animation, blend out time is not supported
		PendingBlendOutTime = bLooping ? -1.f : BlendOutTime;

		// Disable Actor AnimEnd notification.
		SetActorAnimEndNotification(FALSE);

#if 0 // DEBUG
		UAnimNodeSequence* SeqNode = GetCustomAnimNodeSeq();
		debugf(TEXT("PlayCustomAnim %s, CustomChildIndex: %d"), *AnimName, CustomChildIndex);
#endif
		return SeqNode->GetAnimPlaybackLength();
	}
	else
	{
		debugf(TEXT("UAnimNodeSlot::PlayCustomAnim, Child %d, is not hooked up to a AnimNodeSequence."), CustomChildIndex);
	}

	return 0.f;
}


/**
 * Play a custom animation. 
 * Auto adjusts the animation's rate to match a given duration in seconds.
 * Supports many features, including blending in and out.
 *
 * @param	AnimName		Name of animation to play.
 * @param	Duration		duration in seconds the animation should be played.
 * @param	BlendInTime		Blend duration to play anim.
 * @param	BlendOutTime	Time before animation ends (in seconds) to blend out.
 *							-1.f means no blend out. 
 *							0.f = instant switch, no blend. 
 *							otherwise it's starting to blend out at AnimDuration - BlendOutTime seconds.
 * @param	bLooping		Should the anim loop? (and play forever until told to stop)
 * @param	bOverride		play same animation over again only if bOverride is set to true.
 */
void UAnimNodeSlot::PlayCustomAnimByDuration(FName AnimName, FLOAT Duration, FLOAT BlendInTime, FLOAT BlendOutTime, UBOOL bLooping, UBOOL bOverride)
{
	// Pre requisites
	if( AnimName == NAME_None || Duration <= 0.f )
	{
		return;
	}

	UAnimSequence* AnimSeq = SkelComponent->FindAnimSequence(AnimName);
	if( AnimSeq )
	{
		FLOAT NewRate = AnimSeq->SequenceLength / Duration;
		if( AnimSeq->RateScale > 0.f )
		{
			NewRate /= AnimSeq->RateScale;
		}

		PlayCustomAnim(AnimName, NewRate, BlendInTime, BlendOutTime, bLooping, bOverride);
	}
	else
	{
		debugf(TEXT("UAnimNodeSlot::PlayAnim - AnimSequence for %s not found"), *AnimName.ToString());
	}
}

/** Returns the Name of the currently played animation or NAME_None otherwise. */
FName UAnimNodeSlot::GetPlayedAnimation()
{
	UAnimNodeSequence* SeqNode = GetCustomAnimNodeSeq();
	if( SeqNode )
	{	
		return SeqNode->AnimSeqName;
	}

	return NAME_None;
}


/** 
 * Stop playing a custom animation. 
 * Used for blending out of a looping custom animation.
 */
void UAnimNodeSlot::StopCustomAnim(FLOAT BlendOutTime)
{
	if( bIsPlayingCustomAnim )
	{
#if 0 // DEBUG
		UAnimNodeSequence* SeqNode = GetCustomAnimNodeSeq();
		debugf(TEXT("StopCustomAnim %s, CustomChildIndex: %d"), *SeqNode->AnimSeqName, CustomChildIndex);
#endif

		bIsPlayingCustomAnim = FALSE;
		SetActiveChild(0, BlendOutTime);
	}
}


/** 
 * Stop playing a custom animation. 
 * Used for blending out of a looping custom animation.
 */
void UAnimNodeSlot::SetCustomAnim(FName AnimName)
{
	if( bIsPlayingCustomAnim )
	{
		UAnimNodeSequence* SeqNode = GetCustomAnimNodeSeq();
		if( SeqNode && SeqNode->AnimSeqName != AnimName )
		{
			SeqNode->SetAnim(AnimName);
		}
	}
}

/** 
 * Set bCauseActorAnimEnd flag to receive AnimEnd() notification.
 */
void UAnimNodeSlot::SetActorAnimEndNotification(UBOOL bNewStatus)
{
	// Set all childs but the active one to FALSE. 
	// Active one is set to bNewStatus
	for(INT i=1; i<Children.Num(); i++)
	{
		UAnimNodeSequence* SeqNode = Cast<UAnimNodeSequence>(Children(i).Anim);

		if( SeqNode )
		{
			SeqNode->bCauseActorAnimEnd = (bIsPlayingCustomAnim && i == CustomChildIndex ? bNewStatus : FALSE);
		}
	}
}


/** 
 * Returns AnimNodeSequence currently selected for playing animations.
 * Note that calling PlayCustomAnim *may* change which node plays the animation.
 * (Depending on the blend in time, and how many nodes are available, to provide smooth transitions.
 */
UAnimNodeSequence* UAnimNodeSlot::GetCustomAnimNodeSeq()
{
	if( CustomChildIndex > 0 )
	{
		return Cast<UAnimNodeSequence>(Children(CustomChildIndex).Anim);
	}

	return NULL;
}


/**
 * Set custom animation root bone options.
 */
void UAnimNodeSlot::SetRootBoneAxisOption(BYTE AxisX, BYTE AxisY, BYTE AxisZ)
{
	UAnimNodeSequence* SeqNode = GetCustomAnimNodeSeq();

	if( SeqNode )
	{
		SeqNode->RootBoneOption[0] = AxisX;
		SeqNode->RootBoneOption[1] = AxisY;
		SeqNode->RootBoneOption[2] = AxisZ;
	}
}


/** 
 * Synchronize this animation with others. 
 * @param GroupName	Add node to synchronization group named group name.
 */
void UAnimNodeSlot::AddToSynchGroup(FName GroupName)
{
	UAnimNodeSequence* SeqNode = GetCustomAnimNodeSeq();

	if( !SeqNode || SeqNode->SynchGroupName == GroupName )
	{
		return;
	}

	// Find synchronization node if necessary
	if (SynchNode == NULL)
	{
		TArray<UAnimNode*> Nodes;
		SkelComponent->Animations->GetNodes(Nodes);
		for(INT i = 0; i < Nodes.Num(); i++)
		{
			SynchNode = Cast<UAnimNodeSynch>(Nodes(i));
			if (SynchNode != NULL)
			{
				break;
			}
		}
	}

	if (SynchNode != NULL)
	{
		// If node had a group, leave it
		if( SeqNode->SynchGroupName != NAME_None )
		{
			SynchNode->RemoveNodeFromGroup(SeqNode, SeqNode->SynchGroupName);
		}

		// If a new group name is set, assign it
		if( GroupName != NAME_None )
		{
			SynchNode->AddNodeToGroup(SeqNode, GroupName);
		}
	}
}


/**  */
void UAnimNodeSlot::TickAnim(FLOAT DeltaSeconds, FLOAT TotalWeight)
{
	if( bIsPlayingCustomAnim   )
	{
		UAnimNodeSequence *SeqNode = GetCustomAnimNodeSeq();

		// Hmm there's no animations to play, so we shouldn't be here
		if( !SeqNode || !SeqNode->AnimSeq )
		{
			StopCustomAnim(0.f);
		}

		// check for custom animation timing, to blend out before it is over.
		if( PendingBlendOutTime >= 0.f )
		{
			if( SeqNode && SeqNode->AnimSeq )
			{
				const FLOAT TimeLeft = SeqNode->GetTimeLeft();

				// if it's time to blend back to previous animation, do so.
				if( TimeLeft <= PendingBlendOutTime )
				{
					StopCustomAnim(TimeLeft);
				}
			}
		}
	}

	check(Children.Num() == TargetWeight.Num());

	// Do nothing if BlendTimeToGo is zero.
	if( BlendTimeToGo > 0.f )
	{
		// So we don't overshoot.
		if( BlendTimeToGo <= DeltaSeconds )
		{
			BlendTimeToGo = 0.f; 

			for(INT i=0; i<Children.Num(); i++)
			{
				Children(i).Weight = TargetWeight(i);
			}
		}
		else
		{
			for(INT i=0; i<Children.Num(); i++)
			{
				// Amount left to blend.
				const FLOAT BlendDelta = TargetWeight(i) - Children(i).Weight;
				Children(i).Weight += (BlendDelta / BlendTimeToGo) * DeltaSeconds;
			}

			BlendTimeToGo -= DeltaSeconds;
		}
	}

	// Tick child nodes
	Super::TickAnim(DeltaSeconds, TotalWeight);
}


/**
 * The the child to blend up to full Weight (1.0)
 * 
 * @param ChildIndex Index of child node to ramp in. If outside range of children, will set child 0 to active.
 * @param BlendTime How long for child to take to reach weight of 1.0.
 */
void UAnimNodeSlot::SetActiveChild(INT ChildIndex, FLOAT BlendTime)
{
	check(Children.Num() == TargetWeight.Num());

	if( ChildIndex < 0 || ChildIndex >= Children.Num() )
	{
		debugf( TEXT("UAnimNodeBlendList::SetActiveChild : %s ChildIndex (%d) outside number of Children (%d)."), *GetName(), ChildIndex, Children.Num() );
		ChildIndex = 0;
	}

	// Scale blend time by weight of target child.
	BlendTime *= (1.f - Children(ChildIndex).Weight);

	// If time is too small, consider instant blend
	if( BlendTime < ZERO_ANIMWEIGHT_THRESH )
	{
		BlendTime = 0.f;
	}

	for(INT i=0; i<Children.Num(); i++)
	{
		// Child becoming active
		if( i == ChildIndex )
		{
			TargetWeight(i) = 1.0f;

			// If we basically want this straight away - dont wait until a tick to update weights.
			if( BlendTime == 0.0f )
			{
				Children(i).Weight = 1.0f;
			}
		}
		// Others, blending to zero weight
		else
		{
			TargetWeight(i) = 0.f;

			if( BlendTime == 0.0f )
			{
				Children(i).Weight = 0.0f;
			}
		}
	}

	BlendTimeToGo		= BlendTime;
	TargetChildIndex	= ChildIndex;
}


/** Called when we add a child to this node. We reset the TargetWeight array when this happens. */
void UAnimNodeSlot::OnAddChild(INT ChildNum)
{
	Super::OnAddChild(ChildNum);

	ResetTargetWeightArray(TargetWeight, Children.Num());
}

/** Called when we remove a child to this node. We reset the TargetWeight array when this happens. */
void UAnimNodeSlot::OnRemoveChild(INT ChildNum)
{
	Super::OnRemoveChild(ChildNum);

	ResetTargetWeightArray(TargetWeight, Children.Num());
}


