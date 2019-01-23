/**********************************************************************

Filename    :   GFxXML.h
Content     :   DOM support
Created     :   December 13, 2007
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFxXML_H
#define INC_GFxXML_H

#include "GConfig.h"

#ifndef GFC_NO_XML_SUPPORT

#include <GFxLoader.h>
#include <GFxStringHash.h>

#include <GFxXMLParser.h>
#include <GFxXMLDocument.h>


//
// This is an implementation of the DOM tree builder. It receives events
// from the parser and builds the tree appropriately, and returns a
// GFxXMLDocument object (the root of the DOM). Applications developers
// are free to instantiate an instance of this class to generate a DOM
// of an XML file using the parser state installed on the loader, or
// a separate parser instance. Instances of the document builder are used
// internally by the XML actionscript class.
//


// 
// DOM tree builder implementation
//
class GFxXMLDOMBuilder : public GFxXMLParserHandler
{
public:
    GFxXMLDOMBuilder(GPtr<GFxXMLSupportBase> pxmlParser, bool ignorews = false);

    GPtr<GFxXMLDocument>    ParseFile(const char* pfilename, GFxFileOpenerBase* pfo, 
                                      GPtr<GFxXMLObjectManager> objMgr = NULL);
    GPtr<GFxXMLDocument>    ParseString(const char* pdata, UPInt len, 
                                        GPtr<GFxXMLObjectManager> objMgr = NULL);

    static void             DropWhiteSpaceNodes(GFxXMLDocument* document);

    // GFxXMLDOMBuilder implementation
    void                    StartDocument();
    void                    EndDocument();
    void                    StartElement(const GFxXMLStringRef& prefix, 
                                         const GFxXMLStringRef& localname,
                                         const GFxXMLParserAttributes& atts);
    void                    EndElement(const GFxXMLStringRef& prefix,
                                       const GFxXMLStringRef& localname);
    void                    PrefixMapping(const GFxXMLStringRef& prefix, 
                                          const GFxXMLStringRef& uri);
    void                    Characters(const GFxXMLStringRef& text);
    void                    IgnorableWhitespace(const GFxXMLStringRef& ws);
    void                    SkippedEntity(const GFxXMLStringRef& name);
    void                    SetDocumentLocator(const GFxXMLParserLocator* plocator);
    void                    Comment(const GFxXMLStringRef& text);
    void                    Error(const GFxXMLParserException& exception);
    void                    FatalError(const GFxXMLParserException& exception);
    void                    Warning(const GFxXMLParserException& exception);

    bool                    IsError() { return bError; }
    UPInt                   GetTotalBytesToLoad() { return TotalBytesToLoad; }
    UPInt                   GetLoadedBytes() { return LoadedBytes; }

private:

    GPtr<GFxXMLSupportBase>     pXMLParserState;

    GPtr<GFxXMLTextNode>    pAppendChainRoot;
    GStringBuffer           AppendText;

    // Locator object installed by the parser that keeps track of the
    // parser state.
    const GFxXMLParserLocator*  pLocator;

    // Stack used to keep track of the element node heirarchies
    GArray< GPtr<GFxXMLElementNode> > ParseStack;

    // Structure used to define xml namespace declaration ownership
    struct PrefixOwnership
    {
        GPtr<GFxXMLPrefix>      Prefix;
        GPtr<GFxXMLElementNode> Owner;
        PrefixOwnership() 
            : Prefix(NULL), Owner(NULL) {}
        PrefixOwnership(GPtr<GFxXMLPrefix> pprefix, GPtr<GFxXMLElementNode> pnode)
            : Prefix(pprefix), Owner(pnode) {}
    };
    // Stacks used to keep track of the current namespaces
    GArray<PrefixOwnership> PrefixNamespaceStack;
    GArray<PrefixOwnership> DefaultNamespaceStack;

    // The document created by the document builder for each parse
    GPtr<GFxXMLDocument>    pDoc;

    bool                    bIgnoreWhitespace;
    bool                    bError;

    UPInt                   TotalBytesToLoad;
    UPInt                   LoadedBytes;
};


#endif  // #ifndef GFC_NO_XML_SUPPORT

#endif // INC_GFxXML_H
