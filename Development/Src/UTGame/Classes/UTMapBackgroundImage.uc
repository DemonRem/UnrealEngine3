/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTMapBackgroundImage extends UIImage;

event PostInitialize()
{
	local Texture2D MapTex;
	local WorldInfo WI;
	local LocalPlayer LP;
	local UTOnslaughtMapInfo MapInfo;

	Super.PostInitialize();

	// Look up the Background
	LP = GetPlayerOwner();
	if ( LP != None && LP.Actor != None )
	{
		WI = LP.Actor.WorldInfo;
		if ( WI != none )
		{
 			MapInfo = UTOnslaughtMapInfo( WI.GetMapInfo() );
			if ( MapInfo != none )
			{
				MapTex = Texture2D(MapInfo.MapTexture);
				if ( MapTex != none )
				{
					ImageComponent.SetImage(MapTex);
					ImageComponent.DisableCustomColor();
					return;
				}
				else
				{
					SetVisibility(false);
				}

			}
		}
	}
}

defaultproperties
{
}
