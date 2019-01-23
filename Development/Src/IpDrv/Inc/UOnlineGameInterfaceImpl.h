/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

protected:

	/**
	 * Ticks the lan beacon for lan support
	 *
	 * @param DeltaTime the time since the last tick
	 */
	virtual void TickLanTasks(FLOAT DeltaTime);

	/**
	 * Determines if the packet header is valid or not
	 *
	 * @param Packet the packet data to check
	 * @param Length the size of the packet buffer
	 * @param ClientNonce out param that reads the client's nonce
	 *
	 * @return true if the header is valid, false otherwise
	 */
	UBOOL IsValidLanQueryPacket(const BYTE* Packet,DWORD Length,QWORD& ClientNonce);

	/**
	 * Determines if the packet header is valid or not
	 *
	 * @param Packet the packet data to check
	 * @param Length the size of the packet buffer
	 *
	 * @return true if the header is valid, false otherwise
	 */
	UBOOL IsValidLanResponsePacket(const BYTE* Packet,DWORD Length);

	/** Stops the lan beacon from accepting broadcasts */
	inline void StopLanBeacon(void)
	{
		// Don't poll anymore since we are shutting it down
		LanBeaconState = LANB_NotUsingLanBeacon;
		// Unbind the lan beacon object
		delete LanBeacon;
		LanBeacon = NULL;
	}

	/**
	 * Adds the game settings data to the packet that is sent by the host
	 * in reponse to a server query
	 *
	 * @param Packet the writer object that will encode the data
	 * @param GameSettings the game settings to add to the packet
	 */
	void AppendGameSettingsToPacket(FNboSerializeToBuffer& Packet,UOnlineGameSettings* GameSettings);

	/**
	 * Reads the game settings data from the packet and applies it to the
	 * specified object
	 *
	 * @param Packet the reader object that will read the data
	 * @param GameSettings the game settings to copy the data to
	 */
	void ReadGameSettingsFromPacket(FNboSerializeFromBuffer& Packet,UOnlineGameSettings* NewServer);

	/**
	 * Builds a LAN query and broadcasts it
	 *
	 * @return an error/success code
	 */
	DWORD FindLanGames(void);

	/**
	 * Creates a new lan enabled game
	 *
	 * @param HostingPlayerNum the player hosting the game
	 *
	 * @return S_OK if it succeeded, otherwise an error code
	 */
	DWORD CreateLanGame(BYTE HostingPlayerNum);

	/**
	 * Terminates a LAN session
	 *
	 * @return an error/success code
	 */
	DWORD DestroyLanGame(void);

	/**
	 * Parses a LAN packet and handles responses/search population
	 * as needed
	 *
	 * @param PacketData the packet data to parse
	 * @param PacketLength the amount of data that was received
	 */
	void ProcessLanPacket(BYTE* PacketData,INT PacketLength);

	/**
	 * Generates a random nonce (number used once) of the desired length
	 *
	 * @param Nonce the buffer that will get the randomized data
	 * @param Length the number of bytes to generate random values for
	 */
	inline void GenerateNonce(BYTE* Nonce,DWORD Length)
	{
//@todo joeg -- switch to CryptGenRandom() if possible or something equivalent
		// Loop through generating a random value for each byte
		for (DWORD NonceIndex = 0; NonceIndex < Length; NonceIndex++)
		{
			Nonce[NonceIndex] = (BYTE)(appRand() & 255);
		}
	}

public:

	/**
	 * Ticks this object to update any async tasks
	 *
	 * @param DeltaTime the time since the last tick
	 */
	virtual void Tick(FLOAT DeltaTime);

protected:

	/**
	 * Builds an internet query and broadcasts it
	 *
	 * @return an error/success code
	 */
	virtual DWORD FindInternetGames(void)
	{
		return (DWORD)-1;
	}

	/**
	 * Creates a new internet enabled game
	 *
	 * @param HostingPlayerNum the player hosting the game
	 *
	 * @return S_OK if it succeeded, otherwise an error code
	 */
	virtual DWORD CreateInternetGame(BYTE HostingPlayerNum)
	{
		return (DWORD)-1;
	}

	/**
	 * Joins the specified internet enabled game
	 *
	 * @param PlayerNum the player joining the game
	 *
	 * @return S_OK if it succeeded, otherwise an error code
	 */
	virtual DWORD JoinInternetGame(BYTE PlayerNum)
	{
		return (DWORD)-1;
	}

	/**
	 * Starts the specified internet enabled game
	 *
	 * @return S_OK if it succeeded, otherwise an error code
	 */
	virtual DWORD StartInternetGame(void)
	{
		return (DWORD)-1;
	}

	/**
	 * Ends the specified internet enabled game
	 *
	 * @return S_OK if it succeeded, otherwise an error code
	 */
	virtual DWORD EndInternetGame(void)
	{
		return (DWORD)-1;
	}

	/**
	 * Terminates an internet session with the provider
	 *
	 * @return an error/success code
	 */
	virtual DWORD DestroyInternetGame(void)
	{
		return (DWORD)-1;
	}

	/**
	 * Updates any pending internet tasks and fires event notifications as needed
	 *
	 * @param DeltaTime the amount of time that has elapsed since the last call
	 */
	virtual void TickInternetTasks(FLOAT DeltaTime)
	{
	}
