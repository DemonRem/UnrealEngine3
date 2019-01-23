//=============================================================================
// DemoRecSpectator - spectator for demo recordings to replicate ClientMessages
//=============================================================================

class DemoRecSpectator extends UTPlayerController;

var bool bTempBehindView;
var bool bFoundPlayer;
var string RemoteViewTarget;	// Used to track targets without a controller

/** if set, camera rotation is always forced to viewtarget rotation */
var config bool bLockRotationToViewTarget;

/** If set, automatically switches players every AutoSwitchPlayerInterval seconds */
var config bool bAutoSwitchPlayers;
/** Interval to use if bAutoSwitchPlayers is TRUE */
var config float AutoSwitchPlayerInterval;

simulated event PostBeginPlay()
{
	Super.PostBeginPlay();

	if ( PlayerReplicationInfo != None )
	{
		PlayerReplicationInfo.bOutOfLives = true;
	}
}

simulated event ReceivedPlayer()
{
	Super.ReceivedPlayer();

	// DemoRecSpectators don't go through the login process, so manually call ClientSetHUD()
	// so the spectator has it when playing back the demo
	if (Role == ROLE_Authority && WorldInfo.Game != None)
	{
		ClientSetHUD(WorldInfo.Game.HUDType, WorldInfo.Game.ScoreBoardType);
	}
}

function InitPlayerReplicationInfo()
{
	Super.InitPlayerReplicationInfo();
	PlayerReplicationInfo.PlayerName = "DemoRecSpectator";
	PlayerReplicationInfo.bIsSpectator = true;
	PlayerReplicationInfo.bOnlySpectator = true;
	PlayerReplicationInfo.bOutOfLives = true;
	PlayerReplicationInfo.bWaitingPlayer = false;
}

exec function ViewClass( class<actor> aClass, optional bool bQuiet, optional bool bCheat )
{
	local actor other, first;
	local bool bFound;

	first = None;

	ForEach AllActors( aClass, other )
	{
		if ( bFound || (first == None) )
		{
			first = other;
			if ( bFound )
				break;
		}
		if ( other == ViewTarget )
			bFound = true;
	}

	if ( first != None )
	{
		SetViewTarget(first);
		bBehindView = ( ViewTarget != self );

		if ( bBehindView )
			ViewTarget.BecomeViewTarget(self);
	}
	else
		SetViewTarget(self);
}

//==== Called during demo playback ============================================

exec function DemoViewNextPlayer()
{
	local Pawn P, Pick;
	local bool bFound;

	// view next player

	foreach DynamicActors(class'Pawn', P)
	{
		if (Pick == None)
		{
			Pick = P;
		}
		if (bFound)
		{
			Pick = P;
			break;
		}
		else
		{
			bFound = (RealViewTarget == P.PlayerReplicationInfo || ViewTarget == P);
		}
	}

	SetViewTarget(Pick);
}

auto state Spectating
{
	function BeginState(Name PreviousStateName)
	{
		super.BeginState(PreviousStateName);

		if( bAutoSwitchPlayers )
		{
			SetTimer( AutoSwitchPlayerInterval, true, 'DemoViewNextPlayer');
		}
	}

	exec function Fire( optional float F )
	{
		bBehindView = false;
		DemoViewNextPlayer();
	}

	exec function AltFire( optional float F )
	{
		bBehindView = !bBehindView;
	}

	event PlayerTick( float DeltaTime )
	{
		Global.PlayerTick( DeltaTime );

		// attempt to find a player to view.
		if( Role == ROLE_AutonomousProxy && (RealViewTarget==None || RealViewTarget==PlayerReplicationInfo) && !bFoundPlayer )
		{
			DemoViewNextPlayer();
			if( RealViewTarget!=None && RealViewTarget!=PlayerReplicationInfo)
				bFoundPlayer = true;
		}

/* @FIXME:
		// hack to go to 3rd person during deaths
		if( RealViewTarget!=None && RealViewTarget.Pawn==None )
		{
			if (!bBehindview)
			{
				if( !bTempBehindView )
				{
					bTempBehindView = true;
					bBehindView = true;
				}
			}
		}
		else
		if( bTempBehindView )
		{
			bBehindView = false;
			bTempBehindView = false;
		}
*/
	}
}

/* @FIXME:
event GetPlayerViewPoint(out vector CameraLocation, out rotator CameraRotation)
{
	local Rotator R;

	if( RealViewTarget != None )
	{
		R = RealViewTarget.Rotation;
	}

	Super.GetPlayerViewPoint(CameraLocation, CameraRotation );

	if( RealViewTarget != None )
	{
		if ( !bBehindView )
		{
			CameraRotation = R;
			if ( Pawn(ViewTarget) != None )
				CameraLocation.Z += Pawn(ViewTarget).BaseEyeHeight; // FIXME TEMP
		}
		RealViewTarget.SetRotation(R);
	}
}
*/

function UpdateRotation(float DeltaTime)
{
	if (bLockRotationToViewTarget)
	{
		SetRotation(ViewTarget.Rotation);
	}
	else
	{
		Super.UpdateRotation(DeltaTime);
	}
}

defaultproperties
{
	RemoteRole=ROLE_AutonomousProxy
	bDemoOwner=1
}

