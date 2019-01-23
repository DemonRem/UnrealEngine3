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

#ifndef INCLUDED_ENGINE_SPEEDTREE_ENUMS
#define INCLUDED_ENGINE_SPEEDTREE_ENUMS 1


#endif // !INCLUDED_ENGINE_SPEEDTREE_ENUMS
#endif // !NO_ENUMS

#if !ENUMS_ONLY

#ifndef NAMES_ONLY
#define AUTOGENERATE_NAME(name) extern FName ENGINE_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name)
#endif


#ifndef NAMES_ONLY

#ifndef INCLUDED_ENGINE_SPEEDTREE_CLASSES
#define INCLUDED_ENGINE_SPEEDTREE_CLASSES 1

class ASpeedTreeActor : public AActor
{
public:
    //## BEGIN PROPS SpeedTreeActor
    class USpeedTreeComponent* SpeedTreeComponent;
    //## END PROPS SpeedTreeActor

    DECLARE_CLASS(ASpeedTreeActor,AActor,0,Engine)
	/**
	 * Function that gets called from within Map_Check to allow this actor to check itself
	 * for any potential errors and register them with map check dialog.
	 */
	virtual void CheckForErrors();
};

class USpeedTreeActorFactory : public UActorFactory
{
public:
    //## BEGIN PROPS SpeedTreeActorFactory
    class USpeedTree* SpeedTree;
    //## END PROPS SpeedTreeActorFactory

    DECLARE_CLASS(USpeedTreeActorFactory,UActorFactory,0|CLASS_Config,Engine)
	virtual AActor*	CreateActor(const FVector* const Location, const FRotator* const Rotation, const class USeqAct_ActorFactory* const ActorFactoryData);
	virtual UBOOL CanCreateActor(FString& OutErrorMsg);
	virtual void AutoFillFields(class USelection* Selection);
	virtual FString	GetMenuName();
};

struct FSpeedTreeStaticLight
{
    FGuid Guid;
    class UShadowMap1D* BranchAndFrondShadowMap;
    class UShadowMap1D* LeafMeshShadowMap;
    class UShadowMap1D* LeafCardShadowMap;
    class UShadowMap1D* BillboardShadowMap;
};

class USpeedTreeComponent : public UPrimitiveComponent
{
public:
    //## BEGIN PROPS SpeedTreeComponent
    class USpeedTree* SpeedTree;
    BITFIELD bUseLeaves:1;
    BITFIELD bUseBranches:1;
    BITFIELD bUseFronds:1;
    BITFIELD bUseBillboards:1;
    FLOAT LodNearDistance;
    FLOAT LodFarDistance;
    FLOAT LodLevelOverride;
    class UMaterialInterface* BranchMaterial;
    class UMaterialInterface* FrondMaterial;
    class UMaterialInterface* LeafMaterial;
    class UMaterialInterface* BillboardMaterial;
    class UTexture2D* SpeedTreeIcon;
    TArrayNoInit<struct FSpeedTreeStaticLight> StaticLights;
    FLightMapRef BranchAndFrondLightMap;
    FLightMapRef LeafMeshLightMap;
    FLightMapRef LeafCardLightMap;
    FLightMapRef BillboardLightMap;
    FMatrix RotationOnlyMatrix;
    FLOAT WindMatrixOffset;
    //## END PROPS SpeedTreeComponent

    DECLARE_CLASS(USpeedTreeComponent,UPrimitiveComponent,0,Engine)
	// UPrimitiveComponent interface
#if WITH_SPEEDTREE
	virtual void UpdateBounds();
	FPrimitiveSceneProxy* CreateSceneProxy();
	virtual void GetStaticLightingInfo(FStaticLightingPrimitiveInfo& OutPrimitiveInfo,const TArray<ULightComponent*>& InRelevantLights,const FLightingBuildOptions& Options);
	virtual	void InvalidateLightingCache();

	virtual	UBOOL PointCheck(FCheckResult& cResult, const FVector& cLocation, const FVector& cExtent, DWORD dwTraceFlags);
	virtual UBOOL LineCheck(FCheckResult& cResult, const FVector& cEnd, const FVector& cStart, const FVector& cExtent, DWORD dwTraceFlags);
#if WITH_NOVODEX
	virtual void InitComponentRBPhys(UBOOL bFixed);
#endif
#endif

	// UActorComponent interface
	virtual void SetParentToWorld(const FMatrix& ParentToWorld);
	virtual UBOOL IsValidComponent() const;

	// UObject interface.
	virtual void Serialize(FArchive& Ar);
	virtual void PostLoad();
	virtual	void PostEditChange(UProperty* puPropertyThatChanged);
	virtual void PostEditUndo();
	
	/** Initialization constructor. */
	USpeedTreeComponent();
};

class USpeedTreeComponentFactory : public UPrimitiveComponentFactory
{
public:
    //## BEGIN PROPS SpeedTreeComponentFactory
    class USpeedTreeComponent* SpeedTreeComponent;
    //## END PROPS SpeedTreeComponentFactory

    DECLARE_CLASS(USpeedTreeComponentFactory,UPrimitiveComponentFactory,0,Engine)
	virtual UBOOL FactoryIsValid( ) 
	{ 
		return SpeedTreeComponent != NULL && Super::FactoryIsValid( ); 
	}
	virtual UPrimitiveComponent* CreatePrimitiveComponent(UObject* InOuter);
};

class USpeedTree : public UObject
{
public:
    //## BEGIN PROPS SpeedTree
    class FSpeedTreeResourceHelper* SRH;
    INT RandomSeed;
    FLOAT Sink;
    FLOAT LeafStaticShadowOpacity;
    class UMaterialInterface* BranchMaterial;
    class UMaterialInterface* FrondMaterial;
    class UMaterialInterface* LeafMaterial;
    class UMaterialInterface* BillboardMaterial;
    FLOAT MaxBendAngle;
    FLOAT BranchExponent;
    FLOAT LeafExponent;
    FLOAT Response;
    FLOAT ResponseLimiter;
    FLOAT Gusting_Strength;
    FLOAT Gusting_Frequency;
    FLOAT Gusting_Duration;
    FLOAT BranchHorizontal_LowWindAngle;
    FLOAT BranchHorizontal_LowWindSpeed;
    FLOAT BranchHorizontal_HighWindAngle;
    FLOAT BranchHorizontal_HighWindSpeed;
    FLOAT BranchVertical_LowWindAngle;
    FLOAT BranchVertical_LowWindSpeed;
    FLOAT BranchVertical_HighWindAngle;
    FLOAT BranchVertical_HighWindSpeed;
    FLOAT LeafRocking_LowWindAngle;
    FLOAT LeafRocking_LowWindSpeed;
    FLOAT LeafRocking_HighWindAngle;
    FLOAT LeafRocking_HighWindSpeed;
    FLOAT LeafRustling_LowWindAngle;
    FLOAT LeafRustling_LowWindSpeed;
    FLOAT LeafRustling_HighWindAngle;
    FLOAT LeafRustling_HighWindSpeed;
    //## END PROPS SpeedTree

    DECLARE_CLASS(USpeedTree,UObject,0,Engine)
	void StaticConstructor(void);

	virtual void BeginDestroy();
	virtual UBOOL IsReadyForFinishDestroy();
	virtual void FinishDestroy();
	
	virtual void PreEditChange(UProperty* PropertyAboutToChange);
	virtual void PostEditChange(UProperty* PropertyThatChanged);
	virtual void Serialize(FArchive& Ar);
	virtual void PostLoad();

	virtual	INT	GetResourceSize(void);
	virtual	FString	GetDetailedDescription( INT InIndex );
	virtual FString	GetDesc(void);
	
	UBOOL IsInitialized();
};

#endif // !INCLUDED_ENGINE_SPEEDTREE_CLASSES
#endif // !NAMES_ONLY


#ifndef NAMES_ONLY
#undef AUTOGENERATE_NAME
#undef AUTOGENERATE_FUNCTION
#endif

#ifdef STATIC_LINKING_MOJO
#ifndef ENGINE_SPEEDTREE_NATIVE_DEFS
#define ENGINE_SPEEDTREE_NATIVE_DEFS

DECLARE_NATIVE_TYPE(Engine,USpeedTree);
DECLARE_NATIVE_TYPE(Engine,ASpeedTreeActor);
DECLARE_NATIVE_TYPE(Engine,USpeedTreeActorFactory);
DECLARE_NATIVE_TYPE(Engine,USpeedTreeComponent);
DECLARE_NATIVE_TYPE(Engine,USpeedTreeComponentFactory);

#define AUTO_INITIALIZE_REGISTRANTS_ENGINE_SPEEDTREE \
	USpeedTree::StaticClass(); \
	ASpeedTreeActor::StaticClass(); \
	USpeedTreeActorFactory::StaticClass(); \
	USpeedTreeComponent::StaticClass(); \
	USpeedTreeComponentFactory::StaticClass(); \

#endif // ENGINE_SPEEDTREE_NATIVE_DEFS

#ifdef NATIVES_ONLY
#endif // NATIVES_ONLY
#endif // STATIC_LINKING_MOJO

#ifdef VERIFY_CLASS_SIZES
VERIFY_CLASS_OFFSET_NODIE(U,SpeedTree,SRH)
VERIFY_CLASS_OFFSET_NODIE(U,SpeedTree,LeafRustling_HighWindSpeed)
VERIFY_CLASS_SIZE_NODIE(USpeedTree)
VERIFY_CLASS_OFFSET_NODIE(A,SpeedTreeActor,SpeedTreeComponent)
VERIFY_CLASS_SIZE_NODIE(ASpeedTreeActor)
VERIFY_CLASS_OFFSET_NODIE(U,SpeedTreeActorFactory,SpeedTree)
VERIFY_CLASS_SIZE_NODIE(USpeedTreeActorFactory)
VERIFY_CLASS_OFFSET_NODIE(U,SpeedTreeComponent,SpeedTree)
VERIFY_CLASS_OFFSET_NODIE(U,SpeedTreeComponent,WindMatrixOffset)
VERIFY_CLASS_SIZE_NODIE(USpeedTreeComponent)
VERIFY_CLASS_OFFSET_NODIE(U,SpeedTreeComponentFactory,SpeedTreeComponent)
VERIFY_CLASS_SIZE_NODIE(USpeedTreeComponentFactory)
#endif // VERIFY_CLASS_SIZES
#endif // !ENUMS_ONLY

#if SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif