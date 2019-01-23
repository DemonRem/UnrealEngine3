/*===========================================================================
    C++ class definitions exported from UnrealScript.
    This is automatically generated by the tools.
    DO NOT modify this manually! Edit the corresponding .uc files instead!
    Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
===========================================================================*/
#if SUPPORTS_PRAGMA_PACK
#pragma pack (push,4)
#endif


// Split enums from the rest of the header so they can be included earlier
// than the rest of the header file by including this file twice with different
// #define wrappers. See Engine.h and look at EngineClasses.h for an example.
#if !NO_ENUMS && !defined(NAMES_ONLY)

#ifndef INCLUDED_ENGINE_SOUND_ENUMS
#define INCLUDED_ENGINE_SOUND_ENUMS 1

enum SoundDistanceModel
{
    ATTENUATION_Linear      =0,
    ATTENUATION_Logarithmic =1,
    ATTENUATION_Inverse     =2,
    ATTENUATION_LogReverse  =3,
    ATTENUATION_NaturalSound=4,
    ATTENUATION_MAX         =5,
};

#endif // !INCLUDED_ENGINE_SOUND_ENUMS
#endif // !NO_ENUMS

#if !ENUMS_ONLY

#ifndef NAMES_ONLY
#define AUTOGENERATE_NAME(name) extern FName ENGINE_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name)
#endif


#ifndef NAMES_ONLY

#ifndef INCLUDED_ENGINE_SOUND_CLASSES
#define INCLUDED_ENGINE_SOUND_CLASSES 1

class AAmbientSound : public AKeypoint
{
public:
    //## BEGIN PROPS AmbientSound
    BITFIELD bAutoPlay:1;
    BITFIELD bIsPlaying:1;
    class UAudioComponent* AudioComponent;
    //## END PROPS AmbientSound

    DECLARE_CLASS(AAmbientSound,AKeypoint,0,Engine)
public:
	// AActor interface.
	/**
	 * Function that gets called from within Map_Check to allow this actor to check itself
	 * for any potential errors and register them with map check dialog.
	 */
	virtual void CheckForErrors();

protected:
	/**
	 * Starts audio playback if wanted.
	 */
	virtual void UpdateComponentsInternal(UBOOL bCollisionUpdate = FALSE);
public:
};

class AAmbientSoundSimple : public AAmbientSound
{
public:
    //## BEGIN PROPS AmbientSoundSimple
    class USoundNodeAmbient* AmbientProperties;
    class USoundCue* SoundCueInstance;
    class USoundNodeAmbient* SoundNodeInstance;
    //## END PROPS AmbientSoundSimple

    DECLARE_CLASS(AAmbientSoundSimple,AAmbientSound,0,Engine)
	/**
	 * Helper function used to sync up instantiated objects.
	 */
	void SyncUpInstantiatedObjects();

	/**
	 * Called from within SpawnActor, calling SyncUpInstantiatedObjects.
	 */
	virtual void Spawned();

	/**
	 * Called when after .t3d import of this actor (paste, duplicate or .t3d import),
	 * calling SyncUpInstantiatedObjects.
	 */
	virtual void PostEditImport();

	/**
	 * Used to temporarily clear references for duplication.
	 *
	 * @param PropertyThatWillChange	property that will change
	 */
	virtual void PreEditChange(UProperty* PropertyThatWillChange);

	/**
	 * Used to reset audio component when AmbientProperties change
	 *
	 * @param PropertyThatChanged	property that changed
	 */
	virtual void PostEditChange(UProperty* PropertyThatChanged);

	virtual void EditorApplyScale(const FVector& DeltaScale, const FMatrix& ScaleMatrix, const FVector* PivotLocation, UBOOL bAltDown, UBOOL bShiftDown, UBOOL bCtrlDown);
	/**
	 * Function that gets called from within Map_Check to allow this actor to check itself
	 * for any potential errors and register them with map check dialog.
	 */
	virtual void CheckForErrors();
};

