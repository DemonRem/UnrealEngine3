/**********************************************************************

Filename    :   GFxUIStats.cpp
Content     :   Source declarations for GFx statistics tracking

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Portions of the integration code is from Epic Games as identified by Perforce annotations.
Copyright 2010 Epic Games, Inc. All rights reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxUI.h"

#if WITH_GFx

#include "GFxUIStats.h"

DECLARE_STATS_GROUP(TEXT("GFx"), STATGROUP_Scaleform);

// Amount of time it takes to make the RenderUI call on Scaleform GFx
DECLARE_CYCLE_STAT(TEXT("RenderUI Game"), STAT_GFxRenderUIGame, STATGROUP_Scaleform);
DECLARE_CYCLE_STAT(TEXT("RenderUI RT"), STAT_GFxRenderUIRT, STATGROUP_Scaleform);

// Amount of time it takes to make the RenderTextures call
DECLARE_CYCLE_STAT(TEXT("RenderTextures Game"), STAT_GFxRenderTexturesGame, STATGROUP_Scaleform);
DECLARE_CYCLE_STAT(TEXT("RenderTextures RT"), STAT_GFxRenderTexturesRT, STATGROUP_Scaleform);

// Amount of time in Tick for HUD movies
DECLARE_CYCLE_STAT(TEXT("Total Tick UI"), STAT_GFxTickUI, STATGROUP_Scaleform);

// Amount of time in Tick for RTT movies
DECLARE_CYCLE_STAT(TEXT("Total Tick Textures"), STAT_GFxTickRTT, STATGROUP_Scaleform);

// Amount of memory used internally by Scaleform GFx
DECLARE_MEMORY_STAT(TEXT("Internal Memory"), STAT_GFxInternalMem, STATGROUP_Scaleform);

// Amount of memory used (total) by Scaleform GFx
DECLARE_MEMORY_STAT(TEXT("Peak Internal Mem This Frame"), STAT_GFxFramePeakMem, STATGROUP_Scaleform);

// Amount of memory used (total) by Scaleform GFx
DECLARE_MEMORY_STAT(TEXT("Total Memory"), STAT_GFxTotalMem, STATGROUP_Scaleform);

// Number of lines drawn this cycle
DECLARE_DWORD_COUNTER_STAT(TEXT("# of Lines Drawn"), STAT_GFxLinesDrawn, STATGROUP_Scaleform);

// Number of lines drawn this cycle
DECLARE_DWORD_COUNTER_STAT(TEXT("# of Triangles Drawn"), STAT_GFxTrianglesDrawn, STATGROUP_Scaleform);

// Number of objects of unknown type managed by Scaleform GFx
DECLARE_DWORD_COUNTER_STAT(TEXT("GC Managed Objects"), STAT_GFxGCManagedCount, STATGROUP_Scaleform);

// Normal textures
DECLARE_DWORD_COUNTER_STAT(TEXT("Other Textures"), STAT_GFxTextureCount, STATGROUP_Scaleform);
DECLARE_MEMORY_STAT(TEXT("Other Texture Memory"), STAT_GFxTextureMem, STATGROUP_Scaleform);

DECLARE_DWORD_COUNTER_STAT(TEXT("Font Textures"), STAT_GFxFontTextureCount, STATGROUP_Scaleform);
DECLARE_MEMORY_STAT(TEXT("Font Texture Memory"), STAT_GFxFontTextureMem, STATGROUP_Scaleform);

DECLARE_DWORD_COUNTER_STAT(TEXT("Video Textures"), STAT_GFxVideoTextureCount, STATGROUP_Scaleform);
DECLARE_MEMORY_STAT(TEXT("Video Texture Memory"), STAT_GFxVideoTextureMem, STATGROUP_Scaleform);

DECLARE_DWORD_COUNTER_STAT(TEXT("Render Targets"), STAT_GFxRTTextureCount, STATGROUP_Scaleform);
DECLARE_MEMORY_STAT(TEXT("Render Target Memory"), STAT_GFxRTTextureMem, STATGROUP_Scaleform);


// Force linker to include the above DECLARE constructors by calling this function
// externally from GFxUI.cpp.
void GFX_InitUnStats() { }

// MA: Needed !?
#include "GFxUIClasses.h"

#endif // WITH_GFx