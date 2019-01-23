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

enum EDistributionVectorMirrorFlags
{
    EDVMF_Same              =0,
    EDVMF_Different         =1,
    EDVMF_Mirror            =2,
    EDVMF_MAX               =3,
};
enum EDistributionVectorLockFlags
{
    EDVLF_None              =0,
    EDVLF_XY                =1,
    EDVLF_XZ                =2,
    EDVLF_YZ                =3,
    EDVLF_XYZ               =4,
    EDVLF_MAX               =5,
};

#endif // !NO_ENUMS

#if !ENUMS_ONLY

#ifndef NAMES_ONLY
#define AUTOGENERATE_NAME(name) extern FName CORE_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name)
#endif

AUTOGENERATE_NAME(Main)

#ifndef NAMES_ONLY

struct Commandlet_eventMain_Parms
{
    FString Params;
    INT ReturnValue;
    Commandlet_eventMain_Parms(EEventParm)
    {
    }
};
class UCommandlet : public UObject
{
public:
    //## BEGIN PROPS Commandlet
    FStringNoInit HelpDescription;
    FStringNoInit HelpUsage;
    FStringNoInit HelpWebLink;
    TArrayNoInit<FString> HelpParamNames;
    TArrayNoInit<FString> HelpParamDescriptions;
    BITFIELD IsServer:1;
    BITFIELD IsClient:1;
    BITFIELD IsEditor:1;
    BITFIELD LogToConsole:1;
    BITFIELD ShowErrorCount:1;
    //## END PROPS Commandlet

    virtual INT Main(const FString& Params);
    DECLARE_FUNCTION(execMain)
    {
        P_GET_STR(Params);
        P_FINISH;
        *(INT*)Result=Main(Params);
    }
    INT eventMain(const FString& Params)
    {
        Commandlet_eventMain_Parms Parms(EC_EventParm);
        Parms.ReturnValue=0;
        Parms.Params=Params;
        ProcessEvent(FindFunctionChecked(CORE_Main),&Parms);
        return Parms.ReturnValue;
    }
    DECLARE_ABSTRACT_CLASS(UCommandlet,UObject,0|CLASS_Transient,Core)
	/**
	 * Parses a string into tokens, separating switches (beginning with - or /) from
	 * other parameters
	 *
	 * @param	CmdLine		the string to parse
	 * @param	Tokens		[out] filled with all parameters found in the string
	 * @param	Switches	[out] filled with all switches found in the string
	 *
	 * @return	@todo
	 */
	static void ParseCommandLine( const TCHAR* CmdLine, TArray<FString>& Tokens, TArray<FString>& Switches )
	{
		FString NextToken;
		while ( ParseToken(CmdLine, NextToken, FALSE) )
		{
			if ( **NextToken == TCHAR('-') || **NextToken == TCHAR('/') )
			{
				new(Switches) FString(NextToken.Mid(1));
			}
			else
			{
				new(Tokens) FString(NextToken);
			}
		}
	}

	/**
	 * This is where you put any custom code that needs to be executed from InitializeIntrinsicPropertyValues() in
	 * your commandlet
	 */
	void StaticInitialize() {}

	/**
	 * Allows commandlets to override the default behavior and create a custom engine class for the commandlet. If
	 * the commandlet implements this function, it should fully initialize the UEngine object as well.  Commandlets
	 * should indicate that they have implemented this function by assigning the custom UEngine to GEngine.
	 */
	virtual void CreateCustomEngine() {}
};

class UHelpCommandlet : public UCommandlet
{
public:
    //## BEGIN PROPS HelpCommandlet
    //## END PROPS HelpCommandlet

    virtual INT Main(const FString& Params);
    DECLARE_CLASS(UHelpCommandlet,UCommandlet,0|CLASS_Transient,Core)
    NO_DEFAULT_CONSTRUCTOR(UHelpCommandlet)
};

class UComponent : public UObject
{
public:
    //## BEGIN PROPS Component
    class UClass* TemplateOwnerClass;
    FName TemplateName;
    //## END PROPS Component

    DECLARE_ABSTRACT_CLASS(UComponent,UObject,0,Core)
	/**
	 * Given a subobject and an owner class, save a refernce to it for retrieveing defaults on load
	 * @param OriginalSubObject	The original template for this subobject (or another instance for a duplication?)
	 * @param OwnerClass			The class that contains the original template
	 * @param SubObjectName		If the OriginalSubObject is NULL, manually set the name of the subobject to this
	 */
	void LinkToSourceDefaultObject(UComponent* OriginalComponent, UClass* OwnerClass, FName ComponentName = NAME_None);

	/**
	 * Copies the SourceDefaultObject onto our own memory to propagate any modified defaults
	 * @param Ar	The archive used to serialize the pointer to the subobject template
	 */
	void PreSerialize(FArchive& Ar);

	/**
	 * Copies the Source DefaultObject onto our own memory to propagate any modified defaults
	 * @return The object pointed to by the SourceDefaultActorClass and SourceDefaultSubObjectName
	 */
	UComponent* ResolveSourceDefaultObject();

	/**
	 * Returns name to use for this component in component instancing maps.
	 *
	 * @return 	a name for this component which is unique within a single object graph.
	 */
	FName GetInstanceMapName() const;

	/**
	 * Returns whether this component was instanced from a component template.
	 *
	 * @return	TRUE if this component was instanced from a template.  FALSE if this component was created manually at runtime.
	 */
	UBOOL IsInstanced() const;

	/**
	 * Returns whether native properties are identical to the one of the passed in component.
	 *
	 * @param	Other	Other component to compare against
	 *
	 * @return TRUE if native properties are identical, FALSE otherwise
	 */
	virtual UBOOL AreNativePropertiesIdenticalTo( UComponent* Other ) const;

	/**
	 * Callback for retrieving a textual representation of natively serialized properties.  Child classes should implement this method if they wish
	 * to have natively serialized property values included in things like diffcommandlet output.
	 *
	 * @param	out_PropertyValues	receives the property names and values which should be reported for this object.  The map's key should be the name of
	 *								the property and the map's value should be the textual representation of the property's value.  The property value should
	 *								be formatted the same way that UProperty::ExportText formats property values (i.e. for arrays, wrap in quotes and use a comma
	 *								as the delimiter between elements, etc.)
	 * @param	ExportFlags			bitmask of EPropertyPortFlags used for modifying the format of the property values
	 *
	 * @return	return TRUE if property values were added to the map.
	 */
	virtual UBOOL GetNativePropertyValues( TMap<FString,FString>& out_PropertyValues, DWORD ExportFlags=0 ) const;

	// UObject interface.

	virtual UBOOL IsPendingKill() const;

};

struct FRawDistributionFloat : public FRawDistribution
{
    class UDistributionFloat* Distribution;

#if !CONSOLE
	/**
	 * Initialize a raw distribution from the original Unreal distribution
	 */
	void Initialize();
#endif
	 	
	/**
	 * Gets a pointer to the raw distribution if you can just call FRawDistribution::GetValue1 on it, otherwise NULL 
	 */
	const FRawDistribution* GetFastRawDistribution();

	/**
	 * Get the value at the specified F
	 */
	FLOAT GetValue(FLOAT F=0.0f, UObject* Data=NULL);

	/**
	 * Get the min and max values
	 */
	void GetOutRange(FLOAT& MinOut, FLOAT& MaxOut);

	/**
	 * Is this distribution a uniform type? (ie, does it have two values per entry?)
	 */
	inline UBOOL IsUniform() { return LookupTableNumElements == 2; }

};

class UDistributionFloat : public UComponent, public FCurveEdInterface
{
public:
    //## BEGIN PROPS DistributionFloat
    BITFIELD bCanBeBaked:1;
    BITFIELD bIsDirty:1;
    //## END PROPS DistributionFloat

    DECLARE_ABSTRACT_CLASS(UDistributionFloat,UComponent,0,Core)

#if !CONSOLE
	/**
	 * Return the operation used at runtime to calculate the final value
	 */
	virtual ERawDistributionOperation GetOperation() { return RDO_None; }
	
	/**
	 * Fill out an array of floats and return the number of elements in the entry
	 *
	 * @param Time The time to evaluate the distribution
	 * @param Values An array of values to be filled out, guaranteed to be big enough for 4 values
	 * @return The number of elements (values) set in the array
	 */
	virtual DWORD InitializeRawEntry(FLOAT Time, FLOAT* Values);
#endif
	virtual FLOAT	GetValue( FLOAT F = 0.f, UObject* Data = NULL );

	virtual void	GetInRange(FLOAT& MinIn, FLOAT& MaxIn);
	virtual void	GetOutRange(FLOAT& MinOut, FLOAT& MaxOut);
	
	/**
	 * Return whether or not this distribution can be baked into a FRawDistribution lookup table
	 */
	virtual UBOOL CanBeBaked() const 
	{
		return bCanBeBaked; 
	}

	/** UObject interface */
	virtual void Serialize(FArchive& Ar);

	virtual void PostEditChange(UProperty* PropertyThatChanged);
	
	/**
	 * If the distribution can be baked, then we don't need it on the client or server
	 */
	virtual UBOOL NeedsLoadForClient() const;
	virtual UBOOL NeedsLoadForServer() const;
};

struct FRawDistributionVector : public FRawDistribution
{
    class UDistributionVector* Distribution;

#if !CONSOLE
	/**
	 * Initialize a raw distribution from the original Unreal distribution
	 */
	void Initialize();
#endif

	/**
 	 * Gets a pointer to the raw distribution if you can just call FRawDistribution::GetValue3 on it, otherwise NULL 
 	 */
 	const FRawDistribution *GetFastRawDistribution();

	/**
	 * Get the value at the specified F
	 */
	FVector GetValue(FLOAT F=0.0f, UObject* Data=NULL, INT LastExtreme=0);

	/**
	 * Get the min and max values
	 */
	void GetOutRange(FLOAT& MinOut, FLOAT& MaxOut);

	/**
	 * Is this distribution a uniform type? (ie, does it have two values per entry?)
	 */
	inline UBOOL IsUniform() { return LookupTableNumElements == 2; }

};

class UDistributionVector : public UComponent, public FCurveEdInterface
{
public:
    //## BEGIN PROPS DistributionVector
    BITFIELD bCanBeBaked:1;
    BITFIELD bIsDirty:1;
    //## END PROPS DistributionVector

    DECLARE_ABSTRACT_CLASS(UDistributionVector,UComponent,0,Core)

#if !CONSOLE
	/**
	 * Return the operation used at runtime to calculate the final value
	 */
	virtual ERawDistributionOperation GetOperation() { return RDO_None; }
	
	/**
	 * Fill out an array of vectors and return the number of elements in the entry
	 *
	 * @param Time The time to evaluate the distribution
	 * @param Values An array of values to be filled out, guaranteed to be big enough for 2 vectors
	 * @return The number of elements (values) set in the array
	 */
	virtual DWORD InitializeRawEntry(FLOAT Time, FVector* Values);
#endif

	virtual FVector	GetValue( FLOAT F = 0.f, UObject* Data = NULL, INT LastExtreme = 0 );

	virtual void	GetInRange(FLOAT& MinIn, FLOAT& MaxIn);
	virtual void	GetOutRange(FLOAT& MinOut, FLOAT& MaxOut);
	virtual	void	GetRange(FVector& OutMin, FVector& OutMax);

	/**
	 * Return whether or not this distribution can be baked into a FRawDistribution lookup table
	 */
	virtual UBOOL CanBeBaked() const 
	{
		return bCanBeBaked; 
	}

	/** UObject interface */
	virtual void Serialize(FArchive& Ar);

	virtual void PostEditChange(UProperty* PropertyThatChanged);
	
	/**
	 * If the distribution can be baked, then we don't need it on the client or server
	 */
	virtual UBOOL NeedsLoadForClient() const;
	virtual UBOOL NeedsLoadForServer() const;
};

