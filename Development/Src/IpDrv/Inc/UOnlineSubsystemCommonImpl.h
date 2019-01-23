/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


protected:

	/**
	 * Ticks any game interface background tasks
	 *
	 * @param DeltaTime the time since the last tick
	 */
	FORCEINLINE void TickGameInterfaceTasks(FLOAT DeltaTime)
	{
		check(GameInterfaceImpl);
		// Tick the game interface
		GameInterfaceImpl->Tick(DeltaTime);
	}

public:
