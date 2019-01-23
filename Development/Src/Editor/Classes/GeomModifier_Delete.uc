/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

/**
 * Deletes selected objects.
 */
class GeomModifier_Delete
	extends GeomModifier_Edit
	native;
	
cpptext
{
	/**
	 * @return		TRUE if the specified selection type is supported by this modifier, FALSE otherwise.
	 */
	virtual UBOOL Supports(INT InSelType);

protected:
	/**
	 * Implements the modifier application.
	 */
 	virtual UBOOL OnApply();
}
	
defaultproperties
{
	Description="Delete"
	bPushButton=True
}
