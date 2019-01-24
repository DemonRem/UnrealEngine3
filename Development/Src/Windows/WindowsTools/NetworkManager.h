/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */

#ifndef _NETWORKMANAGER_H_
#define _NETWORKMANAGER_H_

#include <map>
#include "Common.h"
#include "CriticalSection.h"
#include "Socket.h"
#include "WindowsTarget.h"

//forward declarations
enum EDebugServerMessageType;

typedef FReferenceCountPtr<CWindowsTarget> TargetPtr;
typedef map<TARGETHANDLE, TargetPtr> TargetMap;

/**
 * This class contains all of the network code for interacting with windows targets.
 */
class FNetworkManager
{
private:
	/** True if the FNetworkManager instance has been initialized. */
	bool bInitialized;
	/** Information about the system. */
	SYSTEM_INFO SysInfo;

	/** A map of targets. */
	TargetMap Targets;
	/** Synch object for accessing the target map/list. */
	FCriticalSection TargetsLock;
	
	/** TCP client used to communicate with the game. */
	FSocket* BroadcastSocket;
	time_t LastPingTime;

	/** The broadcast address to ping for iThings */
	sockaddr_in BroadcastAddr;

	/** The subnet to iterate to ping for iThings */
	bool bDoSubnetSearch;
	sockaddr_in SubnetSearchAddr;
	/**
	 * Removes the target with the specified address.
	 *
	 * @param	Handle		The handle of the target to be removed.
	 */
	void RemoveTarget(TARGETHANDLE Handle);

public:
	FNetworkManager();
	~FNetworkManager();

	/**
	 * Initalizes winsock and the FNetworkManager instance.
	 */
	void Initialize();

	/**
	 * Cleans up winsock and all of the resources allocated by the FNetworkManager instance.
	 */
	void Cleanup();

	/**
	 * Tells network manager to explicitly search a class-D block represented by ip when doing broadcasts
	 * 
	 * @param	ip		The IP of the subnet (i.e. "10.1.33.0"). 
	 * Note that the last digit doesn't matter but it should be a number so it parses.
	 */
	void SetSubnetSearch(const char* ip);

	/**
	 * Retrieves a target with the specified IP Address.
	 *
	 * @param	Address		The address of the target to retrieve.
	 * @return	NULL if the target could not be found, otherwise a valid reference pointer.
	 */
	TargetPtr GetTarget(const sockaddr_in &Address);

	/**
	 * Retrieves a target with the specified IP Address.
	 *
	 * @param	Handle		The handle of the target to retrieve.
	 * @return	NULL if the target could not be found, otherwise a valid reference pointer.
	 */
	TargetPtr GetTarget(const TARGETHANDLE Handle);

	/**
	 * Connects to the target with the specified name.
	 *
	 * @param	TargetName		The name of the target to connect to.
	 * @return	Handle to the target that was connected or else INVALID_TARGETHANDLE.
	 */
	TARGETHANDLE ConnectToTarget(const wchar_t* TargetName);

	/**
	 * Connects to the target with the specified handle.
	 *
	 * @param	Handle		The handle of the target to connect to.
	 */
	bool ConnectToTarget(TARGETHANDLE Handle);

	/**
	 * Returns the number of targets available.
	 */
	int GetNumberOfTargets();

	/**
	 * Exists for compatability with UnrealConsole. Index is disregarded and CurrentTarget is disconnected if it contains a valid pointer.
	 *
	 * @param	Handle		Handle to the target to disconnect.
	 */
	void DisconnectTarget(const TARGETHANDLE Handle);
	
	/**
	 *	Sends message using given UDP client.
	 *	@param Handle			client to send message to
	 *	@param MessageType		sent message type
	 *	@param Message			actual text of the message
	 *	@return true on success, false otherwise
	 */
	bool SendToConsole(const TARGETHANDLE Handle, const wchar_t* Message );

	/**
	 * Broadcast a ping and call UpdateTargets()
	 */
	void DetermineTargets();

	/**
	 * Read from the socket any replies from the device pings
	 * (ideally this should be called more often than DetermineTargets and would signal the host with a callback when the targets list changes)
	 */
	void UpdateTargets();

	/**
	 * Gets the default target.
	 */
	TargetPtr GetDefaultTarget();

	/**
	 * Retrieves a handle to each available target.
	 *
	 * @param	OutTargetList			An array to copy all of the target handles into.
	 * @param	InOutTargetListSize		This variable needs to contain the size of OutTargetList. When the function returns it will contain the number of target handles copied into OutTargetList.
	 */
	void GetTargets(TARGETHANDLE *OutTargetList, int *InOutTargetListSize);

	/**
	 * Forces a stub target to be created to await connection
	 *
	 * @returns Handle of new stub target
	 */
	TARGETHANDLE ForceAddTarget( const wchar_t* TargetIP );
};

#endif
