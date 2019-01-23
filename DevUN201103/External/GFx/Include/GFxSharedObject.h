/**********************************************************************

Filename    :   GFxSharedObject.h
Content     :   AS2 Shared object interfaces
Created     :   January 20, 2009
Authors     :   Prasad Silva

Notes       :   
History     :   

Copyright   :   (c) 1998-2009 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXSharedObjectState_H
#define INC_GFXSharedObjectState_H

#include "GConfig.h"
#ifndef GFC_NO_FXPLAYER_AS_SHAREDOBJECT

#include "GMemory.h"
#include "GFxPlayerStats.h"
#include "GFxLoader.h"
#include "GFxPlayer.h"

// ***** GFxSharedObjectState
class GASGlobalContext;
class GASStringContext;
class GFxSharedObjectManagerBase;

//
// A visitor interface for shared object representations
// It is used both in reading and writing data to/from
// physical media and other sources.
//
// Reading occurs from ActionScript when the SharedObject.getLocal
// method is invoked. Writing occurs from ActionScript when the
// SharedObject.flush method is invoked.
//
class GFxSharedObjectVisitor : public GRefCountBaseNTS<GFxSharedObjectVisitor, GStat_Default_Mem>
{
public:
    virtual void Begin() = 0;
    virtual void PushObject( const GString& name ) = 0;
    virtual void PushArray( const GString& name ) = 0;
    virtual void AddProperty( const GString& name, const GString& value, GFxValue::ValueType type) = 0;
    virtual void PopObject() = 0;
    virtual void PopArray() = 0;
    virtual void End() = 0;
};


//
// A manager for shared objects. It provides an interface to 
// load shared object data, and also provides a specialized writer
// to save the shared object data.
//
// A default implementation is provided by FxPlayer.
//
class GFxSharedObjectManagerBase : public GFxState
{
public:
    GFxSharedObjectManagerBase() 
        : GFxState(State_SharedObject) {}

    virtual bool            LoadSharedObject(const GString& name, 
                                             const GString& localPath, 
                                             GFxSharedObjectVisitor* psobj,
                                             GFxFileOpenerBase* pfo) = 0;
    
    //
    // Return a new writer to save the shared object data
    //
    // The returned pointer should be assigned using the following pattern:
    //  GPtr<GFxSharedObjectVisitor> ptr = *pSharedObjManager->CreateWriter(name, localPath);
    //
    virtual GFxSharedObjectVisitor* CreateWriter(const GString& name, const GString& localPath,
        GFxFileOpenerBase* pfileOpener) = 0;
};


//
// Shared state accessors
//
inline void  GFxStateBag::SetSharedObjectManager(GFxSharedObjectManagerBase* ptr) 
{ 
    SetState(GFxState::State_SharedObject, ptr); 
}
inline GPtr<GFxSharedObjectManagerBase> GFxStateBag::GetSharedObjectManager() const
{ 
    return *(GFxSharedObjectManagerBase*) GetStateAddRef(GFxState::State_SharedObject); 
}


#endif // GFC_NO_FXPLAYER_AS_SHAREDOBJECT

#endif // INC_GFXSharedObjectState_H

