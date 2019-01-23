/*=============================================================================
	UnNovodexSupport.h: Novodex support
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef _INC_UNNOVODEXSUPPORT
#define _INC_UNNOVODEXSUPPORT

#undef check

#define NOMINMAX

// When compiling on XBox or PS3, disable Novodex fluid and force static libs.
#if defined(XBOX) || PS3
#define NX_USE_SDK_STATICLIBS
#endif // XBOX || PS3

#pragma pack( push, 8 )
#include "NxFoundation.h"
#include "NxStream.h"
#include "NxPhysics.h"
#include "NxCooking.h"
#include "NxSceneQuery.h"
#ifndef NX_DISABLE_FLUIDS
	#include "fluids/NxFluid.h"
    #include "fluids/NxParticleData.h"
	#include "fluids/NxFluidEmitterDesc.h"	
#endif
#pragma pack( pop )


//JOEGTODO : Please fix me!
#if DO_CHECK
	#define check(expr)  {if(!(expr)) appFailAssert( #expr, __FILE__, __LINE__ );}
#else
	#if COMPILER_SUPPORTS_NOOP
		// MS compilers support noop which discards everything inside the parens
		#define check __noop
	#else
		#define check(expr) {}
	#endif
#endif


#define PHYSICS_DEBUG 1
#define PHYSICS_DEBUG_MAGNITUDE_THRESHOLD 10000
inline void setLinearVelocity(NxActor* nActor, const NxVec3 &newVel)
{
	#if !FINAL_RELEASE && PHYSICS_DEBUG
		if(newVel.magnitude() > PHYSICS_DEBUG_MAGNITUDE_THRESHOLD)
		{
			warnf(TEXT("Setting Linear Velocity to a large magnitude. Setting to: %f %f %f"),newVel[0],newVel[1],newVel[2]);
		}
	#endif
	nActor->setLinearVelocity(newVel);
}
inline void addForce(NxActor* nActor, const NxVec3 &force, NxForceMode mode=NX_FORCE, bool bWakeUp=true)
{
	
	#if !FINAL_RELEASE && PHYSICS_DEBUG
		if(force.magnitude() > PHYSICS_DEBUG_MAGNITUDE_THRESHOLD)
		{
			warnf(TEXT("Adding a large force ( %f %f %f ) to an object."),force[0],force[1],force[2]);
		}
	#endif
	nActor->addForce(force,mode,bWakeUp);
}

/** Global pointer to Novodex SDK object. */
extern NxPhysicsSDK*			GNovodexSDK;

/** Entry into physics scene map for holding pointer to software and possibly hardware scene. */
struct NxScenePair
{
	NxScenePair() 
	{ 
		PrimaryScene = NULL;
		RigidBodyCompartment = NULL;
		FluidCompartment = NULL;
		ClothCompartment = NULL;
		SoftBodyCompartment = NULL;
		SceneQuery = NULL;
	}

	NxScene       *PrimaryScene;
	NxCompartment *RigidBodyCompartment;
	NxCompartment *FluidCompartment;
	NxCompartment *ClothCompartment;
	NxCompartment *SoftBodyCompartment;

	/** Used for async line checks. */
	NxSceneQuery*	SceneQuery;
};

/** Structure used to read back information from Novodex fluid simulation */
class NxFluidParticle
{
public:
	NxReal mPos[3];
	NxReal mVel[3];
	NxReal mDensity;
	NxReal mLife;
	NxU32  mID;
};

/** Structure containing "graphics" related extra data for particles. */
class NxFluidParticleEx
{
public:
	NxQuat mRot;
	NxVec3 mAngVel;
	NxVec3 mSize;
};


/** 
 *	Map from SceneIndex to actual NxScene. This indirection allows us to set it to null when we kill the scene, 
 *	and therefore abort trying to destroy Novodex objects after the scene has been destroyed (eg. on game exit). 
 */
extern TMap<INT, NxScenePair>	GNovodexSceneMap;	// hardware scene support - using NxScenePair


/** 
 *	Array of Actors waiting to be cleaned up.
 *	Normally this is done right away, but just in case it tries to happen while DuringAsync is going on, they are deferred to here.
 */
extern TArray<NxActor*>			GNovodexPendingKillActor;

/** 
 *	Array of NxConvexMesh objects which are awaiting cleaning up.
 *	This needs to be deferred, as you can't destroy them until all uses of the mesh have been destroyed, and GC guarantees no order.
 */
extern TArray<NxConvexMesh*>	GNovodexPendingKillConvex;

/** 
 *	Array of NxTriangleMesh objects which are awaiting cleaning up.
 *	This needs to be deferred, as you can't destroy them until all uses of the mesh have been destroyed, and GC guarantees no order.
 */
extern TArray<NxTriangleMesh*>	GNovodexPendingKillTriMesh;

/** Array of heightfield objects to be cleaned up. */
extern TArray<NxHeightField*>	GNovodexPendingKillHeightfield;

/**
 *	Array of NxCCDSkeleton objects which are awaiting cleaning up.
 *	This needs to be deferred, as you can't destroy them until all uses of the mesh have been destroyed, and GC guarantees no order.
 */
extern TArray<NxCCDSkeleton*>	GNovodexPendingKillCCDSkeletons;


/**
 *	Array of NxFluid objects which are awaiting cleaning up.
 */
#ifndef NX_DISABLE_FLUIDS
extern TArray<NxFluid*>			GNovodexPendingKillFluids;
#endif

#if !NX_DISABLE_CLOTH
/** 
 *	Array of ForceFields waiting to be cleaned up.
 *	Normally this is done right away, but just in case it tries to happen while DuringAsync is going on, they are deferred to here.
 */
extern TArray<NxCloth*>			GNovodexPendingKillCloths;

/** 
*	Array of NxClothMesh objects which are awaiting cleaning up.
*	This needs to be deferred, as you can't destroy them until all uses of the mesh have been destroyed, and GC guarantees no order.
*/
extern TArray<NxClothMesh*>		GNovodexPendingKillClothMesh;
#endif // !NX_DISABLE_CLOTH

/** Total number of PhysX convex meshes around currently. */
extern INT						GNumPhysXConvexMeshes;

/** Total number of PhysX triangle meshes around currently. */
extern INT						GNumPhysXTriMeshes;

#define		UNX_GROUP_DEFAULT			(0)
#define		UNX_GROUP_NOTIFYCOLLIDE		(2)
#define		UNX_GROUP_MODIFYCONTACT		(3)


#define		SHOW_PHYS_INIT_COSTS		(!FINAL_RELEASE)
//#define		SHOW_SLOW_CONVEX
#define		USE_QUICKLOAD_CONVEX	1
//#define		ENABLE_CCD

/** Number of frames to wait before releasing a mesh - to avoid any problems where it is still being used. */
const INT DelayNxMeshDestruction = 2;

/** Allows you to force data to be re-cooked on load, instead of using cooked data in packages. */
const UBOOL bUsePrecookedPhysData = TRUE;

/** Util for seeing if PhysX hardware is present in this computer. */
UBOOL IsPhysXHardwarePresent();

/** Get a pointer to the scene-container struct holding software/hardware NxScene pointers. */
NxScenePair* GetNovodexScenePairFromIndex(INT InNovodexSceneIndex);

/** Get the Primary Scene for a given scen pair index. */
NxScene* GetNovodexPrimarySceneFromIndex(INT InNovodexSceneIndex);

/** Wait for a novodex scene to finish all simulations. */
void WaitForNovodexScene(NxScene &nScene);

/** PhysX Contact notify implementation */
class FNxContactReport : public NxUserContactReport
{
public:
	virtual void  onContactNotify(NxContactPair& pair, NxU32 events);
};

/** Other PhysX notification implementation */
class FNxNotify : public NxUserNotify
{
public:
	virtual bool onJointBreak(NxReal breakingForce, NxJoint& brokenJoint);
	virtual void onWake(NxActor** actors, NxU32 count) {}
	virtual void onSleep(NxActor** actors, NxU32 count) {}
};

/** PhysX contact modification callback. */
class FNxModifyContact : public NxUserContactModify
{
public:
	virtual bool onContactConstraint(
		NxU32& changeFlags, 
		const NxShape* shape0, 
		const NxShape* shape1, 
		const NxU32 featureIndex0, 
		const NxU32 featureIndex1,
		NxContactCallbackData& data);
};

/** Utility wrapper for a BYTE TArray for loading/saving from Novodex. */
class FNxMemoryBuffer : public NxStream
{
public:
	TArray<BYTE>			*Data;
	mutable UINT			ReadPos;

	FNxMemoryBuffer(TArray<BYTE> *InData)
	{
		ReadPos = 0;
		Data = InData;
	}

	// Loading API
	virtual		NxU8			readByte()								const;
	virtual		NxU16			readWord()								const;
	virtual		NxU32			readDword()								const;
	virtual		NxF32			readFloat()								const;
	virtual		NxF64			readDouble()							const;
	virtual		void			readBuffer(void* buffer, NxU32 size)	const;

	// Saving API
	virtual		NxStream&		storeByte(NxU8 b);
	virtual		NxStream&		storeWord(NxU16 w);
	virtual		NxStream&		storeDword(NxU32 d);
	virtual		NxStream&		storeFloat(NxF32 f);
	virtual		NxStream&		storeDouble(NxF64 f);
	virtual		NxStream&		storeBuffer(const void* buffer, NxU32 size);
};

// Unreal to Novodex conversion
NxMat34 U2NMatrixCopy(const FMatrix& uTM);
NxMat34 U2NTransform(const FMatrix& uTM);

NxVec3 U2NVectorCopy(const FVector& uVec);
NxVec3 U2NPosition(const FVector& uVec);

NxQuat U2NQuaternion(const FQuat& uQuat);

// Novodex to Unreal conversion
FMatrix N2UTransform(const NxMat34& nTM);

FVector N2UVectorCopy(const NxVec3& nVec);
FVector N2UPosition(const NxVec3& nVec);

FQuat	N2UQuaternion(const NxQuat& nQuat);

/** Utility for creating a PhysX 'groups mask' for filtering collision from the Unreal filtering info. */
NxGroupsMask CreateGroupsMask(BYTE Channel, FRBCollisionChannelContainer* CollidesChannels);

/** Utility for setting the material on all shapes that make up this NxActor physics object. */
void SetNxActorMaterial(NxActor* nActor, NxMaterialIndex NewMaterial, const UPhysicalMaterial* PhysMat);


/** Utility for comparing two Novodex matrices, to see if they are equal (to some supplied tolerance). */
UBOOL	MatricesAreEqual(const NxMat34& M1, const NxMat34& M2, FLOAT Tolerance);


/** Util for creating simple, collisionless kinematic actor, usually for creating springs to and animating etc. */
NxActor* CreateDummyKinActor(NxScene* NovodexScene, const FMatrix& ActorTM);

/** Util to destroy an actor created with the code above. */
void DestroyDummyKinActor(NxActor* KinActor);


// Utils
UBOOL RepresentConvexAsBox( NxActorDesc* ActorDesc, NxConvexMesh* ConvexMesh, UBOOL bCreateCCDSkel );
void MakeCCDSkelForSphere(NxSphereShapeDesc* SphereDesc);
void MakeCCDSkelForBox(NxBoxShapeDesc* BoxDesc);
void MakeCCDSkelForSphyl(NxCapsuleShapeDesc* SphylDesc);
void MakeCCDSkelForConvex(NxConvexShapeDesc* ConvexDesc);

/** Util to force-set the ref count of an NxTriangleMesh */
void SetNxTriMeshRefCount( NxTriangleMesh * tm, int refCount );

/** Util to force-set the ref count of an NxConvexMesh */
void SetNxConvexMeshRefCount( NxConvexMesh * cm, int refCount );

/** Add a force to an NxActor, but do not modify its waking state. */
void AddForceNoWake(NxActor *Actor, const NxVec3 &force);

/** Add a force to an NxActor, but only if the force is non-zero. */
void AddForceZeroCheck(NxActor *Actor, const NxVec3 &force);

/** Perform a cloth line check, uses an approximate obb for non zero extent checks. */
UBOOL ClothLineCheck(
				USkeletalMeshComponent* SkelComp,
				FCheckResult &Result,
                const FVector& End,
                const FVector& Start,
                const FVector& Extent,
				DWORD TraceFlags);

/** Perform a point check against a cloth(uses an approximate OBB) */
UBOOL ClothPointCheck(FCheckResult &Result, USkeletalMeshComponent* SkelComp, const FVector& Location, const FVector& Extent);

DWORD FindNovodexSceneStat(NxScene* NovodexScene, const TCHAR* StatNxName, UBOOL bMaxValue);

#endif

