/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTUDamage extends UTTimedPowerup;

/** sound played when our owner fires */
var SoundCue UDamageFireSound;
/** sound played when the UDamage is running out */
var SoundCue UDamageFadingSound;
/** last time we played that sound, so it isn't too often */
var float LastUDamageSoundTime;
/** overlay material applied to owner */
var MaterialInstance OverlayMaterialInstance;
/** ambient sound played while active*/
var SoundCue DamageAmbientSound;

function GivenTo(Pawn NewOwner, bool bDoNotActivate)
{
	local UTPawn P;

	Super.GivenTo(NewOwner, bDoNotActivate);

	// boost damage
	NewOwner.DamageScaling *= 2.0;
	P = UTPawn(NewOwner);
	if (P != None)
	{
		// apply UDamage overlay
		P.SetWeaponOverlayFlag(0);
		P.SetPawnAmbientSound(DamageAmbientSound);
	}
	// set timer for ending sounds
	SetTimer(TimeRemaining - 3.0, false, 'PlayUDamageFadingSound');
}

function ItemRemovedFromInvManager()
{
	local UTPawn P;

	Pawn(Owner).DamageScaling *= 0.5;
	P = UTPawn(Owner);
	if (P != None)
	{
		P.ClearWeaponOverlayFlag( 0 );
		P.SetPawnAmbientSound(none);
	}
	SetTimer(0.0, false, 'PlayUDamageFadingSound');
}

simulated function OwnerEvent(name EventName)
{
	if (EventName == 'FiredWeapon' && Instigator != None && WorldInfo.TimeSeconds - LastUDamageSoundTime > 0.25)
	{
		LastUDamageSoundTime = WorldInfo.TimeSeconds;
		Instigator.PlaySound(UDamageFireSound, false, true);
	}
}

/** called on a timer to play UDamage ending sound */
function PlayUDamageFadingSound()
{
	// reset timer if time got added
	if (TimeRemaining > 3.0)
	{
		SetTimer(TimeRemaining - 3.0, false, 'PlayUDamageFadingSound');
	}
	else
	{
		Instigator.PlaySound(UDamageFadingSound);
		SetTimer(0.75, false, 'PlayUDamageFadingSound');
	}
}

simulated function RenderOverlays(HUD H)
{

	// ******* FIX ME JOE */
	/*

	local float Scale, LeftPos, TopPos, XL, YL, Width, Height;
	local UTHUD UTH;

	UTH = UTHUD(H);
	if (UTH != None)
	{
		Scale = UTH.Canvas.ClipX / 1024.0;

		UTH.SizeInt(88.0, XL, YL, Scale);
		Width = XL + (42.0 * Scale) + (30.0 * Scale);
		Height = (63.0 * Scale);

		LeftPos = UTH.Canvas.ClipX - Width - 5.0;
		TopPos = 0.5 * UTH.Canvas.ClipY - Height;

		TopPos += (10.0 * Scale);
		LeftPos += (10.0 * Scale);

		UTH.Canvas.SetDrawColor(255, 255, 255, 255);
		UTH.Canvas.SetPos(LeftPos, TopPos);
		UTH.Canvas.DrawTile(UTH.HudTexture, 50.0 * Scale, 50.0 * Scale, 0.0, 168.0, 76.0, 80.0);

		UTH.Canvas.SetDrawColor(255, 0, 255, 255);
		LeftPos += (55.0 * Scale);
		TopPos += (10.0 * Scale);
		UTH.DrawInt(Max(0.0, TimeRemaining), LeftPos, TopPos, Scale);
	}
	*/
}

defaultproperties
{
	Begin Object Class=StaticMeshComponent Name=MeshComponentA
		StaticMesh=StaticMesh'Pickups.Udamage.Mesh.S_Pickups_UDamage'
		AlwaysLoadOnClient=true
		AlwaysLoadOnServer=true
		CastShadow=false
		bForceDirectLightMap=true
		bCastDynamicShadow=false
		bAcceptsLights=true
		CollideActors=false
		BlockRigidBody=false
		Scale3D=(X=0.7,Y=0.7,Z=0.7)
		CullDistance=8000
		bUseAsOccluder=FALSE
	End Object
	DroppedPickupMesh=MeshComponentA
	PickupFactoryMesh=MeshComponentA

	bReceiveOwnerEvents=true
	bRenderOverlays=true
	PickupSound=SoundCue'A_Pickups_Powerups.PowerUps.A_Powerup_UDamage_PickupCue'
	UDamageFireSound=SoundCue'A_Pickups_Powerups.PowerUps.A_Powerup_UDamage_FireCue'
	UDamageFadingSound=SoundCue'A_Pickups_Powerups.PowerUps.A_Powerup_UDamage_WarningCue'
	PowerupOverSound=SoundCue'A_Pickups_Powerups.PowerUps.A_Powerup_UDamage_EndCue'
	OverlayMaterialInstance=Material'Pickups.UDamage.M_UDamage_Overlay'
	DamageAmbientSound=SoundCue'A_Pickups_Powerups.PowerUps.A_Powerup_UDamage_PowerLoopCue'
	HudIndex=0
}
