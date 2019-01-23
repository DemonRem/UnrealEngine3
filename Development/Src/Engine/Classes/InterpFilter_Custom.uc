/** 
 * InterpFilter_Custom.uc: Filter class for filtering matinee groups.  
 * Used by the matinee editor to let users organize tracks/groups.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class InterpFilter_Custom extends InterpFilter
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

/** Which groups are included in this filter. */
var editoronly	array<InterpGroup>	GroupsToInclude;