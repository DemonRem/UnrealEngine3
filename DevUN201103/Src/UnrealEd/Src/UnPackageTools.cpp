/*=============================================================================
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "UnPackageTools.h"


#include "UnObjectTools.h"
#include "BusyCursor.h"

#include "LocalizationExport.h"
#include "SourceControl.h"

#include "FileHelpers.h"

#if GB_SUPPORTED
#include "GenericBrowser.h"
#endif



/** Pointer to a function Called during GC, after reachability analysis is performed but before garbage is purged. */
typedef void (*EditorPostReachabilityAnalysisCallbackType)();
extern EditorPostReachabilityAnalysisCallbackType EditorPostReachabilityAnalysisCallback;

namespace PackageTools
{
	/** State passed to RestoreStandaloneOnReachableObjects. */
	static UPackage* PackageBeingUnloaded = NULL;
	static TMap<UObject*,UObject*> ObjectsThatHadFlagsCleared;

	/**
	 * Called during GC, after reachability analysis is performed but before garbage is purged.
	 * Restores RF_Standalone to objects in the package-to-be-unloaded that are still reachable.
	 */
	void RestoreStandaloneOnReachableObjects()
	{
		for( FObjectIterator It ; It ; ++It )
		{
			UObject* Object = *It;
			if ( !Object->HasAnyFlags(RF_Unreachable) && Object->GetOutermost() == PackageBeingUnloaded )
			{
				if ( ObjectsThatHadFlagsCleared.Find(Object) )
				{
					Object->SetFlags(RF_Standalone);
				}
			}
		}
	}



	/**
	 * Filters the global set of packages.
	 *
	 * @param	ObjectTypes					List of allowed object types (or NULL for all types)
	 * @param	OutGroupPackages			The map that receives the filtered list of group packages.
	 * @param	OutPackageList				The array that will contain the list of filtered packages.
	 */
	void GetFilteredPackageList( const TMap< UClass*, TArray< UGenericBrowserType* > >* ObjectTypes,
								 TSet<const UPackage*>& OutFilteredPackageMap,
								 TSet<UPackage*>* OutGroupPackages,
								 TArray<UPackage*>& OutPackageList )
	{
		// The UObject list is iterated rather than the UPackage list because we need to be sure we are only adding
		// group packages that contain things the generic browser cares about.  The packages are derived by walking
		// the outer chain of each object.

		// Assemble a list of packages.  Only show packages that match the current resource type filter.
		for( TObjectIterator<UObject> It ; It ; ++It )
		{
			UObject* Obj = *It;

			// Make sure that we support displaying this object type
			UBOOL bIsSupported = ObjectTools::IsObjectBrowsable( Obj, ObjectTypes );
			if( bIsSupported )
			{
				UPackage* ObjectPackage = Cast< UPackage >( Obj->GetOutermost() );
				if( ObjectPackage != NULL )
				{
					OutFilteredPackageMap.Add( ObjectPackage );
				}

				if( OutGroupPackages != NULL )
				{
					for ( UObject* NextOuter = Obj->GetOuter(); NextOuter && NextOuter->GetOuter(); NextOuter = NextOuter->GetOuter() )
					{
						UPackage* NextGroup = Cast<UPackage>(NextOuter);
						if ( NextGroup != NULL && !OutGroupPackages->Contains(NextGroup) )
						{
							OutGroupPackages->Add(NextGroup);
						}
					}
				}
			}
		}

		// Make a TArray copy of PackageMap.
		for ( TSet<const UPackage*>::TConstIterator It( OutFilteredPackageMap ) ; It ; ++It )
		{
			OutPackageList.AddItem( const_cast<UPackage*>( *It ) );
		}
	}

	/**
	 * Fills the OutObjects list with all valid objects that are supported by the current
	 * browser settings and that reside withing the set of specified packages.
	 *
	 * @param	InPackages			Filters objects based on package.
	 * @param	ObjectTypes			List of allowed object types (or NULL for all types)
	 * @param	OutObjects			[out] Receives the list of objects
	 */
	void GetObjectsInPackages( const TArray<UPackage*>* InPackages,
							   const TMap< UClass*, TArray< UGenericBrowserType* > >* ObjectTypes,
							   TArray<UObject*>& OutObjects )
	{
		// Iterate over all objects.
		for( TObjectIterator<UObject> It ; It ; ++It )
		{
			UObject* Obj = *It;

			// Should we filter based on a list of allowed packages?
			if( InPackages != NULL )
			{
				// Make sure this object resides in one of the specified packages.
				UBOOL bIsInPackage = FALSE;
				for ( INT PackageIndex = 0 ; PackageIndex < InPackages->Num() ; ++PackageIndex )
				{
					const UPackage* Package = ( *InPackages )(PackageIndex);
					if ( Obj->IsIn(Package) )
					{
						bIsInPackage = TRUE;
						break;
					}
				}

				if( !bIsInPackage )
				{
					continue;
				}
			}


			// Filter out invalid objects early.
			UBOOL bIsSupported = ObjectTools::IsObjectBrowsable( Obj, ObjectTypes );
			if( bIsSupported )
			{
				// Add to the list.
				OutObjects.AddItem( Obj );

					// if usage filter is enabled, and this object's Outer isn't part of the original list of objects to display
					// (because it wasn't directly referenced by an object in the referencer container list, for example), add it now
					// and mark it with RF_TagImp to indicate that we've added this object
					// @todo CB: Usage filter shenanigans [reviewed: post-APR09 QA build]
// 					if ( bUsageFilterEnabled )
// 					{
// 						for ( UObject* ResourceOuter = Obj->GetOuter(); ResourceOuter && !ResourceOuter->HasAnyFlags(RF_TagImp|RF_TagExp) &&
// 							ResourceOuter->GetClass() != UPackage::StaticClass(); ResourceOuter = ResourceOuter->GetOuter() )
// 						{
// 							OutObjects.AddItem(ResourceOuter);
// 							ResourceOuter->SetFlags(RF_TagImp);
// 						}
// 					}
			}
		}
	}

	/**
	 * Handles fully loading passed in packages.
	 *
	 * @param	TopLevelPackages	Packages to be fully loaded.
	 * @param	OperationString		Localization key for a string describing the operation; appears in the warning string presented to the user.
	 * 
	 * @return TRUE if all packages where fully loaded, FALSE otherwise
	 */
	UBOOL HandleFullyLoadingPackages( const TArray<UPackage*>& TopLevelPackages, const TCHAR* OperationString )
	{
		UBOOL bSuccessfullyCompleted = TRUE;

		// whether or not to suppress the ask to fully load message
		UBOOL bSuppress = GEditor->AccessUserSettings().bSuppressFullyLoadPrompt;

		// Make sure they are all fully loaded.
		UBOOL bNeedsUpdate = FALSE;
		for( INT PackageIndex=0; PackageIndex<TopLevelPackages.Num(); PackageIndex++ )
		{
			UPackage* TopLevelPackage = TopLevelPackages(PackageIndex);
			check( TopLevelPackage );
			check( TopLevelPackage->GetOuter() == NULL );

			if( !TopLevelPackage->IsFullyLoaded() )
			{	
				// Ask user to fully load or suppress the message and just fully load
				if(bSuppress || appMsgf( AMT_YesNo, *FString::Printf( LocalizeSecure(LocalizeUnrealEd(TEXT("NeedsToFullyLoadPackageF")), *TopLevelPackage->GetName(), *LocalizeUnrealEd(OperationString)) ) ) )
				{
					// Fully load package.
					const FScopedBusyCursor BusyCursor;
					GWarn->BeginSlowTask( *LocalizeUnrealEd(TEXT("FullyLoadingPackages")), TRUE );
					TopLevelPackage->FullyLoad();
					GWarn->EndSlowTask();
					bNeedsUpdate = TRUE;
				}
				// User declined abort operation.
				else
				{
					bSuccessfullyCompleted = FALSE;
					debugf(TEXT("Aborting operation as %s was not fully loaded."),*TopLevelPackage->GetName());
					break;
				}
			}
		}

		// no need to refresh content browser here as UPackage::FullyLoad() already does this
		return bSuccessfullyCompleted;
	}

