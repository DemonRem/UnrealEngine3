#ifndef NX_FLUIDS_NXPARTICLEDATA
#define NX_FLUIDS_NXPARTICLEDATA
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

/**
\brief Descriptor-like user-side class describing a set of fluid particles.

NxParticleData is used to submit particles to the simulation and
to retrieve information about particles in the simulation. 

Each particle is described by its position, velocity, lifetime, density, and a set of (NxParticleFlag) flags.
The memory for the particle data is allocated by the application,
making this class a "user buffer wrapper".
*/
class NxParticleData
	{
	public:

	/**
	\brief Specifies the maximal number of particle elements the buffers can hold.
	*/
	NxU32					maxParticles;
    
	/**
	\brief Points to the user-allocated memory holding the number of elements stored in the buffers. 
	
	If the SDK writes to a given particle buffer, it also sets the numbers of elements written. If 
	this is set to NULL, the SDK can't write to the given buffer.
	*/
	NxU32*					numParticlesPtr;

	/**
	\brief The pointer to the user-allocated buffer for particle positions.

	A position consists of three consecutive 32-bit floats. If set to NULL, positions are not read or written to.
	*/
	NxF32*					bufferPos;
	
	/**
	\brief The pointer to the user-allocated buffer for particle velocities.

	A velocity consists of three consecutive 32-bit 
	floats. If set to NULL, velocities are not read or written to.
	*/
	NxF32*					bufferVel;
	
	/**
	\brief The pointer to the user-allocated buffer for particle lifetimes.

	A particle lifetime consists of one 32-bit 
	float. If set to NULL, lifetimes are not read or written to.
	*/
	NxF32*					bufferLife;
    
	/**
	\brief The pointer to the user-allocated buffer but for particle densities.

	A particle density consists of one 32-bit float. If set to NULL, densities are not read or written to.
	*/
	NxF32*					bufferDensity;

	/**
	\brief The separation (in bytes) between consecutive particle positions.

	The position of the first particle is found at location <tt>bufferPos</tt>;
	the second is at <tt>bufferPos + bufferPosByteStride</tt>;
	and so on.
	*/
	NxU32					bufferPosByteStride;

	/**
	\brief The separation (in bytes) between consecutive particle velocities.

	The velocity of the first particle is found at location <tt>bufferVel</tt>;
	the second is at <tt>bufferVel + bufferVelByteStride</tt>;
	and so on.
	*/
	NxU32					bufferVelByteStride;

	/**
	\brief The separation (in bytes) between consecutive particle lifetimes.

	The lifetime of the first particle is found at location <tt>bufferLife</tt>;
	the second is at <tt>bufferLife + bufferLifeByteStride</tt>;
	and so on.
	*/
	NxU32					bufferLifeByteStride;

	/**
	\brief The separation (in bytes) between consecutive particle densities.

	The density of the first particle is found at location <tt>bufferDensity</tt>;
	the second is at <tt>bufferDensity + bufferDensityByteStride</tt>;
	and so on.
	*/
	NxU32					bufferDensityByteStride;

	const char*				name;			//!< Possible debug name. The string is not copied by the SDK, only the pointer is stored.

	NX_INLINE ~NxParticleData();
	/**
	\brief (Re)sets the structure to the default.	
	*/
	NX_INLINE void setToDefault();
	/**
	\brief Returns true if the current settings are valid
	*/
	NX_INLINE bool isValid() const;

	/**
	\brief Constructor sets to default.
	*/
	NX_INLINE	NxParticleData();
	};

NX_INLINE NxParticleData::NxParticleData()
	{
	setToDefault();
	}

NX_INLINE NxParticleData::~NxParticleData()
	{
	}

NX_INLINE void NxParticleData::setToDefault()
	{
	maxParticles			= 0;
	numParticlesPtr			= NULL;
	bufferPos				= NULL;
	bufferVel				= NULL;
	bufferLife				= NULL;
	bufferDensity			= NULL;
	bufferPosByteStride		= 0;
	bufferVelByteStride		= 0;
	bufferLifeByteStride	= 0;
	bufferDensityByteStride = 0;
	name					= NULL;
	}

NX_INLINE bool NxParticleData::isValid() const
	{
	if (numParticlesPtr && !(bufferPos || bufferVel || bufferLife || bufferDensity)) return false;
	if ((bufferPos || bufferVel || bufferLife || bufferDensity) && !numParticlesPtr) return false;
	if (bufferPos && !bufferPosByteStride) return false;
	if (bufferVel && !bufferVelByteStride) return false;
	if (bufferLife && !bufferLifeByteStride) return false;
	if (bufferDensity && !bufferDensityByteStride) return false;
    
	return true;
	}

/** @} */
#endif


//AGCOPYRIGHTBEGIN
///////////////////////////////////////////////////////////////////////////
// Copyright © 2005 AGEIA Technologies.
// All rights reserved. www.ageia.com
///////////////////////////////////////////////////////////////////////////
//AGCOPYRIGHTEND

