/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTConsolePlayerController extends UTPlayerController
	config(Game)
	native;

/** Whether TargetAdhesion is enabled or not **/
var() config bool bTargetAdhesionEnabled;

var config protected bool bDebugTargetAdhesion;

/** If true, have wheeled vehicles steer towards the camera look direction. */
var config bool bLookToSteer;

/** Set when use fails to enter nearby vehicle (to prevent smart use from also putting you on hoverboard) */
var bool bJustFoundVehicle;


// @todo amitt update this to work with version 2 of the controller UI mapping system
struct native DigitalButtonActionsToCommandDatum
{
	var string Command;
};

var array<DigitalButtonActionsToCommandDatum> DigitalButtonActionsToCommandMapping;

// @todo amitt update this to work with version 2 of the controller UI mapping system
struct native ProfileSettingToUE3BindingDatum
{
	var name ProfileSettingName;
	var name UE3BindingName;

};

var array<ProfileSettingToUE3BindingDatum> ProfileSettingToUE3BindingMapping;


cpptext
{
	/**
	 * This will score both Adhesion and Friction targets.  We want the same scoring function as we
	 * don't want the two different systems fighting over targets that are close.
	 **/
	virtual FLOAT ScoreTargetAdhesionFrictionTarget( const APawn* P, FLOAT MaxDistance, const FVector& CamLoc, const FRotator& CamRot ) const;

	/** Determines whether this Pawn can be used for TargetAdhesion **/
	virtual UBOOL IsValidTargetAdhesionFrictionTarget( const APawn* P, FLOAT MaxDistance ) const;
}


/**
 * This will find the best AdhesionFriction target based on the params passed in.
 **/
native function Pawn GetTargetAdhesionFrictionTarget( FLOAT MaxDistance, const out vector CamLoc, const out Rotator CamRot ) const;

/**
 * We need to override this function so we can do our adhesion code.
 *
 * Would be nice to have have a function or something be able to be inserted between the set up
 * and processing.
 **/
function UpdateRotation( float DeltaTime )
{
	local Rotator	DeltaRot, NewRotation, ViewRotation;

	ViewRotation	= Rotation;
	DesiredRotation = ViewRotation; //save old rotation

	// Calculate Delta to be applied on ViewRotation
	DeltaRot.Yaw	= PlayerInput.aTurn;
	DeltaRot.Pitch	= PlayerInput.aLookUp;


	// NOTE:  we probably only want to ApplyTargetAdhesion when we are moving as it hides the Adhesion a lot better
	if( ( bTargetAdhesionEnabled )
		&& ( Pawn != none )
		&& ( PlayerInput.aForward != 0 )
		)
	{
		UTConsolePlayerInput(PlayerInput).ApplyTargetAdhesion( DeltaTime, UTWeapon(Pawn.Weapon), DeltaRot.Yaw, DeltaRot.Pitch );
	}


	ProcessViewRotation( DeltaTime, ViewRotation, DeltaRot );

	SetRotation( ViewRotation );

	ViewShake( DeltaTime );

	NewRotation = ViewRotation;
	NewRotation.Roll = Rotation.Roll;

	if( Pawn != None )
	{
		Pawn.FaceRotation(NewRotation, DeltaTime);
	}
}

function bool AimingHelp(bool bInstantHit)
{
	return true;
}

/**
* @returns the distance from the collision box of the target to accept aiming help (for instant hit shots)
*/
function float AimHelpModifier()
{
	return (FOVAngle < DefaultFOV - 8) ? 0.75 : 1.0;
}

simulated function bool PerformedUseAction()
{
	bJustFoundVehicle = false;
	if ( Super.PerformedUseAction() )
	{
		return true;
	}
	else if ( (Role == ROLE_Authority) && !bJustFoundVehicle )
	{
		// console smart use - bring out translocator or hoverboard if no other use possible
		ClientSmartUse();
		return true;
	}
	return false;
}

/** returns the Vehicle passed in if it can be driven
  */
function UTVehicle CheckPickedVehicle(UTVehicle V, bool bEnterVehicle)
{
	local bool bHadRealVehicle;

	bHadRealVehicle = (V != None);
	V = super.CheckPickedVehicle(V, bEnterVehicle);
	if ( bHadRealVehicle && V == None )
	{
		bJustFoundVehicle = true;
	}
	return V;
}

unreliable client function ClientSmartUse()
{
	ToggleTranslocator();
}

exec function PrevWeapon()
{
	if (Pawn == None || Vehicle(Pawn) != None)
	{
		if ( UTVehicleBase(Pawn) != None )
		{
			UTVehicleBase(Pawn).AdjacentSeat(-1, self);
		}
		else
		{
			AdjustCameraScale(true);
		}
	}
	else if (!Pawn.IsInState('FeigningDeath'))
	{
		Super.PrevWeapon();
	}
}

exec function NextWeapon()
{
	if (Pawn == None || Vehicle(Pawn) != None)
	{
		if ( UTVehicleBase(Pawn) != None )
		{
			UTVehicleBase(Pawn).AdjacentSeat(1, self);
		}
		else
		{
			AdjustCameraScale(false);
		}
	}
	else if (!Pawn.IsInState('FeigningDeath'))
	{
		Super.NextWeapon();
	}
}

function ResetPlayerMovementInput()
{
	local UTConsolePlayerInput ConsoleInput;

	Super.ResetPlayerMovementInput();

	ConsoleInput = UTConsolePlayerInput(PlayerInput);
	if (ConsoleInput != None)
	{
		ConsoleInput.ForcedDoubleClick = DCLICK_None;
	}
}

/** Gathers player settings from the client's profile. */
exec function RetrieveSettingsFromProfile()
{
	local int OutIntValue;
	local UTProfileSettings Profile;
	local UTConsolePlayerInput ConsolePlayerInput;
	local int BindingIdx;


	Profile = UTProfileSettings(OnlinePlayerData.ProfileProvider.Profile);
	ConsolePlayerInput = UTConsolePlayerInput(PlayerInput);

	// ControllerSensitivityMultiplier
	if(Profile.GetProfileSettingValueIntByName('ControllerSensitivityMultiplier', OutIntValue))
	{
		ConsolePlayerInput.SensitivityMultiplier = OutIntValue / 10.0f;
	}

	// TurningAccelerationFactor
	if(Profile.GetProfileSettingValueIntByName('TurningAccelerationFactor', OutIntValue))
	{
		// @todo: Hookup
		//ConsolePlayerInput.TurningAccelerationFactor = OutIntValue / 10.0f;
	}

	// @todo amitt update this to work with version 2 of the controller UI mapping system
	for( BindingIdx = 0; BindingIdx < ProfileSettingToUE3BindingMapping.length; ++BindingIdx )
	{
		//`log( "RetrieveSettingsFromProfile: " $ ProfileSettingToUE3BindingMapping[BindingIdx].ProfileSettingName );

		if( Profile.GetProfileSettingValueIdByName( ProfileSettingToUE3BindingMapping[BindingIdx].ProfileSettingName, OutIntValue) )
		{
			//`log( "  RetrieveSettingsFromProfile: FOUND: " $ ProfileSettingToUE3BindingMapping[BindingIdx].ProfileSettingName );
			UpdateControllerSettings_Worker( ProfileSettingToUE3BindingMapping[BindingIdx].UE3BindingName, DigitalButtonActionsToCommandMapping[OutIntValue].Command );
		}
	}
	
	// Handle stick mappings.
	if(Profile.GetProfileSettingValueId(class'UTProfileSettings'.const.UTPID_GamepadBinding_AnalogStickPreset, OutIntValue))
	{
		`Log("Setting Stick configuration: "$EAnalogStickActions(OutIntValue));

		switch(EAnalogStickActions(OutIntValue))
		{
			case ESA_Legacy:
				UpdateControllerSettings_Worker( 'XboxTypeS_LeftX', "Axis aTurn Speed=1.0 DeadZone=0.3" );
				UpdateControllerSettings_Worker( 'XboxTypeS_LeftY', "Axis aBaseY Speed=1.0 DeadZone=0.3" );
				UpdateControllerSettings_Worker( 'XboxTypeS_RightX', "Axis aStrafe Speed=1.0 DeadZone=0.3" );
				UpdateControllerSettings_Worker( 'XboxTypeS_RightY', "Axis aLookup Speed=0.75 DeadZone=0.3" );
			break;
			case ESA_SouthPaw:
				UpdateControllerSettings_Worker( 'XboxTypeS_LeftX', "Axis aTurn Speed=1.0 DeadZone=0.3" );
				UpdateControllerSettings_Worker( 'XboxTypeS_LeftY', "Axis aLookup Speed=-0.75 DeadZone=0.3" );
				UpdateControllerSettings_Worker( 'XboxTypeS_RightX', "Axis aStrafe Speed=1.0 DeadZone=0.3" );
				UpdateControllerSettings_Worker( 'XboxTypeS_RightY', "Axis aBaseY Speed=-1.0 DeadZone=0.3" );
			break;
			case ESA_LegacySouthPaw:
				UpdateControllerSettings_Worker( 'XboxTypeS_LeftX', "Axis aStrafe Speed=1.0 DeadZone=0.3" );
				UpdateControllerSettings_Worker( 'XboxTypeS_LeftY', "Axis aLookup Speed=-0.75 DeadZone=0.3" );
				UpdateControllerSettings_Worker( 'XboxTypeS_RightX', "Axis aTurn Speed=1.0 DeadZone=0.3" );
				UpdateControllerSettings_Worker( 'XboxTypeS_RightY', "Axis aBaseY Speed=-1.0 DeadZone=0.3" );
			break;
			case ESA_Normal: default:
				UpdateControllerSettings_Worker( 'XboxTypeS_LeftX', "Axis aStrafe Speed=1.0 DeadZone=0.3" );
				UpdateControllerSettings_Worker( 'XboxTypeS_LeftY', "Axis aBaseY Speed=1.0 DeadZone=0.3" );
				UpdateControllerSettings_Worker( 'XboxTypeS_RightX', "Axis aTurn Speed=1.0 DeadZone=0.3" );
				UpdateControllerSettings_Worker( 'XboxTypeS_RightY', "Axis aLookup Speed=0.75 DeadZone=0.3" );
			break;
		}
	}

	// make sure to call the super version after, since it updates the current pawn as well.
	Super.RetrieveSettingsFromProfile();
}


private function UpdateControllerSettings_Worker( name TheName, string TheCommand )
{
	local int BindIndex;

	//`log( "    UpdateControllerSettings_Worker: N" $ TheName $ " C: " $ TheCommand);

	for( BindIndex = 0; BindIndex < PlayerInput.Bindings.Length; ++BindIndex )
	{
		if( PlayerInput.Bindings[BindIndex].Name == TheName )
		{
			//`log( "     TheName: " $ TheName $ " command: " $ TheCommand );
			PlayerInput.Bindings[BindIndex].Command = TheCommand;
		}
	}
}



defaultproperties
{
	InputClass=class'UTGame.UTConsolePlayerInput'
	VehicleCheckRadiusScaling=1.5

	DigitalButtonActionsToCommandMapping(DBA_Fire)=(Command="Fire")
	DigitalButtonActionsToCommandMapping(DBA_AltFire)=(Command="AltFire")
	DigitalButtonActionsToCommandMapping(DBA_Jump)=(Command="SmartJump | Axis aUp Speed=1.0 AbsoluteAxis=100")
	DigitalButtonActionsToCommandMapping(DBA_SmartDodge)=(Command="SmartDodge")
	DigitalButtonActionsToCommandMapping(DBA_Use)=(Command="Use")
	DigitalButtonActionsToCommandMapping(DBA_ToggleMelee)=(Command="ToggleMelee | Axis aUp Speed=-1.0 AbsoluteAxis=100")
	DigitalButtonActionsToCommandMapping(DBA_ShowScores)=(Command="showscores| onrelease ReleaseShowScores")
	DigitalButtonActionsToCommandMapping(DBA_ShowMap)=(Command="showmap")
	DigitalButtonActionsToCommandMapping(DBA_FeignDeath)=(Command="FeignDeath")
	DigitalButtonActionsToCommandMapping(DBA_ToggleSpeaking)=(Command="ToggleSpeaking true | OnRelease ToggleSpeaking false")
	DigitalButtonActionsToCommandMapping(DBA_ToggleMinimap)=(Command="ToggleMinimap")
	DigitalButtonActionsToCommandMapping(DBA_WeaponPicker)=(Command="ShowQuickPick | Onrelease HideQuickPick")
	DigitalButtonActionsToCommandMapping(DBA_NextWeapon)=(Command="NextWeapon")
	DigitalButtonActionsToCommandMapping(DBA_BestWeapon)=(Command="SwitchToBestWeapon | Axis aUp Speed=-1.0 AbsoluteAxis=100")

	// @todo amitt update this to work with version 2 of the controller UI mapping system
	ProfileSettingToUE3BindingMapping(0)=(ProfileSettingName="GamepadBinding_ButtonA",UE3BindingName="Xbox_TypeS_A")
	ProfileSettingToUE3BindingMapping(1)=(ProfileSettingName="GamepadBinding_ButtonB",UE3BindingName="Xbox_TypeS_B")
	ProfileSettingToUE3BindingMapping(2)=(ProfileSettingName="GamepadBinding_ButtonX",UE3BindingName="Xbox_TypeS_X")
	ProfileSettingToUE3BindingMapping(3)=(ProfileSettingName="GamepadBinding_ButtonY",UE3BindingName="Xbox_TypeS_Y")
	ProfileSettingToUE3BindingMapping(4)=(ProfileSettingName="GamepadBinding_Back",UE3BindingName="Xbox_TypeS_Back")
	ProfileSettingToUE3BindingMapping(5)=(ProfileSettingName="GamepadBinding_Start",UE3BindingName="Xbox_TypeS_Start")
	ProfileSettingToUE3BindingMapping(6)=(ProfileSettingName="GamepadBinding_RightBumper",UE3BindingName="XboxTypeS_RightShoulder")
	ProfileSettingToUE3BindingMapping(7)=(ProfileSettingName="GamepadBinding_LeftBumper",UE3BindingName="XboxTypeS_LeftShoulder")
	ProfileSettingToUE3BindingMapping(8)=(ProfileSettingName="GamepadBinding_RightTrigger",UE3BindingName="XboxTypeS_RightTrigger")
	ProfileSettingToUE3BindingMapping(9)=(ProfileSettingName="GamepadBinding_LeftTrigger",UE3BindingName="XboxTypeS_LeftTrigger")
	ProfileSettingToUE3BindingMapping(10)=(ProfileSettingName="GamepadBinding_RightThumbstickPressed",UE3BindingName="XboxTypeS_RightThumbstick")
	ProfileSettingToUE3BindingMapping(11)=(ProfileSettingName="GamepadBinding_LeftThumbstickPressed",UE3BindingName="XboxTypeS_LeftThumbstick")
	ProfileSettingToUE3BindingMapping(12)=(ProfileSettingName="GamepadBinding_DPadUp",UE3BindingName="XboxTypeS_DPad_Up")
	ProfileSettingToUE3BindingMapping(13)=(ProfileSettingName="GamepadBinding_DPadDown",UE3BindingName="XboxTypeS_DPad_Down")
	ProfileSettingToUE3BindingMapping(14)=(ProfileSettingName="GamepadBinding_DPadLeft",UE3BindingName="XboxTypeS_DPad_Left")
	ProfileSettingToUE3BindingMapping(15)=(ProfileSettingName="GamepadBinding_DPadRight",UE3BindingName="XboxTypeS_DPad_Right")
}



