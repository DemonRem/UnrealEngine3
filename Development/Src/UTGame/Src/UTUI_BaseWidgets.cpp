//=============================================================================
// Copyright 1998-2007 Epic Games - All Rights Reserved.
// Confidential.
//
// This holds all of the base UI Objects
//=============================================================================
#include "UTGame.h"
#include "UTGameUIClasses.h"
#include "EngineUISequenceClasses.h"

IMPLEMENT_CLASS(UUIComp_DrawTeamColoredImage);
IMPLEMENT_CLASS(UUIComp_UTGlowString);
IMPLEMENT_CLASS(UUTUI_Widget);
IMPLEMENT_CLASS(UUTUIComboBox);
IMPLEMENT_CLASS(UUTUIList);
IMPLEMENT_CLASS(UUTUINumericEditBox);
IMPLEMENT_CLASS(UUTUIMenuList);
IMPLEMENT_CLASS(UUTUIButtonBar);
IMPLEMENT_CLASS(UUTUIButtonBarButton);
IMPLEMENT_CLASS(UUIComp_UTUIMenuListPresenter);
IMPLEMENT_CLASS(UUTDrawPanel);
IMPLEMENT_CLASS(UUTTabPage);
IMPLEMENT_CLASS(UUTUITabControl);
IMPLEMENT_CLASS(UUTSimpleList);

/*=========================================================================================
  UTUI_Widget - UI_Widgets are collections of other widgets that share game logic.
  ========================================================================================= */

/** 
 * Cache a reference to the scene owner
 * @see UUIObject.Intiailize
 */

void UUTUI_Widget::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner )
{
	UTSceneOwner = Cast<UUTUIScene>(inOwnerScene);	
	UUTUIScene::AutoPlaceChildren(this);

	Super::Initialize(inOwnerScene, inOwner);
}

/** 
 * !!! WARNING !!! This function does not check the destination and assumes it is valid.
 *
 * LookupProperty - Finds a property of a source actor and returns it's value.
 *
 * @param		SourceActor			The actor to search
 * @param		SourceProperty		The property to look up
 * @out param 	DestPtr				A Point to the storgage of the value
 *
 * @Returns true if the look up succeeded
 */

UBOOL UUTUI_Widget::LookupProperty(AActor *SourceActor, FName SourceProperty, BYTE* DestPtr)
{
	if ( SourceActor && SourceProperty != NAME_None )
	{
		UProperty* Prop = FindField<UProperty>( SourceActor->GetClass(), SourceProperty);
		if ( Prop )
		{
			BYTE* PropLoc = (BYTE*) SourceActor + Prop->Offset;
			if ( Cast<UIntProperty>(Prop) )
			{
				UIntProperty* IntProp = Cast<UIntProperty>(Prop);
				IntProp->CopySingleValue( (INT*)(DestPtr), PropLoc);
				return true;
			}
			else if ( Cast<UBoolProperty>(Prop) )
			{
				UBoolProperty* BoolProp = Cast<UBoolProperty>(Prop);
				BoolProp->CopySingleValue( (UBOOL*)(DestPtr), PropLoc);
				return true;
			}
			else if (Cast<UFloatProperty>(Prop) )
			{
				UFloatProperty* FloatProp = Cast<UFloatProperty>(Prop);
				FloatProp->CopySingleValue( (FLOAT*)(DestPtr), PropLoc);
				return true;
			}
			else if (Cast<UByteProperty>(Prop) )
			{
				UByteProperty* ByteProp = Cast<UByteProperty>(Prop);
				ByteProp->CopySingleValue( (BYTE*)(DestPtr), PropLoc);
				return true;
			}
			else
			{
				debugf(TEXT("Unhandled Property Type (%s)"),*Prop->GetName());
				return false;
			}
		}
	}
	debugf(TEXT("Illegal Proptery Operation (%s) %s"),*GetName(), *SourceProperty.ToString());
	return false;
}		

/** 
 * If bShowBounds is true, this control will always render it's bounds 
 *
 * @Param	Canvas		The drawing surface to use
 */

void UUTUI_Widget::Render_Widget(FCanvas* Canvas)
{
	if ( bShowBounds ) 
	{
		// Render the bounds 
		FLOAT X  = RenderBounds[UIFACE_Left];
		FLOAT Y  = RenderBounds[UIFACE_Top];
		FLOAT tW = RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left];
		FLOAT tH = RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top];

		DrawTile(Canvas, X,Y, tW, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,FLinearColor(1.0,1.0,1.0,1.0));
		DrawTile(Canvas, X,Y, 1.0f, tH, 0.0f, 0.0f, 1.0f, 1.0f,FLinearColor(1.0,1.0,1.0,1.0));

		DrawTile(Canvas, X,Y+tH, tW, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,FLinearColor(1.0,1.0,1.0,1.0));
		DrawTile(Canvas, X+tW,Y, 1.0f, tH, 0.0f, 0.0f, 1.0f, 1.0f,FLinearColor(1.0,1.0,1.0,1.0));
	}
}

/**
 * By default, Widgets do not propagate position change refresh events to their children.  Since UTUI_Widgets
 * are meant by nature to have children, we pass this event along.
 */
void UUTUI_Widget::RefreshPosition()
{
	Super::RefreshPosition();
	
	for ( INT ChildIdx=0 ; ChildIdx < Children.Num(); ChildIdx++ )
	{
		Children(ChildIdx)->RefreshPosition();
	}
}

/*=========================================================================================
	UTUINumericEditBox - UI_Widgets are collections of other widgets that share game logic.
========================================================================================= */

void UUTUINumericEditBox::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner )
{
	Super::Initialize(inOwnerScene, inOwner);

	// Replace styles in child widgets.
	IncrementButton->BackgroundImageComponent->ImageStyle.DefaultStyleTag = TEXT("SpinnerIncrementButtonBackground");
	IncrementButton->BackgroundImageComponent->ImageStyle.RequiredStyleClass = UUIStyle_Image::StaticClass();
	IncrementButton->BackgroundImageComponent->StyleResolverTag = TEXT("Increment Image Style");

	DecrementButton->BackgroundImageComponent->ImageStyle.DefaultStyleTag = TEXT("SpinnerDecrementButtonBackground");
	DecrementButton->BackgroundImageComponent->ImageStyle.RequiredStyleClass = UUIStyle_Image::StaticClass();
	DecrementButton->BackgroundImageComponent->StyleResolverTag = TEXT("Decrement Image Style");

}

/*=========================================================================================
  UUIComp_UTGlowString is a special UIComp_DrawString that will render the string twice using
  two styles
  ========================================================================================= */
/**
 * Returns the combo style data being used by this string rendering component.  If the component's StringStyle is not set, the style data
 * will be pulled from the owning widget's PrimaryStyle, if possible.
 *
 * This version resolves the additional style reference property declared by the UTGlowString component.
 *
 * @param	DesiredMenuState	the menu state for the style data to retrieve; if not specified, uses the owning widget's current menu state.
 * @param	SourceSkin			the skin to use for resolving this component's combo style; only relevant when the component's combo style is invalid
 *								(or if TRUE is passed for bClearExistingValue). If the combo style is invalid and a value is not specified, returned value
 *								will be NULL.
 * @param	bClearExistingValue	used to force the component's combo style to be re-resolved from the specified skin; if TRUE, you must supply a valid value for
 *								SourceSkin.
 *
 * @return	the combo style data used to render this component's string for the specified menu state.
 */
UUIStyle_Combo* UUIComp_UTGlowString::GetAppliedStringStyle( UUIState* DesiredMenuState/*=NULL*/, UUISkin* SourceSkin/*=NULL*/, UBOOL bClearExistingValue/*=FALSE*/ )
{
	UUIObject* OwnerWidget = GetOuterUUIObject();

	// if no menu state was specified, use the owning widget's current menu state
	if ( DesiredMenuState == NULL )
	{
		DesiredMenuState = OwnerWidget->GetCurrentState();
	}
	check(DesiredMenuState);

	// only attempt to resolve if the GlowStyle has a valid style reference
	if ( GlowStyle.GetResolvedStyle() != NULL
	||	(SourceSkin != NULL && GlowStyle.AssignedStyleID.IsValid()) )
	{
		const UBOOL bIsStyleManaged = OwnerWidget->IsPrivateBehaviorSet(UCONST_PRIVATE_ManagedStyle);
		if ( bClearExistingValue && !bIsStyleManaged )
		{
			GlowStyle.InvalidateResolvedStyle();
		}

		// get the UIStyle corresponding to the UIStyleReference that we're going to pull from
		// GetResolvedStyle() is guaranteed to return a valid style if a valid UISkin is passed in, unless the style reference is completely invalid

		// If the owner widget's style is being managed by someone else, then StyleToResolve's AssignedStyleID will always be zero, so never pass in
		// a valid skin reference.  This style should have already been resolved by whoever is managing the style, and if the resolved style doesn't live
		// in the currently active skin (for example, it's being inherited from a base skin package), GetResolvedStyle will clear the reference and reset
		// the ResolvedStyle back to the default style for this style reference.
		UUIStyle* ResolvedStyle = GlowStyle.GetResolvedStyle(bIsStyleManaged ? NULL : SourceSkin);
		checkf(ResolvedStyle, TEXT("Unable to resolve style reference (%s)' for '%s.%s'"), *GlowStyle.DefaultStyleTag.ToString(), *OwnerWidget->GetWidgetPathName(), *GetName());
	}

	// always return the draw string component's primary string style
	return Super::GetAppliedStringStyle(DesiredMenuState, SourceSkin, bClearExistingValue);
}

/**
 * Resolves the glow style for this string rendering component.
 *
 * @param	ActiveSkin			the skin the use for resolving the style reference.
 * @param	bClearExistingValue	if TRUE, style references will be invalidated first.
 * @param	CurrentMenuState	the menu state to use for resolving the style data; if not specified, uses the current
 *								menu state of the owning widget.
 * @param	StyleProperty		if specified, only the style reference corresponding to the specified property
 *								will be resolved; otherwise, all style references will be resolved.
 */
UBOOL UUIComp_UTGlowString::NotifyResolveStyle(UUISkin* ActiveSkin,UBOOL bClearExistingValue,UUIState* CurrentMenuState/*=NULL*/,const FName StylePropertyName/*=NAME_None*/)
{
	UBOOL bResult = FALSE;

	// the UIComp_DrawString will call GetAppliedStringStyle (which will cause the glow style to be resolved) if the StylePropertyName is None or StringStyle
	// so we only need to do something if the StylePropertyName is GlowStyle since our parent class won't handle that.
	if ( StylePropertyName == TEXT("GlowStyle") )
	{
		GetAppliedStringStyle(CurrentMenuState, ActiveSkin, bClearExistingValue);
		bResult = GlowStyle.GetResolvedStyle() != NULL;

		// don't think we ever want to apply the GlowStyle to the string from NotifyResolveStyle - that will happen in Render_String
#if 0
		if ( ValueString != NULL )
		{
			// apply this component's per-instance style settings
			FUICombinedStyleData FinalStyleData(ComboStyleData);
			CustomizeAppliedStyle(FinalStyleData);

			// apply the style data to the string
			bResult = ValueString->SetStringStyle(FinalStyleData);
			const UBOOL bNeedsReformat = bResult || ValueString->StringClipMode != WrapMode || ValueString->StringClipAlignment != ClipAlignment;
			if ( bNeedsReformat )
			{
				ReapplyFormatting();
			}
		}
#endif
	}

	return Super::NotifyResolveStyle(ActiveSkin,bClearExistingValue,CurrentMenuState,StylePropertyName) || bResult;
}

#if 0
/** 
 * Resolve a style Reference to a given style. See UUIComp_DrawString for more inf.
 *
 * @Params	StyleToResolve		The style to resolve
 */
UUIStyle_Combo* UUIComp_UTGlowString::InternalResolve_StringStyle(FUIStyleReference StyleToResolve)
{
	UUIStyle_Combo* Result = NULL;
	UUIObject* OwnerWidget = GetOuterUUIObject();

	UUIStyle* ResolvedStyle = StyleToResolve.GetResolvedStyle( OwnerWidget->GetActiveSkin() );
	if ( ResolvedStyle )
	{
		// retrieve the style data for this menu state
		UUIStyle_Data* StyleData = ResolvedStyle->GetStyleForState( OwnerWidget->GetCurrentState() );

		if ( StyleData )
		{
			Result = Cast<UUIStyle_Combo>(StyleData);
		}
	}

	return Result;
}
#endif

/**
 * We override InternalRender_String so that we can render twice.  Once with the glow style, once with the normal
 * style
 *
 * @param	Canvas		the canvas to use for rendering this string
 * @param	Parameters	The render parameters for this string
 */

void UUIComp_UTGlowString::InternalRender_String( FCanvas* Canvas, FRenderParameters& Parameters )
{
	checkSlow(ValueString);

	if ( ValueString != NULL )
	{
#if 0
		UUIStyle_Combo* DrawStyle = InternalResolve_StringStyle(GlowStyle);

		if ( DrawStyle )
		{
			ValueString->SetStringStyle(DrawStyle);
/*
			if (GIsGame)
			{
				RefreshAppliedStyleData();
			}
*/
			ValueString->Render_String(Canvas, Parameters);
		}

		if ( GIsGame )
		{
			DrawStyle = InternalResolve_StringStyle(StringStyle);
			if ( DrawStyle )
			{
				/*
				ValueString->SetStringStyle(DrawStyle);
				RefreshAppliedStyleData();
				*/

				ValueString->Render_String(Canvas, Parameters);
			}
		}
#else
		UUIObject* OwnerWidget = GetOuterUUIObject();
		UUIState* CurrentMenuState = OwnerWidget->GetCurrentState(OwnerWidget->GetBestPlayerIndex());
		check(CurrentMenuState);

		UUIStyle_Combo* StateStyleData = NULL;

		UUIStyle* DrawStyle = GlowStyle.GetResolvedStyle();
		if ( DrawStyle )
		{
			StateStyleData = Cast<UUIStyle_Combo>(DrawStyle->GetStyleForState(CurrentMenuState));
			if ( StateStyleData != NULL )
			{
				FUICombinedStyleData FinalStyleData(StateStyleData);
				CustomizeAppliedStyle(FinalStyleData);

				if ( ValueString->SetStringStyle(FinalStyleData) )
				{
					ValueString->Render_String(Canvas, Parameters);
				}
			}
		}

		if ( GIsGame )
		{
			DrawStyle = StringStyle.GetResolvedStyle();
			if ( DrawStyle )
			{
				StateStyleData = Cast<UUIStyle_Combo>(DrawStyle->GetStyleForState(CurrentMenuState));
				if ( StateStyleData != NULL )
				{
					FUICombinedStyleData FinalStyleData(StateStyleData);
					CustomizeAppliedStyle(FinalStyleData);

					if ( ValueString->SetStringStyle(FinalStyleData) )
					{
						ValueString->Render_String(Canvas, Parameters);
					}
				}
			}
		}
#endif
	}
}

/*=========================================================================================
  UIComp_DrawTeamColoredImage - Automatically teamcolor an image
  ========================================================================================= */
/**
 * We override RenderComponent to make sure we can adjust the color
 *
 * @param	Canvas		the canvas to render the image to
 * @param	Parameters	the bounds for the region that this texture can render to.
 */
void UUIComp_DrawTeamColoredImage::RenderComponent( FCanvas* Canvas, FRenderParameters Parameters )
{
	if ( ImageRef != NULL && Canvas != NULL )
	{
		// Last Team Color is always the Default
		FLinearColor TeamColor = TeamColors( TeamColors.Num() -1 );

		if ( GIsGame )
		{
			AWorldInfo* WorldInfo = GWorld->GetWorldInfo();
			if (WorldInfo->GRI != NULL && WorldInfo->GRI->GameClass != NULL)
			{
				AGameInfo* GI = Cast<AGameInfo>(GWorld->GetWorldInfo()->GRI->GameClass->GetDefaultActor());
				if (GI != NULL && GI->bTeamGame)
				{
					UUIObject* OwnerWidget = GetOuterUUIObject();
					AUTPlayerController* UTPC = Cast<AUTPlayerController>( OwnerWidget->GetPlayerOwner()->Actor );
					if ( UTPC && UTPC->PlayerReplicationInfo && UTPC->PlayerReplicationInfo->Team )
					{
						INT TeamIndex = UTPC->PlayerReplicationInfo->Team->TeamIndex;
						if ( TeamIndex <= TeamColors.Num() )
						{
							TeamColor = TeamColors( TeamIndex );

							if (UTPC->bPulseTeamColor)
							{
								FLOAT Perc = Abs( appSin( WorldInfo->TimeSeconds * 5 ) );
								TeamColor.R =  TeamColor.R * (2.0 * Perc) + 0.5;
								TeamColor.G =  TeamColor.G * (2.0 * Perc) + 0.5;
								TeamColor.B =  TeamColor.B * (2.0 * Perc) + 0.5;
							}

						}
					}
				}
			}
		}
		else
		{
			if ( EditorTeamIndex < TeamColors.Num() )
			{
				TeamColor = TeamColors( EditorTeamIndex );
			}
		}

		StyleCustomization.SetCustomDrawColor(TeamColor);
		if ( HasValidStyleReference(NULL) )
		{
			RefreshAppliedStyleData();
		}
	}

	Super::RenderComponent(Canvas, Parameters);
}

/*=========================================================================================
	UUTUIMenuList - UT UI FrontEnd Menu List
========================================================================================= */

namespace
{
	const INT MenuListItemPadding = 0;
}

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
void UUTUIMenuList::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner )
{
	// Initialize Widget
	Super::Initialize(inOwnerScene, inOwner);


}

/**
 * Tick callback for this widget.
 */
void UUTUIMenuList::Tick_Widget(FLOAT DeltaTime)
{
	 if(bIsRotating)
	 {
		 // Update component
		 UUIComp_UTUIMenuListPresenter* ListComp = Cast<UUIComp_UTUIMenuListPresenter>(CellDataComponent);

		 if(ListComp)
		 {
			 ListComp->UpdatePrefabPosition();
		 }
	 }
}


/**
 * Callback that happens the first time the scene is rendered, any widget positioning initialization should be done here.
 *
 * By default this function recursively calls itself on all of its children.
 */
void UUTUIMenuList::PreRenderCallback()
{
	// Initialize Component Prefabs
	UUIComp_UTUIMenuListPresenter* ListComp = Cast<UUIComp_UTUIMenuListPresenter>(CellDataComponent);

	if(ListComp)
	{
		DataProvider = ResolveListElementProvider();
		ListComp->InitializePrefabs();
		ListComp->UpdatePrefabPosition();
		ListComp->UpdatePrefabMarkup();
	}
}

/**
 * Evalutes the Position value for the specified face into an actual pixel value.  Should only be
 * called from UIScene::ResolvePositions.  Any special-case positioning should be done in this function.
 *
 * @param	Face	the face that should be resolved
 */
void UUTUIMenuList::ResolveFacePosition( EUIWidgetFace Face )
{
	Super::ResolveFacePosition(Face);
}

/**
 * Called when the list's index has changed.
 *
 * @param	PreviousIndex	the list's Index before it was changed
 * @param	PlayerIndex		the index of the player associated with this index change.
 */
void UUTUIMenuList::NotifyIndexChanged(INT PreviousIndex, INT PlayerIndex )
{
	Super::NotifyIndexChanged(PreviousIndex, PlayerIndex);

	UUIComp_UTUIMenuListPresenter* ListComp = Cast<UUIComp_UTUIMenuListPresenter>(CellDataComponent);

	if(ListComp)
	{
		if(ListComp->InstancedPrefabs.Num() > 0 && ListComp->InstancedPrefabs.Num() != GetItemCount())
		{
			ListComp->InitializePrefabs();
			ListComp->UpdatePrefabMarkup();
			ListComp->UpdatePrefabPosition();
		}
	}

	// Start the rotation animation for the widget.
	bIsRotating = TRUE;
	StartRotationTime = GWorld->GetRealTimeSeconds();
	PreviousItem = PreviousIndex;
}

/**
 * Gets the cell field value for a specified list and list index.
 *
 * @param InList		List to get the cell field value for.
 * @param InCellTag		Tag to get the value for.
 * @param InListIndex	Index to get the value for.
 * @param OutValue		Storage variable for the final value.
 */
UBOOL UUTUIMenuList::GetCellFieldValue(UUIList* InList, FName InCellTag, INT InListIndex, FUIProviderFieldValue &OutValue)
{
	UBOOL bResult = FALSE;

	if (InList != NULL && InList->DataProvider)
	{
	TScriptInterface<IUIListElementCellProvider> CellProvider = InList->DataProvider->GetElementCellValueProvider(InList->DataSource.DataStoreField, InListIndex);
	if ( CellProvider )
	{
		FUIProviderFieldValue CellValue(EC_EventParm);
		if ( CellProvider->GetCellFieldValue(InCellTag, InListIndex, CellValue) == TRUE )
		{
			OutValue = CellValue;
			bResult = TRUE;
		}
	}
	}

	return bResult;
}

/** returns the first list index the has the specified value for the specified cell, or INDEX_NONE if it couldn't be found */
INT UUTUIMenuList::FindCellFieldString(UUIList* InList, FName InCellTag, const FString& FindValue, UBOOL bCaseSensitive)
{
	INT Result = INDEX_NONE;

	if (InList != NULL && InList->DataProvider)
	{
		FUIProviderFieldValue CellValue(EC_EventParm);
		
		for (INT i = 0; i < InList->Items.Num(); i++)
		{
			TScriptInterface<IUIListElementCellProvider> CellProvider = InList->DataProvider->GetElementCellValueProvider(InList->DataSource.DataStoreField, i);
			if ( CellProvider && CellProvider->GetCellFieldValue(InCellTag, i, CellValue) &&
				(bCaseSensitive ? CellValue.StringValue == FindValue : appStricmp(*CellValue.StringValue, *FindValue) == 0) )
			{
				Result = i;
				break;
			}
		}
	}

	return Result;
}

/**
 * Returns the width of the specified row.
 *
 * @param	RowIndex		the index for the row to get the width for.  If the index is invalid, the list's configured RowHeight is returned instead.
 */
FLOAT UUTUIMenuList::GetRowHeight( INT RowIndex/*=INDEX_NONE*/ ) const
{
	FLOAT Result = 0.0f;
	UUIComp_UTUIMenuListPresenter* ListComp = Cast<UUIComp_UTUIMenuListPresenter>(CellDataComponent);

	if(ListComp && ListComp->InstancedPrefabs.IsValidIndex(RowIndex) && ListComp->InstancedPrefabs(RowIndex).PrefabInstance)
	{
		UUIPrefabInstance* PrefabIsnt = ListComp->InstancedPrefabs(RowIndex).PrefabInstance;

		Result = PrefabIsnt->Position.GetBoundsExtent(PrefabIsnt, UIORIENT_Vertical, EVALPOS_PixelViewport) + MenuListItemPadding;
	}
	else
	{
		Result = RowHeight.GetValue(this);
	}

	return Result;
}

/**
 * Resolves this subscriber's data store binding and updates the subscriber with the current value from the data store.
 *
 * @return	TRUE if this subscriber successfully resolved and applied the updated value.
 */
UBOOL UUTUIMenuList::RefreshSubscriberValue( INT BindingIndex )
{
	UBOOL bResult = Super::RefreshSubscriberValue(BindingIndex);

	if( bResult && BindingIndex < UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		UUIComp_UTUIMenuListPresenter* ListComp = Cast<UUIComp_UTUIMenuListPresenter>(CellDataComponent);

		if(ListComp)
		{
			ListComp->InitializePrefabs();
			ListComp->UpdatePrefabMarkup();
			ListComp->UpdatePrefabPosition();
		}
	}
	
	return bResult;
}

/*=========================================================================================
	UUIComp_UTUIMenuListPresenter - List presenter for the UTUIMenuList
========================================================================================= */

/**
 * Initializes the component's prefabs.
 */
void UUIComp_UTUIMenuListPresenter::InitializePrefabs()
{
	UUIList* Owner = GetOuterUUIList();
	
	if(Owner)
	{	
		FName SelectedPrefabName = TEXT("SelectedItem_Prefab");

		// Remove all children
		Owner->RemoveChildren(Owner->Children);
		InstancedPrefabs.Empty();

		// @todo: This is here to make old instances of this widget work, will remove before ship.
		if(NormalItemPrefab==NULL)
		{
			NormalItemPrefab=SelectedItemPrefab;
		}

		// @todo: For now, only generate prefabs ingame, change this later.
		if(GIsGame && Owner->DataProvider)
		{
			if(NormalItemPrefab)
			{
				TArray<INT> Elements;
				Owner->DataProvider->GetListElements(Owner->DataSource.DataStoreField, Elements);
				const INT NumItems = Elements.Num();
				const FLOAT AngleDelta = 360.0f / NumItems;
	
				for(INT ItemIdx=0; ItemIdx<NumItems; ItemIdx++)
				{
					FName ItemName = *FString::Printf(TEXT("Item_%i"), ItemIdx);
					UUIPrefabInstance* NewPrefabInstance = NormalItemPrefab->InstancePrefab(Owner, ItemName);

					if(NewPrefabInstance!=NULL)
					{
						// Set some private behavior for the prefab.
						NewPrefabInstance->SetPrivateBehavior(UCONST_PRIVATE_NotEditorSelectable, TRUE);
						NewPrefabInstance->SetPrivateBehavior(UCONST_PRIVATE_TreeHiddenRecursive, TRUE);
						
						// Add the prefab to the list.
						Owner->InsertChild(NewPrefabInstance);

						// Make the entire prefab percentage owner
						NewPrefabInstance->Position.ChangeScaleType(Owner, UIFACE_Top, EVALPOS_PercentageOwner);
						NewPrefabInstance->Position.ChangeScaleType(Owner, UIFACE_Bottom, EVALPOS_PercentageOwner);
						NewPrefabInstance->Position.ChangeScaleType(Owner, UIFACE_Left, EVALPOS_PercentageOwner);
						NewPrefabInstance->Position.ChangeScaleType(Owner, UIFACE_Right, EVALPOS_PercentageOwner);



						// Store info about the newly generated prefab.
						INT NewIdx = InstancedPrefabs.AddZeroed(1);
						FInstancedPrefabInfo &Info = InstancedPrefabs(NewIdx);

						Info.PrefabInstance = NewPrefabInstance;

						// Resolve references to all of the objects we will be touching within the prefab.
						Info.ResolvedObjects.AddZeroed(PrefabMarkupReplaceList.Num());
						for(INT ChildIdx=0; ChildIdx<PrefabMarkupReplaceList.Num(); ChildIdx++)
						{
							FName WidgetTag = PrefabMarkupReplaceList(ChildIdx).WidgetTag;
							UUIObject* ResolvedObject = NewPrefabInstance->FindChild(WidgetTag, TRUE);

							Info.ResolvedObjects(ChildIdx) = ResolvedObject;
						}
					}	
				}
			}
		}
	}
}

/**
 * Renders the elements in this list.
 *
 * @param	RI					the render interface to use for rendering
 */
void UUIComp_UTUIMenuListPresenter::Render_List( FCanvas* Canvas )
{
	
}

/**
 * Renders the list element specified.
 *
 * @param	Canvas			the canvas to use for rendering
 * @param	ElementIndex	the index for the list element to render
 * @param	Parameters		Used for various purposes:
 *							DrawX:		[in]	specifies the pixel location of the start of the horizontal bounding region that should be used for
 *												rendering this element
 *										[out]	unused
 *							DrawY:		[in]	specifies the pixel Y location of the bounding region that should be used for rendering this list element.
 *										[out]	Will be set to the Y position of the rendering "pen" after rendering this element.  This is the Y position for rendering
 *												the next element should be rendered
 *							DrawXL:		[in]	specifies the pixel location of the end of the horizontal bounding region that should be used for rendering this element.
 *										[out]	unused
 *							DrawYL:		[in]	specifies the height of the bounding region, in pixels.  If this value is not large enough to render the specified element,
 *												the element will not be rendered.
 *										[out]	Will be reduced by the height of the element that was rendered. Thus represents the "remaining" height available for rendering.
 *							DrawFont:	[in]	specifies the font to use for retrieving the size of the characters in the string
 *							Scale:		[in]	specifies the amount of scaling to apply when rendering the element
 */
void UUIComp_UTUIMenuListPresenter::Render_ListElement( FCanvas* Canvas, INT ElementIndex, FRenderParameters& Parameters )
{
	
}

/**
 * Called when a member property value has been changed in the editor.
 */
void UUIComp_UTUIMenuListPresenter::PostEditChange( UProperty* PropertyThatChanged )
{
	// Reinitialize prefabs if the selected item prefab changed.
	if(PropertyThatChanged->GetName()==TEXT("SelectedItemPrefab"))
	{
		InitializePrefabs();
	}
}

/**
 * Updates the prefab widgets we are dynamically changing the markup of to use a new list row for their datasource.
 *
 * @param NewIndex	New list row index to rebind the widgets with.
 */
void UUIComp_UTUIMenuListPresenter::UpdatePrefabMarkup()
{
	UUIList* Owner = GetOuterUUIList();

	if(Owner)
	{
		TArray<INT> CurrentListItems;
		Owner->DataProvider->GetListElements(Owner->DataSource.DataStoreField, CurrentListItems);

		for(INT PrefabIdx=0; PrefabIdx<InstancedPrefabs.Num(); PrefabIdx++)
		{
			FInstancedPrefabInfo &Info = InstancedPrefabs(PrefabIdx);
			UUIPrefabInstance* PrefabInstance = Info.PrefabInstance;

			for(INT ChildIdx=0; ChildIdx<PrefabMarkupReplaceList.Num(); ChildIdx++)
			{
				if(Info.ResolvedObjects.IsValidIndex(ChildIdx))
				{
					UUIObject* ResolvedObject = Info.ResolvedObjects(ChildIdx);
					if(ResolvedObject && CurrentListItems.IsValidIndex(PrefabIdx))
					{
						INT DataSourceIndex = CurrentListItems(PrefabIdx);

						// get the UIListElementCellProvider for the specified element
						TScriptInterface<IUIListElementCellProvider> CellProvider = Owner->DataProvider->GetElementCellValueProvider(Owner->DataSource.DataStoreField, DataSourceIndex);
						if ( CellProvider )
						{
							FName CellFieldName = PrefabMarkupReplaceList(ChildIdx).CellTag;
							FUIProviderFieldValue CellValue(EC_EventParm);
							if ( CellProvider->GetCellFieldValue(CellFieldName, DataSourceIndex, CellValue) == TRUE )
							{
								// if the cell provider is a UIDataProvider, generate the markup string required to access this property
								UUIDataProvider* CellProviderObject = Cast<UUIDataProvider>(Owner->DataProvider.GetObject());
								if ( CellProviderObject != NULL )
								{
									// get the data field path to the collection data field that the list is bound to...
									FString CellFieldMarkup;
									FString DataSourceMarkup = CellProviderObject->BuildDataFieldPath(*Owner->DataSource, Owner->DataSource.DataStoreField);
									if ( DataSourceMarkup.Len() != 0 )
									{
										// then append the list index and the cell name
										CellFieldMarkup = FString::Printf(TEXT("<%s;%i.%s>"), *DataSourceMarkup, DataSourceIndex, *CellFieldName.ToString());
									}
									else
									{
										// if the CellProviderObject doesn't support the data field that the list is bound to, the CellProviderObject is an internal provider of
										// the data store that the list is bound to.  allow the data provider to generate its own markup by passing in the CellFieldName
										CellFieldMarkup = CellProviderObject->GenerateDataMarkupString(*Owner->DataSource,CellFieldName);
									}

									IUIDataStoreSubscriber* Subscriber = static_cast<IUIDataStoreSubscriber*>(ResolvedObject->GetInterfaceAddress(IUIDataStoreSubscriber::UClassType::StaticClass()));
									if ( Subscriber != NULL )
									{
										Subscriber->SetDataStoreBinding(CellFieldMarkup);
									}
								}		
							}
						}
					}
				}
			}
		}
	}
}

/** Updates the position of the selected item prefab. */
void UUIComp_UTUIMenuListPresenter::UpdatePrefabPosition()
{
	UUTUIMenuList* Owner = Cast<UUTUIMenuList>(GetOuterUUIList());

	if(Owner)
	{
		const FLOAT RotationTime = 0.125f;
		const FLOAT ZRadius = 500.0f;
		const INT SelectedIndex = Owner->Index;
		
		FLOAT ItemY = 0.0f;

		for(INT PrefabIdx=0; PrefabIdx<InstancedPrefabs.Num(); PrefabIdx++)
		{
			FInstancedPrefabInfo &Info = InstancedPrefabs(PrefabIdx);
			UUIPrefabInstance* PrefabInstance = Info.PrefabInstance;

			FLOAT Distance = SelectedIndex - PrefabIdx;

			// If we are changing items, then smoothly move to the new selection item.
			if(Owner->bIsRotating)
			{
				FLOAT TimeElapsed = GWorld->GetRealTimeSeconds() - Owner->StartRotationTime;
				if(TimeElapsed > RotationTime)
				{
					Owner->bIsRotating = FALSE;
					TimeElapsed = RotationTime;
				}

				// Used to interpolate the scaling to create a smooth looking selection effect.
				const INT Diff = (SelectedIndex - Owner->PreviousItem);
				FLOAT Alpha = 1.0f - (TimeElapsed / RotationTime);

/*
				// Smooth out the interpolation
				if (Alpha < 0.5f) 
				{
					Alpha *= 2.0f;
					Alpha = appPow(Alpha,3.0f);
					Alpha /= 2.0f;
				}
				else
				{
					Alpha = (1.0f-Alpha);
					Alpha *= 2.0f;
					Alpha = appPow(Alpha,3.0f);
					Alpha /= 2.0f;
					Alpha = (1.0f - Alpha);
				}
*/

				Distance += -Diff*Alpha;
			}

			// Set the item's position
			const INT ItemRange = 2;
			Distance = Clamp<FLOAT>(Distance, -ItemRange, ItemRange);
			Distance /= ItemRange;
			Distance *= (PI / 2.0f);

			const FLOAT ScaleFactor = Max<FLOAT>(appCos(Distance),0.5f);
			const FLOAT ItemHeight = SelectedItemHeight*ScaleFactor;

			PrefabInstance->Opacity = Max<FLOAT>(appCos(Distance),0.5f);
			PrefabInstance->Opacity *= PrefabInstance->Opacity;

			PrefabInstance->SetPosition(ItemY, UIFACE_Top, EVALPOS_PixelOwner);
			PrefabInstance->SetPosition(PrefabInstance->GetPosition(UIFACE_Top, EVALPOS_PixelViewport) + ItemHeight, UIFACE_Bottom, EVALPOS_PixelViewport);
			
			PrefabInstance->SetPosition(0.0f, UIFACE_Left, EVALPOS_PercentageOwner);
			PrefabInstance->SetPosition(1.0f, UIFACE_Right, EVALPOS_PercentageOwner);

			ItemY += ItemHeight + MenuListItemPadding;
		}


		Owner->RequestSceneUpdate(FALSE,TRUE,FALSE,TRUE);
	}
}


/*=========================================================================================
	UUTUIEditBox - UT specific version of the editbox widget
========================================================================================= */
IMPLEMENT_CLASS(UUTUIEditBox);


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
void UUTUIEditBox::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner )
{
	Super::Initialize(inOwnerScene, inOwner);

	// Change the editbox background style from just being the default image style.
	BackgroundImageComponent->ImageStyle.DefaultStyleTag = TEXT("UTEditBoxBackground");
	RequestSceneUpdate(FALSE,FALSE,FALSE,TRUE);
}


/**
 * Resolves this subscriber's data store binding and updates the subscriber with the current value from the data store.
 *
 * @param	BindingIndex		indicates which data store binding should be modified.  Valid values and their meanings are:
 *									-1:	all data sources
 *									0:	list data source
 *									1:	caption data source
 *
 * @return	TRUE if this subscriber successfully resolved and applied the updated value.
 */
UBOOL UUTUIEditBox::RefreshSubscriberValue( INT BindingIndex/*=INDEX_NONE*/ )
{
	UBOOL bResult = FALSE;
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		bResult = ResolveDefaultDataBinding(BindingIndex);
	}
	else if ( StringRenderComponent != NULL && IsInitialized() )
	{
		FUIProviderFieldValue FieldValue(EC_EventParm);

		if(GetDataStoreFieldValue(DataSource.MarkupString, FieldValue))
		{
			StringRenderComponent->SetValue(FieldValue.StringValue);
			StringRenderComponent->ReapplyFormatting();
			bResult = TRUE;
		}
	}

	if(bResult==FALSE)
	{
		bResult = Super::RefreshSubscriberValue(BindingIndex);
	}

	return bResult;
}


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
UBOOL UUTUIEditBox::SaveSubscriberValue( TArray<UUIDataStore*>& out_BoundDataStores, INT BindingIndex/*=INDEX_NONE*/ )
{
	// Send the current value the datastore.
	FUIProviderFieldValue FieldValue(EC_EventParm);
	FieldValue.StringValue = StringRenderComponent->GetValue();

	GetBoundDataStores(out_BoundDataStores);
	return SetDataStoreFieldValue(DataSource.MarkupString, FieldValue);
}


/*=========================================================================================
	UUTUIComboBox - UT specific version of the combobox widget
========================================================================================= */

/**
 * Called whenever the selected item is modified.  Activates the SliderValueChanged kismet event and calls the OnValueChanged
 * delegate.
 *
 * @param	PlayerIndex		the index of the player that generated the call to this method; used as the PlayerIndex when activating
 *							UIEvents; if not specified, the value of GetBestPlayerIndex() is used instead.
 * @param	NotifyFlags		optional parameter for individual widgets to use for passing additional information about the notification.
 */
void UUTUIComboBox::NotifyValueChanged( INT PlayerIndex, INT NotifyFlags )
{
	Super::NotifyValueChanged(PlayerIndex, NotifyFlags);

	// Send the current value the datastore.
	FUIProviderFieldValue FieldValue(EC_EventParm);
	FieldValue.StringValue = ComboEditbox->GetValue();

	SetDataStoreFieldValue(ComboList->GetDataStoreBinding(), FieldValue);
}


/* === IUIDataStoreSubscriber interface === */
/**
 * Sets the data store binding for this object to the text specified.
 *
 * @param	MarkupText			a markup string which resolves to data exposed by a data store.  The expected format is:
 *								<DataStoreTag:DataFieldTag>
 * @param	BindingIndex		indicates which data store binding should be modified.  Valid values and their meanings are:
 *									0:	list data source
 *									1:	caption data source
 */
void UUTUIComboBox::SetDataStoreBinding( const FString& MarkupText, INT BindingIndex/*=INDEX_NONE*/ )
{
	Super::SetDataStoreBinding(MarkupText, BindingIndex);
}

/**
 * Retrieves the markup string corresponding to the data store that this object is bound to.
 *
 * @param	BindingIndex		indicates which data store binding should be modified.  Valid values and their meanings are:
 *									0:	list data source
 *									1:	caption data source
 *
 * @return	a datastore markup string which resolves to the datastore field that this object is bound to, in the format:
 *			<DataStoreTag:DataFieldTag>
 */
FString UUTUIComboBox::GetDataStoreBinding( INT BindingIndex/*=INDEX_NONE*/ ) const
{
	return Super::GetDataStoreBinding(BindingIndex);
}

/**
 * Resolves this subscriber's data store binding and updates the subscriber with the current value from the data store.
 *
 * @param	BindingIndex		indicates which data store binding should be modified.  Valid values and their meanings are:
 *									-1:	all data sources
 *									0:	list data source
 *									1:	caption data source
 *
 * @return	TRUE if this subscriber successfully resolved and applied the updated value.
 */
UBOOL UUTUIComboBox::RefreshSubscriberValue( INT BindingIndex/*=INDEX_NONE*/ )
{
	return Super::RefreshSubscriberValue(BindingIndex);
}


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
UBOOL UUTUIComboBox::SaveSubscriberValue( TArray<UUIDataStore*>& out_BoundDataStores, INT BindingIndex/*=INDEX_NONE*/ )
{
	// Send the current value the datastore.
	FUIProviderFieldValue FieldValue(EC_EventParm);
	FieldValue.StringValue = ComboEditbox->GetValue();

	GetBoundDataStores(out_BoundDataStores);
	return SetDataStoreFieldValue(ComboList->GetDataStoreBinding(), FieldValue);
}


/**
 * Retrieves the list of data stores bound by this subscriber.
 *
 * @param	out_BoundDataStores		receives the array of data stores that subscriber is bound to.
 */
void UUTUIComboBox::GetBoundDataStores( TArray<UUIDataStore*>& out_BoundDataStores )
{
	Super::GetBoundDataStores(out_BoundDataStores);
}

/**
 * Notifies this subscriber to unbind itself from all bound data stores
 */
void UUTUIComboBox::ClearBoundDataStores()
{
	Super::ClearBoundDataStores();
}


/** Initializes styles for child widgets. */
void UUTUIComboBox::SetupChildStyles()
{
	UGameUISceneClient* SceneClient = UUIRoot::GetSceneClient();

	if(SceneClient)
	{
		if(ComboButton)
		{
			ComboButton->SetDataStoreBinding(TEXT(""));
			UUIStyle* Style = SceneClient->ActiveSkin->FindStyle(ToggleButtonStyleName);
			ComboButton->BackgroundImageComponent->ImageStyle.SetStyle(Style);
			ComboButton->BackgroundImageComponent->RefreshAppliedStyleData();

			Style = SceneClient->ActiveSkin->FindStyle(ToggleButtonStyleName);
			ComboButton->CheckedBackgroundImageComponent->ImageStyle.SetStyle(Style);
			ComboButton->CheckedBackgroundImageComponent->RefreshAppliedStyleData();
		}

		if(ComboEditbox)
		{
			UUIStyle* Style = SceneClient->ActiveSkin->FindStyle(EditboxBGStyleName);
			ComboEditbox->BackgroundImageComponent->ImageStyle.SetStyle(Style);
			ComboEditbox->BackgroundImageComponent->RefreshAppliedStyleData();

			// Get the default value for the edit box.
			FUIProviderFieldValue FieldValue(EC_EventParm);

			if(GetDataStoreFieldValue(ComboList->GetDataStoreBinding(), FieldValue) && FieldValue.HasValue())
			{
				debugf(TEXT("%s: %s"), *ComboList->GetDataStoreBinding(), *FieldValue.StringValue);
				ComboEditbox->SetDataStoreBinding(FieldValue.StringValue);
			}
		}
	}
}


/*=========================================================================================
  UUTDrawPanel - A panel that gives normal Canvas drawing functions to script
  ========================================================================================= */

void UUTDrawPanel::PostRender_Widget( FCanvas* NewCanvas )
{
	Super::PostRender_Widget(NewCanvas);

	// Create a temporary canvas if there isn't already one.
	UCanvas* CanvasObject = FindObject<UCanvas>(UObject::GetTransientPackage(),TEXT("CanvasObject"));
	if( !CanvasObject )
	{
		CanvasObject = ConstructObject<UCanvas>(UCanvas::StaticClass(),UObject::GetTransientPackage(),TEXT("CanvasObject"));
		CanvasObject->AddToRoot();
	}
	if (CanvasObject)
	{
		CanvasObject->Canvas = NewCanvas;

		CanvasObject->Init();

		FVector2D ViewportSize;
		GetViewportSize(ViewportSize);

		FLOAT Left = GetPosition(UIFACE_Left, EVALPOS_PixelViewport);
		FLOAT Top = GetPosition(UIFACE_Top, EVALPOS_PixelViewport);
		FLOAT Right = GetPosition(UIFACE_Right, EVALPOS_PixelViewport);
		FLOAT Bottom = GetPosition(UIFACE_Bottom, EVALPOS_PixelViewport);
		
		CanvasObject->SizeX = appTrunc( Right - Left ); //RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left]);
		CanvasObject->SizeY = appTrunc( Bottom - Top ); // RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top]);

		CanvasObject->SceneView = NULL;
		CanvasObject->Update();

		CanvasObject->OrgX = Left; //RenderBounds[UIFACE_Left];
		CanvasObject->OrgY = Top; //RenderBounds[UIFACE_Top];

		if ( !DELEGATE_IS_SET(DrawDelegate) || !delegateDrawDelegate( CanvasObject) )
		{
			Canvas = CanvasObject;
			eventDrawPanel();
			Canvas = NULL;
		}

		CanvasObject->SizeX = appTrunc(ViewportSize.X);
		CanvasObject->SizeY = appTrunc(ViewportSize.Y);
		CanvasObject->OrgX = 0.0;
		CanvasObject->OrgY = 0.0;
		CanvasObject->Init();
		CanvasObject->Update();
	}
}

void UUTDrawPanel::Draw2DLine(INT X1,INT Y1,INT X2,INT Y2,FColor LineColor)
{
	check(Canvas);
	DrawLine2D(Canvas->Canvas, FVector2D(Canvas->OrgX+X1, Canvas->OrgY+Y1), FVector2D(Canvas->OrgX+X2, Canvas->OrgY+Y2), LineColor);
}

/*=========================================================================================
	UUTUIButtonBar
========================================================================================= */
/**
 * Determines whether this widget can become the focused control.
 *
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player to check focus availability
 *
 * @return	TRUE if this widget (or any of its children) is capable of becoming the focused control.
 */
UBOOL UUTUIButtonBar::CanAcceptFocus( INT PlayerIndex ) const
{
#if CONSOLE
	return FALSE;
#else
	return Super::CanAcceptFocus(PlayerIndex);
#endif
}


/*=========================================================================================
	UUTUIButtonBarButton
========================================================================================= */
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
void UUTUIButtonBarButton::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner )
{
	// Make sure this button isnt focusable, mouseoverable, or pressable on console.
#if CONSOLE
	DefaultStates.RemoveItem(UUIState_Focused::StaticClass());
	DefaultStates.RemoveItem(UUIState_Active::StaticClass());
	DefaultStates.RemoveItem(UUIState_Pressed::StaticClass());

	for(INT StateIdx=0; StateIdx<InactiveStates.Num(); StateIdx++)
	{
		if(InactiveStates(StateIdx)->IsA(UUIState_Focused::StaticClass()) || InactiveStates(StateIdx)->IsA(UUIState_Active::StaticClass()) || InactiveStates(StateIdx)->IsA(UUIState_Pressed::StaticClass()))
		{
			InactiveStates.Remove(StateIdx);
			StateIdx--;
		}
	}
#endif

	Super::Initialize(inOwnerScene, inOwner);
}
/**
 * Determines whether this widget can become the focused control.
 *
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player to check focus availability
 *
 * @return	TRUE if this widget (or any of its children) is capable of becoming the focused control.
 */
UBOOL UUTUIButtonBarButton::CanAcceptFocus( INT PlayerIndex ) const
{
#if CONSOLE
	return FALSE;
#else
	return Super::CanAcceptFocus(PlayerIndex);
#endif
}

/*=========================================================================================
	UUIComp_UTDrawStateImage
========================================================================================= */
IMPLEMENT_CLASS(UUIComp_UTDrawStateImage)

/**
 * Applies the current style data (including any style data customization which might be enabled) to the component's image.
 */
void UUIComp_UTDrawStateImage::RefreshAppliedStyleData()
{
	if ( ImageRef == NULL )
	{
		// we have no image if we've never been assigned a value...if this is the case, create one
		// so that the style's DefaultImage will be rendererd
		SetImage(NULL);
	}
	else
	{
		// get the style data that should be applied to the image
		UUIStyle_Image* ImageStyleData = GetAppliedImageStyle(ImageState->GetDefaultObject<UUIState>());

		// ImageStyleData will be NULL if this component has never resolved its style
		if ( ImageStyleData != NULL )
		{
			// apply this component's per-instance image style settings
			FUICombinedStyleData FinalStyleData(ImageStyleData);
			CustomizeAppliedStyle(FinalStyleData);

			// apply the style data to the image
			ImageRef->SetImageStyle(FinalStyleData);
		}
	}
}

/*=========================================================================================
	UUTUISlider
========================================================================================= */
IMPLEMENT_CLASS(UUTUISlider)

/**
 * Render this button.
 *
 * @param	Canvas	the FCanvas to use for rendering this widget
 */
void UUTUISlider::Render_Widget( FCanvas* Canvas )
{
	FRenderParameters Parameters(
		RenderBounds[UIFACE_Left], RenderBounds[UIFACE_Top],
		RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left],
		RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top]
	);

	// first, render the slider background
	if ( BackgroundImageComponent != NULL )
	{
		BackgroundImageComponent->RenderComponent(Canvas, Parameters);
	}

	// next, render the slider bar
	if ( SliderBarImageComponent != NULL )
	{
		FRenderParameters BarRenderParameters = Parameters;
		if ( SliderOrientation == UIORIENT_Horizontal )
		{
			BarRenderParameters.DrawXL = GetMarkerPosition() - BarRenderParameters.DrawX + MarkerWidth.GetValue(this) / 2.0f;
			BarRenderParameters.DrawYL = BarSize.GetValue(this);
			BarRenderParameters.DrawY += (Parameters.DrawYL - BarRenderParameters.DrawYL) / 2;
		}
		else
		{
			BarRenderParameters.DrawYL = GetMarkerPosition() - BarRenderParameters.DrawY;
		}

		SliderBarImageComponent->RenderComponent(Canvas, BarRenderParameters);
	}

	// next, render the marker 
	if ( MarkerImageComponent != NULL )
	{
		FLOAT MarkerPosition = GetMarkerPosition();
		FRenderParameters MarkerRenderParameters;

		if ( SliderOrientation == UIORIENT_Horizontal )
		{
			MarkerRenderParameters.DrawX = MarkerPosition;
			MarkerRenderParameters.DrawY = RenderBounds[UIFACE_Top];
			MarkerRenderParameters.DrawXL = MarkerWidth.GetValue(this);
			MarkerRenderParameters.DrawYL = RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top];

			MarkerRenderParameters.ImageExtent.Y = MarkerHeight.GetValue(this);
		}
		else
		{
			FLOAT ActualMarkerHeight = MarkerHeight.GetValue(this);

			MarkerRenderParameters.DrawX = RenderBounds[UIFACE_Left];
			MarkerRenderParameters.DrawY = MarkerPosition + ActualMarkerHeight;
			MarkerRenderParameters.DrawXL = RenderBounds[UIFACE_Right] - RenderBounds[UIFACE_Left];
			MarkerRenderParameters.DrawYL = ActualMarkerHeight;

			MarkerRenderParameters.ImageExtent.X = MarkerWidth.GetValue(this);
		}

		MarkerImageComponent->RenderComponent(Canvas, MarkerRenderParameters);
	}
}

/*=========================================================================================
	UUTUIOptionButton
========================================================================================= */
IMPLEMENT_CLASS(UUTUIOptionButton)

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
void UUTUIOptionButton::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner )
{
	// Initialize String Component
	if ( StringRenderComponent != NULL )
	{
		TScriptInterface<IUIDataStoreSubscriber> Subscriber(this);
		StringRenderComponent->InitializeComponent(&Subscriber);
	}

	// Must initialize after the component
	Super::Initialize(inOwnerScene, inOwner);

	// Set style tags for the arrow buttons.
	ArrowLeftButton->BackgroundImageComponent->ImageStyle.DefaultStyleTag = TEXT("DefaultOptionButtonLeftArrowStyle");
	ArrowLeftButton->BackgroundImageComponent->ImageStyle.RequiredStyleClass = UUIStyle_Image::StaticClass();
	ArrowLeftButton->BackgroundImageComponent->StyleResolverTag = TEXT("Left Arrow Image Style");

	ArrowRightButton->BackgroundImageComponent->ImageStyle.DefaultStyleTag = TEXT("DefaultOptionButtonRightArrowStyle");
	ArrowRightButton->BackgroundImageComponent->ImageStyle.RequiredStyleClass = UUIStyle_Image::StaticClass();
	ArrowRightButton->BackgroundImageComponent->StyleResolverTag = TEXT("Right Arrow Image Style");

	// Get current list element provider and selected index.
	ResolveListElementProvider();
	RefreshSubscriberValue();

	// Set private flags for the widget if we are in-game.
	if(GIsGame)
	{
		ArrowLeftButton->SetPrivateBehavior(UCONST_PRIVATE_NotFocusable, TRUE);
		ArrowRightButton->SetPrivateBehavior(UCONST_PRIVATE_NotFocusable, TRUE);
	}

	// Update states for arrows.
	ActivateArrowState(NULL, OPTBUT_ArrowRight);
	ActivateArrowState(NULL, OPTBUT_ArrowLeft);

	RequestSceneUpdate(FALSE,FALSE,FALSE,TRUE);
}

/**
 * Provides a way for widgets to fill their style subscribers array prior to performing any other initialization tasks.
 *
 * This version adds the LabelBackground (if non-NULL) to the StyleSubscribers array.
 */
void UUTUIOptionButton::InitializeStyleSubscribers()
{
	Super::InitializeStyleSubscribers();

	AddStyleSubscriber(BackgroundImageComponent);
	AddStyleSubscriber(StringRenderComponent);
}


/**
 * Adds the specified face to the DockingStack for the specified widget
 *
 * @param	DockingStack	the docking stack to add this docking node to.  Generally the scene's DockingStack.
 * @param	Face			the face that should be added
 *
 * @return	TRUE if a docking node was added to the scene's DockingStack for the specified face, or if a docking node already
 *			existed in the stack for the specified face of this widget.
 */
UBOOL UUTUIOptionButton::AddDockingNode( TArray<FUIDockingNode>& DockingStack, EUIWidgetFace Face )
{
	return StringRenderComponent 
		? StringRenderComponent->AddDockingNode(DockingStack, Face)
		: Super::AddDockingNode(DockingStack, Face);
}

/**
 * Evaluates the Position value for the specified face into an actual pixel value.  Should only be
 * called from UIScene::ResolvePositions.  Any special-case positioning should be done in this function.
 *
 * @param	Face	the face that should be resolved
 */
void UUTUIOptionButton::ResolveFacePosition( EUIWidgetFace Face )
{
	Super::ResolveFacePosition(Face);

	if ( StringRenderComponent != NULL )
	{
		StringRenderComponent->ResolveFacePosition(Face);
	}
}

/**
 * Marks the Position for any faces dependent on the specified face, in this widget or its children,
 * as out of sync with the corresponding RenderBounds.
 *
 * @param	Face	the face to modify; value must be one of the EUIWidgetFace values.
 */
void UUTUIOptionButton::InvalidatePositionDependencies( BYTE Face )
{
	Super::InvalidatePositionDependencies(Face);
	if ( StringRenderComponent != NULL )
	{
		StringRenderComponent->InvalidatePositionDependencies(Face);
	}
}

/**
 * Called when a property is modified that could potentially affect the widget's position onscreen.
 */
void UUTUIOptionButton::RefreshPosition()
{
	Super::RefreshPosition();

	RefreshFormatting();
}

/**
 * Called to globally update the formatting of all UIStrings.
 */
void UUTUIOptionButton::RefreshFormatting()
{
	if ( StringRenderComponent != NULL )
	{
		StringRenderComponent->ReapplyFormatting();
	}
}


void UUTUIOptionButton::ResolveListElementProvider()
{
	TScriptInterface<IUIListElementProvider> Result;
	if ( DataSource.ResolveMarkup( this ) )
	{
		DataProvider = DataSource->ResolveListElementProvider(DataSource.DataStoreField.ToString());
	}
}

/** @return Returns the number of possible values for the field we are bound to. */
INT UUTUIOptionButton::GetNumValues()
{
	INT NumValues = 0;

	if(DataProvider)
	{
		NumValues = DataProvider->GetElementCount(DataSource.DataStoreField);
	}
	
	return NumValues;
}

/** 
 * @param Index		List index to get the value of.
 *
 * @return Returns the number of possible values for the field we are bound to. 
 */
UBOOL UUTUIOptionButton::GetListValue(INT ListIndex, FString &OutValue)
{
	UBOOL bResult = FALSE;

	if(DataProvider)
	{
		FUIProviderFieldValue FieldValue;
		appMemzero(&FieldValue, sizeof(FUIProviderFieldValue));

		TScriptInterface<class IUIListElementCellProvider> ValueProvider = DataProvider->GetElementCellValueProvider(DataSource.DataStoreField, ListIndex);

		if(ValueProvider)
		{
			bResult = ValueProvider->GetCellFieldValue(DataSource.DataStoreField, ListIndex, FieldValue);

			if(bResult)
			{
				OutValue = FieldValue.StringValue;
			}
		}
	}

	return bResult;
}	

/** Updates the string value using the current index. */
void UUTUIOptionButton::UpdateCurrentStringValue()
{
	FString OutValue;

	if(GetListValue(CurrentIndex, OutValue))
	{
		StringRenderComponent->SetValue(OutValue);
	}
}

/**
 * Generates a array of UI Action keys that this widget supports.
 *
 * @param	out_KeyNames	Storage for the list of supported keynames.
 */
void UUTUIOptionButton::GetSupportedUIActionKeyNames(TArray<FName> &out_KeyNames )
{
	Super::GetSupportedUIActionKeyNames(out_KeyNames);

	out_KeyNames.AddItem(UIKEY_MoveSelectionRight);
	out_KeyNames.AddItem(UIKEY_MoveSelectionLeft);
}

/**
 * Render this button.
 *
 * @param	Canvas	the FCanvas to use for rendering this widget
 */
void UUTUIOptionButton::Render_Widget( FCanvas* Canvas )
{
	const FLOAT ButtonSpacingPixels = ButtonSpacing.GetValue(this, EVALPOS_PixelOwner);
	const FLOAT ButtonSize = ArrowRightButton->GetPosition(UIFACE_Right, EVALPOS_PixelViewport)-ArrowLeftButton->GetPosition(UIFACE_Left, EVALPOS_PixelViewport);

	// Render background
	if ( BackgroundImageComponent != NULL )
	{
		FRenderParameters Parameters(
			RenderBounds[UIFACE_Left], RenderBounds[UIFACE_Top],
			ArrowLeftButton->GetPosition(UIFACE_Left, EVALPOS_PixelViewport)-RenderBounds[UIFACE_Left]-ButtonSpacingPixels,
			RenderBounds[UIFACE_Bottom] - RenderBounds[UIFACE_Top]
		);

		BackgroundImageComponent->RenderComponent(Canvas, Parameters);
	}

	// Render String Text
	Canvas->PushRelativeTransform(FTranslationMatrix(FVector(-(ButtonSize+ButtonSpacingPixels), 0.0f, 0.0f)));
	{
		StringRenderComponent->Render_String(Canvas);
	}
	Canvas->PopTransform();
}

/**
 * Handles input events for this button.
 *
 * @param	EventParms		the parameters for the input event
 *
 * @return	TRUE to consume the key event, FALSE to pass it on.
 */
UBOOL UUTUIOptionButton::ProcessInputKey( const struct FSubscribedInputEventParameters& EventParms )
{
	UBOOL bResult = FALSE;
	// Move Left Event
	if ( EventParms.InputAliasName == UIKEY_MoveSelectionLeft )
	{
		if(IsFocused()==FALSE)
		{
			SetFocus(NULL);
		}

		if ( EventParms.EventType == IE_Pressed )
		{
			if(HasPrevValue())
			{
				ActivateArrowState(UUIState_Pressed::StaticClass(), OPTBUT_ArrowLeft);
			}

			bResult = TRUE;
		}
		else if ( EventParms.EventType == IE_Released || EventParms.EventType == IE_Repeat )
		{
			eventOnMoveSelectionLeft(EventParms.PlayerIndex);

			bResult = TRUE;
		}
	}

	// Move Right Event
	if ( EventParms.InputAliasName == UIKEY_MoveSelectionRight )
	{
		if(IsFocused()==FALSE)
		{
			SetFocus(NULL);
		}

		if ( EventParms.EventType == IE_Pressed )
		{
			if(HasNextValue())
			{
				ActivateArrowState(UUIState_Pressed::StaticClass(), OPTBUT_ArrowRight);
			}

			bResult = TRUE;
		}
		else if ( EventParms.EventType == IE_Released || EventParms.EventType == IE_Repeat )
		{
			eventOnMoveSelectionRight(EventParms.PlayerIndex);

			bResult = TRUE;
		}
	}

	// Make sure to call the superclass's implementation after trying to consume input ourselves so that
	// we can respond to events defined in the super's class.
	if(bResult == FALSE)
	{
		bResult = Super::ProcessInputKey(EventParms);
	}

	return bResult;
}

/**
 * Handles input events for this button.
 *
 * @param	InState		State class to activate, if NULL, the arrow will be enabled or disabled state depending on whether the option button can move selection in the direction of the arrow.
 * @param	Arrow		Which arrow to activate the state for, LEFT is 0, RIGHT is 1.
 *
 * @return	TRUE to consume the key event, FALSE to pass it on.
 */
void UUTUIOptionButton::ActivateArrowState( UClass* InState, EOptionButtonArrow Arrow )
{
	if(InState == NULL)
	{
		InState = GetArrowEnabledState(Arrow);
	}

	if(Arrow == OPTBUT_ArrowRight)
	{
		if(InState != UUIState_Focused::StaticClass())
		{
			ArrowRightButton->DeactivateStateByClass(UUIState_Focused::StaticClass(),0);
		}

		if(InState != UUIState_Pressed::StaticClass())
		{
			ArrowRightButton->DeactivateStateByClass(UUIState_Pressed::StaticClass(),0);
		}

		ArrowRightButton->ActivateStateByClass(InState,0);
		ArrowRightButton->RequestSceneUpdate(FALSE,FALSE,FALSE,TRUE);
	}
	else
	{
		if(InState != UUIState_Focused::StaticClass())
		{
			ArrowLeftButton->DeactivateStateByClass(UUIState_Focused::StaticClass(),0);
		}

		if(InState != UUIState_Pressed::StaticClass())
		{
			ArrowLeftButton->DeactivateStateByClass(UUIState_Pressed::StaticClass(),0);
		}

		ArrowLeftButton->ActivateStateByClass(InState,0);
		ArrowLeftButton->RequestSceneUpdate(FALSE,FALSE,FALSE,TRUE);
	}
}

/**
 * Returns the current arrow state(Enabled or disabled) depending on whether or not the widget can shift left or right.
 */
UClass* UUTUIOptionButton::GetArrowEnabledState(EOptionButtonArrow Arrow)
{
	UClass* State = UUIState_Enabled::StaticClass();

	if(Arrow == OPTBUT_ArrowLeft)
	{
		if(HasPrevValue() == FALSE)
		{
			State = UUIState_Disabled::StaticClass();
		}
		else
		{
			if(IsFocused())
			{
				State = UUIState_Focused::StaticClass();
			}
		}
	}
	else
	{
		if(HasNextValue() == FALSE)
		{
			State = UUIState_Disabled::StaticClass();
		}
		else
		{
			if(IsFocused())
			{
				State = UUIState_Focused::StaticClass();
			}
		}
	}


	return State;
}


/**
 * @return TRUE if the CurrentIndex is at the start of the ValueMappings array, FALSE otherwise.
 */
UBOOL UUTUIOptionButton::HasPrevValue()
{
	if(bWrapOptions)
	{
		return (GetNumValues()>1); 
	}
	else
	{
		return (CurrentIndex>0);
	}
}

/**
 * @return TRUE if the CurrentIndex is at the start of the ValueMappings array, FALSE otherwise.
 */
UBOOL UUTUIOptionButton::HasNextValue()
{
	if(bWrapOptions)
	{
		return (GetNumValues()>1);
	}
	else
	{
		return (CurrentIndex<(GetNumValues()-1));
	}
}

/**
 * Moves the current index back by 1 in the valuemappings array if it isn't already at the front of the array.
 */
void UUTUIOptionButton::SetPrevValue()
{
	if(DataProvider)
	{
		const INT NumValues = GetNumValues();

		if(CurrentIndex > 0)
		{
			SetCurrentIndex(CurrentIndex-1);
		}
		else if((NumValues>1) && bWrapOptions)
		{
			SetCurrentIndex(NumValues - 1);
		}
	}
}

/**
 * Moves the current index forward by 1 in the valuemappings array if it isn't already at the end of the array.
 */
void UUTUIOptionButton::SetNextValue()
{
	if(DataProvider)
	{
		const INT NumValues = GetNumValues();

		if(NumValues - 1 > CurrentIndex && DataProvider)
		{
			SetCurrentIndex(CurrentIndex+1);
		}
		else if((NumValues>1) && bWrapOptions)
		{
			SetCurrentIndex(0);
		}
	}
}

/**
 * Activates the UIState_Focused menu state and updates the pertinent members of FocusControls.
 *
 * @param	FocusedChild	the child of this widget that should become the "focused" control for this widget.
 *							A value of NULL indicates that there is no focused child.
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player that generated the focus change.
 */
UBOOL UUTUIOptionButton::GainFocus( UUIObject* FocusedChild, INT PlayerIndex )
{
	// Make sure to call super first since it updates our focused state properly.
	const UBOOL bResult = Super::GainFocus(FocusedChild, PlayerIndex);

	// Update states for arrows.
	ActivateArrowState(NULL, OPTBUT_ArrowRight);
	ActivateArrowState(NULL, OPTBUT_ArrowLeft);

	return bResult;
}

/**
 * Deactivates the UIState_Focused menu state and updates the pertinent members of FocusControls.
 *
 * @param	FocusedChild	the child of this widget that is currently "focused" control for this widget.
 *							A value of NULL indicates that there is no focused child.
 * @param	PlayerIndex		the index [into the Engine.GamePlayers array] for the player that generated the focus change.
 */
UBOOL UUTUIOptionButton::LoseFocus( UUIObject* FocusedChild, INT PlayerIndex )
{
	// Make sure to call super first since it updates our focused state properly.
	const UBOOL bResult = Super::LoseFocus(FocusedChild, PlayerIndex);

	// Update states for arrows.
	ActivateArrowState(NULL, OPTBUT_ArrowRight);
	ActivateArrowState(NULL, OPTBUT_ArrowLeft);

	return bResult;
}

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
UBOOL UUTUIOptionButton::SaveSubscriberValue( TArray<UUIDataStore*>& out_BoundDataStores, INT BindingIndex/*=INDEX_NONE*/ )
{
	UBOOL bResult = FALSE;
	FUIProviderFieldValue FieldValue(EC_EventParm);

	if(GetListValue(CurrentIndex,FieldValue.StringValue))
	{
		bResult = SetDataStoreFieldValue(DataSource.MarkupString, FieldValue, GetScene(), GetPlayerOwner());
	}

	return bResult;
}


/**
 * Called when the option button's index has changed.
 *
 * @param	PreviousIndex	the list's Index before it was changed
 * @param	PlayerIndex		the index of the player associated with this index change.
 */
void UUTUIOptionButton::NotifyIndexChanged( INT PreviousIndex, INT PlayerIndex )
{
	// Update the text for the option button
	UpdateCurrentStringValue();

	// update arrow state
	ActivateArrowState(NULL, OPTBUT_ArrowLeft);
	ActivateArrowState(NULL, OPTBUT_ArrowRight);

	if(GIsGame)
	{
		// activate the on value changed event
		if ( DELEGATE_IS_SET(OnValueChanged) )
		{
			// notify unrealscript that the user has submitted the selection
			delegateOnValueChanged(this,PlayerIndex);
		}

		ActivateEventByClass(PlayerIndex,UUIEvent_ValueChanged::StaticClass(), this);
	}

	//@todo/@hack: For UT3 we need to save subscriber values immediately instead of on scene close.
	TArray<UUIDataStore*> OutDataStores;
	SaveSubscriberValue(OutDataStores);
}

/**
 * @return Returns the current index for the option button.
 */
INT UUTUIOptionButton::GetCurrentIndex()
{
	return CurrentIndex;
}

/**
 * Sets a new index for the option button.
 *
 * @param NewIndex		New index for the option button.
 */
void UUTUIOptionButton::SetCurrentIndex(INT NewIndex)
{
	if(GetNumValues() > NewIndex && NewIndex >= 0)
	{
		INT PreviousIndex = CurrentIndex;
		CurrentIndex = NewIndex;

		// Activate index changed events.
		TArray<INT> Players;
		GetInputMaskPlayerIndexes(Players);

		INT PlayerIdx = 0;

		if(Players.Num() == 1)
		{
			PlayerIdx = Players(0);
		}

		NotifyIndexChanged(PreviousIndex, PlayerIdx);
	}
}


/** === UIDataSourceSubscriber interface === */
/**
 * Sets the data store binding for this object to the text specified.
 *
 * @param	MarkupText			a markup string which resolves to data exposed by a data store.  The expected format is:
 *								<DataStoreTag:DataFieldTag>
 * @param	BindingIndex		optional parameter for indicating which data store binding is being requested for those
 *								objects which have multiple data store bindings.  How this parameter is used is up to the
 *								class which implements this interface, but typically the "primary" data store will be index 0.
 */
void UUTUIOptionButton::SetDataStoreBinding( const FString& MarkupText, INT BindingIndex/*=-1*/ )
{
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		SetDefaultDataBinding(MarkupText,BindingIndex);
	}
	else if ( DataSource.MarkupString != MarkupText )
	{
		Modify();
        DataSource.MarkupString = MarkupText;

		// Resolve the list element provider.
		ResolveListElementProvider();

		// Refresh 
		RefreshSubscriberValue(BindingIndex);
	}
}

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
FString UUTUIOptionButton::GetDataStoreBinding( INT BindingIndex/*=-1*/ ) const
{
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		return GetDefaultDataBinding(BindingIndex);
	}
	return DataSource.MarkupString;
}

/**
 * Resolves this subscriber's data store binding and updates the subscriber with the current value from the data store.
 *
 * @return	TRUE if this subscriber successfully resolved and applied the updated value.
 */
UBOOL UUTUIOptionButton::RefreshSubscriberValue( INT BindingIndex/*=INDEX_NONE*/ )
{
	UBOOL bResult = FALSE;
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		bResult = ResolveDefaultDataBinding(BindingIndex);
	}
	else if ( StringRenderComponent != NULL && IsInitialized() )
	{
		// Get the default value for the edit box.
		FUIProviderFieldValue FieldValue(EC_EventParm);

		if(GetDataStoreFieldValue(GetDataStoreBinding(), FieldValue, GetScene(), GetPlayerOwner()) && FieldValue.HasValue())
		{
			StringRenderComponent->SetValue(FieldValue.StringValue);

			// Loop through all list values and try to match the current string value to a index.
			FString OutValue;
			CurrentIndex = 0;

			for(INT OptIdx=0; OptIdx<GetNumValues(); OptIdx++)
			{
				if(GetListValue(OptIdx, OutValue))
				{
					if(FieldValue.StringValue==OutValue)
					{
						CurrentIndex = OptIdx;
						break;
					}
				}
			}
			bResult = TRUE;
		}
	}

	return bResult;
}

/**
 * Retrieves the list of data stores bound by this subscriber.
 *
 * @param	out_BoundDataStores		receives the array of data stores that subscriber is bound to.
 */
void UUTUIOptionButton::GetBoundDataStores( TArray<UUIDataStore*>& out_BoundDataStores )
{
	GetDefaultDataStores(out_BoundDataStores);
	// add overall data store to the list
	if ( DataSource )
	{
		out_BoundDataStores.AddUniqueItem(*DataSource);
	}
}

/**
 * Notifies this subscriber to unbind itself from all bound data stores
 */
void UUTUIOptionButton::ClearBoundDataStores()
{
	TMultiMap<FName,FUIDataStoreBinding*> DataBindingMap;
	GetDataBindings(DataBindingMap);

	TArray<FUIDataStoreBinding*> DataBindings;
	DataBindingMap.GenerateValueArray(DataBindings);
	for ( INT BindingIndex = 0; BindingIndex < DataBindings.Num(); BindingIndex++ )
	{
		FUIDataStoreBinding* Binding = DataBindings(BindingIndex);
		Binding->ClearDataBinding();
	}

	TArray<UUIDataStore*> DataStores;
	GetBoundDataStores(DataStores);

	for ( INT DataStoreIndex = 0; DataStoreIndex < DataStores.Num(); DataStoreIndex++ )
	{
		UUIDataStore* DataStore = DataStores(DataStoreIndex);
		DataStore->eventSubscriberDetached(this);
	}
}


/*=========================================================================================
	UTSimpleList
========================================================================================= */

IMPLEMENT_COMPARE_CONSTREF(FSimpleListData,UTUI_BaseWidgets, 
	{ 
		return appStricmp(*A.Text,*B.Text);
	} 
);




int UUTSimpleList::Find(const FString& SearchText)
{
	for (INT i=0;i<List.Num();i++)
	{
		if ( List(i).Text == SearchText )
		{
			return i;
		}
	}
	return INDEX_NONE;
}

int UUTSimpleList::FindTag(INT SearchTag)
{
	for (INT i=0;i<List.Num();i++)
	{
		if ( List(i).Tag == SearchTag )
		{
			return i;
		}
	}
	return INDEX_NONE;
}

void UUTSimpleList::SortList()
{
	Sort <USE_COMPARE_CONSTREF(FSimpleListData,UTUI_BaseWidgets) > ( &List(0), List.Num() );
}


void UUTSimpleList::UpdateAnimation(FLOAT DeltaTime)
{
	UBOOL bNeedsInvalidation = false;
	for (INT i=0;i<List.Num(); i++)
	{
		if ( List(i).CellTransitionTime > 0.0f )
		{
			FLOAT Pos = 1.0 - (List(i).CellTransitionTime / TransitionTime);
			List(i).CurHeightMultiplier = Lerp<FLOAT>( List(i).HeightMultipliers[0], List(i).HeightMultipliers[1], Pos);
			bNeedsInvalidation = true;

			List(i).CellTransitionTime -= DeltaTime;

		}
		else
		{
			List(i).CurHeightMultiplier = List(i).HeightMultipliers[1];
		}
	}

	if ( bNeedsInvalidation )
	{
		bInvalidated = true;
	}
}

void UUTSimpleList::PostEditChange(UProperty* PropertyThatChanged)
{
	if(PropertyThatChanged && PropertyThatChanged->GetName()==TEXT("Selection"))
	{
		if (List.Num() > 0)
		{
			eventSelectItem(Selection);
		}
	}
	else
	{
		if (List.Num() > 0)
		{
			eventSelectItem(0);
		}
	}
}
