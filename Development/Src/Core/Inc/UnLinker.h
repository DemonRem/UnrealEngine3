/*=============================================================================
	UnLinker.h: Unreal object linker.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

// should really be a typedef, but that requires lots of code to be changed

/**
 * Index into a ULnker's ImportMap or ExportMap.
 * Values greater than zero indicate that this is an index into the ExportMap.  The
 * actual array index will be (PACKAGE_INDEX - 1).
 *
 * Values less than zero indicate that this is an index into the ImportMap. The actual
 * array index will be (-PACKAGE_INDEX - 1)
 */
#define PACKAGE_INDEX	INT

/**
 * A PACKAGE_INDEX of 0 for an FObjectResource's OuterIndex indicates a
 * top-level UPackage which will be the LinkerRoot for a ULinkerLoad
 */
#define ROOTPACKAGE_INDEX 0

/**
 * A PACKAGE_INDEX of 0 for an FObjectExport's ClassIndex indicates that the
 * export is a UClass object
 */
#define UCLASS_INDEX 0

/**
 * A PACKAGE_INDEX of 0 for an FObjectExport's SuperIndex indicates that this
 * object does not have a parent struct, either because it is not a UStruct-derived
 * object, or it does not extend any other UStruct
 */
#define NULLSUPER_INDEX 0

/**
 * A PACKAGE_INDEX of 0 for an FObjectExport's ArchetypeIndex indicate that the
 * ObjectArchetype for this export's object is the class default object.
 */
#define CLASSDEFAULTS_INDEX 0

/**
 * Clarify checks for import indexes
 */
#define IS_IMPORT_INDEX(index) (index < 0)

/**
 * Base class for UObject resource types.  FObjectResources are used to store UObjects on disk
 * via ULinker's ImportMap (for resources contained in other packages) and ExportMap (for resources
 * contained within the same package)
 */
struct FObjectResource
{
	/**
	 * The name of the UObject represented by this resource.
	 * Serialized
	 */
	FName			ObjectName;

	/**
	 * Location of the resource for this resource's Outer.  Values of 0 indicate that this resource
	 * represents a top-level UPackage object (the linker's LinkerRoot).
	 * Serialized
	 */
	PACKAGE_INDEX	OuterIndex;

	FObjectResource();
	FObjectResource( UObject* InObject );
};

/*-----------------------------------------------------------------------------
	FObjectExport.
-----------------------------------------------------------------------------*/

typedef DWORD EExportFlags;
/** No flags																*/
#define EF_None						0x00000000		
/** Whether the export was forced into the export table via RF_ForceTagExp.	*/
#define	EF_ForcedExport				0x00000001		
/** All flags																*/
#define EF_AllFlags					0xFFFFFFFF

/**
 * UObject resource type for objects that are contained within this package and can
 * be referenced by other packages.
 */
struct FObjectExport : public FObjectResource
{
	/**
	 * Location of the resource for this export's class (if non-zero).  A value of zero
	 * indicates that this export represents a UClass object; there is no resource for
	 * this export's class object
	 * Serialized
	 */
	PACKAGE_INDEX  	ClassIndex;

	/**
	 * Location of the resource for this export's SuperField (parent).  Only valid if
	 * this export represents a UStruct object. A value of zero indicates that the object
	 * represented by this export isn't a UStruct-derived object.
	 * Serialized
	 */
	PACKAGE_INDEX 	SuperIndex;

	/**
	 * Location of the resource for this resource's template.  Values of 0 indicate that this object's
	 * template is the class default object.
	 */
	PACKAGE_INDEX	ArchetypeIndex;

	/**
	 * The object flags for the UObject represented by this resource.  Only flags that
	 * match the RF_Load combination mask will be loaded from disk and applied to the UObject.
	 * Serialized
	 */
	EObjectFlags	ObjectFlags;

	/**
	 * The number of bytes to serialize when saving/loading this export's UObject.
	 * Serialized
	 */
	INT         	SerialSize;

	/**
	 * The location (into the ULinker's underlying file reader archive) of the beginning of the
	 * data for this export's UObject.  Used for verification only.
	 * Serialized
	 */
	INT         	SerialOffset;

	/**
	 * The location (into the ULinker's underlying file reader archive) of the beginning of the
	 * portion of this export's data that is serialized using script serialization.
	 * Transient
	 */
	INT				ScriptSerializationStartOffset;

	/**
	 * The location (into the ULinker's underlying file reader archive) of the end of the
	 * portion of this export's data that is serialized using script serialization.
	 * Transient
	 */
	INT				ScriptSerializationEndOffset;

	/**
	 * The UObject represented by this export.  Assigned the first time CreateExport is called for this export.
	 * Transient
	 */
	UObject*		_Object;

	/**
	 * The index into the ULinker's ExportMap for the next export in the linker's export hash table.
	 * Transient
	 */
	INT				_iHashNext;

	/**
	 * Name to ExportMap-index map for component templates objects.
	 * Persistent
	 */
	TMap<FName,INT>	ComponentMap;

	/**
	 * Set of flags/ attributes, e.g. including information whether the export was forced into the export 
	 * table via RF_ForceTagExp.
	 * Serialized
	 */
	EExportFlags	ExportFlags;

	/** If this object is a top level package (which must have been forced into the export table via RF_ForceTagExp)
	 * this contains the number of net serializable objects in each generation of that package
	 * Serialized
	 */
	TArray<INT> GenerationNetObjectCount;

	/** If this object is a top level package (which must have been forced into the export table via RF_ForceTagExp)
	 * this is the GUID for the original package file
	 * Serialized
	 */
	FGuid PackageGuid;

	/**
	 * Constructors
	 */
	FObjectExport();
	FObjectExport( UObject* InObject );
	
	/** I/O function */
	friend FArchive& operator<<( FArchive& Ar, FObjectExport& E );

	/**
	 * Retrieves export flags.
	 * @return Current export flags.
	 */
	FORCEINLINE EExportFlags GetFlags() const
	{
		return ExportFlags;
	}
	/**
	 * Sets the passed in flags
	 * @param NewFlags		Flags to set
	 */
	FORCEINLINE void SetFlags( EExportFlags NewFlags )
	{
		ExportFlags |= NewFlags;
	}
	/**
	 * Clears the passed in export flags.
	 * @param FlagsToClear	Flags to clear
	 */
	FORCEINLINE void ClearFlags( EExportFlags FlagsToClear )
	{
		ExportFlags &= ~FlagsToClear;
	}
	/**
	 * Used to safely check whether any of the passed in flags are set. This is required
	 * in case we extend EExportFlags to a 64 bit data type and keep UBOOL a 32 bit data 
	 * type so simply using GetFlags() & RF_MyFlagBiggerThanMaxInt won't work correctly 
	 * when assigned directly to an UBOOL.
	 *
	 * @param FlagsToCheck	Export flags to check for.
	 * @return				TRUE if any of the passed in flags are set, FALSE otherwise  (including no flags passed in).
	 */
	FORCEINLINE UBOOL HasAnyFlags( EExportFlags FlagsToCheck ) const
	{
		return (ExportFlags & FlagsToCheck) != 0 || FlagsToCheck == EF_AllFlags;
	}
	/**
	 * Used to safely check whether all of the passed in flags are set. This is required
	 * in case we extend EExportFlags to a 64 bit data type and keep UBOOL a 32 bit data 
	 * type so simply using GetFlags() & RF_MyFlagBiggerThanMaxInt won't work correctly 
	 * when assigned directly to an UBOOL.
	 *
	 * @param FlagsToCheck	Object flags to check for
	 * @return TRUE if all of the passed in flags are set (including no flags passed in), FALSE otherwise
	 */
	FORCEINLINE UBOOL HasAllFlags( EExportFlags FlagsToCheck ) const
	{
		return ((ExportFlags & FlagsToCheck) == FlagsToCheck);
	}
	/**
	 * Returns object flags that are both in the mask and set on the object.
	 *
	 * @param Mask	Mask to mask object flags with
	 * @return Objects flags that are set in both the object and the mask
	 */
	FORCEINLINE EExportFlags GetMaskedFlags( EExportFlags Mask ) const
	{
		return ExportFlags & Mask;
	}
};

/*-----------------------------------------------------------------------------
	FObjectImport.
-----------------------------------------------------------------------------*/

/**
 * UObject resource type for objects that are referenced by this package, but contained
 * within another package.
 */
struct FObjectImport : public FObjectResource
{
	/**
	 * The name of the package that contains the class of the UObject represented by this resource.
	 * Serialized
	 */
	FName			ClassPackage;

	/**
	 * The name of the class for the UObject represented by this resource.
	 * Serialized
	 */
	FName			ClassName;

	/**
	 * The UObject represented by this resource.  Assigned the first time CreateImport is called for this import.
	 * Transient
	 */
	UObject*		XObject;

	/**
	 * The linker that contains the original FObjectExport resource associated with this import.
	 * Transient
	 */
	ULinkerLoad*	SourceLinker;

	/**
	 * Index into SourceLinker's ExportMap for the export associated with this import's UObject.
	 * Transient
	 */
	INT             SourceIndex;

	/**
	 * Constructors
	 */
	FObjectImport();
	FObjectImport( UObject* InObject );
	
	/** I/O function */
	friend FArchive& operator<<( FArchive& Ar, FObjectImport& I );
};

/**
 * Information about a compressed chunk in a file.
 */
struct FCompressedChunk
{
	/** Default constructor, zero initializing all members. */
	FCompressedChunk();

	/** Original offset in uncompressed file.	*/
	INT		UncompressedOffset;
	/** Uncompressed size in bytes.				*/
	INT		UncompressedSize;
	/** Offset in compressed file.				*/
	INT		CompressedOffset;
	/** Compressed size in bytes.				*/
	INT		CompressedSize;

	/** I/O function */
	friend FArchive& operator<<(FArchive& Ar,FCompressedChunk& Chunk);
};

/*----------------------------------------------------------------------------
	Items stored in Unrealfiles.
----------------------------------------------------------------------------*/

/**
 * Revision data for an Unreal package file.
 */
//@todo: shouldn't need ExportCount/NameCount with the linker free package map; if so, clean this stuff up
struct FGenerationInfo
{
	/**
	 * Number of exports in the linker's ExportMap for this generation.
	 */
	INT ExportCount;

	/**
	 * Number of names in the linker's NameMap for this generation.
	 */
	INT NameCount;

	/** number of net serializable objects in the package for this generation */
	INT NetObjectCount;

	/** Constructor */
	FGenerationInfo( INT InExportCount, INT InNameCount, INT InNetObjectCount );

	/** I/O function
	 * we use a function instead of operator<< so we can pass in the package file summary for version tests, since Ar.Ver() hasn't been set yet
	 */
	void Serialize(FArchive& Ar, const struct FPackageFileSummary& Summary);
};

/**
 * A "table of contents" for an Unreal package file.  Stored at the top of the file.
 */
struct FPackageFileSummary
{
	/**
	 * Magic tag compared against PACKAGE_FILE_TAG to ensure that package is an Unreal package.
	 */
	INT		Tag;

protected:
	/**
	 * The packge file version number when this package was saved.
	 *
	 * Lower 16 bits stores the main engine version
	 * Upper 16 bits stores the licensee version
	 */
	INT		FileVersion;

public:
	/**
	 * Total size of all information that needs to be read in to create a ULinkerLoad. This includes
	 * the package file summary, name table and import & export maps.
	 */
	INT		TotalHeaderSize;

	/**
	 * The flags for the package
	 */
	DWORD	PackageFlags;

	/**
	 * The Generic Browser folder name that this package lives in
	 */
	FString	FolderName;

	/**
	 * Number of names used in this package
	 */
	INT		NameCount;

	/**
	 * Location into the file on disk for the name data
	 */
	INT 	NameOffset;

	/**
	 * Number of exports contained in this package
	 */
	INT		ExportCount;

	/**
	 * Location into the file on disk for the ExportMap data
	 */
	INT		ExportOffset;

	/**
	 * Number of imports contained in this package
	 */
	INT     ImportCount;

	/**
	 * Location into the file on disk for the ImportMap data
	 */
	INT		ImportOffset;

	/**
	* Location into the file on disk for the DependsMap data
	*/
	INT		DependsOffset;

	/**
	 * Current id for this package
	 */
	FGuid	Guid;

	/**
	 * Data about previous versions of this package
	 */
	TArray<FGenerationInfo> Generations;

	/**
	 * Engine version this package was saved with.
	 */
	INT		EngineVersion;

	/**
	 * CookedContent version this package was saved with.
	 *
	 * This is used to make certain that the content in the Cooked dir is the correct cooked
	 * version.  So we can just auto cook content and it will do the correct thing.
	 */
	INT     CookedContentVersion;

	/**
	 * Flags used to compress the file on save and uncompress on load.
	 */
	DWORD	CompressionFlags;

	/**
	 * Array of compressed chunks in case this package was stored compressed.
	 */
	TArray<FCompressedChunk> CompressedChunks;

	/** Constructor */
	FPackageFileSummary();

	INT GetFileVersion() const
	{
		// This code is mirrored in the << operator for FPackageFileSummary found in UnLinker.cpp 
		return (FileVersion & 0xffff); 
	}

	INT GetFileVersionLicensee() const
	{
		return ((FileVersion >> 16) & 0xffff);
	}

	INT GetCookedContentVersion() const
	{
		return CookedContentVersion;
	}

	void SetFileVersions(INT Epic, INT Licensee)
	{
		FileVersion = ((Licensee << 16) | Epic);
	}

	/** I/O function */
	friend FArchive& operator<<( FArchive& Ar, FPackageFileSummary& Sum );
};

/*----------------------------------------------------------------------------
	ULinker.
----------------------------------------------------------------------------*/

/**
 * Manages the data associated with an Unreal package.  Acts as the bridge between
 * the file on disk and the UPackage object in memory for all Unreal package types.
 */
class ULinker : public UObject
{
	DECLARE_CLASS(ULinker,UObject,CLASS_Transient|CLASS_Intrinsic,Core)
	NO_DEFAULT_CONSTRUCTOR(ULinker)

	/** The top-level UPackage object for the package associated with this linker */
	UPackage*				LinkerRoot;

	/** Table of contents for this package's file */
	FPackageFileSummary		Summary;

	/** Names used by objects contained within this package */
	TArray<FName>			NameMap;

	/** Resources for all UObjects referenced by this package */
	TArray<FObjectImport>	ImportMap;

	/** Resources for all UObjects contained within this package */
	TArray<FObjectExport>	ExportMap;

	/** Mapping of exports to all imports they need (not in the ExportMap so it can be easily skipped over on seekfree packages) */
	TArray<TArray<INT> > DependsMap;

	/** The name of the file for this package */
	FString					Filename;

	/** Used to filter out exports that should not be created */
	EObjectFlags			_ContextFlags;

#if TRACK_SERIALIZATION_PERFORMANCE || LOOKING_FOR_PERF_ISSUES
	/** Per linker serialization performance data */
	class FStructEventMap	SerializationPerfTracker;
#endif

