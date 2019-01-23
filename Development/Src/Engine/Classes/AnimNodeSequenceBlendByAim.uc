
/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class AnimNodeSequenceBlendByAim extends AnimNodeSequenceBlendBase
	native(Anim)
	hidecategories(Animations);

/** Angle of aiming, between -1..+1 */
var()	Vector2d	Aim;
var()	Vector2d	HorizontalRange;
var()	Vector2d	VerticalRange;

/** Angle offset applied to Aim before processing */
var()	Vector2d	AngleOffset;

//
// Animations
//

// Left
var()	Name	AnimName_LU;
var()	Name	AnimName_LC;
var()	Name	AnimName_LD;

// Center
var()	Name	AnimName_CU;
var()	Name	AnimName_CC;
var()	Name	AnimName_CD;

// Right
var()	Name	AnimName_RU;
var()	Name	AnimName_RC;
var()	Name	AnimName_RD;

cpptext
{
	/** Override this function in a subclass, and return normalized Aim from Pawn. */
	virtual FVector2D GetAim();

	// AnimNode interface
	virtual	void TickAnim(FLOAT DeltaSeconds, FLOAT TotalWeight);
	/** make sure animations are up date */
	virtual void CheckAnimsUpToDate();

	// For slider support	
	virtual INT GetNumSliders() const { return 1; }
	virtual ESliderType GetSliderType(INT InIndex) const { return ST_2D; }
	virtual FLOAT GetSliderPosition(INT SliderIndex, INT ValueIndex);
	virtual void HandleSliderMove(INT SliderIndex, INT ValueIndex, FLOAT NewSliderValue);
	virtual FString GetSliderDrawValue(INT SliderIndex);
}

defaultproperties
{
	HorizontalRange=(X=-1,Y=+1)
	VerticalRange=(X=-1,Y=+1)

	Anims(0)=(Weight=1.0)
	Anims(1)=()
	Anims(2)=()
	Anims(3)=()
	Anims(4)=()
	Anims(5)=()
	Anims(6)=()
	Anims(7)=()
	Anims(8)=()
}