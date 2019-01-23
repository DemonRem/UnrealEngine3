/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqAct_SetMatInstTexParam extends SequenceAction
	native(Sequence);

cpptext
{
	void Activated();
}

var() MaterialInstanceConstant	MatInst;
var() Texture					NewTexture;
var() Name						ParamName;

defaultproperties
{
	ObjName="Set TextureParam"
	ObjCategory="Material Instance"
	VariableLinks.Empty()
}
