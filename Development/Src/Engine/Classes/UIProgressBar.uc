/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */



class UIProgressBar extends UIObject
	native(UIPrivate)
	implements(UIDataStorePublisher);

/** Component for rendering the background image */
var(Image)	editinline	const	noclear	UIComp_DrawImage		BackgroundImageComponent;

/** Component for rendering the fill image */
var(Image)	editinline	const	noclear	UIComp_DrawImage		FillImageComponent;

/** Component for rendering the overlay image */
var(Image)	editinline	const	noclear	UIComp_DrawImage		OverlayImageComponent;

var protected{private}	deprecated	UITexture				Background;
var protected{private}	deprecated	UITexture				BarFill;
var protected{private}	deprecated	UITexture				Overlay;
var protected{private}	deprecated	TextureCoordinates		BackgroundCoordinates;
var protected{private}	deprecated	TextureCoordinates		BarFillCoordinates;
var protected{private}	deprecated	TextureCoordinates		OverlayCoordinates;
var protected{private}	deprecated	UIStyleReference		FillStyle;
var protected{private}	deprecated	UIStyleReference		OverlayStyle;

/**
 * specifies whether to draw the overlay texture or not
 */
var(Image)									bool					bDrawOverlay;

/**
 * The data source that this progressbar's value will be linked to.
 */
var(Data)	editconst private				UIDataStoreBinding		DataSource;

/**
 * The value and range parameters for this progressbar.
 */
var(ProgressBar)							UIRangeData				ProgressBarValue;

/**
 * Controls whether this progressbar is vertical or horizontal
 */
var(ProgressBar)							EUIOrientation			ProgressBarOrientation;

cpptext
{
	/* === UUIProgressBar interface === */
	/**
	 * Changes the background image for this progressbar, creating the wrapper UITexture if necessary.
	 *
	 * @param	NewBarImage		the new surface to use for the progressbar's background image
	 */
	void SetBackgroundImage( class USurface* NewBarImage );

	/**
	 * Changes the fill image for this progressbar, creating the wrapper UITexture if necessary.
	 *
	 * @param	NewFillImage		the new surface to use for the progressbar's marker
	 */
	void SetFillImage( class USurface* NewFillImage );

	/**
	 * Changes the overlay image for this progressbar, creating the wrapper UITexture if necessary.
	 *
	 * @param	NewOverlayImage		the new surface to use for the progressbar's overlay image
	 */
	void SetOverlayImage( class USurface* NewOverlayImage );

	/**
	 * Returns the pixel extent of the progressbar fill based on the current progressbar value
	 */
	FLOAT GetBarFillExtent();

	/* === UIObject interface === */
	/**
	 * Called whenever the value of the progressbar is modified.  Activates the ProgressBarValueChanged kismet event and calls the OnValueChanged
	 * delegate.
	 *
	 * @param	PlayerIndex		the index of the player that generated the call to this method; used as the PlayerIndex when activating
	 *							UIEvents; if not specified, the value of GetBestPlayerIndex() is used instead.
	 * @param	NotifyFlags		optional parameter for individual widgets to use for passing additional information about the notification.
	 */
	virtual void NotifyValueChanged( INT PlayerIndex=INDEX_NONE, INT NotifyFlags=0 );

	/**
	 * Provides a way for widgets to fill their style subscribers array prior to performing any other initialization tasks.
	 *
	 * This version adds the image components (if non-NULL) to the StyleSubscribers array.
	 */
	virtual void InitializeStyleSubscribers();

	/* === UUIScreenObject interface === */
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
	 * Render this progressbar.
	 *
	 * @param	Canvas	the canvas to use for rendering this widget
	 */
	virtual void Render_Widget( FCanvas* Canvas );

	/* === UObject interface === */
	/**
	 * Called when a property value from a member struct or array has been changed in the editor, but before the value has actually been modified.
	 */
	virtual void PreEditChange( FEditPropertyChain& PropertyThatChanged );

	/**
	 * Called when a property value from a member struct or array has been changed in the editor.
	 */
	virtual void PostEditChange( FEditPropertyChain& PropertyThatChanged );

	/**
	 * Called after this object has been completely de-serialized.  This version migrates values for the deprecated image values
	 * over to the corresponding components.
	 */
	virtual void PostLoad();
}

/* === Natives === */


/* == Kismet action handlers == */
protected final function OnSetProgressBarValue( UIAction_SetProgressBarValue Action )
{
	if ( Action != None )
	{
		SetValue(Action.NewValue, Action.bPercentageValue);
	}
}

