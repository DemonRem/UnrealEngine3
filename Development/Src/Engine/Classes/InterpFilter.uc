/** 
 * InterpFilter.uc: Filter class for filtering matinee groups.  
 * By default no groups are filtered.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class InterpFilter extends Object
	native(Interpolation);

cpptext
{
	/** 
	 * Given a interpdata object, outputs the groups to display after being filtered.
	 *
	 * @param InData			Data to filter.
	 * @param OutInterpGroups	Filtered array of groups to display.
	 */
	virtual void FilterData(class USeqAct_Interp* InData, TArray<class UInterpGroup*> &OutInterpGroups);
}

/** Caption for this filter. */
var string Caption;
