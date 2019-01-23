/*===========================================================================
    C++ class definitions exported from UnrealScript.
    This is automatically generated by the tools.
    DO NOT modify this manually! Edit the corresponding .uc files instead!
    Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
===========================================================================*/
#if SUPPORTS_PRAGMA_PACK
#pragma pack (push,4)
#endif

#include "EngineNames.h"

// Split enums from the rest of the header so they can be included earlier
// than the rest of the header file by including this file twice with different
// #define wrappers. See Engine.h and look at EngineClasses.h for an example.
#if !NO_ENUMS && !defined(NAMES_ONLY)

#ifndef INCLUDED_ENGINE_LIGHT_ENUMS
#define INCLUDED_ENGINE_LIGHT_ENUMS 1

enum EDynamicLightEnvironmentBoundsMethod
{
    DLEB_OwnerComponents    =0,
    DLEB_ManualOverride     =1,
    DLEB_ActiveComponents   =2,
    DLEB_MAX                =3,
};
#define FOREACH_ENUM_EDYNAMICLIGHTENVIRONMENTBOUNDSMETHOD(op) \
    op(DLEB_OwnerComponents) \
    op(DLEB_ManualOverride) \
    op(DLEB_ActiveComponents) 

#endif // !INCLUDED_ENGINE_LIGHT_ENUMS
#endif // !NO_ENUMS

#if !ENUMS_ONLY

#ifndef NAMES_ONLY
#define AUTOGENERATE_FUNCTION(cls,idx,name)
#endif


#ifndef NAMES_ONLY

#ifndef INCLUDED_ENGINE_LIGHT_CLASSES
#define INCLUDED_ENGINE_LIGHT_CLASSES 1
#define ENABLE_DECLARECLASS_MACRO 1
#include "UnObjBas.h"
#undef ENABLE_DECLARECLASS_MACRO

class ALightVolume : public AVolume
{
public:
    //## BEGIN PROPS LightVolume
    //## END PROPS LightVolume

    DECLARE_CLASS(ALightVolume,AVolume,0,Engine)
	/**
	 * Called after property has changed via e.g. property window or set command.
	 *
	 * @param	PropertyThatChanged	UProperty that has been changed, NULL if unknown
	 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);
};

class ALight : public AActor
{
public:
    //## BEGIN PROPS Light
    class ULightComponent* LightComponent;
    BITFIELD bEnabled:1;
    SCRIPT_ALIGN;
    //## END PROPS Light

    DECLARE_CLASS(ALight,AActor,0,Engine)
public:
	// AActor interface.
	/**
	 * Function that gets called from within Map_Check to allow this actor to check itself
	 * for any potential errors and register them with map check dialog.
	 */
#if WITH_EDITOR
	virtual void CheckForErrors();
#endif

	/**
	 * This will determine which icon should be displayed for this light.
	 **/
	virtual void DetermineAndSetEditorIcon();

	/**
	 * For this type of light, set the values which would make it affect Dynamic Primitives.
	 **/
	virtual void SetValuesForLight_DynamicAffecting();

	/**
	 * For this type of light, set the values which would make it affect Static Primitives.
	 **/
	virtual void SetValuesForLight_StaticAffecting();

	/**
	 * For this type of light, set the values which would make it affect Dynamic and Static Primitives.
	 **/
	virtual void SetValuesForLight_DynamicAndStaticAffecting();

	/**
	 * Returns true if the light supports being toggled off and on on-the-fly
	 *
	 * @return For 'toggleable' lights, returns true
	 */
	virtual UBOOL IsToggleable() const
	{
		// By default, lights are not toggleable.  You can override this in derived classes.
		return FALSE;
	}

    /** Invalidates lighting for a lighting rebuild. */
    void InvalidateLightingForRebuild();
};

class ADirectionalLight : public ALight
{
public:
    //## BEGIN PROPS DirectionalLight
    //## END PROPS DirectionalLight

    DECLARE_CLASS(ADirectionalLight,ALight,0,Engine)
public:
	/**
	 * This will determine which icon should be displayed for this light.
	 **/
	virtual void DetermineAndSetEditorIcon();

	/**
	 * Called from within SpawnActor, setting up the default value for the Lightmass light source angle.
	 */
	virtual void Spawned();
};

class ADirectionalLightToggleable : public ADirectionalLight
{
public:
    //## BEGIN PROPS DirectionalLightToggleable
    //## END PROPS DirectionalLightToggleable

    DECLARE_CLASS(ADirectionalLightToggleable,ADirectionalLight,0,Engine)
public:
	/**
	 * This will determine which icon should be displayed for this light.
	 **/
	virtual void DetermineAndSetEditorIcon();

	/** 
	 * Static affecting Toggleables can't have UseDirectLightmaps=TRUE  So even tho they are not "free" 
	 * lightmapped data, they still are classified as static as it is the best they can be.
	 **/
	virtual void SetValuesForLight_StaticAffecting();

	/**
	 * Returns true if the light supports being toggled off and on on-the-fly
	 *
	 * @return For 'toggleable' lights, returns true
	 **/
	virtual UBOOL IsToggleable() const
	{
		// DirectionalLightToggleable supports being toggled on the fly!
		return TRUE;
	}
};

class ADominantDirectionalLight : public ADirectionalLight
{
public:
    //## BEGIN PROPS DominantDirectionalLight
    //## END PROPS DominantDirectionalLight

    DECLARE_CLASS(ADominantDirectionalLight,ADirectionalLight,0,Engine)
	// UObject interface
#if WITH_EDITOR
	virtual void CheckForErrors();
#endif

