/**
 * Copyright © 2006 Epic Games, Inc. All Rights Reserved.
 */

#include "UnIpDrv.h"

/** Structure used to capture an online session */
struct FSecureSessionInfo
{
	/** The nonce for the session */
	ULONGLONG Nonce;
	/** The handle associated with the session */
	HANDLE Handle;
	/** The host address information associated with the session */
	XSESSION_INFO XSessionInfo;

	/** Default ctor that zeros everything */
	inline FSecureSessionInfo(void)
	{
		appMemzero(this,sizeof(FSecureSessionInfo));
	}
};

/**
 * Class responsible for sending/receiving UDP broadcasts for system link match
 * discovery
 */
class FSystemLinkBeacon
{
	/** Builds the broadcast address and caches it */
	FInternetIpAddr BroadcastAddr;
	/** The socket to listen for requests on */
	FSocket* ListenSocket;
	/** The address in bound requests come in on */
	FInternetIpAddr ListenAddr;

public:
	/** Sets the broadcast address for this object */
	inline FSystemLinkBeacon(void) :
		ListenSocket(NULL)
	{
	}

	/** Frees the broadcast socket */
	~FSystemLinkBeacon(void)
	{
		delete ListenSocket;
	}

	/**
	 * Initializes the socket
	 *
	 * @param Port the port to listen on
	 *
	 * @return TRUE if both socket was created successfully, FALSE otherwise
	 */
	UBOOL Init(INT Port);

	/**
	 * Called to poll the socket for pending data. Any data received is placed
	 * in the specified packet buffer
	 *
	 * @param PacketData the buffer to get the socket's packet data
	 * @param BufferSize the size of the packet buffer
	 *
	 * @return the number of bytes read (0 if none or an error)
	 */
	INT Poll(BYTE* PacketData,INT BufferSize);

	/**
	 * Uses the cached broadcast address to send packet to a subnet
	 *
	 * @param Packet the packet to send
	 * @param Length the size of the packet to send
	 */
	inline UBOOL BroadcastPacket(BYTE* Packet,INT Length)
	{
		INT BytesSent = 0;
		return ListenSocket->SendTo(Packet,Length,BytesSent,BroadcastAddr) && BytesSent == Length;
	}
};

/**
 * Data writer that has support for various online structs
 */
template <DWORD Size>
class TOnlineDataWriter :
	public TNboSerializeBuffer<Size>
{
public:
	/**
	 * Adds an FSettingsData member to the buffer
	 */
	friend inline TOnlineDataWriter<Size>& operator<<(TOnlineDataWriter<Size>& Ar,FSettingsData& Data)
	{
		// Write the type
		Ar << Data.Type;
		// Now write out the held data
		switch (Data.Type)
		{
			case SDT_Float:
			{
				Ar << *(FLOAT*)&Data.Value1;
				break;
			}
			case SDT_Int32:
			{
				Ar << Data.Value1;
				break;
			}
			case SDT_Int64:
			{
				Ar << *(QWORD*)&Data.Value1;
				break;
			}
			case SDT_Double:
			{
				Ar << *(DOUBLE*)&Data.Value1;
				break;
			}
			case SDT_Blob:
			{
				// Write the length
				Ar << Data.Value1;
				BYTE* OutData = (BYTE*)Data.Value2;
				// Followed by each byte of data
				for (INT Index = 0; Index < Data.Value1; Index++)
				{
					Ar << OutData[Index];
				}
				break;
			}
			case SDT_String:
			{
				// This will write a length prefixed string
				Ar << (TCHAR*)Data.Value2;
				break;
			}
		}
		return Ar;
	}

	/**
	 * Adds an FLocalizedStringSetting member to the buffer
	 */
	friend inline TOnlineDataWriter<Size>& operator<<(TOnlineDataWriter<Size>& Ar,FLocalizedStringSetting& Context)
	{
		Ar << Context.Id << Context.ValueIndex;
		return Ar;
	}

	/**
	 * Adds an FSettingsProperty member to the buffer
	 */
	friend inline TOnlineDataWriter<Size>& operator<<(TOnlineDataWriter<Size>& Ar,FSettingsProperty& Property)
	{
		Ar << Property.PropertyId;
		Ar << Property.Data;
		return Ar;
	}

	/**
	 * Writes an FOnlineProfileSetting member from the buffer
	 */
	friend inline TOnlineDataWriter<Size>& operator<<(TOnlineDataWriter<Size>& Ar,FOnlineProfileSetting& Setting)
	{
		Ar << Setting.Owner;
		Ar << Setting.ProfileSetting;
		return Ar;
	}
};

/** Typedef for writing profile settings to a buffer the same size as our max */
typedef TOnlineDataWriter<3000> FProfileSettingsWriter;

/**
 * Class used to write data into packets for sending via system link
 */
class FSystemLinkPacketWriter :
	public TOnlineDataWriter<512>
{
public:
	/** Default constructor zeros num bytes*/
	FSystemLinkPacketWriter(void) :
		TOnlineDataWriter<512>()
	{
	}

	/**
	 * Adds an XNADDR to the buffer
	 */
	friend inline FSystemLinkPacketWriter& operator<<(FSystemLinkPacketWriter& Ar,XNADDR& Addr)
	{
		checkSlow(Ar.NumBytes + sizeof(XNADDR) <= 512);
		appMemcpy(&Ar.Data[Ar.NumBytes],&Addr,sizeof(XNADDR));
		Ar.NumBytes += sizeof(XNADDR);
		return Ar;
	}

	/**
	 * Adds an XNKID to the buffer
	 */
	friend inline FSystemLinkPacketWriter& operator<<(FSystemLinkPacketWriter& Ar,XNKID& KeyId)
	{
		checkSlow(Ar.NumBytes + 8 <= 512);
		appMemcpy(&Ar.Data[Ar.NumBytes],KeyId.ab,8);
		Ar.NumBytes += 8;
		return Ar;
	}

	/**
	 * Adds an XNKEY to the buffer
	 */
	friend inline FSystemLinkPacketWriter& operator<<(FSystemLinkPacketWriter& Ar,XNKEY& Key)
	{
		checkSlow(Ar.NumBytes + 16 <= 512);
		appMemcpy(&Ar.Data[Ar.NumBytes],Key.ab,16);
		Ar.NumBytes += 16;
		return Ar;
	}

	/**
	 * Writes an FUniqueNetId member to the buffer
	 */
	friend inline FSystemLinkPacketWriter& operator<<(FSystemLinkPacketWriter& Ar,FUniqueNetId& Id)
	{
		checkSlow(Ar.NumBytes + 8 <= 512);
		// Read the 8 bytes of the XUID
		Ar.Data[Ar.NumBytes++] = Id.Uid[0];
		Ar.Data[Ar.NumBytes++] = Id.Uid[1];
		Ar.Data[Ar.NumBytes++] = Id.Uid[2];
		Ar.Data[Ar.NumBytes++] = Id.Uid[3];
		Ar.Data[Ar.NumBytes++] = Id.Uid[4];
		Ar.Data[Ar.NumBytes++] = Id.Uid[5];
		Ar.Data[Ar.NumBytes++] = Id.Uid[6];
		Ar.Data[Ar.NumBytes++] = Id.Uid[7];
		return Ar;
	}
};

#pragma warning( push )
// Disable used without initialization warning because the reads are initializing
#pragma warning( disable : 4700 )

/**
 * Class used to write data into packets for sending via system link
 */
class FSystemLinkPacketReader
{
	/** Pointer to the data this reader is attached to */
	BYTE* Data;
	/** The size of the data in bytes */
	const INT NumBytes;
	/** The current location in the byte stream for reading */
	INT CurrentOffset;

public:
	/**
	 * Intializes the buffer, size, and zeros the read offset
	 */
	FSystemLinkPacketReader(BYTE* Packet,INT Length) :
		Data(Packet),
		NumBytes(Length),
		CurrentOffset(0)
	{
	}

	/**
	 * Reads an FSettingsData member from the buffer
	 */
	friend inline FSystemLinkPacketReader& operator>>(FSystemLinkPacketReader& Ar,FSettingsData& Data)
	{
		// Read the type
		Ar >> Data.Type;
		// Now read the held data
		switch (Data.Type)
		{
			case SDT_Float:
			{
				FLOAT Val;
				Ar >> Val;
				Data.SetData(Val);
				break;
			}
			case SDT_Int32:
			{
				INT Val;
				Ar >> Val;
				Data.SetData(Val);
				break;
			}
			case SDT_Int64:
			{
				QWORD Val;
				Ar >> Val;
				Data.SetData(Val);
				break;
			}
			case SDT_Double:
			{
				DOUBLE Val;
				Ar >> Val;
				Data.SetData(Val);
				break;
			}
			case SDT_Blob:
			{
				INT Length;
				// Read the length
				Ar >> Length;
				checkSlow(Ar.CurrentOffset + Length <= Ar.NumBytes);
				// Now directly copy the blob data
				Data.SetData(Length,&Ar.Data[Ar.CurrentOffset]);
				Ar.CurrentOffset += Length;
				break;
			}
			case SDT_String:
			{
				FString Val;
				Ar >> Val;
				Data.SetData(Val);
				break;
			}
		}
		return Ar;
	}

	/**
	 * Reads an FLocalizedStringSetting member from the buffer
	 */
	friend inline FSystemLinkPacketReader& operator>>(FSystemLinkPacketReader& Ar,FLocalizedStringSetting& Context)
	{
		Ar >> Context.Id >> Context.ValueIndex;
		return Ar;
	}

	/**
	 * Reads an FSettingsProperty member from the buffer
	 */
	friend inline FSystemLinkPacketReader& operator>>(FSystemLinkPacketReader& Ar,FSettingsProperty& Property)
	{
		Ar >> Property.PropertyId;
		Ar >> Property.Data;
		return Ar;
	}

	/**
	 * Reads an FOnlineProfileSetting member from the buffer
	 */
	friend inline FSystemLinkPacketReader& operator>>(FSystemLinkPacketReader& Ar,FOnlineProfileSetting& Setting)
	{
		Ar >> Setting.Owner;
		Ar >> Setting.ProfileSetting;
		return Ar;
	}

	/**
	 * Reads an XNADDR from the buffer
	 */
	friend inline FSystemLinkPacketReader& operator>>(FSystemLinkPacketReader& Ar,XNADDR& Addr)
	{
		checkSlow(Ar.CurrentOffset + (INT)sizeof(XNADDR) <= Ar.NumBytes);
		appMemcpy(&Addr,&Ar.Data[Ar.CurrentOffset],sizeof(XNADDR));
		Ar.CurrentOffset += sizeof(XNADDR);
		return Ar;
	}

	/**
	 * Reads an XNKID from the buffer
	 */
	friend inline FSystemLinkPacketReader& operator>>(FSystemLinkPacketReader& Ar,XNKID& KeyId)
	{
		checkSlow(Ar.CurrentOffset + 8 <= Ar.NumBytes);
		appMemcpy(KeyId.ab,&Ar.Data[Ar.CurrentOffset],8);
		Ar.CurrentOffset += 8;
		return Ar;
	}

	/**
	 * Reads an XNKEY from the buffer
	 */
	friend inline FSystemLinkPacketReader& operator>>(FSystemLinkPacketReader& Ar,XNKEY& Key)
	{
		checkSlow(Ar.CurrentOffset + 16 <= Ar.NumBytes);
		appMemcpy(Key.ab,&Ar.Data[Ar.CurrentOffset],16);
		Ar.CurrentOffset += 16;
		return Ar;
	}

	/**
	 * Reads a char from the buffer
	 */
	friend inline FSystemLinkPacketReader& operator>>(FSystemLinkPacketReader& Ar,char Ch)
	{
		checkSlow(Ar.CurrentOffset + 1 <= Ar.NumBytes);
		Ch = Ar.Data[Ar.CurrentOffset++];
		return Ar;
	}

	/**
	 * Reads a byte from the buffer
	 */
	friend inline FSystemLinkPacketReader& operator>>(FSystemLinkPacketReader& Ar,BYTE& B)
	{
		checkSlow(Ar.CurrentOffset + 1 <= Ar.NumBytes);
		B = Ar.Data[Ar.CurrentOffset++];
		return Ar;
	}

	/**
	 * Reads an INT from the buffer
	 */
	friend inline FSystemLinkPacketReader& operator>>(FSystemLinkPacketReader& Ar,INT& I)
	{
		return Ar >> *(DWORD*)&I;
	}

	/**
	 * Reads a DWORD from the buffer
	 */
	friend inline FSystemLinkPacketReader& operator>>(FSystemLinkPacketReader& Ar,DWORD& D)
	{
		checkSlow(Ar.CurrentOffset + 4 <= Ar.NumBytes);
		DWORD D1 = Ar.Data[Ar.CurrentOffset + 0];
		DWORD D2 = Ar.Data[Ar.CurrentOffset + 1];
		DWORD D3 = Ar.Data[Ar.CurrentOffset + 2];
		DWORD D4 = Ar.Data[Ar.CurrentOffset + 3];
		D = D1 << 24 | D2 << 16 | D3 << 8 | D4;
		Ar.CurrentOffset += 4;
		return Ar;
	}

	/**
	 * Adds a QWORD to the buffer
	 */
	friend inline FSystemLinkPacketReader& operator>>(FSystemLinkPacketReader& Ar,QWORD& Q)
	{
		checkSlow(Ar.CurrentOffset + 8 <= Ar.NumBytes);
		Q = ((QWORD)Ar.Data[Ar.CurrentOffset + 0] << 56) |
			((QWORD)Ar.Data[Ar.CurrentOffset + 1] << 48) |
			((QWORD)Ar.Data[Ar.CurrentOffset + 2] << 40) |
			((QWORD)Ar.Data[Ar.CurrentOffset + 3] << 32) |
			((QWORD)Ar.Data[Ar.CurrentOffset + 4] << 24) |
			((QWORD)Ar.Data[Ar.CurrentOffset + 5] << 16) |
			((QWORD)Ar.Data[Ar.CurrentOffset + 6] << 8) |
			(QWORD)Ar.Data[Ar.CurrentOffset + 7];
		Ar.CurrentOffset += 8;
		return Ar;
	}

	/**
	 * Reads a FLOAT from the buffer
	 */
	friend inline FSystemLinkPacketReader& operator>>(FSystemLinkPacketReader& Ar,FLOAT& F)
	{
		return Ar >> *(DWORD*)&F;
	}

	/**
	 * Adds a DOUBLE to the buffer
	 */
	friend inline FSystemLinkPacketReader& operator>>(FSystemLinkPacketReader& Ar,DOUBLE& Dbl)
	{
		return Ar >> *(QWORD*)&Dbl;
	}

	/**
	 * Reads a FString from the buffer
	 */
	friend inline FSystemLinkPacketReader& operator>>(FSystemLinkPacketReader& Ar,FString& String)
	{
		// We send strings length prefixed
		INT Len;
		Ar >> Len;
		checkSlow(Ar.CurrentOffset + Len <= Ar.NumBytes);
		// Handle strings of zero length
		if (Len > 0)
		{
			char* Temp = (char*)appAlloca(Len + 1);
			// memcpy it infrom the buffer
			appMemcpy(Temp,&Ar.Data[Ar.CurrentOffset],Len);
			Ar.CurrentOffset += Len;
			// Terminate and copy
			Temp[Len] = '\0';
			String = Temp;
		}
		else
		{
			String.Empty();
		}
		return Ar;
	}

	/**
	 * Reads an FUniqueNetId member from the buffer
	 */
	friend inline FSystemLinkPacketReader& operator>>(FSystemLinkPacketReader& Ar,FUniqueNetId& Id)
	{
		checkSlow(Ar.CurrentOffset + 8 <= Ar.NumBytes);
		// Read the 8 bytes of the XUID
		Id.Uid[0] = Ar.Data[Ar.CurrentOffset++];
		Id.Uid[1] = Ar.Data[Ar.CurrentOffset++];
		Id.Uid[2] = Ar.Data[Ar.CurrentOffset++];
		Id.Uid[3] = Ar.Data[Ar.CurrentOffset++];
		Id.Uid[4] = Ar.Data[Ar.CurrentOffset++];
		Id.Uid[5] = Ar.Data[Ar.CurrentOffset++];
		Id.Uid[6] = Ar.Data[Ar.CurrentOffset++];
		Id.Uid[7] = Ar.Data[Ar.CurrentOffset++];
		return Ar;
	}
};

#pragma warning( pop )

/**
 * Base class for all per task data that needs to be freed once a Live
 * task has completed
 */
struct FLiveAsyncTaskData
{
	/** Base destructor. Here to force proper destruction */
	virtual ~FLiveAsyncTaskData(void)
	{
	}
};

/**
 * Class that holds an async overlapped structure and the delegate to call
 * when the event is complete. This allows the Live ticking to check a set of
 * events for completion and trigger script code without knowing the specific
 * details of the event
 */
class FLiveAsyncTask
{
protected:
	/** The overlapped structure to check for being done */
	XOVERLAPPED Overlapped;
	/** The delegate to fire when the overlapped task is complete */
	FScriptDelegate* ScriptDelegate;
	/** Any per task data that needs to be cleaned up once the task is done */
	FLiveAsyncTaskData* TaskData;
	/** Method for determining the type of task */
	const TCHAR* AsyncTaskName;
#if !FINAL_RELEASE
	/** Tracks how long the task took */
	FLOAT StartTime;
#endif

	/** Hidden on purpose */
	FLiveAsyncTask(void) {}

public:
	/**
	 * Zeros the overlapped structure and sets the delegate to call
	 *
	 * @param InScriptDelegate the delegate to fire off when complete
	 * @param InTaskData the data associated with the task. Freed when compelete
	 * @param InAsyncTaskName pointer to static data identifying the type of task
	 */
	FLiveAsyncTask(FScriptDelegate* InScriptDelegate,FLiveAsyncTaskData* InTaskData,const TCHAR* InAsyncTaskName) :
		ScriptDelegate(InScriptDelegate),
		TaskData(InTaskData),
		AsyncTaskName(InAsyncTaskName)
	{
		appMemzero(&Overlapped,sizeof(XOVERLAPPED));
#if !FINAL_RELEASE
		// Grab the start time, so we can get a rough estimate of async time
		// when the item is deleted
		StartTime = appSeconds();
#endif
	}

	/**
	 * Frees the task data if any was specified
	 */
	virtual ~FLiveAsyncTask(void)
	{
		delete TaskData;
#if !FINAL_RELEASE
		debugf(NAME_DevLive,TEXT("Async task '%s' completed in %f seconds with 0x%08X"),
			AsyncTaskName,
			appSeconds() - StartTime,
			GetCompletionCode());
#endif
	}

	/**
	 * Checks the completion status of the task
	 *
	 * @return TRUE if done, FALSE otherwise
	 */
	FORCEINLINE UBOOL HasTaskCompleted(void) const
	{
		return XHasOverlappedIoCompleted(&Overlapped);
	}

	/**
	 * Returns the status code of the completed task. Assumes the task
	 * has finished.
	 *
	 * @return ERROR_SUCCESS if ok, an HRESULT error code otherwise
	 */
	virtual DWORD GetCompletionCode(void)
	{
		return XGetOverlappedExtendedError(&Overlapped);
	}

	/** Operator to use with passing the overlapped structure */
	FORCEINLINE operator PXOVERLAPPED(void)
	{
		return &Overlapped;
	}

	/**
	 * Returns the delegate for this task
	 */
	FORCEINLINE FScriptDelegate* GetDelegate(void) const
	{
		return ScriptDelegate;
	}

	/**
	 * Used to route the final processing of the data to the correct subsystem
	 * function. Basically, this is a function pointer for doing final work
	 *
	 * @param LiveSubsystem the object to make the final call on
	 *
	 * @return TRUE if this task should be cleaned up, FALSE to keep it
	 */
	virtual UBOOL ProcessAsyncResults(class UOnlineSubsystemLive* LiveSubsystem)
	{
		return TRUE;
	}
};

/**
 * Holds the buffer used in the search
 */
class FLiveAsyncTaskDataSearch :
	public FLiveAsyncTaskData
{
	/** The chunk of memory to place the search results in */
	BYTE* SearchBuffer;
	static const DWORD NumQosRequests = 50;
	/** The array of server addresses to query */
	XNADDR* ServerAddrs[NumQosRequests];
	/** The array of server session ids to use for the query */
	XNKID* ServerKids[NumQosRequests];
	/** The array of server session keys to use for the query */
	XNKEY* ServerKeys[NumQosRequests];
	/** Holds the QoS data returned by the QoS query */
	XNQOS* QosData;
	/** Holds the Live version of our property structure for the async call */
	PXUSER_PROPERTY Properties;
	/** Holds the Live array of contexts to use for the search */
	PXUSER_CONTEXT Contexts;

	/** Hidden on purpose */
	FLiveAsyncTaskDataSearch(void) {}

public:
	/**
	 * Copies the pointer passed in
	 *
	 * @param InBuffer the buffer to use to hold the data
	 * @param InSearch the search object to write the results to
	 */
	FLiveAsyncTaskDataSearch(BYTE* InBuffer) :
		SearchBuffer(InBuffer),
		QosData(NULL),
		Properties(NULL),
		Contexts(NULL)
	{
		appMemzero(ServerAddrs,sizeof(void*) * NumQosRequests);
		appMemzero(ServerKids,sizeof(void*) * NumQosRequests);
		appMemzero(ServerKeys,sizeof(void*) * NumQosRequests);
	}

	/**
	 * Allocates an internal buffer of the specified size
	 *
	 * @param InBuffer the buffer to use to hold the data
	 */
	FLiveAsyncTaskDataSearch(DWORD BufferSize) :
		QosData(NULL),
		Properties(NULL)
	{
		SearchBuffer = new BYTE[BufferSize];
		appMemzero(ServerAddrs,sizeof(void*) * NumQosRequests);
		appMemzero(ServerKids,sizeof(void*) * NumQosRequests);
		appMemzero(ServerKeys,sizeof(void*) * NumQosRequests);
	}

	/** Base destructor. Here to force proper destruction */
	virtual ~FLiveAsyncTaskDataSearch(void)
	{
		delete [] SearchBuffer;
		// Free the QoS data
		if (QosData != NULL)
		{
			XNetQosRelease(QosData);
		}
		delete [] Properties;
		delete [] Contexts;
	}

	/** Operator that gives access to the data buffer */
	FORCEINLINE operator PXSESSION_SEARCHRESULT_HEADER(void)
	{
		return (PXSESSION_SEARCHRESULT_HEADER)SearchBuffer;
	}

	/** Returns the array to use for XNADDRs in the QoS query */
	FORCEINLINE XNADDR** GetXNADDRs(void)
	{
		return ServerAddrs;
	}

	/** Returns the array to use for XNKIDs in the QoS query */
	FORCEINLINE XNKID** GetXNKIDs(void)
	{
		return ServerKids;
	}

	/** Returns the array to use for XNKEYs in the QoS query */
	FORCEINLINE XNKEY** GetXNKEYs(void)
	{
		return ServerKeys;
	}

	/** Returns the pointer to the QoS data to be processed */
	FORCEINLINE XNQOS** GetXNQOS(void)
	{
		return &QosData;
	}

	/**
	 * Allocates memory for the properties array for the async task
	 *
	 * @param NumProps the number of properties that are going to be copied
	 */
	FORCEINLINE PXUSER_PROPERTY AllocateProperties(INT NumProps)
	{
		Properties = new XUSER_PROPERTY[NumProps];
		return Properties;
	}

	/**
	 * Allocates memory for the contexts array for the async task
	 *
	 * @param NumContexts the number of contexts that are going to be copied
	 */
	FORCEINLINE PXUSER_CONTEXT AllocateContexts(INT NumContexts)
	{
		Contexts = new XUSER_CONTEXT[NumContexts];
		return Contexts;
	}
};

/**
 * Handles search specific async task
 */
class FLiveAsyncTaskSearch :
	public FLiveAsyncTask
{
	/** Whether we are waiting for Live or waiting for QoS results */
	UBOOL bIsWaitingForLive;

public:
	/**
	 * Forwards the call to the base class for proper initialization
	 *
	 * @param InScriptDelegate the delegate to fire off when complete
	 * @param InTaskData the data associated with the task. Freed when compelete
	 */
	FLiveAsyncTaskSearch(FScriptDelegate* InScriptDelegate,FLiveAsyncTaskData* InTaskData) :
		FLiveAsyncTask(InScriptDelegate,InTaskData,TEXT("XSessionSearch()")),
		bIsWaitingForLive(TRUE)
	{
	}

	/**
	 * Routes the call to the function on the subsystem for parsing search results
	 *
	 * @param LiveSubsystem the object to make the final call on
	 *
	 * @return TRUE if this task should be cleaned up, FALSE to keep it
	 */
	virtual UBOOL ProcessAsyncResults(class UOnlineSubsystemLive* LiveSubsystem);
};

/**
 * Holds the buffer used in the read async call
 */
class FLiveAsyncTaskDataReadProfileSettings :
	public FLiveAsyncTaskData
{
	/** Which user we are reading the profile data for */
	DWORD UserIndex;
	/** Buffer to hold the data from the game specific fields in */
	BYTE* GameSettingsBuffer;
	/** Holds the IDs for the three game settings fields */
	DWORD GameSettingsIds[3];
	/** Holds the amount of data that was allocated */
	DWORD GameSettingsBufferSize;
	/** The chunk of memory to place the read results in */
	BYTE* Buffer;
	/** The number of IDs the game originally asked for */
	DWORD NumIds;
	/** Chunk of memory that holds the mapped profile setting ids */
	DWORD* ReadIds;
	/** The size of the memory needed to hold the results */
	DWORD ReadIdsSize;
	/** The action currently being performed */
	DWORD Action;
	/** The working buffer for parsing out game settings */
	BYTE WorkingBuffer[3000];
	/** The amount of space used by the working buffer */
	DWORD WorkingBufferUsed;

	/** Hidden on purpose */
	FLiveAsyncTaskDataReadProfileSettings(void) {}

public:
	/** The state the async task is in */
	enum
	{
		/** Currently fetching the settings from our 3 game settings */
		ReadingGameSettings,
		/** Reading from Live the settings that weren't in our game settings */
		ReadingLiveSettings
	};

	/**
	 * Allocates an internal buffer of the specified size
	 *
	 * @param InUserIndex the user the profile read is being done for
	 * @param InNumIds the number of ids to allocate
	 */
	FLiveAsyncTaskDataReadProfileSettings(DWORD InUserIndex,DWORD InNumIds) :
		UserIndex(InUserIndex),
		Buffer(NULL),
		NumIds(InNumIds),
		ReadIdsSize(0)
	{
		// Set the 3 fields that we are going to read
		GameSettingsIds[0] = XPROFILE_TITLE_SPECIFIC1;
		GameSettingsIds[1] = XPROFILE_TITLE_SPECIFIC2;
		GameSettingsIds[2] = XPROFILE_TITLE_SPECIFIC3;
		// Allocate where we are going to fill in the used Ids
		ReadIds = new DWORD[NumIds];
		appMemzero(ReadIds,sizeof(DWORD) * NumIds);
		GameSettingsBufferSize = 0;
		// Figure out how big a buffer is needed
		XUserReadProfileSettings(0,0,3,GameSettingsIds,&GameSettingsBufferSize,NULL,NULL);
		check(GameSettingsBufferSize > 0);
		// Now we can allocate the
		GameSettingsBuffer = new BYTE[GameSettingsBufferSize];
		appMemzero(GameSettingsBuffer,GameSettingsBufferSize);
		// We are reading our saved game settings first
		Action = ReadingGameSettings;
		// Zero the working buffer
		appMemzero(WorkingBuffer,3000);
		WorkingBufferUsed = 0;
	}

	/** Base destructor. Here to force proper destruction */
	virtual ~FLiveAsyncTaskDataReadProfileSettings(void)
	{
		delete [] Buffer;
		delete [] ReadIds;
		delete [] GameSettingsBuffer;
	}

	/** Returns the user index associated with this profile read */
	inline DWORD GetUserIndex(void) const
	{
		return UserIndex;
	}

	/** Gets the buffer that holds the results of the items to be read */
	inline PXUSER_READ_PROFILE_SETTING_RESULT GetProfileBuffer(void)
	{
		return (PXUSER_READ_PROFILE_SETTING_RESULT)Buffer;
	}

	/** Accessor to the DWORD array memory */
	inline DWORD* GetIds(void)
	{
		return ReadIds;
	}

	/** Returns the number of IDs the game asked for */
	inline DWORD GetIdsCount(void)
	{
		return NumIds;
	}

	/** Returns the size in memory of the buffer */
	inline DWORD GetIdsSize(void)
	{
		return ReadIdsSize;
	}

	/** Gets the buffer that holds the results of the game settings read */
	inline PXUSER_READ_PROFILE_SETTING_RESULT GetGameSettingsBuffer(void)
	{
		return (PXUSER_READ_PROFILE_SETTING_RESULT)GameSettingsBuffer;
	}

	/** Gets the set of IDs for the game settings */
	inline DWORD* GetGameSettingsIds(void)
	{
		return GameSettingsIds;
	}

	/** Returns the number of game settings to read */
	inline DWORD GetGameSettingsIdsCount(void)
	{
		return 3;
	}

	/** Returns the size of the buffer */
	inline DWORD GetGameSettingsSize(void)
	{
		return GameSettingsBufferSize;
	}

	/** Returns the current state the task is in */
	inline DWORD GetCurrentAction(void)
	{
		return Action;
	}

	/** Sets the current state the task is in */
	inline void SetCurrentAction(DWORD NewState)
	{
		Action = NewState;
	}

	/** Grants access to the working buffer */
	inline BYTE* GetWorkingBuffer(void)
	{
		return WorkingBuffer;
	}

	/** Grants access to the working buffer size */
	inline DWORD GetWorkingBufferSize(void)
	{
		return WorkingBufferUsed;
	}

	/**
	 * Allocates the space needed by the Live reading portion
	 *
	 * @param IdsCount the number of IDs that are actually being read
	 */
	inline void AllocateBuffer(DWORD IdsCount)
	{
		ReadIdsSize = 0;
		// Ask live for the size
		XUserReadProfileSettings(0,0,IdsCount,ReadIds,&ReadIdsSize,NULL,NULL);
		check(ReadIdsSize > 0);
		// Now do the alloc
		Buffer = new BYTE[ReadIdsSize];
	}

	/**
	 * Coalesces the game settings data into one buffer instead of 3
	 */
	void CoalesceGameSettings(void);
};

/**
 * Handles profile settings read async task
 */
class FLiveAsyncTaskReadProfileSettings :
	public FLiveAsyncTask
{
	/**
	 * Reads the online profile settings from the buffer into the specified array
	 *
	 * @param Settings the array to populate from the game settings
	 */
	void SerializeGameSettings(TArray<FOnlineProfileSetting>& Settings);

public:
	/**
	 * Forwards the call to the base class for proper initialization
	 *
	 * @param InScriptDelegate the delegate to fire off when complete
	 * @param InTaskData the data associated with the task. Freed when compelete
	 */
	FLiveAsyncTaskReadProfileSettings(FScriptDelegate* InScriptDelegate,
		FLiveAsyncTaskData* InTaskData) :
		FLiveAsyncTask(InScriptDelegate,InTaskData,TEXT("XUserReadProfileSettings()"))
	{
	}

	/**
	 * Routes the call to the function on the subsystem for parsing search results
	 *
	 * @param LiveSubsystem the object to make the final call on
	 *
	 * @return TRUE if this task should be cleaned up, FALSE to keep it
	 */
	virtual UBOOL ProcessAsyncResults(class UOnlineSubsystemLive* LiveSubsystem);
};

/**
 * Holds the buffer used in the write async call
 */
class FLiveAsyncTaskDataWriteProfileSettings :
	public FLiveAsyncTaskData
{
	/** Which user we are reading the profile data for */
	DWORD UserIndex;
	/** Buffer that will hold the data during the async write */
	BYTE Buffer[3000];
	/** The three game settings that we are writing to */
	XUSER_PROFILE_SETTING GameSettings[3];

	/** Hidden on purpose */
	FLiveAsyncTaskDataWriteProfileSettings(void) {}

public:
	/**
	 * Allocates an array of profile settings for writing
	 *
	 * @param InUserIndex the user the write is being performed for
	 * @param Source the data to write in our 3 game settings slots
	 * @param Length the amount of data being written
	 */
	FLiveAsyncTaskDataWriteProfileSettings(BYTE InUserIndex,BYTE* Source,DWORD Length) :
		UserIndex(InUserIndex)
	{
		check(Length <= 3000);
		// Clear the byte buffer
		appMemzero(Buffer,3000);
		appMemcpy(Buffer,Source,Length);
		// Clear the game settings so we can set them up
		appMemzero(GameSettings,sizeof(XUSER_PROFILE_SETTING) * 3);
		// Set the common header stuff
		GameSettings[0].dwSettingId = XPROFILE_TITLE_SPECIFIC1;
		GameSettings[0].source = XSOURCE_TITLE;
		GameSettings[0].data.type = XUSER_DATA_TYPE_BINARY;
		GameSettings[1].dwSettingId = XPROFILE_TITLE_SPECIFIC2;
		GameSettings[1].source = XSOURCE_TITLE;
		GameSettings[1].data.type = XUSER_DATA_TYPE_BINARY;
		GameSettings[2].dwSettingId = XPROFILE_TITLE_SPECIFIC3;
		GameSettings[2].source = XSOURCE_TITLE;
		GameSettings[2].data.type = XUSER_DATA_TYPE_BINARY;
		// Determine how much each game setting will hold
		DWORD BlobSize1 = Min<DWORD>(Length,1000);
		Length -= BlobSize1;
		DWORD BlobSize2 = Min<DWORD>(Length,1000);
		Length -= BlobSize2;
		DWORD BlobSize3 = Min<DWORD>(Length,1000);
		check(Length - BlobSize3 == 0);
		// Now point the blobs into the buffer
		GameSettings[0].data.binary.cbData = BlobSize1;
		GameSettings[0].data.binary.pbData = Buffer;
		GameSettings[1].data.binary.cbData = BlobSize2;
		GameSettings[1].data.binary.pbData = &Buffer[BlobSize1];
		GameSettings[2].data.binary.cbData = BlobSize3;
		GameSettings[2].data.binary.pbData = &Buffer[BlobSize2];
	}

	/** Returns the user index associated with this profile read */
	inline DWORD GetUserIndex(void) const
	{
		return UserIndex;
	}

	/** Operator that gives access to the data buffer */
	inline operator PXUSER_PROFILE_SETTING(void)
	{
		return GameSettings;
	}
};

/**
 * Handles writing profile settings asynchronously
 */
class FLiveAsyncTaskWriteProfileSettings :
	public FLiveAsyncTask
{
public:
	/**
	 * Forwards the call to the base class for proper initialization
	 *
	 * @param InScriptDelegate the delegate to fire off when complete
	 * @param InTaskData the data associated with the task. Freed when compelete
	 */
	FLiveAsyncTaskWriteProfileSettings(FScriptDelegate* InScriptDelegate,
		FLiveAsyncTaskData* InTaskData) :
		FLiveAsyncTask(InScriptDelegate,InTaskData,TEXT("XUserWriteProfileSettings()"))
	{
	}

	/**
	 * Routes the call to the function on the subsystem for parsing search results
	 *
	 * @param LiveSubsystem the object to make the final call on
	 *
	 * @return TRUE if this task should be cleaned up, FALSE to keep it
	 */
	virtual UBOOL ProcessAsyncResults(class UOnlineSubsystemLive* LiveSubsystem);
};

/** Changes the state of the game session */
class FLiveAsyncTaskSessionStateChange :
	public FLiveAsyncTask
{
	/** Holds the state to switch to when the async task completes */
	EOnlineGameState StateToTransitionTo;

public:
	/**
	 * Forwards the call to the base class for proper initialization
	 *
	 * @param InState the state to transition the game to once the async task is complete
	 * @param InScriptDelegate the delegate to fire off when complete
	 * @param InAsyncTaskName the name of the task for debugging purposes
	 */
	FLiveAsyncTaskSessionStateChange(EOnlineGameState InState,
		FScriptDelegate* InScriptDelegate,const TCHAR* InAsyncTaskName) :
		FLiveAsyncTask(InScriptDelegate,NULL,InAsyncTaskName),
		StateToTransitionTo(InState)
	{
	}

	/**
	 * Changes the state from the current state to whatever was specified
	 *
	 * @param LiveSubsystem the object to make the final call on
	 *
	 * @return TRUE if this task should be cleaned up, FALSE to keep it
	 */
	virtual UBOOL ProcessAsyncResults(class UOnlineSubsystemLive* LiveSubsystem);
};

/**
 * This async task starts the session and will shrink the public size to the
 * number of active players after arbitration handshaking
 */
class FLiveAsyncTaskStartSession :
	public FLiveAsyncTaskSessionStateChange
{
	/** Whether to shrink the session size to the number of arbitrated players */
	UBOOL bUsesArbitration;

public:
	/**
	 * Forwards the call to the base class for proper initialization
	 *
	 * @param InbUsesArbitration true if using arbitration, false otherwise
	 * @param InScriptDelegate the delegate to fire off when complete
	 * @param InAsyncTaskName the name of the task for debugging purposes
	 */
	FLiveAsyncTaskStartSession(UBOOL InbUsesArbitration,
		FScriptDelegate* InScriptDelegate,const TCHAR* InAsyncTaskName) :
		FLiveAsyncTaskSessionStateChange(OGS_InProgress,InScriptDelegate,InAsyncTaskName),
		bUsesArbitration(InbUsesArbitration)
	{
	}

	/**
	 * Checks the arbitration flag and issues a session size change if arbitrated
	 *
	 * @param LiveSubsystem the object to make the final call on
	 *
	 * @return TRUE if this task should be cleaned up, FALSE to keep it
	 */
	virtual UBOOL ProcessAsyncResults(class UOnlineSubsystemLive* LiveSubsystem);
};

/**
 * Once the session has been created, this class automatically registers
 * the list of local players
 */
class FLiveAsyncTaskCreateSession :
	public FLiveAsyncTask
{
	/** Whether this is a create or a join */
	UBOOL bIsCreate;
	/** Whether the join is from an invite or from a matchmaking search */
	UBOOL bIsFromInvite;

public:
	/**
	 * Forwards the call to the base class for proper initialization
	 *
	 * @param InScriptDelegate the delegate to fire off when complete
	 * @param InIsCreate whether we are creating or joining a match
	 * @param InIsFromInvite whether this join is from an invite or search
	 */
	FLiveAsyncTaskCreateSession(FScriptDelegate* InScriptDelegate,
		UBOOL InIsCreate = TRUE,UBOOL InIsFromInvite = FALSE) :
		FLiveAsyncTask(InScriptDelegate,NULL,TEXT("XSessionCreate()")),
		bIsCreate(InIsCreate),
		bIsFromInvite(InIsFromInvite)
	{
	}

	/**
	 * Routes the call to the function on the subsystem for parsing search results
	 *
	 * @param LiveSubsystem the object to make the final call on
	 *
	 * @return TRUE if this task should be cleaned up, FALSE to keep it
	 */
	virtual UBOOL ProcessAsyncResults(class UOnlineSubsystemLive* LiveSubsystem);
};

/**
 * Holds the buffer used to register all of the local players
 */
class FLiveAsyncTaskDataRegisterLocalPlayers :
	public FLiveAsyncTaskData
{
	/** Holds the list of indices to join the session */
	DWORD Players[4];
	/** Indicates how many local players are joining */
	DWORD Count;
	/** Private slot list */
	BOOL PrivateSlots[4];

public:
	/** Zeroes members */
	FLiveAsyncTaskDataRegisterLocalPlayers(void) :
		Count(0)
	{
		appMemzero(PrivateSlots,sizeof(BOOL) * 4);
	}

	/** Operator that gives access to the data buffer */
	const DWORD* GetPlayers(void)
	{
		return Players;
	}

	/** Returns the number of players being locally registered */
	FORCEINLINE DWORD GetCount(void)
	{
		return Count;
	}

	/** Returns aset of private slots used */
	FORCEINLINE BOOL* GetPrivateSlots(void)
	{
		return PrivateSlots;
	}

	/**
	 * Sets the number of specified slots as private
	 *
	 * @param NumSlots the number of private slots to consume
	 */
	FORCEINLINE SetPrivateSlotsUsed(DWORD NumSlots)
	{
		for (DWORD Index = 0; Index < NumSlots; Index++)
		{
			PrivateSlots[Index] = TRUE;
		}
	}

	/**
	 * Adds a player's index to the list to register
	 */
	FORCEINLINE void AddPlayer(DWORD Index)
	{
		Players[Count++] = Index;
	}
};

/**
 * Holds the data needed to iterate over an enumeration
 */
class FLiveAsyncTaskDataEnumeration :
	public FLiveAsyncTaskData
{
	/** The handle to the enumerator */
	HANDLE Handle;
	/** Block of data used to hold the results */
	BYTE* Data;
	/** Size of the data in bytes */
	DWORD SizeNeeded;
	/** The number of items left to read */
	DWORD NumToRead;
	/** Number of items that were read */
	DWORD ReadCount;
	/** Whether all possible items have been read or not */
	UBOOL bIsAtEndOfList;
	/** The player this read is for */
	DWORD PlayerIndex;

protected:
	/** Hidden to prevent usage */
	FLiveAsyncTaskDataEnumeration() {}

public:
	/**
	 * Initializes members and allocates the buffer needed to read the enumeration
	 *
	 * @param InPlayerIndex the player we are enumerating for
	 * @param InHandle the handle of the enumeration task
	 * @param InSizeNeeded the size of the buffer to allocate
	 * @param InNumToRead the number of items to read from Live
	 */
	FLiveAsyncTaskDataEnumeration(DWORD InPlayerIndex,HANDLE InHandle,DWORD InSizeNeeded,DWORD InNumToRead) :
		Handle(InHandle),
		Data(NULL),
		SizeNeeded(InSizeNeeded),
		NumToRead(InNumToRead),
		ReadCount(0),
		bIsAtEndOfList(FALSE),
		PlayerIndex(InPlayerIndex)
	{
		// Allocate our buffer
		Data = new BYTE[SizeNeeded];
	}

	/**
	 * Frees the resources allocated by this class
	 */
	virtual ~FLiveAsyncTaskDataEnumeration(void)
	{
		delete [] Data;
		CloseHandle(Handle);
	}

	/** Accesses the handle member*/
	FORCEINLINE HANDLE GetHandle(void)
	{
		return Handle;
	}

	/** Accesses the data buffer we allocated */
	FORCEINLINE VOID* GetBuffer(void)
	{
		return (VOID*)Data;
	}

	/** Returns a pointer to the DWORD that gets the number of items returned */
	FORCEINLINE DWORD* GetReturnCountStorage(void)
	{
		return &ReadCount;
	}

	/** Returns the number of friends items that were read */
	FORCEINLINE DWORD GetNumRead(void) const
	{
		return ReadCount;
	}

	/** Updates the number of friends left to read */
	FORCEINLINE void UpdateAmountToRead(void)
	{
		check(NumToRead >= ReadCount);
		NumToRead -= ReadCount;
	}

	/** Returns the player index this read is associated with */
	FORCEINLINE DWORD GetPlayerIndex(void)
	{
		return PlayerIndex;
	}
};

/**
 * Holds the data to enumerate over content as well as open it
 */
class FLiveAsyncTaskContent :
	public FLiveAsyncTaskDataEnumeration
{
	/** Unique drive name that maps to the content files */
	FString ContentDrive;

	/** Friendly that can be used to display the content if needed */
	FString FriendlyName;

	/** Hidden to prevent usage */
	FLiveAsyncTaskContent() {}

public:

	/**
	 * Initializes members and allocates the buffer needed to read the enumeration
	 *
	 * @param InPlayerIndex the player we are enumerating for
	 * @param InHandle the handle of the enumeration task
	 * @param InSizeNeeded the size of the buffer to allocate
	 * @param InNumToRead the number of items to read from Live
	 */
	FLiveAsyncTaskContent(DWORD InPlayerIndex,HANDLE InHandle,DWORD InSizeNeeded,DWORD InNumToRead) :
	  FLiveAsyncTaskDataEnumeration(InPlayerIndex, InHandle, InSizeNeeded, InNumToRead)
	{
	}

	/** Accesses the content drive member*/
	FORCEINLINE const FString& GetContentDrive(void)
	{
		return ContentDrive;
	}

	/** Sets the content drive member*/
	FORCEINLINE void SetContentDrive(const FString& InContentDrive)
	{
		ContentDrive = InContentDrive;
	}

	/** Accesses the friendly name member*/
	FORCEINLINE const FString& GetFriendlyName(void)
	{
		return FriendlyName;
	}

	/** Sets the friendly name member*/
	FORCEINLINE void SetFriendlyName(const FString& InFriendlyName)
	{
		FriendlyName = InFriendlyName;
	}
};

/**
 * This class parses the results of a friends request read and continues to read
 * the list until all friends are read
 */
class FLiveAsyncTaskReadFriends :
	public FLiveAsyncTask
{
public:
	/**
	 * Forwards the call to the base class for proper initialization
	 *
	 * @param InScriptDelegate the delegate to fire off when complete
	 * @param InTaskData the data associated with the task. Freed when compelete
	 */
	FLiveAsyncTaskReadFriends(FScriptDelegate* InScriptDelegate,FLiveAsyncTaskData* InTaskData) :
		FLiveAsyncTask(InScriptDelegate,InTaskData,TEXT("XFriendsCreateEnumerator()"))
	{
	}

	/**
	 * Routes the call to the function on the subsystem for parsing friends
	 * results. Also, continues searching as needed until there are no more
	 * friends to read
	 *
	 * @param LiveSubsystem the object to make the final call on
	 *
	 * @return TRUE if this task should be cleaned up, FALSE to keep it
	 */
	virtual UBOOL ProcessAsyncResults(class UOnlineSubsystemLive* LiveSubsystem);

	/**
	 * Returns the status code of the completed task. Assumes the task
	 * has finished.
	 *
	 * @return ERROR_SUCCESS if ok, an HRESULT error code otherwise
	 */
	virtual DWORD GetCompletionCode(void)
	{
		DWORD Result = XGetOverlappedExtendedError(&Overlapped);
		// Handle being at the end of the list as an OK thing
		return Result == 0x80070012 ? ERROR_SUCCESS : Result;
	}
};


enum EContentTaskMode
{
	/** The task is currently enumerating content packages */
	CTM_Enumerate,
	/** The task is currently opening (creating) a content package */
	CTM_Create,
};

/**
 * This class parses the results of a content request read and continues to read
 * the list until all content is read
 */
class FLiveAsyncTaskReadContent :
	public FLiveAsyncTask
{
private:
	/** What the task is currently performing */
	EContentTaskMode TaskMode;

public:
	/**
	 * Forwards the call to the base class for proper initialization
	 *
	 * @param InScriptDelegate the delegate to fire off when complete
	 * @param InTaskData the data associated with the task. Freed when compelete
	 */
	FLiveAsyncTaskReadContent(FScriptDelegate* InScriptDelegate, FLiveAsyncTaskData* InTaskData) :
		FLiveAsyncTask(InScriptDelegate, InTaskData,TEXT("XContentCreateEnumerator()")),
		TaskMode(CTM_Enumerate)
	{
	}

	/**
	 * Routes the call to the function on the subsystem for parsing friends
	 * results. Also, continues searching as needed until there are no more
	 * friends to read
	 *
	 * @param LiveSubsystem the object to make the final call on
	 *
	 * @return TRUE if this task should be cleaned up, FALSE to keep it
	 */
	virtual UBOOL ProcessAsyncResults(class UOnlineSubsystemLive* LiveSubsystem);
};

/**
 * Holds the buffer used in the async keyboard input
 */
class FLiveAsyncTaskDataKeyboard :
	public FLiveAsyncTaskData
{
	/** The chunk of memory to place the string results in */
	TCHAR* KeyboardInputBuffer;
	/** Whether to use the text vetting APIs to validate the results */
	UBOOL bShouldValidate;
	/** Holds the data needed by the validation API */
	STRING_DATA ValidationInput;
	/** Holds the validation results buffer */
	BYTE* ValidationResults;
	/** The text to place in the edit box by default */
	FString DefaultText;
	/** The text to place in the title */
	FString TitleText;
	/** The description of what the user is entering */
	FString DescriptionText;

	/** Hidden on purpose */
	FLiveAsyncTaskDataKeyboard(void) {}

public:
	/**
	 * Allocates an internal buffer of the specified size and sets
	 * whether we need validation or not
	 *
	 * @param InTitleText the title text to use
	 * @param InDefaultText the default text to use
	 * @param InDescriptionText the description to show the user
	 * @param Length the number of TCHARs to hold
	 * @param bValidate whether to validate the string or not
	 */
	inline FLiveAsyncTaskDataKeyboard(const FString& InTitleText,
		const FString& InDefaultText,const FString& InDescriptionText,
		DWORD Length,UBOOL bValidate) :
		bShouldValidate(bValidate),
		ValidationResults(NULL),
		DefaultText(InDefaultText),
		TitleText(InTitleText),
		DescriptionText(InDescriptionText)
	{
		KeyboardInputBuffer = new TCHAR[Length + 1];
		// Allocate the buffer if the results should be validated
		if (bShouldValidate)
		{
			appMemzero(&ValidationInput,sizeof(STRING_DATA));
			// Allocate and zero the results buffer
			ValidationResults = new BYTE[sizeof(STRING_VERIFY_RESPONSE) + sizeof(HRESULT)];
			appMemzero(ValidationResults,sizeof(STRING_VERIFY_RESPONSE) + sizeof(HRESULT));
		}
	}

	/** Base destructor. Here to force proper destruction */
	virtual ~FLiveAsyncTaskDataKeyboard(void)
	{
		delete [] KeyboardInputBuffer;
		delete ValidationResults;
	}

	/** Operator that gives access to the string buffer */
	inline operator TCHAR*(void)
	{
		return KeyboardInputBuffer;
	}

	/** Operator that gives access to the results buffer */
	inline operator STRING_VERIFY_RESPONSE*(void)
	{
		return (STRING_VERIFY_RESPONSE*)ValidationResults;
	}

	/** Operator that gives access to the validation input buffer */
	inline operator STRING_DATA*(void)
	{
		return &ValidationInput;
	}

	/** Indicates whether we need to validate the data too */
	inline UBOOL NeedsStringValidation(void)
	{
		return bShouldValidate;
	}

	/** Returns the pointer to the default text */
	inline const TCHAR* GetDefaultText(void)
	{
		return *DefaultText;
	}

	/** Returns the pointer to the description text */
	inline const TCHAR* GetDescriptionText(void)
	{
		return *DescriptionText;
	}

	/** Returns the pointer to the title text */
	inline const TCHAR* GetTitleText(void)
	{
		return *TitleText;
	}
};

/**
 * This class parses the results of a keyboard input session and optionally
 * validates the string with the string vetting API
 */
class FLiveAsyncTaskKeyboard :
	public FLiveAsyncTask
{
	/** Whether the task is currently waiting on validation results or not */
	UBOOL bIsValidating;

public:
	/**
	 * Forwards the call to the base class for proper initialization
	 *
	 * @param InScriptDelegate the delegate to fire off when complete
	 * @param InTaskData the data associated with the task. Freed when compelete
	 */
	FLiveAsyncTaskKeyboard(FScriptDelegate* InScriptDelegate,FLiveAsyncTaskDataKeyboard* InTaskData) :
		FLiveAsyncTask(InScriptDelegate,InTaskData,TEXT("XShowKeyboardUI()")),
		bIsValidating(FALSE)
	{
	}

	/**
	 * Copies the resulting string into subsytem buffer. Optionally, will start
	 * another async task to validate the string if requested
	 *
	 * @param LiveSubsystem the object to make the final call on
	 *
	 * @return TRUE if this task should be cleaned up, FALSE to keep it
	 */
	virtual UBOOL ProcessAsyncResults(class UOnlineSubsystemLive* LiveSubsystem);
};

/**
 * Holds the buffer used in the read async call
 */
class FLiveAsyncTaskDataWriteAchievement :
	public FLiveAsyncTaskData
{
	XUSER_ACHIEVEMENT Achievement;

	/** Hidden on purpose */
	FLiveAsyncTaskDataWriteAchievement(void) {}

public:
	/**
	 * Sets the achievement data
	 *
	 * @param InUserIndex the user the achievement is for
	 * @param InAchievementId the achievement being written
	 */
	FLiveAsyncTaskDataWriteAchievement(DWORD InUserIndex,DWORD InAchievementId)
	{
		Achievement.dwUserIndex = InUserIndex;
		Achievement.dwAchievementId = InAchievementId;
	}

	/**
	 * Returns a pointer to the achievement buffer that will persist througout
	 * the async call
	 */
	FORCEINLINE PXUSER_ACHIEVEMENT GetAchievement(void)
	{
		return &Achievement;
	}
};

/**
 * Holds the buffer and user used in the read async call
 */
class FLiveAsyncTaskDataQueryDownloads :
	public FLiveAsyncTaskData
{
	/** Holds the results of the async downloadable content query */
	XOFFERING_CONTENTAVAILABLE_RESULT AvailContent;
	/** The user this is being done for */
	DWORD UserIndex;

	/** Hidden on purpose */
	FLiveAsyncTaskDataQueryDownloads(void) {}

public:
	/**
	 * Sets the user that is performing the query
	 *
	 * @param InUserIndex the user the achievement is for
	 */
	FLiveAsyncTaskDataQueryDownloads(DWORD InUserIndex)
	{
		UserIndex = InUserIndex;
	}

	/**
	 * Returns a pointer to the content availability query that will persist
	 * througout the async call
	 */
	FORCEINLINE XOFFERING_CONTENTAVAILABLE_RESULT* GetQuery(void)
	{
		return &AvailContent;
	}

	/** Returns the user index for this query */
	FORCEINLINE DWORD GetUserIndex(void)
	{
		return UserIndex;
	}
};

/**
 * This class parses the results of a keyboard input session and optionally
 * validates the string with the string vetting API
 */
class FLiveAsyncTaskQueryDownloads :
	public FLiveAsyncTask
{
public:
	/**
	 * Forwards the call to the base class for proper initialization
	 *
	 * @param InScriptDelegate the delegate to fire off when complete
	 * @param InTaskData the data associated with the task. Freed when compelete
	 */
	FLiveAsyncTaskQueryDownloads(FScriptDelegate* InScriptDelegate,FLiveAsyncTaskDataQueryDownloads* InTaskData) :
		FLiveAsyncTask(InScriptDelegate,InTaskData,TEXT("XContentGetMarketplaceCounts()"))
	{
	}

	/**
	 * Copies the download query results into the per user storage on the subsystem
	 *
	 * @param LiveSubsystem the object to make the final call on
	 *
	 * @return TRUE if this task should be cleaned up, FALSE to keep it
	 */
	virtual UBOOL ProcessAsyncResults(class UOnlineSubsystemLive* LiveSubsystem);
};

/** The max number of views that can be written to/read from */
#define MAX_VIEWS 5

/** The max number of stat columns that can be written/read */
#define MAX_STATS 64

/** The max number of players that we can read stats for */
#define MAX_PLAYERS_TO_READ	32

/**
 * This class holds the stats data for async operation and handles the
 * notification of completion
 */
class FLiveAsyncTaskWriteStats :
	public FLiveAsyncTask
{
	/** The view data needed for async completion */
	XSESSION_VIEW_PROPERTIES Views[MAX_VIEWS];
	/** Holds the properties that are written to the views */
	XUSER_PROPERTY Stats[MAX_STATS];

public:
	/**
	 * Forwards the call to the base class for proper initialization
	 *
	 * @param InScriptDelegate the delegate to fire off when complete
	 */
	FLiveAsyncTaskWriteStats(FScriptDelegate* InScriptDelegate) :
		FLiveAsyncTask(InScriptDelegate,NULL,TEXT("XSessionWriteStats()"))
	{
		appMemzero(Views,sizeof(XSESSION_VIEW_PROPERTIES) * MAX_VIEWS);
		appMemzero(Stats,sizeof(XUSER_PROPERTY) * MAX_STATS);
	}

	/** Accessor that returns the buffer that needs to hold the async view data */
	FORCEINLINE XSESSION_VIEW_PROPERTIES* GetViews(void)
	{
		return Views;
	}

	/** Accessor that returns the buffer that needs to hold the async stats data */
	FORCEINLINE XUSER_PROPERTY* GetStats(void)
	{
		return Stats;
	}
};

/**
 * This class holds the stats spec data for async operation and handles the
 * notification of completion
 */
class FLiveAsyncTaskReadStats :
	public FLiveAsyncTask
{
	/** The stats spec data that tells live what to read */
	XUSER_STATS_SPEC StatSpecs[MAX_VIEWS];
	/** The set of players that we are doing the read for */
	XUID Players[MAX_PLAYERS_TO_READ];
	/** The full set of players to read (in case paging must happen) */
	TArray<FUniqueNetId> PlayersToRead;
	/** The number of players in the read request from Live */
	DWORD NumToRead;
	/** The buffer size that was allocated */
	DWORD BufferSize;
	/** The buffer that holds the stats read results */
	BYTE* ReadResults;

public:
	/**
	 * Forwards the call to the base class for proper initialization
	 *
	 * @param NetIds the set of people to read leaderboards for
	 * @param InScriptDelegate the delegate to fire off when complete
	 */
	FLiveAsyncTaskReadStats(const TArray<FUniqueNetId>& NetIds,FScriptDelegate* InScriptDelegate) :
		FLiveAsyncTask(InScriptDelegate,NULL,TEXT("XUserReadStats()")),
		ReadResults(NULL),
		NumToRead(0),
		BufferSize(0)
	{
		appMemzero(StatSpecs,sizeof(XUSER_STATS_SPEC) * MAX_VIEWS);
		appMemzero(Players,sizeof(XUID) * MAX_PLAYERS_TO_READ);
		// Copy the full set to read
		PlayersToRead = NetIds;
		UpdatePlayersToRead();
	}

	/** Frees any allocated memory */
	virtual ~FLiveAsyncTaskReadStats(void)
	{
		delete [] ReadResults;
	}

	/** Updates the players data and the number of items being read */
	FORCEINLINE void UpdatePlayersToRead(void)
	{
		// Figure out how many we are copying
		NumToRead = Min(PlayersToRead.Num(),MAX_PLAYERS_TO_READ);
		// Copy the first set of XUIDs over
		appMemcpy(Players,PlayersToRead.GetData(),sizeof(XUID) * NumToRead);
		// Now remove the number we copied from the full set. If there are any
		// remaining they will be read subsequently
		PlayersToRead.Remove(0,NumToRead);
	}

	/** Accessor that returns the buffer that needs to hold the async spec data */
	FORCEINLINE XUSER_STATS_SPEC* GetSpecs(void)
	{
		return StatSpecs;
	}

	/** Accessor that returns the buffer that holds the players we are reading data for */
	FORCEINLINE XUID* GetPlayers(void)
	{
		return Players;
	}

	/** Returns the number of players in the current read request */
	FORCEINLINE DWORD GetPlayerCount(void)
	{
		return NumToRead;
	}

	/** Returns the of the read buffer */
	FORCEINLINE DWORD GetBufferSize(void)
	{
		return BufferSize;
	}

	/**
	 * Allocates the space requested as the results buffer
	 *
	 * @param Size the number of bytes to allocate
	 */
	FORCEINLINE void AllocateResults(DWORD Size)
	{
		BufferSize = Size;
		ReadResults = new BYTE[Size];
	}

	/** Accessor that returns the buffer to use to hold the read results */
	FORCEINLINE XUSER_STATS_READ_RESULTS* GetReadResults(void)
	{
		return (XUSER_STATS_READ_RESULTS*)ReadResults;
	}

	/**
	 * Tells the Live subsystem to parse the results of the stats read
	 *
	 * @param LiveSubsystem the object to make the final call on
	 *
	 * @return TRUE if this task should be cleaned up, FALSE to keep it
	 */
	virtual UBOOL ProcessAsyncResults(class UOnlineSubsystemLive* LiveSubsystem);
};

/**
 * This class holds the enumeration handle and buffers for async reads
 */
class FLiveAsyncTaskReadStatsByRank :
	public FLiveAsyncTask
{
	/** The stats spec data that tells live what to read */
	XUSER_STATS_SPEC StatSpecs[MAX_VIEWS];
	/** The handle associated with the async enumeration */
	HANDLE hEnumerate;
	/** The buffer that holds the stats read results */
	BYTE* ReadResults;

public:
	/**
	 * Forwards the call to the base class for proper initialization
	 *
	 * @param InScriptDelegate the delegate to fire off when complete
	 */
	FLiveAsyncTaskReadStatsByRank(FScriptDelegate* InScriptDelegate) :
		FLiveAsyncTask(InScriptDelegate,NULL,TEXT("XUserCreateStatsEnumeratorByRank()")),
		hEnumerate(NULL),
		ReadResults(NULL)
	{
		appMemzero(StatSpecs,sizeof(XUSER_STATS_SPEC) * MAX_VIEWS);
	}

	/** Frees any allocated memory */
	virtual ~FLiveAsyncTaskReadStatsByRank(void)
	{
		delete [] ReadResults;
		if (hEnumerate != NULL)
		{
			CloseHandle(hEnumerate);
		}
	}

	/** Accessor that returns the buffer to use to hold the read results */
	FORCEINLINE XUSER_STATS_READ_RESULTS* GetReadResults(void)
	{
		return (XUSER_STATS_READ_RESULTS*)ReadResults;
	}

	/** Accessor that returns the buffer that needs to hold the async spec data */
	FORCEINLINE XUSER_STATS_SPEC* GetSpecs(void)
	{
		return StatSpecs;
	}

	/**
	 * Initializes the handle and buffers for the returned data
	 *
	 * @param hIn the handle for the enumerate
	 * @param Size the size of the buffer to allocate
	 */
	FORCEINLINE void Init(HANDLE hIn,DWORD Size)
	{
		hEnumerate = hIn;
		ReadResults = new BYTE[Size];
	}

	/**
	 * Tells the Live subsystem to parse the results of the stats read
	 *
	 * @param LiveSubsystem the object to make the final call on
	 *
	 * @return TRUE if this task should be cleaned up, FALSE to keep it
	 */
	virtual UBOOL ProcessAsyncResults(class UOnlineSubsystemLive* LiveSubsystem);
};

/**
 * This class handles the steps needed to join a game via invite
 */
class FLiveAsyncTaskJoinGameInvite :
	public FLiveAsyncTask
{
	/** The user index that is joining the game */
	DWORD UserNum;
	/** The buffer that holds the game search results */
	BYTE* Results;
	/** The current state the task is in */
	DWORD State;

//@todo joeg -- add states for shutting down a previous game
	enum { WaitingForSearch, JoiningGame };

public:
	/**
	 * Forwards the call to the base class for proper initialization
	 *
	 * @param InUserNum the user doing the game invite
	 * @param SearchSize the amount of data needed for the results
	 * @param Delegate the delegate to fire off when complete
	 */
	FLiveAsyncTaskJoinGameInvite(DWORD InUserNum,DWORD SearchSize,FScriptDelegate* Delegate) :
		FLiveAsyncTask(Delegate,NULL,TEXT("XInviteGetAcceptedInfo/XSessionSearchByID()")),
		UserNum(InUserNum),
		State(WaitingForSearch)
	{
		Results = new BYTE[SearchSize];
	}

	/** Frees any allocated memory */
	virtual ~FLiveAsyncTaskJoinGameInvite(void)
	{
		delete [] Results;
	}

	/** Accessor to the data buffer */
	FORCEINLINE PXSESSION_SEARCHRESULT_HEADER GetResults(void)
	{
		return (PXSESSION_SEARCHRESULT_HEADER)Results;
	}

	/**
	 * Tells the Live subsystem to continue the async game invite join
	 *
	 * @param LiveSubsystem the object to make the final call on
	 *
	 * @return TRUE if this task should be cleaned up, FALSE to keep it
	 */
	virtual UBOOL ProcessAsyncResults(class UOnlineSubsystemLive* LiveSubsystem);
};

/**
 * This class handles the async task/data for arbitration registration
 */
class FLiveAsyncTaskArbitrationRegistration :
	public FLiveAsyncTask
{
	/** The buffer that holds the registration results */
	BYTE* Results;

public:
	/**
	 * Allocates the result buffer and calls base initialization
	 *
	 * @param BufferSize the size needed for the registration results buffer
	 * @param InScriptDelegate the script delegate to call when done
	 */
	FLiveAsyncTaskArbitrationRegistration(DWORD BufferSize,FScriptDelegate* InScriptDelegate) :
		FLiveAsyncTask(InScriptDelegate,NULL,TEXT("XSessionArbitrationRegister()"))
	{
		Results = new BYTE[BufferSize];
	}

	/** Frees any allocated memory */
	virtual ~FLiveAsyncTaskArbitrationRegistration(void)
	{
		delete [] Results;
	}

	/** Accessor to the data buffer */
	FORCEINLINE PXSESSION_REGISTRATION_RESULTS GetResults(void)
	{
		return (PXSESSION_REGISTRATION_RESULTS)Results;
	}

	/**
	 * Parses the arbitration results and stores them in the arbitration list
	 *
	 * @param LiveSubsystem the object to make the final call on
	 *
	 * @return TRUE if this task should be cleaned up, FALSE to keep it
	 */
	virtual UBOOL ProcessAsyncResults(class UOnlineSubsystemLive* LiveSubsystem);
};

#pragma pack(push,8)

/**
 * This class holds the data used for un/registering a player in a session
 * asynchronously
 */
class FLiveAsyncPlayer :
	public FLiveAsyncTask
{
	/** The XUID of the player that is joining */
	XUID PlayerXuid;

public:
	/**
	 * Forwards the call to the base class for proper initialization
	 *
	 * @param InScriptDelegate the delegate to fire off when complete
	 */
	FLiveAsyncPlayer(XUID InXuid,FScriptDelegate* InScriptDelegate,const TCHAR* Type) :
		FLiveAsyncTask(InScriptDelegate,NULL,Type),
		PlayerXuid(InXuid)
	{
	}

	/** Accessor that returns the buffer holding the xuids */
	FORCEINLINE XUID* GetXuids(void)
	{
		return &PlayerXuid;
	}

	/**
	 * Checks to see if the match is arbitrated and shrinks it by one if it is
	 *
	 * @param LiveSubsystem the object to make the final call on
	 *
	 * @return TRUE if this task should be cleaned up, FALSE to keep it
	 */
	virtual UBOOL ProcessAsyncResults(class UOnlineSubsystemLive* LiveSubsystem);
};

/**
 * This class holds the data used for registering a player in a session
 * asynchronously
 */
class FLiveAsyncRegisterPlayer :
	public FLiveAsyncPlayer
{
	/** The array of private invite flags */
	BOOL bPrivateInvite;
	/** Whether this is the second attempt or not */
	UBOOL bIsSecondTry;

public:
	/**
	 * Forwards the call to the base class for proper initialization
	 *
	 * @param InScriptDelegate the delegate to fire off when complete
	 * @param bWasInvite whether to use private/public slots
	 *&
	 */
	FLiveAsyncRegisterPlayer(XUID InXuid,UBOOL bWasInvite,FScriptDelegate* InScriptDelegate) :
		FLiveAsyncPlayer(InXuid,InScriptDelegate,TEXT("XSessionJoinRemote()")),
		bPrivateInvite(bWasInvite),
		bIsSecondTry(FALSE)
	{
	}

	/** Accessor that returns the buffer holding the private invite flags */
	FORCEINLINE BOOL* GetPrivateInvites(void)
	{
		return &bPrivateInvite;
	}

	/**
	 * Checks to see if the join worked. If this was an invite it may need to
	 * try private and then public.
	 *
	 * @param LiveSubsystem the object to make the final call on
	 *
	 * @return TRUE if this task should be cleaned up, FALSE to keep it
	 */
	virtual UBOOL ProcessAsyncResults(class UOnlineSubsystemLive* LiveSubsystem);
};

#pragma pack(pop)

/**
 * This class holds the data used for handling destruction callback
 */
class FLiveAsyncDestroySession :
	public FLiveAsyncTask
{
	/** The handle of the session to close after deleting */
	HANDLE Handle;

public:
	/**
	 * Forwards the call to the base class for proper initialization
	 *
	 * @param InHandle the handle to close once the task is complete
	 * @param InScriptDelegate the delegate to fire off when complete
	 */
	FLiveAsyncDestroySession(HANDLE InHandle,FScriptDelegate* InScriptDelegate) :
		FLiveAsyncTask(InScriptDelegate,NULL,TEXT("XSessionDelete()")),
		Handle(InHandle)
	{
	}

	/** Cleans up the handle upon deletion */
	~FLiveAsyncDestroySession()
	{
		if (Handle != NULL)
		{
			CloseHandle(Handle);
		}
	}
};

#include "VoiceInterface.h"

#include "OnlineSubsystemLiveClasses.h"
