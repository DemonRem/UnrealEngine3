/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTWeaponAttachment extends Actor
	abstract
	dependson(UTPhysicalMaterialProperty);

/*********************************************************************************************
 Animations and Sounds
********************************************************************************************* */

var class<Actor> SplashEffect;

/*********************************************************************************************
 Weapon Components
********************************************************************************************* */

/** Weapon SkelMesh */
var SkeletalMeshComponent Mesh;

/*********************************************************************************************
 Overlays
********************************************************************************************* */

/** mesh for overlay - Each weapon will need to add it's own overlay mesh in it's default props */
var protected SkeletalMeshComponent OverlayMesh;

/*********************************************************************************************
 Muzzle Flash
********************************************************************************************* */

/** Holds the name of the socket to attach a muzzle flash too */
var name					MuzzleFlashSocket;

/** Muzzle flash PSC and Templates*/

var ParticleSystemComponent	MuzzleFlashPSC;
var ParticleSystem			MuzzleFlashPSCTemplate, MuzzleFlashAltPSCTemplate;
var color					MuzzleFlashColor;
var bool					bMuzzleFlashPSCLoops;

/** dynamic light */
var class<UTExplosionLight> MuzzleFlashLightClass;
var	UTExplosionLight		MuzzleFlashLight;

/** How long the Muzzle Flash should be there */
var float					MuzzleFlashDuration;

/** TEMP for guns with no muzzleflash socket */
var SkeletalMeshComponent OwnerMesh;
var Name AttachmentSocket;

/*********************************************************************************************
 Effects
********************************************************************************************* */

/** impact effects by material type */
var array<MaterialImpactEffect> ImpactEffects, AltImpactEffects;
/** default impact effect to use if a material specific one isn't found */
var MaterialImpactEffect DefaultImpactEffect, DefaultAltImpactEffect;
/** sound that is played when the bullets go whizzing past your head */
var SoundCue BulletWhip;

var float MaxImpactEffectDistance;
var float MaxFireEffectDistance;

var bool bAlignToSurfaceNormal;

var class<UTWeapon> WeaponClass;

var bool bSuppressSounds;

/*********************************************************************************************
 Anim
********************************************************************************************* */

var enum EWeapAnimType
{
	EWAT_Default,
	EWAT_Pistol,
	EWAT_DualPistols,
	EWAT_ShoulderRocket,
	EWAT_Stinger
} WeapAnimType;

simulated function CreateOverlayMesh()
{
	if (WorldInfo.NetMode != NM_DedicatedServer)
	{
		OverlayMesh = new(self) Mesh.Class;
		if (OverlayMesh != None)
		{
			OverlayMesh.SetScale(1.1);
			OverlayMesh.SetSkeletalMesh(Mesh.SkeletalMesh);
			OverlayMesh.AnimSets = Mesh.AnimSets;
			OverlayMesh.SetAnimTreeTemplate(Mesh.AnimTreeTemplate);
			OverlayMesh.SetParentAnimComponent(Mesh);
			OverlayMesh.bUpdateSkelWhenNotRendered = false;
			OverlayMesh.bIgnoreControllersWhenNotRendered = true;
			OverlayMesh.bOverrideAttachmentOwnerVisibility = true;

			if (UTSkeletalMeshComponent(OverlayMesh) != none)
			{
				UTSkeletalMeshComponent(OverlayMesh).SetFOV(UTSkeletalMeshComponent(Mesh).FOV);
			}
		}
	}
}


function SetSkin(Material NewMaterial)
{
	local int i,Cnt;

	if ( NewMaterial == None )	// Clear the materials
	{
		if ( default.Mesh.Materials.Length > 0 )
		{
			Cnt = Default.Mesh.Materials.Length;
			for (i=0;i<Cnt;i++)
			{
				Mesh.SetMaterial( i, Default.Mesh.GetMaterial(i) );
			}
		}
		else if (Mesh.Materials.Length > 0)
		{
			Cnt = Mesh.Materials.Length;
			for ( i=0; i < Cnt; i++ )
			{
				Mesh.SetMaterial(i,none);
			}
		}
	}
	else
	{
		if ( default.Mesh.Materials.Length > 0 || mesh.GetNumElements() > 0 )
		{
			Cnt = default.Mesh.Materials.Length > 0 ? default.Mesh.Materials.Length : mesh.GetNumElements();
			for ( i=0; i < Cnt; i++ )
			{
				Mesh.SetMaterial(i,NewMaterial);
			}
		}
	}
}

/**
 * Allows a child to setup custom parameters on the muzzle flash
 */
simulated function SetMuzzleFlashParams(ParticleSystemComponent PSC)
{
	PSC.SetColorParameter('MuzzleFlashColor', MuzzleFlashColor);
}

/**
 * Called on a client, this function Attaches the WeaponAttachment
 * to the Mesh.
 */
simulated function AttachTo(UTPawn OwnerPawn)
{
	SetWeaponOverlayFlags( UTPawn(Instigator) );

	if (OwnerPawn.Mesh != None)
	{
		// Attach Weapon mesh to player skelmesh
		if ( Mesh != None )
		{
			OwnerMesh = OwnerPawn.Mesh;
			AttachmentSocket = OwnerPawn.WeaponSocket;

			// Weapon Mesh Shadow
			Mesh.SetShadowParent(OwnerPawn.Mesh);
			Mesh.SetLightEnvironment(OwnerPawn.LightEnvironment);

			if (OwnerPawn.ReplicatedBodyMaterial != None)
			{
				SetSkin(OwnerPawn.ReplicatedBodyMaterial);
			}

			OwnerPawn.Mesh.AttachComponentToSocket(Mesh, OwnerPawn.WeaponSocket);
		}

		if (OverlayMesh != none)
		{
			OwnerPawn.Mesh.AttachComponentToSocket(OverlayMesh, OwnerPawn.WeaponSocket);
		}
	}

	if (MuzzleFlashSocket != '')
	{
		if (MuzzleFlashPSCTemplate != None || MuzzleFlashAltPSCTemplate != None)
		{
			MuzzleFlashPSC = new(self) class'UTParticleSystemComponent';
			MuzzleFlashPSC.bAutoActivate = false;
			Mesh.AttachComponentToSocket(MuzzleFlashPSC, MuzzleFlashSocket);
		}
	}

	OwnerPawn.SetWeapAnimType(WeapAnimType);

	GotoState('CurrentlyAttached');
}


/** sets whether the weapon is being dual wielded */
simulated function SetDualWielding(bool bNowDual);
/** sets whether the weapon is being put away */
simulated function SetPuttingDownWeapon(bool bNowPuttingDown);

/**
 * Detach weapon from skeletal mesh
 */
simulated function DetachFrom( SkeletalMeshComponent MeshCpnt )
{
	SetSkin(None);

	// Weapon Mesh Shadow
	if ( Mesh != None )
	{
		Mesh.SetShadowParent(None);
		Mesh.SetLightEnvironment(None);
		// muzzle flash effects
		if (MuzzleFlashPSC != None)
		{
			Mesh.DetachComponent(MuzzleFlashPSC);
		}
		if (MuzzleFlashLight != None)
		{
			Mesh.DetachComponent(MuzzleFlashLight);
		}
	}
	if ( MeshCpnt != None )
	{
		// detach weapon mesh from player skelmesh
		if ( Mesh != None )
		{
			MeshCpnt.DetachComponent( mesh );
		}

		if ( OverlayMesh != none )
		{
			MeshCpnt.DetachComponent( OverlayMesh );
		}
	}

	GotoState('');
}

/**
 * Turns the MuzzleFlashlight off
 */
simulated function MuzzleFlashTimer()
{
	if ( MuzzleFlashLight != None )
	{
		MuzzleFlashLight.SetEnabled(FALSE);
	}

	if (MuzzleFlashPSC != none && (!bMuzzleFlashPSCLoops) )
	{
		MuzzleFlashPSC.DeactivateSystem();
	}
}

/**
 * Causes the muzzle flash to turn on and setup a time to
 * turn it back off again.
 */
simulated function CauseMuzzleFlash()
{
	// only enable muzzleflash light if performance is high enough
	if ( !WorldInfo.bDropDetail )
	{
		if ( MuzzleFlashLight == None )
		{
			if ( MuzzleFlashLightClass != None )
			{
				MuzzleFlashLight = new(Outer) MuzzleFlashLightClass;
				if (Mesh != None && Mesh.GetSocketByName(MuzzleFlashSocket) != None)
				{
					Mesh.AttachComponentToSocket(MuzzleFlashLight, MuzzleFlashSocket);
				}
				else if ( OwnerMesh != None )
				{
					OwnerMesh.AttachComponentToSocket(MuzzleFlashLight, AttachmentSocket);
				}
			}
		}
		else
		{
			MuzzleFlashLight.ResetLight();
		}
	}

	if (MuzzleFlashPSC != none)
	{
		if ( !bMuzzleFlashPSCLoops || !MuzzleFlashPSC.bIsActive)
		{
			if (Instigator != None && Instigator.FiringMode == 1 && MuzzleFlashAltPSCTemplate != None)
			{
				MuzzleFlashPSC.SetTemplate(MuzzleFlashAltPSCTemplate);
			}
			else
			{
				MuzzleFlashPSC.SetTemplate(MuzzleFlashPSCTemplate);
			}

			SetMuzzleFlashParams(MuzzleFlashPSC);
			MuzzleFlashPSC.ActivateSystem();
		}
	}

	// Set when to turn it off.
	SetTimer(MuzzleFlashDuration,false,'MuzzleFlashTimer');

}

/**
 * Stops the muzzle flash
 */
simulated function StopMuzzleFlash()
{
	ClearTimer('MuzzleFlashTimer');
	MuzzleFlashTimer();

	if ( MuzzleFlashPSC != none )
	{
		MuzzleFlashPSC.DeactivateSystem();
	}
}

/**
 * The Weapon attachment, though hidden, is also responsible for controlling
 * the first person effects for a weapon.
 */

simulated function FirstPersonFireEffects(Weapon PawnWeapon, vector HitLocation)	// Should be subclassed
{
	if (PawnWeapon!=None)
	{
		// Tell the weapon to cause the muzzle flash, etc.
		PawnWeapon.PlayFireEffects( Pawn(Owner).FiringMode, HitLocation );
	}
}

simulated function StopFirstPersonFireEffects(Weapon PawnWeapon)	// Should be subclassed
{
	if (PawnWeapon!=None)
	{
		// Tell the weapon to cause the muzzle flash, etc.
		PawnWeapon.StopFireEffects( Pawn(Owner).FiringMode );
	}
}


simulated function bool EffectIsRelevant(vector SpawnLocation, bool bForceDedicated, optional float CullDistance )
{
	if ( Instigator != None )
	{
		if ( SpawnLocation == Location )
			SpawnLocation = Instigator.Location;
		return Instigator.EffectIsRelevant(SpawnLocation, bForceDedicated, CullDistance);
	}
	return Super.EffectIsRelevant(SpawnLocation, bForceDedicated, CullDistance);
}

/**
 * Spawn all of the effects that will be seen in behindview/remote clients.  This
 * function is called from the pawn, and should only be called when on a remote client or
 * if the local client is in a 3rd person mode.
*/
simulated function ThirdPersonFireEffects(vector HitLocation)
{
	local UTPawn P;

	if ( EffectIsRelevant(Location,false,MaxFireEffectDistance) )
	{
		// Light it up
		CauseMuzzleFlash();
	}

	// Have pawn play firing anim
	P = UTPawn(Instigator);
	if (P != None && P.GunRecoilNode != None)
	{
		// Use recoil node to move arms when we fire
		P.GunRecoilNode.bPlayRecoil = true;
	}
}

simulated event StopThirdPersonFireEffects()
{
	StopMuzzleFlash();
}

/** returns the impact sound that should be used for hits on the given physical material */
simulated function MaterialImpactEffect GetImpactEffect(PhysicalMaterial HitMaterial)
{
	local int i;
	local UTPhysicalMaterialProperty PhysicalProperty;

	if (HitMaterial != None)
	{
		PhysicalProperty = UTPhysicalMaterialProperty(HitMaterial.GetPhysicalMaterialProperty(class'UTPhysicalMaterialProperty'));
	}
	if (UTPawn(Owner).FiringMode > 0)
	{
		if (PhysicalProperty != None && PhysicalProperty.MaterialType != 'None')
		{
			i = AltImpactEffects.Find('MaterialType', PhysicalProperty.MaterialType);
			if (i != -1)
			{
				return AltImpactEffects[i];
			}
		}
		return DefaultAltImpactEffect;
	}
	else
	{
		if (PhysicalProperty != None && PhysicalProperty.MaterialType != 'None')
		{
			i = ImpactEffects.Find('MaterialType', PhysicalProperty.MaterialType);
			if (i != -1)
			{
				return ImpactEffects[i];
			}
		}
		return DefaultImpactEffect;
	}
}

simulated function bool AllowImpactEffects(Actor HitActor, vector HitLocation, vector HitNormal)
{
	return (PortalTeleporter(HitActor) == None);
}

simulated function PlayHitPawnEffects(vector HitLocation, pawn HitPawn);

/**
 * Spawn any effects that occur at the impact point.  It's called from the pawn.
 */

simulated function PlayImpactEffects(vector HitLocation)
{
	local vector NewHitLoc, HitNormal, FireDir;
	local Actor HitActor;
	local TraceHitInfo HitInfo;
	local MaterialImpactEffect ImpactEffect;
	local Vehicle V;
	local PlayerController PC;
	local DecalLifetimeData LifetimeData;

	if (UTPawn(Owner) != None)
	{
		HitNormal = Normal(Owner.Location - HitLocation);
		FireDir = -1 * HitNormal;
		HitActor = Trace(NewHitLoc, HitNormal, (HitLocation - (HitNormal * 32)), HitLocation + (HitNormal * 32), true,, HitInfo, TRACEFLAG_Bullet);
		if (Pawn(HitActor) != None)
		{
			CheckHitInfo(HitInfo, Pawn(HitActor).Mesh, -HitNormal, NewHitLoc);
			PlayHitPawnEffects(HitLocation, Pawn(HitActor));
		}
		// figure out the impact sound to use
		ImpactEffect = GetImpactEffect(HitInfo.PhysMaterial);
		if (ImpactEffect.Sound != None && !bSuppressSounds)
		{
			// if hit a vehicle controlled by the local player, always play it full volume
			V = Vehicle(HitActor);
			if (V != None && V.IsLocallyControlled() && V.IsHumanControlled())
			{
				PlayerController(V.Controller).ClientPlaySound(ImpactEffect.Sound);
			}
			else
			{
				if ( BulletWhip != None )
				{
					ForEach LocalPlayerControllers(class'PlayerController', PC)
					{
						if ( (UTPlayerController(PC) != None) && !WorldInfo.GRI.OnSameTeam(Owner,PC) )
							UTPlayerController(PC).CheckBulletWhip(BulletWhip, Owner.Location, FireDir, HitLocation);
					}
				}
				PlaySound(ImpactEffect.Sound, true,,, HitLocation);
			}

			if ( UTVehicle(V) != none && Role < ROLE_Authority && !WorldInfo.GRI.OnSameTeam(Owner,V) )
			{
				UTVehicle(V).ApplyMorphDamage(HitLocation, WeaponClass.Default.InstantHitDamage[UTPawn(Owner).FiringMode], WeaponClass.Default.InstantHitMomentum[UTPawn(Owner).FiringMode]*FireDir);
			}
		}

		// play effects
		if (EffectIsRelevant(HitLocation, false, MaxImpactEffectDistance))
		{
			// Pawns handle their own hit effects
			if ( HitActor != None &&
				 (Pawn(HitActor) == None || Vehicle(HitActor) != None) &&
				 AllowImpactEffects(HitActor, HitLocation, HitNormal) )
			{
				if (ImpactEffect.DecalMaterial != None)
				{
					// FIXME: decal lifespan based on detail level
					LifetimeData = new(HitActor.Outer) class'DecalLifetimeDataAge';
					HitActor.AddDecal( ImpactEffect.DecalMaterial, HitLocation, rotator(-HitNormal), FRand() * 360,
								ImpactEffect.DecalWidth, ImpactEffect.DecalHeight, 10.0, false, HitInfo.HitComponent, LifetimeData, true, false, HitInfo.BoneName, HitInfo.Item, HitInfo.LevelIndex );
				}

				if (!bAlignToSurfaceNormal)
				{
					HitNormal = normal(FireDir - ( 2 *  HitNormal * (FireDir dot HitNormal) ) ) ;
//					HitNormal = Normal(HitLocation - Owner.Location);
				}

				if (ImpactEffect.ParticleTemplate != None)
				{
					WorldInfo.MyEmitterPool.SpawnEmitter(ImpactEffect.ParticleTemplate, HitLocation, rotator(HitNormal), HitActor);
				}
			}
		}
	}
}

/** When an attachment is attached to a pawn, it enters the CurrentlyAttached state.  */
state CurrentlyAttached
{
}

/* FIXMESTEVE
simulated function CheckForSplash()
{
	local Actor HitActor;
	local vector HitNormal, HitLocation;

	if ( !WorldInfo.bDropDetail && (WorldInfo.DetailMode != DM_Low) && (SplashEffect != None) && !Instigator.PhysicsVolume.bWaterVolume )
	{
		// check for splash
		HitActor = Trace(HitLocation,HitNormal,mHitLocation,Instigator.Location,true,,true);
		if ( (PhysicsVolume(HitActor) != None) && PhysicsVolume(HitActor).bWaterVolume )
			Spawn(SplashEffect,,,HitLocation,rot(16384,0,0));
	}
}
*/

simulated function SetWeaponOverlayFlags(UTPawn OwnerPawn)
{
	local MaterialInterface InstanceToUse;
	local byte Flags;
	local int i;
	local UTGameReplicationInfo GRI;

	GRI = UTGameReplicationInfo(WorldInfo.GRI);
	if (GRI != None)
	{
		Flags = OwnerPawn.WeaponOverlayFlags;
		for (i = 0; i < GRI.WeaponOverlays.length; i++)
		{
			if (GRI.WeaponOverlays[i] != None && bool(Flags & (1 << i)))
			{
				InstanceToUse = GRI.WeaponOverlays[i];
				break;
			}
		}
	}
	if (InstanceToUse != none)
	{
		if (OverlayMesh == None)
		{
			CreateOverlayMesh();
		}
		if ( OverlayMesh != none )
		{
			for (i=0;i<OverlayMesh.GetNumElements(); i++)
			{
				OverlayMesh.SetMaterial(i, InstanceToUse);
			}

			OverlayMesh.SetHidden(false);
			if (!OverlayMesh.bAttached)
			{
				OwnerPawn.Mesh.AttachComponentToSocket(OverlayMesh, OwnerPawn.WeaponSocket);
			}
		}
	}
	else if ( OverlayMesh != none )
	{
		OverlayMesh.SetHidden(true);
		OwnerPawn.Mesh.DetachComponent(OverlayMesh);
	}
}

simulated function ChangeVisibility(bool bIsVisible)
{
	if (Mesh != None)
	{
		Mesh.SetHidden(!bIsVisible);
	}

	if (OverlayMesh != none)
	{
		OverlayMesh.SetHidden(!bIsVisible);
	}
}

simulated function FireModeUpdated(byte FiringMode, bool bViaReplication);

/** @return the starting location for effects (generally tracers) */
simulated function vector GetEffectLocation()
{
	local vector SocketLocation;

	if (MuzzleFlashSocket != 'None')
	{
		Mesh.GetSocketWorldLocationAndRotation(MuzzleFlashSocket, SocketLocation);
		return SocketLocation;
	}
	else
	{
		return Mesh.Bounds.Origin + (vect(45,0,0) >> Instigator.Rotation);
	}
}

defaultproperties
{
	// Weapon SkeletalMesh
	Begin Object Class=SkeletalMeshComponent Name=SkeletalMeshComponent0
		bOwnerNoSee=true
		bOnlyOwnerSee=false
		CollideActors=false
		AlwaysLoadOnClient=true
		AlwaysLoadOnServer=true
		CullDistance=4000
		bUseAsOccluder=FALSE
		bForceRefPose=true
		bUpdateSkelWhenNotRendered=false
		bIgnoreControllersWhenNotRendered=true
		bOverrideAttachmentOwnerVisibility=true
		bAcceptsDecals=false
	End Object
	Mesh=SkeletalMeshComponent0

	NetUpdateFrequency=10
	RemoteRole=ROLE_None
	bReplicateInstigator=true
	MaxImpactEffectDistance=4000.0
	MaxFireEffectDistance=5000.0
	bAlignToSurfaceNormal=true
	MuzzleFlashDuration=0.3
	MuzzleFlashColor=(R=255,G=255,B=255,A=255)
}
