/*=============================================================================
	NvApexDestructibleAsset.cpp : Handles APEX destructible assets.
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
#include "NvApexManager.h"
#include "UnNovodexSupport.h"



#if WITH_APEX

#include <NxApexSDK.h>
#include <NxUserRenderResourceManager.h>
#include <NxParameterized.h>
#include "NvApexCommands.h"
#include <NxModuleDestructible.h>
#include <NxDestructibleAsset.h>
#include <NxDestructiblePreview.h>
#include <NxDestructibleActor.h>

#include <PxBounds3.h>
#include <PsString.h>
#include "NvApexCommands.h"
#include "NvApexGenericAsset.h"

using namespace physx::apex;

#endif


/*
 *	UApexDestructibleAsset
 *
 */
IMPLEMENT_CLASS(UApexDestructibleAsset);

void UApexDestructibleAsset::PostLoad()
{
#if WITH_APEX
	FixupAsset();
#endif
	Super::PostLoad();
}

void UApexDestructibleAsset::Serialize(FArchive& Ar)
{

	Super::Serialize( Ar );

#if WITH_APEX
	InitializeApex();
	DWORD bAssetValid = MApexAsset ? 1 : 0;
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
            if ( bAssetValid >= 1 )
			{
			}

			TArray<BYTE> Buffer;
			DWORD Size;
			Ar << Size;
			Buffer.Add( Size );
			Ar.Serialize( Buffer.GetData(), Size );
#if WITH_APEX
			char scratch[1024];
			GetApexAssetName(this,scratch,1024,".pda");
     		MApexAsset = GApexCommands->GetApexAssetFromMemory(scratch, (const void *)Buffer.GetData(), (NxU32)Size,  TCHAR_TO_ANSI(*OriginalApexName), this );
			if ( MApexAsset )
			{
				MApexAsset->IncRefCount(0);
				assert( MApexAsset->GetType() == AAT_DESTRUCTIBLE );
			}

			if ( GApexManager )
			{
				NxApexSDK *apexSDK = GApexManager->GetApexSDK();
				NxResourceProvider* ApexResourceProvider = apexSDK->getNamedResourceProvider();
				if( ApexResourceProvider )
				{
					for(INT i=0; i<Materials.Num(); i++)
					{
						UMaterialInterface* UseMat = Materials(i);
						if( UseMat != NULL )
						{
							ApexResourceProvider->setResource( "ApexMaterials", TCHAR_TO_ANSI(*UseMat->GetPathName()), UseMat );
						}
					}
				}
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
			assert( MApexAsset->GetType() == AAT_DESTRUCTIBLE );
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

TArray<FString> UApexDestructibleAsset::GetGenericBrowserInfo()
{
	TArray<FString> Info;
#if WITH_APEX
	NxDestructibleAsset* nDestructibleAsset = NULL;
	if ( MApexAsset )
	{
		NxApexAsset *a_asset = MApexAsset->GetNxApexAsset();
		nDestructibleAsset = static_cast< NxDestructibleAsset *>(a_asset);
	}
	if( nDestructibleAsset != NULL )
	{
		FString nameString( nDestructibleAsset->getName() );
		Info.AddItem( FString::Printf( TEXT("%s"), *nameString ) );
		Info.AddItem( FString::Printf( TEXT("%d Chunks, %d Levels"), nDestructibleAsset->chunkCount(), nDestructibleAsset->depthCount() ) );
	}
#endif
	return Info;
}


#if WITH_APEX

static UBOOL getParamBool(::NxParameterized::Interface *pm,const char *name)
{
	UBOOL ret = FALSE;
	if ( pm )
	{
#if NX_APEX_SDK_RELEASE >= 100
		::NxParameterized::Handle handle(*pm);
#else
		::NxParameterized::Handle handle;
#endif
		if ( pm->getParameterHandle(name,handle) == ::NxParameterized::ERROR_NONE )
		{
			bool bRet;
#if NX_APEX_SDK_RELEASE >= 100
			::NxParameterized::ErrorType vret = handle.getParamBool(bRet);
#else
			::NxParameterized::ErrorType vret = pm->getParamBool(handle,bRet);
#endif
			ret = bRet ? TRUE : FALSE;
			assert( vret == ::NxParameterized::ERROR_NONE );
		}
		else
			assert(0);
	}
	return ret;
}

static void getParamF32(::NxParameterized::Interface *pm,const char *name,FLOAT &value)
{
	if( !pm ) return;
#if NX_APEX_SDK_RELEASE >= 100
	::NxParameterized::Handle handle(*pm);
#else
	::NxParameterized::Handle handle;
#endif
	if ( pm->getParameterHandle(name,handle) == ::NxParameterized::ERROR_NONE )
	{
#if NX_APEX_SDK_RELEASE >= 100
		::NxParameterized::ErrorType ret = handle.getParamF32(value);
#else
		::NxParameterized::ErrorType ret = pm->getParamF32(handle,value);
#endif
		assert( ret == ::NxParameterized::ERROR_NONE  );
	}
	else
		assert(0);
}

static void getParamInt(::NxParameterized::Interface *pm,const char *name,INT &value)
{
	if ( !pm ) return;
#if NX_APEX_SDK_RELEASE >= 100
	::NxParameterized::Handle handle(*pm);
#else
	::NxParameterized::Handle handle;
#endif
	if ( pm->getParameterHandle(name,handle) == ::NxParameterized::ERROR_NONE )
	{
#if NX_APEX_SDK_RELEASE >= 100
		::NxParameterized::ErrorType ret = handle.getParamI32(value);
#else
		::NxParameterized::ErrorType ret = pm->getParamI32(handle,value);
#endif
		if  ( ret != ::NxParameterized::ERROR_NONE  )
		{
			physx::PxU32 uv;
#if NX_APEX_SDK_RELEASE >= 100
			ret = handle.getParamU32(uv);
#else
			ret = pm->getParamU32(handle,uv);
#endif
			assert( ret == ::NxParameterized::ERROR_NONE );
			value = (INT)uv;
		}
	}
	else
		assert(0);
}


static void getParamString(::NxParameterized::Interface *pm,const char *name,FString &value)
{
	if ( !pm ) return;
#if NX_APEX_SDK_RELEASE >= 100
	::NxParameterized::Handle handle(*pm);
#else
	::NxParameterized::Handle handle;
#endif
	if ( pm->getParameterHandle(name,handle) == ::NxParameterized::ERROR_NONE )
	{
		const char *str = NULL;
#if NX_APEX_SDK_RELEASE >= 100
		handle.getParamString(str);
#else
		pm->getParamString(handle,str);
#endif
		if ( str )
		{
			value = str;
		}
	}
	else
		assert(0);
}

static void getParam(::NxParameterized::Interface *pm,const char *name,physx::PxBounds3 &value)
{
	if ( !pm ) return;
#if NX_APEX_SDK_RELEASE >= 100
	::NxParameterized::Handle handle(*pm);
#else
	::NxParameterized::Handle handle;
#endif
	if ( pm->getParameterHandle(name,handle) == ::NxParameterized::ERROR_NONE )
	{
#if NX_APEX_SDK_RELEASE >= 100
		::NxParameterized::ErrorType ret = handle.getParamBounds3(value);
#else
		::NxParameterized::ErrorType ret = pm->getParamBounds3(handle,value);
#endif
		assert( ret == ::NxParameterized::ERROR_NONE );
	}
	else
		assert(0);
}
#endif

#if WITH_APEX
static const char* GetNameDestructible(const char *Src);

static void SetApexNamedMatIndices(const char** Names, INT NumMaterials)
{
	// Get APEX's named resource provider
	NxResourceProvider* ApexResourceProvider = NULL;
	if ( GApexManager != NULL )
	{
		NxApexSDK* ApexSDK = GApexManager->GetApexSDK();
		if ( ApexSDK != NULL )
		{
			ApexResourceProvider = ApexSDK->getNamedResourceProvider();
		}
	}
	check( ApexResourceProvider );

	// Map unique names to material indices within APEX, must be done after ApplyMaterialRemap
	for ( INT MatIdx = 0; MatIdx < NumMaterials; ++MatIdx )
	{
		// Index offset by 1 so we can differentiate between no material and material index 0
		ApexResourceProvider->setResource( APEX_MATERIALS_NAME_SPACE, Names[MatIdx], (void*)(MatIdx+1) );
	}
}

static void SetApexMatIndices(UApexAsset* Asset)
{
	INT NumMaterials = Asset->GetNumMaterials();
	FString NamePrefix = Asset->GetFullName();
	NamePrefix += TEXT(".");

	// Create a list of ASCII char* names for ApplyMaterialRemap.
	const char **Names = new const char *[NumMaterials];
	for ( INT MatIdx = 0; MatIdx < NumMaterials; ++MatIdx )
	{
		// Create the unique name and convert to ASCII
		FString UniqueName = NamePrefix + appItoa( MatIdx );
		Names[MatIdx] = GetNameDestructible(TCHAR_TO_ANSI(*UniqueName));
	}

	SetApexNamedMatIndices( Names, NumMaterials );

	// Free memory allocated by GetNameDestructible
	for ( INT MatIdx = 0; MatIdx < NumMaterials; ++MatIdx )
	{
		if ( Names[MatIdx] != NULL )
		{
			appFree( (void *)Names[MatIdx] );
		}
	}
	delete [] Names;
}

static void SetUniqueAssetMaterialNames(UApexAsset* Asset, FIApexAsset* ApexAssetWrapper)
{
	check( GIsEditor && !GIsGame );

	UBOOL bAssetNeedsUpdate = FALSE;
	INT NumMaterials = Asset->GetNumMaterials();
	FString NamePrefix = Asset->GetFullName();
	NamePrefix += TEXT(".");

	// TODO: Check to see if we really need to update the asset
#if 0
	const ::NxParameterized::Interface* Params = ApexAssetWrapper->GetNxApexAsset()->getAssetNxParameterized();

	for ( INT MatIdx = 0; MatIdx < NumMaterials; ++MatIdx )
	{
		const char* AssetMaterialName;
		char MatName[MAX_SPRINTF];

		// Read the value of materialNames[N] from APEX
		appSprintfANSI( MatName, "renderMeshAsset.materialNames[%d]", MatIdx );

		::NxParameterized::Handle MatNameHandle( *Params );
		::NxParameterized::ErrorType Err = MatNameHandle.getParameter( MatName );
		check( Err == ::NxParameterized::ERROR_NONE );
		if ( Err == ::NxParameterized::ERROR_NONE )
		{
			Err = MatNameHandle.getParamString( AssetMaterialName );
			
			// Check if it's set to the correct unique name
			FString UniqueName = NamePrefix + appItoa( MatIdx );
			if ( appStrcmp(*UniqueName, ANSI_TO_TCHAR(AssetMaterialName)) != 0 )
			{
				bAssetNeedsUpdate = TRUE;
				break;
			}
		}
	}
#else
	bAssetNeedsUpdate = TRUE;
#endif

	if ( bAssetNeedsUpdate )
	{
		// Create a list of ASCII char* names for ApplyMaterialRemap.
		const char **Names = new const char *[NumMaterials];
		for ( INT MatIdx = 0; MatIdx < NumMaterials; ++MatIdx )
		{
			// Create the unique name and convert to ASCII
			FString UniqueName = NamePrefix + appItoa( MatIdx );
			Names[MatIdx] = GetNameDestructible(TCHAR_TO_ANSI(*UniqueName));
		}

		// Write new material names into APEX asset
		ApexAssetWrapper->ApplyMaterialRemap( (physx::PxU32)NumMaterials, Names );

		// Map unique names to material indices within APEX, must be done after ApplyMaterialRemap
		SetApexNamedMatIndices( Names, NumMaterials );

		// Free memory allocated by GetNameDestructible
		for ( INT MatIdx = 0; MatIdx < NumMaterials; ++MatIdx )
		{
			if ( Names[MatIdx] != NULL )
			{
				appFree( (void *)Names[MatIdx] );
			}
		}
		delete [] Names;
	}
}
#endif

UBOOL UApexDestructibleAsset::Import( const BYTE* Buffer, INT BufferSize, const FString& Name, UBOOL convertToUE3Coordinates )
{
#if WITH_APEX && !FINAL_RELEASE && !WITH_APEX_SHIPPING
	InitializeApex();
	OnApexAssetLost();
	if ( MApexAsset )
	{
		MApexAsset->DecRefCount(0);
	}

	char scratch[1024];
	GetApexAssetName(this,scratch,1024,".pda");
	MApexAsset = GApexCommands->GetApexAssetFromMemory(scratch, Buffer, BufferSize, GApexManager->GetCurrentImportFileName(), this );

	if ( MApexAsset )
	{
		AnsiToString( MApexAsset->GetOriginalApexName(),OriginalApexName );
		assert( MApexAsset->GetType() == AAT_DESTRUCTIBLE );
		MApexAsset->IncRefCount(0);
		::NxParameterized::Interface *params = MApexAsset->GetDefaultApexActorDesc();
		if ( params )
		{
			bDynamic = getParamBool(params,"dynamic");

			getParamString(params,"crumbleEmitterName",CrumbleEmitterName);
			getParamString(params,"dustEmitterName",DustEmitterName);
			bDynamic = getParamBool(params,"dynamic");

			getParamF32(params,"destructibleParameters.damageThreshold",			DestructibleParameters.DamageThreshold);
			getParamF32(params,"destructibleParameters.damageToRadius",				DestructibleParameters.DamageToRadius);
			getParamF32(params,"destructibleParameters.damageCap",					DestructibleParameters.DamageCap);
			getParamF32(params,"destructibleParameters.forceToDamage",				DestructibleParameters.ForceToDamage);
			DestructibleParameters.ForceToDamage *= P2UScale;
			getParamF32(params,"destructibleParameters.impactVelocityThreshold",	DestructibleParameters.ImpactVelocityThreshold);
			DestructibleParameters.ImpactVelocityThreshold *= P2UScale;
			getParamF32(params,"destructibleParameters.materialStrength",			DestructibleParameters.MaterialStrength);
			DestructibleParameters.MaterialStrength *= P2UScale;
			getParamF32(params,"destructibleParameters.damageToPercentDeformation",	DestructibleParameters.DamageToPercentDeformation);
			getParamF32(params,"destructibleParameters.deformationPercentLimit",	DestructibleParameters.DeformationPercentLimit);
			int FormExtendedStructures;
			getParamInt(params,"destructibleParameters.formExtendedStructures",		FormExtendedStructures);
			DestructibleParameters.bFormExtendedStructures = FormExtendedStructures != 0;
			getParamInt(params,"destructibleParameters.supportDepth",				DestructibleParameters.SupportDepth);
			getParamInt(params,"destructibleParameters.debrisDepth",				DestructibleParameters.DebrisDepth);
			getParamInt(params,"destructibleParameters.essentialDepth",				DestructibleParameters.EssentialDepth);
			getParamF32(params,"destructibleParameters.debrisLifetimeMin",			DestructibleParameters.DebrisLifetimeMin);
			getParamF32(params,"destructibleParameters.debrisLifetimeMax",			DestructibleParameters.DebrisLifetimeMax);
			getParamF32(params,"destructibleParameters.debrisMaxSeparationMin",		DestructibleParameters.DebrisMaxSeparationMin);
			getParamF32(params,"destructibleParameters.debrisMaxSeparationMax",		DestructibleParameters.DebrisMaxSeparationMax);

			physx::PxBounds3 b;
			getParam(params,"destructibleParameters.validBounds",b);

			if( b.isEmpty() )
			{
				DestructibleParameters.ValidBounds.IsValid = FALSE;
			}
			else
			{
				DestructibleParameters.ValidBounds.Min.X = b.minimum.x * P2UScale;
				DestructibleParameters.ValidBounds.Min.Y = b.minimum.y * P2UScale;
				DestructibleParameters.ValidBounds.Min.Z = b.minimum.z * P2UScale;
														
				DestructibleParameters.ValidBounds.Max.X = b.maximum.x * P2UScale;
				DestructibleParameters.ValidBounds.Max.Y = b.maximum.y * P2UScale;
				DestructibleParameters.ValidBounds.Max.Z = b.maximum.z * P2UScale;

				DestructibleParameters.ValidBounds.IsValid = TRUE;
			}

			getParamF32(params,"destructibleParameters.maxChunkSpeed",				DestructibleParameters.MaxChunkSpeed);
			DestructibleParameters.MaxChunkSpeed *= P2UScale;
			getParamF32(params,"destructibleParameters.massScaleExponent",			DestructibleParameters.MassScaleExponent);

			DestructibleParameters.Flags.ACCUMULATE_DAMAGE = 		getParamBool(params,"destructibleParameters.flags.ACCUMULATE_DAMAGE");
			DestructibleParameters.Flags.ASSET_DEFINED_SUPPORT =   	getParamBool(params,"destructibleParameters.flags.ASSET_DEFINED_SUPPORT");
			DestructibleParameters.Flags.WORLD_SUPPORT =           	getParamBool(params,"destructibleParameters.flags.WORLD_SUPPORT");
			DestructibleParameters.Flags.DEBRIS_TIMEOUT =          	getParamBool(params,"destructibleParameters.flags.DEBRIS_TIMEOUT");
			DestructibleParameters.Flags.DEBRIS_MAX_SEPARATION =   	getParamBool(params,"destructibleParameters.flags.DEBRIS_MAX_SEPARATION");
			DestructibleParameters.Flags.CRUMBLE_SMALLEST_CHUNKS = 	getParamBool(params,"destructibleParameters.flags.CRUMBLE_SMALLEST_CHUNKS");
			DestructibleParameters.Flags.ACCURATE_RAYCASTS =       	getParamBool(params,"destructibleParameters.flags.ACCURATE_RAYCASTS");
			DestructibleParameters.Flags.USE_VALID_BOUNDS =        	getParamBool(params,"destructibleParameters.flags.USE_VALID_BOUNDS");

   			getParamF32(params,"destructibleParameters.grbVolumeLimit",      DestructibleParameters.GrbVolumeLimit);
   			getParamF32(params,"destructibleParameters.grbParticleSpacing",  DestructibleParameters.GrbParticleSpacing);
			DestructibleParameters.GrbParticleSpacing *= P2UScale;
   			getParamF32(params,"destructibleParameters.fractureImpulseScale",DestructibleParameters.FractureImpulseScale);

			// Check for a sane maximum number of depth parameters; some older APEX assets may have more
			// sets of depth parameters than the actual number of fracture levels.
			physx::PxU32 NumFractureLevels = 256;
			{
				NxDestructibleAsset* ApexDestructibleAsset = NULL;
				NxApexAsset* ApexAsset = MApexAsset->GetNxApexAsset();
				ApexDestructibleAsset = static_cast<NxDestructibleAsset*>(ApexAsset);
				if( ApexDestructibleAsset != NULL )
				{
					NumFractureLevels = ApexDestructibleAsset->depthCount();
				}
			}

			for (physx::PxU32 i=0; i<NumFractureLevels; i++)
			{
				FNxDestructibleDepthParameters dp;
				char scratch[512];
				physx::string::sprintf_s(scratch,512,"depthParameters[%d].TAKE_IMPACT_DAMAGE",i);
#if NX_APEX_SDK_RELEASE >= 100
				::NxParameterized::Handle handle(*params);
#else
				::NxParameterized::Handle handle;
#endif
				if ( params->getParameterHandle(scratch,handle) != ::NxParameterized::ERROR_NONE ) break;
				dp.TAKE_IMPACT_DAMAGE = getParamBool(params,scratch);

				physx::string::sprintf_s(scratch,512,"depthParameters[%d].IGNORE_POSE_UPDATES",i);
				dp.IGNORE_POSE_UPDATES = getParamBool(params,scratch);

				physx::string::sprintf_s(scratch,512,"depthParameters[%d].IGNORE_RAYCAST_CALLBACKS",i);
				dp.IGNORE_RAYCAST_CALLBACKS = getParamBool(params,scratch);

				physx::string::sprintf_s(scratch,512,"depthParameters[%d].IGNORE_CONTACT_CALLBACKS",i);
				dp.IGNORE_CONTACT_CALLBACKS = getParamBool(params,scratch);

				physx::string::sprintf_s(scratch,512,"depthParameters[%d].USER_FLAG_0",i);
				dp.USER_FLAG_0 = getParamBool(params,scratch);

				physx::string::sprintf_s(scratch,512,"depthParameters[%d].USER_FLAG_1",i);
				dp.USER_FLAG_1 = getParamBool(params,scratch);

				physx::string::sprintf_s(scratch,512,"depthParameters[%d].USER_FLAG_2",i);
				dp.USER_FLAG_2 = getParamBool(params,scratch);

				physx::string::sprintf_s(scratch,512,"depthParameters[%d].USER_FLAG_3",i);
				dp.USER_FLAG_3 = getParamBool(params,scratch);

				DestructibleParameters.DepthParameters.AddItem(dp);
			}
		}

		// Allocate one FractureMaterial slot for each fracture level
		NxDestructibleAsset* ApexDestructibleAsset = static_cast<NxDestructibleAsset*>(MApexAsset->GetNxApexAsset());
		if( ApexDestructibleAsset != NULL )
		{
			FractureMaterials.Empty();
			FractureMaterials.AddZeroed( ApexDestructibleAsset->depthCount() );
		}

		// Give materials unique names inside of APEX asset so we can override materials in the actor
		SetUniqueAssetMaterialNames( this, MApexAsset );
		bHasUniqueAssetMaterialNames = TRUE;

		physx::PxMat44 mat = physx::PxMat44::getIdentity();
		if ( convertToUE3Coordinates )
		{
			physx::PxF32 *temp = (physx::PxF32 *)mat.front();
			temp[1*4+1] = -1;
		}

		MApexAsset->ApplyTransformation(mat,1,TRUE,TRUE,FALSE);
	}
	GApexCommands->AddUpdateMaterials(this);
#endif
	return TRUE;
}

void UApexDestructibleAsset::UpdateMaterials(void)
{
#if WITH_APEX
	Materials.Empty();
    if ( MApexAsset )
	{
		UINT NumSubmeshes = (UINT)MApexAsset->GetNumMaterials();
		Materials.Empty(NumSubmeshes);
		for(UINT i=0; i<NumSubmeshes; i++)
		{
			const char         *MaterialNameA = MApexAsset->GetMaterialName(i);
			LoadMaterialForApex(MaterialNameA, Materials);
		}
	}
#endif
}

void UApexDestructibleAsset::FixupAsset()
{
#if WITH_APEX
	if ( bHasUniqueAssetMaterialNames )
	{
		SetApexMatIndices(this);
	}

	if( GIsEditor && !GIsGame )
	{
		NxDestructibleAsset* ApexDestructibleAsset = NULL;
		if( MApexAsset )
		{
			NxApexAsset* ApexAsset = MApexAsset->GetNxApexAsset();
			ApexDestructibleAsset = static_cast<NxDestructibleAsset*>(ApexAsset);
		}

		if( ApexDestructibleAsset != NULL )
		{
			INT FractureDepth = static_cast<INT>(ApexDestructibleAsset->depthCount());
			// Fix size of FractureMaterials for old assets
			if( FractureDepth != FractureMaterials.Num() )
			{
				FractureMaterials.Empty();
				FractureMaterials.AddZeroed(FractureDepth);
				MarkPackageDirty();
			}

			// Fix size of DepthParameters for old assets
			if( FractureDepth < DestructibleParameters.DepthParameters.Num() )
			{
				INT NumItemsToRemove = DestructibleParameters.DepthParameters.Num() - FractureDepth;
				DestructibleParameters.DepthParameters.Remove(FractureDepth, NumItemsToRemove);
				MarkPackageDirty();
			}

			// Give materials unique names inside of APEX asset so we can override materials in the actor
			if ( !bHasUniqueAssetMaterialNames )
			{
				// TODO: Make sure this is safe when duplicating an asset
				SetUniqueAssetMaterialNames( this, MApexAsset );
				bHasUniqueAssetMaterialNames = TRUE;
			}
		}
	}
#endif
}

#if WITH_APEX
static const char * GetNameDestructible(const char *Src)
{
	if (Src == NULL )
	{
		Src = "";
	}
	INT Len = appStrlen(Src);
	char *Temp = (char *)appMalloc(Len+1);
	appMemcpy(Temp,Src,Len+1);
	return Temp;
}
#endif

void UApexDestructibleAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;
#if WITH_APEX
	if(PropertyThatChanged && PropertyThatChanged->GetFName() == FName(TEXT("Materials")))
	{
		OnApexAssetLost();
		INT Mcount = Materials.Num();
		if ( Mcount )
		{
			for(INT MatIdx=0; MatIdx<Mcount; ++MatIdx)
			{
				UMaterialInterface *Material = Materials(MatIdx);
				InitMaterialForApex(Material);
			}
		}
		if( MApexAsset )
		{
			// Give materials unique names inside of APEX asset so we can override materials in the actor
			SetUniqueAssetMaterialNames( this, MApexAsset );
			bHasUniqueAssetMaterialNames = TRUE;
		}
	}
	if ( PropertyThatChanged )
	{
		// validate that certain property change events are within a valid numeric range.
		if (PropertyThatChanged->GetFName() == FName(TEXT("DamageThreshold")))
		{
			DestructibleParameters.DamageThreshold = Max( DestructibleParameters.DamageThreshold, 0.0f );
		}
		else if(PropertyThatChanged->GetFName() == FName(TEXT("DamageToRadius")))
		{
			DestructibleParameters.DamageToRadius = Max( DestructibleParameters.DamageToRadius, 0.0f );
		}
		else if(PropertyThatChanged->GetFName() == FName(TEXT("DamageCap")))
		{
			DestructibleParameters.DamageCap = Max( DestructibleParameters.DamageCap, 0.0f );
		}
		else if(PropertyThatChanged->GetFName() == FName(TEXT("ForceToDamage")))
		{
			DestructibleParameters.ForceToDamage = Max( DestructibleParameters.ForceToDamage, 0.0f );
		}
		else if(PropertyThatChanged->GetFName() == FName(TEXT("DamageToPercentDeformation")))
		{
			DestructibleParameters.DamageToPercentDeformation = Max( DestructibleParameters.DamageToPercentDeformation, 0.0f );
		}
		else if(PropertyThatChanged->GetFName() == FName(TEXT("DeformationPercentLimit")))
		{
			DestructibleParameters.DeformationPercentLimit = Max( DestructibleParameters.DeformationPercentLimit, 0.0f );
		}
		else if(PropertyThatChanged->GetFName() == FName(TEXT("SupportDepth")))
		{
			DestructibleParameters.SupportDepth = Clamp( DestructibleParameters.SupportDepth, 0, DestructibleParameters.DepthParameters.Num()-1 );
		}
		else if(PropertyThatChanged->GetFName() == FName(TEXT("DebrisDepth")))
		{
			DestructibleParameters.DebrisDepth = Clamp( DestructibleParameters.DebrisDepth, -1, DestructibleParameters.DepthParameters.Num()-1 );
		}
		else if(PropertyThatChanged->GetFName() == FName(TEXT("DebrisLifetimeMin")))
		{
			DestructibleParameters.DebrisLifetimeMin = Clamp( DestructibleParameters.DebrisLifetimeMin, 0.0f, DestructibleParameters.DebrisLifetimeMax );
		}
		else if(PropertyThatChanged->GetFName() == FName(TEXT("DebrisLifetimeMax")))
		{
			DestructibleParameters.DebrisLifetimeMax = Max( DestructibleParameters.DebrisLifetimeMax, DestructibleParameters.DebrisLifetimeMin );
		}
		else if(PropertyThatChanged->GetFName() == FName(TEXT("DebrisMaxSeparationMin")))
		{
			DestructibleParameters.DebrisMaxSeparationMin = Clamp( DestructibleParameters.DebrisMaxSeparationMin, 0.0f, DestructibleParameters.DebrisMaxSeparationMax );
		}
		else if(PropertyThatChanged->GetFName() == FName(TEXT("DebrisMaxSeparationMax")))
		{
			DestructibleParameters.DebrisMaxSeparationMax = Max( DestructibleParameters.DebrisMaxSeparationMax, DestructibleParameters.DebrisMaxSeparationMin );
		}
		else if (PropertyThatChanged->GetFName() == FName(TEXT("GrbVolumeLimit")))
		{
			DestructibleParameters.GrbVolumeLimit = Clamp( DestructibleParameters.GrbVolumeLimit, 0.0f, 1.0f );
		}
		else if (PropertyThatChanged->GetFName() == FName(TEXT("MaxChunkSpeed")))
		{
			DestructibleParameters.MaxChunkSpeed = Max( DestructibleParameters.MaxChunkSpeed, 0.0f );
		}
		else if (PropertyThatChanged->GetFName() == FName(TEXT("MassScaleExponent")))
		{
			DestructibleParameters.MassScaleExponent = Clamp( DestructibleParameters.MassScaleExponent, 0.0f, 1.0f );
		}
		else if (PropertyThatChanged->GetFName() == FName(TEXT("EssentialDepth")))
		{
			DestructibleParameters.EssentialDepth = Clamp( DestructibleParameters.EssentialDepth, 0, 5 );
		}
	}

#endif

}

/** Fix up unique material names in the APEX asset after rename */
void UApexDestructibleAsset::PostRename()
{
	Super::PostRename();
#if WITH_EDITOR && WITH_APEX
	OnApexAssetLost();
	SetUniqueAssetMaterialNames( this, MApexAsset );
	bHasUniqueAssetMaterialNames = TRUE;
#endif
}

/** Fix up unique material names in the APEX asset after duplication */
void UApexDestructibleAsset::PostDuplicate()
{
	Super::PostDuplicate();
#if WITH_EDITOR && WITH_APEX
	SetUniqueAssetMaterialNames( this, MApexAsset );
	bHasUniqueAssetMaterialNames = TRUE;
#endif
}

void UApexDestructibleAsset::BeginDestroy(void)
{
	Super::BeginDestroy();
#if WITH_APEX
	if ( GApexCommands )
	{
		GApexCommands->NotifyDestroy(this);
	}
	// BRG - commenting this check out (along with check in UApexDestructibleAsset::ReleaseDestructiblePreview)
	// seems to allow asset re-import without any side-effects.  This should be examined.
//	check(ApexComponents.Num() == 0);
	OnApexAssetLost();
	if ( MApexAsset )
	{
		MApexAsset->DecRefCount(0);
		MApexAsset = NULL;
	}
#endif
}

void UApexDestructibleAsset::ReleaseDestructibleActor(physx::apex::NxDestructibleActor &ApexDestructibleActor, UApexComponentBase &Component)
{
#if WITH_APEX 
	INT Index = 0;
	UBOOL Found = ApexComponents.FindItem(&Component, Index);
	check(Found);
	if(Found)
	{
		ApexComponents.Remove(Index);
	}
	ApexDestructibleActor.release();
#endif
}

physx::apex::NxDestructiblePreview *UApexDestructibleAsset::CreateDestructiblePreview(UApexComponentBase &Component)
{
	physx::apex::NxDestructiblePreview *ApexPreview = NULL;
#if WITH_APEX
	check(MApexAsset);
	if( MApexAsset )
	{
		NxApexAsset *a_asset = MApexAsset->GetNxApexAsset();
		if ( a_asset )
		{
			NxParameterized::Interface *descParams = a_asset->getDefaultAssetPreviewDesc();
			ApexPreview = (physx::NxDestructiblePreview *)a_asset->createApexAssetPreview(*descParams);
			check(ApexPreview);
		}
	}
	if(ApexPreview)
	{
		ApexComponents.AddItem(&Component);
	}
#endif
	return ApexPreview;
}	

void UApexDestructibleAsset::ReleaseDestructiblePreview(class physx::apex::NxDestructiblePreview &ApexDestructiblePreview, class UApexComponentBase &Component)
{
#if WITH_APEX 
	INT Index = 0;
	UBOOL Found = ApexComponents.FindItem(&Component, Index);
	// BRG - commenting this check out (along with check in UApexDestructibleAsset::BeginDestroy)
	// seems to allow asset re-import without any side-effects.  This should be examined.
//	check(Found);
	if(Found)
	{
		MDestructibleThumbnailComponent = NULL;
		ApexComponents.Remove(Index);
	}
	ApexDestructiblePreview.release();
#endif
}


void * UApexDestructibleAsset::GetNxParameterized(void)
{
	void *ret = NULL;
#if WITH_APEX
	if ( MApexAsset )
	{
		::NxParameterized::Interface *params = MApexAsset->GetDefaultApexActorDesc();
		if ( params )
		{
			NxParameterized::setParamF32(*params,"destructibleParameters.damageThreshold",				DestructibleParameters.DamageThreshold);
			NxParameterized::setParamF32(*params,"destructibleParameters.damageToRadius",				DestructibleParameters.DamageToRadius);
			NxParameterized::setParamF32(*params,"destructibleParameters.damageCap",					DestructibleParameters.DamageCap);
			NxParameterized::setParamF32(*params,"destructibleParameters.forceToDamage",				DestructibleParameters.ForceToDamage*U2PScale);
			NxParameterized::setParamF32(*params,"destructibleParameters.impactVelocityThreshold",		DestructibleParameters.ImpactVelocityThreshold*U2PScale);
			NxParameterized::setParamF32(*params,"destructibleParameters.materialStrength",				DestructibleParameters.MaterialStrength*U2PScale);
			NxParameterized::setParamF32(*params,"destructibleParameters.damageToPercentDeformation",	DestructibleParameters.DamageToPercentDeformation);
			NxParameterized::setParamF32(*params,"destructibleParameters.deformationPercentLimit",		DestructibleParameters.DeformationPercentLimit);
			NxParameterized::setParamU32(*params,"destructibleParameters.formExtendedStructures",		DestructibleParameters.bFormExtendedStructures ? 1 : 0);
			NxParameterized::setParamU32(*params,"destructibleParameters.supportDepth",				DestructibleParameters.SupportDepth);
			NxParameterized::setParamI32(*params,"destructibleParameters.debrisDepth",				DestructibleParameters.DebrisDepth);
			NxParameterized::setParamU32(*params,"destructibleParameters.essentialDepth",				DestructibleParameters.EssentialDepth);
			NxParameterized::setParamF32(*params,"destructibleParameters.debrisLifetimeMin",			DestructibleParameters.DebrisLifetimeMin);
			NxParameterized::setParamF32(*params,"destructibleParameters.debrisLifetimeMax",			DestructibleParameters.DebrisLifetimeMax);
			NxParameterized::setParamF32(*params,"destructibleParameters.debrisMaxSeparationMin",		DestructibleParameters.DebrisMaxSeparationMin);
			NxParameterized::setParamF32(*params,"destructibleParameters.debrisMaxSeparationMax",		DestructibleParameters.DebrisMaxSeparationMax);

			physx::PxBounds3 b;

			if( DestructibleParameters.ValidBounds.IsValid )
			{
				b.minimum.x = DestructibleParameters.ValidBounds.Min.X * U2PScale;
				b.minimum.y = DestructibleParameters.ValidBounds.Min.Y * U2PScale;
				b.minimum.z = DestructibleParameters.ValidBounds.Min.Z * U2PScale;
																	   
				b.maximum.x = DestructibleParameters.ValidBounds.Max.X * U2PScale;
				b.maximum.y = DestructibleParameters.ValidBounds.Max.Y * U2PScale;
				b.maximum.z = DestructibleParameters.ValidBounds.Max.Z * U2PScale;
			}
			else
			{
				b.setEmpty();
			}

			NxParameterized::setParamBounds3(*params,"destructibleParameters.validBounds",b);

			NxParameterized::setParamF32(*params,"destructibleParameters.maxChunkSpeed",					DestructibleParameters.MaxChunkSpeed*U2PScale);
			NxParameterized::setParamF32(*params,"destructibleParameters.massScaleExponent",				DestructibleParameters.MassScaleExponent);

			NxParameterized::setParamBool(*params,"destructibleParameters.flags.ACCUMULATE_DAMAGE",        DestructibleParameters.Flags.ACCUMULATE_DAMAGE);
			NxParameterized::setParamBool(*params,"destructibleParameters.flags.ASSET_DEFINED_SUPPORT",    DestructibleParameters.Flags.ASSET_DEFINED_SUPPORT);
			NxParameterized::setParamBool(*params,"destructibleParameters.flags.WORLD_SUPPORT",            DestructibleParameters.Flags.WORLD_SUPPORT);
			NxParameterized::setParamBool(*params,"destructibleParameters.flags.DEBRIS_TIMEOUT",           DestructibleParameters.Flags.DEBRIS_TIMEOUT);
			NxParameterized::setParamBool(*params,"destructibleParameters.flags.DEBRIS_MAX_SEPARATION",    DestructibleParameters.Flags.DEBRIS_MAX_SEPARATION);
			NxParameterized::setParamBool(*params,"destructibleParameters.flags.CRUMBLE_SMALLEST_CHUNKS",  DestructibleParameters.Flags.CRUMBLE_SMALLEST_CHUNKS);
			NxParameterized::setParamBool(*params,"destructibleParameters.flags.ACCURATE_RAYCASTS",        DestructibleParameters.Flags.ACCURATE_RAYCASTS);
			NxParameterized::setParamBool(*params,"destructibleParameters.flags.USE_VALID_BOUNDS",         DestructibleParameters.Flags.USE_VALID_BOUNDS);

			NxParameterized::setParamF32(*params,"destructibleParameters.grbVolumeLimit",      DestructibleParameters.GrbVolumeLimit);
   			NxParameterized::setParamF32(*params,"destructibleParameters.grbParticleSpacing",  DestructibleParameters.GrbParticleSpacing*U2PScale);
   			NxParameterized::setParamF32(*params,"destructibleParameters.fractureImpulseScale",DestructibleParameters.FractureImpulseScale);


			for (INT i=0; i<DestructibleParameters.DepthParameters.Num(); i++)
			{
				const FNxDestructibleDepthParameters dp = DestructibleParameters.DepthParameters(i);

				char scratch[512];
				physx::string::sprintf_s(scratch,512,"depthParameters[%d].TAKE_IMPACT_DAMAGE",i);
				NxParameterized::setParamBool(*params,scratch,dp.TAKE_IMPACT_DAMAGE);

				physx::string::sprintf_s(scratch,512,"depthParameters[%d].IGNORE_POSE_UPDATES",i);
				NxParameterized::setParamBool(*params,scratch,dp.IGNORE_POSE_UPDATES);

				physx::string::sprintf_s(scratch,512,"depthParameters[%d].IGNORE_RAYCAST_CALLBACKS",i);
				NxParameterized::setParamBool(*params,scratch,dp.IGNORE_RAYCAST_CALLBACKS);

				physx::string::sprintf_s(scratch,512,"depthParameters[%d].IGNORE_CONTACT_CALLBACKS",i);
				NxParameterized::setParamBool(*params,scratch,dp.IGNORE_CONTACT_CALLBACKS);

				physx::string::sprintf_s(scratch,512,"depthParameters[%d].USER_FLAG_0",i);
				NxParameterized::setParamBool(*params,scratch,dp.USER_FLAG_0);

				physx::string::sprintf_s(scratch,512,"depthParameters[%d].USER_FLAG_1",i);
				NxParameterized::setParamBool(*params,scratch,dp.USER_FLAG_1);

				physx::string::sprintf_s(scratch,512,"depthParameters[%d].USER_FLAG_2",i);
				NxParameterized::setParamBool(*params,scratch,dp.USER_FLAG_2);

				physx::string::sprintf_s(scratch,512,"depthParameters[%d].USER_FLAG_3",i);
				NxParameterized::setParamBool(*params,scratch,dp.USER_FLAG_3);


			}
		}

		ret = params;
	}
#endif
	return ret;
}

