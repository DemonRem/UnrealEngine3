/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_ScorpionGlob_Base extends UTProjectile
	abstract
	native;

var int BounceCount;

/** Max bounces before explode on bounce */
var int MaxBounces;

var Pawn SeekPawn;
var ParticleSystem BounceTemplate;

/** Used in TickSpecial() seeking code */
var float LastTraceTime;

/** How well to track */
var float TrackingFactor;

cpptext
{
	virtual void TickSpecial(float DeltaTime);
}

replication
{
	if ( bNetInitial )
		SeekPawn;
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
	local float VelMag, Dist, BestDist, SavedVelZ;
	local Pawn P;
	local vector PDir, VelDir;

	Super.Init(Direction);

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
					if ( (PDir dot VelDir) > 0.8 )
					{
						Dist = VSize(PDir);
						if ( Abs(PDir.Z) < Dist )
						{
							PDir = P.Location + P.Velocity*(VelMag/Dist);
							PDir.Z = 0;

							if ( (Dist < BestDist) && ((PDir dot VelDir) > 0.7) && FastTrace(Location, P.Location) )
							{
								BestDist = Dist;
								SeekPawn = P;
							}
						}
					}
				}
			}
			if ( SeekPawn != None )
			{
				SavedVelZ = Velocity.Z;
				Velocity = VelMag * Normal(SeekPawn.Location + SeekPawn.Velocity*(BestDist/VelMag) - Location);
				Velocity.Z = SavedVelZ;

				// only track UTHoverVehicles really well
				TrackingFactor = (UTHoverVehicle(SeekPawn) != None) ? 10.0 : 5.0;;
			}
		}
	}
}

simulated function HitWall( Vector HitNormal, Actor Wall, PrimitiveComponent WallComp )
{
	bBlockedByInstigator = true;
	if ( (BounceCount > MaxBounces) || (Pawn(Wall) != None) || VSizeSq(Velocity) < 40000.0 )
	{
		Wall.TakeDamage(Damage, InstigatorController, Location, MomentumTransfer * Normal(Velocity), MyDamageType,, self);
		Explode(Location, HitNormal);
		return;
	}
	if ( WorldInfo.NetMode != NM_DedicatedServer )
	{
		PlaySound(ImpactSound, true);
		WorldInfo.MyEmitterPool.SpawnEmitter(BounceTemplate, Location, rotator(HitNormal));
	}
	Velocity = 0.9 * sqrt(Abs(HitNormal.Z)) * (Velocity - 2*(Velocity dot HitNormal)*HitNormal);   // Reflect off Wall w/damping
	if (Velocity.Z > 400)
	{
		Velocity.Z = 0.5 * (400 + Velocity.Z);
	}
	BounceCount++;
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
	LifeSpan=4.0
	MaxEffectDistance=7000.0
	Buoyancy=1.5
	TossZ=0.0
	CheckRadius=40.0
	MaxBounces=3
	Physics=PHYS_Falling
}
