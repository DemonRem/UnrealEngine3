/*=============================================================================
	UnSceneCapture.h: render scenes to texture
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

// forward decls
class UTextureRenderTarget;
class UTextureRenderTarget2D;
class FSceneRenderer;

/** 
 * Probes added to the scene for rendering captures to a target texture
 * For use on the rendering thread
 */
class FSceneCaptureProbe
{
public:
	
	/** 
	* Constructor (default) 
	*/
	FSceneCaptureProbe(
		const AActor* InViewActor,
		UTextureRenderTarget* InTextureTarget,
		const EShowFlags& InShowFlags,
		const FLinearColor& InBackgroundColor,
		const FLOAT InFrameRate,
		const UPostProcessChain* InPostProcess,
		FSceneViewStateInterface* InViewState,
		const UBOOL bInSkipUpdateIfOwnerOccluded,
		const FLOAT InMaxUpdateDist
		)
		:	ViewActor(InViewActor)
		,	ShowFlags(InShowFlags)
		,	TextureTarget(InTextureTarget)
		,	BackgroundColor(InBackgroundColor)
		,	PostProcess(InPostProcess)
		,	LastCaptureTime(0)
		,	ViewState(InViewState)
		,	TimeBetweenCaptures(InFrameRate > 0 ? 1/InFrameRate : 0)
		,	bSkipUpdateIfOwnerOccluded(bInSkipUpdateIfOwnerOccluded)
		,	MaxUpdateDistSq(Square(InMaxUpdateDist))
	{
	}

	/** 
	* Destructor 
	*/
	virtual ~FSceneCaptureProbe()
	{
	}

	/**
	* Called by the rendering thread to render the scene for the capture
	* @param	MainSceneRenderer - parent scene renderer with info needed 
	*			by some of the capture types.
	*/
	virtual void CaptureScene( FSceneRenderer* MainSceneRenderer ) = 0;

	/** 
	* Determine if a capture is needed based on the given ViewFamily
	* @param ViewFamily - the main renderer's ViewFamily
	* @return TRUE if the capture needs to be updated
	*/
	UBOOL UpdateRequired(const FSceneViewFamily& ViewFamily) const;
	
protected:
	/** The actor which is being viewed from. */
	const AActor* ViewActor;
	/** show flags needed for a scene capture */
	EShowFlags ShowFlags;
	/** render target for scene capture */
	UTextureRenderTarget* TextureTarget;
	/** background scene color */
	FLinearColor BackgroundColor;
	/** Post process chain to be used by this capture */
	const UPostProcessChain* PostProcess;
	/** time in seconds when last captured */
	FLOAT LastCaptureTime;
	/** view state, for occlusion */
	FSceneViewStateInterface* ViewState;

private:

	/** time in seconds between each capture. if == 0 then scene is captured only once */
	FLOAT TimeBetweenCaptures;
	/** if true, skip updating the scene capture if the Owner of the component has not been rendered recently */
	UBOOL bSkipUpdateIfOwnerOccluded;
	/** if > 0, skip updating the scene capture if the Owner is further than this many units away from the viewer */
	FLOAT MaxUpdateDistSq;
};

/** 
 * Renders a scene to a 2D texture target
 * These can be added to a scene without a corresponding 
 * USceneCaptureComponent since they are not dependent. 
 */
class FSceneCaptureProbe2D : public FSceneCaptureProbe
{
public:

	/** 
	* Constructor (init) 
	*/
	FSceneCaptureProbe2D(
		const AActor* InViewActor,
		UTextureRenderTarget* InTextureTarget,
		const EShowFlags& InShowFlags,
		const FLinearColor& InBackgroundColor,
		const FLOAT InFrameRate,
		const UPostProcessChain* InPostProcess,
		FSceneViewStateInterface* InViewState,
		const UBOOL bInSkipUpdateIfOwnerOccluded,
		const FLOAT InMaxUpdateDist,
		const FMatrix& InViewMatrix,
		const FMatrix& InProjMatrix
		)
		:	FSceneCaptureProbe(InViewActor,InTextureTarget,InShowFlags,InBackgroundColor,InFrameRate,InPostProcess,InViewState,bInSkipUpdateIfOwnerOccluded,InMaxUpdateDist)
		,	ViewMatrix(InViewMatrix)
		,	ProjMatrix(InProjMatrix)
	{
	}

	/**
	* Called by the rendering thread to render the scene for the capture
	* @param	MainSceneRenderer - parent scene renderer with info needed 
	*			by some of the capture types.
	*/
	virtual void CaptureScene( FSceneRenderer* MainSceneRenderer );

private:
	/** view matrix for capture render */
	FMatrix ViewMatrix;
	/** projection matrix for capture render */
	FMatrix ProjMatrix;
};

/** 
* Renders a scene to a cube texture target
* These can be added to a scene without a corresponding 
* USceneCaptureComponent since they are not dependent. 
*/
class FSceneCaptureProbeCube : public FSceneCaptureProbe
{
public:

