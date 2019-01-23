/*----------------------------------------------------------------------
  Copyright	(c)	2005 Ageia Technologies, Inc.

  NxuPhysicsExport.cpp

  This is	the	common exporter	implementation that	contains format	independent	export logic.

  Changelist
----------

	*/
#ifndef NXU_PHYSICS_EXPORT_H

#define NXU_PHYSICS_EXPORT_H

#include "NXU_schema.h"
#include "NXU_customcopy.h"

namespace NXU
{

class	NxuPhysicsExport
{
public:

	NxuPhysicsExport(NxuPhysicsCollection *c);
	~NxuPhysicsExport();

	bool Write(NxPhysicsSDK *p); // save out the NxPhysics SDK descriptor
	bool Write(NxParameter p,	NxReal v);

#if NX_SDK_VERSION_NUMBER >= 260
	const char * Write(NxCompartment *c,const char *id=0);
#endif

	bool Write(const ::NxSceneDesc *scene,const char *userProperties,const char *id=0);
	bool Write(NxJoint *j,const char *userProperties,const char *id=0);
	bool Write(NxTriangleMesh	*mesh,const char *userProperties,const char *id=0);
	bool Write(NxConvexMesh	*mesh,const char *userProperties,const char *id=0);
	bool Write(NxHeightField *heightfield,const char *userProperties,const char *id=0);
	const char * Write(NxCCDSkeleton *skeleton,const char *userProperties,const char *id=0);
	bool Write(NxEffector	*e,const char *userProperties,const char *id=0);

#if NX_USE_FLUID_API
	bool Write(NxFluid *fluid,const char *userProperties,const char *id=0);
#endif

#if NX_USE_CLOTH_API
	bool Write(NxCloth *cloth,const char *userProperties,const char *id=0);
  const char * Write(NxClothMesh *clothMesh,const char *id=0);
#endif

	bool Write(NxPairFlag	*a,const char *userProperties,const char *id=0);
	bool Write(NxMaterial	*a,const char *userProperties,const char *id=0);
	bool Write(NxActor *a,const char *userProperties,const char *id=0);

#if NX_USE_SOFTBODY_API
	bool Write(NxSoftBody *softbody,const char *userProperties,const char *id=0);
  const char * Write(NxSoftBodyMesh *softBodyMesh,const char *id=0);
#endif


	bool setFilterOps(bool filter,::NxFilterOp op0,::NxFilterOp op1,::NxFilterOp op2,const ::NxGroupsMask &m1,const ::NxGroupsMask &m2);

	bool addGroupCollisionFlag(NxU32 group1,NxU32 group2,bool enable);
	bool addActorGroupFlag(NxU32 group1,NxU32 group2,NxU32 flags);
	bool addSceneInstance(const char *id,const char *sceneName,const NxMat34 &rootNode,bool ignorePlane=true,const char *userProperties=0);
	bool addToCurrentSceneInstance(const char *id,const char *sceneName,const NxMat34 &rootNode,bool ignorePlane=true,const char *userProperties=0);
private:

	bool addActor(NxActor *a); // if this actor doesn't already exist, add it.

#if NX_USE_CLOTH_API
  NxClothAttachDesc * getClothShapeAttachment(NxShape *shape); // turn this into a serializable object.
#endif

#if NX_USE_SOFTBODY_API
  NxSoftBodyAttachDesc * getSoftBodyShapeAttachment(NxShape *shape); // turn this into a serializable object.
#endif

  NxSceneDesc * getCurrentScene(void);

  NxuPhysicsCollection	*mCollection;
};

}; // end of namespace

#endif

//AGCOPYRIGHTBEGIN
///////////////////////////////////////////////////////////////////////////
// Copyright (c) 2005	AGEIA	Technologies.
// All rights	reserved.	www.ageia.com
///////////////////////////////////////////////////////////////////////////
//AGCOPYRIGHTEND
