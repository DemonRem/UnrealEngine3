/**********************************************************************

Filename    :   GAmpRenderer.h
Content     :   Visual art profiling
Created     :   
Authors     :   
Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GAMPRENDERER_H
#define INC_GAMPRENDERER_H

#include "GRenderer.h"
#include "GHash.h"

class GAmpRenderer
{
public:
    enum ProfileFlags
    {
        Channel_Red    = 0,
	    Channel_Green  = 16,
	    Channel_Blue   = 32,
	    Profile_Fill   = 0x100,
	    Profile_Mask   = 0x200,
	    Profile_Filter = 0x400,
    };

    virtual ~GAmpRenderer() {}
    virtual void DisableProfile() = 0;
    virtual void ProfileCounters (UInt64 modes) = 0;
    virtual void GetAmpRenderStats(GRenderer::Stats* stats, bool reset) = 0;
};


template <class BaseTexture,
  class BaseRenderer,
  typename NativeTextureType = typename BaseRenderer::NativeTextureType,
  typename InitArg2 = bool
>
class GAmpTexture : public BaseTexture
{
    GAmpRenderer*     pAmpRenderer;
    GRenderer*        pRenderer;
    GPtr<BaseTexture> pBaseTexture;

    class AmpChangeHandler : public GTexture::ChangeHandler, public GNewOverrideBase<GStat_Default_Mem>
    {
        GAmpTexture* pTexture;
        GTexture::ChangeHandler *pHandler;

    public:
        AmpChangeHandler(GAmpTexture* ptex, GTexture::ChangeHandler* phandler) : pTexture(ptex), pHandler(phandler) { }

        virtual void    OnChange(GRenderer* prenderer, GTexture::ChangeHandler::EventType changeType)
        {
            GUNUSED(prenderer);
            if (changeType == GTexture::ChangeHandler::Event_RendererReleased)
                pTexture->pRenderer = 0;
            pHandler->OnChange(pTexture->pRenderer, changeType);
        }
        virtual bool    Recreate(GRenderer* prenderer)
        {
            GUNUSED(prenderer);
            return pHandler->Recreate(pTexture->pRenderer);
        }
    };

    GHashIdentityLH<GTexture::ChangeHandler*,AmpChangeHandler*> AmpHandlers;

public:
    GAmpTexture(GRenderer* prenderer, GAmpRenderer* pamprenderer, BaseTexture* ptex)
      : pAmpRenderer(pamprenderer), pRenderer(prenderer), pBaseTexture(*ptex) {}
    inline void SetRefCountZero() { this->RefCount = 0; }

    inline const BaseTexture* GetBaseTexture() const { return pBaseTexture; }
    inline BaseTexture*       GetBaseTexture() { return pBaseTexture; }

    virtual bool        InitTexture(NativeTextureType tex, InitArg2 arg2)
    {
        return pBaseTexture->InitTexture(tex, arg2);
    }
    virtual NativeTextureType GetNativeTexture() const
    {
        return pBaseTexture->GetNativeTexture();
    }

    virtual bool        InitTexture(GImageBase* pim, UInt usage = GTexture::Usage_Wrap)
    {
        return pBaseTexture->InitTexture(pim, usage);
    }

    virtual bool        InitDynamicTexture(int width, int height, GImage::ImageFormat format, int mipmaps, UInt usage)
    {
        return pBaseTexture->InitDynamicTexture(width, height, format, mipmaps, usage);
    }

    virtual void        Update(int level, int n, const GTexture::UpdateRect *rects, const GImageBase *pim)
    {
        pBaseTexture->Update(level, n, rects, pim);
    }

    virtual int         Map(int level, int n, GTexture::MapRect* maps, int flags=0)
    {
        return pBaseTexture->Map(level, n, maps, flags);
    }
    virtual bool        Unmap(int level, int n, GTexture::MapRect* maps, int flags=0)
    {
        return pBaseTexture->Unmap(level, n, maps, flags);
    }

    virtual GRenderer*  GetRenderer() const
    {
        return pRenderer;
    }
    virtual bool        IsDataValid() const
    {
        return pBaseTexture->IsDataValid();
    }

    virtual GTexture::Handle      GetUserData() const
    {
        return pBaseTexture->GetUserData();
    }
    virtual void        SetUserData(GTexture::Handle hdata)
    {
        pBaseTexture->SetUserData(hdata);
    }

    virtual void        AddChangeHandler(GTexture::ChangeHandler *phandler)
    {
        AmpChangeHandler* pamphandler = new AmpChangeHandler(this,phandler);
        AmpHandlers.Add(phandler,pamphandler);
        pBaseTexture->AddChangeHandler(pamphandler);
    }
    virtual void        RemoveChangeHandler(GTexture::ChangeHandler *phandler)
    {
        AmpChangeHandler** pamphandler = AmpHandlers.Get(phandler);
        if (pamphandler)
        {
            AmpChangeHandler* ptemp = *pamphandler;
			AmpHandlers.Remove(phandler);
            pBaseTexture->RemoveChangeHandler(ptemp);
            delete ptemp;
        }
    }
};

template <class BaseRenderer, 
  typename BaseTexture = typename BaseRenderer::TextureType,
  typename AmpTexture  = GAmpTexture<BaseTexture, BaseRenderer>
>
class GAmpRendererImpl : public BaseRenderer, public GAmpRenderer
{
    GPtr<BaseRenderer>   pBaseRenderer;

    GRenderer::BlendType OverrideBlend;
    bool                 OverrideMasks;
    bool                 OverrideFilters;
    bool                 IsDrawingMask;
    SInt                 MaskCount;
    SInt                 FilterCount;
    GRenderer::Cxform*   OverrideCxform;
    GRenderer::Matrix    CurMatrix;

    UInt                 Mode;
    GRenderer::Cxform    FillCxform, MaskCxform, FilterCxform;


    class AmpEventHandler : public GRendererEventHandler
    {
        GRenderer *pRenderer;
        GRendererEventHandler *pHandler;

    public:
        AmpEventHandler(GRenderer* prenderer, GRendererEventHandler* phandler) : pRenderer(prenderer), pHandler(phandler) { }

        virtual void    OnEvent(GRenderer* prenderer, EventType changeType)
        {
            GUNUSED(prenderer);
            pHandler->OnEvent(pRenderer, changeType);
        }
    };

    GHashIdentityLH<GRendererEventHandler*,AmpEventHandler*> AmpHandlers;

public:
    typedef BaseTexture TextureType;

private:
    const BaseTexture* GetBaseTexture(const GTexture *pti) const
    {
        return pti ? ((const AmpTexture *) pti)->GetBaseTexture() : 0;
    }
    BaseTexture* GetBaseTexture(GTexture *pti) const
    {
        return pti ? ((AmpTexture *) pti)->GetBaseTexture() : 0;
    }

public:
    GAmpRendererImpl(BaseRenderer *pbase) : pBaseRenderer(pbase)
    {
        Mode = 0;
	    OverrideBlend = GRenderer::Blend_None;
	    OverrideMasks = 0;
	    OverrideFilters = 0;
	    OverrideCxform = 0;
    }

    void DisableProfile() 
    {
    	OverrideBlend = GRenderer::Blend_None;
        OverrideMasks = 0;
        OverrideFilters = 0;
        OverrideCxform = 0;
    }

    void ProfileCounters (UInt64 modes)
    {
        DisableProfile();
    	memset (FillCxform.M_, 0, sizeof(FillCxform.M_));
	    memset (MaskCxform.M_, 0, sizeof(MaskCxform.M_));
    	FillCxform.M_[3][1] = 255;
	    MaskCxform.M_[3][1] = 255;
	    FilterCxform.M_[3][1] = 255;

	    OverrideBlend = GRenderer::Blend_Add;
	    OverrideMasks = 1;
        OverrideFilters = 1;
	    OverrideCxform = &FillCxform;

	    for (int i = 0; i < 3; i++)
	    {
	        UInt64 mode = modes >> (i << 4);
	        if (mode & Profile_Fill)
    		    FillCxform.M_[i][1] += (mode & 0xff);
	        if (mode & Profile_Mask)
	    	    MaskCxform.M_[i][1] += (mode & 0xff);
	        if (mode & Profile_Filter)
		        FilterCxform.M_[i][1] += (mode & 0xff);
	    }
    }

    virtual void GetAmpRenderStats(GRenderer::Stats* stats, bool reset)
    {
        pBaseRenderer->GetRenderStats(stats, reset);

        if (stats)
        {
            stats->Masks += MaskCount;
            stats->Filters += FilterCount;
        }

        if (reset)
        {
            MaskCount = 0;
            FilterCount = 0;
        }
    }

    virtual bool        GetRenderCaps(GRenderer::RenderCaps *pcaps)
    {
        return pBaseRenderer->GetRenderCaps(pcaps);
    }

    virtual TextureType*   CreateTexture()
    {
        BaseTexture *ptex = pBaseRenderer->CreateTexture();
	    if (!ptex)
	        return 0;
	return new AmpTexture(this, this, ptex);
    }

    virtual TextureType*   CreateTextureYUV()
    {
        BaseTexture *ptex = pBaseRenderer->CreateTextureYUV();
    	if (!ptex)
	        return 0;
	return new AmpTexture(this, this, ptex);
    }

    virtual void        BeginFrame() { pBaseRenderer->BeginFrame(); }
    virtual void        EndFrame() { pBaseRenderer->EndFrame(); }

    
    // Bracket the displaying of a frame from a movie.
    // Fill the background color, and set up default transforms, etc.   
    virtual void        BeginDisplay(
                            GColor backgroundColor,
                            const GViewport& viewport,
                            Float x0, Float x1, Float y0, Float y1)
    {
        if (OverrideBlend != GRenderer::Blend_None)
	    {
	        backgroundColor = GColor::Black;
	        backgroundColor.Channels.Alpha = 255;
	    }

    	pBaseRenderer->BeginDisplay(backgroundColor, viewport, x0,x1,y0,y1);

        if (OverrideBlend != GRenderer::Blend_None)
	        pBaseRenderer->PushBlendMode(OverrideBlend);

	    if (OverrideCxform)
	        pBaseRenderer->SetCxform(FillCxform);

	    IsDrawingMask = 0;
    }
    virtual void        EndDisplay()
    {
        if (OverrideBlend != GRenderer::Blend_None)
	        pBaseRenderer->PopBlendMode();
        pBaseRenderer->EndDisplay();
    }
    
    virtual void        SetMatrix(const GRenderer::Matrix& m)      { pBaseRenderer->SetMatrix(m); CurMatrix = m; }
    virtual void        SetUserMatrix(const GRenderer::Matrix& m)  { pBaseRenderer->SetUserMatrix(m); }
    virtual void        SetCxform(const GRenderer::Cxform& cx)     { if (!OverrideCxform) pBaseRenderer->SetCxform(cx); }

    virtual void        PushBlendMode(GRenderer::BlendType mode)
    {
        if (OverrideBlend == GRenderer::Blend_None)
	    pBaseRenderer->PushBlendMode(mode);
    }
    virtual void        PopBlendMode()
    {
        if (OverrideBlend == GRenderer::Blend_None)
	    pBaseRenderer->PopBlendMode();
    }


    virtual bool        PushUserData(GRenderer::UserData* pdata)   { return pBaseRenderer->PushUserData(pdata); }
    virtual void        PopUserData()                              { pBaseRenderer->PopUserData(); }

#ifndef GFC_NO_3D
    virtual void  SetPerspective3D(const GMatrix3D &projMatIn)    { pBaseRenderer->SetPerspective3D(projMatIn); }
    virtual void  SetView3D(const GMatrix3D &viewMatIn)           { pBaseRenderer->SetView3D(viewMatIn); }
    virtual void  SetWorld3D(const GMatrix3D *pWorldMatIn)        { pBaseRenderer->SetWorld3D(pWorldMatIn); }

    virtual void  SetStereoDisplay(GRenderer::StereoDisplay sDisplay, bool setstate = 0) { pBaseRenderer->SetStereoDisplay(sDisplay, setstate); }
    virtual void  SetStereoParams(GRenderer::StereoParams params) { pBaseRenderer->SetStereoParams(params); }
#endif

    virtual void        SetVertexData(const void* pvertices, int numVertices,
                                      GRenderer::VertexFormat vf, GRenderer::CacheProvider *pcache = 0)
    {
        GUNUSED(pcache);
        pBaseRenderer->SetVertexData(pvertices, numVertices, vf, 0);
    }

    virtual void        SetIndexData(const void* pindices, int numIndices,
                                     GRenderer::IndexFormat idxf, GRenderer::CacheProvider *pcache = 0)
    {
        GUNUSED(pcache);
        pBaseRenderer->SetIndexData(pindices, numIndices, idxf, 0);
    }

    virtual void        ReleaseCachedData(GRenderer::CachedData *pdata, GRenderer::CachedDataType type)
    {
        GUNUSED2(pdata,type);
    }

    virtual void        DrawIndexedTriList(
                                    int baseVertexIndex, int minVertexIndex, int numVertices,
                                    int startIndex, int triangleCount)
    {
        pBaseRenderer->DrawIndexedTriList(baseVertexIndex, minVertexIndex, numVertices, startIndex, triangleCount);
    }

    virtual void        DrawLineStrip(int baseVertexIndex, int lineCount)
    {
        pBaseRenderer->DrawLineStrip(baseVertexIndex, lineCount);
    }

    virtual void        LineStyleDisable()
    {
        pBaseRenderer->LineStyleDisable();
    }
    virtual void        LineStyleColor(GColor color)
    {
        pBaseRenderer->LineStyleColor(color);
    }

    virtual void        FillStyleDisable()
    {
        pBaseRenderer->FillStyleDisable();
    }
    virtual void        FillStyleColor(GColor color)
    {
        pBaseRenderer->FillStyleColor(color);
    }

    virtual void        FillStyleBitmap(const GRenderer::FillTexture* pfill)
    {
        if (pfill)
	{
	    GRenderer::FillTexture fill (*pfill);
	    fill.pTexture = GetBaseTexture (fill.pTexture);

	    pBaseRenderer->FillStyleBitmap(&fill);
	}
	else
	    pBaseRenderer->FillStyleBitmap(0);
    }

    virtual void        FillStyleGouraud(GRenderer::GouraudFillType fillType,
                                         const GRenderer::FillTexture *ptexture0 = 0,
                                         const GRenderer::FillTexture *ptexture1 = 0,
                                         const GRenderer::FillTexture *ptexture2 = 0)
    {
        GRenderer::FillTexture fills[3];
	    if (ptexture0)
            {
	        fills[0] = *ptexture0;
	        fills[0].pTexture = GetBaseTexture (fills[0].pTexture);
	        ptexture0 = &fills[0];
	    }
	    if (ptexture1)
            {
	        fills[1] = *ptexture1;
	        fills[1].pTexture = GetBaseTexture (fills[1].pTexture);
	        ptexture1 = &fills[1];
	    }
	    if (ptexture2)
            {
	        fills[2] = *ptexture2;
	        fills[2].pTexture = GetBaseTexture (fills[2].pTexture);
	        ptexture2 = &fills[2];
	    }
        pBaseRenderer->FillStyleGouraud(fillType, ptexture0, ptexture1, ptexture2);
    }

    
    virtual void        DrawBitmaps(GRenderer::BitmapDesc* pbitmapList, int listSize,
				    int startIndex, int count,
				    const GTexture* pti, const GRenderer::Matrix& m,
				    GRenderer::CacheProvider *pcache = 0 )
    {
        GUNUSED(pcache);
        pBaseRenderer->DrawBitmaps(pbitmapList, listSize, startIndex, count, GetBaseTexture(pti), m, 0);
    }

    virtual void        DrawDistanceFieldBitmaps(GRenderer::BitmapDesc* pbitmapList, int listSize,
                    int startIndex, int count,
                    const GTexture* pti, const GRenderer::Matrix& m, const GRenderer::DistanceFieldParams& params,
                    GRenderer::CacheProvider *pcache = 0 )
    {
        GUNUSED(pcache);
        pBaseRenderer->DrawDistanceFieldBitmaps(pbitmapList, listSize, startIndex, count, GetBaseTexture(pti), m, params, 0);
    }

    virtual void        BeginSubmitMask(GRenderer::SubmitMaskMode maskMode = GRenderer::Mask_Clear)
    {
	    if (OverrideCxform)
	        pBaseRenderer->SetCxform(MaskCxform);
        if (OverrideMasks)
	    {
	        MaskCount++;
	        return;
	    }
	    IsDrawingMask = 1;
	    pBaseRenderer->BeginSubmitMask(maskMode);
    }
    virtual void        EndSubmitMask()
    {
	    if (OverrideCxform)
	        pBaseRenderer->SetCxform(FillCxform);
        if (OverrideMasks)
	        return;

	    IsDrawingMask = 0;
	    pBaseRenderer->EndSubmitMask();
    }
    virtual void        DisableMask()
    {
	    if (OverrideCxform)
	        pBaseRenderer->SetCxform(FillCxform);
        if (OverrideMasks)
	        return;

	    IsDrawingMask = 0;
	    pBaseRenderer->DisableMask();
    }
    
    virtual void        GetRenderStats(GRenderer::Stats *pstats, bool resetStats = 0)
    {
        pBaseRenderer->GetRenderStats(pstats, resetStats);
        if (pstats != NULL)
        {
            pstats->Masks += MaskCount;
        }
        if (resetStats)
        {
            MaskCount = 0;
        }
    }

    virtual void        GetStats(GStatBag* pbag, bool reset = true)
    {
	// XXX mask & filter count
        pBaseRenderer->GetStats(pbag,reset);
    }

    virtual void        ReleaseResources()
    {
        pBaseRenderer->ReleaseResources();
    }

    virtual bool        AddEventHandler(GRendererEventHandler *phandler)
    {
        bool result = false;
        if (!phandler->GetRenderer())
        {
            AmpEventHandler* pamphandler = new AmpEventHandler(this,phandler);
            result = pBaseRenderer->AddEventHandler(pamphandler);
            if (result)
                AmpHandlers.Add(phandler,pamphandler);
            else
                delete pamphandler;
            phandler->BindRenderer(this);
        }
        return result;
    }

    virtual void        RemoveEventHandler(GRendererEventHandler *phandler)
    {
        AmpEventHandler** pamphandler = AmpHandlers.Get(phandler);
        if (pamphandler)
        {
            pBaseRenderer->RemoveEventHandler(*pamphandler);

            delete *pamphandler; //!AB the order: delete, then Remove, not other way around!
            AmpHandlers.Remove(phandler);
        }
    }

    virtual typename BaseRenderer::DisplayStatus CheckDisplayStatus() const
    {
        return pBaseRenderer->CheckDisplayStatus();
    }

    //
    // Render Target Support - pass thru methods
    //
    virtual GRenderTarget *CreateRenderTarget()
    {
        return pBaseRenderer->CreateRenderTarget();
    }

    virtual void SetDisplayRenderTarget(GRenderTarget *renderTarget, bool bSetState = true)
    {
        return pBaseRenderer->SetDisplayRenderTarget(renderTarget, bSetState);
    }

    virtual void PushRenderTarget(const GRectF& frameRect, GRenderTarget* prt)
    {
        return pBaseRenderer->PushRenderTarget(frameRect, prt);
    }

    virtual void PopRenderTarget()
    {
        return pBaseRenderer->PopRenderTarget();
    }

    virtual BaseTexture *PushTempRenderTarget(const GRectF& frameRect, UInt w, UInt h, bool wantStencil = 0)
    {
        BaseTexture* ptmptex = pBaseRenderer->PushTempRenderTarget(frameRect, w, h, wantStencil);
	    AmpTexture* pamptex = new AmpTexture(this, this, ptmptex);
	    ptmptex->AddRef();
	    pamptex->SetRefCountZero();
	    return pamptex;
    }

    virtual void ReleaseTempRenderTargets(UInt keepArea)
    {
	pBaseRenderer->ReleaseTempRenderTargets(keepArea);
    }

    virtual void DrawBlurRect(GTexture* psrcin, const GRectF& insrcrect, const GRectF& destrect, const GRenderer::BlurFilterParams& params)
    {
	    psrcin = GetBaseTexture(psrcin);

	    if (OverrideFilters)
	    {
	        Float cmatrix[20];
	        memset(cmatrix, 0, sizeof(Float) * 20);
	        cmatrix[0] = cmatrix[5] = cmatrix[10] = cmatrix[15] = 1;
	        for (UInt i = 0; i < 4; i++)
		        cmatrix[16+i] = FilterCxform.M_[i][1];

	        pBaseRenderer->DrawColorMatrixRect(psrcin, insrcrect, destrect, cmatrix);
	    }
	    else
	        pBaseRenderer->DrawBlurRect(psrcin, insrcrect, destrect, params);
    }

    virtual void DrawColorMatrixRect(GTexture* psrcin, const GRectF& insrcrect, const GRectF& destrect, const Float *matrix)
    {
	    psrcin = GetBaseTexture(psrcin);

	    if (OverrideFilters)
	    {
	        Float cmatrix[20];
	        memset(cmatrix, 0, sizeof(Float) * 20);
	        cmatrix[0] = cmatrix[5] = cmatrix[10] = cmatrix[15] = 1;
            for (UInt i = 0; i < 4; i++)
                cmatrix[16+i] = FilterCxform.M_[i][1] * 1.f/255.f;

	        pBaseRenderer->DrawColorMatrixRect(psrcin, insrcrect, destrect, cmatrix);
	    }
	    else
	        pBaseRenderer->DrawColorMatrixRect(psrcin, insrcrect, destrect, matrix);
    }

    virtual UInt CheckFilterSupport(const GRenderer::BlurFilterParams &bfp) 
    { 
        return pBaseRenderer->CheckFilterSupport(bfp);
    }

};

#endif
