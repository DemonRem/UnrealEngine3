/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SeqAct_ActorFactory extends SeqAct_Latent
	native(Sequence);

cpptext
{
	virtual void PostEditChange(UProperty* PropertyThatChanged);

	void Activated();
	virtual UBOOL UpdateOp(FLOAT DeltaTime);
	void DeActivated();

	virtual void Spawned(UObject *NewSpawn);

	/**
	 * Checks any of the bEnabled inputs and sets the new value.
	 */
	void CheckToggle()
	{
		if (InputLinks(1).bHasImpulse)
		{
			bEnabled = TRUE;
		}
		else
		if (InputLinks(2).bHasImpulse)
		{
			bEnabled = FALSE;
		}
		else
		if (InputLinks(3).bHasImpulse)
		{
			bEnabled = !bEnabled;
		}
	}
};

enum EPointSelection
{
	/** Try each spawn point in a linear method */
	PS_Normal,
	/** Pick the first available randomly selected point */
	PS_Random,
	/** PS_Normal, but in reverse */
	PS_Reverse,
};

/** Is this factory enabled? */
var() bool bEnabled;

/** Is this factory currently in the process of spawning? */
var bool bIsSpawning;

/** Type of actor factory to use when creating the actor */
var() export editinline ActorFactory Factory;

/** Method of spawn point selection */
var() EPointSelection				PointSelection;

/** Set of points where Objects will be spawned */
var() array<Actor> SpawnPoints;

/** Number of actors to create */
var() int							SpawnCount;

/** Delay applied after creating an actor before creating the next one */
var() float							SpawnDelay;

/** Prevent spawning at locations with bBlockActors */
var() bool							bCheckSpawnCollision;

/** Last index used to spawn at, for PS_Normal/PS_Reverse */
var int LastSpawnIdx;

/** Number of actors spawned so far */
var int	SpawnedCount;

/** Remaining time before attempting the next spawn */
var float RemainingDelay;


defaultproperties
{
	ObjName="Actor Factory"
	ObjCategory="Actor"

	InputLinks(0)=(LinkDesc="Spawn Actor")
	InputLinks(1)=(LinkDesc="Enable")
	InputLinks(2)=(LinkDesc="Disable")
	InputLinks(3)=(LinkDesc="Toggle")

	VariableLinks.Empty
	VariableLinks(0)=(ExpectedType=class'SeqVar_Object',LinkDesc="Spawn Point",PropertyName=SpawnPoints)
	VariableLinks(1)=(ExpectedType=class'SeqVar_Object',LinkDesc="Spawned",MinVars=0,bWriteable=true)
	VariableLinks(2)=(ExpectedType=class'SeqVar_Int',LinkDesc="Spawn Count",PropertyName=SpawnCount)

	SpawnCount=1
	SpawnDelay=0.5f
	bCheckSpawnCollision=TRUE
	LastSpawnIdx=-1
	bEnabled=TRUE
}
