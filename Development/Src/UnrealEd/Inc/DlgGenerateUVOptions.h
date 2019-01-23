/*=============================================================================

Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __DLGGENERATEUVOPTIONS_H__
#define __DLGGENERATEUVOPTIONS_H__


class WxDlgGenerateUVOptions : public wxDialog
{
public:
	WxDlgGenerateUVOptions(
		wxWindow* Parent, 
		const TArray<FString>& LODComboOptions, 
		const TArray<INT>& InLODNumTexcoords,
		FLOAT MaxDesiredStretchDefault);

	virtual ~WxDlgGenerateUVOptions();

	int ShowModal();

	UINT ChosenLODIndex;
	UINT ChosenTexIndex;
	FLOAT MaxStretch;

private:
	TArray<INT> LODNumTexcoords;

	void OnOk( wxCommandEvent& In );
	void OnCancel( wxCommandEvent& In );
	void OnChangeLODCombo( wxCommandEvent& In );

	wxButton*		OkButton;
	wxButton*		CancelButton;
	wxComboBox*		LODComboBox;
	wxComboBox*		TexIndexComboBox;
	wxTextCtrl*		MaxStretchEntry;

	DECLARE_EVENT_TABLE()
};

#endif // __DLGGENERATEUVOPTIONS_H__
