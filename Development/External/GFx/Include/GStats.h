/**********************************************************************

Filename    :   GStats.h
Content     :   Statistics tracking and reporting APIs
Created     :   May 20, 2008
Authors     :   Michael Antonov, Boris Rayskiy

Notes       :   

Copyright   :   (c) 2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GSTATS_H
#define INC_GSTATS_H

#include "GConfig.h"
#include "GTypes.h"
#include "GTimer.h"

// Classes Used
class GMemoryHeap;


// ***** Stat Group Identifiers

// Stat identifiers are organized into groups corresponding to different
// sub-systems such as the renderer, MovieDef, etc. Every group receives
// its own Id range, allowing identifiers within that system to be added
// without having to modify GStatId (those identifiers are normally defined
// as a part of a separate enumeration in their own header file).

// All identifier constants must follow this naming convention:
// - Start with a system group prefix such as GStatGroup_ (for group ids),
//   GStatHeap_, GStatRenderer_, GFxStatMV_, etc.
// - User one of the following endings to identify stat types:
//     - _Mem for GMemoryStat.
//     - _Tks for GTimerStat.
//     - _Cnt for GCounterStat.


enum GStatGroup
{
    GStatGroup_Default              = 0,
    
    GStatGroup_Core                 = 16,    
    GStatGroup_Renderer             = 1 << 6,
    GStatGroup_RenderGen            = 2 << 6,

    GStatGroup_GFxFontCache         = 3 << 6,
    GStatGroup_GFxMovieData         = 4 << 6,
    GStatGroup_GFxMovieView         = 5 << 6,
    GStatGroup_GFxRenderCache       = 6 << 6,
    GStatGroup_GFxPlayer            = 7 << 6,
    GStatGroup_GFxIME               = 8 << 6,

    // General Memory

    GStat_Mem                   = GStatGroup_Default + 1,
    GStat_Default_Mem,
    GStat_Image_Mem,
#ifndef GFC_NO_SOUND
    GStat_Sound_Mem,
#endif
    GStat_String_Mem,
#ifndef GFC_NO_VIDEO
    GStat_Video_Mem,
#endif
    // Memory allocated for debugging purposes.
    GStat_Debug_Mem,
        GStat_DebugHUD_Mem,
        GStat_DebugTracker_Mem,
        GStat_StatBag_Mem,

    // Core entries.
    GStatHeap_Start           =  GStatGroup_Core,
    // 16 slots for HeapSummary


    // How many entries we support by default
    GStat_MaxId         = 64 << 6, // 64 * 64 = 4096
    GStat_EntryCount    = 512
};


// ***** Identifiers for important heaps
//
// This IDs can be associated with additional statically defined 
// data to display. In particular, the heaps can be associated with
// a colors to be displayed in the HUD.
//------------------------------
enum GHeapId
{
    GHeapId_Global      = 1,
    GHeapId_MovieDef    = 2,
    GHeapId_MovieView   = 3,
    GHeapId_MovieData   = 4,
    GHeapId_MeshCache   = 5,
    GHeapId_FontCache   = 6,
    GHeapId_Images      = 7,
    GHeapId_OtherHeaps  = 8,
    GHeapId_HUDHeaps    = 9,
};


// ***** GStat

// Base class for all of the statistics. This class does not actually store
// any data, expecting derived classes such as GMemoryStat to do so.

class GStat
{
public:

    // Different statistics types that we collect.
    enum StatType
    {
        Stat_LogicalGroup,
        Stat_Memory,
        Stat_Timer,
        Stat_Counter,
        // The number of supported types.
        Stat_TypeCount
    };

    // A structure that represents a single statistics value.
    struct StatValue
    {
        enum ValueType
        {
            VT_None,
            VT_Int,
            VT_Int64,
            VT_Float
        };

        ValueType       Type;
        const char*     pName;
        union
        {
            UPInt       IValue;
            UInt64      I64Value;
            Float       FValue;
        };

        StatValue() : Type(VT_None), pName(""), IValue(0)
        { }
        StatValue(const StatValue& src)
            : Type(src.Type), pName(src.pName), I64Value(src.I64Value)
        { }

        StatValue& operator = (const StatValue& src)
        {
            Type = src.Type; pName = src.pName; I64Value = src.I64Value;
            return *this;
        }

        // Assignment of particular types
        void SetInt(const char* pname, UPInt val)
        {
            Type   = VT_Int;
            pName  = pname;
            IValue = val;
        }

        void SetInt64(const char* pname, UInt64 val)
        {
            Type   = VT_Int64;
            pName  = pname;
            I64Value = val;
        }

        void SetFloat(const char* pname, Float val)
        {
            Type   = VT_Float;
            pName  = pname;
            FValue = val;
        }
    };

};


// ***** GMemoryStat

// Records memory allocation statics for a particular allocation Id type.
// Keeps track of the number of bytes allocated and used, including use peak.
// Unlike allocation size, used includes the number of bytes occupied
// in the allocator for maintaining the block, including the different
// header and alignment space.

class GMemoryStat : public GStat
{
#ifdef GFC_NO_STAT

    // *** Null Stat implementation
public:

    GMemoryStat() { }
    
    void        Init()  { }
    void        Increment(UPInt, UPInt ) { }
    GMemoryStat& operator += (GMemoryStat& )   { return *this; }
    GMemoryStat& operator -= (GMemoryStat& )   { return *this; }

    StatType    GetStatType() const     { return Stat_Memory; }
    UInt        GetStatCount() const    { return 3; }

    void        GetStat(StatValue* pval, UInt index) const
    {
        switch(index)
        {
        case 0: pval->SetInt("Allocated", 0);   break;
        case 1: pval->SetInt("Used", 0);        break;   
        case 2: pval->SetInt("AllocCount", 0);  break;
        }
    }

    void        Add(GMemoryStat*)       { }
    void        SetMax(GMemoryStat*)    { }
    void        SetMin(GMemoryStat*)    { }
    void        Reset()                 { }

    UPInt       GetAllocated() const    { return 0; }
    UPInt       GetUsed() const         { return 0; }
    UPInt       GetAllocCount() const   { return 0; }

#else
    
    // *** Default Stat implementation
    UPInt   Allocated;
    UPInt   Used;    
    UPInt   AllocCount;

public:

    GMemoryStat()
    {
        Init();
    }

    void Init()
    {
        Allocated = 0;
        Used      = 0;
        AllocCount= 0;        
    }


    void    Increment(UPInt alloc, UPInt use)
    {
        Allocated+= alloc;
        Used      += use;
        AllocCount++;        
    }


    GMemoryStat& operator += (GMemoryStat& other)
    {
        Allocated += other.Allocated;
        Used      += other.Used;
        AllocCount+= other.AllocCount;
        return *this;
    }

    GMemoryStat& operator -= (GMemoryStat& other)
    {
        Allocated -= other.Allocated;
        Used      -= other.Used;
        AllocCount-= other.AllocCount;
        return *this;
    }


    // ***  General accessibility API.
    StatType    GetStatType() const     { return Stat_Memory; }
    UInt        GetStatCount() const    { return 3; }

    void        GetStat(StatValue* pval, UInt index) const
    {
        switch(index)
        {
        case 0: pval->SetInt("Allocated", Allocated);   break;
        case 1: pval->SetInt("Used", Used);             break;
        case 2: pval->SetInt("AllocCount", AllocCount); break;    
        }
    }
    
    // Reset, Add, SetMin, SetMax
    void            Add(GMemoryStat* p)
    { *this += *p; }

    void            SetMin(GMemoryStat* p)
    { 
        Allocated = (p->Allocated < Allocated) ? p->Allocated : Allocated;
        Used      = (p->Used      < Used     ) ? p->Used      : Used;
        AllocCount= (p->AllocCount<AllocCount) ? p->AllocCount: AllocCount;
    }
    void            SetMax(GMemoryStat* p)
    { 
        Allocated = (p->Allocated > Allocated) ? p->Allocated : Allocated;
        Used      = (p->Used      > Used     ) ? p->Used      : Used;
        AllocCount= (p->AllocCount>AllocCount) ? p->AllocCount: AllocCount;
    }


    void  Reset()
    {      
        Allocated = 0;
        Used      = 0;
        AllocCount= 0;
    }

    UPInt GetAllocated() const      { return Allocated; }
    UPInt GetUsed() const           { return Used; }
    UPInt GetAllocCount() const     { return AllocCount; }

#endif
};



// ***** GTimerStat

// GTimerStat records the number of nano-second ticks used for a certain
// measured activity, such as rendering or tessellation. The time is most
// easily measured by using the GTimerStat::ScopeTimer scope object around
// the code whose performance is to be measured. Every time that block of
// code is executed, the number of ticks will be incremented appropriately.

class GTimerStat : public GStat
{
#ifdef GFC_NO_STAT

    // *** Null Stat implementation
public:
    GTimerStat() { }

    void        Init() { }
    GTimerStat& operator += (GTimerStat&) { return *this; }
    StatType    GetStatType() const     { return Stat_Timer; }
    UInt        GetStatCount() const    { return 1; }

    void        GetStat(StatValue* pval, UInt index) const
    {
        switch(index)
        {
        case 0: pval->SetInt64("Ticks", 0);     break;   
        }
    }

    void            Add(GTimerStat*) { }
    void            SetMax(GTimerStat*) { }
    void            SetMin(GTimerStat*) { }
    void            AddTicks(UInt64) { }     
    void            Reset() { }

    class ScopeTimer
    {           
    public:
        ScopeTimer(GTimerStat*) { }
    };

    class ScopeRawTimer
    {           
    public:
        ScopeRawTimer(GTimerStat*) { }
    };

    UInt64 GetTicks() const         { return 0; }  

#else

    // *** Full GTimerStat implementation

    // Tracks the number of nano-second ticks that passed.
    UInt64   Ticks;    

public:

    GTimerStat()
    {
        Init();
    }

    void Init()
    {
        Ticks     = 0;     
    }

    GTimerStat& operator += (GTimerStat& other)
    {
        Ticks += other.Ticks;
        return *this;
    }


    // ***  General accessibility API.
    StatType    GetStatType() const     { return Stat_Timer; }
    UInt        GetStatCount() const    { return 1; }

    void        GetStat(StatValue* pval, UInt index) const
    {
        switch(index)
        {
        case 0: pval->SetInt64("Ticks", Ticks);         break;
        }
    }

    // Reset, Clear, Add
    void            Add(GTimerStat* p)
    { *this += *p; }

    void            SetMin(GTimerStat* p)
    { 
        Ticks = (p->Ticks < Ticks) ? p->Ticks : Ticks;
    }
    void            SetMax(GTimerStat* p)
    { 
        Ticks = (p->Ticks > Ticks) ? p->Ticks : Ticks;
    }

    void            AddTicks(UInt64 ticks)
    {
        Ticks += ticks;   
    }


    void  Reset()
    {
        Ticks = 0;
    }


    // Scope used for timer measurements.
    class ScopeTimer
    {
        GTimerStat* pTimer;
        UInt64      StartTicks;
    public:
     
        ScopeTimer(GTimerStat* ptimer)
            : pTimer(ptimer)
        {
            StartTicks = GTimer::GetProfileTicks();
        }
        ~ScopeTimer()
        {
            UInt64 endTicks = GTimer::GetProfileTicks();
            pTimer->AddTicks(endTicks - StartTicks);
        }
    };

    class ScopeRawTimer
    {
        GTimerStat* pTimer;
        UInt64      StartTicks;
    public:
     
        ScopeRawTimer(GTimerStat* ptimer)
            : pTimer(ptimer)
        {
            if (pTimer != NULL)
            {
                StartTicks = GTimer::GetRawTicks();
            }
            else
                StartTicks = 0;
        }
        ~ScopeRawTimer()
        {
            if (pTimer != NULL)
            {
                pTimer->AddTicks(GTimer::GetRawTicks() - StartTicks);
            }
        }
    };

    UInt64 GetTicks() const         { return Ticks; }   

#endif
};




// ***** GCounterStat

// A counter stat tracks the number of times a certain measured event
// took place. The counter can also be used to record the number of
// certain objects processed by the algorithm, such as the number of
// DrawPrimitives or triangles drawn by the renderer.

class GCounterStat : public GStat
{
#ifdef GFC_NO_STAT

    // *** Null GCounterStat
public:
    GCounterStat() { }
    GCounterStat(UPInt) { }

    void        Init() { }
    GCounterStat& operator += (GCounterStat&) { return *this; }
    StatType    GetStatType() const     { return Stat_Counter; }
    UInt        GetStatCount() const    { return 1; }

    void        GetStat(StatValue* pval, UInt index) const
    {
        switch(index)
        {
        case 0: pval->SetInt("Count", 0);         break;        
        }
    }

    // Reset, Add, SetMin, SetMax
    void        Add(GCounterStat*)    { }
    void        SetMin(GCounterStat*) { }
    void        SetMax(GCounterStat*) { }
    void        Reset()               { }
    void        AddCount(UPInt)       { }
    
    UPInt       GetCount() const         { return 0; }    

#else
    // *** Full GCounterStat implementation

    UPInt   Count;    

public:
    GCounterStat()
    {
        Init();
    }
    GCounterStat(UPInt count)
    {        
        Count = count;
    }

    void Init()
    {
        Count     = 0;
    }

    GCounterStat& operator += (GCounterStat& other)
    {
        Count += other.Count;
        return *this;
    }


    // ***  General accessibility API.
    StatType    GetStatType() const     { return Stat_Counter; }
    UInt        GetStatCount() const    { return 1; }

    void        GetStat(StatValue* pval, UInt index) const
    {
        switch(index)
        {
        case 0: pval->SetInt("Count", Count);         break;        
        }
    }

    // Reset, Add
    void            Add(GCounterStat* p)
    { *this += *p; }

    void            SetMin(GCounterStat* p)
    { 
        Count = (p->Count < Count) ? p->Count : Count;
    }
    void            SetMax(GCounterStat* p)
    { 
        Count = (p->Count > Count) ? p->Count : Count;
    }

    void            AddCount(UPInt count)
    {
        Count += count;
    }

    void  Reset()
    {    
        Count = 0;
    }

    UPInt GetCount() const         { return Count; }

#endif
};



/* ***** Statistics Group Types

    Group (Logical)
     - A logical group organizes a set of other stats into one group, giving
       them a common display title.

    Sum Group
     - A sum group maintains its own counter that includes the values of child
       item counters, but can be greater. It is expected that sum group can have
       additional 'remainder' values not accounted for by its children.
       The group is updated manually by the user using scopes or calling the
       appropriate Add function for the stat is question.

    AutoSum Group
     - A sum group that is calculated automatically during the call to
       GStatBag::UpdateGroups; the user does not need to update the sum since it 
       is computed by adding up all of its child values.
*/

// Stat descriptor structure used by the GDECLARE_??_STAT macros.
struct GStatDesc
{
    enum
    {    
        SF_Group         = 0x01,
        SF_Sum           = 0x02,    // This group is a manual sum of its sub-items.
        SF_AutoSum       = 0x04,    // This an automatically computed sum of its items.

        // Combination of flags
        SF_SumGroup      = SF_Group | SF_Sum,
        SF_AutoSumGroup  = SF_Group | SF_AutoSum
    };

    // Iterator used to enumerate children.
    class Iterator
    {
        const GStatDesc*    pDesc;
    public:     

        Iterator(const GStatDesc* p)        { pDesc = p; }
        Iterator(const Iterator&s)          { pDesc = s.pDesc; }
        Iterator& operator = (const Iterator&s)  { pDesc = s.pDesc; return *this; }

        const GStatDesc*    operator * () const  { return pDesc; }
        const GStatDesc*    operator -> () const { return pDesc; }

#ifdef GFC_NO_STAT

        bool    operator ++ () { return false; } 
        UInt    GetId() const  { return 0; }

#else // GFC_NO_STAT

        bool    operator ++ () { if (pDesc) pDesc = pDesc->pNextSibling; return (pDesc != 0); } 
        UInt    GetId() const  { return pDesc->Id; }

#endif // GFC_NO_STAT

        bool    operator == (const Iterator&s) const { return pDesc == s.pDesc; }
        bool    operator != (const Iterator&s) const { return pDesc != s.pDesc; }
        bool    IsEnd() const                        { return pDesc == 0; }
    };

    static void     InitChildTree();

#ifdef GFC_NO_STAT
    
    static const GStatDesc*  GCDECL GetDesc(UInt)   { return 0;             }
    GStatDesc(GStat::StatType, UByte, UInt, const char*, UInt) { }

    bool            IsAutoSumGroup() const          { return false;         }
    const char*     GetName() const                 { return "";            }
    Iterator        GetChildIterator() const        { return Iterator(0);   }
    static Iterator GetGroupIterator(UInt)          { return Iterator(0);   }
    UInt            GetId() const                   { return 0;             }

#else  // GFC_NO_STAT

    // Register descriptors so that they can be looked up by id.
    static void             RegisterDesc(GStatDesc* pdesc);
    static const GStatDesc* GCDECL GetDesc(UInt id);

private:

    UByte           Type;
    UByte           Flags;    
    UInt            Id;
    UInt            GroupId;
    const char*     pName;

    // Tree data representation, established by RegisterDesc.  A parent descriptor
    // can have a list if children which are enumerated by accessing pChild and
    // then traversing the chain of pNextSibling.
    GStatDesc*      pChild;
    GStatDesc*      pNextSibling;   

public:

