/*=============================================================================
	NvApexResourceCallback.cpp : Manages resource requests from APEX
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

#if WITH_APEX


#include "NvApexResourceCallback.h"
#include "EnginePhysicsClasses.h"
#include "EngineSequenceClasses.h"
#include "EngineParticleClasses.h"
#include "EngineClasses.h"
#include "EngineMeshClasses.h"

#include "UnNovodexSupport.h"
#include <NxApexSDK.h>
#include <NxResourceCallback.h>
#include <NxUserRenderResourceManager.h>
#include <NxModuleDestructible.h>
#include <NxModuleClothing.h>
#include <NxClothingMaterialLibrary.h>
#include <NxModuleParticles.h>
#include <NxRendermeshAsset.h>
#include <NxDestructibleAsset.h>
#include <NxClothingAsset.h>
#include <NxApexEmitterAsset.h>
#include <NxGroundEmitterAsset.h>
#include <NxImpactEmitterAsset.h>
#include <NxFluidIosAsset.h>
#include <NxBasicIosAsset.h>
#include <NxIofxAsset.h>
#include "NvApexScene.h"
#include <NxActorDesc.h>
#include <NxStream.h>
#include <stdarg.h>
#include <NxSerializer.h>
#include <NxUserOpaqueMesh.h>
#include "NvApexCommands.h"

using namespace physx::apex;

// Can safely set this to 0 if you have no pre-QA_APPROVED_BUILD_OCT_2010 APEX clothing assets you need to convert
#define SUPPORT_APEX_0_9_CLOTHING_ASSETS	1

/**
 * Helper function to find an APEX asset from the object containing it
**/
template< class T >
static void* FindApexAsset( const FString UnrealName )
{
	UBOOL               bIsValidName = IsValidObjectPathName(UnrealName);

	if( !IsInGameThread() )
	{
		debugf( NAME_DevPhysics, TEXT("NvApexResourceCallback: Attempting to load resource %s in a thread other than the game thread.  Returning NULL."), *UnrealName );
	}
	else
	{
		T* ObjectToLoad = NULL;
		if(bIsValidName)
		{
			ObjectToLoad = FindObject<T>(0, *UnrealName);
			if ( ObjectToLoad && ObjectToLoad->MApexAsset )
			{
				return ObjectToLoad->MApexAsset->GetNxApexAsset();
			}
		}
	}

	return NULL;
}

#if SUPPORT_APEX_0_9_CLOTHING_ASSETS
// Specialization for loading APEX 0.9 ACMLs stored in UApexGenericAsset containers
template<>
static void* FindApexAsset<UApexGenericAsset>( const FString UnrealName )
{
	UBOOL               bIsValidName = IsValidObjectPathName(UnrealName);

	if( !IsInGameThread() )
	{
		debugf( NAME_DevPhysics, TEXT("NvApexResourceCallback: Attempting to load resource %s in a thread other than the game thread.  Returning NULL."), *UnrealName );
	}
	else
	{
		UApexGenericAsset* ObjectToLoad = NULL;
		if(bIsValidName)
		{
			ObjectToLoad = FindObject<UApexGenericAsset>(0, *UnrealName);
			if ( GIsEditor && !GIsGame )
			{
				if( ObjectToLoad == NULL )
				{
					// Attempt to dynamically load the object since it isn't already loaded
					ObjectToLoad = LoadObject<UApexGenericAsset>(NULL, *UnrealName, NULL, LOAD_None, NULL);
				}
				if ( ObjectToLoad && (ObjectToLoad->MApexAsset == NULL) )
				{
					// We may have arrived here because an APEX 0.9 clothing asset is trying to get the
					// clothing material data stored in a separate APEX generic asset. APEX 1.0 needs that
					// data up front to convert it to the new APEX 1.0 clothing asset format. The clothing
					// asset will notice it's been updated in PostLoad and mark the package dirty so we never 
					// have to do this again.
					if( ObjectToLoad->HasAnyFlags(RF_NeedLoad) )
					{
						check(ObjectToLoad->GetLinker());
						ObjectToLoad->GetLinker()->Preload( ObjectToLoad );
					}
				}
			}
			if ( ObjectToLoad && ObjectToLoad->MApexAsset )
			{
				return ObjectToLoad->MApexAsset->GetNxApexAsset();
			}
		}
	}

	return NULL;
}
#endif	// SUPPORT_APEX_0_9_CLOTHING_ASSETS


namespace NI_APEX_RESOURCE_CALLBACK
{

class NvApexResourceCallback : public NxResourceCallback
{
public:
	NvApexResourceCallback(void)
	{
	}

	/**
	Will be called by the ApexSDK if a named resource is required but has not yet been provided.
	The resource pointer is returned directly, NxResourceProvider::setResource() should not be called.
	This function will be called at most once per named resource, unless an intermediate call to
	releaseResource() has been made.
	*/
    virtual void *requestResource( const char *pcNameSpace, const char *pcName )
    {
		FIApexManager *manager = GApexCommands ? GApexCommands->GetApexManager() : 0;
		if ( !manager ) return 0;
    	NxApexSDK* pkApexSDK = manager->GetApexSDK();
		if ( !pkApexSDK ) return 0;

        void *ret = NULL;

		if (  strcmp(pcNameSpace,NX_DESTRUCTIBLE_AUTHORING_TYPE_NAME) == 0 )
		{
			ret = FindApexAsset<UApexDestructibleAsset>(ANSI_TO_TCHAR(pcName));
		}
		else if (  strcmp(pcNameSpace,NX_CLOTHING_AUTHORING_TYPE_NAME) == 0 )
		{
			ret = FindApexAsset<UApexClothingAsset>(ANSI_TO_TCHAR(pcName));
		}
		else if (  strcmp(pcNameSpace,NX_CLOTHING_MATERIAL_AUTHORING_TYPE_NAME) == 0 )
		{
			ret = FindApexAsset<UApexGenericAsset>(ANSI_TO_TCHAR(pcName));
		}
		else if ( strcmp(pcNameSpace,APEX_MATERIALS_NAME_SPACE) == 0 )
		{
			if( !IsInGameThread() )
			{
				debugf( NAME_DevPhysics, TEXT("NvApexResourceCallback: Attempting to load resource %s in a thread other than the game thread.  Returning NULL."), ANSI_TO_TCHAR(pcName) );
			}
			else
			{
   				UMaterialInterface *MaterialInterface = FindMaterialForApex(pcName);
#if 0
   				// If we couldn't load the material, or its not set to work on apex meshes, then load the default material...
				if(!MaterialInterface || !MaterialInterface->CheckMaterialUsage(MATUSAGE_APEXMesh))
   				{
   					MaterialInterface = GEngine->DefaultMaterial;
   				}
#endif
				ret = MaterialInterface;
			}
       	}
		else if ( strcmp(pcNameSpace,APEX_OPAQUE_MESH_NAME_SPACE) == 0 )
		{
			UStaticMesh *Mesh = (UStaticMesh*)UObject::StaticLoadObject(UStaticMesh::StaticClass(),NULL,TEXT("EngineMeshes.ParticleCube"),NULL,LOAD_None,NULL);
			ret = Mesh;
		}
		else if ( strcmp(pcNameSpace,APEX_COLLISION_GROUP_NAME_SPACE) == 0 )
		{
			if ( strcmp(pcName,"PhysXFluidCollisionGroup") == 0 )
			{
				ret = (void *)UNX_GROUP_DEFAULT;
			}
			else
			{
				PX_ALWAYS_ASSERT();
			}
		}
		else if ( strcmp(pcNameSpace,APEX_COLLISION_GROUP_MASK_NAME_SPACE) == 0 )
		{
			PX_ALWAYS_ASSERT();
		}
		else if ( strcmp(pcNameSpace,APEX_COLLISION_GROUP_128_NAME_SPACE) == 0 )
		{
			if ( strcmp(pcName,"PhysXFluidCollisionGroup") == 0 )
			{
				FRBCollisionChannelContainer c;
				c.Bitfield = 0;
				c.Default = 1;
				c.GameplayPhysics = 1;
				c.FluidDrain = 1;
				static NxGroupsMask mask = CreateGroupsMask(UNX_GROUP_DEFAULT,&c);
				ret = &mask;
			}
			else
			{
				PX_ALWAYS_ASSERT();
			}
		}
		else if( strcmp(pcNameSpace,NX_APEX_EMITTER_AUTHORING_TYPE_NAME) == 0  ||
				 strcmp(pcNameSpace,NX_GROUND_EMITTER_AUTHORING_TYPE_NAME) == 0 ||
				 strcmp(pcNameSpace,NX_IMPACT_EMITTER_AUTHORING_TYPE_NAME) == 0 ||
				 strcmp(pcNameSpace,NX_BASIC_IOS_AUTHORING_TYPE_NAME) == 0 ||
				 strcmp(pcNameSpace,NX_FLUID_IOS_AUTHORING_TYPE_NAME) == 0 ||
				 strcmp(pcNameSpace,NX_IOFX_AUTHORING_TYPE_NAME) == 0 ||
				 strcmp(pcNameSpace,NX_RENDER_MESH_AUTHORING_TYPE_NAME) == 0 ||
				 strcmp(pcNameSpace,APEX_AUTHORABLE_ASSETS_TYPES_NAME_SPACE) == 0 ||
				 strcmp(pcNameSpace,APEX_PHYSICS_MATERIAL_NAME_SPACE) == 0 )
		{
			FIApexAsset *asset = GApexCommands->GetApexAsset(pcName);
			if ( asset )
			{
				ret = asset->GetNxApexAsset();
			}
			else
			{
#if WITH_APEX_PARTICLES
				if ( strcmp(pcNameSpace,"NxBasicIosAsset") == 0 && strcmp(pcName,"DefaultBasicIOS") == 0  )
				{
					asset = GApexCommands->CreateDefaultApexAsset(pcName,AAT_BASIC_IOS,NULL,NULL);
					PX_ASSERT(asset);
					if ( asset )
					{
						ret = asset->GetNxApexAsset();
					}
				}
				else if ( strcmp(pcNameSpace,"NxFluidIosAsset") == 0 && strcmp(pcName,"DefaultFluidIOS") == 0  )
				{
					asset = GApexCommands->CreateDefaultApexAsset(pcName,AAT_FLUID_IOS,NULL,NULL);
					PX_ASSERT(asset);
					if ( asset )
					{
						ret = asset->GetNxApexAsset();
					}
				}

#endif
			}
			if (ret == NULL )
			{
				debugf( NAME_DevPhysics, TEXT("NvApexResourceCallback: Unable to located Apex resource %s!"), ANSI_TO_TCHAR(pcName) );
			}
		}
		else if( (strcmp(pcNameSpace,APEX_CUSTOM_VB_NAME_SPACE) == 0) && (strcmp(pcName, "DCC_INDEX") == 0) )
		{
			// Intentionally ignored. It's a custom vertex buffer semantic which tells which index
			// the vertex had in the DCC tools.
		}
        else
        {
			debugf( NAME_DevPhysics, TEXT("NvApexResourceCallback: Unknown resource type %s!"), ANSI_TO_TCHAR(pcNameSpace) );
			check(0);
        }
   		return ret;
    }

	/**
	Will be called by the ApexSDK when all internal references to a named resource have been released.
	If this named resource is required again in the future, a new call to requestResource() will be made.
	*/
    //---------------------------------------------------------------------------
    virtual void releaseResource( const char *pcNameSpace, const char *pcName, void *pvResource )
    {
    }

private:
};

}; // end of namespace

using namespace NI_APEX_RESOURCE_CALLBACK;


/**
 * Creates a resource callback function
 * @return	null if it fails, else returns a pointer to the resource callback. 
**/
NxResourceCallback * CreateResourceCallback(void)
{
	NvApexResourceCallback *nv = new NvApexResourceCallback;
	return static_cast< NxResourceCallback *>(nv);
}
/**
 * Release the resource callback function
 * @param [in,out]	Callback	The resource callback. 
**/
void ReleaseResourceCallback(NxResourceCallback *Callback)
{
	NvApexResourceCallback *nv = static_cast< NvApexResourceCallback *>(Callback);
	delete nv;
}

#endif

#endif
