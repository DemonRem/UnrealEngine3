/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "DlgActorFactory.h"
#include "DlgActorSearch.h"
#include "DlgBuildProgress.h"
#include "DlgTipOfTheDay.h"
#include "LevelViewportToolBar.h"
#include "MainToolBar.h"
#include "UnLinkedObjEditor.h"
#include "FileHelpers.h"
#include "Kismet.h"
#include "MaterialInstanceConstantEditor.h"
#include "MaterialInstanceTimeVaryingEditor.h"
#include "EngineMaterialClasses.h"
#include "LevelBrowser.h"
#include "GenericBrowser.h"
#include "UnTexAlignTools.h"
#include "BusyCursor.h"
#include "LightingBuildOptions.h"
#include "ViewportsContainer.h"
#include "ScopedTransaction.h"
#include "LevelUtils.h"
#include "..\..\engine\inc\EngineSequenceClasses.h"
#include "..\..\engine\inc\EnginePhysicsClasses.h"
#include "..\..\engine\inc\EnginePrefabClasses.h"
#include "..\..\engine\inc\EngineAnimClasses.h"
#include "EngineDecalClasses.h"
#include "..\..\Launch\Resources\resource.h"
#include "SurfaceIterators.h"

#if WITH_FACEFX
	#include "../../../External/FaceFX/Studio/Main/Inc/FxStudioApp.h"
#endif // WITH_FACEFX

////////////////////////////////////////////////////////////////////////////////////////////

/**
* Menu for controlling options related to the rotation grid.
*/
class WxScaleGridMenu : public wxMenu
{
public:
	WxScaleGridMenu()
	{
		AppendCheckItem( IDM_DRAG_GRID_SNAPSCALE, *LocalizeUnrealEd("SnapScaling"), *LocalizeUnrealEd("ToolTip_SnapScaling") );
		AppendSeparator();
		AppendCheckItem( IDM_SCALE_GRID_001, *LocalizeUnrealEd("1Percent"), TEXT("") );
		AppendCheckItem( IDM_SCALE_GRID_002, *LocalizeUnrealEd("2Percent"), TEXT("") );
		AppendCheckItem( IDM_SCALE_GRID_005, *LocalizeUnrealEd("5Percent"), TEXT("") );
		AppendCheckItem( IDM_SCALE_GRID_010, *LocalizeUnrealEd("10Percent"), TEXT("") );
		AppendCheckItem( IDM_SCALE_GRID_025, *LocalizeUnrealEd("25Percent"), TEXT("") );
		AppendCheckItem( IDM_SCALE_GRID_050, *LocalizeUnrealEd("50Percent"), TEXT("") );
	}
};


/**
 * Menu for controlling options related to the rotation grid.
 */
class WxRotationGridMenu : public wxMenu
{
public:
	WxRotationGridMenu()
	{
		AppendCheckItem( IDM_ROTATION_GRID_TOGGLE, *LocalizeUnrealEd("UseRotationGrid"), *LocalizeUnrealEd("ToolTip_139") );
		AppendSeparator();
		AppendCheckItem( IDM_ROTATION_GRID_512, *LocalizeUnrealEd("2Degrees"), TEXT("") );
		AppendCheckItem( IDM_ROTATION_GRID_1024, *LocalizeUnrealEd("5Degrees"), TEXT("") );
		AppendCheckItem( IDM_ROTATION_GRID_4096, *LocalizeUnrealEd("22Degrees"), TEXT("") );
		AppendCheckItem( IDM_ROTATION_GRID_8192, *LocalizeUnrealEd("45Degrees"), TEXT("") );
		AppendCheckItem( IDM_ROTATION_GRID_16384, *LocalizeUnrealEd("90Degrees"), TEXT("") );
	}
};



/**
* Menu for controlling options related to the autosave interval
*/
class WxAutoSaveIntervalMenu : public wxMenu
{
public:
	WxAutoSaveIntervalMenu()
	{
		AppendCheckItem( IDM_AUTOSAVE_001, *LocalizeUnrealEd("AutoSaveInterval1"), TEXT("") );
		AppendCheckItem( IDM_AUTOSAVE_002, *LocalizeUnrealEd("AutoSaveInterval2"), TEXT("") );
		AppendCheckItem( IDM_AUTOSAVE_003, *LocalizeUnrealEd("AutoSaveInterval3"), TEXT("") );
		AppendCheckItem( IDM_AUTOSAVE_004, *LocalizeUnrealEd("AutoSaveInterval4"), TEXT("") );
		AppendCheckItem( IDM_AUTOSAVE_005, *LocalizeUnrealEd("AutoSaveInterval5"), TEXT("") );
	}
};

/**
 * Menu for controlling options related to the drag grid.
 */
class WxDragGridMenu : public wxMenu
{
public:
	WxDragGridMenu()
	{
		AppendCheckItem( IDM_DRAG_GRID_TOGGLE, *LocalizeUnrealEd("UseDragGrid"), *LocalizeUnrealEd("ToolTip_138") );
		AppendSeparator();
		AppendCheckItem( IDM_DRAG_GRID_1, *LocalizeUnrealEd("1"), TEXT("") );
		AppendCheckItem( IDM_DRAG_GRID_2, *LocalizeUnrealEd("2"), TEXT("") );
		AppendCheckItem( IDM_DRAG_GRID_4, *LocalizeUnrealEd("4"), TEXT("") );
		AppendCheckItem( IDM_DRAG_GRID_8, *LocalizeUnrealEd("8"), TEXT("") );
		AppendCheckItem( IDM_DRAG_GRID_16, *LocalizeUnrealEd("16"), TEXT("") );
		AppendCheckItem( IDM_DRAG_GRID_32, *LocalizeUnrealEd("32"), TEXT("") );
		AppendCheckItem( IDM_DRAG_GRID_64, *LocalizeUnrealEd("64"), TEXT("") );
		AppendCheckItem( IDM_DRAG_GRID_128, *LocalizeUnrealEd("128"), TEXT("") );
		AppendCheckItem( IDM_DRAG_GRID_256, *LocalizeUnrealEd("256"), TEXT("") );
		AppendCheckItem( IDM_DRAG_GRID_512, *LocalizeUnrealEd("512"), TEXT("") );
		AppendCheckItem( IDM_DRAG_GRID_1024, *LocalizeUnrealEd("1024"), TEXT("") );
	}
};

/*-----------------------------------------------------------------------------
	WxMainMenu.
-----------------------------------------------------------------------------*/

/**
 * The menu that sits at the top of the main editor frame.
 */
class WxMainMenu : public wxMenuBar
{
private:
	wxMenu		*FileMenu;
	wxMenu		*ImportMenu;
	wxMenu		*ExportMenu;
	wxMenu		*EditMenu;
	wxMenu		*ViewMenu;
	wxMenu		*BrowserMenu;
	wxMenu		*ViewportConfigMenu;
	wxMenu		*BrushMenu;
	wxMenu		*VolumeMenu;
	wxMenu		*BuildMenu;
	wxMenu		*ToolsMenu;
	wxMenu		*HelpMenu;

public:
	FMRUList	*MRUFiles;
	wxMenu		*MRUMenu;
	wxMenu		*ConsoleMenu[20]; // @todo: put a define somewhere for number of console, and num menu items per console

	WxMainMenu()
	{
		// Allocations

		FileMenu = new wxMenu();
		MRUMenu = new wxMenu();
		ImportMenu = new wxMenu();
		ExportMenu = new wxMenu();
		EditMenu = new wxMenu();
		ViewMenu = new wxMenu();
		BrowserMenu = new wxMenu();
		ViewportConfigMenu = new wxMenu();
		BrushMenu = new wxMenu();
		BuildMenu = new wxMenu();
		ToolsMenu = new wxMenu();
		HelpMenu = new wxMenu();
		VolumeMenu = new wxMenu();

		// File menu
		{
			// Popup Menus
			{
				ImportMenu->Append( IDM_IMPORT_NEW, *LocalizeUnrealEd("IntoNewMapE"), *LocalizeUnrealEd("ToolTip_76") );
				ImportMenu->Append( IDM_IMPORT_MERGE, *LocalizeUnrealEd("IntoExistingMapE"), *LocalizeUnrealEd("ToolTip_77") );

				ExportMenu->Append( IDM_EXPORT_ALL, *LocalizeUnrealEd("AllE"), *LocalizeUnrealEd("ToolTip_78") );
				ExportMenu->Append( IDM_EXPORT_SELECTED, *LocalizeUnrealEd("SelectedOnlyE"), *LocalizeUnrealEd("ToolTip_79") );
			}

			FileMenu->Append( IDM_NEW, *LocalizeUnrealEd("&New"), *LocalizeUnrealEd("ToolTip_80") );
			FileMenu->Append( IDM_OPEN, *LocalizeUnrealEd("&OpenE"), *LocalizeUnrealEd("ToolTip_81") );
			FileMenu->AppendSeparator();
			FileMenu->Append( IDM_SAVE, *LocalizeUnrealEd("&SaveCurrentLevel"), *LocalizeUnrealEd("ToolTip_82") );
			FileMenu->Append( IDM_SAVE_AS, *LocalizeUnrealEd("Save&AsE"), *LocalizeUnrealEd("ToolTip_83") );
			FileMenu->Append( IDM_SAVE_ALL, *LocalizeUnrealEd("SaveA&ll"), *LocalizeUnrealEd("ToolTip_84") );
			FileMenu->Append( IDM_FORCE_SAVE_ALL, *LocalizeUnrealEd("ForceSaveAll"), *LocalizeUnrealEd("ToolTip_ForceSaveAll") );
			//FileMenu->AppendSeparator();
			//FileMenu->Append( IDM_SAVE_ALL_LEVELS, *LocalizeUnrealEd("SaveAllLevels"), *LocalizeUnrealEd("ToolTip_82") );
			FileMenu->AppendSeparator();
			FileMenu->Append( IDM_IMPORT, *LocalizeUnrealEd("&Import"), ImportMenu );
			FileMenu->Append( IDM_EXPORT, *LocalizeUnrealEd("&Export"), ExportMenu );
			//		FileMenu->Append( IDM_CREATEARCHETYPE, TEXT("Save As Archetype...") );
			FileMenu->AppendSeparator();
			FileMenu->Append( IDM_MRU_LIST, *LocalizeUnrealEd("&Recent"), MRUMenu );
			FileMenu->AppendSeparator();
			FileMenu->Append( IDM_EXIT, *LocalizeUnrealEd("E&xit"), *LocalizeUnrealEd("ToolTip_85") );

			Append( FileMenu, *LocalizeUnrealEd("File") );
		}

		// Edit menu
		{
			EditMenu->Append( IDM_UNDO, *LocalizeUnrealEd("Undo"), *LocalizeUnrealEd("ToolTip_86") );
			EditMenu->Append( IDM_REDO, *LocalizeUnrealEd("Redo"), *LocalizeUnrealEd("ToolTip_87") );
			EditMenu->AppendSeparator();
			EditMenu->AppendCheckItem( ID_EDIT_SHOW_WIDGET, *LocalizeUnrealEd("ShowWidget"), *LocalizeUnrealEd("ToolTip_88") );
			EditMenu->AppendCheckItem( ID_EDIT_TRANSLATE, *LocalizeUnrealEd("Translate"), *LocalizeUnrealEd("ToolTip_89") );
			EditMenu->AppendCheckItem( ID_EDIT_ROTATE, *LocalizeUnrealEd("Rotate"), *LocalizeUnrealEd("ToolTip_90") );
			EditMenu->AppendCheckItem( ID_EDIT_SCALE, *LocalizeUnrealEd("Scale"), *LocalizeUnrealEd("ToolTip_91") );
			EditMenu->AppendSeparator();
			EditMenu->Append( IDM_SEARCH, *LocalizeUnrealEd("Search"), *LocalizeUnrealEd("ToolTip_92") );
			EditMenu->AppendSeparator();
			EditMenu->Append( IDM_CUT, *LocalizeUnrealEd("Cut"), *LocalizeUnrealEd("ToolTip_93") );
			EditMenu->Append( IDM_COPY, *LocalizeUnrealEd("Copy"), *LocalizeUnrealEd("ToolTip_94") );
			EditMenu->Append( IDM_PASTE, *LocalizeUnrealEd("Paste"), *LocalizeUnrealEd("ToolTip_95") );
			EditMenu->Append( IDM_DUPLICATE, *LocalizeUnrealEd("Duplicate"), *LocalizeUnrealEd("ToolTip_96") );
			EditMenu->AppendSeparator();
			EditMenu->Append( IDM_DELETE, *LocalizeUnrealEd("Delete"), *LocalizeUnrealEd("ToolTip_97") );
			EditMenu->AppendSeparator();
			EditMenu->Append( IDM_SELECT_NONE, *LocalizeUnrealEd("SelectNone"), *LocalizeUnrealEd("ToolTip_98") );
			EditMenu->Append( IDM_SELECT_ALL, *LocalizeUnrealEd("SelectAll"), *LocalizeUnrealEd("ToolTip_99") );
			EditMenu->Append( IDM_SELECT_INVERT, *LocalizeUnrealEd("InvertSelections"), *LocalizeUnrealEd("ToolTip_100") );
			EditMenu->Append( IDM_SELECT_ByProperty, *LocalizeUnrealEd("SelectByProperty"), *LocalizeUnrealEd("ToolTip_SelectByProperty") );

			Append( EditMenu, *LocalizeUnrealEd("Edit") );
		}

		// View menu
		{
			// Popup Menus
			{
				GUnrealEd->GetBrowserManager()->InitializeBrowserMenu(BrowserMenu);

				BrowserMenu->AppendSeparator();
				BrowserMenu->Append( IDM_OPEN_KISMET, *LocalizeUnrealEd("Kismet"), *LocalizeUnrealEd("ToolTip_107") );

				for( INT x = 0 ; x < GApp->EditorFrame->ViewportConfigTemplates.Num() ; ++x )
				{
					FViewportConfig_Template* vct = GApp->EditorFrame->ViewportConfigTemplates(x);
					ViewportConfigMenu->AppendCheckItem( IDM_VIEWPORT_CONFIG_START+x, *(vct->Desc), *FString::Printf( *LocalizeUnrealEd("ToolTip_108"), *(vct->Desc) ) );
				}
			}

			ViewMenu->Append( IDM_BROWSER, *LocalizeUnrealEd("BrowserWindows"), BrowserMenu );
			ViewMenu->AppendSeparator();
			ViewMenu->Append( IDM_ACTOR_PROPERTIES, *LocalizeUnrealEd("ActorProperties"), *LocalizeUnrealEd("ToolTip_109") );
			ViewMenu->Append( IDM_SURFACE_PROPERTIES, *LocalizeUnrealEd("SurfaceProperties"), *LocalizeUnrealEd("ToolTip_110") );
			ViewMenu->Append( IDM_WORLD_PROPERTIES, *LocalizeUnrealEd("WorldProperties"), *LocalizeUnrealEd("ToolTip_111") );
			ViewMenu->AppendSeparator();
			ViewMenu->Append( IDM_DRAG_GRID, *LocalizeUnrealEd("DragGrid"), GApp->EditorFrame->GetDragGridMenu() );
			ViewMenu->Append( IDM_ROTATION_GRID, *LocalizeUnrealEd("RotationGrid"), GApp->EditorFrame->GetRotationGridMenu() );
			ViewMenu->Append( IDM_AUTOSAVE_INTERVAL, *LocalizeUnrealEd("AutoSaveInterval"), GApp->EditorFrame->GetAutoSaveIntervalMenu() );
			ViewMenu->Append( IDM_SCALE_GRID, *LocalizeUnrealEd("ScaleGrid"), GApp->EditorFrame->GetScaleGridMenu() );
			ViewMenu->Append( IDM_VIEWPORT_CONFIG, *LocalizeUnrealEd("ViewportConfiguration"), ViewportConfigMenu );
			ViewMenu->AppendCheckItem( IDM_VIEWPORT_RESIZE_TOGETHER, *LocalizeUnrealEd("ResizeTopAndBottomViewportsTogether"), *LocalizeUnrealEd("ToolTip_ResizeTopAndBottomViewportsTogether"));
			ViewMenu->AppendSeparator();
			ViewMenu->AppendCheckItem( IDM_BRUSHPOLYS, *LocalizeUnrealEd("BrushMarkerPolys"), *LocalizeUnrealEd("ToolTip_112") );
			ViewMenu->AppendSeparator();
			ViewMenu->AppendCheckItem( IDM_TogglePrefabsLocked, *LocalizeUnrealEd("TogglePrefabLock"), *LocalizeUnrealEd("TogglePrefabLock") );
			ViewMenu->AppendSeparator();
			ViewMenu->AppendCheckItem( IDM_FULLSCREEN, *LocalizeUnrealEd("Fullscreen"), *LocalizeUnrealEd("ToolTip_113") );

			Append( ViewMenu, *LocalizeUnrealEd("View") );
		}

		// Brush menu
		{
			BrushMenu->Append( IDM_BRUSH_ADD, *LocalizeUnrealEd("Add"), *LocalizeUnrealEd("ToolTip_114") );
			BrushMenu->Append( IDM_BRUSH_SUBTRACT, *LocalizeUnrealEd("Subtract"), *LocalizeUnrealEd("ToolTip_115") );
			BrushMenu->Append( IDM_BRUSH_INTERSECT, *LocalizeUnrealEd("Intersect"), *LocalizeUnrealEd("ToolTip_116") );
			BrushMenu->Append( IDM_BRUSH_DEINTERSECT, *LocalizeUnrealEd("Deintersect"), *LocalizeUnrealEd("ToolTip_117") );
			BrushMenu->AppendSeparator();
			BrushMenu->Append( IDM_BRUSH_ADD_MOVER, *LocalizeUnrealEd("AddMover"), *LocalizeUnrealEd("ToolTip_118") );
			BrushMenu->Append( IDM_BRUSH_ADD_SPECIAL, *LocalizeUnrealEd("AddSpecial"), *LocalizeUnrealEd("ToolTip_120") );
			BrushMenu->AppendSeparator();
			BrushMenu->Append( ID_BRUSH_IMPORT, *LocalizeUnrealEd("ImportE"), *LocalizeUnrealEd("ToolTip_121") );
			BrushMenu->Append( ID_BRUSH_EXPORT, *LocalizeUnrealEd("ExportE"), *LocalizeUnrealEd("ToolTip_122") );

			Append( BrushMenu, *LocalizeUnrealEd("Brush") );

			// Volume menu
			{
				BrushMenu->AppendSeparator();

				// Get sorted array of volume classes then create a menu item for each one
				TArray< UClass* > VolumeClasses;

				GApp->EditorFrame->GetSortedVolumeClasses( &VolumeClasses );

				INT ID = IDM_VolumeClasses_START;

				for( INT VolumeIdx = 0; VolumeIdx < VolumeClasses.Num(); VolumeIdx++ )
				{
					VolumeMenu->Insert( 0, ID, *VolumeClasses( VolumeIdx )->GetName(), TEXT(""), 0 );
					ID++;
				}

				BrushMenu->Append( IDMENU_ActorPopupVolumeMenu, *LocalizeUnrealEd("AddVolumePopUp"), VolumeMenu );
			}
		}

		// Build menu
		{
			BuildMenu->Append( IDM_BUILD_GEOMETRY, *LocalizeUnrealEd("Geometry"), *LocalizeUnrealEd("ToolTip_123") );
			BuildMenu->Append( IDM_BUILD_VISIBLEGEOMETRY, *LocalizeUnrealEd("BuildVisibleGeometry"), *LocalizeUnrealEd("BuildVisibleGeometry_HelpText"));
			BuildMenu->Append( IDM_BUILD_LIGHTING, *LocalizeUnrealEd("Lighting"), *LocalizeUnrealEd("ToolTip_124") );
			BuildMenu->Append( IDM_BUILD_AI_PATHS, *LocalizeUnrealEd("AIPaths"), *LocalizeUnrealEd("ToolTip_125") );
			if (appStricmp(appGetGameName(),TEXT("War")) == 0)
			{
				BuildMenu->Append( IDM_BUILD_COVER, *LocalizeUnrealEd("BuildCover"), *LocalizeUnrealEd("ToolTip_126") );
			}
			BuildMenu->AppendSeparator();
			BuildMenu->Append( IDM_BUILD_ALL, *LocalizeUnrealEd("BuildAll"), *LocalizeUnrealEd("ToolTip_127") );

			BuildMenu->AppendSeparator();
			// if we have any console plugins, add them to the list of places we can play the level
			if (GConsoleSupportContainer->GetNumConsoleSupports() > 0)
			{
				// make a new submenu (reusing a pointer here, icky)
				ConsoleMenu[0] = new wxMenu();

				// we always can play in the editor
				ConsoleMenu[0]->Append(IDM_BuildPlayInEditor, *LocalizeUnrealEd("InEditor"), *LocalizeUnrealEd("ToolTip_128"));
				// loop through all consoles (only support 20 consoles)
				INT ConsoleIndex = 0;
				for (FConsoleSupportIterator It; It && ConsoleIndex < 20; ++It, ConsoleIndex++)
				{
					// add a per-console Play On XXX menu
					ConsoleMenu[0]->Append(
						IDM_BuildPlayConsole_START + ConsoleIndex, 
						*FString::Printf(*LocalizeUnrealEd("OnF"), It->GetConsoleName()), 
						*FString::Printf(*LocalizeUnrealEd("ToolTip_129"), It->GetConsoleName())
						);
				}
				// stick this submenu in the menu
				BuildMenu->Append(IDM_BuildPlayConsoleMenu, *LocalizeUnrealEd("ToolTip_130"), ConsoleMenu[0]);
			}
			else
			{
				// if there are no patforms, then just put the play in editor menu item in the base Build menu
				BuildMenu->Append(IDM_BuildPlayInEditor, *LocalizeUnrealEd("PlayLevel"), *LocalizeUnrealEd("ToolTip_128"));
			}

			Append( BuildMenu, *LocalizeUnrealEd("Build") );

			// Add any console-specific menus (max out at 20 consoles)
			INT ConsoleIndex = 0;
			for (FConsoleSupportIterator It; It && ConsoleIndex < 20; ++It, ConsoleIndex++)
			{
				// If this console has any menu items, add the menu
				if (It->GetNumMenuItems() > 0)
				{
					// make a new menu
					ConsoleMenu[ConsoleIndex] = new wxMenu();

					// Add all the items (max out at 40 items)
					for (INT ItemIndex = 0; ItemIndex < It->GetNumMenuItems() && ConsoleIndex < 40; ItemIndex++)
					{
						bool bIsChecked, bIsRadio;
						const TCHAR* MenuLabel = It->GetMenuItem(ItemIndex, bIsChecked, bIsRadio);
						// @todo: when wx supports radio menu items properly, use this for better feedback to user
						// wxItemKind Kind = bIsRadio ? wxITEM_RADIO : wxITEM_CHECK;
						wxItemKind Kind = wxITEM_CHECK;

						// if it's the special separator text, append a separator, not a real menu item
						if (appStricmp(MenuLabel, MENU_SEPARATOR_LABEL) == 0)
						{
							ConsoleMenu[ConsoleIndex]->AppendSeparator();
						}
						else
						{
							ConsoleMenu[ConsoleIndex]->Append(IDM_ConsoleSpecific_START + ConsoleIndex * MAX_CONSOLES_TO_DISPLAY_IN_MENU + ItemIndex, MenuLabel, TEXT(""), Kind);
							ConsoleMenu[ConsoleIndex]->Check(IDM_ConsoleSpecific_START + ConsoleIndex * MAX_CONSOLES_TO_DISPLAY_IN_MENU + ItemIndex, bIsChecked);
						}
					}
					// Put the new menu on the main menu bar
					Append(ConsoleMenu[ConsoleIndex], It->GetConsoleName());
				}
			}
		}

		// Tools menu
		{
			ToolsMenu->Append( IDM_WIZARD_NEW_TERRAIN, *LocalizeUnrealEd("NewTerrainE"), *LocalizeUnrealEd("ToolTip_132") );
			ToolsMenu->Append( IDMN_TOOL_CHECK_ERRORS, *LocalizeUnrealEd("CheckMapForErrorsE"), *LocalizeUnrealEd("ToolTip_133") );
			ToolsMenu->AppendSeparator();
			ToolsMenu->Append( IDM_CleanBSPMaterials, *LocalizeUnrealEd("CleanBSPMaterials"), TEXT("") );
			ToolsMenu->AppendSeparator();
			ToolsMenu->Append( IDM_REPLACESKELMESHACTORS, *LocalizeUnrealEd("ReplaceSkeletalMeshActors"), TEXT("") );

			Append( ToolsMenu, *LocalizeUnrealEd("Tools") );
		}

		// Help Menu
		{
			HelpMenu->Append( IDMENU_ABOUTBOX, *LocalizeUnrealEd("AboutUnrealEdE"), *LocalizeUnrealEd("ToolTip_134") );
			HelpMenu->Append( IDMENU_ONLINEHELP, *LocalizeUnrealEd("OnlineHelp"), *LocalizeUnrealEd("ToolTip_OnlineHelp") );
			HelpMenu->Append( IDMENU_TIPOFTHEDAY, *LocalizeUnrealEd("DlgTipOfTheDay_TipOfTheDay"), *LocalizeUnrealEd("DlgTipOfTheDay_HelpText_TipOfTheDay") );

			Append( HelpMenu, *LocalizeUnrealEd("Help") );
		}

		// MRU list

		MRUFiles = new FMRUList( *LocalizeUnrealEd("MRU"), MRUMenu );
	}

	~WxMainMenu()
	{
		MRUFiles->WriteINI();
		delete MRUFiles;
	}
};

/*-----------------------------------------------------------------------------
	WxDlgLightingBuildOptions
-----------------------------------------------------------------------------*/

/**
 * Dialog window for setting lighting build options. Values are read from the ini and stored again if the user
 * presses OK.
 */
class WxDlgLightingBuildOptions : public wxDialog
{
public:
	/**
	 * Default constructor, initializing dialog.
	 */
	WxDlgLightingBuildOptions();

	/** Checkbox for BSP setting */
	wxCheckBox*	BuildBSPCheckBox;
	/** Checkbox for static mesh setting */
	wxCheckBox*	BuildStaticMeshesCheckBox;
	/** Checkbox for selected actors setting */
	wxCheckBox*	BuildOnlySelectedActorsCheckBox;
	/** Checkbox for current level setting */
	wxCheckBox*	BuildOnlyCurrentLevelCheckBox;
	/** Checkbox for changed lighting setting */
	wxCheckBox*	BuildOnlyChangedLightingCheckBox;
	/** Checkbox for building only levels selected in the level browser. */
	wxCheckBox*	BuildOnlySelectedLevelsCheckBox;
	/** Checkbox for performing full quality build. */
	wxCheckBox* PerformFullQualityBuildCheckBox;

	/**
	 * Shows modal dialog populating default Options by reading from ini. The code will store the newly set options
	 * back to the ini if the user presses OK.
	 *
	 * @param	Options		Lighting rebuild options to set.
	 */
	UBOOL ShowModal( FLightingBuildOptions& Options );

	/**
	 * Route OK to wxDialog.
	 */
	void OnOK( wxCommandEvent& In ) 
	{
		wxDialog::OnOK( In );
	}

	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(WxDlgLightingBuildOptions, wxDialog)
	EVT_BUTTON( wxID_OK, WxDlgLightingBuildOptions::OnOK )
END_EVENT_TABLE()

/**
 * Default constructor, creating dialog.
 */
WxDlgLightingBuildOptions::WxDlgLightingBuildOptions()
{
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
	wxDialog::Create( GApp->EditorFrame, wxID_ANY, *LocalizeUnrealEd(TEXT("LBO_LightingBuildOptions")), wxDefaultPosition, wxDefaultSize, wxCAPTION|wxDIALOG_MODAL|wxTAB_TRAVERSAL );

	wxBoxSizer* BoxSizer2 = new wxBoxSizer(wxVERTICAL);
	SetSizer(BoxSizer2);

	wxStaticBox* StaticBoxSizer3Static = new wxStaticBox(this, wxID_ANY, *LocalizeUnrealEd(TEXT("LBO_LightingBuildOptions")));
	wxStaticBoxSizer* StaticBoxSizer3 = new wxStaticBoxSizer(StaticBoxSizer3Static, wxVERTICAL);
	BoxSizer2->Add(StaticBoxSizer3, 1, wxGROW|wxALL, 5);

	BuildBSPCheckBox = new wxCheckBox( this, wxID_ANY, *LocalizeUnrealEd(TEXT("LBO_BuildBSP")), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
	BuildBSPCheckBox->SetValue(TRUE);
	StaticBoxSizer3->Add(BuildBSPCheckBox, 0, wxGROW|wxALL, 5);

	BuildStaticMeshesCheckBox = new wxCheckBox( this, wxID_ANY, *LocalizeUnrealEd(TEXT("LBO_BuildStaticMeshes")), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
	BuildStaticMeshesCheckBox->SetValue(TRUE);
	StaticBoxSizer3->Add(BuildStaticMeshesCheckBox, 0, wxGROW|wxALL, 5);

	BuildOnlySelectedActorsCheckBox = new wxCheckBox( this, wxID_ANY, *LocalizeUnrealEd(TEXT("LBO_BuildOnlySelectedActors")), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
	BuildOnlySelectedActorsCheckBox->SetValue(FALSE);
	StaticBoxSizer3->Add(BuildOnlySelectedActorsCheckBox, 0, wxGROW|wxALL, 5);

	BuildOnlyCurrentLevelCheckBox = new wxCheckBox( this, wxID_ANY, *LocalizeUnrealEd(TEXT("LBO_BuildOnlyCurrentLevel")), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
	BuildOnlyCurrentLevelCheckBox->SetValue(FALSE);
	StaticBoxSizer3->Add(BuildOnlyCurrentLevelCheckBox, 0, wxGROW|wxALL, 5);

	BuildOnlyChangedLightingCheckBox = new wxCheckBox( this, wxID_ANY, *LocalizeUnrealEd(TEXT("LBO_BuildOnlyChangedLighting")), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
	BuildOnlyChangedLightingCheckBox->SetValue(FALSE);
	StaticBoxSizer3->Add(BuildOnlyChangedLightingCheckBox, 0, wxGROW|wxALL, 5);

	BuildOnlySelectedLevelsCheckBox = new wxCheckBox( this, wxID_ANY, *LocalizeUnrealEd(TEXT("LBO_BuildOnlySelectedLevels")), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
	BuildOnlySelectedLevelsCheckBox->SetValue(FALSE);
	StaticBoxSizer3->Add(BuildOnlySelectedLevelsCheckBox, 0, wxGROW|wxALL, 5);

	PerformFullQualityBuildCheckBox = new wxCheckBox( this, wxID_ANY, *LocalizeUnrealEd(TEXT("LBO_FullQualityBuild")), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
	PerformFullQualityBuildCheckBox->SetValue(TRUE);
	StaticBoxSizer3->Add(PerformFullQualityBuildCheckBox, 0, wxGROW|wxALL, 5);

	wxBoxSizer* BoxSizer9 = new wxBoxSizer(wxHORIZONTAL);
	BoxSizer2->Add(BoxSizer9, 0, wxGROW|wxALL, 5);

	wxButton* Button10 = new wxButton( this, wxID_OK, *LocalizeUnrealEd(TEXT("&OK")) );
	Button10->SetDefault();
	BoxSizer9->Add(Button10, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

	wxButton* Button11 = new wxButton( this, wxID_CANCEL, *LocalizeUnrealEd(TEXT("&Cancel")) );
	BoxSizer9->Add(Button11, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);
}

/**
 * Shows modal dialog populating default Options by reading from ini. The code will store the newly set options
 * back to the ini if the user presses OK.
 *
 * @param	Options		Lighting rebuild options to set.
 * @return				TRUE if the lighting rebuild should go ahead.
 */
UBOOL WxDlgLightingBuildOptions::ShowModal( FLightingBuildOptions& Options )
{
	// Retrieve values from ini.
	GConfig->GetBool( TEXT("LightingBuildOptions"), TEXT("OnlyBuildSelectedActors"),Options.bOnlyBuildSelectedActors,	GEditorIni );
	GConfig->GetBool( TEXT("LightingBuildOptions"), TEXT("OnlyBuildCurrentLevel"),	Options.bOnlyBuildCurrentLevel,		GEditorIni );
	GConfig->GetBool( TEXT("LightingBuildOptions"), TEXT("OnlyBuildChanged"),		Options.bOnlyBuildChanged,			GEditorIni );
	GConfig->GetBool( TEXT("LightingBuildOptions"), TEXT("BuildBSP"),				Options.bBuildBSP,					GEditorIni );
	GConfig->GetBool( TEXT("LightingBuildOptions"), TEXT("BuildActors"),			Options.bBuildActors,				GEditorIni );
	GConfig->GetBool( TEXT("LightingBuildOptions"), TEXT("OnlyBuildSelectedLevels"),Options.bOnlyBuildSelectedLevels,	GEditorIni );
	GConfig->GetBool( TEXT("LightingBuildOptions"), TEXT("FullQualityBuild"),		Options.bPerformFullQualityBuild,	GEditorIni );

	// Populate dialog with read in values.
	BuildOnlySelectedActorsCheckBox->SetValue( Options.bOnlyBuildSelectedActors == TRUE );
	BuildOnlyCurrentLevelCheckBox->SetValue( Options.bOnlyBuildCurrentLevel == TRUE );
	BuildOnlyChangedLightingCheckBox->SetValue( Options.bOnlyBuildChanged == TRUE );
	BuildBSPCheckBox->SetValue( Options.bBuildBSP == TRUE );
	BuildStaticMeshesCheckBox->SetValue( Options.bBuildActors == TRUE );
	BuildOnlySelectedLevelsCheckBox->SetValue( Options.bOnlyBuildSelectedLevels == TRUE );
	PerformFullQualityBuildCheckBox->SetValue( Options.bPerformFullQualityBuild == TRUE );

	UBOOL bProceedWithBuild = FALSE;

	// Run dialog...
	if( wxDialog::ShowModal() == wxID_OK )
	{
		bProceedWithBuild = TRUE;

		// ... and retrieve options if user pressed okay.
		Options.bOnlyBuildSelectedActors	= BuildOnlySelectedActorsCheckBox->GetValue();
		Options.bOnlyBuildCurrentLevel		= BuildOnlyCurrentLevelCheckBox->GetValue();
		Options.bOnlyBuildChanged			= BuildOnlyChangedLightingCheckBox->GetValue();
		Options.bBuildBSP					= BuildBSPCheckBox->GetValue();
		Options.bBuildActors				= BuildStaticMeshesCheckBox->GetValue();
		Options.bOnlyBuildSelectedLevels	= BuildOnlySelectedLevelsCheckBox->GetValue();
		Options.bPerformFullQualityBuild	= PerformFullQualityBuildCheckBox->GetValue();

		// If necessary, retrieve a list of levels currently selected in the level browser.
		if ( Options.bOnlyBuildSelectedLevels )
		{
			Options.SelectedLevels.Empty();

			WxLevelBrowser* LevelBrowser = GUnrealEd->GetBrowser<WxLevelBrowser>( TEXT("LevelBrowser") );
			if ( LevelBrowser )
			{
				// Assemble an ignore list from the levels that are currently selected in the level browser.
				for ( WxLevelBrowser::TSelectedLevelIterator It( LevelBrowser->SelectedLevelIterator() ) ; It ; ++It )
				{
					ULevel* Level = *It;
					if ( Level )
					{
						Options.SelectedLevels.AddItem( Level );
					}
				}
			}
			else
			{
				// Level browser wasn't found -- notify the user and abort the build.
				appMsgf( AMT_OK, *LocalizeUnrealEd("LBO_CantFindLevelBrowser") );
				bProceedWithBuild = FALSE;
			}
		}

		// Save options to ini if things are still looking good.
		if ( bProceedWithBuild )
		{
			GConfig->SetBool( TEXT("LightingBuildOptions"), TEXT("OnlyBuildSelectedActors"),Options.bOnlyBuildSelectedActors,	GEditorIni );
			GConfig->SetBool( TEXT("LightingBuildOptions"), TEXT("OnlyBuildCurrentLevel"),	Options.bOnlyBuildCurrentLevel,		GEditorIni );
			GConfig->SetBool( TEXT("LightingBuildOptions"), TEXT("OnlyBuildChanged"),		Options.bOnlyBuildChanged,			GEditorIni );
			GConfig->SetBool( TEXT("LightingBuildOptions"), TEXT("BuildBSP"),				Options.bBuildBSP,					GEditorIni );
			GConfig->SetBool( TEXT("LightingBuildOptions"), TEXT("BuildActors"),			Options.bBuildActors,				GEditorIni );
			GConfig->SetBool( TEXT("LightingBuildOptions"), TEXT("OnlyBuildSelectedLevels"),Options.bOnlyBuildSelectedLevels,	GEditorIni );
			GConfig->SetBool( TEXT("LightingBuildOptions"), TEXT("FullQualityBuild"),		Options.bPerformFullQualityBuild,	GEditorIni );
		}
	}

	return bProceedWithBuild;
}

/*-----------------------------------------------------------------------------
	WxDlgImportBrush.
-----------------------------------------------------------------------------*/

class WxDlgImportBrush : public wxDialog
{
public:
	WxDlgImportBrush()
	{
		const bool bSuccess = wxXmlResource::Get()->LoadDialog( this, GApp->EditorFrame, TEXT("ID_DLG_IMPORTBRUSH") );
		check( bSuccess );

		MergeFacesCheck = (wxCheckBox*)FindWindow( XRCID( "IDCK_MERGEFACES" ) );
		check( MergeFacesCheck != NULL );
		SolidRadio = (wxRadioButton*)FindWindow( XRCID( "IDRB_SOLID" ) );
		check( SolidRadio != NULL );
		NonSolidRadio = (wxRadioButton*)FindWindow( XRCID( "IDRB_NONSOLID" ) );
		check( NonSolidRadio != NULL );

		SolidRadio->SetValue( 1 );

		FWindowUtil::LoadPosSize( TEXT("DlgImportBrush"), this );
		FLocalizeWindow( this );
	}

	~WxDlgImportBrush()
	{
		FWindowUtil::SavePosSize( TEXT("DlgImportBrush"), this );
	}

	int ShowModal( const FString& InFilename )
	{
		Filename = InFilename;
		return wxDialog::ShowModal();
	}

private:
	wxCheckBox *MergeFacesCheck;
	wxRadioButton *SolidRadio, *NonSolidRadio;

	FString Filename;

	void OnOK( wxCommandEvent& In )
	{
		GUnrealEd->Exec(*FString::Printf(TEXT("BRUSH IMPORT FILE=\"%s\" MERGE=%d FLAGS=%d"),
						*Filename,
						MergeFacesCheck->GetValue() ? 1 : 0,
						NonSolidRadio->GetValue() ? PF_NotSolid : 0) );

		GWorld->GetBrush()->Brush->BuildBound();

		wxDialog::OnOK( In );
	}

	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(WxDlgImportBrush, wxDialog)
	EVT_BUTTON( wxID_OK, WxDlgImportBrush::OnOK )
END_EVENT_TABLE()

/*-----------------------------------------------------------------------------
	WxDlgAbout.
-----------------------------------------------------------------------------*/

class WxDlgAbout : public wxDialog
{
public:
	WxDlgAbout()
	{
		const bool bSuccess = wxXmlResource::Get()->LoadDialog( this, GApp->EditorFrame, TEXT("ID_DLG_ABOUT") );
		check( bSuccess );

		Bitmap.LoadLiteral( TEXT("Splash\\EdSplash") );
		BitmapStatic = (wxStaticBitmap*)FindWindow( XRCID( "IDSC_BITMAP" ) );
		check ( BitmapStatic != NULL );
		BitmapStatic->SetBitmap( Bitmap );
		FLocalizeWindow( this );

		VersionStatic = (wxStaticText*)FindWindow( XRCID( "IDSC_VERSION_NUMBER" ) );
		check( VersionStatic != NULL );
		VersionStatic->SetLabel( *FString::Printf( TEXT("%d  BuiltFromChangelist: %d"), GEngineVersion, GBuiltFromChangeList ) );

		FWindowUtil::LoadPosSize( TEXT("DlgAbout"), this );
	}

	~WxDlgAbout()
	{
		FWindowUtil::SavePosSize( TEXT("DlgAbout"), this );
	}

private:
	WxBitmap Bitmap;
	wxStaticBitmap* BitmapStatic;
	wxStaticText* VersionStatic;

	void OnOK( wxCommandEvent& In )
	{
		wxDialog::OnOK( In );
	}

	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(WxDlgAbout, wxDialog)
	EVT_BUTTON( wxID_OK, WxDlgAbout::OnOK )
END_EVENT_TABLE()

////////////////////////////////////////////////////////////////////////////////////////////

extern class WxUnrealEdApp* GApp;

BEGIN_EVENT_TABLE( WxEditorFrame, wxFrame )

	EVT_ICONIZE( WxEditorFrame::OnIconize )
	EVT_MAXIMIZE( WxEditorFrame::OnMaximize )
	EVT_SIZE( WxEditorFrame::OnSize )
	EVT_CLOSE( WxEditorFrame::OnClose )
	EVT_MOVE( WxEditorFrame::OnMove )

	EVT_SPLITTER_SASH_POS_CHANGING( ID_SPLITTERWINDOW, WxEditorFrame::OnSplitterChanging )
	EVT_SPLITTER_SASH_POS_CHANGING( ID_SPLITTERWINDOW+1, WxEditorFrame::OnSplitterChanging )
	EVT_SPLITTER_SASH_POS_CHANGING( ID_SPLITTERWINDOW+2, WxEditorFrame::OnSplitterChanging )
	EVT_SPLITTER_DCLICK( ID_SPLITTERWINDOW, WxEditorFrame::OnSplitterDblClk )
	EVT_SPLITTER_DCLICK( ID_SPLITTERWINDOW+1, WxEditorFrame::OnSplitterDblClk )
	EVT_SPLITTER_DCLICK( ID_SPLITTERWINDOW+2, WxEditorFrame::OnSplitterDblClk )

	EVT_MENU( IDM_NEW, WxEditorFrame::MenuFileNew )
	EVT_MENU( IDM_OPEN, WxEditorFrame::MenuFileOpen )
	EVT_MENU( IDM_SAVE, WxEditorFrame::MenuFileSave )
	EVT_MENU( IDM_SAVE_AS, WxEditorFrame::MenuFileSaveAs )
	EVT_MENU( IDM_SAVE_ALL, WxEditorFrame::MenuFileSaveAll )
	EVT_MENU( IDM_FORCE_SAVE_ALL, WxEditorFrame::MenuFileForceSaveAll )
	EVT_MENU( IDM_SAVE_ALL_LEVELS, WxEditorFrame::MenuFileSaveAllLevels )
	EVT_MENU( IDM_IMPORT_NEW, WxEditorFrame::MenuFileImportNew )
	EVT_MENU( IDM_IMPORT_MERGE, WxEditorFrame::MenuFileImportMerge )
	EVT_MENU( IDM_EXPORT_ALL, WxEditorFrame::MenuFileExportAll )
	EVT_MENU( IDM_EXPORT_SELECTED, WxEditorFrame::MenuFileExportSelected )
	EVT_MENU( IDM_CREATEARCHETYPE, WxEditorFrame::MenuFileCreateArchetype )
	EVT_MENU( IDM_EXIT, WxEditorFrame::MenuFileExit )
	EVT_MENU( IDM_UNDO, WxEditorFrame::MenuEditUndo )
	EVT_MENU( IDM_REDO, WxEditorFrame::MenuEditRedo )
	EVT_SLIDER( ID_FarPlaneSlider, WxEditorFrame::MenuFarPlaneScrollChanged )
	EVT_MENU( ID_EDIT_MOUSE_LOCK, WxEditorFrame::MenuEditMouseLock )
	EVT_MENU( ID_EDIT_SHOW_WIDGET, WxEditorFrame::MenuEditShowWidget )
	EVT_MENU( ID_EDIT_TRANSLATE, WxEditorFrame::MenuEditTranslate )
	EVT_MENU( ID_EDIT_ROTATE, WxEditorFrame::MenuEditRotate )
	EVT_MENU( ID_EDIT_SCALE, WxEditorFrame::MenuEditScale )
	EVT_MENU( ID_EDIT_SCALE_NONUNIFORM, WxEditorFrame::MenuEditScaleNonUniform )
	EVT_COMBOBOX( IDCB_COORD_SYSTEM, WxEditorFrame::CoordSystemSelChanged )
	EVT_MENU( IDM_SEARCH, WxEditorFrame::MenuEditSearch )
	EVT_MENU( IDM_CUT, WxEditorFrame::MenuEditCut )
	EVT_MENU( IDM_COPY, WxEditorFrame::MenuEditCopy )
	EVT_MENU( IDM_PASTE, WxEditorFrame::MenuEditPasteOriginalLocation )
	EVT_MENU( IDM_PASTE_ORIGINAL_LOCATION, WxEditorFrame::MenuEditPasteOriginalLocation )
	EVT_MENU( IDM_PASTE_WORLD_ORIGIN, WxEditorFrame::MenuEditPasteWorldOrigin )
	EVT_MENU( IDM_PASTE_HERE, WxEditorFrame::MenuEditPasteHere )
	EVT_MENU( IDM_DUPLICATE, WxEditorFrame::MenuEditDuplicate )
	EVT_MENU( IDM_DELETE, WxEditorFrame::MenuEditDelete )
	EVT_MENU( IDM_SELECT_NONE, WxEditorFrame::MenuEditSelectNone )
	EVT_MENU( IDM_SELECT_ALL, WxEditorFrame::MenuEditSelectAll )
	EVT_MENU( IDM_SELECT_ByProperty, WxEditorFrame::MenuEditSelectByProperty )
	EVT_MENU( IDM_SELECT_INVERT, WxEditorFrame::MenuEditSelectInvert )
	EVT_MENU( IDM_TogglePrefabsLocked, WxEditorFrame::MenuTogglePrefabsLocked )
	EVT_MENU( IDM_FULLSCREEN, WxEditorFrame::MenuViewFullScreen )
	EVT_MENU( IDM_BRUSHPOLYS, WxEditorFrame::MenuViewBrushPolys )
	EVT_MENU( IDM_DISTRIBUTION_TOGGLE, WxEditorFrame::MenuViewDistributionToggle )
	EVT_MENU( ID_CAMSPEED_SLOW, WxEditorFrame::MenuViewCamSpeedSlow )
	EVT_MENU( ID_CAMSPEED_NORMAL, WxEditorFrame::MenuViewCamSpeedNormal )
	EVT_MENU( ID_CAMSPEED_FAST, WxEditorFrame::MenuViewCamSpeedFast )
	EVT_MENU( IDM_ACTOR_PROPERTIES, WxEditorFrame::MenuActorProperties )
	EVT_MENU( IDMENU_ActorPopupProperties, WxEditorFrame::MenuActorProperties )
	EVT_MENU( ID_SyncGenericBrowser, WxEditorFrame::MenuSyncGenericBrowser )
	EVT_MENU( ID_MakeSelectedActorsLevelCurrent, WxEditorFrame::MenuMakeSelectedActorsLevelCurrent )
	EVT_MENU( ID_MoveSelectedActorsToCurrentLevel, WxEditorFrame::MenuMoveSelectedActorsToCurrentLevel )
	EVT_MENU( ID_SelectLevelInLevelBrowser, WxEditorFrame::MenuSelectLevelInLevelBrowser )
	EVT_MENU( ID_SelectLevelOnlyInLevelBrowser, WxEditorFrame::MenuSelectLevelOnlyInLevelBrowser )
	EVT_MENU( ID_DeselectLevelInLevelBrowser, WxEditorFrame::MenuDeselectLevelInLevelBrowser )
	EVT_MENU( IDMENU_FindActorInKismet, WxEditorFrame::MenuActorFindInKismet )
	EVT_MENU( IDM_SURFACE_PROPERTIES, WxEditorFrame::MenuSurfaceProperties )
	EVT_MENU( IDM_WORLD_PROPERTIES, WxEditorFrame::MenuWorldProperties )
	EVT_MENU( IDM_BRUSH_ADD, WxEditorFrame::MenuBrushCSG )
	EVT_MENU( IDM_BRUSH_SUBTRACT, WxEditorFrame::MenuBrushCSG )
	EVT_MENU( IDM_BRUSH_INTERSECT, WxEditorFrame::MenuBrushCSG )
	EVT_MENU( IDM_BRUSH_DEINTERSECT, WxEditorFrame::MenuBrushCSG )
	EVT_MENU( IDM_BRUSH_ADD_SPECIAL, WxEditorFrame::MenuBrushAddSpecial )
	EVT_MENU( IDM_BuildPlayInEditor, WxEditorFrame::MenuBuildPlayInEditor )
	EVT_MENU_RANGE( IDM_BuildPlayConsole_START, IDM_BuildPlayConsole_END, WxEditorFrame::MenuBuildPlayOnConsole )
	EVT_MENU_RANGE( IDM_ConsoleSpecific_START, IDM_ConsoleSpecific_END, WxEditorFrame::MenuConsoleSpecific )
	EVT_UPDATE_UI_RANGE(IDM_ConsoleSpecific_START, IDM_ConsoleSpecific_END, WxEditorFrame::UpdateUIConsoleSpecific)
	EVT_MENU( IDM_BUILD_GEOMETRY, WxEditorFrame::MenuBuild )
	EVT_MENU( IDM_BUILD_VISIBLEGEOMETRY, WxEditorFrame::MenuBuild )
	EVT_MENU( IDM_BUILD_LIGHTING, WxEditorFrame::MenuBuild )
	EVT_MENU( IDM_BUILD_AI_PATHS, WxEditorFrame::MenuBuild )
	EVT_MENU( IDM_BUILD_COVER, WxEditorFrame::MenuBuild )
	EVT_MENU( IDM_BUILD_ALL, WxEditorFrame::MenuBuild )
	EVT_MENU_RANGE( IDM_BROWSER_START, IDM_BROWSER_END, WxEditorFrame::MenuViewShowBrowser )
	EVT_MENU_RANGE( IDM_MRU_START, IDM_MRU_END, WxEditorFrame::MenuFileMRU )
	EVT_MENU_RANGE( IDM_VIEWPORT_CONFIG_START, IDM_VIEWPORT_CONFIG_END, WxEditorFrame::MenuViewportConfig )
	EVT_MENU_RANGE( IDM_DRAG_GRID_START, IDM_DRAG_GRID_END, WxEditorFrame::MenuDragGrid )
	EVT_MENU_RANGE( ID_BackdropPopupGrid1, ID_BackdropPopupGrid1024, WxEditorFrame::MenuDragGrid )
	EVT_MENU_RANGE( IDM_ROTATION_GRID_START, IDM_ROTATION_GRID_END, WxEditorFrame::MenuRotationGrid )
	EVT_MENU_RANGE( IDM_AUTOSAVE_START, IDM_AUTOSAVE_END, WxEditorFrame::MenuAutoSaveInterval )
	EVT_MENU_RANGE( IDM_SCALE_GRID_START, IDM_SCALE_GRID_END, WxEditorFrame::MenuScaleGrid )
	EVT_MENU( IDM_VIEWPORT_RESIZE_TOGETHER, WxEditorFrame::MenuViewportResizeTogether )
	EVT_MENU( IDM_OPEN_KISMET, WxEditorFrame::MenuOpenKismet )
	EVT_MENU( WM_REDRAWALLVIEWPORTS, WxEditorFrame::MenuRedrawAllViewports )
	EVT_MENU( IDMN_ALIGN_WALL, WxEditorFrame::MenuAlignWall )
	EVT_MENU( IDMN_TOOL_CHECK_ERRORS, WxEditorFrame::MenuToolCheckErrors )
	EVT_MENU( ID_ReviewPaths, WxEditorFrame::MenuReviewPaths )
	EVT_MENU( ID_RotateActors, WxEditorFrame::MenuRotateActors )
	EVT_MENU( ID_ResetParticleEmitters, WxEditorFrame::MenuResetParticleEmitters )
	EVT_MENU( ID_EditSelectAllSurfs, WxEditorFrame::MenuSelectAllSurfs )
	EVT_MENU( ID_BrushAdd, WxEditorFrame::MenuBrushAdd )
	EVT_MENU( ID_BrushSubtract, WxEditorFrame::MenuBrushSubtract )
	EVT_MENU( ID_BrushIntersect, WxEditorFrame::MenuBrushIntersect )
	EVT_MENU( ID_BrushDeintersect, WxEditorFrame::MenuBrushDeintersect )
	EVT_MENU( ID_BrushAddSpecial, WxEditorFrame::MenuBrushAddSpecial )
	EVT_MENU( ID_BrushOpen, WxEditorFrame::MenuBrushOpen )
	EVT_MENU( ID_BrushSaveAs, WxEditorFrame::MenuBrushSaveAs )
	EVT_MENU( ID_BRUSH_IMPORT, WxEditorFrame::MenuBrushImport )
	EVT_MENU( ID_BRUSH_EXPORT, WxEditorFrame::MenuBrushExport )
	EVT_MENU( IDM_WIZARD_NEW_TERRAIN, WxEditorFrame::MenuWizardNewTerrain )	
	EVT_MENU( IDM_REPLACESKELMESHACTORS, WxEditorFrame::MenuReplaceSkelMeshActors )	
	EVT_MENU( IDM_CleanBSPMaterials, WxEditorFrame::MenuCleanBSPMaterials )
	EVT_MENU( IDMENU_ABOUTBOX, WxEditorFrame::MenuAboutBox )	
	EVT_MENU( IDMENU_ONLINEHELP, WxEditorFrame::MenuOnlineHelp )
	EVT_MENU( IDMENU_TIPOFTHEDAY, WxEditorFrame::MenuTipOfTheDay )
	EVT_MENU( ID_BackdropPopupAddClassHere, WxEditorFrame::MenuBackdropPopupAddClassHere )
	EVT_MENU_RANGE( ID_BackdropPopupAddLastSelectedClassHere_START, ID_BackdropPopupAddLastSelectedClassHere_END, WxEditorFrame::MenuBackdropPopupAddLastSelectedClassHere )
	EVT_MENU( IDM_BackDropPopupPlayFromHereInEditor, WxEditorFrame::MenuPlayFromHereInEditor )
	EVT_MENU_RANGE( IDM_BackDropPopupPlayFromHereConsole_START, IDM_BackDropPopupPlayFromHereConsole_END, WxEditorFrame::MenuPlayFromHereOnConsole )
	EVT_COMBOBOX( IDCB_ObjectPropagation, WxEditorFrame::ObjectPropagationSelChanged )
	EVT_MENU( IDM_PUSHVIEW_StartStop, WxEditorFrame::PushViewStartStop )
	EVT_MENU( IDM_PUSHVIEW_SYNC, WxEditorFrame::PushViewSync )
	EVT_MENU( ID_SurfPopupApplyMaterial, WxEditorFrame::MenuSurfPopupApplyMaterial )
	EVT_MENU( ID_SurfPopupAlignPlanarAuto, WxEditorFrame::MenuSurfPopupAlignPlanarAuto )
	EVT_MENU( ID_SurfPopupAlignPlanarWall, WxEditorFrame::MenuSurfPopupAlignPlanarWall )
	EVT_MENU( ID_SurfPopupAlignPlanarFloor, WxEditorFrame::MenuSurfPopupAlignPlanarFloor )
	EVT_MENU( ID_SurfPopupAlignBox, WxEditorFrame::MenuSurfPopupAlignBox )
	EVT_MENU( ID_SurfPopupAlignFace, WxEditorFrame::MenuSurfPopupAlignFace )
	EVT_MENU( ID_SurfPopupUnalign, WxEditorFrame::MenuSurfPopupUnalign )
	EVT_MENU( ID_SurfPopupSelectMatchingGroups, WxEditorFrame::MenuSurfPopupSelectMatchingGroups )
	EVT_MENU( ID_SurfPopupSelectMatchingItems, WxEditorFrame::MenuSurfPopupSelectMatchingItems )
	EVT_MENU( ID_SurfPopupSelectMatchingBrush, WxEditorFrame::MenuSurfPopupSelectMatchingBrush )
	EVT_MENU( ID_SurfPopupSelectMatchingTexture, WxEditorFrame::MenuSurfPopupSelectMatchingTexture )
	EVT_MENU( ID_SurfPopupSelectAllAdjacents, WxEditorFrame::MenuSurfPopupSelectAllAdjacents )
	EVT_MENU( ID_SurfPopupSelectAdjacentCoplanars, WxEditorFrame::MenuSurfPopupSelectAdjacentCoplanars )
	EVT_MENU( ID_SurfPopupSelectAdjacentWalls, WxEditorFrame::MenuSurfPopupSelectAdjacentWalls )
	EVT_MENU( ID_SurfPopupSelectAdjacentFloors, WxEditorFrame::MenuSurfPopupSelectAdjacentFloors )
	EVT_MENU( ID_SurfPopupSelectAdjacentSlants, WxEditorFrame::MenuSurfPopupSelectAdjacentSlants )
	EVT_MENU( ID_SurfPopupSelectReverse, WxEditorFrame::MenuSurfPopupSelectReverse )
	EVT_MENU( ID_SurfPopupMemorize, WxEditorFrame::MenuSurfPopupSelectMemorize )
	EVT_MENU( ID_SurfPopupRecall, WxEditorFrame::MenuSurfPopupRecall )
	EVT_MENU( ID_SurfPopupOr, WxEditorFrame::MenuSurfPopupOr )
	EVT_MENU( ID_SurfPopupAnd, WxEditorFrame::MenuSurfPopupAnd )
	EVT_MENU( ID_SurfPopupXor, WxEditorFrame::MenuSurfPopupXor )
	EVT_MENU( IDMENU_ActorPopupSelectAllClass, WxEditorFrame::MenuActorPopupSelectAllClass )
	EVT_MENU( IDMENU_ActorPopupSelectMatchingStaticMeshesThisClass, WxEditorFrame::MenuActorPopupSelectMatchingStaticMeshesThisClass )
	EVT_MENU( IDMENU_ActorPopupSelectMatchingStaticMeshesAllClasses, WxEditorFrame::MenuActorPopupSelectMatchingStaticMeshesAllClasses )
	EVT_MENU( IDMENU_ActorPopupToggleDynamicChannel, WxEditorFrame::MenuActorPopupToggleDynamicChannel )
	EVT_MENU( IDMENU_ActorPopupSelectAllLights, WxEditorFrame::MenuActorPopupSelectAllLights )
	EVT_MENU( IDMENU_ActorPopupSelectAllLightsWithSameClassification, WxEditorFrame::MenuActorPopupSelectAllLightsWithSameClassification )
	EVT_MENU( IDMENU_ActorPopupSelectKismetReferenced, WxEditorFrame::MenuActorPopupSelectKismetReferenced )
	EVT_MENU( IDMENU_ActorPopupSelectKismetUnreferenced, WxEditorFrame::MenuActorPopupSelectKismetUnreferenced )
	EVT_MENU( IDMENU_ActorPopupSelectMatchingSpeedTrees, WxEditorFrame::MenuActorPopupSelectMatchingSpeedTrees )
	EVT_MENU( IDMENU_ActorPopupAlignCameras, WxEditorFrame::MenuActorPopupAlignCameras )
	EVT_MENU( IDMENU_ActorPopupLockMovement, WxEditorFrame::MenuActorPopupLockMovement )
	EVT_MENU( IDMENU_ActorPopupSnapViewToCamera, WxEditorFrame::MenuActorPopupSnapViewToCamera )
	EVT_MENU( IDMENU_ActorPopupMerge, WxEditorFrame::MenuActorPopupMerge )
	EVT_MENU( IDMENU_ActorPopupSeparate, WxEditorFrame::MenuActorPopupSeparate )
	EVT_MENU( IDMENU_ActorPopupToFirst, WxEditorFrame::MenuActorPopupToFirst )
	EVT_MENU( IDMENU_ActorPopupToLast, WxEditorFrame::MenuActorPopupToLast )
	EVT_MENU( IDMENU_ActorPopupToBrush, WxEditorFrame::MenuActorPopupToBrush )
	EVT_MENU( IDMENU_ActorPopupFromBrush, WxEditorFrame::MenuActorPopupFromBrush )
	EVT_MENU( IDMENU_ActorPopupMakeAdd, WxEditorFrame::MenuActorPopupMakeAdd )
	EVT_MENU( IDMENU_ActorPopupMakeSubtract, WxEditorFrame::MenuActorPopupMakeSubtract )
	EVT_MENU( IDMENU_ActorPopupPathPosition, WxEditorFrame::MenuActorPopupPathPosition )
	EVT_MENU( IDMENU_ActorPopupPathProscribe, WxEditorFrame::MenuActorPopupPathProscribe )
	EVT_MENU( IDMENU_ActorPopupPathForce, WxEditorFrame::MenuActorPopupPathForce )
	EVT_MENU( IDMENU_ActorPopupPathOverwriteRoute, WxEditorFrame::MenuActorPopupPathAssignNavPointsToRoute )
	EVT_MENU( IDMENU_ActorPopupPathAddRoute, WxEditorFrame::MenuActorPopupPathAssignNavPointsToRoute )
	EVT_MENU( IDMENU_ActorPopupPathRemoveRoute, WxEditorFrame::MenuActorPopupPathAssignNavPointsToRoute )
	EVT_MENU( IDMENU_ActorPopupPathClearRoute, WxEditorFrame::MenuActorPopupPathAssignNavPointsToRoute )
	EVT_MENU( IDMENU_ActorPopupPathOverwriteCoverGroup, WxEditorFrame::MenuActorPopupPathAssignLinksToCoverGroup )
	EVT_MENU( IDMENU_ActorPopupPathAddCoverGroup, WxEditorFrame::MenuActorPopupPathAssignLinksToCoverGroup )
	EVT_MENU( IDMENU_ActorPopupPathRemoveCoverGroup, WxEditorFrame::MenuActorPopupPathAssignLinksToCoverGroup )
	EVT_MENU( IDMENU_ActorPopupPathClearCoverGroup, WxEditorFrame::MenuActorPopupPathAssignLinksToCoverGroup )
	EVT_MENU( IDMENU_ActorPopupPathClearProscribed, WxEditorFrame::MenuActorPopupPathClearProscribed )
	EVT_MENU( IDMENU_ActorPopupPathClearForced, WxEditorFrame::MenuActorPopupPathClearForced )
	EVT_MENU( IDMENU_ActorPopupPathStitchCover, WxEditorFrame::MenuActorPopupPathStitchCover )
	EVT_MENU( IDMENU_SnapToFloor, WxEditorFrame::MenuSnapToFloor )
	EVT_MENU( IDMENU_AlignToFloor, WxEditorFrame::MenuAlignToFloor )
	EVT_MENU( IDMENU_MoveToGrid, WxEditorFrame::MenuMoveToGrid )
	EVT_MENU( IDMENU_SaveBrushAsCollision, WxEditorFrame::MenuSaveBrushAsCollision )

	EVT_MENU( IDMENU_ActorPopupConvertKActorToStaticMesh, WxEditorFrame::MenuConvertActors )
	EVT_MENU( IDMENU_ActorPopupConvertKActorToMover, WxEditorFrame::MenuConvertActors )
	EVT_MENU( IDMENU_ActorPopupConvertStaticMeshToKActor, WxEditorFrame::MenuConvertActors )
	EVT_MENU( IDMENU_ActorPopupConvertStaticMeshToMover, WxEditorFrame::MenuConvertActors )
	EVT_MENU( IDMENU_ActorPopupConvertMoverToStaticMesh, WxEditorFrame::MenuConvertActors )
	EVT_MENU( IDMENU_ActorPopupConvertMoverToKActor, WxEditorFrame::MenuConvertActors )

	EVT_MENU( IDMENU_IDMENU_ActorPopupConvertLightToLightDynamicAffecting, WxEditorFrame::MenuSetLightDataBasedOnClassification )
	EVT_MENU( IDMENU_IDMENU_ActorPopupConvertLightToLightStaticAffecting, WxEditorFrame::MenuSetLightDataBasedOnClassification )
	EVT_MENU( IDMENU_IDMENU_ActorPopupConvertLightToLightDynamicAndStaticAffecting, WxEditorFrame::MenuSetLightDataBasedOnClassification )

	EVT_MENU( IDMENU_QuantizeVertices, WxEditorFrame::MenuQuantizeVertices )
	EVT_MENU( IDMENU_ConvertToStaticMesh, WxEditorFrame::MenuConvertToStaticMesh )
	EVT_MENU( IDMENU_ActorPopupResetPivot, WxEditorFrame::MenuActorPivotReset )
	EVT_MENU( IDM_SELECT_SHOW, WxEditorFrame::MenuActorSelectShow )
	EVT_MENU( IDM_SELECT_HIDE, WxEditorFrame::MenuActorSelectHide )
	EVT_MENU( IDM_SELECT_INVERT, WxEditorFrame::MenuActorSelectInvert )
	EVT_MENU( IDM_SHOW_ALL, WxEditorFrame::MenuActorShowAll )
	EVT_MENU( ID_BackdropPopupPivot, WxEditorFrame::MenuActorPivotMoveHere )
	EVT_MENU( ID_BackdropPopupPivotSnapped, WxEditorFrame::MenuActorPivotMoveHereSnapped )
	EVT_MENU( ID_BackdropPopupPivotSnappedCenterSelection, WxEditorFrame::MenuActorPivotMoveCenterOfSelection )
	EVT_MENU_RANGE( IDMENU_ActorFactory_Start, IDMENU_ActorFactory_End, WxEditorFrame::MenuUseActorFactory )
	EVT_MENU_RANGE( IDMENU_ActorFactoryAdv_Start, IDMENU_ActorFactoryAdv_End, WxEditorFrame::MenuUseActorFactoryAdv )
	EVT_MENU( IDMENU_ActorPopupMirrorX, WxEditorFrame::MenuActorMirrorX )
	EVT_MENU( IDMENU_ActorPopupMirrorY, WxEditorFrame::MenuActorMirrorY )
	EVT_MENU( IDMENU_ActorPopupMirrorZ, WxEditorFrame::MenuActorMirrorZ )
	EVT_COMMAND_RANGE( IDM_VolumeClasses_START, IDM_VolumeClasses_END, wxEVT_COMMAND_MENU_SELECTED, WxEditorFrame::OnAddVolumeClass )
	EVT_MENU( IDMENU_ActorPopupMakeSolid, WxEditorFrame::MenuActorPopupMakeSolid )
	EVT_MENU( IDMENU_ActorPopupMakeSemiSolid, WxEditorFrame::MenuActorPopupMakeSemiSolid )
	EVT_MENU( IDMENU_ActorPopupMakeNonSolid, WxEditorFrame::MenuActorPopupMakeNonSolid )
	EVT_MENU( IDMENU_ActorPopupSelectBrushesAdd, WxEditorFrame::MenuActorPopupBrushSelectAdd )
	EVT_MENU( IDMENU_ActorPopupSelectBrushesSubtract, WxEditorFrame::MenuActorPopupBrushSelectSubtract )
	EVT_MENU( IDMENU_ActorPopupSelectBrushesNonsolid, WxEditorFrame::MenuActorPopupBrushSelectNonSolid )
	EVT_MENU( IDMENU_ActorPopupSelectBrushesSemisolid, WxEditorFrame::MenuActorPopupBrushSelectSemiSolid )
	EVT_MENU(IDMENU_EmitterPopupOptionsAutoPopulate, WxEditorFrame::MenuEmitterAutoPopulate)
	EVT_MENU(IDMENU_EmitterPopupOptionsReset, WxEditorFrame::MenuEmitterReset)

	EVT_MENU( IDM_CREATEARCHETYPE, WxEditorFrame::CreateArchetype )
	EVT_MENU( IDM_CREATEPREFAB, WxEditorFrame::CreatePrefab )
	EVT_MENU( IDM_ADDPREFAB, WxEditorFrame::AddPrefab )
	EVT_MENU( IDM_SELECTALLACTORSINPREFAB, WxEditorFrame::SelectPrefabActors )
	EVT_MENU( IDM_UPDATEPREFABFROMINSTANCE, WxEditorFrame::UpdatePrefabFromInstance )
	EVT_MENU( IDM_RESETFROMPREFAB, WxEditorFrame::ResetInstanceFromPrefab )
	EVT_MENU( IDM_CONVERTPREFABTONORMALACTORS, WxEditorFrame::PrefabInstanceToNormalActors )
	EVT_MENU( IDM_OPENPREFABINSTANCESEQUENCE, WxEditorFrame::PrefabInstanceOpenSequence )
	EVT_MENU_RANGE (IDM_CREATE_MATERIAL_INSTANCE_CONSTANT_START, IDM_CREATE_MATERIAL_INSTANCE_CONSTANT_END, WxEditorFrame::MenuCreateMaterialInstanceConstant)
	EVT_MENU_RANGE (IDM_EDIT_MATERIAL_INSTANCE_CONSTANT_START, IDM_EDIT_MATERIAL_INSTANCE_CONSTANT_END, WxEditorFrame::MenuCreateMaterialInstanceConstant)

	EVT_MENU_RANGE (IDM_CREATE_MATERIAL_INSTANCE_TIME_VARYING_START, IDM_CREATE_MATERIAL_INSTANCE_TIME_VARYING_END, WxEditorFrame::MenuCreateMaterialInstanceTimeVarying)
	EVT_MENU_RANGE (IDM_EDIT_MATERIAL_INSTANCE_TIME_VARYING_START, IDM_EDIT_MATERIAL_INSTANCE_TIME_VARYING_END, WxEditorFrame::MenuCreateMaterialInstanceTimeVarying)

	EVT_MENU_RANGE (IDM_ASSIGNMATERIALINSTANCE_START, IDM_ASSIGNMATERIALINSTANCE_END, WxEditorFrame::MenuAssignMaterial)

	EVT_UPDATE_UI( IDM_UNDO, WxEditorFrame::UI_MenuEditUndo )
	EVT_UPDATE_UI( IDM_REDO, WxEditorFrame::UI_MenuEditRedo )
	EVT_UPDATE_UI( ID_EDIT_MOUSE_LOCK, WxEditorFrame::UI_MenuEditMouseLock )
	EVT_UPDATE_UI( ID_EDIT_SHOW_WIDGET, WxEditorFrame::UI_MenuEditShowWidget )
	EVT_UPDATE_UI( ID_EDIT_TRANSLATE, WxEditorFrame::UI_MenuEditTranslate )
	EVT_UPDATE_UI( ID_EDIT_ROTATE, WxEditorFrame::UI_MenuEditRotate )
	EVT_UPDATE_UI( ID_EDIT_SCALE, WxEditorFrame::UI_MenuEditScale )
	EVT_UPDATE_UI( ID_EDIT_SCALE_NONUNIFORM, WxEditorFrame::UI_MenuEditScaleNonUniform )
	EVT_UPDATE_UI_RANGE( IDM_VIEWPORT_CONFIG_START, IDM_VIEWPORT_CONFIG_END, WxEditorFrame::UI_MenuViewportConfig )
	EVT_UPDATE_UI_RANGE( IDM_DRAG_GRID_START, IDM_DRAG_GRID_END, WxEditorFrame::UI_MenuDragGrid )
	EVT_UPDATE_UI_RANGE( ID_BackdropPopupGrid1, ID_BackdropPopupGrid1024, WxEditorFrame::UI_MenuDragGrid )
	EVT_UPDATE_UI_RANGE( IDM_ROTATION_GRID_START, IDM_ROTATION_GRID_END, WxEditorFrame::UI_MenuRotationGrid )
	EVT_UPDATE_UI_RANGE( IDM_SCALE_GRID_START, IDM_SCALE_GRID_END, WxEditorFrame::UI_MenuScaleGrid )
	EVT_UPDATE_UI( IDM_VIEWPORT_RESIZE_TOGETHER, WxEditorFrame::UI_MenuViewResizeViewportsTogether )
	EVT_UPDATE_UI( IDM_FULLSCREEN, WxEditorFrame::UI_MenuViewFullScreen )
	EVT_UPDATE_UI( IDM_BRUSHPOLYS, WxEditorFrame::UI_MenuViewBrushPolys )
	EVT_UPDATE_UI( IDM_TogglePrefabsLocked, WxEditorFrame::UI_MenuTogglePrefabLock )
	EVT_UPDATE_UI( IDM_DISTRIBUTION_TOGGLE, WxEditorFrame::UI_MenuViewDistributionToggle )
	EVT_UPDATE_UI( ID_CAMSPEED_SLOW, WxEditorFrame::UI_MenuViewCamSpeedSlow )
	EVT_UPDATE_UI( ID_CAMSPEED_NORMAL, WxEditorFrame::UI_MenuViewCamSpeedNormal )
	EVT_UPDATE_UI( ID_CAMSPEED_FAST, WxEditorFrame::UI_MenuViewCamSpeedFast )

	EVT_UPDATE_UI( ID_MakeSelectedActorsLevelCurrent, WxEditorFrame::UI_ContextMenuMakeCurrentLevel )

	EVT_DOCKINGCHANGE( WxEditorFrame::OnDockingChange )

	EVT_MENU( IDM_CoverEditMenu_ToggleEnabled, WxEditorFrame::CoverEdit_ToggleEnabled )
	EVT_MENU( IDM_CoverEditMenu_ToggleType, WxEditorFrame::CoverEdit_ToggleType )
	EVT_MENU( IDM_CoverEditMenu_ToggleCoverslip, WxEditorFrame::CoverEdit_ToggleCoverslip )
	EVT_MENU( IDM_CoverEditMenu_ToggleSwatTurn, WxEditorFrame::CoverEdit_ToggleSwatTurn )
	EVT_MENU( IDM_CoverEditMenu_ToggleMantle, WxEditorFrame::CoverEdit_ToggleMantle )
	EVT_MENU( IDM_CoverEditMenu_TogglePopup, WxEditorFrame::CoverEdit_TogglePopup )
	EVT_MENU( IDM_CoverEditMenu_ToggleClimbUp, WxEditorFrame::CoverEdit_ToggleClimbUp )

END_EVENT_TABLE()

// Used for dynamic creation of the window. This must be declared for any
// subclasses of WxEditorFrame
IMPLEMENT_DYNAMIC_CLASS(WxEditorFrame,wxFrame);

/**
 * Default constructor. Construction is a 2 phase process: class creation
 * followed by a call to Create(). This is required for dynamically determining
 * which editor frame class to create for the editor's main frame.
 */
WxEditorFrame::WxEditorFrame() :
	ViewportContainer( NULL )
{
	MainMenuBar = NULL;
	MainToolBar = NULL;
	DragGridMenu = NULL;
	RotationGridMenu = NULL;
	AutoSaveIntervalMenu = NULL;
	ButtonBar = NULL;
	ViewportConfigData = NULL;
	bViewportResizeTogether = FALSE;

	OptionProxies = NULL;

	FramePos.x = -1;
	FramePos.y = -1;
	FrameSize.Set( -1, -1 );
	bFrameMaximized = TRUE;

	// Set default grid sizes, powers of two
	for( INT i = 0 ; i < FEditorConstraints::MAX_GRID_SIZES ; ++i )
	{
		GEditor->Constraints.GridSizes[i] = (float)(1 << i);
	}

}

/**
 * Part 2 of the 2 phase creation process. First it loads the localized caption.
 * Then it creates the window with that caption. And finally finishes the
 * window initialization
 */
void WxEditorFrame::Create()
{
	wxString Caption = *GetLocalizedCaption();

	UBOOL bShouldBeMaximized = bFrameMaximized;
	GConfig->GetInt( TEXT("EditorFrame"), TEXT("FramePos.x"), (INT&)FramePos.x, GEditorUserSettingsIni );
	GConfig->GetInt( TEXT("EditorFrame"), TEXT("FramePos.y"), (INT&)FramePos.y, GEditorUserSettingsIni );
	GConfig->GetInt( TEXT("EditorFrame"), TEXT("FrameSize.x"), (INT&)FrameSize.x, GEditorUserSettingsIni );
	GConfig->GetInt( TEXT("EditorFrame"), TEXT("FrameSize.y"), (INT&)FrameSize.y, GEditorUserSettingsIni );
	GConfig->GetInt( TEXT("EditorFrame"), TEXT("FrameMaximized"), (INT&)bShouldBeMaximized, GEditorUserSettingsIni );
	GConfig->GetBool( TEXT("EditorFrame"), TEXT("ViewportResizeTogether"), bViewportResizeTogether, GEditorUserSettingsIni );

	// Assert if this fails
	const bool bSuccess = wxFrame::Create( NULL, -1, Caption, FramePos, FrameSize );
	check( bSuccess );

	// Create all the status bars and set the default one.
	StatusBars[ SB_Standard ] = new WxStatusBarStandard;
	StatusBars[ SB_Standard ]->Create( this, -1 );
	StatusBars[ SB_Standard ]->Show( 0 );
	StatusBars[ SB_Progress ] = new WxStatusBarProgress;
	StatusBars[ SB_Progress ]->Create( this, -1 );
	StatusBars[ SB_Progress ]->Show( 0 );

	bFrameMaximized = bShouldBeMaximized;
	if ( bFrameMaximized )
	{
		Maximize();
	}

	// Make window foreground.
	HWND WindowHnd = (HWND)this->GetHandle();
	if( WindowHnd )
	{
		SetWindowPos( WindowHnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
		SetForegroundWindow( WindowHnd );
		SetWindowPos( WindowHnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
	}
}

/**
 * Returns the localized caption for this frame window. It looks in the
 * editor's INI file to determine which localization file to use
 */
FString WxEditorFrame::GetLocalizedCaption()
{
	// Load the name of the "package" that the localization is in
	// (MyBrowser.int for instance)
	FString Package(TEXT("UnrealEd"));
	GConfig->GetString(TEXT("EditorFrame"),TEXT("LocalizationFile"),
		Package,GEditorIni);
	FString LocalizedCaption;
	// Get the localized name for this window
	LocalizedCaption = Localize(TEXT("EditorFrame"),TEXT("Caption"),*Package,
		NULL,TRUE);
	// In case there isn't any localization for this window, use the default
	if (LocalizedCaption.Len() == 0)
	{
		LocalizedCaption = TEXT("UnrealEd");
	}
	return LocalizedCaption;
}

WxEditorFrame::~WxEditorFrame()
{
	if( GEditorModeTools().GetCurrentModeID() == EM_CoverEdit )
	{
		GEditorModeTools().SetCurrentMode( EM_Default );
	}

	// Save the viewport configuration to the INI file
	if( ViewportConfigData )
	{
		ViewportConfigData->SaveToINI();
	}
	delete ViewportConfigData;
	ViewportConfigData = NULL;

	// Save out any config settings for the editor so they don't get lost
	GEditor->SaveConfig();
	GEditorModeTools().SaveConfig();

	// Let the browser manager save its state before destroying it
	GUnrealEd->GetBrowserManager()->SaveState();
	GUnrealEd->GetBrowserManager()->DestroyBrowsers();

	// Save last used windowed size:
	GConfig->SetInt( TEXT("EditorFrame"), TEXT("FramePos.x"), FramePos.x, GEditorUserSettingsIni );
	GConfig->SetInt( TEXT("EditorFrame"), TEXT("FramePos.y"), FramePos.y, GEditorUserSettingsIni );
	GConfig->SetInt( TEXT("EditorFrame"), TEXT("FrameSize.x"), FrameSize.x, GEditorUserSettingsIni );
	GConfig->SetInt( TEXT("EditorFrame"), TEXT("FrameSize.y"), FrameSize.y, GEditorUserSettingsIni );
	GConfig->SetInt( TEXT("EditorFrame"), TEXT("FrameMaximized"), bFrameMaximized, GEditorUserSettingsIni );

	GConfig->SetBool( TEXT("EditorFrame"), TEXT("ViewportResizeTogether"), bViewportResizeTogether, GEditorUserSettingsIni );

	// Can't use appRequestExit as it uses PostQuitMessage which is not what we want here.
	GIsRequestingExit = 1;

	//delete MainMenuBar;
	//delete MainToolBar;
	//delete AcceleratorTable;
	//delete DragGridMenu;
	//delete RotationGridMenu;
}

// Gives elements in the UI a chance to update themselves based on internal engine variables.

void WxEditorFrame::UpdateUI()
{
	// Left side button bar

	if( ButtonBar )
	{
		ButtonBar->UpdateUI();
	}

	// Viewport toolbars

	if( ViewportConfigData )
	{
		for( INT x = 0 ; x < 4 ; ++x )
		{
			if( ViewportConfigData->Viewports[x].bEnabled )
			{
				ViewportConfigData->Viewports[x].ViewportWindow->ToolBar->UpdateUI();
			}
		}
	}

	// Status bars

	if( StatusBars[ SB_Standard ] )
	{
		StatusBars[ SB_Standard ]->UpdateUI();
	}

	// Main toolbar

	if( MainToolBar )
	{
		MainToolBar->CoordSystemCombo->SetSelection( GEditorModeTools().CoordSystem );
	}
}

// Creates the child windows for the frame and sets everything to it's initial state.

void WxEditorFrame::SetUp()
{
	// Child windows that control the client area

	ViewportContainer = new WxViewportsContainer( (wxWindow*)this, IDW_VIEWPORT_CONTAINER );
	ViewportContainer->SetTitle( wxT("ViewportContainer") );

	// Mode bar

	ModeBar = NULL;

	// Menus

	DragGridMenu = new WxDragGridMenu;
	RotationGridMenu = new WxRotationGridMenu;
	AutoSaveIntervalMenu = new WxAutoSaveIntervalMenu;
	ScaleGridMenu = new WxScaleGridMenu;

	// Common resources

	WhitePlaceholder.Load( TEXT("WhitePlaceholder" ) );
	WizardB.Load( TEXT("Wizard") );
	DownArrowB.Load( TEXT("DownArrow") );
	MaterialEditor_RGBAB.Load( TEXT("MaterialEditor_RGBA") );
	MaterialEditor_RB.Load( TEXT("MaterialEditor_R") );
	MaterialEditor_GB.Load( TEXT("MaterialEditor_G") );
	MaterialEditor_BB.Load( TEXT("MaterialEditor_B") );
	MaterialEditor_AB.Load( TEXT("MaterialEditor_A") );
	MaterialEditor_ControlPanelFillB.Load( TEXT("MaterialEditor_ControlPanelFill") );
	MaterialEditor_ControlPanelCapB.Load( TEXT("MaterialEditor_ControlPanelCap") );
	LeftHandle.Load( TEXT("MaterialEditor_LeftHandle") );
	RightHandle.Load( TEXT("MaterialEditor_RightHandle") );
	RightHandleOn.Load( TEXT("MaterialEditor_RightHandleOn") );
	ArrowUp.Load( TEXT("UpArrowLarge") );
	ArrowDown.Load( TEXT("DownArrowLarge") );
	ArrowRight.Load( TEXT("RightArrowLarge") );

	// Load grid settings

	for( INT i = 0 ; i < FEditorConstraints::MAX_GRID_SIZES ; ++i )
	{
		FString Key = FString::Printf( TEXT("GridSize%d"), i );
		GConfig->GetFloat( TEXT("GridSizes"), *Key, GEditor->Constraints.GridSizes[i], GEditorIni );
	}

	// Viewport configuration options

	FViewportConfig_Template* Template;

	Template = new FViewportConfig_Template;
	Template->Set( VC_2_2_Split );
	ViewportConfigTemplates.AddItem( Template );

	Template = new FViewportConfig_Template;
	Template->Set( VC_1_2_Split );
	ViewportConfigTemplates.AddItem( Template );

	Template = new FViewportConfig_Template;
	Template->Set( VC_1_1_Split );
	ViewportConfigTemplates.AddItem( Template );

	StatusBars[ SB_Standard ]->SetUp();
	StatusBars[ SB_Progress ]->SetUp();

	// Browser window initialization
	GUnrealEd->GetBrowserManager()->Initialize();

	// Main UI components

	MainMenuBar = new WxMainMenu;
	SetMenuBar( MainMenuBar );

	MainToolBar = new WxMainToolBar( (wxWindow*)this, -1 );
	SetToolBar( MainToolBar );

	MainMenuBar->MRUFiles->ReadINI();
	MainMenuBar->MRUFiles->UpdateMenu();

	SetStatusBar( SB_Standard );

	// Clean up
	wxSizeEvent DummyEvent;
	OnSize( DummyEvent );
}

// Changes the active status bar

void WxEditorFrame::SetStatusBar( EStatusBar InStatusBar )
{
	wxFrame::SetStatusBar( StatusBars[ InStatusBar ] );

	// Make all statusbars the same size as the SB_Standard one
	// FIXME : This is a bit of a hack as I suspect there must be a nice way of doing this, but I can't see it right now.

	wxRect rect = StatusBars[ SB_Standard ]->GetRect();
	for( INT x = 0 ; x < SB_Max ; ++x )
	{
		StatusBars[ x ]->SetSize( rect );
	}

	// Hide all status bars, except the active one

	for( INT x = 0 ; x < SB_Max ; ++x )
	{
		StatusBars[ x ]->Show( x == InStatusBar );
	}
}

///////////////////////////////////////////////////////////////////////////////

void WxEditorFrame::Send(ECallbackEventType Event)
{
	//@todo RefreshCaption
}

/**
 * Updates the application's title bar caption.
 */
void WxEditorFrame::RefreshCaption(const FFilename* LevelFilename)
{
	FString Caption = *FString::Printf( *LocalizeUnrealEd("UnrealEdCaption_F"), (LevelFilename->Len()?*LevelFilename->GetBaseFilename():*LocalizeUnrealEd("Untitled")) );
	SetTitle( *Caption );
}

///////////////////////////////////////////////////////////////////////////////

// Changes the viewport configuration

void WxEditorFrame::SetViewportConfig( EViewportConfig InConfig )
{
	FViewportConfig_Data* SaveConfig = ViewportConfigData;
	ViewportConfigData = new FViewportConfig_Data;

	SaveConfig->Save();

	ViewportContainer->DestroyChildren();

	ViewportConfigData->SetTemplate( InConfig );
	ViewportConfigData->Load( SaveConfig );

	// The user is changing the viewport config via the main menu. In this situation
	// we want to reset the splitter positions so they are in their default positions.

	GConfig->SetInt( TEXT("ViewportConfig"), TEXT("Splitter0"), 0, GEditorIni );
	GConfig->SetInt( TEXT("ViewportConfig"), TEXT("Splitter1"), 0, GEditorIni );
	GConfig->SetInt( TEXT("ViewportConfig"), TEXT("Splitter2"), 0, GEditorIni );
	GConfig->SetInt( TEXT("ViewportConfig"), TEXT("Splitter3"), 0, GEditorIni );

	ViewportConfigData->Apply( ViewportContainer );

	delete SaveConfig;
}

FGetInfoRet WxEditorFrame::GetInfo( INT Item )
{
	FGetInfoRet Ret;

	Ret.iValue = 0;
	Ret.String = TEXT("");

	// ACTORS
	if( Item & GI_NUM_SELECTED
			|| Item & GI_CLASSNAME_SELECTED
			|| Item & GI_CLASS_SELECTED )
	{
		INT NumActors = 0;
		UBOOL bAnyClass = FALSE;
		UClass*	AllClass = NULL;

		for ( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
		{
			AActor* Actor = static_cast<AActor*>( *It );
			checkSlow( Actor->IsA(AActor::StaticClass()) );

			if( bAnyClass && Actor->GetClass() != AllClass )
			{
				AllClass = NULL;
			}
			else
			{
				AllClass = Actor->GetClass();
			}

			bAnyClass = TRUE;
			NumActors++;
		}

		if( Item & GI_NUM_SELECTED )
		{
			Ret.iValue = NumActors;
		}
		if( Item & GI_CLASSNAME_SELECTED )
		{
			if( bAnyClass && AllClass )
			{
				Ret.String = AllClass->GetName();
			}
			else
			{
				Ret.String = TEXT("Actor");
			}
		}
		if( Item & GI_CLASS_SELECTED )
		{
			if( bAnyClass && AllClass )
			{
				Ret.pClass = AllClass;
			}
			else
			{
				Ret.pClass = NULL;
			}
		}
	}

	// SURFACES
	if( Item & GI_NUM_SURF_SELECTED)
	{
		INT NumSurfs = 0;

		for( INT i=0; i<GWorld->GetModel()->Surfs.Num(); ++i )
		{
			FBspSurf *Poly = &GWorld->GetModel()->Surfs(i);

			if( Poly->PolyFlags & PF_Selected )
			{
				NumSurfs++;
			}
		}

		if( Item & GI_NUM_SURF_SELECTED )
		{
			Ret.iValue = NumSurfs;
		}
	}

	return Ret;
}

void WxEditorFrame::ClearOptionProxies()
{
	if ( OptionProxies )
	{
		OptionProxies->Empty();
	}
}

// Initializes the classes for the generic dialog system (UOption* classes).  This needs to be
// called every map change so that UOptions-dervied objects can be linked up to the current GWorld.
void WxEditorFrame::OptionProxyInit()
{
	if ( !OptionProxies )
	{
		OptionProxies = new TMap<INT,UOptionsProxy*>;
	}
	else
	{
		ClearOptionProxies();
	}

	CREATE_OPTION_PROXY( OPTIONS_2DSHAPERSHEET, UOptions2DShaperSheet );
	CREATE_OPTION_PROXY( OPTIONS_2DSHAPEREXTRUDE, UOptions2DShaperExtrude );
	CREATE_OPTION_PROXY( OPTIONS_2DSHAPEREXTRUDETOPOINT, UOptions2DShaperExtrudeToPoint );
	CREATE_OPTION_PROXY( OPTIONS_2DSHAPEREXTRUDETOBEVEL, UOptions2DShaperExtrudeToBevel );
	CREATE_OPTION_PROXY( OPTIONS_2DSHAPERREVOLVE, UOptions2DShaperRevolve );
	CREATE_OPTION_PROXY( OPTIONS_2DSHAPERBEZIERDETAIL, UOptions2DShaperBezierDetail );
	CREATE_OPTION_PROXY( OPTIONS_DUPOBJECT, UOptionsDupObject );
	CREATE_OPTION_PROXY( OPTIONS_NEWCLASSFROMSEL, UOptionsNewClassFromSel );
}

UOptionsProxy** WxEditorFrame::FindOptionProxy(INT Key)
{
	check( OptionProxies );
	return OptionProxies->Find( Key );
}

wxMenu* WxEditorFrame::GetMRUMenu()
{
	return MainMenuBar->MRUMenu;
}

FMRUList* WxEditorFrame::GetMRUFiles()
{
	return MainMenuBar->MRUFiles;
}

void WxEditorFrame::OnSize( wxSizeEvent& InEvent )
{
	if( MainToolBar )
	{
		wxRect rc = GetClientRect();
		wxRect tbrc = MainToolBar->GetClientRect();
		INT ToolBarH = tbrc.GetHeight();

		wxRect rcmb( 0, 0, rc.GetWidth(), ModeBar ? ModeBar->GetRect().GetHeight() : 32 );

		if( rcmb.GetWidth() == 0 )
		{
			rcmb.width = 1000;
		}
		if( rcmb.GetHeight() == 0 )
		{
			rcmb.height = 32;
		}

		INT ModeBarH = 0;
		if( ModeBar )
		{
			ModeBar->SetSize( rcmb );
			ModeBarH = rcmb.GetHeight();
		}

		if( !ModeBarH && ModeBar && ModeBar->SavedHeight != -1 )
		{
			ModeBarH = ModeBar->SavedHeight;
			ModeBar->SetSize( rc.GetWidth(), ModeBarH );
		}

		ButtonBar->SetSize( 0, ModeBarH, LEFT_BUTTON_BAR_SZ, rc.GetHeight()-ModeBarH );

		// Figure out the client area remaining for viewports once the docked windows are taken into account

		wxSize OldSize = ViewportContainer->GetSize();
		wxSize NewSize( rc.GetWidth() - LEFT_BUTTON_BAR_SZ, rc.GetHeight() - ModeBarH );
		if ( bFrameMaximized != UBOOL(IsMaximized()) && ViewportConfigData )
		{
			ViewportConfigData->ResizeProportionally( FLOAT(NewSize.x)/FLOAT(OldSize.x), FLOAT(NewSize.y)/FLOAT(OldSize.y), FALSE );
		}
		ViewportContainer->SetSize( LEFT_BUTTON_BAR_SZ, ModeBarH, NewSize.x, NewSize.y );

		// SetSize() can be deferred for later, and Layout() doesn't recognize this. But it does use VirtualSize, if specified.
		ViewportContainer->SetVirtualSize( NewSize.x, NewSize.y );
		ViewportContainer->Layout();
		ViewportContainer->SetVirtualSize( -1, -1 );

		if ( ViewportConfigData )
		{
			ViewportConfigData->Layout();
		}

		// Apparently, OnSize is never called when minimizing. Which is good for us in this case. IsIconized() is always false.
		if ( !IsIconized() && !IsMaximized() && InEvent.GetSize().x != 0 && InEvent.GetSize().y != 0 )
		{
			FrameSize = InEvent.GetSize();
		}
		if ( !IsIconized() )
		{
			bFrameMaximized = IsMaximized() ? TRUE : FALSE;
		}
	}
}

void WxEditorFrame::OnMove( wxMoveEvent& InEvent )
{
	if ( !IsIconized() && !IsMaximized() )
	{
		FramePos = GetPosition();
	}
}

void WxEditorFrame::OnSplitterChanging( wxSplitterEvent& InEvent )
{
	// Prevent user from resizing if we've maximized a viewport
	if ( ViewportConfigData && ViewportConfigData->IsViewportMaximized() )
	{
		InEvent.Veto();
	}
}

void WxEditorFrame::OnSplitterDblClk( wxSplitterEvent& InEvent )
{
	// Always disallow double-clicking on the splitter bars. Default behavior is otherwise to unsplit.
	InEvent.Veto();
}

void WxEditorFrame::OnClose( wxCloseEvent& InEvent )
{
	check(GEngine);

	// if PIE is still happening, stop it before doing anything
	if (GEditor->PlayWorld)
	{
		GEditor->EndPlayMap();
	}

        // If a FaceFX Studio window is open we need to make sure it is closed in
	// a certain way so that it can prompt the user to save changes.
#if WITH_FACEFX
	if( OC3Ent::Face::FxStudioApp::GetMainWindow() )
	{
		OC3Ent::Face::FxStudioApp::GetMainWindow()->ProcessEvent(InEvent);
	}
#endif // WITH_FACEFX

	// if this returns false, then we don't want to actually shut down, and we should go back to 
	// true means to save the map (this way all package failures are handled the same)
	if (!GEngine->SaveDirtyPackages(true))
	{
		// if we are forcefully being close, we can't stop the rush
		if (!InEvent.CanVeto())
		{
			appErrorf(TEXT("The user didn't want to quit, but wxWindows is forcing us to quit."));
		}
		InEvent.Veto();
	}
	else
	{
		// otherwise, we destroy the window
		Destroy();
	}
}

/**
* Called when the application is minimized.
*/
void WxEditorFrame::OnIconize( wxIconizeEvent& InEvent )
{
	// Loop through all children and set them to a non-minimized state when the user restores the editor window.
	const UBOOL bMinimized = IsIconized();

	if( bMinimized == FALSE )
	{
		RestoreAllChildren();
	}
}

/**
 * Called when the application is maximized.
 */
void WxEditorFrame::OnMaximize( wxMaximizeEvent& InEvent )
{
	// Loop through all children and set them to a non-minimized state when the user restores the editor window.
	const UBOOL bMaximized = IsMaximized();

	if( bMaximized == TRUE )
	{
		RestoreAllChildren();
	}
}

/**
 * Restore's all minimized children.
 */
void WxEditorFrame::RestoreAllChildren()
{
	wxWindowList Children = GetChildren();

	wxWindowListNode *Node = Children.GetFirst();
	while (Node)
	{

		wxWindow *Window = Node->GetData();
		const UBOOL bCanBeRestored = Window->IsKindOf(CLASSINFO(wxTopLevelWindow));

		if(bCanBeRestored==TRUE)
		{
			wxTopLevelWindow *TopLevelWindow = wxDynamicCast(Window, wxTopLevelWindow);

			if(TopLevelWindow->IsIconized())
			{
				TopLevelWindow->Iconize(false);
			}
		}

		Node = Node->GetNext();
	}
}


void WxEditorFrame::MenuFileNew( wxCommandEvent& In )
{
	FEditorFileUtils::NewMap();
}

void WxEditorFrame::MenuFileOpen( wxCommandEvent& In )
{
	FEditorFileUtils::LoadMap();
}


void WxEditorFrame::MenuFileSave( wxCommandEvent& In )
{
	FEditorFileUtils::SaveLevel( GWorld->CurrentLevel );
}

void WxEditorFrame::MenuFileSaveAs( wxCommandEvent& In )
{
	FEditorFileUtils::SaveAs( GWorld );
}

void WxEditorFrame::MenuFileSaveAll( wxCommandEvent& In )
{
	FEditorFileUtils::SaveAllLevels( TRUE );
}

void WxEditorFrame::MenuFileForceSaveAll( wxCommandEvent& In )
{
	FEditorFileUtils::SaveAllLevels( FALSE );
}

void WxEditorFrame::MenuFileSaveAllLevels( wxCommandEvent& In )
{
	FEditorFileUtils::SaveAllLevels( TRUE );
}

void WxEditorFrame::MenuFileImportNew( wxCommandEvent& In )
{
	// Import, and ask for save first (bMerging flag set to FALSE).
	FEditorFileUtils::Import( FALSE );
}

void WxEditorFrame::MenuFileImportMerge( wxCommandEvent& In )
{
	// Import but don't bother saving since we're merging (bMerging flag set to TRUE).
	FEditorFileUtils::Import( TRUE );
}

void WxEditorFrame::MenuFileExportAll( wxCommandEvent& In )
{
	// Export with bExportSelectedActorsOnly set to FALSE.
	FEditorFileUtils::Export( FALSE );
}

void WxEditorFrame::MenuFileExportSelected( wxCommandEvent& In )
{
	// Export with bExportSelectedActorsOnly set to TRUE.
	FEditorFileUtils::Export( TRUE );
}

void WxEditorFrame::MenuFileMRU( wxCommandEvent& In )
{
	// Save the name of the file we are attempting to load as VerifyFile/AskSaveChanges might rearrange the MRU list on us
	const FFilename NewFilename = MainMenuBar->MRUFiles->GetItem( In.GetId() - IDM_MRU_START );
	
	if( MainMenuBar->MRUFiles->VerifyFile( In.GetId() - IDM_MRU_START ) )
	{
		// Prompt the user to save any outstanding changes.
		FEditorFileUtils::AskSaveChanges();

		// Load the requested level.
		FEditorFileUtils::LoadMap( NewFilename );

		MainMenuBar->MRUFiles->UpdateMenu();
	}
}

void WxEditorFrame::MenuFileExit( wxCommandEvent& In )
{
	// By calling Close instead of destroy, our Close Event handler will get called
	Close();
}

void WxEditorFrame::MenuEditUndo( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("TRANSACTION UNDO") );
}

void WxEditorFrame::MenuEditRedo( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("TRANSACTION REDO") );
}

void WxEditorFrame::MenuFarPlaneScrollChanged( wxCommandEvent& In )
{
	const int Value = In.GetInt();

	FLOAT Dists[80];
	FLOAT LastDist = 1024.f;
	for ( INT i = 0 ; i < 80 ; ++i )
	{
		Dists[i] = LastDist;
		LastDist *= 1.06f;
	}

	FLOAT SelectedDist = 0.f;
	if ( Value < 80 )
	{
		SelectedDist = Dists[Value];
	}
	GUnrealEd->Exec( *FString::Printf(TEXT("FARPLANE DIST=%f"), SelectedDist) );
}

void WxEditorFrame::MenuEditMouseLock( wxCommandEvent& In )
{
	GEditorModeTools().SetMouseLock( !GEditorModeTools().GetMouseLock() );
	GUnrealEd->RedrawAllViewports();
}

void WxEditorFrame::MenuEditShowWidget( wxCommandEvent& In )
{
	GEditorModeTools().SetShowWidget( !GEditorModeTools().GetShowWidget() );
	GUnrealEd->RedrawAllViewports();
}

void WxEditorFrame::MenuEditTranslate( wxCommandEvent& In )
{
	GEditorModeTools().SetWidgetMode( FWidget::WM_Translate );
	GUnrealEd->RedrawAllViewports();
}

void WxEditorFrame::MenuEditRotate( wxCommandEvent& In )
{
	GEditorModeTools().SetWidgetMode( FWidget::WM_Rotate );
	GUnrealEd->RedrawAllViewports();
}

void WxEditorFrame::MenuEditScale( wxCommandEvent& In )
{
	GEditorModeTools().SetWidgetMode( FWidget::WM_Scale );
	GUnrealEd->RedrawAllViewports();
}

void WxEditorFrame::MenuEditScaleNonUniform( wxCommandEvent& In )
{
	GEditorModeTools().SetWidgetMode( FWidget::WM_ScaleNonUniform );
	GUnrealEd->RedrawAllViewports();
}

void WxEditorFrame::CoordSystemSelChanged( wxCommandEvent& In )
{
	GEditorModeTools().CoordSystem = (ECoordSystem)In.GetInt();
	GUnrealEd->RedrawAllViewports();
}

void WxEditorFrame::MenuEditSearch( wxCommandEvent& In )
{
	GApp->DlgActorSearch->Show(1);
}

void WxEditorFrame::MenuEditCut( wxCommandEvent& In )
{
	//FString cmd = FString::Printf( TEXT("EDIT CUT CLIPPAD=%d"), ( GetAsyncKeyState(VK_SHIFT) & 0x8000 ) ? 1 : 0 );
	FString cmd = TEXT("EDIT CUT");
	GUnrealEd->Exec( *cmd );
}

void WxEditorFrame::MenuEditCopy( wxCommandEvent& In )
{
	//FString cmd = FString::Printf( TEXT("EDIT COPY CLIPPAD=%d"), ( GetAsyncKeyState(VK_SHIFT) & 0x8000 ) ? 1 : 0 );
	FString cmd = TEXT("EDIT COPY");
	GUnrealEd->Exec( *cmd );
}

void WxEditorFrame::MenuEditPasteOriginalLocation( wxCommandEvent& In )
{
	//FString cmd = FString::Printf( TEXT("EDIT PASTE CLIPPAD=%d"), ( GetAsyncKeyState(VK_SHIFT) & 0x8000 ) ? 1 : 0 );
	FString cmd = TEXT("EDIT PASTE");
	GUnrealEd->Exec( *cmd );
}

void WxEditorFrame::MenuEditPasteWorldOrigin( wxCommandEvent& In )
{
	//FString cmd = FString::Printf( TEXT("EDIT PASTE TO=ORIGIN CLIPPAD=%d"), ( GetAsyncKeyState(VK_SHIFT) & 0x8000 ) ? 1 : 0 );
	FString cmd = TEXT("EDIT PASTE TO=ORIGIN");
	GUnrealEd->Exec( *cmd );
}

void WxEditorFrame::MenuEditPasteHere( wxCommandEvent& In )
{
	//FString cmd = FString::Printf( TEXT("EDIT PASTE TO=HERE CLIPPAD=%d"), ( GetAsyncKeyState(VK_SHIFT) & 0x8000 ) ? 1 : 0 );
	FString cmd = TEXT("EDIT PASTE TO=HERE");
	GUnrealEd->Exec( *cmd );
}

void WxEditorFrame::MenuEditDuplicate( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("DUPLICATE") );
}

void WxEditorFrame::MenuEditDelete( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("DELETE") );
}

void WxEditorFrame::MenuEditSelectNone( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("SELECT NONE") );
}

void WxEditorFrame::MenuEditSelectAll( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("ACTOR SELECT ALL") );
}

void WxEditorFrame::MenuEditSelectByProperty( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("ACTOR SELECT BYPROPERTY") );
}

void WxEditorFrame::MenuEditSelectInvert( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("ACTOR SELECT INVERT") );
}

void WxEditorFrame::MenuTogglePrefabsLocked( wxCommandEvent& In )
{
	GUnrealEd->bPrefabsLocked = !GUnrealEd->bPrefabsLocked;
	GEditor->SaveConfig();
}

void WxEditorFrame::MenuViewFullScreen( wxCommandEvent& In )
{
	ShowFullScreen( !IsFullScreen(), wxFULLSCREEN_NOBORDER | wxFULLSCREEN_NOCAPTION );
}

void WxEditorFrame::MenuViewBrushPolys( wxCommandEvent& In )
{
	GEditor->Exec( *FString::Printf( TEXT("MODE SHOWBRUSHMARKERPOLYS=%d"), !GEditor->bShowBrushMarkerPolys ? 1 : 0 ) );
	GEditor->SaveConfig();
}

void WxEditorFrame::MenuViewDistributionToggle( wxCommandEvent& In )
{
	// @GEMINI_TODO: Less global var hack
	extern DWORD GDistributionType;
	GDistributionType ^= 1;
}

void WxEditorFrame::MenuViewCamSpeedSlow( wxCommandEvent& In )
{
	GEditor->MovementSpeed = MOVEMENTSPEED_SLOW;
}

void WxEditorFrame::MenuViewCamSpeedNormal( wxCommandEvent& In )
{
	GEditor->MovementSpeed = MOVEMENTSPEED_NORMAL;
}

void WxEditorFrame::MenuViewCamSpeedFast( wxCommandEvent& In )
{
	GEditor->MovementSpeed = MOVEMENTSPEED_FAST;
}

void WxEditorFrame::MenuViewportConfig( wxCommandEvent& In )
{
	EViewportConfig ViewportConfig = VC_2_2_Split;

	switch( In.GetId() )
	{
		case IDM_VIEWPORT_CONFIG_2_2_SPLIT:	ViewportConfig = VC_2_2_Split;	break;
		case IDM_VIEWPORT_CONFIG_1_2_SPLIT:	ViewportConfig = VC_1_2_Split;	break;
		case IDM_VIEWPORT_CONFIG_1_1_SPLIT:	ViewportConfig = VC_1_1_Split;	break;
	}

	SetViewportConfig( ViewportConfig );
}

void WxEditorFrame::MenuViewportResizeTogether(wxCommandEvent& In )
{
	if( In.IsChecked() )
	{
		bViewportResizeTogether = TRUE;

		//Make the splitter sashes match up.
		ViewportContainer->MatchSplitterPositions();
	}
	else
	{
		bViewportResizeTogether = FALSE;
	}
}

void WxEditorFrame::MenuViewShowBrowser( wxCommandEvent& In )
{
	GUnrealEd->GetBrowserManager()->ShowWindowByMenuID(In.GetId());
}

void WxEditorFrame::MenuOpenKismet( wxCommandEvent& In )
{
	WxKismet::OpenKismet( NULL, FALSE, GApp->EditorFrame );
}

void WxEditorFrame::MenuActorFindInKismet( wxCommandEvent& In )
{
	AActor* FindActor = GEditor->GetSelectedActors()->GetTop<AActor>();
	if(FindActor)
	{
		WxKismet::FindActorReferences(FindActor);
	}
}

void WxEditorFrame::MenuActorProperties( wxCommandEvent& In )
{
	GUnrealEd->ShowActorProperties();
}

void WxEditorFrame::MenuFileCreateArchetype( wxCommandEvent& In )
{
	GEditor->edactArchetypeSelected();
}

void WxEditorFrame::MenuSyncGenericBrowser( wxCommandEvent& In )
{
	TArray<UObject*> Objects;

	// If the user has any BSP surfaces selected, sync to the materials on them.
	UBOOL bFoundSurfaceMaterial = FALSE;

	for ( TSelectedSurfaceIterator<> It ; It ; ++It )
	{
		FBspSurf* Surf = *It;
		UMaterialInterface* Material = Surf->Material;
		if( Material )
		{
			Objects.AddUniqueItem( Material );
			bFoundSurfaceMaterial = TRUE;
			//break;
		}
	}

	// Otherwise, assemble a list of resources from selected actors.
	if( !bFoundSurfaceMaterial )
	{
		for ( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
		{
			AActor* Actor						= static_cast<AActor*>( *It );
			checkSlow( Actor->IsA(AActor::StaticClass()) );

			AStaticMeshActor* StaticMesh		= Cast<AStaticMeshActor>( Actor );
			ADynamicSMActor* DynamicSM			= Cast<ADynamicSMActor>( Actor );
			ASkeletalMeshActor* SkeletalMesh	= Cast<ASkeletalMeshActor>( Actor );
			AEmitter* Emitter					= Cast<AEmitter>( Actor );
			ADecalActor* Decal					= Cast<ADecalActor>( Actor );
			APrefabInstance* PrefabInstance		= Cast<APrefabInstance>( Actor );
			AKAsset* KAsset						= Cast<AKAsset>( Actor );

			UObject* CurObject					= NULL;
			if( StaticMesh )
			{
				CurObject = StaticMesh->StaticMeshComponent->StaticMesh;
			}
			else if( DynamicSM )
			{
				CurObject = DynamicSM->StaticMeshComponent->StaticMesh;
			}
			else if( SkeletalMesh )
			{
				CurObject = SkeletalMesh->SkeletalMeshComponent->SkeletalMesh;
			}
			else if( Emitter )
			{
				CurObject = Emitter->ParticleSystemComponent->Template;
			}
			else if( Decal )
			{
				CurObject = Decal->Decal->DecalMaterial;
			}
			else if( PrefabInstance )
			{
				CurObject = PrefabInstance->TemplatePrefab;
			}
			else if( KAsset )
			{
				CurObject = KAsset->SkeletalMeshComponent->PhysicsAsset;
			}

			// If an object was found, add it to the list.
			if ( CurObject )
			{
				Objects.AddItem( CurObject );
			}
		}
	}

	// Sync the generic browser to the object list.
	if ( Objects.Num() > 0 )
	{
		WxGenericBrowser* GenericBrowser = GUnrealEd->GetBrowser<WxGenericBrowser>( TEXT("GenericBrowser") );
		if ( GenericBrowser )
		{
			// Make sure the window is visible.  The window needs to be visible *before*
			// the browser is sync'd to objects so that redraws actually happen!
			GUnrealEd->GetBrowserManager()->ShowWindow( GenericBrowser->GetDockID(), TRUE );

			// Sync.
			GenericBrowser->SyncToObjects( Objects );
		}
	}
}

void WxEditorFrame::MenuMakeSelectedActorsLevelCurrent( wxCommandEvent& In )
{
	GUnrealEd->MakeSelectedActorsLevelCurrent();
}

void WxEditorFrame::MenuMoveSelectedActorsToCurrentLevel( wxCommandEvent& In )
{
    GEditor->MoveSelectedActorsToCurrentLevel();
}

void WxEditorFrame::MenuSelectLevelInLevelBrowser( wxCommandEvent& In )
{
	WxLevelBrowser* LevelBrowser = GUnrealEd->GetBrowser<WxLevelBrowser>( TEXT("LevelBrowser") );
	if ( LevelBrowser != NULL )
	{
		for ( USelection::TObjectIterator Itor = GEditor->GetSelectedActors()->ObjectItor() ; Itor ; ++Itor )
		{
			AActor* Actor = Cast<AActor>( *Itor );
			if ( Actor )
			{
				ULevel* ActorLevel = Actor->GetLevel();
				LevelBrowser->SelectLevel( ActorLevel );
			}
		}

		// Make sure the window is visible.
		GUnrealEd->GetBrowserManager()->ShowWindow( LevelBrowser->GetDockID(), TRUE );
	}
}

void WxEditorFrame::MenuSelectLevelOnlyInLevelBrowser( wxCommandEvent& In )
{
	WxLevelBrowser* LevelBrowser = GUnrealEd->GetBrowser<WxLevelBrowser>( TEXT("LevelBrowser") );
	if ( LevelBrowser != NULL )
	{
		LevelBrowser->DeselectAllLevels();

		for ( USelection::TObjectIterator Itor = GEditor->GetSelectedActors()->ObjectItor() ; Itor ; ++Itor )
		{
			AActor* Actor = Cast<AActor>( *Itor );
			if ( Actor )
			{
				ULevel* ActorLevel = Actor->GetLevel();
				LevelBrowser->SelectLevel( ActorLevel );
			}
		}

		// Make sure the window is visible.
		GUnrealEd->GetBrowserManager()->ShowWindow( LevelBrowser->GetDockID(), TRUE );
	}
}

void WxEditorFrame::MenuDeselectLevelInLevelBrowser( wxCommandEvent& In )
{
	WxLevelBrowser* LevelBrowser = GUnrealEd->GetBrowser<WxLevelBrowser>( TEXT("LevelBrowser") );
	if ( LevelBrowser != NULL )
	{
		for ( USelection::TObjectIterator Itor = GEditor->GetSelectedActors()->ObjectItor() ; Itor ; ++Itor )
		{
			AActor* Actor = Cast<AActor>( *Itor );
			if ( Actor )
			{
				ULevel* ActorLevel = Actor->GetLevel();
				LevelBrowser->DeselectLevel( ActorLevel );
			}
		}

		// Make sure the window is visible.
		GUnrealEd->GetBrowserManager()->ShowWindow( LevelBrowser->GetDockID(), TRUE );
	}
}

void WxEditorFrame::MenuSurfaceProperties( wxCommandEvent& In )
{
	GApp->DlgSurfProp->Show( 1 );
}

void WxEditorFrame::MenuWorldProperties( wxCommandEvent& In )
{
	GUnrealEd->ShowWorldProperties();
}

void WxEditorFrame::MenuBrushCSG( wxCommandEvent& In )
{
	switch( In.GetId() )
	{
		case IDM_BRUSH_ADD:				GUnrealEd->Exec( TEXT("BRUSH ADD") );					break;
		case IDM_BRUSH_SUBTRACT:		GUnrealEd->Exec( TEXT("BRUSH SUBTRACT") );				break;
		case IDM_BRUSH_INTERSECT:		GUnrealEd->Exec( TEXT("BRUSH FROM INTERSECTION") );		break;
		case IDM_BRUSH_DEINTERSECT:		GUnrealEd->Exec( TEXT("BRUSH FROM DEINTERSECTION") );	break;
	}

	GUnrealEd->RedrawLevelEditingViewports();
}

void WxEditorFrame::MenuBrushAddSpecial( wxCommandEvent& In )
{
	//GDlgAddSpecial->Show();
}

void WxEditorFrame::MenuBuildPlayInEditor( wxCommandEvent& In )
{
	GUnrealEd->PlayMap();
}

void WxEditorFrame::MenuBuildPlayOnConsole( wxCommandEvent& In )
{
	GUnrealEd->PlayMap(NULL, NULL, In.GetId() - IDM_BuildPlayConsole_START);
}

void WxEditorFrame::MenuConsoleSpecific( wxCommandEvent& In )
{
	// Get the index, with the first one possible at 0
	const INT Index = In.GetId() - IDM_ConsoleSpecific_START;
	// There are 40 items per console, so every 40 items is a new console
	const INT Console = Index / MAX_CONSOLES_TO_DISPLAY_IN_MENU;
	// Use mod to get the index inside the console
	const INT MenuItem = Index % MAX_CONSOLES_TO_DISPLAY_IN_MENU;

	// Let the plug-in manage it
	TCHAR OutputConsoleCommand[1024] = TEXT("\0");
	GConsoleSupportContainer->GetConsoleSupport(Console)->ProcessMenuItem(MenuItem, OutputConsoleCommand, sizeof(OutputConsoleCommand));

	// if the meun item needs to exec a command, do it now
	if (OutputConsoleCommand[0] != 0)
	{
		// handle spcial QUERYVALUE command
		UBOOL bIsQueryingValue = appStrnicmp(OutputConsoleCommand, TEXT("QUERYVALUE"), 10) == 0;
		FStringOutputDevice QueryOutputDevice;

		FOutputDevice& Ar = bIsQueryingValue ? (FOutputDevice&)QueryOutputDevice : (FOutputDevice&)*GLog;
		GEngine->Exec(OutputConsoleCommand, Ar);
		if (bIsQueryingValue)
		{
			GConsoleSupportContainer->GetConsoleSupport(Console)->SetValueCallback(*QueryOutputDevice);
		}
	}

	// just in case the object propagation IP address changed, let the editor update the addr
	GEditor->UpdateObjectPropagationIPAddress();
}

void WxEditorFrame::UpdateUIConsoleSpecific(wxUpdateUIEvent& In)
{
	// Get the index, with the first one possible at 0
	const INT Index = In.GetId() - IDM_ConsoleSpecific_START;
	// There are MAX_CONSOLES_TO_DISPLAY_IN_MENU items per console, so every MAX_CONSOLES_TO_DISPLAY_IN_MENU items is a new console
	const INT Console = Index / MAX_CONSOLES_TO_DISPLAY_IN_MENU;
	// Use mod to get the index inside the console
	const INT MenuItem = Index % MAX_CONSOLES_TO_DISPLAY_IN_MENU;

	// reget the menu item text, in case it changed by selecting it
	bool bIsChecked = false;
	bool bIsRadio = false;
	const TCHAR* MenuLabel = GConsoleSupportContainer->GetConsoleSupport(Console)->GetMenuItem(MenuItem, bIsChecked, bIsRadio);
	MainMenuBar->ConsoleMenu[Console]->SetLabel(IDM_ConsoleSpecific_START + Console * MAX_CONSOLES_TO_DISPLAY_IN_MENU + MenuItem, MenuLabel);

	// update checked status
	In.Check(bIsChecked);
}

void WxEditorFrame::ObjectPropagationSelChanged( wxCommandEvent& In )
{
	// TRUE if propagation destination is 'None'
	const UBOOL bTargetNotNone = In.GetInt() != OPD_None;

	// Enable/disable the ViewPush button based on whether the object propagation destination is 'None'.
	MainToolBar->EnablePushView( bTargetNotNone );

	if ( bTargetNotNone )
	{
		MessageBoxA(NULL, 
			"Live Update/Object Propagation supports the following:\n"
			" Basic:\n"
			"   - Moving and deleting non-light actors\n"
			" Creation:\n"
			"   - Actor duplication and copy & paste of valid actors\n"
			"   - Creation of StaticMesh, SkeletalMesh, and AmbientSound actors (ONLY when using loaded content)\n"
			" Properties:\n"
			"   - Changes made in actor/component property dialogs (other property dialogs _may_ work)\n"
			"\n"
			"The following will not work:\n"
			" - Moving/deleting/modifying lights\n"
			" - Making references to unloaded (on the console) content\n"
			" - Modifying content (textures, materials, etc)\n",
			"\n"
			"\n"
			"Note: Xbox 360's will need secure networking disabled! (bUseVDP = false)"
			"Live Update support information",
			MB_ICONWARNING | MB_OK);
	}

	// let the editor handle the logic
	GEditor->SetObjectPropagationDestination(In.GetInt());
}

#include "UnPath.h"

void WxEditorFrame::MenuBuild( wxCommandEvent& In )
{
	GWarn->MapCheck_Clear();

	// Make sure to set this flag to FALSE before ALL builds.
	GEditor->SetMapBuildCancelled( FALSE );

	// Show the build progress dialog.
	GApp->DlgBuildProgress->ShowDialog();
	GApp->DlgBuildProgress->MarkBuildStartTime();

	// Will be set to FALSE if, for some reason, the build does not happen.
	UBOOL bDoBuild = TRUE;
	// Indicates whether the persistent level should be dirtied at the end of a build.
	UBOOL bDirtyPersistentLevel = TRUE;

	// Stop rendering thread so we're not wasting CPU cycles.
	StopRenderingThread();

	const FString HiddenLevelWarning( FString::Printf(TEXT("%s"), *LocalizeUnrealEd(TEXT("HiddenLevelsContinueWithBuildQ")) ) );
	switch( In.GetId() )
	{
	case IDM_BUILD_GEOMETRY:
		{
			// We can't set the busy cursor for all windows, because lighting
			// needs a cursor for the lighting options dialog.
			const FScopedBusyCursor BusyCursor;

			GUnrealEd->Exec( TEXT("MAP REBUILD") );

			// No need to dirty the persient level if we're building BSP for a sub-level.
			bDirtyPersistentLevel = FALSE;
			break;
		}
	case IDM_BUILD_VISIBLEGEOMETRY:
		{
			// We can't set the busy cursor for all windows, because lighting
			// needs a cursor for the lighting options dialog.
			const FScopedBusyCursor BusyCursor;

			GUnrealEd->Exec( TEXT("MAP REBUILD ALLVISIBLE") );
			break;
		}

	case IDM_BUILD_LIGHTING:
		{
			FLightingBuildOptions LightingBuildOptions;
			WxDlgLightingBuildOptions LightingBuildOptionsDialog;
			if( LightingBuildOptionsDialog.ShowModal( LightingBuildOptions ) )
			{
				// We can't set the busy cursor for all windows, because lighting
				// needs a cursor for the lighting options dialog.
				const FScopedBusyCursor BusyCursor;
				GUnrealEd->BuildLighting( LightingBuildOptions );
			}
			break;
		}

	case IDM_BUILD_AI_PATHS:
		{
			bDoBuild = GEditor->WarnAboutHiddenLevels( *HiddenLevelWarning );
			if ( bDoBuild )
			{
				// We can't set the busy cursor for all windows, because lighting
				// needs a cursor for the lighting options dialog.
				const FScopedBusyCursor BusyCursor;

				FPathBuilder::Exec( TEXT("DEFINEPATHS REVIEWPATHS=1 SHOWMAPCHECK=0") );
				FPathBuilder::Exec( TEXT("BUILDCOVER FROMDEFINEPATHS=0") );
				FPathBuilder::Exec( TEXT("BUILDCOMBATZONES FROMDEFINEPATHS=0") );
			}

			break;
		}

	case IDM_BUILD_COVER:
		{
			bDoBuild = GEditor->WarnAboutHiddenLevels( *HiddenLevelWarning );
			if ( bDoBuild )
			{
				// We can't set the busy cursor for all windows, because lighting
				// needs a cursor for the lighting options dialog.
				const FScopedBusyCursor BusyCursor;

				FPathBuilder::Exec( TEXT("BUILDCOVER FROMDEFINEPATHS=0") );
				FPathBuilder::Exec( TEXT("BUILDCOMBATZONES FROMDEFINEPATHS=0") );
			}

			break;
		}

	case IDM_BUILD_ALL:
		{
			bDoBuild = GEditor->WarnAboutHiddenLevels( *HiddenLevelWarning );
			if ( bDoBuild )
			{
				// We can't set the busy cursor for all windows, because lighting
				// needs a cursor for the lighting options dialog.
				const FScopedBusyCursor BusyCursor;

				GUnrealEd->Exec( TEXT("MAP REBUILD ALLVISIBLE") );

				//Do a canceled check before moving on to the next step of the build.
				if( GEditor->GetMapBuildCancelled() )
				{
					break;
				}

				GUnrealEd->RebuildBSP();

				//Do a canceled check before moving on to the next step of the build.
				if( GEditor->GetMapBuildCancelled() )
				{
					break;
				}

				FLightingBuildOptions LightingOptions;
				GUnrealEd->BuildLighting( LightingOptions );

				//Do a canceled check before moving on to the next step of the build.
				if( GEditor->GetMapBuildCancelled() )
				{
					break;
				}

				FPathBuilder::Exec( TEXT("DEFINEPATHS REVIEWPATHS=1 SHOWMAPCHECK=0") );
				
				//Do a canceled check before moving on to the next step of the build.
				if( GEditor->GetMapBuildCancelled() )
				{
					break;
				}

				FPathBuilder::Exec( TEXT("BUILDCOVER FROMDEFINEPATHS=0") );
			}
			break;
		}

	default:
		break;
	}

	// Re-start the rendering thread after build operations completed.
	if (GUseThreadedRendering)
	{
		StartRenderingThread();
	}

	if ( bDoBuild )
	{
		// Display elapsed build time.
		debugf( TEXT("Build time %s"), *GApp->DlgBuildProgress->BuildElapsedTimeString() );
		GUnrealEd->IssueDecalUpdateRequest();
	}

	// Build completed, hide the build progress dialog.
	GApp->DlgBuildProgress->Show( false );

	GUnrealEd->RedrawLevelEditingViewports();

	if ( bDoBuild )
	{
		if ( bDirtyPersistentLevel )
		{
			GWorld->MarkPackageDirty();
		}
		GCallbackEvent->Send( CALLBACK_LevelDirtied );
	}

	// Don't show map check if we cancelled build because it may have some bogus data
	const UBOOL bBuildCompleted = bDoBuild && !GEditor->GetMapBuildCancelled();
	if( bBuildCompleted )
	{
		GWarn->MapCheck_ShowConditionally();
	}

	GEditor->ResetAutosaveTimer();
}

void WxEditorFrame::UI_MenuViewportConfig( wxUpdateUIEvent& In )
{
	switch( In.GetId() )
	{
		case IDM_VIEWPORT_CONFIG_2_2_SPLIT:		In.Check( ViewportConfigData->Template == VC_2_2_Split );			break;
		case IDM_VIEWPORT_CONFIG_1_2_SPLIT:		In.Check( ViewportConfigData->Template == VC_1_2_Split );			break;
		case IDM_VIEWPORT_CONFIG_1_1_SPLIT:		In.Check( ViewportConfigData->Template == VC_1_1_Split );			break;
	}
}

void WxEditorFrame::UI_MenuEditUndo( wxUpdateUIEvent& In )
{
	In.Enable( GUnrealEd->Trans->CanUndo() == TRUE );
	In.SetText( *FString::Printf( *LocalizeUnrealEd("Undo_F"), *GUnrealEd->Trans->GetUndoDesc() ) );

}

void WxEditorFrame::UI_MenuEditRedo( wxUpdateUIEvent& In )
{
	In.Enable( GUnrealEd->Trans->CanRedo() == TRUE );
	In.SetText( *FString::Printf( *LocalizeUnrealEd("Redo_F"), *GUnrealEd->Trans->GetRedoDesc() ) );

}

void WxEditorFrame::UI_MenuEditShowWidget( wxUpdateUIEvent& In )
{
	In.Check( GEditorModeTools().GetShowWidget() == TRUE );
}

void WxEditorFrame::UI_MenuEditMouseLock( wxUpdateUIEvent& In )
{
	In.Check( GEditorModeTools().GetMouseLock() == TRUE );
}

void WxEditorFrame::UI_MenuEditTranslate( wxUpdateUIEvent& In )
{
	In.Check( GEditorModeTools().GetWidgetMode() == FWidget::WM_Translate );
	In.Enable( GEditorModeTools().GetShowWidget() == TRUE );
}

void WxEditorFrame::UI_MenuEditRotate( wxUpdateUIEvent& In )
{
	In.Check( GEditorModeTools().GetWidgetMode() == FWidget::WM_Rotate );
	In.Enable( GEditorModeTools().GetShowWidget() == TRUE );
}

void WxEditorFrame::UI_MenuEditScale( wxUpdateUIEvent& In )
{
	In.Check( GEditorModeTools().GetWidgetMode() == FWidget::WM_Scale );
	In.Enable( GEditorModeTools().GetShowWidget() == TRUE );
}

void WxEditorFrame::UI_MenuEditScaleNonUniform( wxUpdateUIEvent& In )
{
	In.Check( GEditorModeTools().GetWidgetMode() == FWidget::WM_ScaleNonUniform );

	// Special handling

	switch( GEditorModeTools().GetWidgetMode() )
	{
		// Non-uniform scaling is only possible in local space

		case FWidget::WM_ScaleNonUniform:
			GEditorModeTools().CoordSystem = COORD_Local;
			MainToolBar->CoordSystemCombo->SetSelection( GEditorModeTools().CoordSystem );
			MainToolBar->CoordSystemCombo->Disable();
			break;

		default:
			MainToolBar->CoordSystemCombo->Enable();
			break;
	}

	In.Enable( GEditorModeTools().GetShowWidget() == TRUE );
}

void WxEditorFrame::UI_MenuDragGrid( wxUpdateUIEvent& In )
{
	INT id = In.GetId();

	if( IDM_DRAG_GRID_TOGGLE == id )
	{
		In.Check( GUnrealEd->Constraints.GridEnabled );
	}
	else if( IDM_DRAG_GRID_SNAPSCALE == id )
	{
		In.Check( GUnrealEd->Constraints.SnapScaleEnabled );
	}
	else 
	{
		INT GridIndex;

		GridIndex = -1;

		if( id >= IDM_DRAG_GRID_1 && id <= IDM_DRAG_GRID_1024 )
		{
			GridIndex = id - IDM_DRAG_GRID_1 ;
		}
		else if( id >= ID_BackdropPopupGrid1 && id <= ID_BackdropPopupGrid1024 )
		{
			GridIndex = id - ID_BackdropPopupGrid1 ;
		}

		if( GridIndex >= 0 && GridIndex < FEditorConstraints::MAX_GRID_SIZES )
		{
			float GridSize;

			GridSize = GEditor->Constraints.GridSizes[GridIndex];
			In.SetText( *FString::Printf(TEXT("%g"), GridSize ) ) ;
			In.Check( GUnrealEd->Constraints.CurrentGridSz == GridIndex );
		}
	}
}

void WxEditorFrame::UI_MenuRotationGrid( wxUpdateUIEvent& In )
{
	switch( In.GetId() )
	{
		case IDM_ROTATION_GRID_TOGGLE:	In.Check( GUnrealEd->Constraints.RotGridEnabled );	break;
		case IDM_ROTATION_GRID_512:		In.Check( GUnrealEd->Constraints.RotGridSize.Pitch == 512 );		break;
		case IDM_ROTATION_GRID_1024:	In.Check( GUnrealEd->Constraints.RotGridSize.Pitch == 1024 );		break;
		case IDM_ROTATION_GRID_4096:	In.Check( GUnrealEd->Constraints.RotGridSize.Pitch == 4096 );		break;
		case IDM_ROTATION_GRID_8192:	In.Check( GUnrealEd->Constraints.RotGridSize.Pitch == 8192 );		break;
		case IDM_ROTATION_GRID_16384:	In.Check( GUnrealEd->Constraints.RotGridSize.Pitch == 16384 );		break;
	}
}

void WxEditorFrame::UI_MenuScaleGrid( wxUpdateUIEvent& In )
{
	switch( In.GetId() )
	{
		case IDM_DRAG_GRID_SNAPSCALE:	In.Check( GUnrealEd->Constraints.SnapScaleEnabled );	break;
		case IDM_SCALE_GRID_001:	In.Check( GUnrealEd->Constraints.ScaleGridSize == 1 );		break;
		case IDM_SCALE_GRID_002:	In.Check( GUnrealEd->Constraints.ScaleGridSize == 2 );		break;
		case IDM_SCALE_GRID_005:	In.Check( GUnrealEd->Constraints.ScaleGridSize == 5 );		break;
		case IDM_SCALE_GRID_010:	In.Check( GUnrealEd->Constraints.ScaleGridSize == 10 );		break;
		case IDM_SCALE_GRID_025:	In.Check( GUnrealEd->Constraints.ScaleGridSize == 25 );		break;
		case IDM_SCALE_GRID_050:	In.Check( GUnrealEd->Constraints.ScaleGridSize == 50 );		break;
	}
}

void WxEditorFrame::MenuDragGrid( wxCommandEvent& In )
{
	INT id = In.GetId();

	if( IDM_DRAG_GRID_TOGGLE == id )
	{	
		GUnrealEd->Exec( *FString::Printf( TEXT("MODE GRID=%d"), !GUnrealEd->Constraints.GridEnabled ? 1 : 0 ) );	
	}
	else 
	{
		INT GridIndex;

		GridIndex = -1;

		if( id >= IDM_DRAG_GRID_1 && id <= IDM_DRAG_GRID_1024 )
		{
			GridIndex = id - IDM_DRAG_GRID_1;
		}
		else if( id >= ID_BackdropPopupGrid1 && id <= ID_BackdropPopupGrid1024 )
		{
			GridIndex = id - ID_BackdropPopupGrid1;
		}

		if( GridIndex >= 0 && GridIndex < FEditorConstraints::MAX_GRID_SIZES )
		{
			GEditor->Constraints.SetGridSz( GridIndex );
		}
	}
}

void WxEditorFrame::MenuRotationGrid( wxCommandEvent& In )
{
	INT Angle = 0;
	switch( In.GetId() )
	{
		case IDM_ROTATION_GRID_512:			Angle = 512;		break;
		case IDM_ROTATION_GRID_1024:		Angle = 1024;		break;
		case IDM_ROTATION_GRID_4096:		Angle = 4096;		break;
		case IDM_ROTATION_GRID_8192:		Angle = 8192;		break;
		case IDM_ROTATION_GRID_16384:		Angle = 16384;		break;
	}

	switch( In.GetId() )
	{
		case IDM_ROTATION_GRID_TOGGLE:
			GUnrealEd->Exec( *FString::Printf( TEXT("MODE ROTGRID=%d"), !GUnrealEd->Constraints.RotGridEnabled ? 1 : 0 ) );
			break;

		case IDM_ROTATION_GRID_512:
		case IDM_ROTATION_GRID_1024:
		case IDM_ROTATION_GRID_4096:
		case IDM_ROTATION_GRID_8192:
		case IDM_ROTATION_GRID_16384:
			GUnrealEd->Exec( *FString::Printf( TEXT("MAP ROTGRID PITCH=%d YAW=%d ROLL=%d"), Angle, Angle, Angle ) );
			break;
	}
}


void WxEditorFrame::MenuAutoSaveInterval( wxCommandEvent& In )
{
	INT Interval = 0;
	switch( In.GetId() )
	{
	case IDM_AUTOSAVE_001:		Interval = 1;		break;
	case IDM_AUTOSAVE_002:		Interval = 5;		break;
	case IDM_AUTOSAVE_003:		Interval = 10;		break;
	case IDM_AUTOSAVE_004:		Interval = 15;		break;
	case IDM_AUTOSAVE_005:		Interval = 30;		break;
	}

	GUnrealEd->AutosaveTimeMinutes = Interval;

}

void WxEditorFrame::MenuScaleGrid( wxCommandEvent& In )
{
	if( IDM_DRAG_GRID_SNAPSCALE == In.GetId() )
	{	
		GUnrealEd->Constraints.SnapScaleEnabled = !GUnrealEd->Constraints.SnapScaleEnabled;
	}
	else
	{
		INT Scale = 0;

		switch( In.GetId() )
		{
		case IDM_SCALE_GRID_001:		Scale = 1;		break;
		case IDM_SCALE_GRID_002:		Scale = 2;		break;
		case IDM_SCALE_GRID_005:		Scale = 5;		break;
		case IDM_SCALE_GRID_010:		Scale = 10;		break;
		case IDM_SCALE_GRID_025:		Scale = 25;		break;
		case IDM_SCALE_GRID_050:		Scale = 50;		break;
		}

		GEditor->Constraints.ScaleGridSize = Scale;
	}

	UpdateUI();
}

void WxEditorFrame::UI_MenuViewResizeViewportsTogether( wxUpdateUIEvent& In )
{
	In.Check( bViewportResizeTogether == TRUE );
}


void WxEditorFrame::UI_MenuViewFullScreen( wxUpdateUIEvent& In )
{
	In.Check( IsFullScreen() );
}

void WxEditorFrame::UI_MenuViewBrushPolys( wxUpdateUIEvent& In )
{
	In.Check( GEditor->bShowBrushMarkerPolys );
}

void WxEditorFrame::UI_MenuTogglePrefabLock( wxUpdateUIEvent& In )
{
	In.Check( GEditor->bPrefabsLocked );
}

void WxEditorFrame::UI_MenuViewDistributionToggle( wxUpdateUIEvent& In )
{
	// @GEMINI_TODO: Less global var hack
	extern DWORD GDistributionType;
	In.Check(GDistributionType == 0);
}

void WxEditorFrame::UI_MenuViewCamSpeedSlow( wxUpdateUIEvent& In )
{
	In.Check( GEditor->MovementSpeed == MOVEMENTSPEED_SLOW );
}

void WxEditorFrame::UI_MenuViewCamSpeedNormal( wxUpdateUIEvent& In )
{
	In.Check( GEditor->MovementSpeed == MOVEMENTSPEED_NORMAL );
}

void WxEditorFrame::UI_MenuViewCamSpeedFast( wxUpdateUIEvent& In )
{
	In.Check( GEditor->MovementSpeed == MOVEMENTSPEED_FAST );
}

void WxEditorFrame::UI_ContextMenuMakeCurrentLevel( wxUpdateUIEvent& In )
{
	// Look to the selected actors for the level to make current.
	// If actors from multiple levels are selected, disable the
	// the "Make selected actor's level current" context menu item.
	ULevel* LevelToMakeCurrent = NULL;
	for ( USelection::TObjectIterator It = GEditor->GetSelectedActors()->ObjectItor() ; It ; ++It )
	{
		AActor* Actor = Cast<AActor>( *It );
		if ( Actor )
		{
			ULevel* ActorLevel = Actor->GetLevel();
			if ( !LevelToMakeCurrent )
			{
				// First assignment.
				LevelToMakeCurrent = ActorLevel;
			}
			else if ( LevelToMakeCurrent != ActorLevel )
			{
				// Actors from multiple levels are selected -- abort.
				LevelToMakeCurrent = NULL;
				break;
			}
		}
	}
	In.Enable( LevelToMakeCurrent != NULL );
}

void WxEditorFrame::CoverEdit_ToggleEnabled( wxCommandEvent& In )
{
	TArray<ACoverLink*> SelectedLinks;
	if (GEditor->GetSelectedActors()->GetSelectedObjects<ACoverLink>(SelectedLinks) > 0)
	{
		for (INT Idx = 0; Idx < SelectedLinks.Num(); Idx++)
		{
			ACoverLink *Link = SelectedLinks(Idx);
			for (INT SlotIdx = 0; SlotIdx < Link->Slots.Num(); SlotIdx++)
			{
				if (Link->Slots(SlotIdx).bSelected)
				{
					Link->Slots(SlotIdx).bEnabled = !Link->Slots(SlotIdx).bEnabled;
				}
			}
		}
	}
}

void WxEditorFrame::CoverEdit_ToggleType( wxCommandEvent& In )
{
	TArray<ACoverLink*> SelectedLinks;
	if (GEditor->GetSelectedActors()->GetSelectedObjects<ACoverLink>(SelectedLinks) > 0)
	{
		for (INT Idx = 0; Idx < SelectedLinks.Num(); Idx++)
		{
			ACoverLink *Link = SelectedLinks(Idx);
			for (INT SlotIdx = 0; SlotIdx < Link->Slots.Num(); SlotIdx++)
			{
				if (Link->Slots(SlotIdx).bSelected)
				{
					if (Link->Slots(SlotIdx).ForceCoverType == CT_Standing)
					{
						Link->Slots(SlotIdx).ForceCoverType = CT_MidLevel;
					}
					else if (Link->Slots(SlotIdx).ForceCoverType == CT_MidLevel)
					{
						Link->Slots(SlotIdx).ForceCoverType = CT_None;
					}
					else
					{
						Link->Slots(SlotIdx).ForceCoverType = CT_Standing;
					}

					Link->AutoAdjustSlot(SlotIdx,TRUE);
				}
			}
		}
	}
}

void WxEditorFrame::CoverEdit_ToggleCoverslip( wxCommandEvent& In )
{
	TArray<ACoverLink*> SelectedLinks;
	if (GEditor->GetSelectedActors()->GetSelectedObjects<ACoverLink>(SelectedLinks) > 0)
	{
		for (INT Idx = 0; Idx < SelectedLinks.Num(); Idx++)
		{
			ACoverLink *Link = SelectedLinks(Idx);
			for (INT SlotIdx = 0; SlotIdx < Link->Slots.Num(); SlotIdx++)
			{
				if (Link->Slots(SlotIdx).bSelected)
				{
					Link->Slots(SlotIdx).bAllowCoverSlip = !Link->Slots(SlotIdx).bAllowCoverSlip;
				}
			}
		}
	}
}

void WxEditorFrame::CoverEdit_ToggleSwatTurn( wxCommandEvent& In )
{
	TArray<ACoverLink*> SelectedLinks;
	if (GEditor->GetSelectedActors()->GetSelectedObjects<ACoverLink>(SelectedLinks) > 0)
	{
		for (INT Idx = 0; Idx < SelectedLinks.Num(); Idx++)
		{
			ACoverLink *Link = SelectedLinks(Idx);
			for (INT SlotIdx = 0; SlotIdx < Link->Slots.Num(); SlotIdx++)
			{
				if (Link->Slots(SlotIdx).bSelected)
				{
					Link->Slots(SlotIdx).bAllowSwatTurn = !Link->Slots(SlotIdx).bAllowSwatTurn;
				}
			}
		}
	}
}

void WxEditorFrame::CoverEdit_ToggleMantle( wxCommandEvent& In )
{
	TArray<ACoverLink*> SelectedLinks;
	if (GEditor->GetSelectedActors()->GetSelectedObjects<ACoverLink>(SelectedLinks) > 0)
	{
		for (INT Idx = 0; Idx < SelectedLinks.Num(); Idx++)
		{
			ACoverLink *Link = SelectedLinks(Idx);
			for (INT SlotIdx = 0; SlotIdx < Link->Slots.Num(); SlotIdx++)
			{
				if (Link->Slots(SlotIdx).bSelected)
				{
					Link->Slots(SlotIdx).bAllowMantle = !Link->Slots(SlotIdx).bAllowMantle;
				}
			}
		}
	}
}

void WxEditorFrame::CoverEdit_TogglePopup( wxCommandEvent& In )
{
	TArray<ACoverLink*> SelectedLinks;
	if (GEditor->GetSelectedActors()->GetSelectedObjects<ACoverLink>(SelectedLinks) > 0)
	{
		for (INT Idx = 0; Idx < SelectedLinks.Num(); Idx++)
		{
			ACoverLink *Link = SelectedLinks(Idx);
			for (INT SlotIdx = 0; SlotIdx < Link->Slots.Num(); SlotIdx++)
			{
				if (Link->Slots(SlotIdx).bSelected)
				{
					Link->Slots(SlotIdx).bAllowPopup = !Link->Slots(SlotIdx).bAllowPopup;
				}
			}
		}
	}
}

void WxEditorFrame::CoverEdit_ToggleClimbUp( wxCommandEvent& In )
{
	TArray<ACoverLink*> SelectedLinks;
	if (GEditor->GetSelectedActors()->GetSelectedObjects<ACoverLink>(SelectedLinks) > 0)
	{
		for (INT Idx = 0; Idx < SelectedLinks.Num(); Idx++)
		{
			ACoverLink *Link = SelectedLinks(Idx);
			for (INT SlotIdx = 0; SlotIdx < Link->Slots.Num(); SlotIdx++)
			{
				if (Link->Slots(SlotIdx).bSelected)
				{
					Link->Slots(SlotIdx).bAllowClimbUp = !Link->Slots(SlotIdx).bAllowClimbUp;
				}
			}
		}
	}
}

void WxEditorFrame::MenuRedrawAllViewports( wxCommandEvent& In )
{
	GUnrealEd->RedrawAllViewports();
}

void WxEditorFrame::MenuAlignWall( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("POLY TEXALIGN WALL") );
}

void WxEditorFrame::MenuToolCheckErrors( wxCommandEvent& In )
{
	GUnrealEd->Exec(TEXT("MAP CHECK"));
}

void WxEditorFrame::MenuReviewPaths( wxCommandEvent& In )
{
	GWarn->MapCheck_Clear();
	FPathBuilder::Exec( TEXT("REVIEWPATHS") );
	GWarn->MapCheck_ShowConditionally();
}

void WxEditorFrame::MenuRotateActors( wxCommandEvent& In )
{
}

void WxEditorFrame::MenuResetParticleEmitters( wxCommandEvent& In )
{
}

void WxEditorFrame::MenuSelectAllSurfs( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("POLY SELECT ALL") );
}

void WxEditorFrame::MenuBrushAdd( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("BRUSH ADD") );
	GUnrealEd->RedrawLevelEditingViewports();
}

void WxEditorFrame::MenuBrushSubtract( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("BRUSH SUBTRACT") );
	GUnrealEd->RedrawLevelEditingViewports();
}

void WxEditorFrame::MenuBrushIntersect( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("BRUSH FROM INTERSECTION") );
	GUnrealEd->RedrawLevelEditingViewports();
}

void WxEditorFrame::MenuBrushDeintersect( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("BRUSH FROM DEINTERSECTION") );
	GUnrealEd->RedrawLevelEditingViewports();
}

void WxEditorFrame::MenuBrushOpen( wxCommandEvent& In )
{
	WxFileDialog OpenFileDialog(this, 
		TEXT("Open Brush"), 
		*(appGameDir() + TEXT("Content\\Maps")),
		TEXT(""),
		TEXT("Brushes (*.u3d)|*.u3d|All Files|*.*"),
		wxOPEN | wxFILE_MUST_EXIST | wxHIDE_READONLY,
		wxDefaultPosition);

	if( OpenFileDialog.ShowModal() == wxID_OK )
	{
		FString S(OpenFileDialog.GetPath());
		GUnrealEd->Exec( *FString::Printf(TEXT("BRUSH LOAD FILE=\"%s\""), *S));
		GUnrealEd->RedrawLevelEditingViewports();
	}

	GFileManager->SetDefaultDirectory();
}

void WxEditorFrame::MenuBrushSaveAs( wxCommandEvent& In )
{
	WxFileDialog SaveFileDialog(this, 
		TEXT("Save Brush"), 
		*(appGameDir() + TEXT("Content\\Maps")),
		TEXT(""),
		TEXT("Brushes (*.u3d)|*.u3d|All Files|*.*"),
		wxSAVE,
		wxDefaultPosition);

	if( SaveFileDialog.ShowModal() == wxID_OK )
	{
		FString S(SaveFileDialog.GetPath());
		GUnrealEd->Exec( *FString::Printf(TEXT("BRUSH SAVE FILE=\"%s\""), *S));
	}

	GFileManager->SetDefaultDirectory();
}

void WxEditorFrame::MenuBrushImport( wxCommandEvent& In )
{
	WxFileDialog ImportFileDialog( this, 
		TEXT("Import Brush"), 
		*(GApp->LastDir[LD_BRUSH]),
		TEXT(""),
		TEXT("Import Types (*.t3d, *.dxf, *.asc, *.ase)|*.t3d;*.dxf;*.asc;*.ase;|All Files|*.*"),
		wxOPEN | wxFILE_MUST_EXIST | wxHIDE_READONLY,
		wxDefaultPosition);

	// Display the Open dialog box.
	if( ImportFileDialog.ShowModal() == wxID_OK )
	{
		WxDlgImportBrush dlg;
		dlg.ShowModal( FString(ImportFileDialog.GetPath()) );

		GUnrealEd->RedrawLevelEditingViewports();

		FString S(ImportFileDialog.GetPath());
		GApp->LastDir[LD_BRUSH] = S.Left( S.InStr( TEXT("\\"), 1 ) );
	}

	GFileManager->SetDefaultDirectory();
}

void WxEditorFrame::MenuBrushExport( wxCommandEvent& In )
{
	WxFileDialog SaveFileDialog(this, 
		TEXT("Export Brush"), 
		*(GApp->LastDir[LD_BRUSH]),
		TEXT(""),
		TEXT("Unreal Text (*.t3d)|*.t3d|All Files|*.*"),
		wxSAVE,
		wxDefaultPosition);

	if( SaveFileDialog.ShowModal() == wxID_OK )
	{
		FString S(SaveFileDialog.GetPath());

		GUnrealEd->Exec( *FString::Printf(TEXT("BRUSH EXPORT FILE=\"%s\""), *S));

		GApp->LastDir[LD_BRUSH] = S.Left( S.InStr( TEXT("\\"), 1 ) );
	}

	GFileManager->SetDefaultDirectory();
}

void WxEditorFrame::MenuWizardNewTerrain( wxCommandEvent& In )
{
	WxWizard_NewTerrain wiz( this );
	wiz.RunWizard();
}

void WxEditorFrame::MenuAboutBox( wxCommandEvent& In )
{
	WxDlgAbout dlg;
	dlg.ShowModal();
}

void WxEditorFrame::MenuOnlineHelp( wxCommandEvent& In )
{
	appLaunchURL(TEXT("https://udn.epicgames.com/Three/WebHome"));
}

/** Shows the tip of the day dialog. */
void WxEditorFrame::MenuTipOfTheDay(wxCommandEvent& In )
{
	GApp->DlgTipOfTheDay->Show();
}

/**
 * Creates an actor of the specified type, trying first to find an actor factory,
 * falling back to "ACTOR ADD" exec and SpawnActor if no factory is found.
 * Does nothing if ActorClass is NULL.
 */
static void PrivateAddActor(const UClass* ActorClass)
{
	if ( ActorClass )
	{
		// Look for an actor factory capable of creating actors of that type.
		UActorFactory* ActorFactory = GEditor->FindActorFactory( ActorClass );
		if( ActorFactory )
		{
			GEditor->UseActorFactory( ActorFactory, FALSE, FALSE );
		}
		else
		{
			// No actor factory was found; use SpawnActor instead.
			GUnrealEd->Exec( *FString::Printf( TEXT("ACTOR ADD CLASS=%s"), *ActorClass->GetName() ) );
		}
	}
}

void WxEditorFrame::MenuBackdropPopupAddClassHere( wxCommandEvent& In )
{
	const UClass* Class = GEditor->GetSelectedObjects()->GetTop<UClass>();
	PrivateAddActor( Class );
}

void WxEditorFrame::MenuBackdropPopupAddLastSelectedClassHere( wxCommandEvent& In )
{
	const INT Idx = In.GetId() - ID_BackdropPopupAddLastSelectedClassHere_START;
	const UClass* Class = GEditor->GetSelectedActors()->GetSelectedClass( Idx );
	PrivateAddActor( Class );
}

void WxEditorFrame::MenuSurfPopupApplyMaterial( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("POLY SETMATERIAL") );
}

void WxEditorFrame::MenuSurfPopupAlignPlanarAuto( wxCommandEvent& In )
{
	GTexAlignTools.GetAligner( TEXALIGN_PlanarAuto )->Align( TEXALIGN_PlanarAuto, GWorld->GetModel() );
}

void WxEditorFrame::MenuSurfPopupAlignPlanarWall( wxCommandEvent& In )
{
	GTexAlignTools.GetAligner( TEXALIGN_PlanarWall )->Align( TEXALIGN_PlanarWall, GWorld->GetModel() );
}

void WxEditorFrame::MenuSurfPopupAlignPlanarFloor( wxCommandEvent& In )
{
	GTexAlignTools.GetAligner( TEXALIGN_PlanarFloor )->Align( TEXALIGN_PlanarFloor, GWorld->GetModel() );
}

void WxEditorFrame::MenuSurfPopupAlignBox( wxCommandEvent& In )
{
	GTexAlignTools.GetAligner( TEXALIGN_Box )->Align( TEXALIGN_Box, GWorld->GetModel() );
}

void WxEditorFrame::MenuSurfPopupAlignFace( wxCommandEvent& In )
{
	GTexAlignTools.GetAligner( TEXALIGN_Face )->Align( TEXALIGN_Face, GWorld->GetModel() );
}

void WxEditorFrame::MenuSurfPopupUnalign( wxCommandEvent& In )
{
	GTexAlignTools.GetAligner( TEXALIGN_Default )->Align( TEXALIGN_Default, GWorld->GetModel() );
}

void WxEditorFrame::MenuSurfPopupSelectMatchingGroups( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("POLY SELECT MATCHING GROUPS") );
}

void WxEditorFrame::MenuSurfPopupSelectMatchingItems( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("POLY SELECT MATCHING ITEMS") );
}

void WxEditorFrame::MenuSurfPopupSelectMatchingBrush( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("POLY SELECT MATCHING BRUSH") );
}

void WxEditorFrame::MenuSurfPopupSelectMatchingTexture( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("POLY SELECT MATCHING TEXTURE") );
}

void WxEditorFrame::MenuSurfPopupSelectAllAdjacents( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("POLY SELECT ADJACENT ALL") );
}

void WxEditorFrame::MenuSurfPopupSelectAdjacentCoplanars( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("POLY SELECT ADJACENT COPLANARS") );
}

void WxEditorFrame::MenuSurfPopupSelectAdjacentWalls( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("POLY SELECT ADJACENT WALLS") );
}

void WxEditorFrame::MenuSurfPopupSelectAdjacentFloors( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("POLY SELECT ADJACENT FLOORS") );
}

void WxEditorFrame::MenuSurfPopupSelectAdjacentSlants( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("POLY SELECT ADJACENT SLANTS") );
}

void WxEditorFrame::MenuSurfPopupSelectReverse( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("POLY SELECT REVERSE") );
}

void WxEditorFrame::MenuSurfPopupSelectMemorize( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("POLY SELECT MEMORY SET") );
}

void WxEditorFrame::MenuSurfPopupRecall( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("POLY SELECT MEMORY RECALL") );
}

void WxEditorFrame::MenuSurfPopupOr( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("POLY SELECT MEMORY INTERSECTION") );
}

void WxEditorFrame::MenuSurfPopupAnd( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("POLY SELECT MEMORY UNION") );
}

void WxEditorFrame::MenuSurfPopupXor( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("POLY SELECT MEMORY XOR") );
}

void WxEditorFrame::MenuActorPopupSelectAllClass( wxCommandEvent& In )
{
	FGetInfoRet gir = GetInfo( GI_NUM_SELECTED | GI_CLASSNAME_SELECTED );

	if( gir.iValue )
	{
		GUnrealEd->Exec( *FString::Printf( TEXT("ACTOR SELECT OFCLASS CLASS=%s"), *gir.String ) );
	}
}

void WxEditorFrame::MenuActorPopupSelectMatchingStaticMeshesThisClass( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("ACTOR SELECT MATCHINGSTATICMESH") );
}

void WxEditorFrame::MenuActorPopupSelectMatchingStaticMeshesAllClasses( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("ACTOR SELECT MATCHINGSTATICMESH ALLCLASSES") );
}

void WxEditorFrame::MenuActorPopupSelectMatchingSpeedTrees( wxCommandEvent& In )
{
	GUnrealEd->SelectMatchingSpeedTrees();
}

/**
 * Toggle the dynamic channel of selected lights without invalidating any cached lighting.
 */
void WxEditorFrame::MenuActorPopupToggleDynamicChannel( wxCommandEvent& In )
{
	// Select all light actors.
	for( FSelectedActorIterator It; It; ++It )
	{
		ALight* Light = Cast<ALight>(*It);
		if( Light && Light->LightComponent )
		{
			FComponentReattachContext ReattachContext( Light->LightComponent );
			Light->LightComponent->LightingChannels.Dynamic = !Light->LightComponent->LightingChannels.Dynamic;
		}
	}
}

void WxEditorFrame::MenuActorPopupSelectAllLights( wxCommandEvent& In )
{
	// Select all light actors.
	for( FActorIterator It; It; ++It )
	{
		ALight* Light = Cast<ALight>(*It);
		if( Light && !Light->IsInPrefabInstance() )
		{
			GUnrealEd->SelectActor( Light, TRUE, NULL, TRUE, FALSE );
		}
	}
}

void WxEditorFrame::MenuActorPopupSelectAllLightsWithSameClassification( wxCommandEvent& In )
{
	ELightAffectsClassification LightAffectsClassification = LAC_MAX;
	// Find first selected light and use its classification.
	for( FSelectedActorIterator It; It; ++It )
	{
		ALight* Light = Cast<ALight>(*It);
		if( Light && Light->LightComponent )
		{
			LightAffectsClassification = (ELightAffectsClassification)Light->LightComponent->LightAffectsClassification;
			break;
		}
	}
	// Select all lights matching the light classification.
	for( FActorIterator It; It; ++It )
	{
		ALight* Light = Cast<ALight>(*It);
		if( Light && Light->LightComponent )
		{
			if( LightAffectsClassification == Light->LightComponent->LightAffectsClassification )
			{
				if ( !Light->IsInPrefabInstance() )
				{
					GUnrealEd->SelectActor( Light, TRUE, NULL, TRUE, FALSE );
				}
			}
		}
	}
}

void WxEditorFrame::MenuActorPopupSelectKismetUnreferenced( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("ACTOR SELECT KISMETREF 0") );
}

void WxEditorFrame::MenuActorPopupSelectKismetReferenced( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("ACTOR SELECT KISMETREF 1") );
}

void WxEditorFrame::MenuActorPopupAlignCameras( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT( "CAMERA ALIGN" ) );
}

void WxEditorFrame::MenuActorPopupLockMovement( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("ACTOR TOGGLE LOCKMOVEMENT") );
}

void WxEditorFrame::MenuActorPopupSnapViewToCamera( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("CAMERA SNAP") );
}

void WxEditorFrame::MenuActorPopupMerge( wxCommandEvent& In )
{
	GUnrealEd->Exec(TEXT("BRUSH MERGEPOLYS"));
}

void WxEditorFrame::MenuActorPopupSeparate( wxCommandEvent& In )
{
	GUnrealEd->Exec(TEXT("BRUSH SEPARATEPOLYS"));
}

void WxEditorFrame::MenuActorPopupToFirst( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("MAP SENDTO FIRST") );
}

void WxEditorFrame::MenuActorPopupToLast( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("MAP SENDTO LAST") );
}

void WxEditorFrame::MenuActorPopupToBrush( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("MAP BRUSH GET") );
}

void WxEditorFrame::MenuActorPopupFromBrush( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("MAP BRUSH PUT") );
}

void WxEditorFrame::MenuActorPopupMakeAdd( wxCommandEvent& In )
{
	GUnrealEd->Exec( *FString::Printf(TEXT("MAP SETBRUSH CSGOPER=%d"), (INT)CSG_Add) );
}

void WxEditorFrame::MenuActorPopupMakeSubtract( wxCommandEvent& In )
{
	GUnrealEd->Exec( *FString::Printf(TEXT("MAP SETBRUSH CSGOPER=%d"), (INT)CSG_Subtract) );
}

void WxEditorFrame::MenuSnapToFloor( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("ACTOR ALIGN SNAPTOFLOOR ALIGN=0") );
}

void WxEditorFrame::MenuAlignToFloor( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("ACTOR ALIGN SNAPTOFLOOR ALIGN=1") );
}

void WxEditorFrame::MenuMoveToGrid( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("ACTOR ALIGN MOVETOGRID") );
}

void WxEditorFrame::MenuSaveBrushAsCollision( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("STATICMESH SAVEBRUSHASCOLLISION") );
}

void WxEditorFrame::MenuActorSelectShow( wxCommandEvent& In )			
{	
	GUnrealEd->Exec(TEXT("ACTOR HIDE UNSELECTED"));	
}

void WxEditorFrame::MenuActorSelectHide( wxCommandEvent& In )			
{	
	GUnrealEd->Exec(TEXT("ACTOR HIDE SELECTED"));	
}

void WxEditorFrame::MenuActorSelectInvert( wxCommandEvent& In )		
{	
	GUnrealEd->Exec(TEXT("ACTOR SELECT INVERT"));	
}

void WxEditorFrame::MenuActorShowAll( wxCommandEvent& In )				
{	
	GUnrealEd->Exec(TEXT("ACTOR UNHIDE ALL"));	
}

/** Utility . */
struct FConvertStaticMeshActorInfo
{
	/** The level the source actor belonged to, and into which the new actor is created. */
	ULevel*						SourceLevel;

	// Actor properties
	FVector						Location;
	FRotator					Rotation;
	FVector						PrePivot;
	FLOAT						DrawScale;
	FVector						DrawScale3D;
	UBOOL						bHidden;
	UBOOL						bCollideActors;
	UBOOL						bCollideWorld;
	UBOOL						bBlockActors;
	UBOOL						bPathColliding;
	UBOOL						bNoEncroachCheck;
	AActor*						Base;
	FName						BaseBoneName;
	USkeletalMeshComponent*		BaseSkelComponent;
	UEdLayer*					Layer;

	// Component properties
	UStaticMesh*						StaticMesh;
	TArrayNoInit<UMaterialInterface*>	Materials;
	TArray<FGuid>						IrrelevantLights;
	FLOAT								CachedCullDistance;
	UBOOL								CastShadow;
	UBOOL								CollideActors;
	UBOOL								BlockActors;
	UBOOL								BlockZeroExtent;
	UBOOL								BlockNonZeroExtent;
	UBOOL								BlockRigidBody;
	UPhysicalMaterial*					PhysMaterialOverride;

	TArray<USeqVar_Object*>		KismetObjectVars;
	TArray<USequenceEvent*>		KismetEvents;

	void GetFromActor(AActor* Actor, UStaticMeshComponent* MeshComp)
	{
		SourceLevel				= Actor->GetLevel();

		Location				= Actor->Location;
		Rotation				= Actor->Rotation;
		PrePivot				= Actor->PrePivot;
		DrawScale				= Actor->DrawScale;
		DrawScale3D				= Actor->DrawScale3D;
		bHidden					= Actor->bHidden;
		bCollideActors			= Actor->bCollideActors;
		bCollideWorld			= Actor->bCollideWorld;
		bBlockActors			= Actor->bBlockActors;
		bPathColliding			= Actor->bPathColliding;
		bNoEncroachCheck		= Actor->bNoEncroachCheck;
		Base					= Actor->Base;
		BaseBoneName			= Actor->BaseBoneName;
		BaseSkelComponent		= Actor->BaseSkelComponent;
		Layer					= Actor->Layer;

		StaticMesh				= MeshComp->StaticMesh;
		Materials				= MeshComp->Materials;
		IrrelevantLights		= MeshComp->IrrelevantLights;
		CachedCullDistance		= MeshComp->CachedCullDistance;
		CastShadow				= MeshComp->CastShadow;
		CollideActors			= MeshComp->CollideActors;
		BlockActors				= MeshComp->BlockActors;
		BlockZeroExtent			= MeshComp->BlockZeroExtent;
		BlockNonZeroExtent		= MeshComp->BlockNonZeroExtent;
		BlockRigidBody			= MeshComp->BlockRigidBody;
		PhysMaterialOverride	= MeshComp->PhysMaterialOverride;
	}

	void SetToActor(AActor* Actor, UStaticMeshComponent* MeshComp)
	{
		if ( Actor->GetLevel() != SourceLevel )
		{
			appErrorf( *LocalizeUnrealEd("Error_ActorConversionLevelMismatch") );
		}

		Actor->Location				= Location;
		Actor->Rotation				= Rotation;
		Actor->PrePivot				= PrePivot;
		Actor->DrawScale			= DrawScale;
		Actor->DrawScale3D			= DrawScale3D;
		Actor->bHidden				= bHidden;
		Actor->bCollideActors		= bCollideActors;
		Actor->bCollideWorld		= bCollideWorld;
		Actor->bBlockActors			= bBlockActors;
		Actor->bPathColliding		= bPathColliding;
		Actor->bNoEncroachCheck		= bNoEncroachCheck;
		Actor->Base					= Base;
		Actor->BaseBoneName			= BaseBoneName;
		Actor->BaseSkelComponent	= BaseSkelComponent;
		Actor->Layer				= Layer;

		MeshComp->StaticMesh			= StaticMesh;
		MeshComp->Materials				= Materials;
		MeshComp->IrrelevantLights		= IrrelevantLights;
		MeshComp->CachedCullDistance	= CachedCullDistance;
		MeshComp->CastShadow			= CastShadow;
		MeshComp->CollideActors			= CollideActors;
		MeshComp->BlockActors			= BlockActors;
		MeshComp->BlockZeroExtent		= BlockZeroExtent;
		MeshComp->BlockNonZeroExtent	= BlockNonZeroExtent;
		MeshComp->BlockRigidBody		= BlockRigidBody;
		MeshComp->PhysMaterialOverride	= PhysMaterialOverride;
	}
};



/** Utility for converting between StaticMeshes, KActors and InterpActors (Movers). */
void WxEditorFrame::MenuConvertActors( wxCommandEvent& In )
{
	const FScopedTransaction Transaction( *LocalizeUnrealEd(TEXT("ConvertMeshes")) );

	const UBOOL bFromStaticMesh = (In.GetId() == IDMENU_ActorPopupConvertStaticMeshToKActor || In.GetId() == IDMENU_ActorPopupConvertStaticMeshToMover);
	const UBOOL bFromKActor = (In.GetId() == IDMENU_ActorPopupConvertKActorToStaticMesh || In.GetId() == IDMENU_ActorPopupConvertKActorToMover);
	const UBOOL bFromMover = (In.GetId() == IDMENU_ActorPopupConvertMoverToStaticMesh || In.GetId() == IDMENU_ActorPopupConvertMoverToKActor);

	const UBOOL bToStaticMesh = (In.GetId() == IDMENU_ActorPopupConvertKActorToStaticMesh || In.GetId() == IDMENU_ActorPopupConvertMoverToStaticMesh);
	const UBOOL bToKActor = (In.GetId() == IDMENU_ActorPopupConvertStaticMeshToKActor || In.GetId() == IDMENU_ActorPopupConvertMoverToKActor);
	const UBOOL bToMover = (In.GetId() == IDMENU_ActorPopupConvertStaticMeshToMover || In.GetId() == IDMENU_ActorPopupConvertKActorToMover);


	TArray<AActor*>				SourceActors;
	TArray<FConvertStaticMeshActorInfo>	ConvertInfo;

	// Provide the option to abort up-front.
	UBOOL bIgnoreKismetReferenced = FALSE;
	if ( GUnrealEd->ShouldAbortActorDeletion( bIgnoreKismetReferenced ) )
	{
		return;
	}

	// Iterate over selected Actors.
	for ( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
	{
		AActor* Actor				= static_cast<AActor*>( *It );
		checkSlow( Actor->IsA(AActor::StaticClass()) );

		// Reject Kismet-referenced actors, if the user specified.
		if ( bIgnoreKismetReferenced && Actor->IsReferencedByKismet() )
		{
			continue;
		}

		AStaticMeshActor* SMActor	= bFromStaticMesh ? Cast<AStaticMeshActor>(Actor) : NULL;
		AKActor* KActor				= bFromKActor ? Cast<AKActor>(Actor) : NULL;
		AInterpActor* InterpActor	= bFromMover ? Cast<AInterpActor>(Actor) : NULL;

		const UBOOL bFoundActorToConvert = SMActor || KActor || InterpActor;
		if ( bFoundActorToConvert )
		{
			// If its the type we are converting 'from' copy its properties and remember it.
			FConvertStaticMeshActorInfo Info;
			appMemzero(&Info, sizeof(FConvertStaticMeshActorInfo));

			UBOOL bFoundActorToConvert = FALSE;
			if( SMActor )
			{
				SourceActors.AddItem(Actor);
				Info.GetFromActor(SMActor, SMActor->StaticMeshComponent);
			}
			else if( KActor )
			{
				SourceActors.AddItem(Actor);
				Info.GetFromActor(KActor, KActor->StaticMeshComponent);
			}
			else if( InterpActor )
			{
				SourceActors.AddItem(Actor);
				Info.GetFromActor(InterpActor, InterpActor->StaticMeshComponent);
			}

			// Get the sequence corresponding to the actor's level.
			ULevel* SourceLevel = Actor->GetLevel();
			USequence* RootSeq = GWorld->GetGameSequence( SourceLevel );
			if ( RootSeq )
			{
				// Look through Kismet to find any references to this Actor and remember them.
				TArray<USequenceObject*> SeqObjects;
				RootSeq->FindSeqObjectsByObjectName(Actor->GetFName(), SeqObjects);
				for(INT i=0; i<SeqObjects.Num(); i++)
				{
					USequenceEvent* Event = Cast<USequenceEvent>( SeqObjects(i) );
					if(Event)
					{
						check(Event->Originator == Actor);
						Info.KismetEvents.AddUniqueItem(Event);
					}

					USeqVar_Object* ObjVar = Cast<USeqVar_Object>( SeqObjects(i) );
					if(ObjVar)
					{
						check(ObjVar->ObjValue == Actor);
						Info.KismetObjectVars.AddUniqueItem(ObjVar);
					}
				}
			}

			const INT NewIndex = ConvertInfo.AddZeroed();
			ConvertInfo(NewIndex) = Info;
		}
	}

	// Then clear selection, select and delete the source actors.
	GEditor->SelectNone( FALSE, FALSE );
	for( INT ActorIndex = 0 ; ActorIndex < SourceActors.Num() ; ++ActorIndex )
	{
		AActor* SourceActor = SourceActors(ActorIndex);
		GEditor->SelectActor( SourceActor, TRUE, NULL, FALSE );
	}

	if ( GUnrealEd->edactDeleteSelected( FALSE, bIgnoreKismetReferenced ) )
	{
		// Now we need to spawn some new actors at the desired locations.
		ULevel* OldCurrentLevel = GWorld->CurrentLevel;
		for( INT i = 0 ; i < ConvertInfo.Num() ; ++i )
		{
			FConvertStaticMeshActorInfo& Info = ConvertInfo(i);

			// Spawn correct type, and copy properties from intermediate struct.
			AActor* Actor = NULL;
			if( bToStaticMesh )
			{
				// Make current the level into which the new actor is spawned.
				GWorld->CurrentLevel = Info.SourceLevel;
				AStaticMeshActor* SMActor = CastChecked<AStaticMeshActor>( GWorld->SpawnActor(AStaticMeshActor::StaticClass(), NAME_None, Info.Location, Info.Rotation) );
				SMActor->ClearComponents();
				Info.SetToActor(SMActor, SMActor->StaticMeshComponent);
				SMActor->ConditionalUpdateComponents();
				GEditor->SelectActor( SMActor, TRUE, NULL, FALSE );
				Actor = SMActor;
			}
			else if( bToKActor )
			{
				// Make current the level into which the new actor is spawned.
				GWorld->CurrentLevel = Info.SourceLevel;
				AKActor* KActor = CastChecked<AKActor>( GWorld->SpawnActor(AKActor::StaticClass(), NAME_None, Info.Location, Info.Rotation) );
				KActor->ClearComponents();
				Info.SetToActor(KActor, KActor->StaticMeshComponent);
				KActor->ConditionalUpdateComponents();
				GEditor->SelectActor( KActor, TRUE, NULL, FALSE );
				Actor = KActor;
			}
			else if( bToMover )
			{
				// Make current the level into which the new actor is spawned.
				GWorld->CurrentLevel = Info.SourceLevel;
				AInterpActor* InterpActor = CastChecked<AInterpActor>( GWorld->SpawnActor(AInterpActor::StaticClass(), NAME_None, Info.Location, Info.Rotation) );
				InterpActor->ClearComponents();
				Info.SetToActor(InterpActor, InterpActor->StaticMeshComponent);
				InterpActor->ConditionalUpdateComponents();
				GEditor->SelectActor( InterpActor, TRUE, NULL, FALSE );
				Actor = InterpActor;
			}

			// Fix up Kismet events and obj vars to new Actor.
			if( Actor )
			{
				for(INT j=0; j<Info.KismetEvents.Num(); j++)
				{
					Info.KismetEvents(j)->Originator = Actor;
				}

				for(INT j=0; j<Info.KismetObjectVars.Num(); j++)
				{
					Info.KismetObjectVars(j)->ObjValue = Actor;
				}
			}
		}

		// Restore the current level.
		GWorld->CurrentLevel = OldCurrentLevel;
	}

	GEditor->NoteSelectionChange();
}


void WxEditorFrame::MenuSetLightDataBasedOnClassification( wxCommandEvent& In )
{
	const FScopedTransaction Transaction( *LocalizeUnrealEd(TEXT("ChangeLightClassification")) );

	const UBOOL bToLightDynamicAffecting = ( In.GetId() == IDMENU_IDMENU_ActorPopupConvertLightToLightDynamicAffecting );
	const UBOOL bToLightStaticAffecting = ( In.GetId() == IDMENU_IDMENU_ActorPopupConvertLightToLightStaticAffecting );
	const UBOOL bToLightDynamicsAndStaticAffecting = ( In.GetId() == IDMENU_IDMENU_ActorPopupConvertLightToLightDynamicAndStaticAffecting );

	// Iterate over selected actors.
	for( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
	{
		AActor* Actor = static_cast<AActor*>( *It );
		checkSlow( Actor->IsA(AActor::StaticClass()) );

		ALight* LightActor	= Cast<ALight>(Actor);

		if( ( LightActor != NULL ) && ( LightActor->LightComponent != NULL ) )
		{
			if( bToLightDynamicAffecting == TRUE )
			{
				LightActor->SetValuesForLight_DynamicAffecting();
			}
			else if( bToLightStaticAffecting == TRUE )
			{
				LightActor->SetValuesForLight_StaticAffecting();
			}
			else if ( bToLightDynamicsAndStaticAffecting == TRUE )
			{
				LightActor->SetValuesForLight_DynamicAndStaticAffecting();
			}

			// now set the icon as we have all the data we need
			LightActor->DetermineAndSetEditorIcon();

			// We need to invalidate the cached lighting as we might have toggled lightmap vs no lightmap.
			LightActor->InvalidateLightingCache();
		}
	}

	GEditor->NoteSelectionChange();
}

void WxEditorFrame::MenuQuantizeVertices( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("ACTOR ALIGN") );
}

void WxEditorFrame::MenuConvertToStaticMesh( wxCommandEvent& In )
{
	WxDlgPackageGroupName dlg;

	WxGenericBrowser* gb = GUnrealEd->GetBrowser<WxGenericBrowser>( TEXT("GenericBrowser") );

	UPackage* pkg = gb->GetTopPackage();
	UPackage* grp = gb->GetGroup();

	const FString PackageName = pkg ? pkg->GetName() : TEXT("MyPackage");
	const FString GroupName = grp ? grp->GetName() : TEXT("");

	if( dlg.ShowModal( PackageName, GroupName, TEXT("MyMesh") ) == wxID_OK )
	{
		FString Wk = FString::Printf( TEXT("STATICMESH FROM SELECTION PACKAGE=%s"), *dlg.GetPackage() );
		if( dlg.GetGroup().Len() > 0 )
		{
			Wk += FString::Printf( TEXT(" GROUP=%s"), *dlg.GetGroup() );
		}
		Wk += FString::Printf( TEXT(" NAME=%s"), *dlg.GetObjectName() );

		GUnrealEd->Exec( *Wk );
	}
}

void WxEditorFrame::MenuActorPivotReset( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("ACTOR RESET PIVOT") );
}

void WxEditorFrame::MenuActorMirrorX( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("ACTOR MIRROR X=-1") );
}

void WxEditorFrame::MenuActorMirrorY( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("ACTOR MIRROR Y=-1") );
}

void WxEditorFrame::MenuActorMirrorZ( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("ACTOR MIRROR Z=-1") );
}

void WxEditorFrame::MenuActorPopupMakeSolid( wxCommandEvent& In )
{
	GUnrealEd->Exec( *FString::Printf( TEXT("MAP SETBRUSH CLEARFLAGS=%d SETFLAGS=%d"), PF_Semisolid + PF_NotSolid, 0 ) );
}

void WxEditorFrame::MenuActorPopupMakeSemiSolid( wxCommandEvent& In )
{
	GUnrealEd->Exec( *FString::Printf( TEXT("MAP SETBRUSH CLEARFLAGS=%d SETFLAGS=%d"), (INT)(PF_Semisolid + PF_NotSolid), (INT)PF_Semisolid ) );
}

void WxEditorFrame::MenuActorPopupMakeNonSolid( wxCommandEvent& In )
{
	GUnrealEd->Exec( *FString::Printf( TEXT("MAP SETBRUSH CLEARFLAGS=%d SETFLAGS=%d"), (INT)(PF_Semisolid + PF_NotSolid), (INT)PF_NotSolid ) );
}

void WxEditorFrame::MenuActorPopupBrushSelectAdd( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("MAP SELECT ADDS") );
}

void WxEditorFrame::MenuActorPopupBrushSelectSubtract( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("MAP SELECT SUBTRACTS") );
}

void WxEditorFrame::MenuActorPopupBrushSelectNonSolid( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("MAP SELECT NONSOLIDS") );
}

void WxEditorFrame::MenuActorPopupBrushSelectSemiSolid( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("MAP SELECT SEMISOLIDS") );
}

/**
 * Forces all selected navigation points to position themselves as though building
 * paths (ie FindBase).
 */
void WxEditorFrame::MenuActorPopupPathPosition( wxCommandEvent& In )
{
	TArray<ANavigationPoint*> NavPts;
	GEditor->GetSelectedActors()->GetSelectedObjects<ANavigationPoint>(NavPts);
	for (INT Idx = 0; Idx < NavPts.Num(); Idx++)
	{
		NavPts(Idx)->FindBase();
	}
}

/**
 * Proscribes a path between all selected nodes.
 */
void WxEditorFrame::MenuActorPopupPathProscribe( wxCommandEvent& In )
{
	TArray<ANavigationPoint*> NavPts;
	GEditor->GetSelectedActors()->GetSelectedObjects<ANavigationPoint>(NavPts);
	for (INT Idx = 0; Idx < NavPts.Num(); Idx++)
	{
		for (INT ProscribeIdx = 0; ProscribeIdx < NavPts.Num(); ProscribeIdx++)
		{
			if (ProscribeIdx != Idx)
			{
				UBOOL bHasPath = FALSE;
				for (INT PathIdx = 0; PathIdx < NavPts(Idx)->EditorProscribedPaths.Num(); PathIdx++)
				{
					if (NavPts(Idx)->EditorProscribedPaths(PathIdx).Nav == NavPts(ProscribeIdx))
					{
						bHasPath = TRUE;
						break;
					}
				}
				if (!bHasPath)
				{
					// add to the list
					FNavReference NavRef(NavPts(ProscribeIdx),NavPts(ProscribeIdx)->NavGuid);
					NavPts(Idx)->EditorProscribedPaths.AddItem(NavRef);
					// check to see if we're breaking an existing path
					UReachSpec *Spec = NavPts(Idx)->GetReachSpecTo(NavPts(ProscribeIdx));
					if (Spec != NULL)
					{
						// remove the old Spec
						NavPts(Idx)->PathList.RemoveItem(Spec);
					}
					// create a new Proscribed Spec
					NavPts(Idx)->ProscribePathTo(NavPts(ProscribeIdx));
				}
			}
		}
	}
}

/**
 * Forces a path between all selected nodes.
 */
void WxEditorFrame::MenuActorPopupPathForce( wxCommandEvent& In )
{
	TArray<ANavigationPoint*> NavPts;
	GEditor->GetSelectedActors()->GetSelectedObjects<ANavigationPoint>(NavPts);
	for (INT Idx = 0; Idx < NavPts.Num(); Idx++)
	{
		for (INT ForceIdx = 0; ForceIdx < NavPts.Num(); ForceIdx++)
		{
			if (ForceIdx != Idx)
			{
				// if not already ForceIdx
				UBOOL bHasPath = FALSE;
				for (INT PathIdx = 0; PathIdx < NavPts(Idx)->EditorForcedPaths.Num(); PathIdx++)
				{
					if (NavPts(Idx)->EditorForcedPaths(PathIdx).Nav == NavPts(ForceIdx))
					{
						bHasPath = TRUE;
						break;
					}
				}
				if (!bHasPath)
				{
					// add to the list
					FNavReference NavRef(NavPts(ForceIdx),NavPts(ForceIdx)->NavGuid);
					NavPts(Idx)->EditorForcedPaths.AddItem(NavRef);
					// remove any normal spec
					UReachSpec *Spec = NavPts(Idx)->GetReachSpecTo(NavPts(ForceIdx));
					if (Spec != NULL)
					{
						NavPts(Idx)->PathList.RemoveItem(Spec);
					}
					// and create a new ForceIdxd Spec
					NavPts(Idx)->ForcePathTo(NavPts(ForceIdx));
				}
			}
		}
	}
}

/**
 * Assigns selected navigation points to selected route actors.
 */
void WxEditorFrame::MenuActorPopupPathAssignNavPointsToRoute( wxCommandEvent& In )
{
	TArray<ARoute*> Routes;
	TArray<ANavigationPoint*> Points;

	for( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
	{
		ARoute* Route = Cast<ARoute>(*It);
		if( Route )
		{
			Routes.AddItem( Route );
			continue;
		}
		ANavigationPoint* Point = Cast<ANavigationPoint>(*It);
		if( Point )
		{
			Points.AddItem( Point );
			continue;
		}
	}

	for( INT Idx = 0; Idx < Routes.Num(); Idx++ )
	{
		ARoute* Route = Routes(Idx);

		// Get fill action
		ERouteFillAction Action = RFA_Overwrite;
		if( In.GetId() == IDMENU_ActorPopupPathAddRoute )
		{
			Action = RFA_Add;
		}
		else
		if( In.GetId() == IDMENU_ActorPopupPathRemoveRoute )
		{
			Action = RFA_Remove;
		}
		else
		if( In.GetId() == IDMENU_ActorPopupPathClearRoute )
		{
			Action = RFA_Clear;
		}

		// Tell route to fill w/ points
		Route->AutoFillRoute( Action, Points );
	}
}

/**
 * Assigns selected navigation points to selected route actors.
 */
void WxEditorFrame::MenuActorPopupPathAssignLinksToCoverGroup( wxCommandEvent& In )
{
	TArray<ACoverGroup*>	Groups;
	TArray<ACoverLink*>		Links;

	for( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
	{
		ACoverGroup* Group = Cast<ACoverGroup>(*It);
		if( Group )
		{
			Groups.AddItem( Group );
			continue;
		}
		ACoverLink* Link = Cast<ACoverLink>(*It);
		if( Link )
		{
			Links.AddItem( Link );
			continue;
		}
	}

	for( INT Idx = 0; Idx < Groups.Num(); Idx++ )
	{
		ACoverGroup* Group = Groups(Idx);

		// Get fill action
		ECoverGroupFillAction Action = CGFA_Overwrite;
		if( In.GetId() == IDMENU_ActorPopupPathAddCoverGroup )
		{
			Action = CGFA_Add;
		}
		else
		if( In.GetId() == IDMENU_ActorPopupPathRemoveCoverGroup )
		{
			Action = CGFA_Remove;
		}
		else
		if( In.GetId() == IDMENU_ActorPopupPathClearCoverGroup )
		{
			Action = CGFA_Clear;
		}

		// Tell route to fill w/ Links
		Group->AutoFillGroup( Action, Links );
	}
}

/**
 * Clears all Proscribed paths between selected nodes, or if just one node
 * is selected, clears all of it's Proscribed paths.
 */
void WxEditorFrame::MenuActorPopupPathClearProscribed( wxCommandEvent& In )
{
	TArray<ANavigationPoint*> NavPts;
	GEditor->GetSelectedActors()->GetSelectedObjects<ANavigationPoint>(NavPts);
	if (NavPts.Num() == 1)
	{
		NavPts(0)->EditorProscribedPaths.Empty();
		// remove any Proscribed Specs
		for (INT Idx = 0; Idx < NavPts(0)->PathList.Num(); Idx++)
		{
			if (NavPts(0)->PathList(Idx) != NULL &&
				NavPts(0)->PathList(Idx)->IsProscribed())
			{
				NavPts(0)->PathList.Remove(Idx--,1);
			}
		}
	}
	else
	{
		// clear any Proscribed points between the selected nodes
		for (INT Idx = 0; Idx < NavPts.Num(); Idx++)
		{
			ANavigationPoint *Nav = NavPts(Idx);
			for (INT ProscribeIdx = 0; ProscribeIdx < NavPts.Num(); ProscribeIdx++)
			{
				if (ProscribeIdx != Idx)
				{
					for (INT PathIdx = 0; PathIdx < Nav->EditorProscribedPaths.Num(); PathIdx++)
					{
						if (Nav->EditorProscribedPaths(PathIdx).Nav == NavPts(ProscribeIdx))
						{
							Nav->EditorProscribedPaths.Remove(PathIdx--,1);
						}
					}
					// remove any Proscribed Specs to the nav
					for (INT SpecIdx = 0; SpecIdx < NavPts(Idx)->PathList.Num(); SpecIdx++)
					{
						if (Nav->PathList(SpecIdx) != NULL &&
							Nav->PathList(SpecIdx)->End == NavPts(ProscribeIdx) &&
							Nav->PathList(SpecIdx)->IsProscribed())
						{
							Nav->PathList.Remove(SpecIdx--,1);
						}
					}
				}
			}
		}
	}
}

/**
 * Clears all ForceIdxd paths between selected nodes, or if just one node
 * is slected, clears all of it's ForceIdxd paths.
 */
void WxEditorFrame::MenuActorPopupPathClearForced( wxCommandEvent& In )
{
	TArray<ANavigationPoint*> NavPts;
	GEditor->GetSelectedActors()->GetSelectedObjects<ANavigationPoint>(NavPts);
	if (NavPts.Num() == 1)
	{
		NavPts(0)->EditorForcedPaths.Empty();
		// remove any ForceIdxd Specs
		for (INT Idx = 0; Idx < NavPts(0)->PathList.Num(); Idx++)
		{
			if (NavPts(0)->PathList(Idx) != NULL &&
				NavPts(0)->PathList(Idx)->IsForced())
			{
				NavPts(0)->PathList.Remove(Idx--,1);
			}
		}
	}
	else
	{
		// clear any forced points between the selected nodes
		for (INT Idx = 0; Idx < NavPts.Num(); Idx++)
		{
			ANavigationPoint *Nav = NavPts(Idx);
			for (INT ForceIdx = 0; ForceIdx < NavPts.Num(); ForceIdx++)
			{
				if (ForceIdx != Idx)
				{
					for (INT PathIdx = 0; PathIdx < Nav->EditorForcedPaths.Num(); PathIdx++)
					{
						if (Nav->EditorForcedPaths(PathIdx).Nav == NavPts(ForceIdx))
						{
							Nav->EditorForcedPaths.Remove(PathIdx--,1);
						}
					}
					// remove any forced specs to the nav
					for (INT SpecIdx = 0; SpecIdx < NavPts(Idx)->PathList.Num(); SpecIdx++)
					{
						if (Nav->PathList(SpecIdx) != NULL &&
							Nav->PathList(SpecIdx)->End == NavPts(ForceIdx) &&
							Nav->PathList(SpecIdx)->IsForced())
						{
							Nav->PathList.Remove(SpecIdx--,1);
						}
					}
				}
			}
		}
	}
}

/**
 * Stitches selected coverlinks together, placing all the slots into a single 
 * CoverLink actor.
 */
void WxEditorFrame::MenuActorPopupPathStitchCover( wxCommandEvent& In )
{
	TArray<ACoverLink*> Links;
	GEditor->GetSelectedActors()->GetSelectedObjects<ACoverLink>(Links);
	ACoverLink *DestLink = NULL;
	for (INT Idx = 0; Idx < Links.Num(); Idx++)
	{
		ACoverLink *Link = Links(Idx);
		if (Link == NULL)
		{
			continue;
		}
		// pick the first link as the dest link
		if (DestLink == NULL)
		{
			DestLink = Link;
		}
		else
		{
			// add all of the slots to the destlink
			for (INT SlotIdx = 0; SlotIdx < Link->Slots.Num(); SlotIdx++)
			{
				FVector SlotLocation = Link->GetSlotLocation(SlotIdx);
				FRotator SlotRotation = Link->GetSlotRotation(SlotIdx);
				DestLink->AddCoverSlot(SlotLocation,SlotRotation,Link->Slots(SlotIdx));
			}
			GWorld->DestroyActor(Link);
		}
	}
	// update the dest link
	if (DestLink != NULL)
	{
		for (INT SlotIdx = 0; SlotIdx < DestLink->Slots.Num(); SlotIdx++)
		{
			// update the slot info
			DestLink->AutoAdjustSlot(SlotIdx,FALSE);
			DestLink->AutoAdjustSlot(SlotIdx,TRUE);
			DestLink->BuildSlotInfo(SlotIdx);
		}
		DestLink->ForceUpdateComponents(FALSE,FALSE);
	}
	GUnrealEd->RedrawAllViewports();
}

/**
* For use with the templated sort. Sorts by class name, ascending
*/
namespace WxEditorFrameCompareFunctions
{
	struct FClassNameCompare
	{
		static INT Compare(UClass* A, UClass* B)
		{
			return appStricmp(*B->GetName(),*A->GetName());
		}
	};
}

/**
* Puts all of the AVolume classes into the passed in array and sorts them by class name.
*
* @param	VolumeClasses		Array to populate with AVolume classes.
*/
void WxEditorFrame::GetSortedVolumeClasses( TArray< UClass* >* VolumeClasses )
{
	// Add all of the volume classes to the passed in array and then sort it
	for( TObjectIterator<UClass> It ; It ; ++It )
	{
		if( It->IsChildOf(AVolume::StaticClass()) )
		{
			if ( !(It->ClassFlags & CLASS_Deprecated)
				&& !(It->ClassFlags & CLASS_Abstract)
				&& (It->ClassFlags & CLASS_Placeable) )
			{
				VolumeClasses->AddUniqueItem( *It );
			}
		}
	}

	Sort<UClass*, WxEditorFrameCompareFunctions::FClassNameCompare>( &(*VolumeClasses)(0), VolumeClasses->Num() );
}

void WxEditorFrame::OnAddVolumeClass( wxCommandEvent& In )
{
	const INT sel = In.GetId() - IDM_VolumeClasses_START;

	UClass* Class = NULL;

	TArray< UClass* > VolumeClasses;

	GApp->EditorFrame->GetSortedVolumeClasses( &VolumeClasses );

	check ( sel < VolumeClasses.Num() );

	Class = VolumeClasses( sel );

	if( Class )
	{
		GUnrealEd->Exec( *FString::Printf( TEXT("BRUSH ADDVOLUME CLASS=%s"), *Class->GetName() ) );
	}
}

/** Callback for creating or editing a material instance. */
void WxEditorFrame::MenuCreateMaterialInstanceConstant( wxCommandEvent &In )
{
	INT MaterialIdx = -1;
	INT EventID = In.GetId();
	UBOOL bCreate = FALSE;
	
	if(EventID >= IDM_CREATE_MATERIAL_INSTANCE_CONSTANT_START && EventID <= IDM_CREATE_MATERIAL_INSTANCE_CONSTANT_END)
	{
		MaterialIdx = In.GetId() - IDM_CREATE_MATERIAL_INSTANCE_CONSTANT_START;
		bCreate = TRUE;
	}
	else
	{
		MaterialIdx = In.GetId() - IDM_EDIT_MATERIAL_INSTANCE_CONSTANT_START;
	}

	// Find the material and mesh the user chose.
	USelection* SelectedActors = GEditor->GetSelectedActors();

	if(SelectedActors->Num()==1)
	{
		AActor* FirstActor = SelectedActors->GetTop<AActor>();

		if(FirstActor)
		{
			UMeshComponent* SelectedMesh = NULL;
			UMaterialInterface* SelectedMaterial = NULL;

			// Find which mesh the user clicked on first.
			for(INT ComponentIdx=0;ComponentIdx<FirstActor->Components.Num() && MaterialIdx >= 0; ComponentIdx++)
			{
				UMeshComponent* MeshComponent = Cast<UMeshComponent>(FirstActor->Components(ComponentIdx));

				if(MeshComponent != NULL)
				{
					const INT NumElements = MeshComponent->GetNumElements();
					if(MaterialIdx >= NumElements)
					{
						MaterialIdx -= NumElements;
					}
					else
					{
						SelectedMesh = MeshComponent;
						SelectedMaterial = MeshComponent->GetMaterial(MaterialIdx);
						break;
					}
				}
			}

			// If a material was found, prompt the user for a new material instance name and then create the material instance
			if(SelectedMaterial)
			{
				UMaterialInstanceConstant* InstanceToEdit = NULL;
				UPackage* LevelPackage = SelectedMesh->GetOutermost();

				if(bCreate == FALSE)
				{
					if(SelectedMaterial->GetOuter() == LevelPackage)
					{
						InstanceToEdit = Cast<UMaterialInstanceConstant>(SelectedMaterial);
					}
					else
					{
						if( appMsgf( AMT_YesNo, *LocalizeUnrealEd("NoMaterialFoundCreate") ) )
						{
							bCreate = TRUE;
						}
					}
				}

				// This mesh doesn't have an overridden material yet, so create one for this slot.
				if(bCreate && InstanceToEdit == NULL)
				{
					// Ask the user for a name and then check to see if its taken already.
					FString DefaultName = SelectedMaterial->GetName() + TEXT("_INST");
					wxTextEntryDialog TextDlg(this, *LocalizeUnrealEd("EnterMaterialInstanceName"), *LocalizeUnrealEd("PleaseEnterValue"), *DefaultName);
					if(TextDlg.ShowModal()==wxID_OK)
					{
						wxString ObjectName = TextDlg.GetValue();

						UObject* ExistingObject = FindObject<UObject>(LevelPackage, ObjectName.c_str());

						if(ExistingObject==NULL)
						{	
							InstanceToEdit = ConstructObject<UMaterialInstanceConstant>(UMaterialInstanceConstant::StaticClass(), LevelPackage, ObjectName.c_str(), RF_Transactional);

							if(InstanceToEdit)
							{
								InstanceToEdit->SetParent(SelectedMaterial);
								SelectedMesh->SetMaterial(MaterialIdx, InstanceToEdit);
								FirstActor->ForceUpdateComponents(FALSE,FALSE);
							}
						}
						else
						{
							appMsgf( AMT_OK, *FString::Printf(*LocalizeUnrealEd("MaterialInstanceNameTaken_F"), ObjectName.c_str()) );
						}
					}
				}

				// Show the material instance editor if we have an instance
				if(InstanceToEdit != NULL)
				{
					wxFrame* MaterialInstanceEditor = new WxMaterialInstanceConstantEditor( (wxWindow*)GApp->EditorFrame,-1, InstanceToEdit );
					MaterialInstanceEditor->SetSize(1024,768);
					MaterialInstanceEditor->Show();
				}

			}
		}
	}
}

/** Callback for creating or editing a material instance. */
void WxEditorFrame::MenuCreateMaterialInstanceTimeVarying( wxCommandEvent &In )
{
	INT MaterialIdx = -1;
	INT EventID = In.GetId();
	UBOOL bCreate = FALSE;

	if(EventID >= IDM_CREATE_MATERIAL_INSTANCE_TIME_VARYING_START && EventID <= IDM_CREATE_MATERIAL_INSTANCE_TIME_VARYING_END)
	{
		MaterialIdx = In.GetId() - IDM_CREATE_MATERIAL_INSTANCE_TIME_VARYING_START;
		bCreate = TRUE;
	}
	else
	{
		MaterialIdx = In.GetId() - IDM_EDIT_MATERIAL_INSTANCE_TIME_VARYING_START;
	}

	// Find the material and mesh the user chose.
	USelection* SelectedActors = GEditor->GetSelectedActors();

	if(SelectedActors->Num()==1)
	{
		AActor* FirstActor = SelectedActors->GetTop<AActor>();

		if(FirstActor)
		{
			UMeshComponent* SelectedMesh = NULL;
			UMaterialInterface* SelectedMaterial = NULL;

			// Find which mesh the user clicked on first.
			for(INT ComponentIdx=0;ComponentIdx<FirstActor->Components.Num() && MaterialIdx >= 0; ComponentIdx++)
			{
				UMeshComponent* MeshComponent = Cast<UMeshComponent>(FirstActor->Components(ComponentIdx));

				if(MeshComponent != NULL)
				{
					const INT NumElements = MeshComponent->GetNumElements();
					if(MaterialIdx >= NumElements)
					{
						MaterialIdx -= NumElements;
					}
					else
					{
						SelectedMesh = MeshComponent;
						SelectedMaterial = MeshComponent->GetMaterial(MaterialIdx);
						break;
					}
				}
			}

			// If a material was found, prompt the user for a new material instance name and then create the material instance
			if(SelectedMaterial)
			{
				UMaterialInstanceTimeVarying* InstanceToEdit = NULL;
				UPackage* LevelPackage = SelectedMesh->GetOutermost();

				if(bCreate == FALSE)
				{
					if(SelectedMaterial->GetOuter() == LevelPackage)
					{
						InstanceToEdit = Cast<UMaterialInstanceTimeVarying>(SelectedMaterial);
					}
					else
					{
						if( appMsgf( AMT_YesNo, *LocalizeUnrealEd("NoMaterialFoundCreate") ) )
						{
							bCreate = TRUE;
						}
					}
				}

				// This mesh doesn't have an overridden material yet, so create one for this slot.
				if(bCreate && InstanceToEdit == NULL)
				{
					// Ask the user for a name and then check to see if its taken already.
					FString DefaultName = SelectedMaterial->GetName() + TEXT("_INST");
					wxTextEntryDialog TextDlg(this, *LocalizeUnrealEd("EnterMaterialInstanceName"), *LocalizeUnrealEd("PleaseEnterValue"), *DefaultName);
					if(TextDlg.ShowModal()==wxID_OK)
					{
						wxString ObjectName = TextDlg.GetValue();

						UObject* ExistingObject = FindObject<UObject>(LevelPackage, ObjectName.c_str());

						if(ExistingObject==NULL)
						{	
							InstanceToEdit = ConstructObject<UMaterialInstanceTimeVarying>(UMaterialInstanceTimeVarying::StaticClass(), LevelPackage, ObjectName.c_str(), RF_Transactional);

							if(InstanceToEdit)
							{
								InstanceToEdit->SetParent(SelectedMaterial);
								SelectedMesh->SetMaterial(MaterialIdx, InstanceToEdit);
								FirstActor->ForceUpdateComponents(FALSE,FALSE);
							}
						}
						else
						{
							appMsgf( AMT_OK, *FString::Printf(*LocalizeUnrealEd("MaterialInstanceNameTaken_F"), ObjectName.c_str()) );
						}
					}
				}

				// Show the material instance editor if we have an instance
				if(InstanceToEdit != NULL)
				{
					wxFrame* MaterialInstanceEditor = new WxMaterialInstanceTimeVaryingEditor( (wxWindow*)GApp->EditorFrame,-1, InstanceToEdit );
					MaterialInstanceEditor->SetSize(1024,768);
					MaterialInstanceEditor->Show();
				}

			}
		}
	}
}


/** Assigns the currently selected generic browser material to the selected material slot. */
void WxEditorFrame::MenuAssignMaterial( wxCommandEvent &In )
{
	INT MaterialIdx = In.GetId() - IDM_ASSIGNMATERIALINSTANCE_START;

	// Find the material and mesh the user chose.
	USelection* SelectedActors = GEditor->GetSelectedActors();

	if(SelectedActors->Num()==1)
	{
		AActor* FirstActor = SelectedActors->GetTop<AActor>();

		if(FirstActor)
		{
			// Find which mesh the user clicked on first.
			for(INT ComponentIdx=0;ComponentIdx<FirstActor->Components.Num(); ComponentIdx++)
			{
				UMeshComponent* MeshComponent = Cast<UMeshComponent>(FirstActor->Components(ComponentIdx));

				if(MeshComponent != NULL)
				{
					const INT NumElements = MeshComponent->GetNumElements();
					if(MaterialIdx >= NumElements)
					{
						MaterialIdx -= NumElements;
					}
					else
					{
						USelection* MaterialSelection = GEditor->GetSelectedObjects();

						if(MaterialSelection && MaterialSelection->Num()==1)
						{
							UMaterialInterface* InstanceToAssign = MaterialSelection->GetTop<UMaterialInterface>();

							if(InstanceToAssign)
							{
								MeshComponent->SetMaterial(ComponentIdx, InstanceToAssign);
								FirstActor->ForceUpdateComponents(FALSE,FALSE);
							}
						}
						break;
					}
				}
			}
		}
	}
}

void WxEditorFrame::MenuActorPivotMoveHere( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("PIVOT HERE") );
}

void WxEditorFrame::MenuActorPivotMoveHereSnapped( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("PIVOT SNAPPED") );
}

void WxEditorFrame::MenuActorPivotMoveCenterOfSelection( wxCommandEvent& In )
{
	GUnrealEd->Exec( TEXT("PIVOT CENTERSELECTION") );
}

void WxEditorFrame::MenuEmitterAutoPopulate(wxCommandEvent& In)
{
	// Iterate over selected Actors.
	for ( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
	{
		AActor* Actor		= static_cast<AActor*>( *It );
		checkSlow( Actor->IsA(AActor::StaticClass()) );

		AEmitter* Emitter	= Cast<AEmitter>( Actor );
		if (Emitter)
		{
			Emitter->AutoPopulateInstanceProperties();
		}
	}
}

void WxEditorFrame::MenuEmitterReset(wxCommandEvent& In)
{
	// Iterate over selected Actors.
	for ( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
	{
		AActor* Actor		= static_cast<AActor*>( *It );
		checkSlow( Actor->IsA(AActor::StaticClass()) );

		AEmitter* Emitter	= Cast<AEmitter>( Actor );
		if (Emitter)
		{
			UParticleSystemComponent* PSysComp = Emitter->ParticleSystemComponent;
			if (PSysComp)
			{
				PSysComp->ResetParticles();
				PSysComp->ActivateSystem();
			}
		}
	}
}

void WxEditorFrame::CreateArchetype(wxCommandEvent& In)
{
	// Create an archetype from the selected actor(s).
	GUnrealEd->edactArchetypeSelected();
}

void WxEditorFrame::CreatePrefab(wxCommandEvent& In)
{
	GUnrealEd->edactPrefabSelected();
}

void WxEditorFrame::AddPrefab(wxCommandEvent& In)
{
	GUnrealEd->edactAddPrefab();
}

void WxEditorFrame::SelectPrefabActors(wxCommandEvent& In)
{
	GUnrealEd->edactSelectPrefabActors();
}

void WxEditorFrame::UpdatePrefabFromInstance(wxCommandEvent& In)
{
	GUnrealEd->edactUpdatePrefabFromInstance();
}

void WxEditorFrame::ResetInstanceFromPrefab(wxCommandEvent& In)
{
	GUnrealEd->edactResetInstanceFromPrefab();
}

void WxEditorFrame::PrefabInstanceToNormalActors(wxCommandEvent& In)
{
	GUnrealEd->edactPrefabInstanceToNormalActors();
}

void WxEditorFrame::PrefabInstanceOpenSequence(wxCommandEvent& In)
{
	APrefabInstance* PrefInst = GEditor->GetSelectedActors()->GetTop<APrefabInstance>();
	if(PrefInst && PrefInst->SequenceInstance)
	{
		WxKismet::OpenSequenceInKismet( PrefInst->SequenceInstance, GApp->EditorFrame );
	}
}

static void PlayFromHere(INT WhichConsole = -1)
{
	UCylinderComponent*	DefaultCollisionComponent = CastChecked<UCylinderComponent>(APlayerStart::StaticClass()->GetDefaultActor()->CollisionComponent);
	FVector				CollisionExtent = FVector(DefaultCollisionComponent->CollisionRadius,DefaultCollisionComponent->CollisionRadius,DefaultCollisionComponent->CollisionHeight),
						StartLocation = GEditor->ClickLocation + GEditor->ClickPlane * (FBoxPushOut(GEditor->ClickPlane,CollisionExtent) + 0.1f);

	FRotator*			StartRotation = NULL;
	// look for a 3d viewport to get the rotation for the play from here
	for (INT ViewportIndex = 0; ViewportIndex < GEditor->ViewportClients.Num(); ViewportIndex++)
	{
		// perspective means 3d, it can have a rotaiton
		if (GEditor->ViewportClients(ViewportIndex)->ViewportType == LVT_Perspective)
		{
			// grab a pointer to the rotation
			StartRotation = &GEditor->ViewportClients(ViewportIndex)->ViewRotation;
			// we just use the first 3d viewport we find
			break;
		}
	}

	// kick off the play from here request
	GUnrealEd->PlayMap(&StartLocation, StartRotation, WhichConsole);
}

void WxEditorFrame::MenuPlayFromHereInEditor( wxCommandEvent& In )
{
	// start a play from here, in the local editor
	PlayFromHere();
}

void WxEditorFrame::MenuPlayFromHereOnConsole( wxCommandEvent& In )
{
	// start a play from here, using the console menu index as which console to run it on
	PlayFromHere(In.GetId() - IDM_BackDropPopupPlayFromHereConsole_START);
}

void WxEditorFrame::PushViewStartStop(wxCommandEvent& In)
{
	// Enable/disable the view pushing flag.
	GEditor->bIsPushingView = In.IsChecked() ? TRUE : FALSE;

	// Update the toolbar.
	MainToolBar->SetPushViewState( GEditor->bIsPushingView );

	// Exec the update.
	GEngine->Exec( GEditor->bIsPushingView ? TEXT("REMOTE PUSHVIEW START") : TEXT("REMOTE PUSHVIEW STOP"), *GLog );
}

void WxEditorFrame::PushViewSync(wxCommandEvent& In)
{
	GEngine->Exec(TEXT("REMOTE PUSHVIEW SYNC"), *GLog);
}

void WxEditorFrame::MenuUseActorFactory( wxCommandEvent& In )
{
	INT ActorFactoryIndex = In.GetId() - IDMENU_ActorFactory_Start;
	check( ActorFactoryIndex >= 0 && ActorFactoryIndex < GEditor->ActorFactories.Num() );

	UActorFactory* ActorFactory = GEditor->ActorFactories(ActorFactoryIndex);
	
	GEditor->UseActorFactory(ActorFactory);
}

void WxEditorFrame::MenuUseActorFactoryAdv( wxCommandEvent& In )
{
	INT ActorFactoryIndex = In.GetId() - IDMENU_ActorFactoryAdv_Start;
	check( ActorFactoryIndex >= 0 && ActorFactoryIndex < GEditor->ActorFactories.Num() );

	UActorFactory* ActorFactory = GEditor->ActorFactories(ActorFactoryIndex);

	// Have a first stab at filling in the factory properties.
	ActorFactory->AutoFillFields( GEditor->GetSelectedObjects() );

	// Display actor factory dialog.
	GApp->DlgActorFactory->ShowDialog( ActorFactory );
}

void WxEditorFrame::MenuReplaceSkelMeshActors(  wxCommandEvent& In )
{
	TArray<ASkeletalMeshActor*> NewSMActors;

	for( FActorIterator It; It; ++It )
	{
		ASkeletalMeshActor* SMActor = Cast<ASkeletalMeshActor>( *It );
		if( SMActor && !NewSMActors.ContainsItem(SMActor) )
		{
			USkeletalMesh* SkelMesh = SMActor->SkeletalMeshComponent->SkeletalMesh;
			FVector Location = SMActor->Location;
			FRotator Rotation = SMActor->Rotation;

			UAnimSet* AnimSet = NULL; 
			if(SMActor->SkeletalMeshComponent->AnimSets.Num() > 0)
			{
				AnimSet = SMActor->SkeletalMeshComponent->AnimSets(0);
			}

			// Find any objects in Kismet that reference this SkeletalMeshActor
			TArray<USequenceObject*> SeqVars;
			if(GWorld->GetGameSequence())
			{
				GWorld->GetGameSequence()->FindSeqObjectsByObjectName(SMActor->GetFName(), SeqVars);
			}

			GWorld->DestroyActor(SMActor);
			SMActor = NULL;

			ASkeletalMeshActor* NewSMActor = CastChecked<ASkeletalMeshActor>( GWorld->SpawnActor( ASkeletalMeshActor::StaticClass(), NAME_None, Location, Rotation, NULL ) );

			// Set up the SkeletalMeshComponent based on the old one.
			NewSMActor->ClearComponents();

			NewSMActor->SkeletalMeshComponent->SkeletalMesh = SkelMesh;

			if(AnimSet)
			{
				NewSMActor->SkeletalMeshComponent->AnimSets.AddItem( AnimSet );
			}

			NewSMActor->ConditionalUpdateComponents();

			// Set Kismet Object Vars to new SMActor
			for(INT j=0; j<SeqVars.Num(); j++)
			{
				USeqVar_Object* ObjVar = Cast<USeqVar_Object>( SeqVars(j) );
				if(ObjVar)
				{
					ObjVar->ObjValue = NewSMActor;
				}
			}
			

			// Remeber this SkeletalMeshActor so we don't try and destroy it.
			NewSMActors.AddItem(NewSMActor);
		}
	}
}

void WxEditorFrame::MenuCleanBSPMaterials(wxCommandEvent& In)
{
	GUnrealEd->Exec( TEXT("CLEANBSPMATERIALS") );
}

/**
 * Handles the deferred docking event notification. Updates the browser panes
 * based upon the event passed in
 */
void WxEditorFrame::OnDockingChange(WxDockEvent& Event)
{
	switch (Event.GetDockingChangeType())
	{
		case DCT_Docking:
		{
			GUnrealEd->GetBrowserManager()->DockBrowserWindow(Event.GetDockID());
			break;
		}
		case DCT_Floating:
		{
			GUnrealEd->GetBrowserManager()->UndockBrowserWindow(Event.GetDockID());
			break;
		}
		case DCT_Clone:
		{
			GUnrealEd->GetBrowserManager()->CloneBrowser(Event.GetDockID());
			break;
		}
		case DCT_Remove:
		{
			GUnrealEd->GetBrowserManager()->RemoveBrowser(Event.GetDockID());
			break;
		}
	}
}


/**
* @return	A pointer to the scale grid menu.
*/
WxScaleGridMenu* WxEditorFrame::GetScaleGridMenu()
{
	return ScaleGridMenu;
}

/**
 * @return	A pointer to the rotation grid menu.
 */
WxRotationGridMenu* WxEditorFrame::GetRotationGridMenu()
{
	return RotationGridMenu;
}

/**
* @return	A pointer to the autosave interval menu.
*/
WxAutoSaveIntervalMenu* WxEditorFrame::GetAutoSaveIntervalMenu()
{
	return AutoSaveIntervalMenu;
}

/**
 * @return	A pointer to the drag grid menu.
 */
WxDragGridMenu* WxEditorFrame::GetDragGridMenu()
{
	return DragGridMenu;
}
