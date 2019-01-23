/*=============================================================================
	UnTexCompress.cpp: Unreal texture compression functions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"

#if _MSC_VER && !CONSOLE
#pragma pack (push,8)
#include "../../../nvDXT/Inc/dxtlib.h"
#pragma pack (pop)
#ifdef _DEBUG
#pragma comment(lib, "nvDXTlibMTDLLd.vc8.lib")
#else
#pragma comment(lib, "nvDXTlibMTDLL.vc8.lib")
#endif
#endif


/*-----------------------------------------------------------------------------
	DXT functions.
-----------------------------------------------------------------------------*/

// Callbacks required by nvDXT library.
#if _MSC_VER && !CONSOLE

NV_ERROR_CODE CompressionCallback(const void *Data, size_t NumBytes, const MIPMapData * MipMapData, void * UserData )
{
	UTexture2D* Texture2D = Cast<UTexture2D>((UObject*)UserData);
	if( MipMapData && Texture2D )
	{
		FTexture2DMipMap* MipMap = new(Texture2D->Mips)FTexture2DMipMap;
		
		MipMap->SizeX = Max<UINT>(MipMapData->width,4);
		MipMap->SizeY = Max<UINT>(MipMapData->height,4);
		MipMap->Data.Lock(LOCK_READ_WRITE);
		appMemcpy( MipMap->Data.Realloc(NumBytes), Data, NumBytes );
		MipMap->Data.Unlock();
	}
	return NV_OK;
}

#endif

/**
 * Shared compression functionality. 
 */
void UTexture::Compress()
{
	// High dynamic range textures are currently always stored as RGBE (shared exponent) textures.
	RGBE = (CompressionSettings == TC_HighDynamicRange);
}

/** Defines an image's data. */
class FImageData
{
public:

	BYTE* Buffer;
	INT SizeX;
	INT SizeY;
	INT SizeZ;
	INT StrideY;
	INT StrideZ;

	/** Initialization constructor. */
	FImageData(BYTE* InBuffer,INT InSizeX,INT InSizeY,INT InSizeZ,INT InStrideY,INT InStrideZ):
		Buffer(InBuffer),
		SizeX(InSizeX),
		SizeY(InSizeY),
		SizeZ(InSizeZ),
		StrideY(InStrideY),
		StrideZ(InStrideZ)
	{}
};

/** Defines the format of an image. */
class FImageFormat
{
public:

	INT Format;
	UBOOL bSRGB;
	UBOOL bRGBE;

	/** Initialization constructor. */
	FImageFormat(INT InFormat,UBOOL bInSRGB,UBOOL bInRGBE):
		Format(InFormat),
		bSRGB(bInSRGB),
		bRGBE(bInRGBE)
	{}	
};

/**
 * Generates a mip-map for an A8R8G8B8 image using a box filter.
 * @param SourceImageData - The source image's data.
 * @param DestImageData - The destination image's data.
 * @param ImageFormat - The format of both the source and destination images.
 * @param bDither - Dither the mip-map for smooth transitions.
 * @param bPreserveBorderR - If TRUE, preserve color in border pixels.
 * @param bPreserveBorderG - If TRUE, preserve color in border pixels.
 * @param bPreserveBorderB - If TRUE, preserve color in border pixels.
 * @param bPreserveBorderA - If TRUE, preserve color in border pixels.
 */
static void GenerateMipMapA8R8G8B8(const FImageData& SourceImageData,const FImageData& DestImageData,const FImageFormat& ImageFormat,UBOOL bDither,UBOOL bPreserveBorderR,UBOOL bPreserveBorderG,UBOOL bPreserveBorderB,UBOOL bPreserveBorderA)
{
	// Was the preservation of border color requested?
	const UBOOL bPreserveBorderColor = bPreserveBorderR || bPreserveBorderG || bPreserveBorderB || bPreserveBorderA;

	// Set up a random number stream for dithering.
	FRandomStream RandomStream(0);

	// Compute the box filter's parameters.
	const INT FilterSizeX = SourceImageData.SizeX / DestImageData.SizeX;
	const INT FilterSizeY = SourceImageData.SizeY / DestImageData.SizeY;
	const INT FilterSizeZ = SourceImageData.SizeZ / DestImageData.SizeZ;
	const FLOAT InverseFilterSum = 1.0f / (FilterSizeX * FilterSizeY * FilterSizeZ);

	if(ImageFormat.Format == PF_A8R8G8B8)
	{
		for(INT DestZ = 0;DestZ < DestImageData.SizeZ;DestZ++)
		{
			for(INT DestY = 0;DestY < DestImageData.SizeY;DestY++)
			{
				const UBOOL bDestIsBorderY = (DestY==0) || (DestY==DestImageData.SizeY-1);
				for(INT DestX = 0;DestX < DestImageData.SizeX;DestX++)
				{
					const UBOOL bDestIsBorderX = (DestX==0) || (DestX==DestImageData.SizeX-1);
					FLinearColor FilteredColor(0,0,0,0);

					// Apply a box filter to the source image's pixels corresponding to the destination image's pixel.
					if ( bPreserveBorderColor && (bDestIsBorderY||bDestIsBorderX) )
					{
						// If border preservation was requested and the dest pixel is on the border,
						// only consider source image pixels that are themselves border pixels.
						INT NumSamples = 0;
						for(INT FilterZ = 0;FilterZ < FilterSizeZ;FilterZ++)
						{
							for(INT FilterX = 0;FilterX < FilterSizeX;FilterX++)
							{
								for(INT FilterY = 0;FilterY < FilterSizeY;FilterY++)
								{
									// Lookup the source pixel for this component of the filter.
									const INT SourceX = DestX * FilterSizeX + FilterX;
									const INT SourceY = DestY * FilterSizeY + FilterY;

									// Only consider border pixels in the source image.
									if ( SourceX == 0 || SourceX == SourceImageData.SizeX-1
										|| SourceY == 0 || SourceY == SourceImageData.SizeY-1 )
									{
										const INT SourceZ = DestZ * FilterSizeZ + FilterZ;

										const FColor& SourceColor = *(FColor*)
											(SourceImageData.Buffer +
												SourceX * sizeof(FColor) +
												SourceY * SourceImageData.StrideY +
												SourceZ * SourceImageData.StrideZ);

										// Transform the source color into a linear color.
										FLinearColor LinearSourceColor;
										if(ImageFormat.bRGBE)
										{
											LinearSourceColor = SourceColor.FromRGBE();
										}
										else if(ImageFormat.bSRGB)
										{
											LinearSourceColor = FLinearColor(SourceColor);
										}
										else
										{
											LinearSourceColor = SourceColor.ReinterpretAsLinear();
										}

										// Filter channels on border pixels in the source image.
										if ( !bPreserveBorderR ) { LinearSourceColor.R = 0.f; }
										if ( !bPreserveBorderG ) { LinearSourceColor.G = 0.f; }
										if ( !bPreserveBorderB ) { LinearSourceColor.B = 0.f; }
										if ( !bPreserveBorderA ) { LinearSourceColor.A = 0.f; }

										// Accumulate the filtered color.
										FilteredColor += LinearSourceColor;
										++NumSamples;
									}
								}
							}
						}
						// Border pixels in the dest must have border pixels in the src.
						check( NumSamples > 0 );
						FilteredColor *= 1.0f/NumSamples;
					}
					else
					{
						// Consider all source pixels that pass the filter.
						for(INT FilterZ = 0;FilterZ < FilterSizeZ;FilterZ++)
						{
							for(INT FilterX = 0;FilterX < FilterSizeX;FilterX++)
							{
								for(INT FilterY = 0;FilterY < FilterSizeY;FilterY++)
								{
									// Lookup the source pixel for this component of the filter.
									const INT SourceX = DestX * FilterSizeX + FilterX;
									const INT SourceY = DestY * FilterSizeY + FilterY;
									const INT SourceZ = DestZ * FilterSizeZ + FilterZ;
									const FColor& SourceColor = *(FColor*)
										(SourceImageData.Buffer +
											SourceX * sizeof(FColor) +
											SourceY * SourceImageData.StrideY +
											SourceZ * SourceImageData.StrideZ);

									// Transform the source color into a linear color.
									FLinearColor LinearSourceColor;
									if(ImageFormat.bRGBE)
									{
										LinearSourceColor = SourceColor.FromRGBE();
									}
									else if(ImageFormat.bSRGB)
									{
										LinearSourceColor = FLinearColor(SourceColor);
									}
									else
									{
										LinearSourceColor = SourceColor.ReinterpretAsLinear();
									}

									// Accumulate the filtered color.
									FilteredColor += LinearSourceColor;
								}
							}
						}
						FilteredColor *= InverseFilterSum;
					}

					if(bDither)
					{
						// Dither the alpha of any pixel which passes an alpha threshold test.
						static const FLOAT AlphaThreshold = 5.0f / 255.0f;
						static const FLOAT MinRandomAlpha = 85.0f / 255.0f;
						static const FLOAT MaxRandomAlpha = 255.0f / 255.0f;
						if(FilteredColor.A > AlphaThreshold)
						{
							FilteredColor.A = Lerp(MinRandomAlpha,MaxRandomAlpha,RandomStream.GetFraction());
						}
					}

					// Transform the linear color into the destination format.
					FColor FormattedColor;
					if(ImageFormat.bRGBE)
					{
						FormattedColor = FilteredColor.ToRGBE();
					}
					else if(ImageFormat.bSRGB)
					{
						FormattedColor = FColor(FilteredColor);
					}
					else
					{
						FormattedColor = FilteredColor.Quantize();
					}

					// Set the destination pixel.
					FColor& DestColor = *(FColor*)
						(DestImageData.Buffer +
							DestX * sizeof(FColor) +
							DestY * DestImageData.StrideY +
							DestZ * DestImageData.StrideZ);
					DestColor = FormattedColor;
				}
			}
		}
	}
}

static void GenerateMips2D(UTexture2D* Texture)
{
	// Remove any existing non-toplevel mipmaps.
	if(Texture->Mips.Num() > 1)
	{
		// We need to flush all rendering commands before we can modify a textures' mip array.
		FlushRenderingCommands();
		// Remove non top-level mipmaps.
		Texture->Mips.Remove(1,Texture->Mips.Num() - 1);
	}

	// Only generate mip-maps for 32-bit RGBA images.
	if(Texture->Format == PF_A8R8G8B8)
	{
		// Allocate the new mipmaps.
		while(TRUE)
		{
			const INT MipIndex = Texture->Mips.Num();

			FTexture2DMipMap* const SourceMip = &Texture->Mips(MipIndex - 1);

			const INT DestSizeX = Max((INT)GPixelFormats[Texture->Format].BlockSizeX,Texture->SizeX >> MipIndex);
			const INT DestSizeY = Max((INT)GPixelFormats[Texture->Format].BlockSizeY,Texture->SizeY >> MipIndex);
			const SIZE_T DestImageSize = CalculateImageBytes(DestSizeX,DestSizeY,0,(EPixelFormat)Texture->Format);

			check(MipIndex == Texture->Mips.Num());

			FTexture2DMipMap* const DestMip = new(Texture->Mips) FTexture2DMipMap;
			DestMip->SizeX = DestSizeX;
			DestMip->SizeY = DestSizeY;
			DestMip->Data.Lock( LOCK_READ_WRITE );
			DestMip->Data.Realloc( DestImageSize );
			DestMip->Data.Unlock();

			check(Texture->Format == PF_A8R8G8B8);

			BYTE* const DestPtr = (BYTE*) DestMip->Data.Lock( LOCK_READ_WRITE );
			BYTE* const SrcPtr = (BYTE*) SourceMip->Data.Lock( LOCK_READ_ONLY );

			GenerateMipMapA8R8G8B8(
				FImageData(SrcPtr,SourceMip->SizeX,SourceMip->SizeY,1,SourceMip->SizeX * sizeof(FColor),0),
				FImageData(DestPtr,DestMip->SizeX,DestMip->SizeY,1,DestMip->SizeX * sizeof(FColor),0),
				FImageFormat(Texture->Format,Texture->SRGB,Texture->RGBE),
				Texture->bDitherMipMapAlpha,
				Texture->bPreserveBorderR,
				Texture->bPreserveBorderG,
				Texture->bPreserveBorderB,
				Texture->bPreserveBorderA
				);

			DestMip->Data.Unlock();
			SourceMip->Data.Unlock();

			// Once we've created mip-maps down to 1x1, we're done.
			if(DestSizeX == 1 && DestSizeY == 1)
			{
				break;
			}
		}
	}
	// Grayscale mip map generation
	else if ( Texture->Format == PF_G8 )
	{
		// Allocate the new mipmaps.
		while(TRUE)
		{
			const INT MipIndex = Texture->Mips.Num();

			INT DestSizeX =	Max((INT)GPixelFormats[Texture->Format].BlockSizeX,Texture->SizeX >> MipIndex);
			INT DestSizeY = Max((INT)GPixelFormats[Texture->Format].BlockSizeY,Texture->SizeY >> MipIndex);

			FTexture2DMipMap* MipMap = new(Texture->Mips) FTexture2DMipMap;

			MipMap->SizeX = DestSizeX;
			MipMap->SizeY = DestSizeY;

			SIZE_T ImageSize = CalculateImageBytes(DestSizeX,DestSizeY,0,(EPixelFormat)Texture->Format);

			MipMap->Data.Lock( LOCK_READ_WRITE );
			MipMap->Data.Realloc( ImageSize );
			MipMap->Data.Unlock();

			FTexture2DMipMap* SourceMip = &Texture->Mips(MipIndex - 1);
			FTexture2DMipMap* DestMip	= &Texture->Mips(MipIndex);

			INT	SubSizeX = SourceMip->SizeX / DestMip->SizeX;
			INT SubSizeY = SourceMip->SizeY / DestMip->SizeY;
			BYTE SubShift = appCeilLogTwo(SubSizeX) + appCeilLogTwo(SubSizeY);
			FLOAT SubScale = 1.f / (SubSizeX + SubSizeY);

			// single channel grayscale
			BYTE*	DestPtr	= (BYTE*) DestMip->Data.Lock( LOCK_READ_WRITE );
			BYTE*	SrcPtr	= (BYTE*) SourceMip->Data.Lock( LOCK_READ_ONLY );

			// generate this mip level
			for(INT Y = 0;Y < DestMip->SizeY;Y++)
			{
				for(INT X = 0;X < DestMip->SizeX;X++)
				{
					INT	R = 0;
					for(INT SubX = 0;SubX <	SubSizeX;SubX++)
					{
						for(INT SubY = 0;SubY <	SubSizeY;SubY++)
						{
							R += SrcPtr[SubY * SourceMip->SizeX + SubX];
						}
					}

					*DestPtr = R >> SubShift;
					DestPtr++;
					SrcPtr += 2;
				}
				SrcPtr += SourceMip->SizeX;
			}
			DestMip->Data.Unlock();
			SourceMip->Data.Unlock();

			// Once we've created mip-maps down to 1x1, we're done.
			if(DestSizeX == 1 && DestSizeY == 1)
			{
				break;
			}
		}
	}
}