class AAmbientSoundNonLoop : public AAmbientSoundSimple
{
public:
    //## BEGIN PROPS AmbientSoundNonLoop
    //## END PROPS AmbientSoundNonLoop

    DECLARE_CLASS(AAmbientSoundNonLoop,AAmbientSoundSimple,0,Engine)
public:
	// AActor interface.
	/**
	 * Function that gets called from within Map_Check to allow this actor to check itself
	 * for any potential errors and register them with map check dialog.
	 */
	virtual void CheckForErrors();
};

class UDistributionFloatSoundParameter : public UDistributionFloatParameterBase
{
public:
    //## BEGIN PROPS DistributionFloatSoundParameter
    //## END PROPS DistributionFloatSoundParameter

    DECLARE_CLASS(UDistributionFloatSoundParameter,UDistributionFloatParameterBase,0,Engine)
	virtual UBOOL GetParamValue(UObject* Data, FName ParamName, FLOAT& OutFloat);
};

class USoundNodeAttenuation : public USoundNode
{
public:
    //## BEGIN PROPS SoundNodeAttenuation
    BYTE DistanceModel;
    struct FRawDistributionFloat MinRadius;
    struct FRawDistributionFloat MaxRadius;
    FLOAT dBAttenuationAtMax;
    struct FRawDistributionFloat LPFMinRadius;
    struct FRawDistributionFloat LPFMaxRadius;
    BITFIELD bSpatialize:1;
    BITFIELD bAttenuate:1;
    BITFIELD bAttenuateWithLowPassFilter:1;
    //## END PROPS SoundNodeAttenuation

    DECLARE_CLASS(USoundNodeAttenuation,USoundNode,0,Engine)
	// USoundNode interface.

	virtual void ParseNodes( UAudioDevice* AudioDevice, USoundNode* Parent, INT ChildIndex, class UAudioComponent* AudioComponent, TArray<FWaveInstance*>& WaveInstances );
	virtual FLOAT MaxAudibleDistance(FLOAT CurrentMaxDistance) { return ::Max<FLOAT>(CurrentMaxDistance,MaxRadius.GetValue(0.9f)); }
};

class USoundNodeAmbient : public USoundNode
{
public:
    //## BEGIN PROPS SoundNodeAmbient
    BYTE DistanceModel;
    struct FRawDistributionFloat MinRadius;
    struct FRawDistributionFloat MaxRadius;
    struct FRawDistributionFloat LPFMinRadius;
    struct FRawDistributionFloat LPFMaxRadius;
    BITFIELD bSpatialize:1;
    BITFIELD bAttenuate:1;
    BITFIELD bAttenuateWithLowPassFilter:1;
    class USoundNodeWave* Wave;
    struct FRawDistributionFloat VolumeModulation;
    struct FRawDistributionFloat PitchModulation;
    //## END PROPS SoundNodeAmbient

    DECLARE_CLASS(USoundNodeAmbient,USoundNode,0,Engine)
	// USoundNode interface.
	virtual void ParseNodes( UAudioDevice* AudioDevice, USoundNode* Parent, INT ChildIndex, class UAudioComponent* AudioComponent, TArray<FWaveInstance*>& WaveInstances );
	virtual void GetNodes( class UAudioComponent* AudioComponent, TArray<USoundNode*>& SoundNodes );
	virtual void GetAllNodes( TArray<USoundNode*>& SoundNodes ); // Like above but returns ALL (not just active) nodes.
	virtual FLOAT MaxAudibleDistance(FLOAT CurrentMaxDistance) { return ::Max<FLOAT>(CurrentMaxDistance,MaxRadius.GetValue(0.9f)); }
	virtual INT GetMaxChildNodes() { return 0; }
		
	/**
	 * Notifies the sound node that a wave instance in its subtree has finished.
	 *
	 * @param WaveInstance	WaveInstance that was finished 
	 */
	virtual void NotifyWaveInstanceFinished( struct FWaveInstance* WaveInstance );

