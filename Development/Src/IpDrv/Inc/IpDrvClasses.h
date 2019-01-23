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


#endif // !NO_ENUMS

#if !ENUMS_ONLY

#ifndef NAMES_ONLY
#define AUTOGENERATE_NAME(name) extern FName IPDRV_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name)
#endif

AUTOGENERATE_NAME(GetPlayerNicknameFromIndex)
AUTOGENERATE_NAME(GetPlayerUniqueNetIdFromIndex)
AUTOGENERATE_NAME(OnArbitrationRegistrationComplete)
AUTOGENERATE_NAME(OnCreateOnlineGameComplete)
AUTOGENERATE_NAME(OnDestroyOnlineGameComplete)
AUTOGENERATE_NAME(OnEndOnlineGameComplete)
AUTOGENERATE_NAME(OnFindOnlineGamesComplete)
AUTOGENERATE_NAME(OnGameInviteAccepted)
AUTOGENERATE_NAME(OnJoinOnlineGameComplete)
AUTOGENERATE_NAME(OnRegisterDedicatedServerComplete)
AUTOGENERATE_NAME(OnRegisterPlayerComplete)
AUTOGENERATE_NAME(OnStartOnlineGameComplete)
AUTOGENERATE_NAME(OnUnregisterDedicatedServerComplete)
AUTOGENERATE_NAME(OnUnregisterPlayerComplete)

#ifndef NAMES_ONLY

struct OnlineGameInterfaceImpl_eventOnUnregisterDedicatedServerComplete_Parms
{
    UBOOL bWasSuccessful;
    OnlineGameInterfaceImpl_eventOnUnregisterDedicatedServerComplete_Parms(EEventParm)
    {
    }
};
struct OnlineGameInterfaceImpl_eventOnRegisterDedicatedServerComplete_Parms
{
    UBOOL bWasSuccessful;
    OnlineGameInterfaceImpl_eventOnRegisterDedicatedServerComplete_Parms(EEventParm)
    {
    }
};
struct OnlineGameInterfaceImpl_eventOnGameInviteAccepted_Parms
{
    class UOnlineGameSettings* GameInviteSettings;
    OnlineGameInterfaceImpl_eventOnGameInviteAccepted_Parms(EEventParm)
    {
    }
};
struct OnlineGameInterfaceImpl_eventOnArbitrationRegistrationComplete_Parms
{
    UBOOL bWasSuccessful;
    OnlineGameInterfaceImpl_eventOnArbitrationRegistrationComplete_Parms(EEventParm)
    {
    }
};
struct OnlineGameInterfaceImpl_eventOnEndOnlineGameComplete_Parms
{
    UBOOL bWasSuccessful;
    OnlineGameInterfaceImpl_eventOnEndOnlineGameComplete_Parms(EEventParm)
    {
    }
};
struct OnlineGameInterfaceImpl_eventOnStartOnlineGameComplete_Parms
{
    UBOOL bWasSuccessful;
    OnlineGameInterfaceImpl_eventOnStartOnlineGameComplete_Parms(EEventParm)
    {
    }
};
struct OnlineGameInterfaceImpl_eventOnUnregisterPlayerComplete_Parms
{
    UBOOL bWasSuccessful;
    OnlineGameInterfaceImpl_eventOnUnregisterPlayerComplete_Parms(EEventParm)
    {
    }
};
struct OnlineGameInterfaceImpl_eventOnRegisterPlayerComplete_Parms
{
    UBOOL bWasSuccessful;
    OnlineGameInterfaceImpl_eventOnRegisterPlayerComplete_Parms(EEventParm)
    {
    }
};
struct OnlineGameInterfaceImpl_eventOnJoinOnlineGameComplete_Parms
{
    UBOOL bWasSuccessful;
    OnlineGameInterfaceImpl_eventOnJoinOnlineGameComplete_Parms(EEventParm)
    {
    }
};
struct OnlineGameInterfaceImpl_eventOnDestroyOnlineGameComplete_Parms
{
    UBOOL bWasSuccessful;
    OnlineGameInterfaceImpl_eventOnDestroyOnlineGameComplete_Parms(EEventParm)
    {
    }
};
struct OnlineGameInterfaceImpl_eventOnCreateOnlineGameComplete_Parms
{
    UBOOL bWasSuccessful;
    OnlineGameInterfaceImpl_eventOnCreateOnlineGameComplete_Parms(EEventParm)
    {
    }
};
struct OnlineGameInterfaceImpl_eventOnFindOnlineGamesComplete_Parms
{
    UBOOL bWasSuccessful;
    OnlineGameInterfaceImpl_eventOnFindOnlineGamesComplete_Parms(EEventParm)
    {
    }
};
class UOnlineGameInterfaceImpl : public UObject
{
public:
    //## BEGIN PROPS OnlineGameInterfaceImpl
    class UOnlineSubsystemCommonImpl* OwningSubsystem;
    class UOnlineGameSettings* GameSettings;
    class UOnlineGameSearch* GameSearch;
    BYTE CurrentGameState;
    BYTE LanBeaconState;
    BYTE LanNonce[8];
    TArrayNoInit<FScriptDelegate> CreateOnlineGameCompleteDelegates;
    TArrayNoInit<FScriptDelegate> DestroyOnlineGameCompleteDelegates;
    TArrayNoInit<FScriptDelegate> JoinOnlineGameCompleteDelegates;
    TArrayNoInit<FScriptDelegate> StartOnlineGameCompleteDelegates;
    TArrayNoInit<FScriptDelegate> EndOnlineGameCompleteDelegates;
    TArrayNoInit<FScriptDelegate> FindOnlineGamesCompleteDelegates;
    TArrayNoInit<FScriptDelegate> RegisterDedicatedServerCompleteDelegates;
    TArrayNoInit<FScriptDelegate> UnregisterDedicatedServerCompleteDelegates;
    INT LanAnnouncePort;
    INT LanGameUniqueId;
    INT LanPacketPlatformMask;
    FLOAT LanQueryTimeLeft;
    FLOAT LanQueryTimeout;
    FLanBeacon* LanBeacon;
    FSessionInfo* SessionInfo;
    FScriptDelegate __OnFindOnlineGamesComplete__Delegate;
    FScriptDelegate __OnCreateOnlineGameComplete__Delegate;
    FScriptDelegate __OnDestroyOnlineGameComplete__Delegate;
    FScriptDelegate __OnJoinOnlineGameComplete__Delegate;
    FScriptDelegate __OnRegisterPlayerComplete__Delegate;
    FScriptDelegate __OnUnregisterPlayerComplete__Delegate;
    FScriptDelegate __OnStartOnlineGameComplete__Delegate;
    FScriptDelegate __OnEndOnlineGameComplete__Delegate;
    FScriptDelegate __OnArbitrationRegistrationComplete__Delegate;
    FScriptDelegate __OnGameInviteAccepted__Delegate;
    FScriptDelegate __OnRegisterDedicatedServerComplete__Delegate;
    FScriptDelegate __OnUnregisterDedicatedServerComplete__Delegate;
    //## END PROPS OnlineGameInterfaceImpl

    virtual UBOOL CreateOnlineGame(BYTE HostingPlayerNum,class UOnlineGameSettings* NewGameSettings);
    virtual UBOOL DestroyOnlineGame();
    virtual UBOOL FindOnlineGames(BYTE SearchingPlayerNum,class UOnlineGameSearch* SearchSettings);
    virtual void FreeSearchResults();
    virtual UBOOL JoinOnlineGame(BYTE PlayerNum,const struct FOnlineGameSearchResult& DesiredGame);
    virtual UBOOL GetResolvedConnectString(FString& ConnectInfo);
    virtual UBOOL StartOnlineGame();
    virtual UBOOL EndOnlineGame();
    DECLARE_FUNCTION(execCreateOnlineGame)
    {
        P_GET_BYTE(HostingPlayerNum);
        P_GET_OBJECT(UOnlineGameSettings,NewGameSettings);
        P_FINISH;
        *(UBOOL*)Result=CreateOnlineGame(HostingPlayerNum,NewGameSettings);
    }
    DECLARE_FUNCTION(execDestroyOnlineGame)
    {
        P_FINISH;
        *(UBOOL*)Result=DestroyOnlineGame();
    }
    DECLARE_FUNCTION(execFindOnlineGames)
    {
        P_GET_BYTE(SearchingPlayerNum);
        P_GET_OBJECT(UOnlineGameSearch,SearchSettings);
        P_FINISH;
        *(UBOOL*)Result=FindOnlineGames(SearchingPlayerNum,SearchSettings);
    }
    DECLARE_FUNCTION(execFreeSearchResults)
    {
        P_FINISH;
        FreeSearchResults();
    }
    DECLARE_FUNCTION(execJoinOnlineGame)
    {
        P_GET_BYTE(PlayerNum);
        P_GET_STRUCT_REF(struct FOnlineGameSearchResult,DesiredGame);
        P_FINISH;
        *(UBOOL*)Result=JoinOnlineGame(PlayerNum,DesiredGame);
    }
    DECLARE_FUNCTION(execGetResolvedConnectString)
    {
        P_GET_STR_REF(ConnectInfo);
        P_FINISH;
        *(UBOOL*)Result=GetResolvedConnectString(ConnectInfo);
    }
    DECLARE_FUNCTION(execStartOnlineGame)
    {
        P_FINISH;
        *(UBOOL*)Result=StartOnlineGame();
    }
    DECLARE_FUNCTION(execEndOnlineGame)
    {
        P_FINISH;
        *(UBOOL*)Result=EndOnlineGame();
    }
    void delegateOnUnregisterDedicatedServerComplete(UBOOL bWasSuccessful)
    {
        OnlineGameInterfaceImpl_eventOnUnregisterDedicatedServerComplete_Parms Parms(EC_EventParm);
        Parms.bWasSuccessful=bWasSuccessful ? FIRST_BITFIELD : 0;
        ProcessDelegate(IPDRV_OnUnregisterDedicatedServerComplete,&__OnUnregisterDedicatedServerComplete__Delegate,&Parms);
    }
    void delegateOnRegisterDedicatedServerComplete(UBOOL bWasSuccessful)
    {
        OnlineGameInterfaceImpl_eventOnRegisterDedicatedServerComplete_Parms Parms(EC_EventParm);
        Parms.bWasSuccessful=bWasSuccessful ? FIRST_BITFIELD : 0;
        ProcessDelegate(IPDRV_OnRegisterDedicatedServerComplete,&__OnRegisterDedicatedServerComplete__Delegate,&Parms);
    }
    void delegateOnGameInviteAccepted(class UOnlineGameSettings* GameInviteSettings)
    {
        OnlineGameInterfaceImpl_eventOnGameInviteAccepted_Parms Parms(EC_EventParm);
        Parms.GameInviteSettings=GameInviteSettings;
        ProcessDelegate(IPDRV_OnGameInviteAccepted,&__OnGameInviteAccepted__Delegate,&Parms);
    }
    void delegateOnArbitrationRegistrationComplete(UBOOL bWasSuccessful)
    {
        OnlineGameInterfaceImpl_eventOnArbitrationRegistrationComplete_Parms Parms(EC_EventParm);
        Parms.bWasSuccessful=bWasSuccessful ? FIRST_BITFIELD : 0;
        ProcessDelegate(IPDRV_OnArbitrationRegistrationComplete,&__OnArbitrationRegistrationComplete__Delegate,&Parms);
    }
    void delegateOnEndOnlineGameComplete(UBOOL bWasSuccessful)
    {
        OnlineGameInterfaceImpl_eventOnEndOnlineGameComplete_Parms Parms(EC_EventParm);
        Parms.bWasSuccessful=bWasSuccessful ? FIRST_BITFIELD : 0;
        ProcessDelegate(IPDRV_OnEndOnlineGameComplete,&__OnEndOnlineGameComplete__Delegate,&Parms);
    }
    void delegateOnStartOnlineGameComplete(UBOOL bWasSuccessful)
    {
        OnlineGameInterfaceImpl_eventOnStartOnlineGameComplete_Parms Parms(EC_EventParm);
        Parms.bWasSuccessful=bWasSuccessful ? FIRST_BITFIELD : 0;
        ProcessDelegate(IPDRV_OnStartOnlineGameComplete,&__OnStartOnlineGameComplete__Delegate,&Parms);
    }
    void delegateOnUnregisterPlayerComplete(UBOOL bWasSuccessful)
    {
        OnlineGameInterfaceImpl_eventOnUnregisterPlayerComplete_Parms Parms(EC_EventParm);
        Parms.bWasSuccessful=bWasSuccessful ? FIRST_BITFIELD : 0;
        ProcessDelegate(IPDRV_OnUnregisterPlayerComplete,&__OnUnregisterPlayerComplete__Delegate,&Parms);
    }
    void delegateOnRegisterPlayerComplete(UBOOL bWasSuccessful)
    {
        OnlineGameInterfaceImpl_eventOnRegisterPlayerComplete_Parms Parms(EC_EventParm);
        Parms.bWasSuccessful=bWasSuccessful ? FIRST_BITFIELD : 0;
        ProcessDelegate(IPDRV_OnRegisterPlayerComplete,&__OnRegisterPlayerComplete__Delegate,&Parms);
    }
    void delegateOnJoinOnlineGameComplete(UBOOL bWasSuccessful)
    {
        OnlineGameInterfaceImpl_eventOnJoinOnlineGameComplete_Parms Parms(EC_EventParm);
        Parms.bWasSuccessful=bWasSuccessful ? FIRST_BITFIELD : 0;
        ProcessDelegate(IPDRV_OnJoinOnlineGameComplete,&__OnJoinOnlineGameComplete__Delegate,&Parms);
    }
    void delegateOnDestroyOnlineGameComplete(UBOOL bWasSuccessful)
    {
        OnlineGameInterfaceImpl_eventOnDestroyOnlineGameComplete_Parms Parms(EC_EventParm);
        Parms.bWasSuccessful=bWasSuccessful ? FIRST_BITFIELD : 0;
        ProcessDelegate(IPDRV_OnDestroyOnlineGameComplete,&__OnDestroyOnlineGameComplete__Delegate,&Parms);
    }
    void delegateOnCreateOnlineGameComplete(UBOOL bWasSuccessful)
    {
        OnlineGameInterfaceImpl_eventOnCreateOnlineGameComplete_Parms Parms(EC_EventParm);
        Parms.bWasSuccessful=bWasSuccessful ? FIRST_BITFIELD : 0;
        ProcessDelegate(IPDRV_OnCreateOnlineGameComplete,&__OnCreateOnlineGameComplete__Delegate,&Parms);
    }
    void delegateOnFindOnlineGamesComplete(UBOOL bWasSuccessful)
    {
        OnlineGameInterfaceImpl_eventOnFindOnlineGamesComplete_Parms Parms(EC_EventParm);
        Parms.bWasSuccessful=bWasSuccessful ? FIRST_BITFIELD : 0;
        ProcessDelegate(IPDRV_OnFindOnlineGamesComplete,&__OnFindOnlineGamesComplete__Delegate,&Parms);
    }
    DECLARE_CLASS(UOnlineGameInterfaceImpl,UObject,0|CLASS_Config,IpDrv)
    DECLARE_WITHIN(UOnlineSubsystemCommonImpl)
    static const TCHAR* StaticConfigName() {return TEXT("Engine");}

    #include "UOnlineGameInterfaceImpl.h"
};

struct OnlineSubsystemCommonImpl_eventGetPlayerUniqueNetIdFromIndex_Parms
{
    INT UserIndex;
    struct FUniqueNetId ReturnValue;
    OnlineSubsystemCommonImpl_eventGetPlayerUniqueNetIdFromIndex_Parms(EEventParm)
    {
    }
};
struct OnlineSubsystemCommonImpl_eventGetPlayerNicknameFromIndex_Parms
{
    INT UserIndex;
    FString ReturnValue;
    OnlineSubsystemCommonImpl_eventGetPlayerNicknameFromIndex_Parms(EEventParm)
    {
    }
};
class UOnlineSubsystemCommonImpl : public UOnlineSubsystem
{
public:
    //## BEGIN PROPS OnlineSubsystemCommonImpl
    class FVoiceInterface* VoiceEngine;
    INT MaxLocalTalkers;
    INT MaxRemoteTalkers;
    BITFIELD bIsUsingSpeechRecognition:1;
    class UOnlineGameInterfaceImpl* GameInterfaceImpl;
    //## END PROPS OnlineSubsystemCommonImpl

    struct FUniqueNetId eventGetPlayerUniqueNetIdFromIndex(INT UserIndex)
    {
        OnlineSubsystemCommonImpl_eventGetPlayerUniqueNetIdFromIndex_Parms Parms(EC_EventParm);
        appMemzero(&Parms.ReturnValue,sizeof(Parms.ReturnValue));
        Parms.UserIndex=UserIndex;
        ProcessEvent(FindFunctionChecked(IPDRV_GetPlayerUniqueNetIdFromIndex),&Parms);
        return Parms.ReturnValue;
    }
    FString eventGetPlayerNicknameFromIndex(INT UserIndex)
    {
        OnlineSubsystemCommonImpl_eventGetPlayerNicknameFromIndex_Parms Parms(EC_EventParm);
        Parms.UserIndex=UserIndex;
        ProcessEvent(FindFunctionChecked(IPDRV_GetPlayerNicknameFromIndex),&Parms);
        return Parms.ReturnValue;
    }
    DECLARE_CLASS(UOnlineSubsystemCommonImpl,UOnlineSubsystem,0|CLASS_Config,IpDrv)
    static const TCHAR* StaticConfigName() {return TEXT("Engine");}

