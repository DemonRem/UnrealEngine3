/*=============================================================================
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
//#include "..\..\Launch\Resources\resource.h"
#include "EngineSequenceClasses.h"
#include "EnginePhysicsClasses.h"
#include "EnginePrefabClasses.h"
#include "Menus.h"
#include "SpeedTree.h"

/*-----------------------------------------------------------------------------
	WxMainContextMenuBase.
-----------------------------------------------------------------------------*/

WxMainContextMenuBase::WxMainContextMenuBase()
{
	ActorFactoryMenu = NULL;
	ActorFactoryMenuAdv = NULL;
	RecentClassesMenu = NULL;
}

/**
 * @param	Class	The class to query.
 * @return			TRUE if the specified class can be added to a level.
 */
static UBOOL IsClassPlaceable(const UClass* Class)
{
	const UBOOL bIsAdable =
		Class
		&&  (Class->ClassFlags & CLASS_Placeable)
		&& !(Class->ClassFlags & CLASS_Abstract)
		&& !(Class->ClassFlags & CLASS_Deprecated)
		&& Class->IsChildOf( AActor::StaticClass() );
	return bIsAdable;
}

void WxMainContextMenuBase::AppendAddActorMenu()
{
	USelection* SelectionSet = GEditor->GetSelectedObjects();

	// Actor visibility menu
	wxMenu* HideMenu = new wxMenu();

	HideMenu->Append( IDM_SHOW_ALL, *LocalizeUnrealEd("ShowAllActors"), TEXT("") );
	HideMenu->Append( IDM_SELECT_SHOW, *LocalizeUnrealEd("ShowSelectedActors"), TEXT("") );
	HideMenu->Append( IDM_SELECT_HIDE, *LocalizeUnrealEd("HideSelectedActors"), TEXT("") );
	HideMenu->Append( IDM_SELECT_INVERT, *LocalizeUnrealEd("InvertSelections"), TEXT("") );
	
	Append( IDMENU_HideMenu, *LocalizeUnrealEd("ShowHideActors"), HideMenu );

	// Add 'add actor of selected class' option.
	const UClass* SelClass = SelectionSet->GetTop<UClass>();
	if( IsClassPlaceable( SelClass ) )
	{
		const FString wk = FString::Printf( *LocalizeUnrealEd("AddHereF"), *SelClass->GetName() );
		Append( ID_BackdropPopupAddClassHere, *wk, TEXT("") );
	}

	// 'Add Prefab' menu option, if a prefab is selected.
	UPrefab* Prefab = SelectionSet->GetTop<UPrefab>();
	if(Prefab)
	{
		Append( IDM_ADDPREFAB, *FString::Printf( *LocalizeUnrealEd("AddPrefabF"), *Prefab->GetName() ), TEXT("") );
	}

	// Add an "add <class>" option here for the most recent actor classes that were selected in the level.

	RecentClassesMenu = new wxMenu();

	USelection::TClassConstIterator Itor = GEditor->GetSelectedActors()->ClassConstItor();
	for( ; Itor ; ++Itor )
	{
		if( IsClassPlaceable( *Itor ) )
		{
			const FString wk = FString::Printf( *LocalizeUnrealEd("AddF"), *(*Itor)->GetName() );
			RecentClassesMenu->Append( ID_BackdropPopupAddLastSelectedClassHere_START+Itor.GetIndex(), *wk, TEXT("") );
		}
	}

	Append( IDMENU_SurfPopupAddRecentMenu, *LocalizeUnrealEd("AddRecent"), RecentClassesMenu );

	// Create actor factory entries.
	ActorFactoryMenu = new wxMenu();
	ActorFactoryMenuAdv = new wxMenu();

	FString ActorErrorMsg;
	for(INT i=0; i<GEditor->ActorFactories.Num(); i++)
	{
		UActorFactory* Factory = GEditor->ActorFactories(i);

		// The basic menu only shows factories that can be run without any intervention
		Factory->AutoFillFields( SelectionSet );
		if( Factory->CanCreateActor( ActorErrorMsg ) )
		{
			ActorFactoryMenu->Append( IDMENU_ActorFactory_Start+i, *(Factory->GetMenuName()), TEXT("") );
		}

		// The advanced menu shows all of them.
		ActorFactoryMenuAdv->Append( IDMENU_ActorFactoryAdv_Start+i, *(Factory->GetMenuName()), TEXT("") );
	}

	ActorFactoryMenu->AppendSeparator();
	ActorFactoryMenu->Append( IDMENU_SurfPopupAddActorAdvMenu, *LocalizeUnrealEd("AllTemplates"), ActorFactoryMenuAdv );

	Append( IDMENU_SurfPopupAddActorMenu, *LocalizeUnrealEd("AddActor"), ActorFactoryMenu );

	AppendSeparator();
}

/*-----------------------------------------------------------------------------
	WxMainContextMenu.
-----------------------------------------------------------------------------*/

WxMainContextMenu::WxMainContextMenu()
{
	// Look at what is selected and record information about it for later.

	UBOOL bHaveBrush = FALSE;
	UBOOL bHaveStaticMesh = FALSE;
	UBOOL bHaveDynamicStaticMesh = FALSE;
	UBOOL bHaveSelectedActors = FALSE;
	UBOOL bSelectedActorsBelongToSameLevel = TRUE;
	UBOOL bHaveCamera = FALSE;					// True if a camera is selected.
	UBOOL bHaveKActor = FALSE;
	UBOOL bHaveMover = FALSE;
	UBOOL bHaveEmitter = FALSE;
	UBOOL bHaveRoute = FALSE;
	UBOOL bHaveCoverGroup = FALSE;
	UBOOL bHavePrefabInstance = FALSE;
	UBOOL bHaveActorInPrefab = FALSE;
	UBOOL bHaveLight = FALSE;
	UBOOL bHaveSpeedTree = FALSE;
	UBOOL bAllSelectedLightsHaveSameClassification = TRUE;
	UBOOL bAllSelectedStaticMeshesHaveCollisionModels = TRUE;	// For deciding whether or not to offer StaticMesh->KActor conversion
	UBOOL bFoundLockedActor = FALSE;
	AActor* FirstActor = NULL;
	INT NumSelectedActors = 0;
	INT NumNavPoints = 0;
	INT NumCoverLinks = 0;
	ULevel* SharedLevel = NULL;			// If non-NULL, all selected actors belong to this level.
	ELightAffectsClassification LightAffectsClassification = LAC_MAX; // Only valid if bAllSelectedLightsHaveSameClassification is TRUE.

	for ( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
	{
		AActor* Actor = static_cast<AActor*>( *It );
		checkSlow( Actor->IsA(AActor::StaticClass()) );

		if ( bSelectedActorsBelongToSameLevel )
		{
			ULevel* ActorLevel = Actor->GetLevel();
			if ( !SharedLevel )
			{
				// This is the first selected actor we've encountered.
				SharedLevel = ActorLevel;
			}
			else
			{
				// Does this actor's level match the others?
				if ( SharedLevel != ActorLevel )
				{
					bSelectedActorsBelongToSameLevel = FALSE;
					SharedLevel = NULL;
				}
			}
		}
		
		FirstActor = Actor;

		bHaveSelectedActors = TRUE;

		NumSelectedActors++;

		if ( Actor->bLockLocation )
		{
			bFoundLockedActor = TRUE;
		}

		if( !bHaveBrush && Actor->IsBrush() )
		{
			bHaveBrush = TRUE;
		}
		else if(Actor->IsA(ASpeedTreeActor::StaticClass()))
		{
			bHaveSpeedTree = TRUE;
		}
		else if( Actor->IsA(AStaticMeshActor::StaticClass()) )
		{
			bHaveStaticMesh = TRUE;
			AStaticMeshActor* StaticMeshActor = (AStaticMeshActor*)( Actor );
			if ( StaticMeshActor->StaticMeshComponent )
			{
				bAllSelectedStaticMeshesHaveCollisionModels &= ( (StaticMeshActor->StaticMeshComponent->StaticMesh && StaticMeshActor->StaticMeshComponent->StaticMesh->BodySetup) ? TRUE : FALSE );
			}
			else
			{
				bAllSelectedStaticMeshesHaveCollisionModels = FALSE;
				//appErrorf( TEXT("Static mesh actor has no static mesh component") );
			}
		}
		else if( Actor->IsA(ADynamicSMActor::StaticClass()) )
		{
			bHaveDynamicStaticMesh = TRUE;
			ADynamicSMActor* StaticMeshActor = (ADynamicSMActor*)( Actor );
			if ( StaticMeshActor->StaticMeshComponent )
			{
				bAllSelectedStaticMeshesHaveCollisionModels &= ( (StaticMeshActor->StaticMeshComponent->StaticMesh && StaticMeshActor->StaticMeshComponent->StaticMesh->BodySetup) ? TRUE : FALSE );
			}
			else
			{
				bAllSelectedStaticMeshesHaveCollisionModels = FALSE;
				//appErrorf( TEXT("DynamicSM actor has no static mesh component") );
			}

			if( Actor->IsA(AKActor::StaticClass()) )
			{
				bHaveKActor = TRUE;
			}
			else if( Actor->IsA(AInterpActor::StaticClass()) )
			{ 
				bHaveMover = TRUE; 
			} 
		}
		else if( Actor->IsA(ANavigationPoint::StaticClass()) )
		{
			NumNavPoints++;
			if (Actor->IsA(ACoverLink::StaticClass()))
			{
				NumCoverLinks++;
			}
		}
		else if (Actor->IsA(APrefabInstance::StaticClass()))
		{
			bHavePrefabInstance = TRUE;
		}
		else if ( Actor->IsA(ACameraActor::StaticClass()) )
		{
			bHaveCamera = TRUE;
		}
		else if (Actor->IsA(AEmitter::StaticClass()))
		{
			bHaveEmitter = TRUE;
		}
		else if ( Actor->IsA(ARoute::StaticClass()) )
		{
			bHaveRoute = TRUE;
		}
		else if ( Actor->IsA(ACoverGroup::StaticClass()) )
		{
			bHaveCoverGroup = TRUE;
		}
		else if( Actor->IsA(ALight::StaticClass()) )
		{
			bHaveLight = TRUE;
			if( bAllSelectedLightsHaveSameClassification )
			{
				ALight* Light = (ALight*)( Actor );
				if( Light->LightComponent )
				{
					if( LightAffectsClassification == LAC_MAX )
					{
						LightAffectsClassification = (ELightAffectsClassification)Light->LightComponent->LightAffectsClassification;
					}
					else if( LightAffectsClassification != Light->LightComponent->LightAffectsClassification )
					{
						bAllSelectedLightsHaveSameClassification = FALSE;
					}
				}
				else
				{
					bAllSelectedLightsHaveSameClassification = FALSE;			
				}				
			}
		}

		if( !bHaveActorInPrefab )
		{
			bHaveActorInPrefab = Actor->IsInPrefabInstance();
		}
	}

	// Edit menu functions

	Append( IDM_CUT, *LocalizeUnrealEd("Cut"), *LocalizeUnrealEd("ToolTip_93") );
	Append( IDM_COPY, *LocalizeUnrealEd("Copy"), *LocalizeUnrealEd("ToolTip_94") );
	Append( IDM_PASTE, *LocalizeUnrealEd("Paste"), *LocalizeUnrealEd("ToolTip_95") );
	Append( IDM_PASTE_HERE, *LocalizeUnrealEd("PasteHere"), TEXT("ToolTip_158") );
	AppendSeparator();

	FGetInfoRet gir = GApp->EditorFrame->GetInfo( GI_NUM_SELECTED | GI_CLASSNAME_SELECTED | GI_CLASS_SELECTED );

	if( bHaveSelectedActors )
	{
		Append( IDMENU_ActorPopupProperties, *FString::Printf( *LocalizeUnrealEd("PropertiesSelectedF"), *(gir.String), gir.iValue ), TEXT("") );
		Append( ID_SyncGenericBrowser, *LocalizeUnrealEd("SyncGenericBrowser"), TEXT(""), 0 );
		AppendSeparator();

		// If all selected actors belong to the same level, offer the option to make that level current.
		FString MakeCurrentLevelString;
		if ( bSelectedActorsBelongToSameLevel )
		{
			check( SharedLevel );
			UPackage* LevelPackage = CastChecked<UPackage>( SharedLevel->GetOutermost() );
			MakeCurrentLevelString = FString::Printf( *LocalizeUnrealEd("MakeActorLevelCurrentF"), *LevelPackage->GetName() );
		}
		else
		{
			MakeCurrentLevelString = LocalizeUnrealEd("MakeActorLevelCurrentMultiple");
		}
		Append( ID_MakeSelectedActorsLevelCurrent, *MakeCurrentLevelString, TEXT(""), 0 );
		Append( ID_MoveSelectedActorsToCurrentLevel, *LocalizeUnrealEd("MoveSelectedActorsToCurrentLevel"), TEXT(""), 0 );
		Append( ID_SelectLevelInLevelBrowser, *LocalizeUnrealEd("SelectLevels"), TEXT(""), 0 );
		Append( ID_SelectLevelOnlyInLevelBrowser, *LocalizeUnrealEd("SelectLevelsOnly"), TEXT(""), 0 );
		Append( ID_DeselectLevelInLevelBrowser, *LocalizeUnrealEd("DeselectLevels"), TEXT(""), 0 );

		// If only 1 Actor is selected, and its referenced by Kismet, offer option to find it.
		if(NumSelectedActors == 1)
		{
			check(FirstActor);
			// Get the kismet sequence for the level the actor belongs to.
			USequence* RootSeq = GWorld->GetGameSequence( FirstActor->GetLevel() );
			if( RootSeq && RootSeq->ReferencesObject(FirstActor) )
			{
				Append( IDMENU_FindActorInKismet, *FString::Printf(*LocalizeUnrealEd("FindKismetF"), *FirstActor->GetName()), TEXT("") );
			}
		}

		AppendSeparator();
	}
	
	// add some path building options if nav points are selected
	if( NumNavPoints > 0 || 
		bHaveRoute ||
		bHaveCoverGroup )
	{
		PathMenu = new wxMenu();

		if( NumNavPoints > 0 )
		{
			PathMenu->Append( IDMENU_ActorPopupPathPosition, *LocalizeUnrealEd("AutoPosition"), TEXT("") );
			PathMenu->AppendSeparator();
		
			// if multiple path nodes are selected, add options to force/proscribe
			if( NumNavPoints > 1 )
			{
				PathMenu->Append( IDMENU_ActorPopupPathProscribe, *LocalizeUnrealEd("ProscribePaths"), TEXT("") );
				PathMenu->Append( IDMENU_ActorPopupPathForce, *LocalizeUnrealEd("ForcePaths"), TEXT("") );
				PathMenu->AppendSeparator();
			}
			PathMenu->Append( IDMENU_ActorPopupPathClearProscribed, *LocalizeUnrealEd("ClearProscribedPaths"), TEXT("") );
			PathMenu->Append( IDMENU_ActorPopupPathClearForced, *LocalizeUnrealEd("ClearForcedPaths"), TEXT("") );

			if ( NumCoverLinks > 1 )
			{
				PathMenu->AppendSeparator();
				PathMenu->Append( IDMENU_ActorPopupPathStitchCover, *LocalizeUnrealEd("StitchCover"), TEXT("") );
			}
		}

		if( bHaveRoute || bHaveCoverGroup  )
		{
			ComplexPathMenu = new wxMenu();
			if( bHaveRoute )
			{
				ComplexPathMenu->Append( IDMENU_ActorPopupPathOverwriteRoute, *LocalizeUnrealEd("OverwriteNavPointsInRoute"), TEXT("") );
				ComplexPathMenu->Append( IDMENU_ActorPopupPathAddRoute, *LocalizeUnrealEd("AddNavPointsToRoute"), TEXT("") );
				ComplexPathMenu->Append( IDMENU_ActorPopupPathRemoveRoute, *LocalizeUnrealEd("RemoveNavPointsFromRoute"), TEXT("") );
				ComplexPathMenu->Append( IDMENU_ActorPopupPathClearRoute, *LocalizeUnrealEd("ClearNavPointsFromRoute"), TEXT("") );
			}
			if( bHaveRoute && bHaveCoverGroup )
			{
				ComplexPathMenu->AppendSeparator();
			}
			if( bHaveCoverGroup )
			{
				ComplexPathMenu->Append( IDMENU_ActorPopupPathOverwriteCoverGroup, *LocalizeUnrealEd("OverwriteLinksInCoverGroup"), TEXT("") );
				ComplexPathMenu->Append( IDMENU_ActorPopupPathAddCoverGroup, *LocalizeUnrealEd("AddLinksToCoverGroup"), TEXT("") );
				ComplexPathMenu->Append( IDMENU_ActorPopupPathRemoveCoverGroup, *LocalizeUnrealEd("RemoveLinksFromCoverGroup"), TEXT("") );
				ComplexPathMenu->Append( IDMENU_ActorPopupPathClearCoverGroup, *LocalizeUnrealEd("ClearLinksFromCoverGroup"), TEXT("") );
			}

			if( NumNavPoints > 0 )
			{
				PathMenu->AppendSeparator();
			}
			PathMenu->Append( IDMENU_ActorPopupComplexPathMenu, *LocalizeUnrealEd("ComplexPathOptions"), ComplexPathMenu );
		}
		else
		{
			ComplexPathMenu = NULL;
		}

		Append( IDMENU_ActorPopupPathMenu, *LocalizeUnrealEd("PathOptions"), PathMenu );
		AppendSeparator();
	}
	else
	{
		PathMenu = NULL;
	}

	// Pivot menu - Note that we are grouping pivot and transform submenus in with the Brush menu
	// items if they exist

	PivotMenu = new wxMenu();

	PivotMenu->Append( IDMENU_ActorPopupResetPivot, *LocalizeUnrealEd("Reset"), TEXT("") );
	PivotMenu->Append( ID_BackdropPopupPivot, *LocalizeUnrealEd("MoveHere"), TEXT("") );
	PivotMenu->Append( ID_BackdropPopupPivotSnapped, *LocalizeUnrealEd("MoveHereSnapped"), TEXT("") );
	PivotMenu->Append( ID_BackdropPopupPivotSnappedCenterSelection, *LocalizeUnrealEd("MoveCenterSelection"), TEXT("") );
	Append( ID_BackdropPopupPivotMenu, *LocalizeUnrealEd("Pivot"), PivotMenu );

	// Transform menu

	TransformMenu = new wxMenu();

	TransformMenu->Append( IDMENU_ActorPopupMirrorX, *LocalizeUnrealEd("MirrorXAxis"), TEXT("") );
	TransformMenu->Append( IDMENU_ActorPopupMirrorY, *LocalizeUnrealEd("MirrorYAxis"), TEXT("") );
	TransformMenu->Append( IDMENU_ActorPopupMirrorZ, *LocalizeUnrealEd("MirrorZAxis"), TEXT("") );
	Append( ID_BackdropPopupTransformMenu, *LocalizeUnrealEd("Transform"), TransformMenu );


	AppendSeparator();

	if(bHaveBrush)
	{
		OrderMenu = new wxMenu();
		OrderMenu->Append( IDMENU_ActorPopupToFirst, *LocalizeUnrealEd("ToFirst"), TEXT("") );
		OrderMenu->Append( IDMENU_ActorPopupToLast, *LocalizeUnrealEd("ToLast"), TEXT("") );
		Append( IDMENU_ActorPopupOrderMenu, *LocalizeUnrealEd("Order"), OrderMenu );

		PolygonsMenu = new wxMenu();
		PolygonsMenu->Append( IDMENU_ActorPopupToBrush, *LocalizeUnrealEd("ToBrush"), TEXT("") );
		PolygonsMenu->Append( IDMENU_ActorPopupFromBrush, *LocalizeUnrealEd("FromBrush"), TEXT("") );
		PolygonsMenu->AppendSeparator();
		PolygonsMenu->Append( IDMENU_ActorPopupMerge, *LocalizeUnrealEd("Merge"), TEXT("") );
		PolygonsMenu->Append( IDMENU_ActorPopupSeparate, *LocalizeUnrealEd("Separate"), TEXT("") );
		Append( IDMENU_ActorPopupPolysMenu, *LocalizeUnrealEd("Polygons"), PolygonsMenu );

		CSGMenu = new wxMenu();
		CSGMenu->Append( IDMENU_ActorPopupMakeAdd, *LocalizeUnrealEd("Additive"), TEXT("") );
		CSGMenu->Append( IDMENU_ActorPopupMakeSubtract, *LocalizeUnrealEd("Subtractive"), TEXT("") );
		Append( IDMENU_ActorPopupCSGMenu, *LocalizeUnrealEd("CSG"), CSGMenu );

		SolidityMenu = new wxMenu();
		SolidityMenu->Append( IDMENU_ActorPopupMakeSolid, *LocalizeUnrealEd("Solid"), TEXT("") );
		SolidityMenu->Append( IDMENU_ActorPopupMakeSemiSolid, *LocalizeUnrealEd("SemiSolid"), TEXT("") );
		SolidityMenu->Append( IDMENU_ActorPopupMakeNonSolid, *LocalizeUnrealEd("NonSolid"), TEXT("") );
		Append( IDMENU_ActorPopupSolidityMenu, *LocalizeUnrealEd("Solidity"), SolidityMenu );


		// Volume menu
		{
			TArray< UClass* > VolumeClasses;

			GApp->EditorFrame->GetSortedVolumeClasses( &VolumeClasses );

			// Create the actual menu by looping through our sorted array and adding the menu items
			VolumeMenu = new wxMenu();

			INT ID = IDM_VolumeClasses_START;

			for( INT VolumeIdx = 0; VolumeIdx < VolumeClasses.Num(); VolumeIdx++ )
			{
				VolumeMenu->Insert( 0, ID, *VolumeClasses( VolumeIdx )->GetName(), TEXT(""), 0 );

				ID++;
			}

			Append( IDMENU_ActorPopupVolumeMenu, *LocalizeUnrealEd("AddVolumePopup"), VolumeMenu );
		}

		SelectMenu = new wxMenu();
		SelectMenu->Append( IDMENU_ActorPopupSelectBrushesAdd, *LocalizeUnrealEd("AddsSolids"), TEXT("") );
		SelectMenu->Append( IDMENU_ActorPopupSelectBrushesSubtract, *LocalizeUnrealEd("Subtracts"), TEXT("") );
		SelectMenu->Append( IDMENU_ActorPopupSelectBrushesSemisolid, *LocalizeUnrealEd("SemiSolids"), TEXT("") );
		SelectMenu->Append( IDMENU_ActorPopupSelectBrushesNonsolid, *LocalizeUnrealEd("NonSolids"), TEXT("") );
		Append( IDMENU_ActorPopupSelectBrushMenu, *LocalizeUnrealEd("Select"), SelectMenu );

		// Select all by class.
		Append( IDMENU_ActorPopupSelectAllClass, *FString::Printf( *LocalizeUnrealEd("SelectAllF"), *(gir.String) ), TEXT("") );
	}
	else
	{	
		OrderMenu = NULL;
		PolygonsMenu = NULL;
		CSGMenu = NULL;
	}

	if( bHaveSelectedActors )
	{
		// If any selected actors have locked locations, don't present the option to align/snap to floor.
		if ( !bFoundLockedActor )
		{

			AlignMenu = new wxMenu();
			AlignMenu->Append( IDMENU_SnapToFloor, *LocalizeUnrealEd("SnapToFloor"), TEXT("") );
			AlignMenu->Append( IDMENU_AlignToFloor, *LocalizeUnrealEd("AlignToFloor"), TEXT("") );
			AlignMenu->Append( IDMENU_MoveToGrid, *LocalizeUnrealEd("MoveActorToGrid"), TEXT("") );
			Append( IDMENU_ActorPopupAlignMenu, *LocalizeUnrealEd("Align"), AlignMenu );
		}

		Append( IDMENU_ActorPopupAlignCameras, *LocalizeUnrealEd("AlignCameras"), TEXT("") );
		Append( IDMENU_ActorPopupLockMovement, *LocalizeUnrealEd("ToggleLockLocations"), TEXT("") );

		if ( bHaveCamera )
		{
			Append( IDMENU_ActorPopupSnapViewToCamera, *LocalizeUnrealEd("SnapViewToCamera"), TEXT("") );
		}
		AppendSeparator();
	}

	// Only display this down here if we do not have any brush commands.
	if( !bHaveBrush )
	{
		Append( IDMENU_ActorPopupSelectAllClass, *FString::Printf( *LocalizeUnrealEd("SelectAllF"), *(gir.String) ), TEXT("") );
	}

	// Select by property.
	FString PropertyValue;
	UProperty* Property;
	UClass* CommonBaseClass;
	FEditPropertyChain* PropertyChain;
	GEditor->GetPropertyColorationTarget( PropertyValue, Property, CommonBaseClass, PropertyChain );
	if ( Property && CommonBaseClass )
	{
		Append( IDM_SELECT_ByProperty, *FString::Printf( *LocalizeUnrealEd("SelectByPropertyF"), *CommonBaseClass->GetName(), *Property->GetName(), *PropertyValue ), TEXT("") );
	}

	if( bHaveStaticMesh || bHaveDynamicStaticMesh )
	{
		Append( IDMENU_ActorPopupSelectMatchingStaticMeshesThisClass, *LocalizeUnrealEd("SelectMatchingStaticMeshesThisClass"), TEXT("") );
		Append( IDMENU_ActorPopupSelectMatchingStaticMeshesAllClasses, *LocalizeUnrealEd("SelectMatchingStaticMeshesAllClasses"), TEXT("") );
	}

	if(bHaveSpeedTree)
	{
		Append( IDMENU_ActorPopupSelectMatchingSpeedTrees, *LocalizeUnrealEd("SelectMatchingSpeedTrees"), TEXT("") );
	}

	Append( IDMENU_ActorPopupSelectKismetReferenced, *LocalizeUnrealEd("SelectKismetReferencedActors"), TEXT("") );
	Append( IDMENU_ActorPopupSelectKismetUnreferenced, *LocalizeUnrealEd("SelectKismetUnreferencedActors"), TEXT("") );

	//
	// Conversion Related Items
	//

	if(bHaveBrush)
	{
		Append( IDMENU_QuantizeVertices, *LocalizeUnrealEd("SnapToGrid"), TEXT("") );
		Append( IDMENU_ConvertToStaticMesh, *LocalizeUnrealEd("ConvertStaticMesh"), TEXT("") );

	}

	Append( IDMENU_SaveBrushAsCollision, *LocalizeUnrealEd("SaveBrushAsCollision"), TEXT("") );

	if( bHaveKActor || bHaveStaticMesh || bHaveMover )
	{
		wxMenu* ConvertMenu = new wxMenu();

		if(bHaveKActor)
		{
			ConvertMenu->Append(IDMENU_ActorPopupConvertKActorToStaticMesh, *LocalizeUnrealEd("ConvertKActorToStaticMesh"), TEXT("") );
			ConvertMenu->Append(IDMENU_ActorPopupConvertKActorToMover, *LocalizeUnrealEd("ConvertKActorToMover"), TEXT("") );
		}

		if(bHaveStaticMesh)
		{
			if ( bAllSelectedStaticMeshesHaveCollisionModels )
			{
				ConvertMenu->Append(IDMENU_ActorPopupConvertStaticMeshToKActor, *LocalizeUnrealEd("ConvertStaticMeshToKActor"), TEXT("") );
			}
			ConvertMenu->Append(IDMENU_ActorPopupConvertStaticMeshToMover, *LocalizeUnrealEd("ConvertStaticMeshToMover"), TEXT("") );
		}

		if(bHaveMover)
		{
			ConvertMenu->Append(IDMENU_ActorPopupConvertMoverToStaticMesh, *LocalizeUnrealEd("ConvertMoverToStaticMesh"), TEXT("") );
			ConvertMenu->Append(IDMENU_ActorPopupConvertMoverToKActor, *LocalizeUnrealEd("ConvertMoverToKActor"), TEXT("") );
		}

		Append( IDMENU_ActorPopupConvertMenu, *LocalizeUnrealEd("ConvertMenu"), ConvertMenu);
	}

	if (bHaveEmitter)
	{
		wxMenu* EmitterOptionsMenu = new wxMenu();

		AppendSeparator();
		EmitterOptionsMenu->Append(IDMENU_EmitterPopupOptionsAutoPopulate, *LocalizeUnrealEd("EmitterAutoPopulate"), TEXT(""));
		EmitterOptionsMenu->Append(IDMENU_EmitterPopupOptionsReset, *LocalizeUnrealEd("EmitterReset"), TEXT(""));
		Append(IDMENU_EmitterPopupOptionsMenu, *LocalizeUnrealEd("EmitterPopupMenu"), EmitterOptionsMenu);
	}

	if( bHaveLight == TRUE )
	{
		Append( IDMENU_ActorPopupToggleDynamicChannel, *LocalizeUnrealEd("ToggleDynamicChannel"), TEXT("") );
		Append( IDMENU_ActorPopupSelectAllLights, *LocalizeUnrealEd("SelectAllLights"), TEXT("") );
		if( bAllSelectedLightsHaveSameClassification )
		{
			Append( IDMENU_ActorPopupSelectAllLightsWithSameClassification, *LocalizeUnrealEd("SelectAllLightsWithSameClassification"), TEXT("") );
		}
		
		wxMenu* ConvertMenu = new wxMenu();
		ConvertMenu->Append( IDMENU_IDMENU_ActorPopupConvertLightToLightDynamicAffecting, *LocalizeUnrealEd("ConvertLightToLightDynamicAffecting"), TEXT("") );
		ConvertMenu->Append( IDMENU_IDMENU_ActorPopupConvertLightToLightStaticAffecting, *LocalizeUnrealEd("ConvertLightToLightStaticAffecting"), TEXT("") );
		ConvertMenu->Append( IDMENU_IDMENU_ActorPopupConvertLightToLightDynamicAndStaticAffecting, *LocalizeUnrealEd("ConvertLightToLightDynamicAndStaticAffecting"), TEXT("") );
		Append( IDMENU_ActorPopupConvertMenu, *LocalizeUnrealEd("LightAffectsClassificationMenuEntry"), ConvertMenu);
	}

	// Hook in archetype and materials menu.
	if(NumSelectedActors > 0)
	{
		AppendSeparator();	
		if(bHaveActorInPrefab)
		{
			Append( IDM_SELECTALLACTORSINPREFAB, *LocalizeUnrealEd("Prefab_SelectActors"), TEXT("") );
		}

		if(bHavePrefabInstance)
		{
			APrefabInstance* PrefInst = GEditor->GetSelectedActors()->GetTop<APrefabInstance>();
			check(PrefInst);

			Append( IDM_UPDATEPREFABFROMINSTANCE, *FString::Printf(*LocalizeUnrealEd("Prefab_UpdateFromInstance"), *PrefInst->TemplatePrefab->GetName()), TEXT("") );
			Append( IDM_RESETFROMPREFAB, *FString::Printf(*LocalizeUnrealEd("Prefab_ResetFromPrefab"), *PrefInst->TemplatePrefab->GetName()), TEXT("") );
			Append( IDM_CONVERTPREFABTONORMALACTORS, *LocalizeUnrealEd("Prefab_ToNormalActors"), TEXT("") );

			if(PrefInst->SequenceInstance)
			{
				Append( IDM_OPENPREFABINSTANCESEQUENCE, *LocalizeUnrealEd("Prefab_OpenInstanceSequence"), TEXT("") );
			}
		}
		if(NumSelectedActors == 1)
		{
			Append( IDM_CREATEARCHETYPE, *LocalizeUnrealEd("Archetype_Create"), TEXT("") );
		}
		Append( IDM_CREATEPREFAB, *LocalizeUnrealEd("Prefab_Create"), TEXT("") );


		// Add Materials Menu
		if(FirstActor != NULL)
		{
			// check to see if the current actor has any mesh components.
			TArray<UMeshComponent*> MeshComponents;
			for(INT ComponentIdx=0;ComponentIdx<FirstActor->Components.Num(); ComponentIdx++)
			{
				UMeshComponent* MeshComponent = Cast<UMeshComponent>(FirstActor->Components(ComponentIdx));

				if(MeshComponent != NULL)
				{
					MeshComponents.AddItem(MeshComponent);
				}
			}

			if(MeshComponents.Num())
			{
				wxMenu* MeshMenu = new wxMenu();
				INT MenuIdx = 0;

				// Add a submenu for each mesh
				for(INT MeshIdx=0; MeshIdx<MeshComponents.Num(); MeshIdx++)
				{
					UMeshComponent* MeshComponent = MeshComponents(MeshIdx);
					wxMenu *MaterialMenu;

					// If we only have 1 mesh, dont bother showing a mesh submenu.
					if(MeshComponents.Num()==1)
					{
						MaterialMenu = MeshMenu;
					}
					else
					{
						MaterialMenu = new wxMenu;
					}
					
					for(INT MaterialIdx=0; MaterialIdx<MeshComponent->GetNumElements(); MaterialIdx++)
					{
						UMaterialInterface* MaterialInterface = MeshComponent->GetMaterial(MaterialIdx);

						wxMenu* EditCreateMenu = new wxMenu;
						{
							EditCreateMenu->Append(IDM_CREATE_MATERIAL_INSTANCE_CONSTANT_START+MenuIdx, *LocalizeUnrealEd("CreateMaterialInstanceConstant"), TEXT(""));
							EditCreateMenu->Append(IDM_EDIT_MATERIAL_INSTANCE_CONSTANT_START+MenuIdx, *LocalizeUnrealEd("EditMaterialInstanceConstant"), TEXT(""));
							EditCreateMenu->Append(IDM_CREATE_MATERIAL_INSTANCE_TIME_VARYING_START+MenuIdx, *LocalizeUnrealEd("CreateMaterialInstanceTimeVarying"), TEXT(""));
							EditCreateMenu->Append(IDM_EDIT_MATERIAL_INSTANCE_TIME_VARYING_START+MenuIdx, *LocalizeUnrealEd("EditMaterialInstanceTimeVarying"), TEXT(""));
							EditCreateMenu->Append(IDM_ASSIGNMATERIALINSTANCE_START+MenuIdx, *LocalizeUnrealEd("AssignMaterial"), TEXT(""));
						}
						MaterialMenu->Append(wxID_ANY, *MaterialInterface->GetName(), EditCreateMenu);
						MenuIdx++;
					}

					if(MaterialMenu != MeshMenu)
					{
						MeshMenu->Append(wxID_ANY, *MeshComponent->GetName(), MaterialMenu);
					}
				}

				AppendSeparator();
				Append( wxID_ANY, *LocalizeUnrealEd("MaterialsSubMenu"), MeshMenu );
			}
		}
	}		

	// Hook in AddActor menu.
	AppendSeparator();

	AppendAddActorMenu();

	// if we have any console plugins, add them to the list of places we can play the level
	if (GConsoleSupportContainer->GetNumConsoleSupports() > 0)
	{
		// loop through all consoles (only support 20 consoles)
		INT ConsoleIndex = 0;
		for (FConsoleSupportIterator It; It && ConsoleIndex < 20; ++It, ConsoleIndex++)
		{
			// add a per-console Play From Here On XXX menu
			Append(
				IDM_BackDropPopupPlayFromHereConsole_START + ConsoleIndex, 
				*FString::Printf(*LocalizeUnrealEd("PlayOnF"), It->GetConsoleName()), 
				*FString::Printf(*LocalizeUnrealEd("ToolTip_136"), It->GetConsoleName())
				);
		}
	}

	// we always can play in the editor
	Append(IDM_BackDropPopupPlayFromHereInEditor, *LocalizeUnrealEd("ToolTip_137"), *LocalizeUnrealEd("ToolTip_135"));
}

/*-----------------------------------------------------------------------------
	WxMainContextSurfaceMenu.
-----------------------------------------------------------------------------*/

WxMainContextSurfaceMenu::WxMainContextSurfaceMenu()
{
	FGetInfoRet gir = GApp->EditorFrame->GetInfo( GI_NUM_SURF_SELECTED );

	FString TmpString;

	TmpString = FString::Printf( *LocalizeUnrealEd("SurfacePropertiesF"), gir.iValue );
	Append( IDM_SURFACE_PROPERTIES, *TmpString, TEXT("") );
	Append( ID_SyncGenericBrowser, *LocalizeUnrealEd("SyncGenericBrowser"), TEXT(""), 0 );

	AppendSeparator();

	// Edit menu functions

	Append( IDM_CUT, *LocalizeUnrealEd("Cut"), *LocalizeUnrealEd("ToolTip_93") );
	Append( IDM_COPY, *LocalizeUnrealEd("Copy"), *LocalizeUnrealEd("ToolTip_94") );
	Append( IDM_PASTE, *LocalizeUnrealEd("Paste"), *LocalizeUnrealEd("ToolTip_95") );
	Append( IDM_PASTE_HERE, *LocalizeUnrealEd("PasteHere"), TEXT("ToolTip_158") );

	AppendSeparator();

	SelectSurfMenu = new wxMenu();
	SelectSurfMenu->Append( ID_SurfPopupSelectMatchingGroups, *LocalizeUnrealEd("MatchingGroups"), TEXT("") );
	SelectSurfMenu->Append( ID_SurfPopupSelectMatchingItems, *LocalizeUnrealEd("MatchingItems"), TEXT("") );
	SelectSurfMenu->Append( ID_SurfPopupSelectMatchingBrush, *LocalizeUnrealEd("MatchingBrush"), TEXT("") );
	SelectSurfMenu->Append( ID_SurfPopupSelectMatchingTexture, *LocalizeUnrealEd("MatchingTexture"), TEXT("") );
	SelectSurfMenu->AppendSeparator();
	SelectSurfMenu->Append( ID_SurfPopupSelectAllAdjacents, *LocalizeUnrealEd("AllAdjacents"), TEXT("") );
	SelectSurfMenu->Append( ID_SurfPopupSelectAdjacentCoplanars, *LocalizeUnrealEd("AdjacentCoplanars"), TEXT("") );
	SelectSurfMenu->Append( ID_SurfPopupSelectAdjacentWalls, *LocalizeUnrealEd("AdjacentWalls"), TEXT("") );
	SelectSurfMenu->Append( ID_SurfPopupSelectAdjacentFloors, *LocalizeUnrealEd("AdjacentFloors"), TEXT("") );
	SelectSurfMenu->Append( ID_SurfPopupSelectAdjacentSlants, *LocalizeUnrealEd("AdjacentSlants"), TEXT("") );
	SelectSurfMenu->AppendSeparator();
	SelectSurfMenu->Append( ID_SurfPopupSelectReverse, *LocalizeUnrealEd("Reverse"), TEXT("") );
	SelectSurfMenu->AppendSeparator();
	SelectSurfMenu->Append( ID_SurfPopupMemorize, *LocalizeUnrealEd("MemorizeSet"), TEXT("") );
	SelectSurfMenu->Append( ID_SurfPopupRecall, *LocalizeUnrealEd("RecallMemory"), TEXT("") );
	SelectSurfMenu->Append( ID_SurfPopupOr, *LocalizeUnrealEd("OrWithMemory"), TEXT("") );
	SelectSurfMenu->Append( ID_SurfPopupAnd, *LocalizeUnrealEd("AndWithMemory"), TEXT("") );
	SelectSurfMenu->Append( ID_SurfPopupXor, *LocalizeUnrealEd("XorWithMemory"), TEXT("") );
	Append( IDMNEU_SurfPopupSelectSurfMenu, *LocalizeUnrealEd("SelectSurfaces"), SelectSurfMenu );

	Append( ID_EditSelectAllSurfs, *LocalizeUnrealEd("SelectAllSurface"), TEXT("") );
	Append( ID_EditSelectNone, *LocalizeUnrealEd("SelectNone"), TEXT("") );

	AppendSeparator();

	UMaterialInterface* mi = GEditor->GetSelectedObjects()->GetTop<UMaterialInterface>();
	FString Wk = *LocalizeUnrealEd("ApplyMaterial");
	if( mi )
	{
		Wk = FString::Printf( *LocalizeUnrealEd("ApplyMaterialF"), *mi->GetName() );
	}
	Append( ID_SurfPopupApplyMaterial, *Wk, TEXT("") );
	Append( ID_SurfPopupReset, *LocalizeUnrealEd("Reset"), TEXT("") );

	AlignmentMenu = new wxMenu();
	AlignmentMenu->Append( ID_SurfPopupUnalign, *LocalizeUnrealEd("Default"), TEXT("") );
	AlignmentMenu->Append( ID_SurfPopupAlignPlanarAuto, *LocalizeUnrealEd("Planar"), TEXT("") );
	AlignmentMenu->Append( ID_SurfPopupAlignPlanarWall, *LocalizeUnrealEd("PlanarWall"), TEXT("") );
	AlignmentMenu->Append( ID_SurfPopupAlignPlanarFloor, *LocalizeUnrealEd("PlanarFloor"), TEXT("") );
	AlignmentMenu->Append( ID_SurfPopupAlignBox, *LocalizeUnrealEd("Box"), TEXT("") );
	AlignmentMenu->Append( ID_SurfPopupAlignFace, *LocalizeUnrealEd("Face"), TEXT("") );
	Append( IDMENU_SurfPopupAlignmentMenu, *LocalizeUnrealEd("Alignment"), AlignmentMenu );

	AppendSeparator();

	AppendAddActorMenu();

	// if we have any console plugins, add them to the list of places we can play the level
	if (GConsoleSupportContainer->GetNumConsoleSupports() > 0)
	{
		// loop through all consoles (only support 20 consoles)
		INT ConsoleIndex = 0;
		for (FConsoleSupportIterator It; It && ConsoleIndex < 20; ++It, ConsoleIndex++)
		{
			// add a per-console Play From Here On XXX menu
			Append(
				IDM_BackDropPopupPlayFromHereConsole_START + ConsoleIndex, 
				*FString::Printf(*LocalizeUnrealEd("PlayOnF"), It->GetConsoleName()), 
				*FString::Printf(*LocalizeUnrealEd("ToolTip_136"), It->GetConsoleName())
				);
		}
	}

	// we always can play in the editor
	Append(IDM_BackDropPopupPlayFromHereInEditor, *LocalizeUnrealEd("ToolTip_137"), *LocalizeUnrealEd("ToolTip_135"));
}

/*-----------------------------------------------------------------------------
	WxMainContextCoverSlotMenu.
-----------------------------------------------------------------------------*/

WxMainContextCoverSlotMenu::WxMainContextCoverSlotMenu(class ACoverLink *Link, FCoverSlot &Slot)
{
	Append( -1, *FString::Printf(TEXT("Cover: %s"),*Link->GetName()), TEXT("") );
	AppendSeparator();
	Append( IDM_CoverEditMenu_ToggleEnabled, *FString::Printf(TEXT("Toggle Enabled [%s]"),Slot.bEnabled?TEXT("Enabled"):TEXT("Disabled")), TEXT("") );
	Append( IDM_CoverEditMenu_ToggleType, *FString::Printf(TEXT("Toggle Type [%s]"),Slot.ForceCoverType == CT_Standing?TEXT("Standing"):Slot.ForceCoverType==CT_MidLevel?TEXT("MidLevel"):TEXT("Auto")), TEXT("") );
	Append( IDM_CoverEditMenu_ToggleCoverslip, *FString::Printf(TEXT("Toggle Coverslip [%s]"),Slot.bAllowCoverSlip?TEXT("Allowed"):TEXT("Not Allowed")), TEXT("") );
	Append( IDM_CoverEditMenu_ToggleSwatTurn, *FString::Printf(TEXT("Toggle Swat Turn [%s]"),Slot.bAllowSwatTurn?TEXT("Allowed"):TEXT("Not Allowed")), TEXT("") );
	if (Slot.CoverType == CT_MidLevel)
	{
		Append( IDM_CoverEditMenu_ToggleMantle, *FString::Printf(TEXT("Toggle Mantle [%s]"),Slot.bAllowMantle?TEXT("Allowed"):TEXT("Not Allowed")), TEXT("") );
		Append( IDM_CoverEditMenu_TogglePopup, *FString::Printf(TEXT("Toggle Popup [%s]"),Slot.bAllowPopup?TEXT("Allowed"):TEXT("Not Allowed")), TEXT("") );
		Append( IDM_CoverEditMenu_ToggleClimbUp, *FString::Printf(TEXT("Toggle Climb Up [%s]"),Slot.bAllowClimbUp?TEXT("Allowed"):TEXT("Not Allowed")), TEXT("") );
	}
}
