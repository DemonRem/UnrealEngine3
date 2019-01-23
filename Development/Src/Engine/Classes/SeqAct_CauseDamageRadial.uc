/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqAct_CauseDamageRadial extends SequenceAction
	native(Sequence);

cpptext
{
	void Activated();
}

/** Type of damage to apply */
var() class<DamageType>		DamageType;

/** Amount of momentum to apply */
var() float					Momentum;

/** Amount of damage to apply */
var() float			DamageAmount;

/** Distance to Instigator within which to damage actors */
var()	float		DamageRadius;

/** Whether damage should decay linearly based on distance from the instigator. */
var()	bool		bDamageFalloff;

/** player that should take credit for the damage (Controller or Pawn) */
var Actor Instigator;

defaultproperties
{
	ObjName="Cause Damage Radial"
	ObjCategory="Actor"
	ObjClassVersion=2

	VariableLinks(1)=(ExpectedType=class'SeqVar_Float',LinkDesc="Amount",PropertyName=DamageAmount)
	VariableLinks(2)=(ExpectedType=class'SeqVar_Object',LinkDesc="Instigator",PropertyName=Instigator)
	Momentum=500.f
	DamageRadius=200.f
}
