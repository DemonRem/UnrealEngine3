/**
 * UTDamageType
 *
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

class UTDamageType extends DamageType
	abstract;

var	LinearColor		DamageBodyMatColor;
var float           DamageOverlayTime;
var float           DeathOverlayTime;

var		bool			bDirectDamage;
var		bool            bSeversHead;
var		bool			bCauseConvulsions;
var		bool			bUseTearOffMomentum;	// For ragdoll death. Add entirety of killing hit's momentum to ragdoll's initial velocity.
var		bool			bThrowRagdoll;
var		bool			bLeaveBodyEffect;
var		bool            bBulletHit;
var		bool			bVehicleHit;		// caused by vehicle running over you

/** If true, this type of damage blows up barricades */
var		bool			bDestroysBarricades;

var		float           GibPerterbation;				// When gibbing, the chunks will fly off in random directions.
var		int				GibThreshold;					// Health threshold at which this damagetype gibs
var		int				MinAccumulateDamageThreshold;	// Minimum damage in one tick to cause gibbing when health is below gib threshold
var		int				AlwaysGibDamageThreshold;		// Minimum damage in one tick to always cause gibbing
/** magnitude of momentum that must be caused by this damagetype for physics based takehit animations to be used on the target */
var float PhysicsTakeHitMomentumThreshold;

/** Information About the weapon that caused this if available */

var 	class<UTWeapon>			DamageWeaponClass;
var		int						DamageWeaponFireMode;

 /** This will delegate to the death effects to the damage type.  This allows us to have specific
 *  damage effects without polluting the Pawn / Weapon class with checking for the damage type
 *
 **/
var() bool                  bUseDamageBasedDeathEffects;

/** Particle system trail to attach to gibs caused by this damage type */
var ParticleSystem GibTrail;

/** This is the Camera Effect you get when you die from this Damage Type **/
var class<UTEmitCameraEffect> DeathCameraEffectVictim;
/** This is the Camera Effect you get when you cause from this Damage Type **/
var class<UTEmitCameraEffect> DeathCameraEffectInstigator;

/** Name of animation to play upon death. */
var() name DeathAnim;
/** If true, char is stopped and root bone is animated using a bone spring for this type of death. */
var() bool bAnimateHipsForDeathAnim;

/** camera anim played instead of the default damage shake when taking this type of damage */
var CameraAnim DamageCameraAnim;

/** Damage scaling when hit warfare node/core */
var float NodeDamageScaling;

/**
 * Possibly spawn a custom hit effect
 */
static function SpawnHitEffect(Pawn P, float Damage, vector Momentum, name BoneName, vector HitLocation);

/** @return duration of hit effect, primarily used for replication timeout to avoid replicating out of date hits to clients when pawns become relevant */
static function float GetHitEffectDuration(Pawn P, float Damage)
{
	return 0.5;
}

static function PawnTornOff(UTPawn DeadPawn);

/** allows DamageType to spawn additional effects on gibs (such as flame trails) */
static function SpawnGibEffects(UTGib Gib)
{
	local ParticleSystemComponent Effect;

	if (default.GibTrail != None)
	{
		Effect = new(Gib) class'UTParticleSystemComponent';
		Effect.SetTemplate(Default.GibTrail);
		Gib.AttachComponent(Effect);
	}
}

/**
* @param DeadPawn is pawn killed by this damagetype
* @return whether or not we should gib due to damage
*/
static function bool ShouldGib(UTPawn DeadPawn)
{
	return ( !Default.bNeverGibs && (Default.bAlwaysGibs || (DeadPawn.AccumulateDamage > Default.AlwaysGibDamageThreshold) || ((DeadPawn.Health < Default.GibThreshold) && (DeadPawn.AccumulateDamage > Default.MinAccumulateDamageThreshold))) );
}


static function DoCustomDamageEffects(UTPawn ThePawn, class<UTDamageType> TheDamageType, const out TraceHitInfo HitInfo, vector HitLocation)
{
	`log("UTDamageType base DoCustomDamageEffects should never be called");
	ScriptTrace();
}


/**
* This will create a skeleton (white boney skeleton) on death.
*
* Currently it doesn't play any Player Death effects as we don't have them yet.
**/
static function CreateDeathSkeleton(UTPawn ThePawn, class<UTDamageType> TheDamageType, const out TraceHitInfo HitInfo, vector HitLocation)
{
	local Array<Attachment> PreviousAttachments;
	local int				i, Idx;
	local array<texture>	Textures;
	local SkeletalMeshComponent PawnMesh;
	local vector Impulse;
	local vector ShotDir;
	local MaterialInstanceConstant MIC_BurnOut;
	local class<UTFamilyInfo> FamilyInfo;


	PawnMesh = ThePawn.Mesh;
	ShotDir = Normal(ThePawn.TearOffMomentum);

	//Mesh.bIgnoreControllers = 1;
	PreviousAttachments = PawnMesh.Attachments;
	ThePawn.SetCollisionSize( 1.0f, 1.0f );
	ThePawn.CylinderComponent.SetTraceBlocking( FALSE, FALSE );

	PawnMesh.SetSkeletalMesh( ThePawn.GetFamilyInfo().default.DeathMeshSkelMesh, TRUE );
	PawnMesh.SetPhysicsAsset( ThePawn.GetFamilyInfo().default.DeathMeshPhysAsset );

	MIC_BurnOut = new(PawnMesh.outer) class'MaterialInstanceConstant';
	MIC_BurnOut.SetParent( Material'CH_Skeletons.Materials.M_CH_Skeletons_Human_01_BO' );
	MIC_BurnOut.SetScalarParameterValue( 'BurnAmount', 0.0f );

	// this can have a max of 6 before it wraps and become visible again
	PawnMesh.SetMaterial( 0, MIC_BurnOut );
	ThePawn.bDeathMeshIsActive = TRUE;

	PawnMesh.MotionBlurScale = 0.0f;

	// Make sure all bodies are unfixed
	if( PawnMesh.PhysicsAssetInstance != none )
	{
		PawnMesh.PhysicsAssetInstance.SetAllBodiesFixed(FALSE);

		// Turn off motors
		PawnMesh.PhysicsAssetInstance.SetAllMotorsAngularPositionDrive(FALSE, FALSE);
	}
	else
	{
		`log( "PawnMesh.PhysicsAssetInstance is NONE!!" );
	}

	for( Idx = 0; Idx < PreviousAttachments.length; ++Idx )
	{
		PawnMesh.AttachComponent( PreviousAttachments[Idx].Component, PreviousAttachments[Idx].BoneName,
					PreviousAttachments[Idx].RelativeLocation, PreviousAttachments[Idx].RelativeRotation,
					PreviousAttachments[Idx].RelativeScale );
	}


	FamilyInfo = ThePawn.GetFamilyInfo();

	// set all of the materials on the death mesh to be resident
	for( Idx = 0; Idx < FamilyInfo.default.DeathMeshNumMaterialsToSetResident; ++Idx )
	{
		Textures = FamilyInfo.default.DeathMeshSkelMesh.Materials[Idx].GetMaterial().GetTextures();

		for( i = 0; i < Textures.Length; ++i )
		{
			//`log( "Texture setting bForceMiplevelsToBeResident TRUE: " $ Textures[i] );
			Texture2D(Textures[i]).bForceMiplevelsToBeResident = TRUE;
		}
	}


	Impulse = ShotDir * TheDamageType.default.KDamageImpulse;
	BoneBreaker( ThePawn, PawnMesh, Impulse, HitLocation, HitInfo.BoneName );
}


/**
 * This will take a bonename and impulse info and break the constraint at that bone.
 *
 **/
static function BoneBreaker(UTPawn ThePawn, SkeletalMeshComponent TheMesh, vector Impulse, vector HitLocation, name BoneName)
{
	local int ConstraintIndex;

	// the issue is that for some weapon hits there is no bone name hit
	if (BoneName == 'None')
	{
		BoneName = ThePawn.GetFamilyInfo().default.DeathMeshBreakableJoints[ Rand( ThePawn.GetFamilyInfo().default.DeathMeshBreakableJoints.Length ) ];
	}

	ConstraintIndex = TheMesh.FindConstraintIndex( BoneName );

	if (ConstraintIndex != INDEX_NONE)
	{
		TheMesh.PhysicsAssetInstance.Constraints[ConstraintIndex].TermConstraint();

		// @see PlayDying:  we also do this in the steps after init ragdoll to the full body
		TheMesh.AddImpulse( Impulse, HitLocation, BoneName );
	}
	else
	{
		`log( "was unable to find the Constraint!!!" );
	}
}


/**
* This will create the gore chunks from a special death
*
* Note: temp here until we get real chunks and know the set of death types we are going to have
**/
static function CreateDeathGoreChunks(UTPawn ThePawn, class<UTDamageType> TheDamageType, const out TraceHitInfo HitInfo, vector HitLocation)
{
	local int i;
	local bool bSpawnHighDetail;
	local UTGib TheGib;
	local GibInfo GibInfo;

	// gib particles
	if( ThePawn.GibExplosionTemplate != None && ThePawn.EffectIsRelevant(ThePawn.Location, false, 7000) )
	{
		ThePawn.WorldInfo.MyEmitterPool.SpawnEmitter(ThePawn.GibExplosionTemplate, ThePawn.Location, ThePawn.Rotation);
	}

	// spawn all other gibs
	bSpawnHighDetail = !ThePawn.WorldInfo.bDropDetail && (ThePawn.Worldinfo.TimeSeconds - ThePawn.LastRenderTime < 1);
	for( i = 0; i < ThePawn.CurrFamilyInfo.default.Gibs.length; ++i )
	{
		GibInfo = ThePawn.CurrFamilyInfo.default.Gibs[i];

		if( bSpawnHighDetail || !GibInfo.bHighDetailOnly )
		{
			TheGib = ThePawn.SpawnGib(GibInfo.GibClass, GibInfo.BoneName, TheDamageType, HitLocation);
			SpawnExtraGibEffects( TheGib );
		}
	}

	ThePawn.bGibbed = true;
}

/** allows special effects when gibs are spawned via DoCustomDamageEffects() instead of the normal way */
simulated static function SpawnExtraGibEffects( UTGib TheGib );

/** This will Spawn Camera effects on the controller **/
simulated static function SpawnCameraEffectVictim( UTPawn ThePawn )
{
	if( UTPlayerController(ThePawn.Controller) != none )
	{
		UTPlayerController(ThePawn.Controller).ClientSpawnCameraEffect( ThePawn, default.DeathCameraEffectVictim );
	}
}

simulated static function SpawnCameraEffectInstigator( UTPawn ThePawn )
{
	if( UTPlayerController(ThePawn.Controller) != none )
	{
		UTPlayerController(ThePawn.Controller).ClientSpawnCameraEffect( ThePawn, default.DeathCameraEffectInstigator );
	}
}

simulated static function DrawKillIcon(Canvas Canvas, float ScreenX, float ScreenY, float HUDScaleX, float HUDScaleY)
{
	if ( default.DamageWeaponClass != None )
	{
		default.DamageWeaponClass.static.DrawKillIcon(Canvas, ScreenX, ScreenY, HUDScaleX, HUDScaleY);
	}
}

defaultproperties
{
	DamageBodyMatColor=(R=10)
	DamageOverlayTime=0.1
	DeathOverlayTime=0.1
	bDirectDamage=true
	GibPerterbation=0.06
	GibThreshold=-50
	GibTrail=ParticleSystem'T_FX.Effects.P_FX_Bloodsmoke_Trail'
	MinAccumulateDamageThreshold=50
	AlwaysGibDamageThreshold=150
	PhysicsTakeHitMomentumThreshold=250.0
	RadialDamageImpulse=750

	bAnimateHipsForDeathAnim=TRUE
	NodeDamageScaling=1.0
	//Texture'Envy_Effects.Energy.Materials.T_EFX_Energy_Loop_3'

	//MIC_MaterialWithDamageEffects=MaterialInstanceConstant'CH_Gore.SpecificWeaponGibs.M_CH_LinkGun'
}