class UInterface : public UObject
{
public:
    DECLARE_ABSTRACT_CLASS(UInterface,UObject,0|CLASS_Interface,Core)
    NO_DEFAULT_CONSTRUCTOR(UInterface)
};

class IInterface
{
public:
	typedef UInterface UClassType;
    NO_DEFAULT_CONSTRUCTOR(IInterface)
};

class USubsystem : public UObject, public FExec
{
public:
    //## BEGIN PROPS Subsystem
    //## END PROPS Subsystem

    DECLARE_ABSTRACT_CLASS(USubsystem,UObject,0|CLASS_Transient,Core)

	// USubsystem interface.
	virtual void Tick( FLOAT DeltaTime )
	{}

	// FExec interface.
	virtual UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar ) { return 0; }

};

#endif

AUTOGENERATE_FUNCTION(UCommandlet,-1,execMain);
AUTOGENERATE_FUNCTION(UHelpCommandlet,-1,execMain);

#ifndef NAMES_ONLY
#undef AUTOGENERATE_NAME
#undef AUTOGENERATE_FUNCTION
#endif

#ifdef STATIC_LINKING_MOJO
#ifndef CORE_NATIVE_DEFS
#define CORE_NATIVE_DEFS

DECLARE_NATIVE_TYPE(Core,UCommandlet);
DECLARE_NATIVE_TYPE(Core,UComponent);
DECLARE_NATIVE_TYPE(Core,UDistributionFloat);
DECLARE_NATIVE_TYPE(Core,UDistributionVector);
DECLARE_NATIVE_TYPE(Core,UHelpCommandlet);
DECLARE_NATIVE_TYPE(Core,UInterface);
DECLARE_NATIVE_TYPE(Core,UObject);
DECLARE_NATIVE_TYPE(Core,USubsystem);

#define AUTO_INITIALIZE_REGISTRANTS_CORE \
	UArrayProperty::StaticClass(); \
	UBoolProperty::StaticClass(); \
	UByteProperty::StaticClass(); \
	UClass::StaticClass(); \
	UClassProperty::StaticClass(); \
	UCommandlet::StaticClass(); \
	GNativeLookupFuncs[Lookup++] = &FindCoreUCommandletNative; \
	UComponent::StaticClass(); \
	UComponentProperty::StaticClass(); \
	UConst::StaticClass(); \
	UDelegateProperty::StaticClass(); \
	UDistributionFloat::StaticClass(); \
	UDistributionVector::StaticClass(); \
	UEnum::StaticClass(); \
	UExporter::StaticClass(); \
	UFactory::StaticClass(); \
	UField::StaticClass(); \
	UFloatProperty::StaticClass(); \
	UFunction::StaticClass(); \
	UHelpCommandlet::StaticClass(); \
	GNativeLookupFuncs[Lookup++] = &FindCoreUHelpCommandletNative; \
	UInterface::StaticClass(); \
	UInterfaceProperty::StaticClass(); \
	UIntProperty::StaticClass(); \
	ULinker::StaticClass(); \
	ULinkerLoad::StaticClass(); \
	ULinkerSave::StaticClass(); \
	UMapProperty::StaticClass(); \
	UMetaData::StaticClass(); \
	UNameProperty::StaticClass(); \
	UObject::StaticClass(); \
	GNativeLookupFuncs[Lookup++] = &FindCoreUObjectNative; \
	UObjectProperty::StaticClass(); \
	UObjectRedirector::StaticClass(); \
	UObjectSerializer::StaticClass(); \
	UPackage::StaticClass(); \
	UPackageMap::StaticClass(); \
	UProperty::StaticClass(); \
	UScriptStruct::StaticClass(); \
	UState::StaticClass(); \
	UStrProperty::StaticClass(); \
	UStruct::StaticClass(); \
	UStructProperty::StaticClass(); \
	USubsystem::StaticClass(); \
	USystem::StaticClass(); \
	UTextBuffer::StaticClass(); \
	UTextBufferFactory::StaticClass(); \

#endif // CORE_NATIVE_DEFS

#ifdef NATIVES_ONLY
NATIVE_INFO(UCommandlet) GCoreUCommandletNatives[] = 
{ 
	MAP_NATIVE(UCommandlet,execMain)
	{NULL,NULL}
};
IMPLEMENT_NATIVE_HANDLER(Core,UCommandlet);

NATIVE_INFO(UHelpCommandlet) GCoreUHelpCommandletNatives[] = 
{ 
	MAP_NATIVE(UHelpCommandlet,execMain)
	{NULL,NULL}
};
IMPLEMENT_NATIVE_HANDLER(Core,UHelpCommandlet);

NATIVE_INFO(UObject) GCoreUObjectNatives[] = 
{ 
	MAP_NATIVE(UObject,execIsPendingKill)
	MAP_NATIVE(UObject,execGetAngularFromDotDist)
	MAP_NATIVE(UObject,execGetAngularDistance)
	MAP_NATIVE(UObject,execGetDotDistance)
	MAP_NATIVE(UObject,execPointDistToLine)
	MAP_NATIVE(UObject,execGetPerObjectConfigSections)
	MAP_NATIVE(UObject,execStaticSaveConfig)
	MAP_NATIVE(UObject,execSaveConfig)
	MAP_NATIVE(UObject,execFindObject)
	MAP_NATIVE(UObject,execDynamicLoadObject)
	MAP_NATIVE(UObject,execGetEnum)
	MAP_NATIVE(UObject,execDisable)
	MAP_NATIVE(UObject,execEnable)
	MAP_NATIVE(UObject,execDumpStateStack)
	MAP_NATIVE(UObject,execPopState)
	MAP_NATIVE(UObject,execPushState)
	MAP_NATIVE(UObject,execGetStateName)
	MAP_NATIVE(UObject,execIsChildState)
	MAP_NATIVE(UObject,execIsInState)
	MAP_NATIVE(UObject,execGotoState)
	MAP_NATIVE(UObject,execIsUTracing)
	MAP_NATIVE(UObject,execSetUTracing)
	MAP_NATIVE(UObject,execGetFuncName)
	MAP_NATIVE(UObject,execScriptTrace)
	MAP_NATIVE(UObject,execLocalize)
	MAP_NATIVE(UObject,execWarnInternal)
	MAP_NATIVE(UObject,execLogInternal)
	MAP_NATIVE(UObject,execSubtract_QuatQuat)
	MAP_NATIVE(UObject,execAdd_QuatQuat)
	MAP_NATIVE(UObject,execQuatSlerp)
	MAP_NATIVE(UObject,execQuatToRotator)
	MAP_NATIVE(UObject,execQuatFromRotator)
	MAP_NATIVE(UObject,execQuatFromAxisAndAngle)
	MAP_NATIVE(UObject,execQuatFindBetween)
	MAP_NATIVE(UObject,execQuatRotateVector)
	MAP_NATIVE(UObject,execQuatInvert)
	MAP_NATIVE(UObject,execQuatDot)
	MAP_NATIVE(UObject,execQuatProduct)
	MAP_NATIVE(UObject,execNotEqual_NameName)
	MAP_NATIVE(UObject,execEqualEqual_NameName)
	MAP_NATIVE(UObject,execIsA)
	MAP_NATIVE(UObject,execClassIsChildOf)
	MAP_NATIVE(UObject,execNotEqual_InterfaceInterface)
	MAP_NATIVE(UObject,execEqualEqual_InterfaceInterface)
	MAP_NATIVE(UObject,execNotEqual_ObjectObject)
	MAP_NATIVE(UObject,execEqualEqual_ObjectObject)
	MAP_NATIVE(UObject,execPathName)
	MAP_NATIVE(UObject,execParseStringIntoArray)
	MAP_NATIVE(UObject,execRepl)
	MAP_NATIVE(UObject,execAsc)
	MAP_NATIVE(UObject,execChr)
	MAP_NATIVE(UObject,execLocs)
	MAP_NATIVE(UObject,execCaps)
	MAP_NATIVE(UObject,execRight)
	MAP_NATIVE(UObject,execLeft)
	MAP_NATIVE(UObject,execMid)
	MAP_NATIVE(UObject,execInStr)
	MAP_NATIVE(UObject,execLen)
	MAP_NATIVE(UObject,execSubtractEqual_StrStr)
	MAP_NATIVE(UObject,execAtEqual_StrStr)
	MAP_NATIVE(UObject,execConcatEqual_StrStr)
	MAP_NATIVE(UObject,execComplementEqual_StrStr)
	MAP_NATIVE(UObject,execNotEqual_StrStr)
	MAP_NATIVE(UObject,execEqualEqual_StrStr)
	MAP_NATIVE(UObject,execGreaterEqual_StrStr)
	MAP_NATIVE(UObject,execLessEqual_StrStr)
	MAP_NATIVE(UObject,execGreater_StrStr)
	MAP_NATIVE(UObject,execLess_StrStr)
	MAP_NATIVE(UObject,execAt_StrStr)
	MAP_NATIVE(UObject,execConcat_StrStr)
	MAP_NATIVE(UObject,execRDiff)
	MAP_NATIVE(UObject,execNormalizeRotAxis)
	MAP_NATIVE(UObject,execRInterpTo)
	MAP_NATIVE(UObject,execRSmerp)
	MAP_NATIVE(UObject,execRLerp)
	MAP_NATIVE(UObject,execNormalize)
	MAP_NATIVE(UObject,execOrthoRotation)
	MAP_NATIVE(UObject,execRotRand)
	MAP_NATIVE(UObject,execGetUnAxes)
	MAP_NATIVE(UObject,execGetAxes)
	MAP_NATIVE(UObject,execClockwiseFrom_IntInt)
	MAP_NATIVE(UObject,execSubtractEqual_RotatorRotator)
	MAP_NATIVE(UObject,execAddEqual_RotatorRotator)
	MAP_NATIVE(UObject,execSubtract_RotatorRotator)
	MAP_NATIVE(UObject,execAdd_RotatorRotator)
	MAP_NATIVE(UObject,execDivideEqual_RotatorFloat)
	MAP_NATIVE(UObject,execMultiplyEqual_RotatorFloat)
	MAP_NATIVE(UObject,execDivide_RotatorFloat)
	MAP_NATIVE(UObject,execMultiply_FloatRotator)
	MAP_NATIVE(UObject,execMultiply_RotatorFloat)
	MAP_NATIVE(UObject,execNotEqual_RotatorRotator)
	MAP_NATIVE(UObject,execEqualEqual_RotatorRotator)
	MAP_NATIVE(UObject,execClampLength)
	MAP_NATIVE(UObject,execVInterpTo)
	MAP_NATIVE(UObject,execIsZero)
	MAP_NATIVE(UObject,execProjectOnTo)
	MAP_NATIVE(UObject,execMirrorVectorByNormal)
	MAP_NATIVE(UObject,execVRand)
	MAP_NATIVE(UObject,execVSmerp)
	MAP_NATIVE(UObject,execVLerp)
	MAP_NATIVE(UObject,execNormal)
	MAP_NATIVE(UObject,execVSizeSq2D)
	MAP_NATIVE(UObject,execVSizeSq)
	MAP_NATIVE(UObject,execVSize2D)
	MAP_NATIVE(UObject,execVSize)
	MAP_NATIVE(UObject,execSubtractEqual_VectorVector)
	MAP_NATIVE(UObject,execAddEqual_VectorVector)
	MAP_NATIVE(UObject,execDivideEqual_VectorFloat)
	MAP_NATIVE(UObject,execMultiplyEqual_VectorVector)
	MAP_NATIVE(UObject,execMultiplyEqual_VectorFloat)
	MAP_NATIVE(UObject,execCross_VectorVector)
	MAP_NATIVE(UObject,execDot_VectorVector)
	MAP_NATIVE(UObject,execNotEqual_VectorVector)
	MAP_NATIVE(UObject,execEqualEqual_VectorVector)
	MAP_NATIVE(UObject,execGreaterGreater_VectorRotator)
	MAP_NATIVE(UObject,execLessLess_VectorRotator)
	MAP_NATIVE(UObject,execSubtract_VectorVector)
	MAP_NATIVE(UObject,execAdd_VectorVector)
	MAP_NATIVE(UObject,execDivide_VectorFloat)
	MAP_NATIVE(UObject,execMultiply_VectorVector)
	MAP_NATIVE(UObject,execMultiply_FloatVector)
	MAP_NATIVE(UObject,execMultiply_VectorFloat)
	MAP_NATIVE(UObject,execSubtract_PreVector)
	MAP_NATIVE(UObject,execFInterpTo)
	MAP_NATIVE(UObject,execFCubicInterp)
	MAP_NATIVE(UObject,execLerp)
	MAP_NATIVE(UObject,execFClamp)
	MAP_NATIVE(UObject,execFMax)
	MAP_NATIVE(UObject,execFMin)
	MAP_NATIVE(UObject,execFRand)
	MAP_NATIVE(UObject,execPow)
	MAP_NATIVE(UObject,execSquare)
	MAP_NATIVE(UObject,execSqrt)
	MAP_NATIVE(UObject,execLoge)
	MAP_NATIVE(UObject,execExp)
	MAP_NATIVE(UObject,execAtan)
	MAP_NATIVE(UObject,execTan)
	MAP_NATIVE(UObject,execAcos)
	MAP_NATIVE(UObject,execCos)
	MAP_NATIVE(UObject,execAsin)
	MAP_NATIVE(UObject,execSin)
	MAP_NATIVE(UObject,execAbs)
	MAP_NATIVE(UObject,execSubtractEqual_FloatFloat)
	MAP_NATIVE(UObject,execAddEqual_FloatFloat)
	MAP_NATIVE(UObject,execDivideEqual_FloatFloat)
	MAP_NATIVE(UObject,execMultiplyEqual_FloatFloat)
	MAP_NATIVE(UObject,execNotEqual_FloatFloat)
	MAP_NATIVE(UObject,execComplementEqual_FloatFloat)
	MAP_NATIVE(UObject,execEqualEqual_FloatFloat)
	MAP_NATIVE(UObject,execGreaterEqual_FloatFloat)
	MAP_NATIVE(UObject,execLessEqual_FloatFloat)
	MAP_NATIVE(UObject,execGreater_FloatFloat)
	MAP_NATIVE(UObject,execLess_FloatFloat)
	MAP_NATIVE(UObject,execSubtract_FloatFloat)
	MAP_NATIVE(UObject,execAdd_FloatFloat)
	MAP_NATIVE(UObject,execPercent_FloatFloat)
	MAP_NATIVE(UObject,execDivide_FloatFloat)
	MAP_NATIVE(UObject,execMultiply_FloatFloat)
	MAP_NATIVE(UObject,execMultiplyMultiply_FloatFloat)
	MAP_NATIVE(UObject,execSubtract_PreFloat)
	MAP_NATIVE(UObject,execToHex)
	MAP_NATIVE(UObject,execClamp)
	MAP_NATIVE(UObject,execMax)
	MAP_NATIVE(UObject,execMin)
	MAP_NATIVE(UObject,execRand)
	MAP_NATIVE(UObject,execSubtractSubtract_Int)
	MAP_NATIVE(UObject,execAddAdd_Int)
	MAP_NATIVE(UObject,execSubtractSubtract_PreInt)
	MAP_NATIVE(UObject,execAddAdd_PreInt)
	MAP_NATIVE(UObject,execSubtractEqual_IntInt)
	MAP_NATIVE(UObject,execAddEqual_IntInt)
	MAP_NATIVE(UObject,execDivideEqual_IntFloat)
	MAP_NATIVE(UObject,execMultiplyEqual_IntFloat)
	MAP_NATIVE(UObject,execOr_IntInt)
	MAP_NATIVE(UObject,execXor_IntInt)
	MAP_NATIVE(UObject,execAnd_IntInt)
	MAP_NATIVE(UObject,execNotEqual_IntInt)
	MAP_NATIVE(UObject,execEqualEqual_IntInt)
	MAP_NATIVE(UObject,execGreaterEqual_IntInt)
	MAP_NATIVE(UObject,execLessEqual_IntInt)
	MAP_NATIVE(UObject,execGreater_IntInt)
	MAP_NATIVE(UObject,execLess_IntInt)
	MAP_NATIVE(UObject,execGreaterGreaterGreater_IntInt)
	MAP_NATIVE(UObject,execGreaterGreater_IntInt)
	MAP_NATIVE(UObject,execLessLess_IntInt)
	MAP_NATIVE(UObject,execSubtract_IntInt)
	MAP_NATIVE(UObject,execAdd_IntInt)
	MAP_NATIVE(UObject,execDivide_IntInt)
	MAP_NATIVE(UObject,execMultiply_IntInt)
	MAP_NATIVE(UObject,execSubtract_PreInt)
	MAP_NATIVE(UObject,execComplement_PreInt)
	MAP_NATIVE(UObject,execSubtractSubtract_Byte)
	MAP_NATIVE(UObject,execAddAdd_Byte)
	MAP_NATIVE(UObject,execSubtractSubtract_PreByte)
	MAP_NATIVE(UObject,execAddAdd_PreByte)
	MAP_NATIVE(UObject,execSubtractEqual_ByteByte)
	MAP_NATIVE(UObject,execAddEqual_ByteByte)
	MAP_NATIVE(UObject,execDivideEqual_ByteByte)
	MAP_NATIVE(UObject,execMultiplyEqual_ByteFloat)
	MAP_NATIVE(UObject,execMultiplyEqual_ByteByte)
	MAP_NATIVE(UObject,execOrOr_BoolBool)
	MAP_NATIVE(UObject,execXorXor_BoolBool)
	MAP_NATIVE(UObject,execAndAnd_BoolBool)
	MAP_NATIVE(UObject,execNotEqual_BoolBool)
	MAP_NATIVE(UObject,execEqualEqual_BoolBool)
	MAP_NATIVE(UObject,execNot_PreBool)
	{NULL,NULL}
};
IMPLEMENT_NATIVE_HANDLER(Core,UObject);

