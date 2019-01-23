//=============================================================================
// ParticleEmitter
// The base class for any particle emitter objects.
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================
class ParticleEmitter extends Object
	native(Particle)
	dependson(ParticleEmitterInstance)
	dependson(ParticleLODLevel)
	dependson(ParticleEmitterInstance)
	collapsecategories
	hidecategories(Object)
	editinlinenew
	abstract;

//=============================================================================
//	General variables
//=============================================================================
var(Particle)							name						EmitterName;
var										bool						UseLocalSpace;
var										bool						KillOnDeactivate;
var										bool						bKillOnCompleted;

var										rawdistributionfloat		SpawnRate;
var										float						EmitterDuration;
var										int							EmitterLoops; // 0 indicates loop continuously

var(RequiredData) transient editinline	ParticleModuleRequired		RequiredModule;

//=============================================================================
//	Burst emissions
//=============================================================================
enum EParticleBurstMethod
{
	EPBM_Instant,
	EPBM_Interpolated
};

struct native ParticleBurst
{
	var()				int		Count;		// The number of particles to emit
	var()				int		CountLow;	// If > 0, use as a range [CountLow..Count]
	var()				float	Time;		// The time at which to emit them (0..1 - emitter lifetime)
};

var										EParticleBurstMethod		ParticleBurstMethod;
var						export noclear	array<ParticleBurst>		BurstList;

//=============================================================================
//	SubUV-related
//=============================================================================
enum EParticleSubUVInterpMethod
{
	PSUVIM_None,
    PSUVIM_Linear,
    PSUVIM_Linear_Blend,
    PSUVIM_Random,
    PSUVIM_Random_Blend
};

var										EParticleSubUVInterpMethod	InterpolationMethod;
var										int							SubImages_Horizontal;
var										int							SubImages_Vertical;
var										bool						ScaleUV;
var										float						RandomImageTime;
var										int							RandomImageChanges;
var										bool						DirectUV;
var	transient							int							SubUVDataOffset;

//=============================================================================
//	Cascade-related
//=============================================================================
enum EEmitterRenderMode
{
	ERM_Normal,
	ERM_Point,
	ERM_Cross,
	ERM_None
};

var									EEmitterRenderMode				EmitterRenderMode;
var									color							EmitterEditorColor;
var									bool							bEnabled;

//=============================================================================
//	'Private' data - not required by the editor
//=============================================================================
var editinline export				array<ParticleLODLevel>			LODLevels;
var editinline export				array<ParticleModule>			Modules;

// Module<SINGULAR> used for emitter type "extension".
var export							ParticleModule					TypeDataModule;
// Modules used for initialization.
var native							array<ParticleModule>			SpawnModules;
// Modules used for ticking.
var native							array<ParticleModule>			UpdateModules;
var									bool							ConvertedModules;
var									int								PeakActiveParticles;

//=============================================================================
//	Performance/LOD Data
//=============================================================================
var deprecated editfixedsize		array<float>					LODSpawnRatios;

/**
 *	Initial allocation count - overrides calculated peak count if > 0
 */
var(Particle)						int								InitialAllocationCount;

//=============================================================================
//	C++
//=============================================================================
cpptext
{
	virtual void PreEditChange(UProperty* PropertyThatWillChange);
	virtual void PostEditChange(UProperty* PropertyThatChanged);
	virtual FParticleEmitterInstance* CreateInstance(UParticleSystemComponent* InComponent);

	virtual void SetToSensibleDefaults() {}

	virtual void PostLoad();
	virtual void UpdateModuleLists();

	void SetEmitterName(FName Name);
	FName& GetEmitterName();
	virtual	void						SetLODCount(INT LODCount);

	// For Cascade
	void	AddEmitterCurvesToEditor(UInterpCurveEdSetup* EdSetup);
	void	RemoveEmitterCurvesFromEditor(UInterpCurveEdSetup* EdSetup);
	void	ChangeEditorColor(FColor& Color, UInterpCurveEdSetup* EdSetup);

	void	AutoPopulateInstanceProperties(UParticleSystemComponent* PSysComp);

	// LOD
	INT					CreateLODLevel(INT LODLevel, UBOOL bGenerateModuleData = TRUE);
	UBOOL				IsLODLevelValid(INT LODLevel);

	/** GetCurrentLODLevel
	*	Returns the currently set LODLevel. Intended for game-time usage.
	*	Assumes that the given LODLevel will be in the [0..# LOD levels] range.
	*	
	*	@return NULL if the requested LODLevel is not valid.
	*			The pointer to the requested UParticleLODLevel if valid.
	*/
	inline UParticleLODLevel* GetCurrentLODLevel(FParticleEmitterInstance* Instance)
	{
		// for the game (where we care about perf) we don't branch
		if (GIsGame == TRUE)
		{
			return Instance->CurrentLODLevel;
		}
		else
		{
			EditorUpdateCurrentLOD( Instance );
			return Instance->CurrentLODLevel;
		}

	}

	void EditorUpdateCurrentLOD(FParticleEmitterInstance* Instance);

	UParticleLODLevel*	GetLODLevel(INT LODLevel);
	UParticleLODLevel*	GetClosestLODLevel(INT LODLevel);
	UBOOL				GetClosestLODLevels(INT LODLevel, UParticleLODLevel*& HigherLOD, UParticleLODLevel*& LowerLOD);

	UBOOL				GetLODLevelModules(INT LODLevel, TArray<UParticleModule*>*& LevelModules);
	UBOOL				GetClosestLODLevelModules(INT LODLevel, TArray<UParticleModule*>*& HigherLODModule, TArray<UParticleModule*>*& LowerLODModules);
	UBOOL				GetLODLevelSpawnModules(INT LODLevel, TArray<UParticleModule*>*& LevelSpawnModules);
	UBOOL				GetClosestLODLevelSpawnModules(INT LODLevel, TArray<UParticleModule*>*& HigherLODSpawnModule, TArray<UParticleModule*>*& LowerLODSpawnModules);
	UBOOL				GetLODLevelUpdateModules(INT LODLevel, TArray<UParticleModule*>*& LevelUpdateModules);
	UBOOL				GetClosestLODLevelUpdateModules(INT LODLevel, TArray<UParticleModule*>*& HigherLODUpdateModule, TArray<UParticleModule*>*& LowerLODUpdateModules);
	
	virtual UBOOL		AutogenerateLowestLODLevel(UBOOL bDuplicateHighest = FALSE);
	
	/**
	 *	CalculateMaxActiveParticleCount
	 *	Determine the maximum active particles that could occur with this emitter.
	 *	This is to avoid reallocation during the life of the emitter.
	 *
	 *	@return	TRUE	if the number was determined
	 *			FALSE	if the number could not be determined
	 */
	virtual UBOOL		CalculateMaxActiveParticleCount();
}

//=============================================================================
//	Default properties
//=============================================================================
defaultproperties
{
	EmitterName="Particle Emitter"

	EmitterRenderMode=ERM_Normal

	ConvertedModules=true
	PeakActiveParticles=0

	EmitterDuration=1.0
	EmitterLoops=0

	EmitterEditorColor=(R=0,G=150,B=150)

	bEnabled=true
	
	Begin Object Class=DistributionFloatConstant Name=DistributionSpawnRate
	End Object
	SpawnRate=(Distribution=DistributionSpawnRate)

	InterpolationMethod=PSUVIM_None
	SubImages_Horizontal=1
	SubImages_Vertical=1
	RandomImageTime=1.0
	RandomImageChanges=0
	ScaleUV=true
	DirectUV=false
}
