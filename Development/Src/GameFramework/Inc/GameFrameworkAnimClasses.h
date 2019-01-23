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

enum ERecoilStart
{
    ERS_Zero                =0,
    ERS_Random              =1,
    ERS_MAX                 =2,
};

#endif // !NO_ENUMS

#if !ENUMS_ONLY

#ifndef NAMES_ONLY
#define AUTOGENERATE_NAME(name) extern FName GAMEFRAMEWORK_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name)
#endif


#ifndef NAMES_ONLY

struct FRecoilParams
{
    BYTE X;
    BYTE Y;
    BYTE Z;
    BYTE Padding;
};

struct FRecoilDef
{
    FLOAT TimeToGo;
    FLOAT TimeDuration;
    FVector RotAmplitude;
    FVector RotFrequency;
    FVector RotSinOffset;
    struct FRecoilParams RotParams;
    FRotator RotOffset;
    FVector LocAmplitude;
    FVector LocFrequency;
    FVector LocSinOffset;
    struct FRecoilParams LocParams;
    FVector LocOffset;
};

class UGameSkelCtrl_Recoil : public USkelControlBase
{
public:
    //## BEGIN PROPS GameSkelCtrl_Recoil
    struct FRecoilDef Recoil;
    FVector2D Aim;
    BITFIELD bPlayRecoil:1;
    BITFIELD bOldPlayRecoil:1;
    BITFIELD bApplyControl:1;
    //## END PROPS GameSkelCtrl_Recoil

    DECLARE_CLASS(UGameSkelCtrl_Recoil,USkelControlBase,0,GameFramework)
	/** Pull aim information from Pawn */
	virtual FVector2D GetAim(USkeletalMeshComponent* SkelComponent);

	/** Is skeleton currently mirrored */
	virtual UBOOL IsMirrored(USkeletalMeshComponent* SkelComponent);

	// USkelControlBase interface
	virtual void TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp);
	virtual void GetAffectedBones(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<INT>& OutBoneIndices);
	virtual void CalculateNewBoneTransforms(INT BoneIndex, USkeletalMeshComponent* SkelComp, TArray<FMatrix>& OutBoneTransforms);	
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
#ifndef GAMEFRAMEWORK_ANIM_NATIVE_DEFS
#define GAMEFRAMEWORK_ANIM_NATIVE_DEFS

DECLARE_NATIVE_TYPE(GameFramework,UGameSkelCtrl_Recoil);

#define AUTO_INITIALIZE_REGISTRANTS_GAMEFRAMEWORK_ANIM \
	UGameSkelCtrl_Recoil::StaticClass(); \

#endif // GAMEFRAMEWORK_ANIM_NATIVE_DEFS

#ifdef NATIVES_ONLY
#endif // NATIVES_ONLY
#endif // STATIC_LINKING_MOJO

#ifdef VERIFY_CLASS_SIZES
VERIFY_CLASS_OFFSET_NODIE(U,GameSkelCtrl_Recoil,Recoil)
VERIFY_CLASS_OFFSET_NODIE(U,GameSkelCtrl_Recoil,Aim)
VERIFY_CLASS_SIZE_NODIE(UGameSkelCtrl_Recoil)
#endif // VERIFY_CLASS_SIZES
#endif // !ENUMS_ONLY