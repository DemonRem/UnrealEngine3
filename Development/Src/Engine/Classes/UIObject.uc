/**
 * Base class for all UI widgets.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UIObject extends UIScreenObject
	native(UIPrivate)
	abstract;

`include(Core/Globals.uci)

/** Unique identifier for this widget */
var	noimport					WIDGET_ID						WidgetID;

/** Unique non-localized name for this widget which is used to reference the widget without needing to know its GUID */
var(Presentation) editconst		name							WidgetTag;

/** the UIObject that contains this widget in its Children array */
var const private duplicatetransient		UIObject			Owner;

/** The scene that owns this widget */
var	const private duplicatetransient		UIScene				OwnerScene;

/** Specifies the style data to use for this widget */
var								UIStyleReference				PrimaryStyle;

/**
 * Controls which widgets are given focus when this widget receives an event that changes
 * the currently focused widget.
 */
var(Focus)						UINavigationData				NavigationTargets;

/**
 * Allows the designer to specify where this widget occurs in the bound (i.e. tab, shift+tab) navigation network of this widget's parent.
 */
var(Focus)						int								TabIndex;

/**
 * The widgets that this widget should be docked to.  For the 'right' and 'bottom' faces, if the widget has
 * no dock target, it is considered docked to the 'left' and 'top' faces, respectively.
 */
var(Presentation) editconst		UIDockingSet					DockTargets;

/** Represents the bounding region available for the widget to render itself in.  Set through the docking system. */
var(Presentation) editconst private	const transient		float	RenderBounds[EUIWidgetFace.UIFACE_MAX];

/**
 * Represents the location of the corners of the widget including any tranforms, in absolute pixels (pixel space).  Starts
 * at the upper-left corner of the widget and goes clockwise.
 */
var(Presentation) editconst private const transient		Vector2D RenderBoundsVertices[EUIWidgetFace.UIFACE_MAX];

/** Rotation of the widget. */
var(Presentation)				UIRotation						Rotation;

/** Screenspace offset to apply to the widget's rendering. */
var(Presentation)				vector							RenderOffset;

/**
 * For widgets using percentage values, transforms the widget's bounds to negate the effects of rotation
 */
//var								Matrix							BoundsAdjustment;

/**
 * Stores a bitmask of flags which modify/define which operations may be performed to this widget (such as renaming, reparenting, selecting, etc.).
 *  Valid behavior flags are defined in UIRoot.uc, as consts which begin with PRIVATE_
 */
var private{private}			int								PrivateFlags;

/** used to differentiate tooltip bindings from others */
const	FIRST_DEFAULT_DATABINDING_INDEX=100;
const	TOOLTIP_BINDING_INDEX=100;
const	CONTEXTMENU_BINDING_INDEX=101;

/**
 * The tool tip for this widget; only relevant for widgets that implement the UIDataStoreSubscriber interface.
 */
var(Data)	private				UIDataStoreBinding				ToolTip;
var(Data)	private	editconst	UIDataStoreBinding				ContextMenuData;

// ===============================================
// ANIMATIONS
// ===============================================
/** Used as the parent in animation sequences */
var						UIObject								AnimationParent;

/** - The following is used in UTGame's UI animation system.
    - It is subject to change at any time.
  */

var transient vector AnimationPosition;

/** This is the stack of animations currently being applied to this UIObject */
var transient array<UIAnimSeqRef> AnimStack;



// ===============================================
// Components
// ===============================================
/**
 * List of objects/components contained by this widget which contain their own style references.  When this widget's style is resolved,
 * each element in this list will receive a notification to resolve its style references as well.
 *
 * Elements should be added to this list either from the native InitializeStyleSubscribers method [for native classes], the Initialized event
 * [for non-native classes], or the native PostEditChange method (when e.g. components are created or removed using the UI editor's property window).
 *
 * You should NEVER add elements to this array using defaultproperties, since interface properties will not be updated to point to the subobject/component
 * instance when this widget is created.
 */
var	transient		array<UIStyleResolver>			StyleSubscribers;

/**
 * Indicates that this widget should receive a call each tick with the location of the mouse cursor while it's the active control (NotifyMouseOver)
 * (caution: slightly degrades performance)
 */
var private{private}	bool						bEnableActiveCursorUpdates;

/**
 * Temp hack to allow widgets to remove "Primary Style" from the styles listed in the context menu for that widget if they no longer use it.
 * Will be removed once I am ready to deprecate the PrimaryStyle property.
 */
var	const			bool							bSupportsPrimaryStyle;

/**
 * Set to true to render an outline marking the widget's RenderBounds.
 */
var(Debug)			bool							bDebugShowBounds;

/**
 * if bRenderBoundingRegion is TRUE, specifies the color to use for this widget.
 */
var(Debug)			color							DebugBoundsColor;

