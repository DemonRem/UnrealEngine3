/**********************************************************************

Filename    :   GSound.h
Content     :   Sound samples
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


#ifndef INC_GSOUND_H
#define INC_GSOUND_H

#include "GConfig.h"
#ifndef GFC_NO_SOUND

#include "GTypes.h"
#include "GRefCount.h"

// 
// **** GSoundDataBase
//
// The base class for representing sound data in GFx.
// This class does not hold any sound data nor it provides a pointer
// to the sound data because its derivatives can hold only a file name of
// a file with sound data
//
class GSoundDataBase : public GRefCountBaseNTS<GSoundDataBase,GStat_Sound_Mem>
{
public:
    enum SampleFormat
    {
        Sample_None      = 0,
        Sample_8         = 0x001,  // each sample takes 8 bit
        Sample_16        = 0x002,  // each sample takes 16 bit
        Sample_Stereo    = 0x008,  // stereo sound data
        Sample_PCM       = 0x100,  // uncompressed PCM sound data 
        Sample_MP3       = 0x200,  // compressed MP3 sound data
        Sample_ADPCM     = 0x300,  // compressed ADPCM sound data
        Sample_Format    = 0x700,
        Sample_Stream    = 0x1000, // streaming sound data
        Sample_File      = 0x2000  // sound data comes from a file
    };

    GSoundDataBase(UInt format, UInt rate, UInt length) : Format(format), Rate(rate), Length(length), SeekSample(0) {}

    inline UInt      GetSampleSize() const      { return (Format & 0x7) * ((Format & Sample_Stereo) ? 2 : 1); }
    inline bool      IsDataCompressed() const   { return Format >= Sample_MP3; }
    inline UInt      GetFormat() const          { return Format; }
    inline UInt      GetChannelNumber() const   { return Format & Sample_Stereo ? 2 : 1; }

    // size of one uncompressed sample
    inline UInt      GetRate() const            { return Rate; }
    inline UInt      GetRawSize() const         { return Length * GetSampleSize(); }
    inline Float     GetDuration() const        { return Float(Length) / Float(Rate); }

    inline bool      IsStreamSample() const     { return (Format & Sample_Stream) != 0; }
    inline bool      IsFileSample() const       { return (Format & Sample_File) != 0; }
    inline UInt      GetSampleCount() const     { return Length; }

    inline void      SetSeekSample(UInt sample) { SeekSample = sample; }
    inline UInt      GetSeekSample() const      { return SeekSample; }

protected:
    UInt               Format;     // format flags
    UInt               Rate;       // sample rate
    UInt               Length;     // number of samples
    UInt               SeekSample; // number of samples that should be skipped from the beginning 
                                   // of the sound data when it stars playing

};

//
// ***** GSoundData class
//
// This class holds sound data that was loaded from a SWF file
// It is used for SWF event sound where data is loaded all at once.
//
class GSoundData : public GSoundDataBase
{
public:

    // Base constructor
    GSoundData(UInt format, UInt rate, UInt lenght, UInt dsize);
    ~GSoundData();

    // returns a pointer to the sound data
    UByte*           GetData() const     { return pData; }
    // returns the sound data size
    UInt             GetDataSize() const { return DataSize; }
    // returns the size of the buffer what was allocated for the sound data
    //UInt             GetBuffSize() const { return DataSize; }

protected:

    UByte*             pData;
    UInt               DataSize;
};

//
// ***** GAppendableSoundData class
//
// This class is used for representing streaming sound data from a SWF file
// This type of a SWF sound is not loaded all at once. It comes in chunks during 
// loading of swf frames. Each swf frame has its own sound data chunk.
//
class GAppendableSoundData : public GSoundDataBase
{
public:
    GAppendableSoundData(UInt format, UInt rate);
    ~GAppendableSoundData();

    // locks the sound returns a pointer where a new sound data chunk can be read to.
    // params:
    //      pdata  - buffer for sound data
    //      length - number of sound samples
    UByte*           LockDataForAppend(UInt length, UInt dsize);
    // unlock the sound that was previously locked by the call to LockDataForAppend method
    void             UnlockData();
    // read a buffer from the sound data
    UInt             GetData(UByte* buff, UInt dsize);
    // seek the position in the sound data in bytes
    bool             SeekPos(UInt pos);

    UInt             GetDataSize() const { return DataSize; }
protected:

    struct DataChunk : public GNewOverrideBase<GStat_Sound_Mem>
    {
        DataChunk(UInt capacity);
        ~DataChunk();

        DataChunk* pNext;
        UByte*     pData;
        UInt       DataSize;
        UInt       StartSample;
        UInt       SampleCount;
    };

    DataChunk*     pFirstChunk;
    DataChunk*     pFillChunk;
    DataChunk*     pReadChunk;
    UInt           DataSize;
    UInt           ReadPos;
    GLock          ChunkLock;
};

//
// ***** SoundFile class
//
// This class sound data that is referenced by a file name
// It is used for loading sound data that was extracted from a swf file
// by gfxexport utility.
//
class GSoundFile : public GSoundDataBase
{
public:
    GSoundFile(const char* fname, UInt rate, UInt length, bool streaming);
    ~GSoundFile();

    const char* GetFileName() const { return pFileName; }

protected:
    char* pFileName;
};

#endif // GFC_NO_SOUND

#endif 
