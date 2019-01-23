#ifndef NX_PHYSICS_NX_CLOTH
#define NX_PHYSICS_NX_CLOTH
/** \addtogroup cloth
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
#include "NxClothDesc.h"

class NxRay;
class NxScene;
class NxShape;
class NxBounds3;
class NxStream;

class NxCloth
{
protected:
	NX_INLINE NxCloth() : userData(NULL) {}
	virtual ~NxCloth() {}

public:
	/**
	\brief Saves the cloth descriptor.

	\param[out] desc The descriptor used to retrieve the state of the object.
	\return True on success.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc
	*/
	virtual	bool saveToDesc(NxClothDesc& desc) const = 0;

	/**
	\brief Returns a pointer to the corresponding cloth mesh.

	\return The cloth mesh associated with this cloth.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.clothMesh
	*/
	virtual	NxClothMesh* getClothMesh() const = 0;

	/**
	\brief Sets the cloth bending stiffness in the range from 0 to 1.
 
	\param[in] stiffness The stiffness of this cloth.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.bendingStiffness getBendingStiffness()
	*/
	virtual void setBendingStiffness(NxReal stiffness) = 0;

	/**
	\brief Retrieves the cloth bending stiffness.

	\return Bending stiffness of cloth.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.bendingStiffness setBendingStiffness()
	*/
	virtual NxReal getBendingStiffness() const = 0;

	/**
	\brief Sets the cloth stretching stiffness in the range from 0 to 1.

	Note: The stretching stiffness must be larger than 0.

	\param[in] stiffness Stiffness of cloth.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.stretchingStiffness getStretchingStiffness()
	*/
	virtual void setStretchingStiffness(NxReal stiffness) = 0;

	/**
	\brief Retrieves the cloth stretching stiffness.

	\return stretching stiffness of cloth.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.stretchingStiffness setStretchingStiffness()
	*/
	virtual NxReal getStretchingStiffness() const = 0;

	/**
	\brief Sets the damping coefficient in the range from 0 to 1.

	\param[in] dampingCoefficient damping coefficient of cloth.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.dampingCoefficient getDampingCoefficient()
	*/
	virtual void setDampingCoefficient(NxReal dampingCoefficient) = 0;

	/**
	\brief Retrieves the damping coefficient.

	\return damping coefficient of cloth.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes
	
	@see NxClothDesc.dampingCoefficient setDampingCoefficient()
	*/
	virtual NxReal getDampingCoefficient() const = 0;

	/**
	\brief Sets the cloth friction coefficient in the range from 0 to 1.

	\param[in] friction The friction of the cloth.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.friction getFriction()
	*/
	virtual void setFriction(NxReal friction) = 0;

	/**
	\brief Retrieves the cloth friction coefficient.

	\return Friction coefficient of cloth.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes
	
	@see NxClothDesc.friction setFriction()
	*/
	virtual NxReal getFriction() const = 0;

	/**
	\brief Sets the cloth pressure coefficient (must be positive).

	\param[in] pressure The pressure applied to the cloth.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.pressure getPressure()
	*/
	virtual void setPressure(NxReal pressure) = 0;

	/**
	\brief Retrieves the cloth pressure coefficient.

	\return Pressure of cloth.
	
	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.pressure setPressure()
	*/
	virtual NxReal getPressure() const = 0;

	/**
	\brief Sets the cloth tear factor (must be larger than one).

	\param[in] factor The tear factor for the cloth

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.tearFactor getTearFactor()
	*/
	virtual void setTearFactor(NxReal factor) = 0;

	/**
	\brief Retrieves the cloth tear factor.

	\return tear factor of cloth.
	
	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.tearFactor setTearFactor()
	*/
	virtual NxReal getTearFactor() const = 0;

	/**
	\brief Sets the cloth attachment tear factor (must be larger than one).

	\param[in] factor The attachment tear factor for the cloth

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.attachmentTearFactor getAttachmentTearFactor()
	*/
	virtual void setAttachmentTearFactor(NxReal factor) = 0;

	/**
	\brief Retrieves the attachment cloth tear factor.

	\return tear attachment factor of cloth.
	
	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.attachmentTearFactor setAttachmentTearFactor()
	*/
	virtual NxReal getAttachmentTearFactor() const = 0;

	/**
	\brief Sets the cloth thickness (must be positive).

	\param[in] thickness The thickness of the cloth.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.thickness getThickness()
	*/
	virtual void setThickness(NxReal thickness) = 0;

	/**
	\brief Gets the cloth thickness.

	\return thickness of cloth.
	
	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.thickness setThickness()
	*/
	virtual NxReal getThickness() const = 0;

	/**
	\brief Gets the cloth density.

	\return density of cloth.
	
	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.density
	*/
	virtual NxReal getDensity() const = 0;

	/**
	\brief Retrieves the cloth solver iterations.

	\return solver iterations of cloth.
	
	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.solverIterations setSolverIterations()
	*/
	virtual NxU32 getSolverIterations() const = 0;

	/**
	\brief Sets the cloth solver iterations.

	\param[in] iterations The new solver iteration count for the cloth.
	
	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.solverIterations getSolverIterations()
	*/
	virtual void setSolverIterations(NxU32 iterations) = 0;

	/**
	\brief Returns a world space AABB enclosing all cloth points.

	\param[out] bounds Retrieves the world space bounds.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxBounds3
	*/
	virtual void getWorldBounds(NxBounds3& bounds) const = 0;

	/**
	\brief Attaches the cloth to a shape. All cloth points currently inside the shape are attached.

	\note This method only works with primitive and convex shapes. Since the inside of a general 
	triangle mesh is not clearly defined.

	\param[in] shape Shape to which the cloth should be attached to.
	\param[in] attachmentFlags One or two way interaction, tearable or non-tearable
	
	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothAttachmentFlag freeVertex() attachToCollidingShapes()
	*/
	virtual void attachToShape(const NxShape *shape, NxU32 attachmentFlags) = 0;

	/**
	\brief Attaches the cloth to all shapes, currently colliding. 

	\note This method only works with primitive and convex shapes. Since the inside of a general 
	triangle mesh is not clearly defined.

	\param[in] attachmentFlags One or two way interaction, tearable or non-tearable

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothAttachmentFlag NxClothDesc.attachmentTearFactor NxClothDesc.attachmentResponseCoefficient freeVertex()
	*/
	virtual void attachToCollidingShapes(NxU32 attachmentFlags) = 0;

	/**
	\brief Detaches the cloth from a shape it has been attached to before. 

	If the cloth has not been attached to the shape before, the call has no effect.

	\param[in] shape Shape from which the cloth should be detached.
	
	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothAttachmentFlag NxClothDesc.attachmentTearFactor NxClothDesc.attachmentResponseCoefficient freeVertex() attachToShape()
	*/
	virtual void detachFromShape(const NxShape *shape) = 0;

	/**
	\brief Attaches a cloth vertex to a local position within a shape.

	\param[in] vertexId Index of the vertex to attach.
	\param[in] shape Shape to attach the vertex to.
	\param[in] localPos The position relative to the pose of the shape.
	\param[in] attachmentFlags One or two way interaction, tearable or non-tearable

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxShape freeVertex() NxClothAttachmentFlag attachToShape()
	*/
	virtual void attachVertexToShape(NxU32 vertexId, const NxShape *shape, const NxVec3 &localPos, NxU32 attachmentFlags) = 0;

	/**
	\brief Attaches a cloth vertex to a position in world space.

	\param[in] vertexId Index of the vertex to attach.
	\param[in] pos The position in world space.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothAttachmentFlag NxClothDesc.attachmentTearFactor NxClothDesc.attachmentResponseCoefficient freeVertex() attachToShape()
	*/
	virtual void attachVertexToGlobalPosition(const NxU32 vertexId, const NxVec3 &pos) = 0;

	/**
	\brief Frees a previously attached cloth point.

	\param[in] vertexId Index of the vertex to free.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see attachVertexToGlobalPosition() attachVertexToShape() detachFromShape()
	*/
	virtual void freeVertex(const NxU32 vertexId) = 0;

	/**
	\brief Tears the cloth at a given vertex. 
	
	First the vertex is duplicated. The triangles on one side of the split plane keep 
	the original vertex. For all triangles on the opposite side the original vertex is 
	replaced by the new one. The split plane is defined by the world location of the 
	vertex and the normal provided by the user.

	Note: TearVertex performs a user defined vertex split in contrast to an automatic split
	that is performed when the flag NX_CLF_TEARABLE is set. Therefore, tearVertex works 
	even if NX_CLF_TEARABLE is not set in NxClothDesc.flags.

	Note: For tearVertex to work in hardware mode, the clothMesh has to be cooked with the
	flag NX_CLOTH_MESH_TEARABLE set in NxClothMeshDesc.flags.

	\param[in] vertexId Index of the vertex to tear.
	\param[in] normal The normal of the split plane.
	\return true if the split had an effect (i.e. there were triangles on both sides of the split plane)

	@see NxClothFlag, NxClothMeshFlags, NxClothDesc.flags NxSimpleTriangleMesh.flags

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes
	*/
	virtual bool tearVertex(const NxU32 vertexId, const NxVec3 &normal) = 0;

	/**
	\brief Executes a raycast against the cloth.

	\param[in] worldRay The ray in world space.
	\param[out] hit The hit position.
	\param[out] vertexId Index to the nearest vertex hit by the raycast.

	\return true if the ray hits the cloth.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes
	*/

	virtual bool raycast(const NxRay& worldRay, NxVec3 &hit, NxU32 &vertexId) = 0;

	/**
	\brief Sets which collision group this cloth is part of.

	\param[in] collisionGroup The collision group for this cloth.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxCollisionGroup
	*/
	virtual void setGroup(NxCollisionGroup collisionGroup) = 0;

	/**
	\brief Retrieves the value set with #setGroup().

	\return The collision group this cloth belongs to.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxCollisionGroup
	*/
	virtual NxCollisionGroup getGroup() const = 0;

#ifdef NX_SUPPORT_NEW_FILTERING
	/**
	\brief Sets 128-bit mask used for collision filtering.

	\param[in] groupsMask The group mask to set for the cloth.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see getGroupsMask() NxGroupsMask
	*/
	virtual void setGroupsMask(const NxGroupsMask& groupsMask) = 0;

	/**
	\brief Sets 128-bit mask used for collision filtering.

	\return The group mask for the cloth.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see setGroupsMask() NxGroupsMask
	*/
	virtual const NxGroupsMask getGroupsMask() const = 0;
#endif

	/**
	\brief Sets the user buffer wrapper for the cloth mesh.

	\param[in,out] meshData User buffer wrapper.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxMeshData NxClothDesc.meshData
	*/
	virtual void setMeshData(NxMeshData& meshData) = 0;

	/**
	\brief Returns a copy of the user buffer wrapper for the cloth mesh.
	
	\return User buffer wrapper.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxMeshData setMeshData() NxClothDesc.meshData
	*/
	virtual NxMeshData getMeshData() = 0;

	
	/**
	\brief Sets the positions of the cloth.

	The user must supply a buffer containing all positions (i.e same number of elements as number of particles).

	\param[in] buffer The user supplied buffer containing all positions for the cloth.
	\param[in] byteStride The stride in bytes between the position vectors in the buffer. Default is size of NxVec3.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see getPositions(void* buffer) setVelocities(void* buffer) getVelocities(void* buffer) getNumberOfParticles()
	*/
	virtual void setPositions(void* buffer, NxU32 byteStride = sizeof(NxVec3)) = 0;

	/**
	\brief Gets the positions of the cloth.

	The user must supply a buffer large enough to hold all positions (i.e same number of elements as number of particles).

	\param[in] buffer The user supplied buffer to hold all positions of the cloth.
	\param[in] byteStride The stride in bytes between the position vectors in the buffer. Default is size of NxVec3.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see setPositions(void* buffer) setVelocities(void* buffer) getVelocities(void* buffer) getNumberOfParticles()
	*/
	virtual void getPositions(void* buffer, NxU32 byteStride = sizeof(NxVec3)) = 0;

	/**
	\brief Sets the velocities of the cloth.

	The user must supply a buffer containing all velocities (i.e same number of elements as number of particles).

	\param[in] buffer The user supplied buffer containing all velocities for the cloth.
	\param[in] byteStride The stride in bytes between the velocity vectors in the buffer. Default is size of NxVec3.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see getVelocities(void* buffer) setPositions(void* buffer) getPositions(void* buffer) getNumberOfParticles()
	*/

	virtual void setVelocities(void* buffer, NxU32 byteStride = sizeof(NxVec3)) = 0;

	/**
	\brief Gets the velocities of the cloth.

	The user must supply a buffer large enough to hold all velocities (i.e same number of elements as number of particles).

	\param[in] buffer The user supplied buffer to hold all velocities of the cloth.
	\param[in] byteStride The stride in bytes between the velocity vectors in the buffer. Default is size of NxVec3.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see setVelocities(void* buffer) setPositions(void* buffer) getPositions(void* buffer) getNumberOfParticles()
	*/

	virtual void getVelocities(void* buffer, NxU32 byteStride = sizeof(NxVec3)) = 0;

	/**
	\brief Gets the number of cloth particles.
	
	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see setVelocities(void* buffer) getVelocities(void* buffer) setPositions(void* buffer) getPositions(void* buffer) 
	*/
	virtual NxU32 getNumberOfParticles() const = 0;

	/**
	\brief Gets the byte size of the current cloth state.
	
	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see getNumberOfParticles()
	*/
	virtual NxU32 getStateByteSize() = 0;


	/**
	\brief Saves the current cloth state to a stream.

	\param[in] stream The user supplied stream to hold the cloth state.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : No
	\li PS3  : No
	\li XB360: Yes

	@see loadStateFromStream(NxStream& stream) setVelocities(void* buffer) getVelocities(void* buffer) setPositions(void* buffer) getPositions(void* buffer) 
	*/
	
	virtual void saveStateToStream(NxStream& stream) = 0;

	/**
	\brief Loads the current cloth state from a stream.

	\param[in] stream The user supplied stream that holds the cloth state.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : No
	\li PS3  : No
	\li XB360: Yes

	@see saveStateToStream(NxStream& stream) setVelocities(void* buffer) getVelocities(void* buffer) setPositions(void* buffer) getPositions(void* buffer) 
	*/
	
	virtual void loadStateFromStream(NxStream& stream) = 0;


	/**
	\brief Sets the collision response coefficient.

	\param[in] coefficient The collision response coefficient in the range from 0 to 1.
	
	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.collisionResponseCoefficient getCollisionResponseCoefficient()
	*/
	virtual void setCollisionResponseCoefficient(NxReal coefficient) = 0;

	/**
	\brief Retrieves the collision response coefficient.

	\return The collision response coefficient.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.collisionResponseCoefficient setCollisionResponseCoefficient()
	*/
	virtual NxReal getCollisionResponseCoefficient() const = 0;

	/**
	\brief Sets the attachment response coefficient

	\param[in] coefficient The attachment response coefficient in the range from 0 to 1.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.attachmentResponseCoefficient getAttachmentResponseCoefficient()
	*/
	virtual void setAttachmentResponseCoefficient(NxReal coefficient) = 0;

	/**
	\brief Retrieves the attachment response coefficient

	\return The attachment response coefficient.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.attachmentResponseCoefficient setAttachmentResponseCoefficient()
	*/
	virtual NxReal getAttachmentResponseCoefficient() const = 0;

	/**
	\brief Sets an external acceleration which affects all non attached particles of the cloth

	\param[in] acceleration The acceleration vector (unit length / s^2)

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.externalAcceleration setExternalAcceleration()
	*/
	virtual void setExternalAcceleration(NxVec3 acceleration) = 0;

	/**
	\brief Retrieves the external acceleration which affects all non attached particles of the cloth

	\return The acceleration vector (unit length / s^2)

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.externalAcceleration setExternalAcceleration()
	*/
	virtual NxVec3 getExternalAcceleration() const = 0;

	/**
	\brief Returns true if this cloth is sleeping.

	When a cloth does not move for a period of time, it is no longer simulated in order to save time. This state
	is called sleeping. However, because the object automatically wakes up when it is either touched by an awake object,
	or one of its properties is changed by the user, the entire sleep mechanism should be transparent to the user.
	
	If a cloth is asleep after the call to NxScene::fetchResults() returns, it is guaranteed that the position of the cloth 
	vertices was not changed. You can use this information to avoid updating dependent objects.
	
	\return True if the cloth is sleeping.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see isSleeping() getSleepLinearVelocity() wakeUp() putToSleep()
	*/
	virtual bool isSleeping() const = 0;

	/**
	\brief Returns the linear velocity below which a cloth may go to sleep.
	
	A cloth whose linear velocity is above this threshold will not be put to sleep.
    
    @see isSleeping

	\return The threshold linear velocity for sleeping.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see isSleeping() getSleepLinearVelocity() wakeUp() putToSleep() setSleepLinearVelocity()
	*/
    virtual NxReal getSleepLinearVelocity() const = 0;

    /**
	\brief Sets the linear velocity below which a cloth may go to sleep.
	
	A cloth whose linear velocity is above this threshold will not be put to sleep.
	
	If the threshold value is negative,	the velocity threshold is set using the NxPhysicsSDK's 
	NX_DEFAULT_SLEEP_LIN_VEL_SQUARED parameter.
    
	\param[in] threshold Linear velocity below which a cloth may sleep. <b>Range:</b> (0,inf]

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see isSleeping() getSleepLinearVelocity() wakeUp() putToSleep()
	*/
    virtual void setSleepLinearVelocity(NxReal threshold) = 0;

	/**
	\brief Wakes up the cloth if it is sleeping.  

	The wakeCounterValue determines how long until the cloth is put to sleep, a value of zero means 
	that the cloth is sleeping. wakeUp(0) is equivalent to NxCloth::putToSleep().

	\param[in] wakeCounterValue New sleep counter value. <b>Range:</b> [0,inf]
	
	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see isSleeping() getSleepLinearVelocity() putToSleep()
	*/
	virtual void wakeUp(NxReal wakeCounterValue = NX_SLEEP_INTERVAL) = 0;

	/**
	\brief Forces the cloth to sleep. 
	
	The cloth will fall asleep.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see isSleeping() getSleepLinearVelocity() wakeUp()
	*/
	virtual void putToSleep() = 0;

	/**
	\brief Sets the flags, a combination of the bits defined by the enum ::NxClothFlag.

	\param[in] flags #NxClothFlag combination.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.flags NxClothFlag getFlags()
	*/

	virtual void setFlags(NxU32 flags) = 0;

	/**
	\brief Retrieves the flags.

	\return The cloth flags.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxClothDesc.flags NxClothFlag setFlags()
	*/
	virtual NxU32 getFlags() const = 0;

	/**
	\brief Sets a name string for the object that can be retrieved with getName().
	
	This is for debugging and is not used by the SDK. The string is not copied by 
	the SDK, only the pointer is stored.

	\param[in] name String to set the objects name to.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see getName()
	*/

	virtual void setName(const char* name) = 0;

	/**
	\brief Applies a force (or impulse) defined in the global coordinate frame, to a particular 
	vertex of the cloth. 

	Because forces are reset at the end of every timestep, 
	you can maintain a total external force on an object by calling this once every frame.

    ::NxForceMode determines if the force is to be conventional or impulsive.

	\param[in] force Force/impulse to add, defined in the global frame. <b>Range:</b> force vector
	\param[in] vertexId Number of the vertex to add the force at. <b>Range:</b> position vector
	\param[in] mode The mode to use when applying the force/impulse 
	(see #NxForceMode, supported modes are NX_FORCE, NX_IMPULSE, NX_ACCELERATION, NX_VELOCITY_CHANGE)

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxForceMode 
	*/

	virtual	void addForceAtVertex(const NxVec3& force, NxU32 vertexId, NxForceMode mode = NX_FORCE) = 0;

	/**
	\brief Applies a force (or impulse) defined in the global coordinate frame at a particular
	position. All vertices within radius will be affected with a quadratic drop-off. 

	Because forces are reset at the end of every timestep, 
	you can maintain a total external force on an object by calling this once every frame.

    ::NxForceMode determines if the force is to be conventional or impulsive.

	\param[in] position Position to apply force at.
	\param[in] force Force/impulse to add, defined in the global frame. <b>Range:</b> force vector
	\param[in] radius The sphere radius in which particles will be affected . <b>Range:</b> position vector
	\param[in] mode The mode to use when applying the force/impulse 
	(see #NxForceMode, supported modes are NX_FORCE, NX_IMPULSE, NX_ACCELERATION, NX_VELOCITY_CHANGE)

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxForceMode 
	*/

	virtual	void addForceAtPos(const NxVec3& position, const NxVec3& force, NxReal radius, NxForceMode mode = NX_FORCE) = 0;

	/**
	\brief Finds triangles touching the input bounds.

	\warning This method returns a pointer to an internal structure using the indices member. Hence the
	user should use or copy the indices before calling any other API function.

	\param[in] bounds Bounds to test against in world space. <b>Range:</b> See #NxBounds3
	\param[out] nb Retrieves the number of triangle indices touching the AABB.
	\param[out] indices Returns an array of touching triangle indices. 
	The triangle indices correspond to the triangles referenced to by NxClothDesc.meshdata (#NxMeshData).
	Triangle i has the vertices 3i, 3i+1 and 3i+2 in the array NxMeshData.indicesBegin.
	\return True if there is an overlap.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxBounds3 NxClothDesc NxMeshData
	*/
	virtual	bool overlapAABBTriangles(const NxBounds3& bounds, NxU32& nb, const NxU32*& indices) const = 0;

	/**
	\brief Retrieves the scene which this cloth belongs to.

	\return Owner Scene.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see NxScene
	*/
	virtual NxScene& getScene() const = 0;

	/**
	\brief Retrieves the name string set with setName().

	\return Name string associated with object.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see setName()
	*/
	virtual const char* getName() const = 0;

	//public variables:
public:
	void* userData; //!< user can assign this to whatever, usually to create a 1:1 relationship with a user object.
};
/** @} */
#endif
//AGCOPYRIGHTBEGIN
///////////////////////////////////////////////////////////////////////////
// Copyright © 2005 AGEIA Technologies.
// All rights reserved. www.ageia.com
///////////////////////////////////////////////////////////////////////////
//AGCOPYRIGHTEND
