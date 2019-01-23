/*===========================================================================
    C++ class definitions exported from UnrealScript.
    This is automatically generated by the tools.
    DO NOT modify this manually! Edit the corresponding .uc files instead!
    Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
===========================================================================*/
#if SUPPORTS_PRAGMA_PACK
#pragma pack (push,4)
#endif

#include "GameFrameworkNames.h"

// Split enums from the rest of the header so they can be included earlier
// than the rest of the header file by including this file twice with different
// #define wrappers. See Engine.h and look at EngineClasses.h for an example.
#if !NO_ENUMS && !defined(NAMES_ONLY)

#ifndef INCLUDED_GAMEFRAMEWORK_GAMESTATS_ENUMS
#define INCLUDED_GAMEFRAMEWORK_GAMESTATS_ENUMS 1

enum GameSessionType
{
    GT_SessionInvalid       =0,
    GT_SinglePlayer         =1,
    GT_Coop                 =2,
    GT_Multiplayer          =3,
    GT_MAX                  =4,
};
#define FOREACH_ENUM_GAMESESSIONTYPE(op) \
    op(GT_SessionInvalid) \
    op(GT_SinglePlayer) \
    op(GT_Coop) \
    op(GT_Multiplayer) 

#endif // !INCLUDED_GAMEFRAMEWORK_GAMESTATS_ENUMS
#endif // !NO_ENUMS

#if !ENUMS_ONLY

#ifndef NAMES_ONLY
#define AUTOGENERATE_FUNCTION(cls,idx,name)
#endif


#ifndef NAMES_ONLY

#ifndef INCLUDED_GAMEFRAMEWORK_GAMESTATS_CLASSES
#define INCLUDED_GAMEFRAMEWORK_GAMESTATS_CLASSES 1
#define ENABLE_DECLARECLASS_MACRO 1
#include "UnObjBas.h"
#undef ENABLE_DECLARECLASS_MACRO

struct FTeamState
{
    INT TeamIndex;
    TArray<INT> PlayerIndices;

    /** Constructors */
    FTeamState() {}
    FTeamState(EEventParm)
    {
        appMemzero(this, sizeof(FTeamState));
    }
};

struct FPlayerState
{
    INT PlayerIndex;
    INT CurrentTeamIndex;
    FLOAT TimeSpawned;
    FLOAT TimeAliveSinceLastDeath;

    /** Constructors */
    FPlayerState() {}
    FPlayerState(EEventParm)
    {
        appMemzero(this, sizeof(FPlayerState));
    }
};

class UGameStateObject : public UGameplayEventsHandler
{
public:
    //## BEGIN PROPS GameStateObject
    TArray<FTeamState*> TeamStates;
    TArray<FPlayerState*> PlayerStates;
    BYTE SessionType;
    SCRIPT_ALIGN;
    BITFIELD bIsMatchStarted:1;
    BITFIELD bIsRoundStarted:1;
    INT RoundNumber;
    INT MaxRoundNumber;
    //## END PROPS GameStateObject

    virtual void PreProcessStream();
    virtual void Reset();
    DECLARE_FUNCTION(execReset)
    {
        P_FINISH;
        this->Reset();
    }
    DECLARE_CLASS(UGameStateObject,UGameplayEventsHandler,0|CLASS_Config,GameFramework)
	/** Return the round number in a given match, -1 if its not multiplayer */
	INT GetRoundNumber() { if (SessionType == GT_Multiplayer) { return RoundNumber; } else { return -1; } }

	/*
	 *   Get a given team's current state, creating a new one if necessary
	 *   @param TeamIndex - index of team to return state for
	 *   @return State for given team
	 */
	virtual FTeamState* GetTeamState(INT TeamIndex)
	{
		INT TeamStateIdx = 0;
		for (; TeamStateIdx < TeamStates.Num(); TeamStateIdx++)
		{
			if (TeamStates(TeamStateIdx)->TeamIndex == TeamIndex)
			{
				break;
			}
		}

		//Create a new team if necessary
		if (TeamStateIdx == TeamStates.Num())
		{
			FTeamState* NewTeamState = new FTeamState;
			NewTeamState->TeamIndex = TeamIndex;
			TeamStateIdx = TeamStates.AddItem(NewTeamState);
		}

		return TeamStates(TeamStateIdx);
	}

	/*
	 *   Get a given player's current state, creating a new one if necessary
	 *   @param PlayerIndex - index of player to return state for
	 *   @return State for given player
	 */
	virtual FPlayerState* GetPlayerState(INT PlayerIndex)
	{
		INT PlayerStateIdx = 0;
		for (; PlayerStateIdx < PlayerStates.Num(); PlayerStateIdx++)
		{
			if (PlayerStates(PlayerStateIdx)->PlayerIndex == PlayerIndex)
			{
				break;
			}
		}

		//Create a new player if necessary
		if (PlayerStateIdx == PlayerStates.Num())
		{
			FPlayerState* NewPlayerState = new FPlayerState;
			NewPlayerState->PlayerIndex = PlayerIndex;
			NewPlayerState->CurrentTeamIndex = INDEX_NONE;
			NewPlayerState->TimeSpawned = 0;
			NewPlayerState->TimeAliveSinceLastDeath = 0;
			PlayerStateIdx = PlayerStates.AddItem(NewPlayerState);
		}

		return PlayerStates(PlayerStateIdx);
	}

	/*
	 *   Get the team index for a given player
	 * @param PlayerIndex - player to get team index for
	 * @return Game specific team index
	 */
	INT GetTeamIndexForPlayer(INT PlayerIndex)
	{
		 const FPlayerState* PlayerState = GetPlayerState(PlayerIndex);
		 return PlayerState->CurrentTeamIndex;
	}

	/** Handlers for parsing the game stats stream */

	// Game Event Handling
	virtual void HandleGameStringEvent(struct FGameEventHeader& GameEvent, struct FGameStringEvent* GameEventData);
	virtual void HandleGameIntEvent(struct FGameEventHeader& GameEvent, struct FGameIntEvent* GameEventData);
	virtual void HandleGameFloatEvent(struct FGameEventHeader& GameEvent, struct FGameFloatEvent* GameEventData);
	virtual void HandleGamePositionEvent(struct FGameEventHeader& GameEvent, struct FGamePositionEvent* GameEventData);

	// Team Event Handling
	virtual void HandleTeamStringEvent(struct FGameEventHeader& GameEvent, struct FTeamStringEvent* GameEventData);
	virtual void HandleTeamIntEvent(struct FGameEventHeader& GameEvent, struct FTeamIntEvent* GameEventData);
	virtual void HandleTeamFloatEvent(struct FGameEventHeader& GameEvent, struct FTeamFloatEvent* GameEventData);

	// Player Event Handling
	virtual void HandlePlayerIntEvent(struct FGameEventHeader& GameEvent, struct FPlayerIntEvent* GameEventData);
	virtual void HandlePlayerFloatEvent(struct FGameEventHeader& GameEvent, struct FPlayerFloatEvent* GameEventData);
	virtual void HandlePlayerStringEvent(struct FGameEventHeader& GameEvent, struct FPlayerStringEvent* GameEventData);
	virtual void HandlePlayerSpawnEvent(struct FGameEventHeader& GameEvent, struct FPlayerSpawnEvent* GameEventData);
	virtual void HandlePlayerLoginEvent(struct FGameEventHeader& GameEvent, struct FPlayerLoginEvent* GameEventData);
	virtual void HandlePlayerKillDeathEvent(struct FGameEventHeader& GameEvent, struct FPlayerKillDeathEvent* GameEventData);
	virtual void HandlePlayerPlayerEvent(struct FGameEventHeader& GameEvent, struct FPlayerPlayerEvent* GameEventData);
	virtual void HandlePlayerLocationsEvent(struct FGameEventHeader& GameEvent, struct FPlayerLocationsEvent* GameEventData);
	virtual void HandleWeaponIntEvent(struct FGameEventHeader& GameEvent, struct FWeaponIntEvent* GameEventData);
	virtual void HandleDamageIntEvent(struct FGameEventHeader& GameEvent, struct FDamageIntEvent* GameEventData);
	virtual void HandleProjectileIntEvent(struct FGameEventHeader& GameEvent, struct FProjectileIntEvent* GameEventData);

	/*
	 * Called when end of round event is parsed, allows for any current
	 * state values to be closed out (time alive, etc) 
	 * @param TimeStamp - time of the round end event
	 */
	virtual void CleanupRoundState(FLOAT TimeStamp);
	/*
	 * Called when end of match event is parsed, allows for any current
	 * state values to be closed out (round events, etc) 
	 * @param TimeStamp - time of the match end event
	 */
	virtual void CleanupMatchState(FLOAT TimeStamp);

	/*
	 *   Cleanup a player state (round over/logout)
	 */
	virtual void CleanupPlayerState(INT PlayerIndex, FLOAT TimeStamp);
};

struct FGameEvent
{
    TArray<FLOAT> EventCountByTimePeriod;

		FGameEvent()
		{}
		FGameEvent(EEventParm)
		{
			appMemzero(this, sizeof(FGameEvent));
		}
		/** 
		 * Accumulate data for a given time period
		 * @param TimePeriod - time period slot to use (0 - game total, 1+ round total)
		 * @param Value - value to accumulate 
		 */
		void AddEventData(INT TimePeriod, FLOAT Value)
		{
			if (TimePeriod >= 0 && TimePeriod < 100) //sanity check
			{
				if (!EventCountByTimePeriod.IsValidIndex(TimePeriod))
				{
					EventCountByTimePeriod.AddZeroed(TimePeriod - EventCountByTimePeriod.Num() + 1);
				}

				check(EventCountByTimePeriod.IsValidIndex(TimePeriod));
				EventCountByTimePeriod(TimePeriod) += Value;
			}
			else
			{
				debugf(TEXT("AddEventData: Timeperiod %d way out of range."), TimePeriod);
			}
		}

		/*
		 *	Get accumulated data for a given time period
		 * @param TimePeriod - time period slot to get (0 - game total, 1+ round total)
		 */
		FLOAT GetEventData(INT TimePeriod) const
		{
			if (EventCountByTimePeriod.IsValidIndex(TimePeriod))
			{
				return EventCountByTimePeriod(TimePeriod);
			}
			else
			{
				return 0.0f;
			}
		}
	
};

struct FGameEvents
{
    TMap<INT, FGameEvent> Events;

		FGameEvents()
		{}

		/* 
		 *   Accumulate an event's data
		 * @param EventID - the event to record
		 * @param Value - the events recorded value
		 * @param TimePeriod - a given time period (0 - game total, 1+ round total) 
		 */
		void AddEvent(INT EventID, FLOAT Value, INT TimePeriod);

		/** @return Number of events in the list */
		INT Num() const
		{
			return Events.Num();
		}
	
};

struct FWeaponEvents
{
    struct FGameEvents TotalEvents;
    TArrayNoInit<struct FGameEvents> EventsByWeaponClass;

		FWeaponEvents()
		{}

		/* 
		 *   Accumulate a weapon event's data
		 * @param EventID - the event to record
		 * @param GameEventData - the event data
		 * @param TimePeriod - a given time period (0 - game total, 1+ round total) 
		 */
		void AddWeaponIntEvent(INT EventID, struct FWeaponIntEvent* GameEventData, INT TimePeriod);
	
};

struct FProjectileEvents
{
    struct FGameEvents TotalEvents;
    TArrayNoInit<struct FGameEvents> EventsByProjectileClass;

		FProjectileEvents()
		{}

		/* 
		 *   Accumulate a projectile event's data
		 * @param EventID - the event to record
		 * @param GameEventData - the event data
		 * @param TimePeriod - a given time period (0 - game total, 1+ round total) 
		 */
		void AddProjectileIntEvent(INT EventID, struct FProjectileIntEvent* GameEventData, INT TimePeriod);
	
};

struct FDamageEvents
{
    struct FGameEvents TotalEvents;
    TArrayNoInit<struct FGameEvents> EventsByDamageClass;

		FDamageEvents()
		{}

		/* 
		 *   Accumulate a kill event for a given damage type
		 * @param EventID - the event to record
		 * @param KillTypeID - the ID of the kill type recorded
		 * @param GameEventData - the event data
		 * @param TimePeriod - a given time period (0 - game total, 1+ round total) 
		 */
		void AddKillEvent(INT EventID, INT KillTypeID, struct FPlayerKillDeathEvent* GameEventData, INT TimePeriod);
		/* 
		 *   Accumulate a death event for a given damage type
		 * @param EventID - the event to record
		 * @param KillTypeID - the ID of the kill type recorded
		 * @param GameEventData - the event data
		 * @param TimePeriod - a given time period (0 - game total, 1+ round total) 
		 */
		void AddDeathEvent(INT EventID, INT KillTypeID, struct FPlayerKillDeathEvent* GameEventData, INT TimePeriod);
		/* 
		 *   Accumulate a damage event for a given damage type
		 * @param EventID - the event to record
		 * @param GameEventData - the event data
		 * @param TimePeriod - a given time period (0 - game total, 1+ round total) 
		 */
		void AddDamageIntEvent(INT EventID, struct FDamageIntEvent* GameEventData, INT TimePeriod);
	
};

struct FPawnEvents
{
    struct FGameEvents TotalEvents;
    TArrayNoInit<struct FGameEvents> EventsByPawnClass;

		FPawnEvents()
		{}

		/* 
		 *   Accumulate a pawn event for a given pawn type
		 * @param EventID - the event to record
		 * @param GameEventData - the event data
		 * @param TimePeriod - a given time period (0 - game total, 1+ round total) 
		 */
		void AddPlayerSpawnEvent(INT EventID, struct FPlayerSpawnEvent* GameEventData, INT TimePeriod);
	
};

struct FTeamEvents
{
    struct FGameEvents TotalEvents;
    struct FWeaponEvents WeaponEvents;
    struct FDamageEvents DamageAsPlayerEvents;
    struct FDamageEvents DamageAsTargetEvents;
    struct FProjectileEvents ProjectileEvents;
    struct FPawnEvents PawnEvents;

		FTeamEvents()
		{}

		/** 
		 * Accumulate data for a generic event
		 * @param EventID - the event to record
		 * @param TimePeriod - time period slot to use (0 - game total, 1+ round total)
		 * @param Value - value to accumulate 
		 */
		void AddEvent(INT EventID, FLOAT Value, INT TimePeriod);
		/* 
		 *   Accumulate a kill event for a given damage type
		 * @param EventID - the event to record
		 * @param KillTypeID - the ID of the kill type recorded
		 * @param GameEventData - the event data
		 * @param TimePeriod - a given time period (0 - game total, 1+ round total) 
		 */
		void AddKillEvent(INT EventID, INT KillTypeID, struct FPlayerKillDeathEvent* GameEventData, INT TimePeriod);
		/* 
		 *   Accumulate a death event for a given damage type
		 * @param EventID - the event to record
		 * @param KillTypeID - the ID of the kill type recorded
		 * @param GameEventData - the event data
		 * @param TimePeriod - a given time period (0 - game total, 1+ round total) 
		 */
		void AddDeathEvent(INT EventID, INT KillTypeID, struct FPlayerKillDeathEvent* GameEventData, INT TimePeriod);
		/* 
		 *   Accumulate a weapon event's data
		 * @param EventID - the event to record
		 * @param GameEventData - the event data
		 * @param TimePeriod - a given time period (0 - game total, 1+ round total) 
		 */
		void AddWeaponIntEvent(INT EventID, struct FWeaponIntEvent* GameEventData, INT TimePeriod);
		/* 
		 *   Accumulate a damage event for a given damage type where the team member was the attacker
		 * @param EventID - the event to record
		 * @param GameEventData - the event data
		 * @param TimePeriod - a given time period (0 - game total, 1+ round total) 
		 */
		void AddDamageDoneIntEvent(INT EventID, struct FDamageIntEvent* GameEventData, INT TimePeriod);
		/* 
		 *   Accumulate a damage event for a given damage type where the team member was the target
		 * @param EventID - the event to record
		 * @param GameEventData - the event data
		 * @param TimePeriod - a given time period (0 - game total, 1+ round total) 
		 */
		void AddDamageTakenIntEvent(INT EventID, struct FDamageIntEvent* GameEventData, INT TimePeriod);
		/* 
		 *   Accumulate a pawn event for a given pawn type
		 * @param EventID - the event to record
		 * @param GameEventData - the event data
		 * @param TimePeriod - a given time period (0 - game total, 1+ round total) 
		 */
		void AddPlayerSpawnEvent(INT EventID, struct FPlayerSpawnEvent* GameEventData, INT TimePeriod);
		/* 
		 *   Accumulate a projectile event's data
		 * @param EventID - the event to record
		 * @param GameEventData - the event data
		 * @param TimePeriod - a given time period (0 - game total, 1+ round total) 
		 */
		void AddProjectileIntEvent(INT EventID, struct FProjectileIntEvent* GameEventData, INT TimePeriod);
	
};

struct FPlayerEvents
{
    struct FGameEvents TotalEvents;
    struct FWeaponEvents WeaponEvents;
    struct FDamageEvents DamageAsPlayerEvents;
    struct FDamageEvents DamageAsTargetEvents;
    struct FProjectileEvents ProjectileEvents;
    struct FPawnEvents PawnEvents;

		FPlayerEvents()
		{}

		/** 
		 * Accumulate data for a generic event
		 * @param EventID - the event to record
		 * @param TimePeriod - time period slot to use (0 - game total, 1+ round total)
		 * @param Value - value to accumulate 
		 */
		void AddEvent(INT EventID, FLOAT Value, INT TimePeriod);
		/* 
		 *   Accumulate a kill event for a given damage type
		 * @param EventID - the event to record
		 * @param KillTypeID - the ID of the kill type recorded
		 * @param GameEventData - the event data
		 * @param TimePeriod - a given time period (0 - game total, 1+ round total) 
		 */
		void AddKillEvent(INT EventID, INT KillTypeID, struct FPlayerKillDeathEvent* GameEventData, INT TimePeriod);
		/* 
		 *   Accumulate a death event for a given damage type
		 * @param EventID - the event to record
		 * @param KillTypeID - the ID of the kill type recorded
		 * @param GameEventData - the event data
		 * @param TimePeriod - a given time period (0 - game total, 1+ round total) 
		 */
		void AddDeathEvent(INT EventID, INT KillTypeID, struct FPlayerKillDeathEvent* GameEventData, INT TimePeriod);
		/* 
		 *   Accumulate a weapon event's data
		 * @param EventID - the event to record
		 * @param GameEventData - the event data
		 * @param TimePeriod - a given time period (0 - game total, 1+ round total) 
		 */
		void AddWeaponIntEvent(INT EventID, struct FWeaponIntEvent* GameEventData, INT TimePeriod);
		/* 
		 *   Accumulate a damage event for a given damage type where the player was the attacker
		 * @param EventID - the event to record
		 * @param GameEventData - the event data
		 * @param TimePeriod - a given time period (0 - game total, 1+ round total) 
		 */
		void AddDamageDoneIntEvent(INT EventID, struct FDamageIntEvent* GameEventData, INT TimePeriod);
		/* 
		 *   Accumulate a damage event for a given damage type where the player was the target
		 * @param EventID - the event to record
		 * @param GameEventData - the event data
		 * @param TimePeriod - a given time period (0 - game total, 1+ round total) 
		 */
		void AddDamageTakenIntEvent(INT EventID, struct FDamageIntEvent* GameEventData, INT TimePeriod);
		/* 
		 *   Accumulate a pawn event for a given pawn type
		 * @param EventID - the event to record
		 * @param GameEventData - the event data
		 * @param TimePeriod - a given time period (0 - game total, 1+ round total) 
		 */
		void AddPlayerSpawnEvent(INT EventID, struct FPlayerSpawnEvent* GameEventData, INT TimePeriod);
		/* 
		 *   Accumulate a projectile event's data
		 * @param EventID - the event to record
		 * @param GameEventData - the event data
		 * @param TimePeriod - a given time period (0 - game total, 1+ round total) 
		 */
		void AddProjectileIntEvent(INT EventID, struct FProjectileIntEvent* GameEventData, INT TimePeriod);
	
};

