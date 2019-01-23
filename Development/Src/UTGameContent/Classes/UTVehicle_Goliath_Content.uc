/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVehicle_Goliath_Content extends UTVehicle_Goliath;

/** The fuel cans placed around the tank*/
var StaticMeshComponent FuelCan[4];
/** non-fuel dressing for the tank */
var StaticMeshComponent MeshDressing[7];
/** health level of the mesh dressing  might want to move into a skelcontrol (see TakeDamage)*/
var float ExtraPiecesHealth[11];

var MaterialInstance BurnOutMaterialTread[2];

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	if ( bDeleteMe )
		return;

	if (WorldInfo.NetMode != NM_DedicatedServer && Mesh != None)
	{
		// set up material instance (for overlay effects)
		LeftTreadMaterialInstance = Mesh.CreateMaterialInstance(1);
		RightTreadMaterialInstance = Mesh.CreateMaterialInstance(2);
	}

	Mesh.AttachComponentToSocket(AntennaMesh,'AntennaSocket');

	AntennaBeamControl = UTSkelControl_CantileverBeam(AntennaMesh.FindSkelControl('Beam'));

	if(AntennaBeamControl != none)
	{
		AntennaBeamControl.EntireBeamVelocity = GetVelocity;
	}

	Mesh.AttachComponentToSocket(FuelCan[0],'FuelCan01Socket');
	Mesh.AttachComponentToSocket(FuelCan[1],'FuelCan02Socket');
	Mesh.AttachComponentToSocket(FuelCan[2],'FuelCan03Socket');
	Mesh.AttachComponentToSocket(FuelCan[3],'FuelCan04Socket');
	Mesh.AttachComponentToSocket(MeshDressing[0],'Pack01socket');
	Mesh.AttachComponentToSocket(MeshDressing[1],'Pack02socket');
	Mesh.AttachComponentToSocket(MeshDressing[2],'Pack03socket');
	Mesh.AttachComponentToSocket(MeshDressing[3],'Rack01Socket');
	Mesh.AttachComponentToSocket(MeshDressing[4],'Rack02Socket');
	Mesh.AttachComponentToSocket(MeshDressing[5],'Wheel01Socket');
	Mesh.AttachComponentToSocket(MeshDressing[6],'Wheel02Socket');

	Seats[1].TurretControllers[0].LagDegreesPerSecond = 240;
	Seats[1].TurretControllers[1].LagDegreesPerSecond = 240;
}

simulated function MaybeBreak(optional name BoneName)
{
	local int ConstraintIndex;

	if (BoneName == '')
	{
		ConstraintIndex = Mesh.FindConstraintIndex('Turret');
		if (ConstraintIndex != -1)
		{
			BoneName = 'Turret';
		}
	}
	Super.MaybeBreak(BoneName);
	if (BoneName == 'Turret')
	{
		// blow the turret into the air as well, for some drama:
		Mesh.AddImpulse(Vect(0,0,5000),Vect(0,0,0),BoneName);
	}
}

simulated function VehicleWeaponImpactEffects(vector HitLocation, int SeatIndex)
{
	local UTEmitter E;
	local rotator HitDir;
	local vector EffectLocation;

	Super.VehicleWeaponImpactEffects(HitLocation, SeatIndex);

	if (SeatIndex == 1)
	{
		EffectLocation = GetEffectLocation(SeatIndex);

		HitDir = rotator(HitLocation - EffectLocation);

		E = Spawn(class'UTEmitter',,, EffectLocation, HitDir);
		E.SetTemplate(ParticleSystem'VH_Goliath.Effects.P_MiniGun_Tracer');
		E.LifeSpan = VSize(HitLocation - EffectLocation) / 1200.0;
		E.SetVectorParameter('BeamEndPoint', HitLocation);
	}
}

simulated event TakeDamage(int Damage, Controller EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
{
	local int i;
	local float BestDistSq, DistSq;
	local int BestCan;
	local int BestPart;
	local UTBrokenCarPart part;
	local vector Offset;
	local UTEmitter E;

	if (DamageType == class'UTDmgType_MantaBolt')
	{
		Damage *= 0.80;
	}

	// init these vars to there being no best part in case we have no parts
	BestCan = -1;
	BestPart = -1;

	Super.TakeDamage(Damage, EventInstigator, HitLocation, Momentum, DamageType, HitInfo, DamageCauser);

	if(WorldInfo.NetMode != NM_DedicatedServer) // just a visual effect right now, if we want gameplay change this as needed
	{
		BestCan=-1;
		BestDistSq=10000.0f; // More than 100 units (10,000 = 100^2) and it doesn't effect the tanks
		for(i=0;i<4;++i)
		{
			if(FuelCan[i] != none)
			{
				DistSq = VSizeSq(FuelCan[i].GetPosition()-HitLocation);
				if(DistSq < BestDistSq)
				{
					BestDistSq = DistSq;
					BestCan=i;
				}
			}
		}
		for(i=0;i<7;++i)
		{
			if(MeshDressing[i] != none)
			{
				DistSq = VSizeSq(MeshDressing[i].GetPosition()-HitLocation);
				if(DistSq < BestDistSq)
				{
					BestCan = -1;
					BestPart = i;
					BestDistSq = DistSq;
				}
			}
		}
		if(BestCan != -1)
		{
			ExtraPiecesHealth[BestCan] -= Damage * (100/BestDistSq); // closer means more damage to the tank.
			if(ExtraPiecesHealth[BestCan] < 0) // blow the tank
			{
				// offset is to move away from the tank so it won't interpenetrate on spawn
				offset = (FuelCan[BestCan].GetPosition() - location)/15;
				part = Spawn(class'UTProj_PartDressing',,,FuelCan[BestCan].GetPosition()+offset,rotation);
				part.mesh.SetRBAngularVelocity(momentum+VRand()*0.5);
				part.mesh.AddImpulse(Vect(0,0,250));
				DetachComponent(FuelCan[BestCan]);
				FuelCan[BestCan].SetStaticMesh(none);
				FuelCan[BestCan] = none;
				E = Spawn(class'UTEmitter',,, part.Location, rotation);
				E.SetTemplate(ParticleSystem'VH_Goliath.Effects.P_VH_Goliath_Explo_Gascan');
				E.LifeSpan = 4.0;
			}
		}
		else if(BestPart != -1)
		{
			ExtraPiecesHealth[BestPart+4] -= Damage * (100/BestDistSq);
			if(ExtraPiecesHealth[BestPart+4] < 0) // part falls
			{
				offset = (MeshDressing[BestPart].GetPosition() - location)/15;
				part = Spawn(class'UTProj_PartDressing',,,MeshDressing[BestPart].GetPosition()+offset,rotation);
				part.mesh.SetStaticMesh(MeshDressing[BestPart].StaticMesh);
				MeshDressing[BestPart].SetStaticMesh(none);
				MeshDressing[BestPart] = none;
				part.mesh.AddImpulse(Vect(0,0,100));
			}
		}
	}
}

simulated function SwitchToRagDoll()
{
	local rotator TurretRotation;

	TurretRotation = QuatToRotator(Mesh.GetBoneQuaternion('Object01')) + RagdollRotationOffset;

	Super.SwitchToRagdoll();

	Mesh.SetRBRotation(TurretRotation, 'Turret');
}

simulated function CauseMuzzleFlashLight(int SeatIndex)
{
	Super.CauseMuzzleFlashLight(SeatIndex);
	if (SeatIndex==0)
		VehicleEvent('GoliathTurret');
	else if (SeatIndex==1)
		VehicleEvent('GoliathMachineGun');
}

exec function ToggleTread()
{
	local UTSkelControl_TankTread T;
	T = UTSkelControl_TankTread(mesh.FindSkelControl('TreadControl'));
	T.bDormant = !T.bDormant;
}

// @TODO FIXMESTEVE remove
// temporary for tweaking culldistance of attached pieces
exec function Culld(float F)
{
	local int i;

	for ( i=0; i<4; i++ )
	{
		FuelCan[i].SetCullDistance(F);
	}

	for ( i=0; i<7; i++ )
	{
		MeshDressing[i].SetCullDistance(F);
	}
	AntennaMesh.SetCullDistance(F);
}
simulated function SetBurnOut()
{
	// material 1 = left tread, 2 = right tread
	if (BurnOutMaterialTread[GetTeamNum()] != None)
	{
		Mesh.SetMaterial(1,BurnOutMaterialTread[GetTeamNum()]);
		Mesh.SetMaterial(2,BurnOutMaterialTread[GetTeamNum()]);
	}
	super.SetBurnOut();
}

defaultproperties
{
	Begin Object Name=CollisionCylinder
		CollisionHeight=60.0
		CollisionRadius=260.0
		Translation=(X=0.0,Y=0.0,Z=0.0)
	End Object
	CylinderComponent=CollisionCylinder

	Begin Object Class=CylinderComponent Name=RagdollCollisionCylinder
		CollisionHeight=60.0
		CollisionRadius=280.0
		Translation=(X=80.0,Y=0.0,Z=-75.0)
	End Object
	RagdollCylinder=RagdollCollisionCylinder

	Begin Object Name=SVehicleMesh
		SkeletalMesh=SkeletalMesh'VH_Goliath.Mesh.SK_VH_Goliath01'
		MorphSets[0]=MorphTargetSet'VH_Goliath.Mesh.SK_VH_Goliath_Morph'
		AnimTreeTemplate=AnimTree'VH_Goliath.Anims.AT_VH_Goliath'
		PhysicsAsset=PhysicsAsset'VH_Goliath.Mesh.PA_VH_Goliath'
	End Object

	DamageMorphTargets(0)=(InfluenceBone=Chasis,AssociatedSkelControl=none,MorphNodeName=MorphNodeW_Front,LinkedMorphNodeName=none,Health=200,DamagePropNames=(Damage2))
	DamageMorphTargets(1)=(InfluenceBone=Suspension_LHS_01,AssociatedSkelControl=none,MorphNodeName=MorphNodeW_Back,LinkedMorphNodeName=none,Health=200,DamagePropNames=(Damage3))
	DamageMorphTargets(2)=(InfluenceBone=Suspension_LHS_02,AssociatedSkelControl=none,MorphNodeName=MorphNodeW_LHS,LinkedMorphNodeName=none,Health=200,DamagePropNames=(Damage1))
	DamageMorphTargets(3)=(InfluenceBone=Suspension_RHS_02,AssociatedSkelControl=none,MorphNodeName=MorphNodeW_RHS,LinkedMorphNodeName=none,Health=200,DamagePropNames=(Damage1))
	DamageMorphTargets(4)=(InfluenceBone=Object01,AssociatedSkelControl=none,MorphNodeName=MorphNodeW_Turret,LinkedMorphNodeName=none,Health=200,DamagePropNames=(Damage5))

	Begin Object Class=AudioComponent name=AmbientSoundComponent
		bShouldRemainActiveIfDropped=true
		bStopWhenOwnerDestroyed=true
		SoundCue=SoundCue'A_Vehicle_Goliath.SoundCues.A_Vehicle_Goliath_TurretFire_Cue'
	End Object
	MachineGunAmbient=AmbientSoundComponent
	Components.Add(AmbientSoundComponent)

	DrawScale=1.35

	// turret twist sound.
	Begin Object Class=AudioComponent Name=TurretTwistSound
		SoundCue=SoundCue'A_Vehicle_SPMA.SoundCues.A_Vehicle_SPMA_CannonRotate'
	End Object
	Components.Add(TurretTwistSound);


	Seats(0)={(	GunClass=class'UTVWeap_GoliathTurret',
				GunSocket=(TurretFireSocket),
				GunPivotPoints=(Object01),
				TurretVarPrefix="",
				TurretControls=(TurretPitch,TurretRotate),
				CameraTag=GunViewSocket,
				CameraOffset=-420,
				MuzzleFlashLightClass=class'UTTankMuzzleFlash')}

	Seats(1)={(	GunClass=class'UTVWeap_GoliathMachineGun',
				GunSocket=(GunFireSocket),
				GunPivotPoints=(Object10),
				TurretVarPrefix="Gunner",
				TurretControls=(gun_rotate,gun_pitch),
				CameraTag=GunViewSocket,
				CameraOffset=-64,
				CameraBaseOffset=(Z=30.0),
				MuzzleFlashLightClass=class'UTTankeMinigunMuzzleFlashLight')}

	// Muzzle Flashes
	VehicleEffects(0)=(EffectStartTag=GoliathTurret,EffectTemplate=ParticleSystem'VH_Goliath.Effects.PS_Goliath_Cannon_MuzzleFlash',EffectSocket=TurretFireSocket)
	VehicleEffects(1)=(EffectStartTag=GoliathMachineGun,EffectTemplate=ParticleSystem'VH_Goliath.Effects.P_Goliath_MiniGun_MuzzleFlash',EffectSocket=GunFireSocket)

	// Exhaust smoke
	VehicleEffects(2)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Goliath.Effects.PS_Goliath_Exhaust_Smoke',EffectSocket=Exhaust_Smoke01)
	VehicleEffects(3)=(EffectStartTag=EngineStart,EffectEndTag=EngineStop,EffectTemplate=ParticleSystem'VH_Goliath.Effects.PS_Goliath_Exhaust_Smoke',EffectSocket=Exhaust_Smoke02)

	// Damage
	VehicleEffects(4)=(EffectStartTag=DamageSmoke,EffectEndTag=NoDamageSmoke,bRestartRunning=false,EffectTemplate=ParticleSystem'Envy_Effects.Vehicle_Damage.P_Vehicle_Damage_1_Goliath',EffectSocket=DamageSmoke01)

	// Sounds
	// Engine sound.
	Begin Object Class=AudioComponent Name=ScorpionEngineSound
		SoundCue=SoundCue'A_Vehicle_Goliath.SoundCues.A_Vehicle_Goliath_EngineLoop'
	End Object
	EngineSound=ScorpionEngineSound
	Components.Add(ScorpionEngineSound);

	// Collision sound.
	Begin Object Class=AudioComponent Name=ScorpionCollideSound
		SoundCue=SoundCue'A_Vehicle_Goliath.SoundCues.A_Vehicle_Goliath_Collide'
	End Object
	CollideSound=ScorpionCollideSound
	Components.Add(ScorpionCollideSound);

	EnterVehicleSound=SoundCue'A_Vehicle_Goliath.SoundCues.A_Vehicle_Goliath_Start'
	ExitVehicleSound=SoundCue'A_Vehicle_Goliath.SoundCues.A_Vehicle_Goliath_Stop'

	WheelParticleEffects[0]=(MaterialType=Dirt,ParticleTemplate=None)
	WheelParticleEffects[1]=(MaterialType=Water,ParticleTemplate=ParticleSystem'Envy_Level_Effects_2.Vehicle_Water_Effects.P_Goliath_Water_Splash')

	// Initialize sound parameters.
	CollisionIntervalSecs=1.0
	SquealThreshold=250.0
	EngineStartOffsetSecs=2.0
	EngineStopOffsetSecs=1.0

	IconXStart=0
	IconYStart=0.5
	IconXWidth=0.25
	IconYWidth=0.25

	SpawnInTemplates[0]=ParticleSystem'VH_Goliath.Effects.P_VH_Goliath_Spawn_Red'
	SpawnInTemplates[1]=ParticleSystem'VH_Goliath.Effects.P_VH_Goliath_Spawn_Blue'
	SpawnInTime=5.00

	RagdollMesh=SkeletalMesh'VH_Goliath.Mesh.SK_VH_Goliath01_Destroyed'
	RagdollAsset=PhysicsAsset'VH_Goliath.Mesh.SK_VH_Goliath01_Destroyed_Physics'
	BigExplosionTemplate=ParticleSystem'VH_Goliath.Effects.P_VH_Goliath_DeathExplode'

	TreadSpeedParameterName=Goliath_Tread_Speed

	FlagBone=Object01
	FlagOffset=(X=-95.0,Y=59,Z=50)

	Begin Object Class=SkeletalMeshComponent Name=SAntennaMesh
		SkeletalMesh=SkeletalMesh'VH_Goliath.Mesh.SK_VH_Goliath_Antenna'
		AnimTreeTemplate=AnimTree'VH_Goliath.Anims.AT_VH_Goliath_Antenna'
		ShadowParent = SVehicleMesh
		BlockRigidBody=false
		LightEnvironment = MyLightEnvironment
		PhysicsWeight=0.0
		TickGroup=TG_PostASyncWork
		bUseAsOccluder=FALSE
		CullDistance=1300.0
		CollideActors=false
		bUpdateSkelWhenNotRendered=false
		bIgnoreControllersWhenNotRendered=true
	End Object
	AntennaMesh=SAntennaMesh

	Begin Object Class=StaticMeshComponent Name=Fuel01
		StaticMesh=StaticMesh'VH_Goliath.Mesh.S_VH_Goliath_FuelCan02'
		LightEnvironment=MyLightEnvironment
		ShadowParent=SVehicleMesh
		BlockRigidBody=false
		TickGroup=TG_PostASyncWork
		bUseAsOccluder=FALSE
		CullDistance=5000.0
		CollideActors=false
	End Object
	FuelCan[0]=Fuel01

	Begin Object Class=StaticMeshComponent Name=Fuel02
		StaticMesh=StaticMesh'VH_Goliath.Mesh.S_VH_Goliath_FuelCan02'
		LightEnvironment=MyLightEnvironment
		BlockRigidBody=false
		ShadowParent=SVehicleMesh
		TickGroup=TG_PostASyncWork
		bUseAsOccluder=FALSE
		CullDistance=5000.0
		CollideActors=false
	End Object
	FuelCan[1]=Fuel02

	Begin Object Class=StaticMeshComponent Name=Fuel03
		StaticMesh=StaticMesh'VH_Goliath.Mesh.S_VH_Goliath_FuelCan02'
		LightEnvironment=MyLightEnvironment
		BlockRigidBody=false
		ShadowParent=SVehicleMesh
		TickGroup=TG_PostASyncWork
		bUseAsOccluder=FALSE
		CullDistance=5000.0
		CollideActors=false
	End Object
	FuelCan[2]=Fuel03

	Begin Object Class=StaticMeshComponent Name=Fuel04
		StaticMesh=StaticMesh'VH_Goliath.Mesh.S_VH_Goliath_FuelCan02'
		LightEnvironment=MyLightEnvironment
		ShadowParent=SVehicleMesh
		BlockRigidBody=false
		TickGroup=TG_PostASyncWork
		bUseAsOccluder=FALSE
		CullDistance=5000.0
		CollideActors=false
	End Object
	FuelCan[3]=Fuel04

	Begin Object Class=StaticMeshComponent Name=Dressing01
		StaticMesh=StaticMesh'VH_Goliath.Mesh.S_VH_Goliath_Pack01'
		LightEnvironment=MyLightEnvironment
		ShadowParent=SVehicleMesh
		BlockRigidBody=false
		TickGroup=TG_PostASyncWork
		bUseAsOccluder=FALSE
		CullDistance=5000.0
		CollideActors=false
	End Object
	MeshDressing[0]=Dressing01

	Begin Object Class=StaticMeshComponent Name=Dressing02
		StaticMesh=StaticMesh'VH_Goliath.Mesh.S_VH_Goliath_Pack02'
		LightEnvironment=MyLightEnvironment
		ShadowParent=SVehicleMesh
		BlockRigidBody=false
		TickGroup=TG_PostASyncWork
		bUseAsOccluder=FALSE
		CullDistance=5000.0
		CollideActors=false
	End Object
	MeshDressing[1]=Dressing02

	Begin Object Class=StaticMeshComponent Name=Dressing03
		StaticMesh=StaticMesh'VH_Goliath.Mesh.S_VH_Goliath_Pack03'
		LightEnvironment=MyLightEnvironment
		ShadowParent=SVehicleMesh
		BlockRigidBody=false
		TickGroup=TG_PostASyncWork
		bUseAsOccluder=FALSE
		CullDistance=5000.0
		CollideActors=false
	End Object
	MeshDressing[2]=Dressing03

	Begin Object Class=StaticMeshComponent Name=Dressing04
		StaticMesh=StaticMesh'VH_Goliath.Mesh.S_VH_Goliath_Rack01'
		LightEnvironment=MyLightEnvironment
		BlockRigidBody=false
		ShadowParent=SVehicleMesh
		TickGroup=TG_PostASyncWork
		bUseAsOccluder=FALSE
		CullDistance=5000.0
		CollideActors=false
	End Object
	MeshDressing[3]=Dressing04

	Begin Object Class=StaticMeshComponent Name=Dressing05
		StaticMesh=StaticMesh'VH_Goliath.Mesh.S_VH_Goliath_Rack01'
		LightEnvironment=MyLightEnvironment
		BlockRigidBody=false
		ShadowParent=SVehicleMesh
		TickGroup=TG_PostASyncWork
		bUseAsOccluder=FALSE
		CullDistance=5000.0
		CollideActors=false
	End Object
	MeshDressing[4]=Dressing05

	Begin Object Class=StaticMeshComponent Name=Dressing06
		StaticMesh=StaticMesh'VH_Goliath.Mesh.S_VH_Goliath_Wheel01'
		LightEnvironment=MyLightEnvironment
		BlockRigidBody=false
		ShadowParent=SVehicleMesh
		TickGroup=TG_PostASyncWork
		bUseAsOccluder=FALSE
		CullDistance=5000.0
		CollideActors=false
	End Object
	MeshDressing[5]=Dressing06

	Begin Object Class=StaticMeshComponent Name=Dressing07
		StaticMesh=StaticMesh'VH_Goliath.Mesh.S_VH_Goliath_Wheel02'
		LightEnvironment=MyLightEnvironment
		BlockRigidBody=false
		ShadowParent=SVehicleMesh
		TickGroup=TG_PostASyncWork
		bUseAsOccluder=FALSE
		CullDistance=5000.0
		CollideActors=false
	End Object
	MeshDressing[6]=Dressing07


	// Note: These are not direct damage numbers; distance is attenuated in, so numbers should generally be low
	ExtraPiecesHealth[0]=5.0
	ExtraPiecesHealth[1]=5.0
	ExtraPiecesHealth[2]=5.0
	ExtraPiecesHealth[3]=5.0
	ExtraPiecesHealth[4]=5.0
	ExtraPiecesHealth[5]=5.0
	ExtraPiecesHealth[6]=5.0
	ExtraPiecesHealth[7]=5.0
	ExtraPiecesHealth[8]=5.0
	ExtraPiecesHealth[9]=5.0
	ExtraPiecesHealth[10]=5.0

	ExplosionSound=SoundCue'A_Vehicle_Goliath.SoundCues.A_Vehicle_Goliath_Explode'

	HoverBoardAttachSockets=(HoverAttach00,HoverAttach01)

	TeamMaterials[0]=MaterialInstanceConstant'VH_Goliath.Materials.MI_VH_Goliath01_Red'
	TeamMaterials[1]=MaterialInstanceConstant'VH_Goliath.Materials.MI_VH_Goliath01_Blue'

	PawnHudScene=VHud_Goliath'UI_Scenes_HUD.VehicleHUDs.VH_Goliath'

	DrivingPhysicalMaterial=PhysicalMaterial'vh_goliath.physmat_goliathdriving'
	DefaultPhysicalMaterial=PhysicalMaterial'vh_goliath.physmat_goliath'

	BurnOutMaterial[0]=MaterialInstance'VH_Goliath.Materials.MI_VH_Goliath01_Red_BO'
	BurnOutMaterial[1]=MaterialInstance'VH_Goliath.Materials.MI_VH_Goliath01_Blue_BO'
	BurnOutMaterialTread[0]=MaterialInstance'VH_Goliath.Materials.MI_VH_Goliath01_Red_Tread_BO'
	BurnOutMaterialTread[1]=MaterialInstance'VH_Goliath.Materials.MI_VH_Goliath01_Blue_Tread_BO'
	PassengerTeamBeaconOffset=(X=-100.0f,Y=0.0f,Z=125.0f);
}
