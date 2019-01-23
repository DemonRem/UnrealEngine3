/**
* Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
*/
class MaterialInstanceTimeVarying extends MaterialInstance
	native(Material);


struct native FontParameterValueOverTime
{
	var() name		ParameterName;
	var() Font		FontValue;
	var() int		FontPage;
	var	  guid		ExpressionGUID;
};

struct native ScalarParameterValueOverTime
{
	var() name	ParameterName;

	/** This allows MITVs to have both single scalar and curve values **/
	var() float	ParameterValue;

	/** when this is parameter is to start "ticking" then this value will be set to the current game time **/
	var float StartTime;
	/** This will auto activate this MITV **/
	var() bool bAutoActivate;

	/** This will automatically be used if there are any values in this Curve **/
	var() InterpCurveFloat ParameterValueCurve;

	var	  guid	ExpressionGUID;
};

struct native TextureParameterValueOverTime
{
	var() name		ParameterName;
	var() Texture	ParameterValue;
	var	  guid		ExpressionGUID;
};

struct native VectorParameterValueOverTime
{
	var() name			ParameterName;
	var() LinearColor	ParameterValue;
	var	  guid			ExpressionGUID;
};



var() const array<FontParameterValueOverTime>		FontParameterValues;
var() const array<ScalarParameterValueOverTime>		ScalarParameterValues;
var() const array<TextureParameterValueOverTime>	TextureParameterValues;
var() const array<VectorParameterValueOverTime>		VectorParameterValues;


cpptext
{
	// Constructor.
	UMaterialInstanceTimeVarying();

	// UMaterialInterface interface.
	virtual UBOOL GetFontParameterValue(FName ParameterName,class UFont*& OutFontValue, INT& OutFontPage);
	/**
	 * For MITVs you can utilize both single Scalar values and InterpCurve values.
	 *
	 * If there is any data in the InterpCurve, then the MITV will utilize that. Else it will utilize the Scalar value
	 * of the same name.
	 **/
	virtual UBOOL GetScalarParameterValue(FName ParameterName,FLOAT& OutValue);
	virtual UBOOL GetScalarCurveParameterValue(FName ParameterName,FInterpCurveFloat& OutValue);
	virtual UBOOL GetTextureParameterValue(FName ParameterName,class UTexture*& OutValue);
	virtual UBOOL GetVectorParameterValue(FName ParameterName,FLinearColor& OutValue);

	// UObject interface.
	virtual void PostLoad();
	virtual void PostEditChange(UProperty* PropertyThatChanged);

	/**
	* Refreshes parameter names using the stored reference to the expression object for the parameter.
	*/
	virtual void UpdateParameterNames();
};



// SetParent - Updates the parent.

native function SetParent(MaterialInterface NewParent);

// Set*ParameterValue - Updates the entry in ParameterValues for the named parameter, or adds a new entry.

/**
 * For MITVs you can utilize both single Scalar values and InterpCurve values.
 *
 * If there is any data in the InterpCurve, then the MITV will utilize that. Else it will utilize the Scalar value
 * of the same name.
 **/
native function SetScalarParameterValue(name ParameterName, float Value);
native function SetScalarCurveParameterValue(name ParameterName, InterpCurveFloat Value);

/** This sets how long after the MITV has been spawned to start "ticking" the named Scalar InterpCurve **/
native function SetScalarStartTime(name ParameterName, float Value);


native function SetTextureParameterValue(name ParameterName, Texture Value);
native function SetVectorParameterValue(name ParameterName, LinearColor Value);

/**
* Sets the value of the given font parameter.
*
* @param	ParameterName	The name of the font parameter
* @param	OutFontValue	New font value to set for this MIC
* @param	OutFontPage		New font page value to set for this MIC
*/
native function SetFontParameterValue(name ParameterName, Font FontValue, int FontPage);


/** Removes all parameter values */
native function ClearParameterValues();



defaultproperties
{

}
