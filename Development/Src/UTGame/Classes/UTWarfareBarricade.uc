/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTWarfareBarricade extends UTOnslaughtSpecialObjective
	hidecategories(UTOnslaughtSpecialObjective)
	abstract;

var		UTWarfareChildBarricade NextBarricade;
var		int TotalDamage;

/** Used to check if someone is shooting at me */
var		int FakeDamage;

var		float LastDamageTime;

/** wake up call to fools shooting invalid target :) */
var SoundCue ShieldHitSound;

var float DestroyedTime;

simulated function RegisterChild(UTWarfareChildBarricade B)
{
	local UTWarfareChildBarricade Next;

	if ( NextBarricade == None )
	{
		NextBarricade = B;
	}
	else
	{
		Next = NextBarricade;
		while ( Next.NextBarricade != None )
		{
			Next = Next.NextBarricade;
		}
		Next.NextBarricade = B;
	}
	if ( bDisabled )
	{
		B.DisableBarricade();
	}
}

/** broadcasts the requested message index */
function BroadcastObjectiveMessage(int Switch)
{
	if ( Switch == 0 )
	{
		// don't broadcast "Under attack" announcements
		return;
	}
	super.BroadcastObjectiveMessage(Switch);
}

simulated function vector GetHUDOffset(PlayerController PC, Canvas Canvas)
{
	local float Z;

	Z = 300;
	if ( PC.ViewTarget != None )
	{
		Z += 0.02 * VSize(PC.ViewTarget.Location - Location);
	}

	return Z*vect(0,0,1);
}


/**
PostRenderFor()
Hook to allow objectives to render HUD overlays for themselves.
Called only if objective was rendered this tick.
Assumes that appropriate font has already been set
*/
simulated function PostRenderFor(PlayerController PC, Canvas Canvas, vector CameraPosition, vector CameraDir)
{
	local float TextXL, XL, YL, BeaconPulseScale; // Dist, 
	local vector ScreenLoc;
	local LinearColor TeamColor;
	local Color TextColor;
	local UTWeapon Weap;

	// must be in front of player to render HUD overlay
	if ( bHidden || ((CameraDir dot (Location - CameraPosition)) <= 0) || !ValidTargetFor(PC) )
		return;

	// only render if player can destroy it (ask weapon)
	if ( PC.Pawn != None )
	{
		if ( UTVehicle(PC.Pawn) != None )
		{
			if ( UTVehicle(PC.Pawn).Driver != None )
			{
				Weap = UTWeapon(UTVehicle(PC.Pawn).Driver.Weapon);
			}
		}
		else
		{
			Weap = UTWeapon(PC.Pawn.Weapon);
		}
	}
	if ( (Weap == None) || !Weap.bCanDestroyBarricades )
	{
		return;
	}

	screenLoc = Canvas.Project(Location + GetHUDOffset(PC,Canvas));

	// make sure not clipped out
	if (screenLoc.X < 0 ||
		screenLoc.X >= Canvas.ClipX ||
		screenLoc.Y < 0 ||
		screenLoc.Y >= Canvas.ClipY)
	{
		return;
	}

	// must have been rendered
	if ( !LocalPlayer(PC.Player).GetActorVisibility(self) )
		return;

	// fade if close to crosshair
	if (screenLoc.X > 0.45*Canvas.ClipX &&
		screenLoc.X < 0.55*Canvas.ClipX &&
		screenLoc.Y > 0.45*Canvas.ClipY &&
		screenLoc.Y < 0.55*Canvas.ClipY)
	{
		TeamColor.A = FMax(FMin(1.0, FMax(0.0,Abs(screenLoc.X - 0.5*Canvas.ClipX) - 0.025*Canvas.ClipX)/(0.025*Canvas.ClipX)), FMin(1.0, FMax(0.0, Abs(screenLoc.Y - 0.5*Canvas.ClipY)-0.025*Canvas.ClipX)/(0.025*Canvas.ClipY)));
		if ( TeamColor.A == 0.0 )
		{
			return;
		}
	}

	// make sure not behind weapon
	if ( (Weap != None) && (UTPawn(PC.Pawn) != None) && Weap.CoversScreenSpace(screenLoc, Canvas) )
	{
		return;
	}

	// pulse "key" objective
	BeaconPulseScale = (self == UTPlayerController(PC).LastAutoObjective) ? UTPlayerController(PC).BeaconPulseScale : 1.0;

	Canvas.StrLen(ObjectiveName, TextXL, YL);
	XL = FMax(TextXL * BeaconPulseScale, 0.05*Canvas.ClipX);
	YL *= BeaconPulseScale;

	class'UTHUD'.Static.GetTeamColor( 255, TeamColor, TextColor);
	class'UTHUD'.static.DrawBackground(ScreenLoc.X-0.7*XL,ScreenLoc.Y-3*YL,1.4*XL,3.5*YL, TeamColor, Canvas);

	// draw node name
	Canvas.DrawColor = TextColor;
	Canvas.SetPos(ScreenLoc.X-0.5*TextXL,ScreenLoc.Y - 1.75*YL);
	Canvas.DrawTextClipped(ObjectiveName, true);
}

simulated function RenderLinks( UTMapInfo MP, Canvas Canvas, UTPlayerController PlayerOwner, float ColorPercent, bool bFullScreenMap, bool bSelected )
{
	if ( !bDisabled && (self == PlayerOwner.LastAutoObjective) && ValidTargetFor(PlayerOwner) )
	{
		MP.SetCurrentObjective(HudLocation);
	}
	if ( WorldInfo.TimeSeconds - DestroyedTime < 4.0 )
	{
		DrawAttackIcon(Canvas, ColorPercent);
	}
}

State DestroyableObjective
{
	event TakeDamage(int Damage, Controller InstigatedBy, vector HitLocation, vector Momentum, class<DamageType> DamageType, optional TraceHitInfo HitInfo, optional Actor DamageCauser)
	{
		if ( (class<UTDamageType>(DamageType) != None) && class<UTDamageType>(DamageType).default.bDestroysBarricades ) 
		{
			TotalDamage += Damage;
			if ( TotalDamage > 1000 )
			{
				Global.TakeDamage(Damage, InstigatedBy, HitLocation, Momentum, DamageType, HitInfo, DamageCauser);

				if ( (InstigatedBy != None) && UTPlayerReplicationInfo(InstigatedBy.PlayerReplicationInfo) != None )
				{
					AddScorer( UTPlayerReplicationInfo(InstigatedBy.PlayerReplicationInfo), 1 );
				}

				DisableObjective( instigatedBy );
			}
		}
		else if ( PlayerController(InstigatedBy) != None )
		{
			if ( WorldInfo.TimeSeconds - LastDamageTime > 5.0 )
			{
				FakeDamage = Damage;
			}
			else
			{
				FakeDamage += Damage;
				if ( FakeDamage > 100 )
				{
					PlayerController(InstigatedBy).ReceiveLocalizedMessage(class'UTWarfareBarricadeMessage', 0);
					PlayerController(InstigatedBy).ClientPlaySound(ShieldHitSound);
					LastDamageTime = 0;
					return;
				}
			}
			LastDamageTime = WorldInfo.TimeSeconds;
		}
	}
}

function Reset()
{
	super.Reset();

	SetDisabled(false);
}

simulated function SetDisabled(bool bNewValue)
{
	local UTWarfareChildBarricade Next;

	bDisabled = bNewValue;

	if ( bDisabled )
	{
		// disable (collision and visibility) barricade and children
		Next = NextBarricade;
		while ( Next != None )
		{
			Next.DisableBarricade();
			Next = Next.NextBarricade;
		}
		SetCollision(false, false);
		SetHidden(true);
		CollisionComponent.SetBlockRigidBody(false);
	}
	else
	{
		// enable (collision and visibility) barricade and children
		Next = NextBarricade;
		while ( Next != None )
		{
			Next.EnableBarricade();
			Next = Next.NextBarricade;
		}
		SetCollision(true, true);
		SetHidden(false);
		CollisionComponent.SetBlockRigidBody(true);
	}
}

defaultproperties
{
	Begin Object Name=CollisionCylinder
		CollideActors=false
		BlockActors=false
	End Object

	// this will make it such that this can be statically lit
	bMovable=FALSE
	bNoDelete=true

	bProjTarget=true
	bCollideActors=true
	bBlockActors=true
	ObjectiveType=OBJ_Destroyable
	bTriggerOnceOnly=true
	bInitiallyActive=true
	bTeamControlled=false
	bShowOnMap=false
	bMustCompleteToAttackNode=false
	DestroyedTime=-1000.0
}
