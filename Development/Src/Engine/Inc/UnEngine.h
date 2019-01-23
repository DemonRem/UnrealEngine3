/*=============================================================================
	UnEngine.h: Unreal engine helper definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/*-----------------------------------------------------------------------------
	UServerCommandlet.
-----------------------------------------------------------------------------*/

BEGIN_COMMANDLET(Server,Engine)
	void StaticInitialize()
	{
		IsClient = FALSE;
		IsEditor = FALSE;
		LogToConsole = TRUE;
	}
END_COMMANDLET


BEGIN_COMMANDLET(SmokeTest,Engine)
	void StaticInitialize()
	{
		IsClient = FALSE;
		IsEditor = FALSE;
		LogToConsole = TRUE;
	}
END_COMMANDLET



//
//	FPlayerIterator - Iterates over local players in the game.
//

class FPlayerIterator
{
protected:

	UEngine*	Engine;
	INT			Index;

public:

	// Constructor.

	FPlayerIterator(UEngine* InEngine):
		Engine(InEngine),
		Index(-1)
	{
		++*this;
	}

	void operator++()
	{
		if ( Engine != NULL )
		{
			while (Engine->GamePlayers.IsValidIndex(++Index) && !Engine->GamePlayers(Index));
		}
	}
	ULocalPlayer* operator*()
	{
		return Engine && Engine->GamePlayers.IsValidIndex(Index) ? Engine->GamePlayers(Index) : NULL;
	}
	ULocalPlayer* operator->()
	{
		return Engine && Engine->GamePlayers.IsValidIndex(Index) ? Engine->GamePlayers(Index) : NULL;
	}
	operator UBOOL()
	{
		return Engine && Engine->GamePlayers.IsValidIndex(Index);
	}
	void RemoveCurrent()
	{
		checkSlow(Engine);
		check(Engine->GamePlayers.IsValidIndex(Index));
		Engine->GamePlayers.Remove(Index--);
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

