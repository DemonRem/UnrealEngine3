/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqAct_GiveInventory extends SequenceAction;

var() array<class<Inventory> >			InventoryList;
var() bool bClearExisting;

defaultproperties
{
	ObjName="Give Inventory"
	ObjCategory="Pawn"
}
