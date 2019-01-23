/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTSkelControl_DarkWalkerHeadTurret extends UTSkelControl_TurretConstrained
	native(Animation);

var(DarkWalker) name FramePlayerName;
var transient UTAnimNodeFramePlayer Player;

cpptext
{
	virtual void TickSkelControl(FLOAT DeltaSeconds, USkeletalMeshComponent* SkelComp);
}


defaultproperties
{
}
