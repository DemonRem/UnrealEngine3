/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Extended version of editbox for UT3.
 */
class UTUIEditBox extends UIEditBox
	native(UI);

cpptext
{
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
}


/**
 * Resolves this subscriber's data store binding and updates the subscriber with the current value from the data store.
 *
 * @return	TRUE if this subscriber successfully resolved and applied the updated value.
 */
native virtual function bool RefreshSubscriberValue( optional int BindingIndex=INDEX_NONE );

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
native virtual function bool SaveSubscriberValue( out array<UIDataStore> out_BoundDataStores, optional int BindingIndex=INDEX_NONE );
