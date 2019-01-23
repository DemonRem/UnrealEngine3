/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class RB_RadialImpulseComponent extends PrimitiveComponent
	collapsecategories
	hidecategories(Object)
	native(Physics);

var()	ERadialImpulseFalloff	ImpulseFalloff;
var()	float					ImpulseStrength;
var()	float					ImpulseRadius;
var()	bool					bVelChange;

var		DrawSphereComponent		PreviewSphere;

cpptext
{
protected:
	// UActorComponent interface.
	virtual void Attach();
public:
}

native function FireImpulse( Vector Origin );

defaultproperties
{
	// Various physics related items need to be ticked pre physics update
	TickGroup=TG_PreAsyncWork

	ImpulseFalloff=RIF_Constant
	ImpulseStrength=900.0
	ImpulseRadius=200.0
}
