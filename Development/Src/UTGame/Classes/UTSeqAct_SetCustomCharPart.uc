/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTSeqAct_SetCustomCharPart extends SequenceAction;

var()	UTCustomChar_Data.ECharPart		Part;
var()	string							PartID;

defaultproperties
{
	ObjName="Set Character Part"
	ObjCategory="CustomChar"
}