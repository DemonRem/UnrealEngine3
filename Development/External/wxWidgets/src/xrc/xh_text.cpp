/////////////////////////////////////////////////////////////////////////////
// Name:        src/xrc/xh_text.cpp
// Purpose:     XRC resource for wxTextCtrl
// Author:      Aleksandras Gluchovas
// Created:     2000/03/21
// RCS-ID:      $Id: xh_text.cpp 40443 2006-08-04 11:10:53Z VZ $
// Copyright:   (c) 2000 Aleksandras Gluchovas
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_XRC

#if wxUSE_TEXTCTRL

#include "wx/xrc/xh_text.h"

#ifndef WX_PRECOMP
    #include "wx/textctrl.h"
#endif

IMPLEMENT_DYNAMIC_CLASS(wxTextCtrlXmlHandler, wxXmlResourceHandler)

wxTextCtrlXmlHandler::wxTextCtrlXmlHandler() : wxXmlResourceHandler()
{
    XRC_ADD_STYLE(wxTE_NO_VSCROLL);
    XRC_ADD_STYLE(wxTE_AUTO_SCROLL);
    XRC_ADD_STYLE(wxTE_PROCESS_ENTER);
    XRC_ADD_STYLE(wxTE_PROCESS_TAB);
    XRC_ADD_STYLE(wxTE_MULTILINE);
    XRC_ADD_STYLE(wxTE_PASSWORD);
    XRC_ADD_STYLE(wxTE_READONLY);
    XRC_ADD_STYLE(wxHSCROLL);
    XRC_ADD_STYLE(wxTE_RICH);
    XRC_ADD_STYLE(wxTE_RICH2);
    XRC_ADD_STYLE(wxTE_AUTO_URL);
    XRC_ADD_STYLE(wxTE_NOHIDESEL);
    XRC_ADD_STYLE(wxTE_LEFT);
    XRC_ADD_STYLE(wxTE_CENTRE);
    XRC_ADD_STYLE(wxTE_RIGHT);
    XRC_ADD_STYLE(wxTE_DONTWRAP);
#if WXWIN_COMPATIBILITY_2_6
    XRC_ADD_STYLE(wxTE_LINEWRAP);
#endif // WXWIN_COMPATIBILITY_2_6
    XRC_ADD_STYLE(wxTE_CHARWRAP);
    XRC_ADD_STYLE(wxTE_WORDWRAP);
    AddWindowStyles();
}

wxObject *wxTextCtrlXmlHandler::DoCreateResource()
{
	XRC_MAKE_INSTANCE(text, wxTextCtrl)

	text->Create(m_parentAsWindow,
				 GetID(),
				 GetText(wxT("value")),
				 GetPosition(), GetSize(),
				 GetStyle(),
				 wxDefaultValidator,
				 GetName());

	SetupWindow(text);

	if (HasParam(wxT("maxlength")))
	{
		text->SetMaxLength(GetLong(wxT("maxlength")));
	}

	return text;
}

bool wxTextCtrlXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("wxTextCtrl"));
}

#endif // wxUSE_TEXTCTRL


// @UE3 11-17-2008: Added support for loading wxRichTextCtrl from resource files
#if wxUSE_RICHTEXT

#include "wx/richtext/richtextctrl.h"

IMPLEMENT_DYNAMIC_CLASS(wxRichTextCtrlXmlHandler, wxXmlResourceHandler)

wxRichTextCtrlXmlHandler::wxRichTextCtrlXmlHandler() : wxXmlResourceHandler()
{
    XRC_ADD_STYLE(wxTE_NO_VSCROLL);
    XRC_ADD_STYLE(wxTE_AUTO_SCROLL);
    XRC_ADD_STYLE(wxTE_PROCESS_ENTER);
    XRC_ADD_STYLE(wxTE_PROCESS_TAB);
	XRC_ADD_STYLE(wxRE_MULTILINE);
    XRC_ADD_STYLE(wxTE_PASSWORD);
    XRC_ADD_STYLE(wxTE_READONLY);
	XRC_ADD_STYLE(wxVSCROLL);
    XRC_ADD_STYLE(wxHSCROLL);
    XRC_ADD_STYLE(wxTE_RICH);
    XRC_ADD_STYLE(wxTE_RICH2);
    XRC_ADD_STYLE(wxTE_AUTO_URL);
    XRC_ADD_STYLE(wxTE_NOHIDESEL);
    XRC_ADD_STYLE(wxTE_LEFT);
    XRC_ADD_STYLE(wxTE_CENTRE);
    XRC_ADD_STYLE(wxTE_RIGHT);
    XRC_ADD_STYLE(wxTE_DONTWRAP);
#if WXWIN_COMPATIBILITY_2_6
    XRC_ADD_STYLE(wxTE_LINEWRAP);
#endif // WXWIN_COMPATIBILITY_2_6
    XRC_ADD_STYLE(wxTE_CHARWRAP);
    XRC_ADD_STYLE(wxTE_WORDWRAP);
    AddWindowStyles();
}

wxObject *wxRichTextCtrlXmlHandler::DoCreateResource()
{
	XRC_MAKE_INSTANCE(richtext,wxRichTextCtrl)

	richtext->Create(m_parentAsWindow,
					GetID(),
					GetText(wxT("value")),
					GetPosition(), GetSize(),
					GetStyle(wxT("style"), wxTE_RICH2|wxTE_AUTO_SCROLL|wxRE_MULTILINE|wxTE_AUTO_URL|wxTE_NOHIDESEL),
					wxDefaultValidator,
					GetName());

	SetupWindow(richtext);

	if (HasParam(wxT("maxlength")))
	{
		richtext->SetMaxLength(GetLong(wxT("maxlength")));
	}

	return richtext;
}

bool wxRichTextCtrlXmlHandler::CanHandle(wxXmlNode* node)
{
    return IsOfClass(node, wxT("wxRichTextCtrl"));
}

#endif // wxUSE_RICHTEXT

#endif // wxUSE_XRC
