/*=============================================================================
	UnTickable.h: Interface for tickable objects.

	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/**
 * This class provides common registration for tickable objects. It is an
 * abstract base class requiring you to implement the Tick() method.
 */
class FTickableObject
{
public:
	/** Static array of tickable objects */
	static TArray<FTickableObject*>	TickableObjects;

	/** Static array of tickable objects that are ticked from rendering thread*/
	static TArray<FTickableObject*>	RenderingThreadTickableObjects;

	/**
	 * Registers this instance with the static array of tickable objects.	
	 *
	 * @param bIsRenderingThreadObject TRUE if this object is owned by the rendering thread.
	 * @param bRegisterImmediately TRUE if the object should be registered immediately.
	 */
	FTickableObject(UBOOL bIsRenderingThreadObject=FALSE,UBOOL bRegisterImmediately=TRUE)
	{
		if ( !GIsAffectingClassDefaultObject )
		{
			// Register the object unless deferral is requested.
			if(bRegisterImmediately)
			{
				Register(bIsRenderingThreadObject);
			}
		}
	}

	/**
	 * Removes this instance from the static array of tickable objects.
	 */
	virtual ~FTickableObject()
	{
		// attempt to remove this object from both arrays (so we don't need to keep track
		// of which array it came from)
		TickableObjects.RemoveItem( this );
		RenderingThreadTickableObjects.RemoveItem( this );
	}

	/**
	 * Registers the object for ticking.
	 * @param bIsRenderingThreadObject TRUE if this object is owned by the rendering thread.
	 */
	void Register(UBOOL bIsRenderingThreadObject = FALSE)
	{
		// insert this object into the appropriate list
		if (bIsRenderingThreadObject)
		{
			RenderingThreadTickableObjects.AddItem( this );
		}
		else
		{
			TickableObjects.AddItem( this );
		}
	}

	/**
	 * Pure virtual that must be overloaded by the inheriting class. It will
	 * be called from within UnLevTick.cpp after ticking all actors.
	 *
	 * @param DeltaTime	Game time passed since the last call.
	 */
	virtual void Tick( FLOAT DeltaTime ) = 0;

	/**
	 * Pure virtual that must be overloaded by the inheriting class. It is
	 * used to determine whether an object is ready to be ticked. This is 
	 * required for example for all UObject derived classes as they might be
	 * loaded async and therefore won't be ready immediately.
	 *
	 * @return	TRUE if class is ready to be ticked, FALSE otherwise.
	 */
	virtual UBOOL IsTickable() = 0;

	/**
	 * Used to determine if an object should be ticked when the game is paused.
	 * Defaults to false, as that mimics old behavior.
	 *
	 * @return TRUE if it should be ticked when paused, FALSE otherwise
	 */
	virtual UBOOL IsTickableWhenPaused(void)
	{
		return FALSE;
	}

	/**
	 * Used to determine whether the object should be ticked in the editor.  Defaults to FALSE since
	 * that is the previous behavior.
	 *
	 * @return	TRUE if this tickable object can be ticked in the editor
	 */
	virtual UBOOL IsTickableInEditor(void)
	{
		return FALSE;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
