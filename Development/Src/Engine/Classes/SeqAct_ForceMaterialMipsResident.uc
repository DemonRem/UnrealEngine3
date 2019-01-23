/**
 * SeqAct_WaitForLevelsVisible
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class SeqAct_ForceMaterialMipsResident extends SeqAct_Latent
	native(Sequence);

cpptext
{
	virtual void Activated();
	virtual UBOOL UpdateOp(FLOAT DeltaTime);
};

/** Time to enforce the bForceMiplevelsToBeResident for. */
var()	float						ForceDuration;

/** Array of Materials to set bForceMiplevelsToBeResident on their textures for the duration of this action. */
var()	array<MaterialInterface>		ForceMaterials;

/** Time left before we reset the flag. */
var		float						RemainingTime;

/** Textures that need to reset bForceMiplevelsToBeResident to FALSE once action completes. */
var		array<Texture2D>			ModifiedTextures;

defaultproperties
{
	ObjName="Force Material Mips Resident"
	ObjCategory="Misc"

	ForceDuration = 1.f

	InputLinks(0)=(LinkDesc="Start")

	VariableLinks.Empty
}