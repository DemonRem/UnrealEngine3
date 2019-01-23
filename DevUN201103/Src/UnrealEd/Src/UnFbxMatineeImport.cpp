/*
* Copyright 2010 Autodesk, Inc.  All Rights Reserved.
*
* Permission to use, copy, modify, and distribute this software in object
* code form for any purpose and without fee is hereby granted, provided
* that the above copyright notice appears in all copies and that both
* that copyright notice and the limited warranty and restricted rights
* notice below appear in all supporting documentation.
*
* AUTODESK PROVIDES THIS PROGRAM "AS IS" AND WITH ALL FAULTS.
* AUTODESK SPECIFICALLY DISCLAIMS ANY AND ALL WARRANTIES, WHETHER EXPRESS
* OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED WARRANTY
* OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR USE OR NON-INFRINGEMENT
* OF THIRD PARTY RIGHTS.  AUTODESK DOES NOT WARRANT THAT THE OPERATION
* OF THE PROGRAM WILL BE UNINTERRUPTED OR ERROR FREE.
*
* In no event shall Autodesk, Inc. be liable for any direct, indirect,
* incidental, special, exemplary, or consequential damages (including,
* but not limited to, procurement of substitute goods or services;
* loss of use, data, or profits; or business interruption) however caused
* and on any theory of liability, whether in contract, strict liability,
* or tort (including negligence or otherwise) arising in any way out
* of such code.
*
* This software is provided to the U.S. Government with the same rights
* and restrictions as described herein.
*/

/*=============================================================================
	FBX importer for Unreal Engine 3.
==============================================================================*/

#include "UnrealEd.h"

#if WITH_FBX

#include "EngineAnimClasses.h"
#include "EngineInterpolationClasses.h"
#include "EngineSequenceClasses.h"
#include "UnLinkedObjDrawUtils.h"
#include "UnFbxImporter.h"

namespace UnFbx {

/**
 * Retrieves whether there are any unknown camera instances within the FBX document that the camera is not in Unreal scene.
 */
inline UBOOL _HasUnknownCameras( USeqAct_Interp* MatineeSequence, KFbxNode* FbxNode, const TCHAR* Name )
{
	KFbxNodeAttribute* Attr = FbxNode->GetNodeAttribute();
	if (Attr && Attr->GetAttributeType() == KFbxNodeAttribute::eCAMERA)
	{
		// If we have a Matinee, try to name-match the node with a Matinee group name
		if( MatineeSequence != NULL && MatineeSequence->InterpData != NULL )
		{
			UInterpGroupInst* GroupInst = MatineeSequence->FindFirstGroupInstByName( FString( Name ) );
			if( GroupInst != NULL )
			{
				AActor * GrActor = GroupInst->GetGroupActor();
				// Make sure we have an actor
				if( GrActor != NULL &&
					GrActor->IsA( ACameraActor::StaticClass() ) )
				{
					// OK, we found an existing camera!
					return FALSE;
				}
			}
		}
		
		// Attempt to name-match the scene node for this camera with one of the actors.
		AActor* Actor = FindObject<AActor>( ANY_PACKAGE, Name );
		if ( Actor == NULL || Actor->bDeleteMe )
		{
			return TRUE;
		}
		else
		{
			// If you trigger this assertion, then you've got a name
			// clash between the FBX file and the UE3 level.
			check( Actor->IsA( ACameraActor::StaticClass() ) );
		}
	}
	
	return FALSE;
}

UBOOL CFbxImporter::HasUnknownCameras( USeqAct_Interp* MatineeSequence ) const
{
	if ( FbxScene == NULL )
	{
		return FALSE;
	}

	// check recursively
	KFbxNode* RootNode = FbxScene->GetRootNode();
	INT NodeCount = RootNode->GetChildCount();
	for ( INT NodeIndex = 0; NodeIndex < NodeCount; ++NodeIndex )
	{
		KFbxNode* FbxNode = RootNode->GetChild(NodeIndex);
		if ( _HasUnknownCameras( MatineeSequence, FbxNode, ANSI_TO_TCHAR(FbxNode->GetName()) ) )
		{
			return TRUE;
		}

		// Look through children as well
		INT ChildNodeCount = FbxNode->GetChildCount();
		for( INT ChildIndex = 0; ChildIndex < ChildNodeCount; ++ChildIndex )
		{
			KFbxNode* ChildNode = FbxNode->GetChild(ChildIndex);
			if( _HasUnknownCameras( MatineeSequence, ChildNode, ANSI_TO_TCHAR(ChildNode->GetName() ) ) )
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}

UBOOL CFbxImporter::IsNodeAnimated(KFbxNode* FbxNode, KFbxAnimLayer* AnimLayer)
{
	if (!AnimLayer)
	{
		KFbxAnimStack* AnimStack = FbxScene->GetMember(FBX_TYPE(KFbxAnimStack), 0);
		if (!AnimStack) return FALSE;

		AnimLayer = AnimStack->GetMember(FBX_TYPE(KFbxAnimLayer), 0);
		if (AnimLayer == NULL) return FALSE;
	}
	
	// verify that the node is animated.
	UBOOL bIsAnimated = FALSE;
	KTime Start, Stop;

	// translation animation
	KFbxProperty TransProp = FbxNode->LclTranslation;
	for (INT i = 0; i < TransProp.GetSrcObjectCount(FBX_TYPE(KFbxAnimCurveNode)); i++)
	{
		KFbxAnimCurveNode* CurveNode = KFbxCast<KFbxAnimCurveNode>(TransProp.GetSrcObject(KFbxAnimCurveNode::ClassId, i));
		if (CurveNode && AnimLayer->IsConnectedSrcObject(CurveNode))
		{
			bIsAnimated |= (UBOOL)CurveNode->GetAnimationInterval(Start, Stop);
			break;
		}
	}
	// rotation animation
	KFbxProperty RotProp = FbxNode->LclRotation;
	for (INT i = 0; IsAnimated == FALSE && i < RotProp.GetSrcObjectCount(FBX_TYPE(KFbxAnimCurveNode)); i++)
	{
		KFbxAnimCurveNode* CurveNode = KFbxCast<KFbxAnimCurveNode>(RotProp.GetSrcObject(KFbxAnimCurveNode::ClassId, i));
		if (CurveNode && AnimLayer->IsConnectedSrcObject(CurveNode))
		{
			bIsAnimated |= (UBOOL)CurveNode->GetAnimationInterval(Start, Stop);
		}
	}
	
	return bIsAnimated;
}

/** 
 * Finds a camera in the passed in node or any child nodes 
 * @return NULL if the camera is not found, a valid pointer if it is
 */
static KFbxCamera* FindCamera( KFbxNode* Parent )
{
	KFbxCamera* Camera = Parent->GetCamera();
	if( !Camera )
	{
		INT NodeCount = Parent->GetChildCount();
		for ( INT NodeIndex = 0; NodeIndex < NodeCount && !Camera; ++NodeIndex )
		{
			KFbxNode* Child = Parent->GetChild( NodeIndex );
			Camera = Child->GetCamera();
		}
	}

	return Camera;
}

void CFbxImporter::ImportMatineeSequence(USeqAct_Interp* MatineeSequence)
{
	if (FbxScene == NULL || MatineeSequence == NULL) return;
	
	// merge animation layer at first
	KFbxAnimStack* AnimStack = FbxScene->GetMember(FBX_TYPE(KFbxAnimStack), 0);
	if (!AnimStack) return;
		
	MergeAllLayerAnimation(AnimStack, KTime::GetFrameRate(FbxScene->GetGlobalSettings().GetTimeMode()));

	KFbxAnimLayer* AnimLayer = AnimStack->GetMember(FBX_TYPE(KFbxAnimLayer), 0);
	if (AnimLayer == NULL) return;

	// If the Matinee editor is not open, we need to initialize the sequence.
	UBOOL InitializeMatinee = MatineeSequence->InterpData == NULL;
	if (InitializeMatinee)
	{
		// Force the initialization of the sequence
		// This sets the sequence in edition mode as well?
		MatineeSequence->InitInterp();
	}

	UInterpData* MatineeData = MatineeSequence->InterpData;
	FLOAT InterpLength = -1.0f;

	KFbxNode* RootNode = FbxScene->GetRootNode();
	INT NodeCount = RootNode->GetChildCount();
	for (INT NodeIndex = 0; NodeIndex < NodeCount; ++NodeIndex)
	{
		KFbxNode* FbxNode = RootNode->GetChild(NodeIndex);

		AActor* Actor = NULL;

		// First check to see if the scene node name matches a Matinee group name
		UInterpGroupInst* FoundGroupInst =
			MatineeSequence->FindFirstGroupInstByName( FString( FbxNode->GetName() ) );
		if( FoundGroupInst != NULL )
		{
			// OK, we found an actor bound to a Matinee group that matches this scene node name
			Actor = FoundGroupInst->GetGroupActor();
		}


		// Attempt to name-match the scene node with one of the actors.
		if( Actor == NULL )
		{
			Actor = FindObject<AActor>( ANY_PACKAGE, ANSI_TO_TCHAR(FbxNode->GetName()) );
		}


		if ( Actor == NULL || Actor->bDeleteMe )
		{
			KFbxCamera* CameraNode = FindCamera(FbxNode);
			if ( bCreateUnknownCameras && CameraNode != NULL )
			{
				Actor = GWorld->SpawnActor( ACameraActor::StaticClass(), ANSI_TO_TCHAR(CameraNode->GetName()) );
			}
			else
			{
				continue;
			}
		}

		UInterpGroupInst* MatineeGroup = MatineeSequence->FindGroupInst(Actor);

		// Before attempting to create/import a movement track: verify that the node is animated.
		UBOOL IsAnimated = IsNodeAnimated(FbxNode, AnimLayer);

		if (IsAnimated)
		{
			if (MatineeGroup == NULL)
			{
				MatineeGroup = CreateMatineeGroup(MatineeSequence, Actor, FString(FbxNode->GetName()));
			}

			FLOAT TimeLength = ImportMatineeActor(FbxNode, MatineeGroup);
			InterpLength = Max(InterpLength, TimeLength);
		}

		// Right now, cameras are the only supported import entity type.
		if (Actor->IsA(ACameraActor::StaticClass()))
		{
			// there is a pivot node between the FbxNode and node attribute
			KFbxCamera* FbxCamera = NULL;
			if ((KFbxNode*)FbxNode->GetChild(0))
			{
				FbxCamera= ((KFbxNode*)FbxNode->GetChild(0))->GetCamera();
			}

			if (FbxCamera)
			{
				if (MatineeGroup == NULL)
				{
					MatineeGroup = CreateMatineeGroup(MatineeSequence, Actor, FString(FbxNode->GetName()));
				}

				ImportCamera((ACameraActor*) Actor, MatineeGroup, FbxCamera);
			}
		}

		if (MatineeGroup != NULL)
		{
			MatineeGroup->Modify();
		}
	}

	MatineeData->InterpLength = (InterpLength < 0.0f) ? 5.0f : InterpLength;
	MatineeSequence->Modify();

	if (InitializeMatinee)
	{
		MatineeSequence->TermInterp();
	}
}

void CFbxImporter::ImportCamera(ACameraActor* Actor, UInterpGroupInst* MatineeGroup, KFbxCamera* FbxCamera)
{
	// Get the real camera node that stores customed camera attributes
	// Note: there is a pivot node between the Fbx camera Node and node attribute
	KFbxNode* FbxCameraNode = FbxCamera->GetNode()->GetParent();
	// Import the aspect ratio
	//Actor->AspectRatio = FbxCamera->AspectHeight.Get() / FbxCamera->AspectWidth.Get();
	Actor->AspectRatio = FbxCamera->FilmAspectRatio.Get(); // Assumes the FBX comes from Unreal or Maya
	ImportAnimatedProperty(&Actor->AspectRatio, TEXT("AspectRatio"), MatineeGroup, 
				Actor->AspectRatio, FbxCameraNode->FindProperty("UE_AspectRatio") );

	// Import the FOVAngle
	//Actor->FOVAngle = FbxCamera->FieldOfView.Get();
	Actor->FOVAngle = FbxCamera->ComputeFieldOfView(FbxCamera->FocalLength.Get()); // Assumes the FBX comes from Unreal or Maya
	ImportAnimatedProperty(&Actor->FOVAngle, TEXT("FOVAngle"), MatineeGroup, FbxCamera->FieldOfView.Get(),
			FbxCamera->FieldOfView );
	
	// Look for a depth-of-field or motion blur description in the FBX extra information structure.
	if (FbxCamera->UseDepthOfField.Get())
	{
		ImportAnimatedProperty(&Actor->CamOverridePostProcess.DOF_FocusDistance, TEXT("CamOverridePostProcess.DOF_FocusDistance"), MatineeGroup,
				FbxCamera->FocusDistance.Get(), FbxCameraNode->FindProperty("UE_DOF_FocusDistance") );
	}
	else if (FbxCamera->UseMotionBlur.Get())
	{
		ImportAnimatedProperty(&Actor->CamOverridePostProcess.MotionBlur_Amount, TEXT("CamOverridePostProcess.MotionBlur_Amount"), MatineeGroup,
				FbxCamera->MotionBlurIntensity.Get(), FbxCameraNode->FindProperty("UE_MotionBlur_Amount") );
	}
}

void CFbxImporter::ImportAnimatedProperty(FLOAT* Value, const TCHAR* ValueName, UInterpGroupInst* MatineeGroup, const FLOAT FbxValue, KFbxProperty FbxProperty)
{
	if (FbxScene == NULL || FbxProperty == NULL || Value == NULL || MatineeGroup == NULL) return;

	// Retrieve the FBX animated element for this value and verify that it contains an animation curve.
	if (!FbxProperty.IsValid() || !FbxProperty.GetFlag(KFbxProperty::eANIMATABLE))
	{
		return;
	}
	
	// verify the animation curve and it has valid key
	KFbxAnimCurveNode* CurveNode = FbxProperty.GetCurveNode();
	if (!CurveNode)
	{
		return;
	}
	KFbxAnimCurve* FbxCurve = CurveNode->GetCurve(0U);
	if (!FbxCurve || FbxCurve->KeyGetCount() <= 1)
	{
		return;
	}
	
	*Value = FbxValue;

	// Look for a track for this property in the Matinee group.
	UInterpTrackFloatProp* PropertyTrack = NULL;
	INT TrackCount = MatineeGroup->Group->InterpTracks.Num();
	for (INT TrackIndex = 0; TrackIndex < TrackCount; ++TrackIndex)
	{
		UInterpTrackFloatProp* Track = Cast<UInterpTrackFloatProp>( MatineeGroup->Group->InterpTracks(TrackIndex) );
		if (Track != NULL && Track->PropertyName == ValueName)
		{
			PropertyTrack = Track;
			PropertyTrack->FloatTrack.Reset(); // Remove all the existing keys from this track.
			break;
		}
	}

	// If a track for this property was not found, create one.
	if (PropertyTrack == NULL)
	{
		PropertyTrack = ConstructObject<UInterpTrackFloatProp>( UInterpTrackFloatProp::StaticClass(), MatineeGroup->Group, NAME_None, RF_Transactional );
		MatineeGroup->Group->InterpTracks.AddItem(PropertyTrack);
		UInterpTrackInstFloatProp* PropertyTrackInst = ConstructObject<UInterpTrackInstFloatProp>( UInterpTrackInstFloatProp::StaticClass(), MatineeGroup, NAME_None, RF_Transactional );
		MatineeGroup->TrackInst.AddItem(PropertyTrackInst);
		PropertyTrack->PropertyName = ValueName;
		PropertyTrack->TrackTitle = ValueName;
		PropertyTrackInst->InitTrackInst(PropertyTrack);
	}
	FInterpCurveFloat& Curve = PropertyTrack->FloatTrack;


	INT KeyCount = FbxCurve->KeyGetCount();
	// create each key in the first path
	// for animation curve for all property in one track, they share time and interpolation mode in animation keys
	for (INT KeyIndex = Curve.Points.Num(); KeyIndex < KeyCount; ++KeyIndex)
	{
		KFbxAnimCurveKey CurKey = FbxCurve->KeyGet(KeyIndex);

		// Create the curve keys
		FInterpCurvePoint<FLOAT> Key;
		Key.InVal = CurKey.GetTime().GetSecondDouble();

		Key.InterpMode = GetUnrealInterpMode(CurKey);

		// Add this new key to the curve
		Curve.Points.AddItem(Key);
	}

	// Fill in the curve keys with the correct data for this dimension.
	for (INT KeyIndex = 0; KeyIndex < KeyCount; ++KeyIndex)
	{
		KFbxAnimCurveKey CurKey = FbxCurve->KeyGet(KeyIndex);
		FInterpCurvePoint<FLOAT>& UnrealKey = Curve.Points( KeyIndex );
		
		// Prepare the FBX values to import into the track key.
		FLOAT OutVal = CurKey.GetValue();	// FIXME FBX: No conversion here?

		FLOAT ArriveTangent = 0.0f;
		FLOAT LeaveTangent = 0.0f;

		// Convert the Bezier control points, if available, into Hermite tangents
		if( CurKey.GetInterpolation() == KFbxAnimCurveDef::eINTERPOLATION_CUBIC )
		{
			FLOAT LeftTangent = FbxCurve->KeyGetLeftDerivative(KeyIndex);
			FLOAT RightTangent = FbxCurve->KeyGetRightDerivative(KeyIndex);

			if (KeyIndex > 0)
			{
				ArriveTangent = LeftTangent * (CurKey.GetTime().GetSecondDouble() - FbxCurve->KeyGetTime(KeyIndex-1).GetSecondDouble());
			}

			if (KeyIndex < KeyCount - 1)
			{
				LeaveTangent = RightTangent * (FbxCurve->KeyGetTime(KeyIndex+1).GetSecondDouble() - CurKey.GetTime().GetSecondDouble());
			}
		}

		UnrealKey.OutVal = OutVal;
		UnrealKey.ArriveTangent = ArriveTangent;
		UnrealKey.LeaveTangent = LeaveTangent;
	}
}

UInterpGroupInst* CFbxImporter::CreateMatineeGroup(USeqAct_Interp* MatineeSequence, AActor* Actor, FString GroupName)
{
	// There are no groups for this actor: create the Matinee group data structure.
	UInterpGroup* MatineeGroupData = ConstructObject<UInterpGroup>( UInterpGroup::StaticClass(), MatineeSequence->InterpData, NAME_None, RF_Transactional );
	MatineeGroupData->GroupName = FName( *GroupName );
	MatineeSequence->InterpData->InterpGroups.AddItem(MatineeGroupData);

	// Instantiate the Matinee group data structure.
	UInterpGroupInst* MatineeGroup = ConstructObject<UInterpGroupInst>( UInterpGroupInst::StaticClass(), MatineeSequence, NAME_None, RF_Transactional );
	MatineeSequence->GroupInst.AddItem(MatineeGroup);
	MatineeGroup->InitGroupInst(MatineeGroupData, Actor);
	MatineeGroup->SaveGroupActorState();
	MatineeSequence->UpdateConnectorsFromData();

	// Retrieve the Kismet connector for the new group.
	INT ConnectorIndex = MatineeSequence->FindConnectorIndex(MatineeGroupData->GroupName.ToString(), LOC_VARIABLE);
	FIntPoint ConnectorPos = MatineeSequence->GetConnectionLocation(LOC_VARIABLE, ConnectorIndex);

	// Look for this actor in the Kismet object variables
	USequence* Sequence = MatineeSequence->ParentSequence;
	INT ObjectCount = Sequence->SequenceObjects.Num();
	USeqVar_Object* KismetVariable = NULL;
	for (INT ObjectIndex = 0; ObjectIndex < ObjectCount; ++ObjectIndex)
	{
		USeqVar_Object* Variable = Cast<USeqVar_Object>(Sequence->SequenceObjects(ObjectIndex));
		if (Variable != NULL && Variable->ObjValue == Actor)
		{
			KismetVariable = Variable;
			break;
		}
	}

	if (KismetVariable == NULL)
	{
		// Create the object variable in Kismet.
		KismetVariable = ConstructObject<USeqVar_Object>( USeqVar_Object::StaticClass(), Sequence, NAME_None, RF_Transactional );
		KismetVariable->ObjValue = Actor;
		KismetVariable->ObjPosX = MatineeSequence->ObjPosX + ConnectorIndex * LO_MIN_SHAPE_SIZE * 3 / 2; // ConnectorPos.X is not yet valid. It becomes valid only at the first render.
		KismetVariable->ObjPosY = ConnectorPos.Y + LO_MIN_SHAPE_SIZE * 2;
		Sequence->AddSequenceObject(KismetVariable);
		KismetVariable->OnCreated();
	}

	// Connect this new object variable with the Matinee group data connector.
	FSeqVarLink& VariableConnector = MatineeSequence->VariableLinks(ConnectorIndex);
	VariableConnector.LinkedVariables.AddItem(KismetVariable);
	KismetVariable->OnConnect(MatineeSequence, ConnectorIndex);

	// Dirty the Kismet sequence
	Sequence->Modify();

	return MatineeGroup;
}

/**
 * Imports a FBX scene node into a Matinee actor group.
 */
FLOAT CFbxImporter::ImportMatineeActor(KFbxNode* FbxNode, UInterpGroupInst* MatineeGroup)
{
	FName DefaultName(NAME_None);

	if (FbxScene == NULL || FbxNode == NULL || MatineeGroup == NULL) return -1.0f;

	// Search for a Movement track in the Matinee group.
	UInterpTrackMove* MovementTrack = NULL;
	INT TrackCount = MatineeGroup->Group->InterpTracks.Num();
	for (INT TrackIndex = 0; TrackIndex < TrackCount && MovementTrack == NULL; ++TrackIndex)
	{
		MovementTrack = Cast<UInterpTrackMove>( MatineeGroup->Group->InterpTracks(TrackIndex) );
	}

	// Check whether the actor should be pivoted in the FBX document.

	AActor* Actor = MatineeGroup->GetGroupActor();
	check( Actor != NULL ); // would this ever be triggered?

	// Find out whether the FBX node is animated.
	// Bucket the transforms at the same time.
	// The Matinee Movement track can take in a Translation vector
	// and three animated Euler rotation angles.
	KFbxAnimStack* AnimStack = FbxScene->GetMember(FBX_TYPE(KFbxAnimStack), 0);
	if (!AnimStack) return -1.0f;

	MergeAllLayerAnimation(AnimStack, KTime::GetFrameRate(FbxScene->GetGlobalSettings().GetTimeMode()));

	KFbxAnimLayer* AnimLayer = AnimStack->GetMember(FBX_TYPE(KFbxAnimLayer), 0);
	if (AnimLayer == NULL) return -1.0f;
	
	UBOOL bNodeAnimated = IsNodeAnimated(FbxNode, AnimLayer);
	UBOOL ForceImportSampling = FALSE;

	// Add a Movement track if the node is animated and the group does not already have one.
	if (MovementTrack == NULL && bNodeAnimated)
	{
		MovementTrack = ConstructObject<UInterpTrackMove>( UInterpTrackMove::StaticClass(), MatineeGroup->Group, NAME_None, RF_Transactional );
		MatineeGroup->Group->InterpTracks.AddItem(MovementTrack);
		UInterpTrackInstMove* MovementTrackInst = ConstructObject<UInterpTrackInstMove>( UInterpTrackInstMove::StaticClass(), MatineeGroup, NAME_None, RF_Transactional );
		MatineeGroup->TrackInst.AddItem(MovementTrackInst);
		MovementTrackInst->InitTrackInst(MovementTrack);
	}

	// List of casted subtracks in this movement track.
	TArray< UInterpTrackMoveAxis* > SubTracks;

	// Remove all the keys in the Movement track
	if (MovementTrack != NULL)
	{
		MovementTrack->PosTrack.Reset();
		MovementTrack->EulerTrack.Reset();
		MovementTrack->LookupTrack.Points.Reset();

		if( MovementTrack->SubTracks.Num() > 0 )
		{
			for( INT SubTrackIndex = 0; SubTrackIndex < MovementTrack->SubTracks.Num(); ++SubTrackIndex )
			{
				UInterpTrackMoveAxis* SubTrack = CastChecked<UInterpTrackMoveAxis>( MovementTrack->SubTracks( SubTrackIndex ) );
				SubTrack->FloatTrack.Reset();
				SubTrack->LookupTrack.Points.Reset();
				SubTracks.AddItem( SubTrack );
			}
		}
	}

	FLOAT TimeLength = -1.0f;

	// Fill in the Movement track with the FBX keys
	if (bNodeAnimated)
	{
		// Check: The position and rotation tracks must have the same number of keys, the same key timings and
		// the same segment interpolation types.
		KFbxAnimCurve *TransCurves[6], *RealCurves[6];
		
		TransCurves[0] = FbxNode->LclTranslation.GetCurve<KFbxAnimCurve>(AnimLayer, KFCURVENODE_T_X, true);
		TransCurves[1] = FbxNode->LclTranslation.GetCurve<KFbxAnimCurve>(AnimLayer, KFCURVENODE_T_Y, true);
		TransCurves[2] = FbxNode->LclTranslation.GetCurve<KFbxAnimCurve>(AnimLayer, KFCURVENODE_T_Z, true);

		TransCurves[3] = FbxNode->LclRotation.GetCurve<KFbxAnimCurve>(AnimLayer, KFCURVENODE_R_X, true);
		TransCurves[4] = FbxNode->LclRotation.GetCurve<KFbxAnimCurve>(AnimLayer, KFCURVENODE_R_Y, true);
		TransCurves[5] = FbxNode->LclRotation.GetCurve<KFbxAnimCurve>(AnimLayer, KFCURVENODE_R_Z, true);
		// remove empty curves
		INT CurveIndex;
		INT RealCurveNum = 0;
		for (CurveIndex = 0; CurveIndex < 6; CurveIndex++)
		{
			if (TransCurves[CurveIndex] && TransCurves[CurveIndex]->KeyGetCount() > 1)
			{
				RealCurves[RealCurveNum++] = TransCurves[CurveIndex];
			}
		}
		
		UBOOL bResample = FALSE;
		if (RealCurveNum > 1)
		{
			INT KeyCount = RealCurves[0]->KeyGetCount();
			// check key count of all curves
			for (CurveIndex = 1; CurveIndex < RealCurveNum; CurveIndex++)
			{
				if (KeyCount != RealCurves[CurveIndex]->KeyGetCount())
				{
					bResample = TRUE;
					break;
				}
			}
			// check key time for each key
			for (INT KeyIndex = 0; !bResample && KeyIndex < KeyCount; KeyIndex++)
			{
				KTime KeyTime = RealCurves[0]->KeyGetTime(KeyIndex);
				KFbxAnimCurveDef::EInterpolationType Interpolation = RealCurves[0]->KeyGetInterpolation(KeyIndex);
				//KFbxAnimCurveDef::ETangentMode Tangent = RealCurves[0]->KeyGetTangentMode(KeyIndex);
				
				for (CurveIndex = 1; CurveIndex < RealCurveNum; CurveIndex++)
				{
					if (KeyTime != RealCurves[CurveIndex]->KeyGetTime(KeyIndex) ||
						Interpolation != RealCurves[CurveIndex]->KeyGetInterpolation(KeyIndex) ) // ||
						//Tangent != RealCurves[CurveIndex]->KeyGetTangentMode(KeyIndex))
					{
						bResample = TRUE;
						break;
					}
				}
			}
			
			if (bResample)
			{
				// Get the re-sample time span
				KTime Start, Stop;
				Start = RealCurves[0]->KeyGetTime(0);
				Stop = RealCurves[0]->KeyGetTime(RealCurves[0]->KeyGetCount() - 1);
				for (CurveIndex = 1; CurveIndex < RealCurveNum; CurveIndex++)
				{
					if (Start > RealCurves[CurveIndex]->KeyGetTime(0))
					{
						Start = RealCurves[CurveIndex]->KeyGetTime(0);
					}
					
					if (Stop < RealCurves[CurveIndex]->KeyGetTime(RealCurves[CurveIndex]->KeyGetCount() - 1))
					{
						Stop = RealCurves[CurveIndex]->KeyGetTime(RealCurves[CurveIndex]->KeyGetCount() - 1);
					}
				}
				
				DOUBLE ResampleRate;
				ResampleRate = KTime::GetFrameRate(FbxScene->GetGlobalSettings().GetTimeMode());
				KTime FramePeriod;
				FramePeriod.SetSecondDouble(1.0 / ResampleRate);
				
				for (CurveIndex = 0; CurveIndex < 6; CurveIndex++)
				{
					UBOOL bRemoveConstantKey = FALSE;
					// for the constant animation curve, the key may be not in the resample time range,
					// so we need to remove the constant key after resample,
					// otherwise there must be one more key
					if (TransCurves[CurveIndex]->KeyGetCount() == 1 && TransCurves[CurveIndex]->KeyGetTime(0) < Start)
					{
						bRemoveConstantKey = TRUE;
					}

					// only re-sample from Start to Stop
					KFCurveUtils::Resample(*TransCurves[CurveIndex]->GetKFCurve(), FramePeriod, Start, Stop, true);

					// remove the key that is not in the resample time range
					// the constant key always at the time 0, so it is OK to remove the first key
					if (bRemoveConstantKey)
					{
						TransCurves[CurveIndex]->KeyRemove(0);
					}
				}

			}
			
		}
		
		// Reset the actor position.
		KFbxXMatrix Matrix = ComputeTotalMatrix(FbxNode);
		KFbxVector4 DefaultPos = FbxNode->LclTranslation.Get();
		KFbxVector4 DefaultRot = FbxNode->LclRotation.Get();
		KFbxVector4 PreRotation = FbxNode->GetPreRotation(KFbxNode::eSOURCE_SET);
		KFbxVector4 PostRotation = FbxNode->GetPostRotation(KFbxNode::eSOURCE_SET);
		fbxDouble3 Rotation(DefaultRot[0], -DefaultRot[1], -DefaultRot[2]);

		Actor->SetLocation( FVector( DefaultPos[0], -DefaultPos[1], DefaultPos[2] ) );

		// Only process the rotation if there's a pre/post rotation
		if ((PreRotation.Length() > SMALL_NUMBER) || (PostRotation.Length() > SMALL_NUMBER))
		{
			KFbxXMatrix PreRotationMatrix;
			KFbxXMatrix PostRotationMatrix;
			KFbxXMatrix LocalMatrix;
			PreRotationMatrix.SetR(PreRotation);
			PostRotationMatrix.SetR(PostRotation);
			LocalMatrix.SetR(Rotation);

			// Calculate the final rotation
			LocalMatrix = PreRotationMatrix * LocalMatrix * PostRotationMatrix;

			Rotation = LocalMatrix.GetR();
		}

		// If the camera was animated directly, we need to account for the fact that FBX 
		// cameras default to a <Up, Forward, Left> of <Y, -Z, -X> while Unreal cameras are <Z, X, -Y>
		if (FbxNode->GetCamera())
		{
			Rotation[0] -= 90.0f;
		}
		
		Actor->SetRotation( FRotator::MakeFromEuler( FVector( Rotation[0], Rotation[1], Rotation[2] ) ) );
		
		if( MovementTrack->SubTracks.Num() > 0 )
		{
			ImportMoveSubTrack(TransCurves[0], 0, SubTracks(0), 0, FALSE, RealCurves[0], DefaultPos[0]);
			ImportMoveSubTrack(TransCurves[1], 1, SubTracks(1), 1, TRUE, RealCurves[0], DefaultPos[1]);
			ImportMoveSubTrack(TransCurves[2], 2, SubTracks(2), 2, FALSE, RealCurves[0], DefaultPos[2]);
			ImportMoveSubTrack(TransCurves[3], 3, SubTracks(3), 0, FALSE, RealCurves[0], DefaultRot[0]);
			ImportMoveSubTrack(TransCurves[4], 4, SubTracks(4), 1, TRUE, RealCurves[0], DefaultRot[1]);
			ImportMoveSubTrack(TransCurves[5], 5, SubTracks(5), 2, TRUE, RealCurves[0], DefaultRot[2]);

			FixupMatineeMovementSubTrackRotations(SubTracks, FbxNode);

			for( INT SubTrackIndex = 0; SubTrackIndex < SubTracks.Num(); ++SubTrackIndex )
			{
				UInterpTrackMoveAxis* SubTrack = SubTracks( SubTrackIndex );
				// Generate empty look-up keys.
				INT KeyIndex;
				for ( KeyIndex = 0; KeyIndex < SubTrack->FloatTrack.Points.Num(); ++KeyIndex )
				{
					SubTrack->LookupTrack.AddPoint( SubTrack->FloatTrack.Points( KeyIndex).InVal, DefaultName );
				}
			}

			FLOAT StartTime;
			// Scale the track timing to ensure that it is large enough
			MovementTrack->GetTimeRange( StartTime, TimeLength );
		}
		else
		{
			ImportMatineeAnimated(TransCurves[0], 0, MovementTrack->PosTrack, 0, FALSE, RealCurves[0], DefaultPos[0]);
			ImportMatineeAnimated(TransCurves[1], 1, MovementTrack->PosTrack, 1, TRUE, RealCurves[0], DefaultPos[1]);
			ImportMatineeAnimated(TransCurves[2], 2, MovementTrack->PosTrack, 2, FALSE, RealCurves[0], DefaultPos[2]);
			ImportMatineeAnimated(TransCurves[3], 3, MovementTrack->EulerTrack, 0, FALSE, RealCurves[0], DefaultRot[0]);
			ImportMatineeAnimated(TransCurves[4], 4, MovementTrack->EulerTrack, 1, TRUE, RealCurves[0], DefaultRot[1]);
			ImportMatineeAnimated(TransCurves[5], 5, MovementTrack->EulerTrack, 2, TRUE, RealCurves[0], DefaultRot[2]);

			FixupMatineeMovementTrackRotations(MovementTrack, FbxNode);

			// Generate empty look-up keys.
			INT KeyIndex;
			for ( KeyIndex = 0; KeyIndex < RealCurves[0]->KeyGetCount(); ++KeyIndex )
			{
				MovementTrack->LookupTrack.AddPoint( (FLOAT)RealCurves[0]->KeyGet(KeyIndex).GetTime().GetSecondDouble(), DefaultName );
			}

			// Scale the track timing to ensure that it is large enough
			TimeLength = (FLOAT)RealCurves[0]->KeyGet(KeyIndex - 1).GetTime().GetSecondDouble();
		}

	}

	// Inform the engine and UI that the tracks have been modified.
	if (MovementTrack != NULL)
	{
		MovementTrack->Modify();
	}
	MatineeGroup->Modify();
	
	return TimeLength;
}

void CFbxImporter::FixupMatineeMovementTrackRotations(UInterpTrackMove* MovementTrack, KFbxNode* FbxNode)
{
	KFbxVector4 PreRotation = FbxNode->GetPreRotation(KFbxNode::eSOURCE_SET);
	KFbxVector4 PostRotation = FbxNode->GetPostRotation(KFbxNode::eSOURCE_SET);

	// Only process the keys if there's a pre/post rotation or the camera was directly animated
	if ((PreRotation.Length() > SMALL_NUMBER) || (PostRotation.Length() > SMALL_NUMBER) || FbxNode->GetCamera())
	{
		KFbxXMatrix PreRotationMatrix;
		KFbxXMatrix PostRotationMatrix;
		PreRotationMatrix.SetR(PreRotation);
		PostRotationMatrix.SetR(PostRotation);

		for (INT RotKeyIdx = 0; RotKeyIdx < MovementTrack->EulerTrack.Points.Num(); ++RotKeyIdx)
		{
			FInterpCurvePoint<FVector>& Key = MovementTrack->EulerTrack.Points(RotKeyIdx);

			fbxDouble3 Rotation(Key.OutVal.X, Key.OutVal.Y, Key.OutVal.Z);
			
			if ((PreRotation.Length() > SMALL_NUMBER) || (PostRotation.Length() > SMALL_NUMBER))
			{
				KFbxXMatrix LocalMatrix;
				LocalMatrix.SetR(Rotation);
				
				// Calculate the final rotation
				LocalMatrix = PreRotationMatrix * LocalMatrix * PostRotationMatrix;

				Rotation = LocalMatrix.GetR();
			}

			// If the camera was animated directly, we need to account for the fact that FBX 
			// cameras default to a <Up, Forward, Left> of <Y, -Z, -X> while Unreal cameras are <Z, X, -Y>
			if (FbxNode->GetCamera())
			{
				Rotation[0] -= 90.0f;
			}

			Key.OutVal.X = Rotation[0];
			Key.OutVal.Y = Rotation[1];
			Key.OutVal.Z = Rotation[2];

			// Recalculate tangents
			for (INT CurveIdx = 0; CurveIdx < 3; ++CurveIdx)
			{
				if (RotKeyIdx > 0)
				{
					// Curve tangents are stored as slopes. To get a smooth curve, we take a pair of keys, P0 and P1,
					// and do (P1 - P0) / DeltaTime to find the slope between the two keys
					FInterpCurvePoint<FVector>& PreviousKey = MovementTrack->EulerTrack.Points(RotKeyIdx - 1);
					FLOAT DeltaTime = Key.InVal - PreviousKey.InVal;
					FLOAT NewTangent = (Key.OutVal[CurveIdx] - PreviousKey.OutVal[CurveIdx]) / DeltaTime;
			
					// For P0 and P1, set the leave tangent for P0 and the arrive tangent for P1
					if (RotKeyIdx < MovementTrack->EulerTrack.Points.Num() - 1)
					{
						PreviousKey.LeaveTangent[CurveIdx] = NewTangent;
						Key.ArriveTangent[CurveIdx] = NewTangent;
					}
					
					// Handle assigning tangents to the first key on the curve
					if (RotKeyIdx == 1)
					{
						PreviousKey.ArriveTangent[CurveIdx] = NewTangent;

						// Edge case when the curve only has two keys
						if (MovementTrack->EulerTrack.Points.Num() == 2)
						{
							PreviousKey.LeaveTangent[CurveIdx] = NewTangent;
						}
					}
					
					// Handle assigning tangents to the last key on the curve
					if (RotKeyIdx == MovementTrack->EulerTrack.Points.Num() - 1)
					{
						Key.ArriveTangent[CurveIdx] = NewTangent;
						Key.LeaveTangent[CurveIdx] = NewTangent;
					}
				}
			}
		}

		warnf(TEXT("FBX Import: Matinee - Due to axis conversions, the animation curve tangents for camera %s had to be recalculated."), ANSI_TO_TCHAR(FbxNode->GetName()));
	}
}

void CFbxImporter::FixupMatineeMovementSubTrackRotations(TArray<UInterpTrackMoveAxis*>& SubTracks, KFbxNode* FbxNode)
{
	check(SubTracks.Num() == 6);

	KFbxVector4 PreRotation = FbxNode->GetPreRotation(KFbxNode::eSOURCE_SET);
	KFbxVector4 PostRotation = FbxNode->GetPostRotation(KFbxNode::eSOURCE_SET);

	// Only process the keys if there's a pre/post rotation or the camera was directly animated
	if ((PreRotation.Length() > SMALL_NUMBER) || (PostRotation.Length() > SMALL_NUMBER) || FbxNode->GetCamera())
	{
		KFbxXMatrix PreRotationMatrix;
		KFbxXMatrix PostRotationMatrix;
		PreRotationMatrix.SetR(PreRotation);
		PostRotationMatrix.SetR(PostRotation);

		INT RotationTrackKeyIndex[3] = {0, 0, 0};
		FLOAT RotationTrackKeyTime[3] = {0.0f, 0.0f, 0.0f};
		FLOAT RotationTrackKeyValue[3] = {0.0f, 0.0f, 0.0f};
		BITFIELD CurrentTracks = 0;

		// This is a bit messy because we can't assume that each keyframe has all three rotational components keyed.
		// So, we basically progress down the timeline processing each key as we encounter them.

		UBOOL AllKeysProcessed = FALSE;
		while (!AllKeysProcessed)
		{
			AllKeysProcessed = TRUE;
			CurrentTracks = 0;

			INT RotationAxis = 0;
			FLOAT CurrentTime = MAX_FLT;
			// Find the next key or set of keys
			for (RotationAxis = 0; RotationAxis < 3; ++RotationAxis)
			{
				RotationTrackKeyTime[RotationAxis] = SubTracks(RotationAxis + 3)->FloatTrack.Points(RotationTrackKeyIndex[RotationAxis]).InVal;
				RotationTrackKeyValue[RotationAxis] = SubTracks(RotationAxis + 3)->FloatTrack.Points(RotationTrackKeyIndex[RotationAxis]).OutVal;

				if (RotationTrackKeyTime[RotationAxis] < (CurrentTime - SMALL_NUMBER))
				{
					CurrentTime = RotationTrackKeyTime[RotationAxis];
					CurrentTracks = 1 << RotationAxis;
				}
				else if (appIsNearlyEqual(RotationTrackKeyTime[RotationAxis], CurrentTime)) // More than one rotational component is keyed at this frame
				{
					CurrentTracks += 1 << RotationAxis;
				}
			}

			fbxDouble3 Rotation;
			// Build the Euler rotation vector
			for (RotationAxis = 0; RotationAxis < 3; ++RotationAxis)
			{
				if (CurrentTracks & (1 << RotationAxis)) // If the rotational component was keyed, use the key's value
				{
					Rotation[RotationAxis] = RotationTrackKeyValue[RotationAxis];
				}
				else // If the rotational component wasn't keyed, we need to sample the curve at the given time
				{
					SubTracks(RotationAxis + 3)->FloatTrack.Eval(CurrentTime, Rotation[RotationAxis]);
				}
			}

			if ((PreRotation.Length() > SMALL_NUMBER) || (PostRotation.Length() > SMALL_NUMBER))
			{
				KFbxXMatrix LocalMatrix;
				LocalMatrix.SetR(Rotation);
		
				// Calculate the final rotation
				LocalMatrix = PreRotationMatrix * LocalMatrix * PostRotationMatrix;

				Rotation = LocalMatrix.GetR();
			}
			
			// If the camera was animated directly, we need to account for the fact that FBX 
			// cameras default to a <Up, Forward, Left> of <Y, -Z, -X> while Unreal cameras are <Z, X, -Y>
			if (FbxNode->GetCamera())
			{
				Rotation[0] -= 90.0f;
			}
			
			for (RotationAxis = 0; RotationAxis < 3; ++RotationAxis)
			{
				if (CurrentTracks & (1 << RotationAxis)) // If the rotational component was keyed, simply change it's value
				{
					SubTracks(RotationAxis + 3)->FloatTrack.Points(RotationTrackKeyIndex[RotationAxis]).OutVal = Rotation[RotationAxis];
					SubTracks(RotationAxis + 3)->FloatTrack.Points(RotationTrackKeyIndex[RotationAxis]).InterpMode = CIM_CurveAuto; // There's no easy way to maintain the curve type, so we just default to auto
				}
				else  // If the rotational component wasn't keyed, create a new key
				{
					FInterpCurvePoint<FLOAT> Key;
					Key.InVal = CurrentTime;
					Key.OutVal = Rotation[RotationAxis];
					Key.InterpMode = CIM_CurveAuto;

					SubTracks(RotationAxis + 3)->FloatTrack.Points.InsertItem(Key, RotationTrackKeyIndex[RotationAxis]);
				}

				++RotationTrackKeyIndex[RotationAxis];

				AllKeysProcessed &= (RotationTrackKeyIndex[RotationAxis] == SubTracks(RotationAxis + 3)->FloatTrack.Points.Num());
			}
		}

		// Recalculate tangents
		for (INT CurveIdx = 0; CurveIdx < 3; ++CurveIdx)
		{
			INT NumKeys = SubTracks(CurveIdx + 3)->FloatTrack.Points.Num();

			for (INT RotKeyIdx = 1; RotKeyIdx < NumKeys; ++RotKeyIdx)
			{
				// Curve tangents are stored as slopes. To get a smooth curve, we take a pair of keys, P0 and P1,
				// and do (P1 - P0) / DeltaTime to find the slope between the two keys
				FInterpCurvePoint<FLOAT>& PreviousKey = SubTracks(CurveIdx + 3)->FloatTrack.Points(RotKeyIdx - 1);
				FInterpCurvePoint<FLOAT>& Key = SubTracks(CurveIdx + 3)->FloatTrack.Points(RotKeyIdx);
				FLOAT DeltaTime = Key.InVal - PreviousKey.InVal;
				FLOAT NewTangent = (Key.OutVal - PreviousKey.OutVal) / DeltaTime;

				// For P0 and P1, set the leave tangent for P0 and the arrive tangent for P1
				if (RotKeyIdx < NumKeys - 1)
				{
					PreviousKey.LeaveTangent = NewTangent;
					Key.ArriveTangent = NewTangent;
				}

				// Handle assigning tangents to the first key on the curve
				if (RotKeyIdx == 1)
				{
					PreviousKey.ArriveTangent = NewTangent;

					// Edge case when the curve only has two keys
					if (NumKeys == 2)
					{
						PreviousKey.LeaveTangent = NewTangent;
					}
				}

				// Handle assigning tangents to the last key on the curve
				if (RotKeyIdx == NumKeys - 1)
				{
					Key.ArriveTangent = NewTangent;
					Key.LeaveTangent = NewTangent;
				}
			}
		}

		warnf(TEXT("FBX Import: Matinee - Due to axis conversions, the animation curve tangents for camera %s had to be recalculated."), ANSI_TO_TCHAR(FbxNode->GetName()));
	}
}

BYTE CFbxImporter::GetUnrealInterpMode(KFbxAnimCurveKey FbxKey)
{
	BYTE Mode = CIM_CurveUser;
	// Convert the interpolation type from FBX to Unreal.
	switch( FbxKey.GetInterpolation() )
	{
		case KFbxAnimCurveDef::eINTERPOLATION_CUBIC:
		{
			switch (FbxKey.GetTangentMode())
			{
				// Auto tangents will now be imported as user tangents to allow the
				// user to modify them without inadvertently resetting other tangents
// 				case KFbxAnimCurveDef::eTANGENT_AUTO:
// 					if ((KFbxAnimCurveDef::eTANGENT_GENERIC_CLAMP & FbxKey.GetTangentMode(true)))
// 					{
// 						Mode = CIM_CurveAutoClamped;
// 					}
// 					else
// 					{
// 						Mode = CIM_CurveAuto;
// 					}
// 					break;
				case KFbxAnimCurveDef::eTANGENT_BREAK:
					Mode = CIM_CurveBreak;
					break;
				case KFbxAnimCurveDef::eTANGENT_AUTO:
				case KFbxAnimCurveDef::eTANGENT_USER:
				case KFbxAnimCurveDef::eTANGENT_TCB:
					Mode = CIM_CurveUser;
					break;
				default:
					break;
			}
			break;
		}

		case KFbxAnimCurveDef::eINTERPOLATION_CONSTANT:
			if (FbxKey.GetTangentMode() != (KFbxAnimCurveDef::ETangentMode)KFbxAnimCurveDef::eCONSTANT_STANDARD)
			{
				// warning not support
				;
			}
			Mode = CIM_Constant;
			break;

		case KFbxAnimCurveDef::eINTERPOLATION_LINEAR:
			Mode = CIM_Linear;
			break;
	}
	return Mode;
}

void CFbxImporter::ImportMoveSubTrack( KFbxAnimCurve* FbxCurve, INT FbxDimension, UInterpTrackMoveAxis* SubTrack, INT CurveIndex, UBOOL bNegative, KFbxAnimCurve* RealCurve, FLOAT DefaultVal )
{
	if (CurveIndex >= 3) return;

	FInterpCurveFloat& Curve = SubTrack->FloatTrack;
	// the FBX curve has no valid keys, so fake the Unreal Matinee curve
	if (FbxCurve == NULL || FbxCurve->KeyGetCount() < 2)
	{
		INT KeyIndex;
		for ( KeyIndex = Curve.Points.Num(); KeyIndex < RealCurve->KeyGetCount(); ++KeyIndex )
		{
			FLOAT Time = (FLOAT)RealCurve->KeyGet(KeyIndex).GetTime().GetSecondDouble();
			// Create the curve keys
			FInterpCurvePoint<FLOAT> Key;
			Key.InVal = Time;
			Key.InterpMode = GetUnrealInterpMode(RealCurve->KeyGet(KeyIndex));

			Curve.Points.AddItem(Key);
		}

		for ( KeyIndex = 0; KeyIndex < RealCurve->KeyGetCount(); ++KeyIndex )
		{
			FInterpCurvePoint<FLOAT>& Key = Curve.Points( KeyIndex );
			Key.OutVal = DefaultVal;
			Key.ArriveTangent = 0;
			Key.LeaveTangent = 0;
		}
	}
	else
	{
		INT KeyCount = (INT) FbxCurve->KeyGetCount();

		for (INT KeyIndex = Curve.Points.Num(); KeyIndex < KeyCount; ++KeyIndex)
		{
			KFbxAnimCurveKey CurKey = FbxCurve->KeyGet( KeyIndex );

			// Create the curve keys
			FInterpCurvePoint<FLOAT> Key;
			Key.InVal = CurKey.GetTime().GetSecondDouble();

			Key.InterpMode = GetUnrealInterpMode(CurKey);

			// Add this new key to the curve
			Curve.Points.AddItem(Key);
		}

		// Fill in the curve keys with the correct data for this dimension.
		for (INT KeyIndex = 0; KeyIndex < KeyCount; ++KeyIndex)
		{
			KFbxAnimCurveKey CurKey = FbxCurve->KeyGet( KeyIndex );
			FInterpCurvePoint<FLOAT>& UnrealKey = Curve.Points( KeyIndex );

			// Prepare the FBX values to import into the track key.
			// Convert the Bezier control points, if available, into Hermite tangents
			FLOAT OutVal = bNegative ? -CurKey.GetValue() : CurKey.GetValue();

			FLOAT ArriveTangent = 0.0f;
			FLOAT LeaveTangent = 0.0f;

			if( CurKey.GetInterpolation() == KFbxAnimCurveDef::eINTERPOLATION_CUBIC )
			{
				ArriveTangent = bNegative? -FbxCurve->KeyGetLeftDerivative(KeyIndex): FbxCurve->KeyGetLeftDerivative(KeyIndex);
				LeaveTangent = bNegative? -FbxCurve->KeyGetRightDerivative(KeyIndex): FbxCurve->KeyGetRightDerivative(KeyIndex);
			}

			// Fill in the track key with the prepared values
			UnrealKey.OutVal = OutVal;
			UnrealKey.ArriveTangent = ArriveTangent;
			UnrealKey.LeaveTangent = LeaveTangent;
		}
	}
}

void CFbxImporter::ImportMatineeAnimated(KFbxAnimCurve* FbxCurve, INT FbxDimension, FInterpCurveVector& Curve, INT CurveIndex, UBOOL bNegative, KFbxAnimCurve* RealCurve, FLOAT DefaultVal)
{
	if (CurveIndex >= 3) return;
	
	// the FBX curve has no valid keys, so fake the Unreal Matinee curve
	if (FbxCurve == NULL || FbxCurve->KeyGetCount() < 2)
	{
		INT KeyIndex;
		for ( KeyIndex = Curve.Points.Num(); KeyIndex < RealCurve->KeyGetCount(); ++KeyIndex )
		{
			FLOAT Time = (FLOAT)RealCurve->KeyGet(KeyIndex).GetTime().GetSecondDouble();
			// Create the curve keys
			FInterpCurvePoint<FVector> Key;
			Key.InVal = Time;
			Key.InterpMode = GetUnrealInterpMode(RealCurve->KeyGet(KeyIndex));
			
			Curve.Points.AddItem(Key);
		}
		
		for ( KeyIndex = 0; KeyIndex < RealCurve->KeyGetCount(); ++KeyIndex )
		{
			FInterpCurvePoint<FVector>& Key = Curve.Points( KeyIndex );
			switch (CurveIndex)
			{
			case 0:
				Key.OutVal.X = DefaultVal;
				Key.ArriveTangent.X = 0;
				Key.LeaveTangent.X = 0;
				break;
			case 1:
				Key.OutVal.Y = DefaultVal;
				Key.ArriveTangent.Y = 0;
				Key.LeaveTangent.Y = 0;
				break;
			case 2:
			default:
				Key.OutVal.Z = DefaultVal;
				Key.ArriveTangent.Z = 0;
				Key.LeaveTangent.Z = 0;
				break;
			}
		}
	}
	else
	{
		INT KeyCount = (INT) FbxCurve->KeyGetCount();
		
		for (INT KeyIndex = Curve.Points.Num(); KeyIndex < KeyCount; ++KeyIndex)
		{
			KFbxAnimCurveKey CurKey = FbxCurve->KeyGet( KeyIndex );

			// Create the curve keys
			FInterpCurvePoint<FVector> Key;
			Key.InVal = CurKey.GetTime().GetSecondDouble();

			Key.InterpMode = GetUnrealInterpMode(CurKey);

			// Add this new key to the curve
			Curve.Points.AddItem(Key);
		}

		// Fill in the curve keys with the correct data for this dimension.
		for (INT KeyIndex = 0; KeyIndex < KeyCount; ++KeyIndex)
		{
			KFbxAnimCurveKey CurKey = FbxCurve->KeyGet( KeyIndex );
			FInterpCurvePoint<FVector>& UnrealKey = Curve.Points( KeyIndex );
			
			// Prepare the FBX values to import into the track key.
			// Convert the Bezier control points, if available, into Hermite tangents
			FLOAT OutVal = bNegative ? -CurKey.GetValue() : CurKey.GetValue();

			FLOAT ArriveTangent = 0.0f;
			FLOAT LeaveTangent = 0.0f;
			
			if( CurKey.GetInterpolation() == KFbxAnimCurveDef::eINTERPOLATION_CUBIC )
			{
				ArriveTangent = bNegative? -FbxCurve->KeyGetLeftDerivative(KeyIndex): FbxCurve->KeyGetLeftDerivative(KeyIndex);
				LeaveTangent = bNegative? -FbxCurve->KeyGetRightDerivative(KeyIndex): FbxCurve->KeyGetRightDerivative(KeyIndex);
			}

			// Fill in the track key with the prepared values
			switch (CurveIndex)
			{
			case 0:
				UnrealKey.OutVal.X = OutVal;
				UnrealKey.ArriveTangent.X = ArriveTangent;
				UnrealKey.LeaveTangent.X = LeaveTangent;
				break;
			case 1:
				UnrealKey.OutVal.Y = OutVal;
				UnrealKey.ArriveTangent.Y = ArriveTangent;
				UnrealKey.LeaveTangent.Y = LeaveTangent;
				break;
			case 2:
			default:
				UnrealKey.OutVal.Z = OutVal;
				UnrealKey.ArriveTangent.Z = ArriveTangent;
				UnrealKey.LeaveTangent.Z = LeaveTangent;
				break;
			}
		}
	}
}

} // namespace UnFBX

#endif	// WITH_FBX


