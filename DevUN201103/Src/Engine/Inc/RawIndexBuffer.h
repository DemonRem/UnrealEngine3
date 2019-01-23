/*=============================================================================
	RawIndexBuffer.h: Raw index buffer definitions.
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
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

#if DISALLOW_32BIT_INDICES

// if 32 bit indices are disallowed, then use 16 bits in the FRawIndexBuffer16or32
typedef FRawIndexBuffer FRawIndexBuffer16or32;

#else

class FRawIndexBuffer16or32 : public FIndexBuffer
{
public:

	TArray<DWORD> Indices;
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
	friend FArchive& operator<<(FArchive& Ar,FRawIndexBuffer16or32& I);
};
#endif

class FRawStaticIndexBuffer : public FIndexBuffer
{
public:
	TResourceArray<WORD,INDEXBUFFER_ALIGNMENT> Indices;
	
	/**
	* Constructor
	* @param InNeedsCPUAccess - TRUE if resource array data should be CPU accessible
	*/
	FRawStaticIndexBuffer(UBOOL InNeedsCPUAccess=FALSE)
	:	Indices(InNeedsCPUAccess)
	,	NumVertsPerInstance(0)
	,	bSetupForInstancing(FALSE)
	{
	}

	/**
	* Create the index buffer RHI resource and initialize its data
	*/
	virtual void InitRHI();

	/**
	* Serializer for this class
	* @param Ar - archive to serialize to
	* @param I - data to serialize
	*/
	friend FArchive& operator<<(FArchive& Ar,FRawStaticIndexBuffer& I);

	/**
	* Converts a triangle list into a triangle strip.
	*/
	INT Stripify();

	/**
	* Orders a triangle list for better vertex cache coherency.
	*/
	void CacheOptimize();

	/** Sets the index buffer up for hardware vertex instancing. */
	void SetupForInstancing(UINT InNumVertsPerInstance)
	{
		bSetupForInstancing = TRUE;
		NumVertsPerInstance = InNumVertsPerInstance;
	}

	UINT GetNumVertsPerInstance() const	{ return NumVertsPerInstance; }
	UBOOL GetSetupForInstancing() const	{ return bSetupForInstancing; }

private:
	UINT			NumVertsPerInstance;  // provided by the user (often 1+ the max index in the buffer, but not always)
	UBOOL			bSetupForInstancing;
};

#if DISALLOW_32BIT_INDICES

// if 32 bit indices are disallowed, then use 16 bits in the FRawStaticIndexBuffer16or32
typedef FRawStaticIndexBuffer FRawStaticIndexBuffer16or32;

#else

template <typename INDEX_TYPE>
class FRawStaticIndexBuffer16or32 : public FIndexBuffer
{
public:
	TResourceArray<INDEX_TYPE,INDEXBUFFER_ALIGNMENT> Indices;
	
	/**
	* Constructor
	* @param InNeedsCPUAccess - TRUE if resource array data should be CPU accessible
	*/
	FRawStaticIndexBuffer16or32(UBOOL InNeedsCPUAccess=FALSE)
	:	Indices(InNeedsCPUAccess)
	,	NumVertsPerInstance(0)
	,	bSetupForInstancing(FALSE)
	{
	}

	/**
	* Create the index buffer RHI resource and initialize its data
	*/
	virtual void InitRHI()
	{
		UINT Size = Indices.Num() * sizeof(INDEX_TYPE);
		if(Indices.Num())
		{
			if (bSetupForInstancing)
			{
				// Create an instanced index buffer.
				check(NumVertsPerInstance > 0);
				// Create the index buffer.
				UINT NumInstances = 0;
				IndexBufferRHI = RHICreateInstancedIndexBuffer(sizeof(INDEX_TYPE),Size,RUF_Static,NumInstances);
				check(NumInstances);
				// Initialize the buffer.
				INDEX_TYPE* Buffer = (INDEX_TYPE *)RHILockIndexBuffer(IndexBufferRHI,0,Size * NumInstances);
				INDEX_TYPE Offset = 0;
				//check((NumInstances + 1) * NumVertsPerInstance < (UINT)MAXDWORD);
				for (UINT Instance = 0; Instance < NumInstances; Instance++)
				{
					for (INT Index = 0; Index < Indices.Num(); Index++)
					{
						*Buffer++ = Indices(Index) + Offset;
					}
					Offset += (INDEX_TYPE)NumVertsPerInstance;
				}
				RHIUnlockIndexBuffer(IndexBufferRHI);
			}
			else
			{
				// Create the index buffer.
				IndexBufferRHI = RHICreateIndexBuffer(sizeof(INDEX_TYPE),Size,&Indices,RUF_Static);
			}
		}    
	}

	/**
	* Serializer for this class
	* @param Ar - archive to serialize to
	* @param I - data to serialize
	*/
	void Serialize( FArchive& Ar )
	{
		if (Ar.IsLoading() && (Ar.Ver() < VER_DWORD_SKELETAL_MESH_INDICES)) // 16 bit
		{
			TResourceArray<WORD,INDEXBUFFER_ALIGNMENT> WORDIndexBuffer;
			WORDIndexBuffer.BulkSerialize(Ar);
			for (INT i = 0; i < WORDIndexBuffer.Num(); ++i)
			{
				Indices.AddItem(WORDIndexBuffer(i));
			}
		}
		else
		{
			Indices.BulkSerialize( Ar );
		}
		if (Ar.IsLoading())
		{
			// Make sure these are set to no-instancing values
			NumVertsPerInstance = 0;
			bSetupForInstancing = FALSE;
		}
	}

	/**
	* Converts a triangle list into a triangle strip.
	*/
	INT Stripify();

	/**
	* Orders a triangle list for better vertex cache coherency.
	*/
	void CacheOptimize();


	/** Sets the index buffer up for hardware vertex instancing. */
	void SetupForInstancing(UINT InNumVertsPerInstance)
	{
		bSetupForInstancing = TRUE;
		NumVertsPerInstance = InNumVertsPerInstance;
	}

	UINT GetNumVertsPerInstance() const	{ return NumVertsPerInstance; }
	UBOOL GetSetupForInstancing() const	{ return bSetupForInstancing; }
	UBOOL GetNeedsCPUAccess() const { return Indices.GetAllowCPUAccess(); }

private:
	UINT			NumVertsPerInstance;  // provided by the user (often 1+ the max index in the buffer, but not always)
	UBOOL			bSetupForInstancing;
};
#endif


/**
 *	FRawGPUIndexBuffer represents a basic index buffer GPU resource with
 *	no CPU-side data.
 *	The member functions are meant to be called from the renderthread only.
 */
class FRawGPUIndexBuffer : public FIndexBuffer
{
public:
	/**
	 *	Default constructor
	 */
	FRawGPUIndexBuffer();

	/**
	 *	Setup constructor
	 *	@param InNumIndices		- Number of indices to allocate space for
	 *	@param InIsDynamic		- TRUE if the index buffer should be dynamic
	 *	@param InStride			- Number of bytes per index
	 */
	FRawGPUIndexBuffer(UINT InNumIndices, UBOOL InIsDynamic=FALSE, UINT InStride=sizeof(WORD));

	/**
	 *	Sets up the index buffer, if the default constructor was used.
	 *	@param InNumIndices		- Number of indices to allocate space for
	 *	@param InIsDynamic		- TRUE if the index buffer should be dynamic
	 *	@param InStride			- Number of bytes per index
	 */
	void Setup(UINT InNumIndices, UBOOL InIsDynamic=FALSE, UINT InStride=sizeof(WORD));

	/**
	 *	Returns TRUE if the index buffer hasn't been filled in with data yet,
	 *	or if it's a dynamic resource that has been re-created due to Device Lost.
	 *	Calling Lock + Unlock will make the index buffer non-empty again.
	 */
	UBOOL IsEmpty() const
	{
		return bIsEmpty;
	}

	/**
	 *	Returns the number of indices that are allocated in the buffer.
	 */
	UINT GetNumIndices() const
	{
		return NumIndices;
	}

	/**
	 *	Create an empty static index buffer RHI resource.
	 */
	virtual void InitRHI();

	/**
	 *	Releases a dynamic index buffer RHI resource.
	 *	Called when the resource is released, or when reseting all RHI resources.
	 */
	virtual void ReleaseRHI();

	/**
	 *	Create an empty dynamic index buffer RHI resource.
	 */
	virtual void InitDynamicRHI();

	/**
	 *	Releases a dynamic index buffer RHI resource.
	 *	Called when the resource is released, or when reseting all RHI resources.
	 */
	virtual void ReleaseDynamicRHI();

	/**
	 *	Locks the index buffer and returns a pointer to the first index in the locked region.
	 *
	 *	@param FirstIndex		- First index in the locked region. Defaults to the first index in the buffer.
	 *	@param NumIndices		- Number of indices to lock. Defaults to the remainder of the buffer.
	 */
	void*	Lock( UINT FirstIndex=0, UINT NumIndices=0 );

	/**
	 *	Unlocks the index buffer.
	 */
	void	Unlock( );

protected:
	UINT	NumIndices;
	UINT	Stride;
	UBOOL	bIsDynamic;
	UBOOL	bIsEmpty;
};
