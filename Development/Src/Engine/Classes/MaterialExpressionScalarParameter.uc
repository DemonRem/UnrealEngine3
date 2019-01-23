/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class MaterialExpressionScalarParameter extends MaterialExpressionParameter
	native(Material)
	collapsecategories
	hidecategories(Object);

var() name	ParameterName;
var() float	DefaultValue;

cpptext
{
	virtual INT Compile(FMaterialCompiler* Compiler);
	virtual FString GetCaption() const;
}
