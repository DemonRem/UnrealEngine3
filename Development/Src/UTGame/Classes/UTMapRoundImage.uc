/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTMapRoundImage extends UIImage;

var() Material MapRingMaterial;
var transient MaterialInstanceConstant MapRingMaterialInstance;


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
					SetupMaterialInstance(MapTex);
					if ( MapRingMaterialInstance != none )
					{
						ImageComponent.SetImage(MapRingMaterialInstance);
					}
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


function SetupMaterialInstance(Texture2D MapTex)
{
	if ( MapRingMaterialInstance == none && MapRingMaterial != none )
	{
		MapRingMaterialInstance = new(Outer) class'MaterialInstanceConstant';
		MapRingMaterialInstance.SetParent(MapRingMaterial);
		MapRingMaterialInstance.SetTextureParameterValue('LevelMap', MapTex);
	}
}

function SetScale(float NewScale)
{
	if ( MapRingMaterialInstance != none )
	{
		MapRingMaterialInstance.SetScalarParameterValue('MapScale_Parameter',NewScale);
	}
}

function SetOrigin(float X, float Y)
{
	if ( MapRingMaterialInstance != none )
	{
		MapRingMaterialInstance.SetScalarParameterValue('U_Parameter',X);
		MapRingMaterialInstance.SetScalarParameterValue('V_Parameter',Y);
	}
}

defaultproperties
{
}
