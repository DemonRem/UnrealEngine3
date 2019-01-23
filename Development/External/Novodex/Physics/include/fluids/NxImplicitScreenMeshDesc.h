#ifndef NX_FLUIDS_NX_IMPLICIT_SCREEN_MESH_DESC
#define NX_FLUIDS_NX_IMPLICIT_SCREEN_MESH_DESC
/** \addtogroup fluids
  @{
*/
/*----------------------------------------------------------------------------*\
|
|						Public Interface to Ageia PhysX Technology
|
|							     www.ageia.com
|
\*----------------------------------------------------------------------------*/

#include "fluids/NxImplicitScreenMesh.h"
#include "fluids/NxMeshData.h"

/*********************************************************************************
 * Added for Fast Fluid Surface Generation (Unique ID: FAST_FLUID_SURFACE_GENERATION)
 * 2006.1.20 in St.Louis
 *********************************************************************************/

/**
\brief Specifies the parameters for screen mesh generation.

\warning <b>Preliminary API, subject to change</b>

@see NxScreenMesh
*/
class NxImplicitScreenMeshDesc
{
public:

	/**
	\brief The spacing of the 2D map.
	
	The larger this spacing is set, the coarser the surface gets and the less resolution of the density & depth map 
	has.
	
	This parameter has a large impact on the performance of surface generation. 

	\warning <b>Preliminary API, subject to change</b>
	*/
	NxReal		screenSpacing;

	/**
	\brief The hierarchy level used to control the resolution of mesh generated.
	Currently only available in SW.
	
	This parameter has a large impact on the performance of surface generation. 

	\warning <b>Preliminary API, subject to change</b>
	*/
	NxU32		hierarchyLevels;

	/**
	\brief 	Specifies input particle's radius.
	
	Smaller values produce less bumpy surfaces which have more temporal discontinuities (popping).

	\warning <b>Preliminary API, subject to change</b>
	*/
	NxReal		particleRadius;

	/**
	\brief 	Specifies the minimum density that generates a surface.
	
	Smaller values produce more triangles where the fluid is thinner.

	\warning <b>Preliminary API, subject to change</b>
	*/
	NxReal		threshold;

	/**
	\brief Sets the depth smoothing filter size.
	
	Larger values produces smoother surface.

	\warning <b>Preliminary API, subject to change</b>
  	*/
	NxU32		depthSmoothing;

	/**
	\brief Sets the silhouette smoothing filter size.
	
	Larger values gives a smoother silhouette mesh.

	\warning <b>Preliminary API, subject to change</b>
	*/
	NxU32	    silhouetteSmoothing;

	
	/**
	\brief Sets the viewport width from outside rendering.
	
	\warning <b>Preliminary API, subject to change</b>
	*/
	NxU32	width;
	/**
	\brief Sets the viewport height from outside rendering.

	\warning <b>Preliminary API, subject to change</b>
	*/
	NxU32	height;

	/**
	\brief Sets the projection matrix from outside rendering.

	the layout is as follows (same as the matrix in OpenGL):
	<pre>
M =	| 0  4  8  12 |
	| 1  5  9  13 |
	| 2  6 10  14 |
	| 3  7 11  15 |</pre>
	    
	\warning <b>Preliminary API, subject to change</b>
	*/
	NxReal*	projectionMatrix;			


	/**
	\brief Sets the triangle winding to get a more regular triangulation.

	\warning <b>Preliminary API, subject to change</b>
	*/
	NxU8	triangleWindingPositive;

	/**
	\brief Defines how the generated mesh is written to user mesh buffers.

	\warning <b>Preliminary API, subject to change</b>

	@see NxMeshData
	*/
	NxMeshData	meshData;

	void*	    userData;		//!< Will be copied to NxImplicitScreenMesh::userData.
	const char*	name;			//!< Possible debug name. The string is not copied by the SDK, only the pointer is stored.

	NX_INLINE ~NxImplicitScreenMeshDesc();
	/**
	\brief (Re)sets the structure to the default.	

	\warning <b>Preliminary API, subject to change</b>
	*/
	NX_INLINE void setToDefault();

	/**
	\brief Returns true if the current settings are valid

	\warning <b>Preliminary API, subject to change</b>
	*/
	NX_INLINE bool isValid() const;

	/**
	\brief Constructor sets to default.

	\warning <b>Preliminary API, subject to change</b>
	*/
	NX_INLINE	NxImplicitScreenMeshDesc();
};


NX_INLINE NxImplicitScreenMeshDesc::NxImplicitScreenMeshDesc()
{
	setToDefault();
}

NX_INLINE NxImplicitScreenMeshDesc::~NxImplicitScreenMeshDesc()
{
}

NX_INLINE void NxImplicitScreenMeshDesc::setToDefault()
{
    triangleWindingPositive = 1;
	screenSpacing       = 5.0f;
    threshold           = 0.5f;
	particleRadius      = 0.05f;
	hierarchyLevels     = 1;
	depthSmoothing      = 0;
	silhouetteSmoothing = 0;
	width               = 512;
	height              = 512;

	projectionMatrix    = NULL;
	meshData						.setToDefault();
	
	meshData.verticesPosByteStride    = sizeof(float)*3;
	meshData.verticesNormalByteStride = sizeof(float)*3;
	meshData.indicesByteStride        = sizeof(int);

	userData						= NULL;
	name							= NULL;
}

NX_INLINE bool NxImplicitScreenMeshDesc::isValid() const
{
	/* Data range */
	if (screenSpacing  <= 0.0f)		return false;
	if (screenSpacing  >10.0f)      return false;
	if (particleRadius <= 0.0f)		return false;
	if (hierarchyLevels < 1)		return false;
	if (hierarchyLevels > 10)		return false;
	if (depthSmoothing < 0)		    return false;
	if (depthSmoothing > 10)	    return false; //SW has this limitation
	if (silhouetteSmoothing < 0)    return false;
	if (silhouetteSmoothing > 10)   return false; //SW has this limitation
	if (width <= 0.0f)				return false;
	if (height <= 0.0f)				return false;
	if (projectionMatrix == NULL)   return false;

    // Max grid resolution
    NxU32 gridW = (NxU32)ceil((width / screenSpacing)) + 1;
    NxU32 gridH = (NxU32)ceil((height/ screenSpacing)) + 1;

	/* For HW & SW, we have different resolution problem */
	/* For HW, this should be less than 256???           */
    // This is clamped in NfImplicitScreenMeshPPUInterfaceHw::controllerInitializeParameters(), so here we don't process it.
	//if ( gridW > 512) return false;
    //if ( gridH > 512) return false;

	if (!meshData.isValid()) return false;
	
    // is mesh large enough?
    NxU32 maxVerts = (NxU32)(gridW * gridH * 2.5);
    NxU32 maxIndices = gridW * gridH * 3*3;
    if (meshData.maxVertices < maxVerts) return false;
    if (meshData.maxIndices < maxIndices) return false;

	return true;
}

/** @} */
#endif
