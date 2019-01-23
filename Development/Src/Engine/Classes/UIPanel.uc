/**
 * Base class for all containers which need a background image.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UIPanel extends UIContainer
	placeable
	native(UIPrivate);

/** Component for rendering the background image */
var(Image)	editinline	const	UIComp_DrawImage		BackgroundImageComponent;


/** The background image for this panel */
var deprecated	UITexture				PanelBackground;
/**
 * the texture atlas coordinates for the button background. Values of 0 indicate that
 * the texture is not part of an atlas.
 */
var deprecated	TextureCoordinates		Coordinates;

cpptext
{
	/* === UIPanel interface === */
	/**
	 * Changes the background image for this button, creating the wrapper UITexture if necessary.
	 *
	 * @param	NewImage		the new surface to use for this UIImage
	 */
	virtual void SetBackgroundImage( class USurface* NewImage );

	/* === UIObject interface === */
	/**
	 * Provides a way for widgets to fill their style subscribers array prior to performing any other initialization tasks.
	 *
	 * This version adds the BackgroundImageComponent (if non-NULL) to the StyleSubscribers array.
	 */
	virtual void InitializeStyleSubscribers();

	/* === UUIScreenObject interface === */
	/**
	 * Render this button.
	 *
	 * @param	Canvas	the canvas to use for rendering this widget
	 */
	virtual void Render_Widget( FCanvas* Canvas );

	/* === UObject interface === */
	/**
	 * Called when a property value from a member struct or array has been changed in the editor, but before the value has actually been modified.
	 */
	virtual void PreEditChange( FEditPropertyChain& PropertyThatChanged );

	/**
	 * Called when a property value from a member struct or array has been changed in the editor.
	 */
	virtual void PostEditChange( FEditPropertyChain& PropertyThatChanged );

	/**
	 * Called after this object has been completely de-serialized.  This version migrates values for the deprecated PanelBackground,
	 * Coordinates, and PrimaryStyle properties over to the BackgroundImageComponent.
	 */
	virtual void PostLoad();
}

/* === Unrealscript === */
/**
 * Changes the background image for this panel, creating the wrapper UITexture if necessary.
 *
 * @param	NewImage		the new surface to use for this UIImage
 */
final function SetBackgroundImage( Surface NewImage )
{
	if ( BackgroundImageComponent != None )
	{
		BackgroundImageComponent.SetImage(NewImage);
	}
}

DefaultProperties
{
	PrimaryStyle=(DefaultStyleTag="PanelBackground",RequiredStyleClass=class'Engine.UIStyle_Image')
	bSupportsPrimaryStyle=false

	Begin Object class=UIComp_DrawImage Name=PanelBackgroundTemplate
		ImageStyle=(DefaultStyleTag="PanelBackground",RequiredStyleClass=class'Engine.UIStyle_Image')
		StyleResolverTag="Panel Background Style"
	End Object
	BackgroundImageComponent=PanelBackgroundTemplate
}
