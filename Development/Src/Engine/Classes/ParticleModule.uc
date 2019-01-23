/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ParticleModule extends Object
	native(Particle)
	editinlinenew
	collapsecategories
	hidecategories(Object)
	abstract;

struct native transient ParticleCurvePair
{
	var		string	CurveName;
	var		object	CurveObject;
};

/** If TRUE, the module performs operations on particles during Spawning		*/
var				bool			bSpawnModule;
/** If TRUE, the module performs operations on particles during Updating		*/
var				bool			bUpdateModule;
/** If TRUE, the module displays vector curves as colors						*/
var				bool			bCurvesAsColor;
/** If TRUE, the module should render its 3D visualization helper				*/
var				bool			b3DDrawMode;
/** If TRUE, the module supports rendering a 3D visualization helper			*/
var				bool			bSupported3DDrawMode;
/** If TRUE, the module is enabled												*/
var				bool			bEnabled;
/** If TRUE, the module has had editing enabled on it							*/
var				bool			bEditable;

/** The color to draw the modules curves in the curve editor. 
 *	If bCurvesAsColor is TRUE, it overrides this value.
 */
var(Cascade)	color			ModuleEditorColor;

/** ModuleType
 *	Indicates the kind of emitter the module can be applied to.
 *	ie, EPMT_Beam - only applies to beam emitters.
 *
 *	The TypeData field is present to speed up finding the TypeData module.
 */
enum EModuleType
{
	/** General - all emitter types can use it			*/
	EPMT_General,
	/** TypeData - TypeData modules						*/
	EPMT_TypeData,
	/** Beam - only applied to beam emitters			*/
	EPMT_Beam,
	/** Trail - only applied to trail emitters			*/
	EPMT_Trail
};

/** 
 *	Particle Selection Method, for any emitters that utilize particles
 *	as the source points.
 */
enum EParticleSourceSelectionMethod
{
	/** Random		- select a particle at random		*/
	EPSSM_Random,
	/** Sequential	- select a particle in order		*/
	EPSSM_Sequential
};

cpptext
{
	/**
	 *	Called on a particle that is freshly spawned by the emitter.
	 *	
	 *	@param	Owner		The FParticleEmitterInstance that spawned the particle.
	 *	@param	Offset		The modules offset into the data payload of the particle.
	 *	@param	SpawnTime	The time of the spawn.
	 */
	virtual void	Spawn(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime);
	/**
	 *	Called on a particle that is being updated by its emitter.
	 *	
	 *	@param	Owner		The FParticleEmitterInstance that 'owns' the particle.
	 *	@param	Offset		The modules offset into the data payload of the particle.
	 *	@param	DeltaTime	The time since the last update.
	 */
	virtual void	Update(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime);

	/**
	 *	Returns the number of bytes that the module requires in the particle payload block.
	 *
	 *	@param	Owner		The FParticleEmitterInstance that 'owns' the particle.
	 *
	 *	@return	UINT		The number of bytes the module needs per particle.
	 */
	virtual UINT	RequiredBytes(FParticleEmitterInstance* Owner = NULL);
	/**
	 *	Returns the number of bytes the module requires in the emitters 'per-instance' data block.
	 *	
	 *	@param	Owner		The FParticleEmitterInstance that 'owns' the particle.
	 *
	 *	@return UINT		The number fo bytes the module needs per emitter instance.
	 */
	virtual UINT	RequiredBytesPerInstance(FParticleEmitterInstance* Owner = NULL);

	// For Cascade
	/**
	 *	In the editor, called on a particle that is freshly spawned by the emitter.
	 *	Is responsible for performing the intepolation between LOD levels for realtime 
	 *	previewing.
	 *	
	 *	@param	Owner			The FParticleEmitterInstance that spawned the particle.
	 *	@param	Offset			The modules offset into the data payload of the particle.
	 *	@param	SpawnTime		The time of the spawn.
	 *	@param	LowerLODModule	The next lower LOD module - used during interpolation.
	 *	@param	Multiplier		The interpolation alpha value to use.
	 */
	virtual void	SpawnEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT SpawnTime, UParticleModule* LowerLODModule, FLOAT Multiplier);
	/**
	 *	In the editor, called on a particle that is being updated by its emitter.
	 *	Is responsible for performing the intepolation between LOD levels for realtime 
	 *	previewing.
	 *	
	 *	@param	Owner			The FParticleEmitterInstance that 'owns' the particle.
	 *	@param	Offset			The modules offset into the data payload of the particle.
	 *	@param	DeltaTime		The time since the last update.
	 *	@param	LowerLODModule	The next lower LOD module - used during interpolation.
	 *	@param	Multiplier		The interpolation alpha value to use.
	 */
	virtual void	UpdateEditor(FParticleEmitterInstance* Owner, INT Offset, FLOAT DeltaTime, UParticleModule* LowerLODModule, FLOAT Multiplier);

	/**
	 *	Called when the module is created, this function allows for setting values that make
	 *	sense for the type of emitter they are being used in.
	 *
	 *	@param	Owner			The UParticleEmitter that the module is being added to.
	 */
	virtual void SetToSensibleDefaults(UParticleEmitter* Owner);
	
	/** 
	 *	Fill an array with each Object property that fulfills the FCurveEdInterface interface.
	 *
	 *	@param	OutCurve	The array that should be filled in.
	 */
	virtual void	GetCurveObjects(TArray<FParticleCurvePair>& OutCurves);
	/** 
	 *	Add all curve-editable Objects within this module to the curve editor.
	 *
	 *	@param	EdSetup		The CurveEd setup to use for adding curved.
	 */
	virtual	void	AddModuleCurvesToEditor(UInterpCurveEdSetup* EdSetup);
	/** 
	 *	Remove all curve-editable Objects within this module from the curve editor.
	 *
	 *	@param	EdSetup		The CurveEd setup to remove the curve from.
	 */
	void	RemoveModuleCurvesFromEditor(UInterpCurveEdSetup* EdSetup);
	/** 
	 *	Does the module contain curves?
	 *
	 *	@return	UBOOL		TRUE if it does, FALSE if not.
	 */
	UBOOL	ModuleHasCurves();
	/** 
	 *	Are the modules curves displayed in the curve editor?
	 *
	 *	@param	EdSetup		The CurveEd setup to check.
	 *
	 *	@return	UBOOL		TRUE if they are, FALSE if not.
	 */
	UBOOL	IsDisplayedInCurveEd(UInterpCurveEdSetup* EdSetup);
	/** 
	 *	Helper function for updating the curve editor when the module editor color changes.
	 *
	 *	@param	Color		The new color the module is using.
	 *	@param	EdSetup		The CurveEd setup for the module.
	 */
	void	ChangeEditorColor(FColor& Color, UInterpCurveEdSetup* EdSetup);

	/** 
	 *	Render the modules 3D visualization helper primitive.
	 *
	 *	@param	Owner		The FParticleEmitterInstance that 'owns' the module.
	 *	@param	View		The scene view that is being rendered.
	 *	@param	PDI			The FPrimitiveDrawInterface to use for rendering.
	 */
	virtual void Render3DPreview(FParticleEmitterInstance* Owner, const FSceneView* View,FPrimitiveDrawInterface* PDI)	{};

	/**
	 *	Retrieve the ModuleType of this module.
	 *
	 *	@return	EModuleType		The type of module this is.
	 */
	virtual EModuleType	GetModuleType() const	{	return EPMT_General;	}

	/**
	 *	Helper function used by the editor to auto-populate a placed AEmitter with any
	 *	instance parameters that are utilized.
	 *
	 *	@param	PSysComp		The particle system component to be populated.
	 */
	virtual void	AutoPopulateInstanceProperties(UParticleSystemComponent* PSysComp);
	
	/**
	 *	Helper function used by the editor to auto-generate LOD values from a source module
	 *	and a percentage value used to scale its values.
	 *
	 *	@param	SourceModule	The ParticleModule to utilize as the template.
	 *	@param	Percentage		The value to use when scaling the source values.
	 */
	virtual UBOOL	GenerateLODModuleValues(UParticleModule* SourceModule, FLOAT Percentage, UParticleLODLevel* LODLevel);

	/**
	 *	Conversion functions for distributions.
	 *
	 *	Used to convert from an old representation of data (rotations in degrees, etc.)
	 *	to new ones (rotations in Turns/Sec, etc.).
	 */
	virtual UBOOL	ConvertFloatDistribution(UDistributionFloat* FloatDist, UDistributionFloat* SourceFloatDist, FLOAT Percentage);
	virtual UBOOL	ConvertVectorDistribution(UDistributionVector* VectorDist, UDistributionVector* SourceVectorDist, FLOAT Percentage);
	virtual UBOOL	ConvertColorFloatDistribution(UDistributionFloat* FloatDist);
	virtual UBOOL	ConvertColorVectorDistribution(UDistributionVector* VectorDist);
}

defaultproperties
{
	bSupported3DDrawMode=false
	b3DDrawMode=false
	bEnabled=true
	bEditable=true
}
