/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqAct_RandomSwitch extends SeqAct_Switch
	native(Sequence);

cpptext
{
	virtual void Activated()
	{
		// build a list of enabled links
		TArray<INT> ValidLinks;
		for (INT Idx = 0; Idx < OutputLinks.Num(); Idx++)
		{
			if (!OutputLinks(Idx).bDisabled && !(OutputLinks(Idx).bDisabledPIE && GIsEditor))
			{
				ValidLinks.AddItem(Idx);
			}
		}
		if (ValidLinks.Num() > 0)
		{
			// pick a random link to activate
			INT OutIdx = ValidLinks(appRand() % ValidLinks.Num());
			OutputLinks(OutIdx).bHasImpulse = TRUE;
			if (bAutoDisableLinks)
			{
				OutputLinks(OutIdx).bDisabled = TRUE;
			}
			// fill any variables attached
			for (INT Idx = 0; Idx < Indices.Num(); Idx++)
			{
				// offset by 1 for non-programmer friendliness
				Indices(Idx) = OutIdx + 1;
			}
		}
	}
};

defaultproperties
{
	ObjName="Random"
	ObjCategory="Switch"
	VariableLinks.Empty
	VariableLinks(0)=(ExpectedType=class'SeqVar_Int',LinkDesc="Active Link",bWriteable=true,MinVars=0,PropertyName=Indices)
}
