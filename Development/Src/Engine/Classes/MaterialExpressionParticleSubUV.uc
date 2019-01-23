/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
// class MaterialExpressionParticleSubUV extends MaterialExpression
class MaterialExpressionParticleSubUV extends MaterialExpressionTextureSample
	native(Material);

//var() Texture		Texture;
//var() INT			SubDivisions_Horizontal;
//var() INT			SubDivisions_Vertical;

cpptext
{
	virtual INT Compile(FMaterialCompiler* Compiler);
	virtual void GetOutputs(TArray<FExpressionOutput>& Outputs) const;
	virtual INT GetWidth() const;
	virtual FString GetCaption() const;
	virtual INT GetLabelPadding() { return 8; }
}
