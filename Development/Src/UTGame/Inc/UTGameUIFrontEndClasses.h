/*===========================================================================
    C++ class definitions exported from UnrealScript.
    This is automatically generated by the tools.
    DO NOT modify this manually! Edit the corresponding .uc files instead!
===========================================================================*/
#if SUPPORTS_PRAGMA_PACK
#pragma pack (push,4)
#endif


// Split enums from the rest of the header so they can be included earlier
// than the rest of the header file by including this file twice with different
// #define wrappers. See Engine.h and look at EngineClasses.h for an example.
#if !NO_ENUMS && !defined(NAMES_ONLY)

enum TabAnimDir
{
    TAD_None                =0,
    TAD_LeftIn              =1,
    TAD_LeftOut             =2,
    TAD_RightOut            =3,
    TAD_RightIn             =4,
    TAD_MAX                 =5,
};

#endif // !NO_ENUMS

#if !ENUMS_ONLY

#ifndef NAMES_ONLY
#define AUTOGENERATE_NAME(name) extern FName UTGAME_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name)
#endif

AUTOGENERATE_NAME(GetNumLoggedInPlayers)
AUTOGENERATE_NAME(GetPlayerInput)
AUTOGENERATE_NAME(OnAcceptOptions)
AUTOGENERATE_NAME(OnOptionChanged)
AUTOGENERATE_NAME(OnOptionFocused)
AUTOGENERATE_NAME(OnPartSelected)
AUTOGENERATE_NAME(OnPreviewPartChanged)
AUTOGENERATE_NAME(OnUpdateAnim)
AUTOGENERATE_NAME(UpdateLoadingPackage)
AUTOGENERATE_NAME(UpdateProfileLabels)

#ifndef NAMES_ONLY

struct FGeneratedStatisticInfo
{
    FName DataTag;
    class UUILabel* KeyObj;
    class UUILabel* ValueObj;
};

class UUTUIStatsList : public UUIScrollFrame, public IUIDataStoreSubscriber
{
public:
    //## BEGIN PROPS UTUIStatsList
    TArrayNoInit<struct FGeneratedStatisticInfo> GeneratedObjects;
    struct FUIDataStoreBinding DataSource;
    TScriptInterface<class IUIListElementProvider> DataProvider;
    //## END PROPS UTUIStatsList

    virtual void RegenerateOptions();
    virtual void RepositionOptions();
    virtual void SetStatsIndex(INT ResultIdx);
    virtual void SetDataStoreBinding(const FString& MarkupText,INT BindingIndex=-1);
    virtual FString GetDataStoreBinding(INT BindingIndex=-1) const;
    virtual UBOOL RefreshSubscriberValue(INT BindingIndex=-1);
    virtual void GetBoundDataStores(TArray<class UUIDataStore*>& out_BoundDataStores);
    virtual void ClearBoundDataStores();
    DECLARE_FUNCTION(execRegenerateOptions)
    {
        P_FINISH;
        RegenerateOptions();
    }
    DECLARE_FUNCTION(execRepositionOptions)
    {
        P_FINISH;
        RepositionOptions();
    }
    DECLARE_FUNCTION(execSetStatsIndex)
    {
        P_GET_INT(ResultIdx);
        P_FINISH;
        SetStatsIndex(ResultIdx);
    }
    DECLARE_FUNCTION(execSetDataStoreBinding)
    {
        P_GET_STR(MarkupText);
        P_GET_INT_OPTX(BindingIndex,-1);
        P_FINISH;
        SetDataStoreBinding(MarkupText,BindingIndex);
    }
    DECLARE_FUNCTION(execGetDataStoreBinding)
    {
        P_GET_INT_OPTX(BindingIndex,-1);
        P_FINISH;
        *(FString*)Result=GetDataStoreBinding(BindingIndex);
    }
    DECLARE_FUNCTION(execRefreshSubscriberValue)
    {
        P_GET_INT_OPTX(BindingIndex,-1);
        P_FINISH;
        *(UBOOL*)Result=RefreshSubscriberValue(BindingIndex);
    }
    DECLARE_FUNCTION(execGetBoundDataStores)
    {
        P_GET_TARRAY_REF(class UUIDataStore*,out_BoundDataStores);
        P_FINISH;
        GetBoundDataStores(out_BoundDataStores);
    }
    DECLARE_FUNCTION(execClearBoundDataStores)
    {
        P_FINISH;
        ClearBoundDataStores();
    }
    DECLARE_CLASS(UUTUIStatsList,UUIScrollFrame,0,UTGame)
    virtual UObject* GetUObjectInterfaceUIDataStoreSubscriber(){return this;}
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
	 * Callback that happens the first time the scene is rendered, any widget positioning initialization should be done here.
	 *
	 * By default this function recursively calls itself on all of its children.
	 */
	virtual void PreRenderCallback();
};

struct UTUITabPage_CharacterPart_eventOnUpdateAnim_Parms
{
    FLOAT DeltaTime;
    UTUITabPage_CharacterPart_eventOnUpdateAnim_Parms(EEventParm)
    {
    }
};
struct UTUITabPage_CharacterPart_eventOnPreviewPartChanged_Parms
{
    BYTE PartType;
    FString InPartID;
    UTUITabPage_CharacterPart_eventOnPreviewPartChanged_Parms(EEventParm)
    {
    }
};
struct UTUITabPage_CharacterPart_eventOnPartSelected_Parms
{
    BYTE PartType;
    FString InPartID;
    UTUITabPage_CharacterPart_eventOnPartSelected_Parms(EEventParm)
    {
    }
};
class UUTUITabPage_CharacterPart : public UUTTabPage
{
public:
    //## BEGIN PROPS UTUITabPage_CharacterPart
    FVector OffsetVector;
    FLOAT AnimTimeElapsed;
    BYTE AnimDir;
    BYTE CharPartType;
    class UUTUIMenuList* PartList;
    FScriptDelegate __OnPartSelected__Delegate;
    FScriptDelegate __OnPreviewPartChanged__Delegate;
    //## END PROPS UTUITabPage_CharacterPart

    void eventOnUpdateAnim(FLOAT DeltaTime)
    {
        UTUITabPage_CharacterPart_eventOnUpdateAnim_Parms Parms(EC_EventParm);
        Parms.DeltaTime=DeltaTime;
        ProcessEvent(FindFunctionChecked(UTGAME_OnUpdateAnim),&Parms);
    }
    void delegateOnPreviewPartChanged(BYTE PartType,const FString& InPartID)
    {
        UTUITabPage_CharacterPart_eventOnPreviewPartChanged_Parms Parms(EC_EventParm);
        Parms.PartType=PartType;
        Parms.InPartID=InPartID;
        ProcessDelegate(UTGAME_OnPreviewPartChanged,&__OnPreviewPartChanged__Delegate,&Parms);
    }
    void delegateOnPartSelected(BYTE PartType,const FString& InPartID)
    {
        UTUITabPage_CharacterPart_eventOnPartSelected_Parms Parms(EC_EventParm);
        Parms.PartType=PartType;
        Parms.InPartID=InPartID;
        ProcessDelegate(UTGAME_OnPartSelected,&__OnPartSelected__Delegate,&Parms);
    }
    DECLARE_CLASS(UUTUITabPage_CharacterPart,UUTTabPage,0,UTGame)
	virtual void Tick_Widget(FLOAT DeltaTime)
	{
		if(AnimDir != TAD_None)
		{
			eventOnUpdateAnim(DeltaTime);
		}
	}

	/**
	 * Routes rendering calls to children of this screen object.
	 *
	 * @param	Canvas	the canvas to use for rendering
	 */
	virtual void Render_Children( FCanvas* Canvas );
};

struct FGeneratedObjectInfo
{
    FName OptionProviderName;
    class UUIObject* LabelObj;
    class UUIObject* OptionObj;
    class UUIDataProvider* OptionProvider;
};

struct UTUIOptionList_eventOnAcceptOptions_Parms
{
    class UUIScreenObject* InObject;
    INT PlayerIndex;
    UTUIOptionList_eventOnAcceptOptions_Parms(EEventParm)
    {
    }
};
struct UTUIOptionList_eventOnOptionChanged_Parms
{
    class UUIScreenObject* InObject;
    FName OptionName;
    INT PlayerIndex;
    UTUIOptionList_eventOnOptionChanged_Parms(EEventParm)
    {
    }
};
struct UTUIOptionList_eventOnOptionFocused_Parms
{
    class UUIScreenObject* InObject;
    class UUIDataProvider* OptionProvider;
    UTUIOptionList_eventOnOptionFocused_Parms(EEventParm)
    {
    }
};
class UUTUIOptionList : public UUTUI_Widget, public IUIDataStoreSubscriber
{
public:
    //## BEGIN PROPS UTUIOptionList
    INT CurrentIndex;
    INT PreviousIndex;
    FLOAT StartMovementTime;
    BITFIELD bAnimatingBGPrefab:1;
    TArrayNoInit<struct FGeneratedObjectInfo> GeneratedObjects;
    struct FUIDataStoreBinding DataSource;
    TScriptInterface<class IUIListElementProvider> DataProvider;
    class UUIPrefab* BGPrefab;
    class UUIPrefabInstance* BGPrefabInstance;
    FScriptDelegate __OnOptionFocused__Delegate;
    FScriptDelegate __OnOptionChanged__Delegate;
    FScriptDelegate __OnAcceptOptions__Delegate;
    //## END PROPS UTUIOptionList

    virtual void RegenerateOptions();
    virtual void RepositionOptions();
    virtual void SetSelectedOptionIndex(INT OptionIdx);
    virtual void SetDataStoreBinding(const FString& MarkupText,INT BindingIndex=-1);
    virtual FString GetDataStoreBinding(INT BindingIndex=-1) const;
    virtual UBOOL RefreshSubscriberValue(INT BindingIndex=-1);
    virtual void GetBoundDataStores(TArray<class UUIDataStore*>& out_BoundDataStores);
    virtual void ClearBoundDataStores();
    DECLARE_FUNCTION(execRegenerateOptions)
    {
        P_FINISH;
        RegenerateOptions();
    }
    DECLARE_FUNCTION(execRepositionOptions)
    {
        P_FINISH;
        RepositionOptions();
    }
    DECLARE_FUNCTION(execSetSelectedOptionIndex)
    {
        P_GET_INT(OptionIdx);
        P_FINISH;
        SetSelectedOptionIndex(OptionIdx);
    }
    DECLARE_FUNCTION(execSetDataStoreBinding)
    {
        P_GET_STR(MarkupText);
        P_GET_INT_OPTX(BindingIndex,-1);
        P_FINISH;
        SetDataStoreBinding(MarkupText,BindingIndex);
    }
    DECLARE_FUNCTION(execGetDataStoreBinding)
    {
        P_GET_INT_OPTX(BindingIndex,-1);
        P_FINISH;
        *(FString*)Result=GetDataStoreBinding(BindingIndex);
    }
    DECLARE_FUNCTION(execRefreshSubscriberValue)
    {
        P_GET_INT_OPTX(BindingIndex,-1);
        P_FINISH;
        *(UBOOL*)Result=RefreshSubscriberValue(BindingIndex);
    }
    DECLARE_FUNCTION(execGetBoundDataStores)
    {
        P_GET_TARRAY_REF(class UUIDataStore*,out_BoundDataStores);
        P_FINISH;
        GetBoundDataStores(out_BoundDataStores);
    }
    DECLARE_FUNCTION(execClearBoundDataStores)
    {
        P_FINISH;
        ClearBoundDataStores();
    }
    void delegateOnAcceptOptions(class UUIScreenObject* InObject,INT PlayerIndex)
    {
        UTUIOptionList_eventOnAcceptOptions_Parms Parms(EC_EventParm);
        Parms.InObject=InObject;
        Parms.PlayerIndex=PlayerIndex;
        ProcessDelegate(UTGAME_OnAcceptOptions,&__OnAcceptOptions__Delegate,&Parms);
    }
    void delegateOnOptionChanged(class UUIScreenObject* InObject,FName OptionName,INT PlayerIndex)
    {
        UTUIOptionList_eventOnOptionChanged_Parms Parms(EC_EventParm);
        Parms.InObject=InObject;
        Parms.OptionName=OptionName;
        Parms.PlayerIndex=PlayerIndex;
        ProcessDelegate(UTGAME_OnOptionChanged,&__OnOptionChanged__Delegate,&Parms);
    }
    void delegateOnOptionFocused(class UUIScreenObject* InObject,class UUIDataProvider* OptionProvider)
    {
        UTUIOptionList_eventOnOptionFocused_Parms Parms(EC_EventParm);
        Parms.InObject=InObject;
        Parms.OptionProvider=OptionProvider;
        ProcessDelegate(UTGAME_OnOptionFocused,&__OnOptionFocused__Delegate,&Parms);
    }
    DECLARE_CLASS(UUTUIOptionList,UUTUI_Widget,0,UTGame)
    virtual UObject* GetUObjectInterfaceUIDataStoreSubscriber(){return this;}
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
};

struct UTUIKeyBindingList_eventGetPlayerInput_Parms
{
    class UPlayerInput* ReturnValue;
    UTUIKeyBindingList_eventGetPlayerInput_Parms(EEventParm)
    {
    }
};
class UUTUIKeyBindingList : public UUTUIOptionList
{
public:
    //## BEGIN PROPS UTUIKeyBindingList
    INT NumButtons;
    BITFIELD bCurrentlyBindingKey:1;
    class UUIObject* CurrentlyBindingObject;
    FName PreviousBinding;
    class UUTUIScene_MessageBox* MessageBoxReference;
    //## END PROPS UTUIKeyBindingList

    virtual void RegenerateOptions();
    virtual void RepositionOptions();
    virtual void RefreshBindingLabels();
    DECLARE_FUNCTION(execRefreshBindingLabels)
    {
        P_FINISH;
        RefreshBindingLabels();
    }
    class UPlayerInput* eventGetPlayerInput()
    {
        UTUIKeyBindingList_eventGetPlayerInput_Parms Parms(EC_EventParm);
        Parms.ReturnValue=0;
        ProcessEvent(FindFunctionChecked(UTGAME_GetPlayerInput),&Parms);
        return Parms.ReturnValue;
    }
    DECLARE_CLASS(UUTUIKeyBindingList,UUTUIOptionList,0,UTGame)
protected:
	/**
	 * Handles input events for this widget.
	 *
	 * @param	EventParms		the parameters for the input event
	 *
	 * @return	TRUE to consume the key event, FALSE to pass it on.
	 */
	virtual UBOOL ProcessInputKey( const struct FSubscribedInputEventParameters& EventParms );
};

class UUTUIFrontEnd : public UUTUIScene
{
public:
    //## BEGIN PROPS UTUIFrontEnd
    class UUTUIButtonBar* ButtonBar;
    class UUTUITabControl* TabControl;
    //## END PROPS UTUIFrontEnd

    DECLARE_CLASS(UUTUIFrontEnd,UUTUIScene,0,UTGame)
    NO_DEFAULT_CONSTRUCTOR(UUTUIFrontEnd)
};

#define UCONST_CHARACTERCUSTOMIZATION_BUTTONBAR_BACK 1
#define UCONST_CHARACTERCUSTOMIZATION_BUTTONBAR_ACCEPT 0

struct UTUIFrontEnd_CharacterCustomization_eventUpdateLoadingPackage_Parms
{
    UTUIFrontEnd_CharacterCustomization_eventUpdateLoadingPackage_Parms(EEventParm)
    {
    }
};
class UUTUIFrontEnd_CharacterCustomization : public UUTUIFrontEnd
{
public:
    //## BEGIN PROPS UTUIFrontEnd_CharacterCustomization
    class AUTCustomChar_Preview* PreviewActor;
    class UUTUITabPage_CharacterPart* CurrentPanel;
    class UUTUITabPage_CharacterPart* PreviousPanel;
    class UUIObject* LoadingPanel;
    class UUIObject* CharacterPanel;
    TArrayNoInit<class UUTUITabPage_CharacterPart*> PartPanels;
    TArrayNoInit<class UUIImage*> PartImages;
    class UUTCharFamilyAssetStore* LoadedPackage;
    class UUTCharFamilyAssetStore* PendingPackage;
    class USoundCue* TabShiftSoundCue;
    FStringNoInit Faction;
    FStringNoInit CharacterID;
    struct FCharacterInfo Character;
    BITFIELD bHaveLoadedCharData:1;
    struct FCustomCharData LoadedCharData;
    INT RotationDirection;
    //## END PROPS UTUIFrontEnd_CharacterCustomization

