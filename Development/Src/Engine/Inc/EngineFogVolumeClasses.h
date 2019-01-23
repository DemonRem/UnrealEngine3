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

#ifndef INCLUDED_ENGINE_FOGVOLUME_ENUMS
#define INCLUDED_ENGINE_FOGVOLUME_ENUMS 1


#endif // !INCLUDED_ENGINE_FOGVOLUME_ENUMS
#endif // !NO_ENUMS

#if !ENUMS_ONLY

#ifndef NAMES_ONLY
#define AUTOGENERATE_NAME(name) extern FName ENGINE_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name)
#endif


#ifndef NAMES_ONLY

#ifndef INCLUDED_ENGINE_FOGVOLUME_CLASSES
#define INCLUDED_ENGINE_FOGVOLUME_CLASSES 1

class AFogVolumeDensityInfo : public AInfo
{
public:
    //## BEGIN PROPS FogVolumeDensityInfo
    class UFogVolumeDensityComponent* DensityComponent;
    BITFIELD bEnabled:1;
    //## END PROPS FogVolumeDensityInfo

    DECLARE_ABSTRACT_CLASS(AFogVolumeDensityInfo,AInfo,0,Engine)
    NO_DEFAULT_CONSTRUCTOR(AFogVolumeDensityInfo)
};

class AFogVolumeConeDensityInfo : public AFogVolumeDensityInfo
{
public:
    //## BEGIN PROPS FogVolumeConeDensityInfo
    //## END PROPS FogVolumeConeDensityInfo

    DECLARE_ABSTRACT_CLASS(AFogVolumeConeDensityInfo,AFogVolumeDensityInfo,0,Engine)
	// AActor interface.
	virtual void EditorApplyScale(const FVector& DeltaScale, const FMatrix& ScaleMatrix, const FVector* PivotLocation, UBOOL bAltDown, UBOOL bShiftDown, UBOOL bCtrlDown);
};

class AFogVolumeConstantDensityInfo : public AFogVolumeDensityInfo
{
public:
    //## BEGIN PROPS FogVolumeConstantDensityInfo
    //## END PROPS FogVolumeConstantDensityInfo

    DECLARE_CLASS(AFogVolumeConstantDensityInfo,AFogVolumeDensityInfo,0,Engine)
    NO_DEFAULT_CONSTRUCTOR(AFogVolumeConstantDensityInfo)
};

class AFogVolumeConstantHeightDensityInfo : public AFogVolumeDensityInfo
{
public:
    //## BEGIN PROPS FogVolumeConstantHeightDensityInfo
    //## END PROPS FogVolumeConstantHeightDensityInfo

    DECLARE_CLASS(AFogVolumeConstantHeightDensityInfo,AFogVolumeDensityInfo,0,Engine)
    NO_DEFAULT_CONSTRUCTOR(AFogVolumeConstantHeightDensityInfo)
};

class AFogVolumeLinearHalfspaceDensityInfo : public AFogVolumeDensityInfo
{
public:
    //## BEGIN PROPS FogVolumeLinearHalfspaceDensityInfo
    //## END PROPS FogVolumeLinearHalfspaceDensityInfo

    DECLARE_CLASS(AFogVolumeLinearHalfspaceDensityInfo,AFogVolumeDensityInfo,0,Engine)
    NO_DEFAULT_CONSTRUCTOR(AFogVolumeLinearHalfspaceDensityInfo)
};

class AFogVolumeSphericalDensityInfo : public AFogVolumeDensityInfo
{
public:
    //## BEGIN PROPS FogVolumeSphericalDensityInfo
    //## END PROPS FogVolumeSphericalDensityInfo

    DECLARE_CLASS(AFogVolumeSphericalDensityInfo,AFogVolumeDensityInfo,0,Engine)
	// AActor interface.
	virtual void EditorApplyScale(const FVector& DeltaScale, const FMatrix& ScaleMatrix, const FVector* PivotLocation, UBOOL bAltDown, UBOOL bShiftDown, UBOOL bCtrlDown);
};

