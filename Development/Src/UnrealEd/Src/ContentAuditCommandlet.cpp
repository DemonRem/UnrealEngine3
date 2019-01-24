/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

#include "UnrealEd.h"
#include "EngineMaterialClasses.h"
#include "EngineSequenceClasses.h"
#include "UnPropertyTag.h"
#include "EngineUIPrivateClasses.h"
#include "EnginePhysicsClasses.h"
#include "EngineParticleClasses.h"
#include "LensFlare.h"
#include "EngineAnimClasses.h"
#include "UnTerrain.h"
#include "EngineFoliageClasses.h"
#include "SpeedTree.h"
#include "EnginePrefabClasses.h"
#include "Database.h"
#include "EngineSoundClasses.h"

#include "SourceControl.h"

#include "PackageHelperFunctions.h"
#include "PackageUtilityWorkers.h"

#include "PerfMem.h"
#include "AnimationEncodingFormat.h"
#include "AnimationUtils.h"

#include "UnFile.h"
#include "DiagnosticTable.h"

#if WITH_MANAGED_CODE
#include "GameAssetDatabaseShared.h"

FGADHelper CommandletGADHelper;

#endif // WITH_MANAGED_CODE



IMPLEMENT_COMPARE_CONSTREF( FString, ContentAuditCommandlet, { return appStricmp(*A,*B); } )


/**
 * This will look for any tags with Audit. in their name and then output them in a format
 * that the build system will be able to send out in email.
 **/
static void OutputAllAuditSummary()
{
#if WITH_MANAGED_CODE
	CommandletGADHelper.Initialize();

	// get all of the tags back
	TArray<FString> AllTags;
	FGameAssetDatabase::Get().QueryAllTags( AllTags );

	TArray<FString> AuditTags;

	for( INT i = 0; i < AllTags.Num(); ++i )
	{
		const FString& ATag = AllTags(i);		

		// then look for ones with Audit. in their name
		if( ATag.InStr( TEXT("Audit.") ) != INDEX_NONE )
		{
			AuditTags.AddItem( ATag );
		}
	}

	// sort them here
	Sort<USE_COMPARE_CONSTREF( FString, ContentAuditCommandlet )>( AuditTags.GetTypedData(), AuditTags.Num() );

	warnf( TEXT( "[REPORT] All Audit Tags and Their Counts" ) );

	// then query for membership
	for( INT i = 0; i < AuditTags.Num(); ++i )
	{
		const FString& ATag = AuditTags(i);		

		TArray<FString> AllAssetsTagged;
		CommandletGADHelper.QueryTaggedAssets( ATag, AllAssetsTagged );

		warnf( TEXT( "[REPORT] %s: %d" ), *ATag, AllAssetsTagged.Num() );
	}

#endif // WITH_MANAGED_CODE
}



/**
 * This will look for Textures which:
 *
 *   0) are probably specular (based off their name) and see if they have an LODBias of two
 *   1) have a negative LODBias
 *   2) have neverstream set
 *   3) a texture which looks to be a normalmap but doesn't have the correct texture compression set 
 *
 * All of the above are things which can be considered suspicious and probably should be changed
 * for best memory usage.  
 *
 * Specifically:  Specular with an LODBias of 2 was not noticeably different at the texture resolutions we used for gears; 512+
 *
 **/
struct TextureCheckerFunctor
{
	void CleanUpGADTags()
	{
		// some pain here as we need to clear all of the tags we are about to set and we have to just duplicate their names here.
		UpdateGADClearTags( TEXT("Audit.Texture_NormalmapNonDXT1CompressionNoAlphaIsTrue") );
		UpdateGADClearTags( TEXT("Audit.Texture_LODBiasIsNegative") );
		UpdateGADClearTags( TEXT("Audit.Texture_NeverStream") );
		UpdateGADClearTags( TEXT("Audit.Texture_G8") );
	}

