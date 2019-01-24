/**********************************************************************

Filename    :   GFxUIRendererPS3.cpp
Content     :   GFx GRenderer implementation for UE3, PS3 specific

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

namespace FGFxRendererImpl
{
	static UByte* LoadTextureSzBlock(UByte* Dest, const UByte* Src, UInt w, UInt h, SInt Bpp, SInt Pitch, UInt x, UInt y, UInt b)
	{
		if (b == 1)
		{
			switch (Bpp)
			{
			case 4:
				Dest[2] = Src[y*Pitch+x*4+0];
				Dest[1] = Src[y*Pitch+x*4+1];
				Dest[0] = Src[y*Pitch+x*4+2];
				Dest[3] = Src[y*Pitch+x*4+3];
				return Dest + 4;

			case 3:
				Dest[3] = 255;
				Dest[2] = Src[y*Pitch+x*3+0];
				Dest[1] = Src[y*Pitch+x*3+1];
				Dest[0] = Src[y*Pitch+x*3+2];
				return Dest + 4;

			case 1:
				Dest[0] = Src[y*Pitch+x];
				return Dest + 1;

			default:
				return 0;
			}
		}
		else
		{
			Dest = LoadTextureSzBlock(Dest, Src, w,h,Bpp,Pitch, x, y, b-1);
			Dest = LoadTextureSzBlock(Dest, Src, w,h,Bpp,Pitch, x + (1<<(b-2)), y, b-1);
			Dest = LoadTextureSzBlock(Dest, Src, w,h,Bpp,Pitch, x, y + (1<<(b-2)), b-1);
			return  LoadTextureSzBlock(Dest, Src, w,h,Bpp,Pitch, x + (1<<(b-2)), y + (1<<(b-2)), b-1);
		}
	}

	static UByte* LoadTextureSz(UByte* Dest, const UByte* Src, UInt w, UInt h, SInt Bpp, SInt Pitch)
	{
		UInt    lw = 0; while (w != (1U << lw)) lw++;
		UInt    lh = 0; while (h != (1U << lh)) lh++;

		if (w == h)
		{
			return LoadTextureSzBlock(Dest, Src, w,h,Bpp,Pitch, 0,0,lw+1);
		}
		else if (w > h)
		{
			for (UInt i = 0; i < (1U << (lw-lh)); i++)
			{
				Dest = LoadTextureSzBlock(Dest, Src, w,h,Bpp,Pitch, i<<lh, 0, lh+1);
			}
		}
		else
		{
			for (UInt i = 0; i < (1U << (lh-lw)); i++)
			{
				Dest = LoadTextureSzBlock(Dest, Src, w,h,Bpp,Pitch, 0, i<<lw, lw+1);
			}
		}

		return Dest;
	}
}

void FGFxTexture::LoadMipLevel(int Level, int DestPitch, const UByte* Src, UInt w, UInt h, SInt Bpp, UInt Pitch)
{
    if (Texture2D)
    {
        FTexture2DMipMap& tex = Texture2D->Mips(Level);
        UByte* Dest = (UByte*)tex.Data.Lock(LOCK_READ_WRITE);
        FGFxRendererImpl::LoadTextureSz(Dest, Src, w, h, Bpp, Pitch);
        tex.Data.Unlock();
    }
}
