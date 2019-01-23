/**
 * This component when present in a widget is supposed add ability to auto align its children widgets in a specified fashion
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UIComp_AutoAlignment extends UIComponent
	within UIObject
	native(UIPrivate)
	HideCategories(Object)
	editinlinenew;
//
/// ** vertical auto alignment orientation setting * /
//enum EUIAutoAlignVertical
//{
//	UIAUTOALIGNV_None,
//	UIAUTOALIGNV_Top,
//	UIAUTOALIGNV_Center,
//	UIAUTOALIGNV_Bottom
//};
//
/// ** auto alignment orientation setting * /
//enum EUIAutoAlignHorizontal
//{
//	UIAUTOALIGNH_None,
//	UIAUTOALIGNH_Left,
//	UIAUTOALIGNH_Center,
//	UIAUTOALIGNH_Right
//};

/**
 * The settings which determines how this component will be aligning children widgets
 */
var			deprecated EUIAlignment		Vertical;
var			deprecated EUIAlignment		Horizontal;

var()		EUIAlignment	HorzAlignment;
var()		EUIAlignment	VertAlignment;

cpptext
{
	/**
	 * Adjusts the child widget's positions according to the specified autoalignment setting
	 *
	 * @param	Face	the face that should be resolved
	 */
	void ResolveFacePosition( EUIWidgetFace Face );

	/**
	 * Called when a property value has been changed in the editor.
	 */
	void PostEditChange( UProperty* PropertyThatChanged );

	/**
	 * Converts the value of the Vertical and Horizontal alignment
	 */
	virtual void Serialize( FArchive& Ar );

protected:
	/**
	 *	Updates the horizontal position of child widgets according to the specified alignment setting
	 *
	 * @param	ContainerWidget			The widget to whose bounds the widgets will be aligned
	 * @param	HorizontalAlignment		The horizontal alignment setting
	 */
	void AlignWidgetsHorizontally( UUIObject* ContainerWidget, EUIAlignment HorizontalAlignment );

	/**
	 *	Updates the vertical position of child widgets according to the specified alignment setting
	 *
	 * @param	ContainerWidget			The widget to whose bounds the widgets will be aligned
	 * @param	VerticalAlignment		The vertical alignment setting
	 */
	void AlignWidgetsVertically( UUIObject* ContainerWidget, EUIAlignment VerticalAlignment );
}


DefaultProperties
{
	HorzAlignment=UIALIGN_Default
	VertAlignment=UIALIGN_Default
}
