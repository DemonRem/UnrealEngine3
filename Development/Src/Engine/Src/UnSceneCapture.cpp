/*=============================================================================
	UnSceneCapture.cpp: render scenes to texture
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineMaterialClasses.h"
#include "ScenePrivate.h"

IMPLEMENT_CLASS(USceneCaptureComponent);
IMPLEMENT_CLASS(USceneCapture2DComponent);
IMPLEMENT_CLASS(USceneCaptureCubeMapComponent);
IMPLEMENT_CLASS(USceneCaptureParaboloidComponent);
IMPLEMENT_CLASS(USceneCaptureReflectComponent);
IMPLEMENT_CLASS(USceneCapturePortalComponent);
IMPLEMENT_CLASS(ASceneCaptureActor);
IMPLEMENT_CLASS(ASceneCapture2DActor);
IMPLEMENT_CLASS(ASceneCaptureCubeMapActor);
IMPLEMENT_CLASS(ASceneCaptureReflectActor);
IMPLEMENT_CLASS(ASceneCapturePortalActor);
IMPLEMENT_CLASS(APortalTeleporter);

// arbitrary min/max
static const FLOAT MAX_FAR_PLANE = FLT_MAX;
static const FLOAT MIN_NEAR_PLANE = 1.f;

/*-----------------------------------------------------------------------------
USceneCaptureComponent
-----------------------------------------------------------------------------*/

/** 
* Constructor 
*/
USceneCaptureComponent::USceneCaptureComponent()
:	bNeedsSceneUpdate(TRUE)
{
}

/**
 * Adds a capture proxy for this component to the scene
 */
void USceneCaptureComponent::Attach()
{
    Super::Attach();
	bNeedsSceneUpdate = TRUE;
}

/**
* Removes a capture proxy for thsi component from the scene
*/
void USceneCaptureComponent::Detach()
{
	if( Scene )
	{
		// remove this capture component from the scene
		Scene->RemoveSceneCapture(this);
	}

	Super::Detach();
}

/**
 * Tick the component to handle updates
 */
void USceneCaptureComponent::Tick( FLOAT DeltaTime )
{
	Super::Tick( DeltaTime );

	if( Scene &&
		bNeedsSceneUpdate &&
		GSceneRenderTargets.IsInitialized() &&
		GSceneRenderTargets.GetBufferSizeX() > 0 &&
		GSceneRenderTargets.GetBufferSizeY() > 0 )
	{
		// add this capture component to the scene
		Scene->RemoveSceneCapture(this);
		Scene->AddSceneCapture(this);

		bNeedsSceneUpdate = FALSE;
	}
}

/**
* Sets the ParentToWorld transform the component is attached to.
* @param ParentToWorld - The ParentToWorld transform the component is attached to.
*/
void USceneCaptureComponent::SetParentToWorld(const FMatrix& ParentToWorld)
{
	Super::SetParentToWorld( ParentToWorld );

	// readd the probe with updated matrix data
	if( Scene )
	{
		Scene->RemoveSceneCapture(this);
		Scene->AddSceneCapture(this);
	}
}

/**
* Map the various capture view settings to show flags.
*/
EShowFlags USceneCaptureComponent::GetSceneShowFlags()
{
	// start with default settings
	EShowFlags Result = SHOW_DefaultGame & (~SHOW_SceneCaptureUpdates);

	// lighting modes
	switch( ViewMode )
	{
	case SceneCapView_Unlit:
		Result = (Result&~SHOW_ViewMode_Mask)|SHOW_ViewMode_Unlit;
		break;
	case SceneCapView_Wire:
		Result = (Result&~SHOW_ViewMode_Mask)|SHOW_ViewMode_Wireframe;
		break;
	case SceneCapView_Lit:
		Result = (Result&~SHOW_ViewMode_Mask)|SHOW_ViewMode_Lit;
		break;
	case SceneCapView_LitNoShadows:
		Result &= ~SHOW_DynamicShadows;
		Result = (Result&~SHOW_ViewMode_Mask)|SHOW_ViewMode_Lit;
		break;
	default:		
		break;

	}
	// toggle fog
	if( !bEnableFog ) 
	{
		Result &= ~SHOW_Fog;
	}
	// toggle post-processing
	if( !bEnablePostProcess ) 
	{
		Result &= ~SHOW_PostProcess;
	}
	return Result;
}

void USceneCaptureComponent::FinishDestroy()
{
	if (ViewState != NULL)
	{
		ViewState->Destroy();
		ViewState = NULL;
	}

	Super::FinishDestroy();
}

void USceneCaptureComponent::SetFrameRate(FLOAT NewFrameRate)
{
	FrameRate = NewFrameRate;
	bNeedsSceneUpdate = TRUE;
}

/*-----------------------------------------------------------------------------
FSceneCaptureProbe
-----------------------------------------------------------------------------*/

/** 
* Determine if a capture is needed based on TimeBetweenCaptures and LastCaptureTime
* @param CurrentTime - seconds since start of play in the world
* @return TRUE if the capture needs to be updated
*/
UBOOL FSceneCaptureProbe::UpdateRequired(const FSceneViewFamily& ViewFamily) const
{
	checkSlow(ViewFamily.CurrentWorldTime >= LastCaptureTime);
	if (ViewActor != NULL)
	{
		if (bSkipUpdateIfOwnerOccluded && ViewFamily.CurrentWorldTime - ViewActor->LastRenderTime > 1.0f)
		{
			return FALSE;
		}
		else if (MaxUpdateDistSq > 0.f)
		{
			UBOOL bInRange = FALSE;
			for (INT i = 0; i < ViewFamily.Views.Num(); i++)
			{
				if ((ViewActor->Location - ViewFamily.Views(i)->ViewOrigin).SizeSquared() <= MaxUpdateDistSq)
				{
					bInRange = TRUE;
					break;
				}
			}
			if (!bInRange)
			{
				return FALSE;
			}
		}
	}

    return (	(TimeBetweenCaptures == 0 && LastCaptureTime == 0) ||
				(TimeBetweenCaptures > 0 && (ViewFamily.CurrentWorldTime - LastCaptureTime) >= TimeBetweenCaptures)		);
}

/*-----------------------------------------------------------------------------
USceneCapture2DComponent
-----------------------------------------------------------------------------*/

/**
 * Attach a new 2d capture component
 */
void USceneCapture2DComponent::Attach()
{
	// clamp near/far clip distances
	NearPlane = Max( NearPlane, MIN_NEAR_PLANE );
	FarPlane = Clamp<FLOAT>( FarPlane, NearPlane, MAX_FAR_PLANE );
	// clamp fov
	FieldOfView = Clamp<FLOAT>( FieldOfView, 1.f, 179.f );	

    Super::Attach();
}

/**
* Sets the ParentToWorld transform the component is attached to.
* @param ParentToWorld - The ParentToWorld transform the component is attached to.
*/
void USceneCapture2DComponent::SetParentToWorld(const FMatrix& ParentToWorld)
{
	// only affects the view matrix
	if( bUpdateMatrices )
	{
		// set view to location/orientation of parent
		ViewMatrix = ParentToWorld.Inverse();
		// swap axis st. x=z,y=x,z=y (unreal coord space) so that z is up
		ViewMatrix = ViewMatrix * FMatrix(
				FPlane(0,	0,	1,	0),
				FPlane(1,	0,	0,	0),
				FPlane(0,	1,	0,	0),
				FPlane(0,	0,	0,	1)); 
	}

	Super::SetParentToWorld( ParentToWorld );
}

/** 
 * Update the projection matrix using the fov,near,far,aspect
 */
void USceneCapture2DComponent::UpdateProjMatrix()
{
	// matrix updates can be skipped  
	if( bUpdateMatrices	)
	{
		// projection matrix based on the fov,near/far clip settings
		ProjMatrix = FPerspectiveMatrix(
			FieldOfView * (FLOAT)PI / 360.0f,
			TextureTarget ? (FLOAT)TextureTarget->GetSurfaceWidth() : (FLOAT)GSceneRenderTargets.GetBufferSizeX(),
			TextureTarget ? (FLOAT)TextureTarget->GetSurfaceHeight() : (FLOAT)GSceneRenderTargets.GetBufferSizeY(),
			NearPlane,
			FarPlane
			);
	}
}

/**
* Create a new probe with info needed to render the scene
*/
FSceneCaptureProbe* USceneCapture2DComponent::CreateSceneCaptureProbe() 
{	
	// make sure projection matrix is updated before adding to scene
	UpdateProjMatrix();

	// allocate view state, if necessary
	if (ViewState == NULL)
	{
		ViewState = AllocateViewState();
	}

	// any info from the component needed for capturing the scene should be copied to the probe
	return new FSceneCaptureProbe2D(
		Owner,
		TextureTarget,
		GetSceneShowFlags(),
		FLinearColor(ClearColor),
		FrameRate,
		PostProcess,
		ViewState,
		bSkipUpdateIfOwnerOccluded,
		MaxUpdateDist,
		ViewMatrix,
		ProjMatrix); 
}

/*-----------------------------------------------------------------------------
FSceneCaptureProbe2D
-----------------------------------------------------------------------------*/

/**
* Called by the rendering thread to render the scene for the capture
* @param	MainSceneRenderer - parent scene renderer with info needed 
*			by some of the capture types.
*/
void FSceneCaptureProbe2D::CaptureScene( FSceneRenderer* MainSceneRenderer )
{
	check(MainSceneRenderer);

	// render target resource to render with
	FTextureRenderTargetResource* RTResource = TextureTarget ? TextureTarget->GetRenderTargetResource() : NULL;
	if( RTResource &&
		MainSceneRenderer->ViewFamily.Views.Num() &&
		UpdateRequired(MainSceneRenderer->ViewFamily) )
	{	
		LastCaptureTime = MainSceneRenderer->ViewFamily.CurrentWorldTime;

		// should have a 2d render target resource
		check(RTResource->GetTextureRenderTarget2DResource());
		// create a temp view family for rendering the scene. use the same scene as parent
		FSceneViewFamilyContext ViewFamily(
 			RTResource,
 			(FSceneInterface*)MainSceneRenderer->Scene,
 			ShowFlags,
 			MainSceneRenderer->ViewFamily.CurrentWorldTime,
			MainSceneRenderer->ViewFamily.CurrentRealTime,
			NULL
 			);
		// create a view for the capture
		FSceneView* View = new FSceneView(
			&ViewFamily,
			ViewState,
			NULL,
			ViewActor,
			PostProcess,
			NULL,
			NULL,
			0,
			0,
			RTResource->GetSizeX(),
			RTResource->GetSizeY(),
			ViewMatrix,
			ProjMatrix,
			BackgroundColor,
			FLinearColor(0.f,0.f,0.f,0.f),
			FLinearColor::White,
			TArray<FPrimitiveSceneInfo*>()
			);
		// add the view to the family
		ViewFamily.Views.AddItem(View);
		// create a new scene renderer for rendering the capture
		FSceneRenderer* CaptureSceneRenderer = ::new FSceneRenderer(
			&ViewFamily,
			NULL,
			MainSceneRenderer->CanvasTransform
			);
		// set the rendering context for the capture renderer
		CaptureSceneRenderer->GlobalContext = MainSceneRenderer->GlobalContext;
		// render the scene to the target
		CaptureSceneRenderer->Render();
		// copy the results of the scene rendering from the target surface to its texture
		RHICopyToResolveTarget(RTResource->GetRenderTargetSurface(), FALSE);

		delete CaptureSceneRenderer;
	}
}

/*-----------------------------------------------------------------------------
USceneCaptureCubeMapComponent
-----------------------------------------------------------------------------*/

/**
* Attach a new cube capture component
*/
void USceneCaptureCubeMapComponent::Attach()
{
    // clamp near/far cliip distances
    NearPlane = Max( NearPlane, MIN_NEAR_PLANE );
	FarPlane = Clamp<FLOAT>( FarPlane, NearPlane, MAX_FAR_PLANE );

	Super::Attach();
}

/**
* Sets the ParentToWorld transform the component is attached to.
* @param ParentToWorld - The ParentToWorld transform the component is attached to.
*/
void USceneCaptureCubeMapComponent::SetParentToWorld(const FMatrix& ParentToWorld)
{
	// world location of component 
	WorldLocation = ParentToWorld.GetOrigin();

	Super::SetParentToWorld( ParentToWorld );
}

/**
* Create a new probe with info needed to render the scene
*/
FSceneCaptureProbe* USceneCaptureCubeMapComponent::CreateSceneCaptureProbe() 
{
	// allocate view state, if necessary
	if (ViewState == NULL)
	{
		ViewState = AllocateViewState();
	}

	// any info from the component needed for capturing the scene should be copied to the probe
	return new FSceneCaptureProbeCube(
		Owner,
		TextureTarget,
		GetSceneShowFlags(),
		FLinearColor(ClearColor),
		FrameRate,
		PostProcess,
		ViewState,
		bSkipUpdateIfOwnerOccluded,
		MaxUpdateDist,
		WorldLocation,
		NearPlane,
		FarPlane
		);
}

/*-----------------------------------------------------------------------------
FSceneCaptureProbeCube
-----------------------------------------------------------------------------*/

/**
* Called by the rendering thread to render the scene for the capture
* @param	MainSceneRenderer - parent scene renderer with info needed 
*			by some of the capture types.
*/
void FSceneCaptureProbeCube::CaptureScene( FSceneRenderer* MainSceneRenderer )
{
	check(MainSceneRenderer);

	// render target resource to render with
	FTextureRenderTargetResource* RTResource = TextureTarget ? TextureTarget->GetRenderTargetResource() : NULL;
	if( RTResource &&
		MainSceneRenderer->ViewFamily.Views.Num() &&
		UpdateRequired(MainSceneRenderer->ViewFamily) )
	{
		LastCaptureTime = MainSceneRenderer->ViewFamily.CurrentWorldTime;

		// projection matrix based on the fov,near/far clip settings
		// each face always uses a 90 degree field of view
		FPerspectiveMatrix ProjMatrix(
			90.f * (FLOAT)PI / 360.0f,
			(FLOAT)RTResource->GetSizeX(),
			(FLOAT)RTResource->GetSizeY(),
			NearPlane,
			FarPlane
			);

		// should have a cube render target resource
		FTextureRenderTargetCubeResource* RTCubeResource = RTResource->GetTextureRenderTargetCubeResource();		
		check(RTCubeResource);
        // render scene once for each cube face
		for( INT FaceIdx=CubeFace_PosX; FaceIdx < CubeFace_MAX; FaceIdx++ )
		{
			// set current target face on cube render target
			RTCubeResource->SetCurrentTargetFace((ECubeFace)FaceIdx);
			// create a temp view family for rendering the scene. use the same scene as parent
			FSceneViewFamilyContext ViewFamily(
				RTCubeResource,
				(FSceneInterface*)MainSceneRenderer->Scene,
				ShowFlags,
				MainSceneRenderer->ViewFamily.CurrentWorldTime,
				MainSceneRenderer->ViewFamily.CurrentRealTime,
				NULL
				);			
			// create a view for the capture
			FSceneView* View = new FSceneView(
				&ViewFamily,
				ViewState,
				NULL,
				ViewActor,
				PostProcess,
				NULL,
				NULL,
				0,
				0,
				RTResource->GetSizeX(),
				RTResource->GetSizeY(),
				CalcCubeFaceViewMatrix((ECubeFace)FaceIdx),
				ProjMatrix,
				BackgroundColor,
				FLinearColor(0.f,0.f,0.f,0.f),
				FLinearColor::White,
				TArray<FPrimitiveSceneInfo*>()
				);
			// add the view to the family
			ViewFamily.Views.AddItem(View);
			// create a new scene renderer for rendering the capture
			FSceneRenderer* CaptureSceneRenderer = ::new FSceneRenderer(
				&ViewFamily,
				NULL,
				MainSceneRenderer->CanvasTransform
				);
			// set the rendering context for the capture renderer
			CaptureSceneRenderer->GlobalContext = MainSceneRenderer->GlobalContext;
			// render the scene to the target
			CaptureSceneRenderer->Render();			
			// copy the results of the scene rendering from the target surface to its texture
			FResolveParams ResolveParams;
			ResolveParams.CubeFace = (ECubeFace)FaceIdx;
			RHICopyToResolveTarget(RTResource->GetRenderTargetSurface(), FALSE,ResolveParams);

			delete CaptureSceneRenderer;
		}
	}
}

/**
* Generates a view matrix for a cube face direction 
* @param	Face - enum for the cube face to use as the facing direction
* @return	view matrix for the cube face direction
*/
FMatrix FSceneCaptureProbeCube::CalcCubeFaceViewMatrix( ECubeFace Face )
{
	FMatrix Result(FMatrix::Identity);

	static const FVector XAxis(1.f,0.f,0.f);
	static const FVector YAxis(0.f,1.f,0.f);
	static const FVector ZAxis(0.f,0.f,1.f);

	// vectors we will need for our basis
	FVector vUp(YAxis);
	FVector vDir;

	switch( Face )
	{
	case CubeFace_PosX:
		//vUp = YAxis;
		vDir = XAxis;
		break;
	case CubeFace_NegX:
		//vUp = YAxis;
		vDir = -XAxis;
		break;
	case CubeFace_PosY:
		vUp = -ZAxis;
		vDir = YAxis;
		break;
	case CubeFace_NegY:
		vUp = ZAxis;
		vDir = -YAxis;
		break;
	case CubeFace_PosZ:
		//vUp = YAxis;
		vDir = ZAxis;
		break;
	case CubeFace_NegZ:
		//vUp = YAxis;
		vDir = -ZAxis;
		break;
	}

	// derive right vector
	FVector vRight( vUp ^ vDir );
	// create matrix from the 3 axes
	Result = FBasisVectorMatrix( vRight, vUp, vDir, -WorldLocation );	

	return Result;
}

/*-----------------------------------------------------------------------------
USceneCaptureReflectComponent
-----------------------------------------------------------------------------*/

/**
* Attach a new reflect capture component
*/
void USceneCaptureReflectComponent::Attach()
{
	Super::Attach();
}

/**
* Create a new probe with info needed to render the scene
*/
FSceneCaptureProbe* USceneCaptureReflectComponent::CreateSceneCaptureProbe() 
{ 
	// use the actor's view direction as the reflecting normal
	FVector ViewActorDir( Owner ? Owner->Rotation.Vector() : FVector(0,0,1) );
	ViewActorDir.Normalize();
	FVector VieaActorPos( Owner ? Owner->Location : FVector(0,0,0) );

	// create the mirror plane	
	FPlane MirrorPlane = FPlane( ViewActorDir, -1.f * (VieaActorPos | ViewActorDir) );
	
	// allocate view state, if necessary
	if (ViewState == NULL)
	{
		ViewState = AllocateViewState();
	}

	// any info from the component needed for capturing the scene should be copied to the probe
	return new FSceneCaptureProbeReflect(
		Owner,
		TextureTarget,
		GetSceneShowFlags(),
		FLinearColor(ClearColor),
		FrameRate,
		PostProcess,
		ViewState,
		bSkipUpdateIfOwnerOccluded,
		MaxUpdateDist,
		MirrorPlane
		);
}

/*-----------------------------------------------------------------------------
FSceneCaptureProbeReflect
-----------------------------------------------------------------------------*/

/**
* Called by the rendering thread to render the scene for the capture
* @param	MainSceneRenderer - parent scene renderer with info needed 
*			by some of the capture types.
*/
void FSceneCaptureProbeReflect::CaptureScene( FSceneRenderer* MainSceneRenderer )
{
	check(MainSceneRenderer);

	// render target resource to render with
	FTextureRenderTargetResource* RTResource = TextureTarget ? TextureTarget->GetRenderTargetResource() : NULL;
	if (RTResource != NULL && MainSceneRenderer->ViewFamily.Views.Num() && UpdateRequired(MainSceneRenderer->ViewFamily))
	{
		LastCaptureTime = MainSceneRenderer->ViewFamily.CurrentWorldTime;
		// should have a 2d render target resource
		check(RTResource->GetTextureRenderTarget2DResource());
		// create a temp view family for rendering the scene. use the same scene as parent
		FSceneViewFamilyContext ViewFamily(
			RTResource,
			(FSceneInterface*)MainSceneRenderer->Scene,
			ShowFlags,
			MainSceneRenderer->ViewFamily.CurrentWorldTime,
			MainSceneRenderer->ViewFamily.CurrentRealTime,
			NULL
			);

		INT MaxViewIdx = GIsEditor ? 1 : MainSceneRenderer->ViewFamily.Views.Num(); 
		for( INT ViewIdx=0; ViewIdx < MaxViewIdx; ViewIdx++ )
		{
			// use the first view as the parent
			FSceneView* ParentView = (FSceneView*)MainSceneRenderer->ViewFamily.Views(ViewIdx);

			// create a mirror matrix and premultiply the view transform by it
			FMirrorMatrix MirrorMatrix( MirrorPlane );
			FMatrix ViewMatrix(MirrorMatrix * ParentView->ViewMatrix);

			// get the inverse-transpose of the mirrored view matrix
			FMatrix ViewMatInvT( ViewMatrix.Inverse() );
			ViewMatInvT = ViewMatInvT.Transpose();
			// transform the clip plane so that it is in view space
			FPlane MirrorPlaneViewSpace( ViewMatInvT.TransformFVector4(FVector4(MirrorPlane.X,MirrorPlane.Y,MirrorPlane.Z,MirrorPlane.W)) );

			// create a skewed projection matrix that aligns the near plane with the mirror plane
			// so that items behind the plane are properly clipped
			FClipProjectionMatrix ClipProjectionMatrix( ParentView->ProjectionMatrix, MirrorPlaneViewSpace );
 
			// make sure the render targets are big enough
			GSceneRenderTargets.Allocate(RTResource->GetSizeX(), RTResource->GetSizeY());

			FLOAT X = (FLOAT)RTResource->GetSizeX() * (ParentView->X / GSceneRenderTargets.GetBufferSizeX());
			FLOAT Y = (FLOAT)RTResource->GetSizeY() * (ParentView->Y / GSceneRenderTargets.GetBufferSizeY());
			FLOAT SizeX = (FLOAT)RTResource->GetSizeX() * (ParentView->SizeX / GSceneRenderTargets.GetBufferSizeX());
			FLOAT SizeY = (FLOAT)RTResource->GetSizeY() * (ParentView->SizeY / GSceneRenderTargets.GetBufferSizeY());
			// clamp to RT size
			SizeX = Min<FLOAT>(SizeX,RTResource->GetSizeX());
			SizeY = Min<FLOAT>(SizeY,RTResource->GetSizeY());

			// create a view for the capture
			FSceneView* View = new FSceneView(
				&ViewFamily,
				ViewState,
				NULL,
				ViewActor,
				PostProcess,
				NULL,
				NULL,
				X,
				Y,
				SizeX,
				SizeY,
				ViewMatrix,
				ClipProjectionMatrix,
				BackgroundColor,
				FLinearColor(0.f,0.f,0.f,0.f),
				FLinearColor::White,
				TArray<FPrimitiveSceneInfo*>()
				);
			// add the view to the family
			ViewFamily.Views.AddItem(View);
		}
		
		// create a new scene renderer for rendering the capture
		FSceneRenderer* CaptureSceneRenderer = ::new FSceneRenderer(
			&ViewFamily,
			NULL,
			MainSceneRenderer->CanvasTransform
			);
		// set the rendering context for the capture renderer
		CaptureSceneRenderer->GlobalContext = MainSceneRenderer->GlobalContext;

		// render the scene to the target
		CaptureSceneRenderer->Render();

		// copy the results of the scene rendering from the target surface to its texture
		RHICopyToResolveTarget(RTResource->GetRenderTargetSurface(), FALSE);

		delete CaptureSceneRenderer;
	}
}

/*-----------------------------------------------------------------------------
USceneCapturePortalComponent
-----------------------------------------------------------------------------*/

/**
* Attach a new portal capture component
*/
void USceneCapturePortalComponent::Attach()
{
	Super::Attach();
}

void USceneCapturePortalComponent::execSetCaptureParameters(FFrame& Stack, RESULT_DECL)
{
	P_GET_OBJECT_OPTX(UTextureRenderTarget2D, NewTextureTarget, TextureTarget);
	P_GET_FLOAT_OPTX(NewScaleFOV, ScaleFOV);
	P_GET_ACTOR_OPTX(NewViewDest, ViewDestination);
	P_FINISH;

	// set the parameters
	TextureTarget = NewTextureTarget;
	ScaleFOV = NewScaleFOV;
	ViewDestination = NewViewDest;

	// update the sync components
	ASceneCaptureActor* CaptureActor = Cast<ASceneCaptureActor>(GetOwner());
	if (CaptureActor != NULL)
	{
		CaptureActor->SyncComponents();
	}

	bNeedsSceneUpdate = TRUE;
}

/**
* Create a new probe with info needed to render the scene
*/
FSceneCaptureProbe* USceneCapturePortalComponent::CreateSceneCaptureProbe() 
{
	// source actor is always owner
	AActor* SrcActor = Owner;
	// each portal has a destination actor (default to our Owner if one is not avaialble)
	AActor* DestActor = ViewDestination ? ViewDestination : Owner;
	check(SrcActor && DestActor);

	FRotator SrcRotation = SrcActor->Rotation + RelativeEntryRotation;

	// Inverse world transform for the source view location/orientation
	FMatrix SrcWorldToLocalM( FTranslationMatrix(-SrcActor->Location) *
		FInverseRotationMatrix( FVector(-SrcRotation.Vector()).Rotation() ) );

	// World transform for the the destination view location/orientation
	FMatrix DestLocalToWorldM( FRotationMatrix( FVector(-DestActor->Rotation.Vector()).Rotation() ) *
		FTranslationMatrix( DestActor->Location ) );

	// Transform for source to destination view
	FMatrix SrcToDestChangeBasisM( (SrcWorldToLocalM * DestLocalToWorldM).Inverse() );

	// create the Clip plane	
	// use the actor's view direction as the reflecting normal
	FVector DestActorDir( DestActor->Rotation.Vector() );
	DestActorDir.Normalize();
	FVector DestActorPos( DestActor->Location );
	FPlane ClipPlane = FPlane( DestActorDir, -1.f * (DestActorPos | DestActorDir) );

	// allocate view state, if necessary
	if (ViewState == NULL)
	{
		ViewState = AllocateViewState();
	}

	// any info from the component needed for capturing the scene should be copied to the probe
	return new FSceneCaptureProbePortal(
		Owner,
		TextureTarget,
		GetSceneShowFlags(),
		FLinearColor(ClearColor),
		FrameRate,
		PostProcess,
		ViewState,
		bSkipUpdateIfOwnerOccluded,
		MaxUpdateDist,
		SrcWorldToLocalM,
		DestLocalToWorldM,
		SrcToDestChangeBasisM,
		DestActor,
		ClipPlane
		);
}

/*-----------------------------------------------------------------------------
FSceneCaptureProbePortal
-----------------------------------------------------------------------------*/

/**
* Called by the rendering thread to render the scene for the capture
* @param	MainSceneRenderer - parent scene renderer with info needed 
*			by some of the capture types.
*/
void FSceneCaptureProbePortal::CaptureScene( FSceneRenderer* MainSceneRenderer )
{
	// render target resource to render with
	FTextureRenderTargetResource* RTResource = TextureTarget ? TextureTarget->GetRenderTargetResource() : NULL;
	if (RTResource != NULL && MainSceneRenderer->ViewFamily.Views.Num() && UpdateRequired(MainSceneRenderer->ViewFamily))
	{
		LastCaptureTime = MainSceneRenderer->ViewFamily.CurrentWorldTime;
		// should have a 2d render target resource
		check(RTResource->GetTextureRenderTarget2DResource());
		// create a temp view family for rendering the scene. use the same scene as parent
		FSceneViewFamilyContext ViewFamily(
			RTResource,
			(FSceneInterface*)MainSceneRenderer->Scene,
			ShowFlags,
			MainSceneRenderer->ViewFamily.CurrentWorldTime,
			MainSceneRenderer->ViewFamily.CurrentRealTime,
			NULL
			);

		// all prim components from the destination portal should be hidden
		TArray<FPrimitiveSceneInfo*> HiddenPrimitives;
		for( INT CompIdx=0; CompIdx < DestViewActor->Components.Num(); CompIdx++ )
		{
			UPrimitiveComponent* PrimComponent = Cast<UPrimitiveComponent>(DestViewActor->Components(CompIdx));
			if( PrimComponent &&
				PrimComponent->SceneInfo )
			{
				HiddenPrimitives.AddItem(PrimComponent->SceneInfo);
			}
		}

		INT MaxViewIdx = GIsEditor ? 1 : MainSceneRenderer->ViewFamily.Views.Num(); 
		for( INT ViewIdx=0; ViewIdx < MaxViewIdx; ViewIdx++ )
		{
			// use the first view as the parent
			FSceneView* ParentView = (FSceneView*)MainSceneRenderer->ViewFamily.Views(ViewIdx);

			// transform view matrix so that it is relative to Destination 
			FMatrix ViewMatrix = SrcToDestChangeBasisM * ParentView->ViewMatrix;			
			
			// get the inverse-transpose of the mirrored view matrix
			FMatrix ViewMatInvT( ViewMatrix.Inverse() );
			ViewMatInvT = ViewMatInvT.Transpose();
			// transform the clip plane so that it is in view space
			FPlane MirrorPlaneViewSpace( ViewMatInvT.TransformFVector4(FVector4(ClipPlane.X,ClipPlane.Y,ClipPlane.Z,ClipPlane.W)) );

			// create a skewed projection matrix that aligns the near plane with the mirror plane
			// so that items behind the plane are properly clipped
			FClipProjectionMatrix ClipProjectionMatrix( ParentView->ProjectionMatrix, MirrorPlaneViewSpace );

			// make sure the render targets are big enough
			GSceneRenderTargets.Allocate(RTResource->GetSizeX(), RTResource->GetSizeY());

			FLOAT X = (FLOAT)RTResource->GetSizeX() * (ParentView->X / GSceneRenderTargets.GetBufferSizeX());
			FLOAT Y = (FLOAT)RTResource->GetSizeY() * (ParentView->Y / GSceneRenderTargets.GetBufferSizeY());
			FLOAT SizeX = (FLOAT)RTResource->GetSizeX() * (ParentView->SizeX / GSceneRenderTargets.GetBufferSizeX());
			FLOAT SizeY = (FLOAT)RTResource->GetSizeY() * (ParentView->SizeY / GSceneRenderTargets.GetBufferSizeY());
			// clamp to RT size
			SizeX = Min<FLOAT>(SizeX,RTResource->GetSizeX());
			SizeY = Min<FLOAT>(SizeY,RTResource->GetSizeY());

			// create a view for the capture
			FSceneView* View = new FSceneView(
				&ViewFamily,
				ViewState,
				NULL,
				ViewActor,
				PostProcess,
				NULL,
				NULL,
				X,
				Y,
				SizeX,
				SizeY,
				ViewMatrix,
				ClipProjectionMatrix,
				BackgroundColor,
				FLinearColor(0.f,0.f,0.f,0.f),
				FLinearColor::White,
				HiddenPrimitives
				);
			// add the view to the family
			ViewFamily.Views.AddItem(View);
		}

		// create a new scene renderer for rendering the capture
		FSceneRenderer* CaptureSceneRenderer = ::new FSceneRenderer(
			&ViewFamily,
			NULL,
			MainSceneRenderer->CanvasTransform
			);
		// set the rendering context for the capture renderer
		CaptureSceneRenderer->GlobalContext = MainSceneRenderer->GlobalContext;

		// render the scene to the target
		CaptureSceneRenderer->Render();

		// copy the results of the scene rendering from the target surface to its texture
		RHICopyToResolveTarget(RTResource->GetRenderTargetSurface(), FALSE);

		delete CaptureSceneRenderer;
	}

}

/*-----------------------------------------------------------------------------
ASceneCaptureActor
-----------------------------------------------------------------------------*/

/** synchronize components when a property changes */
void ASceneCaptureActor::PostEditChange( UProperty* PropertyThatChanged )
{
	SyncComponents();
	Super::PostEditChange( PropertyThatChanged );
}

/** synchronize components after load */
void ASceneCaptureActor::PostLoad()
{
	Super::PostLoad();
	SyncComponents();
}

void ASceneCaptureActor::CheckForErrors()
{
	Super::CheckForErrors();
	if( SceneCapture == NULL )
	{
		GWarn->MapCheck_Add( MCTYPE_WARNING, this, *FString::Printf(TEXT("%s : SceneCaptureActor has NULL SceneCapture property - please delete!"), *GetName() ), MCACTION_DELETE, TEXT("SceneCaptureNull") );
	}
}

/*-----------------------------------------------------------------------------
ASceneCapture2DActor
-----------------------------------------------------------------------------*/

/**
 * Sync updated properties
 */
void ASceneCapture2DActor::SyncComponents()
{
	USceneCapture2DComponent* SceneCapture2D = Cast<USceneCapture2DComponent>(SceneCapture);

	// update the draw frustum component to match the scene capture settings
	if( DrawFrustum &&
		SceneCapture2D )
	{
		//@r2t todo - texture on near plane doesn't display ATM
		// set texture to display on near plane
        DrawFrustum->Texture = SceneCapture2D->TextureTarget;
		// FOV
		DrawFrustum->FrustumAngle = SceneCapture2D->FieldOfView;
        // near/far plane
		const FLOAT MIN_NEAR_DIST = 50.f;
		const FLOAT MIN_FAR_DIST = 200.f;
		DrawFrustum->FrustumStartDist = Max( SceneCapture2D->NearPlane, MIN_NEAR_DIST );
		DrawFrustum->FrustumEndDist = Max( SceneCapture2D->FarPlane, MIN_FAR_DIST );
		// update aspect ratio
		if( SceneCapture2D->TextureTarget )
		{
			FLOAT fAspect = (FLOAT)SceneCapture2D->TextureTarget->SizeX / (FLOAT)SceneCapture2D->TextureTarget->SizeY;
			DrawFrustum->FrustumAspectRatio = fAspect;
		}
	}
}

/*-----------------------------------------------------------------------------
ASceneCaptureCubeMapActor
-----------------------------------------------------------------------------*/

/**
* Init the helper components 
*/
void ASceneCaptureCubeMapActor::Init()
{
	if( GEngine->SceneCaptureCubeActorMaterial && !CubeMaterialInst )
	{
		// create new instance
		// note that it has to be RF_Transient or the map won't save
		CubeMaterialInst = ConstructObject<UMaterialInstanceConstant>( 
			UMaterialInstanceConstant::StaticClass(),
			INVALID_OBJECT, 
			NAME_None, 
			RF_Transient, 
			NULL );

		// init with the parent material
		CubeMaterialInst->SetParent( GEngine->SceneCaptureCubeActorMaterial );
	}

	if( StaticMesh && CubeMaterialInst )
	{
		// assign the material instance to the static mesh plane
		if( StaticMesh->Materials.Num() == 0 )
		{
			// add a slot if needed
			StaticMesh->Materials.Add();
		}
		// only one material entry
		StaticMesh->Materials(0) = CubeMaterialInst;
	}
}

/**
* Called when the actor is loaded
*/
void ASceneCaptureCubeMapActor::PostLoad()
{
	Super::PostLoad();

	// initialize components
	Init();	

	// update
	SyncComponents();
}

/**
* Called when the actor is spawned
*/
void ASceneCaptureCubeMapActor::Spawned()
{
	Super::Spawned();

	// initialize components
	Init();	

	// update
	SyncComponents();
}

/**
* Called when the actor is destroyed
*/
void ASceneCaptureCubeMapActor::FinishDestroy()
{
	// clear the references to the cube material instance
	if( StaticMesh )
	{
		for( INT i=0; i < StaticMesh->Materials.Num(); i++ )
		{
			if( StaticMesh->Materials(i) == CubeMaterialInst ) {
				StaticMesh->Materials(i) = NULL;
			}
		}
	}
	CubeMaterialInst = NULL;

	Super::FinishDestroy();
}

/**
* Sync updated properties
*/
void ASceneCaptureCubeMapActor::SyncComponents()
{
	USceneCaptureCubeMapComponent* SceneCaptureCube = Cast<USceneCaptureCubeMapComponent>(SceneCapture);

	if( !SceneCaptureCube )
		return;

	if( CubeMaterialInst )
	{
		// update the material instance texture parameter
		CubeMaterialInst->SetTextureParameterValue( FName(TEXT("TexCube")), SceneCaptureCube->TextureTarget );        
	}
}

/*-----------------------------------------------------------------------------
ASceneCaptureReflectActor
-----------------------------------------------------------------------------*/

/**
 * Init the helper components 
 */
void ASceneCaptureReflectActor::Init()
{
	if( GEngine->SceneCaptureReflectActorMaterial && !ReflectMaterialInst )
	{
		// create new instance
		// note that it has to be RF_Transient or the map won't save
		ReflectMaterialInst = ConstructObject<UMaterialInstanceConstant>( 
			UMaterialInstanceConstant::StaticClass(),
			INVALID_OBJECT, 
			NAME_None, 
			RF_Transient, 
			NULL );

		// init with the parent material
		ReflectMaterialInst->SetParent( GEngine->SceneCaptureReflectActorMaterial );		
	}

	if( StaticMesh && ReflectMaterialInst )
	{
		// assign the material instance to the static mesh plane
		if( StaticMesh->Materials.Num() == 0 )
		{
			// add a slot if needed
			StaticMesh->Materials.Add();
		}
		// only one material entry
		StaticMesh->Materials(0) = ReflectMaterialInst;
	}
}

/**
 * Called when the actor is loaded
 */
void ASceneCaptureReflectActor::PostLoad()
{
	Super::PostLoad();

	// initialize components
    Init();	

	// update
	SyncComponents();
}

/**
 * Called when the actor is spawned
 */
void ASceneCaptureReflectActor::Spawned()
{
	Super::Spawned();

	// initialize components
    Init();	

	// update
	SyncComponents();
}

/**
 * Called when the actor is destroyed
 */
void ASceneCaptureReflectActor::FinishDestroy()
{
	// clear the references to the reflect material instance
	if( StaticMesh )
	{
        for( INT i=0; i < StaticMesh->Materials.Num(); i++ )
		{
			if( StaticMesh->Materials(i) == ReflectMaterialInst ) {
				StaticMesh->Materials(i) = NULL;
			}
		}
	}
	ReflectMaterialInst = NULL;

	Super::FinishDestroy();
}

/**
 * Sync updated properties
 */
void ASceneCaptureReflectActor::SyncComponents()
{
	USceneCaptureReflectComponent* SceneCaptureReflect = Cast<USceneCaptureReflectComponent>(SceneCapture);	

	if( !SceneCaptureReflect )
		return;

	if( ReflectMaterialInst )
	{
		// update the material instance texture parameter
        ReflectMaterialInst->SetTextureParameterValue( FName(TEXT("ScreenTex")), SceneCaptureReflect->TextureTarget );        
	}
}

/*-----------------------------------------------------------------------------
ASceneCapturePortalActor
-----------------------------------------------------------------------------*/

/**
* Sync updated properties
*/
void ASceneCapturePortalActor::SyncComponents()
{
	USceneCapturePortalComponent* SceneCapturePortal = Cast<USceneCapturePortalComponent>(SceneCapture);

	if( !SceneCapturePortal )
		return;

	if( ReflectMaterialInst )
	{
		// update the material instance texture parameter
		ReflectMaterialInst->SetTextureParameterValue( FName(TEXT("ScreenTex")), SceneCapturePortal->TextureTarget );        
	}
}


/*-----------------------------------------------------------------------------
APortalTeleporter
-----------------------------------------------------------------------------*/

void APortalTeleporter::Spawned()
{
	// create a render to texture target for the portal
	USceneCapturePortalComponent* PortalComponent = Cast<USceneCapturePortalComponent>(SceneCapture);
	if (PortalComponent != NULL)
	{
		PortalComponent->TextureTarget = CreatePortalTexture();
	}
	Super::Spawned();
}

void APortalTeleporter::PostLoad()
{
	// create a render to texture target for the portal
	USceneCapturePortalComponent* PortalComponent = Cast<USceneCapturePortalComponent>(SceneCapture);
	if (PortalComponent != NULL)
	{
		PortalComponent->TextureTarget = CreatePortalTexture();
		// also make sure destination is correct
		PortalComponent->ViewDestination = SisterPortal;
	}
	Super::PostLoad();
}

void APortalTeleporter::PostEditChange(UProperty* PropertyThatChanged)
{
	USceneCapturePortalComponent* PortalComponent = Cast<USceneCapturePortalComponent>(SceneCapture);
	if (PropertyThatChanged != NULL)
	{
		if (PropertyThatChanged->GetFName() == FName(TEXT("TextureResolutionX")) || PropertyThatChanged->GetFName() == FName(TEXT("TextureResolutionY")))
		{
			// enforce valid positive power of 2 values for resolution
			if (TextureResolutionX <= 2)
			{
				TextureResolutionX = 2;
			}
			else
			{
				TextureResolutionX = 1 << appCeilLogTwo(TextureResolutionX);
			}
			if (TextureResolutionY <= 2)
			{
				TextureResolutionY = 2;
			}
			else
			{
				TextureResolutionY = 1 << appCeilLogTwo(TextureResolutionY);
			}
			if (PortalComponent != NULL)
			{
				if (PortalComponent->TextureTarget == NULL)
				{
					PortalComponent->TextureTarget = CreatePortalTexture();
				}
				else
				{
					// update the size of the texture
					PortalComponent->TextureTarget->Init(
						TextureResolutionX,TextureResolutionY,(EPixelFormat)PortalComponent->TextureTarget->Format);
				}
			}
		}
	}

	// propagate bMovable
	if (bMovablePortal != bMovable)
	{
		bMovable = bMovablePortal;
		// bMovable affects static shadows, so lightning must be rebuilt if this flag is changed
		GWorld->GetWorldInfo()->SetMapNeedsLightingFullyRebuilt( TRUE );
	}

	if (PortalComponent != NULL)
	{
		// try to update the sister portal if the view destination was set
		if (PropertyThatChanged->GetFName() == FName(TEXT("ViewDestination")))
		{
			SisterPortal = Cast<APortalTeleporter>(PortalComponent->ViewDestination);
		}
		// make sure the component has the proper view destination
		if (PropertyThatChanged->GetFName() == FName(TEXT("SisterPortal")))
		{
			PortalComponent->ViewDestination = SisterPortal;
		}
		// update entry rotation arrow
		if (EntryRotationArrow != NULL)
		{
			EntryRotationArrow->Rotation = PortalComponent->RelativeEntryRotation;
		}
	}

	Super::PostEditChange(PropertyThatChanged);
}

UTextureRenderTarget2D* APortalTeleporter::CreatePortalTexture()
{
	if (TextureResolutionX <= 2 || TextureResolutionY <= 2)
	{
		debugf(NAME_Warning, TEXT("%s cannot create portal texture because invalid resolution specified"), *GetName());
		return NULL;
	}
	else
	{
		// make sure resolution is a power of 2
		TextureResolutionX = 1 << appCeilLogTwo(TextureResolutionX);
		TextureResolutionY = 1 << appCeilLogTwo(TextureResolutionY);
		// create and initialize the texture
		UTextureRenderTarget2D* PortalTexture = CastChecked<UTextureRenderTarget2D>(StaticConstructObject(UTextureRenderTarget2D::StaticClass(), GetOuter(), NAME_None, RF_Transient));
		PortalTexture->Init(TextureResolutionX, TextureResolutionY, PF_A8R8G8B8);
		return PortalTexture;
	}
}

UBOOL APortalTeleporter::TransformActor(AActor* A)
{
	// if there's no destination or we're not allowed to teleport the actor, abort
	USceneCapturePortalComponent* PortalComponent = Cast<USceneCapturePortalComponent>(SceneCapture);
	if (SisterPortal == NULL || PortalComponent == NULL || !CanTeleport(A))
	{
		return FALSE;
	}

	FMatrix WorldToLocalM = WorldToLocal() * FRotationMatrix(PortalComponent->RelativeEntryRotation);
	FMatrix SisterLocalToWorld = SisterPortal->LocalToWorld();

	FVector LocalLocation = WorldToLocalM.TransformFVector(A->Location);
	LocalLocation.X *= -1.f;
	LocalLocation = SisterLocalToWorld.TransformFVector(LocalLocation);

	if (!GWorld->FarMoveActor(A, LocalLocation, 0, 0, 0))
	{
		return FALSE;
	}

	FRotationMatrix SourceAxes(Rotation + PortalComponent->RelativeEntryRotation);
	FVector SourceXAxis = SourceAxes.GetAxis(0);
	FVector SourceYAxis = SourceAxes.GetAxis(1);
	FVector SourceZAxis = SourceAxes.GetAxis(2);

	FRotationMatrix DestAxes(SisterPortal->Rotation);
	FVector DestXAxis = DestAxes.GetAxis(0);
	FVector DestYAxis = DestAxes.GetAxis(1);
	FVector DestZAxis = DestAxes.GetAxis(2);

	// transform velocity
	FVector LocalVelocity = A->Velocity;
	FVector LocalVector;
	LocalVector.X = SourceXAxis | LocalVelocity;
	LocalVector.Y = SourceYAxis | LocalVelocity;
	LocalVector.Z = SourceZAxis | LocalVelocity;
	A->Velocity = LocalVector.X * DestXAxis + LocalVector.Y * DestYAxis + LocalVector.Z * DestZAxis;

	// transform acceleration
	FVector LocalAcceleration = A->Acceleration;
	LocalVector.X = SourceXAxis | LocalAcceleration;
	LocalVector.Y = SourceYAxis | LocalAcceleration;
	LocalVector.Z = SourceZAxis | LocalAcceleration;
	A->Acceleration = LocalVector.X * DestXAxis + LocalVector.Y * DestYAxis + LocalVector.Z * DestZAxis;

	// transform rotation
	INT SavedRoll = A->Rotation.Roll;
	FVector RotDir = A->Rotation.Vector();
	LocalVector.X = SourceXAxis | RotDir;
	LocalVector.Y = SourceYAxis | RotDir;
	LocalVector.Z = SourceZAxis | RotDir;
	RotDir = LocalVector.X * DestXAxis + LocalVector.Y * DestYAxis + LocalVector.Z * DestZAxis;
	FRotator NewRotation = RotDir.Rotation();
	NewRotation.Roll = SavedRoll;
	FCheckResult Hit(1.f);
	GWorld->MoveActor(A, FVector(0,0,0), NewRotation, 0, Hit);

	APawn* P = A->GetAPawn();
	if (P != NULL && P->Controller != NULL)
	{
		// transform Controller rotation
		SavedRoll = P->Controller->Rotation.Roll;
		RotDir = P->Controller->Rotation.Vector();
		LocalVector.X = SourceXAxis | RotDir;
		LocalVector.Y = SourceYAxis | RotDir;
		LocalVector.Z = SourceZAxis | RotDir;
		RotDir = LocalVector.X * DestXAxis + LocalVector.Y * DestYAxis + LocalVector.Z * DestZAxis;
		NewRotation = RotDir.Rotation();
		NewRotation.Roll = SavedRoll;
		GWorld->MoveActor(P->Controller, FVector(0,0,0), NewRotation, 0, Hit);

		P->Anchor = MyMarker;
		P->Controller->MoveTimer = -1.0f;
	}

	return TRUE;
}

FVector APortalTeleporter::TransformVector(FVector V)
{
	USceneCapturePortalComponent* PortalComponent = Cast<USceneCapturePortalComponent>(SceneCapture);
	if (!SisterPortal || PortalComponent == NULL)
		return V;

	FRotationMatrix SourceAxes(Rotation + PortalComponent->RelativeEntryRotation);
	FVector SourceXAxis = SourceAxes.GetAxis(0);
	FVector SourceYAxis = SourceAxes.GetAxis(1);
	FVector SourceZAxis = SourceAxes.GetAxis(2);

	FRotationMatrix DestAxes(SisterPortal->Rotation);
	FVector DestXAxis = DestAxes.GetAxis(0);
	FVector DestYAxis = DestAxes.GetAxis(1);
	FVector DestZAxis = DestAxes.GetAxis(2);
	FVector LocalVector;

	// transform velocity
	LocalVector.X = SourceXAxis | V;
	LocalVector.Y = SourceYAxis | V;
	LocalVector.Z = SourceZAxis | V;
	return LocalVector.X * DestXAxis + LocalVector.Y * DestYAxis + LocalVector.Z * DestZAxis;
}

FVector APortalTeleporter::TransformHitLocation(FVector HitLocation)
{
	USceneCapturePortalComponent* PortalComponent = Cast<USceneCapturePortalComponent>(SceneCapture);
	if (!SisterPortal || PortalComponent == NULL)
		return HitLocation;

	FMatrix WorldToLocalM = WorldToLocal() * FRotationMatrix(PortalComponent->RelativeEntryRotation);
	FMatrix SisterLocalToWorld = SisterPortal->LocalToWorld();

	FVector LocalLocation = WorldToLocalM.TransformFVector(HitLocation);
	LocalLocation.X *= -1.f;
	return SisterLocalToWorld.TransformFVector(LocalLocation);
}

void APortalTeleporter::TickSpecial(FLOAT DeltaTime)
{
	Super::TickSpecial(DeltaTime);

	// update ourselves in Controllers' VisiblePortals array
	for (AController* C = WorldInfo->ControllerList; C != NULL; C = C->NextController)
	{
		if (C->SightCounter < 0.0f)
		{
			FCheckResult Hit(1.0f);
			if (GWorld->SingleLineCheck(Hit, this, Location, C->GetViewTarget()->Location, TRACE_World | TRACE_StopAtAnyHit))
			{
				// we are visible to C
				C->VisiblePortals.AddUniqueItem(this);
			}
			else
			{
				C->VisiblePortals.RemoveItem(this);
			}
		}
	}
}

UBOOL APortalTeleporter::CanTeleport(AActor* A)
{
	if (A == NULL)
	{
		return FALSE;
	}
	else if (bAlwaysTeleportNonPawns && A->GetAPawn() == NULL)
	{
		return TRUE;
	}
	else
	{
		return (A->bCanTeleport && (bCanTeleportVehicles || Cast<AVehicle>(A) == NULL));
	}
}
