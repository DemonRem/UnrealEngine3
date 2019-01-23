/**
 * Class description
 *
 * Copyright 2007 Epic Games, Inc. All Rights Reserved
 */
class UIMeshWidget extends UIObject
	native(UIPrivate)
	placeable;

cpptext
{
	/* === UUIObject interface === */
	/**
	 * Updates 3D primitives for this widget.
	 *
	 * @param	CanvasScene		the scene to use for updating any 3D primitives
	 */
	virtual void UpdateWidgetPrimitives( FCanvasScene* CanvasScene );

	/* === UUIScreenObject interface === */
	/**
	 * Attach and initialize any 3D primitives for this widget and its children.
	 *
	 * @param	CanvasScene		the scene to use for attaching 3D primitives
	 */
	virtual void InitializePrimitives( class FCanvasScene* CanvasScene );
}

var()	const	editconst	StaticMeshComponent		Mesh;



DefaultProperties
{
	bSupports3DPrimitives=true
	bSupportsPrimaryStyle=false	// no style

	bDebugShowBounds=true
	DebugBoundsColor=(R=128,G=0,B=64)

	Begin Object Class=StaticMeshComponent Name=WidgetMesh
	End Object
	Mesh=WidgetMesh
}
