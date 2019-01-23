#ifndef NX_FLUIDS_NX_IMPLICIT_SCREEN_MESH
#define NX_FLUIDS_NX_IMPLICIT_SCREEN_MESH
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
#include "Nxp.h"
#include "NxPhysicsSDK.h"

/*********************************************************************************
 * Added for Fast Fluid Surface Generation (Unique ID: FAST_FLUID_SURFACE_GENERATION)
 * 2006.1.20 in St.Louis
 *********************************************************************************/

class NxMeshData;
class NxParticleData;
class NxImplicitScreenMeshDesc;

/**
\brief The fast fluid surface mesh class is used to generate a fluid surface mesh or a
implicit mesh defined by a set of user points.

The mesh generation works as follows:

Each point or particle is projected to the current view port to get the 2D density and depth map,
and then sample this 2D view-dependent map to generate the triangles. 
The user mesh buffers (indices and vertices) must be specified using the 
NxMeshData wrapper in order to generate a mesh.
If particles are specified using setParticles(), the screen mesh
for these particles is generated.
If the Fluid data mesh is owned by an NxFluid instance, a mesh is 
generated only for the fluid, and the particles specified with setParticles()
are ignored.

\warning <b>Preliminary API, subject to change</b>
*/
class NxImplicitScreenMesh
{
protected:
	NX_INLINE						NxImplicitScreenMesh() : userData(NULL)	{}
	virtual							~NxImplicitScreenMesh()	{}

public:

	/**
	\brief Sets particles which are used for this screen mesh generation.
	
	This is done by passing an NxParticleData user buffer wrapper. The specified buffers are only 
	read until the function returns. Calling this method when the NxImplicitScreenMesh 
	instance is owned by an NxFluid instance has no effect. 

	\warning <b>Preliminary API, subject to change</b>

	\param[in] particleData Structure describing the particles to use for mesh generation.

	@see NxParticleData
	*/
	virtual		void 				setParticles(NxParticleData& particleData) const = 0;

	/**
	\brief Sets the user buffer wrapper for the mesh.

	\warning <b>Preliminary API, subject to change</b>

	\param[in] meshData User buffer wrapper.

	@see NxMeshData
	*/
	virtual		void 				setMeshData(NxMeshData& meshData) = 0;

	/**
	\brief Returns a copy of the user buffer wrapper for the mesh.

  This returns the buffer wrapper only, does not copy a new surface
  into the buffer.

	\warning <b>Preliminary API, subject to change</b>

	\return User buffer wrapper.

	@see NxMeshData
	*/
	virtual		NxMeshData			getMeshData() = 0;
	
  /**
	\brief Begin generation of a new surface

	\warning <b>Preliminary API, subject to change</b>

	\return True if surface generation began successfully.

	@see NxMeshData
	*/
  virtual		bool			generateMesh() = 0;

  /**
	\brief Copy mesh into buffers defined by NxMeshData

  If mesh generation is not complete, this will block.

	\warning <b>Preliminary API, subject to change</b>

	\return True if surface generation completed successfully.

	@see NxMeshData
	*/
	virtual		bool			getMesh() = 0;
  
  /**
	\brief Checks whether surface generation has completed

	\warning <b>Preliminary API, subject to change</b>

	\return True if surface generation is complete.

	@see NxMeshData
	*/
	virtual		bool			generateMeshIsDone() const = 0;

	/**
	\brief Returns the spacing of the 2D map.

	\warning <b>Preliminary API, subject to change</b>

	\return Surface spacing.

	@see NxImplicitScreenMeshDesc
	*/
	virtual		NxReal				getScreenSpacing() const = 0;

	/**
	\brief Sets the spacing of the 2D map.

	\warning <b>Preliminary API, subject to change</b>

	\param[in] spacing The new spacing.

	@see NxImplicitScreenMeshDesc
	*/
	virtual		void 				setScreenSpacing(NxReal spacing) = 0;

    /**
	\brief Returns the minimum density that generates a surface.

	\warning <b>Preliminary API, subject to change</b>

	\return The threshold parameter of the implicit mesh.

	@see NxImplicitScreenMeshDesc
	*/
	virtual		NxReal				getThreshold() const = 0;

	/**
	\brief Sets the minimum density that generates a surface.

	\warning <b>Preliminary API, subject to change</b>

	\param[in] threshold The minimum density that generates a surface.

	@see NxImplicitScreenMeshDesc
	*/
	virtual		void 				setThreshold(NxReal threshold) = 0;

	/**
	\brief Gets the hierarchy level of the triangle mesh generation, currently existing in Software mode.	

	\warning <b>Preliminary API, subject to change</b>

	\return The minimal distance of the mesh to the particles.

	@see NxImplicitScreenMeshDesc
	*/
	virtual		NxU32				getHierarchyLevels() const = 0;

	/**
	\brief Sets the hierarchy level of the triangle mesh generation, currently existing in Software mode.	

	\warning <b>Preliminary API, subject to change</b>

	\param[in] hierarchyLevels The hierarchy Level.

	@see NxImplicitScreenMeshDesc
	*/
	virtual		void 				setHierarchyLevels(NxU32 hierarchyLevels) = 0;

	/**
	\brief Returns the particles' radius.

	\warning <b>Preliminary API, subject to change</b>

	\return The blend parameter of the implicit mesh.

	@see NxImplicitScreenMeshDesc
	*/
	virtual		NxReal				getParticleRadius() const = 0;

	/**
	\brief Sets the particles' radius.

	\warning <b>Preliminary API, subject to change</b>

	\param[in] particleRadius The input particle's radius.

	@see NxImplicitScreenMeshDesc
	*/
	virtual		void 				setParticleRadius(NxReal particleRadius) = 0;

	/**
	\brief Returns the depth smoothing filter size.

	\warning <b>Preliminary API, subject to change</b>

	\return The depth smoothing filter size.

	@see NxImplicitScreenMeshDesc
	*/
	virtual		NxU32				getDepthSmoothing() const = 0;

	/**
	\brief Sets the depth smoothing filter size.

	\warning <b>Preliminary API, subject to change</b>

	\param[in] smoothing The input depth smoothing filter size.

	@see NxImplicitScreenMeshDesc
	*/
	virtual		void 				setDepthSmoothing(NxU32 smoothing) = 0;

	/**
	\brief Returns the silhouette smoothing filter size.

	\warning <b>Preliminary API, subject to change</b>

	\return The silhouette smoothing filter size.

	@see NxImplicitScreenMeshDesc
	*/
	virtual		NxU32				getSilhouetteSmoothing() const = 0;

	/**
	\brief Sets the silhouette smoothing filter size.

	\warning <b>Preliminary API, subject to change</b>

	\param[in] smoothing The input silhouette smoothing filter size.

	@see NxImplicitScreenMeshDesc
	*/
	virtual		void 				setSilhouetteSmoothing(NxU32 smoothing) = 0;

	/**
	\brief Sets the projection parameters.

	\warning <b>Preliminary API, subject to change</b>

	\param[in] projection44 The projection matrix.
	           width        The rendering window's width
			   height       The rendering window's height

	@see NxImplicitScreenMeshDesc
	*/
	virtual		void 				setProjectionMatrix(const NxReal *projection44) = 0;

	/**
	\brief Gets the projection parameters.

	\warning <b>Preliminary API, subject to change</b>

	\return The projection matrix.

	@see NxImplicitScreenMeshDesc
	*/	
	virtual		const NxReal*		getProjectionMatrix() const = 0;

	/**
	\brief Sets the projection parameters.

	\warning <b>Preliminary API, subject to change</b>

	\param[in] width        The rendering window's width
			   
	@see NxImplicitScreenMeshDesc
	*/
	virtual		void 				setWindowWidth(NxU32 width) = 0;

	/**
	\brief Gets the projection parameters.

	\warning <b>Preliminary API, subject to change</b>

	\return The window width.

	@see NxImplicitScreenMeshDesc
	*/	
	virtual		NxU32				getWindowWidth() const = 0;

	/**
	\brief Sets the projection parameters.

	\warning <b>Preliminary API, subject to change</b>

	\param[in] height        The rendering window's height
			   
	@see NxImplicitScreenMeshDesc
	*/
	virtual		void 				setWindowHeight(NxU32 height) = 0;

	/**
	\brief Gets the projection parameters.

	\warning <b>Preliminary API, subject to change</b>

	\return The window height.

	@see NxImplicitScreenMeshDesc
	*/	
	virtual		NxU32				getWindowHeight() const = 0;

	/**
	\brief Sets the triangle winding parameter.

	\warning <b>Preliminary API, subject to change</b>

	\param[in] positive The triangle winding parameter.
	@see NxImplicitScreenMeshDesc
	*/
	virtual		void 				setTriangleWinding(NxU8 positive) = 0;

	/**
	\brief Gets the triangle winding parameter.

	\warning <b>Preliminary API, subject to change</b>

	\return The triangle winding parameter.
	@see NxImplicitScreenMeshDesc
	*/
	virtual		NxU8 				getTriangleWinding() const= 0;
	
	/**
	\brief Creates the triangles based on the input parameters.

	\warning <b>Preliminary API, subject to change</b>

	\param[in] numParticles The number of particles.
	           particles    The input particles.

	@see NxImplicitScreenMeshDesc
	*/
	//virtual	void  buildSurface(	NxU32 numParticles,const NxF32 *particles, NxU32 particleByteStride,const NxF32 *densities, NxU32 densityByteStride) =0;
	
	/**
	\brief Sets a name string for the object that can be retrieved with getName().  
	
	This is for debugging and is not used by the SDK.  The string is not copied by the SDK; 
	only the pointer is stored. 

	\warning <b>Preliminary API, subject to change</b>

	\param[in] name The new name.
	*/
	virtual	void			setName(const char* name)		= 0;

	/**
	\brief Retrieves the name string set with setName().

	\warning <b>Preliminary API, subject to change</b>
	*/
	virtual	const char*		getName()			const	= 0;

	//public variables:
	void*					userData;	//!< user can assign this to whatever, usually to create a 1:1 relationship with a user object.
};
/** @} */
#endif
