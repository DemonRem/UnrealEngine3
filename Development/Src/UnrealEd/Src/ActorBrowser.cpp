/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "UnClassTree.h"
#include "ActorBrowser.h"

/**
 * The menu for the actor browser.
 */
class WxMBActorBrowser : public wxMenuBar
{
public:
	WxMBActorBrowser()
	{
		// File menu
		wxMenu* FileMenu = new wxMenu();

		FileMenu->Append( IDMN_AB_FileOpen, *LocalizeUnrealEd("OpenE"), TEXT("") );
		FileMenu->Append( IDMN_AB_EXPORT_ALL, *LocalizeUnrealEd("ExportAllScripts"), TEXT("") );

		Append( FileMenu, *LocalizeUnrealEd("File") );

		WxBrowser::AddDockingMenu( this );
	}
};

BEGIN_EVENT_TABLE( WxActorBrowser, WxBrowser )
	EVT_MENU( IDMN_AB_FileOpen, WxActorBrowser::OnFileOpen )
	EVT_MENU( IDMN_AB_EXPORT_ALL, WxActorBrowser::OnFileExportAll )
	EVT_MENU( IDMN_ActorBrowser_ViewSource, WxActorBrowser::OnViewSource )
	EVT_MENU( IDMN_ActorBrowser_CreateArchetype, WxActorBrowser::OnCreateArchetype )
END_EVENT_TABLE()

WxActorBrowser::WxActorBrowser()
	:	bUpdateOnActivated( FALSE )
{
	// Register the ActorBrowser event
	GCallbackEvent->Register(CALLBACK_RefreshEditor_ActorBrowser,this);

	// Create a class tree.
	ClassTree = new FClassTree( UObject::StaticClass() );
	check( ClassTree );
	for( TObjectIterator<UClass> It ; It ; ++It )
	{
		UClass* CurClass = *It;
		ClassTree->AddClass( CurClass );
	}

	RightClickedClass = NULL;
}

WxActorBrowser::~WxActorBrowser()
{
	delete ClassTree;
}

/**
 * Forwards the call to our base class to create the window relationship.
 * Creates any internally used windows after that
 *
 * @param DockID the unique id to associate with this dockable window
 * @param FriendlyName the friendly name to assign to this window
 * @param Parent the parent of this window (should be a Notebook)
 */
void WxActorBrowser::Create(INT DockID,const TCHAR* FriendlyName,wxWindow* Parent)
{
	WxBrowser::Create(DockID,FriendlyName,Parent);

	Panel = (wxPanel*)wxXmlResource::Get()->LoadPanel( this, TEXT("ID_PANEL_ACTORCLASSBROWSER") );
	check( Panel );
	Panel->Fit();

	TreeCtrl = wxDynamicCast( FindWindow( XRCID( "IDTC_CLASSES" ) ), wxTreeCtrl );
	check( TreeCtrl != NULL );
	ActorAsParentCheck = wxDynamicCast( FindWindow( XRCID( "IDCK_USE_ACTOR_AS_PARENT" ) ), wxCheckBox );
	check( ActorAsParentCheck != NULL );
	PlaceableCheck = wxDynamicCast( FindWindow( XRCID( "IDCK_PLACEABLE_CLASSES_ONLY" ) ), wxCheckBox );
	check( PlaceableCheck != NULL );
	FullPathStatic = wxDynamicCast( FindWindow( XRCID( "IDST_FULLPATH" ) ), wxStaticText );
	check( FullPathStatic != NULL );
	PackagesList = wxDynamicCast( FindWindow( XRCID( "IDLB_PACKAGES" ) ), wxListBox );
	check( PackagesList != NULL );

	Update();

	MenuBar = new WxMBActorBrowser();

	ADDEVENTHANDLER( XRCID("IDTC_CLASSES"), wxEVT_COMMAND_TREE_ITEM_EXPANDING, &WxActorBrowser::OnItemExpanding );
	ADDEVENTHANDLER( XRCID("IDTC_CLASSES"), wxEVT_COMMAND_TREE_SEL_CHANGED, &WxActorBrowser::OnSelChanged );
	ADDEVENTHANDLER( XRCID("IDTC_CLASSES"), wxEVT_COMMAND_TREE_ITEM_RIGHT_CLICK, &WxActorBrowser::OnItemRightClicked );
	ADDEVENTHANDLER( XRCID("IDCK_USE_ACTOR_AS_PARENT"), wxEVT_COMMAND_CHECKBOX_CLICKED , &WxActorBrowser::OnUseActorAsParent );
	ADDEVENTHANDLER( XRCID("IDCK_PLACEABLE_CLASSES_ONLY"), wxEVT_COMMAND_CHECKBOX_CLICKED , &WxActorBrowser::OnPlaceableClassesOnly );

	FLocalizeWindow( this );
}

void WxActorBrowser::Update()
{
	BeginUpdate();

	if ( IsShown() )
	{
		UpdatePackageList();
		UpdateActorList();
		bUpdateOnActivated = FALSE;
	}
	else
	{
		PackagesList->Clear();
		TreeCtrl->DeleteAllItems();
		bUpdateOnActivated = TRUE;
	}

	EndUpdate();
}

void WxActorBrowser::Activated()
{
	WxBrowser::Activated();
	if ( bUpdateOnActivated )
	{
		Update();
	}
}

void WxActorBrowser::UpdatePackageList()
{
	TArray<UPackage*> Packages;
	GUnrealEd->GetPackageList( &Packages, NULL );

	PackagesList->Clear();

	for( INT x = 0 ; x < Packages.Num() ; ++x )
	{
		PackagesList->Append( *Packages(x)->GetName(), Packages(x) );
	}
}

void WxActorBrowser::UpdateActorList()
{
	TreeCtrl->DeleteAllItems();

	if( ActorAsParentCheck->IsChecked() )
	{
		TreeCtrl->AddRoot( *LocalizeUnrealEd("Actor"), -1, -1, new WxTreeObjectWrapper( AActor::StaticClass() ) );
	}
	else
	{
		TreeCtrl->AddRoot( *LocalizeUnrealEd("Object"), -1, -1, new WxTreeObjectWrapper( UObject::StaticClass() ) );
	}

	AddChildren( TreeCtrl->GetRootItem() );
	TreeCtrl->Expand( TreeCtrl->GetRootItem() );
}

static UBOOL HasChildren(const FClassTree* ClassNode, UBOOL bPlaceableOnly)
{
	TArray<const FClassTree*> ChildNodes;
	ClassNode->GetChildClasses( ChildNodes );

	for ( INT i = 0 ; i < ChildNodes.Num() ; ++i )
	{
		UClass* CurClass = ChildNodes(i)->GetClass();
		if ( CurClass->ClassFlags & CLASS_Hidden )
		{
			return FALSE;
		}
		else if ( !bPlaceableOnly || (CurClass->ClassFlags & CLASS_Placeable) )
		{
			return TRUE;
		}
		else if ( HasChildren( ChildNodes(i), bPlaceableOnly ) )
		{
			return TRUE;
		}
	}
	return FALSE;
}

class FActorBrowserClassMask
{
public:
	FActorBrowserClassMask(UBOOL InPlaceableOnly)
		:	bPlaceableOnly( InPlaceableOnly )
	{}

	UBOOL IsValidClass(const FClassTree* Node) const
	{
		const UClass* Class = Node->GetClass();

		if ( Class->HasAnyClassFlags(CLASS_Hidden | CLASS_Deprecated) )
		{
			return FALSE;
		}
		if ( bPlaceableOnly && !Class->HasAnyClassFlags( CLASS_Placeable ) )
		{
			if ( !HasChildren( Node, TRUE ) )
			{
				return FALSE;
			}
		}
		return TRUE;
	}
private:
	const UBOOL bPlaceableOnly;
};

void WxActorBrowser::AddChildren( wxTreeItemId InID )
{
	WxTreeObjectWrapper* ItemData = (WxTreeObjectWrapper*) TreeCtrl->GetItemData( InID );
	if( ItemData )
	{
		// Prune the complete class tree at the specified class.
		UClass*	Class		= ItemData->GetObject<UClass>();
		FClassTree* RootedTree = ClassTree->GetNode( Class );

		if ( RootedTree )
		{
			// Filter the tree against placeable, non-hidden, non-deprecated classes.
			const UBOOL	bPlaceableOnly	= PlaceableCheck->IsChecked();

			FClassTree* MaskedTree = RootedTree->GenerateMaskedClassTree( FActorBrowserClassMask( bPlaceableOnly ) );
			if ( MaskedTree )
			{
				// Get a list of all direct descendants of this class.
				TArray<FClassTree*> ChildClasses;
				MaskedTree->GetChildClasses( ChildClasses );

				for ( INT ClassIndex = 0 ; ClassIndex < ChildClasses.Num() ; ++ClassIndex )
				{
					FClassTree* ChildTree = ChildClasses( ClassIndex );
					UClass* CurClass = ChildTree->GetClass();

					const wxTreeItemId Wk = TreeCtrl->AppendItem( InID, *CurClass->GetName(), -1, -1, new WxTreeObjectWrapper( CurClass ) );

					// Mark placeable classes in bold.
					if( CurClass->HasAnyClassFlags(CLASS_Placeable) && !CurClass->HasAnyClassFlags(CLASS_Abstract) )
					{
						TreeCtrl->SetItemBold( Wk );
					}
					else
					{
						TreeCtrl->SetItemTextColour( Wk, wxColour(96,96,96) );
					}

					const UBOOL bHasChildren = HasChildren( ChildTree, bPlaceableOnly );
					TreeCtrl->SetItemHasChildren( Wk, bHasChildren == TRUE );
				}

				// Clean up.
				delete MaskedTree;
			}
		}

		TreeCtrl->SortChildren( InID );
	}
}

void WxActorBrowser::OnFileOpen( wxCommandEvent& In )
{
	WxFileDialog OpenFileDialog( this, 
		*LocalizeUnrealEd("OpenPackage"), 
		*appScriptOutputDir(),
		TEXT(""),
		TEXT("Class Packages (*.u)|*.u|All Files|*.*\0\0"),
		wxOPEN | wxFILE_MUST_EXIST | wxHIDE_READONLY | wxMULTIPLE,
		wxDefaultPosition);

	if( OpenFileDialog.ShowModal() == wxID_OK )
	{
		wxArrayString	OpenFilePaths;
		OpenFileDialog.GetPaths(OpenFilePaths);

		for(UINT FileIndex = 0;FileIndex < OpenFilePaths.Count();FileIndex++)
		{
			UObject::LoadPackage( NULL, *FString(OpenFilePaths[FileIndex]), 0 );
		}
	}

	GCallbackEvent->Send( CALLBACK_RefreshEditor_AllBrowsers );
	Update();
}

void WxActorBrowser::OnFileExportAll( wxCommandEvent& In )		
{
	if( ::MessageBox( (HWND)GetHandle(), *LocalizeUnrealEd("Prompt_18"), *LocalizeUnrealEd("Prompt_19"), MB_YESNO) == IDYES)
	{
		GUnrealEd->Exec( TEXT("CLASS SPEW") );
	}
}

void WxActorBrowser::OnItemExpanding( wxCommandEvent& In )
{
	wxTreeEvent* TreeEvent = static_cast<wxTreeEvent*>(&In);
	TreeCtrl->DeleteChildren( TreeEvent->GetItem() );
	AddChildren( TreeEvent->GetItem() );
}

void WxActorBrowser::OnSelChanged( wxCommandEvent& In )
{
	wxTreeEvent* TreeEvent = static_cast<wxTreeEvent*>(&In);
	wxTreeItemId SelectedItem  = TreeEvent->GetItem();

	if( SelectedItem.IsOk() )
	{	
		WxTreeObjectWrapper* ItemData = (WxTreeObjectWrapper*) TreeCtrl->GetItemData( SelectedItem );
		if( ItemData )
		{
			GUnrealEd->SetCurrentClass( ItemData->GetObject<UClass>() );
		}
	}

	UClass* SelectedClass = GEditor->GetSelectedObjects()->GetTop<UClass>();
	if( SelectedClass != NULL )
	{
		FullPathStatic->SetLabel( *SelectedClass->GetPathName() );
	}
}

void WxActorBrowser::OnItemRightClicked( wxCommandEvent& In )
{
	wxTreeEvent* TreeEvent = static_cast<wxTreeEvent*>(&In);
	wxTreeItemId SelectedItem  = TreeEvent->GetItem();

	if( SelectedItem.IsOk() )
	{	
		WxTreeObjectWrapper* ItemData = (WxTreeObjectWrapper*) TreeCtrl->GetItemData( SelectedItem );
		if( ItemData )
		{
			RightClickedClass = ItemData->GetObject<UClass>();

			class WxMBActorBrowserContext : public wxMenu
			{
			public:
				WxMBActorBrowserContext()
				{
					Append(IDMN_ActorBrowser_CreateArchetype,*LocalizeUnrealEd("Archetype_Create"),TEXT(""));
					Append(IDMN_ActorBrowser_ViewSource,*LocalizeUnrealEd("ViewSource"),TEXT(""));
				};
			} Menu;

			FTrackPopupMenu tpm( this, &Menu );
			tpm.Show();
		}
	}
}

void WxActorBrowser::OnViewSource( wxCommandEvent& In )
{
	FString Script = RightClickedClass->ScriptText->Text;
	TCHAR FilenameBuffer[1024];
	appCreateTempFilename( *GSys->CachePath, FilenameBuffer, ARRAY_COUNT(FilenameBuffer) );

	FArchive* Archive;
	Archive = GFileManager->CreateFileWriter( FilenameBuffer );

	if( Archive )
	{
		*Archive << Script;

		Archive->Close();
		delete Archive;

		appCreateProc( TEXT("notepad.exe"), FilenameBuffer );
	}
}

/** Creates an archteype from the actor class selected in the Actor Browser. */
void WxActorBrowser::OnCreateArchetype(wxCommandEvent& In )
{
	if( RightClickedClass->IsChildOf( AActor::StaticClass() ) )
	{
		FString ArchetypeName, PackageName, GroupName;
		AActor* Actor = GWorld->SpawnActor( RightClickedClass );
		GUnrealEd->Archetype_CreateFromObject( Actor, ArchetypeName, PackageName, GroupName );
		GWorld->EditorDestroyActor( Actor, FALSE );
		UObject::CollectGarbage( GARBAGE_COLLECTION_KEEPFLAGS );
	}
}

void WxActorBrowser::OnUseActorAsParent( wxCommandEvent& In )
{
	UpdateActorList();
}

void WxActorBrowser::OnPlaceableClassesOnly( wxCommandEvent& In )
{
	UpdateActorList();
}
