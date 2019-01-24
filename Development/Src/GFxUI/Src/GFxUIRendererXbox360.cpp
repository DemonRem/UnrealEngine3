/**********************************************************************

Filename    :   GFxUIRendererXbox360.cpp
Content     :   GFx GRenderer implementation for UE3, XboX360 specific

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#define GFXUIRENDERER_MIN_TEXTURE_SIZE 32

void FGFxTexture::LoadMipLevel(int Level, int DestPitch, const UByte* Src, UInt w, UInt h, SInt Bpp, UInt Pitch)
{
    if (Texture2D)
    {
        FTexture2DMipMap& Tex = Texture2D->Mips(Level);

        Texture2D->RequestedMips = 1;
        UByte* Dest = (UByte*)Tex.Data.Lock(LOCK_READ_WRITE);
        UInt Destbpp = Bpp > 1 ? 4 : 1;

        check(w >= GFXUIRENDERER_MIN_TEXTURE_SIZE);
        check(h >= GFXUIRENDERER_MIN_TEXTURE_SIZE);

        D3DTexture Dummy;
        XGTEXTURE_DESC Desc;
        XGSetTextureHeaderEx( w,h, 1, 0, Bpp == 1 ? D3DFMT_A8 : D3DFMT_A8R8G8B8, 0, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, &Dummy, NULL, NULL);
        XGGetTextureDesc( &Dummy, Level, &Desc );

        Dest = (UByte*)Tex.Data.Realloc(Desc.SlicePitch);
        BYTE* Tmp = new BYTE[Desc.SlicePitch];

        if (Bpp == 4)
		{
            for (UInt j = 0; j < h; j++)
			{
                for (UInt i = 0; i < w; i++)
                {
                    UByte *p = Tmp + j * Desc.RowPitch + i * 4;
                    p[1] = Src[j*Pitch+i*4+0];
                    p[2] = Src[j*Pitch+i*4+1];
                    p[3] = Src[j*Pitch+i*4+2];
                    p[0] = Src[j*Pitch+i*4+3];
                }
			}
		}
        else if (Bpp == 3)
		{
            for (UInt j = 0; j < h; j++)
			{
                for (UInt i = 0; i < w; i++)
                {
                    UByte *p = Tmp + j * Desc.RowPitch + i * 4;
                    p[1] = Src[j*Pitch+i*3+0];
                    p[2] = Src[j*Pitch+i*3+1];
                    p[3] = Src[j*Pitch+i*3+2];
                    p[0] = 255;
                }
			}
		}
        else if (Bpp == 1)
		{
            for (UInt j = 0; j < h; j++)
			{
                for (UInt i = 0; i < w; i++)
				{
                    Tmp[j*Desc.RowPitch+i] = Src[j*Pitch+i];
				}
			}
		}
        else
		{
            check(0);
		}

        if( XGIsTiledFormat(Desc.Format) )
        {
            RECT  Rect = {0, 0, w, h};
            XGTileSurface(Dest, w, h, NULL, Tmp, Desc.RowPitch, &Rect, Desc.BytesPerBlock);
        }
        else
		{
            appMemcpy(Dest, Tmp, Desc.SlicePitch);
		}

        delete [] Tmp;	

        Tex.Data.Unlock();
    }
}