	/**
	 * Attempts to save the specified packages; helper function for by e.g. SaveSelectedPackages().
	 *
	 * @param		PackagesToSave				The content packages to save.
	 * @param		bUnloadPackagesAfterSave	If TRUE, unload each package if it was saved successfully.
	 * @param		pLastSaveDirectory			if specified, initializes the "Save File" dialog with this value
	 *											[out] will be filled in with the directory the user chose for this save operation.
	 * @param		InResourceTypes				If specified and the user saved a new package under a different name, we can 
	 *											delete the original objects to avoid duplicated objects. 
	 *
	 * @return									TRUE if all packages were saved successfully, FALSE otherwise.
	 */
	UBOOL SavePackages( const TArray<UPackage*>& PackagesToSave, UBOOL bUnloadPackagesAfterSave, FString* pLastSaveDirectory /*=NULL*/, const TArray< UGenericBrowserType* >* InResourceTypes/*=NULL*/ )
	{
		// Get outermost packages, in case groups were selected.
		TArray<UPackage*> Packages;
		for( INT PackageIndex = 0 ; PackageIndex < PackagesToSave.Num() ; ++PackageIndex )
		{
			UPackage* Package = PackagesToSave(PackageIndex);
			if ( Package != NULL )
			{
				Packages.AddUniqueItem( Package->GetOutermost() ? (UPackage*)Package->GetOutermost() : Package );
			}
		}

		// Packages must be fully loaded before they can be saved.
		if( !HandleFullyLoadingPackages( Packages, TEXT("Save") ) )
		{
			return FALSE;
		}

		// Assume that all packages were saved.
		UBOOL bAllPackagesWereSaved = TRUE;
		// If any packages were saved, we'll need to update the package list
		UBOOL bAnyPackagesWereSaved = FALSE;

		// Present the user with a 'save file' dialog.
		FString FileTypes( TEXT("All Files|*.*") );
		for(INT i=0; i<GSys->Extensions.Num(); i++)
		{
			FileTypes += FString::Printf( TEXT("|(*.%s)|*.%s"), *GSys->Extensions(i), *GSys->Extensions(i) );
		}

		TArray<UPackage*> PackagesToUnload;

		GWarn->BeginSlowTask( *LocalizeUnrealEd(TEXT("SavingPackage")), TRUE );
		for(INT i=0; i<Packages.Num(); i++)
		{
			UPackage* PackageToSave = Packages(i);
			if ( PackageToSave != NULL )
			{
				GWarn->StatusUpdatef( i, Packages.Num(), *FString::Printf( LocalizeSecure(LocalizeUnrealEd("SavingPackagef"), *PackageToSave->GetName() )) );

				// Prevent level packages from being saved via the Generic Browser.
				if( FindObject<UWorld>( PackageToSave, TEXT("TheWorld") ) )
				{
					appMsgf( AMT_OK, LocalizeSecure(LocalizeUnrealEd("Error_CantSaveMapViaGB"), *PackageToSave->GetName()) );
					bAllPackagesWereSaved = FALSE;
				}
				else
				{
					FString SaveFileName;

					FString PackageName = PackageToSave->GetName();
					FString ExistingFile;

					if( GPackageFileCache->FindPackageFile( *PackageName, NULL, ExistingFile ) )
					{
						// If the package already exists, save to the existing filename.
						SaveFileName = ExistingFile;
					}
					else
					{
						FString LastSaveDirectory;
						if ( pLastSaveDirectory != NULL )
						{
							LastSaveDirectory = *pLastSaveDirectory;
						}

						// Couldn't find the package name; present a file dialog so the user can specify a name.
						const FString File = FString::Printf( TEXT("%s.upk"), *PackageName );
						debugf(TEXT("NO_EXISTING: %s, %s, %s"), *ExistingFile, *File, *LastSaveDirectory);

						// Loop through until a valid filename is given or the user presses cancel
						UBOOL bFilenameIsValid = FALSE;
						while( !bFilenameIsValid )
						{
							WxFileDialog SaveFileDialog(GApp->EditorFrame, 
								*LocalizeUnrealEd("SavePackage"), 
								*LastSaveDirectory,
								*File,
								*FileTypes,
								wxSAVE,
								wxDefaultPosition);

							if( SaveFileDialog.ShowModal() == wxID_OK )
							{
								LastSaveDirectory = SaveFileDialog.GetDirectory().c_str();
								SaveFileName = FString( SaveFileDialog.GetPath() );
								
								FFilename FileInfo = SaveFileName;
								// If the supplied file name is missing an extension then give it the default package
								// file extension.
								if( SaveFileName.Len() > 0 && FileInfo.GetExtension().Len() == 0 )
								{
									SaveFileName += TEXT( ".upk" );
								}						
								
								if( !FEditorFileUtils::IsFilenameValidForSaving( SaveFileName ) )
								{
									SaveFileName.Empty();
									appMsgf( AMT_OK, LocalizeSecure( LocalizeUnrealEd( "Error_FilenameIsTooLongForCooking" ), *FileInfo.GetBaseFilename(), FEditorFileUtils::MAX_UNREAL_FILENAME_LENGTH ) );
									// Start the loop over, prompting for save again
									continue;
								}
								else
								{
									// Filename is valid, dont promt the user again
									bFilenameIsValid = TRUE;
								}

								if ( pLastSaveDirectory != NULL )
								{
									*pLastSaveDirectory = LastSaveDirectory;
								}
							}
							else
							{
								// user canceled the save dialog.  Do not prompt again
								break;
							}
						}
					}

					if ( SaveFileName.Len() )
					{
						FString SavePackageName = FFilename(SaveFileName).GetBaseFilename();
						const UBOOL bSavedPackageAsANewName = ((PackageToSave->GetName() != SavePackageName));

						if( bSavedPackageAsANewName )
						{
							// At this point, we are saving a new package (i.e. a package that hasn't been saved to disk yet). 
							// We have to remove this package from the package view BEFORE SAVING THE PACKAGE. Otherwise, we 
							// will have duplicate entries of the package in the package view, one listed under the folder 
							// it was saved on disk and one in the "NewPackages" folder.
							GCallbackEvent->Send(FCallbackEventParameters(NULL, CALLBACK_RefreshContentBrowser, CBR_NewPackageSaved, PackageToSave, SavePackageName));
						}

						const FScopedBusyCursor BusyCursor;
						debugf(TEXT("saving as %s"), *SaveFileName);
						if( GFileManager->IsReadOnly( *SaveFileName ) )
						{
							appMsgf( AMT_OK, *FString::Printf( LocalizeSecure(LocalizeUnrealEd("Error_CouldntWriteToFile_F"), *SaveFileName)) );
							bAllPackagesWereSaved = FALSE;
						}
						else if( !GUnrealEd->Exec( *FString::Printf(TEXT("OBJ SAVEPACKAGE PACKAGE=\"%s\" FILE=\"%s\""), *PackageName, *SaveFileName) ) )
						{
							// Couldn't save.
							appMsgf( AMT_OK, *LocalizeUnrealEd("Error_CouldntSavePackage") );
							bAllPackagesWereSaved = FALSE;
						}
						else
						{
							bAnyPackagesWereSaved = TRUE;

							// The package saved successfully.  Unload it?
							if ( bUnloadPackagesAfterSave || bSavedPackageAsANewName )
							{
								PackagesToUnload.AddItem( PackageToSave );
							}
						}
					}
					else
					{
						// Couldn't save; no filename to save as.
						bAllPackagesWereSaved = FALSE;
					}
				}
			}
		}


		if( PackagesToUnload.Num() )
		{
			PackageTools::UnloadPackages( PackagesToUnload );
		}

		GWarn->EndSlowTask();

		if ( bAnyPackagesWereSaved )
		{
			GCallbackEvent->Send(FCallbackEventParameters(NULL, CALLBACK_RefreshContentBrowser, CBR_UpdatePackageListUI));
		}

		return bAllPackagesWereSaved;
	}
	
