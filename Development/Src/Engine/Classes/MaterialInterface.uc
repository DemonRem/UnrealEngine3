/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class MaterialInterface extends Surface
	abstract
	native;

cpptext
{

	// GetMaterial - Get the material which this is an instance of.
	virtual class UMaterial* GetMaterial() PURE_VIRTUAL(UMaterialInterface::GetMaterial,return NULL;);

	/**
	* Tests this material instance for dependency on a given material instance.
	* @param	TestDependency - The material instance to test this instance for dependency upon.
	* @return	True if the material instance is dependent on TestDependency.
	*/
	virtual UBOOL IsDependent(UMaterialInterface* TestDependency) { return FALSE; }

	/**
	* Returns a pointer to the FMaterialRenderProxy used for rendering.
	*
	* @param	Selected	specify TRUE to return an alternate material used for rendering this material when part of a selection
	*						@note: only valid in the editor!
	*
	* @return	The resource to use for rendering this material instance.
	*/
	virtual FMaterialRenderProxy* GetRenderProxy(UBOOL Selected) const PURE_VIRTUAL(UMaterialInterface::GetRenderProxy,return NULL;);

	/**
	* Returns a pointer to the physical material used by this material instance.
	* @return The physical material.
	*/
	virtual UPhysicalMaterial* GetPhysicalMaterial() const PURE_VIRTUAL(UMaterialInterface::GetPhysicalMaterial,return NULL;);

	/**
	* Gathers the textures used to render the material instance.
	* @param OutTextures	Upon return contains the textures used to render the material instance.
	* @param bOnlyAddOnce	Whether to add textures that are sampled multiple times uniquely or not
	*/
	void GetTextures(TArray<UTexture*>& OutTextures,UBOOL bOnlyAddOnce = TRUE);

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
	 * Checks if the material has the appropriate flag set to be used with gamma correction.
	 * If it's the editor, and it isn't set, it sets it.
	 * If it's the game, and it isn't set, it will return False.
	 * @return True if the material may be used with gamma correction, False if the non-gamma correction version can be used
	 */
	UBOOL UseWithGammaCorrection();
	
	/**
	 * Checks if the material has the appropriate flag set to be used with lens flare.
	 * If it's the editor, and it isn't set, it sets it.
	 * If it's the game, and it isn't set, it will return False.
	 * @return True if the material may be used with lens flare, False if the default material should be used.
	 */
	UBOOL UseWithLensFlare();

	/**
	* Allocates a new material resource
	* @return	The allocated resource
	*/
	virtual FMaterialResource* AllocateResource() PURE_VIRTUAL(UMaterialInterface::AllocateResource,return NULL;);

	/**
	 * Gets the static permutation resource if the instance has one
	 * @return - the appropriate FMaterialResource if one exists, otherwise NULL
	 */
	virtual FMaterialResource * GetMaterialResource(EShaderPlatform Platform=GRHIShaderPlatform) { return NULL; }

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
	virtual UBOOL CompileStaticPermutation(
		FStaticParameterSet* StaticParameters, 
		FMaterialResource* StaticPermutation, 
		EShaderPlatform Platform, 
		EMaterialShaderPlatform MaterialPlatform,
		UBOOL bFlushExistingShaderMaps)
		PURE_VIRTUAL(UMaterialInterface::CompileStaticPermutation,return FALSE;);

	/**
	* Gets the value of the given static switch parameter
	*
	* @param	ParameterName	The name of the static switch parameter
	* @param	OutValue		Will contain the value of the parameter if successful
	* @return					True if successful
	*/
	virtual UBOOL GetStaticSwitchParameterValue(FName ParameterName,UBOOL &OutValue,FGuid &OutExpressionGuid) 
		PURE_VIRTUAL(UMaterialInterface::GetStaticSwitchParameterValue,return FALSE;);

	/**
	* Gets the value of the given static component mask parameter
	*
	* @param	ParameterName	The name of the parameter
	* @param	R, G, B, A		Will contain the values of the parameter if successful
	* @return					True if successful
	*/
	virtual UBOOL GetStaticComponentMaskParameterValue(FName ParameterName, UBOOL &R, UBOOL &G, UBOOL &B, UBOOL &A, FGuid &OutExpressionGuid) 
		PURE_VIRTUAL(UMaterialInterface::GetStaticComponentMaskParameterValue,return FALSE;);

	/**
	* Sets overrides in the material's static parameters
	*
	* @param	Permutation		The set of static parameters to override and their values	
	*/
	virtual void SetStaticParameterOverrides(const FStaticParameterSet* Permutation) PURE_VIRTUAL(UMaterialInterface::SetStaticParameterOverrides,return;);

	/**
	* Clears static parameter overrides so that static parameter expression defaults will be used
	*	for subsequent compiles.
	*/
	virtual void ClearStaticParameterOverrides() PURE_VIRTUAL(UMaterialInterface::ClearStaticParameterOverrides,return;);

	virtual UBOOL IsFallbackMaterial() { return FALSE; }

	/** @return The material's view relevance. */
	FMaterialViewRelevance GetViewRelevance();

	INT GetWidth() const;
	INT GetHeight() const;

	// USurface interface
	virtual FLOAT GetSurfaceWidth() const { return GetWidth(); }
	virtual FLOAT GetSurfaceHeight() const { return GetHeight(); }
}

/** The mesh used by the material editor to preview the material.*/
var() editoronly string PreviewMesh;

native final noexport function Material GetMaterial();

/**
* Returns a pointer to the physical material used by this material instance.
* @return The physical material.
*/
native final noexport function PhysicalMaterial GetPhysicalMaterial() const;

// Get*ParameterValue - Gets the entry from the ParameterValues for the named parameter.
// Returns false is parameter is not found.


native function bool GetFontParameterValue(name ParameterName,out font OutFontValue, out int OutFontPage);
native function bool GetScalarParameterValue(name ParameterName, out float OutValue);
native function bool GetScalarCurveParameterValue(name ParameterName, out InterpCurveFloat OutValue);
native function bool GetTextureParameterValue(name ParameterName, out Texture OutValue);
native function bool GetVectorParameterValue(name ParameterName, out LinearColor OutValue);



defaultproperties
{

}
