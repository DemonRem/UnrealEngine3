#ifndef NX_FOUNDATION_NXREMOTEDEBUGGER
#define NX_FOUNDATION_NXREMOTEDEBUGGER
/*----------------------------------------------------------------------------*\
|
|						Public Interface to Ageia PhysX Technology
|
|							     www.ageia.com
|
\*----------------------------------------------------------------------------*/

/** \addtogroup foundation
@{
*/


// ---- Groups - Use these when calling NX_DBG macros ----
// Basic object
#define NX_DBG_EVENTGROUP_BASIC_OBJECTS					0x00000001
#define NX_DBG_EVENTGROUP_BASIC_OBJECTS_DYNAMIC_DATA	0x00000002
#define NX_DBG_EVENTGROUP_BASIC_OBJECTS_STATIC_DATA		0x00000004

// Joints
#define NX_DBG_EVENTGROUP_JOINTS						0x00000008
#define NX_DBG_EVENTGROUP_JOINTS_DATA					0x00000010

// Contacts
#define NX_DBG_EVENTGROUP_CONTACTS						0x00000020
#define NX_DBG_EVENTGROUP_CONTACTS_DATA					0x00000040

// Triggers
#define NX_DBG_EVENTGROUP_TRIGGERS						0x00000080

// Profiling
#define NX_DBG_EVENTGROUP_PROFILING						0x00000100

// ---- Masks - Use these when calling connect
#define NX_DBG_EVENTMASK_EVERYTHING						0xFFFFFFFF

// Basic object
#define NX_DBG_EVENTMASK_BASIC_OBJECTS					(NX_DBG_EVENTGROUP_BASIC_OBJECTS)
#define NX_DBG_EVENTMASK_BASIC_OBJECTS_DYNAMIC_DATA		(NX_DBG_EVENTGROUP_BASIC_OBJECTS | NX_DBG_EVENTGROUP_BASIC_OBJECTS_DYNAMIC_DATA)
#define NX_DBG_EVENTMASK_BASIC_OBJECTS_STATIC_DATA		(NX_DBG_EVENTGROUP_BASIC_OBJECTS | NX_DBG_EVENTGROUP_BASIC_OBJECTS_STATIC_DATA)
#define NX_DBG_EVENTMASK_BASIC_OBJECTS_ALL_DATA			(NX_DBG_EVENTMASK_BASIC_OBJECTS_DYNAMIC_DATA | NX_DBG_EVENTMASK_BASIC_OBJECTS_STATIC_DATA)

// Joints
#define NX_DBG_EVENTMASK_JOINTS							(NX_DBG_EVENTGROUP_JOINTS | NX_DBG_EVENTMASK_BASIC_OBJECTS)
#define NX_DBG_EVENTMASK_JOINTS_DATA					(NX_DBG_EVENTGROUP_JOINTS | NX_DBG_EVENTGROUP_JOINTS_DATA |NX_DBG_EVENTMASK_BASIC_OBJECTS)

// Contacts
#define NX_DBG_EVENTMASK_CONTACTS						(NX_DBG_EVENTGROUP_CONTACTS | NX_DBG_EVENTMASK_BASIC_OBJECTS)
#define NX_DBG_EVENTMASK_CONTACTS_DATA					(NX_DBG_EVENTGROUP_CONTACTS | NX_DBG_EVENTGROUP_CONTACTS_DATA | NX_DBG_EVENTMASK_BASIC_OBJECTS)

// Triggers
#define NX_DBG_EVENTMASK_TRIGGERS						(NX_DBG_EVENTGROUP_TRIGGERS)

// Profiling
#define NX_DBG_EVENTMASK_PROFILING						(NX_DBG_EVENTGROUP_PROFILING)

#define NX_DBG_DEFAULT_PORT 5425

enum NxRemoteDebuggerObjectType
{
	NX_DBG_OBJECTTYPE_GENERIC = 0,
	NX_DBG_OBJECTTYPE_ACTOR = 1,
	NX_DBG_OBJECTTYPE_PLANE = 2,
	NX_DBG_OBJECTTYPE_BOX = 3,
	NX_DBG_OBJECTTYPE_SPHERE = 4,
	NX_DBG_OBJECTTYPE_CAPSULE = 5,
	NX_DBG_OBJECTTYPE_CYLINDER = 6,
	NX_DBG_OBJECTTYPE_CONVEX = 7,
	NX_DBG_OBJECTTYPE_MESH = 8,
	NX_DBG_OBJECTTYPE_WHEEL = 9,
	NX_DBG_OBJECTTYPE_JOINT = 10,
	NX_DBG_OBJECTTYPE_CONTACT = 11,
	NX_DBG_OBJECTTYPE_BOUNDINGBOX = 12,
	NX_DBG_OBJECTTYPE_VECTOR = 13,
	NX_DBG_OBJECTTYPE_CAMERA = 14
};

//#define NX_DISABLE_REMOTE_DEBUG

#ifndef NX_DISABLE_REMOTE_DEBUG

#include "NxSimpleTypes.h"
#include "NxPlane.h"
#include "NxVec3.h"

#ifdef CORELIB
#define NX_DBG_IS_CONNECTED() (NxFoundation::FoundationSDK::getInstance().getRemoteDebugger()->isConnected())
#define NX_DBG_FRAME_BREAK() NxFoundation::FoundationSDK::getInstance().getRemoteDebugger()->frameBreak();
#define NX_DBG_CREATE_OBJECT(object, type, className, mask) NxFoundation::FoundationSDK::getInstance().getRemoteDebugger()->createObject(object, type, className, mask);
#define NX_DBG_REMOVE_OBJECT(object, mask) NxFoundation::FoundationSDK::getInstance().getRemoteDebugger()->removeObject(object, mask);
#define NX_DBG_ADD_CHILD(object, child, mask) NxFoundation::FoundationSDK::getInstance().getRemoteDebugger()->addChild(object, child, mask);
#define NX_DBG_REMOVE_CHILD(object, child, mask) NxFoundation::FoundationSDK::getInstance().getRemoteDebugger()->removeChild(object, child, mask);
#define NX_DBG_CREATE_PARAMETER(parameter, object, name, mask) NxFoundation::FoundationSDK::getInstance().getRemoteDebugger()->writeParameter(parameter, object, true, name, mask);
#define NX_DBG_SET_PARAMETER(parameter, object, name, mask) NxFoundation::FoundationSDK::getInstance().getRemoteDebugger()->writeParameter(parameter, object, false, name, mask);
#define NX_DBG_EVENTMASK() (NxFoundation::FoundationSDK::getInstance().getRemoteDebugger()->getMask())
#define NX_DBG_FLUSH() NxFoundation::FoundationSDK::getInstance().getRemoteDebugger()->flush();
#else
#define NX_DBG_IS_CONNECTED() (NxGetFoundationSDK()->getRemoteDebugger()->isConnected())
#define NX_DBG_FRAME_BREAK() NxGetFoundationSDK()->getRemoteDebugger()->frameBreak();
#define NX_DBG_CREATE_OBJECT(object, type, className, mask) NxGetFoundationSDK()->getRemoteDebugger()->createObject(object, type, className, mask);
#define NX_DBG_REMOVE_OBJECT(object, mask) NxGetFoundationSDK()->getRemoteDebugger()->removeObject(object, mask);
#define NX_DBG_ADD_CHILD(object, child, mask) NxGetFoundationSDK()->getRemoteDebugger()->addChild(object, child, mask);
#define NX_DBG_REMOVE_CHILD(object, child, mask) NxGetFoundationSDK()->getRemoteDebugger()->removeChild(object, child, mask);
#define NX_DBG_CREATE_PARAMETER(parameter, object, name, mask) NxGetFoundationSDK()->getRemoteDebugger()->writeParameter(parameter, object, true, name, mask);
#define NX_DBG_SET_PARAMETER(parameter, object, name, mask) NxGetFoundationSDK()->getRemoteDebugger()->writeParameter(parameter, object, false, name, mask);
#define NX_DBG_EVENTMASK() (NxGetFoundationSDK()->getRemoteDebugger()->getMask())
#define NX_DBG_FLUSH() NxGetFoundationSDK()->getRemoteDebugger()->flush();
#endif

/**
\brief Singleton class used to communicate with the Visual Remote Debugger. 
Generally this class is only accessed through the NX_DBG_X macros.
*/
class NxRemoteDebugger
{
public:
	/**
	Connects the SDK to the Visual Remote Debugger application

	\param host The host name of the computer running the VRD, e.g. "localhost".
	\param port The port that the VRD is listening to.
	\param eventMask 32 bit mask used to filter information sent to the debugger, constructed from NX_DBG_EVENTMASK_X macros.
	*/
	virtual void connect(const char* host, unsigned int port = NX_DBG_DEFAULT_PORT, NxU32 eventMask = NX_DBG_EVENTMASK_EVERYTHING) = 0;

	/**
	Flushes the output stream, i.e. forces it to write/send any queued data.
	*/
	virtual void flush() = 0;

	/**
	Returns whether the debugger is connected or not.
	*/
	virtual bool isConnected() = 0;

	/**
	Write a frame break to the output stream.
	*/
	virtual void frameBreak() = 0;

	/**
	Create an object in the output stream.
	
	\param object Object identifier, usually a pointer.
	\param type Type of object to create.
	\param className Class name of the object, e.g. "NxBox" or "PulseRifleBolt".
	\param mask Event group mask that this event is contained in, e.g. NX_DBG_EVENTGROUP_JOINTS.
	*/
	virtual void createObject(void *object, NxRemoteDebuggerObjectType type, const char *className, NxU32 mask) = 0;

	/**
	Remove an object in the output stream.

	\param object Object identifier of the object to remove.
	\param mask Event group mask that this event is contained in, e.g. NX_DBG_EVENTGROUP_JOINTS.
	*/
	virtual void removeObject(void *object, NxU32 mask) = 0;

	/**
	Add an object as child to another object.

	\param object Object identifier of the parent.
	\param child Object identifier of the child.
	\param mask Event group mask that this event is contained in, e.g. NX_DBG_EVENTGROUP_JOINTS.
	*/
	virtual void addChild(void *object, void *child, NxU32 mask) = 0;

	/**
	Remove a child object from another object.

	\param object Object identifier of the parent.
	\param child Object identifier of the child.
	\param mask Event group mask that this event is contained in, e.g. NX_DBG_EVENTGROUP_JOINTS.
	*/
	virtual void removeChild(void *object, void *child, NxU32 mask) = 0;

	/**
	Write a real parameter to output stream.
	
	\param parameter The value to write.
	\param object The object to write a parameter to.
	\param create Must be true at parameter creation, and should be false for all consecutive writes of the same parameter for optimal stream size.
	\param name Name of the parameter.
	\param mask Event group mask that this event is contained in, e.g. NX_DBG_EVENTGROUP_JOINTS.
	*/
	virtual void writeParameter(const NxReal &parameter, void *object, bool create, const char *name, NxU32 mask) = 0;

	/**
	Write a NxU32 parameter to output stream.

	\param parameter The value to write.
	\param object The object to write a parameter to.
	\param create Must be true at parameter creation, and should be false for all consecutive writes of the same parameter for optimal stream size.
	\param name Name of the parameter.
	\param mask Event group mask that this event is contained in, e.g. NX_DBG_EVENTGROUP_JOINTS.
	*/
	virtual void writeParameter(const NxU32 &parameter, void *object, bool create, const char *name, NxU32 mask) = 0;

	/**
	Write a Vector parameter to output stream.

	\param parameter The value to write.
	\param object The object to write a parameter to.
	\param create Must be true at parameter creation, and should be false for all consecutive writes of the same parameter for optimal stream size.
	\param name Name of the parameter.
	\param mask Event group mask that this event is contained in, e.g. NX_DBG_EVENTGROUP_JOINTS.
	*/
	virtual void writeParameter(const NxVec3 &parameter, void *object, bool create, const char *name, NxU32 mask) = 0;

	/**
	Write a Plane parameter to output stream.

	\param parameter The value to write.
	\param object The object to write a parameter to.
	\param create Must be true at parameter creation, and should be false for all consecutive writes of the same parameter for optimal stream size.
	\param name Name of the parameter.
	\param mask Event group mask that this event is contained in, e.g. NX_DBG_EVENTGROUP_JOINTS.
	*/
	virtual void writeParameter(const NxPlane &parameter, void *object, bool create, const char *name, NxU32 mask) = 0;

	/**
	Write an NxMat34 parameter to output stream.

	\param parameter The value to write.
	\param object The object to write a parameter to.
	\param create Must be true at parameter creation, and should be false for all consecutive writes of the same parameter for optimal stream size.
	\param name Name of the parameter.
	\param mask Event group mask that this event is contained in, e.g. NX_DBG_EVENTGROUP_JOINTS.
	*/
	virtual void writeParameter(const NxMat34 &parameter, void *object, bool create, const char *name, NxU32 mask) = 0;

	/**
	Write an NxMat33 parameter to output stream.

	\param parameter The value to write.
	\param object The object to write a parameter to.
	\param create Must be true at parameter creation, and should be false for all consecutive writes of the same parameter for optimal stream size.
	\param name Name of the parameter.
	\param mask Event group mask that this event is contained in, e.g. NX_DBG_EVENTGROUP_JOINTS.
	*/
	virtual void writeParameter(const NxMat33 &parameter, void *object, bool create, const char *name, NxU32 mask) = 0;

	/**
	Write an NxU32 parameter to output stream.

	\param parameter Pointer to a binary chunk of data to write. The first 4 bytes
	must be an NxU32 containing the data size, including those 4 size
	bytes. The data is assumed to be of the same endianness as the output
	stream.
	\param object The object to write a parameter to.
	\param create Must be true at parameter creation, and should be false for all consecutive writes of the same parameter for optimal stream size.
	\param name Name of the parameter.
	\param mask Event group mask that this event is contained in, e.g. NX_DBG_EVENTGROUP_JOINTS.
	*/
	virtual void writeParameter(const NxU8 *parameter, void *object, bool create, const char *name, NxU32 mask) = 0;

	/**
	Write a string parameter to output stream.

	\param parameter The value to write.
	\param object The object to write a parameter to.
	\param create Must be true at parameter creation, and should be false for all consecutive writes of the same parameter for optimal stream size.
	\param name Name of the parameter.
	\param mask Event group mask that this event is contained in, e.g. NX_DBG_EVENTGROUP_JOINTS.
	*/
	virtual void writeParameter(const char *parameter, void *object, bool create, const char *name, NxU32 mask) = 0;

	/**
	Write a boolean parameter to output stream.

	\param parameter The value to write.
	\param object The object to write a parameter to.
	\param create Must be true at parameter creation, and should be false for all consecutive writes of the same parameter for optimal stream size.
	\param name Name of the parameter.
	\param mask Event group mask that this event is contained in, e.g. NX_DBG_EVENTGROUP_JOINTS.
	*/
	virtual void writeParameter(const bool &parameter, void *object, bool create, const char *name, NxU32 mask) = 0;

	/**
	Write an object ID parameter to output stream.

	\param parameter The value to write.
	\param object The object to write a parameter to.
	\param create Must be true at parameter creation, and should be false for all consecutive writes of the same parameter for optimal stream size.
	\param name Name of the parameter.
	\param mask Event group mask that this event is contained in, e.g. NX_DBG_EVENTGROUP_JOINTS.
	*/
	virtual void writeParameter(const void *parameter, void *object, bool create, const char *name, NxU32 mask) = 0;

	/**
	Returns the mask the debug stream was created with
	*/
	virtual NxU32 getMask() = 0;


	/**
	Returns the object that is currently picked by the debugger
	*/
	virtual void *getPickedObject() = 0;

	/**
	Returns the current picking point in the debugger
	*/
	virtual NxVec3 getPickPoint() = 0;

protected:
	virtual ~NxRemoteDebugger(){};
};

#else

#define NX_DBG_FRAME_BREAK(mask) ((void) 0);
#define NX_DBG_CREATE_OBJECT(object, type, name, mask) ((void) 0);
#define NX_DBG_REMOVE_OBJECT(object, mask) ((void) 0);
#define NX_DBG_ADD_CHILD(object, child, mask) ((void) 0);
#define NX_DBG_REMOVE_CHILD(object, child, mask) ((void) 0);
#define NX_DBG_CREATE_PARAMETER(parameter, object, name, mask) ((void) 0);
#define NX_DBG_SET_PARAMETER(parameter, object, name, mask) ((void) 0);
#define NX_DBG_IS_CONNECTED() (false)
#define NX_DBG_EVENTMASK() (0)
#define NX_DBG_FLUSH() ((void) 0);

#endif // NX_DISABLE_REMOTE_DEBUG

/** @} */
#endif // NX_FOUNDATION_NXREMOTEDEBUGGER
//AGCOPYRIGHTBEGIN
///////////////////////////////////////////////////////////////////////////
// Copyright © 2005 AGEIA Technologies.
// All rights reserved. www.ageia.com
///////////////////////////////////////////////////////////////////////////
//AGCOPYRIGHTEND