struct FAggregateEventMapping
{
    INT EventID;
    INT AggregateID;
    INT TargetAggregateID;

    /** Constructors */
    FAggregateEventMapping() {}
    FAggregateEventMapping(EEventParm)
    {
        appMemzero(this, sizeof(FAggregateEventMapping));
    }
};

#define UCONST_GAMEEVENT_AGGREGATED_GAME_SPECIFIC 11000
#define UCONST_GAMEEVENT_AGGREGATED_PAWN_SPAWN 10400
#define UCONST_GAMEEVENT_AGGREGATED_WEAPON_FIRED 10300
#define UCONST_GAMEEVENT_AGGREGATED_DAMAGE_RECEIVED_MELEE_DAMAGE 10205
#define UCONST_GAMEEVENT_AGGREGATED_DAMAGE_RECEIVED_WEAPON_DAMAGE 10204
#define UCONST_GAMEEVENT_AGGREGATED_DAMAGE_DEALT_MELEE_DAMAGE 10203
#define UCONST_GAMEEVENT_AGGREGATED_DAMAGE_DEALT_WEAPON_DAMAGE 10202
#define UCONST_GAMEEVENT_AGGREGATED_DAMAGE_DEATHS 10201
#define UCONST_GAMEEVENT_AGGREGATED_DAMAGE_KILLS 10200
#define UCONST_GAMEEVENT_AGGREGATED_TEAM_ROUND_WON 10104
#define UCONST_GAMEEVENT_AGGREGATED_TEAM_MATCH_WON 10103
#define UCONST_GAMEEVENT_AGGREGATED_TEAM_GAME_SCORE 10102
#define UCONST_GAMEEVENT_AGGREGATED_TEAM_DEATHS 10101
#define UCONST_GAMEEVENT_AGGREGATED_TEAM_KILLS 10100
#define UCONST_GAMEEVENT_AGGREGATED_PLAYER_WASNORMALKILL 10007
#define UCONST_GAMEEVENT_AGGREGATED_PLAYER_NORMALKILL 10006
#define UCONST_GAMEEVENT_AGGREGATED_PLAYER_ROUND_WON 10005
#define UCONST_GAMEEVENT_AGGREGATED_PLAYER_MATCH_WON 10004
#define UCONST_GAMEEVENT_AGGREGATED_PLAYER_DEATHS 10003
#define UCONST_GAMEEVENT_AGGREGATED_PLAYER_KILLS 10002
#define UCONST_GAMEEVENT_AGGREGATED_PLAYER_TIMEALIVE 10001
#define UCONST_GAMEEVENT_AGGREGATED_DATA 10000

class UGameStatsAggregator : public UGameplayEventsHandler
{
public:
    //## BEGIN PROPS GameStatsAggregator
    class UGameStateObject* GameState;
    TArrayNoInit<struct FAggregateEventMapping> AggregatesList;
    TMap<INT, struct FAggregateEventMapping> AggregateEventsMapping;
    TArrayNoInit<struct FGameplayEventMetaData> AggregateEvents;
    struct FGameEvents AllGameEvents;
    TArrayNoInit<struct FTeamEvents> AllTeamEvents;
    TArrayNoInit<struct FPlayerEvents> AllPlayerEvents;
    struct FWeaponEvents AllWeaponEvents;
    struct FProjectileEvents AllProjectileEvents;
    struct FPawnEvents AllPawnEvents;
    struct FDamageEvents AllDamageEvents;
    //## END PROPS GameStatsAggregator

    virtual void PreProcessStream();
    virtual void PostProcessStream();
    virtual UBOOL GetAggregateMappingIDs(INT EventID,INT& AggregateID,INT& TargetAggregateID);
    DECLARE_FUNCTION(execPostProcessStream)
    {
        P_FINISH;
        this->PostProcessStream();
    }
    DECLARE_FUNCTION(execGetAggregateMappingIDs)
    {
        P_GET_INT(EventID);
        P_GET_INT_REF(AggregateID);
        P_GET_INT_REF(TargetAggregateID);
        P_FINISH;
        *(UBOOL*)Result=this->GetAggregateMappingIDs(EventID,AggregateID,TargetAggregateID);
    }
    DECLARE_CLASS(UGameStatsAggregator,UGameplayEventsHandler,0|CLASS_Config,GameFramework)
	/*
	 *   Set the game state this aggregator will use
	 * @param InGameState - game state object to use
	 */
	virtual void SetGameState(class UGameStateObject* InGameState);

	/*
	 *   GameStatsFileReader Interface (handles parsing of the data stream)
	 */
	
	// Game Event Handling
	virtual void HandleGameStringEvent(struct FGameEventHeader& GameEvent, struct FGameStringEvent* GameEventData);
	virtual void HandleGameIntEvent(struct FGameEventHeader& GameEvent, struct FGameIntEvent* GameEventData);
	virtual void HandleGameFloatEvent(struct FGameEventHeader& GameEvent, struct FGameFloatEvent* GameEventData);
	virtual void HandleGamePositionEvent(struct FGameEventHeader& GameEvent, struct FGamePositionEvent* GameEventData);

	// Team Event Handling
	virtual void HandleTeamStringEvent(struct FGameEventHeader& GameEvent, struct FTeamStringEvent* GameEventData);
	virtual void HandleTeamIntEvent(struct FGameEventHeader& GameEvent, struct FTeamIntEvent* GameEventData);
	virtual void HandleTeamFloatEvent(struct FGameEventHeader& GameEvent, struct FTeamFloatEvent* GameEventData);

	// Player Event Handling
	virtual void HandlePlayerIntEvent(struct FGameEventHeader& GameEvent, struct FPlayerIntEvent* GameEventData);
	virtual void HandlePlayerFloatEvent(struct FGameEventHeader& GameEvent, struct FPlayerFloatEvent* GameEventData);
	virtual void HandlePlayerStringEvent(struct FGameEventHeader& GameEvent, struct FPlayerStringEvent* GameEventData);
	virtual void HandlePlayerSpawnEvent(struct FGameEventHeader& GameEvent, struct FPlayerSpawnEvent* GameEventData);
	virtual void HandlePlayerLoginEvent(struct FGameEventHeader& GameEvent, struct FPlayerLoginEvent* GameEventData);
	virtual void HandlePlayerKillDeathEvent(struct FGameEventHeader& GameEvent, struct FPlayerKillDeathEvent* GameEventData);
	virtual void HandlePlayerPlayerEvent(struct FGameEventHeader& GameEvent, struct FPlayerPlayerEvent* GameEventData);
	virtual void HandlePlayerLocationsEvent(struct FGameEventHeader& GameEvent, struct FPlayerLocationsEvent* GameEventData);
	
	virtual void HandleWeaponIntEvent(struct FGameEventHeader& GameEvent, struct FWeaponIntEvent* GameEventData);
	virtual void HandleDamageIntEvent(struct FGameEventHeader& GameEvent, struct FDamageIntEvent* GameEventData);
	virtual void HandleProjectileIntEvent(struct FGameEventHeader& GameEvent, struct FProjectileIntEvent* GameEventData);

	/** 
	 * Cleanup for a given player at the end of a round
	 * @param PlayerIndex - player to cleanup/record stats for
	 */
	virtual void AddPlayerEndOfRoundStats(INT PlayerIndex);
	/** Triggered by the end of round event, adds any additional aggregate stats required */
	virtual void AddEndOfRoundStats();
	/** Triggered by the end of match event, adds any additional aggregate stats required */
	virtual void AddEndOfMatchStats();

	/** Returns the metadata associated with the given index, overloaded to access aggregate events not found in the stream directly */
	virtual const FGameplayEventMetaData& GetEventMetaData(INT EventID) const;

	/** 
	 * Get the team event container for the given team
	 * @param TeamIndex - team of interest (-1/255 are considered same team)
	 */
	FTeamEvents& GetTeamEvents(INT TeamIndex) { if (TeamIndex >=0 && TeamIndex < 255) { return AllTeamEvents(TeamIndex); } else { return AllTeamEvents(AllTeamEvents.Num() - 1); } }
	/** 
	 * Get the player event container for the given player
	 * @param PlayerIndex - player of interest (-1 is valid and returns container for "invalid player")
	 */
	FPlayerEvents& GetPlayerEvents(INT PlayerIndex) { if (PlayerIndex >=0) { return AllPlayerEvents(PlayerIndex); } else { return AllPlayerEvents(AllPlayerEvents.Num() - 1); } }
};

#undef DECLARE_CLASS
#undef DECLARE_CASTED_CLASS
#undef DECLARE_ABSTRACT_CLASS
#undef DECLARE_ABSTRACT_CASTED_CLASS
#endif // !INCLUDED_GAMEFRAMEWORK_GAMESTATS_CLASSES
#endif // !NAMES_ONLY

AUTOGENERATE_FUNCTION(UGameStateObject,-1,execReset);
AUTOGENERATE_FUNCTION(UGameStateObject,-1,execPreProcessStream);
AUTOGENERATE_FUNCTION(UGameStatsAggregator,-1,execGetAggregateMappingIDs);
AUTOGENERATE_FUNCTION(UGameStatsAggregator,-1,execPostProcessStream);
AUTOGENERATE_FUNCTION(UGameStatsAggregator,-1,execPreProcessStream);

#ifndef NAMES_ONLY
#undef AUTOGENERATE_FUNCTION
#endif

#ifdef STATIC_LINKING_MOJO
#ifndef GAMEFRAMEWORK_GAMESTATS_NATIVE_DEFS
#define GAMEFRAMEWORK_GAMESTATS_NATIVE_DEFS

#define AUTO_INITIALIZE_REGISTRANTS_GAMEFRAMEWORK_GAMESTATS \
	UGameStateObject::StaticClass(); \
	GNativeLookupFuncs.Set(FName("GameStateObject"), GGameFrameworkUGameStateObjectNatives); \
	UGameStatsAggregator::StaticClass(); \
	GNativeLookupFuncs.Set(FName("GameStatsAggregator"), GGameFrameworkUGameStatsAggregatorNatives); \

#endif // GAMEFRAMEWORK_GAMESTATS_NATIVE_DEFS

#ifdef NATIVES_ONLY
FNativeFunctionLookup GGameFrameworkUGameStateObjectNatives[] = 
{ 
	MAP_NATIVE(UGameStateObject, execReset)
	MAP_NATIVE(UGameStateObject, execPreProcessStream)
	{NULL, NULL}
};

FNativeFunctionLookup GGameFrameworkUGameStatsAggregatorNatives[] = 
{ 
	MAP_NATIVE(UGameStatsAggregator, execGetAggregateMappingIDs)
	MAP_NATIVE(UGameStatsAggregator, execPostProcessStream)
	MAP_NATIVE(UGameStatsAggregator, execPreProcessStream)
	{NULL, NULL}
};

#endif // NATIVES_ONLY
#endif // STATIC_LINKING_MOJO

#ifdef VERIFY_CLASS_SIZES
VERIFY_CLASS_OFFSET_NODIE(UGameStateObject,GameStateObject,TeamStates)
VERIFY_CLASS_OFFSET_NODIE(UGameStateObject,GameStateObject,MaxRoundNumber)
VERIFY_CLASS_SIZE_NODIE(UGameStateObject)
VERIFY_CLASS_OFFSET_NODIE(UGameStatsAggregator,GameStatsAggregator,GameState)
VERIFY_CLASS_OFFSET_NODIE(UGameStatsAggregator,GameStatsAggregator,AllDamageEvents)
VERIFY_CLASS_SIZE_NODIE(UGameStatsAggregator)
#endif // VERIFY_CLASS_SIZES
#endif // !ENUMS_ONLY

#if SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif
