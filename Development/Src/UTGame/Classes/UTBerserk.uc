/**
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTBerserk extends UTTimedPowerup;

/** ambient sound on the pawn when Berserk is active */
var SoundCue BerserkAmbientSound;
/** sound played when the Berserk is running out */
var SoundCue BerserkFadingSound;
/** overlay material applied to owner */
var MaterialInstance OverlayMaterialInstance;

/** adds or removes our bonus from the given pawn */
simulated function AdjustPawn(UTPawn P, bool bRemoveBonus)
{
	if (P != None)
	{
		if (bRemoveBonus)
		{
			P.FireRateMultiplier *= 2.0;
			if (P.Weapon != None && P.Weapon.IsTimerActive('RefireCheckTimer'))
			{
				// make currently firing weapon slow down firing rate
				P.Weapon.ClearTimer('RefireCheckTimer');
				P.Weapon.TimeWeaponFiring(P.Weapon.CurrentFireMode);
			}
			if (P.DrivenVehicle != None && P.DrivenVehicle.Weapon != None && P.DrivenVehicle.Weapon.IsTimerActive('RefireCheckTimer'))
			{
				// make currently firing vehicle weapon slow down firing rate
				P.DrivenVehicle.Weapon.ClearTimer('RefireCheckTimer');
				P.DrivenVehicle.Weapon.TimeWeaponFiring(P.DrivenVehicle.Weapon.CurrentFireMode);
			}
		}
		else
		{
			// halve firing time
			P.FireRateMultiplier *= 0.5;
		}
	}
}

function GivenTo(Pawn NewOwner, bool bDoNotActivate)
{
	local UTPawn P;

	Super.GivenTo(NewOwner, bDoNotActivate);

	P = UTPawn(NewOwner);
	if (P != None)
	{
		// apply Berserk overlay
		P.SetWeaponOverlayFlag(1);

		P.SetPawnAmbientSound(BerserkAmbientSound);

		AdjustPawn(P, false);

		// max ammo on all weapons
		if ( UTInventoryManager(P.InvManager) != None )
		{
			UTInventoryManager(P.InvManager).AllAmmo();
		}
	}
	// set timer for ending sounds
	SetTimer(TimeRemaining - 3.0, false, 'PlayBerserkFadingSound');
}

reliable client function ClientGivenTo(Pawn NewOwner, bool bDoNotActivate)
{
	Super.ClientGivenTo(NewOwner, bDoNotActivate);

	if (Role < ROLE_Authority)
	{
		AdjustPawn(UTPawn(NewOwner), false);
	}
}

function ItemRemovedFromInvManager()
{
	local UTPawn P;

	P = UTPawn(Owner);
	if ( P != None )
	{
		P.ClearWeaponOverlayFlag(1);
		P.SetPawnAmbientSound(None);
		AdjustPawn(P, true);
	}
	SetTimer(0.0, false, 'PlayBerserkFadingSound');
}

simulated event Destroyed()
{
	if (Role < ROLE_Authority)
	{
		AdjustPawn(UTPawn(Owner), true);
	}

	Super.Destroyed();
}

/** called on a timer to play Berserk ending sound */
function PlayBerserkFadingSound()
{
	// reset timer if time got added
	if (TimeRemaining > 3.0)
	{
		SetTimer(TimeRemaining - 3.0, false, 'PlayBerserkFadingSound');
	}
	else
	{
		Instigator.PlaySound(BerserkFadingSound);
		SetTimer(0.75, false, 'PlayBerserkFadingSound');
	}
}

simulated function RenderOverlays(HUD H)
{
/*** FIX ME
	local float Scale, LeftPos, TopPos, XL, YL, Width;
	local UTHUD UTH;

	UTH = UTHUD(H);
	if (UTH != None)
	{
		Scale = UTH.Canvas.ClipX / 1024.0;

		UTH.SizeInt(88.0, XL, YL, Scale);
		Width = XL + (42.0 * Scale) + (30.0 * Scale);

		LeftPos = UTH.Canvas.ClipX - Width - 5.0;
		TopPos = 0.5 * UTH.Canvas.ClipY;

		TopPos += (10.0 * Scale);
		LeftPos += (10.0 * Scale);

		UTH.Canvas.SetDrawColor(255, 100, 0, 255);
		UTH.Canvas.SetPos(LeftPos, TopPos);
		UTH.Canvas.DrawTile(UTH.HudTexture, 50.0 * Scale, 50.0 * Scale, 0.0, 168.0, 76.0, 80.0);

		LeftPos += (55.0 * Scale);
		TopPos += (10.0 * Scale);
		UTH.DrawInt(Max(0.0, TimeRemaining), LeftPos, TopPos, Scale);
	}
*/
}

defaultproperties
{
	Begin Object Class=StaticMeshComponent Name=MeshComponentA
		StaticMesh=StaticMesh'Pickups.Berserk.Mesh.S_Pickups_Berserk'
		AlwaysLoadOnClient=true
		AlwaysLoadOnServer=true
		CastShadow=false
		bForceDirectLightMap=true
		bCastDynamicShadow=false
		bAcceptsLights=true
		CollideActors=false
		Scale3D=(X=0.7,Y=0.7,Z=0.7)
		CullDistance=8000
		Materials(0)=Material'Pickups.Berserk.Materials.M_Pickups_Berserk'
		bUseAsOccluder=FALSE
	End Object
	DroppedPickupMesh=MeshComponentA
	PickupFactoryMesh=MeshComponentA

	bReceiveOwnerEvents=true
	bRenderOverlays=true
	PickupSound=SoundCue'A_Pickups_Powerups.PowerUps.A_Powerup_Berzerk_PickupCue'

	BerserkAmbientSound=SoundCue'A_Pickups_Powerups.PowerUps.A_Powerup_Berzerk_PowerLoopCue'
	BerserkFadingSound=SoundCue'A_Pickups_Powerups.PowerUps.A_Powerup_Berzerk_WarningCue'
	OverlayMaterialInstance=Material'Pickups.Berserk.M_Berserk_Overlay'
	HudIndex=1
	PowerupOverSound=SoundCue'A_Pickups_Powerups.PowerUps.A_Powerup_Berzerk_EndCue'
}
