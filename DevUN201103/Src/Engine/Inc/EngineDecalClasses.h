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

#ifndef INCLUDED_ENGINE_DECAL_ENUMS
#define INCLUDED_ENGINE_DECAL_ENUMS 1

enum EFilterMode
{
    FM_None                 =0,
    FM_Ignore               =1,
    FM_Affect               =2,
    FM_MAX                  =3,
};
#define FOREACH_ENUM_EFILTERMODE(op) \
    op(FM_None) \
    op(FM_Ignore) \
    op(FM_Affect) 
enum EDecalTransform
{
    DecalTransform_OwnerAbsolute=0,
    DecalTransform_OwnerRelative=1,
    DecalTransform_SpawnRelative=2,
    DecalTransform_MAX      =3,
};
#define FOREACH_ENUM_EDECALTRANSFORM(op) \
    op(DecalTransform_OwnerAbsolute) \
    op(DecalTransform_OwnerRelative) \
    op(DecalTransform_SpawnRelative) 

#endif // !INCLUDED_ENGINE_DECAL_ENUMS
#endif // !NO_ENUMS

#if !ENUMS_ONLY

#ifndef NAMES_ONLY
#define AUTOGENERATE_FUNCTION(cls,idx,name)
#endif


#ifndef NAMES_ONLY

#ifndef INCLUDED_ENGINE_DECAL_CLASSES
#define INCLUDED_ENGINE_DECAL_CLASSES 1
#define ENABLE_DECLARECLASS_MACRO 1
#include "UnObjBas.h"
#undef ENABLE_DECLARECLASS_MACRO

class ADecalActorBase : public AActor
{
public:
    //## BEGIN PROPS DecalActorBase
    class UDecalComponent* Decal;
    //## END PROPS DecalActorBase

    DECLARE_ABSTRACT_CLASS(ADecalActorBase,AActor,0,Engine)
	// UObject interface.
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);

	// AActor interface.
	virtual void PostEditMove(UBOOL bFinished);

	/**
	 * Function that gets called from within Map_Check to allow this actor to check itself
	 * for any potential errors and register them with map check dialog.
	 */
#if WITH_EDITOR
	virtual void EditorApplyScale(const FVector& DeltaScale, const FMatrix& ScaleMatrix, const FVector* PivotLocation, UBOOL bAltDown, UBOOL bShiftDown, UBOOL bCtrlDown);
	virtual void CheckForErrors();
#endif
};

class ADecalActor : public ADecalActorBase
{
public:
    //## BEGIN PROPS DecalActor
    //## END PROPS DecalActor

    DECLARE_CLASS(ADecalActor,ADecalActorBase,0,Engine)
    NO_DEFAULT_CONSTRUCTOR(ADecalActor)
};

class ADecalActorMovable : public ADecalActorBase
{
public:
    //## BEGIN PROPS DecalActorMovable
    //## END PROPS DecalActorMovable

    DECLARE_CLASS(ADecalActorMovable,ADecalActorBase,0,Engine)
    NO_DEFAULT_CONSTRUCTOR(ADecalActorMovable)
};

struct FActiveDecalInfo
{
    class UDecalComponent* Decal;
    FLOAT LifetimeRemaining;

    /** Constructors */
    FActiveDecalInfo() {}
    FActiveDecalInfo(EEventParm)
    {
        appMemzero(this, sizeof(FActiveDecalInfo));
    }
};

struct DecalManager_eventDecalFinished_Parms
{
    class UDecalComponent* Decal;
    DecalManager_eventDecalFinished_Parms(EEventParm)
    {
    }
};
class ADecalManager : public AActor
{
public:
    //## BEGIN PROPS DecalManager
    class UDecalComponent* DecalTemplate;
    TArrayNoInit<class UDecalComponent*> PoolDecals;
    INT MaxActiveDecals;
    FLOAT DecalLifeSpan;
    FLOAT DecalDepthBias;
    FVector2D DecalBlendRange;
    TArrayNoInit<struct FActiveDecalInfo> ActiveDecals;
    //## END PROPS DecalManager

    UBOOL AreDynamicDecalsEnabled();
    DECLARE_FUNCTION(execAreDynamicDecalsEnabled)
    {
        P_FINISH;
        *(UBOOL*)Result=this->AreDynamicDecalsEnabled();
    }
    void eventDecalFinished(class UDecalComponent* Decal)
    {
        DecalManager_eventDecalFinished_Parms Parms(EC_EventParm);
        Parms.Decal=Decal;
        ProcessEvent(FindFunctionChecked(ENGINE_DecalFinished),&Parms);
    }
    DECLARE_CLASS(ADecalManager,AActor,0|CLASS_Config,Engine)
    static const TCHAR* StaticConfigName() {return TEXT("Game");}

	virtual void TickSpecial(FLOAT DeltaTime);
};

struct FDecalReceiver
{
    class UPrimitiveComponent* Component;
    class FDecalRenderData* RenderData;

    /** Constructors */
    FDecalReceiver() {}
    FDecalReceiver(EEventParm)
    {
        appMemzero(this, sizeof(FDecalReceiver));
    }
};

class UDecalComponent : public UPrimitiveComponent
{
public:
    //## BEGIN PROPS DecalComponent
private:
    class UMaterialInterface* DecalMaterial;
public:
    FLOAT Width;
    FLOAT Height;
    FLOAT TileX;
    FLOAT TileY;
    FLOAT OffsetX;
    FLOAT OffsetY;
    FLOAT DecalRotation;
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
    BITFIELD bMovableDecal:1;
    BITFIELD bHasBeenAttached:1;
    class UPrimitiveComponent* HitComponent;
    FName HitBone;
    INT HitNodeIndex;
    INT HitLevelIndex;
    INT FracturedStaticMeshComponentIndex;
    TArrayNoInit<INT> HitNodeIndices;
    TArrayNoInit<struct FDecalReceiver> DecalReceivers;
    TArrayNoInit<class FStaticReceiverData*> StaticReceivers;
    FRenderCommandFence* ReleaseResourcesFence;
    TArrayNoInit<FPlane> Planes;
    FLOAT DepthBias;
    FLOAT SlopeScaleDepthBias;
    INT SortOrder;
    FLOAT BackfaceAngle;
    FVector2D BlendRange;
    BYTE DecalTransform;
    BYTE FilterMode;
    TArrayNoInit<class AActor*> Filter;
    TArrayNoInit<class UPrimitiveComponent*> ReceiverImages;
    FVector ParentRelativeLocation;
    FRotator ParentRelativeOrientation;
    FVector OriginalParentRelativeLocation;
    FVector OriginalParentRelativeOrientationVec;
    //## END PROPS DecalComponent

    void ResetToDefaults();
    void SetDecalMaterial(class UMaterialInterface* NewDecalMaterial);
    class UMaterialInterface* GetDecalMaterial() const;
    DECLARE_FUNCTION(execResetToDefaults)
    {
        P_FINISH;
        this->ResetToDefaults();
    }
    DECLARE_FUNCTION(execSetDecalMaterial)
    {
        P_GET_OBJECT(UMaterialInterface,NewDecalMaterial);
        P_FINISH;
        this->SetDecalMaterial(NewDecalMaterial);
    }
    DECLARE_FUNCTION(execGetDecalMaterial)
    {
        P_FINISH;
        *(class UMaterialInterface**)Result=this->GetDecalMaterial();
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
	 * If the decal is non-static, allocates a sort key and, if the decal is lit, a random
	 * depth bias to the decal.
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
	 * Disconnects the decal from its manager and detaches receivers matching the given primitive component.
	 * This will also release the resources associated with the receiver component.
	 *
	 * @param Receiver	The receiving primitive to detach decal render data for
	 */
	void DetachFromReceiver(UPrimitiveComponent* Receiver);

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
	 * Retrieves the materials used in this component
	 *
	 * @param OutMaterials	The list of used materials.
	 */
	virtual void GetUsedMaterials( TArray<UMaterialInterface*>& OutMaterials ) const;

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

	/**
	 * Determines whether the proxy for this primitive type needs to be recreated whenever the primitive moves.
	 * @return TRUE to recreate the proxy when UpdateTransform is called.
	 */
	virtual UBOOL ShouldRecreateProxyOnUpdateTransform() const;

	/**
	* convert interval specified in degrees to clamped dot product range
	* @return min,max range of blending decal
	*/
	virtual FVector2D CalcDecalDotProductBlendRange() const;

	virtual INT GetNumElements() const
	{
		return 1; // DecalMaterial
	}
	virtual UMaterialInterface* GetElementMaterial(INT ElementIndex) const
	{
		return (ElementIndex == 0) ? DecalMaterial : NULL;
	}
	virtual void SetElementMaterial(INT ElementIndex, UMaterialInterface* InMaterial)
	{
		if (ElementIndex == 0)
		{
			SetDecalMaterial(InMaterial);
		}
	}

	// ActorComponent interface.
#if WITH_EDITOR
	virtual void CheckForErrors();
#endif

	// UObject interface;
	virtual void BeginPlay();
	virtual void BeginDestroy();
	virtual UBOOL IsReadyForFinishDestroy();
	virtual void FinishDestroy();
	virtual void PreSave();
	virtual void Serialize(FArchive& Ar);
	virtual	INT	GetResourceSize(void);
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);
	virtual void AddReferencedObjects(TArray<UObject*>& ObjectArray);

protected:
	void ReleaseResources(UBOOL bBlockOnRelease, UPrimitiveComponent* Receiver=NULL);

	// ActorComponent interface.
	virtual void Attach();
	virtual void SetParentToWorld(const FMatrix& ParentToWorld);
	virtual void UpdateTransform();
	virtual void Detach( UBOOL bWillReattach = FALSE );
};

class UActorFactoryDecal : public UActorFactory
{
public:
    //## BEGIN PROPS ActorFactoryDecal
    class UMaterialInterface* DecalMaterial;
    //## END PROPS ActorFactoryDecal

    DECLARE_CLASS(UActorFactoryDecal,UActorFactory,0|CLASS_Config,Engine)
	/**
	 * Called to create an actor at the supplied location/rotation
	 *
	 * @param	Location			Location to create the actor at
	 * @param	Rotation			Rotation to create the actor with
	 * @param	ActorFactoryData	Kismet object which spawns actors, could potentially have settings to use/override
	 *
	 * @return	The newly created actor, NULL if it could not be created
	 */
	virtual AActor* CreateActor( const FVector* const Location, const FRotator* const Rotation, const class USeqAct_ActorFactory* const ActorFactoryData );
	
	/**
	 * If the ActorFactory thinks it could create an Actor with the current settings.
	 * Can Used to determine if we should add to context menu or if the factory can be used for drag and drop.
	 *
	 * @param	OutErrorMsg		Receives localized error string name if returning FALSE.
	 * @param	bFromAssetOnly	If true, the actor factory will check that a valid asset has been assigned from selection.  If the factory always requires an asset to be selected, this param does not matter
	 * @return	True if the actor can be created with this factory
	 */
	virtual UBOOL CanCreateActor(FString& OutErrorMsg, UBOOL bFromAssetOnly = FALSE);
	
	/**
	 * Fill the data fields of this actor with the current selection
	 *
	 * @param	Selection	Selection to use to fill this actor's data fields with
	 */
	virtual void AutoFillFields(class USelection* Selection);
	
	/**
	 * Returns the name this factory should show up as in a context-sensitive menu
	 *
	 * @return	Name this factory should show up as in a menu
	 */
	virtual FString GetMenuName();

	/**
	 * Clears references to resources [usually set by the call to AutoFillFields] when the factory has done its work.  The default behavior
	 * (which is to call AutoFillFields() with an empty selection set) should be sufficient for most factories, but this method is provided
	 * to allow customized behavior.
	 */
	virtual void ClearFields();
};

class UActorFactoryDecalMovable : public UActorFactoryDecal
{
public:
    //## BEGIN PROPS ActorFactoryDecalMovable
    //## END PROPS ActorFactoryDecalMovable

    DECLARE_CLASS(UActorFactoryDecalMovable,UActorFactoryDecal,0|CLASS_Config,Engine)
    NO_DEFAULT_CONSTRUCTOR(UActorFactoryDecalMovable)
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
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);
	virtual void PreSave();
	virtual void PostLoad();
	virtual void Serialize(FArchive& Ar);
};

#undef DECLARE_CLASS
#undef DECLARE_CASTED_CLASS
#undef DECLARE_ABSTRACT_CLASS
#undef DECLARE_ABSTRACT_CASTED_CLASS
#endif // !INCLUDED_ENGINE_DECAL_CLASSES
#endif // !NAMES_ONLY

AUTOGENERATE_FUNCTION(ADecalManager,-1,execAreDynamicDecalsEnabled);
AUTOGENERATE_FUNCTION(UDecalComponent,-1,execGetDecalMaterial);
AUTOGENERATE_FUNCTION(UDecalComponent,-1,execSetDecalMaterial);
AUTOGENERATE_FUNCTION(UDecalComponent,-1,execResetToDefaults);

#ifndef NAMES_ONLY
#undef AUTOGENERATE_FUNCTION
#endif

#ifdef STATIC_LINKING_MOJO
#ifndef ENGINE_DECAL_NATIVE_DEFS
#define ENGINE_DECAL_NATIVE_DEFS

#define AUTO_INITIALIZE_REGISTRANTS_ENGINE_DECAL \
	ADecalActorBase::StaticClass(); \
	ADecalActor::StaticClass(); \
	ADecalActorMovable::StaticClass(); \
	ADecalManager::StaticClass(); \
	GNativeLookupFuncs.Set(FName("DecalManager"), GEngineADecalManagerNatives); \
	UDecalComponent::StaticClass(); \
	GNativeLookupFuncs.Set(FName("DecalComponent"), GEngineUDecalComponentNatives); \
	UActorFactoryDecal::StaticClass(); \
	UActorFactoryDecalMovable::StaticClass(); \
	UDecalMaterial::StaticClass(); \

#endif // ENGINE_DECAL_NATIVE_DEFS

#ifdef NATIVES_ONLY
FNativeFunctionLookup GEngineADecalManagerNatives[] = 
{ 
	MAP_NATIVE(ADecalManager, execAreDynamicDecalsEnabled)
	{NULL, NULL}
};

FNativeFunctionLookup GEngineUDecalComponentNatives[] = 
{ 
	MAP_NATIVE(UDecalComponent, execGetDecalMaterial)
	MAP_NATIVE(UDecalComponent, execSetDecalMaterial)
	MAP_NATIVE(UDecalComponent, execResetToDefaults)
	{NULL, NULL}
};

#endif // NATIVES_ONLY
#endif // STATIC_LINKING_MOJO

#ifdef VERIFY_CLASS_SIZES
VERIFY_CLASS_OFFSET_NODIE(ADecalActorBase,DecalActorBase,Decal)
VERIFY_CLASS_SIZE_NODIE(ADecalActorBase)
VERIFY_CLASS_SIZE_NODIE(ADecalActor)
VERIFY_CLASS_SIZE_NODIE(ADecalActorMovable)
VERIFY_CLASS_OFFSET_NODIE(ADecalManager,DecalManager,DecalTemplate)
VERIFY_CLASS_OFFSET_NODIE(ADecalManager,DecalManager,ActiveDecals)
VERIFY_CLASS_SIZE_NODIE(ADecalManager)
VERIFY_CLASS_OFFSET_NODIE(UDecalComponent,DecalComponent,DecalMaterial)
VERIFY_CLASS_OFFSET_NODIE(UDecalComponent,DecalComponent,OriginalParentRelativeOrientationVec)
VERIFY_CLASS_SIZE_NODIE(UDecalComponent)
VERIFY_CLASS_OFFSET_NODIE(UActorFactoryDecal,ActorFactoryDecal,DecalMaterial)
VERIFY_CLASS_SIZE_NODIE(UActorFactoryDecal)
VERIFY_CLASS_SIZE_NODIE(UActorFactoryDecalMovable)
VERIFY_CLASS_SIZE_NODIE(UDecalMaterial)
#endif // VERIFY_CLASS_SIZES
#endif // !ENUMS_ONLY

#if SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif
