/**
 * Base class for all classes that handle interacting with the user.
 *
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
class UIRoot extends Object
	native(UserInterface)
	HideCategories(Object,UIRoot)
	DependsOn(SequenceOp,WorldInfo)
	abstract;

/**
 * Data field categorizations.
 */
enum EUIDataProviderFieldType
{
	/**
	 * this tag represents a bindable data field that corresponds to a simple data type
	 */
	DATATYPE_Property<DisplayName=Property>,

	/**
	 * this tag represents an internal data provider; cannot be bound to a widget
	 */
	DATATYPE_Provider<DisplayName=Internal Provider>,

	/**
	 * this tag represents a field that can only be represented by widgets that can display range values, such as
	 * sliders, progress bars, and spinners.
	 */
	DATATYPE_RangeProperty<DisplayName=Range Property>,

	/**
	 * Holds a UniqueNetId value - cannot be represented by a normal property because it's internal members are natively serialized.
	 */
	DATATYPE_NetIdProperty<DisplayName=Unique NetId Property>,

	/**
	 * this tag represents a bindable array data field; can be bound to lists or individual elements can be bound to widgets
	 */
	DATATYPE_Collection<DisplayName=Array>,

	/**
	 * this tag represents an array of internal data providers. Can be bound to lists or the properties for individual elements
	 * can be bound to widgets
	 */
	DATATYPE_ProviderCollection<DisplayName=Array Of Providers>,
};

/**
 * The list of platforms which can provide input to the engine.  Not necessarily the platform the game is being played on -
 * for example, if the game is running on a PC, but the player is using an Xbox controller, the current InputPlatformType
 * would be IPT_360.
 */
enum EInputPlatformType
{
	/**
	 * Generally for PCs only, but could also be used for consoles which support keyboard/mouse.
	 */
	IPT_PC,

	/**
	 * Microsoft Xbox 360 TypeS-style Gamepad
	 */
	IPT_360,

	/**
	 * Sony Playstation 3 SIXAxis Gamepad
	 */
	IPT_PS3,

	// add any additional platforms supported by your game here:
	// IPT_Wii, IPT_PSP, etc.
};

/**
 * Contains information about a data value that must be within a specific range.
 */
struct native UIRangeData
{
	/** the current value of this UIRange */
	var(Range)	public{private}		float	CurrentValue;

	/**
	 * The minimum value for this UIRange.  The value of this UIRange must be greater than or equal to this value.
	 */
	var(Range)						float	MinValue;

	/**
	 * The maximum value for this UIRange.  The value of this UIRange must be less than or equal to this value.
	 */
	var(Range)						float	MaxValue;

	/**
	 * Controls the amount to increment or decrement this UIRange's value when used by widgets that support "nudging".
	 * If NudgeValue is zero, reported NudgeValue will be 1% of MaxValue - MinValue.
	 */
	var(Range)	public{private}		float	NudgeValue;

	/**
	 * Indicates whether the values in this UIRange should be treated as ints.
	 */
	var(Range)						bool	bIntRange;

structcpptext
{
	/** Constructors */
	FUIRangeData() {}
	FUIRangeData(EEventParm)
	: CurrentValue(0.f), MinValue(0.f), MaxValue(0.f), NudgeValue(0.f), bIntRange(FALSE)
	{}

	FUIRangeData( const FUIRangeData& Other )
	: CurrentValue(Other.CurrentValue)
	, MinValue(Other.MinValue), MaxValue(Other.MaxValue)
	, NudgeValue(Other.NudgeValue), bIntRange(Other.bIntRange)
	{}

	/** Comparison operators */
	UBOOL operator==( const FUIRangeData& Other ) const;
	UBOOL operator!=( const FUIRangeData& Other ) const;

	/**
	 * Returns true if any values in this struct are non-zero.
	 */
	UBOOL HasValue() const;

	/**
	 * Returns the amount that this range should be incremented/decremented when nudging.
	 */
	FLOAT GetNudgeValue() const;

	/**
	 * Sets the NudgeValue for this UIRangeData to the value specified.
	 */
	void SetNudgeValue( FLOAT NewNudgeValue )
	{
		NudgeValue = NewNudgeValue;
	}

	/**
	 * Returns the current value of this UIRange.
	 */
	FLOAT GetCurrentValue() const;

	/**
	 * Sets the value of this UIRange.
	 *
	 * @param	NewValue				the new value to assign to this UIRange.
	 * @param	bClampInvalidValues		specify TRUE to automatically clamp NewValue to a valid value for this UIRange.
	 *
	 * @return	TRUE if the value was successfully assigned.  FALSE if NewValue was outside the valid range and
	 *			bClampInvalidValues was FALSE or MinValue <= MaxValue.
	 */
	UBOOL SetCurrentValue( FLOAT NewValue, UBOOL bClampInvalidValues=TRUE );
}
};

/** Coordinates for mapping an individual texture of a texture atlas */
struct native TextureCoordinates
{
	var()	float		U, V, UL, VL;

structcpptext
{
	/** Constructors */
	FTextureCoordinates()
	{ }

	FTextureCoordinates(EEventParm)
	: U(0), V(0), UL(0), VL(0)
	{}

	FTextureCoordinates( FLOAT inU, FLOAT inV, FLOAT inUL, FLOAT inVL )
	: U(inU), V(inV), UL(inUL), VL(inVL)
	{ }

	/**
	 * Returns whether the values in this coordinate are zero, accounting for floating point
	 * precision errors.
	 */
	inline UBOOL IsZero() const
	{
		return	Abs(U) < DELTA && Abs(V) < DELTA
			&&	Abs(UL) < DELTA && Abs(VL) < DELTA;
	}

	/** Comparison operators */
	inline UBOOL operator==( const FTextureCoordinates& Other ) const
	{
		return ARE_FLOATS_EQUAL(U,Other.U)
			&& ARE_FLOATS_EQUAL(V,Other.V)
			&& ARE_FLOATS_EQUAL(UL,Other.UL)
			&& ARE_FLOATS_EQUAL(VL,Other.VL);
	}
	inline UBOOL operator!=( const FTextureCoordinates& Other ) const
	{
		return !ARE_FLOATS_EQUAL(U,Other.U)
			|| !ARE_FLOATS_EQUAL(V,Other.V)
			|| !ARE_FLOATS_EQUAL(UL,Other.UL)
			|| !ARE_FLOATS_EQUAL(VL,Other.VL);
	}
}
};

/**
 * Contains the value for a property, as either text or an image.  Used for allowing script-only data provider classes to
 * resolve data fields parsed from UIStrings.
 */
struct native UIProviderScriptFieldValue
{
	/** the name of this resource; set natively after the list of available tags are retrieved from script */
	var			name		PropertyTag;

	/** the type of field this tag corresponds to */
	var			EUIDataProviderFieldType	PropertyType;

	/** If PropertyTag corresponds to data that should be represented as text, contains the value for this resource */
	var			string		StringValue;

	/** If PropertyTag correspondsd to data that should be represented as an image, contains the value for this resource */
	var			Surface		ImageValue;

	/** If PropertyTag corresponds to data that should be represented as a list of untyped data, contains the value of the selected elements */
	var			array<int>	ArrayValue;

	/** If PropertyTag corresponds to data that should be represented as value within a specific range, contains the value for this resource */
	var			UIRangeData	RangeValue;

	/** If PropertyTag corresponds to data that should be represented as a UniqueNetId, contains the value. */
	var			UniqueNetId	NetIdValue;

	/**
	 * Specifies the coordinates for ImageValue if it corresponds to a texture atlas
	 */
	var			TextureCoordinates	AtlasCoordinates;

	structcpptext
	{
		/** Constructors */
		FUIProviderScriptFieldValue() {}
		FUIProviderScriptFieldValue(EEventParm)
		{
			appMemzero(this, sizeof(FUIProviderScriptFieldValue));
		}

	    /** Copy constructor */
	    FUIProviderScriptFieldValue( const FUIProviderScriptFieldValue& Other )
	    : PropertyTag(Other.PropertyTag)
	    , PropertyType(Other.PropertyType)
	    , StringValue(Other.StringValue)
	    , ImageValue(Other.ImageValue)
	    , ArrayValue(Other.ArrayValue)
	    , RangeValue(Other.RangeValue)
	    , NetIdValue(Other.NetIdValue)
	    , AtlasCoordinates(Other.AtlasCoordinates)
	    {
		}

	    /**
	     * Returns true if this field value has been assigned.
	     */
	    UBOOL HasValue() const;

		/** @name Comparison operators */
		//@{
		UBOOL operator==( const struct FUIProviderScriptFieldValue& Other ) const;
		UBOOL operator!=( const struct FUIProviderScriptFieldValue& Other ) const;
		UBOOL operator==( const struct FUIProviderFieldValue& Other ) const;
		UBOOL operator!=( const struct FUIProviderFieldValue& Other ) const;
		//@}
	}
};


/**
 * This extension of UIProviderScriptFieldValue is used when resolving values for markup text found in UIStrings.  This struct
 * allows data stores to provide the UIStringNode that should be used when rendering the value for the data field represented
 * this struct.
 */
struct native UIProviderFieldValue extends UIProviderScriptFieldValue
{
	/**
	 * Only used by native code; allows the data store to create and initialize string nodes manually, rather than allowing
	 * the calling code to create a UIStringNode based on the value of StringValue or ImageValue
	 */
	var	const	native	transient	pointer		CustomStringNode{struct FUIStringNode};

	structcpptext
	{
		/** Constructor */
		FUIProviderFieldValue()
		: FUIProviderScriptFieldValue(), CustomStringNode(NULL)
		{ }
		FUIProviderFieldValue(EEventParm)
		: FUIProviderScriptFieldValue(EC_EventParm), CustomStringNode(NULL)
		{ }

		/** Copy constructor */
		FUIProviderFieldValue( const FUIProviderFieldValue& Other )
		: FUIProviderScriptFieldValue( (const FUIProviderScriptFieldValue&)Other ), CustomStringNode(Other.CustomStringNode)
		{ }

		FUIProviderFieldValue( const FUIProviderScriptFieldValue& Other )
		: FUIProviderScriptFieldValue(Other), CustomStringNode(NULL)
		{ }

		/** @name Comparison operators */
		//@{
		UBOOL operator==( const struct FUIProviderScriptFieldValue& Other ) const;
		UBOOL operator!=( const struct FUIProviderScriptFieldValue& Other ) const;
		UBOOL operator==( const struct FUIProviderFieldValue& Other ) const;
		UBOOL operator!=( const struct FUIProviderFieldValue& Other ) const;
		//@}
	}
};


const DEFAULT_SIZE_X = 1024;
const DEFAULT_SIZE_Y = 768;

const MAX_SUPPORTED_GAMEPADS=4;

/**
 * Associates a UIAction with input key name.
 */
struct native InputKeyAction
{
	/** the input key name that will activate the action */
	var()	name								InputKeyName;

	/** the state (pressed, released, etc.) that will activate the action */
	var()	EInputEvent							InputKeyState;

	/** The sequence operations to activate when the input key is received */
	var	array<SequenceOp.SeqOpOutputInputLink>	TriggeredOps;
	var	deprecated	array<SequenceOp>			ActionsToExecute;

	// FInputKeyAction's == operator doesn't consider the triggeredops array, so
	// we have to compare these ourselves;  actually we should fix this because
	// otherwise states might lose data when saved if the only different between
	// their archetype is the linked ops
	structcpptext
	{
		/** Default constructor; don't initialize any members or we'll overwrite values serialized from disk. */
		FInputKeyAction() {}

		/** Initialization constructor - zero initialize all members */
		FInputKeyAction(EEventParm)
		{
			appMemzero(this, sizeof(FInputKeyAction));
			InputKeyName = NAME_None;
			InputKeyState = IE_Released;
		}

		/** Copy constructor */
		FInputKeyAction( const FInputKeyAction& Other );

		/** Standard ctor */
		FInputKeyAction( FName InKeyName, EInputEvent InKeyState )
		{
			appMemzero(this, sizeof(FInputKeyAction));
			InputKeyName = InKeyName;
			InputKeyState = InKeyState;
		}

		/** Comparison operator */
		UBOOL operator==( const FInputKeyAction& Other ) const;

		/** Serialization operator */
		friend FArchive& operator<<(FArchive& Ar,FInputKeyAction& MyInputKeyAction);

		/**
		 * @return	TRUE if this input key action is linked to the sequence op.
		 */
		UBOOL IsLinkedTo( const class USequenceOp* CheckOp ) const;
	}

	structdefaultproperties
	{
		InputKeyState=IE_Released
	}
};

/**
 * Contains information about a UI data store binding, including the markup text used to reference the data store and
 * the resolved value of the markup text.
 *
 * @NOTE: if you move this struct declaration to another class, make sure to update UUIObject::GetDataBindingProperties()
 */
struct native UIDataStoreBinding
{
	/**
	 * The UIDataStoreSubscriber that contains this UIDataStoreBinding
	 */
	var		const	transient		UIDataStoreSubscriber		Subscriber;

	/**
	 * Indicates which type of data fields can be used in this data store binding
	 */
	var()	const	editconst		EUIDataProviderFieldType	RequiredFieldType;

	/**
	 * A datastore markup string which resolves to a property/data type exposed by a UI data store.
	 *
	 * @note: cannot be editconst until we have full editor support for manipulating markup strings (e.g. inserting embedded
	 * markup, etc.)
	 */
	var()	const	/*editconst*/		string					MarkupString;

	/**
	 * Used to differentiate multiple data store properties in a single class.
	 */
	var		const	transient		int							BindingIndex;

	/** the name of the data store resolved from MarkupString */
	var		const	transient		name						DataStoreName;

	/** the name of the field resolved from MarkupString; must be a field supported by ResolvedDataStore */
	var		const	transient		name						DataStoreField;

	/** a pointer to the data store resolved from MarkupString */
	var		const	transient		UIDataStore					ResolvedDataStore;

	structcpptext
	{
		/**
		 * Registers the current subscriber with ResolvedDataStore's list of RefreshSubscriberNotifies
		 */
		void RegisterSubscriberCallback();

		/**
		 * Removes the current subscriber from ResolvedDataStore's list of RefreshSubscriberNotifies.
		 */
		void UnregisterSubscriberCallback();

		/**
		 * Determines whether the specified data field can be assigned to this data store binding.
		 *
		 * @param	DataField	the data field to verify.
		 *
		 * @return	TRUE if DataField's FieldType is compatible with the RequiredFieldType for this data binding.
		 */
		UBOOL IsValidDataField( const struct FUIDataProviderField& DataField ) const;

		/**
		 * Determines whether the specified field type is valid for this data store binding.
		 *
		 * @param	FieldType	the data field type to check
		 *
		 * @return	TRUE if FieldType is compatible with the RequiredFieldType for this data binding.
		 */
		UBOOL IsValidDataField( EUIDataProviderFieldType FieldType ) const;

		/**
		 * Resolves the value of MarkupString into a data store reference, and fills in the values for all members of this struct
		 *
		 * @param	InSubscriber	the subscriber that contains this data store binding
		 *
		 * @return	TRUE if the markup was successfully resolved.
		 */
		UBOOL ResolveMarkup( TScriptInterface<class IUIDataStoreSubscriber> InSubscriber );

		/**
		 * Retrieves the value for this data store binding from the ResolvedDataStore.
		 *
		 * @param	out_ResolvedValue	will contain the value of the data store binding.
		 *
		 * @return	TRUE if the value for this data store binding was successfully retrieved from the data store.
		 */
		UBOOL GetBindingValue( struct FUIProviderFieldValue& out_ResolvedValue ) const;

		/**
		 * Publishes the value for this data store binding to the ResolvedDataStore.
		 *
		 * @param	NewValue	contains the value that should be published to the data store
		 *
		 * @return	TRUE if the value was successfully published to the data store.
		 */
		UBOOL SetBindingValue( const struct FUIProviderScriptFieldValue& NewValue ) const;

		/**
		 * Unregisters any bound data stores and clears all references.
		 */
		UBOOL ClearDataBinding();

	    /** Constructors */
		FUIDataStoreBinding() {}
	    FUIDataStoreBinding(EEventParm)
		{
			appMemzero(this, sizeof(FUIDataStoreBinding));
		}

		/**
		 * Member access operator.  Provides transparent access to the ResolvedDataStore pointer contained by this UIDataStoreBinding
		 */
		FORCEINLINE class UUIDataStore* operator->()
		{
			return ResolvedDataStore;
		}

		/**
		 * Dereference operator.  Provides transparent access to the ResolvedDataStore pointer contained by this UIDataStoreBinding
		 *
		 * @return	ResolvedDataStore
		 */
		FORCEINLINE class UUIDataStore*& operator*()
		{
			return ResolvedDataStore;
		}

		/**
		 * Boolean operator.  Provides transparent access to the ResolvedDataStore pointer contained by this UIDataStoreBinding
		 *
		 * @return	TRUE if ResolvedDataStore is non-NULL.
		 */
		FORCEINLINE operator UBOOL() const
		{
			return ResolvedDataStore != NULL;
		}

		/** Comparison operators */
		FORCEINLINE UBOOL operator==( const FUIDataStoreBinding& Other ) const
		{
			return	Subscriber			== Other.Subscriber
				&&	RequiredFieldType	== Other.RequiredFieldType
				&&	MarkupString		== Other.MarkupString
				&&	DataStoreName		== Other.DataStoreName
				&&	DataStoreField		== Other.DataStoreField
				&&	ResolvedDataStore	== Other.ResolvedDataStore;
		}
		FORCEINLINE UBOOL operator!=( const FUIDataStoreBinding& Other ) const
		{
			return !(FUIDataStoreBinding::operator==(Other));
		}

		/* Editor serialization operator */
		friend FArchive& operator<<( FArchive& Ar, FUIDataStoreBinding& Binding )
		{
			return Ar << (UObject*&)Binding.ResolvedDataStore << Binding.Subscriber;
		}
		friend FArchive& operator<<( FArchive& Ar, FUIDataStoreBinding* Binding )
		{
			if ( Binding != NULL )
			{
				Ar << (UObject*&)Binding->ResolvedDataStore << Binding->Subscriber;
			}
			return Ar;
		}
	}

	structdefaultproperties
	{
		BindingIndex=INDEX_NONE
		RequiredFieldType=DATATYPE_MAX
	}
};


/**
 * This struct contains all data used by the various UI input processing methods.
 */
struct native transient InputEventParameters
{
	/**
	 * Index [into the Engine.GamePlayers array] for the player that generated this input event.  If PlayerIndex is not
	 * a valid index for the GamePlayers array, it indicates that this input event was generated by a gamepad that is not
	 * currently associated with an active player
	 */
	var	const transient	int				PlayerIndex;

	/**
	 * The ControllerId that generated this event.  Not guaranteed to be a ControllerId associated with a valid player.
	 */
	var	const transient	int				ControllerId;

	/**
	 * Name of the input key that was generated, such as KEY_Left, KEY_Enter, etc.
	 */
	var	const transient	name			InputKeyName;

	/**
	 * The type of input event generated (i.e. IE_Released, IE_Pressed, IE_Axis, etc.)
	 */
	var	const transient	EInputEvent		EventType;

	/**
	 * For input key events generated by analog buttons, represents the amount the button was depressed.
	 * For input axis events (i.e. joystick, mouse), represents the distance the axis has traveled since the last update.
	 */
	var	const transient	float			InputDelta;

	/**
	 * For input axis events, represents the amount of time that has passed since the last update.
	 */
	var	const transient	float			DeltaTime;

	/**
	 * For PC input events, tracks whether the corresponding modifier keys are pressed.
	 */
	var	const transient bool			bAltPressed, bCtrlPressed, bShiftPressed;

	structcpptext
	{
		/** Default constructor */
		FInputEventParameters();

		/** Input Key Event constructor */
		FInputEventParameters( INT InPlayerIndex, INT InControllerId, FName KeyName, EInputEvent Event, UBOOL bAlt, UBOOL bCtrl, UBOOL bShift, FLOAT AmountDepressed=1.f );

		/** Input Axis Event constructor */
		FInputEventParameters( INT InPlayerIndex, INT InControllerId, FName KeyName, FLOAT AxisAmount, FLOAT InDeltaTime, UBOOL bAlt, UBOOL bCtrl, UBOOL bShift );
	}
};

/**
 * Contains additional data for an input event which a widget has registered for (via UUIComp_Event::RegisterInputEvents).is
 * in the correct state capable of processing is registered to handle.the data for a Stores the UIInputAlias name translated from a combination of input key, input event type, and modifier keys.
 */
struct native transient SubscribedInputEventParameters extends InputEventParameters
{
	/**
	 * Name of the UI input alias determined from the current input key, event type, and active modifiers.
	 */
	var	const transient	name			InputAliasName;

	structcpptext
	{
		/** Default constructor */
		FSubscribedInputEventParameters();

		/** Input Key Event constructor */
		FSubscribedInputEventParameters( INT InPlayerIndex, INT InControllerId, FName KeyName, EInputEvent Event, FName InInputAliasName, UBOOL bAlt, UBOOL bCtrl, UBOOL bShift, FLOAT AmountDepressed=1.f );

		/** Input Axis Event constructor */
		FSubscribedInputEventParameters( INT InPlayerIndex, INT InControllerId, FName KeyName, FName InInputAliasName, FLOAT AxisAmount, FLOAT InDeltaTime, UBOOL bAlt, UBOOL bCtrl, UBOOL bShift );

		/** Copy constructor */
		FSubscribedInputEventParameters( const FSubscribedInputEventParameters& Other );
		FSubscribedInputEventParameters( const FInputEventParameters& Other, FName InInputAliasName );
	}
};

/**
 * Contains information for simulating a button press input event in response to axis input.
 */
struct native UIAxisEmulationDefinition
{
	/**
	 * The axis input key name that this definition represents.
	 */
	var	name	AxisInputKey;

	/**
	 * The axis input key name that represents the other axis of the joystick associated with this axis input.
	 * e.g. if AxisInputKey is MouseX, AdjacentAxisInputKey would be MouseY.
	 */
	var	name	AdjacentAxisInputKey;

	/**
	 * Indicates whether button press/release events should be generated for this axis key
	 */
	var	bool	bEmulateButtonPress;

	/**
	 * The button input key that this axis input should emulate.  The first element corresponds to the input key
	 * that should be emulated when the axis value is positive; the second element corresponds to the input key
	 * that should be emulated when the axis value is negative.
	 */
	var	name	InputKeyToEmulate[2];
};

struct native export RawInputKeyEventData
{
	/** the name of the key (i.e. 'Left' [KEY_Left], 'LeftMouseButton' [KEY_LeftMouseButton], etc.) */
	var		name	InputKeyName;

	/**
	 * a bitmask of values indicating which modifier keys are associated with this input key event, or which modifier
	 * keys are excluded.  Bit values are:
	 *	0: Alt active (or required)
	 *	1: Ctrl active (or required)
	 *	2: Shift active (or required)
	 *	3: Alt excluded
	 *	4: Ctrl excluded
	 *	5: Shift excluded
	 *
	 * (key states)
	 *	6: Pressed
	 *	7: Released
	 */
	var		byte	ModifierKeyFlags;

	structdefaultproperties
	{
		ModifierKeyFlags=56		//	1<<3 + 1<<4 + 1<<5 (alt, ctrl, shift excluded)
	}

	structcpptext
	{
		/** Constructors */
		FRawInputKeyEventData() {}
		FRawInputKeyEventData(EEventParm)
		{
			appMemzero(this, sizeof(FRawInputKeyEventData));
		}

		explicit FRawInputKeyEventData( FName InKeyName, BYTE InModifierFlags=(KEYMODIFIER_AltExcluded|KEYMODIFIER_CtrlExcluded|KEYMODIFIER_ShiftExcluded) )
		: InputKeyName(InKeyName), ModifierKeyFlags(InModifierFlags)
		{}

		FRawInputKeyEventData( const FRawInputKeyEventData& Other )
		: InputKeyName(Other.InputKeyName), ModifierKeyFlags(Other.ModifierKeyFlags)
		{}

		/** Comparison operators */
		FORCEINLINE UBOOL operator==( const FRawInputKeyEventData& Other ) const
		{
			return InputKeyName == Other.InputKeyName && ModifierKeyFlags == Other.ModifierKeyFlags;
		}
		FORCEINLINE UBOOL operator!=( const FRawInputKeyEventData& Other ) const
		{
			return InputKeyName != Other.InputKeyName || ModifierKeyFlags != Other.ModifierKeyFlags;
		}
		FORCEINLINE FRawInputKeyEventData& operator=( const FRawInputKeyEventData& Other )
		{
			InputKeyName = Other.InputKeyName; ModifierKeyFlags = Other.ModifierKeyFlags;
			return *this;
		}
		/** Required in order for FRawInputKeyEventData to be used as the key in a map */
		friend inline DWORD GetTypeHash( const FRawInputKeyEventData& KeyEvt )
		{
			return GetTypeHash(KeyEvt.InputKeyName);
		}

		/**
		 * Applies the specified modifier key bitmask to ModifierKeyFlags
		 */
		FORCEINLINE void SetModifierKeyFlags( BYTE ModifierFlags )
		{
			ModifierKeyFlags |= ModifierFlags;
		}
		/** Clears the specified modifier key bitmask from ModifierKeyFlags */
		FORCEINLINE void ClearModifierKeyFlags( BYTE ModifierFlags )
		{
			ModifierKeyFlags &= ~ModifierFlags;
		}

		/**
		 * Returns TRUE if ModifierKeyFlags contains any of the bits in FlagsToCheck.
		 */
		FORCEINLINE UBOOL HasAnyModifierKeyFlags( BYTE FlagsToCheck ) const
		{
			return (ModifierKeyFlags&FlagsToCheck) != 0 || FlagsToCheck == KEYMODIFIER_All;
		}

		/**
		 * Returns TRUE if ModifierKeyFlags contains all of the bits in FlagsToCheck
		 */
		FORCEINLINE UBOOL HasAllModifierFlags( BYTE FlagsToCheck ) const
		{
			return (ModifierKeyFlags&FlagsToCheck) == FlagsToCheck;
		}
	}
};

cpptext
{
	/**
	 * Returns the friendly name for the specified input event from the EInputEvent enum.
	 *
	 * @return	the textual representation of the enum member specified, or "Unknown" if the value is invalid.
	 */
	static FString GetInputEventText( BYTE InputEvent );

	/**
	 * Returns the friendly name for the specified field type from the UIDataProviderFieldType enum.
	 *
	 * @return	the textual representation of the enum member specified, or "Unknown" if the value is invalid.
	 */
	static FString GetDataProviderFieldTypeText( BYTE FieldType );

	/**
	 * Returns the friendly name for the specified platform type from the EInputPlatformType enum.
	 *
	 * @return	the textual representation of the enum member specified, or "Unknown" if the value is invalid.
	 */
	static FString GetInputPlatformTypeText( BYTE PlatformType );

	/**
	 * Returns the platform type for the current input device.  This is not necessarily the platform the game is actually running
	 * on; for example, if the game is running on a PC, but the player is using an Xbox controller, the current InputPlatformType
	 * would be IPT_360.
	 *
	 * @param	OwningPlayer	if specified, the returned InputPlatformType will reflect the actual input device the player
	 *							is using.  Otherwise, the returned InputPlatformType is always the platform the game is running on.
	 *
	 * @return	the platform type for the current input device (if a player is specified) or the host platform.
	 */
	static EInputPlatformType GetInputPlatformType( ULocalPlayer* OwningPlayer=NULL );

	/**
	 * Returns the UIController class set for this game.
	 *
	 * @return	a pointer to a UIInteraction class which is set as the value for GameViewportClient.UIControllerClass.
	 */
	static class UClass* GetUIControllerClass();

	/**
	 * Returns the default object for the UIController class set for this game.
	 *
	 * @return	a pointer to the CDO for UIInteraction class configured for this game.
	 */
	static class UUIInteraction* GetDefaultUIController();

	/**
	 * Returns the UIInteraction instance currently controlling the UI system, which is valid in game.
	 *
	 * @return	a pointer to the UIInteraction object currently controlling the UI system.
	 */
	static class UUIInteraction* GetCurrentUIController();

	/**
	 * Returns the game's scene client.
	 *
	 * @return 	a pointer to the UGameUISceneClient instance currently managing the scenes for the UI System.
	 */
	static class UGameUISceneClient* GetSceneClient();

	/**
	 * Resolves a data store from markup.
	 *
	 * @param	DatafieldMarkupString	The markup string to resolve
	 * @param	InOwnerScene			The scene to use (can be null)
	 * @param	InOwnerPlayer			The local player to use (can be null)
	 * @param	out_ResolvedProvider	The provider associated with the markup
	 * @param 	out_DataFieldName		The Datafield associated with the markup
	 * @param	out_ResolvedDataStore	The resolved data store
	 *
	 */
	static UBOOL ResolveDataStoreMarkup(const FString &DataFieldMarkupString, ULocalPlayer* InOwnerPlayer,
					class UUIDataProvider*& out_ResolvedProvider, FString& out_DataFieldName, class UUIDataStore** out_ResolvedDataStore=NULL );

}


/**
 * Returns the platform type for the current input device.  This is not necessarily the platform the game is actually running
 * on; for example, if the game is running on a PC, but the player is using an Xbox controller, the current InputPlatformType
 * would be IPT_360.
 *
 * @param	OwningPlayer	if specified, the returned InputPlatformType will reflect the actual input device the player
 *							is using.  Otherwise, the returned InputPlatformType is always the platform the game is running on.
 *
 * @return	the platform type for the current input device (if a player is specified) or the host platform.
 *
 * @note: noexport because the C++ version is static too.
 */
native static final noexport function EInputPlatformType GetInputPlatformType( optional LocalPlayer OwningPlayer );

/**
 * Returns the UIInteraction instance currently controlling the UI system, which is valid in game.
 *
 * @return	a pointer to the UIInteraction object currently controlling the UI system.
 */
native static final noexport function UIInteraction GetCurrentUIController();

/**
 * Returns the game's scene client.
 *
 * @return 	a pointer to the UGameUISceneClient instance currently managing the scenes for the UI System.
 */
native static final noexport function GameUISceneClient GetSceneClient();

static final function UIDataStore StaticResolveDataStore( name DataStoreTag, optional LocalPlayer InPlayerOwner )
{
	local UIDataStore Result;
	local DataStoreClient DSClient;

	DSClient = class'UIInteraction'.static.GetDataStoreClient();
	if ( DSClient != None )
	{
		Result = DSClient.FindDataStore(DataStoreTag, InPlayerOwner);
	}

	return Result;
}

/**
 * Sets the string value of the datastore entry specified.
 *
 * @param InDataStoreMarkup		Markup to find the field we want to set the value of.
 * @param InFieldValue			Value to set the datafield's value to.
 * @param OwnerScene			Owner scene for the datastore, used when dealing with scene specific datastores.
 * @param OwnerPlayer			Owner player for the datastore, used when dealing with player datastores.
 *
 * @return TRUE if the value was set, FALSE otherwise.
 */
native static final function bool SetDataStoreFieldValue(string InDataStoreMarkup, const out UIProviderFieldValue InFieldValue, optional LocalPlayer OwnerPlayer);


/**
 * Sets the string value of the datastore entry specified.
 *
 * @param InDataStoreMarkup		Markup to find the field we want to set the value of.
 * @param InStringValue			Value to set the datafield's string value to.
 * @param OwnerScene			Owner scene for the datastore, used when dealing with scene specific datastores.
 * @param OwnerPlayer			Owner player for the datastore, used when dealing with player datastores.
 *
 * @return TRUE if the value was set, FALSE otherwise.
 */
static function bool SetDataStoreStringValue(string InDataStoreMarkup, string InStringValue, optional LocalPlayer OwnerPlayer)
{
	local UIProviderFieldValue FieldValue;

	FieldValue.StringValue = InStringValue;
	FieldValue.PropertyType = DATATYPE_Property;

	return SetDataStoreFieldValue(InDataStoreMarkup, FieldValue, OwnerPlayer);
}


/**
 * Gets the field value struct of the datastore entry specified.
 *
 * @param InDataStoreMarkup		Markup to find the field we want to retrieve the value of.
 * @param OutFieldValue			Variable to store the result field value in.
 * @param OwnerScene			Owner scene for the datastore, used when dealing with scene specific datastores.
 * @param OwnerPlayer			Owner player for the datastore, used when dealing with player datastores.
 *
 * @return TRUE if the value was retrieved, FALSE otherwise.
 */
native static final function bool GetDataStoreFieldValue(string InDataStoreMarkup, out UIProviderFieldValue OutFieldValue, optional LocalPlayer OwnerPlayer);

/**
 * Gets the string value of the datastore entry specified.
 *
 * @param InDataStoreMarkup		Markup to find the field we want to retrieve the value of.
 * @param OutStringValue		Variable to store the result string in.
 * @param OwnerScene			Owner scene for the datastore, used when dealing with scene specific datastores.
 * @param OwnerPlayer			Owner player for the datastore, used when dealing with player datastores.
 *
 * @return TRUE if the value was retrieved, FALSE otherwise.
 */
static function bool GetDataStoreStringValue(string InDataStoreMarkup, out string OutStringValue, optional LocalPlayer OwnerPlayer=none)
{
	local UIProviderFieldValue FieldValue;
	local bool Result;

	if(GetDataStoreFieldValue(InDataStoreMarkup, FieldValue, OwnerPlayer))
	{
		OutStringValue = FieldValue.StringValue;
		Result = TRUE;
	}

	return Result;
}


/**
 * Wrapper for getting a reference to the online subsystem's game interface.
 */
static final function OnlineGameInterface GetOnlineGameInterface()
{
	local OnlineSubsystem OnlineSub;
	local OnlineGameInterface Result;

	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
	if ( OnlineSub != None )
	{
		Result = OnlineSub.GameInterface;
	}
	else
	{
		`Log("GetOnlinePlayerInterfaceEx: Unable to find OnlineSubSystem!");
	}

	return Result;
}

/**
 * Wrapper for getting a reference to the online subsystem's player interface.
 */
static final function OnlinePlayerInterface GetOnlinePlayerInterface()
{
	local OnlineSubsystem OnlineSub;
	local OnlinePlayerInterface Result;

	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
	if ( OnlineSub != None )
	{
		Result = OnlineSub.PlayerInterface;
	}
	else
	{
		`Log("GetOnlinePlayerInterfaceEx: Unable to find OnlineSubSystem!");
	}

	return Result;
}

/**
 * Wrapper for getting a reference to the extended online player interface
 */
static final function OnlinePlayerInterfaceEx GetOnlinePlayerInterfaceEx()
{
	local OnlineSubsystem OnlineSub;
	local OnlinePlayerInterfaceEx PlayerIntEx;

	// Display the login UI
	OnlineSub = class'GameEngine'.static.GetOnlineSubsystem();
	if (OnlineSub != None)
	{
		PlayerIntEx = OnlineSub.PlayerInterfaceEx;
	}
	else
	{
		`Log("GetOnlinePlayerInterfaceEx: Unable to find OnlineSubSystem!");
	}

	return PlayerIntEx;
}

defaultproperties
{
}
