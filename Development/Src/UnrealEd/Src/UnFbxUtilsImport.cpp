/*
* Copyright 2008 Autodesk, Inc.  All Rights Reserved.
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

#include "UnrealEd.h"
#if WITH_FBX

#include "Factories.h"
#include "Engine.h"
#include "UnTextureLayout.h"
#include "EngineMaterialClasses.h"
#include "UnFbxImporter.h"

using namespace UnFbx;

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
FVector CBasicDataConverter::ConvertPos(KFbxVector4 Vector)
{
	FVector Out;
	Out[0] = Vector[0];
	// flip Y, then the right-handed axis system is converted to LHS
	Out[1] = -Vector[1];
	Out[2] = Vector[2];
	return Out;
}


//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
FVector CBasicDataConverter::ConvertDir(KFbxVector4 Vector)
{
	FVector Out;
	Out[0] = Vector[0];
	Out[1] = Vector[1];
	Out[2] = Vector[2];
	return Out;
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
FVector CBasicDataConverter::ConvertScale(fbxDouble3 Vector)
{
	FVector Out;
	Out[0] = Vector[0];
	Out[1] = Vector[1];
	Out[2] = Vector[2];
	return Out;
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
FVector CBasicDataConverter::ConvertScale(KFbxVector4 Vector)
{
	FVector Out;
	Out[0] = Vector[0];
	Out[1] = Vector[1];
	Out[2] = Vector[2];
	return Out;
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
FRotator CBasicDataConverter::ConvertRotation(KFbxQuaternion Quaternion)
{
	FRotator Out(ConvertRotToQuat(Quaternion));
	return Out;
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
FVector CBasicDataConverter::ConvertRotationToFVect(KFbxQuaternion Quaternion, UBOOL bInvertOrient)
{
	FQuat UnrealQuaternion = ConvertRotToQuat(Quaternion);
	FVector Euler;
	Euler = UnrealQuaternion.Euler();
	if (bInvertOrient)
	{
		Euler.Y = -Euler.Y;
		Euler.Z = 180.f+Euler.Z;
	}
	return Euler;
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
FQuat CBasicDataConverter::ConvertRotToQuat(KFbxQuaternion Quaternion)
{
	FQuat UnrealQuat;
	UnrealQuat.X = Quaternion[0];
	UnrealQuat.Y = -Quaternion[1];
	UnrealQuat.Z = Quaternion[2];
	UnrealQuat.W = -Quaternion[3];
	
	return UnrealQuat;
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
FQuat CBasicDataConverter::ConvertRotToAnimQuat(KFbxQuaternion Quaternion, UBOOL bForAnimation)
{
	FQuat UnrealQuat;
	UnrealQuat.X = Quaternion[0];
	UnrealQuat.Y = -Quaternion[1];
	UnrealQuat.Z = Quaternion[2];
	UnrealQuat.W = bForAnimation? Quaternion[3] : -Quaternion[3] ; // not minus for animation quats, because it's inverted otherwise ! For whatever reason, Epic animation system doesn't rotate in the same way.
	return UnrealQuat;
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
FLOAT CBasicDataConverter::ConvertDist(fbxDouble1 Distance)
{
	FLOAT Out;
	Out = (FLOAT)Distance;
	return Out;
}

KFbxVector4 CBasicDataConverter::ConvertToFbxPos(FVector Vector)
{
	KFbxVector4 Out;
	Out[0] = Vector[0];
	Out[1] = -Vector[1];
	Out[2] = Vector[2];
	
	return Out;
}

KFbxVector4 CBasicDataConverter::ConvertToFbxRot(FVector Vector)
{
	KFbxVector4 Out;
	Out[0] = Vector[0];
	Out[1] = -Vector[1];
	Out[2] = -Vector[2];

	return Out;
}

KFbxVector4 CBasicDataConverter::ConvertToFbxScale(FVector Vector)
{
	KFbxVector4 Out;
	Out[0] = Vector[0];
	Out[1] = Vector[1];
	Out[2] = Vector[2];

	return Out;
}

KFbxVector4 CBasicDataConverter::ConvertToFbxColor(FColor Color)
{
	KFbxVector4 Out;
	Out[0] = Color.R / 255.0f;
	Out[1] = Color.G / 255.0f;
	Out[2] = Color.B / 255.0f;
	Out[3] = Color.A / 255.0f;

	return Out;
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
USequence* CFbxImporter::GetKismetSequence(ULevel* Level)
{
	check(Level);
	USequence* KismetSequence = Level->GetGameSequence();
	if (!KismetSequence)
	{
		// create a game sequence
		KismetSequence = ConstructObject<USequence>( USequence::StaticClass(), Level, NAME_None, RF_Standalone|RF_Transactional );
		GWorld->SetGameSequence(KismetSequence,Level);
		// probably redundant, but just to be sure we don't side-step any special-case code in GetGameSequence
		KismetSequence = GWorld->GetGameSequence(Level);
	}
	return KismetSequence;
}

#endif // WITH_FBX
