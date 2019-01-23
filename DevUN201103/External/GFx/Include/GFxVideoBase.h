/**********************************************************************

Filename    :   GFxVideoBase.h
Content     :   GFxVideoBase interface
Created     :   June 2008
Authors     :   Maxim Didenko, Vladislav Merker

Copyright   :   (c) 2008-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXVIDEOPLAYERSTATE_H
#define INC_GFXVIDEOPLAYERSTATE_H

#include "GConfig.h"
#ifndef GFC_NO_VIDEO

#include "GMemory.h"
#include "GFxLoader.h"

class GASGlobalContext;
class GASStringContext;
class GFxLoadProcess;
class GFxSprite;

class GASObject;
class GFxVideoPlayer;


//////////////////////////////////////////////////////////////////////////
//

// GFxVideoBase interface has four responsibilities:
// 1. Create an instance of a GFxVideoPlayer interface when GFx library
//    requires a video play back. Each video requires its own GFxVideoPlayer
// 2. Attach the audio from the video source to a movieclip object
// 3. Register all Action Script objects that are required for video playback
// 4. Read video stream tags from a Flash file.

// Also, this interface provides two ways for implementing background loading of 
// game data while a video file is playing.
// 1. Using a callback object which will notify a user when a video system needs 
//    to read a new chunk of video data and when the read operation has finished.
// 2. Using IsIORequired method to detect if the reading of video data is required 
//    and EnbaleIO method to disable/enable video read operations.

class GFxVideoBase : public GFxState
{
public:
    GFxVideoBase();
    virtual ~GFxVideoBase();

    // Create an instance of a video player.
    virtual GFxVideoPlayer* CreateVideoPlayer(GMemoryHeap*, GFxTaskManager*, 
                                              GFxFileOpenerBase*, GFxLog*) = 0;

    // Callback interface for notifying the application then video a read operation
    // is required and when it has finished.
    class ReadCallback : public GRefCountBase<ReadCallback, GStat_Video_Mem>
    {
    public:
        virtual ~ReadCallback() {}

        // Notify then a video read operation is required. Upon receiving this notification
        // the application should stop all its disk IO operations (and wait until they are really
        // finished). After returning form this method the video system will start file read 
        // operation immediately.
        virtual void OnReadRequested() = 0;
        // Notify then the video read operation has finished. Upon receiving this notification 
        // the application can resume its disk IO operations until it receives OnReadRequested 
        // notification.
        virtual void OnReadCompleted() = 0;
    };
    // Set a read call back instance to the video system.
    virtual void            SetReadCallback(ReadCallback*) = 0;

    // Query a video system if it need to perform any data read operation
    // as soon as this function returns true the application should immediately
    // stop all its disk IO operations and enable video read operations.
    virtual bool            IsIORequired() const  = 0;
    // Enable/Disable video read operations.
    virtual void            EnableIO(bool enable) = 0;

    // Attach the audio from the video source (NetStream AS call) to a movieclip
    // which holds this video source.
    virtual void            AttachAudio(GASObject* psource, GFxSprite* ptarget) = 0;

    virtual void            RegisterASClasses(GASGlobalContext& gc, GASStringContext& sc) = 0;
    virtual void            ReadDefineVideoStreamTag(GFxLoadProcess* p, const GFxTagInfo& tagInfo) = 0;

    // Get a pointer to the memory heap used by video system.
    virtual GMemoryHeap*    GetHeap() const = 0;
};


//////////////////////////////////////////////////////////////////////////
//

inline void  GFxStateBag::SetVideo(GFxVideoBase* ptr) 
{ 
    SetState(GFxState::State_Video, ptr); 
}
inline GPtr<GFxVideoBase> GFxStateBag::GetVideo() const
{ 
    return *(GFxVideoBase*) GetStateAddRef(GFxState::State_Video); 
}

#endif // GFC_NO_VIDEO

#endif // INC_GFXVIDEOPLAYER_H

