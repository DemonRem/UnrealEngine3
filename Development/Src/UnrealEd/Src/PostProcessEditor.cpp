/*=============================================================================
	PostProcessEditor.cpp: PostProcess editing
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "UnLinkedObjEditor.h"
#include "UnLinkedObjDrawUtils.h"
#include "PostProcessEditor.h"
#include "PropertyWindow.h"

const FColor& GetNodeBorderColor(UBOOL bSelected)
{
	static const FColor NodeBorderColor(255,0,255);
	static const FColor NodeSelectedBorderColor(255,255,0);
	return bSelected ? NodeSelectedBorderColor : NodeBorderColor;
}

const FColor& GetNodeBkgColor(UBOOL bSelected)
{
	static const FColor NodeBkgColor(112,112,112);
	static const FColor NodeSelectedBkgColor(180,180,180);
	return bSelected ? NodeSelectedBkgColor : NodeBkgColor;
}

const FColor& GetSplineColor()
{
	static const FColor SplineColor(0,255,0);
	return SplineColor;
}

const FColor& GetOutConnectColor()
{
	static const FColor OutConnectColor(0,255,0);
	return OutConnectColor;
}

const FColor& GetInConnectColor()
{
	static const FColor InConnectColor(255,0,0);
	return InConnectColor;
}

/*
* Initial node in any post process chain. Dummy class representing start point
*/
class USceneRenderTarget : public UPostProcessEffect
{
public:
	DECLARE_CLASS_INTRINSIC(USceneRenderTarget,UPostProcessEffect,0,UnrealEd)

	USceneRenderTarget()
	{
		NodePosX = 100;
		NodePosY = 100;
	}
};
IMPLEMENT_CLASS(USceneRenderTarget);


/**
 * Construct a WxPostProcessEditorToolBar.
 *
 * @param	Parent window of the toolbar
 * @param	ID of the toolbar
 */
WxPostProcessEditorToolBar::WxPostProcessEditorToolBar( wxWindow* InParent, wxWindowID InID )
: WxToolBar( InParent, InID, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL | wxTB_FLAT | wxTB_3DBUTTONS )
{
	// Load required images
	SyncBrowserBitmap.Load( TEXT("FindInBrowser.png") );

	SetToolBitmapSize( wxSize( 18, 18 ) );

	// Add "sync browser" button to the toolbar
	AddTool( IDM_POSTPROCESS_SYNC_BROWSER, SyncBrowserBitmap, *LocalizeUnrealEd("SyncGenericBrowser") );
	Realize();
}

/*
* Draws a node representing a UPostProcessEffect
*/
void WxPostProcessEditor::DrawNode( UPostProcessEffect* Node, FCanvas* Canvas, UBOOL bSelected)
{
	// Construct the FLinkedObjDrawInfo for use the linked-obj drawing utils.
	FLinkedObjDrawInfo ObjInfo;

	// add input/output connections
	if(!Node->IsA(USceneRenderTarget::StaticClass()))
	{
		ObjInfo.Inputs.AddItem( FLinkedObjConnInfo(TEXT("In"),GetInConnectColor() ) );
	}
	ObjInfo.Outputs.AddItem( FLinkedObjConnInfo(TEXT("Out"),GetOutConnectColor()) );
	ObjInfo.ObjObject = Node;

	// Use util to draw box with connectors etc.
	FLinkedObjDrawUtils::DrawLinkedObj( 
		Canvas, 
		ObjInfo, 
		*Node->GetClass()->GetDescription(), 
		NULL, 
		GetNodeBorderColor(bSelected), 
		GetNodeBkgColor(bSelected), 
		FIntPoint(Node->NodePosX, Node->NodePosY) 
		);

	// Read back draw locations of connectors, so we can draw lines in the correct places.
	if(!Node->IsA(USceneRenderTarget::StaticClass()))
	{
		Node->InDrawY = ObjInfo.InputY(0);
	}
	Node->OutDrawY = ObjInfo.OutputY(0);
	Node->DrawWidth = ObjInfo.DrawWidth;
	Node->DrawHeight = ObjInfo.DrawHeight;
	INT SliderDrawY = Node->NodePosY + ObjInfo.DrawHeight;

	// Draw link to the previous node, if we have one
	for(INT i=0; i<PostProcess->Effects.Num(); i++)
	{
		if(PostProcess->Effects(i) == Node)
		{
			FIntPoint Start; 
			FIntPoint End;  
			
			// First node connects to USceneRenderTarget
			if(i == 0)
			{
				End = GetConnectionLocation(TreeNodes(0),LOC_OUTPUT, i);
				Start =   GetConnectionLocation(Node,LOC_INPUT, 0);
			}
			else
			{
				End = GetConnectionLocation(PostProcess->Effects(i-1),LOC_OUTPUT, i);
				Start =   GetConnectionLocation(Node,LOC_INPUT, 0);
			}

			FLOAT Tension = Abs<INT>(Start.X - End.X);
			FLinkedObjDrawUtils::DrawSpline(Canvas, End, Tension * FVector2D(1,0), Start, Tension * FVector2D(1,0), GetSplineColor(), true);
		}
	}
}

void WxPostProcessEditor::OpenNewObjectMenu()
{
	class WxMBPostProcessEdNewNode : public wxMenu
	{
	public:
		WxMBPostProcessEdNewNode(WxPostProcessEditor* PostProcessEd)
		{
			INT Count = 0;
			for(TObjectIterator<UClass> It;It;++It)
			{
				if( It->IsChildOf(UPostProcessEffect::StaticClass()) && 
					!(It->ClassFlags & CLASS_Abstract) && 
					!(It->ClassFlags & CLASS_Deprecated) && 
					*It != UPostProcessEffect::StaticClass() &&
					*It != USceneRenderTarget::StaticClass() )
				{
					Append( IDM_POSTPROCESS_NEW_NODE_START+Count, *It->GetName(), TEXT("") );
					Count++;
				}
			}
		}
	};

	WxMBPostProcessEdNewNode menu( this );
	FTrackPopupMenu tpm( this, &menu );
	tpm.Show();
}

void WxPostProcessEditor::OpenObjectOptionsMenu()
{
	class WxMBPostProcessEdNodeOptions : public wxMenu
	{
	public:
		WxMBPostProcessEdNodeOptions(WxPostProcessEditor* PostProcessEd)
		{
			Append( IDM_POSTPROCESS_BREAK_ALL_LINKS, *LocalizeUnrealEd("BreakAllLinks"), TEXT("") );
			AppendSeparator();

			const INT NumSelected = PostProcessEd->SelectedNodes.Num();
			if( NumSelected == 1 )
			{
				Append( IDM_POSTPROCESS_DELETE_NODE, *LocalizeUnrealEd("DeleteSelectedItem"), TEXT("") );
			}
			else
			{
				Append( IDM_POSTPROCESS_DELETE_NODE, *LocalizeUnrealEd("DeleteSelectedItems"), TEXT("") );
			}
		}
	};

	WxMBPostProcessEdNodeOptions menu( this );
	FTrackPopupMenu tpm( this, &menu );
	tpm.Show();
}

void WxPostProcessEditor::DrawObjects(FViewport* Viewport, FCanvas* Canvas)
{
	if (BackgroundTexture != NULL)
	{
		Canvas->PushAbsoluteTransform(FMatrix::Identity);

		Clear(Canvas, FColor(161,161,161) );

		const INT ViewWidth = LinkedObjVC->Viewport->GetSizeX();
		const INT ViewHeight = LinkedObjVC->Viewport->GetSizeY();

		// draw the texture to the side, stretched vertically
		DrawTile(Canvas, ViewWidth - BackgroundTexture->SizeX, 0,
			BackgroundTexture->SizeX, ViewHeight,
			0.f, 0.f,
			1.f, 1.f,
			FLinearColor::White,
			BackgroundTexture->Resource );

		// stretch the left part of the texture to fill the remaining gap
		if (ViewWidth > BackgroundTexture->SizeX)
		{
			DrawTile(Canvas, 0, 0,
				ViewWidth - BackgroundTexture->SizeX, ViewHeight,
				0.f, 0.f,
				0.1f, 0.1f,
				FLinearColor::White,
				BackgroundTexture->Resource );
		}

		Canvas->PopTransform();
	}

	for(INT i=0; i<TreeNodes.Num(); i++)
	{
		UPostProcessEffect* DrawnNode = TreeNodes(i);

		const UBOOL bNodeSelected = SelectedNodes.ContainsItem( DrawnNode );
		DrawNode(DrawnNode,Canvas, bNodeSelected);

		FLinkedObjDrawUtils::DrawTile( Canvas, DrawnNode->NodePosX,		DrawnNode->NodePosY - 3 - 8, 10,	10, 0.f, 0.f, 1.f, 1.f, FColor(0,0,0) );
		FLinkedObjDrawUtils::DrawTile( Canvas, DrawnNode->NodePosX + 1,	DrawnNode->NodePosY - 2 - 8, 8,	8,	0.f, 0.f, 1.f, 1.f, FColor(255,215,0) );
	}
}

void WxPostProcessEditor::UpdatePropertyWindow()
{
	// If we have both controls and nodes selected - just show nodes in property window...
	if(SelectedNodes.Num() > 0)
	{
		PropertyWindow->SetObjectArray( SelectedNodes, EPropertyWindowFlags::ShouldShowCategories );
	}
}

void WxPostProcessEditor::EmptySelection()
{
	SelectedNodes.Empty();
}

void WxPostProcessEditor::AddToSelection( UObject* Obj )
{
	if( Obj->IsA(UPostProcessEffect::StaticClass()) )
	{
		SelectedNodes.AddItem( (UPostProcessEffect*)Obj );
	}	
}

void WxPostProcessEditor::RemoveFromSelection( UObject* Obj )
{
	if( Obj->IsA(UPostProcessEffect::StaticClass()) )
	{
		SelectedNodes.RemoveItem( (UPostProcessEffect*)Obj );
	}	
}

UBOOL WxPostProcessEditor::IsInSelection( UObject* Obj ) const
{
	UBOOL bIsSelected = FALSE;
	if( Obj->IsA(UPostProcessEffect::StaticClass()) )
	{
		bIsSelected = SelectedNodes.ContainsItem( (UPostProcessEffect*)Obj );
	}
	return bIsSelected;
}

INT WxPostProcessEditor::GetNumSelected() const
{
	return SelectedNodes.Num();
}

void WxPostProcessEditor::SetSelectedConnector( FLinkedObjectConnector& Connector )
{
	ConnObj = Connector.ConnObj;
	ConnType = Connector.ConnType;
	ConnIndex = Connector.ConnIndex;
}

FIntPoint WxPostProcessEditor::GetConnectionLocation(UObject* ConnObj, INT ConnType, INT ConnIndex)
{
	UPostProcessEffect* EffectNode = Cast<UPostProcessEffect>(ConnObj);
	if(ConnType == LOC_INPUT && EffectNode)
	{
		check(ConnIndex == 0);
		return FIntPoint( EffectNode->NodePosX - LO_CONNECTOR_LENGTH, EffectNode->InDrawY );
	}
	else if(EffectNode)
	{
		return FIntPoint( EffectNode->NodePosX + EffectNode->DrawWidth+ LO_CONNECTOR_LENGTH, EffectNode->OutDrawY );
	}

	return FIntPoint(0,0);
}

FIntPoint WxPostProcessEditor::GetSelectedConnLocation(FCanvas* Canvas)
{
	if(ConnObj)
	{
		return GetConnectionLocation(ConnObj,ConnType, ConnIndex);
	}

	return FIntPoint(0,0);
}

INT WxPostProcessEditor::GetSelectedConnectorType()
{
	return ConnType;
}

FColor WxPostProcessEditor::GetMakingLinkColor()
{
	return FColor(0,0,0);
}

void WxPostProcessEditor::MakeConnectionToConnector( FLinkedObjectConnector& Connector )
{
	// Avoid connections to yourself.
	if(!Connector.ConnObj || !ConnObj || Connector.ConnObj == ConnObj)
	{
		return;
	}

	UPostProcessEffect* EndConnNode = Cast<UPostProcessEffect>(Connector.ConnObj);
	check(EndConnNode);

	UPostProcessEffect* ConnNode = Cast<UPostProcessEffect>(ConnObj);
	check(ConnNode);

	// If connected to root node (USceneRenderTarget), add to start of chain
	if(ConnNode->IsA(USceneRenderTarget::StaticClass()))
	{
		// First, remove if already exists somewhere else in the chain
		PostProcess->Effects.RemoveItem(EndConnNode);

		// Add at start of chain
		PostProcess->Effects.InsertItem(EndConnNode,0);
	}
	// Otherwise figure out where on the chain this is being added
	else
	{
	    // First, remove if already exists somewhere else in the chain
	    PostProcess->Effects.RemoveItem(EndConnNode);
	    // Now insert into correct new place
	    for(INT I=0;I<PostProcess->Effects.Num();I++)
	    {
		    if(PostProcess->Effects(I) == ConnNode)
		    {
			    if(I == PostProcess->Effects.Num()-1)
			    {
				    PostProcess->Effects.AddItem(EndConnNode);
			    }
			    else
			    {
				    PostProcess->Effects.InsertItem(EndConnNode,I+1);
			    }
			    break;
		    }
	    }
	}

	// Mark the package as dirty since the object changed. 
	NotifyObjectsChanged();
}

void WxPostProcessEditor::MakeConnectionToObject( UObject* EndObj )
{
	INT nop=0;
	nop=nop;
}

void WxPostProcessEditor::MoveSelectedObjects( INT DeltaX, INT DeltaY )
{
	for(INT i=0; i<SelectedNodes.Num(); i++)
	{
		UPostProcessEffect* Node = SelectedNodes(i);

		Node->NodePosX += DeltaX;
		Node->NodePosY += DeltaY;
	}
}

void WxPostProcessEditor::EdHandleKeyInput(FViewport* Viewport, FName Key, EInputEvent Event)
{
	UBOOL bCtrlDown = Viewport->KeyState(KEY_LeftControl) || Viewport->KeyState(KEY_RightControl);
	UBOOL bShiftDown = Viewport->KeyState(KEY_LeftShift) || Viewport->KeyState(KEY_RightShift);
	UBOOL bAltDown = Viewport->KeyState(KEY_LeftAlt) || Viewport->KeyState(KEY_RightAlt);

	if(Event == IE_Pressed)
	{
		if(Key == KEY_Delete)
		{
			DeleteSelectedObjects();
		}
		else if(Key == KEY_W && bCtrlDown)
		{
			DuplicateSelectedObjects();
		}
	}
}

/** The little button for doing automatic blending in/out of controls uses special index 1, so we look for it being clicked here. */
UBOOL WxPostProcessEditor::SpecialClick( INT NewX, INT NewY, INT SpecialIndex, FViewport* Viewport, UObject* ProxyObj )
{
	// Handle clicking on slider region jumping straight to that value.
	if(SpecialIndex == 0)
	{
		SpecialDrag(0, 0, NewX, NewY, SpecialIndex);
	}
	
	return FALSE;
}


/**
 * Called when one or more objects changed
 */
void WxPostProcessEditor::NotifyObjectsChanged()
{
	if( PostProcess != NULL )
	{
		PostProcess->MarkPackageDirty();
	}
}

/*-----------------------------------------------------------------------------
	WxPostProcessEditor
-----------------------------------------------------------------------------*/

BEGIN_EVENT_TABLE( WxPostProcessEditor, wxFrame )
	EVT_CLOSE( WxPostProcessEditor::OnClose )
	EVT_MENU_RANGE( IDM_POSTPROCESS_NEW_NODE_START, IDM_POSTPROCESS_NEW_NODE_END, WxPostProcessEditor::OnNewPostProcessNode )
	EVT_MENU( IDM_POSTPROCESS_DELETE_NODE, WxPostProcessEditor::OnDeleteObjects )
	EVT_MENU( IDM_POSTPROCESS_BREAK_ALL_LINKS, WxPostProcessEditor::OnBreakAllLinks )
	EVT_MENU( IDM_POSTPROCESS_SYNC_BROWSER, WxPostProcessEditor::OnSyncBrowser )
	EVT_SIZE( WxPostProcessEditor::OnSize )
END_EVENT_TABLE()

IMPLEMENT_COMPARE_POINTER( UClass, PostProcessEditor, { return appStricmp( *A->GetName(), *B->GetName() ); } )

WxPostProcessEditor::WxPostProcessEditor( wxWindow* InParent, wxWindowID InID, class UPostProcessChain* InPostProcess )
	:	wxFrame( InParent, InID, TEXT(""), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT | wxFRAME_NO_TASKBAR )
	,	FDockingParent(this)
	,	bEditorClosing(FALSE)
	,	PostProcess(InPostProcess)
	,	SceneRenderTarget(ConstructObject<USceneRenderTarget>(USceneRenderTarget::StaticClass(), PostProcess, NAME_None))
	,	LinkedObjVC(NULL)
	,   ToolBar(NULL)
{
	check(PostProcess);
	check(SceneRenderTarget);

	SetTitle( *FString::Printf( LocalizeSecure(LocalizeUnrealEd("PostProcessEditor_F"), *PostProcess->GetName()) ) );
	
	SetSize(1024, 768);
	// Load the desired window position from .ini file
	FWindowUtil::LoadPosSize(TEXT("PostProcessEditor"), this, 256, 256, 1024, 768);

	// Create toolbar
	ToolBar = new WxPostProcessEditorToolBar( this, wxID_ANY );
	SetToolBar( ToolBar );
	
	// Create property window
	PropertyWindow = new WxPropertyWindowHost;
	PropertyWindow->Create( this, this );

	// Create linked-object tree window
	WxLinkedObjVCHolder* TreeWin = new WxLinkedObjVCHolder( this, -1, this );
	LinkedObjVC = TreeWin->LinkedObjVC;

	{
		AddDockingWindow(PropertyWindow, FDockingParent::DH_Bottom, *FString::Printf(LocalizeSecure(LocalizeUnrealEd("PropertiesCaption_F"), *PostProcess->GetName())), *LocalizeUnrealEd("Properties"));

		AddDockingWindow( TreeWin, FDockingParent::DH_None, NULL );

		LoadDockingLayout();
	}
	
	wxMenuBar* MenuBar = new wxMenuBar;
	AppendWindowMenu(MenuBar);
	SetMenuBar(MenuBar);

	BackgroundTexture = LoadObject<UTexture2D>(NULL, TEXT("EditorMaterials.AnimTreeBackGround"), NULL, LOAD_None, NULL);

	// Add the SceneRenderTarget node.
	TreeNodes.AddItem(SceneRenderTarget);

	// add existing effects from post process chain
	for(INT I=0;I<InPostProcess->Effects.Num();I++)
	{
		UPostProcessEffect* Effect = InPostProcess->Effects(I);
		if( Effect )
		{
			TreeNodes.AddItem(Effect);
		}
	}
	// select the last effect from the chain
	UPostProcessEffect* SelectedEffect = SceneRenderTarget;
	if( InPostProcess->Effects.Num() )
	{
		SelectedEffect = InPostProcess->Effects(InPostProcess->Effects.Num()-1);
	}	
	if( SelectedEffect )
	{
		AddToSelection(SelectedEffect);
	}
	
	UpdatePropertyWindow();
}

WxPostProcessEditor::~WxPostProcessEditor()
{
	SaveDockingLayout();

	FWindowUtil::SavePosSize(TEXT("PostProcessEditor"), this);
}

void WxPostProcessEditor::OnClose( wxCloseEvent& In )
{
	//PostProcess->bBeingEdited = false;
	bEditorClosing = true;
	this->Destroy();
}

void WxPostProcessEditor::Serialize(FArchive& Ar)
{
//	PreviewVC->Serialize(Ar);

	// Make sure we don't garbage collect nodes/control that are not currently linked into the tree.
	if(!Ar.IsLoading() && !Ar.IsSaving())
	{
		Ar << SceneRenderTarget;
		for(INT i=0; i<TreeNodes.Num(); i++)
		{
			Ar << TreeNodes(i);
		}
	}
}

//////////////////////////////////////////////////////////////////
// Menu Handlers
//////////////////////////////////////////////////////////////////

void WxPostProcessEditor::OnNewPostProcessNode(wxCommandEvent& In)
{
	const INT NewNodeClassIndex = In.GetId() - IDM_POSTPROCESS_NEW_NODE_START;

	UClass* NewNodeClass = 0;
	INT Index = 0;
	for(TObjectIterator<UClass> It;It;++It)
	{
		if( It->IsChildOf(UPostProcessEffect::StaticClass()) && 
			!(It->ClassFlags & CLASS_Abstract) && 
			!(It->ClassFlags & CLASS_Deprecated) && 
			*It != UPostProcessEffect::StaticClass() &&
			*It != USceneRenderTarget::StaticClass() )
		{
			if(NewNodeClassIndex == Index)
			{
				NewNodeClass = *It;
			}
			Index++;
		}
	}

	check(NewNodeClass && NewNodeClass->IsChildOf(UPostProcessEffect::StaticClass()) );

	UPostProcessEffect* NewNode = ConstructObject<UPostProcessEffect>( NewNodeClass, PostProcess, NAME_None);

	NewNode->NodePosX = (LinkedObjVC->NewX - LinkedObjVC->Origin2D.X)/LinkedObjVC->Zoom2D;
	NewNode->NodePosY = (LinkedObjVC->NewY - LinkedObjVC->Origin2D.Y)/LinkedObjVC->Zoom2D;

	TreeNodes.AddItem(NewNode);

	// Mark the package as dirty since the object changed. 
	NotifyObjectsChanged();

	RefreshViewport();
}

/** Break all links going to or from the selected node(s). */
void WxPostProcessEditor::OnBreakAllLinks(wxCommandEvent& In)
{
	UBOOL bLinkBroken = FALSE;

	for(INT i=0; i<SelectedNodes.Num(); i++)
	{
		UPostProcessEffect* Tree = Cast<UPostProcessEffect>(SelectedNodes(i));
		PostProcess->Effects.RemoveItem(Tree);
		bLinkBroken = TRUE;
	}

	// Mark the package as dirty since the object changed. 
	if( bLinkBroken )
	{
		NotifyObjectsChanged();
	}

	RefreshViewport();
}

void WxPostProcessEditor::OnDeleteObjects(wxCommandEvent& In)
{
	DeleteSelectedObjects();
}

/**
 * Called when the user presses the "Sync Browser" button. Finds and selects
 * the current post process chain within the generic/content browser.
 *
 * @param	In	Event generated by wxWidgets when the user presses the relevant button
 */
void WxPostProcessEditor::OnSyncBrowser( wxCommandEvent& In )
{
	// Sync the browser to the post process chain being edited
	TArray<UObject*> ObjsToSync;
	ObjsToSync.AddItem( PostProcess );
	GApp->EditorFrame->SyncBrowserToObjects( ObjsToSync );
}

void WxPostProcessEditor::OnSize(wxSizeEvent& In)
{
	RefreshViewport();
	In.Skip();
}

//////////////////////////////////////////////////////////////////
// Utils
//////////////////////////////////////////////////////////////////

void WxPostProcessEditor::RefreshViewport()
{
	if (LinkedObjVC)
	{
		if (LinkedObjVC->Viewport)
		{
			LinkedObjVC->Viewport->Invalidate();
		}
	}
}

void WxPostProcessEditor::DeleteSelectedObjects()
{
	UBOOL bNodeDeleted = FALSE;
	// Break all links
	for(INT i=0; i<SelectedNodes.Num(); i++)
	{
		UPostProcessEffect* Effect = Cast<UPostProcessEffect>(SelectedNodes(i));
		PostProcess->Effects.RemoveItem(Effect);
		TreeNodes.RemoveItem(Effect);
		bNodeDeleted = TRUE;
	}
	SelectedNodes.Empty();

	// Mark the package as dirty since the object changed. 
	if( bNodeDeleted )
	{
		NotifyObjectsChanged();
	}

	UpdatePropertyWindow();
	RefreshViewport();
}

void WxPostProcessEditor::DuplicateSelectedObjects()
{
	// We set the selection to being the newly created nodes.
	EmptySelection();
}

///////////////////////////////////////////////////////////////////////////////////////
// Properties window NotifyHook stuff

void WxPostProcessEditor::NotifyPreChange( void* Src, UProperty* PropertyAboutToChange )
{
	GEditor->BeginTransaction( *LocalizeUnrealEd("EditLinkedObj") );

	for ( WxPropertyWindow::TObjectIterator Itor( PropertyWindow->ObjectIterator() ) ; Itor ; ++Itor )
	{
		(*Itor)->Modify();
	}
}

void WxPostProcessEditor::NotifyPostChange( void* Src, UProperty* PropertyThatChanged )
{
	GEditor->EndTransaction();
	RefreshViewport();
}

void WxPostProcessEditor::NotifyExec( void* Src, const TCHAR* Cmd )
{
	GUnrealEd->NotifyExec(Src, Cmd);
}


///////////////////////////////////////////////////////////////////////////////////////
// FDockingParent Interface


/**
 *	This function returns the name of the docking parent.  This name is used for saving and loading the layout files.
 *  @return A string representing a name to use for this docking parent.
 */
const TCHAR* WxPostProcessEditor::GetDockingParentName() const
{
	return TEXT("PostProcessEditor");
}

/**
 * @return The current version of the docking parent, this value needs to be increased every time new docking windows are added or removed.
 */
const INT WxPostProcessEditor::GetDockingParentVersion() const
{
	return 0;
}