	/**
	 * Loads the specified package file (or returns an existing package if it's already loaded.)
	 *
	 * @param	InFilename	File name of package to load
	 *
	 * @return	The loaded package (or NULL if something went wrong.)
	 */
	UPackage* LoadPackage( FFilename InFilename )
	{
		// Detach all components while loading a package.
		// This is necessary for the cases where the load replaces existing objects which may be referenced by the attached components.
		FGlobalComponentReattachContext ReattachContext;

		// if the package is already loaded, reset it so it will be fully reloaded
		UObject* ExistingPackage = UObject::StaticFindObject(UPackage::StaticClass(), ANY_PACKAGE, *InFilename.GetBaseFilename());
		if (ExistingPackage)
		{
			UObject::ResetLoaders(ExistingPackage);
		}

		// record the name of this file to make sure we load objects in this package on top of in-memory objects in this package
		GEditor->UserOpenedFile = InFilename;

		// clear any previous load errors
		EdClearLoadErrors();

		UPackage* Package = Cast<UPackage>(UObject::LoadPackage( NULL, *InFilename, 0 ));

		// display any load errors that happened while loading the package
		if (GEdLoadErrors.Num())
		{
			GCallbackEvent->Send( CALLBACK_DisplayLoadErrors );
		}

		// reset the opened package to nothing
		GEditor->UserOpenedFile = FString("");

		// If a script package was loaded, update the
		// actor browser in case a script package was loaded
		if ( Package != NULL )
		{
			if ( (Package->PackageFlags & PKG_ContainsScript) != 0 )
			{
				GCallbackEvent->Send( CALLBACK_RefreshEditor_ActorBrowser );
			}

			GCallbackEvent->Send(FCallbackEventParameters(NULL, CALLBACK_RefreshContentBrowser, CBR_UpdatePackageList, Package));
		}

		return Package;
	}


	/**
	 * Helper function that attempts to unlaod the specified top-level packages.
	 *
	 * @param	PackagesToUnload	the list of packages that should be unloaded
	 *
	 * @return	TRUE if the set of loaded packages was changed
	 */
	UBOOL UnloadPackages( const TArray<UPackage*>& TopLevelPackages )
	{
		UBOOL bResult = FALSE;

		// Get outermost packages, in case groups were selected.
		TArray<UPackage*> PackagesToUnload;

		// Split the set of selected top level packages into packages which are dirty (and thus cannot be unloaded)
		// and packages that are not dirty (and thus can be unloaded).
		TArray<UPackage*> DirtyPackages;
		for ( INT PackageIndex = 0 ; PackageIndex < TopLevelPackages.Num() ; ++PackageIndex )
		{
			UPackage* Package = TopLevelPackages(PackageIndex);
			if( Package != NULL )
			{
				if ( Package->IsDirty() )
				{
					DirtyPackages.AddItem( Package );
				}
				else
				{
					PackagesToUnload.AddUniqueItem( Package->GetOutermost() ? Package->GetOutermost() : Package );
				}
			}
		}

		// Inform the user that dirty packages won't be unloaded.
		if ( DirtyPackages.Num() > 0 )
		{
			FString DirtyPackagesMessage( LocalizeUnrealEd(TEXT("UnloadDirtyPackagesList")) );
			for ( INT PackageIndex = 0 ; PackageIndex < DirtyPackages.Num() ; ++PackageIndex )
			{
				DirtyPackagesMessage += FString::Printf( TEXT("\n    %s"), *DirtyPackages(PackageIndex)->GetName() );
			}
			DirtyPackagesMessage += FString::Printf(TEXT("\n%s"), *LocalizeUnrealEd(TEXT("UnloadDirtyPackagesSave")) );
			appMsgf( AMT_OK, TEXT("%s"), *DirtyPackagesMessage );
		}

		if ( PackagesToUnload.Num() > 0 )
		{
			const FScopedBusyCursor BusyCursor;

			// Complete any load/streaming requests, then lock IO.
			UObject::FlushAsyncLoading();
			(*GFlushStreamingFunc)();
			//FIOManagerScopedLock ScopedLock(GIOManager);

			// Remove potential references to to-be deleted objects from the GB selection set and thumbnail manager preview components.
			GEditor->GetSelectedObjects()->DeselectAll();
			GUnrealEd->GetThumbnailManager()->ClearComponents();

#if GB_SUPPORTED

			// Prevent the right container from updating (due to paint events) and creating references to contenxt.
			if( GBCompatTools::GetGenericBrowser() != NULL )
			{
				GBCompatTools::GetGenericBrowser()->RightContainer->DisableUpdate();
			}
#endif

			// Set the callback for restoring RF_Standalone post reachability analysis.
			// GC will call this function before purging objects, allowing us to restore RF_Standalone
			// to any objects that have not been marked RF_Unreachable.
			EditorPostReachabilityAnalysisCallback = RestoreStandaloneOnReachableObjects;

			UBOOL bScriptPackageWasUnloaded = FALSE;

			GWarn->BeginSlowTask( *LocalizeUnrealEd(TEXT("Unloading")), TRUE );
			for ( INT PackageIndex = 0 ; PackageIndex < PackagesToUnload.Num() ; ++PackageIndex )
			{
				PackageBeingUnloaded = PackagesToUnload(PackageIndex);
				check( !PackageBeingUnloaded->IsDirty() );
				GWarn->StatusUpdatef( PackageIndex, PackagesToUnload.Num(), *FString::Printf( LocalizeSecure(LocalizeUnrealEd("Unloadingf"), *PackageBeingUnloaded->GetName()) ) );

				PackageBeingUnloaded->bHasBeenFullyLoaded = FALSE;
				if ( PackageBeingUnloaded->PackageFlags & PKG_ContainsScript )
				{
					bScriptPackageWasUnloaded = TRUE;
				}

				// Clear RF_Standalone flag from objects in the package to be unloaded so they get GC'd.
				for( FObjectIterator It ; It ; ++It )
				{
					UObject* Object = *It;
					if( Object->HasAnyFlags(RF_Standalone) && Object->GetOutermost() == PackageBeingUnloaded )
					{
						Object->ClearFlags(RF_Standalone);
						ObjectsThatHadFlagsCleared.Set( Object, Object );
					}
				}

				// Collect garbage.
				UObject::CollectGarbage( GARBAGE_COLLECTION_KEEPFLAGS );

				// Cleanup.
				ObjectsThatHadFlagsCleared.Empty();
				PackageBeingUnloaded = NULL;
				bResult = TRUE;
			}
			GWarn->EndSlowTask();

			// Set the post reachability callback.
			EditorPostReachabilityAnalysisCallback = NULL;

			UObject::CollectGarbage( GARBAGE_COLLECTION_KEEPFLAGS );

#if GB_SUPPORTED
			WxGenericBrowser* GB = GBCompatTools::GetGenericBrowser();
			if( GB != NULL )
			{
				GB->RightContainer->EnableUpdate();
				GB->Update();
			}
#endif

			// Update the actor browser if a script package was unloaded
			if ( bScriptPackageWasUnloaded )
			{
				GCallbackEvent->Send( CALLBACK_RefreshEditor_ActorBrowser );
			}

			if ( bResult )
			{
				GCallbackEvent->Send(FCallbackEventParameters(NULL, CALLBACK_RefreshContentBrowser, CBR_UpdatePackageListUI));
			}
		}
		return bResult;
	}

	/**
	 *	Exports the given packages to files.
	 *
	 *	@param	PackagesToExport	The set of packages to export.
	 * @param	out_ExportPath		receives the value of the path the user chose for exporting.
	 */
	void ExportPackages( const TArray<UPackage*>& PackagesToExport, FString* out_ExportPath/*=NULL*/ )
	{
		if ( PackagesToExport.Num() == 0 )
		{
			return;
		}

		FString LastExportPath = out_ExportPath != NULL ? *out_ExportPath : GApp->LastDir[LD_GENERIC_EXPORT];
		FFilename SelectedExportPath;

		// Get the directory to export to...
		wxDirDialog ChooseDirDialog(
			GApp->EditorFrame,
			*LocalizeUnrealEd("ChooseADirectory"),
			*LastExportPath
			);

		if ( ChooseDirDialog.ShowModal() != wxID_OK )
		{
			return;
		}
		SelectedExportPath = FFilename( ChooseDirDialog.GetPath() );

		// Copy off the selected path for future export operations.
		LastExportPath = SelectedExportPath;

		GWarn->BeginSlowTask( *LocalizeUnrealEd(TEXT("Exporting")), TRUE );

		// Create an array of all available exporters.
		TArray<UExporter*> Exporters;
		ObjectTools::AssembleListOfExporters( Exporters );

		// Export the objects.
		for (INT Index = 0; Index < PackagesToExport.Num(); Index++)
		{
			GWarn->StatusUpdatef( Index, PackagesToExport.Num(), *FString::Printf( LocalizeSecure(LocalizeUnrealEd("Exportingf"), Index, PackagesToExport.Num()) ) );

			UPackage* PackageToExport = PackagesToExport(Index);
			if (!PackageToExport)
			{
				continue;
			}

			// Can't export cooked content.
			if (PackageToExport->PackageFlags & PKG_Cooked)
			{
				continue;
			}

			UPackageExporterT3D* PackageExporter = new UPackageExporterT3D();
			check(PackageExporter);
			FFilename SaveFileName;

			// Assemble a filename from the export directory and the object path.
			SaveFileName = SelectedExportPath;
			// Assemble a path only from the package name.
			SaveFileName *= PackageToExport->GetName();
			SaveFileName += TEXT(".T3DPKG");
			debugf(TEXT("Exporting \"%s\" to \"%s\""), *PackageToExport->GetPathName(), *SaveFileName );

			// Create the path, then make sure the target file is not read-only.
			const FString ObjectExportPath( SaveFileName.GetPath() );
			if ( !GFileManager->MakeDirectory( *ObjectExportPath, TRUE ) )
			{
				appMsgf( AMT_OK, *FString::Printf( LocalizeSecure(LocalizeUnrealEd("Error_FailedToMakeDirectory"), *ObjectExportPath)) );
			}
			else if( GFileManager->IsReadOnly( *SaveFileName ) )
			{
				appMsgf( AMT_OK, *FString::Printf( LocalizeSecure(LocalizeUnrealEd("Error_CouldntWriteToFile_F"), *SaveFileName)) );
			}
			else
			{
				// We have a writeable file.  Now go through that list of exporters again and find the right exporter and use it.
				// If an exporter was found, use it.
				if( PackageExporter )
				{
					ObjectTools::WxDlgExportGeneric dlg;
					dlg.ShowModal( SaveFileName, PackageToExport, PackageExporter, FALSE );
				}
			}
		}

		GWarn->EndSlowTask();

		if ( out_ExportPath )
		{
			*out_ExportPath = LastExportPath;
		}
	}

