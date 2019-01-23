/*=============================================================================
	UnObjBas.h: Unreal object base class.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/*-----------------------------------------------------------------------------
	Core enumerations.
-----------------------------------------------------------------------------*/

//
// Flags for loading objects.
//
enum ELoadFlags
{
	LOAD_None			= 0x00000000,	// No flags.
	LOAD_SeekFree		= 0x00000001,	// Loads the package via the seek free loading path/ reader
	LOAD_NoWarn			= 0x00000002,	// Don't display warning if load fails.
	LOAD_Throw			= 0x00000008,	// Throw exceptions upon failure.
	LOAD_Verify			= 0x00000010,	// Only verify existance; don't actually load.
	LOAD_AllowDll		= 0x00000020,	// Allow plain DLLs.
	LOAD_DisallowFiles  = 0x00000040,	// Don't load from file.
	LOAD_NoVerify       = 0x00000080,   // Don't verify imports yet.
	LOAD_Quiet			= 0x00002000,   // No log warnings.
	LOAD_FindIfFail		= 0x00004000,	// Tries FindObject if a linker cannot be obtained (e.g. package is currently being compiled)
	LOAD_MemoryReader	= 0x00008000,	// Loads the file into memory and serializes from there.
};

//
// Flags for saving packages
//
enum ESaveFlags
{
	SAVE_None			= 0x00000000,	// No flags
	SAVE_NoError		= 0x00000001,	// Don't generate errors on save
};

//
// Package flags.
//
enum EPackageFlags
{
	PKG_AllowDownload				= 0x00000001,	// Allow downloading package.
	PKG_ClientOptional				= 0x00000002,	// Purely optional for clients.
	PKG_ServerSideOnly				= 0x00000004,   // Only needed on the server side.
	PKG_Cooked						= 0x00000008,	// Whether this package has been cooked for the target platform.
	PKG_Unsecure					= 0x00000010,   // Not trusted.
	PKG_SavedWithNewerVersion		= 0x00000020,	// Package was saved with newer version.
	PKG_Need						= 0x00008000,	// Client needs to download this package.
	PKG_Compiling					= 0x00010000,	// package is currently being compiled
	PKG_ContainsMap					= 0x00020000,	// Set if the package contains a ULevel/ UWorld object
	PKG_Trash						= 0x00040000,	// Set if the package was loaded from the trashcan
	PKG_DisallowLazyLoading			= 0x00080000,	// Set if the archive serializing this package cannot use lazy loading
	PKG_PlayInEditor				= 0x00100000,	// Set if the package was created for the purpose of PIE
	PKG_ContainsScript				= 0x00200000,	// Package is allowed to contain UClasses and unrealscript
	PKG_ContainsDebugInfo			= 0x00400000,	// Package contains debug info (for UDebugger)
	PKG_RequireImportsAlreadyLoaded	= 0x00800000,	// Package requires all its imports to already have been loaded
	PKG_SelfContainedLighting		= 0x01000000,	// All lighting in this package should be self contained
	PKG_StoreCompressed				= 0x02000000,	// Package is being stored compressed, requires archive support for compression
	PKG_StoreFullyCompressed		= 0x04000000,	// Package is serialized normally, and then fully compressed after (must be decompressed before LoadPackage is called)
	PKG_DownloadButNoLoad			= 0x08000000,	// Package needs to be downloaded to clients, but should not be loaded while LoadPackage'ing everything in the package map (useful for Standalone seekfree packages) 
};

//
// Internal enums.
//
enum ENativeConstructor    {EC_NativeConstructor};
enum EStaticConstructor    {EC_StaticConstructor};
enum EInternal             {EC_Internal};
enum ECppProperty          {EC_CppProperty};
enum EEventParm            {EC_EventParm};

//
// Result of GotoState.
//
enum EGotoState
{
	GOTOSTATE_NotFound		= 0,
	GOTOSTATE_Success		= 1,
	GOTOSTATE_Preempted		= 2,
};

/**
 * Flags describing a class.
 */
enum EClassFlags
{
	/** @name Base flags */
	//@{
	CLASS_None				  = 0x00000000,
	/** Class is abstract and can't be instantiated directly. */
	CLASS_Abstract            = 0x00000001,
	/** Script has been compiled successfully. */
	CLASS_Compiled			  = 0x00000002,
	/** Load object configuration at construction time. */
	CLASS_Config			  = 0x00000004,
	/** This object type can't be saved; null it out at save time. */
	CLASS_Transient			  = 0x00000008,
	/** Successfully parsed. */
	CLASS_Parsed              = 0x00000010,
	/** Class contains localized text. */
	CLASS_Localized           = 0x00000020,
	/** Objects of this class can be safely replaced with default or NULL. */
	CLASS_SafeReplace         = 0x00000040,
	/** Class is a native class - native interfaces will have CLASS_Native set, but not RF_Native */
	CLASS_Native			  = 0x00000080,
	/** Don't export to C++ header. */
	CLASS_NoExport            = 0x00000100,
	/** Allow users to create in the editor. */
	CLASS_Placeable           = 0x00000200,
	/** Handle object configuration on a per-object basis, rather than per-class. */
	CLASS_PerObjectConfig     = 0x00000400,
	/** Replication handled in C++. */
	CLASS_NativeReplication   = 0x00000800,
	/** Class can be constructed from editinline New button. */
	CLASS_EditInlineNew		  = 0x00001000,
	/** Display properties in the editor without using categories. */
	CLASS_CollapseCategories  = 0x00002000,
	/** Class is an interface **/
	CLASS_Interface           = 0x00004000,
	/** IsA UProperty */
    CLASS_IsAUProperty        = 0x00008000,
	/** IsA UObjectProperty */
    CLASS_IsAUObjectProperty  = 0x00010000,
	/** IsA UBoolProperty */
    CLASS_IsAUBoolProperty    = 0x00020000,
	/** IsA UState */
    CLASS_IsAUState           = 0x00040000,
	/** IsA UFunction */
    CLASS_IsAUFunction        = 0x00080000,
	/** IsA UStructProperty */
	CLASS_IsAUStructProperty  = 0x00100000,
	/**
	 * Indicates that this class contains object properties which are marked 'instanced' (or editinline export).  Set by the script compiler after all properties in the
	 * class have been parsed.  Used by the loading code as an optimization to attempt to instance newly added properties only for relevant classes
	 */
	CLASS_HasInstancedProps   = 0x00200000,
	/** Class needs its defaultproperties imported */
	CLASS_NeedsDefProps		  = 0x00400000,
	/** Class has component properties. */
	CLASS_HasComponents		  = 0x00800000,
	/** Don't show this class in the editor class browser or edit inline new menus. */
	CLASS_Hidden			  = 0x01000000,
	/** Don't save objects of this class when serializing */
	CLASS_Deprecated		  = 0x02000000,
	/** Class not shown in editor drop down for class selection */
	CLASS_HideDropDown		  = 0x04000000,
	/** Class has been exported to a header file */
	CLASS_Exported			  = 0x08000000,
	/** Class has no unrealscript counter-part */
	CLASS_Intrinsic			  = 0x10000000,
	/** Class is a unique component class */
	CLASS_UniqueComponent	  = 0x20000000,


	//@}


	/** @name Flags to inherit from base class */
	//@{
	CLASS_Inherit           = CLASS_Transient | CLASS_Config | CLASS_Localized | CLASS_SafeReplace | CLASS_PerObjectConfig | CLASS_Placeable
							| CLASS_IsAUProperty | CLASS_IsAUObjectProperty | CLASS_IsAUBoolProperty | CLASS_IsAUStructProperty | CLASS_IsAUState | CLASS_IsAUFunction
							| CLASS_HasComponents | CLASS_Deprecated | CLASS_Intrinsic | CLASS_HasInstancedProps,

	/** these flags will be cleared by the compiler when the class is parsed during script compilation */
	CLASS_RecompilerClear   = CLASS_Inherit | CLASS_Abstract | CLASS_NoExport | CLASS_NativeReplication | CLASS_Native,

	/** these flags will be inherited from the base class only for non-intrinsic classes */
	CLASS_ScriptInherit		= CLASS_Inherit | CLASS_EditInlineNew | CLASS_CollapseCategories,
	//@}

	CLASS_AllFlags			= 0xFFFFFFFF,
};

//
// Flags associated with each property in a class, overriding the
// property's default behavior.
//

// For compilers that don't support 64 bit enums.
#define	CPF_Edit					DECLARE_UINT64(0x0000000000000001)		// Property is user-settable in the editor.
#define	CPF_Const					DECLARE_UINT64(0x0000000000000002)		// Actor's property always matches class's default actor property.
#define CPF_Input					DECLARE_UINT64(0x0000000000000004)		// Variable is writable by the input system.
#define CPF_ExportObject			DECLARE_UINT64(0x0000000000000008)		// Object can be exported with actor.
#define CPF_OptionalParm			DECLARE_UINT64(0x0000000000000010)		// Optional parameter (if CPF_Param is set).
#define CPF_Net						DECLARE_UINT64(0x0000000000000020)		// Property is relevant to network replication.
#define CPF_EditFixedSize			DECLARE_UINT64(0x0000000000000040)		// Indicates that elements of an array can be modified, but its size cannot be changed.
#define CPF_Parm					DECLARE_UINT64(0x0000000000000080)		// Function/When call parameter.
#define CPF_OutParm					DECLARE_UINT64(0x0000000000000100)		// Value is copied out after function call.
#define CPF_SkipParm				DECLARE_UINT64(0x0000000000000200)		// Property is a short-circuitable evaluation function parm.
#define CPF_ReturnParm				DECLARE_UINT64(0x0000000000000400)		// Return value.
#define CPF_CoerceParm				DECLARE_UINT64(0x0000000000000800)		// Coerce args into this function parameter.
#define CPF_Native      			DECLARE_UINT64(0x0000000000001000)		// Property is native: C++ code is responsible for serializing it.
#define CPF_Transient   			DECLARE_UINT64(0x0000000000002000)		// Property is transient: shouldn't be saved, zero-filled at load time.
#define CPF_Config      			DECLARE_UINT64(0x0000000000004000)		// Property should be loaded/saved as permanent profile.
#define CPF_Localized   			DECLARE_UINT64(0x0000000000008000)		// Property should be loaded as localizable text.

#define CPF_EditConst   			DECLARE_UINT64(0x0000000000020000)		// Property is uneditable in the editor.
#define CPF_GlobalConfig			DECLARE_UINT64(0x0000000000040000)		// Load config from base class, not subclass.
#define CPF_Component				DECLARE_UINT64(0x0000000000080000)		// Property containts component references.
#define CPF_AlwaysInit				DECLARE_UINT64(0x0000000000100000)		// Property should never be exported as NoInit	(@todo - this doesn't need to be a property flag...only used during make)
#define CPF_DuplicateTransient		DECLARE_UINT64(0x0000000000200000)		// Property should always be reset to the default value during any type of duplication (copy/paste, binary duplication, etc.)
#define CPF_NeedCtorLink			DECLARE_UINT64(0x0000000000400000)		// Fields need construction/destruction.
#define CPF_NoExport    			DECLARE_UINT64(0x0000000000800000)		// Property should not be exported to the native class header file.
#define	CPF_NoImport				DECLARE_UINT64(0x0000000001000000)		// Property should not be imported when creating an object from text (copy/paste)
#define CPF_NoClear					DECLARE_UINT64(0x0000000002000000)		// Hide clear (and browse) button.
#define CPF_EditInline				DECLARE_UINT64(0x0000000004000000)		// Edit this object reference inline.

#define CPF_EditInlineUse			DECLARE_UINT64(0x0000000010000000)		// EditInline with Use button.
#define CPF_Deprecated  			DECLARE_UINT64(0x0000000020000000)		// Property is deprecated.  Read it from an archive, but don't save it.
#define CPF_DataBinding				DECLARE_UINT64(0x0000000040000000)		// Indicates that this property should be exposed to data stores

#define CPF_RepNotify				DECLARE_UINT64(0x0000000100000000)		// Notify actors when a property is replicated
#define CPF_Interp					DECLARE_UINT64(0x0000000200000000)		// interpolatable property for use with matinee
#define CPF_NonTransactional		DECLARE_UINT64(0x0000000400000000)		// Property isn't transacted

#define CPF_EditorOnly				DECLARE_UINT64(0x0000000800000000)		// Property should only be loaded in the editor
#define CPF_NotForConsole			DECLARE_UINT64(0x0000001000000000)		// Property should not be loaded on console (or be a console cooker commandlet)

//@todo ronp - temporary until we add UArchetypeProperty
#define CPF_ArchetypeProperty		DECLARE_UINT64(0x0000010000000000)		// property should be ignored by archives which have ArIgnoreArchetypeRef set


/** @name Combinations flags */
//@{
#define	CPF_ParmFlags				(CPF_OptionalParm | CPF_Parm | CPF_OutParm | CPF_SkipParm | CPF_ReturnParm | CPF_CoerceParm)
#define	CPF_PropagateFromStruct		(CPF_Const | CPF_Native | CPF_Transient)
#define	CPF_PropagateToArrayInner	(CPF_ExportObject | CPF_EditInline | CPF_EditInlineUse | CPF_Localized | CPF_Component | CPF_Config | CPF_AlwaysInit | CPF_EditConst)

/** the flags that should never be set on interface properties */
#define CPF_InterfaceClearMask		(CPF_NeedCtorLink|CPF_ExportObject)
//@}

/** @name ObjectFlags
 * Flags describing an object instance
 */
//@{
typedef QWORD EObjectFlags;													/** @warning: mirrored in UnName.h */
// For compilers that don't support 64 bit enums.
// unused							DECLARE_UINT64(0x0000000000000001)
#define RF_InSingularFunc			DECLARE_UINT64(0x0000000000000002)		// In a singular function.
#define RF_StateChanged				DECLARE_UINT64(0x0000000000000004)		// Object did a state change.
#define RF_DebugPostLoad			DECLARE_UINT64(0x0000000000000008)		// For debugging PostLoad calls.
#define RF_DebugSerialize			DECLARE_UINT64(0x0000000000000010)		// For debugging Serialize calls.
#define RF_DebugFinishDestroyed		DECLARE_UINT64(0x0000000000000020)		// For debugging FinishDestroy calls.
#define RF_EdSelected				DECLARE_UINT64(0x0000000000000040)		// Object is selected in one of the editors browser windows.
#define RF_ZombieComponent			DECLARE_UINT64(0x0000000000000080)		// This component's template was deleted, so should not be used.
#define RF_Protected				DECLARE_UINT64(0x0000000000000100)		// Property is protected (may only be accessed from its owner class or subclasses)
#define RF_ClassDefaultObject		DECLARE_UINT64(0x0000000000000200)		// this object is its class's default object
#define RF_ArchetypeObject			DECLARE_UINT64(0x0000000000000400)		// this object is a template for another object - treat like a class default object
#define RF_ForceTagExp				DECLARE_UINT64(0x0000000000000800)		// Forces this object to be put into the export table when saving a package regardless of outer
#define RF_TokenStreamAssembled		DECLARE_UINT64(0x0000000000001000)		// Set if reference token stream has already been assembled
#define RF_MisalignedObject			DECLARE_UINT64(0x0000000000002000)		// Object's size no longer matches the size of its C++ class (only used during make, for native classes whose properties have changed)
#define RF_RootSet					DECLARE_UINT64(0x0000000000004000)		// Object's size no longer matches the size of its C++ class (only used during make, for native classes whose properties have changed)
#define RF_BeginDestroyed			DECLARE_UINT64(0x0000000000008000)		// BeginDestroy has been called on the object.
#define RF_FinishDestroyed			DECLARE_UINT64(0x0000000000010000)		// BeginDestroy has been called on the object.
#define RF_DebugBeginDestroyed		DECLARE_UINT64(0x0000000000020000)		// Whether object is rooted as being part of the root set (garbage collection)
#define RF_MarkedByCooker			DECLARE_UINT64(0x0000000000040000)		// Marked by content cooker.
#define RF_LocalizedResource		DECLARE_UINT64(0x0000000000080000)		// Whether resource object is localized.
#define RF_InitializedProps			DECLARE_UINT64(0x0000000000100000)		// whether InitProperties has been called on this object
// unused							DECLARE_UINT64(0x0000000000200000)
// unused							DECLARE_UINT64(0x0000000000400000)
// unused							DECLARE_UINT64(0x0000000000800000)
// unused							DECLARE_UINT64(0x0000000001000000)
// unused							DECLARE_UINT64(0x0000000002000000)
// unused							DECLARE_UINT64(0x0000000004000000)
// unused							DECLARE_UINT64(0x0000000008000000)
// unused							DECLARE_UINT64(0x0000000010000000)
// unused							DECLARE_UINT64(0x0000000020000000)
// unused							DECLARE_UINT64(0x0000000040000000)
#define RF_Saved					DECLARE_UINT64(0x0000000080000000)		// Object has been saved via SavePackage. Temporary.
#define	RF_Transactional			DECLARE_UINT64(0x0000000100000000)		// Object is transactional.
#define	RF_Unreachable				DECLARE_UINT64(0x0000000200000000)		// Object is not reachable on the object graph.
#define RF_Public					DECLARE_UINT64(0x0000000400000000)		// Object is visible outside its package.
#define	RF_TagImp					DECLARE_UINT64(0x0000000800000000)		// Temporary import tag in load/save.
#define RF_TagExp					DECLARE_UINT64(0x0000001000000000)		// Temporary export tag in load/save.
#define	RF_Obsolete					DECLARE_UINT64(0x0000002000000000)		// Object marked as obsolete and should be replaced.
#define	RF_TagGarbage				DECLARE_UINT64(0x0000004000000000)		// Check during garbage collection.
#define RF_DisregardForGC			DECLARE_UINT64(0x0000008000000000)		// Object is being disregard for GC as its static and itself and all references are always loaded.
#define	RF_PerObjectLocalized		DECLARE_UINT64(0x0000010000000000)		// Object is localized by instance name, not by class.
#define RF_NeedLoad					DECLARE_UINT64(0x0000020000000000)		// During load, indicates object needs loading.
#define RF_AsyncLoading				DECLARE_UINT64(0x0000040000000000)		// Object is being asynchronously loaded.
#define	RF_NeedPostLoadSubobjects	DECLARE_UINT64(0x0000080000000000)		// During load, indicates that the object still needs to instance subobjects and fixup serialized component references
#define RF_Suppress					DECLARE_UINT64(0x0000100000000000)		// @warning: Mirrored in UnName.h. Suppressed log name.
#define RF_InEndState				DECLARE_UINT64(0x0000200000000000)		// Within an EndState call.
#define RF_Transient				DECLARE_UINT64(0x0000400000000000)		// Don't save object.
#define RF_Cooked					DECLARE_UINT64(0x0000800000000000)		// Whether the object has already been cooked
#define RF_LoadForClient			DECLARE_UINT64(0x0001000000000000)		// In-file load for client.
#define RF_LoadForServer			DECLARE_UINT64(0x0002000000000000)		// In-file load for client.
#define RF_LoadForEdit				DECLARE_UINT64(0x0004000000000000)		// In-file load for client.
#define RF_Standalone				DECLARE_UINT64(0x0008000000000000)		// Keep object around for editing even if unreferenced.
#define RF_NotForClient				DECLARE_UINT64(0x0010000000000000)		// Don't load this object for the game client.
#define RF_NotForServer				DECLARE_UINT64(0x0020000000000000)		// Don't load this object for the game server.
#define RF_NotForEdit				DECLARE_UINT64(0x0040000000000000)		// Don't load this object for the editor.
// unused							DECLARE_UINT64(0x0080000000000000)
#define RF_NeedPostLoad				DECLARE_UINT64(0x0100000000000000)		// Object needs to be postloaded.
#define RF_HasStack					DECLARE_UINT64(0x0200000000000000)		// Has execution stack.
#define RF_Native					DECLARE_UINT64(0x0400000000000000)		// Native (UClass only).
#define RF_Marked					DECLARE_UINT64(0x0800000000000000)		// Marked (for debugging).
#define RF_ErrorShutdown			DECLARE_UINT64(0x1000000000000000)		// ShutdownAfterError called.
#define RF_PendingKill				DECLARE_UINT64(0x2000000000000000)		// Objects that are pending destruction (invalid for gameplay but valid objects)
// unused							DECLARE_UINT64(0x4000000000000000)
// unused							DECLARE_UINT64(0x8000000000000000)


#define RF_ContextFlags				(RF_NotForClient | RF_NotForServer | RF_NotForEdit) // All context flags.
#define RF_LoadContextFlags			(RF_LoadForClient | RF_LoadForServer | RF_LoadForEdit) // Flags affecting loading.
#define RF_Load  					(RF_ContextFlags | RF_LoadContextFlags | RF_Public | RF_Standalone | RF_Native | RF_Obsolete| RF_Protected | RF_Transactional | RF_HasStack | RF_PerObjectLocalized | RF_ClassDefaultObject | RF_ArchetypeObject | RF_LocalizedResource ) // Flags to load from Unrealfiles.
#define RF_Keep						(RF_Native | RF_Marked | RF_PerObjectLocalized | RF_MisalignedObject | RF_DisregardForGC | RF_RootSet | RF_LocalizedResource ) // Flags to persist across loads.
#define RF_ScriptMask				(RF_Transactional | RF_Public | RF_Transient | RF_NotForClient | RF_NotForServer | RF_NotForEdit | RF_Standalone) // Script-accessible flags.
#define RF_UndoRedoMask				(RF_PendingKill)									// Undo/ redo will store/ restore these
#define RF_PropagateToSubObjects	(RF_Public|RF_ArchetypeObject|RF_Transactional)		// Sub-objects will inherit these flags from their SuperObject.

#define RF_AllFlags					DECLARE_UINT64(0xFFFFFFFFFFFFFFFF)
//@}

//NEW: trace facility
enum ProcessEventType
{
	PE_Toggle,
	PE_Suppress,
	PE_Enable,
};

/**
 * Determines which ticking group an Actor/Component belongs to
 * @see Object.uc for original definition
 */
enum ETickingGroup
{
	/**
	 * Any item that needs to be updated before asynchronous work is done
	 */
	TG_PreAsyncWork,
	/**
	 * Any item that can be run in parallel of our async work
	 */
	TG_DuringAsyncWork,
	/**
	 * Any item that needs the async work to be done before being updated
	 */
	TG_PostAsyncWork
};

/*----------------------------------------------------------------------------
	Core types.
----------------------------------------------------------------------------*/

//
// Globally unique identifier.
//
class FGuid
{
public:
	DWORD A,B,C,D;
	FGuid()
	{}
	FGuid( DWORD InA, DWORD InB, DWORD InC, DWORD InD )
	: A(InA), B(InB), C(InC), D(InD)
	{}

	/**
	 * Returns whether this GUID is valid or not. We reserve an all 0 GUID to represent "invalid".
	 *
	 * @return TRUE if valid, FALSE otherwise
	 */
	UBOOL IsValid() const
	{
		return (A | B | C | D) != 0;
	}

	/** Invalidates the GUID. */
	void Invalidate()
	{
		A = B = C = D = 0;
	}

	friend UBOOL operator==(const FGuid& X, const FGuid& Y)
	{
		return ((X.A ^ Y.A) | (X.B ^ Y.B) | (X.C ^ Y.C) | (X.D ^ Y.D)) == 0;
	}
	friend UBOOL operator!=(const FGuid& X, const FGuid& Y)
	{
		return ((X.A ^ Y.A) | (X.B ^ Y.B) | (X.C ^ Y.C) | (X.D ^ Y.D)) != 0;
	}
	DWORD& operator[]( INT Index )
	{
		checkSlow(Index>=0);
		checkSlow(Index<4);
		switch(Index)
		{
		case 0: return A;
		case 1: return B;
		case 2: return C;
		case 3: return D;
		}

		return A;
	}
	const DWORD& operator[]( INT Index ) const
	{
		checkSlow(Index>=0);
		checkSlow(Index<4);
		switch(Index)
		{
		case 0: return A;
		case 1: return B;
		case 2: return C;
		case 3: return D;
		}

		return A;
	}
	friend FArchive& operator<<( FArchive& Ar, FGuid& G )
	{
		return Ar << G.A << G.B << G.C << G.D;
	}
	FString String() const
	{
		return FString::Printf( TEXT("%08X%08X%08X%08X"), A, B, C, D );
	}
	friend DWORD GetTypeHash(const FGuid& Guid)
	{
		return appMemCrc(&Guid,sizeof(FGuid));
	}
};

/*----------------------------------------------------------------------------
	Core macros.
----------------------------------------------------------------------------*/

// Special canonical package for FindObject, ParseObject.
#define ANY_PACKAGE ((UPackage*)-1)

// Special prefix for default objects (the UObject in a UClass containing the default values, etc)
#define DEFAULT_OBJECT_PREFIX TEXT("Default__")

// Special prefix for default objects (the UObject in a UClass containing the default values, etc)
#define TRASHCAN_DIRECTORY_NAME TEXT("__Trashcan")

// Define private default constructor.
#define NO_DEFAULT_CONSTRUCTOR(cls) \
	protected: cls() {} public:

// Verify the a class definition and C++ definition match up.
#define VERIFY_CLASS_OFFSET(Pre,ClassName,Member) \
    {for( TFieldIterator<UProperty,CLASS_IsAUProperty> It( FindObjectChecked<UClass>( Pre##ClassName::StaticClass()->GetOuter(), TEXT(#ClassName) ) ); It; ++It ) \
		if( appStricmp(*It->GetName(),TEXT(#Member))==0 ) \
			if( It->Offset != STRUCT_OFFSET(Pre##ClassName,Member) ) \
				appErrorf( NAME_Error, TEXT("Class %s Member %s problem: Script=%i C++=%i" ), TEXT(#ClassName), TEXT(#Member), It->Offset, STRUCT_OFFSET(Pre##ClassName,Member) );}

// Verify that C++ and script code agree on the size of a class.
#define VERIFY_CLASS_SIZE(ClassName) \
	check(sizeof(ClassName)==Align(ClassName::StaticClass()->GetPropertiesSize(),ClassName::StaticClass()->GetMinAlignment()));

// Verify a class definition and C++ definition match up (don't assert).
#define VERIFY_CLASS_OFFSET_NODIE(Pre,ClassName,Member) \
{ \
	for( TFieldIterator<UProperty,CLASS_IsAUProperty> It( FindObjectChecked<UClass>( Pre##ClassName::StaticClass()->GetOuter(), TEXT(#ClassName) ) ); It; ++It ) \
	if( appStricmp(*It->GetName(),TEXT(#Member))==0 ) { \
		if( It->Offset != STRUCT_OFFSET(Pre##ClassName,Member) ) \
        { \
			debugf( NAME_Error, TEXT("VERIFY: Class %s Member %s problem: Script=%i C++=%i\n" ), TEXT(#ClassName), TEXT(#Member), It->Offset, STRUCT_OFFSET(Pre##ClassName,Member) ); \
			Mismatch = TRUE; \
		} \
	} \
}

// Verify that C++ and script code agree on the size of a class (don't assert).
#define VERIFY_CLASS_SIZE_NODIE(ClassName) \
	if (sizeof(ClassName)!=Align(ClassName::StaticClass()->GetPropertiesSize(),ClassName::StaticClass()->GetMinAlignment())) \
    { \
		debugf( NAME_Error, TEXT("VERIFY: Class %s size problem; Script=%i C++=%i MinAlignment=%i" ), TEXT(#ClassName), (int) ClassName::StaticClass()->GetPropertiesSize(), sizeof(ClassName), ClassName::StaticClass()->GetMinAlignment()); \
		Mismatch = TRUE; \
	}


// Declare the base UObject class.
typedef const TCHAR* (*StaticConfigFunction)();

#define DECLARE_BASE_CLASS( TClass, TSuperClass, TStaticFlags, TStaticPlatformFlags, TPackage ) \
public: \
	friend void LoadAllNativeScriptPackages( UBOOL bExcludeGamePackages ); \
	friend void CheckNativeClassSizes(); \
	/* Identification */ \
	enum {StaticClassFlags=TStaticFlags}; \
	enum {StaticPlatformFlags=TStaticPlatformFlags}; \
	private: \
	static UClass* PrivateStaticClass; \
	public: \
	typedef TSuperClass Super;\
	typedef TClass ThisClass;\
	static UClass* GetPrivateStaticClass##TClass( const TCHAR* Package ); \
	static void InitializePrivateStaticClass##TClass(); \
	static UClass* StaticClass() \
	{ \
		if (!PrivateStaticClass) \
		{ \
			PrivateStaticClass = GetPrivateStaticClass##TClass( TEXT(#TPackage) ); \
			InitializePrivateStaticClass##TClass(); \
		} \
		return PrivateStaticClass; \
	} \
	virtual UBOOL HasUniqueStaticConfigName() const; \
	virtual UBOOL HasParentClassChanged() const; \
	void* operator new( const size_t InSize, UObject* InOuter=(UObject*)GetTransientPackage(), FName InName=NAME_None, EObjectFlags InSetFlags=0 ) \
		{ return StaticAllocateObject( StaticClass(), InOuter, InName, InSetFlags ); } \
	void* operator new( const size_t InSize, EInternal* InMem ) \
		{ return (void*)InMem; }

#define DECLARE_SERIALIZER_AND_DTOR( TClass ) \
	friend FArchive &operator<<( FArchive& Ar, TClass*& Res ) \
		{ return Ar << *(UObject**)&Res; } \
	protected: \
	virtual ~TClass() \
		{ ConditionalDestroy(); } \
	public:

// Declare a concrete class.
#define DECLARE_CLASS( TClass, TSuperClass, TStaticFlags, TPackage ) \
	DECLARE_BASE_CLASS( TClass, TSuperClass, TStaticFlags, 0, TPackage ) \
	DECLARE_SERIALIZER_AND_DTOR( TClass ) \
	static void InternalConstructor( void* X ) \
		{ new( (EInternal*)X )TClass; } \

#define DECLARE_PLATFORM_CLASS( TClass, TSuperClass, TStaticFlags, TStaticPlatformFlags, TPackage ) \
	DECLARE_BASE_CLASS( TClass, TSuperClass, TStaticFlags, TStaticPlatformFlags, TPackage ) \
	DECLARE_SERIALIZER_AND_DTOR( TClass ) \
	static void InternalConstructor( void* X ) \
		{ new( (EInternal*)X )TClass; } \

// Declare an abstract class.
#if CHECK_PUREVIRTUALS

#define DECLARE_ABSTRACT_CLASS( TClass, TSuperClass, TStaticFlags, TPackage ) \
	DECLARE_BASE_CLASS( TClass, TSuperClass, TStaticFlags | CLASS_Abstract, 0, TPackage ) \
	DECLARE_SERIALIZER_AND_DTOR( TClass )

#define DECLARE_ABSTRACT_PLATFORM_CLASS( TClass, TSuperClass, TStaticFlags, TStaticPlatformFlags, TPackage ) \
	DECLARE_BASE_CLASS( TClass, TSuperClass, TStaticFlags | CLASS_Abstract, TStaticPlatformFlags, TPackage ) \
	DECLARE_SERIALIZER_AND_DTOR( TClass )

#else

#define DECLARE_ABSTRACT_CLASS( TClass, TSuperClass, TStaticFlags, TPackage ) \
	DECLARE_CLASS( TClass, TSuperClass, TStaticFlags | CLASS_Abstract, TPackage )

#define DECLARE_ABSTRACT_PLATFORM_CLASS( TClass, TSuperClass, TStaticFlags, TStaticPlatformFlags, TPackage ) \
	DECLARE_PLATFORM_CLASS( TClass, TSuperClass, TStaticFlags | CLASS_Abstract, TStaticPlatformFlags, TPackage )

#endif	// CHECK_PUREVIRTUALS

// Declare that objects of class being defined reside within objects of the specified class.
#define DECLARE_WITHIN( TWithinClass ) \
	typedef class TWithinClass WithinClass; \
	TWithinClass* GetOuter##TWithinClass() const { return (TWithinClass*)GetOuter(); }

// Register a class at startup time.

#define IMPLEMENT_CLASS(TClass) \
	UClass* TClass::PrivateStaticClass = NULL; \
	UClass* TClass::GetPrivateStaticClass##TClass( const TCHAR* Package ) \
	{ \
		UClass* ReturnClass; \
		ReturnClass = ::new UClass \
		( \
			EC_StaticConstructor, \
			sizeof(TClass), \
			StaticClassFlags, \
			StaticPlatformFlags, \
			TEXT(#TClass)+1, \
			Package, \
			StaticConfigName(), \
			RF_Public | RF_Standalone | RF_Transient | RF_Native | RF_RootSet | RF_DisregardForGC, \
			(void(*)(void*))TClass::InternalConstructor, \
			(void(UObject::*)())&TClass::StaticConstructor, \
			(void(UObject::*)())&TClass::InitializeIntrinsicPropertyValues \
		); \
		check(ReturnClass); \
		return ReturnClass; \
	} \
	/* Called from ::StaticClass after GetPrivateStaticClass */ \
	void TClass::InitializePrivateStaticClass##TClass() \
	{ \
		/* No recursive ::StaticClass calls allowed. Setup extras. */ \
		if (TClass::Super::StaticClass() != TClass::PrivateStaticClass) \
			TClass::PrivateStaticClass->SuperField = TClass::Super::StaticClass(); \
		else \
			TClass::PrivateStaticClass->SuperField = NULL; \
		TClass::PrivateStaticClass->ClassWithin = TClass::WithinClass::StaticClass(); \
		TClass::PrivateStaticClass->SetClass(UClass::StaticClass()); \
		/* Perform UObject native registration. */ \
		if( TClass::PrivateStaticClass->GetInitialized() && TClass::PrivateStaticClass->GetClass()==TClass::PrivateStaticClass->StaticClass() ) \
			TClass::PrivateStaticClass->Register(); \
	} \
	UBOOL TClass::HasUniqueStaticConfigName() const \
	{ \
		const StaticConfigFunction ThisConfigFunction = &TClass::StaticConfigName; \
		const StaticConfigFunction ParentConfigFunction = &TClass::Super::StaticConfigName; \
		return ThisConfigFunction == ParentConfigFunction; \
	} \
	UBOOL TClass::HasParentClassChanged() const \
	{ \
		const UClass* CurrentParentClass = GetClass()->GetSuperClass(); \
		const UClass* StaticParentClass = TClass::Super::StaticClass(); \
		return CurrentParentClass != StaticParentClass; \
	} \


/*-----------------------------------------------------------------------------
	ERenameFlags.

	Options to the UObject::Rename function, bit flag
-----------------------------------------------------------------------------*/

typedef DWORD ERenameFlags;

#define REN_None				(0x0000)
#define REN_ForceNoResetLoaders	(0x0001) // Rename won't call ResetLoaders - most likely you should never specify this option (unless you are renaming a UPackage possibly)
#define REN_Test				(0x0002) // Just test to make sure that the rename is guaranteed to succeed if an non test rename immediately follows

/*-----------------------------------------------------------------------------
	Misc.
-----------------------------------------------------------------------------*/

typedef void* FPointer;

typedef void (*FAsyncCompletionCallback)( UObject* LinkerRoot, void* CallbackUserData );

/** all information required to call a specified callback function when async loading a package completes */
struct FAsyncCompletionCallbackInfo
{
	/** the function to call */
	FAsyncCompletionCallback Callback;
	/** user data to pass into the callback */
	void* UserData;

	FAsyncCompletionCallbackInfo(FAsyncCompletionCallback InCallback, void* InUserData)
	: Callback(InCallback), UserData(InUserData)
	{}

	UBOOL operator==(const FAsyncCompletionCallbackInfo& Other)
	{
		return (Callback == Other.Callback && UserData == Other.UserData);
	}
};

/*-----------------------------------------------------------------------------
	Garbage collection callbacks.
-----------------------------------------------------------------------------*/

/** Typedef function pointer for garbage collection callbacks */
typedef void (*FGCCallback)( void );

#define IMPLEMENT_PRE_GARBAGE_COLLECTION_CALLBACK( DummyName, Function, Index )		\
	static INT GCCallback##DummyName = GRegisterPreGCCallback( Function, Index );

#define IMPLEMENT_POST_GARBAGE_COLLECTION_CALLBACK( DummyName, Function, Index )		\
	static INT GCCallback##DummyName = GRegisterPostGCCallback( Function, Index );

/**
 * Registers passed in callback to be called before UObject::CollectGarbage executes. The order
 * of operation is important which is why we pass in an index.
 *
 * @param	Callback	Callback to register
 * @param	Index		Index to register at.
 * @return index into callback array this entry has been assigned
 */
INT GRegisterPreGCCallback( FGCCallback Callback, INT Index );

/**
 * Registers passed in callback to be called before UObject::CollectGarbage returns. The order
 * of operation is important which is why we pass in an index.
 *
 * @param	Callback	Callback to register
 * @param	Index		Index to register at.
 * @return index into callback array this entry has been assigned
 */
INT GRegisterPostGCCallback( FGCCallback Callback, INT Index );

/**
 * Helper enum to ensure uniqueness of indices.
 */
enum EPreGCCallbacks
{
	GCCB_PRE_FlushAsyncLoading						= 0,
	GCCB_PRE_PrepareLevelsForGC,
};

/**
 * Helper enum to ensure uniqueness of indices.
 */
enum EPostGCCallbacks
{
	GCCB_POST_VerifyLevelsGotRemovedByGC			= 0,
	GCCB_POST_VerifyNoUnreachableActorsReferenced,
};

/** Array of callbacks called first thing in UObject::CollectGarbage. */
extern FGCCallback	GPreGarbageCollectionCallbacks[10];

/** Array of callbacks called last thing in UObject::CollectGarbage. */
extern FGCCallback	GPostGarbageCollectionCallbacks[10];

/*-----------------------------------------------------------------------------
	UObject.
-----------------------------------------------------------------------------*/

namespace UE3
{
	/**
	 * Controls how calls to LoadConfig() should be propagated
	 */
	enum ELoadConfigPropagationFlags
	{
		LCPF_None					=	0x0,
		/**
		 * Indicates that the object should read ini values from each section up its class's hierarchy chain;
		 * Useful when calling LoadConfig on an object after it has already been initialized against its archetype
		 */
		LCPF_ReadParentSections		=	0x1,

		/**
		 * Indicates that LoadConfig() should be also be called on the class default objects for all children of the original class.
		 */
		LCPF_PropagateToChildDefaultObjects		=	0x2,

		/**
		 * Indicates that LoadConfig() should be called on all instances of the original class.
		 */
		LCPF_PropagateToInstances	=	0x4,

		/**
		 * Indicates that this object is reloading its config data
		 */
		LCPF_ReloadingConfigData	=	0x8,

		// Combination flags
		LCPF_PersistentFlags		=	LCPF_ReloadingConfigData,
	};
}

/**
 * The object hash size to use
 *
 * NOTE: This must be power of 2 so that (size - 1) turns on all bits!
 */
#define OBJECT_HASH_BINS 8192

/**
 * Calculates the object's hash just using the object's name index
 *
 * @param ObjName the object's name to use the index of
 */
FORCEINLINE INT GetObjectHash(FName ObjName)
{
	return (ObjName.GetIndex() ^ ObjName.GetNumber()) & (OBJECT_HASH_BINS - 1);
}

/**
 * Calculates the object's hash just using the object's name index
 * XORed with the outer. Yields much better spread in the hash
 * buckets, but requires knowledge of the outer, which isn't available
 * in all cases.
 *
 * @param ObjName the object's name to use the index of
 * @param Outer the object's outer pointer treated as an INT
 */
FORCEINLINE INT GetObjectOuterHash(FName ObjName,INT Outer)
{
	return ((ObjName.GetIndex() ^ ObjName.GetNumber()) ^ Outer) & (OBJECT_HASH_BINS - 1);
}

//
// The base class of all objects.
//
class UObject
{
	// Declarations.
	DECLARE_BASE_CLASS(UObject,UObject,CLASS_Abstract|CLASS_NoExport,0,Core)
	typedef UObject WithinClass;
	static const TCHAR* StaticConfigName() {return TEXT("Engine");}

	// Friends.
	friend class FObjectIterator;
	friend class TSelectedObjectIterator;
	friend class ULinkerLoad;
	friend class ULinkerSave;
	friend class UPackageMap;
	friend class FArchiveRealtimeGC;
	friend struct FObjectImport;
	friend struct FObjectExport;
	friend class UWorld;
	friend struct FAsyncPackage;
	friend class FEngineLoop;
	friend class UPackage;

	friend class FArchiveFindCulprit;
	
	friend const TCHAR* DebugFName(UObject*);

private:
	// Internal per-object variables.

	/** Index of object into GObjObjects array. */
	INT								Index;

	/** Flags used to track and report various object states. This needs to be 8 byte aligned! */
	EObjectFlags					ObjectFlags;

	/** Next object in this hash bin. */
	UObject*						HashNext;

	/** Next object in the hash bin that includes outers */
	UObject*						HashOuterNext;

	/** Main script execution stack. */
	FStateFrame*					StateFrame;

	/**
	 * Linker that contains the FObjectExport resource corresponding to
	 * this object.  NULL if this object is native only (i.e. never stored
	 * in an Unreal package), or if this object has been detached from its
	 * linker, for e.g. renaming operations, saving the package, etc.
	 */
	ULinkerLoad*					_Linker;

	/**
	 * Index into the linker's ExportMap array for the FObjectExport resource
	 * corresponding to this object.
	 */
	PTRINT							_LinkerIndex;

	/** index into Outermost's NetObjects array, used for replicating references to this object
	 * INDEX_None means references to this object cannot be replicated
	 */
	INT								NetIndex;

	/** Object this object resides in. */
	UObject*						Outer;

	/** Name of the object. */
	FName							Name;

	/** Class the object belongs to. */
	UClass*							Class;

	/**
	 * Object this object is based on - defaults from ObjectArchetype
	 * are copied onto this object at creation, prior to loading this
	 * object's data from disk.
	 */
	UObject*						ObjectArchetype;

	// Private system wide variables.

	/** Whether initialized.												*/
	static UBOOL					GObjInitialized;
	/** Registration disable.												*/
	static UBOOL					GObjNoRegister;
	/** Count for BeginLoad multiple loads.									*/
	static INT						GObjBeginLoadCount;
	/** ProcessRegistrants entry counter.									*/
	static INT						GObjRegisterCount;
	/** Imports for EndLoad optimization.									*/
	static INT						GImportCount;
	/** Forced exports for EndLoad optimization.							*/
	static INT						GForcedExportCount;
	/** Object hash.														*/
	static UObject*					GObjHash[OBJECT_HASH_BINS];
	/** Object hash that also uses the outer								*/
	static UObject*					GObjHashOuter[OBJECT_HASH_BINS];
	/** Objects to automatically register.									*/
	static UObject*					GAutoRegister;
	/** Objects that might need preloading.									*/
	static TArray<UObject*>			GObjLoaded;
	/** Objects that have been constructed during async loading phase.		*/
	static TArray<UObject*>			GObjConstructedDuringAsyncLoading;
	/** List of all objects.												*/
	static TArray<UObject*>			GObjObjects;
	/** Available object indices.											*/
	static TArray<INT>				GObjAvailable;	
	/** Array of loaders.													*/
	static TArray<ULinkerLoad*>		GObjLoaders;
	/** Transient package.													*/
	static UPackage*				GObjTransientPkg;		
	/** Language.															*/
	static TCHAR					GObjCachedLanguage[32]; 
	/**  Registrants during ProcessRegistrants call.						*/
	static TArray<UObject*>			GObjRegistrants;
	/** Array of packages that are being preloaded							*/
	static TArray<struct FAsyncPackage>	GObjAsyncPackages;
	/** Language.															*/
	static TCHAR					GLanguage[64];
	/** Whether incremental object purge is in progress						*/
	static UBOOL					GObjIncrementalPurgeIsInProgress;	
	/** Whether FinishDestroy has already been routed to all unreachable objects. */
	static UBOOL					GObjFinishDestroyHasBeenRoutedToAllObjects;
	/** Whether we need to purge objects or not.							*/
	static UBOOL					GObjPurgeIsRequired;
	/** Current object index for incremental purge.							*/
	static INT						GObjCurrentPurgeObjectIndex;
	/** First index into objects array taken into account for GC.			*/
	static INT						GObjFirstGCIndex;
	/** Index pointing to last object created in range disregarded for GC.	*/
	static INT						GObjLastNonGCIndex;
	/** Size in bytes of pool for objects disregarded for GC.				*/
	static INT						GPermanentObjectPoolSize;
	/** Begin of pool for objects disregarded for GC.						*/
	static BYTE*					GPermanentObjectPool;
	/** Current position in pool for objects disregarded for GC.			*/
	static BYTE*					GPermanentObjectPoolTail;


	// Private functions.
	void AddObject( INT Index );
	void HashObject();
	void UnhashObject();

	// Private systemwide functions.
	static ULinkerLoad* GetLoader( INT i );
	static UBOOL ResolveName( UObject*& Outer, FString& Name, UBOOL Create, UBOOL Throw );
	static void SafeLoadError( UObject* Outer, DWORD LoadFlags, const TCHAR* Error, const TCHAR* Fmt, ... );

	/**
	 * Incrementally purge garbage by deleting all unreferenced objects after routing Destroy.
	 *
	 * Calling code needs to be EXTREMELY careful when and how to call this function as 
	 * RF_Unreachable cannot change on any objects unless any pending purge has completed!
	 *
	 * @param	bUseTimeLimit	whether the time limit parameter should be used
	 * @param	TimeLimit		soft time limit for this function call
	 */
	static void IncrementalPurgeGarbage( UBOOL bUseTimeLimit, FLOAT TimeLimit = 0.002 );

#ifdef __GNUC__
	// GCC doesn't like operator new being private
public:
#endif
	// Private delete function so only GC can delete objects.
	void operator delete( void* Object, size_t Size );

public:
	// Constructors.
	UObject();
	UObject( const UObject& Src );
	UObject( ENativeConstructor, UClass* InClass, const TCHAR* InName, const TCHAR* InPackageName, EObjectFlags InFlags );
	UObject( EStaticConstructor, const TCHAR* InName, const TCHAR* InPackageName, EObjectFlags InFlags );

	UObject& operator=( const UObject& );

	/**
	 * Perform any static initialization, such as adding UProperties to the class (for intrinsic classes), or initializing static variable values.
	 * It is not recommended to use this function to initialize member property values, since those values will be overwritten when the class's default
	 * object is re-initialized against its archetype.  Use InitializeInstrinsicPropertyValues instead.
	 */
	void StaticConstructor();

	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();

	static void InternalConstructor( void* X )
		{ new( (EInternal*)X )UObject; }

	// Destructors.
	virtual ~UObject();

	FORCEINLINE const FName& GetPureName(void) const
	{
		return this->Name;
	}

	//==========================================
	// UObject interface.
	//==========================================	
	/**
	 * Called from within SavePackage on the passed in base/ root. The return value of this function will be passed to
	 * PostSaveRoot. This is used to allow objects used as base to perform required actions before saving and cleanup
	 * afterwards.
	 * @param Filename: Name of the file being saved to (includes path)
	 *
	 * @return	Whether PostSaveRoot needs to perform internal cleanup
	 */
	virtual UBOOL PreSaveRoot(const TCHAR* Filename)
	{
		return TRUE;
	}
	/**
	 * Called from within SavePackage on the passed in base/ root. This function is being called after the package
	 * has been saved and can perform cleanup.
	 *
	 * @param	bCleanupIsRequired	Whether PreSaveRoot dirtied state that needs to be cleaned up
	 */
	virtual void PostSaveRoot( UBOOL bCleanupIsRequired ) 
	{
	}
	/**
	 * Presave function. Gets called once before an object gets serialized for saving. This function is necessary
	 * for save time computation as Serialize gets called three times per object from within UObject::SavePackage.
	 *
	 * @warning: Objects created from within PreSave will NOT have PreSave called on them!!!
	 */
	virtual void PreSave() 
	{
	}
	/**
	 * Note that the object has been modified.  If we are currently recording into the 
	 * transaction buffer (undo/redo), save a copy of this object into the buffer and 
	 * marks the package as needing to be saved.
	 *
	 * @param	bAlwaysMarkDirty	if TRUE, marks the package dirty even if we aren't
	 *								currently recording an active undo/redo transaction
	 */
	virtual void Modify( UBOOL bAlwaysMarkDirty=FALSE );
	virtual void PostLoad();

	/**
	 * Called before destroying the object.  This is called immediately upon deciding to destroy the object, to allow the object to begin an
	 * asynchronous cleanup process.
	 */
	virtual void BeginDestroy();

	/**
	 * Called to check if the object is ready for FinishDestroy.  This is called after BeginDestroy to check the completion of the
	 * potentially asynchronous object cleanup.
	 * @return True if the object's asynchronous cleanup has completed and it is ready for FinishDestroy to be called.
	 */
	virtual UBOOL IsReadyForFinishDestroy() { return TRUE; }

	/**
	 * Changes the linker and linker index to the passed in one. A linker of NULL and linker index of INDEX_NONE
	 * indicates that the object is without a linker.
	 *
	 * @param LinkerLoad	New LinkerLoad object to set
	 * @param LinkerIndex	New LinkerIndex to set
	 */
#if !CONSOLE
 	virtual void SetLinker( ULinkerLoad* LinkerLoad, INT LinkerIndex );
#else
	// Does not need to be virtual on consoles!
	void SetLinker( ULinkerLoad* LinkerLoad, INT LinkerIndex );
#endif

	/**
	 * Called to finish destroying the object.  After UObject::FinishDestroy is called, the object's memory should no longer be accessed.
	 *
	 * note: because ExitProperties() is called here, Super::FinishDestroy() should always be called at the end of your child class's
	 * FinishDestroy() method, rather than at the beginning.
	 */
	virtual void FinishDestroy();

	/** serializes NetIndex from the passed in archive; in a seperate function to share with default object serialization */
	void SerializeNetIndex(FArchive& Ar);

	virtual void Serialize( FArchive& Ar );

	/**
	 * Serializes the unrealscript property data located at Data.  When saving, only saves those properties which differ from the corresponding
	 * value in the specified 'DiffObject' (usually the object's archetype).
	 *
	 * @param	Ar				the archive to use for serialization
	 * @param	DiffObject		the object to use for determining which properties need to be saved (delta serialization);
	 *							if not specified, the ObjectArchetype is used
	 * @param	DefaultsCount	maximum number of bytes to consider for delta serialization; any properties which have an Offset+ElementSize greater
	 *							that this value will NOT use delta serialization when serializing data;
	 *							if not specified, the result of DiffObject->GetClass()->GetPropertiesSize() will be used.
	 */
	virtual void SerializeScriptProperties( FArchive& Ar, UObject* DiffObject=NULL, INT DiffCount=0 ) const;

	/**
	 * Checks the RF_PendingKill flag to see if it is dead but memory still valid
	 */
	virtual UBOOL IsPendingKill() const
	{
		return HasAnyFlags(RF_PendingKill);
	}

	/**
	 * Marks this object as RF_PendingKill.
	 */
	void MarkPendingKill()
	{
		SetFlags( RF_PendingKill );
	}

	virtual void ShutdownAfterError();
	virtual void PreEditChange(UProperty* PropertyAboutToChange);
	virtual void PostEditChange(UProperty* PropertyThatChanged);

	/**
	 * This alternate version of PreEditChange is called when properties inside structs are modified.  The property that was actually modified
	 * is located at the tail of the list.  The head of the list of the UStructProperty member variable that contains the property that was modified.
	 */
	virtual void PreEditChange( class FEditPropertyChain& PropertyAboutToChange );

	/**
	 * This alternate version of PostEditChange is called when properties inside structs are modified.  The property that was actually modified
	 * is located at the tail of the list.  The head of the list of the UStructProperty member variable that contains the property that was modified.
	 */
	virtual void PostEditChange( class FEditPropertyChain& PropertyThatChanged );

	/** Called before applying a transaction to the object.  Default implementation simply calls PreEditChange. */
	virtual void PreEditUndo();

	/** Called after applying a transaction to the object.  Default implementation simply calls PostEditChange. */
	virtual void PostEditUndo();

	virtual void PostRename() {}
	/**
	 * Called after duplication & serialization and before PostLoad. Used to e.g. make sure UStaticMesh's UModel
	 * gets copied as well.
	 */
	virtual void PostDuplicate() {}

	/* === Archetype-to-instance property value propagation methods === */

	/**
	 * Determines whether changes to archetypes of this class should be immediately propagated to instances of this
	 * archetype.
	 *
	 * @param	PropagationManager	if specified, receives a pointer to the object which manages propagation for this archetype.
	 *
	 * @return	TRUE if this object's archetype propagation is managed by another object (usually the case when the object
	 *			is part of a prefab or something); FALSE if this object is a standalone archetype.
	 *
	 */
	virtual UBOOL UsesManagedArchetypePropagation( class UObject** PropagationManager=NULL ) const;

	/**
	 * Called just before a property in this object's archetype is to be modified, prior to serializing this object into
	 * the archetype propagation archive.
	 *
	 * Allows objects to perform special cleanup or preparation before being serialized into an FArchetypePropagationArc
	 * against its archetype. Only called for instances of archetypes, where the archetype has the RF_ArchetypeObject flag.  
	 */
	virtual void PreSerializeIntoPropagationArchive();

	/**
	 * Called just before a property in this object's archetype is to be modified, immediately after this object has been
	 * serialized into the archetype propagation archive.
	 *
	 * Allows objects to perform special cleanup or preparation before being serialized into an FArchetypePropagationArc
	 * against its archetype. Only called for instances of archetypes, where the archetype has the RF_ArchetypeObject flag.  
	 */
	virtual void PostSerializeIntoPropagationArchive();

	/**
	 * Called just after a property in this object's archetype is modified, prior to serializing this object from the archetype
	 * propagation archive.
	 *
	 * Allows objects to perform reinitialization specific to being de-serialized from an FArchetypePropagationArc and
	 * reinitialized against an archetype. Only called for instances of archetypes, where the archetype has the RF_ArchetypeObject flag.  
	 */
	virtual void PreSerializeFromPropagationArchive();

	/**
	 * Called just after a property in this object's archetype is modified, immediately after this object has been de-serialized
	 * from the archetype propagation archive.
	 *
	 * Allows objects to perform reinitialization specific to being de-serialized from an FArchetypePropagationArc and
	 * reinitialized against an archetype. Only called for instances of archetypes, where the archetype has the RF_ArchetypeObject flag.  
	 */
	virtual void PostSerializeFromPropagationArchive();

	/**
	 * Builds a list of objects which have this object in their archetype chain.
	 *
	 * @param	Instances	receives the list of objects which have this one in their archetype chain
	 */
	virtual void GetArchetypeInstances( TArray<UObject*>& Instances );

	/**
	 * Serializes all objects which have this object as their archetype into GMemoryArchive, then recursively calls this function
	 * on each of those objects until the full list has been processed.
	 * Called when a property value is about to be modified in an archetype object. 
	 *
	 * @param	AffectedObjects		the array of objects which have this object in their ObjectArchetype chain and will be affected by the change.
	 *								Objects which have this object as their direct ObjectArchetype are removed from the list once they're processed.
	 */
	virtual void SaveInstancesIntoPropagationArchive( TArray<UObject*>& AffectedObjects );

	/**
	 * De-serializes all objects which have this object as their archetype from the GMemoryArchive, then recursively calls this function
	 * on each of those objects until the full list has been processed.
	 *
	 * @param	AffectedObjects		the array of objects which have this object in their ObjectArchetype chain and will be affected by the change.
	 *								Objects which have this object as their direct ObjectArchetype are removed from the list once they're processed.
	 */
	virtual void LoadInstancesFromPropagationArchive( TArray<UObject*>& AffectedObjects );

	virtual void Register();
	virtual void LanguageChange();

	virtual UBOOL NeedsLoadForClient() const { return !HasAnyFlags(RF_NotForClient); }
	virtual UBOOL NeedsLoadForServer() const { return !HasAnyFlags(RF_NotForServer); }
	virtual UBOOL NeedsLoadForEdit() const { return !HasAnyFlags(RF_NotForEdit); }

	/** returns this object's NetIndex */
	FORCEINLINE INT GetNetIndex()
	{
		return NetIndex;
	}

	//==========================================
	// Systemwide functions.
	//==========================================
private:
	/**
	 * Private internal version of StaticFindObjectFast that allows using 0 exclusion flags.
	 *
	 * @param	Class			The to be found object's class
	 * @param	InOuter			The to be found object's outer
	 * @param	InName			The to be found object's class
	 * @param	ExactClass		Whether to require an exact match with the passed in class
	 * @param	AnyPackage		Whether to look in any package
	 * @param	ExclusiveFlags	Ignores objects that contain any of the specified exclusive flags
	 * @return	Returns a pointer to the found object or NULL if none could be found
	 */
	static UObject* StaticFindObjectFastInternal( UClass* Class, UObject* InOuter, FName InName, UBOOL ExactClass=0, UBOOL AnyPackage=0, EObjectFlags ExclusiveFlags=0 );

	/** sets the NetIndex associated with this object for network replication */
	void SetNetIndex(INT InNetIndex);
public:
	/**
	 * Fast version of StaticFindObject that relies on the passed in FName being the object name
	 * without any group/ package qualifiers.
	 *
	 * @param	Class			The to be found object's class
	 * @param	InOuter			The to be found object's outer
	 * @param	InName			The to be found object's class
	 * @param	ExactClass		Whether to require an exact match with the passed in class
	 * @param	AnyPackage		Whether to look in any package
	 * @param	ExclusiveFlags	Ignores objects that contain any of the specified exclusive flags
	 * @return	Returns a pointer to the found object or NULL if none could be found
	 */
	static UObject* StaticFindObjectFast( UClass* Class, UObject* InOuter, FName InName, UBOOL ExactClass=0, UBOOL AnyPackage=0, EObjectFlags ExclusiveFlags=0 );
	static UObject* StaticFindObject( UClass* Class, UObject* InOuter, const TCHAR* Name, UBOOL ExactClass=0 );
	static UObject* StaticFindObjectChecked( UClass* Class, UObject* InOuter, const TCHAR* Name, UBOOL ExactClass=0 );
	/**
	 * Find or load an object by string name with optional outer and filename specifications.
	 * These are optional because the InName can contain all of the necessary information.
	 *
	 * @param ObjectClass	The class (or a superclass) of the object to be loaded.
	 * @param InOuter		An optional object to narrow where to find/load the object from
	 * @param InName		String name of the object. If it's not fully qualified, InOuter and/or Filename will be needed
	 * @param Filename		An optional file to load from (or find in the file's package object)
	 * @param LoadFlags		Flags controlling how to handle loading from disk
	 * @param Sandbox		A list of packages to restrict the search for the object
	 * @param bAllowObjectReconciliation	Whether to allow the object to be found via FindObject in the case of seek free loading
	 *
	 * @return The object that was loaded or found. NULL for a failure.
	 */
	static UObject* StaticLoadObject( UClass* Class, UObject* InOuter, const TCHAR* Name, const TCHAR* Filename, DWORD LoadFlags, UPackageMap* Sandbox, UBOOL bAllowObjectReconciliation = TRUE );
	static UClass* StaticLoadClass( UClass* BaseClass, UObject* InOuter, const TCHAR* Name, const TCHAR* Filename, DWORD LoadFlags, UPackageMap* Sandbox );
	/**
	 * Create a new instance of an object or replace an existing object.  If both an Outer and Name are specified, and there is an object already in memory with the same Class, Outer, and Name, the
	 * existing object will be destructed, and the new object will be created in its place.
	 * 
	 * @param	Class		the class of the object to create
	 * @param	InOuter		the object to create this object within (the Outer property for the new object will be set to the value specified here).
	 * @param	Name		the name to give the new object. If no value (NAME_None) is specified, the object will be given a unique name in the form of ClassName_#.
	 * @param	SetFlags	the ObjectFlags to assign to the new object. some flags can affect the behavior of constructing the object.
	 * @param	Template	if specified, the property values from this object will be copied to the new object, and the new object's ObjectArchetype value will be set to this object.
	 *						If NULL, the class default object is used instead.
	 * @param	Error		the output device to use for logging errors
	 * @param	Ptr			the address to use for allocating the new object.  If specified, new object will be created in the same memory block as the existing object.
	 * @param	SubobjectRoot
	 *						Only used to when duplicating or instancing objects; in a nested subobject chain, corresponds to the first object that is not a subobject.
	 *						A value of INVALID_OBJECT for this parameter indicates that we are calling StaticConstructObject to duplicate or instance a non-subobject (which will be the subobject root for any subobjects of the new object)
	 *						A value of NULL indicates that we are not instancing or duplicating an object.
	 * @param	InstanceGraph
	 *						contains the mappings of instanced objects and components to their templates
	 *
	 * @return	a pointer to a fully initialized object of the specified class.
	 */
	static UObject* StaticAllocateObject( UClass* Class, UObject* InOuter=(UObject*)GetTransientPackage(), FName Name=NAME_None, EObjectFlags SetFlags=0, UObject* Template=NULL, FOutputDevice* Error=GError, UObject* Ptr=NULL, UObject* SubobjectRoot=NULL, struct FObjectInstancingGraph* InstanceGraph=NULL );
	/**
	 * Create a new instance of an object.  The returned object will be fully initialized.  If InFlags contains RF_NeedsLoad (indicating that the object still needs to load its object data from disk), components
	 * are not instanced (this will instead occur in PostLoad()).  The different between StaticConstructObject and StaticAllocateObject is that StaticConstructObject will also call the class constructor on the object
	 * and instance any components.
	 * 
	 * @param	Class		the class of the object to create
	 * @param	InOuter		the object to create this object within (the Outer property for the new object will be set to the value specified here).
	 * @param	Name		the name to give the new object. If no value (NAME_None) is specified, the object will be given a unique name in the form of ClassName_#.
	 * @param	SetFlags	the ObjectFlags to assign to the new object. some flags can affect the behavior of constructing the object.
	 * @param	Template	if specified, the property values from this object will be copied to the new object, and the new object's ObjectArchetype value will be set to this object.
	 *						If NULL, the class default object is used instead.
	 * @param	Error		the output device to use for logging errors
	 * @param	SubobjectRoot
	 *						Only used to when duplicating or instancing objects; in a nested subobject chain, corresponds to the first object that is not a subobject.
	 *						A value of INVALID_OBJECT for this parameter indicates that we are calling StaticConstructObject to duplicate or instance a non-subobject (which will be the subobject root for any subobjects of the new object)
	 *						A value of NULL indicates that we are not instancing or duplicating an object.
	 * @param	InstanceGraph
	 *						contains the mappings of instanced objects and components to their templates
	 *
	 * @return	a pointer to a fully initialized object of the specified class.
	 */
	static UObject* StaticConstructObject( UClass* Class, UObject* InOuter=(UObject*)GetTransientPackage(), FName Name=NAME_None, EObjectFlags SetFlags=0, UObject* Template=NULL, FOutputDevice* Error=GError, UObject* SubobjectRoot=NULL, struct FObjectInstancingGraph* InstanceGraph=NULL );
	/**
	 * Creates a copy of SourceObject using the Outer and Name specified, as well as copies of all objects contained by SourceObject.  
	 * Any objects referenced by SourceOuter or RootObject and contained by SourceOuter are also copied, maintaining their name relative to SourceOuter.  Any
	 * references to objects that are duplicated are automatically replaced with the copy of the object.
	 *
	 * @param	SourceObject	the object to duplicate
	 * @param	RootObject		should always be the same value as SourceObject (unused)
	 * @param	DestOuter		the object to use as the Outer for the copy of SourceObject
	 * @param	DestName		the name to use for the copy of SourceObject
	 * @param	FlagMask		a bitmask of EObjectFlags that should be propagated to the object copies.  The resulting object copies will only have the object flags
	 *							specified copied from their source object.
	 * @param	DestClass		optional class to specify for the destination object. MUST BE SERIALIZATION COMPATIBLE WITH SOURCE OBJECT!!!
	 * @param	bMigrateArchetypes
	 *							if TRUE, sets the archetype for each duplicated object to its corresponding source object (after duplication) 
	 *
	 * @return	the duplicate of SourceObject.
	 *
	 * @note: this version is deprecated in favor of StaticDuplicateObjectEx
	 */
	static UObject* StaticDuplicateObject(UObject* SourceObject,UObject* RootObject,UObject* DestOuter,const TCHAR* DestName,EObjectFlags FlagMask = ~0,UClass* DestClass=NULL, UBOOL bMigrateArchetypes=FALSE);
	static UObject* StaticDuplicateObjectEx( struct FObjectDuplicationParameters& Parameters );

	static void StaticInit();
	static void StaticExit();
	static UBOOL StaticExec( const TCHAR* Cmd, FOutputDevice& Ar=*GLog );
	static void StaticTick( FLOAT DeltaTime );
	
	/**
	 * Loads a package and all contained objects that match context flags.
	 *
	 * @param	InOuter		Package to load new package into (usually NULL or ULevel->GetOuter())
	 * @param	Filename	Name of file on disk
	 * @param	LoadFlags	Flags controlling loading behavior
	 * @return	Loaded package if successful, NULL otherwise
	 */
	static UPackage* LoadPackage( UPackage* InOuter, const TCHAR* Filename, DWORD LoadFlags );
	/**
	 * Asynchronously load a package and all contained objects that match context flags. Non- blocking.
	 *
	 * @param	PackageName			Name of package to load
	 * @param	CompletionCallback	Callback called on completion of loading
	 * @param	CallbackUserData	User data passed to callback
	 */
	static void LoadPackageAsync( const FString& PackageName, FAsyncCompletionCallback CompletionCallback, void* CallbackUserData = NULL );
	/**
	 * Returns the async load percentage for a package in flight with the passed in name or -1 if there isn't one.
	 *
	 * @param	PackageName			Name of package to query load percentage for
	 * @return	Async load percentage if package is currently being loaded, -1 otherwise
	 */
	static FLOAT GetAsyncLoadPercentage( const FString& PackageName );

	/**
	 * Save one specific object (along with any objects it references contained within the same Outer) into an Unreal package.
	 * 
	 * @param	InOuter							the outer to use for the new package
	 * @param	Base							the object that should be saved into the package
	 * @param	TopLevelFlags					For all objects which are not referenced [either directly, or indirectly] through Base, only objects
	 *											that contain any of these flags will be saved.  If 0 is specified, only objects which are referenced
	 *											by Base will be saved into the package.
	 * @param	Filename						the name to use for the new package file
	 * @param	Error							error output
	 * @param	Conform							if non-NULL, all index tables for this will be sorted to match the order of the corresponding index table
	 *											in the conform package
	 * @param	bForceByteSwapping				whether we should forcefully byte swap before writing to disk
	 * @param	bWarnOfLongFilename				[opt] If TRUE (the default), warn when saving to a long filename.
	 * @param	SaveFlags						Flags to control saving
	 *
	 * @return	TRUE if the package was saved successfully.
	 */
	static UBOOL SavePackage( UPackage* InOuter, UObject* Base, EObjectFlags TopLevelFlags, const TCHAR* Filename, FOutputDevice* Error=GError, ULinkerLoad* Conform=NULL, UBOOL bForceByteSwapping=FALSE, UBOOL bWarnOfLongFilename=TRUE, DWORD SaveFlags=SAVE_None );
	/** 
	 * Deletes all unreferenced objects, keeping objects that have any of the passed in KeepFlags set
	 *
	 * @param	KeepFlags			objects with those flags will be kept regardless of being referenced or not
	 * @param	bPerformFullPurge	if TRUE, perform a full purge after the mark pass
	 */
	static void CollectGarbage( EObjectFlags KeepFlags, UBOOL bPerformFullPurge = TRUE );
	static void SerializeRootSet( FArchive& Ar, EObjectFlags KeepFlags );

	/**
	 * Returns whether an incremental purge is still pending/ in progress.
	 *
	 * @return	TRUE if incremental purge needs to be kicked off or is currently in progress, FALSE othwerise.
	 */
	static UBOOL IsIncrementalPurgePending();

	/**
	 * Create a unique name by combining a base name and an arbitrary number string.
	 * The object name returned is guaranteed not to exist.
	 *
	 * @param	Parent		the outer for the object that needs to be named
	 * @param	Class		the class for the object
	 * @param	BaseName	optional base name to use when generating the unique object name; if not specified, the class's name is used
	 *
	 * @return	name is the form BaseName_##, where ## is the number of objects of this
	 *			type that have been created since the last time the class was garbage collected.
	 */
	static FName MakeUniqueObjectName( UObject* Outer, UClass* Class, FName BaseName=NAME_None );
	static UBOOL IsReferenced( UObject*& Res, EObjectFlags KeepFlags );
	/**
	 * Blocks till all pending package/ linker requests are fulfilled.
	 */
	static void FlushAsyncLoading();
	/**
	 * Returns whether we are currently async loading a package.
	 *
	 * @return TRUE if we are async loading a package, FALSE otherwise
	 */
	static UBOOL IsAsyncLoading();
	/**
	 * Serializes a bit of data each frame with a soft time limit. The function is designed to be able
	 * to fully load a package in a single pass given sufficient time.
	 *
	 * @param	bUseTimeLimit	Whether to use a time limit
	 * @param	TimeLimit		Soft limit of time this function is allowed to consume
	 */
	static void ProcessAsyncLoading( UBOOL bUseTimeLimit, FLOAT TimeLimit );
	/**
	 * Dissociates all linker import and forced export object references. This currently needs to 
	 * happen as the referred objects might be destroyed at any time.
	 */
	static void DissociateImportsAndForcedExports();
	static void BeginLoad();
	static void EndLoad();

	/**
	 * Wrapper for InitProperties which calls ExitProperties first if this object has already had InitProperties called on it at least once.
	 */
	void SafeInitProperties( BYTE* Data, INT DataCount, UClass* DefaultsClass, BYTE* DefaultData, INT DefaultsCount, UObject* DestObject=NULL, UObject* SubobjectRoot=NULL, struct FObjectInstancingGraph* InstanceGraph=NULL );

	/**
	 * Binary initialize object properties to zero or defaults.
	 *
	 * @param	Data				the data that needs to be initialized
	 * @param	DataCount			the size of the buffer pointed to by Data
	 * @param	DefaultsClass		the class to use for initializing the data
	 * @param	DefaultData			the buffer containing the source data for the initialization
	 * @param	DefaultsCount		the size of the buffer pointed to by DefaultData
	 * @param	DestObject			if non-NULL, corresponds to the object that is located at Data
	 * @param	SubobjectRoot
	 *						Only used to when duplicating or instancing objects; in a nested subobject chain, corresponds to the first object in DestObject's Outer chain that is not a subobject (including DestObject).
	 *						A value of INVALID_OBJECT for this parameter indicates that we are calling StaticConstructObject to duplicate or instance a non-subobject (which will be the subobject root for any subobjects of the new object)
	 *						A value of NULL indicates that we are not instancing or duplicating an object.
	 * @param	InstanceGraph
	 *						contains the mappings of instanced objects and components to their templates
	 */
	static void InitProperties( BYTE* Data, INT DataCount, UClass* DefaultsClass, BYTE* DefaultData, INT DefaultsCount, UObject* DestObject=NULL, UObject* SubobjectRoot=NULL, struct FObjectInstancingGraph* InstanceGraph=NULL );
	static void ExitProperties( BYTE* Data, UClass* Class );
	static void ResetLoaders( UObject* InOuter );

	static void LoadLocalizedProp(			class UProperty* Prop,		const TCHAR* IntName, const TCHAR* SectionName, const TCHAR* KeyPrefix, UObject* Parent, BYTE* Data );
	static void LoadLocalizedStruct(		class UStruct* Struct,		const TCHAR* IntName, const TCHAR* SectionName, const TCHAR* KeyPrefix, UObject* Parent, BYTE* Data );
	static void LoadLocalizedDynamicArray(	class UArrayProperty* Prop,	const TCHAR* IntName, const TCHAR* SectionName, const TCHAR* KeyPrefix, UObject* Parent, BYTE* Data );

	/**
	 * Find an existing package by name
	 * @param InOuter		The Outer object to search inside
	 * @param PackageName	The name of the package to find
	 *
	 * @return The package if it exists
	 */
	static UPackage* FindPackage(UObject* InOuter, const TCHAR* PackageName);

	/**
	 * Find an existing package by name or create it if it doesn't exist
	 * @param InOuter		The Outer object to search inside
	 * @param PackageName	The name of the package to find
	 *
	 * @return The existing package or a newly created one
	 */
	static UPackage* CreatePackage(UObject* InOuter, const TCHAR* PackageName);

	static ULinkerLoad* GetPackageLinker( UPackage* InOuter, const TCHAR* Filename, DWORD LoadFlags, UPackageMap* Sandbox, FGuid* CompatibleGuid );
	static void StaticShutdownAfterError();
	static UObject* GetIndexedObject( INT Index );
	static void GlobalSetProperty( const TCHAR* Value, UClass* Class, UProperty* Property, INT Offset, UBOOL bNotifyObjectOfChange );

	/**
	 * Exports the property values for the specified object as text to the output device.
	 *
	 * @param	Context			Context from which the set of 'inner' objects is extracted.  If NULL, an object iterator will be used.
	 * @param	Out				the output device to send the exported text to
	 * @param	ObjectClass		the class of the object to dump properties for
	 * @param	Object			the address of the object to dump properties for
	 * @param	Indent			number of spaces to prepend to each line of output
	 * @param	DiffClass		the class to use for comparing property values when delta export is desired.
	 * @param	Diff			the address of the object to use for determining whether a property value should be exported.  If the value in Object matches the corresponding
	 *							value in Diff, it is not exported.  Specify NULL to export all properties.
	 * @param	Parent			the UObject corresponding to Object
	 * @param	PortFlags		flags used for modifying the output and/or behavior of the export
	 */
	static void ExportProperties( const class FExportObjectInnerContext* Context, FOutputDevice& Out, UClass* ObjectClass, BYTE* Object, INT Indent, UClass* DiffClass, BYTE* Diff, UObject* Parent, DWORD PortFlags=0 );
	static UBOOL GetInitialized();
	static UPackage* GetTransientPackage();
	static void VerifyLinker( ULinkerLoad* Linker );
	static void ProcessRegistrants();
	static void BindPackage( UPackage* Pkg );
	static const TCHAR* GetLanguage();
	static void SetLanguage( const TCHAR* LanguageExt );
	/**
	 * Returns size of objects array
	 *
	 * @return	size of objects array
	 */
	static INT GetObjectArrayNum()
	{
		return GObjObjects.Num();
	}

	// Functions.
	void AddToRoot();
	void RemoveFromRoot();

	/**
	 * Returns the fully qualified pathname for this object as well as the name of the class, in the format:
	 * 'ClassName Outermost[.Outer].Name'.
	 *
	 * @param	StopOuter	if specified, indicates that the output string should be relative to this object.  if StopOuter
	 *						does not exist in this object's Outer chain, the result would be the same as passing NULL.
	 *
	 * @note	safe to call on NULL object pointers!
	 */
	FString GetFullName( const UObject* StopOuter=NULL ) const;
	/**
	 * Returns the fully qualified pathname for this object, in the format:
	 * 'Outermost[.Outer].Name'
	 *
	 * @param	StopOuter	if specified, indicates that the output string should be relative to this object.  if StopOuter
	 *						does not exist in this object's Outer chain, the result would be the same as passing NULL.
	 *
	 * @note	safe to call on NULL object pointers!
	 */
	FString GetPathName( const UObject* StopOuter=NULL ) const;
	/**
	 * Walks up the chain of packages until it reaches the top level, which it ignores.
	 *
	 * @param	bStartWithOuter		whether to include this object's name in the returned string
	 * @return	string containing the path name for this object, minus the outermost-package's name
	 */
	FString GetFullGroupName( UBOOL bStartWithOuter ) const;
	UBOOL IsValid();
	void ConditionalRegister();
	UBOOL ConditionalBeginDestroy();
	UBOOL ConditionalFinishDestroy();
	void ConditionalDestroy();
	void ConditionalPostLoad();

	/**
	 * Instances subobjects and components for objects being loaded from disk, if necessary.  Ensures that references
	 * between nested components are fixed up correctly.
	 *
	 * @param	OuterInstanceGraph	when calling this method on subobjects, specifies the instancing graph which contains all instanced
	 *								subobjects and components for a subobject root.
	 */
	void ConditionalPostLoadSubobjects( struct FObjectInstancingGraph* OuterInstanceGraph=NULL );

	void ConditionalShutdownAfterError();

	/**
	 * Called after importing property values for this object (paste, duplicate or .t3d import)
	 * Allow the object to perform any cleanup for properties which shouldn't be duplicated or
	 * are unsupported by the script serialization
	 */
	virtual void PostEditImport() {}

	/**
	 * @return	TRUE if this object is of the specified type.
	 */
	UBOOL IsA( const UClass* SomeBaseClass ) const;

	/**
	 * @return	TRUE if the specified object appears somewhere in this object's outer chain.
	 */
	UBOOL IsIn( const UObject* SomeOuter ) const;

	/**
	 * Find out if this object is inside (has an outer) that is of the specified class
	 * @param SomeBaseClass	The base class to compare against
	 * @return True if this object is in an object of the given type.
	 */
	UBOOL IsInA( const UClass* SomeBaseClass ) const;

	/**
	 * Checks whether object is part of permanent object pool.
	 *
	 * @return TRUE if object is part of permanent object pool, FALSE otherwise
	 */
	FORCEINLINE UBOOL ResidesInPermanentPool() const
	{
		return ((BYTE*)this >= GPermanentObjectPool) && ((BYTE*)this <= GPermanentObjectPoolTail);
	}

	/**
	 * Determine if this object has SomeObject in its archetype chain.
	 */
	inline UBOOL IsBasedOnArchetype( const UObject* const SomeObject ) const;

	/**
	 * Determines whether this object is contained within a UPrefab.
	 *
	 * @param	OwnerPrefab		if specified, receives a pointer to the owning prefab.
	 *
	 * @return	TRUE if this object is contained within a UPrefab; FALSE if it IS a UPrefab or isn't contained within one.
	 */
	virtual UBOOL IsAPrefabArchetype( class UObject** OwnerPrefab=NULL ) const;

	/**
	 * @return		TRUE if the object is a UPrefabInstance or part of a prefab instance.
	 */
	virtual UBOOL IsInPrefabInstance() const;

	virtual UBOOL Rename( const TCHAR* NewName=NULL, UObject* NewOuter=NULL, ERenameFlags Flags=REN_None );
	UField* FindObjectField( FName InName, UBOOL Global=0 );
	UFunction* FindFunction( FName InName, UBOOL Global=0 );
	UFunction* FindFunctionChecked( FName InName, UBOOL Global=0 );
	UState* FindState( FName InName );

	/**
	 * Finds the component that is contained within this object that has the specified component name.
	 *
	 * @param	ComponentName	the component name to search for
	 * @param	bRecurse		if TRUE, also searches all objects contained within this object for the component specified
	 *
	 * @return	a pointer to a component contained within this object that has the specified component name, or
	 *			NULL if no components were found within this object with the specified name.
	 */
	virtual UComponent* FindComponent( FName ComponentName, UBOOL bRecurse=FALSE );

	/**
	 * Uses the TArchiveObjectReferenceCollector to build a list of all components referenced by this object which have this object as the outer
	 *
	 * @param	out_ComponentMap			the map that should be populated with the components "owned" by this object
	 * @param	bIncludeNestedComponents	controls whether components which are contained by this object, but do not have this object
	 *										as its direct Outer should be included
	 */
	void CollectComponents( TMap<FName,UComponent*>& out_ComponentMap, UBOOL bIncludeNestedComponents=FALSE );

	/**
	 * Uses the TArchiveObjectReferenceCollector to build a list of all components referenced by this object which have this object as the outer
	 *
	 * @param	out_ComponentArray	the array that should be populated with the components "owned" by this object
	 * @param	bIncludeNestedComponents	controls whether components which are contained by this object, but do not have this object
	 *										as its direct Outer should be included
	 */
	void CollectComponents( TArray<UComponent*>& out_ComponentArray, UBOOL bIncludeNestedComponents=FALSE );

	/**
	 * Debugging function for dumping the component properties and values of this object to the log.
	 */
	void DumpComponents();

	void SaveConfig( QWORD Flags=CPF_Config, const TCHAR* Filename=NULL );

	/**
	 * Imports property values from an .ini file.
	 *
	 * @param	Class				the class to use for determining which section of the ini to retrieve text values from
	 * @param	Filename			indicates the filename to load values from; if not specified, uses ConfigClass's ClassConfigName
	 * @param	PropagationFlags	indicates how this call to LoadConfig should be propagated; expects a bitmask of UE3::ELoadConfigPropagationFlags values.
	 * @param	PropertyToLoad		if specified, only the ini value for the specified property will be imported.
	 */
	void LoadConfig( UClass* ConfigClass=NULL, const TCHAR* Filename=NULL, DWORD PropagationFlags=UE3::LCPF_None, class UProperty* PropertyToLoad=NULL );

	/**
	 * Wrapper method for LoadConfig that is used when reloading the config data for objects at runtime which have already loaded their config data at least once.
	 * Allows the objects the receive a callback that it's configuration data has been reloaded.
	 */
	void ReloadConfig( UClass* ConfigClass=NULL, const TCHAR* Filename=NULL, DWORD PropagationFlags=UE3::LCPF_None, class UProperty* PropertyToLoad=NULL );

	/**
	 * Called from ReloadConfig after the object has reloaded its configuration data.
	 */
	virtual void PostReloadConfig( class UProperty* PropertyThatWasLoaded ) {}

	/**
	 * Imports the localized property values for this object.
	 *
	 * @param	LocBase					the object to use for determing where to load the localized property from; defaults to 'this';  should always be
	 *									either 'this' or an archetype of 'this'
	 * @param	bLoadHierachecally		specify TRUE to have this object import the localized property data from its archetype's localization location first.
	 */
	void LoadLocalized( UObject* LocBase=NULL, UBOOL bLoadHierachecally=FALSE );

	/**
	 * Wrapper method for LoadLocalized that is used when reloading localization data for objects at runtime which have already loaded their localization data at least once.
	 */
	void ReloadLocalized();

	/**
	 * Retrieves the location of the property values for this object's localized properties.
	 *
	 * @param	LocBase			the object to use for determing where to load the localized property from; should always be
	 *							either 'this' or an archetype of 'this'
	 * @param	LocFilename		[out] receives the filename which contains the loc data for this object
	 * @param	LocSection		[out] receives the section name that contains the loc data for this object
	 * @param	LocPrefix		[out] receives the prefix string to use when reading keynames from the loc section; usually only relevant when loading
	 *							loading loc data for subobjects, and in that case will always be the name of the subobject template
	 *
	 * @return	TRUE if LocFilename and LocSection were filled with valid values; FALSE if this object's class isn't localized or the loc data location
	 *			couldn't be found for some reason.
	 */
	UBOOL GetLocalizationDataLocation( UObject* LocBase, FString& LocFilename, FString& LocSection, FString& LocPrefix );

	/**
	 * Initializes the properties for this object based on the property values of the
	 * specified class's default object
	 *
	 * @param	InClass			the class to use for initializing this object
	 * @param	SetOuter		TRUE if the Outer for this object should be changed
	 * @param	bPseudoObject	TRUE if 'this' does not point to a real UObject.  Used when
	 *							treating an arbitrary block of memory as a UObject.  Specifying
	 *							TRUE for this parameter has the following side-effects:
	 *							- vtable for this UObject is copied from the specified class
	 *							- sets the Class for this object to the specified class
	 *							- sets the Index property of this UObject to INDEX_NONE
	 */
	void InitClassDefaultObject( UClass* InClass, UBOOL SetOuter = 0, UBOOL bPseudoObject = 0 );

	void ParseParms( const TCHAR* Parms );
	virtual void NetDirty(UProperty* property) {}
	void OutputReferencers( FOutputDevice& Ar, UBOOL bIncludeTransients );

	/**
	 * Used by various commandlets to purge Editor only data from the object.
	 * 
	 * @param TargetPlatform Platform the object will be saved for (ie PC vs console cooking, etc)
	 */
	virtual void StripData(UE3::EPlatformType TargetPlatform) {}

	//
	// UnrealEd browser utility functions
	//

	/** 
	 * Returns a one line description of an object for viewing in the thumbnail view of the generic browser
	 */
	virtual FString GetDesc() { return TEXT( "" ); }

	/** 
	 * Returns detailed info to populate listview columns (defaults to the one line description)
	 */
	virtual FString GetDetailedDescription( INT InIndex );

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
	virtual UBOOL GetNativePropertyValues( TMap<FString,FString>& out_PropertyValues, DWORD ExportFlags=0 ) const
	{
		return FALSE;
	}

	/**
	 * Find most derived object in our archetype chain that is correctly aligned
	 *
	 * @return	pointer to a UObject that is not marked as RF_MisalignedObject
	 */
	UObject* GetStableArchetype()
	{
		UObject* Result = GetArchetype();
		if ( Result == NULL )
			return this;

		while ( Result->HasAnyFlags(RF_MisalignedObject) && Result->GetArchetype() != NULL )
		{
			Result = Result->GetArchetype();
		}

		return Result;
	}

	/**
	* Determines whether this object is a template object
	*
	* @return	TRUE if this object is a template object (owned by a UClass)
	*/
	FORCEINLINE UBOOL IsTemplate( EObjectFlags TemplateTypes = (RF_ArchetypeObject|RF_ClassDefaultObject) ) const
	{
		for ( const UObject* TestOuter = this; TestOuter; TestOuter = TestOuter->GetOuter() )
		{
			if ( TestOuter->HasAnyFlags(TemplateTypes) )
				return TRUE;
		}

		return FALSE;
	}

	/**
	 * @return		TRUE if the object is selected, FALSE otherwise.
	 */
	virtual UBOOL IsSelected() const;

	// Marks the package containing this object as needing to be saved.
	void MarkPackageDirty( UBOOL InDirty = 1 ) const;

	/**
	 * Creates a new archetype based on this UObject.  The archetype's property values will match
	 * the current values of this UObject.
	 *
	 * @param	ArchetypeName			the name for the new class
	 * @param	ArchetypeOuter			the outer to create the new class in (package?)
	 * @param	AlternateArchetype		if specified, is set as the ObjectArchetype for the newly created archetype, after the new archetype
	 *									is initialized against "this".  Should only be specified in cases where you need the new archetype to
	 *									inherit the property values of this object, but don't want this object to be the new archetype's ObjectArchetype.
	 * @param	InstanceGraph			contains the mappings of instanced objects and components to their templates
	 *
	 * @return	a pointer to a UObject which has values identical to this object
	 */
	virtual UObject* CreateArchetype( const TCHAR* ArchetypeName, UObject* ArchetypeOuter, UObject* AlternateArchetype=NULL, struct FObjectInstancingGraph* InstanceGraph=NULL );

	/**
	 *	Update the ObjectArchetype of this UObject based on this UObject's properties.
	 */
	virtual void UpdateArchetype();

	/**
	 *	Called on a new Archetype just after it has been created and is being added to a Prefab.
	 *	Allows doing any Actor-specific cleanup required.
	 */
	virtual void OnAddToPrefab() {}


	// Accessors.
	FORCEINLINE UClass* GetClass() const
	{
		return Class;
	}
	void SetClass(UClass* NewClass)
	{
		Class = NewClass;
	}
	FORCEINLINE EObjectFlags GetFlags() const
	{
		return ObjectFlags;
	}
	FORCEINLINE void SetFlags( EObjectFlags NewFlags )
	{
		ObjectFlags |= NewFlags;
		checkSlow(Name!=NAME_None || !(ObjectFlags&RF_Public));
	}
	FORCEINLINE void ClearFlags( EObjectFlags NewFlags )
	{
		ObjectFlags &= ~NewFlags;
		checkSlow(Name!=NAME_None || !(ObjectFlags&RF_Public));
	}
	/**
	 * Used to safely check whether any of the passed in flags are set. This is required
	 * as EObjectFlags currently is a 64 bit data type and UBOOL is a 32 bit data type so
	 * simply using GetFlags() & RF_MyFlagBiggerThanMaxInt won't work correctly when
	 * assigned directly to an UBOOL.
	 *
	 * @param FlagsToCheck	Object flags to check for.
	 * @return				TRUE if any of the passed in flags are set, FALSE otherwise  (including no flags passed in).
	 */
	FORCEINLINE UBOOL HasAnyFlags( EObjectFlags FlagsToCheck ) const
	{
		return (ObjectFlags & FlagsToCheck) != 0 || FlagsToCheck == RF_AllFlags;
	}
	/**
	 * Used to safely check whether all of the passed in flags are set. This is required
	 * as EObjectFlags currently is a 64 bit data type and UBOOL is a 32 bit data type so
	 * simply using GetFlags() & RF_MyFlagBiggerThanMaxInt won't work correctly when
	 * assigned directly to an UBOOL.
	 *
	 * @param FlagsToCheck	Object flags to check for
	 * @return TRUE if all of the passed in flags are set (including no flags passed in), FALSE otherwise
	 */
	FORCEINLINE UBOOL HasAllFlags( EObjectFlags FlagsToCheck ) const
	{
		return ((ObjectFlags & FlagsToCheck) == FlagsToCheck);
	}
	/**
	 * Returns object flags that are both in the mask and set on the object.
	 *
	 * @param Mask	Mask to mask object flags with
	 * @param Objects flags that are set in both the object and the mask
	 */
	FORCEINLINE EObjectFlags GetMaskedFlags( EObjectFlags Mask ) const
	{
		return ObjectFlags & Mask;
	}
	/**
	 * Returns the name of this object (with no path information)
	 * 
	 * @return Name of the object.
	 */
	FORCEINLINE FString GetName() const
	{
		if( this == NULL )
		{
			return TEXT("None");
		}
		else
		{
			if (Index == INDEX_NONE)
			{
				return TEXT("<uninitialized>"); 
			}
			else
			{
				return Name.ToString();
			}
		}
	}
	FORCEINLINE const FName GetFName() const
	{
		return Index != INDEX_NONE ? Name : TEXT("<uninitialized>");
	}
	FORCEINLINE UObject* GetOuter() const
	{
		return Outer;
	}
	/** 
	 * Walks up the list of outers until it finds the highest one.
	 *
	 * @return outermost non NULL Outer.
	 */
	UPackage* GetOutermost() const;
	/**
	 * Returns the object's index into the objects array.
	 *
	 * @return object's index in objects array
	 */
	FORCEINLINE DWORD GetIndex() const
	{
		return Index;
	}
	/**
	 * Returns the linker for this object.
	 *
	 * @return	a pointer to the linker for this object, or NULL if this object has no linker
	 */
	FORCEINLINE ULinkerLoad* GetLinker() const
	{
		return _Linker;
	}
	/**
	 * Returns this object's LinkerIndex.
	 *
	 * @return	the index into my linker's ExportMap for the FObjectExport
	 *			corresponding to this object.
	 */
	FORCEINLINE INT GetLinkerIndex() const
	{
		return _LinkerIndex;
	}
	/**
	 * Returns the version of the linker for this object.
	 *
	 * @return	the version of the engine's package file when this object
	 *			was last saved, or GPackageFileVersion (current version) if
	 *			this object does not have a linker, which indicates that
	 *			a) this object is a native only class,
	 *			b) this object's linker has been detached, in which case it is already fully loaded
	 */
	INT GetLinkerVersion() const;

	/**
	 * Returns the licensee version of the linker for this object.
	 *
	 * @return	the licensee version of the engine's package file when this object
	 *			was last saved, or GPackageFileLicenseeVersion (current version) if
	 *			this object does not have a linker, which indicates that
	 *			a) this object is a native only class, or
	 *			b) this object's linker has been detached, in which case it is already fully loaded
	 */
	INT GetLinkerLicenseeVersion() const;

	FORCEINLINE FStateFrame* GetStateFrame() const
	{
		return StateFrame;
	}

	/**
	 * Returns the size of the object/ resource for display to artists/ LDs in the Editor. The
	 * default behavior is to return 0 which indicates that the resource shouldn't display its
	 * size which is used to not confuse people by displaying small sizes for e.g. objects like
	 * materials
	 *
	 * @return size of resource as to be displayed to artists/ LDs in the Editor.
	 */
	virtual INT GetResourceSize()
	{
		return 0;
	}

	/** 
	 * Returns the name of the exporter factory used to export this object
	 * Used when multiple factories have the same extension
	 */
	virtual FName GetExporterName( void )
	{
		return( FName( TEXT( "" ) ) );
	}

	/**
	 * Returns whether this wave file is a localized resource.
	 *
	 * @return TRUE if it is a localized resource, FALSE otherwise.
	 */
	virtual UBOOL IsLocalizedResource()
	{
		return HasAnyFlags( RF_LocalizedResource );
	}

	/**
	 * Wrapper function for InitProperties() - initialize this object using the
	 * object specified.
	 *
	 * @param	SourceObject	the object to use for initializing this object
	 * @param	InstanceGraph	contains the mappings of instanced objects and components to their templates
	 */
	virtual void InitializeProperties( UObject* SourceObject=NULL, struct FObjectInstancingGraph* InstanceGraph=NULL );

	/**
	 * Sets the ObjectArchetype for this object, optionally reinitializing this object
	 * from the new archetype.
	 *
	 * @param	NewArchetype	the object to change this object's ObjectArchetype to
	 * @param	bReinitialize	TRUE if we should the property values should be reinitialized
	 *							using the new archetype.
	 * @param	InstanceGraph	contains the mappings of instanced objects and components to their templates; only relevant
	 *							if bReinitialize is TRUE
	 */
	virtual void SetArchetype( UObject* NewArchetype, UBOOL bReinitialize=FALSE, struct FObjectInstancingGraph* InstanceGraph=NULL );

	/**
	 * Return the template this object is based on.
	 */
	inline UObject* GetArchetype() const
	{
		return ObjectArchetype;
	}

	template<class T>
	T* GetArchetype() const
	{
		return ObjectArchetype && ObjectArchetype->IsA(T::StaticClass()) ? (T*)ObjectArchetype : NULL;
	}

	/**
	 * Wrapper for calling UClass::InstanceSubobjectTemplates
	 */
	void InstanceSubobjectTemplates( struct FObjectInstancingGraph* InstanceGraph );

	/**
	 * Wrapper for calling UClass::InstanceComponentTemplates() for this object.
	 */
	void InstanceComponentTemplates( struct FObjectInstancingGraph* InstanceGraph );

	/**
	 * Returns a pointer to this object safely converted to a pointer of the specified interface class.
	 *
	 * @param	InterfaceClass	the interface class to use for the returned type
	 *
	 * @return	a pointer that can be assigned to a variable of the interface type specified, or NULL if this object's
	 *			class doesn't implement the interface indicated.  Will be the same value as 'this' if the interface class
	 *			isn't native.
	 */
	void* GetInterfaceAddress( UClass* InterfaceClass );

	/**
	 * Callback used to allow object register its direct object references that are not already covered by
	 * the token stream.
	 *
	 * @param ObjectArray	array to add referenced objects to via AddReferencedObject
	 */
	virtual void AddReferencedObjects( TArray<UObject*>& ObjectArray ) {}

	/**
	 * Helper function to add a referenced object to the passed in array. The function ensures that the item
	 * won't be added twice by checking the RF_Unreachable flag.
	 *
	 * @todo rtgc: temporary helper as references cannot be NULLed out this way
	 *
	 * @param ObjectARray	array to add object to
	 * @param Object		Object to add if it isn't already part of the array (is reachable)
	 */
	static void AddReferencedObject( TArray<UObject*>& ObjectArray, UObject* Object );

	/**
	 * Helper function to add referenced objects via serialization and an FArchiveObjectReferenceCollector 
	 *
	 * @param ObjectArray	array to add referenced objects to via AddReferencedObject
	 */
	void AddReferencedObjectsViaSerialization( TArray<UObject*>& ObjectArray );

	//==========================================
	// Script Virtual Machine
	//==========================================
	virtual void ProcessEvent( UFunction* Function, void* Parms, void* Result=NULL );
	virtual void ProcessDelegate( FName DelegateName, struct FScriptDelegate* Delegate, void* Parms, void* Result=NULL );
	virtual void ProcessState( FLOAT DeltaSeconds );
	virtual UBOOL ProcessRemoteFunction( UFunction* Function, void* Parms, FFrame* Stack );

	virtual void InitExecution();
	virtual UBOOL GotoLabel( FName Label );
	virtual EGotoState GotoState( FName State, UBOOL bForceEvents = 0, UBOOL bKeepStack = 0 );

	virtual UBOOL ScriptConsoleExec( const TCHAR* Cmd, FOutputDevice& Ar, UObject* Executor );
	void CallFunction( FFrame& Stack, RESULT_DECL, UFunction* Function );
	void ProcessInternal( FFrame& Stack, RESULT_DECL );

	/** advances Stack's code past the parameters to the given Function and if the function has a return value, copies the zero value for that property to the memory for the return value
	 * @param Stack the script stack frame
	 * @param Result pointer to where the return value should be written
	 * @param Function the function being called
	 */
	void SkipFunction(FFrame& Stack, RESULT_DECL, UFunction* Function);

	UBOOL IsProbing( FName ProbeName );

	// exec function bodies
	void PushState(FName NewState, FName NewLabel=NAME_None);
	void PopState(UBOOL bPopAll=FALSE);


	// delegates
	// DELEGATE_IS_SET returns true if the delegate has a FunctionName assigned and the object which will execute the delegate is not marked as pending kill
	#define DELEGATE_IS_SET(del) (__##del##__Delegate.IsCallable(this))
	#define OBJ_DELEGATE_IS_SET(obj,del) (obj && obj->__##del##__Delegate.IsCallable(obj))

	// UnrealScript intrinsics.
	#define DECLARE_FUNCTION(func) void func( FFrame& Stack, RESULT_DECL )
	DECLARE_FUNCTION(execUndefined);
	DECLARE_FUNCTION(execLocalVariable);
	DECLARE_FUNCTION(execInstanceVariable);
	DECLARE_FUNCTION(execDefaultVariable);
	DECLARE_FUNCTION(execLocalOutVariable);
	DECLARE_FUNCTION(execInterfaceVariable);
	DECLARE_FUNCTION(execInterfaceContext);
	DECLARE_FUNCTION(execDefaultParmValue);
	DECLARE_FUNCTION(execArrayElement);
	DECLARE_FUNCTION(execDynArrayElement);
	DECLARE_FUNCTION(execDynArrayLength);
	DECLARE_FUNCTION(execDynArrayInsert);
	DECLARE_FUNCTION(execDynArrayRemove);
	DECLARE_FUNCTION(execDynArrayFind);
	DECLARE_FUNCTION(execDynArrayFindStruct);
	DECLARE_FUNCTION(execDynArrayAdd);
	DECLARE_FUNCTION(execDynArrayAddItem);
	DECLARE_FUNCTION(execDynArrayInsertItem);
	DECLARE_FUNCTION(execDynArrayRemoveItem);
	DECLARE_FUNCTION(execDynArrayIterator);
	DECLARE_FUNCTION(execBoolVariable);
	DECLARE_FUNCTION(execClassDefaultVariable);
	DECLARE_FUNCTION(execEndFunctionParms);
	DECLARE_FUNCTION(execNothing);
	DECLARE_FUNCTION(execEndOfScript);
	DECLARE_FUNCTION(execReturnNothing);
	DECLARE_FUNCTION(execIteratorPop);
	DECLARE_FUNCTION(execEmptyParmValue);
	DECLARE_FUNCTION(execStop);
	DECLARE_FUNCTION(execEndCode);
	DECLARE_FUNCTION(execSwitch);
	DECLARE_FUNCTION(execCase);
	DECLARE_FUNCTION(execJump);
	DECLARE_FUNCTION(execJumpIfNot);
	DECLARE_FUNCTION(execAssert);
	DECLARE_FUNCTION(execGotoLabel);
	DECLARE_FUNCTION(execLet);
	DECLARE_FUNCTION(execLetBool);
	DECLARE_FUNCTION(execLetDelegate);
	DECLARE_FUNCTION(execEatReturnValue);
	DECLARE_FUNCTION(execSelf);
	DECLARE_FUNCTION(execContext);
	DECLARE_FUNCTION(execVirtualFunction);
	DECLARE_FUNCTION(execFinalFunction);
	DECLARE_FUNCTION(execGlobalFunction);
	DECLARE_FUNCTION(execDelegateFunction);
	DECLARE_FUNCTION(execDelegateProperty);
	DECLARE_FUNCTION(execStructCmpEq);
	DECLARE_FUNCTION(execStructCmpNe);
	DECLARE_FUNCTION(execStructMember);
	DECLARE_FUNCTION(execIntConst);
	DECLARE_FUNCTION(execFloatConst);
	DECLARE_FUNCTION(execStringConst);
	DECLARE_FUNCTION(execUnicodeStringConst);
	DECLARE_FUNCTION(execObjectConst);
	DECLARE_FUNCTION(execNameConst);
	DECLARE_FUNCTION(execByteConst);
	DECLARE_FUNCTION(execIntZero);
	DECLARE_FUNCTION(execIntOne);
	DECLARE_FUNCTION(execTrue);
	DECLARE_FUNCTION(execFalse);
	DECLARE_FUNCTION(execNoObject);
	DECLARE_FUNCTION(execIntConstByte);
	DECLARE_FUNCTION(execDynamicCast);
	DECLARE_FUNCTION(execMetaCast);
	DECLARE_FUNCTION(execPrimitiveCast);
	DECLARE_FUNCTION(execInterfaceCast);
	DECLARE_FUNCTION(execByteToInt);
	DECLARE_FUNCTION(execByteToBool);
	DECLARE_FUNCTION(execByteToFloat);
	DECLARE_FUNCTION(execByteToString);
	DECLARE_FUNCTION(execIntToByte);
	DECLARE_FUNCTION(execIntToBool);
	DECLARE_FUNCTION(execIntToFloat);
	DECLARE_FUNCTION(execIntToString);
	DECLARE_FUNCTION(execBoolToByte);
	DECLARE_FUNCTION(execBoolToInt);
	DECLARE_FUNCTION(execBoolToFloat);
	DECLARE_FUNCTION(execBoolToString);
	DECLARE_FUNCTION(execFloatToByte);
	DECLARE_FUNCTION(execFloatToInt);
	DECLARE_FUNCTION(execFloatToBool);
	DECLARE_FUNCTION(execFloatToString);
	DECLARE_FUNCTION(execRotationConst);
	DECLARE_FUNCTION(execVectorConst);
	DECLARE_FUNCTION(execPointDistToLine);
	DECLARE_FUNCTION(execGetDotDistance);
	DECLARE_FUNCTION(execGetAngularDistance);
	DECLARE_FUNCTION(execGetAngularFromDotDist);
	DECLARE_FUNCTION(execStringToVector);
	DECLARE_FUNCTION(execStringToRotator);
	DECLARE_FUNCTION(execVectorToBool);
	DECLARE_FUNCTION(execVectorToString);
	DECLARE_FUNCTION(execVectorToRotator);
	DECLARE_FUNCTION(execRotatorToBool);
	DECLARE_FUNCTION(execRotatorToVector);
	DECLARE_FUNCTION(execRotatorToString);
    DECLARE_FUNCTION(execRotRand);
    DECLARE_FUNCTION(execGetUnAxes);
    DECLARE_FUNCTION(execGetAxes);
    DECLARE_FUNCTION(execSubtractEqual_RotatorRotator);
    DECLARE_FUNCTION(execAddEqual_RotatorRotator);
    DECLARE_FUNCTION(execSubtract_RotatorRotator);
    DECLARE_FUNCTION(execAdd_RotatorRotator);
    DECLARE_FUNCTION(execDivideEqual_RotatorFloat);
    DECLARE_FUNCTION(execMultiplyEqual_RotatorFloat);
    DECLARE_FUNCTION(execDivide_RotatorFloat);
    DECLARE_FUNCTION(execMultiply_FloatRotator);
    DECLARE_FUNCTION(execMultiply_RotatorFloat);
    DECLARE_FUNCTION(execNotEqual_RotatorRotator);
    DECLARE_FUNCTION(execEqualEqual_RotatorRotator);
	DECLARE_FUNCTION(execGetRand);
	DECLARE_FUNCTION(execRSize);
	DECLARE_FUNCTION(execRMin);
	DECLARE_FUNCTION(execRMax);
	DECLARE_FUNCTION(execMirrorVectorByNormal);
    DECLARE_FUNCTION(execVRand);
	DECLARE_FUNCTION(execVLerp);
	DECLARE_FUNCTION(execVSmerp);
	DECLARE_FUNCTION(execVInterpTo);
	DECLARE_FUNCTION(execClampLength);
    DECLARE_FUNCTION(execNormal);
    DECLARE_FUNCTION(execVSize);
    DECLARE_FUNCTION(execVSize2D);
    DECLARE_FUNCTION(execVSizeSq);
	DECLARE_FUNCTION(execVSizeSq2D);
    DECLARE_FUNCTION(execSubtractEqual_VectorVector);
    DECLARE_FUNCTION(execAddEqual_VectorVector);
    DECLARE_FUNCTION(execDivideEqual_VectorFloat);
    DECLARE_FUNCTION(execMultiplyEqual_VectorVector);
    DECLARE_FUNCTION(execMultiplyEqual_VectorFloat);
    DECLARE_FUNCTION(execCross_VectorVector);
    DECLARE_FUNCTION(execDot_VectorVector);
    DECLARE_FUNCTION(execNotEqual_VectorVector);
    DECLARE_FUNCTION(execEqualEqual_VectorVector);
    DECLARE_FUNCTION(execGreaterGreater_VectorRotator);
    DECLARE_FUNCTION(execLessLess_VectorRotator);
    DECLARE_FUNCTION(execSubtract_VectorVector);
    DECLARE_FUNCTION(execAdd_VectorVector);
    DECLARE_FUNCTION(execDivide_VectorFloat);
    DECLARE_FUNCTION(execMultiply_VectorVector);
    DECLARE_FUNCTION(execMultiply_FloatVector);
    DECLARE_FUNCTION(execMultiply_VectorFloat);
    DECLARE_FUNCTION(execSubtract_PreVector);
	DECLARE_FUNCTION(execOrthoRotation);
	DECLARE_FUNCTION(execNormalize);
	DECLARE_FUNCTION(execRLerp);
	DECLARE_FUNCTION(execRSmerp);
	DECLARE_FUNCTION(execRInterpTo);
	DECLARE_FUNCTION(execNormalizeRotAxis);
	DECLARE_FUNCTION(execRDiff);
	DECLARE_FUNCTION(execClockwiseFrom_IntInt);
	DECLARE_FUNCTION(execObjectToBool);
	DECLARE_FUNCTION(execObjectToInterface);
	DECLARE_FUNCTION(execObjectToString);
	DECLARE_FUNCTION(execInterfaceToString);
	DECLARE_FUNCTION(execInterfaceToBool);
	DECLARE_FUNCTION(execInterfaceToObject);
	DECLARE_FUNCTION(execNameToBool);
	DECLARE_FUNCTION(execNameToString);
	DECLARE_FUNCTION(execStringToName);
	DECLARE_FUNCTION(execStringToByte);
	DECLARE_FUNCTION(execStringToInt);
	DECLARE_FUNCTION(execStringToBool);
	DECLARE_FUNCTION(execStringToFloat);
	DECLARE_FUNCTION(execNot_PreBool);
	DECLARE_FUNCTION(execEqualEqual_BoolBool);
	DECLARE_FUNCTION(execNotEqual_BoolBool);
	DECLARE_FUNCTION(execAndAnd_BoolBool);
	DECLARE_FUNCTION(execXorXor_BoolBool);
	DECLARE_FUNCTION(execOrOr_BoolBool);
	DECLARE_FUNCTION(execMultiplyEqual_ByteByte);
	DECLARE_FUNCTION(execMultiplyEqual_ByteFloat);
	DECLARE_FUNCTION(execDivideEqual_ByteByte);
	DECLARE_FUNCTION(execAddEqual_ByteByte);
	DECLARE_FUNCTION(execSubtractEqual_ByteByte);
	DECLARE_FUNCTION(execAddAdd_PreByte);
	DECLARE_FUNCTION(execSubtractSubtract_PreByte);
	DECLARE_FUNCTION(execAddAdd_Byte);
	DECLARE_FUNCTION(execSubtractSubtract_Byte);
	DECLARE_FUNCTION(execComplement_PreInt);
	DECLARE_FUNCTION(execSubtract_PreInt);
	DECLARE_FUNCTION(execMultiply_IntInt);
	DECLARE_FUNCTION(execDivide_IntInt);
	DECLARE_FUNCTION(execAdd_IntInt);
	DECLARE_FUNCTION(execSubtract_IntInt);
	DECLARE_FUNCTION(execLessLess_IntInt);
	DECLARE_FUNCTION(execGreaterGreater_IntInt);
	DECLARE_FUNCTION(execGreaterGreaterGreater_IntInt);
	DECLARE_FUNCTION(execLess_IntInt);
	DECLARE_FUNCTION(execGreater_IntInt);
	DECLARE_FUNCTION(execLessEqual_IntInt);
	DECLARE_FUNCTION(execGreaterEqual_IntInt);
	DECLARE_FUNCTION(execEqualEqual_IntInt);
	DECLARE_FUNCTION(execNotEqual_IntInt);
	DECLARE_FUNCTION(execAnd_IntInt);
	DECLARE_FUNCTION(execXor_IntInt);
	DECLARE_FUNCTION(execOr_IntInt);
	DECLARE_FUNCTION(execMultiplyEqual_IntFloat);
	DECLARE_FUNCTION(execDivideEqual_IntFloat);
	DECLARE_FUNCTION(execAddEqual_IntInt);
	DECLARE_FUNCTION(execSubtractEqual_IntInt);
	DECLARE_FUNCTION(execAddAdd_PreInt);
	DECLARE_FUNCTION(execSubtractSubtract_PreInt);
	DECLARE_FUNCTION(execAddAdd_Int);
	DECLARE_FUNCTION(execSubtractSubtract_Int);
	DECLARE_FUNCTION(execRand);
	DECLARE_FUNCTION(execMin);
	DECLARE_FUNCTION(execMax);
	DECLARE_FUNCTION(execClamp);
	DECLARE_FUNCTION(execToHex);
	DECLARE_FUNCTION(execSubtract_PreFloat);
	DECLARE_FUNCTION(execMultiplyMultiply_FloatFloat);
	DECLARE_FUNCTION(execMultiply_FloatFloat);
	DECLARE_FUNCTION(execDivide_FloatFloat);
	DECLARE_FUNCTION(execPercent_FloatFloat);
	DECLARE_FUNCTION(execAdd_FloatFloat);
	DECLARE_FUNCTION(execSubtract_FloatFloat);
	DECLARE_FUNCTION(execLess_FloatFloat);
	DECLARE_FUNCTION(execGreater_FloatFloat);
	DECLARE_FUNCTION(execLessEqual_FloatFloat);
	DECLARE_FUNCTION(execGreaterEqual_FloatFloat);
	DECLARE_FUNCTION(execEqualEqual_FloatFloat);
	DECLARE_FUNCTION(execNotEqual_FloatFloat);
	DECLARE_FUNCTION(execComplementEqual_FloatFloat);
	DECLARE_FUNCTION(execMultiplyEqual_FloatFloat);
	DECLARE_FUNCTION(execDivideEqual_FloatFloat);
	DECLARE_FUNCTION(execAddEqual_FloatFloat);
	DECLARE_FUNCTION(execSubtractEqual_FloatFloat);
	DECLARE_FUNCTION(execAbs);
	DECLARE_FUNCTION(execDebugInfo); //DEBUGGER
	DECLARE_FUNCTION(execSin);
	DECLARE_FUNCTION(execAsin);
	DECLARE_FUNCTION(execCos);
	DECLARE_FUNCTION(execAcos);
	DECLARE_FUNCTION(execTan);
	DECLARE_FUNCTION(execAtan);
	DECLARE_FUNCTION(execExp);
	DECLARE_FUNCTION(execLoge);
	DECLARE_FUNCTION(execSqrt);
	DECLARE_FUNCTION(execSquare);
	DECLARE_FUNCTION(execPow);
	DECLARE_FUNCTION(execFRand);
	DECLARE_FUNCTION(execFMin);
	DECLARE_FUNCTION(execFMax);
	DECLARE_FUNCTION(execFClamp);
	DECLARE_FUNCTION(execLerp);
	DECLARE_FUNCTION(execFCubicInterp);
	DECLARE_FUNCTION(execFInterpTo);
	DECLARE_FUNCTION(execConcat_StrStr);
	DECLARE_FUNCTION(execAt_StrStr);
	DECLARE_FUNCTION(execLess_StrStr);
	DECLARE_FUNCTION(execGreater_StrStr);
	DECLARE_FUNCTION(execLessEqual_StrStr);
	DECLARE_FUNCTION(execGreaterEqual_StrStr);
	DECLARE_FUNCTION(execEqualEqual_StrStr);
	DECLARE_FUNCTION(execNotEqual_StrStr);
	DECLARE_FUNCTION(execComplementEqual_StrStr);
	DECLARE_FUNCTION(execConcatEqual_StrStr);
	DECLARE_FUNCTION(execAtEqual_StrStr);
	DECLARE_FUNCTION(execSubtractEqual_StrStr);
	DECLARE_FUNCTION(execLen);
	DECLARE_FUNCTION(execInStr);
	DECLARE_FUNCTION(execMid);
	DECLARE_FUNCTION(execLeft);
	DECLARE_FUNCTION(execRight);
	DECLARE_FUNCTION(execCaps);
	DECLARE_FUNCTION(execLocs);
	DECLARE_FUNCTION(execChr);
	DECLARE_FUNCTION(execAsc);
	DECLARE_FUNCTION(execRepl);
	DECLARE_FUNCTION(execParseStringIntoArray);
	DECLARE_FUNCTION(execQuatProduct);
	DECLARE_FUNCTION(execQuatDot);
	DECLARE_FUNCTION(execQuatInvert);
	DECLARE_FUNCTION(execQuatRotateVector);
	DECLARE_FUNCTION(execQuatFindBetween);
	DECLARE_FUNCTION(execQuatFromAxisAndAngle);
	DECLARE_FUNCTION(execQuatFromRotator);
	DECLARE_FUNCTION(execQuatToRotator);
	DECLARE_FUNCTION(execQuatSlerp);
	DECLARE_FUNCTION(execAdd_QuatQuat);
	DECLARE_FUNCTION(execSubtract_QuatQuat);
	DECLARE_FUNCTION(execEqualEqual_ObjectObject);
	DECLARE_FUNCTION(execNotEqual_ObjectObject);
	DECLARE_FUNCTION(execEqualEqual_InterfaceInterface);
	DECLARE_FUNCTION(execNotEqual_InterfaceInterface);
	DECLARE_FUNCTION(execEqualEqual_NameName);
	DECLARE_FUNCTION(execNotEqual_NameName);
	DECLARE_FUNCTION(execLogInternal);
	DECLARE_FUNCTION(execWarnInternal);
	DECLARE_FUNCTION(execConditional);
	DECLARE_FUNCTION(execNew);
	DECLARE_FUNCTION(execClassIsChildOf);
	DECLARE_FUNCTION(execClassContext);
	DECLARE_FUNCTION(execGoto);
	DECLARE_FUNCTION(execGotoState);
	DECLARE_FUNCTION(execPushState);
	DECLARE_FUNCTION(execPopState);
	DECLARE_FUNCTION(execDumpStateStack);
	DECLARE_FUNCTION(execIsA);
	DECLARE_FUNCTION(execEnable);
	DECLARE_FUNCTION(execDisable);
	DECLARE_FUNCTION(execIterator);
	DECLARE_FUNCTION(execLocalize);
	DECLARE_FUNCTION(execNativeParm);
	DECLARE_FUNCTION(execSaveConfig);
	DECLARE_FUNCTION(execStaticSaveConfig);
	DECLARE_FUNCTION(execGetPerObjectConfigSections);
	DECLARE_FUNCTION(execGetEnum);
	DECLARE_FUNCTION(execDynamicLoadObject);
	DECLARE_FUNCTION(execFindObject);
	DECLARE_FUNCTION(execIsInState);
	DECLARE_FUNCTION(execIsChildState);
	DECLARE_FUNCTION(execGetStateName);
	DECLARE_FUNCTION(execGetFuncName);
	DECLARE_FUNCTION(execScriptTrace);
	DECLARE_FUNCTION(execSetUTracing);
	DECLARE_FUNCTION(execIsUTracing);
	DECLARE_FUNCTION(execHighNative0);
	DECLARE_FUNCTION(execHighNative1);
	DECLARE_FUNCTION(execHighNative2);
	DECLARE_FUNCTION(execHighNative3);
	DECLARE_FUNCTION(execHighNative4);
	DECLARE_FUNCTION(execHighNative5);
	DECLARE_FUNCTION(execHighNative6);
	DECLARE_FUNCTION(execHighNative7);
	DECLARE_FUNCTION(execHighNative8);
	DECLARE_FUNCTION(execHighNative9);
	DECLARE_FUNCTION(execHighNative10);
	DECLARE_FUNCTION(execHighNative11);
	DECLARE_FUNCTION(execHighNative12);
	DECLARE_FUNCTION(execHighNative13);
	DECLARE_FUNCTION(execHighNative14);
	DECLARE_FUNCTION(execHighNative15);

	DECLARE_FUNCTION(execProjectOnTo);
	DECLARE_FUNCTION(execIsZero);
	DECLARE_FUNCTION(execPathName);

	DECLARE_FUNCTION(execIsPendingKill)
	{
		P_FINISH;
		*(UBOOL*)Result = IsPendingKill();
	}

	// UnrealScript calling stubs.
	struct FStateEvent
	{
		FName StateName;
		FStateEvent(FName InStateName) : StateName(InStateName) {}
	};
    void eventBeginState(FName PreviousStateName)
    {
		FStateEvent Parms(PreviousStateName);
        ProcessEvent(FindFunctionChecked(NAME_BeginState),&Parms);
    }
    void eventEndState(FName NextStateName)
    {
		FStateEvent Parms(NextStateName);
        ProcessEvent(FindFunctionChecked(NAME_EndState),&Parms);
    }
	void eventPushedState()
	{
		ProcessEvent(FindFunctionChecked(NAME_PushedState),NULL);
	}
	void eventPoppedState()
	{
		ProcessEvent(FindFunctionChecked(NAME_PoppedState),NULL);
	}
	void eventPausedState()
	{
		ProcessEvent(FindFunctionChecked(NAME_PausedState),NULL);
	}
	void eventContinuedState()
	{
		ProcessEvent(FindFunctionChecked(NAME_ContinuedState),NULL);
	}
};

/*-----------------------------------------------------------------------------
	FObjectDuplicationParameters.
-----------------------------------------------------------------------------*/

/**
 * This struct is used for passing parameter values to the UObject::StaticDuplicateObject() method.  Only the constructor parameters are required to
 * be valid - all other members are optional.
 */
struct FObjectDuplicationParameters
{
	/**
	 * The object to be duplicated
	 */
	UObject*		SourceObject;

	/**
	 * The object to use as the Outer for the duplicate of SourceObject.
	 */
	UObject*		DestOuter;

	/**
	 * The name to use for the duplicate of SourceObject
	 */
	FName			DestName;

	/**
	 * a bitmask of EObjectFlags to propagate to the duplicate of SourceObject (and its subobjects).
	 */
	EObjectFlags	FlagMask;

	/**
	 * a bitmask of EObjectFlags to set on each duplicate object created.  Different from FlagMask in that only the bits
	 * from FlagMask which are also set on the source object will be set on the duplicate, while the flags in this value
	 * will always be set.
	 */
	EObjectFlags	ApplyFlags;

	/**
	 * optional class to specify for the destination object.
	 * @note: MUST BE SERIALIZATION COMPATIBLE WITH SOURCE OBJECT, AND DOES NOT WORK WELL FOR OBJECT WHICH HAVE COMPLEX COMPONENT HIERARCHIES!!!
	 */
	UClass*			DestClass;

	/**
	 * indicates that the archetype for each duplicated object should be set to its corresponding source object (after duplication)
	 */
	UBOOL			bMigrateArchetypes;

	/**
	 * Objects to use for prefilling the dup-source => dup-target map used by StaticDuplicateObject.  Can be used to allow individual duplication of several objects that share
	 * a common Outer in cases where you don't want to duplicate the shared Outer but need references between the objects to be replaced anyway.
	 *
	 * Objects in this map will NOT be duplicated
	 * Key should be the source object; value should be the object which will be used as its duplicate.
	 */
	TMap<UObject*,UObject*>	DuplicationSeed;

	/**
	 * If non-NULL, this will be filled with the list of objects created during the call to StaticDuplicateObject.
	 *
	 * Key will be the source object; value will be the duplicated object
	 */
	TMap<UObject*,UObject*>* CreatedObjects;

	/**
	 * Constructor
	 */
	FObjectDuplicationParameters( UObject* InSourceObject, UObject* InDestOuter );
};

/*-----------------------------------------------------------------------------
	FScriptDelegate.
-----------------------------------------------------------------------------*/
struct FScriptDelegate
{
	UObject* Object;
	FName FunctionName;

	inline UBOOL IsCallable( UObject* OwnerObject ) const
	{
		// if Object is NULL, it means that the delegate was assigned to a member function through defaultproperties; in this case, OwnerObject
		// will be the object that contains the function referenced by FunctionName.
		return FunctionName != NAME_None && (Object != NULL ? !Object->IsPendingKill() : (OwnerObject != NULL && !OwnerObject->IsPendingKill()));
	}

	friend FArchive& operator<<( FArchive& Ar, FScriptDelegate& D )
	{
		return Ar << D.Object << D.FunctionName;
	}
};

/*----------------------------------------------------------------------------
	Core templates.
----------------------------------------------------------------------------*/

// Hash function.
inline DWORD GetTypeHash( const UObject* A )
{
	return A ? A->GetIndex() : 0;
}

// Parse an object name in the input stream.
template< class T > UBOOL ParseObject( const TCHAR* Stream, const TCHAR* Match, T*& Obj, UObject* Outer )
{
	return ParseObject( Stream, Match, T::StaticClass(), *(UObject **)&Obj, Outer );
}

// Find an optional object.
template< class T > T* FindObject( UObject* Outer, const TCHAR* Name, UBOOL ExactClass=0 )
{
	return (T*)UObject::StaticFindObject( T::StaticClass(), Outer, Name, ExactClass );
}

// Find an object, no failure allowed.
template< class T > T* FindObjectChecked( UObject* Outer, const TCHAR* Name, UBOOL ExactClass=0 )
{
	return (T*)UObject::StaticFindObjectChecked( T::StaticClass(), Outer, Name, ExactClass );
}



// Load an object.
template< class T > T* LoadObject( UObject* Outer, const TCHAR* Name, const TCHAR* Filename, DWORD LoadFlags, UPackageMap* Sandbox )
{
	return (T*)UObject::StaticLoadObject( T::StaticClass(), Outer, Name, Filename, LoadFlags, Sandbox );
}

// Load a class object.
template< class T > UClass* LoadClass( UObject* Outer, const TCHAR* Name, const TCHAR* Filename, DWORD LoadFlags, UPackageMap* Sandbox )
{
	return UObject::StaticLoadClass( T::StaticClass(), Outer, Name, Filename, LoadFlags, Sandbox );
}

// Get default object of a class.
template< class T > T* GetDefault()
{
	return (T*)T::StaticClass()->GetDefaults();
}

#define INVALID_NAME_CHARACTERS			TEXT("\"' ,\n\r\t")
#define INVALID_OBJECTNAME_CHARACTERS	TEXT("\"' ,.\n\r\t")
#define INVALID_PACKAGENAME_CHARACTERS	TEXT("\"' ,/.\n\r\t")

/**
 * Takes an FName and checks to see that it follows the rules that Unreal requires.
 *
 * @param	InName			The name to check
 * @param	InInvalidChars	The set of invalid characters that the name cannot contain
 * @param	InReason		If the check fails, this string is filled in with the reason why.
 *
 * @return	1 if the name is valid, 0 if it is not
 */
inline UBOOL FIsValidXName( const FName& InName, FString InvalidChars=INVALID_NAME_CHARACTERS, FString* Reason=NULL )
{
	FString Name = InName.ToString();

	// See if the name contains invalid characters.
	FString Char;
	for( INT x = 0; x < InvalidChars.Len() ; ++x )
	{
		Char = InvalidChars.Mid( x, 1 );

		if( Name.InStr( Char ) != INDEX_NONE )
		{
			if ( Reason != NULL )
			{
				*Reason = FString::Printf( TEXT("Name contains an invalid character : [%s]"), *Char );
			}
			return FALSE;
		}
	}

	return TRUE;
}

/**
 * Takes an FName and checks to see that it follows the rules that Unreal requires.
 *
 * @param	InName			The name to check
 * @param	InReason		If the check fails, this string is filled in with the reason why.
 * @param	InInvalidChars	The set of invalid characters that the name cannot contain
 *
 * @return	1 if the name is valid, 0 if it is not
 */

inline UBOOL FIsValidXName( const FName& InName, FString& InReason, FString InvalidChars=INVALID_NAME_CHARACTERS )
{
	return FIsValidXName(InName,InvalidChars,&InReason);
}

inline UBOOL FIsValidObjectName( const FName& InName, FString& InReason )
{
	return FIsValidXName( InName, InReason, INVALID_OBJECTNAME_CHARACTERS );
}

inline UBOOL FIsValidGroupName( const FName& InName, FString& InReason )
{
	return FIsValidXName( InName, InReason, INVALID_PACKAGENAME_CHARACTERS );
}

/**
 * Takes an FName and checks to see that it is unique among all loaded objects.
 *
 * @param	InName		The name to check
 * @param	Outer		The context for validating this object name. Should be a group/package, but could be ANY_PACKAGE if you want to check across the whole system (not recommended)
 * @param	InReason	If the check fails, this string is filled in with the reason why.
 *
 * @return	1 if the name is valid, 0 if it is not
 */

inline UBOOL FIsUniqueObjectName( const FName& InName, UObject* Outer, FString* InReason=NULL )
{
	// See if the name is already in use.
	if( UObject::StaticFindObject( UObject::StaticClass(), Outer, *InName.ToString() ) != NULL )
	{
		if ( InReason != NULL )
		{
			*InReason = TEXT("Name is already in use by another object.");
		}
		return FALSE;
	}

	return TRUE;
}

/**
 * Takes an FName and checks to see that it is unique among all loaded objects.
 *
 * @param	InName		The name to check
 * @param	Outer		The context for validating this object name. Should be a group/package, but could be ANY_PACKAGE if you want to check across the whole system (not recommended)
 * @param	InReason	If the check fails, this string is filled in with the reason why.
 *
 * @return	1 if the name is valid, 0 if it is not
 */

inline UBOOL FIsUniqueObjectName( const FName& InName, UObject* Outer, FString& InReason )
{
	return FIsUniqueObjectName(InName,Outer,&InReason);
}


/**
 * Determines whether the specified array contains objects of the specified class.
 *
 * @param	ObjectArray		the array to search - must be an array of pointers to instances of a UObject-derived class
 * @param	ClassToCheck	the object class to search for
 * @param	bExactClass		TRUE to consider only those objects that have the class specified, or FALSE to consider objects
 *							of classes derived from the specified SearhClass as well
 * @param	out_Objects		if specified, any objects that match the SearchClass will be added to this array
 */
template <class T>
UBOOL ContainsObjectOfClass( const TArray<T*>& ObjectArray, UClass* ClassToCheck, UBOOL bExactClass=FALSE, TArray<T*>* out_Objects=NULL )
{
	UBOOL bResult = FALSE;
	for ( INT ArrayIndex = 0; ArrayIndex < ObjectArray.Num(); ArrayIndex++ )
	{
		if ( ObjectArray(ArrayIndex) != NULL )
		{
			UBOOL bMatchesSearchCriteria = bExactClass
				? ObjectArray(ArrayIndex)->GetClass() == ClassToCheck
				: ObjectArray(ArrayIndex)->IsA(ClassToCheck);

			if ( bMatchesSearchCriteria )
			{
				bResult = TRUE;
				if ( out_Objects != NULL )
				{
					out_Objects->AddItem(ObjectArray(ArrayIndex));
				}
				else
				{
					// if we don't need a list of objects that match the search criteria, we can stop as soon as we find at least one object of that class
					break;
				}
			}
		}
	}

	return bResult;
}

/*----------------------------------------------------------------------------
	Object iterators.
----------------------------------------------------------------------------*/

/**
 * Class for iterating through all objects, including class default objects.
 * Note that when Playing In Editor, this will find objects in the
 * editor as well as the PIE world, in an indeterminate order.
 */
class FObjectIterator
{
public:
	FObjectIterator( UClass* InClass=UObject::StaticClass() )
	:	Class( InClass ), Index( -1 )
	{
		check(Class);
		++*this;
	}
	//@warning: behavior is partially mirrored in UnObjGC.cpp. Make sure to adapt code there as well if you make changes below.
	void operator++()
	{		
		// We don't want to return any objects that are currently being background loaded unless we're using the object iterator during async loading.
		EObjectFlags ExclusionFlags	= RF_Unreachable;
		if( !GIsAsyncLoading )
		{
			ExclusionFlags |= RF_AsyncLoading;
		}
		while( ++Index<UObject::GObjObjects.Num() && (!UObject::GObjObjects(Index) || UObject::GObjObjects(Index)->HasAnyFlags(ExclusionFlags) || !UObject::GObjObjects(Index)->IsA(Class)) );
	}
	FORCEINLINE UObject* operator*()
	{
		return UObject::GObjObjects(Index);
	}
	FORCEINLINE UObject* operator->()
	{
		return UObject::GObjObjects(Index);
	}
	FORCEINLINE operator UBOOL()
	{
		return Index<UObject::GObjObjects.Num();
	}
protected:
	UClass* Class;
	INT Index;
};

/**
 * Class for iterating through all objects which inherit from a
 * specified base class.  Does not include any class default objects.
 * Note that when Playing In Editor, this will find objects in the
 * editor as well as the PIE world, in an indeterminate order.
 */
template< class T > class TObjectIterator : public FObjectIterator
{
public:
	TObjectIterator()
	:	FObjectIterator( T::StaticClass() )
	{
		if ( (*this) && (*this)->HasAnyFlags(RF_ClassDefaultObject) )
			++(*this);
	}
	void operator++()
	{
		do
		{
			FObjectIterator::operator++();
			// don't include class default objects in TObjectIterator
		} while( (*this) && (*this)->HasAnyFlags(RF_ClassDefaultObject) );
	}
	FORCEINLINE T* operator* ()
	{
		return (T*)FObjectIterator::operator*();
	}
	FORCEINLINE T* operator-> ()
	{
		return (T*)FObjectIterator::operator->();
	}
};

/** specialization for T == UObject that does not call IsA() unnecessarily */
template<> class TObjectIterator<UObject> : public FObjectIterator
{
public:
	TObjectIterator()
	{
		Class = UObject::StaticClass();
		Index = -1;
		++*this;
	}
	//@warning: partially copied from FObjectIterator
	void operator++()
	{		
		// We don't want to return any objects that are currently being background loaded unless we're using the object iterator during async loading.
		EObjectFlags ExclusionFlags	= RF_Unreachable | RF_ClassDefaultObject;
		if (!GIsAsyncLoading)
		{
			ExclusionFlags |= RF_AsyncLoading;
		}
		do
		{
			Index++;
		}
		while (this->operator UBOOL() && (this->operator*() == NULL || (*this)->HasAnyFlags(ExclusionFlags)));
	}
};

/**
 * Utility struct for restoring object flags for all objects.
 */
struct FScopedObjectFlagMarker
{
	/**
	 * Map that tracks the ObjectFlags set on all objects; we use a map rather than iterating over all objects twice because FObjectIterator
	 * won't return objects that have RF_Unreachable set, and we may want to actually unset that flag.
	 */
	TMap<UObject*,EObjectFlags> StoredObjectFlags;

	/**
	 * Stores the object flags for all objects in the tracking array.
	 */
	void SaveObjectFlags()
	{
		StoredObjectFlags.Empty();

		for ( FObjectIterator It; It; ++It )
		{
			StoredObjectFlags.Set(*It, It->GetFlags());
		}
	}

	/**
	 * Restores the object flags for all objects from the tracking array.
	 */
	void RestoreObjectFlags()
	{
		for ( TMap<UObject*,EObjectFlags>::TIterator It(StoredObjectFlags); It; ++It )
		{
			UObject* Object = It.Key();
			EObjectFlags PreviousObjectFlags = It.Value();

			// clear all flags
			Object->ClearFlags(RF_AllFlags);

			// then reset the ones that were originally set
			Object->SetFlags(PreviousObjectFlags);
		}
	}

	/** Constructor */
	FScopedObjectFlagMarker()
	{
		SaveObjectFlags();
	}

	/** Destructor */
	~FScopedObjectFlagMarker()
	{
		RestoreObjectFlags();
	}
};
