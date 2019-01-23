/*
* Copyright 2009 - 2010 Autodesk, Inc.  All Rights Reserved.
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
	Main implementation of CFbxImporter : import FBX data to Unreal
=============================================================================*/

#include "UnrealEd.h"
#include "FFeedbackContextEditor.h"

#if WITH_FBX

#include "Factories.h"
#include "Engine.h"

#include "SkelImport.h"
#include "EnginePrefabClasses.h"
#include "EngineAnimClasses.h"
#include "UnFbxImporter.h"

namespace UnFbx
{

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
CFbxImporter::CFbxImporter()
	: bFirstMesh(TRUE)
{
	UnrealFBXMemoryAllocator MyUnrealFBXMemoryAllocator;

	// Specify a custom memory allocator to be used by the FBX SDK
	KFbxSdkManager::SetMemoryAllocator(&MyUnrealFBXMemoryAllocator);

	// Create the SdkManager
	FbxSdkManager = KFbxSdkManager::Create();
	
	// create an IOSettings object
	KFbxIOSettings * ios = KFbxIOSettings::Create(FbxSdkManager, IOSROOT );
	FbxSdkManager->SetIOSettings(ios);

	// Create the geometry converter
	FbxGeometryConverter = new KFbxGeometryConverter(FbxSdkManager);
	FbxScene = NULL;
	
	ImportOptions = new FBXImportOptions();
	
	CurPhase = NOTSTARTED;
}
	
//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
CFbxImporter::~CFbxImporter()
{
	CleanUp();
}

const FLOAT CFbxImporter::SCALE_TOLERANCE = 0.000001;

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
CFbxImporter* CFbxImporter::GetInstance()
{
	static CFbxImporter* ImporterInstance = NULL;
	
	if (ImporterInstance == NULL)
	{
		ImporterInstance = new CFbxImporter();
	}
	return ImporterInstance;
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
void CFbxImporter::CleanUp()
{
	ReleaseScene();
	
	if (FbxSdkManager)
	{
		FbxSdkManager->Destroy();
	}
	FbxSdkManager = NULL;
	delete FbxGeometryConverter;
	FbxGeometryConverter = NULL;
	delete ImportOptions;
	ImportOptions = NULL;
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
void CFbxImporter::ReleaseScene()
{
	if (Importer)
	{
		Importer->Destroy();
		Importer = NULL;
	}
	
	if (FbxScene)
	{
		FbxScene->Destroy();
		FbxScene = NULL;
	}
	
	// reset
	CollisionModels.Clear();
	SkelMeshToMorphMap.Reset();
	SkelMeshToMorphMap.Shrink();
	CurPhase = NOTSTARTED;
	bFirstMesh = TRUE;
}

FBXImportOptions* UnFbx::CFbxImporter::GetImportOptions()
{
	return ImportOptions;
}

INT CFbxImporter::DetectDeformer(const FFilename& InFilename)
{
	INT Result = 0;
	FString Filename = InFilename;
	
	if (OpenFile(Filename, TRUE))
	{
		KFbxStatistics Statistics;
		Importer->GetStatistics(&Statistics);
		INT ItemIndex;
		KString ItemName;
		INT ItemCount;
		for ( ItemIndex = 0; ItemIndex < Statistics.GetNbItems(); ItemIndex++ )
		{
			Statistics.GetItemPair(ItemIndex, ItemName, ItemCount);
			if ( ItemName == "Deformer" && ItemCount > 0 )
			{
				Result = 1;
				break;
			}
		}
		Importer->Destroy();
		Importer = NULL;
		CurPhase = NOTSTARTED;
	}
	else
	{
		Result = -1;
	}
	
	return Result; 
}

UBOOL CFbxImporter::GetSceneInfo(FString Filename, FbxSceneInfo& SceneInfo)
{
	UBOOL Result = TRUE;
	FFeedbackContextEditor FbxImportWarn;
	FbxImportWarn.BeginSlowTask( TEXT("Parse FBX file to get scene info"), TRUE );
	
	switch (CurPhase)
	{
	case NOTSTARTED:
		if (!OpenFile(Filename, FALSE))
		{
			Result = FALSE;
			break;
		}
		FbxImportWarn.UpdateProgress( 40, 100 );
	case FILEOPENED:
		if (!ImportFile(Filename))
		{
			Result = FALSE;
			break;
		}
		FbxImportWarn.UpdateProgress( 90, 100 );
	case IMPORTED:
	
	default:
		break;
	}
	
	if (Result)
	{
		KTime GlobalStart(KTIME_INFINITE);
		KTime GlobalEnd(KTIME_MINUS_INFINITE);
		
		TArray<KFbxNode*> LinkNodes;
		
		SceneInfo.TotalMaterialNum = FbxScene->GetMaterialCount();
		SceneInfo.TotalTextureNum = FbxScene->GetTextureCount();
		SceneInfo.TotalGeometryNum = 0;
		SceneInfo.NonSkinnedMeshNum = 0;
		SceneInfo.SkinnedMeshNum = 0;
		for ( INT GeometryIndex = 0; GeometryIndex < FbxScene->GetGeometryCount(); GeometryIndex++ )
		{
			KFbxGeometry * Geometry = FbxScene->GetGeometry(GeometryIndex);
			
			if (Geometry->GetAttributeType() == KFbxNodeAttribute::eMESH)
			{
				KFbxNode* GeoNode = Geometry->GetNode();
				UBOOL bIsLinkNode = FALSE;

				// check if this geometry node is used as link
				for ( INT i = 0; i < LinkNodes.Num(); i++ )
				{
					if ( GeoNode == LinkNodes(i))
					{
						bIsLinkNode = TRUE;
						break;
					}
				}
				// if the geometry node is used as link, ignore it
				if (bIsLinkNode)
				{
					continue;
				}

				SceneInfo.TotalGeometryNum++;
				
				KFbxMesh* Mesh = (KFbxMesh*)Geometry;
				SceneInfo.MeshInfo.Add();
				FbxMeshInfo& MeshInfo = SceneInfo.MeshInfo.Last();
				MeshInfo.Name = MakeName(GeoNode->GetName());
				MeshInfo.bTriangulated = Mesh->IsTriangleMesh();
				MeshInfo.MaterialNum = GeoNode->GetMaterialCount();
				MeshInfo.FaceNum = Mesh->GetPolygonCount();
				MeshInfo.VertexNum = Mesh->GetControlPointsCount();
				
				// LOD info
				MeshInfo.LODGroup = NULL;
				KFbxNode* ParentNode = GeoNode->GetParent();
				if ( ParentNode->GetNodeAttribute() && ParentNode->GetNodeAttribute()->GetAttributeType() == KFbxNodeAttribute::eLODGROUP)
				{
					KFbxNodeAttribute* LODGroup = ParentNode->GetNodeAttribute();
					MeshInfo.LODGroup = MakeName(ParentNode->GetName());
					for (INT LODIndex = 0; LODIndex < ParentNode->GetChildCount(); LODIndex++)
					{
						if ( GeoNode == ParentNode->GetChild(LODIndex))
						{
							MeshInfo.LODLevel = LODIndex;
							break;
						}
					}
				}
				
				// skeletal mesh
				if (Mesh->GetDeformerCount(KFbxDeformer::eSKIN) > 0)
				{
					SceneInfo.SkinnedMeshNum++;
					MeshInfo.bIsSkelMesh = TRUE;
					MeshInfo.MorphNum = Mesh->GetShapeCount();
					// skeleton root
					KFbxSkin* Skin = (KFbxSkin*)Mesh->GetDeformer(0, KFbxDeformer::eSKIN);
					KFbxNode* Link = Skin->GetCluster(0)->GetLink();
					while (Link->GetParent() && Link->GetParent()->GetSkeleton())
					{
						Link = Link->GetParent();
					}
					MeshInfo.SkeletonRoot = MakeName(Link->GetName());
					MeshInfo.SkeletonElemNum = Link->GetChildCount(true);
					
					KTime Start(KTIME_INFINITE);
					KTime End(KTIME_MINUS_INFINITE);
					Link->GetAnimationInterval(Start,End);
					if ( Start < GlobalStart )
					{
						GlobalStart = Start;
					}
					if ( End > GlobalEnd )
					{
						 GlobalEnd = End;
					}
				}
				else
				{
					SceneInfo.NonSkinnedMeshNum++;
					MeshInfo.bIsSkelMesh = FALSE;
					MeshInfo.SkeletonRoot = NULL;
				}
			}
		}
		
		// TODO: display multiple anim stack
		SceneInfo.TakeName = NULL;
		for( INT AnimStackIndex = 0; AnimStackIndex < FbxScene->GetSrcObjectCount(KFbxAnimStack::ClassId); AnimStackIndex++ )
		{
			KFbxAnimStack* CurAnimStack = KFbxCast<KFbxAnimStack>(FbxScene->GetSrcObject(KFbxAnimStack::ClassId, 0));
			// TODO: skip empty anim stack
			const char* AnimStackName = CurAnimStack->GetName();
			SceneInfo.TakeName = new char[strlen(AnimStackName) + 1];
			strcpy_s(SceneInfo.TakeName, strlen(AnimStackName) + 1, AnimStackName);
		}
		SceneInfo.FrameRate = KTime::GetFrameRate(FbxScene->GetGlobalSettings().GetTimeMode());
		
		if ( GlobalEnd > GlobalStart )
		{
			SceneInfo.TotalTime = (GlobalEnd.GetMilliSeconds() - GlobalStart.GetMilliSeconds())/1000.f * SceneInfo.FrameRate;
		}
		else
		{
			SceneInfo.TotalTime = 0;
		}
	}
	
	FbxImportWarn.EndSlowTask();
	return Result;
}


UBOOL CFbxImporter::OpenFile(FString Filename, UBOOL bParseStatistics)
{
	UBOOL Result = TRUE;
	
	if (CurPhase != NOTSTARTED)
	{
		// something go wrong
		return FALSE;
	}

	INT SDKMajor,  SDKMinor,  SDKRevision;

	// Create an importer.
	Importer = KFbxImporter::Create(FbxSdkManager,"");

	// Get the version number of the FBX files generated by the
	// version of FBX SDK that you are using.
	KFbxSdkManager::GetFileFormatVersion(SDKMajor, SDKMinor, SDKRevision);

	// Initialize the importer by providing a filename.
	if (bParseStatistics)
	{
		Importer->ParseForStatistics(true);
	}
	
	const UBOOL bImportStatus = Importer->Initialize(TCHAR_TO_ANSI(*Filename));

	if( !bImportStatus )  // Problem with the file to be imported
	{
		warnf(NAME_Error,TEXT("Call to KFbxImporter::Initialize() failed."));
		warnf(TEXT("Error returned: %s"), Importer->GetLastErrorString());

		if (Importer->GetLastErrorID() ==
			KFbxIO::eFILE_VERSION_NOT_SUPPORTED_YET ||
			Importer->GetLastErrorID() ==
			KFbxIO::eFILE_VERSION_NOT_SUPPORTED_ANYMORE)
		{
			warnf(TEXT("FBX version number for this FBX SDK is %d.%d.%d"),
				SDKMajor, SDKMinor, SDKRevision);
		}

		return FALSE;
	}

	CurPhase = FILEOPENED;
	// Destroy the importer
	//Importer->Destroy();

	return Result;
}

#ifdef IOS_REF
#undef  IOS_REF
#define IOS_REF (*(FbxSdkManager->GetIOSettings()))
#endif

UBOOL CFbxImporter::ImportFile(FString Filename)
{
	UBOOL Result = TRUE;
	
	UBOOL bStatus;
	
	FFilename FilePath(Filename);
	FileBasePath = FilePath.GetPath();

	// Create the Scene
	FbxScene = KFbxScene::Create(FbxSdkManager,"");
	warnf(TEXT("Loading FBX Scene from %s"), *Filename);

	INT FileMajor, FileMinor, FileRevision;

	IOS_REF.SetBoolProp(IMP_FBX_MATERIAL,		true);
	IOS_REF.SetBoolProp(IMP_FBX_TEXTURE,		 true);
	IOS_REF.SetBoolProp(IMP_FBX_LINK,			true);
	IOS_REF.SetBoolProp(IMP_FBX_SHAPE,		   true);
	IOS_REF.SetBoolProp(IMP_FBX_GOBO,			true);
	IOS_REF.SetBoolProp(IMP_FBX_ANIMATION,	   true);
	IOS_REF.SetBoolProp(IMP_SKINS,			   true);
	IOS_REF.SetBoolProp(IMP_DEFORMATION,		 true);
	IOS_REF.SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
	IOS_REF.SetBoolProp(IMP_TAKE,				true);

	// Import the scene.
	bStatus = Importer->Import(FbxScene);

	// Get the version number of the FBX file format.
	Importer->GetFileVersion(FileMajor, FileMinor, FileRevision);

	if(bStatus == FALSE &&	 // The import file may have a password
		Importer->GetLastErrorID() == KFbxIO::ePASSWORD_ERROR)
	{
		warnf(NAME_Error,TEXT("FBX file requires a password - unsupported."));
	}
	// output result
	if(bStatus)
	{
		warnf(TEXT("FBX Scene Loaded Succesfully"));
		CurPhase = IMPORTED;
	}
	else
	{
		ErrorMessage = ANSI_TO_TCHAR(FbxSdkManager->GetError().GetLastErrorString());
		warnf(NAME_Error,TEXT("FBX Scene Loading Failed : %s"),ANSI_TO_TCHAR(FbxSdkManager->GetError().GetLastErrorString()));
		CleanUp();
		Result = FALSE;
		CurPhase = NOTSTARTED;
	}
	
	Importer->Destroy();
	Importer = NULL;
	
	return Result;
}

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
UBOOL CFbxImporter::ImportFromFile(const TCHAR* Filename)
{
	UBOOL Result = TRUE;
	KFbxAxisSystem::eFrontVector FrontVector = (KFbxAxisSystem::eFrontVector)-KFbxAxisSystem::ParityOdd;
	const KFbxAxisSystem UnrealZUp(KFbxAxisSystem::ZAxis, FrontVector, KFbxAxisSystem::RightHanded);
	
	switch (CurPhase)
	{
	case NOTSTARTED:
		if (!OpenFile(FString(Filename), FALSE))
		{
			Result = FALSE;
			break;
		}
	case FILEOPENED:
		if (!ImportFile(FString(Filename)))
		{
			Result = FALSE;
			CurPhase = NOTSTARTED;
			break;
		}
	case IMPORTED:
		// convert axis to Z-up
		KFbxRootNodeUtility::RemoveAllFbxRoots( FbxScene );
		UnrealZUp.ConvertScene( FbxScene );


		// convert name to unreal-supported format
		// actually, crashes...
		//KFbxSceneRenamer renamer(FbxScene);
		//renamer.ResolveNameClashing(false,false,true,true,true,KString(),"TestBen",false,false);
		
	default:
		break;
	}
	
	return Result;
}

char* CFbxImporter::MakeName(const char* Name)
{
	int SpecialChars[] = {'.', ',', '/', '`', '%'};
	char* TmpName = new char[strlen(Name)+1];
	int len = strlen(Name);
	strcpy_s(TmpName, strlen(Name) + 1, Name);

	for ( INT i = 0; i < 5; i++ )
	{
		char* CharPtr = TmpName;
		while ( (CharPtr = strchr(CharPtr,SpecialChars[i])) != NULL )
		{
			CharPtr[0] = '_';
		}
	}

	// Remove namespaces
	char* NewName;
	NewName = strchr (TmpName, ':');
	  
	// there may be multiple namespace, so find the last ':'
	while (NewName && strchr(NewName + 1, ':'))
	{
		NewName = strchr(NewName + 1, ':');
	}

	if (NewName)
	{
		return NewName + 1;
	}

	return TmpName;
}

FName CFbxImporter::MakeNameForMesh(FString InName, KFbxObject* FbxObject)
{
	FName OutputName;

	// "Name" field can't be empty
	if (ImportOptions->bUsedAsFullName && InName != FString("None"))
	{
		OutputName = *InName;
	}
	else
	{
		char Name[512];
		int SpecialChars[] = {'.', ',', '/', '`', '%'};
		sprintf_s(Name,512,"%s",FbxObject->GetName());

		for ( INT i = 0; i < 5; i++ )
		{
			char* CharPtr = Name;
			while ( (CharPtr = strchr(CharPtr,SpecialChars[i])) != NULL )
			{
				CharPtr[0] = '_';
			}
		}

		// for mesh, replace ':' with '_' because Unreal doesn't support ':' in mesh name
		char* NewName = NULL;
		NewName = strchr (Name, ':');

		if (NewName)
		{
			char* Tmp;
			Tmp = NewName;
			while (Tmp)
			{

				// Always remove namespaces
				NewName = Tmp + 1;
				
				// there may be multiple namespace, so find the last ':'
				Tmp = strchr(NewName + 1, ':');
			}
		}
		else
		{
			NewName = Name;
		}

		if ( InName == FString("None"))
		{
			OutputName = FName( *FString::Printf(TEXT("%s"), ANSI_TO_TCHAR(NewName )) );
		}
		else
		{
			OutputName = FName( *FString::Printf(TEXT("%s_%s"), *InName,ANSI_TO_TCHAR(NewName)) );
		}
	}
	
	return OutputName;
}

KFbxXMatrix CFbxImporter::ComputeTotalMatrix(KFbxNode* Node)
{
	KFbxXMatrix Geometry;
	KFbxVector4 Translation, Rotation, Scaling;
	Translation = Node->GetGeometricTranslation(KFbxNode::eSOURCE_SET);
	Rotation = Node->GetGeometricRotation(KFbxNode::eSOURCE_SET);
	Scaling = Node->GetGeometricScaling(KFbxNode::eSOURCE_SET);
	Geometry.SetT(Translation);
	Geometry.SetR(Rotation);
	Geometry.SetS(Scaling);

	//For Single Matrix situation, obtain transfrom matrix from eDESTINATION_SET, which include pivot offsets and pre/post rotations.
	KFbxXMatrix& GlobalTransform = FbxScene->GetEvaluator()->GetNodeGlobalTransform(Node);
	
	KFbxXMatrix TotalMatrix;
	TotalMatrix = GlobalTransform * Geometry;

	return TotalMatrix;
}

/**
* Recursively get skeletal mesh count
*
* @param Node Root node to find skeletal meshes
* @return INT skeletal mesh count
*/
INT GetFbxSkeletalMeshCount(KFbxNode* Node)
{
	INT SkeletalMeshCount = 0;
	if (Node->GetMesh() && (Node->GetMesh()->GetDeformerCount(KFbxDeformer::eSKIN)>0))
	{
		SkeletalMeshCount = 1;
	}

	INT ChildIndex;
	for (ChildIndex=0; ChildIndex<Node->GetChildCount(); ++ChildIndex)
	{
		SkeletalMeshCount += GetFbxSkeletalMeshCount(Node->GetChild(ChildIndex));
	}

	return SkeletalMeshCount;
}

/**
* Get mesh count (including static mesh and skeletal mesh, except collision models) and find collision models
*
* @param Node Root node to find meshes
* @param FbxImporter
* @return INT mesh count
*/
INT CFbxImporter::GetFbxMeshCount(KFbxNode* Node, UnFbx::CFbxImporter* FbxImporter)
{
	INT MeshCount = 0;
	if (Node->GetMesh())
	{
		if (!FbxImporter->FillCollisionModelList(Node))
		{
			MeshCount = 1;
		}
	}

	INT ChildIndex;
	for (ChildIndex=0; ChildIndex<Node->GetChildCount(); ++ChildIndex)
	{
		MeshCount += GetFbxMeshCount(Node->GetChild(ChildIndex), FbxImporter);
	}

	return MeshCount;
}

/**
* Get all Fbx mesh objects
*
* @param Node Root node to find meshes
* @param outMeshArray return Fbx meshes
*/
void CFbxImporter::FillFbxMeshArray(KFbxNode* Node, TArray<KFbxNode*>& outMeshArray, UnFbx::CFbxImporter* FbxImporter)
{
	if (Node->GetMesh())
	{
		if (!FbxImporter->FillCollisionModelList(Node))
		{ 
			outMeshArray.AddItem(Node);
		}
	}

	INT ChildIndex;
	for (ChildIndex=0; ChildIndex<Node->GetChildCount(); ++ChildIndex)
	{
		FillFbxMeshArray(Node->GetChild(ChildIndex), outMeshArray, FbxImporter);
	}
}

/**
* Get all Fbx skeletal mesh objects
*
* @param Node Root node to find skeletal meshes
* @param outSkelMeshArray return Fbx meshes
*/
void FillFbxSkelMeshArray(KFbxNode* Node, TArray<KFbxNode*>& outSkelMeshArray)
{
	if (Node->GetMesh() && Node->GetMesh()->GetDeformerCount(KFbxDeformer::eSKIN) > 0 )
	{
		outSkelMeshArray.AddItem(Node);
	}

	INT ChildIndex;
	for (ChildIndex=0; ChildIndex<Node->GetChildCount(); ++ChildIndex)
	{
		FillFbxSkelMeshArray(Node->GetChild(ChildIndex), outSkelMeshArray);
	}
}

void CFbxImporter::RecursiveFixSkeleton(KFbxNode* Node)
{
	KFbxNodeAttribute* Attr = Node->GetNodeAttribute();
	KString n = Node->GetName();
	const char* name = n.Buffer();
	if (Attr && (Attr->GetAttributeType() == KFbxNodeAttribute::eMESH || Attr->GetAttributeType() == KFbxNodeAttribute::eNULL))
	{
		//replace with skeleton
		KFbxSkeleton* FbxSkeleton = KFbxSkeleton::Create(FbxSdkManager,"");
		Node->SetNodeAttribute(FbxSkeleton);
		FbxSkeleton->SetSkeletonType(KFbxSkeleton::eLIMB_NODE);
	}

	for (INT i = 0; i < Node->GetChildCount(); i++)
	{
		RecursiveFixSkeleton(Node->GetChild(i));
	}
}

KFbxNode* CFbxImporter::GetRootSkeleton(KFbxNode* Link)
{
	KFbxNode* RootBone = Link;
	// get FBX skeleton root
	while (RootBone->GetParent() && RootBone->GetParent()->GetSkeleton())
	{
		RootBone = RootBone->GetParent();
	}

	// get Unreal skeleton root
	// mesh and dummy are used as bone if they are in the skeleton hierarchy
	while (RootBone->GetParent())
	{
		KFbxNodeAttribute* Attr = RootBone->GetParent()->GetNodeAttribute();
		if (Attr && 
			(Attr->GetAttributeType() == KFbxNodeAttribute::eMESH || Attr->GetAttributeType() == KFbxNodeAttribute::eNULL) &&
			RootBone->GetParent() != FbxScene->GetRootNode())
		{
			// in some case, skeletal mesh can be ancestor of bones
			// this avoids this situation
			if (Attr->GetAttributeType() == KFbxNodeAttribute::eMESH )
			{
				KFbxMesh* FbxMesh = (KFbxMesh*)Attr;
				if (FbxMesh->GetDeformerCount(KFbxDeformer::eSKIN) > 0)
				{
					break;
				}
			}

			RootBone = RootBone->GetParent();
		}
		else
		{
			break;
		}
	}

	return RootBone;
}

/**
* Get all Fbx skeletal mesh objects which are grouped by skeleton they bind to
*
* @param Node Root node to find skeletal meshes
* @param outSkelMeshArray return Fbx meshes they are grouped by skeleton
* @param SkeletonArray
* @param ExpandLOD flag of expanding LOD to get each mesh
*/
void CFbxImporter::RecursiveFindFbxSkelMesh(KFbxNode* Node, TArray< TArray<KFbxNode*>* >& outSkelMeshArray, TArray<KFbxNode*>& SkeletonArray, UBOOL ExpandLOD)
{
	KFbxNode* SkelMeshNode = NULL;
	KFbxNode* NodeToAdd = Node;

	if (Node->GetMesh() && Node->GetMesh()->GetDeformerCount(KFbxDeformer::eSKIN) > 0 )
	{
		SkelMeshNode = Node;
	}
	else if (Node->GetNodeAttribute() && Node->GetNodeAttribute()->GetAttributeType() == KFbxNodeAttribute::eLODGROUP)
	{
		// for LODgroup, add the LODgroup to OutSkelMeshArray according to the skeleton that the first child bind to
		SkelMeshNode = Node->GetChild(0);
		// check if the first child is skeletal mesh
		if (!(SkelMeshNode->GetMesh() && SkelMeshNode->GetMesh()->GetDeformerCount(KFbxDeformer::eSKIN) > 0))
		{
			SkelMeshNode = NULL;
		}
		else if (ExpandLOD)
		{
			// if ExpandLOD is TRUE, only add the first LODGroup level node
			NodeToAdd = SkelMeshNode;
		}
		// else NodeToAdd = Node;
	}

	if (SkelMeshNode)
	{
		// find root skeleton
		KFbxSkin* Deformer = (KFbxSkin*)SkelMeshNode->GetMesh()->GetDeformer(0);
		KFbxNode* Link = Deformer->GetCluster(0)->GetLink();
		Link = GetRootSkeleton(Link);

		INT i;
		for (i = 0; i < SkeletonArray.Num(); i++)
		{
			if ( Link == SkeletonArray(i) || Link == SkeletonArray(i)->GetParent() )
			{
				// append to existed outSkelMeshArray element
				TArray<KFbxNode*>* TempArray = outSkelMeshArray(i);
				TempArray->AddItem(NodeToAdd);
				break;
			}
		}

		// if there is no outSkelMeshArray element that is bind to this skeleton
		// create new element for outSkelMeshArray
		if ( i == SkeletonArray.Num() )
		{
			TArray<KFbxNode*>* TempArray = new TArray<KFbxNode*>();
			TempArray->AddItem(NodeToAdd);
			outSkelMeshArray.AddItem(TempArray);
			SkeletonArray.AddItem(Link);
		}
	}
	else
	{
		INT ChildIndex;
		for (ChildIndex=0; ChildIndex<Node->GetChildCount(); ++ChildIndex)
		{
			RecursiveFindFbxSkelMesh(Node->GetChild(ChildIndex), outSkelMeshArray, SkeletonArray, ExpandLOD);
		}
	}
}

void CFbxImporter::RecursiveFindRigidMesh(KFbxNode* Node, TArray< TArray<KFbxNode*>* >& outSkelMeshArray, TArray<KFbxNode*>& SkeletonArray, UBOOL ExpandLOD)
{
	UBOOL RigidNodeFound = FALSE;
	KFbxNode* RigidMeshNode = NULL;

	if (Node->GetMesh())
	{
		// ignore skeletal mesh
		if (Node->GetMesh()->GetDeformerCount(KFbxDeformer::eSKIN) == 0 )
		{
			for (INT MatIndex = 0; !RigidNodeFound && MatIndex < Node->GetMaterialCount(); MatIndex++)
			{
				KFbxSurfaceMaterial* Mat = Node->GetMaterial(MatIndex);
				INT TexIndex;
				FOR_EACH_TEXTURE(TexIndex)
				{
					KFbxProperty FbxProperty = Mat->FindProperty(KFbxLayerElement::TEXTURE_CHANNEL_NAMES[TexIndex]);
					if( FbxProperty.IsValid() && FbxProperty.GetSrcObjectCount(KFbxTexture::ClassId))
					{
						// texture found
						RigidMeshNode = Node;
						RigidNodeFound = TRUE;
						break;
					}
				}
			}
		}
	}
	else if (Node->GetNodeAttribute() && Node->GetNodeAttribute()->GetAttributeType() == KFbxNodeAttribute::eLODGROUP)
	{
		// for LODgroup, add the LODgroup to OutSkelMeshArray according to the skeleton that the first child bind to
		KFbxNode* FirstLOD = Node->GetChild(0);
		// check if the first child is skeletal mesh
		if (FirstLOD->GetMesh())
		{
			if (FirstLOD->GetMesh()->GetDeformerCount(KFbxDeformer::eSKIN) == 0 )
			{
				for (INT MatIndex = 0; !RigidNodeFound && MatIndex < RigidMeshNode->GetMaterialCount(); MatIndex++)
				{
					KFbxSurfaceMaterial* Mat = FirstLOD->GetMaterial(MatIndex);
					INT TexIndex;
					FOR_EACH_TEXTURE(TexIndex)
					{
						KFbxProperty FbxProperty = Mat->FindProperty(KFbxLayerElement::TEXTURE_CHANNEL_NAMES[TexIndex]);
						if( FbxProperty.IsValid() && FbxProperty.GetSrcObjectCount(KFbxTexture::ClassId))
						{
							// texture found
							RigidNodeFound = TRUE;
							break;
						}
					}
				}
			}
		}

		if (RigidNodeFound)
		{
			if (ExpandLOD)
			{
				RigidMeshNode = FirstLOD;
			}
			else
			{
				RigidMeshNode = Node;
			}

		}
	}

	if (RigidMeshNode)
	{
		// find root skeleton
		KFbxNode* Link = GetRootSkeleton(RigidMeshNode);

		INT i;
		for (i = 0; i < SkeletonArray.Num(); i++)
		{
			if ( Link == SkeletonArray(i))
			{
				// append to existed outSkelMeshArray element
				TArray<KFbxNode*>* TempArray = outSkelMeshArray(i);
				TempArray->AddItem(RigidMeshNode);
				break;
			}
		}

		// if there is no outSkelMeshArray element that is bind to this skeleton
		// create new element for outSkelMeshArray
		if ( i == SkeletonArray.Num() )
		{
			TArray<KFbxNode*>* TempArray = new TArray<KFbxNode*>();
			TempArray->AddItem(RigidMeshNode);
			outSkelMeshArray.AddItem(TempArray);
			SkeletonArray.AddItem(Link);
		}
	}

	// for LODGroup, we will not deep in.
	if (!(Node->GetNodeAttribute() && Node->GetNodeAttribute()->GetAttributeType() == KFbxNodeAttribute::eLODGROUP))
	{
		INT ChildIndex;
		for (ChildIndex=0; ChildIndex<Node->GetChildCount(); ++ChildIndex)
		{
			RecursiveFindRigidMesh(Node->GetChild(ChildIndex), outSkelMeshArray, SkeletonArray, ExpandLOD);
		}
	}
}

/**
* Get all Fbx skeletal mesh objects in the scene. these meshes are grouped by skeleton they bind to
*
* @param Node Root node to find skeletal meshes
* @param outSkelMeshArray return Fbx meshes they are grouped by skeleton
*/
void CFbxImporter::FillFbxSkelMeshArrayInScene(KFbxNode* Node, TArray< TArray<KFbxNode*>* >& outSkelMeshArray, UBOOL ExpandLOD)
{
	TArray<KFbxNode*> SkeletonArray;

	// a) find skeletal meshes
	
	RecursiveFindFbxSkelMesh(Node, outSkelMeshArray, SkeletonArray, ExpandLOD);
	// for skeletal mesh, we convert the skeleton system to skeleton
	// in less we recognize bone mesh as rigid mesh if they are textured
	for ( INT SkelIndex = 0; SkelIndex < SkeletonArray.Num(); SkelIndex++)
	{
		RecursiveFixSkeleton(SkeletonArray(SkelIndex));
	}

	SkeletonArray.Empty();
	// b) find rigid mesh
	
	// for rigid meshes, we don't convert to bone
	if (ImportOptions->bImportAnimSet && ImportOptions->bImportRigidAnim)
	{
		RecursiveFindRigidMesh(Node, outSkelMeshArray, SkeletonArray, ExpandLOD);
	}
}

KFbxNode* CFbxImporter::FindFBXMeshesByBone(USkeletalMesh* FillInMesh, UBOOL bExpandLOD, TArray<KFbxNode*>& OutFBXMeshNodeArray)
{
	// get the root bone of Unreal skeletal mesh
	FMeshBone& Bone = FillInMesh->RefSkeleton(0);
	FString BoneNameString = Bone.Name.ToString();
	const char *BoneName = TCHAR_TO_ANSI(*BoneNameString);

	// we do not need to check if the skeleton root node is a skeleton
	// because the animation may be a rigid animation
	KFbxNode* SkeletonRoot = NULL;

	// find the FBX skeleton node according to name
	SkeletonRoot = FbxScene->FindNodeByName(TCHAR_TO_ANSI(*BoneNameString));

	// take the namespace into account, match name without namespace
	// when import the skeletal mesh, the namespace in node name may be stripped
	if (!SkeletonRoot)
	{
		for (INT NodeIndex = 0; NodeIndex < FbxScene->GetNodeCount(); NodeIndex++)
		{
			KFbxNode* FbxNode = FbxScene->GetNode(NodeIndex);
			char NodeName[512];
			sprintf_s(NodeName, 512, "%s", FbxNode->GetName());
			char* NameRmNS = strchr (NodeName, ':');

			// there may be multiple namespace, so find the last ':'
			while (NameRmNS && strchr(NameRmNS + 1, ':'))
			{
				NameRmNS = strchr(NameRmNS + 1, ':');
			}

			if (NameRmNS)
			{
				NameRmNS = NameRmNS + 1;
			}
			else
			{
				NameRmNS = NodeName;
			}

			if (!strcmp(NameRmNS, TCHAR_TO_ANSI(*BoneNameString)))
			{
				// name is matched after strip the namespace
				SkeletonRoot = FbxNode;
				break;
			}
		}
	}

	// return if do not find matched FBX skeleton
	if (!SkeletonRoot)
	{
		return NULL;
	}
	

	// Get Mesh nodes array that bind to the skeleton system
	// 1, get all skeltal meshes in the FBX file
	TArray< TArray<KFbxNode*>* > SkelMeshArray;
	FillFbxSkelMeshArrayInScene(FbxScene->GetRootNode(), SkelMeshArray, FALSE);

	// 2, then get skeletal meshes that bind to this skeleton
	for (INT SkelMeshIndex = 0; SkelMeshIndex < SkelMeshArray.Num(); SkelMeshIndex++)
	{
		KFbxNode* FbxNode = (*SkelMeshArray(SkelMeshIndex))(0);
		KFbxNode* MeshNode;
		if (FbxNode->GetNodeAttribute() && FbxNode->GetNodeAttribute()->GetAttributeType() == KFbxNodeAttribute::eLODGROUP)
		{
			MeshNode = FbxNode->GetChild(0);
		}
		else
		{
			MeshNode = FbxNode;
		}

		// 3, get the root bone that the mesh bind to
		KFbxSkin* Deformer = (KFbxSkin*)MeshNode->GetMesh()->GetDeformer(0);
		KFbxNode* Link = Deformer->GetCluster(0)->GetLink();
		Link = GetRootSkeleton(Link);
		// 4, fill in the mesh node
		if (Link == SkeletonRoot)
		{
			// copy meshes
			if (bExpandLOD)
			{
				TArray<KFbxNode*> SkelMeshes = 	*SkelMeshArray(SkelMeshIndex);
				for (INT NodeIndex = 0; NodeIndex < SkelMeshes.Num(); NodeIndex++)
				{
					KFbxNode* Node = SkelMeshes(NodeIndex);
					if (Node->GetNodeAttribute() && Node->GetNodeAttribute()->GetAttributeType() == KFbxNodeAttribute::eLODGROUP)
					{
						OutFBXMeshNodeArray.AddItem(Node->GetChild(0));
					}
					else
					{
						OutFBXMeshNodeArray.AddItem(Node);
					}
				}
			}
			else
			{
				OutFBXMeshNodeArray.Append(*SkelMeshArray(SkelMeshIndex));
			}
			break;
		}
	}

	for (INT i = 0; i < SkelMeshArray.Num(); i++)
	{
		delete SkelMeshArray(i);
	}

	return SkeletonRoot;
}

/**
* Get the first Fbx mesh node.
*
* @param Node Root node
* @param bIsSkelMesh if we want a skeletal mesh
* @return KFbxNode* the node containing the first mesh
*/
KFbxNode* GetFirstFbxMesh(KFbxNode* Node, UBOOL bIsSkelMesh)
{
	if (Node->GetMesh())
	{
		if (bIsSkelMesh)
		{
			if (Node->GetMesh()->GetDeformerCount(KFbxDeformer::eSKIN)>0)
			{
				return Node;
			}
		}
		else
		{
			return Node;
		}
	}

	INT ChildIndex;
	for (ChildIndex=0; ChildIndex<Node->GetChildCount(); ++ChildIndex)
	{
		KFbxNode* FirstMesh;
		FirstMesh = GetFirstFbxMesh(Node->GetChild(ChildIndex), bIsSkelMesh);

		if (FirstMesh)
		{
			return FirstMesh;
		}
	}

	return NULL;
}

void CFbxImporter::CheckSmoothingInfo(KFbxMesh* FbxMesh)
{
	if (bFirstMesh)
	{
		bFirstMesh = FALSE;	 // don't check again
		
		KFbxLayer* LayerSmoothing = FbxMesh->GetLayer(0, KFbxLayerElement::eSMOOTHING);
		if (!LayerSmoothing)
		{
			appMsgf( AMT_OK, *LocalizeUnrealEd("Prompt_NoSmoothgroupForFBXScene"));
		}
	}
}


//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
KFbxNode* CFbxImporter::RetrieveObjectFromName(const TCHAR* ObjectName, KFbxNode* Root)
{
	KFbxNode* Result = NULL;
	
	if ( FbxScene != NULL )
	{
		if (Root == NULL)
		{
			Root = FbxScene->GetRootNode();
		}

		for (INT ChildIndex=0;ChildIndex<Root->GetChildCount() && !Result;++ChildIndex)
		{
			KFbxNode* Node = Root->GetChild(ChildIndex);
			KFbxMesh* FbxMesh = Node->GetMesh();
			if (FbxMesh && 0 == appStrcmp(ObjectName,ANSI_TO_TCHAR(Node->GetName())))
			{
				Result = Node;
			}
			else
			{
				Result = RetrieveObjectFromName(ObjectName,Node);
			}
		}
	}
	return Result;
}

} // namespace UnFbx

#endif //WITH_FBX
