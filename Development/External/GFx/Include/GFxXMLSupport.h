/**********************************************************************

Filename    :   GFxXMLParserState.h
Content     :   SAX2 compliant parser interface
Created     :   March 7, 2008
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFxXMLParserState_H
#define INC_GFxXMLParserState_H

#include "GConfig.h"

#ifndef GFC_NO_XML_SUPPORT

#include "GFxLoader.h"

#include "GFxPlayerStats.h"

class GASGlobalContext;
class GASStringContext;
class GFxXMLParserHandler;
class GASEnvironment;
class GASObject;

class GFxXMLParser;
class GFxXMLParserExpat;

//
// ActionScript XML file loader interface. 
//
// This interface is used by the GFx core to load an XML file to an 
// ActionScript XML object. An instance of this object is provided by 
// the XML library and passed through the movie load queues. The two 
// methods are called at the appropriate moments from within GFx. Its 
// implementation is defined in the XML library. This interface only 
// serves as a temporary wrapper for the duration of passing through 
// the GFx core.
//
class GFxASXMLFileLoader : public GRefCountBase<GFxASXMLFileLoader, GFxStatMV_XML_Mem>
{
public:
    GFxASXMLFileLoader() { }
    virtual         ~GFxASXMLFileLoader() {}

    //
    // Load a an xml file into a DOM tree using the provide file opener
    //
    virtual void Load(const GString& filename, GFxFileOpener* pfo) = 0;

    //
    // Initialize the ActionScript XML object with the loaded DOM
    //
    virtual void    InitASXml(GASEnvironment* penv, GASObject* pTarget) = 0;
};


// 
// The pluggable XML parser
//
// This interface is implemented by pluggable XML parser instances.
// 
//
class GFxXMLParser : public GRefCountBaseNTS<GFxXMLParser, GStat_Default_Mem>
{
public:    
    virtual ~GFxXMLParser() {}

    //
    // Parse methods
    //
    virtual bool    ParseFile(const char* pfilename, GFxFileOpenerBase* pfo, 
                              GFxXMLParserHandler* pphandler) = 0;
    virtual bool    ParseString(const char* pdata, UPInt len, 
                                GFxXMLParserHandler* pphandler) = 0;
};


//
// XML support state
//
// This interface is used to define a pluggable XML parser as a GFxLoader
// state. When an instance is registered as a loader state, its methods
// will be called appropriately for loading XML files and strings from
// ActionScript. It can also be used by application developers to parse
// XML files from C++.
//
class GFxXMLSupportBase : public GFxState
{
public:
    GFxXMLSupportBase() : GFxState(State_XMLSupport) {}
    virtual         ~GFxXMLSupportBase() {}

    // Load file given the provided file opener and DOM builder.
    virtual bool        ParseFile(const char* pfilename, GFxFileOpenerBase* pfo, 
                                  GFxXMLParserHandler* pdb) = 0;

    // Load a string using the provided DOM builder
    virtual bool        ParseString(const char* pdata, UPInt len, 
                                    GFxXMLParserHandler* pdb) = 0;

    // Register the XML and XMLNode ActionScript classes
    virtual void        RegisterASClasses(GASGlobalContext& gc, 
                                          GASStringContext& sc) = 0;
};

//
// XML support state implementation
//
// A separate implementation is provided so that XML can be compiled
// out completely from GFx core and the resultant application executable
//
class GFxXMLSupport : public GFxXMLSupportBase
{
    GPtr<GFxXMLParser>  pParser;

public:
    GFxXMLSupport(GPtr<GFxXMLParser> pparser) : pParser(pparser) 
    {
        GFC_DEBUG_WARNING(pparser.GetPtr() == NULL, "GFxXMLSupport Ctor - GFxXMLParser argument is NULL!");
    }
    ~GFxXMLSupport() {}

    bool        ParseFile(const char* pfilename, GFxFileOpenerBase* pfo, 
                          GFxXMLParserHandler* pdb);

    bool        ParseString(const char* pdata, UPInt len, 
                            GFxXMLParserHandler* pdb);

    void        RegisterASClasses(GASGlobalContext& gc, 
                                  GASStringContext& sc);
};


//
// Implementation of Set/GetXMLParserState members of the GFxStateBag class.
// They are defined here because the GFxXMLParserState interface is declared
// here.
//
inline void GFxStateBag::SetXMLSupport(GFxXMLSupportBase *ptr)         
{ 
    SetState(GFxState::State_XMLSupport, ptr); 
}
inline GPtr<GFxXMLSupportBase>  GFxStateBag::GetXMLSupport() const     
{ 
    return *(GFxXMLSupportBase*) GetStateAddRef(GFxState::State_XMLSupport); 
}


#endif  // GFC_NO_XML_SUPPORT

#endif  // INC_GFxXMLParserState_H
