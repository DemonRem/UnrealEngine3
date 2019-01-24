/*=============================================================================
	AITreeEditor.h: AITree editing
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __AITREEEDITOR_H__
#define __AITREEEDITOR_H__


class WxK2AITree_HostPanel;


#include "UnrealEd.h"
#include "UnAITree.h"


class WxAITreeEditor : public WxTrackableFrame, public FDockingParent
{
	/** Pointer to AITree object being edited */
	UAITree*				EditAITree;

	/** K-based control for editing graph of nodes */
	class WxK2AITree_HostPanel*		K2Host;
	class WxK2AITree_UtilityPanel*	K2EditPanel;
	class WxK2AITree_GatherPanel*	K2GatherPanel;

public:
	WxAITreeEditor(wxWindow* InParent, wxWindowID InID, UAITree* InTree);
	virtual ~WxAITreeEditor();
	
protected:
	virtual const TCHAR* GetDockingParentName() const;
	virtual const INT GetDockingParentVersion() const;

private:
	DECLARE_EVENT_TABLE()
};

#endif
	