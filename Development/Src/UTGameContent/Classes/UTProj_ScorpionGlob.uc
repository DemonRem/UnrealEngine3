/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_ScorpionGlob extends UTProjectile;

var int BounceCount;
var repnotify Pawn SeekPawn;
var ParticleSystem BounceTemplate;

replication
{
	if ( bNetInitial )
		SeekPawn;
}

simulated event ReplicatedEvent(name VarName)
{
	if ( varName == 'SeekPawn' )
	{
		if ( SeekPawn != None )
		{
			SetTimer(0.1, true, 'SeekUpdate');
		}
	}
	else
	{
		super.ReplicatedEvent(varname);
	}
}


simulated event PhysicsVolumeChange(PhysicsVolume NewVolume)
{
	if ( PhysicsVolume.bWaterVolume && !NewVolume.bWaterVolume )
		Buoyancy = 0.5 * (Buoyancy + 1.08);
}

simulated event Landed(vector HitNormal, Actor FloorActor)
{
	FloorActor.TakeDamage(Damage, InstigatorController, Location, MomentumTransfer * Normal(Velocity), MyDamageType,, self);
	Explode(Location, HitNormal);
}

/**
 * Initalize the Projectile
 */
function Init(vector Direction)
{
	Super.Init(Direction);
	UpdateSeeking(true);
}

simulated function HitWall( Vector HitNormal, Actor Wall, PrimitiveComponent WallComp )
{
	local UTEmitter Effect;

	bBlockedByInstigator = true;
	if ( (BounceCount > 4) || (Pawn(Wall) != None) || VSizeSq(Velocity) < 40000.0 )
	{
		Wall.TakeDamage(Damage, InstigatorController, Location, MomentumTransfer * Normal(Velocity), MyDamageType,, self);
		Explode(Location, HitNormal);
		return;
	}
	if ( WorldInfo.NetMode != NM_DedicatedServer )
	{
		PlaySound(ImpactSound, true);
		Effect = Spawn(class'UTEmitter',,,, rotator(HitNormal));
		Effect.SetTemplate(BounceTemplate, true);
	}
	Velocity = 0.9 * sqrt(Abs(HitNormal.Z)) * (Velocity - 2*(Velocity dot HitNormal)*HitNormal);   // Reflect off Wall w/damping
	if (Velocity.Z > 400)
	{
		Velocity.Z = 0.5 * (400 + Velocity.Z);
	}

	UpdateSeeking(false);
}

simulated function UpdateSeeking(bool bInitialSeek)
{
	local float VelMag, Dist, BestDist, SavedVelZ;
	local Pawn P, BestP;
	local vector PDir, VelDir;

	// @todo fixmesteve move to C++
	// seeking bounce
	if ( Instigator != None )
	{
		BestDist = 5000;
		VelDir = Velocity;
		VelDir.Z = 0;
		VelMag = VSize2D(VelDir);
		if ( VelMag > 0 )
		{
			VelDir = Normal(VelDir);
			ForEach WorldInfo.AllPawns(class'Pawn', P)
			{
				if ( (P.PlayerReplicationInfo != None) && !P.bCanFly && !WorldInfo.GRI.OnSameTeam(P,Instigator) )
				{
					PDir = Normal(P.Location - Location);
					if ( (PDir dot VelDir) > 0.7 )
					{
						Dist = VSize(PDir);
						if ( Abs(PDir.Z) < Dist )
						{
							PDir = P.Location + P.Velocity*(VelMag/Dist);
							PDir.Z = 0;

							if ( (Dist < BestDist) && ((PDir dot VelDir) > 0.7) && FastTrace(Location, P.Location) )
							{
								BestDist = Dist;
								BestP = P;
							}
						}
					}
				}
			}
			if ( BestP != None )
			{
				if ( bInitialSeek )
				{
					SeekPawn = BestP;
				}
				else
				{
					SavedVelZ = Velocity.Z;
					Velocity = VelMag * Normal(BestP.Location + BestP.Velocity*(BestDist/VelMag) - Location);
					Velocity.Z = SavedVelZ;

					// only track UTHoverVehicles
					SeekPawn = UTHoverVehicle(BestP);
				}
			}
			else
			{
				SeekPawn = None;
			}
			if ( SeekPawn != None )
			{
				SetTimer(0.1, true, 'SeekUpdate');
			}
			else
			{
				ClearTimer('SeekUpdate');
			}
		}
	}
	BounceCount++;
}

simulated function SeekUpdate()
{
	local float VelMag;

	if ( (SeekPawn == None) || SeekPawn.bDeleteMe )
	{
		SeekPawn = None;
		ClearTimer('SeekUpdate');
	}

	if ( (SeekPawn != None) && (VSizeSq(SeekPawn.Location - Location) < 4000000.0) && FastTrace(Location, SeekPawn.Location) )
	{
		VelMag = VSize(Velocity);
		Velocity = VelMag * Normal(Normal(Velocity) + Normal(SeekPawn.Location - Location));
	}
}

simulated function ProcessTouch(Actor Other, Vector HitLocation, Vector HitNormal)
{
	if ( Other != Instigator )
	{
		Other.TakeDamage(Damage, InstigatorController, HitLocation, MomentumTransfer * Normal(Velocity), MyDamageType,, self);
		Explode(HitLocation, HitNormal);
	}
}

/**
 * Explode this glob
 */
simulated function Explode(Vector HitLocation, vector HitNormal)
{
	SpawnExplosionEffects(HitLocation, HitNormal );
	Shutdown();
}

simulated function MyOnParticleSystemFinished(ParticleSystemComponent PSC)
{
	return;
}

defaultproperties
{
	bBounce=True
	Speed=3000.0
	MaxSpeed=3000.0
	Damage=36.0
	MomentumTransfer=40000
	MyDamageType=class'UTDmgType_ScorpionGlob'
	LifeSpan=7.0
	MaxEffectDistance=7000.0
	Buoyancy=1.5
	TossZ=0.0
	CheckRadius=40.0

	ProjFlightTemplate=ParticleSystem'VH_Scorpion.Effects.P_Scorpion_Bounce_Projectile'
	BounceTemplate=ParticleSystem'VH_Scorpion.Effects.P_VH_Scorpion_Fireimpact'
	ProjExplosionTemplate=ParticleSystem'VH_Scorpion.Effects.PS_Scorpion_Gun_Impact'

	Explosionsound=SoundCue'A_Weapon.BioRifle.Cue.A_Weapon_Bio_BurstSmall01_Cue'
	ImpactSound=SoundCue'A_Weapon.BioRifle.Cue.A_Weapon_Bio_BurstLarge01_Cue'
	Physics=PHYS_Falling

	Components.Remove(ProjectileMesh)
}
