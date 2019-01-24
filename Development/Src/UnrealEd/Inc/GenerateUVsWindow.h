/*=============================================================================
Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __GenerateUVsWindow_h__
#define __GenerateUVsWindow_h__

class WxStaticMeshEditor;


class WxGenerateUVsWindow
	: public wxPanel
{
public:
	WxGenerateUVsWindow( WxStaticMeshEditor* Parent );

	virtual ~WxGenerateUVsWindow();


	UINT ChosenLODIndex;
	UINT ChosenTexIndex;
	FLOAT MinChartSpacingPercent;
	FLOAT BorderSpacingPercent;
	UINT MaxCharts;
	FLOAT MaxStretch;
	UBOOL bKeepExistingUVs;

	/** True if we should generate charts based on MaxStretch; otherwise, MaxCharts should be used */
	UBOOL bUseMaxStretch;


	/** Called by the parent Static Mesh Editor when the currently-edited mesh or LOD has changed */
	void OnStaticMeshChanged();


private:
	TArray<INT> LODNumTexcoords;

	void OnApply( wxCommandEvent& In );
	void OnChangeLODCombo( wxCommandEvent& In );
	void OnChangeChartMethodRadio( wxCommandEvent& In );

	/** Sets the static mesh that we're generating UVs for */
	void SetStaticMesh( UStaticMesh* InStaticMesh );


	/**
	 * Updates the state of controls within this window
	 */
	void UpdateControlStates();

	wxButton*		ApplyButton;
	wxButton*		CancelButton;
	wxComboBox*		LODComboBox;
	wxComboBox*		TexIndexComboBox;
	wxRadioButton*	MaxChartsRadio;
	wxTextCtrl*		MaxChartsEntry;
	wxRadioButton*	MaxStretchRadio;
	wxTextCtrl*		MaxStretchEntry;
	wxTextCtrl*		MinChartSpacingPercentEntry;
	wxTextCtrl*		BorderSpacingPercentEntry;
	wxCheckBox*		KeepExistingUVCheckBox;
	wxStaticText*	ResultText;

	/** Static Mesh Editor that owns us */
	WxStaticMeshEditor* StaticMeshEditor;

	/** Static mesh that we're operating on (not serialized -- owner class should already retain a reference.) */
	UStaticMesh* StaticMesh;

	DECLARE_EVENT_TABLE()
};

#endif // __GenerateUVsWindow_h__
