/*=============================================================================
	Factories.h: Unreal Engine factory types.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __FACTORIES_H__
#define __FACTORIES_H__


class UTextureCubeFactoryNew : public UFactory
{
	DECLARE_CLASS(UTextureCubeFactoryNew,UFactory,CLASS_CollapseCategories|CLASS_Intrinsic,Editor)
	void StaticConstructor();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateNew( UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn );
};

class UMaterialInstanceConstantFactoryNew : public UFactory
{
	DECLARE_CLASS(UMaterialInstanceConstantFactoryNew,UFactory,CLASS_CollapseCategories|CLASS_Intrinsic,Editor);
public:

	void StaticConstructor();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn);
};

class UMaterialInstanceTimeVaryingFactoryNew : public UFactory
{
	DECLARE_CLASS(UMaterialInstanceTimeVaryingFactoryNew,UFactory,CLASS_CollapseCategories|CLASS_Intrinsic,Editor);
public:

	void StaticConstructor();
	/**
	* Initializes property values for intrinsic classes.  It is called immediately after the class default object
	* is initialized against its archetype, but before any objects of this class are created.
	*/
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn);
};

class UMaterialFactoryNew : public UFactory
{
	DECLARE_CLASS(UMaterialFactoryNew,UFactory,CLASS_CollapseCategories,Editor);
public:

	void StaticConstructor();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn);
};

class UDecalMaterialFactoryNew : public UFactory
{
	DECLARE_CLASS(UDecalMaterialFactoryNew,UFactory,CLASS_CollapseCategories,Editor);
public:

	void StaticConstructor();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn);
};

class UTerrainMaterialFactoryNew : public UFactory
{
	DECLARE_CLASS(UTerrainMaterialFactoryNew,UFactory,CLASS_CollapseCategories,Editor);
public:

	void StaticConstructor();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn);
};

class UTerrainLayerSetupFactory : public UFactory
{
	DECLARE_CLASS(UTerrainLayerSetupFactory,UFactory,CLASS_CollapseCategories,Editor);
	UTerrainLayerSetupFactory();
	void StaticConstructor();
	UObject* FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn);
};

class UTerrainLayerSetupFactoryNew : public UFactory
{
	DECLARE_CLASS(UTerrainLayerSetupFactoryNew,UFactory,CLASS_CollapseCategories,Editor);
public:

	void StaticConstructor();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn);
};

class UClassFactoryUC : public UFactory
{
	DECLARE_CLASS(UClassFactoryUC,UFactory,0,Editor)
	UClassFactoryUC();
	UObject* FactoryCreateText( UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn );
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	static const TCHAR* const ProcessedFileExtension;
	static const TCHAR* const ProcessedFileDirectory;
	static const TCHAR* const ExportPostProcessedParameter;
	static const TCHAR* const SuppressPreprocessorParameter;

private:
	/**
	 * Parses the text specified for a collection of comma-delimited class names, surrounded by parenthesis, using the specified keyword.
	 *
	 * @param	InputText				pointer to the text buffer to parse; will be advanced past the text that was parsed.
	 * @param	GroupName				the group name to parse (i.e. DependsOn, Implements, Inherits, etc.)
	 * @param	out_ParsedClassNames	receives the list of class names that were parsed.
	 *
	 * @return	TRUE if the group name specified was found and entries were added to the list
	 */
	UBOOL ParseDependentClassGroup( const TCHAR*& InputText, const TCHAR* const GroupName, TArray<FName>& out_ParsedClassNames );
};

class ULevelFactory : public UFactory
{
	DECLARE_CLASS(ULevelFactory,UFactory,0,Editor)
	ULevelFactory();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateText( UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn );
};

class UPolysFactory : public UFactory
{
	DECLARE_CLASS(UPolysFactory,UFactory,0,Editor)
	UPolysFactory();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateText( UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn );
};