	/** Constructor. */
	ULinker( UPackage* InRoot, const TCHAR* InFilename );

	/**
	 * Static constructor called once per class during static initialization via IMPLEMENT_CLASS
	 * macro. Used to e.g. emit object reference tokens for realtime garbage collection or expose
	 * properties for native- only classes.
	 */
	void StaticConstructor();

	/**
	 * I/O function
	 * 
	 * @param	Ar	the archive to read/write into
	 */
	void Serialize( FArchive& Ar );

	/**
	 * Return the path name of the UObject represented by the specified import. 
	 * (can be used with StaticFindObject)
	 * 
	 * @param	ImportIndex	index into the ImportMap for the resource to get the name for
	 *
	 * @return	the path name of the UObject represented by the resource at ImportIndex
	 */
	FString GetImportPathName(INT ImportIndex);

	/**
	 * Return the path name of the UObject represented by the specified export.
	 * (can be used with StaticFindObject)
	 * 
	 * @param	ExportIndex				index into the ExportMap for the resource to get the name for
	 * @param	FakeRoot				Optional name to replace use as the root package of this object instead of the linker
	 * @param	bResolveForcedExports	if TRUE, the package name part of the return value will be the export's original package,
	 *									not the name of the package it's currently contained within.
	 *
	 * @return	the path name of the UObject represented by the resource at ExportIndex
	 */
	FString GetExportPathName(INT ExportIndex, const TCHAR* FakeRoot=NULL,UBOOL bResolveForcedExports=FALSE);

	/**
	 * Return the full name of the UObject represented by the specified import.
	 * 
	 * @param	ImportIndex	index into the ImportMap for the resource to get the name for
	 *
	 * @return	the full name of the UObject represented by the resource at ImportIndex
	 */
	FString GetImportFullName(INT ImportIndex);

	/**
	 * Return the full name of the UObject represented by the specified export.
	 * 
	 * @param	ExportIndex				index into the ExportMap for the resource to get the name for
	 * @param	FakeRoot				Optional name to replace use as the root package of this object instead of the linker
	 * @param	bResolveForcedExports	if TRUE, the package name part of the return value will be the export's original package,
	 *									not the name of the package it's currently contained within.
	 *
	 * @return	the full name of the UObject represented by the resource at ExportIndex
	 */
	FString GetExportFullName(INT ExportIndex, const TCHAR* FakeRoot=NULL,UBOOL bResolveForcedExports=FALSE);
};

/*----------------------------------------------------------------------------
	ULinkerLoad.
----------------------------------------------------------------------------*/

/**
 * Helper struct to keep track of all objects needed by an export (recursive dependency caching)
 */
struct FDependencyRef
{
	/** The Linker the export lives in */
	ULinkerLoad* Linker;

	/** Index into Linker's ExportMap for this object */
	INT ExportIndex;

	/**
	 * Comparison operator
	 */
	UBOOL operator==(const FDependencyRef& Other) const
	{
		return Linker == Other.Linker && ExportIndex == Other.ExportIndex;
	}
};

/**
 * Handles loading Unreal package files, including reading UObject data from disk.
 */
class ULinkerLoad : public ULinker, public FArchive
{
	DECLARE_CLASS(ULinkerLoad,ULinker,CLASS_Transient|CLASS_Intrinsic,Core)
	NO_DEFAULT_CONSTRUCTOR(ULinkerLoad)

	// Friends.
	friend class UObject;
	friend class UPackageMap;
	friend struct FAsyncPackage;

	// Variables.
public:
	/** Flags determining loading behavior.																					*/
	DWORD					LoadFlags;
	/** Indicates whether the imports for this loader have been verified													*/
	UBOOL					bHaveImportsBeenVerified;
	/** Hash table for exports.																								*/
	INT						ExportHash[256];
	/** Bulk data that does not need to be loaded when the linker is loaded.												*/
	TArray<FUntypedBulkData*> BulkDataLoaders;
	/** The archive that actually reads the raw data from disk.																*/
	FArchive*				Loader;

private:
	// Variables used during async linker creation.

	/** Current index into name map, used by async linker creation for spreading out serializing name entries.				*/
	INT						NameMapIndex;
	/** Current index into import map, used by async linker creation for spreading out serializing importmap entries.		*/	
	INT						ImportMapIndex;
	/** Current index into export map, used by async linker creation for spreading out serializing exportmap entries.		*/
	INT						ExportMapIndex;
	/** Current index into depends map, used by async linker creation for spreading out serializing dependsmap entries.		*/
	INT						DependsMapIndex;
	/** Current index into export hash map, used by async linker creation for spreading out hashing exports.				*/
	INT						ExportHashIndex;


	/** Whether we already serialized the package file summary.																*/
	UBOOL					bHasSerializedPackageFileSummary;
	/** Whether we already fixed up import map.																				*/
	UBOOL					bHasFixedUpImportMap;
	/** Whether we already matched up existing exports.																		*/
	UBOOL					bHasFoundExistingExports;
	/** Whether we are already fully initialized.																			*/
	UBOOL					bHasFinishedInitialization;

	/** Whether time limit is/ has been exceeded in current/ last tick.														*/
	UBOOL					bTimeLimitExceeded;
	/** Call count of IsTimeLimitExceeded.																					*/
	INT						IsTimeLimitExceededCallCount;
	/** Whether to use a time limit for async linker creation.																*/
	UBOOL					bUseTimeLimit;
	/** Current time limit to use if bUseTimeLimit is TRUE.																	*/
	FLOAT					TimeLimit;
	/** Time at begin of Tick function. Used for time limit determination.													*/
	DOUBLE					TickStartTime;

	/** Whether we are gathering dependencies, can be used to streamline VerifyImports, etc									*/
	UBOOL					bIsGatheringDependencies;
	/**
	 * Helper struct to keep track of background file reads
	 */
	struct FPackagePrecacheInfo
	{
		/** Synchronization object used to wait for completion of async read. POinter so it can be copied around, etc */
		FThreadSafeCounter* SynchronizationObject;

		/** Memory that contains the package data read off disk */
		void* PackageData;

		/** Size of the buffer pointed to by PackageData */
		INT PackageDataSize;

		/**
		 * Basic constructor
		 */
		FPackagePrecacheInfo()
		: SynchronizationObject(NULL)
		, PackageData(NULL)
		, PackageDataSize(0)
		{
		}
		/**
		 * Destructor that will free the sync object
		 */
		~FPackagePrecacheInfo()
		{
			delete SynchronizationObject;
		}
	};

	/** Map that keeps track of any precached full package reads															*/
	static TMap<FString, FPackagePrecacheInfo> PackagePrecacheMap;

public:
	/**
	 * Returns whether linker has finished (potentially) async initializtion.
	 *
	 * @return TRUE if initialized, FALSE if pending.
	 */
	UBOOL HasFinishedInitializtion() const
	{
        return bHasFinishedInitialization;
	}

	/**
	 * If this archive is a ULinkerLoad or ULinkerSave, returns a pointer to the ULinker portion.
	 */
	virtual ULinker* GetLinker() { return this; }

	/**
	 * Creates and returns a ULinkerLoad object.
	 *
	 * @param	Parent		Parent object to load into, can be NULL (most likely case)
	 * @param	Filename	Name of file on disk to load
	 * @param	LoadFlags	Load flags determining behavior
	 *
	 * @return	new ULinkerLoad object for Parent/ Filename
	 */
	static ULinkerLoad* CreateLinker( UPackage* Parent, const TCHAR* Filename, DWORD LoadFlags );

	void Verify();

	FName GetExportClassPackage( INT i );
	FName GetExportClassName( INT i );
	virtual FName GetArchiveName() const;

	/**
	 * Recursively gathers the dependencies of a given export (the recursive chain of imports
	 * and their imports, and so on)

	 * @param ExportIndex Index into the linker's ExportMap that we are checking dependencies
	 * @param Dependencies Array of all dependencies needed
	 */
	void GatherExportDependencies(INT ExportIndex, TArray<FDependencyRef>& Dependencies);

	/**
	 * Recursively gathers the dependencies of a given import (the recursive chain of imports
	 * and their imports, and so on)

	 * @param ImportIndex Index into the linker's ImportMap that we are checking dependencies
	 * @param Dependencies Array of all dependencies needed
	 */
	void GatherImportDependencies(INT ImportIndex, TArray<FDependencyRef>& Dependencies);

	/**
	 * A wrapper around VerifyImportInner. If the VerifyImportInner (previously VerifyImport) fails, this function
	 * will look for a UObjectRedirector that will point to the real location of the object. You will see this if
	 * an object was renamed to a different package or group, but something that was referencing the object was not
	 * not currently open. (Rename fixes up references of all loaded objects, but naturally not for ones that aren't
	 * loaded).
	 *
	 * @param	i	The index into this package's ImportMap to verify
	 */
	void VerifyImport( INT i );
	
	/**
	 * Loads all objects in package.
	 *
	 * @param bForcePreload	Whether to explicitly call Preload (serialize) right away instead of being
	 *						called from EndLoad()
	 */
	void LoadAllObjects( UBOOL bForcePreload = FALSE );

	/**
	 * Returns the ObjectName associated with the resource indicated.
	 * 
	 * @param	ResourceIndex	location of the object resource
	 *
	 * @return	ObjectName for the FObjectResource at ResourceIndex, or NAME_None if not found
	 */
	FName ResolveResourceName( PACKAGE_INDEX ResourceIndex );
	
	INT FindExportIndex( FName ClassName, FName ClassPackage, FName ObjectName, INT ExportOuterIndex );
	
	/**
	 * Function to create the instance of, or verify the presence of, an object as found in this Linker.
	 *
	 * @param ObjectClass	The class of the object
	 * @param ObjectName	The name of the object
	 * @param Outer			Optional outer that this object must be in (for finding objects in a specific group when there are multiple groups with the same name)
	 * @param LoadFlags		Flags used to determine if the object is being verified or should be created
	 * @param Checked		Whether or not a failure will throw an error
	 * @return The created object, or (UObject*)-1 if this is just verifying
	 */
	UObject* Create( UClass* ObjectClass, FName ObjectName, UObject* Outer, DWORD LoadFlags, UBOOL Checked );

	/**
	 * Serialize the object data for the specified object from the unreal package file.  Loads any
	 * additional resources required for the object to be in a valid state to receive the loaded
	 * data, such as the object's Outer, Class, or ObjectArchetype.
	 *
	 * When this function exits, Object is guaranteed to contain the data stored that was stored on disk.
	 *
	 * @param	Object	The object to load data for.  If the data for this object isn't stored in this
	 *					ULinkerLoad, routes the call to the appropriate linker.  Data serialization is 
	 *					skipped if the object has already been loaded (as indicated by the RF_NeedLoad flag
	 *					not set for the object), so safe to call on objects that have already been loaded.
	 *					Note that this function assumes that Object has already been initialized against
	 *					its template object.
	 *					If Object is a UClass and the class default object has already been created, calls
	 *					Preload for the class default object as well.
	 */
	void Preload( UObject* Object );

	/**
	 * Before loading a persistent object from disk, this function can be used to discover
	 * the object in memory. This could happen in the editor when you save a package (which
	 * destroys the linker) and then play PIE, which would cause the Linker to be
	 * recreated. However, the objects are still in memory, so there is no need to reload
	 * them.
	 *
	 * @param ExportIndex	The index of the export to hunt down
	 * @return The object that was found, or NULL if it wasn't found
	 */
	UObject* FindExistingExport(INT ExportIndex);

	/**
	 * Kick off an async load of a package file into memory
	 * 
	 * @param PackageName Name of package to read in. Must be the same name as passed into LoadPackage/CreateLinker
	 */
	static void AsyncPreloadPackage(const TCHAR* PackageName);

	/**
	 * Called when an object begins serializing property data using script serialization.
	 */
	virtual void MarkScriptSerializationStart( const UObject* Obj );

	/**
	 * Called when an object stops serializing property data using script serialization.
	 */
	virtual void MarkScriptSerializationEnd( const UObject* Obj );

private:
	UObject* CreateExport( INT Index );
	UObject* CreateImport( INT Index );
	UObject* IndexToObject( PACKAGE_INDEX Index );

	void DetachExport( INT i );

	// UObject interface.
	void BeginDestroy();

	// FArchive interface.
	/**
	 * Hint the archive that the region starting at passed in offset and spanning the passed in size
	 * is going to be read soon and should be precached.
	 *
	 * The function returns whether the precache operation has completed or not which is an important
	 * hint for code knowing that it deals with potential async I/O. The archive is free to either not 
	 * implement this function or only partially precache so it is required that given sufficient time
	 * the function will return TRUE. Archives not based on async I/O should always return TRUE.
	 *
	 * This function will not change the current archive position.
	 *
	 * @param	PrecacheOffset	Offset at which to begin precaching.
	 * @param	PrecacheSize	Number of bytes to precache
	 * @return	FALSE if precache operation is still pending, TRUE otherwise
	 */
	virtual UBOOL Precache( INT PrecacheOffset, INT PrecacheSize );
	
	/**
	 * Attaches/ associates the passed in bulk data object with the linker.
	 *
	 * @param	Owner		UObject owning the bulk data
	 * @param	BulkData	Bulk data object to associate
	 */
	virtual void AttachBulkData( UObject* Owner, FUntypedBulkData* BulkData );
	/**
	 * Detaches the passed in bulk data object from the linker.
	 *
	 * @param	BulkData	Bulk data object to detach
	 * @param	bEnsureBulkDataIsLoaded	Whether to ensure that the bulk data is loaded before detaching
	 */
	virtual void DetachBulkData( FUntypedBulkData* BulkData, UBOOL bEnsureBulkDataIsLoaded );
	/**
	 * Detaches all attached bulk  data objects.
	 *
	 * @param	bEnsureBulkDataIsLoaded	Whether to ensure that the bulk data is loaded before detaching
	 */
	virtual void DetachAllBulkData( UBOOL bEnsureBulkDataIsLoaded );

	/**
	 * Detaches linker from bulk data/ exports and removes itself from array of loaders.
	 *
	 * @param	bEnsureAllBulkDataIsLoaded	Whether to load all bulk data first before detaching.
	 */
	virtual void Detach( UBOOL bEnsureAllBulkDataIsLoaded );


	void Seek( INT InPos );
	INT Tell();
	INT TotalSize();
	void Serialize( void* V, INT Length );
	virtual FArchive& operator<<( UObject*& Object );
	virtual FArchive& operator<<( FName& Name );

	/**
	 * Safely verify that an import in the ImportMap points to a good object. This decides whether or not
	 * a failure to load the object redirector in the wrapper is a fatal error or not (return value)
	 *
	 * @param	i				The index into this packages ImportMap to verify
	 * @param	WarningSuffix	[out] additional information about the load failure that should be appended to
	 *							the name of the object in the load failure dialog.
	 *
	 * @return TRUE if the wrapper should crash if it can't find a good object redirector to load
	 */
	UBOOL VerifyImportInner(INT i, FString& WarningSuffix);

	//
	// ULinkerLoad creation helpers BEGIN
	//

	/**
	 * Creates a ULinkerLoad object for async creation. Tick has to be called manually till it returns
	 * TRUE in which case the returned linker object has finished the async creation process.
	 *
	 * @param	Parent		Parent object to load into, can be NULL (most likely case)
	 * @param	Filename	Name of file on disk to load
	 * @param	LoadFlags	Load flags determining behavior
	 *
	 * @return	new ULinkerLoad object for Parent/ Filename
	 */
	static ULinkerLoad* CreateLinkerAsync( UPackage* Parent, const TCHAR* Filename, DWORD LoadFlags );

	/**
	 * Ticks an in-flight linker and spends InTimeLimit seconds on creation. This is a soft time limit used
	 * if bInUseTimeLimit is TRUE.
	 *
	 * @param	InTimeLimit		Soft time limit to use if bInUseTimeLimit is TRUE
	 * @param	bInUseTimeLimit	Whether to use a (soft) timelimit
	 * 
	 * @return	TRUE if linker has finished creation, FALSE if it is still in flight
	 */
	UBOOL Tick( FLOAT InTimeLimit, UBOOL bInUseTimeLimit );

	/**
	 * Private constructor, passing arguments through from CreateLinker.
	 *
	 * @param	Parent		Parent object to load into, can be NULL (most likely case)
	 * @param	Filename	Name of file on disk to load
	 * @param	LoadFlags	Load flags determining behavior
	 */
	ULinkerLoad( UPackage* InParent, const TCHAR* InFilename, DWORD InLoadFlags );

	/**
	 * Returns whether the time limit allotted has been exceeded, if enabled.
	 *
	 * @param CurrentTask	description of current task performed for logging spilling over time limit
	 * @param Granularity	Granularity on which to check timing, useful in cases where appSeconds is slow (e.g. PC)
	 *
	 * @return TRUE if time limit has been exceeded (and is enabled), FALSE otherwise (including if time limit is disabled)
	 */
	UBOOL IsTimeLimitExceeded( const TCHAR* CurrentTask, INT Granularity = 1 );

	/**
	 * Creates loader used to serialize content.
	 */
	UBOOL CreateLoader();

	/**
	 * Serializes the package file summary.
	 */
	UBOOL SerializePackageFileSummary();

	/**
	 * Serializes the name map.
	 */
	UBOOL SerializeNameMap();

	/**
	 * Serializes the import map.
	 */
	UBOOL SerializeImportMap();

	/**
	 * Fixes up the import map, performing remapping for backward compatibility and such.
	 */
	UBOOL FixupImportMap();

	/**
	 * Serializes the export map.
	 */
	UBOOL SerializeExportMap();

	/**
	 * Serializes the import map.
	 */
	UBOOL SerializeDependsMap();

	/** 
	 * Creates the export hash.
	 */
	UBOOL CreateExportHash();

	/**
	 * Finds existing exports in memory and matches them up with this linker. This is required for PIE to work correctly
	 * and also for script compilation as saving a package will reset its linker and loading will reload/ replace existing
	 * objects without a linker.
	 */
	UBOOL FindExistingExports();

	/**
	 * Finalizes linker creation, adding linker to loaders array and potentially verifying imports.
	 */
	UBOOL FinalizeCreation();

	//
	// ULinkerLoad creation helpers END
	//
};

/*----------------------------------------------------------------------------
	ULinkerSave.
----------------------------------------------------------------------------*/

/**
 * Handles saving Unreal package files.
 */
class ULinkerSave : public ULinker, public FArchive
{
	DECLARE_CLASS(ULinkerSave,ULinker,CLASS_Transient|CLASS_Intrinsic,Core);
	NO_DEFAULT_CONSTRUCTOR(ULinkerSave);

	// Variables.
	/** The archive that actually writes the data to disk. */
	FArchive* Saver;

	/** Index array - location of the resource for a UObject is stored in the ObjectIndices array using the UObject's Index */
	TArray<PACKAGE_INDEX> ObjectIndices;

	/** Index array - location of the name in the NameMap array for each FName is stored in the NameIndices array using the FName's Index */
	TArray<INT> NameIndices;

	/** Constructor */
	ULinkerSave( UPackage* InParent, const TCHAR* InFilename, UBOOL bForceByteSwapping );
	void BeginDestroy();

	// FArchive interface.
	virtual INT MapName( const FName* Name ) const;
	virtual INT MapObject( const UObject* Object ) const;
	FArchive& operator<<( FName& InName )
	{
		INT Save = NameIndices(InName.GetIndex());
		INT Number = InName.GetNumber();
		FArchive& Ar = *this;
		return Ar << Save << Number;
	}
	FArchive& operator<<( UObject*& Obj )
	{
		INT Save = Obj ? ObjectIndices(Obj->GetIndex()) : 0;
		FArchive& Ar = *this;
		return Ar << Save;
	}

	/**
	 * If this archive is a ULinkerLoad or ULinkerSave, returns a pointer to the ULinker portion.
	 */
	virtual ULinker* GetLinker() { return this; }

	void Seek( INT InPos );
	INT Tell();
	void Serialize( void* V, INT Length );

	/**
	 * Detaches file saver and hence file handle.
	 */
	void Detach();
};


/*-----------------------------------------------------------------------------
	Lazy loading.
-----------------------------------------------------------------------------*/

/**
 * This will log out that an LazyArray::Load has occurred IFF 
 * #if defined(LOG_LAZY_ARRAY_LOADS) || LOOKING_FOR_PERF_ISSUES
 * is true.
 *
 **/
void LogLazyArrayPerfIssue();

/**
 * Flags serialized with the lazy loader.
 */
typedef DWORD ELazyLoaderFlags;

/**
 * Empty flag set.
 */
#define LLF_None					0x00000000
/**
 * If set, payload is [going to be] stored in separate file		
 */
#define	LLF_PayloadInSeparateFile	0x00000001
/**
 * If set, payload should be [un]compressed during serialization. Only bulk data that doesn't require 
 * any special serialization or endian conversion can be compressed! The code will simply serialize a 
 * block from disk and use the byte order agnostic Serialize( Data, Length ) function to fill the memory.
 */
#define	LLF_SerializeCompressed		0x00000002
/**
 * Mask of all flags
 */
#define	LLF_AllFlags				0xFFFFFFFF

/**
 * Lazy loader base class.
 *
 * @deprecated with VER_REPLACED_LAZY_ARRAY_WITH_UNTYPED_BULK_DATA
 */
class FLazyLoaderDeprecated
{
protected:
	/** Archive this lazy loader is attached to, NULL if detached.			*/
	FArchive*	AttachedAr;
	/** Size in bytes of payload, if compressed is compressed size			*/
	INT			PayloadSize;
	/** Flags associated with lazy loader.									*/
	DWORD		LazyLoaderFlags;
	/** Offset of payload in archive/ file.									*/
	INT			PayloadOffset;
	/** Filename of Payload if stored in separate file, NAME_None otherwise	*/
	FName		PayloadFilename;

public:
	/**
	 * Default constructor initializing all its member variables.
	 */
	FLazyLoaderDeprecated()
	: AttachedAr( NULL )
	, PayloadSize( INDEX_NONE )
	, LazyLoaderFlags( 0 )
	, PayloadOffset( INDEX_NONE )
	, PayloadFilename( NAME_None )
	{}

	/**
	 * Virtual destructor, detaching loader if it has been attached to an archive.
	 */
	virtual ~FLazyLoaderDeprecated()
	{
		Detach();
	}
	
	/**
	 * Tells lazy loader to load it's payload.
	 */
	virtual void Load()=0;
	/**
	 * Tells lazy loader to unload it's payload.
	 */
	virtual void Unload()=0;

	/**
	 * Detaches lazy loader from attached archive.
	 */
	void Detach()
	{
		if( AttachedAr )
		{
			check(HasPayloadInSeparateFile()==FALSE);
			//AttachedAr->DetachLazyLoader( this );
		}
	}

	/** 
	 * Returns whether we are attached to an archive.
	 *
	 * @return TRUE if we are attached, FALSE, otherwise
	 */
	FORCEINLINE UBOOL IsAttached()
	{
		return AttachedAr != NULL;
	}
	
	/** 
	 * Returns whether this lazy loaders payload is stored separately.
	 *
	 * @return TRUE if stored separately, FALSE otherwise
	 */
	FORCEINLINE UBOOL HasPayloadInSeparateFile()
	{
		return HasAnyFlags( LLF_PayloadInSeparateFile );
	}

	/**
	 * Validates internal state and asserts on error. Always returns TRUE so we can use it from within check
	 * and it can be compiled out but still assert in the appropriate place in non final release builds.
	 *
	 * @return TRUE
	 */
	UBOOL IsInternalStateValid()
	{
		if( HasPayloadInSeparateFile() )
		{
			check( AttachedAr == NULL );
			check( PayloadOffset != INDEX_NONE );
			check( PayloadFilename != NAME_None );
		}
		else
		{
			check( !AttachedAr || (PayloadOffset != INDEX_NONE) );
		}
		// Dummy return as we're rather asserting on the explicit line.
		return TRUE;
	}

	/**
	 * Retrieve the filename associated with this loader. Used by e.g. the streaming code.
	 *
	 * @param Parent		Parent UObject containing the FLazyLoader
	 * @param OutFilname	[out] Filename to set
	 * @return TRUE if filename could be determined, FALSE otherwise
	 */
	UBOOL RetrieveFilename( UObject* Parent, FString& OutFilename )
	{
		// Payload filename was stored with loader.
		if( PayloadFilename != NAME_None )
		{
			return GPackageFileCache->FindPackageFile( *PayloadFilename.ToString(), NULL, OutFilename );
		}
		// We can use the linker's file name in case we're attached and the parent has a linker. Calling SetLinker on an object
		// won't detach the lazy loader which makes the check for a non NULL linker important.
		else if( Parent->GetLinker() && IsAttached() )
		{
			// We are guaranteed to have a linker if we're attached.
			check( Parent->GetLinker() );
			// Use the filename from the parent's linker.
			OutFilename = Parent->GetLinker()->Filename;
			// Success.
			return TRUE;
		}
		// Loaders have been reset on this object or its linker.
		else if( !Parent->GetLinker() )
		{
			// Use the name of the parent's package as a key into the package file cache.
			return GPackageFileCache->FindPackageFile( *Parent->GetOutermost()->GetName(), NULL, OutFilename );
		}
		// We're neither attached nor have the payload store in a separate file nor have a linker. Not sure
		// how we could've gotten here.	
		else
		{
			appErrorf(TEXT("Fatal error calling RetrieveFilename on %s."),*Parent->GetFullName());
			return FALSE;
		}
	}

	/**
	 * Retrieves export flags.
	 * @return Current export flags.
	 */
	FORCEINLINE ELazyLoaderFlags GetFlags() const
	{
		return LazyLoaderFlags;
	}
	/**
	 * Sets the passed in flags
	 * @param NewFlags		Flags to set
	 */
	FORCEINLINE void SetFlags( ELazyLoaderFlags NewFlags )
	{
		LazyLoaderFlags |= NewFlags;
	}
	/**
	 * Clears the passed in export flags.
	 * @param FlagsToClear	Flags to clear
	 */
	FORCEINLINE void ClearFlags( ELazyLoaderFlags FlagsToClear )
	{
		LazyLoaderFlags &= ~FlagsToClear;
	}
	/**
	 * Used to safely check whether any of the passed in flags are set. This is required
	 * in case we extend ELazyLoaderFlags to a 64 bit data type and keep UBOOL a 32 bit data 
	 * type so simply using GetFlags() & LLF_MyFlagBiggerThanMaxInt won't work correctly 
	 * when assigned directly to an UBOOL.
	 *
	 * @param FlagsToCheck	Export flags to check for.
	 * @return				TRUE if any of the passed in flags are set, FALSE otherwise  (including no flags passed in).
	 */
	FORCEINLINE UBOOL HasAnyFlags( ELazyLoaderFlags FlagsToCheck ) const
	{
		return (LazyLoaderFlags & FlagsToCheck) != 0 || FlagsToCheck == LLF_AllFlags;
	}
	/**
	 * Used to safely check whether all of the passed in flags are set. This is required
	 * in case we extend ELazyLoaderFlags to a 64 bit data type and keep UBOOL a 32 bit data 
	 * type so simply using GetFlags() & LLF_MyFlagBiggerThanMaxInt won't work correctly 
	 * when assigned directly to an UBOOL.
	 *
	 * @param FlagsToCheck	Object flags to check for
	 * @return TRUE if all of the passed in flags are set (including no flags passed in), FALSE otherwise
	 */
	FORCEINLINE UBOOL HasAllFlags( ELazyLoaderFlags FlagsToCheck ) const
	{
		return ((LazyLoaderFlags & FlagsToCheck) == FlagsToCheck);
	}
	/**
	 * Returns object flags that are both in the mask and set on the object.
	 *
	 * @param Mask	Mask to mask object flags with
	 * @return Objects flags that are set in both the object and the mask
	 */
	FORCEINLINE ELazyLoaderFlags GetMaskedFlags( ELazyLoaderFlags Mask ) const
	{
		return LazyLoaderFlags & Mask;
	}
};

/**
 * Lazy-loadable dynamic array.
 *
 * @warning: the code makes a few implicit assumptions about how TArrays are being serialized and what
 * @warning: kind of data to skip for various functions.
 *
 * GetOffset, GetPayloadSize, SerializeArrayData, operator <<, ...
 *
 * @deprecated with VER_REPLACED_LAZY_ARRAY_WITH_UNTYPED_BULK_DATA
 */
template <class T> class TLazyArrayDeprecated : public TArray<T>, public FLazyLoaderDeprecated
{
protected:
	/** Cached array count.						*/
	INT		LoadedArrayNum;
	/** Whether the array has been loaded.		*/
	UBOOL	bHasArrayBeenLoaded;

	/** Payload offset of last save				*/
	INT		SavedPayloadOffset;
	/** Payload size of last save				*/
	INT		SavedPayloadSize;
	/** ArrayNum of last save					*/
	INT		SavedArrayNum;

private:
	/**
	 * Copies data from one array into this array after detaching the lazy loader.
	 * Also resets all internal state.
	 *
	 * @param Other the source array to copy
	 */
	void Copy(const TLazyArrayDeprecated<T>& Other)
	{
		check( IsInternalStateValid() );
		check( !HasPayloadInSeparateFile() );
		if( this != &Other )
		{
			// Detach potentially attached lazy loader so we're not clobbering anything.
			Detach();

			// Use TArray assignment operator to copy array.
			(TArray<T>&)(*this) = Other;

			// Saving will fill those in again.
			PayloadOffset		= INDEX_NONE;
			PayloadSize			= INDEX_NONE;
			LoadedArrayNum		= INDEX_NONE;
			// Reset to default.
			LazyLoaderFlags		= 0;
			PayloadFilename		= NAME_None;
			bHasArrayBeenLoaded	= FALSE;
			SavedPayloadOffset	= INDEX_NONE;
			SavedPayloadSize	= INDEX_NONE;
			SavedArrayNum		= INDEX_NONE;
		}
	}

	/**
	 * Copies the source array into this one after detaching the lazy loader.
	 *
	 * @param Other the source array to copy
	 */
	TLazyArrayDeprecated& operator=( const TLazyArrayDeprecated& Other )
	{
		Copy( Other );
		return *this;
	}

	/**
	 * Copy constructor. Use the common routine to perform the copy.
	 *
	 * @param Other the source array to copy
	 */
	TLazyArrayDeprecated(const TLazyArrayDeprecated& Other) 
	: TArray<T>()
	, FLazyLoaderDeprecated()
	{
		Copy( Other );
	}

public:
	TLazyArrayDeprecated( INT InNum=0 )
	:	TArray<T>( InNum )
	,	FLazyLoaderDeprecated()
	,	LoadedArrayNum( INDEX_NONE )
	,	bHasArrayBeenLoaded( FALSE )
	,	SavedPayloadOffset( INDEX_NONE )
	,	SavedPayloadSize( INDEX_NONE )
	,	SavedArrayNum( INDEX_NONE )
	{}

	/**
	 * Returns the byte offset to the payload, skipping the array size serialized
	 * by TArray's serializer.
	 *
	 * @return offset in bytes from beginning of file to beginning of data
	 */
	INT GetPayloadOffset() 
	{ 
		check( IsInternalStateValid() );
		check( PayloadOffset != INDEX_NONE );

		// Skip over TArray's serialized ArrayNum.
		return PayloadOffset + sizeof(INT);
	}

	/**
	 * Returns the size in bytes of TArray payload, disregarding the serialized
	 * array size.
	 */
	INT GetPayloadSize()
	{
		check( IsInternalStateValid() );
		check( PayloadOffset != INDEX_NONE );
		
		// Disregard serialized ArrayNum.
		return PayloadSize - sizeof(INT);
	}

	/**
	 * Returns saved offset in bytes from beginning of file to beginning of raw data
	 *
	 * @return saved offset in bytes from beginning of file to beginning of raw data
	 */
	INT GetSavedOffset() 
	{ 
		check( IsInternalStateValid() );
		check( SavedPayloadOffset != INDEX_NONE );
		return SavedPayloadOffset;
	}

	/**
	 * Returns the saved size in bytes.
	 *
	 * @return saved size in bytes
	 */
	INT GetSavedSize()
	{
		check( IsInternalStateValid() );
		check( SavedPayloadOffset != INDEX_NONE );
		return SavedPayloadSize;
	}

	/**
	 * Returns the saved array num.
	 *
	 * @return saved array num
	 */
	INT GetSavedArrayNum()
	{
		check( IsInternalStateValid() );
		check( SavedArrayNum != INDEX_NONE );
		return SavedArrayNum;
	}

	/**
	 * Returns array size regardless of whether data has been loaded or not.
	 *
	 * @return Array size
	 */
	INT GetLoadedArrayNum()
	{
		check( IsInternalStateValid() );

		if( IsAttached() || HasPayloadInSeparateFile() )
		{
			check( LoadedArrayNum != INDEX_NONE );
			return LoadedArrayNum;
		}
		else
		{
			return this->Num();
		}
	}

	/**
	 * Sets whether separation of data from lazy array is wanted or not.
	 *
	 * @param	bShouldStoreInSeparateFile	Whether we want to store the payload when serializing this loader
	 * @param	OtherPayloadFilename		Filename used by RetrieveFilename, NAME_None means to use attached linker or linker associated with outer
	 * @param	OtherPayloadOffset			Offset in other file if we're not serializing bulk data
	 * @param	OtherPayloadSize			Size of payload if we're not serializing bulk data
	 * @param	OtherArrayNum				Loaded array num to set.
	 */
	void StoreInSeparateFile( UBOOL bShouldStoreInSeparateFile, FName OtherPayloadFilename = NAME_None, INT OtherPayloadOffset = 0, INT OtherPayloadSize = 0, INT OtherArrayNum = 0 )
	{
		if( bShouldStoreInSeparateFile )
		{
			// Make sure data is fully loaded so we can undo this operation.
			Load();
			// Detach from archive.
			Detach();
	
			// Update this loader with the payload offset and size from lazy array serialized in separate file.
			SavedPayloadOffset	= OtherPayloadOffset;
			SavedPayloadSize	= OtherPayloadSize;
			// Update array with other size, this happens e.g. if we're replacing an uncooked array with an offset to a cooked one for e.g. textures
			SavedArrayNum		= OtherArrayNum;

			// This is != NAME_None for bulk data that was fully forced into a package.
			PayloadFilename		= OtherPayloadFilename;

			// Set flag to indicate that we're separated...
			SetFlags( LLF_PayloadInSeparateFile );
		}
		else
		{
			// Saving will fill those in again.
			SavedPayloadOffset	= INDEX_NONE;
			SavedPayloadSize	= INDEX_NONE;
			SavedArrayNum		= INDEX_NONE;

			// This is != NAME_None for bulk data that was fully forced into a package.
			PayloadFilename		= OtherPayloadFilename;

			// Clear flag to indicate that we're stored in same file as archive that serializes us.
			ClearFlags( LLF_PayloadInSeparateFile );
		}
	}

	/**
	 * Serializes array data with archive, including handling of [un]compression.
	 *
	 * @param	Ar	Archive to serialize with.
	 */
	void SerializeArrayData( FArchive& Ar )
	{
		// This function should only be called if we are either saving or loading!
		check( Ar.IsLoading() || Ar.IsSaving() );

		// Serialize compressed.
		if( HasAnyFlags( LLF_SerializeCompressed ) )
		{
			// Due to the way we compress directly into memory we can only serialize bulk data this way like e.g. already
			// converted textures, compressed audio, ... Regularly serialized data is going to be compressed via the pacakge
			// export compression code.
			check( !TTypeInfo<T>::NeedsConstructor );
			check( sizeof(T)==1 );
			
			// Serialize array count.
			INT UsedArrayNum = this->Num();
			Ar << UsedArrayNum;
	
			// We're loading so the data is stored compressed and we want it uncompressed.
			if( Ar.IsLoading() )
			{
				// Create space for compressed data.
				TArray<BYTE> CompressedData;
				CompressedData.Add( PayloadSize - sizeof(INT) );
				
				// Create space for uncompressed data.
				this->Empty( UsedArrayNum );
				this->Add( UsedArrayNum );
				
				// Serialize raw compressed data into memory reader.
				Ar.Serialize( CompressedData.GetData(), CompressedData.Num() );
				
				// Create memory reader and propagated forced byte swapping option.
				FMemoryReader MemoryReader( CompressedData, TRUE );
				MemoryReader.SetByteSwapping( Ar.ForceByteSwapping() );

				// Deserialize/ uncompress from memory reader into array.
				// all old compressed data used zlib
				MemoryReader.SerializeCompressed( this->GetData(), UsedArrayNum * this->GetTypeSize(), COMPRESS_ZLIB );
			}
			// We're saving so the data is currently uncompressed and we want to save it compressed.
			else if( Ar.IsSaving() )
			{
				// Placeholder for to be compressed data.
				TArray<BYTE> CompressedData;

				// Initialize memory writer with blank compressed data array and propagate forced byte swapping
				FMemoryWriter MemoryWriter( CompressedData, TRUE );
				MemoryWriter.SetByteSwapping( Ar.ForceByteSwapping() );

				// Serialize (and compress) array contents to memory writer which in turn is using CompressedData array.
				// all old compressed data used zlib
				MemoryWriter.SerializeCompressed( this->GetData(), UsedArrayNum * this->GetTypeSize(), COMPRESS_ZLIB );

				// Serialize compressed data with passed in archive.
				Ar.Serialize( CompressedData.GetData(), CompressedData.Num() * CompressedData.GetTypeSize() );
			}
		}
		// Regular TArray serialization.
		else
		{
			Ar << (TArray<T>&)*this;
		}
	}

	/**
	 * Tells the lazy array to load it's data if it hasn't already.
	 */
	void Load()
	{
		check( IsInternalStateValid() );
		check( !HasPayloadInSeparateFile() );

		// Make sure this array is loaded.
		if( IsAttached() && !bHasArrayBeenLoaded )
		{
			LogLazyArrayPerfIssue();

			// Lazy load it now.
			check(AttachedAr);
 			INT PushedPos = AttachedAr->Tell();
			AttachedAr->Seek( PayloadOffset );
			SerializeArrayData( *AttachedAr );
			AttachedAr->Seek( PushedPos );
			bHasArrayBeenLoaded = TRUE;
		}
	}

	/**
	 * Tells the lazy array to unload its data if it is currently loaded.
	 */
	void Unload()
	{
		check( IsInternalStateValid() );
		check( !HasPayloadInSeparateFile() );

		// Make sure this array is unloaded.
		if( IsAttached() && bHasArrayBeenLoaded )
		{
			// Unload it now.
			this->Empty();
			bHasArrayBeenLoaded = FALSE;
		}
	}

	/**
	 * Serialization operator.
	 */
	friend FArchive& operator<<( FArchive& Ar, TLazyArrayDeprecated& This )
	{
		// Load any unloaded array data if we're saving unless we forcefully separated in which case
		// the data is guaranteed to have been already loaded.
		if( Ar.IsSaving() && !This.HasPayloadInSeparateFile() )
		{
			This.Load();
		}

		// Don't use lazy array serialization if we're collecting garbage and/ or are serializing
		// the transaction buffer. Do NOT change this behavior as things will subtly break due to
		// the fact that we might be loading over modified data.
		UBOOL bUseTArraySerialization = Ar.IsTransacting() || (!Ar.IsLoading() && !Ar.IsSaving());
		if( bUseTArraySerialization )
		{
			check( This.IsInternalStateValid() );
			Ar << (TArray<T>&)This;
		}
		else
		{
			// We're loading from the archive.
			if( Ar.IsLoading() )
			{	
				// Serialize end of data position as we're going to seek to it afterwards.
				INT EndPos=0;
				Ar << EndPos;

				// Serialize payload size. Will be compressed size if loader is compressed.
				if( Ar.Ver() >=	VER_LAZYARRAY_COMPRESSION )
				{
					Ar << This.PayloadSize;
				}
				else
				{
					This.PayloadSize = INDEX_NONE;
				}
	
				// Check to see whether the payload is stored in a separate file or not.
				if( Ar.Ver() >= VER_LAZYARRAY_SERIALIZATION_CHANGE )
				{
					Ar << This.LazyLoaderFlags;
				}
				else
				{
					This.ClearFlags( LLF_AllFlags );
				}

				// Load filename to get bulk data from in case it was split off.
				if( Ar.Ver() >= VER_LAZYLOADER_PAYLOADFILENAME )
				{
					Ar << This.PayloadFilename;
				}
				else
				{
					This.PayloadFilename = NAME_None;
				}

				// We have the payload stored separately.
				if( This.HasPayloadInSeparateFile() )
				{
					// This means we serialized only the offset into the file...
                    Ar << This.PayloadOffset;
					//... and the loaded array count.
					Ar << This.LoadedArrayNum;
					This.bHasArrayBeenLoaded = FALSE;
				}
				// We are not allowed to lazy load.
				else if( TRUE /*!Ar.IsAllowingLazyLoading() || (GUglyHackFlags & HACK_NoLazyArrayLazyLoading)*/ ) //@warning: code change due to pending removal of TLazyArray
				{
					// Keep track of the payload offset.
					This.PayloadOffset = Ar.Tell();
					// So we simply serialize the payload aka the TArray...
					This.SerializeArrayData( Ar );
					This.LoadedArrayNum = This.Num();
					// ... and mark it has having been loaded.
					This.bHasArrayBeenLoaded = TRUE;
				}
				// We are lazy loading.
				else
				{
					// Attach this loader with the archive.
					//Ar.AttachLazyLoader( &This ); @warning: code change due to pending removal of TLazyArray
					// Serializes first member of TArray which is ArrayNum.
					Ar << This.LoadedArrayNum;
					// Mark the array as not having been loaded yet.
					This.bHasArrayBeenLoaded = FALSE;
				}

				// Seek to the end of the serialized data, regardless of how much we actually loaded.
				Ar.Seek( EndPos );

				check( This.IsInternalStateValid() );
			}
			// We're saving to the archive.
			else if( Ar.IsSaving() )
			{
				appErrorf(TEXT("@warning: TLazyArray has been deprecated with version VER_REPLACED_LAZY_ARRAY_WITH_UNTYPED_BULK_DATA and will be removed"));
				check( This.IsInternalStateValid() );

				// Keep track of start position...
				INT StartPos = Ar.Tell();
				// ... and serialize dummy placeholder for EndPos being serialized here at the end.
				Ar << StartPos; 

				// Serialize dummy placeholder for payload size.
				INT PayloadSize = INDEX_NONE;
				Ar << PayloadSize;

				// Serialize loader flags.
				Ar << This.LazyLoaderFlags;

				// Serialize filename to get data from in case of separation or forced exports, NAME_None otherwise.
				Ar << This.PayloadFilename;

				// We forcefully separated this loaded from the bulk data.
				if( This.HasAnyFlags( LLF_PayloadInSeparateFile ) )
				{
					// So all we need to do is serialize the offset into the file.
					Ar << This.SavedPayloadOffset;
					// And the count. We cannot use This.LoadedArrayNum directly as we don't know
					// what state the loader is in so the wrapper is the safe alternative.
					INT Count = This.SavedArrayNum;
					Ar << Count;
					// Use payload size from archive.
					PayloadSize = This.SavedPayloadSize;
				}
				// We save the full array so it can be loaded from the archive.
				else
				{
					INT PayloadStartPos = Ar.Tell();
					// Serialize payload/ array.
					This.SerializeArrayData( Ar );
					INT PayloadEndPos = Ar.Tell();
					// Compute payload size.
					PayloadSize = PayloadEndPos - PayloadStartPos;
					// Update saved payload offset, size and loaded array num in lazy array if archive supported Tell()
					if( PayloadStartPos != INDEX_NONE )
					{
						This.SavedPayloadOffset	= PayloadStartPos;
						This.SavedPayloadSize	= PayloadSize;
						This.SavedArrayNum		= This.Num();
					}
				}
					
				// Keep track of current position.
				INT EndPos = Ar.Tell();
				// Seek to the beginning of the data where we saved a dummy value.
				Ar.Seek( StartPos );
				// Overwrite it with the end position
				Ar << EndPos;
				// Overwrite data with PayloadSize
				Ar << PayloadSize;
				// And seek forward to the end so subsequent writes to the archive are not clobbering data.
				Ar.Seek( EndPos );
			}
		}
		return Ar;
	}
};

/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/
