/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class Material extends MaterialInterface
	native
	hidecategories(object)
	collapsecategories;

enum EBlendMode
{
	BLEND_Opaque,
	BLEND_Masked,
	BLEND_Translucent,
	BLEND_Additive,
	BLEND_Modulate
};

enum EMaterialLightingModel
{
	MLM_Phong,
	MLM_NonDirectional,
	MLM_Unlit,
	MLM_SHPRT,
	MLM_Custom
};

// Material input structs.

struct MaterialInput
{
	var MaterialExpression	Expression;
	var int					Mask,
							MaskR,
							MaskG,
							MaskB,
							MaskA;
	var int					GCC64_Padding; // @todo 64: if the C++ didn't mismirror this structure (with ExpressionInput), we might not need this
};

struct ColorMaterialInput extends MaterialInput
{
	var bool				UseConstant;
	var color	Constant;
};

struct ScalarMaterialInput extends MaterialInput
{
	var bool				UseConstant;
	var float	Constant;
};

struct VectorMaterialInput extends MaterialInput
{
	var bool				UseConstant;
	var vector	Constant;
};

struct Vector2MaterialInput extends MaterialInput
{
	var bool				UseConstant;
	var float	ConstantX,
				ConstantY;
};

// Physics.

/** Physical material to use for this graphics material. Used for sounds, effects etc.*/
var() PhysicalMaterial		PhysMaterial;

/** For backwards compatibility only. */
var class<PhysicalMaterial>	PhysicalMaterial;

// Reflection.

var ColorMaterialInput		DiffuseColor;
var ColorMaterialInput		SpecularColor;
var ScalarMaterialInput		SpecularPower;
var VectorMaterialInput		Normal;

// Emission.

var ColorMaterialInput		EmissiveColor;

// Transmission.

var ScalarMaterialInput		Opacity;
var ScalarMaterialInput		OpacityMask;

/** If BlendMode is BLEND_Masked, the surface is not rendered where OpacityMask < OpacityMaskClipValue. */
var() float OpacityMaskClipValue;

var Vector2MaterialInput	Distortion;

var() EBlendMode BlendMode;

var() EMaterialLightingModel LightingModel;

var ColorMaterialInput		CustomLighting;

var ScalarMaterialInput		TwoSidedLightingMask;
var ColorMaterialInput		TwoSidedLightingColor;
var() bool TwoSided;

var(Usage) const bool bUsedAsLightFunction;
var(Usage) const bool bUsedWithFogVolumes;
var(Usage) const bool bUsedAsSpecialEngineMaterial;
var(Usage) const bool bUsedWithSkeletalMesh;
var		   const bool bUsedWithParticleSystem;
var(Usage) const bool bUsedWithParticleSprites;
var(Usage) const bool bUsedWithBeamTrails;
var(Usage) const bool bUsedWithParticleSubUV;
var(Usage) const bool bUsedWithFoliage;
var(Usage) const bool bUsedWithSpeedTree;
var /*(Usage)*/ const bool bUsedWithStaticLighting;
var(Usage) const bool bUsedWithLensFlare;
/** Adds an extra pow instruction to the shader using the current render target's gamma value */
var(Usage) const bool bUsedWithGammaCorrection;

var() bool Wireframe;

var private deprecated bool	Unlit;
var private deprecated bool	NonDirectionalLighting;

/**
 * This is deprecated(moved into FMaterialResource), but the GUID is used to allow caching shaders in Gemini
 * without requiring a resave of all packages.
 */
var private deprecated Guid	PersistentIds[2];

/** Indicates that the material will be used as a fallback on sm2 platforms */
var(Usage) bool bIsFallbackMaterial;

/** The fallback material, which will be used on sm2 platforms */
var() Material FallbackMaterial;

// Two resources for sm3 and sm2, indexed by EMaterialShaderPlatform
var const native duplicatetransient pointer MaterialResources[2]{FMaterialResource};

var const native duplicatetransient pointer DefaultMaterialInstances[2]{class FDefaultMaterialInstance};

var int		EditorX,
			EditorY,
			EditorPitch,
			EditorYaw;

/** Array of material expressions, excluding Comments and Compounds.  Used by the material editor. */
var array<MaterialExpression>			Expressions;

