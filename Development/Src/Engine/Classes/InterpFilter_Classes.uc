/** 
 * InterpFilter_Classes.uc: Filter class for filtering matinee groups.  
 * Used by the matinee editor to filter groups to specific classes of attached actors.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class InterpFilter_Classes extends InterpFilter
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

/** Which class to filter groups by. */
var editoronly class<Object>	ClassToFilterBy;