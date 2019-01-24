/////////////////////////////////////////////////////////////////////////////
// Name:        wx/xrc/xh_text.h
// Purpose:     XML resource handler for wxTextCtrl
// Author:      Aleksandras Gluchovas
// Created:     2000/03/21
// RCS-ID:      $Id: xh_text.h 41590 2006-10-03 14:53:40Z VZ $
// Copyright:   (c) 2000 Aleksandras Gluchovas
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_XH_TEXT_H_
#define _WX_XH_TEXT_H_

#include "wx/xrc/xmlres.h"

#if wxUSE_XRC 

#if wxUSE_TEXTCTRL

class WXDLLIMPEXP_XRC wxTextCtrlXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(wxTextCtrlXmlHandler)

public:
    wxTextCtrlXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);
};

#endif // wxUSE_TEXTCTRL

// @UE3 11-19-2008: Added support for loading wxRichTextCtrl from resource files
#if wxUSE_RICHTEXT

class WXDLLIMPEXP_XRC wxRichTextCtrlXmlHandler : public wxXmlResourceHandler
{
	DECLARE_DYNAMIC_CLASS(wxRichTextCtrlXmlHandler)

public:
	wxRichTextCtrlXmlHandler();
	virtual wxObject* DoCreateResource();
	virtual bool CanHandle(wxXmlNode *node);
};

#endif // wxUSE_RICHTEXT

#endif // wxUSE_XRC

#endif // _WX_XH_TEXT_H_
