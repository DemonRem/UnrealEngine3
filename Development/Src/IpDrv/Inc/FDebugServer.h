/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * This file contains the definitions of the debug server.
 */

#ifndef __DEBUG_SERVER_H__
#define __DEBUG_SERVER_H__


#include "FMultiThreadedRingBuffer.h"
#include "GameType.h"

/**
 * Protocol flow:
 *
 * Client				Server
 * SA			->
 *					<-	SR<ComputerNameLen><ComputerName><GameNameLen><GameName><GameType><PlatformType><UpdatePort>
 * CC			->
 *					<-	No response, communication begins
 *
 *			Sample types of messages:
 *
 * 1. Text message.
 *					<-	CT<Channel><Message>
 *
 * 2. Command to be executed.
 * ST<Command>	->
 *
 * 3. Other, subsystem specific messages....
 *
 *
 * CD			->
 *					<-	No response, stops update packets
 *
 */

/**
 *	General purpose debug server used by remote tools to communicate with UE3 engine.
 */
class FDebugServer
{
protected:
	/**
	 * Holds information about a given client connection
	 */
	struct FClientConnection
	{
		/**
		 * The IP address of the client
		 */
		FIpAddr Address;
		
		/**
		 *	User data.
		 */
		void* UserData;

		/**
		 * Constructor. Copies the IP address and sets this to a new connection
		 */
		FClientConnection(FIpAddr InAddr) :
			Address(InAddr)
		{
		}

		/** Default ctor */
		FClientConnection(void) :
			UserData(NULL)
		{
		}
	};

	/**
	 * Holds the list of clients that we are multicasting data to. This class
	 * is thread safe
	 */
	class FClientList
	{
		/**
		 * Holds the list of IP addresses to send the data to
		 */
		TArray<FClientConnection> Clients;
		/**
		 * The port to send the data on
		 */
		INT PortNo;
		/**
		 * The synchronization object for thread safe access
		 */
		FCriticalSection* AccessSync;
		/**
		 * Used during iteration of clients
		 */
		INT CurrentIndex;

	public:
		/**
		 * Initalizes the port and creates our synchronization object
		 */
		FClientList(INT InPortNo) :
			PortNo(InPortNo)
		{
			AccessSync = GSynchronizeFactory->CreateCriticalSection();
			check(AccessSync);
		}

		/**
		 * Adds a client to our list
		 *
		 * @param Client the new client to add to the list
		 */
		inline bool AddClient(FIpAddr Client)
		{
			FScopeLock sl(AccessSync);

			// See if the same client already exists (same IP, port doesn't matter)
			for (INT i = 0; i < Clients.Num(); i++)
				if (Clients(i).Address.Addr == Client.Addr)
					return false;

			// Assign the common port number
			Client.Port = PortNo;
			// Add it to our list
			new(Clients)FClientConnection(Client);

			return true;
		}

		/**
		 * Removes a client from our list
		 *
		 * @param Client the client to remove from the list
		 */
		inline void RemoveClient(FIpAddr Client)
		{
			FScopeLock sl(AccessSync);
			UBOOL bFound = FALSE;
			// Search for a client that matches and remove it
			for (INT Index = 0; Index < Clients.Num(); Index++)
				if (Client.Addr == Clients(Index).Address.Addr)
				{
					Clients.Remove(Index);
					return;
				}
		}

		/**
		 * Provides for thread safe forward iteration through the client list.
		 * This method starts the iteration by locking the data. Must call the
		 * EndIterator() function to unlock the list.
		 */
		inline FClientConnection* BeginIterator(void)
		{
			// Manually lock. Locks across scope
			AccessSync->Lock();
			CurrentIndex = 0;
			FClientConnection* Current = NULL;
			if (Clients.Num() > 0)
			{
				Current = &Clients(CurrentIndex);
			}
			return Current;
		}

		/**
		 * Provides for thread safe forward iteration through the client list.
		 * This method ends the iteration by unlocking the data
		 */
		inline void EndIterator(void)
		{
			// Manually unlock. Any blocked threads start now
			AccessSync->Unlock();
		}

		/**
		 * Provides for thread safe forward iteration through the client list.
		 * Moves to the next item in the list. Returns null, when at the end
		 * of the list
		 */
		inline FClientConnection* GetNext(void)
		{
			CurrentIndex++;
			FClientConnection* Current = NULL;
			// Return null if we are at the end to indicate being done
			if (CurrentIndex < Clients.Num())
			{
				Current = &Clients(CurrentIndex);
			}
			return Current;
		}

		/**
		 * Determines the number of connections that are present
		 */
		inline INT GetConnectionCount(void)
		{
			FScopeLock sl(AccessSync);
			return Clients.Num();
		}
	};

	/**
	 * This is the runnable that will perform the async beacon queries
	 */
	class FListenerRunnable : public FRunnable
	{
	protected:
		/**
		 * Holds the port that beacon listens for client broadcasts on
		 */
		INT ListenPort;
		/**
		 * This is the port that will be used to send stats traffic to a client
		 */
		INT TrafficPort;
		/**
		 * This is the "time to die" event. Triggered when UDP notify provider
		 * is shutting down
		 */
		FEvent* TimeToDie;
		/**
		 * The thread safe list of clients that we are sending stats data to
		 */
		FClientList* ClientList;
		/** The socket to listen for requests on */
		FSocket* ListenSocket;
		/** The address in bound requests come in on */
		FInternetIpAddr ListenAddr;

		/** The server this listener is working for. */
		FDebugServer* DebugServer;

		/**
		 * Tells whether output device was added; if some client connects and
		 * it wasn't added it should be added.
		 */
		static UBOOL OutputDeviceAdded;

		/**
		 * Hidden on purpose
		 */
		FListenerRunnable();
	
		/**
		 * Receives data from a given call to Poll(). For the beacon, we
		 * just send an announcement directly to the source address
		 *
		 * @param SrcAddr the source address of the request
		 * @param Data the packet data
		 * @param Count the number of bytes in the packet
		 */
		void ProcessPacket(FIpAddr SrcAddr, BYTE* Data, INT Count);

	// Interface

		virtual void OnClientConnected(const FIpAddr& SrcAddr) {}
		virtual void OnClientDisconnected(const FIpAddr& SrcAddr) {}

	public:
		/**
		 * Copies the specified values to use as the beacon
		 *
		 * @param InPort the port to listen for client requests on
		 * @param InTrafficPort the port that stats updates will be sent on
		 * @param InClients the list that clients will be added to/removed from
		 */
		FListenerRunnable(INT InPort,INT InTrafficPort,FClientList* InClients,FDebugServer* InDebugServer);

	// FRunnable interface

		/**
		 * Binds to the specified port
		 *
		 * @return True if initialization was successful, false otherwise
		 */
		virtual UBOOL Init(void);

		/**
		 * Sends an announcement message on start up and periodically thereafter
		 *
		 * @return The exit code of the runnable object
		 */
		virtual DWORD Run(void);

		/**
		 * Triggers the time to die event causing the thread to exit
		 */
		virtual void Stop(void)
		{
			TimeToDie->Trigger();
		}

		/**
		 * Releases the time to die event
		 */
		virtual void Exit(void)
		{
			GSynchronizeFactory->Destroy(TimeToDie);
			TimeToDie = NULL;
			if (ListenSocket != NULL)
			{
				GSocketSubsystem->DestroySocket(ListenSocket);
				ListenSocket = NULL;
			}
		}
	};

	/**
	 * This is the runnable that will perform the async sends of stats data
	 * to remote clients
	 */
	class FSenderRunnable : public FRunnable
	{
	protected:
		/**
		 * Flag indicating whether it's time to shut down or not
		 */
		UBOOL bIsTimeToDie;
		/**
		 * This is the event that tells the sender thread to wake up and
		 * send its packet data
		 */
		FEvent* WorkEvent;

		/**
		 * The synchronization object for thread safe access to the queue
		 */
		FCriticalSection* AccessSync;
		/**
		 * The thread safe list of clients that we are sending stats data to
		 */
		FClientList* ClientList;

		/** The socket to send responses on */
		FSocket* SenderSocket;

		/** Cyclic buffer for logs multicasted to all clients. */
		FMultiThreadedRingBuffer* LogBuffer;

		/**
		 * Hidden on purpose
		 */
		FSenderRunnable();

	public:
		/**
		 * Copies the specified values to use for the send thread
		 *
		 * @param InSync the critical section to use
		 * @param InClients the list of clients that will receive updates
		 */
		FSenderRunnable(FCriticalSection* InSync,FClientList* InClients,FMultiThreadedRingBuffer* InLogBuffer);

	// FRunnable interface

		/**
		 * Creates the event that will tell the thread there is work to do
		 *
		 * @return True if initialization was successful, false otherwise
		 */
		virtual UBOOL Init(void);

		/**
		 * Waits for the work event to be triggered. Once triggered it sends
		 * each pending packet out to each client in the clients list
		 *
		 * @return The exit code of the runnable object
		 */
		virtual DWORD Run(void);

		/**
		 * Triggers the work event after setting the time to die flag
		 */
		virtual void Stop(void)
		{
			appInterlockedIncrement((INT*)&bIsTimeToDie);
			WorkEvent->Trigger();
		}

		/**
		 * Cleans up any allocated resources
		 */
		virtual void Exit(void);

		/**
		 *	Wakes up sender thread to check if there's any pending job to do.
		 */
		virtual void WakeUp();

	protected:

		// Interface

		virtual void Tick();
	};

	/**
	 * The thread that handles beacon sends and queries
	 */
	FRunnableThread* ListenerThread;
	/**
	 * The runnable object used to handle client beacon requests
	 */
	FRunnable* ListenerRunnable;
	/**
	 * The thread safe list of clients that we are sending stats data to
	 */
	FClientList* ClientList;
	/**
	 * The thread that handles beacon sends and queries
	 */
	FRunnableThread* SenderThread;
	/**
	 * The runnable object used to send data to the connected clients
	 */
	FSenderRunnable* SenderRunnable;
	/**
	 *	Buffer containing logs that are waiting to be sent to all clients.
	 */
	FMultiThreadedRingBuffer*	LogBuffer;

	enum
	{
		DEFAULT_BUFFER_SIZE = 16384,
		MAX_COMMANDS = 16,
		MAX_COMMANDLENGTH = 128,
		DEFAULT_MAX_UDP_PACKET_SIZE = 1024
	};

	/// Maximal allowed UDP packet size
	INT					MaxUDPPacketSize;

	/// Command buffer
	TCHAR				Commands[MAX_COMMANDS][MAX_COMMANDLENGTH];
	/// Number of commands waiting to be executed
	INT					NumCommands;
	/// Command buffer synchronization
	FCriticalSection*	CommandSync;

public:
	/**
	 * Default constructor.
	 */
	FDebugServer();

	/**
	 * Initializes the threads that handle the network layer
	 */
	virtual UBOOL Init(INT ListenPort, INT TrafficPort, INT BufferSize = DEFAULT_BUFFER_SIZE, INT MaxUDPPacketSize = DEFAULT_MAX_UDP_PACKET_SIZE);

	/**
	 * Shuts down the network threads
	 */
	virtual void Destroy();

	/**
	 *	Per-frame debug server tick.
	 */
	virtual void Tick();

// Interface

	virtual FSenderRunnable* CreateSenderRunnable(FCriticalSection* InSync, FClientList* InClients);
	virtual FListenerRunnable* CreateListenerRunnable(INT InPort, INT InTrafficPort, FClientList* InClients);
	virtual void OnReceiveText(const FIpAddr& SrcAddr, const ANSICHAR* Characters, INT Length);

	void SendText(const ANSICHAR* Channel, const ANSICHAR* Text);

protected:
	void SendText(const ANSICHAR* Channel, const ANSICHAR* Text, const INT Length);

};

#endif // __DEBUG_SERVER_H__
