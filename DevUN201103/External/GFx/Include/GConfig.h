/**********************************************************************

Filename    :   GConfig.h
Content     :   GFx configuration file - contains #ifdefs for
                the optional components of the library
Created     :   June 15, 2005
Authors     :   Michael Antonov

Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GCONFIG_H
#define INC_GCONFIG_H

#include "GTypes.h"     // so that OS defines are set

// Determine if we are using debug configuration. This needs to be done
// here because some of the later #defines rely on it.
#if !defined(GFC_BUILD_DEBUG) && !defined(GFC_BUILD_RELEASE)
# if defined(_DEBUG) || defined(_DEBUGOPT)
#   define GFC_BUILD_DEBUG
# endif
#endif

#ifndef GFC_BUILD_DEBUGOPT
# ifdef _DEBUGOPT
#   define GFC_BUILD_DEBUGOPT
# endif
#endif


// Allows developers to replace the math.h
//#define GFC_MATH_H  <math.h>

// Enable/disable addons for Msvc builds
#if defined(GFC_OS_WIN32) || defined(GFC_OS_XBOX360)
    #include "GConfigAddons.h"

    #if defined(GFC_OS_WIN32) && defined(GFC_USE_VIDEO_WIN32)
        #define GFC_USE_VIDEO
    #elif defined(GFC_OS_XBOX360) && defined(GFC_USE_VIDEO_XBOX360)
        #define GFC_USE_VIDEO
    #endif
#endif


#define GFC_MAX_MICE_SUPPORTED          4
#define GFC_MAX_KEYBOARD_SUPPORTED      4


// #define  GFC_NO_THREADSUPPORT

// Define this to disable statistics tracking; this is useful for the final build.
//#define GFC_NO_STAT

// Define this to disable 3D flash support
//#define GFC_NO_3D

// This macro needs to be defined if it is necessary to avoid the use of Double.
// In that case Double in defined as Float and thus extra #ifdef checks on
// overloads need to be done.
// NOTE: By default, PS2 and PSP define this if not manually defined here.
//#define GFC_NO_DOUBLE

// Undefine this macro to disable use of LIBJPEG and make JPEGUtil a no-op stub.
// If disabled, SWF JPEG image loading will stop functioning.
// - saves roughly 60k in release build.
//#define GFC_USE_LIBJPEG

// Define this macro if whole JPEGLIB is compiled as C++ code. By default libjpeg
// is pure C library and public names are not mangled. Though, it might be
// necessary to mangle jpeglib's names in order to resolve names clashing issues
// (for example, with XBox360's xmedia.lib).
//#define GFC_CPP_LIBJPEG

// Undefine this macro to disable use of ZLIB and comment out GZLibFile class.
// If disabled, compressed SWF files will no longer load.
// - saves roughly 20k in release build.
#define GFC_USE_ZLIB

// Define this macro to enable use of LIBPNG.
// If disabled, SWF PNG image loading is not functioning.
// Note: beside the uncommenting the line below, it is necessary
// to provide the path and the library name for the linker.
// The name of the library is libpng.lib for all configurations;
// the path is different. For example, VC8, 64-bit, Release Static
// library is located at %(GFX)\3rdParty\libpng\lib\x64\msvc80\release_mt_static
#if defined(GFC_OS_WIN32)
	#define GFC_USE_LIBPNG
#endif


// ***** Memory/Allocation Configuration

// These defines control memory tracking used for statistics and leak detection.
// By default, memory debug functionality is enabled automatically by GFC_BUILD_DEBUG.
#ifdef GFC_BUILD_DEBUG
    // Enable debug memory tracking. This passes __FILE__ and __LINE__ data into
    // allocations and enabled DebugStorage in the memory heap used to track
    // statistics and memory leaks.
    #define GHEAP_DEBUG_INFO

    // Wipe memory in the allocator; this is useful for debugging.
    #define GHEAP_MEMSET_ALL

    // Check for corrupted memory in the free chunks. May significantly
    // slow down the allocations.
    //#define GHEAP_CHECK_CORRUPTION
#endif

#ifdef WITH_AMP
	#define GFX_AMP_SERVER
#endif

// Enable remote memory and performance profiling
#if defined(GFC_BUILD_DEBUG) && !defined(GFC_NO_THREADSUPPORT)

	//Disabling by default so "RELEASE" configs don't suffer from heap debug performance issues
	// Enable debug memory tracking when AMP is enabled
	#define GHEAP_DEBUG_INFO
#endif

// Use for extreme debug only! This define enforces the use of system 
// memory allocator as much as possible. That is: no granulator; absolute 
// minimal heap and system granularities; no reserve; no segment caching.
// It increases the chances of early memory corruption detection on 
// segmentation fault. Works only with GSysAllocWinAPI and GSysAllocMMAP.
//#define GHEAP_FORCE_SYSALLOC

// Use for extreme debug only! Enforces the use of _aligned_malloc/_aligned_free
// No memory heaps functionality, no statistics, no heap memory limits.
//#define GHEAP_FORCE_MALLOC

// Trace all essential operations: CreateHeap, DestroyHeap, Alloc, Free, Realloc.
// See GMemoryHeap::HeapTracer for details.
//#define GHEAP_TRACE_ALL

// ***** GFx Specific options

// If this macro is defined GFx will not use the stroker to render lines;
// - saves roughly 20K in release build.
// Note, the stroker is required to produce faux bold glyphs. If it is disabled then
// faux bold fonts will not be generated.
//#define GFC_NO_FXPLAYER_STROKER

// If this macro is defined GFx will not use the strokerAA to render lines;
//#define GFC_NO_FXPLAYER_STROKERAA

// If this macro is defined GFx will not include EdgeAA support;
// - saves roughly 30K in release build.
//#define GFC_NO_FXPLAYER_EDGEAA


// Default mouse support enable state for GFx. Enabling mouse is also
// dynamically controlled by GFxMovieView::EnableMouseSupport().
#if defined(GFC_OS_XBOX) || defined(GFC_OS_XBOX360) || \
    defined(GFC_OS_PSP) || defined(GFC_OS_PS2) || defined(GFC_OS_PS3) || \
    defined(GFC_OS_GAMECUBE)
#define GFC_MOUSE_SUPPORT_ENABLED   0
#else
#define GFC_MOUSE_SUPPORT_ENABLED   1
#endif

// *** Logging options

// Define this macro to enable verbose logging of the CLIK_loadCallback / CLIK_unloadCallback
#define UE3_FXPLAYER_LOADER_LOGS

// Define this macro to eliminate all support for verbose parsing of input files.
// If this option is set, none of verbose parse options are available, and the
// GFxLoader::SetVerboseParse call will have no effect.
// Game production release builds should probably define this option.
#if !defined(GFC_OS_WIN32)
	#define GFC_NO_FXPLAYER_VERBOSE_PARSE
#endif

#ifdef GFC_NO_FXPLAYER_VERBOSE_PARSE
    #define GFC_NO_FXPLAYER_VERBOSE_PARSE_ACTION
    #define GFC_NO_FXPLAYER_VERBOSE_PARSE_SHAPE
    #define GFC_NO_FXPLAYER_VERBOSE_PARSE_MORPHSHAPE
#else

    // Define this macro to eliminate all support for verbose parsing of actions
    // (disables support for disassembly during loading).
    //#define GFC_NO_FXPLAYER_VERBOSE_PARSE_ACTION

    // Define this macro to eliminate all support for verbose parsing
    // of shape character structures.
    //#define GFC_NO_FXPLAYER_VERBOSE_PARSE_SHAPE

    // Define this macro to eliminate all support for verbose parsing
    // of morph shape character structures.
    //#define GFC_NO_FXPLAYER_VERBOSE_PARSE_MORPHSHAPE

#endif


// Define this macro to eliminate custom wctype tables for functions
// like G_iswspace, G_towlower, g_towupper and so on. If this macro
// is defined GFx will use system Unicode functions (which are incredibly
// slow on Microsoft Windows).
//#define GFC_NO_WCTYPE

// Define this macro to eliminate support for verbose logging of executed ActionScript opcodes.
// If this macro is defined, GFxMovie::SetVerboseAction will have no effect.
// Game production release builds should probably define this option.
#if !defined(GFC_OS_WIN32)
	#define GFC_NO_FXPLAYER_VERBOSE_ACTION
#endif

#if !defined(GFC_BUILD_DEBUG) && !defined(GFC_BUILD_DEBUGOPT) && !defined(GFC_NO_FXPLAYER_VERBOSE_ACTION)
    // Turn verbose action OFF for Release builds
    #define GFC_NO_FXPLAYER_VERBOSE_ACTION
#endif

// Define this macro to eliminate support for verbose logging of ActionScript run-time errors.
// If this macro is defined, GFxMovie::SetVerboseActionErrors will have no effect.
// Game production release builds should probably define this option.
#if !defined(GFC_OS_WIN32)
	#define GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
#endif

// Define this macro to throw assertion if any font texture is generated during
// the runtime.
//#define GFC_ASSERT_ON_FONT_BITMAP_GEN

// Define this macro to throw assertion if any gradient texture is generated during
// the runtime.
//#define GFC_ASSERT_ON_GRADIENT_BITMAP_GEN

// Define this macro to throw assertion if any re-sampling occurred in renderer during
// the runtime.
//#define GFC_ASSERT_ON_RENDERER_RESAMPLING

// Define this macro to throw assertion if any mipmap levels generation occurred in
// renderer during the runtime.
//#define GFC_ASSERT_ON_RENDERER_MIPMAP_GEN

// Define this macro to exclude FontGlyphPacker
#if !defined(GFC_OS_WIN32)
	#define GFC_NO_FONT_GLYPH_PACKER
#endif

// Define this macro to exclude the dynamic glyph cache
//#define GFC_NO_GLYPH_CACHE

// Define this macro to exclude gradient generation
//#define GFC_NO_GRADIENT_GEN

// Define this macro to exclude sound support (including core and ActionScript)
#define GFC_NO_SOUND

// Define this macro to exclude video support (including core and ActionScript)
#define GFC_NO_VIDEO

#ifdef GFC_NO_THREADSUPPORT
    // Video can be used only with multithreading support
    #define GFC_NO_VIDEO
#endif // GFC_NO_THREADSUPPORT

// Disable core CSS support
//#define GFC_NO_CSS_SUPPORT

// Disable core XML support
#define GFC_NO_XML_SUPPORT

// Enable IME support only on PC platform
#if !defined(GFC_OS_WIN32)
	#define GFC_NO_IME_SUPPORT
#endif

// Disable built-in core Korean IME logic
#define GFC_NO_BUILTIN_KOREAN_IME

#ifdef GFC_NO_IME_SUPPORT
    #ifdef GFC_USE_IME
        #undef GFC_USE_IME
    #endif // GFC_USE_IME
#endif // GFC_NO_IME_SUPPORT

// Disable garbage collection
//#define GFC_NO_GC

// *** Disabling ActionScript Options

// Disable ActionScript object user data storage (GFxValue::SetUserData/GetUserData)
//#define GFC_NO_FXPLAYER_AS_USERDATA

// Disable *Filter class support.
//#define GFC_NO_FXPLAYER_AS_FILTERS

// Disable 'Date' ActionScript class support.
//#define GFC_NO_FXPLAYER_AS_DATE

// Disable 'Point' ActionScript class support.
//#define GFC_NO_FXPLAYER_AS_POINT

// Disable 'Rectangle' ActionScript class support
//#define GFC_NO_FXPLAYER_AS_RECTANGLE

// Disable 'Transform' ActionScript class support
//#define GFC_NO_FXPLAYER_AS_TRANSFORM

// Disable 'ColorTransform' ActionScript class support
//#define GFC_NO_FXPLAYER_AS_COLORTRANSFORM

// Disable 'Matrix' ActionScript class support
//#define GFC_NO_FXPLAYER_AS_MATRIX

// Disable 'TextSnapshot' class support
//#define GFC_NO_FXPLAYER_AS_TEXTSNAPSHOT

// Disable 'SharedObject' class support
//#define GFC_NO_FXPLAYER_AS_SHAREDOBJECT

// Disable 'MovieClipLoader' ActionScript class support
//#define GFC_NO_FXPLAYER_AS_MOVIECLIPLOADER

// Disable 'LoadVars' ActionScript class support
//#define GFC_NO_FXPLAYER_AS_LOADVARS

// Disable 'BitmapData' ActionScript class support. Note, if BitmapData is disabled then textfield
// does not support <IMG> HTML tags and image substitutions.
//#define GFC_NO_FXPLAYER_AS_BITMAPDATA

// Disable 'System.capabilites' ActionScript class support
//#define GFC_NO_FXPLAYER_AS_CAPABILITES

// Disable 'Color' ActionScript class support
//#define GFC_NO_FXPLAYER_AS_COLOR

// Disable 'TextFormat' ActionScript class support
//#define GFC_NO_FXPLAYER_AS_TEXTFORMAT

// Disable 'Selection' ActionScript class support.
//#define GFC_NO_FXPLAYER_AS_SELECTION

// Disable 'Stage' ActionScript class support. Stage.height and Stage.width will not be supported as well.
//#define GFC_NO_FXPLAYER_AS_STAGE

// Disable 'Mouse' ActionScript class support. 
//#define GFC_NO_FXPLAYER_AS_MOUSE

// Disable DrawText API
// NOTE: When compiling libs for GFxMediaPlayer we want this to be enabled as some of their GUI uses it
#define GFC_NO_DRAWTEXT_SUPPORT

// Disable keyboard support. No Key AS class will be provided, HandleEvent with
// GFxKeyEvent will not be supported, PAD keys on consoles will not work.
//#define GFC_NO_KEYBOARD_SUPPORT

// Disables mouse support completely. This option also disables Mouse AS class.
//#define GFC_NO_MOUSE_SUPPORT

// Disable font compactor (compaction during the run-time). Fonts compacted
// by the gfxexport (with option -fc) will be working.
#if !defined(GFC_OS_WIN32)
	#define GFC_NO_FONTCOMPACTOR_SUPPORT
#endif

// Disable usage of compacted fonts (fonts, compacted by gfxexport (option -fc))
//#define GFC_NO_COMPACTED_FONT_SUPPORT

// Disable TextField ActionScript extension functions. If this option is used
// then standard GFxPlayer's HUD will not work.
//#define GFC_NO_TEXTFIELD_EXTENSIONS

// Disable text editing. Text selection will be disabled as well (since it
// is a part of text editing).
//#define GFC_NO_TEXT_INPUT_SUPPORT

// Disable morphing (shape tween) support
//#define GFC_NO_MORPHING_SUPPORT

// *** Example defines for shipping (final) GFX build

#ifdef GFC_BUILD_SHIPPING
    #define GFC_NO_STAT 
    #define GFC_NO_FXPLAYER_VERBOSE_PARSE 
    #define GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS 
#endif

// *** Example defines for lightweight GFX on small consoles

#ifdef GFC_BUILD_LITE
    #undef GFC_USE_LIBJPEG
    #undef GFC_USE_ZLIB
    #undef GFC_USE_LIBPNG

    #define GFC_NO_STAT

    #define GFC_NO_FXPLAYER_VERBOSE_PARSE
    #define GFC_NO_FXPLAYER_VERBOSE_PARSE_ACTION
    #define GFC_NO_FXPLAYER_VERBOSE_PARSE_SHAPE
    #define GFC_NO_FXPLAYER_VERBOSE_PARSE_MORPHSHAPE
    #define GFC_NO_FXPLAYER_VERBOSE_ACTION
    #define GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS

    #define GFC_NO_FONT_GLYPH_PACKER

    #define GFC_NO_GRADIENT_GEN

    //#ifdef GFC_OS_PSP
        #define GFC_NO_VIDEO
    //#endif

    #define GFC_NO_CSS_SUPPORT
    #define GFC_NO_XML_SUPPORT
    #define GFC_NO_IME_SUPPORT
    #define GFC_NO_BUILTIN_KOREAN_IME

    #ifdef GFC_NO_IME_SUPPORT
    #ifdef GFC_USE_IME
    #undef GFC_USE_IME
    #endif // GFC_USE_IME
    #endif // GFC_NO_IME_SUPPORT

    // *** Disabling ActionScript Options
    #define GFC_NO_FXPLAYER_AS_USERDATA
    #define GFC_NO_FXPLAYER_AS_FILTERS
    #define GFC_NO_FXPLAYER_AS_DATE
    #define GFC_NO_FXPLAYER_AS_POINT
    #define GFC_NO_FXPLAYER_AS_RECTANGLE
    #define GFC_NO_FXPLAYER_AS_TRANSFORM
    #define GFC_NO_FXPLAYER_AS_COLORTRANSFORM
    #define GFC_NO_FXPLAYER_AS_MATRIX
    #define GFC_NO_FXPLAYER_AS_TEXTSNAPSHOT
    #define GFC_NO_FXPLAYER_AS_SHAREDOBJECT
    #define GFC_NO_FXPLAYER_AS_MOVIECLIPLOADER
    #define GFC_NO_FXPLAYER_AS_LOADVARS
    #define GFC_NO_FXPLAYER_AS_BITMAPDATA
    #define GFC_NO_FXPLAYER_AS_CAPABILITES
    //#define GFC_NO_FXPLAYER_AS_COLOR
    #define GFC_NO_FXPLAYER_AS_TEXTFORMAT
    #define GFC_NO_FXPLAYER_AS_SELECTION
    //#define GFC_NO_FXPLAYER_AS_STAGE
    #define GFC_NO_DRAWTEXT_SUPPORT

    #define GFC_NO_FONTCOMPACTOR_SUPPORT
    #define GFC_NO_TEXTFIELD_EXTENSIONS

    // no text input support on consoles
    #if defined(GFC_OS_XBOX) || defined(GFC_OS_XBOX360) || \
        defined(GFC_OS_PSP) || defined(GFC_OS_PS2) || defined(GFC_OS_PS3) || \
        defined(GFC_OS_GAMECUBE) || defined(GFC_OS_WII)
        #define GFC_NO_TEXT_INPUT_SUPPORT
    #endif

    #if defined(GFC_OS_XBOX) || defined(GFC_OS_XBOX360) || \
        defined(GFC_OS_PSP) || defined(GFC_OS_PS2) || defined(GFC_OS_PS3) || \
        defined(GFC_OS_GAMECUBE)
        #define GFC_NO_MOUSE_SUPPORT
    #endif

#endif

//
// Define the standard set of functionality for full builds (used by gfxexport)
//
#ifdef GFC_BUILD_FULL

    #define GFC_USE_LIBJPEG
    #define GFC_USE_ZLIB
    #define GFC_USE_LIBPNG

    #undef GFC_NO_3D
    #undef GFC_NO_BUILTIN_KOREAN_IME
    #undef GFC_NO_COMPACTED_FONT_SUPPORT
    #undef GFC_NO_CSS_SUPPORT
    #undef GFC_NO_DOUBLE
    #undef GFC_NO_DRAWTEXT_SUPPORT
    #undef GFC_NO_FONT_GLYPH_PACKER
    #undef GFC_NO_FONTCOMPACTOR_SUPPORT
    #undef GFC_NO_FXPLAYER_AS_BITMAPDATA
    #undef GFC_NO_FXPLAYER_AS_CAPABILITES
    #undef GFC_NO_FXPLAYER_AS_COLOR
    #undef GFC_NO_FXPLAYER_AS_COLORTRANSFORM
    #undef GFC_NO_FXPLAYER_AS_DATE
    #undef GFC_NO_FXPLAYER_AS_FILTERS
    #undef GFC_NO_FXPLAYER_AS_LOADVARS
    #undef GFC_NO_FXPLAYER_AS_MATRIX
    #undef GFC_NO_FXPLAYER_AS_MOUSE
    #undef GFC_NO_FXPLAYER_AS_MOVIECLIPLOADER
    #undef GFC_NO_FXPLAYER_AS_POINT
    #undef GFC_NO_FXPLAYER_AS_RECTANGLE
    #undef GFC_NO_FXPLAYER_AS_SELECTION
    #undef GFC_NO_FXPLAYER_AS_SHAREDOBJECT
    #undef GFC_NO_FXPLAYER_AS_STAGE
    #undef GFC_NO_FXPLAYER_AS_TEXTFORMAT
    #undef GFC_NO_FXPLAYER_AS_TEXTSNAPSHOT
    #undef GFC_NO_FXPLAYER_AS_TRANSFORM
    #undef GFC_NO_FXPLAYER_AS_USERDATA
    #undef GFC_NO_FXPLAYER_EDGEAA
    #undef GFC_NO_FXPLAYER_STROKER
    #undef GFC_NO_FXPLAYER_STROKERAA
    #undef GFC_NO_FXPLAYER_VERBOSE_ACTION
    #undef GFC_NO_FXPLAYER_VERBOSE_ACTION_ERRORS
    #undef GFC_NO_FXPLAYER_VERBOSE_PARSE 
    #undef GFC_NO_FXPLAYER_VERBOSE_PARSE_ACTION
    #undef GFC_NO_FXPLAYER_VERBOSE_PARSE_MORPHSHAPE
    #undef GFC_NO_FXPLAYER_VERBOSE_PARSE_SHAPE
    #undef GFC_NO_GC
    #undef GFC_NO_GLYPH_CACHE
    #undef GFC_NO_GRADIENT_GEN
    #undef GFC_NO_IME_SUPPORT
    #undef GFC_NO_KEYBOARD_SUPPORT
    #undef GFC_NO_MORPHING_SUPPORT
    #undef GFC_NO_MOUSE_SUPPORT
    #undef GFC_NO_SOUND
    #undef GFC_NO_STAT 
    #undef GFC_NO_TEXT_INPUT_SUPPORT
    #undef GFC_NO_TEXTFIELD_EXTENSIONS
    #undef GFC_NO_THREADSUPPORT
    #undef GFC_NO_VIDEO
    #undef GFC_NO_WCTYPE
    #undef GFC_NO_XML_SUPPORT

#endif

// *** Automatic class dependency checks (DO NOT EDIT THE CODE BELOW)

//It might be possible to have limited IME functionality, so it should be investigated 
#if  defined(GFC_NO_FXPLAYER_AS_MOVIECLIPLOADER)
#define GFC_NO_IME_SUPPORT
#ifdef GFC_USE_IME
    #undef GFC_USE_IME
#endif // GFC_USE_IME
#endif 


#if defined(GFC_NO_MOUSE_SUPPORT) && !defined(GFC_NO_FXPLAYER_AS_MOUSE)
    #define GFC_NO_FXPLAYER_AS_MOUSE
#endif
    
#ifdef GFC_NO_FXPLAYER_AS_POINT
    // GASRectangle is heavily dependent on GASPoint
    #define GFC_NO_FXPLAYER_AS_RECTANGLE
#else
    #ifdef GFC_NO_FXPLAYER_AS_RECTANGLE
        #ifdef GFC_NO_FXPLAYER_AS_MATRIX
            // GASPoint is useless without GASRectangle or GASMatrix
            #define GFC_NO_FXPLAYER_AS_POINT
        #endif
    #endif
#endif


#ifdef GFC_NO_FXPLAYER_AS_TRANSFORM
    // GASColorTransform is useless without GASTransform
    #define GFC_NO_FXPLAYER_AS_COLORTRANSFORM
#endif

#ifdef GFC_NO_FXPLAYER_AS_MATRIX
    #ifdef GFC_NO_FXPLAYER_AS_RECTANGLE
        #ifdef GFC_NO_FXPLAYER_AS_COLORTRANSFORM
            // GASTransform is useless without GASMatrix, GASTransform, GASColorTransform
            #define GFC_NO_FXPLAYER_AS_TRANSFORM
        #endif
    #endif
#endif

#endif
