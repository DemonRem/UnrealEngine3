/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Options tab page, autocreates a set of options widgets using the datasource provided.
 */

class UTUIOptionList extends UTUI_Widget
	placeable
	native(UIFrontEnd)
	implements(UIDataStoreSubscriber);

cpptext
{
	/** Updates the positioning of the background prefab. */
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

	/* === UUIScreenObject interface === */
	/**
	 * Generates a array of UI Action keys that this widget supports.
	 *
	 * @param	out_KeyNames	Storage for the list of supported keynames.
	 */
	virtual void GetSupportedUIActionKeyNames(TArray<FName> &out_KeyNames );

	/**
	 * Callback that happens the first time the scene is rendered, any widget positioning initialization should be done here.
	 *
	 * By default this function recursively calls itself on all of its children.
	 */
	virtual void PreRenderCallback();

protected:
	/**
	 * Handles input events for this button.
	 *
	 * @param	EventParms		the parameters for the input event
	 *
	 * @return	TRUE to consume the key event, FALSE to pass it on.
	 */
	virtual UBOOL ProcessInputKey( const struct FSubscribedInputEventParameters& EventParms );
}
/** Info about an option we have generated. */
struct native GeneratedObjectInfo
{
	var name			OptionProviderName;
	var UIObject		LabelObj;
	var UIObject		OptionObj;
	var UIDataProvider	OptionProvider;
};

/** Current option index. */
var transient int CurrentIndex;

/** Previously selected option index. */
var transient int PreviousIndex;

/** Start time for animating option switches. */
var transient float StartMovementTime;

/** Whether or not we are currently animating the background prefab. */
var transient bool	bAnimatingBGPrefab;

/** List of auto-generated objects, anything in this array will be removed from the children's array before presave. */
var transient array<GeneratedObjectInfo>	GeneratedObjects;

/** The data store that this list is bound to */
var(Data)						UIDataStoreBinding		DataSource;

/** the list element provider referenced by DataSource */
var	const	transient			UIListElementProvider	DataProvider;

/** Background prefab for the currently selected item. */
var transient					UIPrefab	BGPrefab;

/** Instance of the background prefab. */
var transient					UIPrefabInstance BGPrefabInstance;

/** Delegate called when an option gains focus. */
delegate OnOptionFocused(UIScreenObject InObject, UIDataProvider OptionProvider);

/** Delegate for when the user changes one of the options in this option list. */
delegate OnOptionChanged(UIScreenObject InObject, name OptionName, int PlayerIndex);

/** Accept button was pressed on the option list. */
delegate OnAcceptOptions(UIScreenObject InObject, int PlayerIndex);

/** Generates widgets for all of the options. */
native function RegenerateOptions();

/** Repositions all of the visible options. */
native function RepositionOptions();

/** Sets the currently selected option index. */
native function SetSelectedOptionIndex(int OptionIdx);

/** Post initialize, binds callbacks for all of the generated options. */
event PostInitialize()
{
	local int ObjectIdx;

	Super.PostInitialize();
	
	RegenerateOptions();

	// Go through all of the generated object and set the OnValueChanged delegate.
	for(ObjectIdx=0; ObjectIdx < GeneratedObjects.length; ObjectIdx++)
	{
		GeneratedObjects[ObjectIdx].OptionObj.OnValueChanged = OnValueChanged;
		GeneratedObjects[ObjectIdx].OptionObj.NotifyActiveStateChanged = OnOption_NotifyActiveStateChanged;
	}
}

/** @return Returns the object info struct index given a provider namename. */
function int GetObjectInfoIndexFromName(name ProviderName)
{
	local int ObjectIdx;
	local int Result;

	Result = INDEX_NONE;

	// Reoslve the option name
	for(ObjectIdx=0; ObjectIdx < GeneratedObjects.length; ObjectIdx++)
	{
		if(GeneratedObjects[ObjectIdx].OptionProviderName==ProviderName)
		{
			Result = ObjectIdx;
			break;
		}
	}

	return Result;
}

/** @return Returns the object info struct given a sender object. */
function int GetObjectInfoIndexFromObject(UIObject Sender)
{
	local int ObjectIdx;
	local int Result;

	Result = INDEX_NONE;

	// Reoslve the option name
	for(ObjectIdx=0; ObjectIdx < GeneratedObjects.length; ObjectIdx++)
	{
		if(GeneratedObjects[ObjectIdx].OptionObj==Sender)
		{
			Result = ObjectIdx;
			break;
		}
	}

	return Result;
}

/** Callback for all of the options we generated. */
function OnValueChanged( UIObject Sender, int PlayerIndex )
{
	local name OptionProviderName;
	local int ObjectIdx;

	OptionProviderName = '';

	// Reoslve the option name
	ObjectIdx = GetObjectInfoIndexFromObject(Sender);

	if(ObjectIdx != INDEX_NONE)
	{
		OptionProviderName = GeneratedObjects[ObjectIdx].OptionProviderName;
	}

	// Call the option changed delegate
	OnOptionChanged(Sender, OptionProviderName, PlayerIndex);
}

/** Callback for when the object's active state changes. */
function OnOption_NotifyActiveStateChanged( UIScreenObject Sender, int PlayerIndex, UIState NewlyActiveState, optional UIState PreviouslyActiveState )
{
	local int ObjectIndex;

	ObjectIndex = GetObjectInfoIndexFromObject(UIObject(Sender));

	if(ObjectIndex != INDEX_NONE)
	{
		if(NewlyActiveState.Class == class'UIState_Focused'.default.Class)
		{
			SetSelectedOptionIndex(ObjectIndex);
			OnOptionFocused(Sender, GeneratedObjects[ObjectIndex].OptionProvider);
		}
	}
}

/** UIDataSourceSubscriber interface */
/**
 * Sets the data store binding for this object to the text specified.
 *
 * @param	MarkupText			a markup string which resolves to data exposed by a data store.  The expected format is:
 *								<DataStoreTag:DataFieldTag>
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 */
native final virtual function SetDataStoreBinding( string MarkupText, optional int BindingIndex=INDEX_NONE );

/**
 * Retrieves the markup string corresponding to the data store that this object is bound to.
 *
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 *
 * @return	a datastore markup string which resolves to the datastore field that this object is bound to, in the format:
 *			<DataStoreTag:DataFieldTag>
 */
native final virtual function string GetDataStoreBinding( optional int BindingIndex=INDEX_NONE ) const;

/**
 * Resolves this subscriber's data store binding and updates the subscriber with the current value from the data store.
 *
 * @return	TRUE if this subscriber successfully resolved and applied the updated value.
 */
native final virtual function bool RefreshSubscriberValue( optional int BindingIndex=INDEX_NONE );

/**
 * Retrieves the list of data stores bound by this subscriber.
 *
 * @param	out_BoundDataStores		receives the array of data stores that subscriber is bound to.
 */
native final virtual function GetBoundDataStores( out array<UIDataStore> out_BoundDataStores );

/**
 * Notifies this subscriber to unbind itself from all bound data stores
 */
native final virtual function ClearBoundDataStores();


defaultproperties
{
	DefaultStates.Add(class'Engine.UIState_Focused')
	DataSource=(RequiredFieldType=DATATYPE_Collection)
	BGPrefab=UIPrefab'UI_Scenes_FrontEnd.Prefabs.OptionBG'
	bRequiresTick=true
}

