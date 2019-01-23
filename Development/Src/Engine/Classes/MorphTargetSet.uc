/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class MorphTargetSet extends Object
	native(Anim);
	
cpptext
{
	/** 
	 * Returns a one line description of an object for viewing in the thumbnail view of the generic browser
	 */
	virtual FString GetDesc();
}

/** Array of pointers to MorphTarget objects, containing vertex deformation information. */ 
var	array<MorphTarget>		Targets;

/** SkeletalMesh that this MorphTargetSet works on. */
var	SkeletalMesh			BaseSkelMesh;

/** Find a morph target by name in this MorphTargetSet. */ 
native final function MorphTarget FindMorphTarget( Name MorphTargetName );
