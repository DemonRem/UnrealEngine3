/**
 * SceneCapture2DComponent
 *
 * Allows a scene capture to a 2D texture render target
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */
class SceneCapture2DComponent extends SceneCaptureComponent
	native;

/** render target resource to set as target for capture */
var(Capture) TextureRenderTarget2D TextureTarget;
/** horizontal field of view */
var(Capture) float FieldOfView;
/** near plane clip distance */
var(Capture) float NearPlane;
/** far plane clip distance */ 
var(Capture) float FarPlane;
/** set to false to disable automatic updates of the view/proj matrices */
var	bool bUpdateMatrices;

// transients
/** view matrix used for rendering */
var transient matrix ViewMatrix;
/** projection matrix used for rendering */
var transient matrix ProjMatrix;

cpptext 
{
protected:
	
	// UActorComponent interface

	/**
	* Attach a new 2d capture component
	*/
	virtual void Attach();

	/**
	 * Sets the ParentToWorld transform the component is attached to.
	 * @param ParentToWorld - The ParentToWorld transform the component is attached to.
	 */
	virtual void SetParentToWorld(const FMatrix& ParentToWorld);

public:

	/**
	* Constructor
	*/
	USceneCapture2DComponent() :
		ViewMatrix(FMatrix::Identity),
		ProjMatrix(FMatrix::Identity)
		{}

	/** 
	* Update the projection matrix using the fov,near,far,aspect
	*/
	void UpdateProjMatrix();

	// SceneCaptureComponent interface

	/**
	* Create a new probe with info needed to render the scene
	*/
	virtual class FSceneCaptureProbe* CreateSceneCaptureProbe();
}

defaultproperties
{
	NearPlane=20
	FarPlane=500
	FieldOfView=80	
	bUpdateMatrices=true
}
