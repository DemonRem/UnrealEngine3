/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

#ifndef __K2_EDITOR_SHARED_H__
#define __K2_EDITOR_SHARED_H__

#include "InteropShared.h"

#ifdef __cplusplus_cli

ref class MK2Panel;

#endif // __cplusplus_cli

/** Wrapper for K2 editor window */
class WxK2HostPanel : public wxPanel
{
public:
	/** Constructor */
	WxK2HostPanel(wxWindow* Parent, UDMC_Prototype* DMC, class FK2OwnerInterface* OwnerInt);

	/** Destructor */
	virtual ~WxK2HostPanel();

protected:

	/** Managed reference to the actual editor */
	GCRoot( MK2Panel^ ) K2Panel;


	/** Called when the browser window is resized */
	void OnSize( wxSizeEvent& In );

	/** Called when the browser window receives focus */
	void OnReceiveFocus( wxFocusEvent& Event );

	DECLARE_EVENT_TABLE();
};

#endif // #define __K2_EDITOR_SHARED_H__
