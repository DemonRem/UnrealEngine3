/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


/** Causes target actor to make a noise that AI will hear */
class SeqAct_MakeNoise extends SequenceAction;

var() float Loudness;

defaultproperties
{
	ObjName="MakeNoise"
	ObjCategory="AI"
	ObjClassVersion=2

	Loudness=1.f
}