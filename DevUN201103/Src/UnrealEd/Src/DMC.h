/*=============================================================================
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __DMC_H__
#define __DMC_H__

class WxDMCToolMenuBar;
class WxK2HostPanel;

#include "K2OwnerInterface.h"

class WxDMC : public WxTrackableFrame, public FDockingParent, public FK2OwnerInterface
{
	/** Pointer to DMC_Prototype object being edited */
	UDMC_Prototype*			EditProto;
	
	/** Property window */
	//WxPropertyWindowHost*	ProtoProps;

	/** Property window */
	WxPropertyWindowHost*	DefObjProps;

	/** Showing class defaults */
	UBOOL					bShowingClassDefs;

	/** Text box for entering code */
	WxTextCtrl*				TextBox;

	/** Menu bar */
	WxDMCToolMenuBar*		MenuBar;

	/** */
#if WITH_MANAGED_CODE
	/** Content Browser */
	WxK2HostPanel*			K2Host;
#endif

public:
	WxDMC(wxWindow* InParent, wxWindowID InID, UDMC_Prototype* InProto);
	virtual ~WxDMC();

	/** This will create a new class using the supplied DMC_Prototype object. New class will have 'GEN' in the title */
	static UClass* GenerateClassFromDMCProto(UDMC_Prototype* InProto, const FString& ExtraPropText);

	static void ReplaceAllActorsOfClass(UClass* OldClass, UClass* NewClass);

	//// FK2OwnerInterface
	virtual void OnSelectionChanged(TArray<UK2NodeBase*> NewSelectedNodes);
	////

protected:
	virtual const TCHAR* GetDockingParentName() const;
	virtual const INT GetDockingParentVersion() const;
private:

	void UpdateDMCFromTool();

	/** Generate code text from the K2 graph */
	UBOOL GenerateCode();

	/** Regenerate the UClass, based on the DMC_Prototype */
	UBOOL RecompileClass(const FString& ExtraPropText);

	/** Add a new variable of the supplied type to the DMC */
	void AddNewVar(FName VarType);

	/** Add a new component of the supplied class name to the DMC, and include the supplied text into its default properties */
	void AddNewComponent(const FString& CompClassName, const FString& CompDefProps);

	// Event handlers

	void OnRecompile( wxCommandEvent& In );

	void OnAddBoolVar( wxCommandEvent& In );
	void OnAddIntVar( wxCommandEvent& In );
	void OnAddFloatVar( wxCommandEvent& In );
	void OnAddVectorVar( wxCommandEvent& In );
	void OnAddCompVar( wxCommandEvent& In );

	void OnRemoveVar( wxCommandEvent& In );


	DECLARE_EVENT_TABLE()
};

#endif
