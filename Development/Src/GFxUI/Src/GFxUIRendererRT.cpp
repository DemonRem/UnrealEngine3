/**********************************************************************

Filename    :   GFxUIRendererRT.cpp
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

#include "GFxUI.h"

#if WITH_GFx

#include "GFxUIEngine.h"
#include "GFxUIRenderer.h"
#include "GFxUIRendererRT.h"
#include "GFxUIStats.h"

#define GFX_ENQUEUE_RENDER_COMMAND(TypeName,Params) \
{   \
	TypeName RenderCmd Params;  \
	RenderCmd.Execute();    \
}

#define GFX_ENQUEUE_UNIQUE_RENDER_COMMAND(TypeName,Code) { Code; }
#define GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(TypeName,ParamType1,ParamName1,ParamValue1,Code) \
{   \
	ParamType1 ParamName1 = ParamValue1;    \
	Code;   \
}
#define GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(TypeName,ParamType1,ParamName1,ParamValue1,ParamType2,ParamName2,ParamValue2,Code) \
{   \
	ParamType1 ParamName1 = ParamValue1;    \
	ParamType2 ParamName2 = ParamValue2;    \
	Code;   \
}
#define GFX_ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(TypeName,ParamType1,ParamName1,ParamValue1,ParamType2,ParamName2,ParamValue2,ParamType3,ParamName3,ParamValue3,Code) \
{   \
	ParamType1 ParamName1 = ParamValue1;    \
	ParamType2 ParamName2 = ParamValue2;    \
	ParamType3 ParamName3 = ParamValue3;    \
	Code;   \
}

namespace FGFxRendererImplRT
{
	UTexture2D* CreateNativeTexture(GRenderer* pRenderer, INT Type, INT StatId)
	{
		if (IsInGameThread())
			return FGFxRendererImpl::CreateNativeTexture(pRenderer,Type,StatId);

		GLock::Locker lock (&((FGFxRendererRT*)pRenderer)->RTTexturesLock);
		TArray<UTexture2D*> *pTextures = &((FGFxRendererRT*)pRenderer)->RTAllocTextures[Type];

		if (pTextures->Num() == 1)
		{
			debugf(TEXT("Not enough preallocated render thread texture objects. Texture corruption will occur."));
			UTexture2D* pTexture = (*pTextures)(0);
			pTexture->Mips.Empty();
			return pTexture;
		}
		return pTextures->Pop();
	}

	void        TermGCState(GRenderer* pRendererIn, UTexture* pTextureIn)
	{
		UTexture2D* pTexture = Cast<UTexture2D>(pTextureIn);
		if (!pTexture)
			return; // RTT, not supported in Background Movie
		if (pTexture->GetOuter() != UObject::GetTransientPackage())
		{
			return;
		}

		FGFxRendererRT* pRenderer = (FGFxRendererRT*)pRendererIn;
		GLock::Locker lock (&pRenderer->RTTexturesLock);
		TMap<UTexture2D*,INT> *pAllTextures = &pRenderer->RTAllTextures;

		// release RHI so it is ready for next user
		if (IsInGameThread())
		{
			pTexture->ReleaseResource();
		}
		else
		{
			if (pTexture->Resource)
			{
				pTexture->Resource->ReleaseResource();
				delete pTexture->Resource;
				pTexture->Resource = NULL;
			}
		}
		pTexture->Mips.Empty();

		INT *Type = pAllTextures->Find(pTexture);
		if (Type == NULL)
		{
			if (IsInGameThread())
			{
				check ( NULL != pTexture );
				const UBOOL Result = GGFxGCManager->RemoveGCReferenceFor(pTexture);
				check( TRUE == Result );
			}
			else
			{
				pRenderer->RTAllocTextures[0].Push(pTexture);
				pRenderer->RTAllTextures.Set(pTexture,0);
			}
		}
		else
			pRenderer->RTAllocTextures[*Type].Push(pTexture);
	}
}

#define GFXUIRENDERER_NO_COMMON
#define GFXUIRENDERER_ONCE(x) ;
#include "GFxUIRendererRT.h"

#endif // WITH_GFx