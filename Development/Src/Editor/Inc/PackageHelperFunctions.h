/**
 * File to hold common package helper functions.
 *
 * 	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */

#ifndef _PACKAGE_HELPER_FUNCTIONS_H_
#define _PACKAGE_HELPER_FUNCTIONS_H_


/**
 * Flags which modify the way that NormalizePackageNames works.
 */
enum EPackageNormalizationFlags
{
	/** reset the linker for any packages currently in memory that are part of the output list */
	NORMALIZE_ResetExistingLoaders		= 0x01,
	/** do not include map packages in the result array; only relevant if the input array is empty */
	NORMALIZE_ExcludeMapPackages		= 0x02,
	/** do not include content packages in the result array; only relevant if the input array is empty */
	NORMALIZE_ExcludeContentPackages	= 0x04,
	/** include cooked packages in the result array; only relevant if the input array is empty */
	NORMALIZE_IncludeCookedPackages 	= 0x10,
	
	/** Combo flags */
	NORMALIZE_DefaultFlags				= NORMALIZE_ResetExistingLoaders,
};

static void SearchDirectoryRecursive( const FFilename& SearchPathMask, TArray<FString>& out_PackageNames, TArray<FFilename>& out_PackageFilenames )
{
	const FFilename SearchPath = SearchPathMask.GetPath();
	TArray<FString> PackageNames;
	GFileManager->FindFiles( PackageNames, *SearchPathMask, TRUE, FALSE );
	if ( PackageNames.Num() > 0 )
	{
		for ( INT PkgIndex = 0; PkgIndex < PackageNames.Num(); PkgIndex++ )
		{
			new(out_PackageFilenames) FFilename( SearchPath * PackageNames(PkgIndex) );
		}

		out_PackageNames += PackageNames;
	}

	// now search all subdirectories
	TArray<FString> Subdirectories;
	GFileManager->FindFiles( Subdirectories, *(SearchPath * TEXT("*")), FALSE, TRUE );
	for ( INT DirIndex = 0; DirIndex < Subdirectories.Num(); DirIndex++ )
	{
		SearchDirectoryRecursive( SearchPath * Subdirectories(DirIndex) * SearchPathMask.GetCleanFilename(), out_PackageNames, out_PackageFilenames);
	}
}

/**
 * Takes an array of package names (in any format) and converts them into relative pathnames for each package.
 *
 * @param	PackageNames		the array of package names to normalize.  If this array is empty, the complete package list will be used.
 * @param	PackagePathNames	will be filled with the complete relative path name for each package name in the input array
 * @param	PackageWildcard		if specified, allows the caller to specify a wildcard to use for finding package files
 * @param	PackageFilter		allows the caller to limit the types of packages returned.
 *
 * @return	TRUE if packages were found successfully, FALSE otherwise.
 */
static UBOOL NormalizePackageNames( TArray<FString> PackageNames, TArray<FFilename>& PackagePathNames, const FString& PackageWildcard=FString(TEXT("*.*")), BYTE PackageFilter=NORMALIZE_DefaultFlags )
{
	PackagePathNames.Empty();
	if ( PackageNames.Num() == 0 )
	{
		GFileManager->FindFiles( PackageNames, *PackageWildcard, TRUE, FALSE );
	}

	if( PackageNames.Num() == 0 )
	{
		// if no files were found, it might be an unqualified path; try prepending the .u output path
		// if one were going to make it so that you could use unqualified paths for package types other
		// than ".u", here is where you would do it
		SearchDirectoryRecursive( appScriptOutputDir() * PackageWildcard, PackageNames, PackagePathNames );
		if ( PackageNames.Num() == 0 )
		{
			TArray<FString> Paths;
			if ( GConfig->GetArray( TEXT("Core.System"), TEXT("Paths"), Paths, GEngineIni ) > 0 )
			{
				for ( INT i = 0; i < Paths.Num(); i++ )
				{
					FFilename SearchWildcard = Paths(i) * PackageWildcard;
					SearchDirectoryRecursive( SearchWildcard, PackageNames, PackagePathNames );
				}
			}
		}
		else
		{
			PackagePathNames.Empty(PackageNames.Num());

			// re-add the path information so that GetPackageLinker finds the correct version of the file.
			FFilename WildcardPath = appScriptOutputDir() * PackageWildcard;
			for ( INT FileIndex = 0; FileIndex < PackageNames.Num(); FileIndex++ )
			{
				PackagePathNames(FileIndex) = WildcardPath.GetPath() * PackageNames(FileIndex);
			}
		}

		// Try finding package in package file cache.
		if ( PackageNames.Num() == 0 )
		{
			FString Filename;
			if( GPackageFileCache->FindPackageFile( *PackageWildcard, NULL, Filename ) )
			{
				new(PackagePathNames) FString(Filename);
			}
		}
	}
	else
	{
		PackagePathNames.Empty(PackageNames.Num());

		// re-add the path information so that GetPackageLinker finds the correct version of the file.
		const FString WildcardPath = FFilename(*PackageWildcard).GetPath();
		for ( INT FileIndex = 0; FileIndex < PackageNames.Num(); FileIndex++ )
		{
			PackagePathNames(FileIndex) = WildcardPath * PackageNames(FileIndex);
		}
	}

	if ( PackagePathNames.Num() == 0 )
	{
		warnf(NAME_Error, TEXT("No packages found using '%s'!"), *PackageWildcard);
		return FALSE;
	}

	// now apply any filters to the list of packages
	for ( INT PackageIndex = PackagePathNames.Num() - 1; PackageIndex >= 0; PackageIndex-- )
	{
		FString PackageExtension = PackagePathNames(PackageIndex).GetExtension();
		if ( !GSys->Extensions.ContainsItem(PackageExtension) )
		{
			// not a valid package file - remove it
			PackagePathNames.Remove(PackageIndex);
		}
		else
		{
			if ( (PackageFilter&NORMALIZE_ExcludeMapPackages) != 0 )
			{
				if ( PackageExtension == FURL::DefaultMapExt )
				{
					PackagePathNames.Remove(PackageIndex);
					continue;
				}
			}
			if ( (PackageFilter&NORMALIZE_ExcludeContentPackages) != 0 )
			{
				if ( PackageExtension != FURL::DefaultMapExt )
				{
					PackagePathNames.Remove(PackageIndex);
					continue;
				}
			}
			if ( (PackageFilter&NORMALIZE_IncludeCookedPackages) == 0 )
			{
				if ( PackageExtension == TEXT("xxx") )
				{
					PackagePathNames.Remove(PackageIndex);
					continue;
				}
			}
		}
	}

	if ( (PackageFilter&NORMALIZE_ResetExistingLoaders) != 0 )
	{
		// reset the loaders for the packages we want to load so that we don't find the wrong version of the file
		for ( INT PackageIndex = 0; PackageIndex < PackageNames.Num(); PackageIndex++ )
		{
			// (otherwise, attempting to run a commandlet on e.g. Engine.xxx will always return results for Engine.u instead)
			const FString& PackageName = FPackageFileCache::PackageFromPath(*PackageNames(PackageIndex));
			UPackage* ExistingPackage = FindObject<UPackage>(NULL, *PackageName, TRUE);
			if ( ExistingPackage != NULL )
			{
				UObject::ResetLoaders(ExistingPackage);
			}
		}
	}

	return TRUE;
}


/** 
 * Helper function to save a package that may or may not be a map package
 *
 * @param Package The package to save
 * @param Filename The location to save the package to
 *
 * @return TRUE if successful
 */
static UBOOL SavePackageHelper(UPackage* Package, FString Filename)
{
	// look for a world object in the package (if there is one, there's a map)
	UWorld* World = FindObject<UWorld>(Package, TEXT("TheWorld"));
	UBOOL bSavedCorrectly;
	if (World)
	{	
		bSavedCorrectly = UObject::SavePackage(Package, World, 0, *Filename, GWarn);
	}
	else
	{
		bSavedCorrectly = UObject::SavePackage(Package, NULL, RF_Standalone, *Filename, GWarn);
	}

	// return success
	return bSavedCorrectly;
}



#endif // _PACKAGE_HELPER_FUNCTIONS_H_
