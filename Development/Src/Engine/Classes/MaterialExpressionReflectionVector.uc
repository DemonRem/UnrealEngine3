/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class MaterialExpressionReflectionVector extends MaterialExpression
	native(Material)
	collapsecategories
	hidecategories(Object);

cpptext
{
	virtual INT Compile(FMaterialCompiler* Compiler);
	virtual FString GetCaption() const;
}
