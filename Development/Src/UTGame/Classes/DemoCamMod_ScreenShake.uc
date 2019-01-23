/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class DemoCamMod_ScreenShake extends CameraModifier
	native
	config(Camera);

/**
 * DemoCamMod_ScreenShake
 * Screen Shake Camera modifier
 */

/** Shake start offset parameter */
enum EShakeParam
{
	ESP_OffsetRandom,	// Start with random offset (default)
	ESP_OffsetZero,		// Start with zero offset
};

/** Shake vector params */
struct native ShakeParams
{
	var() EShakeParam	X, Y, Z;

	var transient const byte Padding;
};

struct native ScreenShakeStruct
{
	/** Time in seconds to go until current screen shake is finished */
	var()	float	TimeToGo;
	/** Duration in seconds of current screen shake */
	var()	float	TimeDuration;

	/** view rotation amplitude */
	var()	vector	RotAmplitude;
	/** view rotation frequency */
	var()	vector	RotFrequency;
	/** view rotation Sine offset */
	var		vector	RotSinOffset;
	/** rotation parameters */
	var()	ShakeParams	RotParam;

	/** view offset amplitude */
	var()	vector	LocAmplitude;
	/** view offset frequency */
	var()	vector	LocFrequency;
	/** view offset Sine offset */
	var		vector	LocSinOffset;
	/** location parameters */
	var()	ShakeParams	LocParam;

	/** FOV amplitude */
	var()	float	FOVAmplitude;
	/** FOV frequency */
	var()	float	FOVFrequency;
	/** FOV Sine offset */
	var		float	FOVSinOffset;
	/** FOV parameters */
	var()	EShakeParam	FOVParam;

	structdefaultproperties
	{
		TimeDuration=1.f
		RotAmplitude=(X=100,Y=100,Z=200)
		RotFrequency=(X=10,Y=10,Z=25)
		LocAmplitude=(X=0,Y=3,Z=5)
		LocFrequency=(X=1,Y=10,Z=20)
		FOVAmplitude=2
		FOVFrequency=5
	}
};

/** Active ScreenShakes array */
var		Array<ScreenShakeStruct>	Shakes;

/** Always active ScreenShake for testing purposes */
var()	ScreenShakeStruct			TestShake;


/** Add a new screen shake to the list */
final function AddScreenShake( ScreenShakeStruct NewShake )
{
	// Initialize screen shake and add it to the list of active shakes
	Shakes[Shakes.Length] = InitializeShake( NewShake );
}

/** Initialize screen shake structure */
final function ScreenShakeStruct InitializeShake( ScreenShakeStruct NewShake )
{
	NewShake.TimeToGo	= NewShake.TimeDuration;

	if( !IsZero( NewShake.RotAmplitude ) )
	{
		NewShake.RotSinOffset.X		= InitializeOffset( NewShake.RotParam.X );
		NewShake.RotSinOffset.Y		= InitializeOffset( NewShake.RotParam.Y );
		NewShake.RotSinOffset.Z		= InitializeOffset( NewShake.RotParam.Z );
	}

	if( !IsZero( NewShake.LocAmplitude ) )
	{
		NewShake.LocSinOffset.X		= InitializeOffset( NewShake.LocParam.X );
		NewShake.LocSinOffset.Y		= InitializeOffset( NewShake.LocParam.Y );
		NewShake.LocSinOffset.Z		= InitializeOffset( NewShake.LocParam.Z );
	}

	if( NewShake.FOVAmplitude != 0 )
	{
		NewShake.FOVSinOffset		= InitializeOffset( NewShake.FOVParam );
	}

	return NewShake;
}

/** Initialize sin wave start offset */
final static function float InitializeOffset( EShakeParam Param )
{
	Switch( Param )
	{
		case ESP_OffsetRandom	: return FRand() * 2 * Pi;	break;
		case ESP_OffsetZero		: return 0;					break;
	}

	return 0;
}

/**
 * ComposeNewShake
 * Take Screen Shake parameters and create a new ScreenShakeStruct variable
 *
 * @param	Duration			Duration in seconds of shake
 * @param	newRotAmplitude		view rotation amplitude (pitch,yaw,roll)
 * @param	newRotFrequency		frequency of rotation shake
 * @param	newLocAmplitude		relative view offset amplitude (x,y,z)
 * @param	newLocFrequency		frequency of view offset shake
 * @param	newFOVAmplitude		fov shake amplitude
 * @param	newFOVFrequency		fov shake frequency
 */
final function ScreenShakeStruct ComposeNewShake
(
	float	Duration,
	vector	newRotAmplitude,
	vector	newRotFrequency,
	vector	newLocAmplitude,
	vector	newLocFrequency,
	float	newFOVAmplitude,
	float	newFOVFrequency
)
{
	local ScreenShakeStruct	NewShake;

	NewShake.TimeDuration	= Duration;

	NewShake.RotAmplitude	= newRotAmplitude;
	NewShake.RotFrequency	= newRotFrequency;

	NewShake.LocAmplitude	= newLocAmplitude;
	NewShake.LocFrequency	= newLocFrequency;

	NewShake.FOVAmplitude	= newFOVAmplitude;
	NewShake.FOVFrequency	= newFOVFrequency;

	return NewShake;
}

/**
 * StartNewShake
 *
 * @param	Duration			Duration in seconds of shake
 * @param	newRotAmplitude		view rotation amplitude (pitch,yaw,roll)
 * @param	newRotFrequency		frequency of rotation shake
 * @param	newLocAmplitude		relative view offset amplitude (x,y,z)
 * @param	newLocFrequency		frequency of view offset shake
 * @param	newFOVAmplitude		fov shake amplitude
 * @param	newFOVFrequency		fov shake frequency
 */
function StartNewShake
(
	float	Duration,
	vector	newRotAmplitude,
	vector	newRotFrequency,
	vector	newLocAmplitude,
	vector	newLocFrequency,
	float	newFOVAmplitude,
	float	newFOVFrequency
)
{
	local ScreenShakeStruct	NewShake;

	// check for a shake abort
	if (Duration == -1.f)
	{
		Shakes.Length = 0;
	}
	else
	{
		NewShake = ComposeNewShake
		(
			Duration,
			newRotAmplitude,
			newRotFrequency,
			newLocAmplitude,
			newLocFrequency,
			newFOVAmplitude,
			newFOVFrequency
		);

		AddScreenShake( NewShake );
	}
}

function DumpShakeInfo( ScreenShakeStruct Shake )
{
	`log("TimeToGo:"		@ Shake.TimeToGo );
	`log("TimeDuration"	@ Shake.TimeDuration );
	`log("RotAmplitude"	@ Shake.RotAmplitude );
	`log("RotFrequency"	@ Shake.RotFrequency );
	`log("RotSinOffset"	@ Shake.RotSinOffset );
	//`log("RotParam"		@ Shake.RotParam );

	`log("LocAmplitude"	@ Shake.LocAmplitude );
	`log("LocFrequency"	@ Shake.LocFrequency );
	`log("LocSinOffset"	@ Shake.LocSinOffset );
	//`log("LocParam"		@ Shake.LocParam );

	`log("FOVAmplitude"	@ Shake.FOVAmplitude );
	`log("FOVFrequency"	@ Shake.FOVFrequency );
	`log("FOVSinOffset"	@ Shake.FOVSinOffset );
	//`log("FOVParam"		@ Shake.FOVParam );
}

/** Update a ScreenShake */
native function UpdateScreenShake(float DeltaTime, out ScreenShakeStruct Shake, out TPOV OutPOV);


/** @see CameraModifer::ModifyCamera */
function bool ModifyCamera
(
		Camera	Camera,
		float	DeltaTime,
	out TPOV	OutPOV
)
{
	local int i;
	local ScreenShakeStruct CurrentScreenShake;

	// Call super where modifier may be disabled
	super.ModifyCamera(Camera, DeltaTime, OutPOV);

	// Update Screen Shakes array
	if( Shakes.Length > 0 )
	{
		for(i=0; i<Shakes.Length; i++)
		{
			// compiler won't let us pass individual elements of a dynamic array as the value for an out parm, so use a local
			//@fixme ronp - TTPRO #40059
			CurrentScreenShake = Shakes[i];
			UpdateScreenShake(DeltaTime, CurrentScreenShake, OutPOV);
			Shakes[i] = CurrentScreenShake;
		}

		// Delete any obsolete shakes
		for(i=Shakes.Length-1; i>=0; i--)
		{
			if( Shakes[i].TimeToGo <= 0 )
			{
				Shakes.Remove(i,1);
			}
		}
	}

	// Update Test Shake
	UpdateScreenShake(DeltaTime, TestShake, OutPOV);
	return FALSE;
}

defaultproperties
{
}
