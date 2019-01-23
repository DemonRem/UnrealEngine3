/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * List widget used in many of the UT menus that renders using z-ordering to simulate a 'circular' list.
 */
class UTUIMenuList extends UIList
	native(UI);

cpptext
{
	/** Tick callback for this widget. */
	virtual void Tick_Widget(FLOAT DeltaTime);

	/**
	 * Perform all initialization for this widget. Called on all widgets when a scene is opened,
	 * once the scene has been completely initialized.
	 * For widgets added at runtime, called after the widget has been inserted into its parent's
	 * list of children.
	 *
	 * @param	inOwnerScene	the scene to add this widget to.
	 * @param	inOwner			the container widget that will contain this widget.  Will be NULL if the widget
	 *							is being added to the scene's list of children.
	 */
	virtual void Initialize( UUIScene* inOwnerScene, UUIObject* inOwner=NULL );


	/**
	 * Callback that happens the first time the scene is rendered, any widget positioning initialization should be done here.
	 *
	 * By default this function recursively calls itself on all of its children.
	 */
	virtual void PreRenderCallback();

	/**
	 * Evalutes the Position value for the specified face into an actual pixel value.  Should only be
	 * called from UIScene::ResolvePositions.  Any special-case positioning should be done in this function.
	 *
	 * @param	Face	the face that should be resolved
	 */
	virtual void ResolveFacePosition( EUIWidgetFace Face );

	/**
	 * Called when the list's index has changed.
	 *
	 * @param	PreviousIndex	the list's Index before it was changed
	 * @param	PlayerIndex		the index of the player associated with this index change.
	 */
	virtual void NotifyIndexChanged( INT PreviousIndex, INT PlayerIndex );
}

/**
 * The data source that this list will get and save its currently selected indices from.
 */
var(Data)	editconst private		UIDataStoreBinding		SelectedIndexDataSource;

/** Whether or not we are currently animating. */
var bool bIsRotating;

/** Time we started rotating the items in this widget. */
var float StartRotationTime;

/** Store the last item selected so we know which direction to animate in. */
var int PreviousItem;

event PostInitialize()
{
	OnSubmitSelection = none;
	OnValueChanged = none;

	Super.PostInitialize();
}

/**
 * Gets the cell field value for a specified list and list index.
 *
 * @param InList		List to get the cell field value for.
 * @param InCellTag		Tag to get the value for.
 * @param InListIndex	Index to get the value for.
 * @param OutValue		Storage variable for the final value.
 */
static function native bool GetCellFieldValue(UIList InList, name InCellTag, int InListIndex, out UIProviderFieldValue OutValue);

/**
 * Returns the width of the specified row.
 *
 * @param	RowIndex		the index for the row to get the width for.  If the index is invalid, the list's configured RowHeight is returned instead.
 */
native virtual function float GetRowHeight( optional int RowIndex=INDEX_NONE ) const;

/**
 * Gets the cell field value for a specified list and list index.
 *
 * @param InList		List to get the cell field value for.
 * @param InCellTag		Tag to get the value for.
 * @param InListIndex	Index to get the value for.
 * @param OutValue		Storage variable for the final value.
 */
static final function bool GetCellFieldString(UIList InList, name InCellTag, int InListIndex, out string OutValue)
{
	local bool bResult;
	local UIProviderFieldValue FieldValue;

	bResult = false;

	if(GetCellFieldValue(InList, InCellTag, InListIndex, FieldValue) == true)
	{
		OutValue = FieldValue.StringValue;
		bResult = true;
	}
	`log("### GCFS:"$InCellTag@InListIndex@OutValue);

	return bResult;
}

/** returns the first list index the has the specified value for the specified cell, or INDEX_NONE if it couldn't be found */
native static final function int FindCellFieldString(UIList InList, name InCellTag, string FindValue, optional bool bCaseSensitive);

/**
 * Resolves this subscriber's data store binding and updates the subscriber with the current value from the data store.
 *
 * @return	TRUE if this subscriber successfully resolved and applied the updated value.
 */
native virtual function bool RefreshSubscriberValue( optional int BindingIndex=INDEX_NONE );

defaultproperties
{
	Begin Object Class=UIComp_UTUIMenuListPresenter Name=MenuListPresentationComponent
	End Object
	CellDataComponent=MenuListPresentationComponent

	WrapType=LISTWRAP_Jump
}
