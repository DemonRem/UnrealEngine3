/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class AnimNodeMirror extends AnimNodeBlendBase
	native(Anim)
	hidecategories(Object);

var()	bool	bEnableMirroring;
	
cpptext
{
	virtual void GetBoneAtoms(TArray<FBoneAtom>& Atoms, const TArray<BYTE>& DesiredBones, FBoneAtom& RootMotionDelta, INT& bHasRootMotion);
}
	
defaultproperties
{
	Children(0)=(Name="Child",Weight=1.0)
	bFixNumChildren=true
	
	bEnableMirroring=true
}
