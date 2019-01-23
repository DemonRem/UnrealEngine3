/*================================================================================
	MeshPaintEdMode.cpp: Mesh paint tool
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
================================================================================*/

#include "UnrealEd.h"
#include "MeshPaintEdMode.h"
#include "Factories.h"
#include "../../Engine/Src/ScenePrivate.h"
#include "ScopedTransaction.h"
#include "MeshPaintRendering.h"
#include "ImageUtils.h"

#if WITH_MANAGED_CODE
#include "MeshPaintWindowShared.h"
#endif

// @todo MeshPaint
const FName PaintTextureName( TEXT( "PaintTexture" ) ); // TEXT( "Diffuse" ) );


/** Static: Global mesh paint settings */
FMeshPaintSettings FMeshPaintSettings::StaticMeshPaintSettings;



/** Constructor */
FEdModeMeshPaint::FEdModeMeshPaint() 
	: FEdMode(),
	  bIsPainting( FALSE ),
	  bIsFloodFill( FALSE ),
	  bPushInstanceColorsToMesh( FALSE ),
	  PaintingStartTime( 0.0 ),
	  ModifiedStaticMeshes(),
	  TexturePaintingStaticMeshComponent( NULL ),
	  PaintingTexture2D( NULL ),
	  PaintRenderTargetTexture( NULL ),
	  CloneRenderTargetTexture( NULL )
{
	ID = EM_MeshPaint;
	Desc = TEXT( "Mesh Painting" );
}



/** Destructor */
FEdModeMeshPaint::~FEdModeMeshPaint()
{
}



/** FSerializableObject: Serializer */
void FEdModeMeshPaint::Serialize( FArchive &Ar )
{
	// Call parent implementation
	FEdMode::Serialize( Ar );

	Ar << ModifiedStaticMeshes;
	Ar << StaticMeshToTempkDOPMap;
	Ar << TexturePaintingStaticMeshComponent;
	Ar << PaintingTexture2D;
	Ar << PaintRenderTargetTexture;
	Ar << CloneRenderTargetTexture;
}



/** FEdMode: Called when the mode is entered */
void FEdModeMeshPaint::Enter()
{
	// Call parent implementation
	FEdMode::Enter();


	{
		// Load config from file
		{
			const FString SectionName = TEXT( "MeshPaint" );

			// WindowPosition ("X,Y")
			const FString WindowPositionFieldName = TEXT( "WindowPosition" );
			FString WindowPositionString;
			if( GConfig->GetString( *SectionName,
									*WindowPositionFieldName,
									WindowPositionString,
									GEditorUserSettingsIni ) )
			{
				TArray< FString > PositionValues;
				const UBOOL bCullEmptyStrings = TRUE;
				WindowPositionString.ParseIntoArray( &PositionValues, TEXT( "," ), bCullEmptyStrings );
				if( PositionValues.Num() >= 1 )
				{
					FMeshPaintSettings::Get().WindowPositionX = appAtoi( *PositionValues( 0 ) );
				}
				if( PositionValues.Num() >= 2 )
				{
					FMeshPaintSettings::Get().WindowPositionY = appAtoi( *PositionValues( 1 ) );
				}
			}
		}


	}

#if WITH_MANAGED_CODE
	// Create the mesh paint window
	MeshPaintWindow.Reset( FMeshPaintWindow::CreateMeshPaintWindow( this ) );
	check( MeshPaintWindow.IsValid() );
#endif



	// Change the engine to draw selected objects without a color boost, but unselected objects will
	// be darkened slightly.  This just makes it easier to paint on selected objects without the
	// highlight effect distorting the appearance.
	GEngine->SelectedMaterialColor = FLinearColor::Black;		// Additive (black = off)
//	GEngine->UnselectedMaterialColor = FLinearColor( 0.05f, 0.0f, 0.0f, 1.0f );



	// Force real-time viewports.  We'll back up the current viewport state so we can restore it when the
	// user exits this mode.
	const UBOOL bWantRealTime = TRUE;
	const UBOOL bRememberCurrentState = TRUE;
	ForceRealTimeViewports( bWantRealTime, bRememberCurrentState );

	// Set viewport show flags
	const UBOOL bAllowColorViewModes = TRUE;
	SetViewportShowFlags( bAllowColorViewModes );
}



/** FEdMode: Called when the mode is exited */
void FEdModeMeshPaint::Exit()
{
	// Restore real-time viewport state if we changed it
	const UBOOL bWantRealTime = FALSE;
	const UBOOL bRememberCurrentState = FALSE;
	ForceRealTimeViewports( bWantRealTime, bRememberCurrentState );

	// Disable color view modes if we set those
	const UBOOL bAllowColorViewModes = FALSE;
	SetViewportShowFlags( bAllowColorViewModes );

	// Restore selection color
	GEngine->SelectedMaterialColor = GEditorModeTools().GetHighlightWithBrackets() ? FLinearColor::Black : GEngine->DefaultSelectedMaterialColor;
//	GEngine->UnselectedMaterialColor = FLinearColor::Black;			// Additive (black = off)

	// Remove all custom made kDOP's
	StaticMeshToTempkDOPMap.Empty();

	// Save any settings that may have changed
#if WITH_MANAGED_CODE
	// Kill the mesh paint window
	MeshPaintWindow.Reset();
#endif

	// Call parent implementation
	FEdMode::Exit();
}



/** FEdMode: Called when the mouse is moved over the viewport */
UBOOL FEdModeMeshPaint::MouseMove( FEditorLevelViewportClient* ViewportClient, FViewport* Viewport, INT x, INT y )
{
	// We only care about perspective viewports
	if( ViewportClient->IsPerspective() )
	{
		// ...
	}

	return FALSE;
}



/**
 * Called when the mouse is moved while a window input capture is in effect
 *
 * @param	InViewportClient	Level editor viewport client that captured the mouse input
 * @param	InViewport			Viewport that captured the mouse input
 * @param	InMouseX			New mouse cursor X coordinate
 * @param	InMouseY			New mouse cursor Y coordinate
 *
 * @return	TRUE if input was handled
 */
UBOOL FEdModeMeshPaint::CapturedMouseMove( FEditorLevelViewportClient* InViewportClient, FViewport* InViewport, INT InMouseX, INT InMouseY )
{
	// We only care about perspective viewports
	if( InViewportClient->IsPerspective() )
	{
		if( bIsPainting )
		{
			// Compute a world space ray from the screen space mouse coordinates
			FSceneViewFamilyContext ViewFamily(
				InViewportClient->Viewport, InViewportClient->GetScene(),
				InViewportClient->ShowFlags,
				GWorld->GetTimeSeconds(),
				GWorld->GetDeltaSeconds(),
				GWorld->GetRealTimeSeconds(),
				InViewportClient->IsRealtime()
				);
			FSceneView* View = InViewportClient->CalcSceneView( &ViewFamily );
			FViewportCursorLocation MouseViewportRay( View, (FEditorLevelViewportClient*)InViewport->GetClient(), InMouseX, InMouseY );

			
			// Paint!
			const UBOOL bVisualCueOnly = FALSE;
			const EMeshPaintAction::Type PaintAction = GetPaintAction(InViewport);
			// Apply stylus pressure
			const FLOAT StrengthScale = InViewport->IsPenActive() ? InViewport->GetTabletPressure() : 1.f;

			UBOOL bAnyPaintAbleActorsUnderCursor = FALSE;
			DoPaint( View->ViewOrigin, MouseViewportRay.GetOrigin(), MouseViewportRay.GetDirection(), NULL, PaintAction, bVisualCueOnly, StrengthScale, bAnyPaintAbleActorsUnderCursor );

			return TRUE;
		}
	}

	return FALSE;
}



/** FEdMode: Called when a mouse button is pressed */
UBOOL FEdModeMeshPaint::StartTracking()
{
	return TRUE;
}



/** FEdMode: Called when the a mouse button is released */
UBOOL FEdModeMeshPaint::EndTracking()
{
	return TRUE;
}



/** FEdMode: Called when a key is pressed */
UBOOL FEdModeMeshPaint::InputKey( FEditorLevelViewportClient* InViewportClient, FViewport* InViewport, FName InKey, EInputEvent InEvent )
{
	UBOOL bHandled = FALSE;

	// We only care about perspective viewports
	if( InViewportClient->IsPerspective() )
	{
		const UBOOL bIsLeftButtonDown = ( InKey == KEY_LeftMouseButton && InEvent != IE_Released ) || InViewport->KeyState( KEY_LeftMouseButton );
		const UBOOL bIsCtrlDown = ( ( InKey == KEY_LeftControl || InKey == KEY_RightControl ) && InEvent != IE_Released ) || InViewport->KeyState( KEY_LeftControl ) || InViewport->KeyState( KEY_RightControl );

		// Does the user want to paint right now?
		const UBOOL bUserWantsPaint = bIsLeftButtonDown && bIsCtrlDown;
		UBOOL bAnyPaintAbleActorsUnderCursor = FALSE;

		// Stop current tracking if the user released Ctrl+LMB
		if( bIsPainting && !bUserWantsPaint )
		{
			bHandled = TRUE;
			bIsPainting = FALSE;

			// Rebuild any static meshes that we painted on last stroke
			{
				for( INT CurMeshIndex = 0; CurMeshIndex < ModifiedStaticMeshes.Num(); ++CurMeshIndex )
				{
					UStaticMesh* CurStaticMesh = ModifiedStaticMeshes( CurMeshIndex );

					// @todo MeshPaint: Do we need to do bother doing a full rebuild even with real-time turbo-rebuild?
					if( 0 )
					{
						// Rebuild the modified mesh
						CurStaticMesh->Build();
					}
				}

				ModifiedStaticMeshes.Empty();
			}	
		}
		else if( !bIsPainting && bUserWantsPaint )
		{
			// Re-initialize new tracking only if a new button was pressed, otherwise we continue the previous one.
			bIsPainting = TRUE;
			PaintingStartTime = appSeconds();
			bHandled = TRUE;


			// Go ahead and paint immediately
			{
				// Compute a world space ray from the screen space mouse coordinates
				FSceneViewFamilyContext ViewFamily(
					InViewportClient->Viewport, InViewportClient->GetScene(),
					InViewportClient->ShowFlags,
					GWorld->GetTimeSeconds(),
					GWorld->GetDeltaSeconds(),
					GWorld->GetRealTimeSeconds(),
					InViewportClient->IsRealtime()
					);
				FSceneView* View = InViewportClient->CalcSceneView( &ViewFamily );
				FViewportCursorLocation MouseViewportRay( View, (FEditorLevelViewportClient*)InViewport->GetClient(), InViewport->GetMouseX(), InViewport->GetMouseY() );

				// Paint!
				const UBOOL bVisualCueOnly = FALSE;
				const EMeshPaintAction::Type PaintAction = GetPaintAction(InViewport);
				const FLOAT StrengthScale = 1.0f;
				DoPaint( View->ViewOrigin, MouseViewportRay.GetOrigin(), MouseViewportRay.GetDirection(), NULL, PaintAction, bVisualCueOnly, StrengthScale, bAnyPaintAbleActorsUnderCursor );
			}
		}


		// Also absorb other mouse buttons, and Ctrl/Alt/Shift events that occur while we're painting as these would cause
		// the editor viewport to start panning/dollying the camera
		{
			const UBOOL bIsOtherMouseButtonEvent = ( InKey == KEY_MiddleMouseButton || InKey == KEY_RightMouseButton );
			const UBOOL bCtrlButtonEvent = (InKey == KEY_LeftControl || InKey == KEY_RightControl);
			const UBOOL bShiftButtonEvent = (InKey == KEY_LeftShift || InKey == KEY_RightShift);
			const UBOOL bAltButtonEvent = (InKey == KEY_LeftAlt || InKey == KEY_RightAlt);
			if( bIsPainting && ( bIsOtherMouseButtonEvent || bShiftButtonEvent || bAltButtonEvent ) )
			{
				bHandled = TRUE;
			}

			if( bCtrlButtonEvent)
			{
				bHandled = FALSE;
			}
			//allow ctrl select to add new items to the selection
			else if( bIsCtrlDown)
			{
				//default to assuming this is a paint command
				bHandled = TRUE;

				//if no other button was pressed && if a first press and we click OFF of an actor
				if ((!(bShiftButtonEvent || bAltButtonEvent || bIsOtherMouseButtonEvent)) && ((InKey == KEY_LeftMouseButton) && ((InEvent == IE_Pressed) || (InEvent == IE_Released)) && (!bAnyPaintAbleActorsUnderCursor)))
				{
					//Let multi-select have a quick crack at this
					bHandled = FALSE;
					bIsPainting = FALSE;
				}
			}
		}
	}


	return bHandled;
}




/** Mesh paint parameters */
class FMeshPaintParameters
{

public:

	EMeshPaintMode::Type PaintMode;
	EMeshPaintAction::Type PaintAction;
	FVector BrushPosition;
	FVector BrushNormal;
	FLinearColor BrushColor;
	FLOAT SquaredBrushRadius;
	FLOAT BrushRadialFalloffRange;
	FLOAT InnerBrushRadius;
	FLOAT BrushDepth;
	FLOAT BrushDepthFalloffRange;
	FLOAT InnerBrushDepth;
	FLOAT BrushStrength;
	FMatrix BrushToWorldMatrix;
	UBOOL bWriteRed;
	UBOOL bWriteGreen;
	UBOOL bWriteBlue;
	UBOOL bWriteAlpha;
	INT TotalWeightCount;
	INT PaintWeightIndex;
	INT UVChannel;

};




/** Static: Determines if a world space point is influenced by the brush and reports metrics if so */
UBOOL FEdModeMeshPaint::IsPointInfluencedByBrush( const FVector& InPosition,
												  const FMeshPaintParameters& InParams,
												  FLOAT& OutSquaredDistanceToVertex2D,
												  FLOAT& OutVertexDepthToBrush )
{
	// Project the vertex into the plane of the brush
	FVector BrushSpaceVertexPosition = InParams.BrushToWorldMatrix.InverseTransformFVectorNoScale( InPosition );
	FVector2D BrushSpaceVertexPosition2D( BrushSpaceVertexPosition.X, BrushSpaceVertexPosition.Y );

	// Is the brush close enough to the vertex to paint?
	const FLOAT SquaredDistanceToVertex2D = BrushSpaceVertexPosition2D.SizeSquared();
	if( SquaredDistanceToVertex2D <= InParams.SquaredBrushRadius )
	{
		// OK the vertex is overlapping the brush in 2D space, but is it too close or
		// two far (depth wise) to be influenced?
		const FLOAT VertexDepthToBrush = Abs( BrushSpaceVertexPosition.Z );
		if( VertexDepthToBrush <= InParams.BrushDepth )
		{
			OutSquaredDistanceToVertex2D = SquaredDistanceToVertex2D;
			OutVertexDepthToBrush = VertexDepthToBrush;
			return TRUE;
		}
	}

	return FALSE;
}



/** Paints the specified vertex!  Returns TRUE if the vertex was in range. */
UBOOL FEdModeMeshPaint::PaintVertex( const FVector& InVertexPosition,
									 const FMeshPaintParameters& InParams,
									 const UBOOL bIsPainting,
									 FColor& InOutVertexColor )
{
	FLOAT SquaredDistanceToVertex2D;
	FLOAT VertexDepthToBrush;
	if( IsPointInfluencedByBrush( InVertexPosition, InParams, SquaredDistanceToVertex2D, VertexDepthToBrush ) )
	{
		if( bIsPainting )
		{
			// Compute amount of paint to apply
			FLOAT PaintAmount = 1.0f;

			// Apply radial-based falloff
			{
				// Compute the actual distance
				FLOAT DistanceToVertex2D = 0.0f;
				if( SquaredDistanceToVertex2D > KINDA_SMALL_NUMBER )
				{
					DistanceToVertex2D = appSqrt( SquaredDistanceToVertex2D );
				}

				if( DistanceToVertex2D > InParams.InnerBrushRadius )
				{
					const FLOAT RadialBasedFalloff = ( DistanceToVertex2D - InParams.InnerBrushRadius ) / InParams.BrushRadialFalloffRange;
					PaintAmount *= 1.0f - RadialBasedFalloff;
				}
			}

			// Apply depth-based falloff
			{
				if( VertexDepthToBrush > InParams.InnerBrushDepth )
				{
					const FLOAT DepthBasedFalloff = ( VertexDepthToBrush - InParams.InnerBrushDepth ) / InParams.BrushDepthFalloffRange;
					PaintAmount *= 1.0f - DepthBasedFalloff;
					
//						debugf( TEXT( "Painted Vertex:  DepthBasedFalloff=%.2f" ), DepthBasedFalloff );
				}
			}

			PaintAmount *= InParams.BrushStrength;

			
			// Paint!

			// NOTE: We manually perform our own conversion between FColor and FLinearColor (and vice versa) here
			//	  as we want values to be linear (not gamma corrected.)  These color values are often used as scalars
			//	  to blend between textures, etc, and must be linear!

			const FLinearColor OldColor = InOutVertexColor.ReinterpretAsLinear();
			FLinearColor NewColor = OldColor;



			if( InParams.PaintMode == EMeshPaintMode::PaintColors )
			{
				// Color painting

				if( InParams.bWriteRed )
				{
					if( OldColor.R < InParams.BrushColor.R )
					{
						NewColor.R = Min( InParams.BrushColor.R, OldColor.R + PaintAmount );
					}
					else
					{
						NewColor.R = Max( InParams.BrushColor.R, OldColor.R - PaintAmount );
					}
				}

				if( InParams.bWriteGreen )
				{
					if( OldColor.G < InParams.BrushColor.G )
					{
						NewColor.G = Min( InParams.BrushColor.G, OldColor.G + PaintAmount );
					}
					else
					{
						NewColor.G = Max( InParams.BrushColor.G, OldColor.G - PaintAmount );
					}
				}

				if( InParams.bWriteBlue )
				{
					if( OldColor.B < InParams.BrushColor.B )
					{
						NewColor.B = Min( InParams.BrushColor.B, OldColor.B + PaintAmount );
					}
					else
					{
						NewColor.B = Max( InParams.BrushColor.B, OldColor.B - PaintAmount );
					}
				}

				if( InParams.bWriteAlpha )
				{
					if( OldColor.A < InParams.BrushColor.A )
					{
						NewColor.A = Min( InParams.BrushColor.A, OldColor.A + PaintAmount );
					}
					else
					{
						NewColor.A = Max( InParams.BrushColor.A, OldColor.A - PaintAmount );
					}
				}
			}
			else if( InParams.PaintMode == EMeshPaintMode::PaintWeights )
			{
				// Weight painting
				
				
				// Total number of texture blend weights we're using
				check( InParams.TotalWeightCount > 0 );
				check( InParams.TotalWeightCount <= MeshPaintDefs::MaxSupportedWeights );

				// True if we should assume the last weight index is composed of one minus the sum of all
				// of the other weights.  This effectively allows an additional weight with no extra memory
				// used, but potentially requires extra pixel shader instructions to render.
				//
				// NOTE: If you change the default here, remember to update the MeshPaintWindow UI and strings
				//
				// NOTE: Materials must be authored to match the following assumptions!
				const UBOOL bUsingOneMinusTotal =
					InParams.TotalWeightCount == 2 ||		// Two textures: Use a lerp() in pixel shader (single value)
					InParams.TotalWeightCount == 5;			// Five texture: Requires 1.0-sum( R+G+B+A ) in shader
				check( bUsingOneMinusTotal || InParams.TotalWeightCount <= MeshPaintDefs::MaxSupportedPhysicalWeights );

				// Prefer to use RG/RGB instead of AR/ARG when we're only using 2/3 physical weights
				const INT TotalPhysicalWeights = bUsingOneMinusTotal ? InParams.TotalWeightCount - 1 : InParams.TotalWeightCount;
				const UBOOL bUseColorAlpha =
					TotalPhysicalWeights != 2 &&			// Two physical weights: Use RG instead of AR
					TotalPhysicalWeights != 3;				// Three physical weights: Use RGB instead of ARG

				// Index of the blend weight that we're painting
				check( InParams.PaintWeightIndex >= 0 && InParams.PaintWeightIndex < MeshPaintDefs::MaxSupportedWeights );


				// Convert the color value to an array of weights
				FLOAT Weights[ MeshPaintDefs::MaxSupportedWeights ];
				{
					for( INT CurWeightIndex = 0; CurWeightIndex < InParams.TotalWeightCount; ++CurWeightIndex )
					{											  
						if( CurWeightIndex == TotalPhysicalWeights )
						{
							// This weight's value is one minus the sum of all previous weights
							FLOAT OtherWeightsTotal = 0.0f;
							for( INT OtherWeightIndex = 0; OtherWeightIndex < CurWeightIndex; ++OtherWeightIndex )
							{
								OtherWeightsTotal += Weights[ OtherWeightIndex ];
							}
							Weights[ CurWeightIndex ] = 1.0f - OtherWeightsTotal;
						}
						else
						{
							switch( CurWeightIndex )
							{
								case 0:
									Weights[ CurWeightIndex ] = bUseColorAlpha ? OldColor.A : OldColor.R;
									break;

								case 1:
									Weights[ CurWeightIndex ] = bUseColorAlpha ? OldColor.R : OldColor.G;
									break;

								case 2:
									Weights[ CurWeightIndex ] = bUseColorAlpha ? OldColor.G : OldColor.B;
									break;

								case 3:
									check( bUseColorAlpha );
									Weights[ CurWeightIndex ] = OldColor.B;
									break;

								default:
									appErrorf( TEXT( "Invalid weight index" ) );
									break;
							}
						}
					}
				}
				

				// Go ahead any apply paint!
				{
					Weights[ InParams.PaintWeightIndex ] += PaintAmount;
					Weights[ InParams.PaintWeightIndex ] = Clamp( Weights[ InParams.PaintWeightIndex ], 0.0f, 1.0f );
				}


				// Now renormalize all of the other weights
				{
					FLOAT OtherWeightsTotal = 0.0f;
					for( INT CurWeightIndex = 0; CurWeightIndex < InParams.TotalWeightCount; ++CurWeightIndex )
					{
						if( CurWeightIndex != InParams.PaintWeightIndex )
						{
							OtherWeightsTotal += Weights[ CurWeightIndex ];
						}
					}
					const FLOAT NormalizeTarget = 1.0f - Weights[ InParams.PaintWeightIndex ];
					for( INT CurWeightIndex = 0; CurWeightIndex < InParams.TotalWeightCount; ++CurWeightIndex )
					{
						if( CurWeightIndex != InParams.PaintWeightIndex )
						{
							if( OtherWeightsTotal == 0.0f )
							{
								Weights[ CurWeightIndex ] = NormalizeTarget / ( InParams.TotalWeightCount - 1 );
							}
							else
							{
								Weights[ CurWeightIndex ] = Weights[ CurWeightIndex ] / OtherWeightsTotal * NormalizeTarget;
							}
						}
					}
				}


				// The total of the weights should now always equal 1.0
				{
					FLOAT WeightsTotal = 0.0f;
					for( INT CurWeightIndex = 0; CurWeightIndex < InParams.TotalWeightCount; ++CurWeightIndex )
					{
						WeightsTotal += Weights[ CurWeightIndex ];
					}
					check( appIsNearlyEqual( WeightsTotal, 1.0f, 0.01f ) );
				}


				// Convert the weights back to a color value					
				{
					for( INT CurWeightIndex = 0; CurWeightIndex < InParams.TotalWeightCount; ++CurWeightIndex )
					{
						// We can skip the non-physical weights as it's already baked into the others
						if( CurWeightIndex != TotalPhysicalWeights )
						{
							switch( CurWeightIndex )
							{
								case 0:
									if( bUseColorAlpha )
									{
										NewColor.A = Weights[ CurWeightIndex ];
									}
									else
									{
										NewColor.R = Weights[ CurWeightIndex ];
									}
									break;

								case 1:
									if( bUseColorAlpha )
									{
										NewColor.R = Weights[ CurWeightIndex ];
									}
									else
									{
										NewColor.G = Weights[ CurWeightIndex ];
									}
									break;

								case 2:
									if( bUseColorAlpha )
									{
										NewColor.G = Weights[ CurWeightIndex ];
									}
									else
									{
										NewColor.B = Weights[ CurWeightIndex ];
									}
									break;

								case 3:
									NewColor.B = Weights[ CurWeightIndex ];
									break;

								default:
									appErrorf( TEXT( "Invalid weight index" ) );
									break;
							}
						}
					}
				}
				
			}




			// Save the new color
			InOutVertexColor.R = Clamp( appTrunc( NewColor.R * 255.0f ), 0, 255 );
			InOutVertexColor.G = Clamp( appTrunc( NewColor.G * 255.0f ), 0, 255 );
			InOutVertexColor.B = Clamp( appTrunc( NewColor.B * 255.0f ), 0, 255 );
			InOutVertexColor.A = Clamp( appTrunc( NewColor.A * 255.0f ), 0, 255 );


			// debugf( TEXT( "Painted Vertex:  OldColor=[%.2f,%.2f,%.2f,%.2f], NewColor=[%.2f,%.2f,%.2f,%.2f]" ), OldColor.R, OldColor.G, OldColor.B, OldColor.A, NewColor.R, NewColor.G, NewColor.B, NewColor.A );
		}

		return TRUE;
	}


	// Out of range
	return FALSE;
}

/* Builds a temporary kDOP tree for doing line checks on meshes without collision */
static void BuildTempKDOPTree( UStaticMesh* Mesh, FEdModeMeshPaint::kDOPTreeType& OutKDOPTree )
{
	// First LOD is the only lod supported for mesh paint.
	FStaticMeshRenderData& LODModel = Mesh->LODModels( 0 );
	TArray<FkDOPBuildCollisionTriangle<WORD> > kDOPBuildTriangles;

	// Get the kDOP triangles from the lod model and build the tree
	LODModel.GetKDOPTriangles( kDOPBuildTriangles );
	OutKDOPTree.Build( kDOPBuildTriangles );
}


/** Paint the mesh that impacts the specified ray */
void FEdModeMeshPaint::DoPaint( const FVector& InCameraOrigin,
							    const FVector& InRayOrigin,
							    const FVector& InRayDirection,
								FPrimitiveDrawInterface* PDI,
								const EMeshPaintAction::Type InPaintAction,
								const UBOOL bVisualCueOnly,
								const FLOAT InStrengthScale,
								OUT UBOOL& bAnyPaintAbleActorsUnderCursor)
{
	const FLOAT BrushRadius = FMeshPaintSettings::Get().BrushRadius;

	// Fire out a ray to see if there is a *selected* static mesh under the mouse cursor.
	// NOTE: We can't use a GWorld line check for this as that would ignore actors that have collision disabled
	TArray< AActor* > PaintableActors;
	FCheckResult BestTraceResult;
	{
		const FVector TraceStart( InRayOrigin );
		const FVector TraceEnd( InRayOrigin + InRayDirection * HALF_WORLD_MAX );

		// Iterate over selected actors looking for static meshes
		USelection& SelectedActors = *GEditor->GetSelectedActors();
		TArray< AActor* > ValidSelectedActors;
		for( INT CurSelectedActorIndex = 0; CurSelectedActorIndex < SelectedActors.Num(); ++CurSelectedActorIndex )
		{
			UBOOL bHasKDOPTree = TRUE;
			UBOOL bCurActorIsValid = FALSE;
			AActor* CurActor = CastChecked< AActor >( SelectedActors( CurSelectedActorIndex ) );

			// No matter the actor type, disregard hidden or non-selected actors
			if ( CurActor->bHidden || !CurActor->IsSelected() )
			{
				continue;
			}
			const UBOOL bOldCollideActors = CurActor->bCollideActors;
			UBOOL bOldCollideActorsComponent = FALSE;

			UStaticMesh* CurStaticMesh = NULL;
			UStaticMeshComponent* CurStaticMeshComponent = NULL;

			// Is this a static mesh actor?
			AStaticMeshActor* StaticMeshActor = Cast< AStaticMeshActor >( CurActor );
			if( StaticMeshActor != NULL &&
				StaticMeshActor->StaticMeshComponent != NULL &&
				StaticMeshActor->StaticMeshComponent->StaticMesh != NULL )
			{
				bCurActorIsValid = TRUE;

				CurStaticMesh = StaticMeshActor->StaticMeshComponent->StaticMesh;
				CurStaticMeshComponent = StaticMeshActor->StaticMeshComponent;
				bOldCollideActorsComponent = StaticMeshActor->StaticMeshComponent->CollideActors;

				if( CurStaticMesh->kDOPTree.Triangles.Num() == 0 )
				{
					// This mesh does not have a valid kDOP tree so the line check will fail. 
					// We will build a temporary one so we can paint on meshes without collision
					bHasKDOPTree = FALSE;

					// See if the mesh already has a kDOP tree we previously built.  
					kDOPTreeType* Tree = StaticMeshToTempkDOPMap.Find( CurStaticMesh );
					if( !Tree )
					{
						// We need to build a new kDOP tree
						kDOPTreeType kDOPTree;
						BuildTempKDOPTree( CurStaticMesh, kDOPTree);
						// Set the mapping between the current static mesh and the new kDOP tree.  
						// This will avoid having to rebuild the tree over and over
						StaticMeshToTempkDOPMap.Set( CurStaticMesh, kDOPTree );
					}
				}

				// Line check requires that the static mesh component and its owner are both marked to collide with actors, so
				// here we temporarily force the collision enabled if it's not already. Note we do not use SetCollision because
				// there is no reason to incur the cost of component reattaches when the collision value will be restored back to
				// what it was momentarily.
				if ( !bOldCollideActors || !bOldCollideActorsComponent )
				{
					CurActor->bCollideActors = TRUE;
					CurStaticMeshComponent->CollideActors = TRUE;
				}
			}
			// If this wasn't a static mesh actor, is it a dynamic static mesh actor?
			else
			{
				ADynamicSMActor* DynamicSMActor = Cast< ADynamicSMActor >( CurActor );
				if ( DynamicSMActor != NULL &&
					 DynamicSMActor->StaticMeshComponent != NULL &&
					 DynamicSMActor->StaticMeshComponent->StaticMesh != NULL )
				{
					bCurActorIsValid = TRUE;

					CurStaticMesh = DynamicSMActor->StaticMeshComponent->StaticMesh;
					CurStaticMeshComponent = DynamicSMActor->StaticMeshComponent;
					bOldCollideActorsComponent = DynamicSMActor->StaticMeshComponent->CollideActors;

					if( CurStaticMesh->kDOPTree.Triangles.Num() == 0 )
					{
						// This mesh does not have a valid kDOP tree so the line check will fail. 
						// We will build a temporary one so we can paint on meshes without collision
						bHasKDOPTree = FALSE;

						// See if the mesh already has a kDOP tree we previously built.  
						kDOPTreeType* Tree = StaticMeshToTempkDOPMap.Find( CurStaticMesh );
						if( !Tree )
						{
							// We need to build a new kDOP tree
							kDOPTreeType kDOPTree;
							BuildTempKDOPTree( CurStaticMesh, kDOPTree);
							// Set the mapping between the current static mesh and the new kDOP tree.  
							// This will avoid having to rebuild the tree over and over
							StaticMeshToTempkDOPMap.Set( CurStaticMesh, kDOPTree );
						}
					}

					// Line check requires that the static mesh component and its owner are both marked to collide with actors, so
					// here we temporarily force the collision enabled if it's not already. Note we do not use SetCollision because
					// there is no reason to incur the cost of component reattaches when the collision value will be restored back to
					// what it was momentarily.
					if ( !bOldCollideActors || !bOldCollideActorsComponent )
					{
						CurActor->bCollideActors = TRUE;
						CurStaticMeshComponent->CollideActors = TRUE;
					}
				}
			}

			if ((InPaintAction == EMeshPaintAction::Fill) && (CurStaticMesh))
			{
				PaintableActors.AddItem( CurActor );
				continue;
			}
			else if ((InPaintAction == EMeshPaintAction::PushInstanceColorsToMesh) && CurStaticMesh && (SelectedActors.Num() == 1))
			{
				PaintableActors.AddItem( CurActor );
				break;
			}

			// If the actor was a static mesh or dynamic static mesh actor, it's potentially valid for mesh painting
			if ( bCurActorIsValid )
			{
				ValidSelectedActors.AddItem( CurActor );

				// Ray trace
				FCheckResult TestTraceResult( 1.0f );
				const FVector TraceExtent( 0.0f, 0.0f, 0.0f );

				kDOPTreeType OldTree;

				if( !bHasKDOPTree )
				{
					// Temporarily replace the current static meshes kDOP tree with one we built.  This will ensure we get good results from the line check
					OldTree = CurStaticMesh->kDOPTree;
					kDOPTreeType* kDOPTree = StaticMeshToTempkDOPMap.Find( CurStaticMesh );
					if( kDOPTree )
					{
						CurStaticMesh->kDOPTree = *kDOPTree;
					}
				}

				if( !CurActor->ActorLineCheck( TestTraceResult, TraceEnd, TraceStart, TraceExtent, TRACE_ComplexCollision | TRACE_Accurate | TRACE_Visible ) )
				{
					// Find the closest impact
					if( BestTraceResult.Actor == NULL || ( TestTraceResult.Time < BestTraceResult.Time ) )
					{
						BestTraceResult = TestTraceResult;
					}
				}

				if( !bHasKDOPTree )
				{
					// Replace the kDOP tree we built with static meshes actual one.
					CurStaticMesh->kDOPTree = OldTree;
				}
				
				// Restore the old collision values for the actor and component if they were forcibly changed to support mesh painting
				if ( !bOldCollideActors || !bOldCollideActorsComponent )
				{
					CurActor->bCollideActors = bOldCollideActors;
					CurStaticMeshComponent->CollideActors = bOldCollideActorsComponent;
				}

			}
		}

		if( BestTraceResult.Actor != NULL )
		{
			// If we're using texture paint, just use the best trace result we found as we currently only
			// support painting a single mesh at a time in that mode.
			if( FMeshPaintSettings::Get().ResourceType == EMeshPaintResource::Texture )
			{
				PaintableActors.AddItem( BestTraceResult.Actor );
			}
			else
			{
				FBox BrushBounds = FBox::BuildAABB( BestTraceResult.Location, FVector( BrushRadius * 1.25f, BrushRadius * 1.25f, BrushRadius * 1.25f ) );

				// Vertex paint mode, so we want all valid actors overlapping the brush
				for( INT CurActorIndex = 0; CurActorIndex < ValidSelectedActors.Num(); ++CurActorIndex )
				{
					AActor* CurValidActor = ValidSelectedActors( CurActorIndex );

					const FBox ActorBounds = CurValidActor->GetComponentsBoundingBox( TRUE );
					
					if( ActorBounds.Intersect( BrushBounds ) )
					{
						// OK, this mesh potentially overlaps the brush!
						PaintableActors.AddItem( CurValidActor );
					}
				}
			}
		}
	}

	// Iterate over the selected static meshes under the cursor and paint them!
	for( INT CurActorIndex = 0; CurActorIndex < PaintableActors.Num(); ++CurActorIndex )
	{
		AStaticMeshActor* HitStaticMeshActor = Cast<AStaticMeshActor>( PaintableActors( CurActorIndex ) );
		ADynamicSMActor* HitDynamicSMActor = Cast<ADynamicSMActor> ( PaintableActors( CurActorIndex ) );
		check( HitStaticMeshActor || HitDynamicSMActor );

		UStaticMeshComponent* StaticMeshComponent = HitStaticMeshActor ? HitStaticMeshActor->StaticMeshComponent : HitDynamicSMActor->StaticMeshComponent;
		UStaticMesh* StaticMesh = StaticMeshComponent->StaticMesh;

		check( StaticMesh->LODModels.Num() > PaintingMeshLODIndex );
		FStaticMeshRenderData& LODModel = StaticMeshComponent->StaticMesh->LODModels( PaintingMeshLODIndex );
		
		// Brush properties
		const FLOAT BrushDepth = FMeshPaintSettings::Get().BrushRadius;	// NOTE: Actually half of the total depth (like a radius)
		const FLOAT BrushFalloffAmount = FMeshPaintSettings::Get().BrushFalloffAmount;
		const FLinearColor BrushColor = ((InPaintAction == EMeshPaintAction::Paint) || (InPaintAction == EMeshPaintAction::Fill))? FMeshPaintSettings::Get().PaintColor : FMeshPaintSettings::Get().EraseColor;

		// NOTE: We square the brush strength to maximize slider precision in the low range
		const FLOAT BrushStrength =
			FMeshPaintSettings::Get().BrushStrength * FMeshPaintSettings::Get().BrushStrength *
			InStrengthScale;

		// Display settings
		const FLOAT VisualBiasDistance = 0.15f;
		const FLOAT NormalLineSize( BrushRadius * 0.35f );	// Make the normal line length a function of brush size
		const FLinearColor NormalLineColor( 0.3f, 1.0f, 0.3f );
		const FLinearColor BrushCueColor = bIsPainting ? FLinearColor( 1.0f, 1.0f, 0.3f ) : FLinearColor( 0.3f, 1.0f, 0.3f );
		const FLinearColor InnerBrushCueColor = bIsPainting ? FLinearColor( 0.5f, 0.5f, 0.1f ) : FLinearColor( 0.1f, 0.5f, 0.1f );

		FVector BrushXAxis, BrushYAxis;
		BestTraceResult.Normal.FindBestAxisVectors( BrushXAxis, BrushYAxis );
		const FVector BrushVisualPosition = BestTraceResult.Location + BestTraceResult.Normal * VisualBiasDistance;


		// Precache model -> world transform
		const FMatrix ActorToWorldMatrix = HitStaticMeshActor ? HitStaticMeshActor->LocalToWorld() : HitDynamicSMActor->LocalToWorld();


		// Compute the camera position in actor space.  We need this later to check for
		// backfacing triangles.
		const FVector ActorSpaceCameraPosition( ActorToWorldMatrix.InverseTransformFVector( InCameraOrigin ) );
		const FVector ActorSpaceBrushPosition( ActorToWorldMatrix.InverseTransformFVector( BestTraceResult.Location ) );
		
		// @todo MeshPaint: Input vector doesn't work well with non-uniform scale
		const FLOAT ActorSpaceBrushRadius = ActorToWorldMatrix.InverseTransformNormal( FVector( BrushRadius, 0.0f, 0.0f ) ).Size();
		const FLOAT ActorSpaceSquaredBrushRadius = ActorSpaceBrushRadius * ActorSpaceBrushRadius;


		if( PDI != NULL )
		{
			// Draw brush circle
			const INT NumCircleSides = 64;
			DrawCircle( PDI, BrushVisualPosition, BrushXAxis, BrushYAxis, BrushCueColor, BrushRadius, NumCircleSides, SDPG_World );

			// Also draw the inner brush radius
			const FLOAT InnerBrushRadius = BrushRadius - BrushFalloffAmount * BrushRadius;
			DrawCircle( PDI, BrushVisualPosition, BrushXAxis, BrushYAxis, InnerBrushCueColor, InnerBrushRadius, NumCircleSides, SDPG_World );

			// If we just started painting then also draw a little brush effect
			if( bIsPainting )
			{
				const FLOAT EffectDuration = 0.2f;

				const DOUBLE CurTime = appSeconds();
				const FLOAT TimeSinceStartedPainting = (FLOAT)( CurTime - PaintingStartTime );
				if( TimeSinceStartedPainting <= EffectDuration )
				{
					// Invert the effect if we're currently erasing
					FLOAT EffectAlpha = TimeSinceStartedPainting / EffectDuration;
					if( InPaintAction == EMeshPaintAction::Erase )
					{
						EffectAlpha = 1.0f - EffectAlpha;
					}

					const FLinearColor EffectColor( 0.1f + EffectAlpha * 0.4f, 0.1f + EffectAlpha * 0.4f, 0.1f + EffectAlpha * 0.4f );
					const FLOAT EffectRadius = BrushRadius * EffectAlpha * EffectAlpha;	// Squared curve here (looks more interesting)
					DrawCircle( PDI, BrushVisualPosition, BrushXAxis, BrushYAxis, EffectColor, EffectRadius, NumCircleSides, SDPG_World );
				}
			}

			// Draw trace surface normal
			const FVector NormalLineEnd( BrushVisualPosition + BestTraceResult.Normal * NormalLineSize );
			PDI->DrawLine( BrushVisualPosition, NormalLineEnd, NormalLineColor, SDPG_World );
		}



		// Mesh paint settings
		FMeshPaintParameters Params;
		{
			Params.PaintMode = FMeshPaintSettings::Get().PaintMode;
			Params.PaintAction = InPaintAction;
			Params.BrushPosition = BestTraceResult.Location;
			Params.BrushNormal = BestTraceResult.Normal;
			Params.BrushColor = BrushColor;
			Params.SquaredBrushRadius = BrushRadius * BrushRadius;
			Params.BrushRadialFalloffRange = BrushFalloffAmount * BrushRadius;
			Params.InnerBrushRadius = BrushRadius - Params.BrushRadialFalloffRange;
			Params.BrushDepth = BrushDepth;
			Params.BrushDepthFalloffRange = BrushFalloffAmount * BrushDepth;
			Params.InnerBrushDepth = BrushDepth - Params.BrushDepthFalloffRange;
			Params.BrushStrength = BrushStrength;
			Params.BrushToWorldMatrix = FMatrix( BrushXAxis, BrushYAxis, Params.BrushNormal, Params.BrushPosition );
			Params.bWriteRed = FMeshPaintSettings::Get().bWriteRed;
			Params.bWriteGreen = FMeshPaintSettings::Get().bWriteGreen;
			Params.bWriteBlue = FMeshPaintSettings::Get().bWriteBlue;
			Params.bWriteAlpha = FMeshPaintSettings::Get().bWriteAlpha;
			Params.TotalWeightCount = FMeshPaintSettings::Get().TotalWeightCount;

			// Select texture weight index based on whether or not we're painting or erasing
			{
				const INT PaintWeightIndex = 
					( InPaintAction == EMeshPaintAction::Paint ) ? FMeshPaintSettings::Get().PaintWeightIndex : FMeshPaintSettings::Get().EraseWeightIndex;

				// Clamp the weight index to fall within the total weight count
				Params.PaintWeightIndex = Clamp( PaintWeightIndex, 0, Params.TotalWeightCount - 1 );
			}

			// @todo MeshPaint: Ideally we would default to: TexturePaintingStaticMeshComponent->StaticMesh->LightMapCoordinateIndex
			//		Or we could indicate in the GUI which channel is the light map set (button to set it?)
			Params.UVChannel = FMeshPaintSettings::Get().UVChannel;
		}

		// Are we actually applying paint here?
		const UBOOL bShouldApplyPaint = (bIsPainting && !bVisualCueOnly) || 
			(InPaintAction == EMeshPaintAction::Fill) || 
			(InPaintAction == EMeshPaintAction::PushInstanceColorsToMesh);

		if( FMeshPaintSettings::Get().ResourceType == EMeshPaintResource::VertexColors )
		{
			// Painting vertex colors
			PaintMeshVertices( StaticMeshComponent, Params, bShouldApplyPaint, LODModel, ActorSpaceCameraPosition, ActorToWorldMatrix, PDI, VisualBiasDistance);

		}
		else
		{
			// Painting textures
			PaintMeshTexture( StaticMeshComponent, Params, bShouldApplyPaint, LODModel, ActorSpaceCameraPosition, ActorToWorldMatrix, ActorSpaceSquaredBrushRadius, ActorSpaceBrushPosition );

		}
	}

	bAnyPaintAbleActorsUnderCursor = (PaintableActors.Num() > 0);
}



/** Paints mesh vertices */
void FEdModeMeshPaint::PaintMeshVertices( 
	UStaticMeshComponent* StaticMeshComponent, 
	const FMeshPaintParameters& Params, 
	const UBOOL bShouldApplyPaint, 
	FStaticMeshRenderData& LODModel, 
	const FVector& ActorSpaceCameraPosition, 
	const FMatrix& ActorToWorldMatrix, 
	FPrimitiveDrawInterface* PDI, 
	const FLOAT VisualBiasDistance)
{
	const UBOOL bOnlyFrontFacing = FMeshPaintSettings::Get().bOnlyFrontFacingTriangles;
	const UBOOL bUsingInstancedVertexColors = ( FMeshPaintSettings::Get().VertexPaintTarget == EMeshVertexPaintTarget::ComponentInstance ) && (Params.PaintAction != EMeshPaintAction::PushInstanceColorsToMesh);

	const FLOAT InfluencedVertexCuePointSize = 3.5f;

	UStaticMesh* StaticMesh = StaticMeshComponent->StaticMesh;


	// Paint the mesh
	UINT NumVerticesInfluencedByBrush = 0;
	{
		TScopedPointer< FStaticMeshComponentReattachContext > MeshComponentReattachContext;
		TScopedPointer< FComponentReattachContext > ComponentReattachContext;


		FStaticMeshComponentLODInfo* InstanceMeshLODInfo = NULL;
		if( bUsingInstancedVertexColors)
		{
			if( bShouldApplyPaint )
			{
				// We're only changing instanced vertices on this specific mesh component, so we
				// only need to detach our mesh component
				ComponentReattachContext.Reset( new FComponentReattachContext( StaticMeshComponent ) );

				// Mark the mesh component as modified
				StaticMeshComponent->Modify();

				// Ensure LODData has enough entries in it, free not required.
				StaticMeshComponent->SetLODDataCount(PaintingMeshLODIndex + 1, StaticMeshComponent->LODData.Num());

				InstanceMeshLODInfo = &StaticMeshComponent->LODData( PaintingMeshLODIndex );

				// Destroy the instance vertex  color array if it doesn't fit
				if(InstanceMeshLODInfo->OverrideVertexColors
					&& InstanceMeshLODInfo->OverrideVertexColors->GetNumVertices() != LODModel.NumVertices)
				{
					InstanceMeshLODInfo->ReleaseOverrideVertexColorsAndBlock();
				}

				if(InstanceMeshLODInfo->OverrideVertexColors)
				{
					InstanceMeshLODInfo->BeginReleaseOverrideVertexColors();
					FlushRenderingCommands();
				}
				else
				{
					// Setup the instance vertex color array if we don't have one yet
					InstanceMeshLODInfo->OverrideVertexColors = new FColorVertexBuffer;

					if(LODModel.ColorVertexBuffer.GetNumVertices() >= LODModel.NumVertices)
					{
						// copy mesh vertex colors to the instance ones
						InstanceMeshLODInfo->OverrideVertexColors->InitFromColorArray(&LODModel.ColorVertexBuffer.VertexColor(0), LODModel.NumVertices);
					}
					else
					{
						UBOOL bConvertSRGB = FALSE;
						FColor FillColor = Params.BrushColor.ToFColor(bConvertSRGB);
						// Original mesh didn't have any colors, so just use a default color
						InstanceMeshLODInfo->OverrideVertexColors->InitFromSingleColor(FColor(255, 255, 255), LODModel.NumVertices);
					}

					// The instance vertex color byte count has changed so tell the mesh paint window
					// to refresh it's properties
#if WITH_MANAGED_CODE
					MeshPaintWindow->RefreshAllProperties();
#endif
				}
				// See if the component has to cache its mesh vertex positions associated with override colors
				StaticMeshComponent->CacheMeshVertexPositionsIfNecessary();
			}
			else
			{
				if( StaticMeshComponent->LODData.Num() > PaintingMeshLODIndex )
				{
					InstanceMeshLODInfo = &StaticMeshComponent->LODData( PaintingMeshLODIndex );
				}
			}
		}
		else
		{
			if( bShouldApplyPaint )
			{
				// We're changing the mesh itself, so ALL static mesh components in the scene will need
				// to be detached for this (and reattached afterwards.)
				MeshComponentReattachContext.Reset( new FStaticMeshComponentReattachContext( StaticMesh ) );

				// Dirty the mesh
				StaticMesh->Modify();

				// Add to our modified list
				ModifiedStaticMeshes.AddUniqueItem( StaticMesh );

				// Release the static mesh's resources.
				StaticMesh->ReleaseResources();

				// Flush the resource release commands to the rendering thread to ensure that the build doesn't occur while a resource is still
				// allocated, and potentially accessing the UStaticMesh.
				StaticMesh->ReleaseResourcesFence.Wait();
			}
		}



		// Paint the mesh vertices
		{
			if (Params.PaintAction == EMeshPaintAction::Fill)
			{
				//flood fill
				UBOOL bConvertSRGB = FALSE;
				FColor FillColor = Params.BrushColor.ToFColor(bConvertSRGB);
				FColor NewMask = FColor(Params.bWriteRed ? 255 : 0, Params.bWriteGreen ? 255 : 0, Params.bWriteBlue ? 255 : 0, Params.bWriteAlpha ? 255 : 0);
				FColor KeepMaskColor (~NewMask.DWColor());

				FColor MaskedFillColor = FillColor;
				MaskedFillColor.R &= NewMask.R;
				MaskedFillColor.G &= NewMask.G;
				MaskedFillColor.B &= NewMask.B;
				MaskedFillColor.A &= NewMask.A;

				//make sure there is room if we're painting on the source mesh
				if( !bUsingInstancedVertexColors && LODModel.ColorVertexBuffer.GetNumVertices() == 0 )
				{
					// Mesh doesn't have a color vertex buffer yet!  We'll create one now.
					LODModel.ColorVertexBuffer.InitFromSingleColor(FColor( 255, 255, 255, 255), LODModel.NumVertices);
				}

				for (UINT ColorIndex = 0; ColorIndex < LODModel.NumVertices; ++ColorIndex)
				{
					FColor CurrentColor;
					if( bUsingInstancedVertexColors )
					{
						check(InstanceMeshLODInfo->OverrideVertexColors);
						check((UINT)ColorIndex < InstanceMeshLODInfo->OverrideVertexColors->GetNumVertices());

						CurrentColor = InstanceMeshLODInfo->OverrideVertexColors->VertexColor( ColorIndex );
					}
					else
					{
						CurrentColor = LODModel.ColorVertexBuffer.VertexColor( ColorIndex );
					}

					CurrentColor.R &= KeepMaskColor.R;
					CurrentColor.G &= KeepMaskColor.G;
					CurrentColor.B &= KeepMaskColor.B;
					CurrentColor.A &= KeepMaskColor.A;
					CurrentColor += MaskedFillColor;

					if( bUsingInstancedVertexColors )
					{
						InstanceMeshLODInfo->OverrideVertexColors->VertexColor( ColorIndex ) = CurrentColor;
					}
					else
					{
						LODModel.ColorVertexBuffer.VertexColor( ColorIndex ) = CurrentColor;
					}
				}
				GCallbackEvent->Send( CALLBACK_RedrawAllViewports );
			}
			else if (Params.PaintAction == EMeshPaintAction::PushInstanceColorsToMesh)
			{
				InstanceMeshLODInfo = &StaticMeshComponent->LODData( PaintingMeshLODIndex );

				UINT LODColorNum = LODModel.ColorVertexBuffer.GetNumVertices();
				UINT InstanceColorNum = InstanceMeshLODInfo->OverrideVertexColors ? InstanceMeshLODInfo->OverrideVertexColors->GetNumVertices() : 0;

				if ((InstanceColorNum > 0) && 
					((LODColorNum == InstanceColorNum) || (LODColorNum == 0)))
				{
					LODModel.ColorVertexBuffer.InitFromColorArray(&InstanceMeshLODInfo->OverrideVertexColors->VertexColor(0), InstanceColorNum);
				}

				RemoveInstanceVertexColors();

				GCallbackEvent->Send( CALLBACK_RedrawAllViewports );
			}
			else
			{
				// @todo MeshPaint: Use a spatial database to reduce the triangle set here (kdop)


				// Make sure we're dealing with triangle lists
				const INT NumIndexBufferIndices = LODModel.IndexBuffer.Indices.Num();
				check( NumIndexBufferIndices % 3 == 0 );

				// We don't want to paint the same vertex twice and many vertices are shared between
				// triangles, so we use a set to track unique front-facing vertex indices
				static TSet< INT > FrontFacingVertexIndices;
				FrontFacingVertexIndices.Empty( NumIndexBufferIndices );


				// For each triangle in the mesh
				const INT NumTriangles = NumIndexBufferIndices / 3;
				for( INT TriIndex = 0; TriIndex < NumTriangles; ++TriIndex )
				{
					// Grab the vertex indices and points for this triangle
					INT VertexIndices[ 3 ];
					FVector TriVertices[ 3 ];
					for( INT TriVertexNum = 0; TriVertexNum < 3; ++TriVertexNum )
					{
						VertexIndices[ TriVertexNum ] = LODModel.IndexBuffer.Indices( TriIndex * 3 + TriVertexNum );
						TriVertices[ TriVertexNum ] = LODModel.PositionVertexBuffer.VertexPosition( VertexIndices[ TriVertexNum ] );
					}

					// Check to see if the triangle is front facing
					FVector TriangleNormal = ( TriVertices[ 1 ] - TriVertices[ 0 ] ^ TriVertices[ 2 ] - TriVertices[ 0 ] ).SafeNormal();
					const FLOAT SignedPlaneDist = FPointPlaneDist( ActorSpaceCameraPosition, TriVertices[ 0 ], TriangleNormal );
					if( !bOnlyFrontFacing || SignedPlaneDist < 0.0f )
					{
						FrontFacingVertexIndices.Add( VertexIndices[ 0 ] );
						FrontFacingVertexIndices.Add( VertexIndices[ 1 ] );
						FrontFacingVertexIndices.Add( VertexIndices[ 2 ] );
					}
				}


				for( TSet< INT >::TConstIterator CurIndexIt( FrontFacingVertexIndices ); CurIndexIt != NULL; ++CurIndexIt )
				{
					// Grab the mesh vertex and transform it to world space
					const INT VertexIndex = *CurIndexIt;
					const FVector& ModelSpaceVertexPosition = LODModel.PositionVertexBuffer.VertexPosition( VertexIndex );
					FVector WorldSpaceVertexPosition = ActorToWorldMatrix.TransformFVector( ModelSpaceVertexPosition );

					FColor OriginalVertexColor = FColor( 255, 255, 255 );

					// Grab vertex color (read/write)
					if( bUsingInstancedVertexColors )
					{
						if( InstanceMeshLODInfo 
						&& InstanceMeshLODInfo->OverrideVertexColors
						&& InstanceMeshLODInfo->OverrideVertexColors->GetNumVertices() == LODModel.NumVertices )
						{
							// Actor mesh component LOD
							OriginalVertexColor = InstanceMeshLODInfo->OverrideVertexColors->VertexColor( VertexIndex );
						}
					}
					else
					{
						// Static mesh
						if( bShouldApplyPaint )
						{
							if( LODModel.ColorVertexBuffer.GetNumVertices() == 0 )
							{
								// Mesh doesn't have a color vertex buffer yet!  We'll create one now.
								LODModel.ColorVertexBuffer.InitFromSingleColor(FColor( 255, 255, 255, 255), LODModel.NumVertices);

								// @todo MeshPaint: Make sure this is the best place to do this
								BeginInitResource( &LODModel.ColorVertexBuffer );
							}
						}

						if( LODModel.ColorVertexBuffer.GetNumVertices() > 0 )
						{
							check( (INT)LODModel.ColorVertexBuffer.GetNumVertices() > VertexIndex );
							OriginalVertexColor = LODModel.ColorVertexBuffer.VertexColor( VertexIndex );
						}
					}


					// Paint the vertex!
					FColor NewVertexColor = OriginalVertexColor;
					UBOOL bVertexInRange = FALSE;
					{
						FColor PaintedVertexColor = OriginalVertexColor;
						bVertexInRange = PaintVertex( WorldSpaceVertexPosition, Params, TRUE, PaintedVertexColor );

						if( bShouldApplyPaint )
						{
							NewVertexColor = PaintedVertexColor;
						}
					}


					if( bVertexInRange )
					{
						++NumVerticesInfluencedByBrush;

						// Update the mesh!
						if( bShouldApplyPaint )
						{
							if( bUsingInstancedVertexColors )
							{
								check(InstanceMeshLODInfo->OverrideVertexColors);
								check((UINT)VertexIndex < InstanceMeshLODInfo->OverrideVertexColors->GetNumVertices());

								InstanceMeshLODInfo->OverrideVertexColors->VertexColor( VertexIndex ) = NewVertexColor;
							}
							else
							{
								LODModel.ColorVertexBuffer.VertexColor( VertexIndex ) = NewVertexColor;
							}
						}


						// Draw vertex visual cue
						if( PDI != NULL )
						{
							const FLinearColor InfluencedVertexCueColor( NewVertexColor );
							const FVector VertexVisualPosition = WorldSpaceVertexPosition + Params.BrushNormal * VisualBiasDistance;
							PDI->DrawPoint( VertexVisualPosition, InfluencedVertexCueColor, InfluencedVertexCuePointSize, SDPG_World );
						}
					}
				} 
			}
		}

		if( bShouldApplyPaint )
		{
			if( bUsingInstancedVertexColors )
			{
				BeginInitResource(InstanceMeshLODInfo->OverrideVertexColors);
			}
			else
			{
				// Reinitialize the static mesh's resources.
				StaticMesh->InitResources();
			}
		}
	}


	// Were any vertices in the brush's influence?
	if( NumVerticesInfluencedByBrush > 0 )
	{
		// Also paint raw vertices
		const INT RawTriangleCount = LODModel.RawTriangles.GetElementCount();
		FStaticMeshTriangle* RawTriangleData =
			(FStaticMeshTriangle*)LODModel.RawTriangles.Lock( bShouldApplyPaint ? LOCK_READ_WRITE : LOCK_READ_ONLY );

		// @todo MeshPaint: Ideally we could reduce the triangle set here using a spatial database
		for( INT RawTriangleIndex = 0; RawTriangleIndex < RawTriangleCount; ++RawTriangleIndex )
		{
			FStaticMeshTriangle& CurRawTriangle = RawTriangleData[ RawTriangleIndex ];


			// Check to see if the triangle is front facing
			FVector TriangleNormal = ( CurRawTriangle.Vertices[ 1 ] - CurRawTriangle.Vertices[ 0 ] ^ CurRawTriangle.Vertices[ 2 ] - CurRawTriangle.Vertices[ 0 ] ).SafeNormal();
			const FLOAT SignedPlaneDist = FPointPlaneDist( ActorSpaceCameraPosition, CurRawTriangle.Vertices[ 0 ], TriangleNormal );
			if( !bOnlyFrontFacing || SignedPlaneDist < 0.0f )
			{
				// For each vertex in this triangle
				for( INT CurTriVertexIndex = 0; CurTriVertexIndex < 3; ++CurTriVertexIndex )
				{
					const FVector& ActorSpaceVertexPosition = CurRawTriangle.Vertices[ CurTriVertexIndex ];
					FVector WorldSpaceVertexPosition = ActorToWorldMatrix.TransformFVector( ActorSpaceVertexPosition );

					// Grab vertex color (read/write)
					FColor& VertexColor = CurRawTriangle.Colors[ CurTriVertexIndex ];


					// Paint the vertex!
					FColor NewVertexColor = VertexColor;
					UBOOL bVertexInRange = PaintVertex( WorldSpaceVertexPosition, Params, bShouldApplyPaint && !bUsingInstancedVertexColors, NewVertexColor );


					if( bVertexInRange )
					{
						// Should we actually update the color in the static mesh's raw vertex array?  We
						// only want to do this when configured to edit the actual static mesh (rather than
						// instanced vertex color data on the actor's component.)
						if( bShouldApplyPaint && !bUsingInstancedVertexColors )
						{
							VertexColor = NewVertexColor;
						}

					}
				}
			}
		}

		LODModel.RawTriangles.Unlock();
	}
}



/** Paints mesh texture */
void FEdModeMeshPaint::PaintMeshTexture( UStaticMeshComponent* StaticMeshComponent, const FMeshPaintParameters& Params, const UBOOL bShouldApplyPaint, FStaticMeshRenderData& LODModel, const FVector& ActorSpaceCameraPosition, const FMatrix& ActorToWorldMatrix, const FLOAT ActorSpaceSquaredBrushRadius, const FVector& ActorSpaceBrushPosition )
{
	const UBOOL bOnlyFrontFacing = FMeshPaintSettings::Get().bOnlyFrontFacingTriangles;
	if( bShouldApplyPaint )
	{
		// @todo MeshPaint: Use a spatial database to reduce the triangle set here (kdop)



		// Make sure we're dealing with triangle lists
		const INT NumIndexBufferIndices = LODModel.IndexBuffer.Indices.Num();
		check( NumIndexBufferIndices % 3 == 0 );
		const INT NumTriangles = NumIndexBufferIndices / 3;

		// Keep a list of front-facing triangles that are within a reasonable distance to the brush
		static TArray< INT > InfluencedTriangles;
		InfluencedTriangles.Empty( NumTriangles );

		// Use a bit of distance bias to make sure that we get all of the overlapping triangles.  We
		// definitely don't want our brush to be cut off by a hard triangle edge
		const FLOAT SquaredRadiusBias = ActorSpaceSquaredBrushRadius * 0.025f;

		// For each triangle in the mesh
		for( INT TriIndex = 0; TriIndex < NumTriangles; ++TriIndex )
		{
			// Grab the vertex indices and points for this triangle
			FVector TriVertices[ 3 ];
			for( INT TriVertexNum = 0; TriVertexNum < 3; ++TriVertexNum )
			{
				const INT VertexIndex = LODModel.IndexBuffer.Indices( TriIndex * 3 + TriVertexNum );
				TriVertices[ TriVertexNum ] = LODModel.PositionVertexBuffer.VertexPosition( VertexIndex );
			}

			// Check to see if the triangle is front facing
			FVector TriangleNormal = ( TriVertices[ 1 ] - TriVertices[ 0 ] ^ TriVertices[ 2 ] - TriVertices[ 0 ] ).SafeNormal();
			const FLOAT SignedPlaneDist = FPointPlaneDist( ActorSpaceCameraPosition, TriVertices[ 0 ], TriangleNormal );
			if( !bOnlyFrontFacing || SignedPlaneDist < 0.0f )
			{
				// Compute closest point to the triangle in actor space
				// @todo MeshPaint: Perform AABB test first to speed things up?
				FVector ClosestPointOnTriangle = ClosestPointOnTriangleToPoint( ActorSpaceBrushPosition, TriVertices[ 0 ], TriVertices[ 1 ], TriVertices[ 2 ] );
				FVector ActorSpaceBrushToMeshVector = ActorSpaceBrushPosition - ClosestPointOnTriangle;
				const FLOAT ActorSpaceSquaredDistanceToBrush = ActorSpaceBrushToMeshVector.SizeSquared();
				if( ActorSpaceSquaredDistanceToBrush <= ( ActorSpaceSquaredBrushRadius + SquaredRadiusBias ) )
				{
					// At least one triangle vertex was influenced, so add this triangle!
					InfluencedTriangles.AddItem( TriIndex );
				}
			}
		}


		{
			// @todo MeshPaint: Support multiple mesh components with texture painting?
			if( TexturePaintingStaticMeshComponent != StaticMeshComponent )
			{
				// Mesh has changed, so finish up with our previous texture
				FinishPaintingTexture();
			}

			if( TexturePaintingStaticMeshComponent == NULL )
			{
				StartPaintingTexture( StaticMeshComponent );
			}

			if( TexturePaintingStaticMeshComponent != NULL )
			{
				PaintTexture( Params, InfluencedTriangles, ActorToWorldMatrix );
			}
		}
	}
	else
	{
		if( !bIsPainting )
		{
			FinishPaintingTexture();
		}
	}
}




/** Starts painting a texture */
void FEdModeMeshPaint::StartPaintingTexture( UStaticMeshComponent* InStaticMeshComponent )
{
	check( InStaticMeshComponent != NULL );

	check( TexturePaintingStaticMeshComponent == NULL );
	check( PaintingTexture2D == NULL );


	UBOOL bStartedPainting = FALSE;


	// @todo MeshPaint
	const INT MaterialIndex = 0;

	UTexture2D* Texture2D = NULL;
	UMaterialInterface* PaintingMaterialInterface =
		InStaticMeshComponent->GetMaterial( MaterialIndex, PaintingMeshLODIndex );
	if( PaintingMaterialInterface != NULL )
	{
		// Figure out which texture to paint on
		UTexture* Texture = NULL;
		if( PaintingMaterialInterface->GetTextureParameterValue( PaintTextureName, Texture ) &&
			Texture != NULL )
		{
			// We only support painting on 2D textures
			Texture2D = Cast<UTexture2D>( Texture );
			if( Texture2D != NULL )
			{
				// @todo MeshPaint (we can force a streaming status update here too to speed things up if needed)
				// @todo MeshPaint: Use source data instead?
				const UBOOL bIsSourceTextureStreamedIn = Texture2D->IsFullyStreamedIn();
				if( bIsSourceTextureStreamedIn )
				{
					const INT TextureWidth = Texture->GetSurfaceWidth();
					const INT TextureHeight = Texture->GetSurfaceHeight();


					// @todo MeshPaint: We're relying on GC to clean up render targets, can we free up remote memory more quickly?

					// Create our render target texture
					if( PaintRenderTargetTexture == NULL ||
						PaintRenderTargetTexture->GetSurfaceWidth() != TextureWidth ||
						PaintRenderTargetTexture->GetSurfaceHeight() != TextureHeight )
					{
						PaintRenderTargetTexture = NULL;

						PaintRenderTargetTexture = CastChecked<UTextureRenderTarget2D>( UObject::StaticConstructObject( UTextureRenderTarget2D::StaticClass(), UObject::GetTransientPackage(), NAME_None, RF_Transient ) );
						const UBOOL bForceLinearGamma = TRUE;
						PaintRenderTargetTexture->bUpdateImmediate = TRUE;
						PaintRenderTargetTexture->Init( TextureWidth, TextureHeight, PF_A8R8G8B8, bForceLinearGamma );
					}
					PaintRenderTargetTexture->AddressX = Texture2D->AddressX;
					PaintRenderTargetTexture->AddressY = Texture2D->AddressY;


					// Create the "clone" render target texture
					if( CloneRenderTargetTexture == NULL ||
						CloneRenderTargetTexture->GetSurfaceWidth() != TextureWidth ||
						CloneRenderTargetTexture->GetSurfaceHeight() != TextureHeight )
					{
						CloneRenderTargetTexture = NULL;

						CloneRenderTargetTexture = CastChecked<UTextureRenderTarget2D>( UObject::StaticConstructObject( UTextureRenderTarget2D::StaticClass(), UObject::GetTransientPackage(), NAME_None, RF_Transient ) );
						const UBOOL bForceLinearGamma = TRUE;
						PaintRenderTargetTexture->bUpdateImmediate = TRUE;
						CloneRenderTargetTexture->Init( TextureWidth, TextureHeight, PF_A8R8G8B8, bForceLinearGamma );

						CloneRenderTargetTexture->AddressX = TA_Clamp;
 						CloneRenderTargetTexture->AddressY = TA_Clamp;
					}



					// @todo MeshPaint: OverrideTexture doesn't serialize ref to the overridden texture (but we do, bother fixing?)
					PaintingMaterialInterface->OverrideTexture( Texture, PaintRenderTargetTexture );


					bStartedPainting = TRUE;
				}
			}
		}
	}


	if( bStartedPainting )
	{
		TexturePaintingStaticMeshComponent = InStaticMeshComponent;

		check( Texture2D != NULL );
		PaintingTexture2D = Texture2D;

		// OK, now we need to make sure our render target is filled in with data
		SetupInitialRenderTargetData();
	}

}



/** Paints on a texture */
void FEdModeMeshPaint::PaintTexture( const FMeshPaintParameters& InParams,
									 const TArray< INT >& InInfluencedTriangles,
									 const FMatrix& InActorToWorldMatrix )
{
	// First, we'll copy the current image to a scratch texture that we can pass to a texture sampler
	{
		check( CloneRenderTargetTexture != NULL );
		CopyTextureToRenderTargetTexture( PaintRenderTargetTexture, CloneRenderTargetTexture );
	}


	check( PaintRenderTargetTexture != NULL );


	const FMatrix WorldToBrushMatrix = InParams.BrushToWorldMatrix.Inverse();


	// Grab the actual render target resource from the texture.  Note that we're absolutely NOT ALLOWED to
	// dereference this pointer.  We're just passing it along to other functions that will use it on the render
	// thread.  The only thing we're allowed to do is check to see if it's NULL or not.
	FTextureRenderTargetResource* RenderTargetResource = PaintRenderTargetTexture->GameThread_GetRenderTargetResource();
	check( RenderTargetResource != NULL );


	{
		// Create a canvas for the render target and clear it to black
		FCanvas Canvas( RenderTargetResource, NULL );


		FStaticMeshRenderData& LODModel = TexturePaintingStaticMeshComponent->StaticMesh->LODModels( PaintingMeshLODIndex );

		for( INT CurIndex = 0; CurIndex < InInfluencedTriangles.Num(); ++CurIndex )
		{
			const INT TriIndex = InInfluencedTriangles( CurIndex );

			const INT PaintUVCoordinateIndex =
				Clamp< INT >( InParams.UVChannel, 0, LODModel.VertexBuffer.GetNumTexCoords() - 1 );

			// Grab the vertex indices and points for this triangle
			FVector TriVertices[ 3 ];
			FVector2D TriUVs[ 3 ];
			FVector2D UVMin( 99999.9f, 99999.9f );
			FVector2D UVMax( -99999.9f, -99999.9f );
			for( INT TriVertexNum = 0; TriVertexNum < 3; ++TriVertexNum )
			{																		 
				const INT VertexIndex = LODModel.IndexBuffer.Indices( TriIndex * 3 + TriVertexNum );
				TriVertices[ TriVertexNum ] = InActorToWorldMatrix.TransformFVector( LODModel.PositionVertexBuffer.VertexPosition( VertexIndex ) );
				TriUVs[ TriVertexNum ] = LODModel.VertexBuffer.GetVertexUV( VertexIndex, PaintUVCoordinateIndex );

				// Update bounds
				FLOAT U = TriUVs[ TriVertexNum ].X;
				FLOAT V = TriUVs[ TriVertexNum ].Y;
				if( U < UVMin.X )
				{
					UVMin.X = U;
				}
				if( U > UVMax.X )
				{
					UVMax.X = U;
				}
				if( V < UVMin.Y )
				{
					UVMin.Y = V;
				}
				if( V > UVMax.Y )
				{
					UVMax.Y = V;
				}
			}



			// If the triangle lies entirely outside of the 0.0-1.0 range, we'll transpose it back
			FVector2D UVOffset( 0.0f, 0.0f );
			if( UVMax.X > 1.0f )
			{
				UVOffset.X = -appFloor( UVMin.X );
			}
			else if( UVMin.X < 0.0f )
			{
				UVOffset.X = 1.0f + appFloor( -UVMax.X );
			}
			
			if( UVMax.Y > 1.0f )
			{
				UVOffset.Y = -appFloor( UVMin.Y );
			}
			else if( UVMin.Y < 0.0f )
			{
				UVOffset.Y = 1.0f + appFloor( -UVMax.Y );
			}


			// Note that we "wrap" the texture coordinates here to handle the case where the user
			// is painting on a tiling texture, or with the UVs out of bounds.  Ideally all of the
			// UVs would be in the 0.0 - 1.0 range but sometimes content isn't setup that way.
			// @todo MeshPaint: Handle triangles that cross the 0.0-1.0 UV boundary?
			FVector2D TrianglePoints[ 3 ];
			for( INT TriVertexNum = 0; TriVertexNum < 3; ++TriVertexNum )
			{
				TriUVs[ TriVertexNum ].X += UVOffset.X;
				TriUVs[ TriVertexNum ].Y += UVOffset.Y;

				// @todo: Need any half-texel offset adjustments here?
				TrianglePoints[ TriVertexNum ].X = TriUVs[ TriVertexNum ].X * PaintRenderTargetTexture->GetSurfaceWidth();
				TrianglePoints[ TriVertexNum ].Y = TriUVs[ TriVertexNum ].Y * PaintRenderTargetTexture->GetSurfaceHeight();
			}




			/** Batched element parameters for mesh paint shaders */
			class FMeshPaintBatchedElementParameters
				: public FBatchedElementParameters
			{

			public:

				/** Binds vertex and pixel shaders for this element */
				virtual void BindShaders_RenderThread( const FMatrix& InTransform,
													   const FLOAT InGamma )
				{
					MeshPaintRendering::SetMeshPaintShaders_RenderThread( InTransform, InGamma, ShaderParams );
				}


			public:

				/** Shader parameters */
				MeshPaintRendering::FMeshPaintShaderParameters ShaderParams;

			};


			TRefCountPtr< FMeshPaintBatchedElementParameters > MeshPaintBatchedElementParameters( new FMeshPaintBatchedElementParameters() );
			{
				MeshPaintBatchedElementParameters->ShaderParams.CloneTexture = CloneRenderTargetTexture;
				MeshPaintBatchedElementParameters->ShaderParams.WorldToBrushMatrix = WorldToBrushMatrix;
				MeshPaintBatchedElementParameters->ShaderParams.BrushRadius = InParams.InnerBrushRadius + InParams.BrushRadialFalloffRange;
				MeshPaintBatchedElementParameters->ShaderParams.BrushRadialFalloffRange = InParams.BrushRadialFalloffRange;
				MeshPaintBatchedElementParameters->ShaderParams.BrushDepth = InParams.InnerBrushDepth + InParams.BrushDepthFalloffRange;
				MeshPaintBatchedElementParameters->ShaderParams.BrushDepthFalloffRange = InParams.BrushDepthFalloffRange;
				MeshPaintBatchedElementParameters->ShaderParams.BrushStrength = InParams.BrushStrength;
				MeshPaintBatchedElementParameters->ShaderParams.BrushColor = InParams.BrushColor;
			}


			DrawTriangle2DWithParameters(
				&Canvas,
				TrianglePoints[ 0 ],
				TriUVs[ 0 ],
				FLinearColor( TriVertices[ 0 ].X, TriVertices[ 0 ].Y, TriVertices[ 0 ].Z ),
				TrianglePoints[ 1 ],
				TriUVs[ 1 ],
				FLinearColor( TriVertices[ 1 ].X, TriVertices[ 1 ].Y, TriVertices[ 1 ].Z ),
				TrianglePoints[ 2 ],
				TriUVs[ 2 ],
				FLinearColor( TriVertices[ 2 ].X, TriVertices[ 2 ].Y, TriVertices[ 2 ].Z ),
				MeshPaintBatchedElementParameters,	// Parameters
				FALSE );		// Alpha blend?
		} 



		// Tell the rendering thread to draw any remaining batched elements
		Canvas.Flush();
	}


	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			UpdateMeshPaintRTCommand,
			FTextureRenderTargetResource*, RenderTargetResource, RenderTargetResource,
		{
			// Copy (resolve) the rendered image from the frame buffer to its render target texture
			RHICopyToResolveTarget(
				RenderTargetResource->GetRenderTargetSurface(),		// Source texture
				TRUE,												// Do we need the source image content again?
				FResolveParams() );									// Resolve parameters
		});

	}

}




/** Finishes painting a texture */
void FEdModeMeshPaint::FinishPaintingTexture()
{
	if( TexturePaintingStaticMeshComponent != NULL )
	{
		check( PaintingTexture2D != NULL );


		// @todo MeshPaint
		const INT MaterialIndex = 0;

	/*
		// Copy the contents of the remote texture to system memory
		// NOTE: OutRawImageData must be a preallocated buffer!
		RenderTargetResource->ReadPixels(
			OutThumbnail.AccessImageData(),		// Out: Image data
			CubeFace_PosX,						// Cube face index
			0,									// Top left X
			0,									// Top left Y
			OutThumbnail.GetImageWidth(),		// Width
			OutThumbnail.GetImageHeight() );	// Height
			*/

		check( TexturePaintingStaticMeshComponent != NULL );


		// Commit the texture
		{
			const INT TexWidth = PaintRenderTargetTexture->SizeX;
			const INT TexHeight = PaintRenderTargetTexture->SizeY;
			TArray< FColor > TexturePixels;
			TexturePixels.Add( TexWidth * TexHeight );

			// Copy the contents of the remote texture to system memory
			// NOTE: OutRawImageData must be a preallocated buffer!
			FTextureRenderTargetResource* RenderTargetResource = PaintRenderTargetTexture->GameThread_GetRenderTargetResource();
			RenderTargetResource->ReadPixels( ( BYTE* )&TexturePixels( 0 ) );

			// Compress the data
			FPNGHelper PNGHelper;
			PNGHelper.InitRaw( &TexturePixels( 0 ), TexturePixels.Num() * sizeof( FColor ), TexWidth, TexHeight );
			const TArray<BYTE>& CompressedData = PNGHelper.GetCompressedData();

			// Store source art
			PaintingTexture2D->SetUncompressedSourceArt( TexturePixels.GetData(), TexturePixels.Num() * sizeof( FColor ) );

			// If render target gamma used was 1.0 then disable SRGB for the static texture
			PaintingTexture2D->SRGB = Abs( RenderTargetResource->GetDisplayGamma() - 1.0f ) >= KINDA_SMALL_NUMBER;


			// Update the texture (generate mips, compress if needed)
			PaintingTexture2D->PostEditChange();
		}


		// Clear texture overrides for this mesh
		UMaterialInterface* PaintingMaterialInterface = TexturePaintingStaticMeshComponent->GetMaterial( MaterialIndex, PaintingMeshLODIndex );
		if( PaintingMaterialInterface != NULL )
		{
			// @todo MeshPaint: OverrideTexture doesn't serialize ref to the overridden texture (but we do, bother fixing?)
			PaintingMaterialInterface->OverrideTexture( PaintingTexture2D, NULL );
		}

		PaintingTexture2D = NULL;
		TexturePaintingStaticMeshComponent = NULL;
	}
}




/** FEdMode: Called when mouse drag input it applied */
UBOOL FEdModeMeshPaint::InputDelta( FEditorLevelViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale )
{
	// We only care about perspective viewports
	if( InViewportClient->IsPerspective() )
	{
		// ...
	}

	return FALSE;
}

/** Returns TRUE if we need to force a render/update through based fill/copy */
UBOOL FEdModeMeshPaint::IsForceRendered (void) const
{
	return (bIsFloodFill || bPushInstanceColorsToMesh);
}


/** FEdMode: Render the mesh paint tool */
void FEdModeMeshPaint::Render( const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI )
{
	/** Call parent implementation */
	FEdMode::Render( View, Viewport, PDI );

	// We only care about perspective viewports
	const UBOOL bIsPerspectiveViewport = ( View->ProjectionMatrix.M[ 3 ][ 3 ] < ( 1.0f - SMALL_NUMBER ) );
	if( bIsPerspectiveViewport )
	{
		// Make sure perspective viewports are still set to real-time
		const UBOOL bWantRealTime = TRUE;
		const UBOOL bRememberCurrentState = FALSE;
		ForceRealTimeViewports( bWantRealTime, bRememberCurrentState );


		// Set viewport show flags
		const UBOOL bAllowColorViewModes = TRUE;
		SetViewportShowFlags( bAllowColorViewModes );


		// Make sure the cursor is visible OR we're flood filling.  No point drawing a paint cue when there's no cursor.
		if( Viewport->IsCursorVisible() || IsForceRendered())
		{
			// Make sure the cursor isn't underneath the mesh paint window (unless we're already painting)
			if( bIsPainting || IsForceRendered()
	#if WITH_MANAGED_CODE
				|| MeshPaintWindow == NULL || !MeshPaintWindow->IsMouseOverWindow()
	#endif
				)
			{
				if( !PDI->IsHitTesting() )
				{
					// Grab the mouse cursor position
					FIntPoint MousePosition;
					Viewport->GetMousePos( MousePosition );

					// Is the mouse currently over the viewport? or flood filling
					if(IsForceRendered() || ( MousePosition.X >= 0 && MousePosition.Y >= 0 && MousePosition.X < (INT)Viewport->GetSizeX() && MousePosition.Y < (INT)Viewport->GetSizeY()) )
					{
						// Compute a world space ray from the screen space mouse coordinates
						FViewportCursorLocation MouseViewportRay( View, (FEditorLevelViewportClient*)Viewport->GetClient(), MousePosition.X, MousePosition.Y );


						// Unless "Flow" mode is enabled, we'll only draw a visual cue while rendering and won't
						// do any actual painting.  When "Flow" is turned on we will paint here, too!
						const UBOOL bVisualCueOnly = !FMeshPaintSettings::Get().bEnableFlow;
						FLOAT StrengthScale = FMeshPaintSettings::Get().bEnableFlow ? FMeshPaintSettings::Get().FlowAmount : 1.0f;

						// Apply stylus pressure if it's active
						if( Viewport->IsPenActive() )
						{
							StrengthScale *= Viewport->GetTabletPressure();
						}

						const EMeshPaintAction::Type PaintAction = GetPaintAction(Viewport);
						UBOOL bAnyPaintAbleActorsUnderCursor = FALSE;
						DoPaint( View->ViewOrigin, MouseViewportRay.GetOrigin(), MouseViewportRay.GetDirection(), PDI, PaintAction, bVisualCueOnly, StrengthScale, bAnyPaintAbleActorsUnderCursor );
					}
				}
			}
		}
	}
}



/** FEdMode: Render HUD elements for this tool */
void FEdModeMeshPaint::DrawHUD( FEditorLevelViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas )
{
	// We only care about perspective viewports
	if( ViewportClient->IsPerspective() )
	{
		// Draw screen space bounding box around selected actors.  We do this because while in mesh paint mode,
		// we opt to not highlight selected actors (as that makes it more difficult to paint colors.)  Drawing
		// a box around the selected actors doesn't interfere with painting as much.
		DrawBrackets(ViewportClient, Viewport, View, Canvas);
	}
}



/** FEdMode: Called when the currently selected actor has changed */
void FEdModeMeshPaint::ActorSelectionChangeNotify()
{
	// Make sure the property window is up to date
#if WITH_MANAGED_CODE
	if( MeshPaintWindow != NULL )
	{
		MeshPaintWindow->RefreshAllProperties();
	}
#endif
}



/** Forces real-time perspective viewports */
void FEdModeMeshPaint::ForceRealTimeViewports( const UBOOL bEnable, const UBOOL bStoreCurrentState )
{
	// Force perspective views to be real-time
	for( INT CurViewportIndex = 0; CurViewportIndex < GApp->EditorFrame->ViewportConfigData->GetViewportCount(); ++CurViewportIndex )
	{
		WxLevelViewportWindow* CurLevelViewportWindow =
			GApp->EditorFrame->ViewportConfigData->AccessViewport( CurViewportIndex ).ViewportWindow;
		if( CurLevelViewportWindow != NULL )
		{
			if( CurLevelViewportWindow->ViewportType == LVT_Perspective )
			{				
				if( bEnable )
				{
					CurLevelViewportWindow->SetRealtime( bEnable, bStoreCurrentState );
				}
				else
				{
					const UBOOL bAllowDisable = TRUE;
					CurLevelViewportWindow->RestoreRealtime( bAllowDisable );
				}

			}
		}
	}
}



/** Sets show flags for perspective viewports */
void FEdModeMeshPaint::SetViewportShowFlags( const UBOOL bAllowColorViewModes )
{
	// Force perspective views to be real-time
	for( INT CurViewportIndex = 0; CurViewportIndex < GApp->EditorFrame->ViewportConfigData->GetViewportCount(); ++CurViewportIndex )
	{
		WxLevelViewportWindow* CurLevelViewportWindow =
			GApp->EditorFrame->ViewportConfigData->AccessViewport( CurViewportIndex ).ViewportWindow;
		if( CurLevelViewportWindow != NULL )
		{
			if( CurLevelViewportWindow->ViewportType == LVT_Perspective )
			{				
				// Update viewport show flags
				{
					const EShowFlags VertexColorShowFlags = SHOW_BSPTriangles | SHOW_Materials | SHOW_PostProcess | SHOW_Fog | SHOW_VertexColors;

					EMeshPaintColorViewMode::Type ColorViewMode = FMeshPaintSettings::Get().ColorViewMode;
					if( !bAllowColorViewModes )
					{
						ColorViewMode = EMeshPaintColorViewMode::Normal;
					}

					if(ColorViewMode == EMeshPaintColorViewMode::Normal)
					{
						if( ( CurLevelViewportWindow->ShowFlags & SHOW_VertexColors ) == SHOW_VertexColors )
						{
							// If we're transitioning to normal mode then restore the backup
							CurLevelViewportWindow->ShowFlags = ( SHOW_DefaultEditor & ~(SHOW_ViewMode_Mask | SHOW_BuilderBrush) ) | CurLevelViewportWindow->PreviousEdModeShowFlags | (CurLevelViewportWindow->ShowFlags & SHOW_BuilderBrush);
							GVertexColorViewMode = EVertexColorViewMode::Color;
						}
					}
					else
					{
						// If we're transitioning from normal mode then backup the current view mode
						if( ( CurLevelViewportWindow->ShowFlags & SHOW_VertexColors ) == 0 )
						{
							CurLevelViewportWindow->PreviousEdModeShowFlags = CurLevelViewportWindow->ShowFlags & SHOW_ViewMode_Mask;
						}

						CurLevelViewportWindow->ShowFlags = ( SHOW_DefaultEditor & ~(SHOW_ViewMode_Mask | SHOW_BuilderBrush) ) | VertexColorShowFlags | (CurLevelViewportWindow->ShowFlags & SHOW_BuilderBrush);

						switch( ColorViewMode )
						{
							case EMeshPaintColorViewMode::RGB:
								{
									GVertexColorViewMode = EVertexColorViewMode::Color;
								}
								break;

							case EMeshPaintColorViewMode::Alpha:
								{
									GVertexColorViewMode = EVertexColorViewMode::Alpha;
								}
								break;

							case EMeshPaintColorViewMode::Red:
								{
									GVertexColorViewMode = EVertexColorViewMode::Red;
								}
								break;

							case EMeshPaintColorViewMode::Green:
								{
									GVertexColorViewMode = EVertexColorViewMode::Green;
								}
								break;

							case EMeshPaintColorViewMode::Blue:
								{
									GVertexColorViewMode = EVertexColorViewMode::Blue;
								}
								break;
						}
					}
				}
			}
		}
	}
}



/** Makes sure that the render target is ready to paint on */
void FEdModeMeshPaint::SetupInitialRenderTargetData()
{
	check( PaintingTexture2D != NULL );


/*
	// Copy the source texture to the render target
	{
		// @todo MeshPaint
		if( PaintingTexture2D->SourceArt.GetBulkDataSize() > 0 )
		{
			FPNGHelper PNG;
			PNG.InitCompressed( PaintingTexture2D->SourceArt.Lock( LOCK_READ_ONLY ), PaintingTexture2D->SourceArt.GetBulkDataSize(), PaintingTexture2D->SizeX, PaintingTexture2D->SizeY );
			const TArray<BYTE>& RawData = PNG.GetRawData();
			PaintingTexture2D->SourceArt.Unlock();

			// RawData

		}
		else if( PaintingTexture2D->Format == PF_A8R8G8B8 && PaintingTexture2D->Mips.Num() > 0 )
		{
			FTexture2DMipMap& BaseMip = PaintingTexture2D->Mips( 0);
			BYTE* RawData = (BYTE*)BaseMip.Data.Lock( LOCK_READ_ONLY );

			// RawData

			BaseMip.Data.Unlock();
		}
	}
  */

	if( PaintingTexture2D->HasSourceArt() )
	{
		// Great, we have source data!  We'll use that as our image source.

		// Decompress PNG image
		TArray<BYTE> RawData;
		PaintingTexture2D->GetUncompressedSourceArt(RawData);

		// Create a texture in memory from the source art
		{
			// @todo MeshPaint: This generates a lot of memory thrash -- try to cache this texture and reuse it?
			UTexture2D* TempSourceArtTexture = CreateTempUncompressedTexture( PaintingTexture2D->SizeX, PaintingTexture2D->SizeY, RawData, PaintingTexture2D->SRGB );
			check( TempSourceArtTexture != NULL );

			// Copy the texture to the render target using the GPU
			CopyTextureToRenderTargetTexture( TempSourceArtTexture, PaintRenderTargetTexture );

			// NOTE: TempSourceArtTexture is no longer needed (will be GC'd)
		}
	}
	else
	{
		// Just copy (render) the texture in GPU memory to our render target.  Hopefully it's not
		// compressed already!
		check( PaintingTexture2D->IsFullyStreamedIn() );
		CopyTextureToRenderTargetTexture( PaintingTexture2D, PaintRenderTargetTexture );
	}

}



/** Static: Creates a temporary texture used to transfer data to a render target in memory */
UTexture2D* FEdModeMeshPaint::CreateTempUncompressedTexture( const INT InWidth, const INT InHeight, const TArray< BYTE >& InRawData, const UBOOL bInUseSRGB )
{
	check( InWidth > 0 && InHeight > 0 && InRawData.Num() > 0 );

	// Allocate the new texture
	const EObjectFlags ObjectFlags = RF_NotForClient | RF_NotForServer | RF_Transient;
	UTexture2D* NewTexture2D = CastChecked< UTexture2D >( UObject::StaticConstructObject( UTexture2D::StaticClass(), UObject::GetTransientPackage(), NAME_None, ObjectFlags ) );
	NewTexture2D->Init( InWidth, InHeight, PF_A8R8G8B8 );
	
	// Fill in the base mip for the texture we created
	BYTE* MipData = ( BYTE* )NewTexture2D->Mips( 0 ).Data.Lock( LOCK_READ_WRITE );
	for( INT y=0; y<InHeight; y++ )
	{
		BYTE* DestPtr = &MipData[(InHeight - 1 - y) * InWidth * sizeof(FColor)];
		const FColor* SrcPtr = &( (FColor*)( &InRawData(0) ) )[ ( InHeight - 1 - y ) * InWidth ];
		for( INT x=0; x<InWidth; x++ )
		{
			*DestPtr++ = SrcPtr->B;
			*DestPtr++ = SrcPtr->G;
			*DestPtr++ = SrcPtr->R;
			*DestPtr++ = 0xFF;
			SrcPtr++;
		}
	}
	NewTexture2D->Mips( 0 ).Data.Unlock();

	// Set options
	NewTexture2D->SRGB = bInUseSRGB;
	NewTexture2D->CompressionNone = TRUE;
	NewTexture2D->MipGenSettings = TMGS_NoMipmaps;
	NewTexture2D->CompressionSettings = TC_Default;

	// Update the remote texture data
	NewTexture2D->UpdateResource();

	return NewTexture2D;
}



/** Static: Copies a texture to a render target texture */
void FEdModeMeshPaint::CopyTextureToRenderTargetTexture( UTexture* SourceTexture, UTextureRenderTarget2D* RenderTargetTexture )
{	
	check( SourceTexture != NULL );
	check( RenderTargetTexture != NULL );

	/*
	// Grab an editor viewport to use as a frame buffer
	FViewport* Viewport = NULL;
	for( INT CurMainViewportIndex = 0; CurMainViewportIndex < GApp->EditorFrame->ViewportConfigData->GetViewportCount(); ++CurMainViewportIndex )
	{
		// Grab the first level viewport to use for rendering
		FEditorLevelViewportClient* LevelVC = GApp->EditorFrame->ViewportConfigData->AccessViewport( CurMainViewportIndex ).ViewportWindow;
		if( LevelVC != NULL )
		{
			Viewport = LevelVC->Viewport;
			break;
		}
	}
	check( Viewport != NULL );

	// Begin scene
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		BeginDrawingCommand,
		FViewport*,Viewport,Viewport,
	{
		Viewport->BeginRenderFrame();
	});*/



	// Grab the actual render target resource from the texture.  Note that we're absolutely NOT ALLOWED to
	// dereference this pointer.  We're just passing it along to other functions that will use it on the render
	// thread.  The only thing we're allowed to do is check to see if it's NULL or not.
	FTextureRenderTargetResource* RenderTargetResource = RenderTargetTexture->GameThread_GetRenderTargetResource();
	check( RenderTargetResource != NULL );


	{
		// Create a canvas for the render target and clear it to black
		FCanvas Canvas( RenderTargetResource, NULL );

		const UINT Width = RenderTargetTexture->GetSurfaceWidth();
		const UINT Height = RenderTargetTexture->GetSurfaceHeight();

		// @todo MeshPaint: Need full color/alpha writes enabled to get alpha
		// @todo MeshPaint: Texels need to line up perfectly to avoid bilinear artifacts
		// @todo MeshPaint: Potential gamma issues here
		// @todo MeshPaint: Probably using CLAMP address mode when reading from source (if texels line up, shouldn't matter though.)

		// @todo MeshPaint: Should use scratch texture built from original source art (when possible!)
		//		-> Current method will have compression artifacts!


		// Grab the texture resource.  We only support 2D textures and render target textures here.
		FTexture* TextureResource = NULL;
		UTexture2D* Texture2D = Cast<UTexture2D>( SourceTexture );
		if( Texture2D != NULL )
		{
			TextureResource = Texture2D->Resource;
		}
		else
		{
			UTextureRenderTarget2D* TextureRenderTarget2D = Cast<UTextureRenderTarget2D>( SourceTexture );
			TextureResource = TextureRenderTarget2D->GameThread_GetRenderTargetResource();
		}
		check( TextureResource != NULL );


		// Draw a quad to copy the texture over to the render target
		{		
			const FLOAT MinU = 0.0f;
			const FLOAT MinV = 0.0f;
			const FLOAT MaxU = 1.0f;
			const FLOAT MaxV = 1.0f;
			const FLOAT MinX = 0.0f;
			const FLOAT MinY = 0.0f;
			const FLOAT MaxX = Width;
			const FLOAT MaxY = Height;

			DrawTriangle2D(
				&Canvas,
				FVector2D( MinX, MinY ),
				FVector2D( MinU, MinV ),
				FVector2D( MaxX, MinY ),
				FVector2D( MaxU, MinV ),
				FVector2D( MaxX, MaxY ),
				FVector2D( MaxU, MaxV ),
				FLinearColor::White,
				TextureResource,
				FALSE );		// Alpha blend?

			DrawTriangle2D(
				&Canvas,
				FVector2D( MaxX, MaxY ),
				FVector2D( MaxU, MaxV ),
				FVector2D( MinX, MaxY ),
				FVector2D( MinU, MaxV ),
				FVector2D( MinX, MinY ),
				FVector2D( MinU, MinV ),
				FLinearColor::White,
				TextureResource,
				FALSE );		// Alpha blend?
		}

		// Tell the rendering thread to draw any remaining batched elements
		Canvas.Flush();
	}


	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			UpdateMeshPaintRTCommand,
			FTextureRenderTargetResource*, RenderTargetResource, RenderTargetResource,
		{
			// Copy (resolve) the rendered image from the frame buffer to its render target texture
			RHICopyToResolveTarget(
				RenderTargetResource->GetRenderTargetSurface(),		// Source texture
				TRUE,												// Do we need the source image content again?
				FResolveParams() );									// Resolve parameters
		});

	}

	  /*
	// End scene
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		EndDrawingCommand,
		FViewport*,Viewport,Viewport,
	{
		const UBOOL bShouldPresent = FALSE;
		const UBOOL bLockToVSync = FALSE;
		Viewport->EndRenderFrame( bShouldPresent, bLockToVSync );
	});
	*/
}

/** Helper function to get the current paint action for use in DoPaint */
EMeshPaintAction::Type FEdModeMeshPaint::GetPaintAction(FViewport* InViewport)
{
	check(InViewport);
	const UBOOL bShiftDown = InViewport->KeyState( KEY_LeftShift ) || InViewport->KeyState( KEY_RightShift );
	EMeshPaintAction::Type PaintAction;
	if (bIsFloodFill)
	{
		PaintAction = EMeshPaintAction::Fill;
		//turn off so we don't do this next frame!
		bIsFloodFill = FALSE;
	}
	else if (bPushInstanceColorsToMesh)
	{
		PaintAction = EMeshPaintAction::PushInstanceColorsToMesh;
		//turn off so we don't do this next frame!
		bPushInstanceColorsToMesh = FALSE;
	}
	else
	{
		PaintAction = bShiftDown ? EMeshPaintAction::Erase : EMeshPaintAction::Paint;
	}
	return PaintAction;

}

/** Removes vertex colors associated with the currently selected mesh */
void FEdModeMeshPaint::RemoveInstanceVertexColors() const
{
	USelection& SelectedActors = *GEditor->GetSelectedActors();
	for( INT CurSelectedActorIndex = 0; CurSelectedActorIndex < SelectedActors.Num(); ++CurSelectedActorIndex )
	{
		UStaticMeshComponent* StaticMeshComponent = NULL;
		AStaticMeshActor* StaticMeshActor = Cast< AStaticMeshActor >( SelectedActors( CurSelectedActorIndex ) );
		if( StaticMeshActor != NULL )
		{
			StaticMeshComponent = StaticMeshActor->StaticMeshComponent;
		}
		else
		{
			ADynamicSMActor* DynamicSMActor = Cast< ADynamicSMActor >( SelectedActors( CurSelectedActorIndex ) );
			if( DynamicSMActor != NULL )
			{
				StaticMeshComponent = DynamicSMActor->StaticMeshComponent;
			}
		}

		if( StaticMeshComponent != NULL )
		{
			check( StaticMeshComponent->StaticMesh->LODModels.Num() > PaintingMeshLODIndex );

			// Make sure we have component-level LOD information
			if( StaticMeshComponent->LODData.Num() > PaintingMeshLODIndex )
			{
				FStaticMeshComponentLODInfo* InstanceMeshLODInfo = &StaticMeshComponent->LODData( PaintingMeshLODIndex );

				if(InstanceMeshLODInfo->OverrideVertexColors)
				{
					// Detach all instances of this static mesh from the scene.
					FComponentReattachContext ComponentReattachContext( StaticMeshComponent );

					// @todo MeshPaint: Should make this undoable

					// Mark the mesh component as modified
					StaticMeshComponent->Modify();

					InstanceMeshLODInfo->ReleaseOverrideVertexColorsAndBlock();

#if WITH_MANAGED_CODE
					// The instance vertex color byte count has changed so tell the mesh paint window
					// to refresh it's properties
					MeshPaintWindow->RefreshAllProperties();
#endif
				}
			}
		}
	}
}

/** Returns whether the instance vertex colors associated with the currently selected mesh need to be fixed up or not */
UBOOL FEdModeMeshPaint::RequiresInstanceVertexColorsFixup() const
{
	UBOOL bRequiresFixup = FALSE;

	// Find each static mesh component of any selected actors
	USelection& SelectedActors = *GEditor->GetSelectedActors();
	for( INT CurSelectedActorIndex = 0; CurSelectedActorIndex < SelectedActors.Num(); ++CurSelectedActorIndex )
	{
		UStaticMeshComponent* StaticMeshComponent = NULL;
		AStaticMeshActor* StaticMeshActor = Cast< AStaticMeshActor >( SelectedActors( CurSelectedActorIndex ) );
		if( StaticMeshActor )
		{
			StaticMeshComponent = StaticMeshActor->StaticMeshComponent;
		}
		else
		{
			ADynamicSMActor* DynamicSMActor = Cast< ADynamicSMActor >( SelectedActors( CurSelectedActorIndex ) );
			if( DynamicSMActor )
			{
				StaticMeshComponent = DynamicSMActor->StaticMeshComponent;
			}
		}

		// If a static mesh component was found and it requires fixup, exit out and indicate as such
		TArray<INT> LODsToFixup;
		if( StaticMeshComponent && StaticMeshComponent->RequiresOverrideVertexColorsFixup( LODsToFixup ) )
		{
			bRequiresFixup = TRUE;
			break;
		}
	}

	return bRequiresFixup;
}

/** Attempts to fix up the instance vertex colors associated with the currently selected mesh, if necessary */
void FEdModeMeshPaint::FixupInstanceVertexColors() const
{
	// Find each static mesh component of any selected actors
	USelection& SelectedActors = *GEditor->GetSelectedActors();
	for( INT CurSelectedActorIndex = 0; CurSelectedActorIndex < SelectedActors.Num(); ++CurSelectedActorIndex )
	{
		UStaticMeshComponent* StaticMeshComponent = NULL;
		AStaticMeshActor* StaticMeshActor = Cast< AStaticMeshActor >( SelectedActors( CurSelectedActorIndex ) );
		if( StaticMeshActor != NULL )
		{
			StaticMeshComponent = StaticMeshActor->StaticMeshComponent;
		}
		else
		{
			ADynamicSMActor* DynamicSMActor = Cast< ADynamicSMActor >( SelectedActors( CurSelectedActorIndex ) );
			if( DynamicSMActor != NULL )
			{
				StaticMeshComponent = DynamicSMActor->StaticMeshComponent;
			}
		}

		// If a static mesh component was found, attempt to fixup its override colors
		if( StaticMeshComponent )
		{
			StaticMeshComponent->FixupOverrideColorsIfNecessary();
		}
	}

#if WITH_MANAGED_CODE
	MeshPaintWindow->RefreshAllProperties();
#endif
}

/** Fills the vertex colors associated with the currently selected mesh*/
void FEdModeMeshPaint::FillInstanceVertexColors()
{
	//force this on for next render
	bIsFloodFill = TRUE;
	GCallbackEvent->Send( CALLBACK_RedrawAllViewports );
}

/** Pushes instance vertex colors to the  mesh*/
void FEdModeMeshPaint::PushInstanceVertexColorsToMesh()
{
	INT NumVertexColorBytes = 0;
	UBOOL bHasInstanceMaterialAndTexture = FALSE;

	// Check that there's actually a mesh selected and that it has instanced vertex colors before actually proceeding
	const UBOOL bMeshSelected = GetSelectedMeshInfo( NumVertexColorBytes, bHasInstanceMaterialAndTexture );
	if ( bMeshSelected && NumVertexColorBytes > 0 )
	{
		// Prompt the user to see if they really want to push the vert colors to the source mesh and to explain
		// the ramifications of doing so
		WxChoiceDialog ConfirmationPrompt(
			LocalizeUnrealEd("PushInstanceVertexColorsPrompt_Message"), 
			LocalizeUnrealEd("PushInstanceVertexColorsPrompt_Title"),
			WxChoiceDialogBase::Choice( ART_Yes, LocalizeUnrealEd("PushInstanceVertexColorsPrompt_Ok"), WxChoiceDialogBase::DCT_DefaultAffirmative ),
			WxChoiceDialogBase::Choice( ART_No, LocalizeUnrealEd("PushInstanceVertexColorsPrompt_Cancel"), WxChoiceDialogBase::DCT_DefaultCancel ) );
		ConfirmationPrompt.ShowModal();
		
		if ( ConfirmationPrompt.GetChoice().ReturnCode == ART_Yes )
		{
			//force this on for next render
			bPushInstanceColorsToMesh = TRUE;
			GCallbackEvent->Send( CALLBACK_RedrawAllViewports );
		}
	}
}

/** Creates a paintable material/texture for the selected mesh */
void FEdModeMeshPaint::CreateInstanceMaterialAndTexture() const
{
	USelection& SelectedActors = *GEditor->GetSelectedActors();
	for( INT CurSelectedActorIndex = 0; CurSelectedActorIndex < SelectedActors.Num(); ++CurSelectedActorIndex )
	{
		AActor* CurSelectedActor = CastChecked<AActor>( SelectedActors( CurSelectedActorIndex ) );
		UStaticMeshComponent* StaticMeshComponent = NULL;
		AStaticMeshActor* StaticMeshActor = Cast< AStaticMeshActor >( CurSelectedActor );
		if( StaticMeshActor != NULL )
		{
			StaticMeshComponent = StaticMeshActor->StaticMeshComponent;
		}
		else
		{
			ADynamicSMActor* DynamicSMActor = Cast< ADynamicSMActor >( CurSelectedActor );
			if( DynamicSMActor != NULL )
			{
				StaticMeshComponent = DynamicSMActor->StaticMeshComponent;
			}
		}
		if( StaticMeshComponent != NULL )
		{
			check( StaticMeshComponent->StaticMesh->LODModels.Num() > PaintingMeshLODIndex );
			const FStaticMeshLODInfo& LODInfo = StaticMeshComponent->StaticMesh->LODInfo( PaintingMeshLODIndex );

			// @todo MeshPaint: Make mesh element selectable?
			const INT MeshElementIndex = 0;

			UMaterialInterface* OriginalMaterial = NULL;

			// First check to see if we have an existing override material on the actor instance.
			if( StaticMeshComponent->Materials.Num() > MeshElementIndex )
			{
				if( StaticMeshComponent->Materials( MeshElementIndex ) != NULL )
				{
					OriginalMaterial = StaticMeshComponent->Materials( MeshElementIndex );
				}
			}

			if( OriginalMaterial == NULL )
			{
				// Grab the material straight from the mesh
				if( LODInfo.Elements.Num() > MeshElementIndex )
				{
					const FStaticMeshLODElement& ElementInfo = LODInfo.Elements( MeshElementIndex );
					if( ElementInfo.Material != NULL )
					{
						OriginalMaterial = ElementInfo.Material;
					}
				}
			}


			if( OriginalMaterial != NULL )
			{
				// @todo MeshPaint: Undo support

				FComponentReattachContext ComponentReattachContext( StaticMeshComponent );


				// Create the new texture
				UTexture2D* NewTexture = NULL;
				{
					// @todo MeshPaint: Make configurable
					// @todo MeshPaint: Expose compression options? (default to off?)
					// @todo MeshPaint: Expose SRGB/RGBE options?
					// @todo MeshPaint: Use bWantSourceArt=FALSE to speed this up
					const INT TexWidth = 1024;
					const INT TexHeight = 1024;
					const UBOOL bUseAlpha = TRUE;

					TArray< FColor > ImageData;
					ImageData.Add( TexWidth * TexHeight );
					const FColor WhiteColor( 255, 255, 255, 255 );
					for( INT CurTexelIndex = 0; CurTexelIndex < ImageData.Num(); ++CurTexelIndex )
					{
						ImageData( CurTexelIndex ) = WhiteColor;
					}

					FCreateTexture2DParameters Params;
					Params.bUseAlpha = bUseAlpha;

					NewTexture = FImageUtils::CreateTexture2D(
						TexWidth,
						TexHeight,
						ImageData,
						CurSelectedActor->GetOuter(),
						FString::Printf( TEXT( "%s_PaintTexture" ), *CurSelectedActor->GetName() ),
						RF_Public,
						Params );

					NewTexture->MarkPackageDirty();
				}


				// Create the new material instance
				UMaterialInstanceConstant* NewMaterialInstance = NULL;
				{
					NewMaterialInstance = CastChecked<UMaterialInstanceConstant>( UObject::StaticConstructObject(
						UMaterialInstanceConstant::StaticClass(), 
						CurSelectedActor->GetOuter(), 
						FName( *FString::Printf( TEXT( "%s_PaintMIC" ), *CurSelectedActor->GetName() ) ),
						RF_Public
						));

					// Bind texture to the material instance
					NewMaterialInstance->SetTextureParameterValue( PaintTextureName, NewTexture );

					NewMaterialInstance->SetParent( OriginalMaterial );
					NewMaterialInstance->MarkPackageDirty();
				}

				
				// Assign material to the static mesh component
				{
					StaticMeshComponent->Modify();
					while( StaticMeshComponent->Materials.Num() <= MeshElementIndex )
					{
						StaticMeshComponent->Materials.AddZeroed();
					}
					StaticMeshComponent->Materials( MeshElementIndex ) = NewMaterialInstance;
				}
			}
		}
	}
}



/** Removes instance of paintable material/texture for the selected mesh */
void FEdModeMeshPaint::RemoveInstanceMaterialAndTexture() const
{
	USelection& SelectedActors = *GEditor->GetSelectedActors();
	for( INT CurSelectedActorIndex = 0; CurSelectedActorIndex < SelectedActors.Num(); ++CurSelectedActorIndex )
	{
		AActor* CurSelectedActor = CastChecked<AActor>( SelectedActors( CurSelectedActorIndex ) );
		UStaticMeshComponent* StaticMeshComponent = NULL;
		AStaticMeshActor* StaticMeshActor = Cast< AStaticMeshActor >( CurSelectedActor );
		if( StaticMeshActor != NULL )
		{
			StaticMeshComponent = StaticMeshActor->StaticMeshComponent;
		}
		else
		{
			ADynamicSMActor* DynamicSMActor = Cast< ADynamicSMActor >( CurSelectedActor );
			if( DynamicSMActor != NULL )
			{
				StaticMeshComponent = DynamicSMActor->StaticMeshComponent;
			}
		}

		if( StaticMeshComponent != NULL )
		{
			// ...
		}
	}
}



/** Returns information about the currently selected mesh */
UBOOL FEdModeMeshPaint::GetSelectedMeshInfo( INT& OutTotalInstanceVertexColorBytes, UBOOL& bOutHasInstanceMaterialAndTexture ) const
{
	OutTotalInstanceVertexColorBytes = 0;
	bOutHasInstanceMaterialAndTexture = FALSE;

	INT NumValidMeshes = 0;

	USelection& SelectedActors = *GEditor->GetSelectedActors();
	for( INT CurSelectedActorIndex = 0; CurSelectedActorIndex < SelectedActors.Num(); ++CurSelectedActorIndex )
	{
		AActor* CurSelectedActor = CastChecked<AActor>( SelectedActors( CurSelectedActorIndex ) );
		UStaticMeshComponent* StaticMeshComponent = NULL;
		AStaticMeshActor* StaticMeshActor = Cast< AStaticMeshActor >( CurSelectedActor );
		if( StaticMeshActor != NULL )
		{
			StaticMeshComponent = StaticMeshActor->StaticMeshComponent;
		}
		else
		{
			ADynamicSMActor* DynamicSMActor = Cast< ADynamicSMActor >( CurSelectedActor );
			if( DynamicSMActor != NULL )
			{
				StaticMeshComponent = DynamicSMActor->StaticMeshComponent;
			}
		}
		if( StaticMeshComponent != NULL )
		{
			check( StaticMeshComponent->StaticMesh->LODModels.Num() > PaintingMeshLODIndex );

			// Make sure we have component-level LOD information
			if( StaticMeshComponent->LODData.Num() > PaintingMeshLODIndex )
			{
				const FStaticMeshComponentLODInfo& InstanceMeshLODInfo = StaticMeshComponent->LODData( PaintingMeshLODIndex );

				if( InstanceMeshLODInfo.OverrideVertexColors )
				{
					OutTotalInstanceVertexColorBytes += InstanceMeshLODInfo.OverrideVertexColors->GetAllocatedSize();
				}
			}

			++NumValidMeshes;
		}
	}

	return ( NumValidMeshes > 0 );
}
