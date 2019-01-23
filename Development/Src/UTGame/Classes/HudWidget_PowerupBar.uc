/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class HudWidget_PowerupBar extends UTUI_HudWidget;

struct PowerupData
{
	var UIImage	PowerupImage;
	var UILabel	PowerupLabel;
	var bool bActive;
	var bool bWasActive;
	var int TimeRemaining;
};

/** Holds quick access to each Powerup's data.  Note, they must be in the array in the order defined by UTTimedPowerup.HudIndex */
var transient array<PowerupData> Powerups;
var transient array<string> WidgetPrefix;

event Initialized()
{
	local int i;

	Super.Initialized();

	Powerups.length = WidgetPrefix.length;
	for (i = 0; i < WidgetPrefix.Length; i++)
	{
		Powerups[i].PowerUpImage = UIImage( FindChild( name (WidgetPrefix[i]$"Image"),true ) );
		Powerups[i].PowerUpLabel = UILabel( FindChild( name (WidgetPrefix[i]$"Label"),true ) );

		Powerups[i].PowerupImage.ImageComponent.OnFadeComplete = FadeComplete;
		Powerups[i].PowerupLabel.StringRenderComponent.OnFadeComplete = FadeComplete;

		Powerups[i].PowerUpImage.SetVisibility(False);
		Powerups[i].PowerUpLabel.SetVisibility(False);
	}
}

event WidgetTick(float DeltaTime)
{
	local Pawn P;
	local UTTimedPowerup TP;
	local int i,TimeRemaining;
	local float FadeAlpha,Top, CTop;
	local int ActiveCount;

	// They all default to inactive
	for (i=0;i<Powerups.Length;i++)
	{
		Powerups[i].bWasActive = Powerups[i].bActive;
		Powerups[i].bActive = false;
	}

	// Find out which powerups are active
	P = UTHudSceneOwner.GetPawnOwner();
	if ( UTVehicleBase(P) != none )
	{
		P = UTVehicleBase(P).Driver;
	}

	if (P != none && P.InvManager != none )
	{
		foreach P.InvManager.InventoryActors(class'UTTimedPowerup', TP)
		{
			if (TP.HudIndex < Powerups.Length)
			{
			    	Powerups[TP.HudIndex].bActive = true;
			    	ActiveCount++;
			    	TimeRemaining = int(TP.TimeRemaining + 1.0f);
			    	if (TimeRemaining != Powerups[TP.HudIndex].TimeRemaining)
			    	{
					Powerups[TP.HudIndex].PowerupLabel.SetValue(string(TimeRemaining));
			        }
				Powerups[TP.HudIndex].TimeRemaining = TimeRemaining;
			}
		}
	}

	// Set the Visibility and Fades
	for (i=0;i<Powerups.Length;i++)
	{
		if ( Powerups[i].bActive )
		{
			if ( !Powerups[i].bWasActive )
			{
				Powerups[i].PowerupImage.SetVisibility(true);
				Powerups[i].PowerupLabel.SetVisibility(true);

				Powerups[i].PowerupImage.ImageComponent.Fade( 0.0, 1.0, 0.25);
				Powerups[i].PowerupLabel.StringRenderComponent.Fade(0.0, 1.0, 0.25);

				// Always start a new control directly in the middle

				Powerups[i].PowerupImage.SetPosition(0.35,UIFACE_Top,EVALPOS_PercentageOwner);
				Powerups[i].PowerupLabel.SetPosition(0.42,UIFACE_Top,EVALPOS_PercentageOwner);
			}

			if (Powerups[i].TimeRemaining == 5 && Powerups[i].PowerupImage.ImageComponent.FadeType == EFT_None)
			{
				Powerups[i].PowerupImage.ImageComponent.Pulse( 1.0, 0.4, 3);
			}
		}
		else
		{
			if (Powerups[i].bWasActive)
			{
				FadeAlpha = Powerups[i].PowerupImage.ImageComponent.FadeAlpha;
				Powerups[i].PowerupImage.ImageComponent.Fade( FadeAlpha, 0.0, 0.5 * FadeAlpha );
				Powerups[i].PowerupLabel.StringRenderComponent.Fade(1.0, 0.0, 0.5);
			}
		}
	}

	// Calculate the Starting Top Value
	Top = 0.5 - (0.25 * ActiveCount * 0.5);

	for (i = 0; i < Powerups.length; i++)
	{
		if ( Powerups[i].bActive )
		{
			CTop = Powerups[i].PowerupImage.GetPosition(UIFACE_Top, EVALPOS_PercentageOwner);
			if (CTop < Top)
			{
				CTop += (DeltaTime * 0.85);
				CTop = FMin( CTop, Top );
				Powerups[i].PowerupImage.SetPosition(CTop,UIFACE_Top,EVALPOS_PercentageOwner);
				Powerups[i].PowerupLabel.SetPosition(CTop+0.07,UIFACE_Top,EVALPOS_PercentageOwner);
			}
			else if ( CTop > Top )
			{
				CTop -= (DeltaTime * 0.85);
				CTop = FMax( CTop , Top );
				Powerups[i].PowerupImage.SetPosition(CTop,UIFACE_Top,EVALPOS_PercentageOwner);
				Powerups[i].PowerupLabel.SetPosition(CTop+0.07,UIFACE_Top,EVALPOS_PercentageOwner);
			}

			Top += 0.25;
		}
	}

	Super.WidgetTick(DeltaTime);
}

function FadeComplete(UIComp_DrawComponents Sender)
{
	// If they have faded out, then hide them
	if ( Sender.FadeAlpha <= 0.0 )
	{
		Sender.ResetFade();
		Sender.SetVisibility(false);
	}
}


defaultproperties
{
	WidgetPrefix(0)="UDamage"
	WidgetPrefix(1)="Berserk"
	WidgetPrefix(2)="Invisibility"
	WidgetPrefix(3)="Invulnerability"

	WidgetTag=PowerupBarWidget
	bRequiresTick=true
}