class UModelFactory : public UFactory
{
	DECLARE_CLASS(UModelFactory,UFactory,0,Editor)
	UModelFactory();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateText( UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn );
};

class UStaticMeshFactory : public UFactory
{
	DECLARE_CLASS(UStaticMeshFactory,UFactory,0,Editor)
	INT	Pitch,
		Roll,
		Yaw;
	UBOOL bOneConvexPerUCXObject;
	UStaticMeshFactory();
	void StaticConstructor();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateText( UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn );
};

class USkeletalMeshFactory : public UFactory
{
	DECLARE_CLASS(USkeletalMeshFactory,UFactory,0,Editor)	
    UBOOL bAssumeMayaCoordinates;
	USkeletalMeshFactory();
	void StaticConstructor();	
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateBinary( UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const BYTE*& Buffer, const BYTE* BufferEnd, FFeedbackContext* Warn );
};

class UColladaFactory : public UFactory
{
	DECLARE_CLASS(UColladaFactory,UFactory,0,Editor)	

	/** If TRUE, import the mesh as a skeletal mesh.  If FALSE, import the mesh as a static mesh.*/
	UBOOL bImportAsSkeletalMesh;

	UColladaFactory();
	void StaticConstructor();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UClass* ResolveSupportedClass();
	UObject* FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn);
};

class USoundFactory : public UFactory
{
	DECLARE_CLASS( USoundFactory, UFactory, 0, Editor )
	USoundFactory( void );
	void StaticConstructor( void );
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues( void );
	UObject* FactoryCreateBinary( UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const BYTE*& Buffer, const BYTE* BufferEnd, FFeedbackContext* Warn );
	UBOOL bAutoCreateCue;

	/**
	 * If TRUE, add an attenuation node in series with the wave.
	 */
	UBOOL bIncludeAttenuationNode;

	FLOAT CueVolume;
};

class USoundTTSFactory : public UFactory
{
	DECLARE_CLASS( USoundTTSFactory, UFactory, 0, Editor )

	void StaticConstructor( void );

	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues( void );

	/** 
	 * Create a new instance of USoundNodeWave
	 */
	UObject* FactoryCreateNew( UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn );

	/** 
	 * Automatically create sound cue with this wave
	 */
	UBOOL bAutoCreateCue;

	/**
	 * If TRUE, add an attenuation node in series with the wave.
	 */
	UBOOL bIncludeAttenuationNode;

	UBOOL bUseTTS;
	FString SpokenText;
	FLOAT CueVolume;
};

class USoundSurroundFactory : public UFactory
{
	DECLARE_CLASS( USoundSurroundFactory, UFactory, 0, Editor )
	USoundSurroundFactory( void );
	void StaticConstructor( void );
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues( void );
	UObject* FactoryCreateBinary( UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const BYTE*& Buffer, const BYTE* BufferEnd, FFeedbackContext* Warn );
	UBOOL FactoryCanImport( const FFilename& Filename );

	FLOAT CueVolume;

	static const FString SpeakerLocations[SPEAKER_Count];
};

class USoundCueFactoryNew : public UFactory
{
	DECLARE_CLASS( USoundCueFactoryNew, UFactory, 0, Editor )
	void StaticConstructor( void );
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues( void );
	UObject* FactoryCreateNew( UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn );
};

class UFonixFactory : public UFactory
{
	DECLARE_CLASS( UFonixFactory, UFactory, 0, Editor )
	UFonixFactory( void );
	void StaticConstructor( void );
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues( void );
	UObject* FactoryCreateNew( UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn );
};

class ULensFlareFactoryNew : public UFactory
{
	DECLARE_CLASS(ULensFlareFactoryNew,UFactory,0,Editor)
	void StaticConstructor();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn);
};

class UParticleSystemFactoryNew : public UFactory
{
	DECLARE_CLASS(UParticleSystemFactoryNew,UFactory,0,Editor)
	void StaticConstructor();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn);
};

