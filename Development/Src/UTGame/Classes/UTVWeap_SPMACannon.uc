/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTVWeap_SPMACannon extends UTVehicleWeapon
	native(Vehicle)
	abstract
	HideDropDown;

/** Holds a link to the remote camera */
var UTProj_SPMACamera RemoteCamera;
var vector TargetVelocity;
var SoundCue BoomSound;
var bool bCanHitTargetVector;


replication
{
	if (Role==ROLE_Authority)
		RemoteCamera;
}

cpptext
{
	//virtual void TickSpecial( FLOAT DeltaSeconds );
}
/**
 * Override BeginFire and restrict firing when the vehicle isn't deployed
 *
 * @Param 	FireModeNum		0 = Fire | 1 = AltFire
 *
 */
simulated function BeginFire(byte FireModeNum)
{
	local UTVehicle_Deployable DeployableVehicle;

	DeployableVehicle = UTVehicle_Deployable(MyVehicle);
	if (DeployableVehicle != None && !DeployableVehicle.IsDeployed())
	{
		DeployableVehicle.ServerToggleDeploy();
	}
	// If we have a remote camera pending, then deal with it
	// depending on its deployed state
	else if (Role == ROLE_Authority && FireModeNum == 1 && RemoteCamera != None)
	{
		if (RemoteCamera.bDeployed)
		{
			RemoteCamera.Disconnect();		// Kill it
		}
		else
		{
			RemoteCamera.Deploy();			// Deploy it
		}
		ClearPendingFire(1);
	}
	else
	{
		Super.BeginFire(FireModeNum);
	}
}

/**
 * Called by a SPMA camera projectile, this handles the disconnection in terms of the gun and
 * the vehicle/pawn.
 *
 * @Param	Camera		The remote camera to disconnect
 */
function DisconnectCamera(UTProj_SPMACamera Camera)
{
	if (Camera == RemoteCamera)
	{
		if ( PlayerController(Instigator.Controller) != none )
		{
			PlayerController(Instigator.Controller).ClientSetViewTarget( Instigator );
			PlayerController(Instigator.Controller).SetViewTarget( Instigator );
		}

		RemoteCamera = none;
	}
}

/**
 * If we do not have a camera out, force a camera
 */

function class<Projectile> GetProjectileClass()
{
	if (RemoteCamera == none)
	{
		return WeaponProjectiles[1];
	}
	else
	{
		return Super.GetProjectileClass();
	}
}

/**
 * In the case of Alt-Fire, handle the camera
 */
simulated function Projectile ProjectileFire()
{
	local Projectile Proj;
	local UTProj_SPMACamera CamProj;
	local UTProj_SPMAShell ShellProj;
	local PlayerController PC;
	local vector TargetLoc;

	// Tweak the projectile afterwards
	// Check to see if it's a camera.  If it is, set the view from it
	Proj = Super.ProjectileFire();
	CamProj = UTProj_SPMACamera(Proj);

	if ( CamProj == None )
	{
		ShellProj = UTProj_SPMAShell(Proj);
		if ( ShellProj != none )
		{
			if ( RemoteCamera == None )
			{
				ShellProj.SetFuse( 0.3 );
			}
			else
			{
				if (bCanHitTargetVector)
				{
					Proj.Velocity = TargetVelocity;
				}
				else
				{
					Proj.Velocity = VSize(TargetVelocity) * Normal(Proj.Velocity);
				}

				// If we are viewing remotely, use the TargetVelocity
				foreach WorldInfo.AllControllers(class'PlayerController',PC)
				{
					PC.ClientPlaySound(BoomSound);
				}

				TargetLoc = RemoteCamera.GetCurrentTargetLocation(Instigator.Controller);
				ShellProj.SetFuse( VSize2D(TargetLoc - Location)/VSize2D(Proj.Velocity) );

				//FIXMEJOE: Add code for the incoming shell noise
			}
		}
	}
	else if ( Role == ROLE_Authority )
	{
		RemoteCamera = CamProj;
		RemoteCamera.InstigatorGun = self;
		if ( PlayerController(Instigator.Controller) != none )
		{
			PlayerController(Instigator.Controller).ClientSetViewTarget( RemoteCamera );
			PlayerController(Instigator.Controller).SetViewTarget( RemoteCamera );
		}
		ClearPendingFire(1);
	}

	return Proj;
}

/**
 * Calculates the velocity needed to lob a SPMA shell to a destination.  Returns true if the SPMA can
 * hit the currently target vector
 */

simulated event CalcTargetVelocity()
{
	local vector TargetLoc, StartLoc, Extent, Aim, SocketLocation;
	local rotator SocketRotation;
	local class<UTProjectile> ProjectileClass;

	ProjectileClass = class<UTProjectile>(WeaponProjectiles[0]);
	Extent = ProjectileClass.default.CollisionComponent.Bounds.BoxExtent;

	MyVehicle.GetBarrelLocationAndRotation(SeatIndex, SocketLocation, SocketRotation);
	Aim = vector(SocketRotation);

	if ( RemoteCamera != none )
	{
		// Grab the Start and End points.
		TargetLoc = RemoteCamera.GetCurrentTargetLocation(Instigator.Controller);
	}
	else
	{
		TargetLoc = GetDesiredAimPoint();
	}

	StartLoc = MyVehicle.GetPhysicalFireStartLoc(self);

	// Get the Suggested toss velocity
	bCanHitTargetVector = SuggestTossVelocity(TargetVelocity, TargetLoc, StartLoc, ProjectileClass.default.Speed, ProjectileClass.default.TossZ, 0.5, Extent, ProjectileClass.default.TerminalVelocity);

	if ( bCanHitTargetVector )
	{
		bCanHitTargetVector = ( (Aim Dot Normal(TargetVelocity)) > 0.98 );
	}
}

simulated event vector GetPhysFireStartLocation()
{
	return MyVehicle.GetPhysicalFireStartLoc(self);
}

/**
 * IsAimCorrect - Returns true if the turret associated with a given seat is aiming correctly
 *
 * @return TRUE if we can hit where the controller is aiming
 */
simulated function bool IsAimCorrect()
{
	if (WeaponProjectiles.length == 0 || WeaponProjectiles[0] == None || !ClassIsChildOf(WeaponProjectiles[0], class'UTProjectile'))
	{
		return Super.IsAimCorrect();
	}
	else
	{
		return bCanHitTargetVector;
	}
}
/*
simulated function bool ShowCrosshair()
{
	return RemoteCamera == none;
}*/
simulated function DrawWeaponCrosshair( Hud HUD )
{
	if(RemoteCamera == none)
	{
		Super.DrawWeaponCrosshair(HUD);
	}
}

/**
 * Each frame, we have to caculate the best angle for the turret */
simulated function Tick(float DeltaTime)
{
	local bool bAimCorrect;
	Super.Tick(DeltaTime);

	if (RemoteCamera != None && RemoteCamera.bDeployed && Instigator != None && Instigator.IsLocallyControlled())
	{
		bAimCorrect = IsAimCorrect();
		RemoteCamera.SetTargetIndicator( bAimCorrect );
		RemoteCamera.SimulateTrajectory(TargetVelocity);
		/*
		else if(RemoteCamera.PSC_Trail != none && RemoteCamera.PSC_Trail.bIsActive)
		{
			RemoteCamera.KillTrajectory();
		}
		*/
	}

}


function HolderDied()
{
	Super.HolderDied();

	if (RemoteCamera != None && RemoteCamera.bDeployed)
	{
		RemoteCamera.Disconnect();
	}
}
defaultproperties
{

}
