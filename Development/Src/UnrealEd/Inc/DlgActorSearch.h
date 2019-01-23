/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __DLGACTORSEARCH_H__
#define __DLGACTORSEARCH_H__

class WxDlgActorSearch : public wxDialog, public FSerializableObject
{
public:
	WxDlgActorSearch( wxWindow* InParent );
	virtual ~WxDlgActorSearch();

	/**
	 * Empties the search list, releases actor references, etc.
	 */
	void Clear();

	/** Updates the results list using the current filter and search options set. */
	void UpdateResults();

	/** Serializes object references to the specified archive. */
	virtual void Serialize(FArchive& Ar);

	class FActorSearchOptions
	{
	public:
		FActorSearchOptions()
			: Column( 0 )
			, bSortAscending( TRUE )
		{}
		/** The column currently being used for sorting. */
		int Column;
		/** Denotes ascending/descending sort order. */
		UBOOL bSortAscending;
	};

protected:
	wxTextCtrl *SearchForEdit;
	wxRadioButton *StartsWithRadio;
	wxComboBox *InsideOfCombo;
	wxListCtrl *ResultsList;
	wxStaticText *ResultsLabel;

	TArray<AActor*> ReferencedActors;

	FActorSearchOptions SearchOptions;

	/** Wx Event Handlers */
	void OnSearchTextChanged( wxCommandEvent& In );
	void OnColumnClicked( wxListEvent& In );
	void OnItemActivated( wxListEvent& In );
	void OnGoTo( wxCommandEvent& In );
	void OnDelete( wxCommandEvent& In );
	void OnProperties( wxCommandEvent& In );

	/** Refreshes the results list when the window gets focus. */
	void OnActivate( wxActivateEvent& In);
};

#endif // __DLGACTORSEARCH_H__