void UTexture2D::Compress()
{
#if _MSC_VER && !CONSOLE
	Super::Compress();

	switch( Format )
	{
	case PF_A8R8G8B8:
	case PF_G8:
	case PF_DXT1:
	case PF_DXT3:
	case PF_DXT5:
		// Handled formats, break.
		break;

	case PF_Unknown:
	case PF_A32B32G32R32F:
	case PF_G16:
	default:
		// Unhandled, return.
		return;
	}

	// Return if no source art is present (maybe old package).
	if( SourceArt.GetBulkDataSize() == 0 )
	{
		return;
	}

	// Decompress source art.
	FPNGHelper PNG;
	PNG.InitCompressed( SourceArt.Lock(LOCK_READ_WRITE), SourceArt.GetBulkDataSize(), SizeX, SizeY );
	TArray<BYTE> RawData = PNG.GetRawData();
	SourceArt.Unlock();

	// Don't compress textures smaller than DXT blocksize.
	if( SizeX < 4 || SizeY < 4 )
	{
		CompressionNone = 1;
	}

	// Displacement maps get stored as PF_G8
	if( CompressionSettings == TC_Displacementmap )
	{
		Init(SizeX,SizeY,PF_G8);
		
		FTexture2DMipMap& TopLevelMip = Mips(0);

		FColor* RawColor	= (FColor*) &RawData(0);
		BYTE*	DestColor	= (BYTE*) TopLevelMip.Data.Lock(LOCK_READ_WRITE);

		for( INT i=0; i<SizeX * SizeY; i++ )
		{
			*(DestColor++)	= (RawColor++)->A;
		}

		TopLevelMip.Data.Unlock();
	}
	// Grayscale textures are stored uncompressed.
	else
	if( CompressionSettings == TC_Grayscale )
	{
		Init(SizeX,SizeY,PF_G8);
		
		FTexture2DMipMap& TopLevelMip = Mips(0);

		FColor* RawColor	= (FColor*) &RawData(0);
		BYTE*	DestColor	= (BYTE*) TopLevelMip.Data.Lock(LOCK_READ_WRITE);

		for( INT i=0; i<SizeX * SizeY; i++ )
		{
			*(DestColor++) = (RawColor++)->R;
		}

		TopLevelMip.Data.Unlock();

		// check for mip generation
		if( !CompressionNoMipmaps )
		{
			GenerateMips2D(this);
		}
	}
	// Certain textures (icons in Editor) need to be accessed by code so we can't compress them.
	else
	if( CompressionNone || (CompressionSettings == TC_HighDynamicRange && CompressionFullDynamicRange ) )
	{
		Init(SizeX,SizeY,PF_A8R8G8B8);
		
		FTexture2DMipMap& TopLevelMip = Mips(0);

		check( TopLevelMip.Data.GetBulkDataSize() == RawData.Num() );
		appMemcpy( TopLevelMip.Data.Lock(LOCK_READ_WRITE), RawData.GetData(), RawData.Num() );
		TopLevelMip.Data.Unlock();

		if( !CompressionNoMipmaps )
		{
			GenerateMips2D(this);
		}
	}
	// Regular textures.
	else
	{
		UBOOL	Opaque			= 1,
				FreeSourceData	= 0;
		FColor*	SourceData		= (FColor*) &RawData(0);

		// Artists sometimes have alpha channel in source art though don't want to use it.
		if( ! (CompressionNoAlpha || CompressionSettings == TC_Normalmap) )
		{
			// Figure out whether texture is opaque or not.
			FColor*	Color = SourceData;
			for( INT y=0; y<SizeY; y++ )
			{
				for( INT x=0; x<SizeX; x++ )
				{
					if( (Color++)->A != 255 )
					{
						Opaque = 0;
						break;
					}
				}
			}
		}

		// We need to fiddle with the exponent for RGBE textures.
		if( CompressionSettings == TC_HighDynamicRange && RGBE )
		{
			FreeSourceData	= 1;
			SourceData		= new FColor[SizeY*SizeX];
			appMemcpy( SourceData, &RawData(0), SizeY * SizeX * sizeof(FColor) );

			// Clamp exponent to -8, 7 range, translate into 0..15 and shift into most significant bits so compressor doesn't throw the data away.
			FColor*	Color = SourceData;
			for( INT y=0; y<SizeY; y++ )
			{
				for( INT x=0; x<SizeX; x++ )
				{
					Color->A = (Clamp(Color->A - 128, -8, 7) + 8) * 16;
					Color++;
				}
			}
		}

		// DXT1 if opaque (or override) and DXT5 otherwise. DXT3 is only suited for masked textures though DXT5 works fine for this purpose as well.
		EPixelFormat PixelFormat = Opaque ? PF_DXT1 : PF_DXT5;
		
		// DXT3's explicit 4 bit alpha works well with RGBE textures as we can limit the exponent to 4 bit.
		if( RGBE )
		{
			PixelFormat = PF_DXT3;
		}

		nvTextureFormats	TextureFormat	= PixelFormat == PF_DXT1 ? kDXT1 : PixelFormat == PF_DXT3 ? kDXT3 : kDXT5;
		UBOOL				bIsNormalMap	= (CompressionSettings == TC_Normalmap) || (CompressionSettings == TC_NormalmapAlpha);

		// We need to flush all rendering commands before we can modify a textures' mip array.
		if( Mips.Num() )
		{
			// Flush rendering commands.
			FlushRenderingCommands();
			// Start with a clean plate.
			Mips.Empty();
		}

		// Allocate the new mipmaps.
		while(TRUE)
		{
			const INT MipIndex = Mips.Num();

			const INT DestSizeX = Max(1,SizeX >> MipIndex);
			const INT DestSizeY = Max(1,SizeY >> MipIndex);

			check(MipIndex == Mips.Num());

			// If this isn't the top mip-map, generate the data from the previous mip-map.
			if(MipIndex > 0)
			{
				const INT SourceSizeX = Max(1,SizeX >> (MipIndex - 1));
				const INT SourceSizeY = Max(1,SizeY >> (MipIndex - 1));
				FColor* const MipMapData = new FColor[DestSizeX * DestSizeY];

				GenerateMipMapA8R8G8B8(
					FImageData((BYTE*)SourceData,SourceSizeX,SourceSizeY,1,SourceSizeX * sizeof(FColor),0),
					FImageData((BYTE*)MipMapData,DestSizeX,DestSizeY,1,DestSizeX * sizeof(FColor),0),
					FImageFormat(PF_A8R8G8B8,SRGB,RGBE),
					bDitherMipMapAlpha,
					bPreserveBorderR,
					bPreserveBorderG,
					bPreserveBorderB,
					bPreserveBorderA
					);

				if( FreeSourceData )
				{
					delete [] SourceData;
				}
				
				SourceData = MipMapData;
				FreeSourceData = TRUE;
			}

			// Constructor fills in default data for CompressionOptions.
			nvCompressionOptions nvOptions; 
			nvOptions.mipMapGeneration		= kNoMipMaps;
			nvOptions.mipFilterType			= kMipFilterBox;
			nvOptions.textureType			= kTextureTypeTexture2D;
			nvOptions.textureFormat			= TextureFormat;
			nvOptions.bEnableFilterGamma	= SRGB;
			nvOptions.filterGamma			= 2.2f;
			//nvOptions.bRGBE				= RGBE; // Clamps exponent to -8, 7 range, translates into 0..15 range and shifts into most significant bits.
			nvOptions.weightType			= bIsNormalMap ? kTangentSpaceNormalMapWeighting : kLuminanceWeighting;
			nvOptions.user_data				= this;

			// Compress...
			nvDDS::nvDXTcompress(
				(BYTE*)SourceData,			// src
				DestSizeX,					// width
				DestSizeY,					// height
				DestSizeX * sizeof(FColor),	// pitch
				nvBGRA,						// pixel order
				&nvOptions,					// compression options
				CompressionCallback			// callback
				);

			// Once we've created mip-maps down to 1x1, we're done.
			if(DestSizeX == 1 && DestSizeY == 1)
			{
				break;
			}
		}

		if( FreeSourceData )
		{
			delete [] SourceData;
			SourceData = NULL;
			FreeSourceData = FALSE;
		}

		Format = PixelFormat;
	}

	// We modified the texture data and potentially even the format so we can't stream it from disk.
	bHasBeenLoadedFromPersistentArchive = FALSE;

	// Create the texture's resource.
	UpdateResource();
#endif
}

