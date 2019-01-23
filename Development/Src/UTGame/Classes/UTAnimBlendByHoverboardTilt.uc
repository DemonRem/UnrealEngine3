/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTAnimBlendByHoverboardTilt extends AnimNodeBlendBase
	native(Animation);


cpptext
{
	// AnimNode interface
	virtual	void TickAnim( float DeltaSeconds, float TotalWeight  );

	virtual INT GetNumSliders() const { return 1; }
	virtual ESliderType GetSliderType(INT InIndex) const { return ST_2D; }
	virtual FLOAT GetSliderPosition(INT SliderIndex, INT ValueIndex);
	virtual void HandleSliderMove(INT SliderIndex, INT ValueIndex, FLOAT NewSliderValue);
	virtual FString GetSliderDrawValue(INT SliderIndex);
}

var		vector UpVector;

var()	float TiltScale;
var()	float TiltDeadZone;
var()	float TiltYScale;

defaultproperties
{
	TiltYScale=1.0

	Children(0)=(Name="Flat",Weight=1.0)
	Children(1)=(Name="Forward")
	Children(2)=(Name="Backward")
	Children(3)=(Name="Left")
	Children(4)=(Name="Right")
	bFixNumChildren=true
}