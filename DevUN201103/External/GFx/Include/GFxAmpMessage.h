/**********************************************************************

Filename    :   GFxAmpMessage.h
Content     :   Messages sent back and forth to AMP
Created     :   December 2009
Authors     :   Alex Mantzaris

Copyright   :   (c) 2005-2009 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

// This file includes a number of different classes
// All of them derive from GFxMessage and represent the types of messages
// sent back and forth between AMP and the GFx application

#ifndef INCLUDE_GFX_AMP_MESSAGE_H
#define INCLUDE_GFX_AMP_MESSAGE_H

class GFxAmpStream;
class GFxAmpMsgHandler;

#include "GRefCount.h"
#include "GString.h"
#include "GFxAmpProfileFrame.h"

// Generic message that includes only the type
// Although not abstract, this class is intended as a base class for concrete messages
class GFxAmpMessage : public GRefCountBase<GFxAmpMessage, GStat_Default_Mem>, 
                        public GListNode<GFxAmpMessage>
{
public:
    virtual ~GFxAmpMessage() { }

    // Called by the message handler when a message of unknown type needs to be handled
    // AcceptHandler in turn calls a message-specific method of the message handler
    // This paradigm allows the message handler to process messages in an object-oriented way
    virtual bool            AcceptHandler(GFxAmpMsgHandler* msgHandler) const;

    // Urgent messages can be handled as they are received, bypassing the queue
    virtual bool            ShouldQueue() const     { return true; }

    // Message serialization methods
    virtual void            Read(GFxAmpStream& str);
    virtual void            Write(GFxAmpStream& str) const;

    // Message printing for debugging
    virtual GString         ToString() const        { return "Unknown message"; }

    // Type comparison for queue filtering
    bool        IsSameType(const GFxAmpMessage& msg) const  { return MsgType == msg.MsgType; }

    // version for backwards-compatibility
    void        SetVersion(UInt32 version)                  { Version = version; }
    UInt32      GetVersion() const                          { return Version; }

    // Static method that reads a stream and creates a message of the appropriate type
    static GFxAmpMessage*   CreateAndReadMessage(GFxAmpStream& str, GMemoryHeap* heap);

    // Version for backwards compatibility
    static UInt32           GetLatestVersion()          { return Version_Latest; }

protected:

    // Message types
    enum MessageType
    {
        Msg_None,
        Msg_Heartbeat,

        // Server to client only
        Msg_Log,
        Msg_CurrentState,
        Msg_ProfileFrame,
        Msg_SwdFile,
        Msg_SourceFile,

        // Client to server only
        Msg_SwdRequest,
        Msg_SourceRequest,
        Msg_AppControl,

        // Broadcast
        Msg_Port,

        // Image
        Msg_ImageRequest,
        Msg_ImageData,
        // Add new types to the end of the list so that backwards compatibility is preserved
    };

    enum VersionType
    {
        Version_Latest = 11
    };

    MessageType                 MsgType;  // for serialization
    UInt32                      Version;

    // Protected constructor because GFxAmpMessage is intended to be used as a base class only
    GFxAmpMessage(MessageType msgType = Msg_None);

    // Static helper that creates a message of the specified type
    static GFxAmpMessage* CreateMessage(MessageType msgType, GMemoryHeap* heap);
};


// Message that includes a text field
// Intended as a base class
class GFxAmpMessageText : public GFxAmpMessage
{
public:

    virtual ~GFxAmpMessageText() { }

    // Serialization
    virtual void        Read(GFxAmpStream& str);
    virtual void        Write(GFxAmpStream& str) const;

    // Accessor
    const GStringLH&    GetText() const;

protected:

    GStringLH           TextValue;

    // Protected constructor because GFxAmpMessageText is intended as a base class
    GFxAmpMessageText(MessageType msgType, const GString& textValue = "");
};

// Message that includes an integer field
// Intended as a base class
class GFxAmpMessageInt : public GFxAmpMessage
{
public:
    virtual ~GFxAmpMessageInt() { }

    // Serialization
    virtual void        Read(GFxAmpStream& str);
    virtual void        Write(GFxAmpStream& str) const;

    // Accessor
    UInt32              GetValue() const;

protected:

    UInt32 BaseValue;

    // Protected constructor because GFxAmpMessageInt is intended as a base class
    GFxAmpMessageInt(MessageType msgType, UInt32 intValue = 0);
};

// Heartbeat message
// Used for connection verification
// A lack of received messages for a long time period signifies lost connection
// Not handled by the message handler, but at a lower level by the thread manager
class GFxAmpMessageHeartbeat : public GFxAmpMessage
{
public:

    GFxAmpMessageHeartbeat();
    virtual ~GFxAmpMessageHeartbeat() { }

    virtual GString         ToString() const        { return "Heartbeat"; }
};

// Log message
// Sent from server to client
class GFxAmpMessageLog : public GFxAmpMessageText
{
public:

    GFxAmpMessageLog(const GString& logText = "", UInt32 logCategory = 0, const UInt64 timeStamp = 0);
    virtual ~GFxAmpMessageLog() { }

    // GFxAmpMessage overrides
    virtual bool            AcceptHandler(GFxAmpMsgHandler* handler) const;
    virtual void            Read(GFxAmpStream& str);
    virtual void            Write(GFxAmpStream& str) const;
    virtual GString         ToString() const        { return "Log"; }

    // Accessors
    UInt32                  GetLogCategory() const;
    const GStringLH&        GetTimeStamp() const;

protected:

    UInt32                  LogCategory;
    GStringLH               TimeStamp;
};

// Message describing AMP current state
// Sent from server to client
// The state is stored in the base class member
class GFxAmpMessageCurrentState : public GFxAmpMessage
{
public:
    GFxAmpMessageCurrentState(const GFxAmpCurrentState* state = NULL);
    virtual ~GFxAmpMessageCurrentState() { }

    // GFxAmpMessage overrides
    virtual bool            AcceptHandler(GFxAmpMsgHandler* handler) const;
    virtual void            Read(GFxAmpStream& str);
    virtual void            Write(GFxAmpStream& str) const;
    virtual bool            ShouldQueue() const     { return false; }
    virtual GString         ToString() const        { return "Current state"; }

    // Accessors
    const GFxAmpCurrentState*   GetCurrentState() const   { return State; }

protected:
    GPtr<GFxAmpCurrentState>    State;
};

// Message describing a single frame's profiling results
// Sent by server to client every frame
class GFxAmpMessageProfileFrame : public GFxAmpMessage
{
public:

    GFxAmpMessageProfileFrame(GPtr<GFxAmpProfileFrame> frameInfo = NULL);
    virtual ~GFxAmpMessageProfileFrame();

    // GFxAmpMessage overrides
    virtual bool                AcceptHandler(GFxAmpMsgHandler* handler) const;
    virtual GString             ToString() const        { return "Frame data"; }
    virtual void                Read(GFxAmpStream& str);
    virtual void                Write(GFxAmpStream& str) const;

    // Data Accessor
    const GFxAmpProfileFrame*   GetFrameInfo() const;

protected:

    GPtr<GFxAmpProfileFrame>    FrameInfo;
};


// Message with the contents of a SWD file
// The contents are not parsed but are sent in the form of an array of bytes
// Sent by server to client upon request
// The base class member holds the corresponding SWF file handle, generated by the server
class GFxAmpMessageSwdFile : public GFxAmpMessageInt
{
public:

    GFxAmpMessageSwdFile(UInt32 swfHandle = 0, UByte* bufferData = NULL, UInt bufferSize = 0, 
                                                                         const char* filename = "");
    virtual ~GFxAmpMessageSwdFile() { }

    // GFxAmpMessage overrides
    virtual bool            AcceptHandler(GFxAmpMsgHandler* handler) const;
    virtual GString         ToString() const        { return "SWD file"; }
    virtual void            Read(GFxAmpStream& str);
    virtual void            Write(GFxAmpStream& str) const;

    // Accessors
    const GArrayLH<UByte>&  GetFileData() const;
    const char*             GetFilename() const;

protected:

    GArrayLH<UByte>         FileData;
    GStringLH               Filename;
};

// Sent by server to client upon request
class GFxAmpMessageSourceFile : public GFxAmpMessage
{
public:

    GFxAmpMessageSourceFile(UInt64 fileHandle = 0, UByte* bufferData = NULL, UInt bufferSize = 0, 
        const char* filename = "");
    virtual ~GFxAmpMessageSourceFile() { }

    // GFxAmpMessage overrides
    virtual bool            AcceptHandler(GFxAmpMsgHandler* handler) const;
    virtual GString         ToString() const        { return "Source file"; }
    virtual void            Read(GFxAmpStream& str);
    virtual void            Write(GFxAmpStream& str) const;

    // Accessors
    UInt64                  GetFileHandle() const;
    const GArrayLH<UByte>&  GetFileData() const;
    const char*             GetFilename() const;

protected:

    UInt64                  FileHandle;
    GArrayLH<UByte>         FileData;
    GStringLH               Filename;
};

// Sent by client to server
// Base class holds the SWF handle
class GFxAmpMessageSwdRequest : public GFxAmpMessageInt
{
public:

    GFxAmpMessageSwdRequest(UInt32 swfHandle = 0, bool requestContents = false);
    virtual ~GFxAmpMessageSwdRequest() { }

    // GFxAmpMessage overrides
    virtual bool            AcceptHandler(GFxAmpMsgHandler* handler) const;
    virtual GString         ToString() const        { return "SWD request"; }
    virtual void            Read(GFxAmpStream& str);
    virtual void            Write(GFxAmpStream& str) const;

    // Accessors
    bool                    IsRequestContents() const;

protected:

    // may just request filename for a given SWF handle, or the entire SWD file
    bool                    RequestContents; 
};

// Sent by client to server
class GFxAmpMessageSourceRequest : public GFxAmpMessage
{
public:

    GFxAmpMessageSourceRequest(UInt64 handle = 0, bool requestContents = false);
    virtual ~GFxAmpMessageSourceRequest() { }

    // GFxAmpMessage overrides
    virtual bool            AcceptHandler(GFxAmpMsgHandler* handler) const;
    virtual GString         ToString() const        { return "Source request"; }
    virtual void            Read(GFxAmpStream& str);
    virtual void            Write(GFxAmpStream& str) const;

    // Accessors
    UInt64                  GetFileHandle() const;
    bool                    IsRequestContents() const;

protected:

    UInt64                  FileHandle;
    bool                    RequestContents; 
};

// Message that controls the GFx application in some way
// Normally sent by client to server
// Server sends this to client to signify supported capabilities
// Control information stored as bits in the base class member
class GFxAmpMessageAppControl : public GFxAmpMessageInt
{
public:

    GFxAmpMessageAppControl(UInt32 flags = 0);
    virtual ~GFxAmpMessageAppControl() { }

    // GFxAmpMessage overrides
    virtual bool            AcceptHandler(GFxAmpMsgHandler* handler) const;
    virtual GString         ToString() const        { return "App control"; }
    virtual void            Read(GFxAmpStream& str);
    virtual void            Write(GFxAmpStream& str) const;

    // Accessors
    bool                    IsToggleWireframe() const;
    void                    SetToggleWireframe(bool wireframeToggle);
    bool                    IsTogglePause() const;
    void                    SetTogglePause(bool pauseToggle);
    bool                    IsToggleAmpRecording() const;
    void                    SetToggleAmpRecording(bool recordingToggle);
    bool                    IsToggleOverdraw() const;
    void                    SetToggleOverdraw(bool overdrawToggle);
    bool                    IsToggleInstructionProfile() const;
    void                    SetToggleInstructionProfile(bool instToggle);
    bool                    IsToggleFastForward() const;
    void                    SetToggleFastForward(bool ffToggle);
    bool                    IsToggleAaMode() const;
    void                    SetToggleAaMode(bool aaToggle);
    bool                    IsToggleStrokeType() const;
    void                    SetToggleStrokeType(bool strokeToggle);
    bool                    IsRestartMovie() const;
    void                    SetRestartMovie(bool restart);
    bool                    IsNextFont() const;
    void                    SetNextFont(bool next);
    bool                    IsCurveToleranceUp() const;
    void                    SetCurveToleranceUp(bool up);
    bool                    IsCurveToleranceDown() const;
    void                    SetCurveToleranceDown(bool down);
    bool                    IsForceInstructionProfile() const;
    void                    SetForceInstructionProfile(bool instProf);
    bool                    IsDebugPause() const;
    void                    SetDebugPause(bool debug);
    bool                    IsDebugStep() const;
    void                    SetDebugStep(bool debug);
    bool                    IsDebugStepIn() const;
    void                    SetDebugStepIn(bool debug);
    bool                    IsDebugStepOut() const;
    void                    SetDebugStepOut(bool debug);
    bool                    IsDebugNextMovie() const;
    void                    SetDebugNextMovie(bool debug);
    bool                    IsToggleMemReport() const;
    void                    SetToggleMemReport(bool toggle);
    const GStringLH&        GetLoadMovieFile() const;
    void                    SetLoadMovieFile(const char* fileName);

protected:

    enum OptionFlags
    {
        OF_ToggleWireframe              = 0x00000001,
        OF_TogglePause                  = 0x00000002,
        OF_ToggleAmpRecording           = 0x00000004,
        OF_ToggleOverdraw               = 0x00000008,
        OF_ToggleInstructionProfile     = 0x00000010,
        OF_ToggleFastForward            = 0x00000020,
        OF_ToggleAaMode                 = 0x00000040,
        OF_ToggleStrokeType             = 0x00000080,
        OF_RestartMovie                 = 0x00000100,
        OF_NextFont                     = 0x00000200,
        OF_CurveToleranceUp             = 0x00000400,
        OF_CurveToleranceDown           = 0x00000800,
        OF_ForceInstructionProfile      = 0x00001000,
        OF_DebugPause                   = 0x00002000,
        OF_DebugStep                    = 0x00004000,
        OF_DebugStepIn                  = 0x00008000,
        OF_DebugStepOut                 = 0x00010000,
        OF_DebugNextMovie               = 0x00020000,
        OF_ToggleMemReport              = 0x00040000
    };

    GStringLH               LoadMovieFile;
};

// Message containing the server listening port
// Broadcast via UDP
// The server IP address is known from the origin of the UDP packet
class GFxAmpMessagePort : public GFxAmpMessageInt
{
public:
    enum PlatformType
    {
        PlatformOther = 0,
        PlatformWindows,
        PlatformXbox360,
        PlatformPs3,
        PlatformWii,
        PlatformIphone,
        PlatformMac,
        PlatformLinux,
        PlatformPsp,
        PlatformPs2,
        PlatformXbox,
        PlatformGamecube,
        PlatformQnx,
        PlatformSymbian,
    };

    GFxAmpMessagePort(UInt32 port = 0, const char* appName = NULL, const char* FileName = NULL);
    virtual ~GFxAmpMessagePort() { }

    // GFxAmpMessage overrides
    virtual bool        AcceptHandler(GFxAmpMsgHandler* handler) const;
    virtual void        Read(GFxAmpStream& str);
    virtual void        Write(GFxAmpStream& str) const;

    // Accessors
    PlatformType        GetPlatform() const;
    const GStringLH&    GetAppName() const;
    const GStringLH&    GetFileName() const;

protected:
    PlatformType        Platform;
    GStringLH           AppName;
    GStringLH           FileName;
};

// Sent by client to server
// Base class holds the Image ID
class GFxAmpMessageImageRequest : public GFxAmpMessageInt
{
public:
    GFxAmpMessageImageRequest(UInt32 imageId = 0);
    virtual ~GFxAmpMessageImageRequest() { }

    // GFxAmpMessage overrides
    virtual bool            AcceptHandler(GFxAmpMsgHandler* handler) const;
    virtual GString         ToString() const        { return "Image request"; }
};

// Message with the contents of an image
// Sent by server to client upon request
// The base class member holds the corresponding image ID
class GFxAmpMessageImageData : public GFxAmpMessageInt
{
public:
    GFxAmpMessageImageData(UInt32 imageId = 0, UByte* bufferData = NULL, UInt bufferSize = 0);
    virtual ~GFxAmpMessageImageData() { }

    // GFxAmpMessage overrides
    virtual bool            AcceptHandler(GFxAmpMsgHandler* handler) const;
    virtual GString         ToString() const        { return "Image data"; }
    virtual void            Read(GFxAmpStream& str);
    virtual void            Write(GFxAmpStream& str) const;

    // Accessors
    const GArrayLH<UByte>&  GetImageData() const;

protected:
    GArrayLH<UByte>         ImageData;
};


#endif

