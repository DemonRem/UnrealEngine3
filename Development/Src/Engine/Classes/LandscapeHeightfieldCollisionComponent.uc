/**
 * Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
 */
class LandscapeHeightfieldCollisionComponent extends PrimitiveComponent
	native(Terrain);

/** The collision height values. */
var native const UntypedBulkData_Mirror	CollisionHeightData{FWordBulkData};


/** Offset of component in landscape quads */
var const int SectionBaseX;
var const int SectionBaseY;

/** Size of component in collision quads */
var int CollisionSizeQuads;

/** Collision scale: (ComponentSizeQuads+1) / (CollisionSizeQuads+1) */
var float CollisionScale;

/** The flags for each collision quad. See ECollisionQuadFlags. */
var const array<byte> CollisionQuadFlags;

var const array<PhysicalMaterial> PhysicalMaterials;

/** Physics engine version of heightfield data. */
var const native pointer RBHeightfield{class NxHeightField};

/** Cached bounds, created at heightmap update time */
var const BoxSphereBounds CachedBoxSphereBounds;

cpptext
{
	enum ECollisionQuadFlags
	{
		QF_PhysicalMaterialMask = 63,	// Mask value for the physical material index, stored in the lower 6 bits.
		QF_EdgeTurned = 64,				// This quad's diagonal has been turned.
		QF_NoCollision = 128,			// This quad has no collision.
	};

	// UActorComponent interface.
	virtual void Attach();
	virtual void Detach(UBOOL bWillReattach);

	// UPrimitiveComponent interface.
	virtual void UpdateBounds();
	virtual void SetParentToWorld(const FMatrix& ParentToWorld);
	virtual void InitComponentRBPhys(UBOOL bFixed);
	virtual UBOOL PointCheck(FCheckResult& Result,const FVector& Location,const FVector& Extent,DWORD TraceFlags);
	virtual UBOOL LineCheck(FCheckResult& Result,const FVector& End,const FVector& Start,const FVector& Extent,DWORD TraceFlags);

	// UObject Interface
	virtual void Serialize(FArchive& Ar);
	virtual void BeginDestroy();
#if WITH_EDITOR
	virtual void ExportCustomProperties(FOutputDevice& Out, UINT Indent);
	virtual void ImportCustomProperties(const TCHAR* SourceText, FFeedbackContext* Warn);
	virtual void PostLoad();
	virtual void PostEditImport();

	// Register ourselves with the actor.
	void SetupActor();
#endif

	/** Return's the landscape actor associated with this component. */
	class ALandscape* GetLandscapeActor() const
	{
		return CastChecked<ALandscape>(GetOuter());
	}

	/** Recreate heightfield and restart physics */
	void RecreateHeightfield();
}

defaultproperties
{
	CollideActors=TRUE
	BlockActors=TRUE
	BlockZeroExtent=TRUE
	BlockNonZeroExtent=TRUE
	BlockRigidBody=TRUE
	CastShadow=TRUE
	bAcceptsLights=TRUE
	bAcceptsDecals=TRUE
	bAcceptsStaticDecals=TRUE
	bUsePrecomputedShadows=TRUE
	bUseAsOccluder=TRUE
	bAllowCullDistanceVolume=FALSE
}
