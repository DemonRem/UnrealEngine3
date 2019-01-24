/*=============================================================================
	NvApexScene.cpp : Implements the FIApexClothing interface and various utility methods.
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

#if WITH_APEX

#include "UnNovodexSupport.h"
#include "EngineMeshClasses.h"
#include "NvApexScene.h"
#include "NvApexManager.h"
#include "NvApexCommands.h"
#if WITH_APEX_PARTICLES
#include "NxApexEmitterActor.h"
#endif
#include <NxArray.h>
#include <NxActorDesc.h>
#include <NxApexAsset.h>
#include <NxDestructibleAsset.h>
#include <NxDestructibleActor.h>
#include <NxClothingAsset.h>
#include <NxClothingActor.h>
#include <NxApexSDK.h>
#include <NxParameterized.h>
#include <PxQuat.h>
#include <NxScene.h>
#include <NxApexRenderVolume.h>
#include "NvApexRender.h"
#include <PxCudaContextManager.h>
#include <PxTaskManager.h>
#include "EngineAnimClasses.h"

#define USE_UE3_THREADPOOL (0)
#if USE_UE3_THREADPOOL
#include <PxTask.h>
#endif


using namespace physx::apex;
#define FAKE_LAG 0 // introduces extreme fake lag into the system to help debug/identify physics lagging problems.  Off by default of course.

#if FAKE_LAG
#include <windows.h>
#endif


physx::PxU32	   GActiveApexSceneCount=0;

namespace APEX_SCENE
{

	const physx::PxF32 MIN_BENEFIT=0.001f;

class ApexScene;

class ApexActor : public FIApexActor
{
public:

  ApexActor(ApexScene *parentScene,
   			FIApexAsset *apexAsset,
   			const ::NxParameterized::Interface *params);

  ~ApexActor(void)
  {
	if ( MActor )
	{
		MActor->release();
		if ( MApexAsset )
		{
			MApexAsset->DecRefCount(this);
		}
	}
  }

  UBOOL Recreate(ApexScene *parentScene,const ::NxParameterized::Interface *params);

  virtual void *              GetUserData(void)
  {
    void *ret = NULL;
    if ( MActor )
    {
      ret = MActor->userData;
    }
    return ret;
  }

  virtual void Release(void);

  UBOOL IsOk(void) const
  {
	  return MActor ? TRUE : FALSE;
  }

  virtual void	UpdateRenderResources(void)
  {

	  if ( MActor && GApexCommands->IsShowApex() )
	  {
  		  switch ( MApexAsset->GetType() )
		  {
			case AAT_DESTRUCTIBLE:
			  {
        		NxDestructibleActor *da = static_cast< NxDestructibleActor *>(MActor);
				da->lockRenderResources();
				da->updateRenderResources();
				da->unlockRenderResources();
			  }
			  break;
			case AAT_CLOTHING:
			  {
				NxClothingActor *da = static_cast< NxClothingActor *>(MActor);

				//GApexNormalSign = -1;
				//GReverseCulling = FALSE;
				da->lockRenderResources();
				da->updateRenderResources();
				da->unlockRenderResources();
				//GApexNormalSign = 1;
				//GReverseCulling = TRUE;
			  }
			  break;
		  }

	  }
  }

  virtual void Render(NxUserRenderer &renderer)
  {
	  if ( MActor && GApexCommands->IsShowApex() )
	  {
	  	  GApexRender->ProcessFEnqueDataRenderingThread(); // process any deferred render resource creation requests...
		  switch ( MApexAsset->GetType() )
		  {
			case AAT_DESTRUCTIBLE:
			  {
        		NxDestructibleActor *da = static_cast< NxDestructibleActor *>(MActor);
        		da->dispatchRenderResources(renderer);
			  }
			  break;
			case AAT_CLOTHING:
			  {
				NxClothingActor *da = static_cast< NxClothingActor *>(MActor);
        		da->dispatchRenderResources(renderer);
			  }
			  break;
		  }
	  }
  }

  FIApexAsset * GetApexAsset(void) const { return MApexAsset; };

  virtual NxApexActor * GetNxApexActor(void) const { return MActor; };

  virtual ApexActorType::Enum GetApexActorType(void) const
  {
    return MType;
  }

  virtual void				  NotifyAssetGone(void) // notify the actor that the asset which is was created with is now gone.
  {
  	if ( MActor )
  	{
  		MActor->release();
  		MActor = NULL;
  	}
  }

  virtual void				  NotifyAssetImport(void) // notify the actor that a fresh asset has been imported and the actor can be re-created.
  {
  }

//private:
	ApexActorType::Enum     MType;
	NxApexActor      		*MActor;
	FIApexAsset      		*MApexAsset;
	ApexScene        		*MParent;
};

class ApexClothing;

static UBOOL GetParam(::NxParameterized::Interface *pm,const char *name,physx::PxF32 &value)
{
	if ( !pm ) return FALSE;
	UBOOL ret = TRUE;
#if NX_APEX_SDK_RELEASE >= 100
	::NxParameterized::Handle handle(*pm);
#else
	::NxParameterized::Handle handle;
#endif
	if ( pm->getParameterHandle(name,handle) == ::NxParameterized::ERROR_NONE )
	{
#if NX_APEX_SDK_RELEASE >= 100
		handle.getParamF32(value);
#else
		pm->getParamF32(handle,value);
#endif
	}
	else
	{
		ret = FALSE;
	}

	return ret;
}

struct BaseBoneRef
{
	BaseBoneRef(void)
	{
		MTransform = physx::PxMat44::getIdentity();
		MBaseIndex = 0;
	}
	physx::PxMat44			MTransform;
	physx::PxU32			MBaseIndex;
};

typedef NxArray< BaseBoneRef > TBaseBoneRefArray;
typedef NxArray< physx::PxU32 > TPxU32Array;

class ApexClothingPiece : public FIApexClothingPiece
{
public:
	ApexClothingPiece(FIApexAsset *asset,ApexClothing *parent,::NxParameterized::Interface *np,physx::PxU32 materialIndex, USkeletalMeshComponent* skeletalMeshComp)
	{
		MNewFrame = 0;
		MMaterialIndex = materialIndex;
		MSimulateCount = 0;
		MAsset		= asset;
		MParent     = parent;
		MSkeletalMeshComponent = skeletalMeshComp;
		MActor  	= NULL;
		MReady		= FALSE;
		MWasVisible = FALSE;
		MParameterized = NULL;
		MContinuousRotationThreshold = 0.1f;
		mContinousDistanceThreshold = 1.0f;
		MRootPose = physx::PxMat44::getIdentity();
		MGraphicalLODNum = 0;
		bForcedHidden = FALSE;
		if ( np )
		{
			NxApexSDK *sdk = GApexManager->GetApexSDK();
			::NxParameterized::Traits *traits = sdk->getParameterizedTraits();
			if ( traits )
			{
				MParameterized = traits->createNxParameterized( np->className() );
				if ( MParameterized )
				{
					MParameterized->copy(*np);


					UApexAsset *ue = asset->GetUE3Object();
					UApexClothingAsset *cue = static_cast< UApexClothingAsset *>(ue);
					MContinuousRotationThreshold = cosf(NxMath::degToRad(cue->ContinuousRotationThreshold));
					mContinousDistanceThreshold = cue->ContinuousDistanceThreshold*U2PScale;

        			GetParam(MParameterized,"lodWeights.maxDistance",MLodWeightsMaxDistance);
        			GetParam(MParameterized,"lodWeights.distanceWeight",MLodWeightsDistanceWeight);
        			GetParam(MParameterized,"lodWeights.bias",MLodWeightsBias);
        			GetParam(MParameterized,"lodWeights.benefitsBias",MLodWeightsBenefitsBias);
					NxParameterized::Handle handle(*MParameterized);
					handle.getParameter("graphicalLods");
					handle.getArraySize(MGraphicalLODNum);

					MTeleportationMode = (cue->bResetAfterTeleport)? NxClothingActor::TM_TeleportAndReset : NxClothingActor::TM_Teleport;
				}
			}
		}
		bIsFirstFrame = TRUE;
		mContinousDistanceThreshold*=mContinousDistanceThreshold;
		MPrevLOD = 0.0f;
		MCurrGraphicalLOD = 0;
		MGraphicalLODAvailable = TRUE;
	}

	~ApexClothingPiece(void);

	virtual FIApexAsset	* GetApexAsset(void)
	{
		return MAsset;
	}

	void SyncPieceTransforms(const physx::PxMat44 *transforms,UBOOL previouslyVisible, const physx::PxMat44& LocalToWorld, UBOOL bForceTeleportAndReset, UBOOL bForceTeleport);
	void UpdateRenderResources(void);

	void SetNonVisible(void)
	{
		if ( MWasVisible )
		{
			//debugf(TEXT("CLOTHING NO LONGER VISIBLE"));
			MWasVisible = FALSE;
			NxClothingActor *Act = static_cast< NxClothingActor *>(MActor->GetNxApexActor());
			if ( Act )
			{
				NxParameterized::Interface* ActorDesc = Act->getActorDesc();
				NxParameterized::setParamF32(*ActorDesc, "lodWeights.bias", 0);
				if( !bForcedHidden )
				{
					Act->setVisible(false);
				}

				if (MSkeletalMeshComponent->bAutoFreezeApexClothingWhenNotRendered)
				{
					Act->setFrozen(true);
				}
			}
		}
	}

	void SetVisible(UBOOL bEnable)
	{
		bForcedHidden = !bEnable;
		if (MActor)
		{
			NxClothingActor *Act = static_cast< NxClothingActor *>(MActor->GetNxApexActor());
			if ( Act )
			{
				Act->setVisible(bEnable ? true : false);
			}
		}
	}

	void Pump(void);

	UBOOL IsReady(const physx::PxF32 *eyePos);

	virtual void Render(NxUserRenderer &renderer);

	virtual void RefreshApexClothingPiece(FIApexAsset * /*asset*/,::NxParameterized::Interface *np)
	{
		NxApexSDK *sdk = GApexManager->GetApexSDK();
		::NxParameterized::Traits *traits = sdk->getParameterizedTraits();
		if ( traits )
		{
			MParameterized = traits->createNxParameterized( np->className() );
			if ( MParameterized )
			{
				MParameterized->copy(*np);
       			GetParam(MParameterized,"lodWeights.maxDistance",MLodWeightsMaxDistance);
       			GetParam(MParameterized,"lodWeights.distanceWeight",MLodWeightsDistanceWeight);
       			GetParam(MParameterized,"lodWeights.bias",MLodWeightsBias);
       			GetParam(MParameterized,"lodWeights.benefitsBias",MLodWeightsBenefitsBias);
			}
		}
	}

	void BuildBoneMapping(void);
	void InitGraphicalLOD(void);
	void SetGraphicalLOD(physx::pubfnd2::PxU32 lod);
	void NotifySceneGone(void);

	virtual UBOOL UsesMaterialIndex(physx::PxU32 materialIndex) const
	{
		return materialIndex == MMaterialIndex ? TRUE : FALSE;
	}

	void DisableSimulateThisFrame(void)
	{
		if ( MActor )
		{
    		NxClothingActor *act = static_cast< NxClothingActor *>(MActor->GetNxApexActor());
    		if ( act )
    		{
				act->setLODWeights(0,0,MIN_BENEFIT,MLodWeightsBenefitsBias);
    		}
		}
	}

	void SetWind(physx::PxF32 adaptTime,const physx::PxF32 *windVec)
	{
		if ( MActor )
		{
			physx::PxVec3 w;
			w.x = windVec[0];
			w.y = windVec[1];
			w.z = windVec[2];
			physx::apex::NxClothingActor *a = static_cast< physx::apex::NxClothingActor *>(MActor->GetNxApexActor());
			if ( a )
			{
				a->setWind(adaptTime,w);
			}
		}
	}

	void SetMaxDistanceScale(physx::PxF32 scale, INT ScaleMode)
	{
		if ( MActor )
		{
			physx::apex::NxClothingActor *a = static_cast< physx::apex::NxClothingActor *>(MActor->GetNxApexActor());
			if ( a )
			{
				a->updateMaxDistanceScale(scale, (ScaleMode == MDSM_Multiply) ? true : false);
			}
		}
	}

	virtual void ToggleClothingPieceSimulation(const UBOOL& bEnableSimulation)
	{
		if ( MActor )
		{
    		NxClothingActor *Act = static_cast< NxClothingActor *>(MActor->GetNxApexActor());
    		if ( Act )
    		{
				Act->setFrozen(!bEnableSimulation);
    		}
		}
	}

	UBOOL			 			MReady;
	UBOOL						MWasVisible;
	FIApexAsset					*MAsset;
	FIApexActor				   	*MActor;
	ApexClothing			   	*MParent;
	USkeletalMeshComponent		*MSkeletalMeshComponent;
	physx::PxMat44			    MRootPose;
	physx::PxU32						MSimulateCount;
	physx::PxF32						MContinuousRotationThreshold;
	physx::PxF32						mContinousDistanceThreshold;
    physx::PxF32						MLodWeightsMaxDistance;
    physx::PxF32						MLodWeightsDistanceWeight;
    physx::PxF32						MLodWeightsBias;
    physx::PxF32						MLodWeightsBenefitsBias;

	::NxParameterized::Interface *MParameterized;
	TBaseBoneRefArray			MBoneMapping;
	physx::PxU32						MMaterialIndex;

	physx::PxU32						MNewFrame;	// The current frame we have computed the camera distance for.
	physx::PxF32						MViewDistanceSquared; // the squared distance between this piece of clothing and the nearest camera
	physx::PxF32						MPrevLOD;
	physx::PxI32						MCurrGraphicalLOD;
	UBOOL								MGraphicalLODAvailable;
	NxArray<physx::pubfnd2::PxU32>		MGraphicalLODs;
	physx::pubfnd2::PxI32				MGraphicalLODNum;
	NxClothingActor::TeleportMode MTeleportationMode;
	UBOOL						bIsFirstFrame;
	UBOOL						bForcedHidden;
};

typedef NxArray< ApexClothingPiece * > TApexClothingPieceVector;

struct BoneRef
{
	BoneRef(const char *BoneName)
	{
		MBoneName  = BoneName;
		bUsed      = FALSE;
	}

	const char		*MBoneName;      // The normalized name of the bone
	UBOOL		     bUsed;          // Indicates whether this bone is actively used by any piece of clothing.
};

typedef NxArray< BoneRef > TBoneRefVector;

//** support for the apex clothing class, which associates multiple pieces of clothing with a single set of transforms.
class ApexClothing : public FIApexClothing
{
public:
	ApexClothing(FIApexScene *scene)
  	{
  		MUpdateReady = 0; // semaphore indicating that the rendering resources have been updated this frames
  		MReadyToRender = 0; // semaphore indicating that the rendering pipeline wanted to render the clothing this frame; true whether it actually rendered it or not.
  		MNeedsRefresh = FALSE;
  		MMatrices 	= NULL;
  		MBoneCount 	= 0;
		MSyncReady	= FALSE;
		MApexScene = scene;
		MPumpCount = 0;
		MPreviouslyVisible = FALSE; // by default not previously visible.
		MMaxDistanceScale = 1.0f;
		MMaxDistanceScaleTarget = 1.0f;
		MMaxDistanceScaleChangeVel = 0.0f;
		MMaxDistanceScaleMode = MDSM_Multiply;

		bForceTeleport = FALSE;
		bForceTeleportAndReset = FALSE;
 	}

 	~ApexClothing(void)
 	{
		Cleanup();
 	}

	void Cleanup()
	{
		for (TApexClothingPieceVector::iterator i=MPieces.begin(); i!=MPieces.end(); ++i)
		{
			ApexClothingPiece *ap = (*i);
			delete ap;
		}
		MPieces.clear();
		if ( MMatrices )
		{
			appFree(MMatrices);
			MMatrices = 0;
		}
	}

	virtual UBOOL UsesMaterialIndex(physx::PxU32 materialIndex) const
	{
		UBOOL ret = FALSE;
		for (TApexClothingPieceVector::const_iterator i=MPieces.begin(); i!=MPieces.end(); ++i)
		{
			ApexClothingPiece *ap = (*i);
			ret = ap->UsesMaterialIndex(materialIndex);
			if ( ret == TRUE )
			{
				break;
			}
		}
		return ret;
	}

	void Release(void);


	void NotifySceneGone(void)
	{
		MApexScene = NULL;
 		for (TApexClothingPieceVector::iterator i=MPieces.begin(); i!=MPieces.end(); ++i)
 		{
 			ApexClothingPiece *ap = (*i);
 			ap->NotifySceneGone();
 		}
	}

	virtual void Render(NxUserRenderer &renderer)
	{
 		for (TApexClothingPieceVector::iterator i=MPieces.begin(); i!=MPieces.end(); ++i)
 		{
 			ApexClothingPiece *ap = (*i);
 			ap->Render(renderer);
 		}
	}

	virtual FIApexClothingPiece * AddApexClothingPiece(FIApexAsset *asset,::NxParameterized::Interface *np,physx::PxU32 materialIndex, USkeletalMeshComponent* skeletalMeshComp)
	{
		ApexClothingPiece *ap = new ApexClothingPiece(asset,this,np,materialIndex, skeletalMeshComp);
		UBOOL found = FALSE;
		// look for an empty slot.
		for (TApexClothingPieceVector::iterator i=MPieces.begin(); i!=MPieces.end(); ++i)
		{
			if ( ap == NULL )
			{
				(*i) = ap;
				found = TRUE;
				break;
			}
		}
		if ( !found )
		{
			MPieces.push_back(ap);
		}
		return static_cast< FIApexClothingPiece *>(ap);
	}

	virtual void RemoveApexClothingPiece(FIApexClothingPiece *p)
	{
		ApexClothingPiece *ap = static_cast< ApexClothingPiece *>(p);
		if ( ap )
		{
			for (TApexClothingPieceVector::iterator i=MPieces.begin(); i!=MPieces.end(); ++i)
			{
				if ( ap == (*i) )
				{
					delete ap;
					(*i) = NULL;
					break;
				}
			}
		}
	}

	virtual void Pump(void)
	{
		MPumpCount++;
		for (TApexClothingPieceVector::iterator i=MPieces.begin(); i !=MPieces.end(); ++i)
		{
			ApexClothingPiece *ap = (*i);
			if ( ap )
			{
				ap->Pump();
			}
		}
	}

	virtual UBOOL IsReadyToRender(NxUserRenderer &renderer,const physx::PxF32 *eyePos)
	{
		UBOOL ret = !MPieces.empty();
		if ( ret )
		{
       		PX_ASSERT(MApexScene);
       		MVisibleFrame = MApexScene->GetSimulationCount();
       		for (TApexClothingPieceVector::iterator i=MPieces.begin(); i!=MPieces.end(); ++i)
       		{
       			ApexClothingPiece *ap = (*i);
       			if ( !ap->IsReady(eyePos) )
       			{
       				ret = FALSE;
       				break;
       			}
       		}
       		if ( ret )
       		{
				MReadyToRender = 2; 
				if  ( TRUE /*MUpdateReady*/ )
				{
           			for (TApexClothingPieceVector::iterator i=MPieces.begin(); i != MPieces.end(); ++i)
           			{
           				ApexClothingPiece *ap = (*i);
       					ap->Render(renderer);
           			}
        		}
				else
				{
					ret = FALSE;
				}
        	}
    	}

		if(FALSE /*!ret*/)
		{
			debugf(TEXT("--------IsReadyToRender = FALSE!!!!"));
		}

		return ret;
	}

	virtual void UpdateRenderResources(void)
	{
		if ( TRUE /*MReadyToRender*/ )
		{
			MReadyToRender--;
			MUpdateReady = 1; // set the resource update semaphore
   			for (TApexClothingPieceVector::iterator i=MPieces.begin(); i != MPieces.end(); ++i)
   			{
   				ApexClothingPiece *ap = (*i);
   				ap->UpdateRenderResources();
   			}
		}
		else
		{
			MUpdateReady = 0;
		}
	}

	// as 4x4 transforms
	virtual void SyncTransforms(physx::PxU32 boneCount,
                                const physx::PxF32 *matrices,
								physx::PxU32 stride,
								const FMatrix& LocalToWorld)
	{
		MSyncReady = TRUE;
		if ( boneCount == 0 ) return;

		if ( MMatrices == NULL )
		{
			MBoneCount = MBones.size();
			MMatrices = (physx::PxMat44 *)appMalloc(sizeof(physx::PxMat44)*MBoneCount);
		}


		const physx::PxU8 *scan = (const physx::PxU8 *)matrices;
		for (physx::PxU32 i=0; i<boneCount; i++)
		{
			physx::PxU32 index = MUsedBones[i];
			const physx::PxMat44 *pmat = (const physx::PxMat44 *)scan;
			MMatrices[index] = *pmat;
			scan+=stride;
		}

		for (TApexClothingPieceVector::iterator i=MPieces.begin(); i != MPieces.end(); ++i)
		{
			ApexClothingPiece *ap = (*i);
			ap->SyncPieceTransforms(MMatrices,MPreviouslyVisible, (physx::PxMat44&)LocalToWorld, bForceTeleportAndReset, bForceTeleport);
		}

		bForceTeleportAndReset = FALSE;
		bForceTeleport = FALSE;

		MPreviouslyVisible = IsVisible(false);

	}

	virtual void ForceNextUpdateTeleportAndReset()
	{
		bForceTeleportAndReset = TRUE;
	}

	virtual void ForceNextUpdateTeleport()
	{
		bForceTeleport = TRUE;
	}

	virtual void SetGraphicalLod(physx::pubfnd2::PxU32 lod)
	{
		for (TApexClothingPieceVector::iterator i=MPieces.begin(); i != MPieces.end(); ++i)
		{
			ApexClothingPiece *ap = (*i);
			ap->SetGraphicalLOD(lod);
		}
	}
	virtual FIApexScene * GetApexScene(void)
	{
		return MApexScene;
	}

	virtual physx::PxU32 GetPumpCount(void) const { return MPumpCount; };


	virtual UBOOL NeedsRefreshAsset(void)  // semaphore, TRUE if any of the underlying assets have been released and need to be re-loaded.
	{
		UBOOL ret = MNeedsRefresh;
		MNeedsRefresh = FALSE;
		return ret;
	}

	virtual void RefreshApexClothingPiece(FIApexAsset *asset,::NxParameterized::Interface *iface)
	{
		for (TApexClothingPieceVector::iterator i=MPieces.begin(); i != MPieces.end(); ++i)
		{
			ApexClothingPiece *ap = (*i);
			ap->RefreshApexClothingPiece(asset,iface);
		}
	}

	void SetNeedsRefresh(UBOOL state)
	{
		MNeedsRefresh = state;
	}

	// configure the bone mapping.
	virtual void AddBoneName(const char *_boneName)
	{
		const char *boneName = GApexCommands->GetPersistentString(_boneName);
		BoneRef b(boneName);
		MBones.push_back(b);
	}

	/****
	* Every time a child piece of clothing is created, we need to rebuild the master bone mapping table.
	***/
	void RefreshBoneMapping(void)
	{

		for (TBoneRefVector::iterator i=MBones.begin(); i!=MBones.end(); ++i)
		{
			(*i).bUsed = false;
		}

		MUsedBones.clear();

		for (TApexClothingPieceVector::iterator i=MPieces.begin(); i!=MPieces.end(); ++i)
		{
			ApexClothingPiece *ap = (*i);
			if ( ap )
			{
				// For each piece of clothing we iterate through the bones that it uses.
				for (TBaseBoneRefArray::iterator j=ap->MBoneMapping.begin(); j!= ap->MBoneMapping.end(); ++j)
				{
					BaseBoneRef &b = (*j);
					if ( !MBones[ b.MBaseIndex ].bUsed ) // if this bone is not already used, mark it as used.
					{
						MBones[b.MBaseIndex].bUsed = TRUE; // make it as used.
						MUsedBones.push_back( b.MBaseIndex ); // save the inverse lookup
					}
				}
			}
		}
		if ( MMatrices )
		{
			appFree(MMatrices);
			MMatrices = NULL;
		}
	}

	/**
	* Returns the number of bones actively used/referenced by clothing and
	* a pointer to an array of indices pointing back to the application source skeleton.
	*
	* @param : BoneCount a reference which returns the number of bones actively used.
	*
	* @return : A const pointer of indices mapping back to the original application skeleton.
	*/
	virtual const physx::PxU32 * GetBonesUsed(physx::PxU32 &BoneCount) const
	{
		BoneCount = MUsedBones.size();
		return BoneCount ? &MUsedBones[0] : NULL;
	}


	/***
	Returns the name of the used bone at this index number.
	*/
	virtual const char * GetBoneUsedName(physx::PxU32 BoneIndex) const
	{
		PX_ASSERT( BoneIndex < MUsedBones.size() );
		physx::PxU32 idx = MUsedBones[BoneIndex];
		PX_ASSERT( idx < MBones.size() );
		return MBones[idx].MBoneName;
	}

	void DisableSimulateThisFrame()
	{
		for (TApexClothingPieceVector::iterator i=MPieces.begin(); i!=MPieces.end(); ++i)
		{
			ApexClothingPiece *ap = (*i);
			if ( ap )
			{
				ap->DisableSimulateThisFrame();
			}
		}
	}

	/***
	Returns TRUE if this clothing is recently visible
	*/
	virtual UBOOL IsVisible(UBOOL updateBias)
	{
		UBOOL ret = FALSE;
		if ( MApexScene )
		{
			physx::PxU32 diff = MApexScene->GetSimulationCount() - MVisibleFrame;
			ret = diff < 4 ? TRUE : FALSE; // it is considered 'visible' if it has been on screen within the past 4 frames.
		}
		if ( ret == FALSE && updateBias )
		{
			for (TApexClothingPieceVector::iterator i=MPieces.begin(); i!=MPieces.end(); ++i)
			{
				ApexClothingPiece *ap = (*i);
				if ( ap )
				{
					ap->SetNonVisible();
				}
			}
		}
		return ret;
	}

	virtual void SetWind(physx::PxF32 adaptTime,const physx::PxF32 *windVector)
	{
		for (TApexClothingPieceVector::iterator i=MPieces.begin(); i!=MPieces.end(); ++i)
		{
			ApexClothingPiece *ap = (*i);
			if ( ap )
			{
				ap->SetWind(adaptTime,windVector);
			}
		}
	}

	virtual void Tick(FLOAT DeltaTime)
	{
		if (MMaxDistanceScaleChangeVel != 0.0f)
		{
			FLOAT DeltaScale = MMaxDistanceScaleChangeVel * DeltaTime;
			MMaxDistanceScale += DeltaScale;

			for (TApexClothingPieceVector::iterator i=MPieces.begin(); i!=MPieces.end(); ++i)
			{
				ApexClothingPiece *ap = (*i);
				if ( ap )
				{
					ap->SetMaxDistanceScale(MMaxDistanceScale, MMaxDistanceScaleMode);
				}
			}
		}
	}

	virtual void SetMaxDistanceScale(FLOAT StartScale, FLOAT EndScale, INT ScaleMode, FLOAT Duration)
	{
		MMaxDistanceScale = StartScale;
		MMaxDistanceScaleTarget = EndScale;
		MMaxDistanceScaleMode = static_cast<EMaxDistanceScaleMode>(ScaleMode);

		if (Duration > 0)
		{
			MMaxDistanceScaleChangeVel = (EndScale - StartScale) / Duration;
		}
		else
		{
			MMaxDistanceScale = EndScale;
			MMaxDistanceScaleChangeVel = 0;
			for (TApexClothingPieceVector::iterator i=MPieces.begin(); i!=MPieces.end(); ++i)
			{
				ApexClothingPiece *ap = (*i);
				if ( ap )
				{
					ap->SetMaxDistanceScale(MMaxDistanceScale, MMaxDistanceScaleMode);
				}
			}
		}
	}

	virtual UBOOL IsNewFrame(physx::PxU32 &NewFrame) const
	{
		UBOOL ret = FALSE;
		if ( MApexScene && NewFrame != MApexScene->GetSimulationCount() )
		{
			NewFrame = MApexScene->GetSimulationCount();
			ret = TRUE;
		}
		return ret;
	}
	virtual void ToggleClothingSimulation(const UBOOL& bEnableSimulation)
	{
		for (TApexClothingPieceVector::iterator i=MPieces.begin(); i!=MPieces.end(); ++i)
		{
			ApexClothingPiece *Ap = (*i);
			if ( Ap )
			{
				Ap->ToggleClothingPieceSimulation(bEnableSimulation);
			}
		}
	}

	virtual void SetVisible( UBOOL bEnable )
	{
		for (TApexClothingPieceVector::iterator i=MPieces.begin(); i!=MPieces.end(); ++i)
		{
			ApexClothingPiece* ClothingPiece = (*i);
			if ( ClothingPiece )
			{
				ClothingPiece->SetVisible( bEnable );
			}
		}
	}


//private:

	FIApexScene			   		*MApexScene;
	TApexClothingPieceVector 	MPieces;
	physx::PxU32						MBoneCount;
	UBOOL						MSyncReady;
	physx::PxMat44	    		*MMatrices;
	physx::PxU32						MPumpCount;
	UBOOL				    	MNeedsRefresh;
	TBoneRefVector				MBones;
	TPxU32Array					MUsedBones;
	physx::PxU32						MVisibleFrame;
	UBOOL						MPreviouslyVisible; // true if this clothing was previously visible on the last frame.
	physx::PxU32                       MUpdateReady;
	physx::PxU32						MReadyToRender;

	physx::PxF32				MMaxDistanceScale;
	physx::PxF32				MMaxDistanceScaleTarget;
	physx::PxF32				MMaxDistanceScaleChangeVel;
	EMaxDistanceScaleMode		MMaxDistanceScaleMode;

	/** Used to indicate we should force 'teleport' during the next call to SyncPieceTransforms */
	UBOOL bForceTeleport;
	/** Used to indicate we should force 'teleport and reset' during the next call to SyncPieceTransforms */
	UBOOL bForceTeleportAndReset;
};

ApexClothingPiece::~ApexClothingPiece(void)
{
	if ( MActor )
	{
		MActor->Release();
	}
}

void ApexClothingPiece::NotifySceneGone(void)
{
	if ( MActor )
	{
		MActor->Release();
		MActor = NULL;
	}
}

UBOOL ApexClothingPiece::IsReady(const physx::PxF32 *eyePos)
{
	if ( MAsset && !MAsset->IsReady() )
	{
		if(FALSE)
		{
			debugf(TEXT("CLOTHING NOT READY! MAsset=%x IsReady()=%d"), MAsset, MAsset->IsReady());
		}

		return FALSE;
	}

	physx::PxVec3 eye(eyePos[0],eyePos[1],eyePos[2]);
	physx::PxVec3 trans = MRootPose.getPosition();
	physx::PxF32 distanceSquared = trans.distanceSquared(eye);

	if ( MParent->IsNewFrame(MNewFrame) )
	{
		MViewDistanceSquared = distanceSquared;
	}
	else if ( distanceSquared < MViewDistanceSquared )
	{
		MViewDistanceSquared = distanceSquared;
	}

	UBOOL bHasLODChanged = FALSE;
	if(MActor)
	{
		NxClothingActor *Act = static_cast< NxClothingActor *>(MActor->GetNxApexActor());
		if(Act)
		{
			bHasLODChanged = (MPrevLOD > 0.0f && Act->getActivePhysicalLod() == 0.0f);
		}
	}

	UBOOL bIsReady = MReady && (MSimulateCount > 1 ? TRUE : FALSE) && !bHasLODChanged && MGraphicalLODAvailable;

	if(FALSE /*!bIsReady*/)
	{
		debugf(TEXT("CLOTHING NOT READY!  MReady=%d MSimulateCount=%d bHasLODChanged=%d MGraphicalLODAvailable=%d"), MReady, MSimulateCount, bHasLODChanged, MGraphicalLODAvailable);
	}

	return bIsReady;
};

/** Called to update the bone transforms of one piece of clothing */
void ApexClothingPiece::SyncPieceTransforms(const physx::PxMat44 *transforms,UBOOL previouslyVisible, const physx::PxMat44& LocalToWorld, UBOOL bForceTeleportAndReset, UBOOL bForceTeleport)
{
	if ( MActor && !MBoneMapping.empty() )
	{
		physx::PxMat44 CurrentTransform = transforms[ MBoneMapping[0].MBaseIndex ];
		physx::PxMat33 A( CurrentTransform.getBasis(0), CurrentTransform.getBasis(1), CurrentTransform.getBasis(2) );
		physx::PxMat33 B( MRootPose.getBasis(0), MRootPose.getBasis(1), MRootPose.getBasis(2) );
		physx::PxMat33 AInvB = A*B.getInverse();
		physx::PxF32 Trace = AInvB(0,0) + AInvB(1,1) + AInvB(2,2);
		physx::PxF32 CosineTheta = (Trace - 1.0f) / 2.0f;

		physx::PxF32 dist = physx::PxMax( 0.0f, physx::PxSqrt( MViewDistanceSquared ) );
		physx::PxF32 distScale = dist < MLodWeightsMaxDistance ? MLodWeightsMaxDistance - dist : MIN_BENEFIT;

		// if visible, or want to not 
		if ( previouslyVisible || !MSkeletalMeshComponent->bAutoFreezeApexClothingWhenNotRendered)
		{
			NxClothingActor *Act = static_cast< NxClothingActor *>(MActor->GetNxApexActor());
			if ( !MWasVisible )
			{
				//debugf(TEXT("WARP: Clothing is now visible but previously was not"));
				if (Act != NULL)
				{
					if( !bForcedHidden )
					{
						Act->setVisible(true);
					}
					Act->setFrozen(false);
					//debugf(TEXT("VIS/FROZEN"));
				}
			}

			NxClothingActor::TeleportMode TeleMode = NxClothingActor::TM_Continuous;

			if(bForceTeleportAndReset)
			{
				TeleMode = NxClothingActor::TM_TeleportAndReset;
			}
			else if(bForceTeleport)
			{
				TeleMode = NxClothingActor::TM_Teleport;
			}
			else if(bIsFirstFrame)
			{
				// Always teleport and reset on first frame
				TeleMode = NxClothingActor::TM_TeleportAndReset;
			}
			else if ( CosineTheta < MContinuousRotationThreshold ) // has the root bone rotated too much.
			{
				TeleMode = MTeleportationMode;
				//debugf(TEXT("WARP: Continuous Rotation Threshold"));
			}
			else
			{
				physx::PxF32 dist = MRootPose.getPosition().distanceSquared(CurrentTransform.getPosition());
				if ( dist > mContinousDistanceThreshold ) // if it has traveled too far.
				{
					TeleMode = MTeleportationMode;
					//debugf(TEXT("WARP: Continuous Distance Threshold"));
				}
			}

			bIsFirstFrame = FALSE;
			MRootPose = CurrentTransform;
			physx::PxMat44 globalPose = physx::PxMat44::getIdentity();
			TBaseBoneRefArray::iterator i;
			for (i=MBoneMapping.begin(); i!=MBoneMapping.end(); i++)
			{
				BaseBoneRef &b = (*i);
				b.MTransform = transforms[b.MBaseIndex];
			}
			if ( Act )
			{
				MPrevLOD = Act->getActivePhysicalLod();
				NxParameterized::Interface* ActorDesc = Act->getActorDesc();
				NxParameterized::setParamF32(*ActorDesc, "lodWeights.bias", distScale);
				Act->updateState(LocalToWorld,&MBoneMapping[0].MTransform,sizeof(BaseBoneRef),(physx::PxU32)MBoneMapping.size(), TeleMode);
				//debugf(TEXT("  updateState %d"), (INT)TeleMode);

#if 0
				// Debug code for drawing bone locations
				if(TeleMode == NxClothingActor::TM_TeleportAndReset)
				{
					for (i=MBoneMapping.begin(); i!=MBoneMapping.end(); i++)
					{
						BaseBoneRef &b = (*i);
						physx::PxMat44 PTM = transforms[b.MBaseIndex];
						physx::PxVec3 PBonePos = PTM.getPosition();
						FVector BonePos(PBonePos.x, PBonePos.y, PBonePos.z);
						debugf(TEXT("DrawBone %s"), *BonePos.ToString());
						GWorld->PersistentLineBatcher->DrawLine(FVector(0,0,0), P2UScale * BonePos, FColor(255,255,0), SDPG_World);
					}
				}
#endif
				MReady = (Act->getActivePhysicalLod() > 0);
			}
			else
			{
				MReady = TRUE ;
			}
		}
		else
		{
			if ( MWasVisible )
			{
				//debugf(TEXT("CLOTHING NO LONGER VISIBLE"));
				MWasVisible = FALSE;
				NxClothingActor *Act = static_cast< NxClothingActor *>(MActor->GetNxApexActor());
				if ( Act )
				{
					MPrevLOD = Act->getActivePhysicalLod();
					NxParameterized::Interface* ActorDesc = Act->getActorDesc();
					NxParameterized::setParamF32(*ActorDesc, "lodWeights.bias", 0);
				}
			}
			MReady = TRUE;
		}
	}
}

void ApexClothingPiece::Render(NxUserRenderer &renderer)
{
	if ( MActor )
	{
		MActor->Render(renderer);
	}
}

void ApexClothingPiece::UpdateRenderResources(void)
{
	if ( MActor )
	{
		MActor->UpdateRenderResources();
	}
}

void ApexClothingPiece::InitGraphicalLOD()
{
	const NxParameterized::Interface *assetParams = MAsset->GetNxApexAsset()->getAssetNxParameterized();
	if ( assetParams )
	{
		NxParameterized::Handle handle(*assetParams);
		handle.getParameter("graphicalLods");
		handle.getArraySize(MGraphicalLODNum);
		MGraphicalLODs.resize(MGraphicalLODNum);
		for (int i = 0; i < MGraphicalLODNum; ++i)
		{
			// select child i
			handle.set(i);
			NxParameterized::Interface* lodParams;
			handle.getParamRef(lodParams);
			// go back to parent element
			handle.popIndex();
			NxParameterized::getParamU32(*lodParams, "lod", MGraphicalLODs[i]);
		}
	}
}


void ApexClothingPiece::SetGraphicalLOD( physx::pubfnd2::PxU32 lod )
{
	NxClothingActor *Act = static_cast< NxClothingActor *>(MActor->GetNxApexActor());
	MCurrGraphicalLOD = lod;
	
	if(Act)
	{
		if(MGraphicalLODNum > MCurrGraphicalLOD)
		{	
			MGraphicalLODAvailable = TRUE;
			if(Act->isVisible())
			{
				 Act->setFrozen(FALSE);
			}
			Act->setGraphicalLOD(lod);
		}
		else
		{
			MGraphicalLODAvailable = FALSE;
			if(MSkeletalMeshComponent->bAutoFreezeApexClothingWhenNotRendered)
			{
				Act->setFrozen(TRUE);
			}
		}
	}
}

void ApexClothingPiece::Pump(void)
{

	if ( MActor == 0 ) // if we haven't created an actor yet....
	{
		if ( MParent->MSyncReady ) // and our parent has received live transform data...
		{
			MActor = MParent->GetApexScene()->CreateApexActor(MAsset,MParameterized);
			if ( MParameterized )
			{
				MParameterized->destroy();
				MParameterized = NULL;
			}
			InitGraphicalLOD();
			BuildBoneMapping();
		}
	}
	else if ( MActor->GetNxApexActor() == 0  && MParent->MMatrices  && MParameterized )
	{
	  	MParent->GetApexScene()->RecreateApexActor(MActor,MParameterized);
		MParameterized->destroy();
		MParameterized = NULL;
		InitGraphicalLOD();
		BuildBoneMapping();
	}
	else if ( MActor )
	{
		if ( MActor->GetNxApexActor() )
		{
			MSimulateCount++;
		}
		else
		{
			MParent->SetNeedsRefresh(TRUE);
			MSimulateCount = 0;
			MReady = FALSE;
		}
	}
}

static void GetBoneName(const char *bone,char *dest)
{
	for (physx::PxU32 i=0; i<255; i++)
	{
		char c = *bone++;
		if ( c == 0 )
		{
		    break;
		}
		if ( c >= 'A' && c <= 'Z' )
		{
		    c+=32;
		}
		if ( c == 32 )
		{
		    c = '-';
		}
		if ( c == '_' )
		{
		    c = '-';
		}
		*dest++ = c;
	}
	*dest = 0;
}
static UBOOL SameBoneName(const char *a,const char *b)
{
	char bone1[256];
	char bone2[256];
	GetBoneName(a,bone1);
	GetBoneName(b,bone2);
	return strcmp(bone1,bone2) == 0 ? TRUE : FALSE;
}

void ApexClothingPiece::BuildBoneMapping(void)
{

	MBoneMapping.clear();

	if ( MAsset )
	{
		NxApexAsset *apexAsset = MAsset->GetNxApexAsset();
		NxClothingAsset *clothingAsset = static_cast< NxClothingAsset *>(apexAsset);
		physx::PxU32 usedCount = clothingAsset->getNumUsedBones();

		for (physx::PxU32 i=0; i<usedCount; i++)
		{
			const char *boneName = clothingAsset->getBoneName(i);
			UBOOL found = FALSE;
			physx::PxU32 refIndex = 0;
			physx::PxMat44 worldBindPose;
			if ( MParent->MBones.empty() )
			{
				refIndex = i;
				found = TRUE;
			}
			else
			{
    			TBoneRefVector::iterator j;
    			for (j=MParent->MBones.begin(); j!=MParent->MBones.end(); ++j)
    			{
    				BoneRef &b = (*j);
    				if ( SameBoneName(b.MBoneName, boneName) )
    				{
    					found = TRUE;
    					break;
    				}
    				refIndex++;
    			}
    		}
			PX_ASSERT(found);
			if ( !found )
			{
				PX_ASSERT(0);
			    refIndex = 0;
			}
			BaseBoneRef b;
			b.MBaseIndex = refIndex;
			clothingAsset->getBoneBasePose(i,b.MTransform);
			MBoneMapping.push_back(b);
		}
	}
	MParent->RefreshBoneMapping();
}


typedef NxArray< ApexClothing * >	TApexClothingVector;
typedef NxArray< ApexActor * > 		TApexActorVector;

class ApexScene;

#if WITH_APEX_PARTICLES
class ApexEmitter;
typedef NxArray< ApexEmitter * >	TApexEmitterVector;


class ApexEmitter : public FApexEmitter
{
public:

	ApexEmitter(FIApexScene *scene,FIApexAsset *asset)
	{
		mRenderVolume = NULL;
		mScene = scene;
		mApexActor = NULL;
		NxParameterized::Interface *iface = asset->GetDefaultApexActorDesc();
		PX_ASSERT(iface);
		if ( iface )
		{
			physx::PxMat44 _pose = physx::PxMat44::getIdentity();
			_pose.setPosition(physx::PxVec3(0,0,0));
			physx::PxMat34Legacy pose = _pose;
			NxParameterized::setParamMat34(*iface,"initialPose",pose);
			mApexActor = scene->CreateApexActor(asset,iface);
			if ( mApexActor && mApexActor->GetNxApexActor() )
			{
				NxModuleIofx *iofx = GApexManager->GetModuleIofx();
				physx::PxBounds3 b;
				b.setInfinite();
				mRenderVolume = iofx->createRenderVolume(*mScene->GetApexScene(),b,0,true);
				NxApexEmitterActor *aea = static_cast<NxApexEmitterActor *>(mApexActor->GetNxApexActor());
				aea->setPreferredRenderVolume(mRenderVolume);
			}
		}
	}

	~ApexEmitter(void)
	{
		if ( mApexActor )
		{
			mApexActor->Release();
			mApexActor = NULL;
		}
		if ( mRenderVolume )
		{
			NxModuleIofx *iofx = GApexManager->GetModuleIofx();
			iofx->releaseRenderVolume(*mRenderVolume);
			mRenderVolume = NULL;
		}
	}

	virtual void release(void);

	virtual void notifySceneGone(void)
	{
		if ( mApexActor )
		{
			mApexActor->Release();
			mApexActor = NULL;
		}
	}

  virtual void							Render(physx::apex::NxUserRenderer &Renderer) 
  {
	  PX_FORCE_PARAMETER_REFERENCE(Renderer);
	  if ( mRenderVolume )
	  {
		 //debugf(NAME_DevPhysics, TEXT("EmitterRender"));
		  mRenderVolume->dispatchRenderResources(Renderer);
	  }
  }

  /**
  * Gathers the rendering resources for this APEX actor
  **/
  virtual void							UpdateRenderResources(void) 
  {
	  if ( mRenderVolume )
	  {
		  //debugf(NAME_DevPhysics, TEXT("EmitterUpdate"));
		  mRenderVolume->lockRenderResources();
		  mRenderVolume->updateRenderResources();
		  mRenderVolume->unlockRenderResources();
	  }
  }

  virtual UBOOL IsEmpty(void)
  {
	  UBOOL ret = TRUE;
	  if ( mRenderVolume )
	  {
		  if ( !mRenderVolume->getBounds().isEmpty() )
		  {
			  ret = FALSE;
		  }
	  }
	  return ret;
  }

	NxApexRenderVolume	*mRenderVolume;
	FIApexScene *mScene;
	FIApexActor	*mApexActor;
};
#endif
class FApexClothingDelete : public FDeferredCleanupInterface
{
public:
	FApexClothingDelete(ApexClothing *ac)
  	{
		ac->Cleanup();
		MApexClothing = ac;
	}

	virtual ~FApexClothingDelete()
	{
		delete MApexClothing;
	}

	virtual void FinishCleanup()
	{
		delete this;
	}
private:
	ApexClothing	*MApexClothing;
};

static physx::PxMat44 PxTransformFromUETransform( const FMatrix& InTransform )
{
	physx::PxMat44* OutTransform;
	FMatrix Transform(InTransform);

	Transform.ScaleTranslation( FVector(U2PScale) );
	OutTransform = reinterpret_cast<physx::PxMat44*>( &Transform );

	return *OutTransform;
}

#if USE_UE3_THREADPOOL

class ApexTask : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<ApexTask>;

	physx::pxtask::BaseTask *Target;

	ApexTask(physx::pxtask::BaseTask *InTarget)
		: Target(InTarget)
	{
	}

	void DoWork()
	{
		Target->run();
		Target->release();
	}

	static const TCHAR *Name()
	{
		return TEXT("ApexTask");
	}
};

class ApexDispatcher : public physx::pxtask::CpuDispatcher
{
public:
	virtual void submitTask( physx::pxtask::BaseTask& task )
	{
		(new FAutoDeleteAsyncTask<ApexTask>(&task))->StartHiPriorityTask();
	}
};

ApexDispatcher GApexDispatcher;

#endif

class ApexScene : public FIApexScene
{
public:

	ApexScene(NxScene *scene,FIApexManager *sdk,UBOOL useDebugRenderable)
  	{
  		MSimulateCount = 0;
		MUseDebugRenderable = useDebugRenderable;
	  	MScene = scene;
	  	MApexManager   = sdk;
	  	MApexSDK = sdk->GetApexSDK();
	  	MApexScene = NULL;
	  	if ( MApexSDK )
	  	{
			NxApexSceneDesc apexSceneDesc;

			mTaskManager = MApexSDK->createTaskManager();
			apexSceneDesc.taskManager = mTaskManager;
#if USE_UE3_THREADPOOL
			if ( mTaskManager )
			{
				mTaskManager->setCpuDispatcher(GApexDispatcher);
			}
#endif
			physx::pxtask::CudaContextManager *cudaContext = GApexManager->GetCudaContextManager();
			if ( mTaskManager && cudaContext  )
			{
				mTaskManager->setGpuDispatcher(*cudaContext->getGpuDispatcher());
			}

		  	apexSceneDesc.scene = MScene;
		  	apexSceneDesc.useDebugRenderable = MUseDebugRenderable ? true : false;
		  	MApexScene = MApexSDK->createScene(apexSceneDesc);
	  	}
	  	if ( MApexScene )
		{
			MApexScene->setUseDebugRenderable(MUseDebugRenderable ? true : false);
			physx::PxMat44 view = physx::PxMat44::getIdentity();
			// TODO: Is LOOK_AT_LH appropriate for all platforms?
			MViewID = MApexScene->allocViewMatrix( physx::apex::ViewMatrixType::LOOK_AT_LH );
			MApexScene->setViewMatrix(view,MViewID);
			MProjID = MApexScene->allocProjMatrix( physx::apex::ProjMatrixType::USER_CUSTOMIZED );
			MApexScene->setLODResourceBudget( GSystemSettings.ApexLODResourceBudget );
		}
	    GActiveApexSceneCount++;
		GApexCommands->AddApexScene(this);
  	}

  ~ApexScene(void)
  {
	  GApexCommands->RemoveApexScene(this);
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER
	(
		FEnqueData,
		FIApexRender *, e, GApexRender,
		{
			GApexRender->FlushFEnqueDataRenderingThread();
		}
	);

	GApexRender->FlushFEnqueDataGameThread();
#if WITH_APEX_PARTICLES
	for (TApexEmitterVector::iterator i=MEmitters.begin(); i!=MEmitters.end(); ++i)
	{
		ApexEmitter *ae = (*i);
		if ( ae )
		{
			ae->notifySceneGone();
			ae->release();
		}
	}
#endif
	for (TApexClothingVector::iterator i=MApexClothing.begin(); i != MApexClothing.end(); ++i)
	{
		ApexClothing *aa = (*i);
		if ( aa )
		{
			aa->NotifySceneGone();
		}
	}

	  if ( MApexScene )
	  {
		  MApexScene->setPhysXScene(0);
		  MApexScene->release();
	  }

	  GActiveApexSceneCount--;
  }

  virtual void Simulate(physx::PxF32 dtime)
  {
		GApexCommands->Pump(); //  complete the 'force load' operation on outstanding APEX Assets from the Game Thread
#if FAKE_LAG
		Sleep(60);
#endif

		if( GIsEditor && !GIsGame && (dtime > (1.0f/10.0f)) )
		{
			// if the simulation delta time is greater than 1/10th of a second, don't simulate clothing
			// Works around bad behavior in AnimSetViewer when holding down a button for a long time
			for (TApexClothingVector::iterator i=MApexClothing.begin(); i != MApexClothing.end(); ++i)
			{
				ApexClothing *aa = (*i);
				if ( aa )
				{
					aa->DisableSimulateThisFrame();
				}
			}

		}

		if (MApexScene )
		{
			MApexScene->simulate(dtime);
		}
		MSimulateCount++;
  }

  virtual UBOOL FetchResults(UBOOL forceSync,physx::PxU32 *errorState)
  {
	  UBOOL ret = TRUE;
	  if ( MApexScene )
	  {
			ret = MApexScene->fetchResults(forceSync ? true : false,errorState);
		    // ok, process clothing.
        	for (TApexClothingVector::iterator i=MApexClothing.begin(); i!=MApexClothing.end(); ++i)
        	{
        		ApexClothing *aa = (*i);
        		if ( aa )
        		{
        			aa->Pump();
        		}
        	}
	  }
	  return ret;
  }

  virtual NxApexScene * GetApexScene(void) const { return MApexScene; };

  virtual FIApexActor   *CreateApexActor(FIApexAsset *ia,const ::NxParameterized::Interface *params)
  {
    FIApexActor *ret = NULL;

	if ( ia && params == NULL )
	{
		params = ia->GetDefaultApexActorDesc();
	}

    if ( ia && params )
    {
        ApexActor *aa = new ApexActor(this,ia,params);
        if ( aa->IsOk() )
        {
		  aa->MApexAsset->IncRefCount(aa);
          ret = static_cast< FIApexActor *>(aa);
        }
        else
        {
          delete aa;
        }
    }
    return ret;
  }

  virtual UBOOL			RecreateApexActor(FIApexActor *actor,const ::NxParameterized::Interface *params)
  {
	UBOOL ret = FALSE;

  	ApexActor *aa = static_cast< ApexActor *>(actor);
  	if ( aa )
  	{
  		ret = aa->Recreate(this,params);
  	}
  	return ret;
  }

  virtual NxApexScene *GetNxApexScene(void) const
  {
	  return MApexScene;
  }

  virtual const NxDebugRenderable * GetDebugRenderable(void)
  {
    const NxDebugRenderable *ret = NULL;
    if ( MApexScene && MUseDebugRenderable && GApexCommands->IsVisualizeApex() )
    {
		MApexScene->lockRenderResources();
		MApexScene->updateRenderResources();
		MApexScene->dispatchRenderResources(*GApexCommands->GetNxUserRenderer());
		MApexScene->unlockRenderResources();
		ret = MApexScene->getDebugRenderable();
    }
	  return ret;
  }

  virtual void SetDebugRenderState(physx::PxF32 visScale)
  {
	  PX_FORCE_PARAMETER_REFERENCE(visScale);
#if 0 // TODO:JWR
    if ( MApexSDK )
	  {
		  MApexSDK->setParameter(NxApexParam::VISUALIZATION_SCALE, visScale);
	  }
#endif
  }

	virtual	FIApexClothing * CreateApexClothing(void)
	{
		ApexClothing *ac = new ApexClothing(this);
		UBOOL found = FALSE;
		for (TApexClothingVector::iterator i=MApexClothing.begin(); i!=MApexClothing.end(); ++i)
		{
			if ( (*i) == NULL )
			{
				(*i) = ac;
				found = TRUE;
				break;
			}
		}
		if ( !found )
		{
			MApexClothing.push_back(ac);
		}

		return static_cast< FIApexClothing *>(ac);
	}

	virtual void ReleaseApexActor(FIApexActor &actor)
	{
		ApexActor *ac = static_cast< ApexActor *>(&actor);
		delete ac;
	}

	void ReleaseApexClothing(ApexClothing *ac)
	{
		for (TApexClothingVector::iterator i=MApexClothing.begin(); i!=MApexClothing.end(); ++i)
		{
			if ( ac == (*i) )
			{
				FApexClothingDelete *fd = new FApexClothingDelete(ac); // defers the deletion..
				BeginCleanup(fd);
				(*i) = NULL;
				break;
			}
		}
	}

	virtual physx::PxU32 GetSimulationCount(void) const 
	{
		return MSimulateCount;
	}

#if WITH_APEX_PARTICLES
	virtual FApexEmitter * CreateApexEmitter(const char *assetName)
	{
		FApexEmitter *ret = NULL;
		PX_FORCE_PARAMETER_REFERENCE(assetName);
		FIApexAsset * asset = GApexCommands->GetApexAsset(assetName);
		if ( asset )
		{
			if ( asset->GetType() == AAT_APEX_EMITTER )
			{
				ApexEmitter *ae = new ApexEmitter(this,asset);
				ret = static_cast< FApexEmitter *>(ae);
				for (physx::PxU32 i=0; i<MEmitters.size(); i++)
				{
					if ( MEmitters[i] == NULL )
					{
						MEmitters[i] = ae;
						ae = NULL;
						break;
					}
				}
				if ( ae )
				{
					MEmitters.pushBack(ae);
				}
			}
			else
			{
				debugf(NAME_DevPhysics, TEXT("Not an APEX emitter asset"));
			}
		}
		else
		{
      		debugf(NAME_DevPhysics, TEXT("Failed to locate apex emitter asset"));
		}
		return ret;
	}

	void notifyEmitterGone(ApexEmitter *ae)
	{
		for (physx::PxU32 i=0; i<MEmitters.size(); i++)
		{
			if ( MEmitters[i] == ae )
			{
				MEmitters[i] = NULL;
			}
		}
	}
#endif

	virtual void UpdateProjection(const FMatrix& ProjMatrix, FLOAT FOV, FLOAT Width, FLOAT Height, FLOAT MinZ)
	{
		MApexScene->setProjMatrix( PxTransformFromUETransform(ProjMatrix), MProjID );
		MApexScene->setProjParams( 
			U2PScale*MinZ, 
			MAX_FLT, 
			FOV, 
			appTrunc(Width), 
			appTrunc(Height),
			MProjID
			);
	}

	virtual void UpdateView(const FMatrix& ViewMatrix)
	{
		MApexScene->setViewMatrix( PxTransformFromUETransform(ViewMatrix), MViewID );
	}


private:
	FIApexManager             *MApexManager;
	NxScene	              	  *MScene;
	NxApexSDK                 *MApexSDK;
	NxApexScene               *MApexScene;
	TApexClothingVector   	   MApexClothing;
#if WITH_APEX_PARTICLES
	TApexEmitterVector		   MEmitters;
#endif
	UBOOL					   MUseDebugRenderable;
	physx::PxU32			   MSimulateCount;
	physx::pxtask::TaskManager	*mTaskManager;
	physx::PxU32				MViewID;
	physx::PxU32				MProjID;
};

UBOOL ApexActor::Recreate(ApexScene *parentScene,const ::NxParameterized::Interface *params)
{
	PX_ASSERT(MApexAsset);
	if ( MActor )
	{
		MActor->release();
		MActor = NULL;
	}

  MParent = parentScene;
  NxApexAsset *aa                = MApexAsset->GetNxApexAsset();
  NxApexScene *apexScene         = parentScene->GetNxApexScene();
  MType = ApexActorType::UNKNOWN;
  if ( params && apexScene )
  {
	MActor = aa->createApexActor(*params,*apexScene);
	if ( MActor )
	{
		switch ( MApexAsset->GetType() )
		{
			case AAT_DESTRUCTIBLE:
				MType = ApexActorType::DESTRUCTIBLE;
				break;
			case AAT_CLOTHING:
				MType =  ApexActorType::CLOTHING;
				break;
		}
	}
  }
  return MActor ? TRUE : FALSE;
}

ApexActor::ApexActor(ApexScene *parentScene,
					FIApexAsset *apexAsset,
					const ::NxParameterized::Interface *params)
{
  MParent = parentScene;
  MApexAsset = apexAsset;
  NxApexAsset *aa                = apexAsset->GetNxApexAsset();
  NxApexScene *apexScene         = parentScene->GetNxApexScene();
  MActor = NULL;
  MType = ApexActorType::UNKNOWN;
  if ( params && apexScene )
  {
	MActor = aa->createApexActor(*params,*apexScene);
	if ( MActor )
	{
		switch ( apexAsset->GetType() )
		{
#if WITH_APEX_PARTICLES
			case AAT_APEX_EMITTER:
				MType = ApexActorType::EMITTER;
				{
					NxApexEmitterActor *a = static_cast< NxApexEmitterActor *>(MActor);
					physx::PxVec3 pos(0,0,100);
					a->setCurrentPosition(pos);
					a->startEmit(true);
				}
				break;
#endif
			case AAT_DESTRUCTIBLE:
				MType = ApexActorType::DESTRUCTIBLE;
				break;
			case AAT_CLOTHING:
				MType =  ApexActorType::CLOTHING;
				break;
		}
	}
  }
}


void ApexActor::Release(void)
{
  delete this;
}


void ApexClothing::Release(void)
{
	if ( MApexScene )
	{
		ApexScene *as = static_cast< ApexScene *>(MApexScene);
		as->ReleaseApexClothing(this);
	}
	else
	{
		FApexClothingDelete *fd = new FApexClothingDelete(this); // defers the deletion..
		BeginCleanup(fd);

	}
}

#if WITH_APEX_PARTICLES
void ApexEmitter::release(void)
{
	ApexScene *as = static_cast< ApexScene *>(mScene);
	as->notifyEmitterGone(this);
	delete this;
}
#endif


}; // end of APEX_SCENE namespace

using namespace APEX_SCENE;

/**
 * Creates the APEX Scene
 * @param [in,out]	Scene - If non-null, the NxScene.
 * @param [in,out]	Sdk	- If non-null, the FIApexManager.
 * @param	bUseDebugRenderable - TRUE to use debug renderable.
 *
 * @return	null if it fails, else a pointer to the FIApexScene.
**/
FIApexScene * CreateApexScene(NxScene *scene,FIApexManager *manager,UBOOL useDebugRenderable)
{
#ifndef PS3
	scene;
	manager;
	useDebugRenderable;
#endif
  ApexScene *as = new ApexScene(scene,manager,useDebugRenderable);
  if ( as->GetApexScene() == NULL )
  {
	  delete as;
	  as = NULL;
  }
  return static_cast< FIApexScene *>(as);
}

/**
 * Releases the APEX Scene
 * @param [in,out]	scene	If non-null, the scene.
**/
void          ReleaseApexScene(FIApexScene *scene)
{
  ApexScene *as = static_cast< ApexScene *>(scene);
  delete as;
}

#endif //WITH_APEX

#endif //WITH_NOVODEX