class UFogVolumeDensityComponent : public UActorComponent
{
public:
    //## BEGIN PROPS FogVolumeDensityComponent
    BITFIELD bEnabled:1 GCC_BITFIELD_MAGIC;
    FLinearColor ApproxFogLightColor;
    TArrayNoInit<class AActor*> FogVolumeActors;
    //## END PROPS FogVolumeDensityComponent

    void SetEnabled(UBOOL bSetEnabled);
    DECLARE_FUNCTION(execSetEnabled)
    {
        P_GET_UBOOL(bSetEnabled);
        P_FINISH;
        SetEnabled(bSetEnabled);
    }
    DECLARE_ABSTRACT_CLASS(UFogVolumeDensityComponent,UActorComponent,0,Engine)
private:
	/** Adds the fog volume components to the scene */
	void AddFogVolumeComponents();

	/** Removes the fog volume components from the scene */
	void RemoveFogVolumeComponents();

	/** 
	 * Sets up FogVolumeActors's mesh components to defaults that are common usage with fog volumes.  
	 * Collision is disabled for the actor, each component gets assigned the default fog volume material,
	 * lighting, shadowing, decal accepting, and occluding are disabled.
	 */
	void SetFogActorDefaults();

protected:
	// ActorComponent interface.
	virtual void Attach();
	virtual void UpdateTransform();
	virtual void Detach();

	// UObject interface
	virtual void PostEditChange( FEditPropertyChain& PropertyThatChanged );

public:
	// FogVolumeDensityComponent interface.
	virtual class FFogVolumeDensitySceneInfo* CreateFogVolumeDensityInfo(FBox VolumeBounds) const PURE_VIRTUAL(UFogVolumeDensityComponent::CreateFogVolumeDensityInfo,return NULL;);

	/** 
	 * Checks for partial fog volume setup that will not render anything.
	 */
	virtual void CheckForErrors();
};

class UFogVolumeConeDensityComponent : public UFogVolumeDensityComponent
{
public:
    //## BEGIN PROPS FogVolumeConeDensityComponent
    FLOAT MaxDensity;
    FVector ConeVertex;
    FLOAT ConeRadius;
    FVector ConeAxis;
    FLOAT ConeMaxAngle;
    class UDrawLightConeComponent* PreviewCone;
    //## END PROPS FogVolumeConeDensityComponent

    DECLARE_CLASS(UFogVolumeConeDensityComponent,UFogVolumeDensityComponent,0,Engine)
protected:
	// ActorComponent interface.
	virtual void SetParentToWorld(const FMatrix& ParentToWorld);
	virtual void Attach();

public:
	// FogVolumeDensityComponent interface.
	virtual class FFogVolumeDensitySceneInfo* CreateFogVolumeDensityInfo(FBox VolumeBounds) const;
};

class UFogVolumeConstantDensityComponent : public UFogVolumeDensityComponent
{
public:
    //## BEGIN PROPS FogVolumeConstantDensityComponent
    FLOAT Density;
    //## END PROPS FogVolumeConstantDensityComponent

    DECLARE_CLASS(UFogVolumeConstantDensityComponent,UFogVolumeDensityComponent,0,Engine)
public:
	// FogVolumeDensityComponent interface.
	virtual class FFogVolumeDensitySceneInfo* CreateFogVolumeDensityInfo(FBox VolumeBounds) const;
};

class UFogVolumeConstantHeightDensityComponent : public UFogVolumeDensityComponent
{
public:
    //## BEGIN PROPS FogVolumeConstantHeightDensityComponent
    FLOAT Density;
    FLOAT Height;
    //## END PROPS FogVolumeConstantHeightDensityComponent

    DECLARE_CLASS(UFogVolumeConstantHeightDensityComponent,UFogVolumeDensityComponent,0,Engine)
protected:
	// ActorComponent interface.
	virtual void SetParentToWorld(const FMatrix& ParentToWorld);

public:
	// FogVolumeDensityComponent interface.
	virtual class FFogVolumeDensitySceneInfo* CreateFogVolumeDensityInfo(FBox VolumeBounds) const;
};

