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

#ifndef INCLUDED_ENGINE_DECAL_ENUMS
#define INCLUDED_ENGINE_DECAL_ENUMS 1

enum EFilterMode
{
    FM_None                 =0,
    FM_Ignore               =1,
    FM_Affect               =2,
    FM_MAX                  =3,
};

#endif // !INCLUDED_ENGINE_DECAL_ENUMS
#endif // !NO_ENUMS

#if !ENUMS_ONLY

#ifndef NAMES_ONLY
#define AUTOGENERATE_NAME(name) extern FName ENGINE_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name)
#endif


#ifndef NAMES_ONLY

#ifndef INCLUDED_ENGINE_DECAL_CLASSES
#define INCLUDED_ENGINE_DECAL_CLASSES 1

class ADecalActor : public AActor
{
public:
    //## BEGIN PROPS DecalActor
    class UDecalComponent* Decal;
    //## END PROPS DecalActor

    DECLARE_CLASS(ADecalActor,AActor,0,Engine)
	// UObject interface.
	virtual void PostEditChange(UProperty* PropertyThatChanged);

	// AActor interface.
	virtual void PostEditImport();
	virtual void PostEditMove(UBOOL bFinished);
	virtual void EditorApplyScale(const FVector& DeltaScale, const FMatrix& ScaleMatrix, const FVector* PivotLocation, UBOOL bAltDown, UBOOL bShiftDown, UBOOL bCtrlDown);

	// AActor interface.
	/**
	 * Function that gets called from within Map_Check to allow this actor to check itself
	 * for any potential errors and register them with map check dialog.
	 */
	virtual void CheckForErrors();
};

class UActorFactoryDecal : public UActorFactory
{
public:
    //## BEGIN PROPS ActorFactoryDecal
    class UMaterialInterface* DecalMaterial;
    //## END PROPS ActorFactoryDecal

    DECLARE_CLASS(UActorFactoryDecal,UActorFactory,0|CLASS_Config,Engine)
	virtual AActor* CreateActor( const FVector* const Location, const FRotator* const Rotation, const class USeqAct_ActorFactory* const ActorFactoryData );
	virtual UBOOL CanCreateActor(FString& OutErrorMsg);
	virtual void AutoFillFields(class USelection* Selection);
	virtual FString GetMenuName();
};

struct FDecalReceiver
{
    class UPrimitiveComponent* Component;
    class FDecalRenderData* RenderData;
};

class UDecalComponent : public UPrimitiveComponent
{
public:
    //## BEGIN PROPS DecalComponent
    class UMaterialInterface* DecalMaterial;
    FLOAT Width;
    FLOAT Height;
    FLOAT TileX;
    FLOAT TileY;
    FLOAT OffsetX;
    FLOAT OffsetY;
    FLOAT DecalRotation;
    FLOAT Thickness;
    FLOAT FieldOfView;
    FLOAT NearPlane;
    FLOAT FarPlane;
    FVector Location;
    FRotator Orientation;
    FVector HitLocation;
    FVector HitNormal;
    FVector HitTangent;
    FVector HitBinormal;
    BITFIELD bNoClip:1;
    BITFIELD bStaticDecal:1;
    BITFIELD bProjectOnBackfaces:1;
    BITFIELD bProjectOnHidden:1;
    BITFIELD bProjectOnBSP:1;
    BITFIELD bProjectOnStaticMeshes:1;
    BITFIELD bProjectOnSkeletalMeshes:1;
    BITFIELD bProjectOnTerrain:1;
    BITFIELD bFlipBackfaceDirection:1;
    class UPrimitiveComponent* HitComponent;
    FName HitBone;
    INT HitNodeIndex;
    INT HitLevelIndex;
    TArrayNoInit<INT> HitNodeIndices;
    class UDecalLifetimeData* LifetimeData;
    TArrayNoInit<struct FDecalReceiver> DecalReceivers;
    TArrayNoInit<class FStaticReceiverData*> StaticReceivers;
    TArrayNoInit<class FDecalRenderData*> RenderData;
    FRenderCommandFence* ReleaseResourcesFence;
    TArrayNoInit<FPlane> Planes;
    FLOAT DepthBias;
    FLOAT SlopeScaleDepthBias;
    INT SortOrder;
    FLOAT BackfaceAngle;
    BYTE FilterMode;
    TArrayNoInit<class AActor*> Filter;
    TArrayNoInit<class UPrimitiveComponent*> ReceiverImages;
    //## END PROPS DecalComponent

    virtual class UDecalManager* GetManager();
    virtual void ConnectToManager();
    virtual void DisconnectFromManager();
    DECLARE_FUNCTION(execGetManager)
    {
        P_FINISH;
        *(class UDecalManager**)Result=GetManager();
    }
    DECLARE_FUNCTION(execConnectToManager)
    {
        P_FINISH;
        ConnectToManager();
    }
    DECLARE_FUNCTION(execDisconnectFromManager)
    {
        P_FINISH;
        DisconnectFromManager();
    }
    DECLARE_CLASS(UDecalComponent,UPrimitiveComponent,0,Engine)
public:
	/**
	 * Builds orthogonal planes from HitLocation, HitNormal, Width, Height and Depth.
	 */
	void UpdateOrthoPlanes();

	/**
	 * @return	TRUE if the decal is enabled as specified in GEngine, FALSE otherwise.
	 */
	UBOOL IsEnabled() const;

	/**
	 * Allocates a sort key to the decal if the decal is non-static.
	 */
	void AllocateSortKey();

	/**
	 * Fills in the specified vertex list with the local-space decal frustum vertices.
	 */
	void GenerateDecalFrustumVerts(FVector Verts[8]) const;

	/**
	 * Fills in the specified decal state object with this decal's state.
	 */
	void CaptureDecalState(class FDecalState* DecalState) const;

	void AttachReceiver(UPrimitiveComponent* Receiver);

	/**
	 * Updates ortho planes, computes the receiver set, and connects to a decal manager.
	 */
	void ComputeReceivers();

	/**
	 * Attaches the decal to static receivers, according to the data stored in StaticReceivers.
	 */
	void AttachToStaticReceivers();

	/**
	 * Disconnects the decal from its manager and detaches receivers.
	 */
	void DetachFromReceivers();

	/**
	 * @return		TRUE if the application filter passes the specified component, FALSE otherwise.
	 */
	UBOOL FilterComponent(UPrimitiveComponent* Component) const;

	/**
	 * Frees any StaticReceivers data.
	 */
	void FreeStaticReceivers();

	// PrimitiveComponent interface.
	/**
	 * Sets the component's bounds based on the vertices of the decal frustum.
	 */
	virtual void UpdateBounds();

	/**
	 * Updates all associated lightmaps with the new value for allowing directional lightmaps.
	 * This is only called when switching lightmap modes in-game and should never be called on cooked builds.
	 */
	virtual void UpdateLightmapType(UBOOL bAllowDirectionalLightMaps);

	/**
	 * Creates a FPrimitiveSceneProxy proxy to implement debug rendering of the decal in the rendering thread.
	 *
	 * @return		A heap-allocated proxy rendering object corresponding to this decal component.
	 */
	virtual FPrimitiveSceneProxy* CreateSceneProxy();

	/**
	 * @return		TRUE if both IsEnabled() and Super::IsValidComponent() return TRUE.
	 */
	virtual UBOOL IsValidComponent() const;

	// ActorComponent interface.
	virtual void CheckForErrors();

	// UObject interface;
	virtual void BeginPlay();
	virtual void BeginDestroy();
	virtual UBOOL IsReadyForFinishDestroy();
	virtual void FinishDestroy();
	virtual void PreSave();
	virtual void Serialize(FArchive& Ar);

protected:
	void ReleaseResources(UBOOL bBlockOnRelease);

	// ActorComponent interface.
	virtual void Attach();
	virtual void UpdateTransform();
	virtual void Detach();
};

class UDecalLifetime : public UObject
{
public:
    //## BEGIN PROPS DecalLifetime
    FStringNoInit PolicyName;
    TArrayNoInit<class UDecalComponent*> ManagedDecals;
    //## END PROPS DecalLifetime

    DECLARE_ABSTRACT_CLASS(UDecalLifetime,UObject,0,Engine)
public:
	/**
	 * Called by UDecalManager::Tick.
	 */
	virtual void Tick(FLOAT DeltaSeconds);

	/**
	 * Instructs this policy to manage the specified decal.  Called by UDecalManager::AddDynamicDecal.
	 */
	void AddDecal(UDecalComponent* InDecalComponent)
	{
		// Invariant: This lifetime policy doesn't already manage this decal.
		ManagedDecals.AddItem( InDecalComponent );
	}

	/**
	 * Instructs this policy to stop managing the specified decal.  Called by UDecalManager::RemoveDynamicDecal.
	 */
	void RemoveDecal(UDecalComponent* InDecalComponent)
	{
		ManagedDecals.RemoveItem( InDecalComponent );
	}

public:
	/**
	 * Helper function for derived classes that enacpsulates decal detachment, owner components array clearing, etc.
	 */
	void EliminateDecal(UDecalComponent* InDecalComponent);
};

class UDecalLifetimeAge : public UDecalLifetime
{
public:
    //## BEGIN PROPS DecalLifetimeAge
    //## END PROPS DecalLifetimeAge

    DECLARE_CLASS(UDecalLifetimeAge,UDecalLifetime,0,Engine)
public:
	/**
	 * Called by UDecalManager::Tick.
	 */
	virtual void Tick(FLOAT DeltaSeconds);
};

class UDecalLifetimeData : public UObject
{
public:
    //## BEGIN PROPS DecalLifetimeData
    FStringNoInit LifetimePolicyName;
    //## END PROPS DecalLifetimeData

    DECLARE_CLASS(UDecalLifetimeData,UObject,0,Engine)
    NO_DEFAULT_CONSTRUCTOR(UDecalLifetimeData)
};

class UDecalLifetimeDataAge : public UDecalLifetimeData
{
public:
    //## BEGIN PROPS DecalLifetimeDataAge
    FLOAT Age;
    FLOAT LifeSpan;
    //## END PROPS DecalLifetimeDataAge

    DECLARE_CLASS(UDecalLifetimeDataAge,UDecalLifetimeData,0,Engine)
    NO_DEFAULT_CONSTRUCTOR(UDecalLifetimeDataAge)
};

class UDecalManager : public UObject
{
public:
    //## BEGIN PROPS DecalManager
    TArrayNoInit<class UDecalLifetime*> LifetimePolicies;
    //## END PROPS DecalManager

    virtual void Connect(class UDecalComponent* InDecalComponent);
    virtual void Disconnect(class UDecalComponent* InDecalComponent);
    DECLARE_FUNCTION(execConnect)
    {
        P_GET_OBJECT(UDecalComponent,InDecalComponent);
        P_FINISH;
        Connect(InDecalComponent);
    }
    DECLARE_FUNCTION(execDisconnect)
    {
        P_GET_OBJECT(UDecalComponent,InDecalComponent);
        P_FINISH;
        Disconnect(InDecalComponent);
    }
    DECLARE_CLASS(UDecalManager,UObject,0,Engine)
public:
	/**
	 * Initializes the LifetimePolicies array with instances of all classes that derive from UDecalLifetime.
	 */
	void Init();

	/**
	 * Updates the manager's lifetime policies array with any policies that were added since the manager was first created.
	 */
	void UpdatePolicies();

	/**
	 * Ticks all policies in the LifetimePolicies array.
	 */
	void Tick(FLOAT DeltaSeconds);
	
	/**
	 * Returns the named lifetime policy, or NULL if no lifetime policy with that name exists in LifetimePolicies.
	 */
	UDecalLifetime* GetPolicy(const FString& PolicyName) const;
};

class UDecalMaterial : public UMaterial
{
public:
    //## BEGIN PROPS DecalMaterial
    //## END PROPS DecalMaterial

    DECLARE_CLASS(UDecalMaterial,UMaterial,0,Engine)
	// UMaterial interface.
	virtual FMaterialResource* AllocateResource();

	// UObject interface.
	virtual void PostEditChange(UProperty* PropertyThatChanged);
	virtual void PreSave();
	virtual void PostLoad();
};

#endif // !INCLUDED_ENGINE_DECAL_CLASSES
#endif // !NAMES_ONLY

AUTOGENERATE_FUNCTION(UDecalComponent,-1,execDisconnectFromManager);
AUTOGENERATE_FUNCTION(UDecalComponent,-1,execConnectToManager);
AUTOGENERATE_FUNCTION(UDecalComponent,-1,execGetManager);
AUTOGENERATE_FUNCTION(UDecalManager,-1,execDisconnect);
AUTOGENERATE_FUNCTION(UDecalManager,-1,execConnect);

#ifndef NAMES_ONLY
#undef AUTOGENERATE_NAME
#undef AUTOGENERATE_FUNCTION
#endif

#ifdef STATIC_LINKING_MOJO
#ifndef ENGINE_DECAL_NATIVE_DEFS
#define ENGINE_DECAL_NATIVE_DEFS

DECLARE_NATIVE_TYPE(Engine,UActorFactoryDecal);
DECLARE_NATIVE_TYPE(Engine,ADecalActor);
DECLARE_NATIVE_TYPE(Engine,UDecalComponent);
DECLARE_NATIVE_TYPE(Engine,UDecalLifetime);
DECLARE_NATIVE_TYPE(Engine,UDecalLifetimeAge);
DECLARE_NATIVE_TYPE(Engine,UDecalLifetimeData);
DECLARE_NATIVE_TYPE(Engine,UDecalLifetimeDataAge);
DECLARE_NATIVE_TYPE(Engine,UDecalManager);
DECLARE_NATIVE_TYPE(Engine,UDecalMaterial);

#define AUTO_INITIALIZE_REGISTRANTS_ENGINE_DECAL \
	UActorFactoryDecal::StaticClass(); \
	ADecalActor::StaticClass(); \
	UDecalComponent::StaticClass(); \
	GNativeLookupFuncs[Lookup++] = &FindEngineUDecalComponentNative; \
	UDecalLifetime::StaticClass(); \
	UDecalLifetimeAge::StaticClass(); \
	UDecalLifetimeData::StaticClass(); \
	UDecalLifetimeDataAge::StaticClass(); \
	UDecalManager::StaticClass(); \
	GNativeLookupFuncs[Lookup++] = &FindEngineUDecalManagerNative; \
	UDecalMaterial::StaticClass(); \

#endif // ENGINE_DECAL_NATIVE_DEFS

#ifdef NATIVES_ONLY
NATIVE_INFO(UDecalComponent) GEngineUDecalComponentNatives[] = 
{ 
	MAP_NATIVE(UDecalComponent,execDisconnectFromManager)
	MAP_NATIVE(UDecalComponent,execConnectToManager)
	MAP_NATIVE(UDecalComponent,execGetManager)
	{NULL,NULL}
};
IMPLEMENT_NATIVE_HANDLER(Engine,UDecalComponent);

NATIVE_INFO(UDecalManager) GEngineUDecalManagerNatives[] = 
{ 
	MAP_NATIVE(UDecalManager,execDisconnect)
	MAP_NATIVE(UDecalManager,execConnect)
	{NULL,NULL}
};
IMPLEMENT_NATIVE_HANDLER(Engine,UDecalManager);

#endif // NATIVES_ONLY
#endif // STATIC_LINKING_MOJO

#ifdef VERIFY_CLASS_SIZES
VERIFY_CLASS_OFFSET_NODIE(U,ActorFactoryDecal,DecalMaterial)
VERIFY_CLASS_SIZE_NODIE(UActorFactoryDecal)
VERIFY_CLASS_OFFSET_NODIE(A,DecalActor,Decal)
VERIFY_CLASS_SIZE_NODIE(ADecalActor)
VERIFY_CLASS_OFFSET_NODIE(U,DecalComponent,DecalMaterial)
VERIFY_CLASS_OFFSET_NODIE(U,DecalComponent,ReceiverImages)
VERIFY_CLASS_SIZE_NODIE(UDecalComponent)
VERIFY_CLASS_OFFSET_NODIE(U,DecalLifetime,PolicyName)
VERIFY_CLASS_OFFSET_NODIE(U,DecalLifetime,ManagedDecals)
VERIFY_CLASS_SIZE_NODIE(UDecalLifetime)
VERIFY_CLASS_SIZE_NODIE(UDecalLifetimeAge)
VERIFY_CLASS_OFFSET_NODIE(U,DecalLifetimeData,LifetimePolicyName)
VERIFY_CLASS_SIZE_NODIE(UDecalLifetimeData)
VERIFY_CLASS_OFFSET_NODIE(U,DecalLifetimeDataAge,Age)
VERIFY_CLASS_OFFSET_NODIE(U,DecalLifetimeDataAge,LifeSpan)
VERIFY_CLASS_SIZE_NODIE(UDecalLifetimeDataAge)
VERIFY_CLASS_OFFSET_NODIE(U,DecalManager,LifetimePolicies)
VERIFY_CLASS_SIZE_NODIE(UDecalManager)
VERIFY_CLASS_SIZE_NODIE(UDecalMaterial)
#endif // VERIFY_CLASS_SIZES
#endif // !ENUMS_ONLY

#if SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif
