/**********************************************************************

Filename    :   GFxXMLParser.h
Content     :   SAX2 compliant interface
Created     :   February 21, 2008
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFxXMLParser_H
#define INC_GFxXMLParser_H

#include "GConfig.h"

#ifndef GFC_NO_XML_SUPPORT

#include "GMemory.h"
#include "GFxStringHash.h"

#include <GFxXMLSupport.h>


//
// The classes and structs used by SAX2 parser implementations exist
// here. They define a common interface between the parser implementation
// and the DOM builder. Parsers should invoke the callbacks in the DOM 
// builder interface declared below for appropriate parse events.
//


// 
// Wrapper for a UTF8 string passed to the DOM builder
//
// The data should be managed by the parser. 
//
typedef GFxStringDataPtr GFxXMLStringRef;


// 
// Wrapper for an XML attribute/s passed to the DOM builder
//
struct GFxXMLParserAttribute
{
    // Attribute name
    GFxXMLStringRef      Name;

    // Attribute value
    GFxXMLStringRef      Value;
};

struct GFxXMLParserAttributes
{
    // Contiguous array of GFxXMLParserAttribute objects
    GFxXMLParserAttribute*      Attributes;

    // Number of objects in the array
    UPInt                       Length;
};


// 
// SAX2 Locator2 Object
//
// Register a locator object with the DOM builder before parsing occurs.
// The parser is expected to fill in the locator object values 
// appropriately, since the DOM builder uses the most recent values
// when processing the tree.
//
struct GFxXMLParserLocator
{
    int                 Column;
    int                 Line;

    // XML declaration 
    GFxXMLStringRef     Encoding;
    GFxXMLStringRef     XMLVersion;
    int                 StandAlone;

    UPInt               TotalBytesToLoad;
    UPInt               LoadedBytes;

    GFxXMLParserLocator() : StandAlone(-1) {}
};


// 
// SAX2 ParserException Object
//
// When an error occurs in the parser, pass an instance of this class
// to the DOM builder.
//
struct GFxXMLParserException
{
    // The error description
    GFxXMLStringRef      ErrorMessage;

    GFxXMLParserException(const GFxXMLStringRef& error)
        : ErrorMessage(error) {}
};


// 
// SAX2 Consolidated Handler
//
// The DOM builder is an interface similar to a SAX2 parser handler. 
// The parser implementation is expected to call the appropriate callback
// method for certain events.
//
//
class GFxXMLParserHandler : public GRefCountBase<GFxXMLParserHandler, GFxStatMV_XML_Mem>
{
public:
    GFxXMLParserHandler() { }
    virtual ~GFxXMLParserHandler() {}

    //
    // Beginning and end of documents
    //
    virtual void        StartDocument() = 0;
    virtual void        EndDocument() = 0;

    //
    // Start and end of a tag element
    //
    virtual void        StartElement(const GFxXMLStringRef& prefix,
                                     const GFxXMLStringRef& localname,
                                     const GFxXMLParserAttributes& atts) = 0;
    virtual void        EndElement(const GFxXMLStringRef& prefix,
                                   const GFxXMLStringRef& localname) = 0;

    //
    // Namespace declaration. Next element will be the parent of the mappings
    //
    virtual void        PrefixMapping(const GFxXMLStringRef& prefix, 
                                      const GFxXMLStringRef& uri) = 0;
    //
    // Text data, in between tag elements
    //
    virtual void        Characters(const GFxXMLStringRef& text) = 0;

    //
    // Whitespace
    //
    virtual void        IgnorableWhitespace(const GFxXMLStringRef& ws) = 0;

    //
    // Unprocessed elements
    //
    virtual void        SkippedEntity(const GFxXMLStringRef& name) = 0;

    //
    // GFxXMLParser implementors are REQURIED to set a document locator
    // before any callbacks occur. GFxXMLParserHandler implementations
    // require a locator object for error reporting and correctly processing
    // the encoding, xmlversion and standalone properties.
    //
    virtual void        SetDocumentLocator(const GFxXMLParserLocator* plocator) = 0;

    //
    // Comments
    //
    virtual void        Comment(const GFxXMLStringRef& text) = 0;

    // ErrorHandler Callbacks
    virtual void        Error(const GFxXMLParserException& exception) = 0;
    virtual void        FatalError(const GFxXMLParserException& exception) = 0;
    virtual void        Warning(const GFxXMLParserException& exception) = 0;

};


#endif  // #ifndef GFC_NO_XML_SUPPORT

#endif  // INC_GFxXMLParser_H
