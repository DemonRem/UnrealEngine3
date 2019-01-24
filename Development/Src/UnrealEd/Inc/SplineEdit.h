/*=============================================================================
	SplineEdit.h
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef _SPLINE_EDIT_H
#define _SPLINE_EDIT_H

/** Hit proxy used for SplineActor tangent handles */
struct HSplineHandleProxy : public HHitProxy
{
	DECLARE_HIT_PROXY(HSplineHandleProxy,HHitProxy);

	/** SplineActor that this is a handle for */
	ASplineActor* SplineActor;
	/** If this is an arrive or leave handle */
	UBOOL bArrive;

	HSplineHandleProxy(ASplineActor* InSplineActor, UBOOL bInArrive): 
	HHitProxy(HPP_UI), 
		SplineActor(InSplineActor),
		bArrive(bInArrive)
	{}

	/** Show cursor as cross when over this handle */
	virtual EMouseCursor GetMouseCursor()
	{
		return MC_Cross;
	}
	
	virtual void Serialize(FArchive& Ar)
	{
		Ar << SplineActor;
	}
};

/** Util to break a spline, given the hit proxy that was clicked on */
void SplineBreakGivenProxy(HSplineProxy* SplineProxy);

/*------------------------------------------------------------------------------
	FEdModeSpline
------------------------------------------------------------------------------*/


/** Editor mode switched to when selecting SplineActors */
class FEdModeSpline : public FEdMode
{
	/** SplineActor that we are currently modifying the tangent of */
	ASplineActor* ModSplineActor;
	/** If we are modifying the arrive or leave handle on this actor */
	UBOOL bModArrive;
	/** How much to scale tangent handles when drawing them */
	FLOAT TangentHandleScale;

public:
	FEdModeSpline();
	virtual ~FEdModeSpline();

	virtual void Render(const FSceneView* View,FViewport* Viewport,FPrimitiveDrawInterface* PDI);
	virtual UBOOL InputKey(FEditorLevelViewportClient* ViewportClient,FViewport* Viewport,FName Key,EInputEvent Event);
	virtual UBOOL InputDelta(FEditorLevelViewportClient* InViewportClient,FViewport* InViewport,FVector& InDrag,FRotator& InRot,FVector& InScale);	
	virtual UBOOL AllowWidgetMove();
	virtual void ActorsDuplicatedNotify(TArray<AActor*>& PreDuplicateSelection, TArray<AActor*>& PostDuplicateSelection, UBOOL bOffsetLocations);
};

#endif

