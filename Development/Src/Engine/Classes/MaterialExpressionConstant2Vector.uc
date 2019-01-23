/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class MaterialExpressionConstant2Vector extends MaterialExpression
	native(Material)
	collapsecategories
	hidecategories(Object);

var() float	R,
			G;

cpptext
{
	virtual INT Compile(FMaterialCompiler* Compiler);
	virtual FString GetCaption() const;
}