#endif // NATIVES_ONLY
#endif // STATIC_LINKING_MOJO

#ifdef VERIFY_CLASS_SIZES
VERIFY_CLASS_OFFSET_NODIE(U,Commandlet,HelpDescription)
VERIFY_CLASS_OFFSET_NODIE(U,Commandlet,HelpParamDescriptions)
VERIFY_CLASS_SIZE_NODIE(UCommandlet)
VERIFY_CLASS_OFFSET_NODIE(U,Component,TemplateOwnerClass)
VERIFY_CLASS_OFFSET_NODIE(U,Component,TemplateName)
VERIFY_CLASS_SIZE_NODIE(UComponent)
VERIFY_CLASS_SIZE_NODIE(UDistributionFloat)
VERIFY_CLASS_SIZE_NODIE(UDistributionVector)
VERIFY_CLASS_SIZE_NODIE(UHelpCommandlet)
VERIFY_CLASS_SIZE_NODIE(UInterface)
VERIFY_CLASS_OFFSET_NODIE(U,Object,ObjectFlags)
VERIFY_CLASS_OFFSET_NODIE(U,Object,HashNext)
VERIFY_CLASS_OFFSET_NODIE(U,Object,HashOuterNext)
VERIFY_CLASS_OFFSET_NODIE(U,Object,StateFrame)
VERIFY_CLASS_OFFSET_NODIE(U,Object,Outer)
VERIFY_CLASS_OFFSET_NODIE(U,Object,Name)
VERIFY_CLASS_OFFSET_NODIE(U,Object,Class)
VERIFY_CLASS_OFFSET_NODIE(U,Object,ObjectArchetype)
VERIFY_CLASS_SIZE_NODIE(UObject)
VERIFY_CLASS_SIZE_NODIE(USubsystem)
#endif // VERIFY_CLASS_SIZES
#endif // !ENUMS_ONLY

#if SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif
