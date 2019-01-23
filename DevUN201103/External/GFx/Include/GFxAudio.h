/**********************************************************************

Filename    :   GFxAudio.h
Content     :   

Created     :   September 2008
Authors     :   Maxim Didenko

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFxAudio_H
#define INC_GFxAudio_H

#include "GConfig.h"
#ifndef GFC_NO_SOUND

#include "GFxLoader.h"
#include "GSoundRenderer.h"

class GASGlobalContext;
class GASStringContext;
class GFxSoundTagsReader;

//
// ***** GFxAudioBase
//
// The purpose of the state it to provide the instance of GSoundRenderer to the 
// rest of GFx library and to set synchronization parameters for SWF streaming sound.
//
// GFxAudioBase is an abstract class separate from GFxAudio to avoid audio linking
// for projects that don't use built-in audio.
//
class GFxAudioBase : public GFxState
{
public:

    enum SyncTypeFlags
    {       
        NoSync               = 0x00,         // Audio and video frames do not need to be synchronized.
        VideoMaster          = 0x01,         // Audio frames should be synchronized to video frames.
        AudioMaster          = 0x02          // Video frames should be synchronized to audio frames (SWF behavior).
    };

    GFxAudioBase(Float maxTimeDiffernce = 100/1000.f, 
        UInt checkFrameInterval = 15, SyncTypeFlags syncType = AudioMaster) 
        : GFxState(State_Audio), MaxTimeDifference(maxTimeDiffernce), CheckFrameInterval(checkFrameInterval),
        SyncType(syncType) {}
    virtual ~GFxAudioBase() {}

    inline void             SetMaxTimeDifference(Float maxTimeDifference)  { MaxTimeDifference = maxTimeDifference; }
    inline Float            GetMaxTimeDifference() const                   { return MaxTimeDifference; }

    inline void             SetCheckFrameInterval(UInt checkFrameInterval) { CheckFrameInterval = checkFrameInterval; }
    inline UInt             GetCheckFrameInterval() const                  { return CheckFrameInterval; }

    inline void             SetSyncType(SyncTypeFlags syncType)            { SyncType = syncType; }
    inline SyncTypeFlags    GetSyncType() const                            { return SyncType; }

    virtual GSoundRenderer* GetRenderer() const = 0;

    // Internally used APIs.
    virtual GFxSoundTagsReader* GetSoundTagsReader() const  = 0;
    virtual void               RegisterASClasses(GASGlobalContext& gc, GASStringContext& sc) = 0;

protected:

    Float              MaxTimeDifference;       // max allowed difference between video frame time and audio frame time
    UInt               CheckFrameInterval;      // how often the sync check should be performed
    SyncTypeFlags      SyncType;

};


// ***** GFxAudio
//
// The implementation of GFxAudioBase interface
//
// A separate implementation is provided so that Sound classes can be compiled
// out completely from GFx core and the resultant application executable
//
//
class GFxAudio : public GFxAudioBase
{ 
public:

    GFxAudio(GSoundRenderer* pplayer, Float maxTimeDiffernce = 100/1000.f, 
             UInt checkFrameInterval = 15, SyncTypeFlags syncType = AudioMaster);
    ~GFxAudio();

    virtual GSoundRenderer*    GetRenderer() const;

    virtual GFxSoundTagsReader* GetSoundTagsReader() const;

    virtual void               RegisterASClasses(GASGlobalContext& gc, GASStringContext& sc);

protected:

    GPtr<GSoundRenderer> pPlayer;
    GFxSoundTagsReader*  pTagsReader;

};


inline void  GFxStateBag::SetAudio(GFxAudioBase* ptr) 
{ 
    SetState(GFxState::State_Audio, ptr); 
}
inline GPtr<GFxAudioBase>  GFxStateBag::GetAudio() const 
{ 
    return *(GFxAudioBase*) GetStateAddRef(GFxState::State_Audio); 
}
inline GSoundRenderer*  GFxStateBag::GetSoundRenderer() const
{
    GPtr<GFxAudioBase> p = GetAudio();
    return p ? p->GetRenderer() : 0;
}

#endif // GFC_NO_SOUND

#endif
