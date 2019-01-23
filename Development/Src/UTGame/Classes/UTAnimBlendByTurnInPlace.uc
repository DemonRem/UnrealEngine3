/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTAnimBlendByTurnInPlace extends UTAnimBlendBase
	native(Animation);

var() float	RootYawSpeedThresh;
var() float TurnInPlaceBlendSpeed;
var const transient UTPawn OwnerUTP;

cpptext
{
	// AnimNode interface
	virtual void InitAnim(USkeletalMeshComponent* MeshComp, UAnimNodeBlendBase* Parent);
	virtual	void TickAnim( float DeltaSeconds, FLOAT TotalWeight  );
}

defaultproperties
{
	Children(0)=(Name="Idle",Weight=1.0)
	Children(1)=(Name="TurnInPlace")
	bFixNumChildren=true
}
