/*=============================================================================
	UnTex.h: Unreal texture related classes.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/** The below defines are mirrored in Texture2D.uc */
 
/** A request has been kicked off and is in flight.							*/
#define TEXTURE_REQUEST_IN_FLIGHT			 3
/** Initial request has completed and finalization needs to be kicked off.	*/
#define TEXTURE_READY_FOR_FINALIZATION		 2
/** Finalization has been kicked off and is in progress.					*/
#define TEXTURE_FINALIZATION_IN_PROGRESS	 1
/** There are no pending requests/ all requests have been fulfilled.		*/
#define TEXTURE_READY_FOR_REQUESTS			 0
/** The renderer hasn't created the resource yet.							*/
#define TEXTURE_PENDING_INITIALIZATION		-1

/** 
 * The rendering resource which represents a texture.
 */
class FTextureResource : public FTexture
{
public:

	FRenderCommandFence ReleaseFence;

	FTextureResource()
	{}
	virtual ~FTextureResource() {}
};

/**
 * FTextureResource implementation for streamable 2D textures.
 */
class FTexture2DResource : public FTextureResource
{
public:
	/**
	 * Minimal initialization constructor.
	 *
	 * @param InOwner			UTexture2D which this FTexture2DResource represents.
	 * @param InitialMipCount	Initial number of miplevels to upload to card
	 * @param InFilename		Filename to read data from
 	 */
	FTexture2DResource( UTexture2D* InOwner, INT InitialMipCount, const FString& InFilename );

	/**
	 * Destructor, freeing MipData in the case of resource being destroyed without ever 
	 * having been initialized by the rendering thread via InitRHI.
	 */
	virtual ~FTexture2DResource();

	// FRenderResource interface.

	/**
	 * Called when the resource is initialized. This is only called by the rendering thread.
	 */
	virtual void InitRHI();
	/**
	 * Called when the resource is released. This is only called by the rendering thread.
	 */
	virtual void ReleaseRHI();

	/** Returns the width of the texture in pixels. */
	virtual UINT GetSizeX() const;

	/** Returns the height of the texture in pixels. */
	virtual UINT GetSizeY() const;

	/**
	 * Called from the game thread to kick off a change in ResidentMips after modifying
	 * RequestedMips.
	 */
	void BeginUpdateMipCount();
	/**
	 * Called from the game thread to kick off finalization of mip change.
	 */
	void BeginFinalizeMipCount();
	/**
	 * Called from the game thread to kick off cancelation of async operations for request.
	 */
	void BeginCancelUpdate();

	/** 
	 * Accessor
	 * @return Texture2DRHI
	 */
	FTexture2DRHIRef GetTexture2DRHI()
	{
		return Texture2DRHI;
	}

private:
	/** Texture streaming command classes that need to be friends in order to call Update/FinalizeMipCount.	*/
	friend class FUpdateMipCountCommand;
	friend class FFinalinzeMipCountCommand;
	friend class FCancelUpdateCommand;

	/** The UTexture2D which this resource represents.														*/
	const UTexture2D*	Owner;
	
	/** First miplevel used.																				*/
	INT					FirstMip;
	/** Cached filename.																					*/
	FString				Filename;

	/** Local copy/ cache of mip data between creation and first call to InitRHI.							*/
	void*				MipData[MAX_TEXTURE_MIP_COUNT];
	/** Potentially outstanding texture I/O requests.														*/
	QWORD				IORequestIndices[MAX_TEXTURE_MIP_COUNT];
	/** Number of file I/O requests for current request														*/
	INT					IORequestCount;

	/** 2D texture version of TextureRHI which is used to lock the 2D texture during mip transitions.		*/
	FTexture2DRHIRef	Texture2DRHI;
	/** Intermediate texture used to fulfill mip change requests. Swapped in FinalizeMipCount.				*/
	FTexture2DRHIRef	IntermediateTextureRHI;	

#if STATS
	/** Cached texture size for stats.																		*/
	INT					TextureSize;
	/** Cached intermediate texture size for stats.															*/
	INT					IntermediateTextureSize;
#endif

	/**
	 * Writes the data for a single mip-level into a destination buffer.
	 * @param MipIndex	The index of the mip-level to read.
	 * @param Dest		The address of the destination buffer to receive the mip-level's data.
	 * @param DestPitch	Number of bytes per row
	 */
	void GetData( UINT MipIndex,void* Dest,UINT DestPitch );

	/**
	 * Called from the rendering thread to perform the work to kick off a change in ResidentMips.
	 */
	void UpdateMipCount();
	/**
	 * Called from the rendering thread to finalize a mip change.
	 */
	void FinalizeMipCount();
	/**
	 * Called from the rendering thread to cancel async operations for request.
	 */
	void CancelUpdate();
};

/**
 * FDeferredUpdateResource for resources that need to be updated after scene rendering has begun
 * (should only be used on the rendering thread)
 */
class FDeferredUpdateResource
{
public:
	/**
	 * Constructor, initializing UpdateListLink.
	 */
	FDeferredUpdateResource()
		:	UpdateListLink(NULL)
		,	bOnlyUpdateOnce(FALSE)
	{}

	/**
	 * Iterate over the global list of resources that need to
	 * be updated and call UpdateResource on each one.
	 */
	static void UpdateResources();

	/** 
	 * This is reset after all viewports have been rendered
	 */
	static void ResetNeedsUpdate()
	{
		bNeedsUpdate = TRUE;
	}

	// FDeferredUpdateResource interface

	/**
	 * Updates the resource
	 */
	virtual void UpdateResource() = 0;

protected:

	/**
	 * Add this resource to deferred update list
	 * @param OnlyUpdateOnce - flag this resource for a single update if TRUE
	 */
	void AddToDeferredUpdateList( UBOOL OnlyUpdateOnce=FALSE );

	/**
	 * Remove this resource from deferred update list
	 */
	void RemoveFromDeferredUpdateList();

private:
	/** 
	 * Resources can be added to this list if they need a deferred update during scene rendering.
	 * @return global list of resource that need to be updated. 
	 */
	static TLinkedList<FDeferredUpdateResource*>*& GetUpdateList();
	/** This resource's link in the global list of resources needing clears. */
	TLinkedList<FDeferredUpdateResource*> UpdateListLink;
	/** if TRUE then UpdateResources needs to be called */
	static UBOOL bNeedsUpdate;
	/** if TRUE then remove this resource from the update list after a single update */
	UBOOL bOnlyUpdateOnce;
};

/**
 * FTextureResource type for render target textures.
 */
class FTextureRenderTargetResource : public FTextureResource, public FRenderTarget, public FDeferredUpdateResource
{
public:
	/**
	 * Constructor, initializing ClearLink.
	 */
	FTextureRenderTargetResource()
	{}

	/** 
	 * Return true if a render target of the given format is allowed
	 * for creation
	 */
	static UBOOL IsSupportedFormat( EPixelFormat Format );

	// FTextureRenderTargetResource interface
	
	virtual class FTextureRenderTarget2DResource* GetTextureRenderTarget2DResource()
	{
		return NULL;
	}
	virtual class FTextureRenderTargetCubeResource* GetTextureRenderTargetCubeResource()
	{
		return NULL;
	}

	// FRenderTarget interface.
	virtual UINT GetSizeX() const = 0;
	virtual UINT GetSizeY() const = 0;

	/** 
	 * Render target resource should be sampled in linear color space
	 *
	 * @return display gamma expected for rendering to this render target 
	 */
	virtual FLOAT GetDisplayGamma() const 
	{		
		return 1.0f; 
	}
};

/**
 * FTextureResource type for 2D render target textures.
 */
class FTextureRenderTarget2DResource : public FTextureRenderTargetResource
{
public:
	
	/** 
	 * Constructor
	 * @param InOwner - 2d texture object to create a resource for
	 */
	FTextureRenderTarget2DResource(const class UTextureRenderTarget2D& InOwner);

	// FTextureRenderTargetResource interface

	/** 
	 * 2D texture RT resource interface 
	 */
	virtual class FTextureRenderTarget2DResource* GetTextureRenderTarget2DResource()
	{
		return this;
	}
	
	// FRenderResource interface.

	/**
	 * Initializes the dynamic RHI resource and/or RHI render target used by this resource.
	 * Called when the resource is initialized, or when reseting all RHI resources.
	 * This is only called by the rendering thread.
	 */
	virtual void InitDynamicRHI();

	/**
	 * Release the dynamic RHI resource and/or RHI render target used by this resource.
	 * Called when the resource is released, or when reseting all RHI resources.
	 * This is only called by the rendering thread.
	 */
	virtual void ReleaseDynamicRHI();

	// FDeferredClearResource interface

	/**
	 * Clear contents of the render target
	 */
	virtual void UpdateResource();

	// FRenderTarget interface.

	virtual UINT GetSizeX() const;
	virtual UINT GetSizeY() const;	

private:
	/** The UTextureRenderTarget2D which this resource represents. */
	const class UTextureRenderTarget2D& Owner;
    /** Texture resource used for rendering with and resolving to */
    FTexture2DRHIRef Texture2DRHI;
	/** the color the texture is cleared to */
	FLinearColor ClearColor;
};

/**
 * FTextureResource type for cube render target textures.
 */
class FTextureRenderTargetCubeResource : public FTextureRenderTargetResource
{
public:

	/** 
	 * Constructor
	 * @param InOwner - cube texture object to create a resource for
	 */
	FTextureRenderTargetCubeResource(const class UTextureRenderTargetCube& InOwner)
		:	Owner(InOwner)
	{
	}

	/**
	 * We can only render to one face as a time. So, set the current 
	 * face which will be used as the render target surface.
	 * @param Face - face to use as current target face
	 */
	void SetCurrentTargetFace(ECubeFace Face);

	// FTextureRenderTargetResource interface

