/**********************************************************************

Filename    :   GFxAmpServer.h
Content     :   Class encapsulating communication with AMP
                Embedded in application to be profiled
Created     :   December 2009
Authors     :   Alex Mantzaris

Copyright   :   (c) 2005-2009 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INCLUDE_FX_AMP_SERVER_H
#define INCLUDE_FX_AMP_SERVER_H

// AMP server states
enum AmpServerStateType
{
    Amp_Default =                   0x0000,
    Amp_Paused =                    0x0001,
    Amp_Disabled =                  0x0002,
    Amp_ProfileInstructions =       0x0004,
    Amp_RenderOverdraw =            0x0008,
    Amp_InstructionSampling =       0x0010,
    Amp_MemoryReports =             0x0020,
    Amp_App_Paused =                0x1000,
    Amp_App_Wireframe =             0x2000,
    Amp_App_FastForward =           0x4000,
};

#define GFX_AMP_BROADCAST_PORT 7533

#ifdef GFX_AMP_SERVER

#include "GFxAmpMessage.h"
#include "GFxLog.h"
#include "GFxAmpInterfaces.h"
#include "GRefCount.h"
#include "GFxAmpProfileFrame.h"
#include "GRenderer.h"

class GString;
class GFxAmpThreadMgr;
class GFxLoader;
class GFxAmpAppControlInterface;
class GFxAmpSendThreadCallback;
class GFxAmpStatusChangedCallback;
class GFxMovieRoot;
class GAmpRenderer;
class GFxAmpViewStats;
class GFxSocketImplFactory;

// GFxAmpServer encapsulates the communication of a GFx application with an AMP client
// It is a singleton so that it can be easily accessed throughout GFx
// The object has several functions:
// - it keeps track of statistics per frame
// - it manages the socket connection with the client
// - it handles received messages
// The server needs to be thread-safe, as it can be accessed from anywhere in GFx
class GFxAmpServer : public GFxAmpMsgHandler
{
public:
    // Singleton accessor
    static GFxAmpServer&    GetInstance();

    // Singleton initialization and uninitialization
    // Called from GSystem
    static void             Init(UPInt arena = 0, GSysAllocPaged* arenaAllocator = NULL);
    static void             Uninit();

    // Connection status
    bool        OpenConnection();
    void        CloseConnection();
    bool        IsValidConnection() const;
    bool        IsState(AmpServerStateType state) const;
    void        SetState(AmpServerStateType state, bool stateValue);
    void        ToggleAmpState(UInt32 toggleState);
    UInt32      GetCurrentState() const;
    void        SetConnectedApp(const char* playerTitle);
    void        SetAaMode(const char* aaMode);
    void        SetStrokeType(const char* strokeType);
    void        SetCurrentLocale(const char* locale);
    void        SetCurveTolerance(float tolerance);
    void        UpdateState(const GFxAmpCurrentState* state);
    bool        IsEnabled() const               { return !IsState(Amp_Disabled); }
    bool        IsPaused() const                { return IsState(Amp_Paused); }
    bool        IsProfiling() const;
    bool        IsInstructionProfiling() const  { return IsState(Amp_ProfileInstructions); }
    bool        IsInstructionSampling() const   { return IsState(Amp_InstructionSampling); }
    bool        IsMemReports() const            { return IsState(Amp_MemoryReports); }
    void        WaitForAmpConnection(UInt maxDelayMilliseconds = GFC_WAIT_INFINITE);
    void        SetAppControlCaps(const GFxAmpMessageAppControl* caps);

    // Configuration options
    void        SetListeningPort(UInt32 port);
    void        SetBroadcastPort(UInt32 port);
    void        SetConnectionWaitTime(UInt waitTimeMilliseconds);
    void        SetHeapLimit(UPInt memLimit);
    void        SetInitSocketLib(bool initSocketLib);
    void        SetSocketImplFactory(GFxSocketImplFactory* socketImplFactory);

    // AdvanceFrame needs to be called once per frame
    // It is called from GRenderer::EndFrame
    void        AdvanceFrame();

    // Custom callback that handles application-specific messages
    void        SetAppControlCallback(GFxAmpAppControlInterface* callback);

    // Message handler
    bool        HandleNextMessage();

    // Specific messages
    void        SendLog(const char* message, int messageLength, GFxLogConstants::LogMessageType msgType);
    void        SendCurrentState();
    void        SendAppControlCaps();

    // GFxAmpMsgHandler interface implementation
    bool        HandleAppControl(const GFxAmpMessageAppControl* message);
    bool        HandleSwdRequest(const GFxAmpMessageSwdRequest* message);
    bool        HandleSourceRequest(const GFxAmpMessageSourceRequest* message);
    bool        HandleImageRequest(const GFxAmpMessageImageRequest* message);
    bool        IsInitSocketLib() const { return InitSocketLib; }

    // AMP keeps track of active Movie Views
    void        AddMovie(GFxMovieRoot* movie);
    void        RemoveMovie(GFxMovieRoot* movie);
    void        RefreshMovieStats();

    // AMP keeps track of active Loaders (for renderer access)
    void        AddLoader(GFxLoader* loader);
    void        RemoveLoader(GFxLoader* loader);

    // AMP renderer is used to render overdraw
    void        AddAmpRenderer(GAmpRenderer* renderer);

    // AMP server generates unique handles for each SWF
    // The 16-byte SWD debug ID is not used because it is not appropriate as a hash key
    // AMP keeps a map of SWD handles to actual SWD ID and filename
    UInt32      GetNextSwdHandle() const;
    UInt32      AddSwf(const char* swdId, const char* filename);
    GString     GetSwdId(UInt32 handle) const;
    GString     GetSwdFilename(UInt32 handle) const;

    GString     GetSourceFilename(UInt64 handle) const;
    void        AddSourceFile(UInt64 fileHandle, const char* fileName);

    // Increments glyph raster cache texture updates 
    void        IncrementFontCacheTextureUpdate()       { ++NumFontCacheTextureUpdates; }

private:

    // Struct that holds loaded SWF information
    struct SwdInfo : public GRefCountBase<SwdInfo, GStat_Default_Mem>
    {
        GStringLH SwdId;
        GStringLH Filename;
    };
    typedef GHashLH<UInt32, GPtr<SwdInfo> > SwdMap;

    // Struct that holds AS file information
    struct SourceFileInfo : public GRefCountBase<SourceFileInfo, GStat_Default_Mem>
    {
        GStringLH Filename;
    };
    typedef GHashLH<UInt64, GPtr<SourceFileInfo> > SourceFileMap;

    struct ViewStats : public GRefCountBase<ViewStats, GStat_Default_Mem>
    {
        ViewStats(GFxMovieRoot* movie);
        void CollectStats(GFxAmpProfileFrame* frameProfile, UPInt index);
        void ClearStats();
        GPtr<GFxAmpViewStats>   DisplayStats;
        GPtr<GFxAmpViewStats>   AdvanceStats;
    };

    // Member variables
    GFxAmpCurrentState                          CurrentState;   // Paused, Disabled, etc
    mutable GLock                               CurrentStateLock;
    UInt32                                      ToggleState;   // Paused, Disabled, etc
    mutable GLock                               ToggleStateLock;
    UInt32                                      Port;           // For socket connection to client
    UInt32                                      BroadcastPort;  // For broadcasting IP address to AMP
    GPtr<GFxAmpThreadMgr>                       ThreadMgr;      // Socket threads
    mutable GLock                               ConnectionLock;
    mutable GLock                               FrameDataLock;
    GArrayLH<GFxMovieRoot*>                     Movies;
    bool                                        CloseConnectionOnUnloadMovies;
    GArrayLH< GPtr<ViewStats> >                 MovieStats;
    mutable GLock                               MovieLock;
    GArrayLH<GFxLoader*>                        Loaders;
    GArrayLH<GAmpRenderer*>                     AmpRenderers;
    mutable GLock                               LoaderLock;
    SwdMap                                      HandleToSwdIdMap; // Map of unique handle to SWF information
    mutable GLock                               SwfLock;
    SourceFileMap                               HandleToSourceFileMap; 
    mutable GLock                               SourceFileLock;
    GEvent                                      ConnectedEvent; // Used to suspend GFx until connected to AMP
    GEvent                                      SendingEvent; // Used to suspend GFx until message queue is empty
    UInt32                                      NumFontCacheTextureUpdates;
    UInt                                        ConnectionWaitDelay;  // milliseconds
    bool                                        InitSocketLib;  // Initialize socket library?
    GFxSocketImplFactory*                       SocketImplFactory;

    // Handles app requests from AMP client
    GFxAmpAppControlInterface*                  AppControlCallback;
    // Callback from Send Thread
    GPtr<GFxAmpSendThreadCallback>              SendThreadCallback;
    // Callback for connection status change
    GPtr<GFxAmpStatusChangedCallback>           StatusChangedCallback;

    // Caps so that Client knows what app control functionality is supported
    GPtr<GFxAmpMessageAppControl>               AppControlCaps;

    // private constructor 
    // Create singleton with GFxAmpServer::Init
    GFxAmpServer();
    ~GFxAmpServer();

    // Internal helper methods
    bool        IsSocketCreated() const;

    void        CollectMemoryData(GFxAmpProfileFrame* frameProfile);
    void        CollectMovieData(GFxAmpProfileFrame* frameProfile);
    void        ClearMovieData();
    void        CollectRendererData(GFxAmpProfileFrame* frameProfile);
    void        ClearRendererData();
    void        SendFrameStats();
    void        CollectRendererStats(GFxAmpProfileFrame* frameProfile, const GRenderer::Stats& stats);
    void        WaitForAmpThread(GFxAmpViewStats* viewStats, UInt32 waitMillisecs);

    typedef GStringHash< GArray< GPtr<GFxResource> > > HeapResourceMap;
    typedef GStringHash< GArray<GString> > FontResourceMap;
    void        CollectImageData(HeapResourceMap* resourceMap, FontResourceMap* fontMap);

    // Static variables
    static GFxAmpServer*    AmpServerSingleton;     // the singleton
};

#endif  // GFX_AMP_SERVER

#endif  // INCLUDE_FX_AMP_SERVER_H