	/**
	 * Returns true if the light supports being toggled off and on on-the-fly
	 **/
	virtual UBOOL IsToggleable() const
	{
		return TRUE;
	}
};

class ADominantDirectionalLightMovable : public ADominantDirectionalLight
{
public:
    //## BEGIN PROPS DominantDirectionalLightMovable
    //## END PROPS DominantDirectionalLightMovable

    DECLARE_CLASS(ADominantDirectionalLightMovable,ADominantDirectionalLight,0,Engine)
    NO_DEFAULT_CONSTRUCTOR(ADominantDirectionalLightMovable)
};

class APointLight : public ALight
{
public:
    //## BEGIN PROPS PointLight
    //## END PROPS PointLight

    DECLARE_CLASS(APointLight,ALight,0,Engine)
#if WITH_EDITOR
	// AActor interface.
	virtual void EditorApplyScale(const FVector& DeltaScale, const FMatrix& ScaleMatrix, const FVector* PivotLocation, UBOOL bAltDown, UBOOL bShiftDown, UBOOL bCtrlDown);
#endif
	/**
	 * This will determine which icon should be displayed for this light.
	 **/
	virtual void DetermineAndSetEditorIcon();

	/**
	 * Called after this actor has been pasted into a level.  Attempts to catch cases where designers are pasting in really old
	 * T3D data that was created when component instancing wasn't working correctly.
	 */
	virtual void PostEditImport();

	/**
	 * Called from within SpawnActor, setting up the default value for the Lightmass light source radius.
	 */
	virtual void Spawned();
};

class ADominantPointLight : public APointLight
{
public:
    //## BEGIN PROPS DominantPointLight
    //## END PROPS DominantPointLight

    DECLARE_CLASS(ADominantPointLight,APointLight,0,Engine)
	/**
	 * Returns true if the light supports being toggled off and on on-the-fly
	 **/
	virtual UBOOL IsToggleable() const
	{
		return TRUE;
	}
};

class APointLightMovable : public APointLight
{
public:
    //## BEGIN PROPS PointLightMovable
    //## END PROPS PointLightMovable

    DECLARE_CLASS(APointLightMovable,APointLight,0,Engine)
public:
	/**
	 * This will determine which icon should be displayed for this light.
	 **/
	virtual void DetermineAndSetEditorIcon();

	/**
	 * Returns true if the light supports being toggled off and on on-the-fly
	 *
	 * @return For 'toggleable' lights, returns true
	 **/
	virtual UBOOL IsToggleable() const
	{
		// PointLightMovable supports being toggled on the fly!
		return TRUE;
	}
};

class APointLightToggleable : public APointLight
{
public:
    //## BEGIN PROPS PointLightToggleable
    //## END PROPS PointLightToggleable

    DECLARE_CLASS(APointLightToggleable,APointLight,0,Engine)
public:
	/**
	 * This will determine which icon should be displayed for this light.
	 **/
	virtual void DetermineAndSetEditorIcon();

	/**
	 * Static affecting Toggleables can't have UseDirectLightmaps=TRUE  So even tho they are not "free"
	 * lightmapped data, they still are classified as static as it is the best they can be.
	 **/
	virtual void SetValuesForLight_StaticAffecting();

	/**
	 * Returns true if the light supports being toggled off and on on-the-fly
	 *
	 * @return For 'toggleable' lights, returns true
	 **/
	virtual UBOOL IsToggleable() const
	{
		// PointLightToggleable supports being toggled on the fly!
		return TRUE;
	}
};

class ASkyLight : public ALight
{
public:
    //## BEGIN PROPS SkyLight
    //## END PROPS SkyLight

    DECLARE_CLASS(ASkyLight,ALight,0,Engine)
    NO_DEFAULT_CONSTRUCTOR(ASkyLight)
};

class ASkyLightToggleable : public ASkyLight
{
public:
    //## BEGIN PROPS SkyLightToggleable
    //## END PROPS SkyLightToggleable

    DECLARE_CLASS(ASkyLightToggleable,ASkyLight,0,Engine)
public:
	/**
	 * Returns true if the light supports being toggled off and on on-the-fly
	 *
	 * @return For 'toggleable' lights, returns true
	 **/
	virtual UBOOL IsToggleable() const
	{
		// SkyLightToggleable supports being toggled on the fly!
		return TRUE;
	}
};

class ASpotLight : public ALight
{
public:
    //## BEGIN PROPS SpotLight
    //## END PROPS SpotLight

    DECLARE_CLASS(ASpotLight,ALight,0,Engine)
#if WITH_EDITOR
	// AActor interface.
	virtual void EditorApplyScale(const FVector& DeltaScale, const FMatrix& ScaleMatrix, const FVector* PivotLocation, UBOOL bAltDown, UBOOL bShiftDown, UBOOL bCtrlDown);
#endif
	/**
	 * This will determine which icon should be displayed for this light.
	 **/
	virtual void DetermineAndSetEditorIcon();

	/**
	 * Called after this actor has been pasted into a level.  Attempts to catch cases where designers are pasting in really old
	 * T3D data that was created when component instancing wasn't working correctly.
	 */
	virtual void PostEditImport();

    /**
	 * Called from within SpawnActor, setting up the default value for the Lightmass light source radius.
	 */
	virtual void Spawned();
};

class ADominantSpotLight : public ASpotLight
{
public:
    //## BEGIN PROPS DominantSpotLight
    //## END PROPS DominantSpotLight

    DECLARE_CLASS(ADominantSpotLight,ASpotLight,0,Engine)
	/**
	 * Returns true if the light supports being toggled off and on on-the-fly
	 **/
	virtual UBOOL IsToggleable() const
	{
		return TRUE;
	}
};

class AGeneratedMeshAreaLight : public ASpotLight
{
public:
    //## BEGIN PROPS GeneratedMeshAreaLight
    //## END PROPS GeneratedMeshAreaLight

    DECLARE_CLASS(AGeneratedMeshAreaLight,ASpotLight,0,Engine)
    NO_DEFAULT_CONSTRUCTOR(AGeneratedMeshAreaLight)
};

class ASpotLightMovable : public ASpotLight
{
public:
    //## BEGIN PROPS SpotLightMovable
    //## END PROPS SpotLightMovable

    DECLARE_CLASS(ASpotLightMovable,ASpotLight,0,Engine)
public:
	/**
	 * This will determine which icon should be displayed for this light.
	 **/
	virtual void DetermineAndSetEditorIcon();

	/**
	 * Returns true if the light supports being toggled off and on on-the-fly
	 *
	 * @return For 'toggleable' lights, returns true
	 **/
	virtual UBOOL IsToggleable() const
	{
		// SpotLightMovable supports being toggled on the fly!
		return TRUE;
	}

};

class ASpotLightToggleable : public ASpotLight
{
public:
    //## BEGIN PROPS SpotLightToggleable
    //## END PROPS SpotLightToggleable

    DECLARE_CLASS(ASpotLightToggleable,ASpotLight,0,Engine)
public:
	/**
	 * This will determine which icon should be displayed for this light.
	 **/
	virtual void DetermineAndSetEditorIcon();

	/**
	 * Static affecting Toggleables can't have UseDirectLightmaps=TRUE  So even tho they are not "free"
	 * lightmapped data, they still are classified as static as it is the best they can be.
	 **/
	virtual void SetValuesForLight_StaticAffecting();

	/**
	 * Returns true if the light supports being toggled off and on on-the-fly
	 *
	 * @return For 'toggleable' lights, returns true
	 **/
	virtual UBOOL IsToggleable() const
	{
		// SpotLightToggleable supports being toggled on the fly!
		return TRUE;
	}
};

class AStaticLightCollectionActor : public ALight
{
public:
    //## BEGIN PROPS StaticLightCollectionActor
    TArrayNoInit<class ULightComponent*> LightComponents;
    INT MaxLightComponents;
    //## END PROPS StaticLightCollectionActor

    DECLARE_CLASS(AStaticLightCollectionActor,ALight,0|CLASS_Config,Engine)
    static const TCHAR* StaticConfigName() {return TEXT("Engine");}

	/* === AActor interface === */
	/**
	 * Updates the CachedLocalToWorld transform for all attached components.
	 */
	virtual void UpdateComponentsInternal( UBOOL bCollisionUpdate=FALSE );


	/* === UObject interface === */
	/**
	 * Serializes the LocalToWorld transforms for the StaticMeshComponents contained in this actor.
	 */
	virtual void Serialize( FArchive& Ar );
};

class UDirectionalLightComponent : public ULightComponent
{
public:
    //## BEGIN PROPS DirectionalLightComponent
    FLOAT TraceDistance;
    FLOAT WholeSceneDynamicShadowRadius;
    INT NumWholeSceneDynamicShadowCascades;
    FLOAT CascadeDistributionExponent;
    struct FLightmassDirectionalLightSettings LightmassSettings;
    //## END PROPS DirectionalLightComponent

    DECLARE_CLASS(UDirectionalLightComponent,ULightComponent,0,Engine)
	virtual FLightSceneInfo* CreateSceneInfo() const;
	virtual FVector4 GetPosition() const;
	virtual ELightComponentType GetLightType() const;

	/**
	 * Called after property has changed via e.g. property window or set command.
	 *
	 * @param	PropertyThatChanged	UProperty that has been changed, NULL if unknown
	 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);
};

class UDominantDirectionalLightComponent : public UDirectionalLightComponent
{
public:
    //## BEGIN PROPS DominantDirectionalLightComponent
private:
    struct FDominantShadowInfo DominantLightShadowInfo;
    TArrayNoInit<WORD> DominantLightShadowMap;
public:
    //## END PROPS DominantDirectionalLightComponent

    DECLARE_CLASS(UDominantDirectionalLightComponent,UDirectionalLightComponent,0,Engine)
    virtual void Serialize(FArchive& Ar);
	virtual ELightComponentType GetLightType() const;
    virtual void InvalidateLightingCache();
	/**
	* Called after property has changed via e.g. property window or set command.
	*
	* @param	PropertyThatChanged	UProperty that has been changed, NULL if unknown
	*/
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);
	virtual void FinishDestroy();
    /** Returns information about the data used to calculate dominant shadow transition distance. */
    void GetInfo(INT& SizeX, INT& SizeY, SIZE_T& ShadowMapBytes) const;
    /** Populates DominantLightShadowMap and DominantLightShadowInfo with the results from a lighting build. */
    void Initialize(const FDominantShadowInfo& InInfo, const TArray<WORD>& InShadowMap);
    /** Returns the distance to the nearest dominant shadow transition, in world space units, starting from the edge of the bounds. */
    FLOAT GetDominantShadowTransitionDistance(const FBoxSphereBounds& Bounds, FLOAT MaxSearchDistance, UBOOL bDebugSearch, TArray<class FDebugShadowRay>& DebugRays, UBOOL& bLightingIsBuilt) const;
};

