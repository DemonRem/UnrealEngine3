/*=============================================================================
	AnimSetViewerMain.cpp: AnimSet viewer main
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "EngineAnimClasses.h"
#include "AnimSetViewer.h"
#include "UnColladaImporter.h"
#include "Properties.h"
#include "DlgGenericComboEntry.h"
#include "DlgRename.h"
#include "DlgRotateAnimSequence.h"
#include "BusyCursor.h"
#include "AnimationUtils.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Static helpers
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {
	/**
	 * Recompresses the sequences in specified anim set by applying each sequence's compression scheme.
	 * 
	 */
	static void RecompressSet(UAnimSet* AnimSet, USkeletalMesh* SkeletalMesh)
	{
		for( INT SequenceIndex = 0 ; SequenceIndex < AnimSet->Sequences.Num() ; ++SequenceIndex )
		{
			UAnimSequence* Seq = AnimSet->Sequences( SequenceIndex );
			UAnimationCompressionAlgorithm* CompressionAlgorithm = Seq->CompressionScheme ? Seq->CompressionScheme : FAnimationUtils::GetDefaultAnimationCompressionAlgorithm();
			CompressionAlgorithm->Reduce( Seq, SkeletalMesh, FALSE );
		}
	}

	/**
	 * Recompresses the specified sequence in anim set by applying the sequence's compression scheme.
	 * 
	 */
	static void RecompressSequence(UAnimSequence* Seq, USkeletalMesh* SkeletalMesh)
	{
		UAnimationCompressionAlgorithm* CompressionAlgorithm = Seq->CompressionScheme ? Seq->CompressionScheme : FAnimationUtils::GetDefaultAnimationCompressionAlgorithm();
		CompressionAlgorithm->Reduce( Seq, SkeletalMesh, FALSE );
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// WxAnimSetViewer
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void WxAnimSetViewer::SetSelectedSkelMesh(USkeletalMesh* InSkelMesh, UBOOL bClearMorphTarget)
{
	check(InSkelMesh);

	// Before we change mesh, clear any component we using for previewing attachments.
	ClearSocketPreviews();

	SelectedSkelMesh = InSkelMesh;

	SkelMeshCombo->Freeze();
	SkelMeshCombo->Clear();
	for(TObjectIterator<USkeletalMesh> It; It; ++It)
	{
		USkeletalMesh* SkelMesh = *It;

		if( !(SkelMesh->GetOutermost()->PackageFlags & PKG_Trash) )
		{
			SkelMeshCombo->Append( *SkelMesh->GetName(), SkelMesh );
		}
	}
	SkelMeshCombo->Thaw();

	// Set combo box to reflect new selection.
	for(INT i=0; i<SkelMeshCombo->GetCount(); i++)
	{
		if( SkelMeshCombo->GetClientData(i) == SelectedSkelMesh )
		{
			SkelMeshCombo->SetSelection(i); // This won't cause a new IDM_ANIMSET_SKELMESHCOMBO event.
		}
	}

	PreviewSkelComp->SetSkeletalMesh(SelectedSkelMesh);

	// When changing primary skeletal mesh - reset all extra ones.
	SkelMeshAux1Combo->SetSelection(0);
	PreviewSkelCompAux1->SetSkeletalMesh(NULL);
	PreviewSkelCompAux1->UpdateParentBoneMap();

	SkelMeshAux2Combo->SetSelection(0);
	PreviewSkelCompAux2->SetSkeletalMesh(NULL);
	PreviewSkelCompAux2->UpdateParentBoneMap();

	SkelMeshAux3Combo->SetSelection(0);
	PreviewSkelCompAux3->SetSkeletalMesh(NULL);
	PreviewSkelCompAux3->UpdateParentBoneMap();

	MeshProps->SetObject( NULL, true, false, true );
	MeshProps->SetObject( SelectedSkelMesh, true, false, true );

	// Update the AnimSet combo box to only show AnimSets that can be played on the new skeletal mesh.
	UpdateAnimSetCombo();

	// Update the Socket Manager
	UpdateSocketList();
	SetSelectedSocket(NULL);
	RecreateSocketPreviews();

	// Try and re-select the select AnimSet with the new mesh.
	SetSelectedAnimSet(SelectedAnimSet, false);

	// Select no morph target.
	if(bClearMorphTarget)
	{
		SetSelectedMorphSet(NULL, false);
	}

	// Reset LOD to Auto mode.
	PreviewSkelComp->ForcedLodModel = 0;
	MenuBar->Check( IDM_ANIMSET_LOD_AUTO, true );
	ToolBar->ToggleTool( IDM_ANIMSET_LOD_AUTO, true );

	// Update the buttons used for changing the desired LOD.
	UpdateForceLODButtons();

	// Turn off cloth sim when we change mesh.
	ToolBar->ToggleTool( IDM_ANIMSET_TOGGLECLOTH, false );
	PreviewSkelComp->TermClothSim(NULL);
	PreviewSkelComp->bEnableClothSimulation = FALSE;

	UpdateStatusBar();

	FASVViewportClient* ASVPreviewVC = GetPreviewVC();
	check( ASVPreviewVC );
	ASVPreviewVC->Viewport->Invalidate();

	// Remember we want to use this mesh with the current anim/morph set.
	if(SelectedSkelMesh && SelectedAnimSet)
	{
		FString MeshName = SelectedSkelMesh->GetPathName();
		SelectedAnimSet->PreviewSkelMeshName = FName( *MeshName );
	}
}

// If trying to set the AnimSet to one that cannot be played on selected SkelMesh, show message and just select first instead.
void WxAnimSetViewer::SetSelectedAnimSet(UAnimSet* InAnimSet, UBOOL bAutoSelectMesh)
{
	// Only allow selection of compatible AnimSets. 
	// AnimSetCombo should contain all AnimSets that can played on SelectedSkelMesh.

	// In case we have loaded some new packages, rebuild the list before finding the entry.
	UpdateAnimSetCombo();

	INT AnimSetIndex = INDEX_NONE;
	for(INT i=0; i<AnimSetCombo->GetCount(); i++)
	{
		if(AnimSetCombo->GetClientData(i) == InAnimSet)
		{
			AnimSetIndex = i;
		}
	}

	// If specified AnimSet is not compatible with current skeleton, show message and pick the first one.
	if(AnimSetIndex == INDEX_NONE)
	{
		if(InAnimSet)
		{
			appMsgf( AMT_OK, *LocalizeUnrealEd("Error_AnimSetCantBePlayedOnSM"), *InAnimSet->GetName(), *SelectedSkelMesh->GetName() );
		}

		AnimSetIndex = 0;
	}

	// Handle case where combo is empty - select no AnimSet
	if(AnimSetCombo->GetCount() > 0)
	{
		AnimSetCombo->SetSelection(AnimSetIndex);

		// Assign selected AnimSet
		SelectedAnimSet = (UAnimSet*)AnimSetCombo->GetClientData(AnimSetIndex);

		// Add newly selected AnimSet to the AnimSets array of the preview skeletal mesh.
		PreviewSkelComp->AnimSets.Empty();
		PreviewSkelComp->AnimSets.AddItem(SelectedAnimSet);

		AnimSetProps->SetObject( NULL, true, false, true );
		AnimSetProps->SetObject( SelectedAnimSet, true, false, true );
	}
	else
	{
		SelectedAnimSet = NULL;

		PreviewSkelComp->AnimSets.Empty();

		AnimSetProps->SetObject( NULL, true, false, true );
	}

	// Refresh the animation sequence list.
	UpdateAnimSeqList();

	// Select the first sequence (if present) - or none at all.
	if( AnimSeqList->GetCount() > 0 )
	{
		SetSelectedAnimSequence( (UAnimSequence*)(AnimSeqList->GetClientData(0)) );
	}
	else
	{
		SetSelectedAnimSequence( NULL );
	}

	UpdateStatusBar();

	// The menu title bar displays the full path name of the selected AnimSet
	FString WinTitle;
	if(SelectedAnimSet)
	{
		WinTitle = FString::Printf( *LocalizeUnrealEd("AnimSetEditor_F"), *SelectedAnimSet->GetPathName() );
	}
	else
	{
		WinTitle = FString::Printf( *LocalizeUnrealEd("AnimSetEditor") );
	}

	SetTitle( *WinTitle );


	// See if there is a skeletal mesh we would like to use with this AnimSet
	if(bAutoSelectMesh && SelectedAnimSet->PreviewSkelMeshName != NAME_None)
	{
		USkeletalMesh* PreviewSkelMesh = (USkeletalMesh*)UObject::StaticFindObject( USkeletalMesh::StaticClass(), ANY_PACKAGE, *SelectedAnimSet->PreviewSkelMeshName.ToString() );
		if(PreviewSkelMesh)
		{
			SetSelectedSkelMesh(PreviewSkelMesh, false);
		}
	}
}

void WxAnimSetViewer::SetSelectedAnimSequence(UAnimSequence* InAnimSeq)
{
	if(!InAnimSeq)
	{
		PreviewAnimNode->SetAnim(NAME_None);
		SelectedAnimSeq = NULL;
		AnimSeqProps->SetObject( NULL, true, false, true );
	}
	else
	{
		INT AnimSeqIndex = INDEX_NONE;
		for(INT i=0; i<AnimSeqList->GetCount(); i++)
		{
			if( AnimSeqList->GetClientData(i) == InAnimSeq )
			{
				AnimSeqIndex = i;
			}
		}		

		if(AnimSeqIndex == INDEX_NONE)
		{
			PreviewAnimNode->SetAnim(NAME_None);
			SelectedAnimSeq = NULL;
			AnimSeqProps->SetObject( NULL, true, false, true );
		}
		else
		{
			AnimSeqList->SetSelection( AnimSeqIndex );
			PreviewAnimNode->SetAnim( InAnimSeq->SequenceName );
			SelectedAnimSeq = InAnimSeq;

			AnimSeqProps->SetObject( NULL, true, false, true );
			AnimSeqProps->SetObject( SelectedAnimSeq, true, false, true );
		}
	}

	UpdateStatusBar();
}

/** Set the supplied MorphTargetSet as the selected one. */
void WxAnimSetViewer::SetSelectedMorphSet(UMorphTargetSet* InMorphSet, UBOOL bAutoSelectMesh)
{
	// In case we have loaded some new packages, rebuild the list before finding the entry.
	UpdateMorphSetCombo();

	INT MorphSetIndex = INDEX_NONE;
	for(INT i=0; i<MorphSetCombo->GetCount(); i++)
	{
		if(MorphSetCombo->GetClientData(i) == InMorphSet)
		{
			MorphSetIndex = i;
		}
	}

	// Combo should never be empty - there is always a -None- slot.
	check(MorphSetCombo->GetCount() > 0);
	MorphSetCombo->SetSelection(MorphSetIndex);

	SelectedMorphSet = (UMorphTargetSet*)MorphSetCombo->GetClientData(MorphSetIndex);
	// Note, it may be NULL at this point, because combo contains a 'None' entry.

	// assign MorphTargetSet to array in SkeletalMeshComponent. 
	PreviewSkelComp->MorphSets.Empty();
	if(SelectedMorphSet)
	{
		PreviewSkelComp->MorphSets.AddItem(SelectedMorphSet);
	}

	// Refresh the animation sequence list.
	UpdateMorphTargetList();

	// Select the first sequence (if present) - or none at all.
	if( MorphTargetList->GetCount() > 0 )
	{
		SetSelectedMorphTarget( (UMorphTarget*)(MorphTargetList->GetClientData(0)) );
	}
	else
	{
		SetSelectedMorphTarget( NULL );
	}

	UpdateStatusBar();

	// Set the BaseSkelMesh as the selected skeletal mesh.
	if( bAutoSelectMesh && 
		SelectedMorphSet && 
		SelectedMorphSet->BaseSkelMesh && 
		SelectedMorphSet->BaseSkelMesh != SelectedSkelMesh)
	{
		SetSelectedSkelMesh(SelectedMorphSet->BaseSkelMesh, false);
	}
}

/** Set the selected MorphTarget as the desired. */
void WxAnimSetViewer::SetSelectedMorphTarget(UMorphTarget* InMorphTarget)
{
	if(!InMorphTarget)
	{
		SelectedMorphTarget = NULL;
		PreviewMorphPose->SetMorphTarget(NAME_None);
	}
	else
	{
		INT MorphTargetIndex = INDEX_NONE;
		for(INT i=0; i<MorphTargetList->GetCount(); i++)
		{
			if( MorphTargetList->GetClientData(i) == InMorphTarget )
			{
				MorphTargetIndex = i;
			}
		}		

		if(MorphTargetIndex == INDEX_NONE)
		{
			SelectedMorphTarget = NULL;
			PreviewMorphPose->SetMorphTarget(NAME_None);
		}
		else
		{
			MorphTargetList->SetSelection( MorphTargetIndex );
			SelectedMorphTarget = InMorphTarget;
			PreviewMorphPose->SetMorphTarget(InMorphTarget->GetFName());
		}
	}
}

void WxAnimSetViewer::ImportCOLLADAAnim()
{
	if(!SelectedAnimSet)
	{
		appMsgf( AMT_OK, *LocalizeUnrealEd("Error_SelectAnimSetForImport") );
		return;
	}

	WxFileDialog ImportFileDialog( this, 
		*LocalizeUnrealEd("ImportCOLLADA_Anim_File"), 
		*(GApp->LastDir[LD_COLLADA_ANIM]),
		TEXT(""),
		TEXT("COLLADA .dae files|*.dae"),
		wxOPEN | wxFILE_MUST_EXIST | wxHIDE_READONLY | wxMULTIPLE,
		wxDefaultPosition);

	if( ImportFileDialog.ShowModal() == wxID_OK )
	{
		// Remember if this AnimSet was brand new
		const UBOOL bSetWasNew = SelectedAnimSet->TrackBoneNames.Num() == 0 ? TRUE : FALSE;

		wxArrayString ImportFilePaths;
		ImportFileDialog.GetPaths(ImportFilePaths);

		for(UINT FileIndex = 0; FileIndex < ImportFilePaths.Count(); FileIndex++)
		{
			const FFilename Filename( (const TCHAR*) ImportFilePaths[FileIndex] );
			GApp->LastDir[LD_COLLADA_ANIM] = Filename.GetPath(); // Save path as default for next time.

			UEditorEngine::ImportCOLLADAANIMIntoAnimSet( SelectedAnimSet, *Filename, SelectedSkelMesh );

			SelectedAnimSet->MarkPackageDirty();
		}

		// If this set was new - we have just created the bone-track-table.
		// So we now need to check if we can still play it on our selected SkeletalMesh.
		// If we can't, we look for a few one which can play it.
		// If we can't find any mesh to play this new set on, we fail the import and reset the AnimSet.
		if(bSetWasNew)
		{
			if( !SelectedAnimSet->CanPlayOnSkeletalMesh(SelectedSkelMesh) )
			{
				USkeletalMesh* NewSkelMesh = NULL;
				for(TObjectIterator<USkeletalMesh> It; It && !NewSkelMesh; ++It)
				{
					USkeletalMesh* TestSkelMesh = *It;
					if( SelectedAnimSet->CanPlayOnSkeletalMesh(TestSkelMesh) )
					{
						NewSkelMesh = TestSkelMesh;
					}
				}

				if(NewSkelMesh)
				{
					SetSelectedSkelMesh(NewSkelMesh, TRUE);
				}
				else
				{
					appMsgf( AMT_OK, *LocalizeUnrealEd("Error_PSACouldNotBePlayed") );
					SelectedAnimSet->ResetAnimSet();
					return;
				}
			}
		}

		// Refresh AnimSet combo to show new number of sequences in this AnimSet.
		const INT CurrentSelection = AnimSetCombo->GetSelection();
		UpdateAnimSetCombo();
		AnimSetCombo->SetSelection(CurrentSelection);

		// Refresh AnimSequence list box to show any new anims.
		UpdateAnimSeqList();

		// Reselect current animation sequence. If none selected, pick the first one in the box (if not empty).
		if(SelectedAnimSeq)
		{
			SetSelectedAnimSequence(SelectedAnimSeq);
		}
		else
		{
			if(AnimSeqList->GetCount() > 0)
			{
				SetSelectedAnimSequence( (UAnimSequence*)(AnimSeqList->GetClientData(0)) );
			}
		}

		SelectedAnimSet->MarkPackageDirty();
	}
	return; 	
}

void WxAnimSetViewer::ImportPSA()
{
	if(!SelectedAnimSet)
	{
		appMsgf( AMT_OK, *LocalizeUnrealEd("Error_SelectAnimSetForImport") );
		return;
	}

	WxFileDialog ImportFileDialog( this, 
		*LocalizeUnrealEd("ImportPSAFile"), 
		*(GApp->LastDir[LD_PSA]),
		TEXT(""),
		TEXT("PSA files|*.psa"),
		wxOPEN | wxFILE_MUST_EXIST | wxHIDE_READONLY | wxMULTIPLE,
		wxDefaultPosition);

	if( ImportFileDialog.ShowModal() == wxID_OK )
	{
		// Remember if this AnimSet was brand new
		const UBOOL bSetWasNew = SelectedAnimSet->TrackBoneNames.Num() == 0 ? TRUE : FALSE;

		wxArrayString ImportFilePaths;
		ImportFileDialog.GetPaths(ImportFilePaths);

		for(UINT FileIndex = 0; FileIndex < ImportFilePaths.Count(); FileIndex++)
		{
			const FFilename Filename( (const TCHAR*)ImportFilePaths[FileIndex] );
			GApp->LastDir[LD_PSA] = Filename.GetPath(); // Save path as default for next time.

			UEditorEngine::ImportPSAIntoAnimSet( SelectedAnimSet, *Filename, SelectedSkelMesh );

			SelectedAnimSet->MarkPackageDirty();
		}

		// If this set was new - we have just created the bone-track-table.
		// So we now need to check if we can still play it on our selected SkeletalMesh.
		// If we can't, we look for a few one which can play it.
		// If we can't find any mesh to play this new set on, we fail the import and reset the AnimSet.
		if(bSetWasNew)
		{
			if( !SelectedAnimSet->CanPlayOnSkeletalMesh(SelectedSkelMesh) )
			{
				USkeletalMesh* NewSkelMesh = NULL;
				for(TObjectIterator<USkeletalMesh> It; It && !NewSkelMesh; ++It)
				{
					USkeletalMesh* TestSkelMesh = *It;
					if( SelectedAnimSet->CanPlayOnSkeletalMesh(TestSkelMesh) )
					{
						NewSkelMesh = TestSkelMesh;
					}
				}

				if(NewSkelMesh)
				{
					SetSelectedSkelMesh(NewSkelMesh, true);
				}
				else
				{
					appMsgf( AMT_OK, *LocalizeUnrealEd("Error_PSACouldNotBePlayed") );
					SelectedAnimSet->ResetAnimSet();
					return;
				}
			}
		}

		// Refresh AnimSet combo to show new number of sequences in this AnimSet.
		const INT CurrentSelection = AnimSetCombo->GetSelection();
		UpdateAnimSetCombo();
		AnimSetCombo->SetSelection(CurrentSelection);
			
		// Refresh AnimSequence list box to show any new anims.
		UpdateAnimSeqList();

		// Reselect current animation sequence. If none selected, pick the first one in the box (if not empty).
		if(SelectedAnimSeq)
		{
			SetSelectedAnimSequence(SelectedAnimSeq);
		}
		else
		{
			if(AnimSeqList->GetCount() > 0)
			{
				SetSelectedAnimSequence( (UAnimSequence*)(AnimSeqList->GetClientData(0)) );
			}
		}

		SelectedAnimSet->MarkPackageDirty();
	}
}

IMPLEMENT_COMPARE_CONSTREF( BYTE, AnimSetViewerTools, { return (A - B); } )

/** Import a .PSK file as an LOD of the selected SkeletalMesh. */
void WxAnimSetViewer::ImportMeshLOD()
{
	if(!SelectedSkelMesh)
	{
		return;
	}

	// First, display the file open dialog for selecting the .PSK file.
	WxFileDialog ImportFileDialog( this, 
		*LocalizeUnrealEd("ImportMeshLOD"), 
		*(GApp->LastDir[LD_PSA]),
		TEXT(""),
		TEXT("PSK files|*.psk"),
		wxOPEN | wxFILE_MUST_EXIST,
		wxDefaultPosition);

	// Only continue if we pressed OK and have only one file selected.
	if( ImportFileDialog.ShowModal() == wxID_OK )
	{
		wxArrayString ImportFilePaths;
		ImportFileDialog.GetPaths(ImportFilePaths);

		if(ImportFilePaths.Count() == 0)
		{
			appMsgf( AMT_OK, *LocalizeUnrealEd("NoFileSelectedForLOD") );
		}
		else if(ImportFilePaths.Count() > 1)
		{
			appMsgf( AMT_OK, *LocalizeUnrealEd("MultipleFilesSelectedForLOD") );
		}
		else
		{
			// Now display combo to choose which LOD to import this mesh as.
			TArray<FString> LODStrings;
			LODStrings.AddZeroed( SelectedSkelMesh->LODModels.Num() );
			for(INT i=0; i<SelectedSkelMesh->LODModels.Num(); i++)
			{
				LODStrings(i) = FString::Printf( TEXT("%d"), i+1 );
			}

			WxDlgGenericComboEntry dlg;
			if( dlg.ShowModal( TEXT("ChooseLODLevel"), TEXT("LODLevel:"), LODStrings, 0, TRUE ) == wxID_OK )
			{
				INT DesiredLOD = dlg.GetComboBox().GetSelection() + 1;

				// If the LOD we want
				if( DesiredLOD > 0 && DesiredLOD <= SelectedSkelMesh->LODModels.Num() )
				{
					FFilename Filename = (const TCHAR*)ImportFilePaths[0];
					GApp->LastDir[LD_PSA] = Filename.GetPath(); // Save path as default for next time.

					// Load the data from the file into a byte array.
					TArray<BYTE> Data;
					if( appLoadFileToArray( Data, *Filename ) )
					{
						Data.AddItem( 0 );
						const BYTE* Ptr = &Data( 0 );

						// Use the SkeletalMeshFactory to load this SkeletalMesh into a temporary SkeletalMesh.
						USkeletalMeshFactory* SkelMeshFact = new USkeletalMeshFactory();
						USkeletalMesh* TempSkelMesh = (USkeletalMesh*)SkelMeshFact->FactoryCreateBinary( 
							USkeletalMesh::StaticClass(), UObject::GetTransientPackage(), NAME_None, 0, NULL, NULL, Ptr, Ptr+Data.Num()-1, GWarn );

						if(TempSkelMesh)
						{
							// Now we copy the base FStaticLODModel from the imported skeletal mesh as the new LOD in the selected mesh.
							check(TempSkelMesh->LODModels.Num() == 1);

							// Names of root bones must match.
							if(TempSkelMesh->RefSkeleton(0).Name != SelectedSkelMesh->RefSkeleton(0).Name)
							{
								appMsgf( AMT_OK, *LocalizeUnrealEd("LODRootNameIncorrect"), *TempSkelMesh->RefSkeleton(0).Name.ToString(), *SelectedSkelMesh->RefSkeleton(0).Name.ToString() );
								return;
							}

							// We do some checking here that for every bone in the mesh we just imported, it's in our base ref skeleton, and the parent is the same.
							for(INT i=0; i<TempSkelMesh->RefSkeleton.Num(); i++)
							{
								INT LODBoneIndex = i;
								FName LODBoneName = TempSkelMesh->RefSkeleton(LODBoneIndex).Name;
								INT BaseBoneIndex = SelectedSkelMesh->MatchRefBone(LODBoneName);
								if( BaseBoneIndex == INDEX_NONE )
								{
									// If we could not find the bone from this LOD in base mesh - we fail.
									appMsgf( AMT_OK, *LocalizeUnrealEd("LODBoneDoesNotMatch"), *LODBoneName.ToString(), *SelectedSkelMesh->GetName() );
									return;
								}

								if(i>0)
								{
									INT LODParentIndex = TempSkelMesh->RefSkeleton(LODBoneIndex).ParentIndex;
									FName LODParentName = TempSkelMesh->RefSkeleton(LODParentIndex).Name;

									INT BaseParentIndex = SelectedSkelMesh->RefSkeleton(BaseBoneIndex).ParentIndex;
									FName BaseParentName = SelectedSkelMesh->RefSkeleton(BaseParentIndex).Name;

									if(LODParentName != BaseParentName)
									{
										// If bone has different parents, display an error and don't allow import.
										appMsgf( AMT_OK, *LocalizeUnrealEd("LODBoneHasIncorrectParent"), *LODBoneName.ToString(), *LODParentName.ToString(), *BaseParentName.ToString() );
										return;
									}
								}
							}

							FStaticLODModel& NewLODModel = TempSkelMesh->LODModels(0);

							// Enforce LODs having only single-influence vertices.
							UBOOL bCheckSingleInfluence;
							GConfig->GetBool( TEXT("AnimSetViewer"), TEXT("CheckSingleInfluenceLOD"), bCheckSingleInfluence, GEditorIni );
							if(bCheckSingleInfluence)
							{
								for(INT ChunkIndex = 0;ChunkIndex < NewLODModel.Chunks.Num();ChunkIndex++)
								{
									if(NewLODModel.Chunks(ChunkIndex).SoftVertices.Num() > 0)
									{
										appMsgf( AMT_OK, *LocalizeUnrealEd("LODHasSoftVertices") );
									}
								}
							}

							// If this LOD is going to be the lowest one, we check all bones we have sockets on are present in it.
							if( DesiredLOD == SelectedSkelMesh->LODModels.Num() || 
								DesiredLOD == SelectedSkelMesh->LODModels.Num()-1 )
							{
								for(INT i=0; i<SelectedSkelMesh->Sockets.Num(); i++)
								{
									// Find bone index the socket is attached to.
									USkeletalMeshSocket* Socket = SelectedSkelMesh->Sockets(i);
									INT SocketBoneIndex = TempSkelMesh->MatchRefBone( Socket->BoneName );

									// If this LOD does not contain the socket bone, abort import.
									if( SocketBoneIndex == INDEX_NONE )
									{
										appMsgf( AMT_OK, *LocalizeUnrealEd("LODMissingSocketBone"), *Socket->BoneName.ToString(), *Socket->SocketName.ToString() );
										return;
									}
								}
							}

							// Fix up the ActiveBoneIndices array.
							for(INT i=0; i<NewLODModel.ActiveBoneIndices.Num(); i++)
							{
								INT LODBoneIndex = NewLODModel.ActiveBoneIndices(i);
								FName LODBoneName = TempSkelMesh->RefSkeleton(LODBoneIndex).Name;
								INT BaseBoneIndex = SelectedSkelMesh->MatchRefBone(LODBoneName);
								NewLODModel.ActiveBoneIndices(i) = BaseBoneIndex;
							}

							// Fix up the chunk BoneMaps.
							for(INT ChunkIndex = 0;ChunkIndex < NewLODModel.Chunks.Num();ChunkIndex++)
							{
								FSkelMeshChunk& Chunk = NewLODModel.Chunks(ChunkIndex);
								for(INT i=0; i<Chunk.BoneMap.Num(); i++)
								{
									INT LODBoneIndex = Chunk.BoneMap(i);
									FName LODBoneName = TempSkelMesh->RefSkeleton(LODBoneIndex).Name;
									INT BaseBoneIndex = SelectedSkelMesh->MatchRefBone(LODBoneName);
									Chunk.BoneMap(i) = BaseBoneIndex;
								}
							}

							// Create the RequiredBones array in the LODModel from the ref skeleton.
							NewLODModel.RequiredBones.Empty();
							for(INT i=0; i<TempSkelMesh->RefSkeleton.Num(); i++)
							{
								FName LODBoneName = TempSkelMesh->RefSkeleton(i).Name;
								INT BaseBoneIndex = SelectedSkelMesh->MatchRefBone(LODBoneName);
								if(BaseBoneIndex != INDEX_NONE)
								{
									NewLODModel.RequiredBones.AddItem(BaseBoneIndex);
								}
							}

							// Also sort the RequiredBones array to be strictly increasing.
							Sort<USE_COMPARE_CONSTREF(BYTE, AnimSetViewerTools)>( &NewLODModel.RequiredBones(0), NewLODModel.RequiredBones.Num() );


							// To be extra-nice, we apply the difference between the root transform of the meshes to the verts.
							FMatrix LODToBaseTransform = TempSkelMesh->GetRefPoseMatrix(0).Inverse() * SelectedSkelMesh->GetRefPoseMatrix(0);

							for(INT ChunkIndex = 0;ChunkIndex < NewLODModel.Chunks.Num();ChunkIndex++)
							{
								FSkelMeshChunk& Chunk = NewLODModel.Chunks(ChunkIndex);
								// Fix up rigid verts.
								for(INT i=0; i<Chunk.RigidVertices.Num(); i++)
								{
									Chunk.RigidVertices(i).Position = LODToBaseTransform.TransformFVector( Chunk.RigidVertices(i).Position );
									Chunk.RigidVertices(i).TangentX = LODToBaseTransform.TransformNormal( Chunk.RigidVertices(i).TangentX );
									Chunk.RigidVertices(i).TangentY = LODToBaseTransform.TransformNormal( Chunk.RigidVertices(i).TangentY );
									Chunk.RigidVertices(i).TangentZ = LODToBaseTransform.TransformNormal( Chunk.RigidVertices(i).TangentZ );
								}

								// Fix up soft verts.
								for(INT i=0; i<Chunk.SoftVertices.Num(); i++)
								{
									Chunk.SoftVertices(i).Position = LODToBaseTransform.TransformFVector( Chunk.SoftVertices(i).Position );
									Chunk.SoftVertices(i).TangentX = LODToBaseTransform.TransformNormal( Chunk.SoftVertices(i).TangentX );
									Chunk.SoftVertices(i).TangentY = LODToBaseTransform.TransformNormal( Chunk.SoftVertices(i).TangentY );
									Chunk.SoftVertices(i).TangentZ = LODToBaseTransform.TransformNormal( Chunk.SoftVertices(i).TangentZ );
								}
							}

							// Shut down the skeletal mesh component that is previewing this mesh.
							{
								FComponentReattachContext ReattachContextPreview(PreviewSkelComp);

								// If we want to add this as a new LOD to this mesh - add to LODModels/LODInfo array.
								if(DesiredLOD == SelectedSkelMesh->LODModels.Num())
								{
									new(SelectedSkelMesh->LODModels)FStaticLODModel();

									// Add element to LODInfo array.
									SelectedSkelMesh->LODInfo.AddZeroed();
									check( SelectedSkelMesh->LODInfo.Num() == SelectedSkelMesh->LODModels.Num() );

									SelectedSkelMesh->LODInfo(DesiredLOD).LODHysteresis = 0.02f;
								}

								// Set up LODMaterialMap to number of materials in new mesh.
								FSkeletalMeshLODInfo& LODInfo = SelectedSkelMesh->LODInfo(DesiredLOD);
								LODInfo.LODMaterialMap.Empty();

								// Now set up the material mapping array.
								for(INT MatIdx = 0; MatIdx < TempSkelMesh->Materials.Num(); MatIdx++)
								{
									// Try and find the auto-assigned material in the array.
									INT LODMatIndex = SelectedSkelMesh->Materials.FindItemIndex( TempSkelMesh->Materials(MatIdx) );

									// If we didn't just use the index - but make sure its within range of the Materials array.
									if(LODMatIndex == INDEX_NONE)
									{
										LODMatIndex = ::Clamp(MatIdx, 0, SelectedSkelMesh->Materials.Num() - 1);
									}

									LODInfo.LODMaterialMap.AddItem(LODMatIndex);
								}

								// Assign new FStaticLODModel to desired slot in selected skeletal mesh.
								SelectedSkelMesh->LODModels(DesiredLOD) = NewLODModel;

								// ReattachContexts go out of scope here, reattaching skel components to the scene.
							}

							// Update buttons to reflect new LOD.
							UpdateForceLODButtons();
								
							// Pop up a box saying it worked.
							appMsgf( AMT_OK, *LocalizeUnrealEd("LODImportSuccessful"), DesiredLOD );

							// Mark package containing skeletal mesh as dirty.
							SelectedSkelMesh->MarkPackageDirty();
						}
					}
				}
			}
		}
	}
}

/**
* Displays and handles dialogs necessary for importing 
* a new morph target mesh from file. The new morph
* target is placed in the currently selected MorphTargetSet
*
* @param bImportToLOD - if TRUE then new files will be treated as morph target LODs 
* instead of new morph target resources
*/
void WxAnimSetViewer::ImportMorphTarget(UBOOL bImportToLOD)
{
	if( !SelectedMorphSet )
	{
		appMsgf( AMT_OK, *LocalizeUnrealEd("Error_SelectMorphSetForImport") );
		return;
	}

	// The MorphTargetSet should be bound to a skel mesh on creation
	if( !SelectedMorphSet->BaseSkelMesh )
	{	
		return;
	}

	// First, display the file open dialog for selecting the .PSK files.
	WxFileDialog ImportFileDialog( this, 
		*LocalizeUnrealEd("ImportMorphTarget"), 
		*(GApp->LastDir[LD_PSA]),
		TEXT(""),
		TEXT("PSK files|*.psk|COLLADA document|*.dae|All files|*.*"),
		wxOPEN | wxFILE_MUST_EXIST | wxMULTIPLE,
		wxDefaultPosition);
	
	// show dialog and handle OK
	if( ImportFileDialog.ShowModal() == wxID_OK )
	{
		// get list of files from dialog
		wxArrayString ImportFilePaths;
		ImportFileDialog.GetPaths(ImportFilePaths);

		// iterate over all selected files
		for( UINT FileIdx=0; FileIdx < ImportFilePaths.Count(); FileIdx++ )
		{
			// path and filename to import from
			FFilename Filename = (const TCHAR*)ImportFilePaths[FileIdx];
			// Check the file extension for PSK/DAE switch. Anything that isn't .DAE is considered a PSK file.
			const FString FileExtension = Filename.GetExtension();
			const UBOOL bIsCOLLADA = appStricmp(*FileExtension, TEXT("DAE")) == 0;
			// keep track of error reported
			EMorphImportError ImportError=MorphImport_OK;

			// handle importing to LOD level of an existing morph target
			if( bImportToLOD &&
				SelectedMorphSet->Targets.Num() > 0 )
			{
				// create a list of morph targets to display in the dlg
				TArray<UObject*> MorphList;
				for( INT Idx=0; Idx < SelectedMorphSet->Targets.Num(); Idx++)
				{
					MorphList.AddItem(SelectedMorphSet->Targets(Idx));
				}
				// only allow up to same number of LODs as the base skel mesh
				INT MaxLODIdx = SelectedMorphSet->BaseSkelMesh->LODModels.Num()-1;
				check(MaxLODIdx >= 0);
				// show dialog for importing an LOD level
				WxDlgMorphLODImport Dlg(this,MorphList,MaxLODIdx,*Filename.GetBaseFilename());
				if( Dlg.ShowModal() == wxID_OK )
				{
					// get user input from dlg
					INT SelectedMorphIdx = Dlg.GetSelectedMorph();
					INT SelectedLODIdx = Dlg.GetLODLevel();

					// import mesh to LOD
					if( !bIsCOLLADA )
					{
						// create the morph target importer
						FMorphTargetBinaryImport MorphImport( SelectedMorphSet->BaseSkelMesh, SelectedLODIdx, GWarn );
						// import the mesh LOD
						if( SelectedMorphSet->Targets.IsValidIndex(SelectedMorphIdx) )
						{
							MorphImport.ImportMorphLODModel(
								SelectedMorphSet->Targets(SelectedMorphIdx),
								*Filename,
								SelectedLODIdx,
								&ImportError
								);			
						}
						else
						{
							ImportError = MorphImport_MissingMorphTarget;
						}
					}
					else
					{
						UnCollada::CImporter* ColladaImporter = UnCollada::CImporter::GetInstance();
						if ( !ColladaImporter->ImportFromFile(*Filename) )
						{
							// Log the error message and fail the import.
							GWarn->Log( NAME_Error, ColladaImporter->GetErrorMessage() );
							ImportError = MorphImport_CantLoadFile;
						}
						else
						{
							//@todo sz - handle importing morph LOD from collada file
							GWarn->Log( TEXT("Morph target LOD importing from Collada format not supported at this time.") );
						}
					}					
				}
			}
			// handle importing a new morph target object
			else
			{
				// show dialog to specify the new morph target's name
				WxDlgGenericStringEntry NameDlg;
				if( NameDlg.ShowModal(TEXT("NewMorphTargetName"), TEXT("NewMorphTargetName"), *Filename.GetBaseFilename()) == wxID_OK )
				{
					// create the morph target importer
					FMorphTargetBinaryImport MorphImport( SelectedMorphSet->BaseSkelMesh, 0, GWarn );

					// get the name entered
					FName NewTargetName( *NameDlg.GetEnteredString() );

					// import the mesh
					if ( !bIsCOLLADA )
					{
						MorphImport.ImportMorphMeshToSet( 
							SelectedMorphSet, 
							NewTargetName, 
							*Filename,
							FALSE,
							&ImportError );
					}
					else
					{
						UnCollada::CImporter* ColladaImporter = UnCollada::CImporter::GetInstance();
						if ( !ColladaImporter->ImportFromFile(*Filename) )
						{
							// Log the error message and fail the import.
							GWarn->Log( NAME_Error, ColladaImporter->GetErrorMessage() );
							ImportError = MorphImport_CantLoadFile;
						}
						else
						{
							// Log the import message and import the animation.
							GWarn->Log( ColladaImporter->GetErrorMessage() );

							// Retrieve the list of all the meshes within the COLLADA document
							TArray<const TCHAR*> ColladaMeshes;
							ColladaImporter->RetrieveEntityList(ColladaMeshes, TRUE);
							ColladaImporter->RetrieveEntityList(ColladaMeshes, FALSE);
							if ( ColladaMeshes.Num() > 0 )
							{
								// TODO: Fill these arrays with only the wanted names to import.
								// For now: pick the first (up to) three possibilities and import them.
								TArray<FName> MorphTargetNames;
								TArray<const TCHAR*> ColladaImportNames;
								for( INT MeshIndex = 0; MeshIndex < ColladaMeshes.Num(); ++MeshIndex )
								{
									MorphTargetNames.AddItem( FName( ColladaMeshes( MeshIndex ) ) );
									ColladaImportNames.AddItem( ColladaMeshes( MeshIndex) );
								}

								ColladaImporter->ImportMorphTarget( SelectedMorphSet->BaseSkelMesh, SelectedMorphSet, MorphTargetNames, ColladaImportNames, TRUE, &ImportError );
							}
							else
							{
								ImportError = MorphImport_CantLoadFile;
							}
						}
					}

					// handle an existing target by asking the user to replace the existing one
					if( ImportError == MorphImport_AlreadyExists &&
						appMsgf( AMT_YesNoCancel, *LocalizeUnrealEd("Prompt_29") )==0 )
					{
						MorphImport.ImportMorphMeshToSet( 
							SelectedMorphSet, 
							NewTargetName, 
							*Filename,
							TRUE,
							&ImportError );
					}					
				}
			}

			// handle errors
			switch( ImportError )
			{
			case MorphImport_AlreadyExists:
				//appMsgf( AMT_OK, *LocalizeUnrealEd("Error_MorphImport_AlreadyExists") );
				break;
			case MorphImport_CantLoadFile:
				appMsgf( AMT_OK, *LocalizeUnrealEd("Error_MorphImport_CantLoadFile"), *Filename );
				break;
			case MorphImport_InvalidMeshFormat:
				appMsgf( AMT_OK, *LocalizeUnrealEd("Error_MorphImport_InvalidMeshFormat") );
				break;
			case MorphImport_MismatchBaseMesh:
				appMsgf( AMT_OK, *LocalizeUnrealEd("Error_MorphImport_MismatchBaseMesh") );
				break;
			case MorphImport_ReimportBaseMesh:
				appMsgf( AMT_OK, *LocalizeUnrealEd("Error_MorphImport_ReimportBaseMesh") );
				break;
			case MorphImport_MissingMorphTarget:
				appMsgf( AMT_OK, *LocalizeUnrealEd("Error_MorphImport_MissingMorphTarget") );
				break;
			case MorphImport_InvalidLODIndex:
				appMsgf( AMT_OK, *LocalizeUnrealEd("Error_MorphImport_InvalidLODIndex") );
				break;
			}
		}        
	}

	// Refresh combo to show new number of targets in this MorphTargetSet.
	INT CurrentSelection = MorphSetCombo->GetSelection();
	UpdateMorphSetCombo();
	MorphSetCombo->SetSelection(CurrentSelection);

	// Refresh morph target list box to show any new anims.
	UpdateMorphTargetList();

	// Reselect current morph target. If none selected, pick the first one in the box (if not empty).
	if(SelectedMorphTarget)
	{
		SetSelectedMorphTarget(SelectedMorphTarget);
	}
	else
	{
		if(MorphTargetList->GetCount() > 0)
		{
			SetSelectedMorphTarget( (UMorphTarget*)(MorphTargetList->GetClientData(0)) );
		}
		else
		{
			SetSelectedMorphTarget(NULL);
		}
	}

	// Mark as dirty.
	SelectedMorphSet->MarkPackageDirty();
}

/** rename the currently selected morph target */
void WxAnimSetViewer::RenameSelectedMorphTarget()
{
	if( SelectedMorphTarget )
	{
		check( SelectedMorphSet );
		check( SelectedMorphTarget->GetOuter() == SelectedMorphSet );
		check( SelectedMorphSet->Targets.ContainsItem(SelectedMorphTarget) );

		WxDlgGenericStringEntry dlg;
		const INT Result = dlg.ShowModal( TEXT("RenameMorph"), TEXT("NewMorphTargetName"), *SelectedMorphTarget->GetName() );
		if( Result == wxID_OK )
		{
			const FString NewItemString = dlg.GetEnteredString();
			const FName NewItemName =  FName( *NewItemString );

			if( SelectedMorphSet->FindMorphTarget( NewItemName ) )
			{
				// if we are trying to rename to an already existing name then prompt
				appMsgf( AMT_OK, *LocalizeUnrealEd("Prompt_28"), *NewItemName.ToString() );
			}
			else
			{
				// rename the object
				SelectedMorphTarget->Rename( *NewItemName.ToString() );
				// refresh the list
				UpdateMorphTargetList();
				// reselect the morph target
				SetSelectedMorphTarget( SelectedMorphTarget );
				// mark as dirty so a resave is required
				SelectedMorphSet->MarkPackageDirty();
			}
		}
	}
}

/** delete the currently selected morph target */
void WxAnimSetViewer::DeleteSelectedMorphTarget()
{
	if( SelectedMorphTarget )
	{
		check( SelectedMorphSet );
		check( SelectedMorphTarget->GetOuter() == SelectedMorphSet );
		check( SelectedMorphSet->Targets.ContainsItem(SelectedMorphTarget) );

		// show confirmation message dialog
		UBOOL bDoDelete = appMsgf( AMT_YesNo, *FString::Printf( *LocalizeUnrealEd("Prompt_27"), *SelectedMorphTarget->GetName() ) );
		if( bDoDelete )
		{
			SelectedMorphSet->Targets.RemoveItem( SelectedMorphTarget );

			// Refresh list
			UpdateMorphTargetList();

			// Select the first item (if present) - or none at all.
			if( MorphTargetList->GetCount() > 0 )
			{
				SetSelectedMorphTarget( (UMorphTarget*)(MorphTargetList->GetClientData(0)) );
			}
			else
			{
				SelectedMorphTarget = NULL;
				SetSelectedAnimSequence( NULL );
			}
		}

		// mark as dirty
		SelectedMorphSet->MarkPackageDirty();
	}
}


void WxAnimSetViewer::CreateNewAnimSet()
{
	FString Package = TEXT("");
	FString Group = TEXT("");

	if(SelectedSkelMesh)
	{
		// Bit yucky this...
		check( SelectedSkelMesh->GetOuter() );

		// If there are 2 levels above this mesh - top is packages, then group
		if( SelectedSkelMesh->GetOuter()->GetOuter() )
		{
			Group = SelectedSkelMesh->GetOuter()->GetFullGroupName(0);
			Package = SelectedSkelMesh->GetOuter()->GetOuter()->GetName();
		}
		else // Otherwise, just a package.
		{
			Group = TEXT("");
			Package = SelectedSkelMesh->GetOuter()->GetName();
		}
	}

	WxDlgRename RenameDialog;
	RenameDialog.SetTitle( *LocalizeUnrealEd("NewAnimSet") );
	if( RenameDialog.ShowModal( Package, Group, TEXT("NewAnimSet") ) == wxID_OK )
	{
		if( RenameDialog.GetNewName().Len() == 0 || RenameDialog.GetNewPackage().Len() == 0 )
		{
			appMsgf( AMT_OK, *LocalizeUnrealEd("Error_MustSpecifyNewAnimSetName") );
			return;
		}

		UPackage* Pkg = GEngine->CreatePackage(NULL,*RenameDialog.GetNewPackage());
		if( RenameDialog.GetNewGroup().Len() )
		{
			Pkg = GEngine->CreatePackage(Pkg,*RenameDialog.GetNewGroup());
		}

		UAnimSet* NewAnimSet = ConstructObject<UAnimSet>( UAnimSet::StaticClass(), Pkg, FName( *RenameDialog.GetNewName() ), RF_Public|RF_Standalone );

		// Will update AnimSet list, which will include new AnimSet.
		SetSelectedSkelMesh(SelectedSkelMesh, true);
		SetSelectedAnimSet(NewAnimSet, false);

		SelectedAnimSet->MarkPackageDirty();
	}
}

IMPLEMENT_COMPARE_POINTER( UAnimSequence, AnimSetViewer, { return appStricmp(*A->SequenceName.ToString(),*B->SequenceName.ToString()); } );

void WxAnimSetViewer::UpdateAnimSeqList()
{
	AnimSeqList->Freeze();
	AnimSeqList->Clear();

	if(!SelectedAnimSet)
		return;

	Sort<USE_COMPARE_POINTER(UAnimSequence,AnimSetViewer)>(&SelectedAnimSet->Sequences(0),SelectedAnimSet->Sequences.Num());

	for(INT i=0; i<SelectedAnimSet->Sequences.Num(); i++)
	{
		UAnimSequence* Seq = SelectedAnimSet->Sequences(i);

		FString SeqString = FString::Printf( TEXT("%s [%d]"), *Seq->SequenceName.ToString(), Seq->NumFrames );
		AnimSeqList->Append( *SeqString, Seq );
	}
	AnimSeqList->Thaw();
}

// Note - this will clear the current selection in the combo- must manually select an AnimSet afterwards.
void WxAnimSetViewer::UpdateAnimSetCombo()
{
	AnimSetCombo->Freeze();
	AnimSetCombo->Clear();

	for(TObjectIterator<UAnimSet> It; It; ++It)
	{
		UAnimSet* ItAnimSet = *It;
		
		if( ItAnimSet->CanPlayOnSkeletalMesh(SelectedSkelMesh) )
		{
			FString AnimSetString = FString::Printf( TEXT("%s [%d]"), *ItAnimSet->GetName(), ItAnimSet->Sequences.Num() );
			AnimSetCombo->Append( *AnimSetString, ItAnimSet );
		}
	}
	AnimSetCombo->Thaw();
}

IMPLEMENT_COMPARE_POINTER( UMorphTarget, AnimSetViewer, { return appStricmp(*A->GetName(), *B->GetName()); } );


/** Update the list of MorphTargets in the list box. */
void WxAnimSetViewer::UpdateMorphTargetList()
{
	MorphTargetList->Freeze();
	MorphTargetList->Clear();

	if(!SelectedMorphSet)
		return;

	Sort<USE_COMPARE_POINTER(UMorphTarget,AnimSetViewer)>(&SelectedMorphSet->Targets(0),SelectedMorphSet->Targets.Num());

	for(INT i=0; i<SelectedMorphSet->Targets.Num(); i++)
	{
		UMorphTarget* Target = SelectedMorphSet->Targets(i);

		FString TargetString = FString::Printf( TEXT("%s"), *Target->GetName() );
		MorphTargetList->Append( *TargetString, Target );
	}
	MorphTargetList->Thaw();
}

/**
 *	Update the list of MorphTargetSets in the combo box.
 *	Note - this will clear the current selection in the combo- must manually select an AnimSet afterwards.
 */
void WxAnimSetViewer::UpdateMorphSetCombo()
{
	MorphSetCombo->Freeze();
	MorphSetCombo->Clear();

	// Add 'none' entry, for not using any morph target. This guarantees there is always an element zero in the MorphSetCombo.
	MorphSetCombo->Append( *LocalizeUnrealEd("-None-"), (void*)NULL );

	for(TObjectIterator<UMorphTargetSet> It; It; ++It)
	{
		UMorphTargetSet* ItMorphSet = *It;

		FString MorphSetString = FString::Printf( TEXT("%s [%d]"), *ItMorphSet->GetName(), ItMorphSet->Targets.Num() );
		MorphSetCombo->Append( *MorphSetString, ItMorphSet );
	}
	MorphSetCombo->Thaw();
}


// Update the UI to match the current animation state (position, playing, looping)
void WxAnimSetViewer::RefreshPlaybackUI()
{
	// Update scrub bar (can only do if we have an animation selected.
	if(SelectedAnimSeq)
	{
		check(PreviewAnimNode->AnimSeq == SelectedAnimSeq);

		FLOAT CurrentPos = PreviewAnimNode->CurrentTime / SelectedAnimSeq->SequenceLength;

		TimeSlider->SetValue( appRound(CurrentPos * (FLOAT)ASV_SCRUBRANGE) );
	}


	// Update Play/Stop button
	if(PreviewAnimNode->bPlaying)
	{
		// Only set the bitmap label if we actually are changing state.
		const wxBitmap& CurrentBitmap = PlayButton->GetBitmapLabel();

		if(CurrentBitmap != StopB)
		{
			PlayButton->SetBitmapLabel( StopB );
			PlayButton->Refresh();
		}
	}
	else
	{
		// Only set the bitmap label if we actually are changing state.
		const wxBitmap& CurrentBitmap = PlayButton->GetBitmapLabel();

		if(CurrentBitmap != PlayB)
		{
			PlayButton->SetBitmapLabel( PlayB );
			PlayButton->Refresh();
		}
	}



	// Update Loop toggle
	if(PreviewAnimNode->bLooping)
	{
		// Only set the bitmap label if we actually are changing state.
		const wxBitmap& CurrentBitmap = LoopButton->GetBitmapLabel();

		if(CurrentBitmap != LoopB)
		{
			LoopButton->SetBitmapLabel( LoopB );
			LoopButton->Refresh();
		}
	}
	else
	{
		// Only set the bitmap label if we actually are changing state.
		const wxBitmap& CurrentBitmap = LoopButton->GetBitmapLabel();

		if(CurrentBitmap != NoLoopB)
		{
			LoopButton->SetBitmapLabel( NoLoopB );
			LoopButton->Refresh();
		}
	}

	
}

void WxAnimSetViewer::TickViewer(FLOAT DeltaSeconds)
{
	// Tick the PreviewSkelComp to move animation forwards, then Update to update bone locations.
	PreviewSkelComp->TickAnimNodes(DeltaSeconds);
	PreviewSkelComp->UpdateSkelPose();

	// Apply wind forces.
	PreviewSkelComp->UpdateClothWindForces(DeltaSeconds);

	// Run physics
	FLOAT MaxSubstep = NxTIMESTEP;
	FLOAT MaxDeltaTime = NxMAXDELTATIME;
	FLOAT PhysDT = ::Min(DeltaSeconds, MaxDeltaTime);
	TickRBPhysScene(RBPhysScene, PhysDT, MaxSubstep, FALSE);
	WaitRBPhysScene(RBPhysScene);

	// Update components
	PreviewSkelComp->ConditionalUpdateTransform(FMatrix::Identity);
	PreviewSkelCompAux1->ConditionalUpdateTransform(FMatrix::Identity);
	PreviewSkelCompAux2->ConditionalUpdateTransform(FMatrix::Identity);
	PreviewSkelCompAux3->ConditionalUpdateTransform(FMatrix::Identity);

	// Move scrubber to reflect current animation position etc.
	RefreshPlaybackUI();
}

void WxAnimSetViewer::UpdateSkelComponents()
{
	FComponentReattachContext ReattachContext1(PreviewSkelComp);
	FComponentReattachContext ReattachContext2(PreviewSkelCompAux1);
	FComponentReattachContext ReattachContext3(PreviewSkelCompAux2);
	FComponentReattachContext ReattachContext4(PreviewSkelCompAux3);
}

void WxAnimSetViewer::UpdateForceLODButtons()
{
	if(SelectedSkelMesh)
	{
		ToolBar->EnableTool( IDM_ANIMSET_LOD_1, SelectedSkelMesh->LODModels.Num() > 1 );
		ToolBar->EnableTool( IDM_ANIMSET_LOD_2, SelectedSkelMesh->LODModels.Num() > 2 );
		ToolBar->EnableTool( IDM_ANIMSET_LOD_3, SelectedSkelMesh->LODModels.Num() > 3 );
	}
}

void WxAnimSetViewer::EmptySelectedSet()
{
	if(SelectedAnimSet)
	{
		UBOOL bDoEmpty = appMsgf( AMT_YesNo, *LocalizeUnrealEd("Prompt_1") );
		if( bDoEmpty )
		{
			SelectedAnimSet->ResetAnimSet();
			UpdateAnimSeqList();
			SetSelectedAnimSequence(NULL);

			SelectedAnimSet->MarkPackageDirty();
		}
	}
}

void WxAnimSetViewer::DeleteTrackFromSelectedSet()
{
	if( SelectedAnimSet )
	{
		// Make a list of all tracks in the animset.
		TArray<FString> Options;
		for ( INT TrackIndex = 0 ; TrackIndex < SelectedAnimSet->TrackBoneNames.Num() ; ++TrackIndex )
		{
			const FName& TrackName = SelectedAnimSet->TrackBoneNames(TrackIndex);
			Options.AddItem(TrackName.ToString());
		}

		// Present the user with a list of potential tracks to delete.
		WxDlgGenericComboEntry dlg;
		const INT Result = dlg.ShowModal( TEXT("DeleteTrack"), TEXT("DeleteTrack"), Options, 0, TRUE );

		if ( Result == wxID_OK )
		{
			const FName SelectedTrackName( *dlg.GetSelectedString() );

			const INT SelectedTrackIndex = SelectedAnimSet->TrackBoneNames.FindItemIndex( SelectedTrackName );
			if ( SelectedTrackIndex != INDEX_NONE )
			{
				UAnimNodeMirror* Pre = PreviewAnimMirror;
				// Eliminate the selected track from all sequences in the set.
				for( INT SequenceIndex = 0 ; SequenceIndex < SelectedAnimSet->Sequences.Num() ; ++SequenceIndex )
				{
					UAnimSequence* Seq = SelectedAnimSet->Sequences( SequenceIndex );
					check( Seq->RawAnimData.Num() == SelectedAnimSet->TrackBoneNames.Num() );
					Seq->RawAnimData.Remove( SelectedTrackIndex );
				}
				UAnimNodeMirror* Post = PreviewAnimMirror;

				// Element the track bone name.
				SelectedAnimSet->TrackBoneNames.Remove( SelectedTrackIndex );

				// Flush the linkup cache.
				SelectedAnimSet->LinkupCache.Empty();

				// Flag the animset for resaving.
				SelectedAnimSet->MarkPackageDirty();

				// Reinit anim trees so that eg new linkup caches are created.
				for(TObjectIterator<USkeletalMeshComponent> It;It;++It)
				{
					USkeletalMeshComponent* SkelComp = *It;
					if(!SkelComp->IsPendingKill())
					{
						SkelComp->InitAnimTree();
					}
				}

				// Recompress the anim set if compression was applied.
				RecompressSet( SelectedAnimSet, SelectedSkelMesh );
			}
		}
	}
}

void WxAnimSetViewer::RenameSelectedSeq()
{
	if(SelectedAnimSeq)
	{
		check(SelectedAnimSet);

		WxDlgGenericStringEntry dlg;
		const INT Result = dlg.ShowModal( TEXT("RenameAnimSequence"), TEXT("NewSequenceName"), *SelectedAnimSeq->SequenceName.ToString() );
		if( Result == wxID_OK )
		{
			const FString NewSeqString = dlg.GetEnteredString();
			const FName NewSeqName =  FName( *NewSeqString );

			// If there is a sequence with that name already, see if we want to over-write it.
			UAnimSequence* FoundSeq = SelectedAnimSet->FindAnimSequence(NewSeqName);
			if(FoundSeq)
			{
				const FString ConfirmMessage = FString::Printf( *LocalizeUnrealEd("Prompt_2"), *NewSeqName.ToString() );
				const UBOOL bDoDelete = appMsgf( AMT_YesNo, *ConfirmMessage );
				if(!bDoDelete)
					return;

				SelectedAnimSet->Sequences.RemoveItem(FoundSeq);
			}

			SelectedAnimSeq->SequenceName = NewSeqName;

			UpdateAnimSeqList();

			// This will reselect this sequence, update the AnimNodeSequence, status bar etc.
			SetSelectedAnimSequence(SelectedAnimSeq);


			SelectedAnimSet->MarkPackageDirty();
		}
	}
}

void WxAnimSetViewer::DeleteSelectedSequence()
{
	if(SelectedAnimSeq)
	{
		check( SelectedAnimSeq->GetOuter() == SelectedAnimSet );
		check( SelectedAnimSet->Sequences.ContainsItem(SelectedAnimSeq) );

		FString ConfirmMessage = FString::Printf( *LocalizeUnrealEd("Prompt_3"), *SelectedAnimSeq->SequenceName.ToString() );

		UBOOL bDoDelete = appMsgf( AMT_YesNo, *ConfirmMessage );
		if(bDoDelete)
		{
			SelectedAnimSet->Sequences.RemoveItem( SelectedAnimSeq );

			// Refresh list
			UpdateAnimSeqList();

			// Select the first sequence (if present) - or none at all.
			if( AnimSeqList->GetCount() > 0 )
				SetSelectedAnimSequence( (UAnimSequence*)(AnimSeqList->GetClientData(0)) );
			else
				SetSelectedAnimSequence( NULL );
		}

		SelectedAnimSet->MarkPackageDirty();
	}
}

/**
 * Pop up a dialog asking for axis and angle (in debgrees), and apply that rotation to all keys in selected sequence.
 * Basically 
 */
void WxAnimSetViewer::SequenceApplyRotation()
{
	if(SelectedAnimSeq)
	{
		WxDlgRotateAnimSeq dlg;
		INT Result = dlg.ShowModal();
		if( Result == wxID_OK )
		{
			// Angle (in radians) to rotate AnimSequence by
			FLOAT RotAng = dlg.Degrees * (PI/180.f);

			// Axis to rotate AnimSequence about.
			FVector RotAxis;
			if( dlg.Axis == AXIS_X )
			{
				RotAxis = FVector(1.f, 0.f, 0.f);
			}
			else if( dlg.Axis == AXIS_Y )
			{
				RotAxis = FVector(0.f, 1.f, 0.0f);
			}
			else if( dlg.Axis == AXIS_Z )
			{
				RotAxis = FVector(0.f, 0.f, 1.0f);
			}
			else
			{
				check(0);
			}

			// Make transformation matrix out of rotation (via quaternion)
			const FQuat RotQuat( RotAxis, RotAng );

			// Hmm.. animations don't have any idea of heirarchy, so we just rotate the first track. Can we be sure track 0 is the root bone?
			FRawAnimSequenceTrack& RawTrack = SelectedAnimSeq->RawAnimData(0);
			for( INT r = 0 ; r < RawTrack.RotKeys.Num() ; ++r )
			{
				FQuat& Quat = RawTrack.RotKeys(r);
				Quat = RotQuat * Quat;
				Quat.Normalize();
			}
			for( INT p = 0 ; p < RawTrack.PosKeys.Num() ; ++p)
			{
				RawTrack.PosKeys(p) = RotQuat.RotateVector(RawTrack.PosKeys(p));
			}

			RecompressSequence(SelectedAnimSeq, SelectedSkelMesh);

			SelectedAnimSeq->MarkPackageDirty();
		}
	}
}

void WxAnimSetViewer::SequenceReZeroToCurrent()
{
	if(SelectedAnimSeq)
	{
		// Find vector that would translate current root bone location onto origin.
		FVector ApplyTranslation = -1.f * PreviewSkelComp->SpaceBases(0).GetOrigin();

		// Convert into world space and eliminate 'z' translation. Don't want to move character into ground.
		FVector WorldApplyTranslation = PreviewSkelComp->LocalToWorld.TransformNormal(ApplyTranslation);
		WorldApplyTranslation.Z = 0.f;
		ApplyTranslation = PreviewSkelComp->LocalToWorld.InverseTransformNormal(WorldApplyTranslation);

		// As above, animations don't have any idea of heirarchy, so we don't know for sure if track 0 is the root bone's track.
		FRawAnimSequenceTrack& RawTrack = SelectedAnimSeq->RawAnimData(0);
		for(INT i=0; i<RawTrack.PosKeys.Num(); i++)
		{
			RawTrack.PosKeys(i) += ApplyTranslation;
		}

		RecompressSequence(SelectedAnimSeq, SelectedSkelMesh);
		SelectedAnimSeq->MarkPackageDirty();
	}
}

/**
 * Crop a sequence either before or after the current position. This is made slightly more complicated due to the basic compression
 * we do where tracks which had all identical keyframes are reduced to just 1 frame.
 * 
 * @param bFromStart Should we remove the sequence before or after the selected position.
 */
void WxAnimSetViewer::SequenceCrop(UBOOL bFromStart)
{
	if(SelectedAnimSeq)
	{
		// Crop the raw anim data.
		SelectedAnimSeq->CropRawAnimData( PreviewAnimNode->CurrentTime, bFromStart );

		if(bFromStart)
		{
			PreviewAnimNode->CurrentTime = 0.f;
		}
		else
		{		
			PreviewAnimNode->CurrentTime = SelectedAnimSeq->SequenceLength;
		}

		UpdateAnimSeqList();
		SetSelectedAnimSequence(SelectedAnimSeq);
	}
}

/** Tool for updating the bounding sphere/box of the selected skeletal mesh. Shouldn't generally be needed, except for fixing stuff up possibly. */
void WxAnimSetViewer::UpdateMeshBounds()
{
	if(SelectedSkelMesh)
	{
		FBox BoundingBox(0);
		check(SelectedSkelMesh->LODModels.Num() > 0);
		FStaticLODModel& LODModel = SelectedSkelMesh->LODModels(0);

		for(INT ChunkIndex = 0;ChunkIndex < LODModel.Chunks.Num();ChunkIndex++)
		{
			const FSkelMeshChunk& Chunk = LODModel.Chunks(ChunkIndex);
			for(INT i=0; i<Chunk.RigidVertices.Num(); i++)
			{
				BoundingBox += Chunk.RigidVertices(i).Position;
			}
			for(INT i=0; i<Chunk.SoftVertices.Num(); i++)
			{
				BoundingBox += Chunk.SoftVertices(i).Position;
			}
		}

		const FBox Temp			= BoundingBox;
		const FVector MidMesh	= 0.5f*(Temp.Min + Temp.Max);
		BoundingBox.Min			= Temp.Min + 1.0f*(Temp.Min - MidMesh);
		BoundingBox.Max			= Temp.Max + 1.0f*(Temp.Max - MidMesh);

		// Tuck up the bottom as this rarely extends lower than a reference pose's (e.g. having its feet on the floor).
		BoundingBox.Min.Z	= Temp.Min.Z + 0.1f*(Temp.Min.Z - MidMesh.Z);
		SelectedSkelMesh->Bounds = FBoxSphereBounds(BoundingBox);

		SelectedSkelMesh->MarkPackageDirty();
	}
}

/** Update the external force on the cloth sim. */
void WxAnimSetViewer::UpdateClothWind()
{
	const FVector WindDir = WindRot.Vector();
	PreviewSkelComp->ClothWind = WindDir * WindStrength;
}
