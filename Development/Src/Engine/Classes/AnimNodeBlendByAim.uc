
/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 * 11/07/2006 -- This is node is now deprecated. Use AnimNodeSequenceBlendByAim instead.
 */

class AnimNodeBlendByAim extends AnimNodeBlendBase
	native(Anim)
	deprecated
	hidecategories(Object);

/** Angle of aiming, between -1..+1 */
var()	Vector2d	Aim;
var()	Vector2d	HorizontalRange;
var()	Vector2d	VerticalRange;

/** Angle offset applied to Aim before processing */
var()	Vector2d	AngleOffset;

// Internal. Only update node, if different from previous aim.
var		transient	Vector2d	PreviousAim;

/** internal variable to initialized node */
var transient bool bInitialized;

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

// Internal, precache childs to avoid casts
var	transient AnimNodeSequence	SeqNode1;
var	transient AnimNodeSequence	SeqNode2;
var transient AnimNodeSequence	SeqNode3;
var transient AnimNodeSequence	SeqNode4;


cpptext
{
	/** returns current aim. Override this to pull information from somewhere else, like Pawn actor for example. */
	virtual FVector2D GetAim() { return Aim; }

	// AnimNode interface
	virtual	void TickAnim( FLOAT DeltaSeconds, FLOAT TotalWeight );

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

	bFixNumChildren=TRUE
	Children(0)=(Name="SeqNode1",Weight=1.0)
	Children(1)=(Name="SeqNode2")
	Children(2)=(Name="SeqNode3")
	Children(3)=(Name="SeqNode4")
}