/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class MaterialExpressionVectorParameter extends MaterialExpressionParameter
	native(Material)
	collapsecategories
	hidecategories(Object);

var() name			ParameterName;
var() LinearColor	DefaultValue;

cpptext
{
	virtual INT Compile(FMaterialCompiler* Compiler);
	virtual void GetOutputs(TArray<FExpressionOutput>& Outputs) const;
	virtual FString GetCaption() const;
}
