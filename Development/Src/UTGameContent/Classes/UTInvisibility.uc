/**
 *
 * Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


class UTInvisibility extends UTTimedPowerup;

var Material InvisibilityMaterial;
var audiocomponent ActiveSound;
var SoundCue WarningSound;
function GivenTo(Pawn NewOwner, bool bDoNotActivate)
{
	local UTPawn P;

	Super.GivenTo(NewOwner, bDoNotActivate);

	// boost damage
	P = UTPawn(NewOwner);
	if (P != None)
	{
		// apply invisibility material
		P.SetSkin(InvisibilityMaterial);
		// reduce AI visibility
		P.bIsInvisible = true;
		P.SetOverlayHidden(true);
		if(P.IsLocallyControlled() && P.IsHumanControlled() )
		{
			if(ActiveSound != none)
			{
				ActiveSound.Play();
			}
			SetTimer(TimeRemaining - 3.0, false, 'PlayWarningSound');
		}
	}
}

/** Plays a warning the powerup is running low */
function PlayWarningSound()
{
	// reset timer if time got added
	if (TimeRemaining > 3.0)
	{
		SetTimer(TimeRemaining - 3.0, false, 'PlayWarningSound');
	}
	else
	{
		Instigator.PlaySound(WarningSound);
		SetTimer(0.75, false, 'PlayWarningSound');
	}
}


function ItemRemovedFromInvManager()
{
	local UTPawn P;

	P = UTPawn(Owner);
	if (P != None)
	{
		P.SetSkin(None);
		P.bIsInvisible = P.Default.bIsInvisible;
		P.SetOverlayHidden(false);
		SetTimer(0.0,false,'PlayWarningSound');
		if(ActiveSound != none)
		{
			ActiveSound.Stop();
		}
	}
}

simulated function RenderOverlays(HUD H)
{
/****** FIX ME JOE
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
		TopPos = 0.5 * (UTH.Canvas.ClipY - Height);

		TopPos += (10.0 * Scale);
		LeftPos += (10.0 * Scale);

		UTH.Canvas.SetDrawColor(255, 255, 255, 255);
		UTH.Canvas.SetPos(LeftPos, TopPos);
		UTH.Canvas.DrawTile(UTH.HudTexture, 50.0 * Scale, 50.0 * Scale, 0.0, 248.0, 76.0, -80.0);

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
		StaticMesh=StaticMesh'Pickups.UDamage.Mesh.S_Udamage'
		AlwaysLoadOnClient=true
		AlwaysLoadOnServer=true
		CastShadow=false
		bForceDirectLightMap=true
		bCastDynamicShadow=false
		bAcceptsLights=false
		CollideActors=false
		Scale3D=(X=0.7,Y=0.7,Z=-0.7)
		CullDistance=8000
		bUseAsOccluder=FALSE
	End Object
	DroppedPickupMesh=MeshComponentA
	PickupFactoryMesh=MeshComponentA

	bReceiveOwnerEvents=true
	bRenderOverlays=true
	PickupSound=SoundCue'A_Pickups_Powerups.PowerUps.A_Powerup_Invisibility_PickupCue'

	InvisibilityMaterial=Material'Pickups.Invis.M_Invis_01'
	HudIndex=2


	Begin Object Class=AudioComponent Name=InvisAmbient
		SoundCue=SoundCue'A_Pickups_Powerups.PowerUps.A_Powerup_Invisibility_PowerLoopCue'
	End Object
	ActiveSound=InvisAmbient
	Components.Add(InvisAmbient)

	WarningSound=SoundCue'A_Pickups_Powerups.PowerUps.A_Powerup_Invisibility_WarningCue'
	PowerupOverSound=SoundCue'A_Pickups_Powerups.PowerUps.A_Powerup_Invisibility_EndCue'
}
