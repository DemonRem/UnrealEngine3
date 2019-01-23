/*=============================================================================
	UnSkelControl.cpp: Skeletal mesh bone controllers and IK.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/ 

#include "EnginePrivate.h"
#include "EngineAnimClasses.h"

IMPLEMENT_CLASS(USkelControlBase);
IMPLEMENT_CLASS(USkelControlSingleBone);
IMPLEMENT_CLASS(USkelControlLookAt);
IMPLEMENT_CLASS(USkelControlSpline);
IMPLEMENT_CLASS(USkelControlLimb);
IMPLEMENT_CLASS(USkelControlFootPlacement);
IMPLEMENT_CLASS(USkelControlWheel);
IMPLEMENT_CLASS(USkelControlTrail);

#define CONTROL_DIAMOND_SIZE	(3.f)

static void CopyRotationPart(FMatrix& DestMatrix, const FMatrix& SrcMatrix)
{
	DestMatrix.M[0][0] = SrcMatrix.M[0][0];
	DestMatrix.M[0][1] = SrcMatrix.M[0][1];
	DestMatrix.M[0][2] = SrcMatrix.M[0][2];

	DestMatrix.M[1][0] = SrcMatrix.M[1][0];
	DestMatrix.M[1][1] = SrcMatrix.M[1][1];
	DestMatrix.M[1][2] = SrcMatrix.M[1][2];

	DestMatrix.M[2][0] = SrcMatrix.M[2][0];
	DestMatrix.M[2][1] = SrcMatrix.M[2][1];
	DestMatrix.M[2][2] = SrcMatrix.M[2][2];
}

/** Utility function for turning axis indicator enum into direction vector, possibly inverted. */
FVector USkelControlBase::GetAxisDirVector(BYTE InAxis, UBOOL bInvert)
{
	FVector AxisDir;

	if(InAxis == AXIS_X)
	{
		AxisDir = FVector(1,0,0);
	}
	else if(InAxis == AXIS_Y)
	{
		AxisDir =  FVector(0,1,0);
	}
	else
	{
		AxisDir =  FVector(0,0,1);
	}

	if(bInvert)
	{
		AxisDir *= -1.f;
	}

	return AxisDir;
}

/** 
 *	Create a matrix given two arbitrary rows of it.
 *	We generate the missing row using another cross product, but we have to get the order right to avoid changing handedness.
 */
FMatrix USkelControlBase::BuildMatrixFromVectors(BYTE Vec1Axis, const FVector& Vec1, BYTE Vec2Axis, const FVector& Vec2)
{
	check(Vec1 != Vec2);

	FMatrix OutMatrix = FMatrix::Identity;

	if(Vec1Axis == AXIS_X)
	{
		OutMatrix.SetAxis(0, Vec1);

		if(Vec2Axis == AXIS_Y)
		{
			OutMatrix.SetAxis(1, Vec2);
			OutMatrix.SetAxis(2, Vec1 ^ Vec2);
		}
		else // AXIS_Z
		{
			OutMatrix.SetAxis(2, Vec2);
			OutMatrix.SetAxis(1, Vec2 ^ Vec1 );
		}
	}
	else if(Vec1Axis == AXIS_Y)
	{
		OutMatrix.SetAxis(1, Vec1);

		if(Vec2Axis == AXIS_X)
		{
			OutMatrix.SetAxis(0, Vec2);
			OutMatrix.SetAxis(2, Vec2 ^ Vec1);
		}
		else // AXIS_Z
		{
			OutMatrix.SetAxis(2, Vec2);
			OutMatrix.SetAxis(0, Vec1 ^ Vec2 );
		}
	}
	else // AXIS_Z
	{
		OutMatrix.SetAxis(2, Vec1);

		if(Vec2Axis == AXIS_X)
		{
			OutMatrix.SetAxis(0, Vec2);
			OutMatrix.SetAxis(1, Vec1 ^ Vec2);
		}
		else // AXIS_Y
		{
			OutMatrix.SetAxis(1, Vec2);
			OutMatrix.SetAxis(0, Vec2 ^ Vec1 );
		}
	}

	FLOAT Det = OutMatrix.Determinant();
	if( OutMatrix.Determinant() <= 0.f )
	{
		debugf( TEXT("BuildMatrixFromVectors : Bad Determinant (%f)"), Det );
		debugf( TEXT("Vec1: %d (%f %f %f)"), Vec1Axis, Vec1.X, Vec1.Y, Vec1.Z );
		debugf( TEXT("Vec2: %d (%f %f %f)"), Vec2Axis, Vec2.X, Vec2.Y, Vec2.Z );
	}
	//check( OutMatrix.Determinant() > 0.f );

	return OutMatrix;
}


/** Given two unit direction vectors, find the axis and angle between them. */
void USkelControlBase::FindAxisAndAngle(const FVector& A, const FVector& B, FVector& OutAxis, FLOAT& OutAngle)
{
	OutAxis = A ^ B;
	OutAngle = appAsin( OutAxis.Size() ); // Will always result in positive OutAngle.
	OutAxis.Normalize();

	// If dot product is negative, adjust the angle accordingly.
	if((A | B) < 0.f)
	{
		OutAngle = (FLOAT)PI - OutAngle; 
	}

}

/*-----------------------------------------------------------------------------
	USkelControlBase
-----------------------------------------------------------------------------*/


void USkelControlBase::TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp)
{
	// Update SkelComponent pointer.
	SkelComponent = SkelComp;

	// Check if we should set the control's strength to match a set of specific anim nodes.
	if( bSetStrengthFromAnimNode && SkelComp && SkelComp->Animations )
	{
		// if node is node initalized, cache list of nodes
		// in the editor, we do this every frame to catch nodes that have been edited/added/removed
		if( !bInitializedCachedNodeList || (GIsEditor && !GIsGame) )
		{
			bInitializedCachedNodeList = TRUE;

			CachedNodeList.Reset();

			// get all nodes
			TArray<UAnimNode*>	Nodes;
			SkelComp->Animations->GetNodes(Nodes);

			// iterate through list of nodes
			for(INT i=0; i<Nodes.Num(); i++)
			{
				UAnimNode* Node = Nodes(i);

				if( Node && Node->NodeName != NAME_None )
				{
					// iterate through our list of names
					for(INT ANodeNameIdx=0; ANodeNameIdx<StrengthAnimNodeNameList.Num(); ANodeNameIdx++)
					{
						if( Node->NodeName == StrengthAnimNodeNameList(ANodeNameIdx) )
						{
							CachedNodeList.AddItem(Node);
							break;
						}
					}
				}
			}
		}

		FLOAT Strength = 0.f;

		// iterate through list of cached nodes
		for(INT i=0; i<CachedNodeList.Num(); i++)
		{
			const UAnimNode* Node = CachedNodeList(i);
			
			if( Node && Node->bRelevant )
			{
				Strength += Node->NodeTotalWeight;
			}

			/* Here is if we want to match the weight of the most relevant node.
			// if the node's weight is greater, use that
			if( Node && Node->NodeTotalWeight > Strength )
			{
				Strength = Node->NodeTotalWeight;
			}
			*/
		}

		ControlStrength	= Min(Strength, 1.f);
		StrengthTarget	= ControlStrength;
	}

	if( BlendTimeToGo != 0.f ||	ControlStrength != StrengthTarget )
	{
		// Update the blend status, if one is active.
		const FLOAT BlendDelta = StrengthTarget - ControlStrength; // Amount we want to change ControlStrength by.

		if( BlendTimeToGo > DeltaSeconds )
		{
			ControlStrength	+= (BlendDelta / BlendTimeToGo) * DeltaSeconds;
			BlendTimeToGo	-= DeltaSeconds;
		}
		else
		{
			BlendTimeToGo	= 0.f; 
			ControlStrength	= StrengthTarget;
		}

		//debugf(TEXT("%3.2f SkelControl ControlStrength: %f, StrengthTarget: %f, BlendTimeToGo: %f"), GWorld->GetTimeSeconds(), ControlStrength, StrengthTarget, BlendTimeToGo);
	}
}


void USkelControlBase::SetSkelControlActive(UBOOL bInActive)
{
	if( bInActive )
	{
		StrengthTarget	= 1.f;
		BlendTimeToGo	= BlendInTime;
	}
	else
	{
		StrengthTarget	= 0.f;
		BlendTimeToGo	= BlendOutTime;
	}

	// If we want this weight NOW - update straight away (dont wait for TickSkelControl).
	if( BlendTimeToGo <= 0.f )
	{
		ControlStrength = StrengthTarget;
		BlendTimeToGo	= 0.f;
	}

	// If desired, send active call to next control in chain.
	if( NextControl && bPropagateSetActive )
	{
		NextControl->SetSkelControlActive(bInActive);
	}
}

/**
 * Set custom strength with optional blend time.
 * @param	NewStrength		Target Strength for this controller.
 * @param	InBlendTime		Time it will take to reach that new strength. (0.f == Instant)
 */
void USkelControlBase::SetSkelControlStrength(FLOAT NewStrength, FLOAT InBlendTime)
{
	// Make sure parameters are valid
	NewStrength = Clamp<FLOAT>(NewStrength, 0.f, 1.f);
	InBlendTime	= Max(InBlendTime, 0.f);

	if( StrengthTarget != NewStrength || InBlendTime < BlendTimeToGo )
	{
		StrengthTarget	= NewStrength;
		BlendTimeToGo	= InBlendTime;

		// If blend time is zero, apply now and don't wait a frame.
		if( BlendTimeToGo <= 0.f )
		{
			ControlStrength = StrengthTarget;
			BlendTimeToGo	= 0.f;
		}
	}
}


/** 
 * Get Alpha for this control. By default it is ControlStrength. 
 * 0.f means no effect, 1.f means full effect.
 * ControlStrength controls whether or not CalculateNewBoneTransforms() is called.
 * By modifying GetControlAlpha() you can still get CalculateNewBoneTransforms() called
 * but not have the controller's effect applied on the mesh.
 * This is useful for cases where you need to have the skeleton built in mesh space
 * for testing, which is not available in TickSkelControl().
 */
FLOAT USkelControlBase::GetControlAlpha()
{
	return ControlStrength;
}

void USkelControlBase::HandleControlSliderMove(FLOAT NewSliderValue)
{
	ControlStrength = NewSliderValue;
	StrengthTarget = NewSliderValue;
}


/*-----------------------------------------------------------------------------
	USkelControlSingleBone
-----------------------------------------------------------------------------*/

void USkelControlSingleBone::GetAffectedBones(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<INT>& OutBoneIndices)
{
	check(OutBoneIndices.Num() == 0);
	OutBoneIndices.AddItem(BoneIndex);
}

//
// USkelControlSingleBone::ApplySkelControl
//
void USkelControlSingleBone::CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms)
{
	check(OutBoneTransforms.Num() == 0);

	FMatrix NewBoneTM = SkelComp->SpaceBases(BoneIndex);

	if(bApplyRotation)
	{
		// SpaceBases are in component space - so we need to calculate the BoneRotationSpace -> Component transform
		FMatrix ComponentToFrame = SkelComp->CalcComponentToFrameMatrix(BoneIndex, BoneRotationSpace, RotationSpaceBoneName);
		ComponentToFrame.SetOrigin( FVector(0.f) );

		FMatrix FrameToComponent = ComponentToFrame.Inverse();

		FRotationMatrix RotInFrame(BoneRotation);

		// Add to existing rotation
		FMatrix RotInComp;
		if(bAddRotation)
		{
			RotInComp = NewBoneTM * (ComponentToFrame * RotInFrame * FrameToComponent);
		}
		// Replace current rotation
		else
		{
			RotInComp = RotInFrame * FrameToComponent;
		}
		RotInComp.SetOrigin( NewBoneTM.GetOrigin() );

		NewBoneTM = RotInComp;
	}

	if(bApplyTranslation)
	{
		FMatrix ComponentToFrame = SkelComp->CalcComponentToFrameMatrix(BoneIndex, BoneTranslationSpace, TranslationSpaceBoneName);

		// Add to current transform
		if(bAddTranslation)
		{
			FVector TransInComp = ComponentToFrame.InverseTransformNormal(BoneTranslation);

			FVector NewOrigin = NewBoneTM.GetOrigin() + TransInComp;
			NewBoneTM.SetOrigin(NewOrigin);
		}
		// Replace current translation
		else
		{
			// Translation in the component reference frame.
			FVector TransInComp = ComponentToFrame.InverseTransformFVector(BoneTranslation);

			NewBoneTM.SetOrigin(TransInComp);
		}
	}

	OutBoneTransforms.AddItem(NewBoneTM);
}

INT USkelControlSingleBone::GetWidgetCount()
{
	if(bApplyTranslation)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

FMatrix USkelControlSingleBone::GetWidgetTM(INT WidgetIndex, USkeletalMeshComponent* SkelComp, INT BoneIndex)
{
	check(WidgetIndex == 0);
	FMatrix ComponentToFrame = SkelComp->CalcComponentToFrameMatrix(BoneIndex, BoneTranslationSpace, TranslationSpaceBoneName);
	FMatrix FrameToComponent = ComponentToFrame.Inverse() * SkelComp->LocalToWorld;
	FrameToComponent.SetOrigin( SkelComp->LocalToWorld.TransformFVector( SkelComp->SpaceBases(BoneIndex).GetOrigin() ) );

	return FrameToComponent;
}

void USkelControlSingleBone::HandleWidgetDrag(INT WidgetIndex, const FVector& DragVec)
{
	check(WidgetIndex == 0);
	BoneTranslation += DragVec;
}

/*-----------------------------------------------------------------------------
	USkelControlLookAt
-----------------------------------------------------------------------------*/

/** LookAtAlpha allows to cancel head look when going beyond boundaries */
FLOAT USkelControlLookAt::GetControlAlpha()
{
	return ControlStrength * LookAtAlpha;
}

void USkelControlLookAt::SetLookAtAlpha(FLOAT DesiredAlpha, FLOAT DesiredBlendTime)
{
	if( LookAtAlphaTarget != DesiredAlpha )
	{
		LookAtAlphaTarget			= DesiredAlpha;
		LookAtAlphaBlendTimeToGo	= DesiredBlendTime * Abs(LookAtAlphaTarget - LookAtAlpha);
	}
}

UBOOL USkelControlLookAt::CanLookAtPoint(FVector PointLoc, UBOOL bDrawDebugInfo, UBOOL bDebugUsePersistentLines, UBOOL bDebugFlushLinesFirst)
{
	// we need access to the SpaceBases array..
	if( !SkelComponent || (GWorld->GetWorldInfo()->TimeSeconds - SkelComponent->LastRenderTime) > 1.0f )
	{
		debugf(TEXT("USkelControlLookAt::CanLookAtPoint, no SkelComponent, or not rendered recently."));
		return FALSE;
	}

	const UAnimTree* Tree = Cast<UAnimTree>(SkelComponent->Animations);

	if( !Tree )
	{
		debugf(TEXT("USkelControlLookAt::CanLookAtPoint, no AnimTree."));
		return FALSE;
	}

	// Find our bone index... argh wish there was a better way to know
	INT ControlBoneIndex = INDEX_NONE;
	for(INT i=0; i<SkelComponent->RequiredBones.Num() && ControlBoneIndex == INDEX_NONE; i++)
	{
		const INT BoneIndex = SkelComponent->RequiredBones(i);

		if( (SkelComponent->SkelControlIndex.Num() > 0) && (SkelComponent->SkelControlIndex(BoneIndex) != 255) )
		{
			const INT ControlIndex = SkelComponent->SkelControlIndex(BoneIndex);
			USkelControlBase* Control = Tree->SkelControlLists(ControlIndex).ControlHead;
			while( Control )
			{
				// we found us... so we found the boneindex... wheee
				if( Control == this )
				{
					ControlBoneIndex = BoneIndex;
					break;
				}
				Control = Control->NextControl;
			}
		}
	}

	if( ControlBoneIndex == INDEX_NONE )
	{
		debugf(TEXT("USkelControlLookAt::CanLookAtPoint, BoneIndex not found."));
		return FALSE;
	}

	// Original look dir, not influenced by look at controller.
	FVector OriginalLookDir		= BaseLookDir;
	FVector BonePosCompSpace	= BaseBonePos;

	// If BoneController has not been calculated recently, BaseLookDir and BaseBonePos information is not accurate.
	// So we grab it from the mesh. But it may be affected by other skel controllers...
	// So to be safe, it's better to use SetLookAtAlpha() than SetSkelControlActive(), so the controller always updates BaseLookDir and BaseBonePos.
	if( (GWorld->GetWorldInfo()->TimeSeconds - LastCalcTime) > 1.0f )
	{
		OriginalLookDir		= SkelComponent->SpaceBases(ControlBoneIndex).TransformNormal( GetAxisDirVector(LookAtAxis, bInvertLookAtAxis) ).SafeNormal();
		BonePosCompSpace	= SkelComponent->SpaceBases(ControlBoneIndex).GetOrigin();
	}

	// Get target location, in component space.
	const FMatrix ComponentToFrame	= SkelComponent->CalcComponentToFrameMatrix(ControlBoneIndex, BCS_WorldSpace, NAME_None);
	const FVector TargetCompSpace	= ComponentToFrame.InverseTransformFVector(PointLoc);

	// Find direction vector we want to look in - again in component space.
	FVector DesiredLookDir	= (TargetCompSpace - BaseBonePos).SafeNormal();

	// Draw debug information if requested
	if( bDrawDebugInfo )
	{
		AActor* Actor = SkelComponent->GetOwner();
	
		if( bDebugFlushLinesFirst )
		{
			Actor->FlushPersistentDebugLines();
		}

		Actor->DrawDebugSphere(PointLoc, 8.f, 8, 255, 000, 000, bDebugUsePersistentLines);

		// Test to make sure i got it correct!
		const FVector TargetWorldSpace = SkelComponent->LocalToWorld.TransformFVector(TargetCompSpace);
		Actor->DrawDebugSphere(TargetWorldSpace, 4.f, 8, 000, 255, 000, bDebugUsePersistentLines);

		const FVector OriginWorldSpace = SkelComponent->LocalToWorld.TransformFVector(BonePosCompSpace);
		Actor->DrawDebugSphere(OriginWorldSpace, 8.f, 8, 000, 255, 000, bDebugUsePersistentLines);

		const FVector BaseLookDirWorldSpace	= SkelComponent->LocalToWorld.TransformNormal(OriginalLookDir).SafeNormal();
		Actor->DrawDebugLine(OriginWorldSpace, OriginWorldSpace + BaseLookDirWorldSpace * 25.f, 000, 255, 000, bDebugUsePersistentLines);

		const FVector DesiredLookDirWorldSpace	= SkelComponent->LocalToWorld.TransformNormal(DesiredLookDir).SafeNormal();
		Actor->DrawDebugLine(OriginWorldSpace, OriginWorldSpace + DesiredLookDirWorldSpace * 25.f, 255, 000, 000, bDebugUsePersistentLines);

		const FLOAT MaxAngleRadians = MaxAngle * ((FLOAT)PI/180.f);
		Actor->DrawDebugCone(OriginWorldSpace, BaseLookDirWorldSpace, 64.f, MaxAngleRadians, MaxAngleRadians, 16, FColor(0, 0, 255), bDebugUsePersistentLines);
	}

	return !ApplyLookDirectionLimits(DesiredLookDir, BaseLookDir, ControlBoneIndex, SkelComponent);
}

void USkelControlLookAt::TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp)
{
	Super::TickSkelControl(DeltaSeconds, SkelComp);

	// Interpolate LookAtAlpha
	const FLOAT BlendDelta = LookAtAlphaTarget - LookAtAlpha;
	if( LookAtAlphaBlendTimeToGo > KINDA_SMALL_NUMBER || Abs(BlendDelta) > KINDA_SMALL_NUMBER )
	{
		if( LookAtAlphaBlendTimeToGo <= DeltaSeconds || Abs(BlendDelta) <= KINDA_SMALL_NUMBER )
		{
			LookAtAlpha					= LookAtAlphaTarget;
			LookAtAlphaBlendTimeToGo	= 0.f;
		}
		else
		{
			LookAtAlpha					+= (BlendDelta / LookAtAlphaBlendTimeToGo) * DeltaSeconds;
			LookAtAlphaBlendTimeToGo	-= DeltaSeconds;
		}
	}
	else
	{
		LookAtAlpha					= LookAtAlphaTarget;
		LookAtAlphaBlendTimeToGo	= 0.f;
	}
}


//
// USkelControlLookAt::GetAffectedBones
//
void USkelControlLookAt::GetAffectedBones(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<INT>& OutBoneIndices)
{
	// Bone is not allowed to rotate, early out
	if( !bAllowRotationX && !bAllowRotationY && !bAllowRotationZ )
	{
		return;
	}

	check(OutBoneIndices.Num() == 0);
	OutBoneIndices.AddItem(BoneIndex);
}

/** 
 * ApplyLookDirectionLimits.  Factored out to allow overriding of limit-enforcing behavior 
 * Returns TRUE if DesiredLookDir was beyond MaxAngle limit.
 */
UBOOL USkelControlLookAt::ApplyLookDirectionLimits(FVector& DesiredLookDir, const FVector &CurrentLookDir, INT BoneIndex, USkeletalMeshComponent* SkelComp)
{
	UBOOL bResult = FALSE;

	// If we have a dead-zone, update the DesiredLookDir.
	FLOAT DeadZoneRadians = 0.f;
	if( DeadZoneAngle > 0.f ) 
	{
		FVector ErrorAxis;
		FLOAT	ErrorAngle;
		FindAxisAndAngle(CurrentLookDir, DesiredLookDir, ErrorAxis, ErrorAngle);

		DeadZoneRadians = DeadZoneAngle * ((FLOAT)PI/180.f);
		FLOAT NewAngle = ::Max( ErrorAngle - DeadZoneRadians, 0.f );
		FQuat UpdateQuat(ErrorAxis, NewAngle);

		DesiredLookDir = UpdateQuat.RotateVector(CurrentLookDir);
	}

	if( bEnableLimit )
	{
		if( bLimitBasedOnRefPose )
		{
			// Calculate transform of bone in ref-pose.
			const INT ParentIndex		= SkelComp->SkeletalMesh->RefSkeleton(BoneIndex).ParentIndex;
			const FMatrix BoneRefPose	= FQuatRotationTranslationMatrix(SkelComp->SkeletalMesh->RefSkeleton(BoneIndex).BonePos.Orientation, SkelComp->SkeletalMesh->RefSkeleton(BoneIndex).BonePos.Position) * SkelComp->SpaceBases(ParentIndex);

			// Calculate ref-pose look dir.
			LimitLookDir = BoneRefPose.TransformNormal(GetAxisDirVector(LookAtAxis, bInvertLookAtAxis));
			LimitLookDir = LimitLookDir.SafeNormal();
		}
		else
		{
			LimitLookDir = CurrentLookDir;
		}

		// Turn into axis and angle.
		FVector ErrorAxis;
		FLOAT	ErrorAngle;
		FindAxisAndAngle(LimitLookDir, DesiredLookDir, ErrorAxis, ErrorAngle);

		// If too great - update.
		const FLOAT MaxAngleRadians = MaxAngle * ((FLOAT)PI/180.f);
		if( ErrorAngle > MaxAngleRadians )
		{
			bResult = TRUE;

			// Going beyond limit, so cancel out controller
			if( bDisableBeyondLimit )
			{
				if( LookAtAlphaTarget > ZERO_ANIMWEIGHT_THRESH )
				{
					if( bNotifyBeyondLimit && SkelComp->GetOwner() != NULL )
					{
						SkelComp->GetOwner()->eventNotifySkelControlBeyondLimit( this );
					}

					SetLookAtAlpha(0.f, BlendOutTime);
				}
			}

			FQuat LookQuat(ErrorAxis, MaxAngleRadians);
			DesiredLookDir = LookQuat.RotateVector(LimitLookDir);
		}
		// Otherwise, if we are within the constraint and dead buffer
		else if( bDisableBeyondLimit && ErrorAngle <= (MaxAngleRadians - DeadZoneRadians) )
		{
			if( LookAtAlphaTarget < 1.f - ZERO_ANIMWEIGHT_THRESH )
			{
				SetLookAtAlpha(1.f, BlendInTime);
			}
		}
	}

	return bResult;
}

//
// USkelControlLookAt::CalculateNewBoneTransforms
//
void USkelControlLookAt::CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms)
{
	check(OutBoneTransforms.Num() == 0);

	// Bone transform in mesh space
	FMatrix NewBoneTM = SkelComp->SpaceBases(BoneIndex);

	// Find the base look direction vector.
	BaseLookDir		= SkelComp->SpaceBases(BoneIndex).TransformNormal( GetAxisDirVector(LookAtAxis, bInvertLookAtAxis) ).SafeNormal();
	// Get bone position (will be in component space).
	BaseBonePos		= SkelComp->SpaceBases(BoneIndex).GetOrigin();
	// Keep track of when BaseLookDir was last updated.
	LastCalcTime	= GWorld->GetWorldInfo()->TimeSeconds;

	// Get target location, in component space.
	const FMatrix ComponentToFrame	= SkelComp->CalcComponentToFrameMatrix(BoneIndex, TargetLocationSpace, TargetSpaceBoneName);
	const FVector TargetCompSpace	= ComponentToFrame.InverseTransformFVector(TargetLocation);

	// Find direction vector we want to look in - again in component space.
	FVector DesiredLookDir	= (TargetCompSpace - BaseBonePos).SafeNormal();

	// Use limits to update DesiredLookDir if desired.
	// We also use these to play with the 'LookAtAlpha'
	ApplyLookDirectionLimits(DesiredLookDir, BaseLookDir, BoneIndex, SkelComp);

	// Below we not touching LookAtAlpha anymore. So if it's zero, there's no point going further.
	if( GetControlAlpha() < ZERO_ANIMWEIGHT_THRESH )
	{
		return;
	}

	// If we are not defining an 'up' axis as well, we calculate the minimum rotation needed to point the axis in 
	// the right direction. This is nice because we still have some animation acting on the roll of the bone.
	if( !bDefineUpAxis )
	{
		// Calculate a quaternion that gets us from our current rotation to the desired one.
		const FQuat DeltaLookQuat = FQuatFindBetween(BaseLookDir, DesiredLookDir);
		const FQuatRotationTranslationMatrix DeltaLookTM( DeltaLookQuat, FVector(0.f) );

		NewBoneTM.SetOrigin( FVector(0.f) );
		NewBoneTM = NewBoneTM * DeltaLookTM;
		NewBoneTM.SetOrigin( BaseBonePos );
	}
	// If we are defining an 'up' axis as well, we can calculate the entire bone transform explicitly.
	else
	{
		if( UpAxis == LookAtAxis )
		{
			debugf( TEXT("USkelControlLookAt (%s): UpAxis and LookAtAxis cannot be the same."), *ControlName.ToString() );
		}

		// Invert look at direction if desired.
		if( bInvertLookAtAxis )
		{
			DesiredLookDir *= -1.f;
		}

		// Calculate 'world up' (+Z) in the component ref frame.
		const FVector UpCompSpace = SkelComp->LocalToWorld.InverseTransformNormal( FVector(0,0,1) );

		// Then calculate our desired up vector using 2 cross products. Probably a more elegant way to do this...
		FVector TempRight = DesiredLookDir ^ UpCompSpace;

		// Handle case when DesiredLookDir ~= (0,0,1) ie looking straight up in this mode.
		if( TempRight.IsNearlyZero() )
		{
			TempRight = FVector(0,1,0);
		}
		else
		{
			TempRight.Normalize();
		}

		FVector DesiredUpDir = TempRight ^ DesiredLookDir;

		// Reverse direction if desired.
		if( bInvertUpAxis )
		{
			DesiredUpDir *= -1.f;
		}

		// Do some sanity checking on vectors before using them to generate vectors.
		if((DesiredLookDir != DesiredUpDir) && (DesiredLookDir | DesiredUpDir) < 0.1f)
		{
			// Then build the bone matrix. 
			NewBoneTM = BuildMatrixFromVectors(LookAtAxis, DesiredLookDir, UpAxis, DesiredUpDir);
			NewBoneTM.SetOrigin( BaseBonePos );
		}
	}

	// See if we should do some per rotation axis filtering
	if( !bAllowRotationX || !bAllowRotationY || !bAllowRotationZ )
	{
		FQuat CompToFrameQuat(SkelComp->CalcComponentToFrameMatrix(BoneIndex, AllowRotationSpace, AllowRotationOtherBoneName));
		FQuat CurrentQuatRot = CompToFrameQuat * FQuat(SkelComp->SpaceBases(BoneIndex));
		FQuat DesiredQuatRot = CompToFrameQuat * FQuat(NewBoneTM);
		CurrentQuatRot.Normalize();
		DesiredQuatRot.Normalize();

		FQuat DeltaQuat = DesiredQuatRot * (-CurrentQuatRot);
		DeltaQuat.Normalize();

		FRotator DeltaRot = FQuatRotationTranslationMatrix(DeltaQuat, FVector(0.f)).Rotator();

		// Filter out any of the Roll (X), Pitch (Y), Yaw (Z) in bone relative space
		if( !bAllowRotationX )
		{
			DeltaRot.Roll	= 0;
		}
		if( !bAllowRotationY )
		{
			DeltaRot.Pitch	= 0;
		}
		if( !bAllowRotationZ )
		{
			DeltaRot.Yaw	= 0;
		}

		// Find new desired rotation
		DesiredQuatRot = DeltaRot.Quaternion() * CurrentQuatRot;
		DesiredQuatRot.Normalize();

		FQuat NewQuat = (-CompToFrameQuat) * DesiredQuatRot;
		NewQuat.Normalize();

		// Turn into new bone position.
		NewBoneTM = FQuatRotationTranslationMatrix(NewQuat, BaseBonePos);
	}

	OutBoneTransforms.AddItem(NewBoneTM);
}


FMatrix USkelControlLookAt::GetWidgetTM(INT WidgetIndex, USkeletalMeshComponent* SkelComp, INT BoneIndex)
{
	check(WidgetIndex == 0);
	FMatrix ComponentToFrame = SkelComp->CalcComponentToFrameMatrix(BoneIndex, TargetLocationSpace, TargetSpaceBoneName);
	FVector WorldLookTarget = SkelComp->LocalToWorld.TransformFVector( ComponentToFrame.InverseTransformFVector(TargetLocation) );

	FMatrix FrameToComponent = ComponentToFrame.Inverse() * SkelComp->LocalToWorld;
	FrameToComponent.SetOrigin(WorldLookTarget);

	return FrameToComponent;
}

void USkelControlLookAt::HandleWidgetDrag(INT WidgetIndex, const FVector& DragVec)
{
	check(WidgetIndex == 0);
	TargetLocation += DragVec;
}

void USkelControlLookAt::DrawSkelControl3D(const FSceneView* View, FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelComp, INT BoneIndex)
{
	const FMatrix ComponentToFrame	= SkelComp->CalcComponentToFrameMatrix(BoneIndex, TargetLocationSpace, TargetSpaceBoneName);
	const FVector WorldLookTarget	= SkelComp->LocalToWorld.TransformFVector( ComponentToFrame.InverseTransformFVector(TargetLocation) );
	const FMatrix TargetTM			= FTranslationMatrix(WorldLookTarget);
	DrawWireDiamond(PDI,TargetTM, CONTROL_DIAMOND_SIZE, FColor(128,255,255), SDPG_Foreground);

	if( bEnableLimit && bShowLimit && SkelComp->SkeletalMesh )
	{
		// Calculate transform for cone.
		FVector YAxis, ZAxis;
		LimitLookDir.FindBestAxisVectors(YAxis, ZAxis);
		const FVector	ConeOrigin		= SkelComp->SpaceBases(BoneIndex).GetOrigin();
		const FLOAT		MaxAngleRadians = MaxAngle * ((FLOAT)PI/180.f);
		const FMatrix	ConeToWorld		= FScaleMatrix(FVector(30.f)) * FMatrix(LimitLookDir, YAxis, ZAxis, ConeOrigin) * SkelComp->LocalToWorld;

		UMaterialInterface* LimitMaterial = LoadObject<UMaterialInterface>(NULL, TEXT("EditorMaterials.PhAT_JointLimitMaterial"), NULL, LOAD_None, NULL);

		DrawCone(PDI,ConeToWorld, MaxAngleRadians, MaxAngleRadians, 40, TRUE, FColor(64,255,64), LimitMaterial->GetRenderProxy(FALSE), SDPG_World);
	}
}


/*-----------------------------------------------------------------------------
	USkelControlSpline
-----------------------------------------------------------------------------*/

//
// USkelControlSpline::GetAffectedBones
//
void USkelControlSpline::GetAffectedBones(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<INT>& OutBoneIndices)
{
	check(OutBoneIndices.Num() == 0);

	if(SplineLength < 2)
	{
		return;
	}

	// The incoming BoneIndex is the 'end' of the spline chain. We need to find the 'start' by walking SplineLength bones up hierarchy.
	// Fail if we walk past the root bone.

	INT WalkBoneIndex = BoneIndex;
	OutBoneIndices.Add(SplineLength); // Allocate output array of bone indices.
	OutBoneIndices(SplineLength-1) = BoneIndex;

	for(INT i=1; i<SplineLength; i++)
	{
		INT OutTransformIndex = SplineLength-(i+1);

		// If we are at the root but still need to move up, chain is too long, so clear the OutBoneIndices array and give up here.
		if(WalkBoneIndex == 0)
		{
			debugf( TEXT("USkelControlSpline : Spling passes root bone of skeleton.") );
			OutBoneIndices.Empty();
			return;
		}
		else
		{
			// Get parent bone.
			WalkBoneIndex = SkelComp->SkeletalMesh->RefSkeleton(WalkBoneIndex).ParentIndex;

			// Insert indices at the start of array, so that parents are before children in the array.
			OutBoneIndices(OutTransformIndex) = WalkBoneIndex;
		}
	}
}

//
// USkelControlSpline::CalculateNewBoneTransforms
//
void USkelControlSpline::CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms)
{
	check(OutBoneTransforms.Num() == 0);

	// Because we do not modify the start or end bone, with a chain less than 2 - nothing is changed!
	if(SplineLength < 2)
	{
		return;
	}

	// We should have checked this is a valid chain in GetAffectedBones, so can assume its ok here.

	UBOOL bPastRoot = false;
	INT StartBoneIndex = BoneIndex;
	for(INT i=0; i<SplineLength; i++)
	{
		check(StartBoneIndex != 0);
		StartBoneIndex = SkelComp->SkeletalMesh->RefSkeleton(StartBoneIndex).ParentIndex;
	}

	FVector StartBonePos = SkelComp->SpaceBases(StartBoneIndex).GetOrigin();
	FVector StartAxisDir = GetAxisDirVector(SplineBoneAxis, bInvertSplineBoneAxis);
	FVector StartBoneTangent = StartSplineTension * SkelComp->SpaceBases(StartBoneIndex).TransformNormal(StartAxisDir);

	FVector EndBonePos = SkelComp->SpaceBases(BoneIndex).GetOrigin();
	FVector EndAxisDir = GetAxisDirVector(SplineBoneAxis, bInvertSplineBoneAxis);
	FVector EndBoneTangent = EndSplineTension * SkelComp->SpaceBases(BoneIndex).TransformNormal(EndAxisDir);

	// Allocate array for output transforms. Final bone transform is not modified by this controlled, so can just copy that straight.
	OutBoneTransforms.Add(SplineLength);
	OutBoneTransforms( SplineLength-1 ) = SkelComp->SpaceBases(BoneIndex);

	INT ModifyBoneIndex = SkelComp->SkeletalMesh->RefSkeleton(BoneIndex).ParentIndex;
	for(INT i=1; i<SplineLength; i++)
	{
		INT OutTransformIndex = SplineLength-(i+1);
		FLOAT Alpha = 1.f - ((FLOAT)i/(FLOAT)SplineLength);

		// Calculate the position for this point on the curve.
		FVector NewBonePos = CubicInterp(StartBonePos, StartBoneTangent, EndBonePos, EndBoneTangent, Alpha);

		// Option that points bones in spline along the spline.
		if(BoneRotMode == SCR_AlongSpline)
		{
			FVector NewBoneDir = CubicInterpDerivative(StartBonePos, StartBoneTangent, EndBonePos, EndBoneTangent, Alpha);
			UBOOL bNonZero = NewBoneDir.Normalize();

			// Only try and correct direction if we get a non-zero tangent.
			if(bNonZero)
			{
				// Calculate the direction that bone is currently pointing.
				FVector CurrentBoneDir = SkelComp->SpaceBases(ModifyBoneIndex).TransformNormal( GetAxisDirVector(SplineBoneAxis, bInvertSplineBoneAxis) );
				CurrentBoneDir = CurrentBoneDir.SafeNormal();

				// Calculate a quaternion that gets us from our current rotation to the desired one.
				FQuat DeltaLookQuat = FQuatFindBetween(CurrentBoneDir, NewBoneDir);
				FQuatRotationTranslationMatrix DeltaLookTM( DeltaLookQuat, FVector(0.f) );

				// Apply to the current bone transform.
				OutBoneTransforms(OutTransformIndex) = SkelComp->SpaceBases(ModifyBoneIndex);
				OutBoneTransforms(OutTransformIndex).SetOrigin( FVector(0.f) );
				OutBoneTransforms(OutTransformIndex) = OutBoneTransforms(OutTransformIndex) * DeltaLookTM;
			}
		}
		// Option that interpolates the rotation of the bone between the start and end rotation.
		else if(SCR_Interpolate)
		{
			FQuat StartBoneQuat( SkelComp->SpaceBases(StartBoneIndex) );
			FQuat EndBoneQuat( SkelComp->SpaceBases(BoneIndex) );

			FQuat NewBoneQuat = SlerpQuat( StartBoneQuat, EndBoneQuat, Alpha );

			OutBoneTransforms(OutTransformIndex) = FQuatRotationTranslationMatrix( NewBoneQuat, FVector(0.f) );
		}

		OutBoneTransforms(OutTransformIndex).SetOrigin(NewBonePos);

		ModifyBoneIndex = SkelComp->SkeletalMesh->RefSkeleton(ModifyBoneIndex).ParentIndex;
	}
}

/*-----------------------------------------------------------------------------
	USkelControlLimb
-----------------------------------------------------------------------------*/

//
// USkelControlLimb::GetAffectedBones
//
void USkelControlLimb::GetAffectedBones(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<INT>& OutBoneIndices)
{
	check(OutBoneIndices.Num() == 0);

	// Get indices of the lower and upper limb bones.
	UBOOL bInvalidLimb = FALSE;
	if( BoneIndex == 0 )
	{
		bInvalidLimb = TRUE;
	}

	const INT LowerLimbIndex = SkelComp->SkeletalMesh->RefSkeleton(BoneIndex).ParentIndex;
	if( LowerLimbIndex == 0 )
	{
		bInvalidLimb = TRUE;
	}

	const INT UpperLimbIndex = SkelComp->SkeletalMesh->RefSkeleton(LowerLimbIndex).ParentIndex;

	// If we walked past the root, this controlled is invalid, so return no affected bones.
	if( bInvalidLimb )
	{
		debugf( TEXT("USkelControlLimb : Cannot find 2 bones above controlled bone. Too close to root.") );
		return;
	}
	else
	{
		OutBoneIndices.Add(3);
		OutBoneIndices(0) = UpperLimbIndex;
		OutBoneIndices(1) = LowerLimbIndex;
		OutBoneIndices(2) = BoneIndex;
	}
}


