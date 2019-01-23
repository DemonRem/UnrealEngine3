/**
 * Copyright 2004-2007 Epic Games, Inc. All Rights Reserved.
 */
class MaterialExpressionStaticSwitchParameter extends MaterialExpressionParameter
	native(Material)
	collapsecategories
	hidecategories(Object);

var() name	ParameterName;
var() bool	DefaultValue;
var() bool	ExtendedCaptionDisplay;

var ExpressionInput A;
var ExpressionInput B;

//the override that will be set when this expression is being compiled from a static permutation
var const native transient pointer InstanceOverride{const FStaticSwitchParameter};

cpptext
{
	virtual INT Compile(FMaterialCompiler* Compiler);
	virtual FString GetCaption() const;

	/**
	 * Replaces references to the passed in expression with references to a different expression or NULL.
	 * @param	OldExpression		Expression to find reference to.
	 * @param	NewExpression		Expression to replace reference with.
	 */
	virtual void SwapReferenceTo(UMaterialExpression* OldExpression,UMaterialExpression* NewExpression = NULL);
}
