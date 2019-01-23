/*=============================================================================
	RenderResource.cpp: Render resource implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"

TLinkedList<FRenderResource*>*& FRenderResource::GetResourceList()
{
	static TLinkedList<FRenderResource*>* FirstResourceLink = NULL;
	return FirstResourceLink;
}

void FRenderResource::Init()
{
	check(IsInRenderingThread());
	if(!bInitialized)
	{
		ResourceLink = TLinkedList<FRenderResource*>(this);
		ResourceLink.Link(GetResourceList());
		if(GIsRHIInitialized)
		{
			InitDynamicRHI();
			InitRHI();
		}
		bInitialized = TRUE;
	}
}

void FRenderResource::Release()
{
	check(IsInRenderingThread());
	if(bInitialized)
	{
		if(GIsRHIInitialized)
		{
			ReleaseRHI();
			ReleaseDynamicRHI();
		}
		ResourceLink.Unlink();
		bInitialized = FALSE;
	}
}

void FRenderResource::UpdateRHI()
{
	check(IsInRenderingThread());
	if(bInitialized && GIsRHIInitialized)
	{
		ReleaseRHI();
		ReleaseDynamicRHI();
		InitDynamicRHI();
		InitRHI();
	}
}

FRenderResource::~FRenderResource()
{
	if (bInitialized)
	{
		appErrorf(TEXT("FRenderResource '%s' was deleted without being released first!"), *GetFriendlyName());
	}
}

void BeginInitResource(FRenderResource* Resource)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		InitCommand,
		FRenderResource*,Resource,Resource,
		{
			Resource->Init();
		});
}

void BeginUpdateResourceRHI(FRenderResource* Resource)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		UpdateCommand,
		FRenderResource*,Resource,Resource,
		{
			Resource->UpdateRHI();
		});
}

void BeginReleaseResource(FRenderResource* Resource)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		ReleaseCommand,
		FRenderResource*,Resource,Resource,
		{
			Resource->Release();
		});
}

void ReleaseResourceAndFlush(FRenderResource* Resource)
{
	// Send the release message.
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		ReleaseCommand,
		FRenderResource*,Resource,Resource,
		{
			Resource->Release();
		});

	FlushRenderingCommands();
}

/** The global null color vertex buffer, which is set with a stride of 0 on meshes without a color component. */
TGlobalResource<FNullColorVertexBuffer> GNullColorVertexBuffer;

