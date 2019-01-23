/*=============================================================================
	RawIndexBuffer.h: Raw index buffer definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

class FRawIndexBuffer : public FIndexBuffer
{
public:

	TArray<WORD> Indices;

	/**
	 * Converts a triangle list into a triangle strip.
	 */
	INT Stripify();

	/**
	 * Orders a triangle list for better vertex cache coherency.
	 */
	void CacheOptimize();

	// FRenderResource interface.
	virtual void InitRHI();

	// Serialization.
	friend FArchive& operator<<(FArchive& Ar,FRawIndexBuffer& I);
};

class FRawIndexBuffer32 : public FIndexBuffer
{
public:

	TArray<DWORD> Indices;

	// FRenderResource interface.
	virtual void InitRHI();

	// Serialization.
	friend FArchive& operator<<(FArchive& Ar,FRawIndexBuffer32& I);
};


INT StripifyIndexBuffer(TArray<WORD>& Indices);
void CacheOptimizeIndexBuffer(TArray<WORD>& Indices);

template <UBOOL bIsCPUAccessible> 
class FRawStaticIndexBuffer : public FIndexBuffer
{
public:
	TResourceArray<WORD,bIsCPUAccessible,INDEXBUFFER_ALIGNMENT> Indices;


	/**
	* Create the index buffer RHI resource and initialize its data
	*/
	virtual void InitRHI()
	{
		if(Indices.Num())
		{
			// Create the index buffer.
			IndexBufferRHI = RHICreateIndexBuffer(sizeof(WORD),Indices.Num() * sizeof(WORD),&Indices,FALSE);
		}    
	}

	/**
	* Serializer for this class
	* @param Ar - archive to serialize to
	* @param I - data to serialize
	*/
	friend FArchive& operator<<(FArchive& Ar,FRawStaticIndexBuffer& I)
	{
		I.Indices.BulkSerialize( Ar );
		if(Ar.Ver() < VER_RENDERING_REFACTOR)
		{
			DWORD Size = 0;
			Ar << Size;
		}
		return Ar;
	}

	/**
	* Converts a triangle list into a triangle strip.
	*/
	INT Stripify()
	{
		return StripifyIndexBuffer(Indices);
	}

	/**
	* Orders a triangle list for better vertex cache coherency.
	*/
	void CacheOptimize()
	{
		CacheOptimizeIndexBuffer(Indices);
	}
};
