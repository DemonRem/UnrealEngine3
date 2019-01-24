/*=============================================================================
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "EngineAIClasses.h"
#include "AITreeEditor.h"
#include "UnAITree.h"

#if WITH_MANAGED_CODE
#include "K2_AITreeEditorShared.h"
#endif

BEGIN_EVENT_TABLE( WxAITreeEditor, WxTrackableFrame )
END_EVENT_TABLE()


WxAITreeEditor::WxAITreeEditor(wxWindow* InParent, wxWindowID InID, UAITree* InTree)
:	WxTrackableFrame( InParent, InID, *LocalizeUnrealEd("AITreeEditor"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT | wxFRAME_NO_TASKBAR )
,	FDockingParent(this)
{
	EditAITree = InTree;

	FWindowUtil::LoadPosSize( GetDockingParentName(), this, 64,64,800,650 );

	wxPanel* MainPanel = new wxPanel( this, -1 );
	wxBoxSizer* SizerV = new wxBoxSizer( wxVERTICAL );
	wxBoxSizer* SizerH = new wxBoxSizer( wxHORIZONTAL );
	{

		// K2 graphical editing control
#if WITH_MANAGED_CODE
		K2Host		  = new WxK2AITree_HostPanel(MainPanel, InTree);
		K2EditPanel   = new WxK2AITree_UtilityPanel(MainPanel, K2Host, InTree);
		K2GatherPanel = new WxK2AITree_GatherPanel(MainPanel, K2Host, InTree);
		
		SizerH->Add( K2Host, 4, wxGROW|wxALL, 0 );
		SizerH->Add( K2GatherPanel, 1, wxGROW|wxALL, 0 );
		SizerV->Add( SizerH, 3, wxGROW|wxALL, 0 );
		SizerV->Add( K2EditPanel, 2, wxGROW|wxALL, 0 );
#endif
	}
	MainPanel->SetSizer(SizerV);


	AddDockingWindow( MainPanel, FDockingParent::DH_None, TEXT("Main") );
	SetDockHostSize(FDockingParent::DH_Right, 400);
}

WxAITreeEditor::~WxAITreeEditor()
{
	FWindowUtil::SavePosSize( GetDockingParentName(), this );

	SaveDockingLayout();
}

const TCHAR* WxAITreeEditor::GetDockingParentName() const
{
	return TEXT("AITree");
}

const INT WxAITreeEditor::GetDockingParentVersion() const
{
	return 1;
}
