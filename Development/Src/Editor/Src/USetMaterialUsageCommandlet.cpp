/*=============================================================================
	USetMaterialUsageCommandlet.cpp - Commandlet which finds what types of geometry a material is used on.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EditorPrivate.h"
#include "EngineMaterialClasses.h"
#include "EngineParticleClasses.h"
#include "EngineSequenceClasses.h"
#include "EngineAnimClasses.h"
#include "UnTerrain.h"
#include "EngineFoliageClasses.h"
#include "SpeedTree.h"

#include "..\..\UnrealEd\Inc\scc.h"
#include "..\..\UnrealEd\Inc\SourceControlIntegration.h"

struct FPackageMaterialInfo
{
	TArray<FString> SkeletalMeshMaterials;
	TArray<FString> ParticleSpriteMaterials;
	TArray<FString> BeamTrailMaterials;
	TArray<FString> ParticleSubUVMaterials;
	TArray<FString> StaticLightingMaterials;
};

static void SetMaterialUsage(
	TDynamicMap<FName,FPackageMaterialInfo>& PackageInfoMap,
	UMaterialInterface* MaterialInterface,
	UBOOL bUsedWithSkeletalMesh,
	UBOOL bUsedWithParticleSprites,
	UBOOL bUsedWithBeamTrails,
	UBOOL bUsedWithParticleSubUV,
	UBOOL bUsedWithStaticLighting
	)
{
	UMaterial* Material = MaterialInterface->GetMaterial();
	if(Material)
	{
		FPackageMaterialInfo* PackageInfo = PackageInfoMap.Find(Material->GetOutermost()->GetFName());
		if(!PackageInfo)
		{
			PackageInfo = &PackageInfoMap.Set(Material->GetOutermost()->GetFName(),FPackageMaterialInfo());
		}
		if(bUsedWithSkeletalMesh && !Material->bUsedWithSkeletalMesh)
		{
			PackageInfo->SkeletalMeshMaterials.AddUniqueItem(Material->GetPathName());
		}
		if(bUsedWithParticleSprites && !Material->bUsedWithParticleSprites)
		{
			PackageInfo->ParticleSpriteMaterials.AddUniqueItem(Material->GetPathName());
		}
		if(bUsedWithBeamTrails && !Material->bUsedWithBeamTrails)
		{
			PackageInfo->BeamTrailMaterials.AddUniqueItem(Material->GetPathName());
		}
		if(bUsedWithParticleSubUV && !Material->bUsedWithParticleSubUV)
		{
			PackageInfo->ParticleSubUVMaterials.AddUniqueItem(Material->GetPathName());
		}
		if(bUsedWithStaticLighting && !Material->bUsedWithStaticLighting)
		{
			PackageInfo->StaticLightingMaterials.AddUniqueItem(Material->GetPathName());
		}
	}
}

INT USetMaterialUsageCommandlet::Main( const FString& Params )
{
	const TCHAR* Parms = *Params;

	// Retrieve list of all packages in .ini paths.
	TArray<FString> PackageList;

	FString PackageWildcard;
	FString PackagePrefix;
	if(ParseToken(Parms,PackageWildcard,FALSE))
	{
		GFileManager->FindFiles(PackageList,*PackageWildcard,TRUE,FALSE);
		PackagePrefix = FFilename(PackageWildcard).GetPath() * TEXT("");
	}
	else
	{
		PackageList = GPackageFileCache->GetPackageFileList();
	}
	if( !PackageList.Num() )
		return 0;

	FSourceControlIntegration* SCC = new FSourceControlIntegration;

	// Iterate over all packages.
	TDynamicMap<FName,FPackageMaterialInfo> PackageInfoMap;
	for( INT PackageIndex = 0; PackageIndex < PackageList.Num(); PackageIndex++ )
	{
		FFilename Filename = PackagePrefix * PackageList(PackageIndex);

		warnf(NAME_Log, TEXT("Loading %s"), *Filename);

		UPackage* Package = UObject::LoadPackage( NULL, *Filename, 0 );
		if (Package != NULL)
		{
			// Iterate over all objects in the package.
			for(FObjectIterator ObjectIt;ObjectIt;++ObjectIt)
			{
				if(ObjectIt->IsIn(Package))
				{
					USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(*ObjectIt);
					USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(*ObjectIt);
					UParticleSpriteEmitter* SpriteEmitter = Cast<UParticleSpriteEmitter>(*ObjectIt);
					UModelComponent* ModelComponent = Cast<UModelComponent>(*ObjectIt);
					UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(*ObjectIt);
					UTerrainMaterial* TerrainMaterial = Cast<UTerrainMaterial>(*ObjectIt);
					UFoliageComponent* FoliageComponent = Cast<UFoliageComponent>(*ObjectIt);
					USpeedTreeComponent* SpeedTreeComponent = Cast<USpeedTreeComponent>(*ObjectIt);
					if(SkeletalMeshComponent)
					{
						// Mark all the materials referenced by the skeletal mesh component as being used with a skeletal mesh.
						for(INT MaterialIndex = 0;MaterialIndex < SkeletalMeshComponent->Materials.Num();MaterialIndex++)
						{
							UMaterialInterface* Material = SkeletalMeshComponent->Materials(MaterialIndex);
							if(Material)
							{
								SetMaterialUsage(PackageInfoMap,Material,TRUE,FALSE,FALSE,FALSE,FALSE);
							}
						}
					}
					else if(SkeletalMesh)
					{
						// Mark all the materials referenced by the skeletal mesh as being used with a skeletal mesh.
						for(INT MaterialIndex = 0;MaterialIndex < SkeletalMesh->Materials.Num();MaterialIndex++)
						{
							UMaterialInterface* Material = SkeletalMesh->Materials(MaterialIndex);
							if(Material)
							{
								SetMaterialUsage(PackageInfoMap,Material,TRUE,FALSE,FALSE,FALSE,FALSE);
							}
						}
					}
					else if(SpriteEmitter)
					{
						// Mark the sprite's material as being used with a particle system.
						if(SpriteEmitter->Material)
						{
							SetMaterialUsage(PackageInfoMap,SpriteEmitter->Material,FALSE,TRUE,TRUE,TRUE,FALSE);
						}
						for (INT LODIndex = 0; LODIndex < SpriteEmitter->LODLevels.Num(); LODIndex++)
						{
							UParticleLODLevel* LODLevel = SpriteEmitter->LODLevels(LODIndex);
							if (LODLevel)
							{
								if (LODLevel->RequiredModule)
								{
									if (LODLevel->RequiredModule->Material)
									{
										SetMaterialUsage(PackageInfoMap,LODLevel->RequiredModule->Material,FALSE,TRUE,TRUE,TRUE,FALSE);
									}
								}
							}
						}
					}
					else if(ModelComponent)
					{
						// Check each of the model's elements for static lighting, and if present mark the element's material as being used with static lighting.
						for(INT ElementIndex = 0;ElementIndex < ModelComponent->GetElements().Num();ElementIndex++)
						{
							const FModelElement& Element = ModelComponent->GetElements()(ElementIndex);
							if(Element.Material)
							{
								const UBOOL bHasStaticLighting = Element.LightMap != NULL || Element.ShadowMaps.Num();
								SetMaterialUsage(PackageInfoMap,Element.Material,FALSE,FALSE,FALSE,FALSE,TRUE);
							}
						}
					}
					else if(StaticMeshComponent && StaticMeshComponent->IsValidComponent())
					{
						// Determine if the component has static lighting.
						UBOOL bHasStaticLighting = FALSE;
						for(INT LODIndex = 0;LODIndex < StaticMeshComponent->LODData.Num();LODIndex++)
						{
							const FStaticMeshComponentLODInfo& ComponentLODInfo = StaticMeshComponent->LODData(LODIndex);
							if(ComponentLODInfo.LightMap != NULL || ComponentLODInfo.ShadowMaps.Num() || ComponentLODInfo.ShadowVertexBuffers.Num())
							{
								bHasStaticLighting = TRUE;
								break;
							}
						}

						// Check each of the static mesh component's elements for static lighting, and if present mark the element's material as being used with static lighting.
						if(bHasStaticLighting)
						{
							for(INT ElementIndex = 0;ElementIndex < StaticMeshComponent->GetNumElements();ElementIndex++)
							{
								UMaterialInterface* Material = StaticMeshComponent->GetMaterial(ElementIndex);
								if(Material)
								{
									SetMaterialUsage(PackageInfoMap,Material,FALSE,FALSE,FALSE,FALSE,TRUE);
								}
							}
						}
					}
					else if(TerrainMaterial)
					{
						for(INT MeshIndex = 0;MeshIndex < TerrainMaterial->FoliageMeshes.Num();MeshIndex++)
						{
							const FTerrainFoliageMesh& FoliageMesh = TerrainMaterial->FoliageMeshes(MeshIndex);

							// Mark the foliage mesh's material as being used with static lighting.
							UMaterialInterface* Material = NULL;
							if(FoliageMesh.Material)
							{
								Material = FoliageMesh.Material;
							}
							else if(FoliageMesh.StaticMesh->LODModels(0).Elements.Num() && FoliageMesh.StaticMesh->LODModels(0).Elements(0).Material)
							{
								Material = FoliageMesh.StaticMesh->LODModels(0).Elements(0).Material;
							}

							if(Material)
							{
								SetMaterialUsage(PackageInfoMap,Material,FALSE,FALSE,FALSE,FALSE,TRUE);
							}
						}
					}
					else if(FoliageComponent && FoliageComponent->IsValidComponent())
					{
						// Mark the foliage component's material as being used with static lighting.
						UMaterialInterface* Material = NULL;
						if(FoliageComponent->Material)
						{
							Material = FoliageComponent->Material;
						}
						else if(FoliageComponent->InstanceStaticMesh && FoliageComponent->InstanceStaticMesh->LODModels(0).Elements.Num() && FoliageComponent->InstanceStaticMesh->LODModels(0).Elements(0).Material)
						{
							Material = FoliageComponent->InstanceStaticMesh->LODModels(0).Elements(0).Material;
						}

						if(Material)
						{
							SetMaterialUsage(PackageInfoMap,Material,FALSE,FALSE,FALSE,FALSE,TRUE);
						}	
					}
					else if(SpeedTreeComponent && SpeedTreeComponent->IsValidComponent())
					{
						UMaterialInterface* BranchMaterial = SpeedTreeComponent->BranchMaterial;
						UMaterialInterface* FrondMaterial = SpeedTreeComponent->FrondMaterial;
						UMaterialInterface* LeafMaterial = SpeedTreeComponent->LeafMaterial;
						UMaterialInterface* BillboardMaterial = SpeedTreeComponent->BillboardMaterial;

						if(SpeedTreeComponent->SpeedTree)
						{
							if(!BranchMaterial)
							{
								BranchMaterial = SpeedTreeComponent->SpeedTree->BranchMaterial;
							}
							if(!FrondMaterial)
							{
								FrondMaterial = SpeedTreeComponent->SpeedTree->FrondMaterial;
							}
							if(!LeafMaterial)
							{
								LeafMaterial = SpeedTreeComponent->SpeedTree->LeafMaterial;
							}
							if(!BillboardMaterial)
							{
								BillboardMaterial = SpeedTreeComponent->SpeedTree->BillboardMaterial;
							}
						}
					}
					else if(!appStricmp(*ObjectIt->GetClass()->GetName(),TEXT("SeqAct_SetMaterial")))
					{
						// Extract the value of the script NewMaterial property.
						UProperty* MaterialProperty = CastChecked<UProperty>(ObjectIt->FindObjectField(FName(TEXT("NewMaterial"))));
						UMaterialInterface* Material = *(UMaterialInterface**)((BYTE*)*ObjectIt + MaterialProperty->Offset);
						if(Material)
						{
							USequenceAction* SequenceAction = CastChecked<USequenceAction>(*ObjectIt);
							// If the SetMaterial targets include a skeletal mesh, mark the material as being used with a skeletal mesh.
							for(INT TargetIndex = 0;TargetIndex < SequenceAction->Targets.Num();TargetIndex++)
							{
								ASkeletalMeshActor* SkeletalMeshActor = Cast<ASkeletalMeshActor>(SequenceAction->Targets(TargetIndex));
								if(SkeletalMeshActor)
								{
									SetMaterialUsage(PackageInfoMap,Material,TRUE,FALSE,FALSE,FALSE,FALSE);
									break;
								}
							}
						}
					}
				}
			}
		}

		UObject::CollectGarbage(RF_Native);
		SaveLocalShaderCaches();
	}

	UINT NumStaticLightingMaterials = 0;
	UINT NumTotalMaterials = 0;

	for(TDynamicMap<FName,FPackageMaterialInfo>::TConstIterator PackageIt(PackageInfoMap);PackageIt;++PackageIt)
	{
		const FPackageMaterialInfo& PackageInfo = PackageIt.Value();
		// Only save dirty packages.
		if(PackageInfo.SkeletalMeshMaterials.Num() 
			|| PackageInfo.ParticleSpriteMaterials.Num() 
			|| PackageInfo.BeamTrailMaterials.Num() 
			|| PackageInfo.ParticleSubUVMaterials.Num() 
			|| PackageInfo.StaticLightingMaterials.Num())
		{
			warnf(
				TEXT("Package %s is dirty(%u,%u,%u,%u,%u)"),
				*PackageIt.Key().ToString(),
				PackageInfo.SkeletalMeshMaterials.Num(),
				PackageInfo.ParticleSpriteMaterials.Num(),
				PackageInfo.BeamTrailMaterials.Num(),
				PackageInfo.ParticleSubUVMaterials.Num(),
				PackageInfo.StaticLightingMaterials.Num()
				);

			FString Filename;
			if(GPackageFileCache->FindPackageFile(*PackageIt.Key().ToString(),NULL,Filename))
			{
				UPackage* Package = UObject::LoadPackage( NULL, *Filename, 0 );
				if(Package)
				{
					UObject::ResetLoaders(Package);

					// Count the materials in the package.
					for(TObjectIterator<UMaterial> MaterialIt;MaterialIt;++MaterialIt)
					{
						NumTotalMaterials++;
					}

					for(INT MaterialIndex = 0;MaterialIndex < PackageInfo.SkeletalMeshMaterials.Num();MaterialIndex++)
					{
						UMaterial* Material = FindObjectChecked<UMaterial>(NULL,*PackageInfo.SkeletalMeshMaterials(MaterialIndex));
						Material->UseWithSkeletalMesh();
					}

					for(INT MaterialIndex = 0;MaterialIndex < PackageInfo.ParticleSpriteMaterials.Num();MaterialIndex++)
					{
						UMaterial* Material = FindObjectChecked<UMaterial>(NULL,*PackageInfo.ParticleSpriteMaterials(MaterialIndex));
						Material->UseWithParticleSprites();
					}

					for(INT MaterialIndex = 0;MaterialIndex < PackageInfo.BeamTrailMaterials.Num();MaterialIndex++)
					{
						UMaterial* Material = FindObjectChecked<UMaterial>(NULL,*PackageInfo.BeamTrailMaterials(MaterialIndex));
						Material->UseWithBeamTrails();
					}

					for(INT MaterialIndex = 0;MaterialIndex < PackageInfo.ParticleSubUVMaterials.Num();MaterialIndex++)
					{
						UMaterial* Material = FindObjectChecked<UMaterial>(NULL,*PackageInfo.ParticleSubUVMaterials(MaterialIndex));
						Material->UseWithParticleSubUV();
					}

					for(INT MaterialIndex = 0;MaterialIndex < PackageInfo.StaticLightingMaterials.Num();MaterialIndex++)
					{
						UMaterial* Material = FindObjectChecked<UMaterial>(NULL,*PackageInfo.StaticLightingMaterials(MaterialIndex));
						Material->UseWithStaticLighting();
						NumStaticLightingMaterials++;
					}

					if(Package->IsDirty())
					{
						if (GFileManager->IsReadOnly(*Filename))
						{
							SCC->CheckOut(Package);
						}
						if (!GFileManager->IsReadOnly(*Filename))
						{
							// resave the package
							UWorld* World = FindObject<UWorld>( Package, TEXT("TheWorld") );
							if( World )
							{	
								UObject::SavePackage( Package, World, 0, *Filename, GWarn );
							}
							else
							{
								UObject::SavePackage( Package, NULL, RF_Standalone, *Filename, GWarn );
							}
						}
					}
				}
			}

			UObject::CollectGarbage(RF_Native);
			SaveLocalShaderCaches();
		}
	}

	delete SCC; // clean up our allocated SCC

	warnf(TEXT("%u static lighting materials out of %u total materials."),NumStaticLightingMaterials,NumTotalMaterials); 

	return 0;
}

IMPLEMENT_CLASS(USetMaterialUsageCommandlet);

