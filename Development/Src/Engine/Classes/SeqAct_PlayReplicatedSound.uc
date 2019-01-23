/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


/** sound player that replicates the sound to all clients instead of playing it locally */
class SeqAct_PlayReplicatedSound extends SequenceAction
	deprecated; // OBSOLETE - SeqAct_PlaySound now supports replication

var() SoundCue SoundToPlay;

/** if this array contains any elements, the sound is only replicated to the client that owns these actors */
var array<Actor> OnlyPlayFor;

event Activated()
{
	local Actor A;
	local PlayerController PC;
	local Pawn P;
	local int i;

	if (OnlyPlayFor.length > 0)
	{
		for (i = 0; i < OnlyPlayFor.length; i++)
		{
			// find the PlayerController
			PC = PlayerController(OnlyPlayFor[i]);
			if (PC == None)
			{
				P = Pawn(OnlyPlayFor[i]);
				if (P != None)
				{
					PC = PlayerController(P.Controller);
				}
			}
			if (PC != None)
			{
				// play the sound on all target actors for this PC
				if (Targets.length > 0)
				{
					for (i = 0; i < Targets.length; i++)
					{
						A = Actor(Targets[i]);
						if (A != None)
						{
							PC.ClientHearSound(SoundToPlay, A, A.Location, false, !PC.FastTrace(A.Location, PC.GetViewTarget().Location));
						}
					}
				}
				else
				{
					PC.ClientPlaySound(SoundToPlay);
				}
			}
		}
	}
	else
	{
		if (Targets.length > 0)
		{
			for (i = 0; i < Targets.length; i++)
			{
				A = Actor(Targets[i]);
				if (A != None)
				{
					A.PlaySound(SoundToPlay);
				}
			}
		}
		else
		{
			// play the sound globally instead
			foreach GetWorldInfo().AllControllers(class'PlayerController', PC)
			{
				PC.ClientPlaySound(SoundToPlay);
			}
		}
	}
}

defaultproperties
{
	bCallHandler=false
	ObjName="Play Replicated Sound"
	ObjCategory="Sound"
	ObjClassVersion=2

	VariableLinks(1)=(ExpectedType=class'SeqVar_Object',LinkDesc="Only Play For",PropertyName=OnlyPlayFor)
}
