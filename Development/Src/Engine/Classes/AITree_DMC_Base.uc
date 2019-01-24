/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
class AITree_DMC_Base extends Object
	native(AI);

function k2call PrintString(string InString)
{
	`log(InString);
}

function k2call float GetWorldTime( Actor InActor )
{
	return InActor.WorldInfo.TimeSeconds;
}

static function k2pure float Conv_IntToFloat(int InInt)
{
	return InInt;
}

static function k2pure string Conv_FloatToString(float InFloat)
{
	return string(InFloat);
}

static function k2pure string Conv_IntToString(int InInt)
{
	return string(InInt);
}

static function k2pure string Conv_BoolToString(bool InBool)
{
	return string(InBool);
}

static function k2pure string Conv_VectorToString(vector InVec)
{
	return string(InVec);
}

static function k2pure string Conv_RotatorToString(rotator InRot)
{
	return string(InRot);
}

static function k2pure vector MakeVector(float X, float Y, float Z)
{
	local vector RetVec;
	RetVec.X = X;
	RetVec.Y = Y;
	RetVec.Z = Z;
	return RetVec;
}

static function k2pure BreakVector(vector InVec, out float X, out float Y, out float Z)
{
	X = InVec.X;
	Y = InVec.Y;
	Z = InVec.Z;
}

static function k2pure rotator MakeRot(float Pitch, float Yaw, float Roll)
{
	local rotator RetRot;
	RetRot.Pitch = Round(65536.0 * (Pitch / 360.0));
	RetRot.Yaw = Round(65536.0 * (Yaw / 360.0));
	RetRot.Roll = Round(65536.0 * (Roll / 360.0));
	return RetRot;
}

static function k2pure BreakRot(rotator InRot, out float Pitch, out float Yaw, out float Roll)
{
	Pitch = 360.0 * (InRot.Pitch / 65536.0);
	Yaw = 360.0 * (InRot.Yaw / 65536.0);
	Roll = 360.0 * (InRot.Roll / 65536.0);
}

event k2override CalculateUtilityRating( AICommandNodeBase CommandNode );

cpptext
{
	void ClearDMC();

	void RegenDMC();

	virtual void ProcessEvent( UFunction* Function, void* Parms, void* UnusedResult=NULL );

	virtual void PostLoad();

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);
}

defaultproperties
{
} 