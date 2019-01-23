/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/
 
class SpeedTree extends Object
	native(SpeedTree);
	
/** Helper object to allow SpeedTree to be script exposed.										*/
var duplicatetransient native const pointer	SRH{class FSpeedTreeResourceHelper};

// Editor-accesible variables

/** Random seed for tree creation.																*/
var() int							RandomSeed;						
/** Sink the tree partially underground.														*/
var() float							Sink;

/** The probability of a shadow ray being blocked by the leaf material. */
var(Lighting) float LeafStaticShadowOpacity;

/** Branch material.																			*/
var(Material) MaterialInterface		BranchMaterial;
/** Frond material.																				*/
var(Material) MaterialInterface		FrondMaterial;
/** Leaf material.																				*/
var(Material) MaterialInterface		LeafMaterial;
/** Billboard material.																			*/
var(Material) MaterialInterface		BillboardMaterial;

// SpeedWind variables (explained in the SpeedTreeCAD documentation)
var(Wind) float						MaxBendAngle;
var(Wind) float						BranchExponent;
var(Wind) float						LeafExponent;
var(Wind) float						Response;
var(Wind) float						ResponseLimiter;
var(Wind) float						Gusting_Strength;
var(Wind) float						Gusting_Frequency;
var(Wind) float						Gusting_Duration;
var(Wind) float						BranchHorizontal_LowWindAngle;
var(Wind) float						BranchHorizontal_LowWindSpeed;
var(Wind) float						BranchHorizontal_HighWindAngle;
var(Wind) float						BranchHorizontal_HighWindSpeed;
var(Wind) float						BranchVertical_LowWindAngle;
var(Wind) float						BranchVertical_LowWindSpeed;
var(Wind) float						BranchVertical_HighWindAngle;
var(Wind) float						BranchVertical_HighWindSpeed;
var(Wind) float						LeafRocking_LowWindAngle;
var(Wind) float						LeafRocking_LowWindSpeed;
var(Wind) float						LeafRocking_HighWindAngle;
var(Wind) float						LeafRocking_HighWindSpeed;
var(Wind) float						LeafRustling_LowWindAngle;
var(Wind) float						LeafRustling_LowWindSpeed;
var(Wind) float						LeafRustling_HighWindAngle;
var(Wind) float						LeafRustling_HighWindSpeed;

cpptext
{
	void StaticConstructor(void);

	virtual void BeginDestroy();
	virtual UBOOL IsReadyForFinishDestroy();
	virtual void FinishDestroy();
	
	virtual void PreEditChange(UProperty* PropertyAboutToChange);
	virtual void PostEditChange(UProperty* PropertyThatChanged);
	virtual void Serialize(FArchive& Ar);
	virtual void PostLoad();

	virtual	INT	GetResourceSize(void);
	virtual	FString	GetDetailedDescription( INT InIndex );
	virtual FString	GetDesc(void);
	
	UBOOL IsInitialized();
}

defaultproperties
{
	RandomSeed=1
	
	LeafStaticShadowOpacity=0.5

	// SpeedWind values
	MaxBendAngle=60.0
	BranchExponent=3.0
	LeafExponent=1.0
	Response=0.7
	ResponseLimiter=0.2
	Gusting_Strength=0.3
	Gusting_Frequency=0.4
	Gusting_Duration=5.0
	BranchHorizontal_LowWindAngle=5.0
	BranchHorizontal_LowWindSpeed=0.5
	BranchHorizontal_HighWindAngle=2.0
	BranchHorizontal_HighWindSpeed=7.0
	BranchVertical_LowWindAngle=3.0
	BranchVertical_LowWindSpeed=0.8
	BranchVertical_HighWindAngle=5.0
	BranchVertical_HighWindSpeed=8.0
	LeafRocking_LowWindAngle=7.0
	LeafRocking_LowWindSpeed=0.3
	LeafRocking_HighWindAngle=5.0
	LeafRocking_HighWindSpeed=0.75
	LeafRustling_LowWindAngle=7.0
	LeafRustling_LowWindSpeed=0.1
	LeafRustling_HighWindAngle=5.0
	LeafRustling_HighWindSpeed=15.0
}

