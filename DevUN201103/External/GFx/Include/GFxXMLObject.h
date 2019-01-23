/**********************************************************************

Filename    :   GFxXMLObject.h
Content     :   XML object support
Created     :   March 17, 2008
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFxXMLObject_H
#define INC_GFxXMLObject_H

#include <GConfig.h>

#ifndef GFC_NO_XML_SUPPORT

#include <GRefCount.h>
#include <GArray.h>

#include <GFxPlayer.h>

//
// This file declares a DOM string manager built for XML file processing
// and DOM tree creation. The string manager manages unique strings
// by hashing them. All strings are expected to be in UTF-8.
//


// Forward declarations
class   GFxXMLDOMString;
class   GFxXMLDOMStringManager;
struct  GFxXMLDOMStringNode;

struct GFxXMLNode;
struct GFxXMLPrefix;
struct GFxXMLAttribute;
struct GFxXMLElementNode;
struct GFxXMLTextNode;
struct GFxXMLDocument;
struct GFxXMLShadowRef;
struct GFxASXMLRootNode;

struct GFxXMLObject;
class GFxXMLObjectManager;
class GFxMovieRoot;


//
// A wrapper class to compare a char array with an XML DOM string
//
struct GFxXMLDOMStringCompareWrapper
{
    // Char array
    const char*     pData;

    // Char array byte size
    UPInt           Length;

    GFxXMLDOMStringCompareWrapper(const char* pdata, UPInt len)
        : pData(pdata), Length(len) {}
};

// 
// String node - stored in the manager table.
//
struct GFxXMLDOMStringNode
{        
    enum FlagConstants
    {
        Flag_HashMask       = 0x00FFFFFF,
    };

    const char*         pData;
    union
    {
        GFxXMLDOMStringManager*   pManager;
        GFxXMLDOMStringNode*      pNextAlloc;
    };
    UInt32              HashFlags;
    UInt                Size;

    SInt32          RefCount;

    // *** Utility functions

    GFxXMLDOMStringManager*  GetManager() const
    {
        return pManager;
    }
    inline UInt32  GetHashCode()
    {
        return HashFlags & Flag_HashMask;
    }

    void    AddRef()
    {
        RefCount++;
    }
    void    Release()
    {
        if (--RefCount == 0)
        {  
            // Refcount is 0
            // Objects should clean themselves
            ReleaseNode();
        }
    }

    // Releases the node to the manager.
    void    ReleaseNode();    
};


// 
// XMLString - XML DOM string implementation.

// XMLString is represented as a pointer to a unique node so that
// comparisons can be implemented very fast. Nodes are allocated by
// and stored within XMLStringManager.
// XMLString objects can be created only by string manager; default
// constructor is not implemented.

struct GFxXMLDOMStringNodeHolder
{
    GFxXMLDOMStringNode*  pNode;
};

// Do not derive from GNewOverrideBase and do not new!
class GFxXMLDOMString : public GFxXMLDOMStringNodeHolder
{
    friend class GFxXMLDOMStringManager;

public:

    // *** Create/Destroy: can

    explicit GFxXMLDOMString(GFxXMLDOMStringNode *pnode);

    GFxXMLDOMString(const GFxXMLDOMString& src);

    ~GFxXMLDOMString();

    // *** General Functions

    void        Clear();

    // Pointer to raw buffer.
    const char* ToCStr() const          { return pNode->pData; }
    const char* GetBuffer() const       { return pNode->pData; }

    // Size of string characters without 0. Raw count, not UTF8.
    UInt        GetSize() const         { return pNode->Size; }
    bool        IsEmpty() const         { return pNode->Size == 0; }

    UInt        GetHashFlags() const    { return pNode->HashFlags; }
    UInt        GetHash() const         { return GetHashFlags() & GFxXMLDOMStringNode::Flag_HashMask; }

    GFxXMLDOMStringNode* GetNode() const      { return pNode; }

    static UInt32   HashFunction(const char *pchar, UPInt length);

    void    operator = (const GFxXMLDOMString& src)
    {
        AssignNode(src.pNode);
    }

    // Comparison.
    bool    operator == (const GFxXMLDOMString& str) const
    {
        return pNode == str.pNode;
    }    
    bool    operator != (const GFxXMLDOMString& str) const
    {
        return pNode != str.pNode;
    }
    bool    operator == (const GFxXMLDOMStringCompareWrapper& str)
    {
        return (strncmp(pNode->pData, str.pData, str.Length) == 0);
    }

    void    AssignNode(GFxXMLDOMStringNode *pnode);
};

//
// Hash key 
//
struct GFxXMLDOMStringKey
{
    const char* pStr;
    UPInt       HashValue;
    UPInt       Length;

    GFxXMLDOMStringKey(const GFxXMLDOMStringKey &src)
        : pStr(src.pStr), HashValue(src.HashValue), Length(src.Length)
    { }
    GFxXMLDOMStringKey(const char* pstr, UPInt hashValue, UPInt length)
        : pStr(pstr), HashValue(hashValue), Length(length)
    { }
};


//
// Hash functions
//
template<class C>
class GFxXMLDOMStringNodeHashFunc
{
public:
    typedef C ValueType;

    // Hash code is stored right in the node
    UPInt  operator() (const C& data) const
    {
        return data->HashFlags;
    }

    UPInt  operator() (const GFxXMLDOMStringKey &str) const
    {
        return str.HashValue;
    }

    // Hash update - unused.
    static UPInt    GetCachedHash(const C& data)                { return data->HashFlags; }
    static void     SetCachedHash(C& data, UPInt hashValue)     { GUNUSED2(data, hashValue); }
    // Value access.
    static C&       GetValue(C& data)                           { return data; }
    static const C& GetValue(const C& data)                     { return data; }

};

//
// String node hash set 
//
// Keeps track of all strings currently existing in the manager.
//
typedef GHashSetUncachedLH<GFxXMLDOMStringNode*, 
GFxXMLDOMStringNodeHashFunc<GFxXMLDOMStringNode*>, GFxXMLDOMStringNodeHashFunc<GFxXMLDOMStringNode*>, 
GFxStatMV_XML_Mem>  XMLDOMStringNodeHash;



// 
// XML string manager
//
class GFxXMLDOMStringManager
{    
    friend class GFxXMLDOMString;  
    friend struct GFxXMLDOMStringNode;

    XMLDOMStringNodeHash StringSet;

    //
    // Allocation Page structures, used to avoid many small allocations.
    //
    struct StringNodePage
    {
        enum { StringNodeCount = 127 };     
        // Node array starts here.
        GFxXMLDOMStringNode       Nodes[StringNodeCount];
        // Next allocated page; save so that it can be released.
        StringNodePage  *   pNext;
    };

    //
    // Strings text is also allocated in pages, to make small string 
    // management efficient.
    //
    struct TextPage
    {
        // The size of buffer is usually 12 or 16, depending on platform.
        enum  {
            BuffSize   = (sizeof(void*) <= 4) ? (sizeof(void*) * 3) 
            : (sizeof(void*) * 2),
            // Use -2 because we do custom alignment and we want to fit in 
            // allocator block.
            BuffCount  = (2048 / BuffSize) - 2
        };

        struct Entry
        {
            union
            {   // Entry for free node list.
                Entry*  pNextAlloc;
                char    Buff[BuffSize];
            };
        };

        Entry       Entries[BuffCount];
        TextPage*   pNext;
        void*       pMem;
    };


    //
    // Free string nodes that can be used.
    //
    GFxXMLDOMStringNode*      pFreeStringNodes;

    //
    // List of allocated string node pages, so that they can be released.
    // Note that these are allocated for the duration of XMLStringManager lifetime.
    //
    StringNodePage*     pStringNodePages;

    //
    // Free string buffers that can be used, together with their owner nodes.
    //
    TextPage::Entry*    pFreeTextBuffers;
    TextPage*           pTextBufferPages;

    //
    // Heap used for string allocation
    //
    GMemoryHeap*        pHeap;

    //
    // Pointer to the available string node.
    //
    GFxXMLDOMStringNode       EmptyStringNode;

    void            AllocateStringNodes();
    void            AllocateTextBuffers();

    GFxXMLDOMStringNode*  AllocStringNode()
    {
        if (!pFreeStringNodes)
            AllocateStringNodes();
        // Grab next node from list.
        GFxXMLDOMStringNode* pnode = pFreeStringNodes;
        if (pFreeStringNodes)
            pFreeStringNodes = pnode->pNextAlloc;      
        // Assign manager
        pnode->pManager = this;       
        return pnode;
    }

    void            FreeStringNode(GFxXMLDOMStringNode* pnode)
    {
        if (pnode->pData)
        {
            FreeTextBuffer(const_cast<char*>(pnode->pData), pnode->Size);
            pnode->pData = 0;
        }
        // Insert into free list.
        pnode->pNextAlloc = pFreeStringNodes;
        pFreeStringNodes  = pnode;
    }

    //
    // Get buffer for text (adds 0 to length).
    //
    char*           AllocTextBuffer(UPInt length);

    //
    // Allocates text buffer and copies length characters into it. Appends 0.
    //
    char*           AllocTextBuffer(const char* pbuffer, UPInt length);
    void            FreeTextBuffer(char* pbuffer, UPInt length);


    // Various functions for creating string nodes.
    // Returns a node copy/reference to text in question.
    // All lengths specified must be exact and not include trailing characters.

    //
    // Allocates node owning buffer.
    //
    GFxXMLDOMStringNode*    CreateStringNode(const char* pstr, UPInt length);

    //
    // These functions also perform string concatenation.
    //
    GFxXMLDOMStringNode*    CreateStringNode(const char* pstr1, UPInt l1,
        const char* pstr2, UPInt l2);

    GFxXMLDOMStringNode*    GetEmptyStringNode() { return &EmptyStringNode;  }

public:

    GFxXMLDOMStringManager();
    ~GFxXMLDOMStringManager();

    //
    // Return the instance of empty string
    //
    GFxXMLDOMString   CreateEmptyString()
    {
        return GFxXMLDOMString(GetEmptyStringNode());
    }

    //
    // Create a string from a buffer.
    //
    GFxXMLDOMString   CreateString(const char *pstr, UPInt length)
    {
        return GFxXMLDOMString(CreateStringNode(pstr, length));
    }     
    void   AppendString(GFxXMLDOMString& str1, const char* str2, UPInt length)
    {
        str1.AssignNode(CreateStringNode(str1.pNode->pData, str1.pNode->Size,
            str2, length));
    }
};


// --------------------------------------------------------------------


// 
// The heterogeneous object manager
// 
// Wraps all object allocations into a single interface
//
class GFxXMLObjectManager : public GRefCountBaseNTS<GFxXMLObjectManager, GFxStatMV_XML_Mem>, public GFxExternalLibPtr
{
    //
    // Hashed string pool that maintains only one copy 
    // of a string
    //
    GFxXMLDOMStringManager         StringPool; 

    //
    // The heap used for object allocation. This heap is
    // the same as the heap used to allocate the object manager
    // unless a different one is explicitly specified.
    //
    GMemoryHeap*                    pHeap;

public:
    GFxXMLObjectManager(GMemoryHeap* pheap, GFxMovieRoot* powner = NULL);
    GFxXMLObjectManager(GFxMovieRoot* powner = NULL);
    virtual ~GFxXMLObjectManager();

    GMemoryHeap*        GetHeap()   { return pHeap; }

    //
    // Object creation methods
    //

    GFxXMLDOMString CreateString(const char* str, UPInt len)
    {        
        return StringPool.CreateString(str, len);
    }
    GFxXMLDOMString EmptyString()
    {
        return StringPool.CreateEmptyString();
    }
    void   AppendString(GFxXMLDOMString& str1, const char* str2, UPInt len)
    {
        StringPool.AppendString(str1, str2, len);
    }

    GFxXMLDocument*     CreateDocument();
    GFxXMLElementNode*  CreateElementNode(GFxXMLDOMString value);
    GFxXMLTextNode*     CreateTextNode(GFxXMLDOMString value);
    GFxXMLAttribute*    CreateAttribute(GFxXMLDOMString name, GFxXMLDOMString value);
    GFxXMLPrefix*       CreatePrefix(GFxXMLDOMString name, GFxXMLDOMString value);
    GFxXMLShadowRef*    CreateShadowRef();
    GFxASXMLRootNode*   CreateRootNode(GFxXMLNode* pdom);
};


#endif  // #ifndef GFC_NO_XML_SUPPORT

#endif // INC_GFxXMLObject_H