	template< typename OBJECTYPE >
	void DoIt( UCommandlet* Commandlet, UPackage* Package, TArray<FString>& Tokens, TArray<FString>& Switches )
	{
		for( TObjectIterator<OBJECTYPE> It; It; ++It )
		{
			OBJECTYPE* Texture2D = *It;

			if( Texture2D->IsIn( Package ) == FALSE )
			{
				continue;
			}

			const FString&  TextureName = Texture2D->GetPathName();
			const INT		LODBias     = Texture2D->LODBias;

			FString OrigDescription = TEXT( "" );
			TArray<FString> TextureGroupNames = FTextureLODSettings::GetTextureGroupNames();
			if( Texture2D->LODGroup < TextureGroupNames.Num() )
			{
				OrigDescription = TextureGroupNames(Texture2D->LODGroup);
			}

			//warnf( TEXT( " checking %s" ), *TextureName );

			// if we are a TEXTUREGROUP_Cinematic then we don't care as that will be some crazy size / settings to look awesome :-)
			// TEXTUREGROUP_Cinematic:  implies being baked out
			if( Texture2D->LODGroup == TEXTUREGROUP_Cinematic )
			{
				continue;
			}

			// if this has been named as a specular texture
			if( ( ( TextureName.ToUpper().InStr( TEXT("SPEC" )) != INDEX_NONE )  // gears
				   || ( TextureName.ToUpper().InStr( TEXT("_S0" )) != INDEX_NONE )  // ut
				   || ( TextureName.ToUpper().InStr( TEXT("_S_" )) != INDEX_NONE )  // ut
				   || ( (TextureName.ToUpper().Right(2)).InStr( TEXT("_S" )) != INDEX_NONE )  // ut
				   )
				&& ( !( ( LODBias == 0 ) // groups are in charge of the spec LODBias
       				      //|| ( Texture2D->LODGroup == TEXTUREGROUP_WorldSpecular )
						  || ( Texture2D->LODGroup == TEXTUREGROUP_CharacterSpecular )  // we hand set these
						  || ( Texture2D->LODGroup == TEXTUREGROUP_WeaponSpecular ) // we hand set these
						 // || ( Texture2D->LODGroup == TEXTUREGROUP_VehicleSpecular )
					   )
				    )
				)		 
			{
				warnf( TEXT("%s:  Desired LODBias of 2 not correct.  ( Currently has %d )  OR not set to a SpecularTextureGroup (Currently has: %d (%s))"), *TextureName, LODBias, Texture2D->LODGroup, *OrigDescription );
			}

			if( (
				   ( TextureName.ToUpper().InStr( TEXT("_N0" )) != INDEX_NONE )  // ut
				|| ( TextureName.ToUpper().InStr( TEXT("_N_" )) != INDEX_NONE )  // ut
				|| ( (TextureName.ToUpper().Right(2)).InStr( TEXT("_N" )) != INDEX_NONE )  // ut
				|| ( Texture2D->LODGroup == TEXTUREGROUP_WorldNormalMap )
				|| ( Texture2D->LODGroup == TEXTUREGROUP_CharacterNormalMap )
				|| ( Texture2D->LODGroup == TEXTUREGROUP_WeaponNormalMap )
				|| ( Texture2D->LODGroup == TEXTUREGROUP_VehicleNormalMap )
				 ) )
			{
	
				// DXT5 have an alpha channel which usually is not used.
				// so we don't want to waste memory by having it be used / in memory
				if( ( Texture2D->CompressionSettings == TC_Normalmap)
					&& (( Texture2D->Format != PF_DXT1 )  // prob DXT5
					&& ( Texture2D->CompressionNoAlpha == FALSE ) )
					)
				{
					UpdateGADSetTags( Texture2D->GetFullName(), TEXT("Audit.Texture_NormalmapNonDXT1CompressionNoAlphaIsTrue"), TEXT("Audit.Texture_NormalmapNonDXT1CompressionNoAlphaIsTrue_Whitelist") );
					warnf( TEXT( "%s:  Normalmap CompressionNoAlpha should probably be TRUE or should be reimported with Format as DXT1.  Current Compression is: %d.  Current Format is: %d.  You need to look at the usage of this texture and see if there are any Material nodes are using the Alpha channel." ), *TextureName, Texture2D->CompressionSettings, Texture2D->Format );
				}

				// this should be automatically repaired by the SetTextureLODGroup Commandlet
// 				// checks for TC_NormalMapAlpha
// 				if( ( Texture2D->CompressionSettings == TC_NormalmapAlpha)
// 					&& (( Texture2D->Format == PF_DXT1 )
// 					|| ( Texture2D->CompressionNoAlpha != FALSE ) )
// 					)
// 				{
// 					warnf( TEXT( "%s: NormalMapAlpha CompressionNoAlpha should be FALSE and Format should not be DXT1" ), *TextureName );
// 				}
// 
// 				// SRGB should be false
// 				if( Texture2D->SRGB == FALSE )
// 				{
// 					warnf( TEXT( "%s: Normalmap should have SRGB = FALSE"), *TextureName);
// 				}
			}



			// if this has a negative LOD Bias that might be suspicious :-) (artists will often bump up the LODBias on their pet textures to make certain they are full full res)
			if( ( LODBias < 0 )
				)
			{
				UpdateGADSetTags( Texture2D->GetFullName(), TEXT("Audit.Texture_LODBiasIsNegative"), TEXT("Audit.Texture_LODBiasIsNegative_Whitelist") );
				warnf( TEXT("%s:  LODBias is negative ( Currently has %d )"), *TextureName, LODBias );
			}


			// check for neverstream
			if( (  Texture2D->LODGroup != TEXTUREGROUP_UI ) // UI textures are NeverStream
				&& ( Texture2D->NeverStream == TRUE )
				&& ( TextureName.InStr( TEXT("TerrainWeightMapTexture" )) == INDEX_NONE )  // TerrainWeightMapTextures are neverstream so we don't care about them
				
				)
			{
				UpdateGADSetTags( Texture2D->GetFullName(), TEXT("Audit.Texture_NeverStream"), TEXT("Audit.Texture_NeverStream_Whitelist") );
				warnf( TEXT("%s:  NeverStream is set to true" ), *TextureName );
			}

			// if this texture is a G8 it usually can be converted to a dxt without loss of quality.  Remember a 1024x1024 G* is always 1024!
			if( Texture2D->CompressionSettings == TC_Grayscale )
			{
				UpdateGADSetTags( Texture2D->GetFullName(), TEXT("Audit.Texture_G8"), TEXT("Audit.Texture_G8_Whitelist") );
				warnf( TEXT("%s:  G8 texture"), *TextureName );
			}

		}
	}
};






/**
* This will find materials which are missing Physical Materials
**/
struct MaterialMissingPhysMaterialFunctor
{
	void CleanUpGADTags()
	{
		// some pain here as we need to clear all of the tags we are about to set and we have to just duplicate their names here.
		UpdateGADClearTags( TEXT("Audit.Material_MissingPhysicalMaterial") );
	}

	template< typename OBJECTYPE >
	void DoIt( UCommandlet* Commandlet, UPackage* Package, TArray<FString>& Tokens, TArray<FString>& Switches )
	{
		for( TObjectIterator<OBJECTYPE> It; It; ++It )
		{
			OBJECTYPE* Material = *It;

			if( Material->IsIn( Package ) == FALSE )
			{
				continue;
			}

			UBOOL bHasPhysicalMaterial = FALSE;

			if( Material->PhysMaterial != NULL )
			{
				bHasPhysicalMaterial = TRUE;
			}

			if( bHasPhysicalMaterial == FALSE )
			{
				const FString& MaterialName = Material->GetPathName();
				UpdateGADSetTags( Material->GetFullName(), TEXT("Audit.Material_MissingPhysicalMaterial"), TEXT("Audit.Material_MissingPhysicalMaterial_Whitelist") );
				warnf( TEXT("%s:  Lacking PhysicalMaterial"), *MaterialName  );
			}
		}
	}
};



/** Will find all of the PhysicsAssets bodies which do not have a phys material assigned to them **/
struct SkeletalMeshesMissingPhysMaterialFunctor
{
	void CleanUpGADTags()
	{
		// some pain here as we need to clear all of the tags we are about to set and we have to just duplicate their names here.
		UpdateGADClearTags( TEXT("Audit.PhysAsset_MissingPhysicalMaterial") );
	}

	template< typename OBJECTYPE >
	void DoIt( UCommandlet* Commandlet, UPackage* Package, TArray<FString>& Tokens, TArray<FString>& Switches )
	{

		for( TObjectIterator<OBJECTYPE> It; It; ++It )
		{
			OBJECTYPE* PhysicsAsset = *It;

			if( PhysicsAsset->IsIn( Package ) == FALSE )
			{
				continue;
			}

			for( INT i = 0; i < PhysicsAsset->BodySetup.Num(); ++i )
			{
				const UPhysicalMaterial* PhysMat = PhysicsAsset->BodySetup(i)->PhysMaterial;
				if( PhysMat == NULL )
				{
					UpdateGADSetTags( PhysicsAsset->GetFullName(), TEXT("Audit.PhysAsset_MissingPhysicalMaterial"), TEXT("Audit.PhysAsset_MissingPhysicalMaterial_Whitelist") );
					warnf( TEXT( "Missing PhysMaterial on PhysAsset: %s Bone: %s" ), *PhysicsAsset->GetPathName(), *PhysicsAsset->BodySetup(i)->BoneName.ToString() );
				}
			}
		}
	}
};



