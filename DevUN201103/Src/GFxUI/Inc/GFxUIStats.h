/**********************************************************************

Filename    :   GFxUIStats.h
Content     :   Declarations for GFx statistics tracking

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Portions of the integration code is from Epic Games as identified by Perforce annotations.
Copyright 2010 Epic Games, Inc. All rights reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef GFxUIStats_h
#define GFxUIStats_h

#if WITH_GFx

/*!
Stat group for Scaleform GFx
*/
enum EScaleformStatGroup
{
	// Amount of time it takes to render the UI on the game thread
	STAT_GFxRenderUIGame = STAT_GFxFirstStat,
    // Amount of time it takes to render the RTT  on the game thread
    STAT_GFxRenderTexturesGame,
    // Amount of time it takes to render the UI on the game thread
    STAT_GFxRenderUIRT,
    // Amount of time it takes to render the RTT  on the game thread
    STAT_GFxRenderTexturesRT,

    // Amount of time in Tick for HUD movies
    STAT_GFxTickUI,
    // Amount of time in Tick for RTT movies
    STAT_GFxTickRTT,

    // Amount of memory that Scaleform allocates outside of the normal Unreal Object tree.
	STAT_GFxInternalMem,
	// Amount of memory peak (per frame) that Scaleform has allocated
	STAT_GFxFramePeakMem,
	// Total estimate of memory used by Scaleform
	STAT_GFxTotalMem,

    // Normal Textures - do not reorder the texture stats
    STAT_GFxTextureCount,
    STAT_GFxTextureMem,

    // Updatable Textures (dynamic font cache)
    STAT_GFxFontTextureCount,
    STAT_GFxFontTextureMem,

    // Video Textures
    STAT_GFxVideoTextureCount,
    STAT_GFxVideoTextureMem,

    // Internal Render target Textures
    STAT_GFxRTTextureCount,
    STAT_GFxRTTextureMem,

    // Number of lines drawn
	STAT_GFxLinesDrawn,
	// Number of Triangles
	STAT_GFxTrianglesDrawn,
	// Number of objects in GC array
	STAT_GFxGCManagedCount
};

#endif // WITH_GFx

#endif // GFxUIStats_h
