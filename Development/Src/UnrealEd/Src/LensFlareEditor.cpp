/*=============================================================================
	LensFlareEditor.cpp: LensFlare editor
	Copyright © 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"

#include "CurveEd.h"
//#include "EngineLensFlareClasses.h"
#include "LensFlare.h"
#include "EngineMaterialClasses.h"
#include "Properties.h"
#include "LensFlareEditor.h"

//IMPLEMENT_CLASS(ULensFlareEditorElementStub);
IMPLEMENT_CLASS(ULensFlareEditorOptions);
IMPLEMENT_CLASS(ULensFlarePreviewComponent);

/*-----------------------------------------------------------------------------
	FLensFlareEditorNotifyHook
-----------------------------------------------------------------------------*/
void FLensFlareEditorNotifyHook::NotifyDestroy(void* Src)
{
	if (WindowOfInterest == Src)
	{
	}
}

/*-----------------------------------------------------------------------------
	WxLensFlareEditor
-----------------------------------------------------------------------------*/
/**
 * Versioning Info for the Docking Parent layout file.
 */
namespace
{
	static const TCHAR*	DockingParent_Name = TEXT("LensFlareEditor");
	// Needs to be incremented every time a new dock window is added or removed from this docking parent.
	static const INT	DockingParent_Version = 0;
}


// Undo/Redo support
bool WxLensFlareEditor::BeginTransaction(const TCHAR* pcTransaction)
{
	if (TransactionInProgress())
	{
		FString kError(*LocalizeUnrealEd("Error_FailedToBeginTransaction"));
		kError += kTransactionName;
		check(!*kError);
		return FALSE;
	}

	GEditor->Trans->Begin(pcTransaction);
	kTransactionName = FString(pcTransaction);
	bTransactionInProgress = TRUE;

	ModifyLensFlare();

	return TRUE;
}

bool WxLensFlareEditor::EndTransaction(const TCHAR* pcTransaction)
{
	if (!TransactionInProgress())
	{
		FString kError(*LocalizeUnrealEd("Error_FailedToEndTransaction"));
		kError += kTransactionName;
		check(!*kError);
		return FALSE;
	}

	if (appStrcmp(*kTransactionName, pcTransaction) != 0)
	{
		return FALSE;
	}

	GEditor->Trans->End();

	kTransactionName = TEXT("");
	bTransactionInProgress = FALSE;

	return TRUE;
}

bool WxLensFlareEditor::TransactionInProgress()
{
	return bTransactionInProgress;
}

void WxLensFlareEditor::ModifyLensFlare()
{
	if (LensFlare)
	{
		LensFlare->Modify();
	}

	if (LensFlareComp)
	{
		LensFlareComp->Modify();
	}
}

void WxLensFlareEditor::LensFlareEditorUndo()
{
	GEditor->Trans->Undo();
	LensFlareEditorTouch();
}

void WxLensFlareEditor::LensFlareEditorRedo()
{
	GEditor->Trans->Redo();
	LensFlareEditorTouch();
}

void WxLensFlareEditor::LensFlareEditorTouch()
{
	// 'Refresh' the viewport
	ElementEdVC->Viewport->Invalidate();
}

void WxLensFlareEditor::ResetLensFlareInLevel()
{
	FlushRenderingCommands();
	// Reset all the templates on actors in the level
	for (TObjectIterator<ULensFlareComponent> It;It;++It)
	{
		if (It->Template == LensFlareComp->Template)
		{
			ULensFlareComponent* LFComp = *It;

			if (LFComp->Template == LensFlare)
			{
				LFComp->BeginDeferredReattach();
			}
			else
			{
				LFComp->SetTemplate(LensFlare);
			}
		}
	}
}

// PostProces
/**
 *	Update the post process chain according to the show options
 */
void WxLensFlareEditor::UpdatePostProcessChain()
{
/***
	if (DefaultPostProcess && PreviewVC)
	{
		UPostProcessEffect* BloomEffect = NULL;
		UPostProcessEffect* DOFEffect = NULL;
		UPostProcessEffect* MotionBlurEffect = NULL;
		UPostProcessEffect* PPVolumeEffect = NULL;

		for (INT EffectIndex = 0; EffectIndex < DefaultPostProcess->Effects.Num(); EffectIndex++)
		{
			UPostProcessEffect* Effect = DefaultPostProcess->Effects(EffectIndex);
			if (Effect)
			{
				if (Effect->EffectName.ToString() == FString(TEXT("LensFlareEditorDOFAndBloom")))
				{
					BloomEffect = Effect;
					DOFEffect = Effect;
				}
				else
				if (Effect->EffectName.ToString() == FString(TEXT("LensFlareEditorMotionBlur")))
				{
					MotionBlurEffect = Effect;
				}
				else
				if (Effect->EffectName.ToString() == FString(TEXT("LensFlareEditorPPVolumeMaterial")))
				{
					PPVolumeEffect = Effect;
				}
			}
		}

		if (BloomEffect)
		{
			if (PreviewVC->ShowPPFlags & CASC_SHOW_BLOOM)
			{
				BloomEffect->bShowInEditor = TRUE;
				BloomEffect->bShowInGame = TRUE;
			}
			else
			{
				BloomEffect->bShowInEditor = FALSE;
				BloomEffect->bShowInGame = FALSE;
			}
		}
		if (DOFEffect)
		{
			if (PreviewVC->ShowPPFlags & CASC_SHOW_DOF)
			{
				DOFEffect->bShowInEditor = TRUE;
				DOFEffect->bShowInGame = TRUE;
			}
			else
			{
				DOFEffect->bShowInEditor = FALSE;
				DOFEffect->bShowInGame = FALSE;
			}
		}
		if (MotionBlurEffect)
		{
			if (PreviewVC->ShowPPFlags & CASC_SHOW_MOTIONBLUR)
			{
				MotionBlurEffect->bShowInEditor = TRUE;
				MotionBlurEffect->bShowInGame = TRUE;
			}
			else
			{
				MotionBlurEffect->bShowInEditor = FALSE;
				MotionBlurEffect->bShowInGame = FALSE;
			}
		}
		if (PPVolumeEffect)
		{
			if (PreviewVC->ShowPPFlags & CASC_SHOW_PPVOLUME)
			{
				PPVolumeEffect->bShowInEditor = TRUE;
				PPVolumeEffect->bShowInGame = TRUE;
			}
			else
			{
				PPVolumeEffect->bShowInEditor = FALSE;
				PPVolumeEffect->bShowInGame = FALSE;
			}
		}
	}
	else
	{
		warnf(TEXT("LensFlareEditor::UpdatePostProcessChain> Nno post process chain."));
	}
***/
}

BEGIN_EVENT_TABLE(WxLensFlareEditor, wxFrame)
	EVT_SIZE(WxLensFlareEditor::OnSize)
//	EVT_MENU( IDM_LENSFLAREEDITOR_ENABLE_ELEMENT, WxLensFlareEditor::OnEnableElement )
//	EVT_MENU( IDM_LENSFLAREEDITOR_DISABLE_ELEMENT, WxLensFlareEditor::OnDisableElement )
	EVT_MENU( IDM_LENSFLAREEDITOR_CURVE_MATERIALINDEX, WxLensFlareEditor::OnAddCurve )
	EVT_MENU( IDM_LENSFLAREEDITOR_CURVE_SCALING, WxLensFlareEditor::OnAddCurve )
	EVT_MENU( IDM_LENSFLAREEDITOR_CURVE_AXISSCALING, WxLensFlareEditor::OnAddCurve )
	EVT_MENU( IDM_LENSFLAREEDITOR_CURVE_ROTATION, WxLensFlareEditor::OnAddCurve )
	EVT_MENU( IDM_LENSFLAREEDITOR_CURVE_COLOR, WxLensFlareEditor::OnAddCurve )
	EVT_MENU( IDM_LENSFLAREEDITOR_CURVE_ALPHA, WxLensFlareEditor::OnAddCurve )
	EVT_MENU( IDM_LENSFLAREEDITOR_CURVE_OFFSET, WxLensFlareEditor::OnAddCurve )
	EVT_MENU( IDM_LENSFLAREEDITOR_ELEMENT_ADD, WxLensFlareEditor::OnElementAdd )
	EVT_MENU( IDM_LENSFLAREEDITOR_ELEMENT_ADD_BEFORE, WxLensFlareEditor::OnElementAddBefore )
	EVT_MENU( IDM_LENSFLAREEDITOR_ELEMENT_ADD_AFTER, WxLensFlareEditor::OnElementAddAfter )
	EVT_MENU( IDM_LENSFLAREEDITOR_RESET_ELEMENT, WxLensFlareEditor::OnResetElement )
	EVT_MENU( IDM_LENSFLAREEDITOR_DELETE_ELEMENT, WxLensFlareEditor::OnDeleteElement )
	EVT_MENU( IDM_LENSFLAREEDITOR_RESETINLEVEL, WxLensFlareEditor::OnResetInLevel )
	EVT_MENU( IDM_LENSFLAREEDITOR_SAVECAM, WxLensFlareEditor::OnSaveCam )
	EVT_TOOL( IDM_LENSFLAREEDITOR_ORBITMODE, WxLensFlareEditor::OnOrbitMode )
	EVT_TOOL(IDM_LENSFLAREEDITOR_WIREFRAME, WxLensFlareEditor::OnWireframe)
	EVT_TOOL(IDM_LENSFLAREEDITOR_BOUNDS, WxLensFlareEditor::OnBounds)
	EVT_TOOL(IDM_LENSFLAREEDITOR_POSTPROCESS, WxLensFlareEditor::OnPostProcess)
	EVT_TOOL(IDM_LENSFLAREEDITOR_TOGGLEGRID, WxLensFlareEditor::OnToggleGrid)
	EVT_TOOL(IDM_LENSFLAREEDITOR_REALTIME, WxLensFlareEditor::OnRealtime)
	EVT_TOOL(IDM_LENSFLAREEDITOR_BACKGROUND_COLOR, WxLensFlareEditor::OnBackgroundColor)
	EVT_TOOL(IDM_LENSFLAREEDITOR_UNDO, WxLensFlareEditor::OnUndo)
	EVT_TOOL(IDM_LENSFLAREEDITOR_REDO, WxLensFlareEditor::OnRedo)
	EVT_MENU( IDM_LENSFLAREEDITOR_VIEW_AXES, WxLensFlareEditor::OnViewAxes )
	EVT_MENU(IDM_LENSFLAREEDITOR_SAVE_PACKAGE, WxLensFlareEditor::OnSavePackage)
	EVT_MENU(IDM_LENSFLAREEDITOR_SHOWPP_BLOOM, WxLensFlareEditor::OnShowPPBloom)
	EVT_MENU(IDM_LENSFLAREEDITOR_SHOWPP_DOF, WxLensFlareEditor::OnShowPPDOF)
	EVT_MENU(IDM_LENSFLAREEDITOR_SHOWPP_MOTIONBLUR, WxLensFlareEditor::OnShowPPMotionBlur)
	EVT_MENU(IDM_LENSFLAREEDITOR_SHOWPP_PPVOLUME, WxLensFlareEditor::OnShowPPVolumeMaterial)
END_EVENT_TABLE()


#define LENSFLAREEDITOR_NUM_SASHES		4

WxLensFlareEditor::WxLensFlareEditor(wxWindow* InParent, wxWindowID InID, ULensFlare* InLensFlare) : 
	wxFrame(InParent, InID, TEXT(""), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT | wxFRAME_NO_TASKBAR, 
		InLensFlare ? *(InLensFlare->GetPathName()) : TEXT("EMPTY")),
	FDockingParent(this),
	MenuBar(NULL),
	ToolBar(NULL),
	PreviewVC(NULL)
//	, SelectedElementPropertyStub(NULL)
{
	DefaultPostProcessName = TEXT("");
    DefaultPostProcess = NULL;

	EditorOptions = ConstructObject<ULensFlareEditorOptions>(ULensFlareEditorOptions::StaticClass());
	check(EditorOptions);

//	SelectedElementPropertyStub = ConstructObject<ULensFlareEditorElementStub>(ULensFlareEditorElementStub::StaticClass());
//	check(SelectedElementPropertyStub);

	// Load the desired window position from .ini file
	FWindowUtil::LoadPosSize(TEXT("LensFlareEditorEditor"), this, 256, 256, 1024, 768);
	
	// Set up pointers to interp objects
	LensFlare = InLensFlare;

	// Set up for undo/redo!
	LensFlare->SetFlags(RF_Transactional);

	// Nothing selected initially
	SelectedElementIndex = -2;

	CurveToReplace = NULL;

	bOrbitMode = TRUE;
	bWireframe = FALSE;
	bBounds = FALSE;

	bTransactionInProgress = FALSE;

	PropertyWindow = new WxPropertyWindow;
	PropertyWindow->Create(this, this);

	// Create particle system preview
	WxLensFlareEditorPreview* PreviewWindow = new WxLensFlareEditorPreview( this, -1, this );
	PreviewVC = PreviewWindow->LensFlareEditorPreviewVC;
//	PreviewVC->SetPreviewCamera(LensFlare->ThumbnailAngle, LensFlare->ThumbnailDistance);
	PreviewVC->SetViewLocationForOrbiting( FVector(0.f,0.f,0.f) );
	if (EditorOptions->bShowGrid == TRUE)
	{
		PreviewVC->ShowFlags |= SHOW_Grid;
	}
	else
	{
		PreviewVC->ShowFlags &= ~SHOW_Grid;
	}

	UpdatePostProcessChain();

	// Create new curve editor setup if not already done
	if (!LensFlare->CurveEdSetup)
	{
		LensFlare->CurveEdSetup = ConstructObject<UInterpCurveEdSetup>( 
			UInterpCurveEdSetup::StaticClass(), LensFlare, NAME_None, 
			RF_NotForClient | RF_NotForServer | RF_Transactional );
	}

	// Create graph editor to work on systems CurveEd setup.
	CurveEd = new WxCurveEditor( this, -1, LensFlare->CurveEdSetup );
	// Register this window with the Curve editor so we will be notified of various things.
	CurveEd->SetNotifyObject(this);

	// Create emitter editor
	LensFlareElementEdWindow = new WxLensFlareElementEd(this, -1, this);
	ElementEdVC = LensFlareElementEdWindow->ElementEdVC;

	// Create Docking Windows
	{
		AddDockingWindow(PropertyWindow, FDockingParent::DH_Bottom, *FString::Printf( *LocalizeUnrealEd("PropertiesCaption_F"), *LensFlare->GetName() ), *LocalizeUnrealEd("Properties") );
		AddDockingWindow(CurveEd, FDockingParent::DH_Bottom, *FString::Printf( *LocalizeUnrealEd("CurveEditorCaption_F"), *LensFlare->GetName() ), *LocalizeUnrealEd("CurveEditor") );
		AddDockingWindow(PreviewWindow, FDockingParent::DH_Left, *FString::Printf( *LocalizeUnrealEd("PreviewCaption_F"), *LensFlare->GetName() ), *LocalizeUnrealEd("Preview") );
		
		SetDockHostSize(FDockingParent::DH_Left, 500);

		wxPane* PreviewPane = new wxPane( this );
		{
			PreviewPane->ShowHeader(false);
			PreviewPane->ShowCloseButton( false );
			PreviewPane->SetClient(LensFlareElementEdWindow);
		}
		LayoutManager->SetLayout( wxDWF_SPLITTER_BORDERS, PreviewPane );

		// Try to load a existing layout for the docking windows.
		LoadDockingLayout();
	}

	// Create menu bar
	MenuBar = new WxLensFlareEditorMenuBar(this);
	AppendDockingMenu(MenuBar);
	SetMenuBar( MenuBar );

	// Create tool bar
	ToolBar	= NULL;
	ToolBar = new WxLensFlareEditorToolBar( this, -1 );
	SetToolBar( ToolBar );

	// Set window title to particle system we are editing.
	SetTitle( *FString::Printf( *LocalizeUnrealEd("LensFlareEditorCaption_F"), *(LensFlare->GetName()) ) );

	// Set the lensflare component to use the LensFlare we are editing.
	LensFlareComp->SetTemplate(LensFlare);

	SetSelectedElement(-1);

	PreviewVC->BackgroundColor = EditorOptions->LFED_BackgroundColor;
/***
	// Setup the accelerator table
	TArray<wxAcceleratorEntry> Entries;
	// Allow derived classes an opportunity to register accelerator keys.
	// Bind SPACE to reset.
	Entries.AddItem(wxAcceleratorEntry(wxACCEL_NORMAL, WXK_SPACE, IDM_LENSFLAREEDITOR_RESETSYSTEM));
	// Create the new table with these.
	SetAcceleratorTable(wxAcceleratorTable(Entries.Num(),Entries.GetTypedData()));
***/
}

WxLensFlareEditor::~WxLensFlareEditor()
{
	GEditor->Trans->Reset(TEXT("QuitLensFlareEditor"));

	// Save the desired window position to the .ini file
	FWindowUtil::SavePosSize(TEXT("LensFlareEditorEditor"), this);
	
	SaveDockingLayout();

	// Destroy preview viewport before we destroy the level.
	GEngine->Client->CloseViewport(PreviewVC->Viewport);
	PreviewVC->Viewport = NULL;

	delete PreviewVC;
	PreviewVC = NULL;

	delete PropertyWindow;
}

wxToolBar* WxLensFlareEditor::OnCreateToolBar(long style, wxWindowID id, const wxString& name)
{
	if (name == TEXT("LensFlareEditor"))
	{
		return new WxLensFlareEditorToolBar(this, -1);
	}

	wxToolBar* ReturnToolBar = OnCreateToolBar(style, id, name);
	return ReturnToolBar;
}

void WxLensFlareEditor::Serialize(FArchive& Ar)
{
	PreviewVC->Serialize(Ar);

	Ar << EditorOptions;
//	Ar << SelectedElementPropertyStub;
}

/**
 * Pure virtual that must be overloaded by the inheriting class. It will
 * be called from within UnLevTick.cpp after ticking all actors.
 *
 * @param DeltaTime	Game time passed since the last call.
 */
void WxLensFlareEditor::Tick( FLOAT DeltaTime )
{
}

// FCurveEdNotifyInterface
/**
 *	PreEditCurve
 *	Called by the curve editor when N curves are about to change
 *
 *	@param	CurvesAboutToChange		An array of the curves about to change
 */
void WxLensFlareEditor::PreEditCurve(TArray<UObject*> CurvesAboutToChange)
{
	debugf(TEXT("LensFlareEditor: PreEditCurve - %2d curves"), CurvesAboutToChange.Num());

	BeginTransaction(*LocalizeUnrealEd("CurveEdit"));

	// Call Modify on all tracks with keys selected
	for (INT i = 0; i < CurvesAboutToChange.Num(); i++)
	{
		// If this keypoint is from a distribution, call Modify on it to back up its state.
		UDistributionFloat* DistFloat = Cast<UDistributionFloat>(CurvesAboutToChange(i));
		if (DistFloat)
		{
			DistFloat->Modify();
		}
		UDistributionVector* DistVector = Cast<UDistributionVector>(CurvesAboutToChange(i));
		if (DistVector)
		{
			DistVector->Modify();
		}
	}
}

/**
 *	PostEditCurve
 *	Called by the curve editor when the edit has completed.
 */
void WxLensFlareEditor::PostEditCurve()
{
	debugf(TEXT("LensFlareEditor: PostEditCurve"));
	EndTransaction(*LocalizeUnrealEd("CurveEdit"));
}

/**
 *	MovedKey
 *	Called by the curve editor when a key has been moved.
 */
void WxLensFlareEditor::MovedKey()
{
}

/**
 *	DesireUndo
 *	Called by the curve editor when an Undo is requested.
 */
void WxLensFlareEditor::DesireUndo()
{
	debugf(TEXT("LensFlareEditor: DesireUndo"));
	LensFlareEditorUndo();
}

/**
 *	DesireRedo
 *	Called by the curve editor when an Redo is requested.
 */
void WxLensFlareEditor::DesireRedo()
{
	debugf(TEXT("LensFlareEditor: DesireRedo"));
	LensFlareEditorRedo();
}

void WxLensFlareEditor::OnSize( wxSizeEvent& In )
{
	In.Skip();
	Refresh();
}

///////////////////////////////////////////////////////////////////////////////////////
// Menu Callbacks

void WxLensFlareEditor::OnDuplicateElement( wxCommandEvent& In )
{
/***
	// Make sure there is a selected emitter
	if (!SelectedEmitter)
		return;

	UBOOL bShare = FALSE;
	if (In.GetId() == IDM_LENSFLAREEDITOR_DUPLICATE_SHARE_EMITTER)
	{
		bShare = TRUE;
	}

	BeginTransaction(TEXT("EmitterDuplicate"));

	PartSys->PreEditChange(NULL);
	PartSysComp->PreEditChange(NULL);

	if (!DuplicateEmitter(SelectedEmitter, PartSys, bShare))
	{
	}
	PartSysComp->PostEditChange(NULL);
	PartSys->PostEditChange(NULL);

	EndTransaction(TEXT("EmitterDuplicate"));
***/
	// Refresh viewport
	ElementEdVC->Viewport->Invalidate();
}

void WxLensFlareEditor::OnDeleteElement(wxCommandEvent& In)
{
	DeleteSelectedElement();
}

void WxLensFlareEditor::OnAddCurve( wxCommandEvent& In )
{
	if (LensFlare == NULL)
	{
		return;
	}

	FString CurveName;
	switch (In.GetId())
	{
	case IDM_LENSFLAREEDITOR_CURVE_MATERIALINDEX:	CurveName = FString(TEXT("LFMaterialIndex"));		break;
	case IDM_LENSFLAREEDITOR_CURVE_SCALING:			CurveName = FString(TEXT("Scaling"));				break;
	case IDM_LENSFLAREEDITOR_CURVE_AXISSCALING:		CurveName = FString(TEXT("AxisScaling"));			break;
	case IDM_LENSFLAREEDITOR_CURVE_ROTATION:		CurveName = FString(TEXT("Rotation"));				break;
	case IDM_LENSFLAREEDITOR_CURVE_COLOR:			CurveName = FString(TEXT("Color"));					break;
	case IDM_LENSFLAREEDITOR_CURVE_ALPHA:			CurveName = FString(TEXT("Alpha"));					break;
	case IDM_LENSFLAREEDITOR_CURVE_OFFSET:			CurveName = FString(TEXT("Offset"));				break;
	}

	LensFlare->AddElementCurveToEditor(SelectedElementIndex, CurveName, LensFlare->CurveEdSetup);
	SetSelectedInCurveEditor();
	CurveEd->CurveEdVC->Viewport->Invalidate();
}

void WxLensFlareEditor::OnElementAdd( wxCommandEvent& In )
{
	BeginTransaction(TEXT("ElementAdd"));
	LensFlare->PreEditChange(NULL);
	LensFlareComp->PreEditChange(NULL);

	// Insert the value at the current index
	INT NewIndex = LensFlare->Reflections.AddZeroed();
	LensFlare->InitializeElement(NewIndex);

	LensFlareComp->PostEditChange(NULL);
	LensFlare->PostEditChange(NULL);
	LensFlare->MarkPackageDirty();

	EndTransaction(TEXT("ElementAdd"));

	SetSelectedElement(NewIndex);

	// Refresh viewport
	ElementEdVC->Viewport->Invalidate();
}

void WxLensFlareEditor::OnElementAddBefore( wxCommandEvent& In )
{
	if (SelectedElementIndex == -1)
	{
		appMsgf(AMT_OK, *LocalizeUnrealEd("Error_LensFlareEditor_CantAddBeforeSource"));
		return;
	}

	BeginTransaction(TEXT("ElementAddBefore"));
	LensFlare->PreEditChange(NULL);
	LensFlareComp->PreEditChange(NULL);

	// Insert the value at the current index
	LensFlare->Reflections.InsertZeroed(SelectedElementIndex);
	LensFlare->InitializeElement(SelectedElementIndex);

	LensFlareComp->PostEditChange(NULL);
	LensFlare->PostEditChange(NULL);
	LensFlare->MarkPackageDirty();

	SetSelectedElement(SelectedElementIndex);

	EndTransaction(TEXT("ElementAddBefore"));

	// Refresh viewport
	ElementEdVC->Viewport->Invalidate();
}

void WxLensFlareEditor::OnElementAddAfter( wxCommandEvent& In )
{
	BeginTransaction(TEXT("ElementAddAfter"));
	LensFlare->PreEditChange(NULL);
	LensFlareComp->PreEditChange(NULL);

	// Insert the value at the current index
	LensFlare->Reflections.InsertZeroed(SelectedElementIndex + 1);
	LensFlare->InitializeElement(SelectedElementIndex + 1);

	LensFlareComp->PostEditChange(NULL);
	LensFlare->PostEditChange(NULL);
	LensFlare->MarkPackageDirty();

	SetSelectedElement(SelectedElementIndex + 1);

	EndTransaction(TEXT("ElementAddAfter"));

	// Refresh viewport
	ElementEdVC->Viewport->Invalidate();
}

void WxLensFlareEditor::OnEnableElement(wxCommandEvent& In)
{
	EnableSelectedElement();
}

void WxLensFlareEditor::OnDisableElement(wxCommandEvent& In)
{
	DisableSelectedElement();
}

void WxLensFlareEditor::OnResetElement(wxCommandEvent& In)
{
	ResetSelectedElement();
}

void WxLensFlareEditor::OnResetInLevel(wxCommandEvent& In)
{
	ResetLensFlareInLevel();
}

void WxLensFlareEditor::OnSaveCam(wxCommandEvent& In)
{
//	PartSys->ThumbnailAngle = PreviewVC->PreviewAngle;
//	PartSys->ThumbnailDistance = PreviewVC->PreviewDistance;
//	PartSys->PreviewComponent = NULL;

	PreviewVC->bCaptureScreenShot = TRUE;
}

void WxLensFlareEditor::OnSavePackage(wxCommandEvent& In)
{
	debugf(TEXT("SAVE PACKAGE"));
	if (!LensFlare)
	{
		appMsgf(AMT_OK, TEXT("No lens flare active..."));
		return;
	}

	UPackage* Package = Cast<UPackage>(LensFlare->GetOutermost());
	if (Package)
	{
		debugf(TEXT("Have a package!"));

		FString FileTypes( TEXT("All Files|*.*") );
		
		for (INT i=0; i<GSys->Extensions.Num(); i++)
		{
			FileTypes += FString::Printf( TEXT("|(*.%s)|*.%s"), *GSys->Extensions(i), *GSys->Extensions(i) );
		}

		if (FindObject<UWorld>(Package, TEXT("TheWorld")))
		{
			appMsgf(AMT_OK, *LocalizeUnrealEd("Error_CantSaveMapViaLensFlareEditor"), *Package->GetName());
		}
		else
		{
			FString ExistingFile, File, Directory;
			FString PackageName = Package->GetName();

			if (GPackageFileCache->FindPackageFile( *PackageName, NULL, ExistingFile ))
			{
				FString Filename, Extension;
				GPackageFileCache->SplitPath( *ExistingFile, Directory, Filename, Extension );
				File = FString::Printf( TEXT("%s.%s"), *Filename, *Extension );
			}
			else
			{
				Directory = TEXT("");
				File = FString::Printf( TEXT("%s.upk"), *PackageName );
			}

			WxFileDialog SaveFileDialog( this, 
				*LocalizeUnrealEd("SavePackage"), 
				*Directory,
				*File,
				*FileTypes,
				wxSAVE,
				wxDefaultPosition);

			FString SaveFileName;

			if ( SaveFileDialog.ShowModal() == wxID_OK )
			{
				SaveFileName = FString( SaveFileDialog.GetPath() );

				if ( GFileManager->IsReadOnly( *SaveFileName ) || !GUnrealEd->Exec( *FString::Printf(TEXT("OBJ SAVEPACKAGE PACKAGE=\"%s\" FILE=\"%s\""), *PackageName, *SaveFileName) ) )
				{
					appMsgf( AMT_OK, *LocalizeUnrealEd("Error_CouldntSavePackage") );
				}
			}
		}

		if (LensFlare)
		{
			LensFlare->PostEditChange(NULL);
		}
	}
}

void WxLensFlareEditor::OnOrbitMode(wxCommandEvent& In)
{
	bOrbitMode = In.IsChecked();

	//@todo. actually handle this...
	if (bOrbitMode)
	{
		PreviewVC->SetPreviewCamera(PreviewVC->PreviewAngle, PreviewVC->PreviewDistance);
	}
}

void WxLensFlareEditor::OnWireframe(wxCommandEvent& In)
{
	bWireframe = In.IsChecked();
	PreviewVC->bWireframe = bWireframe;
}

void WxLensFlareEditor::OnBounds(wxCommandEvent& In)
{
	bBounds = In.IsChecked();
	PreviewVC->bBounds = bBounds;
}

/**
 *	Handler for turning post processing on and off.
 *
 *	@param	In	wxCommandEvent
 */
void WxLensFlareEditor::OnPostProcess(wxCommandEvent& In)
{
/***
	WxLensFlareEditorPostProcessMenu* menu = new WxLensFlareEditorPostProcessMenu(this);
	if (menu)
	{
		FTrackPopupMenu tpm(this, menu);
		tpm.Show();
		delete menu;
	}
***/
}

/**
 *	Handler for turning the grid on and off.
 *
 *	@param	In	wxCommandEvent
 */
void WxLensFlareEditor::OnToggleGrid(wxCommandEvent& In)
{
	bool bShowGrid = In.IsChecked();

	if (PreviewVC)
	{
		// Toggle the grid and worldbox.
		EditorOptions->bShowGrid = bShowGrid;
		EditorOptions->SaveConfig();
		PreviewVC->DrawHelper.bDrawGrid = bShowGrid;
		if (bShowGrid)
		{
			PreviewVC->ShowFlags |= SHOW_Grid;
		}
		else
		{
			PreviewVC->ShowFlags &= ~SHOW_Grid;
		}
	}
}

void WxLensFlareEditor::OnViewAxes(wxCommandEvent& In)
{
	PreviewVC->bDrawOriginAxes = In.IsChecked();
}

void WxLensFlareEditor::OnShowPPBloom( wxCommandEvent& In )
{
/***
	check(PreviewVC);
	if (In.IsChecked())
	{
		PreviewVC->ShowPPFlags |= CASC_SHOW_BLOOM;
	}
	else
	{
		PreviewVC->ShowPPFlags &= ~CASC_SHOW_BLOOM;
	}
	EditorOptions->ShowPPFlags = PreviewVC->ShowPPFlags;
	EditorOptions->SaveConfig();
	UpdatePostProcessChain();
***/
}

void WxLensFlareEditor::OnShowPPDOF( wxCommandEvent& In )
{
/***
	check(PreviewVC);
	if (In.IsChecked())
	{
		PreviewVC->ShowPPFlags |= CASC_SHOW_DOF;
	}
	else
	{
		PreviewVC->ShowPPFlags &= ~CASC_SHOW_DOF;
	}
	EditorOptions->ShowPPFlags = PreviewVC->ShowPPFlags;
	EditorOptions->SaveConfig();
	UpdatePostProcessChain();
***/
}

void WxLensFlareEditor::OnShowPPMotionBlur( wxCommandEvent& In )
{
/***
	check(PreviewVC);
	if (In.IsChecked())
	{
		PreviewVC->ShowPPFlags |= CASC_SHOW_MOTIONBLUR;
	}
	else
	{
		PreviewVC->ShowPPFlags &= ~CASC_SHOW_MOTIONBLUR;
	}
	EditorOptions->ShowPPFlags = PreviewVC->ShowPPFlags;
	EditorOptions->SaveConfig();
	UpdatePostProcessChain();
***/
}

void WxLensFlareEditor::OnShowPPVolumeMaterial( wxCommandEvent& In )
{
/***
	check(PreviewVC);
	if (In.IsChecked())
	{
		PreviewVC->ShowPPFlags |= CASC_SHOW_PPVOLUME;
	}
	else
	{
		PreviewVC->ShowPPFlags &= ~CASC_SHOW_PPVOLUME;
	}
	EditorOptions->ShowPPFlags = PreviewVC->ShowPPFlags;
	EditorOptions->SaveConfig();
	UpdatePostProcessChain();
***/
}

void WxLensFlareEditor::OnRealtime(wxCommandEvent& In)
{
	PreviewVC->SetRealtime(In.IsChecked());
}

void WxLensFlareEditor::OnBackgroundColor(wxCommandEvent& In)
{
	wxColour wxColorIn(PreviewVC->BackgroundColor.R, PreviewVC->BackgroundColor.G, PreviewVC->BackgroundColor.B);

	wxColour wxColorOut = wxGetColourFromUser(this, wxColorIn);
	if (wxColorOut.Ok())
	{
		PreviewVC->BackgroundColor.R = wxColorOut.Red();
		PreviewVC->BackgroundColor.G = wxColorOut.Green();
		PreviewVC->BackgroundColor.B = wxColorOut.Blue();
	}
/***
	//@todo. Remove this once the buffer clear code properly clears the depth values...
	PreviewVC->BackgroundMatInst->SetVectorParameterValue(FName(TEXT("Color")), 
		FLinearColor(
			(FLOAT)PreviewVC->BackgroundColor.R / 255.9f, 
			(FLOAT)PreviewVC->BackgroundColor.G / 255.9f, 
			(FLOAT)PreviewVC->BackgroundColor.B / 255.9f, 
			1.f));
	//@todo. END - Remove this once the buffer clear code properly clears the depth values...
***/
	EditorOptions->LFED_BackgroundColor = PreviewVC->BackgroundColor;
	EditorOptions->SaveConfig();
}

void WxLensFlareEditor::OnUndo(wxCommandEvent& In)
{
	LensFlareEditorUndo();
}

void WxLensFlareEditor::OnRedo(wxCommandEvent& In)
{
	LensFlareEditorRedo();
}

///////////////////////////////////////////////////////////////////////////////////////
// Properties window NotifyHook stuff

void WxLensFlareEditor::NotifyDestroy( void* Src )
{

}

static FLensFlareElement PreviousSource;
static TArray<FLensFlareElement> PreviousReflections;
void WxLensFlareEditor::NotifyPreChange( FEditPropertyChain* PropertyChain )
{
	BeginTransaction(TEXT("LensFlareEditorPropertyChange"));

	PreviousSource = LensFlare->SourceElement;
	PreviousReflections = LensFlare->Reflections;
}

void WxLensFlareEditor::NotifyPostChange( FEditPropertyChain* PropertyChain )
{
	for (INT ReflectionIndex = -1; ReflectionIndex < LensFlare->Reflections.Num() ; ReflectionIndex++)
	{
		FLensFlareElement* PrevElement = NULL;
		FLensFlareElement* Element = NULL;

		if (ReflectionIndex == -1)
		{
			PrevElement = &PreviousSource;
			Element = &(LensFlare->SourceElement);
		}
		else
		if (PreviousReflections.Num() > ReflectionIndex)
		{
			PrevElement = &(PreviousReflections(ReflectionIndex));
			Element = &(LensFlare->Reflections(ReflectionIndex));
		}
		else
		{
			continue;
		}

		if ((PrevElement == NULL) || (Element == NULL))
		{
			continue;
		}

		UObject* OldCurve = NULL;
		UObject* NewCurve = NULL;

		// Check each distribution...
		// This is horribly hard-coded and needs to be fixed!!!!
		
		if (PrevElement->LFMaterialIndex.Distribution != Element->LFMaterialIndex.Distribution)
		{
			OldCurve = PrevElement->LFMaterialIndex.Distribution;
			NewCurve = Element->LFMaterialIndex.Distribution;
		}
		else			 
		if (PrevElement->Scaling.Distribution != Element->Scaling.Distribution)
		{
			OldCurve = PrevElement->Scaling.Distribution;
			NewCurve = Element->Scaling.Distribution;
		}
		else			 
		if (PrevElement->AxisScaling.Distribution != Element->AxisScaling.Distribution)
		{
			OldCurve = PrevElement->AxisScaling.Distribution;
			NewCurve = Element->AxisScaling.Distribution;
		}
		else			 
		if (PrevElement->Rotation.Distribution != Element->Rotation.Distribution)
		{
			OldCurve = PrevElement->Rotation.Distribution;
			NewCurve = Element->Rotation.Distribution;
		}
		else			 
		if (PrevElement->Color.Distribution != Element->Color.Distribution)
		{
			OldCurve = PrevElement->Color.Distribution;
			NewCurve = Element->Color.Distribution;
		}
		else			 
		if (PrevElement->Alpha.Distribution != Element->Alpha.Distribution)
		{
			OldCurve = PrevElement->Alpha.Distribution;
			NewCurve = Element->Alpha.Distribution;
		}
		else			 
		if (PrevElement->Offset.Distribution != Element->Offset.Distribution)
		{
			OldCurve = PrevElement->Offset.Distribution;
			NewCurve = Element->Offset.Distribution;
		}

		if (NewCurve)
		{
			LensFlare->CurveEdSetup->ReplaceCurve(OldCurve, NewCurve);
			CurveEd->CurveChanged();
		}
	}
	PreviousReflections.Empty();

	if (LensFlare)
	{
		LensFlare->PostEditChange(PropertyChain->GetActiveNode()->GetValue());
		if (SelectedElement)
		{
			TArray<FLensFlareElementCurvePair> Curves;
			SelectedElement->GetCurveObjects(Curves);
			for (INT i=0; i<Curves.Num(); i++)
			{
				UDistributionFloat* DistF = Cast<UDistributionFloat>(Curves(i).CurveObject);
				UDistributionVector* DistV = Cast<UDistributionVector>(Curves(i).CurveObject);
				if (DistF)
					DistF->bIsDirty = TRUE;
				if (DistV)
					DistV->bIsDirty = TRUE;

				CurveEd->EdSetup->ChangeCurveColor(Curves(i).CurveObject, 
					FLinearColor(0.2f,0.2f,0.2f) * SelectedElementIndex);
					//SelectedElement->ModuleEditorColor);
				CurveEd->CurveChanged();
			}
		}
	}

	LensFlare->ThumbnailImageOutOfDate = TRUE;
	ElementEdVC->Viewport->Invalidate();

	check(TransactionInProgress());
	EndTransaction(TEXT("LensFlareEditorPropertyChange"));
}

void WxLensFlareEditor::NotifyExec( void* Src, const TCHAR* Cmd )
{
	GUnrealEd->NotifyExec(Src, Cmd);
}

///////////////////////////////////////////////////////////////////////////////////////
// Utils
void WxLensFlareEditor::CreateNewElement(INT ElementIndex)
{
	if (LensFlare == NULL)
	{
		return;
	}

	BeginTransaction(TEXT("CreateNewElement"));

	LensFlare->PreEditChange(NULL);
	LensFlareComp->PreEditChange(NULL);

	// Construct it and insert it into the array

	LensFlareComp->PostEditChange(NULL);
	LensFlare->PostEditChange(NULL);

	LensFlare->MarkPackageDirty();

	EndTransaction(TEXT("CreateNewElement"));

	// Refresh viewport
	ElementEdVC->Viewport->Invalidate();
}

void WxLensFlareEditor::SetSelectedElement(INT NewSelectedElement)
{
	if ((LensFlare == NULL) || 
		(NewSelectedElement == SelectedElementIndex) ||
		(NewSelectedElement < -1)
		)
	{
		return;
	}

	if (NewSelectedElement == -1)
	{
		// Set the 'source' element as selected.
		SelectedElementIndex = NewSelectedElement;
		SelectedElement = &(LensFlare->SourceElement);
	}
	else
	if (NewSelectedElement < LensFlare->Reflections.Num())
	{
		SelectedElementIndex = NewSelectedElement;
		SelectedElement = &(LensFlare->Reflections(SelectedElementIndex));
	}
	else
	{
		// Outside of the available element range... get out.
		return;
	}

#if 1
	PropertyWindow->SetObject(LensFlare, 1, 0, 1);
#else
	SelectedElementPropertyStub->FlareElement = SelectedElement;
	PropertyWindow->SetObject(SelectedElementPropertyStub, 1, 0, 1);
#endif
	SetSelectedInCurveEditor();
	ElementEdVC->Viewport->Invalidate();
}

void WxLensFlareEditor::DeleteSelectedElement()
{
	if ((LensFlare == NULL) || (SelectedElement == NULL))
	{
		return;
	}

	if (SelectedElementIndex == -1)
	{
		// Not allowed to delete the source of the lens flare.
		return;
	}

	BeginTransaction(TEXT("DeleteSelectedElement"));

	LensFlare->PreEditChange(NULL);
	LensFlareComp->PreEditChange(NULL);

	LensFlare->RemoveElementCurvesFromEditor(SelectedElementIndex, CurveEd->EdSetup);
	LensFlare->Reflections.Remove(SelectedElementIndex);

	LensFlareComp->PostEditChange(NULL);
	LensFlare->PostEditChange(NULL);

	SetSelectedElement(-1);

	ElementEdVC->Viewport->Invalidate();

	LensFlare->MarkPackageDirty();

	EndTransaction(TEXT("DeleteSelectedElement"));
}

void WxLensFlareEditor::EnableSelectedElement()
{
	if ((LensFlare == NULL) || (SelectedElement == NULL))
	{
		return;
	}

	BeginTransaction(TEXT("EnableSelectedElement"));

	LensFlare->PreEditChange(NULL);
	LensFlareComp->PreEditChange(NULL);

	SelectedElement->bIsEnabled = TRUE;

	LensFlareComp->PostEditChange(NULL);
	LensFlare->PostEditChange(NULL);
	LensFlare->MarkPackageDirty();

	EndTransaction(TEXT("EnableSelectedElement"));

	ElementEdVC->Viewport->Invalidate();
}

void WxLensFlareEditor::DisableSelectedElement()
{
	if ((LensFlare == NULL) || (SelectedElement == NULL))
	{
		return;
	}

	BeginTransaction(TEXT("DisableSelectedElement"));

	LensFlare->PreEditChange(NULL);
	LensFlareComp->PreEditChange(NULL);

	SelectedElement->bIsEnabled = FALSE;

	LensFlareComp->PostEditChange(NULL);
	LensFlare->PostEditChange(NULL);

	ElementEdVC->Viewport->Invalidate();

	LensFlare->MarkPackageDirty();

	EndTransaction(TEXT("DisableSelectedElement"));
}

void WxLensFlareEditor::ResetSelectedElement()
{
}

void WxLensFlareEditor::MoveSelectedElement(INT MoveAmount)
{
	if ((LensFlare == NULL) || (SelectedElement == NULL))
	{
		return;
	}

	if (SelectedElementIndex == -1)
	{
		// Not allowed to move the source of the lens flare.
		return;
	}
	if (SelectedElementIndex >= LensFlare->Reflections.Num())
	{
		// This should not happen!
		SelectedElementIndex = -1;
		return;
	}

	BeginTransaction(TEXT("MoveSelectedElement"));

	INT CurrentElementIndex = SelectedElementIndex;
	INT NewElementIndex = Clamp<INT>(CurrentElementIndex + MoveAmount, 0, LensFlare->Reflections.Num() - 1);

	if (NewElementIndex != CurrentElementIndex)
	{
		LensFlare->PreEditChange(NULL);
		LensFlareComp->PreEditChange(NULL);

		FLensFlareElement MovingElement = LensFlare->Reflections(SelectedElementIndex);
//		LensFlareElement
//		LensFlare->Reflections.RemoveItem(*SelectedElement);
//		LensFlare->Reflections.InsertZeroed(NewElementIndex);
//		LensFlare->Reflections(NewElementIndex) = *SelectedElement;

		LensFlareComp->PostEditChange(NULL);
		LensFlare->PostEditChange(NULL);

		ElementEdVC->Viewport->Invalidate();
	}

	LensFlare->MarkPackageDirty();

	EndTransaction(TEXT("MoveSelectedElement"));
}

void WxLensFlareEditor::AddSelectedToGraph()
{
	if ((LensFlare == NULL) || (SelectedElement == NULL))
	{
		return;
	}

	LensFlare->AddElementCurvesToEditor(SelectedElementIndex, LensFlare->CurveEdSetup);
	SetSelectedInCurveEditor();

	CurveEd->CurveEdVC->Viewport->Invalidate();
}

void WxLensFlareEditor::SetSelectedInCurveEditor()
{
	CurveEd->ClearAllSelectedCurves();
	if (SelectedElement)
	{
		TArray<FLensFlareElementCurvePair> Curves;
		SelectedElement->GetCurveObjects(Curves);
		for (INT CurveIndex = 0; CurveIndex < Curves.Num(); CurveIndex++)
		{
			UObject* Distribution = Curves(CurveIndex).CurveObject;
			if (Distribution)
			{
				CurveEd->SetCurveSelected(Distribution, TRUE);
			}
		}
	}

	CurveEd->CurveEdVC->Viewport->Invalidate();
}

/**
 *	This function returns the name of the docking parent.  This name is used for saving and loading the layout files.
 *  @return A string representing a name to use for this docking parent.
 */
const TCHAR* WxLensFlareEditor::GetDockingParentName() const
{
	return DockingParent_Name;
}

/**
 * @return The current version of the docking parent, this value needs to be increased every time new docking windows are added or removed.
 */
const INT WxLensFlareEditor::GetDockingParentVersion() const
{
	return DockingParent_Version;
}


//
// ULensFlareEditorOptions
// 
