/** 
 * this class manages a pool of ParticleSystemComponents
 * it is meant to be used for single shot effects spawned during gameplay
 * to reduce object overhead that would otherwise be caused by spawning dozens of Emitters
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class EmitterPool extends Actor
	native
	config(Game);

/** template to base pool components off of - should not be used for effects or attached to anything */
var protected ParticleSystemComponent PSCTemplate;
/** components currently in the pool */
var array<ParticleSystemComponent> PoolComponents;
/** components currently active */
var array<ParticleSystemComponent> ActiveComponents;
/** maximum allowed active components - if this is greater than 0 and is exceeded, the oldest active effect is taken */
var int MaxActiveEffects;
/** option to log out the names of all active effects when the pool overflows */
var globalconfig bool bLogPoolOverflow;

/** list of components that should be relative to an Actor */
struct native EmitterBaseInfo
{
	var ParticleSystemComponent PSC;
	var Actor Base;
	var vector RelativeLocation;
	var rotator RelativeRotation;
};
var array<EmitterBaseInfo> RelativePSCs;

cpptext
{
	virtual void TickSpecial(FLOAT DeltaTime);
}

/** set to each pool component's finished delegate to return it to the pool */
function OnParticleSystemFinished(ParticleSystemComponent PSC)
{
	local int i;

	// remove from active arrays
	ActiveComponents.RemoveItem(PSC);
	i = RelativePSCs.Find('PSC', PSC);
	if (i != INDEX_NONE)
	{
		RelativePSCs.Remove(i, 1);
	}
	// if the component is already pending kill then we can't put it back in the pool
	if (!PSC.IsPendingKill())
	{
		PSC.ResetToDefaults();
		PoolComponents[PoolComponents.length] = PSC;
	}
}

/** plays the specified effect at the given location and rotation, taking a component from the pool or creating as necessary
 * @note: the component is returned so the caller can perform any additional modifications (parameters, etc),
 * 	but it shouldn't keep the reference around as the component will be returned to the pool as soon as the effect is complete
 * @param EmitterTemplate - particle system to create
 * @param SpawnLocation - location to place the effect in world space
 * @param SpawnRotation (opt) - rotation to place the effect in world space
 * @param AttachToActor (opt) - if specified, component will move along with this Actor
 * @return the ParticleSystemComponent the effect will use
 */
function ParticleSystemComponent SpawnEmitter(ParticleSystem EmitterTemplate, vector SpawnLocation, optional rotator SpawnRotation, optional Actor AttachToActor)
{
	local int i;
	local ParticleSystemComponent Result;

	// AttachToActor is only for movement, so if it can't move, then there is no point in using it
	if (AttachToActor != None && (AttachToActor.bStatic || !AttachToActor.bMovable))
	{
		AttachToActor = None;
	}

	// try to grab one from the pool
	while (PoolComponents.length > 0)
	{
		i = PoolComponents.length - 1;
		Result = PoolComponents[i];
		PoolComponents.Remove(i, 1);
		if (Result != None && !Result.IsPendingKill())
		{
			break;
		}
		else
		{
			Result = None;
		}
	}

	if (Result == None)
	{
		// clear out old entries
		i = 0;
		while (i < ActiveComponents.length)
		{
			if (ActiveComponents[i] != None && !ActiveComponents[i].IsPendingKill())
			{
				i++;
			}
			else
			{
				ActiveComponents.Remove(i, 1);
			}
		}

		if (MaxActiveEffects > 0 && ActiveComponents.length >= MaxActiveEffects)
		{
			if (bLogPoolOverflow)
			{
				`Warn("Exceeded max active pooled emitters! Effect list:");
				for (i = 0; i < MaxActiveEffects; i++)
				{
					`Log(PathName(ActiveComponents[i].Template));
				}
			}
			// overwrite oldest emitter
			Result = ActiveComponents[0];
			Result.OnSystemFinished = None; // so OnParticleSystemFinished() doesn't get called and mess with the arrays
			Result.ResetToDefaults();
			ActiveComponents.Remove(0, 1);
		}
		else
		{
			Result = new(self) PSCTemplate.Class(PSCTemplate);
		}
	}

	if (AttachToActor != None)
	{
		i = RelativePSCs.length;
		RelativePSCs.length = i + 1;
		RelativePSCs[i].PSC = Result;
		RelativePSCs[i].Base = AttachToActor;
		RelativePSCs[i].RelativeLocation = SpawnLocation - AttachToActor.Location;
		RelativePSCs[i].RelativeRotation = SpawnRotation - AttachToActor.Rotation;
	}
	Result.SetTranslation(SpawnLocation);
	Result.SetRotation(SpawnRotation);
	AttachComponent(Result);
	Result.SetTemplate(EmitterTemplate);
	ActiveComponents.AddItem(Result);
	Result.OnSystemFinished = OnParticleSystemFinished;
	return Result;
}

defaultproperties
{
	Begin Object Class=ParticleSystemComponent Name=ParticleSystemComponent0
		AbsoluteTranslation=true
		AbsoluteRotation=true
	End Object
	PSCTemplate=ParticleSystemComponent0
}