	/**
	 * Wrapper method for multiple objects at once.
	 *
	 * @param	TopLevelPackages		the packages to be export
	 * @param	ClassToObjectTypeMap	mapping of class to GBT's which support that class.
	 * @param	LastExportPath			the path that the user last exported assets to
	 * @param	FilteredClasses			if specified, set of classes that should be the only types exported if not exporting to single file
	 *
	 * @return	the path that the user chose for the export.
	 */
	FString DoBulkExport(const TArray<UPackage*>& TopLevelPackages, FString LastExportPath, TMap<UClass*, TArray<UGenericBrowserType*> >* ClassToObjectTypeMap, const TSet<UClass*>* FilteredClasses /* = NULL */ )
	{
		// Disallow export if any packages are cooked.
		if (!DisallowOperationOnCookedPackages(TopLevelPackages)
		&&	HandleFullyLoadingPackages( TopLevelPackages, TEXT("BulkExportE") ) )
		{
			// Prompt the user to see if they want to export the packages in a single file.
			const UBOOL bDumpSingleFilePackage = appMsgf(AMT_YesNo, *LocalizeUnrealEd(TEXT("Prompt_ExportToSinglePackage")));
			if ( bDumpSingleFilePackage )
			{
				ExportPackages(TopLevelPackages, &LastExportPath);
			}
			else
			{
				TArray<UObject*> ObjectsInPackages;
				GetObjectsInPackages(&TopLevelPackages, ClassToObjectTypeMap, ObjectsInPackages);

				// If a filtered set of classes was provided, only export objects that are of the same class as
				// one of the filtered classes
				TArray<UObject*> FilteredObjects;
				if ( FilteredClasses )
				{
					// Present the user with a warning that only the filtered types are being exported
					WxSuppressableWarningDialog PromptAboutFiltering( LocalizeUnrealEd("BulkExport_FilteredWarning"), LocalizeUnrealEd("BulkExport_FilteredWarning_Title"), "BulkExportFilterWarning" );
					PromptAboutFiltering.ShowModal();

					for ( TArray<UObject*>::TConstIterator ObjIter(ObjectsInPackages); ObjIter; ++ObjIter )
					{
						UObject* CurObj = *ObjIter;
						if ( CurObj && FilteredClasses->Contains( CurObj->GetClass() ) )
						{
							FilteredObjects.AddItem( CurObj );
						}
					}
				}

				// If a filtered set was provided, export the filtered objects array; otherwise, export all objects in the packages
				TArray<UObject*>& ObjectsToExport = FilteredClasses ? FilteredObjects : ObjectsInPackages;

				// Prompt the user about how many objects will be exported before proceeding.
				const UBOOL bProceed = appMsgf( AMT_YesNo, *FString::Printf( LocalizeSecure(LocalizeUnrealEd(TEXT("Prompt_AboutToBulkExportNItems_F")), ObjectsToExport.Num()) ) );
				if ( bProceed )
				{
					ObjectTools::ExportObjects( ObjectsToExport, FALSE, &LastExportPath );
				}
			}
		}

		return LastExportPath;
	}


	/**
	* Bulk Imports files based on directory structure
	* 
	* @param	The last import path the user selected
	*
	* @return	The import path the user chose for the bulk export
	*/
	FString DoBulkImport(FString LastImportPath)
	{
		wxDirDialog ChooseDirDialog( GApp->EditorFrame, *LocalizeUnrealEd("ChooseADirectory"), *LastImportPath );

		if ( ChooseDirDialog.ShowModal() == wxID_OK )
		{
			FString ImportPath( ChooseDirDialog.GetPath() );

			// All the factories we can possibly use
			TArray< UFactory* > Factories;

			// FStrings required by AssembleListOfImportFactores.  We dont use them here.
			FString FileTypes, FileExtensions;

			// Get the list of factories we can use to import
			ObjectTools::AssembleListOfImportFactories( Factories, FileTypes, FileExtensions );

			// Get a list of files, and ask the user if its ok to proceed with importing the files.  Last chance to back out!
			TArray < FString > FoundFiles;
			appFindFilesInDirectory( FoundFiles, *ImportPath, FALSE, TRUE);

			if( FoundFiles.Num() == 0)
			{
				appMsgf(AMT_OK, LocalizeSecure( LocalizeUnrealEd("Error_NoFilesInDirectory"), *ImportPath ) );
			}
			else if( appMsgf(AMT_YesNo, LocalizeSecure( LocalizeUnrealEd("Prompt_AboutToBulkImportNItems_F"), FoundFiles.Num() ) ) )
			{
				if( !ObjectTools::BulkImportFiles(ImportPath, Factories) )
				{
					appMsgf( AMT_OK, *LocalizeUnrealEd("BulkImport_SomeFilesNotImported") );
				}
				else
				{
					appMsgf( AMT_OK, *LocalizeUnrealEd("BulkImport_Success") );
				}

				GCallbackEvent->Send(FCallbackEventParameters(NULL, CALLBACK_RefreshContentBrowser, CBR_UpdatePackageList, NULL));

				LastImportPath = ImportPath;
			}
		
		}

		return LastImportPath;
	}
	/**
	 * Utility method for exporting localized text for the assets contained in the specified packages.
	 *
	 * @param	TopLevelPackages	
	 */
	FString ExportLocalization( const TArray<UPackage*>& TopLevelPackages, FString LastExportPath, TMap<UClass*, TArray<UGenericBrowserType*> >* ClassToObjectTypeMap )
	{
		// Disallow export if any packages are cooked.
		if (!DisallowOperationOnCookedPackages(TopLevelPackages)
		&&	HandleFullyLoadingPackages( TopLevelPackages, TEXT("FullLocExportE") ) )
		{
			const UBOOL bExportLocBinaries = appMsgf( AMT_YesNo, *LocalizeUnrealEd(TEXT("Prompt_ExportLocBinaries")) );
			if ( bExportLocBinaries )
			{
				// 1. Do bulk export first:
				LastExportPath = DoBulkExport(TopLevelPackages, LastExportPath, ClassToObjectTypeMap);
			}
			else
			{
				// prompt the user to select a target directory.
				wxDirDialog ChooseDirDialog(
					GApp->EditorFrame,
					*LocalizeUnrealEd("ChooseADirectory"),
					*LastExportPath
					);

				if ( ChooseDirDialog.ShowModal() != wxID_OK )
				{
					return LastExportPath;
				}

				// Copy off the selected path for future export operations.
				LastExportPath = ChooseDirDialog.GetPath();
			}

			// 2. Do exportloc call:
			const UBOOL bCompareAgainstDefaults = appMsgf( AMT_YesNo, *LocalizeUnrealEd(TEXT("Prompt_ExportLocCompareAgainstDefaults")) );
			for ( INT PackageIndex = 0 ; PackageIndex < TopLevelPackages.Num() ; ++PackageIndex )
			{
				UPackage* Package		= TopLevelPackages(PackageIndex);
				FString IntName			= Package->GetName();

				INT i = IntName.InStr( TEXT("."), TRUE );
				if ( i >= 0 )
				{
					IntName = IntName.Left(i);
				}
				IntName += TEXT(".int");

				// Remove path separators from the name.
				i = IntName.InStr( TEXT("/"), TRUE );
				if ( i >= 0 )
				{
					IntName = IntName.Right( IntName.Len()-i-1 );
				}
				i = IntName.InStr( TEXT("\\"), TRUE );
				if ( i >= 0 )
				{
					IntName = IntName.Right( IntName.Len()-i-1 );
				}

				IntName = LastExportPath + PATH_SEPARATOR + IntName;
				FLocalizationExport::ExportPackage( Package, *IntName, bCompareAgainstDefaults, TRUE );
			}
		}

		return LastExportPath;
	}

	/**
	 * Displays an error message if any of the packages in the list have been cooked.
	 *
	 * @param	Packages	The list of packages to check for cooked status.
	 *
	 * @return	TRUE if cooked packages were found; false otherwise.
	 */
	UBOOL DisallowOperationOnCookedPackages(const TArray<UPackage*>& Packages)
	{
		for( INT PackageIndex = 0 ; PackageIndex < Packages.Num() ; ++PackageIndex )
		{
			const UPackage* Package = Packages(PackageIndex);
			if( Package->PackageFlags & PKG_Cooked )
			{
				appMsgf( AMT_OK, *LocalizeUnrealEd("Error_OperationDisallowedOnCookedContent") );
				return FALSE;
			}
		}

		return FALSE;
	}

	void CheckOutRootPackages( const TArray<UPackage*>& Packages )
	{
#if HAVE_SCC
		if (FSourceControl::IsEnabled())
		{
			// Update to the latest source control state.
			FSourceControl::UpdatePackageStatus(Packages);

			TArray<UPackage*> TouchedPackages;
			TArray<FString> TouchedPackageNames;
			UBOOL bCheckedSomethingOut = FALSE;
			for( INT PackageIndex = 0 ; PackageIndex < Packages.Num() ; ++PackageIndex )
			{
				UPackage* Package = Packages(PackageIndex);
				INT SCCState = GPackageFileCache->GetSourceControlState(*Package->GetName());
				if( SCCState == SCC_ReadOnly)
				{
					// The package is still available, so do the check out.
					bCheckedSomethingOut = TRUE;
					TouchedPackages.AddUniqueItem(Package);
					TouchedPackageNames.AddItem(Package->GetName());
				}
				else
				{
					// The status on the package has changed to something inaccessible, so we have to disallow the check out.
					// Don't warn if the file isn't in the depot.
					if ( SCCState != SCC_NotInDepot )
					{			
						appMsgf( AMT_OK, *LocalizeUnrealEd("Error_PackageStatusChanged") );
					}
				}
			}

			// Synchronize source control state if something was checked out.
			DWORD UpdateMask = CBR_UpdatePackageListUI;
			if ( bCheckedSomethingOut )
			{
				UpdateMask |= CBR_UpdateSCCState;
			}

			FSourceControl::ConvertPackageNamesToSourceControlPaths(TouchedPackageNames);
			FSourceControl::CheckOut(NULL, TouchedPackageNames);

			// Refresh.
			FCallbackEventParameters Parms(NULL, CALLBACK_RefreshContentBrowser, UpdateMask);
			for ( INT ObjIndex = 0; ObjIndex < TouchedPackages.Num(); ObjIndex++ )
			{
				Parms.EventObject = TouchedPackages(ObjIndex);
				GCallbackEvent->Send(Parms);			
			}
		}
#endif
	}

	/**
	 * Checks if the passed in path is in an external directory. I.E Ones not found automatically in the content directory
	 *
	 * @param	PackagePath	Path of the package to check, relative or absolute
	 * @return	TRUE if PackagePath points to an external location
	 */
	UBOOL IsPackagePathExternal( const FString& PackagePath )
	{
		UBOOL bIsExternal = TRUE;
		TArray< FString > Paths;
		GConfig->GetArray( TEXT("Core.System"), TEXT("Paths"), Paths, GEngineIni );
	
		//check for script paths too as they might be passed in
		TArray< FString > ScriptPackagePaths;  
		appGetScriptPackageDirectories( ScriptPackagePaths );

		//add script paths to the full list of paths to check
		Paths.Append( ScriptPackagePaths );

		FFilename PackageFilename = appConvertRelativePathToFull( PackagePath );

		// absolute path of the package that was passed in, without the actual name of the package
		FString PackageFullPath = PackageFilename.GetPath();

		for(INT pathIdx = 0; pathIdx < Paths.Num(); ++pathIdx)
		{ 
			const FString& AbsolutePathName = appConvertRelativePathToFull( Paths( pathIdx ) );

			// check if the package path is within the list of paths the engine searches.
			if( PackageFullPath.InStr( AbsolutePathName, FALSE, TRUE ) != -1 )
			{
				bIsExternal = FALSE;
				break;
			}
		}

		return bIsExternal;
	}

	/**
	 * Checks if the passed in package's filename is in an external directory. I.E Ones not found automatically in the content directory
	 *
	 * @param	Package	The package to check
	 * @return	TRUE if the package points to an external filename
	 */
	UBOOL IsPackageExternal(const UPackage& Package)
	{
		FString FileString;
		GPackageFileCache->FindPackageFile( *Package.GetName(), NULL, FileString );

		return IsPackagePathExternal( FileString );
	}
	/**
	 * Checks if the passed in packages have any references to  externally loaded packages.  I.E Ones not found automatically in the content directory
	 *
	* @param	PackagesToCheck					The packages to check
	 * @param	OutPackagesWithExternalRefs		Optional list of packages that have external references
	 * @return	TRUE if PackageToCheck has references to an externally loaded package
	 */
	UBOOL CheckForReferencesToExternalPackages(const TArray<UPackage*>& PackagesToCheck, TArray<UPackage*>* OutPackagesWithExternalRefs)
	{
		UBOOL bHasExternalPackageRefs = FALSE;

		// Find all external packages
		TSet< const UPackage* > FilteredPackageMap;
		TArray< UPackage* > LoadedPackageList;
		GetFilteredPackageList( NULL, FilteredPackageMap, NULL, LoadedPackageList );

		TArray< UPackage* > ExternalPackages;
		for(INT pkgIdx = 0; pkgIdx < LoadedPackageList.Num(); ++pkgIdx)
		{
			UPackage* Pkg = LoadedPackageList( pkgIdx );
			FString OutFilename;
			const FString PackageName = Pkg->GetName();
			const FGuid PackageGuid = Pkg->GetGuid();

			GPackageFileCache->FindPackageFile( *PackageName, &PackageGuid, OutFilename );

			if( OutFilename.Len() > 0 && IsPackageExternal( *Pkg ) )
			{
				ExternalPackages.AddItem(Pkg);
			}
		}

		// get all the objects in the external packages and make sure they aren't referenced by objects in a package being checked
		TArray< UObject* > ObjectsInExternalPackages;
		TArray< UObject* > ObjectsInPackageToCheck;
		GetObjectsInPackages( &ExternalPackages, NULL, ObjectsInExternalPackages );
		GetObjectsInPackages( &PackagesToCheck, NULL, ObjectsInPackageToCheck );
	
		for(INT ExtObjIdx = 0; ExtObjIdx < ObjectsInExternalPackages.Num(); ++ExtObjIdx)
		{
			UObject* ExternalObject = ObjectsInExternalPackages( ExtObjIdx );

			// only check objects which are in packages to be saved.  This should greatly reduce the overhead by not searching through objects we don't intend to save
			for(INT CheckObjIdx = 0; CheckObjIdx < ObjectsInPackageToCheck.Num(); ++CheckObjIdx)
			{
				UObject* CheckObject = ObjectsInPackageToCheck( CheckObjIdx );

				FArchiveFindCulprit ArFind( ExternalObject, CheckObject, FALSE );
				if( ArFind.GetCount() > 0 )
				{
					if( OutPackagesWithExternalRefs )
					{
						OutPackagesWithExternalRefs->AddItem( CheckObject->GetOutermost() );
							}
							bHasExternalPackageRefs = TRUE;
						}
					}
				}

		return bHasExternalPackageRefs;
	}
}


// EOF