	/** 
	 * Cube texture RT resource interface 
	 */
	virtual class FTextureRenderTargetCubeResource* GetTextureRenderTargetCubeResource()
	{
		return this;
	}

	// FRenderResource interface.

	/**
	 * Initializes the dynamic RHI resource and/or RHI render target used by this resource.
	 * Called when the resource is initialized, or when reseting all RHI resources.
	 * This is only called by the rendering thread.
	 */
	virtual void InitDynamicRHI();

	/**
	 * Releases the dynamic RHI resource and/or RHI render target used by this resource.
	 * Called when the resource is released, or when reseting all RHI resources.
	 * This is only called by the rendering thread.
	 */
	virtual void ReleaseDynamicRHI();	

	// FDeferredClearResource interface

	/**
	 * Clear contents of the render target. Clears each face of the cube
	 * This is only called by the rendering thread.
	 */
	virtual void UpdateResource();

	// FRenderTarget interface.

	/**
	 * @return width of the target
	 */
	virtual UINT GetSizeX() const;
	/**
	 * @return height of the target
	 */
	virtual UINT GetSizeY() const;

private:
	/** The UTextureRenderTargetCube which this resource represents. */
	const class UTextureRenderTargetCube& Owner;
	/** Texture resource used for rendering with and resolving to */
	FTextureCubeRHIRef TextureCubeRHI;
	/** Target surfaces for each cube face */
	FSurfaceRHIRef CubeFaceSurfacesRHI[CubeFace_MAX];
	/** Face currently used for target surface */
	ECubeFace CurrentTargetFace;
};

/**
 * FTextureResource type for movie textures.
 */
class FTextureMovieResource : public FTextureResource, public FRenderTarget, public FDeferredUpdateResource
{
public:

	/** 
	 * Constructor
	 * @param InOwner - movie texture object to create a resource for
	 */
	FTextureMovieResource(const class UTextureMovie& InOwner)
		:	Owner(InOwner)
	{
	}

	// FRenderResource interface.

	/**
	 * Initializes the dynamic RHI resource and/or RHI render target used by this resource.
	 * Called when the resource is initialized, or when reseting all RHI resources.
	 * This is only called by the rendering thread.
	 */
	virtual void InitDynamicRHI();

	/**
	 * Release the dynamic RHI resource and/or RHI render target used by this resource.
	 * Called when the resource is released, or when reseting all RHI resources.
	 * This is only called by the rendering thread.
	 */
	virtual void ReleaseDynamicRHI();	

	// FDeferredClearResource interface

	/**
	 * Decodes the next frame of the movie stream and renders the result to this movie texture target
	 */
	virtual void UpdateResource();

	// FRenderTarget interface.

	virtual UINT GetSizeX() const;
	virtual UINT GetSizeY() const;

private:
	/** The UTextureRenderTarget2D which this resource represents. */
	const class UTextureMovie& Owner;
	/** Texture resource used for rendering with and resolving to */
	FTexture2DRHIRef Texture2DRHI;
};

/**
 * Structure containing all information related to an LOD group and providing helper functions to calculate
 * the LOD bias of a given group.
 */
struct FTextureLODSettings
{
	/**
	 * Initializes LOD settings by reading them from the passed in filename/ section.
	 *
	 * @param	IniFilename		Filename of ini to read from.
	 * @param	IniSection		Section in ini to look for settings
	 */
	void Initialize( const TCHAR* IniFilename, const TCHAR* IniSection );

	/**
	 * Calculates and returns the LOD bias based on texture LOD group, LOD bias and maximum size.
	 *
	 * @param	Texture		Texture object to calculate LOD bias for.
	 * @return	LOD bias
	 */
	INT CalculateLODBias( UTexture* Texture ) const;

	/**
	 * Will return the LODBias for a passed in LODGroup
	 *
	 * @param	InLODGroup		The LOD Group ID 
	 * @return	LODBias
	 */
	INT GetTextureLODGroupLODBias( INT InLODGroup ) const;

	/**
	 * Returns the texture group names, sorted like enum.
	 *
	 * @return array of texture group names
	 */
	static TArray<FString> GetTextureGroupNames();

private:
	/**
	 * Reads a single entry and parses it into the group array.
	 *
	 * @param	GroupId			Id/ enum of group to parse
	 * @param	GroupName		Name of group to look for in ini
	 * @param	IniFilename		Filename of ini to read from.
	 * @param	IniSection		Section in ini to look for settings
	 */
	void ReadEntry( INT GroupId, const TCHAR* GroupName, const TCHAR* IniFilename, const TCHAR* IniSection );

	/** Array of LOD settings with entries per group. */
	struct FTextureLODGroup 
	{
		/** Minimum LOD mip count below which the code won't bias.						*/
		INT MinLODMipCount;
		/** Maximum LOD mip count. Bias will be adjusted so texture won't go above.		*/
		INT MaxLODMipCount;
		/** Group LOD bias.																*/
		INT LODBias;
	} TextureLODGroups[TEXTUREGROUP_MAX];
};








