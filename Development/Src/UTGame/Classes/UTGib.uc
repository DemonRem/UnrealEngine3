/**
 * base class for gibs 
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTGib extends Actor
	config(Game)
	native
	abstract;

/** sound played when we hit a wall */
var SoundCue HitSound;

/** the component that will be set to the chosen mesh */
var MeshComponent GibMeshComp;

/** the component that will be set to the chosen mesh */
var private SkeletalMeshComponent GibMesh_SkeletalMesh;
/** the component that will be set to the chosen mesh */
var private StaticMeshComponent GibMesh_StaticMesh;


/** This is the MIC to use for the gib **/
var MaterialInstanceConstant MIC_Gib;

/** This is the MIC to use for the decal that this gib may spawn **/
var MaterialInstance MIC_Decal;

/** PSC for the GibEffect (e.g. for the link gun we play a little lightning) **/
var ParticleSystemComponent PSC_GibEffect;


struct native StaticMeshDatum
{
	var StaticMesh TheStaticMesh;

	var SkeletalMesh TheSkelMesh;
	var PhysicsAsset ThePhysAsset;
};

/** list of generic gib meshes from which one will be chosen at random */
var array<StaticMeshDatum> GibMeshesData;


cpptext
{
	void TickSpecial( FLOAT DeltaSeconds );
}

simulated event PreBeginPlay()
{
	Super.PreBeginPlay();

	ChooseGib();
}


/** This will choose between the two types of gib meshes that we can have. **/
simulated function ChooseGib()
{
	local StaticMeshDatum SMD;
	local StaticMeshComponent SMC;
	local SkeletalMeshComponent SKMC;

	SMD = GibMeshesData[Rand(GibMeshesData.length)];

	// do the static mesh version of the gib
	if( SMD.ThePhysAsset == NONE )
	{
		GibMeshComp=GibMesh_StaticMesh;
		CollisionComponent=GibMesh_StaticMesh;

		SMC = StaticMeshComponent(GibMeshComp);
		SMC.SetStaticMesh( SMD.TheStaticMesh );
		AttachComponent(GibMeshComp);
	}
	// do the skeletal mesh version of the gib
	else
	{
		GibMeshComp=GibMesh_SkeletalMesh;
		CollisionComponent=GibMesh_SkeletalMesh;

		SKMC = SkeletalMeshComponent(GibMeshComp);
		SKMC.SetSkeletalMesh( SMD.TheSkelMesh );
		SKMC.SetPhysicsAsset( SMD.ThePhysAsset );
		AttachComponent(GibMeshComp);
		SKMC.SetHasPhysicsAssetInstance( TRUE ); // this need to comes after the AttachComponent
	}
}


function Timer()
{
	local PlayerController PC;

	ForEach LocalPlayerControllers(class'PlayerController', PC)
	{
		if( PC.ViewTarget == self )
		{
			SetTimer( 4.0, FALSE );
			return;
		}
	}

	Destroy();
}

event BecomeViewTarget( PlayerController PC )
{
	SetHidden( TRUE );
	LifeSpan = 0;
	SetTimer( 4.0, FALSE );
}


/**
 *	Calculate camera view point, when viewing this actor.
 *
 * @param	fDeltaTime	delta time seconds since last update
 * @param	out_CamLoc	Camera Location
 * @param	out_CamRot	Camera Rotation
 * @param	out_FOV		Field of View
 *
 * @return	true if Actor should provide the camera point of view.
 */
simulated function bool CalcCamera( float fDeltaTime, out vector out_CamLoc, out rotator out_CamRot, out float out_FOV )
{
	if( Physics != PHYS_None )
	{
		out_CamRot = Rotation;
	}

	out_CamLoc = Location;
	return false;
}

simulated event RigidBodyCollision( PrimitiveComponent HitComponent, PrimitiveComponent OtherComponent,
					const CollisionImpactData RigidCollisionData, int ContactIndex )
{
	// don't spawn a decal unless we have more than likely hit something other than the orig pawn or other Gibs
	if( (WorldInfo.TimeSeconds - CreationTime ) > 0.4 )
	{
		//`log( "RBC: " $ self $ " against " $ HitComponent );

		PlaySound( HitSound, TRUE );
		GibMeshComp.SetNotifyRigidBodyCollision( FALSE );

		// only do one decal
		if( MIC_Decal == none )
		{
			LeaveADecal( Location, vect(0,0,0) );
		}
	}
}


/** Overridden by the derived classes to do their specific form of decal leaving (e.g. blood for bodies, green goop for aliens, etc.) **/
simulated function LeaveADecal( vector HitLoc, vector HitNorm );


simulated function Tick( float DeltaTime )
{
	local float CurrVal;

	Super.Tick( DeltaTime );

	// @todo move to native land in tick special
	if( MIC_Decal != None  )
	{
		MIC_Decal.GetScalarParameterValue( 'DissolveAmount', CurrVal );
		CurrVal += DeltaTime/4; // this is too fast for the value we have atm.  Need the new system for the scaling N to N+K over S seconds
		MIC_Decal.SetScalarParameterValue( 'DissolveAmount', CurrVal );

		//`log( self $ " setting BloodAlpha to: " $ CurrVal );
	}
	else
	{
		//`log( self @ " Disabling TICK!!!!! " $ DecalScaleValue );
		Disable( 'Tick' );
	}
}


simulated function StartDissolving()
{
	Enable( 'Tick' );
}

 

defaultproperties
{
	Begin Object Class=StaticMeshComponent Name=GibStaticMeshComp
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
	GibMesh_StaticMesh=GibStaticMeshComp

	Begin Object Class=SkeletalMeshComponent Name=GibSkeletalMeshComp
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
		bUpdateSkelWhenNotRendered=false
		bHasPhysicsAssetInstance=false
		bAcceptsDecals=false
	End Object
	GibMesh_SkeletalMesh=GibSkeletalMeshComp

	TickGroup=TG_PostAsyncWork
	RemoteRole=ROLE_None
	Physics=PHYS_RigidBody
	bNoEncroachCheck=true
	bCollideActors=true
	bBlockActors=false
	bWorldGeometry=false
	bCollideWorld=true
	bProjTarget=true
	LifeSpan=8.0
	bGameRelevant=true

	HitSound=SoundCue'A_Character_BodyImpacts.BodyImpacts.A_Character_BodyImpact_GibMix_Cue'
}
