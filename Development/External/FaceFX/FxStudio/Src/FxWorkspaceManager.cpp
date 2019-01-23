//------------------------------------------------------------------------------
// A manager for the various workspaces in the system.
//
// Owner: John Briggs
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "stdwx.h"
#include "FxWorkspaceManager.h"

#ifdef __UNREAL__
	#include "XWindow.h"
#else
	// wx-specific includes
#endif

namespace OC3Ent
{

namespace Face
{

//------------------------------------------------------------------------------
// FxWorkspace
//------------------------------------------------------------------------------
// Construction / Destruction
//------------------------------------------------------------------------------
#define kCurrentWorkspaceVersion 0

FX_IMPLEMENT_CLASS(FxWorkspace, kCurrentWorkspaceVersion, FxNamedObject)

FxWorkspace::FxWorkspace()
	: _actor(NULL)
	, _anim(NULL)
	, _animController(NULL)
	, _focusedElement(NULL)
	, _captureElement(NULL)
	, _selectedElement(NULL)
	, _focusedSlider(NULL)
	, _mode(WM_Combiner)
	, _autoKey(FxFalse)
	, _isDirty(FxFalse)
	, _textColour(*wxLIGHT_GREY)
{
}

FxWorkspace::FxWorkspace(const FxName& workspaceName)
	: FxNamedObject(workspaceName)
	, _actor(NULL)
	, _anim(NULL)
	, _animController(NULL)
	, _focusedElement(NULL)
	, _captureElement(NULL)
	, _selectedElement(NULL)
	, _focusedSlider(NULL)
	, _mode(WM_Combiner)
	, _autoKey(FxFalse)
	, _isDirty(FxFalse)
	, _textColour(*wxLIGHT_GREY)
{
}

FxWorkspace::~FxWorkspace()
{
}

//------------------------------------------------------------------------------
// Accessors.
//------------------------------------------------------------------------------

void FxWorkspace::SetWorkspaceRect(const wxRect& workspaceRect)
{
  	_workspaceRect = workspaceRect;

	FxList<FxWorkspaceElementSlider>::Iterator currSlider = _sliders.Begin();
	FxList<FxWorkspaceElementSlider>::Iterator endSlider  = _sliders.End();
	while( currSlider != endSlider )
	{
		currSlider->SetWorkspaceRect(_workspaceRect);
		++currSlider;
	}

	FxList<FxWorkspaceElementBackground>::Iterator currBg = _backgrounds.Begin();
	FxList<FxWorkspaceElementBackground>::Iterator endBg  = _backgrounds.End();
	while( currBg != endBg )
	{
		currBg->SetWorkspaceRect(_workspaceRect);
		++currBg;
	}

	FxList<FxWorkspaceElementViewport>::Iterator currVp = _viewports.Begin();
	FxList<FxWorkspaceElementViewport>::Iterator endVp  = _viewports.End();
	while( currVp != endVp )
	{
		currVp->SetWorkspaceRect(_workspaceRect);
		++currVp;
	}
}

// Returns the rect used as the bounds of the workspace.
const wxRect& FxWorkspace::GetWorkspaceRect() const
{
	return _workspaceRect;
}

void FxWorkspace::SetActor(FxActor* pActor)
{
	_actor = pActor;
	if( _actor )
	{
		FxList<FxWorkspaceElementSlider>::Iterator currSlider = _sliders.Begin();
		FxList<FxWorkspaceElementSlider>::Iterator endSlider  = _sliders.End();
		while( currSlider != endSlider )
		{
			currSlider->LinkToNodes(&_actor->GetFaceGraph());
			++currSlider;
		}
	}
}

void FxWorkspace::SetAnim(FxAnim* pAnim)
{
	_anim = pAnim;
	FxList<FxWorkspaceElementSlider>::Iterator currSlider = _sliders.Begin();
	FxList<FxWorkspaceElementSlider>::Iterator endSlider  = _sliders.End();
	while( currSlider != endSlider )
	{
		currSlider->LinkToAnim(_anim);
		++currSlider;
	}
}

void FxWorkspace::SetAnimController(FxAnimController* pAnimController)
{
	_animController = pAnimController;
	if( _animController )
	{
		FxList<FxWorkspaceElementSlider>::Iterator currSlider = _sliders.Begin();
		FxList<FxWorkspaceElementSlider>::Iterator endSlider  = _sliders.End();
		while( currSlider != endSlider )
		{
			currSlider->LinkToAnimController(_animController);
			++currSlider;
		}
	}
}

void FxWorkspace::SetCurrentTime(FxReal currentTime)
{
	_currentTime = currentTime;
	FxList<FxWorkspaceElementSlider>::Iterator currSlider = _sliders.Begin();
	FxList<FxWorkspaceElementSlider>::Iterator endSlider  = _sliders.End();
	while( currSlider != endSlider )
	{
		currSlider->SetCurrentTime(_currentTime);
		++currSlider;
	}
}

FxSize FxWorkspace::GetNumSliders()
{
	return _sliders.Length();
}

FxWorkspaceElementSlider* FxWorkspace::GetSlider(FxSize index)
{
	FxList<FxWorkspaceElementSlider>::Iterator retval = _sliders.Begin();
	retval.Advance(index);
	return &*retval;
}

FxSize FxWorkspace::GetNumViewports()
{
	return _viewports.Length();
}

FxWorkspaceElementViewport* FxWorkspace::GetViewport(FxSize index)
{
	FxList<FxWorkspaceElementViewport>::Iterator retval = _viewports.Begin();
	retval.Advance(index);
	return &*retval;
}

FxWorkspaceElement* FxWorkspace::GetSelectedElement()
{
	return _selectedElement;
}

FxWorkspaceMode FxWorkspace::GetMode()
{
	return _mode;
}

void FxWorkspace::SetMode(FxWorkspaceMode mode)
{
	_mode = mode;
	
	FxList<FxWorkspaceElementSlider>::Iterator currSlider = _sliders.Begin();
	FxList<FxWorkspaceElementSlider>::Iterator endSlider  = _sliders.End();
	while( currSlider != endSlider )
	{
		currSlider->SetMode(_mode);
		++currSlider;
	}
}

// Sets the workspace options.
void FxWorkspace::SetOption(FxBool autoKey)
{
	_autoKey = autoKey;

	FxList<FxWorkspaceElementSlider>::Iterator currSlider = _sliders.Begin();
	FxList<FxWorkspaceElementSlider>::Iterator endSlider  = _sliders.End();
	while( currSlider != endSlider )
	{
		currSlider->SetOption(_autoKey);
		++currSlider;
	}
}

// Gets the text colour.
wxColour FxWorkspace::GetTextColour()
{
	return _textColour;
}

// Sets the text colour.
void FxWorkspace::SetTextColour(const wxColour& colour)
{
	_textColour = colour;
}

//------------------------------------------------------------------------------
// Element addition.
//------------------------------------------------------------------------------

void FxWorkspace::AddSlider(const FxWorkspaceElementSlider& slider)
{
	_sliders.PushBack(slider);
	_sliders.Back().LinkToAnimController(_animController);
	_sliders.Back().LinkToAnim(_anim);
	_sliders.Back().LinkToNodes(&_actor->GetFaceGraph());
	_sliders.Back().SetCurrentTime(_currentTime);
	_sliders.Back().SetMode(_mode);
	_sliders.Back().SetOption(_autoKey);
}

void FxWorkspace::AddBackground(const FxWorkspaceElementBackground& background)
{
	_backgrounds.PushBack(background);
	_backgrounds.Back().SetWorkspaceRect(_workspaceRect);
}

void FxWorkspace::AddViewport(const FxWorkspaceElementViewport& viewport)
{
	_viewports.PushBack(viewport);
	_viewports.Back().SetWorkspaceRect(_workspaceRect);
}

FxBool FxWorkspace::RemoveElement(FxWorkspaceElement* pElem)
{
	FxList<FxWorkspaceElementSlider>::Iterator currSlider = _sliders.Begin();
	FxList<FxWorkspaceElementSlider>::Iterator endSlider  = _sliders.End();
	while( currSlider != endSlider )
	{
		if( pElem == &*currSlider )
		{
			if( _selectedElement == &*currSlider )
			{
				_selectedElement = NULL;
			}
			if( _captureElement == &*currSlider )
			{
				_captureElement = NULL;
			}
			if( _focusedElement == &*currSlider )
			{
				_captureElement = NULL;
			}
			if( _focusedSlider == &* currSlider )
			{
				_focusedSlider = NULL;
			}
			_sliders.Remove(currSlider);
			return FxTrue;
		}
		++currSlider;
	}

	FxList<FxWorkspaceElementBackground>::Iterator currBg = _backgrounds.Begin();
	FxList<FxWorkspaceElementBackground>::Iterator endBg  = _backgrounds.End();
	while( currBg != endBg )
	{
		if( pElem == &*currBg )
		{
			if( _selectedElement == &*currBg )
			{
				_selectedElement = NULL;
			}
			if( _captureElement == &*currBg )
			{
				_captureElement = NULL;
			}
			if( _focusedElement == &*currBg )
			{
				_captureElement = NULL;
			}
			_backgrounds.Remove(currBg);
			return FxTrue;
		}
		++currBg;
	}

	FxList<FxWorkspaceElementViewport>::Iterator currVp = _viewports.Begin();
	FxList<FxWorkspaceElementViewport>::Iterator endVp  = _viewports.End();
	while( currVp != endVp )
	{
		if( pElem == &*currVp )
		{
			if( _selectedElement == &*currVp )
			{
				_selectedElement = NULL;
			}
			if( _captureElement == &*currVp )
			{
				_captureElement = NULL;
			}
			if( _focusedElement == &*currVp )
			{
				_captureElement = NULL;
			}
			_viewports.Remove(currVp);
			return FxTrue;
		}
		++currVp;
	}

	return FxFalse;
}

//------------------------------------------------------------------------------
// User interaction.
//------------------------------------------------------------------------------

void FxWorkspace::DrawOn(wxDC* pDC, FxBool editMode, FxBool drawGrid)
{
	// Clear the workspace to white.
	pDC->SetBrush(*wxWHITE_BRUSH);
	pDC->SetPen(*wxWHITE_PEN);
	pDC->DrawRectangle(_workspaceRect);
	pDC->SetBrush(wxNullBrush);
	pDC->SetPen(wxNullPen);

	wxFont systemFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
	pDC->SetFont(systemFont);
	pDC->SetTextForeground(_textColour);

	FxList<FxWorkspaceElementBackground>::Iterator currBg = _backgrounds.Begin();
	FxList<FxWorkspaceElementBackground>::Iterator endBg  = _backgrounds.End();
	while( currBg != endBg )
	{
		currBg->DrawOn(pDC, editMode, &*currBg == _selectedElement);
		++currBg;
	}

	FxList<FxWorkspaceElementViewport>::Iterator currVp = _viewports.Begin();
	FxList<FxWorkspaceElementViewport>::Iterator endVp  = _viewports.End();
	while( currVp != endVp )
	{
		currVp->DrawOn(pDC, editMode, &*currVp == _selectedElement);
		++currVp;
	}

	if( drawGrid )
	{
		wxPen solidPen(_textColour, 1, wxSOLID);
		wxPen dashPen(_textColour, 1, wxLONG_DASH);
		wxPen dotPen(_textColour, 1, wxDOT);
		static const FxInt32 posGrid = 2;
		const FxInt32 maxGrid = 100/posGrid;
		for( FxSize i = 0; i < maxGrid; ++i )
		{
			if( i % 10 == 0 )
			{
				pDC->SetPen(solidPen);
			}
			else if( i % 5 == 0 )
			{
				pDC->SetPen(dashPen);
			}
			else
			{
				pDC->SetPen(dotPen);
			}
			FxReal xPos = (static_cast<FxReal>(i * posGrid) / 100.0f) * _workspaceRect.GetWidth();
			FxReal yPos = (static_cast<FxReal>(i * posGrid) / 100.0f) * _workspaceRect.GetHeight();
			pDC->DrawLine(_workspaceRect.GetLeft() + xPos, _workspaceRect.GetTop(), _workspaceRect.GetLeft() + xPos, _workspaceRect.GetBottom());
			pDC->DrawLine(_workspaceRect.GetLeft(), _workspaceRect.GetTop() + yPos, _workspaceRect.GetRight(), _workspaceRect.GetTop() + yPos);
		}
	}

	FxList<FxWorkspaceElementSlider>::Iterator currSlider = _sliders.Begin();
	FxList<FxWorkspaceElementSlider>::Iterator endSlider  = _sliders.End();
	while( currSlider != endSlider )
	{
		FxBool cachedHighlight = currSlider->GetHighlight();
		if( FxWorkspaceManager::Instance()->GetDisplayNames() )
		{
			currSlider->SetHighlight(FxTrue);
		}
		currSlider->DrawOn(pDC, editMode, (&*currSlider == _selectedElement));
		if( FxWorkspaceManager::Instance()->GetDisplayNames() )
		{
			currSlider->SetHighlight(cachedHighlight);
		}
		++currSlider;
	}
}

void FxWorkspace::ProcessMouseEvent(wxMouseEvent& event, FxInt32& needsRefresh, FxBool editMode)
{
	FxWorkspaceElement* prevSelElem = _selectedElement;
	if( _captureElement )
	{
		if( !_captureElement->ProcessMouse(event, needsRefresh, editMode) )
		{
			_captureElement = NULL;
		}

		if( editMode && event.Dragging() && event.LeftIsDown() && needsRefresh )
		{
			// Not a 100% accurate way of knowing if the element's transform
			// changed, but pretty darn close.
			_isDirty = FxTrue;
		}

		return;
	}

	FxBool foundElement = FxFalse;
	if( editMode )
	{
		// Clear the selected element.
		if( event.LeftDown() )
		{
			_selectedElement = NULL;
		}

		FxList<FxWorkspaceElementSlider>::ReverseIterator currSlider = _sliders.ReverseBegin();
		FxList<FxWorkspaceElementSlider>::ReverseIterator endSlider  = _sliders.ReverseEnd();
		while( currSlider != endSlider && !foundElement)
		{
			FxWorkspaceElement::HitResult res = currSlider->HitTest(event.GetPosition(), editMode);

			if( res == FxWorkspaceElement::HR_Passed )
			{
				if( _focusedSlider != &*currSlider )
				{
					if( _focusedSlider )
					{
						_focusedSlider->SetHighlight(FxFalse);
					}
					_focusedSlider = &*currSlider;
					_focusedSlider->SetHighlight(FxTrue);
					needsRefresh = WRF_Workspaces;
				}
			}
			else if( _focusedSlider && _focusedSlider == &*currSlider && 
					 res == FxWorkspaceElement::HR_Failed)
			{
				_focusedSlider->SetHighlight(FxFalse);
				_focusedSlider = NULL;
				needsRefresh = WRF_Workspaces;
			}

			// If we have found our hit element, we can stop.
			if( res != FxWorkspaceElement::HR_Failed &&
				res != FxWorkspaceElement::HR_Passed )
			{
				FxReal rot = currSlider->GetRotation();
				rot += currSlider->GetScale().x < 0 ? 90.f : 0.f;
				rot += currSlider->GetScale().y < 0 ? 90.f : 0.f;
				SelectCursor(res, rot, currSlider->GetScale());
				_focusedElement = &*currSlider;
				foundElement = FxTrue;
			}

			if( (foundElement || res == FxWorkspaceElement::HR_Passed) &&
				event.LeftDown() )
			{
				_selectedElement = &*currSlider;
			}
				
			++currSlider;
		}

		if( !foundElement )
		{
			FxList<FxWorkspaceElementViewport>::ReverseIterator currVp = _viewports.ReverseBegin();
			FxList<FxWorkspaceElementViewport>::ReverseIterator endVp = _viewports.ReverseEnd();
			while( currVp != endVp && !foundElement )
			{
				FxWorkspaceElement::HitResult res = currVp->HitTest(event.GetPosition(), editMode);

				// If we have found our hit element, we can stop.
				if( res != FxWorkspaceElement::HR_Failed &&
					res != FxWorkspaceElement::HR_Passed )
				{
					SelectCursor(res, currVp->GetRotation(), currVp->GetScale());
					_focusedElement = &*currVp;
					foundElement = FxTrue;
				}

				if( (foundElement || res == FxWorkspaceElement::HR_Passed) &&
					event.LeftDown() && !_selectedElement)
				{
					_selectedElement = &*currVp;
				}

				++currVp;
			}

		}

		if( !foundElement )
		{
			FxList<FxWorkspaceElementBackground>::ReverseIterator currBg = _backgrounds.ReverseBegin();
			FxList<FxWorkspaceElementBackground>::ReverseIterator endBg  = _backgrounds.ReverseEnd();
			while( currBg != endBg && !foundElement)
			{
				FxWorkspaceElement::HitResult res = currBg->HitTest(event.GetPosition(), editMode);

				// If we have found our hit element, we can stop.
				if( res != FxWorkspaceElement::HR_Failed &&
					res != FxWorkspaceElement::HR_Passed )
				{
					SelectCursor(res, currBg->GetRotation(), currBg->GetScale());
					_focusedElement = &*currBg;
					foundElement = FxTrue;
				}

				if( (foundElement || res == FxWorkspaceElement::HR_Passed) &&
					event.LeftDown() && !_selectedElement)
				{
					_selectedElement = &*currBg;
				}

				++currBg;
			}
		}
	}
	else // not edit mode - limit interactions.
	{
		FxList<FxWorkspaceElementSlider>::ReverseIterator currSlider = _sliders.ReverseBegin();
		FxList<FxWorkspaceElementSlider>::ReverseIterator endSlider  = _sliders.ReverseEnd();
		while( currSlider != endSlider && !foundElement)
		{
			FxWorkspaceElement::HitResult res = currSlider->HitTest(event.GetPosition(), editMode);

			if( res == FxWorkspaceElement::HR_Passed )
			{
				if( _focusedSlider != &*currSlider )
				{
					if( _focusedSlider )
					{
						_focusedSlider->SetHighlight(FxFalse);
					}
					_focusedSlider = &*currSlider;
					_focusedSlider->SetHighlight(FxTrue);
					needsRefresh = WRF_Workspaces;
				}
			}
			else if( _focusedSlider && _focusedSlider == &*currSlider && 
				res == FxWorkspaceElement::HR_Failed)
			{
				_focusedSlider->SetHighlight(FxFalse);
				_focusedSlider = NULL;
				needsRefresh = WRF_Workspaces;
			}

			// If we have found our hit element, we can stop.
			if( res == FxWorkspaceElement::HR_Passed )
			{
				_focusedElement = &*currSlider;
				foundElement = FxTrue;
			}

			++currSlider;
		}
	}

	if( !foundElement )
	{
		_focusedElement = NULL;
		::wxSetCursor(wxCursor(wxCURSOR_ARROW));
	}

	if( _focusedElement && !_captureElement )
	{
		if( _focusedElement->ProcessMouse(event, needsRefresh, editMode) )
		{
			_captureElement = _focusedElement;
		}
	}

	//UpdateAllSliders();

	// Ensure a refresh happens if the selected element changes.
	if( prevSelElem != _selectedElement )
	{
		needsRefresh |= WRF_Workspaces;
	}
}

void FxWorkspace::ProcessKeyDown(wxKeyEvent& event, FxInt32& needsRefresh, FxBool editMode)
{
	if( _focusedSlider )
	{
		_focusedSlider->ProcessKeyDown(event, needsRefresh, editMode);
	}
}

void FxWorkspace::UpdateAllSliders()
{
	// Update all sliders.
	FxList<FxWorkspaceElementSlider>::Iterator currSlider = _sliders.Begin();
	FxList<FxWorkspaceElementSlider>::Iterator endSlider  = _sliders.End();
	while( currSlider != endSlider )
	{
		currSlider->UpdateSlider();
		++currSlider;
	}
}

//------------------------------------------------------------------------------
// Member functions
//------------------------------------------------------------------------------
void FxWorkspace::SelectCursor(FxWorkspaceElement::HitResult res, FxReal rot, const FxVec2& scale)
{
	FxReal initialRot = rot;
	switch(res)
	{
	case FxWorkspaceElement::HR_ScaleNE:
		initialRot += 45.f;
	case FxWorkspaceElement::HR_ScaleE:
		initialRot += 45.f;
	case FxWorkspaceElement::HR_ScaleSE:
		initialRot += 45.f;
	case FxWorkspaceElement::HR_ScaleS:
		initialRot += 45.f;
	case FxWorkspaceElement::HR_ScaleSW:
		initialRot += 45.f;
	case FxWorkspaceElement::HR_ScaleW:
		initialRot += 45.f;
	case FxWorkspaceElement::HR_ScaleNW:
		initialRot += 45.f;
	case FxWorkspaceElement::HR_ScaleN:
		{
			initialRot += 45.f * 0.5f;
			// Bring the rotation into range.
			while( !(0.f <= initialRot && initialRot <= 180.f ) )
			{
				if( initialRot < 0.f )
				{
					initialRot += 180.f;
				}
				else
				{
					initialRot -= 180.f;
				}
			}

			FxReal scaleSign = scale.x * scale.y < 0.0f ? -1.0f : 1.0f;

			if( (initialRot <= 45.f && scaleSign > 0.f) || (90.0f < initialRot && initialRot <= 135.f && scaleSign < 1.f))
			{
				::wxSetCursor(wxCursor(wxCURSOR_SIZENS));
			}
			else if( 45.f < initialRot && initialRot <= 90.f )
			{
				::wxSetCursor(wxCursor(wxCURSOR_SIZENWSE));
			}
			else if( (initialRot <= 135.f && scaleSign > 0.f) || (initialRot < 45.f && scaleSign < 0.f) )
			{
				::wxSetCursor(wxCursor(wxCURSOR_SIZEWE));
			}
			else if( initialRot <= 180.f )
			{
				::wxSetCursor(wxCursor(wxCURSOR_SIZENESW));
			}
		}
		break;
	case FxWorkspaceElement::HR_Rotate:
		//@todo need a rotate cursor =/
		::wxSetCursor(wxCursor(wxCURSOR_SIZING));
		break;
	case FxWorkspaceElement::HR_Translate:
		::wxSetCursor(wxCursor(wxCURSOR_SIZING));
		break;
	default:
		break;
	};
}


//------------------------------------------------------------------------------
// Serialization
//------------------------------------------------------------------------------

// Background bitmap serialization.
#define kCurrentFxWorkspaceElementBackgroundVersion 0

FxArchive& operator<<(FxArchive& arc, FxWorkspaceElementBackground& obj)
{
	FxUInt16 version = kCurrentFxWorkspaceElementBackgroundVersion;
	arc << version;

	// Element transform info.
	arc << obj._pos.x << obj._pos.y << obj._scale.x << obj._scale.y << obj._rot;
	if( arc.IsLoading() )
	{
		obj.SetPosition(obj._pos);
		obj.SetScale(obj._scale);
		obj.SetRotation(obj._rot);
	}

	FxInt32 w = 0;
	FxInt32 h = 0;
	if( arc.IsSaving() )
	{
		w = obj._original.GetWidth();
		h = obj._original.GetHeight();
	}
	
	// Serialize the width and height
	arc << w << h;
	if( arc.IsLoading() )
	{
		// Must use malloc here - that's what the wxImage expects
		FxUChar* imageData = static_cast<FxUChar*>(malloc(w * h * 3 * sizeof(FxUChar)));
		arc.SerializePODArray(imageData, w * h * 3);
		obj._original = wxImage(w, h, imageData);
	}
	else
	{
		FxUChar* imageData = obj._original.GetData();
		arc.SerializePODArray(imageData, w * h * 3);
	}

	return arc;
}

// Slider serialization.
#define kCurrentFxWorkspaceElementSliderVersion 0

FxArchive& operator<<(FxArchive& arc, FxWorkspaceElementSlider& obj)
{
	FxUInt16 version = kCurrentFxWorkspaceElementSliderVersion;
	arc << version;

	// Element transform info.
	arc << obj._pos.x << obj._pos.y << obj._scale.x << obj._scale.y << obj._rot;
	if( arc.IsLoading() )
	{
		obj.SetPosition(obj._pos);
		obj.SetScale(obj._scale);
		obj.SetRotation(obj._rot);
	}

	// Slider-specific info.
	arc << obj._nodeNameX << obj._nodeNameY;

	return arc;
}

// Viewport serialization.
#define kCurrentFxWorkspaceElementViewportVersion 0

FxArchive& operator<<(FxArchive& arc, FxWorkspaceElementViewport& obj)
{
	FxUInt16 version = kCurrentFxWorkspaceElementViewportVersion;
	arc << version;

	// Element transform info.
	arc << obj._pos.x << obj._pos.y << obj._scale.x << obj._scale.y << obj._rot;
	if( arc.IsLoading() )
	{
		obj.SetPosition(obj._pos);
		obj.SetScale(obj._scale);
		obj.SetRotation(obj._rot);
	}

	// Viewport-specific info.
	arc << obj._cameraName;

	return arc;
}

void FxWorkspace::Serialize( FxArchive& arc )
{
	Super::Serialize(arc);

	FxUInt16 version = FX_GET_CLASS_VERSION(FxWorkspace);
	arc << version;

	arc << _backgrounds << _sliders << _viewports;
}

//------------------------------------------------------------------------------
// FxWorkspaceManager
//------------------------------------------------------------------------------

FxWorkspaceManager* FxWorkspaceManager::_instance = NULL;

void FxWorkspaceManager::Startup()
{
	FxWorkspace blankWorkspace("Workspace 1");
	Instance()->Add(blankWorkspace);
}

FxWorkspaceManager* FxWorkspaceManager::Instance()
{
	if( !_instance )
	{
		_instance = new FxWorkspaceManager();
	}
	return _instance;
}

void FxWorkspaceManager::Shutdown()
{
	delete _instance;
	_instance = NULL;
}

void FxWorkspaceManager::Clear()
{
	// Tell the containers we're performing a close operation.
	DispatchWorkspaceClose();

	// Remove all the workspaces from the manager.
	while( _workspaces.Length() > 0 )
	{
		// Remove returns true when we've removed the last workspace and
		// added a default workspace.
		if( Remove(_workspaces.Front().GetName()) )
		{
			break;
		}
	}
}

void FxWorkspaceManager::Add(const FxWorkspace& workspace)
{
	_workspaces.PushBack(workspace);
	DispatchWorkspaceAdded(workspace.GetName());
}

FxBool FxWorkspaceManager::Remove(const FxName& workspaceName)
{
	FxList<FxWorkspace>::Iterator curr = _workspaces.Begin();
	FxList<FxWorkspace>::Iterator end  = _workspaces.End();
	while( curr != end )
	{
		if( curr->GetName() == workspaceName )
		{
			DispatchWorkspacePreRemove(workspaceName);
			_workspaces.Remove(curr);
			DispatchWorkspacePostRemove(workspaceName);
			break;
		}
		++curr;
	}

	// If we removed the last workspace, add a new one in its stead.
	if( _workspaces.Length() == 0 )
	{
		FxWorkspace blankWorkspace("Workspace 1");
		Instance()->Add(blankWorkspace);
		DispatchWorkspaceAdded(blankWorkspace.GetName());
		return FxTrue;
	}

	return FxFalse;
}

// Renames a workspace.
void FxWorkspaceManager::Rename(const FxName& originalName, const FxName& newName)
{
	FxList<FxWorkspace>::Iterator curr = _workspaces.Begin();
	FxList<FxWorkspace>::Iterator end  = _workspaces.End();
	while( curr != end )
	{
		if( curr->GetName() == originalName )
		{
			curr->SetName(newName);
			DispatchWorkspaceRenamed(originalName, newName);
			break;
		}
		++curr;
	}
}

// Modifies a workspace.
void FxWorkspaceManager::Modify(const FxName& workspaceName, const FxWorkspace& newValue)
{
	FxList<FxWorkspace>::Iterator curr = _workspaces.Begin();
	FxList<FxWorkspace>::Iterator end  = _workspaces.End();
	while( curr != end )
	{
		if( curr->GetName() == workspaceName )
		{
			(*curr) = newValue;
			DispatchWorkspaceModified(workspaceName);
			break;
		}
		++curr;
	}
}

void FxWorkspaceManager::FlagWorkspacesNeedUpdate()
{
	FxList<FxWorkspaceContainer*>::Iterator curr = _workspaceContainers.Begin();
	FxList<FxWorkspaceContainer*>::Iterator end  = _workspaceContainers.End();
	while( curr != end )
	{
		(*curr)->OnWorkspaceNeedsUpdate();
		++curr;
	}
}

void FxWorkspaceManager::ChangeMode(FxWorkspaceMode newMode)
{
	FxList<FxWorkspaceContainer*>::Iterator curr = _workspaceContainers.Begin();
	FxList<FxWorkspaceContainer*>::Iterator end  = _workspaceContainers.End();
	while( curr != end )
	{
		(*curr)->OnWorkspaceModeSwitch(newMode);
		++curr;
	}
}

void FxWorkspaceManager::ChangeOption(FxBool autoKey)
{
	FxList<FxWorkspaceContainer*>::Iterator curr = _workspaceContainers.Begin();
	FxList<FxWorkspaceContainer*>::Iterator end  = _workspaceContainers.End();
	while( curr != end )
	{
		(*curr)->OnWorkspaceOptionChanged(autoKey);
		++curr;
	}
}

FxBool FxWorkspaceManager::GetDisplayNames()
{
	return _shouldDisplayNames;
}

void FxWorkspaceManager::SetDisplayNames(FxBool displayNames)
{
	_shouldDisplayNames = displayNames;
}

const FxWorkspace& FxWorkspaceManager::Find(const FxName& workspaceName)
{
	static FxWorkspace emptyWorkspace;
	FxList<FxWorkspace>::Iterator curr = _workspaces.Begin();
	FxList<FxWorkspace>::Iterator end  = _workspaces.End();
	while( curr != end )
	{
		if( curr->GetName() == workspaceName )
		{
			return *curr;
		}
		++curr;
	}
	return emptyWorkspace;
}

FxSize FxWorkspaceManager::GetNumWorkspaces() const
{
	return _workspaces.Length();
}

const FxWorkspace& FxWorkspaceManager::GetWorkspace(FxSize index)
{
	FxList<FxWorkspace>::Iterator curr = _workspaces.Begin();
	curr.Advance(index);
	return *curr;
}

void FxWorkspaceManager::AddWorkspaceContainer(FxWorkspaceContainer* pContainer)
{
	_workspaceContainers.PushBack(pContainer);
}

void FxWorkspaceManager::RemoveWorkspaceContainer(FxWorkspaceContainer* pContainer)
{
	FxList<FxWorkspaceContainer*>::Iterator toRemove = _workspaceContainers.Find(pContainer);
	if( toRemove != _workspaceContainers.End() )
	{
		_workspaceContainers.Remove(toRemove);
	}
}

void FxWorkspaceManager::DispatchWorkspaceAdded(const FxName& newWorkspaceName)
{
	FxList<FxWorkspaceContainer*>::Iterator curr = _workspaceContainers.Begin();
	FxList<FxWorkspaceContainer*>::Iterator end  = _workspaceContainers.End();
	while( curr != end )
	{
		(*curr)->OnWorkspaceAdded(newWorkspaceName);
		++curr;
	}
}

void FxWorkspaceManager::DispatchWorkspacePreRemove(const FxName& workspaceToRemove)
{
	FxList<FxWorkspaceContainer*>::Iterator curr = _workspaceContainers.Begin();
	FxList<FxWorkspaceContainer*>::Iterator end  = _workspaceContainers.End();
	while( curr != end )
	{
		(*curr)->OnWorkspacePreRemove(workspaceToRemove);
		++curr;
	}
}

void FxWorkspaceManager::DispatchWorkspacePostRemove(const FxName& removedWorkspace)
{
	FxList<FxWorkspaceContainer*>::Iterator curr = _workspaceContainers.Begin();
	FxList<FxWorkspaceContainer*>::Iterator end  = _workspaceContainers.End();
	while( curr != end )
	{
		(*curr)->OnWorkspacePostRemove(removedWorkspace);
		++curr;
	}
}

void FxWorkspaceManager::DispatchWorkspaceRenamed(const FxName& oldName, const FxName& newName)
{
	FxList<FxWorkspaceContainer*>::Iterator curr = _workspaceContainers.Begin();
	FxList<FxWorkspaceContainer*>::Iterator end  = _workspaceContainers.End();
	while( curr != end )
	{
		(*curr)->OnWorkspaceRenamed(oldName, newName);
		++curr;
	}
}

void FxWorkspaceManager::DispatchWorkspaceModified(const FxName& modifiedWorkspace)
{
	FxList<FxWorkspaceContainer*>::Iterator curr = _workspaceContainers.Begin();
	FxList<FxWorkspaceContainer*>::Iterator end  = _workspaceContainers.End();
	while( curr != end )
	{
		(*curr)->OnWorkspaceModified(modifiedWorkspace);
		++curr;
	}
}

void FxWorkspaceManager::DispatchWorkspaceClose()
{
	FxList<FxWorkspaceContainer*>::Iterator curr = _workspaceContainers.Begin();
	FxList<FxWorkspaceContainer*>::Iterator end  = _workspaceContainers.End();
	while( curr != end )
	{
		(*curr)->OnWorkspaceClose();
		++curr;
	}
}

#define kCurrentFxWorkspaceManagerVersion 0

// Serialize the workspaces to/from an archive.
void FxWorkspaceManager::Serialize(FxArchive& arc)
{
	FxUInt16 version = kCurrentFxWorkspaceManagerVersion;
	arc << version;

	if( arc.IsLoading() )
	{
		// Clear all the workspaces from the manager.
		DispatchWorkspaceClose();
		Clear();
	}

	arc << _workspaces;
	
	if( arc.IsLoading() )
	{
		FxList<FxWorkspace>::Iterator curr = _workspaces.Begin();
		FxList<FxWorkspace>::Iterator end  = _workspaces.End();
		while( curr != end )
		{
			DispatchWorkspaceAdded(curr->GetName());
			++curr;
		}
	}
}

FxWorkspaceManager::FxWorkspaceManager()
	: _shouldDisplayNames(FxFalse)
{
}

FxWorkspaceManager::~FxWorkspaceManager()
{
}

} // namespace Face

} // namespace OC3Ent