/**
 * This specialized version of UIString is used by preview panels in style editors.  Since those strings are created using
 * CDOs as their Outer, the menu state used to apply style data must be set manually.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved
 */
class UIPreviewString extends UIString
	native(Private)
	transient;

var		private{private}	UIState			CurrentMenuState;

cpptext
{
	/* === UIPreviewString interface === */
	/**
	 * Changes the current menu state for this UIPreviewString.
	 */
	void SetCurrentMenuState( class UUIState* NewMenuState );

	/* === UIString interface === */
	/**
	 * Retrieves the UIState that should be used for applying style data.
	 */
	virtual class UUIState* GetCurrentMenuState() const;
}

DefaultProperties
{

}
