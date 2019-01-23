/**
 * UTUI_FrontEnd.cpp: Implementation file for all front end UnrealScript UI classes.
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
#include "UTGame.h"
#include "UTGameUIClasses.h"
#include "UTGameUIFrontEndClasses.h"

IMPLEMENT_CLASS(UUTUIFrontEnd)
IMPLEMENT_CLASS(UUTUIFrontEnd_MapSelection)
IMPLEMENT_CLASS(UUTUIFrontEnd_CharacterCustomization)

//////////////////////////////////////////////////////////////////////////
// UUTUITabPage_CharacterPart
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUTUITabPage_CharacterPart)
/**
 * Routes rendering calls to children of this screen object.
 *
 * @param	Canvas	the canvas to use for rendering
 */
void UUTUITabPage_CharacterPart::Render_Children( FCanvas* Canvas )
{
	Canvas->AlphaModulate = Opacity;
	Canvas->PushRelativeTransform(FTranslationMatrix(OffsetVector));
	{
		Super::Render_Children(Canvas);
	}
	Canvas->PopTransform();
}

//////////////////////////////////////////////////////////////////////////
// UUTUIFrontEnd_CharacterCustomization
//////////////////////////////////////////////////////////////////////////
void UUTUIFrontEnd_CharacterCustomization::Tick( FLOAT DeltaTime )
{
	Super::Tick(DeltaTime);

	// Update our loading package status
	eventUpdateLoadingPackage();

	// Rotate the actor if there is a rotation key held down.
	if(RotationDirection != 0)
	{	
		FVector CurrentEulerRotation = PreviewActor->Rotation.Euler();
		const FLOAT RotationRate = RotationDirection*72.0f;
		CurrentEulerRotation.Z += RotationRate*DeltaTime;
		PreviewActor->SetRotation(FRotator::MakeFromEuler(CurrentEulerRotation));
	}
}

//////////////////////////////////////////////////////////////////////////
// UUTUIFrontEnd_TitleScreen
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUTUIFrontEnd_TitleScreen)

/** Creates a local player for every signed in controller. */
void UUTUIFrontEnd_TitleScreen::UpdateGamePlayersArray()
{
	FString Error;

	// If a player is logged in and doesn't have a LP yet, then create one.
	for(INT ControllerId=0; ControllerId<UCONST_MAX_SUPPORTED_GAMEPADS; ControllerId++)
	{
		const INT PlayerIndex = UUIInteraction::GetPlayerIndex(ControllerId);

		if(eventIsLoggedIn(ControllerId))
		{
			if(PlayerIndex==INDEX_NONE)
			{
				debugf(TEXT("UUTUIFrontEnd_TitleScreen::UpdateGamePlayersArray() - Creating LocalPlayer with Controller Id: %i"), ControllerId);
				GEngine->GameViewport->eventCreatePlayer(ControllerId, Error, TRUE);
			}
		}
	}

	// If a player is not logged in, and has a LP, then remove it, but make sure there is always 1 LP in existence.
	for(INT ControllerId=0; ControllerId<UCONST_MAX_SUPPORTED_GAMEPADS && GEngine->GamePlayers.Num() > 1; ControllerId++)
	{
		if(eventIsLoggedIn(ControllerId)==FALSE)
		{
			const INT PlayerIndex = UUIInteraction::GetPlayerIndex(ControllerId);

			if(PlayerIndex!=INDEX_NONE)
			{
				debugf(TEXT("UUTUIFrontEnd_TitleScreen::UpdateGamePlayersArray() - Removing LocalPlayer(Index: %i) with Controller Id: %i"), PlayerIndex, ControllerId);
				GEngine->GameViewport->eventRemovePlayer(GEngine->GamePlayers(PlayerIndex));
			}
		}
	}

	// Update the profile labels with the new login status of the players.
	eventUpdateProfileLabels();
}

/** Tick function for the scene, launches attract mode movie. */
void UUTUIFrontEnd_TitleScreen::Tick(FLOAT DeltaTime)
{
	Super::Tick(DeltaTime);

	// Only try to launch attract movie if we are the top scene in the stack.
	UGameUISceneClient* TheSceneClient = GetSceneClient();

	if(TheSceneClient)
	{
		const INT NumScenes = TheSceneClient->ActiveScenes.Num();
		if(NumScenes && TheSceneClient->ActiveScenes(NumScenes-1)==this)
		{
			if(bInMovie==FALSE)
			{
				TimeElapsed += DeltaTime;
				if(TimeElapsed > TimeTillAttractMovie)
				{
					StartMovie();
				}
			}
			else
			{
				UpdateMovieStatus();
			}
		}
	}

	if(bUpdatePlayersOnNextTick)
	{
		UpdateGamePlayersArray();
		bUpdatePlayersOnNextTick=FALSE;
	}
}

/** Starts the attract mode movie. */
void UUTUIFrontEnd_TitleScreen::StartMovie()
{
	debugf(TEXT("UUTUIFrontEnd_TitleScreen::StartMovie() - Starting Attract Mode Movie"));

	if( GFullScreenMovie )
	{
		// Play movie and block on playback.
		GFullScreenMovie->GameThreadPlayMovie(MM_PlayOnceFromStream, TEXT("Attract_Movie"));
	}

	bInMovie = TRUE;
}

/** Stops the currently playing movie. */
void UUTUIFrontEnd_TitleScreen::StopMovie()
{
	debugf(TEXT("UUTUIFrontEnd_TitleScreen::StopMovie() - Stopping Attract Mode Movie"));

	bInMovie = FALSE;
	TimeElapsed = 0.0f;

	if( GFullScreenMovie )
	{
		// Stop Movie
		GFullScreenMovie->GameThreadStopMovie();
	}
}

/** Checks to see if a movie is done playing. */
void UUTUIFrontEnd_TitleScreen::UpdateMovieStatus()
{
	if(GFullScreenMovie && GFullScreenMovie->GameThreadIsMovieFinished(TEXT("Attract_Movie")))
	{
		StopMovie();
	}
}

//////////////////////////////////////////////////////////////////////////
// UUTUIFrontEnd_Credits
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUTUIFrontEnd_Credits)

namespace
{
	static DOUBLE DoubleStartTime;
}

/** Sets up the scene objects to begin fading/scrolling the credits widgets. */
void UUTUIFrontEnd_Credits::SetupScene()
{
	// Front
	PhotoImage[0] = Cast<UUIImage>(FindChild(TEXT("imgPhoto_Front"),TRUE));
	QuoteLabels[0] = Cast<UUILabel>(FindChild(TEXT("lblQuote_Front_1"),TRUE));
	QuoteLabels[1] = Cast<UUILabel>(FindChild(TEXT("lblQuote_Front_2"),TRUE));
	QuoteLabels[2] = Cast<UUILabel>(FindChild(TEXT("lblQuote_Front_3"),TRUE));

	// Back
	PhotoImage[1] = Cast<UUIImage>(FindChild(TEXT("imgPhoto_Back"),TRUE));
	QuoteLabels[3] = Cast<UUILabel>(FindChild(TEXT("lblQuote_Back_1"),TRUE));
	QuoteLabels[4] = Cast<UUILabel>(FindChild(TEXT("lblQuote_Back_2"),TRUE));
	QuoteLabels[5] = Cast<UUILabel>(FindChild(TEXT("lblQuote_Back_3"),TRUE));

	TextLabels[0] = Cast<UUILabel>(FindChild(TEXT("credits_01"),TRUE));
	TextLabels[1] = Cast<UUILabel>(FindChild(TEXT("credits_02"),TRUE));
	TextLabels[2] = Cast<UUILabel>(FindChild(TEXT("credits_03"),TRUE));

	for(INT WidgetIdx=0; WidgetIdx < 2; WidgetIdx++)
	{
		if(PhotoImage[WidgetIdx])
		{
			PhotoImage[WidgetIdx]->Opacity = 0.0f;
		}
	}

	DoubleStartTime = GWorld->GetRealTimeSeconds();
}

/**
 * Callback that happens the first time the scene is rendered, any widget positioning initialization should be done here.
 *
 * By default this function recursively calls itself on all of its children.
 */
void UUTUIFrontEnd_Credits::PreRenderCallback()
{
	Super::PreRenderCallback();

	if(GIsGame)
	{
		UpdateWidgets(TRUE);
		UpdateCreditsText();
	}
}

/** 
 * Updates the position and text for credits widgets.
 */
void UUTUIFrontEnd_Credits::UpdateCreditsText()
{
	const FLOAT CurrentTime = (FLOAT)(GWorld->GetRealTimeSeconds() - DoubleStartTime);
	const FLOAT Offset = CurrentTime / SceneTimeInSec * ScrollAmount;

	if(CurrentTextSet==0)
	{
		// Setup initial labels.
		TextLabels[0]->SetDataStoreBinding(TextSets(0));
		TextLabels[0]->RefreshPosition();

		TextLabels[1]->SetDataStoreBinding(TextSets(1));
		TextLabels[1]->RefreshPosition();

		TextLabels[2]->SetDataStoreBinding(TextSets(2));
		TextLabels[2]->RefreshPosition();

		StartOffset[0] = GetPosition(UIFACE_Bottom, EVALPOS_PixelViewport);
		((UUIScreenObject*)TextLabels[0])->SetPosition(Offset + StartOffset[0], UIFACE_Top, EVALPOS_PixelViewport);

		StartOffset[1] = TextLabels[0]->GetPosition(UIFACE_Bottom, EVALPOS_PixelViewport);
		((UUIScreenObject*)TextLabels[1])->SetPosition(Offset + StartOffset[1], UIFACE_Top, EVALPOS_PixelViewport);
		
		StartOffset[2] = TextLabels[1]->GetPosition(UIFACE_Bottom, EVALPOS_PixelViewport);
		((UUIScreenObject*)TextLabels[2])->SetPosition(Offset + StartOffset[2], UIFACE_Top, EVALPOS_PixelViewport);

		CurrentTextSet = 3;
	}
	else 
	{
		for(INT LabelIdx=0; LabelIdx<3; LabelIdx++)
		{
			if(TextLabels[LabelIdx]->RenderBounds[UIFACE_Bottom] < 0)
			{
				if(CurrentTextSet < TextSets.Num())
				{
					const INT OffsetIdx = (LabelIdx+2)%3;
					TextLabels[LabelIdx]->SetDataStoreBinding(TextSets(CurrentTextSet));
					TextLabels[LabelIdx]->RefreshPosition();

					StartOffset[LabelIdx] = StartOffset[OffsetIdx] + TextLabels[OffsetIdx]->RenderBounds[UIFACE_Bottom] - TextLabels[OffsetIdx]->RenderBounds[UIFACE_Top];
					CurrentTextSet++;
				}
				else
				{
					TextLabels[LabelIdx]->eventSetVisibility(FALSE);
				}
			}
		}
	}

	((UUIScreenObject*)TextLabels[0])->SetPosition(Offset + StartOffset[0], UIFACE_Top, EVALPOS_PixelViewport);
	((UUIScreenObject*)TextLabels[1])->SetPosition(Offset + StartOffset[1], UIFACE_Top, EVALPOS_PixelViewport);
	((UUIScreenObject*)TextLabels[2])->SetPosition(Offset + StartOffset[2], UIFACE_Top, EVALPOS_PixelViewport);
}

/** 
 * Changes the datastore bindings for widgets to the next image set.
 *
 * @param bFront Whether we are updating front or back widgets.
 */
void UUTUIFrontEnd_Credits::UpdateWidgets(UBOOL bFront)
{
	if(ImageSets.IsValidIndex(CurrentImageSet))
	{
		FCreditsImageSet &ImageSet = ImageSets(CurrentImageSet);
		const INT ImgIdx = bFront ? 0 : 1;

		if(PhotoImage[ImgIdx])
		{
			PhotoImage[ImgIdx]->Coordinates = ImageSet.TexCoords;
			PhotoImage[ImgIdx]->ImageComponent->SetCoordinates( PhotoImage[ImgIdx]->Coordinates );
			PhotoImage[ImgIdx]->SetValue(ImageSet.TexImage);

			if(CurrentImageSet==ImageSets.Num() - 1)
			{
				const FLOAT Top = PhotoImage[ImgIdx]->GetPosition(UIFACE_Top, EVALPOS_PixelViewport);
				PhotoImage[ImgIdx]->SetPosition(Top+ImageSet.TexCoords.VL, UIFACE_Bottom,EVALPOS_PixelViewport);
			}
		}

		for(INT LabelIdx=0; LabelIdx < 3; LabelIdx++)
		{
			const INT FinalIdx = LabelIdx + ImgIdx*3;

			if(QuoteLabels[FinalIdx])
			{	
				if(ImageSet.LabelMarkup.IsValidIndex(LabelIdx))
				{
					QuoteLabels[FinalIdx]->SetDataStoreBinding(ImageSet.LabelMarkup(LabelIdx));
					QuoteLabels[FinalIdx]->eventSetVisibility(TRUE);
				}
				else
				{
					QuoteLabels[FinalIdx]->eventSetVisibility(FALSE);
				}
			}
		}

		CurrentImageSet++;
	}
}

/**
 * The scene's tick function, updates the different objects in the scene.
 */
void UUTUIFrontEnd_Credits::Tick( FLOAT DeltaTime  )
{
	Super::Tick(DeltaTime);

	if(GIsGame)
	{
		const FLOAT CurrentTime = (FLOAT)(GWorld->GetRealTimeSeconds() - DoubleStartTime);

		if(CurrentTime < SceneTimeInSec)
		{
			const FLOAT SetTime = ((SceneTimeInSec-(DelayBeforePictures+DelayAfterPictures)) / ImageSets.Num()) * 2.0f;	// We multiply by 2 because each period consists of 2 image sets.
			const FLOAT FadeTime = SetTime * 0.1f;

			// We need an extra fade to fade out the last picture set.
			if(CurrentTime > DelayBeforePictures && CurrentTime < (SceneTimeInSec - DelayAfterPictures + FadeTime))
			{
				static UBOOL bUpdateSet = FALSE;

				const FLOAT SolidTime = SetTime * 0.5f - FadeTime;
				const FLOAT LocalTime = appFmod(CurrentTime-DelayBeforePictures, SetTime);
				UBOOL bFrontVisible = FALSE;
				UBOOL bBackVisible = FALSE;
				FLOAT FrontAlpha;

				// The fading of images is done in a piecewise fashion for a period of SetTime->
				// 1) Front widgets fade in / Back fade out
				// 2) Front widgets stay at 1.0f opacity for some time.
				// 3) Front fade out/ Back fade in
				// 4) Back stay at 1.0f opacity.
				if(LocalTime <= FadeTime)
				{
					FrontAlpha = CubicInterp<FLOAT>(0.0f, 0.0f, 1.0f, 0.0f, LocalTime / FadeTime);	

					if(CurrentImageSet < ImageSets.Num())
					{
						bUpdateSet = TRUE;
						bFrontVisible = TRUE;
					}

					bBackVisible = TRUE;
				}
				else if(LocalTime <= (FadeTime + SolidTime))
				{
					FrontAlpha = 1.0f;
					bFrontVisible = TRUE;

					if(bUpdateSet)
					{
						UpdateWidgets(FALSE);
						bUpdateSet = FALSE;
					}
				}
				else if(LocalTime <= (FadeTime*2 + SolidTime))
				{
					FrontAlpha = 1.0f - CubicInterp<FLOAT>(0.0f, 0.0f, 1.0f, 0.0f, (LocalTime - (FadeTime + SolidTime)) / FadeTime);	
					bFrontVisible = TRUE;
					if(CurrentImageSet < ImageSets.Num())
					{
						bUpdateSet = TRUE;
						bBackVisible = TRUE;
					}
				}
				else
				{
					FrontAlpha = 0.0f;
					bBackVisible = TRUE;

					if(bUpdateSet)
					{
						UpdateWidgets(TRUE);
						bUpdateSet = FALSE;
					}
				}

				if(PhotoImage[0] && PhotoImage[1])
				{
					FLOAT BackAlpha = 1.0f - FrontAlpha;

					if(CurrentImageSet==ImageSets.Num()  && (CurrentImageSet%2)==0 && bFrontVisible==FALSE)
					{
						FrontAlpha = 0.0f;
					}
				
					// make sure we arent fading out the back widgets while we fade in for the first time.
					if(CurrentImageSet==1 || (CurrentImageSet==ImageSets.Num()  && (CurrentImageSet%2)==1 && bBackVisible==FALSE))
					{
						BackAlpha = 0.0f;
					}
					
					PhotoImage[0]->Opacity = FrontAlpha;
					PhotoImage[1]->Opacity = BackAlpha;
				}
			}
			else
			{
				// Hide picture/quote widgets when we are near the start or end of the credits cycle.
				for(INT WidgetIdx=0; WidgetIdx < 2; WidgetIdx++)
				{
					if(PhotoImage[WidgetIdx])
					{
						PhotoImage[WidgetIdx]->Opacity = 0.0f;
					}
				}
			}

			// Update scrolling text
			UpdateCreditsText();
		}
		else
		{
			UGameUISceneClient* SceneClient = UUIInteraction::GetSceneClient();

			if(SceneClient)
			{
				// Close ourselves.
				SceneClient->CloseScene(this);
			}
		}
	}
}




//////////////////////////////////////////////////////////////////////////
// UUTUIStatsList
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUTUIStatsList)

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
void UUTUIStatsList::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner )
{
	Super::Initialize(inOwnerScene, inOwner);
}

/** Uses the currently bound list element provider to generate a set of child option widgets. */
void UUTUIStatsList::RegenerateOptions()
{
	GeneratedObjects.Empty();

	// Only generate options ingame.
	if(GIsGame)
	{
		// Generate new options
		if ( DataSource.ResolveMarkup(this) )
		{
			DataProvider = DataSource->ResolveListElementProvider(DataSource.DataStoreField.ToString());

			if(DataProvider)
			{
				TScriptInterface<IUIListElementCellProvider> SchemaProvider = DataProvider->GetElementCellSchemaProvider(DataSource.DataStoreField);
				TMap<FName,FString> TagMap;
				SchemaProvider->GetElementCellTags(TagMap);
					
				TArray<FName> Tags;
				TArray<FString> Headers;
				TagMap.GenerateKeyArray(Tags);
				TagMap.GenerateValueArray(Headers);

				for(INT TagIdx=0; TagIdx<Tags.Num(); TagIdx++)
				{
					// Create a label for the field.
					UUILabel* KeyObj = Cast<UUILabel>(CreateWidget(this, UUILabel::StaticClass()));
					UUILabel* ValueObj = Cast<UUILabel>(CreateWidget(this, UUILabel::StaticClass()));
					
					if(KeyObj && ValueObj)
					{
						InsertChild(KeyObj);
						InsertChild(ValueObj);

						KeyObj->SetDataStoreBinding(Headers(TagIdx));
						ValueObj->SetDataStoreBinding(TEXT("XXXX"));
						
						// Store referneces to the generated objects.
						FGeneratedStatisticInfo StatInfo;
						StatInfo.DataTag = Tags(TagIdx);
						StatInfo.KeyObj = KeyObj;
						StatInfo.ValueObj = ValueObj;
						GeneratedObjects.AddItem(StatInfo);
					}
				}
			}
		}
	}
}

/** Repositions the previously generated options. */
void UUTUIStatsList::RepositionOptions()
{
	if(GIsGame)
	{
		const FLOAT OptionOffsetPercentage = 0.6f;
		const FLOAT OptionHeight = 32.0f;
		const FLOAT OptionPadding = 8.0f;
		FLOAT OptionY = 0.0f;

		for(INT OptionIdx = 0; OptionIdx<GeneratedObjects.Num(); OptionIdx++)
		{
			UUIObject* KeyObj = GeneratedObjects(OptionIdx).KeyObj;
			UUIObject* ValueObj = GeneratedObjects(OptionIdx).ValueObj;

			// Position Label
			KeyObj->SetPosition(OptionY, UIFACE_Top, EVALPOS_PixelOwner);
			KeyObj->SetPosition(OptionHeight, UIFACE_Bottom, EVALPOS_PixelOwner);

			// Position Widget
			ValueObj->Position.ChangeScaleType(ValueObj, UIFACE_Left, EVALPOS_PercentageOwner);
			ValueObj->Position.ChangeScaleType(ValueObj, UIFACE_Right, EVALPOS_PercentageOwner);
			ValueObj->Position.ChangeScaleType(ValueObj, UIFACE_Top, EVALPOS_PercentageOwner);
			ValueObj->Position.ChangeScaleType(ValueObj, UIFACE_Bottom, EVALPOS_PercentageOwner);

			ValueObj->SetPosition(OptionOffsetPercentage, UIFACE_Left, EVALPOS_PercentageOwner);
			ValueObj->SetPosition(1.0f - OptionOffsetPercentage, UIFACE_Right, EVALPOS_PercentageOwner);
			ValueObj->SetPosition(OptionY, UIFACE_Top, EVALPOS_PixelOwner);
			ValueObj->SetPosition(OptionHeight, UIFACE_Bottom);

			// Increment position
			OptionY += OptionHeight + OptionPadding;
		}
	}
}

/** Sets which result row to get stats values from and then retrieves the stats values. */
void UUTUIStatsList::SetStatsIndex(INT ResultIdx)
{
	if(DataProvider)
	{
		TScriptInterface<IUIListElementCellProvider> ValueProvider = DataProvider->GetElementCellValueProvider(DataSource.DataStoreField, ResultIdx);

		if(ValueProvider)
		{
			for(INT TagIdx=0; TagIdx<GeneratedObjects.Num(); TagIdx++)
			{
				FUIProviderFieldValue FieldValue(EC_EventParm);
				if(ValueProvider->GetCellFieldValue(GeneratedObjects(TagIdx).DataTag, ResultIdx, FieldValue))
				{
					GeneratedObjects(TagIdx).ValueObj->SetDataStoreBinding(FieldValue.StringValue);
				}
			}
		}
	}
}

/**
 * Callback that happens the first time the scene is rendered, any widget positioning initialization should be done here.
 *
 * By default this function recursively calls itself on all of its children.
 */
void UUTUIStatsList::PreRenderCallback()
{
	RepositionOptions();
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
void UUTUIStatsList::SetDataStoreBinding( const FString& MarkupText, INT BindingIndex/*=-1*/ )
{
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		SetDefaultDataBinding(MarkupText,BindingIndex);
	}
	else if ( DataSource.MarkupString != MarkupText )
	{
		Modify();
        DataSource.MarkupString = MarkupText;

		RefreshSubscriberValue(BindingIndex);

		// Regenerate options.
		RegenerateOptions();
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
FString UUTUIStatsList::GetDataStoreBinding( INT BindingIndex/*=-1*/ ) const
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
UBOOL UUTUIStatsList::RefreshSubscriberValue( INT BindingIndex/*=INDEX_NONE*/ )
{
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		return ResolveDefaultDataBinding(BindingIndex);
	}
	else 
		return TRUE;
}

/**
 * Retrieves the list of data stores bound by this subscriber.
 *
 * @param	out_BoundDataStores		receives the array of data stores that subscriber is bound to.
 */
void UUTUIStatsList::GetBoundDataStores( TArray<UUIDataStore*>& out_BoundDataStores )
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
void UUTUIStatsList::ClearBoundDataStores()
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



//////////////////////////////////////////////////////////////////////////
// UUTUIOptionList
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUTUIOptionList)

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
void UUTUIOptionList::Initialize( UUIScene* inOwnerScene, UUIObject* inOwner )
{

	// Since UITabPage used to be a child of UIScrollFrame, we need to clear the NotifyPositionChanged delegate for
	// any children that still have that delegate pointing to OnChildRepositioned.  OnChildRepositioned is a method in
	// UIScrollFrame that doesn't exist in UIPanel
	TArray<UUIObject*> TabPageChildren;
	GetChildren(TabPageChildren, TRUE);

	for ( INT ChildIndex = 0; ChildIndex < TabPageChildren.Num(); ChildIndex++ )
	{
		UUIObject* Child = TabPageChildren(ChildIndex);
		if ( OBJ_DELEGATE_IS_SET(Child,NotifyPositionChanged) )
		{
			FScriptDelegate& Delegate = Child->__NotifyPositionChanged__Delegate;
			if ( Delegate.FunctionName == TEXT("OnChildRepositioned") && Delegate.Object == this )
			{
				Delegate.FunctionName = NAME_None;
				Delegate.Object = NULL;
			}
		}
	}

	Super::Initialize(inOwnerScene, inOwner);

}

/** Uses the currently bound list element provider to generate a set of child option widgets. */
void UUTUIOptionList::RegenerateOptions()
{
	GeneratedObjects.Empty();

	// Only generate options ingame.
	if(GIsGame)
	{
		// Remove all children.
		RemoveChildren(Children);

		// Generate new options
		if ( DataSource.ResolveMarkup(this) )
		{
			DataProvider = DataSource->ResolveListElementProvider(DataSource.DataStoreField.ToString());

			if(DataProvider)
			{
				TArray<INT> ListElements;
				DataProvider->GetListElements(DataSource.DataStoreField, ListElements);


					
				for(INT ElementIdx=0; ElementIdx<ListElements.Num(); ElementIdx++)
				{
					TScriptInterface<IUIListElementCellProvider> ElementProvider = DataProvider->GetElementCellValueProvider(DataSource.DataStoreField, ListElements(ElementIdx));

					if(ElementProvider)
					{
						UUTUIDataProvider_MenuOption* OptionProvider = Cast<UUTUIDataProvider_MenuOption>(ElementProvider->GetUObjectInterfaceUIListElementCellProvider());

						if(OptionProvider)
						{

							// Create a label for the option.
							UUIObject* NewOptionObj = NULL;
							UUIObject* NewOptionLabelObject = CreateWidget(this, UUILabel::StaticClass());
							UUILabel* NewOptionLabel = Cast<UUILabel>(NewOptionLabelObject);
							
							if(NewOptionLabel)
							{
								InsertChild(NewOptionLabel);
								NewOptionLabel->SetDataStoreBinding(OptionProvider->FriendlyName);
								NewOptionLabel->SetEnabled(FALSE);
								NewOptionLabel->StringRenderComponent->StringStyle.DefaultStyleTag = TEXT("OptionListLabel");
							}

							switch((EUTOptionType)OptionProvider->OptionType)
							{
							case UTOT_Spinner:
							#if !CONSOLE
								{
									UUTUINumericEditBox* NewOption = Cast<UUTUINumericEditBox>(CreateWidget(this, UUTUINumericEditBox::StaticClass()));

									if(NewOption)
									{	
										NewOptionObj = NewOption;
										NewOptionObj->TabIndex = ElementIdx;
										InsertChild(NewOption);

										NewOption->NumericValue = OptionProvider->RangeData;
										NewOption->SetDataStoreBinding(OptionProvider->DataStoreMarkup);
									}	
								}
								break;
							#endif
							case UTOT_Slider:
								{
									UUTUISlider* NewOption = Cast<UUTUISlider>(CreateWidget(this, UUTUISlider::StaticClass()));
					
									if(NewOption)
									{	
										NewOptionObj = NewOption;
										NewOptionObj->TabIndex = ElementIdx;
										InsertChild(NewOption);

										NewOption->SliderValue = OptionProvider->RangeData;
										NewOption->SetDataStoreBinding(OptionProvider->DataStoreMarkup);
									}				
								}
								break;
							case UTOT_EditBox:
							#if !CONSOLE
								{
									UUTUIEditBox* NewOption = Cast<UUTUIEditBox>(CreateWidget(this, UUTUIEditBox::StaticClass()));

									if(NewOption)
									{	
										NewOptionObj = NewOption;
										NewOptionObj->TabIndex = ElementIdx;
										InsertChild(NewOption);
										NewOption->SetDataStoreBinding(OptionProvider->DataStoreMarkup);
									}	
								}
								break;
							#endif
							default:
								{
									// If we are on Console, create an option button, otherwise create a combobox.
									 //#if CONSOLE
										UUTUIOptionButton* NewOption = Cast<UUTUIOptionButton>(CreateWidget(this, UUTUIOptionButton::StaticClass()));

										if(NewOption)
										{	
											NewOptionObj = NewOption;
											NewOptionObj->TabIndex = ElementIdx;
											InsertChild(NewOption);

											NewOption->SetDataStoreBinding(OptionProvider->DataStoreMarkup);
										}							
									/*#else
										UUTUIComboBox* NewOption = Cast<UUTUIComboBox>(CreateWidget(this, UUTUIComboBox::StaticClass()));

										if(NewOption)
										{	
											NewOptionObj = NewOption;
											NewOptionObj->TabIndex = ElementIdx;
											InsertChild(NewOption);


											NewOption->ComboList->RowHeight.SetValue(NewOption->ComboList, 28.0f, EVALPOS_PixelViewport);
											NewOption->ComboList->SetDataStoreBinding(OptionProvider->DataStoreMarkup);
											NewOption->SetupChildStyles();	// Need to call this to set the default combobox value since we changed the markup for the list.
											NewOption->ComboEditbox->bReadOnly = TRUE;
											NewOption->ComboList->EnableColumnHeaderRendering(FALSE);
										}
									#endif*/
								}
								break;
							}

							// Store a reference to the new object
							if(NewOptionObj)
							{
							    FGeneratedObjectInfo OptionInfo;
								OptionInfo.LabelObj = NewOptionLabelObject;
								OptionInfo.OptionObj = NewOptionObj;
								OptionInfo.OptionProviderName = OptionProvider->GetFName();
								OptionInfo.OptionProvider = OptionProvider;
								GeneratedObjects.AddItem(OptionInfo);

								NewOptionLabelObject->SetEnabled(FALSE);
							}
						}
					}
				}

				// Make focus for the first and last objects wrap around.
				if(GeneratedObjects.Num())
				{
					UUIObject* FirstWidget = GeneratedObjects(0).OptionObj;
					UUIObject* LastWidget = GeneratedObjects(GeneratedObjects.Num()-1).OptionObj;

					if(FirstWidget && LastWidget)
					{
						FirstWidget->SetForcedNavigationTarget(UIFACE_Top, LastWidget);
						LastWidget->SetForcedNavigationTarget(UIFACE_Bottom, FirstWidget);
					}
				}
			}
		}

		// Instance a prefab BG.
		if(BGPrefab)
		{
			BGPrefabInstance = BGPrefab->InstancePrefab(Owner, TEXT("BGPrefab"));

			if(BGPrefabInstance!=NULL)
			{
				// Set some private behavior for the prefab.
				BGPrefabInstance->SetPrivateBehavior(UCONST_PRIVATE_NotEditorSelectable, TRUE);
				BGPrefabInstance->SetPrivateBehavior(UCONST_PRIVATE_TreeHiddenRecursive, TRUE);

				// Add the prefab to the list.
				InsertChild(BGPrefabInstance,0);
			}
		}
	}
}

/** Repositions the previously generated options. */
void UUTUIOptionList::RepositionOptions()
{
	if(GIsGame)
	{
		const FLOAT AnimationTime = 0.1f;
		const FLOAT OptionOffsetPercentage = 0.6f;
		const FLOAT OptionHeight = 32.0f;
		const FLOAT OptionPadding = 8.0f;
		const FLOAT FinalOptionHeight = OptionPadding+OptionHeight;
		FLOAT OptionY = 0.0f;

		// Max number of visible options
		INT MaxVisible = appFloor(GetBounds(UIORIENT_Vertical, EVALPOS_PixelViewport) / FinalOptionHeight);
		INT MiddleItem = MaxVisible / 2;
		INT TopItem = 0;
		INT OldTopItem = 0;

		if(GeneratedObjects.Num() > MaxVisible)
		{
			// Calculate old top item
			if(PreviousIndex > MiddleItem)
			{
				OldTopItem = PreviousIndex - MiddleItem;
			}
			OldTopItem = Clamp<INT>(OldTopItem, 0, GeneratedObjects.Num()-MaxVisible);

			// Calculate current top item
			if(CurrentIndex > MiddleItem)
			{
				TopItem = CurrentIndex - MiddleItem;
			}
			TopItem = Clamp<INT>(TopItem, 0, GeneratedObjects.Num()-MaxVisible);
		}

		// Loop through all generated objects and reposition them.
		FLOAT InterpTop = 1.0f;

		if(bAnimatingBGPrefab)
		{
			FLOAT TimeElapsed = GWorld->GetRealTimeSeconds() - StartMovementTime;
			if(TimeElapsed > AnimationTime)
			{
				TimeElapsed = AnimationTime;
			}
			
			InterpTop = TimeElapsed / AnimationTime;

			// Ease In
			InterpTop = 1.0f - InterpTop;
			InterpTop = appPow(InterpTop, 2);
			InterpTop = 1.0f - InterpTop;
		}

		// Animate list movement 
		InterpTop = InterpTop * (TopItem-OldTopItem) + OldTopItem;
		OptionY = -InterpTop * FinalOptionHeight;
		const FLOAT FadeDist = 1.0f;

		for(INT OptionIdx = 0; OptionIdx<GeneratedObjects.Num(); OptionIdx++)
		{
			UUIObject* NewOptionObj = GeneratedObjects(OptionIdx).OptionObj;
			UUIObject* NewOptionLabelObject = GeneratedObjects(OptionIdx).LabelObj;
			FLOAT WidgetOpacity = 1.0f;
			
			// Calculate opacity for elements off the visible area of the screen.
			FLOAT CurrentRelativeTop = OptionIdx - InterpTop;
			if(CurrentRelativeTop < 0.0f)
			{
				CurrentRelativeTop = Max<FLOAT>(CurrentRelativeTop, -FadeDist);
				WidgetOpacity = 1.0f - CurrentRelativeTop / -FadeDist;
			}
			else if(CurrentRelativeTop - MaxVisible + 1 > 0.0f)
			{
				CurrentRelativeTop = Min<FLOAT>(CurrentRelativeTop - MaxVisible + 1, FadeDist);
				WidgetOpacity = 1.0f - CurrentRelativeTop / FadeDist;
			}

			// Position Label
			NewOptionLabelObject->Opacity = WidgetOpacity;
			NewOptionLabelObject->SetPosition(0.025f, UIFACE_Left, EVALPOS_PercentageOwner);
			NewOptionLabelObject->SetPosition(OptionY, UIFACE_Top, EVALPOS_PixelOwner);
			NewOptionLabelObject->SetPosition(OptionHeight, UIFACE_Bottom, EVALPOS_PixelOwner);

			// Position Widget
			NewOptionObj->Opacity = WidgetOpacity;
			NewOptionObj->Position.ChangeScaleType(NewOptionObj, UIFACE_Left, EVALPOS_PercentageOwner);
			NewOptionObj->Position.ChangeScaleType(NewOptionObj, UIFACE_Right, EVALPOS_PercentageOwner);
			NewOptionObj->Position.ChangeScaleType(NewOptionObj, UIFACE_Top, EVALPOS_PercentageOwner);
			NewOptionObj->Position.ChangeScaleType(NewOptionObj, UIFACE_Bottom, EVALPOS_PercentageOwner);

			NewOptionObj->SetPosition(OptionOffsetPercentage, UIFACE_Left, EVALPOS_PercentageOwner);
			NewOptionObj->SetPosition(1.0f - OptionOffsetPercentage, UIFACE_Right, EVALPOS_PercentageOwner);
			NewOptionObj->SetPosition(OptionY, UIFACE_Top, EVALPOS_PixelOwner);
			NewOptionObj->SetPosition(OptionHeight, UIFACE_Bottom);

			// Increment position
			OptionY += FinalOptionHeight;
		}

		// Position the background prefab.
		if(BGPrefabInstance)
		{
			FLOAT TopFace = GeneratedObjects.IsValidIndex(CurrentIndex) ? GeneratedObjects(CurrentIndex).OptionObj->GetPosition(UIFACE_Top,EVALPOS_PixelOwner) : 0.0f;

			if(bAnimatingBGPrefab)
			{
				FLOAT Distance = GeneratedObjects(CurrentIndex).OptionObj->GetPosition(UIFACE_Top,EVALPOS_PixelOwner) - 
					GeneratedObjects(PreviousIndex).OptionObj->GetPosition(UIFACE_Top,EVALPOS_PixelOwner);
				FLOAT TimeElapsed = GWorld->GetRealTimeSeconds() - StartMovementTime;
				if(TimeElapsed > AnimationTime)
				{
					bAnimatingBGPrefab = FALSE;
					TimeElapsed = AnimationTime;
				}

				// Used to interpolate the scaling to create a smooth looking selection effect.
				FLOAT Alpha = 1.0f - (TimeElapsed / AnimationTime);
			
				// Ease in
				Alpha = appPow(Alpha,2);

				TopFace += -Distance*Alpha;
			}


			// Make the entire prefab percentage owner
			BGPrefabInstance->Position.ChangeScaleType(Owner, UIFACE_Top, EVALPOS_PercentageOwner);
			BGPrefabInstance->Position.ChangeScaleType(Owner, UIFACE_Bottom, EVALPOS_PercentageOwner);
			BGPrefabInstance->Position.ChangeScaleType(Owner, UIFACE_Left, EVALPOS_PercentageOwner);
			BGPrefabInstance->Position.ChangeScaleType(Owner, UIFACE_Right, EVALPOS_PercentageOwner);

			BGPrefabInstance->SetPosition(0.0f, UIFACE_Left, EVALPOS_PercentageOwner);
			BGPrefabInstance->SetPosition(1.0f, UIFACE_Right, EVALPOS_PercentageOwner);
			BGPrefabInstance->SetPosition(TopFace-OptionPadding/2.0f, UIFACE_Top, EVALPOS_PixelOwner);
			BGPrefabInstance->SetPosition(FinalOptionHeight, UIFACE_Bottom);
		}

		RequestSceneUpdate(FALSE,TRUE,FALSE,TRUE);
	}
}

void UUTUIOptionList::SetSelectedOptionIndex(INT OptionIdx)
{
	PreviousIndex = CurrentIndex;
	CurrentIndex = OptionIdx;

	// Change widget state
	if(GeneratedObjects.IsValidIndex(PreviousIndex) && GeneratedObjects(PreviousIndex).LabelObj)
	{
		GeneratedObjects(PreviousIndex).LabelObj->SetEnabled(FALSE);
	}

	if(GeneratedObjects.IsValidIndex(CurrentIndex) && GeneratedObjects(CurrentIndex).LabelObj)
	{
		GeneratedObjects(CurrentIndex).LabelObj->SetEnabled(TRUE);
	}

	bAnimatingBGPrefab = true;
	StartMovementTime = GWorld->GetRealTimeSeconds();

	RepositionOptions();
}

void UUTUIOptionList::Tick_Widget(FLOAT DeltaTime)
{
	if(bAnimatingBGPrefab)
	{
		RepositionOptions();
	}
}

// UObject interface
void UUTUIOptionList::GetSupportedUIActionKeyNames(TArray<FName> &out_KeyNames)
{
	Super::GetSupportedUIActionKeyNames(out_KeyNames);

	out_KeyNames.AddItem(UIKEY_Clicked);
}


/**
 * Callback that happens the first time the scene is rendered, any widget positioning initialization should be done here.
 *
 * By default this function recursively calls itself on all of its children.
 */
void UUTUIOptionList::PreRenderCallback()
{
	RepositionOptions();
}

/**
 * Handles input events for this button.
 *
 * @param	EventParms		the parameters for the input event
 *
 * @return	TRUE to consume the key event, FALSE to pass it on.
 */
UBOOL UUTUIOptionList::ProcessInputKey( const FSubscribedInputEventParameters& EventParms )
{
	UBOOL bResult = FALSE;

	// Accept Options Event
	if ( EventParms.InputAliasName == UIKEY_Clicked )
	{
		if ( EventParms.EventType == IE_Released )
		{
			if ( DELEGATE_IS_SET(OnAcceptOptions) )
			{
				delegateOnAcceptOptions(this, EventParms.PlayerIndex);
			}

			bResult = TRUE;
		}
		else if(EventParms.EventType == IE_Pressed || EventParms.EventType == IE_DoubleClick )
		{
			bResult = TRUE;
		}
	}

	// Make sure to call the superclass's implementation after trying to consume input ourselves so that
	// we can respond to events defined in the super's class.
	bResult = bResult || Super::ProcessInputKey(EventParms);
	return bResult;
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
void UUTUIOptionList::SetDataStoreBinding( const FString& MarkupText, INT BindingIndex/*=-1*/ )
{
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		SetDefaultDataBinding(MarkupText,BindingIndex);
	}
	else if ( DataSource.MarkupString != MarkupText )
	{
		Modify();
        DataSource.MarkupString = MarkupText;

		RefreshSubscriberValue(BindingIndex);

		// Regenerate options.
		RegenerateOptions();
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
FString UUTUIOptionList::GetDataStoreBinding( INT BindingIndex/*=-1*/ ) const
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
UBOOL UUTUIOptionList::RefreshSubscriberValue( INT BindingIndex/*=INDEX_NONE*/ )
{
	if ( BindingIndex >= UCONST_FIRST_DEFAULT_DATABINDING_INDEX )
	{
		return ResolveDefaultDataBinding(BindingIndex);
	}
	else 
		return TRUE;
}

/**
 * Retrieves the list of data stores bound by this subscriber.
 *
 * @param	out_BoundDataStores		receives the array of data stores that subscriber is bound to.
 */
void UUTUIOptionList::GetBoundDataStores( TArray<UUIDataStore*>& out_BoundDataStores )
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
void UUTUIOptionList::ClearBoundDataStores()
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


//////////////////////////////////////////////////////////////////////////
// UUTUIKeyBindingList
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CLASS(UUTUIKeyBindingList)

/** Uses the currently bound list element provider to generate a set of child option widgets. */
void UUTUIKeyBindingList::RegenerateOptions()
{
	GeneratedObjects.Empty();

	// Only generate options ingame.
	if(GIsGame)
	{
		// Generate new options
		if ( DataSource.ResolveMarkup(this) )
		{
			DataProvider = DataSource->ResolveListElementProvider(DataSource.DataStoreField.ToString());

			if(DataProvider)
			{
				TArray<INT> ListElements;
				DataProvider->GetListElements(DataSource.DataStoreField, ListElements);

				for(INT ElementIdx=0; ElementIdx<ListElements.Num(); ElementIdx++)
				{
					TScriptInterface<IUIListElementCellProvider> ElementProvider = DataProvider->GetElementCellValueProvider(DataSource.DataStoreField, ListElements(ElementIdx));

					if(ElementProvider)
					{
						UUTUIDataProvider_KeyBinding* OptionProvider = Cast<UUTUIDataProvider_KeyBinding>(ElementProvider->GetUObjectInterfaceUIListElementCellProvider());

						if(OptionProvider)
						{
							// Create a label for the option.
							UUIObject* NewOptionLabelObject = CreateWidget(this, UUILabel::StaticClass());
							UUILabel* NewOptionLabel = Cast<UUILabel>(NewOptionLabelObject);
							
							if(NewOptionLabel)
							{
								InsertChild(NewOptionLabel);
								NewOptionLabel->SetEnabled(FALSE);
								NewOptionLabel->SetDataStoreBinding(OptionProvider->FriendlyName);
							}

							for(INT NewObjIdx=0; NewObjIdx<NumButtons; NewObjIdx++)
							{
								UUIObject* NewOptionObj = NULL;
								UUILabelButton* NewOption = NULL;
								
								NewOption = Cast<UUILabelButton>(CreateWidget(this, UUILabelButton::StaticClass()));

								if(NewOption)
								{	
									NewOptionObj = NewOption;
									NewOption->TabIndex = GeneratedObjects.Num();

									InsertChild(NewOptionObj);

									// Store a reference to the new object.
									FGeneratedObjectInfo OptionInfo;
									OptionInfo.LabelObj = NewOptionLabelObject;
									OptionInfo.OptionObj = NewOptionObj;
									OptionInfo.OptionProviderName = OptionProvider->GetFName();
									OptionInfo.OptionProvider = OptionProvider;
									GeneratedObjects.AddItem(OptionInfo);
								}
							}
						}
					}
				}
			}

			
			// Instance a prefab BG.
			if(BGPrefab)
			{
				BGPrefabInstance = BGPrefab->InstancePrefab(Owner, TEXT("BGPrefab"));

				if(BGPrefabInstance!=NULL)
				{
					// Set some private behavior for the prefab.
					BGPrefabInstance->SetPrivateBehavior(UCONST_PRIVATE_NotEditorSelectable, TRUE);
					BGPrefabInstance->SetPrivateBehavior(UCONST_PRIVATE_TreeHiddenRecursive, TRUE);

					// Add the prefab to the list.
					InsertChild(BGPrefabInstance,0);
				}
			}

			// Refreshes the binding labels for all of the binding buttons we generated.
			RefreshBindingLabels();


			const INT TotalBindings = GeneratedObjects.Num() / NumButtons;

			// Make options wrap left/right
			for(INT ElementIdx=0; ElementIdx<TotalBindings; ElementIdx++)
			{
				INT ButtonIdx = ElementIdx*NumButtons;

				UUIObject* FirstObject = GeneratedObjects(ButtonIdx).OptionObj;
				UUIObject* LastObject = GeneratedObjects(ButtonIdx+NumButtons-1).OptionObj;

				FirstObject->SetForcedNavigationTarget(UIFACE_Left, LastObject);
				LastObject->SetForcedNavigationTarget(UIFACE_Right, FirstObject);
			}

			// Make options wrap up/down
			INT LastButtonIdx = (TotalBindings-1)*NumButtons;

			for(INT ButtonIdx=0; ButtonIdx<NumButtons; ButtonIdx++)
			{
				UUIObject* FirstObject = GeneratedObjects(ButtonIdx).OptionObj;
				UUIObject* LastObject = GeneratedObjects(LastButtonIdx+ButtonIdx).OptionObj;

				FirstObject->SetForcedNavigationTarget(UIFACE_Top, LastObject);
				LastObject->SetForcedNavigationTarget(UIFACE_Bottom, FirstObject);
			}
		}
	}
}


/** Repositions the previously generated options. */
void UUTUIKeyBindingList::RepositionOptions()
{
	if(GIsGame)
	{
		const FLOAT AnimationTime = 0.1f;
		const FLOAT OptionOffsetPercentage = 0.6f;
		const FLOAT OptionButtonPadding = 0.025f;
		const FLOAT OptionWidth = (1.0f - OptionOffsetPercentage) / NumButtons - OptionButtonPadding * (NumButtons-1);
		const FLOAT OptionHeight = 32.0f;
		const FLOAT OptionPadding = 8.0f;
		FLOAT OptionY = 0.0f;

		// Max number of visible options
		INT MaxVisible = appFloor(GetBounds(UIORIENT_Vertical, EVALPOS_PixelViewport) / (OptionHeight+OptionPadding))-1;
		INT MiddleItem = MaxVisible / 2;
		INT TopItem = 0;
		INT OldTopItem = 0;
		INT RealCurrentIndex = CurrentIndex/NumButtons;
		INT RealPreviousIndex = PreviousIndex/NumButtons;
		INT NumElements = GeneratedObjects.Num() / NumButtons;
		if(NumElements > MaxVisible)
		{
			// Calculate old top item
			if(RealPreviousIndex > MiddleItem)
			{
				OldTopItem = RealPreviousIndex - MiddleItem;
			}
			OldTopItem = Clamp<INT>(OldTopItem, 0, NumElements-MaxVisible);

			// Calculate current top item
			if(RealCurrentIndex > MiddleItem)
			{
				TopItem = RealCurrentIndex - MiddleItem;
			}
			TopItem = Clamp<INT>(TopItem, 0, NumElements-MaxVisible);
		}

		// Loop through all generated objects and reposition them.
		FLOAT InterpTop = 1.0f;

		if(bAnimatingBGPrefab)
		{
			FLOAT TimeElapsed = GWorld->GetRealTimeSeconds() - StartMovementTime;
			if(TimeElapsed > AnimationTime)
			{
				TimeElapsed = AnimationTime;
			}

			InterpTop = TimeElapsed / AnimationTime;

			// Ease In
			InterpTop = 1.0f - InterpTop;
			InterpTop = appPow(InterpTop, 2);
			InterpTop = 1.0f - InterpTop;
		}

		// Animate list movement 
		InterpTop = InterpTop * (TopItem-OldTopItem) + OldTopItem;
		OptionY = -InterpTop * (OptionHeight + OptionPadding);

		for(INT OptionIdx = 0; OptionIdx<GeneratedObjects.Num(); OptionIdx++)
		{
			UUIObject* NewOptionObj = GeneratedObjects(OptionIdx).OptionObj;
			UUIObject* NewOptionLabelObject = GeneratedObjects(OptionIdx).LabelObj;
			FLOAT WidgetOpacity = 1.0f;

			// Calculate opacity for elements off the visible area of the screen.
			INT RealIdx = OptionIdx / NumButtons;
			FLOAT CurrentRelativeTop = RealIdx - InterpTop;
			const FLOAT FadeDist = 1.0f;

			if(CurrentRelativeTop < 0.0f)
			{
				CurrentRelativeTop = Max<FLOAT>(CurrentRelativeTop, -FadeDist);
				WidgetOpacity = 1.0f - CurrentRelativeTop / -FadeDist;
			}
			else if(CurrentRelativeTop - MaxVisible + 1 > 0.0f)
			{
				CurrentRelativeTop = Min<FLOAT>(CurrentRelativeTop - MaxVisible + 1, FadeDist);
				WidgetOpacity = 1.0f - CurrentRelativeTop / FadeDist;
			}

			// Set the visibility of the object to FALSE if its basically invisible.
			NewOptionObj->eventSetVisibility(WidgetOpacity > KINDA_SMALL_NUMBER);
			NewOptionLabelObject->eventSetVisibility(WidgetOpacity > KINDA_SMALL_NUMBER);


			// Position Label
			NewOptionLabelObject->Opacity = WidgetOpacity;
			NewOptionLabelObject->SetPosition(OptionY, UIFACE_Top, EVALPOS_PixelOwner);
			NewOptionLabelObject->SetPosition(OptionHeight, UIFACE_Bottom);

			// Position Widget
			NewOptionObj->Opacity = WidgetOpacity;
			NewOptionObj->Position.ChangeScaleType(NewOptionObj, UIFACE_Left, EVALPOS_PercentageOwner);
			NewOptionObj->Position.ChangeScaleType(NewOptionObj, UIFACE_Right, EVALPOS_PercentageOwner);
			NewOptionObj->Position.ChangeScaleType(NewOptionObj, UIFACE_Top, EVALPOS_PercentageOwner);
			NewOptionObj->Position.ChangeScaleType(NewOptionObj, UIFACE_Bottom, EVALPOS_PercentageOwner);

			NewOptionObj->SetPosition(OptionOffsetPercentage + (OptionIdx%NumButtons)*(OptionWidth + OptionButtonPadding), 
				UIFACE_Left, EVALPOS_PercentageOwner);
			NewOptionObj->SetPosition(OptionWidth, UIFACE_Right, EVALPOS_PercentageOwner);
			NewOptionObj->SetPosition(OptionY, UIFACE_Top, EVALPOS_PixelOwner);
			NewOptionObj->SetPosition(OptionHeight, UIFACE_Bottom);

			// Increment position
			if(OptionIdx%NumButtons==(NumButtons-1))
			{
				OptionY += OptionHeight + OptionPadding;
			}
		}

		// Position the background prefab.
		if(BGPrefabInstance)
		{
			FLOAT TopFace = GeneratedObjects.IsValidIndex(CurrentIndex) ? GeneratedObjects(CurrentIndex).OptionObj->GetPosition(UIFACE_Top,EVALPOS_PixelOwner) : 0.0f;

			if(bAnimatingBGPrefab)
			{
				FLOAT Distance = GeneratedObjects(CurrentIndex).OptionObj->GetPosition(UIFACE_Top,EVALPOS_PixelOwner) - 
					GeneratedObjects(PreviousIndex).OptionObj->GetPosition(UIFACE_Top,EVALPOS_PixelOwner);
				FLOAT TimeElapsed = GWorld->GetRealTimeSeconds() - StartMovementTime;
				if(TimeElapsed > AnimationTime)
				{
					bAnimatingBGPrefab = FALSE;
					TimeElapsed = AnimationTime;
				}

				// Used to interpolate the scaling to create a smooth looking selection effect.
				FLOAT Alpha = 1.0f - (TimeElapsed / AnimationTime);

				// Ease in
				Alpha = appPow(Alpha,2);

				TopFace += -Distance*Alpha;
			}


			// Make the entire prefab percentage owner
			BGPrefabInstance->Position.ChangeScaleType(Owner, UIFACE_Top, EVALPOS_PercentageOwner);
			BGPrefabInstance->Position.ChangeScaleType(Owner, UIFACE_Bottom, EVALPOS_PercentageOwner);
			BGPrefabInstance->Position.ChangeScaleType(Owner, UIFACE_Left, EVALPOS_PercentageOwner);
			BGPrefabInstance->Position.ChangeScaleType(Owner, UIFACE_Right, EVALPOS_PercentageOwner);

			BGPrefabInstance->SetPosition(0.0f, UIFACE_Left, EVALPOS_PercentageOwner);
			BGPrefabInstance->SetPosition(1.0f, UIFACE_Right, EVALPOS_PercentageOwner);
			BGPrefabInstance->SetPosition(TopFace-OptionPadding/2.0f, UIFACE_Top, EVALPOS_PixelOwner);
			BGPrefabInstance->SetPosition(OptionHeight+OptionPadding, UIFACE_Bottom);
		}

		RequestSceneUpdate(FALSE,TRUE,FALSE,TRUE);
	}
}


/**
 * Handles input events for this button.
 *
 * @param	EventParms		the parameters for the input event
 *
 * @return	TRUE to consume the key event, FALSE to pass it on.
 */
UBOOL UUTUIKeyBindingList::ProcessInputKey( const FSubscribedInputEventParameters& EventParms )
{
	// @todo: Trap input when the user presses a key.
	return Super::ProcessInputKey(EventParms);
}

/** Refreshes the binding labels for all of the generated objects. */
void UUTUIKeyBindingList::RefreshBindingLabels()
{
	UPlayerInput* PInput = eventGetPlayerInput();
	TMap<FString, INT> StartingIndices;

	if(PInput != NULL)
	{
		// Go through all of the generated objects and update their labels using the currently bound keys in the player input object.
		for(INT ObjectIdx=0; ObjectIdx < GeneratedObjects.Num(); ObjectIdx++)
		{

			UUILabelButton* ButtonObj = Cast<UUILabelButton>(GeneratedObjects(ObjectIdx).OptionObj);
			UUTUIDataProvider_KeyBinding* KeyBindingProvider = Cast<UUTUIDataProvider_KeyBinding>(GeneratedObjects(ObjectIdx).OptionProvider);

			if(KeyBindingProvider && ButtonObj)
			{	
				// See if we already have a starting index for this command.
				const FString CommandStr = KeyBindingProvider->Command;
				INT* SearchIndex = StartingIndices.Find(CommandStr);
				INT StartIndex = -1;

				// If we found a starting index, then use that.
				if(SearchIndex)
				{
					StartIndex = *SearchIndex;
				}
				
				// If our start index is different from our old index then we found a command.
				FString BindingKey = PInput->GetBindNameFromCommand(CommandStr, &StartIndex );
				if(StartIndex != -1)
				{
					ButtonObj->SetDataStoreBinding(BindingKey);

					// Store the last starting index so if we have this command again we will search from where we left off.
					StartingIndices.Set(*CommandStr, StartIndex - 1);
				}
				else	// No bound key found, set the datastore binding accordingly
				{
					ButtonObj->SetDataStoreBinding(TEXT("<Strings:UTGameUI.Settings.NotBound>"));
				}
			}
		}
	}
}





