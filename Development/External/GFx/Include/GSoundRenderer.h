/**********************************************************************

Filename    :   GSoundRenderer.h
Content     :   Sound Player/Driver interface
Created     :   January 23, 2007
Authors     :   Andrew Reisse

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GSOUNDPLAYER_H
#define INC_GSOUNDPLAYER_H

#include "GConfig.h"
#ifndef GFC_NO_SOUND

#include "GTypes.h"
#include "GRefCount.h"
#include "GArray.h"

#include "GSound.h"

// ***** Declared Classes
class   GSoundRenderer;
class   GSoundSample;
class   GSoundInfoBase;
class   GSoundInfo;


// ***** GSoundSample

// GSoundSample represents a soundrenderer-specific version of a sound that should be
// associated with an instance of GSoundDataBase class which loaded from a swf file

// An instance of this call is created by calling to one of the CreateSample... methods
// of GSoundRenderer class. To play a sound sample an instance of GSoundChannel should be 
// created by calling to PlaySample method of GSoundRenderer. There could be multiple 
// instances of GSoundChannel class associated with the same instance of GSoundSample class

// This class has a custom refcount implementation because care should be taken when is 
// detached from GSoundRenderer in the multi-threaded environment.

class GSoundSample : public GNewOverrideBase<GStat_Sound_Mem>
{
public:
    GSoundSample()            { RefCount.Value = 1; }
    virtual ~GSoundSample()   {}

    // Obtains the renderer that created this sound sample
    // GetRenderer can return 0 if the renderer has been released, in that
    // case surviving GSoundSample object should be just a stub
    virtual GSoundRenderer*  GetSoundRenderer() const                          = 0;
    virtual bool             IsDataValid() const                               = 0;

    // Obtains the sound duration
    virtual Float            GetDuration() const                               = 0;
    // Returns the size, in bytes
    virtual SInt             GetBytesTotal() const                             = 0;
    // Returns the number of bytes loaded 
    virtual SInt             GetBytesLoaded() const                            = 0;

private:
    // Refcount implementation
    GAtomicInt<SInt32>     RefCount;
public:
    
    inline void        AddRef() { RefCount.Increment_NoSync(); }  
    inline void        Release()
    {
        if ((RefCount.ExchangeAdd_Acquire(-1) - 1) == 0)
            delete this;
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
};


// ***** GSoundChannel

// GSoundChannel represents an active (playing) sound and is used to control
// its attributes like volume level, status (paused, stopped).

// It is created by calling to PlaySample or AttachAuxStreamer methods of 
// GSoundRenderer class. 

class GSoundChannel : public GRefCountBase<GSoundChannel,GStat_Sound_Mem>
{
public:
    // Sound volume transformation descriptor
    struct Transform
    {
        Float Position;     // Position in seconds
        Float LeftVolume;   // Volume for left channel (0.0..1.0)
        Float RightVolume;  // Volume for right channel (0.0..1.0)
    };

    // Sound spatial position, orientation, velocity, etc.
    struct Vector
    {
        Float X, Y, Z;      // 3D vector
        Vector(): X(0.0f), Y(0.0f), Z(0.0f) {}
    };

    GSoundChannel() {}

    // Obtains the renderer that created this sound channel
    // GetRenderer can return 0 if this channel has not been associated with any 
    // sound renderers (in case if a separate sound system is used to play a sound
    // track from the video file)
    virtual GSoundRenderer*  GetSoundRenderer() const                          = 0;
    // Obtains the sound sample that was used to created this sound channel
    // GetSample can return 0 if this channel has not been associated with any 
    // sound samples (in case if a separate sound system is used to play a sound
    // track from the video file)
    virtual GSoundSample*    GetSample() const                                 = 0;

    // Stop the active sound. After calling to this method calls to all methods of 
    // object should not changed the state and attributes of this object.
    virtual void           Stop()                                              = 0;
    // Pause/resume the active sound
    virtual void           Pause(bool pause)                                   = 0;
    // Check if the active sound has been stopped
    virtual bool           IsPlaying() const                                   = 0;
    // Set the playing position in seconds
    virtual void           SetPosition(Float seconds)                          = 0;
    // Get the playing position of the active sound 
    virtual Float          GetPosition()                                       = 0;
    // Set the sound's loop attributes (number of loops, start and end position in seconds)
    virtual void           Loop(SInt count, Float start = 0, Float end = 0)    = 0;
    // Get the current volume level (0.0 ... 1.0)
    virtual Float          GetVolume()                                         = 0;
    // Set the current volume level (0.0 ... 1.0)
    virtual void           SetVolume(Float volume)                             = 0;
    // Get the current pan (-1.0 ... 0 ... 1.0 )
    virtual Float          GetPan()                                            = 0;
    // Set the current pan (-1.0 ... 0 ... 1.0)
    virtual void           SetPan(Float volume)                                = 0;
    // Set a volume level transformation vector 
    virtual void           SetTransforms(const GArray<Transform>& transforms)  = 0;
    // Sets sound spatial information such as position, orientation and velocity
    virtual void           SetSpatialInfo(const GArray<Vector> spatinfo[])     = 0;
};

// ***** GSoundRenderer

// GSoundRenderer is a base sound renderer class used for playing all sounds in 
// a Flash file; it is essentially a low level interface to the hardware.

class GSoundRenderer : public GRefCountBase<GSoundRenderer,GStat_Sound_Mem>
{
public:

    enum SoundRendererCapBits
    {
        Cap_NoMP3        = 0x00000001,  // SoundRenderer CAN NOT play MP3 compressed sound data
        Cap_NoVideoSound = 0x00000002,  // SoundRenderer DOES NOT support video sound playback
        Cap_NoStreaming  = 0x00000004   // SoundRenderer DOES NOT support streaming sound
    };

    // This interface is used to access the audio for a video source
    class AuxStreamer : public GRefCountBase<AuxStreamer,GStat_Sound_Mem>
    {
    public:
        // Specifies the format of PCM data which a streamer will be providing.
        enum PCMFormat
        {
            PCM_SInt16,
            PCM_Float
        };
        virtual ~AuxStreamer() {}
        // This method is called then needs the next chunk of audio data from
        // a video source.
        virtual UInt GetPCMData(UByte* pdata, UInt datasize) = 0;
        // Set the position (in seconds) in the sound streamer
        virtual bool SetPosition(Float /*seconds*/) { return false; }       
    };

    GSoundRenderer() { }
    virtual ~GSoundRenderer() {}

    // Helper function to query sound renderer capabilities.
    virtual bool        GetRenderCaps(UInt32 *pcaps)                          = 0;

    // Returns created objects with a refCount of 1, must be user-released. 
    // The created sound object is associated with the sound data which
    // comes from a separate file (mp3, wav). 
    // streaming parameters specifies if a file should be completely loaded
    // before it starts playing.
    virtual GSoundSample*   CreateSampleFromFile(const char* /* fname */, 
                                                 bool /* streaming */)         = 0;

    // Convenience creation functions, create & initialize sample at the same time
    // The created sound sample object is associated with the sound data which is 
    // kept in swf resource library in was loaded during swf file loading process 
    virtual GSoundSample*    CreateSampleFromData(GSoundDataBase* psd) = 0;

    // Returns created objects with a refCount of 1, must be user-released. 
    // The created sound associated with the audio stream of a video source. 
    virtual GSoundSample*    CreateSampleFromAuxStreamer(AuxStreamer*            /* pStreamder */, 
                                                         UInt32                  /* channels */, 
                                                         UInt32                  /* samplerate */,
                                                         AuxStreamer::PCMFormat  /* fmt */) { return NULL; }

    // Create a sound channel object form a given sound sample object.
    // if paused parameter is set to false the sound will not start playing
    // until GSoundChannel::Paused(false) method is called.
    virtual GSoundChannel*  PlaySample(GSoundSample* ps, bool paused)  = 0;

    // Get the master volume level for the sound renderer
    virtual Float           GetMasterVolume()                                   = 0;
    // Set the master volume level for all sound created by this renderer
    virtual void            SetMasterVolume(Float volume)                       = 0;
    
    // Updates internal sound renderer systems. This method should be called periodically
    // in the game main loop
    // Returns time remaining till next Update must be called, in seconds.
    virtual Float           Update()                                            = 0;
    // Mutes/unmutes all sounds playing by the sound renderer. 
    virtual void            Mute(bool mute)                                     = 0;

};

// ***** GSoundInfoBase

// GSoundInfoBase represents a sound within GFxPlayer

class GSoundInfoBase : public GRefCountBaseNTS<GSoundInfoBase,GStat_Sound_Mem>
{
public:
    virtual UInt            GetLength() const                    = 0;
    virtual UInt            GetRate() const                      = 0;

    // Obtains the SoundSample pointer from the data,
    // for a given sound renderer. Optionally create the SoundSample
    virtual GSoundSample*   GetSoundSample(GSoundRenderer* prenderer)    = 0;

    // SetSoundData is a part of the interface but it does not need to be implemented
    // in versions of GSoundInfoBase which do not support initialization with a SoundData.
    // If supported, the class can AddRef to the Sound to keep its data.
    // The default '.gfx' file loading procedure will use this method to
    // initialize Sound contents after the base file has been loaded.
    virtual bool        SetSoundData(GSoundDataBase* pSound)
    { GUNUSED(pSound); return 0; }

    // Returns true, if the instance can be played.
    virtual bool        IsPlayable() const { return false; }

    virtual void        ReleaseResource() = 0;

};

// ***** GSoundInfo

// GSoundInfoBase version that keeps sound sample data within allocated GSoundData object.

class GSoundInfo : public GSoundInfoBase
{
protected:
    // Source Sound data for the SoundSample
    GPtr<GSoundDataBase>    pSound;
    GPtr<GSoundSample>      pSoundSample;

public:

    explicit GSoundInfo(GSoundDataBase* psound = 0) : pSound(psound) { }
    virtual ~GSoundInfo();

    // Simple accessors
    GSoundDataBase*   GetSound() const { return pSound; }

    // GSoundInfoBase Implementation
    UInt              GetLength() const { return pSound->GetSampleCount(); }
    UInt              GetRate() const; 

    GSoundSample*     GetSoundSample(GSoundRenderer* prenderer);    
    bool              SetSoundData(GSoundDataBase* pSound);

    virtual bool      IsPlayable() const { return pSound.GetPtr() != 0; }

    virtual void      ReleaseResource();

};

#endif // GFC_NO_SOUND

#endif
