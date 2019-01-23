/**
 * This control will attempt to seed this array by looking at it's children and
 * any UIImages with the WidgetTag SeatImgXX will be placed here.  They should
 * corresponds to the # of seats in the vehicle's Seat array
 *
 * Copyright 1997-2007 Epic Games, Inc. All Rights Reserved.
 */

class HudWidget_VehicleDoll extends HudWidget_PawnDoll;


var transient array<UIImage> SeatImages;

event PostInitialize()
{
	local int i;
	local int SeatIndex;
	local string WTag, SeatIndexStr;
	local UIImage Child;

	Super.PostInitialize();

	SeatImages.Length = 0;

	for ( i=0; i<Children.Length; i++ )
	{
		Child = UIImage(Children[i]);

		if ( Child != none )
		{
			WTag = String(Child.WidgetTag);
			if (  Left( WTag, 7 ) ~= "SeatImg" )
			{
				SeatIndexStr = Right( WTag, Len(WTag) - 7 );
				SeatIndex = int(SeatIndexStr);
				if (SeatIndex >= SeatImages.Length)
				{
					SeatImages.Length = SeatIndex + 1;
					SeatImages[SeatIndex]=none;
				}

				if ( SeatImages[SeatIndex] != none )
				{
					`log("Error in "$self$".PostInitialize - 2 Seat Images with the same seat tag"@SeatIndex@SeatImages[SeatIndex].WidgetTag@Child.WidgetTag);
				}
				else
				{
					SeatImages[SeatIndex] = Child;
				}
			}
		}
	}
}

/**
 * Look at the vehicle and show the seats in use
 */

event WidgetTick(FLOAT DeltaTime)
{
	local UTVehicleBase BaseVehicle;
	local UTVehicle TargetVehicle;
	local int i;
	local int Mask, VMask;

	BaseVehicle = UTVehicleBase( UTHudSceneOwner.GetPawnOwner() );
	if ( BaseVehicle != none )
	{

		if ( UTVehicle(BaseVehicle) == none )
		{
			if ( UTWeaponPawn(BaseVehicle) == none )
			{
				`log("Error in "$self$".WidgetTick - PawnOwner isn't a base vehicle");
				bRequiresTick=false;
				return;
			}
			TargetVehicle = UTWeaponPawn(BaseVehicle).MyVehicle;
		}
		else
		{
			TargetVehicle = UTVehicle(BaseVehicle);
		}
	}

	if ( TargetVehicle != none )
	{
		Mask = 1;
		VMask = TargetVehicle.SeatMask;
		for ( i=0; i< SeatImages.Length; i++ )
		{
			if ( SeatImages[i] != none )
			{
				if ( (VMask & Mask) > 0 )
				{
					SeatImages[i].ImageComponent.Fade(1.0,0.5,0.25);
				}
				else
				{
					SeatImages[i].ImageComponent.Fade(0.5,1.0,0.25);
				}
			}
			Mask = Mask << 1;
		}
	}
}


defaultproperties
{
	bRequiresTick=true
	WidgetTag=PawnDoll

}
