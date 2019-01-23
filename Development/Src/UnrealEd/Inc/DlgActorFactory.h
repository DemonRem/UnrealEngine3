/*=============================================================================
	DlgActorFactory.h: UnrealEd General Purpose Actor Factory Dialog
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __DLGACTORFACTORY_H__
#define __DLGACTORFACTORY_H__

/**
  * UnrealEd General Purpose Actor Factory Dialog.
  */
class WxDlgActorFactory : public wxDialog
{
public:
	WxDlgActorFactory();
	~WxDlgActorFactory();

	/** 
	  * Displays the dialog using the passed in factory's properties.
	  *
	  * @param InFactory  Actor factory to use for displaying properties.
	  */
	void ShowDialog( UActorFactory* InFactory );

private:
	UActorFactory* Factory;
	WxPropertyWindow* PropertyWindow;
	wxStaticText *NameText;

	void OnOK( wxCommandEvent& In );

	DECLARE_EVENT_TABLE()
};



#endif

