/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class UTWeap_LinkGun extends UTBeamWeapon;

/** This defines the max. number of links that will be considered for damage, etc. */
var int MaxLinkStrength;

/** Holds the actor that this weapon is linked to. */
var Actor	LinkedTo;
/** Holds the component we hit on the linked actor, for determining the linked beam endpoint on multi-component actors (such as Onslaught powernodes) */
var PrimitiveComponent LinkedComponent;

/** players holding link guns within this distance of each other automatically link up */
var float WeaponLinkDistance;
/** Holds the list of link guns linked to this weapon */
var array<UTWeap_LinkGun> LinkedList;	// I made a funny Hahahahah :)

/** Holds the Actor currently being hit by the beam */
var Actor	Victim;

/** Holds the current strength (in #s) of the link */
var repnotify int LinkStrength;

/** Holds the amount of flexibility of the link beam */
var float 	LinkFlexibility;

/** Holds the amount of time to maintain the link before breaking it.  This is important so that you can pass through
    small objects without having to worry about regaining the link */
var float 	LinkBreakDelay;

/** Momentum transfer for link beam (per second) */
var float	MomentumTransfer;
/** beam ammo consumption (per second) */
var float BeamAmmoUsePerSecond;

/** This is a time used with LinkBrekaDelay above */
var float	ReaccquireTimer;

/** true if beam currently hitting target */
var bool	bBeamHit;

/** saved partial damage (in case of high frame rate */
var float	SavedDamage;

/** Saved partial ammo consumption */
var float	PartialAmmo;

var MaterialInstanceConstant WeaponMaterialInstance;

var UTLinkBeamLight BeamLight;

var SoundCue StartAltFireSound;
var SoundCue EndAltFireSound;

var UTEmitter BeamEndpointEffect;

/** activated whenever we're linked to other players (LinkStrength > 1) */
var ParticleSystemComponent PoweredUpEffect;
/** socket to attach PoweredUpEffect to on our mesh */
var name PoweredUpEffectSocket;

/** Where beam that isn't hitting a target is currently attached */
var vector BeamAttachLocation;

/** Last time new beam attachment location was calculated */
var float  LastBeamAttachTime;

/** Normal for beam attachment */
var vector BeamAttachNormal;

/** Actor to which beam is being attached */
var actor  BeamAttachActor;

/** cached cast of attachment class for calling coloring functions */
var class<UTAttachment_LinkGun> LinkAttachmentClass;

var ParticleSystem TeamMuzzleFlashTemplates[3];
var ParticleSystem HighPowerMuzzleFlashTemplate;

replication
{
	if (bNetDirty)
		LinkedTo, LinkStrength, bBeamHit;
}

simulated function PostBeginPlay()
{
	local color DefaultBeamColor;

	Super.PostBeginPlay();

	if (Role == ROLE_Authority)
	{
		SetTimer(0.25, true, 'FindLinkedWeapons');
	}

	LinkAttachmentClass = class<UTAttachment_LinkGun>(AttachmentClass);

	if (WorldInfo.NetMode != NM_DedicatedServer && Mesh != None)
	{
		LinkAttachmentClass.static.GetTeamBeamInfo(255, DefaultBeamColor);
		WeaponMaterialInstance = Mesh.CreateAndSetMaterialInstanceConstant(0);
		WeaponMaterialInstance.SetVectorParameterValue('TeamColor', ColorToLinearColor(DefaultBeamColor));
	}
}

simulated function ParticleSystem GetTeamMuzzleFlashTemplate(byte TeamNum)
{
	if (TeamNum >= ArrayCount(default.TeamMuzzleFlashTemplates))
	{
		TeamNum = ArrayCount(default.TeamMuzzleFlashTemplates) - 1;
	}
	return default.TeamMuzzleFlashTemplates[TeamNum];
}

simulated function SetSkin(Material NewMaterial)
{
	Super.SetSkin(NewMaterial);

	if (NewMaterial == None && Mesh != None)
	{
		Mesh.SetMaterial(0, WeaponMaterialInstance);
	}
}

simulated function AttachWeaponTo(SkeletalMeshComponent MeshCpnt, optional Name SocketName)
{
	Super.AttachWeaponTo(MeshCpnt, SocketName);

	if (PoweredUpEffect != None && !PoweredUpEffect.bAttached && SkeletalMeshComponent(Mesh) != None)
	{
		SkeletalMeshComponent(Mesh).AttachComponentToSocket(PoweredUpEffect, PoweredUpEffectSocket);
	}
}

