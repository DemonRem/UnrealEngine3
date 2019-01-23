#ifndef CONVEX_DECOMPOSITION_H

#define CONVEX_DECOMPOSITION_H

namespace ConvexDecomposition
{

class ConvexResult
{
public:
  ConvexResult(void)
  {
    mHullVcount = 0;
    mHullVertices = 0;
    mHullTcount = 0;
    mHullIndices = 0;
  }

  ConvexResult(unsigned int hvcount,const float *hvertices,unsigned int htcount,const unsigned int *hindices)
  {
    mHullVcount = hvcount;
    if ( mHullVcount )
    {
      mHullVertices = new float[mHullVcount*sizeof(float)*3];
      memcpy(mHullVertices, hvertices, sizeof(float)*3*mHullVcount );
    }
    else
    {
      mHullVertices = 0;
    }

    mHullTcount = htcount;

    if ( mHullTcount )
    {
      mHullIndices = new unsigned int[sizeof(unsigned int)*mHullTcount*3];
      memcpy(mHullIndices,hindices, sizeof(unsigned int)*mHullTcount*3 );
    }
    else
    {
      mHullIndices = 0;
    }

  }

  ConvexResult(const ConvexResult &r)
  {
    mHullVcount = r.mHullVcount;
    if ( mHullVcount )
    {
      mHullVertices = new float[mHullVcount*sizeof(float)*3];
      memcpy(mHullVertices, r.mHullVertices, sizeof(float)*3*mHullVcount );
    }
    else
    {
      mHullVertices = 0;
    }
    mHullTcount = r.mHullTcount;
    if ( mHullTcount )
    {
      mHullIndices = new unsigned int[sizeof(unsigned int)*mHullTcount*3];
      memcpy(mHullIndices, r.mHullIndices, sizeof(unsigned int)*mHullTcount*3 );
    }
    else
    {
      mHullIndices = 0;
    }
  }

  ~ConvexResult(void)
  {
    delete mHullVertices;
    delete mHullIndices;
  }

// the convex hull.
  unsigned int		    mHullVcount;
  float *						  mHullVertices;
  unsigned  int       mHullTcount;
  unsigned int			 *mHullIndices;



};

class ConvexDecompInterface
{
public:

	virtual void ConvexDebugTri(const float *p1,const float *p2,const float *p3,unsigned int color) { };
	virtual void ConvexDebugPoint(const float *p,float dist,unsigned int color) { };
  virtual void ConvexDebugBound(const float *bmin,const float *bmax,unsigned int color) { };

  virtual void ConvexDecompResult(ConvexResult &result) = 0;



};

// just to avoid passing a zillion parameters to the method the
// options are packed into this descriptor.
class DecompDesc
{
public:
  DecompDesc(void)
  {
  	mVcount = 0;
  	mVertices = 0;
  	mTcount   = 0;
  	mIndices  = 0;
  	mDepth    = 5;
  	mCpercent = 5;
  	mPpercent = 5;
  	mMaxVertices = 32;
  	mSkinWidth   = 0;
  	mCallback    = 0;
  }

// describes the input triangle.
  unsigned	int	mVcount;   // the number of vertices in the source mesh.
  const float  *mVertices; // start of the vertex position array.  Assumes a stride of 3 floats.
  unsigned int  mTcount;   // the number of triangles in the source mesh.
  unsigned int *mIndices;  // the indexed triangle list array (zero index based)

// options
  unsigned int  mDepth;    // depth to split, a maximum of 10, generally not over 7.
  float         mCpercent; // the concavity threshold percentage.  0=20 is reasonable.
	float         mPpercent; // the percentage volume conservation threshold to collapse hulls. 0-30 is reasonable.

// hull output limits.
  unsigned int  mMaxVertices; // maximum number of vertices in the output hull. Recommended 32 or less.
  float         mSkinWidth;   // a skin width to apply to the output hulls.

	ConvexDecompInterface *mCallback; // the interface to receive back the results.

};

// perform approximate convex decomposition on a mesh.
unsigned int performConvexDecomposition(const DecompDesc &desc); // returns the number of hulls produced.

};

#endif