    GStatDesc(GStat::StatType type, UByte flags, UInt id,
              const char* pname, UInt groupId)
    {
        Type    = (UByte)type;
        Flags   = flags;
        Id      = id;
        GroupId = groupId;
        pName   = pname;
        pChild  = 0;
        pNextSibling = 0;

        RegisterDesc(this);
    }
    
    UByte           GetType() const { return Type; }

    UInt            GetId() const { return Id; }
    void            SetId(UInt value) { Id = value; }

    UInt            GetGroupId() const { return GroupId; }
    void            SetGroupId(UInt value) { GroupId = value; }


    bool            IsAutoSumGroup() const
    {
        return (Flags & SF_AutoSumGroup) == SF_AutoSumGroup;
    }

    const char*     GetName() const { return pName; }

    // Obtain Iterator for the first child. Users can call IsEnd() to determine
    // if a child actually exists.
    Iterator        GetChildIterator() const        { return Iterator(pChild); }
    
    // Get an item Iterator based on it. 
    static Iterator GetGroupIterator(UInt groupId)  { return Iterator(GetDesc(groupId)); }

#endif  // GFC_NO_STAT

};


// We need to create this table during initialization.
// statId -> StatDesc
// statId -> StatInterface [identified by type]

#define GDECLARE_STAT_GROUP(id, name, group)       GStatDesc GSTAT_##id(GStat::Stat_LogicalGroup, GStatDesc::SF_Group, id, name, group);

// Manual sum groups.
#define GDECLARE_MEMORY_STAT_SUM_GROUP(id, name, group)       GStatDesc GSTAT_##id(GStat::Stat_Memory, GStatDesc::SF_SumGroup, id, name, group);
#define GDECLARE_TIMER_STAT_SUM_GROUP(id, name, group)        GStatDesc GSTAT_##id(GStat::Stat_Timer, GStatDesc::SF_SumGroup, id, name, group);
#define GDECLARE_COUNTER_STAT_SUM_GROUP(id, name, group)      GStatDesc GSTAT_##id(GStat::Stat_Counter, GStatDesc::SF_SumGroup, id, name, group);
// Automatically added groups.
#define GDECLARE_MEMORY_STAT_AUTOSUM_GROUP(id, name, group)   GStatDesc GSTAT_##id(GStat::Stat_Memory, GStatDesc::SF_AutoSumGroup, id, name, group);
#define GDECLARE_TIMER_STAT_AUTOSUM_GROUP(id, name, group)    GStatDesc GSTAT_##id(GStat::Stat_Timer, GStatDesc::SF_AutoSumGroup, id, name, group);
#define GDECLARE_COUNTER_STAT_AUTOSUM_GROUP(id, name, group)  GStatDesc GSTAT_##id(GStat::Stat_Counter, GStatDesc::SF_AutoSumGroup, id, name, group);
// Stats.
#define GDECLARE_MEMORY_STAT(id, name, group)   GStatDesc GSTAT_##id(GStat::Stat_Memory, 0, id, name, group);
#define GDECLARE_TIMER_STAT(id, name, group)    GStatDesc GSTAT_##id(GStat::Stat_Timer, 0, id, name, group);
#define GDECLARE_COUNTER_STAT(id, name, group)  GStatDesc GSTAT_##id(GStat::Stat_Counter, 0, id, name, group);




// {stat_data, stat_id} -> Stat interface.

class GStatInfo : public GStat
{
public:
    // Abstract interface use to obtain Stat
    class StatInterface
    {
    public:
        virtual ~StatInterface() { }

        virtual StatType        GetType(GStat* p)                               = 0;
        virtual UInt            GetStatCount(GStat* p)                          = 0;
        virtual void            GetStat(GStat* p, StatValue* pval, UInt index)  = 0;
        virtual UPInt           GetStatDataSize() const                         = 0;
        // Reset, Clear, Add
        virtual void            Init(GStat* p)                                  = 0;
        virtual void            Add(GStat* p1, GStat* p2)                       = 0;
        virtual void            SetMin(GStat* p1, GStat* p2)                    = 0;
        virtual void            SetMax(GStat* p1, GStat* p2)                    = 0;
        virtual void            Reset(GStat* p)                                 = 0;
    };

private:
    UInt            StatId;
    StatInterface*  pInterface;
    GStat*          pData;

public:

    GStatInfo()
        : StatId(0), pInterface(0), pData(0)
    {}

    GStatInfo(UInt statId, StatInterface* psi, GStat* pdata)
        : StatId(statId), pInterface(psi), pData(pdata)
    { }

    GStatInfo(const GStatInfo& src)
    {
        StatId      = src.StatId;
        pInterface  = src.pInterface;
        pData       = src.pData;
    }

    bool        IsNull() const                          { return pInterface == 0; }

    // Access individual stat components.
    StatType    GetType() const                         { return pInterface->GetType(pData); }
    UInt        GetStatCount() const                    { return pInterface->GetStatCount(pData); }
    void        GetStat(StatValue* pval, UInt index)    { pInterface->GetStat(pData, pval, index); }
    
    void        Add(GStatInfo& si2)
    {
        GASSERT(si2.GetType() == GetType());
        pInterface->Add(pData, si2.pData);
    }
    void        SetMin(GStatInfo& si2)
    {
        GASSERT(si2.GetType() == GetType());
        pInterface->SetMin(pData, si2.pData);
    }
    void        SetMax(GStatInfo& si2)
    {
        GASSERT(si2.GetType() == GetType());
        pInterface->SetMax(pData, si2.pData);
    }
    void        Reset()
    {        
        pInterface->Reset(pData);
    }

    const char* GetName() const { return GStatDesc::GetDesc(StatId)->GetName(); }

    
    // Manual interpretation.
    bool        IsMemory() const        { return GetType() == GStat::Stat_Memory; }
    bool        IsTimer() const         { return GetType() == GStat::Stat_Timer; }
    bool        IsCounter() const       { return GetType() == GStat::Stat_Counter; }

    GMemoryStat* ToMemoryStat() const   { GASSERT(IsMemory()); return (GMemoryStat*)pData; }
    GTimerStat*  ToTimerStat() const    { GASSERT(IsTimer()); return (GTimerStat*)pData; }
    GCounterStat*  ToCounterStat() const{ GASSERT(IsCounter()); return (GCounterStat*)pData; }

};


// Template class implementing GStatInfo::StatInterface
template<class C>
class GStatInfo_InterfaceImpl : public GStatInfo::StatInterface
{
    typedef GStat::StatType  StatType;
    typedef GStat::StatValue StatValue;

    StatType        GetType(GStat* p)           { return ((C*)p)->GetStatType(); }
    UInt            GetStatCount(GStat* p)      { return ((C*)p)->GetStatCount(); }
    void            GetStat(GStat* p, StatValue* pval, UInt index) { ((C*)p)->GetStat(pval, index); }
    UPInt           GetStatDataSize() const     { return sizeof(C); }
    void            Init(GStat* p)              { ((C*)p)->Init(); }
    void            Add(GStat* p, GStat* p2)    { ((C*)p)->Add((C*)p2); }
    void            SetMin(GStat* p, GStat* p2) { ((C*)p)->SetMin((C*)p2); }
    void            SetMax(GStat* p, GStat* p2) { ((C*)p)->SetMax((C*)p2); }
    void            Reset(GStat* p)             { ((C*)p)->Reset(); }
};


// ***** GStatBag

// Represents a bag, or collection of statistics data. Any type of GStat can
// be added into the bag; after statistics data is added it can be iterated
// through with Iterator class, obtained from GetIterator. 


class GStatBag
{

public:

    typedef GStatInfo::StatInterface StatInterface;


#ifdef GFC_NO_STAT

    // *** Null GStatBag

    GStatBag(GMemoryHeap* = 0, UInt = 0) { }
    GStatBag(const GStatBag&) { }
    ~GStatBag() { }
    
    void    Clear() { }
    void    Reset() { }
    bool    Add(UInt, GStat*)                   { return false;                     }    
    GStatBag& operator += (const GStatBag&)     { return *this;                     }
    GStatBag& operator  = (const GStatBag&)     { return *this;                     }
    void    UpdateGroups() { }

    bool    GetStat(GStatInfo *, UInt) const    { return false;                     }

    inline bool AddStat(UInt, const GStat&)     { return false;                     }

    // Add/SetMin/SetMax statistic value of a certain id.
    bool    SetMin(UInt statId, GStat* pstat)   { GUNUSED(statId); GUNUSED(pstat); return false; }
    bool    SetMax(UInt statId, GStat* pstat)   { GUNUSED(statId); GUNUSED(pstat); return false; }

    void    SetMin(const GStatBag& other)       { GUNUSED(other);                   }
    void    SetMax(const GStatBag& other)       { GUNUSED(other);                   }

    struct Iterator
    {
        GStatInfo   Result;    

        // Default Iterator start points to no data.
        Iterator(GStatBag*, UInt, UInt) { }
        Iterator(const Iterator&) { }

        const GStatInfo&    operator * () const { return Result; }
        const GStatInfo*    operator -> () const { return &Result; }
        bool    operator ++ () { return false; }
        UInt    GetId() const  { return GStatGroup_Default; }
        bool    operator == (const Iterator&) const { return false; }
        bool    operator != (const Iterator&) const { return true; }
        bool    IsEnd() const { return true; }
    };
    Iterator    GetIterator(UInt groupId = GStat_MaxId) { return Iterator (this, 0, groupId);}

#else // GFC_NO_STAT


    // *** Full GStatBag Implementation

    enum
    {
        StatBag_MemGranularity = 8,
        StatBag_PageShift      = 4,
        StatBag_PageSize       = 1 << StatBag_PageShift,
        StatBag_PageTableSize  = GStat_MaxId / StatBag_PageSize,
        StatBag_EndId          = GStat_MaxId,
        
        // Page table entries are set to this value if no memory
        // or slot is allocated to them.
        StatBag_IdUnused        = 0xFFFF
    };

    static StatInterface* GetInterface(UInt id);
  
private:

    // Allocated memory and pointer of how much of it is left.
    UByte*  pMem;
    UInt    MemSize;
    UPInt   MemAllocOffset;

    // Id page table. 
    UInt16 IdPageTable[StatBag_PageTableSize];

    GStat*  GetStatRef(UInt statId) const;
    UByte*  AllocStatData(UInt statId, UPInt size);

    void    RecursiveGroupUpdate(GStatDesc::Iterator it);

    // Combines statistics from two bags based on a function.
    void    CombineStatBags(const GStatBag& other, bool (GStatBag::*func)(UInt id, GStat*));
    
public:
    // Create a stat bag with specified number of entries.
    GStatBag(GMemoryHeap* pheap = 0, UInt memReserve = GStat_EntryCount * 16);
    GStatBag(const GStatBag& source);

    ~GStatBag();

public:

    // Clear out the bag, removing all states.
    void    Clear();
    // Reset stat values.
    void    Reset();


    // Add/SetMin/SetMax statistic value of a certain id.
    bool    Add(UInt statId, GStat* pstat);
    bool    SetMin(UInt statId, GStat* pstat);
    bool    SetMax(UInt statId, GStat* pstat);

    inline bool AddStat(UInt statId, const GStat& stat)
    {
        return Add(statId, const_cast<GStat*>(&stat));
    }

    // Some optimization for GMemoryHeapStat
    bool    AddMemoryStat(UInt statId, const GMemoryStat& stat);    
    bool    IncrementMemoryStat(UInt statId, UPInt alloc, UPInt use);

    // Compute minimums and maximums of two bags.
    void    Add(const GStatBag& other)      { CombineStatBags(other, &GStatBag::Add); }
    void    SetMin(const GStatBag& other)   { CombineStatBags(other, &GStatBag::SetMin); }
    void    SetMax(const GStatBag& other)   { CombineStatBags(other, &GStatBag::SetMax); }

    // Add values of a different stat bag to ours.
    GStatBag& operator += (const GStatBag& other) { Add(other); return *this; }
    GStatBag& operator = (const GStatBag& other)
    {
        if (this == &other)
            return *this;
        Clear();
        Add(other);
        return *this;
    }

    // Update all automatically computed groups in the list.
    void    UpdateGroups();
    
    // Does this accumulate data or not?
    bool    GetStat(GStatInfo *pstat, UInt statId) const;

    
    // *** Iterating Stat Data

    struct Iterator
    {
    protected:        
        UInt        Id;                 // Current id we are traversing.
        UInt        GroupId;            // Group id mask.
        GStatBag*   pBag;
        GStatInfo   Result;             // Cached result for entry

        // Advance to this/next valid item or fail.
        bool        AdvanceTillValid();

    public:
                
        // Default Iterator start points to no data.
        Iterator(GStatBag* pbag = 0, UInt id = GStat_MaxId, UInt groupId = GStat_MaxId);
        
        Iterator(const Iterator& src)
            :  Id(src.Id), GroupId(src.GroupId), Result(src.Result)
        { }
           

        const GStatInfo&    operator * () const { return Result; }
        const GStatInfo*    operator -> () const { return &Result; }

        bool    operator ++ ()
        {
            GASSERT(pBag != 0);
            Id++;
            return AdvanceTillValid();
        }
      
        UInt    GetId() const { return Id; }

        bool    operator == (const Iterator& it) const
        {
            return Id == it.Id;           
        }
        bool    operator != (const Iterator& it) const
        {
            return ! (*this == it);
        }

        bool    IsEnd() const
        {
            return (Id >= StatBag_EndId) ;
        }
    };

    // Obtains an Iterator for the specified stat group. Default implementation
    // will return all of the stats in the bag.
    Iterator    GetIterator(UInt groupId = GStat_MaxId);
   

    // Wait for stats to be ready; useful if stat update
    // request is queued up for update in a separate thread.
    //void    WaitForData();

#endif
};


#endif
