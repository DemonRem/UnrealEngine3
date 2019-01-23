/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTAnimBlendBase extends AnimNodeBlendList
	native(Animation);

/** How fast show a given child blend in. */
var(Animation) float BlendTime;

/** Also allow for Blend Overrides */
var(Animation) array<float> ChildBlendTimes;

/** slider position, for animtree editor */
var const	float	SliderPosition;

cpptext
{
	// AnimTree editor interface
	virtual INT GetNumSliders() const { return 1; }
	virtual FLOAT GetSliderPosition(INT SliderIndex, INT ValueIndex);
	virtual void HandleSliderMove(INT SliderIndex, INT ValueIndex, FLOAT NewSliderValue);
	virtual FString GetSliderDrawValue(INT SliderIndex);
}

native final function float GetBlendTime(int ChildIndex, optional bool bGetDefault);

/** If child is an AnimNodeSequence, find its duration at current play rate. */
native final function float GetAnimDuration(int ChildIndex);

defaultproperties
{
	BlendTime=0.25
}
