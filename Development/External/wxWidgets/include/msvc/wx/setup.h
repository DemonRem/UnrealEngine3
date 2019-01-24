/////////////////////////////////////////////////////////////////////////////
// Name:        msvc/wx/msw/setup.h
// Purpose:     wrapper around the real wx/setup.h for Visual C++
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2004-12-12
// RCS-ID:      $Id: setup.h 43687 2006-11-27 15:03:59Z VZ $
// Copyright:   (c) 2004 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// VC++ IDE predefines _DEBUG and _UNICODE for the new projects itself, but
// the other symbols (WXUSINGDLL, __WXUNIVERSAL__, ...) should be defined
// explicitly!



#ifdef _MSC_VER
	#ifdef _WIN64
		#ifdef _UNICODE
			#ifdef WXUSINGDLL
				#ifdef _DEBUG
					#include "../../../lib/vc_dll/x64/mswud/wx/setup.h"
				#else
					#include "../../../lib/vc_dll/x64/mswu/wx/setup.h"
				#endif
			#else
				#ifdef _DEBUG
					#include "../../../lib/vc_lib/x64/mswud/wx/setup.h"
				#else
					#include "../../../lib/vc_lib/x64/mswu/wx/setup.h"
				#endif
			#endif

			#ifdef _DEBUG
				#pragma comment(lib,"wxbase28ud_64")
				#pragma comment(lib,"wxbase28ud_net_64")
				#pragma comment(lib,"wxbase28ud_xml_64")
				#if wxUSE_REGEX
					#pragma comment(lib,"wxregexud_64")
				#endif

				#if wxUSE_GUI
					#if wxUSE_XML
						#pragma comment(lib,"wxexpatd_64")
					#endif
					#if wxUSE_LIBJPEG
						#pragma comment(lib,"wxjpegd_64")
					#endif
					#if wxUSE_LIBPNG
						#pragma comment(lib,"wxpngd_64")
					#endif
					#if wxUSE_LIBTIFF
						#pragma comment(lib,"wxtiffd_64")
					#endif
					#if wxUSE_ZLIB
						#pragma comment(lib,"wxzlibd_64")
					#endif
					#pragma comment(lib,"wxmsw28ud_adv_64")
					#pragma comment(lib,"wxmsw28ud_core_64")
					#pragma comment(lib,"wxmsw28ud_html_64")
					#if wxUSE_GLCANVAS
						#pragma comment(lib,"wxmsw28ud_gl_64")
					#endif
					#if wxUSE_DEBUGREPORT
						#pragma comment(lib,"wxmsw28ud_qa_64")
					#endif
					#if wxUSE_XRC
						#pragma comment(lib,"wxmsw28ud_xrc_64")
					#endif
					#if wxUSE_AUI
						#pragma comment(lib,"wxmsw28ud_aui_64")
					#endif
					#if wxUSE_RICHTEXT
						#pragma comment(lib,"wxmsw28ud_richtext_64")
					#endif
					#if wxUSE_MEDIACTRL
						#pragma comment(lib,"wxmsw28ud_media_64")
					#endif
					#if wxUSE_ODBC
						#pragma comment(lib,"wxbase28ud_odbc_64")
					#endif
				#endif // wxUSE_GUI
			#else // release
				#pragma comment(lib,"wxbase28u_64")
				#pragma comment(lib,"wxbase28u_net_64")
				#pragma comment(lib,"wxbase28u_xml_64")
				#if wxUSE_REGEX
					#pragma comment(lib,"wxregexu_64")
				#endif

				#if wxUSE_GUI
					#if wxUSE_XML
						#pragma comment(lib,"wxexpat_64")
					#endif
					#if wxUSE_LIBJPEG
						#pragma comment(lib,"wxjpeg_64")
					#endif
					#if wxUSE_LIBPNG
						#pragma comment(lib,"wxpng_64")
					#endif
					#if wxUSE_LIBTIFF
						#pragma comment(lib,"wxtiff_64")
					#endif
					#if wxUSE_ZLIB
						#pragma comment(lib,"wxzlib_64")
					#endif
					#pragma comment(lib,"wxmsw28u_adv_64")
					#pragma comment(lib,"wxmsw28u_core_64")
					#pragma comment(lib,"wxmsw28u_html_64")
					#if wxUSE_GLCANVAS
						#pragma comment(lib,"wxmsw28u_gl_64")
					#endif
					#if wxUSE_DEBUGREPORT
						#pragma comment(lib,"wxmsw28u_qa_64")
					#endif
					#if wxUSE_XRC
						#pragma comment(lib,"wxmsw28u_xrc_64")
					#endif
					#if wxUSE_AUI
						#pragma comment(lib,"wxmsw28u_aui_64")
					#endif
					#if wxUSE_RICHTEXT
						#pragma comment(lib,"wxmsw28u_richtext_64")
					#endif
					#if wxUSE_MEDIACTRL
						#pragma comment(lib,"wxmsw28u_media_64")
					#endif
					#if wxUSE_ODBC
						#pragma comment(lib,"wxbase28u_odbc_64")
					#endif
				#endif // wxUSE_GUI
			#endif // debug/release
		#else // !_UNICODE
			#ifdef WXUSINGDLL
				#ifdef _DEBUG
					#include "../../../lib/vc_dll/x64/mswd/wx/setup.h"
				#else
					#include "../../../lib/vc_dll/x64/msw/wx/setup.h"
				#endif
			#else // static lib
				#ifdef _DEBUG
					#include "../../../lib/vc_lib/x64/mswd/wx/setup.h"
				#else
					#include "../../../lib/vc_lib/x64/msw/wx/setup.h"
				#endif
			#endif // shared/static

			#ifdef _DEBUG
				#pragma comment(lib,"wxbase28d_64")
				#pragma comment(lib,"wxbase28d_net_64")
				#pragma comment(lib,"wxbase28d_xml_64")
				#if wxUSE_REGEX
					#pragma comment(lib,"wxregexd_64")
				#endif

				#if wxUSE_GUI
					#if wxUSE_XML
						#pragma comment(lib,"wxexpatd_64")
					#endif
					#if wxUSE_LIBJPEG
						#pragma comment(lib,"wxjpegd_64")
					#endif
					#if wxUSE_LIBPNG
						#pragma comment(lib,"wxpngd_64")
					#endif
					#if wxUSE_LIBTIFF
						#pragma comment(lib,"wxtiffd_64")
					#endif
					#if wxUSE_ZLIB
						#pragma comment(lib,"wxzlibd_64")
					#endif
					#pragma comment(lib,"wxmsw28d_adv_64")
					#pragma comment(lib,"wxmsw28d_core_64")
					#pragma comment(lib,"wxmsw28d_html_64")
					#if wxUSE_GLCANVAS
						#pragma comment(lib,"wxmsw28d_gl_64")
					#endif
					#if wxUSE_DEBUGREPORT
						#pragma comment(lib,"wxmsw28d_qa_64")
					#endif
					#if wxUSE_XRC
						#pragma comment(lib,"wxmsw28d_xrc_64")
					#endif
					#if wxUSE_AUI
						#pragma comment(lib,"wxmsw28d_aui_64")
					#endif
					#if wxUSE_RICHTEXT
						#pragma comment(lib,"wxmsw28d_richtext_64")
					#endif
					#if wxUSE_MEDIACTRL
						#pragma comment(lib,"wxmsw28d_media_64")
					#endif
					#if wxUSE_ODBC
						#pragma comment(lib,"wxbase28d_odbc_64")
					#endif
				#endif // wxUSE_GUI
			#else // release
				#pragma comment(lib,"wxbase28_64")
				#pragma comment(lib,"wxbase28_net_64")
				#pragma comment(lib,"wxbase28_xml_64")
				#if wxUSE_REGEX
					#pragma comment(lib,"wxregex_64")
				#endif

				#if wxUSE_GUI
					#if wxUSE_XML
						#pragma comment(lib,"wxexpat_64")
					#endif
					#if wxUSE_LIBJPEG
						#pragma comment(lib,"wxjpeg_64")
					#endif
					#if wxUSE_LIBPNG
						#pragma comment(lib,"wxpng_64")
					#endif
					#if wxUSE_LIBTIFF
						#pragma comment(lib,"wxtiff_64")
					#endif
					#if wxUSE_ZLIB
						#pragma comment(lib,"wxzlib_64")
					#endif
					#pragma comment(lib,"wxmsw28_adv_64")
					#pragma comment(lib,"wxmsw28_core_64")
					#pragma comment(lib,"wxmsw28_html_64")
					#if wxUSE_GLCANVAS
						#pragma comment(lib,"wxmsw28_gl_64")
					#endif
					#if wxUSE_DEBUGREPORT
						#pragma comment(lib,"wxmsw28_qa_64")
					#endif
					#if wxUSE_XRC
						#pragma comment(lib,"wxmsw28_xrc_64")
					#endif
					#if wxUSE_AUI
						#pragma comment(lib,"wxmsw28_aui_64")
					#endif
					#if wxUSE_RICHTEXT
						#pragma comment(lib,"wxmsw28_richtext_64")
					#endif
					#if wxUSE_MEDIACTRL
						#pragma comment(lib,"wxmsw28_media_64")
					#endif
					#if wxUSE_ODBC
						#pragma comment(lib,"wxbase28_odbc_64")
					#endif
				#endif // wxUSE_GUI
			#endif // debug/release
		#endif // _UNICODE/!_UNICODE
	
	#else	// WIN32
		#ifdef _UNICODE
			#ifdef WXUSINGDLL
				#ifdef _DEBUG
					#include "../../../lib/vc_dll/win32/mswud/wx/setup.h"
				#else
					#include "../../../lib/vc_dll/win32/mswu/wx/setup.h"
				#endif
			#else
				#ifdef _DEBUG
					#include "../../../lib/vc_lib/win32/mswud/wx/setup.h"
				#else
					#include "../../../lib/vc_lib/win32/mswu/wx/setup.h"
				#endif
			#endif

			#ifdef _DEBUG
				#pragma comment(lib,"wxbase28ud")
				#pragma comment(lib,"wxbase28ud_net")
				#pragma comment(lib,"wxbase28ud_xml")
				#if wxUSE_REGEX
					#pragma comment(lib,"wxregexud")
				#endif

				#if wxUSE_GUI
					#if wxUSE_XML
						#pragma comment(lib,"wxexpatd")
					#endif
					#if wxUSE_LIBJPEG
						#pragma comment(lib,"wxjpegd")
					#endif
					#if wxUSE_LIBPNG
						#pragma comment(lib,"wxpngd")
					#endif
					#if wxUSE_LIBTIFF
						#pragma comment(lib,"wxtiffd")
					#endif
					#if wxUSE_ZLIB
						#pragma comment(lib,"wxzlibd")
					#endif
					#pragma comment(lib,"wxmsw28ud_adv")
					#pragma comment(lib,"wxmsw28ud_core")
					#pragma comment(lib,"wxmsw28ud_html")
					#if wxUSE_GLCANVAS
						#pragma comment(lib,"wxmsw28ud_gl")
					#endif
					#if wxUSE_DEBUGREPORT
						#pragma comment(lib,"wxmsw28ud_qa")
					#endif
					#if wxUSE_XRC
						#pragma comment(lib,"wxmsw28ud_xrc")
					#endif
					#if wxUSE_AUI
						#pragma comment(lib,"wxmsw28ud_aui")
					#endif
					#if wxUSE_RICHTEXT
						#pragma comment(lib,"wxmsw28ud_richtext")
					#endif
					#if wxUSE_MEDIACTRL
						#pragma comment(lib,"wxmsw28ud_media")
					#endif
					#if wxUSE_ODBC
						#pragma comment(lib,"wxbase28ud_odbc")
					#endif
				#endif // wxUSE_GUI
			#else // release
				#pragma comment(lib,"wxbase28u")
				#pragma comment(lib,"wxbase28u_net")
				#pragma comment(lib,"wxbase28u_xml")
				#if wxUSE_REGEX
					#pragma comment(lib,"wxregexu")
				#endif

				#if wxUSE_GUI
					#if wxUSE_XML
						#pragma comment(lib,"wxexpat")
					#endif
					#if wxUSE_LIBJPEG
						#pragma comment(lib,"wxjpeg")
					#endif
					#if wxUSE_LIBPNG
						#pragma comment(lib,"wxpng")
					#endif
					#if wxUSE_LIBTIFF
						#pragma comment(lib,"wxtiff")
					#endif
					#if wxUSE_ZLIB
						#pragma comment(lib,"wxzlib")
					#endif
					#pragma comment(lib,"wxmsw28u_adv")
					#pragma comment(lib,"wxmsw28u_core")
					#pragma comment(lib,"wxmsw28u_html")
					#if wxUSE_GLCANVAS
						#pragma comment(lib,"wxmsw28u_gl")
					#endif
					#if wxUSE_DEBUGREPORT
						#pragma comment(lib,"wxmsw28u_qa")
					#endif
					#if wxUSE_XRC
						#pragma comment(lib,"wxmsw28u_xrc")
					#endif
					#if wxUSE_AUI
						#pragma comment(lib,"wxmsw28u_aui")
					#endif
					#if wxUSE_RICHTEXT
						#pragma comment(lib,"wxmsw28u_richtext")
					#endif
					#if wxUSE_MEDIACTRL
						#pragma comment(lib,"wxmsw28u_media")
					#endif
					#if wxUSE_ODBC
						#pragma comment(lib,"wxbase28u_odbc")
					#endif
				#endif // wxUSE_GUI
			#endif // debug/release
		#else // !_UNICODE
			#ifdef WXUSINGDLL
				#ifdef _DEBUG
					#include "../../../lib/vc_dll/win32/mswd/wx/setup.h"
				#else
					#include "../../../lib/vc_dll/win32/msw/wx/setup.h"
				#endif
			#else // static lib
				#ifdef _DEBUG
					#include "../../../lib/vc_lib/win32/mswd/wx/setup.h"
				#else
					#include "../../../lib/vc_lib/win32/msw/wx/setup.h"
				#endif
			#endif // shared/static

			#ifdef _DEBUG
				#pragma comment(lib,"wxbase28d")
				#pragma comment(lib,"wxbase28d_net")
				#pragma comment(lib,"wxbase28d_xml")
				#if wxUSE_REGEX
					#pragma comment(lib,"wxregexd")
				#endif

				#if wxUSE_GUI
					#if wxUSE_XML
						#pragma comment(lib,"wxexpatd")
					#endif
					#if wxUSE_LIBJPEG
						#pragma comment(lib,"wxjpegd")
					#endif
					#if wxUSE_LIBPNG
						#pragma comment(lib,"wxpngd")
					#endif
					#if wxUSE_LIBTIFF
						#pragma comment(lib,"wxtiffd")
					#endif
					#if wxUSE_ZLIB
						#pragma comment(lib,"wxzlibd")
					#endif
					#pragma comment(lib,"wxmsw28d_adv")
					#pragma comment(lib,"wxmsw28d_core")
					#pragma comment(lib,"wxmsw28d_html")
					#if wxUSE_GLCANVAS
						#pragma comment(lib,"wxmsw28d_gl")
					#endif
					#if wxUSE_DEBUGREPORT
						#pragma comment(lib,"wxmsw28d_qa")
					#endif
					#if wxUSE_XRC
						#pragma comment(lib,"wxmsw28d_xrc")
					#endif
					#if wxUSE_AUI
						#pragma comment(lib,"wxmsw28d_aui")
					#endif
					#if wxUSE_RICHTEXT
						#pragma comment(lib,"wxmsw28d_richtext")
					#endif
					#if wxUSE_MEDIACTRL
						#pragma comment(lib,"wxmsw28d_media")
					#endif
					#if wxUSE_ODBC
						#pragma comment(lib,"wxbase28d_odbc")
					#endif
				#endif // wxUSE_GUI
			#else // release
				#pragma comment(lib,"wxbase28")
				#pragma comment(lib,"wxbase28_net")
				#pragma comment(lib,"wxbase28_xml")
				#if wxUSE_REGEX
					#pragma comment(lib,"wxregex")
				#endif

				#if wxUSE_GUI
					#if wxUSE_XML
						#pragma comment(lib,"wxexpat")
					#endif
					#if wxUSE_LIBJPEG
						#pragma comment(lib,"wxjpeg")
					#endif
					#if wxUSE_LIBPNG
						#pragma comment(lib,"wxpng")
					#endif
					#if wxUSE_LIBTIFF
						#pragma comment(lib,"wxtiff")
					#endif
					#if wxUSE_ZLIB
						#pragma comment(lib,"wxzlib")
					#endif
					#pragma comment(lib,"wxmsw28_adv")
					#pragma comment(lib,"wxmsw28_core")
					#pragma comment(lib,"wxmsw28_html")
					#if wxUSE_GLCANVAS
						#pragma comment(lib,"wxmsw28_gl")
					#endif
					#if wxUSE_DEBUGREPORT
						#pragma comment(lib,"wxmsw28_qa")
					#endif
					#if wxUSE_XRC
						#pragma comment(lib,"wxmsw28_xrc")
					#endif
					#if wxUSE_AUI
						#pragma comment(lib,"wxmsw28_aui")
					#endif
					#if wxUSE_RICHTEXT
						#pragma comment(lib,"wxmsw28_richtext")
					#endif
					#if wxUSE_MEDIACTRL
						#pragma comment(lib,"wxmsw28_media")
					#endif
					#if wxUSE_ODBC
						#pragma comment(lib,"wxbase28_odbc")
					#endif
				#endif // wxUSE_GUI
			#endif // debug/release
		#endif // _UNICODE/!_UNICODE
	#endif // _WIN64/!_WIN64

#else
    #error "This file should only be included when using Microsoft Visual C++"
#endif

