/**********************************************************************

Filename    :   GRenderer.h
Content     :   Vector graphics 2D renderer implementation for 
Created     :   June 29, 2005
Authors     :   

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GRENDERER_H
#define INC_GRENDERER_H

#include "GTypes.h"
#include "GAtomic.h"
#include "GRefCount.h"
#include "GColor.h"
#include "GMatrix2D.h"
#include "GMatrix3D.h"
#include "GTypes2DF.h"
#include "GImage.h"
#include "GStats.h"
#include "GList.h"

#include "GRendererEventHandler.h"

// ***** Declared Classes
class   GRenderer;
class   GViewport;
class   GTexture;
class   GImageInfoBase;
class   GImageInfo;


// ***** Renderer Stat Constants

enum GStatRenderer
{
    GStatRender_Default = GStatGroup_Renderer,

    GStatRender_Mem,

    // Video Memory.
    GStatRender_VMem,
      GStatRender_Texture_VMem,
      GStatRender_Buffer_VMem,
    
    // Counts
    GStatRender_Counters,
      GStatRender_TextureUpload_Cnt,
      GStatRender_TextureUpdate_Cnt,
      GStatRender_DP_Cnt,
        GStatRender_DP_Line_Cnt,
        GStatRender_DP_Triangle_Cnt,
      GStatRender_Triangle_Cnt,
      GStatRender_Line_Cnt,
      GStatRender_Mask_Cnt,
      GStatRender_Filter_Cnt,
};


// Base class for 2D graphics rendering, used through subclasses
// GRendererD3D9, GRendererOGL, etc.
// This class will be replaced with a new shape-centric version 
// in the near future.

class GViewport
{
public:
    SInt    BufferWidth, BufferHeight;
    SInt    Left, Top;
    SInt    Width, Height;
    SInt    ScissorLeft,  ScissorTop; // scissor left & top are in same coords as Left & Top (in buffer's ones)
    SInt    ScissorWidth, ScissorHeight;
    Float   Scale, AspectRatio;
    UInt    Flags;

    enum
    {
        View_IsRenderTexture    = 1,

        // Support blending into the destination through a render target texture.
        // Only blend/alpha and add modes are supported. Render the texture with
        // source blend factor 1, dest blend factor 1-Sa.
        View_AlphaComposite     = 2,
        View_UseScissorRect     = 4,

        // Don't set viewport/scissor state on device. Currently only supported by some
        // GRenderer implementations.
        View_NoSetState         = 8,

        View_RenderTextureAlpha = View_IsRenderTexture|View_AlphaComposite
    };

    GViewport()
    { 
        BufferWidth = 0; BufferHeight = 0; Left = Top = 0; Width = Height = 1; ScissorLeft = ScissorTop = ScissorWidth = ScissorHeight = 0; Flags = 0; 
        Scale = AspectRatio = 1.0f;
    }
    GViewport(SInt bw, SInt bh, SInt left, SInt top, SInt w, SInt h, SInt scleft, SInt sctop, SInt scw, SInt sch, UInt flags = 0)
    { 
        BufferWidth = bw; BufferHeight = bh; Left = left; Top = top; Width = w; Height = h; Flags = flags | View_UseScissorRect; 
        Scale = AspectRatio = 1.0f;
        SetScissorRect(scleft, sctop, scw, sch);
    }
    GViewport(SInt bw, SInt bh, SInt left, SInt top, SInt w, SInt h, SInt scleft, SInt sctop, SInt scw, SInt sch, Float scale, Float ratio, UInt flags = 0)
    { 
        BufferWidth = bw; BufferHeight = bh; Left = left; Top = top; Width = w; Height = h; Flags = flags | View_UseScissorRect; 
        Scale = scale; AspectRatio = ratio;
        SetScissorRect(scleft, sctop, scw, sch);
    }
    GViewport(SInt bw, SInt bh, SInt left, SInt top, SInt w, SInt h, UInt flags = 0)
    { 
        BufferWidth = bw; BufferHeight = bh; Left = left; Top = top; Width = w; Height = h; Flags = flags; 
        ScissorLeft = ScissorTop = ScissorWidth = ScissorHeight = 0;
        Scale = AspectRatio = 1.0f;
    }
    GViewport(const GViewport& src)
    { 
        BufferWidth = src.BufferWidth; BufferHeight = src.BufferHeight; Left = src.Left; Top = src.Top; Width = src.Width; Height = src.Height; Flags = src.Flags; 
        ScissorLeft = src.ScissorLeft; ScissorTop = src.ScissorTop; ScissorWidth = src.ScissorWidth; ScissorHeight = src.ScissorHeight; 
        Scale = src.Scale; AspectRatio = src.AspectRatio;
    }


    void SetViewport(SInt bw, SInt bh, SInt left, SInt top, SInt w, SInt h, UInt flags = 0)
    { 
        BufferWidth = bw; BufferHeight = bh; Left = left; Top = top; Width = w; Height = h; Flags = flags; 
        ScissorLeft = ScissorTop = ScissorWidth = ScissorHeight = 0;
        Scale = AspectRatio = 1.0f;
    }


    void SetScissorRect(SInt scleft, SInt sctop, SInt scw, SInt sch)
    {
        ScissorLeft = scleft; ScissorTop = sctop; ScissorWidth = scw; ScissorHeight = sch;
        Flags |= View_UseScissorRect;
    }
};

class GTexture : public GNewOverrideBase<GStatRender_Mem>
{
protected:
    GAtomicInt<SInt32>  RefCount;

public:
    GTexture()                  { RefCount.Value = 1; }
    virtual ~GTexture()         {}

    inline void        AddRef() { RefCount.Increment_NoSync(); }  
    inline void        Release()
    {
        if ((RefCount.ExchangeAdd_Acquire(-1) - 1) == 0)
        {  
            delete this;
        }
    }
    inline bool        AddRef_NotZero()
    {
        while (1)
        {
            SInt32 refCount = RefCount;
            if (refCount == 0)
                return 0;
            if (RefCount.CompareAndSet_NoSync(refCount, refCount+1))
                break;
        }
        return 1;
    }
    inline SInt32      GetRefCount()
    {
        return RefCount.Load_Acquire();
    }

    typedef void* Handle;

    // Init functions initialize texture data, essentially allocating the
    // texture. Allocating texture may fail if there is not enough memory,
    // or the renderer is not ready for allocation.
    // Calling Init a second time will recreate the texture with a new size.
       
    // Target width/height specify the size image should have if it wasn't rounded up 
    // to the next power of two. That is the 'logical' texture size used during rendering.
    // Target width/height of 0 indicates the same size as the image.    
    enum ImageTexUsage
    {
        Usage_Wrap          = 0x0001,
      //Usage_GenMipmaps    = 0x0002,

        // only for dynamic textures
        Usage_Update        = 0x0010,
        Usage_Map           = 0x0020,
        Usage_RenderTarget  = 0x0040 
    };
    virtual bool        InitTexture(GImageBase* pim, UInt usage = Usage_Wrap)   = 0;

    // Init initially empty updateable texture (0 mipmap for regular texture)
    virtual bool        InitDynamicTexture(int width, int height, GImage::ImageFormat format, int mipmaps, UInt usage) = 0;

    // Update a region of a texture. Only guaranteed to work with textures created as updateable.
    // Supplied image must be the same format texture was created with.
    // Only updates specified mipmap level (0 is full size)
    struct UpdateRect
    {
        GPoint<int>     dest;
        GRect<int>      src;
    };
    virtual void        Update(int level, int n, const UpdateRect *rects, const GImageBase *pim) = 0;

    // Map texture data to memory. Format of texture data may vary by platform. Not
    // always supported, especially if the texture was not created as mappable or updatable. A YUV texture
    // uses 3 (4 for YUV+A) maps.
    enum MapFlags
    {
        Map_KeepOld = 1 // preserve original texture contents when mapping
    };
    struct MapRect
    {
        UInt    width, height;
        UByte*  pData;
        UInt    pitch;
    };
    virtual int         Map(int level, int n, MapRect* maps, int flags=0)   = 0;
    virtual bool        Unmap(int level, int n, MapRect* maps, int flags=0) = 0;


    // Obtains the renderer that create TextureInfo
    // GetRenderer can return 0 iff the renderer has been released, in that
    // case surviving GetTexture object should be just a stub
    virtual GRenderer*  GetRenderer() const                                 = 0;
    virtual bool        IsDataValid() const                                 = 0;

    // Set and get user data handle; can be useful to pass extra info
    // about the texture to a custom renderer implementation. Note that renderer
    // should still support the default handle value of null.
    virtual Handle      GetUserData() const                                 = 0;
    virtual void        SetUserData(Handle hdata)                           = 0;


    class ChangeHandler
    {
    public:
        virtual ~ChangeHandler () { }

        enum EventType
        {
            // Data changed due to Init call
            // (it may be possible that data is released here)
            Event_DataChange,       
            // Data was lost due for renderer-specific reasons (video memory, etc.)
            Event_DataLost,
            // Renderer was released, no more data operations are possible
            Event_RendererReleased
        };

        // Called when renderer texture data is lost, or when it changes
        virtual void    OnChange(GRenderer* prenderer, EventType changeType)
            { GUNUSED2(prenderer,changeType); }
        // Called to instruct the texture owner to re-create the texture;
        // this will only happen if the user tried to use texture for
        // rendering while its data was lost.
        virtual bool    Recreate(GRenderer* prenderer)
            { GUNUSED(prenderer); return 0; }
    };

    // Add/Remove notification
    virtual void        AddChangeHandler(ChangeHandler *phandler)           = 0;
    virtual void        RemoveChangeHandler(ChangeHandler *phandler)        = 0;
};

class GRenderTarget : public GNewOverrideBase<GStatRender_Mem>
{
private:
    GAtomicInt<SInt32>  RefCount;

public:
    GRenderTarget()                  { RefCount.Value = 1; }
    virtual ~GRenderTarget()         {}

    inline void        AddRef() { RefCount.Increment_NoSync(); }  
    inline void        Release()
    {
        if ((RefCount.ExchangeAdd_Acquire(-1) - 1) == 0)
        {  
            delete this;
        }
    }
    inline bool        AddRef_NotZero()
    {
        while (1)
        {
            SInt32 refCount = RefCount;
            if (refCount == 0)
                return 0;
            if (RefCount.CompareAndSet_NoSync(refCount, refCount+1))
                break;
        }
        return 1;
    }

    // Initialize a render target that uses ptarget as the color buffer.
    // Not all texture formats are supported; for portability use only Image_ARGB_8888.
    // Optionally, a depth and/or stencil buffer can be provided. The depth and stencil textures must be the
    // appropriate formats. Texture(s) must have been created with Usage_RenderTarget (not all systems will enforce this).
    // For packed depth and stencil (such as with D3D9), specify the same object as pdepth and pstencil.
    virtual bool       InitRenderTarget(GTexture *ptarget, GTexture* pdepth = 0, GTexture* pstencil = 0) = 0;


    virtual GRenderer*  GetRenderer() const                                 = 0;

    typedef GTexture::ChangeHandler ChangeHandler;
    typedef void* Handle;

    // Set and get user data handle; can be useful to pass extra info
    // about the render target to a custom renderer implementation. Note that renderer
    // should still support the default handle value of null.
    virtual Handle      GetUserData() const                                 = 0;
    virtual void        SetUserData(Handle hdata)                           = 0;

    // Add/Remove notification
    virtual void        AddChangeHandler(ChangeHandler *phandler)           = 0;
    virtual void        RemoveChangeHandler(ChangeHandler *phandler)        = 0;
};


template <typename T> T TwipsToPixels(T x) { return T((x) / T(20.)); }
template <typename T> T PixelsToTwips(T x) { return T((x) * T(20.)); }

inline GRectF TwipsToPixels(const GRectF& x) 
{ 
    return GRectF(TwipsToPixels(x.Left),  TwipsToPixels(x.Top), 
                  TwipsToPixels(x.Right), TwipsToPixels(x.Bottom));
}

inline GRectF PixelsToTwips(const GRectF& x) 
{ 
    return GRectF(PixelsToTwips(x.Left),  PixelsToTwips(x.Top), 
                  PixelsToTwips(x.Right), PixelsToTwips(x.Bottom));
}

// Necessary for now
#define GFC_TWIPS_TO_PIXELS(x)  TwipsToPixels(Float(x))
#define GFC_PIXELS_TO_TWIPS(x)  PixelsToTwips(Float(x))
#define DEFAULT_FLASH_FOV 55

class GRenderer : public GRefCountBase<GRenderer, GStatRender_Mem>
{
public:
    // ***** Helper classes

    GRenderer();
    virtual ~GRenderer(); 

    // Local for backwards renderer compatibility.
    typedef GPointF     Point;
    typedef GRectF      Rect;
    typedef GMatrix2D   Matrix;
    typedef void*       Handle;

    // Blend modes supported by Flash.
    enum BlendType
    {
        Blend_None          = 0,    // Same as Normal, but means that BlendMode was not set.
        Blend_Normal        = 1,
        Blend_Layer         = 2,
        Blend_Multiply      = 3,
        Blend_Screen        = 4,
        Blend_Lighten       = 5,
        Blend_Darken        = 6,
        Blend_Difference    = 7,
        Blend_Add           = 8,
        Blend_Subtract      = 9,
        Blend_Invert        = 10,
        Blend_Alpha         = 11,
        Blend_Erase         = 12,
        Blend_Overlay       = 13,
        Blend_HardLight     = 14
    };

    // Operations for the image resizer
    enum ResizeImageType
    {
        ResizeRgbToRgb,     // 24-bit RGB
        ResizeRgbaToRgba,   // 32-bit RGBA
        ResizeRgbToRgba,    // Add Alpha and form RGBA
        ResizeGray          // 8-bit gray scale
    };

    enum VertexFormat
    {
        Vertex_None         = 0,    // Used when vertex data buffer is not provided.
        Vertex_XY16i        = 1,
        Vertex_XY32f        = 2,
        // Edge AA support vertex formats.
        Vertex_XY16iC32     = 3,    // Vertex with colors only (more efficient)
        Vertex_XY16iCF32    = 4     // Vertex with colors and texture factors
    };

    enum IndexFormat
    {
        Index_None      = 0,    // Used when index data buffer is not provided.
        Index_16        = 1,
        Index_32        = 2 
    };

    enum RenderCapBits
    {
        Cap_CacheDataUse    = 0x00000001,       
        Cap_Index16         = 0x00000004,
        Cap_Index32         = 0x00000008,
        Cap_RenderStats     = 0x00000010, // Renderer build supports rendering statistics.
        
        Cap_FillGouraud     = 0x00000100,   // EdgeAA fill modes supported
        Cap_FillGouraudTex  = 0x00000200,   // Fill gouraud texturing is supported

        // Reported if Cxform Add channel is always implemented correctly.
        Cap_CxformAdd       = 0x00001000,

        // Reported if nested submit masks (with counters) are supported.
        Cap_NestedMasks     = 0x00002000,

        // Non power of 2 texture support
        Cap_TexNonPower2    = 0x00004000,
        Cap_TexNonPower2Wrap= 0x00008000,
        Cap_TexNonPower2Mip = 0x00080000,

        // Reported if render textures can generate a Event_DataLost (ex: D3D9).
        Cap_CanLoseData     = 0x00010000,

        // Reported if vertex data cannot be changed (in same frame) after passed to renderer
        // use AddEventHandler to detect this
        Cap_KeepVertexData  = 0x00020000,

        // Reported if updatable textures cannot be overwritten during the same frame
        Cap_NoTexOverwrite  = 0x00040000,

        // Threaded texture creation is supported
        Cap_ThreadedTextureCreation = 0x00100000,

	    // Use render targets at any time
	    Cap_RenderTargets           = 0x00000020,

	    // Use render targets during prepass only
	    // Limited to one outstanding at a time
	    Cap_RenderTargetPrePass     = 0x00000040,

	    // Textures used as render targets allowed to be non power of 2 only if this is set.
	    Cap_RenderTargetNonPow2     = 0x00000080,

	    // Render target textures can have mipmaps (use Usage_GenMipmaps)
	    Cap_RenderTargetMip         = 0x00200000,

	    // Filters
	    Cap_Filter_Blurs            = 0x00400000,
	    Cap_Filter_ColorMatrix      = 0x00800000,

    };

    // for 3D stereo rendering
    enum StereoDisplay
    {
        StereoCenter,       // the non-stereo case
        StereoLeft,         // use the left eye perspective
        StereoRight         // use the right eye perspective
    };

    struct StereoParams
    {
        Float DisplayWidthCm;           // the width of the display, computed internally
        Float Distortion;               // 0 to 1, the strength of the stereo effect, defaults to 0.75
        Float DisplayDiagInches;        // the diagonal size of the output display in inches, defaults to 52
        Float DisplayAspectRatio;  
        Float EyeSeparationCm;          // the distance between eyes in cm, defaults to 6.4
        StereoParams() : DisplayWidthCm(0), Distortion(0.75f), DisplayDiagInches(52.f), DisplayAspectRatio(9.f/16.f), EyeSeparationCm(6.4f) {  }
    };

    struct RenderCaps
    {
        UInt32  CapBits;
        UInt32  VertexFormats;
        UInt32  BlendModes;
        UInt32  MaxTextureSize; 
    };


    // Renderer statistics, filled in by GetRenderStats.
    class Stats
    {
    public:

        // Count of triangles and lines
        UInt    Triangles;
        UInt    Lines;
        UInt    Primitives;
        UInt    Masks;
        UInt    Filters;

        Stats()         { Clear();}
        void Clear()    { Triangles = 0; Lines = 0; Primitives = 0; Masks = 0; Filters = 0; }           
    };


    class CacheProvider;

    enum CachedDataType
    {
        Cached_Vertex      = 1,
        Cached_Index       = 2,
        Cached_BitmapList  = 3
    };


    // CachedData interface. Objects of this type can be allocated/retrived though the CacheProvider interface,
    // and used to make an association between raw buffer data and its video memory representation.
    class CachedData
    {
        friend class CacheProvider;

        // Used to request the renderer to release its own data.
        GRenderer * pRenderer;
        // Data stored for the renderer.
        Handle      hData;

    public:
        GINLINE CachedData();
        GINLINE ~CachedData();

        // Get renderer which owns this data. If not null, data object is in use.
        GINLINE GRenderer*  GetRenderer() const         { return pRenderer; }
        // Sets/retrives renderer data handle associated with this buffer or resource.
        GINLINE Handle      GetRendererData() const     { return hData; }
        GINLINE void        SetRendererData(Handle h)   { hData = h; }

        // Releases cached data object, notifying renderer about destruction.
        GINLINE void        ReleaseData(CachedDataType type);
        // Releases cached data object content, to be used by renderer only, after
        // its cached associated buffers are released and no longer needed.
        // Performs final cleanup of CachedData object, will not call GRenderer::ReleaseCachedData.
        GINLINE void        ReleaseDataByRenderer();        
    };

    // An interface used to request CachedData objects which can be associated with a given
    // buffer set. Created objects must be released with CachedData::ReleaseRendererData.
    class CacheProvider
    {
        friend class GFxCachedStroke;
        friend class GFxMesh;

        CachedData*     pData;      
        bool            DiscardSharedData;  // Set during creation if public data buffers can be discarded.

    public:
        // Internal use interface.
        GINLINE CacheProvider(CachedData* pcache);
        GINLINE ~CacheProvider();
        
        // Retrieves a cached data object associated with this renderer for the
        // given data set. Return 0 if such object was not created yet.
        GINLINE CachedData*     GetCachedData(GRenderer* prenderer);
            
        // Creates a CachedData object that is associated with the specified data buffer.
        // Must call GetCachedData() first to verify if necessary object does not already exist for the renderer.
        // If the data buffer contents ever change the user will need to call GRenderer::ReleaseData.
        GINLINE CachedData*     CreateCachedData(GRenderer::CachedDataType type, GRenderer* prenderer, bool keepSharedData = 1);

        // Returns 1 if cached data objects were created and source buffers can now be discarded.
        GINLINE bool            CanDiscardData() { return DiscardSharedData; }
    };



    // *** Cxform: Color Transform

    class Cxform
    {
    public:
        // 0,1,2,3 -> R,G,B,A
        Float   M_[4][2];   // [RGBA][mult, add]

        Cxform();
        void    Concatenate(const Cxform& c);
        GColor  Transform(const GColor in) const;
        
        // Formats matrix message to a buffer, but 512 bytes at least
        void    Format(char *pbuffer) const;        

        void    SetIdentity();
        bool    IsIdentity() const;

        bool    operator== (const Cxform& x) const
        {
            return M_[0][0] == x.M_[0][0] && M_[1][0] == x.M_[1][0] && M_[2][0] == x.M_[2][0] && M_[3][0] == x.M_[3][0] &&
                M_[0][1] == x.M_[0][1] && M_[1][1] == x.M_[1][1] && M_[2][1] == x.M_[2][1] && M_[3][1] == x.M_[3][1];
        }
        static Cxform   Identity;
    };




    // *** Texture reference, created from images   


    // Helper function to query renderer capabilities.
    // Will fail if the video mode is not set on the device.
    virtual bool        GetRenderCaps(RenderCaps *pcaps)                                = 0;


    // Returns created objects with a refCount of 1, must be user-released.
    // used when DO_NOT_LOAD_BITMAPS is set
    virtual GTexture*   CreateTexture()                                                 = 0;

    // YUV or YUV+A format texture. Not supported on all systems. Can only be initialized with InitMappableTexture
    // and accessed with Map. Can only be rendered with DrawBitmaps.
    virtual GTexture*   CreateTextureYUV()                                              = 0;        

    // Convenience creation functions, create & initialize texture at the same time
    inline GTexture*   CreateTextureFromImage(GImageBase* pim, UInt usage = GTexture::Usage_Wrap);
    

    // Bracket an entire rendering frame. Does setup if necessary
    virtual void        BeginFrame() { }
    virtual void        EndFrame();

    //
    // Render Target support
    //

    virtual GRenderTarget*   CreateRenderTarget()                                       = 0;

    // Set the base render target. Use setstate = false if this render target is already set on the GPU (by the application) to 
    // avoid unnecessary expensive render target changes.
    virtual void        SetDisplayRenderTarget(GRenderTarget* prt, bool setstate = 1)                            = 0;

    // Begin rendering to the specified target; frameRect is the ortho projection.
    // Texture referenced by pRT must not be used until PopRenderTarget.
    virtual void        PushRenderTarget(const GRectF& frameRect, GRenderTarget* prt)                            = 0;

    // Restore previous render target. Contents of Texture in popped render target are now available
    // for rendering.
    virtual void        PopRenderTarget()                                                                        = 0;

    // Push a temp render target. frameRect is the ortho projection. Returned texture must be AddRefed before calling this 
    // renderer again, is not valid for rendering until PopRenderTarget, and must be released when contents are no longer needed.
    virtual GTexture*   PushTempRenderTarget(const GRectF& frameRect, UInt targetW, UInt targetH, bool wantStencil = 0) = 0;

    // Release resources used by temp render targets.
    virtual void        ReleaseTempRenderTargets(UInt keepArea) { GUNUSED(keepArea); }
    
    // Bracket the displaying of a frame from a movie.
    // Fill the background color, and set up default transforms, etc.   
    virtual void        BeginDisplay(
                            GColor backgroundColor,
                            const GViewport& viewport,
                            Float x0, Float x1, Float y0, Float y1)                     = 0;
    virtual void        EndDisplay()                                                    = 0;
    
    // Geometric and color transforms for mesh and LineStrip rendering.
    virtual void        SetMatrix(const Matrix& m)                                      = 0;
    virtual void        SetUserMatrix(const Matrix& m)                                  = 0;
    virtual void        SetCxform(const Cxform& cx)                                     = 0;


    // Pushes a Blend mode onto renderer. Default mode is Normal.
    // Top of stack is the current blend mode that will be used for rendering.
    virtual void        PushBlendMode(BlendType mode)                                   = 0;
    // Pops a blend mode, restoring it to the previous one.
    // Internal stack is automatically emptied on EndDisplay.
    virtual void        PopBlendMode()                                                  = 0;


    // Flags for user data 
    enum UserDataPropertyFlag
    { 
        UD_None = 0x0, 
        UD_HasString, 
        UD_HasFloat,
        UD_HasMatrix
    };     
    // Structure that holds pointers to data set by the user from Actionscript
    struct UserData
    {
        const char*     pString;
        float*          pFloat;
        float*          pMatrix;
        UInt            MatrixSize;
        UByte           PropFlags;
        UserData()
            : pString(NULL), pFloat(NULL), pMatrix(0), MatrixSize(0), PropFlags(UD_None) { }
    };
    // Passes data attached in ActionScript (MovieClip.rendererString, rendererFloat, rendererMatrix).
    // If one of the values was not specified for this object, the corresponding pointer is null.
    // Push function should return true if data was successfully pushed onto the stack.
    virtual bool        PushUserData(UserData* pdata) { GUNUSED(pdata); return false; }
    virtual void        PopUserData() { }

#ifndef GFC_NO_3D
    // Sets the perspective matrix for drawing a 3D movieclip
    virtual void  SetPerspective3D(const GMatrix3D &projMatIn) = 0;

    // Sets the view matrix for drawing a 3D movieclip
    virtual void  SetView3D(const GMatrix3D &viewMatIn) = 0;

    // Sets the world matrix for drawing a 3D movieclip
    virtual void  SetWorld3D(const GMatrix3D *pWorldMatIn) = 0;

    // create default view and perspective matrices
    virtual void MakeViewAndPersp3D(const GRectF &visFrameRectInTwips, GMatrix3D &matView, GMatrix3D &matPersp, 
                                    Float perspFOV, bool bInvertY = false);

    void Adjust3DMatrixForRT(GMatrix3D &UVPMatrix, const GMatrix3D &oldProjMatrix,
                             const GMatrix2D &oldViewMatrix, const GMatrix2D &newViewMatrix);

    // stereo support
    virtual void SetStereoParams(StereoParams sParams) 
    { 
        S3DParams = sParams; 
        if (S3DParams.DisplayWidthCm == 0)
            S3DParams.DisplayWidthCm = sParams.DisplayDiagInches / sqrt(1.0f + 1.f/sParams.DisplayAspectRatio * 1.f/sParams.DisplayAspectRatio) * 2.54f /* inches to cm */; 
    }
#else
    // stereo support
    void SetStereoParams(StereoParams sParams) { GUNUSED(sParams); }
#endif
    virtual void SetStereoDisplay(StereoDisplay sDisplay, bool setstate = 0) { GUNUSED2(sDisplay,setstate); }


    // Sets the vertex data of SInt16/Float {x,y} pairs, to be used by DrawIndexedTriList.
    // The benefit of using this call is that data can be shared by multiple calls.
    // Does not necessarily perform immediate data copy, pvertices data must remain
    // valid until called again with a different buffer or null pvertices pointer.
    virtual void        SetVertexData(const void* pvertices, int numVertices,
                                      VertexFormat vf, CacheProvider *pcache = 0)       = 0;

    virtual void        SetIndexData(const void* pindices, int numIndices,
                                     IndexFormat idxf, CacheProvider *pcache = 0)       = 0;

    // Releases cached data that was allocated from the cache provider.
    // This can be thought of as a single point of cleanup for all cached objects.
    // Renderer does not need to call pdata->ReleaseDataByRenderer, since the
    // user will do so after the call.
    virtual void        ReleaseCachedData(CachedData *pdata, CachedDataType type) = 0;



    // Draw triangles using the current fill-style.     
    // Vertices/Indices must have been specified by SetVertexData/SetDrawIndexData. First accessed
    // vertex will be at baseVertexIndex + minVertexIndex and up to numVertices from that point
    // on will be accessed. The valid index range is [minVertexIndex, minVertexIndex + numVertices).
    // Index data will be accessed beginning with startIndex, consuming triangleCount*3 indices.
    virtual void        DrawIndexedTriList(
                                    int baseVertexIndex, int minVertexIndex, int numVertices,
                                    int startIndex, int triangleCount)                  = 0;
    
    // Draw a line-strip using the current line style. The data is provided
    // through SetVertexData, beginning at baseVertexIndex.
    virtual void        DrawLineStrip(int baseVertexIndex, int lineCount)               = 0;

    
    // Set line and fill styles for mesh & LineStrip rendering. 
    enum BitmapWrapMode
    {
        Wrap_Repeat,
        Wrap_Clamp
    };
    enum BitmapSampleMode
    {
        Sample_Point,
        Sample_Linear
    };

    // Describes texture fill style, including wrapping and transform matrix.
    struct FillTexture
    {
        GTexture*           pTexture;
        Matrix              TextureMatrix;
        BitmapWrapMode      WrapMode;
        BitmapSampleMode    SampleMode;
    };

    
    virtual void        LineStyleDisable()                                              = 0;
    virtual void        LineStyleColor(GColor color)                                    = 0;

    virtual void        FillStyleDisable()                                              = 0;
    virtual void        FillStyleColor(GColor color)                                    = 0;
    virtual void        FillStyleBitmap(const FillTexture* pfill)                       = 0;

    // Fill style helper for simple calls
    GINLINE void        FillStyleBitmap(GTexture* pti, const Matrix& m,
                                        BitmapWrapMode wm, BitmapSampleMode sm)
    {
        FillTexture ft;
        ft.pTexture = pti; ft.TextureMatrix = m; ft.WrapMode = wm; ft.SampleMode = sm; 
        FillStyleBitmap(&ft);
    }
                        
