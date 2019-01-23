/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTAnimBlendByWalkerState extends UTAnimBlendBase
	native(Animation);

cpptext
{
	virtual	void TickAnim( FLOAT DeltaSeconds, FLOAT TotalWeight );
}

defaultproperties
{
	Children(0)=(Name="Parked"),Weight=1.0
	Children(1)=(Name="Crouched")
	Children(2)=(Name="Driven")

	bFixNumChildren=true
}
