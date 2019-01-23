// Actor used by matinee (SeqAct_Interp) objects to replicate activation, playback, and other relevant flags to net clients
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
class MatineeActor extends Actor
	native
	nativereplication;

cpptext
{
	virtual INT* GetOptimizedRepList(BYTE* Recent, FPropertyRetirement* Retire, INT* Ptr, UPackageMap* Map, UActorChannel* Channel);
	virtual void TickSpecial(FLOAT DeltaTime);
	virtual void PreNetReceive();
	virtual void PostNetReceive();
}

/** the SeqAct_Interp associated with this actor (this is set in C++ by the action that spawns this actor)
 *	on the client, the MatineeActor will tick this SeqAct_Interp and notify the actors it should be affecting
 */
var const SeqAct_Interp InterpAction;
/** properties that may change on InterpAction that we need to notify clients about, since the object's properties will not be replicated */
var bool bIsPlaying, bReversePlayback, bPaused;
var float PlayRate;
var float Position;

replication
{
	if (bNetInitial && Role == ROLE_Authority)
		InterpAction;

	if (bNetDirty && Role == ROLE_Authority)
		bIsPlaying, bReversePlayback, bPaused, PlayRate, Position;
}

/** called by InterpAction when significant changes occur. Updates replicated data. */
event Update()
{
	bIsPlaying = InterpAction.bIsPlaying;
	bReversePlayback = InterpAction.bReversePlayback;
	bPaused = InterpAction.bPaused;
	PlayRate = InterpAction.PlayRate;
	Position = InterpAction.Position;
	NetUpdateTime = WorldInfo.TimeSeconds - 1.0;
}

defaultproperties
{
	Begin Object Class=SpriteComponent Name=Sprite
		Sprite=Texture2D'EngineResources.S_Actor'
		HiddenGame=True
		AlwaysLoadOnClient=False
		AlwaysLoadOnServer=False
	End Object
	Components.Add(Sprite)

	bSkipActorPropertyReplication=true
	bAlwaysRelevant=true
	bReplicateMovement=false
	bUpdateSimulatedPosition=false
	bOnlyDirtyReplication=true
	RemoteRole=ROLE_SimulatedProxy
	NetPriority=2.7
	NetUpdateFrequency=1.0
	Position=-1.0
	PlayRate=1.0
}
