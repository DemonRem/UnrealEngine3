/**********************************************************************

Filename    :   GFxAmpProfileFrame.h
Content     :   Profile information sent to AMP
Created     :   February 2010
Authors     :   Alex Mantzaris, Ben Mowery

Copyright   :   (c) 2005-2010 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef GFX_AMP_PROFILE_FRAME
#define GFX_AMP_PROFILE_FRAME

#include "GRefCount.h"
#include "GString.h"
#include "GArray.h"
#include "GHash.h"

class GFile;

// This struct is used for sending the memory report
// It is a tree structure with each node corresponding to a heap
//
struct GFxAmpMemItem : public GRefCountBase<GFxAmpMemItem, GStat_Default_Mem>
{
    GStringLH                           Name;
    UInt32                              Value;
    bool                                HasValue;
    bool                                StartExpanded;
    UInt32                              ID;
    UInt32                              ImageId;
    GArrayLH< GPtr<GFxAmpMemItem> >     Children;

    GFxAmpMemItem(UInt32 id) : Value(0), HasValue(false), StartExpanded(false), ID(id), ImageId(0) 
    { }

    void                        SetValue(UInt32 memValue);
    GFxAmpMemItem*              AddChild(UInt32 id, const char* name);
    GFxAmpMemItem*              AddChild(UInt32 id, const char* name, UInt32 memValue);
    const GFxAmpMemItem*        SearchForName(const char* name) const;
    UInt32                      GetMaxId() const;

    // operators
    GFxAmpMemItem&              operator=(const GFxAmpMemItem& rhs);
    GFxAmpMemItem&              operator/=(UInt numFrames);
    GFxAmpMemItem&              operator*=(UInt num);
    bool                        Merge(const GFxAmpMemItem& other);

    // Serialization
    void                        Read(GFile& str, UInt32 version);
    void                        Write(GFile& str, UInt32 version) const;
    void                        ToString(GStringBuffer* report, UByte indent = 0) const;
};

// Memory segment for fragmentation report
//
struct GFxAmpMemSegment
{
    UInt64              Addr;
    UInt64              Size;
    UByte               Type;
    UInt64              HeapId;   // Heap pointer used as a Heap ID

    GFxAmpMemSegment() : Addr(0), Size(0), Type(0), HeapId(0) {}

    // Serialization
    void                Read(GFile& str, UInt32 version);
    void                Write(GFile& str, UInt32 version) const;
};

// Heap information for fragmentation report
//
struct GFxAmpHeapInfo : public GRefCountBase<GFxAmpMemSegment, GStat_Default_Mem>
{
    UInt64              Arena;
    UInt32              Flags;
    GStringLH           Name;

    GFxAmpHeapInfo(const GMemoryHeap* heap = NULL);

    // Serialization
    void                Read(GFile& str, UInt32 version);
    void                Write(GFile& str, UInt32 version) const;
};

// Fragmentation report
// Includes the memory segments and the associated heap information
//
struct GFxAmpMemFragReport : public GRefCountBase<GFxAmpMemSegment, GStat_Default_Mem>
{
    GArrayLH<GFxAmpMemSegment>                  MemorySegments;
    GHashLH<UInt64, GPtr<GFxAmpHeapInfo> >      HeapInformation; 

    // Serialization
    void                Read(GFile& str, UInt32 version);
    void                Write(GFile& str, UInt32 version) const;
};


// This struct contains the stats for each ActionScript instruction for a given movie
//
// The stats are organized in an array with one element per ActionScript buffer
// The buffer stats include execution times for each instruction, but also the buffer length,
// which is used by the client to map source code lines to execution time
//
// Each buffer is uniquely identified by a SWD handle (unique to each SWF file),
// and the offset into the SWF in bytes.
// 
struct GFxAmpMovieInstructionStats : public GRefCountBase<GFxAmpMovieInstructionStats, GStat_Default_Mem>
{
    struct InstructionTimePair
    {
        UInt32      Offset;  // Byte offset from beginning of buffer
        UInt64      Time;    // Microseconds
    };

    // ActionScript buffer data collected during execution
    struct ScriptBufferStats : GRefCountBase<ScriptBufferStats, GStat_Default_Mem>
    {
        UInt32                          SwdHandle;      // uniquely defines the SWF
        UInt32                          BufferOffset;   // Byte offset from beginning of SWF
        UInt32                          BufferLength;
        GArrayLH<InstructionTimePair>   InstructionTimesArray;

        // Serialization
        void                            Read(GFile& str, UInt32 version);
        void                            Write(GFile& str, UInt32 version) const;
    };

    // Array of pointers, so all data can be allocated on the same heap
    GArrayLH< GPtr<ScriptBufferStats> > BufferStatsArray;

    // operators
    GFxAmpMovieInstructionStats& operator/=(UInt numFrames);
    GFxAmpMovieInstructionStats& operator*=(UInt num);
    void        Merge(const GFxAmpMovieInstructionStats& other);

    // Serialization
    void        Read(GFile& str, UInt32 version);
    void        Write(GFile& str, UInt32 version) const;
};


// Struct for mapping a byte offset to a function name and length
struct GFxAmpFunctionDesc : public GRefCountBase<GFxAmpFunctionDesc, GStat_Default_Mem>
{
    GStringLH   Name;
    UInt        Length;
    UInt64      FileId;
    UInt32      FileLine;
};
typedef GHashLH<UInt64, GPtr<GFxAmpFunctionDesc> > GFxAmpFunctionDescMap;

typedef GHashLH<UInt64, GString> SourceFileDescMap;

// This struct contains the execution times and numbers for each ActionScript function 
// for a given movie
//
// The stats are organized in an array with one element per ActionScript function
// and a hash map from function ID to function name
//
struct GFxAmpMovieFunctionStats : public GRefCountBase<GFxAmpMovieFunctionStats, GStat_Default_Mem>
{
    struct FuncStats
    {
        UInt64      FunctionId;     // SWF handle and offset
        UInt64      CallerId;       // Caller SWF handle and offset
        UInt32      TimesCalled;
        UInt64      TotalTime;      // microseconds
    };

    GArrayLH<FuncStats>                 FunctionTimings;
    GFxAmpFunctionDescMap               FunctionInfo;

    // operators
    GFxAmpMovieFunctionStats& operator/=(UInt numFrames);
    GFxAmpMovieFunctionStats& operator*=(UInt numFrames);
    void        Merge(const GFxAmpMovieFunctionStats& other);

    // Serialization
    void        Read(GFile& str, UInt32 version);
    void        Write(GFile& str, UInt32 version) const;

    // For debugging
    void DebugPrint() const;
};

// This struct contains the execution times and numbers for each ActionScript line 
// for a given movie
//
// The stats are organized in an array with one element per ActionScript line
// and a hash map from file ID to filename
//
struct MovieSourceLineStats : public GRefCountBase<MovieSourceLineStats, GStat_Default_Mem>
{
    struct SourceStats
    {
        UInt64      FileId;         // SWF handle and file ID
        UInt32      LineNumber;
        UInt64      TotalTime;      // microseconds
    };

    GArrayLH<SourceStats>               SourceLineTimings;
    SourceFileDescMap                   SourceFileInfo;

    // operators
    MovieSourceLineStats& operator/=(unsigned numFrames);
    MovieSourceLineStats& operator*=(unsigned numFrames);
    void        Merge(const MovieSourceLineStats& other);

    // Serialization
    void        Read(GFile& str, UInt32 version);
    void        Write(GFile& str, UInt32 version) const;
};

// This struct contains all the profile statistics for a view
// That includes function statistics and instruction statistics
// It is part of the profile sent to AMP every frame
struct GFxMovieStats : public GRefCountBase<GFxMovieStats, GStat_Default_Mem>
{
    struct MarkerInfo : public GRefCountBase<MarkerInfo, GStat_Default_Mem>
    {
        GStringLH       Name;
        UInt32          Number;
    };

    UInt32              ViewHandle;
    UInt32              MinFrame;
    UInt32              MaxFrame;
    GStringLH           ViewName;
    UInt32              Version;
    float               Width;
    float               Height;
    float               FrameRate;
    UInt32              FrameCount;
    GArrayLH< GPtr<GFxMovieStats::MarkerInfo> >    Markers;

    GPtr<GFxAmpMovieInstructionStats> InstructionStats;
    GPtr<GFxAmpMovieFunctionStats> FunctionStats;
    GPtr<MovieSourceLineStats> SourceLineStats;

    // Initialization
    GFxMovieStats();
    
    // operators
    void            Merge(const GFxMovieStats& rhs);
    GFxMovieStats&  operator=(const GFxMovieStats& rhs);
    GFxMovieStats&  operator/=(UInt numFrames);
    GFxMovieStats&  operator*=(UInt num);

    // Serialization
    void            Read(GFile& str, UInt32 version);
    void            Write(GFile& str, UInt32 version) const;
};


// The data in this class consists of all the metrics reported by AMP
// and displayed by the client in various ways (graphs, tables, etc)
//
// The AMP server updates one such object every frame and sends it to the client
// as part of a GFxAmpMessageProfileFrame
// 
class GFxAmpProfileFrame : public GRefCountBase<GFxAmpProfileFrame, GStat_Default_Mem>
{
public:
    // Frame timestamp (microseconds)
    UInt64  TimeStamp;

    UInt32  FramesPerSecond;

    // CPU graph
    UInt32  AdvanceTime;
    UInt32      ActionTime;
    UInt32          SeekTime;
    UInt32      TimelineTime;
    UInt32      InputTime;
    UInt32          MouseTime;
    UInt32  GetVariableTime;
    UInt32  SetVariableTime;
    UInt32  InvokeTime;
    UInt32  DisplayTime;
    UInt32      TesselationTime;
    UInt32      GradientGenTime;
    UInt32  UserTime;

    // Rendering graph
    UInt32  LineCount;
    UInt32  MaskCount;
    UInt32  FilterCount;
    UInt32  TriangleCount;
    UInt32  DrawPrimitiveCount;
    UInt32  StrokeCount;
    UInt32  GradientFillCount;
    UInt32  MeshThrashing;
    UInt32  RasterizedGlyphCount;
    UInt32  FontTextureCount;
    UInt32  NumFontCacheTextureUpdates;

    // Memory graph
    UInt32  TotalMemory;
    UInt32  ImageMemory;
    UInt32  MovieDataMemory;
    UInt32  MovieViewMemory;
    UInt32  MeshCacheMemory;
    UInt32  FontCacheMemory;
    UInt32  VideoMemory;
    UInt32  SoundMemory;
    UInt32  OtherMemory;

    GArrayLH< GPtr<GFxMovieStats> >     MovieStats;
    GArrayLH<UInt32>                    SwdHandles;
    GArrayLH<UInt64>                    FileHandles;

    GPtr<GFxAmpMemItem>                 MemoryByStatId;
    GPtr<GFxAmpMemItem>                 MemoryByHeap;
    GPtr<GFxAmpMemItem>                 Images;
    GPtr<GFxAmpMemItem>                 Fonts;

    GPtr<GFxAmpMemFragReport>           MemFragReport;

    // Initialization
    GFxAmpProfileFrame();

    // Operators for multiple frame reporting
    GFxAmpProfileFrame& operator+=(const GFxAmpProfileFrame& rhs);
    GFxAmpProfileFrame& operator/=(UInt numFrames);
    GFxAmpProfileFrame& operator*=(UInt num);

    // Serialization
    void        Read(GFile& str, UInt32 version);
    void        Write(GFile& str, UInt32 version) const;
};

// This struct holds the current state of AMP
// For feedback to the AMP client
//
class GFxAmpCurrentState : public GRefCountBase<GFxAmpCurrentState, GStat_Default_Mem>
{
public:
    UInt32                  StateFlags;
    GStringLH               ConnectedApp;
    GStringLH               ConnectedFile;
    GStringLH               AaMode;
    GStringLH               StrokeType;
    GStringLH               CurrentLocale;
    GArrayLH<GString>       Locales;
    float                   CurveTolerance;
    float                   CurveToleranceMin;
    float                   CurveToleranceMax;
    float                   CurveToleranceStep;
    UInt64                  CurrentFileId;
    UInt32                  CurrentLineNumber;

    GFxAmpCurrentState();
    GFxAmpCurrentState& operator=(const GFxAmpCurrentState& rhs);
    bool operator!=(const GFxAmpCurrentState& rhs) const;

    // Serialization
    void        Read(GFile& str, UInt32 version);
    void        Write(GFile& str, UInt32 version) const;
};

#endif
