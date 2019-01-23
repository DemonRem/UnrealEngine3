class UTVehicleDeathPiece extends UTGib_Vehicle
	notplaceable;

var ParticleSystemComponent PSC;
var StaticMeshComponent SMMesh;

defaultproperties
{
	Begin Object Name=GibStaticMeshComp
		BlockActors=false
		CollideActors=true
		BlockRigidBody=true
		CastShadow=false
		bCastDynamicShadow=false
		bNotifyRigidBodyCollision=true
		ScriptRigidBodyCollisionThreshold=1.0
		bUseCompartment=true
		RBCollideWithChannels=(Default=true,Pawn=true,Vehicle=true,GameplayPhysics=true,EffectPhysics=true)
		bUseAsOccluder=FALSE
	End Object
	SMMesh = GibStaticMeshComp
	Components.Add(GibStaticMeshComp);

	Begin Object Class=ParticleSystemComponent Name=Particles
		Template=ParticleSystem'Envy_Effects.VH_Deaths.P_VH_Death_SpecialCase_1_Attach'
		bAutoActivate=true
	End Object
	Components.Add(Particles)
	PSC = Particles

	Physics=PHYS_RigidBody
	TickGroup=TG_PostAsyncWork
	RemoteRole=ROLE_None


	Lifespan=10.0

	CollisionComponent=GibStaticMeshComp
}