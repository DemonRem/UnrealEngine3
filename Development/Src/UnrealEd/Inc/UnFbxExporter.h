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
					FBX exporter for Unreal Engine 3
=============================================================================*/
#if WITH_FBX

#ifndef __UNFBXEXPORTER_H__
#define __UNFBXEXPORTER_H__

#include "Factories.h"
#include "MatineeExporter.h"

#include "UnFbxImporter.h"

// Temporarily disable a few warnings due to virtual function abuse in FBX source files
#pragma warning( push )
#pragma warning( disable : 4263 ) // 'function' : member function does not override any base class virtual member function
#pragma warning( disable : 4264 ) // 'virtual_function' : no override available for virtual member function from base 'class'; function is hidden

// Include the fbx sdk header
// temp undef/redef of _O_RDONLY because kfbxcache.h (included by fbxsdk.h) does
// a weird use of these identifiers inside an enum.
#ifdef _O_RDONLY
#define TMP_UNFBX_BACKUP_O_RDONLY _O_RDONLY
#define TMP_UNFBX_BACKUP_O_WRONLY _O_WRONLY
#undef _O_RDONLY
#undef _O_WRONLY
#endif


//Robert G. : Packing was only set for the 64bits platform, but we also need it for 32bits.
//This was found while trying to trace a loop that iterate through all character links.
//The memory didn't match what the debugger displayed, obviously since the packing was not right.
#pragma pack(push,8)
	#include <fbxsdk.h>
#pragma pack(pop)


#ifdef TMP_UNFBX_BACKUP_O_RDONLY
#define _O_RDONLY TMP_FBX_BACKUP_O_RDONLY
#define _O_WRONLY TMP_FBX_BACKUP_O_WRONLY
#undef TMP_UNFBX_BACKUP_O_RDONLY
#undef TMP_UNFBX_BACKUP_O_WRONLY
#endif

#pragma warning( pop )

namespace UnFbx
{

/**
 * Main FBX Exporter class.
 */
class FbxExporter  : public MatineeExporter
{
public:
	/**
	 * Returns the exporter singleton. It will be created on the first request.
	 */
	static FbxExporter* GetInstance();
	
	/**
	 * Creates and readies an empty document for export.
	 */
	virtual void CreateDocument();
	
	/**
	 * Closes the FBX document, releasing its memory.
	 */
	virtual void CloseDocument();
	
	/**
	 * Writes the FBX document to disk and releases it by calling the CloseDocument() function.
	 */
	virtual void WriteToFile(const TCHAR* Filename);
	
	/**
	 * Exports the light-specific information for a UE3 light actor.
	 */
	virtual void ExportLight( ALight* Actor, USeqAct_Interp* MatineeSequence );

	/**
	 * Exports the camera-specific information for a UE3 camera actor.
	 */
	virtual void ExportCamera( ACameraActor* Actor, USeqAct_Interp* MatineeSequence );

	/**
	 * Exports the mesh and the actor information for a UE3 brush actor.
	 */
	virtual void ExportBrush(ABrush* Actor);

	/**
	 * Exports the basic scene information to the FBX document.
	 */
	virtual void ExportLevelMesh( ULevel* Level, USeqAct_Interp* MatineeSequence );

	/**
	 * Exports the given Matinee sequence information into a FBX document.
	 */
	virtual void ExportMatinee(class USeqAct_Interp* MatineeSequence);

	/**
	 * Exports the mesh and the actor information for a UE3 static mesh actor.
	 */
	virtual void ExportStaticMesh( AActor* Actor, UStaticMeshComponent* StaticMeshComponent, USeqAct_Interp* MatineeSequence );

private:
	FbxExporter();
	~FbxExporter();
	
	KFbxSdkManager* FbxSdkManager;
	KFbxScene* FbxScene;
	KFbxAnimStack* AnimStack;
	
	CBasicDataConverter Converter;
	
	TMap<FString,INT> FbxNodeNameToIndexMap;
	TMap<AActor*, KFbxNode*> FbxActors;
	TMap<UMaterial*, KFbxSurfaceMaterial*> FbxMaterials;
	
	/** The frames-per-second (FPS) used when baking transforms */
	static const FLOAT BakeTransformsFPS;
	
	
	void ExportModel(UModel* Model, KFbxNode* Node, const char* Name);
	
	/**
	 * Exports the basic information about a UE3 actor and buffers it.
	 * This function creates one FBX node for the actor with its placement.
	 */
	KFbxNode* ExportActor(AActor* Actor, USeqAct_Interp* MatineeSequence );
	
	KFbxNode* ExportStaticMeshToFbx(FStaticMeshRenderData& RenderMesh, const TCHAR* MeshName, KFbxNode* FbxActor);

	/**
	 * Exports the Matinee movement track into the FBX animation stack.
	 */
	void ExportMatineeTrackMove(KFbxNode* FbxActor, UInterpTrackInstMove* MoveTrackInst, UInterpTrackMove* MoveTrack, FLOAT InterpLength);

	/**
	 * Exports the Matinee float property track into the FBX animation stack.
	 */
	void ExportMatineeTrackFloatProp(KFbxNode* FbxActor, UInterpTrackFloatProp* PropTrack);

	/**
	 * Exports a given interpolation curve into the FBX animation curve.
	 */
	void ExportAnimatedVector(KFbxAnimCurve* FbxCurve, const ANSICHAR* ChannelName, UInterpTrackMove* MoveTrack, UInterpTrackInstMove* MoveTrackInst, UBOOL bPosCurve, INT CurveIndex, UBOOL bNegative, FLOAT InterpLength);
	
	/**
	 * Exports a movement subtrack to an FBX curve
	 */
	void ExportMoveSubTrack(KFbxAnimCurve* FbxCurve, const ANSICHAR* ChannelName, UInterpTrackMoveAxis* SubTrack, UInterpTrackInstMove* MoveTrackInst, UBOOL bPosCurve, INT CurveIndex, UBOOL bNegative, FLOAT InterpLength);
	
	void ExportAnimatedFloat(KFbxProperty* FbxProperty, FInterpCurveFloat* Curve);

	/**
	 * Finds the given UE3 actor in the already-exported list of structures
	 * @return KFbxNode* the FBX node created from the UE3 actor
	 */
	KFbxNode* FindActor(AActor* Actor);
	
	/**
	 * Exports the profile_COMMON information for a UE3 material.
	 */
	KFbxSurfaceMaterial* ExportMaterial(UMaterial* Material);
	
	KFbxSurfaceMaterial* CreateDefaultMaterial();
	
	/**
	 * Create user property in Fbx Node.
	 * Some Unreal animatable property can't be animated in FBX property. So create user property to record the animation of property.
	 *
	 * @param Node  FBX Node the property append to.
	 * @param Value Property value.
	 * @param Name  Property name.
	 * @param Label Property label.
	 */
	void CreateAnimatableUserProperty(KFbxNode* Node, FLOAT Value, const char* Name, const char* Label);
};



} // namespace UnFbx



#endif // __UNFBXEXPORTER_H__

#endif // WITH_FBX
