/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ParticleSystemComponent extends PrimitiveComponent
	native(Particle)
	hidecategories(Object)
	hidecategories(Physics)
	hidecategories(Collision)
	editinlinenew
	dependson(ParticleSystem);

var()				const	ParticleSystem							Template;

struct ParticleEmitterInstance
{
	// No UObject reference
};
var		native transient	const	array<pointer>					EmitterInstances{struct FParticleEmitterInstance};

/**
 *	The static mesh components for a mesh emitter.
 *	This is to prevent the SMCs from being garbage collected.
 */
var private const array<StaticMeshComponent>	SMComponents;
var private const array<MaterialInterface>		SMMaterialInterfaces;

/** If true, activate on creation. */
var()						bool									bAutoActivate;
var					const	bool									bWasCompleted;
var					const	bool									bSuppressSpawning;
var					const	bool									bWasDeactivated;
var()						bool									bResetOnDetach;
/** whether to update the particle system on dedicated servers */
var 						bool 									bUpdateOnDedicatedServer;

/** Indicates that the component has not been ticked since being attached. */
var					const	bool									bJustAttached;

/** INTERNAL
 *	Set to TRUE when InitParticles has been called.
 *	Set to FALSE when ResetParticles has been called.
 *	Used to quick-out of Tick and Render calls
 * (when caching PSysComps and emitter instances).
 */
var	transient				bool									bIsActive;

/** Enum for specifying type of a name instance parameter. */
enum EParticleSysParamType
{
	PSPT_None,
	PSPT_Scalar,
	PSPT_Vector,
	PSPT_Color,
	PSPT_Actor,
	PSPT_Material
};

/** Struct used for a particular named instance parameter for this ParticleSystemComponent. */
struct native ParticleSysParam
{
	var()	name					Name;
	var()	EParticleSysParamType	ParamType;

	var()	float					Scalar;
	var()	vector					Vector;
	var()	color					Color;
	var()	actor					Actor;
	var()	MaterialInterface			Material;
};

/**
 *	Array holding name instance parameters for this ParticleSystemComponent.
 *	Parameters can be used in Cascade using DistributionFloat/VectorParticleParameters.
 */
var()	editinline array<ParticleSysParam>		InstanceParameters;

var		vector									OldPosition;
var		vector									PartSysVelocity;

var		float									WarmupTime;
var 	bool 									bWarmingUp;
var		int										LODLevel;


/**
 * bCanBeCachedInPool
 *
 * If this is true, when the PSC completes it will do the following:
 *    bHidden = TRUE
 *
 * This is used for Particles which are cached in a pool where you need
 * to make certain to NOT kill off the EmitterInstances so we do not
 * re allocate.
 *
 * @see ActivateSystem() where it rewinds the indiv emitters if they need it
 */

var  	bool									bIsCachedInPool;


/**
 * Number of seconds of emitter not being rendered that need to pass before it
 * no longer gets ticked/ becomes inactive.
 */
var()	float									SecondsBeforeInactive;


/**
 *	INTERNAL. Used by the editor to set the LODLevel
 */
var		int										EditorLODLevel;

/** Used to accumulate total tick time to determine whether system can be skipped ticking if not visible. */
var	transient	float							AccumTickTime;

/** indicates that the component's LODMethod overrides the Template's */
var(LOD) bool bOverrideLODMethod;
/** The method of LOD level determination to utilize for this particle system */
var(LOD) ParticleSystemLODMethod LODMethod;

/**
 *	Flag indicating that dynamic updating of render data should NOT occur during Tick.
 *	This is used primarily to allow for warming up and simulated effects to a certain state.
 */
var		bool									bSkipUpdateDynamicDataDuringTick;

/**
 *	Set this to TRUE to have the PSysComponent update during the tick if 'dirty'.
 */
var		bool									bUpdateComponentInTick;

/** This is set when any of our "don't tick me" timeout values have fired */
var transient bool bForcedInActive;

/** This is set when the particle system component is warming up */
var transient bool bIsWarmingUp;

/** The view relevance flags for each LODLevel. */
var		transient	const	array<MaterialViewRelevance>	CachedViewRelevanceFlags;

/** If TRUE, the ViewRelevanceFlags are dirty and should be recached */
var		transient	const	bool							bIsViewRelevanceDirty;

//
delegate OnSystemFinished(ParticleSystemComponent PSystem);	// Called when the particle system is done

native final function SetTemplate(ParticleSystem NewTemplate);
native final function ActivateSystem();
native final function DeactivateSystem();
native final function KillParticlesForced();

/**
 *	Function for setting the bSkipUpdateDynamicDataDuringTick flag.
 */
native final function SetSkipUpdateDynamicDataDuringTick(bool bInSkipUpdateDynamicDataDuringTick);
/**
 *	Function for retrieving the bSkipUpdateDynamicDataDuringTick flag.
 */
native final function bool GetSkipUpdateDynamicDataDuringTick();

/**
 * SetKillOnDeactivate is used to set the KillOnDeactivate flag. If true, when
 * the particle system is deactivated, it will immediately kill the emitter
 * instance. If false, the emitter instance live particles will complete their
 * lifetime.
 *
 * Set this to true for cached ParticleSystems
 *
 *	@param	EmitterIndex		The index of the emitter to set it on
 *	@param	bKill				value to set KillOnDeactivate to
 */
native function SetKillOnDeactivate(int EmitterIndex, bool bKill);

/**
 * SetKillOnDeactivate is used to set the KillOnCompleted( flag. If true, when
 * the particle system is completed, it will immediately kill the emitter
 * instance.
 *
 * Set this to true for cached ParticleSystems
 *
 *	@param	EmitterIndex		The index of the emitter to set it on
 *	@param	bKill				The value to set it to
 **/
native function SetKillOnCompleted(int EmitterIndex, bool bKill);

/**
 * Rewind emitter instances.
 **/
native function RewindEmitterInstance(int EmitterIndex);
native function RewindEmitterInstances();

/**
 *	Beam-related script functions
 */
/**
 *	Set the beam type
 *
 *	@param	EmitterIndex		The index of the emitter to set it on
 *	@param	NewMethod			The new method/type of beam to generate
 */
native function SetBeamType(int EmitterIndex, int NewMethod);
/**
 *	Set the beam tessellation factor
 *
 *	@param	EmitterIndex		The index of the emitter to set it on
 *	@param	NewFactor			The value to set it to
 */
native function SetBeamTessellationFactor(int EmitterIndex, float NewFactor);
/**
 *	Set the beam end point
 *
 *	@param	EmitterIndex		The index of the emitter to set it on
 *	@param	NewEndPoint			The value to set it to
 */
native function SetBeamEndPoint(int EmitterIndex, vector NewEndPoint);
/**
 *	Set the beam distance
 *
 *	@param	EmitterIndex		The index of the emitter to set it on
 *	@param	Distance			The value to set it to
 */
native function SetBeamDistance(int EmitterIndex, float Distance);
/**
 *	Set the beam source point
 *
 *	@param	EmitterIndex		The index of the emitter to set it on
 *	@param	NewSourcePoint		The value to set it to
 *	@param	SourceIndex			Which beam within the emitter to set it on
 */
native function SetBeamSourcePoint(int EmitterIndex, vector NewSourcePoint, int SourceIndex);
/**
 *	Set the beam source tangent
 *
 *	@param	EmitterIndex		The index of the emitter to set it on
 *	@param	NewTangentPoint		The value to set it to
 *	@param	SourceIndex			Which beam within the emitter to set it on
 */
native function SetBeamSourceTangent(int EmitterIndex, vector NewTangentPoint, int SourceIndex);
/**
 *	Set the beam source strength
 *
 *	@param	EmitterIndex		The index of the emitter to set it on
 *	@param	NewSourceStrength	The value to set it to
 *	@param	SourceIndex			Which beam within the emitter to set it on
 */
native function SetBeamSourceStrength(int EmitterIndex, float NewSourceStrength, int SourceIndex);
/**
 *	Set the beam target point
 *
 *	@param	EmitterIndex		The index of the emitter to set it on
 *	@param	NewTargetPoint		The value to set it to
 *	@param	TargetIndex			Which beam within the emitter to set it on
 */
native function SetBeamTargetPoint(int EmitterIndex, vector NewTargetPoint, int TargetIndex);
/**
 *	Set the beam target tangent
 *
 *	@param	EmitterIndex		The index of the emitter to set it on
 *	@param	NewTangentPoint		The value to set it to
 *	@param	TargetIndex			Which beam within the emitter to set it on
 */
native function SetBeamTargetTangent(int EmitterIndex, vector NewTangentPoint, int TargetIndex);
/**
 *	Set the beam target strength
 *
 *	@param	EmitterIndex		The index of the emitter to set it on
 *	@param	NewTargetStrength	The value to set it to
 *	@param	TargetIndex			Which beam within the emitter to set it on
 */
native function SetBeamTargetStrength(int EmitterIndex, float NewTargetStrength, int TargetIndex);

cpptext
{
	// UObject interface
	virtual void PostLoad();
	virtual void BeginDestroy();
	virtual void FinishDestroy();
	virtual void PreEditChange(UProperty* PropertyThatWillChange);
	virtual void PostEditChange(UProperty* PropertyThatChanged);
	virtual void Serialize(FArchive& Ar);

	// Collision Handling...
	virtual UBOOL SingleLineCheck(FCheckResult& Hit, AActor* SourceActor, const FVector& End, const FVector& Start, DWORD TraceFlags, const FVector& Extent);

protected:
	// UActorComponent interface.
	virtual void Attach();
	virtual void UpdateTransform();
	virtual void Detach();
	virtual void UpdateDynamicData(FParticleSystemSceneProxy* Proxy);

public:
	virtual void UpdateDynamicData();

	// UPrimitiveComponent interface
	virtual void UpdateBounds();
	virtual void Tick(FLOAT DeltaTime);
	virtual void TickEditor(FLOAT DeltaTime);

	virtual FPrimitiveSceneProxy* CreateSceneProxy();

	// UParticleSystemComponent interface
	void InitParticles();
	void ResetParticles(UBOOL bEmptyInstances = FALSE);
	void ResetBurstLists();
	void UpdateInstances();
	UBOOL HasCompleted();

	void InitializeSystem();

	/**
	 *	Cache the view-relevance for each emitter at each LOD level.
	 *
	 *	@param	NewTemplate		The UParticleSystem* to use as the template.
	 *							If NULL, use the currently set template.
	 */
	void CacheViewRelevanceFlags(class UParticleSystem* NewTemplate = NULL);

	/**
	*	DetermineLODLevel - determines the appropriate LOD level to utilize.
	*/
	INT DetermineLODLevel(const FSceneView* View);

	// InstanceParameters interface
	UBOOL GetFloatParameter(const FName InName, FLOAT& OutFloat);
	UBOOL GetVectorParameter(const FName InName, FVector& OutVector);
	UBOOL GetMaterialParameter(const FName InName, UMaterialInterface*& OutMaterial);

	void	AutoPopulateInstanceProperties();
}

/**
 *	SetLODLevel - sets the LOD level to use for this instance.
 */
native final function			SetLODLevel(int InLODLevel);
native final function			SetEditorLODLevel(int InLODLevel);

/**
 *	GetLODLevel - gets the LOD level currently set.
 */
native final function int		GetLODLevel();
native final function int		GetEditorLODLevel();

native final function SetFloatParameter(name ParameterName, float Param);
native final function SetVectorParameter(name ParameterName, vector Param);
native final function SetColorParameter(name ParameterName, color Param);
native final function SetActorParameter(name ParameterName, actor Param);
native final function SetMaterialParameter(name ParameterName, MaterialInterface Param);

/** clears the specified parameter, returning it to the default value set in the template
 * @param ParameterName name of parameter to remove
 * @param ParameterType type of parameter to remove; if omitted or PSPT_None is specified, all parameters with the given name are removed
 */
native final function ClearParameter(name ParameterName, optional EParticleSysParamType ParameterType);

/** calls ActivateSystem() or DeactivateSystem() only if the component is not already activated/deactivated
 * necessary because ActivateSystem() resets already active emitters so it shouldn't be called multiple times on looping effects
 * @param bNowActive - whether the system should be active
 */
native final function SetActive(bool bNowActive);

/** stops the emitter, detaches the component, and resets the component's properties to the values of its template */
native final function ResetToDefaults();

defaultproperties
{
	bTickInEditor=true

	bAutoActivate=true
	bResetOnDetach=false
	OldPosition=(X=0,Y=0,Z=0)
	PartSysVelocity=(X=0,Y=0,Z=0)
	WarmupTime=0

	bSkipUpdateDynamicDataDuringTick=false

	TickGroup=TG_DuringAsyncWork
	
	bIsViewRelevanceDirty=true
}
