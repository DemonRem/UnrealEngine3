/**
 * MaterialEditorInstance.uc: This class is used by the material instance editor to hold a set of inherited parameters which are then pushed to a material instance.
 * Copyright � 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class MaterialEditorInstance extends Object
	native
	hidecategories(Object)
	collapsecategories;

struct native EditorVectorParameterValue
{
	var() bool			bOverride;
	var() name			ParameterName;
	var() LinearColor	ParameterValue;
};

struct native EditorScalarParameterValue
{
	var() bool		bOverride;
	var() name		ParameterName;
	var() float		ParameterValue;
};

struct native EditorTextureParameterValue
{
	var() bool		bOverride;
    var() name		ParameterName;
    var() Texture	ParameterValue;
};

struct native EditorFontParameterValue
{
	var() bool		bOverride;
    var() name		ParameterName;
    var() Font		FontValue;
	var() int		FontPage;
};

struct native EditorStaticSwitchParameterValue
{
	var() bool		bOverride;
    var() name		ParameterName;
    var() bool		ParameterValue;
	var   Guid		ExpressionId;
};

struct native ComponentMaskParameter
{
	var() bool R;
	var() bool G;
	var() bool B;
	var() bool A;
};

struct native EditorStaticComponentMaskParameterValue
{
	var() bool		bOverride;
    var() name		ParameterName;
    var() ComponentMaskParameter		ParameterValue;
	var   Guid		ExpressionId;
};

/** Physical material to use for this graphics material. Used for sounds, effects etc.*/
var() PhysicalMaterial						PhysMaterial;

var() MaterialInstance						Parent;
var() array<EditorVectorParameterValue>		VectorParameterValues;
var() array<EditorScalarParameterValue>		ScalarParameterValues;
var() array<EditorTextureParameterValue>	TextureParameterValues;
var() array<EditorFontParameterValue>		FontParameterValues;
var() array<EditorStaticSwitchParameterValue>	StaticSwitchParameterValues;
var() array<EditorStaticComponentMaskParameterValue>	StaticComponentMaskParameterValues;
var	  MaterialInstanceConstant				SourceInstance;


cpptext
{
	// UObject interface.
	virtual void PostEditChange(UProperty* PropertyThatChanged);

	/** Regenerates the parameter arrays. */
	void RegenerateArrays();

	/** Copies the parameter array values back to the source instance. */
	void CopyToSourceInstance();

	/** Copies static parameters to the source instance, which will be marked dirty if a compile was necessary */
	void CopyStaticParametersToSourceInstance();

	/** 
	 * Sets the source instance for this object and regenerates arrays. 
	 *
	 * @param MaterialInstance		Instance to use as the source for this material editor instance.
	 */
	void SetSourceInstance(UMaterialInstanceConstant* MaterialInstance);
}