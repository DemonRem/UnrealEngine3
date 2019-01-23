/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTProj_PartDressing extends UTBrokenCarPart;

simulated function FixCollision()
{
}
simulated event PostBeginPlay()
{
	super.PostBeginPlay();
	Mesh.WakeRigidBody();
}

simulated event HitWall(vector HitNormal, Actor Wall, PrimitiveComponent WallComp)
{
	Velocity = 0.8*(( Velocity dot HitNormal ) * HitNormal * (-2.0) + Velocity);   // Reflect off Wall w/damping
		
	if (Velocity.Z > 400)
	{
		Velocity.Z = 0.5 * (400 + Velocity.Z);
	}
	
}

simulated event Landed( vector HitNormal, actor FloorActor )
{
}

simulated event Tick(float deltatime)
{
	Super(UTProjectile).Tick(deltatime); // remove rotation
}
defaultproperties
{

	Begin Object Name=ProjectileMesh
		StaticMesh=StaticMesh'VH_Goliath.Mesh.S_VH_Goliath_FuelCan02'
		Scale=1.35
		CastShadow=true
		CollideActors=true
		BlockActors=true
		BlockZeroExtent=true
		BlockNonZeroExtent=true
		BlockRigidBody=true
		RBCollideWithChannels=(Default=TRUE,GameplayPhysics=TRUE,EffectPhysics=TRUE,Vehicle=TRUE)
		RBChannel=RBCC_Vehicle
	End Object
	//Physics=PHYS_Falling
	Physics=PHYS_RIGIDBODY
	//Physics=PHYS_Projectile
		LifeSpan=+015.000000
	CollisionComponent=ProjectileMesh
	bCollideActors=true
	bCollideWorld=true
	TickGroup=TG_PostAsyncWork
	

}
