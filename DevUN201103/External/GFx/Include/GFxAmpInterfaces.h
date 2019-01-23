/**********************************************************************

Filename    :   GFxAmpAppControlCallbackInterface.h
Content     :   Interfaces for customizing the behavior of AMP
Created     :   December 2009
Authors     :   Alex Mantzaris

Copyright   :   (c) 2005-2009 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INCLUDE_GFX_AMP_APP_CONTROL_CALLBACK_INTERFACE_H
#define INCLUDE_GFX_AMP_APP_CONTROL_CALLBACK_INTERFACE_H

#include "GRefCount.h"
#include "GFxAmpMessage.h"

class GFxAmpMessageAppControl;

// 
// Custom AMP behavior is achieved by overriding the classes in this file
//

// Both the AMP client and AMP server are derived from GFxAmpMsgHandler
// so that messages can invoke the proper handler function without knowledge of the 
// concrete handler class, and so that messages can be handled differently
// in the client and server
class GFxAmpMsgHandler : public GRefCountBase<GFxAmpMsgHandler, GStat_Default_Mem>
{
public:
    GFxAmpMsgHandler() : RecvPort(0), RecvAddress(0)                        { }
    virtual ~GFxAmpMsgHandler()                                             { }
    virtual bool    HandleSwdFile(const GFxAmpMessageSwdFile*)              { return false; }
    virtual bool    HandleSwdRequest(const GFxAmpMessageSwdRequest*)        { return false; }
    virtual bool    HandleSourceFile(const GFxAmpMessageSourceFile*)        { return false; }
    virtual bool    HandleSourceRequest(const GFxAmpMessageSourceRequest*)  { return false; }
    virtual bool    HandleProfileFrame(const GFxAmpMessageProfileFrame*)    { return false; }
    virtual bool    HandleCurrentState(const GFxAmpMessageCurrentState*)    { return false; }
    virtual bool    HandleLog(const GFxAmpMessageLog*)                      { return false; }
    virtual bool    HandleAppControl(const GFxAmpMessageAppControl*)        { return false; }
    virtual bool    HandlePort(const GFxAmpMessagePort*)                    { return false; }
    virtual bool    HandleImageRequest(const GFxAmpMessageImageRequest*)    { return false; }
    virtual bool    HandleImageData(const GFxAmpMessageImageData*)          { return false; }
    void SetRecvAddress(UInt32 port, UInt32 address, char* name)
    {
        RecvPort = port;
        RecvAddress = address;
        RecvName = name;
    }
    virtual bool    IsInitSocketLib() const                             { return true; }
protected:
    UInt32  RecvPort;
    UInt32  RecvAddress;
    GString RecvName;
};

// GFxAmpAppControlInterface::HandleAmpRequest is called by GFxAmpServer
// whenever a GFxAmpMessageAppControl has been received
class GFxAmpAppControlInterface
{
public:
    virtual ~GFxAmpAppControlInterface() { }
    virtual bool HandleAmpRequest(const GFxAmpMessageAppControl* pMessage) = 0;
};

// GFxAmpSendInterface::OnSendLoop is called once per "frame" 
// from the GFxAmpThreadManager send thread
class GFxAmpSendInterface
{
public:
    virtual ~GFxAmpSendInterface() { }
    virtual bool OnSendLoop() = 0;
};

// GFxAmpConnStatusInterface::OnStatusChanged is called by GFxAmpThreadManager 
// whenever a change in the connection status has been detected
class GFxAmpConnStatusInterface
{
public:
    enum StatusType
    {
        CS_Idle         = 0x0,
        CS_Connecting   = 0x1,
        CS_OK           = 0x2,
        CS_Failed       = 0x3,
    };

    virtual ~GFxAmpConnStatusInterface() { }
    virtual void OnStatusChanged(StatusType newStatus, StatusType oldStatus, const char* message) = 0;
};

#endif