class UPointLightComponent : public ULightComponent
{
public:
    //## BEGIN PROPS PointLightComponent
    FLOAT ShadowRadiusMultiplier;
    FLOAT Radius;
    FLOAT FalloffExponent;
    FLOAT ShadowFalloffExponent;
    FLOAT MinShadowFalloffRadius;
    FMatrix CachedParentToWorld;
    FVector Translation;
    FPlane ShadowPlane;
    class UDrawLightRadiusComponent* PreviewLightRadius;
    struct FLightmassPointLightSettings LightmassSettings;
    class UDrawLightRadiusComponent* PreviewLightSourceRadius;
    //## END PROPS PointLightComponent

    void SetTranslation(FVector NewTranslation);
    DECLARE_FUNCTION(execSetTranslation)
    {
        P_GET_STRUCT(FVector,NewTranslation);
        P_FINISH;
        this->SetTranslation(NewTranslation);
    }
    DECLARE_CLASS(UPointLightComponent,ULightComponent,0,Engine)
protected:
	/**
	 * Updates the light's PreviewLightRadius.
	 */
	void UpdatePreviewLightRadius();

	// UActorComponent interface.
	virtual void SetParentToWorld(const FMatrix& ParentToWorld);
	virtual void Attach();
	virtual void UpdateTransform();
public:

	// ULightComponent interface.
	virtual FLightSceneInfo* CreateSceneInfo() const;
	virtual UBOOL AffectsBounds(const FBoxSphereBounds& Bounds) const;
	virtual FVector4 GetPosition() const;
	virtual FBox GetBoundingBox() const;
	virtual FLinearColor GetDirectIntensity(const FVector& Point) const;
	virtual ELightComponentType GetLightType() const;

	// update the LocalToWorld matrix
	virtual void SetTransformedToWorld();

	/**
	 * Called after property has changed via e.g. property window or set command.
	 *
	 * @param	PropertyThatChanged	UProperty that has been changed, NULL if unknown
	 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);

	virtual void PostLoad();

	/** Update the PreviewLightSourceRadius */
	virtual void UpdatePreviewLightSourceRadius();
};

class UDominantPointLightComponent : public UPointLightComponent
{
public:
    //## BEGIN PROPS DominantPointLightComponent
    //## END PROPS DominantPointLightComponent

    DECLARE_CLASS(UDominantPointLightComponent,UPointLightComponent,0,Engine)
	/**
	* Called after property has changed via e.g. property window or set command.
	*
	* @param	PropertyThatChanged	UProperty that has been changed, NULL if unknown
	*/
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);
    virtual ELightComponentType GetLightType() const;
    /** Returns the distance to the nearest dominant shadow transition, in world space units, starting from the edge of the bounds. */
    FLOAT GetDominantShadowTransitionDistance(const FBoxSphereBounds& Bounds, FLOAT MaxSearchDistance, UBOOL bDebugSearch, TArray<class FDebugShadowRay>& DebugRays, UBOOL& bLightingIsBuilt) const;
};

class USpotLightComponent : public UPointLightComponent
{
public:
    //## BEGIN PROPS SpotLightComponent
    FLOAT InnerConeAngle;
    FLOAT OuterConeAngle;
    FLOAT LightShaftConeAngle;
    class UDrawLightConeComponent* PreviewInnerCone;
    class UDrawLightConeComponent* PreviewOuterCone;
    FRotator Rotation;
    //## END PROPS SpotLightComponent

    void SetRotation(FRotator NewRotation);
    DECLARE_FUNCTION(execSetRotation)
    {
        P_GET_STRUCT(FRotator,NewRotation);
        P_FINISH;
        this->SetRotation(NewRotation);
    }
    DECLARE_CLASS(USpotLightComponent,UPointLightComponent,0,Engine)
	// UActorComponent interface.
	virtual void Attach();

	// ULightComponent interface.
	virtual FLightSceneInfo* CreateSceneInfo() const;
	virtual UBOOL AffectsBounds(const FBoxSphereBounds& Bounds) const;
	virtual FLinearColor GetDirectIntensity(const FVector& Point) const;
	virtual ELightComponentType GetLightType() const;
	virtual void PostLoad();

	// update the LocalToWorld matrix
	virtual void SetTransformedToWorld();
};

class UDominantSpotLightComponent : public USpotLightComponent
{
public:
    //## BEGIN PROPS DominantSpotLightComponent
private:
    struct FDominantShadowInfo DominantLightShadowInfo;
    TArrayNoInit<WORD> DominantLightShadowMap;
public:
    //## END PROPS DominantSpotLightComponent

    DECLARE_CLASS(UDominantSpotLightComponent,USpotLightComponent,0,Engine)
	virtual void Serialize(FArchive& Ar);
	virtual ELightComponentType GetLightType() const;
	virtual void InvalidateLightingCache();
	/**
	* Called after property has changed via e.g. property window or set command.
	*
	* @param	PropertyThatChanged	UProperty that has been changed, NULL if unknown
	*/
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);
	virtual void FinishDestroy();
	/** Returns information about the data used to calculate dominant shadow transition distance. */
	void GetInfo(INT& SizeX, INT& SizeY, SIZE_T& ShadowMapBytes) const;
	/** Populates DominantLightShadowMap and DominantLightShadowInfo with the results from a lighting build. */
	void Initialize(const FDominantShadowInfo& InInfo, const TArray<WORD>& InShadowMap);
    /** Returns the distance to the nearest dominant shadow transition, in world space units, starting from the edge of the bounds. */
    FLOAT GetDominantShadowTransitionDistance(const FBoxSphereBounds& Bounds, FLOAT MaxSearchDistance, UBOOL bDebugSearch, TArray<class FDebugShadowRay>& DebugRays, UBOOL& bLightingIsBuilt) const;
};

class USkyLightComponent : public ULightComponent
{
public:
    //## BEGIN PROPS SkyLightComponent
    FLOAT LowerBrightness;
    FColor LowerColor;
    //## END PROPS SkyLightComponent

    DECLARE_CLASS(USkyLightComponent,ULightComponent,0,Engine)
	/**
	 * Called when a property is being changed.
	 *
	 * @param PropertyThatChanged	Property that changed or NULL if unknown or multiple
	 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);
	/**
	 * Called after data has been serialized.
	 */
	virtual void PostLoad();

	// ULightComponent interface.
	virtual FLightSceneInfo* CreateSceneInfo() const;
	virtual FVector4 GetPosition() const;
	virtual ELightComponentType GetLightType() const;
};

class USphericalHarmonicLightComponent : public ULightComponent
{
public:
    //## BEGIN PROPS SphericalHarmonicLightComponent
    FSHVectorRGB WorldSpaceIncidentLighting;
    BITFIELD bRenderBeforeModShadows:1;
    SCRIPT_ALIGN;
    //## END PROPS SphericalHarmonicLightComponent

    DECLARE_CLASS(USphericalHarmonicLightComponent,ULightComponent,0,Engine)
	// ULightComponent interface.
	virtual FLightSceneInfo* CreateSceneInfo() const;
	virtual FVector4 GetPosition() const;
	virtual ELightComponentType GetLightType() const;
};

class ULightEnvironmentComponent : public UActorComponent
{
public:
    //## BEGIN PROPS LightEnvironmentComponent
protected:
    BITFIELD bEnabled:1;
public:
    BITFIELD bForceNonCompositeDynamicLights:1;
    BITFIELD bAllowDynamicShadowsOnTranslucency:1;
protected:
    BITFIELD bAllowPreShadow:1;
    BITFIELD bTranslucencyShadowed:1;
    FLOAT DominantShadowFactor;
    class ULightComponent* AffectingDominantLight;
    TArrayNoInit<class UPrimitiveComponent*> AffectedComponents;
public:
    //## END PROPS LightEnvironmentComponent

    void SetEnabled(UBOOL bNewEnabled);
    UBOOL IsEnabled() const;
    DECLARE_FUNCTION(execSetEnabled)
    {
        P_GET_UBOOL(bNewEnabled);
        P_FINISH;
        this->SetEnabled(bNewEnabled);
    }
    DECLARE_FUNCTION(execIsEnabled)
    {
        P_FINISH;
        *(UBOOL*)Result=this->IsEnabled();
    }
    DECLARE_CLASS(ULightEnvironmentComponent,UActorComponent,0,Engine)
	/**
	 * Signals to the light environment that a light has changed, so the environment may need to be updated.
	 * @param Light - The light that changed.
	 */
	virtual void UpdateLight(const ULightComponent* Light) {}

	// Methods that update AffectedComponents
	void AddAffectedComponent(UPrimitiveComponent* NewComponent);
	void RemoveAffectedComponent(UPrimitiveComponent* OldComponent);

	// Accessors
	const ULightComponent* GetAffectingDominantLight() const { return AffectingDominantLight; }
	UBOOL AllowDynamicShadowsOnTranslucency() const { return bAllowDynamicShadowsOnTranslucency; }
	UBOOL IsTranslucencyShadowed() const { return bTranslucencyShadowed; }
	UBOOL AllowPreShadow() const { return bAllowPreShadow; }
	FLOAT GetDominantShadowFactor() const { return DominantShadowFactor; }

	/** Adds lights that affect this Light Environment to RelevantLightList. */
	virtual void AddRelevantLights(TArray<ALight*>& RelevantLightList, UBOOL bDominantOnly) const {}

	friend class FDynamicLightEnvironmentState;
	friend void DrawLightEnvironmentDebugInfo(const FSceneView* View,FPrimitiveDrawInterface* PDI);
};

class UDynamicLightEnvironmentComponent : public ULightEnvironmentComponent
{
public:
    //## BEGIN PROPS DynamicLightEnvironmentComponent
    class FDynamicLightEnvironmentState* State;
    FLOAT InvisibleUpdateTime;
    FLOAT MinTimeBetweenFullUpdates;
    FLOAT ShadowInterpolationSpeed;
    INT NumVolumeVisibilitySamples;
    FLOAT LightingBoundsScale;
    FLinearColor AmbientShadowColor;
    FVector AmbientShadowSourceDirection;
    FLinearColor AmbientGlow;
    FLOAT LightDesaturation;
    FLOAT LightDistance;
    FLOAT ShadowDistance;
    BITFIELD bCastShadows:1;
    BITFIELD bCompositeShadowsFromDynamicLights:1;
    BITFIELD bForceCompositeAllLights:1;
    BITFIELD bAffectedBySmallDynamicLights:1;
    BITFIELD bUseBooleanEnvironmentShadowing:1;
    BITFIELD bShadowFromEnvironment:1;
    BITFIELD bDynamic:1;
    BITFIELD bSynthesizeDirectionalLight:1;
    BITFIELD bSynthesizeSHLight:1;
    BITFIELD bForceAllowLightEnvSphericalHarmonicLights:1;
    BITFIELD bRequiresNonLatentUpdates:1;
    BITFIELD bTraceFromClosestBoundsPoint:1;
    BITFIELD bIsCharacterLightEnvironment:1;
    BITFIELD bOverrideOwnerLightingChannels:1;
    FLOAT ModShadowFadeoutTime;
    FLOAT ModShadowFadeoutExponent;
    FLinearColor MaxModulatedShadowColor;
    FLOAT DominantShadowTransitionStartDistance;
    FLOAT DominantShadowTransitionEndDistance;
    INT MinShadowResolution;
    INT MaxShadowResolution;
    INT ShadowFadeResolution;
    BYTE ShadowFilterQuality;
    BYTE LightShadowMode;
    BYTE BoundsMethod;
    FLOAT BouncedLightingFactor;
    FLOAT MinShadowAngle;
    FBoxSphereBounds OverriddenBounds;
    FLightingChannelContainer OverriddenLightingChannels;
    TArrayNoInit<class ULightComponent*> OverriddenLightComponents;
    //## END PROPS DynamicLightEnvironmentComponent

    void ResetEnvironment();
    DECLARE_FUNCTION(execResetEnvironment)
    {
        P_FINISH;
        this->ResetEnvironment();
    }
    DECLARE_CLASS(UDynamicLightEnvironmentComponent,ULightEnvironmentComponent,0,Engine)
	// UObject interface.
	virtual void FinishDestroy();
	virtual void AddReferencedObjects( TArray<UObject*>& ObjectArray );
	virtual void Serialize(FArchive& Ar);
	virtual void PostLoad();
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);

	// UActorComponent interface.
	virtual void Tick(FLOAT DeltaTime);
	virtual void Attach();
	virtual void UpdateTransform();
	virtual void Detach( UBOOL bWillReattach = FALSE );
	virtual void BeginPlay();
#if WITH_EDITOR
	virtual void CheckForErrors();
#endif
	/** Adds lights that affect this DLE to RelevantLightList. */
	virtual void AddRelevantLights(TArray<ALight*>& RelevantLightList, UBOOL bDominantOnly) const;

	// ULightEnvironmentComponent interface.
	virtual void UpdateLight(const ULightComponent* Light);

	friend class FDynamicLightEnvironmentState;
};

class UParticleLightEnvironmentComponent : public UDynamicLightEnvironmentComponent
{
public:
    //## BEGIN PROPS ParticleLightEnvironmentComponent
protected:
    INT ReferenceCount;
public:
    BITFIELD bAllowDLESharing:1;
    SCRIPT_ALIGN;
    //## END PROPS ParticleLightEnvironmentComponent

    DECLARE_CLASS(UParticleLightEnvironmentComponent,UDynamicLightEnvironmentComponent,0,Engine)
	inline void AddRef() { ReferenceCount++; }
	inline void RemoveRef() 
	{ 
		check(ReferenceCount > 0);
		ReferenceCount--; 
	}
	inline INT GetRefCount() const { return ReferenceCount; }

	virtual void UpdateLight(const ULightComponent* Light);

	// UActorComponent interface.
	virtual void Tick(FLOAT DeltaTime);
};

class UDrawLightConeComponent : public UDrawConeComponent
{
public:
    //## BEGIN PROPS DrawLightConeComponent
    //## END PROPS DrawLightConeComponent

    DECLARE_CLASS(UDrawLightConeComponent,UDrawConeComponent,0,Engine)
	/**
	 * Creates a proxy to represent the primitive to the scene manager in the rendering thread.
	 * @return The proxy object.
	 */
	virtual FPrimitiveSceneProxy* CreateSceneProxy();
};

class ULightFunction : public UObject
{
public:
    //## BEGIN PROPS LightFunction
    class UMaterialInterface* SourceMaterial;
    FVector Scale;
    FLOAT DisabledBrightness;
    //## END PROPS LightFunction

    DECLARE_CLASS(ULightFunction,UObject,0,Engine)
    NO_DEFAULT_CONSTRUCTOR(ULightFunction)
};

#undef DECLARE_CLASS
#undef DECLARE_CASTED_CLASS
#undef DECLARE_ABSTRACT_CLASS
#undef DECLARE_ABSTRACT_CASTED_CLASS
#endif // !INCLUDED_ENGINE_LIGHT_CLASSES
#endif // !NAMES_ONLY

AUTOGENERATE_FUNCTION(UPointLightComponent,-1,execSetTranslation);
AUTOGENERATE_FUNCTION(USpotLightComponent,-1,execSetRotation);
AUTOGENERATE_FUNCTION(ULightEnvironmentComponent,-1,execIsEnabled);
AUTOGENERATE_FUNCTION(ULightEnvironmentComponent,-1,execSetEnabled);
AUTOGENERATE_FUNCTION(UDynamicLightEnvironmentComponent,-1,execResetEnvironment);

#ifndef NAMES_ONLY
#undef AUTOGENERATE_FUNCTION
#endif

#ifdef STATIC_LINKING_MOJO
#ifndef ENGINE_LIGHT_NATIVE_DEFS
#define ENGINE_LIGHT_NATIVE_DEFS

#define AUTO_INITIALIZE_REGISTRANTS_ENGINE_LIGHT \
	ALightVolume::StaticClass(); \
	ALight::StaticClass(); \
	ADirectionalLight::StaticClass(); \
	ADirectionalLightToggleable::StaticClass(); \
	ADominantDirectionalLight::StaticClass(); \
	ADominantDirectionalLightMovable::StaticClass(); \
	APointLight::StaticClass(); \
	ADominantPointLight::StaticClass(); \
	APointLightMovable::StaticClass(); \
	APointLightToggleable::StaticClass(); \
	ASkyLight::StaticClass(); \
	ASkyLightToggleable::StaticClass(); \
	ASpotLight::StaticClass(); \
	ADominantSpotLight::StaticClass(); \
	AGeneratedMeshAreaLight::StaticClass(); \
	ASpotLightMovable::StaticClass(); \
	ASpotLightToggleable::StaticClass(); \
	AStaticLightCollectionActor::StaticClass(); \
	ULightComponent::StaticClass(); \
	GNativeLookupFuncs.Set(FName("LightComponent"), GEngineULightComponentNatives); \
	UDirectionalLightComponent::StaticClass(); \
	UDominantDirectionalLightComponent::StaticClass(); \
	UPointLightComponent::StaticClass(); \
	GNativeLookupFuncs.Set(FName("PointLightComponent"), GEngineUPointLightComponentNatives); \
	UDominantPointLightComponent::StaticClass(); \
	USpotLightComponent::StaticClass(); \
	GNativeLookupFuncs.Set(FName("SpotLightComponent"), GEngineUSpotLightComponentNatives); \
	UDominantSpotLightComponent::StaticClass(); \
	USkyLightComponent::StaticClass(); \
	USphericalHarmonicLightComponent::StaticClass(); \
	ULightEnvironmentComponent::StaticClass(); \
	GNativeLookupFuncs.Set(FName("LightEnvironmentComponent"), GEngineULightEnvironmentComponentNatives); \
	UDynamicLightEnvironmentComponent::StaticClass(); \
	GNativeLookupFuncs.Set(FName("DynamicLightEnvironmentComponent"), GEngineUDynamicLightEnvironmentComponentNatives); \
	UParticleLightEnvironmentComponent::StaticClass(); \
	UDrawLightConeComponent::StaticClass(); \
	UDrawLightRadiusComponent::StaticClass(); \
	ULightFunction::StaticClass(); \

#endif // ENGINE_LIGHT_NATIVE_DEFS

#ifdef NATIVES_ONLY
FNativeFunctionLookup GEngineULightComponentNatives[] = 
{ 
	MAP_NATIVE(ULightComponent, execUpdateLightShaftParameters)
	MAP_NATIVE(ULightComponent, execUpdateColorAndBrightness)
	MAP_NATIVE(ULightComponent, execGetDirection)
	MAP_NATIVE(ULightComponent, execGetOrigin)
	MAP_NATIVE(ULightComponent, execSetLightProperties)
	MAP_NATIVE(ULightComponent, execSetEnabled)
	{NULL, NULL}
};

FNativeFunctionLookup GEngineUPointLightComponentNatives[] = 
{ 
	MAP_NATIVE(UPointLightComponent, execSetTranslation)
	{NULL, NULL}
};

FNativeFunctionLookup GEngineUSpotLightComponentNatives[] = 
{ 
	MAP_NATIVE(USpotLightComponent, execSetRotation)
	{NULL, NULL}
};

FNativeFunctionLookup GEngineULightEnvironmentComponentNatives[] = 
{ 
	MAP_NATIVE(ULightEnvironmentComponent, execIsEnabled)
	MAP_NATIVE(ULightEnvironmentComponent, execSetEnabled)
	{NULL, NULL}
};

FNativeFunctionLookup GEngineUDynamicLightEnvironmentComponentNatives[] = 
{ 
	MAP_NATIVE(UDynamicLightEnvironmentComponent, execResetEnvironment)
	{NULL, NULL}
};

#endif // NATIVES_ONLY
#endif // STATIC_LINKING_MOJO

#ifdef VERIFY_CLASS_SIZES
VERIFY_CLASS_SIZE_NODIE(ALightVolume)
VERIFY_CLASS_OFFSET_NODIE(ALight,Light,LightComponent)
VERIFY_CLASS_SIZE_NODIE(ALight)
VERIFY_CLASS_SIZE_NODIE(ADirectionalLight)
VERIFY_CLASS_SIZE_NODIE(ADirectionalLightToggleable)
VERIFY_CLASS_SIZE_NODIE(ADominantDirectionalLight)
VERIFY_CLASS_SIZE_NODIE(ADominantDirectionalLightMovable)
VERIFY_CLASS_SIZE_NODIE(APointLight)
VERIFY_CLASS_SIZE_NODIE(ADominantPointLight)
VERIFY_CLASS_SIZE_NODIE(APointLightMovable)
VERIFY_CLASS_SIZE_NODIE(APointLightToggleable)
VERIFY_CLASS_SIZE_NODIE(ASkyLight)
VERIFY_CLASS_SIZE_NODIE(ASkyLightToggleable)
VERIFY_CLASS_SIZE_NODIE(ASpotLight)
VERIFY_CLASS_SIZE_NODIE(ADominantSpotLight)
VERIFY_CLASS_SIZE_NODIE(AGeneratedMeshAreaLight)
VERIFY_CLASS_SIZE_NODIE(ASpotLightMovable)
VERIFY_CLASS_SIZE_NODIE(ASpotLightToggleable)
VERIFY_CLASS_OFFSET_NODIE(AStaticLightCollectionActor,StaticLightCollectionActor,LightComponents)
VERIFY_CLASS_OFFSET_NODIE(AStaticLightCollectionActor,StaticLightCollectionActor,MaxLightComponents)
VERIFY_CLASS_SIZE_NODIE(AStaticLightCollectionActor)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,SceneInfo)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,WorldToLight)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,LightToWorld)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,LightGuid)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,LightmapGuid)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,Brightness)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,LightColor)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,Function)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,LightEnv_BouncedLightBrightness)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,LightEnv_BouncedModulationColor)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,LightEnvironment)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,OtherLevelsToAffect)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,LightingChannels)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,InclusionVolumes)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,ExclusionVolumes)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,InclusionConvexVolumes)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,ExclusionConvexVolumes)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,LightAffectsClassification)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,LightShadowMode)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,ModShadowColor)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,ModShadowFadeoutTime)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,ModShadowFadeoutExponent)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,LightListIndex)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,ShadowProjectionTechnique)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,ShadowFilterQuality)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,MinShadowResolution)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,MaxShadowResolution)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,ShadowFadeResolution)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,OcclusionDepthRange)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,BloomScale)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,BloomThreshold)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,BloomScreenBlendThreshold)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,BloomTint)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,RadialBlurPercent)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,OcclusionMaskDarkness)
VERIFY_CLASS_OFFSET_NODIE(ULightComponent,LightComponent,ReflectionSpecularBrightness)
VERIFY_CLASS_SIZE_NODIE(ULightComponent)
VERIFY_CLASS_OFFSET_NODIE(UDirectionalLightComponent,DirectionalLightComponent,TraceDistance)
VERIFY_CLASS_OFFSET_NODIE(UDirectionalLightComponent,DirectionalLightComponent,LightmassSettings)
VERIFY_CLASS_SIZE_NODIE(UDirectionalLightComponent)
VERIFY_CLASS_OFFSET_NODIE(UDominantDirectionalLightComponent,DominantDirectionalLightComponent,DominantLightShadowInfo)
VERIFY_CLASS_OFFSET_NODIE(UDominantDirectionalLightComponent,DominantDirectionalLightComponent,DominantLightShadowMap)
VERIFY_CLASS_SIZE_NODIE(UDominantDirectionalLightComponent)
VERIFY_CLASS_OFFSET_NODIE(UPointLightComponent,PointLightComponent,ShadowRadiusMultiplier)
VERIFY_CLASS_OFFSET_NODIE(UPointLightComponent,PointLightComponent,PreviewLightSourceRadius)
VERIFY_CLASS_SIZE_NODIE(UPointLightComponent)
VERIFY_CLASS_SIZE_NODIE(UDominantPointLightComponent)
VERIFY_CLASS_OFFSET_NODIE(USpotLightComponent,SpotLightComponent,InnerConeAngle)
VERIFY_CLASS_OFFSET_NODIE(USpotLightComponent,SpotLightComponent,Rotation)
VERIFY_CLASS_SIZE_NODIE(USpotLightComponent)
VERIFY_CLASS_OFFSET_NODIE(UDominantSpotLightComponent,DominantSpotLightComponent,DominantLightShadowInfo)
VERIFY_CLASS_OFFSET_NODIE(UDominantSpotLightComponent,DominantSpotLightComponent,DominantLightShadowMap)
VERIFY_CLASS_SIZE_NODIE(UDominantSpotLightComponent)
VERIFY_CLASS_OFFSET_NODIE(USkyLightComponent,SkyLightComponent,LowerBrightness)
VERIFY_CLASS_OFFSET_NODIE(USkyLightComponent,SkyLightComponent,LowerColor)
VERIFY_CLASS_SIZE_NODIE(USkyLightComponent)
VERIFY_CLASS_OFFSET_NODIE(USphericalHarmonicLightComponent,SphericalHarmonicLightComponent,WorldSpaceIncidentLighting)
VERIFY_CLASS_SIZE_NODIE(USphericalHarmonicLightComponent)
VERIFY_CLASS_OFFSET_NODIE(ULightEnvironmentComponent,LightEnvironmentComponent,DominantShadowFactor)
VERIFY_CLASS_OFFSET_NODIE(ULightEnvironmentComponent,LightEnvironmentComponent,AffectedComponents)
VERIFY_CLASS_SIZE_NODIE(ULightEnvironmentComponent)
VERIFY_CLASS_OFFSET_NODIE(UDynamicLightEnvironmentComponent,DynamicLightEnvironmentComponent,State)
VERIFY_CLASS_OFFSET_NODIE(UDynamicLightEnvironmentComponent,DynamicLightEnvironmentComponent,OverriddenLightComponents)
VERIFY_CLASS_SIZE_NODIE(UDynamicLightEnvironmentComponent)
VERIFY_CLASS_OFFSET_NODIE(UParticleLightEnvironmentComponent,ParticleLightEnvironmentComponent,ReferenceCount)
VERIFY_CLASS_SIZE_NODIE(UParticleLightEnvironmentComponent)
VERIFY_CLASS_SIZE_NODIE(UDrawLightConeComponent)
VERIFY_CLASS_SIZE_NODIE(UDrawLightRadiusComponent)
VERIFY_CLASS_OFFSET_NODIE(ULightFunction,LightFunction,SourceMaterial)
VERIFY_CLASS_OFFSET_NODIE(ULightFunction,LightFunction,DisabledBrightness)
VERIFY_CLASS_SIZE_NODIE(ULightFunction)
#endif // VERIFY_CLASS_SIZES
#endif // !ENUMS_ONLY

#if SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif
