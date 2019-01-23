/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 * Our Widgets are collections of UIObjects that are group together with
 * the glue logic to make them tick.
 */

class UTUI_Widget extends UIObject
	abstract
	native(UI);

/** If true, this object require tick */
var bool bRequiresTick;

/** If true, this object will render it's bounds */
var(Widget) transient bool bShowBounds;

/** Cached link to the UTUIScene that owns this widget */
var UTUIScene UTSceneOwner;

cpptext
{
	virtual void Initialize( UUIScene* inOwnerScene, UUIObject* inOwner=NULL );
	virtual void Tick_Widget(FLOAT DeltaTime){};
	virtual void PreRender_Widget(FCanvas* Canvas){};
	virtual void Render_Widget(FCanvas* Canvas);
	virtual void RefreshPosition();

	/**
	  * WARNING: This function does not check the destination and assumes it is valid.
	  *
	  * LookupProperty - Finds a property of a source actor and returns it's value.
	  *
	  * @param		SourceActor			The actor to search
	  * @param		SourceProperty		The property to look up
	  * @out param 	DestPtr				A Point to the storgage of the value
	  *
	  * @Returns true if the look up succeeded
	  */
	virtual UBOOL LookupProperty(AActor* SourceActor, FName SourceProperty, BYTE* DestPtr);

}

function NotifyGameSessionEnded();



/** @return Returns a datastore given its tag. */
function UIDataStore FindDataStore(name DataStoreTag)
{
	local UIDataStore	Result;

	Result = GetCurrentUIController().DataStoreManager.FindDataStore(DataStoreTag);

	return Result;
}

/** @return Returns the controller id of a player given its player index. */
function int GetPlayerControllerId(int PlayerIndex)
{
	local int Result;

	Result = GetCurrentUIController().GetPlayerControllerId(PlayerIndex);;

	return Result;
}

defaultproperties
{
}
