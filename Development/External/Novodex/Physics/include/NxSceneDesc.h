#ifndef NX_PHYSICS_NXSCENEDESC
#define NX_PHYSICS_NXSCENEDESC
/*----------------------------------------------------------------------------*\
|
|						Public Interface to Ageia PhysX Technology
|
|							     www.ageia.com
|
\*----------------------------------------------------------------------------*/
/** \addtogroup physics
  @{
*/

/**
\brief Used to specify the timestepping behavior.

<b>Platform:</b>
\li PC SW: Yes
\li PPU  : Yes
\li PS3  : Yes
\li XB360: Yes

@see NxSceneDesc
*/
enum NxTimeStepMethod
	{
	NX_TIMESTEP_FIXED,				//!< The simulation automatically subdivides the passed elapsed time into maxTimeStep-sized substeps.
	NX_TIMESTEP_VARIABLE,			//!< The simulation uses the elapsed time that the user passes as-is, substeps (maxTimeStep, maxIter) are not used.
	NX_NUM_TIMESTEP_METHODS,

	NX_TSM_FORCE_DWORD = 0x7fffffff	//!< Just to make sure sizeof(enum) == 4, not a valid value.
	};

/**
\brief Used to choose between a hardware and software master scene.

This enum is used with the NxSceneDesc::simType member.

\note The master scene is a rigid body scene, HSM managed slave scenes are used for cloth or fluid. Slave
scenes can run in hardware even if the master scene is a software scene.

@see NxSceneDesc
*/
enum NxSimulationType
	{
	NX_SIMULATION_SW	= 0,		//!< Create a software master scene.
	NX_SIMULATION_HW	= 1,		//!< Create a hardware master scene.

	NX_STY_FORCE_DWORD = 0x7fffffff			//!< Just to make sure sizeof(enum) == 4, not a valid value.
	};

enum NxSceneFlags
	{
	/**
	\brief Used to disable use of SSE in the solver.

	SSE is detected at runtime(on appropriate platforms) and used if present by default.

	However use of SSE can be disabled, even if present, using this flag.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : N/A
	\li PS3  : N/A
	\li XB360: N/A
	*/
	NX_SF_DISABLE_SSE	=0x1,

	/**
	\brief Disable all collisions in a scene. Use the flags NX_AF_DISABLE_COLLISION and NX_SF_DISABLE_COLLISION for disabling collisions between specific actors and shapes.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see NX_AF_DISABLE_COLLISION, NX_SF_DISABLE_COLLISION
	*/
	NX_SF_DISABLE_COLLISIONS	=0x2,

	/**
	\brief Perform the simulation in a separate thread.

	By default the SDK runs the physics on a separate thread to the user thread(i.e. the thread which
	calls the API).

	However if this flag is disabled, then the simulation is run in the thread which calls #NxScene::simulate()

	<b>Default:</b> True

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : No
	\li PS3  : Yes
	\li XB360: Yes

	@see NxScene.simulate()
	*/
	NX_SF_SIMULATE_SEPARATE_THREAD	=0x4,

	/**
	\brief Enable internal multi threading.

	This flag enables the multi threading code within the SDK which allows the simulation to
	be divided into tasks for execution on an arbitrary number of threads. 
	
	This is an orthogonal feature to running the simulation in a separate thread, see #NX_SF_SIMULATE_SEPARATE_THREAD.

	\note There may be a small performance penalty for enabling the multi threading code, hence this flag should
	only be enabled if the application intends to use the feature.

	<b>Default:</b> False

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxSceneDesc NX_SF_SIMULATE_SEPARATE_THREAD
	*/
	NX_SF_ENABLE_MULTITHREAD		=0x8,

	/**
	\brief Enable Active Transform Notification.

	This flag enables the the Active Transform Notification feature for a scene.  This
	feature defaults to disabled.  When disabled, the function
	NxScene::getActiveTransforms() will always return a NULL list.

	\note There may be a performance penalty for enabling the Active Transform Notification, hence this flag should
	only be enabled if the application intends to use the feature.

	<b>Default:</b> False

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes
	*/
	NX_SF_ENABLE_ACTIVETRANSFORMS	=0x10,

	/**
	\brief Enable Restricted Scene.

	\note Only applies to hardware scenes.
	
	This flag creates a restricted scene, running the broadphase collision detection on hardware, 
	while limiting the number of actors (see "AGEIA PhysX Hardware Scenes" in the Guide).

	<b>Default:</b> False

	<b>Platform:</b>
	\li PC SW: No
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: No
	*/
	NX_SF_RESTRICTED_SCENE				=0x20,

	/**
	\brief Disable the mutex which serializes scene execution

	Under normal circumstances scene execution is serialized by a mutex. This flag
	can be used to disable this serialization.

	\warning This flag is _experimental_ and in future versions is likely be removed. In favour of 
	completely removing	the mutex.

	If this flag is used the recommended scenario is for use with a software scene and hardware fluid/cloth scene.

	<b>Default:</b> True (change from earlier beta versions)

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes
	*/
	NX_SF_DISABLE_SCENE_MUTEX			=0x40
	};

