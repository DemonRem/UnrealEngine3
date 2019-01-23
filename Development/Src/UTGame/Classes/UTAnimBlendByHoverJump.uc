/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTAnimBlendByHoverJump extends UTAnimBlendByFall
	Native(Animation);

var const transient UTPawn	OwnerUTP;

cpptext
{
	virtual void InitAnim(USkeletalMeshComponent* MeshComp, UAnimNodeBlendBase* Parent );
	virtual	void TickAnim( float DeltaSeconds, float TotalWeight  );
}


defaultproperties
{
	bIgnoreDoubleJumps=true
}
