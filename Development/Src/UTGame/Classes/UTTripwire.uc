/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTTripWire extends UTDeployedActor;

var repnotify vector EndPoint;
var repnotify vector EndNormal;
var vector ScaleFactor;
var repnotify staticmeshcomponent TheBeam;
var repnotify bool bIsActive;
var controller LaidBy;
var() bool bCanBounce;
var float MaxLength;
var float BounceCost;
var float LengthScale;
var SoundCue ActivatedSound;
var SoundCue RemovedSound;
// How long it takes to die from EMP
var() float EMPtoDeathTime;

var int FlickerCount;
var int MaxFlickers;
var UTTripWire child;
/** We use a delegate so that different types of creators can get the OnDestroyed event */
delegate OnDeployableUsedUp(actor ChildDeployable);


replication
{
	if (Role == ROLE_Authority)// && bNetDirty)
		EMPtoDeathTime, bIsActive,Endpoint;
}

simulated event ReplicatedEvent(name VarName)
{
	local vector HitLoc, HitNorm;
	if(VarName=='EndPoint')
	{
		// calc EndNormal:
		Trace(HitLoc,HitNorm,EndPoint,location,false);
		EndNormal = HitNorm;
		SetLaserLocation();
	}
	else if(VarName == 'bIsActive')
	{
		if(bIsActive)
			MakeActive();
		else
			MakeInactive();
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}
simulated event Destroyed()
{
	super.Destroyed();
	PlaySound(RemovedSound);
	if(Role == ROLE_AUTHORITY && child != none)
		child.Destroy();

}

simulated function SetLaserLocation()
{
	local float calcResult;
	local vector trans;

	calcResult = VSize(location-EndPoint);
	calcResult /= LengthScale;// HACK FOR USING FAKE ASSET
	scaleFactor.x = calcResult;
	scaleFactor.z = 1.0;
	scaleFactor.y = 1.0;
	trans = (EndPoint-location)/2;


	TheBeam.SetScale3d(scaleFactor);
	TheBeam.SetTranslation(trans<<rotation); // middle of beam.

}
reliable server function StartLaser(vector End, vector Normal)
{
	local vector HitLoc, HitNorm, reflection;
	EndNormal = Normal;
	EndPoint = End;
	SetLaserLocation();

	reflection = ((End-location) - 2.0 * Normal * ((End-location) dot Normal));
	reflection = reflection/VSize(reflection);
	if(bCanBounce && MaxLength > BounceCost && Trace(HitLoc, HitNorm, ((MaxLength - scaleFactor.x*LengthScale)*reflection)+EndPoint, EndPoint,false) != none)// *LengthScale undo hack
	{
		Child = Spawn(self.class, self,, End, rotator(reflection) );
		if(Child != none)
		{
			Child.MaxLength = MaxLength - scaleFactor.x*LengthScale; // *LengthScale undo hack
			Child.MaxLength -= BounceCost; // cost of a bounce
			Child.StartLaser(HitLoc, HitNorm);
		}

	}
}
simulated function MakeActiveInTime(float TimeTillActive)
{
	setTimer(TimeTillActive,false,'MakeActive');
}
simulated function MakeActive()
{
	bIsActive = true;
	Mesh=TheBeam;
	CollisionComponent = TheBeam;
	//SetPhysics(PHYS_Flying);
	TheBeam.SetOnlyOwnerSee(false);
	if(ActivatedSound != none)
	{
		PlaySound(ActivatedSound);
	}
	if(ROLE == ROLE_AUTHORITY && Child != none && Child.LaidBy == none)
	{
		Child.MakeActiveInTime(GetTimerCount('MakeActive'));
		if(UTTripWire(Owner) != none)
		{
			LaidBy = UTTripWire(Owner).LaidBy;
		}
		else
		{
			LaidBy = Pawn(Owner).Controller;
		}
	}
}
simulated function MakeInactive()
{
	CollisionComponent = none;
	TheBeam.SetOnlyOwnerSee(true);
	bIsActive=false;
	PlaySound(RemovedSound);
	ActivatedSound=none;
}

simulated function DestroyInTime(float TimeTillDestroyed)
{
	setTimer(TimeTillDestroyed*0.11f,false,'Flicker');
	setTimer(TimeTillDestroyed,false,'Destroy'); // @FIXME: TJAMES, for some reason this doesn't work :(
	if(child != none)
		child.DestroyInTime(TimeTillDestroyed);
}
simulated function HitByEMP()
{
	if(bEMPDisables)
	{
		DestroyInTime(EMPtoDeathTime);
	}
}

simulated function Flicker()
{
	if((FlickerCount++)%2==0)
	{
		MakeInactive();
		if(FlickerCount>MaxFlickers+1)
		{
			Destroy();
		}
	}
	else
	{
		MakeActive();
	}
	if(Role == ROLE_AUTHORITY) // server needs to dictate the flicker times
	{
		SetTimer(EMPtoDeathTime*frand()*0.5f,false,'Flicker');
	}
}

function RopeHit(UTPawn Victim, vector HitLocation, vector HitNormal)
{
	/*
	local ImpactInfo impact;
	//local vector closest;

	Impact.HitActor = Victim;
	Impact.HitLocation = HitLocation;
	Impact.HitNormal = HitNormal;
	Impact.RayDir = -HitNormal;
	if(Victim.IsLocationOnHead(Impact,1.0f))
	*/
		Victim.Died(LaidBy,class'UTDmgType_LaserWire',HitLocation);
}
event Touch( Actor Other, PrimitiveComponent OtherComp, vector HitLocation, vector HitNormal )
{
	local UTPawn victim;
	local vector ActualHit, ActualNorm,extent;
	Super.touch(other,OtherComp,hitlocation,hitnormal);
	Victim = UTPawn(Other);

	if(bIsActive && Victim != none)
	{
		extent=Vect(1,1,1);
		if(TraceComponent(ActualHit, ActualNorm, Victim.CollisionComponent, EndPoint, location,extent))
		{
			RopeHit(Victim, ActualHit, ActualNorm);
		}
	}

}
simulated function PostBeginPlay()
{
	Super.PostBeginPlay();
}
defaultproperties
{
	Health=500
/*
	Begin Object Class=StaticMeshComponent Name=DeployableMesh
		StaticMesh=StaticMesh'GamePlaceholders.SM.Mesh.S_HU_Deco_SM_FanBox'
		CollideActors=false
		BlockActors=false
		Scale3D=(X=0.1,Y=0.1,Z=0.75)
		Translation=(Z=-85.0)
		CastShadow=false
		bUseAsOccluder=FALSE
	End Object
	Components.Add(DeployableMesh)
*/

    // content move me
	Begin Object Class=StaticMeshComponent Name=LaserVolume
		StaticMesh=StaticMesh'Pickups.Deployables.Mesh.S_LASERWIRE_BEAM'
		CollideActors=true
		BlockActors=false
		CastShadow=false;
		bOnlyOwnerSee = true;
		AlwaysLoadOnClient=true;
		AlwaysLoadOnServer=true;
		bUseAsOccluder=FALSE
	End Object
	TheBeam = LaserVolume;
	Components.Add(LaserVolume);

	Physics=PHYS_Flying
	bIsActive = false

	Components.Remove(CollisionCylinder)
	Mesh=LaserVolume;
	CollisionComponent =none; // ON PURPOSE until MakeActive is called.

	bCollideActors=true
	bCollideWorld=true
	bBlockActors=false
	bIgnoreEncroachers=true


	bIgnoreRigidBodyPawns=true
	bCanBounce=true;
	bounceCost = 50;
	MaxLength = 1500;
	ActivatedSound = SoundCue'A_Pickups.Deployables.Sounds.TripwireActivatedCue'
	RemovedSound=SoundCue'A_Pickups.Deployables.Sounds.LaserOutCue'
	LengthScale=19;
	bCanTakeDamage=false
	EMPToDeathTime=1.0f
	MaxFlickers=6;
}