	/** 
	* Constructor (init) 
	*/
	FSceneCaptureProbeCube(
		const AActor* InViewActor,
		UTextureRenderTarget* InTextureTarget,
		const EShowFlags& InShowFlags,
		const FLinearColor& InBackgroundColor,
		const FLOAT InFrameRate,
		const UPostProcessChain* InPostProcess,
		FSceneViewStateInterface* InViewState,
		const UBOOL bInSkipUpdateIfOwnerOccluded,
		const FLOAT InMaxUpdateDist,
		const FVector& InLocation,
		FLOAT InNearPlane,
		FLOAT InFarPlane
		)
		:	FSceneCaptureProbe(InViewActor,InTextureTarget,InShowFlags,InBackgroundColor,InFrameRate,InPostProcess,InViewState,bInSkipUpdateIfOwnerOccluded,InMaxUpdateDist)
		,	WorldLocation(InLocation)
		,	NearPlane(InNearPlane)
		,	FarPlane(InFarPlane)
	{
	}

	/**
	* Called by the rendering thread to render the scene for the capture
	* @param	MainSceneRenderer - parent scene renderer with info needed 
	*			by some of the capture types.
	*/
	virtual void CaptureScene( FSceneRenderer* MainSceneRenderer );	

private:

	/**
	* Generates a view matrix for a cube face direction 
	* @param	Face - enum for the cube face to use as the facing direction
	* @return	view matrix for the cube face direction
	*/
	FMatrix CalcCubeFaceViewMatrix( ECubeFace Face );

	/** world position of the cube capture */
	FVector WorldLocation;
	/** for proj matrix calc */
	FLOAT NearPlane;
	/** far plane cull distance used for calculating proj matrix and thus the view frustum */
	FLOAT FarPlane;
};

/** 
* Renders a scene as a reflection to a 2d texture target
* These can be added to a scene without a corresponding 
* USceneCaptureComponent since they are not dependent. 
*/
class FSceneCaptureProbeReflect : public FSceneCaptureProbe
{
public:
	/** 
	* Constructor (init) 
	*/
	FSceneCaptureProbeReflect(
		const AActor* InViewActor,
		UTextureRenderTarget* InTextureTarget,
		const EShowFlags& InShowFlags,
		const FLinearColor& InBackgroundColor,
		const FLOAT InFrameRate,
		const UPostProcessChain* InPostProcess,
		FSceneViewStateInterface* InViewState,
		const UBOOL bInSkipUpdateIfOwnerOccluded,
		const FLOAT InMaxUpdateDist,
		const FPlane& InMirrorPlane
		)
		:	FSceneCaptureProbe(InViewActor,InTextureTarget,InShowFlags,InBackgroundColor,InFrameRate,InPostProcess,InViewState,bInSkipUpdateIfOwnerOccluded,InMaxUpdateDist)
		,	MirrorPlane(InMirrorPlane)
	{
	}

	/**
	* Called by the rendering thread to render the scene for the capture
	* @param	MainSceneRenderer - parent scene renderer with info needed 
	*			by some of the capture types.
	*/
	virtual void CaptureScene( FSceneRenderer* MainSceneRenderer );	

private:
	/** plane to reflect against */
	FPlane MirrorPlane;
};

/** 
* Renders a scene as if viewed through a sister portal to a 2d texture target
* These can be added to a scene without a corresponding 
* USceneCaptureComponent since they are not dependent. 
*/
class FSceneCaptureProbePortal : public FSceneCaptureProbe
{
public:

	/** 
	* Constructor (init) 
	*/
	FSceneCaptureProbePortal(
		const AActor* InViewActor,
		UTextureRenderTarget* InTextureTarget,
		const EShowFlags& InShowFlags,
		const FLinearColor& InBackgroundColor,
		const FLOAT InFrameRate,
		const UPostProcessChain* InPostProcess,
		FSceneViewStateInterface* InViewState,
		const UBOOL bInSkipUpdateIfOwnerOccluded,
		const FLOAT InMaxUpdateDist,
		const FMatrix& InSrcWorldToLocalM,
		const FMatrix& InDestLocalToWorldM,
		const FMatrix& InSrcToDestChangeBasisM,
        const AActor* InDestViewActor,
		const FPlane& InClipPlane
		)
		:	FSceneCaptureProbe(InViewActor,InTextureTarget,InShowFlags,InBackgroundColor,InFrameRate,InPostProcess,InViewState,bInSkipUpdateIfOwnerOccluded,InMaxUpdateDist)
		,	SrcWorldToLocalM(InSrcWorldToLocalM)
		,	DestLocalToWorldM(InDestLocalToWorldM)
		,	SrcToDestChangeBasisM(InSrcToDestChangeBasisM)
		,	DestViewActor(InDestViewActor ? InDestViewActor : InViewActor)
		,	ClipPlane(InClipPlane)
	{
	}

	/**
	* Called by the rendering thread to render the scene for the capture
	* @param	MainSceneRenderer - parent scene renderer with info needed 
	*			by some of the capture types.
	*/
	virtual void CaptureScene( FSceneRenderer* MainSceneRenderer );	

private:
	/** Inverse world transform for the source view location/orientation */
	FMatrix SrcWorldToLocalM;
	/** World transform for the the destination view location/orientation */
	FMatrix DestLocalToWorldM;
	/** Transform for source to destination view */
	FMatrix SrcToDestChangeBasisM;	
	/** the destination actor for this portal */
	const AActor* DestViewActor;
	/** plane aligned to destination view location to clip rendering */
	FPlane ClipPlane;
};


