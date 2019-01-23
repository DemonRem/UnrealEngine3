/**********************************************************************

Filename    :   GFxLoader.h
Content     :   Public SWF loading interface for GFxPlayer
Created     :   June 21, 2005
Authors     :   Michael Antonov

Notes       :   Redesigned to use inherited state objects for GFx 2.0.

Copyright   :   (c) 2005-2007 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXLOADER_H
#define INC_GFXLOADER_H

#include "GTypes.h"
#include "GRefCount.h"
#include "GArray.h"
#include "GJPEGUtil.h"
#include "GStringHash.h"
#include "GFile.h"
#include "GSystem.h"

#include "GFxResource.h"
#include "GFxPlayerStats.h"

#include "GFxWWHelper.h"

// ***** Declared Classes
struct  GFxMovieInfo;
struct  GFxTagInfo;
class   GFxStateBag;
class   GFxLoader;
class   GFxTranslator;
class   GFxGradientParams;

struct  GFxExporterInfo;
struct  GFxImageFileInfo;
class   GFxImageCreateInfo;

// ***** External Classes
class   GFile;
class   GFxMovieDef;
class   GFxMovieDataDef;
class   GImage;
class   GImageInfoBase;
class   GRenderer;
class   GFxSpecialKeysState;
class   GFxTestStream;

// Externally declared states:
class   GFxRenderConfig;
class   GFxRenderStats;
class   GFxLog;
class   GFxExternalInterface;
class   GFxImportVisitor;
class   GFxTaskManager;
class   GFxFontLib;
class   GFxIMEManager;
class   GFxXMLSupportBase;
class   GFxJpegSupportBase;
class   GFxZlibSupportBase;
class   GFxFontCompactorParams;
class   GFxPNGSupportBase;
class   GFxSharedObjectManagerBase;

#ifndef GFC_NO_SOUND
class   GSoundRenderer;
class   GFxAudioBase;
#endif
#ifndef GFC_NO_VIDEO
class   GFxVideoBase;
#endif

// ***** GFxLoader States

// GFxState - a base class for all Loader/Movie states in GFx. These states are
// accessible through the  GShadedState::SetState/GetStateAddRef functions.

class GFxState :  public GRefCountBase<GFxState, GStat_Default_Mem>
{
public:
    enum StateType
    {
        State_None,

        // *** Instance - related states.
        State_RenderConfig,
        State_RenderStats,
        State_Translator,
        State_Log,
        State_ImageLoader,
        State_ActionControl,    // Verbose Action
        State_UserEventHandler,
        State_FSCommandHandler,
        State_ExternalInterface,

        // *** Loading - related states.
        State_FileOpener,
        State_URLBuilder,
        State_ImageCreator,
        State_ParseControl,     // Verbose Parse
        State_ProgressHandler,
        State_ImportVisitor,
//        State_ImageVisitor,
        State_MeshCacheManager,
        State_FontPackParams,
        State_FontCacheManager,
        State_FontLib,
        State_FontProvider,
        State_FontMap,
        State_GradientParams,
        State_TaskManager,
        State_TextClipboard,
        State_TextKeyMap,
        State_PreprocessParams,
        State_IMEManager,
        State_XMLSupport,
        State_JpegSupport,
        State_ZlibSupport,
        State_FontCompactorParams,
        State_ImagePackerParams,
        State_PNGSupport,
        State_Audio,
        State_Video,
        State_TestStream,
        State_SharedObject,
        State_LocSupport
    };

protected:        
    StateType       SType;
public:

    // State Ref-Counting and interaction needs to be thread-safe.
    GFxState(StateType st = State_None) : SType(st) { }
    virtual ~GFxState() { }

    // Use inline to make state lookups more efficient.
    inline StateType   GetStateType() const { return SType;  }
};


// ***** GFxMeshCacheManager

// Internal state used in Display()

class GFxMeshCacheManager : public GFxState
{
    GMemoryHeap*            pHeap;
    class GFxRenderGen*     pRenderGen;
    class GFxMeshCache*     pMeshCache;
    class GRendererEventHandler* pRenEventHandler;

public:
    GFxMeshCacheManager(bool debugHeap = false);
    ~GFxMeshCacheManager();

    // Get and set the memory limit for tessellation, EdgeAA, stroker, 
    // and so on. As soon as this limit is exceeded the memory is released.
    // The RenderGen classes reuse memory and work more efficiently without
    // the necessity to allocate memory. But if the shape is complex the 
    // RenderGen classes can consume too much memory.
    UPInt GetRenderGenMemLimit() const;
    void  SetRenderGenMemLimit(UPInt lim);

    // Get and set the main cache memory limit. Typically 4MB is enough for 
    // most of the practical cases.
    UPInt GetMeshCacheMemLimit() const;
    void  SetMeshCacheMemLimit(UPInt lim);

    // Get and set the number of frames when the cache is locked. The meshes 
    // added during these frames will not be extruded. In most cases it's 1, 
    // but can be 2 for certain rendering back-ends, like Unreal.
    UInt  GetNumLockedFrames() const;
    void  SetNumLockedFrames(UInt num);

    // Enable/disable caching of all meshes for shapes with morphing. 
    // If enabled morphing works more efficiently, but may result in more 
    // intensive cache rotation.
    void  EnableCacheAllMorph     (bool enable);
    // Enable/disable caching of all meshes for shapes with Scale9Grid. 
    // If enabled Scale9Grid works more efficiently, but may result in more 
    // intensive cache rotation.
    void  EnableCacheAllScale9Grid(bool enable);
    bool  IsCacheAllMorphEnabled     () const;
    bool  IsCacheAllScale9GridEnabled() const;

    // Returns the total number of calls to the tessellator since the starting
    // moment. This number always increases. Can be used to estimate the 
    // intensity of tessellation, and indirectly - the efficiency of the cache.
    UInt  GetNumTessellatedShapes() const;
    // Returns the total number of calls to the tessellator since the starting
    // moment. This number always increases. Can be used to estimate the 
    // intensity of tessellation, and indirectly - the efficiency of the cache.
    UInt  GetMeshThrashing() const;

    // Release all of the cached data, including both cached mesh
    // vertices and tessellator/edgeAA engine data (RenderGen).
    // It may be useful to call this after unloading a file.
    void  ClearCache();

    class GFxRenderGen* GetRenderGen() { return pRenderGen; }
    class GFxMeshCache* GetMeshCache() { return pMeshCache; }

    // Returns the total number of GFxMeshSets in the cache.
    UInt        GetNumMeshes() const;
    // Returns the total number of bytes allocated in the cache.
    UPInt       GetNumBytes()  const;
    UInt        GetNumStrokes() const;

    GMemoryHeap* GetHeap() const { return pHeap; }

    // When the shape is destroyed it's impossible to remove the respective 
    // meshes, because it may happen in a different thread. So, the meshes are
    // added to a special kill list which must be cleaned in the end of Display
    // function. Call this function after the frame is complete.
    void ClearKillList();
    // Move the meshes created during the frame to the main queue.
    // Call this function after the frame is complete. It will automatically
    // take care of the NumLockedFrames.
    void AcquireFrameQueue();

    // Calls ClearKillList(), AcquireFrameQueue, and clears RenderGen
    // on exceeding RenderGen Memory limit.
    void EndDisplay();

    class GRendererEventHandler*    GetEventHandler();
    bool                            HasEventHandler() const;

};


// ***** GFxTranslator

// GFxTranslator class is used for international language translation of dynamic
// text fields. Translation is performed through the Translate virtual function
// which is expected to fill in a buffer with a translated string.

class GFxTranslator : public GFxState
{
public:
    enum TranslateCaps
    {
        // Specifies that Translate key can include Flash-HTML tags. If not specified
        // translate will only receive stripped text content (default).
        Cap_ReceiveHtml = 0x1,
        // Forces to strip all trailing new-line symbols before the text is passed
        // to Translate.
        Cap_StripTrailingNewLines = 0x2
    };
    // Handling custom word-wrapping. To turn it on you need to enable custom word-wrapping
    // in GetCaps by adding Cap_CustomWordWrapping bit.
    enum WordWrappingTypes
    {
        WWT_Default       = 0,   // OnWordWrapping will not be invoked
        WWT_Asian         = GFxWWHelper::WWT_Asian, // mostly Chinese
        WWT_Prohibition   = GFxWWHelper::WWT_Prohibition, // Japanese prohibition rule
        WWT_NoHangulWrap  = GFxWWHelper::WWT_NoHangulWrap, // Korean-specific rule
        WWT_Hyphenation   = (GFxWWHelper::WWT_Last<<1), // very simple hyphenation; for demo only
        WWT_Custom        = 0x80 // user defined word-wrapping.
    };
    UInt                   WWMode; // combination of WWT_- flags

    GFxTranslator() : GFxState(State_Translator), WWMode(WWT_Default) { }
    explicit GFxTranslator(UInt wwMode) : GFxState(State_Translator), WWMode(wwMode) { }
  
    // Override this function to specify whether Translate can receive and/or return HTML.
    virtual UInt GetCaps() const         { return 0; }
    inline  bool CanReceiveHtml() const  { return (GetCaps() & Cap_ReceiveHtml) != 0; }
    inline  bool NeedStripNewLines() const { return (GetCaps() & Cap_StripTrailingNewLines) != 0; }
    inline  bool HandlesCustomWordWrapping() const { return (WWMode != WWT_Default); }

    // This class provides data to and from the 'Translate' method, such as original 
    // text, name of textfield's instance, resulting translated text.
    class TranslateInfo
    {
    protected:
        const wchar_t*      pKey;
        GFxWStringBuffer*   pResult;
        const char*         pInstanceName;
        enum
        {
            Flag_Translated = 0x01,
            Flag_ResultHtml = 0x02,
            Flag_SourceHtml = 0x04
        };
        UInt8               Flags;
    public:
        TranslateInfo():pKey(NULL), pResult(NULL), pInstanceName(NULL), Flags(0) {}

        // *Input methods.
        // Returns 'key' string - the original text value of the textfield being translated.
        const wchar_t*  GetKey() const          { return pKey; }

        // Returns true, if key (returned by 'GetKey()') is HTML. Note, this
        // may happen only if flag GFxTranslator::Cap_ReceiveHtml is set in value
        // returned by GFxTranslator::GetCaps(). If this flag is not set and textfield 
        // contains HTML then HTML tags will be stripped out and IsKeyHtml() will return
        // 'false'.
        bool            IsKeyHtml() const { return (Flags & Flag_SourceHtml) != 0; }

        // Returns the instance name of the textfield being translated.
        const char*     GetInstanceName() const { return pInstanceName; }
        
        // *Output methods.
        // Sets translated sting as a plain text. Two variants are supported - wide-character 
        // and UTF-8. Length is optional; null-terminated string should be used if it is omitted.
        // 'resultLen' for wide-char variant indicates number of characters (not bytes) in the string.
        void            SetResult(const wchar_t* presultText, UPInt resultLen = GFC_MAX_UPINT);
        // 'resultLen' for UTF-8 variant indicates number of bytes (not characters) in the string.
        void            SetResult(const char* presultTextUTF8, UPInt resultLen = GFC_MAX_UPINT);

        // Sets translated sting as a HTML text. HTML will be parsed before applying to the textfield.
        // Two variants are supported - wide-character and UTF-8. Length is optional; 
        // null-terminated string should be used if it is omitted.
        // 'resultLen' for wide-char variant indicates number of characters (not bytes) in the string.
        void            SetResultHtml(const wchar_t* presultHtml, UPInt resultLen = GFC_MAX_UPINT)
        {
            SetResult(presultHtml, resultLen);
            Flags |= Flag_ResultHtml;
        }
        // 'resultLen' for UTF-8 variant indicates number of bytes (not characters) in the string.
        void            SetResultHtml(const char* presultHtmlUTF8, UPInt resultLen = GFC_MAX_UPINT)
        {
            SetResult(presultHtmlUTF8, resultLen);
            Flags |= Flag_ResultHtml;
        }
    };

    // Performs lookup of 'ptranslateInfo->GetKey()' string for language translation, 
    // filling in the destination string buffer by calling 'SetResult' or 'SetResultHtml' 
    // method. 'ptranslateInfo' is guaranteed to be not null.
    // If neither 'SetResult' nor 'SetResultHtml' is called then original text will not
    // be changed.
    virtual void Translate(TranslateInfo* ptranslateInfo) { GUNUSED(ptranslateInfo); }

    // This struct provides information of the line text to be formatted like the length 
    // of the text, text position etc. It is mainly used by OnWordWrapping 
    // to control word wrapping of the text.
    struct LineFormatDesc
    {
        const wchar_t*  pParaText;              // [in] paragraph text
        UPInt           ParaTextLen;            // [in] length, in chars
        const Float*    pWidths;                // [in] array of line widths before char at corresponding index, in pixels, size = NumCharsInLine + 1
        UPInt           LineStartPos;           // [in] text position in paragraph of first char in line
        UPInt           NumCharsInLine;         // [in] count of chars currently in the line        
        Float           VisibleRectWidth;       // [in] width of client rect, in pixels
        Float           CurrentLineWidth;       // [in] current line width, in pixels
        Float           LineWidthBeforeWordWrap;// [in] line width till ProposedWordWrapPoint, in pixels
        Float           DashSymbolWidth;        // [in] may be used to calculate hyphenation, in pixels
        enum                                    
        {
            Align_Left      = 0,
            Align_Right     = 1,
            Align_Center    = 2,
            Align_Justify   = 3
        };
        UInt8           Alignment;              // [in] alignment of the line

        UPInt           ProposedWordWrapPoint;  // [in,out] text index of proposed word wrap pos,
                                                //          callback may change it to move wordwrap point
        bool            UseHyphenation;         // [out]    callback may set it to indicate to use hyphenation
    };
    // A virtual method, a callback, which is invoked once a necessity of 
    // word-wrapping for any text field is determined. This method is invoked only
    // if custom word-wrapping is turned on by using the GFxTranslator(wwMode) constructor. 
    virtual bool OnWordWrapping(LineFormatDesc* pdesc);
};



// ***** GFxMovieImageLoader

// Callback used by loadMovie() with user-image protocols, such as 'img://' (bilinear image) 
// or 'imgps://' (point sampled image) is called. The user should populate an image
// and return a pointer to it; the image will be displayed inside of the target movie clip.

class GFxImageLoader : public GFxState
{
public:
    GFxImageLoader() : GFxState(State_ImageLoader) { }

    // Image loading virtual function. purl should be encoded as UTF-8 to support
    // international file names.
    virtual GImageInfoBase*     LoadImage(const char* purl) = 0;
};


// ***** GFxActionControl

// Controls verbosity and runtime behavior of ActionScript.
class GFxActionControl : public GFxState
{
protected:
    UInt            ActionFlags;

public:

    // ActionScript execution control flags.
    enum ActionControlFlags
    {
        Action_Verbose              = 0x01,
        Action_ErrorSuppress        = 0x02,
        Action_LogRootFilenames     = 0x04, //Display filename for root movie 
        Action_LogChildFilenames    = 0x08, //Display filename for child movie
        Action_LogAllFilenames      = 0x04 | 0x08, // Display filenames for all movies
        Action_LongFilenames        = 0x10  //Display full path
    };

    GFxActionControl(UInt actionFlags = Action_LogChildFilenames)
        : GFxState(State_ActionControl),  ActionFlags(actionFlags)
    { }       
    
    inline void    SetActionFlags(UInt actionFlags)   { ActionFlags = actionFlags; }
    inline UInt    GetActionFlags() const             { return ActionFlags; }

    // Enables verbose output to log when executing ActionScript. Useful
    // for debugging, 0 by default. Not available if GFx is compiled
    // with the GFC_NO_FXPLAYER_VERBOSE_ACTION option defined.
    inline void    SetVerboseAction(bool verboseAction)
    {
        ActionFlags = (ActionFlags & ~(Action_Verbose)) | (verboseAction ? Action_Verbose : 0);
    }
    // Turn off/on log for ActionScript errors..
    inline void    SetActionErrorSuppress(bool suppressErrors)
    {
        ActionFlags = (ActionFlags & ~(Action_ErrorSuppress)) | (suppressErrors ? Action_ErrorSuppress : 0);
    } 
    // Print filename with ActionScript messages
    inline void    SetLogRootFilenames(bool logRootFilenames)
    {
        ActionFlags = (ActionFlags & ~(Action_LogRootFilenames)) | (logRootFilenames ? Action_LogRootFilenames : 0);
    } 

    inline void    SetLogChildFilenames(bool logChildFilenames)
    {
        ActionFlags = (ActionFlags & ~(Action_LogChildFilenames)) | (logChildFilenames ? Action_LogChildFilenames : 0);
    } 

    inline void    SetLogAllFilenames(bool logAllFilenames)
    {
        ActionFlags = (ActionFlags & ~(Action_LogAllFilenames)) | (logAllFilenames ? Action_LogAllFilenames : 0);
    } 

    inline void    SetLongFilenames(bool longFilenames)
    {
        ActionFlags = (ActionFlags & ~(Action_LongFilenames)) | (longFilenames ? Action_LongFilenames : 0);
    } 

};



// ***** GFxUserEventHandler

// User-insallable interface to handle events fired FROM the player.
// These events include requests to show/hide mouse, etc.

class GFxUserEventHandler : public GFxState
{
public:
    
    GFxUserEventHandler() : GFxState(State_UserEventHandler) { }

    virtual void HandleEvent(class GFxMovieView* pmovie, const class GFxEvent& event) = 0;
};

// ***** GFxTestStream
// Handles saving and reproducing events (timers, random etc.)

class GFxTestStream : public GFxState
{
public:
    GFxTestStream ()  : GFxState(State_TestStream) {}
    virtual bool GetParameter (const char* parameter, GString* value) = 0;
    virtual bool SetParameter (const char* parameter, const char* value) = 0;
    enum
    {
        Record,
        Play
    } TestStatus;
};


// ***** GFxFSCommandHandler

// ActionScript embedded in a movie can use the built-in fscommand() method
// to send data back to the host application. This method can be used to
// inform the application about button presses, menu selections and other
// UI events. To receive this data developer need to register a handler
// with SetFSCommandCallback.

// If specified, this handler gets called when ActionScript executes
// the fscommand() statement. The handler receives the GFxMovieView*
// interface for the movie that the script is embedded in and two string
// arguments passed by the script to fscommand().


class GFxFSCommandHandler : public GFxState
{
public:
    GFxFSCommandHandler() : GFxState(State_FSCommandHandler) { }

    virtual void Callback(GFxMovieView* pmovie, const char* pcommand, const char* parg) = 0;
};


// ***** GFxFileOpenerBase

// A callback interface that is used for opening files.
// Must return either an open file or 0 value for failure.
// The caller of the function is responsible for Releasing
// the GFile interface.

class GFxFileOpenerBase : public GFxState
{
public:

    GFxFileOpenerBase();

    // Override to opens a file using user-defined function and/or GFile class.
    // The default implementation uses buffer-wrapped GSysFile.
    // The 'purl' should be encoded as UTF-8 to support international file names.
    virtual GFile* OpenFile(const char* purl, 
        SInt flags = GFileConstants::Open_Read|GFileConstants::Open_Buffered, 
        SInt mode = GFileConstants::Mode_ReadWrite) = 0;

    // Returns last modified date/time required for file change detection.
    // Can be implemented to return 0 if no change detection is desired.
    // The 'purl' should be encoded as UTF-8 to support international file names.
    virtual SInt64      GetFileModifyTime(const char* purl) = 0;

    // Open file with customizable log, by relying on OpenFile.
    // If not null, log will receive error messages on failure.
    // The 'purl' should be encoded as UTF-8 to support international file names.
    virtual GFile* OpenFileEx(const char* purl, class GFxLog *plog, 
        SInt flags = GFileConstants::Open_Read|GFileConstants::Open_Buffered, 
        SInt mode = GFileConstants::Mode_ReadWrite) = 0;    
};

// ***** GFxFileOpener
// Default implementation of GFxFileOpenerBase interface.

class GFxFileOpener : public GFxFileOpenerBase
{
public:

    // Override to opens a file using user-defined function and/or GFile class.
    // The default implementation uses buffer-wrapped GSysFile, but only
    // if GFC_USE_SYSFILE is defined.
    // The 'purl' should be encoded as UTF-8 to support international file names.
    virtual GFile* OpenFile(const char* purl, 
        SInt flags = GFileConstants::Open_Read|GFileConstants::Open_Buffered, 
        SInt mode = GFileConstants::Mode_ReadWrite);

    // Returns last modified date/time required for file change detection.
    // Can be implemented to return 0 if no change detection is desired.
    // Default implementation checks file time if GFC_USE_SYSFILE is defined.
    // The 'purl' should be encoded as UTF-8 to support international file names.
    virtual SInt64 GetFileModifyTime(const char* purl);

    // Open file with customizable log, by relying on OpenFile.
    // If not null, log will receive error messages on failure.
    // The 'purl' should be encoded as UTF-8 to support international file names.
    virtual GFile* OpenFileEx(const char* purl, class GFxLog *plog, 
        SInt flags = GFileConstants::Open_Read|GFileConstants::Open_Buffered, 
        SInt mode = GFileConstants::Mode_ReadWrite);
};



// ***** GFxURLBuilder

// URLBuilder class is responsible for building a filename path
// used for loading objects. If can also be overridden if filename
// or extension substitution is desired.

class GFxURLBuilder : public GFxState
{
public:
     
    GFxURLBuilder() : GFxState(State_URLBuilder) { } 

    enum FileUse
    {
        File_Regular,
        File_Import,
        File_ImageImport,
        File_LoadMovie,
        File_LoadVars,
        File_LoadXML,
        File_LoadCSS,
        File_Sound
    };

    struct LocationInfo
    {
        FileUse         Use;
        GString         FileName;
        GString         ParentPath;

        LocationInfo(FileUse use, const GString& filename)
            : Use(use), FileName(filename) { }
        LocationInfo(FileUse use, const char* pfilename)
            : Use(use), FileName(pfilename) { }
        LocationInfo(FileUse use, const GString& filename, const GString& path)
            : Use(use), FileName(filename), ParentPath(path) { }
        LocationInfo(FileUse use, const char* pfilename, const char* ppath)
            : Use(use), FileName(pfilename), ParentPath(ppath) { }
    };

    // Builds a new filename based on the provided filename and a parent path.
    // Uses IsPathAbsolute internally do determine whether the FileName is absolute.
    virtual void        BuildURL(GString *ppath, const LocationInfo& loc);

    // Static helper function used to determine if the path is absolute.
    // Relative paths should be concatenated in TranslateFilename,
    // while absolute ones are used as-is.
    static bool  GCDECL IsPathAbsolute(const char *putf8str);
    // Modifies path to not include the filename, leaves trailing '/'.
    static bool  GCDECL ExtractFilePath(GString *ppath);

    // TBD: IN the future, could handle 'http://', 'file://' prefixes and the like.

    // Default implementation used by BuildURL.
    static void GCDECL DefaultBuildURL(GString *ppath, const LocationInfo& loc); 
};


// ***** GFxImageCreator

// Interface used to create GImageInfoBase objects applied during rendering.
// Passed pimage argument can be either valid or 0; if it is valid, then
// the filled in image should be used. If it is 0, and empty image must be created.

class GFxImageCreator : public GFxState
{
    // Read-Only flag that instructs the player to keep GImage data used to create
    // GImageInfoBase in memory. This must remain read-only after creation since
    // changing it after use would cause MovieData to be bound incorrectly.
    bool    KeepImageBindData;
public:

    GFxImageCreator(bool keepImageBindData = 0)
        : GFxState(State_ImageCreator), KeepImageBindData(keepImageBindData)
    { }
    

    // If false (default), loaded movies would be bound to the image creator and the
    // associated GImage data is subsequently released. The down side of this is that
    // using a different creator will force file re-load. Returning true, however, 
    // will enable delayed image binding, and thus allow binding to different creators
    // by keeping original data copy in memory.
    // GFxLoader::LoadKeepBindData takes precedence over this flag.
    inline bool            IsKeepingImageData() const   { return KeepImageBindData; }


    // Default implementation reads images from DDS/TGA files
    // creating GImageInfo objects. This implementation can rely on
    // FileOpener / RendererConfig states passed in GFxImageCreateInfo.    
    virtual GImageInfoBase* CreateImage(const GFxImageCreateInfo &info);
    
    // Loads a GImage from an open file, assuming the specified file format.
    // Can be used as a helper when implementing CreateImage.
    static GImage* GCDECL   LoadBuiltinImage(GFile* pfile,
                                             GFxFileConstants::FileFormatType format,
                                             GFxResource::ResourceUse use,
                                             GFxLog* plog,
                                             GFxJpegSupportBase* pjpegState, GFxPNGSupportBase* ppngState,
                                             GMemoryHeap* pimageHeap);
};


// ***** GFxParseControl

class GFxParseControl : public GFxState
{
protected:
    UInt            ParseFlags;
public:    

    // Verbose constants, determine what gets displayed during parsing
    enum VerboseParseConstants
    {
        VerboseParseNone        = 0x00,
        VerboseParse            = 0x01,
        VerboseParseAction      = 0x02,
        VerboseParseShape       = 0x10,
        VerboseParseMorphShape  = 0x20,
        VerboseParseAllShapes   = VerboseParseShape | VerboseParseMorphShape,
        VerboseParseAll         = VerboseParse | VerboseParseAction | VerboseParseAllShapes
    };

    GFxParseControl(UInt parseFlags = VerboseParseNone)
        : GFxState(State_ParseControl), ParseFlags(parseFlags)
    { }

    inline void    SetParseFlags(UInt parseFlags)   { ParseFlags = parseFlags; }
    inline UInt    GetParseFlags() const            { return ParseFlags; }

    bool    IsVerboseParse() const              { return (ParseFlags & VerboseParse) != 0; }
    bool    IsVerboseParseShape() const         { return (ParseFlags & VerboseParseShape) != 0; }
    bool    IsVerboseParseMorphShape() const    { return (ParseFlags & VerboseParseMorphShape) != 0; }
    bool    IsVerboseParseAction() const        { return (ParseFlags & VerboseParseAction) != 0; }

};


// ***** GFxLoadProgressHandler

// This state is used to get a progress during the file loading
// If the state is set its methods get called when the certain 
// events happened during file loading.

class GFxProgressHandler : public GFxState
{
public:    
    GFxProgressHandler() : GFxState(State_ProgressHandler) { }

    struct Info
    {
        GString   FileUrl;      // the name of the file which the information is related to
        UInt      BytesLoaded;  // the number of bytes that have been loaded
        UInt      TotalBytes;   // the total file size
        UInt      FrameLoading; // the frame number which is being loaded
        UInt      TotalFrames;  // the total number on frames in the file.

        Info(const GString& fileUrl, UInt bytesLoaded, UInt totalBytes, 
                                       UInt frameLoading, UInt totalFrames)
            : FileUrl(fileUrl), BytesLoaded(bytesLoaded), TotalBytes(totalBytes), 
                                FrameLoading(frameLoading), TotalFrames(totalFrames) {}
    };
    // The method is called periodically called to indicate who many bytes have 
    // already been loaded and what frame is being loaded now.
    virtual void    ProgressUpdate(const Info& info) = 0;   

    struct TagInfo
    {
        GString   FileUrl;       // the name of the file which the information is related to
        int       TagType;       // the tag type id
        int       TagOffset;     // the tag offset in the file
        int       TagLength;     // the tag length
        int       TagDataOffset; 

        TagInfo(const GString& fileUrl, int tagType, int tagOffset, int tagLength, int tagDataOffset)
            : FileUrl(fileUrl), TagType(tagType), TagOffset(tagOffset), 
                                TagLength(tagLength), TagDataOffset(tagDataOffset) {}
    };
    // The method is called the a tag has just been load from the file.
    virtual void    LoadTagUpdate(const TagInfo& info, bool calledFromDefSprite) { GUNUSED2(info, calledFromDefSprite); }
};



// ***** GFxGradientParams

class GFxGradientParams : public GFxState
{
    // Grad size in pixels. 0 - means default size (see GFX_GRADIENT_SIZE_DEFAULT)
    UInt                RadialGradientImageSize; 
    bool                AdaptiveGradients;

public:
    GFxGradientParams()
        : GFxState(State_GradientParams)
    {
        RadialGradientImageSize = 0;
        AdaptiveGradients       = true;
    }

    GFxGradientParams(UInt radialSize)
        : GFxState(State_GradientParams)
    {
        RadialGradientImageSize = radialSize;
        AdaptiveGradients       = false;
    }

    inline bool     IsAdaptive() const              { return AdaptiveGradients; }
    inline void     SetAdaptive(bool adaptive)      { AdaptiveGradients = adaptive; }

    inline UInt     GetRadialImageSize() const      { return RadialGradientImageSize; }
    inline void     SetRadialImageSize(UInt rsz)    { RadialGradientImageSize = rsz; }   
};


// ***** GFxPreprocessParams

// If not null the shapes are pre-tessellated when loading 
// (see GFx_DefineShapeLoader). TessellateScale defines the 
// overall master scale at which the tessellation and EdgeAA occurs.
// TessellateEdgeAA defines whether on not EdgeAA is used. 
class GFxPreprocessParams : public GFxState
{
    Float               TessellateScale;
    bool                TessellateEdgeAA;

public:
    GFxPreprocessParams(Float tessScale=1.0f, bool tessEdgeAA=true)
        : GFxState(State_PreprocessParams)
    {
        TessellateScale  = tessScale;
        TessellateEdgeAA = tessEdgeAA;
    }

    inline Float    GetTessellateScale() const      { return TessellateScale; }
    inline void     SetTessellateScale(Float scale) { TessellateScale = scale; }   

    // It's highly recommended that the EdgeAA flag should match the one
    // used in GFxRenderConfig. Otherwise the pre-tessellated shapes will
    // be ignored.
    inline bool     GetTessellateEdgeAA() const      { return TessellateEdgeAA; }
    inline void     GetTessellateEdgeAA(bool edgeAA) { TessellateEdgeAA = edgeAA; }   
};



// Forward Declarations
class GFxFontResource;

// ***** GFxFontPackParams

// GFxFontPackParams contains the font rendering configuration options 
// necessary to generate glyph textures. By setting TextureConfig, users can
// control the size of glyphs and textures which are generated.
class GFxFontPackParams : public GFxState
{
public:
    struct TextureConfig
    {
        // The nominal size of the final anti-aliased glyphs stored in
        // the texture.  This parameter controls how large the very
        // largest glyphs will be in the texture; most glyphs will be
        // considerably smaller.  This is also the parameter that
        // controls the trade off between texture RAM usage and
        // sharpness of large text. Default is 48.
        int             NominalSize;
     
        // How much space to leave around the individual glyph image.
        // This should be at least 1.  The bigger it is, the smoother
        // the boundaries of minified text will be, but the more texture
        // space is wasted.
        int             PadPixels;
        
        // The dimensions of the textures that the glyphs get packed into.
        // Default size is 1024.
        int             TextureWidth, TextureHeight;

        TextureConfig() : 
            NominalSize(48), 
            PadPixels(3), 
            TextureWidth(1024), 
            TextureHeight(1024)
        {}
    };

public:
    GFxFontPackParams() : 
        GFxState(State_FontPackParams),
        SeparateTextures(false),
        GlyphCountLimit(0)
    {}

    // Size (in TWIPS) of the box that the glyph should stay within.
    // this *should* be 1024, but some glyphs in some fonts exceed it!
    static const Float          GlyphBoundBox;

    // Obtain/modify variables that control how glyphs are packed in textures.
    void            GetTextureConfig(TextureConfig* pconfig) { *pconfig = PackTextureConfig; }
    // Set does some bound checking to it is implemented in the source file.
    void            SetTextureConfig(const TextureConfig& config);

    bool            GetUseSeparateTextures() const    { return SeparateTextures; }
    void            SetUseSeparateTextures(bool flag) { SeparateTextures = flag; }

    int             GetGlyphCountLimit() const        { return GlyphCountLimit; }
    void            SetGlyphCountLimit(int lim)       { GlyphCountLimit = lim;  }

    Float           GetDrawGlyphScale(int nominalGlyphHeight);    

    // Return the pixel height of text, such that the
    // texture glyphs are sampled 1-to-1 texels-to-pixels.
    // I.E. the height of the glyph box, in texels.
    static Float    GSTDCALL GetTextureGlyphMaxHeight(const GFxFontResource* f);

    // State type
    virtual StateType   GetStateType() const { return State_FontPackParams; }

private:
    // Variables that control how glyphs are packed in textures.
    TextureConfig   PackTextureConfig;

    // Use separate textures for each font. Default is False.
    bool            SeparateTextures;

    // The limit of the number of glyphs in a font. 0 means "no limit".
    // If the limit is exceeded the glyphs are not packed. In this 
    // case the dynamic cache may be used if enabled. Default is 0.
    int             GlyphCountLimit;
};

// ***** GFxFontCompactorParams
//
// GFxFontCompactorParams contains the font compactor configuration options 
// There are two parameters that can be set on the contractor of this class
// 1. NominalSize - The nominal glyph size that corresponds with the input 
//    coordinates. Small nominal size results in smaller total data size, 
//    but less accurate glyphs. For example, NominalSize=256 can typically 
//    save about 25% of memory, compared with NominalSize=1024, but really 
//    big glyphs, like 500 pixels may have visual inaccuracy. For typical game 
//    UI text NominalSize=256 is still good enough. In SWF it is typically 1024.
// 2. MergeContours - A Boolean flag that tells, whether or not the 
//    FontCompactor should merge the same contours and glyphs. When merging 
//    the data can be more compact and save 10-70% of memory, depending on fonts.
//    But if the font contains too many glyphs, the hash table may consume 
//    additional memory, 12 (32-bit) or 16 (64-bit) bytes per each unique path, 
//    plus 12 (32-bit) or 16 (64-bit) bytes per each unique glyph.

class GFxFontCompactorParams : public GFxState
{
public:
    GFxFontCompactorParams(UInt nominalSize = 256, bool mergeContours = true)
        : GFxState(State_FontCompactorParams), NominalSize(nominalSize), MergeContours(mergeContours) {}

    ~GFxFontCompactorParams() {}

    UInt GetNominalSize() const { return NominalSize; }
    bool NeedMergeContours() const { return MergeContours; }
private:
    UInt NominalSize;
    bool MergeContours;
};


// ***** GFxFontCacheManager

class GFxFontCacheManager : public GFxState
{
public:
    struct TextureConfig
    {
        UInt TextureWidth;
        UInt TextureHeight; 
        UInt MaxNumTextures; 
        UInt MaxSlotHeight; 
        UInt SlotPadding;
        UInt TexUpdWidth;
        UInt TexUpdHeight;

        TextureConfig():
            TextureWidth(1024),
            TextureHeight(1024),
            MaxNumTextures(1),
            MaxSlotHeight(48),
            SlotPadding(2),
            TexUpdWidth(256),
            TexUpdHeight(512)
        {}
    };

    GFxFontCacheManager(bool enableDynamicCache=true, bool debugHeap = false);
    GFxFontCacheManager(const TextureConfig& config,
                        bool enableDynamicCache=true, bool debugHeap = false);
   ~GFxFontCacheManager();

    // Init glyph textures. In some cases it may be desirable to initialize
    // the textures in advance. The function may be called once before the 
    // main display loop. Without this call the textures will be 
    // initialized on demand as necessary.
    void InitTextures(GRenderer* ren);

    void  GetTextureConfig(TextureConfig* config) { *config = CacheTextureConfig; }
    void  SetTextureConfig(const TextureConfig& config);

    void  EnableDynamicCache(bool v=true) { DynamicCacheEnabled = v; }
    bool  IsDynamicCacheEnabled() const   { return DynamicCacheEnabled; }

    void  EnableText3DVectorization(bool v=true) { Text3DVectorizationEnabled = v; }
    bool  IsText3DVectorizationEnabled() const   { return Text3DVectorizationEnabled; }

    void  SetMaxRasterScale(Float s) { MaxRasterScale = s;    }
    Float GetMaxRasterScale() const  { return MaxRasterScale; }

    void  SetSteadyCount(UInt n) { SteadyCount = n; }
    UInt  GetSteadyCount() const { return SteadyCount; }

    void  SetMaxVectorCacheSize(UInt n);
    UInt  GetMaxVectorCacheSize() const { return MaxVectorCacheSize; }

    void  SetFauxItalicAngle(Float a);
    Float GetFauxItalicAngle() const  { return FauxItalicAngle; }

    void  SetFauxBoldRatio(Float r);
    Float GetFauxBoldRatio() const  { return FauxBoldRatio; }

    void  SetOutlineRatio(Float r);
    Float SetOutlineRatio() const  { return OutlineRatio; }

    void  SetNumLockedFrames(UInt num);
    UInt  GetNumLockedFrames() const { return NumLockedFrames; }

    UInt  GetNumRasterizedGlyphs() const;
    UInt  GetNumTextures() const;

    class GTexture* GetTexture(UInt i) const;

    void  VisitGlyphs(class GFxGlyphCacheVisitor* visitor) const;

    class GFxFontCacheManagerImpl* GetImplementation() { return pCache; }

    GMemoryHeap*                   GetHeap() const;

private:
    void                           InitManager(const TextureConfig& config, bool debugHeap);

    TextureConfig                  CacheTextureConfig;
    bool                           DynamicCacheEnabled;
    bool                           Text3DVectorizationEnabled;
    Float                          MaxRasterScale;
    UInt                           SteadyCount;
    UInt                           MaxVectorCacheSize;
    Float                          FauxItalicAngle;
    Float                          FauxBoldRatio;
    Float                          OutlineRatio;
    UInt                           NumLockedFrames;
    class GFxFontCacheManagerImpl* pCache;
};

// ***** GFxFontProvider

// An external interface to handle device/system fonts

class GFxFont;
class GFxFontProvider : public GFxState
{
public:
    GFxFontProvider() : GFxState(State_FontProvider) {}
    virtual ~GFxFontProvider() {}
    
    // fontFlags style as described by GFxFont::FontFlags
    virtual GFxFont*    CreateFont(const char* name, UInt fontFlags) = 0;
    //void EnumerateFonts interface...

    virtual void        LoadFontNames(GStringHash<GString>& fontnames) = 0;
};


// ***** GFxFontMap

// Font map defines a set of font name substitutions that apply fonts
// looked up/create from GFxFontLib and GFxFontProvider. By installing a
// map you can change fonts used by SWF/GFX file to refer to alternative
// fonts. For example, if the file makes use of the "Helvetica" font while
// your system only provides "Arial", you can install it as a substitution
// by doing the following:
//    GPtr<GFxFontMap> pfontMap = *new GFxFontMap;
//    pfontMap->AddFontMap("Helvetica", "Arial");
//    loader.SetFontMap(Map);

class GFxFontMap : public GFxState
{
    class GFxFontMapImpl *pImpl;
public:
    GFxFontMap();
    virtual ~GFxFontMap();

    enum MapFontFlags
    {
        // Default state,
        MFF_Original        = 0x0010,
        MFF_NoAutoFit       = 0x0020,

        // Other font flag combinations, indicate that a specific font type
        // will be looked up. The must match up with constants in GFxFont.
        MFF_Normal          = 0x0000,
        MFF_Italic          = 0x0001,
        MFF_Bold            = 0x0002,
        MFF_BoldItalic      = MFF_Italic | MFF_Bold,

        MFF_FauxItalic      = 0x0004,
        MFF_FauxBold        = 0x0008,
        MFF_FauxBoldItalic  = MFF_FauxItalic | MFF_FauxBold
    };

    class MapEntry
    {
    public:
        GString         Name;
        Float           ScaleFactor;
        Float           GlyphOffsetX, GlyphOffsetY; // 0..1023, EM square
        MapFontFlags    Flags;

        MapEntry():ScaleFactor(1.f) { Flags = MFF_Original; }
        MapEntry(const GString& name, MapFontFlags flags, Float scaleFactor = 1.0f,
            Float offx = 0, Float offy = 0) : 
            Name(name), ScaleFactor(scaleFactor), GlyphOffsetX(offx), GlyphOffsetY(offy), Flags(flags) { }
        MapEntry(const MapEntry &src) : Name(src.Name), ScaleFactor(src.ScaleFactor), 
            GlyphOffsetX(src.GlyphOffsetX), GlyphOffsetY(src.GlyphOffsetY), Flags(src.Flags) { }        
        MapEntry& operator = (const MapEntry &src)
        { 
            Name = src.Name; ScaleFactor = src.ScaleFactor; Flags = src.Flags; 
            GlyphOffsetX = src.GlyphOffsetX; GlyphOffsetY = src.GlyphOffsetY;
            return *this; 
        }

        // Updates font flags generating new ones based on local MapFontFlags.
        UInt            UpdateFontFlags(UInt originalFlags)
        {
            return (Flags == MFF_Original) ? originalFlags :
                   ((originalFlags & ~MFF_BoldItalic) | (Flags & ~MFF_FauxBoldItalic));
        }
        UInt GetOverridenFlags() const
        {
            return (((Flags & MFF_FauxBoldItalic) ? Flags >> 2 : 0) & MFF_BoldItalic) | (Flags & MFF_NoAutoFit);
        }
    };

    // Adds a font mapping; font names expressed in UTF8.
    // A function will fail if a mapping for this font already exists,
    // overwriting a map is not allowed.
    bool            MapFont(const char* pfontName, const char* pnewFontName,
                            MapFontFlags mff = MFF_Original, Float scaleFactor = 1.0f,
                            Float offx = 0, Float offy = 0);
    // Adds a font mapping; font names expressed in Unicode.
    bool            MapFont(const wchar_t* pfontName, const wchar_t* pnewFontName,
                            MapFontFlags mff = MFF_Original, Float scaleFactor = 1.0f,
                            Float offx = 0, Float offy = 0);
    
    // Obtains a font mapping, filling in MapEntry if succeeds.
    bool            GetFontMapping(MapEntry* pentry, const char *pfontName);
};

// ***** GFxTextClipboard
// The default implementation of text clipboard for textfields. This
// implementation is platform independent. To make a platform specific
// implementation - inherit from this class and overload the OnTextStore
// virtual method.

class GFxTextClipboard : public GFxState
{
protected:
    GFxWStringBuffer        PlainText;
    class GFxStyledText*    pStyledText;
protected:
    void SetPlainText(const wchar_t* ptext, UPInt len);
    void SetStyledText(class GFxStyledText*);
    void ReleaseStyledText();
public:
    GFxTextClipboard();
    virtual ~GFxTextClipboard();

    // plain text storing
    void SetText(const GString&);
    void SetText(const wchar_t* ptext, UPInt len = GFC_MAX_UPINT);
    const GFxWStringBuffer& GetText() const;

    // rich text storing
    void SetTextAndStyledText(const wchar_t* ptext, UPInt len, class GFxStyledText*);
    class GFxStyledText* GetStyledText() const;

    bool ContainsRichText() const { return pStyledText != NULL; }

    virtual void OnTextStore(const wchar_t* ptext, UPInt len) { GUNUSED2(ptext, len); }
};

#ifndef GFC_NO_TEXT_INPUT_SUPPORT
// ***** GFxTextKeyMap
// Text keyboard map that specifies functionality of each keystroke in a textfield.
// This state might be useful to re-define the default key layout on different
// platforms.

class GFxTextKeyMap : public GFxState
{
public:
    enum KeyAction
    {
        KeyAct_None,
        KeyAct_ToggleSelectionMode,
        KeyAct_EnterSelectionMode,
        KeyAct_LeaveSelectionMode,
        KeyAct_Left,
        KeyAct_Right,
        KeyAct_Up,
        KeyAct_Down,
        KeyAct_PageUp,
        KeyAct_PageDown,
        KeyAct_LineHome,
        KeyAct_LineEnd,
        KeyAct_PageHome,
        KeyAct_PageEnd,
        KeyAct_DocHome,
        KeyAct_DocEnd,
        KeyAct_Backspace,
        KeyAct_Delete,
        KeyAct_Return,
        KeyAct_Copy,
        KeyAct_Paste,
        KeyAct_Cut,
        KeyAct_SelectAll
    };
    enum KeyState
    {
        State_Down,
        State_Up
    };
    struct KeyMapEntry
    {
        KeyAction           Action;
        UInt                KeyCode;
        UInt                SpecKeysPressed;
        KeyState            State;

        KeyMapEntry():KeyCode(GFC_MAX_UINT) {}
        KeyMapEntry(KeyAction ka, UInt kc, UInt8 specKeys = 0, KeyState ks = State_Down):
        Action(ka), KeyCode(kc), SpecKeysPressed(specKeys), State(ks) {}
    };
    GArray<KeyMapEntry>     Map;

    GFxTextKeyMap();

    GFxTextKeyMap* InitWindowsKeyMap();
    GFxTextKeyMap* InitMacKeyMap();

    void AddKeyEntry(const KeyMapEntry&);

    const KeyMapEntry* FindFirstEntry(UInt keyCode) const;
    const KeyMapEntry* FindNextEntry(const KeyMapEntry*) const;

    const KeyMapEntry* Find(UInt keyCode, const GFxSpecialKeysState& specKeys, KeyState state) const;
};
#else
class GFxTextKeyMap : public GFxState
{
public: 
    GFxTextKeyMap() { }

    GFxTextKeyMap* InitWindowsKeyMap() { return this; }
    GFxTextKeyMap* InitMacKeyMap()     { return this; }
};
#endif // GFC_NO_TEXT_INPUT_SUPPORT

// ***** GFxImagePacker
// Pack images into large textures. This is intended to be used within GFxExport for producing
// .gfx files that reference packed textures. It can also be used in a player, but causes:
// - All movie loading will wait for completion
// - More memory will be used for images as the original image data from the SWF file remains

class GFxImagePacker;

// Used in the packer to substitute embedded images
class GFxImageSubstProvider : public GRefCountBase<GFxImageSubstProvider, GStat_Default_Mem>
{
public:
    virtual GImage* CreateImage(const char* pname) = 0;
};

class GFxImagePackParamsBase : public GFxState
{
public:
    enum SizeOptionType
    {
        PackSize_1,
        PackSize_4,             // width and height of textures expanded to multiple of 4
        PackSize_PowerOf2
    };

    struct TextureConfig
    {
        int             TextureWidth, TextureHeight; // Max size of one texture. If a smaller image is produced it will follow SizeOptions
        SizeOptionType  SizeOptions;                 // If a renderer is set on the loader, renderer caps will be used instead.

        TextureConfig() : 
            TextureWidth(1024), 
            TextureHeight(1024),
            SizeOptions(PackSize_1)
        {}
    };

public:
    GFxImagePackParamsBase() : 
      GFxState(State_ImagePackerParams), pImageSubstProvider(NULL)
      {}

      // Obtain/modify variables that control how images are packed in textures.
      void            GetTextureConfig(TextureConfig* pconfig) const { *pconfig = PackTextureConfig; }
      void            SetTextureConfig(const TextureConfig& config);
      void            SetImageSubstProvider(GFxImageSubstProvider *pisp) {pImageSubstProvider = pisp;}
      GFxImageSubstProvider* GetImageSubstProvider() const { return pImageSubstProvider; }
      // State type
      virtual StateType   GetStateType() const { return State_ImagePackerParams; }

      virtual GFxImagePacker* Begin(GFxResourceId* pIdGen, GFxImageCreator* pic, GFxImageCreateInfo* pici) const = 0;

private:
    // Variables that control how glyphs are packed in textures.
    TextureConfig   PackTextureConfig;
    GPtr<GFxImageSubstProvider> pImageSubstProvider;
};



class GFxImagePackParams : public GFxImagePackParamsBase
{
public:
    virtual GFxImagePacker* Begin(GFxResourceId* pIdGen, GFxImageCreator* pic, GFxImageCreateInfo* pici) const;

};


// ***** GFxJpegSupportBase
// The purpose of this interface is to provide  Jpeg decompresser to the loader

class GFxJpegSupportBase : public GFxState
{
public:
    GFxJpegSupportBase() : GFxState(State_JpegSupport) {}

    // Read SWF JPEG2-style Header (separate encoding
    // table followed by image data), and create jpeg
    // input object.
    virtual GJPEGInput* CreateSwfJpeg2HeaderOnly(GFile* pin) = 0;

    // Create and read a new image from the stream.
    virtual GImage* ReadJpeg(GFile* pin, GMemoryHeap* pimageHeap) = 0;

    // Create and read a new image from the stream.  Image is pin
    // SWF JPEG2-style Format (the encoding tables come first pin a
    // separate "stream" -- otherwise it's just normal JPEG).  The
    // IJG documentation describes this as "abbreviated" format.
    virtual GImage* ReadSwfJpeg2(GFile* pin, GMemoryHeap* pimageHeap) = 0;

    // Create and read a new image, using a input object that
    // already has tables loaded.
    virtual GImage* ReadSwfJpeg2WithTables(GJPEGInput* pjin, GMemoryHeap* pimageHeap) = 0;

    // For reading SWF JPEG3-style image data, like ordinary JPEG, 
    // but stores the data pin GImage format.
    virtual GImage* ReadSwfJpeg3(GFile* pin, GMemoryHeap* pimageHeap) = 0;
};

// ***** GFxJpegSupport 
// Default implementation of GFxJpegSupportBase interface
// The instance of this class is set on the loader contractor
// If the application does not need JPEG support the loader constructor
// should be called with the third parameter set to NULL 

class GFxJpegSupport : public GFxJpegSupportBase
{
public:
    GFxJpegSupport();

    // Read SWF JPEG2-style Header (separate encoding
    // table followed by image data), and create jpeg
    // input object.
    virtual GJPEGInput* CreateSwfJpeg2HeaderOnly(GFile* pin);

    // Create and read a new image from the stream.
    virtual GImage* ReadJpeg(GFile* pin, GMemoryHeap* pimageHeap);

    // Create and read a new image from the stream.  Image is pin
    // SWF JPEG2-style Format (the encoding tables come first pin a
    // separate "stream" -- otherwise it's just normal JPEG).  The
    // IJG documentation describes this as "abbreviated" format.
    virtual GImage* ReadSwfJpeg2(GFile* pin, GMemoryHeap* pimageHeap);

    // Create and read a new image, using a input object that
    // already has tables loaded.
    virtual GImage* ReadSwfJpeg2WithTables(GJPEGInput* pjin, GMemoryHeap* pimageHeap);

    // For reading SWF JPEG3-style image data, like ordinary JPEG, 
    // but stores the data pin GImage format.
    virtual GImage* ReadSwfJpeg3(GFile* pin, GMemoryHeap* pimageHeap);

private:
    GPtr<GJPEGSystem> pSystem;
};


// ***** GFxPNGSupportBase
// The purpose of this interface is to provide PNG decompresser to the loader
// This state is not set by default. If the application needs PNG support the user
// has to set an instance of an implementation of this class (GFxPNGSupport) 
// to the loader

class GFxPNGSupportBase : public GFxState
{
public:
    GFxPNGSupportBase() : GFxState(State_PNGSupport) {}

    virtual GImage* CreateImage(GFile* pin, GMemoryHeap* pimageHeap) = 0;
};

// ***** GFxPNGSupport
// Default implementation of GFxPNGSupportBase interface

class GFxPNGSupport : public GFxPNGSupportBase
{
public:
    virtual GImage* CreateImage(GFile* pin, GMemoryHeap* pimageHeap);
};

// ***** GFxZlibSupportBase
// The purpose of this interface is to provide ZLIB decompresser to the loader

class GFxStream;
class GFxZlibSupportBase : public GFxState
{
public:
    GFxZlibSupportBase() : GFxState(State_ZlibSupport) {}

    virtual GFile* CreateZlibFile(GFile* in) = 0;
    virtual void   InflateWrapper(GFxStream* pinStream, void* buffer, int BufferBytes) = 0;
};

// ***** GFxZlibSupport
// Default implementation of GFxZlibSupportBase interface
// The instance of this class is set on the loader contractor
// If the application does not need ZLIB support the loader constructor
// should be called with the second parameter set to NULL 

class GFxZlibSupport : public GFxZlibSupportBase
{
public:
    virtual GFile* CreateZlibFile(GFile* in);
    virtual void   InflateWrapper(GFxStream* pinStream, void* buffer, int BufferBytes);
};

// ***** GFxStateBag

// GFxStateBag collects accessor methods for properties that can be
// inherited from GFxLoader to GFxMovieDef, and then to GFxMovieView.
// These properties can be overwritten in any one of above mentioned instances.

class GFxStateBag : public GFxFileConstants
{
protected:
    virtual GFxStateBag* GetStateBagImpl() const { return 0; }

public:
    virtual ~GFxStateBag () { }
    
    // General state access. We need to pass state type to SetState so that we know
    // which state is being cleared when null is passed.    
    virtual void        SetState(GFxState::StateType state, GFxState* pstate)
    {
        GFxStateBag* p = GetStateBagImpl();    
        GASSERT((pstate == 0) ? 1 : (pstate->GetStateType() == state));
        if (p) p->SetState(state, pstate);
    }
    // We always need to return AddRef'ed state value from GetState to remain
    // thread safe, since otherwise another thread might overwrite the state
    // killing it prematurely and causing Get to return a bad pointer.
    virtual GFxState*   GetStateAddRef(GFxState::StateType state) const
    {
        GFxStateBag* p = GetStateBagImpl();
        return p ? p->GetStateAddRef(state) : 0;
    }

    // Fills in a set of states with one call.
    // Initial array pstates must contain null pointers.
    virtual void        GetStatesAddRef(GFxState** pstateList,
                                        const GFxState::StateType *pstates, UInt count) const
    {
        GFxStateBag* p = GetStateBagImpl();
        if (p) p->GetStatesAddRef(pstateList, pstates, count);
    }


    // *** Inlines for convenient state access.
 
    // Sets rendering configuration - implemented in GFxRenderConfig.h.
    inline void                 SetRenderConfig(GFxRenderConfig* pri);
    inline GPtr<GFxRenderConfig> GetRenderConfig() const;

    inline void                 SetRenderConfig(GRenderer *prenderer, Float maxError = 1.0f, UInt rendererFlags = 0);
    inline GPtr<GRenderer>      GetRenderer() const;

    inline void                 SetRenderStats(GFxRenderStats* prs);
    inline GPtr<GFxRenderStats> GetRenderStats() const;

#ifndef GFC_NO_SOUND
    inline void                 SetAudio(GFxAudioBase* ptr);
    inline GPtr<GFxAudioBase>   GetAudio() const;
    inline GSoundRenderer*      GetSoundRenderer() const;
#endif // GFC_NO_SOUND

    // Sets an interface that will be used for logging - implemented in GFxLog.h.
    inline void                 SetLog(GFxLog *plog);
    inline GPtr<GFxLog>         GetLog() const;

    // Set an interface that will be used to translate dynamic text field for international languages.
    inline void                 SetTranslator(GFxTranslator *ptr)           { SetState(GFxState::State_Translator, ptr); }
    inline GPtr<GFxTranslator>  GetTranslator() const                       { return *(GFxTranslator*)GetStateAddRef(GFxState::State_Translator); }

    // Sets loadMovie interface used for images.
    inline void                 SetImageLoader(GFxImageLoader* pmil)        { SetState(GFxState::State_ImageLoader, pmil); }
    inline GPtr<GFxImageLoader> GetImageLoader() const                      { return *(GFxImageLoader*)GetStateAddRef(GFxState::State_ImageLoader); }

    // Loads an image based on installed LoadMovieImageCallback.
    // The 'purl' should be encoded as UTF-8 to support international file names.
    inline GImageInfoBase*      LoadMovieImage(const char* purl)
    { GFxImageLoader* p = GetImageLoader(); return p ? p->LoadImage(purl) : 0; }


    // Set ActionScript control states.
    inline void                 SetActionControl(GFxActionControl* pac)     { SetState(GFxState::State_ActionControl, pac); }
    inline GPtr<GFxActionControl> GetActionControl() const                  { return *(GFxActionControl*)GetStateAddRef(GFxState::State_ActionControl); }

    // UserEventHander - used to control events in 
    inline void                 SetUserEventHandler(GFxUserEventHandler* pri) { SetState(GFxState::State_UserEventHandler, pri); }
    inline GPtr<GFxUserEventHandler> GetUserEventHandler() const            { return *(GFxUserEventHandler*)GetStateAddRef(GFxState::State_UserEventHandler); }

    // Sets loadMovie interface used for images.
    inline void                 SetFSCommandHandler(GFxFSCommandHandler* ps){ SetState(GFxState::State_FSCommandHandler, ps); }
    inline GPtr<GFxFSCommandHandler> GetFSCommandHandler() const            { return *(GFxFSCommandHandler*)GetStateAddRef(GFxState::State_FSCommandHandler); }

    //Set TestStream for recording end reproducing events in Tester
    inline void                 SetTestStream(GFxTestStream* pts)           { SetState(GFxState::State_TestStream, pts); }
    inline GPtr<GFxTestStream>  GetTestStream() const                       { return *(GFxTestStream*)GetStateAddRef(GFxState::State_TestStream); }

    // Sets the external interface used - implemented in GFxPlayer.h.
    inline void                 SetExternalInterface(GFxExternalInterface* p);
    inline GPtr<GFxExternalInterface> GetExternalInterface() const;


    // Installs a callback function that is always used by GFxLoader
    // for opening various files based on a path or url string.
    // If not specified, the system file input will be used
    inline  void                SetFileOpener(GFxFileOpenerBase *ptr)       { SetState(GFxState::State_FileOpener, ptr); }
    inline  GPtr<GFxFileOpenerBase> GetFileOpener() const                   { return *(GFxFileOpenerBase*)GetStateAddRef(GFxState::State_FileOpener); }

    // Open file with customizable log. If not null, log will receive error messages on failure.
    GFile*                      OpenFileEx(const char *pfilename, class GFxLog *plog);
    // Opens a file using the specified callback, same as above but uses local log.
    GFile*                      OpenFile(const char *pfilename);
    
    // File name translation.
    inline void                 SetURLBuilder(GFxURLBuilder *ptr)           { SetState(GFxState::State_URLBuilder, ptr); }
    inline GPtr<GFxURLBuilder>  GetURLBuilder() const                       { return *(GFxURLBuilder*)GetStateAddRef(GFxState::State_URLBuilder); }

    
    // Sets the callback function that will be used to create/initalize images
    // during loading. If no function is set, GImageInfo will be used.
    inline void                 SetImageCreator(GFxImageCreator *ptr)       { SetState(GFxState::State_ImageCreator, ptr); }
    inline GPtr<GFxImageCreator> GetImageCreator() const                    { return *(GFxImageCreator*)GetStateAddRef(GFxState::State_ImageCreator); }

    // Creates an image based on a user-defined creator; if not available uses default logic.
    GImageInfoBase*             CreateImageInfo(const GFxImageCreateInfo& info);   


    // Sets the callback function that will be used to create/initalize images
    // during loading. If no function is set, GImageInfo will be used.
    inline void                 SetParseControl(GFxParseControl *ptr)       { SetState(GFxState::State_ParseControl, ptr); }
    inline GPtr<GFxParseControl> GetParseControl() const                    { return *(GFxParseControl*)GetStateAddRef(GFxState::State_ParseControl); }

    inline void                 SetProgressHandler(GFxProgressHandler *ptr) { SetState(GFxState::State_ProgressHandler, ptr); }
    inline GPtr<GFxProgressHandler> GetProgressHandler() const              { return *(GFxProgressHandler*)GetStateAddRef(GFxState::State_ProgressHandler); }

    // Sets a default visitor that is used when after files are loaded.
    inline void                 SetImportVisitor(GFxImportVisitor *ptr);
    inline GPtr<GFxImportVisitor> GetImportVisitor() const;

    // GFxStateBag access for FontFontPackParams
    inline void                 SetFontPackParams(GFxFontPackParams *ptr)   { SetState(GFxState::State_FontPackParams, ptr); }
    inline GPtr<GFxFontPackParams> GetFontPackParams() const                { return *(GFxFontPackParams*) GetStateAddRef(GFxState::State_FontPackParams); }

    inline void                  SetMeshCacheManager(GFxMeshCacheManager* ptr) { SetState(GFxState::State_MeshCacheManager, ptr); }
    inline GPtr<GFxMeshCacheManager> GetMeshCacheManager() const            { return *(GFxMeshCacheManager*)GetStateAddRef(GFxState::State_MeshCacheManager); }

    inline void                 SetFontCacheManager(GFxFontCacheManager *ptr) { SetState(GFxState::State_FontCacheManager, ptr); }
    inline GPtr<GFxFontCacheManager> GetFontCacheManager() const            { return *(GFxFontCacheManager*) GetStateAddRef(GFxState::State_FontCacheManager); }

    inline void                 SetFontLib(GFxFontLib* pfl);
    inline GPtr<GFxFontLib>     GetFontLib() const;

    inline void                 SetFontProvider(GFxFontProvider *ptr)       { SetState(GFxState::State_FontProvider, ptr); }
    inline GPtr<GFxFontProvider> GetFontProvider() const                    { return *(GFxFontProvider*) GetStateAddRef(GFxState::State_FontProvider); }

    inline void                 SetFontMap(GFxFontMap *ptr)                 { SetState(GFxState::State_FontMap, ptr); }
    inline GPtr<GFxFontMap>     GetFontMap() const                          { return *(GFxFontMap*) GetStateAddRef(GFxState::State_FontMap); }

    inline void                 SetImagePackParams(GFxImagePackParams *ptr) { SetState(GFxState::State_ImagePackerParams, ptr); }
    inline GPtr<GFxImagePackParams> GetImagePackerParams() const          { return *(GFxImagePackParams*) GetStateAddRef(GFxState::State_ImagePackerParams); }

    // Sets an interface to Gradient
    inline void                 SetGradientParams(GFxGradientParams *ptr)   { SetState(GFxState::State_GradientParams, ptr); }
    inline GPtr<GFxGradientParams> GetGradientParams() const                { return *(GFxGradientParams*) GetStateAddRef(GFxState::State_GradientParams); }

    // Sets an interface to Preprocess params.
    inline void                 SetPreprocessParams(GFxPreprocessParams *ptr) { SetState(GFxState::State_PreprocessParams, ptr); }
    inline GPtr<GFxPreprocessParams> GetPreprocessParams() const        { return *(GFxPreprocessParams*) GetStateAddRef(GFxState::State_PreprocessParams); }

    // Sets a task manager - used to issue I/O and general processing tasks on other threads.
    inline void                 SetTaskManager(GFxTaskManager *ptr);
    inline GPtr<GFxTaskManager> GetTaskManager() const;

    inline void                 SetTextClipboard(GFxTextClipboard *ptr) { SetState(GFxState::State_TextClipboard, ptr); }
    inline GPtr<GFxTextClipboard> GetTextClipboard() const              { return *(GFxTextClipboard*) GetStateAddRef(GFxState::State_TextClipboard); }

    inline void                 SetTextKeyMap(GFxTextKeyMap *ptr)       { SetState(GFxState::State_TextKeyMap, ptr); }
    inline GPtr<GFxTextKeyMap>  GetTextKeyMap() const                   { return *(GFxTextKeyMap*) GetStateAddRef(GFxState::State_TextKeyMap); }

    inline void                 SetIMEManager(GFxIMEManager *ptr);
    inline GPtr<GFxIMEManager>  GetIMEManager() const;

    inline void                 SetXMLSupport(GFxXMLSupportBase *ptr);
    inline GPtr<GFxXMLSupportBase>  GetXMLSupport() const;

    inline void                 SetJpegSupport(GFxJpegSupportBase *ptr) { SetState(GFxState::State_JpegSupport, ptr); }
    inline GPtr<GFxJpegSupportBase> GetJpegSupport() const              { return *(GFxJpegSupportBase*) GetStateAddRef(GFxState::State_JpegSupport); }

    inline void                 SetZlibSupport(GFxZlibSupportBase *ptr) { SetState(GFxState::State_ZlibSupport, ptr); }
    inline GPtr<GFxZlibSupportBase> GetZlibSupport() const              { return *(GFxZlibSupportBase*) GetStateAddRef(GFxState::State_ZlibSupport); }

    inline void                 SetFontCompactorParams(GFxFontCompactorParams *ptr) { SetState(GFxState::State_FontCompactorParams, ptr); }
    inline GPtr<GFxFontCompactorParams> GetFontCompactorParams() const  { return *(GFxFontCompactorParams*) GetStateAddRef(GFxState::State_FontCompactorParams); }

    inline void                 SetPNGSupport(GFxPNGSupportBase *ptr)   { SetState(GFxState::State_PNGSupport, ptr); }
    inline GPtr<GFxPNGSupportBase> GetPNGSupport() const                { return *(GFxPNGSupportBase*) GetStateAddRef(GFxState::State_PNGSupport); }
#ifndef GFC_NO_VIDEO
    // defined in GFxVideoBase.h
    inline void                 SetVideo(GFxVideoBase* ptr);
    inline GPtr<GFxVideoBase>   GetVideo() const;
#endif
#ifndef GFC_NO_FXPLAYER_AS_SHAREDOBJECT
    inline void                 SetSharedObjectManager(GFxSharedObjectManagerBase *ptr);
    inline GPtr<GFxSharedObjectManagerBase> GetSharedObjectManager() const;
#endif
}; 

// ***** GFxLoader

//  This is the main public interface that loads SWF Files,
//  and maintains the loader state.

struct GFxMovieInfo
{
    // Flag definitions for Flags field.
    enum SWFFlagConstants
    {
        SWF_Compressed  = 0x0001,
        SWF_Stripped    = 0x0010
    };

    UInt    Version;
    UInt    Flags;
    SInt    Width, Height;
    Float   FPS;
    UInt    FrameCount;
    UInt    TagCount;   

    // Extra exporter info
    UInt16  ExporterVersion; // Example: Version 1.10 will be encoded as 0x10A.
    UInt32  ExporterFlags;   // Same content as GFxExporterInfo::Flags.

    void    Clear()
    {
        Version = 0;
        Flags   = 0;
        Width = Height = 0;
        FrameCount = TagCount = 0;
        FPS = 0.0f;   
        ExporterVersion = 0;
        ExporterFlags   = 0;
    }

    bool    IsStripped() const      { return (Flags & SWF_Stripped) != 0;  }
    bool    IsCompressed() const    { return (Flags & SWF_Compressed) != 0;  }
};


// Information provided to the stripper tool when converting '.swf' file to '.gfx'.

struct GFxExporterInfo
{
    GFxFileConstants::FileFormatType Format;    // Format used for images.
    const char*     pPrefix;                    // Prefix assigned to extracted image files.
    const char*     pSWFName;                   // Original SWF file name from which '.gfx' was created.
    UInt16          Version;                    // Export tool version.

    // Export flag bits.
    enum ExportFlagConstants
    {
        EXF_GlyphTexturesExported       = 0x01,
        EXF_GradientTexturesExported    = 0x02,
        EXF_GlyphsStripped              = 0x10
    };
    UInt            ExportFlags;                // Export flags described by ExportFlagConstants.
};


// GFxLoader provider the following functionality:
//   - collection of states describing loading / execution characteristics
//   - a resource library referenced during loading
//   - a task manager used for threaded loading, if any
//   - An API used to load movies based on the local states

class GFxLoader : public GFxStateBag
{
    // Hidden loader implementation and context pointer.
    class GFxLoaderImpl*    pImpl;
    GFxResourceLib*         pStrongResourceLib;
    UInt                    DefLoadFlags;
protected:
    virtual GFxStateBag* GetStateBagImpl() const;
    virtual bool CheckTagLoader(int tagType) const;
public:

    // Constant flags that affect loading implementation. These are most typically
    // passed to CreateMovie, but can also be specified as a part of GFxLoader
    // constructor (in which cases, the specified flags are automatically
    // applied with 'or' operator to every CreateMovie call).

    enum LoadConstants
    {
        // *** General Loading Flags.

        // Default.
        LoadAll             = 0x00000000,

        // Wait until all of the loading has completed before returning GFxMovieDef.
        LoadWaitCompletion  = 0x00000001,
        LoadWaitFrame1      = 0x00000002,

        // Specified ordered binding, where resources are loaded after main data file.
        // If not specified, resource loading will be interleaved with resource and import binding.
        LoadOrdered         = 0x00000010,
        // Perform MovieData to Resource/Import binding on a separate thread.
        LoadThreadedBinding = 0x00000020,

        // This flag should be set if CreateMovie is called not on the main thread
        // We need this because in this case we need to postpone textures creation 
        // until the renderer needs them on the main thread
        LoadOnThread        = 0x00000040,

        // Set to preserve Creator bind data.
        // If set image/resource data used for creation will be kept so that
        // it can be re-bound with a different creator instance if necessary.
        // Takes precedence over GFxImageLoader::IsKeepingImageData
        LoadKeepBindData    = 0x00000080,

        // Set this flag to allow images to be loaded into root MovieDef;
        // file formats will be detected automatically.
        LoadImageFiles      = 0x00010000,
        // Set to disable loading of SWF files; only GFX content is allowed
        LoadDisableSWF      = 0x00080000,

        // Disable loading of imports. Files loaded without imports may not
        // be functional. This is primarily used for 'gfxexport' tool.
        LoadDisableImports  = 0x00100000,

        // Disable logging error message if requested file is not found
        LoadQuietOpen       = 0x00200000,


        // *** GFxLoader  Constructor only

        // Marks all loaded/created heaps with UserDebug flag, so that they can
        // be subtracted from the memory reports.
        LoadDebugHeap       = 0x10000000
    };


// Define optional library constructor macros. These are defined conditionally
// to force library linking only when used.
#ifdef GFC_USE_ZLIB
    #define GFX_LOADER_NEW_ZLIBSUPPORT *new GFxZlibSupport    
#else
    #define GFX_LOADER_NEW_ZLIBSUPPORT ((GFxZlibSupport*)0)
#endif
#ifdef GFC_USE_LIBJPEG
    #define GFX_LOADER_NEW_JPEGSUPPORT *new GFxJpegSupport
#else
    #define GFX_LOADER_NEW_JPEGSUPPORT ((GFxJpegSupport*)0)
#endif

    // A structure wrapping together different loader parameters into one,
    // including default loading flags and optional link-dependent states.
    // If certain states, such as GFxZlibSupport are not specified here,
    // they are similarly not linked into the application.    
    struct LoaderConfig
    {
        UInt                     DefLoadFlags;
        GPtr<GFxFileOpenerBase>  pFileOpener;
        GPtr<GFxZlibSupportBase> pZLibSupport;
        GPtr<GFxJpegSupportBase> pJpegSupport;
        
        LoaderConfig(UInt loadFlags = 0,
                     const GPtr<GFxFileOpenerBase>& pfileOpener = *new GFxFileOpener,
                     const GPtr<GFxZlibSupportBase>& pzlib = GFX_LOADER_NEW_ZLIBSUPPORT,
                     const GPtr<GFxJpegSupportBase>& pjpeg = GFX_LOADER_NEW_JPEGSUPPORT)
            : DefLoadFlags(loadFlags), 
              pFileOpener(pfileOpener), pZLibSupport(pzlib), pJpegSupport(pjpeg)
        { }
        LoaderConfig(const LoaderConfig& src)
            : DefLoadFlags(src.DefLoadFlags), pFileOpener(src.pFileOpener),
            pZLibSupport(src.pZLibSupport), pJpegSupport(src.pJpegSupport)
        { }
    };


    // GFxLoader Constructors.
    // Default initializers create a number of optional libraries which can
    // be excluded from build if set to 0 instead.
    GFxLoader(const LoaderConfig& config);

    GFxLoader(const GPtr<GFxFileOpenerBase>& pfileOpener = *new GFxFileOpener,
              const GPtr<GFxZlibSupportBase>& zlib = GFX_LOADER_NEW_ZLIBSUPPORT,
              const GPtr<GFxJpegSupportBase>& pjpeg = GFX_LOADER_NEW_JPEGSUPPORT);

    // Create a new loader, copying it's library and states.
    GFxLoader(const GFxLoader& src);

    GEXPORT virtual ~GFxLoader();

    
    // *** Task Manager

    // Specify task manager used for threaded loading, if any.


    // *** Library management

    // GFxResourceLib is a single heterogeneous library, containing image loaded movies,
    // images, gradients and other resources identified by a key. A default library is
    // created by GFxLoader constructor; however, users can change it and/or force library 
    // sharing across loaders by using these methods.
    void               SetResourceLib(GFxResourceLib *plib);
    GFxResourceLib*    GetResourceLib() const;


    // *** SWF File Loading

    // Obtains information about SWF file and checks for its availability.
    // Return 1 if the info was obtained successfully (or was null, but SWF file existed),
    // or 0 if it did not exist. 
    // The 'pfilename' should be encoded as UTF-8 to support international file names.
    bool GetMovieInfo(const char* pfilename, GFxMovieInfo *pinfo, bool getTagCount = 0, 
                      UInt loadConstants = LoadAll);

    // Loads a movie definition, user must release the movie definition.
    // If LoadImports, LoadBitmaps or LoadFontShapes was not specified,
    // user may need to resolve those components manually.
    // The 'pfilename' should be encoded as UTF-8 to support international file names.
    GFxMovieDef* CreateMovie(const char* pfilename, UInt loadConstants = LoadAll, UPInt memoryArena = 0); 
    

    // Unpins all resources held in the library.
    void            UnpinAll();

    // Cancel all loadings
    void            CancelLoading(); 

    class GFxLoaderImpl* GetLoaderImpl() const { return pImpl; }
private:
     void           InitLoader(const LoaderConfig& config);
};


// ***** GFxSystem Initialization class

// GFxSystem initialization must take place before any other GFx objects, such as the loader
// are created; this is done my calling GFxSystem::Init(). Similarly, GFxSystem::Destroy must be
// called before program exist for proper clenup. Both of these tasks can be achieved by
// simply creating GFxSystem object first, allowing its constructor/destructor do the work.
// Note that if an instance of GFxSystem is created, Init/Destroy calls should not be made
// explicitly.

// One of the main things GFxSystem initialization does is establish the GFx memory heap
// to use for allocations. Before that is done, allocations will fail and/or generate errors.

class GFxSystem : public GSystem
{
public:

    // GFxSystem has two version of constructors, so that you can specify:
    //  - No arguments; uses default sys alloc and root heap.
    //  - Custom GSysAllocPaged implementation with default root heap settings.
    //  - Custom root heap settings with default GSysAllocPaged implementation.
    //  - Both custom root heap settings and custom GSysAllocPaged implementation.
    GFxSystem(GSysAllocBase* psysAlloc = GSYSALLOC_DEFAULT_CLASS::InitSystemSingleton())
        : GSystem(psysAlloc)
    {  }    
    GFxSystem(const GMemoryHeap::HeapDesc& rootHeapDesc,
              GSysAllocBase* psysAlloc = GSYSALLOC_DEFAULT_CLASS::InitSystemSingleton())
        : GSystem(rootHeapDesc, psysAlloc)
    {  }


    ~GFxSystem()
    {  }

    // Functions taken from base:
    //   static void Init(GMemoryHeap* pheap = GMEMORY_HEAP_DEFAULT::CreateRootHeap());
    //   static void Destroy();
};


#ifdef GFC_BUILD_DEBUG
extern "C" int GFx_Compile_without_GFC_BUILD_DEBUG;
#else
extern "C" int GFx_Compile_with_GFC_BUILD_DEBUG;
#endif

GINLINE GFxFileOpenerBase::GFxFileOpenerBase() : GFxState(State_FileOpener)
{
#ifdef GFC_BUILD_DEBUG
    GFx_Compile_without_GFC_BUILD_DEBUG = 0;
#else
    GFx_Compile_with_GFC_BUILD_DEBUG = 0;
#endif
}

#endif