	/**
	 * We're looping indefinitely so we're never finished.
	 *
	 * @param	AudioComponent	Audio component containing payload data
	 * @return	FALSE
	 */
	virtual UBOOL IsFinished( class UAudioComponent* /*Unused*/ ) { return FALSE; }
};

struct FAmbientSoundSlot
{
    class USoundNodeWave* Wave;
    FLOAT PitchScale;
    FLOAT VolumeScale;
    FLOAT Weight;
};

class USoundNodeAmbientNonLoop : public USoundNodeAmbient
{
public:
    //## BEGIN PROPS SoundNodeAmbientNonLoop
    struct FRawDistributionFloat DelayTime;
    TArrayNoInit<struct FAmbientSoundSlot> SoundSlots;
    //## END PROPS SoundNodeAmbientNonLoop

    DECLARE_CLASS(USoundNodeAmbientNonLoop,USoundNodeAmbient,0,Engine)
	// USoundNode interface.
	virtual void ParseNodes( UAudioDevice* AudioDevice, USoundNode* Parent, INT ChildIndex, class UAudioComponent* AudioComponent, TArray<FWaveInstance*>& WaveInstances );
	virtual void GetNodes( class UAudioComponent* AudioComponent, TArray<USoundNode*>& SoundNodes );
	virtual void GetAllNodes( TArray<USoundNode*>& SoundNodes ); // Like above but returns ALL (not just active) nodes.

	/**
	 * Notifies the sound node that a wave instance in its subtree has finished.
	 *
	 * @param WaveInstance	WaveInstance that was finished
	 */
	virtual void NotifyWaveInstanceFinished( struct FWaveInstance* WaveInstance );

	/** Pick which slot to play next. */
	INT PickNextSlot();
};

class USoundNodeConcatenator : public USoundNode
{
public:
    //## BEGIN PROPS SoundNodeConcatenator
    TArrayNoInit<FLOAT> InputVolume;
    //## END PROPS SoundNodeConcatenator

    DECLARE_CLASS(USoundNodeConcatenator,USoundNode,0,Engine)
	/**
	 * Ensures the concatenator has a volume for each input.
	 */
	virtual void PostLoad();

	// USoundNode interface.
	virtual void GetNodes( class UAudioComponent* AudioComponent, TArray<USoundNode*>& SoundNodes );
	virtual void ParseNodes( UAudioDevice* AudioDevice, USoundNode* Parent, INT ChildIndex, class UAudioComponent* AudioComponent, TArray<FWaveInstance*>& WaveInstances );

	virtual INT GetMaxChildNodes() { return -1; }	

	/**
	 * Notifies the sound node that a wave instance in its subtree has finished.
	 *
	 * @param WaveInstance	WaveInstance that was finished 
	 */
	virtual void NotifyWaveInstanceFinished( struct FWaveInstance* WaveInstance );

	/**
	 * Returns whether the node is finished after having been notified of buffer
	 * being finished.
	 *
	 * @param	AudioComponent	Audio component containing payload data
	 * @return	TRUE if finished, FALSE otherwise.
	 */
	UBOOL IsFinished( class UAudioComponent* AudioComponent );

	/**
	 * Returns the maximum duration this sound node will play for. 
	 * 
	 * @return maximum duration this sound will play for
	 */
	virtual FLOAT GetDuration();

	/**
	 * Concatenators have two connectors by default.
	 */
	virtual void CreateStartingConnectors();

	/**
	 * Overloaded to add an entry to InputVolume.
	 */
	virtual void InsertChildNode( INT Index );

	/**
	 * Overloaded to remove an entry from InputVolume.
	 */
	virtual void RemoveChildNode( INT Index );
};

class USoundNodeDelay : public USoundNode
{
public:
    //## BEGIN PROPS SoundNodeDelay
    struct FRawDistributionFloat DelayDuration;
    //## END PROPS SoundNodeDelay

    DECLARE_CLASS(USoundNodeDelay,USoundNode,0,Engine)
	// USoundNode interface.
	virtual void ParseNodes( UAudioDevice* AudioDevice, USoundNode* Parent, INT ChildIndex, class UAudioComponent* AudioComponent, TArray<FWaveInstance*>& WaveInstances );