/** 
 * Useful for finding PS with bad FixedRelativeBoundingBox.  
 * Bad is either not set OR set but with defaults for the BoundingBox (e.g. turned on but no one actually set the values. 
 */
struct FindParticleSystemsWithBadFixedRelativeBoundingBoxFunctor
{
	void CleanUpGADTags()
	{
		// some pain here as we need to clear all of the tags we are about to set and we have to just duplicate their names here.
		UpdateGADClearTags( TEXT("Audit.Particle_BadFixedRelativeBoundingBox") );
		UpdateGADClearTags( TEXT("Audit.Particle_MissingFixedRelativeBoundingBox") );
	}

	template< typename OBJECTYPE >
	void DoIt( UCommandlet* Commandlet, UPackage* Package, TArray<FString>& Tokens, TArray<FString>& Switches )
	{
		UBOOL bDirtyPackage = FALSE;

		const FName& PackageName = Package->GetFName(); 
		FString PackageFileName;
		GPackageFileCache->FindPackageFile( *PackageName.ToString(), NULL, PackageFileName );
		//warnf( NAME_Log, TEXT("  Loading2 %s"), *PackageFileName );

		for( TObjectIterator<OBJECTYPE> It; It; ++It )
		{
			OBJECTYPE* PS = *It;

			if( PS->IsIn( Package ) == FALSE )
			{
				continue;
			}

			if( 
				PS->bUseFixedRelativeBoundingBox == TRUE 
				&& PS->FixedRelativeBoundingBox.Min.X == -1
				&& PS->FixedRelativeBoundingBox.Min.Y == -1
				&& PS->FixedRelativeBoundingBox.Min.Z == -1
				&& PS->FixedRelativeBoundingBox.Max.X ==  1
				&& PS->FixedRelativeBoundingBox.Max.Y ==  1
				&& PS->FixedRelativeBoundingBox.Max.Z ==  1
			)
			{
				warnf( TEXT("PS is has bad FixedRelativeBoundingBox: %s"), *PS->GetFullName() );
				UpdateGADSetTags( PS->GetFullName(), TEXT("Audit.Particle_BadFixedRelativeBoundingBox"), TEXT("Audit.Particle_BadFixedRelativeBoundingBox_Whitelist") );
			}

			if( PS->bUseFixedRelativeBoundingBox == FALSE )
			{
				warnf( TEXT("PS is MissingFixedRelativeBoundingBox: %s"), *PS->GetFullName() );
				UpdateGADSetTags( PS->GetFullName(), TEXT("Audit.Particle_MissingFixedRelativeBoundingBox"), TEXT("Audit.Particle_MissingFixedRelativeBoundingBox_Whitelist") );
			}
		}
	}
};

/** 
 * Useful for testing just the DoActionToAllPackages without any side effects.
 **/
struct TestFunctor
{
	void CleanUpGADTags()
	{
	}

	template< typename OBJECTYPE >
	void DoIt( UCommandlet* Commandlet, UPackage* Package, TArray<FString>& Tokens, TArray<FString>& Switches )
	{
	}
};




/** 
 * This will look at all SoundCues and check for the set of "questionable" content cases that have been defined.
 **/
struct SoundCueAuditFunctor
{
	void CleanUpGADTags()
	{
		// some pain here as we need to clear all of the tags we are about to set and we have to just duplicate their names here.
		UpdateGADClearTags( TEXT("Audit.SoundCue_MoreThan3Waves") );
		UpdateGADClearTags( TEXT("Audit.SoundWave_QualityHigherThan40") );
		UpdateGADClearTags( TEXT("Audit.SoundCue_HasMixerWithMoreThan2Children") );
		UpdateGADClearTags( TEXT("Audit.SoundCue_NoSoundClass") );
		UpdateGADClearTags( TEXT("Audit.SoundCue_IncorrectAmbientSoundClass") );
		UpdateGADClearTags( TEXT("Audit.SoundCue_DiconnectedSoundNodes") );
	}

	template< typename OBJECTYPE >
	void DoIt( UCommandlet* Commandlet, UPackage* Package, TArray<FString>& Tokens, TArray<FString>& Switches )
	{
		const FName& PackageName = Package->GetFName(); 
		FString PackageFileName;
		GPackageFileCache->FindPackageFile( *PackageName.ToString(), NULL, PackageFileName );
		//warnf( NAME_Log, TEXT("  Loading2 %s"), *PackageFileName );

		for( TObjectIterator<OBJECTYPE> It; It; ++It )
		{
			OBJECTYPE* SoundCue = *It;

			if( SoundCue->IsIn( Package ) == FALSE )
			{
				continue;
			}

			const FString& CueName = SoundCue->GetFullName();

			// so now that we have a SoundCue we need to look inside it!


			// check to see that the number of SoundNodes that are in the SoundCue are actually the same number that
			// are actually referenced
			TArray<class USoundNode*> OutNodes;
			SoundCue->RecursiveFindAllNodes( SoundCue->FirstNode, OutNodes );
			OutNodes.Shrink();

			INT NumActualNodes = 0;

			for( TMap<USoundNode*,FSoundNodeEditorData>::TIterator It(SoundCue->EditorData); It; ++It )
			{
				if( It.Key() != NULL )
				{
					NumActualNodes++;
				}
			}

			if( NumActualNodes != OutNodes.Num() )
			{
				UpdateGADSetTags( CueName, TEXT("Audit.SoundCue_DiconnectedSoundNodes"), TEXT("Audit.SoundCue_DiconnectedSoundNodes_Whitelist") );

				warnf( TEXT( "%s has %d SoundNodeWaves referenced but only %d used" ), *CueName, NumActualNodes, OutNodes.Num() );
			}


			//// Check to see how many SoundNodeWaves this guy has
			TArray<USoundNodeWave*> Waves;
			SoundCue->RecursiveFindNode<USoundNodeWave>( SoundCue->FirstNode, Waves );

			if( Waves.Num() > 3 )
			{
				UpdateGADSetTags( CueName, TEXT("Audit.SoundCue_MoreThan3Waves"), TEXT("Audit.SoundCue_MoreThan3Waves_Whitelist") );

				warnf( TEXT( "%s has %d SoundNodeWaves referenced" ), *CueName, Waves.Num() );
			}


			//// Check for Compression Quality for soundnodewaves (default is 40; flag if higher than 40)

			for( INT i = 0; i < Waves.Num(); ++i )
			{
				USoundNodeWave* Wave = Waves(i);

				if( Wave->CompressionQuality > 40 )
				{
					UpdateGADSetTags( Wave->GetFullName(), TEXT("Audit.SoundWave_QualityHigherThan40"), TEXT("Audit.SoundWave_QualityHigherThan40_Whitelist") );
					warnf( TEXT( "%s has Compression Quality greater than 40.  Curr: %d" ), *Wave->GetFullName(), Wave->CompressionQuality );
				}
			}


			//// Check to see if any mixers have more than 2 outputs
			TArray<USoundNodeMixer*> Mixers;
			SoundCue->RecursiveFindNode<USoundNodeMixer>( SoundCue->FirstNode, Mixers );

			for( INT i = 0; i < Mixers.Num(); ++i )
			{
				USoundNodeMixer* Mixer = Mixers(i);
				if( Mixer->ChildNodes.Num() > 2 )
				{
					UpdateGADSetTags( SoundCue->GetFullName(), TEXT("Audit.SoundCue_HasMixerWithMoreThan2Children"), TEXT("Audit.SoundCue_HasMixerWithMoreThan2Children_Whitelist") );
					warnf( TEXT( "%s has Mixer with more than 2 children.  Curr: %d" ), *SoundCue->GetFullName(), Mixer->ChildNodes.Num() );
				}
			}




			//// Check to see if a soundclass is set
			if( SoundCue->SoundClass == FName( TEXT( "None" ) ) )
			{
				UpdateGADSetTags( SoundCue->GetFullName(), TEXT("Audit.SoundCue_NoSoundClass"), TEXT("Audit.SoundCue_NoSoundClass_Whitelist") );
				warnf( TEXT( "%s has no SoundClass" ), *CueName  );
			}
			else if( SoundCue->SoundClass == FName( TEXT( "Ambient" ) ) )
			{
				// if not in a correct Ambient package
				if( ( PackageName.ToString().InStr( TEXT( "Ambient_Loop" ) ) == INDEX_NONE ) && ( PackageName.ToString().InStr( TEXT( "Ambient_NonLoop" ) ) == INDEX_NONE ) )
				{
					UpdateGADSetTags( SoundCue->GetFullName(), TEXT("Audit.SoundCue_IncorrectAmbientSoundClass"), TEXT("Audit.SoundCue_IncorrectAmbientSoundClass_Whitelist") );
					warnf( TEXT( "%s is classified as Ambient SoundClass but is not in the correct Package:  %s" ), *CueName, *PackageName.ToString()  );
				}
			}

		}
	}
};





/**
* This will find materials which are missing Physical Materials
**/
struct CheckLightMapUVsFunctor
{
	void CleanUpGADTags()
	{
		// some pain here as we need to clear all of the tags we are about to set and we have to just duplicate their names here.
		UpdateGADClearTags( TEXT("Audit.StaticMesh_WithMissingUVSets") );
		UpdateGADClearTags( TEXT("Audit.StaticMesh_WithBadUVSets") );
	}

	template< typename OBJECTYPE >
	void DoIt( UCommandlet* Commandlet, UPackage* Package, TArray<FString>& Tokens, TArray<FString>& Switches )
	{
		for( TObjectIterator<OBJECTYPE> It; It; ++It )
		{
			OBJECTYPE* StaticMesh = *It;

			if( StaticMesh->IsIn( Package ) == FALSE )
			{
				continue;
			}

			TArray<FString> InOutAssetsWithMissingUVSets;
			TArray<FString> InOutAssetsWithBadUVSets;
			TArray<FString> InOutAssetsWithValidUVSets;

			UStaticMesh::CheckLightMapUVs( StaticMesh, InOutAssetsWithMissingUVSets, InOutAssetsWithBadUVSets, InOutAssetsWithValidUVSets );


			for( INT i = 0; i < InOutAssetsWithMissingUVSets.Num(); ++i )
			{
				UpdateGADSetTags( InOutAssetsWithMissingUVSets(i), TEXT("Audit.StaticMesh_WithMissingUVSets"), TEXT("Audit.StaticMesh_WithMissingUVSets_Whitelist") );
				warnf( TEXT("StaticMesh_WithMissingUVSets: %s"), *InOutAssetsWithMissingUVSets(i) );
			}


			for( INT i = 0; i < InOutAssetsWithBadUVSets.Num(); ++i )
			{
				UpdateGADSetTags( InOutAssetsWithBadUVSets(i), TEXT("Audit.StaticMesh_WithBadUVSets"), TEXT("Audit.StaticMesh_WithBadUVSets_Whitelist") );
				warnf( TEXT("StaticMeshWithBadUVSets: %s"), *InOutAssetsWithBadUVSets(i) );
			}

		}	
	}
};




/** 
*
 */
struct FindStaticMeshEmptySectionsFunctor
{
	void CleanUpGADTags()
	{
		// some pain here as we need to clear all of the tags we are about to set and we have to just duplicate their names here.
		UpdateGADClearTags( TEXT("Audit.StaticMesh_EmptySections") );
	}

	template< typename OBJECTYPE >
	void DoIt( UCommandlet* Commandlet, UPackage* Package, TArray<FString>& Tokens, TArray<FString>& Switches )
	{
		UBOOL bDirtyPackage = FALSE;

		const FName& PackageName = Package->GetFName(); 
		FString PackageFileName;
		GPackageFileCache->FindPackageFile( *PackageName.ToString(), NULL, PackageFileName );
		//warnf( NAME_Log, TEXT("  Loading2 %s"), *PackageFileName );


		for( TObjectIterator<OBJECTYPE> It; It; ++It )
		{
			OBJECTYPE* StaticMesh = *It;

			if( StaticMesh->IsIn( Package ) == FALSE )
			{
				continue;
			}

			UBOOL bAdded = FALSE;
			for (INT LODIdx = 0; LODIdx < StaticMesh->LODModels.Num() && !bAdded; LODIdx++)
			{
				const FStaticMeshRenderData& LODModel = StaticMesh->LODModels(LODIdx);
				for (INT ElementIdx = 0; ElementIdx < LODModel.Elements.Num() && !bAdded; ElementIdx++)
				{
					const FStaticMeshElement& Element = LODModel.Elements(ElementIdx);
					if ((Element.NumTriangles == 0) || (Element.Material == NULL))
					{
						warnf( TEXT("StaticMesh has Empty Section: %s"), *StaticMesh->GetFullName() );
						UpdateGADSetTags( StaticMesh->GetFullName(), TEXT("Audit.StaticMesh_EmptySections"), TEXT("Audit.StaticMesh_EmptySections_Whitelist") );

						bAdded = TRUE;
					}
				}
			}
		}
	}
};







/*-----------------------------------------------------------------------------
SoundCueAudit Commandlet
-----------------------------------------------------------------------------*/
INT USoundCueAuditCommandlet::Main( const FString& Params )
{
	DoActionToAllPackages<USoundCue, SoundCueAuditFunctor>(this, Params);

	return 0;
}
IMPLEMENT_CLASS(USoundCueAuditCommandlet)



/*-----------------------------------------------------------------------------
FindMissingPhysicalMaterials commandlet.
-----------------------------------------------------------------------------*/

INT UFindTexturesWithMissingPhysicalMaterialsCommandlet::Main( const FString& Params )
{
	DoActionToAllPackages<UMaterial, MaterialMissingPhysMaterialFunctor>(this, Params);
	DoActionToAllPackages<UPhysicsAsset, SkeletalMeshesMissingPhysMaterialFunctor>(this, Params);

	return 0;
}
IMPLEMENT_CLASS(UFindTexturesWithMissingPhysicalMaterialsCommandlet)


/*-----------------------------------------------------------------------------
FindQuestionableTextures Commandlet
-----------------------------------------------------------------------------*/

INT UFindQuestionableTexturesCommandlet::Main( const FString& Params )
{
	DoActionToAllPackages<UTexture2D, TextureCheckerFunctor>(this, Params);

	return 0;
}
IMPLEMENT_CLASS(UFindQuestionableTexturesCommandlet)




/*-----------------------------------------------------------------------------
OutputAuditSummary Commandlet
-----------------------------------------------------------------------------*/
INT UOutputAuditSummaryCommandlet::Main( const FString& Params )
{
	OutputAllAuditSummary();
	return 0;
}

IMPLEMENT_CLASS(UOutputAuditSummaryCommandlet)



/*-----------------------------------------------------------------------------
ContentAudit Commandlet
-----------------------------------------------------------------------------*/
INT UContentAuditCommandlet::Main( const FString& Params )
{
	//DoActionToAllPackages<USoundCue, TestFunctor>(this, Params);
	
	const FString NewParams = Params + TEXT( " -SKIPSCRIPTPACKAGES" );

	//// TODO: change this to try and take in a list of TYPES and then the functor

	DoActionToAllPackages<USoundCue, SoundCueAuditFunctor>(this, NewParams);

	DoActionToAllPackages<UTexture2D, TextureCheckerFunctor>(this, NewParams);

	DoActionToAllPackages<UMaterial, MaterialMissingPhysMaterialFunctor>(this, NewParams);
 	
 	DoActionToAllPackages<UPhysicsAsset, SkeletalMeshesMissingPhysMaterialFunctor>(this, NewParams);
 
 	DoActionToAllPackages<UParticleSystem, FindParticleSystemsWithBadFixedRelativeBoundingBoxFunctor>(this, NewParams);
 
 	DoActionToAllPackages<UStaticMesh, CheckLightMapUVsFunctor>(this, NewParams);
 	
 	DoActionToAllPackages<UStaticMesh, FindStaticMeshEmptySectionsFunctor>(this, NewParams);


	// so at the end here we need to ask the GAD for all of the Audit categories and print them out
	OutputAllAuditSummary();


	UCommandlet* Commandlet = ConstructObject<UCommandlet>( UFindUniqueSpecularTextureMaterialsCommandlet::StaticClass() );
	check(Commandlet);

	Commandlet->InitExecution();
	Commandlet->ParseParms( appCmdLine() );
	INT ErrorLevel = Commandlet->Main( appCmdLine() );


	return 0;
}
IMPLEMENT_CLASS(UContentAuditCommandlet)



/**
 * We use the CreateCustomEngine call to set some flags which will allow SerializeTaggedProperties to have the correct settings
 * such that editoronly and notforconsole data can correctly NOT be loaded from startup packages (e.g. engine.u)
 *
 **/
void UContentComparisonCommandlet::CreateCustomEngine()
{
	// get the platform from the Params (MUST happen first thing)
// 	if ( !SetPlatform( appCmdLine() ) )
// 	{
// 		SET_WARN_COLOR(COLOR_RED);
// 		warnf(NAME_Error, TEXT("Platform not specified. You must use:\n   platform=[xenon|pc|pcserver|pcconsole|ps3|linux|macosx|iphone|android]"));
// 		CLEAR_WARN_COLOR();
// 	}
    
	GIsCooking				= TRUE;
	GCookingTarget 			= UE3::PLATFORM_Stripped;

	// force cooking to use highest compat level
	//appSetCompatibilityLevel(FCompatibilityLevelInfo(5,5,5), FALSE);
}





/*-----------------------------------------------------------------------------
ContentAudit Commandlet
-----------------------------------------------------------------------------*/
INT UContentComparisonCommandlet::Main( const FString& Params )
{
	// TODO:  we need to make a baseline for everything really.  Otherwise all of the 
	//        always loaded meshes and such "pollute" the costs  (e.g. we have a ton of gore staticmeshes always loaded
	//        that get dumped into every pawn's costs.
	//  need to turn off guds loadig also prob

	TArray<FString> Tokens;
	TArray<FString> Switches;
	ParseCommandLine(*Params, Tokens, Switches);

	bGenerateAssetGrid = (Switches.FindItemIndex(TEXT("ASSETGRID")) != INDEX_NONE);
	bGenerateFullAssetGrid = (Switches.FindItemIndex(TEXT("FULLASSETGRID")) != INDEX_NONE);
	bSkipTagging = (Switches.FindItemIndex(TEXT("CREATETAGS")) == INDEX_NONE);
	bSkipAssetCSV = (Switches.FindItemIndex(TEXT("SKIPASSETCSV")) != INDEX_NONE);

	if (bSkipTagging == FALSE)
	{
		GADHelper = new FGADHelper();
		check(GADHelper);
		GADHelper->Initialize();
	}

	// so here we need to look at the ClassesToCompare
	// 0) load correct package
	// 1) search for the class
	// 2) analyze the class and save off the data we care about


	CurrentTime = appSystemTimeString();

	// load all of the non native script packages that exist
	const UEditorEngine* EditorEngine = CastChecked<UEditorEngine>(GEngine);
	for( INT i=0; i < EditorEngine->EditPackages.Num(); i++ )
	{
		UObject::BeginLoad();
		warnf( TEXT("Loading EditPackage: %s"), *EditorEngine->EditPackages(i) );
		UPackage* Package = LoadPackage( NULL, *EditorEngine->EditPackages(i), LOAD_NoWarn );
		//ULinkerLoad* Linker = UObject::GetPackageLinker( NULL, *EditorEngine->EditPackages(i), LOAD_NoVerify, NULL, NULL );
		UObject::EndLoad();
	}

	for (TObjectIterator<UObject> It; It; ++It)
	{
		It->SetFlags(RF_Marked);
	}


	FConfigSection* CompareClasses = GConfig->GetSectionPrivate( TEXT("ContentComparisonClassesToCompare"), FALSE, TRUE, GEditorIni );
	if( CompareClasses != NULL )
	{
		for( FConfigSectionMap::TIterator It(*CompareClasses); It; ++It )
		{
			const FString& ClassToCompare = It.Value();

			ClassesToCompare.AddUniqueItem( ClassToCompare );
		}
	}

	// Make a list of classes to compare that have derived classes in the list as well.
	// In these cases, we want to skip objects that are of the derived type when filling
	// in the information on the base.
	TArray<UClass*> Classes;
	for (INT ClassIdx = 0; ClassIdx < ClassesToCompare.Num(); ClassIdx++)
	{
		const FString& ClassToCompare = ClassesToCompare(ClassIdx);
		UClass* TheClass = (UClass*)UObject::StaticLoadObject( UClass::StaticClass(), ANY_PACKAGE, *ClassToCompare, NULL, LOAD_NoWarn|LOAD_Quiet, NULL );
		if (TheClass != NULL)
		{
			Classes.AddUniqueItem(TheClass);
		}
	}
	for (INT OuterClassIdx = 0; OuterClassIdx < Classes.Num(); OuterClassIdx++)
	{
		UClass* OuterClass = Classes(OuterClassIdx);
		for (INT InnerClassIdx = 0; InnerClassIdx < Classes.Num(); InnerClassIdx++)
		{
			if (InnerClassIdx != OuterClassIdx)
			{
				UClass* InnerClass = Classes(InnerClassIdx);
				if (InnerClass->IsChildOf(OuterClass) == TRUE)
				{
					TArray<FString>* DerivedList = ClassToDerivedClassesBeingCompared.Find(OuterClass->GetName());
					if (DerivedList == NULL)
					{
						TArray<FString> Temp;
						ClassToDerivedClassesBeingCompared.Set(OuterClass->GetName(), Temp);
						DerivedList = ClassToDerivedClassesBeingCompared.Find(OuterClass->GetName());
					}
					check(DerivedList);
					DerivedList->AddUniqueItem(InnerClass->GetName());
				}
			}
		}
	}
	Classes.Empty();
	UObject::CollectGarbage(RF_Marked);

	for( INT i = 0; i < ClassesToCompare.Num(); ++i )
	{
		const FString& ClassToCompare = ClassesToCompare(i);

		UClass* TheClass = (UClass*)UObject::StaticLoadObject( UClass::StaticClass(), ANY_PACKAGE, *ClassToCompare, NULL, LOAD_NoWarn|LOAD_Quiet, NULL );

		warnf( TEXT("Loaded Class: %s"), *TheClass->GetFullName() );

		if (TheClass != NULL)
		{
			for (TObjectIterator<UFaceFXAnimSet> FFXIt; FFXIt; ++FFXIt)
			{
				UFaceFXAnimSet* FaceFXAnimSet = *FFXIt;
				FaceFXAnimSet->ReferencedSoundCues.Empty();
			}

			TArray<UClass*> DerivedClasses;
			TArray<FString>* DerivedClassNames = ClassToDerivedClassesBeingCompared.Find(TheClass->GetName());
			if (DerivedClassNames)
			{
				for (INT DerivedIdx = 0; DerivedIdx < DerivedClassNames->Num(); DerivedIdx++)
				{
					const FString& DerivedClassName = (*DerivedClassNames)(DerivedIdx);
					UClass* DerivedClass = (UClass*)UObject::StaticFindObject(UClass::StaticClass(), ANY_PACKAGE, *DerivedClassName, TRUE);
					DerivedClasses.AddItem(DerivedClass);
				}
			}

			InitClassesToGatherDataOn( TheClass->GetFName().ToString() );

			for( TObjectIterator<UClass> It; It; ++It )
			{
				const UClass* const TheAssetClass = *It;

				if( ( TheAssetClass->IsChildOf( TheClass ) == TRUE )
					&& ( TheAssetClass->HasAnyClassFlags(CLASS_Abstract) == FALSE )
					)
				{
					UBOOL bSkipIt = FALSE;
					for (INT CheckIdx = 0; CheckIdx < DerivedClasses.Num(); CheckIdx++)
					{
						UClass* CheckClass = DerivedClasses(CheckIdx);
						if (TheAssetClass->IsChildOf(CheckClass) == TRUE)
						{
							warnf(TEXT("Skipping class derived from other content comparison class..."));
							bSkipIt = TRUE;
						}
					}
					if (bSkipIt == FALSE)
					{
						warnf( TEXT("TheAssetClass: %s"), *TheAssetClass->GetFullName() );

						GatherClassBasedData( TheAssetClass->GetLinker(), TheAssetClass );

						ReportComparisonData( TheAssetClass->GetFullName() );

						UObject::CollectGarbage(RF_Marked);
					}
				}
			}

			// close the current sheet
			if( Table != NULL )
			{
				Table->Close();
				delete Table;
				Table = NULL;
			}

			ReportClassAssetDependencies(TheClass->GetFName().ToString());
		}
	}


	if (bSkipTagging == FALSE)
	{
		GADHelper->Shutdown();
		delete GADHelper;
	}

	return 0;


// old way

// 	for( INT i = 0; i < ClassesToCompare.Num(); ++i )
// 	{
// 		const FString& ClassToCompare = ClassesToCompare(i);
// 
// 		const INT DotIndex = ClassToCompare.InStr( TEXT(".") );
// 
// 		FString PackageName;
// 		FString ClassName;
// 
// 		ClassToCompare.Split( TEXT("."), &PackageName, &ClassName, TRUE );
// 
// 		warnf( TEXT("About to compare: PackageName %s with Class: %s  DotIndex: %d"), *PackageName, *ClassName, DotIndex );
// 
// 		UObject::BeginLoad();
// 		ULinkerLoad* Linker = UObject::GetPackageLinker( NULL, *PackageName, LOAD_NoVerify, NULL, NULL );
// 		UObject::EndLoad();
// 
// 		if( Linker == NULL )
// 		{
// 			warnf( TEXT( "NULL LINKER" ) );
// 		}
// 
// 		GatherClassBasedData( Linker, ClassName );
// 
// 		ReportComparisonData( ClassToCompare );
// 
// 		UObject::CollectGarbage(RF_Native);
// 	}

	if( Table != NULL )
	{
		Table->Close();
	}

	return 0;
}



void UContentComparisonCommandlet::InitClassesToGatherDataOn( const FString& InTableName )
{
	Table = NULL;
	TypesToLookFor.Empty();


	FConfigSection* StatTypes = GConfig->GetSectionPrivate( TEXT("ContentComparisonReferenceTypes"), FALSE, TRUE, GEngineIni );
	if( StatTypes != NULL )
	{
		// make our cvs file
		Table = new FDiagnosticTableViewer( *FDiagnosticTableViewer::GetUniqueTemporaryFilePath( *  ( FString(TEXT("ContentComparison\\")) + FString::Printf(TEXT("ContentComparison-%d-%s\\"), GetChangeListNumberForPerfTesting(), *CurrentTime ) + FString::Printf( TEXT("ContentComparison-%s"), *InTableName ) ) ) );

		Table->AddColumn( TEXT("AssetName") );

		for( FConfigSectionMap::TIterator It(*StatTypes); It; ++It )
		{
			const FString StatType = It.Value();
			UClass* TheClass = (UClass*)UObject::StaticFindObject( UClass::StaticClass(), ANY_PACKAGE, *StatType );
			//UClass* TheClass = (UClass*)UObject::StaticLoadObject( UClass::StaticClass(), NULL, *StatType, NULL, LOAD_None, NULL );

			//warnf( TEXT("StatType: '%s'  TheClass: %s"), *StatType, *TheClass->GetFullName() );

			if( TheClass != NULL )
			{
				TypesToLookForDatum ClassToCompare( StatType, TheClass );
				TypesToLookFor.AddItem( ClassToCompare );

				Table->AddColumn( *StatType );
			}
		}

		Table->CycleRow();
	}


}

void UContentComparisonCommandlet::GatherClassBasedData( ULinkerLoad* Linker, const UClass* const InClassToGatherDataOn )
{
	ClassesTaggedInDependentAssetsList.AddUniqueItem(InClassToGatherDataOn->GetPathName());
	ClassesTaggedInDependentAssetsList_Tags.AddUniqueItem(InClassToGatherDataOn->GetName());

	INT ExportIndex = INDEX_NONE;

	for( INT i = 0; i < Linker->ExportMap.Num(); ++i )
	{
		const FObjectExport& Export = Linker->ExportMap(i);

		// find the name of this object's class
		const INT ClassIndex = Export.ClassIndex;

		const FName ClassName = ClassIndex > 0 
			? Linker->ExportMap(ClassIndex-1).ObjectName
			: IS_IMPORT_INDEX(ClassIndex)
			? Linker->ImportMap(-ClassIndex-1).ObjectName
			: FName(NAME_Class);


		//warnf( TEXT("ClassName: %s %d  %s"), *ClassName.ToString(), Export.SerialSize, *InClassToGatherDataOn->GetFName().ToString() );

		// here we get the export index and we we can use that in the code below
		const FString ClassNameToAnalyze = InClassToGatherDataOn->GetFName().ToString(); // TEXT("GearWeap_Boomshot");
		if( ClassName.ToString().Right(ClassNameToAnalyze.Len()).InStr( ClassNameToAnalyze, TRUE, TRUE ) != INDEX_NONE )
		{
			warnf( TEXT("ClassName: %s %d"), *ClassName.ToString(), Export.SerialSize );
			ExportIndex = i;
			break;
		}
	}


	if( ExportIndex == INDEX_NONE )
	{
		warnf( TEXT( "ARRRGGHHH" ) );
	}



	FString PathName = Linker->GetExportPathName( ExportIndex );

	FString ClassName = Linker->GetExportPathName( ExportIndex, NULL, FALSE );

	//UClass* TheClass = (UClass*)UObject::StaticFindObject( UClass::StaticClass(), ANY_PACKAGE, *ClassName.ToString() );
	//warnf( TEXT("PathName: '%s'  TheClass: %s"), *PathName, *ClassName );

	UClass* TheClass = (UClass*)UObject::StaticLoadObject( UClass::StaticClass(), ANY_PACKAGE, *ClassName, NULL, LOAD_NoWarn|LOAD_Quiet, NULL );

	warnf( TEXT("Loaded Class: %s"), *TheClass->GetFullName() );



	TSet<FDependencyRef> AllDepends;
	Linker->GatherExportDependencies(ExportIndex, AllDepends, FALSE);
	//warnf(TEXT("\t\t\tAll Depends:  Num Depends: %d") , AllDepends.Num());
	INT DependsIndex = 0;
	for(TSet<FDependencyRef>::TConstIterator It(AllDepends);It;++It)
	{

		const FDependencyRef& Ref = *It;

		UBOOL bIsImport = IS_IMPORT_INDEX(Ref.ExportIndex);
//		warnf(TEXT("\t\t\t%i)%s (%i)  IsImport: %d"),
//			DependsIndex++,
//			IS_IMPORT_INDEX(Ref.ExportIndex)
//			? *Ref.Linker->GetImportFullName(Ref.ExportIndex)
//			: *Ref.Linker->GetExportFullName(Ref.ExportIndex),
//			Ref.ExportIndex,
//			bIsImport
//			);

		UObject* Object	= NULL;

		// 			if( IS_IMPORT_INDEX(Ref.ExportIndex) )
		// 			{
		// 				Object = Ref.Linker->CreateImport( Ref.ExportIndex );
		// 			}
		// 			else
		// 			{
		// 				Object = Ref.Linker->CreateExport( Ref.ExportIndex );
		// 			}
		// 

		FString PathName = Ref.Linker->GetExportPathName( Ref.ExportIndex );

		FName ClassName = Ref.Linker->GetExportClassName( Ref.ExportIndex );

		UClass* TheClass = (UClass*)UObject::StaticFindObject( UClass::StaticClass(), ANY_PACKAGE, *ClassName.ToString(), TRUE );

		//UClass* TheClass = (UClass*)UObject::StaticLoadObject( UClass::StaticClass(), NULL, *ClassName.ToString(), NULL, LOAD_None, NULL );

		Object = UObject::StaticFindObject( TheClass, ANY_PACKAGE, *PathName, TRUE );

		//Object = UObject::StaticLoadObject( TheClass, NULL, *PathName, NULL, LOAD_None, NULL );



		if( Object != NULL )
		{
			// This will cause the object to be serialized. We do this here for all objects and
			// not just UClass and template objects, for which this is required in order to ensure
			// seek free loading, to be able introduce async file I/O.
			Ref.Linker->Preload( Object );
		}

		if( Object != NULL )
		{
			Object->StripData(UE3::PLATFORM_Console);

			// now see how big it is
			FArchiveCountMem ObjectToSizeUp( Object ); 

			//warnf( TEXT("\t\t\t\t GetNum: %8d  GetMax: %8d  GetResourceSize: %8d"), ObjectToSizeUp.GetNum(), ObjectToSizeUp.GetMax(), Object->GetResourceSize() );

			//warnf( TEXT("\t\t\t\t STRIPPED GetNum: %8d  GetMax: %8d  GetResourceSize: %8d"), ObjectToSizeUp.GetNum(), ObjectToSizeUp.GetMax(), Object->GetResourceSize() );

			// look in our list and see if this object is one we care about
			for( INT i = 0; i< TypesToLookFor.Num(); ++i )
			{
				TypesToLookForDatum& TheType = TypesToLookFor(i);

				UBOOL bAdded = TheType.PossiblyAddObject( Object );
				if ((bGenerateAssetGrid && bAdded) || bGenerateFullAssetGrid)
				{
					// Don't bother adding packages...
					FString ObjectFullName = Object->GetFullName();
					if ((Object->IsA(UPackage::StaticClass()) == FALSE) &&
						(ObjectFullName.InStr(TEXT(":")) == INDEX_NONE))
					{
						FDependentAssetInfo* Info = DependentAssets.Find(ObjectFullName);
						if (Info == NULL)
						{
							FDependentAssetInfo TempInfo;
							DependentAssets.Set(ObjectFullName, TempInfo);
							Info = DependentAssets.Find(ObjectFullName);

							Info->AssetName = ObjectFullName;
							Info->ResourceSize = Object->GetResourceSize();
						}
						check(Info);
						Info->ClassesThatDependOnAsset.Set(InClassToGatherDataOn->GetPathName(), TRUE);
					}
				}
			}
		}
	}
}

void UContentComparisonCommandlet::ReportComparisonData( const FString& ClassToCompare )
{
	warnf( TEXT("Stats for:  %s"), *ClassToCompare );
	if( Table != NULL )
	{
		Table->AddColumn( TEXT("%s"), *ClassToCompare );
	}


	// report all of the results
	for( INT i = 0; i< TypesToLookFor.Num(); ++i )
	{
		TypesToLookForDatum& TheType = TypesToLookFor(i);

		warnf( TEXT("%s: %f KB"), *TheType.Class->GetName(), TheType.TotalSize/1024.0f );

		if( Table != NULL )
		{
			Table->AddColumn( TEXT("%f"), TheType.TotalSize/1024.0f );
		}


		// reset
		TheType.TotalSize = 0;
	}

	if( Table != NULL )
	{
		Table->CycleRow();
	}

}

void UContentComparisonCommandlet::ReportClassAssetDependencies(const FString& InTableName)
{
	TMap<FString,TArray<FString>> ClassToAssetsTagList;
	TArray<FString> UsedByMultipleClassesAssetList;

	if (bGenerateAssetGrid || bGenerateFullAssetGrid)
	{
		FDiagnosticTableViewer* AssetTable = NULL;
		if (bSkipAssetCSV == FALSE)
		{
			AssetTable = new FDiagnosticTableViewer(
			*FDiagnosticTableViewer::GetUniqueTemporaryFilePath(
				*(
					FString(TEXT("ContentComparison\\")) + 
					FString::Printf(TEXT("ContentComparison-%d-%s\\"), GetChangeListNumberForPerfTesting(), *CurrentTime) +
					FString::Printf(TEXT("ContentComparison-%s-%s"), bGenerateFullAssetGrid ? TEXT("FullAssets") : TEXT("Assets"), *InTableName)
					)
				)
			);
		}

		if (AssetTable != NULL)
		{
			// Fill in the header row
			AssetTable->AddColumn(TEXT("Asset"));
			AssetTable->AddColumn(TEXT("ResourceSize(kB)"));
			for (INT ClassIdx = 0; ClassIdx < ClassesTaggedInDependentAssetsList.Num(); ClassIdx++)
			{
				AssetTable->AddColumn(*(ClassesTaggedInDependentAssetsList(ClassIdx)));
			}
			AssetTable->CycleRow();
		}

		// Fill it in
		for (TMap<FString,FDependentAssetInfo>::TIterator DumpIt(DependentAssets); DumpIt; ++DumpIt)
		{
			FDependentAssetInfo& Info = DumpIt.Value();

			if (AssetTable != NULL)
			{
				AssetTable->AddColumn(*(Info.AssetName));
				AssetTable->AddColumn(TEXT("%f"), Info.ResourceSize/1024.0f);
			}
			for (INT ClassIdx = 0; ClassIdx < ClassesTaggedInDependentAssetsList.Num(); ClassIdx++)
			{
				FString ClassName = ClassesTaggedInDependentAssetsList(ClassIdx);
				FString TagName = ClassesTaggedInDependentAssetsList_Tags(ClassIdx);
				if (Info.ClassesThatDependOnAsset.Find(ClassName) != NULL)
				{
					if (AssetTable != NULL)
					{
						AssetTable->AddColumn(TEXT("XXX"));
					}
					if (bSkipTagging == FALSE)
					{
						// Add to the list of assets for tagging
						TArray<FString>* ClassAssetList = ClassToAssetsTagList.Find(TagName);
						if (ClassAssetList == NULL)
						{
							TArray<FString> TempList;
							ClassToAssetsTagList.Set(TagName, TempList);
							ClassAssetList = ClassToAssetsTagList.Find(TagName);
						}
						check(ClassAssetList);
						ClassAssetList->AddUniqueItem(Info.AssetName);
					}
				}
				else
				{
					if (AssetTable != NULL)
					{
						AssetTable->AddColumn(TEXT(""));
					}
				}
			}
			if (AssetTable != NULL)
			{
				AssetTable->CycleRow();
			}

			if (bSkipTagging == FALSE)
			{
				// If it is used by more than one class, then add it to the list
				// of items to be tagged w/ the "MultipleClasses" tag
				if (Info.ClassesThatDependOnAsset.Num() > 1)
				{
					UsedByMultipleClassesAssetList.AddUniqueItem(Info.AssetName);
				}
			}
		}

		if (AssetTable != NULL)
		{
			// Close it and kill it
			AssetTable->Close();
			delete AssetTable;
		}

		if (bSkipTagging == FALSE)
		{
			// Clear all the tags... this is a drag, but needs to be done.
			FString TagPrefix = TEXT("Audit.");
			TagPrefix += InTableName;

			FString MultipleTag = TagPrefix;
			MultipleTag += TEXT("_MULTIPLECLASSES");
			debugf(TEXT("Clearing tag %s"), *MultipleTag);
			for (INT TagIdx = 0; TagIdx < ClassesTaggedInDependentAssetsList_Tags.Num(); TagIdx++)
			{
				debugf(TEXT("Clearing tag %s_%s"), *TagPrefix, *(ClassesTaggedInDependentAssetsList_Tags(TagIdx)));
			}

			// Tag the multiples
			GADHelper->SetTaggedAssets(MultipleTag, UsedByMultipleClassesAssetList);
			// Tag each class
			for (TMap<FString,TArray<FString>>::TIterator TagIt(ClassToAssetsTagList); TagIt; ++TagIt)
			{
				FString ClassName = TagIt.Key();
				FString TagName = TagPrefix;
				TagName += TEXT("_");
				TagName += ClassName;
				TArray<FString>& AssetList = TagIt.Value();

				GADHelper->SetTaggedAssets(TagName, AssetList);
			}
		}

		// Clear the lists
		ClassesTaggedInDependentAssetsList.Empty();
		DependentAssets.Empty();
	}
}

IMPLEMENT_CLASS(UContentComparisonCommandlet)