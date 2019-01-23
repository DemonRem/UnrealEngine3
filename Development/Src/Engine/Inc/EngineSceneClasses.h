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

#ifndef INCLUDED_ENGINE_SCENE_ENUMS
#define INCLUDED_ENGINE_SCENE_ENUMS 1

enum EDetailMode
{
    DM_Low                  =0,
    DM_Medium               =1,
    DM_High                 =2,
    DM_MAX                  =3,
};
enum ESceneDepthPriorityGroup
{
    SDPG_UnrealEdBackground =0,
    SDPG_World              =1,
    SDPG_Foreground         =2,
    SDPG_UnrealEdForeground =3,
    SDPG_PostProcess        =4,
    SDPG_MAX                =5,
};

#endif // !INCLUDED_ENGINE_SCENE_ENUMS
#endif // !NO_ENUMS

#if !ENUMS_ONLY

#ifndef NAMES_ONLY
#define AUTOGENERATE_NAME(name) extern FName ENGINE_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name)
#endif


#ifndef NAMES_ONLY

#ifndef INCLUDED_ENGINE_SCENE_CLASSES
#define INCLUDED_ENGINE_SCENE_CLASSES 1

#define UCONST_SDPG_NumBits 3

class UScene : public UObject
{
public:
    //## BEGIN PROPS Scene
    //## END PROPS Scene

    DECLARE_CLASS(UScene,UObject,0,Engine)
    NO_DEFAULT_CONSTRUCTOR(UScene)
};

#endif // !INCLUDED_ENGINE_SCENE_CLASSES
#endif // !NAMES_ONLY


#ifndef NAMES_ONLY
#undef AUTOGENERATE_NAME
#undef AUTOGENERATE_FUNCTION
#endif

#ifdef STATIC_LINKING_MOJO
#ifndef ENGINE_SCENE_NATIVE_DEFS
#define ENGINE_SCENE_NATIVE_DEFS

DECLARE_NATIVE_TYPE(Engine,UScene);

#define AUTO_INITIALIZE_REGISTRANTS_ENGINE_SCENE \
	UScene::StaticClass(); \

#endif // ENGINE_SCENE_NATIVE_DEFS

#ifdef NATIVES_ONLY
#endif // NATIVES_ONLY
#endif // STATIC_LINKING_MOJO

#ifdef VERIFY_CLASS_SIZES
VERIFY_CLASS_SIZE_NODIE(UScene)
#endif // VERIFY_CLASS_SIZES
#endif // !ENUMS_ONLY

#if SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif