/**********************************************************************

Filename    :   GFxPlayerStats.h
Content     :   Definitions of GFxPlayer Stat constants.
Created     :   June 21, 2008
Authors     :   Michael Antonov

Notes       :

Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXPLAYERSTATS_H
#define INC_GFXPLAYERSTATS_H

#include "GConfig.h"
#include "GTypes.h"
#include "GStats.h"


// ***** Movie Statistics Identifiers

// Specific statistics.
enum GFxStatMovieData
{
    GFxStatMD_Default = GStatGroup_GFxMovieData,

    // Memory for a movie.
    GFxStatMD_Mem,
      GFxStatMD_CharDefs_Mem,
      GFxStatMD_ShapeData_Mem,
      GFxStatMD_Tags_Mem,
      GFxStatMD_Fonts_Mem,
      GFxStatMD_Images_Mem,
#ifndef GFC_NO_SOUND
      GFxStatMD_Sounds_Mem,
#endif
      GFxStatMD_ActionOps_Mem,
      GFxStatMD_Other_Mem,

    // Different
    GFxStatMD_Time,
      GFxStatMD_Load_Tks,
      GFxStatMD_Bind_Tks
};

enum GFxStatMovieView
{
    GFxStatMV_Default = GStatGroup_GFxMovieView,

    // MovieView memory.
    GFxStatMV_Mem,
      GFxStatMV_MovieClip_Mem,
      GFxStatMV_ActionScript_Mem,
      GFxStatMV_Text_Mem,
      GFxStatMV_XML_Mem,
      GFxStatMV_Other_Mem,

    // MovieView Timings.
    GFxStatMV_Tks,
      GFxStatMV_Advance_Tks,
        GFxStatMV_Action_Tks,
        GFxStatMV_Timeline_Tks,
        GFxStatMV_Input_Tks,
          GFxStatMV_Mouse_Tks,
      GFxStatMV_ScriptCommunication_Tks,
          GFxStatMV_GetVariable_Tks,
          GFxStatMV_SetVariable_Tks,
          GFxStatMV_Invoke_Tks,
      GFxStatMV_Display_Tks,
        GFxStatMV_Tessellate_Tks,
        GFxStatMV_GradientGen_Tks,
};


enum GFxStatIME
{
    GFxStatIME_Default = GStatGroup_GFxIME,

    // IME memory.
    GFxStatIME_Mem
};


enum GFxStatFontCache
{
    GFxStatFC_Default = GStatGroup_GFxFontCache,

    // Font Cache memory.
    GFxStatFC_Mem,
        GFxStatFC_Batch_Mem,
        GFxStatFC_GlyphCache_Mem,
        GFxStatFC_Other_Mem
};


#endif // ! INC_GFXPLAYERSTATS_H