simulated function UpdateBeamEmitter(vector FlashLocation, vector HitNormal)
{
	local color BeamColor;
	local ParticleSystem BeamSystem, BeamEndpointTemplate, MuzzleFlashTemplate;
	local byte TeamNum;

	if (LinkedTo != None)
	{
		FlashLocation = GetLinkedToLocation();
	}

	Super.UpdateBeamEmitter(FlashLocation, HitNormal);

	if (LinkedTo != None && WorldInfo.GRI.GameClass.Default.bTeamGame)
	{
		TeamNum = Instigator.GetTeamNum();
		LinkAttachmentClass.static.GetTeamBeamInfo(TeamNum, BeamColor, BeamSystem, BeamEndpointTemplate);
		MuzzleFlashTemplate = GetTeamMuzzleFlashTemplate(TeamNum);
	}
	else if ( (LinkStrength > 1) || (Instigator.DamageScaling >= 2.0) )
	{
		BeamColor = LinkAttachmentClass.default.HighPowerBeamColor;
		BeamSystem = LinkAttachmentClass.default.HighPowerSystem;
		BeamEndpointTemplate = LinkAttachmentClass.default.HighPowerBeamEndpointTemplate;
		MuzzleFlashTemplate = HighPowerMuzzleFlashTemplate;
	}
	else
	{
		LinkAttachmentClass.static.GetTeamBeamInfo(255, BeamColor, BeamSystem, BeamEndpointTemplate);

		MuzzleFlashTemplate = GetTeamMuzzleFlashTemplate(255);
	}

	if ( BeamLight != None )
	{
		if ( HitNormal == vect(0,0,0) )
		{
			BeamLight.Beamlight.Radius = 48;
			if ( FastTrace(FlashLocation, FlashLocation-vect(0,0,32)) )
				BeamLight.SetLocation(FlashLocation - vect(0,0,32));
			else
				BeamLight.SetLocation(FlashLocation);
		}
		else
		{
			BeamLight.Beamlight.Radius = 32;
			BeamLight.SetLocation(FlashLocation + 16*HitNormal);
		}
		BeamLight.BeamLight.SetLightProperties(, BeamColor);
	}

	if (BeamEmitter[CurrentFireMode] != None)
	{
		BeamEmitter[CurrentFireMode].SetColorParameter('Link_Beam_Color', BeamColor);
		if (BeamEmitter[CurrentFireMode].Template != BeamSystem)
		{
			BeamEmitter[CurrentFireMode].SetTemplate(BeamSystem);
		}
	}

	if (MuzzleFlashPSC != None)
	{
		MuzzleFlashPSC.SetColorParameter('Link_Beam_Color', BeamColor);
		if (MuzzleFlashTemplate != MuzzleFlashPSC.Template)
		{
			MuzzleFlashPSC.SetTemplate(MuzzleFlashTemplate);
		}
	}
	if (UTLinkGunMuzzleFlashLight(MuzzleFlashLight) != None)
	{
		UTLinkGunMuzzleFlashLight(MuzzleFlashLight).SetTeam((LinkedTo != None && WorldInfo.GRI.GameClass.Default.bTeamGame) ? Instigator.GetTeamNum() : byte(255));
	}

	if (WeaponMaterialInstance != None)
	{
		WeaponMaterialInstance.SetVectorParameterValue('TeamColor', ColorToLinearColor(BeamColor));
	}

	if (WorldInfo.NetMode != NM_DedicatedServer && Instigator != None && Instigator.IsFirstPerson())
	{
		if (BeamEndpointEffect != None && !BeamEndpointEffect.bDeleteMe)
		{
			BeamEndpointEffect.SetLocation(FlashLocation);
			BeamEndpointEffect.SetRotation(rotator(HitNormal));
			if (BeamEndpointEffect.ParticleSystemComponent.Template != BeamEndpointTemplate)
			{
				BeamEndpointEffect.SetTemplate(BeamEndpointTemplate, true);
			}
		}
		else
		{
			BeamEndpointEffect = Spawn(class'UTEmitter', self,, FlashLocation, rotator(HitNormal));
			BeamEndpointEffect.SetTemplate(BeamEndpointTemplate, true);
			BeamEndpointEFfect.LifeSpan = 0.0;
		}
	}
}

simulated function Projectile ProjectileFire()
{
	local UTProj_LinkPlasma Proj;

	IncrementFlashCount();
	if (Role == ROLE_Authority)
	{
		CalcLinkStrength();
		Proj = UTProj_LinkPlasma(super.ProjectileFire());
		if (LinkStrength > 1) // powered shot
		{
			if (UTProj_LinkPowerPlasma(Proj) != None)
			{
				UTProj_LinkPowerPlasma(Proj).SetStrength(LinkStrength);
			}
		}


	}
	return none;
}

function class<Projectile> GetProjectileClass()
{
	return (CurrentFireMode == 0 && LinkStrength > 1) ? class'UTProj_LinkPowerPlasma' : Super.GetProjectileClass();
}

/**
 * When destroyed, make sure we clean ourselves from any chains
 */
simulated event Destroyed()
{
	super.Destroyed();
	Unlink();
	LinkedComponent = None;
	if (BeamLight != None)
	{
		BeamLight.Destroy();
	}

	KillEndpointEffect();
}

simulated function SetBeamEmitterHidden(bool bHide)
{
	if (BeamEmitter[CurrentFireMode] != None && bHide)
	{
		KillEndpointEffect();
	}
	Super.SetBeamEmitterHidden(bHide);
}

simulated function KillBeamEmitter()
{
	Super.KillBeamEmitter();

	KillEndpointEffect();
}

/** deactivates the beam endpoint effect, if present */
simulated function KillEndpointEffect()
{
	if (BeamEndpointEffect != None)
	{
		BeamEndpointEffect.ParticleSystemComponent.DeactivateSystem();
		BeamEndpointEffect.LifeSpan = 2.0;
		BeamEndpointEffect = None;
	}
}

