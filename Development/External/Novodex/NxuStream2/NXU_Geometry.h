#ifndef	NXU_GEOMETRY_H

#define	NXU_GEOMETRY_H


class	NxMat34;
class	NxVec3;

namespace	NXU
{

class	NxuGeometry
{
	 public:
		NxuGeometry(void)
		{
			mVcount	=	0;
			mVertices	=	0;
			mTcount	=	0;
			mIndices = 0;
		}
		~NxuGeometry(void)
		{
			reset();
		}

		void reset(void)
		{
			if (mVertices)
			{
					delete mVertices;
			}
			if (mIndices)
			{
					delete mIndices;
			}
			mVcount	=	0;
			mVertices	=	0;
			mTcount	=	0;
			mIndices = 0;
		}

		unsigned int mVcount;
		float	*mVertices;
		unsigned int mTcount;
		unsigned int *mIndices;
};


bool createBox(NxVec3	&dimensions, const NxMat34 *localPose, NxuGeometry &g,float shrink,unsigned int maxV);
bool createSphere(float	radius,	int	slices,	const	NxMat34	*localPose,	NxuGeometry	&g,float shrink,unsigned int maxV);
bool createCapsule(float radius, float height, int axis, int slices, const NxMat34 *localPose, NxuGeometry &g,float shrink,unsigned int maxV);
bool createHull(unsigned int vcount, const float *points,const NxMat34 *localPose, NxuGeometry &g,float shrink,unsigned int maxV);


}

/**************************************************************************
 *																		*
 **************************************************************************/

#endif
