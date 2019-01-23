/*=============================================================================
	UnRedirector.h: Object redirector definition.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/**
 * This class will redirect an object load to another object, so if an object is renamed
 * to a different package or group, external references to the object can be found
 */
class UObjectRedirector : public UObject
{
	DECLARE_CLASS(UObjectRedirector, UObject, CLASS_Intrinsic, Core)
	NO_DEFAULT_CONSTRUCTOR(UObjectRedirector)

	// Variables.
	UObject*		DestinationObject;

	/**
	 * Static constructor, called once during static initialization of global variables for native 
	 * classes. Used to e.g. register object references for native- only classes required for realtime
	 * garbage collection or to associate UProperties.
	 */
	void StaticConstructor();

	// UObject interface.
	virtual void PreSave();
	void Serialize( FArchive& Ar );
};

/**
 * Callback structure to respond to redirect-followed callbacks to determine
 * if a redirector was followed from a single object name. Will auto
 * register and unregister the callback on construction/destruction
 */
class FScopedRedirectorCatcher : public FCallbackEventDevice
{
public:
	/**
	 * Constructor for the callback device, will register itself with the GCallbackEvent
	 *
	 * @param InObjectPathNameToMatch Full pathname of the object refrence that is being compiled by script
	 */
	FScopedRedirectorCatcher(const FString& InObjectPathNameToMatch);

	/**
	 * Destructor. Will unregister the callback
	 */
	~FScopedRedirectorCatcher();

	/**
	 * Returns whether or not a redirector was followed for the ObjectPathName
	 */
	inline UBOOL WasRedirectorFollowed()
	{
		return bWasRedirectorFollowed;
	}

	/**
	 * Responds to CALLBACK_RedirectorFollowed. Records all followed redirections
	 * so they can be cleaned later.
	 *
	 * @param InType Callback type (should only be CALLBACK_RedirectorFollowed)
	 * @param InString Name of the package that pointed to the redirect
	 * @param InObject The UObjectRedirector that was followed
	 */
	virtual void Send( ECallbackEventType InType, const FString& InString, UObject* InObject);

private:
	/** The full path name of the object that we want to match */
	FString ObjectPathNameToMatch;

	/** Was a redirector followed, ONLY for the ObjectPathNameToMatch? */
	UBOOL bWasRedirectorFollowed;
};
