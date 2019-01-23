/*=============================================================================
	UIWidgetTool_WidgetEditor.h: Class implementations for widget editor mode tools
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "EngineUIPrivateClasses.h"

#include "UnObjectEditor.h"
#include "UnUIEditor.h"
#include "ScopedTransaction.h"

#include "UIWidgetTool_WidgetEditor.h"


/*	==================================================================
	FUIWidgetTool_ListEditor
	=============================================================== */
FUIWidgetTool_ListEditor::FUIWidgetTool_ListEditor( WxUIEditorBase* InEditor )
: TUIWidgetTool_WidgetEditor<UUIList>(InEditor)
{
}

/** Called when this tool is 'activated'. */
void FUIWidgetTool_ListEditor::EnterToolMode()
{
	//@todo
	FUIWidgetTool::EnterToolMode();
}

/** Called when this tool is 'deactivated'. */
void FUIWidgetTool_ListEditor::ExitToolMode()
{
	//@todo
	FUIWidgetTool::ExitToolMode();
}

//@todo





// EOF

