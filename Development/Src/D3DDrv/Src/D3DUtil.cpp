/*=============================================================================
	D3DUtil.h: D3D RHI utility implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "D3DDrvPrivate.h"

#if USE_D3D_RHI || USE_NULL_RHI

FString GetD3DErrorString(HRESULT ErrorCode)
{
	FString ErrorCodeText;
#define D3DERR(x) case x: ErrorCodeText = TEXT(#x); break;
	switch(ErrorCode)
	{
		D3DERR(D3D_OK)
		D3DERR(D3DERR_WRONGTEXTUREFORMAT)
		D3DERR(D3DERR_UNSUPPORTEDCOLOROPERATION)
		D3DERR(D3DERR_UNSUPPORTEDCOLORARG)
		D3DERR(D3DERR_UNSUPPORTEDALPHAOPERATION)
		D3DERR(D3DERR_UNSUPPORTEDALPHAARG)
		D3DERR(D3DERR_TOOMANYOPERATIONS)
		D3DERR(D3DERR_CONFLICTINGTEXTUREFILTER)
		D3DERR(D3DERR_UNSUPPORTEDFACTORVALUE)
		D3DERR(D3DERR_CONFLICTINGRENDERSTATE)
		D3DERR(D3DERR_UNSUPPORTEDTEXTUREFILTER)
		D3DERR(D3DERR_CONFLICTINGTEXTUREPALETTE)
		D3DERR(D3DERR_DRIVERINTERNALERROR)
		D3DERR(D3DERR_NOTFOUND)
		D3DERR(D3DERR_MOREDATA)
		D3DERR(D3DERR_DEVICELOST)
		D3DERR(D3DERR_DEVICENOTRESET)
		D3DERR(D3DERR_NOTAVAILABLE)
		D3DERR(D3DERR_OUTOFVIDEOMEMORY)
		D3DERR(D3DERR_INVALIDDEVICE)
		D3DERR(D3DERR_INVALIDCALL)
		D3DERR(D3DERR_DRIVERINVALIDCALL)
		D3DERR(D3DXERR_INVALIDDATA)
		D3DERR(E_OUTOFMEMORY)
		default: ErrorCodeText = FString::Printf(TEXT("%08X"),(INT)ErrorCode);
	}
#undef D3DERR
	return ErrorCodeText;
}

FString GetD3DTextureFormatString(D3DFORMAT TextureFormat)
{
	FString TextureFormatText;
#define D3DFORMATCASE(x) case x: TextureFormatText = TEXT(#x); break;
	switch(TextureFormat)
	{
		D3DFORMATCASE(D3DFMT_A8R8G8B8)
		D3DFORMATCASE(D3DFMT_X8R8G8B8)
		D3DFORMATCASE(D3DFMT_DXT1)
		D3DFORMATCASE(D3DFMT_DXT3)
		D3DFORMATCASE(D3DFMT_DXT5)
		D3DFORMATCASE(D3DFMT_A16B16G16R16F)
		D3DFORMATCASE(D3DFMT_A32B32G32R32F)
		D3DFORMATCASE(D3DFMT_UNKNOWN)
		D3DFORMATCASE(D3DFMT_L8)
		D3DFORMATCASE(D3DFMT_UYVY)
		D3DFORMATCASE(D3DFMT_D24S8)
		D3DFORMATCASE(D3DFMT_D24X8)
		D3DFORMATCASE(D3DFMT_R32F)
		D3DFORMATCASE(D3DFMT_G16R16)
		D3DFORMATCASE(D3DFMT_G16R16F)
		D3DFORMATCASE(D3DFMT_G32R32F)
		D3DFORMATCASE(D3DFMT_A2B10G10R10)
		D3DFORMATCASE(D3DFMT_A16B16G16R16)
		default: TextureFormatText = FString::Printf(TEXT("%08X"),(INT)TextureFormat);
	}
#undef D3DFORMATCASE
	return TextureFormatText;
}

FString GetD3DTextureFlagString(DWORD TextureFlags)
{
	FString TextureFormatText = TEXT("");

	if (TextureFlags & D3DUSAGE_DEPTHSTENCIL)
	{
		TextureFormatText += TEXT("D3DUSAGE_DEPTHSTENCIL ");
	}

	if (TextureFlags & D3DUSAGE_RENDERTARGET)
	{
		TextureFormatText += TEXT("D3DUSAGE_RENDERTARGET ");
	}

	return TextureFormatText;
}

void VerifyD3DResult(HRESULT D3DResult,const ANSICHAR* Code,const ANSICHAR* Filename,UINT Line)
{
	if(FAILED(D3DResult))
	{
		const FString& ErrorString = GetD3DErrorString(D3DResult);
		appErrorf(TEXT("%s failed \n at %s:%u \n with error %s"),ANSI_TO_TCHAR(Code),ANSI_TO_TCHAR(Filename),Line,*ErrorString);
	}
}

void VerifyD3DCreateTextureResult(HRESULT D3DResult,const ANSICHAR* Code,const ANSICHAR* Filename,UINT Line,UINT SizeX,UINT SizeY,BYTE Format,UINT NumMips,DWORD Flags)
{
	if(FAILED(D3DResult))
	{
		const FString& ErrorString = GetD3DErrorString(D3DResult);
		const FString& D3DFormatString = GetD3DTextureFormatString((D3DFORMAT)GPixelFormats[Format].PlatformFormat);
		appErrorf(
			TEXT("%s failed \n at %s:%u \n with error %s, \n SizeX=%i, SizeY=%i, Format=%s=%s, NumMips=%i, Flags=%s, TexMemoryAvailable=%dMB"),
			ANSI_TO_TCHAR(Code),
			ANSI_TO_TCHAR(Filename),
			Line,
			*ErrorString, 
			SizeX, 
			SizeY, 
			GPixelFormats[Format].Name, 
			*D3DFormatString, NumMips, 
			*GetD3DTextureFlagString(Flags),
			RHIGetAvailableTextureMemory());
	}
}

/**
 * Adds a PIX event using the D3DPerf api
 *
 * @param Color The color to draw the event as
 * @param Text The text displayed with the event
 */
void appBeginDrawEvent(const FColor& Color,const TCHAR* Text)
{
	D3DPERF_BeginEvent(Color.DWColor(),Text);
}

/**
 * Ends the current PIX event
 */
void appEndDrawEvent(void)
{
	D3DPERF_EndEvent();
}

//
// Stat declarations.
//

DECLARE_STATS_GROUP(TEXT("RHI"),STATGROUP_RHI);
DECLARE_CYCLE_STAT(TEXT("Present time"),STAT_PresentTime,STATGROUP_RHI);
DECLARE_DWORD_COUNTER_STAT(TEXT("DrawPrimitive calls"),STAT_DrawPrimitiveCalls,STATGROUP_RHI);
DECLARE_DWORD_COUNTER_STAT(TEXT("Triangles drawn"),STAT_RHITriangles,STATGROUP_RHI);
DECLARE_DWORD_COUNTER_STAT(TEXT("Lines drawn"),STAT_RHILines,STATGROUP_RHI);

#endif