	/**
	 * Returns the maximum duration this sound node will play for. 
	 * 
	 * @return maximum duration this sound will play for
	 */
	virtual FLOAT GetDuration();
};

struct FDistanceDatum
{
    struct FRawDistributionFloat FadeInDistance;
    struct FRawDistributionFloat FadeOutDistance;
    FLOAT Volume;

    /** Constructors */
    FDistanceDatum() {}
    FDistanceDatum(EEventParm)
    {
        appMemzero(this, sizeof(FDistanceDatum));
    }
};

class USoundNodeDistanceCrossFade : public USoundNode
{
public:
    //## BEGIN PROPS SoundNodeDistanceCrossFade
    TArrayNoInit<struct FDistanceDatum> CrossFadeInput;
    //## END PROPS SoundNodeDistanceCrossFade

    DECLARE_CLASS(USoundNodeDistanceCrossFade,USoundNode,0,Engine)

	/**
	 * Ensures the Node has the correct Distributions
	 */
	virtual void PostLoad();

	// USoundNode interface.

	virtual void ParseNodes( UAudioDevice* AudioDevice, USoundNode* Parent, INT ChildIndex, class UAudioComponent* AudioComponent, TArray<FWaveInstance*>& WaveInstances );
	virtual INT GetMaxChildNodes() { return -1; }

	/**
	 * DistanceCrossFades have two connectors by default.
	 */
	virtual void CreateStartingConnectors();
	virtual void InsertChildNode( INT Index );
	virtual void RemoveChildNode( INT Index );

	virtual FLOAT MaxAudibleDistance( FLOAT CurrentMaxDistance );
};

class USoundNodeLooping : public USoundNode
{
public:
    //## BEGIN PROPS SoundNodeLooping
    BITFIELD bLoopIndefinitely:1;
    struct FRawDistributionFloat LoopCount;
    //## END PROPS SoundNodeLooping

    DECLARE_CLASS(USoundNodeLooping,USoundNode,0,Engine)
	/** 
	 * Added to make a guess at the loop indefinitely flag
	 */
	virtual void Serialize( FArchive& Ar );

	/**
	 * Notifies the sound node that a wave instance in its subtree has finished.
	 *
	 * @param WaveInstance	WaveInstance that was finished 
	 */
	virtual void NotifyWaveInstanceFinished( struct FWaveInstance* WaveInstance );

	/**
	 * Returns whether the node is finished after having been notified of buffer
	 * being finished.
	 *
	 * @param	AudioComponent	Audio component containing payload data
	 * @return	TRUE if finished, FALSE otherwise.
	 */
	virtual UBOOL IsFinished( class UAudioComponent* AudioComponent );

	/** 
	 * Returns the maximum distance this sound can be heard from. Very large for looping sounds as the
	 * player can move into the hearable range during a loop.
	 */
	virtual FLOAT MaxAudibleDistance( FLOAT CurrentMaxDistance ) { return WORLD_MAX; }
	
	/** 
	 * Returns the duration of the sound accounting for the number of loops.
	 */
	virtual FLOAT GetDuration();

	/** 
	 * Returns whether the sound is looping indefinitely or not.
	 */
	virtual UBOOL IsLoopingIndefinitely( void );

	/** 
	 * Process this node in the sound cue
	 */
	virtual void ParseNodes( UAudioDevice* AudioDevice, USoundNode* Parent, INT ChildIndex, class UAudioComponent* AudioComponent, TArray<FWaveInstance*>& WaveInstances );
};

class USoundNodeMature : public USoundNode
{
public:
    //## BEGIN PROPS SoundNodeMature
    TArrayNoInit<UBOOL> IsMature;
    //## END PROPS SoundNodeMature

