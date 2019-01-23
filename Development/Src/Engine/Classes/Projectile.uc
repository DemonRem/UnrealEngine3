//=============================================================================
// Projectile.
//
// A delayed-hit projectile that moves around for some time after it is created.
// Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
//=============================================================================
class Projectile extends Actor
	abstract
	native;

//-----------------------------------------------------------------------------
// Projectile variables.

// Motion information.
var		float   Speed;					// Initial speed of projectile.
var		float   MaxSpeed;				// Limit on speed of projectile (0 means no limit)
var		bool	bSwitchToZeroCollision; // if collisionextent nonzero, and hit actor with bBlockNonZeroExtents=0, switch to zero extent collision
var		Actor	ZeroCollider;

/** If false, instigator does not collide with this projectile */
var bool bBlockedByInstigator;

var		bool	bBegunPlay;

// Damage attributes.
var   float    Damage;
var	  float	   DamageRadius;
var   float	   MomentumTransfer; // Momentum magnitude imparted by impacting projectile.
var   class<DamageType>	   MyDamageType;

// Projectile sound effects
var   SoundCue	SpawnSound;		// Sound made when projectile is spawned.
var   SoundCue ImpactSound;		// Sound made when projectile hits something.

// explosion effects
var Controller	InstigatorController;
var Actor ImpactedActor;		// Actor hit or touched by this projectile.  Gets full damage, even if radius effect projectile, and then ignored in HurtRadius

// network culling
var float NetCullDistanceSquared;	// Projectile not relevant to client if further from viewer than this

var	CylinderComponent		CylinderComponent;

/** If true, this projectile will have its rotation updated each frame to match its velocity */
var bool bRotationFollowsVelocity;

cpptext
{
	void BoundProjectileVelocity();
	virtual UBOOL ShrinkCollision(AActor *HitActor, const FVector &StartLocation);
	virtual void GrowCollision();
	virtual AProjectile* GetAProjectile() { return this; }
	virtual const AProjectile* GetAProjectile() const { return this; }
	virtual void processHitWall(FVector HitNormal, AActor *HitActor, UPrimitiveComponent* HitComp);
	virtual UBOOL IsNetRelevantFor(APlayerController* RealViewer, AActor* Viewer, const FVector& SrcLocation);
	virtual FLOAT GetNetPriority(const FVector& ViewPos, const FVector& ViewDir, APlayerController* Viewer, UActorChannel* InChannel, FLOAT Time, UBOOL bLowBandwidth);
	virtual UBOOL IgnoreBlockingBy( const AActor *Other) const;
	virtual void TickSpecial( FLOAT DeltaSeconds );
}

//==============
// Encroachment
event bool EncroachingOn( actor Other )
{
	if ( Brush(Other) != None )
		return true;

	return false;
}

simulated event PostBeginPlay()
{
    if ( Role == ROLE_Authority && Instigator != None )
    {
		InstigatorController = Instigator.Controller;
    	if ( InstigatorController != None && InstigatorController.ShotTarget != None && InstigatorController.ShotTarget.Controller != None )
			InstigatorController.ShotTarget.Controller.ReceiveProjectileWarning( Self );
    }
	bBegunPlay = true;
}

/* Init()
initialize velocity and rotation of projectile
*/
function Init(vector Direction)
{
	SetRotation(Rotator(Direction));
	Velocity = Speed * Direction;
}

simulated function bool CanSplash()
{
	return bBegunPlay;
}

function Reset()
{
	Destroy();
}

/**
 * HurtRadius()
 * Hurt locally authoritative actors within the radius.
 */
simulated function bool HurtRadius( float DamageAmount,
								    float InDamageRadius,
				    class<DamageType> DamageType,
									float Momentum,
									vector HurtOrigin,
									optional actor IgnoredActor,
									optional Controller InstigatedByController = Instigator != None ? Instigator.Controller : None,
									optional bool bDoFullDamage
									)
{
	local bool bCausedDamage, bResult;

	if ( bHurtEntry )
		return false;

	bCausedDamage = false;
	if (InstigatedByController == None)
	{
		InstigatedByController = InstigatorController;
	}

	// if ImpactedActor is set, we actually want to give it full damage, and then let him be ignored by super.HurtRadius()
	if ( (ImpactedActor != None) && (ImpactedActor != self)  )
	{
		ImpactedActor.TakeRadiusDamage(InstigatedByController, DamageAmount, InDamageRadius, DamageType, Momentum, HurtOrigin, true, self);
		bCausedDamage = ImpactedActor.bProjTarget;
	}

	bResult = Super.HurtRadius(DamageAmount, InDamageRadius, DamageType, Momentum, HurtOrigin, ImpactedActor, InstigatedByController, bDoFullDamage);
	return ( bResult || bCausedDamage );
}

//==============
// Touching
simulated singular event Touch( Actor Other, PrimitiveComponent OtherComp, vector HitLocation, vector HitNormal )
{
	if ( (Other == None) || Other.bDeleteMe ) // Other just got destroyed in its touch?
		return;

	// don't allow projectiles to explode while spawning on clients
	// because if that were accurate, the projectile would've been destroyed immediately on the server
	// and therefore it wouldn't have been replicated to the client
	if ( Other.StopsProjectile(self) && (Role == ROLE_Authority || bBegunPlay) && (bBlockedByInstigator || (Other != Instigator)) )
	{
		ImpactedActor = Other;
		ProcessTouch(Other, HitLocation, HitNormal);
		ImpactedActor = None;
	}
}

simulated function ProcessTouch(Actor Other, Vector HitLocation, Vector HitNormal)
{
	if ( Other != Instigator )
		Explode( HitLocation, HitNormal );
}

simulated singular event HitWall(vector HitNormal, actor Wall, PrimitiveComponent WallComp)
{
	ImpactedActor = Wall;

	if ( !Wall.bStatic && !Wall.bWorldGeometry )
	{
		if ( DamageRadius == 0 )
		{
			Wall.TakeDamage( Damage, InstigatorController, Location, MomentumTransfer * Normal(Velocity), MyDamageType,, self);
		}
	}

	if ( Role == ROLE_Authority )
	{
		MakeNoise(1.0);
	}

	Explode(Location, HitNormal);
	ImpactedActor = None;
}

simulated event EncroachedBy(Actor Other)
{
	HitWall(Normal(Location - Other.Location), Other, None);
}

simulated function Explode(vector HitLocation, vector HitNormal)
{
	if (Damage > 0 && DamageRadius > 0)
	{
		HurtRadius(Damage, DamageRadius, MyDamageType, MomentumTransfer, HitLocation);
	}
	Destroy();
}

simulated final function RandSpin(float spinRate)
{
	DesiredRotation = RotRand();
	RotationRate.Yaw = spinRate * 2 *FRand() - spinRate;
	RotationRate.Pitch = spinRate * 2 *FRand() - spinRate;
	RotationRate.Roll = spinRate * 2 *FRand() - spinRate;
}

function bool IsStationary()
{
	return false;
}

simulated event FellOutOfWorld(class<DamageType> dmgType)
{
	Explode(Location, vect(0,0,1));
}

/** returns the amount of time it would take the projectile to get to the specified location */
simulated function float GetTimeToLocation(vector TargetLoc)
{
	return (VSize(TargetLoc - Location) / Speed);
}

/** returns the maximum distance this projectile can travel */
simulated static function float GetRange()
{
	if (default.LifeSpan == 0.0)
	{
		return 15000.0;
	}
	else
	{
		return (default.MaxSpeed * default.LifeSpan);
	}
}

defaultproperties
{
	TickGroup=TG_PreAsyncWork

	Begin Object Class=SpriteComponent Name=Sprite
		Sprite=Texture2D'EngineResources.S_Actor'
		HiddenGame=True
		AlwaysLoadOnClient=False
		AlwaysLoadOnServer=False
	End Object
	Components.Add(Sprite)

	Begin Object Class=CylinderComponent Name=CollisionCylinder
		CollisionRadius=0
		CollisionHeight=0
		AlwaysLoadOnClient=True
		AlwaysLoadOnServer=True
	End Object
	CollisionComponent=CollisionCylinder
	CylinderComponent=CollisionCylinder
	Components.Add(CollisionCylinder)

	bCanBeDamaged=true
	DamageRadius=+220.0
	MaxSpeed=+02000.000000
	Speed=+02000.000000
	bRotationFollowsVelocity=false
	bCollideActors=true
	bCollideWorld=true
	bNetTemporary=true
	bGameRelevant=true
	bReplicateInstigator=true
	Physics=PHYS_Projectile
	LifeSpan=+0014.000000
	NetPriority=+00002.500000
	MyDamageType=class'DamageType'
	RemoteRole=ROLE_SimulatedProxy
	NetCullDistanceSquared=+400000000.0
	bBlockedByInstigator=true
}
