/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Modified version of numeric edit box that has some styles replaced.
 */
class UTUINumericEditBox extends UINumericEditBox
	native(UI);

cpptext
{
	/**
	 * Initializes the buttons and creates the background image.
	 *
	 * @param	inOwnerScene	the scene to add this widget to.
	 * @param	inOwner			the container widget that will contain this widget.  Will be NULL if the widget
	 *							is being added to the scene's list of children.
	 */
	virtual void Initialize( UUIScene* inOwnerScene, UUIObject* inOwner=NULL );

}

defaultproperties
{
	Begin Object class=UIComp_DrawImage Name=UTNumericEditboxBackgroundTemplate
		ImageStyle=(DefaultStyleTag="DefaultEditboxImageStyle",RequiredStyleClass=class'Engine.UIStyle_Image')
		StyleResolverTag="Background Image Style"
	End Object
	BackgroundImageComponent=UTNumericEditboxBackgroundTemplate

	// Increment and Decrement Button Styles
	IncrementStyle=(DefaultStyleTag="SpinnerIncrementButtonBackground",RequiredStyleClass=class'Engine.UIStyle_Image')
	DecrementStyle=(DefaultStyleTag="SpinnerDecrementButtonBackground",RequiredStyleClass=class'Engine.UIStyle_Image')
}