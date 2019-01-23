/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTMapInfo extends MapInfo
	dependson(UTMapMusicInfo)
	native;

/** recommended player count range - for display on the UI and the auto number of bots setting */
var() int RecommendedPlayersMin, RecommendedPlayersMax;

/** This is stored in a content package and then pointed to by the map **/
var() UTMapMusicInfo MapMusicInfo;

/** whether the path builder should build translocator/lowgrav/jumpboot paths for this level */
var() bool bBuildTranslocatorPaths;


/*********************  Map Rendering ***********************/

/** reference to the texture to use for the HUD map */
var(Minimap) Texture MapTexture;

/** Allows for additional rotation on the texture to be applied */
var(Minimap) float MapTextureYaw;

/** Location which is used as center for minimap (onslaught) */
var(Minimap) vector MapCenter;

/** Radius of map (how far it extends from center) used by minimap */
var(Minimap) float MapExtent;

/** Default yaw to apply to minimap */
var(Minimap) int MapYaw;

/** range for rotating minimap */
var(Minimap) float RotatingMiniMapRange;

/** Holds the Size of the map when designed */
var(Map) float DefaultMapSize;

/** Holds a list of objectives */
var array<UTGameObjective> Objectives;

/** If true, this map and all of the associated data is up to date. */
var transient bool bMapUpToDate;

//var transient UTGameObjective CurrentNode;
var transient Actor CurrentActor;

/** Holds the node that is currently being hovered over by the mouse */
var transient actor WatchedActor;

/** Holds a reference to the material to use for rendering hud icons */
var transient Material HUDIcons;
var transient Texture2D 	HUDIconsT;

/** Current map coordinate axes */
var vector MapRotX, MapRotY;

/** Current map yaw */
var int CurrentMapRotYaw;

/** Holds the MIC that will be used to scale/rotate the map Background */
var transient MaterialInstanceConstant MapMaterialInstance;

var Material MapMaterialReference;

var	float UseableRadius;

var transient vector ActualMapCenter;
var transient float RadarWidth, RadarRange;
var transient vector CenterPos;
var transient float MapScale;
var transient float ColorPercent;

var transient MaterialInstanceConstant GreenIconMaterialInstance;

var transient vector CurrentObjectiveHUDLocation;

cpptext
{
	virtual void CheckForErrors();
}

/******************************************************
 * Map Rendering
 ******************************************************/

// -- We can probably remove these as soon as the map is 100% in the MI
simulated function VerifyMapExtent()
{
	local int NumNodes;
	local UTGameObjective O;
	local WorldInfo WI;

	if ( MapExtent == 0 )
	{
		`log("NO VALID MINIMAP INFO IN UTONSLAUGHTMAPINFO!!!");
		WI = WorldInfo(Outer);
		MapCenter = vect(0,0,0);
		ForEach WI.AllActors(class'UTGameObjective', O)
		{
			MapCenter += O.Location;
			NumNodes++;
		}

		MapCenter = MapCenter/NumNodes;

		// Calculate the radar range and reset the nodes as not having been rendered
		ForEach WI.AllActors(class'UTGameObjective', O)
		{
			MapExtent = FMax(MapExtent, 2.75 * vsize2D(MapCenter - O.Location));
		}
	}
}

function FindObjectives()
{
	local UTGameObjective Obj;

    bMapUpToDate = true;
	ForEach WorldInfo(Outer).AllNavigationPoints(class'UTGameObjective', Obj)
	{
		Objectives[Objectives.length] = Obj;
	}

	if ( GreenIconMaterialInstance == none )
	{
		GreenIconMaterialInstance = new(Outer) class'MaterialInstanceConstant';
		GreenIconMaterialInstance.SetParent(HUDIcons);
		GreenIconMaterialInstance.SetVectorParameterValue('HUDColor', MakeLinearColor(0,1,0,1));
	}

    if ( MapMaterialInstance == none )
    {
    	MapMaterialInstance = new(Outer) class'MaterialInstanceConstant';
    	MapMaterialInstance.SetParent(MapMaterialReference);
		MapMaterialInstance.SetTextureParameterValue('LevelMap', MapTexture);
		`log("####"@MapMaterialInstance@MapTexture);
	}
}


/**
 * Draw a map on a canvas.
 *
 * @Param	Canvas			The Canvas to draw on
 * @Param	PlayerOwner	    Who is this map being shown to
 * @Param	XPos, YPos		Where on the Canvas should it be drawn
 * @Param   Width,Height	How big
 */

simulated function DrawMap(Canvas Canvas, UTPlayerController PlayerOwner, float XPos, float YPos, float Width, float Height, bool bFullDetail, float AspectRatio)
{
	local int i, j, PlayerYaw, NumObjectives;
	local vector PlayerLocation, ScreenLocation, WatchedLocation;
	local float RadarScaling, MinRadarRange, PlayerScaling;
	local Pawn P;
	local linearcolor FinalColor;
	local color ObjColor;
	local array<UTGameObjective> Sensors;
	local bool bInSensorRange;
	local UTPlayerReplicationInfo PRI;
	local UTPawn UTP;
	local UTVehicle V;
	local Pawn PawnOwner;
	local UTTeamInfo Team;
	local UTGameObjective SpawnObjective;
	local rotator MapRot,r;
	local UTTeleportBeacon Beacon;

	local WorldInfo WI;

	VerifyMapExtent();

	WI = WorldInfo(Outer);

	ColorPercent = 0.5f + Cos((WI.TimeSeconds * 4.0) * 3.14159 * 0.5f) * 0.5f;


	// Make sure we have tracked all of the nodes
	NumObjectives = Objectives.Length;
	if ( NumObjectives == 0 )
	{
		if ( !bMapUpToDate )
		{
			FindObjectives();
			NumObjectives = Objectives.Length;

			if ( NumObjectives == 0 )
			{
				return;
			}
		}
		else
		{
			return;
		}
	}

	// If we aren't rendering for anyone, exit
	if ( (PlayerOwner == None) || (PlayerOwner.PlayerReplicationInfo == None) )
	{
		 return;
	}

	PawnOwner = Pawn(PlayerOwner.ViewTarget);
	SpawnObjective = UTPlayerReplicationInfo(PlayerOwner.PlayerReplicationInfo).StartObjective;

	// Refresh all Positional Data
	CenterPos.Y = YPos + (Height * 0.5);		//0.5*Canvas.ClipY;
	CenterPos.X = XPos + (Width * 0.5); 		//0.5*Canvas.ClipX;

	// MapScale is the different between the original map and this map
	MapScale = ( Height / DefaultMapSize ) * 1.5;
	RadarWidth = Height * (UseableRadius * 2);

	// determine player position on minimap
	ScreenLocation = (PawnOwner != None) ? PawnOwner.Location : PlayerOwner.Location;
	PlayerYaw = PlayerOwner.Rotation.Yaw & 65535;

	RadarRange = MapExtent;
	MapRot.Yaw = (MapYaw * 182.04444444);

	ActualMapCenter = MapCenter;

	// Look to see if we are a rotating map and not full screen
	if ( !bFullDetail && PlayerOwner.bRotateMinimap )
	{
		// map center and rotation based on player position
		ActualMapCenter = ScreenLocation;
		MapRot.Yaw = PlayerYaw + 16384;
		MinRadarRange = RotatingMiniMapRange;

		// Setup the rotating/scaling on the map image.
		DrawMapImage(Canvas, XPos, YPos, Width, Height, PlayerYaw, (MinRadarRange/RadarRange) );
		RadarRange = MinRadarRange;
	}
	else
	{
		// Reset the map image if it's here.
		if (bFullDetail)
		{
			Canvas.SetDrawColor(255,255,255,255);
			Canvas.SetPos(XPos,YPos);
			R.Yaw = (MapTextureYaw * 182.0444444);
			Canvas.DrawRotatedTile(Texture2D(MapTexture), R, Width, Height,0,0,1024,1024);
		}
		else
		{
			DrawMapImage(Canvas, XPos, YPos, Width, Height, 0, 1.0 );
		}

		RadarWidth = Width;
	}
	RadarScaling = RadarWidth / RadarRange;

	ChangeMapRotation(MapRot);

	// draw onslaught map
	Canvas.SetPos(CenterPos.X - 0.5*RadarWidth, CenterPos.Y - 0.5*RadarWidth);

	// draw your player
	PlayerLocation = UpdateHUDLocation(ScreenLocation);
	Canvas.DrawColor = class'UTTeamInfo'.Default.BaseTeamColor[2];
	PlayerScaling = 80 * Canvas.Clipx/1024;
	DrawRotatedMaterialTile(Canvas, GreenIconMaterialInstance, PlayerLocation, PlayerYaw + 16384, PlayerScaling, PlayerScaling, 0.75, 0.75, 0.25, 0.25);

	Canvas.SetDrawColor(255,255,0,0);
	FinalColor = MakeLinearColor(1.0, 1.0, 1.0, 1.0);
	CurrentObjectiveHUDLocation = vect(0,0,0);

	// Resolve all node positions before rendering the links
	for (i = 0; i < NumObjectives; i++)
	{

		Objectives[i].bAlreadyRendered = false;
		Objectives[i].SetHUDLocation(UpdateHUDLocation(Objectives[i].Location)); // FIXMESTEVE - only at start or if change map properties
		if (  WI.GRI.OnSameTeam(Objectives[i], PlayerOwner) )
		{
			if (Objectives[i].bHasSensor)
			{
				Sensors[Sensors.Length] = Objectives[i];
			}
			if ( bFullDetail )
			{
				ObjColor = Objectives[i].ControlColor[Objectives[i].DefenderTeamIndex];
				FinalColor = ColorToLinearColor(ObjColor);
				FinalColor.A = 1.0;

				// draw associated vehicle factories with vehicles
				for ( j=0; j<Objectives[i].VehicleFactories.Length; j++ )
				{
					if ( Objectives[i].VehicleFactories[j].bHasLockedVehicle )
					{
						Objectives[i].VehicleFactories[j].SetHUDLocation(UpdateHUDLocation(Objectives[i].VehicleFactories[j].Location));
						Objectives[i].VehicleFactories[j].RenderMapIcon(self, Canvas, PlayerOwner, FinalColor);
					}
				}
			}
		}

		// @ToDo:  This would be a good spot to add tooltips/etc

		Objectives[i].SensorScale = 2.75 * RadarScaling * Objectives[i].MaxSensorRange;
	}
	if ( bFullDetail )
	{
		// Handle Selection Differently
		if ( WatchedActor != none )
		{
			WatchedLocation = GetActorHudLocation(WatchedActor);

			// Draw the Watched graphic
			Canvas.SetPos(WatchedLocation.X - 15 * MapScale, WatchedLocation.Y - 15 * MapScale * Canvas.ClipY / Canvas.ClipX);
			Canvas.SetDrawColor(255,255,255,128);
			Canvas.DrawTile(class'UTHUD'.default.AltHudTexture,31*MapScale,31*MapScale,273,494,12,13);
		}
	}

	for (i = 0; i < NumObjectives; i++)
	{
		if ( SpawnObjective != none && SpawnObjective == Objectives[i] )
		{
			// TEMP - Need a icon or background
			Canvas.SetPos(Objectives[i].HUDLocation.X - 11 * MapScale, Objectives[i].HUDLocation.Y - 11 * MapScale * AspectRatio); // * Canvas.ClipY / Canvas.ClipX);
			Canvas.SetDrawColor(64,255,64,128);
			Canvas.DrawBox(22 * MapScale,22 * MapScale);
		}

		Objectives[i].RenderLinks(self, Canvas, PlayerOwner, ColorPercent, bFullDetail, Objectives[i] == CurrentActor);
	}


	if ( bFullDetail )
	{
		For ( i=0; i<WI.GRI.PRIArray.Length; i++ )
		{
			PRI = UTPlayerReplicationInfo(WI.GRI.PRIArray[i]);
			if ( PRI != None )
			{
				PRI.bHUDRendered = false;
			}
		}

		forEach WI.DynamicActors(class'UTTeleportBeacon', Beacon)
		{
			if ( WI.GRI.OnSameTeam(PlayerOwner, Beacon) )
			{
				Beacon.SetHudLocation( UpdateHudLocation(Beacon.Location) );
				Beacon.RenderMapIcon(self, canvas, PlayerOwner, MakeLinearColor(1.0,1.0,0.0,1.0));
		    }
		}

		// Draw all vehicles in sensor range that aren't locked (locked vehicles handled by vehicle factory)
		ForEach WI.AllPawns(class'Pawn', P)
		{
			bInSensorRange = false;
			if ( P.bHidden || (P.Health <=0) )
			{
				continue;
			}
			V = UTVehicle(P);
			if ( V != None )
			{
				if ( V.bTeamLocked && UTVehicle_Leviathan(V) == none )
		        {
			        continue;
		        }
			}
			else
			{
				UTP = UTPawn(P);
				if ( UTP == None )
				{
					continue;
				}
			}
			PRI = UTPlayerReplicationInfo(P.PlayerReplicationInfo);

			if ( PRI != none )
			{
				PRI.bHUDRendered = true;
			}

			if ( WI.GRI.OnSameTeam(PlayerOwner, P) )
			{
				bInSensorRange = true;
			}
			else if ( (V == None) || !V.bStealthVehicle )
			{
			    // only draw if close to a sensor
			    for ( i=0; i<Sensors.Length; i++ )
			    {
				    if ( VSize(P.Location - Sensors[i].Location) < Sensors[i].MaxSensorRange )
				    {
					    bInSensorRange = true;
					    break;
				    }
			    }
			}

			if ( bInSensorRange )
			{

				P.SetHUDLocation(UpdateHUDLocation(P.Location));
				if ( V != None )
				{
					class'UTHud'.static.GetTeamColor(V.Team, FinalColor);
					V.RenderMapIcon(self, Canvas, PlayerOwner, FinalColor);
				}
				else
				{
					class'UTHud'.static.GetTeamColor(UTP.PlayerReplicationInfo.Team.TeamIndex, FinalColor);
					UTP.RenderMapIcon(self, Canvas, PlayerOwner, FinalColor);
				}

				if ( (PRI != None) && (PRI.Role == ROLE_Authority)  )
				{
					PRI.SetHUDLocation(UpdateHUDLocation(P.Location));
				}
			}
		}

		// on clients, get map info for non-relevant clients from PRIs
		if ( PlayerOwner.Role < ROLE_Authority )
		{
			For ( i=0; i<WI.GRI.PRIArray.Length; i++ )
			{
				PRI = UTPlayerReplicationInfo(WI.GRI.PRIArray[i]);
				if ( (PRI != None) && !PRI.bHUDRendered )
				{
					bInSensorRange = false;
					if ( PRI.HUDPawnClass == None )
					{
						continue;
					}
					else if ( WI.GRI.OnSameTeam(PlayerOwner, PRI) )
					{
						bInSensorRange = true;
					}
					else
					{
						// only draw if close to a sensor
						for ( j=0; j<Sensors.Length; j++ )
						{
							if ( VSize(PRI.HUDPawnLocation - Sensors[j].Location) < Sensors[j].MaxSensorRange )
							{
								bInSensorRange = true;
								break;
							}
						}
					}

					if ( bInSensorRange )
					{
						PRI.SetHUDLocation(UpdateHUDLocation(PRI.HUDPawnLocation));
						class'UTHud'.static.GetTeamColor(PRI.Team.TeamIndex, FinalColor);
						PRI.RenderMapIcon(self, Canvas, PlayerOwner, FinalColor);
					}
				}
			}
		}
	}

	// draw flags after vehicles/other players
	if ( UTTeamInfo(PlayerOwner.PlayerReplicationInfo.Team) != None )
	{
		// show flag if within sensor range
		for (j = 0; j < ArrayCount(WI.GRI.Teams); j++)
		{
			Team = UTTeamInfo(WI.GRI.Teams[j]);
			if (Team != None && Team.TeamFlag != None)
			{
				bInSensorRange = Team.TeamFlag.ShouldMinimapRenderFor(PlayerOwner);
				if ( !bInSensorRange )
				{
					for ( i=0; i<Sensors.Length; i++ )
					{
						if ( VSize(Team.TeamFlag.Location - Sensors[i].Location) < Sensors[i].MaxSensorRange )
						{
							bInSensorRange = true;
							break;
						}
					}
				}
				if ( bInSensorRange )
				{
					Team.TeamFlag.SetHUDLocation(UpdateHUDLocation(Team.TeamFlag.Location));
					if ( Team == PlayerOwner.PlayerReplicationInfo.Team )
					{
						Team.TeamFlag.RenderMapIcon(self, Canvas,PlayerOwner);
					}
					else if ( i < Sensors.Length )
					{
						Team.TeamFlag.RenderEnemyMapIcon(self, Canvas, PlayerOwner, Sensors[i]);
					}
				}
			}
		}
	}

	// highlight current objective
	if ( CurrentObjectiveHUDLocation != vect(0,0,0) )
	{
		Canvas.SetPos(CurrentObjectiveHUDLocation.X - 12*MapScale, CurrentObjectiveHUDLocation.Y - 12*MapScale*AspectRatio);
		Canvas.SetDrawColor(255,255,0,255 * (1.0-ColorPercent));
		Canvas.DrawTile(Texture2D'UI_HUD.HUD.UI_HUD_MinimapC',23*MapScale, 23*MapScale*AspectRatio, 0,0,71,71);
	}

	Canvas.SetDrawColor(255,255,255,255);
}

function DrawRotatedMaterialTile(Canvas Canvas, MaterialInstanceConstant M, vector MapLocation, int InYaw, float XWidth, float YWidth, float XStart, float YStart, float XLength, float YLength)
{
	local Rotator R;

	R.Yaw = InYaw - CurrentMapRotYaw;
	M.SetScalarParameterValue('TexRotation', 0);
	Canvas.SetPos(MapLocation.X - 0.5*XWidth*MapScale, MapLocation.Y - 0.5*YWidth*MapScale);
	Canvas.DrawRotatedMaterialTile(M, R, XWidth * MapScale, YWidth * MapScale, XStart, YStart, XLength, YLength);
}

/**
 * Updates the Map Location for a given world location
 */
function vector UpdateHUDLocation( Vector InLocation )
{
	local vector ScreenLocation, NewHUDLocation;
	local float sz;

    ScreenLocation = InLocation - ActualMapCenter;

	sz = VSize(ScreenLocation);
	if ( sz > 0.55*RadarRange )
	{
		// draw on circle if extends past edge
		ScreenLocation = 0.55*RadarRange * Normal(ScreenLocation);
	}

	NewHUDLocation.X = CenterPos.X + (ScreenLocation Dot MapRotX) * (RadarWidth/RadarRange);
	NewHUDLocation.Y = CenterPos.Y + (ScreenLocation Dot MapRotY) * (RadarWidth/RadarRange);
	NewHUDLocation.Z = 0;

	return NewHUDLocation;
}

function ChangeMapRotation(Rotator NewMapRotation)
{
	local vector Z;

	if ( CurrentMapRotYaw == NewMapRotation.Yaw )
	{
		return;
	}

	GetAxes(NewMapRotation, MapRotX, MapRotY, Z);
	CurrentMapRotYaw = NewMapRotation.Yaw;
}

function SetCurrentObjective(vector HudLocation)
{
	CurrentObjectiveHUDLocation = HUDLocation;
}

/**
 * Draws the map image for the mini map
 */
function DrawMapImage(Canvas Canvas, float X, float Y, float W, float H, float PlayerYaw, float BkgImgScaling)
{
	local Rotator MapTexRot;
	local vector Offset;
	local float Angle;
	local float pX, pY;

	if ( MapMaterialInstance != none && MapTexture != none )
	{
		// Set Scaling and Rotation

		MapTexRot.Yaw = (PlayerYaw * -1) + (MapTextureYaw * 182.0444444);
		Canvas.SetPos(X,Y);
		Canvas.DrawRotatedMaterialTile(MapMaterialInstance, MapTexRot, W, H);
		MapMaterialInstance.SetScalarParameterValue('MapScale_Parameter',BkgImgScaling);

		// Set Origin.
		MapTexRot.Yaw = (MapYaw * 182.04444444);

		Offset = (MapCenter - ActualMapCenter);				// Find out where in the overall map the player is

		Offset.X = (Offset.X / MapExtent); // * - 1;
		Offset.Y = (Offset.Y / MapExtent);		// Invert Y

		Angle = MapYaw * (PI / 180);

		pX = (cos(Angle) * Offset.X) - (sin(Angle) * Offset.Y);
		pY = (sin(Angle) * Offset.X) + (cos(Angle) * Offset.Y);

		MapMaterialInstance.SetScalarParameterValue('U_Parameter',pX);
		MapMaterialInstance.SetScalarParameterValue('V_Parameter',pY);
	}
}

function vector GetActorHudLocation(Actor CActor)
{
	if ( UTGameObjective(CActor) != none )
	{
		return UTGameObjective(CActor).HudLocation;
	}
	else if (UTVehicle(CActor) != none )
	{
		return UTVehicle(CActor).HudLocation;
	}
	else if ( UTDeployedActor(CActor) != none )
	{
		return UTDeployedActor(CActor).HudLocation;
	}

	return vect(0,0,0);
}

defaultproperties
{
	RecommendedPlayersMin=6
	RecommendedPlayersMax=10
	bBuildTranslocatorPaths=true
	HUDIcons=Material'UI_HUD.Icons.M_UI_HUD_Icons01'
	HUDIconsT=Texture2D'UI_HUD.Icons.T_UI_HUD_Icons01'
	UseableRadius=0.3921
	MapMaterialReference=material'UI_HUD.Materials.MapRing_Mat'
	DefaultMapSize=255
	MapRotX=(X=1,Y=0,Z=0)
	MapRotY=(X=0,Y=1,Z=0)
	CurrentMapRotYaw=0
	RotatingMiniMapRange=6000
}
