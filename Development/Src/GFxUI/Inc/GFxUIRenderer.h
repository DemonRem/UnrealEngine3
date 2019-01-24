/**********************************************************************

Filename    :   GFxUIRenderer.h
Content     :   GFx GRenderer implementation header for UE3, defines
                the FGFxRenderer class wrapped around RHI.

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Portions of the integration code is from Epic Games as identified by Perforce annotations.
Copyright 2010 Epic Games, Inc. All rights reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef GFxUIRenderer_h
#define GFxUIRenderer_h

#if WITH_GFx

#include "GFxUIStats.h"

#if SUPPORTS_PRAGMA_PACK
#pragma pack(push, 8)
#endif

#include "GRenderer.h"
#include "GRendererCommonImpl.h"
#include "GThreads.h"

#if SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif


class UGFxInteraction;

#ifndef GFxUIRenderer_Common_h
#define GFxUIRenderer_Common_h

class FGFxVertexShaderInterface
{
public:
    virtual void SetParameters(
        FVertexShaderRHIParamRef VertexShader,
        const FMatrix& Transform,
        const FMatrix& TextureMatrix,
        const FMatrix& TextureMatrix2 ) const = 0;

    virtual void SetParameterTransform(FVertexShaderRHIParamRef VertexShader, const FMatrix& Transform) const = 0;
    virtual void SetParameterTextureMatrix(FVertexShaderRHIParamRef VertexShader, const FMatrix& TextureMatrix, INT i) const = 0;

    // Hooks for bound shader state caching and parameter manipulating
    virtual FShader*                   GetNativeShader()    = 0;
    virtual FGFxVertexShaderInterface* GetShaderInterface() = 0;
};

class FGFxPixelShaderInterface
{
public:

    //typedef GRenderer::BitmapSampleMode BitmapSampleMode;
    //typedef GRenderer::BitmapWrapMode   BitmapWrapMode;

    virtual void SetParameterTextureRHI(FPixelShaderRHIParamRef PixelShader,FSamplerStateRHIParamRef SamplerState,FTextureRHIParamRef Texture,INT i=0) const = 0;
    inline void SetParameterTexture(FPixelShaderRHIParamRef PixelShader,FSamplerStateRHIParamRef SamplerState,const FTexture* Texture,INT i=0) const
    {
        SetParameterTextureRHI(PixelShader, SamplerState, Texture->TextureRHI, i);
    }
    virtual void SetParameterConstantColor(FPixelShaderRHIParamRef PixelShader,const FLinearColor& ConstantColor) const = 0;
    virtual void SetParametersColorScaleAndColorBias(FPixelShaderRHIParamRef PixelShader,const GRenderer::Cxform& ColorXForm) const = 0;
    virtual void SetParametersCxformAc(FPixelShaderRHIParamRef PixelShader,const GRenderer::Cxform& ColorXForm) const {}
    virtual void SetParameterColorMatrix(FPixelShaderRHIParamRef PixelShader,const FLOAT* ColorMatrix) const {}
    virtual void SetParameterTexScale(FPixelShaderRHIParamRef PixelShader,INT N, FLOAT XScale, FLOAT YScale) const {}
    virtual void SetParameterFilterSize4(FPixelShaderRHIParamRef PixelShader, FLOAT SizeX, FLOAT SizeY, FLOAT Z, FLOAT W) const {}
    inline  void SetParameterFilterSize(FPixelShaderRHIParamRef PixelShader, FLOAT SizeX, FLOAT SizeY)
    {
        SetParameterFilterSize4(PixelShader, SizeX, SizeY, 0, 1.0f/((SizeX*2+1)*(SizeY*2+1)));
    }
    virtual void SetParameterShadowColor(FPixelShaderRHIParamRef PixelShader,INT N, GColor Color) const {}
    virtual void SetParameterShadowOffset(FPixelShaderRHIParamRef PixelShader, FLOAT SizeX, FLOAT SizeY) const {}
    virtual void SetParameterInverseGamma(FPixelShaderRHIParamRef PixelShader,const FLOAT InverseGamma) const = 0;

    virtual void SetDistanceFieldParams(FPixelShaderRHIParamRef PixelShader, const class UTexture2D* Texture, const GRenderer::DistanceFieldParams& Params) const = 0;

    // Hooks for bound shader state caching and parameter manipulating.
    virtual FShader*                  GetNativeShader()    = 0;
    virtual FGFxPixelShaderInterface* GetShaderInterface() = 0;
};

enum EGViewportFlags
{
    GViewport_NoGamma       = 0x1000,
    GViewport_InverseGamma  = 0x2000,
};


class FGFxRenderResources : public GRefCountBase<FGFxRenderResources, GStat_Default_Mem>, public FRenderResource
{
public:
    INT                 SizeX, SizeY;
    FSurfaceRHIRef      DepthSurface;

    FGFxRenderResources()
    {
        SizeY = SizeX = 0;
    }
    ~FGFxRenderResources();

    UInt            GetRefCount() const { return RefCount; }

    void            Allocate_RenderThread(INT InSizeX, INT InSizeY);
    FSurfaceRHIRef  GetDepthSurface() const { return DepthSurface; }

    virtual void InitDynamicRHI();
    virtual void ReleaseDynamicRHI();
};

struct FGFxRenderTargetResource : public FRenderResource
{
    struct NativeRenderTarget
    {
        FRenderTarget*                  Owner;
        class FSceneDepthTargetProxy*   OwnerDepth;

        NativeRenderTarget()
        {
            Owner = NULL;
            OwnerDepth = NULL;
        }
    };

    // Main thread
    FRenderTarget*                  Owner;
    class FSceneDepthTargetProxy*   OwnerDepth;
    SInt                            SizeX, SizeY; // actual size

    // Render thread
    FLOAT                           InverseGamma;
    FTexture2DRHIRef                ColorTexture;
    FSurfaceRHIRef                  ColorBuffer;
    FSurfaceRHIRef                  DepthBuffer;
    UInt                            StatSize;

    FGFxRenderTargetResource()
    {
        Owner = NULL;
        OwnerDepth = NULL;
        StatSize = 0;
    }
    ~FGFxRenderTargetResource();

    bool        InitRenderTarget(const NativeRenderTarget& params);
    bool        InitRenderTarget_RenderThread(const NativeRenderTarget& params);

    // FRenderResource
    virtual void InitDynamicRHI();
    virtual void ReleaseDynamicRHI();
};


//---------------------------------------------------------------------------------------------------------------------
// GFx UI Viewport
//---------------------------------------------------------------------------------------------------------------------

struct FGFxViewportAxisInfo
{
    // Pixel coordinate info
    FLOAT   PixelStart;
    FLOAT   PixelEnd;
    FLOAT   PixelClipStart;
    FLOAT   PixelClipEnd;
    FLOAT   PixelClipLength;
    // Window coordinate info
    INT     ViewStart;
    INT     ViewLength;
    INT     ViewLengthMax;
};

struct FGFxViewportUserParams
{
    FGFxViewportAxisInfo xAxis;				// x-axis data
    FGFxViewportAxisInfo yAxis;				// y-axis data
};

#endif

//---------------------------------------------------------------------------------------------------------------------
// FGFxTexture
//---------------------------------------------------------------------------------------------------------------------

class FGFxTexture : public GTexture
{
public:
    FGFxTexture(GRenderer* const InRenderer) : 
		Renderer(InRenderer),
        Texture(NULL), 
		Texture2D(NULL),
		RenderTarget(NULL) 
	{ }

    virtual ~FGFxTexture();

    void ReleaseTexture();

    virtual bool InitTexture(GImageBase* pim, UInt Usage = Usage_Wrap);

    virtual bool InitTexture(UTexture* ptexture, bool unused = 0);

    // Assumes pfilename is actually "Package.TextureName..." format
    virtual bool InitTextureFromFile(const char* Filename)
    {
        // Attempt to load texture
        Texture = Texture2D = StaticInternalLoadTexture( ANSI_TO_TCHAR( Filename ) );
        if (!Texture2D)
		{
            return FALSE;
		}

        // GC
        const UBOOL Result = GGFxGCManager->AddGCReferenceFor(Texture, STAT_GFxTextureCount);
        check(Result);
        return TRUE;
    }

    // Init initially empty updateable texture
    virtual bool        InitDynamicTexture(int Width, int Height, GImage::ImageFormat Format, int Mipmaps, UInt Usage);

    virtual void        Update(int Level, int N, const UpdateRect* Rects, const GImageBase* Pim);

    // must be called from render thread
    virtual int         Map(int Level, int N, MapRect* Maps, int Flags);
    virtual bool        Unmap(int Level, int N, MapRect* Maps, int Flags);

    virtual GRenderer*  GetRenderer() const { return Renderer; }
    virtual bool        IsDataValid() const { return (Texture2D != NULL || RenderTarget != NULL); }
    virtual Handle      GetUserData() const { return Handle(); }
    virtual void        SetUserData(Handle HData) { }
    // Add/Remove notification
    virtual void        AddChangeHandler(ChangeHandler* Handler) {}
    virtual void        RemoveChangeHandler(ChangeHandler* Handler) {}

    virtual int         IsYUVTexture() const { return 0; }

    // FGFxTexture
    FTexture*           GetTextureResource()       { return Texture ? Texture->Resource : 0; }
    const FTexture*     GetTextureResource() const { return Texture ? Texture->Resource : 0; }
    UTexture*           GetNativeTexture() const   { return Texture; }

    virtual void        Bind(
		int StageIndex, 
		FGFxPixelShaderInterface& PixelShader,
        GRenderer::BitmapWrapMode WrapMode, 
		GRenderer::BitmapSampleMode SampleMode, 
		bool bUseMipmaps) const;

    UPInt               GetSize() const  { return Texture2D ? (UPInt)Texture2D->CalcTextureMemorySize( TMC_AllMips ) : 0; }

	UBOOL				UsesMips() const { return (Texture && Texture->LODGroup == TEXTUREGROUP_UI) ? FALSE : TRUE; }

protected:
    friend class FGFxRenderer;

    static UTexture2D* StaticInternalLoadTexture(const TCHAR* const ResourceName)
    {
        if (!ResourceName)
		{
            return NULL;
		}
        return LoadObject<UTexture2D>(NULL, ResourceName, NULL, LOAD_None, NULL);
    }

    void LoadMipLevel(int Level, int Destpitch, const UByte* Src, UInt W, UInt H, SInt Bpp, UInt Pitch);
    void Update_RenderThread(int Level, int N, const FUpdateTextureRegion2D* Rects, const GImageBase* Pim);

    // Garbage collection
    virtual void InternalTermGCState();

    // No copy or assignment allowed
    FGFxTexture(const FGFxTexture&);
    FGFxTexture& operator=(const FGFxTexture&);

    // Data
    GRenderer* const Renderer;
    UTexture*        Texture;
    UTexture2D*      Texture2D;

    class FGFxRenderTarget* RenderTarget;

    friend class FGFxRenderTarget;
    friend struct FGFxRenderTargetResource;
};

class FGFxRenderTarget : public GRenderTarget
{
public:
    typedef FGFxRenderTargetResource::NativeRenderTarget NativeRenderTarget;
    FGFxRenderTarget(GRenderer* prenderer) : Renderer(prenderer), Texture(NULL), Resource(*new FGFxRenderTargetResource) { IsTemp = 0; }
    ~FGFxRenderTarget();

    virtual bool        InitRenderTarget(GTexture *ptarget, GTexture* pdepth = 0, GTexture* pstencil = 0);
    virtual bool        InitRenderTarget_RenderThread(GTexture *ptarget, FGFxRenderResources* pstencil, UInt InWidth, UInt InHeight);

    virtual bool        InitRenderTarget(const NativeRenderTarget& params)
    {
        Texture = NULL;
#ifdef FGFxRenderTarget
        if (!Resource.InitRenderTarget_RenderThread(params))
            return FALSE;
#else
        if (!Resource.InitRenderTarget(params))
            return FALSE;
#endif
        TargetWidth = Resource.SizeX;
        TargetHeight = Resource.SizeY;
        return TRUE;
    }
    virtual bool        InitRenderTarget_RenderThread(const NativeRenderTarget& params)
    {
        Texture = NULL;
        if (!Resource.InitRenderTarget_RenderThread(params))
            return FALSE;
        TargetWidth = Resource.SizeX;
        TargetHeight = Resource.SizeY;
        return TRUE;
    }

    bool    AdjustBounds(Float* InWidth, Float* InHeight);

    virtual GRenderer*  GetRenderer() const { return Renderer; }
    virtual Handle      GetUserData() const { return Handle(); }
    virtual void        SetUserData(Handle hdata) { }
    virtual void        AddChangeHandler(ChangeHandler *phandler) {}
    virtual void        RemoveChangeHandler(ChangeHandler *phandler) {}


    GRenderer* const            Renderer;
    GPtr<FGFxTexture>           Texture; // can be null
    GPtr<FGFxRenderResources>   StencilBuffer;
    FGFxRenderTargetResource&   Resource;
    UInt                        TargetWidth, TargetHeight; // usable size
    bool                        IsTemp;
};


namespace FGFxRendererImpl
{
    struct FGFxRenderElementStoreBase;
    struct FGFxVertexStore;
    struct FGFxIndexStore;
    struct FGFxBitmapDescStore;

    enum 
	{
        GFxCreateTex_Normal     = 1,
        GFxCreateTex_Updatable  = 2,
        GFxCreateTex_Mappable   = 3,
    };

    UTexture2D* CreateNativeTexture(GRenderer* pRenderer, INT Type, INT StatId);
};

class FGFxRenderer : public GRenderer
{
    friend struct FGFxRendererImpl::FGFxRenderElementStoreBase;

	Stats RenderStats;
public:

	// GFx analogue to GRenderer::FillTexture, exposed to allow cleaner render thread access
	class FFillTextureInfo
	{
	public:
		FTexture*        Texture;
		Matrix           TextureMatrix;
		BitmapWrapMode   WrapMode;
		BitmapSampleMode SampleMode;
		UBOOL            bUseMips;
		
        FFillTextureInfo() : Texture(NULL) { }
		void EndDisplay_RenderThread();
	};

public:
    typedef FGFxTexture       TextureType;
    typedef UTexture*         NativeTextureType;

    // Construction and destruction
	FGFxRenderer();
	virtual ~FGFxRenderer();

	// Hooks expected by UI Engine
    void                Render(class GFxMovieView& Movie);

    GLock*              GetElementsLock() { return &ElementAccessLock; }

	/************************************************************************/
	/* GRenderer                                                            */
	/************************************************************************/

    // Releases all system resources. The method is used when this renderer is used 
    // in multi-threaded environment and should be called on the main thread just before 
    // the last reference on this render goes out of scope. All following calls to the renderer are illegal. 
    void ReleaseResources();

    /**
	* Begins the display of a frame from a movie. Fill the background and set up default transforms, etc.
	* All rendering calls must take place after this function and before EndDisplay.
	* The rectangle formed by {x0, y0, x1, y1} defines the pixel coordinates of the movie that correspond
	* to the viewport bounds. By modifying this rectangle views on any section within the movie can be created.
	*/
	void BeginDisplay(GColor backgroundColor, const GViewport& Viewport, Float x0, Float x1, Float y0, Float y1);

	/**
	* Begins rendering of a submit mask that can be used to cull other rendered contents.
	* All shapes submitted from this point on will generate a mask.
	*/
	void BeginSubmitMask(SubmitMaskMode MaskMode = Mask_Clear);

	// Render-thread analogue of BeginSubmitMask
	void BeginSubmitMask_RenderThread(const SubmitMaskMode MaskMode);

	/**
	* Creates a texture object that can be initialized with an image later on.
	* CreateTexture returns created objects with a refCount of 1, which must be user-released.
    * Used when DO_NOT_LOAD_BITMAPS is set.
	*/
	FGFxTexture* CreateTexture();
    FGFxTexture* CreateTextureYUV() { return NULL; }

	/**
	* Disables the use of a submit mask, all content is fully rendered from this point on.
	* The mask is originally generated by rendering shapes after a call to BeginSubmitMask;
	* it is used after EndSubmitMask.
	*/
	void DisableMask();

	// Render-thread analogue of DisableMask()
	void DisableMask_RenderThread();

	/** TODO: Support caching
	* Draws a set of transformed bitmaps; intended for glyph rendering.
	*/
	void DrawBitmaps(BitmapDesc* BitmapList, int ListSize, int StartIndex, int Count, const GTexture* Ti, const Matrix& M, CacheProvider* Cache = 0);
    void DrawDistanceFieldBitmaps(BitmapDesc* BitmapList, int ListSize, int StartIndex, int Count, const GTexture* Ti, const Matrix& M, const DistanceFieldParams& Params, CacheProvider* Cache = 0);

	// Render-thread analogue of DrawBitmaps
	// Note: BitmapDesc* pOwnedBmDescList is destroyed within this function
    void DrawBitmaps_RenderThread(
		FGFxRendererImpl::FGFxBitmapDescStore* BmpDescStore,
		const INT StartIndex, 
		const INT Count, 
		const FGFxTexture* Texture,
        const Matrix& FontMatrix,
        const DistanceFieldParams* Params);

	/** TODO: Convert to use dynamic vertex and index buffers, support caching
	* Draw triangles using the current fill-style.
	* Vertices/Indices must have been specified by SetVertexData/SetDrawIndexData. First accessed
	* vertex will be at baseVertexIndex + minVertexIndex and up to NumVertices from that point
	* on will be accessed. The valid index range is [minVertexIndex, minVertexIndex + NumVertices).
	* Index data will be accessed beginning with startIndex, consuming triangleCount*3 indices.
	*/
	void DrawIndexedTriList( int BaseVertexIndex, int MinVertexIndex, int NumVertices, int StartIndex, int TriangleCount );

	// Render-thread analogue of DrawIndexedTriList
	void DrawIndexedTriList_RenderThread( const INT BaseVertexIndex, const INT MinVertexIndex, const INT NumVertices, const INT StartIndex, const INT TriangleCount );

	/**
	* Draws a line-strip using the current line style.
	* Vertex coordinate data for the line strip is taken from the buffer specified by SetVertexData.
	* Indexing is not used.
	*/
	void DrawLineStrip(int BaseVertexIndex, int LineCount);

	// Render-thread analogue of DrawLineStrip
	void DrawLineStrip_RenderThread(const INT BaseVertexIndex, const INT LineCount);

	/**
	* Ends rendering of a movie frame.
	*/
	void EndDisplay();

	// Render-thread analogue of EndDisplay
	void EndDisplay_RenderThread();

	/**
	* Ends rendering of a submit mask an enables its use. All shapes rendered from
	* this point on are clipped by the mask. The mask is originally generated by
	* rendering shapes after a call to BeginSubmitMask, it is used after EndSubmitMask.
	*/
	void EndSubmitMask();

	// Render-thread analogue of EndSubmitMask
	void EndSubmitMask_RenderThread();

	void ApplyUITransform_RenderThread(const GRenderer::Matrix& ViewportMatrix, const GRenderer::Matrix& TransformMatrix, FGFxVertexShaderInterface& VertexShader);

	/**
	* Sets the fill style used for triangle mesh rendering to a texture.
	* This fill style is used by the DrawIndexedTriList method.
    */
	void FillStyleBitmap(const FillTexture* Fill);

	// Render-thread analogue of FillStyleBitmap
	void FillStyleBitmap_RenderThread(const FFillTextureInfo& InTexInfo);

	/**
	* Sets the fill style used for triangle mesh rendering to a solid color.
	* This fill style is used by the DrawIndexedTriList method.
	*/
	void FillStyleColor(GColor Color);

	// Render-thread analogue of FillStyleColor
	void FillStyleColor_RenderThread( const GColor Color );

	/**
	* Disables the specified fill style.
	*/
	void FillStyleDisable();

	/**
	* Sets the interpolated color/texture fill style used for shapes with edge anti-aliasing.
	*
	* The specified textures are applied to triangle based on factors of gouraud vertex.
	* Trailing argument texture pointers can be NULL, in which case texture is not applied
	* and vertex colors are used instead. All texture arguments can be null only if
	* GFill_Color fillType is used.
	*
	* The fill style set by FillStyleGouraud is always used in conjunction with
	* VertexXY16iC32 or VertexXY16iCF32 vertex types. See documentation for these vertex formats
	* for details on how textures and vertex colors are mixed together based on specified fillType.
	*
	* If the renderer implementation does not support EdgeAA it can implement this function as a no-op.
	* In this case, the renderer must NOT report the Cap_FillGouraud and Cap_FillGouraudTexture capability flags.
	*/
	void FillStyleGouraud(GouraudFillType FillType, const FillTexture* Texture0 = 0, const FillTexture* Texture1 = 0, const FillTexture* Texture2 = 0);

	// Render-thread analogue of FillStyleGouraud
	void FillStyleGouraud_RenderThread( 
		const GouraudFillType FillType,
		const FFillTextureInfo* const TexInfo0 = 0,
		const FFillTextureInfo* const TexInfo1 = 0,
		const FFillTextureInfo* const TexInfo2 = 0 );

	/**
	* Fills in the renderer capabilities structure.
	* Will fail if the video mode is not set on the device.
	*/
	bool GetRenderCaps(RenderCaps* Caps);

	/** TODO:
	* Obtains render stats, pass pstats = 0 to not fill it in.
	* Specify bResetStats = 1 for reset stat counters after retrieving them.
	*/
	void GetRenderStats(Stats* Stats, bool bResetStats = 0);

	/**
	* Sets the line style used for line strips to a solid color.
	*/
	void LineStyleColor(GColor Color);

	// Render-thread analogue of LineStyleColor
	void LineStyleColor_RenderThread(const GColor Color);

	/**
	* Disables the specified line style
	*/
	void LineStyleDisable();

	/**
	* Pops a blend mode, restoring it to the previous one.
	* Internal stack is automatically emptied on EndDisplay.
	* Top of stack is the current blend mode that will be used for rendering.
	*/
	void PopBlendMode();

	/**
	* Pushes a Blend mode onto renderer. Default mode is Normal.
	* Top of stack is the current blend mode that will be used for rendering.
	*/
	void PushBlendMode(BlendType Mode);

	/** TODO:
	* Releases cached data that was allocated from the cache provider.
	* When this method is called the renderer should delete any of its internal
	* pointers to the specified data object (together with any associated buffer data).
	*
	* This can be thought of as a single point of cleanup for all cached objects.
	* Renderer implementation of this method does not need to call pdata->ReleaseDataByRenderer,
	* since the user will do so after the call.
	*/
	void ReleaseCachedData(CachedData* Data, CachedDataType Type);

	/**
	* Sets the color transform class applied to all rendering.
	*/
	void SetCxform(const Cxform& Cx);

	/**
	* Sets the index data that will be used for mesh rendering;
	* DrawIndexedTriList is currently the only function relying on this data.
	*
	* Data buffer passed to this function must remain valid until this function is called
	* with a different buffer (or a null buffer value), or the renderer allows it to be
	* discarded by creating a CachedData option without the keepData flag.
	*/
	void SetIndexData(const void* Indices, int NumIndices, IndexFormat Idxf, CacheProvider* Cache = 0);

	/**
	* Sets the transform matrix applied to all triangle mesh and line strip rendering.
	*/
	void SetMatrix(const Matrix& M);

	/**
	* Sets a 2D user matrix which is applied in addition to the regular matrix transform.
	* This matrix must be applied outside of BeginDisplay/EndDisplay call window.
	*
	* During SWF rendering GFxPlayer library does not modify the user matrix;
	* this means that the transform you set here will apply to the whole scene.
	* SetUserMatrix is mostly used for debugging, although it could be exploited
	* for special effects as well. Setting the user matrix does not cause extra transform
	* overhead since it is normally factored into the viewport transformation.
	*/
	void SetUserMatrix(const Matrix& M);

	/**
	* Sets the vertex data of SInt16/Float {x,y} pairs that will be used for triangle mesh rendering;
	* it is used by DrawIndexedTriList and DrawLineStrip methods.
	*
	* The benefit of using a separate SetVertexData call is that same vertex data can be shared by
	* multiple mesh rendering calls. Data buffer passed to this function must remain valid until this
	* function is called with a different buffer (or a null buffer value), or the renderer allows it to
	* be discarded by creating a CachedData option without the keepData flag.
	*
	* Implementor of this method may choose to optimize mesh rendering by copying the data into a static
	* vertex buffer and using it from this point on. To make this work efficiently, it can allocate a
	* CachedData object from the pcache interface and use it so store renderer-specific handle associated
	* with the data passed to Vertices. In further calls to SetVertexData, such implementation can check
	* for the presence of the CachedData object and rely on its associated static buffers
	* instead of the user specified data.
	*
	* Current implementations of renderer are required to support Vertex_XY16i format;
	* however, this restriction will be lifted in the near future. The Vertex_XY16iC32 and
	* Vertex_XY16iCF32 formats must be supported for edge antialiasing to work.
	*/
	void SetVertexData(const void* Vertices, int NumVertices, VertexFormat Vf, CacheProvider* Cache = 0);

    void GetStats(GStatBag* Bag, bool bResetStats) { }

    FLinearColor CorrectGamma(FLinearColor Color) const;
    FSamplerStateRHIRef GetSamplerState(BitmapSampleMode SampleMode, BitmapWrapMode WrapMode, UBOOL bUseMips);

    void SetRenderTarget(class FRenderTarget* RT) { RenderTarget = RT; }

    void SetUIViewport(FGFxViewportUserParams& ViewportParams);

#if (GFC_FX_MAJOR_VERSION >= 3 && GFC_FX_MINOR_VERSION >= 2) && !defined(GFC_NO_3D)
    virtual void  SetPerspective3D(const GMatrix3D& ProjMatIn);
    virtual void  SetView3D(const GMatrix3D& ViewMatIn);
    virtual void  SetWorld3D(const GMatrix3D* WorldMatIn);
#endif

    //void                SetGammaShaderParams_RenderThread(int ViewFlags, Float Gamma);

    FGFxRenderTarget*   CreateRenderTarget();
    void                SetDisplayRenderTarget_RenderThread(GRenderTarget* pRT, bool bsetstate);
    void                SetDisplayRenderTarget(GRenderTarget* prt, bool setstate = 1);
    FGFxRenderTarget*   GetCurRenderTarget() { return CurRenderTarget; }

    void                PushRenderTarget_RenderThread(const GRectF& frameRect, GRenderTarget* prt);
    void                PushRenderTarget(const GRectF& frameRect, GRenderTarget* prt);
    void                PopRenderTarget_RenderThread();
    void                PopRenderTarget();

    FGFxTexture*        PushTempRenderTarget(const GRectF& frameRect, UInt targetW, UInt targetH, bool wantStencil);
    FGFxTexture*        PushTempRenderTarget_RenderThread(FGFxTexture* StubTexture, const GRectF& frameRect, UInt targetW, UInt targetH, bool wantStencil);

    void                ReleaseTempRenderTargets(UInt keepArea);
    void                ReleaseTempRenderTargets_RenderThread(UInt keepArea);

    UInt        CheckFilterSupport(const BlurFilterParams& params);
    void        DrawBlurRect_RenderThread(GTexture* psrcin, const GRectF& insrcrect, const GRectF& indestrect, const BlurFilterParams& params);
    void        DrawBlurRect(GTexture* psrcin, const GRectF& insrcrect, const GRectF& indestrect, const BlurFilterParams& params);

    void        DrawColorMatrixRect_RenderThread(GTexture* Texture, const GRectF& SrcRect, const GRectF& DestRect, const Float* CMatrix);
    void        DrawColorMatrixRect(GTexture* psrcin, const GRectF& insrcrect, const GRectF& destrect, const Float *matrix);

    void        SetMaxRenderTargetSize(UInt InSize) { MaxTempRTSize = InSize; }

private:

	class FViewport*	Viewport;
    class FRenderTarget* RenderTarget;
    UInt                 RenderMode;

	// Render-thread only - vertex and index stores
    struct FGFxRendererImpl::FGFxVertexStore* VertexStore;
	struct FGFxRendererImpl::FGFxIndexStore* IndexStore;

	// Render-thread only - vertex and color transforms
	Matrix              UserMatrix;
	Matrix              CurrentMatrix;
	Matrix              ViewportMatrix;
	Cxform              CurrentCxform;
    FGFxViewportUserParams  ViewRect;
    FLOAT               InverseGamma;
    FStencilStateRHIRef CurStencilState;

#if (GFC_FX_MAJOR_VERSION >= 3 && GFC_FX_MINOR_VERSION >= 2) && !defined(GFC_NO_3D)
    // Must be updated on render thread
    GMatrix3D           ViewMatrix, ProjMatrix;
    GMatrix3D           WorldMatrix;
    GMatrix3D           UVPMatrix;
    UBOOL               UVPMatricesChanged;
#endif
    UBOOL               Is3DEnabled;

    // render target - render thread only
    struct RTState
    {
        FGFxRenderTarget*       pRT;
        GMatrix2D               ViewMatrix;
        GMatrix3D               ViewMatrix3D;
        GMatrix3D               PerspMatrix3D;
        GMatrix3D               WorldMatrix3D;
        FGFxViewportUserParams  ViewRect;
        SInt                    RenderMode;
        UBOOL                   Is3DEnabled;
        FStencilStateRHIRef     StencilState;

        RTState() { pRT = 0; }
        RTState(FGFxRenderTarget* prt, const GMatrix2D& vm, 
            const GMatrix3D& vm3d, const GMatrix3D &vp3d, const GMatrix3D& w3d, UBOOL is3d,
            const FGFxViewportUserParams& vp, SInt rm, FStencilStateRHIParamRef ss)
            : pRT(prt), ViewMatrix(vm), ViewMatrix3D(vm3d), PerspMatrix3D(vp3d), 
            WorldMatrix3D(w3d), ViewRect(vp), RenderMode(rm), Is3DEnabled(is3d), StencilState(ss) { }
    };
    typedef GArrayConstPolicy<0, 4, true> NeverShrinkPolicy;
    typedef GArrayLH<RTState, GStat_Default_Mem, NeverShrinkPolicy> RTStackType;

    FGFxRenderTarget*       CurRenderTarget;
    UBOOL                   CurRenderTargetSet;
    RTStackType             RenderTargetStack;

    // Main thread
    GArrayLH<GPtr<FGFxRenderTarget> >  TempRenderTargets;
    GArrayLH<GPtr<FGFxRenderResources> > TempStencilBuffers;

    // A list if all RenderElementStore_ classes (derived from GFxRenderNode)
    GRendererNode       ElementStoreList;
    // Lock used when vertex/index buffer data is being accessed.
    GLock               ElementAccessLock;

	// Render-thread only - blend state management
	BlendType			BlendMode;
	TArray<BlendType>	BlendModeStack;
    UBOOL               bAlphaComposite;
    UInt                MaxTempRTSize;

	// Render-thread only - shader state caching
	TMap<DWORD, FBoundShaderStateRHIRef> BoundShaderStateCache;

	//*** Misc initialization
	struct FMiscRenderStateInitParams
	{
		FGFxRenderer*       Renderer;
		BlendType*          BlendMode;
		TArray<BlendType>*  BlendModeStack;
		DWORD*              StencilCounter;
		FLOAT*              InverseGamma;
		UBOOL*              bAlphaComposite;
		UInt32              ViewFlags;
	};

	void InitUIBlendStackAndMiscRenderState_RenderingThread(FMiscRenderStateInitParams& Params);

#ifdef FGFxRenderer
public:
    GLock                   RTTexturesLock;
    TArray<UTexture2D*>     RTAllocTextures[4];
    TMap<UTexture2D*,INT>   RTAllTextures;
private:
#endif
    void CheckRenderTarget_RenderThread();

	/************************************************************************/
	/* FGFxRenderStyle                                                    */
	/************************************************************************/

	enum EGFxRenderStyleMode
	{
		GFx_SM_Disabled = 0,
        GFx_SM_Color,
		// Only used with fill styles:
		GFx_SM_Bitmap,
		GFx_SM_Gouraud
	};

	// Base class for managing shader parameters
	class FGFxRenderStyle
	{
	public:

		// Render-thread data
		EGFxRenderStyleMode StyleMode;
		GColor              Color;
		Cxform              CxColorMatrix;

		// Game thread function, disables style application
		void Disable();

		// Returns true if style should not be applied
		UBOOL IsDisabled_RenderThread() const 
		{ 
			return GFx_SM_Disabled == StyleMode; 
		}

		// Set to use solid color shading without textures
		void SetStyleColor_RenderThread(const GColor NewColor, const Cxform& PersistentCxMatrix);

		// Resets style state
		virtual void EndDisplay_RenderThread();

		// HACK - using void* to avoid publishing internal shader implementation
		virtual void Apply_RenderThread(
			FGFxRenderer* Renderer,
            const void* const BoundShaderState,
            void* const RenderStyleContext);

		// Determines which pixel shader, vertex shader, and vertex declaration should be used
		virtual void GetEnumeratedBoundShaderState_RenderThread(void* const OutEnumeratedBoundShaderState, const void* const RenderStyleContext);

	private:

		// Shader parameters for solid-color rendering
		void ApplyColor_RenderThread(FGFxRenderer* Renderer, const void* const BoundShaderState, const void* const RenderStyleContext);
		void ApplyColorScaleAndColorBias_RenderThread(const void* const BoundShaderState, const void* const RenderStyleContext);
	};

	// The following is done to distinguish line styles from fill styles
	class FGFxLineStyle : public FGFxRenderStyle
	{
	public:
	};

	// Allows rendering with textures and gouraud shading
	class FGFxFillStyle : public FGFxRenderStyle
	{
	public:

		// Render thread data
		GouraudFillType     GouraudFillMode;
		FFillTextureInfo TexInfo;
		FFillTextureInfo TexInfo2;

		// FGFxRenderStyle
		void EndDisplay_RenderThread();

		void Apply_RenderThread(FGFxRenderer* Renderer, const void* const BoundShaderState, void* const RenderStyleContext);

		// HACK - using void* to avoid publishing internal shader implementation
		void GetEnumeratedBoundShaderState_RenderThread(void* const OutEnumeratedBoundShaderState, const void* const RenderStyleContext);

		// Enables texture-based pixel and vertex shaders
		void SetStyleBitmap_RenderThread(const FFillTextureInfo& InTexInfo, const Cxform& PersistentCxMatrix);

		// Enables gouraud shading and multi-texturing
		void SetStyleGouraud_RenderThread(
			const GouraudFillType InGouraudFillType,
			const FFillTextureInfo* const TexInfo0,
			const FFillTextureInfo* const TexInfo1,
			const FFillTextureInfo* const TexInfo2,
			const Cxform& PersistenctCxMatrix);

	private:

		// Sets shader parameters and also texture sampling

        static void StaticApplyTextureMatrix_RenderThread(
            const void* const BoundShaderState,
			const void* const RenderStyleContext,
			const FFillTextureInfo& TexInfo, INT i );

		static void StaticApplyTexture_RenderThread(
            FGFxRenderer* Renderer,
            const void* const BoundShaderState,
			const void* const RenderStyleContext,
			const FFillTextureInfo& TexInfo, INT i );
	};

    // Render-thread only - sampler states
    FSamplerStateRHIRef SamplerStates[8];

    // Render-thread only - fill and line style data
	FGFxFillStyle FillStyle;
	FGFxLineStyle LineStyle;

	// Render-thread only - for masking support
	DWORD StencilCounter;

	// Disallow copy and assignment;
	FGFxRenderer(const FGFxRenderer&);
	FGFxRenderer& operator=(const FGFxRenderer&);
};


#endif // WITH_GFx

#endif // GFxUIRenderer_h
