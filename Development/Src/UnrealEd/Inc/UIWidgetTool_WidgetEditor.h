/*=============================================================================
	UIWidgetTool_WidgetEditor.h: Class declarations for widget editor mode tools
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __UIWIDGETTOOL_WIDGETEDITOR_H__
#define __UIWIDGETTOOL_WIDGETEDITOR_H__

// include the declarations of the base class
#include "UIWidgetTool.h"

/**
 * Base class for all widget custom editor mode tools.  T must be a class derived from UIScreenObject.
 */
template< class T >
class TUIWidgetTool_WidgetEditor : public FUIWidgetTool, public FSerializableObject
{
public:
	/* === FUIWidgetTool_WidgetEditor interface === */
	/**
	 * Constructor
	 *
	 * @param	InEditor	the UI editor window that contains our widget
	 */
	TUIWidgetTool_WidgetEditor( class WxUIEditorBase* InEditor )
	: FUIWidgetTool(InEditor), FSerializableObject(), CurrentWidget(NULL)
	{
		// we shouldn't be able to invoke the widget editor tool if we don't have exactly one widget selected
		check(InEditor->GetNumSelected() == 1);
		
		TArray<UUIObject*> SelectedWidgets;
		InEditor->GetSelectedWidgets(SelectedWidgets);

		// this widget editor tool shouldn't be invoked if the widget wasn't of the correct type.
		CurrentWidget = CastChecked<T>(SelectedWidgets(0));
	}

	/* === FSerializableObject interface === */
	/**
	 * Method for serializing UObject references that must be kept around through GC's.
	 * 
	 * @param Ar The archive to serialize with
	 */
	virtual void Serialize(FArchive& Ar)
	{
		Ar << (UObject*&)CurrentWidget;
	}

protected:
	/**
	 * The widget being edited by this widget editor mode tool.
	 */
	T* CurrentWidget;
};

/**
 * Custom tool mode editor for UIList widgets.
 */
class FUIWidgetTool_ListEditor : public TUIWidgetTool_WidgetEditor<UUIList>
{
public:
	/* === FUIWidgetTool_ListEditor interface === */
	/**
	 * Constructor
	 *
	 * @param	InEditor	the UI editor window that contains our widget
	 */
	FUIWidgetTool_ListEditor( class WxUIEditorBase* InEditor );

	/* === FUIWidgetTool_WidgetEditor interface === */
	/** Called when this tool is 'activated'. */
	virtual void EnterToolMode();

	/** Called when this tool is 'deactivated'. */
	virtual void ExitToolMode();
	//@todo
};

#endif