class NxUserNotify;
class NxUserContactModify;
class NxUserTriggerReport;
class NxUserContactReport;
class NxBounds3;
class NxUserScheduler;

/**
\brief Class used to retrieve limits(e.g. max number of bodies) for a scene. The limits
are used as a hint to the size of the scene, not as a hard limit (i.e. it will be possible
to create more objects than specified in the scene limits).

<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes
*/
class NxSceneLimits
	{
	public:
	NxU32					maxNbActors;		//!< Expected max number of actors
	NxU32					maxNbBodies;		//!< Expected max number of bodies
	NxU32					maxNbStaticShapes;	//!< Expected max number of static shapes
	NxU32					maxNbDynamicShapes;	//!< Expected max number of dynamic shapes
	NxU32					maxNbJoints;		//!< Expected max number of joints

	NX_INLINE NxSceneLimits();
	};

NX_INLINE NxSceneLimits::NxSceneLimits()	//constructor sets to default
	{
	maxNbActors			= 0;
	maxNbBodies			= 0;
	maxNbStaticShapes	= 0;
	maxNbDynamicShapes	= 0;
	maxNbJoints			= 0;
	}

/**
\brief Descriptor class for scenes. See #NxScene.

@see NxScene NxPhysicsSDK.createScene maxBounds
*/

class NxSceneDesc
	{
	public:

	/**
	\brief Gravity vector

	<b>Range:</b> force vector<br>
	<b>Default:</b> Zero

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see NxScene.setGravity()
	*/
	NxVec3					gravity;

	/**
	\brief Possible notification callback

	<b>Default:</b> NULL

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see NxUserNotify NxScene.setUserNotify() NxScene.getUserNotify()
	*/
	NxUserNotify*			userNotify;

	/**
	\brief Possible asynchronous callback for contact modification

	<b>Default:</b> NULL

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see NxUserContactModify NxScene.setUserContactModify() NxScene.getUserContactModify()
	*/
	NxUserContactModify*		userContactModify;

	/**
	\brief Possible trigger callback

	<b>Default:</b> NULL

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see NxUserTriggerReport NxScene.setUserTriggerReport() NxScene.getUserTriggerReport()
	*/
	NxUserTriggerReport*	userTriggerReport;

	/**
	\brief Possible contact callback

	<b>Default:</b> NULL

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Partial (Only NX_NOTIFY_ON_START_TOUCH and NX_NOTIFY_ON_END_TOUCH)
	\li PS3  : Yes
	\li XB360: Yes

	@see NxUserContactReport NxScene.setUserContactReport() NxScene.getUserContactReport()
	*/
	NxUserContactReport*	userContactReport;

	/**
	\brief Maximum substep size.

	<b>Range:</b> (0,inf)<br>
	<b>Default:</b> 1.0/60.0

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see NxScene.setTiming() maxIter
	*/
	NxReal					maxTimestep;

	/**
	\brief Maximum number of substeps to take

	<b>Default:</b> 8

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see NxScene.setTiming() maxTimestep
	*/
	NxU32					maxIter;

	/**
	\brief Integration method.

	<b>Default:</b> NX_TIMESTEP_FIXED

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see NxTimeStepMethod NxScene.setTiming() maxTimestep maxIter
	*/
	NxTimeStepMethod		timeStepMethod;

	/**
	\brief Max scene bounds.

	If scene bounds are provided (maxBounds in the descriptor), the SDK takes advantage of this information
	to accelerate scene-level collision queries (e.g. raycasting). When using maxBounds, you have to make
	sure created objects stay within the scene bounds. In particular, the position of dynamic shapes should
	stay within the provided bounds. Otherwise the shapes outside the bounds will not be taken into account
	by all scene queries (raycasting, sweep tests, overlap tests, etc). They will nonetheless still work
	correctly for the main physics simulation.

	<b>Range:</b> See #NxBounds3<br>
	<b>Default:</b> NULL

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes
	*/
	NxBounds3*				maxBounds;
	/**
	\brief Expected scene limits (or NULL)

	<b>Default:</b> NULL

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see NxSceneLimits
	*/
	NxSceneLimits*			limits;

	/**
	\brief Used to specify if the scene is a master hardware or software scene.

	<b>Default:</b> NX_SIMULATION_SW

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see NxSimulationType
	*/
	NxSimulationType		simType;

	/**
	\brief Enable/disable default ground plane

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	<b>Default:</b> false
	*/
	NX_BOOL					groundPlane;

	/**
	\brief Enable/disable 6 planes around maxBounds (if available)

	<b>Default:</b> false

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes

	@see maxBounds
	*/
	NX_BOOL					boundsPlanes;

	/**
	\brief Flags used to select scene options.

	<b>Default:</b> NX_SF_SIMULATE_SEPARATE_THREAD | NX_SF_DISABLE_SCENE_MUTEX

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Partial
	\li PS3  : Yes
	\li XB360: Yes

	@see NxSceneFlags
	*/
	NxU32					flags;

	/**
	\brief Sets the number of SDK managed thread used when running the simulation in parallel.

	When internal multi threading is enabled(see #NX_SF_ENABLE_MULTITHREAD) the SDK creates a number of
	threads to run the simulation. This member controls the number of threads.

	This member is ignored if the application takes over control of the work allocation with a custom scheduler.

	<b>Default:</b> 0

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see customScheduler
	*/
	NxU32					internalThreadCount;

	/**
	\brief Sets the number of SDK managed threads which will be processing background tasks.

	For example the SDK may need to preprocess data to be sent to the PhysX card.

	This member is ignored if the application takes over control of the work allocation with a custom scheduler.

	<b>Default:</b> 0

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see customScheduler
	*/
	NxU32					backgroundThreadCount;
	
	/**
	\brief Defines a custom scheduler.

	The application can define a custom scheduler to completely take over the allocation of work items among threads.

	An alternative is for the application to use the built in scheduler. See #internalThreadCount

	<b>Default:</b> NULL (disabled)

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes

	@see NxScheduler NX_SF_ENABLE_MULTITHREAD internalThreadCount
	*/
	NxUserScheduler*			customScheduler;

	/**
	\brief Allows the user to specify which (logical) processor to allocate SDK internal threads to.

	The SDK Will allocate internal threads to processors corresponding to bits which are set. Starting from
	the least significant bit.

	This flag is ignored for platforms which do not associate threads exclusively to specific processors.

	\note The XBox 360 associates threads with specific processors.

	<b>Default:</b> 0x55555554 (allocate SDK threads to even logical processors, excluding the first)

	<b>Platform:</b>
	\li PC SW: No
	\li PPU  : No
	\li PS3  : No
	\li XB360: Yes
	*/
	NxU32					threadMask;

	/**
	\brief Allows the user to specify which (logical) processor to allocate SDK background threads.

	The SDK Will allocate internal threads to processors corresponding to bits which are set. Starting from
	the least significant bit.

	This flag is ignored for platforms which do not associate threads exclusively to specific processors.

	\note The XBox 360 associates threads with specific processors.

	<b>Default:</b> 0x55555554 (allocate SDK threads to even logical processors, excluding the first)

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes
	*/
	NxU32					backgroundThreadMask;


	/**
	\brief Allows the user to specify the stack size for the main simulation thread.

	The value is specified in bytes and rounded to an appropriate size by the operating system.

	Specifying a value of zero will cause the SDK to choose a platform specific default value:

	\li PC SW - OS default
	\li PPU - OS default
	\li PS3 - 256k
	\li XBox 360 - OS default

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes
	*/
	NxU32					simThreadStackSize;
	
	/**
	\brief Sets the thread priority of the main simulation thread.

	The default is normal priority.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes
	*/
	NxThreadPriority		simThreadPriority;

	/**
	\brief Allows the user to specify the stack size for the worker threads created by the SDK.

	The value is specified in bytes and rounded to an appropriate size by the operating system.

	Specifying a value of zero will cause the SDK to choose a platform specific default value.

	
	\li PC SW - OS default
	\li PPU - OS default
	\li XBox 360 - OS default

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes
	*/
	NxU32					workerThreadStackSize;

	/**
	\brief Sets the thread priority of the SDK created worker threads

	The default is normal priority.

	\note Background worker threads are always assigned low priority.

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: Yes
	*/
	NxThreadPriority		workerThreadPriority;

	/**
	\brief Number of automatically managed hardware rigid body scenes.

	The SDK will create this many hardware scenes internally.  Later, actors created may be created
	within these hardware scene(s) using the NxActor::managedRBSceneIndex member.

	If this is nonzero but the device has no hardware support, scene creation will fail.

	At the moment only a single managed scene is supported.  

	<b>Default:</b> 0

	<b>Platform:</b>
	\li PC SW: No
	\li PPU  : Yes
	\li PS3  : No
	\li XB360: No

	*/
	NxU32					numManagedHwRBScenes;

	/**
	\brief Will be copied to NxScene::userData

	<b>Default:</b> NULL

	<b>Platform:</b>
	\li PC SW: Yes
	\li PPU  : Yes
	\li PS3  : Yes
	\li XB360: Yes
	*/
	void*					userData;
	/**
	\brief constructor sets to default (no gravity, no ground plane, collision detection on).
	*/
	NX_INLINE NxSceneDesc();
	/**
	\brief (re)sets the structure to the default (no gravity, no ground plane, collision detection on).	
	*/
	NX_INLINE void setToDefault();
	/**
	\brief Returns true if the descriptor is valid.
	\return true if the current settings are valid (returns always true).
	*/
	NX_INLINE bool isValid() const;
	};
NX_INLINE NxSceneDesc::NxSceneDesc()	//constructor sets to default
	{
	setToDefault();
	}

NX_INLINE void NxSceneDesc::setToDefault()
	{
	gravity.zero();
	userNotify			= NULL;
	userTriggerReport	= NULL;
	userContactReport	= NULL;
	userContactModify	= NULL;

	maxTimestep			= 1.0f/60.0f;
	maxIter				= 8;
	timeStepMethod		= NX_TIMESTEP_FIXED;

	maxBounds			= NULL;
	limits				= NULL;
	simType				= NX_SIMULATION_SW;
	groundPlane			= false;
	boundsPlanes		= false;
	userData			= NULL;
	flags				= NX_SF_SIMULATE_SEPARATE_THREAD | NX_SF_DISABLE_SCENE_MUTEX;

	internalThreadCount   = 0;
	backgroundThreadCount = 0;
	customScheduler		  = NULL;

	simThreadStackSize	  = 0;
	simThreadPriority	  = NX_TP_NORMAL;

	workerThreadStackSize = 0;
	workerThreadPriority  = NX_TP_NORMAL;

	//allocate every other hardware thread(2 threads per core...), 
	//excluding the first core(for main simulation thread)
	threadMask				= 0x55555554;
	backgroundThreadMask	= 0x55555554;
	numManagedHwRBScenes	= 0;
	}

NX_INLINE bool NxSceneDesc::isValid() const
	{
	if(maxTimestep <= 0 || maxIter < 1 || timeStepMethod > NX_NUM_TIMESTEP_METHODS)
		return false;
	if(boundsPlanes && !maxBounds)
		return false;

	if((customScheduler!=NULL)&&(internalThreadCount>0))
		return false;

	if((customScheduler!=NULL)&&(backgroundThreadCount>0))
		return false;

	if((internalThreadCount>0)&&(threadMask==0))
		return false;
	
	if((backgroundThreadCount>0)&&(backgroundThreadMask==0))
		return false;

	if (numManagedHwRBScenes > 1)		//at the moment only a single one is supported
		return false;

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
