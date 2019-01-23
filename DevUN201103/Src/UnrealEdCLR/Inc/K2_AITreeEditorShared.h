/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

#ifndef __K2_AITREEEDITOR_SHARED_H__
#define __K2_AITREEEDITOR_SHARED_H__

#include "InteropShared.h"

#ifdef __cplusplus_cli

ref class MK2AITreePanel;
ref class MK2AITreeUtilityPanel;
ref class MK2AITreeGatherPanel;

#endif // __cplusplus_cli

/** Wrapper for K2 AITree tree view editor window */
class WxK2AITree_HostPanel : public wxPanel
{
public:
	/** Constructor */
	WxK2AITree_HostPanel(wxWindow* Parent, UAITree* InTree);

	/** Destructor */
	virtual ~WxK2AITree_HostPanel();

protected:

	/** Managed reference to the actual editor */
	GCRoot( MK2AITreePanel^ ) K2Panel;

	class WxK2AITree_UtilityPanel*	UtilityPanel;
	class WxK2AITree_GatherPanel*	GatherPanel;


	/** Called when the browser window is resized */
	void OnSize( wxSizeEvent& In );

	/** Called when the browser window receives focus */
	void OnReceiveFocus( wxFocusEvent& Event );

	DECLARE_EVENT_TABLE();

	friend class WxK2AITree_UtilityPanel;
	friend class WxK2AITree_GatherPanel;
};

/** Wrapper for K2 AITree utility editor window */
class WxK2AITree_UtilityPanel : public wxPanel
{
public:
	/** Constructor */
	WxK2AITree_UtilityPanel( wxWindow* Parent, WxK2AITree_HostPanel* InHostPanel, UAITree* InTree );
	/** Destructor */
	virtual ~WxK2AITree_UtilityPanel();

protected:
	WxK2AITree_HostPanel* HostPanel;
	GCRoot( MK2AITreeUtilityPanel^ ) K2UtilityPanel;

	/** Called when the browser window is resized */
	void OnSize( wxSizeEvent& In );

	/** Called when the browser window receives focus */
	void OnReceiveFocus( wxFocusEvent& Event );

	DECLARE_EVENT_TABLE();
};

/** Wrapper for K2 AITree gather editor window */
class WxK2AITree_GatherPanel : public wxPanel
{
public:
	/** Constructor */
	WxK2AITree_GatherPanel( wxWindow* Parent, WxK2AITree_HostPanel* InHostPanel, UAITree* InTree );
	/** Destructor */
	virtual ~WxK2AITree_GatherPanel();

protected:
	WxK2AITree_HostPanel* HostPanel;
	GCRoot( MK2AITreeGatherPanel^ ) K2GatherPanel;

	/** Called when the browser window is resized */
	void OnSize( wxSizeEvent& In );

	/** Called when the browser window receives focus */
	void OnReceiveFocus( wxFocusEvent& Event );

	DECLARE_EVENT_TABLE();
};

#endif // #define __K2_AITREEEDITOR_SHARED_H__