    #include "UOnlineSubsystemCommonImpl.h"
};

#endif

AUTOGENERATE_FUNCTION(UOnlineGameInterfaceImpl,-1,execEndOnlineGame);
AUTOGENERATE_FUNCTION(UOnlineGameInterfaceImpl,-1,execStartOnlineGame);
AUTOGENERATE_FUNCTION(UOnlineGameInterfaceImpl,-1,execGetResolvedConnectString);
AUTOGENERATE_FUNCTION(UOnlineGameInterfaceImpl,-1,execJoinOnlineGame);
AUTOGENERATE_FUNCTION(UOnlineGameInterfaceImpl,-1,execFreeSearchResults);
AUTOGENERATE_FUNCTION(UOnlineGameInterfaceImpl,-1,execFindOnlineGames);
AUTOGENERATE_FUNCTION(UOnlineGameInterfaceImpl,-1,execDestroyOnlineGame);
AUTOGENERATE_FUNCTION(UOnlineGameInterfaceImpl,-1,execCreateOnlineGame);

#ifndef NAMES_ONLY
#undef AUTOGENERATE_NAME
#undef AUTOGENERATE_FUNCTION
#endif

#ifdef STATIC_LINKING_MOJO
#ifndef IPDRV_NATIVE_DEFS
#define IPDRV_NATIVE_DEFS

DECLARE_NATIVE_TYPE(IpDrv,UOnlineGameInterfaceImpl);
DECLARE_NATIVE_TYPE(IpDrv,UOnlineSubsystemCommonImpl);

#define AUTO_INITIALIZE_REGISTRANTS_IPDRV \
	UHTTPDownload::StaticClass(); \
	UOnlineGameInterfaceImpl::StaticClass(); \
	GNativeLookupFuncs[Lookup++] = &FindIpDrvUOnlineGameInterfaceImplNative; \
	UOnlineSubsystemCommonImpl::StaticClass(); \
	UTcpipConnection::StaticClass(); \
	UTcpNetDriver::StaticClass(); \

#endif // IPDRV_NATIVE_DEFS

#ifdef NATIVES_ONLY
NATIVE_INFO(UOnlineGameInterfaceImpl) GIpDrvUOnlineGameInterfaceImplNatives[] = 
{ 
	MAP_NATIVE(UOnlineGameInterfaceImpl,execEndOnlineGame)
	MAP_NATIVE(UOnlineGameInterfaceImpl,execStartOnlineGame)
	MAP_NATIVE(UOnlineGameInterfaceImpl,execGetResolvedConnectString)
	MAP_NATIVE(UOnlineGameInterfaceImpl,execJoinOnlineGame)
	MAP_NATIVE(UOnlineGameInterfaceImpl,execFreeSearchResults)
	MAP_NATIVE(UOnlineGameInterfaceImpl,execFindOnlineGames)
	MAP_NATIVE(UOnlineGameInterfaceImpl,execDestroyOnlineGame)
	MAP_NATIVE(UOnlineGameInterfaceImpl,execCreateOnlineGame)
	{NULL,NULL}
};
IMPLEMENT_NATIVE_HANDLER(IpDrv,UOnlineGameInterfaceImpl);

#endif // NATIVES_ONLY
#endif // STATIC_LINKING_MOJO

#ifdef VERIFY_CLASS_SIZES
VERIFY_CLASS_OFFSET_NODIE(U,OnlineGameInterfaceImpl,OwningSubsystem)
VERIFY_CLASS_OFFSET_NODIE(U,OnlineGameInterfaceImpl,__OnUnregisterDedicatedServerComplete__Delegate)
VERIFY_CLASS_SIZE_NODIE(UOnlineGameInterfaceImpl)
VERIFY_CLASS_OFFSET_NODIE(U,OnlineSubsystemCommonImpl,VoiceEngine)
VERIFY_CLASS_OFFSET_NODIE(U,OnlineSubsystemCommonImpl,GameInterfaceImpl)
VERIFY_CLASS_SIZE_NODIE(UOnlineSubsystemCommonImpl)
#endif // VERIFY_CLASS_SIZES
#endif // !ENUMS_ONLY

#if SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif
