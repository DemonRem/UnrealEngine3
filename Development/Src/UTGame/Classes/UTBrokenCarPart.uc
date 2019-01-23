/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 *
 */

class UTBrokenCarPart extends UTProjectile;

var StaticMeshComponent Mesh;

/** The part's light environment */
var LightEnvironmentComponent LightEnvironment;

simulated function ProcessTouch(Actor Other, Vector HitLocation, Vector HitNormal);

/**
 * Handled lands just like a wall hit and bounce unless our velocity is small
 */
simulated event Landed( vector HitNormal, actor FloorActor )
{
	Destroy();
}

/**
 * Give a little bounce
 */
simulated event HitWall(vector HitNormal, Actor Wall, PrimitiveComponent WallComp)
{
	Velocity = 0.8*(( Velocity dot HitNormal ) * HitNormal * (-2.0) + Velocity);   // Reflect off Wall w/damping
		
	if (Velocity.Z > 400)
	{
		Velocity.Z = 0.5 * (400 + Velocity.Z);
	}
	
	if ( VSize(Velocity) < 64 ) 
	{
		Landed(Location, none);
	}
	
}

simulated event tick(float Deltatime)
{
	local rotator NewRotation;
	
	super.Tick(DeltaTime);
	
	NewRotation = Rotation;
	NewRotation.Pitch -= 16384 * DeltaTime;
	SetRotation( NewRotation );
}

simulated event PostBeginPlay()
{
	Super.PostBeginPlay();
	Settimer(0.5, false, 'FixCollision');
}

simulated function Fixcollision()
{
	SetCollision(true,false);
}


defaultproperties
{
	RemoteRole=ROLE_None
	Physics=PHYS_Falling
	bBounce=true

	Begin Object Class=DynamicLightEnvironmentComponent Name=MyLightEnvironment
	End Object
	Components.Add(MyLightEnvironment)
	LightEnvironment=MyLightEnvironment

	Begin Object Class=StaticMeshComponent Name=ProjectileMesh
		CullDistance=12000
		CastShadow=true
		CollideActors=false
		BlockActors=false
		BlockZeroExtent=false
		BlockNonZeroExtent=false
		BlockRigidBody=false
		bAcceptsLights=true
		Scale=1.0
		LightEnvironment=MyLightEnvironment
		bUseAsOccluder=FALSE
	End Object
	Components.Add(ProjectileMesh)
	
	CollisionComponent=ProjectileMesh
	Mesh=ProjectileMesh

	bCollideActors=false
	bCollideWorld=true
	LifeSpan=+006.000000

	MaxSpeed=1300
	Speed=800
	TossZ=350

}
