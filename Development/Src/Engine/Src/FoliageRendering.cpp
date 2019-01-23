/*=============================================================================
	FoliageRendering.cpp: Foliage rendering definitions.
	Copyright 2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "FoliageRendering.h"

void FFoliageIndexBuffer::SetMaxInstances(INT InMaxInstances)
{
	MaxInstances = InMaxInstances;
}

void FFoliageIndexBuffer::InitRHI()
{
	const TResourceArray<WORD,TRUE,INDEXBUFFER_ALIGNMENT>& MeshIndices = RenderData.IndexBuffer.Indices;
	const UINT NumVerticesPerInstance = RenderData.NumVertices;
	const INT NumInstances = GSupportsVertexInstancing ? 1 : MaxInstances;

	// Determine the type of index to use, and the total size of the buffer.
	const UINT Stride = (MaxInstances * NumVerticesPerInstance > 65535) ? sizeof(DWORD) : sizeof(WORD);
	const UINT Size = Stride * NumInstances * MeshIndices.Num();

	if(Size > 0)
	{
		// Create and lock the index buffer.
		IndexBufferRHI = RHICreateIndexBuffer(Stride,Size,NULL,FALSE);
		void* Buffer = RHILockIndexBuffer(IndexBufferRHI,0,Size);

		if (Stride == sizeof(DWORD))
		{
			DWORD* DestIndex = (DWORD*)Buffer;
			for(INT InstanceIndex = 0;InstanceIndex < NumInstances;InstanceIndex++)
			{
				const DWORD BaseVertexIndex = InstanceIndex * NumVerticesPerInstance;
				const WORD* SourceIndex = &MeshIndices(0);
				for(INT Index = 0;Index < MeshIndices.Num();Index++)
				{
					*DestIndex++ = BaseVertexIndex + *SourceIndex++;
				}
			}
		}
		else
		{
			WORD* DestIndex = (WORD*)Buffer;
			for(INT InstanceIndex = 0;InstanceIndex < NumInstances;InstanceIndex++)
			{
				const WORD BaseVertexIndex = InstanceIndex * NumVerticesPerInstance;
				const WORD* SourceIndex = &MeshIndices(0);
				for(INT Index = 0;Index < MeshIndices.Num();Index++)
				{
					*DestIndex++ = BaseVertexIndex + *SourceIndex++;
				}
			}
		}

		// Unlock the index buffer.
		RHIUnlockIndexBuffer(IndexBufferRHI);
	}
}
