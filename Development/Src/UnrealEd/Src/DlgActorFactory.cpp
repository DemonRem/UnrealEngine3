/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "DlgActorFactory.h"
#include "Properties.h"

BEGIN_EVENT_TABLE(WxDlgActorFactory, wxDialog)
	EVT_BUTTON( wxID_OK, WxDlgActorFactory::OnOK )
END_EVENT_TABLE()

WxDlgActorFactory::WxDlgActorFactory()
{
	const bool bSuccess = wxXmlResource::Get()->LoadDialog( this, GApp->EditorFrame, TEXT("ID_DLG_ACTOR_FACTORY") );
	check( bSuccess );

	NameText = (wxStaticText*)FindWindow( XRCID( "IDEC_NAME" ) );
	check( NameText != NULL );

	// Replace the placeholder window with a property window

	PropertyWindow = new WxPropertyWindow;
	PropertyWindow->Create( this, GUnrealEd );
	wxWindow* win = (wxWindow*)FindWindow( XRCID( "ID_PROPERTY_WINDOW" ) );
	check( win != NULL );
	const wxRect rc = win->GetRect();
	PropertyWindow->SetSize( rc );
	win->Show(0);

	Factory = NULL;

	FWindowUtil::LoadPosSize( TEXT("DlgActorFactory"), this );
	FLocalizeWindow( this );
}

WxDlgActorFactory::~WxDlgActorFactory()
{
	FWindowUtil::SavePosSize( TEXT("DlgActorFactory"), this );
}

void WxDlgActorFactory::ShowDialog( UActorFactory* InFactory )
{
	Factory = InFactory;
	PropertyWindow->SetObject( Factory, 1,1,0 );
	NameText->SetLabel( *(Factory->MenuName) );

	wxDialog::Show( TRUE );
}

void WxDlgActorFactory::OnOK( wxCommandEvent& In )
{
	// Provide a default error value, in case the actor factor doesn't specify one.
	FString ActorErrorMsg( TEXT("Error_CouldNotCreateActor") );

	if( Factory->CanCreateActor( ActorErrorMsg ) )
	{
		GEditor->UseActorFactory( Factory );
	}
	else
	{
		appMsgf( AMT_OK, *LocalizeUnrealEd( *ActorErrorMsg ) );
	}

	wxDialog::OnOK( In );
}
