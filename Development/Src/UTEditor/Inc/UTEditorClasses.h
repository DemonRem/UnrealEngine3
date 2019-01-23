/*===========================================================================
    C++ class definitions exported from UnrealScript.
    This is automatically generated by the tools.
    DO NOT modify this manually! Edit the corresponding .uc files instead!
===========================================================================*/
#if SUPPORTS_PRAGMA_PACK
#pragma pack (push,4)
#endif


// Split enums from the rest of the header so they can be included earlier
// than the rest of the header file by including this file twice with different
// #define wrappers. See Engine.h and look at EngineClasses.h for an example.
#if !NO_ENUMS && !defined(NAMES_ONLY)


#endif // !NO_ENUMS

#if !ENUMS_ONLY

#ifndef NAMES_ONLY
#define AUTOGENERATE_NAME(name) extern FName UTEDITOR_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name)
#endif


#ifndef NAMES_ONLY

class UGenericBrowserType_UTMapMusicInfo : public UGenericBrowserType
{
public:
    //## BEGIN PROPS GenericBrowserType_UTMapMusicInfo
    //## END PROPS GenericBrowserType_UTMapMusicInfo

    DECLARE_CLASS(UGenericBrowserType_UTMapMusicInfo,UGenericBrowserType,0,UTEditor)
	/**
	* Initialize the supported classes for this browser type.
	*/
	virtual void Init();
};

class UUTEditorUISceneClient : public UEditorUISceneClient
{
public:
    //## BEGIN PROPS UTEditorUISceneClient
    //## END PROPS UTEditorUISceneClient

    DECLARE_CLASS(UUTEditorUISceneClient,UEditorUISceneClient,0|CLASS_Transient,UTEditor)
	virtual void Render_Scene( FCanvas* Canvas, UUIScene* Scene );
};

#endif


#ifndef NAMES_ONLY
#undef AUTOGENERATE_NAME
#undef AUTOGENERATE_FUNCTION
#endif

#if SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif

#ifdef STATIC_LINKING_MOJO
#ifndef UTEDITOR_NATIVE_DEFS
#define UTEDITOR_NATIVE_DEFS

DECLARE_NATIVE_TYPE(UTEditor,UGenericBrowserType_UTMapMusicInfo);
DECLARE_NATIVE_TYPE(UTEditor,UUTEditorUISceneClient);

#define AUTO_INITIALIZE_REGISTRANTS_UTEDITOR \
	UGenericBrowserType_UTMapMusicInfo::StaticClass(); \
	UUTEditorUISceneClient::StaticClass(); \
	UUTMapMusicInfoFactoryNew::StaticClass(); \

#endif // UTEDITOR_NATIVE_DEFS

#ifdef NATIVES_ONLY
#endif // NATIVES_ONLY
#endif // STATIC_LINKING_MOJO

#ifdef VERIFY_CLASS_SIZES
VERIFY_CLASS_SIZE_NODIE(UGenericBrowserType_UTMapMusicInfo)
VERIFY_CLASS_SIZE_NODIE(UUTEditorUISceneClient)
#endif // VERIFY_CLASS_SIZES
#endif // !ENUMS_ONLY