    void eventUpdateLoadingPackage()
    {
        ProcessEvent(FindFunctionChecked(UTGAME_UpdateLoadingPackage),NULL);
    }
    DECLARE_CLASS(UUTUIFrontEnd_CharacterCustomization,UUTUIFrontEnd,0,UTGame)
	virtual void Tick( FLOAT DeltaTime );
};

struct FCreditsImageSet
{
    class USurface* TexImage;
    FStringNoInit TexImageName;
    struct FTextureCoordinates TexCoords;
    TArrayNoInit<FString> LabelMarkup;

    /** Constructors */
    FCreditsImageSet() {}
    FCreditsImageSet(EEventParm)
    {
        appMemzero(this, sizeof(FCreditsImageSet));
    }
};

class UUTUIFrontEnd_Credits : public UUTUIFrontEnd
{
public:
    //## BEGIN PROPS UTUIFrontEnd_Credits
    FLOAT SceneTimeInSec;
    FLOAT ScrollAmount;
    FLOAT DelayBeforePictures;
    FLOAT DelayAfterPictures;
    FLOAT StartTime;
    TArrayNoInit<struct FCreditsImageSet> ImageSets;
    INT CurrentObjectOffset;
    INT CurrentImageSet;
    class UUILabel* QuoteLabels[6];
    class UUIImage* PhotoImage[2];
    class UUILabel* TextLabels[3];
    INT CurrentTextSet;
    FLOAT StartOffset[3];
    TArrayNoInit<FString> TextSets;
    //## END PROPS UTUIFrontEnd_Credits

    virtual void SetupScene();
    DECLARE_FUNCTION(execSetupScene)
    {
        P_FINISH;
        SetupScene();
    }
    DECLARE_CLASS(UUTUIFrontEnd_Credits,UUTUIFrontEnd,0,UTGame)
	virtual void Tick( FLOAT DeltaTime );

	/** 
	 * Changes the datastore bindings for widgets to the next image set.
	 *
	 * @param bFront Whether we are updating front or back widgets.
	 */
	void UpdateWidgets(UBOOL bFront);

	/** 
	 * Updates the position and text for credits widgets.
	 */
	void UpdateCreditsText();

	/**
	 * Callback that happens the first time the scene is rendered, any widget positioning initialization should be done here.
	 *
	 * By default this function recursively calls itself on all of its children.
	 */
	virtual void PreRenderCallback();
};

class UUTUIFrontEnd_MapSelection : public UUTUIScene
{
public:
    //## BEGIN PROPS UTUIFrontEnd_MapSelection
    //## END PROPS UTUIFrontEnd_MapSelection

    DECLARE_CLASS(UUTUIFrontEnd_MapSelection,UUTUIScene,0,UTGame)
    NO_DEFAULT_CONSTRUCTOR(UUTUIFrontEnd_MapSelection)
};

struct UTUIFrontEnd_TitleScreen_eventUpdateProfileLabels_Parms
{
    UTUIFrontEnd_TitleScreen_eventUpdateProfileLabels_Parms(EEventParm)
    {
    }
};
struct UTUIFrontEnd_TitleScreen_eventGetNumLoggedInPlayers_Parms
{
    INT ReturnValue;
    UTUIFrontEnd_TitleScreen_eventGetNumLoggedInPlayers_Parms(EEventParm)
    {
    }
};
class UUTUIFrontEnd_TitleScreen : public UUTUIScene
{
public:
    //## BEGIN PROPS UTUIFrontEnd_TitleScreen
    BITFIELD bInMovie:1;
    FLOAT TimeElapsed;
    FStringNoInit MovieName;
    FLOAT TimeTillAttractMovie;
    FStringNoInit MainMenuScene;
    BITFIELD bUpdatePlayersOnNextTick:1;
    //## END PROPS UTUIFrontEnd_TitleScreen

    virtual void UpdateGamePlayersArray();
    virtual void StartMovie();
    virtual void StopMovie();
    virtual void UpdateMovieStatus();
    DECLARE_FUNCTION(execUpdateGamePlayersArray)
    {
        P_FINISH;
        UpdateGamePlayersArray();
    }
    DECLARE_FUNCTION(execStartMovie)
    {
        P_FINISH;
        StartMovie();
    }
    DECLARE_FUNCTION(execStopMovie)
    {
        P_FINISH;
        StopMovie();
    }
    DECLARE_FUNCTION(execUpdateMovieStatus)
    {
        P_FINISH;
        UpdateMovieStatus();
    }
    void eventUpdateProfileLabels()
    {
        ProcessEvent(FindFunctionChecked(UTGAME_UpdateProfileLabels),NULL);
    }
    INT eventGetNumLoggedInPlayers()
    {
        UTUIFrontEnd_TitleScreen_eventGetNumLoggedInPlayers_Parms Parms(EC_EventParm);
        Parms.ReturnValue=0;
        ProcessEvent(FindFunctionChecked(UTGAME_GetNumLoggedInPlayers),&Parms);
        return Parms.ReturnValue;
    }
    DECLARE_CLASS(UUTUIFrontEnd_TitleScreen,UUTUIScene,0,UTGame)
	virtual void Tick( FLOAT DeltaTime );
};

#endif

AUTOGENERATE_FUNCTION(UUTUIFrontEnd_Credits,-1,execSetupScene);
AUTOGENERATE_FUNCTION(UUTUIFrontEnd_TitleScreen,-1,execUpdateMovieStatus);
AUTOGENERATE_FUNCTION(UUTUIFrontEnd_TitleScreen,-1,execStopMovie);
AUTOGENERATE_FUNCTION(UUTUIFrontEnd_TitleScreen,-1,execStartMovie);
AUTOGENERATE_FUNCTION(UUTUIFrontEnd_TitleScreen,-1,execUpdateGamePlayersArray);
AUTOGENERATE_FUNCTION(UUTUIKeyBindingList,-1,execRefreshBindingLabels);
AUTOGENERATE_FUNCTION(UUTUIKeyBindingList,-1,execRepositionOptions);
AUTOGENERATE_FUNCTION(UUTUIKeyBindingList,-1,execRegenerateOptions);
AUTOGENERATE_FUNCTION(UUTUIOptionList,-1,execClearBoundDataStores);
AUTOGENERATE_FUNCTION(UUTUIOptionList,-1,execGetBoundDataStores);
AUTOGENERATE_FUNCTION(UUTUIOptionList,-1,execRefreshSubscriberValue);
AUTOGENERATE_FUNCTION(UUTUIOptionList,-1,execGetDataStoreBinding);
AUTOGENERATE_FUNCTION(UUTUIOptionList,-1,execSetDataStoreBinding);
AUTOGENERATE_FUNCTION(UUTUIOptionList,-1,execSetSelectedOptionIndex);
AUTOGENERATE_FUNCTION(UUTUIOptionList,-1,execRepositionOptions);
AUTOGENERATE_FUNCTION(UUTUIOptionList,-1,execRegenerateOptions);
AUTOGENERATE_FUNCTION(UUTUIStatsList,-1,execClearBoundDataStores);
AUTOGENERATE_FUNCTION(UUTUIStatsList,-1,execGetBoundDataStores);
AUTOGENERATE_FUNCTION(UUTUIStatsList,-1,execRefreshSubscriberValue);
AUTOGENERATE_FUNCTION(UUTUIStatsList,-1,execGetDataStoreBinding);
AUTOGENERATE_FUNCTION(UUTUIStatsList,-1,execSetDataStoreBinding);
AUTOGENERATE_FUNCTION(UUTUIStatsList,-1,execSetStatsIndex);
AUTOGENERATE_FUNCTION(UUTUIStatsList,-1,execRepositionOptions);
AUTOGENERATE_FUNCTION(UUTUIStatsList,-1,execRegenerateOptions);

#ifndef NAMES_ONLY
#undef AUTOGENERATE_NAME
#undef AUTOGENERATE_FUNCTION
#endif

#ifdef STATIC_LINKING_MOJO
#ifndef UTGAME_UIFRONTEND_NATIVE_DEFS
#define UTGAME_UIFRONTEND_NATIVE_DEFS

DECLARE_NATIVE_TYPE(UTGame,UUTUIFrontEnd);
DECLARE_NATIVE_TYPE(UTGame,UUTUIFrontEnd_CharacterCustomization);
DECLARE_NATIVE_TYPE(UTGame,UUTUIFrontEnd_Credits);
DECLARE_NATIVE_TYPE(UTGame,UUTUIFrontEnd_MapSelection);
DECLARE_NATIVE_TYPE(UTGame,UUTUIFrontEnd_TitleScreen);
DECLARE_NATIVE_TYPE(UTGame,UUTUIKeyBindingList);
DECLARE_NATIVE_TYPE(UTGame,UUTUIOptionList);
DECLARE_NATIVE_TYPE(UTGame,UUTUIStatsList);
DECLARE_NATIVE_TYPE(UTGame,UUTUITabPage_CharacterPart);

#define AUTO_INITIALIZE_REGISTRANTS_UTGAME_UIFRONTEND \
	UUTUIFrontEnd::StaticClass(); \
	UUTUIFrontEnd_CharacterCustomization::StaticClass(); \
	UUTUIFrontEnd_Credits::StaticClass(); \
	GNativeLookupFuncs[Lookup++] = &FindUTGameUUTUIFrontEnd_CreditsNative; \
	UUTUIFrontEnd_MapSelection::StaticClass(); \
	UUTUIFrontEnd_TitleScreen::StaticClass(); \
	GNativeLookupFuncs[Lookup++] = &FindUTGameUUTUIFrontEnd_TitleScreenNative; \
	UUTUIKeyBindingList::StaticClass(); \
	GNativeLookupFuncs[Lookup++] = &FindUTGameUUTUIKeyBindingListNative; \
	UUTUIOptionList::StaticClass(); \
	GNativeLookupFuncs[Lookup++] = &FindUTGameUUTUIOptionListNative; \
	UUTUIStatsList::StaticClass(); \
	GNativeLookupFuncs[Lookup++] = &FindUTGameUUTUIStatsListNative; \
	UUTUITabPage_CharacterPart::StaticClass(); \

#endif // UTGAME_UIFRONTEND_NATIVE_DEFS

#ifdef NATIVES_ONLY
NATIVE_INFO(UUTUIFrontEnd_Credits) GUTGameUUTUIFrontEnd_CreditsNatives[] = 
{ 
	MAP_NATIVE(UUTUIFrontEnd_Credits,execSetupScene)
	{NULL,NULL}
};
IMPLEMENT_NATIVE_HANDLER(UTGame,UUTUIFrontEnd_Credits);

NATIVE_INFO(UUTUIFrontEnd_TitleScreen) GUTGameUUTUIFrontEnd_TitleScreenNatives[] = 
{ 
	MAP_NATIVE(UUTUIFrontEnd_TitleScreen,execUpdateMovieStatus)
	MAP_NATIVE(UUTUIFrontEnd_TitleScreen,execStopMovie)
	MAP_NATIVE(UUTUIFrontEnd_TitleScreen,execStartMovie)
	MAP_NATIVE(UUTUIFrontEnd_TitleScreen,execUpdateGamePlayersArray)
	{NULL,NULL}
};
IMPLEMENT_NATIVE_HANDLER(UTGame,UUTUIFrontEnd_TitleScreen);

NATIVE_INFO(UUTUIKeyBindingList) GUTGameUUTUIKeyBindingListNatives[] = 
{ 
	MAP_NATIVE(UUTUIKeyBindingList,execRefreshBindingLabels)
	MAP_NATIVE(UUTUIKeyBindingList,execRepositionOptions)
	MAP_NATIVE(UUTUIKeyBindingList,execRegenerateOptions)
	{NULL,NULL}
};
IMPLEMENT_NATIVE_HANDLER(UTGame,UUTUIKeyBindingList);

NATIVE_INFO(UUTUIOptionList) GUTGameUUTUIOptionListNatives[] = 
{ 
	MAP_NATIVE(UUTUIOptionList,execClearBoundDataStores)
	MAP_NATIVE(UUTUIOptionList,execGetBoundDataStores)
	MAP_NATIVE(UUTUIOptionList,execRefreshSubscriberValue)
	MAP_NATIVE(UUTUIOptionList,execGetDataStoreBinding)
	MAP_NATIVE(UUTUIOptionList,execSetDataStoreBinding)
	MAP_NATIVE(UUTUIOptionList,execSetSelectedOptionIndex)
	MAP_NATIVE(UUTUIOptionList,execRepositionOptions)
	MAP_NATIVE(UUTUIOptionList,execRegenerateOptions)
	{NULL,NULL}
};
IMPLEMENT_NATIVE_HANDLER(UTGame,UUTUIOptionList);

NATIVE_INFO(UUTUIStatsList) GUTGameUUTUIStatsListNatives[] = 
{ 
	MAP_NATIVE(UUTUIStatsList,execClearBoundDataStores)
	MAP_NATIVE(UUTUIStatsList,execGetBoundDataStores)
	MAP_NATIVE(UUTUIStatsList,execRefreshSubscriberValue)
	MAP_NATIVE(UUTUIStatsList,execGetDataStoreBinding)
	MAP_NATIVE(UUTUIStatsList,execSetDataStoreBinding)
	MAP_NATIVE(UUTUIStatsList,execSetStatsIndex)
	MAP_NATIVE(UUTUIStatsList,execRepositionOptions)
	MAP_NATIVE(UUTUIStatsList,execRegenerateOptions)
	{NULL,NULL}
};
IMPLEMENT_NATIVE_HANDLER(UTGame,UUTUIStatsList);

#endif // NATIVES_ONLY
#endif // STATIC_LINKING_MOJO

#ifdef VERIFY_CLASS_SIZES
VERIFY_CLASS_OFFSET_NODIE(U,UTUIFrontEnd,ButtonBar)
VERIFY_CLASS_OFFSET_NODIE(U,UTUIFrontEnd,TabControl)
VERIFY_CLASS_SIZE_NODIE(UUTUIFrontEnd)
VERIFY_CLASS_OFFSET_NODIE(U,UTUIFrontEnd_CharacterCustomization,PreviewActor)
VERIFY_CLASS_OFFSET_NODIE(U,UTUIFrontEnd_CharacterCustomization,RotationDirection)
VERIFY_CLASS_SIZE_NODIE(UUTUIFrontEnd_CharacterCustomization)
VERIFY_CLASS_OFFSET_NODIE(U,UTUIFrontEnd_Credits,SceneTimeInSec)
VERIFY_CLASS_OFFSET_NODIE(U,UTUIFrontEnd_Credits,TextSets)
VERIFY_CLASS_SIZE_NODIE(UUTUIFrontEnd_Credits)
VERIFY_CLASS_SIZE_NODIE(UUTUIFrontEnd_MapSelection)
VERIFY_CLASS_OFFSET_NODIE(U,UTUIFrontEnd_TitleScreen,TimeElapsed)
VERIFY_CLASS_OFFSET_NODIE(U,UTUIFrontEnd_TitleScreen,MainMenuScene)
VERIFY_CLASS_SIZE_NODIE(UUTUIFrontEnd_TitleScreen)
VERIFY_CLASS_OFFSET_NODIE(U,UTUIKeyBindingList,NumButtons)
VERIFY_CLASS_OFFSET_NODIE(U,UTUIKeyBindingList,MessageBoxReference)
VERIFY_CLASS_SIZE_NODIE(UUTUIKeyBindingList)
VERIFY_CLASS_OFFSET_NODIE(U,UTUIOptionList,CurrentIndex)
VERIFY_CLASS_OFFSET_NODIE(U,UTUIOptionList,__OnAcceptOptions__Delegate)
VERIFY_CLASS_SIZE_NODIE(UUTUIOptionList)
VERIFY_CLASS_OFFSET_NODIE(U,UTUIStatsList,GeneratedObjects)
VERIFY_CLASS_OFFSET_NODIE(U,UTUIStatsList,DataProvider)
VERIFY_CLASS_SIZE_NODIE(UUTUIStatsList)
VERIFY_CLASS_OFFSET_NODIE(U,UTUITabPage_CharacterPart,OffsetVector)
VERIFY_CLASS_OFFSET_NODIE(U,UTUITabPage_CharacterPart,__OnPreviewPartChanged__Delegate)
VERIFY_CLASS_SIZE_NODIE(UUTUITabPage_CharacterPart)
#endif // VERIFY_CLASS_SIZES
#endif // !ENUMS_ONLY

#if SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif