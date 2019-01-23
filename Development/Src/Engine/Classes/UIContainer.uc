/**
 * Base class for all widgets which act as containers or grouping boxes for other widgets.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UIContainer extends UIObject
	notplaceable
	HideDropDown
	native(UIPrivate);

cpptext
{
	/* === UUIScreenObject interface === */
	/**
	 * Render this widget.
	 *
	 * @param	Canvas	the canvas to use for rendering this widget
	 */
	virtual void Render_Widget( FCanvas* Canvas ) {}

	/**
	 * Evalutes the Position value for the specified face into an actual pixel value.  Should only be
	 * called from UIScene::ResolvePositions.  Any special-case positioning should be done in this function.
	 *
	 * @param	Face	the face that should be resolved
	 */
	virtual void ResolveFacePosition( EUIWidgetFace Face );

	/* === UObject interface === */
	/**
	 * Called when a property value has been changed in the editor.
	 */
	virtual void PostEditChange( UProperty* PropertyThatChanged );

	/**
	 * Called when a property value has been changed in the editor.
	 */
	virtual void PostEditChange( FEditPropertyChain& PropertyThatChanged );
}

/** optional component for auto-aligning children of this panel */
var(Presentation)							UIComp_AutoAlignment	AutoAlignment;

/* === Unrealscript === */


DefaultProperties
{
	// States
	DefaultStates.Add(class'Engine.UIState_Focused')
}
