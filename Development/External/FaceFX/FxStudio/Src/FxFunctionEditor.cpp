//------------------------------------------------------------------------------
// A little window to edit functions
//
// Owner: John Briggs
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "stdwx.h"
#include "FxFunctionEditor.h"
#include "FxFunctionPlotter.h"
#include "FxVec2.h"

namespace OC3Ent
{

namespace Face
{

WX_IMPLEMENT_DYNAMIC_CLASS( FxFunctionEditor, wxWindow )

BEGIN_EVENT_TABLE( FxFunctionEditor, wxWindow )
	EVT_PAINT( FxFunctionEditor::OnPaint )

	EVT_LEFT_DOWN( FxFunctionEditor::OnLeftDown )
	EVT_MOTION( FxFunctionEditor::OnMouseMove )
	EVT_LEFT_UP( FxFunctionEditor::OnLeftUp )

	EVT_SIZE( FxFunctionEditor::OnSize )

	EVT_MENU( MenuID_FunctionEditorAddKey, FxFunctionEditor::OnAddKey )
	EVT_MENU( MenuID_FunctionEditorDeleteSelection, FxFunctionEditor::OnDeleteSelection )
END_EVENT_TABLE()

FxFunctionEditor::FxFunctionEditor( wxWindow* parent, wxWindowID id, wxPoint position, wxSize size )
	: Super( parent, id, position, size, wxSTATIC_BORDER|wxFULL_REPAINT_ON_RESIZE )
	, _linkFunction( NULL )
	, _xMin( -1 )
	, _xMax( 1 )
	, _yMin( -1 )
	, _yMax( 1 )
	, _selKey( FxInvalidIndex )
	, _mouseAction( MOUSE_NONE )
{
	static const int numEntries = 4;
	wxAcceleratorEntry entries[numEntries];
	entries[0].Set( wxACCEL_CTRL, (int)'R', MenuID_FunctionEditorResetView );
	entries[1].Set( wxACCEL_CTRL, (int)'A', MenuID_FunctionEditorSelectAll );
	entries[2].Set( wxACCEL_NORMAL, WXK_DELETE, MenuID_FunctionEditorDeleteSelection );
	entries[3].Set( wxACCEL_NORMAL, WXK_INSERT, MenuID_FunctionEditorAddKey );
	// Note to self: when adding an accelerator entry, update numEntries. It works better.
	wxAcceleratorTable accelerator( numEntries, entries );
	SetAcceleratorTable( accelerator );
}

FxFunctionEditor::~FxFunctionEditor()
{
}

// Sets the link function to work on.
void FxFunctionEditor::SetLinkFunction( const FxLinkFn* function, const FxLinkFnParameters& parameters )
{
	_linkFunction = function;
	_linkFnParams = parameters;
	if( _linkFunction && !_linkFnParams.pCustomCurve )
	{
		_linkFnParams.pCustomCurve = new FxAnimCurve();
	}
}

// Sets the domain and range to plot over
void FxFunctionEditor::SetDomainRange( FxReal xMin, FxReal xMax, 
									    FxReal yMin, FxReal yMax )
{
	_xMin = xMin;
	_xMax = xMax;
	_yMin = yMin;
	_yMax = yMax;
}

// Sets the input and output nodes
void FxFunctionEditor::SetNodeNames( FxName& inputNode, FxName& outputNode )
{
	_inputNode = inputNode;
	_outputNode = outputNode;
}

// Returns a custom link function, if the user has drawn one.
FxAnimCurve* FxFunctionEditor::GetCustomCurve()
{
	if( _linkFunction )
	{
		if( !_linkFunction->IsA( FxCustomLinkFn::StaticClass() ) ||
			(_linkFnParams.pCustomCurve && 
			_linkFnParams.pCustomCurve->GetNumKeys() == 0 ) )
		{
			delete _linkFnParams.pCustomCurve;
			_linkFnParams.pCustomCurve = NULL;
		}
		FxAnimCurve* toReturn = _linkFnParams.pCustomCurve;
		_linkFnParams.pCustomCurve = NULL;
		return toReturn;
	}
	else
	{
		_linkFnParams.pCustomCurve = NULL;
		return NULL;
	}
}

// Called to draw the control
void FxFunctionEditor::OnPaint( wxPaintEvent& FxUnused(event) )
{
	wxPaintDC front( this );

	wxBrush backgroundBrush( wxColour(255, 255, 255), wxSOLID );

	// Set up the backbuffer
	wxMemoryDC dc;
	wxBitmap backbuffer( front.GetSize().GetWidth(), front.GetSize().GetHeight() );
	dc.SelectObject( backbuffer );

	dc.BeginDrawing();

	if( _linkFunction )
	{
		wxString input( _inputNode.GetAsString().GetData(), wxConvLibc );
		wxString output(_outputNode.GetAsString().GetData(), wxConvLibc );
		wxBitmap plot = FxFunctionPlotter::Plot( _linkFunction, _linkFnParams, 
			_clientRect.GetSize(), _xMin, _xMax, wxColour(192,192,192), _yMin, _yMax, 
			FxTrue, wxColour(170,170,170), *wxBLACK, *wxWHITE, output, input, &_plotRect );
		wxMemoryDC bitmapDC;
		bitmapDC.SelectObject( plot );
		dc.Blit( dc.GetLogicalOrigin(), bitmapDC.GetSize(), &bitmapDC, bitmapDC.GetLogicalOrigin() );

		if( _linkFunction->IsA( FxCustomLinkFn::StaticClass() ) &&
			_linkFnParams.pCustomCurve )
		{
			// Draw the keys
			wxBrush normalKeyBrush( *wxWHITE_BRUSH );
			wxBrush selectedKeyBrush( *wxRED_BRUSH );
			FxAnimCurve* pCurve = _linkFnParams.pCustomCurve;
			for( FxSize i = 0; i < pCurve->GetNumKeys(); ++i )
			{
				const FxAnimKey& key = pCurve->GetKey( i );
				FxInt32 x = FxFunctionPlotter::TimeToCoord( key.GetTime(), _plotRect, _xMin, _xMax );
				FxInt32 y = FxFunctionPlotter::ValueToCoord( key.GetValue(), _plotRect, _yMin, _yMax );
				
				// Select the brush.
				dc.SetBrush( i==_selKey ? selectedKeyBrush : normalKeyBrush );
				if( i == _selKey )
				{
					// Draw the tangent handles.
					wxPoint incTan = GetIncTanHandle(i, x, y);
					wxPoint outTan = GetOutTanHandle(i, x, y);
					dc.DrawLine(x,y, incTan.x, incTan.y);
					dc.DrawLine(x,y, outTan.x, outTan.y);
					dc.DrawCircle(incTan, 2);
					dc.DrawCircle(outTan, 2);
				}

				// Draw the key
				dc.DrawCircle( x, y, 3 );
				
			}
		}
	}
	else
	{
		dc.SetBrush( wxBrush( wxColour(192, 192, 192), wxSOLID ) );
		dc.DrawRectangle( _clientRect );
	}

	dc.EndDrawing();

	// Swap to the front buffer
	front.Blit( _clientRect.GetPosition(), _clientRect.GetSize(), &dc, dc.GetLogicalOrigin(), wxCOPY );
}

// Called on left mouse button down
void FxFunctionEditor::OnLeftDown( wxMouseEvent& event )
{
	SetFocus();
	FxReal value = FxFunctionPlotter::CoordToValue( event.GetY(), _plotRect, _yMin, _yMax );
	FxReal time  = FxFunctionPlotter::CoordToTime( event.GetX(), _plotRect, _xMin, _xMax );

	CaptureMouse();
	if( _linkFunction && _linkFunction->IsA( FxCustomLinkFn::StaticClass() ) &&
		_linkFnParams.pCustomCurve )
	{
		if( _mouseAction == MOUSE_NONE )
		{
			// If we have a key selected, we first need to hit test the two 
			// tangent handles.  If neither hit test succeeds, we can fall through
			// to the default behaviour.
			if( _selKey != FxInvalidIndex )
			{
				FxReal keyTime  = _linkFnParams.pCustomCurve->GetKey(_selKey).GetTime();
				FxReal keyValue = _linkFnParams.pCustomCurve->GetKey(_selKey).GetValue();
				FxInt32 keyX = FxFunctionPlotter::TimeToCoord(keyTime, _plotRect, _xMin, _xMax);
				FxInt32 keyY = FxFunctionPlotter::ValueToCoord(keyValue, _plotRect, _yMin, _yMax);
				wxPoint incTanHandle = GetIncTanHandle(_selKey, keyX, keyY);
				wxPoint outTanHandle = GetOutTanHandle(_selKey, keyX, keyY);
				wxRect incTanHandleRect(incTanHandle, wxSize(5,5));
				incTanHandleRect.Offset(-2,-2);
				wxRect outTanHandleRect(outTanHandle, wxSize(5,5));
				outTanHandleRect.Offset(-2,-2);
				if( incTanHandleRect.Inside(event.GetPosition()) )
				{
					_mouseAction = MOUSE_MOVEINCHANDLE;
					return;
				}
				else if( outTanHandleRect.Inside(event.GetPosition()) )
				{
					_mouseAction = MOUSE_MOVEOUTHANDLE;
					return;
				}
			}

			_selKey = HitTestKeys( event.GetPosition() );
			if( _selKey != FxInvalidIndex )
			{
				_mouseAction = MOUSE_MOVEKEY;
			}
		}
		else if( _mouseAction == MOUSE_ADDKEY )
		{
			_mouseAction = MOUSE_MOVEKEY;
			SetCursor( *wxSTANDARD_CURSOR );
			_selKey = _linkFnParams.pCustomCurve->InsertKey( FxAnimKey( time, value, 0.0f, 0.0f ) );
		}
		Refresh( FALSE );
	}
}

// Called on mouse move
void FxFunctionEditor::OnMouseMove( wxMouseEvent& event )
{
	if( _linkFunction && _linkFunction->IsA( FxCustomLinkFn::StaticClass() ) &&
		_linkFnParams.pCustomCurve && _selKey != FxInvalidIndex )
	{
		FxAnimCurve* pCurve = _linkFnParams.pCustomCurve;
		FxReal mouseTime = FxFunctionPlotter::CoordToTime( event.GetX(), _plotRect, _xMin, _xMax );
		FxReal mouseValue = FxFunctionPlotter::CoordToValue( event.GetY(), _plotRect, _yMin, _yMax );
		if( _mouseAction == MOUSE_MOVEKEY )
		{
			mouseTime = FxClamp( _xMin, mouseTime, _xMax );
			mouseValue = FxClamp( _yMin, mouseValue, _yMax );

			FxAnimKey newKey(pCurve->GetKey(_selKey));
			newKey.SetTime(mouseTime);
			newKey.SetValue(mouseValue);

			_selKey = pCurve->ModifyKey( _selKey, newKey );
		}
		else if( _mouseAction == MOUSE_MOVEINCHANDLE || _mouseAction == MOUSE_MOVEOUTHANDLE )
		{
			FxReal keyTime = pCurve->GetKey(_selKey).GetTime();
			FxReal keyValue = pCurve->GetKey(_selKey).GetValue();
			FxReal slope = (mouseValue - keyValue) / (mouseTime - keyTime);
			if( _mouseAction == MOUSE_MOVEINCHANDLE )
			{
				pCurve->GetKeyM(_selKey).SetSlopeIn(slope);
			}
			else if( _mouseAction == MOUSE_MOVEOUTHANDLE )
			{
				pCurve->GetKeyM(_selKey).SetSlopeOut(slope);
			}
		}
		Refresh( FALSE );
	}
}

// Called on left mouse button up
void FxFunctionEditor::OnLeftUp( wxMouseEvent& event )
{
	if( _linkFunction && _linkFunction->IsA( FxCustomLinkFn::StaticClass() ) &&
		_linkFnParams.pCustomCurve && _selKey != FxInvalidIndex && _mouseAction == MOUSE_MOVEKEY )
	{
		FxAnimCurve* pCurve = _linkFnParams.pCustomCurve;
		FxReal mouseTime = FxFunctionPlotter::CoordToTime( event.GetX(), _plotRect, _xMin, _xMax );
		FxReal mouseValue = FxFunctionPlotter::CoordToValue( event.GetY(), _plotRect, _yMin, _yMax );

		mouseTime = FxClamp( _xMin, mouseTime, _xMax );
		mouseValue = FxClamp( _yMin, mouseValue, _yMax );

		FxAnimKey newKey(pCurve->GetKey(_selKey));
		newKey.SetTime(mouseTime);
		newKey.SetValue(mouseValue);

		_selKey = pCurve->ModifyKey( _selKey, newKey );
	}

	_mouseAction = MOUSE_NONE;
	Refresh( FALSE );

	if( HasCapture() )
	{
		ReleaseMouse();
	}
}

// Called when the window resizes
void FxFunctionEditor::OnSize( wxSizeEvent& event )
{
	_clientRect = GetClientRect();
	event.Skip();
}

void FxFunctionEditor::OnAddKey( wxCommandEvent& event )
{
	_mouseAction = MOUSE_ADDKEY;
	SetCursor( *wxCROSS_CURSOR );
	event.Skip();
}

void FxFunctionEditor::OnDeleteSelection( wxCommandEvent& event )
{
	if( _selKey != FxInvalidIndex )
	{
		FxAnimCurve* pCurve = _linkFnParams.pCustomCurve;
		pCurve->RemoveKey( _selKey );
		_selKey = FxInvalidIndex;
		Refresh(FALSE);
	}
	event.Skip();
}

// Checks which key, if any, the user clicked on
FxSize FxFunctionEditor::HitTestKeys( wxPoint clickLocation )
{
	if( _linkFunction && _linkFunction->IsA( FxCustomLinkFn::StaticClass() ) &&
		_linkFnParams.pCustomCurve )
	{
		FxAnimCurve* pCurve = _linkFnParams.pCustomCurve;
		for( FxSize i = 0; i < pCurve->GetNumKeys(); ++i )
		{
			const FxAnimKey& key = pCurve->GetKey( i );
			FxInt32 x = FxFunctionPlotter::TimeToCoord( key.GetTime(), _plotRect, _xMin, _xMax );
			FxInt32 y = FxFunctionPlotter::ValueToCoord( key.GetValue(), _plotRect, _yMin, _yMax );

			wxRect hitRect( x, y, 5, 5 );
			hitRect.Offset( -2, -2 );

			if( hitRect.Inside( clickLocation ) )
			{
				return i;
			}
		}
	}
	return FxInvalidIndex;
}

// Returns the point of the incoming tangent handle
wxPoint FxFunctionEditor::GetIncTanHandle( FxSize keyIndex, FxInt32 keyX, FxInt32 keyY )
{
	if( _linkFnParams.pCustomCurve )
	{
		const FxAnimKey& key = _linkFnParams.pCustomCurve->GetKey(keyIndex);
		const FxVec2 keyPoint( keyX, keyY );
		FxVec2 incTan = FxVec2( key.GetTime() - 1, key.GetValue() - key.GetSlopeIn() );
		incTan.x = FxFunctionPlotter::TimeToCoord( incTan.x, _plotRect, _xMin, _xMax );
		incTan.y = FxFunctionPlotter::ValueToCoord( incTan.y, _plotRect, _yMin, _yMax );
		incTan -= keyPoint;
		incTan.Normalize();
		incTan *= 50;
		incTan += keyPoint;
		return wxPoint(incTan.x, incTan.y);
	}
	return wxPoint(0,0);
}

// Returns the point of the outgoing tangent handle
wxPoint FxFunctionEditor::GetOutTanHandle( FxSize keyIndex, FxInt32 keyX, FxInt32 keyY )
{
	if( _linkFnParams.pCustomCurve )
	{
		const FxAnimKey& key = _linkFnParams.pCustomCurve->GetKey(keyIndex);
		const FxVec2 keyPoint( keyX, keyY );
		FxVec2 outTan = FxVec2( key.GetTime() + 1, key.GetValue() + key.GetSlopeOut() );
		outTan.x = FxFunctionPlotter::TimeToCoord( outTan.x, _plotRect, _xMin, _xMax );
		outTan.y = FxFunctionPlotter::ValueToCoord( outTan.y, _plotRect, _yMin, _yMax );
		outTan -= keyPoint;
		outTan.Normalize();
		outTan *= 50;
		outTan += keyPoint;
		return wxPoint(outTan.x, outTan.y);
	}
	return wxPoint(0,0);
}

} // namespace Face

} // namespace OC3Ent
