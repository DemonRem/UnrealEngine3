/*=============================================================================
	Texture2DComposite.cpp: Implementation of UTexture2DComposite.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"

IMPLEMENT_CLASS(UTexture2DComposite);

/*-----------------------------------------------------------------------------
UTexture2DComposite
-----------------------------------------------------------------------------*/

/**
* Regenerates this composite texture using the list of source texture regions.
* The existing mips are reallocated and the RHI resource for the texture is updated
*
* @param NumMipsToGenerate - number of mips to generate. if 0 then all mips are created
*/
void UTexture2DComposite::UpdateCompositeTexture(INT NumMipsToGenerate)
{
	// initialize the list of valid regions
	InitValidSourceRegions();

	if( ValidRegions.Num() == 0 )
	{
		debugf( NAME_Warning, TEXT("UTexture2DComposite: no regions to process") );
	}
	else
	{
		// calc index of first available mip common to the set of source textures
		INT FirstSrcMipIdx = GetFirstAvailableMipIndex();

		// calc the texture size for the comp texture based on the MaxLODBias
		INT SrcSizeX = ValidRegions(0).Texture2D->Mips(FirstSrcMipIdx).SizeX;
		INT SrcSizeY = ValidRegions(0).Texture2D->Mips(FirstSrcMipIdx).SizeY;
		// use the same format as the source textures
		EPixelFormat SrcFormat = (EPixelFormat)ValidRegions(0).Texture2D->Format;

		// re-init the texture and add first mip
		Init(SrcSizeX,SrcSizeY,SrcFormat);
		// add the rest of the mips as needed
		InitMips( NumMipsToGenerate );
		// make sure miptail index is 0 so texture is created without a miptail
		MipTailBaseIdx = 0;

		// fill in all of the Mip data 
		CopyRectRegions();

		// Copy other settings.
		SRGB = ValidRegions(0).Texture2D->SRGB;
		RGBE = ValidRegions(0).Texture2D->RGBE;

		for(INT i=0; i<4; i++)
		{
			UnpackMin[i] = ValidRegions(0).Texture2D->UnpackMin[i];
			UnpackMax[i] = ValidRegions(0).Texture2D->UnpackMax[i];
		}

		LODGroup = ValidRegions(0).Texture2D->LODGroup;
		LODBias = ValidRegions(0).Texture2D->LODBias;

		// re-init the texture resource
		UpdateResource();
	}
}

/** Utility that checks to see if all Texture2Ds specified in the SourceRegions array are fully streamed in. */
UBOOL UTexture2DComposite::SourceTexturesFullyStreamedIn()
{
	for(INT i=0; i<SourceRegions.Num(); i++)
	{
		if(SourceRegions(i).Texture2D)
		{
			UBOOL bAllStreamedIn = SourceRegions(i).Texture2D->IsFullyStreamedIn();
			if(!bAllStreamedIn)
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

/** Utils to reset all source region info. */
void UTexture2DComposite::ResetSourceRegions()
{
	SourceRegions.Empty();
	ValidRegions.Empty();
}

/**
* Calculate the first available mip from a set of textures based on the LOD bias for each
* texture.
* 
* @return first available mip index from the source regions
*/
INT UTexture2DComposite::GetFirstAvailableMipIndex()
{
	check(ValidRegions.Num() > 0);

	// find the max LOD Bias from the source textures
	INT MaxLODBias = 0;
	for( INT RegionIdx=0; RegionIdx < ValidRegions.Num(); RegionIdx++ )
	{
		const FSourceTexture2DRegion& Region = ValidRegions(RegionIdx);			
		MaxLODBias = Max<INT>( MaxLODBias, Region.Texture2D->GetCachedLODBias() );
	}

	// max num of mips that can be loaded
	INT	MaxResidentMips	= Max( 1, Min(ValidRegions(0).Texture2D->Mips.Num() - MaxLODBias, GMaxTextureMipCount) );

	// clamp number of mips based on the maximum allowed texture size. if 0 then no clamping
	if( MaxTextureSize > 0 )
	{
		MaxResidentMips = Min<INT>(MaxResidentMips,appCeilLogTwo(MaxTextureSize)+1);
	}
	
	// find the largest common resident num of mips
	INT MaxCurrentResidentMips = MaxResidentMips;
	for( INT RegionIdx=0; RegionIdx < ValidRegions.Num(); RegionIdx++ )
	{
		const FSourceTexture2DRegion& Region = ValidRegions(RegionIdx);			
		MaxCurrentResidentMips = Min<INT>( MaxCurrentResidentMips, Region.Texture2D->ResidentMips );
	}

	// first source mip level to be used
	INT FirstMipIdx = ValidRegions(0).Texture2D->Mips.Num() - MaxCurrentResidentMips;

	return FirstMipIdx;
}

/**
* Initializes the list of ValidRegions with only valid entries from the list of source regions
*/
void UTexture2DComposite::InitValidSourceRegions()
{
	ValidRegions.Empty();

	UTexture2D* CompareTex = NULL;

	// update source regions list with only valid entries
	for( INT SrcIdx=0; SrcIdx < SourceRegions.Num(); SrcIdx++ )
	{
		const FSourceTexture2DRegion& SourceRegion = SourceRegions(SrcIdx);
		// the source texture for this region must exist
		if( SourceRegion.Texture2D == NULL )
		{
			debugf( NAME_Warning, TEXT("FCompositeTexture2DUtil: Source texture missing - skipping...") );
		}
		// texture formats should match
		else if( CompareTex && SourceRegion.Texture2D->Format != CompareTex->Format )
		{
			debugf( NAME_Warning, TEXT("FCompositeTexture2DUtil: Source texture format mismatch [%s] - skipping..."), 
				*SourceRegion.Texture2D->GetPathName() );				
		}
		// SRGB
		else if( CompareTex && SourceRegion.Texture2D->SRGB != CompareTex->SRGB )
		{
			debugf( NAME_Warning, TEXT("FCompositeTexture2DUtil: Source texture SRGB mismatch [%s] - skipping..."),
				*SourceRegion.Texture2D->GetPathName() );
		}
		// RGBE
		else if( CompareTex && SourceRegion.Texture2D->RGBE != CompareTex->RGBE )
		{
			debugf( NAME_Warning, TEXT("FCompositeTexture2DUtil: Source texture RGBE mismatch [%s] - skipping..."),
				*SourceRegion.Texture2D->GetPathName() );
		}
		// source textures must match in SizeX/SizeY
		else if( CompareTex && (SourceRegion.Texture2D->SizeX != CompareTex->SizeX || SourceRegion.Texture2D->SizeY != CompareTex->SizeY) )
		{
			debugf( NAME_Warning, TEXT("FCompositeTexture2DUtil: Source texture size mismatch [%s] (%d,%d) not (%d,%d) from [%s] - skipping..."), 
				*SourceRegion.Texture2D->GetPathName(),
				SourceRegion.Texture2D->SizeX,
				SourceRegion.Texture2D->SizeY,
				CompareTex->SizeX,
				CompareTex->SizeY,
				*CompareTex->GetPathName() );
		}
		// source textures must have the same number of mips
		else if( CompareTex && SourceRegion.Texture2D->Mips.Num() != CompareTex->Mips.Num() )
		{
			debugf( NAME_Warning, TEXT("FCompositeTexture2DUtil: Source texture number Mips mismatch [%s] %d not %d - skipping..."), 
				*SourceRegion.Texture2D->GetPathName(),
				SourceRegion.Texture2D->Mips.Num(),
				CompareTex->Mips.Num() );
		}
		// source region not outside texture area
		else if( CompareTex && ((SourceRegion.OffsetX + SourceRegion.SizeX > CompareTex->SizeX) || (SourceRegion.OffsetY + SourceRegion.SizeY > CompareTex->SizeY)) )
		{
			debugf( NAME_Warning, TEXT("FCompositeTexture2DUtil: Source region outside texture area [%s] - skipping..."), 
				*SourceRegion.Texture2D->GetPathName() );
		}
		// make sure source textures are not streamable
		else if( !SourceRegion.Texture2D->IsFullyStreamedIn() )
		{
			debugf( NAME_Warning, TEXT("FCompositeTexture2DUtil: Source texture is not fully streamed in [%s] - skipping..."), 
				*SourceRegion.Texture2D->GetPathName() );
		}
		// valid so add it to the list
		else
		{
			ValidRegions.AddItem( SourceRegion );
			// First valid texture - remember to compare to others that follow.
			if(!CompareTex)
			{
				CompareTex = SourceRegion.Texture2D;
			}
		}
	}
}

/**
* Reallocates the mips array
*
* @param NumMipsToGenerate - number of mips to generate. if 0 then all mips are created
*/
void UTexture2DComposite::InitMips( INT NumMipsToGenerate )
{
	check(!(SizeX % GPixelFormats[Format].BlockSizeX));
	check(!(SizeY % GPixelFormats[Format].BlockSizeY));

	// max number of mips based on texture size
	INT MaxTexMips = appCeilLogTwo(Max(SizeX,SizeY))+1;
	// clamp num mips to maxmips or force to maxmips if set <= 0
	INT NumTexMips	= NumMipsToGenerate > 0 ? Min( NumMipsToGenerate, MaxTexMips ) : MaxTexMips;

	for( INT MipIdx=Mips.Num(); MipIdx < NumTexMips; MipIdx++ )
	{
		// add new mip entry
		FTexture2DMipMap* MipMap = new(Mips) FTexture2DMipMap;
		// next mip level size
		MipMap->SizeX = Max((INT)GPixelFormats[Format].BlockSizeX,SizeX >> MipIdx);
		MipMap->SizeY = Max((INT)GPixelFormats[Format].BlockSizeY,SizeY >> MipIdx);
		// byte size for this mip level
		SIZE_T ImageSize = CalculateImageBytes(MipMap->SizeX,MipMap->SizeY,0,(EPixelFormat)Format);
		// realloc data block to match size
		MipMap->Data.Lock( LOCK_READ_WRITE );
		MipMap->Data.Realloc( ImageSize );
		MipMap->Data.Unlock();
	}
}

/**
* Locks each region of the source texture mip and copies the block of data
* for that region to the destination mip buffer. This is done for all mip levels.
*/
void UTexture2DComposite::CopyRectRegions()
{
	// fill in all of the Mip data 
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		CopyRectRegions,
		UTexture2DComposite*,CompTex,this,
	{
		CompTex->RenderThread_CopyRectRegions();					
	});
	FlushRenderingCommands();	
}

/**
* Locks each region of the source RHI texture 2d resources and copies the block of data
* for that region to the destination mip buffer. This is done for all mip levels.
*
* (Only called by the rendering thread)
*/
void UTexture2DComposite::RenderThread_CopyRectRegions()
{
	check(ValidRegions.Num() > 0);

	// calc index of first available mip common to the set of source textures
	INT FirstSrcMipIdx = GetFirstAvailableMipIndex();

	// create a temp RHI texture 2d used to hold intermediate mip data
	FTexture2DRHIRef TempTexture2D = RHICreateTexture2D(SizeX,SizeY,Format,Mips.Num(),0);

	// process each mip level
	for( INT MipIdx=0; MipIdx < Mips.Num(); MipIdx++ )
	{
		// destination mip data
		FTexture2DMipMap& DstMip = Mips(MipIdx);
		// list of source regions that need to be copied for this mip level
		TArray<FCopyTextureRegion2D> CopyRegions;

		for( INT RegionIdx=0; RegionIdx < ValidRegions.Num(); RegionIdx++ )		
		{
			FSourceTexture2DRegion& Region = ValidRegions(RegionIdx);
			FTexture2DResource* SrcTex2DResource = (FTexture2DResource*)Region.Texture2D->Resource;		
			if( SrcTex2DResource && 
				SrcTex2DResource->IsInitialized() &&
				Region.Texture2D->IsFullyStreamedIn() &&
				Region.Texture2D->ResidentMips == Region.Texture2D->RequestedMips )
			{
				check( Region.Texture2D->Mips.IsValidIndex(MipIdx + FirstSrcMipIdx) );
				check( Region.Texture2D->ResidentMips >= Mips.Num() );

				// scale regions for the current mip level. The regions are assumed to be sized w/ respect to the base (maybe not loaded) mip level
				INT RegionOffsetX = Region.OffsetX >> (MipIdx + FirstSrcMipIdx);
				INT RegionOffsetY = Region.OffsetY >> (MipIdx + FirstSrcMipIdx);
				INT RegionSizeX = Max(Region.SizeX >> (MipIdx + FirstSrcMipIdx),1);
				INT RegionSizeY = Max(Region.SizeY >> (MipIdx + FirstSrcMipIdx),1);
				// scale create the new region for the RHI texture copy
				FCopyTextureRegion2D& CopyRegion = *new(CopyRegions) FCopyTextureRegion2D(
					SrcTex2DResource->GetTexture2DRHI(),		
					RegionOffsetX,
					RegionOffsetY,
					RegionSizeX,
					RegionSizeY,
					// it is possible for source texture mips > dest mips so adjust the base level when copying from the source
					Region.Texture2D->ResidentMips - Mips.Num()
					);
			}
		}

		// copy from all of the source regions for the current mip level to the temp texture
		RHICopyTexture2D(TempTexture2D,MipIdx,SizeX,SizeY,Format,CopyRegions);
	}

	// find the base level for the miptail
	INT MipTailIdx = RHIGetMipTailIdx(TempTexture2D);
	// set the miptail level for this utexture (-1 means no miptail so just use the last mip)
	MipTailBaseIdx = (MipTailIdx == -1) ? Mips.Num()-1 : MipTailIdx;
	
	// copy the results from the temp texture to the mips array
	for( INT MipIdx=0; MipIdx <= MipTailBaseIdx; MipIdx++ )
	{
		// destination mip data
		FTexture2DMipMap& DstMip = Mips(MipIdx);

		// lock the temp texture
		UINT TempStride;
		BYTE* TempData = (BYTE*)RHILockTexture2D( TempTexture2D, MipIdx, FALSE, TempStride );

		// copy to the mips array
		BYTE* DstData = (BYTE*)DstMip.Data.Lock( LOCK_READ_WRITE );
		appMemcpy(DstData,TempData,DstMip.Data.GetBulkDataSize());
		DstMip.Data.Unlock();

		// unlock the temp RHI texture
		RHIUnlockTexture2D( TempTexture2D, MipIdx );
	}	
}


