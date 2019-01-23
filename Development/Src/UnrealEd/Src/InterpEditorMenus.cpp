/*=============================================================================
	InterpEditorMenus.cpp: Interpolation editing menus
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "EngineSequenceClasses.h"
#include "EngineInterpolationClasses.h"
#include "InterpEditor.h"
#include "UnLinkedObjDrawUtils.h"
#include "UnColladaExporter.h"
#include "UnColladaImporter.h"
#include "Properties.h"
#include "GenericBrowser.h"
#include "DlgGenericComboEntry.h"

///// MENU CALLBACKS

// Add a new keyframe on the selected track 
void WxInterpEd::OnMenuAddKey( wxCommandEvent& In )
{
	AddKey();
}

void WxInterpEd::OnContextNewGroup( wxCommandEvent& In )
{
	// Find out if we want to make a 'Director' group.
	const UBOOL bDirGroup = ( In.GetId() == IDM_INTERP_NEW_DIRECTOR_GROUP );
	const UBOOL bDuplicateGroup = ( In.GetId() == IDM_INTERP_GROUP_DUPLICATE );

	if(bDuplicateGroup && !ActiveGroup)
	{
		appMsgf( AMT_OK, *LocalizeUnrealEd("NoGroupToDuplicate") );
		return;
	}

	// This is temporary - need a unified way to associate tracks with components/actors etc... hmm..
	AActor* GroupActor = NULL;

	if(!bDirGroup && !bDuplicateGroup)
	{
		for ( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
		{
			AActor* Actor = static_cast<AActor*>( *It );
			checkSlow( Actor->IsA(AActor::StaticClass()) );

			GroupActor = Actor;
			break;
		}

		if(GroupActor)
		{
			// Check that the Outermost of both the Matinee action and the Actor are the same
			// We can't create a group for an Actor that is not in the same level as the sequence
			UObject* SequenceOutermost = Interp->GetOutermost();
			UObject* ActorOutermost = GroupActor->GetOutermost();
			if(ActorOutermost != SequenceOutermost)
			{
				appMsgf(AMT_OK, *LocalizeUnrealEd("Error_ActorNotInSequenceLevel"));
				return;
			}

			// Check we do not already have a group acting on this actor.
			for(INT i=0; i<Interp->GroupInst.Num(); i++)
			{
				if( GroupActor == Interp->GroupInst(i)->GroupActor )
				{
					appMsgf(AMT_OK, *LocalizeUnrealEd("Error_GroupAlreadyAssocWithActor"));
					return;
				}
			}
		}
	}

	FName NewGroupName;
	// If not a director group - ask for a name.
	if(!bDirGroup)
	{
		// Otherwise, pop up dialog to enter name.
		WxDlgGenericStringEntry dlg;
		const INT Result = dlg.ShowModal( TEXT("NewGroupName"), TEXT("NewGroupName"), TEXT("InterpGroup") );
		if( Result != wxID_OK )
		{
			return;
		}

		FString TempString = dlg.GetEnteredString();
		TempString = TempString.Replace(TEXT(" "),TEXT("_"));
		NewGroupName = FName( *TempString );
	}

	// Begin undo transaction
	InterpEdTrans->BeginSpecial( TEXT("NewGroup") );
	Interp->Modify();
	IData->Modify();

	// Create new InterpGroup.
	UInterpGroup* NewGroup = NULL;
	if(bDirGroup)
	{
		NewGroup = ConstructObject<UInterpGroupDirector>( UInterpGroupDirector::StaticClass(), IData, NAME_None, RF_Transactional );
	}
	else if(bDuplicateGroup)
	{
		NewGroup = (UInterpGroup*)UObject::StaticDuplicateObject( ActiveGroup, ActiveGroup, IData, TEXT("None"), RF_Transactional );
		NewGroup->GroupName = NewGroupName;
	}
	else
	{
		NewGroup = ConstructObject<UInterpGroup>( UInterpGroup::StaticClass(), IData, NAME_None, RF_Transactional );
		NewGroup->GroupName = NewGroupName;
	}
	IData->InterpGroups.AddItem(NewGroup);


	// All groups must have a unique name.
	NewGroup->EnsureUniqueName();

	// Randomly generate a group colour for the new group.
	NewGroup->GroupColor = FColor::MakeRandomColor();


	NewGroup->Modify();

	// Create new InterpGroupInst
	UInterpGroupInst* NewGroupInst = NULL;
	if(bDirGroup)
	{
		NewGroupInst = ConstructObject<UInterpGroupInstDirector>( UInterpGroupInstDirector::StaticClass(), Interp, NAME_None, RF_Transactional );
		NewGroupInst->InitGroupInst(NewGroup, NULL);
	}
	else
	{
		NewGroupInst = ConstructObject<UInterpGroupInst>( UInterpGroupInst::StaticClass(), Interp, NAME_None, RF_Transactional );
		// Initialise group instance, saving ref to actor it works on.
		NewGroupInst->InitGroupInst(NewGroup, GroupActor);
	}

	const INT NewGroupInstIndex = Interp->GroupInst.AddItem(NewGroupInst);

	NewGroupInst->Modify();

	// Don't need to save state here - no tracks!

	// If a director group, create a director track for it now.
	if(bDirGroup)
	{
		UInterpTrack* NewDirTrack = ConstructObject<UInterpTrackDirector>( UInterpTrackDirector::StaticClass(), NewGroup, NAME_None, RF_Transactional );
		NewGroup->InterpTracks.AddItem(NewDirTrack);

		UInterpTrackInst* NewDirTrackInst = ConstructObject<UInterpTrackInstDirector>( UInterpTrackInstDirector::StaticClass(), NewGroupInst, NAME_None, RF_Transactional );
		NewGroupInst->TrackInst.AddItem(NewDirTrackInst);

		NewDirTrackInst->InitTrackInst(NewDirTrack);
		NewDirTrackInst->SaveActorState(NewDirTrack);

		// Save for undo then redo.
		NewDirTrack->Modify();
		NewDirTrackInst->Modify();
	}
	// If regular track, create a new object variable connector, and variable containing selected actor if there is one.
	else
	{
		Interp->InitGroupActorForGroup(NewGroup, GroupActor);
	}

	InterpEdTrans->EndSpecial();

	// Update graphics to show new group.
	InterpEdVC->Viewport->Invalidate();

	// If adding a camera- make sure its frustum colour is updated.
	UpdateCamColours();

	// Reimage actor world locations.  This must happen after the group was created.
	Interp->RecaptureActorTransforms();
}



void WxInterpEd::OnContextNewTrack( wxCommandEvent& In )
{
	if( !ActiveGroup )
		return;

	// Find the class of the new track we want to add.
	const INT NewTrackClassIndex = In.GetId() - IDM_INTERP_NEW_TRACK_START;
	check( NewTrackClassIndex >= 0 && NewTrackClassIndex < InterpTrackClasses.Num() );

	UClass* NewInterpTrackClass = InterpTrackClasses(NewTrackClassIndex);
	check( NewInterpTrackClass->IsChildOf(UInterpTrack::StaticClass()) );

	AddTrackToSelectedGroup(NewInterpTrackClass, NULL);
}

void WxInterpEd::OnMenuPlay( wxCommandEvent& In )
{
	const UBOOL bShouldLoop = (In.GetId() == IDM_INTERP_PLAYLOOPSECTION);
	StartPlaying(bShouldLoop);
}

void WxInterpEd::OnMenuStop( wxCommandEvent& In )
{
	StopPlaying();
}

void WxInterpEd::OnOpenBindKeysDialog(wxCommandEvent &In)
{
	GApp->DlgBindHotkeys->Show(TRUE);
	GApp->DlgBindHotkeys->SetFocus();
}

void WxInterpEd::OnChangePlaySpeed( wxCommandEvent& In )
{
	PlaybackSpeed = 1.f;

	switch( In.GetId() )
	{
	case IDM_INTERP_SPEED_1:
		PlaybackSpeed = 0.01f;
		break;
	case IDM_INTERP_SPEED_10:
		PlaybackSpeed = 0.1f;
		break;
	case IDM_INTERP_SPEED_25:
		PlaybackSpeed = 0.25f;
		break;
	case IDM_INTERP_SPEED_50:
		PlaybackSpeed = 0.5f;
		break;
	case IDM_INTERP_SPEED_100:
		PlaybackSpeed = 1.0f;
		break;
	}
}

void WxInterpEd::OnMenuStretchSection(wxCommandEvent& In)
{
	// Edit section markers should always be within sequence...

	const FLOAT CurrentSectionLength = IData->EdSectionEnd - IData->EdSectionStart;
	if(CurrentSectionLength < 0.01f)
	{
		appMsgf( AMT_OK, *LocalizeUnrealEd("Error_HighlightNonZeroLength") );
		return;
	}


	const FString CurrentLengthStr = FString::Printf( TEXT("%3.3f"), CurrentSectionLength );

	// Display dialog and let user enter new length for this section.
	WxDlgGenericStringEntry dlg;
	const INT Result = dlg.ShowModal( TEXT("StretchSection"), TEXT("NewLength"), *CurrentLengthStr);
	if( Result != wxID_OK )
		return;

	double dNewSectionLength;
	const UBOOL bIsNumber = dlg.GetStringEntry().GetValue().ToDouble(&dNewSectionLength);
	if(!bIsNumber)
		return;

	const FLOAT NewSectionLength = (FLOAT)dNewSectionLength;
	if(NewSectionLength <= 0.f)
		return;

	InterpEdTrans->BeginSpecial( *LocalizeUnrealEd("StretchSection") );

	IData->Modify();
	Opt->Modify();

	const FLOAT LengthDiff = NewSectionLength - CurrentSectionLength;
	const FLOAT StretchRatio = NewSectionLength/CurrentSectionLength;

	// Iterate over all tracks.
	for(INT i=0; i<IData->InterpGroups.Num(); i++)
	{
		UInterpGroup* Group = IData->InterpGroups(i);
		for(INT j=0; j<Group->InterpTracks.Num(); j++)
		{
			UInterpTrack* Track = Group->InterpTracks(j);

			Track->Modify();

			for(INT k=0; k<Track->GetNumKeyframes(); k++)
			{
				const FLOAT KeyTime = Track->GetKeyframeTime(k);

				// Key is before start of stretched section
				if(KeyTime < IData->EdSectionStart)
				{
					// Leave key as it is
				}
				// Key is in section being stretched
				else if(KeyTime < IData->EdSectionEnd)
				{
					// Calculate new key time.
					const FLOAT FromSectionStart = KeyTime - IData->EdSectionStart;
					const FLOAT NewKeyTime = IData->EdSectionStart + (StretchRatio * FromSectionStart);

					Track->SetKeyframeTime(k, NewKeyTime, FALSE);
				}
				// Key is after stretched section
				else
				{
					// Move it on by the increase in sequence length.
					Track->SetKeyframeTime(k, KeyTime + LengthDiff, FALSE);
				}
			}
		}
	}

	// Move the end of the interpolation to account for changing the length of this section.
	SetInterpEnd(IData->InterpLength + LengthDiff);

	// Move end marker of section to new, stretched position.
	MoveLoopMarker( IData->EdSectionEnd + LengthDiff, FALSE );

	InterpEdTrans->EndSpecial();
}

/** Remove the currernt section, reducing the length of the sequence and moving any keys after the section earlier in time. */
void WxInterpEd::OnMenuDeleteSection(wxCommandEvent& In)
{
	const FLOAT CurrentSectionLength = IData->EdSectionEnd - IData->EdSectionStart;
	if(CurrentSectionLength < 0.01f)
		return;

	InterpEdTrans->BeginSpecial( *LocalizeUnrealEd("DeleteSection") );

	IData->Modify();
	Opt->Modify();

	// Add keys that are within current section to selection
	SelectKeysInLoopSection();

	// Delete current selection
	DeleteSelectedKeys(FALSE);

	// Then move any keys after the current section back by the length of the section.
	for(INT i=0; i<IData->InterpGroups.Num(); i++)
	{
		UInterpGroup* Group = IData->InterpGroups(i);
		for(INT j=0; j<Group->InterpTracks.Num(); j++)
		{
			UInterpTrack* Track = Group->InterpTracks(j);
			Track->Modify();

			for(INT k=0; k<Track->GetNumKeyframes(); k++)
			{
				// Move keys after section backwards by length of the section
				FLOAT KeyTime = Track->GetKeyframeTime(k);
				if(KeyTime > IData->EdSectionEnd)
				{
					// Add to selection for deletion.
					Track->SetKeyframeTime(k, KeyTime - CurrentSectionLength, FALSE);
				}
			}
		}
	}

	// Move the end of the interpolation to account for changing the length of this section.
	SetInterpEnd(IData->InterpLength - CurrentSectionLength);

	// Move section end marker on top of section start marker (section has vanished).
	MoveLoopMarker( IData->EdSectionStart, FALSE );

	InterpEdTrans->EndSpecial();
}

/** Insert an amount of space (specified by user in dialog) at the current position in the sequence. */
void WxInterpEd::OnMenuInsertSpace( wxCommandEvent& In )
{
	WxDlgGenericStringEntry dlg;
	INT Result = dlg.ShowModal( TEXT("InsertEmptySpace"), TEXT("Seconds"), TEXT("1.0"));
	if( Result != wxID_OK )
		return;

	double dAddTime;
	UBOOL bIsNumber = dlg.GetStringEntry().GetValue().ToDouble(&dAddTime);
	if(!bIsNumber)
		return;

	FLOAT AddTime = (FLOAT)dAddTime;

	// Ignore if adding a negative amount of time!
	if(AddTime <= 0.f)
		return;

	InterpEdTrans->BeginSpecial( *LocalizeUnrealEd("InsertSpace") );

	IData->Modify();
	Opt->Modify();

	// Move the end of the interpolation on by the amount we are adding.
	SetInterpEnd(IData->InterpLength + AddTime);

	// Iterate over all tracks.
	for(INT i=0; i<IData->InterpGroups.Num(); i++)
	{
		UInterpGroup* Group = IData->InterpGroups(i);
		for(INT j=0; j<Group->InterpTracks.Num(); j++)
		{
			UInterpTrack* Track = Group->InterpTracks(j);

			Track->Modify();

			for(INT k=0; k<Track->GetNumKeyframes(); k++)
			{
				FLOAT KeyTime = Track->GetKeyframeTime(k);
				if(KeyTime > Interp->Position)
				{
					Track->SetKeyframeTime(k, KeyTime + AddTime, FALSE);
				}
			}
		}
	}

	InterpEdTrans->EndSpecial();
}

void WxInterpEd::OnMenuSelectInSection(wxCommandEvent& In)
{
	SelectKeysInLoopSection();
}

void WxInterpEd::OnMenuDuplicateSelectedKeys(wxCommandEvent& In)
{
	DuplicateSelectedKeys();
}

void WxInterpEd::OnSavePathTime( wxCommandEvent& In )
{
	IData->PathBuildTime = Interp->Position;
}

void WxInterpEd::OnJumpToPathTime( wxCommandEvent& In )
{
	SetInterpPosition(IData->PathBuildTime);
}

void WxInterpEd::OnViewHide3DTracks( wxCommandEvent& In )
{
	bHide3DTrackView = !bHide3DTrackView;
	MenuBar->ViewMenu->Check( IDM_INTERP_VIEW_HIDE3DTRACKS, bHide3DTrackView == TRUE );

	// Save to ini when it changes.
	GConfig->SetBool( TEXT("Matinee"), TEXT("Hide3DTracks"), bHide3DTrackView, GEditorIni );
}

void WxInterpEd::OnViewZoomToScrubPos( wxCommandEvent& In )
{
	bZoomToScrubPos = !bZoomToScrubPos;
	MenuBar->ViewMenu->Check( IDM_INTERP_VIEW_ZOOMTOSCUBPOS, bZoomToScrubPos == TRUE );

	// Save to ini when it changes.
	GConfig->SetBool( TEXT("Matinee"), TEXT("ZoomToScrubPos"), bZoomToScrubPos, GEditorIni );
}

void WxInterpEd::OnContextTrackRename( wxCommandEvent& In )
{
	if(ActiveTrackIndex == INDEX_NONE)
		return;

	check(ActiveGroup);

	UInterpTrack* Track = ActiveGroup->InterpTracks(ActiveTrackIndex);
	check(Track);

	WxDlgGenericStringEntry dlg;
	INT Result = dlg.ShowModal( TEXT("RenameTrack"), TEXT("NewTrackName"), *Track->TrackTitle );
	if( Result != wxID_OK )
		return;

	Track->TrackTitle = dlg.GetEnteredString();

	// In case this track is being displayed on the curve editor, update its name there too.
	FString CurveName = FString::Printf( TEXT("%s_%s"), *ActiveGroup->GroupName.ToString(), *Track->TrackTitle);
	IData->CurveEdSetup->ChangeCurveName(Track, CurveName);
	CurveEd->CurveChanged();
}

void WxInterpEd::OnContextTrackDelete( wxCommandEvent& In )
{
	DeleteSelectedTrack();
}

void WxInterpEd::OnContextTrackChangeFrame( wxCommandEvent& In  )
{
	check(ActiveGroup);

	UInterpTrack* Track = ActiveGroup->InterpTracks(ActiveTrackIndex);
	UInterpTrackMove* MoveTrack = Cast<UInterpTrackMove>(Track);
	if(!MoveTrack)
		return;

	// Find the frame we want to convert to.
	INT Id = In.GetId();
	BYTE DesiredFrame = 0;
	if(Id == IDM_INTERP_TRACK_FRAME_WORLD)
	{
		DesiredFrame = IMF_World;
	}
	else if(Id == IDM_INTERP_TRACK_FRAME_RELINITIAL)
	{
		DesiredFrame = IMF_RelativeToInitial;
	}
	else
	{
		check(0);
	}

	// Do nothing if already in desired frame
	if(DesiredFrame == MoveTrack->MoveFrame)
	{
		return;
	}
	
	// Find the first instance of this group. This is the one we are going to use to store the curve relative to.
	UInterpGroupInst* GrInst = Interp->FindFirstGroupInst(ActiveGroup);
	check(GrInst);

	AActor* Actor = GrInst->GroupActor;
	if(!Actor)
	{
		appMsgf(AMT_OK, *LocalizeUnrealEd("Error_NoActorForThisGroup"));
		return;
	}

	// Get instance of movement track, for the initial TM.
	UInterpTrackInstMove* MoveTrackInst = CastChecked<UInterpTrackInstMove>( GrInst->TrackInst(ActiveTrackIndex) );

	// Find the frame to convert key-frame from.
	FMatrix FromFrameTM = MoveTrack->GetMoveRefFrame(MoveTrackInst);

	// Find the frame to convert the key-frame into.
	AActor* BaseActor = Actor->GetBase();

	FMatrix BaseTM = FMatrix::Identity;
	if(BaseActor)
	{
		BaseTM = FRotationTranslationMatrix( BaseActor->Rotation, BaseActor->Location );
	}

	FMatrix ToFrameTM = FMatrix::Identity;
	if( DesiredFrame == IMF_World )
	{
		if(BaseActor)
		{
			ToFrameTM = BaseTM;
		}
		else
		{
			ToFrameTM = FMatrix::Identity;
		}
	}
	else if( DesiredFrame == IMF_RelativeToInitial )
	{
		if(BaseActor)
		{
			ToFrameTM = MoveTrackInst->InitialTM * BaseTM;
		}
		else
		{
			ToFrameTM = MoveTrackInst->InitialTM;
		}
	}
	FMatrix InvToFrameTM = ToFrameTM.Inverse();


	// Iterate over each keyframe. Convert key into world reference frame, then into new desired reference frame.
	check( MoveTrack->PosTrack.Points.Num() == MoveTrack->EulerTrack.Points.Num() );
	for(INT i=0; i<MoveTrack->PosTrack.Points.Num(); i++)
	{
		FQuat KeyQuat = FQuat::MakeFromEuler( MoveTrack->EulerTrack.Points(i).OutVal );
		FQuatRotationTranslationMatrix KeyTM( KeyQuat, MoveTrack->PosTrack.Points(i).OutVal );

		FMatrix WorldKeyTM = KeyTM * FromFrameTM;

		FVector WorldArriveTan = FromFrameTM.TransformNormal( MoveTrack->PosTrack.Points(i).ArriveTangent );
		FVector WorldLeaveTan = FromFrameTM.TransformNormal( MoveTrack->PosTrack.Points(i).LeaveTangent );

		FMatrix RelKeyTM = WorldKeyTM * InvToFrameTM;

		MoveTrack->PosTrack.Points(i).OutVal = RelKeyTM.GetOrigin();
		MoveTrack->PosTrack.Points(i).ArriveTangent = ToFrameTM.InverseTransformNormal( WorldArriveTan );
		MoveTrack->PosTrack.Points(i).LeaveTangent = ToFrameTM.InverseTransformNormal( WorldLeaveTan );

		MoveTrack->EulerTrack.Points(i).OutVal = FQuat(RelKeyTM).Euler();
	}

	MoveTrack->MoveFrame = DesiredFrame;

	//PropertyWindow->Refresh(); // Don't know why this doesn't work...

	PropertyWindow->SetObject(NULL, 1, 1, 0);
	PropertyWindow->SetObject(Track, 1, 1, 0);
}

void WxInterpEd::OnContextGroupRename( wxCommandEvent& In )
{
	if(!ActiveGroup)
	{
		return;
	}

	WxDlgGenericStringEntry dlg;
	FName NewName = ActiveGroup->GroupName;
	UBOOL bValidName = FALSE;

	while(!bValidName)
	{
		INT Result = dlg.ShowModal( TEXT("RenameGroup"), TEXT("NewGroupName"), *NewName.ToString() );
		if( Result != wxID_OK )
		{
			return;
		}

		NewName = FName(*dlg.GetEnteredString());
		bValidName = TRUE;

		// Check this name does not already exist.
		for(INT i=0; i<IData->InterpGroups.Num() && bValidName; i++)
		{
			if(IData->InterpGroups(i)->GroupName == NewName)
			{
				bValidName = FALSE;
			}
		}

		if(!bValidName)
		{
			appMsgf( AMT_OK, *LocalizeUnrealEd("Error_NameAlreadyExists") );
		}
	}
	
	// We also need to change the name of the variable connector on all SeqAct_Interps in this level using this InterpData
	USequence* RootSeq = Interp->GetRootSequence();
	check(RootSeq);

	TArray<USequenceObject*> MatineeActions;
	RootSeq->FindSeqObjectsByClass( USeqAct_Interp::StaticClass(), MatineeActions );

	for(INT i=0; i<MatineeActions.Num(); i++)
	{
		USeqAct_Interp* TestAction = CastChecked<USeqAct_Interp>( MatineeActions(i) );
		check(TestAction);
	
		UInterpData* TestData = TestAction->FindInterpDataFromVariable();
		if(TestData && TestData == IData)
		{
			INT VarIndex = TestAction->FindConnectorIndex( ActiveGroup->GroupName.ToString(), LOC_VARIABLE );
			if(VarIndex != INDEX_NONE && VarIndex >= 1) // Ensure variable index is not the reserved first one.
			{
				TestAction->VariableLinks(VarIndex).LinkDesc = NewName.ToString();
			}
		}
	}

	// Update any camera cuts to point to new group name
	UInterpGroupDirector* DirGroup = IData->FindDirectorGroup();
	if(DirGroup)
	{
		UInterpTrackDirector* DirTrack = DirGroup->GetDirectorTrack();
		if(DirTrack)
		{
			for(INT i=0; i<DirTrack->CutTrack.Num(); i++)
			{
				FDirectorTrackCut& Cut = DirTrack->CutTrack(i);
				if(Cut.TargetCamGroup == ActiveGroup->GroupName)
				{
					Cut.TargetCamGroup = NewName;
				}	
			}
		}
	}

	// Change the name of the InterpGroup.
	ActiveGroup->GroupName = NewName;
}

void WxInterpEd::OnContextGroupDelete( wxCommandEvent& In )
{
	if(!ActiveGroup)
	{
		return;
	}

	// Check we REALLY want to do this.
	UBOOL bDoDestroy = appMsgf(AMT_YesNo, *LocalizeUnrealEd("Prompt_11"), *ActiveGroup->GroupName.ToString());
	if(!bDoDestroy)
	{
		return;
	}

	DeleteSelectedGroup();
}

/** Prompts the user for a name for a new filter and creates a custom filter. */
void WxInterpEd::OnContextGroupCreateTab( wxCommandEvent& In )
{
	// Display dialog and let user enter new time.
	FString TabName;

	WxDlgGenericStringEntry Dlg;
	const INT Result = Dlg.ShowModal( TEXT("CreateGroupTab_Title"), TEXT("CreateGroupTab_Caption"), TEXT(""));
	if( Result == wxID_OK )
	{
		// Create a new tab.
		if(ActiveGroup != NULL)
		{
			UInterpFilter_Custom* Filter = ConstructObject<UInterpFilter_Custom>(UInterpFilter_Custom::StaticClass(), IData, NAME_None, RF_Transactional);

			if(Dlg.GetEnteredString().Len())
			{
				Filter->Caption = Dlg.GetEnteredString();
			}
			else
			{
				Filter->Caption = Filter->GetName();
			}
	
			Filter->GroupsToInclude.AddItem(ActiveGroup);
			IData->InterpFilters.AddItem(Filter);
		}
	}
}

/** Sends the selected group to the tab the user specified.  */
void WxInterpEd::OnContextGroupSendToTab( wxCommandEvent& In )
{
	INT TabIndex = (In.GetId() - IDM_INTERP_GROUP_SENDTOTAB_START);

	if(TabIndex >=0 && TabIndex < IData->InterpFilters.Num() && ActiveGroup != NULL)
	{
		// Make sure the active group isnt already in the filter's set of groups.
		UInterpFilter_Custom* Filter = Cast<UInterpFilter_Custom>(IData->InterpFilters(TabIndex));

		if(Filter != NULL && Filter->GroupsToInclude.ContainsItem(ActiveGroup) == FALSE)
		{
			Filter->GroupsToInclude.AddItem(ActiveGroup);
		}
	}
}

/** Removes the group from the current tab.  */
void WxInterpEd::OnContextGroupRemoveFromTab( wxCommandEvent& In )
{
	// Make sure the active group exists in the selected filter and that the selected filter isn't a default filter.
	UInterpFilter_Custom* Filter = Cast<UInterpFilter_Custom>(IData->SelectedFilter);

	if(Filter != NULL && Filter->GroupsToInclude.ContainsItem(ActiveGroup) == TRUE && IData->InterpFilters.ContainsItem(Filter) == TRUE)
	{
		Filter->GroupsToInclude.RemoveItem(ActiveGroup);
		ActiveGroup->bVisible = FALSE;
		InterpEdVC->Viewport->Invalidate();
	}
}

/** Deletes the currently selected group tab.  */
void WxInterpEd::OnContextDeleteGroupTab( wxCommandEvent& In )
{
	// Make sure the active group exists in the selected filter and that the selected filter isn't a default filter.
	UInterpFilter_Custom* Filter = Cast<UInterpFilter_Custom>(IData->SelectedFilter);

	if(Filter != NULL)
	{
		IData->InterpFilters.RemoveItem(Filter);

		// Set the selected filter back to the all filter.
		if(IData->DefaultFilters.Num())
		{
			SetSelectedFilter(IData->DefaultFilters(0));
		}
		else
		{
			SetSelectedFilter(NULL);
		}
	}
}


// Iterate over keys changing their interpolation mode and adjusting tangents appropriately.
void WxInterpEd::OnContextKeyInterpMode( wxCommandEvent& In )
{

	for(INT i=0; i<Opt->SelectedKeys.Num(); i++)
	{
		FInterpEdSelKey& SelKey = Opt->SelectedKeys(i);
		UInterpTrack* Track = SelKey.Group->InterpTracks(SelKey.TrackIndex);

		UInterpTrackMove* MoveTrack = Cast<UInterpTrackMove>(Track);
		if(MoveTrack)
		{
			if(In.GetId() == IDM_INTERP_KEYMODE_LINEAR)
			{
				MoveTrack->PosTrack.Points(SelKey.KeyIndex).InterpMode = CIM_Linear;
				MoveTrack->EulerTrack.Points(SelKey.KeyIndex).InterpMode = CIM_Linear;
			}
			else if(In.GetId() == IDM_INTERP_KEYMODE_CURVE)
			{
				MoveTrack->PosTrack.Points(SelKey.KeyIndex).InterpMode = CIM_CurveAuto;
				MoveTrack->EulerTrack.Points(SelKey.KeyIndex).InterpMode = CIM_CurveAuto;
			}
			else if(In.GetId() == IDM_INTERP_KEYMODE_CURVEBREAK)
			{
				MoveTrack->PosTrack.Points(SelKey.KeyIndex).InterpMode = CIM_CurveBreak;
				MoveTrack->EulerTrack.Points(SelKey.KeyIndex).InterpMode = CIM_CurveBreak;
			}
			else if(In.GetId() == IDM_INTERP_KEYMODE_CONSTANT)
			{
				MoveTrack->PosTrack.Points(SelKey.KeyIndex).InterpMode = CIM_Constant;
				MoveTrack->EulerTrack.Points(SelKey.KeyIndex).InterpMode = CIM_Constant;
			}

			MoveTrack->PosTrack.AutoSetTangents(MoveTrack->LinCurveTension);
			MoveTrack->EulerTrack.AutoSetTangents(MoveTrack->AngCurveTension);
		}

		UInterpTrackFloatBase* FloatTrack = Cast<UInterpTrackFloatBase>(Track);
		if(FloatTrack)
		{
			if(In.GetId() == IDM_INTERP_KEYMODE_LINEAR)
				FloatTrack->FloatTrack.Points(SelKey.KeyIndex).InterpMode = CIM_Linear;
			else if(In.GetId() == IDM_INTERP_KEYMODE_CURVE)
				FloatTrack->FloatTrack.Points(SelKey.KeyIndex).InterpMode = CIM_CurveAuto;
			else if(In.GetId() == IDM_INTERP_KEYMODE_CURVEBREAK)
				FloatTrack->FloatTrack.Points(SelKey.KeyIndex).InterpMode = CIM_CurveBreak;
			else if(In.GetId() == IDM_INTERP_KEYMODE_CONSTANT)
				FloatTrack->FloatTrack.Points(SelKey.KeyIndex).InterpMode = CIM_Constant;

			FloatTrack->FloatTrack.AutoSetTangents(FloatTrack->CurveTension);
		}

		UInterpTrackVectorBase* VectorTrack = Cast<UInterpTrackVectorBase>(Track);
		if(VectorTrack)
		{
			if(In.GetId() == IDM_INTERP_KEYMODE_LINEAR)
				VectorTrack->VectorTrack.Points(SelKey.KeyIndex).InterpMode = CIM_Linear;
			else if(In.GetId() == IDM_INTERP_KEYMODE_CURVE)
				VectorTrack->VectorTrack.Points(SelKey.KeyIndex).InterpMode = CIM_CurveAuto;
			else if(In.GetId() == IDM_INTERP_KEYMODE_CURVEBREAK)
				VectorTrack->VectorTrack.Points(SelKey.KeyIndex).InterpMode = CIM_CurveBreak;
			else if(In.GetId() == IDM_INTERP_KEYMODE_CONSTANT)
				VectorTrack->VectorTrack.Points(SelKey.KeyIndex).InterpMode = CIM_Constant;

			VectorTrack->VectorTrack.AutoSetTangents(VectorTrack->CurveTension);
		}
	}

	CurveEd->UpdateDisplay();
}

/** Pops up menu and lets you set the time for the selected key. */
void WxInterpEd::OnContextSetKeyTime( wxCommandEvent& In )
{
	// Only works if one key is selected.
	if(Opt->SelectedKeys.Num() != 1)
	{
		return;
	}

	// Get the time the selected key is currently at.
	FInterpEdSelKey& SelKey = Opt->SelectedKeys(0);
	UInterpTrack* Track = SelKey.Group->InterpTracks(SelKey.TrackIndex);

	FLOAT CurrentKeyTime = Track->GetKeyframeTime(SelKey.KeyIndex);
	const FString CurrentTimeStr = FString::Printf( TEXT("%3.3f"), CurrentKeyTime );

	// Display dialog and let user enter new time.
	WxDlgGenericStringEntry dlg;
	const INT Result = dlg.ShowModal( TEXT("NewKeyTime"), TEXT("NewTime"), *CurrentTimeStr);
	if( Result != wxID_OK )
		return;

	double dNewTime;
	const UBOOL bIsNumber = dlg.GetStringEntry().GetValue().ToDouble(&dNewTime);
	if(!bIsNumber)
		return;

	const FLOAT NewKeyTime = (FLOAT)dNewTime;

	// Move the key. Also update selected to reflect new key index.
	SelKey.KeyIndex = Track->SetKeyframeTime( SelKey.KeyIndex, NewKeyTime );

	// Update positions at current time but with new keyframe times.
	SetInterpPosition(Interp->Position);
	CurveEd->UpdateDisplay();
}

/** Pops up a menu and lets you set the value for the selected key. Not all track types are supported. */
void WxInterpEd::OnContextSetValue( wxCommandEvent& In )
{
	// Only works if one key is selected.
	if(Opt->SelectedKeys.Num() != 1)
	{
		return;
	}

	// Get the time the selected key is currently at.
	FInterpEdSelKey& SelKey = Opt->SelectedKeys(0);
	UInterpTrack* Track = SelKey.Group->InterpTracks(SelKey.TrackIndex);

	// If its a float track - pop up text entry dialog.
	UInterpTrackFloatBase* FloatTrack = Cast<UInterpTrackFloatBase>(Track);
	if(FloatTrack)
	{
		// Get current float value of the key
		FLOAT CurrentKeyVal = FloatTrack->FloatTrack.Points(SelKey.KeyIndex).OutVal;
		const FString CurrentValStr = FString::Printf( TEXT("%3.3f"), CurrentKeyVal );

		// Display dialog and let user enter new value.
		WxDlgGenericStringEntry dlg;
		const INT Result = dlg.ShowModal( TEXT("NewKeyValue"), TEXT("NewValue"), *CurrentValStr);
		if( Result != wxID_OK )
			return;

		double dNewVal;
		const UBOOL bIsNumber = dlg.GetStringEntry().GetValue().ToDouble(&dNewVal);
		if(!bIsNumber)
			return;

		// Set new value, and update tangents.
		const FLOAT NewVal = (FLOAT)dNewVal;
		FloatTrack->FloatTrack.Points(SelKey.KeyIndex).OutVal = NewVal;
		FloatTrack->FloatTrack.AutoSetTangents(FloatTrack->CurveTension);
	}

	// Update positions at current time but with new keyframe times.
	SetInterpPosition(Interp->Position);
	CurveEd->UpdateDisplay();
}


/** Pops up a menu and lets you set the color for the selected key. Not all track types are supported. */
void WxInterpEd::OnContextSetColor( wxCommandEvent& In )
{
	// Only works if one key is selected.
	if(Opt->SelectedKeys.Num() != 1)
	{
		return;
	}

	// Get the time the selected key is currently at.
	FInterpEdSelKey& SelKey = Opt->SelectedKeys(0);
	UInterpTrack* Track = SelKey.Group->InterpTracks(SelKey.TrackIndex);

	// If its a color prop track - pop up color dialog.
	UInterpTrackColorProp* ColorPropTrack = Cast<UInterpTrackColorProp>(Track);
	if(ColorPropTrack)
	{
		// Get the current color and show a color picker dialog.
		FVector CurrentColorVector = ColorPropTrack->VectorTrack.Points(SelKey.KeyIndex).OutVal;
		FColor CurrentColor(FLinearColor(CurrentColorVector.X, CurrentColorVector.Y, CurrentColorVector.Z));
		
		wxColour FinalColor = ::wxGetColourFromUser(this, wxColour(CurrentColor.R, CurrentColor.G, CurrentColor.B));
		
		if(FinalColor.Ok())
		{
			// The user chose a color so set the keyframe color to the color they picked.
			FLinearColor VectorColor(FColor(FinalColor.Red(), FinalColor.Green(), FinalColor.Blue()));

			ColorPropTrack->VectorTrack.Points(SelKey.KeyIndex).OutVal = FVector(VectorColor.R, VectorColor.G, VectorColor.B);
			ColorPropTrack->VectorTrack.AutoSetTangents(ColorPropTrack->CurveTension);
		}
	}

	// Update positions at current time but with new keyframe times.
	SetInterpPosition(Interp->Position);
	CurveEd->UpdateDisplay();
}


/** Pops up menu and lets the user set a group to use to lookup transform info for a movement keyframe. */
void WxInterpEd::OnSetMoveKeyLookupGroup( wxCommandEvent& In )
{
	// Only works if one key is selected.
	if(Opt->SelectedKeys.Num() != 1)
	{
		return;
	}

	// Get the time the selected key is currently at.
	FInterpEdSelKey& SelKey = Opt->SelectedKeys(0);
	UInterpTrack* Track = SelKey.Group->InterpTracks(SelKey.TrackIndex);

	// Only perform work if we are on a movement track.
	UInterpTrackMove* MoveTrack = Cast<UInterpTrackMove>(Track);
	if(MoveTrack)
	{
		// Make array of group names
		TArray<FString> GroupNames;
		for ( INT GroupIdx = 0; GroupIdx < IData->InterpGroups.Num(); GroupIdx++ )
		{
			if(IData->InterpGroups(GroupIdx) != SelKey.Group)
			{
				GroupNames.AddItem( *(IData->InterpGroups(GroupIdx)->GroupName.ToString()) );
			}
		}

		WxDlgGenericComboEntry	dlg;
		const INT	Result = dlg.ShowModal( TEXT("SelectGroup"), TEXT("SelectGroupToLookupDataFrom"), GroupNames, 0, TRUE );
		if ( Result == wxID_OK )
		{
			FName KeyframeLookupGroup = FName( *dlg.GetSelectedString() );
			MoveTrack->SetLookupKeyGroupName(SelKey.KeyIndex, KeyframeLookupGroup);
		}

		
	}
}

/** Clears the lookup group for a currently selected movement key. */
void WxInterpEd::OnClearMoveKeyLookupGroup( wxCommandEvent& In )
{
	// Only works if one key is selected.
	if(Opt->SelectedKeys.Num() != 1)
	{
		return;
	}

	// Get the time the selected key is currently at.
	FInterpEdSelKey& SelKey = Opt->SelectedKeys(0);
	UInterpTrack* Track = SelKey.Group->InterpTracks(SelKey.TrackIndex);

	// Only perform work if we are on a movement track.
	UInterpTrackMove* MoveTrack = Cast<UInterpTrackMove>(Track);
	if(MoveTrack)
	{
		MoveTrack->ClearLookupKeyGroupName(SelKey.KeyIndex);
	}
}


/** Rename an event. Handle removing/adding connectors as appropriate. */
void WxInterpEd::OnContextRenameEventKey(wxCommandEvent& In)
{
	// Only works if one Event key is selected.
	if(Opt->SelectedKeys.Num() != 1)
	{
		return;
	}

	// Find the EventNames of selected key
	FName EventNameToChange;
	FInterpEdSelKey& SelKey = Opt->SelectedKeys(0);
	UInterpTrack* Track = SelKey.Group->InterpTracks(SelKey.TrackIndex);
	UInterpTrackEvent* EventTrack = Cast<UInterpTrackEvent>(Track);
	if(EventTrack)
	{
		EventNameToChange = EventTrack->EventTrack(SelKey.KeyIndex).EventName; 
	}
	else
	{
		return;
	}

	// Pop up dialog to ask for new name.
	WxDlgGenericStringEntry dlg;
	INT Result = dlg.ShowModal( TEXT("EnterNewEventName"), TEXT("NewEventName"), *EventNameToChange.ToString() );
	if( Result != wxID_OK )
		return;		

	FString TempString = dlg.GetEnteredString();
	TempString = TempString.Replace(TEXT(" "),TEXT("_"));
	FName NewEventName = FName( *TempString );

	// If this Event name is already in use- disallow it
	TArray<FName> CurrentEventNames;
	IData->GetAllEventNames(CurrentEventNames);
	if( CurrentEventNames.ContainsItem(NewEventName) )
	{
		appMsgf( AMT_OK, *LocalizeUnrealEd("Error_EventNameInUse") );
		return;
	}
	
	// Then go through all keys, changing those with this name to the new one.
	for(INT i=0; i<IData->InterpGroups.Num(); i++)
	{
		UInterpGroup* Group = IData->InterpGroups(i);
		for(INT j=0; j<Group->InterpTracks.Num(); j++)
		{
			UInterpTrackEvent* EventTrack = Cast<UInterpTrackEvent>( Group->InterpTracks(j) );
			if(EventTrack)
			{
				for(INT k=0; k<EventTrack->EventTrack.Num(); k++)
				{
					if(EventTrack->EventTrack(k).EventName == EventNameToChange)
					{
						EventTrack->EventTrack(k).EventName = NewEventName;
					}	
				}
			}			
		}
	}

	// We also need to change the name of the output connector on all SeqAct_Interps using this InterpData
	USequence* RootSeq = Interp->GetRootSequence();
	check(RootSeq);

	TArray<USequenceObject*> MatineeActions;
	RootSeq->FindSeqObjectsByClass( USeqAct_Interp::StaticClass(), MatineeActions );

	for(INT i=0; i<MatineeActions.Num(); i++)
	{
		USeqAct_Interp* TestAction = CastChecked<USeqAct_Interp>( MatineeActions(i) );
		check(TestAction);
	
		UInterpData* TestData = TestAction->FindInterpDataFromVariable();
		if(TestData && TestData == IData)
		{
			INT OutputIndex = TestAction->FindConnectorIndex( EventNameToChange.ToString(), LOC_OUTPUT );
			if(OutputIndex != INDEX_NONE && OutputIndex >= 2) // Ensure Output index is not one of the reserved first 2.
			{
				TestAction->OutputLinks(OutputIndex).LinkDesc = NewEventName.ToString();
			}
		}
	}
}

void WxInterpEd::OnSetAnimKeyLooping( wxCommandEvent& In )
{
	UBOOL bNewLooping = (In.GetId() == IDM_INTERP_ANIMKEY_LOOP);

	for(INT i=0; i<Opt->SelectedKeys.Num(); i++)
	{
		FInterpEdSelKey& SelKey = Opt->SelectedKeys(i);
		UInterpTrack* Track = SelKey.Group->InterpTracks(SelKey.TrackIndex);
		UInterpTrackAnimControl* AnimTrack = Cast<UInterpTrackAnimControl>(Track);
		if(AnimTrack)
		{
			AnimTrack->AnimSeqs(SelKey.KeyIndex).bLooping = bNewLooping;
		}
	}
}

void WxInterpEd::OnSetAnimOffset( wxCommandEvent& In )
{
	UBOOL bEndOffset = (In.GetId() == IDM_INTERP_ANIMKEY_SETENDOFFSET);

	if(Opt->SelectedKeys.Num() != 1)
	{
		return;
	}

	FInterpEdSelKey& SelKey = Opt->SelectedKeys(0);
	UInterpTrack* Track = SelKey.Group->InterpTracks(SelKey.TrackIndex);
	UInterpTrackAnimControl* AnimTrack = Cast<UInterpTrackAnimControl>(Track);
	if(!AnimTrack)
	{
		return;
	}

	FLOAT CurrentOffset = 0.f;
	if(bEndOffset)
	{
		CurrentOffset = AnimTrack->AnimSeqs(SelKey.KeyIndex).AnimEndOffset;
	}
	else
	{
		CurrentOffset = AnimTrack->AnimSeqs(SelKey.KeyIndex).AnimStartOffset;
	}

	const FString CurrentOffsetStr = FString::Printf( TEXT("%3.3f"), CurrentOffset );

	// Display dialog and let user enter new offste.
	WxDlgGenericStringEntry dlg;
	const INT Result = dlg.ShowModal( TEXT("NewAnimOffset"), TEXT("NewOffset"), *CurrentOffsetStr);
	if( Result != wxID_OK )
		return;

	double dNewOffset;
	const UBOOL bIsNumber = dlg.GetStringEntry().GetValue().ToDouble(&dNewOffset);
	if(!bIsNumber)
		return;

	const FLOAT NewOffset = ::Max( (FLOAT)dNewOffset, 0.f );

	if(bEndOffset)
	{
		AnimTrack->AnimSeqs(SelKey.KeyIndex).AnimEndOffset = NewOffset;
	}
	else
	{
		AnimTrack->AnimSeqs(SelKey.KeyIndex).AnimStartOffset = NewOffset;
	}


	// Update stuff in case doing this has changed it.
	SetInterpPosition(Interp->Position);
}

void WxInterpEd::OnSetAnimPlayRate( wxCommandEvent& In )
{
	if(Opt->SelectedKeys.Num() != 1)
	{
		return;
	}

	FInterpEdSelKey& SelKey = Opt->SelectedKeys(0);
	UInterpTrack* Track = SelKey.Group->InterpTracks(SelKey.TrackIndex);
	UInterpTrackAnimControl* AnimTrack = Cast<UInterpTrackAnimControl>(Track);
	if(!AnimTrack)
	{
		return;
	}

	FLOAT CurrentRate = AnimTrack->AnimSeqs(SelKey.KeyIndex).AnimPlayRate;
	const FString CurrentRateStr = FString::Printf( TEXT("%3.3f"), CurrentRate );

	// Display dialog and let user enter new rate.
	WxDlgGenericStringEntry dlg;
	const INT Result = dlg.ShowModal( TEXT("NewAnimRate"), TEXT("PlayRate"), *CurrentRateStr);
	if( Result != wxID_OK )
		return;

	double dNewRate;
	const UBOOL bIsNumber = dlg.GetStringEntry().GetValue().ToDouble(&dNewRate);
	if(!bIsNumber)
		return;

	const FLOAT NewRate = ::Clamp( (FLOAT)dNewRate, 0.01f, 100.f );

	AnimTrack->AnimSeqs(SelKey.KeyIndex).AnimPlayRate = NewRate;

	// Update stuff in case doing this has changed it.
	SetInterpPosition(Interp->Position);
}

/** Handler for the toggle animation reverse menu item. */
void WxInterpEd::OnToggleReverseAnim( wxCommandEvent& In )
{
	if(Opt->SelectedKeys.Num() != 1)
	{
		return;
	}

	FInterpEdSelKey& SelKey = Opt->SelectedKeys(0);
	UInterpTrack* Track = SelKey.Group->InterpTracks(SelKey.TrackIndex);
	UInterpTrackAnimControl* AnimTrack = Cast<UInterpTrackAnimControl>(Track);
	if(!AnimTrack)
	{
		return;
	}

	AnimTrack->AnimSeqs(SelKey.KeyIndex).bReverse = !AnimTrack->AnimSeqs(SelKey.KeyIndex).bReverse;
}

/** Handler for UI update requests for the toggle anim reverse menu item. */
void WxInterpEd::OnToggleReverseAnim_UpdateUI( wxUpdateUIEvent& In )
{
	if(Opt->SelectedKeys.Num() != 1)
	{
		return;
	}

	FInterpEdSelKey& SelKey = Opt->SelectedKeys(0);
	UInterpTrack* Track = SelKey.Group->InterpTracks(SelKey.TrackIndex);
	UInterpTrackAnimControl* AnimTrack = Cast<UInterpTrackAnimControl>(Track);
	if(!AnimTrack)
	{
		return;
	}

	In.Check(AnimTrack->AnimSeqs(SelKey.KeyIndex).bReverse==TRUE);
}

