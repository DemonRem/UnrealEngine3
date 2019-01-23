/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

class wxStaticText;
class wxTextCtrl;
class wxWindow;

/**
 * The toolbar that sits in the Generic Browser.
 */
class WxGBViewToolBar : public wxToolBar
{
public:
	WxGBViewToolBar(wxWindow* InParent, wxWindowID InID);
	
	/**
	 * Sets the text for the NameFilter TextCtrl.
	 *
	 * @param String to set the TextCtrl value to.
	 *
	 */
	void SetNameFilterText( const TCHAR* Str );

private:
	WxMaskedBitmap ListB, PreviewB, ThumbnailB, SphereB, CubeB, CylinderB, PlaneB, SearchB, GroupByClassB;
	WxMaskedBitmap ReimportB;
	wxStaticText* ZoomST;

public:
	WxMaskedBitmap PSysRealTimeB, UsageFilterB;
	wxComboBox* ViewCB;

private:
	wxStaticText* NameFilterST;
	wxTextCtrl* NameFilterTC;
};