class UFogVolumeLinearHalfspaceDensityComponent : public UFogVolumeDensityComponent
{
public:
    //## BEGIN PROPS FogVolumeLinearHalfspaceDensityComponent
    FLOAT PlaneDistanceFactor;
    FPlane HalfspacePlane;
    //## END PROPS FogVolumeLinearHalfspaceDensityComponent

    DECLARE_CLASS(UFogVolumeLinearHalfspaceDensityComponent,UFogVolumeDensityComponent,0,Engine)
protected:
	// ActorComponent interface.
	virtual void SetParentToWorld(const FMatrix& ParentToWorld);

public:
	// FogVolumeDensityComponent interface.
	virtual class FFogVolumeDensitySceneInfo* CreateFogVolumeDensityInfo(FBox VolumeBounds) const;
};

class UFogVolumeSphericalDensityComponent : public UFogVolumeDensityComponent
{
public:
    //## BEGIN PROPS FogVolumeSphericalDensityComponent
    FLOAT MaxDensity;
    FVector SphereCenter;
    FLOAT SphereRadius;
    class UDrawLightRadiusComponent* PreviewSphereRadius;
    //## END PROPS FogVolumeSphericalDensityComponent

    DECLARE_CLASS(UFogVolumeSphericalDensityComponent,UFogVolumeDensityComponent,0,Engine)
protected:
	// ActorComponent interface.
	virtual void SetParentToWorld(const FMatrix& ParentToWorld);
	virtual void Attach();

public:
	// FogVolumeDensityComponent interface.
	virtual class FFogVolumeDensitySceneInfo* CreateFogVolumeDensityInfo(FBox VolumeBounds) const;
};

#endif // !INCLUDED_ENGINE_FOGVOLUME_CLASSES
#endif // !NAMES_ONLY

AUTOGENERATE_FUNCTION(UFogVolumeDensityComponent,-1,execSetEnabled);

#ifndef NAMES_ONLY
#undef AUTOGENERATE_NAME
#undef AUTOGENERATE_FUNCTION
#endif

#ifdef STATIC_LINKING_MOJO
#ifndef ENGINE_FOGVOLUME_NATIVE_DEFS
#define ENGINE_FOGVOLUME_NATIVE_DEFS

DECLARE_NATIVE_TYPE(Engine,UFogVolumeConeDensityComponent);
DECLARE_NATIVE_TYPE(Engine,AFogVolumeConeDensityInfo);
DECLARE_NATIVE_TYPE(Engine,UFogVolumeConstantDensityComponent);
DECLARE_NATIVE_TYPE(Engine,AFogVolumeConstantDensityInfo);
DECLARE_NATIVE_TYPE(Engine,UFogVolumeConstantHeightDensityComponent);
DECLARE_NATIVE_TYPE(Engine,AFogVolumeConstantHeightDensityInfo);
DECLARE_NATIVE_TYPE(Engine,UFogVolumeDensityComponent);
DECLARE_NATIVE_TYPE(Engine,AFogVolumeDensityInfo);
DECLARE_NATIVE_TYPE(Engine,UFogVolumeLinearHalfspaceDensityComponent);
DECLARE_NATIVE_TYPE(Engine,AFogVolumeLinearHalfspaceDensityInfo);
DECLARE_NATIVE_TYPE(Engine,UFogVolumeSphericalDensityComponent);
DECLARE_NATIVE_TYPE(Engine,AFogVolumeSphericalDensityInfo);

#define AUTO_INITIALIZE_REGISTRANTS_ENGINE_FOGVOLUME \
	UFogVolumeConeDensityComponent::StaticClass(); \
	AFogVolumeConeDensityInfo::StaticClass(); \
	UFogVolumeConstantDensityComponent::StaticClass(); \
	AFogVolumeConstantDensityInfo::StaticClass(); \
	UFogVolumeConstantHeightDensityComponent::StaticClass(); \
	AFogVolumeConstantHeightDensityInfo::StaticClass(); \
	UFogVolumeDensityComponent::StaticClass(); \
	GNativeLookupFuncs[Lookup++] = &FindEngineUFogVolumeDensityComponentNative; \
	AFogVolumeDensityInfo::StaticClass(); \
	UFogVolumeLinearHalfspaceDensityComponent::StaticClass(); \
	AFogVolumeLinearHalfspaceDensityInfo::StaticClass(); \
	UFogVolumeSphericalDensityComponent::StaticClass(); \
	AFogVolumeSphericalDensityInfo::StaticClass(); \

#endif // ENGINE_FOGVOLUME_NATIVE_DEFS

#ifdef NATIVES_ONLY
NATIVE_INFO(UFogVolumeDensityComponent) GEngineUFogVolumeDensityComponentNatives[] = 
{ 
	MAP_NATIVE(UFogVolumeDensityComponent,execSetEnabled)
	{NULL,NULL}
};
IMPLEMENT_NATIVE_HANDLER(Engine,UFogVolumeDensityComponent);

#endif // NATIVES_ONLY
#endif // STATIC_LINKING_MOJO

#ifdef VERIFY_CLASS_SIZES
VERIFY_CLASS_OFFSET_NODIE(U,FogVolumeConeDensityComponent,MaxDensity)
VERIFY_CLASS_OFFSET_NODIE(U,FogVolumeConeDensityComponent,PreviewCone)
VERIFY_CLASS_SIZE_NODIE(UFogVolumeConeDensityComponent)
VERIFY_CLASS_SIZE_NODIE(AFogVolumeConeDensityInfo)
VERIFY_CLASS_OFFSET_NODIE(U,FogVolumeConstantDensityComponent,Density)
VERIFY_CLASS_SIZE_NODIE(UFogVolumeConstantDensityComponent)
VERIFY_CLASS_SIZE_NODIE(AFogVolumeConstantDensityInfo)
VERIFY_CLASS_OFFSET_NODIE(U,FogVolumeConstantHeightDensityComponent,Density)
VERIFY_CLASS_OFFSET_NODIE(U,FogVolumeConstantHeightDensityComponent,Height)
VERIFY_CLASS_SIZE_NODIE(UFogVolumeConstantHeightDensityComponent)
VERIFY_CLASS_SIZE_NODIE(AFogVolumeConstantHeightDensityInfo)
VERIFY_CLASS_OFFSET_NODIE(U,FogVolumeDensityComponent,ApproxFogLightColor)
VERIFY_CLASS_OFFSET_NODIE(U,FogVolumeDensityComponent,FogVolumeActors)
VERIFY_CLASS_SIZE_NODIE(UFogVolumeDensityComponent)
VERIFY_CLASS_OFFSET_NODIE(A,FogVolumeDensityInfo,DensityComponent)
VERIFY_CLASS_SIZE_NODIE(AFogVolumeDensityInfo)
VERIFY_CLASS_OFFSET_NODIE(U,FogVolumeLinearHalfspaceDensityComponent,PlaneDistanceFactor)
VERIFY_CLASS_OFFSET_NODIE(U,FogVolumeLinearHalfspaceDensityComponent,HalfspacePlane)
VERIFY_CLASS_SIZE_NODIE(UFogVolumeLinearHalfspaceDensityComponent)
VERIFY_CLASS_SIZE_NODIE(AFogVolumeLinearHalfspaceDensityInfo)
VERIFY_CLASS_OFFSET_NODIE(U,FogVolumeSphericalDensityComponent,MaxDensity)
VERIFY_CLASS_OFFSET_NODIE(U,FogVolumeSphericalDensityComponent,PreviewSphereRadius)
VERIFY_CLASS_SIZE_NODIE(UFogVolumeSphericalDensityComponent)
VERIFY_CLASS_SIZE_NODIE(AFogVolumeSphericalDensityInfo)
#endif // VERIFY_CLASS_SIZES
#endif // !ENUMS_ONLY

#if SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif