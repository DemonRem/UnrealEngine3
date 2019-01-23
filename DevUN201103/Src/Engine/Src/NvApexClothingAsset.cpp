/*=============================================================================
	NvApexClothingAsset.cpp : Manages APEX clothign assets.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

// This code contains NVIDIA Confidential Information and is disclosed
// under the Mutual Non-Disclosure Agreement.
//
// Notice
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright 2009-2010 NVIDIA Corporation. All rights reserved.
// Copyright 2002-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright 2001-2006 NovodeX. All rights reserved.

#include "EnginePrivate.h"

#if WITH_NOVODEX

#include "NvApexManager.h"

#endif

#include "EngineMeshClasses.h"
#include "EnginePhysicsClasses.h"
#include "EngineParticleClasses.h"
#include "EngineMaterialClasses.h"
#include "MeshUtils.h"
#include "NvApexScene.h"
#include "UnNovodexSupport.h"
#include "NvApexCommands.h"
#include "UnNovodexSupport.h"

#if WITH_APEX

#include <NxApexSDK.h>
#include <NxUserRenderResourceManager.h>
#include <NxParameterized.h>
#include "NvApexCommands.h"
#include "NvApexGenericAsset.h"

#endif


/*
*	UApexClothingAsset
*
*/
IMPLEMENT_CLASS(UApexClothingAsset);

void UApexClothingAsset::PostLoad()
{
#if WITH_APEX && !WITH_APEX_SHIPPING
	if( GIsEditor && !GIsGame )
	{
		if( ApexClothingLibrary_DEPRECATED != NULL )
		{		
#if 0
			// This is an APEX 0.9 asset; force it to be resaved as an APEX 1.0 asset
			MarkPackageDirty();
#endif // 0

			// Reverse tri winding for old assets (because the old applyTransformation method didn't do it)
			MApexAsset->MirrorLegacyClothingNormalsAndTris();
		}
	}
#endif
	Super::PostLoad();
}

void UApexClothingAsset::Serialize(FArchive& Ar)
{
	Super::Serialize( Ar );

#if WITH_APEX
	InitializeApex();
	DWORD bAssetValid = MApexAsset != NULL ? 1 : 0;
#else
	DWORD bAssetValid = 1;
#endif
	Ar.SerializeInt( bAssetValid, 1 );

	if( bAssetValid >= 1  )
	{
		if( Ar.IsLoading() )
		{
			TArray<BYTE> NameBuffer;
			UINT NameBufferSize;
			Ar << NameBufferSize;
			NameBuffer.Add( NameBufferSize );
			Ar.Serialize( NameBuffer.GetData(), NameBufferSize );

			TArray<BYTE> Buffer;
			UINT Size;
			Ar << Size;
			Buffer.Add( Size );
			Ar.Serialize( Buffer.GetData(), Size );
#if WITH_APEX
			if ( MApexAsset != NULL )
			{
				MApexAsset->DecRefCount(0);
				MApexAsset = NULL;
			}
			char scratch[1024];
			GetApexAssetName(this,scratch,1024,".aca");
     		MApexAsset = GApexCommands->GetApexAssetFromMemory(scratch, (const void *)Buffer.GetData(), (NxU32)Size,  TCHAR_TO_ANSI(*OriginalApexName), this );
			if ( MApexAsset )
			{
				MApexAsset->IncRefCount(0);
				assert( MApexAsset->GetType() == AAT_CLOTHING );
			}
#else
#endif
		}
		else if ( Ar.IsSaving() )
		{
			const char *name = "NO_APEX";
#if WITH_APEX
			name = MApexAsset->GetAssetName();
#endif
			if( name )
			{
				DWORD nameBufferSize = strlen( name )+1;
				Ar << nameBufferSize;
				Ar.Serialize( (void*)name, nameBufferSize );
			}
			else
			{
				DWORD nullNameBufferSize  = 2;
				Ar << nullNameBufferSize;
				Ar.Serialize( (void *)"", nullNameBufferSize );
			}
#if WITH_APEX
			assert( MApexAsset->GetType() == AAT_CLOTHING );
			NxU32 dlen=0;
			const void * data = MApexAsset->Serialize(dlen);
			Ar << dlen;
			if ( data )
			{
				Ar.Serialize( (void *)data, dlen );
				MApexAsset->ReleaseSerializeMemory(data);
			}
#else
			DWORD size=0;
			Ar << size;
#endif
		}
	}
}

UBOOL UApexClothingAsset::Rename( const TCHAR* InName, UObject* NewOuter, ERenameFlags Flags )
{
	UBOOL ret = Super::Rename(InName,NewOuter,Flags);
	if ( MApexAsset )
	{
#if WITH_APEX
		MApexAsset->Rename(TCHAR_TO_ANSI(InName));
#endif
	}
	return ret;
}

TArray<FString> UApexClothingAsset::GetGenericBrowserInfo()
{
	TArray<FString> Info;
#if WITH_APEX
	if( MApexAsset )
	{
		FString nameString( MApexAsset->GetAssetName() );
		Info.AddItem( FString::Printf( TEXT("%s"), *nameString ) );
		Info.AddItem(TEXT("APEX CLOTHING ASSET") );
	}
#endif
	return Info;

}

UBOOL UApexClothingAsset::Import( const BYTE* Buffer, INT BufferSize, const FString& Name,UBOOL convertToUE3Coodinates)
{
	UBOOL ret = FALSE;
#if WITH_APEX && !FINAL_RELEASE && !WITH_APEX_SHIPPING
	InitializeApex();
	if ( GApexCommands )
	{
		if ( MApexAsset )
		{
			MApexAsset->DecRefCount(0);
			MApexAsset = NULL;
		}
		char scratch[1024];
		GetApexAssetName(this,scratch,1024,".aca");

		MApexAsset = GApexCommands->GetApexAssetFromMemory(scratch, (const void *)Buffer, (NxU32)BufferSize, GApexManager->GetCurrentImportFileName(), this );

		if ( MApexAsset )
		{
			AnsiToString( MApexAsset->GetOriginalApexName(),OriginalApexName );
			assert( MApexAsset->GetType() == AAT_CLOTHING );
			MApexAsset->IncRefCount(0);
			if ( convertToUE3Coodinates )
			{
				physx::PxMat44 mat = physx::PxMat44::getIdentity();
				physx::PxF32 *temp = (physx::PxF32 *)mat.front();
				temp[1*4+1] = -1;
				MApexAsset->ApplyTransformation(mat,U2PScale,TRUE,TRUE,FALSE);
			}
			else
			{
				physx::PxMat44 mat = physx::PxMat44::getIdentity();
				MApexAsset->ApplyTransformation(mat,1,TRUE,TRUE,FALSE);
			}
			GApexCommands->AddUpdateMaterials(this);
			ret = TRUE;
		}
	}
#endif
	return ret;
}

#if WITH_APEX
static const char * getNameClothing(const char *src)
{
	if (src == NULL ) src = "";
	NxU32 len = (NxU32) strlen(src);
	char *temp = (char *) appMalloc(len+1);
	memcpy(temp,src,len+1);
	return temp;
}
#endif

void UApexClothingAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
#if WITH_APEX
	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	if(PropertyThatChanged && PropertyThatChanged->GetFName() == FName(TEXT("Materials")))
	{
  		if( MApexAsset )
  		{
  			NxU32 mcount = (NxU32)Materials.Num();
  			if ( mcount )
  			{
  				const char **names = new const char *[mcount];
    			for(NxU32 i=0; i<mcount; i++)
    			{
    				UMaterialInterface *Material = Materials(i);
    				InitMaterialForApex(Material);
					const FString MaterialPath = Material ? Material->GetPathName() : TEXT("EngineMaterials.DefaultMaterial");
    				names[i] = getNameClothing(TCHAR_TO_ANSI(*MaterialPath));
    			}
				MApexAsset->ApplyMaterialRemap(mcount,names);
				for (NxU32 i=0; i<mcount; i++)
				{
					appFree( (void *)names[i] );
				}
    			delete []names;
    		}
  		}
	}
	else if( PropertyThatChanged ) // for any other property, cause the actor to be re-created.
	{
		MApexAsset->RefreshActors();
	}
#endif
}

void UApexClothingAsset::BeginDestroy(void)
{
	Super::BeginDestroy();
#if WITH_APEX
	if ( GApexCommands )
	{
		GApexCommands->NotifyDestroy(this);
	}
	if ( MApexAsset )
	{
		MApexAsset->DecRefCount(0);
		MApexAsset = NULL;
	}
#endif
}

void UApexClothingAsset::UpdateMaterials(void)
{
#if WITH_APEX
	Materials.Empty();
    if ( MApexAsset )
    {
        NxU32 NumSubmeshes = MApexAsset->GetNumMaterials();
		Materials.Empty(NumSubmeshes);
		for(UINT i=0; i<NumSubmeshes; i++)
		{
			const char         *MaterialNameA = MApexAsset->GetMaterialName(i);
			LoadMaterialForApex(MaterialNameA, Materials);
		}
	}

#endif
}

void * UApexClothingAsset::GetNxParameterized(void)
{
	void *ret = NULL;
#if WITH_APEX
	if ( MApexAsset )
	{
		::NxParameterized::Interface *params = MApexAsset->GetDefaultApexActorDesc();
		if ( params )
		{
			::NxParameterized::Handle handle(*params);
        	UBOOL bDisablePhysXHardware = !IsPhysXHardwarePresent();
			NxParameterized::setParamBool(*params,"useHardwareCloth",bDisablePhysXHardware ? FALSE : bUseHardwareCloth);
			NxParameterized::setParamBool(*params,"useInternalBoneOrder",TRUE);
			NxParameterized::setParamBool(*params,"multiplyGlobalPoseIntoBones", FALSE);
			NxParameterized::setParamBool(*params,"updateStateWithGlobalMatrices",TRUE);
			NxParameterized::setParamBool(*params,"fallbackSkinning",bFallbackSkinning);
			NxParameterized::setParamBool(*params,"slowStart",bSlowStart);
			NxParameterized::setParamBool(*params,"flags.RecomputeNormals",bRecomputeNormals);
			NxParameterized::setParamU32(*params,"uvChannelForTangentUpdate",UVChannelForTangentUpdate);
			NxParameterized::setParamF32(*params,"maxDistanceBlendTime",MaxDistanceBlendTime);
			NxParameterized::setParamF32(*params,"lodWeights.maxDistance",LodWeightsMaxDistance*U2PScale);
			NxParameterized::setParamF32(*params,"lodWeights.distanceWeight",LodWeightsDistanceWeight);
			NxParameterized::setParamF32(*params,"lodWeights.bias",LodWeightsBias);
			NxParameterized::setParamF32(*params,"lodWeights.benefitsBias",LodWeightsBenefitsBias);
		}
		ret = params;
	}
#endif
	return ret;
}
