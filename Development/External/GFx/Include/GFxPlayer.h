/**********************************************************************

Filename    :   GFxPlayer.h
Content     :   Public interface to SWF File playback and Complex
                Objects API
Created     :   June 21, 2005
Authors     :   Michael Antonov, Prasad Silva

Notes       :

Copyright   :   (c) 2005-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFxPlayer_H
#define INC_GFxPlayer_H

#include "GTypes.h"
#include "GRefCount.h"
#include "GFxRenderConfig.h"
#include "GRenderer.h"
#include "GFxEvent.h"
#include "GFxLoader.h"

#include "GFxPlayerStats.h"

#include <ctype.h>  // for wchar_t
#include <stdarg.h> // for va_list args


// ***** Declared Classes
class GFxMovieDef;
class GFxMovie;
class GFxMovieView;
class GFxMovieRoot;
class GSoundRenderer;
class GASAsFunctionObject;
class GFxASUserData;

// ***** External Classes
class GFile;
class GColor;
class GImageInfoBase;
class GFxLog;

// ***** Movie Definition and Instance interfaces

// GFxMovieDef represents loaded shared data for an SWF movie file. These
// objects are normally created by GFxLoader::CreateMovie and are shared
// by all movie instances. If the movie definition is registered in the
// library during load time (see GFxLoader::LoadUseLibrary flag), loading
// the same file again will return a pointer to the same cached GFxMovieDef.
//
// Individual movie instances can be created by GFxMovieDef::CreateInstance.
// Once the instance is created, it is normally initialized with a
// renderer and used during playback.

class GFxMovieDef : public GFxResource, public GFxStateBag
{
public:

    // Query SWF file version, dimensions, and other stats.
    virtual UInt        GetVersion() const              = 0;
    virtual UInt        GetLoadingFrame() const         = 0;
    virtual Float       GetWidth() const                = 0;
    virtual Float       GetHeight() const               = 0;
    virtual UInt        GetFrameCount() const           = 0;
    virtual Float       GetFrameRate() const            = 0;
    // Returns frame rectangle, in pixels
    virtual GRectF      GetFrameRect() const            = 0;
    // Returns SWF file flags defined by GFxMovieInfo::SWFFlagConstants.
    virtual UInt        GetSWFFlags() const             = 0;
    // Returns the file path/URL from which this SWF was loaded.
    // This will normally be the path passed to GFxLoader::CreateMovie.
    virtual const char* GetFileURL() const              = 0;

    // WaitForLoadFinish waits until movie loading is completed.
    // Calling this function is important before Release() if you use
    // GFxThreadedTaskManager and memory arenas, as it insures that background
    // thread's memory references are released. If 'cancel' flag is passed,
    // loading will be canceled abruptly for the fastest possible shutdown.    
    // This function is used also used internally by GFxFontLib.
    virtual void        WaitForLoadFinish(bool cancel = false) const    = 0;
    // Waits until a specified frame is loaded
    virtual void        WaitForFrame(UInt frame) const  = 0;

    // SWF 8 File attributes, returned by GetFileAttributes
    enum FileAttrFlags
    {
        FileAttr_UseNetwork     = 0x0001,
        FileAttr_HasMetadata    = 0x0010
    };

    // Returns file attributes for SWF file version 8+. Will
    // return 0 for earlier SWF files.
    virtual UInt        GetFileAttributes() const                       = 0;
    // Retrieves meta-data that can be embedded into SWF-8 files. Returns
    // the number of bytes written, up to buffSize. Specify 0 for pbuff
    // to return the buffer size required.
    virtual UInt        GetMetadata(char *pbuff, UInt buffSize) const   = 0;

    virtual GMemoryHeap* GetLoadDataHeap() const        = 0;
    virtual GMemoryHeap* GetBindDataHeap() const        = 0;
    virtual GMemoryHeap* GetImageHeap() const           = 0;    

    virtual GFxResource* GetMovieDataResource() const   = 0;


    // Exporter/strip tool information - returns data about image extraction
    // arguments passed to 'gfxexport' tool when generating this '.gfx' file.
    // Null will be returned if this movie def came from a '.swf'.
    virtual const GFxExporterInfo*  GetExporterInfo() const             = 0;


    struct MemoryParams
    {
        // Heap descriptor for the heap that will be created for the movie. It is
        // possible to specify heap alignment, granularity, limit, flags. If heap '
        // limit is specified then GFx will try to maintain heap footprint on or 
        // below this limit.
        GMemoryHeap::HeapDesc   Desc;

        // A heap limit multiplier (0..1] that is used to determine how the heap limit
        // grows if it is exceeded. The algorithm is as follows:
        // if (allocs since collect >= heap footprint * HeapLimitMultiplier)
        //     collect
        // else
        //     expand(heap footprint + overlimit + heap footprint * HeapLimitMultiplier)
        Float                   HeapLimitMultiplier;

        // Number of roots before garbage collection is executed. This is an initial
        // value of max roots cap; it might be increased during the execution.
        // Set this value to 0 turn off number-of-roots-based collections.
        UInt    MaxCollectionRoots;

        // Number of frames after which collection is forced, even if the max roots
        // cap is not reached. It is useful to perform intermediate collections 
        // in the case if current max roots cap is high, to reduce the cost of that
        // collection when it occurs. It is good to set FramesBetweenCollections to
        // a large value such as 1800, which would force collection every 30 seconds
        // if the file frame rate is 60 FPS. Set this to 0 to turn off frame-based
        // collection.
        UInt    FramesBetweenCollections;

        MemoryParams(UPInt memoryArena = 0)
        {
            Desc.Arena                 = memoryArena;
            HeapLimitMultiplier        = 0.25; // 25%
            MaxCollectionRoots         = ~0u; // Default value will be used.
            FramesBetweenCollections   = ~0u; // Default value will be used.
        }
    };

    // Abstract class that holds memory data for MovieView creation
    class MemoryContext : public GRefCountBase<MemoryContext, GStat_Default_Mem>
    {
    public:
        virtual ~MemoryContext() { }
    protected:
        // private constructor. Use CreateMemoryContext to create.
        MemoryContext() { }
    };

    // Creates a memory context to be passed into CreateInstance, below
    virtual MemoryContext* CreateMemoryContext(const char* heapName, const MemoryParams& memParams, bool debugHeap) = 0;

    // Creates a movie view instance that is used for playback. Each movie
    // instance will have its own timeline, animation and Action-Script state.
    //
    // The returned created movie is AddRef-ed, so it can be stored in
    // a smart pointer and released when no longer necessary. Null pointer
    // will be returned in case of failure (low memory, etc.).    
    //   - memParams :      Specifies memory parameters describing heaps and garbage
    //                      collector behavior. memParams.Desc.Arena might be used
    //                      to specify memory arena index.
    //   - initFirstFrame : Set this to 'true' to to initialize objects of frame 1,
    //                      populating the display list. If false, the display list
    //                      will be empty until the first Advance call.
    // Ex:  GPtr<GFxMovieView> pmovie = *pdef->CreateInstance();    
    virtual GFxMovieView*   CreateInstance(const MemoryParams& memParams, 
                                           bool initFirstFrame = true)  = 0;

    //   - memContext :     Memory context that encapsulates the common MovieView heap, as well as 
    //                      other shared objects, such as the garbage collector.
    virtual GFxMovieView*   CreateInstance(MemoryContext* memContext, 
                                           bool initFirstFrame = true) = 0;                                        

    //   - initFirstFrame : Set this to 'true' to to initialize objects of frame 1,
    //                      populating the display list. If false, the display list
    //                      will be empty until the first Advance call.
    //   - memoryArena :    Index of memory arena to be used for memory allocations.
    GFxMovieView*   CreateInstance(bool initFirstFrame = true, UPInt memoryArena = 0)
    {
        GFxMovieDef::MemoryParams memParams;
        memParams.Desc.Arena = memoryArena;
        return CreateInstance(memParams, initFirstFrame);
    }



    // ***  Import interface

    // Import support interface. SWF files can import dictionary symbols from
    // other SWF files. This is done automatically if GFxLoader::LoadImports flag
    // is specified when loading a movie (default for GFxLoader::CreateMovie).

    // Visitor interface, used for movie enumeration through VisitImportedMovies.
    struct ImportVisitor
    {
        virtual ~ImportVisitor () { }

        // Users must override Visit.
        //  - pimportedMovieFilename is the partial filename being used for import;
        //    to get the full path use pimportDef->GetFileURL().
        virtual void    Visit(GFxMovieDef* pparentDef, GFxMovieDef* pimportDef,
                              const char* pimportedMovieFilename) = 0;
    };

    // Enumerates a list of names of external SWF files imported into this movie.
    // Calls pvistor->Visit() method for each import movie file name.
    virtual void    VisitImportedMovies(ImportVisitor* pvisitor)            = 0;



    // *** Resource enumeration

    // Resource enumeration is used primarily by the GFxExport tool to collect
    // all of the resources within the file.

    // Visitor interface, used for movie enumeration through VisitImportedMovies.
    struct ResourceVisitor : public GFxFileConstants
    {
        virtual ~ResourceVisitor () { }       

        virtual void    Visit(GFxMovieDef* pmovieDef, GFxResource* presource,
                              GFxResourceId rid, const char* pexportName) = 0;
    };

    enum VisitResourceMask
    {
        ResVisit_NestedMovies   = 0x8000,

        // Types of resources to visit:
        ResVisit_Fonts          = 0x01,
        ResVisit_Bitmaps        = 0x02,
        ResVisit_GradientImages = 0x04,
        ResVisit_EditTextFields = 0x08,
        ResVisit_Sounds         = 0x10,
        ResVisit_Sprite         = 0x20,
        
        // Combined flags.
        ResVisit_AllLocalImages = (ResVisit_Bitmaps | ResVisit_GradientImages),
        ResVisit_AllImages      = (ResVisit_Bitmaps | ResVisit_GradientImages | ResVisit_NestedMovies)
    };
    
    // Enumerates all resources.
    // Calls pvistor->Visit() method for each resource in the SWF/GFX file.
    virtual void    VisitResources(ResourceVisitor* pvisitor, UInt visitMask = ResVisit_AllImages) = 0;


    /*
    // Fills in extracted image file info for a given characterId. Character
    // ids are passed to ImageVisitor::Visit method. Returns 0 if such
    // information is not available.
    virtual bool    GetImageFileInfo(GFxImageFileInfo *pinfo, int characterId)      = 0;
    */

    // Retrieves a resource pointer based on an export name. Export name is specified in
    // the flash studio by right-clicking on the bitmap in the library and
    // modifying its Linkage identifier attribute. The symbol must also be
    // exported for runtime or ActionScript sharing.
    virtual GFxResource*     GetResource(const char *pexportName) const         = 0;

};

// ***** GFxImportVisitor

// Default import visitor - used to enumerate imports. This visitor is called 
// during loading (import binding stage) after a each import is resolved.
class   GFxImportVisitor : public GFxState, public GFxMovieDef::ImportVisitor, public GFxFileConstants
{
public:    
    GFxImportVisitor() : GFxState(State_ImportVisitor) { }       
    virtual ~GFxImportVisitor () { }       

    // Users must override Visit.
    //  - pimportedMovieFilename is the partial filename being used for import;
    //    to get the full path use pimportDef->GetFileURL().
    virtual void    Visit(GFxMovieDef* pparentDef, GFxMovieDef* pimportDef,
                          const char* pimportedMovieFilename) = 0;
};

// Sets a default visitor that is used when after files are loaded.
void GFxStateBag::SetImportVisitor(GFxImportVisitor *ptr)
{
    SetState(GFxState::State_ImportVisitor, ptr);
}
GPtr<GFxImportVisitor> GFxStateBag::GetImportVisitor() const
{
    return *(GFxImportVisitor*) GetStateAddRef(GFxState::State_ImportVisitor);
}




// ***** GFxValue - used to pass/receive values from ActionScript.

//
// GFxValue is capable of holding a simple or complex type and the corresponding value.
// These values can be passed to and from the application context to the AS runtime.
// The supported types are:
//  * Undefined     - The AS undefined value
//  * Null          - The AS null value
//  * Boolean       - The AS Boolean type (C++ bool analogue)
//  * Number        - The AS Number type (C++ Double analogue)
//  * String        - The AS String type. Can be a string managed by
//                    the application, or a string from the AS runtime.
//  * StringW       - Similar to String, in that it corresponds to the AS Strinb value,
//                    however, it provides a convenient interface for applications
//                    that work in a wide char environment.
//  * Object        - The AS Object type (and all objects derived from
//                    it). A special API interface is provide to manipulate such 
//                    objects.
//  * Array         - A special case of the Object type. An additional API is provide
//                    to manipulate AS arrays.
//  * DisplayObject - A special case of the Object type. Corresponds to entities on
//                    the stage (MovieClips, Buttons, and TextFields). A custom API
//                    is provide to manipulate display properties of such objects.
//
// ** Strings
// The application is expected to manage their lifetimes when GFxValues are assigned 
// const char* or const wchar_t* pointers. Strings returned from the AS runtime are 
// managed by the runtime. To create a string that is managed by the runtime, use the
// GFxMovie::CreateString() or GFxMovie::CreateStringW() method.
//
// ** Objects
// GFxValues of type Object (and Array, DisplayObject) are returned by the AS runtime.
// The application can create instances of the base Object class type, by using 
// the GFxMovie::CreateObject() method. This method also takes a class name that allows
// users to create instances of specific classes. To create an instance of Array, the 
// GFxMovie::CreateArray() method can be used. Currently, there is no interface to
// create instances on the stage (of type DisplayObject).
//
// ** Storage
// GFxValues are expected to be non-movable, due to the tracking mechanism in debug
// builds. If GFxValues are stored in a dynamic array, use a C++ compliant dynamic 
// array type (such as the STL vector, or the GArrayCPP container). These guarantee
// that the GFxValues are destroyed and created appropriately on container resize.
//


#ifdef GFC_BUILD_DEBUG
//
// The list node interface is used to keep track of AS object references. The user
// is warned in debug builds if there are GFxValues still holding AS object 
// references when the GFxMovieRoot that owns the AS objects is about to be 
// released. This condition can cause a crash because the memory heap used
// by the AS objects is released when the GFxMovieRoot dies.
//
class GFxValue : public GListNode<GFxValue> 
//
#else
//
class GFxValue
//
#endif
{
    friend class GFxMovieRoot;
    friend class GFxASUserData;

public:
    // Structure to modify display properties of an object on the stage (MovieClip,
    // TextField, Button). Used in conjunction with GFxValue::Get/SetDisplayInfo.
    //
    class DisplayInfo
    {
    public:
        enum Flags
        {
            V_x         = 0x001,
            V_y         = 0x002,
            V_rotation  = 0x004,
            V_xscale    = 0x008,
            V_yscale    = 0x010,
            V_alpha     = 0x020,
            V_visible   = 0x040,
            V_z         = 0x080,
            V_xrotation = 0x100,
            V_yrotation = 0x200,
            V_zscale    = 0x400,
            V_perspFOV  = 0x800,
            V_perspMatrix3D = 0x1000,
            V_viewMatrix3D  = 0x2000
        };

        DisplayInfo() : VarsSet(0)                     {}
        DisplayInfo(Double x, Double y) : VarsSet(0)   { SetFlags(V_x | V_y); X = x; Y = y; }
        DisplayInfo(Double rotation) : VarsSet(0)      { SetFlags(V_rotation); Rotation = rotation; }
        DisplayInfo(bool visible) : VarsSet(0)         { SetFlags(V_visible); Visible = visible; }

        void    Initialize(UInt16 varsSet, Double x, Double y, Double rotation, Double xscale, 
                           Double yscale, Double alpha, bool visible,
                           Double z, Double xrotation, Double yrotation, Double zscale)
        {
            VarsSet = varsSet;
            X = x;
            Y = y;
            Rotation = rotation;
            XScale = xscale;
            YScale = yscale;
            Alpha = alpha;
            Visible = visible;            
#ifndef GFC_NO_3D
            Z = z;
            XRotation = xrotation;
            YRotation = yrotation;
            ZScale = zscale;
            PerspFOV = DEFAULT_FLASH_FOV;
#else
            GUNUSED4(z, xrotation, yrotation, zscale);
#endif
        }
        void    Clear()                         { VarsSet = 0; }

        void    SetX(Double x)                  { SetFlags(V_x); X = x; }
        void    SetY(Double y)                  { SetFlags(V_y); Y = y; }
        void    SetRotation(Double degrees)     { SetFlags(V_rotation); Rotation = degrees; }
        void    SetXScale(Double xscale)        { SetFlags(V_xscale); XScale = xscale; }    // 100 == 100%
        void    SetYScale(Double yscale)        { SetFlags(V_yscale); YScale = yscale; }    // 100 == 100%
        void    SetAlpha(Double alpha)          { SetFlags(V_alpha); Alpha = alpha; }
        void    SetVisible(bool visible)        { SetFlags(V_visible); Visible = visible; }

#ifndef GFC_NO_3D
        void    SetZ(Double z)                  { SetFlags(V_z); Z = z; }
        void    SetXRotation(Double degrees)    { SetFlags(V_xrotation); XRotation = degrees; }
        void    SetYRotation(Double degrees)    { SetFlags(V_yrotation); YRotation = degrees; }
        void    SetZScale(Double zscale)        { SetFlags(V_zscale); ZScale = zscale; }    // 100 == 100%
        void    SetPerspFOV(Double fov)         { SetFlags(V_perspFOV); PerspFOV = fov; }
        void    SetPerspective3D(const GMatrix3D *pmat)  
        { 
            if (pmat) 
            { 
                SetFlags(V_perspMatrix3D); 
                PerspectiveMatrix3D = *pmat;
            } 
            else
                ClearFlags(V_perspMatrix3D); 
        }
        void    SetView3D(const GMatrix3D *pmat) 
        { 
            if (pmat) 
            { 
                SetFlags(V_viewMatrix3D); 
                ViewMatrix3D = *pmat;
            } 
            else
                ClearFlags(V_viewMatrix3D); 
        }
#else
        void    SetZ(Double z)                  { GUNUSED(z); }
        void    SetXRotation(Double degrees)    { GUNUSED(degrees); }
        void    SetYRotation(Double degrees)    { GUNUSED(degrees); }
        void    SetZScale(Double zscale)        { GUNUSED(zscale); }
        void    SetPerspFOV(Double fov)         { GUNUSED(fov); }
        void    SetPerspective3D(const GMatrix3D *pmat)  { GUNUSED(pmat); }
        void    SetView3D(const GMatrix3D *pmat)         { GUNUSED(pmat); }
#endif
        void    SetPosition(Double x, Double y)         { SetFlags(V_x | V_y); X = x; Y = y; }
        void    SetScale(Double xscale, Double yscale)  { SetFlags(V_xscale | V_yscale); XScale = xscale; YScale = yscale; }

        void    Set(Double x, Double y, Double rotation, Double xscale, Double yscale, 
                    Double alpha, bool visible,
                    Double z, Double xrotation, Double yrotation, Double zscale)
        {
            X = x;
            Y = y;
            Rotation = rotation;
            XScale = xscale;
            YScale = yscale;
            Alpha = alpha;
            Visible = visible;
            SetFlags(V_x | V_y | V_rotation | V_xscale | V_yscale | V_alpha | V_visible);
#ifndef GFC_NO_3D
            Z = z;
            XRotation = xrotation;
            YRotation = yrotation;
            ZScale = zscale;
            SetFlags(V_z | V_zscale | V_xrotation | V_yrotation);
#else
            GUNUSED4(z, xrotation, yrotation, zscale);
#endif
        }

        Double  GetX() const            { return X; }
        Double  GetY() const            { return Y; }
        Double  GetRotation() const     { return Rotation; }
        Double  GetXScale() const       { return XScale; }
        Double  GetYScale() const       { return YScale; }
        Double  GetAlpha() const        { return Alpha; }
        bool    GetVisible() const      { return Visible; }
#ifndef GFC_NO_3D
        Double  GetZ() const            { return Z; }
        Double  GetXRotation() const    { return XRotation; }
        Double  GetYRotation() const    { return YRotation; }
        Double  GetZScale() const       { return ZScale; }
        Double  GetPerspFOV() const     { return PerspFOV; }
        const GMatrix3D* GetPerspective3D() const   { return (IsFlagSet(V_perspMatrix3D)) ? &PerspectiveMatrix3D : NULL; }
        const GMatrix3D* GetView3D() const          { return (IsFlagSet(V_viewMatrix3D)) ? &ViewMatrix3D : NULL; }
#else
        Double  GetZ() const            { return 0; }
        Double  GetXRotation() const    { return 0; }
        Double  GetYRotation() const    { return 0; }
        Double  GetZScale() const       { return 0; }
        Double  GetPerspFOV() const     { return 0; }
        const GMatrix3D* GetPerspective3D() const   { return NULL; }
        const GMatrix3D* GetView3D() const          { return NULL; }
#endif
        bool    IsFlagSet(UInt flag) const      { return 0 != (VarsSet & flag); }

    private:
        Double  X;
        Double  Y;
        Double  Rotation;
        Double  XScale;
        Double  YScale;
        Double  Alpha;
        bool    Visible;
#ifndef GFC_NO_3D
        Double  Z;
        Double  XRotation;
        Double  YRotation;
        Double  ZScale;
        Double  PerspFOV;
        GMatrix3D ViewMatrix3D;
        GMatrix3D PerspectiveMatrix3D;
#endif        
        UInt16   VarsSet;

        void    SetFlags(UInt flags)    { VarsSet |= flags; }
        void    ClearFlags(UInt flags)  { VarsSet &= ~flags; }
    };

protected:

    enum ValueTypeControl
    {
        // ** Special Bit Flags
        // Explicit coercion to type requested 
        VTC_ConvertBit      = 0x80,
        // Flag that denotes managed resources
        VTC_ManagedBit      = 0x40,

        // ** Type mask
        VTC_TypeMask        = VTC_ConvertBit | 0x0F,
    };

    //
    // Internal interface used by GFxValue to manipulate AS Objects
    //
    class ObjectInterface : public GNewOverrideBase<GFxStatMV_Other_Mem>
    {
    public:
        class ObjVisitor
        {
        public:
            virtual ~ObjVisitor() {}
            virtual void    Visit(const char* name, const GFxValue& val) = 0;
        };
        class ArrVisitor
        {
        public:
            virtual ~ArrVisitor() {}
            virtual void    Visit(UInt idx, const GFxValue& val) = 0;
        };

        ObjectInterface(GFxMovieRoot* pmovieRoot) : pMovieRoot(pmovieRoot) {}

        void    ObjectAddRef(GFxValue* val, void* pobj);
        void    ObjectRelease(GFxValue* val, void* pobj);

        bool    HasMember(void* pdata, const char* name, bool isdobj) const;
        bool    GetMember(void* pdata, const char* name, GFxValue* pval, bool isdobj) const;
        bool    SetMember(void* pdata, const char* name, const GFxValue& value, bool isdobj);
        bool    Invoke(void* pdata, GFxValue* presult, const char* name, const GFxValue* pargs, 
                       UPInt nargs, bool isdobj);
        bool    DeleteMember(void* pdata, const char* name, bool isdobj);
        void    VisitMembers(void* pdata, ObjVisitor* visitor, bool isdobj) const;

        UInt    GetArraySize(void* pdata) const;
        bool    SetArraySize(void* pdata, UInt sz);
        bool    GetElement(void* pdata, UInt idx, GFxValue *pval) const;
        bool    SetElement(void* pdata, UInt idx, const GFxValue& value);
        void    VisitElements(void* pdata, ArrVisitor* visitor, UInt idx, SInt count) const;
        bool    PushBack(void* pdata, const GFxValue& value);
        bool    PopBack(void* pdata, GFxValue* pval);
        bool    RemoveElements(void* pdata, UInt idx, SInt count);

        bool    IsDisplayObjectActive(void* pdata) const;
        bool    GetDisplayInfo(void* pdata, DisplayInfo* pinfo) const;
        bool    SetDisplayInfo(void* pdata, const DisplayInfo& info);        
        bool    GetDisplayMatrix(void* pdata, GMatrix2D* pmat) const;
        bool    SetDisplayMatrix(void* pdata, const GMatrix2D& mat);
        bool    GetMatrix3D(void* pdata, GMatrix3D* pmat) const;
        bool    SetMatrix3D(void* pdata, const GMatrix3D& mat);
        bool    GetCxform(void* pdata, GRenderer::Cxform* pcx) const;
        bool    SetCxform(void* pdata, const GRenderer::Cxform& cx);

        bool    GetText(void* pdata, GFxValue* pval, bool ishtml) const;
        bool    SetText(void* pdata, const char* ptext, bool ishtml);
        bool    SetText(void* pdata, const wchar_t* ptext, bool ishtml);

        bool    CreateEmptyMovieClip(void* pdata, GFxValue* pmc, const char* instanceName, SInt depth);
        bool    AttachMovie(void* pdata, GFxValue* pmc, const char* symbolName, 
                            const char* instanceName, SInt depth, const GFxValue* initObj);
        bool    GotoAndPlay(void* pdata, const char* frame, bool stop);
        bool    GotoAndPlay(void* pdata, UInt frame, bool stop);

        bool    IsSameContext(ObjectInterface* pobjInterface)   { return (pMovieRoot == pobjInterface->pMovieRoot); }

        void    ToString(GString* pstr, const GFxValue& thisVal) const;

#ifndef GFC_NO_FXPLAYER_AS_USERDATA
        void            SetUserData(void* pdata, GFxASUserData* puserdata, bool isdobj);
        GFxASUserData*  GetUserData(void* pdata, bool isdobj) const;
#endif  // GFC_NO_FXPLAYER_AS_USERDATA

    protected:
        GFxMovieRoot*   pMovieRoot;


#ifdef GFC_BUILD_DEBUG
        //
        // ** Special GFxValue tracking interface
        //
    public:
        bool    HasTaggedValues() const { return !ExternalObjRefs.IsEmpty(); }
        void    DumpTaggedValues() const;

    protected:
        //
        // Keeps track of AS object references held by GFxValue. If not empty
        // when GFxMovieRoot is released, warns user to release the GFxValues
        // holding the AS object references. Not releasing the GFxValues will
        // lead to a crash due to freed memory heap.
        //
        GList<GFxValue>     ExternalObjRefs;
#endif
    };

public:

    // The type of value stored in GFxValue.
    enum ValueType
    {
        // ** Type identifiers

        // Basic types
        VT_Undefined        = 0x00,
        VT_Null             = 0x01,
        VT_Boolean          = 0x02,
        VT_Number           = 0x03,
        VT_String           = 0x04,

        // StringW can be passed as an argument type, but it will only be returned 
        // if VT_ConvertStringW was specified, as it is not a native type.
        VT_StringW          = 0x05,  // wchar_t* string
        
        // ActionScript VM objects
        VT_Object           = 0x06,
        VT_Array            = 0x07,
        // Special type for stage instances (MovieClips, Buttons, TextFields)
        VT_DisplayObject    = 0x08,

        // Specify this type to request SetVariable/Invoke result to be converted 
        // to the specified type. After return of the function, the specified type
        // will *ALWAYS* be stored in the value.
        VT_ConvertBoolean   = VTC_ConvertBit | VT_Boolean,
        VT_ConvertNumber    = VTC_ConvertBit | VT_Number,
        VT_ConvertString    = VTC_ConvertBit | VT_String,
        VT_ConvertStringW   = VTC_ConvertBit | VT_StringW
        // Cannot convert to VM objects
    };

    // Constructors.
    GFxValue()                      : pObjectInterface(NULL), Type(VT_Undefined)  { }
    GFxValue(ValueType type)        : pObjectInterface(NULL), Type(type)          
    { 
        // Cannot create complex types. They are only returned from the AS runtime.
        // To create instances of Object and Array, use the GFxMovie::CreateObject
        // and CreateArray methods.
        GASSERT(type != VT_Object && type != VT_Array && type != VT_DisplayObject);
        Value.pString = 0; 
    }
    GFxValue(Double v)              : pObjectInterface(NULL), Type(VT_Number)     { Value.NValue = v; }
    GFxValue(bool v)                : pObjectInterface(NULL), Type(VT_Boolean)    { Value.BValue = v; }
    GFxValue(const char* ps)        : pObjectInterface(NULL), Type(VT_String)     { Value.pString = ps; }
    GFxValue(const wchar_t* ps)     : pObjectInterface(NULL), Type(VT_StringW)    { Value.pStringW = ps; }

    GFxValue(const GFxValue& src)   : pObjectInterface(NULL), Type(src.Type)
    { 
        Value = src.Value;
        if (src.IsManagedValue()) AcquireManagedValue(src);
    }

    ~GFxValue()     { if (IsManagedValue()) ReleaseManagedValue(); }

    const GFxValue& operator = (const GFxValue& src) 
    {         
        if (this != &src) 
        {
            if (IsManagedValue()) ReleaseManagedValue();
            Type = src.Type; 
            Value = src.Value; 
            if (src.IsManagedValue()) AcquireManagedValue(src);
        }
        return *this; 
    }

	bool operator == (const GFxValue& other) const
	{
		if (Type != other.Type) return false;
		switch (Type & 0x0F)
		{
		case VT_Boolean:	return Value.BValue == other.Value.BValue;
		case VT_Number:		return Value.NValue == other.Value.NValue;
		case VT_String:		return !G_strcmp(GetString(), other.GetString());
		case VT_StringW:	return !G_wcscmp(GetStringW(), other.GetStringW());
		default:			return Value.pData == other.Value.pData;
		}
	}
    
    GString         ToString() const;
    const wchar_t*  ToStringW(wchar_t* pwstr, UPInt length) const;

    // Get type based on which you can interpret the value.
    ValueType   GetType() const         { return ValueType(Type & VTC_TypeMask); }

    // Check types
    bool        IsUndefined() const     { return GetType() == VT_Undefined; }
    bool        IsNull() const          { return GetType() == VT_Null; }
    bool        IsBool() const          { return GetType() == VT_Boolean; }
    bool        IsNumber() const        { return GetType() == VT_Number; }
    bool        IsString() const        { return GetType() == VT_String; }
    bool        IsStringW() const       { return GetType() == VT_StringW; }
    bool        IsObject() const        { return (GetType() == VT_Object) || 
                                                          GetType() == VT_Array || 
                                                          GetType() == VT_DisplayObject; }
    bool        IsArray() const         { return GetType() == VT_Array; }
    bool        IsDisplayObject() const { return GetType() == VT_DisplayObject; }

    // Get values for each type.
    bool        GetBool() const         { GASSERT(IsBool()); return Value.BValue; }
    Double      GetNumber() const       { GASSERT(IsNumber()); return Value.NValue;  }
    const char* GetString() const       
    { 
        GASSERT(IsString()); 
        return IsManagedValue() ? *Value.pStringManaged : Value.pString; 
    }
    const wchar_t* GetStringW() const   { GASSERT(IsStringW()); return Value.pStringW; }

    // Set types and values from user context.
    void        SetUndefined()                  { ChangeType(VT_Undefined); }
    void        SetNull()                       { ChangeType(VT_Null); }
    void        SetBoolean(bool v)              { ChangeType(VT_Boolean);   Value.BValue = v; }
    void        SetNumber(Double v)             { ChangeType(VT_Number);    Value.NValue = v; }
    void        SetString(const char* p)        { ChangeType(VT_String);    Value.pString = p; }
    void        SetStringW(const wchar_t* p)    { ChangeType(VT_StringW);   Value.pStringW = p; }

    void        SetConvertBoolean()      { ChangeType(VT_ConvertBoolean); }
    void        SetConvertNumber()       { ChangeType(VT_ConvertNumber); }
    void        SetConvertString()       { ChangeType(VT_ConvertString); }
    void        SetConvertStringW()      { ChangeType(VT_ConvertStringW); }

    //
    // **** Methods to access and manipulate AS objects
    // 

public:
    // Visitor interfaces
    //
    typedef     ObjectInterface::ObjVisitor     ObjectVisitor;
    typedef     ObjectInterface::ArrVisitor     ArrayVisitor;


    // ----------------------------------------------------------------
    // AS Object support. These methods are only valid for Object type
    // (which includes Array and DisplayObject types)
    //
    bool        HasMember(const char* name) const 
    {
        GASSERT(IsObject());
        // If this is pointing to a display object and false is returned, then the 
        // display object may not exist on the stage.
        return pObjectInterface->HasMember(Value.pData, name, IsDisplayObject());
    }
    bool        GetMember(const char* name, GFxValue* pval) const
    {
        GASSERT(IsObject());
        // If the object does not contain the member, false is returned.If this is 
        // pointing to a display object and false is returned, then the display object 
        // may not exist on the stage. 
        return pObjectInterface->GetMember(Value.pData, name, pval, IsDisplayObject());
    }
    bool        SetMember(const char* name, const GFxValue& val)
    {
        GASSERT(IsObject());
        // If this is pointing to a display object and false is returned, then the 
        // display object may not exist on the stage.
        return pObjectInterface->SetMember(Value.pData, name, val, IsDisplayObject());
    }
    bool        Invoke(const char* name, GFxValue* presult, const GFxValue* pargs, UPInt nargs)
    {
        GASSERT(IsObject());
        // If this is pointing to a display object and false is returned, then the 
        // display object may not exist on the stage.
        return pObjectInterface->Invoke(Value.pData, presult, name, pargs, nargs, IsDisplayObject());
    }
    bool        Invoke(const char* name, GFxValue* presult = NULL)   
    { 
        return Invoke(name, presult, NULL, 0); 
    }
    void        VisitMembers(ObjectVisitor* visitor) const
    {
        GASSERT(IsObject());
        pObjectInterface->VisitMembers(Value.pData, visitor, IsDisplayObject());
    }
    bool        DeleteMember(const char* name)
    {
        GASSERT(IsObject());
        // If this is pointing to a display object and false is returned, then the 
        // display object may not exist on the stage.
        return pObjectInterface->DeleteMember(Value.pData, name, IsDisplayObject());
    }
    
#ifndef GFC_NO_FXPLAYER_AS_USERDATA

    void        SetUserData(GFxASUserData* pdata)
    {
        GASSERT(IsObject());
        pObjectInterface->SetUserData(Value.pData, pdata, IsDisplayObject());
    }
    GFxASUserData* GetUserData() const
    {
        GASSERT(IsObject());
        return pObjectInterface->GetUserData(Value.pData, IsDisplayObject());
    }

#endif  // GFC_NO_FXPLAYER_AS_USERDATA

    // ----------------------------------------------------------------
    // AS Array support. These methods are only valid for Array type
    //
    UInt        GetArraySize() const
    {
        GASSERT(IsArray());
        return pObjectInterface->GetArraySize(Value.pData);
    }
    bool        SetArraySize(UInt sz)
    {
        GASSERT(IsArray());
        return pObjectInterface->SetArraySize(Value.pData, sz);
    }
    bool        GetElement(UInt idx, GFxValue* pval) const
    {
        GASSERT(IsArray());
        return pObjectInterface->GetElement(Value.pData, idx, pval);
    }
    bool        SetElement(UInt idx, const GFxValue& val)
    {
        GASSERT(IsArray());
        return pObjectInterface->SetElement(Value.pData, idx, val);
    }
    void        VisitElements(ArrayVisitor* visitor, UInt idx, SInt count = -1) const
    {
        GASSERT(IsArray());
        return pObjectInterface->VisitElements(Value.pData, visitor, idx, count);
    }
    void        VisitElements(ArrayVisitor* visitor) const  { VisitElements(visitor, 0); }
    bool        PushBack(const GFxValue& val)
    {
        GASSERT(IsArray());
        return pObjectInterface->PushBack(Value.pData, val);
    }
    bool        PopBack(GFxValue* pval = NULL)
    {
        GASSERT(IsArray());
        return pObjectInterface->PopBack(Value.pData, pval);
    }
    bool        RemoveElements(UInt idx, SInt count = -1)
    {
        GASSERT(IsArray());
        return pObjectInterface->RemoveElements(Value.pData, idx, count);
    }
    bool        RemoveElement(UInt idx)     { return RemoveElements(idx, 1); }
    bool        ClearElements()             { return RemoveElements(0); }


    // ----------------------------------------------------------------
    // AS display object (MovieClips, Buttons, TextFields) support. These
    // methods are only valid for DisplayObject type
    //
    bool        IsDisplayObjectActive() const
    {
        GASSERT(IsDisplayObject());
        // If false is returned, then the display object does not exist on the stage.
        return pObjectInterface->IsDisplayObjectActive(Value.pData);
    }
    bool        GetDisplayInfo(DisplayInfo* pinfo) const
    {
        GASSERT(IsDisplayObject());
        // If false is returned, then the display object may not exist on the stage
        return pObjectInterface->GetDisplayInfo(Value.pData, pinfo);
    }
    bool        SetDisplayInfo(const DisplayInfo& info)
    {
        GASSERT(IsDisplayObject());
        // If false is returned, then the display object may not exist on the stage
        return pObjectInterface->SetDisplayInfo(Value.pData, info);
    }
    bool        GetDisplayMatrix(GMatrix2D* pmat) const
    {
        // NOTE: Returned matrix contains translation values in pixels, rotation in 
        // radians and scales in normalized form (100% == 1.0).
        GASSERT(IsDisplayObject());
        // If false is returned, then the display object may not exist on the stage
        return pObjectInterface->GetDisplayMatrix(Value.pData, pmat);
    }
    bool        SetDisplayMatrix(const GMatrix2D& mat)
    {
        // NOTE: Param matrix expected to contain translation values in pixels, rotation in 
        // radians and scales in normalized form (100% == 1.0).
        GASSERT(IsDisplayObject());
        // If false is returned, then the display object may not exist on the stage or
        // the matrix has invalid (non-finite) values.
        return pObjectInterface->SetDisplayMatrix(Value.pData, mat);
    }
    bool        GetMatrix3D(GMatrix3D* pmat) const
    {
#ifndef GFC_NO_3D
        // NOTE: Returned matrix contains translation XY values in pixels, Z in world space, rotation in 
        // radians and scales in normalized form (100% == 1.0).
        GASSERT(IsDisplayObject());
        // If false is returned, then the display object may not exist on the stage
        return pObjectInterface->GetMatrix3D(Value.pData, pmat);
#else
        GUNUSED(pmat);
        return false;
#endif
    }
    bool        SetMatrix3D(const GMatrix3D& mat)
    {
#ifndef GFC_NO_3D
        // NOTE: Param matrix expected to contain translation XY values in pixels, Z in world space, rotation in 
        // radians and scales in normalized form (100% == 1.0).
        GASSERT(IsDisplayObject());
        // If false is returned, then the display object may not exist on the stage or
        // the matrix has invalid (non-finite) values.
        return pObjectInterface->SetMatrix3D(Value.pData, mat);
#else
        GUNUSED(mat);
        return false;
#endif
    }
    bool        GetColorTransform(GRenderer::Cxform* pcx) const
    {
        GASSERT(IsDisplayObject());
        // If false is returned, then the display object may not exist on the stage
        return pObjectInterface->GetCxform(Value.pData, pcx);
    }
    bool        SetColorTransform(GRenderer::Cxform& cx)
    {
        GASSERT(IsDisplayObject());
        // If false is returned, then the display object may not exist on the stage
        return pObjectInterface->SetCxform(Value.pData, cx);
    }


    // ----------------------------------------------------------------
    // AS TextField support.
    //
    // The following methods provide efficient access to the TextField.text and 
    // TextField.htmlText properties. For all other display object types, they 
    // will be equivalent to the following:
    //
    //  SetText(..)     -> SetMember("text", ..)
    //  SetTextHTML(..) -> SetMember("htmlText"), ..)
    //  GetText(..)     -> GetMember("text", ..)
    //  GetTextHTML(..) -> GetMember("htmlText", ..)
    //
    bool        SetText(const char* ptext)
    {
        GASSERT(IsDisplayObject());
        // If false is returned, then the display object may not exist on the stage,
        // or does not contain the 'text' property.
        return pObjectInterface->SetText(Value.pData, ptext, false);
    }
    bool        SetText(const wchar_t* ptext)
    {
        GASSERT(IsDisplayObject());
        // If false is returned, then the display object may not exist on the stage,
        // or does not contain the 'text' property.
        return pObjectInterface->SetText(Value.pData, ptext, false);
    }
    bool        SetTextHTML(const char* phtml)
    {
        GASSERT(IsDisplayObject());
        // If false is returned, then the display object may not exist on the stage,
        // or does not contain the 'htmlText' property.
        return pObjectInterface->SetText(Value.pData, phtml, true);
    }
    bool        SetTextHTML(const wchar_t* phtml)
    {
        GASSERT(IsDisplayObject());
        // If false is returned, then the display object may not exist on the stage,
        // or does not contain the 'htmlText' property.
        return pObjectInterface->SetText(Value.pData, phtml, true);
    }
    // Returns the raw text in the textField
    // * If textField is displaying HTML, then the displayed text returned
    bool        GetText(GFxValue* pval) const
    {
        GASSERT(IsDisplayObject());
        // If false is returned, then the display object may not exist on the stage,
        // or does not contain the 'text' property.
        return pObjectInterface->GetText(Value.pData, pval, false);
    }
    // Returns the displayed text of a TextField
    // * If textField is displaying HTML, then the HTML code is returned
    bool        GetTextHTML(GFxValue* pval) const
    {
        GASSERT(IsDisplayObject());
        // If false is returned, then the display object may not exist on the stage,
        // or does not contain the 'htmlText' property.
        return pObjectInterface->GetText(Value.pData, pval, true);
    }

    // ----------------------------------------------------------------
    // AS MovieClip support. These methods are only valid for 
    // MovieClips.
    // 
    bool        CreateEmptyMovieClip(GFxValue* pmc, const char* instanceName, SInt depth = -1)
    {
        GASSERT(IsDisplayObject());
        // If false is returned, then the display object may not exist on the stage,
        // or is not a MovieClip display object.
        return pObjectInterface->CreateEmptyMovieClip(Value.pData, pmc, instanceName, depth);
    }
    bool        AttachMovie(GFxValue* pmc, const char* symbolName, const char* instanceName, 
                            SInt depth = -1, const GFxValue* initObj = NULL)
    {
        GASSERT(IsDisplayObject());
        // If false is returned, then the display object may not exist on the stage,
        // or is not a MovieClip display object.
        return pObjectInterface->AttachMovie(Value.pData, pmc, symbolName, instanceName, depth, initObj);
    }
    bool        GotoAndPlay(const char* frame)
    {
        GASSERT(IsDisplayObject());
        // If false is returned, then the display object may not exist on the stage,
        // or is not a MovieClip display object.
        return pObjectInterface->GotoAndPlay(Value.pData, frame, false);
    }
    bool        GotoAndStop(const char* frame)
    {
        GASSERT(IsDisplayObject());
        // If false is returned, then the display object may not exist on the stage,
        // or is not a MovieClip display object.
        return pObjectInterface->GotoAndPlay(Value.pData, frame, true);
    }
    bool        GotoAndPlay(UInt frame)
    {
        // frame is expected to be 1-based, as in Flash
        GASSERT(IsDisplayObject());
        // If false is returned, then the display object may not exist on the stage,
        // or is not a MovieClip display object.
        return pObjectInterface->GotoAndPlay(Value.pData, frame, false);
    }
    bool        GotoAndStop(UInt frame)
    {
        // frame is expected to be 1-based, as in Flash
        GASSERT(IsDisplayObject());
        // If false is returned, then the display object may not exist on the stage,
        // or is not a MovieClip display object.
        return pObjectInterface->GotoAndPlay(Value.pData, frame, true);
    }


protected:
    ObjectInterface*     pObjectInterface;

    union ValueUnion
    {
        Double          NValue;
        bool            BValue;
        const char*     pString;
        const char**    pStringManaged;
        const wchar_t*  pStringW;
        void*           pData;
    };

    ValueType   Type;
    ValueUnion  Value;

    bool        IsManagedValue() const   
    { 
#ifdef GFC_BUILD_DEBUG
        bool ok = ((Type & VTC_ManagedBit) != 0);
        if (ok) GASSERT(pObjectInterface);
        return ok;
#else
        return (Type & VTC_ManagedBit) != 0; 
#endif
    }
    void        ChangeType(ValueType type) 
    {
        if (IsManagedValue()) ReleaseManagedValue();
        Type = type;
    }
    void        AcquireManagedValue(const GFxValue& src)
    {
        GASSERT(src.Value.pData && (src.pObjectInterface != NULL));
        pObjectInterface = src.pObjectInterface;
        pObjectInterface->ObjectAddRef(this, Value.pData);
    }
    void        ReleaseManagedValue()
    {
        GASSERT(Value.pData && (pObjectInterface != NULL));
        pObjectInterface->ObjectRelease(this, Value.pData);
        pObjectInterface = NULL;
    }
};


// *****

#ifndef GFC_NO_FXPLAYER_AS_USERDATA

//
// GFxASUserData allows developers the ability to store custom data with an
// ActionScript VM object and also be notified when it is destroyed. An 
// instance of this handler can be installed with an AS object via the GFxValue 
// interface: GFxValue::SetUserData. 
//
class GFxASUserData : public GRefCountBase<GFxASUserData, GStat_Default_Mem>
{
    friend class GFxValue;

    // Weak ref descriptor.
    GFxValue::ObjectInterface*  pObjectInterface;
    void*                       pData;

public:
    GFxASUserData() : pObjectInterface(NULL), pData(NULL) {}
    virtual ~GFxASUserData() {}

    // Sets the weak ref descriptor. Only used internally.
    void            SetLastObjectValue(GFxValue::ObjectInterface* pobjIfc, void* pdata);
    // Fills in a GFxValue for the last object to which this User data was assigned
    // Will fail if OnDestroy was called
    bool            GetLastObjectValue(GFxValue* value) const;

    virtual void    OnDestroy(GFxMovieView* pmovie, void* pobject) = 0;
};

#endif   // GFC_NO_FXPLAYER_AS_USERDATA


// *****

// GFxFunctionHandler allows the creation of C++ callbacks that behave like
// any AS2 VM function. By using GFxMovie::CreateFunction, users can create
// AS2 function objects that can be assigned to any variable in the VM. This
// function object can be invoked from within the VM, and the invokation will
// be reported via the Call method.
//
// The following example sets a custom function object to a member of an AS2
// VM object located at '_root.obj':
//
//               GFxValue obj;
//               pmovie->GetVariable(&obj, "_root.obj");
//
//              class MyFunc : public GFxFunctionHandler
//               {
//               public:
//                   virtual void Call(const Params& params)
//                   {
//                       GUNUSED(params);
//                       printf("> MY FUNCTION CALLED (Call)\n");
//                   }
//               };
//               GPtr<MyFunc> customFunc = *GHEAP_NEW(GMemory::GetGlobalHeap()) MyFunc();               
//               GFxValue func, func2;               
//               pmovie->CreateFunction(&func, customFunc);
//               obj.SetMember("func", func);
//
// In ActionScript, the customFunc::Call method can be triggered by simply 
// invoking the AS2 function object:
//               
//               obj.func(param1, param2, ...);
//
//
// The Params::pThis points to the calling context (undefined if not applicable).
// The calling context is usually the object that owns the function object. Ex:
//          var obj:Object = {};
//          obj.myfunc();   <--- obj is pThis
//
// The Params::pRetVal is expected to be filled in by the user if a return
// value is expected by the function's semantics.
//
// The Params::pArgsWithThisRef is used for chaining other function objects. 
// A chain can be created by using the following syntax (if MyOtherFuncObj
// is a reference to a valid function object):
//
//          MyOtherFuncObj::Invoke("call", NULL, params.pRetVal, 
//                                  params.pArgsWithThisRef, params.ArgCount + 1)
// 
// Chaining is useful if there is a need to inject a callback at the beginning
// or end of an existing AS2 VM function. 
//
// The Params::pUserData member can be used for custom function object specific
// data, such as an index that can be used as a switch inside a single function
// context object. This data is set by the GFxMovie::CreateFunction.
//
//
// NOTE: If a user defined GFxFunctionHandler instance has a GFxValue member
//       that is holding a reference to an AS2 VM Object (Object, Array, MovieClip,
//       etc.) then it MUST be cleaned up by the user before the movie root dies.
//       This can be achieved by calling GFxValue.SetUndefined(); on the member.
//

class GFxFunctionHandler : public GRefCountBase<GFxFunctionHandler, GStat_Default_Mem>
{
public:
    // Parameters passed in to the callback from the VM
    struct Params 
    {
        GFxValue*           pRetVal;
        GFxMovieView*       pMovie;
        GFxValue*           pThis;
        GFxValue*           pArgsWithThisRef;
        GFxValue*           pArgs;
        UInt                ArgCount;
        void*               pUserData;
    };

    virtual ~GFxFunctionHandler() {}

    virtual void    Call(const Params& params)      = 0;  
};



//
// GFxMovieView and GFxMovie classes provide the client program's interface to
// an instance of a movie (i.e. an independent movie with a separate timeline
// animation state).
//
// GFxMovie interface is separated from GFxMovieView because it can also
// represent nested movie clips (not exposed in this version). Developers
// can use methods in both classes to manipulate the movie.


class GFxMovie : public GRefCountBase<GFxMovie, GFxStatMV_Other_Mem>
{
public:

    // Obtain the movie definition that created us.
    virtual GFxMovieDef*    GetMovieDef() const                     = 0;

    // Delegated for convenience.
    UInt    GetFrameCount() const   { return GetMovieDef()->GetFrameCount(); }
    Float   GetFrameRate() const    { return GetMovieDef()->GetFrameRate(); }

    // Frame counts in this API are 0-Based (unlike ActionScript)
    virtual UInt    GetCurrentFrame() const                         = 0;
    virtual bool    HasLooped() const                               = 0;


    // Moves a playhead to a specified frame. Note that calling this method
    // may cause ActionScript tags attached to frames to be bypassed.
    virtual void    GotoFrame(UInt frameNumber)                     = 0;
    // Returns true if labeled frame is found.
    virtual bool    GotoLabeledFrame(const char* plabel, SInt offset = 0) = 0;

    enum PlayState
    {
        Playing,
        Stopped
    };
    // Changes playback state, allowing animation to be paused
    // and resumed. Setting the state to Stopped will prevent Advance from
    // advancing movie frames.
    virtual void        SetPlayState(PlayState s)                   = 0;
    virtual PlayState   GetPlayState() const                        = 0;


    // Visibility control.
    // Make the movie visible/invisible.  An invisible movie does not
    // advance and does not render.
    virtual void    SetVisible(bool visible)                        = 0;
    virtual bool    GetVisible() const                              = 0;



    // *** Action Script Interface
    
    // Checks for availability of a field/method/nested clip. This can be used to
    // determine if a certain variable can be accessed, or an Invoke can be called
    // without an unexpected failure warning.
    virtual bool    IsAvailable(const char* ppathToVar) const       = 0;

    // String Creation
    // The lifetime of the string created is maintained by the runtime.
    // If the runtime dies, then the GFxValue is invalid. Make sure to release
    // the GFxValue before destroying the movie.
    //
    virtual void    CreateString(GFxValue* pvalue, const char* pstring)             = 0;
    virtual void    CreateStringW(GFxValue* pvalue, const wchar_t* pstring)         = 0;

    // Object Creation
    // The lifetime of the object/array created is maintained by the runtime.
    // If the runtime dies, then the GFxValue is invalid. Make sure to release
    // the GFxValue before destroying the movie.
    //
    // The className parameter of CreateObject can accept fully qualified class
    // names, such as flash.geom.Matrix. Without using the fully qualified name
    // classes in packages will not created by this method.
    virtual void    CreateObject(GFxValue* pvalue, const char* className = NULL, 
                                 const GFxValue* pargs = NULL, UInt nargs = 0)      = 0;
    virtual void    CreateArray(GFxValue* pvalue)                                   = 0;
    // Create a special function object that wraps a C++ function. The function
    // object has the same functionality as any AS2 object, but supports the ability
    // to be invoked in the AS2 VM.
    virtual void    CreateFunction(GFxValue* pvalue, GFxFunctionHandler* pfc, void* puserData = NULL) = 0;


    // Variable Access

    // Set an ActionScript variable within this movie. This can be used to
    // to set the value of text fields, variables, and properties of
    // named nested characters within the movie.

    enum SetVarType
    {
        SV_Normal,      // Sets variable only if target clip is found.
        SV_Sticky,      // Sets variable if target clip is found, otherwise
                        // queues a set until the clip is created at path.
                        // When the target clip, value will be lost.
        SV_Permanent    // Sets variable applied to this and all future
                        // clips at given path.
    };

    // Set a variable identified by a path to a new value.
    // Specifying "sticky" flag will cause the value assignment to be pending until its target 
    // object is created. This allows initialization of objects created in later key-frames.
    // SetVariable will fail if path is invalid.
    virtual bool    SetVariable(const char* ppathToVar, const GFxValue& value, SetVarType setType = SV_Sticky)     = 0;
    
    // Obtain a variable. If *pval has a VT_Convert type, the result will always be filled in with that type,
    // even if GetVariable fails. Otherwise, the value is untouched on failure.
    virtual bool    GetVariable(GFxValue *pval, const char* ppathToVar) const                                      = 0;
    
    // Convenience inline methods for variable access.
  
    // Set a variable identified by a path to a new value, in UTF8.
    GINLINE bool    SetVariable(const char* ppathToVar, const char* pvalue, SetVarType setType = SV_Sticky);
    // Unicode string version of SetVariable for convenience.
    GINLINE bool    SetVariable(const char* ppathToVar, const wchar_t* pvalue, SetVarType setType = SV_Sticky);
    // Sets a variable to a Double value.
    GINLINE bool    SetVariableDouble(const char* ppathToVar, Double value, SetVarType setType = SV_Sticky);
    
    // Gets a variable as a Double; return 0.0 if path was not found.
    GINLINE Double  GetVariableDouble(const char* ppathToVar) const;
    

    // Variable Array Access

    // Type of array being set.
    enum SetArrayType
    {
        SA_Int,     // Array of 'SInt'
        SA_Double,  // Array of 'Double' in memory
        SA_Float,   // Array of 'Float' in memory
        SA_String,  // Array of 'const char*'.      (Deprecated; use SA_Value)
        SA_StringW, // Array of 'const wchar_t*'.   (Deprecated; use SA_Value)
        SA_Value    // Array of GFxValue.
    };

    // Sets array elements in specified range to data items of specified type.
    // If array does not exist, it is created. If an array already exists but
    // does not contain enough items, it is resized appropriately. Under some
    // conditions index > 0 can cause new elements to be inserted into the
    // array. These elements are initialized to 0 for numeric types, empty
    // string "" for strings, and 'undefined' for values. SetVariableArray 
    // can fail if there is not enough memory or if the path is invalid.
    virtual bool    SetVariableArray(SetArrayType type, const char* ppathToVar,
                                     UInt index, const void* pdata, UInt count, SetVarType setType = SV_Sticky) = 0;

    // Sets array's size. If array doesn't exists it will allocate array and resize it. 
    // No data will be assigned to its elements.
    virtual bool    SetVariableArraySize(const char* ppathToVar, UInt count, SetVarType setType = SV_Sticky) = 0;

    // Returns the size of array buffer necessary for GetVariableArray.
    virtual UInt    GetVariableArraySize(const char* ppathToVar)                                                = 0;
    
    // Populates a buffer with results from an array. The pdata pointer must
    // contain enough space for count items. If a variable path is not found
    // or is not an array, 0 will be returned an no data written to buffer.
    // The buffers allocated for string content are temporary within the
    // GFxMovieView, both when SA_String and SA_Value types are used. These
    // buffers may be overwritten after the next call to GetVariableArray;
    // they will also be lost if GFxMovieView dies. This means that users
    // should copy any string data immediately after the call, as access of 
    // it later may cause a crash.
    virtual bool    GetVariableArray(SetArrayType type, const char* ppathToVar,
                                     UInt index, void* pdata, UInt count)                                       = 0;

    // Convenience methods for GFxValue.
    GINLINE bool    SetVariableArray(const char* ppathToVar, UInt index, const GFxValue* pdata, UInt count,
                                     SetVarType setType = SV_Sticky);
    GINLINE bool    GetVariableArray(const char* ppathToVar, UInt index, GFxValue* pdata, UInt count);    


    // ActionScript Invoke

    // ActionScript method invoke interface. There are two different
    // approaches to passing data into Invoke:
    //  1) Using an array of GFxValue objects (slightly more efficient).
    //  2) Using a format string followed by a variable argument list.
    //
    // Return values are stored in GFxValue object, with potential conversion
    // to UTF8 strings or other types.  If a string is returned, it is stored
    // within a temporary  buffer in GFxMovieView and should be copied
    // immediately (if required by user). The value in a temporary buffer may
    // be overwritten by the next call to GetVariable or Invoke.
    //
    // Use case (1):
    //
    //   GFxValue args[3], result;
    //   args[0].SetNumber(i);
    //   args[1].SetString("test");
    //   args[2].SetNumber(3.5);
    //
    //   pMovie->Invoke("path.to.methodName", &result, args, 3);
    //
    // Use case (2):
    //
    //   pmovie->Invoke("path.to.methodName", "%d, %s, %f", i, "test", 3.5);
    //
    // The printf style format string describes arguments that follow in the
    // same  order; it can include commas and whitespace characters that are
    // ignored. Valid format arguments are:
    //
    //  %u      - undefined value, no argument required
    //  %d      - integer argument
    //  %s      - null-terminated char* string argument
    //  %ls     - null-terminated wchar_t* string argument
    //  %f      - double argument
    //  %hf     - float argument, only in InvokeArgs
    //
    // pmethodName is a name of the method to call, with a possible namespace
    // syntax to access methods in the nested objects and movie clips.
    //
   
    // *** Invokes:

    // presult can be null if no return value is desired.
    virtual bool       Invoke(const char* pmethodName, GFxValue *presult, const GFxValue* pargs, UInt numArgs)     = 0;
    virtual bool       Invoke(const char* pmethodName, GFxValue *presult, const char* pargFmt, ...)                = 0;
    virtual bool       InvokeArgs(const char* pmethodName, GFxValue *presult, const char* pargFmt, va_list args)   = 0;
   
    static Float GSTDCALL GetRenderPixelScale() { return 20.f; }
};


// *** Inline Implementation - GFxMovie

bool    GFxMovie::SetVariable(const char* ppathToVar, const char* pvalue, SetVarType setType)
{
    GFxValue v(pvalue);
    return SetVariable(ppathToVar, v, setType);
}
bool    GFxMovie::SetVariable(const char* ppathToVar, const wchar_t* pvalue, SetVarType setType)
{
    GFxValue v(pvalue);
    return SetVariable(ppathToVar, v, setType);
}
bool    GFxMovie::SetVariableDouble(const char* ppathToVar, Double value, SetVarType setType)
{
    GFxValue v(value);
    return SetVariable(ppathToVar, v, setType);
}
Double   GFxMovie::GetVariableDouble(const char* ppathToVar) const
{
    GFxValue v(GFxValue::VT_ConvertNumber);
    GetVariable(&v, ppathToVar);
    if (v.GetType() == GFxValue::VT_Number)
        return v.GetNumber();
    return 0.0;    
}

// Convenience methods for GFxValue.
bool    GFxMovie::SetVariableArray(const char* ppathToVar,
                                 UInt index, const GFxValue* pdata, UInt count, SetVarType setType)
{
    return SetVariableArray(SA_Value, ppathToVar, index, pdata, count, setType);
}
bool    GFxMovie::GetVariableArray(const char* ppathToVar, UInt index, GFxValue* pdata, UInt count)
{
    return GetVariableArray(SA_Value, ppathToVar, index, pdata, count);
}

// *** End Inline Implementation - GFxMovie


// *** GFxExternalInterface State

class GFxExternalInterface : public GFxState
{
public:
    GFxExternalInterface() : GFxState(State_ExternalInterface) { }

    virtual ~GFxExternalInterface() {}

    // A callback for ExternalInterface.call. 'methodName' may be NULL, if ExternalInterface is
    // called without parameter. 'args' may be NULL, if argCount is 0. The value, returned 
    // by this callback will be returned by 'ExternalInterface.call' method.
    // To return a value, use pmovieView->SetExternalInterfaceRetVal method.
    virtual void Callback(GFxMovieView* pmovieView, const char* methodName, const GFxValue* args, UInt argCount) = 0;
};


// Sets the external interface used.
void GFxStateBag::SetExternalInterface(GFxExternalInterface* p)
{
    SetState(GFxState::State_ExternalInterface, p);
}
GPtr<GFxExternalInterface> GFxStateBag::GetExternalInterface() const
{
    return *(GFxExternalInterface*) GetStateAddRef(GFxState::State_ExternalInterface);
}




// GFxMovieView is created by the GFxMovieDef::CreateInstance function and is a
// primary user interface to a movie instance. After being created,
// GFxMovieView is normally configured by setting the renderer, viewport, and
// fscommand callback, if necessary.
//
// After that the movie is ready for playback. Its animation state can be
// advanced by calling Advance() and it can be rendered at by calling Display().
// Both of those methods are inherited from GFxMovie.

class GFxMovieView : public GFxMovie, public GFxStateBag
{
protected:    
    friend class GFxCharacter;
public:

    // Renderer configuration is now in GFxStateBag.

    // *** Viewport and Scaling configuration

    // Sets the render-target surface viewport to which the movie is scaled;
    // coordinates are specified in pixels. If not specified, the default
    // viewport is (0,0, MovieWidth, MovieHeight).
    virtual void        SetViewport(const GViewport& viewDesc)               = 0;
    virtual void        GetViewport(GViewport *pviewDesc) const              = 0;

    // Viewport inline helper; mostly provided for compatibility.
    void        SetViewport(SInt bufw, SInt bufh, SInt left, SInt top, SInt w, SInt h, UInt flags = 0)
    {
        GViewport desc(bufw, bufh, left, top, w, h, flags);
        SetViewport(desc);
    }
    // Scale mode
    enum ScaleModeType
    {
        SM_NoScale,
        SM_ShowAll,
        SM_ExactFit,
        SM_NoBorder
    };
    virtual void            SetViewScaleMode(ScaleModeType) = 0;
    virtual ScaleModeType   GetViewScaleMode() const        = 0;
    enum AlignType
    {
        Align_Center,
        Align_TopCenter,
        Align_BottomCenter,
        Align_CenterLeft,
        Align_CenterRight,
        Align_TopLeft,
        Align_TopRight,
        Align_BottomLeft,
        Align_BottomRight
    };
    virtual void            SetViewAlignment(AlignType) = 0;
    virtual AlignType       GetViewAlignment() const    = 0;

    // Returns currently visible frame rectangle, in pixels
    virtual GRectF          GetVisibleFrameRect() const = 0;

    // Sets the perspective matrix for a 3D movieclip
    virtual void  SetPerspective3D(const GMatrix3D &projMatIn) = 0;

    // Sets the view matrix for a 3D movieclip
    virtual void  SetView3D(const GMatrix3D &viewMatIn) = 0;

    // Sets/gets safe rectangle, in pixels. GFxMovieView just stores
    // this rectangle, it is user's responsibility to update it. It is 
    // possible to retrieve it through ActionScript - Stage.safeRect 
    // (extensions should be turned on).
    virtual GRectF          GetSafeRect() const             = 0;
    virtual void            SetSafeRect(const GRectF& rect) = 0;
/*
    // *** Execution State
    virtual void           SetSoundPlayer(GSoundRenderer* ps)                     = 0;
    virtual GSoundRenderer*  GetSoundPlayer() const                               = 0;
*/
    virtual void        Restart()                                       = 0;

    // Advance a movie based on the time that has passed since th last
    // advance call, in seconds. In general, the time interval should
    // be <= 1 / movie FrameRate. If it is not, multiple frames will be
    // advanced only of frameCatchUp == 1.
    // Returns time remaining till next advance must be called, in seconds.
    virtual Float       Advance(Float deltaT, UInt frameCatchUpCount = 2) = 0;

    // Outputs the movie view to its renderer.
    virtual void        Display()                                       = 0;

    // Process parts of the display tree that use render targets, for systems
    // with limited render target support.
    virtual void        DisplayPrePass()                                = 0;

    // pause/unpause the movie
    virtual void        SetPause(bool pause)                      = 0;
    virtual bool        IsPaused() const                      = 0;


    // *** Background color interface
    
    virtual void    SetBackgroundColor(const GColor BgColor)        = 0;

    // Sets the background color alpha applied to the movie clip.
    // Set to 0.0f if you do not want the movie to render its background
    // (ex. if you are displaying a hud on top of the game scene).
    // The default value is 1.0f, indication full opacity.
    virtual void    SetBackgroundAlpha(Float alpha)                 = 0;
    virtual Float   GetBackgroundAlpha() const                      = 0;

    // *** Input

    // Event notification interface.
    // Users can call this handler to inform GFxPlayer of mouse and keyboard
    // input, as well as stage and other events.
    enum HE_ReturnValueType
    {
        HE_NotHandled           = 0,
        HE_Handled              = 1,
        HE_NoDefaultAction      = 2,
        HE_Completed            = (HE_Handled | HE_NoDefaultAction)
    };
    virtual UInt        HandleEvent(const GFxEvent &event)          = 0;

    // State interface. This interface can be used to inform about the
    // current mouse/keyboard state. For simple cases, it can be used
    // instead of the event interface; changes in state will automatically
    // generate internal events.

    // Retrieve mouse state, such as coordinates and buttons state.
    // "buttons" is a bit mask, where bit 0 specifies left button, 
    // bit 1 - right, bit 2 - middle.
    virtual void        GetMouseState(UInt mouseIndex, Float* x, Float* y, UInt* buttons) =0;

    // Notifies of mouse state; should be called at regular intervals.
    // The mouse coordinates need to be in the coordinate system of the
    // viewport set by SetViewport.
    // Note, this function is deprecated, use HandleEvent instead.
    virtual void        NotifyMouseState(Float x, Float y, UInt buttons, UInt mouseIndex = 0) = 0;

    enum HitTestType
    {
        HitTest_Bounds = 0,
        HitTest_Shapes = 1,
        HitTest_ButtonEvents = 2,
        HitTest_ShapesNoInvisible = 3
    };
    // Performs a hit test on the movie view and returns whether or not
    // the passed point is inside the valid area.
    // HitTest3D returns the world space intersection point.
    virtual bool        HitTest(Float x, Float y, HitTestType testCond = HitTest_Shapes, UInt controllerIdx = 0) = 0;
    virtual bool        HitTest3D(GPoint3F *ptout, Float x, Float y, UInt controllerIdx = 0) = 0;

    // *** ExternalInterface / User value handling

    // Used to set the return value from GFxExternalInterface::Callback function.
    virtual void        SetExternalInterfaceRetVal(const GFxValue&) = 0;

    // Set and get user data handle; useful for the FsCommand handler.
    virtual void*       GetUserData() const                         = 0;
    virtual void        SetUserData(void*)                          = 0;


    // Installs display callbacks that can be used for client rendering. The callback
    // is called after rendering the object it's attached to.
    // Attach NULL to disable the callback. Returns 1 for success, 0 for failure
    //  (object path not found or not of the right type).
    virtual bool        AttachDisplayCallback(const char* ppathToObject,
                            void (GCDECL *callback)(void* puser), void* puser) = 0;

    // Returns true, if movie has focus.
    virtual bool        IsMovieFocused() const = 0;

    // Returns and optionally resets the "dirty flag" that indicates 
    // whether anything was changed on the stage (and need to be 
    // re-rendered) or not.
    virtual bool        GetDirtyFlag(bool doReset = true) = 0;

    // Sets/gets number of supported mice. If sets to 0 then mouse support 
    // is disabled. Note, supporting mouse
    // incurs extra processing overhead during Advance due to the execution
    // of extra hit-testing logic when objects move; disabling it will
    // cause mouse events and NotifyMouseState to be ignored.
    // The default state is controlled by the GFC_MOUSE_SUPPORT_ENABLED
    // configuration option and is disabled on consoles (all, but Wii) 
    // and set to 1 on PCs.
    virtual void        SetMouseCursorCount(UInt n) = 0;
    virtual UInt        GetMouseCursorCount() const = 0;

    virtual void        SetControllerCount(UInt n) = 0;
    virtual UInt        GetControllerCount() const = 0;

    // Obtains statistics for the movie view and performs a reset
    // for tracked counters.
    //   - If reset is true, it is done after the stats update.
    //   - If pbag is null, only reset can take place.    
    virtual void        GetStats(GStatBag* pbag, bool reset = true)   = 0;

    virtual GMemoryHeap* GetHeap() const                              = 0;    

    // Forces to run garbage collection, if it is enabled. Does nothing otherwise.
    virtual void        ForceCollectGarbage() = 0;

    // Redeclaration from base class for compiler visibility
    virtual bool    GetVariable(GFxValue *pval, const char* ppathToVar) const                                      = 0;
    virtual bool       Invoke(const char* pmethodName, GFxValue *presult, const GFxValue* pargs, UInt numArgs)     = 0;
    virtual bool       Invoke(const char* pmethodName, GFxValue *presult, const char* pargFmt, ...)                = 0;
    virtual bool       InvokeArgs(const char* pmethodName, GFxValue *presult, const char* pargFmt, va_list args)   = 0;

    // (3.1: Deprecated. Use GFxMovie::GetVariable(GFxValue*, ..) instead.)
    // Get the value of an ActionScript variable. Stores a value in a temporary GFxMovie buffer
    // in UTF8 format. The buffer is overwritten by the next call.
    const char*     GetVariable(const char* ppathToVar) const;
    const wchar_t*  GetVariableStringW(const char* ppathToVar) const;

    // (3.1: Deprecated. Use GFxMovie::Invoke(.., GFxValue*, ..) instead.)
    // String - returning versions of Invoke.
    // These versions of Invoke return 0 if Invoke failed and string-converted value otherwise.
    const char* Invoke(const char* pmethodName, const GFxValue* pargs, UInt numArgs);
    const char* Invoke(const char* pmethodName, const char* pargFmt, ...);
    const char* InvokeArgs(const char* pmethodName, const char* pargFmt, va_list args);

    // Translates the point in Flash coordinates to screen 
    // (window) coordinates. These methods takes into account the world matrix
    // of root, the viewport matrix and the user matrix from the renderer. 
    // Source coordinates should be in _root coordinate space, in pixels.
    virtual GPointF TranslateToScreen(const GPointF& p, GRenderer::Matrix userMatrix = GRenderer::Matrix::Identity) =0;

    // Translates the rectangle in Flash coordinates to screen 
    // (window) coordinates. These methods takes into account the world matrix
    // of root, the viewport matrix and the user matrix from the renderer. 
    // Source coordinates should be in _root coordinate space, in pixels.
    virtual GRectF  TranslateToScreen(const GRectF& p, GRenderer::Matrix userMatrix = GRenderer::Matrix::Identity) =0;

    // Translates the point in the character's coordinate space to the point on screen (window).
    // pathToCharacter - path to a movieclip, textfield or button, i.e. "_root.hud.mc";
    // pt is in pixels, in coordinate space of the character, specified by the pathToCharacter
    // This method writes new point coordinates into presPt (value is in pixels of screen).
    // Returns false of character is not found; true otherwise.
    virtual bool    TranslateLocalToScreen(const char* pathToCharacter, 
        const GPointF& pt, 
        GPointF* presPt, 
        GRenderer::Matrix userMatrix = GRenderer::Matrix::Identity) =0;

    // associate a focus group with a controller.
    virtual bool    SetControllerFocusGroup(UInt controllerIdx, UInt focusGroupIndex) =0;

    // returns focus group associated with a controller
    virtual UInt    GetControllerFocusGroup(UInt controllerIdx) const =0;

    // Accessor to memory context
    virtual GFxMovieDef::MemoryContext* GetMemoryContext() const = 0;

    // Release method for custom behavior
    virtual void    Release() = 0;   
};


// ***** Base class for objects defined in external libraries (such as XML)

class GFxExternalLibPtr
{
    // No special implementation. This is simply an empty base class.

protected:
    // Pointer to the movie root that holds this pointer
    // Used by the lib ptr object to unregistered itself
    GFxMovieRoot*   pOwner;

public:
    GFxExternalLibPtr(GFxMovieRoot* pmovieroot) 
        : pOwner(pmovieroot) {}
    virtual ~GFxExternalLibPtr() {}
};


// For compatibility with previous GFx versions
typedef GFxFunctionHandler GFxFunctionContext;


#endif // INC_GFxPlayer_H
