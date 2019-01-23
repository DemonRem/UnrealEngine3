/**
 * This specialized version of UIComp_DrawString handles rendering UIStrings for editboxes.  The responsibilities specific
 * to rendering text in editboxes are:
 *	1. A caret must be rendered at the appropriate location in the string
 *	2. Ensuring that the text surrounding the caret is always visible
 *	3. Tracking the text that was typed by the user independently from the data source that the owning widget is bound to.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * @todo - register this component with the data store for the caret image node so that it receives the refreshdatastore callback
 */
class UIComp_DrawStringEditbox extends UIComp_DrawString
	within UIEditBox
	native(inherit);

/** Contains the text that the user has typed into the editbox */
var		private{protected}	transient	string					UserText;

/** Controls whether and how a caret is displayed */
var(Presentation)						UIStringCaretParameters	StringCaret;

/** the image node that is used for rendering the caret */
var	const	native	private	transient	pointer					CaretNode{struct FUIStringNode_Image};

/** the position of the first visible character in the editbox */
var	const			private	transient	int						FirstCharacterPosition;

/** indicates that the FirstCharacterPosition needs to be re-evaluated the next time the string is reformatted */
var	const					transient	bool					bRecalculateFirstCharacter;

/** the offset (in pixels) from the left edge of the editbox's bounding region for rendering the caret */
var	const					transient	float					CaretOffset;

cpptext
{
	// UIEditboxString needs direct access to UserText
	friend class UUIEditboxString;

	/* === UUIComp_DrawStringEditbox interface === */
	/**
	 * Requests a new UIStringNode_Image from the Images data store.  Initializes the image node
	 * and assigns it to the value of CaretNode.
	 *
	 * @param	bRecreateExisting	specifies what should happen if this component already has a valid CaretNode.  If
	 *								TRUE, the existing caret is deleted and a new one is created.
	 */
	void ResolveCaretImageNode( UBOOL bRecreateExisting=FALSE );

	/**
	 * Inserts the markup necessary to render the caret at the appropriate position in SourceText.
	 *
	 * @param	out_CaretMarkupString	a string containing markup code necessary for the caret image to be resolved into a UIStringNode_Image
	 *
	 * @return	TRUE if out_ProcessedString contains valid caret markup text.
	 *			FALSE if this component is configured to not render a caret, or the caret reference is invalid.
	 */
	UBOOL GenerateCaretMarkup( FString& out_CaretMarkupString );

	/**
	 * Retrieves the image style data associated with the caret's configured style from the currently active
	 * skin and applies that style data to the caret's UITexture.
	 *
	 * @param	CurrentWidgetStyle		the current state of the widget that owns this draw string component.  Used
	 *									for choosing which style data set [from the caret's style] to use for rendering.
	 *									If not specified, the current state of the widget that owns this component will be used.
	 */
	void ApplyCaretStyle( UUIState* CurrentWidgetState=NULL );

	/**
	 * Moves the caret to a new position in the text.
	 *
	 * @param	NewCaretPosition	the location to put the caret.  Should be a non-zero integer between 0 and the length
	 *								of UserText.  Values outside the valid range will be clamed.
	 *
	 * @return	TRUE if the string's new caret position is different than the string's previous caret position.
	 */
	UBOOL SetCaretPosition( INT NewCaretPosition );

	/**
	 * Updates the value of FirstCharacterPosition with the location of the first character of the string that is now visible.
	 */
	void UpdateFirstVisibleCharacter();

	/**
	 * Calculates the total width of the characters that precede the FirstCharacterPosition.
	 *
	 * @param	Parameters	@see UUIString::StringSize() (intentionally passed by value)
	 *
	 * @return	the width (in pixels) of a sub-string containing all characters up to FirstCharacterPosition.
	 */
	FLOAT CalculateFirstVisibleOffset( FRenderParameters Parameters ) const;

	/**
	 * Calculates the total width of the characters that precede the CaretPosition.
	 *
	 * @param	Parameters	@see UUIString::StringSize() (intentionally passed by value)
	 *
	 * @return	the width (in pixels) of a sub-string containing all characters up to StringCaret.CaretPosition.
	 */
	FLOAT CalculateCaretOffset( FRenderParameters Parameters ) const;

	/**
	 * Returns a reference to this editbox component's UserText variable (useful in cases where you need to work with the
	 * string but don't want to make a copy of it).
	 */
	const FString& GetUserTextRef() const { return UserText; }

	/**
	 * @return	the string being rendered in the editbox; equal to UserText unless the editbox is in password mode.
	 */
	FString GetDisplayString() const;

private:
	/**
	 * Deletes the existing caret node if one exists, and unregisters this component from the images data store.
	 */
	virtual void DeleteCaretNode();
public:

	/* === UUIComp_DrawString interface === */
	/**
	 * Initializes this component, creating the UIString needed for rendering text.
	 *
	 * @param	InSubscriberOwner	if this component is owned by a widget that implements the IUIDataStoreSubscriber interface,
	 *								the TScriptInterface containing the interface data for the owner.
	 */
	virtual void InitializeComponent( TScriptInterface<IUIDataStoreSubscriber>* InSubscriberOwner=NULL );

	/**
	 * Changes the text that will be parsed by the UIString, and updates UserText to the resolved value.
	 *
	 * @param	NewText		the new text that should be displayed
	 */
	virtual void SetValue( const FString& NewText );

	/**
	 * Retrieve the text value of this editbox.
	 *
	 * @param	bReturnInputText	specify TRUE to return the value of UserText; FALSE to return the raw text stored
	 *								in the UIString's node's SourceText
	 *
	 * @return	either the raw value of this editbox string component or the text that the user entered
	 */
	virtual FString GetValue( UBOOL bReturnInputText=TRUE ) const;

	/**
	 * Retrieves a list of all data stores resolved by ValueString.
	 *
	 * @param	StringDataStores	receives the list of data stores that have been resolved by the UIString.  Appends all
	 *								entries to the end of the array and does not clear the array first.
	 */
	virtual void GetResolvedDataStores( TArray<class UUIDataStore*>& StringDataStores );

	/**
	 * Flags the component to be reformatted during the next scene update, and flags the editbox to recalculate its
	 * first visible character, if necessary.
	 *
	 * @param	bDockingStackChanged	if TRUE, the scene will rebuild its DockingStack at the beginning
	 *									the next frame
	 * @param	bPositionsChanged		if TRUE, the scene will update the positions for all its widgets
	 *									at the beginning of the next frame
	 * @param	bNavLinksOutdated		if TRUE, the scene will update the navigation links for all widgets
	 *									at the beginning of the next frame
	 * @param	bWidgetStylesChanged	if TRUE, the scene will refresh the widgets reapplying their current styles
	 */
	virtual void ReapplyFormatting( UBOOL bDockingStackChanged=FALSE, UBOOL bPositionsChanged=TRUE, UBOOL bNavLinksOutdated=FALSE, UBOOL bWidgetStylesChanged=FALSE );

	/**
	 * Resolves the combo style for this string rendering component.
	 *
	 * This version also resolves the caret image style.
	 *
	 * @param	ActiveSkin			the skin the use for resolving the style reference.
	 * @param	bClearExistingValue	if TRUE, style references will be invalidated first.
	 * @param	CurrentMenuState	the menu state to use for resolving the style data; if not specified, uses the current
	 *								menu state of the owning widget.
	 * @param	StyleProperty		if specified, only the style reference corresponding to the specified property
	 *								will be resolved; otherwise, all style references will be resolved.
	 */
	virtual UBOOL NotifyResolveStyle( class UUISkin* ActiveSkin, UBOOL bClearExistingValue, class UUIState* CurrentMenuState=NULL, const FName StylePropertyName=NAME_None );

protected:
	/**
	 * Wrapper for calling ApplyFormatting on the string.  Resets the value of bReapplyFormatting and bRecalculateFirstCharacter.
	 *
	 * @param	Parameters		@see UUIString::ApplyFormatting())
	 * @param	bIgnoreMarkup	@see UUIString::ApplyFormatting())
	 */
	virtual void ApplyStringFormatting( struct FRenderParameters& Parameters, UBOOL bIgnoreMarkup );

	/**
	 * Renders the string using the parameters specified.
	 *
	 * @param	Canvas	the canvas to use for rendering this string
	 */
	virtual void InternalRender_String( FCanvas* Canvas, FRenderParameters& Parameters );

public:
	/* === UObject interface === */
	virtual void AddReferencedObjects( TArray<UObject*>& ObjectArray );
	virtual void Serialize( FArchive& Ar );
	virtual void FinishDestroy();

	/**
	 * Called when a property value from a member struct or array has been changed in the editor.
	 */
	virtual void PostEditChange( FEditPropertyChain& PropertyThatChanged );
}

/* == Delegates == */

/* == Natives == */
/**
 * Changes the value of UserText to the specified text without affecting the
 *
 * SetUserText should be used for modifying the "input text"; that is, the text that would potentially be published to
 * the data store this editbox is bound to.
 * SetValue should be used to change the raw string that will be parsed by the underlying UIString.  UserText will be
 * set to the resolved value of the parsed string.
 *
 * @param	NewText		the new text that should be displayed
 *
 * @return	TRUE if the value changed.
 */
native final function bool SetUserText( string NewValue );

/**
 * Returns the length of UserText
 */
native final function int GetUserTextLength() const;

/* == Events == */

/* == UnrealScript == */

/* == SequenceAction handlers == */


DefaultProperties
{
	StringClass=class'Engine.UIEditboxString'
	StringStyle=(DefaultStyleTag="DefaultEditboxStyle",RequiredStyleClass=class'Engine.UIStyle_Combo')
	TextStyleCustomization=(ClipMode=CLIP_Normal,bOverrideClipMode=true,ClipAlignment=UIALIGN_Right,bOverrideAlignment=true)
}
