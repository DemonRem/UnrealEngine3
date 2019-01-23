/**
 * This  widget allows the user to type numeric text into an input field.
 * The value of the text in the input field can be incremented and decremented through the buttons associated with this widget.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved
 *
 * @todo - selection highlight support
 */
class UINumericEditBox extends UIEditBox
	native(inherit);

/** the style to use for the editbox's increment button */
var(Style)									UIStyleReference		IncrementStyle;

/** the style to use for the editbox's decrement button */
var(Style)									UIStyleReference		DecrementStyle;

/** Buttons that can be used to increment and decrement the value stored in the input field. */
var		private								UINumericEditBoxButton	IncrementButton;
var		private								UINumericEditBoxButton	DecrementButton;

/**
 * The value and range parameters for this numeric editbox.
 */
var(Text)									UIRangeData				NumericValue;

/** The number of digits after the decimal point. */
var(Text)									int						DecimalPlaces;

/** The position of the faces of the increment button. */
var(Buttons)								UIScreenValue_Bounds	IncButton_Position;

/** The position of the faces of the Decrement button. */
var(Buttons)								UIScreenValue_Bounds	DecButton_Position;


cpptext
{
	/**
	 * Initializes the buttons and creates the background image.
	 *
	 * @param	inOwnerScene	the scene to add this widget to.
	 * @param	inOwner			the container widget that will contain this widget.  Will be NULL if the widget
	 *							is being added to the scene's list of children.
	 */
	virtual void Initialize( UUIScene* inOwnerScene, UUIObject* inOwner=NULL );

	/**
	 * Generates a array of UI Action keys that this widget supports.
	 *
	 * @param	out_KeyNames	Storage for the list of supported keynames.
	 */
	virtual void GetSupportedUIActionKeyNames(TArray<FName> &out_KeyNames );

	/**
	 * Evalutes the Position value for the specified face into an actual pixel value.  Should only be
	 * called from UIScene::ResolvePositions.  Any special-case positioning should be done in this function.
	 *
	 * @param	Face	the face that should be resolved
	 */
	virtual void ResolveFacePosition( EUIWidgetFace Face );

	/**
	 * Called when a property value has been changed in the editor.
	 */
	virtual void PostEditChange( FEditPropertyChain& PropertyThatChanged );

	/**
	 * Render this editbox.
	 *
	 * @param	Canvas	the FCanvas to use for rendering this widget
	 */
	void Render_Widget( FCanvas* Canvas );

	/**
	 * Called whenever the user presses enter while this editbox is focused.  Activated the SubmitText kismet event and calls the
	 * OnSubmitText delegate.
	 *
	 * @param	PlayerIndex		the index of the player that generated the call to this method; used as the PlayerIndex when activating
	 *							UIEvents; if not specified, the value of GetBestPlayerIndex() is used instead.
	 */
	virtual void NotifySubmitText( INT PlayerIndex=INDEX_NONE );

	/**
	 * Evaluates the value string of the string component to verify that it is a legit numeric value.
	 */
	UBOOL ValidateNumericInputString();

protected:

	/**
	 * Determine whether the specified character should be displayed in the text field.
	 */
	virtual UBOOL IsValidCharacter( TCHAR Character ) const;

	/**
	 * Handles input events for this editbox.
	 *
	 * @param	EventParms		the parameters for the input event
	 *
	 * @return	TRUE to consume the key event, FALSE to pass it on.
	 */
	virtual UBOOL ProcessInputKey( const struct FSubscribedInputEventParameters& EventParms );
}

/**
 * Increments the numeric editbox's value.
 *
 * @param	EventObject	Object that issued the event.
 * @param	PlayerIndex	Player that performed the action that issued the event.
 */
native final function IncrementValue( UIScreenObject Sender, int PlayerIndex );

/**
 * Decrements the numeric editbox's value.
 *
 * @param	EventObject	Object that issued the event.
 * @param	PlayerIndex	Player that performed the action that issued the event.
 */
native final function DecrementValue( UIScreenObject Sender, int PlayerIndex );

/**
 * Initializes the clicked delegates in the increment and decrement buttons to use the editbox's increment and decrement functions.
 * @todo - this is a fix for the issue where delegates don't seem to be getting set properly in defaultproperties blocks.
 */
event Initialized()
{
	Super.Initialized();

	IncrementButton.OnPressed = IncrementValue;
	IncrementButton.OnPressRepeat = IncrementValue;

	DecrementButton.OnPressed = DecrementValue;
	DecrementButton.OnPressRepeat = DecrementValue;

	// Private Behavior
	IncrementButton.SetPrivateBehavior(PRIVATE_NotFocusable | PRIVATE_NotDockable | PRIVATE_TreeHidden, true);
	DecrementButton.SetPrivateBehavior(PRIVATE_NotFocusable | PRIVATE_NotDockable | PRIVATE_TreeHidden, true);
}

/**
 * Change the value of this numeric editbox at runtime. Takes care of conversion from float to internal value string.
 *
 * @param	NewValue				the new value for the editbox.
 * @param	bForceRefreshString		Forces a refresh of the string component, normally the string is only refreshed when the value is different from the current value.
 *
 * @return	TRUE if the editbox's value was changed
 */
native final function bool SetNumericValue( float NewValue, optional bool bForceRefreshString=false );

/**
 * Gets the current value of this numeric editbox.
 */
native final function float GetNumericValue( ) const;


/** UIDataSourceSubscriber interface */
/**
 * Resolves this subscriber's data store binding and updates the subscriber with the current value from the data store.
 *
 * @return	TRUE if this subscriber successfully resolved and applied the updated value.
 */
native virtual function bool RefreshSubscriberValue( optional int BindingIndex=INDEX_NONE );

/**
 * Retrieves the list of data stores bound by this subscriber.
 *
 * @param	out_BoundDataStores		receives the array of data stores that subscriber is bound to.
 */
native virtual function GetBoundDataStores( out array<UIDataStore> out_BoundDataStores );

/**
 * Resolves this subscriber's data store binding and publishes this subscriber's value to the appropriate data store.
 *
 * @param	out_BoundDataStores	contains the array of data stores that widgets have saved values to.  Each widget that
 *								implements this method should add its resolved data store to this array after data values have been
 *								published.  Once SaveSubscriberValue has been called on all widgets in a scene, OnCommit will be called
 *								on all data stores in this array.
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 *
 * @return	TRUE if the value was successfully published to the data store.
 */
native function bool SaveSubscriberValue( out array<UIDataStore> out_BoundDataStores, optional int BindingIndex=INDEX_NONE );


DefaultProperties
{
	DataSource=(MarkupString="Numeric Editbox Text",RequiredFieldType=DATATYPE_RangeProperty)

	PrimaryStyle=(DefaultStyleTag="DefaultEditboxStyle",RequiredStyleClass=class'Engine.UIStyle_Combo')

	// Increment and Decrement Button Styles
	IncrementStyle=(DefaultStyleTag="ButtonBackground",RequiredStyleClass=class'Engine.UIStyle_Image')
	DecrementStyle=(DefaultStyleTag="ButtonBackground",RequiredStyleClass=class'Engine.UIStyle_Image')

	// Restrict the acceptable character set just numbers.
	CharacterSet=CHARSET_NumericOnly

	NumericValue=(MinValue=0.f,MaxValue=100.f,NudgeValue=1.f)
	DecimalPlaces=4
}