cpptext
{
	/* === UUIObject interface === */
	/**
	 * Render this widget.
	 *
	 * @param	Canvas	the canvas to use for rendering this widget
	 */
	virtual void Render_Widget( FCanvas* Canvas ) {}

	/**
	 * Updates 3D primitives for this widget.
	 *
	 * @param	CanvasScene		the scene to use for updating any 3D primitives
	 */
	virtual void UpdateWidgetPrimitives( FCanvasScene* CanvasScene ) {}

	/**
	 * Allow the widget to do any special rendering after its children have been rendered.
	 *
	 * @param	Canvas	the canvas to use for rendering this widget
	 */
	virtual void PostRender_Widget( FCanvas* Canvas );

	/**
	 * Provides a way for widgets to fill their style subscribers array prior to performing any other initialization tasks.
	 * Child classes should override this method when objects need to be added to the StyleSubscribers array
	 */
	virtual void InitializeStyleSubscribers() {}

	/**
	 * Assigns the style for this widget for the property provided and refreshes the widget's styles.
	 *
	 * @param	NewStyle		the new style to assign to this widget
	 * @param	StyleProperty	The style property we are modifying.
	 * @param	ArrayIndex		if the style property corresponds to an array index, specified the array index to apply the style to
	 *
	 * @return	TRUE if the style was successfully applied to this widget.
	 */
	UBOOL SetWidgetStyle( class UUIStyle* NewStyle, struct FStyleReferenceId StyleProperty=FStyleReferenceId(), INT ArrayIndex=INDEX_NONE );

	/**
	 * Resolves the style references contained by this widget from the currently active skin.
	 *
	 * @param	bClearExistingValue		if TRUE, style references will be invalidated first.
	 * @param	StyleProperty			if specified, only the style reference corresponding to the specified property
	 *									will be resolved; otherwise, all style references will be resolved.
	 *
	 * @return	TRUE if all style references were successfully resolved.
	 */
	UBOOL ResolveStyles( UBOOL bClearExistingValue, struct FStyleReferenceId StyleProperty=FStyleReferenceId() );

protected:
	friend class UUISkin;

	/**
	 * Called when a style reference is resolved successfully.
	 *
	 * @param	ResolvedStyle			the style resolved by the style reference
	 * @param	StyleProperty			the style reference property that was resolved.
	 * @param	ArrayIndex				the array index of the style reference that was resolved.  should only be >0 for style reference arrays.
	 * @param	bInvalidateStyleData	if TRUE, the resolved style is different than the style that was previously resolved by this style reference.
	 */
	virtual void OnStyleResolved( class UUIStyle* ResolvedStyle, const struct FStyleReferenceId& StyleProperty, INT ArrayIndex, UBOOL bInvalidateStyleData );

public:
	/**
	 * Retrieves the list of UIStyleReferences contained by this widget class.  Used to refresh the style data for all style
	 * references contained by this widget whenever the active skin or menu state is changed.
	 *
	 * @param	out_StyleReferences		a map of style property references to UIStyleReference values for the style references contained by this class and
	 *									its StyleSubscribers
	 * @param	TargetStyleRef			if specified, only style references associated with the value specified will be added to the map.
	 * @param	SearchObject			the object to search for style reference properties in; if NULL, searches only in this object; specify INVALID_OBJECT to
	 *									search in this object as well as all subscribed style resolvers
	 */
	void GetStyleReferences( TMultiMap<struct FStyleReferenceId,struct FUIStyleReference*>& out_StyleReferences, FStyleReferenceId TargetStyleRef=FStyleReferenceId(), UObject* SearchObject=NULL );

	/**
	 * Retrieves the list of UIStyleReference properties contained by this widget class.
	 *
	 * @param	out_StyleReferences		a list of the style references contained by this class
	 * @param	SearchObject			the object to search for style reference properties in; if NULL, searches only in this object; specify INVALID_OBJECT to
	 *									search in this object as well as all subscribed style resolvers
	 */
	void GetStyleReferenceProperties( TArray<struct FStyleReferenceId>& out_StyleProperties, UObject* SearchObject=NULL );

	/**
	 * Retrieves the list of data store bindings contained by this widget class.
	 *
	 * @param	out_DataBindings		a map of data binding property name to UIDataStoreBinding values for the data bindings contained by this class
	 * @param	TargetPropertyName		if specified, only data bindings associated with the property specified will be added to the map.
	 */
	void GetDataBindings( TMultiMap<FName,struct FUIDataStoreBinding*>& out_DataBindings, const FName TargetPropertyName=NAME_None );

	/**
	 * Retrieves the list of UIDataStoreBinding properties contained by this widget class.
	 *
	 * @param	out_DataBindingProperties	a list of the data store binding properties contained by this class
	 */
	void GetDataBindingProperties( TArray<class UProperty*>& out_DataBindingProperties );

	/**
	 * Applies the value of bShouldBeDirty to the current style data for all style references in this widget.  Used to force
	 * updating of style data.
	 *
	 * @param	bShouldBeDirty	the value to use for marking the style data for the specified menu state of all style references
	 *							in this widget as dirty.
	 * @param	MenuState		if specified, the style data for that menu state will be modified; otherwise, uses the widget's current
	 *							menu state
	 */
	virtual void ToggleStyleDirtiness( UBOOL bShouldBeDirty, class UUIState* MenuState=NULL );

	/**
	 * Determines whether this widget references the specified style.
	 *
	 * @param	CheckStyle		the style to check for referencers
	 */
	virtual UBOOL UsesStyle( class UUIStyle* CheckStyle );

	/**
	 * Verifies that this widget has a valid WIDGET_ID, and generates one if it doesn't.
	 */
	void ValidateWidgetID();

	/**
	 * @param Point	Point to check against the renderbounds of the object.
	 * @return Whether or not this screen object contains the point passed in within its renderbounds.
	 */
	virtual UBOOL ContainsPoint(const FVector2D& Point) const;

	/**
	 * Projects the vertices made from all faces of this widget and stores the results in the RenderBoundsVertices array.
	 *
	 * @param	bRecursive	specify TRUE to propagate this call to all children of this widget.
	 */
	void UpdateRenderBoundsVertices( UBOOL bRecursive=TRUE );

	/* === UUIScreenObject interface === */
	/**
	 * Returns the UIScreenObject that owns this widget.
	 */
	virtual UUIScreenObject* GetParent() const
	{
		return Owner != NULL ? Owner : (UUIScreenObject*)OwnerScene;
	}

	/**
	 * Returns the UIObject that owns this widget.
	 */
	virtual UUIObject* GetOwner() const			{ return Owner; }

	/**
	 * Get the scene that owns this widget.
	 */
	virtual UUIScene* GetScene() 				{ return OwnerScene; }

	/**
	 * Get the scene that owns this widget.
	 */
	virtual const UUIScene* GetScene() const	{ return OwnerScene; }

	/**
	 * returns the unique tag associated with this screen object
	 */
	virtual FName GetTag() const				{ return WidgetTag; }

	/**
	 * Returns a string representation of this widget's hierarchy.
	 * i.e. SomeScene.SomeContainer.SomeWidget
	 */
	virtual FString GetWidgetPathName() const;

	/** get the currently active skin */
	class UUISkin* GetActiveSkin() const;

	/**
	 * Called when this widget is created.
	 */
	virtual void Created( UUIScreenObject* Creator );

	/**
	 * Called immediately after a child has been added to this screen object.
	 *
	 * @param	WidgetOwner		the screen object that the NewChild was added as a child for
	 * @param	NewChild		the widget that was added
	 */
	virtual void NotifyAddedChild( UUIScreenObject* WidgetOwner, UUIObject* NewChild );

	/**
	 * Called immediately after a child has been removed from this screen object.
	 *
	 * @param	WidgetOwner		the screen object that the widget was removed from.
	 * @param	OldChild		the widget that was removed
	 * @param	ExclusionSet	used to indicate that multiple widgets are being removed in one batch; useful for preventing references
	 *							between the widgets being removed from being severed.
	 */
	virtual void NotifyRemovedChild( UUIScreenObject* WidgetOwner, UUIObject* OldChild, TArray<UUIObject*>* ExclusionSet=NULL );

	/**
	 * Notification that this widget's parent is about to remove this widget from its children array.  Allows the widget
	 * to clean up any references to the old parent.
	 *
	 * @param	WidgetOwner		the screen object that this widget was removed from.
	 * @param	ExclusionSet	allows the caller to specify a group of widgets which should not have inter-references (i.e. references
	 *							to other objects in the set)
	 */
	virtual void NotifyRemovedFromParent( UUIScreenObject* WidgetOwner, TArray<UUIObject*>* ExclusionSet=NULL );

	/**
	 * Called when the currently active skin has been changed.  Reapplies this widget's style and propagates
	 * the notification to all children.
	 */
	virtual void NotifyActiveSkinChanged();

	/**
	 * Called when the scene receives a notification that the viewport has been resized.  Propagated down to all children.
	 *
	 * @param	OldViewportSize		the previous size of the viewport
	 * @param	NewViewportSize		the new size of the viewport
	 */
	virtual void NotifyResolutionChanged( const FVector2D& OldViewportSize, const FVector2D& NewViewportSize );

	/**
	 * Called from UGameUISceneClient::UpdateMousePosition; provides a hook for widgets to respond to the precise cursor
	 * position.  Only called on the scene's ActiveControl if the ActiveControl's bEnableActiveCursorUpdates is TRUE and
	 * the mouse is currently over the widget.
	 *
	 * @param	MousePos	the current position of the mouse, in absolute screen pixels.
	 */
	virtual void NotifyMouseOver( const FVector2D& MousePos ) {}

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
	 * Generates a array of UI Action keys that this widget supports.
	 *
	 * @param	out_KeyNames	Storage for the list of supported keynames.
	 */
	virtual void GetSupportedUIActionKeyNames(TArray<FName> &out_KeyNames );

	/**
	 * Determines whether this widget should process the specified input event + state.  If the widget is configured
	 * to respond to this combination of input key/state, any actions associated with this input event are activated.
	 *
	 * Only called if this widget is in the owning scene's InputSubscribers map for the corresponding key.
	 *
	 * @param	EventParms		the parameters for the input event
	 *
	 * @return	TRUE to consume the key event, FALSE to pass it on.
	 */
	virtual UBOOL ProcessInputKey( const struct FSubscribedInputEventParameters& EventParms );

	/**
	 * Tell the scene that it needs to be udpated
	 *
	 * @param	bDockingStackChanged	if TRUE, the scene will rebuild its DockingStack at the beginning
	 *									the next frame
	 * @param	bPositionsChanged		if TRUE, the scene will update the positions for all its widgets
	 *									at the beginning of the next frame
	 * @param	bNavLinksOutdated		if TRUE, the scene will update the navigation links for all widgets
	 *									at the beginning of the next frame
	 * @param	bWidgetStylesChanged	if TRUE, the scene will refresh the widgets reapplying their current styles
	 */
	virtual void RequestSceneUpdate( UBOOL bDockingStackChanged, UBOOL bPositionsChanged, UBOOL bNavLinksOutdated=FALSE, UBOOL bWidgetStylesChanged=FALSE );

	/**
	 * Tells the scene that it should call RefreshFormatting on the next tick.
	 */
	virtual void RequestFormattingUpdate();

	/**
	 * Notifies the owning UIScene that the primitive usage in this scene has changed and sets flags in the scene to indicate that
	 * 3D primitives have been added or removed.
	 *
	 * @param	bReinitializePrimitives		specify TRUE to have the scene detach all primitives and reinitialize the primitives for
	 *										the widgets which have them.  Normally TRUE if we have ADDED a new child to the scene which
	 *										supports primitives.
	 * @param	bReviewPrimitiveUsage		specify TRUE to have the scene re-evaluate whether its bUsesPrimitives flag should be set.  Normally
	 *										TRUE if a child which supports primitives has been REMOVED.
	 */
	virtual void RequestPrimitiveReview( UBOOL bReinitializePrimitives, UBOOL bReviewPrimitiveUsage );

	/**
	 * Calculates the closes sibling widget for each face of this widget and sets that widget as the navigation target
	 * for that face.
	 */
	virtual void GenerateAutoNavigationLinks();

	/**
	 * Sets the actual navigation target for the specified face.  If the new value is different from the current value,
	 * requests the owning scene to update the navigation links for the entire scene.
	 *
	 * @param	Face			the face to set the navigation link for
	 * @param	NewNavTarget	the widget to set as the link for the specified face
	 *
	 * @return	TRUE if the nav link was successfully set.
	 */
	virtual UBOOL SetNavigationTarget( UUIObject* LeftTarget, UUIObject* TopTarget, UUIObject* RightTarget, UUIObject* BottomTarget );

	/**
	 * Sets the designer-specified navigation target for the specified face.  When navigation links for the scene are rebuilt,
	 * the designer-specified navigation target will always override any auto-calculated targets.  If the new value is different from the current value,
	 * requests the owning scene to update the navigation links for the entire scene.
	 *
	 * @param	Face				the face to set the navigation link for
	 * @param	NavTarget			the widget to set as the link for the specified face
	 * @param	bIsNullOverride		if NavTarget is NULL, specify TRUE to indicate that this face's nav target should not
	 *								be automatically calculated.
	 *
	 * @return	TRUE if the nav link was successfully set.
	 */
	virtual UBOOL SetForcedNavigationTarget( UUIObject* LeftTarget, UUIObject* TopTarget, UUIObject* RightTarget, UUIObject* BottomTarget );

	/**
	 * Gets the navigation target for the specified face.  If a designer-specified nav target is set for the specified face,
	 * that object is returned.
	 *
	 * @param	Face		the face to get the nav target for
	 * @param	LinkType	specifies which navigation link type to return.
	 *							NAVLINK_MAX: 		return the designer specified navigation target, if set; otherwise returns the auto-generated navigation target
	 *							NAVLINK_Automatic:	return the auto-generated navigation target, even if the designer specified nav target is set
	 *							NAVLINK_Manual:		return the designer specified nav target, even if it isn't set
	 *
	 * @return	a pointer to a widget that will be the navigation target for the specified direction, or NULL if there is
	 *			no nav target for that face.
	 */
	UUIObject* GetNavigationTarget( EUIWidgetFace Face, ENavigationLinkType LinkType=NAVLINK_MAX ) const;

	/**
	 * Determines if the specified widget is a valid candidate for being the nav target for the specified face of this widget.
	 *
	 * @param	Face			the face that we'd be assigning NewNavTarget to
	 * @param	NewNavTarget	the widget that we'd like to make the nav target for that face
	 *
	 * @return	TRUE if NewNavTarget is allowed to be the navigation target for the specified face of this widget.
	 */
	virtual UBOOL IsValidNavigationTarget( EUIWidgetFace Face, UUIObject* NewNavTarget ) const;

	/**
	 * Determines whether the specified widget can be set as a docking target for the specified face.
	 *
	 * @param	SourceFace		the face on this widget that the potential dock source
	 * @param	Target			the potential docking target
	 * @param	TargetFace		the face on the target widget that we want to check for
	 *
	 * @return	TRUE if a docking link can be safely established between SourceFace and TargetFace.
	 */
	UBOOL IsValidDockTarget( EUIWidgetFace SourceFace, UUIObject* Target, EUIWidgetFace TargetFace );

	/**
     *	Actually update the scene by rebuilding docking and resolving positions.
     */
	virtual void UpdateScene();

	/**
	 * Adds docking nodes for all faces of this widget to the specified scene
	 *
	 * @param	DockingStack	the docking stack to add this widget's docking.  Generally the scene's DockingStack.
	 *
	 * @return	TRUE if docking nodes were successfully added for all faces of this widget.
	 */
	virtual UBOOL AddDockingLink( TArray<struct FUIDockingNode>& DockingStack );

	/**
	 * Adds the specified face to the DockingStack for the specified widget
	 *
	 * @param	DockingStack	the docking stack to add this docking node to.  Generally the scene's DockingStack.
	 * @param	Face			the face that should be added
	 *
	 * @return	TRUE if a docking node was added to the scene's DockingStack for the specified face, or if a docking node already
	 *			existed in the stack for the specified face of this widget.
	 */
	virtual UBOOL AddDockingNode( TArray<struct FUIDockingNode>& DockingStack, EUIWidgetFace Face );

	/**
	 * Evalutes the Position value for the specified face into an actual pixel value.  Should only be
	 * called from UIScene::ResolvePositions.  Any special-case positioning should be done in this function.
	 *
	 * @param	Face	the face that should be resolved
	 */
	virtual void ResolveFacePosition( EUIWidgetFace Face );

	/**
	 * Returns the number of faces this widget has resolved.
	 */
	virtual INT GetNumResolvedFaces() const;

	/**
	 * Returns whether the specified face has been resolved
	 *
	 * @param	Face	the face to check
	 */
	virtual UBOOL HasPositionBeenResolved( EUIWidgetFace Face ) const
	{
		return DockTargets.bResolved[Face] != 0;
	}

protected:
	/**
	 * Marks the Position for any faces dependent on the specified face, in this widget or its children,
	 * as out of sync with the corresponding RenderBounds.
	 *
	 * @param	Face	the face to modify; value must be one of the EUIWidgetFace values.
	 */
	virtual void InvalidatePositionDependencies( BYTE Face );

public:
	/**
	 * Adds the specified state to the screen object's StateStack and refreshes the widget style using the new state.
	 *
	 * @param	StateToActivate		the new state for the widget
	 * @param	PlayerIndex			the index [into the Engine.GamePlayers array] for the player that generated this call
	 *
	 * @return	TRUE if the widget's state was successfully changed to the new state.  FALSE if the widget couldn't change
	 *			to the new state or the specified state already exists in the widget's list of active states
	 */
	virtual UBOOL ActivateState( class UUIState* StateToActivate, INT PlayerIndex );

	/**
	 * Changes the specified preview state on the screen object's StateStack and refreshes the widget style using the new state.
	 *
	 * @param	StateToActivate		the new preview state for the widget to apply within the Editor
	 *
	 * @return	TRUE if the widget's state was successfully changed to the new preview state.  FALSE if the widget couldn't change
	 *			to the new state or the specified state already exists in the widget's list of active states
	 */
	virtual UBOOL ActivatePreviewState( class UUIState* StateToActivate );

	/**
	 * Removes the specified state from the screen object's state stack and refreshes the widget's style using the new state.
	 *
	 * @param	StateToRemove	the state to be removed
	 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player that generated this call
	 *
	 * @return	TRUE if the state was successfully removed, or if the state didn't exist in the widget's list of states;
	 *			false if the state overrode the request to be removed
	 */
	virtual UBOOL DeactivateState( class UUIState* StateToRemove, INT PlayerIndex );

	/**
	 * Activates the tooltip associated with this widget.  Called when this widget (or one of its Children) becomes
	 * the scene client's ActiveControl.  If this widget doesn't support tool-tips or does not have a valid tool-tip
	 * binding, the call is propagated upwards to the parent widget.
	 *
	 * @return	a pointer to the tool-tip that was activated.  This value will be stored in the scene's ActiveToolTip member.
	 */
	class UUIToolTip* ActivateToolTip();

	/**
	 * Activates the context menu for this widget, if it has one.  Called when the user right-clicks (or whatever input key
	 * is configured to activate the ShowContextMenu UI input alias) while this widget (or one of its Children) is the
	 * scene client's ActiveControl.
	 *
	 * @return	a pointer to the context menu that was activated.  This value will be stored in the scene's ActiveContextMenu member.
	 */
	class UUIContextMenu* ActivateContextMenu( INT PlayerIndex );

	/**
	 * Decide if we need to apply this animation
	 *
	 * @Param DeltaTime			How much time since the last call
	 * @Param AnimSeqRef		*OUTPUT* The AnimSeq begin applied
	 *
	 * @Returns true if the animation has reached the end.
	 */
	 virtual UBOOL TickSequence( FLOAT DeltaTime, struct FUIAnimSeqRef& AnimSeqRef );

	/* === UObject interface === */
	/**
	 * Called after this object has been de-serialized from disk.
	 *
	 * This version converts the deprecated PRIVATE_DisallowReparenting flag to PRIVATE_EditorNoReparent, if set.
	 */
	virtual void PostLoad();

	/**
	 * Called when a property value has been changed in the editor.
	 */
	virtual void PostEditChange( UProperty* PropertyThatChanged );

	/**
	 * Called when a property value has been changed in the editor.
	 */
	virtual void PostEditChange( FEditPropertyChain& PropertyThatChanged );

	/**
	 * Called after importing property values for this object (paste, duplicate or .t3d import)
	 * Allow the object to perform any cleanup for properties which shouldn't be duplicated or
	 * are unsupported by the script serialization
	 */
	virtual void PostEditImport();

	/**
	 * Called after this widget is renamed; ensures that the widget's tag matches the name of the widget.
	 */
	virtual void PostRename();
}

/* ==========================================================================================================
	UIObject interface.
========================================================================================================== */

/* == Delegates == */

/**
 * Called when this widget is created
 *
 * @param	CreatedWidget		the widget that was created
 * @param	CreatorContainer	the container that created the widget
 */
delegate OnCreate( UIObject CreatedWidget, UIScreenObject CreatorContainer );

/**
 * Called when the value of this UIObject is changed.  Only called for widgets that contain data values.
 *
 * @param	Sender			the UIObject whose value changed
 * @param	PlayerIndex		the index of the player that generated the call to this method; used as the PlayerIndex when activating
 *							UIEvents; if not specified, the value of GetBestPlayerIndex() is used instead.
 */
delegate OnValueChanged( UIObject Sender, int PlayerIndex );

/**
 * Called when this widget is pressed.  Not implemented by all widget types.
 *
 * @param	EventObject	Object that issued the event.
 * @param	PlayerIndex	Player that performed the action that issued the event.
 */
delegate OnPressed( UIScreenObject EventObject, int PlayerIndex );

/**
 * Called when the widget been pressed and the user is holding the button down.  Not implemented by all widget types.
 *
 * @param	EventObject	Object that issued the event.
 * @param	PlayerIndex	Player that performed the action that issued the event.
 */
delegate OnPressRepeat( UIScreenObject EventObject, int PlayerIndex );

/**
 * Called when the widget is no longer being pressed.  Not implemented by all widget types.
 *
 * @param	EventObject	Object that issued the event.
 * @param	PlayerIndex	Player that performed the action that issued the event.
 */
delegate OnPressRelease( UIScreenObject EventObject, int PlayerIndex );

/**
 * Called when the widget is no longer being pressed.  Not implemented by all widget types.
 *
 * The difference between this delegate and the OnPressRelease delegate is that OnClick will only be called on the
 * widget that received the matching key press. OnPressRelease will be called on whichever widget was under the cursor
 * when the key was released, which might not necessarily be the widget that received the key press.
 *
 * @param EventObject	Object that issued the event.
 * @param PlayerIndex	Player that performed the action that issued the event.
 *
 * @return	return TRUE to prevent the kismet OnClick event from firing.
 */
delegate bool OnClicked(UIScreenObject EventObject, int PlayerIndex);

/**
 * Called when the widget has received a double-click input event.  Not implemented by all widget types.
 *
 * @param	EventObject	Object that issued the event.
 * @param	PlayerIndex	Player that performed the action that issued the event.
 */
delegate OnDoubleClick( UIScreenObject EventObject, int PlayerIndex );

/**
 * Called when this widget (or one of its children) becomes the ActiveControl.  Provides a way for child classes or
 * containers to easily override or short-circuit the standard tooltip that is normally shown.  If this delegate is
 * not assigned to any function, the default tool-tip will be displayed if this widget has a data store binding property
 * named "ToolTipBinding" which is bound to a valid data store.
 *
 * @param	Sender			the widget that will be displaying the tooltip
 * @param	CustomToolTip	to provide a custom tooltip implementation, fill in in this value and return TRUE.  The custom
 *							tool tip object will then be activated by native code.
 *
 * @return	return FALSE to prevent any tool-tips from being shown, including parents.
 */
delegate bool OnQueryToolTip( UIObject Sender, out UIToolTip CustomToolTip );

/**
 * Called when the user right-clicks (or whatever input key is configured to activate the ShowContextMenu UI input alias)
 * this widget.  Provides a way for widgets to customize the context menu that is used or prevent the context menu from being
 * shown entirely.
 *
 * For script customization of the context menu, a custom context menu object must be assigned to the CustomContextMenu variable.
 * It is possible to provide data for the context menu without creating or modifying any existing data stores.  First, get a reference
 * to the scene's default context menu (GetScene()->GetDefaultContextMenu()).  Add the desired elements to the scene's data store then
 * bind the context menu to that data field.
 *
 * @param	Sender				the widget that will be displaying the context menu
 * @param	PlayerIndex			index of the player that generated the input event that triggered the context menu display.
 * @param	CustomContextMenu	to provide a custom tooltip implementation, fill in in this value and return TRUE.  The custom
 *								context menu will then be activated by native code.
 *
 * @return	return FALSE to prevent a context menu from being shown, including any from parent widgets.  Return TRUE to indicate
 *			that the context menu for this widget can be displayed; if a value is not provided for CustomContextMenu,
 *			the default context menu will be displayed, using this widget's context menu data binding to generate the items.
 */
delegate bool OnOpenContextMenu( UIObject Sender, int PlayerIndex, out UIContextMenu CustomContextMenu );

/**
 * Called when the system wants to close the currently active context menu.
 *
 * @param	ContextMenu		the context menu that is going to be closed
 * @param	PlayerIndex		the index of the player that generated the request for the context menu to be closed.
 *
 * @return	TRUE to allow the specified context menu to be closed; FALSE to prevent the context menu from being closed.
 *			Note that there are certain situations where the context menu will be closed regardless of the return value,
 *			such as when the scene which owns the context menu is being closed.
 */
delegate bool OnCloseContextMenu( UIContextMenu ContextMenu, int PlayerIndex );

/**
 * Called when the user selects a choice from a context menu.
 *
 * @param	ContextMenu		the context menu that called this delegate.
 * @param	PlayerIndex		the index of the player that generated the event.
 * @param	ItemIndex		the index [into the context menu's MenuItems array] for the item that was selected.
 */
delegate OnContextMenuItemSelected( UIContextMenu ContextMenu, int PlayerIndex, int ItemIndex );

/* == Events == */


/* == Natives == */
/**
 * Set the markup text for a default data binding to the value specified.
 *
 * @param	NewMarkupText	the new markup text for this widget, either a literal string or a data store markup string
 * @param	BindingIndex	indicates which data store binding to operate on.
 */
native final function SetDefaultDataBinding( string MarkupText, int BindingIndex );

/**
 * Returns the data binding's current value.
 *
 * @param	BindingIndex	indicates which data store binding to operate on.
 */
native final function string GetDefaultDataBinding( int BindingIndex ) const;

/**
 * Resolves the data binding's markup string.
 *
 * @param	BindingIndex	indicates which data store binding to operate on.
 *
 * @return	TRUE if a data store field was successfully resolved from the data binding
 */
native final function bool ResolveDefaultDataBinding( int BindingIndex );

/**
 * Returns the data store providing the data for all default data bindings.
 */
native final function GetDefaultDataStores( out array<UIDataStore> out_BoundDataStores );

/**
 * Clears the reference to the bound data store, if applicable.
 *
 * @param	BindingIndex	indicates which data store binding to operate on.
 */
native final function ClearDefaultDataBinding( int BindingIndex );

/**
 * Generates a string which can be used to interact with temporary data in the scene data store specific to this widget.
 *
 * @param	Group	for now, doesn't matter, as only "ContextMenuItems" is supported
 *
 * @return	a data store markup string which can be used to reference content specific to this widget in the scene's
 *			data store.
 */
native function string GenerateSceneDataStoreMarkup( optional string Group="ContextMenuItems" ) const;


/** ===== Tool tips ===== */
/**
 * Returns the ToolTip data binding's current value after being resolved.
 */
native final function string GetToolTipValue();

/** ===== Rotation ===== */
/**
 * Determines whether this widget has any tranformation applied to it.
 *
 * @param	bIncludeParentTransforms	specify TRUE to check whether this widget's parents are transformed if this one isn't.
 */
native final function bool HasTransform( optional bool bIncludeParentTransforms=true ) const;

/**
 * Sets the location of the widget's rotation anchor, relative to the top-left of this widget's bounds.
 *
 * @param	AnchorPosition	New location for the widget's rotation anchor.
 * @param	InputType		indicates which format the AnchorPos value is in
 */
native final function SetAnchorPosition( vector NewAnchorPosition, optional EPositionEvalType InputType=EVALPOS_PixelViewport );

/**
 * Rotates the widget around the current anchor position by the amount specified.
 *
 * @param	RotationDelta		amount to rotate the widget by in DEGREES.
 * @param	bAccumulateRotation	if FALSE, set the widget's rotation to NewRotationAmount; if TRUE, increments the
 *								widget's rotation by NewRotationAmount
 */
native final function RotateWidget( rotator NewRotationAmount, optional bool bAccumulateRotation );

/**
 * Updates the widget's rotation matrix based on the widget's current rotation.
 */
native final function UpdateRotationMatrix();

/**
 * Returns the current location of the anchor.
 *
 * @param	bRelativeToWidget	specify TRUE to return the anchor position relative to the widget's upper left corner.
 *								specify FALSE to return the anchor position relative to the viewport's origin.
 * @param	bPixelSpace			specify TRUE to convert the anchor position into pixel space (only relevant if the widget is rotated)
 *
 * @return	a vector representing the position of this widget's rotation anchor.
 */
native final function vector GetAnchorPosition( optional bool bRelativeToWidget=true, optional bool bPixelSpace ) const;

/**
 * Generates a matrix which contains a translation for this widget's position (from 0,0 screen space) as well as the widget's
 * current rotation, scale, etc.
 *
 * @param	bIncludeParentTransforms	if TRUE, the matrix will be relative to the parent widget's own transform matrix.
 *
 * @return	a matrix containing the translation and rotation values of this widget.
 *
 * @todo ronp - we REALLY need to cache this baby and update it any time the widget's position, anchor,
 *				rotation, scale, or parent changes.
 */
native final function Matrix GenerateTransformMatrix( optional bool bIncludeParentTransforms=true ) const;

/**
 * Returns this widget's current rotation matrix
 *
 * @param	bIncludeParentRotations	if TRUE, the matrix will be relative to the parent widget's own rotation matrix.
 */
native final function Matrix GetRotationMatrix( optional bool bIncludeParentRotations=true ) const;

/**
 * Called whenever the value of the UIObject is modified (for those UIObjects which can have values).
 * Calls the OnValueChanged delegate.
 *
 * @param	PlayerIndex		the index of the player that generated the call to SetValue; used as the PlayerIndex when activating
 *							UIEvents; if not specified, the value of GetBestPlayerIndex() is used instead.
 * @param	NotifyFlags		optional parameter for individual widgets to use for passing additional information about the notification.
 */
native function NotifyValueChanged( optional int PlayerIndex=INDEX_NONE, optional int NotifyFlags=0 );

/**
 * Returns TRUE if TestWidget is in this widget's Owner chain.
 */
native final function bool IsContainedBy( UIObject TestWidget );

/**
 * Sets the docking target for the specified face.
 *
 * @param	SourceFace	the face of this widget to apply the changes to
 * @param	Target		the widget to dock to
 * @param	TargetFace	the face on the Target widget that SourceFace will dock to
 *
 * @return	TRUE if the changes were successfully applied.
 */
native function bool SetDockTarget( EUIWidgetFace SourceFace, UIScreenObject Target, EUIWidgetFace TargetFace );

/**
 * Sets the padding for the specified docking link.
 *
 * @param	SourceFace	the face of this widget to apply the changes to
 * @param	Padding		the amount of padding to use for this docking set.  Positive values will "push" this widget past the
 *						target face of the other widget, while negative values will "pull" this widget away from the target widget.
 * @param	PaddingInputType
 *						specifies how the Padding value should be interpreted.
 * @param	bModifyPaddingScaleType
 *						specify TRUE to change the DockPadding's ScaleType to the PaddingInputType.
 *
 * @return	TRUE if the changes were successfully applied.
 */
native function bool SetDockPadding( EUIWidgetFace SourceFace, float PaddingValue, optional EUIDockPaddingEvalType PaddingInputType=UIPADDINGEVAL_Pixels, optional bool bModifyPaddingScaleType );

/**
 * Combines SetDockTarget and SetDockPadding into a single function.
 *
 * @param	SourceFace	the face of this widget to apply the changes to
 * @param	Target		the widget to dock to
 * @param	TargetFace	the face on the Target widget that SourceFace will dock to
 * @param	Padding		the amount of padding to use for this docking set.  Positive values will "push" this widget past the
 *						target face of the other widget, while negative values will "pull" this widget away from the target widget.
 * @param	PaddingInputType
 *						specifies how the Padding value should be interpreted.
 * @param	bModifyPaddingScaleType
 *						specify TRUE to change the DockPadding's ScaleType to the PaddingInputType.
 *
 * @return	TRUE if the changes were successfully applied.
 */
native final function bool SetDockParameters( EUIWidgetFace SourceFace, UIScreenObject Target, EUIWidgetFace TargetFace, float PaddingValue, optional EUIDockPaddingEvalType PaddingInputType=UIPADDINGEVAL_Pixels, optional bool bModifyPaddingScaleType );

/**
 * Returns TRUE if this widget is docked to the specified widget.
 *
 * @param	TargetWidget	the widget to check for docking links to
 * @param	SourceFace		if specified, returns TRUE only if the specified face is docked to TargetWidget
 * @param	TargetFace		if specified, returns TRUE only if this widget is docked to the specified face on the target widget.
 */
native final function bool IsDockedTo( const UIScreenObject TargetWidget, optional EUIWidgetFace SourceFace=UIFACE_MAX, optional EUIWidgetFace TargetFace=UIFACE_MAX ) const;

/**
* Sets the actual navigation target for the specified face.  If the new value is different from the current value,
* requests the owning scene to update the navigation links for the entire scene.
*
* @param	Face			the face to set the navigation link for
* @param	NewNavTarget	the widget to set as the link for the specified face
*
* @return	TRUE if the nav link was successfully set.
*/
native final function bool SetNavigationTarget( EUIWidgetFace Face, UIObject NewNavTarget );

/**
* Sets the designer-specified navigation target for the specified face.  When navigation links for the scene are rebuilt,
* the designer-specified navigation target will always override any auto-calculated targets.  If the new value is different from the current value,
* requests the owning scene to update the navigation links for the entire scene.
*
* @param	Face				the face to set the navigation link for
* @param	NavTarget			the widget to set as the link for the specified face
* @param	bIsNullOverride		if NavTarget is NULL, specify TRUE to indicate that this face's nav target should not
*								be automatically calculated.
*
* @return	TRUE if the nav link was successfully set.
*/
native final function bool SetForcedNavigationTarget( EUIWidgetFace Face, UIObject NavTarget, bool bIsNullOverride=FALSE );

/**
 * Determines whether this widget can become the focused control. In the case of this widget we don't want it to gain focus.
 *
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player to check focus availability
 *
 * @return	TRUE if this widget (or any of its children) is capable of becoming the focused control.
 */
native function bool CanAcceptFocus( optional int PlayerIndex=0 ) const;

/**
 * Checks to see if the specified private behavior is set. Valid behavior flags are defined in UIRoot.uc, as consts which begin with PRIVATE_
 *
 * @param	Behavior	the flag of the private behavior that is being checked
 *
 * @return	TRUE if the specified flag is set and FALSE if not.
 */
native final function bool IsPrivateBehaviorSet( int Behavior ) const;

/**
 * Set the specified private behavior for this UIObject. Valid behavior flags are defined in UIRoot.uc, as consts which begin with PRIVATE_
 *
 * @param	Behavior	the flag of the private behavior that is being set
 * @param	Value		whether the flag is being enabled or disabled
 * @param	bRecurse	specify TRUE to apply the flag in all children of this widget as well.
 */
native final function SetPrivateBehavior( int Behavior, bool Value, optional bool bRecurse );

/**
 * Change the value of bEnableActiveCursorUpdates to the specified value.
 */
native function SetActiveCursorUpdate( bool bShouldReceiveCursorUpdates );

/**
 * Returns the value of bEnableActiveCursorUpdates
 */
native final function bool NeedsActiveCursorUpdates() const;

/**
 * Gets the minimum and maximum values for the widget's face positions after rotation (if specified) has been applied.
 *
 * @param	MinX				The minimum x position of this widget.
 * @param	MaxX				The maximum x position of this widget.
 * @param	MinY				The minimum y position of this widget.
 * @param	MaxY				The maximum y position of this widget.
 * @param	bIncludeRotation	Indicates whether the widget's rotation should be applied to the extent values.
 */
native final function GetPositionExtents( out float MinX, out float MaxX, out float MinY, out float MaxY, optional bool bIncludeRotation ) const;

/**
 * Gets the minimum or maximum value for the specified widget face position after rotation has been applied.
 *
 * @param	bIncludeRotation	Indicates whether the widget's rotation should be applied to the extent values.
 */
native final function float GetPositionExtent( EUIWidgetFace Face, optional bool bIncludeRotation ) const;

/**
 * Adds the specified StyleResolver to the list of StyleSubscribers
 *
 * @param	StyleSubscriberId	the name to associate with this UIStyleResolver; used for differentiating styles from
 *								multiple UIStyleResolvers of the same class
 * @param	Subscriber			the UIStyleResolver to add.
 */
native final function AddStyleSubscriber( const out UIStyleResolver Subscriber );

/**
 * Removes the specified StyleResolver from the list of StyleSubscribers.
 *
 * @param	Subscriber		the subscriber to remove
 * @param	SubscriberId	if specified, Subscriber will only be removed if its SubscriberId matches this value.
 */
native final function RemoveStyleSubscriber( const out UIStyleResolver Subscriber );

/**
 * Returns the index [into the StyleSubscriber's array] for the specified UIStyleResolver, or INDEX_NONE if Subscriber
 * is NULL or is not found in the StyleSubscriber's array.
 *
 * @param	Subscriber		the subscriber to find
 * @param	SubscriberId	if specified, it will only be considered a match if the SubscriberId associated with Subscriber matches this value.
 */
native final function int FindStyleSubscriberIndex( const out UIStyleResolver Subscriber );

/**
 * Returns the index [into the StyleSubscriber's array] for the subscriber which has a StyleResolverTag that matches the specified value
 * or INDEX_NONE if StyleSubscriberId is None or is not found in the StyleSubscriber's array.
 *
 * @param	StyleSubscriberId	the tag associated with the UIStyleResolver to find
 */
native final function int FindStyleSubscriberIndexById( name StyleSubscriberId );


/** --------- Animations ------------- */

/**
 * Itterate over the AnimStack and tick each active sequence
 *
 * @Param DeltaTime			How much time since the last call
 */

native function TickAnim(FLOAT DeltaTime);


/* == Unrealscript == */

/**
 * Returns the scene that owns this widget
 */
final function UIScene GetScene()
{
	return OwnerScene;
}

/**
 * Returns the owner of this widget
 */
final function UIObject GetOwner()
{
	return Owner;
}

/**
 * Returns the scene or widget that contains this widget in its Children array.
 */
function UIScreenObject GetParent()
{
	local UIScreenObject Result;

	Result = GetOwner();
	if ( Result == None )
	{
		Result = GetScene();
	}

	return Result;
}


function LogRenderBounds( int Indent )
{
`if(`notdefined(FINAL_RELEASE))
	local int i;
	local string IndentString;

	for ( i = 0; i < Indent; i++ )
	{
		IndentString $= " ";
	}

	`log(IndentString $ "'" $ WidgetTag $ "': (" $ RenderBounds[0] $ "," $ RenderBounds[1] $ "," $ RenderBounds[2] $ "," $ RenderBounds[3] $ ") Pos:(" $ Position.Value[0] $ "," $ Position.Value[1] $ "," $ Position.Value[2] $ "," $ Position.Value[3] $ ")");
	for ( i = 0; i < Children.Length; i++ )
	{
		Children[i].LogRenderBounds(Indent + 3);
	}
`endif
}

/** Kismet Action Handlers */
function OnSetDatastoreBinding( UIAction_SetDatastoreBinding Action )
{
	local UIDataStoreSubscriber Subscriber;

	Subscriber = Self;
	if ( Subscriber != None )
	{
		Subscriber.SetDataStoreBinding(Action.NewMarkup, Action.BindingIndex);
	}
}
// ===============================================
// ANIMATIONS
// ===============================================

/**
 * Note these are accessor functions for the animation system.  They should
 * be subclassed.
 */

native function AnimSetOpacity(float NewOpacity);
native function AnimSetVisibility(bool bIsVisible);
native function AnimSetColor(LinearColor NewColor);
native function AnimSetPosition(Vector NewPosition);
native function AnimSetRelPosition(Vector NewPosition, Vector InitialPosition);
native function AnimSetRotation(Rotator NewRotation);
native function AnimSetScale(float NewScale);

/**
 * Play an animation on this UIObject
 *
 * @Param AnimName			The Name of the Animation to play
 * @Param AnimSeq			Optional, A Sequence Template.  If that's set, we use it instead
 * @Param PlaybackRate  	Optional, How fast to play back the sequence
 * @Param InitialPosition	Optional, Where in the sequence should we start
 *
 */
event PlayUIAnimation(name AnimName, optional UIAnimationSeq AnimSeqTemplate,
						optional float PlaybackRate=1.0,optional bool bLoop, optional float InitialPosition=0.0)
{
	local UIAnimationSeq TargetAnimSeq;
	local GameUISceneClient SClient;
	local int Idx;

	SClient = GetSceneClient();
	if ( SClient == none )
	{
		return;
	}

	// If we are already on the stack, just reset us
	if ( AnimName != '' )
	{
		for (Idx=0;Idx<AnimStack.Length;Idx++)
		{
			if ( AnimStack[Idx].SeqRef.SeqName == AnimName )
			{
				AnimStack[Idx].PlaybackRate = PlaybackRate;
				AnimStack[Idx].bIsPlaying = true;
				AnimStack[Idx].bIsLooping = bLoop;
				AnimStack[Idx].LoopCount = 0;

				AnimStack[Idx].AnimTime = AnimStack[Idx].SeqRef.SeqDuration * InitialPosition;

				// Force the first frame, whatever it may be.
				TickAnim(0.0);

/*
				// Apply the first frame of each track

	            for (i=0;i<AnimStack[Idx].SeqRef.Tracks.Length; i++)
	            {
	            	TargetObj = AnimStack[Idx].SeqRef.Tracks[i].TargetWidget;
					AnimStack[Idx].SeqRef.ApplyAnimation( TargetObj == none ? self : TargetObj, i, 0.0, 0, 0);
				}

				AnimStack[Idx].AnimTime = 0.0f;
*/
				SClient.AnimSubscribe(self);
				return;
			}
		}
	}


	if ( AnimSeqTemplate != none )
	{
		TargetAnimSeq = AnimSeqTemplate;
	}
	else
	{
		TargetAnimSeq = SClient.AnimLookupSequence(AnimName);
	}

	if ( TargetAnimSeq != none && TargetAnimSeq.Tracks.Length > 0 )
	{
		Idx = AnimStack.Length;
		AnimStack.length = AnimStack.length + 1;
		AnimStack[Idx].SeqRef = TargetAnimSeq;
		AnimStack[Idx].PlaybackRate = PlaybackRate;
		AnimStack[Idx].bIsPlaying = true;
		AnimStack[Idx].bIsLooping = bLoop;
		AnimStack[Idx].LoopCount = 0;
		AnimStack[Idx].InitialRenderOffset = RenderOffset;
		AnimStack[Idx].InitialRotation = Rotation.Rotation;

		AnimStack[Idx].AnimTime = TargetAnimSeq.SeqDuration * InitialPosition;
		// Apply the first frame of each track

		TickAnim(0.0);
/*
	for (i=0;i<TargetAnimSeq.Tracks.Length; i++)
	{
		TargetObj = self;

		// If this track affects a child widget, then cache a reference
		// to it here.

			if (TargetAnimSeq.Tracks[i].TrackWidgetTag != '')
			{
				TargetObj = FindChild(TargetAnimSeq.Tracks[i].TrackWidgetTag,true);
				if ( TargetObj != none )
				{
					TargetAnimSeq.Tracks[i].TargetWidget = TargetObj;
				}
				else
				{
					`log("UIAnimation - "$AnimName$" could not find Target Widget "$TargetAnimSeq.Tracks[i].TargetWidget);

					// Reset the TargetObj to the parent

					TargetObj = self;
				}
			}

			TargetAnimSeq.ApplyAnimation(TargetObj, i, 0.0, 0, 0);
		}

		AnimStack[Idx].AnimTime = 0.0f;
*/
		SClient.AnimSubscribe(self);
	}
	else
	{
		`log("UIAnimation - Attempted to play unknown sequence '"$AnimName$"' on "@WidgetTag);
	}
}

/**
 * Stop an animation that is playing.
 *
 * @Param AnimName		The Name of the animation to play
 * @Param AnimSeq		Optional sequence to use.  In case you don't know the name
 * @Param bFinalize		If true, we will force the end frame
 */
event StopUIAnimation(name AnimName, optional UIAnimationSeq AnimSeq, optional bool bFinalize)
{
	local int i, Idx, FIdx;
	local UIAnimationSeq Seq;
	local UIObject TargetWidget;

	for ( Idx=0; Idx < AnimStack.Length; Idx++)
	{
		if ( (AnimName != '' && AnimStack[Idx].SeqRef.SeqName == AnimName) ||
				(AnimSeq != none && AnimStack[Idx].SeqRef == AnimSeq) )
		{
			if (bFinalize)
			{
				Seq = AnimStack[Idx].SeqRef;
				for (i=0; i<Seq.Tracks.Length; i++)
				{

					FIdx = Seq.Tracks[i].KeyFrames.Length - 1;
					if ( Seq.Tracks[i].KeyFrames[FIdx].TimeMark < 1.0 )
					{
						FIdx = 0;
					}

					if (Seq.Tracks[i].TargetWidget != none )
					{
						TargetWidget = Seq.Tracks[i].TargetWidget;
					}
					else
					{
						TargetWidget = self;
					}

					Seq.ApplyAnimation(TargetWidget, i, 1.0, FIdx, FIdx, AnimStack[Idx]);
				}
			}
			UIAnimEnd(Idx);
		}
	}
}

/**
 * Clears the animation from the stack.  Stopping an animation (either naturally or by
 * force) doesn't remove the effects.  In those cases you need to clear the animation
 * from the stack.
 *
 * NOTE: This only affects position and rotation animations.  All other animations
 * 		 are destructive and can't be easilly reset.
 *
 * @Param AnimName		The Name of the animation to play
 * @Param AnimSeq		Optional sequence to use.  In case you don't know the name
 */

event ClearUIAnimation(name AnimName, optional UIAnimationSeq AnimSeq)
{
	local int Idx;
	for ( Idx=0; Idx < AnimStack.Length; Idx++)
	{
		if ( (AnimName != '' && AnimStack[Idx].SeqRef.SeqName == AnimName) ||
				(AnimSeq != none && AnimStack[Idx].SeqRef == AnimSeq) )
		{
			// If we are playing, turn us off
			if ( AnimStack[Idx].bIsPlaying )
			{
				StopUIAnimation('',AnimStack[Idx].SeqRef,false);
			}
			AnimStack.Remove(Idx,1);
		}
	}

}

/**
 * AnimEnd is always called when an animation stops playing either by ending, or
 * when StopAnim is called on it.  It's responsible for unsubscribing from the scene client
 * and performing any housekeeping
 */
event UIAnimEnd(int SeqIndex)
{
	local int i;
	local bool bRemove;

	// Notify anyone listening
	OnUIAnimEnd( Self, SeqIndex, AnimStack[SeqIndex].SeqRef);

	// Notify the Scene
	GetScene().AnimEnd( Self, SeqIndex, AnimStack[SeqIndex].SeqRef);

	// Turn it off

	AnimStack[SeqIndex].bIsPlaying = false;
	AnimStack[SeqIndex].bIsLooping = false;

	// Look at out anim stack.  If we don't have any active animations,
	// remove unsubscribe us.

	bRemove = true;
	for (i=0;i<AnimStack.Length;i++)
	{
		if ( AnimStack[i].bIsPlaying )
		{
			bRemove = false;
		}
	}

	if ( bRemove )
	{
		// Unsubscribe
		GetSceneClient().AnimUnSubscribe( self );
	}
}

/**
 * If set, this delegate is called whenever an animation is finished
 */

delegate OnUIAnimEnd(UIObject AnimTarget, int AnimIndex, UIAnimationSeq AnimSeq);


DefaultProperties
{
	Opacity=1.f
	TabIndex=-1
	PrimaryStyle=(DefaultStyleTag="DefaultComboStyle")
	bSupportsPrimaryStyle=true
	DebugBoundsColor=(R=255,B=255,G=128,A=255)
	ToolTip=(RequiredFieldType=DATATYPE_Property,BindingIndex=TOOLTIP_BINDING_INDEX)
	ContextMenuData=(RequiredFieldType=DATATYPE_Collection,BindingIndex=CONTEXTMENU_BINDING_INDEX)

	// default to Identity matrix
//	BoundsAdjustment=(XPlane=(X=1,Y=0,Z=0,W=0),YPlane=(X=0,Y=1,Z=0,W=0),ZPlane=(X=0,Y=0,Z=1,W=0),WPlane=(X=0,Y=0,Z=0,W=1))

	// Events
	Begin Object Class=UIEvent_Initialized Name=WidgetInitializedEvent
		OutputLinks(0)=(LinkDesc="Output")
		ObjClassVersion=4
	End Object

	Begin Object Class=UIComp_Event Name=WidgetEventComponent
		DefaultEvents.Add((EventTemplate=WidgetInitializedEvent))
	End Object
	EventProvider=WidgetEventComponent
}
