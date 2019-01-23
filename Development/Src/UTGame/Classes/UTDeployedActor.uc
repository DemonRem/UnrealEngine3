/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTDeployedActor extends Actor
	abstract
	native;

/** Who owns this */
var Controller	InstigatorController;

/** Owner team number */
var byte TeamNum;

/** The Mesh */
var MeshComponent Mesh;

/** Here on the hud to display it */
var vector HudLocation;

cpptext
{
	virtual UBOOL IgnoreBlockingBy( const AActor *Other) const;
}
replication
{
	if (Role == ROLE_Authority && bNetDirty)
		TeamNum;
}

/** We use a delegate so that different types of creators can get the OnDestroyed event */
delegate OnDeployableUsedUp(actor ChildDeployable);

simulated function Destroyed()
{
	Super.Destroyed();

	if (Role == ROLE_Authority)
	{
		// Notify the actor that controls this
		OnDeployableUsedUp(self);
	}
}

function PostBeginPlay()
{
	Super.PostBeginPlay();

	if (Instigator != None)
	{
		InstigatorController = Instigator.Controller;
		TeamNum = Instigator.GetTeamNum();
	}
}

simulated event byte GetTeamNum()
{
	return TeamNum;
}

simulated function RenderMapIcon(UTMapInfo MP, Canvas Canvas, UTPlayerController PlayerOwner, LinearColor FinalColor)
{
	Canvas.SetPos(HudLocation.X - 3, HudLocation.Y-6);
	Canvas.SetDrawColor(255,255,255,255);
	Canvas.DrawTile(class'UTHud'.Default.HudTexture,7,13,342,212,23,59);
}

simulated function HitByEMP()
{
	//Destroy();
}

/** function used to update where icon for this actor should be rendered on the HUD
 *  @param NewHUDLocation is a vector whose X and Y components are the X and Y components of this actor's icon's 2D position on the HUD
 */
simulated function SetHUDLocation(vector NewHUDLocation)
{
	HUDLocation = NewHUDLocation;
}

function Reset()
{
	Destroy();
}

defaultproperties
{
	Physics=PHYS_Falling
	bCollideActors=true
	bBlockActors=true
	bCollideWorld=true
	RemoteRole=ROLE_SimulatedProxy
	bReplicateInstigator=true
}