protected final function OnGetProgressBarValue( UIAction_GetProgressBarValue Action )
{
	if ( Action != None )
	{
		Action.Value = GetValue(Action.bPercentageValue);
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
native final function ClearBoundDataStores();

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
native final virtual function bool SaveSubscriberValue( out array<UIDataStore> out_BoundDataStores, optional int BindingIndex=INDEX_NONE );

/**
 * Change the value of this progressbar at runtime.
 *
 * @param	NewValue			the new value for the progressbar.
 * @param	bPercentageValue	TRUE indicates that the new value is formatted as a percentage of the total range of this progressbar.
 *
 * @return	TRUE if the progressbar's value was changed
 */
native final function bool SetValue( coerce float NewValue, optional bool bPercentageValue );

/**
 * Gets the current value of this progressbar
 *
 * @param	bPercentageValue	TRUE to format the result as a percentage of the total range of this progressbar.
 */
native final function float GetValue( optional bool bPercentageValue ) const;

/* === Unrealscript === */
/**
 * Changes the background image for this progressbar, creating the wrapper UITexture if necessary.
 *
 * @param	NewBarImage		the new surface to use for the progressbar's background image
 */
final function SetBackgroundImage( Surface NewImage )
{
	if ( BackgroundImageComponent != None )
	{
		BackgroundImageComponent.SetImage(NewImage);
	}
}

/**
 * Changes the fill image for this progressbar, creating the wrapper UITexture if necessary.
 *
 * @param	NewImage		the new surface to use for progressbar's marker
 */
final function SetFillImage( Surface NewImage )
{
	if ( FillImageComponent != None )
	{
		FillImageComponent.SetImage(NewImage);
	}
}

/**
 * Changes the overlay image for this progressbar, creating the wrapper UITexture if necessary.
 *
 * @param	NewOverlayImage		the new surface to use for the progressbar's overlay image
 */
final function SetOverlayImage( Surface NewImage )
{
	if ( OverlayImageComponent != None )
	{
		OverlayImageComponent.SetImage(NewImage);
	}
}

DefaultProperties
{
	Position=(Value[EUIWidgetFace.UIFACE_Bottom]=32,ScaleType[EUIWidgetFace.UIFACE_Bottom]=EVALPOS_PixelOwner)
	DataSource=(RequiredFieldType=DATATYPE_RangeProperty)

	bDrawOverlay=false

	ProgressBarValue=(MinValue=0.f,MaxValue=100.f,NudgeValue=1.f,CurrentValue=33.f)
	ProgressBarOrientation=UIORIENT_Horizontal

	PrimaryStyle=(DefaultStyleTag="DefaultSliderStyle",RequiredStyleClass=class'Engine.UIStyle_Image')
	FillStyle=(DefaultStyleTag="DefaultSliderBarStyle",RequiredStyleClass=class'UIStyle_Image')
	OverlayStyle=(DefaultStyleTag="DefaultImageStyle",RequiredStyleClass=class'UIStyle_Image')
	bSupportsPrimaryStyle=false

	Begin Object Class=UIComp_DrawImage Name=ProgressBarBackgroundImageTemplate
		ImageStyle=(DefaultStyleTag="DefaultSliderStyle",RequiredStyleClass=class'Engine.UIStyle_Image')
		StyleResolverTag="Background Image Style"
	End Object
	BackgroundImageComponent=ProgressBarBackgroundImageTemplate

	Begin Object Class=UIComp_DrawImage Name=ProgressBarFillImageTemplate
		ImageStyle=(DefaultStyleTag="DefaultSliderBarStyle",RequiredStyleClass=class'UIStyle_Image')
		StyleResolverTag="Fill Image Style"
	End Object
	FillImageComponent=ProgressBarFillImageTemplate

	Begin Object Class=UIComp_DrawImage Name=ProgressBarOverlayImageTemplate
		ImageStyle=(DefaultStyleTag="DefaultImageStyle",RequiredStyleClass=class'UIStyle_Image')
		StyleResolverTag="Overlay Image Style"
	End Object
	OverlayImageComponent=ProgressBarOverlayImageTemplate

	// States
	DefaultStates.Add(class'Engine.UIState_Focused')
	DefaultStates.Add(class'Engine.UIState_Active')
	DefaultStates.Add(class'Engine.UIState_Pressed')
}
