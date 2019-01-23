/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTSeqAct_TurretTrack extends SeqAct_Interp
	native(Sequence)
	hidecategories(SeqAct_Interp);

cpptext
{
	virtual UBOOL UpdateOp(FLOAT deltaTime);
	virtual void StepInterp(FLOAT DeltaTime, UBOOL bPreview=false);
	virtual void Activated();
	virtual void DeActivated();
}

function Reset()
{
	SetPosition(0.0, false);
}

defaultproperties
{
	ObjName="Turret Track"
	InputLinks.Empty()
	InputLinks(0)=(LinkDesc="Spawned")
	OutputLinks.Empty()
}
