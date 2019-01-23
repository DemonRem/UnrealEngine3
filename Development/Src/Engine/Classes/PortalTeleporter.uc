/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class PortalTeleporter extends SceneCapturePortalActor
	native
	abstract
	notplaceable;

/** destination portal */
var() PortalTeleporter SisterPortal;
/** resolution for texture render target; must be a power of 2 */
var() int TextureResolutionX, TextureResolutionY;
/** marker on path network for AI */
var PortalMarker MyMarker;
/** whether or not encroachers (movers, vehicles, and such) can move this portal */
var() bool bMovablePortal;
/** if true, non-Pawn actors are always teleporter, regardless of their bCanTeleport flag */
var bool bAlwaysTeleportNonPawns;
/** whether or not this PortalTeleporter works on vehicles */
var bool bCanTeleportVehicles;
/** arrow drawn in the editor to visualize entry rotation */
var ArrowComponent EntryRotationArrow;

cpptext
{
	virtual APortalTeleporter* GetAPortalTeleporter() { return this; };
	virtual void Spawned();
	virtual void PostLoad();
	virtual void CheckForErrors();
	virtual void PostEditChange(UProperty* PropertyThatChanged);
	virtual INT AddMyMarker(AActor* S);
	virtual void TickSpecial(FLOAT DeltaTime);
	UBOOL CanTeleport(AActor* A);
}

/** teleport an actor to be relative to SisterPortal, including transforming its velocity, acceleration, and rotation
 * @param A the Actor to teleport
 * @return whether the teleport succeeded
 */
native final function bool TransformActor(Actor A);
/** transform the given movement vector to be relative to SisterPortal */
native final function vector TransformVector(vector V);
/** transform the given location to be relative to SisterPortal */
native final function vector TransformHitLocation(vector HitLocation);
/** creates and initializes a TextureRenderTarget2D with size equal to our TextureResolutionX and TextureResolutionY properties */
native final function TextureRenderTarget2D CreatePortalTexture();

/* epic ===============================================
* ::StopsProjectile()
*
* returns true if Projectiles should call ProcessTouch() when they touch this actor
*/
simulated function bool StopsProjectile(Projectile P)
{
	return !TransformActor(P);
}

defaultproperties
{
	Begin Object Name=StaticMeshComponent2
		HiddenGame=false
		CollideActors=true
		BlockActors=true
	End Object
	CollisionComponent=StaticMeshComponent2

	Begin Object Class=ArrowComponent Name=Arrow
		ArrowColor=(R=32,G=200,B=32)
		ArrowSize=0.75
		HiddenGame=true
		AlwaysLoadOnClient=False
		AlwaysLoadOnServer=False
		Rotation=(Yaw=32768)
		Scale3D=(X=-1.0)
		Translation=(X=-40.0)
	End Object
	EntryRotationArrow=Arrow
	Components.Add(Arrow)

	bCollideActors=true
	bBlockActors=true
	bWorldGeometry=true
	bMovable=false
	bAlwaysTeleportNonPawns=true

	TextureResolutionX=256
	TextureResolutionY=256
}
