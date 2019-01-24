/**********************************************************************

Filename    :   GFxUIRendererRT.h
Content     :   

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Portions of the integration code is from Epic Games as identified by Perforce annotations.
Copyright 2010 Epic Games, Inc. All rights reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#define FGFxTexture             FGFxTextureRT
#define FGFxTextureYUV          FGFxTextureYUVRT
#define FGFxRenderTarget        FGFxRenderTargetRT
#define FGFxRenderer            FGFxRendererRT
#define FGFxRendererImpl        FGFxRendererImplRT
#define SetUIRenderElementStore SetUIRenderElementStoreRT
#define SetUITransformMatrix    SetUITransformMatrixRT

#ifndef GFxRendererRT_h
#define GFxRendererRT_h

#undef GFxUIRenderer_h
#include "GFxUIRenderer.h"

#endif

#ifdef GFXUIRENDERER_ONCE
#include "GFxUIRenderer.cpp"
#undef GFXUIRENDERER_ONCE
#endif

#undef FGFxTexture
#undef FGFxTextureYUV
#undef FGFxRenderTarget
#undef FGFxRenderer
#undef FGFxRendererImpl
#undef SetUIRenderElementStore
#undef SetUITransformMatrix

