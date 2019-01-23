/*=============================================================================
	InterpEditorDraw.cpp: Functions covering drawing the Matinee window
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "EngineSequenceClasses.h"
#include "EngineInterpolationClasses.h"
#include "InterpEditor.h"
#include "UnLinkedObjDrawUtils.h"

static const INT GroupHeadHeight = 24;
static const INT TrackHeight = 32;
static const INT LabelWidth = 160;
static const INT TrackTitleMargin = 43;
static const INT HeadTitleMargin = 4;

static const INT TimelineHeight = 40;
static const INT NavHeight = 24;
static const INT TotalBarHeight = (TimelineHeight + NavHeight);

static const INT TimeIndHalfWidth = 2;
static const INT RangeTickHeight = 8;

static const FColor HeadSelectedColor(255, 100, 0);

static const FColor DirHeadUnselectedColor(200, 180, 160);
static const FColor HeadUnselectedColor(160, 160, 160);
static const FColor	TrackUnselectedColor(220, 220, 220);

static const FColor NullRegionColor(60, 60, 60);
static const FColor NullRegionBorderColor(255, 255, 255);

static const FColor InterpMarkerColor(255, 80, 80);
static const FColor SectionMarkerColor(80, 255, 80);

static const FColor KeyRangeMarkerColor(255, 183, 111);

FLOAT GetGridSpacing(INT GridNum)
{
	if(GridNum & 0x01) // Odd numbers
	{
		return appPow( 10.f, 0.5f*((FLOAT)(GridNum-1)) + 1.f );
	}
	else // Even numbers
	{
		return 0.5f * appPow( 10.f, 0.5f*((FLOAT)(GridNum)) + 1.f );
	}
}

/** Calculate the best frames' density. */
UINT CalculateBestFrameStep(FLOAT SnapAmount, FLOAT PixelsPerSec, FLOAT MinPixelsPerGrid)
{
	UINT FrameRate = appCeil(1.0f / SnapAmount);
	UINT FrameStep = 1;
	
	// Calculate minimal-symmetric integer divisor.
	UINT MinFrameStep = FrameRate;
	UINT i = 2;	
	while ( i < MinFrameStep )
	{
		if ( MinFrameStep % i == 0 )
		{
			MinFrameStep /= i;
			i = 1;
		}
		i++;
	}	

	// Find the best frame step for certain grid density.
	while (	FrameStep * SnapAmount * PixelsPerSec < MinPixelsPerGrid )
	{
		FrameStep++;
		if ( FrameStep < FrameRate )
		{
			// Must be divisible by MinFrameStep and divisor of FrameRate.
			while ( !(FrameStep % MinFrameStep == 0 && FrameRate % FrameStep == 0) )
			{
				FrameStep++;
			}
		}
		else
		{
			// Must be multiple of FrameRate.
			while ( FrameStep % FrameRate != 0 )
			{
				FrameStep++;
			}
		}
	}

	return FrameStep;
}


/** Draw gridlines and time labels. */
void FInterpEdViewportClient::DrawGrid(FViewport* Viewport, FCanvas* Canvas, UBOOL bDrawTimeline)
{
	INT ViewX = Viewport->GetSizeX();
	INT ViewY = Viewport->GetSizeY();

	// Calculate desired grid spacing.
	INT MinPixelsPerGrid = 35;
	FLOAT MinGridSpacing = 0.001f;
	FLOAT GridSpacing = MinGridSpacing;
	UINT FrameStep = 1; // Important frames' density.
	UINT AuxFrameStep = 1; // Auxiliary frames' density.
	
	// Time.
	if (!InterpEd->bSnapToFrames)
	{
		INT GridNum = 0;
		while( GridSpacing * PixelsPerSec < MinPixelsPerGrid )
		{
			GridSpacing = MinGridSpacing * GetGridSpacing(GridNum);
			GridNum++;
		}
	} 	
	else // Frames.
	{
		GridSpacing  = InterpEd->SnapAmount;	
		FrameStep = CalculateBestFrameStep(InterpEd->SnapAmount, PixelsPerSec, MinPixelsPerGrid);
		AuxFrameStep = CalculateBestFrameStep(InterpEd->SnapAmount, PixelsPerSec, 6);
	}

	INT LineNum = appFloor(ViewStartTime/GridSpacing);
	while( LineNum*GridSpacing < ViewEndTime )
	{
		FLOAT LineTime = LineNum*GridSpacing;
		INT LinePosX = LabelWidth + (LineTime - ViewStartTime) * PixelsPerSec;
		
		FColor LineColor(125,125,125);
		
		// Change line color for important frames.
		if ( InterpEd->bSnapToFrames && LineNum % FrameStep == 0 )
		{
			LineColor = FColor(170,170,170);
		}

		if(bDrawTimeline)
		{
			
			// Show time or important frames' numbers (based on FrameStep).
			if ( !InterpEd->bSnapToFrames || Abs(LineNum) % FrameStep == 0 )
			{				
				// Draw grid lines and labels in timeline section.
				if( Canvas->IsHitTesting() ) Canvas->SetHitProxy( new HInterpEdTimelineBkg() );						


				FString Label;
				if (InterpEd->bSnapToFrames)
				{
					// Show frames' numbers.
					Label = FString::Printf( TEXT("%d"), LineNum );
				}
				else
				{
					// Show time.
					Label = FString::Printf( TEXT("%3.2f"), LineTime);
				}
				DrawLine2D(Canvas, FVector2D(LinePosX, ViewY - TotalBarHeight), FVector2D(LinePosX, ViewY), LineColor );
				DrawString(Canvas, LinePosX + 2, ViewY - NavHeight - 17, *Label, GEngine->SmallFont, FColor(200, 200, 200) );

				if( Canvas->IsHitTesting() ) Canvas->SetHitProxy( NULL );
			}						
		}
		else
		{
			// Draw grid lines in track view section. 
			if (!InterpEd->bSnapToFrames || (Abs(LineNum) % AuxFrameStep == 0))
			{
				DrawLine2D(Canvas, FVector2D(LinePosX, 0), FVector2D(LinePosX, ViewY - TotalBarHeight), LineColor );
			}
		}		
		LineNum++;
	}
	
}

/** Draw the timeline control at the bottom of the editor. */
void FInterpEdViewportClient::DrawTimeline(FViewport* Viewport, FCanvas* Canvas)
{
	INT ViewX = Viewport->GetSizeX();
	INT ViewY = Viewport->GetSizeY();

	//////// DRAW TIMELINE
	// Entire length is clickable.

	if( Canvas->IsHitTesting() ) Canvas->SetHitProxy( new HInterpEdTimelineBkg() );
	DrawTile(Canvas,LabelWidth, ViewY - TotalBarHeight, ViewX - LabelWidth, TimelineHeight, 0.f, 0.f, 0.f, 0.f, FColor(100,100,100) );
	if( Canvas->IsHitTesting() ) Canvas->SetHitProxy( NULL );

	DrawGrid(Viewport, Canvas, true);

	// Draw black line separating nav from timeline.
	DrawTile(Canvas,0, ViewY - TotalBarHeight, ViewX, 1, 0.f, 0.f, 0.f, 0.f, FLinearColor::Black );

	DrawMarkers(Viewport, Canvas);

	//////// DRAW NAVIGATOR
	INT ViewStart = LabelWidth + ViewStartTime * NavPixelsPerSecond;
	INT ViewEnd = LabelWidth + ViewEndTime * NavPixelsPerSecond;

	if( Canvas->IsHitTesting() ) Canvas->SetHitProxy( new HInterpEdNavigator() );
	DrawTile(Canvas,LabelWidth, ViewY - NavHeight, ViewX - LabelWidth, NavHeight, 0.f, 0.f, 0.f, 0.f, FColor(140,140,140) );
	DrawTile(Canvas,0, ViewY - NavHeight, ViewX, 1, 0.f, 0.f, 0.f, 0.f, FLinearColor::Black );

	DrawTile(Canvas, ViewStart, ViewY - NavHeight, ViewEnd - ViewStart, NavHeight, 0.f, 0.f, 1.f, 1.f, FLinearColor::Black );
	DrawTile(Canvas, ViewStart+1, ViewY - NavHeight + 1, ViewEnd - ViewStart - 2, NavHeight - 2, 0.f, 0.f, 1.f, 1.f, FLinearColor::White );
	if( Canvas->IsHitTesting() ) Canvas->SetHitProxy( NULL );


	// Tick indicating current position in global navigator
	DrawTile(Canvas, LabelWidth + InterpEd->Interp->Position * NavPixelsPerSecond, ViewY - 0.5*NavHeight - 4, 2, 8, 0.f, 0.f, 0.f, 0.f, FColor(80,80,80) );

	//////// DRAW INFO BOX

	DrawTile(Canvas, 0, ViewY - TotalBarHeight, LabelWidth, TotalBarHeight, 0.f, 0.f, 1.f, 1.f, FLinearColor::Black );

	// Draw current time in bottom left.
	INT XL, YL;

	FString PosString = FString::Printf( TEXT("%3.3f / %3.3f %s"), InterpEd->Interp->Position, InterpEd->IData->InterpLength, *LocalizeUnrealEd("Seconds") );
	StringSize( GEngine->SmallFont, XL, YL, *PosString );
	DrawString(Canvas, HeadTitleMargin, ViewY - YL - HeadTitleMargin, *PosString, GEngine->SmallFont, FLinearColor(0,1,0) );

	FString SnapPosString = "";

	const INT SelIndex = InterpEd->ToolBar->SnapCombo->GetSelection();

	// Determine if time should be drawn including frames or keys
	if(SelIndex == ARRAY_COUNT(InterpEdSnapSizes)+ARRAY_COUNT(InterpEdFPSSnapSizes))
	{
		UInterpTrack* Track = NULL;
		INT SelKeyIndex = 0;

		// keys
		SnapPosString = FString::Printf( TEXT("%3.0f %s"), 0.0, *LocalizeUnrealEd("KeyFrames") );

		// Work with the selected keys in a given track for a given group
		if(InterpEd->ActiveGroup && (InterpEd->ActiveTrackIndex != INDEX_NONE))
		{
			Track = InterpEd->ActiveGroup->InterpTracks(InterpEd->ActiveTrackIndex);

			if(InterpEd->Opt->SelectedKeys.Num() > 0)
			{
				FInterpEdSelKey& SelKey = InterpEd->Opt->SelectedKeys(0);
				SelKeyIndex = SelKey.KeyIndex + 1;
			}

			if(Track)
			{
				SnapPosString = FString::Printf( TEXT("%3.0f / %3.0f %s"), SelKeyIndex * 1.0, Track->GetNumKeyframes() * 1.0, *LocalizeUnrealEd("KeyFrames") );
			}
		}

		StringSize( GEngine->SmallFont, XL, YL, *SnapPosString );
	}
	else if(SelIndex < ARRAY_COUNT(InterpEdFPSSnapSizes)+ARRAY_COUNT(InterpEdSnapSizes) && SelIndex >= ARRAY_COUNT(InterpEdSnapSizes))
	{
		// frames
		SnapPosString = FString::Printf( TEXT("%3.1f / %3.1f %s"), (1.0 / InterpEd->SnapAmount) * InterpEd->Interp->Position, (1.0 / InterpEd->SnapAmount) * InterpEd->IData->InterpLength, *LocalizeUnrealEd("Frames") );
		StringSize( GEngine->SmallFont, XL, YL, *SnapPosString );
	}
	else if(SelIndex < ARRAY_COUNT(InterpEdSnapSizes))
	{
		// seconds
		SnapPosString = "";
	}
	else
	{
		// nothing
		SnapPosString = "";
	}
	DrawString(Canvas, HeadTitleMargin, ViewY - YL - INT(2.5 * YL) - HeadTitleMargin, *SnapPosString, GEngine->SmallFont, FLinearColor(0,1,0) );

	// If adjusting current keyframe - draw little record message in bottom-left
	if(InterpEd->bAdjustingKeyframe)
	{
		FLinkedObjDrawUtils::DrawNGon(Canvas, FIntPoint(HeadTitleMargin + 5, ViewY - 1.5*YL - 2*HeadTitleMargin), FColor(255,0,0), 12, 5);

		check(InterpEd->Opt->SelectedKeys.Num() == 1);
		FInterpEdSelKey& SelKey = InterpEd->Opt->SelectedKeys(0);
		FString AdjustString = FString::Printf( *LocalizeUnrealEd("Key_F"), SelKey.KeyIndex );

		DrawString(Canvas, 2*HeadTitleMargin + 10, ViewY - 2*YL - 2*HeadTitleMargin, *AdjustString, GEngine->SmallFont, FColor(255,0,0));
	}

	///////// DRAW SELECTED KEY RANGE

	if(InterpEd->Opt->SelectedKeys.Num() > 0)
	{
		FLOAT KeyStartTime, KeyEndTime;
		InterpEd->CalcSelectedKeyRange(KeyStartTime, KeyEndTime);

		FLOAT KeyRange = KeyEndTime - KeyStartTime;
		if( KeyRange > KINDA_SMALL_NUMBER && (KeyStartTime < ViewEndTime) && (KeyEndTime > ViewStartTime) )
		{
			// Find screen position of beginning and end of range.
			INT KeyStartX = LabelWidth + (KeyStartTime - ViewStartTime) * PixelsPerSec;
			INT ClipKeyStartX = ::Max(KeyStartX, LabelWidth);

			INT KeyEndX = LabelWidth + (KeyEndTime - ViewStartTime) * PixelsPerSec;
			INT ClipKeyEndX = ::Min(KeyEndX, ViewX);

			// Draw vertical ticks
			if(KeyStartX >= LabelWidth)
			{
				DrawLine2D(Canvas,FVector2D(KeyStartX, ViewY - TotalBarHeight - RangeTickHeight), FVector2D(KeyStartX, ViewY - TotalBarHeight), KeyRangeMarkerColor);

				// Draw time above tick.
				FString StartString = FString::Printf( TEXT("%3.2fs"), KeyStartTime );
				if ( InterpEd->bSnapToFrames )
				{
					StartString += FString::Printf( TEXT(" / %df"), appRound(KeyStartTime / InterpEd->SnapAmount));
				}
				StringSize( GEngine->SmallFont, XL, YL, *StartString);
				DrawShadowedString(Canvas, KeyStartX - XL, ViewY - TotalBarHeight - RangeTickHeight - YL - 2, *StartString, GEngine->SmallFont, KeyRangeMarkerColor );
			}

			if(KeyEndX <= ViewX)
			{
				DrawLine2D(Canvas,FVector2D(KeyEndX, ViewY - TotalBarHeight - RangeTickHeight), FVector2D(KeyEndX, ViewY - TotalBarHeight), KeyRangeMarkerColor);

				// Draw time above tick.
				FString EndString = FString::Printf( TEXT("%3.2fs"), KeyEndTime );
				if ( InterpEd->bSnapToFrames )
				{
					EndString += FString::Printf( TEXT(" / %df"), appRound(KeyEndTime / InterpEd->SnapAmount));
				}

				StringSize( GEngine->SmallFont, XL, YL, *EndString);
				DrawShadowedString(Canvas, KeyEndX, ViewY - TotalBarHeight - RangeTickHeight - YL - 2, *EndString, GEngine->SmallFont, KeyRangeMarkerColor );
			}

			// Draw line connecting them.
			INT RangeLineY = ViewY - TotalBarHeight - 0.5f*RangeTickHeight;
			DrawLine2D(Canvas, FVector2D(ClipKeyStartX, RangeLineY), FVector2D(ClipKeyEndX, RangeLineY), KeyRangeMarkerColor);

			// Draw range label above line
			// First find size of range string
			FString RangeString = FString::Printf( TEXT("%3.2fs"), KeyRange );
			if ( InterpEd->bSnapToFrames )
			{
				RangeString += FString::Printf( TEXT(" / %df"), appRound(KeyRange / InterpEd->SnapAmount));
			}

			StringSize( GEngine->SmallFont, XL, YL, *RangeString);

			// Find X position to start label drawing.
			INT RangeLabelX = ClipKeyStartX + 0.5f*(ClipKeyEndX-ClipKeyStartX) - 0.5f*XL;
			INT RangeLabelY = ViewY - TotalBarHeight - RangeTickHeight - YL;

			DrawShadowedString(Canvas,RangeLabelX, RangeLabelY, *RangeString, GEngine->SmallFont, KeyRangeMarkerColor);
		}
		else
		{
			UInterpGroup* Group = InterpEd->Opt->SelectedKeys(0).Group;
			UInterpTrack* Track = Group->InterpTracks( InterpEd->Opt->SelectedKeys(0).TrackIndex );
			FLOAT KeyTime = Track->GetKeyframeTime( InterpEd->Opt->SelectedKeys(0).KeyIndex );

			INT KeyX = LabelWidth + (KeyTime - ViewStartTime) * PixelsPerSec;
			if((KeyX >= LabelWidth) && (KeyX <= ViewX))
			{
				DrawLine2D(Canvas,FVector2D(KeyX, ViewY - TotalBarHeight - RangeTickHeight), FVector2D(KeyX, ViewY - TotalBarHeight), KeyRangeMarkerColor);

				FString KeyString = FString::Printf( TEXT("%3.2fs"), KeyTime );				
				if ( InterpEd->bSnapToFrames )
				{
					KeyString += FString::Printf( TEXT(" / %df"), appRound(KeyTime / InterpEd->SnapAmount));
				}

				StringSize( GEngine->SmallFont, XL, YL, *KeyString);

				INT KeyLabelX = KeyX - 0.5f*XL;
				INT KeyLabelY = ViewY - TotalBarHeight - RangeTickHeight - YL - 3;

				DrawShadowedString(Canvas,KeyLabelX, KeyLabelY, *KeyString, GEngine->SmallFont, KeyRangeMarkerColor);		
			}
		}
	}
}

/** Draw various markers on the timeline */
void FInterpEdViewportClient::DrawMarkers(FViewport* Viewport, FCanvas* Canvas)
{
	INT ViewX = Viewport->GetSizeX();
	INT ViewY = Viewport->GetSizeY();
	INT ScaleTopY = ViewY - TotalBarHeight + 1;

	// Calculate screen X position that indicates current position in track.
	INT TrackPosX = LabelWidth + (InterpEd->Interp->Position - ViewStartTime) * PixelsPerSec;

	// Draw position indicator and line (if in viewed area)
	if( TrackPosX + TimeIndHalfWidth >= LabelWidth && TrackPosX <= ViewX)
	{
		if( Canvas->IsHitTesting() ) Canvas->SetHitProxy( new HInterpEdTimelineBkg() );
		DrawTile(Canvas, TrackPosX - TimeIndHalfWidth - 1, ScaleTopY, (2*TimeIndHalfWidth) + 1, TimelineHeight, 0.f, 0.f, 0.f, 0.f, FColor(10,10,10) );
		if( Canvas->IsHitTesting() ) Canvas->SetHitProxy( NULL );
	}

	INT MarkerArrowSize = 8;

	FIntPoint StartA = FIntPoint(0,					ScaleTopY);
	FIntPoint StartB = FIntPoint(0,					ScaleTopY+MarkerArrowSize);
	FIntPoint StartC = FIntPoint(-MarkerArrowSize,	ScaleTopY);

	FIntPoint EndA = FIntPoint(0,					ScaleTopY);
	FIntPoint EndB = FIntPoint(MarkerArrowSize,		ScaleTopY);
	FIntPoint EndC = FIntPoint(0,					ScaleTopY+MarkerArrowSize);




	// Draw loop section start/end
	FIntPoint EdStartPos( LabelWidth + (InterpEd->IData->EdSectionStart - ViewStartTime) * PixelsPerSec, MarkerArrowSize );
	if(Canvas->IsHitTesting()) Canvas->SetHitProxy( new HInterpEdMarker(ISM_LoopStart) );
	DrawTriangle2D(Canvas, StartA + EdStartPos, FVector2D(0,0), StartB + EdStartPos, FVector2D(0,0), StartC + EdStartPos, FVector2D(0,0), SectionMarkerColor );
	if(Canvas->IsHitTesting()) Canvas->SetHitProxy( NULL );


	FIntPoint EdEndPos( LabelWidth + (InterpEd->IData->EdSectionEnd - ViewStartTime) * PixelsPerSec, MarkerArrowSize );
	if(Canvas->IsHitTesting()) Canvas->SetHitProxy( new HInterpEdMarker(ISM_LoopEnd) );
	DrawTriangle2D(Canvas, EndA + EdEndPos, FVector2D(0,0), EndB + EdEndPos, FVector2D(0,0), EndC + EdEndPos, FVector2D(0,0), SectionMarkerColor );
	if(Canvas->IsHitTesting()) Canvas->SetHitProxy( NULL );

	// Draw sequence start/end markers.
	FIntPoint StartPos( LabelWidth + (0.f - ViewStartTime) * PixelsPerSec, 0 );
	if(Canvas->IsHitTesting()) Canvas->SetHitProxy( new HInterpEdMarker(ISM_SeqStart) );
	DrawTriangle2D( Canvas, StartA + StartPos, FVector2D(0,0), StartB + StartPos, FVector2D(0,0), StartC + StartPos, FVector2D(0,0), InterpMarkerColor );
	if(Canvas->IsHitTesting()) Canvas->SetHitProxy( NULL );

	FIntPoint EndPos( LabelWidth + (InterpEd->IData->InterpLength - ViewStartTime) * PixelsPerSec, 0 );
	if(Canvas->IsHitTesting()) Canvas->SetHitProxy( new HInterpEdMarker(ISM_SeqEnd) );
	DrawTriangle2D( Canvas,  EndA + EndPos, FVector2D(0,0), EndB + EndPos, FVector2D(0,0), EndC + EndPos, FVector2D(0,0), InterpMarkerColor );
	if(Canvas->IsHitTesting()) Canvas->SetHitProxy( NULL );

	// Draw little tick indicating path-building time.
	INT PathBuildPosX = LabelWidth + (InterpEd->IData->PathBuildTime - ViewStartTime) * PixelsPerSec;
	if( PathBuildPosX >= LabelWidth && PathBuildPosX <= ViewX)
	{
		DrawTile(Canvas, PathBuildPosX, ViewY - NavHeight - 10, 1, 11, 0.f, 0.f, 0.f, 0.f, FColor(200, 200, 255) );
	}

}

namespace
{
	const FColor TabColorNormal(128,128,128);
	const FColor TabColorSelected(192,160,128);
	const INT TabPadding = 1;
	const INT TabSpacing = 4;
	const INT TabRowHeight = 22;
}

/** Draw the track editor using the supplied 2D RenderInterface. */
void FInterpEdViewportClient::Draw(FViewport* Viewport, FCanvas* Canvas)
{
	Canvas->PushAbsoluteTransform(FMatrix::Identity);

	// Erase background
	Clear(Canvas, FColor(162,162,162) );

	INT ViewX = Viewport->GetSizeX();
	INT ViewY = Viewport->GetSizeY();

	TrackViewSizeX = ViewX - LabelWidth;

	// Calculate ratio between screen pixels and elapsed time.
	PixelsPerSec = FLOAT(ViewX - LabelWidth)/FLOAT(ViewEndTime - ViewStartTime);
	NavPixelsPerSecond = FLOAT(ViewX - LabelWidth)/InterpEd->IData->InterpLength;

	DrawGrid(Viewport, Canvas, false);

	// Draw 'null regions' if viewing past start or end.
	INT StartPosX = LabelWidth + (0.f - ViewStartTime) * PixelsPerSec;
	INT EndPosX = LabelWidth + (InterpEd->IData->InterpLength - ViewStartTime) * PixelsPerSec;
	if(ViewStartTime < 0.f)
	{
		DrawTile(Canvas,0, 0, StartPosX, ViewY, 0.f, 0.f, 1.f, 1.f, NullRegionColor);
	}

	if(ViewEndTime > InterpEd->IData->InterpLength)
	{
		DrawTile(Canvas,EndPosX, 0, ViewX-EndPosX, ViewY, 0.f, 0.f, 1.f, 1.f, NullRegionColor);
	}

	// Draw lines on borders of 'null area'
	if(ViewStartTime < 0.f)
	{
		DrawLine2D(Canvas,FVector2D(StartPosX, 0), FVector2D(StartPosX, ViewY - TotalBarHeight), NullRegionBorderColor);
	}

	if(ViewEndTime > InterpEd->IData->InterpLength)
	{
		DrawLine2D(Canvas,FVector2D(EndPosX, 0), FVector2D(EndPosX, ViewY - TotalBarHeight), NullRegionBorderColor);
	}

	// Draw loop region.
	INT EdStartPosX = LabelWidth + (InterpEd->IData->EdSectionStart - ViewStartTime) * PixelsPerSec;
	INT EdEndPosX = LabelWidth + (InterpEd->IData->EdSectionEnd - ViewStartTime) * PixelsPerSec;

	DrawTile(Canvas,EdStartPosX, 0, EdEndPosX - EdStartPosX, ViewY - TotalBarHeight, 0.f, 0.f, 1.f, 1.f, InterpEd->RegionFillColor);

	// Draw titles block down left.
	if(Canvas->IsHitTesting()) Canvas->SetHitProxy( new HInterpEdTrackBkg() );
	DrawTile(Canvas, 0, 0, LabelWidth, ViewY - TotalBarHeight, 0.f, 0.f, 0.f, 0.f, FColor(160,160,160) );
	if(Canvas->IsHitTesting()) Canvas->SetHitProxy( NULL );

	// Draw Filter Tabs
	DrawTabs(Viewport, Canvas);

	// Draw visible groups/tracks
	INT YOffset = InterpEd->ThumbPos_Vert+TabRowHeight;

	// Get materials for cam-locked icon.
	UMaterial* CamLockedIcon = (UMaterial*)UObject::StaticLoadObject( UMaterial::StaticClass(), NULL, TEXT("EditorMaterials.MatineeGroups.MAT_Groups_View_On_Mat"), NULL, LOAD_None, NULL );
	check(CamLockedIcon);

	UMaterial* CamUnlockedIcon = (UMaterial*)UObject::StaticLoadObject( UMaterial::StaticClass(), NULL, TEXT("EditorMaterials.MatineeGroups.MAT_Groups_View_Off_Mat"), NULL, LOAD_None, NULL );
	check(CamUnlockedIcon);

	// Get materials for Event direction buttons.
	UMaterial* ForwardEventOnMat = (UMaterial*)UObject::StaticLoadObject( UMaterial::StaticClass(), NULL, TEXT("EditorMaterials.MatineeGroups.MAT_Groups_Right_On_Mat"), NULL, LOAD_None, NULL );
	check(ForwardEventOnMat);

	UMaterial* ForwardEventOffMat = (UMaterial*)UObject::StaticLoadObject( UMaterial::StaticClass(), NULL, TEXT("EditorMaterials.MatineeGroups.MAT_Groups_Right_Off_Mat"), NULL, LOAD_None, NULL );
	check(ForwardEventOffMat);

	UMaterial* BackwardEventOnMat = (UMaterial*)UObject::StaticLoadObject( UMaterial::StaticClass(), NULL, TEXT("EditorMaterials.MatineeGroups.MAT_Groups_Left_On_Mat"), NULL, LOAD_None, NULL );
	check(BackwardEventOnMat);

	UMaterial* BackwardEventOffMat = (UMaterial*)UObject::StaticLoadObject( UMaterial::StaticClass(), NULL, TEXT("EditorMaterials.MatineeGroups.MAT_Groups_Left_Off_Mat"), NULL, LOAD_None, NULL );
	check(BackwardEventOffMat);

	UMaterial* DisableTrackMat = (UMaterial*)UObject::StaticLoadObject( UMaterial::StaticClass(), NULL, TEXT("EditorMaterials.Matinee.MAT_EnableTrack_Mat"), NULL, LOAD_None, NULL );
	check(DisableTrackMat);


	// Get materials for sending to curve editor
	UMaterial* GraphOnMat = (UMaterial*)UObject::StaticLoadObject( UMaterial::StaticClass(), NULL, TEXT("EditorMaterials.MatineeGroups.MAT_Groups_Graph_On_Mat"), NULL, LOAD_None, NULL );
	check(GraphOnMat);

	UMaterial* GraphOffMat = (UMaterial*)UObject::StaticLoadObject( UMaterial::StaticClass(), NULL, TEXT("EditorMaterials.MatineeGroups.MAT_Groups_Graph_Off_Mat"), NULL, LOAD_None, NULL );
	check(GraphOffMat);

	for(INT i=0; i<InterpEd->IData->InterpGroups.Num(); i++)
	{
		// Draw group header
		UInterpGroup* Group = InterpEd->IData->InterpGroups(i);

		// Only draw if the group isn't filtered.
		if(Group->bVisible)
		{
			Canvas->PushRelativeTransform(FTranslationMatrix(FVector(0,YOffset,0)));

			FColor GroupUnselectedColor = (Group->IsA(UInterpGroupDirector::StaticClass())) ? DirHeadUnselectedColor : HeadUnselectedColor;
			FColor GroupColor = (Group == InterpEd->ActiveGroup) ? HeadSelectedColor : GroupUnselectedColor;

			if(Canvas->IsHitTesting()) Canvas->SetHitProxy( new HInterpEdGroupTitle(Group) );
			DrawTile(Canvas, 0, 0, ViewX, GroupHeadHeight, 0.f, 0.f, 1.f, 1.f, GroupColor, InterpEd->BarGradText->Resource );
			if(Canvas->IsHitTesting()) Canvas->SetHitProxy( NULL );

			// Draw little collapse-group widget
			INT HalfColArrowSize = 6;
			FIntPoint A,B,C;
		
			if(Group->bCollapsed)
			{
				A = FIntPoint(HeadTitleMargin + HalfColArrowSize,		0.5*GroupHeadHeight - HalfColArrowSize);
				B = FIntPoint(HeadTitleMargin + HalfColArrowSize,		0.5*GroupHeadHeight + HalfColArrowSize);
				C = FIntPoint(HeadTitleMargin + 2*HalfColArrowSize,		0.5*GroupHeadHeight);
			}
			else
			{
				A = FIntPoint(HeadTitleMargin,							0.5*GroupHeadHeight);
				B = FIntPoint(HeadTitleMargin + HalfColArrowSize,		0.5*GroupHeadHeight + HalfColArrowSize);
				C = FIntPoint(HeadTitleMargin + 2*HalfColArrowSize,		0.5*GroupHeadHeight);
			}

			if(Canvas->IsHitTesting()) Canvas->SetHitProxy( new HInterpEdGroupCollapseBtn(Group) );
			DrawTriangle2D(Canvas, A, FVector2D(0.f, 0.f), B, FVector2D(0.f, 0.f), C, FVector2D(0.f, 0.f), FLinearColor::Black );
			if(Canvas->IsHitTesting()) Canvas->SetHitProxy( NULL );

			// Draw the group name
			INT XL, YL;
			StringSize(GEngine->SmallFont, XL, YL, *Group->GroupName.ToString());
			FColor GroupNameColor = Group->IsA(UInterpGroupDirector::StaticClass()) ? FColor(255,255,128) : FColor(255,255,255);
			DrawShadowedString(Canvas, 2*HeadTitleMargin + 2*HalfColArrowSize, 0.5*GroupHeadHeight - 0.5*YL, *Group->GroupName.ToString(), GEngine->SmallFont, GroupNameColor );
			
			// Draw button for binding camera to this group.

			UMaterial* ButtonMat = (Group == InterpEd->CamViewGroup) ? CamLockedIcon : CamUnlockedIcon;
			if(Canvas->IsHitTesting()) Canvas->SetHitProxy( new HInterpEdGroupLockCamBtn(Group) );
			DrawTile(Canvas, LabelWidth - 26, (0.5*GroupHeadHeight)-8, 16, 16, 0.f, 0.f, 1.f, 1.f, ButtonMat->GetRenderProxy(0) );
			if(Canvas->IsHitTesting()) Canvas->SetHitProxy( NULL );

			// Draw little bar showing group colour
			DrawTile(Canvas, LabelWidth - 6, 0, 6, GroupHeadHeight, 0.f, 0.f, 1.f, 1.f, Group->GroupColor, InterpEd->BarGradText->Resource );

			// Draw line under group header
			DrawTile(Canvas, 0, GroupHeadHeight-1, ViewX, 1, 0.f, 0.f, 1.f, 1.f, FLinearColor::Black );
				
			YOffset += GroupHeadHeight;
		
			Canvas->PopTransform();

			if(!Group->bCollapsed)
			{
				// Draw each track in this group.
				for(INT j=0; j<Group->InterpTracks.Num(); j++)
				{
					UInterpTrack* Track = Group->InterpTracks(j);

					Canvas->PushRelativeTransform(FTranslationMatrix(FVector(LabelWidth,YOffset,0)));

					UBOOL bTrackSelected = false;
					if( Group == InterpEd->ActiveGroup && j == InterpEd->ActiveTrackIndex )
						bTrackSelected = true;

					FColor TrackColor = bTrackSelected ? HeadSelectedColor : TrackUnselectedColor;

					// Call virtual function to draw actual track data.
					Track->DrawTrack( Canvas, j, ViewX - LabelWidth, TrackHeight-1, ViewStartTime, PixelsPerSec, InterpEd->Opt->SelectedKeys );

					// Track title block on left.
					if(Canvas->IsHitTesting()) Canvas->SetHitProxy( new HInterpEdTrackTitle(Group, j) );
					DrawTile(Canvas, -LabelWidth, 0, LabelWidth, TrackHeight - 1, 0.f, 0.f, 1.f, 1.f, TrackColor, InterpEd->BarGradText->Resource );

					// Draw Track Icon
					UMaterial* TrackIconMat = Track->GetTrackIcon();
					check(TrackIconMat);
					DrawTile(Canvas, -LabelWidth + 24, 0.5*(TrackHeight - 16), 16, 16, 0.f, 0.f, 1.f, 1.f, TrackIconMat->GetRenderProxy(0) );

					// Truncate from front if name is too long
					FString TrackTitle = FString( *Track->TrackTitle );
					StringSize(GEngine->SmallFont, XL, YL, *TrackTitle);

					// If too long to fit in label - truncate. TODO: Actually truncate by necessary amount!
					if(XL > LabelWidth - TrackTitleMargin - 2)
					{
						TrackTitle = FString::Printf( TEXT("...%s"), *(TrackTitle.Right(13)) );
						StringSize(GEngine->SmallFont, XL, YL, *TrackTitle);
					}

					FLinearColor TextColor;

					if(Track->bDisableTrack == FALSE)
					{
						TextColor = FLinearColor::White;
					}
					else
					{
						TextColor = FLinearColor(0.5f,0.5f,0.5f);
					}

					DrawShadowedString(Canvas, -LabelWidth + TrackTitleMargin, 0.5*TrackHeight - 0.5*YL, *TrackTitle, GEngine->SmallFont,  TextColor);
					if(Canvas->IsHitTesting()) Canvas->SetHitProxy( NULL );

					UInterpTrackEvent* EventTrack = Cast<UInterpTrackEvent>(Track);
					if(EventTrack)
					{
						UMaterial* ForwardMat = (EventTrack->bFireEventsWhenForwards) ? ForwardEventOnMat : ForwardEventOffMat;
						UMaterial* BackwardMat = (EventTrack->bFireEventsWhenBackwards) ? BackwardEventOnMat : BackwardEventOffMat;

						if(Canvas->IsHitTesting()) Canvas->SetHitProxy( new HInterpEdEventDirBtn(Group, j, IED_Backward) );
						DrawTile(Canvas, -32, TrackHeight-11, 8, 8, 0.f, 0.f, 1.f, 1.f, BackwardMat->GetRenderProxy(0) );
						if(Canvas->IsHitTesting()) Canvas->SetHitProxy( NULL );

						if(Canvas->IsHitTesting()) Canvas->SetHitProxy( new HInterpEdEventDirBtn(Group, j, IED_Forward) );
						DrawTile(Canvas, -22, TrackHeight-11, 8, 8, 0.f, 0.f, 1.f, 1.f, ForwardMat->GetRenderProxy(0) );
						if(Canvas->IsHitTesting()) Canvas->SetHitProxy( NULL );
					}

					if( Track->IsA(UInterpTrackFloatBase::StaticClass()) || Track->IsA(UInterpTrackVectorBase::StaticClass()) || Track->IsA(UInterpTrackMove::StaticClass()) )
					{
						UMaterial* GraphMat = InterpEd->IData->CurveEdSetup->ShowingCurve(Track) ? GraphOnMat : GraphOffMat;

						// Draw button for pushing properties onto graph view.
						if(Canvas->IsHitTesting()) Canvas->SetHitProxy( new HInterpEdTrackGraphPropBtn(Group, j) );
						DrawTile(Canvas, -22, TrackHeight-11, 8, 8, 0.f, 0.f, 1.f, 1.f, GraphMat->GetRenderProxy(0) );
						if(Canvas->IsHitTesting()) Canvas->SetHitProxy( NULL );
					}

					// Draw line under each track
					DrawTile(Canvas, -LabelWidth, TrackHeight - 1, ViewX, 1, 0.f, 0.f, 1.f, 1.f, FLinearColor::Black );

					// Draw an icon to let the user enable/disable a track.
					if(Canvas->IsHitTesting()) Canvas->SetHitProxy( new HInterpEdTrackDisableTrackBtn(Group, j) );
					{
						FVector2D DisableIconSize(16,16);
						FLOAT YPos = (TrackHeight - DisableIconSize.Y) / 2.0f;
						DrawTile(Canvas, -LabelWidth+4, YPos, DisableIconSize.X, DisableIconSize.Y, 0,0,1,1, FLinearColor::Black);

						if(Track->bDisableTrack == FALSE)
						{
							DrawTile(Canvas, -LabelWidth+4, YPos, DisableIconSize.X, DisableIconSize.Y, 0,0,1,1, DisableTrackMat->GetRenderProxy(0));
						}
					}			
					if(Canvas->IsHitTesting()) Canvas->SetHitProxy( NULL );

					YOffset += TrackHeight;

					Canvas->PopTransform();
				}
			}
			else
			{

				const FVector2D TickSize(2.0f, GroupHeadHeight * 0.5f);

				// Since the track is collapsed, draw ticks for each of the track's keyframes.
				for(INT TrackIdx=0; TrackIdx<Group->InterpTracks.Num(); TrackIdx++)
				{
					UInterpTrack* Track = Group->InterpTracks(TrackIdx);

					FVector2D TrackPos(LabelWidth - ViewStartTime * PixelsPerSec, YOffset - GroupHeadHeight);

					Canvas->PushRelativeTransform(FTranslationMatrix(FVector(TrackPos.X,TrackPos.Y,0)));
					{
						for(INT KeyframeIdx=0; KeyframeIdx<Track->GetNumKeyframes(); KeyframeIdx++)
						{
							FLOAT KeyframeTime = Track->GetKeyframeTime(KeyframeIdx);
							FColor KeyframeColor = Track->GetKeyframeColor(KeyframeIdx);
							FVector2D TickPos;

							TickPos.X =  -TickSize.X / 2.0f + KeyframeTime * PixelsPerSec;
							TickPos.Y =  (GroupHeadHeight - 1 - TickSize.Y) / 2.0f;
						

							// Draw a tick mark.
							if(TickPos.X >= ViewStartTime * PixelsPerSec)
							{
								DrawTile(Canvas, TickPos.X, TickPos.Y, TickSize.X, TickSize.Y, 0.f, 0.f, 1.f, 1.f, KeyframeColor );
							}
						}
					}
					Canvas->PopTransform();
				}
			}
		}
	}


	// Draw grid and timeline stuff.
	DrawTimeline(Viewport, Canvas);

	// Draw line between title block and track view for empty space
	DrawTile(Canvas, LabelWidth, YOffset-1, 1, ViewY - YOffset, 0.f, 0.f, 0.f, 0.f, FLinearColor::Black );

	// Draw snap-to line, if mouse button is down.
	if(bMouseDown && bDrawSnappingLine)
	{
		INT SnapPosX = LabelWidth + (SnappingLinePosition - ViewStartTime) * PixelsPerSec;
		DrawLine2D(Canvas, FVector2D(SnapPosX, 0), FVector2D(SnapPosX, ViewY - TotalBarHeight), FColor(0,0,0));
	}
	else
	{
		bDrawSnappingLine = false;
	}

	// Draw vertical position line
	INT TrackPosX = LabelWidth + (InterpEd->Interp->Position - ViewStartTime) * PixelsPerSec;
	if( TrackPosX >= LabelWidth && TrackPosX <= ViewX)
	{
		DrawLine2D(Canvas, FVector2D(TrackPosX, 0), FVector2D(TrackPosX, ViewY - TotalBarHeight), InterpEd->PosMarkerColor);
	}

	// Draw the box select box
	if(bBoxSelecting)
	{
		INT MinX = Min(BoxStartX, BoxEndX);
		INT MinY = Min(BoxStartY, BoxEndY);
		INT MaxX = Max(BoxStartX, BoxEndX);
		INT MaxY = Max(BoxStartY, BoxEndY);

		DrawLine2D(Canvas,FVector2D(MinX, MinY), FVector2D(MaxX, MinY), FColor(255,0,0));
		DrawLine2D(Canvas,FVector2D(MaxX, MinY), FVector2D(MaxX, MaxY), FColor(255,0,0));
		DrawLine2D(Canvas,FVector2D(MaxX, MaxY), FVector2D(MinX, MaxY), FColor(255,0,0));
		DrawLine2D(Canvas,FVector2D(MinX, MaxY), FVector2D(MinX, MinY), FColor(255,0,0));
	}

	// Draw filter tabs
	DrawTabs(Viewport, Canvas);

	Canvas->PopTransform();
}

/**
 * Draws filter tabs for the matinee editor.
 */
void FInterpEdViewportClient::DrawTabs(FViewport* Viewport,FCanvas* Canvas)
{
	// Draw tab background
	DrawTile(Canvas, 0,0, Viewport->GetSizeX(), TabRowHeight, 0,0,1,1, FColor(64,64,64));
	
	// Draw all tabs
	INT TabOffset = TabSpacing;

	// Draw static filters first
	for(INT TabIdx=0; TabIdx<InterpEd->IData->DefaultFilters.Num(); TabIdx++)
	{
		UInterpFilter* Filter = InterpEd->IData->DefaultFilters(TabIdx);
		FVector2D TabSize = DrawTab(Viewport, Canvas, TabOffset, Filter);
		TabOffset += TabSize.X + TabSpacing;
	}

	// Draw user custom filters last.
	for(INT TabIdx=0; TabIdx<InterpEd->IData->InterpFilters.Num(); TabIdx++)
	{
		UInterpFilter* Filter = InterpEd->IData->InterpFilters(TabIdx);
		FVector2D TabSize = DrawTab(Viewport, Canvas, TabOffset, Filter);
		TabOffset += TabSize.X + TabSpacing;
	}
}

/**
 * Draws a filter tab for matinee.
 *
 * @return Size of the tab that was drawn.
 */
FVector2D FInterpEdViewportClient::DrawTab(FViewport* Viewport, FCanvas* Canvas, INT &TabOffset, UInterpFilter* Filter)
{
	FColor TabColor = (Filter == InterpEd->IData->SelectedFilter) ? TabColorSelected : TabColorNormal;
	FVector2D TabPosition(TabOffset,TabSpacing);
	FVector2D TabSize(0,16);

	FIntPoint TabCaptionSize;
	StringSize(GEngine->TinyFont, TabCaptionSize.X, TabCaptionSize.Y, *Filter->Caption);
	TabSize.X = TabCaptionSize.X + 2 + TabPadding*2;
	TabSize.Y = TabCaptionSize.Y + 2 + TabPadding*2;

	Canvas->PushRelativeTransform(FTranslationMatrix(FVector(TabPosition.X, TabPosition.Y,0)));
	{
		Canvas->SetHitProxy(new HInterpEdTab(Filter));
		{
			DrawTile(Canvas, 0, 0, TabSize.X, TabSize.Y, 0, 0, 1, 1, TabColor);
			DrawLine2D(Canvas, FVector2D(0, 0), FVector2D(0, TabSize.Y), FColor(192,192,192));				// Left
			DrawLine2D(Canvas, FVector2D(0, 0), FVector2D(TabSize.X, 0), FColor(192,192,192));				// Top
			DrawLine2D(Canvas, FVector2D(TabSize.X, 0), FVector2D(TabSize.X, TabSize.Y), FColor(0,0,0));	// Right	
			DrawLine2D(Canvas, FVector2D(0, TabSize.Y), FVector2D(TabSize.X, TabSize.Y), FColor(0,0,0));	// Bottom
			DrawString(Canvas, TabPadding, TabPadding, *Filter->Caption, GEngine->TinyFont, FColor(0,0,0));
		}
		Canvas->SetHitProxy(NULL);
	}
	Canvas->PopTransform();

	return TabSize;
}
