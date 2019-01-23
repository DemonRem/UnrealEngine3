/*=============================================================================
	D3DVertexBuffer.cpp: D3D texture RHI implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "D3DDrvPrivate.h"

#if USE_D3D_RHI

/*-----------------------------------------------------------------------------
	Texture allocator support.
-----------------------------------------------------------------------------*/

/**
 * Retrieves texture memory stats. Unsupported with this allocator.
 *
 * @return FALSE, indicating that out variables were left unchanged.
 */
UBOOL RHIGetTextureMemoryStats( INT& /*AllocatedMemorySize*/, INT& /*AvailableMemorySize*/ )
{
	return FALSE;
}


/*-----------------------------------------------------------------------------
	2D texture support.
-----------------------------------------------------------------------------*/

/**
* Creates a 2D RHI texture resource
* @param SizeX - width of the texture to create
* @param SizeY - height of the texture to create
* @param Format - EPixelFormat texture format
* @param NumMips - number of mips to generate or 0 for full mip pyramid
* @param Flags - ETextureCreateFlags creation flags
*/
FTexture2DRHIRef RHICreateTexture2D(UINT SizeX,UINT SizeY,BYTE Format,UINT NumMips,DWORD Flags)
{

	DWORD D3DFlags = 0;
	D3DPOOL CreationPool = D3DPOOL_MANAGED;

	if (Flags & TexCreate_ResolveTargetable)
	{
		D3DFlags |= D3DUSAGE_RENDERTARGET;
		CreationPool = D3DPOOL_DEFAULT;
	}
	if (Flags & TexCreate_DepthStencil)
	{
		D3DFlags |= D3DUSAGE_DEPTHSTENCIL;
		CreationPool = D3DPOOL_DEFAULT;
	}

	FTexture2DRHIRef Texture2D( Flags&TexCreate_SRGB );
	VERIFYD3DCREATETEXTURERESULT(GDirect3DDevice->CreateTexture(
		SizeX,
		SizeY,
		NumMips,
		D3DFlags,
		(D3DFORMAT)GPixelFormats[Format].PlatformFormat,
		CreationPool,
		Texture2D.GetInitReference(),
		NULL
		),
		SizeX, SizeY, Format, NumMips, D3DFlags);
	return Texture2D;
}

/**
* Locks an RHI texture's mip surface for read/write operations on the CPU
* @param Texture - the RHI texture resource to lock
* @param MipIndex - mip level index for the surface to retrieve
* @param bIsDataBeingWrittenTo - used to affect the lock flags 
* @param DestStride - output to retrieve the textures row stride (pitch)
* @param bLockWithinMiptail - for platforms that support packed miptails allow locking of individual mip levels within the miptail
* @return pointer to the CPU accessible resource data
*/
void* RHILockTexture2D(FTexture2DRHIParamRef Texture,UINT MipIndex,UBOOL bIsDataBeingWrittenTo,UINT& DestStride,UBOOL bLockWithinMiptail)
{
	D3DLOCKED_RECT	LockedRect;
	DWORD			LockFlags = D3DLOCK_NOSYSLOCK;
	if( !bIsDataBeingWrittenTo )
	{
		LockFlags |= D3DLOCK_READONLY;
	}
	VERIFYD3DRESULT(Texture->LockRect(MipIndex,&LockedRect,NULL,LockFlags));
	DestStride = LockedRect.Pitch;
	return LockedRect.pBits;
}

/**
* Unlocks a previously locked RHI texture resource
* @param Texture - the RHI texture resource to unlock
* @param MipIndex - mip level index for the surface to unlock
* @param bLockWithinMiptail - for platforms that support packed miptails allow locking of individual mip levels within the miptail
*/
void RHIUnlockTexture2D(FTexture2DRHIParamRef Texture,UINT MipIndex,UBOOL bLockWithinMiptail)
{
	VERIFYD3DRESULT(Texture->UnlockRect(MipIndex));
}

/**
* For platforms that support packed miptails return the first mip level which is packed in the mip tail
* @return mip level for mip tail or -1 if mip tails are not used
*/
INT RHIGetMipTailIdx(FTexture2DRHIParamRef Texture)
{
	return -1;
}

/**
* Copies a region within the same mip levels of one texture to another.  An optional region can be speci
* Note that the textures must be the same size and of the same format.
* @param DstTexture - destination texture resource to copy to
* @param MipIdx - mip level for the surface to copy from/to. This mip level should be valid for both source/destination textures
* @param BaseSizeX - width of the texture (base level). Same for both source/destination textures
* @param BaseSizeY - height of the texture (base level). Same for both source/destination textures 
* @param Format - format of the texture. Same for both source/destination textures
* @param Region - list of regions to specify rects and source textures for the copy
*/
void RHICopyTexture2D(FTexture2DRHIParamRef DstTexture, UINT MipIdx, INT BaseSizeX, INT BaseSizeY, INT Format, const TArray<FCopyTextureRegion2D>& Regions)
{
	check( IsValidRef(DstTexture) );

	// scale the base SizeX,SizeY for the current mip level
	INT MipSizeX = Max((INT)GPixelFormats[Format].BlockSizeX,BaseSizeX >> MipIdx);
	INT MipSizeY = Max((INT)GPixelFormats[Format].BlockSizeY,BaseSizeY >> MipIdx);

	// lock the destination texture
	UINT DstStride;
	BYTE* DstData = (BYTE*)RHILockTexture2D( DstTexture, MipIdx, TRUE, DstStride );

	for( INT RegionIdx=0; RegionIdx < Regions.Num(); RegionIdx++ )		
	{
		const FCopyTextureRegion2D& Region = Regions(RegionIdx);
		check( IsValidRef(Region.SrcTexture) );

		// lock source RHI texture
		UINT SrcStride=0;
		BYTE* SrcData = (BYTE*)RHILockTexture2D( 
			Region.SrcTexture,
			// it is possible for the source texture to have > mips than the dest so start at the FirstMipIdx
			Region.FirstMipIdx + MipIdx,
			FALSE,
			SrcStride
			);	

		// align/truncate the region offset to block size
		INT RegionOffsetX = (Clamp( Region.OffsetX, 0, MipSizeX - GPixelFormats[Format].BlockSizeX ) / GPixelFormats[Format].BlockSizeX) * GPixelFormats[Format].BlockSizeX;
		INT RegionOffsetY = (Clamp( Region.OffsetY, 0, MipSizeY - GPixelFormats[Format].BlockSizeY ) / GPixelFormats[Format].BlockSizeY) * GPixelFormats[Format].BlockSizeY;
		// scale region size to the current mip level. Size is aligned to the block size
		check(Region.SizeX != 0 && Region.SizeY != 0);
		INT RegionSizeX = Clamp( Align( Region.SizeX, GPixelFormats[Format].BlockSizeX), 0, MipSizeX );
		INT RegionSizeY = Clamp( Align( Region.SizeY, GPixelFormats[Format].BlockSizeY), 0, MipSizeY );
		// handle special case for full copy
		if( Region.SizeX == -1 || Region.SizeY == -1 )
		{
			RegionSizeX = MipSizeX;
			RegionSizeY = MipSizeY;
		}

		// size in bytes of an entire row for this mip
		DWORD PitchBytes = (MipSizeX / GPixelFormats[Format].BlockSizeX) * GPixelFormats[Format].BlockBytes;
		// size in bytes of the offset to the starting part of the row to copy for this mip
		DWORD RowOffsetBytes = (RegionOffsetX / GPixelFormats[Format].BlockSizeX) * GPixelFormats[Format].BlockBytes;
		// size in bytes of the amount to copy within each row
		DWORD RowSizeBytes = (RegionSizeX / GPixelFormats[Format].BlockSizeX) * GPixelFormats[Format].BlockBytes;

		// copy each region row in increments of the block size
		for( INT CurOffsetY=RegionOffsetY; CurOffsetY < (RegionOffsetY+RegionSizeY); CurOffsetY += GPixelFormats[Format].BlockSizeY )
		{
			INT CurBlockOffsetY = CurOffsetY / GPixelFormats[Format].BlockSizeY;

			BYTE* SrcOffset = SrcData + (CurBlockOffsetY * PitchBytes) + RowOffsetBytes;
			BYTE* DstOffset = DstData + (CurBlockOffsetY * PitchBytes) + RowOffsetBytes;
			appMemcpy( DstOffset, SrcOffset, RowSizeBytes );
		}

		// done reading from source mip so unlock it
		RHIUnlockTexture2D( Region.SrcTexture, Region.FirstMipIdx + MipIdx );
	}

	// unlock the destination texture
	RHIUnlockTexture2D( DstTexture, MipIdx );
}

/**
 * Copies texture data freom one mip to another
 * Note that the mips must be the same size and of the same format.
 * @param SrcText Source texture to copy from
 * @param SrcMipIndex Mip index into the source texture to copy data from
 * @param DestText Destination texture to copy to
 * @param DestMipIndex Mip index in the destination texture to copy to - note this is probably different from source mip index if the base widths/heights are different
 * @param Size Size of mip data
 * @param Counter Thread safe counter used to flag end of transfer
 */
void RHICopyMipToMipAsync(FTexture2DRHIParamRef SrcTexture, INT SrcMipIndex, FTexture2DRHIParamRef DestTexture, INT DestMipIndex, INT Size, FThreadSafeCounter& Counter)
{
	// Lock old and new texture.
	UINT SrcStride;
	UINT DestStride;

	void* Src = RHILockTexture2D( SrcTexture, SrcMipIndex, FALSE, SrcStride );
	void* Dst = RHILockTexture2D( DestTexture, DestMipIndex, TRUE, DestStride );

#if PLATFORM_SUPPORTS_RENDERING_FROM_LOCKED_TEXTURES
	// Don't use async copy for small requests. @todo streaming: actually tweak size
	if( Size <= 8192 )
	{
		appMemcpy( Dst, Src, Size );
	}
	// Fire off async copy for larger requests so be don't need to block.
	else
	{
		// Increment request counter as we have a request to wait for.
		Counter.Increment();
		// Kick of async memory copy. @todo streaming: this could be aligned on at least consoles
		GThreadPool->AddQueuedWork( new	FAsyncCopy( Dst, Src, Size, &Counter ) );
	}
#else
	appMemcpy( Dst, Src, Size );
	RHIUnlockTexture2D( SrcTexture, SrcMipIndex );
#endif
}

/*-----------------------------------------------------------------------------
Shared texture support.
-----------------------------------------------------------------------------*/

/**
* Computes the device-dependent amount of memory required by a texture.  This size may be passed to RHICreateSharedMemory.
* @param SizeX - The width of the texture.
* @param SizeY - The height of the texture.
* @param SizeZ - The depth of the texture.
* @param Format - The format of the texture.
* @return The amount of memory in bytes required by a texture with the given properties.
*/
SIZE_T RHICalculateTextureBytes(DWORD SizeX,DWORD SizeY,DWORD SizeZ,BYTE Format)
{
	return CalculateImageBytes(SizeX,SizeY,SizeZ,Format);
}

/**
* Create resource memory to be shared by multiple RHI resources
* @param Size - aligned size of allocation
* @return shared memory resource RHI ref
*/
FSharedMemoryResourceRHIRef RHICreateSharedMemory(SIZE_T Size)
{
	// create the shared memory resource
	FSharedMemoryResourceRHIRef SharedMemory(NULL);
	return SharedMemory;
}

/**
 * Creates a RHI texture and if the platform supports it overlaps it in memory with another texture
 * Note that modifying this texture will modify the memory of the overlapped texture as well
 * @param SizeX - The width of the surface to create.
 * @param SizeY - The height of the surface to create.
 * @param Format - The surface format to create.
 * @param ResolveTargetTexture - The 2d texture to use the memory from if the platform allows
 * @param Flags - Surface creation flags
 * @return The surface that was created.
 */
FSharedTexture2DRHIRef RHICreateSharedTexture2D(UINT SizeX,UINT SizeY,BYTE Format,UINT NumMips,FSharedMemoryResourceRHIRef SharedMemory,DWORD Flags)
{
	FTexture2DRHIRef Texture2D( Flags&TexCreate_SRGB );
	VERIFYD3DRESULT(GDirect3DDevice->CreateTexture(
		SizeX,
		SizeY,
		NumMips,
		(Flags&TexCreate_ResolveTargetable) ? D3DUSAGE_RENDERTARGET : 0,
		(D3DFORMAT)GPixelFormats[Format].PlatformFormat,
		(Flags&TexCreate_ResolveTargetable) ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED,
		Texture2D.GetInitReference(),
		NULL
		));
	return Texture2D;
}

/*-----------------------------------------------------------------------------
	Cubemap texture support.
-----------------------------------------------------------------------------*/

/**
* Creates a Cube RHI texture resource
* @param Size - width/height of the texture to create
* @param Format - EPixelFormat texture format
* @param NumMips - number of mips to generate or 0 for full mip pyramid
* @param Flags - ETextureCreateFlags creation flags
*/
FTextureCubeRHIRef RHICreateTextureCube( UINT Size, BYTE Format, UINT NumMips, DWORD Flags )
{
	FTextureCubeRHIRef TextureCube( Flags&TexCreate_SRGB );
	VERIFYD3DRESULT( GDirect3DDevice->CreateCubeTexture(
		Size,
		NumMips,
		(Flags&TexCreate_ResolveTargetable) ? D3DUSAGE_RENDERTARGET : 0,
		(D3DFORMAT)GPixelFormats[Format].PlatformFormat,
		(Flags&TexCreate_ResolveTargetable) ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED,
		TextureCube.GetInitReference(),
		NULL
		));
	return TextureCube;
}

/**
* Locks an RHI texture's mip surface for read/write operations on the CPU
* @param Texture - the RHI texture resource to lock
* @param MipIndex - mip level index for the surface to retrieve
* @param bIsDataBeingWrittenTo - used to affect the lock flags 
* @param DestStride - output to retrieve the textures row stride (pitch)
* @param bLockWithinMiptail - for platforms that support packed miptails allow locking of individual mip levels within the miptail
* @return pointer to the CPU accessible resource data
*/
void* RHILockTextureCubeFace(FTextureCubeRHIParamRef TextureCube,UINT FaceIndex,UINT MipIndex,UBOOL bIsDataBeingWrittenTo,UINT& DestStride,UBOOL bLockWithinMiptail)
{
	D3DLOCKED_RECT LockedRect;
	VERIFYD3DRESULT(TextureCube->LockRect( (D3DCUBEMAP_FACES) FaceIndex, MipIndex, &LockedRect, NULL, 0 ));
	DestStride = LockedRect.Pitch;
	return LockedRect.pBits;
}

/**
* Unlocks a previously locked RHI texture resource
* @param Texture - the RHI texture resource to unlock
* @param MipIndex - mip level index for the surface to unlock
* @param bLockWithinMiptail - for platforms that support packed miptails allow locking of individual mip levels within the miptail
*/
void RHIUnlockTextureCubeFace(FTextureCubeRHIParamRef TextureCube,UINT FaceIndex,UINT MipIndex,UBOOL bLockWithinMiptail)
{
	VERIFYD3DRESULT(TextureCube->UnlockRect( (D3DCUBEMAP_FACES) FaceIndex, MipIndex ));
}

#endif
