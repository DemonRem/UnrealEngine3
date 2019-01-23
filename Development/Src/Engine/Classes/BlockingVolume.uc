//=============================================================================
// BlockingVolume:  a bounding volume
// used to block certain classes of actors
// primary use is to provide collision for non-zero extent traces around static meshes 
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================

class BlockingVolume extends Volume
	native
	placeable;

var() bool bClampFluid;

cpptext
{
	UBOOL IgnoreBlockingBy( const AActor *Other ) const;
}

defaultproperties
{
	Begin Object Name=BrushComponent0
		CollideActors=true
		BlockActors=true
		BlockZeroExtent=false
		BlockNonZeroExtent=true
		BlockRigidBody=true
	End Object

	bWorldGeometry=true
    bCollideActors=True
    bBlockActors=True
    bClampFluid=True
}