/** Handler for the save as camera animation menu item. */
void WxInterpEd::OnContextSaveAsCameraAnimation( wxCommandEvent& In )
{
	if (ActiveGroup == NULL)
	{
		return;
	}

	WxDlgPackageGroupName dlg;
	dlg.SetTitle( *LocalizeUnrealEd("ExportCameraAnim") );
	FString Reason;

	// pre-populate with whatever is selected in Generic Browser
	WxGenericBrowser* GBrowser = GUnrealEd->GetBrowser<WxGenericBrowser>( TEXT("GenericBrowser") );
	FString PackageName, GroupName, ObjName;
	{
		UPackage* SelectedPkg = GBrowser->GetTopPackage();
		PackageName = SelectedPkg ? SelectedPkg->GetName() : TEXT("");

		UPackage* SelectedGrp = GBrowser->GetGroup();
		GroupName = SelectedGrp ? SelectedGrp->GetName() : TEXT("");

		UObject* const SelectedCamAnim = GEditor->GetSelectedObjects()->GetTop<UCameraAnim>();
		ObjName = SelectedCamAnim ? *SelectedCamAnim->GetName() : TEXT("");
	}

	if( dlg.ShowModal( PackageName, GroupName, ObjName ) == wxID_OK )
	{
		FString Pkg;
		if( dlg.GetGroup().Len() > 0 )
		{
			Pkg = FString::Printf(TEXT("%s.%s"), *dlg.GetPackage(), *dlg.GetGroup());
		}
		else
		{
			Pkg = FString::Printf(TEXT("%s"), *dlg.GetPackage());
		}
		UObject* ExistingPackage = UObject::FindPackage(NULL, *Pkg);

		// Make sure packages objects are duplicated into are fully loaded.
		TArray<UPackage*> TopLevelPackages;
		if( ExistingPackage )
		{
			TopLevelPackages.AddItem( ExistingPackage->GetOutermost() );
		}

		if(!dlg.GetPackage().Len() || !dlg.GetObjectName().Len())
		{
			appMsgf(AMT_OK,*LocalizeUnrealEd("Error_InvalidInput"));
		}
		else if( !FIsValidObjectName( *dlg.GetObjectName(), Reason ))
		{
			appMsgf( AMT_OK, *Reason );
		}
		//else 
		else
		{
			UBOOL bNewObject = FALSE, bSavedSuccessfully = FALSE;

			UObject* ExistingObject = GEditor->StaticFindObject(UCameraAnim::StaticClass(), ExistingPackage, *dlg.GetObjectName(), TRUE);

			if (ExistingObject == NULL)
			{
				// attempting to create a new object, need to handle fully loading
				if( GBrowser->HandleFullyLoadingPackages( TopLevelPackages, TEXT("ExportCameraAnim") ) )
				{
					// make sure name of new object is unique
					if (ExistingPackage && !FIsUniqueObjectName(*dlg.GetObjectName(), ExistingPackage, Reason))
					{
						appMsgf(AMT_OK, *Reason);
					}
					else
					{
						// create it, then copy params into it
						ExistingObject = GEditor->StaticConstructObject(UCameraAnim::StaticClass(), ExistingPackage, *dlg.GetObjectName(), RF_Public|RF_Standalone);
						bNewObject = TRUE;
						GBrowser->Update();
					}
				}
			}

			if (ExistingObject)
			{
				// copy params into it
				UCameraAnim* CamAnim = Cast<UCameraAnim>(ExistingObject);
				if (CamAnim->CreateFromInterpGroup(ActiveGroup, Interp))
				{
					bSavedSuccessfully = TRUE;
					CamAnim->MarkPackageDirty();

					// set generic browser selection to the newly-saved object
					GBrowser->GetSelection()->Select(ExistingObject, TRUE);
				}
			}

			if (bNewObject)
			{
				if (bSavedSuccessfully)
				{
					GBrowser->Update();
				}
				else
				{
					// delete the new object
					ExistingObject->MarkPendingKill();
				}
			}
		}
	}
}

/**
 * Prompts the user to edit volumes for the selected sound keys.
 */
void WxInterpEd::OnSetSoundVolume(wxCommandEvent& In)
{
	TArray<INT> SoundTrackKeyIndices;
	UBOOL bFoundVolume = FALSE;
	UBOOL bKeysDiffer = FALSE;
	FLOAT Volume = 1.0f;

	// Make a list of all keys and what their volumes are.
	for( INT i = 0 ; i < Opt->SelectedKeys.Num() ; ++i )
	{
		const FInterpEdSelKey& SelKey		= Opt->SelectedKeys(i);
		UInterpTrack* Track					= SelKey.Group->InterpTracks(SelKey.TrackIndex);
		UInterpTrackSound* SoundTrack		= Cast<UInterpTrackSound>( Track );

		if( SoundTrack )
		{
			SoundTrackKeyIndices.AddItem(i);
			const FSoundTrackKey& SoundTrackKey	= SoundTrack->Sounds(SelKey.KeyIndex);
			if ( !bFoundVolume )
			{
				bFoundVolume = TRUE;
				Volume = SoundTrackKey.Volume;
			}
			else
			{
				if ( Abs(Volume-SoundTrackKey.Volume) > KINDA_SMALL_NUMBER )
				{
					bKeysDiffer = TRUE;
				}
			}
		}
	}

	if ( SoundTrackKeyIndices.Num() )
	{
		// Display dialog and let user enter new rate.
		const FString VolumeStr( FString::Printf( TEXT("%2.2f"), bKeysDiffer ? 1.f : Volume ) );
		WxDlgGenericStringEntry dlg;
		const INT Result = dlg.ShowModal( TEXT("SetSoundVolume"), TEXT("Volume"), *VolumeStr );
		if( Result == wxID_OK )
		{
			double NewVolume;
			const UBOOL bIsNumber = dlg.GetStringEntry().GetValue().ToDouble( &NewVolume );
			if( bIsNumber )
			{
				const FLOAT ClampedNewVolume = ::Clamp( (FLOAT)NewVolume, 0.f, 100.f );
				for ( INT i = 0 ; i < SoundTrackKeyIndices.Num() ; ++i )
				{
					const INT Index						= SoundTrackKeyIndices(i);
					const FInterpEdSelKey& SelKey		= Opt->SelectedKeys(Index);
					UInterpTrack* Track					= SelKey.Group->InterpTracks(SelKey.TrackIndex);
					UInterpTrackSound* SoundTrack		= CastChecked<UInterpTrackSound>( Track );
					FSoundTrackKey& SoundTrackKey		= SoundTrack->Sounds(SelKey.KeyIndex);
					SoundTrackKey.Volume				= ClampedNewVolume;
				}
			}
		}

		Interp->MarkPackageDirty();

		// Update stuff in case doing this has changed it.
		SetInterpPosition( Interp->Position );
	}
}

/**
 * Prompts the user to edit pitches for the selected sound keys.
 */
void WxInterpEd::OnSetSoundPitch(wxCommandEvent& In)
{
	TArray<INT> SoundTrackKeyIndices;
	UBOOL bFoundPitch = FALSE;
	UBOOL bKeysDiffer = FALSE;
	FLOAT Pitch = 1.0f;

	// Make a list of all keys and what their pitches are.
	for( INT i = 0 ; i < Opt->SelectedKeys.Num() ; ++i )
	{
		const FInterpEdSelKey& SelKey		= Opt->SelectedKeys(i);
		UInterpTrack* Track					= SelKey.Group->InterpTracks(SelKey.TrackIndex);
		UInterpTrackSound* SoundTrack		= Cast<UInterpTrackSound>( Track );

		if( SoundTrack )
		{
			SoundTrackKeyIndices.AddItem(i);
			const FSoundTrackKey& SoundTrackKey	= SoundTrack->Sounds(SelKey.KeyIndex);
			if ( !bFoundPitch )
			{
				bFoundPitch = TRUE;
				Pitch = SoundTrackKey.Pitch;
			}
			else
			{
				if ( Abs(Pitch-SoundTrackKey.Pitch) > KINDA_SMALL_NUMBER )
				{
					bKeysDiffer = TRUE;
				}
			}
		}
	}

	if ( SoundTrackKeyIndices.Num() )
	{
		// Display dialog and let user enter new rate.
		const FString PitchStr( FString::Printf( TEXT("%2.2f"), bKeysDiffer ? 1.f : Pitch ) );
		WxDlgGenericStringEntry dlg;
		const INT Result = dlg.ShowModal( TEXT("SetSoundPitch"), TEXT("Pitch"), *PitchStr );
		if( Result == wxID_OK )
		{
			double NewPitch;
			const UBOOL bIsNumber = dlg.GetStringEntry().GetValue().ToDouble( &NewPitch );
			if( bIsNumber )
			{
				const FLOAT ClampedNewPitch = ::Clamp( (FLOAT)NewPitch, 0.f, 100.f );
				for ( INT i = 0 ; i < SoundTrackKeyIndices.Num() ; ++i )
				{
					const INT Index						= SoundTrackKeyIndices(i);
					const FInterpEdSelKey& SelKey		= Opt->SelectedKeys(Index);
					UInterpTrack* Track					= SelKey.Group->InterpTracks(SelKey.TrackIndex);
					UInterpTrackSound* SoundTrack		= CastChecked<UInterpTrackSound>( Track );
					FSoundTrackKey& SoundTrackKey		= SoundTrack->Sounds(SelKey.KeyIndex);
					SoundTrackKey.Pitch					= ClampedNewPitch;
				}
			}
		}

		Interp->MarkPackageDirty();

		// Update stuff in case doing this has changed it.
		SetInterpPosition( Interp->Position );
	}
}

void WxInterpEd::OnContextDirKeyTransitionTime( wxCommandEvent& In )
{
	if(Opt->SelectedKeys.Num() != 1)
	{
		return;
	}

	FInterpEdSelKey& SelKey = Opt->SelectedKeys(0);
	UInterpTrack* Track = SelKey.Group->InterpTracks(SelKey.TrackIndex);
	UInterpTrackDirector* DirTrack = Cast<UInterpTrackDirector>(Track);
	if(!DirTrack)
	{
		return;
	}

	FLOAT CurrentTime = DirTrack->CutTrack(SelKey.KeyIndex).TransitionTime;
	const FString CurrentTimeStr = FString::Printf( TEXT("%3.3f"), CurrentTime );

	// Display dialog and let user enter new time.
	WxDlgGenericStringEntry dlg;
	const INT Result = dlg.ShowModal( TEXT("NewTransitionTime"), TEXT("Time"), *CurrentTimeStr);
	if( Result != wxID_OK )
		return;

	double dNewTime;
	const UBOOL bIsNumber = dlg.GetStringEntry().GetValue().ToDouble(&dNewTime);
	if(!bIsNumber)
		return;

	const FLOAT NewTime = (FLOAT)dNewTime;

	DirTrack->CutTrack(SelKey.KeyIndex).TransitionTime = NewTime;

	// Update stuff in case doing this has changed it.
	SetInterpPosition(Interp->Position);
}

void WxInterpEd::OnFlipToggleKey(wxCommandEvent& In)
{
	for (INT KeyIndex = 0; KeyIndex < Opt->SelectedKeys.Num(); KeyIndex++)
	{
		FInterpEdSelKey& SelKey = Opt->SelectedKeys(KeyIndex);
		UInterpTrack* Track = SelKey.Group->InterpTracks(SelKey.TrackIndex);
		UInterpTrackToggle* ToggleTrack = Cast<UInterpTrackToggle>(Track);
		if (ToggleTrack)
		{
			FToggleTrackKey& ToggleKey = ToggleTrack->ToggleTrack(SelKey.KeyIndex);
			ToggleKey.ToggleAction = (ToggleKey.ToggleAction == ETTA_Off) ? ETTA_On : ETTA_Off;
		}
	}
}

void WxInterpEd::OnMenuUndo(wxCommandEvent& In)
{
	InterpEdUndo();
}

void WxInterpEd::OnMenuRedo(wxCommandEvent& In)
{
	InterpEdRedo();
}

/** Menu handler for cut operations. */
void WxInterpEd::OnMenuCut( wxCommandEvent& In )
{
	CopySelectedGroupOrTrack(TRUE);
}

/** Menu handler for copy operations. */
void WxInterpEd::OnMenuCopy( wxCommandEvent& In )
{
	CopySelectedGroupOrTrack(FALSE);
}

/** Menu handler for paste operations. */
void WxInterpEd::OnMenuPaste( wxCommandEvent& In )
{
	PasteSelectedGroupOrTrack();
}

/** Update UI handler for edit menu items. */
void WxInterpEd::OnMenuEdit_UpdateUI( wxUpdateUIEvent& In )
{
	switch(In.GetId())
	{
	case IDM_INTERP_EDIT_UNDO:
		if(InterpEdTrans->CanUndo())
		{
			FString Label = FString::Printf(TEXT("%s : %s"), *LocalizeUnrealEd("Undo"), *InterpEdTrans->GetUndoDesc());
			In.SetText(*Label);
			In.Enable(TRUE);
		}
		else
		{
			In.Enable(FALSE);
		}
		break;
	case IDM_INTERP_EDIT_REDO:
		if(InterpEdTrans->CanRedo())
		{
			FString Label = FString::Printf(TEXT("%s : %s"), *LocalizeUnrealEd("Redo"), *InterpEdTrans->GetRedoDesc());
			In.SetText(*Label);
			In.Enable(TRUE);
		}
		else
		{
			In.Enable(FALSE);
		}
		break;
	case IDM_INTERP_EDIT_PASTE:
		{
			UBOOL bCanPaste = CanPasteGroupOrTrack();
			In.Enable(bCanPaste==TRUE);
		}
		break;
	}
}

void WxInterpEd::OnMenuImport( wxCommandEvent& )
{
	if( Interp != NULL )
	{
		WxFileDialog ImportFileDialog(this, *LocalizeUnrealEd("ImportMatineeSequence"), *(GApp->LastDir[LD_GENERIC_IMPORT]), TEXT(""), TEXT("COLLADA document|*.dae|All files|*.*"), wxOPEN | wxFILE_MUST_EXIST, wxDefaultPosition);

		// Show dialog and execute the import if the user did not cancel out
		if( ImportFileDialog.ShowModal() == wxID_OK )
		{
			// Get the filename from dialog
			wxString ImportFilename = ImportFileDialog.GetPath();
			FFilename FileName = ImportFilename.c_str();
			GApp->LastDir[LD_GENERIC_IMPORT] = FileName.GetPath(); // Save path as default for next time.
		
			// Import the Matinee information from the COLLADA document.
			UnCollada::CImporter* ColladaImporter = UnCollada::CImporter::GetInstance();
			ColladaImporter->ImportFromFile( ImportFilename.c_str() );
			if (ColladaImporter->IsParsingSuccessful())
			{
				if ( ColladaImporter->HasUnknownCameras() )
				{
					// Ask the user whether to create any missing cameras.
					int LResult = wxMessageBox(*LocalizeUnrealEd("ImportMatineeSequence_MissingCameras"), *LocalizeUnrealEd("ImportMatineeSequence"), wxICON_QUESTION | wxYES_NO | wxCENTRE);
					ColladaImporter->SetProcessUnknownCameras(LResult == wxYES);
				}

				// Re-create the Matinee sequence.
				ColladaImporter->ImportMatineeSequence(Interp);

				// We have modified the sequence, so update its UI.
				NotifyPostChange(NULL, NULL);
			}
			else
			{
				GWarn->Log( NAME_Error, ColladaImporter->GetErrorMessage() );
			}
			ColladaImporter->CloseDocument();
		}
	}
}

void WxInterpEd::OnMenuExport( wxCommandEvent& )
{
	if( Interp != NULL )
	{
		WxFileDialog ExportFileDialog(this, *LocalizeUnrealEd("ExportMatineeSequence"), *(GApp->LastDir[LD_GENERIC_EXPORT]), TEXT(""), TEXT("COLLADA document|*.dae"), wxSAVE | wxHIDE_READONLY | wxOVERWRITE_PROMPT, wxDefaultPosition);

		// Show dialog and execute the import if the user did not cancel out
		if( ExportFileDialog.ShowModal() == wxID_OK )
		{
			// Get the filename from dialog
			wxString ExportFilename = ExportFileDialog.GetPath();
			FFilename FileName = ExportFilename.c_str();
			GApp->LastDir[LD_GENERIC_EXPORT] = FileName.GetPath(); // Save path as default for next time.

			// Export the Matinee information to a COLLADA document.
			UnCollada::CExporter* ColladaExporter = UnCollada::CExporter::GetInstance();
			ColladaExporter->CreateDocument();
			ColladaExporter->ExportLevelMesh(GWorld->PersistentLevel);
			if (GWorld->CurrentLevel != GWorld->PersistentLevel)
			{
				ColladaExporter->ExportLevelMesh(GWorld->CurrentLevel);
			}
			ColladaExporter->ExportMatinee( Interp );
			ColladaExporter->WriteToFile( ExportFilename.c_str() );
		}
	}
}

void WxInterpEd::OnMenuReduceKeys(wxCommandEvent& In)
{
	ReduceKeys();
}

void WxInterpEd::OnToggleCurveEd(wxCommandEvent& In)
{
	bShowCurveEd = !bShowCurveEd;

	// Show curve editor by splitting the window
	if(bShowCurveEd && !GraphSplitterWnd->IsSplit())
	{
		GraphSplitterWnd->SplitHorizontally( CurveEd, TrackWindow, GraphSplitPos );
		CurveEd->Show(TRUE);
	}
	// Hide the graph editor 
	else if(!bShowCurveEd && GraphSplitterWnd->IsSplit())
	{
		GraphSplitterWnd->Unsplit( CurveEd );
	}

	// Update button status.
	ToolBar->ToggleTool( IDM_INTERP_TOGGLE_CURVEEDITOR, bShowCurveEd == TRUE );
	MenuBar->ViewMenu->Check( IDM_INTERP_TOGGLE_CURVEEDITOR, bShowCurveEd == TRUE );

	// Save to ini when it changes.
	GConfig->SetBool( TEXT("Matinee"), TEXT("ShowCurveEd"), bShowCurveEd, GEditorIni );
}

/** Called when sash position changes so we can remember the sash position. */
void WxInterpEd::OnGraphSplitChangePos(wxSplitterEvent& In)
{
	GraphSplitPos = GraphSplitterWnd->GetSashPosition();
}

/** Turn keyframe snap on/off. */
void WxInterpEd::OnToggleSnap(wxCommandEvent& In)
{
	SetSnapEnabled( In.IsChecked() );
}

/** The snap resolution combo box was changed. */
void WxInterpEd::OnChangeSnapSize(wxCommandEvent& In)
{
	const INT NewSelection = In.GetInt();
	check(NewSelection >= 0 && NewSelection <= ARRAY_COUNT(InterpEdSnapSizes)+ARRAY_COUNT(InterpEdFPSSnapSizes));

	if(NewSelection == ARRAY_COUNT(InterpEdSnapSizes)+ARRAY_COUNT(InterpEdFPSSnapSizes))
	{
		bSnapToKeys = TRUE;
		CurveEd->SetInSnap(FALSE, SnapAmount, bSnapToFrames);
	}
	else if(NewSelection<ARRAY_COUNT(InterpEdSnapSizes))	// see if they picked a second snap amount
	{
		bSnapToFrames = FALSE;
		bSnapToKeys = FALSE;
		SnapAmount = InterpEdSnapSizes[NewSelection];
		CurveEd->SetInSnap(bSnapEnabled, SnapAmount, bSnapToFrames);
	}
	else if(NewSelection<ARRAY_COUNT(InterpEdFPSSnapSizes)+ARRAY_COUNT(InterpEdSnapSizes))	// See if they picked a FPS snap amount.
	{
		bSnapToFrames = TRUE;
		bSnapToKeys = FALSE;
		SnapAmount = 1.0f/InterpEdFPSSnapSizes[NewSelection-ARRAY_COUNT(InterpEdSnapSizes)];
		CurveEd->SetInSnap(bSnapEnabled, SnapAmount, bSnapToFrames);
	}

	// Save selected snap mode to INI.
	GConfig->SetInt(TEXT("Matinee"), TEXT("SelectedSnapMode"), NewSelection, GEditorIni);
}

/** Adjust the view so the entire sequence fits into the viewport. */
void WxInterpEd::OnViewFitSequence(wxCommandEvent& In)
{
	InterpEdVC->ViewStartTime = 0.f;
	InterpEdVC->ViewEndTime = IData->InterpLength;

	SyncCurveEdView();
}

/** Adjust the view so the looped section fits into the viewport. */
void WxInterpEd::OnViewFitLoop(wxCommandEvent& In)
{
	// Do nothing if loop section is too small!
	FLOAT LoopRange = IData->EdSectionEnd - IData->EdSectionStart;
	if(LoopRange > 0.01f)
	{
		InterpEdVC->ViewStartTime = IData->EdSectionStart;
		InterpEdVC->ViewEndTime = IData->EdSectionEnd;

		SyncCurveEdView();
	}
}

/** Adjust the view so the looped section fits into the entire sequence. */
void WxInterpEd::OnViewFitLoopSequence(wxCommandEvent& In)
{
	// Adjust the looped section
	IData->EdSectionStart = 0.0f;
	IData->EdSectionEnd = IData->InterpLength;

	// Adjust the view
	InterpEdVC->ViewStartTime = IData->EdSectionStart;
	InterpEdVC->ViewEndTime = IData->EdSectionEnd;

	SyncCurveEdView();
}

/*-----------------------------------------------------------------------------
	WxInterpEdToolBar
-----------------------------------------------------------------------------*/

WxInterpEdToolBar::WxInterpEdToolBar( wxWindow* InParent, wxWindowID InID )
: wxToolBar( InParent, InID, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL | wxTB_3DBUTTONS )
{
	AddB.Load( TEXT("MAT_AddKey") );
	PlayB.Load( TEXT("MAT_Play") );
	LoopSectionB.Load( TEXT("MAT_PlayLoopSection") );
	StopB.Load( TEXT("MAT_Stop") );
	UndoB.Load( TEXT("MAT_Undo") );
	RedoB.Load( TEXT("MAT_Redo") );
	CurveEdB.Load( TEXT("MAT_CurveEd") );
	SnapB.Load( TEXT("MAT_ToggleSnap") );
	FitSequenceB.Load( TEXT("MAT_FitSequence") );
	FitLoopB.Load( TEXT("MAT_FitLoop") );
	FitLoopSequenceB.Load( TEXT("MAT_FitLoopSequence") );

	Speed1B.Load(TEXT("CASC_Speed_1"));
	Speed10B.Load(TEXT("CASC_Speed_10"));
	Speed25B.Load(TEXT("CASC_Speed_25"));
	Speed50B.Load(TEXT("CASC_Speed_50"));
	Speed100B.Load(TEXT("CASC_Speed_100"));

	SetToolBitmapSize( wxSize( 18, 18 ) );

	AddTool( IDM_INTERP_ADDKEY, AddB, *LocalizeUnrealEd("AddKey") );

	AddSeparator();

	AddTool( IDM_INTERP_PLAY, PlayB, *LocalizeUnrealEd("Play") );
	AddTool( IDM_INTERP_PLAYLOOPSECTION, LoopSectionB, *LocalizeUnrealEd("LoopSection") );
	AddTool( IDM_INTERP_STOP, StopB, *LocalizeUnrealEd("Stop") );

	AddSeparator();

	AddRadioTool(IDM_INTERP_SPEED_100,	*LocalizeUnrealEd("FullSpeed"), Speed100B, wxNullBitmap, *LocalizeUnrealEd("FullSpeed") );
	AddRadioTool(IDM_INTERP_SPEED_50,	*LocalizeUnrealEd("50Speed"), Speed50B, wxNullBitmap, *LocalizeUnrealEd("50Speed") );
	AddRadioTool(IDM_INTERP_SPEED_25,	*LocalizeUnrealEd("25Speed"), Speed25B, wxNullBitmap, *LocalizeUnrealEd("25Speed") );
	AddRadioTool(IDM_INTERP_SPEED_10,	*LocalizeUnrealEd("10Speed"), Speed10B, wxNullBitmap, *LocalizeUnrealEd("10Speed") );
	AddRadioTool(IDM_INTERP_SPEED_1,	*LocalizeUnrealEd("1Speed"), Speed1B, wxNullBitmap, *LocalizeUnrealEd("1Speed") );
	ToggleTool(IDM_INTERP_SPEED_100, TRUE);

	AddSeparator();

	AddTool( IDM_INTERP_EDIT_UNDO, UndoB, *LocalizeUnrealEd("Undo") );
	AddTool( IDM_INTERP_EDIT_REDO, RedoB, *LocalizeUnrealEd("Redo") );

	AddSeparator();

	AddCheckTool( IDM_INTERP_TOGGLE_CURVEEDITOR, *LocalizeUnrealEd("ToggleCurveEditor"), CurveEdB, wxNullBitmap, *LocalizeUnrealEd("ToggleCurveEditor") );

	AddSeparator();

	AddCheckTool( IDM_INTERP_TOGGLE_SNAP, *LocalizeUnrealEd("ToggleSnap"), SnapB, wxNullBitmap, *LocalizeUnrealEd("ToggleSnap") );
	
	// Create snap-size combo
	SnapCombo = new WxComboBox( this, IDM_INTERP_SNAPCOMBO, TEXT(""), wxDefaultPosition, wxSize(60, -1), 0, NULL, wxCB_READONLY );

	// Append Second Snap Times
	for(INT i=0; i<ARRAY_COUNT(InterpEdSnapSizes); i++)
	{
		FString SnapCaption = FString::Printf( TEXT("%1.2fs"), InterpEdSnapSizes[i] );
		SnapCombo->Append( *SnapCaption );
	}

	// Append FPS Snap Times
	for(INT i=0; i<ARRAY_COUNT(InterpEdFPSSnapSizes); i++)
	{
		FString SnapCaption = FString::Printf( TEXT("%d FPS"), InterpEdFPSSnapSizes[i] );
		SnapCombo->Append( *SnapCaption );
	}

	SnapCombo->Append(TEXT("Key")); // Add option for snapping to other keys.

	SnapCombo->SetSelection(2);

	AddControl(SnapCombo);

	AddSeparator();
	AddTool( IDM_INTERP_VIEW_FITSEQUENCE, *LocalizeUnrealEd("ViewFitSequence"), FitSequenceB );
	AddTool( IDM_INTERP_VIEW_FITLOOP, *LocalizeUnrealEd("ViewFitLoop"), FitLoopB );
	AddTool( IDM_INTERP_VIEW_FITLOOPSEQUENCE, *LocalizeUnrealEd("ViewFitLoopSequence"), FitLoopSequenceB );

	Realize();
}

WxInterpEdToolBar::~WxInterpEdToolBar()
{
}


/*-----------------------------------------------------------------------------
	WxInterpEdMenuBar
-----------------------------------------------------------------------------*/

WxInterpEdMenuBar::WxInterpEdMenuBar(WxInterpEd* InEditor)
{
	FileMenu = new wxMenu();
	Append( FileMenu, *LocalizeUnrealEd("File") );

	FileMenu->Append( IDM_INTERP_FILE_IMPORT, *LocalizeUnrealEd("Import"), TEXT("") );
	FileMenu->Append( IDM_INTERP_FILE_EXPORT, *LocalizeUnrealEd("Export"), TEXT("") );

	EditMenu = new wxMenu();
	Append( EditMenu, *LocalizeUnrealEd("Edit") );

	EditMenu->Append( IDM_INTERP_EDIT_UNDO, *LocalizeUnrealEd("Undo"), TEXT("") );
	EditMenu->Append( IDM_INTERP_EDIT_REDO, *LocalizeUnrealEd("Redo"), TEXT("") );
	EditMenu->AppendSeparator();
	EditMenu->Append( IDM_INTERP_EDIT_INSERTSPACE, *LocalizeUnrealEd("InsertSpaceCurrent"), TEXT("") );
	EditMenu->Append( IDM_INTERP_EDIT_STRETCHSECTION, *LocalizeUnrealEd("StretchSection"), TEXT("") );
	EditMenu->Append( IDM_INTERP_EDIT_DELETESECTION, *LocalizeUnrealEd("DeleteSection"), TEXT("") );
	EditMenu->Append( IDM_INTERP_EDIT_SELECTINSECTION, *LocalizeUnrealEd("SelectKeysSection"), TEXT("") );
	EditMenu->AppendSeparator();
	EditMenu->Append( IDM_INTERP_EDIT_DUPLICATEKEYS, *LocalizeUnrealEd("DuplicateSelectedKeys"), TEXT("") );
	EditMenu->AppendSeparator();
	EditMenu->Append( IDM_INTERP_EDIT_SAVEPATHTIME, *LocalizeUnrealEd("SavePathBuildingPositions"), TEXT("") );
	EditMenu->Append( IDM_INTERP_EDIT_JUMPTOPATHTIME, *LocalizeUnrealEd("JumpPathBuildingPositions"), TEXT("") );
	EditMenu->AppendSeparator();
	EditMenu->Append( IDM_OPEN_BINDKEYS_DIALOG,	*LocalizeUnrealEd("BindEditorHotkeys"), TEXT("") );
	EditMenu->AppendSeparator();
	EditMenu->Append( IDM_INTERP_EDIT_REDUCEKEYS, *LocalizeUnrealEd("ReduceKeys"), TEXT("") );

	ViewMenu = new wxMenu();
	Append( ViewMenu, *LocalizeUnrealEd("View") );

	ViewMenu->AppendCheckItem( IDM_INTERP_VIEW_HIDE3DTRACKS, *LocalizeUnrealEd("Hide3DTracks"), TEXT("") );
	bool bHideTrack = (InEditor->bHide3DTrackView == TRUE);
	ViewMenu->Check( IDM_INTERP_VIEW_HIDE3DTRACKS, bHideTrack );

	ViewMenu->AppendCheckItem( IDM_INTERP_VIEW_ZOOMTOSCUBPOS, *LocalizeUnrealEd("ZoomToScrubPos"), TEXT("") );
	bool bZoomToScub = (InEditor->bZoomToScrubPos == TRUE);
	ViewMenu->Check( IDM_INTERP_VIEW_ZOOMTOSCUBPOS, bZoomToScub );

	ViewMenu->AppendSeparator();
	ViewMenu->Append( IDM_INTERP_VIEW_FITSEQUENCE,	*LocalizeUnrealEd("ViewFitLoop"), TEXT(""));
	ViewMenu->Append( IDM_INTERP_VIEW_FITLOOP,	*LocalizeUnrealEd("ViewFitSequence"), TEXT(""));
	ViewMenu->Append( IDM_INTERP_VIEW_FITLOOPSEQUENCE,	*LocalizeUnrealEd("ViewFitLoopSequence"), TEXT(""));

	ViewMenu->AppendSeparator();
	ViewMenu->AppendCheckItem( IDM_INTERP_TOGGLE_CURVEEDITOR,	*LocalizeUnrealEd("ToggleCurveEditor"), TEXT(""));
	bool bCurveEditorVisible = (InEditor->bShowCurveEd == TRUE);
	ViewMenu->Check( IDM_INTERP_TOGGLE_CURVEEDITOR, bCurveEditorVisible );
}

WxInterpEdMenuBar::~WxInterpEdMenuBar()
{

}


/*-----------------------------------------------------------------------------
	WxMBInterpEdTabMenu
-----------------------------------------------------------------------------*/

WxMBInterpEdTabMenu::WxMBInterpEdTabMenu(WxInterpEd* InterpEd)
{
	UInterpFilter_Custom* Filter = Cast<UInterpFilter_Custom>(InterpEd->IData->SelectedFilter);
	if(Filter != NULL)
	{
		// make sure this isn't a default filter.
		if(InterpEd->IData->InterpFilters.ContainsItem(Filter))
		{
			Append(IDM_INTERP_GROUP_DELETETAB, *LocalizeUnrealEd("DeleteGroupTab"));
		}
	}
}

WxMBInterpEdTabMenu::~WxMBInterpEdTabMenu()
{

}


/*-----------------------------------------------------------------------------
	WxMBInterpEdGroupMenu
-----------------------------------------------------------------------------*/

WxMBInterpEdGroupMenu::WxMBInterpEdGroupMenu(WxInterpEd* InterpEd)
{
	// Nothing to do if no group selected
	if(!InterpEd->ActiveGroup)
		return;

	UBOOL bIsDirGroup = InterpEd->ActiveGroup->IsA(UInterpGroupDirector::StaticClass());

	for(INT i=0; i<InterpEd->InterpTrackClasses.Num(); i++)
	{
		UInterpTrack* DefTrack = (UInterpTrack*)InterpEd->InterpTrackClasses(i)->GetDefaultObject();
		if(!DefTrack->bDirGroupOnly)
		{
			FString NewTrackString = FString::Printf( *LocalizeUnrealEd("AddNew_F"), *InterpEd->InterpTrackClasses(i)->GetDescription() );
			Append( IDM_INTERP_NEW_TRACK_START+i, *NewTrackString, TEXT("") );
		}
	}

	// Add Director-group specific tracks to separate menu underneath.
	if(bIsDirGroup)
	{
		AppendSeparator();

		for(INT i=0; i<InterpEd->InterpTrackClasses.Num(); i++)
		{
			UInterpTrack* DefTrack = (UInterpTrack*)InterpEd->InterpTrackClasses(i)->GetDefaultObject();
			if(DefTrack->bDirGroupOnly)
			{
				FString NewTrackString = FString::Printf( *LocalizeUnrealEd("AddNew_F"), *InterpEd->InterpTrackClasses(i)->GetDescription() );
				Append( IDM_INTERP_NEW_TRACK_START+i, *NewTrackString, TEXT("") );
			}
		}
	}

	AppendSeparator();

	// Add CameraAnim export option if appropriate
	if (!bIsDirGroup)
	{
		UInterpGroupInst* GrInst = InterpEd->Interp->FindFirstGroupInst(InterpEd->ActiveGroup);
		check(GrInst);
		if (GrInst)
		{
			AActor* const GroupActor = GrInst->GetGroupActor();
			UBOOL bControllingACameraActor = GroupActor && GroupActor->IsA(ACameraActor::StaticClass());
			if (bControllingACameraActor)
			{
				// add strings to unrealed.int
				Append(IDM_INTERP_CAMERA_ANIM_EXPORT, *LocalizeUnrealEd("ExportCameraAnim"), *LocalizeUnrealEd("ExportCameraAnim_Desc"));
				AppendSeparator();
			}
		}
	}

	Append (IDM_INTERP_EDIT_CUT, *LocalizeUnrealEd("CutGroup"), *LocalizeUnrealEd("CutGroup_Desc"));
	Append (IDM_INTERP_EDIT_COPY, *LocalizeUnrealEd("CopyGroup"), *LocalizeUnrealEd("CopyGroup_Desc"));
	Append (IDM_INTERP_EDIT_PASTE, *LocalizeUnrealEd("PasteGroupOrTrack"), *LocalizeUnrealEd("PasteGroupOrTrack_Desc"));

	AppendSeparator();

	Append( IDM_INTERP_GROUP_RENAME, *LocalizeUnrealEd("RenameGroup"), TEXT("") );

	// Cannot duplicate Director group
	if(!bIsDirGroup)
	{
		Append( IDM_INTERP_GROUP_DUPLICATE, *LocalizeUnrealEd("DuplicateGroup"), TEXT("") );
	}

	Append( IDM_INTERP_GROUP_DELETE, *LocalizeUnrealEd("DeleteGroup"), TEXT("") );


	AppendSeparator();

	// Add entries for creating and sending to tabs.
	Append(IDM_INTERP_GROUP_CREATETAB, *LocalizeUnrealEd("CreateNewGroupTab"));

	// See if the user can remove this group from the current tab.
	UInterpFilter* Filter = Cast<UInterpFilter_Custom>(InterpEd->IData->SelectedFilter);
	if(Filter != NULL && InterpEd->ActiveGroup != NULL && InterpEd->IData->InterpFilters.ContainsItem(Filter))
	{
		Append(IDM_INTERP_GROUP_REMOVEFROMTAB, *LocalizeUnrealEd("RemoveFromGroupTab"));
	}

	if(InterpEd->Interp->InterpData->InterpFilters.Num())
	{
		wxMenu* TabMenu = new wxMenu();
		for(INT FilterIdx=0; FilterIdx<InterpEd->IData->InterpFilters.Num(); FilterIdx++)
		{
			UInterpFilter* Filter = InterpEd->IData->InterpFilters(FilterIdx);
			TabMenu->Append(IDM_INTERP_GROUP_SENDTOTAB_START+FilterIdx, *Filter->Caption);
		}

		Append(IDM_INTERP_GROUP_SENDTOTAB_SUBMENU, *LocalizeUnrealEd("SendToGroupTab"), TabMenu);
	}
}

WxMBInterpEdGroupMenu::~WxMBInterpEdGroupMenu()
{

}

/*-----------------------------------------------------------------------------
	WxMBInterpEdTrackMenu
-----------------------------------------------------------------------------*/

WxMBInterpEdTrackMenu::WxMBInterpEdTrackMenu(WxInterpEd* InterpEd)
{
	if(InterpEd->ActiveTrackIndex == INDEX_NONE)
		return;


	Append (IDM_INTERP_EDIT_CUT, *LocalizeUnrealEd("CutTrack"), *LocalizeUnrealEd("CutTrack_Desc"));
	Append (IDM_INTERP_EDIT_COPY, *LocalizeUnrealEd("CopyTrack"), *LocalizeUnrealEd("CopyTrack_Desc"));
	Append (IDM_INTERP_EDIT_PASTE, *LocalizeUnrealEd("PasteGroupOrTrack"), *LocalizeUnrealEd("PasteGroupOrTrack_Desc"));

	AppendSeparator();

	Append( IDM_INTERP_TRACK_RENAME, *LocalizeUnrealEd("RenameTrack"), TEXT("") );
	Append( IDM_INTERP_TRACK_DELETE, *LocalizeUnrealEd("DeleteTrack"), TEXT("") );

	check(InterpEd->ActiveGroup);
	UInterpTrack* Track = InterpEd->ActiveGroup->InterpTracks( InterpEd->ActiveTrackIndex );
	UInterpTrackMove* MoveTrack = Cast<UInterpTrackMove>(Track);

	if( MoveTrack )
	{
		AppendSeparator();
		AppendCheckItem( IDM_INTERP_TRACK_FRAME_WORLD, *LocalizeUnrealEd("WorldFrame"), TEXT("") );
		AppendCheckItem( IDM_INTERP_TRACK_FRAME_RELINITIAL, *LocalizeUnrealEd("RelativeInitial"), TEXT("") );

		// Check the currently the selected movement frame
		if( MoveTrack->MoveFrame == IMF_World )
		{
			Check(IDM_INTERP_TRACK_FRAME_WORLD, TRUE);
		}
		else if( MoveTrack->MoveFrame == IMF_RelativeToInitial )
		{
			Check(IDM_INTERP_TRACK_FRAME_RELINITIAL, TRUE);
		}
	}
}

WxMBInterpEdTrackMenu::~WxMBInterpEdTrackMenu()
{

}

/*-----------------------------------------------------------------------------
	WxMBInterpEdBkgMenu
-----------------------------------------------------------------------------*/

WxMBInterpEdBkgMenu::WxMBInterpEdBkgMenu(WxInterpEd* InterpEd)
{
	Append (IDM_INTERP_EDIT_PASTE, *LocalizeUnrealEd("PasteGroup"), *LocalizeUnrealEd("PasteGroup_Desc"));

	AppendSeparator();

	Append( IDM_INTERP_NEW_GROUP, *LocalizeUnrealEd("AddNewGroup"), TEXT("") );
	
	TArray<UInterpTrack*> Results;
	InterpEd->IData->FindTracksByClass( UInterpTrackDirector::StaticClass(), Results );
	if(Results.Num() == 0)
	{
		Append( IDM_INTERP_NEW_DIRECTOR_GROUP, *LocalizeUnrealEd("AddNewDirectorGroup"), TEXT("") );
	}
}

WxMBInterpEdBkgMenu::~WxMBInterpEdBkgMenu()
{

}

/*-----------------------------------------------------------------------------
	WxMBInterpEdKeyMenu
-----------------------------------------------------------------------------*/

WxMBInterpEdKeyMenu::WxMBInterpEdKeyMenu(WxInterpEd* InterpEd)
{
	UBOOL bHaveMoveKeys = FALSE;
	UBOOL bHaveFloatKeys = FALSE;
	UBOOL bHaveVectorKeys = FALSE;
	UBOOL bHaveColorKeys = FALSE;
	UBOOL bHaveEventKeys = FALSE;
	UBOOL bHaveAnimKeys = FALSE;
	UBOOL bHaveDirKeys = FALSE;
	UBOOL bAnimIsLooping = FALSE;
	UBOOL bHaveToggleKeys = FALSE;

	// TRUE if at least one sound key is selected.
	UBOOL bHaveSoundKeys = FALSE;

	for(INT i=0; i<InterpEd->Opt->SelectedKeys.Num(); i++)
	{
		FInterpEdSelKey& SelKey = InterpEd->Opt->SelectedKeys(i);
		UInterpTrack* Track = SelKey.Group->InterpTracks(SelKey.TrackIndex);

		if( Track->IsA(UInterpTrackMove::StaticClass()) )
		{
			bHaveMoveKeys = TRUE;
		}
		else if( Track->IsA(UInterpTrackEvent::StaticClass()) )
		{
			bHaveEventKeys = TRUE;
		}
		else if( Track->IsA(UInterpTrackDirector::StaticClass()) )
		{
			bHaveDirKeys = TRUE;
		}
		else if( Track->IsA(UInterpTrackAnimControl::StaticClass()) )
		{
			bHaveAnimKeys = TRUE;

			UInterpTrackAnimControl* AnimTrack = (UInterpTrackAnimControl*)Track;
			bAnimIsLooping = AnimTrack->AnimSeqs(SelKey.KeyIndex).bLooping;
		}
		else if( Track->IsA(UInterpTrackFloatBase::StaticClass()) )
		{
			bHaveFloatKeys = TRUE;
		}
		else if( Track->IsA(UInterpTrackColorProp::StaticClass()) )
		{
			bHaveColorKeys = TRUE;
		}
		else if( Track->IsA(UInterpTrackVectorBase::StaticClass()) )
		{
			bHaveVectorKeys = TRUE;
		}

		if( Track->IsA(UInterpTrackSound::StaticClass()) )
		{
			bHaveSoundKeys = TRUE;
		}

		if (Track->IsA(UInterpTrackToggle::StaticClass()))
		{
			bHaveToggleKeys = TRUE;
		}
	}

	if(bHaveMoveKeys || bHaveFloatKeys || bHaveVectorKeys)
	{
		wxMenu* MoveMenu = new wxMenu();
		MoveMenu->Append( IDM_INTERP_KEYMODE_CURVE, *LocalizeUnrealEd("Curve"), TEXT("") );
		MoveMenu->Append( IDM_INTERP_KEYMODE_CURVEBREAK, *LocalizeUnrealEd("CurveBreak"), TEXT("") );
		MoveMenu->Append( IDM_INTERP_KEYMODE_LINEAR, *LocalizeUnrealEd("Linear"), TEXT("") );
		MoveMenu->Append( IDM_INTERP_KEYMODE_CONSTANT, *LocalizeUnrealEd("Constant"), TEXT("") );
		Append( IDM_INTERP_MOVEKEYMODEMENU, *LocalizeUnrealEd("InterpMode"), MoveMenu );
	}

	if(InterpEd->Opt->SelectedKeys.Num() == 1)
	{
		Append( IDM_INTERP_KEY_SETTIME, *LocalizeUnrealEd("SetTime"), TEXT("") );

		if(bHaveMoveKeys)
		{
			AppendSeparator();

			wxMenuItem* SetLookupSourceItem = Append(IDM_INTERP_MOVEKEY_SETLOOKUP, *LocalizeUnrealEd("GetPositionFromAnotherGroup"), TEXT(""));
			wxMenuItem* ClearLookupSourceItem = Append(IDM_INTERP_MOVEKEY_CLEARLOOKUP, *LocalizeUnrealEd("ClearGroupLookup"), TEXT(""));

			FInterpEdSelKey& SelKey = InterpEd->Opt->SelectedKeys(0);
			UInterpTrackMove* MoveTrack = Cast<UInterpTrackMove>(SelKey.Group->InterpTracks(SelKey.TrackIndex));

			if( MoveTrack )
			{
				FName GroupName = MoveTrack->GetLookupKeyGroupName(SelKey.KeyIndex);

				if(GroupName == NAME_None)
				{
					ClearLookupSourceItem->Enable(FALSE);
				}
				else
				{
					ClearLookupSourceItem->SetText(*FString::Printf(*LocalizeUnrealEd("ClearGroupLookup_F"), *GroupName.ToString()));
				}
			}
		}

		if(bHaveFloatKeys)
		{
			Append( IDM_INTERP_KEY_SETVALUE, *LocalizeUnrealEd("SetValue"), TEXT("") );
		}

		if(bHaveColorKeys)
		{
			Append( IDM_INTERP_KEY_SETCOLOR, *LocalizeUnrealEd("SetColor"), TEXT("") );
		}

		if(bHaveEventKeys)
		{
			Append( IDM_INTERP_EVENTKEY_RENAME, *LocalizeUnrealEd("RenameEvent"), TEXT("") );
		}

		if(bHaveDirKeys)
		{
			Append(IDM_INTERP_DIRKEY_SETTRANSITIONTIME, *LocalizeUnrealEd("SetTransitionTime"));
		}

		if (bHaveToggleKeys)
		{
			Append(IDM_INTERP_TOGGLEKEY_FLIP, *LocalizeUnrealEd("FlipToggle"));
		}
	}

	if(bHaveAnimKeys)
	{
		Append(IDM_INTERP_ANIMKEY_LOOP, *LocalizeUnrealEd("SetAnimLooping"));
		Append(IDM_INTERP_ANIMKEY_NOLOOP, *LocalizeUnrealEd("SetAnimNoLooping"));

		if(InterpEd->Opt->SelectedKeys.Num() == 1)
		{
			Append(IDM_INTERP_ANIMKEY_SETSTARTOFFSET,  *LocalizeUnrealEd("SetStartOffset"));
			Append(IDM_INTERP_ANIMKEY_SETENDOFFSET,  *LocalizeUnrealEd("SetEndOffset"));
			Append(IDM_INTERP_ANIMKEY_SETPLAYRATE,  *LocalizeUnrealEd("SetPlayRate"));
			AppendCheckItem(IDM_INTERP_ANIMKEY_TOGGLEREVERSE,  *LocalizeUnrealEd("Reverse"));
		}
	}

	if ( bHaveSoundKeys )
	{
		Append( IDM_INTERP_SoundKey_SetVolume, *LocalizeUnrealEd("SetSoundVolume") );
		Append( IDM_INTERP_SoundKey_SetPitch, *LocalizeUnrealEd("SetSoundPitch") );
	}
}

WxMBInterpEdKeyMenu::~WxMBInterpEdKeyMenu()
{

}

/*-----------------------------------------------------------------------------
	WxMBCameraAnimEdGroupMenu
-----------------------------------------------------------------------------*/

WxMBCameraAnimEdGroupMenu::WxMBCameraAnimEdGroupMenu(WxCameraAnimEd* InterpEd)
{
	// Nothing to do if no group selected
	if(!InterpEd->ActiveGroup)
	{
		return;
	}

	for(INT i=0; i<InterpEd->InterpTrackClasses.Num(); i++)
	{
		UInterpTrack* DefTrack = (UInterpTrack*)InterpEd->InterpTrackClasses(i)->GetDefaultObject();
		if(!DefTrack->bDirGroupOnly)
		{
			FString NewTrackString = FString::Printf( *LocalizeUnrealEd("AddNew_F"), *InterpEd->InterpTrackClasses(i)->GetDescription() );
			Append( IDM_INTERP_NEW_TRACK_START+i, *NewTrackString, TEXT("") );
		}
	}

	AppendSeparator();

	// Add CameraAnim export option if appropriate
	UInterpGroupInst* GrInst = InterpEd->Interp->FindFirstGroupInst(InterpEd->ActiveGroup);
	check(GrInst);
	if (GrInst)
	{
		AActor* const GroupActor = GrInst->GetGroupActor();
		UBOOL bControllingACameraActor = GroupActor && GroupActor->IsA(ACameraActor::StaticClass());
		if (bControllingACameraActor)
		{
			// add strings to unrealed.int
			Append(IDM_INTERP_CAMERA_ANIM_EXPORT, *LocalizeUnrealEd("ExportCameraAnim"), *LocalizeUnrealEd("ExportCameraAnim_Desc"));
			AppendSeparator();
		}
	}

	Append( IDM_INTERP_GROUP_RENAME, *LocalizeUnrealEd("RenameGroup"), TEXT("") );
}

WxMBCameraAnimEdGroupMenu::~WxMBCameraAnimEdGroupMenu()
{
}


