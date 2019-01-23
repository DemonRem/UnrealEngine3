/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_SpiderMine extends UTProjectile;

var float DetectionTimer; // check target every this many seconds
/** check for targets within this many units of us */
var float DetectionRange;
/** extra range beyond DetectionRange in which we keep an already acquired target */
var float KeepTargetExtraRange;
var float ScurrySpeed, ScurryAnimRate;
var float HeightOffset;

var     Pawn    TargetPawn;
var     int     TeamNum;

var	bool	bClosedDown;
var	bool	bGoToTargetLoc;
var	vector	TargetLoc;
var	int	TargetLocFuzz;

var SkeletalMeshComponent Mesh;

//var UTWeap_SpiderMineLauncher MyWeapon;

replication
{
	if (bNetDirty && Role == ROLE_Authority)
		bGoToTargetLoc;
	if (bNetDirty && Role == ROLE_Authority && bGoToTargetLoc)
		TargetLoc;
	if (bNetDirty && Role == ROLE_Authority && !bGoToTargetLoc)
		TargetPawn;
}

simulated function PlayAnim(name NewAnim, float Rate, optional bool bLooped)
{
	local AnimNodeSequence Sequence;

	// do not play on a dedicated server
	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		// Check we have access to mesh and animations
		Sequence = AnimNodeSequence(Mesh.Animations);
		if (Mesh != None && Sequence != None)
		{
			// Set right animation sequence if needed
			if (Sequence.AnimSeq == None || Sequence.AnimSeq.SequenceName != NewAnim)
			{
				Sequence.SetAnim(NewAnim);
			}

			if (Sequence.AnimSeq == None)
			{
				`Warn("Animation" @ NewAnim @ "not found");
			}
			else
			{
				// Play Animation
				Sequence.PlayAnim(bLooped, Rate);
			}
		}
	}
}

simulated function StopAnim()
{

	// do not play on a dedicated server
	if ( WorldInfo.NetMode == NM_DedicatedServer )
		return;

	// Check we have access to mesh and animations
	if ( Mesh != None && AnimNodeSequence(Mesh.Animations) != None )
	{
		AnimNodeSequence(Mesh.Animations).StopAnim();
	}
}



simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	if (Role < ROLE_Authority && Physics == PHYS_None)
	{
		bProjTarget = true;
		GotoState('OnGround');
	}

	PlayAnim('SpiderStartup', 1.0);
}

function Init(vector Direction)
{
	Super.Init(Direction);

	SetRotation(Rotation + rot(16384,0,0));
}

simulated function Destroyed()
{
// 	if (Role == ROLE_Authority && MyWeapon != None)
// 	{
// 		MyWeapon.MineDied(self);
// 	}

	Super.Destroyed();
}

simulated function ProcessTouch(Actor Other, Vector HitLocation, Vector HitNormal)
{
	if ( UTProj_SpiderMine(Other) != none )
	{
		if ( Other.Instigator != Instigator )
		{
			Explode(Location, HitNormal);
		}
		else if (Physics == PHYS_Falling)
			Velocity = vect(0,0,200) + 150 * VRand();
	}
	else if (Pawn(Other) != None)
	{
		if ( Other != Instigator && !WorldInfo.GRI.OnSameTeam(Instigator, Other) )
		{
			Explode(Location, HitNormal);
		}
	}
	else if (Other.bCanBeDamaged && Other.Base != self)
	{
		Explode(Location, HitNormal);
	}
}

simulated function AdjustSpeed()
{
	ScurrySpeed = default.ScurrySpeed / (Attached.length + 1);
	ScurryAnimRate = default.ScurryAnimRate / (Attached.length + 1);
}

simulated function Attach(Actor Other)
{
	AdjustSpeed();
}

simulated function Detach(Actor Other)
{
	AdjustSpeed();
}

simulated function byte GetTeamNum()
{
	return TeamNum;
}

function AcquireTarget()
{
	local Pawn A;
	local float Dist, BestDist;

	TargetPawn = None;

	foreach VisibleCollidingActors(class'Pawn', A, DetectionRange)
	{
		if ( A != Instigator && A.Health > 0 && !WorldInfo.GRI.OnSameTeam(A, self)
		 && (UTVehicle(A) == None || UTVehicle(A).Driver != None || UTVehicle(A).bTeamLocked) )
		{
			Dist = VSize(A.Location - Location);
			if (TargetPawn == None || Dist < BestDist)
			{
				TargetPawn = A;
				BestDist = Dist;
			}
		}
	}
}

function WarnTarget()
{
	if (TargetPawn != None && TargetPawn.Controller != None)
	{
		TargetPawn.Controller.ReceiveProjectileWarning(self);

		if (AIController(TargetPawn.Controller) != None && AIController(TargetPawn.Controller).Skill >= 5.0 && !TargetPawn.IsFiring())
		{
			TargetPawn.Controller.Focus = self;
			TargetPawn.Controller.FireWeaponAt(self);
		}
	}
}

event TakeDamage(int DamageAmount, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
{
	if (DamageAmount > 0 && !WorldInfo.GRI.OnSameTeam(EventInstigator, self))
	{
		Explode(Location, Normal(Momentum));
	}
}

function PhysicsVolumeChange(PhysicsVolume NewVolume)
{
	if (NewVolume.bPainCausing)
	{
		Explode(Location, vector(Rotation));
	}
}

auto state Flying
{
	simulated event Landed( vector HitNormal, actor FloorActor )
	{
		local rotator NewRot;

	//        if ( WorldInfo.NetMode != NM_DedicatedServer )
	//            PlaySound(ImpactSound);

		// FIXME when I get a real sound

		bProjTarget = True;

		NewRot = rotator(HitNormal);
		NewRot.Pitch -= 16384;

		SetRotation(NewRot);

		GotoState(TargetPawn != None ? 'Scurrying' : 'OnGround');
	}

	simulated function HitWall( vector HitNormal, Actor Wall, PrimitiveComponent WallComp )
	{
	    	if (bBounce)
	    	{
			if (Wall != None && Instigator != None && Pawn(Wall) != None)
			{
				if (!WorldInfo.GRI.OnSameTeam(Instigator, Wall))
				{
					Explode(Location, HitNormal);
				}
				/* FIXME:
				else
				{
					bUseCollisionStaticMesh = True;
				}
				*/
			}
			bBounce = false;
		}
		else
		{
			Explode(Location, HitNormal);
		}
	}

	simulated function BeginState(Name PreviousStateName)
	{
		SetPhysics(PHYS_Falling);
	}
}

simulated state OnGround
{
	simulated function Timer()
	{
		if (Role < ROLE_Authority)
		{
			if (TargetPawn != None)
			{
				GotoState('Scurrying');
			}
			else if (bGoToTargetLoc)
			{
				GotoState('ScurryToTargetLoc');
			}

			return;
		}

		if (Instigator == None)
		{
			Explode(Location, vector(Rotation));
		}

		AcquireTarget();

		if (TargetPawn != None)
		{
			GotoState('Scurrying');
		}
	}

	simulated function BeginState(Name PreviousStateName)
	{
		SetPhysics(PHYS_None);
		Velocity = vect(0,0,0);
		SetTimer(DetectionTimer, True);
		Timer();
	}

	simulated function EndState(Name NextStateName)
	{
		SetTimer(0, False);
	}

Begin:
	Sleep(0.4);
	PlayAnim('SpiderCloseDown',1.0);
	bClosedDown = true;
}

simulated state Scurrying
{
	simulated function Timer()
	{
		local vector NewLoc;
		local rotator TargetDirection;
		local float TargetDist;

		if (TargetPawn == None)
		{
			GotoState('Flying');
		}
		else if (Physics == PHYS_Walking)
		{
			NewLoc = TargetPawn.Location - Location;
			TargetDist = VSize(NewLoc);

			if (TargetDist < DetectionRange + KeepTargetExtraRange)
			{
				NewLoc.Z = 0.f;
				Velocity = Normal(NewLoc) * ScurrySpeed;
				if (TargetDist < 225.0)
				{
					GotoState('Flying');
					Velocity *= 1.2;
					Velocity.Z = 350;
					WarnTarget();
				}
				else
				{
					TargetDirection = Rotator(NewLoc);
					TargetDirection.Yaw -= 16384;
					TargetDirection.Roll = 0;
					SetRotation(TargetDirection);
					PlayAnim('SpiderScurry', ScurryAnimRate,true);
				}
			}
			else
			{
				TargetPawn = None;
				GotoState('Flying');
			}
		}
	}

	simulated event Landed(vector HitNormal, Actor FloorActor)
	{
		SetPhysics(PHYS_Walking);
		SetBase(FloorActor);
	}

	function BeginState(Name PreviousStateName)
	{
		WarnTarget();
	}

	simulated function EndState(Name NextStateName)
	{
		StopAnim();
		SetTimer(0.0, False);
	}

Begin:
	SetPhysics(PHYS_Walking);
	if (bClosedDown)
	{
		PlayAnim('SpiderStartUp', 1.0);
		bClosedDown = false;
		Sleep(0.25);
	}
	SetTimer(DetectionTimer * 0.5, true);
}

simulated function Explode(vector HitLocation, vector HitNormal)
{
	local int i;

	//make sure anything attached gets blown up too
	for (i = 0; i < Attached.length; i++)
	{
		Attached[i].TakeDamage(Damage, InstigatorController, Attached[i].Location, vect(0,0,0), MyDamageType,, self);
	}

	Super.Explode(HitLocation, HitNormal);
}

function SetScurryTarget(vector NewTargetLoc)
{
	if (TargetPawn == None)
	{
		TargetLoc = NewTargetLoc + VRand() * Rand(TargetLocFuzz);
		bGoToTargetLoc = true;
		GotoState('ScurryToTargetLoc');
	}
}

simulated state ScurryToTargetLoc extends Scurrying
{
	function SetScurryTarget(vector NewTargetLoc)
	{
		TargetLoc = NewTargetLoc + VRand() * Rand(TargetLocFuzz);
	}

	simulated function Timer()
	{
		local vector NewLoc;
		local rotator TargetDirection;

		if (Physics == PHYS_Walking)
		{
			NewLoc = TargetLoc - Location;
			NewLoc.Z = 0;
			if (VSize(NewLoc) < 250.f)
			{
				AcquireTarget();
				if (TargetPawn != None)
				{
					GotoState('Scurrying');
				}
				else
				{
					GotoState('Flying');
				}
			}
			else
			{
				Velocity = Normal(NewLoc) * ScurrySpeed;
				TargetDirection = rotator(NewLoc);
				TargetDirection.Yaw -= 16384;
				TargetDirection.Roll = 0;
				SetRotation(TargetDirection);
				PlayAnim('SpiderScurry', ScurryAnimRate,true);
			}
		}
	}

	simulated function EndState(Name NextStateName)
	{
		SetTimer(0.0, false);
		bGoToTargetLoc = false;
	}
}

defaultproperties
{
	Components.Remove(ProjectileMesh)

	Begin Object class=AnimNodeSequence Name=MeshSequenceA
	End Object

	Begin Object Class=SkeletalMeshComponent Name=MeshComponentA
		SkeletalMesh=SkeletalMesh'WP_MineLayer.Mesh.SK_WP_MineLayer_Mine'
		AnimSets(0)=AnimSet'WP_MineLayer.Anims.K_WP_MineLayer_Mine'
		Animations=MeshSequenceA
		AlwaysLoadOnClient=true
		AlwaysLoadOnServer=true
		CastShadow=false
		Scale=0.2
		Translation=(Z=-6.0)
		bUseAsOccluder=FALSE
	End Object
	Mesh=MeshComponentA
	Components.Add(MeshComponentA)

	Begin Object Name=CollisionCylinder
		CollisionRadius=10
		CollisionHeight=10
		CollideActors=True
	End Object

	ProjExplosionTemplate=ParticleSystem'WP_RocketLauncher.Effects.P_WP_RocketLauncher_RocketExplosion'
	ExplosionSound=SoundCue'A_Weapon.CanisterGun.Cue.A_Weapon_CGSpider_Death_Cue'
	ExplosionLightClass=class'UTGame.UTRocketExplosionLight'

	MyDamageType=class'UTDmgType_SpiderMine'
	Speed=800.0
	MaxSpeed=800.0
	ScurrySpeed=525.0
	ScurryAnimRate=4.1
	TossZ=0.0
	Damage=95.0
	DamageRadius=250.0
	MomentumTransfer=50000
	Physics=PHYS_Falling
	RotationRate=(Pitch=20000)
	DetectionTimer=0.50
	DetectionRange=750.0
	KeepTargetExtraRange=250.0
	bProjTarget=true
	bCollideWorld=True
	bBlockActors=true

	RemoteRole=ROLE_SimulatedProxy
	bNetTemporary=False
	bUpdateSimulatedPosition=True
	LifeSpan=0.0
	TargetLocFuzz=250

	bBounce=True
	bHardAttach=True
}
