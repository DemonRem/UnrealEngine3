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
#define AUTOGENERATE_NAME(name) extern FName UNREALED_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name)
#endif


#ifndef NAMES_ONLY

class UCascadeParticleSystemComponent : public UParticleSystemComponent
{
public:
    //## BEGIN PROPS CascadeParticleSystemComponent
    class FCascadePreviewViewportClient* CascadePreviewViewportPtr;
    //## END PROPS CascadeParticleSystemComponent

    DECLARE_CLASS(UCascadeParticleSystemComponent,UParticleSystemComponent,0,UnrealEd)
	// Collision Handling...
	virtual UBOOL SingleLineCheck(FCheckResult& Hit, AActor* SourceActor, const FVector& End, const FVector& Start, DWORD TraceFlags, const FVector& Extent);
};

#endif


#ifndef NAMES_ONLY
#undef AUTOGENERATE_NAME
#undef AUTOGENERATE_FUNCTION
#endif

#ifdef STATIC_LINKING_MOJO
#ifndef UNREALED_CASCADE_NATIVE_DEFS
#define UNREALED_CASCADE_NATIVE_DEFS

DECLARE_NATIVE_TYPE(UnrealEd,UCascadeParticleSystemComponent);

#define AUTO_INITIALIZE_REGISTRANTS_UNREALED_CASCADE \
	UCascadeParticleSystemComponent::StaticClass(); \

#endif // UNREALED_CASCADE_NATIVE_DEFS

#ifdef NATIVES_ONLY
#endif // NATIVES_ONLY
#endif // STATIC_LINKING_MOJO

#ifdef VERIFY_CLASS_SIZES
VERIFY_CLASS_OFFSET_NODIE(U,CascadeParticleSystemComponent,CascadePreviewViewportPtr)
VERIFY_CLASS_SIZE_NODIE(UCascadeParticleSystemComponent)
#endif // VERIFY_CLASS_SIZES
#endif // !ENUMS_ONLY

#if SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif
