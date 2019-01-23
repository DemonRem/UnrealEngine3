/*=============================================================================
	D3DVertexBuffer.cpp: D3D vertex buffer RHI implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "D3DDrvPrivate.h"

#if USE_D3D_RHI

FVertexBufferRHIRef RHICreateVertexBuffer(UINT Size,FResourceArrayInterface* ResourceArray,UBOOL bIsDynamic)
{
	// Explicitly check that the size is nonzero before allowing CreateVertexBuffer to opaquely fail.
	check(Size > 0);

	// Determine the appropriate usage flags and pool for the resource.
	const DWORD Usage = bIsDynamic ? (D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY) : 0;
	const D3DPOOL Pool = bIsDynamic ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED;

	// Create the vertex buffer.
	FVertexBufferRHIRef VertexBuffer;
	VERIFYD3DRESULT(GDirect3DDevice->CreateVertexBuffer(Size,Usage,0,Pool,VertexBuffer.GetInitReference(),NULL));

	// If a resource array was provided for the resource, copy the contents into the buffer.
	if(ResourceArray)
	{
		// Initialize the buffer.
		void* Buffer = RHILockVertexBuffer(VertexBuffer,0,Size);
		check(Buffer);
		check(Size == ResourceArray->GetResourceDataSize());
		appMemcpy(Buffer,ResourceArray->GetResourceData(),Size);
		RHIUnlockVertexBuffer(VertexBuffer);

		// Discard the resource array's contents.
		ResourceArray->Discard();
	}

	return VertexBuffer;
}

void* RHILockVertexBuffer(FVertexBufferRHIParamRef VertexBuffer,UINT Offset,UINT Size,UBOOL bReadOnly)
{
	// Determine whether this is a static or dynamic VB.
	D3DVERTEXBUFFER_DESC VertexBufferDesc;
	VERIFYD3DRESULT(VertexBuffer->GetDesc(&VertexBufferDesc));
	const UBOOL bIsDynamic = (VertexBufferDesc.Usage & D3DUSAGE_DYNAMIC) != 0;

	// For dynamic VBs, discard the previous contents before locking.
	const DWORD LockFlags = bIsDynamic ? D3DLOCK_DISCARD : (bReadOnly ? D3DLOCK_READONLY : 0);

	// Lock the vertex buffer.
	void* Data = NULL;
	VERIFYD3DRESULT(VertexBuffer->Lock(Offset,Size,&Data,LockFlags));
	return Data;
}

void RHIUnlockVertexBuffer(FVertexBufferRHIParamRef VertexBuffer)
{
	VERIFYD3DRESULT(VertexBuffer->Unlock());
}

#endif
