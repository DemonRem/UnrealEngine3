/*----------------------------------------------------------------------
  Copyright	(c)	2005 Ageia Technologies, Inc.

  NxuPhysicsExport.cpp

  This is	the	common exporter	implementation that	contains format	independent	export logic.

  Changelist
----------

	*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#pragma warning(disable:4996) // Disabling stupid .NET deprecated warning.

#include "NXU_PhysicsExport.h"
#include "NXU_schema.h"
#include "NXU_customcopy.h"
#include "NXU_string.h"
#include "NXU_SchemaStream.h"
#include "NXU_Streaming.h"

namespace NXU
{

static NxI32	GetShapeIndex(NxActor *actor, NxShape *shape)
{
	NxI32	match	=	0;

	NxU32 count = actor->getNbShapes();
	if ( count )
	{
		NxShape *const*shapes = actor->getShapes();
  	for	(NxU32 i = 0;	i	<	count;	++i)
		{
			if (shapes[i] ==	shape)
			{
				match	=	i;
				break;
			}
		}
	}

	return match;
}

NxuPhysicsExport::NxuPhysicsExport(NxuPhysicsCollection *c)
{
	mCollection = c;
}

NxuPhysicsExport::~NxuPhysicsExport(void)
{
}

bool NxuPhysicsExport::Write(NxPhysicsSDK *p) // save out the NxPhysics SDK descriptor
{

	bool ret = false;

	::NxPhysicsSDKDesc d;


#if 0 // This API call is missing! TODO
	p->saveToDesc(d);
#endif

	CustomCopy cc;
	mCollection->mSDK.copyFrom(d,cc);

  return ret;
}

bool NxuPhysicsExport::Write(NxParameter p,	NxReal v)
{
	bool ret = false;

	NxParameterDesc *d = new NxParameterDesc;

  d->param = p;
  d->value = v;

  mCollection->mParameters.push_back(d);

  return ret;
}

bool NxuPhysicsExport::Write(const ::NxSceneDesc *scene,const char *userProperties,const char *id)
{
	bool ret = false;

	NxSceneDesc *sd = new NxSceneDesc;
	sd->mUserProperties = getGlobalString(userProperties);
	if ( id )
	{
		sd->mId = id;
	}
	else
	{
  	char scratch[512];
  	sprintf(scratch,"Scene_%d", mCollection->mScenes.size() );
  	sd->mId = getGlobalString(scratch);
  }

	CustomCopy cc;
	sd->copyFrom(*scene,cc);


	sd->mUserProperties = getGlobalString(userProperties);

	mCollection->mCurrentScene = sd;

	mCollection->mScenes.push_back(sd);


  return ret;
}

bool NxuPhysicsExport::Write(NxJoint *j,const char *userProperties,const char *id)
{
	bool ret = false;

	NxSceneDesc *current = getCurrentScene();

	CustomCopy cc(mCollection,current);

	NxJointDesc *joint = 0;

	switch ( j->getType() )
	{
		case NX_JOINT_PRISMATIC:
			if ( 1 )
			{
				::NxPrismaticJointDesc d1;
				NxPrismaticJoint *sj = j->isPrismaticJoint();
				sj->saveToDesc(d1);
				addActor( d1.actor[0] );
				addActor( d1.actor[1] );
				NxPrismaticJointDesc *desc = new NxPrismaticJointDesc;
				desc->copyFrom(d1,cc);
				joint = static_cast<NxJointDesc *>(desc);
			}
			break;
		case NX_JOINT_REVOLUTE:
			if ( 1 )
			{
				::NxRevoluteJointDesc d1;
				NxRevoluteJoint *sj = j->isRevoluteJoint();
				sj->saveToDesc(d1);
				addActor( d1.actor[0] );
				addActor( d1.actor[1] );
				NxRevoluteJointDesc *desc = new NxRevoluteJointDesc;
				desc->copyFrom(d1,cc);
				joint = static_cast<NxJointDesc *>(desc);
			}
			break;
		case NX_JOINT_CYLINDRICAL:
			if ( 1 )
			{
				::NxCylindricalJointDesc d1;
				NxCylindricalJoint *sj = j->isCylindricalJoint();
				sj->saveToDesc(d1);
				addActor( d1.actor[0] );
				addActor( d1.actor[1] );
				NxCylindricalJointDesc *desc = new NxCylindricalJointDesc;
				desc->copyFrom(d1,cc);
				joint = static_cast<NxJointDesc *>(desc);
			}
			break;
		case NX_JOINT_SPHERICAL:
			if ( 1 )
			{
				::NxSphericalJointDesc d1;
				NxSphericalJoint *sj = j->isSphericalJoint();
				sj->saveToDesc(d1);
				addActor( d1.actor[0] );
				addActor( d1.actor[1] );
				NxSphericalJointDesc *desc = new NxSphericalJointDesc;
				desc->copyFrom(d1,cc);
				joint = static_cast<NxJointDesc *>(desc);
			}
			break;
		case NX_JOINT_POINT_ON_LINE:
			if ( 1 )
			{
				::NxPointOnLineJointDesc d1;
				NxPointOnLineJoint *sj = j->isPointOnLineJoint();
				sj->saveToDesc(d1);
				addActor( d1.actor[0] );
				addActor( d1.actor[1] );
				NxPointOnLineJointDesc *desc = new NxPointOnLineJointDesc;
				desc->copyFrom(d1,cc);
				joint = static_cast<NxJointDesc *>(desc);
			}
			break;
		case NX_JOINT_POINT_IN_PLANE:
			if ( 1 )
			{
				::NxPointInPlaneJointDesc d1;
				NxPointInPlaneJoint *sj = j->isPointInPlaneJoint();
				sj->saveToDesc(d1);
				addActor( d1.actor[0] );
				addActor( d1.actor[1] );
				NxPointInPlaneJointDesc *desc = new NxPointInPlaneJointDesc;
				desc->copyFrom(d1,cc);
				joint = static_cast<NxJointDesc *>(desc);
			}
			break;
		case NX_JOINT_DISTANCE:
			if ( 1 )
			{
				::NxDistanceJointDesc d1;
				NxDistanceJoint *sj = j->isDistanceJoint();
				sj->saveToDesc(d1);
				addActor( d1.actor[0] );
				addActor( d1.actor[1] );
				NxDistanceJointDesc *desc = new NxDistanceJointDesc;
				desc->copyFrom(d1,cc);
				joint = static_cast<NxJointDesc *>(desc);
			}
			break;
		case NX_JOINT_PULLEY:
			if ( 1 )
			{
				::NxPulleyJointDesc d1;
				NxPulleyJoint *sj = j->isPulleyJoint();
				sj->saveToDesc(d1);
				addActor( d1.actor[0] );
				addActor( d1.actor[1] );
				NxPulleyJointDesc *desc = new NxPulleyJointDesc;
				desc->copyFrom(d1,cc);
				joint = static_cast<NxJointDesc *>(desc);
			}
			break;
		case NX_JOINT_FIXED:
			if ( 1 )
			{
				::NxFixedJointDesc d1;
				NxFixedJoint *sj = j->isFixedJoint();
				sj->saveToDesc(d1);
				addActor( d1.actor[0] );
				addActor( d1.actor[1] );
				NxFixedJointDesc *desc = new NxFixedJointDesc;
				desc->copyFrom(d1,cc);
				joint = static_cast<NxJointDesc *>(desc);
			}
			break;
		case NX_JOINT_D6:
			if ( 1 )
			{
				::NxD6JointDesc d1;
				NxD6Joint *sj = j->isD6Joint();
				sj->saveToDesc(d1);
				addActor( d1.actor[0] );
				addActor( d1.actor[1] );
				NxD6JointDesc *desc = new NxD6JointDesc;
				desc->copyFrom(d1,cc);
				joint = static_cast<NxJointDesc *>(desc);
			}
			break;

	}


	//Add	Limits
	// in	addition,	we also	have to	write	out	its	limit	planes!
	j->resetLimitPlaneIterator();
	if (j->hasMoreLimitPlanes())
	{
		// write limit point
		joint->mOnActor2 = j->getLimitPoint(joint->mPlaneLimitPoint);

		NxArray< NxPlaneInfoDesc *> plist;


		// write the plane normals
		while	(j->hasMoreLimitPlanes())
		{
			NxPlaneInfoDesc *pInfo	=	new	NxPlaneInfoDesc();
			j->getNextLimitPlane(pInfo->mPlaneNormal,	pInfo->mPlaneD);
			plist.push_back(pInfo);
		}

		if ( plist.size() )
		{
			for (int i=plist.size()-1; i>=0; i--)
			{
				NxPlaneInfoDesc *p = plist[i];
				joint->mPlaneInfo.pushBack(p);
			}
		}

	}


	if ( joint )
	{
		if ( id )
		{
			joint->mId = id;
		}
		else
		{
      char scratch[512];
      sprintf(scratch,"Joint_%d", current->mJoints.size());
      joint->mId = getGlobalString(scratch);
      joint->mUserProperties = getGlobalString(userProperties);
    }
		current->mJoints.push_back(joint);
	}

  return ret;
}
#if NX_USE_CLOTH_API
const char * NxuPhysicsExport::Write(NxClothMesh *clothMesh,const char *id)
{
	const char *ret = 0;

	if ( clothMesh )
	{

		for (NxU32 i=0; i<mCollection->mClothMeshes.size(); i++)
		{
			NxClothMeshDesc *c = mCollection->mClothMeshes[i];
			if ( c->mInstance == clothMesh )
			{
				ret = c->mId;
				break;
			}
		}

		if ( !ret )
		{

			::NxClothMeshDesc desc;
 			clothMesh->saveToDesc(desc);
			NxU32	numMesh	=	mCollection->mClothMeshes.size();
    	NxClothMeshDesc *cMesh	=	new NxClothMeshDesc;

      if ( id )
      {
      	cMesh->mId = id;
      }
      else
      {
  			char tempString[512];
  			sprintf(tempString,	"ClothMesh_%d", numMesh);
      	cMesh->mId = getGlobalString(tempString);
      }

    	CustomCopy cc(mCollection,0);

      cMesh->copyFrom(desc,cc);

			cMesh->mInstance = clothMesh;

			mCollection->mClothMeshes.pushBack(cMesh);

			ret = cMesh->mId;
		}

	}
	return ret;
}

#endif
bool NxuPhysicsExport::Write(NxTriangleMesh	*mesh,const char *userProperties,const char *id)
{
	bool ret = false;

  bool found = false;

	for (unsigned int i=0; i<mCollection->mTriangleMeshes.size(); i++)
	{
		NXU::NxTriangleMeshDesc *tm = mCollection->mTriangleMeshes[i];
		if ( tm->mInstance == mesh )
		{
			found = true;
			break;
		}
	}

  if ( !found )
  {
  	CustomCopy cc(mCollection,0);
  	::NxTriangleMeshDesc desc;
  	mesh->saveToDesc(desc);
  	NXU::NxTriangleMeshDesc *tmesh = new NXU::NxTriangleMeshDesc;

		if ( id )
		{
			tmesh->mId = id;
		}
		else
		{
      char scratch[512];
      sprintf(scratch,"TriangleMesh_%d", mCollection->mTriangleMeshes.size() );
      tmesh->mId = getGlobalString(scratch);
    }

    tmesh->mUserProperties = getGlobalString(userProperties);
    tmesh->mInstance = mesh;

    tmesh->copyFrom(desc,cc);

    if ( 1 ) // grab the cooked data.
    {
  		NxPMap pmap;
  		pmap.dataSize = mesh->getPMapSize();
  		NxU32 density = mesh->getPMapDensity();
  		if ( pmap.dataSize )
  		{
  			pmap.data = new NxU8[pmap.dataSize];
  			bool ok	=	mesh->getPMapData(pmap);
  			if ( ok )
  			{

  				tmesh->mPmapDensity   = density;
  				tmesh->mPmapSize      = pmap.dataSize;

  				NxU8 *pd = (NxU8 *) pmap.data;

  				for (NxU32 i=0; i<pmap.dataSize; i++)
  				{
  					NxU8 c = pd[i];
  					tmesh->mPmapData.push_back(c);
  				}
  			}
 				delete pmap.data;
  		}
  	}


  	mCollection->mTriangleMeshes.push_back(tmesh);

    ret = true;
  }
  return ret;
}

bool NxuPhysicsExport::Write(NxConvexMesh	*mesh,const char *userProperties,const char *id)
{
	bool ret = false;

  bool found = false;

	for (unsigned int i=0; i<mCollection->mConvexMeshes.size(); i++)
	{
		NXU::NxConvexMeshDesc *tm = mCollection->mConvexMeshes[i];
		if ( tm->mInstance == mesh )
		{
			found = true;
			break;
		}
	}

  if ( !found )
  {
  	CustomCopy cc(mCollection,0);
  	::NxConvexMeshDesc desc;
  	mesh->saveToDesc(desc);
  	NXU::NxConvexMeshDesc *tmesh = new NXU::NxConvexMeshDesc;

		if ( id )
		{
			tmesh->mId = id;
		}
		else
		{
      char scratch[512];
      sprintf(scratch,"ConvexMesh_%d", mCollection->mConvexMeshes.size() );
      tmesh->mId = getGlobalString(scratch);
    }

    tmesh->mUserProperties = getGlobalString(userProperties);
    tmesh->mInstance = mesh;

    tmesh->copyFrom(desc,cc);

  	mCollection->mConvexMeshes.push_back(tmesh);

    ret = true;
  }

  return ret;
}

bool NxuPhysicsExport::Write(NxHeightField *heightfield,const char *userProperties,const char *id)
{
	bool ret = false;


	bool found = false;

	int	num	=	mCollection->mHeightFields.size();
	for	(int i = 0;	i	<	num; ++i)
	{
		if (heightfield	== mCollection->mHeightFields[i]->mInstance)
		{
			found = true;
			break;
		}
	}
	if ( !found )
	{
	  CustomCopy cc(mCollection,0);
  	NxHeightFieldDesc *hf = new NxHeightFieldDesc;
  	if ( id )
  	{
  		hf->mId = id;
  	}
  	else
  	{
      char scratch[512];
      sprintf(scratch,"HeightField_%d", mCollection->mHeightFields.size() );
      hf->mId = getGlobalString(scratch);
    }

    hf->mUserProperties = getGlobalString(userProperties);
  	hf->mInstance	=	heightfield;
  	::NxHeightFieldDesc desc;
	  heightfield->saveToDesc(desc);
  	NxU32	size = heightfield->getNbRows()	*heightfield->getNbColumns() *heightfield->getSampleStride();

		if ( size )
		{
  	  NxU8 *tempSamples	=	new	NxU8[size];
		  heightfield->saveCells(tempSamples,	size);
			desc.samples = tempSamples;
			hf->copyFrom(desc,cc);
			delete tempSamples;
  		mCollection->mHeightFields.pushBack(hf);
  		ret = true;
		}

  }

  return ret;
}

struct VertexInfo
{
	NxVec3 vertex;
};

#define NOTFLAGSMASK 0x00FFFFFF;

struct EdgeInfo
{
  NxU32 getVertexIndex(NxU32 i) const
	{
		return vertices[i]&NOTFLAGSMASK;
	}
	NxVec3 normal;
	NxU32 vertices[2];			//indexes into vertices[].  High bytes are used for flags.
};


struct TriangleInfo
{
	NxPlane plane;
	NxU32	signedEdgeIndices[3];		//indexes into edges[].
};


const char *NxuPhysicsExport::Write(NxCCDSkeleton *skeleton,const char *userProperties,const char *id)
{
  const char *ret = 0;

	bool found = false;

	NxU32	num	=	mCollection->mSkeletons.size();
	for	(NxU32 i = 0;	i	<	num; ++i)
	{
    NxCCDSkeletonDesc *sd = mCollection->mSkeletons[i];
		if (skeleton ==	(mCollection->mSkeletons[i])->mInstance)
		{
			found = true;
      ret = sd->mId;
			break;
		}
	}

  if ( !found )
  {


  	NxCCDSkeletonDesc *skel = new NxCCDSkeletonDesc;

    if ( id )
    {
    	skel->mId = id;
    }
    else
    {
    	char tempString[512];
    	sprintf(tempString,	"CCD_skeleton%d",	num);
    	ret = skel->mId               = getGlobalString(tempString);
    }

    skel->mUserProperties   = getGlobalString(userProperties);

#if NX_SDK_VERSION_NUMBER >= 261
	  CustomCopy cc(mCollection,0);

		::NxSimpleTriangleMesh mesh;
		NxU32 tcount = skeleton->saveToDesc(mesh);
		if ( tcount )
		{
			NxSimpleTriangleMesh *sm = (NxSimpleTriangleMesh *) skel;
			sm->copyFrom(mesh,cc);
		}

#else

  	NxU32 cookedDataSize = skeleton->getDataSize();

		// here we are walking the edge info structures to extract out the original skeletal mesh
		// as a simple triangle mesh.
  	if ( cookedDataSize )
  	{
    	NxU8 *cookedData	=	new	NxU8[cookedDataSize];
    	skeleton->save(cookedData, sizeof(NxU8)*cookedDataSize);

    	NxU32 headerSize           = sizeof(NxU32) * 2;
    	NxU32 neededBufferSize     = cookedDataSize - headerSize; //trigSize + edgeSize + vertsSize;
    	NxU8 * membufferStart      = (NxU8*)cookedData + headerSize;
    	NxU8 * membufferEnd        = membufferStart + neededBufferSize;
    	TriangleInfo *bufTriangles = (TriangleInfo *)membufferStart;
    	NxU32 * srcBuffer32        = (NxU32 *)cookedData;
    	EdgeInfo *bufEdges         = (EdgeInfo *)(membufferStart + *srcBuffer32);
    	srcBuffer32++;
    	VertexInfo *bufVertices    = (VertexInfo *)(membufferStart + *srcBuffer32);
    	srcBuffer32++;

    	TriangleInfo *endTriangles = (TriangleInfo *)bufEdges;
    	EdgeInfo *endEdges         = (EdgeInfo *)bufVertices;
    	VertexInfo *endVertices    = (VertexInfo *)membufferEnd;

			NxU32 numTriangles = ((int)endTriangles-(int)bufTriangles)/sizeof(TriangleInfo);
			NxU32 numVertices =  ((int)endVertices-(int)bufVertices)/sizeof(VertexInfo);


    	for(unsigned int i = 0; i < numTriangles; i++)
    	{
    		NxTri t;

  		  NxU32 ea = bufTriangles[i].signedEdgeIndices[0];
  		  NxU32 eb = bufTriangles[i].signedEdgeIndices[1];
  		  NxU32 ec = bufTriangles[i].signedEdgeIndices[2];
				NxU32 v0,v1,v2;

				v0 = bufEdges[ea & ~(1 << 31)].getVertexIndex(ea >> 31);
				v1 = bufEdges[eb & ~(1 << 31)].getVertexIndex(eb >> 31);
				v2 = bufEdges[ec & ~(1 << 31)].getVertexIndex(ec >> 31);

  		  t.a = v0;
  		  t.b = v1;
  		  t.c = v2;


        skel->mTriangles.push_back(t);
    	}

    	for(unsigned int i = 0; i < numVertices; i++)
    	{
  		  NxVec3 v = bufVertices[i].vertex;
  		  skel->mPoints.push_back(v);
    	}

      delete cookedData;
    }

#endif
  	skel->mInstance	=	skeleton;
  	mCollection->mSkeletons.pushBack(skel);

  }

  return ret;
}

bool NxuPhysicsExport::Write(NxEffector	*e,const char *userProperties,const char *id)
{
	bool ret = false;


	NxSceneDesc *current = getCurrentScene();
	CustomCopy cc(mCollection,current);

	NxSpringAndDamperEffector	*sade	=	e->isSpringAndDamperEffector();

	if (sade)
	{
		NxSpringAndDamperEffectorDesc *effector = new NxSpringAndDamperEffectorDesc();

    if ( id )
    {
    	effector->mId = id;
    }
    else
    {
    	char tempString[512];
    	sprintf(tempString,	"SpringAndDamperEffector_%d",	current->mEffectors.size() );
    	effector->mId = getGlobalString(tempString);
    }

		effector->mUserProperties = getGlobalString(userProperties);
		effector->mInstance	=	sade;

		::NxSpringAndDamperEffectorDesc sd;
		sade->saveToDesc(sd);

    effector->copyFrom(sd,cc);


		current->mEffectors.push_back(effector);

		ret = true;
	}

  return ret;
}

#if NX_USE_FLUID_API
bool NxuPhysicsExport::Write(NxFluid *fluid,const char *userProperties,const char *id)
{

	NxSceneDesc *current = getCurrentScene();
	CustomCopy cc(mCollection,current);

	NxFluidDesc *fdesc = new NxFluidDesc;

  if ( id )
  {
  	fdesc->mId = id;
  }
  else
  {
    char scratch[512];
    sprintf(scratch,"Fluid_%d", current->mFluids.size() );
  	fdesc->mId = getGlobalString(scratch);
  }

	fdesc->mUserProperties = getGlobalString(userProperties);
	fdesc->mInstance = fluid;

  ::NxFluidDesc desc;
	fluid->saveToDesc(desc);
  desc.particlesWriteData = fluid->getParticlesWriteData();
	fdesc->copyFrom(desc,cc);


	//Save the emitters
	NxU32	count	=	fluid->getNbEmitters();
	NxFluidEmitter **emitters	=	fluid->getEmitters();
	for	(NxU32 i = 0;	i	<	count; ++i)
	{
		NxFluidEmitterDesc	*emitterDesc = new NxFluidEmitterDesc;
		::NxFluidEmitterDesc desc;
		emitters[i]->saveToDesc(desc);
		char scratch[512];
		sprintf(scratch,"Emitter_%d", i );

		emitterDesc->mId = getGlobalString(scratch);
		emitterDesc->mInstance = emitters[i];

		emitterDesc->copyFrom(desc,cc);


#if NX_SDK_VERSION_NUMBER >= 260
    NxShape *shape = emitters[i]->getFrameShape();
    if ( shape )
    {
      NxActor &a = shape->getActor();
      emitterDesc->mFrameActor = cc.getNameFromActor(&a);
      emitterDesc->mFrameShape = GetShapeIndex(&a,shape);
    }
#else
    NxActor *actor = emitters[i]->getFrameActor();
    if ( actor )
    {
    	emitterDesc->mFrameActor = cc.getNameFromActor(actor);
    	emitterDesc->mFrameShape = 0;
    }
#endif

		fdesc->mEmitters.pushBack(emitterDesc);
	}


	current->mFluids.push_back(fdesc);
  return true;
}
#endif

#if NX_USE_CLOTH_API
NxClothAttachDesc * NxuPhysicsExport::getClothShapeAttachment(NxShape *shape) // turn this into a serializable object.
{
	NxClothAttachDesc *ret =0;

	NxSceneDesc *current = getCurrentScene();
	CustomCopy cc(mCollection,current);

	if ( shape )
	{
		NxActor &act      = shape->getActor();
		addActor(&act);
		NxU32 nbShapes = act.getNbShapes();
		NxShape *const*slist = act.getShapes();
		for (NxU32 i=0; i<nbShapes; i++)
		{
			NxShape *s   = slist[i];
			if ( s == shape )
			{
				ret            = new NxClothAttachDesc;
				ret->mAttachActor      = cc.getNameFromActor(&act);
				ret->mAttachShapeIndex = i;
				break;
			}
		}
	}

  return ret;
}
#endif

#if NX_USE_SOFTBODY_API
NxSoftBodyAttachDesc * NxuPhysicsExport::getSoftBodyShapeAttachment(NxShape *shape) // turn this into a serializable object.
{
	NxSoftBodyAttachDesc *ret =0;

	NxSceneDesc *current = getCurrentScene();
	CustomCopy cc(mCollection,current);

	if ( shape )
	{
		NxActor &act      = shape->getActor();
		addActor(&act);
		NxU32 nbShapes = act.getNbShapes();
		NxShape *const*slist = act.getShapes();
		for (NxU32 i=0; i<nbShapes; i++)
		{
			NxShape *s   = slist[i];
			if ( s == shape )
			{
				ret            = new NxSoftBodyAttachDesc;
				ret->mAttachActor      = cc.getNameFromActor(&act);
				ret->mAttachShapeIndex = i;
				break;
			}
		}
	}

  return ret;
}
#endif

#if NX_USE_CLOTH_API
bool NxuPhysicsExport::Write(NxCloth *cloth,const char *userProperties,const char *id)
{
	bool ret = false;

	NxSceneDesc *current = getCurrentScene();
	CustomCopy cc(mCollection,current);

	NxClothDesc *desc = new NxClothDesc();
	desc->mUserProperties = getGlobalString(userProperties);

  if ( id )
  {
  	desc->mId = id;
  }
  else
  {
  	char scratch[512];
  	sprintf(scratch,"Cloth_%d", current->mCloths.size() );
  	desc->mId = getGlobalString(scratch);
  }

	::NxClothDesc cd;
	cloth->saveToDesc(cd);
	desc->mClothMesh = Write(cd.clothMesh,0 );

  desc->copyFrom(cd,cc);


#if NX_SDK_VERSION_NUMBER >= 260

	NxU32 acount = cloth->queryShapePointers();
	if ( acount )
	{
		NxShape **shapes = new NxShape*[acount];
		cloth->getShapePointers(shapes);
	  for (NxU32 i=0; i<acount; i++)
	  {
	  	NxClothAttachDesc *at =	getClothShapeAttachment(shapes[i]);
			desc->mAttachments.push_back(at);
	  }
		delete shapes;
	}

	if ( gUseClothActiveState )
	{
		MemoryWriteBuffer	wb;
		cloth->saveStateToStream(wb);
		const NxU8 *source = (const NxU8 *) wb.data;

		for (NxU32 i=0; i<wb.currentSize; i++)
		{
			NxU8 c = *source++;
			desc->mActiveState.push_back(c);
		}
	}
#endif

	current->mCloths.pushBack(desc);


  return ret;
}
#endif

#if NX_SDK_VERSION_NUMBER >= 260

const char * NxuPhysicsExport::Write(NxCompartment *c,const char *id)
{
	const char *ret = 0;

	NxSceneDesc *current = getCurrentScene();
	CustomCopy cc(mCollection,current);
	::NxCompartmentDesc desc;
	//TODO there should be a saveToDesc method for NxCompartment!
#if 0 // Not yet available
	c->saveToDesc(desc);
#else
	desc.type = c->getType();
	desc.deviceCode = c->getDeviceCode();
	desc.gridHashCellSize = c->getGridHashCellSize();
	desc.gridHashTablePower = c->gridHashTablePower();
#endif
	NxCompartmentDesc *ncd = new NxCompartmentDesc;

  if ( id )
  {
  	ret = ncd->mId = id;
  }
  else
  {
    char scratch[512];
    sprintf(scratch,"Compartment_%d", current->mCompartments.size() );
    ret = ncd->mId = getGlobalString(scratch);
  }
	ncd->mInstance = c;
  ncd->copyFrom(desc,cc);
  current->mCompartments.push_back(ncd);
	return ret;
}
#endif

bool NxuPhysicsExport::Write(NxPairFlag	*a,const char *userProperties,const char *id)
{
	NxSceneDesc *current = getCurrentScene();

	CustomCopy cc(mCollection,current);

	NxPairFlagDesc *pf = new NxPairFlagDesc;

  pf->mUserProperties = getGlobalString(userProperties);

  pf->mFlags = (NxContactPairFlag) a->flags;

  if ( id )
  {
  	pf->mId = id;
  }
  else
  {
    char scratch[512];
    sprintf(scratch,"PairFlag_%d", current->mPairFlags.size() );
    pf->mId = getGlobalString(scratch);
  }

	if (a->isActorPair())
	{
		pf->mIsActorPair = true;
		NxActor	*a0	=	(NxActor*)a->objects[0];
		NxActor	*a1	=	(NxActor*)a->objects[1];
		addActor(a0);
		addActor(a1);
		pf->mActor0 = cc.getNameFromActor(a0);
		pf->mActor1 = cc.getNameFromActor(a1);
		pf->mShapeIndex0 = 0;
		pf->mShapeIndex1 = 0;
	}
	else
	{
		pf->mIsActorPair = false;
		NxShape	*s0	=	(NxShape*)a->objects[0];
		NxShape	*s1	=	(NxShape*)a->objects[1];

		NxActor	*a0	=	&s0->getActor();
		NxActor	*a1	=	&s1->getActor();
		addActor(a0);
		addActor(a1);
		pf->mActor0 = cc.getNameFromActor(a0);
		pf->mActor1 = cc.getNameFromActor(a1);
		pf->mShapeIndex0 = GetShapeIndex(a0,	s0);
		pf->mShapeIndex1 = GetShapeIndex(a1,	s1);

	}

  current->mPairFlags.push_back(pf);

  return true;
}

bool NxuPhysicsExport::Write(NxMaterial	*a,const char *userProperties,const char *id)
{
	bool ret = false;

	NxSceneDesc *current = getCurrentScene();

	if ( current )
	{
		ret = true;
		::NxMaterialDesc d;
		a->saveToDesc(d);


		CustomCopy cc;
		NXU::NxMaterialDesc *desc = new NXU::NxMaterialDesc;

    if ( id )
    {
    	desc->mId = id;
    }
    else
    {
  		char scratch[512];
  		sprintf(scratch,"Material_%d", current->mMaterials.size() );
  		desc->mId = getGlobalString(scratch);
  	}

		desc->mUserProperties = getGlobalString(userProperties);
		desc->copyFrom(d,cc);

#if 1
    NxScene &scene = a->getScene();
    NxU32 nbMaterials = scene.getNbMaterials();

    for (NxU32 i=0; i<nbMaterials; i++)
    {
      NxMaterial *m = scene.getMaterialFromIndex(i);
      if ( m == a )
      {
        desc->mMaterialIndex = (NxU16)i;
        desc->mInstanceIndex = (NxU16)i;
        break;
      }
    }
#endif

		current->mMaterials.push_back(desc);
	}
  return ret;
}

bool NxuPhysicsExport::Write(NxActor *a,const char *userProperties,const char *id)
{
	bool ret = false;

	NxSceneDesc *scene = getCurrentScene();

	if ( scene )
	{
		ret = true;
		::NxActorDesc d;
		::NxBodyDesc b;
		a->saveToDesc(d);
		d.name = a->getName();

		NXU::NxActorDesc *desc = new NXU::NxActorDesc;
    if ( id )
    {
    	desc->mId = id;
    }
    else
    {
  		char scratch[512];
  		sprintf(scratch,"Actor_%d", scene->mActors.size() );
  		desc->mId = getGlobalString(scratch);
  	}

		desc->mUserProperties = getGlobalString(userProperties);

		CustomCopy cc(mCollection,scene);

		desc->copyFrom(d,cc);

		if ( a->saveBodyToDesc(b) )
		{
			desc->mHasBody = true;
			desc->mBody.copyFrom(b,cc);
			desc->density = 0;
		}
		else
		{
			desc->mHasBody = false;
		}

    if ( desc->mHasBody ) 
		{
			if ( desc->mBody.mass != 0.0f )
				assert( desc->density == 0.0f );
			if ( desc->density != 0.0f )
				assert( desc->mBody.mass == 0.0f );
		}

		desc->mInstance = a;

    //ok..now time to copy the shapes
    NxU32 nb = a->getNbShapes();

  	NxShape	*const* actorShapes = a->getShapes();

	  for	(NxU32 k = 0;	k	<	nb; k++)
	  {
		  const NxShape *shape = actorShapes[k];

      NxShapeDesc *save = 0;

      switch ( shape->getType() )
      {
        case NX_SHAPE_PLANE:
          if ( 1 )
          {
            const NxPlaneShape *p = shape->isPlane();
            assert(p);
            ::NxPlaneShapeDesc d;
            p->saveToDesc(d);
            NxPlaneShapeDesc *sd = new NxPlaneShapeDesc;
            sd->copyFrom(d,cc);
            save = static_cast<NxShapeDesc *>(sd);
            save->mType = SC_NxPlaneShapeDesc;
            if ( d.ccdSkeleton )
            {
              sd->mCCDSkeleton = Write(d.ccdSkeleton,0);
            }
          }
          break;
        case NX_SHAPE_SPHERE:
          if ( 1 )
          {
            const NxSphereShape *p = shape->isSphere();
            assert(p);
            ::NxSphereShapeDesc d;
            p->saveToDesc(d);
            NxSphereShapeDesc *sd = new NxSphereShapeDesc;
            sd->copyFrom(d,cc);
            save = static_cast<NxShapeDesc *>(sd);
            save->mType = SC_NxSphereShapeDesc;
            if ( d.ccdSkeleton )
            {
              sd->mCCDSkeleton = Write(d.ccdSkeleton,0);
            }
          }
          break;
        case NX_SHAPE_BOX:
          if ( 1 )
          {
            const NxBoxShape *p = shape->isBox();
            assert(p);
            ::NxBoxShapeDesc d;
            p->saveToDesc(d);
            NxBoxShapeDesc *sd = new NxBoxShapeDesc;
            sd->copyFrom(d,cc);
            save = static_cast<NxShapeDesc *>(sd);
            save->mType = SC_NxBoxShapeDesc;
            if ( d.ccdSkeleton )
            {
              sd->mCCDSkeleton = Write(d.ccdSkeleton,0);
            }
          }
          break;
        case NX_SHAPE_CAPSULE:
          if ( 1 )
          {
            const NxCapsuleShape *p = shape->isCapsule();
            assert(p);
            ::NxCapsuleShapeDesc d;
            p->saveToDesc(d);
            NxCapsuleShapeDesc *sd = new NxCapsuleShapeDesc;
            sd->copyFrom(d,cc);
            save = static_cast<NxShapeDesc *>(sd);
            save->mType = SC_NxCapsuleShapeDesc;
            if ( d.ccdSkeleton )
            {
              sd->mCCDSkeleton = Write(d.ccdSkeleton,0);
            }
          }
          break;
        case NX_SHAPE_WHEEL:
          if ( 1 )
          {
            const NxWheelShape *p = shape->isWheel();
            assert(p);
            ::NxWheelShapeDesc d;
            p->saveToDesc(d);
            NxWheelShapeDesc *sd = new NxWheelShapeDesc;
            sd->copyFrom(d,cc);
            save = static_cast<NxShapeDesc *>(sd);
            save->mType = SC_NxWheelShapeDesc;
            if ( d.ccdSkeleton )
            {
              sd->mCCDSkeleton = Write(d.ccdSkeleton,0);
            }
          }
          break;
        case NX_SHAPE_CONVEX:
          if ( 1 )
          {
            const NxConvexShape *p = shape->isConvexMesh();
            assert(p);
            ::NxConvexShapeDesc d;
            p->saveToDesc(d);

				    if	(d.meshData)
				    {
  					  Write(d.meshData,0);
	    			}


            NxConvexShapeDesc *sd = new NxConvexShapeDesc;
            sd->copyFrom(d,cc);
            save = static_cast<NxShapeDesc *>(sd);
            save->mType = SC_NxConvexShapeDesc;
            if ( d.ccdSkeleton )
            {
              sd->mCCDSkeleton = Write(d.ccdSkeleton,0);
            }
          }
          break;
        case NX_SHAPE_MESH:
          if ( 1 )
          {
            const NxTriangleMeshShape *p = shape->isTriangleMesh();
            assert(p);
            ::NxTriangleMeshShapeDesc d;
            p->saveToDesc(d);
            if ( d.meshData )
            {
              Write(d.meshData,0);
            }
            NxTriangleMeshShapeDesc *sd = new NxTriangleMeshShapeDesc;
            sd->copyFrom(d,cc);
            save = static_cast<NxShapeDesc *>(sd);
            save->mType = SC_NxTriangleMeshShapeDesc;
            if ( d.ccdSkeleton )
            {
              sd->mCCDSkeleton = Write(d.ccdSkeleton,0);
            }
          }
          break;
        case NX_SHAPE_HEIGHTFIELD:
          if ( 1 )
          {
            const NxHeightFieldShape *p = shape->isHeightField();
            assert(p);
            ::NxHeightFieldShapeDesc d;
            p->saveToDesc(d);
            if ( d.heightField )
            {
              Write(d.heightField,0);
            }
            NxHeightFieldShapeDesc *sd = new NxHeightFieldShapeDesc;
            sd->copyFrom(d,cc);
            save = static_cast<NxShapeDesc *>(sd);
            save->mType = SC_NxHeightFieldShapeDesc;
            if ( d.ccdSkeleton )
            {
              sd->mCCDSkeleton = Write(d.ccdSkeleton,0);
            }
          }
          break;
      }

      save->mInstance = (void *)shape;

      desc->mShapes.push_back(save);


  	}

		scene->mActors.push_back(desc);

	}


  return ret;
}


bool NxuPhysicsExport::setFilterOps(bool filter,::NxFilterOp op0,::NxFilterOp op1,::NxFilterOp op2,const ::NxGroupsMask &m1,const ::NxGroupsMask &m2)
{
	bool ret = false;

	NxSceneDesc *scene = getCurrentScene();

  if ( scene )
  {
		CustomCopy cc(mCollection);
  	scene->mFilterBool = filter;
  	scene->mFilterOp0  = (NxFilterOp) op0;
  	scene->mFilterOp1  = (NxFilterOp) op1;
  	scene->mFilterOp2  = (NxFilterOp) op2;
  	scene->mGroupMask0.copyFrom(m1,cc);
  	scene->mGroupMask1.copyFrom(m2,cc);
  	scene->mHasFilter = true;
  }

  return ret;
}


bool NxuPhysicsExport::addGroupCollisionFlag(NxU32 group1,NxU32 group2,bool enable)
{
	bool ret = false;

	NxSceneDesc *scene = getCurrentScene();

  if ( scene )
  {
  	NxCollisionGroupDesc *gc = new NxCollisionGroupDesc;

  	gc->mCollisionGroupA = group1;
  	gc->mCollisionGroupB = group2;
  	gc->mEnable = enable;

  	scene->mCollisionGroups.push_back(gc);

  	ret = true;
  }

  return ret;
}

bool NxuPhysicsExport::addActorGroupFlag(NxU32 group0,NxU32 group1,NxU32 flags)
{
	bool ret = false;

  NxSceneDesc *scene = getCurrentScene();

  if ( scene )
  {
  	NxActorGroupPair *ag = new NxActorGroupPair;
  	ag->group0 = (NxU16) group0;
  	ag->group1 = (NxU16) group1;
  	ag->flags  = (NxContactPairFlag) flags;
  	scene->mActorGroups.push_back(ag);
  }

  return ret;
}


bool NxuPhysicsExport::addSceneInstance(const char *name,const char *sceneName,const NxMat34 &rootNode,bool ignorePlane,const char *userProperties)
{
	bool ret = false;

  NxSceneInstanceDesc *i = new NxSceneInstanceDesc;
	i->mSceneName = getGlobalString(sceneName);
	i->mId          = getGlobalString(name);
	i->mParent        = 0;
  i->mRootNode 			= rootNode;
  i->mIgnorePlane	 	= ignorePlane;
  i->mUserProperties = getGlobalString(userProperties);

	if ( mCollection )
	{
		if ( mCollection->mCurrentSceneInstance )
		{
			i->mParent = mCollection->mCurrentSceneInstance;
			mCollection->mCurrentSceneInstance->mSceneInstances.push_back(i);
		}
		else
		{
			mCollection->mSceneInstances.push_back(i);
		}
		ret = true;
	}

  return ret;
}

bool NxuPhysicsExport::addToCurrentSceneInstance(const char *name,const char *sceneName,const NxMat34 &rootNode,bool ignorePlane,const char *userProperties)
{
	bool ret = false;

  NxSceneInstanceDesc *i = new NxSceneInstanceDesc;
	i->mSceneName = getGlobalString(sceneName);
	i->mId          = getGlobalString(name);
	i->mParent        = 0;
  i->mRootNode 			= rootNode;
  i->mIgnorePlane	 	= ignorePlane;
  i->mUserProperties = getGlobalString(userProperties);

	if ( mCollection )
	{
		NxSceneInstanceDesc *parent = mCollection->mCurrentSceneInstance;
		i->mParent = parent;
		if ( parent )
			parent->mSceneInstances.push_back(i);
		else
		{
			mCollection->mSceneInstances.push_back(i);
		}
		mCollection->mCurrentSceneInstance = i;
		ret = true;
	}

  return ret;
}


NxSceneDesc * NxuPhysicsExport::getCurrentScene(void)
{
	NxSceneDesc *ret = 0;

  if ( mCollection->mCurrentScene )
  {
  	ret = mCollection->mCurrentScene;
  }
  else
  {
  	ret = mCollection->mCurrentScene = new NxSceneDesc;
  	char scratch[512];
  	sprintf(scratch,"Scene_%d", mCollection->mScenes.size() );
  	ret->mId = getGlobalString(scratch);
  	mCollection->mScenes.push_back(ret);
  }

  return ret;
}


bool NxuPhysicsExport::addActor(NxActor *a) // if this actor doesn't already exist, add it.
{
	bool ret = false;

  if ( a )
  {
  	bool found = false;

  	NxSceneDesc *scene = getCurrentScene();
  	for (unsigned int i=0; i<scene->mActors.size(); i++)
    {
    	NxActorDesc *ad = scene->mActors[i];
    	if ( ad->mInstance == a )
    	{
    		found = true;
    		break;
    	}
    }
    if ( !found )
    {
    	ret = true;
    	Write(a,0);
    }
  }

  return ret;
}


#if NX_USE_SOFTBODY_API
bool NxuPhysicsExport::Write(NxSoftBody *softBody,const char *userProperties,const char *id)
{
	bool ret = false;

	NxSceneDesc *current = getCurrentScene();
	CustomCopy cc(mCollection,current);

	NxSoftBodyDesc *desc = new NxSoftBodyDesc();
	desc->mUserProperties = getGlobalString(userProperties);

  if ( id )
  {
  	desc->mId = id;
  }
  else
  {
  	char scratch[512];
  	sprintf(scratch,"SoftBody_%d", current->mSoftBodies.size() );
  	desc->mId = getGlobalString(scratch);
  }

	::NxSoftBodyDesc cd;
	softBody->saveToDesc(cd);
	desc->mSoftBodyMesh = Write(cd.softBodyMesh,0 );

  desc->copyFrom(cd,cc);

	NxU32 acount = softBody->queryShapePointers();
	if ( acount )
	{
		NxShape **shapes = new NxShape*[acount];
		softBody->getShapePointers(shapes);
	  for (NxU32 i=0; i<acount; i++)
	  {
	  	NxSoftBodyAttachDesc *at =	getSoftBodyShapeAttachment(shapes[i]);
			desc->mAttachments.push_back(at);
	  }
		delete shapes;
	}

	if ( gUseClothActiveState )
	{
		MemoryWriteBuffer	wb;
		softBody->saveStateToStream(wb);
		const NxU8 *source = (const NxU8 *) wb.data;

		for (NxU32 i=0; i<wb.currentSize; i++)
		{
			NxU8 c = *source++;
			desc->mActiveState.push_back(c);
		}
	}

	current->mSoftBodies.pushBack(desc);


  return ret;
}

const char * NxuPhysicsExport::Write(NxSoftBodyMesh *softBodyMesh,const char *id)
{
	const char *ret = 0;

	if ( softBodyMesh )
	{

		for (NxU32 i=0; i<mCollection->mSoftBodyMeshes.size(); i++)
		{
			NxSoftBodyMeshDesc *c = mCollection->mSoftBodyMeshes[i];
			if ( c->mInstance == softBodyMesh )
			{
				ret = c->mId;
				break;
			}
		}

		if ( !ret )
		{

			::NxSoftBodyMeshDesc desc;
 			softBodyMesh->saveToDesc(desc);
			NxU32	numMesh	=	mCollection->mSoftBodyMeshes.size();
    	NxSoftBodyMeshDesc *cMesh	=	new NxSoftBodyMeshDesc;

      if ( id )
      {
      	cMesh->mId = id;
      }
      else
      {
  			char tempString[512];
  			sprintf(tempString,	"SoftBodyMesh_%d", numMesh);
      	cMesh->mId = getGlobalString(tempString);
      }

    	CustomCopy cc(mCollection,0);

      cMesh->copyFrom(desc,cc);

			cMesh->mInstance = softBodyMesh;

			mCollection->mSoftBodyMeshes.pushBack(cMesh);

			ret = cMesh->mId;
		}

	}

  return ret;
}

#endif




};

//AGCOPYRIGHTBEGIN
///////////////////////////////////////////////////////////////////////////
// Copyright (c) 2005	AGEIA	Technologies.
// All rights	reserved.	www.ageia.com
///////////////////////////////////////////////////////////////////////////
//AGCOPYRIGHTEND