/** ConsumeBeamAmmo()
consume beam ammo per tick.
*/
function ConsumeBeamAmmo(float DeltaTime)
{
	PartialAmmo += BeamAmmoUsePerSecond * DeltaTime;
	if ( PartialAmmo >= 1)
	{
		PartialAmmo -= 1;
		ConsumeAmmo(1);
	}
}

/**
 * Process the hit info
 */
simulated function ProcessBeamHit(vector StartTrace, vector AimDir, out ImpactInfo TestImpact, float DeltaTime)
{
	local float DamageAmount;
	local vector PushForce, ShotDir, SideDir; //, HitLocation, HitNormal, AttachDir;
//	local actor HitActor;
//	local bool bAlreadyFailedOldSpot;

	Victim = TestImpact.HitActor;
/* @TODO FIXMESTEVE - use this for the extra beam tendrils when they get separated, bot not in low detail mode
	if ( (Victim == None) && !bBeamHit && (Instigator != None) && Instigator.IsLocallyControlled() && Instigator.IsHumanControlled() )
	{
		if ( (VSizeSq(BeamAttachLocation - TestImpact.HitLocation) < 40000)
			&& (WorldInfo.TimeSeconds - LastBeamAttachTime < 0.3) )
		{
			HitActor = Trace(HitLocation, HitNormal, BeamAttachLocation, TestImpact.HitLocation, true,,, TRACEFLAG_Bullet);
			if ( HitActor == None )
			{
				TestImpact.HitActor = BeamAttachActor;
				TestImpact.HitLocation = BeamAttachLocation;
				TestImpact.HitNormal = BeamAttachNormal;
				return;
			}
			else if ( HitActor.bWorldGeometry )
			{
				Victim = HitActor;
				TestImpact.HitActor = HitActor;
				BeamAttachActor = HitActor;
				TestImpact.HitLocation = HitLocation;
				BeamAttachLocation = TestImpact.HitLocation;
				TestImpact.HitNormal = HitNormal;
				BeamAttachNormal = HitNormal;
				return;
			}
			bAlreadyFailedOldSpot = true;
		}
		// Randomly try direction along shoot axis
		AttachDir = (Rand(3) - 1) * Normal(AimDir Cross vect(0,0,1));
		AttachDir.Z = Rand(3) - 1;

		// try tracing along attachdir, and use the hit if hit world
		HitActor = Trace(HitLocation, HitNormal, TestImpact.HitLocation + 160*AttachDir, TestImpact.HitLocation, true,,, TRACEFLAG_Bullet);
		if ( (HitActor != None) && HitActor.bWorldGeometry )
		{
			Victim = HitActor;
			TestImpact.HitActor = HitActor;
			BeamAttachActor = HitActor;
			TestImpact.HitLocation = HitLocation;
			BeamAttachLocation = TestImpact.HitLocation;
			TestImpact.HitNormal = HitNormal;
			BeamAttachNormal = HitNormal;
			LastBeamAttachTime = WorldInfo.TimeSeconds + 0.1*FRand() - 0.05;
			return;
		}
		else if ( !bAlreadyFailedOldSpot && (VSizeSq(BeamAttachLocation - TestImpact.HitLocation) < 40000) )
		{
			HitActor = Trace(HitLocation, HitNormal, BeamAttachLocation, TestImpact.HitLocation, true,,, TRACEFLAG_Bullet);
			if ( HitActor == None )
			{
				TestImpact.HitActor = BeamAttachActor;
				TestImpact.HitLocation = BeamAttachLocation;
				TestImpact.HitNormal = BeamAttachNormal;
				return;
			}
		}
		else
		{
			TestImpact.HitLocation = TestImpact.HitLocation - vect(0,0,100);
		}
		SetFlashLocation(TestImpact.HitLocation);
		return;
	}
*/
	// If we are on the server, attempt to setup the link
	if (Role == ROLE_Authority)
	{
		// Try linking
		AttemptLinkTo(Victim, TestImpact.HitInfo.HitComponent);

		// set the correct firemode on the pawn, since it will change when linked
		SetCurrentFireMode(CurrentFireMode);

		// if we do not have a link, set the flash location to whatever we hit
		// (if we do have one, AttemptLinkTo() will set the correct flash location for the Actor we're linked to)
		if (LinkedTo == None)
		{
			SetFlashLocation(TestImpact.HitLocation);
		}

		// cause damage or add health/power/etc.
		bBeamHit = false;

		// compute damage amount
		CalcLinkStrength();
		DamageAmount = InstantHitDamage[1];
		if ( LinkStrength > 1 )
		{
			DamageAmount *= Min(LinkStrength, MaxLinkStrength);
		}
		DamageAmount = DamageAmount*DeltaTime + SavedDamage;
		SavedDamage = DamageAmount - float(int(DamageAmount));
		DamageAmount -= SavedDamage;
		if ( DamageAmount >= 1 )
		{
			if ( LinkedTo != None )
			{
				// heal them if linked
				// linked players will use ammo when they fire
				if ( UTPawn(LinkedTo) == None )
				{
					if ( LinkedTo.IsA('UTVehicle') || LinkedTo.IsA('UTGameObjective') )
					{
						// use ammo only if we actually healed some damage
						if ( LinkedTo.HealDamage(DamageAmount * Instigator.GetDamageScaling(), Instigator.Controller, InstantHitDamageTypes[1]) )
							ConsumeBeamAmmo(DeltaTime);
					}
					else
					{
						// otherwise always use ammo
						ConsumeBeamAmmo(DeltaTime);
					}
				}
			}
			else
			{
				// If not on the same team, hurt them
				ConsumeBeamAmmo(DeltaTime);
				if (Victim != None && !WorldInfo.Game.GameReplicationInfo.OnSameTeam(Victim, Instigator))
				{
					bBeamHit = !Victim.bWorldGeometry;
					if ( DamageAmount > 0 )
					{
						ShotDir = Normal(TestImpact.HitLocation - Location);
						SideDir = Normal(ShotDir Cross vect(0,0,1));
						PushForce =  vect(0,0,1) + Normal(SideDir * (SideDir dot (TestImpact.HitLocation - Victim.Location)));
						PushForce *= (Victim.Physics == PHYS_Walking) ? 0.1*MomentumTransfer : DeltaTime*MomentumTransfer;
						Victim.TakeDamage(DamageAmount, Instigator.Controller, TestImpact.HitLocation, PushForce, InstantHitDamageTypes[1], TestImpact.HitInfo, self);
					}
				}
			}
		}
	}
	else
	{
		// if we do not have a link, set the flash location to whatever we hit
		// (otherwise beam update will override with link location)
		if (LinkedTo == None)
		{
			SetFlashLocation(TestImpact.HitLocation);
		}
		else if (TestImpact.HitActor == LinkedTo && TestImpact.HitInfo.HitComponent != None)
		{
			// the linked component can't be replicated to the client, so set it here
			LinkedComponent = TestImpact.HitInfo.HitComponent;
		}
	}
}

/**
 * Returns a vector that specifics the point of linking.
 */
simulated function vector GetLinkedToLocation()
{
	local name SocketName;
	local name BestSocket;
	local vector BestLoc;
	local float Dist,BestScore,Score;
	local vector Loc,ToTarget;
	
	BestSocket='';
	
	if (LinkedTo == None)
	{
		return vect(0,0,0);
	}
	else if(UTVehicle(LinkedTo) != none && UTVehicle(LinkedTo).LinkToSockets.Length != 0)
	{
		foreach (UTVehicle(LinkedTo).LinkToSockets)(SocketName)
		{
			UTVehicle(LinkedTo).Mesh.GetSocketWorldLocationAndRotation(SocketName,Loc);
			Dist = VSizeSq(Location-Loc);
			if(BestSocket == '' || Dist<BestScore) // low == closer
			{
				BestSocket = SocketName;
				BestLoc = Loc;
				BestScore = Dist;
			}
		}
		return BestLoc;
	}
	else if(UTOnslaughtPowerNode(LinkedTo) != none && UTOnslaughtPowerNode(LinkedTo).LinkToSockets.Length != 0)
	{
		foreach (UTOnslaughtPowerNode(LinkedTo).LinkToSockets)(SocketName)
		{
			UTOnslaughtPowerNode(LinkedTo).EnergySphere.GetSocketWorldLocationAndRotation(SocketName,Loc);
			ToTarget = Location-Loc;
			Dist = VSizeSq(ToTarget);
			if(Instigator.Controller != none)
			{
				Score = Abs(ToTarget/VSize(ToTarget) dot vector(Instigator.GetViewRotation()));
			}
			else
			{
				Score=1/Dist;
			}
			if(BestSocket == '' || Score>BestScore)
			{
				BestSocket = SocketName;
				BestLoc = Loc;
				BestScore = Score;
			}
		}
		return BestLoc;
	}
	else if (Pawn(LinkedTo) != None)
	{
		return LinkedTo.Location + Pawn(LinkedTo).BaseEyeHeight * vect(0,0,0.5);
	}
	else if (LinkedComponent != None)
	{
		return LinkedComponent.GetPosition();
	}
	else
	{
		return LinkedTo.Location;
	}
}

/**
 * This function looks at how the beam is hitting and determines if this person is linkable
 */
function AttemptLinkTo(Actor Who, PrimitiveComponent HitComponent)
{
	local UTVehicle UTV;
	local UTOnslaughtObjective UTO;
	local Vector 		StartTrace, EndTrace, V, HitLocation, HitNormal;
	local Actor			HitActor;

	// redirect to vehicle if owned by a vehicle and the vehicle allows it
	if( Who != none )
	{
		UTV = UTVehicle(Who.Owner);
		if (UTV != None && UTV.AllowLinkThroughOwnedActor(Who))
		{
			Who = UTV;
		}
	}

	// Check for linking to pawns
	UTV = UTVehicle(Who);
	if ( (UTV != None) && UTV.bValidLinkTarget )
	{
		// Check teams to make sure they are on the same side
		if (WorldInfo.Game.GameReplicationInfo.OnSameTeam(Who,Instigator))
		{
			if (UTV != None)
			{
				LinkedComponent = HitComponent;
				if ( LinkedTo != UTV )
				{
					UnLink();
					LinkedTo = UTV;
					UTV.StartLinkedEffect();
				}
			}
			else
			{
				// not a linkable pawn type, break any links
				UnLink();
			}
		}
		else
		{
			// Enemy got in the way, break any links
			UnLink();
		}
	}
	else
	{
		UTO = UTOnslaughtObjective(Who);
		if ( UTO != none )
		{
			if ( (UTO.LinkHealMult > 0) && WorldInfo.Game.GameReplicationInfo.OnSameTeam(UTO,Instigator) )
			{
				if(LinkedTo != Who)
				{
					Unlink();
				}
				LinkedTo = UTO;
				LinkedComponent = HitComponent;
			}
			else
			{
				UnLink();
			}
		}
	}

	if (LinkedTo != None)
	{
		// Determine if the link has been broken for another reason

		if (LinkedTo.bDeleteMe || (Pawn(LinkedTo) != None && Pawn(LinkedTo).Health <= 0))
		{
			UnLink();
			return;
		}

		// if we were passed in LinkedTo, we know we hit it straight on already, so skip the rest
		if (LinkedTo != Who)
		{
			StartTrace = Instigator.GetWeaponStartTraceLocation();
			EndTrace = GetLinkedtoLocation();

			// First, check to see if we have skewed too much, or if the LinkedTo pawn has died and
			// we didn't get cleaned up.
			V = Normal(EndTrace - StartTrace);
			if ( V dot vector(Instigator.GetViewRotation()) < LinkFlexibility || VSize(EndTrace - StartTrace) > 1.5 * WeaponRange )
			{
				UnLink();
				return;
			}

			//  If something is blocking us and the actor, drop the link
			HitActor = Trace(HitLocation, HitNormal, EndTrace, StartTrace, true);
			if (HitActor != none && HitActor != LinkedTo)
			{
				UnLink(true);		// In this case, use a delayed UnLink
			}
		}
	}

	// if we are linked, make sure the proper flash location is set
	if (LinkedTo != None)
	{
		SetFlashLocation(GetLinkedtoLocation());
	}
}

/**
 * Unlink this weapon from it's parent.  If bDelayed is true, it will give a
 * short delay before unlinking to allow the player to re-establish the link
 */
function UnLink(optional bool bDelayed)
{
	local UTVehicle V;

	if (!bDelayed)
	{
		V = UTVehicle(LinkedTo);
		if(V != none)
		{
			V.StopLinkedEffect();
		}

		LinkedTo = None;
		LinkedComponent = None;
	}
	else if (ReaccquireTimer <= 0)
	{
		// Set the Delay timer
		ReaccquireTimer = LinkBreakDelay;
	}
}

/** checks for nearby friendly link gun users and links to them */
function FindLinkedWeapons()
{
	local UTPawn P;
	local UTWeap_LinkGun Link;

	LinkedList.length = 0;
	if (Instigator != None && (bReadyToFire() || IsFiring()))
	{
		foreach WorldInfo.AllPawns(class'UTPawn', P, Instigator.Location, WeaponLinkDistance)
		{
			if (P != Instigator && !P.bNoWeaponFiring && P.DrivenVehicle == None)
			{
				Link = UTWeap_LinkGun(P.Weapon);
				if (Link != None && WorldInfo.GRI.OnSameTeam(Instigator, P) && FastTrace(P.Location, Instigator.Location))
				{
					LinkedList[LinkedList.length] = Link;
				}
			}
		}
	}
	CalcLinkStrength();

	if (WorldInfo.NetMode != NM_DedicatedServer && PoweredUpEffect != None)
	{
		if (LinkStrength > 1)
		{
			if (!PoweredUpEffect.bIsActive)
			{
				PoweredUpEffect.ActivateSystem();
			}
		}
		else if (PoweredUpEffect.bIsActive)
		{
			PoweredUpEffect.DeactivateSystem();
		}
	}
}

/** gets a list of the entire link chain */
function GetLinkedWeapons(out array<UTWeap_LinkGun> LinkedWeapons)
{
	local int i;

	LinkedWeapons[LinkedWeapons.length] = self;
	for (i = 0; i < LinkedList.length; i++)
	{
		if (LinkedWeapons.Find(LinkedList[i]) == INDEX_NONE)
		{
			LinkedList[i].GetLinkedWeapons(LinkedWeapons);
		}
	}
}

/** this function figures out the strength of this link */
function CalcLinkStrength()
{
	local array<UTWeap_LinkGun> LinkedWeapons;

	GetLinkedWeapons(LinkedWeapons);
	LinkStrength = LinkedWeapons.length;
}

simulated function ChangeVisibility(bool bIsVisible)
{
	Super.ChangeVisibility(bIsVisible);

	if (PoweredUpEffect != None)
	{
		PoweredUpEffect.SetHidden(!bIsVisible);
	}
}

simulated event ReplicatedEvent(name VarName)
{
	if (VarName == 'LinkStrength')
	{
		if (LinkStrength > 1)
		{
			if (!PoweredUpEffect.bIsActive)
			{
				PoweredUpEffect.ActivateSystem();
			}
		}
		else if (PoweredUpEffect.bIsActive)
		{
			PoweredUpEffect.DeactivateSystem();
		}
	}
	else
	{
		Super.ReplicatedEvent(VarName);
	}
}

/*********************************************************************************************
 * State WeaponFiring
 * See UTWeapon.WeaponFiring
 *********************************************************************************************/

simulated state WeaponBeamFiring
{
	/**
	 * When done firing, we have to make sure we unlink the weapon.
	 */
	simulated function EndFire(byte FireModeNum)
	{
		UnLink();
		super.EndFire(FireModeNum);
	}

	simulated function SetCurrentFireMode(byte FiringModeNum)
	{
		local byte InstigatorFireMode;

		CurrentFireMode = FiringModeNum;

		// on the pawn, set a value of 2 if we're linked so the weapon attachment knows the difference
		// and a value of 3 if we're not linked to anyone but others are linked to us
		if (Instigator != None)
		{
			if (CurrentFireMode == 1)
			{
				if (LinkedTo != None)
				{
					InstigatorFireMode = 2;
				}
				else
				{
					CalcLinkStrength();
					if ( (LinkStrength > 1) || (Instigator.DamageScaling >= 2.0) )
					{
						if ( bBeamHit )
							InstigatorFireMode = 4;
						else
							InstigatorFireMode = 3;
					}
					else
					{
						if ( bBeamHit )
							InstigatorFireMode = 5;
						else
							InstigatorFireMode = CurrentFireMode;
					}
				}
			}
			else
			{
				InstigatorFireMode = CurrentFireMode;
			}

			Instigator.SetFiringMode(InstigatorFireMode);
		}
	}

	function SetFlashLocation(vector HitLocation)
	{
		Global.SetFlashLocation(HitLocation);
		// SetFlashLocation() resets Instigator's FiringMode so we need to make sure our overridden value stays applied
		SetCurrentFireMode(CurrentFireMode);
	}

	/**
	 * Update the beam and handle the effects
	 */
	simulated function Tick(float DeltaTime)
	{
		// If we are in danger of losing the link, check to see if
		// time has run out.
		if ( ReaccquireTimer > 0 )
		{
	    		ReaccquireTimer -= DeltaTime;
	    		if (ReaccquireTimer <= 0)
	    		{
		    		ReaccquireTimer = 0.0;
		    		UnLink();
		    	}
		}

		// Retrace everything and see if there is a new LinkedTo or if something has changed.
		UpdateBeam(DeltaTime);
	}

	simulated function BeginState(Name PreviousStateName)
	{
		Super.BeginState(PreviousStateName);

		if ( (PlayerController(Instigator.Controller) != None) && Instigator.IsLocallyControlled() && ((BeamLight == None) || BeamLight.bDeleteMe) )
		{
			BeamLight = spawn(class'UTLinkBeamLight');
		}

		WeaponPlaySound(StartAltFireSound);
	}

	simulated function EndState(Name NextStateName)
	{
		local color EffectColor;

		WeaponPlaySound(EndAltFireSound);

		Super.EndState(NextStateName);

		if ( BeamLight != None )
			BeamLight.Destroy();

		ReaccquireTimer = 0.0;
		UnLink();
		Victim = None;

		// reset material and muzzle flash to default color
		LinkAttachmentClass.static.GetTeamBeamInfo(255, EffectColor);
		if (WeaponMaterialInstance != None)
		{
			WeaponMaterialInstance.SetVectorParameterValue('TeamColor', ColorToLinearColor(EffectColor));
		}
		if (MuzzleFlashPSC != None)
		{
			MuzzleFlashPSC.ClearParameter('Link_Beam_Color');
		}
	}
}

simulated state WeaponPuttingDown
{
	simulated function WeaponIsDown()
	{
		// make sure we're completely unlinked before we change weapons
		Unlink();
		LinkedList.length = 0;

		Super.WeaponIsDown();
	}
}


//-----------------------------------------------------------------
// AI Interface

//@FIXME: fix for new player link system (if kept)
function float GetAIRating()
{
	local UTBot B;
	local UTVehicle V;
	local UTGameObjective O;

	B = UTBot(Instigator.Controller);
	if (B == None || B.Squad == None)
	{
		return AIRating;
	}

	if ( (PlayerController(B.Squad.SquadLeader) != None)
		&& (B.Squad.SquadLeader.Pawn != None)
		&& (UTWeap_LinkGun(B.Squad.SquadLeader.Pawn.Weapon) != None) )
	{
		return 1.2;
	}

	V = B.Squad.GetLinkVehicle(B);
	if ( (V != None)
		&& (VSize(Instigator.Location - V.Location) < 1.5 * WeaponRange)
		&& (V.Health < V.Default.Health) && (V.LinkHealMult > 0) )
	{
		return 1.2;
	}

	V = UTVehicle(B.RouteGoal);
	if ( (V != None) && (B.Enemy == None) && (VSize(Instigator.Location - B.RouteGoal.Location) < 1.5 * WeaponRange)
	     && V.TeamLink(B.GetTeamNum()) )
	{
		return 1.2;
	}

	O = B.Squad.SquadObjective;
	if (O != None && O.TeamLink(B.GetTeamNum()) && O.NeedsHealing()
	     && VSize(Instigator.Location - O.Location) < 1.1 * GetTraceRange() && B.LineOfSightTo(O))
	{
		return 1.2;
	}

	return AIRating * FMin(Pawn(Owner).GetDamageScaling(), 1.5);
}

//@FIXME: fix for new player link system (if kept)
function bool FocusOnLeader(bool bLeaderFiring)
{
	local UTBot B;
	local Pawn LeaderPawn;
	local Actor Other;
	local vector HitLocation, HitNormal, StartTrace;
	local UTVehicle V;

	B = UTBot(Instigator.Controller);
	if ( B == None )
	{
		return false;
	}
	if ( PlayerController(B.Squad.SquadLeader) != None )
	{
		LeaderPawn = B.Squad.SquadLeader.Pawn;
	}
	else
	{
		V = B.Squad.GetLinkVehicle(B);
		if ( V != None )
		{
			LeaderPawn = V;
			bLeaderFiring = (LeaderPawn.Health < LeaderPawn.default.Health) && (V.LinkHealMult > 0)
							&& ((B.Enemy == None) || V.bKeyVehicle);
		}
	}
	if ( LeaderPawn == None )
	{
		LeaderPawn = B.Squad.SquadLeader.Pawn;
		if ( LeaderPawn == None )
		{
			return false;
		}
	}
	if ( !bLeaderFiring && (LeaderPawn.Weapon == None || !LeaderPawn.Weapon.IsFiring()) )
	{
		return false;
	}
	if ( UTVehicle(LeaderPawn) != None
		|| ((UTWeap_LinkGun(LeaderPawn.Weapon) != None) && ((vector(B.Squad.SquadLeader.Rotation) dot Normal(Instigator.Location - LeaderPawn.Location)) < 0.9)) )
	{
		StartTrace = Instigator.GetPawnViewLocation();
		if (VSize(LeaderPawn.Location - StartTrace) < GetTraceRange())
		{
			Other = Trace(HitLocation, HitNormal, LeaderPawn.Location, StartTrace, true);
			if ( Other == LeaderPawn )
			{
				B.Focus = Other;
				return true;
			}
		}
	}
	return false;
}

/* BestMode()
choose between regular or alt-fire
*/
//@FIXME: fix for new player link system (if kept)
function byte BestMode()
{
	local float EnemyDist;
	local UTBot B;
	local UTVehicle V;
	local UTGameObjective ObjTarget;

	B = UTBot(Instigator.Controller);
	if ( B == None )
	{
		return 0;
	}

	ObjTarget = UTGameObjective(B.Focus);
	if ( (ObjTarget != None) && ObjTarget.TeamLink(B.GetTeamNum()) )
	{
		return 1;
	}
	if ( FocusOnLeader(B.Focus == B.Squad.SquadLeader.Pawn) )
	{
		return 1;
	}

	V = UTVehicle(B.Focus);
	if ( (V != None) && WorldInfo.GRI.OnSameTeam(B,V) )
	{
		return 1;
	}
	if ( B.Enemy == None )
	{
		return 0;
	}
	EnemyDist = VSize(B.Enemy.Location - Instigator.Location);
	if ( EnemyDist > WeaponRange )
	{
		return 0;
	}
	// @TODO FIXMESTEVE gamer's day hack (till have new aiming model)
	return 0;
}

function bool CanHeal(Actor Other)
{
	if (!HasAmmo(1))
	{
		return false;
	}
	else if (UTGameObjective(Other) != None)
	{
		return UTGameObjective(Other).TeamLink(Instigator.GetTeamNum());
	}
	else
	{
		return (UTVehicle(Other) != None && UTVehicle(Other).LinkHealMult > 0.f);
	}
}

function float GetOptimalRangeFor(Actor Target)
{
	// return alt beam range if shooting at teammate (healing/linking)
	return (WorldInfo.GRI.OnSameTeam(Target, Instigator) ? WeaponRange : Super.GetOptimalRangeFor(Target));
}

function float SuggestAttackStyle()
{
	return 0.8;
}

function float SuggestDefenseStyle()
{
	return -0.4;
}

defaultproperties
{
	WeaponColor=(R=255,G=255,B=0,A=255)
	FireInterval(0)=+0.16
	FireInterval(1)=+0.35
	PlayerViewOffset=(X=16.0,Y=-18,Z=-18.0)

	FiringStatesArray(1)=WeaponBeamFiring

	Begin Object class=AnimNodeSequence Name=MeshSequenceA
		bCauseActorAnimEnd=true
	End Object

	// Weapon SkeletalMesh
	Begin Object Name=FirstPersonMesh
		SkeletalMesh=SkeletalMesh'WP_LinkGun.Mesh.SK_WP_Linkgun_1P'
		AnimSets(0)=AnimSet'WP_LinkGun.Anims.K_WP_LinkGun_1P_Base'
		Animations=MeshSequenceA
		Scale=0.9
		FOV=60.0
	End Object

	AttachmentClass=class'UTAttachment_Linkgun'

	// Pickup staticmesh
	Begin Object Name=PickupMesh
		SkeletalMesh=SkeletalMesh'WP_LinkGun.Mesh.SK_WP_LinkGun_3P'
	End Object

	Begin Object Class=ParticleSystemComponent Name=PoweredUpComponent
		Template=ParticleSystem'WP_LinkGun.Effects.P_WP_Linkgun_PoweredUp'
		bAutoActivate=false
		DepthPriorityGroup=SDPG_Foreground
		SecondsBeforeInactive=1.0f
	End Object
	PoweredUpEffect=PoweredUpComponent
	PoweredUpEffectSocket=PowerEffectSocket

	FireOffset=(X=12,Y=10,Z=-10)

	WeaponFireTypes(0)=EWFT_Projectile
	WeaponProjectiles(0)=class'UTProj_LinkPlasma' // UTProj_LinkPowerPlasma if linked (see GetProjectileClass() )

	WeaponEquipSnd=SoundCue'A_Weapon_Link.Cue.A_Weapon_Link_RaiseCue'
	WeaponPutDownSnd=SoundCue'A_Weapon_Link.Cue.A_Weapon_Link_LowerCue'
	WeaponFireSnd(0)=SoundCue'A_Weapon_Link.Cue.A_Weapon_Link_FireCue'
	WeaponFireSnd(1)=SoundCue'A_Weapon_Link.Cue.A_Weapon_Link_AltFireCue'

	MaxDesireability=0.7
	AIRating=+0.71
	CurrentRating=+0.71
	BotRefireRate=0.99
	bInstantHit=false
	bSplashJump=false
	bSplashDamage=false
	bRecommendSplashDamage=false
	bSniping=false
	ShouldFireOnRelease(0)=0
	ShouldFireOnRelease(1)=0
	InventoryGroup=5
	GroupWeight=0.5

	WeaponRange=900
	LinkStrength=1
	LinkFlexibility=0.64	// determines how easy it is to maintain a link.
							// 1=must aim directly at linkee, 0=linkee can be 90 degrees to either side of you

	LinkBreakDelay=0.5		// link will stay established for this long extra when blocked (so you don't have to worry about every last tree getting in the way)
	WeaponLinkDistance=350.0

	InstantHitDamage(1)=100
	InstantHitDamageTypes(1)=class'UTDmgType_LinkBeam'

	PickupSound=SoundCue'A_Pickups.Weapons.Cue.A_Pickup_Weapons_Link_Cue'

	AmmoCount=50
	LockerAmmoCount=100
	MaxAmmoCount=220
	MomentumTransfer=50000.0
	BeamAmmoUsePerSecond=8.5

	FireShake(0)=(OffsetMag=(X=0.0,Y=1.0,Z=0.0),OffsetRate=(X=0.0,Y=-2000.0,Z=0.0),OffsetTime=4,RotMag=(X=40.0,Y=0.0,Z=0.0),RotRate=(X=2000.0,Y=0.0,Z=0.0),RotTime=2)
	FireShake(1)=(OffsetMag=(X=0.0,Y=1.0,Z=1.0),OffsetRate=(X=1000.0,Y=1000.0,Z=1000.0),OffsetTime=3,RotMag=(X=0.0,Y=0.0,Z=60.0),RotRate=(X=0.0,Y=0.0,Z=4000.0),RotTime=6)

	EffectSockets(0)=MuzzleFlashSocket
	EffectSockets(1)=MuzzleFlashSocket

	BeamPreFireAnim(1)=WeaponAltFireStart
	BeamFireAnim(1)=WeaponAltFire
	BeamPostFireAnim(1)=WeaponAltFireEnd

	MaxLinkStrength=3

	MuzzleFlashSocket=MuzzleFlashSocket
	MuzzleFlashPSCTemplate=ParticleSystem'WP_LinkGun.Effects.P_FX_LinkGun_MF_Primary'
	MuzzleFlashAltPSCTemplate=ParticleSystem'WP_LinkGun.Effects.P_FX_LinkGun_MF_Beam'
	bMuzzleFlashPSCLoops=true
	MuzzleFlashLightClass=class'UTGame.UTLinkGunMuzzleFlashLight'

	TeamMuzzleFlashTemplates[0]=ParticleSystem'WP_LinkGun.Effects.P_FX_LinkGun_MF_Beam_Red'
	TeamMuzzleFlashTemplates[1]=ParticleSystem'WP_LinkGun.Effects.P_FX_LinkGun_MF_Beam_Blue'
	TeamMuzzleFlashTemplates[2]=ParticleSystem'WP_LinkGun.Effects.P_FX_LinkGun_MF_Beam'
	HighPowerMuzzleFlashTemplate=ParticleSystem'WP_LinkGun.Effects.P_FX_LinkGun_MF_Beam_Gold'

	MuzzleFlashColor=(R=120,G=255,B=120,A=255)
	MuzzleFlashDuration=0.33;

	BeamTemplate[1]=ParticleSystem'WP_LinkGun.Effects.P_WP_Linkgun_Altbeam'
	BeamSockets[1]=MuzzleFlashSocket02
	EndPointParamName=LinkBeamEnd

	IconX=412
	IconY=82
	IconWidth=40
	IconHeight=36

	StartAltFireSound=SoundCue'A_Weapon_Link.Cue.A_Weapon_Link_AltFireStartCue'
	EndAltFireSound=SoundCue'A_Weapon_Link.Cue.A_Weapon_Link_AltFireStopCue'
	CrossHairCoordinates=(U=384,V=0,UL=64,VL=64)

	LockerRotation=(pitch=0,yaw=0,roll=-16384)
	IconCoordinates=(U=273,V=159,UL=84,VL=50)

	QuickPickGroup=3
	QuickPickWeight=0.8
}
