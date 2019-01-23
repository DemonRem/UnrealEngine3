/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTMissionPlayerController extends UTEntryPlayerController
	native;

var array<UTSPGlobe> Globes;

enum ETransitionState
{
	ETS_None,
	ETS_Fly,
	ETS_Done
};

var ETransitionState CameraTransitionState;

struct native CameraTransitionPoint
{
	/** The name of the bone we want to move towards */
	var name BoneName;

	/** The SkeletalMesh of the Globe we are moving to */
	var SkeletalMeshActor DestGlobe;

	/** How far off of the map do we sit */
	var float MapDist;
};

var vector  	CameraLocation;
var rotator  	CameraLook;

var CameraTransitionPoint CameraTransitions[2];

var float CameraTransitionTime;
var float CameraTransitionDuration;

var float Tanmod;

var int GlobeIndex;

var vector DefaultLocation;
var vector DefaultLookAt;

cpptext
{
	UBOOL Tick( FLOAT DeltaTime, enum ELevelTick TickType );
}

simulated function PostBeginPlay()
{
	local UTSPGlobe Globe;

	Super.PostBeginPlay();

	// Find all of the globes in the map

	foreach AllActors(class'UTSPGLobe',Globe)
	{
		Globes[Globes.Length] = Globe;
	}
}

simulated function bool FindGlobe(name GlobeTag, out UTSPGlobe OutGlobe)
{
	local int i;
	for (i=0;i<Globes.Length;i++)
	{
		if ( Globes[i].GlobeTag == GlobeTag )
		{
			OutGlobe = Globes[i];
			return true;
		}
	}
	OutGlobe = none;
	return false;
}



simulated function bool CalcCamera( float fDeltaTime, out vector out_CamLoc, out rotator out_CamRot, out float out_FOV )
{
	out_CamLoc = CameraLocation; // + Vect(0,0,256);
	out_CamRot = CameraLook; //Rotator(normal(Globes[GlobeIndex].Location - out_camLoc)); //
	return false;
}

function SetMissionView(name MapPoint, name MapGlobe, float MapDist)
{
	local int i;

	for (i=0;i<Globes.Length;i++)
	{
		if ( Globes[i].GlobeTag == MapGlobe )
		{
			if ( CameraTransitionState == ETS_None )
			{
				DefaultLocation = Location;
				DefaultLookAt = normal(Globes[0].Location - Location);

				CameraTransitions[0].BoneName = '';
				CameraTransitions[0].DestGlobe = Globes[GlobeIndex];
				CameraTransitions[0].MapDist = MapDist;
			}
			else
			{

				CameraTransitions[0].BoneName = CameraTransitions[1].BoneName;
				CameraTransitions[0].DestGlobe = CameraTransitions[0].DestGlobe;
				CameraTransitions[0].MapDist = CameraTransitions[0].MapDist;
			}

			// Store the New Data

			CameraTransitions[1].BoneName = MapPoint;
			CameraTransitions[1].DestGlobe = Globes[i];
			CameraTransitions[1].MapDist = MapDist;

			GlobeIndex = i;
			CameraTransitionTime = CameraTransitionDuration;

		}
	}
	CameraTransitionState = ETS_Fly;
}


defaultproperties
{
	CameraTransitionDuration=1.0;
}
