/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTCharacter extends UTPawn
	native;

enum ECharacterSkins
{
	Skin_RedTeam,
	Skin_BlueTeam,
	Skin_Normal
};

/** List of alternate materials to use for the body in certain circumstances (see enum) */
var MaterialInstance		BodySkins[ECharacterSkins.EnumCount];
/** which material in the meah is the body material, and gets replaced the BodySkin materials */
var int						BodySkinMaterialInstanceIndex;
/** List of alternate materials to use for the head in certain circumstances (see enum) */
var MaterialInstance		HeadSkins[ECharacterSkins.EnumCount];
/** which material in the meah is the head material, and gets replaced the HeadSkinSkin materials */
var int						HeadSkinMaterialInstanceIndex;

enum EFirstPersonArmSkins
{
	FPArmSkin_Normal,
	FPArmSkin_BlueTeam,
	FPArmSkin_RedTeam,
	FPArmSkin_Invisibility,
};

var MaterialInstance ArmSkins[EFirstPersonArmSkins.EnumCount];
var transient EFirstPersonArmSkins CurrentArmSkin;

cpptext
{
	virtual void TickSpecial( FLOAT DeltaSeconds );
}


exec function zpivot(float f)
{
	Mesh.SetTranslation(Mesh.Translation + Vect(0,0,1) * f);
}

simulated event PostBeginPlay()
{
	super.PostBeginPlay();

	CurrentArmSkin = FPArmSkin_Normal;
}

simulated event Destroyed()
{
	super.Destroyed();
}

/** Plays a Firing Animation */

simulated function PlayFiring(float Rate, name WeaponFiringMode)
{

}

simulated function StopPlayFiring()
{

}

function PossessedBy(Controller C, bool bVehicleTransition)
{
	Super.PossessedBy(C, bVehicleTransition);
	NotifyTeamChanged();
}

simulated function DisplayDebug(HUD HUD, out float out_YL, out float out_YPos)
{
	Super.DisplayDebug(Hud, out_YL, out_YPos);

	if (HUD.ShouldDisplayDebug('twist'))
	{
		Hud.Canvas.SetDrawColor(255,255,200);
		Hud.Canvas.SetPos(4,out_YPos);
		Hud.Canvas.DrawText(""$Self$" - "@Rotation@" RootYaw:"@RootYaw@" CurrentSkelAim"@CurrentSkelAim.X@CurrentSkelAim.Y);
		out_YPos += out_YL;
	}
}

simulated function FlashCountUpdated( bool bViaReplication )
{
	Super.FlashCountUpdated(bViaReplication);
}

simulated function FlashLocationUpdated( bool bViaReplication )
{
	Super.FlashLocationUpdated(bViaReplication);
}

exec function LookAtPawn()
{
	local Pawn P, ClosestPawn;
	local float DistSq, ClosestPawnDistSq;

	// find closest pawn
	ClosestPawn = None;
	foreach VisibleActors(class'Pawn', P, 1024)
	{
		if (ClosestPawn != self)
		{
			DistSq = VSizeSq(Location - P.Location);
			if ( (ClosestPawn == None) || (DistSq < ClosestPawnDistSq) )
			{
				ClosestPawn = P;
				ClosestPawnDistSq = DistSq;
			}
		}
	}
}

exec function StopLookAt()
{

}

/**
 * Event called from native code when Pawn stops crouching.
 * Called on non owned Pawns through bIsCrouched replication.
 * Network: ALL
 *
 * @param	HeightAdjust	height difference in unreal units between default collision height, and actual crouched cylinder height.
 */
simulated event EndCrouch(float HeightAdjust)
{
	super.EndCrouch(HeightAdjust);

	// offset mesh by height adjustment
	if( Mesh != None )
	{
		Mesh.SetTranslation(Mesh.Translation - Vect(0,0,1)*HeightAdjust);
	}
}


/**
 * Event called from native code when Pawn starts crouching.
 * Called on non owned Pawns through bIsCrouched replication.
 * Network: ALL
 *
 * @param	HeightAdjust	height difference in unreal units between default collision height, and actual crouched cylinder height.
 */
simulated event StartCrouch(float HeightAdjust)
{
	super.StartCrouch(HeightAdjust);

	// offset mesh by height adjustment
	if( Mesh != None )
	{
		Mesh.SetTranslation(Mesh.Translation + Vect(0,0,1)*HeightAdjust);
	}
}

simulated function SetTeamColor()
{
	Super.SetTeamColor();

	if (PlayerReplicationInfo != None || (DrivenVehicle != None && DrivenVehicle.PlayerReplicationInfo != None))
	{
		if (VerifyBodyMaterialInstance())
		{
			// skins mapped to team num (unless team num is out of bounds, as it will be for ffa games)
			SetCharSkin(GetTeamNum());
		}
	}
}

/** Apply indicated character skin.  SkinIdx should correspond to one of the ECharacterSkin enums above. */
exec simulated private function SetCharSkin(int SkinIdx)
{
	// protection
	if ( (SkinIdx > ArrayCount(BodySkins)) || (SkinIdx < 0) )
	{
		SkinIdx = Skin_Normal;
	}

	// set the body skin material
	BodyMaterialInstances[BodySkinMaterialInstanceIndex].SetParent(BodySkins[SkinIdx]);

	// do we have a head skin for this as well?
	if (HeadSkins[SkinIdx] != None)
	{
		BodyMaterialInstances[HeadSkinMaterialInstanceIndex].SetParent(HeadSkins[SkinIdx]);
	}
}

defaultproperties
{
	DrawScale=1.0

	Begin Object Class=DynamicLightEnvironmentComponent Name=MyLightEnvironment
	End Object
	Components.Add(MyLightEnvironment)
	LightEnvironment=MyLightEnvironment

	Begin Object Class=SkeletalMeshComponent Name=WPawnSkeletalMeshComponent
		AlwaysLoadOnClient=true
		AlwaysLoadOnServer=true
		bOwnerNoSee=true
		CastShadow=true
		BlockRigidBody=true
		bUpdateSkelWhenNotRendered=false
		bUpdateKinematicBonesFromAnimation=false
		bCastDynamicShadow=true
		Translation=(Z=2.0)
		RBChannel=RBCC_Pawn
		RBCollideWithChannels=(Default=true,Pawn=true,Vehicle=true)
		LightEnvironment=MyLightEnvironment
		bUseAsOccluder=FALSE
		bOverrideAttachmentOwnerVisibility=true
		bAcceptsDecals=false
		PhysicsAsset=PhysicsAsset'CH_AnimHuman.Mesh.SK_CH_BaseMale_Physics'
	End Object
	Mesh=WPawnSkeletalMeshComponent
	Components.Add(WPawnSkeletalMeshComponent)

	// default bone names
	WeaponSocket=WeaponPoint
	WeaponSocket2=DualWeaponPoint
	HeadBone=b_Head
	LeftToeBone=b_LeftToe
	LeftFootBone=b_LeftAnkle
	RightFootBone=b_RightAnkle
	TakeHitPhysicsFixedBones[0]=b_LeftAnkle
	TakeHitPhysicsFixedBones[1]=b_RightAnkle

	Gibs[0]=(BoneName=b_LeftForeArm,GibClass=class'UTGib_HumanArm',bHighDetailOnly=false)
	Gibs[1]=(BoneName=b_RightForeArm,GibClass=class'UTGib_HumanArm',bHighDetailOnly=true)
	Gibs[2]=(BoneName=b_LeftLeg,GibClass=class'UTGib_HumanArm',bHighDetailOnly=false)
	Gibs[3]=(BoneName=b_RightLeg,GibClass=class'UTGib_HumanArm',bHighDetailOnly=false)
	Gibs[4]=(BoneName=b_Spine,GibClass=class'UTGib_HumanChunk',bHighDetailOnly=false)
	Gibs[5]=(BoneName=b_Spine1,GibClass=class'UTGib_HumanChunk',bHighDetailOnly=false)
	Gibs[6]=(BoneName=b_Spine2,GibClass=class'UTGib_HumanChunk',bHighDetailOnly=false)
	Gibs[7]=(BoneName=b_RightClav,GibClass=class'UTGib_HumanBone',bHighDetailOnly=true)
	Gibs[8]=(BoneName=b_LeftClav,GibClass=class'UTGib_HumanBone',bHighDetailOnly=true)
	Gibs[9]=(BoneName=b_LeftLegUpper,GibClass=class'UTGib_HumanBone',bHighDetailOnly=true)

	HeadGib=(BoneName=b_Head,GibClass=class'UTGib_HumanHead',bHighDetailOnly=false)

	ArmSkins(FPArmSkin_Normal)=None
	ArmSkins(FPArmSkin_BlueTeam)=MaterialInstance'CH_IronGuard.Materials.M_CH_IronG_FirstPersonArm_VBlue'		// @fixme, override in utcharacter subclass files when content is ready
	ArmSkins(FPArmSkin_RedTeam)=MaterialInstance'CH_IronGuard.Materials.M_CH_IronG_FirstPersonArm_VRed'
	ArmSkins(FPArmSkin_Invisibility)=Material'Pickups.Invis.M_Invis_01'
}