    DECLARE_CLASS(USoundNodeMature,USoundNode,0,Engine)
	// USoundNode interface.
	virtual void GetNodes( class UAudioComponent* AudioComponent, TArray<USoundNode*>& SoundNodes );
	virtual void ParseNodes( UAudioDevice* AudioDevice, USoundNode* Parent, INT ChildIndex, class UAudioComponent* AudioComponent, TArray<FWaveInstance*>& WaveInstances );

	virtual INT GetMaxChildNodes();
};

class USoundNodeMixer : public USoundNode
{
public:
    //## BEGIN PROPS SoundNodeMixer
    TArrayNoInit<FLOAT> InputVolume;
    //## END PROPS SoundNodeMixer

    DECLARE_CLASS(USoundNodeMixer,USoundNode,0,Engine)
	/**
	 * Ensures the mixer has a volume for each input.
	 */
	virtual void PostLoad();

	// USoundNode interface.

	virtual void ParseNodes( UAudioDevice* AudioDevice, USoundNode* Parent, INT ChildIndex, class UAudioComponent* AudioComponent, TArray<FWaveInstance*>& WaveInstances );
	virtual INT GetMaxChildNodes() { return -1; }

	/**
	 * Mixers have two connectors by default.
	 */
	virtual void CreateStartingConnectors();

	/**
	 * Overloaded to add an entry to InputVolume.
	 */
	virtual void InsertChildNode( INT Index );

	/**
	 * Overloaded to remove an entry from InputVolume.
	 */
	virtual void RemoveChildNode( INT Index );
};

class USoundNodeModulator : public USoundNode
{
public:
    //## BEGIN PROPS SoundNodeModulator
    struct FRawDistributionFloat VolumeModulation;
    struct FRawDistributionFloat PitchModulation;
    //## END PROPS SoundNodeModulator

    DECLARE_CLASS(USoundNodeModulator,USoundNode,0,Engine)
	// USoundNode interface.

	virtual void ParseNodes( UAudioDevice* AudioDevice, USoundNode* Parent, INT ChildIndex, class UAudioComponent* AudioComponent, TArray<FWaveInstance*>& WaveInstances );
};

class USoundNodeModulatorContinuous : public USoundNode
{
public:
    //## BEGIN PROPS SoundNodeModulatorContinuous
    struct FRawDistributionFloat VolumeModulation;
    struct FRawDistributionFloat PitchModulation;
    //## END PROPS SoundNodeModulatorContinuous

    DECLARE_CLASS(USoundNodeModulatorContinuous,USoundNode,0,Engine)
	// USoundNode interface.

	virtual void ParseNodes( UAudioDevice* AudioDevice, USoundNode* Parent, INT ChildIndex, class UAudioComponent* AudioComponent, TArray<FWaveInstance*>& WaveInstances );
};

class USoundNodeOscillator : public USoundNode
{
public:
    //## BEGIN PROPS SoundNodeOscillator
    struct FRawDistributionFloat Amplitude;
    struct FRawDistributionFloat Frequency;
    struct FRawDistributionFloat Offset;
    struct FRawDistributionFloat Center;
    BITFIELD bModulatePitch:1;
    BITFIELD bModulateVolume:1;
    //## END PROPS SoundNodeOscillator

    DECLARE_CLASS(USoundNodeOscillator,USoundNode,0,Engine)
	// USoundNode interface.

	virtual void ParseNodes( UAudioDevice* AudioDevice, USoundNode* Parent, INT ChildIndex, class UAudioComponent* AudioComponent, TArray<FWaveInstance*>& WaveInstances );
};

class USoundNodeRandom : public USoundNode
{
public:
    //## BEGIN PROPS SoundNodeRandom
    TArrayNoInit<FLOAT> Weights;
    BITFIELD bRandomizeWithoutReplacement:1;
    TArrayNoInit<UBOOL> HasBeenUsed;
    INT NumRandomUsed;
    //## END PROPS SoundNodeRandom

    DECLARE_CLASS(USoundNodeRandom,USoundNode,0,Engine)
	// USoundNode interface.
	
	virtual void GetNodes( class UAudioComponent* AudioComponent, TArray<USoundNode*>& SoundNodes );
	virtual void ParseNodes( UAudioDevice* AudioDevice, USoundNode* Parent, INT ChildIndex, class UAudioComponent* AudioComponent, TArray<FWaveInstance*>& WaveInstances );

	virtual INT GetMaxChildNodes() { return -1; }
	
	// Editor interface.
	
	virtual void InsertChildNode( INT Index );
	virtual void RemoveChildNode( INT Index );
	
	// USoundNodeRandom interface
	void FixWeightsArray();
	void FixHasBeenUsedArray();


	/**
	 * Called by the Sound Cue Editor for nodes which allow children.  The default behaviour is to
	 * attach a single connector. Dervied classes can override to e.g. add multiple connectors.
	 */
	virtual void CreateStartingConnectors();
};

#endif // !INCLUDED_ENGINE_SOUND_CLASSES
#endif // !NAMES_ONLY


#ifndef NAMES_ONLY
#undef AUTOGENERATE_NAME
#undef AUTOGENERATE_FUNCTION
#endif

#ifdef STATIC_LINKING_MOJO
#ifndef ENGINE_SOUND_NATIVE_DEFS
#define ENGINE_SOUND_NATIVE_DEFS

DECLARE_NATIVE_TYPE(Engine,AAmbientSound);
DECLARE_NATIVE_TYPE(Engine,AAmbientSoundNonLoop);
DECLARE_NATIVE_TYPE(Engine,AAmbientSoundSimple);
DECLARE_NATIVE_TYPE(Engine,UDistributionFloatSoundParameter);
DECLARE_NATIVE_TYPE(Engine,USoundNodeAmbient);
DECLARE_NATIVE_TYPE(Engine,USoundNodeAmbientNonLoop);
DECLARE_NATIVE_TYPE(Engine,USoundNodeAttenuation);
DECLARE_NATIVE_TYPE(Engine,USoundNodeConcatenator);
DECLARE_NATIVE_TYPE(Engine,USoundNodeDelay);
DECLARE_NATIVE_TYPE(Engine,USoundNodeDistanceCrossFade);
DECLARE_NATIVE_TYPE(Engine,USoundNodeLooping);
DECLARE_NATIVE_TYPE(Engine,USoundNodeMature);
DECLARE_NATIVE_TYPE(Engine,USoundNodeMixer);
DECLARE_NATIVE_TYPE(Engine,USoundNodeModulator);
DECLARE_NATIVE_TYPE(Engine,USoundNodeModulatorContinuous);
DECLARE_NATIVE_TYPE(Engine,USoundNodeOscillator);
DECLARE_NATIVE_TYPE(Engine,USoundNodeRandom);

#define AUTO_INITIALIZE_REGISTRANTS_ENGINE_SOUND \
	AAmbientSound::StaticClass(); \
	AAmbientSoundNonLoop::StaticClass(); \
	AAmbientSoundSimple::StaticClass(); \
	UDistributionFloatSoundParameter::StaticClass(); \
	USoundNodeAmbient::StaticClass(); \
	USoundNodeAmbientNonLoop::StaticClass(); \
	USoundNodeAttenuation::StaticClass(); \
	USoundNodeConcatenator::StaticClass(); \
	USoundNodeDelay::StaticClass(); \
	USoundNodeDistanceCrossFade::StaticClass(); \
	USoundNodeLooping::StaticClass(); \
	USoundNodeMature::StaticClass(); \
	USoundNodeMixer::StaticClass(); \
	USoundNodeModulator::StaticClass(); \
	USoundNodeModulatorContinuous::StaticClass(); \
	USoundNodeOscillator::StaticClass(); \
	USoundNodeRandom::StaticClass(); \

#endif // ENGINE_SOUND_NATIVE_DEFS

#ifdef NATIVES_ONLY
#endif // NATIVES_ONLY
#endif // STATIC_LINKING_MOJO