//
// USkelControlSpline::CalculateNewBoneTransforms
//
void USkelControlLimb::CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms)
{
	check(OutBoneTransforms.Num() == 0);
	OutBoneTransforms.Add(3); // Allocate space for bone transforms.

	// First get indices of the lower and upper limb bones. We should have checked this in GetAffectedBones so can assume its ok now.

	check(BoneIndex != 0);
	const INT LowerLimbIndex = SkelComp->SkeletalMesh->RefSkeleton(BoneIndex).ParentIndex;
	check(LowerLimbIndex != 0);
	const INT UpperLimbIndex = SkelComp->SkeletalMesh->RefSkeleton(LowerLimbIndex).ParentIndex;

	// If we have enough bones to work on (ie at least 2 bones between controlled hand bone and the root) continue.

	// Get current position of root of limb.
	// All position are in Component space.
	const FVector RootPos			= SkelComp->SpaceBases(UpperLimbIndex).GetOrigin();
	const FVector InitialJointPos	= SkelComp->SpaceBases(LowerLimbIndex).GetOrigin();
	const FVector InitialEndPos		= SkelComp->SpaceBases(BoneIndex).GetOrigin();

	// If desired, calc the initial relative transform between the end effector bone and its parent.
	FMatrix EffectorRelTM = FMatrix::Identity;
	if( bMaintainEffectorRelRot ) 
	{
		EffectorRelTM = SkelComp->SpaceBases(BoneIndex) * SkelComp->SpaceBases(LowerLimbIndex).Inverse();
	}

	// Get desired position of effector.
	FMatrix			DesiredComponentToFrame	= SkelComp->CalcComponentToFrameMatrix(BoneIndex, EffectorLocationSpace, EffectorSpaceBoneName);
	const FVector	DesiredPos				= DesiredComponentToFrame.InverseTransformFVector(EffectorLocation);

	const FVector	DesiredDelta		= DesiredPos - RootPos;
	FLOAT			DesiredLength		= DesiredDelta.Size();
	FVector			DesiredDir;

	// Check to handle case where DesiredPos is the same as RootPos.
	if( DesiredLength < (FLOAT)KINDA_SMALL_NUMBER )
	{
		DesiredLength	= (FLOAT)KINDA_SMALL_NUMBER;
		DesiredDir		= FVector(1,0,0);
	}
	else
	{
		DesiredDir		= DesiredDelta/DesiredLength;
	}

	// Get joint target (used for defining plane that joint should be in).
	FMatrix			JointTargetComponentToFrame	= SkelComp->CalcComponentToFrameMatrix(BoneIndex, JointTargetLocationSpace, JointTargetSpaceBoneName);
	const FVector	JointTargetPos				= JointTargetComponentToFrame.InverseTransformFVector(JointTargetLocation);

	const FVector	JointTargetDelta	= JointTargetPos - RootPos;
	const FLOAT		JointTargetLength	= JointTargetDelta.Size();
	FVector JointPlaneNormal, JointBendDir;

	// Same check as above, to cover case when JointTarget position is the same as RootPos.
	if( JointTargetLength < (FLOAT)KINDA_SMALL_NUMBER )
	{
		JointBendDir		= FVector(0,1,0);
		JointPlaneNormal	= FVector(0,0,1);
	}
	else
	{
		JointPlaneNormal = DesiredDir ^ JointTargetDelta;

		// If we are trying to point the limb in the same direction that we are supposed to displace the joint in, 
		// we have to just pick 2 random vector perp to DesiredDir and each other.
		if( JointPlaneNormal.Size() < (FLOAT)KINDA_SMALL_NUMBER )
		{
			DesiredDir.FindBestAxisVectors(JointPlaneNormal, JointBendDir);
		}
		else
		{
			JointPlaneNormal.Normalize();

			// Find the final member of the reference frame by removing any component of JointTargetDelta along DesiredDir.
			// This should never leave a zero vector, because we've checked DesiredDir and JointTargetDelta are not parallel.
			JointBendDir = JointTargetDelta - ((JointTargetDelta | DesiredDir) * DesiredDir);
			JointBendDir.Normalize();
		}
	}

	// Find lengths of upper and lower limb in the ref skeleton.
	const FLOAT LowerLimbLength = SkelComp->SkeletalMesh->RefSkeleton(BoneIndex).BonePos.Position.Size();
	const FLOAT UpperLimbLength = SkelComp->SkeletalMesh->RefSkeleton(LowerLimbIndex).BonePos.Position.Size();
	const FLOAT MaxLimbLength	= LowerLimbLength + UpperLimbLength;

	FVector OutEndPos	= DesiredPos;
	FVector OutJointPos = InitialJointPos;

	// If we are trying to reach a goal beyond the length of the limb, clamp it to something solvable and extend limb fully.
	if( DesiredLength > MaxLimbLength )
	{
		OutEndPos	= RootPos + (MaxLimbLength * DesiredDir);
		OutJointPos = RootPos + (UpperLimbLength * DesiredDir);
	}
	else
	{
		// So we have a triangle we know the side lengths of. We can work out the angle between DesiredDir and the direction of the upper limb
		// using the sin rule:
		const FLOAT CosAngle = ((UpperLimbLength*UpperLimbLength) + (DesiredLength*DesiredLength) - (LowerLimbLength*LowerLimbLength))/(2 * UpperLimbLength * DesiredLength);

		// If CosAngle is less than 0, the upper arm actually points the opposite way to DesiredDir, so we handle that.
		const UBOOL bReverseUpperBone = (CosAngle < 0.f);

		// If CosAngle is greater than 1.f, the triangle could not be made - we cannot reach the target.
		// We just have the two limbs double back on themselves, and EndPos will not equal the desired EffectorLocation.
		if( CosAngle > 1.f || CosAngle < -1.f )
		{
			// Because we want the effector to be a positive distance down DesiredDir, we go back by the smaller section.
			if( UpperLimbLength > LowerLimbLength )
			{
				OutJointPos = RootPos + (UpperLimbLength * DesiredDir);
				OutEndPos	= OutJointPos - (LowerLimbLength * DesiredDir);
			}
			else
			{
				OutJointPos = RootPos - (UpperLimbLength * DesiredDir);
				OutEndPos	= OutJointPos + (LowerLimbLength * DesiredDir);
			}
		}
		else
		{
			// Angle between upper limb and DesiredDir
			const FLOAT Angle = appAcos(CosAngle);

			// Now we calculate the distance of the joint from the root -> effector line.
			// This forms a right-angle triangle, with the upper limb as the hypotenuse.
			const FLOAT JointLineDist = UpperLimbLength * appSin(Angle);

			// And the final side of that triangle - distance along DesiredDir of perpendicular.
			// ProjJointDistSqr can't be neg, because JointLineDist must be <= UpperLimbLength because appSin(Angle) is <= 1.
			const FLOAT ProjJointDistSqr	= (UpperLimbLength*UpperLimbLength) - (JointLineDist*JointLineDist);
			FLOAT		ProjJointDist		= appSqrt(ProjJointDistSqr);
			if( bReverseUpperBone )
			{
				ProjJointDist *= -1.f;
			}

			// So now we can work out where to put the joint!
			OutJointPos = RootPos + (ProjJointDist * DesiredDir) + (JointLineDist * JointBendDir);
		}
	}

	// Update transform for upper bone.
	FVector GraphicJointDir = JointPlaneNormal;
	if( bInvertJointAxis )
	{
		GraphicJointDir *= -1.f;
	}

	FVector UpperLimbDir = (OutJointPos - RootPos).SafeNormal();
	if( bInvertBoneAxis )
	{
		UpperLimbDir *= -1.f;
	}

	// Do some sanity checking, then use vectors to build upper limb matrix
	if((UpperLimbDir != GraphicJointDir) && (UpperLimbDir | GraphicJointDir) < 0.1f)
	{
		FMatrix UpperLimbTM = BuildMatrixFromVectors(BoneAxis, UpperLimbDir, JointAxis, GraphicJointDir);
		UpperLimbTM.SetOrigin(RootPos);
		OutBoneTransforms(0) = UpperLimbTM;
	}
	else
	{
		OutBoneTransforms(0) = SkelComp->SpaceBases(UpperLimbIndex);
	}

	// Update transform for lower bone.
	FVector LowerLimbDir = (OutEndPos - OutJointPos).SafeNormal();
	if( bInvertBoneAxis )
	{
		LowerLimbDir *= -1.f;
	}

	// Do some sanity checking, then use vectors to build lower limb matrix
	if((LowerLimbDir != GraphicJointDir) && (LowerLimbDir | GraphicJointDir) < 0.1f)
	{
		FMatrix LowerLimbTM = BuildMatrixFromVectors(BoneAxis, LowerLimbDir, JointAxis, GraphicJointDir);
		LowerLimbTM.SetOrigin(OutJointPos);
		OutBoneTransforms(1) = LowerLimbTM;
	}
	else
	{
		OutBoneTransforms(1) = SkelComp->SpaceBases(LowerLimbIndex);
	}

	// Update transform for end bone.
	if( bTakeRotationFromEffectorSpace )
	{
		OutBoneTransforms(2) = SkelComp->SpaceBases(BoneIndex);
		FMatrix FrameToComponent = DesiredComponentToFrame.Inverse();
		CopyRotationPart(OutBoneTransforms(2), FrameToComponent);
	}
	else if( bMaintainEffectorRelRot )
	{
		OutBoneTransforms(2) = EffectorRelTM * OutBoneTransforms(1);
	}
	else
	{
		OutBoneTransforms(2) = SkelComp->SpaceBases(BoneIndex);
	}
	OutBoneTransforms(2).SetOrigin(OutEndPos);
}

INT USkelControlLimb::GetWidgetCount()
{
	return 2;
}

FMatrix USkelControlLimb::GetWidgetTM(INT WidgetIndex, USkeletalMeshComponent* SkelComp, INT BoneIndex)
{
	check(WidgetIndex < 2);

	FMatrix ComponentToFrame;
	FVector ComponentLoc;
	if(WidgetIndex == 0)
	{
		ComponentToFrame = SkelComp->CalcComponentToFrameMatrix(BoneIndex, EffectorLocationSpace, EffectorSpaceBoneName);
		ComponentLoc = SkelComp->LocalToWorld.TransformFVector( ComponentToFrame.InverseTransformFVector(EffectorLocation) );
	}
	else
	{
		ComponentToFrame = SkelComp->CalcComponentToFrameMatrix(BoneIndex, JointTargetLocationSpace, JointTargetSpaceBoneName);
		ComponentLoc = SkelComp->LocalToWorld.TransformFVector( ComponentToFrame.InverseTransformFVector(JointTargetLocation) );
	}

	FMatrix FrameToComponent = ComponentToFrame.Inverse() * SkelComp->LocalToWorld;
	FrameToComponent.SetOrigin(ComponentLoc);

	return FrameToComponent;
}

void USkelControlLimb::HandleWidgetDrag(INT WidgetIndex, const FVector& DragVec)
{
	check(WidgetIndex < 2);

	if(WidgetIndex == 0)
	{
		EffectorLocation += DragVec;
	}
	else
	{
		JointTargetLocation += DragVec;
	}
}

void USkelControlLimb::DrawSkelControl3D(const FSceneView* View, FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelComp, INT BoneIndex)
{
	FMatrix ComponentToFrame = SkelComp->CalcComponentToFrameMatrix(BoneIndex, EffectorLocationSpace, EffectorSpaceBoneName);
	FVector ComponentLoc = SkelComp->LocalToWorld.TransformFVector( ComponentToFrame.InverseTransformFVector(EffectorLocation) );
	FMatrix DiamondTM = FTranslationMatrix(ComponentLoc);
	DrawWireDiamond(PDI, DiamondTM, CONTROL_DIAMOND_SIZE, FColor(128,128,255), SDPG_Foreground );

	ComponentToFrame = SkelComp->CalcComponentToFrameMatrix(BoneIndex, JointTargetLocationSpace, JointTargetSpaceBoneName);
	ComponentLoc = SkelComp->LocalToWorld.TransformFVector( ComponentToFrame.InverseTransformFVector(JointTargetLocation) );
	DiamondTM = FTranslationMatrix(ComponentLoc);
	DrawWireDiamond(PDI, DiamondTM, CONTROL_DIAMOND_SIZE, FColor(255,128,128), SDPG_Foreground );
}


/*-----------------------------------------------------------------------------
	USkelControlFootPlacement
-----------------------------------------------------------------------------*/
//
// USkelControlFootPlacement::CalculateNewBoneTransforms
//
void USkelControlFootPlacement::CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms)
{
	// First get indices of the lower and upper limb bones. 
	// We should have checked this in GetAffectedBones so can assume its ok now.

	check(BoneIndex != 0);
	INT LowerLimbIndex = SkelComp->SkeletalMesh->RefSkeleton(BoneIndex).ParentIndex;
	check(LowerLimbIndex != 0);
	INT UpperLimbIndex = SkelComp->SkeletalMesh->RefSkeleton(LowerLimbIndex).ParentIndex;

	// Find the root and end position in world space.
	FVector RootPos = SkelComp->SpaceBases(UpperLimbIndex).GetOrigin();
	FVector WorldRootPos = SkelComp->LocalToWorld.TransformFVector(RootPos);

	FVector EndPos = SkelComp->SpaceBases(BoneIndex).GetOrigin();
	FVector WorldEndPos = SkelComp->LocalToWorld.TransformFVector(EndPos);

	FVector LegDelta = WorldEndPos - WorldRootPos;
	FVector LegDir = LegDelta.SafeNormal();

	// We do a hack here - extend length of line by 100 - to get around Unreals nasty line check fudging.
	FVector CheckEndPos = WorldEndPos + (100.f + FootOffset + MaxDownAdjustment) * LegDir;

	FVector HitLocation, HitNormal;
	UBOOL bHit = SkelComp->LegLineCheck( WorldRootPos, CheckEndPos, HitLocation, HitNormal);

	FLOAT LegAdjust = 0.f;
	if(bHit)
	{
		// Find how much we are adjusting the foot up or down. Postive is down, negative is up.
		LegAdjust = ((HitLocation - WorldEndPos) | LegDir);

		// Reject hits in the 100-unit dead region.
		if( bHit && LegAdjust > (FootOffset + MaxDownAdjustment) )
		{
			bHit = false;
		}
	}

	FVector DesiredPos;
	if(bHit)
	{
		LegAdjust -= FootOffset;

		// Clamp LegAdjust between MaxUp/DownAdjustment.
		LegAdjust = ::Clamp(LegAdjust, -MaxUpAdjustment, MaxDownAdjustment);

		// If bOnlyEnableForUpAdjustment is true, do nothing if we are not adjusting the leg up.
		if(bOnlyEnableForUpAdjustment && LegAdjust >= 0.f)
		{
			return;
		}

		// ..and calculate EffectorLocation.
		DesiredPos = WorldEndPos + (LegAdjust * LegDir);
	}
	else
	{
		if(bOnlyEnableForUpAdjustment)
		{
			return;
		}	

		// No hit - we reach as far as MaxDownAdjustment will allow.
		DesiredPos = WorldEndPos + (MaxDownAdjustment * LegDir);
	}

	EffectorLocation = DesiredPos;
	EffectorLocationSpace = BCS_WorldSpace;

	Super::CalculateNewBoneTransforms(BoneIndex, SkelComp, OutBoneTransforms);

	check(OutBoneTransforms.Num() == 3);

	// OutBoneTransforms(2) will be the transform for the foot here.
	FVector FootPos = (OutBoneTransforms(2) * SkelComp->LocalToWorld).GetOrigin();	

	// Now we orient the foot if desired, and its sufficiently close to our desired position.
	if(bOrientFootToGround && bHit && ((FootPos - DesiredPos).Size() < 1.0f))
	{
		// Find reference frame we are trying to orient to the ground. Its the foot bone transform, with the FootRotOffset applied.
		FMatrix BoneRefMatrix = FRotationMatrix(FootRotOffset) * OutBoneTransforms(2);

		// Find the 'up' vector of that reference frame, in component space.
		FVector CurrentFootDir = BoneRefMatrix.TransformNormal( GetAxisDirVector(FootUpAxis, bInvertFootUpAxis) );
		CurrentFootDir = CurrentFootDir.SafeNormal();	

		// Transform hit normal into component space.
		FVector NormalCompSpace = SkelComp->LocalToWorld.InverseTransformNormal(HitNormal);

		// Calculate a quaternion that gets us from our current rotation to the desired one.
		FQuat DeltaFootQuat = FQuatFindBetween(CurrentFootDir, NormalCompSpace);

		// Limit the maximum amount we are going to correct by to 'MaxFootOrientAdjust'
		FLOAT MaxFootOrientRad = MaxFootOrientAdjust * ((FLOAT)PI/180.f);
		FVector DeltaFootAxis;
		FLOAT DeltaFootAng;
		DeltaFootQuat.ToAxisAndAngle(DeltaFootAxis, DeltaFootAng);
		DeltaFootAng = ::Clamp(DeltaFootAng, -MaxFootOrientRad, MaxFootOrientRad);
		DeltaFootQuat = FQuat(DeltaFootAxis, DeltaFootAng);

		// Convert from quaternion to rotation matrix.
		FQuatRotationTranslationMatrix DeltaFootTM( DeltaFootQuat, FVector(0.f) );

		// Apply rotation matrix to current foot matrix.
		FVector FootBonePos = OutBoneTransforms(2).GetOrigin();
		OutBoneTransforms(2).SetOrigin( FVector(0.f) );
		OutBoneTransforms(2) = OutBoneTransforms(2) * DeltaFootTM;
		OutBoneTransforms(2).SetOrigin( FootBonePos );
	}
}

/*-----------------------------------------------------------------------------
	USkelControlWheel
-----------------------------------------------------------------------------*/

//
// USkelControlWheel::UpdateWheelControl
//
void USkelControlWheel::UpdateWheelControl( FLOAT InDisplacement, FLOAT InRoll, FLOAT InSteering )
{
	WheelDisplacement = InDisplacement;
	WheelRoll = InRoll;
	WheelSteering = InSteering;
}

//
// USkelControlWheel::CalculateNewBoneTransforms
//
void USkelControlWheel::CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms)
{
	FVector TotalScale = SkelComp->Scale * SkelComp->Scale3D;
	if (SkelComp->GetOwner() != NULL)
	{
		TotalScale *= SkelComp->GetOwner()->DrawScale * SkelComp->GetOwner()->DrawScale3D;
	}

	FLOAT RenderDisplacement = ::Min(WheelDisplacement/TotalScale.X, WheelMaxRenderDisplacement);
	BoneTranslation = RenderDisplacement * FVector(0,0,1);
	
	FVector RollAxis = GetAxisDirVector(WheelRollAxis, bInvertWheelRoll);
	FVector SteerAxis = GetAxisDirVector(WheelSteeringAxis, bInvertWheelSteering);

	FQuat TotalRot = FQuat(SteerAxis, WheelSteering * ((FLOAT)PI/180.f)) * FQuat(RollAxis, WheelRoll * ((FLOAT)PI/180.f));
	BoneRotation = FQuat(TotalRot);

	Super::CalculateNewBoneTransforms(BoneIndex, SkelComp, OutBoneTransforms);
}

/*-----------------------------------------------------------------------------
	USkelControlTrail
-----------------------------------------------------------------------------*/

void USkelControlTrail::TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp)
{
	Super::TickSkelControl(DeltaSeconds, SkelComp);

	ThisTimstep = DeltaSeconds;
}

void USkelControlTrail::GetAffectedBones(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<INT>& OutBoneIndices)
{
	check(OutBoneIndices.Num() == 0);

	if( ChainLength < 2 )
	{
		return;
	}

	// The incoming BoneIndex is the 'end' of the spline chain. We need to find the 'start' by walking SplineLength bones up hierarchy.
	// Fail if we walk past the root bone.

	INT WalkBoneIndex = BoneIndex;
	OutBoneIndices.Add(ChainLength); // Allocate output array of bone indices.
	OutBoneIndices(ChainLength-1) = BoneIndex;

	for(INT i=1; i<ChainLength; i++)
	{
		INT OutTransformIndex = ChainLength-(i+1);

		// If we are at the root but still need to move up, chain is too long, so clear the OutBoneIndices array and give up here.
		if(WalkBoneIndex == 0)
		{
			debugf( TEXT("UWarSkelCtrl_Trail : Spling passes root bone of skeleton.") );
			OutBoneIndices.Empty();
			return;
		}
		else
		{
			// Get parent bone.
			WalkBoneIndex = SkelComp->SkeletalMesh->RefSkeleton(WalkBoneIndex).ParentIndex;

			// Insert indices at the start of array, so that parents are before children in the array.
			OutBoneIndices(OutTransformIndex) = WalkBoneIndex;
		}
	}
}

void USkelControlTrail::CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms)
{
	check(OutBoneTransforms.Num() == 0);

	if( ChainLength < 2 )
	{
		return;
	}

	OutBoneTransforms.Add(ChainLength);

	// Build array of bone indices - starting at highest bone and running down to end of chain (where controller is)
	// Same code as in GetAffectedBones above!
	TArray<INT> ChainBoneIndices;
	INT WalkBoneIndex = BoneIndex;
	ChainBoneIndices.Add(ChainLength); // Allocate output array of bone indices.
	ChainBoneIndices(ChainLength-1) = BoneIndex;
	for(INT i=1; i<ChainLength; i++)
	{
		check(WalkBoneIndex != 0);

		// Get parent bone.
		WalkBoneIndex = SkelComp->SkeletalMesh->RefSkeleton(WalkBoneIndex).ParentIndex;

		// Insert indices at the start of array, so that parents are before children in the array.
		INT OutTransformIndex = ChainLength-(i+1);
		ChainBoneIndices(OutTransformIndex) = WalkBoneIndex;
	}

	// If we have >0 this frame, but didn't last time, record positions of all the bones.
	// Also do this if number has changed or array is zero.
	UBOOL bHasValidStrength = (ControlStrength > 0.f);
	if(TrailBoneLocations.Num() != ChainLength || (bHasValidStrength && !bHadValidStrength))
	{
		TrailBoneLocations.Empty();
		TrailBoneLocations.Add(ChainLength);

		for(INT i=0; i<ChainBoneIndices.Num(); i++)
		{
			INT ChildIndex = ChainBoneIndices(i);
			TrailBoneLocations(i) = (SkelComp->SpaceBases(ChildIndex) * SkelComp->LocalToWorld).GetOrigin();
		}
	}
	bHadValidStrength = bHasValidStrength;

	// Root bone of trail is not modified.
	INT RootIndex = ChainBoneIndices(0); 
	OutBoneTransforms(0)	= SkelComp->SpaceBases(RootIndex); // Local space matrix
	TrailBoneLocations(0)	= (OutBoneTransforms(0) * SkelComp->LocalToWorld).GetOrigin(); // World space location

	// Starting one below head of chain, move bones.
	for(INT i=1; i<ChainBoneIndices.Num(); i++)
	{
		// Parent bone position in world space.
		INT ParentIndex = ChainBoneIndices(i-1);
		FVector ParentPos = TrailBoneLocations(i-1);
		FVector ParentAnimPos = (SkelComp->SpaceBases(ParentIndex) * SkelComp->LocalToWorld).GetOrigin();

		// Child bone position in world space.
		INT ChildIndex = ChainBoneIndices(i);
		FVector ChildPos = TrailBoneLocations(i);
		FVector ChildAnimPos = (SkelComp->SpaceBases(ChildIndex) * SkelComp->LocalToWorld).GetOrigin();

		// Desired parent->child offset.
		FVector TargetDelta = (ChildAnimPos - ParentAnimPos);

		// Desired child position.
		FVector ChildTarget = ParentPos + TargetDelta;

		// Find vector from child to target
		FVector Error = ChildTarget - ChildPos;

		// Calculate how much to push the child towards its target
		FLOAT Correction = Clamp(ThisTimstep * TrailRelaxation, 0.f, 1.f);
		//FLOAT Correction = Clamp(TrailRelaxation, 0.f, 1.f);

		// Scale correction vector and apply to get new world-space child position.
		TrailBoneLocations(i) = ChildPos + (Error * Correction);

		// If desired, prevent bones stretching too far.
		if(bLimitStretch)
		{
			FLOAT RefPoseLength = TargetDelta.Size();
			FVector CurrentDelta = TrailBoneLocations(i) - TrailBoneLocations(i-1);
			FLOAT CurrentLength = CurrentDelta.Size();

			// If we are too far - cut it back (just project towards parent particle).
			if(CurrentLength - RefPoseLength > StretchLimit)
			{
				FVector CurrentDir = CurrentDelta / CurrentLength;
				TrailBoneLocations(i) = TrailBoneLocations(i-1) + (CurrentDir * (RefPoseLength + StretchLimit));
			}
		}

		// Modify child matrix
		OutBoneTransforms(i) = SkelComp->SpaceBases(ChildIndex);
		OutBoneTransforms(i).SetOrigin( SkelComp->LocalToWorld.Inverse().TransformFVector( TrailBoneLocations(i) ) );

		// Modify rotation of parent matrix to point at this one.

		// Calculate the direction that parent bone is currently pointing.
		FVector CurrentBoneDir = OutBoneTransforms(i-1).TransformNormal( GetAxisDirVector(ChainBoneAxis, bInvertChainBoneAxis) );
		CurrentBoneDir = CurrentBoneDir.SafeNormal();

		// Calculate vector from parent to child.
		FVector NewBoneDir = (OutBoneTransforms(i).GetOrigin() - OutBoneTransforms(i-1).GetOrigin()).SafeNormal();

		// Calculate a quaternion that gets us from our current rotation to the desired one.
		FQuat DeltaLookQuat = FQuatFindBetween(CurrentBoneDir, NewBoneDir);
		FQuatRotationTranslationMatrix DeltaTM( DeltaLookQuat, FVector(0.f) );

		// Apply to the current parent bone transform.
		FMatrix TmpMatrix = FMatrix::Identity;
		CopyRotationPart(TmpMatrix, OutBoneTransforms(i-1));
		TmpMatrix = TmpMatrix * DeltaTM;
		CopyRotationPart(OutBoneTransforms(i-1), TmpMatrix);
	}

	// For the last bone in the chain, use the rotation from the bone above it.
	CopyRotationPart(OutBoneTransforms(ChainLength-1), OutBoneTransforms(ChainLength-2));
}

