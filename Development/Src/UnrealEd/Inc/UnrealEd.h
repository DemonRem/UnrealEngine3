/*=============================================================================
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef _INC_UNREALED
#define _INC_UNREALED

#ifdef _INC_CORE
#error UnrealEd.h needs to be the first include to ensure the order of wxWindows/ Windows header inclusion
#endif

class WxExpressionBase;

// Used for fazing out support for generic browser as we transition to content browser
#define GB_SUPPORTED 1

/**
 * Function for getting the editor frame window without knowing about it
 */
class wxWindow* GetEditorFrame(void);

/*-----------------------------------------------------------------------------
	Includes.
-----------------------------------------------------------------------------*/

#define LEFT_BUTTON_BAR_SZ			(WxButtonGroup::BUTTON_SZ*2)

/*-----------------------------------------------------------------------------
	Option proxies.
-----------------------------------------------------------------------------*/

enum
{
	OPTIONS_2DSHAPERSHEET,
	OPTIONS_2DSHAPEREXTRUDE,
	OPTIONS_2DSHAPEREXTRUDETOPOINT,
	OPTIONS_2DSHAPEREXTRUDETOBEVEL,
	OPTIONS_2DSHAPERREVOLVE,
	OPTIONS_2DSHAPERBEZIERDETAIL,
	OPTIONS_DUPOBJECT,
	OPTIONS_NEWCLASSFROMSEL,
};

#define CREATE_OPTION_PROXY( ID, Class )\
{\
	GApp->EditorFrame->OptionProxies->Set( ID,\
	CastChecked<UOptionsProxy>(GUnrealEd->StaticConstructObject(Class::StaticClass(),UObject::GetTransientPackage(),NAME_None,RF_Public|RF_Standalone) ) );\
	UOptionsProxy* Proxy = *GApp->EditorFrame->OptionProxies->Find( ID );\
	check(Proxy);\
	Proxy->InitFields();\
}

// Engine
#include "XWindow.h"

#include "Engine.h"

struct FBuilderPoly
{
	TArray<INT> VertexIndices;
	INT Direction;
	FName ItemName;
	INT PolyFlags;
	FBuilderPoly()
	: VertexIndices(), Direction(0), ItemName(NAME_None)
	{}
};

#include "Bitmaps.h"
#include "Controls.h"
#include "Buttons.h"

#include "UnInterpolationHitProxy.h"

#include "DockingParent.h"
#include "Docking.h"

#include "EngineUserInterfaceClasses.h"
#include "UnEdComponents.h"
#include "UnEdViewport.h"

#include "UnrealEdClasses.h"

#include "Editor.h"
#include "ResourceIDs.h"
#include "FEdObjectPropagator.h"

#include "MRUList.h"

enum {
	GI_NUM_SELECTED			= 1,
	GI_CLASSNAME_SELECTED	= 2,
	GI_NUM_SURF_SELECTED	= 4,
	GI_CLASS_SELECTED		= 8
};

typedef struct
{
	INT iValue;
	FString String;
	UClass*	pClass;
} FGetInfoRet;

class FListViewSortOptions
{
public:
	FListViewSortOptions()
	{
		Column = 0;
		bSortAscending = 1;
	}
	~FListViewSortOptions() {}

	/** The column currently being used for sorting. */
	int Column;

	/** Denotes ascending/descending sort order. */
	UBOOL bSortAscending;
};

/*-----------------------------------------------------------------------------
	Helper structure for Bitmap Buttons
-----------------------------------------------------------------------------*/
struct WxButtonCollection
{
	WxBitmapButton*		Button;
	WxBitmap*			Inactive;
	WxBitmap*			Active;
};


// unrealed stuff

#include "..\..\core\inc\unmsg.h"
#include "..\src\GeomFitUtils.h"

extern class UUnrealEdEngine* GUnrealEd;

#include "GenericDlgOptions.h"
#include "UnrealEdMisc.h"
#include "ButtonBar.h"
#include "Utils.h"
#include "UnEdModeInterpolation.h"

#include "EngineParticleClasses.h"
#include "UnrealEdCascadeClasses.h"

#include "DlgNewGeneric.h"
#include "FileDialog.h"

#include "UnrealEdEngine.h"
#include "Dialogs.h"
#include "Viewports.h"
#include "StatusBars.h"
#include "FCallbackDeviceEditor.h"
#include "EditorFrame.h"
#include "..\..\Launch\Inc\LaunchApp.h"
#include "UnrealEdApp.h"
#include "Wizards.h"
#include "SpeedTreeEditor.h"

#include "LensFlareEditor.h"

extern WxUnrealEdApp* GApp;

#endif // _INC_UNREALED
