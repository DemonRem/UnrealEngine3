/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class WeapHud_RocketLauncher extends UTUIScene_WeaponHud
	native(UI);

var transient UIImage DefaultCrosshairImage;

var(Transitions)	float TransitionInTime;
var(Transitions)	float TransitionOutTime;

struct native RLSegmentData
{

	//  Holds the container that holds all of the children
	var UIContainer Container;

	var array<UIImage> ChildrenImages;

	// All of the widgets

	var float OpacityTarget;
	var float Opacity;
	var float OpacityTime;
};

var transient RLSegmentData Segments[3];

var transient int FireMode;
var transient int LoadedShotCount;

var float CrosshairTargetRotation;
var float CrosshairRotation;
var float CrosshairRotationTime;

cpptext
{
	virtual void Tick( FLOAT DeltaTime );
}

event PostInitialize()
{
	local int i,j;
	local name n;
	local UIContainer C;
	local UIImage Img;

	Super.PostInitialize();

    // Find the container for each firing mode

	DefaultCrosshairImage = UIImage(FindChild('DefaultCrosshairImage',true));

	for (i=0;i<3;i++)
	{
		n = name( "RLContainer"$i );
		C = UIContainer( FindChild(n,true) );
		if ( C != none )
		{
			Segments[i].Container = C;
			for (j=0;j<C.Children.Length;j++)
			{
				Img = UIImage(C.Children[j]);
				if ( Img != none )
				{
					Segments[i].ChildrenImages[Segments[i].ChildrenImages.Length] = Img;
				}
			}
		}
	}

}

event TickScene(float DeltaTime)
{
	local int i;

	for (i=0;i<3;i++)
	{
		if ( Segments[i].OpacityTime > 0.0f )
		{
			Segments[i].Opacity += (Segments[i].OpacityTarget - Segments[i].Opacity) * ( Deltatime / Segments[i].OpacityTime );
			Segments[i].OpacityTime -= DeltaTime;
			Segments[i].Opacity = FClamp( Segments[i].Opacity, 0.0 ,1.0 );
		}
		Segments[i].Container.Opacity = Segments[i].Opacity;
	}

	if ( DefaultCrosshairImage != none && CrosshairRotationTime > 0 )
	{
		CrosshairRotation += (CrosshairTargetRotation - CrosshairRotation) * (DeltaTime / CrosshairRotationTime);
		CrosshairRotationTime -= DeltaTime;

		CrosshairRotation = FClamp( CrosshairRotation, 0.0, 32767.0);

		DefaultCrosshairImage.Rotation.Rotation.Yaw = CrosshairRotation;
		DefaultCrosshairImage.UpdateRotationMatrix();
	}
}

simulated function Reset()
{
	local int i,j;
	for (i=0;i<3;i++)
	{
		Segments[i].Opacity = 0.0;
		Segments[i].OpacityTarget = 0.0;
		Segments[i].OpacityTime = 0.0;

		for (j=0;j<3;j++)
		{
			Segments[i].ChildrenImages[j].ImageComponent.ResetFade();
			Segments[i].ChildrenImages[j].Imagecomponent.SetOpacity(0);
		}
	}

	Segments[0].Opacity = 1.0;
	FireMode = 0;
	LoadedShotCount = 0;
}

simulated function Increment( int ShotCount, float Rate )
{
	LoadedShotCount = ShotCount;
	Segments[0].ChildrenImages[LoadedShotCount].ImageComponent.Fade(0.0,1.0, Rate);
	Segments[1].ChildrenImages[LoadedShotCount].ImageComponent.Fade(0.0,1.0, Rate);
	Segments[2].ChildrenImages[LoadedShotCount].ImageComponent.Fade(0.0,1.0, Rate);
}

simulated function ChangeFireMode( int NewFireMode )
{
	switch (NewFireMode)
	{
		case 0:
			Segments[1].OpacityTarget = 0.0;
			Segments[1].OpacityTime = TransitionOutTime;
			Segments[2].OpacityTarget = 0.0;
			Segments[2].OpacityTime = TransitionOutTime;
			break;

		case 1:
			Segments[2].OpacityTarget = 0.0;
			Segments[2].OpacityTime = TransitionOutTime;
			break;

		case 2:
			Segments[0].OpacityTarget = 0.0;
			Segments[0].OpacityTime = TransitionOutTime;
			Segments[2].OpacityTarget = 0.0;
			Segments[2].OpacityTime = TransitionOutTime;
			break;
	}

	FireMode = NewFireMode;
	Segments[FireMode].OpacityTarget = 1.0;
	Segments[FireMode].OpacityTime = TransitionInTime;

}

simulated function Rotate(int NewRotation, float NewTime)
{
	CrosshairTargetRotation = NewRotation;
	CrosshairRotationTime = NewTime;
}



defaultproperties
{
	TransitionInTime=0.25;
	TransitionOutTime=0.25;
}
