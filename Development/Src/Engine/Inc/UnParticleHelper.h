/*=============================================================================
	UnParticleHelper.h: Particle helper definitions/ macros.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef HEADER_UNPARTICLEHELPER
#define HEADER_UNPARTICLEHELPER

#define _ENABLE_PARTICLE_LOD_INGAME_

/*-----------------------------------------------------------------------------
	Helper macros.
-----------------------------------------------------------------------------*/
//	Macro fun.
#define _PARTICLES_USE_PREFETCH_
#if defined(_PARTICLES_USE_PREFETCH_)
	#define	PARTICLE_PREFETCH(Index)					PREFETCH( ParticleData + ParticleStride * ParticleIndices[Index] )
	#define PARTICLE_INSTANCE_PREFETCH(Instance, Index)	PREFETCH( Instance->ParticleData + Instance->ParticleStride * Instance->ParticleIndices[Index] )
	#define	PARTICLE_OWNER_PREFETCH(Index)				PREFETCH( Owner->ParticleData + Owner->ParticleStride * Owner->ParticleIndices[Index] )
#else	//#if defined(_PARTICLES_USE_PREFETCH_)
	#define	PARTICLE_PREFETCH(Index)					
	#define	PARTICLE_INSTANCE_PREFETCH(Instance, Index)	
	#define	PARTICLE_OWNER_PREFETCH(Index)				
#endif	//#if defined(_PARTICLES_USE_PREFETCH_)

#define DECLARE_PARTICLE(Name,Address)		\
	FBaseParticle& Name = *((FBaseParticle*) (Address));

#define DECLARE_PARTICLE_PTR(Name,Address)		\
	FBaseParticle* Name = (FBaseParticle*) (Address);

#define BEGIN_UPDATE_LOOP																								\
	{																													\
		INT&			ActiveParticles = Owner->ActiveParticles;														\
		UINT			CurrentOffset	= Offset;																		\
		const BYTE*		ParticleData	= Owner->ParticleData;															\
		const UINT		ParticleStride	= Owner->ParticleStride;														\
		WORD*			ParticleIndices	= Owner->ParticleIndices;														\
		for(INT i=ActiveParticles-1; i>=0; i--)																			\
		{																												\
			const INT	CurrentIndex	= ParticleIndices[i];															\
			const BYTE* ParticleBase	= ParticleData + CurrentIndex * ParticleStride;									\
			FBaseParticle& Particle		= *((FBaseParticle*) ParticleBase);												\
			if ((Particle.Flags & STATE_Particle_Freeze) == 0)															\
			{																											\

#define END_UPDATE_LOOP																									\
			}																											\
			CurrentOffset				= Offset;																		\
		}																												\
	}

#define CONTINUE_UPDATE_LOOP																							\
		CurrentOffset = Offset;																							\
		continue;

#define SPAWN_INIT																										\
	const INT		ActiveParticles	= Owner->ActiveParticles;															\
	const UINT		ParticleStride	= Owner->ParticleStride;															\
	const BYTE*		ParticleBase	= Owner->ParticleData + Owner->ParticleIndices[ActiveParticles] * ParticleStride;	\
	UINT			CurrentOffset	= Offset;																			\
	FBaseParticle&	Particle		= *((FBaseParticle*) ParticleBase);

#define PARTICLE_ELEMENT(Type,Name)																						\
	Type& Name = *((Type*)(ParticleBase + CurrentOffset));																\
	CurrentOffset += sizeof(Type);

#define KILL_CURRENT_PARTICLE																							\
	{																													\
		ParticleIndices[i]					= ParticleIndices[ActiveParticles-1];										\
		ParticleIndices[ActiveParticles-1]	= CurrentIndex;																\
		ActiveParticles--;																								\
	}

/*-----------------------------------------------------------------------------
	Helper functions.
-----------------------------------------------------------------------------*/

static FLinearColor ColorFromVector(const FVector& ColorVec, const FLOAT fAlpha);

/*-----------------------------------------------------------------------------
	Forward declarations
-----------------------------------------------------------------------------*/
//	Emitter and module types
class UParticleEmitter;
class UParticleSpriteEmitter;
class UParticleMeshEmitter;
class UParticleSpriteSubUVEmitter;
class UParticleModule;
// Data types
class UParticleModuleTypeDataSubUV;
class UParticleModuleTypeDataMesh;
class UParticleModuleTypeDataTrail;
class UParticleModuleTypeDataBeam;
class UParticleModuleTypeDataBeam2;
class UParticleModuleTypeDataTrail2;

class UStaticMeshComponent;
class UParticleSystemComponent;

class UParticleSpriteEmitterInstance;
class UParticleSpriteSubUVEmitterInstance;
class UParticleTrailEmitterInstance;
class UParticleBeamEmitterInstance;

class UParticleBeam2EmitterInstance;
class UParticleModuleBeamSource;
class UParticleModuleBeamTarget;
class UParticleModuleBeamNoise;
class UParticleModuleBeamModifier;

class UParticleTrail2EmitterInstance;
class UParticleModuleTrailSource;
class UParticleModuleTrailSpawn;
class UParticleModuleTrailTaper;

class UParticleModuleOrientationAxisLock;

class UParticleLODLevel;

class FParticleSystemSceneProxy;
class FParticleDynamicData;
struct FDynamicBeam2EmitterData;
struct FDynamicTrail2EmitterData;

struct FParticleSpriteEmitterInstance;
struct FParticleSpriteSubUVEmitterInstance;
struct FParticleMeshEmitterInstance;
struct FParticleTrailEmitterInstance;
struct FParticleBeamEmitterInstance;
struct FParticleBeam2EmitterInstance;
struct FParticleTrail2EmitterInstance;

void PS_DumpBeamDataInformation(TCHAR* Message, 
	UParticleSystemComponent* PSysComp, FParticleSystemSceneProxy* Proxy, 
	FParticleDynamicData* NewPSDynamicData, FParticleDynamicData* OldPSDynamicData, 
	FDynamicBeam2EmitterData* NewBeamData, FDynamicBeam2EmitterData* OldBeamData);
void PS_DumpTrailDataInformation(TCHAR* Message, 
	UParticleSystemComponent* PSysComp, FParticleSystemSceneProxy* Proxy, 
	FParticleDynamicData* NewPSDynamicData, FParticleDynamicData* OldPSDynamicData, 
	FDynamicTrail2EmitterData* NewTrailData, FDynamicTrail2EmitterData* OldTrailData);

/*-----------------------------------------------------------------------------
	FBaseParticle
-----------------------------------------------------------------------------*/
// Mappings for 'standard' particle data
// Only used when required.
struct FBaseParticle
{
	// 16 bytes
	FVector			OldLocation;			// Last frame's location, used for collision
	FLOAT			RelativeTime;			// Relative time, range is 0 (==spawn) to 1 (==death)

	// 16 bytes
	FVector			Location;				// Current location
	FLOAT			OneOverMaxLifetime;		// Reciprocal of lifetime

	// 16 bytes
	FVector			BaseVelocity;			// Velocity = BaseVelocity at the start of each frame.
	FLOAT			Rotation;				// Rotation of particle (in Radians)

	// 16 bytes
	FVector			Velocity;				// Current velocity, gets reset to BaseVelocity each frame to allow 
	FLOAT			BaseRotationRate;		// Initial angular velocity of particle (in Radians per second)

	// 16 bytes
	FVector			BaseSize;				// Size = BaseSize at the start of each frame
	FLOAT			RotationRate;			// Current rotation rate, gets reset to BaseRotationRate each frame

	// 16 bytes
	FVector			Size;					// Current size, gets reset to BaseSize each frame
	INT				Flags;					// Flags indicating various particle states

	// 16 bytes
	FLinearColor	Color;					// Current color of particle.

	// 16 bytes
	FLinearColor	BaseColor;				// Base color of the particle
};

/*-----------------------------------------------------------------------------
	Particle State Flags
-----------------------------------------------------------------------------*/
enum EParticleStates
{
	/** Ignore updates to the particle						*/
	STATE_Particle_Freeze				= 0x00000001,
	/** Ignore collision updates to the particle			*/
	STATE_Particle_IgnoreCollisions		= 0x00000002,
	/**	Stop translations of the particle					*/
	STATE_Particle_FreezeTranslation	= 0x00000004,
	/**	Stop rotations of the particle						*/
	STATE_Particle_FreezeRotation		= 0x00000008,
	/** Delay collision updates to the particle				*/
	STATE_Particle_DelayCollisions		= 0x00000010,
};

/*-----------------------------------------------------------------------------
	FParticlesStatGroup
-----------------------------------------------------------------------------*/
enum EParticleStats
{
	STAT_SpriteParticles = STAT_ParticlesFirstStat,
	STAT_SpriteParticlesRenderCalls,
	STAT_SpriteParticlesRenderCallsCompleted,
	STAT_SpriteParticlesSpawned,
	STAT_SpriteParticlesUpdated,
	STAT_SpriteParticlesKilled,
	STAT_MeshParticles, 
	STAT_SortingTime,
	STAT_SpriteRenderingTime,
	STAT_SpriteResourceUpdateTime,
	STAT_SpriteTickTime,
	STAT_SpriteSpawnTime,
	STAT_SpriteUpdateTime,
	STAT_PSysCompTickTime,
	STAT_ParticleCollisionTime,
};

//	FParticleSpriteVertex
struct FParticleSpriteVertex
{
	/** The position of the particle					*/
	FVector			Position;
	/** The previous position of the particle			*/
	FVector			OldPosition;
	/** The size of the particle						*/
	FVector			Size;
	/** The UV values of the particle					*/
	FLOAT			Tex_U;
	FLOAT			Tex_V;
	/** The rotation of the particle					*/
	FLOAT			Rotation;
	/** The color of the particle						*/
	FLinearColor	Color;
};

//	FParticleSpriteSubVertex
struct FParticleSpriteSubUVVertex : public FParticleSpriteVertex
{
	/** The second UV set for the particle				*/
	FLOAT			Tex_U2;
	FLOAT			Tex_V2;
	/** The interpolation value							*/
	FLOAT			Interp;
	/** Padding...										*/
	FLOAT			Padding;
	/** The size of the sub-image						*/
	FLOAT			SizeU;
	FLOAT			SizeV;
};

//
//  Trail emitter flags and macros
//
// ForceKill: Indicates all the particles in the trail should be killed in the next KillParticles call.
#define TRAIL_EMITTER_FLAG_FORCEKILL	0x00000000
// DeadTrail: indicates that the particle is the start of a trail than should no longer spawn.
//			  It should just fade out as the particles die...
#define TRAIL_EMITTER_FLAG_DEADTRAIL	0x10000000
// Middle: indicates the particle is in the middle of a trail.
#define TRAIL_EMITTER_FLAG_MIDDLE       0x20000000
// Start: indicates the particle is the start of a trail.
#define TRAIL_EMITTER_FLAG_START        0x40000000
// End: indicates the particle is the end of a trail.
#define TRAIL_EMITTER_FLAG_END          0x80000000

//#define TRAIL_EMITTER_FLAG_ONLY	        (TRAIL_EMITTER_FLAG_START | TRAIL_EMITTER_FLAG_END)
#define TRAIL_EMITTER_FLAG_MASK         0xf0000000
#define TRAIL_EMITTER_PREV_MASK         0x0fffc000
#define TRAIL_EMITTER_PREV_SHIFT        14
#define TRAIL_EMITTER_NEXT_MASK         0x00003fff
#define TRAIL_EMITTER_NEXT_SHIFT        0

#define TRAIL_EMITTER_NULL_PREV			(TRAIL_EMITTER_PREV_MASK >> TRAIL_EMITTER_PREV_SHIFT)
#define TRAIL_EMITTER_NULL_NEXT			(TRAIL_EMITTER_NEXT_MASK >> TRAIL_EMITTER_NEXT_SHIFT)

// Helper macros
#define TRAIL_EMITTER_CHECK_FLAG(val, mask, flag)				((val & mask) == flag)
#define TRAIL_EMITTER_SET_FLAG(val, mask, flag)					((val & ~mask) | flag)
#define TRAIL_EMITTER_GET_PREVNEXT(val, mask, shift)			((val & mask) >> shift)
#define TRAIL_EMITTER_SET_PREVNEXT(val, mask, shift, setval)	((val & ~mask) | ((setval << shift) & mask))

// Start/end accessor macros
#define TRAIL_EMITTER_IS_START(index)       TRAIL_EMITTER_CHECK_FLAG(index, TRAIL_EMITTER_FLAG_MASK, TRAIL_EMITTER_FLAG_START)
#define TRAIL_EMITTER_SET_START(index)      TRAIL_EMITTER_SET_FLAG(index, TRAIL_EMITTER_FLAG_MASK, TRAIL_EMITTER_FLAG_START)

