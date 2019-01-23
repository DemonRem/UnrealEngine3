/*=============================================================================
	Editor.h: Unreal editor public header file.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef _INC_EDITOR_COMMANDLETS
#define _INC_EDITOR_COMMANDLETS

BEGIN_COMMANDLET(AnalyzeScript,Editor)
	static UFunction* FindSuperFunction(UFunction* evilFunc);
END_COMMANDLET

BEGIN_COMMANDLET(AnalyzeContent,Editor)
	void StaticInitialize();
END_COMMANDLET


/**
 * Contains stats about a single resource in a package file.
 */
struct FObjectResourceStat
{
	/** the complete path name for this resource */
	INT ResourceNameIndex;

	/** the name of the class for this resource */
	FName ClassName;

	/** the size of this resource, on disk */
	INT ResourceSize;

	/** Standard Constructor */
	FObjectResourceStat( FName InClassName, const FString& InResourceName, INT InResourceSize );

	/** Copy constructor */
	FObjectResourceStat( const FObjectResourceStat& Other )
	{
		ResourceNameIndex = Other.ResourceNameIndex;
		ClassName = Other.ClassName;
		ResourceSize = Other.ResourceSize;
	}
};

/**
 * A mapping of class name to the resource stats for objects of that class
 */
class FClassResourceMap : public TMultiMap<FName,FObjectResourceStat>
{
};

struct FPackageResourceStat
{
	/** the name of the package this struct contains resource stats for */
	FName				PackageName;

	/** the filename of the package; will be different from PackageName if this package is one of the loc packages */
	FName				PackageFilename;

	/** the map of 'Class name' to 'object resources of that class' for this package */
	FClassResourceMap	PackageResources;

	/**
	 * Constructor
	 */
	FPackageResourceStat( FName InPackageName )
	: PackageName(InPackageName)
	{ }

	/**
	 * Creates a new resource stat using the specified parameters.
	 *
	 * @param	ResourceClassName	the name of the class for the resource
	 * @param	ResourcePathName	the complete path name for the resource
	 * @param	ResourceSize		the size on disk for the resource
	 *
	 * @return	a pointer to the FObjectResourceStat that was added
	 */
	struct FObjectResourceStat* AddResourceStat( FName ResourceClassName, const FString& ResourcePathName, INT ResourceSize );
};



enum EReportOutputType
{
	/** write the results to the log only */
	OUTPUTTYPE_Log,

	/** write the results to a CSV file */
	OUTPUTTYPE_CSV,

	/** write the results to an XML file (not implemented) */
	OUTPUTTYPE_XML,
};

/**
 * Generates various types of reports for the list of resources collected by the AnalyzeCookedContent commandlet.  Each derived version of this struct
 * generates a different type of report.
 */
struct FResourceStatReporter
{
	EReportOutputType OutputType;

	/**
	 * Creates a report using the specified stats.  The type of report created depends on the reporter type.
	 *
	 * @param	ResourceStats	the list of resource stats to create a report for.
	 *
	 * @return	TRUE if the report was created successfully; FALSE otherwise.
	 */
	virtual UBOOL CreateReport( const TArray<struct FPackageResourceStat>& ResourceStats )=0;

	/** Constructor */
	FResourceStatReporter()
	: OutputType(OUTPUTTYPE_Log)
	{}

	/** Destructor */
	virtual ~FResourceStatReporter()
	{}
};

/**
 * This reporter generates a report on the disk-space taken by each asset type.
 */
struct FResourceStatReporter_TotalMemoryPerAsset : public FResourceStatReporter
{
	/**
	 * Creates a report using the specified stats.  The type of report created depends on the reporter type.
	 *
	 * @param	ResourceStats	the list of resource stats to create a report for.
	 *
	 * @return	TRUE if the report was created successfully; FALSE otherwise.
	 */
	virtual UBOOL CreateReport( const TArray<struct FPackageResourceStat>& ResourceStats );
};

/**
 * This reporter generates a report which displays objects which are duplicated into more than one package.
 */
struct FResourceStatReporter_AssetDuplication : public FResourceStatReporter
{
	/**
	 * Creates a report using the specified stats.  The type of report created depends on the reporter type.
	 *
	 * @param	ResourceStats	the list of resource stats to create a report for.
	 *
	 * @return	TRUE if the report was created successfully; FALSE otherwise.
	 */
	virtual UBOOL CreateReport( const TArray<struct FPackageResourceStat>& ResourceStats );
};

struct FResourceDiskSize
{
	FString ClassName;
	QWORD TotalSize;

	/** Default constructor */
	FResourceDiskSize( FName InClassName )
	: ClassName(InClassName.ToString()), TotalSize(0)
	{}

	/** Copy constructor */
	FResourceDiskSize( const FResourceDiskSize& Other )
	{
		ClassName = Other.ClassName;
		TotalSize = Other.TotalSize;
	}
};

BEGIN_COMMANDLET(AnalyzeCookedContent,Editor)

	/**
	 * the list of packages to process
	 */
	TArray<FFilename> CookedPackageNames;

	/**
	 * the class, path name, and size on disk for all resources
	 */
	TArray<struct FPackageResourceStat> PackageResourceStats;

	/**
	 * Builds the list of package names to load
	 */
	void Init();

	/**
	 * Loads each package and adds stats for its exports to the main list of stats
	 */
	void AssembleResourceStats();

	/**
	 * Determines which report type is desired based on the command-line parameters specified and creates the appropriate reporter.
	 *
	 * @param	Params	the command-line parameters passed to Main
	 *
	 * @return	a pointer to a reporter which generates output in the desired format, or NULL if no valid report type was specified.
	 */
	struct FResourceStatReporter* CreateReporter( const FString& Params );

END_COMMANDLET

BEGIN_COMMANDLET(AnalyzeCookedPackages,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(AnalyzeReferencedContent,Editor)
	//@todo: this code is in dire need of refactoring

	/**
	 * Encapsulates gathered stats for a particular UStaticMesh object.
	 */
	struct FStaticMeshStats
	{
		/** Constructor, initializing all members. */
		FStaticMeshStats( UStaticMesh* StaticMesh );

		/**
		 * Stringifies gathered stats in CSV format.
		 *
		 * @return comma separated list of stats
		 */
		FString ToCSV() const;

		/**
		 * Returns a header row for CSV
		 *
		 * @return comma separated header row
		 */
		static FString GetCSVHeaderRow();

		/** Resource type.																*/
		FString ResourceType;
		/** Resource name.																*/
		FString ResourceName;
		/** Number of static mesh instances overall.									*/
		INT	NumInstances;
		/** Triangle count of mesh.														*/
		INT	NumTriangles;
		/** Section count of mesh.														*/
		INT NumSections;
		/** Number of convex hulls in the collision geometry of mesh.					*/
		INT NumPrimitives;
		/** Whether resource is referenced by script.									*/
		UBOOL bIsReferencedByScript;
		/** Number of maps this static mesh was used in.								*/
		INT	NumMapsUsedIn;
		/** Resource size of static mesh.												*/
		INT	ResourceSize;
		/** Array of different scales that this mesh is used at							*/
		TArray<FVector> UsedAtScales;
	};

	/**
	 * Encapsulates gathered stats for a particular UTexture object.
	 */
	struct FTextureStats
	{
		/** Constructor, initializing all members */
		FTextureStats( UTexture* Texture );

		/**
		 * Stringifies gathered stats in CSV format.
		 *
		 * @return comma separated list of stats
		 */
		FString ToCSV() const;

		/**
		 * Returns a header row for CSV
		 *
		 * @return comma separated header row
		 */
		static FString GetCSVHeaderRow();

		/** Resource type.																*/
		FString ResourceType;
		/** Resource name.																*/
		FString ResourceName;
		/** Map of materials this textures is being used by.							*/
		TMap<FString,INT> MaterialsUsedBy;
		/** Whether resource is referenced by script.									*/
		UBOOL bIsReferencedByScript;
		/** Number of maps this texture is being used in.								*/
		INT	NumMapsUsedIn;
		/** Resource size of texture.													*/
		INT	ResourceSize;
		/** LOD bias.																	*/
		INT	LODBias;
		/** LOD group.																	*/
		INT LODGroup;
		/** Texture pixel format.														*/
		FString Format;
	};

	/**
	 * Encapsulates gathered stats for a particular UMaterial object.
	 */
	struct FMaterialStats
	{
		/** Constructor, initializing all members */
		FMaterialStats( UMaterial* Material, UAnalyzeReferencedContentCommandlet* Commandlet );

		/**
		 * Stringifies gathered stats in CSV format.
		 *
		 * @return comma separated list of stats
		 */
		FString ToCSV() const;

		/**
		 * Returns a header row for CSV
		 *
		 * @return comma separated header row
		 */
		static FString GetCSVHeaderRow();

		/** Resource type.																*/
		FString ResourceType;
		/** Resource name.																*/
		FString ResourceName;
		/** Number of BSP surfaces this material is applied to.							*/
		INT	NumBrushesAppliedTo;
		/** Number of static mesh instances this material is applied to.				*/
		INT NumStaticMeshInstancesAppliedTo;
		/** Map of static meshes this material is used by.								*/
		TMap<FString,INT> StaticMeshesAppliedTo;
		/** Whether resource is referenced by script.									*/
		UBOOL bIsReferencedByScript;
		/** Number of maps this material is used in.									*/
		INT NumMapsUsedIn;
		/** Array of textures used. Also implies count.									*/
		TArray<FString>	TexturesUsed;
		/** Number of texture samples made to render this material.						*/
		INT NumTextureSamples;
		/** Max depth of depedent texture read chain.									*/
		INT MaxTextureDependencyLength;
		/** Translucent instruction count.												*/
		INT NumInstructionsTranslucent;
		/** Additive instruction count.													*/
		INT NumInstructionsAdditive;
		/** Modulte instruction count.													*/
		INT NumInstructionsModulate;
		/** Base pass no lightmap instruction count.										*/
		INT NumInstructionsBasePassNoLightmap;
		/** Base pass with vertex lightmap instruction count.							*/
		INT NumInstructionsBasePassAndLightmap;
		/** Point light with shadow map instruction count.								*/
		INT NumInstructionsPointLightWithShadowMap;
		/** Resource size of all referenced/ used textures.								*/
		INT ResourceSizeOfReferencedTextures;
	};

	/**
	 * Encapsulates gathered stats for a particular UParticleSystem object
	 */
	struct FParticleStats
	{
		/** Constructor, initializing all members */
		FParticleStats( UParticleSystem* ParticleSystem );

		/**
		 * Stringifies gathered stats in CSV format.
		 *
		 * @return comma separated list of stats
		 */
		FString ToCSV() const;

		/**
		 * Returns a header row for CSV
		 *
		 * @return comma separated header row
		 */
		static FString GetCSVHeaderRow();

		/** Resource type.																*/
		FString ResourceType;
		/** Resource name.																*/
		FString ResourceName;
		/** Whether resource is referenced by script.									*/
		UBOOL bIsReferencedByScript;	
		/** Number of maps this particle system is used in.								*/
		INT NumMapsUsedIn;
		/** Number of emitters in this system.											*/
		INT NumEmitters;
		/** Combined number of modules in all emitters used.							*/
		INT NumModules;
		/** Combined number of peak particles in system.								*/
		INT NumPeakActiveParticles;
	};

	struct FAnimSequenceStats
	{
		/** Constructor, initializing all members */
		FAnimSequenceStats( UAnimSequence* Sequence );

		/**
		 * Stringifies gathered stats in CSV format.
		 *
		 * @return comma separated list of stats
		 */
		FString ToCSV() const;

		/**
		 * Returns a header row for CSV
		 *
		 * @return comma separated header row
		 */
		static FString GetCSVHeaderRow();

		/** Resource type.																*/
		FString ResourceType;
		/** Resource name.																*/
		FString ResourceName;
		/** Whether resource is referenced by script.									*/
		UBOOL bIsReferencedByScript;	
		/** Number of maps this sequence is referenced by.								*/
		INT NumMapsUsedIn;
		/** Type of compression used on this animation.									*/
		enum AnimationCompressionFormat TranslationFormat;
		/** Type of compression used on this animation.									*/
		enum AnimationCompressionFormat RotationFormat;
		/** Name of compression algo class used. */
		FString CompressionScheme;
		/** Size in bytes of this animation. */
		INT	AnimationSize;
	};

	/**
	 * Retrieves/ creates material stats associated with passed in material.
	 *
	 * @warning: returns pointer into TMap, only valid till next time Set is called
	 *
	 * @param	Material	Material to retrieve/ create material stats for
	 * @return	pointer to material stats associated with material
	 */
	FMaterialStats* GetMaterialStats( UMaterial* Material );

	/**
	 * Retrieves/ creates texture stats associated with passed in texture.
	 *
	 * @warning: returns pointer into TMap, only valid till next time Set is called
	 *
	 * @param	Texture		Texture to retrieve/ create texture stats for
	 * @return	pointer to texture stats associated with texture
	 */
	FTextureStats* GetTextureStats( UTexture* Texture );
	
	/**
	 * Retrieves/ creates static mesh stats associated with passed in static mesh.
	 *
	 * @warning: returns pointer into TMap, only valid till next time Set is called
	 *
	 * @param	StaticMesh	Static mesh to retrieve/ create static mesh stats for
	 * @return	pointer to static mesh stats associated with static mesh
	 */
	FStaticMeshStats* GetStaticMeshStats( UStaticMesh* StaticMesh );

	/**
	 * Retrieves/ creates particle stats associated with passed in particle system.
	 *
	 * @warning: returns pointer into TMap, only valid till next time Set is called
	 *
	 * @param	ParticleSystem	Particle system to retrieve/ create static mesh stats for
	 * @return	pointer to particle system stats associated with static mesh
	 */
	FParticleStats* GetParticleStats( UParticleSystem* ParticleSystem );

	/**
	 * Retrieves/ creates animation sequence stats associated with passed in animation sequence.
	 *
	 * @warning: returns pointer into TMap, only valid till next time Set is called
	 *
	 * @param	AnimSequence	Anim sequence to retrieve/ create anim sequence stats for
	 * @return	pointer to particle system stats associated with anim sequence
	 */
	FAnimSequenceStats* GetAnimSequenceStats( UAnimSequence* AnimSequence );

	void StaticInitialize();

	/**
	 * Handles encountered object, routing to various sub handlers.
	 *
	 * @param	Object			Object to handle
	 * @param	LevelPackage	Currently loaded level package, can be NULL if not a level
	 * @param	bIsScriptReferenced Whether object is handled because there is a script reference
	 */
	void HandleObject( UObject* Object, UPackage* LevelPackage, UBOOL bIsScriptReferenced );
	
	/**
	 * Handles gathering stats for passed in static mesh.
	 *
	 * @param StaticMesh	StaticMesh to gather stats for.
	 * @param LevelPackage	Currently loaded level package, can be NULL if not a level
	 * @param bIsScriptReferenced Whether object is handled because there is a script reference
	 */
	void HandleStaticMesh( UStaticMesh* StaticMesh, UPackage* LevelPackage, UBOOL bIsScriptReferenced );
	/**
	 * Handles gathering stats for passed in static mesh component.
	 *
	 * @param StaticMeshComponent	StaticMeshComponent to gather stats for
	 * @param LevelPackage	Currently loaded level package, can be NULL if not a level
	 * @param bIsScriptReferenced Whether object is handled because there is a script reference
	 */
	void HandleStaticMeshComponent( UStaticMeshComponent* StaticMeshComponent, UPackage* LevelPackage, UBOOL bIsScriptReferenced );
	/**
	* Handles special case for stats for passed in static mesh component who is part of a ParticleSystemComponent
	*
	* @param ParticleSystemComponent	ParticleSystemComponent to gather stats for
	* @param LevelPackage	Currently loaded level package, can be NULL if not a level
	* @param bIsScriptReferenced Whether object is handled because there is a script reference
	*/
	void HandleStaticMeshOnAParticleSystemComponent( UParticleSystemComponent* ParticleSystemComponent, UPackage* LevelPackage, UBOOL bIsScriptReferenced );

	/**
	 * Handles gathering stats for passed in material.
	 *
	 * @param Material		Material to gather stats for
 	 * @param LevelPackage	Currently loaded level package, can be NULL if not a level
	 * @param bIsScriptReferenced Whether object is handled because there is a script reference
	 */
	void HandleMaterial( UMaterial* Material, UPackage* LevelPackage, UBOOL bIsScriptReferenced );
	/**
	 * Handles gathering stats for passed in texture.
	 *
	 * @param Texture		Texture to gather stats for
	 * @param LevelPackage	Currently loaded level package, can be NULL if not a level
	 * @param bIsScriptReferenced Whether object is handled because there is a script reference
	 */
	void HandleTexture( UTexture* Texture, UPackage* LevelPackage, UBOOL bIsScriptReferenced );
	/**
	 * Handles gathering stats for passed in brush.
	 *
	 * @param Brush			Brush to gather stats for
	 * @param LevelPackage	Currently loaded level package, can be NULL if not a level
	 * @param bIsScriptReferenced Whether object is handled because there is a script reference
	 */
	void HandleBrush( ABrush* Brush, UPackage* LevelPackage, UBOOL bIsScriptReferenced );
	/**
	 * Handles gathering stats for passed in particle system.
	 *
	 * @param ParticleSystem	Particle system to gather stats for
	 * @param LevelPackage		Currently loaded level package, can be NULL if not a level
	 * @param bIsScriptReferenced Whether object is handled because there is a script reference
	 */
	void HandleParticleSystem( UParticleSystem* ParticleSystem, UPackage* LevelPackage, UBOOL bIsScriptReferenced );
	/**
	 * Handles gathering stats for passed in animation sequence.
	 *
	 * @param AnimSequence		AnimSequence to gather stats for
	 * @param LevelPackage		Currently loaded level package, can be NULL if not a level
	 * @param bIsScriptReferenced Whether object is handled because there is a script reference
	 */
	void HandleAnimSequence( UAnimSequence* AnimSequence, UPackage* LevelPackage, UBOOL bIsScriptReferenced );
	/**
	 * Handles gathering stats for passed in level.
	 *
	 * @param Level				Level to gather stats for
	 * @param LevelPackage		Currently loaded level package, can be NULL if not a level
	 * @param bIsScriptReferenced Whether object is handled because there is a script reference
	 */
	void HandleLevel( ULevel* Level, UPackage* LevelPackage, UBOOL bIsScriptReferenced );


	/** Mapping from a fully qualified resource string (including type) to static mesh stats info.								*/
	TMap<FString,FStaticMeshStats> ResourceNameToStaticMeshStats;
	/** Mapping from a fully qualified resource string (including type) to texture stats info.									*/
	TMap<FString,FTextureStats> ResourceNameToTextureStats;
	/** Mapping from a fully qualified resource string (including type) to material stats info.									*/
	TMap<FString,FMaterialStats> ResourceNameToMaterialStats;
	/** Mapping from a fully qualified resource string (including type) to particle stats info.									*/
	TMap<FString,FParticleStats> ResourceNameToParticleStats;
	/** Mapping from a fully qualified resource string (including type) to anim stats info.									*/
	TMap<FString,FAnimSequenceStats> ResourceNameToAnimStats;

	// Various shader types for logging

	/** Shader type for base pass pixel shader (no lightmap).  */
	FShaderType*	ShaderTypeBasePassNoLightmap;
	/** Shader type for base pass pixel shader (including lightmap). */
	FShaderType*	ShaderTypeBasePassAndLightmap;
	/** Shader type for point light with shadow map pixel shader. */
	FShaderType*	ShaderTypePointLightWithShadowMap;
END_COMMANDLET


BEGIN_COMMANDLET(AnalyzeFallbackMaterials,Editor)
/**
* Encapsulates gathered stats for a particular UMaterial object.
*/
struct FMaterialStats
{
	/** Constructor, initializing all members */
	FMaterialStats( UMaterial* Material, UAnalyzeFallbackMaterialsCommandlet* Commandlet );

	/**
	* Stringifies gathered stats in CSV format.
	*
	* @return comma separated list of stats
	*/
	FString ToCSV() const;

	/**
	* Returns a header row for CSV
	*
	* @return comma separated header row
	*/
	static FString GetCSVHeaderRow();

	/** Resource type.																*/
	FString ResourceType;
	/** Resource name.																*/
	FString ResourceName;
	/** Whether resource is referenced by script.									*/
	UBOOL bIsReferencedByScript;
	/** The components that were dropped when compiling this material for sm2.		*/
	DWORD DroppedFallbackComponents;
};

/**
* Retrieves/ creates material stats associated with passed in material.
*
* @warning: returns pointer into TMap, only valid till next time Set is called
*
* @param	Material	Material to retrieve/ create material stats for
* @return	pointer to material stats associated with material
*/
FMaterialStats* GetMaterialStats( UMaterial* Material );

/**
* Handles encountered object, routing to various sub handlers.
*
* @param	Object			Object to handle
* @param	LevelPackage	Currently loaded level package, can be NULL if not a level
* @param	bIsScriptReferenced Whether object is handled because there is a script reference
*/
void HandleObject( UObject* Object, UPackage* LevelPackage, UBOOL bIsScriptReferenced );

/**
* Handles gathering stats for passed in material.
*
* @param Material		Material to gather stats for
* @param LevelPackage	Currently loaded level package, can be NULL if not a level
* @param bIsScriptReferenced Whether object is handled because there is a script reference
*/
void HandleMaterial( UMaterial* Material, UPackage* LevelPackage, UBOOL bIsScriptReferenced );

/** Mapping from a fully qualified resource string (including type) to material stats info.									*/
TMap<FString,FMaterialStats> ResourceNameToMaterialStats;
END_COMMANDLET


BEGIN_COMMANDLET(BatchExport,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(ExportLoc,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(CompareLoc,Editor)
	
	/**
	 * Contains information about a single localization file, any language.
	 */
	struct FLocalizationFile
	{
	private:
		/**
		 * The filename for the FConfigFile this FLocalizationFile represents.
		 */
		FFilename LocFilename;

		/** sections that do not exist in the counterpart file. */
		TArray<FString> UnmatchedSections;

		/** properties that are missing from the corresponding section in the other file */
		TArray<FString> UnmatchedProperties;

		/** properties that have identical values in the other file */
		TArray<FString> IdenticalProperties;

		/** the FConfigFile which contains the data for this loc file */
		FConfigFile* LocFile;

	public:

		/**
		 * Standard constructor
		 */
		FLocalizationFile( const FString& InPath );

		/** Copy ctor */
		FLocalizationFile( const FLocalizationFile& Other );

		/** Dtor */
		~FLocalizationFile();

		/**
		 * Determines whether this file is the counterpart for the loc file specified
		 */
		UBOOL IsCounterpartFor( const FLocalizationFile& Other ) const;

		/**
		 * Compares the data in this loc file against the data in the specified counterpart file, placing the results in the various tracking arrays.
		 */
		void CompareToCounterpart( FLocalizationFile* Other );

		/** Accessors */
		const FString GetFullName()			const	{ return LocFilename; }
		const FString GetDirectoryName()	const	{ return LocFilename.GetPath(); }
		const FString GetFilename()			const	{ return LocFilename.GetBaseFilename(); }
		const FString GetExtension()		const	{ return LocFilename.GetExtension(); }
		class FConfigFile* GetFile()		const	{ return LocFile; }

		void GetMissingSections( TArray<FString>& out_Sections ) const;
		void GetMissingProperties( TArray<FString>& out_Properties ) const;
		void GetIdenticalProperties( TArray<FString>& out_Properties ) const;
	};

	/**
	 * Contains information about a localization file and its english counterpart.
	 */
	struct FLocalizationFilePair
	{
		FLocalizationFile* EnglishFile, *ForeignFile;

		/** Default ctor */
		FLocalizationFilePair() : EnglishFile(NULL), ForeignFile(NULL) {}
		~FLocalizationFilePair();

		/**
		 * Compares the two loc files against each other.
		 */
		void CompareFiles();

		/**
		 * Builds a list of files which exist in the english directory but don't have a counterpart in the foreign directory.
		 */
		void GetMissingLocFiles( TArray<FString>& Files );

		/**
		 * Builds a list of files which no longer exist in the english loc directories.
		 */
		void GetObsoleteLocFiles( TArray<FString>& Files );

		/**
		 * Builds a list of section names which exist in the english version of the file but don't exist in the foreign version.
		 */
		void GetMissingSections( TArray<FString>& Sections );

		/**
		 * Builds a list of section names which exist in the foreign version but no longer exist in the english version.
		 */
		void GetObsoleteSections( TArray<FString>& Sections );

		/**
		 * Builds a list of key names which exist in the english version of the file but don't exist in the foreign version.
		 */
		void GetMissingProperties( TArray<FString>& Properties );

		/**
		 * Builds a list of section names which exist in the foreign version but no longer exist in the english version.
		 */
		void GetObsoleteProperties( TArray<FString>& Properties );

		/**
		 * Builds a list of property names which have the same value in the english and localized version of the file, indicating that the value isn't translated.
		 */
		void GetUntranslatedProperties( TArray<FString>& Properties );

		/**
		 * Assigns the english version of the loc file pair.
		 */
		UBOOL SetEnglishFile( const FString& EnglishFilename );

		/**
		 * Assigns the foreign version of this loc file pair.
		 */
		UBOOL SetForeignFile( const FString& ForeignFilename );

		/** returns the filename (without path or extension info) for this file pair */
		const FString GetFilename();
		UBOOL HasEnglishFile();
		UBOOL HasForeignFile();
		UBOOL HasEnglishFile( const FString& Filename );
		UBOOL HasForeignFile( const FString& Filename );
	};

	/**
	 * Returns the index of the loc file pair that contains the english version of the specified filename, or INDEX_NONE if it isn't found
	 */
	INT FindEnglishIndex( const FString& Filename );

	/**
	 * Returns the index of the loc file pair that contains the english version of the specified filename, or INDEX_NONE if it isn't found
	 */
	INT FindForeignIndex( const FString& Filename );

	/**
	 * Adds the specified file as the english version for a loc file pair
	 */
	void AddEnglishFile( const FString& Filename );

	/**
	 * Adds the specified file as the foreign version for a loc file pair
	 */
	void AddForeignFile( const FString& Filename );

	/**
	 * Initializes the LocPairs arrays using the list of filenames provided.
	 */
	void ReadLocFiles( const TArray<FString>& EnglishFilenames, TArray<FString>& ForeignFilenames );

	FString LangExt;
	TArray<FLocalizationFilePair> LocPairs;

END_COMMANDLET

BEGIN_COMMANDLET(Conform,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(LoadPackage,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(ConvertEmitters,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(DumpEmitters,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(ConvertUberEmitters,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(FixupEmitters,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(FindEmitterMismatchedLODs,Editor)
	void CheckPackageForMismatchedLODs( const FFilename& Filename );
END_COMMANDLET

BEGIN_COMMANDLET(FindEmitterModifiedLODs,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(FixupRedirects,Editor)
	/**
	 * Allows commandlets to override the default behavior and create a custom engine class for the commandlet. If
	 * the commandlet implements this function, it should fully initialize the UEngine object as well.  Commandlets
	 * should indicate that they have implemented this function by assigning the custom UEngine to GEngine.
	 */
	virtual void CreateCustomEngine();
END_COMMANDLET

BEGIN_COMMANDLET(FixupSourceUVs,Editor)
	virtual void FixupUVs( INT step, FStaticMeshTriangle * RawTriangleData, INT NumRawTriangles );
	virtual UBOOL FindUV( const FStaticMeshTriangle * RawTriangleData, INT UVChannel, INT NumRawTriangles, FVector2D &UV );
	virtual UBOOL ValidateUVChannels( const FStaticMeshTriangle * RawTriangleData, INT NumRawTriangles );
	virtual UBOOL CheckFixableType( INT step, const FStaticMeshTriangle * RawTriangleData, INT NumRawTriangles );
	virtual INT CheckFixable( const FStaticMeshTriangle * RawTriangleData, INT NumRawTriangles );
	virtual UBOOL CheckUVs( FStaticMeshRenderData * StaticMeshRenderData, const FStaticMeshTriangle * RawTriangleData );
END_COMMANDLET

BEGIN_COMMANDLET(CreateStreamingWorld,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(ListPackagesReferencing,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(PkgInfo,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(ResavePackages,Editor)
	/**
	 * Allows the commandlet to perform any additional operations on the object before it is resaved.
	 *
	 * @param	Object			the object in the current package that is currently being processed
	 * @param	bSavePackage	[in]	indicates whether the package is currently going to be saved
	 *							[out]	set to TRUE to resave the package
	 */
	void PerformAdditionalOperations( class UObject* Object, UBOOL& bSavePackage );

	/**
	 * Allows the commandlet to perform any additional operations on the package before it is resaved.
	 *
	 * @param	Package			the package that is currently being processed
	 * @param	bSavePackage	[in]	indicates whether the package is currently going to be saved
	 *							[out]	set to TRUE to resave the package
	 */
	void PerformAdditionalOperations( class UPackage* Package, UBOOL& bSavePackage );

	/**
	 * Removes any UClass exports from packages which aren't script packages.
	 *
	 * @param	Package			the package that is currently being processed
	 *
	 * @return	TRUE to resave the package
	 */
	UBOOL CleanClassesFromContentPackages( class UPackage* Package );

	/**
	 * Instances subobjects for any existing objects with subobject properties pointing to the default object.
	 * This is currently the case when a classes has an object property and subobject definition added to it --
	 * existing instances of such a class will see the new object property refer to the template object.
	 *
	 * @param	Package			The package that is currently being processed.
	 *
	 * @return					TRUE to resave the package.
	 */
	UBOOL InstanceMissingSubObjects(class UPackage* Package);

END_COMMANDLET

BEGIN_COMMANDLET(CutDownContent,Editor)
	/**
	 * Allows commandlets to override the default behavior and create a custom engine class for the commandlet. If
	 * the commandlet implements this function, it should fully initialize the UEngine object as well. Commandlets
	 * should indicate that they have implemented this function by assigning the custom UEngine to GEngine.
	 */
	virtual void CreateCustomEngine();
END_COMMANDLET

BEGIN_COMMANDLET(ScaleAudioVolume,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(CookPackages,Editor)
/** DLL handle to platform-specific xxxTools.dll												*/
	void*							PlatformToolsDllHandle;

	/**
	 * Entry points into xxxTools.dll used for cooking.
	 */
	FuncCreateTextureCooker			CreateTextureCooker;
	FuncDestroyTextureCooker		DestroyTextureCooker;
	FuncCreateSoundCooker			CreateSoundCooker;
	FuncDestroySoundCooker			DestroySoundCooker;
	FuncCreateSkeletalMeshCooker	CreateSkeletalMeshCooker;
	FuncDestroySkeletalMeshCooker	DestroySkeletalMeshCooker;
	FuncCreateStaticMeshCooker		CreateStaticMeshCooker;
	FuncDestroyStaticMeshCooker		DestroyStaticMeshCooker;
	FuncCreateShaderPrecompiler		CreateShaderPrecompiler;
	FuncDestroyShaderPrecompiler	DestroyShaderPrecompiler;
	
	/** What platform are we cooking for?														*/
	UE3::EPlatformType				Platform;
	/** The shader type with which to compile shaders											*/
	EShaderPlatform					ShaderPlatform;
	
	/**
	 * Cooking helper classes for resources requiring platform specific cooking
	 */
	FConsoleTextureCooker*			TextureCooker;
	FConsoleSoundCooker*			SoundCooker;
	FConsoleSkeletalMeshCooker*		SkeletalMeshCooker;
	FConsoleStaticMeshCooker*		StaticMeshCooker;

	/** Cooked data directory																	*/
	FString							CookedDir;
	/** Whether to only cook dependencies of passed in packages/ maps							*/
	UBOOL							bOnlyCookDependencies;
	/** Whether to all non-map packages, even it not referenced									*/
	UBOOL							bCookAllNonMapPackages;
	/** Whether to skip cooking maps.															*/
	UBOOL							bSkipCookingMaps;
	/** Whether to skip saving maps.															*/
	UBOOL							bSkipSavingMaps;
	/**	Whether to skip loading and saving maps not necessarily required like texture packages.	*/
	UBOOL							bSkipNotRequiredPackages;
	/** Always recook maps, even if they haven't changed since the last cooking.				*/
	UBOOL							bAlwaysRecookMaps;
	/** Always recook script files, even if they haven't changed since the last cooking.		*/
	UBOOL							bAlwaysRecookScript;
	/** Only cook ini and localization files.													*/
	UBOOL							bIniFilesOnly;
	/** Generate SHA hashes.																	*/
	UBOOL							bGenerateSHAHashes;
	/** Should the cooker preassemble ini files and copy those to the Xbox						*/
	UBOOL							bShouldPreFinalizeIniFilesInCooker;
	/** TRUE if shared MP resources should be cooked separately rather than duplicated into seek-free MP level packages. */
	UBOOL							bSeparateSharedMPResources;
	/** TRUE to cook out static mesh actors														*/
	UBOOL							bCookOutStaticMeshActors;
	/** TRUE to cook out static light actors													*/
	UBOOL							bCookOutStaticLightActors;
	/** Set containing all packages that need to be cooked if bOnlyCookDependencies	== TRUE.	*/
	TMap<FString,INT>				PackageDependencies;
	/** Regular packages required to be cooked/ present.										*/
	TArray<FString>					RequiredPackages;
	/** Container used to store/ look up information for separating bulk data from archive.		*/
	UCookedBulkDataInfoContainer*	CookedBulkDataInfoContainer;
	/** LOD settings used on target platform.													*/
	FTextureLODSettings				PlatformLODSettings;
	/** Shader cache saved into seekfree packages.												*/
	UShaderCache*					ShaderCache;
	/** Map of materials that have already been put into an always loaded shader cache.			*/
	TMap<UMaterial*,UBOOL>			AlreadyHandledMaterials;
	/** Map of material instances that have already been put into an always loaded shader cache. */
	TMap<class UMaterialInstance*,UBOOL>			AlreadyHandledMaterialInstances;
	/** A list of files to generate SHA hash values for											*/
	TArray<FString>					FilesForSHA;

	/** Remember the name of the target platform's engine .ini file								*/
	TCHAR							PlatformEngineConfigFilename[1024];

	/**
	 * Helper structure encapsulating mapping from src file to cooked data filename.
	 */
	struct FPackageCookerInfo
	{
		/** Src filename.									*/
		FFilename SrcFilename;
		/** Cooked dst filename.							*/
		FFilename DstFilename;
		/** Whether this package should be made seek free.	*/
		UBOOL bShouldBeSeekFree;
		/** Whether this package is a native script file.	*/
		UBOOL bIsNativeScriptFile;
		/** Whether this packages is a startup package.		*/
		UBOOL bIsCombinedStartupPackage;
		/** Whether this packages is standalone seekfree.	*/
		UBOOL bIsStandaloneSeekFreePackage;

		/** Constructor, initializing member variables with passed in ones */
		FPackageCookerInfo( const TCHAR* InSrcFilename, const TCHAR* InDstFilename, UBOOL InbShouldBeSeekFree, UBOOL InbIsNativeScriptFile, UBOOL InbIsCombinedStartupPackage, UBOOL InbIsStandaloneSeekFreePackage )
		:	SrcFilename( InSrcFilename )
		,	DstFilename( InDstFilename )
		,	bShouldBeSeekFree( InbShouldBeSeekFree )
		,	bIsNativeScriptFile( InbIsNativeScriptFile )
		,	bIsCombinedStartupPackage( InbIsCombinedStartupPackage )
		,	bIsStandaloneSeekFreePackage( InbIsStandaloneSeekFreePackage )
		{}
	};

	/**
	 * Cooks passed in object if it hasn't been already.
	 *
	 * @param	 Object		Object to cook
	 */
	void CookObject( UObject* Object );
	/**
	 * Helper function used by CookObject - performs texture specific cooking.
	 *
	 * @param	Texture2D	Texture to cook
	 */
	void CookTexture( UTexture2D* Texture2D );
	/**
	* Helper function used by CookObject - performs movie specific cooking.
	*
	* @param	TextureMovie	Movie texture to cook
	*/
	void CookMovieTexture( UTextureMovie* TextureMovie );
	/**
	 * Helper function used by CookObject - performs ParticleSystem specific cooking.
	 *
	 * @param	ParticleSystem	ParticleSystem to cook
	 */
	void CookParticleSystem(UParticleSystem* ParticleSystem);

	/**
	 * Helper function used by CookObject - performs SkeletalMesh specific cooking.
	 *
	 * @param	SkeletalMesh	SkeletalMesh to cook
	 */
	void CookSkeletalMesh(USkeletalMesh* SkeletalMesh);

	/**
	 * Helper function used by CookObject - performs StaticMesh specific cooking.
	 *
	 * @param	StaticMesh	StaticMesh to cook
	 */
	void CookStaticMesh(UStaticMesh* StaticMesh);

	/**
	 * Cooks out all static mesh actors in the specified package by re-attaching their StaticMeshComponents to
	 * a StaticMeshCollectionActor referenced by the world.
	 *
	 * @param	Package		the package being cooked
	 */
	void CookStaticMeshActors( UPackage* Package );

	/**
	 * Cooks out all static Light actors in the specified package by re-attaching their LightComponents to a 
	 * StaticLightCollectionActor referenced by the world.
	 */
	void CookStaticLightActors( UPackage* Package );

	/**
	 * Prepares object for saving into package. Called once for each object being saved 
	 * into a new package.
	 *
	 * @param	Package						Package going to be saved
	 * @param	Object						Object to prepare
	 * @param	bIsSavedInSeekFreePackage	Whether object is going to be saved into a seekfree package
	 */
	void PrepareForSaving( UPackage* Package, UObject* Object, UBOOL bIsSavedInSeekFreePackage );

	/**
	 * Helper function used by CookObject - performs sound specific cooking.
	 */
	void CookSoundNodeWave( USoundNodeWave* SoundNodeWave );

	/**
	 * Make sure materials are compiled for Xbox 360 and add them to the shader cache embedded into seekfree packages.
	 * @param Material - Material to process
	 */
	void CompileMaterialShaders( UMaterial* Material );

	/**
	* Make sure material instances are compiled and add them to the shader cache embedded into seekfree packages.
	* @param MaterialInterface - MaterialInterface to process
	*/
	void CompileMaterialInstanceShaders( UMaterialInstance* MaterialInterface );

	/**
	 * Setup the commandlet's platform setting based on commandlet params
	 * @param Params The commandline parameters to the commandlet - should include "platform=xxx"
	 *
	 * @return TRUE if a good known platform was found in Params
	 */
	UBOOL SetPlatform(const FString& Params);

	/**
	 * Tried to load the DLLs and bind entry points.
	 *
	 * @return	TRUE if successful, FALSE otherwise
	 */
	UBOOL BindDLLs();

	/**
	 * Precreate all the .ini files that the platform will use at runtime
	 */
	void CreateIniFiles();

	/**
	 * Load all packages in a specified ini section with the Package= key
	 * @param SectionName Name of the .ini section ([Engine.PackagesToAlwaysCook])
	 * @param PackageNames Paths of the loaded packages
	 * @param KeyName Optional name for the key of the list to load (defaults to "Package")
	 * @param bShouldSkipLoading If TRUE, this function will only fill out PackageNames, and not load the package
	 */
	void LoadSectionPackages(const TCHAR* SectionName, TArray<FString>& PackageNames, const TCHAR* KeyName=TEXT("Package"), UBOOL bShouldSkipLoading=FALSE);

	/**
	 * Performs command line and engine specific initialization.
	 *
	 * @param	Params	command line
	 * @return	TRUE if successful, FALSE otherwise
	 */
	UBOOL Init( const TCHAR* Params );

	/**
	 * Generates list of src/ dst filename mappings of packages that need to be cooked after taking the command
	 * line options into account.
	 *
	 * @param [out] FirstRequiredIndex		index of first non-startup required package in returned array, untouched if there are none
	 * @param [out] FirstStartupIndex		index of first startup package in returned array, untouched if there are none
	 * @param [out]	FirstScriptIndex		index of first script package in returned array, untouched if there are none
	 * @param [out] FirstGameScriptIndex	index of first game script package in returned array, untouched if there are none
	 * @param [out] FirstMapIndex			index of first map package in returned array, untouched if there are none
	 * @param [out] FirstMPMapIndex			index of first map package in returned array, untouched if there are none
	 *
	 * @return	array of src/ dst filename mappings for packages that need to be cooked
	 */
	TArray<FPackageCookerInfo> GeneratePackageList( INT& FirstRequiredIndex, INT& FirstStartupIndex, INT& FirstScriptIndex, INT& FirstGameScriptIndex, INT& FirstMapIndex, INT& FirstMPMapIndex );
	/**
	 * Cleans up DLL handles and destroys cookers
	 */
	void Cleanup();
	/**
	 * Collects garbage and verifies all maps have been garbage collected.
	 */
	void CollectGarbageAndVerify();
	/**
	 * Handles duplicating cubemap faces that are about to be saved with the passed in package.
	 *
	 * @param	Package	 Package for which cubemaps that are going to be saved with it need to be handled.
	 */
	void HandleCubemaps( UPackage* Package );
	/**
	 * Saves the passed in package, gathers and stores bulk data info and keeps track of time spent.
	 *
	 * @param	Package						Package to save
	 * @param	Base						Base/ root object passed to SavePackage, can be NULL
	 * @param	TopLevelFlags				Top level "keep"/ save flags, passed onto SavePackage
	 * @param	DstFilename					Filename to save under
	 * @param	bStripEverythingButTextures	Whether to strip everything but textures
	 * @param	bCleanupAfterSave			Whether or not objects should have certain object flags cleared and remember if objects were saved
	 * @param	bRemeberSavedObjects		TRUE if objects should be marked to not be saved again, as well as materials (if bRemeberSavedObjects is set)
	 */
	void SaveCookedPackage( UPackage* Package, UObject* Base, EObjectFlags TopLevelFlags, const TCHAR* DstFilename, UBOOL bStripEverythingButTextures, UBOOL bCleanupAfterSave=FALSE, UBOOL bRemeberSavedObjects=FALSE );
	/**
	 * Returns whether there are any localized resources that need to be handled.
	 *
	 * @param Package			Current package that is going to be saved
	 * @param TopLevelFlags		TopLevelFlags that are going to be passed to SavePackage
	 * 
	 * @return TRUE if there are any localized resources pending save, FALSE otherwise
	 */
	UBOOL AreThereLocalizedResourcesPendingSave( UPackage* Package, EObjectFlags TopLevelFlags );

	/**
	 * Cooks the specified cues into the shared MP sound package.
	 *
	 * @param	MPCueNames		The set of sound cue names to cook.
	 */
	void CookMPSoundPackages(const TArray<FString>& MPCueNames);



	/**
	 * @return A string representing the platform
	 */
	FString GetPlatformString();

	/**
	 * @return THe prefix used in ini files, etc for the platform
	 */
	FString GetPlatformPrefix();

	/**
	 * @return The name of the output cooked data directory
	 */
	FString GetCookedDirectory();

	/**
	 * @return The name of the directory where cooked ini files go
	 */
	FString GetConfigOutputDirectory();

	/**
	 * @return The prefix to pass to appCreateIniNamesAndThenCheckForOutdatedness for non-platform specific inis
	 */
	FString GetConfigOutputPrefix();

	/**
	 * @return The prefix to pass to appCreateIniNamesAndThenCheckForOutdatedness for platform specific inis
	 */
	FString GetPlatformConfigOutputPrefix();

	/**
	 * @return The default ini prefix to pass to appCreateIniNamesAndThenCheckForOutdatedness for 
	 */
	FString GetPlatformDefaultIniPrefix();

	/**
	 * @return TRUE if the destination platform expects pre-byteswapped data (packages, coalesced ini files, etc)
	 */
	UBOOL ShouldByteSwapData();

END_COMMANDLET

BEGIN_COMMANDLET(PrecompileShaders, Editor)
END_COMMANDLET

BEGIN_COMMANDLET(RebuildMap,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(TestCompression,Editor)
	/**
	 * Run a compression/decompress test with the given package and compression options
	 *
	 * @param PackageName		The package to compress/decompress
	 * @param Flags				The options for compression
	 * @param UncompressedSize	The size of the uncompressed package file
	 * @param CompressedSize	The options for compressed package file
	 */
	void RunTest(const FFilename& PackageName, ECompressionFlags Flags, DWORD& UncompressedSize, DWORD& CompressedSize);
END_COMMANDLET

BEGIN_COMMANDLET(StripSource,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(MergePackages,Editor)
END_COMMANDLET


//====================================================================
// UDiffPackagesCommandlet and helper structs
//====================================================================

/**
 * The different types of comparison differences that can exist between packages.
 */
enum EObjectDiff
{
	/** no difference */
	OD_None,

	/** the object exist in the first package only */
	OD_AOnly,

	/** the object exists in the second package only */
	OD_BOnly,

	/** (three-way merges) the value has been changed from the ancestor package, but the new value is identical in the two packages being compared */
	OD_ABSame,

	/** @todo */
	OD_ABConflict,

	/** @todo */
	OD_Invalid,
};

/**
 * Contains an object and the object's path name.
 */
struct FObjectReference
{
	UObject* Object;
	FString ObjectPathName;

	FObjectReference( UObject* InObject )
	: Object(InObject)
	{
		if ( Object != NULL )
		{
			ObjectPathName = Object->GetPathName();
		}
	}
};


/**
 * Represents a single top-level object along with all its subobjects.
 */
struct FObjectGraph
{
	/**
	 * The list of objects in this object graph.  The first element is always the root object.
	 */
	TArray<struct FObjectReference> Objects;

	/**
	 * Constructor
	 *
	 * @param	RootObject			the top-level object for this object graph
	 * @param	PackageIndex		the index [into the Packages array] for the package that this object graph belongs to
	 * @param	ObjectsToIgnore		optional list of objects to not include in this object graph, even if they are contained within RootObject
	 */
	FObjectGraph( UObject* RootObject, INT PackageIndex, TArray<struct FObjectComparison>* ObjectsToIgnore=NULL);

	/**
	 * Returns the root of this object graph.
	 */
	inline UObject* GetRootObject() const { return Objects(0).Object; }
};

/**
 * Contains the natively serialized property data for a single UObject.
 */
struct FNativePropertyData 
{
	/** the object that this property data is for */
	UObject*				Object;

	/** the raw bytes corresponding to this object's natively serialized property data */
	TArray<BYTE>			PropertyData;

	/** the property names and textual representations of this object's natively serialized data */
	TMap<FString,FString>	PropertyText;

	/** Constructor */
	FNativePropertyData( UObject* InObject );

	/**
	 * Changes the UObject associated with this native property data container and re-initializes the
	 * PropertyData and PropertyText members
	 */
	void SetObject( UObject* NewObject );

	/** Comparison operators */
	inline UBOOL operator==( const FNativePropertyData& Other ) const
	{
		return ((Object == NULL) == (Other.Object == NULL)) && PropertyData == Other.PropertyData && PropertyText == Other.PropertyText;
	}
	inline UBOOL operator!=( const FNativePropertyData& Other ) const
	{
		return ((Object == NULL) != (Other.Object == NULL)) || PropertyData != Other.PropertyData || PropertyText != Other.PropertyText;
	}

	/** bool operator */
	inline operator UBOOL() const
	{
		return PropertyData.Num() || PropertyText.Num();
	}
};

BEGIN_COMMANDLET(DiffPackages,Editor)

	/**
	 * Parses the command-line and loads the packages being compared.
	 *
	 * @param	Parms	the full command-line used to invoke this commandlet
	 *
	 * @return	TRUE if all parameters were parsed successfully; FALSE if any of the specified packages couldn't be loaded
	 *			or the parameters were otherwise invalid.
	 */
	UBOOL Initialize( const TCHAR* Parms );

	/**
	 * Generates object graphs for the specified object and its corresponding objects in all packages being diffed.
	 *
	 * @param	RootObject			the object to generate the object comparison for
	 * @param	out_Comparison		the object graphs for the specified object for each package being diffed
	 * @param	ObjectsToIgnore		if specified, this list will be passed to the FObjectGraphs created for this comparison; this will prevent those object graphs from containing
	 *								these objects.  Useful when generating an object comparison for package-root type objects, such as levels, worlds, etc. to prevent their comparison's
	 *								object graphs from containing all objects in the level/world
	 *
	 * @return	TRUE if RootObject was found in any of the packages being compared.
	 */
	UBOOL GenerateObjectComparison( UObject* RootObject, struct FObjectComparison& out_Comparison, TArray<struct FObjectComparison>* ObjectsToIgnore=NULL );
	UBOOL ProcessDiff(struct FObjectComparison& Diff);

	EObjectDiff DiffObjects(UObject* ObjA, UObject* ObjB, UObject* ObjAncestor, struct FObjectComparison& PropDiffs);

	/**
	 * Copies the raw property values for the natively serialized properties of the specified object into the output var.
	 *
	 * @param	Object	the object to load values for
	 * @param	out_NativePropertyData	receives the raw bytes corresponding to Object's natively serialized property values.
	 */
	static void LoadNativePropertyData( UObject* Object, TArray<BYTE>& out_NativePropertyData );

	/**
	 * Compares the natively serialized property values for the specified objects by comparing the non-script serialized portion of each object's data as it
	 * is on disk.  If a different is detected, gives each object the chance to generate a textual representation of its natively serialized property values
	 * that will be displayed to the user in the final comparison report.
	 *
	 * @param	ObjA		the object from the first package being compared.  Can be NULL if both ObjB and ObjAncestor are valid, which indicates that this object
	 *						doesn't exist in the first package.
	 * @param	ObjB		the object from the second package being compared.  Can be NULL if both ObjA and ObjAncestor are valid, which indicates that this object
	 *						doesn't exist in the second package.
	 * @param	ObjAncestor	the object from the optional common base package.  Can only be NULL if both ObjA and ObjB are valid, which indicates that this is either
	 *						a two-comparison (if NumPackages == 2) or the object was added to both packages (if NumPackages == 3)
	 * @param	PropertyValueComparisons	contains the results for all property values that were different so far; for any native property values which are determined
	 *										to be different, new entries will be added to the ObjectComparison's list of PropDiffs.
	 *
	 * @return	The cumulative comparison result type for a comparison of all natively serialized property values.
	 */
	EObjectDiff CompareNativePropertyValues( UObject* ObjA, UObject* ObjB, UObject* ObjAncestor, struct FObjectComparison& PropertyValueComparisons );

	UBOOL bDiffNonEditProps;
	UBOOL bDiffAllProps;

	UPackage* Packages[3];
	INT NumPackages;
END_COMMANDLET

BEGIN_COMMANDLET(Make,Editor)
	/**
	 * Allows commandlets to override the default behavior and create a custom engine class for the commandlet. If
	 * the commandlet implements this function, it should fully initialize the UEngine object as well.  Commandlets
	 * should indicate that they have implemented this function by assigning the custom UEngine to GEngine.
	 */
	virtual void CreateCustomEngine();
	void DeleteEditPackages( INT StartIndex, INT Count=INDEX_NONE ) const;
END_COMMANDLET

BEGIN_COMMANDLET(ShowTaggedProps,Editor)

	/**
	 * Optional list of properties to display values for.  If this array is empty, all serialized property values are logged.
	 *
	 * @note: the map is just for quick lookup; the keys and values are the same, so it should be thought of as an array.
	 */
	TMap<UProperty*,UProperty*> SearchProperties;
	void ShowSavedProperties( UObject* Object ) const;
END_COMMANDLET

BEGIN_COMMANDLET(ShowObjectCount,Editor)
	void StaticInitialize();
END_COMMANDLET

BEGIN_COMMANDLET(CreateDefaultStyle,Editor)
	class UUISkin* DefaultSkin;

	class UUIStyle_Text* CreateTextStyle( const TCHAR* StyleName=TEXT("DefaultTextStyle"), FLinearColor StyleColor=FLinearColor(1.f,1.f,1.f,1.f) ) const;
	class UUIStyle_Image* CreateImageStyle( const TCHAR* StyleName=TEXT("DefaultImageStyle"), FLinearColor StyleColor=FLinearColor(1.f,1.f,1.f,1.f) ) const;
	class UUIStyle_Combo* CreateComboStyle( UUIStyle_Text* TextStyle, UUIStyle_Image* ImageStyle, const TCHAR* StyleName=TEXT("DefaultComboStyle") ) const;

	void CreateAdditionalStyles() const;

	void CreateConsoleStyles() const;

	void CreateMouseCursors() const;
END_COMMANDLET

BEGIN_COMMANDLET(ShowStyles,Editor)
	void DisplayStyleInfo( class UUIStyle* Style );

	INT GetStyleDataIndent( class UUIStyle* Style );
END_COMMANDLET

BEGIN_COMMANDLET(ExamineOuters,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(ListCorruptedComponents,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(FindSoundCuesWithMissingGroups,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(FindTexturesWithMissingPhysicalMaterials,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(FindQuestionableTextures,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(FindDuplicateKismetObjects,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(SetTextureLODGroup,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(WrangleContent,Editor)
END_COMMANDLET


/** lists all content referenced in the default properties of script classes */
BEGIN_COMMANDLET(ListScriptReferencedContent,Editor)
	/** processes a value found by ListReferencedContent(), possibly recursing for inline objects
	 * @param Value the object to be processed
	 * @param Property the property where Value was found (for a dynamic array, this is the Inner property)
	 * @param PropertyDesc string printed as the property Value was assigned to (usually *Property->GetName(), except for arrays, where it's the array name and index)
	 * @param Tab string with a number of tabs for the current tab level of the output
	 */
	void ProcessObjectValue(UObject* Value, UProperty* Property, const FString& PropertyDesc, const FString& Tab);
	/** lists content referenced by the given data
	 * @param Struct the type of the Default data
	 * @param Default the data to look for referenced objects in
	 * @param HeaderName header string printed before any content references found (only if the data might contain content references)
	 * @param Tab string with a number of tabs for the current tab level of the output
	 */
	void ListReferencedContent(UStruct* Struct, BYTE* Default, const FString& HeaderName, const FString& Tab = TEXT(""));
END_COMMANDLET

BEGIN_COMMANDLET(FixAmbiguousMaterialParameters,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(TestWordWrap,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(SetMaterialUsage,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(SetPackageFlags,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(PIEToNormal,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(UT3MapStats,Editor)
END_COMMANDLET

BEGIN_COMMANDLET(PerformMapCheck,Editor)
END_COMMANDLET

#endif