class UAnimSetFactoryNew : public UFactory
{
	DECLARE_CLASS(UAnimSetFactoryNew,UFactory,0,Editor)
	void StaticConstructor();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn);
};

class UAnimTreeFactoryNew : public UFactory
{
	DECLARE_CLASS(UAnimTreeFactoryNew,UFactory,0,Editor)
	void StaticConstructor();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn);
};

class UPostProcessFactoryNew : public UFactory
{
	DECLARE_CLASS(UPostProcessFactoryNew,UFactory,0,Editor)
	void StaticConstructor();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn);
};

class UPhysicalMaterialFactoryNew : public UFactory
{
	DECLARE_CLASS(UPhysicalMaterialFactoryNew,UFactory,0,Editor)
	void StaticConstructor();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn);
};

class UTextureMovieFactory : public UFactory
{
	DECLARE_CLASS(UTextureMovieFactory,UFactory,CLASS_CollapseCategories,Editor);

	/** load to memory or stream from file */
	BYTE	MovieStreamSource;

	UTextureMovieFactory();
	void StaticConstructor();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateBinary( UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const BYTE*& Buffer, const BYTE* BufferEnd, FFeedbackContext* Warn );
};

/** 
 * Factory that creates TextureRenderTarget objects
 */
class UTextureRenderTargetFactoryNew : public UFactory
{
	DECLARE_CLASS(UTextureRenderTargetFactoryNew,UFactory,0,Editor);

	/** width of new texture */
	INT		Width;
	/** height of new texture */
	INT		Height;
	/** surface format of new texture */
	BYTE	Format;
	
	UTextureRenderTargetFactoryNew();
	void StaticConstructor();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn);
};

/** 
 * Factory that creates TextureRenderTargetCube objects
 */
class UTextureRenderTargetCubeFactoryNew : public UFactory
{
	DECLARE_CLASS(UTextureRenderTargetCubeFactoryNew,UFactory,0,Editor);

	/** width of new texture */
	INT		Width;
	/** surface format of new texture */
	BYTE	Format;
	
	UTextureRenderTargetCubeFactoryNew();
	void StaticConstructor();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn);
};

class UTextureFactory : public UFactory
{
	DECLARE_CLASS(UTextureFactory,UFactory,CLASS_CollapseCategories,Editor)
	UBOOL						NoCompression,
								NoAlpha,
								bDeferCompression;
	BYTE						CompressionSettings;

	UBOOL bCreateMaterial, bRGBToDiffuse, bRGBToEmissive, bAlphaToSpecular, bAlphaToEmissive, bAlphaToOpacity, bAlphaToOpacityMask, bTwoSided;
	BYTE Blending;
	BYTE LightingModel;
	BYTE LODGroup;
	UBOOL bFlipBook;
	UBOOL bDitherMipMapAlpha;
	UBOOL bPreserveBorderR;
	UBOOL bPreserveBorderG;
	UBOOL bPreserveBorderB;
	UBOOL bPreserveBorderA;

	UTextureFactory();
	void StaticConstructor();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateBinary( UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const BYTE*& Buffer, const BYTE* BufferEnd, FFeedbackContext* Warn );
	/**
	* Create a texture given the appropriate input parameters
	*/
	virtual UTexture2D* CreateTexture( UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags );
};

/**
* Factory to handle texture reimporting from source media to package files
*/
class UReimportTextureFactory : public UTextureFactory, FReimportHandler
{
	DECLARE_CLASS(UReimportTextureFactory,UTextureFactory,CLASS_CollapseCategories,Editor)    

	UReimportTextureFactory();
	UTexture2D* CreateTexture( UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags );
	UTexture2D* pOriginalTex;    

public:
	void StaticConstructor();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();

	/**
	* Reimports specified texture from its source material, if the meta-data exists
	* @param Package texture to reimport
	*/
	virtual UBOOL Reimport( UObject* Obj );
};

class UVolumeTextureFactory : public UFactory
{
	DECLARE_CLASS(UVolumeTextureFactory,UFactory,CLASS_CollapseCategories,Editor)
	UBOOL						NoAlpha,
								bDeferCompression;
	UVolumeTextureFactory();
	void StaticConstructor();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateBinary( UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const BYTE*& Buffer, const BYTE* BufferEnd, FFeedbackContext* Warn );
};

enum EFontCharacterSet
{
	CHARSET_Default,
	CHARSET_Ansi,
	CHARSET_Symbol,
};

class UFontFactory : public UTextureFactory
{
	DECLARE_CLASS(UFontFactory,UTextureFactory,0,Editor)
	UFontFactory();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateBinary( UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const BYTE*& Buffer, const BYTE* BufferEnd, FFeedbackContext* Warn );
};

class UTrueTypeFontFactory : public UFontFactory
{
	DECLARE_CLASS(UTrueTypeFontFactory,UFontFactory,CLASS_CollapseCategories,Editor)
	FStringNoInit	FontName;
	FLOAT			Height;
	INT				USize;
	INT				VSize;
	INT				XPad;
	INT				YPad;
	INT				Count;
	FLOAT			Gamma;
	FStringNoInit	Chars;
	UBOOL			AntiAlias;
	FString			UnicodeRange;
	FString			Wildcard;
	FString			Path;
	INT				Style; 
	INT				Italic;
	INT				Underline;
	INT             Kerning;
	INT             DropShadowX;
	INT             DropShadowY;
	INT				ExtendBoxTop;
	INT				ExtendBoxBottom;
	INT				ExtendBoxRight;
	INT				ExtendBoxLeft;

	/**
	 * Whether to generate non-printable characters or not
	 */
	UBOOL			bCreatePrintableOnly;

	/**
	 * The character set to use when importing this font.
	 */
	BYTE			FontCharacterSet;

	UTrueTypeFontFactory();
	void StaticConstructor();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateNew( UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn );
	UTexture2D* CreateTextureFromDC( UFont* Font, DWORD dc, INT RowHeight, INT TextureNum );
};

class UTrueTypeMultiFontFactory : public UTrueTypeFontFactory
{
	DECLARE_CLASS(UTrueTypeMultiFontFactory,UTrueTypeFontFactory,CLASS_CollapseCategories,Editor)

	TArray<FLOAT>			ResTests;
	TArray<FLOAT>			ResHeights;
	TArrayNoInit<UFont*>	ResFonts;

	UTrueTypeMultiFontFactory();
	void StaticConstructor();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateNew( UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn );
};

class USphericalHarmonicMapFactorySHM : public UFactory
{
	DECLARE_CLASS(USphericalHarmonicMapFactorySHM,UFactory,0,Editor);
public:

	USphericalHarmonicMapFactorySHM();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateBinary(
		UClass* InClass,
		UObject* InOuter,
		FName InName,
		EObjectFlags InFlags,
		UObject* Context,
		const TCHAR* Type,
		const BYTE*& Buffer,
		const BYTE* BufferEnd,
		FFeedbackContext* Warn
		);
};

class USequenceFactory : public UFactory
{
	DECLARE_CLASS(USequenceFactory,UFactory,0,Editor);
	USequenceFactory();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateText( UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn );
};

struct	FNoiseParameter;
struct	FFilterLimit;
class	UTerrainLayerSetup;
struct	FTerrainFilteredMaterial;
struct	FTerrainDecorationInstance;
struct	FTerrainDecoration;
struct	FTerrainDecoLayer;
struct	FTerrainLayer;
class	UTerrainHeightMapFactory;
struct	FAlphaMap;

class UTerrainFactory : public UFactory
{
	DECLARE_CLASS(UTerrainFactory, UFactory, 0, Editor);
	UTerrainFactory();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn);

	TMap<AActor*,FString>*	ActorMap;

	UBOOL	ParseNoiseParameter(FNoiseParameter* Noise, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn);
	UBOOL	ParseFilterLimit(FFilterLimit* Limit, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn);
	UBOOL	ParseLayerSetupMaterial(ATerrain* Terrain, UTerrainLayerSetup& Setup, FTerrainFilteredMaterial& Material, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn);
	UBOOL	ParseLayerSetup(ATerrain* Terrain, FTerrainLayer& Layer, UTerrainLayerSetup& Setup, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn);
	UBOOL	ParseLayer(ATerrain* Terrain, FTerrainLayer& Layer, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn);
	UBOOL	ParseLayerData(ATerrain* Terrain, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn);
	UBOOL	ParseDecorationInstace(ATerrain* Terrain, FTerrainDecorationInstance& Instance, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn);
	UBOOL	ParseDecoration(ATerrain* Terrain, FTerrainDecoration& Decoration, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn);
	UBOOL	ParseDecoLayer(ATerrain* Terrain, FTerrainDecoLayer& DecoLayer, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn);
	UBOOL	ParseDecoLayerData(ATerrain* Terrain, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn);
	UBOOL	ParseAlphaMap(FAlphaMap& AlphaMap, INT Count, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn);
	UBOOL	ParseAlphaMapData(ATerrain* Terrain, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn);

	static	UTerrainHeightMapFactory*	GetHeightMapImporter(const TCHAR* FactoryName);
};

class UTerrainHeightMapFactory : public UObject
{
	DECLARE_CLASS(UTerrainHeightMapFactory, UObject,CLASS_Abstract|CLASS_Intrinsic, Editor);
	UTerrainHeightMapFactory();
	void StaticConstructor();
	virtual UBOOL ImportHeightDataFromText(ATerrain* Terrain, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn, UBOOL bGenerateTerrainFromHeightMap = false);
	virtual UBOOL ImportHeightDataFromBinary(ATerrain* Terrain, const BYTE*& Buffer, const BYTE* BufferEnd, FFeedbackContext* Warn, UBOOL bGenerateTerrainFromHeightMap = false);
};

class UTerrainHeightMapFactoryTextT3D : public UTerrainHeightMapFactory
{
	DECLARE_CLASS(UTerrainHeightMapFactoryTextT3D, UTerrainHeightMapFactory, 0, Editor);
	UTerrainHeightMapFactoryTextT3D();
	void StaticConstructor();
	virtual UBOOL ImportHeightDataFromText(ATerrain* Terrain, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn, UBOOL bGenerateTerrainFromHeightMap = false);
};

class UTerrainHeightMapFactoryG16BMP : public UTerrainHeightMapFactory
{
	DECLARE_CLASS(UTerrainHeightMapFactoryG16BMP, UTerrainHeightMapFactory, 0, Editor);
	UTerrainHeightMapFactoryG16BMP();
	void StaticConstructor();
	virtual UBOOL ImportHeightDataFromFile(ATerrain* Terrain, const TCHAR* FileName, FFeedbackContext* Warn, UBOOL bGenerateTerrainFromHeightMap = false);
	virtual UBOOL ImportHeightDataFromBinary(ATerrain* Terrain, const BYTE*& Buffer, const BYTE* BufferEnd, FFeedbackContext* Warn, UBOOL bGenerateTerrainFromHeightMap = false);
	virtual UBOOL ImportLayerDataFromFile(ATerrain* Terrain, FTerrainLayer* Layer, const TCHAR* FileName, FFeedbackContext* Warn);
	virtual UBOOL ImportLayerDataFromBinary(ATerrain* Terrain, FTerrainLayer* Layer, const BYTE*& Buffer, const BYTE* BufferEnd, FFeedbackContext* Warn);
};

class UTerrainHeightMapFactoryG16BMPT3D : public UTerrainHeightMapFactoryG16BMP
{
	DECLARE_CLASS(UTerrainHeightMapFactoryG16BMPT3D, UTerrainHeightMapFactoryG16BMP, 0, Editor);
	UTerrainHeightMapFactoryG16BMPT3D();
	void StaticConstructor();
	virtual UBOOL ImportHeightDataFromText(ATerrain* Terrain, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn, UBOOL bGenerateTerrainFromHeightMap = false);
	virtual UBOOL ImportHeightDataFromBinary(ATerrain* Terrain, const BYTE*& Buffer, const BYTE* BufferEnd, FFeedbackContext* Warn, UBOOL bGenerateTerrainFromHeightMap = false);
};


/** morph target import error codes */
enum EMorphImportError
{
	// success
	MorphImport_OK=0,
	// target mesh exists
	MorphImport_AlreadyExists,
	// source file was not loaded
	MorphImport_CantLoadFile,
	// source file format is invalid
	MorphImport_InvalidMeshFormat,
	// source mesh vertex data doesn't match base
	MorphImport_MismatchBaseMesh,
	// source mesh is missing its metadata
	// needs to be reimported
	MorphImport_ReimportBaseMesh,
	// LOD index was out of range by more than 1
	MorphImport_InvalidLODIndex,
	// Missing morph target
	MorphImport_MissingMorphTarget,
	// max
	MorphImport_MAX
};

/**
 * Utility class for importing a new morph target
 */
class FMorphTargetBinaryImport
{
public:
	/** for outputing warnings */
	FFeedbackContext* Warn;
	/** raw mesh data used for calculating differences */
	FMorphMeshRawSource BaseMeshRawData;
	/** base mesh lod entry to use */
	INT BaseLODIndex;
	/** the base mesh */
	UObject* BaseMesh;

	FMorphTargetBinaryImport( USkeletalMesh* InSrcMesh, INT LODIndex=0, FFeedbackContext* InWarn=GWarn );
	FMorphTargetBinaryImport( UStaticMesh* InSrcMesh, INT LODIndex=0, FFeedbackContext* InWarn=GWarn );

	UMorphTarget* ImportMorphMeshToSet( UMorphTargetSet* DestMorphSet, FName Name, const TCHAR* SrcFilename, UBOOL bReplaceExisting, EMorphImportError* Error=NULL );
	void ImportMorphLODModel( UMorphTarget* MorphTarget, const TCHAR* SrcFilename, INT LODIndex, EMorphImportError* Error=NULL );
};

/** CurveEdPresetCurve factory */
class UCurveEdPresetCurveFactoryNew : public UFactory
{
	DECLARE_CLASS(UCurveEdPresetCurveFactoryNew,UFactory,CLASS_CollapseCategories,Editor);
public:

	void StaticConstructor();
	/**
	 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
	 * is initialized against its archetype, but before any objects of this class are created.
	 */
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn);
};

/** CameraAnim factory */
class UCameraAnimFactory : public UFactory
{
	DECLARE_CLASS(UCameraAnimFactory,UFactory,0,Editor);
public:
	void StaticConstructor();
	/**
	* Initializes property values for intrinsic classes.  It is called immediately after the class default object
	* is initialized against its archetype, but before any objects of this class are created.
	*/
	void InitializeIntrinsicPropertyValues();
	UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn);

};

class USpeedTreeFactory : public UFactory
{
	DECLARE_CLASS(USpeedTreeFactory, UFactory, 0, Editor)

	USpeedTreeFactory(void);
	void       StaticConstructor(void);
	void       InitializeIntrinsicPropertyValues(void);

	UObject*   FactoryCreateBinary(UClass* InClass,
		UObject* InOuter,
		FName InName,
		EObjectFlags InFlags,
		UObject* Context,
		const TCHAR* Type,
		const BYTE*& Buffer,
		const BYTE* BufferEnd,
		FFeedbackContext* Warn);

	INT        RandomSeed;
	UBOOL      bUseWindyBranches;
};


#endif // __FACTORIES_H__