/** Array of comments associated with this material; viewed in the material editor. */
var editoronly array<MaterialExpressionComment>	EditorComments;

/** Array of material expression compounds associated with this material; viewed in the material editor. */
var editoronly array<MaterialExpressionCompound> EditorCompounds;

/** TRUE if Material uses distortion */
var private bool						bUsesDistortion;

/** TRUE if Material uses a scene color exprssion */
var private bool						bUsesSceneColor;

/** TRUE if Material is masked and uses custom opacity */
var private bool						bIsMasked;

/** TRUE if Material is the preview material used in the material editor. */
var transient duplicatetransient private bool bIsPreviewMaterial;

/** Array of textures referenced, set in PreSave. */
var private const array<texture> ReferencedTextures;

cpptext
{
	// Constructor.
	UMaterial();

	/** @return TRUE if the material uses distortion */
	UBOOL HasDistortion() const;
	/** @return TRUE if the material uses the scene color texture */
	UBOOL UsesSceneColor() const;

	/**
	 * Allocates a material resource off the heap to be stored in MaterialResource.
	 */
	virtual FMaterialResource* AllocateResource();

	/**
	 * Checks if the material has the appropriate flag set to be used with skeletal meshes.
	 * If it's the editor, and it isn't set, it sets it.
	 * If it's the game, and it isn't set, it will return False.
	 * @return True if the material may be used with the skeletal mesh, False if the default material should be used.
	 */
	UBOOL UseWithSkeletalMesh();

	/**
	 * Checks if the material has the appropriate flag set to be used with particle sprites.
	 * If it's the editor, and it isn't set, it sets it.
	 * If it's the game, and it isn't set, it will return False.
	 * @return True if the material may be used with particle sprites, False if the default material should be used.
	 */
	UBOOL UseWithParticleSprites();

	/**
	 * Checks if the material has the appropriate flag set to be used with beam trails.
	 * If it's the editor, and it isn't set, it sets it.
	 * If it's the game, and it isn't set, it will return False.
	 * @return True if the material may be used with beam trails, False if the default material should be used.
	 */
	UBOOL UseWithBeamTrails();

	/**
	 * Checks if the material has the appropriate flag set to be used with SubUV Particles.
	 * If it's the editor, and it isn't set, it sets it.
	 * If it's the game, and it isn't set, it will return False.
	 * @return True if the material may be used with SubUV Particles, False if the default material should be used.
	 */
	UBOOL UseWithParticleSubUV();

	/**
	 * Checks if the material has the appropriate flag set to be used with foliage.
	 * If it's the editor, and it isn't set, it sets it.
	 * If it's the game, and it isn't set, it will return False.
	 * @return True if the material may be used with the foliage, False if the default material should be used.
	 */
	UBOOL UseWithFoliage();

	/**
	 * Checks if the material has the appropriate flag set to be used with speed trees.
	 * If it's the editor, and it isn't set, it sets it.
	 * If it's the game, and it isn't set, it will return False.
	 * @return True if the material may be used with speed trees, False if the default material should be used.
	 */
	UBOOL UseWithSpeedTree();

	/**
	 * Checks if the material has the appropriate flag set to be used with static lighting.
	 * If it's the editor, and it isn't set, it sets it.
	 * If it's the game, and it isn't set, it will return False.
	 * @return True if the material may be used with static lighting, False if the default material should be used.
	 */
	UBOOL UseWithStaticLighting();

	/**
	 * Checks if the material has the appropriate flag set to be used with lens flare.
	 * If it's the editor, and it isn't set, it sets it.
	 * If it's the game, and it isn't set, it will return False.
	 * @return True if the material may be used with lens flare, False if the default material should be used.
	 */
	UBOOL UseWithLensFlare();

	/**
	 * Checks if the material has the appropriate flag set to be used with gamma correction.
	 * If it's the editor, and it isn't set, it sets it.
	 * If it's the game, and it isn't set, it will return False.
	 * @return True if the material may be used with gamma correction, False if the non-gamma correction version can be used
	 */
	UBOOL UseWithGammaCorrection();

	/**
	 * @param	OutParameterNames		Storage array for the parameter names we are returning.
	 *
	 * @return	Returns a array of vector parameter names used in this material.
	 */
	virtual void GetAllVectorParameterNames(TArray<FName> &OutParameterNames);

	/**
	 * @param	OutParameterNames		Storage array for the parameter names we are returning.
	 *
	 * @return	Returns a array of scalar parameter names used in this material.
	 */
	virtual void GetAllScalarParameterNames(TArray<FName> &OutParameterNames);

	/**
	 * @param	OutParameterNames		Storage array for the parameter names we are returning.
	 *
	 * @return	Returns a array of texture parameter names used in this material.
	 */
	virtual void GetAllTextureParameterNames(TArray<FName> &OutParameterNames);

	/**
	 * @param	OutParameterNames		Storage array for the parameter names we are returning.
	 *
	 * @return	Returns a array of font parameter names used in this material.
	 */
	virtual void GetAllFontParameterNames(TArray<FName> &OutParameterNames);

	/**
	 * @param	OutParameterNames		Storage array for the parameter names we are returning.
	 *
	 * @return	Returns a array of static switch parameter names used in this material.
	 */
	virtual void GetAllStaticSwitchParameterNames(TArray<FName> &OutParameterNames);

	/**
	 * @param	OutParameterNames		Storage array for the parameter names we are returning.
	 *
	 * @return	Returns a array of static component mask parameter names used in this material.
	 */
	virtual void GetAllStaticComponentMaskParameterNames(TArray<FName> &OutParameterNames);

	/**
	 * Attempts to find a expression by its GUID.
	 *
	 * @param InGUID GUID to search for.
	 *
	 * @return Returns a expression object pointer if one is found, otherwise NULL if nothing is found.
	 */
	template<typename ExpressionType>
	ExpressionType* FindExpressionByGUID(const FGuid &InGUID)
	{
		ExpressionType* Result = NULL;

		for(INT ExpressionIndex = 0;ExpressionIndex < Expressions.Num();ExpressionIndex++)
		{
			ExpressionType* ExpressionPtr =
				Cast<ExpressionType>(Expressions(ExpressionIndex));

			if(ExpressionPtr && ExpressionPtr->ExpressionGUID.IsValid() && ExpressionPtr->ExpressionGUID==InGUID)
			{
				Result = ExpressionPtr;
				break;
			}
		}

		return Result;
	}

	/**
	 * Builds a string of parameters in the fallback material that do not exist in the base material.
	 * These parameters won't be set by material instances, which get their parameter list from the base material.
	 *
	 * @param ParameterMismatches - string of unmatches material names to populate
	 */
	virtual void GetFallbackParameterInconsistencies(FString &ParameterMismatches);

	// UMaterialInterface interface.
	virtual UMaterial* GetMaterial();
    virtual UBOOL GetVectorParameterValue(FName ParameterName,FLinearColor& OutValue);
    virtual UBOOL GetScalarParameterValue(FName ParameterName,FLOAT& OutValue);
    virtual UBOOL GetTextureParameterValue(FName ParameterName,class UTexture*& OutValue);
	virtual UBOOL GetFontParameterValue(FName ParameterName,class UFont*& OutFontValue,INT& OutFontPage);

	/**
	 * Gets the value of the given static switch parameter
	 *
	 * @param	ParameterName	The name of the static switch parameter
	 * @param	OutValue		Will contain the value of the parameter if successful
	 * @return					True if successful
	 */
	virtual UBOOL GetStaticSwitchParameterValue(FName ParameterName,UBOOL &OutValue,FGuid &OutExpressionGuid);

	/**
	 * Gets the value of the given static component mask parameter
	 *
	 * @param	ParameterName	The name of the parameter
	 * @param	R, G, B, A		Will contain the values of the parameter if successful
	 * @return					True if successful
	 */
	virtual UBOOL GetStaticComponentMaskParameterValue(FName ParameterName, UBOOL &R, UBOOL &G, UBOOL &B, UBOOL &A, FGuid &OutExpressionGuid);
    
	virtual FMaterialRenderProxy* GetRenderProxy(UBOOL Selected) const;
	virtual UPhysicalMaterial* GetPhysicalMaterial() const;

	/**
	 * Compiles a FMaterialResource on the given platform with the given static parameters
	 *
	 * @param StaticParameters - The set of static parameters to compile for
	 * @param StaticPermutation - The resource to compile
	 * @param Platform - The platform to compile for
	 * @param MaterialPlatform - The material platform to compile for
	 * @param bFlushExistingShaderMaps - Indicates that existing shader maps should be discarded
	 * @return TRUE if compilation was successful or not necessary
	 */
	UBOOL CompileStaticPermutation(
		FStaticParameterSet* StaticParameters, 
		FMaterialResource* StaticPermutation, 
		EShaderPlatform Platform,
		EMaterialShaderPlatform MaterialPlatform,
		UBOOL bFlushExistingShaderMaps);

	/**
	 * Sets overrides in the material's static parameters
	 *
	 * @param	Permutation		The set of static parameters to override and their values	
	 */
	void SetStaticParameterOverrides(const FStaticParameterSet* Permutation);

	/**
	 * Clears static parameter overrides so that static parameter expression defaults will be used
	 *	for subsequent compiles.
	 */
	void ClearStaticParameterOverrides();

	/**
	 * Compiles material resources for the current platform if the shader map for that resource didn't already exist.
	 *
	 * @param bFlushExistingShaderMaps - forces a compile, removes existing shader maps from shader cache.
	 * @param bForceAllPlatforms - compile for all platforms, not just the current.
	 */
	void CacheResourceShaders(UBOOL bFlushExistingShaderMaps=FALSE, UBOOL bForceAllPlatforms=FALSE);

	/**
	 * Flushes existing resource shader maps and resets the material resource's Ids.
	 */
	virtual void FlushResourceShaderMaps();

	/**
	 * Gets the material resource based on the input platform
	 * @return - the appropriate FMaterialResource if one exists, otherwise NULL
	 */
	virtual FMaterialResource * GetMaterialResource(EShaderPlatform Platform=GRHIShaderPlatform);

	// UObject interface.
	/**
	 * Called before serialization on save to propagate referenced textures. This is not done
	 * during content cooking as the material expressions used to retrieve this information will
	 * already have been dissociated via RemoveExpressions
	 */
	void PreSave();

	virtual void Serialize(FArchive& Ar);
	virtual void PostLoad();
	virtual void PreEditChange(UProperty* PropertyAboutToChange);
	virtual void PostEditChange(UProperty* PropertyThatChanged);
	virtual void BeginDestroy();
	virtual UBOOL IsReadyForFinishDestroy();
	virtual void FinishDestroy();

	/**
	 * @return		Sum of the size of textures referenced by this material.
	 */
	virtual INT GetResourceSize();

	/**
	 * Used by various commandlets to purge Editor only data from the object.
	 * 
	 * @param TargetPlatform Platform the object will be saved for (ie PC vs console cooking, etc)
	 */
	virtual void StripData(UE3::EPlatformType TargetPlatform);

	/**
	* Null any material expression references for this material
	*/
	void RemoveExpressions();

	UBOOL IsFallbackMaterial() { return bIsFallbackMaterial; }

	/**
	 * Goes through every material, flushes the specified types and re-initializes the material's shader maps.
	 */
	static void UpdateMaterialShaders(TArray<FShaderType*>& ShaderTypesToFlush, TArray<FVertexFactoryType*>& VFTypesToFlush);
}


/** returns the Referneced Textures so one may set flats on them  (e.g. bForceMiplevelsToBeResident ) **/
function array<texture> GetTextures()
{
	return ReferencedTextures;
}


defaultproperties
{
	BlendMode=BLEND_Opaque
	Unlit=False
	DiffuseColor=(Constant=(R=128,G=128,B=128))
	SpecularColor=(Constant=(R=128,G=128,B=128))
	SpecularPower=(Constant=15.0)
	Distortion=(ConstantX=0,ConstantY=0)
	Opacity=(Constant=1)
	OpacityMask=(Constant=1)
	OpacityMaskClipValue=0.3333
	TwoSidedLightingColor=(Constant=(R=255,G=255,B=255))
	bIsFallbackMaterial=False
	bUsedWithStaticLighting=TRUE
}
