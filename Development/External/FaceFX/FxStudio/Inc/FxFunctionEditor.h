//------------------------------------------------------------------------------
// A little window to edit functions
//
// Owner: John Briggs
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#ifndef FxFunctionEditor_H__
#define FxFunctionEditor_H__

#include "FxPlatform.h"
#include "FxArray.h"
#include "FxLinkFn.h"

namespace OC3Ent
{

namespace Face
{

class FxFunctionEditor : public wxWindow
{
	typedef wxWindow Super;
	WX_DECLARE_DYNAMIC_CLASS( FxFunctionEditor )
	DECLARE_EVENT_TABLE()

public:
	FxFunctionEditor( wxWindow* parent = NULL, wxWindowID id = -1, wxPoint position = wxDefaultPosition, wxSize size = wxDefaultSize );
	~FxFunctionEditor();

	// Sets the link function to work on.
	void SetLinkFunction( const FxLinkFn* function, const FxLinkFnParameters& parameters );
	// Sets the domain and range to plot over
	void SetDomainRange( FxReal xMin, FxReal xMax, FxReal yMin, FxReal yMax );
	// Sets the input and output nodes
	void SetNodeNames( FxName& inputNode, FxName& outputNode );
	// Returns a custom link function, if the user has drawn one.
	FxAnimCurve* GetCustomCurve();

protected:
	// Called to draw the control
	void OnPaint( wxPaintEvent& event );

	// Called on left mouse button down
	void OnLeftDown( wxMouseEvent& event );
	// Called on mouse move
	void OnMouseMove( wxMouseEvent& event );
	// Called on left mouse button up
	void OnLeftUp( wxMouseEvent& event );

	// Called when the window resizes
	void OnSize( wxSizeEvent& event );

	// Called to add a key
	void OnAddKey( wxCommandEvent& event );
	// Called to delete the selection
	void OnDeleteSelection( wxCommandEvent& event );

	// Checks which key, if any, the user clicked on
	FxSize HitTestKeys( wxPoint clickLocation );

	// Returns the point of the incoming tangent handle
	wxPoint GetIncTanHandle( FxSize keyIndex, FxInt32 keyX, FxInt32 keyY );
	// Returns the point of the outgoing tangent handle
	wxPoint GetOutTanHandle( FxSize keyIndex, FxInt32 keyX, FxInt32 keyY );

	// The cached client rectangle
	wxRect _clientRect;
	// The rect of the drawn plot
	wxRect _plotRect;

	// The link function
	const FxLinkFn* _linkFunction;
	FxLinkFnParameters _linkFnParams;

	enum MouseAction
	{
		MOUSE_NONE,
		MOUSE_ADDKEY,
		MOUSE_MOVEKEY,
		MOUSE_MOVEINCHANDLE,
		MOUSE_MOVEOUTHANDLE,
		MOUSE_DRAGSELECTION
	};
	MouseAction _mouseAction;

	// The domain and range
	FxReal _xMin;
	FxReal _xMax;
	FxReal _yMin;
	FxReal _yMax;

	// The selected key
	FxSize _selKey;

	// The node names
	FxName _inputNode;
	FxName _outputNode;
};

} // namespace Face

} // namespace OC3Ent

#endif