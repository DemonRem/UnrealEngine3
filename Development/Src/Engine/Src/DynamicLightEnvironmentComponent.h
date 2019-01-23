/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * The character lighting stats.
 */
enum ECharacterLightingStats
{
	STAT_LightVisibilityTime = STAT_CharacterLightingFirstStat,
	STAT_UpdateEnvironmentTime,
	STAT_CreateLightsTime,
	STAT_NumEnvironments,
	STAT_EnvironmentUpdates,
	STAT_EnvironmentFullUpdates,
	STAT_StaticEnvironmentFullUpdates,
	STAT_StaticEnvironmentFullUpdatesSkipped,
};

DECLARE_STATS_GROUP(TEXT("CharacterLighting"),STATGROUP_CharacterLighting);
DECLARE_CYCLE_STAT(TEXT("Light visibility time"),STAT_LightVisibilityTime,STATGROUP_CharacterLighting);
DECLARE_CYCLE_STAT(TEXT("UpdateEnvironment time"),STAT_UpdateEnvironmentTime,STATGROUP_CharacterLighting);
DECLARE_CYCLE_STAT(TEXT("CreateLights time"),STAT_CreateLightsTime,STATGROUP_CharacterLighting);
DECLARE_DWORD_COUNTER_STAT(TEXT("Light Environments"),STAT_NumEnvironments,STATGROUP_CharacterLighting);
DECLARE_DWORD_COUNTER_STAT(TEXT("Environment Updates"),STAT_EnvironmentUpdates,STATGROUP_CharacterLighting);
DECLARE_DWORD_COUNTER_STAT(TEXT("Environment Full Updates"),STAT_EnvironmentFullUpdates,STATGROUP_CharacterLighting);
DECLARE_DWORD_COUNTER_STAT(TEXT("Static Env Full Updates"),STAT_StaticEnvironmentFullUpdates,STATGROUP_CharacterLighting);
DECLARE_DWORD_COUNTER_STAT(TEXT("Static Env Full Updates Skipped"),STAT_StaticEnvironmentFullUpdatesSkipped,STATGROUP_CharacterLighting);

/** The private light environment state. */
class FDynamicLightEnvironmentState
{
public:

	/** Initialization constructor. */
	FDynamicLightEnvironmentState(UDynamicLightEnvironmentComponent* InComponent);

	/** Updates the light environment. */
	void UpdateEnvironment(FLOAT DeltaTime,UBOOL bPerformFullUpdate,UBOOL bForceStaticLightUpdate);

	/** Updates the light environment's state. */
	void Tick(FLOAT DeltaTime);

	/** Creates a light to represent a light environment. */
	void CreateRepresentativeLight(FSHVectorRGB& RemainingLightEnvironment,UBOOL bCastLight,UBOOL bCastShadows) const;

	/** Detaches the light environment's representative lights. */
	void DetachRepresentativeLights() const;

	/** Creates the lights to represent the character's light environment. */
	void CreateEnvironmentLightList() const;

	/** Builds a list of objects referenced by the state. */
	void AddReferencedObjects(TArray<UObject*>& ObjectArray);

private:

	/** The component which this is state for. */
	UDynamicLightEnvironmentComponent* Component;

	/** The bounds of the owner. */
	FBoxSphereBounds OwnerBounds;

	/** The predicted center of the owner at the time of the next update. */
	FVector PredictedOwnerPosition;

	/** The lighting channels the owner's primitives are affected by. */
	FLightingChannelContainer OwnerLightingChannels;

	/** The owner's level. */
	UPackage* OwnerPackage;

	/** The time the light environment was last updated. */
	FLOAT LastUpdateTime;

	/** Time between updates for invisible objects. */
	FLOAT InvisibleUpdateTime;

	/** Min time between full environment updates. */
	FLOAT MinTimeBetweenFullUpdates;

	/** A pool of unused light components. */
	mutable TArray<ULightComponent*> RepresentativeLightPool;

	/** The character's current static light environment. */
	FSHVectorRGB StaticLightEnvironment;
	/** The current static shadow environment. */
	FSHVectorRGB StaticShadowEnvironment;

	/** The current dynamic light environment. */
	FSHVectorRGB DynamicLightEnvironment;
	/** The current dynamic shadow environment. */
	FSHVectorRGB DynamicShadowEnvironment;

	/** New static light environment to interpolate to. */
	FSHVectorRGB NewStaticLightEnvironment;
	/** New static shadow environment to interpolate to. */
	FSHVectorRGB NewStaticShadowEnvironment;

	/** The dynamic lights which affect the owner. */
	TArray<ULightComponent*> DynamicLights;

	/** Whether light environment has been fully updated at least once. */
	UBOOL bFirstFullUpdate;

	/** The positions relative to the owner's bounding box which are sampled for light visibility. */
	TArray<FVector> LightVisibilitySamplePoints;

	/**
	 * Determines whether a light is visible, using cached results if available.
	 * @param Light - The light to test visibility for.
	 * @param OwnerPosition - The position of the owner to compute the light's effect for.
	 * @param OutVisibilityFactor - Upon return, contains an appromiate percentage of light that reaches the owner's primitives.
	 * @return TRUE if the light reaches the owner's primitives.
	 */
	UBOOL IsLightVisible(const ULightComponent* Light,const FVector& OwnerPosition,FLOAT& OutVisibilityFactor);

	/** Allocates a light, attempting to reuse a light with matching type from the free light pool. */
	template<typename LightType>
	LightType* AllocateLight() const;

	/**
	 * Tests whether a light affects the owner.
	 * @param Light - The light to test.
	 * @param OwnerPosition - The position of the owner to compute the light's effect for.
	 * @return TRUE if the light affects the owner.
	 */
	UBOOL DoesLightAffectOwner(const ULightComponent* Light,const FVector& OwnerPosition);

	/**
	 * Adds the light's contribution to the light environment.
	 * @param	Light				light to add
	 * @param	LightEnvironment	light environment to add the light's contribution to
	 * @param	ShadowEnvironment	The shadow environment to add the light's shadowing to.
	 * @param	OwnerPosition		The position of the owner to compute the light's effect for.
	 */
	void AddLightToEnvironment( const ULightComponent* Light, FSHVectorRGB& LightEnvironment, FSHVectorRGB& ShadowEnvironment, const FVector& OwnerPosition );
};
