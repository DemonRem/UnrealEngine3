/*
* Copyright 2009 Autodesk, Inc.  All Rights Reserved.
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
	Static mesh creation from FBX data.
	Largely based on UnStaticMeshEdit.cpp
=============================================================================*/

#include "UnrealEd.h"

#if WITH_FBX
#include "Factories.h"
#include "Engine.h"
#include "UnTextureLayout.h"
#include "UnFracturedStaticMesh.h"
#include "EnginePhysicsClasses.h"
#include "BSPOps.h"
#include "EngineMaterialClasses.h"
#include "EngineInterpolationClasses.h"
#include "UnLinkedObjDrawUtils.h"

#include "UnFbxImporter.h"

using namespace UnFbx;

struct ExistingStaticMeshData;
extern ExistingStaticMeshData* SaveExistingStaticMeshData(UStaticMesh* ExistingMesh);
extern void RestoreExistingMeshData(struct ExistingStaticMeshData* ExistingMeshDataPtr, UStaticMesh* NewMesh);

//-------------------------------------------------------------------------
//
//-------------------------------------------------------------------------
UObject* UnFbx::CFbxImporter::ImportStaticMesh(UObject* InParent, KFbxNode* Node, const FName& Name, EObjectFlags Flags, UStaticMesh* InStaticMesh, int LODIndex)
{
	TArray<KFbxNode*> MeshNodeArray;
	
	if ( !Node->GetMesh())
	{
		return NULL;
	}
	
	MeshNodeArray.AddItem(Node);
	return ImportStaticMeshAsSingle(InParent, MeshNodeArray, Name, Flags, InStaticMesh, LODIndex);
}

UBOOL UnFbx::CFbxImporter::BuildStaticMeshFromGeometry(KFbxMesh* FbxMesh, UStaticMesh* StaticMesh, int LODIndex)
{
	KFbxNode* Node = FbxMesh->GetNode();

    //remove the bad polygons before getting any data from mesh
    FbxMesh->RemoveBadPolygons();

    //Get the base layer of the mesh
    KFbxLayer* BaseLayer = FbxMesh->GetLayer(0);
    if (BaseLayer == NULL)
    {
        warnf(NAME_Error,TEXT("There is no geometry information in mesh"),ANSI_TO_TCHAR(FbxMesh->GetName()));
        return FALSE;
    }

	//
	//	store the UVs in arrays for fast access in the later looping of triangles 
	//
	// mapping from UVSets to Fbx LayerElementUV
	// Fbx UVSets may be duplicated, remove the duplicated UVSets in the mapping 
	INT LayerCount = FbxMesh->GetLayerCount();
	TArray<FString> UVSets;
	UVSets.Empty();
	if (LayerCount > 0)
	{
		INT UVLayerIndex;
		for (UVLayerIndex = 0; UVLayerIndex<LayerCount; UVLayerIndex++)
		{
			KFbxLayer* lLayer = FbxMesh->GetLayer(UVLayerIndex);
			int UVSetCount = lLayer->GetUVSetCount();
			if(UVSetCount)
			{
				KArrayTemplate<KFbxLayerElementUV const*> EleUVs = lLayer->GetUVSets();
				for (int UVIndex = 0; UVIndex<UVSetCount; UVIndex++)
				{
					KFbxLayerElementUV const* ElementUV = EleUVs[UVIndex];
					if (ElementUV)
					{
						const char* UVSetName = ElementUV->GetName();
						FString LocalUVSetName = ANSI_TO_TCHAR(UVSetName);

						UVSets.AddUniqueItem(LocalUVSetName);
					}
				}
			}
		}
	}

	//
	// create materials
	//
	INT MaterialCount = 0;
	INT MaterialIndex;
	TArray<UMaterial*> Materials;
	Materials.Empty();
	if ( ImportOptions->bImportMaterials )
	{
		CreateNodeMaterials(Node,Materials,UVSets);
	}
	else if ( ImportOptions->bImportTextures )
	{
		ImportTexturesFromNode(Node);
	}
	
	MaterialCount = Node->GetMaterialCount();
	
	INT NewMaterialIndex = StaticMesh->LODModels(LODIndex).Elements.Num();
	for (MaterialIndex=0; MaterialIndex<MaterialCount; MaterialIndex++,NewMaterialIndex++)
	{
		KFbxSurfaceMaterial *FbxMaterial = Node->GetMaterial(MaterialIndex);
		FString MaterialFullName = ANSI_TO_TCHAR(MakeName(FbxMaterial->GetName()));
		
		if ( !ImportOptions->bImportMaterials )
		{
			UMaterial* UnrealMaterial = FindObject<UMaterial>(Parent,*MaterialFullName);
			Materials.AddItem(UnrealMaterial);
		}
		
		new(StaticMesh->LODModels(LODIndex).Elements) FStaticMeshElement(NULL,NewMaterialIndex);
		StaticMesh->LODModels(LODIndex).Elements(NewMaterialIndex).Name = MaterialFullName;
		StaticMesh->LODModels(LODIndex).Elements(NewMaterialIndex).Material = Materials(MaterialIndex);
		// Enable per poly collision if we do it globally, and this is the first lod index
		StaticMesh->LODModels(LODIndex).Elements(NewMaterialIndex).EnableCollision = GBuildStaticMeshCollision && LODIndex == 0 && ImportOptions->bRemoveDegenerates;
		StaticMesh->LODInfo(LODIndex).Elements.Add();
		StaticMesh->LODInfo(LODIndex).Elements(NewMaterialIndex).Material = Materials(MaterialIndex);
	}

	if ( MaterialCount == 0 )
	{
		UPackage* EngineMaterialPackage = UObject::FindPackage(NULL,TEXT("EngineMaterials"));
		UMaterial* DefaultMaterial = FindObject<UMaterial>(EngineMaterialPackage,TEXT("DefaultMaterial"));
		if (DefaultMaterial)
		{
			const INT NewMaterialIndex = StaticMesh->LODModels(LODIndex).Elements.Num();
			new(StaticMesh->LODModels(LODIndex).Elements)FStaticMeshElement(NULL,NewMaterialIndex);

			StaticMesh->LODModels(LODIndex).Elements(NewMaterialIndex).Material = DefaultMaterial;
			// Enable per poly collision if we do it globally, and this is the first lod index
			StaticMesh->LODModels(LODIndex).Elements(NewMaterialIndex).EnableCollision = GBuildStaticMeshCollision && LODIndex == 0 && ImportOptions->bRemoveDegenerates;
			StaticMesh->LODInfo(LODIndex).Elements.Add();
			StaticMesh->LODInfo(LODIndex).Elements(NewMaterialIndex).Material = DefaultMaterial;
		}
	}

	//
	// Convert data format to unreal-compatible
	//

	// Must do this before triangulating the mesh due to an FBX bug in TriangulateMeshAdvance
	INT LayerSmoothingCount = FbxMesh->GetLayerCount(KFbxLayerElement::eSMOOTHING);
	for(INT i = 0; i < LayerSmoothingCount; i++)
	{
		FbxGeometryConverter->ComputePolygonSmoothingFromEdgeSmoothing (FbxMesh, i);
	}

	UBOOL bDestroyMesh = FALSE;
	if (!FbxMesh->IsTriangleMesh())
	{
		warnf(NAME_Log,TEXT("Triangulating static mesh %s"), ANSI_TO_TCHAR(Node->GetName()));
		bool bSuccess;
		FbxMesh = FbxGeometryConverter->TriangulateMeshAdvance(FbxMesh, bSuccess); // not in place ! the old mesh is still there
		if (FbxMesh == NULL)
		{
			warnf(NAME_Error,TEXT("Unable to triangulate mesh"));
			return FALSE; // not clean, missing some dealloc
		}
		// this gets deleted at the end of the import
		bDestroyMesh = TRUE;
	}
	
	// renew the base layer
	BaseLayer = FbxMesh->GetLayer(0);

	//
	//	get the "material index" layer.  Do this AFTER the triangulation step as that may reorder material indices
	//
	KFbxLayerElementMaterial* LayerElementMaterial = BaseLayer->GetMaterials();
	KFbxLayerElement::EMappingMode MaterialMappingMode = LayerElementMaterial ? 
		LayerElementMaterial->GetMappingMode() : KFbxLayerElement::eBY_POLYGON;

	//
	//	store the UVs in arrays for fast access in the later looping of triangles 
	//
	INT UniqueUVCount = UVSets.Num();
	KFbxLayerElementUV** LayerElementUV = NULL;
	KFbxLayerElement::EReferenceMode* UVReferenceMode = NULL;
	KFbxLayerElement::EMappingMode* UVMappingMode = NULL;
	if (UniqueUVCount > 0)
	{
		LayerElementUV = new KFbxLayerElementUV*[UniqueUVCount];
		UVReferenceMode = new KFbxLayerElement::EReferenceMode[UniqueUVCount];
		UVMappingMode = new KFbxLayerElement::EMappingMode[UniqueUVCount];
	}
	LayerCount = FbxMesh->GetLayerCount();
	for (INT UVIndex = 0; UVIndex < UniqueUVCount; UVIndex++)
	{
		UBOOL bFoundUV = FALSE;
		for (INT UVLayerIndex = 0; !bFoundUV && UVLayerIndex<LayerCount; UVLayerIndex++)
		{
			KFbxLayer* lLayer = FbxMesh->GetLayer(UVLayerIndex);
			int UVSetCount = lLayer->GetUVSetCount();
			if(UVSetCount)
			{
				KArrayTemplate<KFbxLayerElementUV const*> EleUVs = lLayer->GetUVSets();
				for (int FbxUVIndex = 0; FbxUVIndex<UVSetCount; FbxUVIndex++)
				{
					KFbxLayerElementUV const* ElementUV = EleUVs[FbxUVIndex];
					if (ElementUV)
					{
						const char* UVSetName = ElementUV->GetName();
						FString LocalUVSetName = ANSI_TO_TCHAR(UVSetName);
						if (LocalUVSetName == UVSets(UVIndex))
						{
							LayerElementUV[UVIndex] = const_cast<KFbxLayerElementUV*>(ElementUV);
							UVReferenceMode[UVIndex] = LayerElementUV[FbxUVIndex]->GetReferenceMode();
							UVMappingMode[UVIndex] = LayerElementUV[FbxUVIndex]->GetMappingMode();
							break;
						}
					}
				}
			}
		}
	}


    //
    // get the smoothing group layer
    //
    UBOOL bSmoothingAvailable = FALSE;

    KFbxLayerElementSmoothing const* SmoothingInfo = BaseLayer->GetSmoothing();
    KFbxLayerElement::EReferenceMode SmoothingReferenceMode(KFbxLayerElement::eDIRECT);
    KFbxLayerElement::EMappingMode SmoothingMappingMode(KFbxLayerElement::eBY_EDGE);
    if (SmoothingInfo)
    {
        if( SmoothingInfo->GetMappingMode() == KFbxLayerElement::eBY_EDGE )
        {
            if (!FbxGeometryConverter->ComputePolygonSmoothingFromEdgeSmoothing(FbxMesh))
            {
                warnf(NAME_Warning,TEXT("Unable to fully convert the smoothing groups for mesh %s"),ANSI_TO_TCHAR(FbxMesh->GetName()));
                bSmoothingAvailable = FALSE;
            }
        }

		if( SmoothingInfo->GetMappingMode() == KFbxLayerElement::eBY_POLYGON )
		{
			bSmoothingAvailable = TRUE;
		}


        SmoothingReferenceMode = SmoothingInfo->GetReferenceMode();
        SmoothingMappingMode = SmoothingInfo->GetMappingMode();
    }

	if(!StaticMesh->LODModels(LODIndex).Elements.Num())
	{
		const INT NewMaterialIndex = StaticMesh->LODModels(LODIndex).Elements.Num();
		new(StaticMesh->LODModels(LODIndex).Elements) FStaticMeshElement(NULL,NewMaterialIndex);
	}

	//
	// get the first vertex color layer
	//
	KFbxLayerElementVertexColor* LayerElementVertexColor = BaseLayer->GetVertexColors();
	KFbxLayerElement::EReferenceMode VertexColorReferenceMode(KFbxLayerElement::eDIRECT);
	KFbxLayerElement::EMappingMode VertexColorMappingMode(KFbxLayerElement::eBY_CONTROL_POINT);
	if (LayerElementVertexColor)
	{
		VertexColorReferenceMode = LayerElementVertexColor->GetReferenceMode();
		VertexColorMappingMode = LayerElementVertexColor->GetMappingMode();
	}

	//
	// get the first normal layer
	//
	KFbxLayerElementNormal* LayerElementNormal = BaseLayer->GetNormals();
    KFbxLayerElementTangent* LayerElementTangent = BaseLayer->GetTangents();
    KFbxLayerElementBinormal* LayerElementBinormal = BaseLayer->GetBinormals();

    //whether there is normal, tangent and binormal data in this mesh
    UBOOL bHasNTBInformation = LayerElementNormal && LayerElementTangent && LayerElementBinormal;

	KFbxLayerElement::EReferenceMode NormalReferenceMode(KFbxLayerElement::eDIRECT);
    KFbxLayerElement::EMappingMode NormalMappingMode(KFbxLayerElement::eBY_CONTROL_POINT);
    if (LayerElementNormal)
    {
        NormalReferenceMode = LayerElementNormal->GetReferenceMode();
        NormalMappingMode = LayerElementNormal->GetMappingMode();
    }

    KFbxLayerElement::EReferenceMode TangentReferenceMode(KFbxLayerElement::eDIRECT);
    KFbxLayerElement::EMappingMode TangentMappingMode(KFbxLayerElement::eBY_CONTROL_POINT);
    if (LayerElementTangent)
    {
        TangentReferenceMode = LayerElementTangent->GetReferenceMode();
        TangentMappingMode = LayerElementTangent->GetMappingMode();
    }

	//
	// build collision
	//
	if (ImportCollisionModels(StaticMesh, new KString(Node->GetName())))
	{
		INT LODModelIndex;
		for(LODModelIndex=0; LODModelIndex<StaticMesh->LODModels(LODIndex).Elements.Num(); LODModelIndex++)
		{
			StaticMesh->LODModels(LODIndex).Elements(LODModelIndex).EnableCollision = TRUE;
		}
	}

	//
	// build un-mesh triangles
	//

    // Construct the matrices for the conversion from right handed to left handed system
    KFbxXMatrix TotalMatrix;
    KFbxXMatrix TotalMatrixForNormal;
    TotalMatrix = ComputeTotalMatrix(Node);
    TotalMatrixForNormal = TotalMatrix.Inverse();
    TotalMatrixForNormal = TotalMatrixForNormal.Transpose();

    KFbxXMatrix LeftToRightMatrix;
    KFbxXMatrix LeftToRightMatrixForNormal;
    LeftToRightMatrix.SetS(KFbxVector4(1.0, -1.0, 1.0));
    LeftToRightMatrixForNormal = LeftToRightMatrix.Inverse();
    LeftToRightMatrixForNormal = LeftToRightMatrixForNormal.Transpose();

	INT ExistingTris = StaticMesh->LODModels(LODIndex).RawTriangles.GetElementCount();
	INT TriangleCount = FbxMesh->GetPolygonCount();
	StaticMesh->LODModels(LODIndex).RawTriangles.Lock(LOCK_READ_WRITE);
	FStaticMeshTriangle* RawTriangleData = (FStaticMeshTriangle*)StaticMesh->LODModels(LODIndex).RawTriangles.Realloc( ExistingTris + TriangleCount );

	RawTriangleData += ExistingTris;
	INT TriangleIndex;
	for( TriangleIndex = 0 ; TriangleIndex < TriangleCount ; TriangleIndex++ )
	{
		FStaticMeshTriangle*	Triangle = (RawTriangleData++);

        Triangle->bOverrideTangentBasis = bHasNTBInformation && (ImportOptions->bOverrideTangents);
		Triangle->bExplicitNormals = ImportOptions->bExplicitNormals;

        //
        // default vertex colors
        //
        Triangle->Colors[0] = FColor(255,255,255,255); // default value
        Triangle->Colors[1] = FColor(255,255,255,255); // default value
        Triangle->Colors[2] = FColor(255,255,255,255); // default value


		INT VertexIndex;
		for ( VertexIndex=0; VertexIndex<3; VertexIndex++)
		{
            int ControlPointIndex = FbxMesh->GetPolygonVertex(TriangleIndex, VertexIndex);
			KFbxVector4 FbxPosition = FbxMesh->GetControlPoints()[ControlPointIndex];
			KFbxVector4 FinalPosition = TotalMatrix.MultT(FbxPosition);
            Triangle->Vertices[VertexIndex] = Converter.ConvertPos(FinalPosition);

            //
            // normals, tangents and binormals
            //
            if( Triangle->bOverrideTangentBasis || Triangle->bExplicitNormals )
            {
                int TmpIndex = TriangleIndex*3 + VertexIndex;
                //normals may have different reference and mapping mode than tangents and binormals
                int NormalMapIndex = (NormalMappingMode == KFbxLayerElement::eBY_CONTROL_POINT) ? 
ControlPointIndex : TmpIndex;
                int NormalValueIndex = (NormalReferenceMode == KFbxLayerElement::eDIRECT) ? 
                    NormalMapIndex : LayerElementNormal->GetIndexArray().GetAt(NormalMapIndex);

				if( Triangle->bOverrideTangentBasis )
               	{
					//tangents and binormals share the same reference, mapping mode and index array
                	INT TangentMapIndex = TmpIndex;

               		KFbxVector4 TempValue = LayerElementTangent->GetDirectArray().GetAt(TangentMapIndex);
                	TempValue = TotalMatrixForNormal.MultT(TempValue);
               		TempValue = LeftToRightMatrixForNormal.MultT(TempValue);
                	Triangle->TangentX[ VertexIndex ] = Converter.ConvertDir(TempValue);

                	TempValue = LayerElementBinormal->GetDirectArray().GetAt(TangentMapIndex);
                	TempValue = TotalMatrixForNormal.MultT(TempValue);
                	TempValue = LeftToRightMatrixForNormal.MultT(TempValue);
                	Triangle->TangentY[ VertexIndex ] = Converter.ConvertDir(TempValue);


				}
				else
				{
					Triangle->TangentX[ VertexIndex ] = FVector( 0.0f, 0.0f, 0.0f );
                    Triangle->TangentY[ VertexIndex ] = FVector( 0.0f, 0.0f, 0.0f );
				}

				KFbxVector4 TempValue = LayerElementNormal->GetDirectArray().GetAt(NormalValueIndex);
               	TempValue = TotalMatrixForNormal.MultT(TempValue);
               	TempValue = LeftToRightMatrixForNormal.MultT(TempValue);
                Triangle->TangentZ[ VertexIndex ] = Converter.ConvertDir(TempValue);
            }
            else
            {
                INT NormalIndex;
                for( NormalIndex = 0; NormalIndex < 3; ++NormalIndex )
                {
                    Triangle->TangentX[ NormalIndex ] = FVector( 0.0f, 0.0f, 0.0f );
                    Triangle->TangentY[ NormalIndex ] = FVector( 0.0f, 0.0f, 0.0f );
                    Triangle->TangentZ[ NormalIndex ] = FVector( 0.0f, 0.0f, 0.0f );
                }
            }

            //
            // vertex colors
            //
            if (LayerElementVertexColor)
            {

                INT VertexColorMappingIndex = (VertexColorMappingMode == KFbxLayerElement::eBY_CONTROL_POINT) ? 
                    FbxMesh->GetPolygonVertex(TriangleIndex,VertexIndex) : (TriangleIndex*3+VertexIndex);

                INT VectorColorIndex = (VertexColorReferenceMode == KFbxLayerElement::eDIRECT) ? 
                    VertexColorMappingIndex : LayerElementVertexColor->GetIndexArray().GetAt(VertexColorMappingIndex);

                KFbxColor VertexColor = LayerElementVertexColor->GetDirectArray().GetAt(VectorColorIndex);

                Triangle->Colors[VertexIndex] = FColor(	BYTE(255.f*VertexColor.mRed),
                                                        BYTE(255.f*VertexColor.mGreen),
                                                        BYTE(255.f*VertexColor.mBlue),
                                                        BYTE(255.f*VertexColor.mAlpha));
            }


		}

		//
		// smoothing mask
		//
		if ( ! bSmoothingAvailable)
		{
			Triangle->SmoothingMask = 0;
		}
		else
		{
			Triangle->SmoothingMask = 0; // default
			if (SmoothingInfo)
			{
				if (SmoothingMappingMode == KFbxLayerElement::eBY_POLYGON)
				{
                    int lSmoothingIndex = (SmoothingReferenceMode == KFbxLayerElement::eDIRECT) ? TriangleIndex : SmoothingInfo->GetIndexArray().GetAt(TriangleIndex);
					Triangle->SmoothingMask = SmoothingInfo->GetDirectArray().GetAt(lSmoothingIndex);
				}
				else
				{
					warnf(NAME_Warning,TEXT("Unsupported Smoothing group mapping mode on mesh %s"),ANSI_TO_TCHAR(FbxMesh->GetName()));
				}
			}
		}

		//
		// uvs
		//
		// In FBX file, the same UV may be saved multiple times, i.e., there may be same UV in LayerElementUV
		// So we don't import the duplicate UVs
		Triangle->NumUVs = Min(UniqueUVCount, 8);
		INT UVLayerIndex;
		for (UVLayerIndex = 0; UVLayerIndex<UniqueUVCount; UVLayerIndex++)
		{
			if (LayerElementUV[UVLayerIndex] != NULL) 
			{
				INT VertexIndex;
				for (VertexIndex=0;VertexIndex<3;VertexIndex++)
				{
                    int lControlPointIndex = FbxMesh->GetPolygonVertex(TriangleIndex, VertexIndex);
                    int UVMapIndex = (UVMappingMode[UVLayerIndex] == KFbxLayerElement::eBY_CONTROL_POINT) ? 
lControlPointIndex : TriangleIndex*3+VertexIndex;
                    INT UVIndex = (UVReferenceMode[UVLayerIndex] == KFbxLayerElement::eDIRECT) ? 
                        UVMapIndex : LayerElementUV[UVLayerIndex]->GetIndexArray().GetAt(UVMapIndex);
					KFbxVector2	UVVector = LayerElementUV[UVLayerIndex]->GetDirectArray().GetAt(UVIndex);

					Triangle->UVs[VertexIndex][UVLayerIndex].X = static_cast<float>(UVVector[0]);
					Triangle->UVs[VertexIndex][UVLayerIndex].Y = 1.f-static_cast<float>(UVVector[1]);   //flip the Y of UVs for DirectX
				}
			}
		}

		if (Triangle->NumUVs == 0) // backup, need at least one channel
		{
			Triangle->UVs[0][0].X = 0;
			Triangle->UVs[0][0].Y = 0;

			Triangle->UVs[1][0].X = 0;
			Triangle->UVs[1][0].Y = 0;

			Triangle->UVs[2][0].X = 0;
			Triangle->UVs[2][0].Y = 0;

			Triangle->NumUVs = 1;
		}

		//
		// material index
		//
		Triangle->MaterialIndex = 0; // default value
		if (MaterialCount>0)
		{
			if (LayerElementMaterial)
			{
				switch(MaterialMappingMode)
				{
					// material index is stored in the IndexArray, not the DirectArray (which is irrelevant with 2009.1)
				case KFbxLayerElement::eALL_SAME:
					{	
						Triangle->MaterialIndex = LayerElementMaterial->GetIndexArray().GetAt(0);
					}
					break;
				case KFbxLayerElement::eBY_POLYGON:
					{	
						Triangle->MaterialIndex = LayerElementMaterial->GetIndexArray().GetAt(TriangleIndex);
					}
					break;
				}

				if (Triangle->MaterialIndex >= MaterialCount || Triangle->MaterialIndex < 0)
				{
					warnf(NAME_Warning,TEXT("Face material index inconsistency - forcing to 0"));
					Triangle->MaterialIndex = 0;
				}
			}
		}

		//
		// fragments - TODO
		//
		Triangle->FragmentIndex = 0;
	}

	//
	// complete build
	//
	StaticMesh->LODModels(LODIndex).RawTriangles.Unlock();
	
	//
	// clean up
	//
	if (UniqueUVCount > 0)
	{
		delete[] LayerElementUV;
		delete[] UVReferenceMode;
		delete[] UVMappingMode;
	}

	if (bDestroyMesh)
	{
		FbxMesh->Destroy(true);
	}

	return TRUE;
}

UObject* UnFbx::CFbxImporter::ReimportStaticMesh(UStaticMesh* Mesh)
{
	char MeshName[1024];
	appStrcpy(MeshName,1024,TCHAR_TO_ANSI(*Mesh->GetName()));
	TArray<KFbxNode*> FbxMeshArray;
	KFbxNode* FbxNode = NULL;
	UObject* NewMesh = NULL;
	
	// get meshes in Fbx file
	//the function also fill the collision models, so we can update collision models correctly
	FillFbxMeshArray(FbxScene->GetRootNode(), FbxMeshArray, this);
	
	// if there is only one mesh, use it without name checking 
	// (because the "Used As Full Name" option enables users name the Unreal mesh by themselves
	if (FbxMeshArray.Num() == 1)
	{
		FbxNode = FbxMeshArray(0);
	}
	else
	{
		// find the Fbx mesh node that the Unreal Mesh matches according to name
		INT MeshIndex;
		for ( MeshIndex = 0; MeshIndex < FbxMeshArray.Num(); MeshIndex++ )
		{
			const char* FbxMeshName = FbxMeshArray(MeshIndex)->GetName();
			// The name of Unreal mesh may have a prefix, so we match from end
			UINT i = 0;
			char* MeshPtr = MeshName + strlen(MeshName) - 1;
			if (strlen(FbxMeshName) <= strlen(MeshName))
			{
				const char* FbxMeshPtr = FbxMeshName + strlen(FbxMeshName) - 1;
				while (i < strlen(FbxMeshName))
				{
					if (*MeshPtr != *FbxMeshPtr)
					{
						break;
					}
					else
					{
						i++;
						MeshPtr--;
						FbxMeshPtr--;
					}
				}
			}

			if (i == strlen(FbxMeshName)) // matched
			{
				// check further
				if ( strlen(FbxMeshName) == strlen(MeshName) ||  // the name of Unreal mesh is full match
					*MeshPtr == '_')							 // or the name of Unreal mesh has a prefix
				{
					FbxNode = FbxMeshArray(MeshIndex);
					break;
				}
			}
		}
	}
	
	if (FbxNode)
	{
		KFbxNode* Parent = FbxNode->GetParent();
		// set import options, how about others?
		ImportOptions->bImportMaterials = FALSE;
		ImportOptions->bImportTextures = FALSE;
		
		// if the Fbx mesh is a part of LODGroup, update LOD
		if (Parent && Parent->GetNodeAttribute() && Parent->GetNodeAttribute()->GetAttributeType() == KFbxNodeAttribute::eLODGROUP)
		{
			NewMesh = ImportStaticMesh(Mesh->GetOuter(), Parent->GetChild(0), *Mesh->GetName(), RF_Public|RF_Standalone, Mesh, 0);
			if (NewMesh)
			{
				// import LOD meshes
				for (INT LODIndex = 1; LODIndex < Parent->GetChildCount(); LODIndex++)
				{
					ImportStaticMesh(Mesh->GetOuter(), Parent->GetChild(LODIndex), *Mesh->GetName(), RF_Public|RF_Standalone, Mesh, LODIndex);
				}
			}
		}
		else
		{
			NewMesh = ImportStaticMesh(Mesh->GetOuter(), FbxNode, *Mesh->GetName(), RF_Public|RF_Standalone, Mesh, 0);
		}
	}
	else
	{
		// no FBX mesh match, maybe the Unreal mesh is imported from multiple FBX mesh (enable option "Import As Single")
		if (FbxMeshArray.Num() > 0)
		{
			NewMesh = ImportStaticMeshAsSingle(Mesh->GetOuter(), FbxMeshArray, *Mesh->GetName(), RF_Public|RF_Standalone, Mesh, 0);
		}
		else // no mesh found in the FBX file
		{
			warnf(NAME_Log,TEXT("No FBX mesh found when reimport Unreal mesh %s. The FBX file is crashed."), *Mesh->GetName());
		}
	}

	return NewMesh;
}

UObject* UnFbx::CFbxImporter::ImportStaticMeshAsSingle(UObject* InParent, TArray<KFbxNode*>& MeshNodeArray, const FName& Name, EObjectFlags Flags, UStaticMesh* InStaticMesh, int LODIndex)
{
	UBOOL bBuildStatus = TRUE;
	struct ExistingStaticMeshData* ExistMeshDataPtr = NULL;

	// Make sure rendering is done - so we are not changing data being used by collision drawing.
	FlushRenderingCommands();

	if (MeshNodeArray.Num() == 0)
	{
		return NULL;
	}
	
	Parent = InParent;
	
	// warning for missing smoothing group info
	CheckSmoothingInfo(MeshNodeArray(0)->GetMesh());

	// create empty mesh
	UStaticMesh*	StaticMesh = NULL;

	UStaticMesh* ExistingMesh = NULL;
	// A mapping of vertex positions to their color in the existing static mesh
	TMap<FVector, FColor>		ExistingVertexColorData;

	if( InStaticMesh == NULL || LODIndex == 0 )
	{
		ExistingMesh = FindObject<UStaticMesh>( InParent, *Name.ToString() );
		
	}
	if (ExistingMesh)
	{
		// What LOD to get vertex colors from.  
		// Currently mesh painting only allows for painting on the first lod.
		const UINT PaintingMeshLODIndex = 0;

		const FStaticMeshRenderData& ExistingRenderData = ExistingMesh->LODModels( PaintingMeshLODIndex );

		// Nothing to copy if there are no colors stored.
		if( ExistingRenderData.ColorVertexBuffer.GetNumVertices() > 0 )
		{
			// Build a mapping of vertex positions to vertex colors.  Using a TMap will allow for fast lookups so we can match new static mesh vertices with existing colors 
			const FPositionVertexBuffer& ExistingPositionVertexBuffer = ExistingRenderData.PositionVertexBuffer;
			for( UINT PosIdx = 0; PosIdx < ExistingPositionVertexBuffer.GetNumVertices(); ++PosIdx )
			{
				const FVector& VertexPos = ExistingPositionVertexBuffer.VertexPosition( PosIdx );

				// Make sure this vertex doesnt already have an assigned color.  
				// If the static mesh had shadow volume verts, the position buffer holds duplicate vertices
				if( ExistingVertexColorData.Find( VertexPos ) == NULL )
				{
					ExistingVertexColorData.Set( VertexPos, ExistingRenderData.ColorVertexBuffer.VertexColor( PosIdx ) );
				}
			}
		}
		
		// Free any RHI resources for existing mesh before we re-create in place.
		ExistingMesh->PreEditChange(NULL);
		ExistMeshDataPtr = SaveExistingStaticMeshData(ExistingMesh);
	}
	
	if( InStaticMesh != NULL && LODIndex > 0 )
	{
		StaticMesh = InStaticMesh;
	}
	else
	{
		StaticMesh = new(InParent,Name,Flags|RF_Public) UStaticMesh;
	}


	if(StaticMesh->LODModels.Num() < LODIndex+1)
	{
		// Add one LOD 
		new(StaticMesh->LODModels) FStaticMeshRenderData();
		StaticMesh->LODInfo.AddItem(FStaticMeshLODInfo());
		
		if (StaticMesh->LODModels.Num() < LODIndex+1)
		{
			LODIndex = StaticMesh->LODModels.Num() - 1;
		}
	}
	
	StaticMesh->SourceFilePath = GFileManager->ConvertToRelativePath(*UFactory::CurrentFilename);

	FFileManager::FTimeStamp Timestamp;
	if (GFileManager->GetTimestamp( *UFactory::CurrentFilename, Timestamp ))
	{
		FFileManager::FTimeStamp::TimestampToFString(Timestamp, /*out*/ StaticMesh->SourceFileTimestamp);
	}
	
	// make sure it has a new lighting guid
	StaticMesh->LightingGuid = appCreateGuid();

	// Set it to use textured lightmaps. Note that Build Lighting will do the error-checking (texcoordindex exists for all LODs, etc).
	StaticMesh->LightMapResolution = 32;
	StaticMesh->LightMapCoordinateIndex = 1;

	INT MeshIndex;
	for (MeshIndex = 0; MeshIndex < MeshNodeArray.Num(); MeshIndex++ )
	{
		KFbxNode* Node = MeshNodeArray(MeshIndex);

		if (Node->GetMesh())
		{
			if (!BuildStaticMeshFromGeometry(Node->GetMesh(), StaticMesh, LODIndex))
			{
				bBuildStatus = FALSE;
				break;
			}
		}
	}

	if (bBuildStatus)
	{
		// Compress the materials array by removing any duplicates.
		for(INT k=0;k<StaticMesh->LODModels.Num();k++)
		{
			TArray<FStaticMeshElement> SaveElements = StaticMesh->LODModels(k).Elements;
			StaticMesh->LODModels(k).Elements.Empty();

			for( INT x = 0 ; x < SaveElements.Num() ; ++x )
			{
				// See if this material is already in the list.  If so, loop through all the raw triangles
				// and change the material index to point to the one already in the list.

				INT newidx = INDEX_NONE;
				for( INT y = 0 ; y < StaticMesh->LODModels(k).Elements.Num() ; ++y )
				{
					if( StaticMesh->LODModels(k).Elements(y).Name == SaveElements(x).Name )
					{
						newidx = y;
						break;
					}
				}

				if( newidx == INDEX_NONE )
				{
					StaticMesh->LODModels(k).Elements.AddItem( SaveElements(x) );
					newidx = StaticMesh->LODModels(k).Elements.Num()-1;
				}

				FStaticMeshTriangle* RawTriangleData = (FStaticMeshTriangle*) StaticMesh->LODModels(k).RawTriangles.Lock(LOCK_READ_WRITE);
				for( INT t = 0 ; t < StaticMesh->LODModels(k).RawTriangles.GetElementCount() ; ++t )
				{
					if( RawTriangleData[t].MaterialIndex == x )
					{
						RawTriangleData[t].MaterialIndex = newidx;
					}
				}
				StaticMesh->LODModels(k).RawTriangles.Unlock();
			}
		}

		if (ExistMeshDataPtr)
		{
			RestoreExistingMeshData(ExistMeshDataPtr, StaticMesh);
		}

		StaticMesh->bRemoveDegenerates = ImportOptions->bRemoveDegenerates;
		StaticMesh->Build(FALSE, FALSE);
		
		// Warn about bad light map UVs if we have any
		{
			TArray< FString > MissingUVSets;
			TArray< FString > BadUVSets;
			TArray< FString > ValidUVSets;
			UStaticMesh::CheckLightMapUVs( StaticMesh, MissingUVSets, BadUVSets, ValidUVSets );

			// NOTE: We don't care about missing UV sets here, just bad ones!
			if( BadUVSets.Num() > 0 )
			{
				appMsgf( AMT_OK, LocalizeSecure( LocalizeUnrealEd( "Error_NewStaticMeshHasBadLightMapUVSet_F" ), *Name.ToString() ) );
			}
		}

 		// Warn about any bad mesh elements
 		if (UStaticMesh::RemoveZeroTriangleElements(StaticMesh, FALSE) == TRUE)
		{
			// Build it again...
			StaticMesh->Build(FALSE, TRUE);
		}
	}
	else
	{
		StaticMesh = NULL;
	}

	return StaticMesh;
}


UBOOL UnFbx::CFbxImporter::FillCollisionModelList(KFbxNode* Node)
{
	KString* NodeName = new KString(Node->GetName());
	if ( NodeName->Find("UCX") == 0 || NodeName->Find("MCDCX") == 0 ||
		 NodeName->Find("UBX") == 0 || NodeName->Find("USP") == 0 )
	{
		// Get name of static mesh that the collision model connect to
		INT StartIndex = NodeName->Find('_') + 1;
		INT TmpEndIndex = NodeName->Find('_', StartIndex);
		INT EndIndex = TmpEndIndex;
		// Find the last '_' (underscore)
		while (TmpEndIndex >= 0)
		{
			EndIndex = TmpEndIndex;
			TmpEndIndex = NodeName->Find('_', EndIndex+1);
		}
		
		KString MeshName;
		if ( EndIndex >= 0 )
		{
			// all characters between the first '_' and the last '_' are the FBX mesh name
			// convert the name to upper because we are case insensitive
			MeshName = NodeName->Mid(StartIndex, EndIndex - StartIndex).Upper();
		}

		KMap<KString, KArrayTemplate<KFbxNode* >* >::RecordType const *Models = CollisionModels.Find(MeshName);
		KArrayTemplate<KFbxNode* >* Record;
		if ( !Models )
		{
			Record = new KArrayTemplate<KFbxNode*>();
			CollisionModels.Insert(MeshName, Record);
		}
		else
		{
			Record = Models->GetValue();
		}
		Record->Add(Node);
		
		return TRUE;
	}

	return FALSE;
}

extern void AddConvexGeomFromVertices( const TArray<FVector>& Verts, FKAggregateGeom* AggGeom, const TCHAR* ObjName );
extern void AddSphereGeomFromVerts( const TArray<FVector>& Verts, FKAggregateGeom* AggGeom, const TCHAR* ObjName );
extern void AddBoxGeomFromTris( const TArray<FPoly>& Tris, FKAggregateGeom* AggGeom, const TCHAR* ObjName );

UBOOL UnFbx::CFbxImporter::ImportCollisionModels(UStaticMesh* StaticMesh, KString* NodeName)
{
	// find collision models
	UBOOL bRemoveEmptyKey = FALSE;
	KString EmptyKey;

	// convert the name to upper because we are case insensitive
	KMap<KString, KArrayTemplate<KFbxNode* >* >::RecordType const *Record = CollisionModels.Find(NodeName->Upper());
	if ( !Record )
	{
		// compatible with old collision name format
		// if CollisionModels has only one entry and the key is ""
		if ( CollisionModels.GetSize() == 1 )
		{
			Record = CollisionModels.Find( EmptyKey );
		}
		if ( !Record ) 
		{
			return FALSE;
		}
		else
		{
			bRemoveEmptyKey = TRUE;
		}
	}

	KArrayTemplate<KFbxNode*>* Models = Record->GetValue();
	if( !StaticMesh->BodySetup )
	{
		StaticMesh->BodySetup = ConstructObject<URB_BodySetup>(URB_BodySetup::StaticClass(), StaticMesh);
	}    

	TArray<FVector>	CollisionVertices;
	TArray<INT>		CollisionFaceIdx;

	// construct collision model
	for (INT i=0; i<Models->GetCount(); i++)
	{
		KFbxNode* Node = Models->GetAt(i);
		KFbxMesh* FbxMesh = Node->GetMesh();

		FbxMesh->RemoveBadPolygons();
		UBOOL bDestroyMesh = FALSE;

		// Must do this before triangulating the mesh due to an FBX bug in TriangulateMeshAdvance
		INT LayerSmoothingCount = FbxMesh->GetLayerCount(KFbxLayerElement::eSMOOTHING);
		for(INT i = 0; i < LayerSmoothingCount; i++)
		{
			FbxGeometryConverter->ComputePolygonSmoothingFromEdgeSmoothing (FbxMesh, i);
		}

		if (!FbxMesh->IsTriangleMesh())
		{
			FString NodeName = ANSI_TO_TCHAR(MakeName(Node->GetName()));
			warnf(NAME_Log,TEXT("Triangulating mesh %s for collision model"), *NodeName);
			bool bSuccess;
			FbxMesh = FbxGeometryConverter->TriangulateMeshAdvance(FbxMesh, bSuccess); // not in place ! the old mesh is still there
			if (FbxMesh == NULL)
			{
				warnf(NAME_Error,TEXT("Unable to triangulate mesh"));
				return FALSE; // not clean, missing some dealloc
			}
			// this gets deleted at the end of the import
			bDestroyMesh = TRUE;
		}

		INT ControlPointsIndex;
		INT ControlPointsCount = FbxMesh->GetControlPointsCount();
		KFbxVector4* ControlPoints = FbxMesh->GetControlPoints();
		KFbxXMatrix Matrix = ComputeTotalMatrix(Node);

		for ( ControlPointsIndex = 0; ControlPointsIndex < ControlPointsCount; ControlPointsIndex++ )
		{
			new(CollisionVertices)FVector( Converter.ConvertPos(Matrix.MultT(ControlPoints[ControlPointsIndex])) );
			
			/*
			FVector vtx;
			vtx.X = ControlPoints[ControlPointsIndex][0];
			vtx.Y = ControlPoints[ControlPointsIndex][1];
			vtx.Z = ControlPoints[ControlPointsIndex][2];

			new(CollisionVertices)FVector(vtx);
			*/
		}

		INT TriangleCount = FbxMesh->GetPolygonCount();
		INT TriangleIndex;
		for ( TriangleIndex = 0 ; TriangleIndex < TriangleCount ; TriangleIndex++ )
		{
			new(CollisionFaceIdx)INT(FbxMesh->GetPolygonVertex(TriangleIndex,0));
			new(CollisionFaceIdx)INT(FbxMesh->GetPolygonVertex(TriangleIndex,1));
			new(CollisionFaceIdx)INT(FbxMesh->GetPolygonVertex(TriangleIndex,2));
		}

		TArray<FPoly> CollisionTriangles;

		// Make triangles
		for(INT x = 0;x < CollisionFaceIdx.Num();x += 3)
		{
			FPoly*	Poly = new( CollisionTriangles ) FPoly();

			Poly->Init();

			new(Poly->Vertices) FVector( CollisionVertices(CollisionFaceIdx(x + 2)) );
			new(Poly->Vertices) FVector( CollisionVertices(CollisionFaceIdx(x + 1)) );
			new(Poly->Vertices) FVector( CollisionVertices(CollisionFaceIdx(x + 0)) );
			Poly->iLink = x / 3;

			Poly->CalcNormal(1);
		}

		// Construct geometry object
		KString *ModelName = new KString(Node->GetName());
		if ( ModelName->Find("UCX") == 0 || ModelName->Find("MCDCX") == 0 )
		{
			AddConvexGeomFromVertices( CollisionVertices, &StaticMesh->BodySetup->AggGeom, (const TCHAR*)Node->GetName() );
		}
		else if ( ModelName->Find("UBX") == 0 )
		{
			AddBoxGeomFromTris( CollisionTriangles, &StaticMesh->BodySetup->AggGeom, (const TCHAR*)Node->GetName() );
		}
		else if ( ModelName->Find("USP") == 0 )
		{
			AddSphereGeomFromVerts( CollisionVertices, &StaticMesh->BodySetup->AggGeom, (const TCHAR*)Node->GetName() );
		}

		// Clear any cached rigid-body collision shapes for this body setup.
		StaticMesh->BodySetup->ClearShapeCache();

		if (bDestroyMesh)
		{
			FbxMesh->Destroy();
		}

		// Remove the empty key because we only use the model once for the first mesh
		if (bRemoveEmptyKey)
		{
			CollisionModels.Remove(EmptyKey);
		}

		CollisionVertices.Empty();
		CollisionFaceIdx.Empty();
	}
		
	return TRUE;
}

#endif // WITH_FBX