#ifdef GFC_OS_PSP
    struct VertexXY16i
    {
        SInt16 x;
        SInt16 y;
        SInt16 z;

        VertexXY16i() { z = 0; }
    };
#else
    struct VertexXY16i
    {
        SInt16 x;
        SInt16 y;
    };
#endif

    // *** Gouraud shaded fills - used for EdgeAA 

    // Compact Gouraud-shaded vertex used for solid color/texture factor EdgeAA.
    // Can be used with GFill_Color, GFill_1Texture, GFill_Texture2.
#ifdef GFC_OS_PSP
    struct VertexXY16iC32
    {
        enum { VFormat = Vertex_XY16iC32 };
        UInt32  Color;
        SInt16  x, y, z;
        SInt16  filler;
    };
#else
    struct VertexXY16iC32
    {
        enum { VFormat = Vertex_XY16iC32 };
        SInt16  x, y;
        UInt32  Color;
    };
#endif

    // Complex vertex that combines gouraud shading and texture factors.
    struct VertexXY16iCF32
    {
        enum { VFormat = Vertex_XY16iCF32 };
        SInt16  x, y;
        UInt32  Color;
        UInt32  Factors;
        // Factors are interpreted as follows:
        //   .a  - texture 0 factor
        //   .b  - texture 1 factor
        //   .g  - texture 2 factor
    };

    // Gouraud shaded fill mode (describes how colors and textures are blended)
    enum GouraudFillType
    {
        GFill_Color,            // Interpolated with alpha channel is applied.
        GFill_1Texture,         // Texture0 is applied, with alpha modulation from Color.Alpha
        GFill_1TextureColor,    // Texture0 is mixed with vertex Colors based on Factors.Alpha.
        GFill_2Texture,         // Texture0 is mixed with Texture1 based on Colors.Alpha.
        GFill_2TextureColor,    // Texture0 is mixed with Texture1 and with vertex colors.
        GFill_3Texture          // Texture0, Texture1, and Texture2 are mixed together.
    };

    // Sets the interpolated color/texture fill style used for shapes with EdgeAA.
    // The specified textures are applied to triangle based on factors of gouraud 
    // vertex. Trailing argument texture pointers can be NULL, in which case
    // texture is not applied and vertex colors used instead.
    virtual void        FillStyleGouraud(GouraudFillType fillType,
                                         const FillTexture *ptexture0 = 0,
                                         const FillTexture *ptexture1 = 0,
                                         const FillTexture *ptexture2 = 0)              = 0;

    
    // Describes bitmap coordinate set used by DrawBitmaps.
    struct BitmapDesc
    {
        Rect    Coords;
        Rect    TextureCoords;
        GColor  Color;
    };

    // Draws a list of textured rectangular bitmaps; intended for glyph rendering.
    // The function ignores current matrix transform and uses m argument instead.
    // listSize describes the size of the list, while only [startIndex, count] piece
    // of it is used for rendering. Full list size is provided in case it is going
    // to be cached in the renderer for reuse.
    virtual void        DrawBitmaps(BitmapDesc* pbitmapList, int listSize,
                                     int startIndex, int count,
                                     const GTexture* pti, const Matrix& m,
                                     CacheProvider *pcache = 0 )                        = 0;


    // not supported
    struct DistanceFieldParams
    {
        Float   Width;

        Float   ShadowWidth;
        GColor  ShadowColor;
        GPointF ShadowOffset;
        GColor  GlowColor;
        Float   GlowSize[4];

        DistanceFieldParams() : Width(6), ShadowWidth(12), ShadowColor(0), ShadowOffset(1,1), GlowColor(0) {}
    };
    virtual void        DrawDistanceFieldBitmaps(BitmapDesc* pbitmapList, int listSize,
                                                 int startIndex, int count,
                                                 const GTexture* pti, const Matrix& m, const DistanceFieldParams& params,
                                                 CacheProvider *pcache = 0 )
    {
        GUNUSED4(pbitmapList,listSize,startIndex,count);
        GUNUSED4(pti,m,params,pcache);
    }


    // Describes the effect of a submit mask. Increment/Decrement modes only available
    // if multi-level masking is supported.
    enum SubmitMaskMode
    {
        Mask_Clear,     // Begins drawing of new mask shape after clearing the buffer.
        Mask_Increment, // Draws a new mask shape incrementing counter. Both masks will be applied.
        Mask_Decrement  // Draws a new mask shape decrementing counter. Only previous masks will be applied.
    };

    virtual void        BeginSubmitMask(SubmitMaskMode maskMode = Mask_Clear)           = 0;
    virtual void        EndSubmitMask()                                                 = 0;
    virtual void        DisableMask()                                                   = 0;

    enum FilterModes
    {
        Filter_Blur       = 1,
        Filter_Shadow     = 2,
        Filter_Highlight  = 4,        // Shadow + Highlight = Bevel

        Filter_Knockout   = 0x100,
        Filter_Inner      = 0x200,
        Filter_HideObject = 0x400,

	Filter_UserModes    = 0x0ffff,
	Filter_SkipLastPass = 0x10000,
	Filter_LastPassOnly = 0x20000,
    };

    enum FilterSupport
    {
        FilterSupport_None       = 0,
        FilterSupport_Ok         = 1,
        FilterSupport_Multipass  = 2,
        FilterSupport_Slow       = 4,
    };

    struct BlurFilterParams
    {
        UInt    Mode;
        Float   BlurX, BlurY;
        UInt    Passes;
        GPointF Offset;
        GColor  Color, Color2;
        Float   Strength;
        Cxform  cxform;

        BlurFilterParams() : Mode(0), BlurX(5), BlurY(5), Passes(1), Offset(0,0), Strength(1) {}
        BlurFilterParams(UInt mode, Float x, Float y, UInt n = 1, GPointF offset = GPointF(0,0), GColor color = GColor(), Float str = 1, Cxform& cx = Cxform::Identity)
            : Mode(mode), BlurX(x), BlurY(y), Passes(n), Offset(offset), Color(color), Strength(str), cxform(cx) {}

        BlurFilterParams(UInt mode, Float x, Float y, UInt n = 1, Float str = 1, Cxform& cx = Cxform::Identity)
            : Mode(mode), BlurX(x), BlurY(y), Passes(n), Offset(0,0), Strength(str), cxform(cx) {}

        bool operator==(const BlurFilterParams& b) const
        {
            return (Mode == b.Mode && BlurX == b.BlurX && BlurY == b.BlurY && Passes == b.Passes);
        }

	BlurFilterParams& Scale(Float x, Float y)
	{
	    BlurX = G_Max<Float>(1,BlurX*x);
	    BlurY = G_Max<Float>(1,BlurY*y);
	    Offset.x *= x;
	    Offset.y *= y;
	    return *this;
	}
    };

    virtual UInt        CheckFilterSupport(const BlurFilterParams& params)              = 0;
    virtual void        DrawBlurRect(GTexture* psrcin, const GRectF& insrcrect, const GRectF& indestrect, const BlurFilterParams& params) = 0;

    virtual void        DrawColorMatrixRect(GTexture* psrcin, const GRectF& insrcrect, const GRectF& destrect, const Float *matrix) = 0;
    
    // Obtains render stats, pass pstats = 0 to not fill it in.
    // Specify restStats = 1 for reset stat counters after retrieving them.
    virtual void        GetRenderStats(Stats *pstats, bool resetStats = 0)              = 0;

    // Obtains statistics for the movie view and performs a reset
    // for tracked counters.
    //   - If reset is true, it is done after the stats update.
    //   - If pbag is null, only reset can take place.    
    virtual void        GetStats(GStatBag* pbag, bool reset = true)                     = 0;


    // Releases all system resources. The method is used when this renderer is used 
    // in multi-threaded environment and should be called on the main thread just before 
    // the last reference on this render goes out of scope. All following calls to the renderer are illegal. 
    virtual void        ReleaseResources()                                              = 0;         

    // Add/Remove notification. Not all renderers support them.
    virtual bool        AddEventHandler(GRendererEventHandler *phandler);
    virtual void        RemoveEventHandler(GRendererEventHandler *phandler);
    void                CallHandlers(GRendererEventHandler::EventType event);

    // Helper function used for resizing images with filtering.
    static void GSTDCALL ResizeImage(UByte* pDst, 
                                    int dstWidth, int dstHeight, int dstPitch,
                                    const UByte* pSrc, 
                                    int srcWidth, int srcHeight, int srcPitch,
                                    ResizeImageType type);

protected:

    GList<GRendererEventHandler> Handlers;

#ifndef GFC_NO_3D
    StereoParams  S3DParams;
    StereoDisplay S3DDisplay;

    void GetStereoProjectionMatrix(GMatrix3D *left, GMatrix3D *right, const GMatrix3D &original, Float screenDist);
#endif
};
    


// Helper methods for texture creation.
inline GTexture*   GRenderer::CreateTextureFromImage(GImageBase* pim, UInt usage)
{
    GPtr<GTexture> ptexure = *CreateTexture();
    if (ptexure && ptexure->InitTexture(pim, usage))
    {
        ptexure->AddRef();
        return ptexure;
    }
    return 0;
}


// ***** Cache support API implementation

// Called by user to destroy data content and notify renderer.
void    GRenderer::CachedData::ReleaseData(GRenderer::CachedDataType type)
{
    if (pRenderer)
        pRenderer->ReleaseCachedData(this, type);
    ReleaseDataByRenderer();
}
// Renderer must call this if it dies or its data is lost internally.
void    GRenderer::CachedData::ReleaseDataByRenderer()
{
    pRenderer   = 0;
    hData       = 0;
}

GRenderer::CachedData::CachedData()
{
    pRenderer   = 0;
    hData       = 0;
}
GRenderer::CachedData::~CachedData()
{
    GASSERT(pRenderer == 0);
}

GRenderer::CacheProvider::CacheProvider(CachedData* pdata)
{
    pData = pdata;
    DiscardSharedData = 0;
}
GRenderer::CacheProvider::~CacheProvider()
{
}

// API for renderer.
GRenderer::CachedData*  GRenderer::CacheProvider::CreateCachedData(GRenderer::CachedDataType type,
                                                                   GRenderer* prenderer, bool keepSharedData)
{       
    GASSERT(prenderer && (pData->pRenderer != prenderer)); // Must not be called again if already created.
    if (pData->pRenderer != prenderer)
        pData->ReleaseData(type);
    pData->pRenderer    = prenderer;
    DiscardSharedData   = !keepSharedData;
    return pData;
}
GRenderer::CachedData*  GRenderer::CacheProvider::GetCachedData(GRenderer* prenderer)
{
    return (prenderer && (prenderer == pData->pRenderer)) ? pData : 0;
}



#endif 
