/**
 * Specialized version of RB_Handle for moving the walker feet
 * Does some secondary interpolation (goal location interpolates to a goal location) to make nice curves.
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTWalkerStepHandle extends RB_Handle
	native(Vehicle)
	notplaceable;

/** Number in range [0..1] representing where in the interpolation we are */
var protected transient float InterpFactor;

/** How long this interpolation should take. */
var protected transient float InterpTime;

/** vector delta for the motion of the interpolation */
var protected transient vector GoalInterpDelta;

/** Where the handle started for this interpolation */
var protected transient vector GoalInterpStartLoc;

cpptext
{
protected:
	virtual void Tick(FLOAT DeltaTime);
public:
};

/** 
 * Start the interpolation.  The rb_handle will interpolate from it's current location to a goal location, 
 * which is itself moving from the specified start location to the specified end location, linearly over the given time.
 */
simulated native function SetSmoothLocationWithGoalInterp(const out vector StartLoc, const out vector EndLoc, float MoveTime);

/**
 * Update the goal location of the rb_handles goal location without resetting the intepolation.
 */
simulated native function UpdateSmoothLocationWithGoalInterp(const out vector NewEndLoc);

/** Stops any goal interpolation that's going on. */
simulated native function StopGoalInterp();

defaultproperties
{
}

