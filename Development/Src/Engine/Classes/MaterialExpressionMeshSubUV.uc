/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class MaterialExpressionMeshSubUV extends MaterialExpressionTextureSample
	native(Material);

cpptext
{
	virtual INT Compile(FMaterialCompiler* Compiler);
	virtual void GetOutputs(TArray<FExpressionOutput>& Outputs) const;
	virtual INT GetWidth() const;
	virtual FString GetCaption() const;
	virtual INT GetLabelPadding() { return 8; }
}
