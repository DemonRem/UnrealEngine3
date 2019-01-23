/**
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class ASVSkelComponent extends SkeletalMeshComponent
	native;

var transient native const pointer	AnimSetViewerPtr;

/** If TRUE, render a wireframe skeleton of the mesh animated with the raw (uncompressed) animation data. */
var bool		bRenderRawSkeleton;

cpptext
{
	// UPrimitiveComponent interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy();
}
