/*=============================================================================
	WindowsTools/WindowsSupport.cpp: Windows platform support.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/ 

#define NON_UE3_APPLICATION

#include "Common.h"
#include "StringUtils.h"
#include "..\..\IpDrv\Inc\GameType.h"
#include "..\..\IpDrv\Inc\DebugServerDefs.h"
#include "ByteStreamConverter.h"
#include "UDPClient.h"

/// Available platform types
enum EPlatformType
{
    EPlatformType_Unknown = 0x00,
	EPlatformType_Windows = 0x01,
	EPlatformType_Xenon = 0x04,
	EPlatformType_PS3 = 0x08,
	EPlatformType_Linux = 0x10,
	EPlatformType_Mac = 0x20,
	EPlatformType_Max
};

/**
 *	Returns platform type name for enum value.
 */
static const char* ToString(EPlatformType type)
{
	switch (type)
	{
		case EPlatformType_Windows: return "Windows";
		case EPlatformType_Xenon: return "Xenon";
		case EPlatformType_PS3: return "PS3";
		case EPlatformType_Linux: return "Linux";
		case EPlatformType_Mac: return "Mac";
	}
	return "Unknown";
}

/**
 *	Simplified Windows support to be used for sending and receiving messages.
 *	Used for Debug Channel for PC.
 */
class CWindowsSupport
#ifndef TESTING_WINDOWS_TOOLS
	: public FConsoleSupport
#endif
{
protected:

	/// Text message received from server
	class CTextMessage
	{
	public:
		CTextMessage(const string& InChannel, const string& InText):
			Channel(InChannel),
			Text(InText),
			Previous(NULL)
		{}

		string Channel;
		string Text;
		CTextMessage* Previous;
	};

	/// Cyclic buffer storing text messages
	class CCyclicTextMessageBuffer
	{
	protected:
		CTextMessage* MessageList; //!< Pointer to first message
		CTextMessage* LastMessage; //!< Pointer to last message
	public:
		CCyclicTextMessageBuffer() :
			MessageList(NULL),
			LastMessage(NULL)
		{
		}

		~CCyclicTextMessageBuffer()
		{
			string Channel, Text;
			while (GetMessage(Channel, Text)) {}
		}

		/**
		 *	Add message to the beginning.
		 */
		void AddMessage(const string& Channel, const string& Text)
		{
			CTextMessage* TextMessage = new CTextMessage(Channel, Text);
			TextMessage->Previous = NULL;

			if (MessageList)
				MessageList->Previous = TextMessage;
			else
				LastMessage = TextMessage;
			MessageList = TextMessage;
		}

		/**
		 *	Retrieve the oldest message.
		 */
		bool GetMessage(string& Channel, string& Text)
		{
			if (!LastMessage)
				return false;

			Channel = LastMessage->Channel;
			Text = LastMessage->Text;
			
			CTextMessage* Temp = LastMessage;
			LastMessage = LastMessage->Previous;
			delete Temp;
			if (!LastMessage)
				MessageList = NULL;

			return true;
		}
	};

	/// Representation of a single UE3 instance running on PC
	class CWindowsTarget
	{
	public:
		CUDPClient UDPClient; //!< UDP client used to connect to this target

		wstring Name; //!< User friendly name of the target
		wstring ComputerName; //!< Computer name
		wstring GameName; //!< Game name
		wstring Configuration; //!< Configuration name
		EGameType GameType; //!< Game type id
		wstring GameTypeName; //!< Game type name
		EPlatformType PlatformType; //!< Platform id
		int ListenPortNo; //!< Port the server listenes on

		__time64_t TimeRegistered; //!< System time of target registration ('server response' received)

		CCyclicTextMessageBuffer ReceivedTextBuffer; //!< Received text cyclic buffer

		// Make the name user friendly
		void UpdateName()
		{
			unsigned int Address = UDPClient.GetIP();
			Name =
				ToWString( inet_ntoa(*(in_addr*) &Address) ) +
				L" (" + ComputerName + L" )";
		}

	};

	CUDPClient Broadcaster; //!< Used to broadcast 'server announce' message
	CUDPClient ServerResponseListener; //!< Used to receive 'server response' from server
	CUDPClient MessageReceiver; //!< Used to receive messages from server

	vector<CWindowsTarget*> Targets; //!< Available targets

	bool NetworkInitialized; //!< Tells whether the network was succesfully initialized

public:
	CWindowsSupport() :
		NetworkInitialized(false)
	{
	}

	void InitNetwork()
	{
		if (NetworkInitialized)
			return;

		DebugOutput("Initializing network...\n");

		// Init Windows Sockets
		WSADATA WSAData;
		WORD WSAVersionRequested = MAKEWORD(2, 2);
		if (WSAStartup(WSAVersionRequested, &WSAData) != 0)
			return;
		DebugOutput("WSA Sockets Initialized.\n");

		// Init broadcaster
		Broadcaster.SetPort(DefaultDebugChannelReceivePort);
		if (!Broadcaster.CreateSocket())
		{
			DebugOutput("Failed to create broadcast socket.\n");
			return;
		}
		Broadcaster.SetBroadcasting(true);

		// Init 'server announce' listener
		ServerResponseListener.SetPort(DefaultDebugChannelSendPort);
		if (!ServerResponseListener.CreateSocket() ||
			!ServerResponseListener.SetNonBlocking(true) ||
			!ServerResponseListener.Bind())
		{
			DebugOutput("Failed to create server response receiver.\n");
			return;
		}

		// Init message receiver
		MessageReceiver.SetPort(DefaultDebugChannelTrafficPort);
		if (!MessageReceiver.CreateSocket() ||
			!MessageReceiver.SetNonBlocking(true) ||
			!MessageReceiver.Bind())
		{
			DebugOutput("Failed to create message receiver.\n");
			return;
		}

		NetworkInitialized = true;
		DebugOutput("WindowsTools sockets created succesfully.\n");
	}

	void Initialize(const wchar_t* /*GameName*/, const wchar_t* /*Configuration*/)
	{
		// Do nothing
	}

	const wchar_t* GetConsoleName()
	{
		return L"PC";
	}

	unsigned int GetIPAddress(int TargetIndex)
	{
		CWindowsTarget* Target = GetTarget(TargetIndex);
		if (!Target) return 0;

		return Target->UDPClient.GetIP();
	}
	
	bool GetIntelByteOrder()
	{
		return true;
	}

	int GetNumTargets()
	{
		// Do the network initialization (if needed)
		InitNetwork();

		// Search for available targets
		DetermineTargets();

		return (int) Targets.size();
	}

	const wchar_t* GetTargetName(int TargetIndex)
	{
		CWindowsTarget* Target = GetTarget(TargetIndex);
		if (!Target) return NULL;
		return Target->Name.c_str();
	}

	const wchar_t* GetTargetComputerName(int TargetIndex)
	{
		CWindowsTarget* Target = GetTarget(TargetIndex);
		if (!Target) return NULL;
		return Target->ComputerName.c_str();
	}

	const wchar_t* GetTargetGameName(int TargetIndex)
	{
		CWindowsTarget* Target = GetTarget(TargetIndex);
		if (!Target) return NULL;
		return Target->GameName.c_str();
	}

	const wchar_t* GetTargetGameType(int TargetIndex)
	{
		CWindowsTarget* Target = GetTarget(TargetIndex);
		if (!Target) return NULL;
		return Target->GameTypeName.c_str();
	}

	bool PlatformNeedsToCopyFiles()
	{
		return false;
	}

	int ConnectToTarget(const wchar_t* TargetName)
	{
		// Find the target
		CWindowsTarget* Target = NULL;
		int TargetIndex = -1;

		for (unsigned int i = 0; i < Targets.size(); i++)
			if (Targets[i] && Targets[i]->Name == TargetName)
			{
				Target = Targets[i];
				TargetIndex = i;
				break;
			}

		// Target not found
		if (!Target)
			return -1;

		// Create the socket used to send messages to server
		if (!Target->UDPClient.CreateSocket())
			return -1;

		// Inform server about connection
		const bool connectionRequestResult =
			SendToConsole(Target->UDPClient, EDebugServerMessageType_ClientConnect);
		if (!connectionRequestResult)
		{
			Target->UDPClient.DestroySocket();
			return -1;
		}

		// Add initial info for the user
		const unsigned int IP = Target->UDPClient.GetIP();
		Target->ReceivedTextBuffer.AddMessage("CONNECTION",
			string("Connected to server:") +
			"\n\tIP: " + inet_ntoa(*(in_addr*) &IP) +
			"\n\tComputer name: " + ToString(Target->ComputerName) +
			"\n\tGame name: " + ToString(Target->GameName) +
			"\n\tGame type: " + ToString(Target->GameType) +
			"\n\tPlatform: " + ToString(Target->PlatformType) + "\n");

		return TargetIndex;
	}

	void DisconnectFromTarget(int TargetIndex)
	{
		CWindowsTarget* Target = GetTarget(TargetIndex);
		if (!Target) return;

		// Inform server about disconnection
		SendToConsole(Target->UDPClient, EDebugServerMessageType_ClientDisconnect);
	}

	bool Reboot(int /*TargetIndex*/)
	{
		return false;
	}

	bool RebootAndRun(int /*TargetIndex*/, const wchar_t* /*Configuration*/, const wchar_t* /*BaseDirectory*/, const wchar_t* /*GameName*/, const wchar_t* /*URL*/)
	{
		return false;
	}

	bool RunGame(const wchar_t* /*URL*/, wchar_t* /*OutputConsoleCommand*/, int /*CommandBufferSize*/)
	{
		return false;
	}
#ifndef TESTING_WINDOWS_TOOLS
	EConsoleState GetConsoleState(int TargetIndex)
	{
		CWindowsTarget* Target = GetTarget(TargetIndex);
		if (!Target) return CS_NotRunning;

		return CS_Running;
	}
#endif
	void ResolveAddressToString(unsigned int Address, wchar_t* OutString, int OutLength)
	{
		swprintf_s(OutString, OutLength, L"%s", ToWString(inet_ntoa(*(in_addr*) &Address)).c_str());
	}

	void SendConsoleCommand(int TargetIndex, const wchar_t* Command)
	{
		if (!Command || wcslen(Command) == 0) return;

		CWindowsTarget* Target = GetTarget(TargetIndex);
		if (!Target) return;

		SendToConsole(Target->UDPClient, EDebugServerMessageType_ClientText, ToString(Command));
	}

	void ReceiveOutputText(int TargetIndex, wchar_t* OutString, int OutLength)
	{
		ReceiveMessages(MessageReceiver, 1);

		CWindowsTarget* Target = GetTarget(TargetIndex);
		if (!Target) return;

		string Channel, Text;
		if (Target->ReceivedTextBuffer.GetMessage(Channel, Text))
		{
			swprintf_s(OutString, OutLength, L"[ %s ]: %s", ToWString(Channel).c_str(), ToWString(Text).c_str());
			return;
		}

		OutString[0] = '\0';
	}
#ifndef TESTING_WINDOWS_TOOLS
	FConsoleSoundCooker* GetGlobalSoundCooker()
	{
		return NULL;
	}

	FConsoleTextureCooker* GetGlobalTextureCooker()
	{
		return NULL;
	}
#endif
protected:

	/**
	 *	Sends message using given UDP client.
	 *	@param Clien client to send message to
	 *	@param MessageType sent message type
	 *	@param Message actual text of the message
	 *	@return true on success, false otherwise
	 */
	bool SendToConsole(CUDPClient& Client, const EDebugServerMessageType MessageType, const string& Message = "")
	{
		// Compose message to be sent (header, followed by actual message)
		const int BufferLength = DebugServerMessageTypeNameLength + (Message.length() > 0 ? (sizeof(int) + (int) Message.length()) : 0);
		char* Buffer = new char[BufferLength];

		memcpy(Buffer, ToString(MessageType), DebugServerMessageTypeNameLength);
		if (Message.length() > 0)
		{
			CByteStreamConverter::StoreIntBE(Buffer + DebugServerMessageTypeNameLength, (int) Message.length());
			memcpy(Buffer + DebugServerMessageTypeNameLength + sizeof(int), Message.c_str(), Message.length());
		}

		// Send the message
		const int NumSentBytes = Client.Send(Buffer, BufferLength);

		// Clean up
		delete[] Buffer;

		return NumSentBytes == BufferLength;
	}

	/**
	 *	Attempts to receive message from given UDP client.
	 *	@param Clien client to receive message from
	 *	@param MessageType received message type
	 *	@param Data received message data
	 *	@param DataSize received message size
	 *	@return true on success, false otherwise
	 */
	bool ReceiveFromConsole(CUDPClient& Client, EDebugServerMessageType& MessageType, char*& Data, int& DataSize, sockaddr_in& SenderAddress)
	{
		// Attempt to receive bytes
		#define MAX_BUFFER_SIZE (1 << 12)
		char Buffer[MAX_BUFFER_SIZE];
		const int NumReceivedBytes = Client.Receive(Buffer, MAX_BUFFER_SIZE, SenderAddress);

		// The message should contain at least 2 characters identyfing message type
		if (NumReceivedBytes < 2)
			return false;

		// Determine message type
		char MessageTypeString[3];
		sprintf_s(MessageTypeString, 3, "%c%c", Buffer[0], Buffer[1]);
		MessageType = ToDebugServerMessageType(MessageTypeString);

		DebugOutput("Received message of type: ");
		DebugOutput(MessageTypeString);
		DebugOutput("\n");

		// Determine message content
		DataSize = NumReceivedBytes - 2;
		Data = new char[DataSize];
		memcpy(Data, Buffer + 2, DataSize);

		return true;
	}

	/**
	 *	Attempts to determine available targets.
	 */
	void DetermineTargets()
	{
		// Invalidate targets older than fixed no. seconds
		__time64_t CurrentTime;
		_time64(&CurrentTime);

		for (int i = 0; i < (int) Targets.size(); i++)
			if (CurrentTime - Targets[i]->TimeRegistered > 10)
			{
				delete Targets[i];
				Targets[i] = Targets[ Targets.size() - 1 ];
				Targets.pop_back();
				i--;
				continue;
			}

		DebugOutput("Determining targets...\n");

		// Broadcast 'server announce' request
		if (!SendToConsole(Broadcaster, EDebugServerMessageType_ServerAnnounce, ""))
			return;

		DebugOutput("Sent broadcast message...\n");

		// Give the servers some time to respond
		Sleep(100);

		// Attempt to receive 'server response'
		ReceiveMessages(ServerResponseListener, 50);
	}

	/**
	 *	Attempts to receive (and dispatch) messages from given client.
	 *	@param Client client to attempt to receive message from
	 *	@param AttemptsCount no. attempts (between each fixed miliseconds waiting happens)
	 */
	void ReceiveMessages(CUDPClient& Client, int AttemptsCount)
	{
		int Counter = AttemptsCount;
		while (Counter > 0)
		{
			// Attempt to receive message
			char* Data = NULL;
			int DataSize = 0;
			EDebugServerMessageType MessageType;
			sockaddr_in ServerAddress;

			if (!ReceiveFromConsole(Client, MessageType, Data, DataSize, ServerAddress))
			{
				// See if we should stop attempting to receive messages
				Counter--;
				if (Counter == 0)
					return;

				// Try again
				Sleep(10);
				continue;
			}

			// Handle each supported message type
			switch (MessageType)
			{
				case EDebugServerMessageType_ServerResponse:
				{
					// See if target already exists
					CWindowsTarget* Target = GetTarget(ServerAddress);

					// Add target
					if (!Target)
					{
						Target = new CWindowsTarget();
						Targets.push_back(Target);
					}

					// Set up target info
					Target->Configuration = L"Unknown configuration";
					Target->ComputerName = ToWString(CByteStreamConverter::LoadStringBE(Data));
					Target->GameName = ToWString(CByteStreamConverter::LoadStringBE(Data));
					Target->GameType = (EGameType) *(Data++);
					Target->GameTypeName = ToWString(ToString(Target->GameType));
					Target->PlatformType = (EPlatformType) *(Data++);
					Target->ListenPortNo = CByteStreamConverter::LoadIntBE(Data);
					_time64(&Target->TimeRegistered);

					// Set up target connection
					Target->UDPClient.SetIP(ServerAddress.sin_addr.s_addr);
					Target->UDPClient.SetPort(DefaultDebugChannelReceivePort);

					Target->UpdateName();

					continue;
				}

				case EDebugServerMessageType_ServerDisconnect:
				{
					// Figure out target
					CWindowsTarget* Target = GetTarget(ServerAddress);
					if (!Target)
					{
						DebugOutput("Received 'server disconnect' from unknown server.\n");
						continue;
					}

					// Disconnect from server
					Target->UDPClient.DestroySocket();

					continue;
				}

				case EDebugServerMessageType_ServerTransmission:
				{
					DebugOutput("Server TEXT transmission.\n");

					// Figure out target
					CWindowsTarget* Target = GetTarget(ServerAddress);
					if (!Target)
					{
						DebugOutput("Received TEXT transmission from unknown server.\n");
						continue;
					}

					// Retrieve channel and text
					const string Channel = CByteStreamConverter::LoadStringBE(Data);
					const string Text = CByteStreamConverter::LoadStringBE(Data);

					DebugOutput("Server TEXT transmission decoded.\n");
					DebugOutput("Channel: ");
					DebugOutput(Channel.c_str());
					DebugOutput("\n");
					DebugOutput("Text: ");
					DebugOutput(Text.c_str());
					DebugOutput("\n");

					// Buffer the message
					Target->ReceivedTextBuffer.AddMessage(Channel, Text);
					continue;
				}

				default:
				{
					DebugOutput("Some server sent message of unknown type.\n");
				}
			}
		}
	}

	/// Retrieves target by index
	CWindowsTarget* GetTarget(int TargetIndex)
	{
		if (TargetIndex < 0 || TargetIndex >= (int) Targets.size())
			return NULL;
		return Targets[TargetIndex];
	}

	/// Retrieves target by address
	CWindowsTarget* GetTarget(sockaddr_in& Address)
	{
		for (unsigned int i = 0; i < Targets.size(); i++)
			if (Targets[i]->UDPClient.GetIP() == Address.sin_addr.s_addr)
				return Targets[i];
		return NULL;
	}
	
	/// Retrieves target by name
	CWindowsTarget* GetTarget(wchar_t* Name)
	{
		for (unsigned int i = 0; i < Targets.size(); i++)
			if (Targets[i] && wcscmp(Name, Targets[i]->Name.c_str()) == 0)
				return Targets[i];
		return NULL;
	}

	/// Output used for debugging WindowsTools only
#ifdef TESTING_WINDOWS_TOOLS
	void DebugOutput(const char* Buffer)
	{
		static FILE* DebugFile = NULL;

		OutputDebugString(Buffer);

		const errno_t Result = fopen_s(&DebugFile, "WindowsTools Debug Log.txt", !DebugFile ? "w" : "a");
		if (Result != 0)
			return;

		fprintf(DebugFile, Buffer);
		fclose(DebugFile);
	}
#else
	void DebugOutput(const char*) {}
#endif

};

#ifndef TESTING_WINDOWS_TOOLS
CONSOLETOOLS_API FConsoleSupport* GetConsoleSupport()
{
	static CWindowsSupport WindowsSupport;
	return &WindowsSupport;
}
#endif