#define TRAIL_EMITTER_IS_END(index)			TRAIL_EMITTER_CHECK_FLAG(index, TRAIL_EMITTER_FLAG_MASK, TRAIL_EMITTER_FLAG_END)
#define TRAIL_EMITTER_SET_END(index)		TRAIL_EMITTER_SET_FLAG(index, TRAIL_EMITTER_FLAG_MASK, TRAIL_EMITTER_FLAG_END)

#define TRAIL_EMITTER_IS_MIDDLE(index)		TRAIL_EMITTER_CHECK_FLAG(index, TRAIL_EMITTER_FLAG_MASK, TRAIL_EMITTER_FLAG_MIDDLE)
#define TRAIL_EMITTER_SET_MIDDLE(index)		TRAIL_EMITTER_SET_FLAG(index, TRAIL_EMITTER_FLAG_MASK, TRAIL_EMITTER_FLAG_MIDDLE)

// Only is used for the first emission from the emitter
#define TRAIL_EMITTER_IS_ONLY(index)		TRAIL_EMITTER_CHECK_FLAG(index, TRAIL_EMITTER_FLAG_MASK, TRAIL_EMITTER_FLAG_START)	&& \
											(TRAIL_EMITTER_GET_NEXT(index) == TRAIL_EMITTER_NULL_NEXT)
#define TRAIL_EMITTER_SET_ONLY(index)		TRAIL_EMITTER_SET_FLAG(index, TRAIL_EMITTER_FLAG_MASK, TRAIL_EMITTER_FLAG_START);	\
											TRAIL_EMITTER_SET_START(index)

#define TRAIL_EMITTER_IS_FORCEKILL(index)	TRAIL_EMITTER_CHECK_FLAG(index, TRAIL_EMITTER_FLAG_MASK, TRAIL_EMITTER_FLAG_FORCEKILL)
#define TRAIL_EMITTER_SET_FORCEKILL(index)	TRAIL_EMITTER_SET_FLAG(index, TRAIL_EMITTER_FLAG_MASK, TRAIL_EMITTER_FLAG_FORCEKILL)

// Prev/Next accessor macros
#define TRAIL_EMITTER_GET_PREV(index)       TRAIL_EMITTER_GET_PREVNEXT(index, TRAIL_EMITTER_PREV_MASK, TRAIL_EMITTER_PREV_SHIFT)
#define TRAIL_EMITTER_SET_PREV(index, prev) TRAIL_EMITTER_SET_PREVNEXT(index, TRAIL_EMITTER_PREV_MASK, TRAIL_EMITTER_PREV_SHIFT, prev)
#define TRAIL_EMITTER_GET_NEXT(index)       TRAIL_EMITTER_GET_PREVNEXT(index, TRAIL_EMITTER_NEXT_MASK, TRAIL_EMITTER_NEXT_SHIFT)
#define TRAIL_EMITTER_SET_NEXT(index, next) TRAIL_EMITTER_SET_PREVNEXT(index, TRAIL_EMITTER_NEXT_MASK, TRAIL_EMITTER_NEXT_SHIFT, next)

/**
 * Particle trail stats
 */
enum EParticleTrailStats
{
	STAT_TrailParticles = STAT_TrailParticlesFirstStat,
	STAT_TrailParticlesRenderCalls,
	STAT_TrailParticlesRenderCallsCompleted,
	STAT_TrailParticlesSpawned,
	STAT_TrailParticlesUpdateCalls,
	STAT_TrailParticlesUpdated,
	STAT_TrailParticlesKilled,
	STAT_TrailParticlesTrianglesRendered,
	STAT_TrailVBPackingTime,
	STAT_TrailIBPackingTime,
	STAT_TrailRenderingTime,
	STAT_TrailResourceUpdateTime,
	STAT_TrailTickTime,
	STAT_TrailSpawnTime,
	STAT_TrailUpdateTime,
	STAT_TrailPSysCompTickTime
};

/**
 * Beam particle stats
 */
enum EBeamParticleStats
{
	STAT_BeamParticles = STAT_BeamParticlesFirstStat,
	STAT_BeamParticlesRenderCalls,
	STAT_BeamParticlesRenderCallsCompleted,
	STAT_BeamParticlesSpawned,
	STAT_BeamParticlesUpdateCalls,
	STAT_BeamParticlesUpdated,
	STAT_BeamParticlesKilled,
	STAT_BeamVBPackingTime,
	STAT_BeamIBPackingTime,
	STAT_BeamRenderingTime,
	STAT_BeamResourceUpdateTime,
	STAT_BeamTickTime,
	STAT_BeamSpawnTime,
	STAT_BeamUpdateTime,
	STAT_BeamPSysCompTickTime
};

#define _BEAM2_USE_MODULES_

/** Structure for multiple beam targets.								*/
struct FBeamTargetData
{
	/** Name of the target.												*/
	FName		TargetName;
	/** Percentage chance the target will be selected (100 = always).	*/
	FLOAT		TargetPercentage;
};

//
//	Helper structures for payload data...
//

//
//	SubUV-related payloads
//
struct FSubUVMeshPayload
{
	FVector	UVOffset;
	FVector	UV2Offset;
};

struct FSubUVMeshRandomPayload : public FSubUVMeshPayload
{
	FLOAT	RandomImageTime;
};

struct FSubUVSpritePayload
{
	FLOAT	Interpolation;
	FLOAT	ImageH;
	FLOAT	ImageV;
	FLOAT	Image2H;
	FLOAT	Image2V;
};

struct FSubUVSpriteRandomPayload : public FSubUVSpritePayload
{
	FLOAT	RandomImageTime;
};

//
//	AttractorParticle
//
struct FAttractorParticlePayload
{
	INT			SourceIndex;
	UINT		SourcePointer;
	FVector		SourceVelocity;
};

//
//	TypeDataBeam2 payload
//
#define BEAM2_TYPEDATA_LOCKED_MASK					0x80000000
#define	BEAM2_TYPEDATA_LOCKED(x)					((x & BEAM2_TYPEDATA_LOCKED_MASK) != 0)
#define	BEAM2_TYPEDATA_SETLOCKED(x, Locked)			(x = Locked ? (x | BEAM2_TYPEDATA_LOCKED_MASK) : (x & ~BEAM2_TYPEDATA_LOCKED_MASK))

#define BEAM2_TYPEDATA_NOISEMAX_MASK				0x40000000
#define	BEAM2_TYPEDATA_NOISEMAX(x)					((x & BEAM2_TYPEDATA_NOISEMAX_MASK) != 0)
#define	BEAM2_TYPEDATA_SETNOISEMAX(x, Max)			(x = Max ? (x | BEAM2_TYPEDATA_NOISEMAX_MASK) : (x & ~BEAM2_TYPEDATA_NOISEMAX_MASK))

#define BEAM2_TYPEDATA_NOISEPOINTS_MASK				0x00000fff
#define	BEAM2_TYPEDATA_NOISEPOINTS(x)				(x & BEAM2_TYPEDATA_NOISEPOINTS_MASK)
#define BEAM2_TYPEDATA_SETNOISEPOINTS(x, Count)		(x = (x & ~BEAM2_TYPEDATA_NOISEPOINTS_MASK) | Count)

#define BEAM2_TYPEDATA_FREQUENCY_MASK				0x00fff000
#define BEAM2_TYPEDATA_FREQUENCY_SHIFT				12
#define	BEAM2_TYPEDATA_FREQUENCY(x)					((x & BEAM2_TYPEDATA_FREQUENCY_MASK) >> BEAM2_TYPEDATA_FREQUENCY_SHIFT)
#define BEAM2_TYPEDATA_SETFREQUENCY(x, Freq)		(x = ((x & ~BEAM2_TYPEDATA_FREQUENCY_MASK) | (Freq << BEAM2_TYPEDATA_FREQUENCY_SHIFT)))

struct FBeam2TypeDataPayload
{
	/** The source of this beam											*/
	FVector		SourcePoint;
	/** The source tangent of this beam									*/
	FVector		SourceTangent;
	/** The stength of the source tangent of this beam					*/
	FLOAT		SourceStrength;

	/** The target of this beam											*/
	FVector		TargetPoint;
	/** The target tangent of this beam									*/
	FVector		TargetTangent;
	/** The stength of the Target tangent of this beam					*/
	FLOAT		TargetStrength;

	/** Target lock, extreme max, Number of noise points				*/
	INT			Lock_Max_NumNoisePoints;

	/** Number of segments to render (steps)							*/
	INT			InterpolationSteps;

	/** Direction to step in											*/
	FVector		Direction;
	/** StepSize (for each segment to be rendered)						*/
	FLOAT		StepSize;
	/** Number of segments to render (steps)							*/
	INT			Steps;
	/** The 'extra' amount to travel (partial segment)					*/
	FLOAT		TravelRatio;

	/** The number of triangles to render for this beam					*/
	INT			TriangleCount;

	/**
	 *	Type and indexing flags
	 * 3               1              0
	 * 1...|...|...|...5...|...|...|..0
	 * TtPppppppppppppppNnnnnnnnnnnnnnn
	 * Tt				= Type flags --> 00 = Middle of Beam (nothing...)
	 * 									 01 = Start of Beam
	 * 									 10 = End of Beam
	 * Ppppppppppppppp	= Previous index
	 * Nnnnnnnnnnnnnnn	= Next index
	 * 		INT				Flags;
	 * 
	 * NOTE: These values DO NOT get packed into the vertex buffer!
	 */
	INT			Flags;
};

/**	Particle Source/Target Data Payload									*/
struct FBeamParticleSourceTargetPayloadData
{
	INT			ParticleIndex;
};

/**	Particle Source Branch Payload										*/
struct FBeamParticleSourceBranchPayloadData
{
	INT			NoiseIndex;
};

/** Particle Beam Modifier Data Payload */
struct FBeamParticleModifierPayloadData
{
	BITFIELD	bModifyPosition:1;
	BITFIELD	bScalePosition:1;
	BITFIELD	bModifyTangent:1;
	BITFIELD	bScaleTangent:1;
	BITFIELD	bModifyStrength:1;
	BITFIELD	bScaleStrength:1;
	FVector		Position;
	FVector		Tangent;
	FLOAT		Strength;

	// Helper functions
	FORCEINLINE void UpdatePosition(FVector& Value)
	{
		if (bModifyPosition == TRUE)
		{
			if (bScalePosition == FALSE)
			{
				Value += Position;
			}
			else
			{
				Value *= Position;
			}
		}
	}

	FORCEINLINE void UpdateTangent(FVector& Value, UBOOL bAbsolute)
	{
		if (bModifyTangent == TRUE)
		{
			FVector ModTangent = Tangent;

			if (bAbsolute == FALSE)
			{
				// Transform the modified tangent so it is relative to the real tangent
				FQuat RotQuat = FQuatFindBetween(FVector(1.0f, 0.0f, 0.0f), Value);
				FMatrix RotMat = FQuatRotationTranslationMatrix(RotQuat, FVector(0.0f));

				ModTangent = RotMat.TransformNormal(Tangent);
			}

			if (bScaleTangent == FALSE)
			{
				Value += ModTangent;
			}
			else
			{
				Value *= ModTangent;
			}
		}
	}

	FORCEINLINE void UpdateStrength(FLOAT& Value)
	{
		if (bModifyStrength == TRUE)
		{
			if (bScaleStrength == FALSE)
			{
				Value += Strength;
			}
			else
			{
				Value *= Strength;
			}
		}
	}
};

//
//	Trail2 payload data
//
struct FTrail2TypeDataPayload
{
	/**
	 *	Type and indexing flags
	 * 3               1              0
	 * 1...|...|...|...5...|...|...|..0
	 * TtPppppppppppppppNnnnnnnnnnnnnnn
	 * Tt				= Type flags --> 00 = Middle of Beam (nothing...)
	 * 									 01 = Start of Beam
	 * 									 10 = End of Beam
	 * Ppppppppppppppp	= Previous index
	 * Nnnnnnnnnnnnnnn	= Next index
	 * 		INT				Flags;
	 * 
	 * NOTE: These values DO NOT get packed into the vertex buffer!
	 */
	INT			Flags;

	/** The trail index - START only							*/
	INT			TrailIndex;
	/** The number of triangle in the trail	- START only		*/
	INT			TriangleCount;
	/** The velocity of the particle - to allow moving trails	*/
	FVector		Velocity;
	/**	Tangent for the trail segment							*/
	FVector		Tangent;
};

/**	Particle Source Data Payload									*/
struct FTrailParticleSourcePayloadData
{
	INT			ParticleIndex;
};

/** Mesh rotation data payload										*/
struct FMeshRotationPayloadData
{
	FVector  Rotation;
	FVector  RotationRate;
	FVector  RotationRateBase;
};

/** ModuleLocationEmitter instance payload							*/
struct FLocationEmitterInstancePayload
{
	INT		LastSelectedIndex;
};

/**
 *	Chain-able Orbit module instance payload
 */
struct FOrbitChainModuleInstancePayload
{
	/** The base offset of the particle from it's tracked location	*/
	FVector	BaseOffset;
	/** The offset of the particle from it's tracked location		*/
	FVector	Offset;
	/** The rotation of the particle at it's offset location		*/
	FVector	Rotation;
	/** The base rotation rate of the particle offset				*/
	FVector	BaseRotationRate;
	/** The rotation rate of the particle offset					*/
	FVector	RotationRate;
	/** The offset of the particle from the last frame				*/
	FVector	PreviousOffset;
};

/**
 *	Payload for instances which use the SpawnPerUnit module.
 */
struct FParticleSpawnPerUnitInstancePayload
{
	FLOAT	CurrentDistanceTravelled;
};

/**
 *	Collision module particle payload
 */
struct FParticleCollisionPayload
{
	FVector	UsedDampingFactor;
	FVector	UsedDampingFactorRotation;
	INT		UsedCollisions;
	FLOAT	Delay;
};

/*-----------------------------------------------------------------------------
	Particle Sorting Helper
-----------------------------------------------------------------------------*/
struct FParticleOrder
{
	INT		ParticleIndex;
	FLOAT	Z;
	
	FParticleOrder(INT InParticleIndex,FLOAT InZ):
		ParticleIndex(InParticleIndex),
		Z(InZ)
	{}
};

/*-----------------------------------------------------------------------------
	Particle Dynamic Data
-----------------------------------------------------------------------------*/
/**
 *	The information required for rendering sprite-based particles
 */
struct FParticleSpriteData
{
	/** Current location of the particle.			*/
	FVector			Location;
	/** Last frame's location of the particle.		*/
	FVector			OldLocation;
	/** Rotation of the particle (in Radians).		*/
	FLOAT			Rotation;
	/** Current size of the particle.				*/
	FVector			Size;
	/** Current color of the particle.				*/
	FLinearColor	Color;
};

/** */
struct FDynamicEmitterDataBase
{
	enum EmitterType
	{
		DET_Unknown = 0,
		DET_Sprite,
		DET_SubUV,
		DET_Mesh,
		DET_Beam,
		DET_Beam2,
		DET_Trail,
		DET_Trail2,
		DET_Custom
	};

	FDynamicEmitterDataBase(INT InParticleCount, INT InParticleStride, UBOOL InbRequiresSorting ) :
		  eEmitterType(DET_Unknown)
		, ActiveParticleCount(0)
		, ParticleCount(InParticleCount)
		, ParticleStride(InParticleStride)
		, ParticleData(NULL)
		, ParticleIndices(NULL)
		, Scale(FVector(1.0f))
		, bSelected(FALSE)
		, bRequiresSorting(InbRequiresSorting)
		, SceneProxy(NULL)
	{
	}
	
	virtual ~FDynamicEmitterDataBase()
	{
		appFree(ParticleData);
		appFree(ParticleIndices);
	}

	/**
	* @return TRUE if the particle system uses FMeshElement.UseDynamicData=TRUE
	*/
	virtual UBOOL HasDynamicMeshElementData()
	{
		// by default all emitters assumed to use dynamic vertex data
		return TRUE;
	}

	// Render thread only draw call
	virtual void Render(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex) = 0;

	/**	The type of emitter.											*/
	EmitterType	eEmitterType;

	/**	The number of particles currently active in this emitter.		*/
	INT			ActiveParticleCount;
	INT			ParticleCount;
	INT			ParticleStride;
	BYTE*		ParticleData;
	WORD*		ParticleIndices;
	FVector		Scale;
	BITFIELD	bSelected : 1;
	/** Whether this emitter requires sorting as specified by artist.	*/
	BITFIELD	bRequiresSorting : 1;
	/** The scene proxy - only used during rendering!					*/
	FParticleSystemSceneProxy* SceneProxy;
};

struct FDynamicSpriteEmitterDataBase : public FDynamicEmitterDataBase
{
	FDynamicSpriteEmitterDataBase(INT InParticleCount, INT InParticleStride, UBOOL InbRequiresSorting, const FMaterialRenderProxy* InMaterialResource, INT InMaxDrawCount) :
		  FDynamicEmitterDataBase(InParticleCount, InParticleStride, InbRequiresSorting)
		, MaterialResource(InMaterialResource)
		, ScreenAlignment(0)
		, bUseLocalSpace(FALSE)
		, bLockAxis(FALSE)
		, LockAxisFlag(0)
		, MaxDrawCount(InMaxDrawCount)
		, OrbitModuleOffset(0)
	{
	}

	virtual ~FDynamicSpriteEmitterDataBase()
	{
	}

	virtual void RenderDebug(FPrimitiveDrawInterface* PDI, const FSceneView* View, UINT DPGIndex, UBOOL bCrosses);

	const FMaterialRenderProxy*	MaterialResource;
	BYTE						ScreenAlignment;
	UBOOL						bUseLocalSpace;
	UBOOL						bLockAxis;
	BYTE						LockAxisFlag;
	INT							MaxDrawCount;
	INT							EmitterRenderMode;
	INT							OrbitModuleOffset;
};

/** */
struct FDynamicSpriteEmitterData : public FDynamicSpriteEmitterDataBase
{
	FDynamicSpriteEmitterData(INT InParticleCount, INT InParticleStride, UBOOL InbRequiresSorting, const FMaterialRenderProxy* InMaterialResource, INT InMaxDrawCount = -1) :
		  FDynamicSpriteEmitterDataBase(InParticleCount, InParticleStride, InbRequiresSorting, InMaterialResource, InMaxDrawCount)
		, PrimitiveCount(0)
		, VertexCount(0)
		, VertexData(NULL)
		, IndexCount(0)
		, IndexStride(0)
		, IndexData(NULL)
		, VertexFactory(NULL)
	{
		eEmitterType = DET_Sprite;
		if (InParticleCount > 0)
		{
			ParticleData = (BYTE*)appRealloc(ParticleData, InParticleCount * InParticleStride);
			check(ParticleData);
			ParticleIndices = (WORD*)appRealloc(ParticleIndices, InParticleCount * sizeof(WORD));
			check(ParticleIndices);
		}
	}

	virtual ~FDynamicSpriteEmitterData()
	{
		appFree(VertexData);
		appFree(IndexData);
		if(VertexFactory)
		{
			VertexFactory->Release();
			delete VertexFactory;
		}
	}

	virtual UBOOL GetVertexAndIndexData(FParticleSpriteVertex* VertexData, void* FillIndexData, TArray<FParticleOrder>* ParticleOrder);

	// Render thread only draw call
	virtual void Render(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex);

	/**	The sprite particle data.										*/
	INT							PrimitiveCount;
	INT							VertexCount;
	FParticleSpriteVertex*		VertexData;			// RENDER-THREAD USAGE ONLY!!!
	INT							IndexCount;
	INT							IndexStride;
	void*						IndexData;			// RENDER-THREAD USAGE ONLY!!!

	TArray<FParticleOrder>		ParticleOrder;

	FParticleVertexFactory*		VertexFactory;		// RENDER-THREAD USAGE ONLY!!!

};

struct FDynamicSubUVEmitterData : public FDynamicSpriteEmitterDataBase
{
	FDynamicSubUVEmitterData(INT InParticleCount, INT InParticleStride, UBOOL InbRequiresSorting, const FMaterialRenderProxy* InMaterialResource, INT InMaxDrawCount = -1) :
		  FDynamicSpriteEmitterDataBase(InParticleCount, InParticleStride, InbRequiresSorting, InMaterialResource, InMaxDrawCount)
		, VertexCount(0)
		, VertexData(NULL)
		, IndexCount(0)
		, IndexStride(0)
		, IndexData(NULL)
		, VertexFactory(NULL)
	{
		eEmitterType = DET_SubUV;
		if (InParticleCount > 0)
		{
			ParticleData = (BYTE*)appRealloc(ParticleData, InParticleCount * InParticleStride);
			check(ParticleData);
			ParticleIndices = (WORD*)appRealloc(ParticleIndices, InParticleCount * sizeof(WORD));
			check(ParticleIndices);
		}
	}

	virtual ~FDynamicSubUVEmitterData()
	{
		appFree(VertexData);
		appFree(IndexData);
		if(VertexFactory)
		{
			VertexFactory->Release();
			delete VertexFactory;
		}
	}

	virtual UBOOL GetVertexAndIndexData(FParticleSpriteSubUVVertex* VertexData, void* FillIndexData, TArray<FParticleOrder>* ParticleOrder);

	// Render thread only draw call
	virtual void Render(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex);

	INT								SubUVDataOffset;
	INT								SubImages_Horizontal;
	INT								SubImages_Vertical;
	UBOOL							bDirectUV;

	/**	The sprite particle data.										*/
	INT								PrimitiveCount;
	INT								VertexCount;
	FParticleSpriteSubUVVertex*		VertexData;			// RENDER-THREAD USAGE ONLY!!!
	INT								IndexCount;
	INT								IndexStride;
	void*							IndexData;			// RENDER-THREAD USAGE ONLY!!!

	TArray<FParticleOrder>			ParticleOrder;

	FParticleSubUVVertexFactory*	VertexFactory;		// RENDER-THREAD USAGE ONLY!!!
};

/** */
class UStaticMesh;
class UMaterialInstanceConstant;

struct FDynamicMeshEmitterData : public FDynamicSpriteEmitterDataBase
{
	FDynamicMeshEmitterData(const FParticleMeshEmitterInstance* InEmitterInstance, 
		INT InParticleCount, INT InParticleStride, UBOOL InbRequiresSorting, const FMaterialRenderProxy* InMaterialResource, 
		INT InSubUVInterpMethod, INT InSubUVDataOffset, UStaticMesh* InStaticMesh, 
		const UStaticMeshComponent* InStaticMeshComponent, INT InMaxDrawCount = -1);

	virtual ~FDynamicMeshEmitterData();

	/**
	* @return TRUE if the particle system uses FMeshElement.UseDynamicData=TRUE
	*/
	virtual UBOOL HasDynamicMeshElementData()
	{
		// mesh emitters don't set any dynamic mesh element data
		return FALSE;
	}

	// Render thread only draw call
	virtual void Render(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex);

	/** Information used by the proxy about a single LOD of the mesh. */
	class FLODInfo
	{
	public:

		/** Information about an element of a LOD. */
		struct FElementInfo
		{
			UMaterialInterface* MaterialInterface;
		};
		TArray<FElementInfo> Elements;

		FLODInfo(const UStaticMeshComponent* Component, const FParticleMeshEmitterInstance* MeshEmitInst, 
			INT LODIndex, UBOOL bSelected);
	};

	/** */
	INT					SubUVInterpMethod;
	INT					SubUVDataOffset;
	INT					SubImages_Horizontal;
	INT					SubImages_Vertical;
	FVector				SubUVOffset;
	UBOOL				bScaleUV;
	INT					MeshRotationOffset;
	BYTE				MeshAlignment;
	FVector				LockedAxis;
	UBOOL				bMeshRotationActive;

	UStaticMesh*		StaticMesh;
	TArray<FLODInfo>	LODs;
};

/** */
struct FDynamicBeam2EmitterData : public FDynamicSpriteEmitterDataBase
{
	static const UINT MaxBeams = 2 * 1024;
	static const UINT MaxInterpolationPoints = 250;
	static const UINT MaxNoiseFrequency = 250;

	FDynamicBeam2EmitterData(INT InParticleCount, INT InParticleStride, UBOOL InbRequiresSorting, const FMaterialRenderProxy* InMaterialResource) :
		  FDynamicSpriteEmitterDataBase(InParticleCount, InParticleStride, InbRequiresSorting, InMaterialResource, -1)
		, PrimitiveCount(0)
		, VertexCount(0)
		, VertexData(NULL)
		, IndexCount(0)
		, IndexStride(0)
		, IndexData(NULL)
		, VertexFactory(NULL)
		, BeamDataOffset(-1)
		, InterpolatedPointsOffset(-1)
		, NoiseRateOffset(-1)
		, NoiseDeltaTimeOffset(-1)
		, TargetNoisePointsOffset(-1)
		, NextNoisePointsOffset(-1)
		, TaperValuesOffset(-1)
		, NoiseDistanceScaleOffset(-1)
		, bRenderGeometry(TRUE)
		, bRenderDirectLine(FALSE)
		, bRenderLines(FALSE)
		, bRenderTessellation(FALSE)
	{
		eEmitterType = DET_Beam2;
		if (InParticleCount > 0)
		{
			check(InParticleCount < (MaxBeams));	// TTP #33330 - Max of 2048 beams from a single emitter
			check(InParticleStride < 
				((MaxInterpolationPoints + 2) * (sizeof(FVector) + sizeof(FLOAT))) + 
				(MaxNoiseFrequency * (sizeof(FVector) + sizeof(FVector) + sizeof(FLOAT) + sizeof(FLOAT)))
				);	// TTP #33330 - Max of 10k per beam (includes interpolation points, noise, etc.)

			ParticleData = (BYTE*)appRealloc(ParticleData, InParticleCount * InParticleStride);
			check(ParticleData);
		}
	}

	virtual ~FDynamicBeam2EmitterData()
	{
		appFree(VertexData);
		appFree(IndexData);
		if(VertexFactory)
		{
			VertexFactory->Release();
			delete VertexFactory;
		}
	}

	// Render thread only draw call
	virtual void Render(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex);
	virtual void RenderDirectLine(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex);
	virtual void RenderLines(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex);

	virtual void RenderDebug(FPrimitiveDrawInterface* PDI, const FSceneView* View, UINT DPGIndex, UBOOL bCrosses);

	INT FillIndexData(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI, const FSceneView* View, UINT DPGIndex);
	INT FillVertexData_NoNoise(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI, const FSceneView* View, UINT DPGIndex);
	INT FillData_Noise(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI, const FSceneView* View, UINT DPGIndex);
	INT FillData_InterpolatedNoise(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI, const FSceneView* View, UINT DPGIndex);

	/**	The sprite particle data.										*/
	TArray<INT>							TrianglesPerSheet;
	INT									PrimitiveCount;
	INT									VertexCount;
	FParticleSpriteVertex*				VertexData;			// RENDER-THREAD USAGE ONLY!!!
	INT									IndexCount;
	INT									IndexStride;
	void*								IndexData;			// RENDER-THREAD USAGE ONLY!!!
	FParticleBeamTrailVertexFactory*	VertexFactory;		// RENDER-THREAD USAGE ONLY!!!

	// 
	UBOOL								bLowFreqNoise_Enabled;
	UBOOL								bHighFreqNoise_Enabled;
	UBOOL								bSmoothNoise_Enabled;
	UBOOL								bUseSource;
	UBOOL								bUseTarget;
	UBOOL								bTargetNoise;
	UINT								TessFactor;
	INT									Sheets;
	INT									Frequency;
	INT									NoiseTessellation;
	FLOAT								NoiseRangeScale;
	FLOAT								NoiseTangentStrength;
	FVector								NoiseSpeed;
	FLOAT								NoiseLockTime;
	FLOAT								NoiseLockRadius;
	FLOAT								NoiseTension;
	INT									TextureTile;
	FLOAT								TextureTileDistance;
	BYTE								TaperMethod;
	INT									TaperCount;
	INT									InterpolationPoints;

	// Offsets to particle data
	INT									BeamDataOffset;
	INT									InterpolatedPointsOffset;
	INT									NoiseRateOffset;
	INT									NoiseDeltaTimeOffset;
	INT									TargetNoisePointsOffset;
	INT									NextNoisePointsOffset;
	INT									TaperValuesOffset;
	INT									NoiseDistanceScaleOffset;

	/** Debugging rendering flags												*/
	UBOOL								bRenderGeometry;
	UBOOL								bRenderDirectLine;
	UBOOL								bRenderLines;
	UBOOL								bRenderTessellation;
};

struct FDynamicTrail2EmitterData : public FDynamicSpriteEmitterDataBase
{
	FDynamicTrail2EmitterData(INT InParticleCount, INT InParticleStride, UBOOL InbRequiresSorting, const FMaterialRenderProxy* InMaterialResource) :
		  FDynamicSpriteEmitterDataBase(InParticleCount, InParticleStride, InbRequiresSorting, InMaterialResource, -1)
		, PrimitiveCount(0)
		, VertexCount(0)
		, VertexData(NULL)
		, IndexCount(0)
		, IndexStride(0)
		, IndexData(NULL)
		, VertexFactory(NULL)
	{
		eEmitterType = DET_Trail2;
		if (InParticleCount > 0)
		{
			check(InParticleCount < (16 * 1024));	// TTP #33330
			check(InParticleStride < (2 * 1024));	// TTP #33330
			ParticleData = (BYTE*)appRealloc(ParticleData, InParticleCount * InParticleStride);
			check(ParticleData);
			ParticleIndices = (WORD*)appRealloc(ParticleIndices, InParticleCount * sizeof(WORD));
			check(ParticleIndices);
		}
	}

	virtual ~FDynamicTrail2EmitterData()
	{
		appFree(VertexData);
		appFree(IndexData);
		if(VertexFactory)
		{
			VertexFactory->Release();
			delete VertexFactory;
		}
	}

	// Render thread only draw call
	virtual void Render(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex);

	virtual void RenderDebug(FPrimitiveDrawInterface* PDI, const FSceneView* View, UINT DPGIndex, UBOOL bCrosses);

	INT FillIndexData(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI, const FSceneView* View, UINT DPGIndex);
	INT FillVertexData(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI, const FSceneView* View, UINT DPGIndex);

	/**	The sprite particle data.										*/
	INT									PrimitiveCount;
	INT									VertexCount;
	FParticleSpriteVertex*				VertexData;			// RENDER-THREAD USAGE ONLY!!!
	INT									IndexCount;
	INT									IndexStride;
	void*								IndexData;			// RENDER-THREAD USAGE ONLY!!!
	FParticleBeamTrailVertexFactory*	VertexFactory;		// RENDER-THREAD USAGE ONLY!!!

	// Payload offsets
	INT									TrailDataOffset;
	INT									TaperValuesOffset;
	INT									ParticleSourceOffset;

	//
	INT									TrailCount;
	INT									Sheets;
	INT									TessFactor;
	INT									TessStrength;
	FLOAT								TessFactorDistance;

	//
    TArray<FLOAT>						TrailSpawnTimes;
    TArray<FVector>						SourcePosition;
    TArray<FVector>						LastSourcePosition;
    TArray<FVector>						CurrentSourcePosition;
    TArray<FVector>						LastSpawnPosition;
    TArray<FVector>						LastSpawnTangent;
    TArray<FLOAT>						SourceDistanceTravelled;
    TArray<FVector>						SourceOffsets;
};

/*-----------------------------------------------------------------------------
 *	Particle dynamic data
 *	This is a copy of the particle system data needed to render the system in
 *	another thread.
 ----------------------------------------------------------------------------*/
class FParticleDynamicData
{
public:
	FParticleDynamicData(UParticleSystemComponent* PartSysComp);

	virtual ~FParticleDynamicData()
	{
		for (INT Index = 0; Index < DynamicEmitterDataArray.Num(); Index++)
		{
			FDynamicEmitterDataBase* Data =	DynamicEmitterDataArray(Index);
			delete Data;
			DynamicEmitterDataArray(Index) = NULL;
		}
		DynamicEmitterDataArray.Empty();
	}

	DWORD GetMemoryFootprint( void ) const { return( sizeof( *this ) + DynamicEmitterDataArray.GetAllocatedSize() ); }

	// Variables
	TArray<FDynamicEmitterDataBase*>	DynamicEmitterDataArray;
};

//
//	Scene Proxies
//

class FParticleSystemSceneProxy : public FPrimitiveSceneProxy
{
public:
	/** Initialization constructor. */
	FParticleSystemSceneProxy(const UParticleSystemComponent* Component);
	virtual ~FParticleSystemSceneProxy();

	// FPrimitiveSceneProxy interface.
	
	/** 
	* Draw the scene proxy as a dynamic element
	*
	* @param	PDI - draw interface to render to
	* @param	View - current view
	* @param	DPGIndex - current depth priority 
	* @param	Flags - optional set of flags from EDrawDynamicElementFlags
	*/
	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View,UINT DPGIndex,DWORD Flags);

	virtual void DrawShadowVolumes(FShadowVolumeDrawInterface* SVDI,const FSceneView* View,const class FLightSceneInfo* Light);	
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View);

	/**
	 *	Called when the rendering thread adds the proxy to the scene.
	 *	This function allows for generating renderer-side resources.
	 */
	virtual UBOOL CreateRenderThreadResources();

	/**
	 *	Called when the rendering thread removes the dynamic data from the scene.
	 */
	virtual UBOOL ReleaseRenderThreadResources();

	void UpdateData(FParticleDynamicData* NewDynamicData);
	void UpdateData_RenderThread(FParticleDynamicData* NewDynamicData);

	void DummyFunction()
	{
		// This is for debugging purposes!
		INT Dummy = 0;
	}

	FParticleDynamicData* GetDynamicData()
	{
		return DynamicData;
	}

	FParticleDynamicData* GetLastDynamicData()
	{
		return LastDynamicData;
	}

	void SetLastDynamicData(FParticleDynamicData* InLastDynamicData)
	{
		LastDynamicData  = InLastDynamicData;
	}


	virtual EMemoryStats GetMemoryStatType( void ) const { return( STAT_GameToRendererMallocPSSP ); }
	virtual DWORD GetMemoryFootprint( void ) const { return( sizeof( *this ) + GetAllocatedSize() ); }
	DWORD GetAllocatedSize( void ) const 
	{ 
		DWORD AdditionalSize = FPrimitiveSceneProxy::GetAllocatedSize();
		if( LastDynamicData )
		{
			AdditionalSize = LastDynamicData->GetMemoryFootprint();
		}

		return( AdditionalSize ); 
	}

	void DetermineLODDistance(const FSceneView* View);


public:
	// While this isn't good OO design, access to everything is made public.
	// This is to allow custom emitter instances to easily be written when extending the engine.

	FPrimitiveSceneInfo* GetPrimitiveSceneInfo() const	{	return PrimitiveSceneInfo;	}

	FMatrix& GetLocalToWorld()			{	return LocalToWorld;			}
	FMatrix GetWorldToLocal()			{	return LocalToWorld.Inverse();	}
	FLOAT GetLocalToWorldDeterminant()	{	return LocalToWorldDeterminant;	}
	AActor* GetOwner()					{	return Owner;					}
	UBOOL GetSelected()					{	return bSelected;				}
	FLOAT GetCullDistance()				{	return CullDistance;			}
	UBOOL GetCastShadow()				{	return bCastShadow;				}
	const FMaterialViewRelevance& GetMaterialViewRelevance() const
	{
		return MaterialViewRelevance;
	}
	FLOAT GetPendingLODDistance()		{	return PendingLODDistance;		}
	INT GetLODMethod()					{	return LODMethod;				}

	FColoredMaterialRenderProxy* GetSelectedWireframeMatInst()		{	return &SelectedWireframeMaterialInstance;		}
	FColoredMaterialRenderProxy* GetDeselectedWireframeMatInst()	{	return &DeselectedWireframeMaterialInstance;	}

	void GetAxisLockValues(FDynamicSpriteEmitterDataBase* DynamicData, FVector& CameraUp, FVector& CameraRight);

protected:
	AActor* Owner;

	UBOOL bSelected;

	FLOAT CullDistance;

	BITFIELD bCastShadow : 1;
	
	FMaterialViewRelevance MaterialViewRelevance;

	FParticleDynamicData* DynamicData;
	FParticleDynamicData* LastDynamicData;

	FColoredMaterialRenderProxy SelectedWireframeMaterialInstance;
	FColoredMaterialRenderProxy DeselectedWireframeMaterialInstance;

	INT LODMethod;
	FLOAT PendingLODDistance;

	friend struct FDynamicSpriteEmitterDataBase;
};

/*-----------------------------------------------------------------------------
 *	ParticleDataManager
 *	Handles the collection of ParticleSystemComponents that are to be 
 *	submitted to the rendering thread for display.
 ----------------------------------------------------------------------------*/
struct FParticleDataManager
{
protected:
	/** The particle system components that need to be sent to the rendering thread */
	TDynamicMap<UParticleSystemComponent*, UBOOL>	PSysComponents;

public:
	/**
	 *	Update the dynamic data for all particle system componets
	 */
	virtual void UpdateDynamicData();
	
	/**
	 *	Add a particle system component to the list.
	 *
	 *	@param		InPSysComp		The particle system component to add.
	 */
	void AddParticleSystemComponent(UParticleSystemComponent* InPSysComp);

	/**
	 *	Remove a particle system component to the list.
	 *
	 *	@param		InPSysComp		The particle system component to remove.
	 */
	void RemoveParticleSystemComponent(UParticleSystemComponent* InPSysComp);

	/**
	 *	Clear all pending components from the queue.
	 */
	void Clear();
};

#endif
