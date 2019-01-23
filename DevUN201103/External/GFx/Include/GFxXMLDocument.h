/**********************************************************************

Filename    :   GFxXMLDocument.h
Content     :   XML DOM support
Created     :   December 13, 2007
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2007 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFxXMLDocument_H
#define INC_GFxXMLDocument_H

#include <GConfig.h>

#ifndef GFC_NO_XML_SUPPORT

#include <GFxXMLObject.h>

// forward declarations
class GFxXMLDOMBuilder;
class GASXmlNodeObject;
struct GFxXMLShadowRef;

// --------------------------------------------------------------------

// 
// Attribute name/value pair
//
// Not refcounted because they are local to one and only one 
// GFxXMLElementNode. It is responsible for managing the
// attribute memory.
//
struct GFxXMLAttribute : public GNewOverrideBase<GFxStatMV_XML_Mem>
{
    // Attribute name
    GFxXMLDOMString         Name;  

    // Attribute value
    GFxXMLDOMString         Value; 

    // The attributes are stored as a linked list
    GFxXMLAttribute*        Next; 

    GFxXMLAttribute(GFxXMLDOMString name, GFxXMLDOMString value) :
        Name(name), Value(value), Next(NULL) {}
};


// --------------------------------------------------------------------


// 
// Prefix declaration
//
// Holds prefix and namespace value. This is only used during
// DOM tree creation and does not exist in a constructed
// tree.
//
struct GFxXMLPrefix : public GRefCountBaseNTS<GFxXMLPrefix, GFxStatMV_XML_Mem>
{
    // Prefix 
    GFxXMLDOMString            Name;  

    // Namespace URL
    GFxXMLDOMString            Value; 

    GFxXMLPrefix(GFxXMLDOMString name, GFxXMLDOMString value)
        : Name(name), Value(value) {}
};


// --------------------------------------------------------------------

// 
// Base class for the DOM nodes
//
// It is the super class of Element and Text nodes
//
struct GFxXMLNode : public GRefCountBaseNTS<GFxXMLNode, GFxStatMV_XML_Mem>
{
    // ObjectManager reference
    GPtr<GFxXMLObjectManager>   MemoryManager;

    // Either qualified name (element node) or text value (text node)
    GFxXMLDOMString         Value;    

    // Parent node
    GFxXMLElementNode*      Parent;  

    // Only next sibling pointers are reference counted because
    // the children representation follows a first-child next-sibling
    // scheme.
    GFxXMLNode*             PrevSibling;
    GPtr<GFxXMLNode>        NextSibling;

    // A pointer to a structure that is shadowing this node
    GFxXMLShadowRef*        pShadow;

    // Type of node.
    // 1: Element, 3: Text, Rest: Everything else
    UByte                   Type; 

    // Return a clone (deep or shallow) of the node
    // Implementation is left to the subclass
    virtual GFxXMLNode*     Clone(bool deep) = 0;

protected:

    GFxXMLNode(GFxXMLObjectManager* memMgr, UByte type);
    GFxXMLNode(GFxXMLObjectManager* memMgr, UByte type, 
        GFxXMLDOMString value);
    ~GFxXMLNode();
};


// --------------------------------------------------------------------


//
// Built-in node types (following AS 2.0 node type constants)
//
const UByte                 GFxXMLElementNodeType  = 1;
const UByte                 GFxXMLTextNodeType     = 3;


// 
// Element node (<element>text</element>)
//
struct GFxXMLElementNode : public GFxXMLNode
{
    // The prefix of the element node
    GFxXMLDOMString         Prefix;

    // The namespace URL of the element node
    GFxXMLDOMString         Namespace;

    // None of attributes are reference counted because
    // the elementnode is responsible for managing them.
    // No external references should be held. If needed,
    // a seperate copy should be created.
    GFxXMLAttribute*        FirstAttribute;
    GFxXMLAttribute*        LastAttribute;

    // Only first child pointers are reference counted because
    // the children representation follows a first-child next-sibling
    // scheme.
    GPtr<GFxXMLNode>        FirstChild;
    GFxXMLNode*             LastChild;

    GFxXMLElementNode(GFxXMLObjectManager* memMgr);
    GFxXMLElementNode(GFxXMLObjectManager* memMgr, GFxXMLDOMString value);
    ~GFxXMLElementNode();

    // Deep or shallow copy of the element node
    GFxXMLNode*             Clone(bool deep);

    // Child management functions
    void                    AppendChild(GFxXMLNode* xmlNode);
    void                    InsertBefore(GFxXMLNode* child, GFxXMLNode* insert);
    void                    RemoveChild(GFxXMLNode* xmlNode);
    bool                    HasChildren();

    // Attribute management functions
    void                    AddAttribute(GFxXMLAttribute* xmlAttrib);
    bool                    RemoveAttribute(const char* str, UInt len);
    void                    ClearAttributes();
    bool                    HasAttributes();

protected:

    void                    CloneHelper(GFxXMLElementNode* clone, bool deep);
};


// --------------------------------------------------------------------


// 
// Text node (<element>text</element>)
//
// Contains no extra member variables besides the ones
// provided by GFxXMLNode. Exists for posterity.
//
struct GFxXMLTextNode : public GFxXMLNode
{
    GFxXMLTextNode(GFxXMLObjectManager* memMgr, GFxXMLDOMString value);
    ~GFxXMLTextNode();

    // Deep or shallow copy of the text node.
    // Deep copy semantics do not exist for this type.
    GFxXMLNode*             Clone(bool deep);
};


// --------------------------------------------------------------------


// 
// The root of the DOM tree
//
struct GFxXMLDocument : public GFxXMLElementNode
{
    // XML declaration holders
    GFxXMLDOMString         XMLVersion;
    GFxXMLDOMString         Encoding;
    SByte                   Standalone;

    GFxXMLDocument(GFxXMLObjectManager* memMgr);
    ~GFxXMLDocument();

    // Deep or shallow copy of the DOM root
    GFxXMLNode*             Clone(bool deep);
};


// --------------------------------------------------------------------

//
// Root object for maintaining correct XML tree lifetimes
// in the ActionScript runtime. 
// All XMLNode objects created in will point to an instance
// of this object. A root node will point to the top most
// node in the actual C++ DOM tree.
//
struct GFxASXMLRootNode : public GRefCountBaseNTS<GFxASXMLRootNode, GFxStatMV_XML_Mem>
{
    GPtr<GFxXMLNode>    pDOMTree;

    GFxASXMLRootNode(GFxXMLNode* pdom) : pDOMTree(pdom) {}
};


#endif  // #ifndef GFC_NO_XML_SUPPORT

#endif // INC_GFxXML_H
