/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
// This class of blend node will ramp the 'active' child up to 1.0

class AnimNodeBlendList extends AnimNodeBlendBase
	native(Anim)
	hidecategories(Object);

/** Array of target weights for each child. Size must be the same as the Children array. */
var		array<float>		TargetWeight;

/** How long before current blend is complete (ie. active child reaches 100%) */
var		float				BlendTimeToGo;

/** Child currently active - that is, at or ramping up to 100%. */
var		INT					ActiveChildIndex;

/** Call play anim when active child is changed */
var() bool	bPlayActiveChild;

cpptext
{
	// AnimNode interface
	virtual void InitAnim( USkeletalMeshComponent* meshComp, UAnimNodeBlendBase* Parent );
	virtual	void TickAnim( FLOAT DeltaSeconds, FLOAT TotalWeight );

	// AnimNodeBlendBase interface
	virtual void OnAddChild(INT ChildNum);
	virtual void OnRemoveChild(INT ChildNum);

	// AnimNodeBlendList interface
}

native function SetActiveChild( INT ChildIndex, FLOAT BlendTime );

defaultproperties
{
	Children(0)=(Name="Child1")
	bFixNumChildren=FALSE
}
