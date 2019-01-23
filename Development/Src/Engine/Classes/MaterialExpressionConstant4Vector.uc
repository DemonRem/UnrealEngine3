/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class MaterialExpressionConstant4Vector extends MaterialExpression
	native(Material)
	collapsecategories
	hidecategories(Object);

var() float	R,
			G,
			B,
			A;

cpptext
{
	virtual INT Compile(FMaterialCompiler* Compiler);
	virtual FString GetCaption() const;
}