#ifdef VERIFY_CLASS_SIZES
VERIFY_CLASS_OFFSET_NODIE(A,AmbientSound,AudioComponent)
VERIFY_CLASS_SIZE_NODIE(AAmbientSound)
VERIFY_CLASS_SIZE_NODIE(AAmbientSoundNonLoop)
VERIFY_CLASS_OFFSET_NODIE(A,AmbientSoundSimple,AmbientProperties)
VERIFY_CLASS_OFFSET_NODIE(A,AmbientSoundSimple,SoundNodeInstance)
VERIFY_CLASS_SIZE_NODIE(AAmbientSoundSimple)
VERIFY_CLASS_SIZE_NODIE(UDistributionFloatSoundParameter)
VERIFY_CLASS_OFFSET_NODIE(U,SoundNodeAmbient,DistanceModel)
VERIFY_CLASS_OFFSET_NODIE(U,SoundNodeAmbient,PitchModulation)
VERIFY_CLASS_SIZE_NODIE(USoundNodeAmbient)
VERIFY_CLASS_OFFSET_NODIE(U,SoundNodeAmbientNonLoop,DelayTime)
VERIFY_CLASS_OFFSET_NODIE(U,SoundNodeAmbientNonLoop,SoundSlots)
VERIFY_CLASS_SIZE_NODIE(USoundNodeAmbientNonLoop)
VERIFY_CLASS_OFFSET_NODIE(U,SoundNodeAttenuation,DistanceModel)
VERIFY_CLASS_OFFSET_NODIE(U,SoundNodeAttenuation,LPFMaxRadius)
VERIFY_CLASS_SIZE_NODIE(USoundNodeAttenuation)
VERIFY_CLASS_OFFSET_NODIE(U,SoundNodeConcatenator,InputVolume)
VERIFY_CLASS_SIZE_NODIE(USoundNodeConcatenator)
VERIFY_CLASS_OFFSET_NODIE(U,SoundNodeDelay,DelayDuration)
VERIFY_CLASS_SIZE_NODIE(USoundNodeDelay)
VERIFY_CLASS_OFFSET_NODIE(U,SoundNodeDistanceCrossFade,CrossFadeInput)
VERIFY_CLASS_SIZE_NODIE(USoundNodeDistanceCrossFade)
VERIFY_CLASS_OFFSET_NODIE(U,SoundNodeLooping,LoopCount)
VERIFY_CLASS_SIZE_NODIE(USoundNodeLooping)
VERIFY_CLASS_OFFSET_NODIE(U,SoundNodeMature,IsMature)
VERIFY_CLASS_SIZE_NODIE(USoundNodeMature)
VERIFY_CLASS_OFFSET_NODIE(U,SoundNodeMixer,InputVolume)
VERIFY_CLASS_SIZE_NODIE(USoundNodeMixer)
VERIFY_CLASS_OFFSET_NODIE(U,SoundNodeModulator,VolumeModulation)
VERIFY_CLASS_OFFSET_NODIE(U,SoundNodeModulator,PitchModulation)
VERIFY_CLASS_SIZE_NODIE(USoundNodeModulator)
VERIFY_CLASS_OFFSET_NODIE(U,SoundNodeModulatorContinuous,VolumeModulation)
VERIFY_CLASS_OFFSET_NODIE(U,SoundNodeModulatorContinuous,PitchModulation)
VERIFY_CLASS_SIZE_NODIE(USoundNodeModulatorContinuous)
VERIFY_CLASS_OFFSET_NODIE(U,SoundNodeOscillator,Amplitude)
VERIFY_CLASS_OFFSET_NODIE(U,SoundNodeOscillator,Center)
VERIFY_CLASS_SIZE_NODIE(USoundNodeOscillator)
VERIFY_CLASS_OFFSET_NODIE(U,SoundNodeRandom,Weights)
VERIFY_CLASS_OFFSET_NODIE(U,SoundNodeRandom,NumRandomUsed)
VERIFY_CLASS_SIZE_NODIE(USoundNodeRandom)
#endif // VERIFY_CLASS_SIZES
#endif // !ENUMS_ONLY

#if SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif
