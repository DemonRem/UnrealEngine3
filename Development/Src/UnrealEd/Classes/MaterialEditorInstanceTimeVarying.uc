/**
 * MaterialEditorInstanceTimeVaryingTimeVarying.uc: This class is used by the material instance editor to hold a set of inherited parameters which are then pushed to a material instance.
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class MaterialEditorInstanceTimeVarying extends Object
	native
	hidecategories(Object)
	collapsecategories;

struct native EditorVectorParameterValueOverTime
{
	var() bool			bOverride;
	var() name			ParameterName;
	var() LinearColor	ParameterValue;
};

struct native EditorScalarParameterValueOverTime
{
	var() bool		bOverride;
	var() name		ParameterName;
	var() float		ParameterValue;
	var() InterpCurveFloat ParameterValueCurve;
};

struct native EditorTextureParameterValueOverTime
{
	var() bool		bOverride;
    var() name		ParameterName;
    var() Texture	ParameterValue;
};

struct native EditorFontParameterValueOverTime
{
	var() bool		bOverride;
    var() name		ParameterName;
    var() Font		FontValue;
	var() int		FontPage;
};

struct native EditorStaticSwitchParameterValueOverTime
{
	var() bool		bOverride;
    var() name		ParameterName;
    var() bool		ParameterValue;
	var   Guid		ExpressionId;
};

struct native ComponentMaskParameterOverTime
{
	var() bool R;
	var() bool G;
	var() bool B;
	var() bool A;
};

struct native EditorStaticComponentMaskParameterValueOverTime
{
	var() bool		bOverride;
    var() name		ParameterName;
    var() ComponentMaskParameterOverTime		ParameterValue;
	var   Guid		ExpressionId;
};

/** Physical material to use for this graphics material. Used for sounds, effects etc.*/
var() PhysicalMaterial						PhysMaterial;

var() MaterialInterface						Parent;
var() array<EditorVectorParameterValueOverTime>		VectorParameterValues;
var() array<EditorScalarParameterValueOverTime>		ScalarParameterValues;
var() array<EditorTextureParameterValueOverTime>	TextureParameterValues;
var() array<EditorFontParameterValueOverTime>		FontParameterValues;
var() array<EditorStaticSwitchParameterValueOverTime>	StaticSwitchParameterValues;
var() array<EditorStaticComponentMaskParameterValueOverTime>	StaticComponentMaskParameterValues;
var	  MaterialInstanceTimeVarying				SourceInstance;


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
	 * @param MaterialInterface		Instance to use as the source for this material editor instance.
	 */
	void SetSourceInstance(UMaterialInstanceTimeVarying* MaterialInterface);
}
