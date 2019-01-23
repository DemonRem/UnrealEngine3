/*=============================================================================
UnPackageUtilities.cpp: Commandlets for viewing information about package files
Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EditorPrivate.h"
#include "EngineMaterialClasses.h"
#include "EngineSequenceClasses.h"
#include "UnPropertyTag.h"
#include "EngineUIPrivateClasses.h"
#include "EnginePhysicsClasses.h"
#include "EngineParticleClasses.h"
#include "LensFlare.h"
#include "EngineAnimClasses.h"

#include "..\..\UnrealEd\Inc\scc.h"
#include "..\..\UnrealEd\Inc\SourceControlIntegration.h"

#include "PackageHelperFunctions.h"

/*-----------------------------------------------------------------------------
ULoadPackageCommandlet
-----------------------------------------------------------------------------*/

/**
* If you pass in -ALL this will recursively load all of the packages from the
* directories listed in the .ini path entries
**/

INT ULoadPackageCommandlet::Main( const FString& Params )
{
	// Parse command line.
	TArray<FString> Tokens;
	TArray<FString> Switches;

	const TCHAR* Parms = *Params;
	ParseCommandLine(Parms, Tokens, Switches);

	const UBOOL bLoadAllPackages = Switches.FindItemIndex(TEXT("ALL")) != INDEX_NONE;
	TArray<FString> FilesInPath;
	if ( bLoadAllPackages )
	{
		FilesInPath = GPackageFileCache->GetPackageFileList();
	}
	else
	{
		for ( INT i = 0; i < Tokens.Num(); i++ )
		{
			FString	PackageWildcard = Tokens(i);	

			GFileManager->FindFiles( FilesInPath, *PackageWildcard, TRUE, FALSE );
			if( FilesInPath.Num() == 0 )
			{
				// if no files were found, it might be an unqualified path; try prepending the .u output path
				// if one were going to make it so that you could use unqualified paths for package types other
				// than ".u", here is where you would do it
				GFileManager->FindFiles( FilesInPath, *(appScriptOutputDir() * PackageWildcard), 1, 0 );

				if ( FilesInPath.Num() == 0 )
				{
					TArray<FString> Paths;
					if ( GConfig->GetArray( TEXT("Core.System"), TEXT("Paths"), Paths, GEngineIni ) > 0 )
					{
						for ( INT j = 0; j < Paths.Num(); j++ )
						{
							GFileManager->FindFiles( FilesInPath, *(Paths(j) * PackageWildcard), 1, 0 );
						}
					}
				}
				else
				{
					// re-add the path information so that GetPackageLinker finds the correct version of the file.
					FFilename WildcardPath = appScriptOutputDir() * PackageWildcard;
					for ( INT FileIndex = 0; FileIndex < FilesInPath.Num(); FileIndex++ )
					{
						FilesInPath(FileIndex) = WildcardPath.GetPath() * FilesInPath(FileIndex);
					}
				}

				// Try finding package in package file cache.
				if ( FilesInPath.Num() == 0 )
				{
					FString Filename;
					if( GPackageFileCache->FindPackageFile( *PackageWildcard, NULL, Filename ) )
					{
						new(FilesInPath)FString(Filename);
					}
				}
			}
			else
			{
				// re-add the path information so that GetPackageLinker finds the correct version of the file.
				FFilename WildcardPath = PackageWildcard;
				for ( INT FileIndex = 0; FileIndex < FilesInPath.Num(); FileIndex++ )
				{
					FilesInPath(FileIndex) = WildcardPath.GetPath() * FilesInPath(FileIndex);
				}
			}
		}
	}

	if( FilesInPath.Num() == 0 )
	{
		warnf(NAME_Warning,TEXT("No packages found matching '%s'"), Parms);
		return 1;
	}

	const UBOOL bSimulateClient = Switches.FindItemIndex(TEXT("NOCLIENT")) == INDEX_NONE;
	const UBOOL bSimulateServer = Switches.FindItemIndex(TEXT("NOSERVER")) == INDEX_NONE;
	const UBOOL bSimulateEditor = Switches.FindItemIndex(TEXT("NOEDITOR")) == INDEX_NONE;
	for( INT FileIndex = 0; FileIndex < FilesInPath.Num(); FileIndex++ )
	{
		GIsClient = bSimulateClient;
		GIsServer = bSimulateServer;
		GIsEditor = bSimulateEditor;

		const FFilename& Filename = FilesInPath(FileIndex);

		// we don't care about trying to load the various shader caches so just skipz0r them
		if(	Filename.InStr( TEXT("LocalShaderCache") ) != INDEX_NONE
			|| Filename.InStr( TEXT("RefShaderCache") ) != INDEX_NONE )
		{
			continue;
		}


		warnf( NAME_Log, TEXT("Loading %s"), *Filename );

		const FString& PackageName = FPackageFileCache::PackageFromPath(*Filename);
		UPackage* Package = FindObject<UPackage>(NULL, *PackageName, TRUE);
		if ( Package != NULL && !bLoadAllPackages )
		{
			ResetLoaders(Package);
		}

		Package = UObject::LoadPackage( NULL, *Filename, LOAD_None );
		if( Package == NULL )
		{
			warnf( NAME_Error, TEXT("Error loading %s!"), *Filename );
		}

		GIsEditor = GIsServer = GIsClient = TRUE;
		SaveLocalShaderCaches();
		UObject::CollectGarbage( RF_Native );
	}

	return 0;
}
IMPLEMENT_CLASS(ULoadPackageCommandlet)


/*-----------------------------------------------------------------------------
UShowObjectCountCommandlet.
-----------------------------------------------------------------------------*/

void UShowObjectCountCommandlet::StaticInitialize()
{
}

struct FPackageObjectCount
{
	INT				Count;
	FString			PackageName;
	FString			ClassName;
	TArray<FString>	ObjectPathNames;

	FPackageObjectCount()
	: Count(0)
	{}

	FPackageObjectCount( const FString& inPackageName, const FString& inClassName, INT InCount=0 )
	: Count(InCount), PackageName(inPackageName), ClassName(inClassName)
	{
	}
};

IMPLEMENT_COMPARE_CONSTREF( FPackageObjectCount, UnPackageUtilities, { INT result = appStricmp(*A.ClassName, *B.ClassName); if ( result == 0 ) { result = B.Count - A.Count; } return result; } )

INT UShowObjectCountCommandlet::Main( const FString& Params )
{
	const TCHAR* Parms = *Params;

	GIsRequestingExit			= 1;	// so CTRL-C will exit immediately
	TArray<FString> Tokens, Switches;
	ParseCommandLine(Parms, Tokens, Switches);

	if ( Tokens.Num() == 0 )
	{
		warnf(TEXT("No class specified!"));
		return 1;
	}

	const UBOOL bIncludeChildren = !Switches.ContainsItem(TEXT("ExactClass"));
	const UBOOL bIgnoreScriptPackages = Switches.ContainsItem(TEXT("IgnoreScript"));
	const UBOOL bIncludeCookedPackages = Switches.ContainsItem(TEXT("IncludeCooked"));
	const UBOOL bCookedPackagesOnly = Switches.ContainsItem(TEXT("CookedOnly"));

	// this flag is useful for skipping over old test packages which can cause the commandlet to crash
	const UBOOL bIgnoreCheckedOutPackages = Switches.ContainsItem(TEXT("IgnoreWriteable"));

	// this flag is useful when you know that the objects you are looking for are not placeable in map packages
	const UBOOL bIgnoreMaps = Switches.ContainsItem(TEXT("IgnoreMaps"));

	const UBOOL bShowObjectNames = Switches.ContainsItem(TEXT("ObjectNames"));
		
	EObjectFlags ObjectMask = RF_LoadForClient|RF_LoadForServer|RF_LoadForEdit;
	if ( ParseParam(appCmdLine(), TEXT("SkipClientOnly")) )
	{
		ObjectMask &= ~RF_LoadForClient;
	}
	if ( ParseParam(appCmdLine(), TEXT("SkipServerOnly")) )
	{
		ObjectMask &= ~RF_LoadForServer;
	}
	if ( ParseParam(appCmdLine(), TEXT("SkipEditorOnly")) )
	{
		ObjectMask &= ~RF_LoadForEdit;
	}

	TArray<UClass*> SearchClasses;
	for ( INT TokenIndex = 0; TokenIndex < Tokens.Num(); TokenIndex++ )
	{
		FString& SearchClassName = Tokens(TokenIndex);
		UClass* SearchClass = LoadClass<UObject>(NULL, *SearchClassName, NULL, 0, NULL);
		if ( SearchClass == NULL )
		{
			warnf(TEXT("Failed to load class specified '%s'"), *SearchClassName);
			return 1;
		}

		SearchClasses.AddUniqueItem(SearchClass);
	}

	TArray<FString> PackageFiles;

	if ( !bCookedPackagesOnly )
	{
		PackageFiles = GPackageFileCache->GetPackageFileList();
	}

	if ( bCookedPackagesOnly || bIncludeCookedPackages )
	{
		const INT StartIndex = PackageFiles.Num();
		const FString CookedPackageDirectory = appGameDir() + TEXT("CookedXenon");
		const FString CookedPackageSearchString = CookedPackageDirectory * TEXT("*.xxx");
		GFileManager->FindFiles(PackageFiles, *CookedPackageSearchString, TRUE, FALSE);

		// re-add the path information so that GetPackageLinker finds the correct version of the file.
		for ( INT FileIndex = StartIndex; FileIndex < PackageFiles.Num(); FileIndex++ )
		{
			PackageFiles(FileIndex) = CookedPackageDirectory * PackageFiles(FileIndex);
		}
	}

	INT GCIndex = 0;
	TArray<FPackageObjectCount> ClassObjectCounts;
	for( INT FileIndex=0; FileIndex<PackageFiles.Num(); FileIndex++ )
	{
		const FString &Filename = PackageFiles(FileIndex);

		if ( bIgnoreCheckedOutPackages && !GFileManager->IsReadOnly(*Filename) )
		{
			continue;
		}

		warnf(NAME_Progress, TEXT("Checking '%s'..."), *Filename);

		UObject::BeginLoad();
		ULinkerLoad* Linker = UObject::GetPackageLinker( NULL, *Filename, LOAD_Quiet|LOAD_NoWarn|LOAD_NoVerify, NULL, NULL );
		UObject::EndLoad();

		if(	Linker != NULL
		&&	(!bIgnoreScriptPackages || (Linker->LinkerRoot != NULL && (Linker->LinkerRoot->PackageFlags&PKG_ContainsScript) == 0))
		&&	(!bIgnoreMaps || (Linker->LinkerRoot != NULL && !Linker->LinkerRoot->ContainsMap())) )
		{
			TArray<INT> ObjectCounts;
			ObjectCounts.AddZeroed(SearchClasses.Num());

			TArray< TArray<FString> > PackageObjectNames;
			if ( bShowObjectNames )
			{
				PackageObjectNames.AddZeroed(SearchClasses.Num());
			}

			UBOOL bContainsObjects=FALSE;
			for ( INT i = 0; i < Linker->ExportMap.Num(); i++ )
			{
				FObjectExport& Export = Linker->ExportMap(i);
				if ( (Export.ObjectFlags&ObjectMask) == 0 )
				{
					continue;
				}

				FString ClassPathName;


				FName ClassFName = NAME_Class;
				PACKAGE_INDEX ClassPackageIndex = 0;

				// get the path name for this Export's class
				if ( IS_IMPORT_INDEX(Export.ClassIndex) )
				{
					FObjectImport& ClassImport = Linker->ImportMap(-Export.ClassIndex -1);
					ClassFName = ClassImport.ObjectName;
					ClassPackageIndex = ClassImport.OuterIndex;
				}
				else if ( Export.ClassIndex != UCLASS_INDEX )
				{
					FObjectExport& ClassExport = Linker->ExportMap(Export.ClassIndex-1);
					ClassFName = ClassExport.ObjectName;
					ClassPackageIndex = ClassExport.OuterIndex;
				}

				FName OuterName = NAME_Core;
				if ( ClassPackageIndex > 0 )
				{
					FObjectExport& OuterExport = Linker->ExportMap(ClassPackageIndex-1);
					OuterName = OuterExport.ObjectName;
				}
				else if ( ClassPackageIndex < 0 )
				{
					FObjectImport& OuterImport = Linker->ImportMap(-ClassPackageIndex-1);
					OuterName = OuterImport.ObjectName;
				}
				else if ( Export.ClassIndex != UCLASS_INDEX )
				{
					OuterName = Linker->LinkerRoot->GetFName();
				}

				ClassPathName = FString::Printf(TEXT("%s.%s"), *OuterName.ToString(), *ClassFName.ToString());
				UClass* ExportClass = FindObject<UClass>(ANY_PACKAGE, *ClassPathName);
				if ( ExportClass == NULL )
				{
					ExportClass = StaticLoadClass(UObject::StaticClass(), NULL, *ClassPathName, NULL, LOAD_NoVerify|LOAD_NoWarn|LOAD_Quiet, NULL);
				}

				if ( ExportClass == NULL )
				{
					continue;
				}

				FString ObjectName;
				for ( INT ClassIndex = 0; ClassIndex < SearchClasses.Num(); ClassIndex++ )
				{
					UClass* SearchClass = SearchClasses(ClassIndex);
					if ( bIncludeChildren ? ExportClass->IsChildOf(SearchClass) : ExportClass == SearchClass )
					{
						bContainsObjects = TRUE;
						INT& CurrentObjectCount = ObjectCounts(ClassIndex);
						CurrentObjectCount++;

						if ( bShowObjectNames )
						{
							TArray<FString>& ClassObjectPaths = PackageObjectNames(ClassIndex);

							if ( ObjectName.Len() == 0 )
							{
								ObjectName = Linker->GetExportFullName(i);
							}
							ClassObjectPaths.AddItem(ObjectName);
						}
					}
				}
			}

			if ( bContainsObjects )
			{
				for ( INT ClassIndex = 0; ClassIndex < ObjectCounts.Num(); ClassIndex++ )
				{
					INT ClassObjectCount = ObjectCounts(ClassIndex);
					if ( ClassObjectCount > 0 )
					{
						FPackageObjectCount* ObjCount = new(ClassObjectCounts) FPackageObjectCount(Filename, SearchClasses(ClassIndex)->GetName(), ClassObjectCount);
						if ( bShowObjectNames )
						{
							ObjCount->ObjectPathNames = PackageObjectNames(ClassIndex);
						}
					}
				}
			}
		}

		// only GC every 10 packages (A LOT faster this way, and is safe, since we are not 
		// acting on objects that would need to go away or anything)
		if ((++GCIndex % 10) == 0)
		{
			UObject::CollectGarbage(RF_Native);
		}
	}

	if( ClassObjectCounts.Num() )
	{
		Sort<USE_COMPARE_CONSTREF(FPackageObjectCount,UnPackageUtilities)>( &ClassObjectCounts(0), ClassObjectCounts.Num() );

		INT TotalObjectCount=0;
		INT PerClassObjectCount=0;

		FString LastReportedClass;
		INT IndexPadding=0;
		for ( INT i = 0; i < ClassObjectCounts.Num(); i++ )
		{
			FPackageObjectCount& PackageObjectCount = ClassObjectCounts(i);
			if ( PackageObjectCount.ClassName != LastReportedClass )
			{
				if ( LastReportedClass.Len() > 0 )
				{
					warnf(TEXT("    Total: %i"), PerClassObjectCount);
				}

				PerClassObjectCount = 0;
				LastReportedClass = PackageObjectCount.ClassName;
				warnf(TEXT("\r\nPackages containing objects of class '%s':"), *LastReportedClass);
				IndexPadding = appItoa(PackageObjectCount.Count).Len();
			}

			warnf(TEXT("%s    Count: %*i    Package: %s"), (i > 0 && bShowObjectNames ? LINE_TERMINATOR : TEXT("")), IndexPadding, PackageObjectCount.Count, *PackageObjectCount.PackageName);
			PerClassObjectCount += PackageObjectCount.Count;
			TotalObjectCount += PackageObjectCount.Count;

			if ( bShowObjectNames )
			{
				warnf(TEXT("        Details:"));
				for ( INT NameIndex = 0; NameIndex < PackageObjectCount.ObjectPathNames.Num(); NameIndex++ )
				{
					warnf(TEXT("        %*i) %s"), IndexPadding, NameIndex, *PackageObjectCount.ObjectPathNames(NameIndex));
				}
			}
		}

		warnf(TEXT("    Total: %i"), PerClassObjectCount);
		warnf(TEXT("\r\nTotal number of object instances: %i"), TotalObjectCount);
	}
	return 0;
}

IMPLEMENT_CLASS(UShowObjectCountCommandlet);

/*-----------------------------------------------------------------------------
UShowTaggedPropsCommandlet.
-----------------------------------------------------------------------------*/

INT UShowTaggedPropsCommandlet::Main(const FString& Params)
{
	const TCHAR* CmdLine = appCmdLine();

	FString	ClassName, PackageFileName, PropertyFilter;
	PackageFileName = ParseToken(CmdLine, FALSE);
	ClassName = ParseToken(CmdLine, FALSE);
	PropertyFilter = ParseToken(CmdLine, FALSE);

	TArray<FString> FilesInPath;
	GFileManager->FindFiles( FilesInPath, *PackageFileName, TRUE, FALSE );
	if( FilesInPath.Num() == 0 )
	{
		// if no files were found, it might be an unqualified path; try prepending the .u output path
		// if one were going to make it so that you could use unqualified paths for package types other
		// than ".u", here is where you would do it
		GFileManager->FindFiles( FilesInPath, *(appScriptOutputDir() * PackageFileName), 1, 0 );

		if ( FilesInPath.Num() == 0 )
		{
			TArray<FString> Paths;
			if ( GConfig->GetArray( TEXT("Core.System"), TEXT("Paths"), Paths, GEngineIni ) > 0 )
			{
				for ( INT i = 0; i < Paths.Num(); i++ )
				{
					GFileManager->FindFiles( FilesInPath, *(Paths(i) * PackageFileName), 1, 0 );
				}
			}
		}
		else
		{
			// re-add the path information so that GetPackageLinker finds the correct version of the file.
			FFilename WildcardPath = appScriptOutputDir() * PackageFileName;
			for ( INT FileIndex = 0; FileIndex < FilesInPath.Num(); FileIndex++ )
			{
				FilesInPath(FileIndex) = WildcardPath.GetPath() * FilesInPath(FileIndex);
			}
		}

		// Try finding package in package file cache.
		if ( FilesInPath.Num() == 0 )
		{
			FString Filename;
			if( GPackageFileCache->FindPackageFile( *PackageFileName, NULL, Filename ) )
			{
				new(FilesInPath) FString(Filename);
			}
		}
	}
	else
	{
		// re-add the path information so that GetPackageLinker finds the correct version of the file.
		FFilename WildcardPath = PackageFileName;
		for ( INT FileIndex = 0; FileIndex < FilesInPath.Num(); FileIndex++ )
		{
			FilesInPath(FileIndex) = WildcardPath.GetPath() * FilesInPath(FileIndex);
		}
	}

	if ( FilesInPath.Num() == 0 )
	{
		warnf(NAME_Error, TEXT("No packages found using '%s'!"), *PackageFileName);
		return 1;
	}

	{
		// reset the loaders for the packages we want to load so that we don't find the wrong version of the file
		// (otherwise, attempting to run pkginfo on e.g. Engine.xxx will always return results for Engine.u instead)
		const FString& PackageName = FPackageFileCache::PackageFromPath(*PackageFileName);
		UPackage* ExistingPackage = FindObject<UPackage>(NULL, *PackageName, TRUE);
		if ( ExistingPackage != NULL )
		{
			ResetLoaders(ExistingPackage);
		}
	}

	warnf(TEXT("Loading '%s'..."), *PackageFileName);
	UObject* Pkg = LoadPackage(NULL, *PackageFileName, LOAD_None);
	UClass* SearchClass = StaticLoadClass(UObject::StaticClass(), NULL, *ClassName, NULL, LOAD_None, NULL);

	if ( SearchClass == NULL && ClassName.Len() > 0 )
	{
		warnf(NAME_Error, TEXT("Failed to load class '%s'"), *ClassName);
		return 1;
	}

	if ( PropertyFilter.Len() > 0 )
	{
		TArray<FString> PropertyNames;
		PropertyFilter.ParseIntoArray(&PropertyNames, TEXT(","), TRUE);

		for ( INT PropertyIndex = 0; PropertyIndex < PropertyNames.Num(); PropertyIndex++ )
		{
			UProperty* Property = FindFieldWithFlag<UProperty,CLASS_IsAUProperty>(SearchClass, FName(*PropertyNames(PropertyIndex)));
			if ( Property != NULL )
			{
				SearchProperties.Set(Property,Property);
			}
		}
	}

	// this is needed in case we end up serializing a script reference which results in VerifyImport being called
	BeginLoad();
	for ( FObjectIterator It; It; ++It )
	{
		UObject* Obj = *It;
		if ( Obj->IsA(SearchClass) && Obj->IsIn(Pkg) )
		{
			ShowSavedProperties(Obj);
		}
	}
	EndLoad();

	return 0;
}

void UShowTaggedPropsCommandlet::ShowSavedProperties( UObject* Object ) const
{
	check(Object);

	ULinkerLoad& Ar = *Object->GetLinker();
	INT LinkerIndex = Object->GetLinkerIndex();
	check(LinkerIndex != INDEX_NONE);

	const UBOOL bIsArchetypeObject = Object->IsTemplate();
	if ( bIsArchetypeObject == TRUE )
	{
		Ar.StartSerializingDefaults();
	}

	FName PropertyName(NAME_None);
	FObjectExport& Export = Ar.ExportMap(LinkerIndex);
	Ar.Loader->Seek(Export.SerialOffset);
	Ar.Loader->Precache(Export.SerialOffset,Export.SerialSize);

	if( Object->HasAnyFlags(RF_HasStack) )
	{
		FStateFrame* DummyStateFrame = new FStateFrame(Object);

		Ar << DummyStateFrame->Node << DummyStateFrame->StateNode;
		Ar << DummyStateFrame->ProbeMask;
		Ar << DummyStateFrame->LatentAction;
		Ar << DummyStateFrame->StateStack;
		if( DummyStateFrame->Node )
		{
			Ar.Preload( DummyStateFrame->Node );
			INT Offset = DummyStateFrame->Code ? DummyStateFrame->Code - &DummyStateFrame->Node->Script(0) : INDEX_NONE;
			Ar << Offset;
			if( Offset!=INDEX_NONE )
			{
				if( Offset<0 || Offset>=DummyStateFrame->Node->Script.Num() )
				{
					appErrorf( TEXT("%s: Offset mismatch: %i %i"), *GetFullName(), Offset, DummyStateFrame->Node->Script.Num() );
				}
			}
			DummyStateFrame->Code = Offset!=INDEX_NONE ? &DummyStateFrame->Node->Script(Offset) : NULL;
		}
		else 
		{
			DummyStateFrame->Code = NULL;
		}

		delete DummyStateFrame;
	}

	if ( Object->IsA(UComponent::StaticClass()) && !Object->HasAnyFlags(RF_ClassDefaultObject) )
		((UComponent*)Object)->PreSerialize(Ar);

	Object->SerializeNetIndex(Ar);

	BYTE* Data = (BYTE*)appAlloca(256*256);
	appMemzero(Data, 256*256);

	// Load tagged properties.
	UClass* ObjClass = Object->GetClass();

	// This code assumes that properties are loaded in the same order they are saved in. This removes a n^2 search 
	// and makes it an O(n) when properties are saved in the same order as they are loaded (default case). In the 
	// case that a property was reordered the code falls back to a slower search.
	UProperty*	Property			= ObjClass->PropertyLink;
	UBOOL		AdvanceProperty		= 0;
	INT			RemainingArrayDim	= Property ? Property->ArrayDim : 0;

	UBOOL bDisplayedObjectName = FALSE;

	// Load all stored properties, potentially skipping unknown ones.
	while( 1 )
	{
		FPropertyTag Tag;
		Ar << Tag;
		if( Tag.Name == NAME_None )
			break;
		PropertyName = Tag.Name;

		// Move to the next property to be serialized
		if( AdvanceProperty && --RemainingArrayDim <= 0 )
		{
			check(Property);
			Property = Property->PropertyLinkNext;
			// Skip over properties that don't need to be serialized.
			while( Property && !Property->ShouldSerializeValue( Ar ) )
			{
				Property = Property->PropertyLinkNext;
			}
			AdvanceProperty		= 0;
			RemainingArrayDim	= Property ? Property->ArrayDim : 0;
		}

		// If this property is not the one we expect (e.g. skipped as it matches the default value), do the brute force search.
		if( Property == NULL || Property->GetFName() != Tag.Name )
		{
			UProperty* CurrentProperty = Property;
			// Search forward...
			for ( ; Property; Property=Property->PropertyLinkNext )
			{
				if( Property->GetFName() == Tag.Name )
				{
					break;
				}
			}
			// ... and then search from the beginning till we reach the current property if it's not found.
			if( Property == NULL )
			{
				for( Property = ObjClass->PropertyLink; Property && Property != CurrentProperty; Property = Property->PropertyLinkNext )
				{
					if( Property->GetFName() == Tag.Name )
					{
						break;
					}
				}

				if( Property == CurrentProperty )
				{
					// Property wasn't found.
					Property = NULL;
				}
			}

			RemainingArrayDim = Property ? Property->ArrayDim : 0;
		}

		const UBOOL bShowPropertyValue = SearchProperties.Num() == 0
			|| (Property != NULL && SearchProperties.Find(Property) != NULL);

		if ( bShowPropertyValue && !bDisplayedObjectName )
		{
			bDisplayedObjectName = TRUE;
			warnf(TEXT("%s:"), *Object->GetPathName());
		}


		//@{
		//@compatibility
		// are we converting old content from UDistributionXXX properties to FRawDistributionYYY structs?
		UBOOL bDistributionHack = FALSE;
		// if we were looking for an object property, but we found a matching struct property,
		// and the struct is of type FRawDistributionYYY, then we need to do some mojo
		if (Ar.Ver() < VER_FDISTRIBUTIONS && Tag.Type == NAME_ObjectProperty && Property && Cast<UStructProperty>(Property,CLASS_IsAUStructProperty) != NULL )
		{
			FName StructName = ((UStructProperty*)Property)->Struct->GetFName();
			if (StructName == NAME_RawDistributionFloat || StructName == NAME_RawDistributionVector)
			{
				bDistributionHack = TRUE;
			}
		}
		//@}

		if( !Property )
		{
			//@{
			//@compatibility
			if (Ar.IsLoading() && Tag.Name == NAME_InitChild2StartBone)
			{
				UProperty* NewProperty = FindField<UProperty>(ObjClass, TEXT("BranchStartBoneName"));
				if (NewProperty != NULL && NewProperty->IsA(UArrayProperty::StaticClass()) && ((UArrayProperty*)NewProperty)->Inner->IsA(UNameProperty::StaticClass()))
				{
					// we don't have access to ULinkerLoad::operator<<(FName&), so we have to reproduce that code here
					FName OldName;

					// serialize the name index
					INT NameIndex;
					Ar << NameIndex;

					if ( !Ar.NameMap.IsValidIndex(NameIndex) )
					{
						appErrorf( TEXT("Bad name index %i/%i"), NameIndex, Ar.NameMap.Num() );
					}

					if ( Ar.NameMap(NameIndex) == NAME_None )
					{
						if (Ar.Ver() >= VER_FNAME_CHANGE_NAME_SPLIT)
						{
							INT TempNumber;
							Ar << TempNumber;
						}
						OldName = NAME_None;
					}
					else
					{
						if ( Ar.Ver() < VER_FNAME_CHANGE_NAME_SPLIT )
						{
							OldName = Ar.NameMap(NameIndex);
						}
						else if ( Ar.Ver() < VER_NAME_TABLE_LOADING_CHANGE )
						{
							INT Number;
							Ar << Number;

							// if there was a number and the namemap got a number from being split by the namemap serialization
							// then that means we had a name like A_6_2 that was split before to A_6, 2 and now it was split again
							// so we need to reconstruct the full thing
							if (Number && Ar.NameMap(NameIndex).GetNumber())
							{
								OldName = FName(*FString::Printf(TEXT("%s_%d"), Ar.NameMap(NameIndex).GetName(), NAME_INTERNAL_TO_EXTERNAL(Ar.NameMap(NameIndex).GetNumber())), Number);
							}
							else
							{
								// otherwise, we can just add them (since at least 1 is zero) to get the actual number
								OldName = FName((EName)Ar.NameMap(NameIndex).GetIndex(), Number + Ar.NameMap(NameIndex).GetNumber());
							}
						}
						else
						{
							INT Number;
							Ar << Number;
							// simply create the name from the NameMap's name index and the serialized instance number
							OldName = FName((EName)Ar.NameMap(NameIndex).GetIndex(), Number);
						}
					}

					((TArray<FName>*)(Data + NewProperty->Offset))->AddItem(OldName);
					AdvanceProperty = FALSE;
					continue;
				}
			}
			//@}

			debugfSlow( NAME_Warning, TEXT("Property %s of %s not found for package:  %s"), *Tag.Name.ToString(), *ObjClass->GetFullName(), *Ar.GetArchiveName().ToString() );
		}
		else if( Tag.Type==NAME_StrProperty && Property->GetID()==NAME_NameProperty )  
		{ 
			FString str;  
			Ar << str; 
			*(FName*)(Data + Property->Offset + Tag.ArrayIndex * Property->ElementSize ) = FName(*str);  
			AdvanceProperty = TRUE;

			if ( bShowPropertyValue )
			{
				FString PropertyValue;
				Property->ExportText(Tag.ArrayIndex, PropertyValue, Data, Data, NULL, PPF_Localized);

				FString PropertyNameText = *Property->GetName();
				if ( Property->ArrayDim != 1 )
					PropertyNameText += FString::Printf(TEXT("[%s]"), *appItoa(Tag.ArrayIndex));

				warnf(TEXT("\t%s%s"), *PropertyNameText.RightPad(32), *PropertyValue);
			}
			continue; 
		}
		else if ( Tag.Type == NAME_ByteProperty && Property->GetID() == NAME_IntProperty )
		{
			// this property's data was saved as a BYTE, but the property has been changed to an INT.  Since there is no loss of data
			// possible, we can auto-convert to the right type.
			BYTE PreviousValue;

			// de-serialize the previous value
			Ar << PreviousValue;

			// now copy the value into the object's address spaace
			*(INT*)(Data + Property->Offset + Tag.ArrayIndex * Property->ElementSize) = PreviousValue;
			AdvanceProperty = TRUE;
			continue;
		}
		else if( !bDistributionHack && Tag.Type!=Property->GetID() )
		{
			debugf( NAME_Warning, TEXT("Type mismatch in %s of %s - Previous (%s) Current(%s) for package:  %s"), *Tag.Name.ToString(), *GetName(), *Tag.Type.ToString(), *Property->GetID().ToString(), *Ar.GetArchiveName().ToString() );
		}
		else if( Tag.ArrayIndex>=Property->ArrayDim )
		{
			debugf( NAME_Warning, TEXT("Array bounds in %s of %s: %i/%i for package:  %s"), *Tag.Name.ToString(), *GetName(), Tag.ArrayIndex, Property->ArrayDim, *Ar.GetArchiveName().ToString() );
		}
		else if( !bDistributionHack && Tag.Type==NAME_StructProperty && Tag.ItemName!=CastChecked<UStructProperty>(Property)->Struct->GetFName() )
		{
			debugf( NAME_Warning, TEXT("Property %s of %s struct type mismatch %s/%s for package:  %s"), *Tag.Name.ToString(), *GetName(), *Tag.ItemName.ToString(), *CastChecked<UStructProperty>(Property)->Struct->GetName(), *Ar.GetArchiveName().ToString() );
		}
		else if( !Property->ShouldSerializeValue(Ar) )
		{
			if ( bShowPropertyValue )
			{
				//@{
				//@compatibility
				// we need to load any existing values for the SourceStyle property of a StyleDataReference struct so that we can save the referenced style's STYLE_ID into
				// the SourceStyleID property of this struct
				if ( Ar.Ver() < VER_ADDED_SOURCESTYLEID
					&& Ar.IsLoading()
					&& GetFName() == NAME_StyleDataReference
					&& Property->GetFName() == NAME_SourceStyle
					)
				{
					// goto may be evil in OOP land, but it's the simplest way to achieve the desired behavior with minimal impact on load times
					goto LoadPropertyValue;
				}
				//@}
				debugf( NAME_Warning, TEXT("Property %s of %s is not serializable for package:  %s"), *Tag.Name.ToString(), *GetName(), *Ar.GetArchiveName().ToString() );
			}
		}
		else if ( bShowPropertyValue )
		{
LoadPropertyValue:
			// This property is ok.
			BYTE* DestAddress = Data + Property->Offset + Tag.ArrayIndex*Property->ElementSize;

			//@{
			//@compatibility
			// if we are doing the distribution fixup hack, serialize into the FRawDistribtion's Distribution variable
			if (bDistributionHack)
			{
				UScriptStruct* RawDistributionStruct = CastChecked<UStructProperty>(Property)->Struct;

				// find the actual UDistributionXXX property inside the struct, 
				// and use that for serializing
				Property = FindField<UObjectProperty>(RawDistributionStruct, TEXT("Distribution"));

				// offset the DestAddress by the amount inside the struct this property is
				DestAddress += Property->Offset;
			}
			//@}

			Tag.SerializeTaggedProperty( Ar, Property, DestAddress, Tag.Size, NULL );

			//@{
			//@compatibility
			if ( Object->GetClass()->GetFName() == NAME_StyleDataReference 
				&& Ar.Ver() < VER_CHANGED_UISTATES
				&& Tag.Name == NAME_SourceState )
			{
				*(UObject**)DestAddress = (*(UClass**)DestAddress)->GetDefaultObject();
			}
			AdvanceProperty = TRUE;

			FString PropertyValue;
			Property->ExportText(Tag.ArrayIndex, PropertyValue, Data, Data, NULL, PPF_Localized);

			FString PropertyNameText = *Property->GetName();
			if ( Property->ArrayDim != 1 )
				PropertyNameText += FString::Printf(TEXT("[%s]"), *appItoa(Tag.ArrayIndex));

			warnf(TEXT("\t%s%s"), *PropertyNameText.RightPad(32), *PropertyValue);
			continue;
		}

		if ( !bShowPropertyValue )
		{
			// if we're not supposed to show the value for this property, just skip it without logging a warning
			AdvanceProperty = TRUE;
			BYTE B;
			for( INT i=0; i<Tag.Size; i++ )
			{
				Ar << B;
			}

			continue;
		}

		AdvanceProperty = FALSE;

		// Skip unknown or bad property.
		debugfSlow( NAME_Warning, TEXT("Skipping %i bytes of type %s for package:  %s"), Tag.Size, *Tag.Type.ToString(), *Ar.GetArchiveName().ToString() );

		BYTE B;
		for( INT i=0; i<Tag.Size; i++ )
		{
			Ar << B;
		}
	}

	if ( bDisplayedObjectName )
		warnf(TEXT(""));

	if ( bIsArchetypeObject == TRUE )
	{
		Ar.StopSerializingDefaults();
	}
}

IMPLEMENT_CLASS(UShowTaggedPropsCommandlet)

/*-----------------------------------------------------------------------------
UListPackagesReferencing commandlet.
-----------------------------------------------------------------------------*/

/**
* Contains the linker name and filename for a package which is referencing another package.
*/
struct FReferencingPackageName
{
	/** the name of the linker root (package name) */
	FName LinkerFName;

	/** the complete filename for the package */
	FString Filename;

	/** Constructor */
	FReferencingPackageName( FName InLinkerFName, const FString& InFilename )
		: LinkerFName(InLinkerFName), Filename(InFilename)
	{
	}

	/** Comparison operator */
	inline UBOOL operator==( const FReferencingPackageName& Other )
	{
		return LinkerFName == Other.LinkerFName;
	}
};

inline DWORD GetTypeHash( const FReferencingPackageName& ReferencingPackageStruct )
{
	return GetTypeHash(ReferencingPackageStruct.LinkerFName);
}

IMPLEMENT_COMPARE_CONSTREF(FReferencingPackageName,UnPackageUtilities,{ return appStricmp(*A.LinkerFName.ToString(),*B.LinkerFName.ToString()); });

INT UListPackagesReferencingCommandlet::Main( const FString& Params )
{
	const TCHAR* Parms = *Params;

	TLookupMap<FReferencingPackageName>	ReferencingPackages;
	TArray<FString> PackageFiles = GPackageFileCache->GetPackageFileList();


	//@todo ronp - add support for searching for references to multiple packages/resources at once.

	FString SearchName;
	if( ParseToken(Parms, SearchName, 0) )
	{

		// determine whether we're searching references to a package or a specific resource
		INT delimPos = SearchName.InStr(TEXT("."), TRUE);

		// if there's no dots in the search name, or the last part of the name is one of the registered package extensions, we're searching for a package
		const UBOOL bIsPackage = delimPos == INDEX_NONE || GSys->Extensions.FindItemIndex(SearchName.Mid(delimPos+1)) != INDEX_NONE;

		FName SearchPackageFName=NAME_None;
		if ( bIsPackage == TRUE )
		{
			// remove any extensions on the package name
			SearchPackageFName = FName(*FFilename(SearchName).GetBaseFilename());
		}
		else
		{
			// validate that this resource exists
			UObject* SearchObject = StaticLoadObject(UObject::StaticClass(), NULL, *SearchName, NULL, LOAD_NoWarn, NULL);
			if ( SearchObject == NULL )
			{
				warnf(TEXT("Unable to load specified resource: %s"), *SearchName);
				return 1;
			}

			// searching for a particular resource - pull off the package name
			SearchPackageFName = SearchObject->GetOutermost()->GetFName();

			// then change the SearchName to the object's actual path name, in case the name passed on the command-line wasn't a complete path name
			SearchName = SearchObject->GetPathName();
		}

		INT GCIndex = 0;
		for( INT FileIndex=0; FileIndex<PackageFiles.Num(); FileIndex++ )
		{
			const FString &Filename = PackageFiles(FileIndex);

			warnf(NAME_Progress, TEXT("Loading '%s'..."), *Filename);

			UObject::BeginLoad();
			ULinkerLoad* Linker = UObject::GetPackageLinker( NULL, *Filename, LOAD_Quiet|LOAD_NoWarn, NULL, NULL );
			UObject::EndLoad();

			if( Linker )
			{
				FName LinkerFName = Linker->LinkerRoot->GetFName();

				// ignore the package if it's the one we're processing
				if( LinkerFName != SearchPackageFName )
				{
					// look for the search package in this package's ImportMap.
					for( INT ImportIndex=0; ImportIndex<Linker->ImportMap.Num(); ImportIndex++ )
					{
						FObjectImport& Import = Linker->ImportMap( ImportIndex );
						UBOOL bImportReferencesSearchPackage = FALSE;

						if ( bIsPackage == TRUE )
						{
							if ( Import.ClassPackage == SearchPackageFName )
							{
								// this import's class is contained in the package we're searching for references to
								bImportReferencesSearchPackage = TRUE;
							}
							else if ( Import.ObjectName == SearchPackageFName && Import.ClassName == NAME_Package && Import.ClassPackage == NAME_Core )
							{
								// this import is the package we're searching for references to
								bImportReferencesSearchPackage = TRUE;
							}
							else if ( Import.OuterIndex != ROOTPACKAGE_INDEX )
							{
								// otherwise, determine if this import's source package is the package we're searching for references to
								// Import.SourceLinker is cleared in UObject::EndLoad, so we can't use that
								PACKAGE_INDEX OutermostLinkerIndex = Import.OuterIndex;
								for ( PACKAGE_INDEX LinkerIndex = Import.OuterIndex; LinkerIndex != ROOTPACKAGE_INDEX; )
								{
									OutermostLinkerIndex = LinkerIndex;

									// this import's outer might be in the export table if the package was saved for seek-free loading
									if ( IS_IMPORT_INDEX(LinkerIndex) )
									{
										LinkerIndex = Linker->ImportMap( -LinkerIndex - 1 ).OuterIndex;
									}
									else
									{
										LinkerIndex = Linker->ExportMap( LinkerIndex - 1 ).OuterIndex;
									}
								}

								// if the OutermostLinkerIndex is ROOTPACKAGE_INDEX, this import corresponds to the root package for this linker
								if ( IS_IMPORT_INDEX(OutermostLinkerIndex) )
								{
									FObjectImport& PackageImport = Linker->ImportMap( -OutermostLinkerIndex - 1 );
									bImportReferencesSearchPackage =	PackageImport.ObjectName	== SearchPackageFName &&
										PackageImport.ClassName		== NAME_Package &&
										PackageImport.ClassPackage	== NAME_Core;
								}
								else
								{
									check(OutermostLinkerIndex != ROOTPACKAGE_INDEX);

									FObjectExport& PackageExport = Linker->ExportMap( OutermostLinkerIndex - 1 );
									bImportReferencesSearchPackage =	PackageExport.ObjectName == SearchPackageFName;
								}
							}
						}
						else
						{
							FString ImportPathName = Linker->GetImportPathName(ImportIndex);
							if ( SearchName == ImportPathName )
							{
								// this is the object we're search for
								bImportReferencesSearchPackage = TRUE;
							}
							else
							{
								// see if this import's class is the resource we're searching for
								FString ImportClassPathName = Import.ClassPackage.ToString() + TEXT(".") + Import.ClassName.ToString();
								if ( ImportClassPathName == SearchName )
								{
									bImportReferencesSearchPackage = TRUE;
								}
								else if ( Import.OuterIndex > ROOTPACKAGE_INDEX )
								{
									// and OuterIndex > 0 indicates that the import's Outer is in the package's export map, which would happen
									// if the package was saved for seek-free loading;
									// we need to check the Outer in this case since we are only iterating through the ImportMap
									FString OuterPathName = Linker->GetExportPathName(Import.OuterIndex - 1);
									if ( SearchName == OuterPathName )
									{
										bImportReferencesSearchPackage = TRUE;
									}
								}
							}
						}

						if ( bImportReferencesSearchPackage )
						{
							ReferencingPackages.AddItem( FReferencingPackageName(LinkerFName, Filename) );
							break;
						}
					}
				}
			}

			// only GC every 10 packages (A LOT faster this way, and is safe, since we are not 
			// acting on objects that would need to go away or anything)
			if ((++GCIndex % 10) == 0)
			{
				UObject::CollectGarbage(RF_Native);
			}
		}

		warnf( TEXT("%i packages reference %s:"), ReferencingPackages.Num(), *SearchName );

		// calculate the amount of padding to use when listing the referencing packages
		INT Padding=appStrlen(TEXT("Package Name"));
		for( INT ReferencerIndex=0; ReferencerIndex<ReferencingPackages.Num(); ReferencerIndex++ )
		{
			Padding = Max(Padding, ReferencingPackages(ReferencerIndex).LinkerFName.ToString().Len());
		}

		warnf( TEXT("  %*s  Filename"), Padding, TEXT("Package Name"));

		// KeySort shouldn't be used with TLookupMap because then the Value for each pair in the Pairs array (which is the index into the Pairs array for that pair)
		// is no longer correct.  That doesn't matter to use because we don't use the value for anything, so sort away!
		ReferencingPackages.KeySort<COMPARE_CONSTREF_CLASS(FReferencingPackageName,UnPackageUtilities)>();

		// output the list of referencers
		for( INT ReferencerIndex=0; ReferencerIndex<ReferencingPackages.Num(); ReferencerIndex++ )
		{
			warnf( TEXT("  %*s  %s"), Padding, *ReferencingPackages(ReferencerIndex).LinkerFName.ToString(), *ReferencingPackages(ReferencerIndex).Filename );
		}
	}

	return 0;
}
IMPLEMENT_CLASS(UListPackagesReferencingCommandlet)

/*-----------------------------------------------------------------------------
UPkgInfo commandlet.
-----------------------------------------------------------------------------*/

struct FExportInfo
{
	FName	Name;
	INT		Size;
};

IMPLEMENT_COMPARE_CONSTREF( FExportInfo, UnPackageUtilities, { return B.Size - A.Size; } )

enum EPackageInfoFlags
{
	PKGINFO_None		=0x00,
	PKGINFO_Names		=0x01,
	PKGINFO_Imports		=0x02,
	PKGINFO_Exports		=0x04,
	PKGINFO_Compact		=0x08,
	PKGINFO_Chunks		=0x10,
	PKGINFO_Depends		=0x20,

	PKGINFO_All			= PKGINFO_Names|PKGINFO_Imports|PKGINFO_Exports|PKGINFO_Chunks|PKGINFO_Depends,
};

INT UPkgInfoCommandlet::Main( const FString& Params )
{
	const TCHAR* Parms = *Params;

	TArray<FString> Tokens, Switches;
	ParseCommandLine(Parms, Tokens, Switches);

	// find out which type of info we're looking for
	DWORD InfoFlags = PKGINFO_None;
	if ( Switches.ContainsItem(TEXT("names")) )
	{
		InfoFlags |= PKGINFO_Names;
	}
	if ( Switches.ContainsItem(TEXT("imports")) )
	{
		InfoFlags |= PKGINFO_Imports;
	}
	if ( Switches.ContainsItem(TEXT("exports")) )
	{
		InfoFlags |= PKGINFO_Exports;
	}
	if ( Switches.ContainsItem(TEXT("simple")) )
	{
		InfoFlags |= PKGINFO_Compact;
	}
	if ( Switches.ContainsItem(TEXT("chunks")) )
	{
		InfoFlags |= PKGINFO_Chunks;
	}
	if ( Switches.ContainsItem(TEXT("depends")) )
	{
		InfoFlags |= PKGINFO_Depends;
	}
	if ( Switches.ContainsItem(TEXT("all")) )
	{
		InfoFlags |= PKGINFO_All;
	}

	const UBOOL bHideOffsets = Switches.ContainsItem(TEXT("HideOffsets"));

	for ( INT TokenIndex = 0; TokenIndex < Tokens.Num(); TokenIndex++ )
	{
		FString& PackageWildcard = Tokens(TokenIndex);

		TArray<FString> FilesInPath;
		GFileManager->FindFiles( FilesInPath, *PackageWildcard, TRUE, FALSE );
		if( FilesInPath.Num() == 0 )
		{
			// if no files were found, it might be an unqualified path; try prepending the .u output path
			// if one were going to make it so that you could use unqualified paths for package types other
			// than ".u", here is where you would do it
			GFileManager->FindFiles( FilesInPath, *(appScriptOutputDir() * PackageWildcard), 1, 0 );

			if ( FilesInPath.Num() == 0 )
			{
				TArray<FString> Paths;
				if ( GConfig->GetArray( TEXT("Core.System"), TEXT("Paths"), Paths, GEngineIni ) > 0 )
				{
					for ( INT i = 0; i < Paths.Num(); i++ )
					{
						GFileManager->FindFiles( FilesInPath, *(Paths(i) * PackageWildcard), 1, 0 );
					}
				}
			}
			else
			{
				// re-add the path information so that GetPackageLinker finds the correct version of the file.
				FFilename WildcardPath = appScriptOutputDir() * PackageWildcard;
				for ( INT FileIndex = 0; FileIndex < FilesInPath.Num(); FileIndex++ )
				{
					FilesInPath(FileIndex) = WildcardPath.GetPath() * FilesInPath(FileIndex);
				}
			}

			// Try finding package in package file cache.
			if ( FilesInPath.Num() == 0 )
			{
				FString Filename;
				if( GPackageFileCache->FindPackageFile( *PackageWildcard, NULL, Filename ) )
				{
					new(FilesInPath)FString(Filename);
				}
			}
		}
		else
		{
			// re-add the path information so that GetPackageLinker finds the correct version of the file.
			FFilename WildcardPath = PackageWildcard;
			for ( INT FileIndex = 0; FileIndex < FilesInPath.Num(); FileIndex++ )
			{
				FilesInPath(FileIndex) = WildcardPath.GetPath() * FilesInPath(FileIndex);
			}
		}

		if ( FilesInPath.Num() == 0 )
		{
			warnf(TEXT("No packages found using '%s'!"), *PackageWildcard);
			continue;
		}

		for( INT FileIndex = 0; FileIndex < FilesInPath.Num(); FileIndex++ )
		{
			const FString &Filename = FilesInPath(FileIndex);

			{
				// reset the loaders for the packages we want to load so that we don't find the wrong version of the file
				// (otherwise, attempting to run pkginfo on e.g. Engine.xxx will always return results for Engine.u instead)
				const FString& PackageName = FPackageFileCache::PackageFromPath(*Filename);
				UPackage* ExistingPackage = FindObject<UPackage>(NULL, *PackageName, TRUE);
				if ( ExistingPackage != NULL )
				{
					ResetLoaders(ExistingPackage);
				}
			}

			UObject::BeginLoad();
			ULinkerLoad* Linker = UObject::GetPackageLinker( NULL, *Filename, LOAD_NoVerify, NULL, NULL );
			UObject::EndLoad();

			if( Linker )
			{
				if ( FileIndex > 0 )
				{
					warnf(TEXT(""));
				}

				// Display information about the package.
				FName LinkerName = Linker->LinkerRoot->GetFName();

				// Display summary info.
				GWarn->Log( TEXT("********************************************") );
				GWarn->Logf( TEXT("Package '%s' Summary"), *LinkerName.ToString() );
				GWarn->Log( TEXT("--------------------------------------------") );

				GWarn->Logf( TEXT("\t         Filename: %s"), *Filename);
				GWarn->Logf( TEXT("\t     File Version: %i"), Linker->Ver() );
				GWarn->Logf( TEXT("\t   Engine Version: %d"), Linker->Summary.EngineVersion);
				GWarn->Logf( TEXT("\t   Cooker Version: %d"), Linker->Summary.CookedContentVersion);
				GWarn->Logf( TEXT("\t     PackageFlags: %X"), Linker->Summary.PackageFlags );
				GWarn->Logf( TEXT("\t        NameCount: %d"), Linker->Summary.NameCount );
				GWarn->Logf( TEXT("\t       NameOffset: %d"), Linker->Summary.NameOffset );
				GWarn->Logf( TEXT("\t      ImportCount: %d"), Linker->Summary.ImportCount );
				GWarn->Logf( TEXT("\t     ImportOffset: %d"), Linker->Summary.ImportOffset );
				GWarn->Logf( TEXT("\t      ExportCount: %d"), Linker->Summary.ExportCount );
				GWarn->Logf( TEXT("\t     ExportOffset: %d"), Linker->Summary.ExportOffset );
				GWarn->Logf( TEXT("\tCompression Flags: %X"), Linker->Summary.CompressionFlags);

				FString szGUID = Linker->Summary.Guid.String();
				GWarn->Logf( TEXT("\t             Guid: %s"), *szGUID );
				GWarn->Log ( TEXT("\t      Generations:"));
				for( INT i = 0; i < Linker->Summary.Generations.Num(); ++i )
				{
					const FGenerationInfo& generationInfo = Linker->Summary.Generations( i );
					GWarn->Logf(TEXT("\t\t\t%d) ExportCount=%d, NameCount=%d, NetObjectCount=%d"), i, generationInfo.ExportCount, generationInfo.NameCount, generationInfo.NetObjectCount);
				}


				if( (InfoFlags&PKGINFO_Chunks) != 0 )
				{
					GWarn->Log( TEXT("--------------------------------------------") );
					GWarn->Log ( TEXT("Compression Chunks"));
					GWarn->Log ( TEXT("=========="));

					for ( INT ChunkIndex = 0; ChunkIndex < Linker->Summary.CompressedChunks.Num(); ChunkIndex++ )
					{
						FCompressedChunk& Chunk = Linker->Summary.CompressedChunks(ChunkIndex);
						GWarn->Log ( TEXT("\t*************************"));
						GWarn->Logf( TEXT("\tChunk %d:"), ChunkIndex );
						GWarn->Logf( TEXT("\t\tUncompressedOffset: %d"), Chunk.UncompressedOffset);
						GWarn->Logf( TEXT("\t\t  UncompressedSize: %d"), Chunk.UncompressedSize);
						GWarn->Logf( TEXT("\t\t  CompressedOffset: %d"), Chunk.CompressedOffset);
						GWarn->Logf( TEXT("\t\t    CompressedSize: %d"), Chunk.CompressedSize);
					}
				}

				if( (InfoFlags&PKGINFO_Names) != 0 )
				{
					GWarn->Log( TEXT("--------------------------------------------") );
					GWarn->Log ( TEXT("Name Map"));
					GWarn->Log ( TEXT("========"));
					for( INT i = 0; i < Linker->NameMap.Num(); ++i )
					{
						FName& name = Linker->NameMap( i );
						GWarn->Logf( TEXT("\t%d: Name '%s' Index %d [Internal: %s, %d]"), i, *name.ToString(), name.GetIndex(), name.GetName(), name.GetNumber() );
					}
				}

				// if we _only_ want name info, skip this part completely
				if ( InfoFlags != PKGINFO_Names )
				{
					if( (InfoFlags&PKGINFO_Imports) != 0 )
					{
						GWarn->Log( TEXT("--------------------------------------------") );
						GWarn->Log ( TEXT("Import Map"));
						GWarn->Log ( TEXT("=========="));
					}

					TArray<FName> DependentPackages;
					for( INT i = 0; i < Linker->ImportMap.Num(); ++i )
					{
						FObjectImport& import = Linker->ImportMap( i );

						FName PackageName = NAME_None;
						FName OuterName = NAME_None;
						if ( import.OuterIndex != ROOTPACKAGE_INDEX )
						{
							if ( IS_IMPORT_INDEX(import.OuterIndex) )
							{
								FObjectImport& OuterImport = Linker->ImportMap(-import.OuterIndex-1);
								OuterName = OuterImport.ObjectName;
							}
							else if ( import.OuterIndex < 0 )
							{
								FObjectExport& OuterExport = Linker->ExportMap(import.OuterIndex-1);
								OuterName = OuterExport.ObjectName;
							}

							// Find the package which contains this import.  import.SourceLinker is cleared in UObject::EndLoad, so we'll need to do this manually now.
							PACKAGE_INDEX OutermostLinkerIndex = import.OuterIndex;
							for ( PACKAGE_INDEX LinkerIndex = import.OuterIndex; LinkerIndex != ROOTPACKAGE_INDEX; )
							{
								OutermostLinkerIndex = LinkerIndex;

								// this import's outer might be in the export table if the package was saved for seek-free loading
								if ( IS_IMPORT_INDEX(LinkerIndex) )
								{
									LinkerIndex = Linker->ImportMap( -LinkerIndex - 1 ).OuterIndex;
								}
								else
								{
									LinkerIndex = Linker->ExportMap( LinkerIndex - 1 ).OuterIndex;
								}
							}

							// if the OutermostLinkerIndex is ROOTPACKAGE_INDEX, this import corresponds to the root package for this linker
							if ( IS_IMPORT_INDEX(OutermostLinkerIndex) )
							{
								FObjectImport& PackageImport = Linker->ImportMap( -OutermostLinkerIndex - 1 );
								PackageName = PackageImport.ObjectName;
							}
							else
							{
								check(OutermostLinkerIndex != ROOTPACKAGE_INDEX);
								FObjectExport& PackageExport = Linker->ExportMap( OutermostLinkerIndex - 1 );
								PackageName = PackageExport.ObjectName;
							}
						}

						if ( (InfoFlags&PKGINFO_Imports) != 0 )
						{
							GWarn->Log ( TEXT("\t*************************"));
							GWarn->Logf( TEXT("\tImport %d: '%s'"), i, *import.ObjectName.ToString() );
							GWarn->Logf( TEXT("\t\t       Outer: '%s' (%d)"), *OuterName.ToString(), import.OuterIndex);
							GWarn->Logf( TEXT("\t\t     Package: '%s'"), *PackageName.ToString());
							GWarn->Logf( TEXT("\t\t       Class: '%s'"), *import.ClassName.ToString() );
							GWarn->Logf( TEXT("\t\tClassPackage: '%s'"), *import.ClassPackage.ToString() );
							GWarn->Logf( TEXT("\t\t     XObject: %s"), import.XObject ? TEXT("VALID") : TEXT("NULL"));
							GWarn->Logf( TEXT("\t\t SourceIndex: %d"), import.SourceIndex );

							// dump depends info
							if (InfoFlags & PKGINFO_Depends)
							{
								TArray<FDependencyRef> AllDepends;
								Linker->GatherImportDependencies(i, AllDepends);
								GWarn->Log(TEXT("\t\t  All Depends:"));
								for (INT DependsIndex = 0; DependsIndex < AllDepends.Num(); DependsIndex++)
								{
									FDependencyRef& Ref = AllDepends(DependsIndex);
									GWarn->Logf(TEXT("\t\t\t%i) %s"), DependsIndex, *Ref.Linker->GetExportFullName(Ref.ExportIndex));
								}
							}
						}

						if ( PackageName == NAME_None && import.ClassPackage == NAME_Core && import.ClassName == NAME_Package )
						{
							PackageName = import.ObjectName;
						}

						if ( PackageName != NAME_None && PackageName != LinkerName )
						{
							DependentPackages.AddUniqueItem(PackageName);
						}

						if ( import.ClassPackage != NAME_None && import.ClassPackage != LinkerName )
						{
							DependentPackages.AddUniqueItem(import.ClassPackage);
						}
					}

					if ( DependentPackages.Num() )
					{
						GWarn->Log( TEXT("--------------------------------------------") );
						warnf(TEXT("\tPackages referenced by %s:"), *LinkerName.ToString());
						for ( INT i = 0; i < DependentPackages.Num(); i++ )
						{
							warnf(TEXT("\t\t%i) %s"), i, *DependentPackages(i).ToString());
						}
					}
				}

				if( (InfoFlags&PKGINFO_Exports) != 0 )
				{
					GWarn->Log( TEXT("--------------------------------------------") );
					GWarn->Log ( TEXT("Export Map"));
					GWarn->Log ( TEXT("=========="));

					if ( (InfoFlags&PKGINFO_Compact) == 0 )
					{
						for( INT i = 0; i < Linker->ExportMap.Num(); ++i )
						{
							GWarn->Log ( TEXT("\t*************************"));
							FObjectExport& Export = Linker->ExportMap( i );

							UBOOL bIsForcedExportPackage=FALSE;

							// determine if this export is a forced export in a cooked package
							FString ForcedExportString;
							if ( (Linker->Summary.PackageFlags&PKG_Cooked) != 0 && Export.HasAnyFlags(EF_ForcedExport) )
							{
								// find the package object this forced export was originally contained within
								INT PackageExportIndex = ROOTPACKAGE_INDEX;
								for ( INT OuterIndex = Export.OuterIndex; OuterIndex != ROOTPACKAGE_INDEX; OuterIndex = Linker->ExportMap(OuterIndex-1).OuterIndex )
								{
									PackageExportIndex = OuterIndex - 1;
								}

								if ( PackageExportIndex == ROOTPACKAGE_INDEX )
								{
									// this export corresponds to a top-level UPackage
									bIsForcedExportPackage = TRUE;
									ForcedExportString = TEXT(" [** FORCED **]");
								}
								else
								{
									// this export was a forced export that is not a top-level UPackage
									FObjectExport& OuterExport = Linker->ExportMap(PackageExportIndex);
									checkf(OuterExport.HasAnyFlags(EF_ForcedExport), TEXT("Export %i (%s) is a forced export but its outermost export %i (%s) is not!"),
										i, *Linker->GetExportPathName(i), PackageExportIndex, *Linker->GetExportPathName(PackageExportIndex));

									ForcedExportString = FString::Printf(TEXT(" [** FORCED: '%s' (%i)]"), *OuterExport.ObjectName.ToString(), PackageExportIndex);
								}
							}
							GWarn->Logf( TEXT("\tExport %d: '%s'%s"), i, *Export.ObjectName.ToString(), *ForcedExportString );

							// find the name of this object's class
							INT ClassIndex = Export.ClassIndex;
							FName ClassName = ClassIndex>0 
								? Linker->ExportMap(ClassIndex-1).ObjectName
								: ClassIndex<0 
								? Linker->ImportMap(-ClassIndex-1).ObjectName
								: FName(NAME_Class);

							// find the name of this object's parent...for UClasses, this will be the parent class
							// for UFunctions, this will be the SuperFunction, if it exists, etc.
							FName ParentName = NAME_None;
							if ( Export.SuperIndex > 0 )
							{
								FObjectExport& ParentExport = Linker->ExportMap(Export.SuperIndex-1);
								ParentName = ParentExport.ObjectName;
							}
							else if ( Export.SuperIndex < 0 )
							{
								FObjectImport& ParentImport = Linker->ImportMap(-Export.SuperIndex-1);
								ParentName = ParentImport.ObjectName;
							}

							// find the name of this object's Outer.  For UClasses, this will generally be the
							// top-level package itself.  For properties, a UClass, etc.
							FName OuterName = NAME_None;
							if ( Export.OuterIndex > 0 )
							{
								FObjectExport& OuterExport = Linker->ExportMap(Export.OuterIndex-1);
								OuterName = OuterExport.ObjectName;
							}
							else if ( Export.OuterIndex < 0 )
							{
								FObjectImport& OuterImport = Linker->ImportMap(-Export.OuterIndex-1);
								OuterName = OuterImport.ObjectName;
							}

							FName TemplateName = NAME_None;
							if ( Export.ArchetypeIndex > 0 )
							{
								FObjectExport& TemplateExport = Linker->ExportMap(Export.ArchetypeIndex-1);
								TemplateName = TemplateExport.ObjectName;
							}
							else if ( Export.ArchetypeIndex < 0 )
							{
								FObjectImport& TemplateImport = Linker->ImportMap(-Export.ArchetypeIndex-1);
								TemplateName = TemplateImport.ObjectName;
							}

							GWarn->Logf( TEXT("\t\t         Class: '%s' (%i)"), *ClassName.ToString(), ClassIndex );
							GWarn->Logf( TEXT("\t\t        Parent: '%s' (%d)"), *ParentName.ToString(), Export.SuperIndex );
							GWarn->Logf( TEXT("\t\t         Outer: '%s' (%d)"), *OuterName.ToString(), Export.OuterIndex );
							GWarn->Logf( TEXT("\t\t     Archetype: '%s' (%d)"), *TemplateName.ToString(), Export.ArchetypeIndex);
							GWarn->Logf( TEXT("\t\t      Pkg Guid: %s"), *Export.PackageGuid.String());
							GWarn->Logf( TEXT("\t\t   ObjectFlags: 0x%016I64X"), Export.ObjectFlags );
							GWarn->Logf( TEXT("\t\t          Size: %d"), Export.SerialSize );
							if ( !bHideOffsets )
							{
								GWarn->Logf( TEXT("\t\t      Offset: %d"), Export.SerialOffset );
							}
							GWarn->Logf( TEXT("\t\t       _Object: %s"), Export._Object ? TEXT("VALID") : TEXT("NULL"));
							if ( !bHideOffsets )
							{
								GWarn->Logf( TEXT("\t\t    _iHashNext: %d"), Export._iHashNext );
							}
							GWarn->Logf( TEXT("\t\t   ExportFlags: %X"), Export.ExportFlags );
							if ( Export.ComponentMap.Num() )
							{
								GWarn->Log(TEXT("\t\t  ComponentMap:"));
								INT Count = 0;
								for ( TMap<FName,INT>::TIterator It(Export.ComponentMap); It; ++It )
								{
									GWarn->Logf(TEXT("\t\t\t%i) %s (%i)"), Count++, *It.Key().ToString(), It.Value());
								}
							}

							if ( bIsForcedExportPackage && Export.GenerationNetObjectCount.Num() > 0 )
							{
								warnf(TEXT("\t\tNetObjectCounts: %d generations"), Export.GenerationNetObjectCount.Num());
								for ( INT GenerationIndex = 0; GenerationIndex < Export.GenerationNetObjectCount.Num(); GenerationIndex++ )
								{
									warnf(TEXT("\t\t\t%d) %d"), GenerationIndex, Export.GenerationNetObjectCount(GenerationIndex));
								}
							}

							// dump depends info
							if (InfoFlags & PKGINFO_Depends)
							{
								if (i < Linker->DependsMap.Num())
								{
									TArray<INT>& Depends = Linker->DependsMap(i);
									GWarn->Log(TEXT("\t\t  DependsMap:"));
									for (INT DependsIndex = 0; DependsIndex < Depends.Num(); DependsIndex++)
									{
										GWarn->Logf(TEXT("\t\t\t%i) %s (%i)"), DependsIndex, 
											IS_IMPORT_INDEX(Depends(DependsIndex)) ? 
												*Linker->GetImportFullName(-Depends(DependsIndex) - 1) :
												*Linker->GetExportFullName(Depends(DependsIndex) - 1),
											Depends(DependsIndex));
									}

									TArray<FDependencyRef> AllDepends;
									Linker->GatherExportDependencies(i, AllDepends);
									GWarn->Log(TEXT("\t\t  All Depends:"));
									for (INT DependsIndex = 0; DependsIndex < AllDepends.Num(); DependsIndex++)
									{
										FDependencyRef& Ref = AllDepends(DependsIndex);
										GWarn->Logf(TEXT("\t\t\t%i) %s"), DependsIndex, *Ref.Linker->GetExportFullName(Ref.ExportIndex));
									}
								}
							}
						}
					}
					else
					{
						for( INT ExportIndex=0; ExportIndex<Linker->ExportMap.Num(); ExportIndex++ )
						{
							const FObjectExport& Export = Linker->ExportMap(ExportIndex);
							warnf(TEXT("  %8i %10i %s"), ExportIndex, Export.SerialSize, *Export.ObjectName.ToString());
						}
					}
				}
			}

			UObject::CollectGarbage(RF_Native);
		}
	}

	return 0;
}
IMPLEMENT_CLASS(UPkgInfoCommandlet)


/*-----------------------------------------------------------------------------
UListCorruptedComponentsCommandlet
-----------------------------------------------------------------------------*/

/**
* This commandlet is designed to find (and in the future, possibly fix) content that is affected by the components bug described in
* TTPRO #15535 and UComponentProperty::InstanceComponents()
*/
INT UListCorruptedComponentsCommandlet::Main(const FString& Params)
{
	// Parse command line args.
	TArray<FString> Tokens;
	TArray<FString> Switches;

	const TCHAR* Parms = *Params;
	ParseCommandLine(Parms, Tokens, Switches);

	// Build package file list.
	const TArray<FString> FilesInPath( GPackageFileCache->GetPackageFileList() );
	if( FilesInPath.Num() == 0 )
	{
		warnf( NAME_Warning, TEXT("No packages found") );
		return 1;
	}

	const UBOOL bCheckVersion = Switches.ContainsItem(TEXT("CHECKVER"));

	// Iterate over all files doing stuff.
	for( INT FileIndex = 0 ; FileIndex < FilesInPath.Num() ; ++FileIndex )
	{
		const FFilename& Filename = FilesInPath(FileIndex);
		warnf( NAME_Log, TEXT("Loading %s"), *Filename );

		UObject* Package = UObject::LoadPackage( NULL, *Filename, LOAD_None );
		if( Package == NULL )
		{
			warnf( NAME_Error, TEXT("Error loading %s!"), *Filename );
		}
		else if( bCheckVersion && Package->GetLinkerVersion() != GPackageFileVersion )
		{
			warnf( NAME_Log, TEXT("Version mismatch. Package [%s] should be resaved."), *Filename );
		}

		UBOOL bInsertNewLine = FALSE;
		for ( TObjectIterator<UComponent> It; It; ++It )
		{
			if ( It->IsIn(Package) && !It->IsTemplate(RF_ClassDefaultObject) )
			{
				UComponent* Component = *It;
				UComponent* ComponentTemplate = Cast<UComponent>(Component->GetArchetype());
				UObject* Owner = Component->GetOuter();
				UObject* TemplateOwner = ComponentTemplate->GetOuter();
				if ( !ComponentTemplate->HasAnyFlags(RF_ClassDefaultObject) )
				{
					if ( TemplateOwner != Owner->GetArchetype() )
					{
						bInsertNewLine = TRUE;

						FString RealArchetypeName;
						if ( Component->TemplateName != NAME_None )
						{
							UComponent* RealArchetype = Owner->GetArchetype()->FindComponent(Component->TemplateName);
							if ( RealArchetype != NULL )
							{
								RealArchetypeName = RealArchetype->GetFullName();
							}
							else
							{
								RealArchetypeName = FString::Printf(TEXT("NULL: no matching components found in Owner Archetype %s"), *Owner->GetArchetype()->GetFullName());
							}
						}
						else
						{
							RealArchetypeName = TEXT("NULL");
						}

						warnf(TEXT("\tPossible corrupted component: '%s'	Archetype: '%s'	TemplateName: '%s'	ResolvedArchetype: '%s'"),
							*Component->GetFullName(), 
							*ComponentTemplate->GetPathName(),
							*Component->TemplateName.ToString(),
							*RealArchetypeName);
					}

					if ( Component->GetClass()->HasAnyClassFlags(CLASS_UniqueComponent) )
					{
						bInsertNewLine = TRUE;
						warnf(TEXT("\tComponent is using unique component class: %s"), *Component->GetFullName());
					}

					if ( ComponentTemplate->GetClass()->HasAnyClassFlags(CLASS_UniqueComponent) )
					{
						bInsertNewLine = TRUE;
						warnf(TEXT("\tComponent archetype has unique component class: '%s'	Archetype: '%s'"), *Component->GetFullName(), *ComponentTemplate->GetPathName());
					}
				}
			}
		}

		if ( bInsertNewLine )
		{
			warnf(TEXT(""));
		}
		UObject::CollectGarbage( RF_Native );
	}

	return 0;
}

IMPLEMENT_CLASS(UListCorruptedComponentsCommandlet);

/*-----------------------------------------------------------------------------
UAnalyzeCookedPackages commandlet.
-----------------------------------------------------------------------------*/

INT UAnalyzeCookedPackagesCommandlet::Main( const FString& Params )
{
	// Parse command line.
	TArray<FString> Tokens;
	TArray<FString> Switches;

	const TCHAR* Parms = *Params;
	ParseCommandLine(Parms, Tokens, Switches);

	// Tokens on the command line are package wildcards.
	for( INT TokenIndex=0; TokenIndex<Tokens.Num(); TokenIndex++ )
	{
		// Find all files matching current wildcard.
		FFilename		Wildcard = Tokens(TokenIndex);
		TArray<FString> Filenames;
		GFileManager->FindFiles( Filenames, *Wildcard, TRUE, FALSE );

		// Iterate over all found files.
		for( INT FileIndex = 0; FileIndex < Filenames.Num(); FileIndex++ )
		{
			const FString& Filename = Wildcard.GetPath() + PATH_SEPARATOR + Filenames(FileIndex);

			UObject::BeginLoad();
			ULinkerLoad* Linker = UObject::GetPackageLinker( NULL, *Filename, LOAD_None, NULL, NULL );
			UObject::EndLoad();

			if( Linker )
			{
				check(Linker->LinkerRoot);
				check(Linker->Summary.PackageFlags & PKG_Cooked);

				// Display information about the package.
				FName LinkerName = Linker->LinkerRoot->GetFName();

				// Display summary info.
				GWarn->Logf( TEXT("********************************************") );
				GWarn->Logf( TEXT("Package '%s' Summary"), *LinkerName.ToString() );
				GWarn->Logf( TEXT("--------------------------------------------") );

				GWarn->Logf( TEXT("\t     Version: %i"), Linker->Ver() );
				GWarn->Logf( TEXT("\tPackageFlags: %x"), Linker->Summary.PackageFlags );
				GWarn->Logf( TEXT("\t   NameCount: %d"), Linker->Summary.NameCount );
				GWarn->Logf( TEXT("\t  NameOffset: %d"), Linker->Summary.NameOffset );
				GWarn->Logf( TEXT("\t ImportCount: %d"), Linker->Summary.ImportCount );
				GWarn->Logf( TEXT("\tImportOffset: %d"), Linker->Summary.ImportOffset );
				GWarn->Logf( TEXT("\t ExportCount: %d"), Linker->Summary.ExportCount );
				GWarn->Logf( TEXT("\tExportOffset: %d"), Linker->Summary.ExportOffset );

				FString szGUID = Linker->Summary.Guid.String();
				GWarn->Logf( TEXT("\t        Guid: %s"), *szGUID );
				GWarn->Logf( TEXT("\t Generations:"));
				for( INT i=0; i<Linker->Summary.Generations.Num(); i++ )
				{
					const FGenerationInfo& GenerationInfo = Linker->Summary.Generations( i );
					GWarn->Logf( TEXT("\t\t%d) ExportCount=%d, NameCount=%d"), i, GenerationInfo.ExportCount, GenerationInfo.NameCount );
				}

				GWarn->Logf( TEXT("") );
				GWarn->Logf( TEXT("Exports:") );
				GWarn->Logf( TEXT("Class Outer Name Size Offset ExportFlags ObjectFlags") );

				for( INT i = 0; i < Linker->ExportMap.Num(); ++i )
				{
					FObjectExport& Export = Linker->ExportMap( i );

					// Find the name of this object's class.
					INT ClassIndex	= Export.ClassIndex;
					FName ClassName = NAME_Class;
					if( ClassIndex > 0 )
					{
						ClassName = Linker->ExportMap(ClassIndex-1).ObjectName;
					}
					else if( ClassIndex < 0 )
					{
						Linker->ImportMap(-ClassIndex-1).ObjectName;
					}

					// Find the name of this object's Outer.  For UClasses, this will generally be the
					// top-level package itself.  For properties, a UClass, etc.
					FName OuterName = NAME_None;
					if ( Export.OuterIndex > 0 )
					{
						FObjectExport& OuterExport = Linker->ExportMap(Export.OuterIndex-1);
						OuterName = OuterExport.ObjectName;
					}
					else if ( Export.OuterIndex < 0 )
					{
						FObjectImport& OuterImport = Linker->ImportMap(-Export.OuterIndex-1);
						OuterName = OuterImport.ObjectName;
					}

					//GWarn->Logf( TEXT("Class Outer Name Size Offset ExportFlags ObjectFlags") );
					GWarn->Logf( TEXT("%s %s %s %i %i %x 0x%016I64X"), 
						*ClassName.ToString(), 
						*OuterName.ToString(), 
						*Export.ObjectName.ToString(), 
						Export.SerialSize, 
						Export.SerialOffset, 
						Export.ExportFlags, 
						Export.ObjectFlags );
				}
			}

			UObject::CollectGarbage(RF_Native);
		}
	}
	return 0;
}
IMPLEMENT_CLASS(UAnalyzeCookedPackagesCommandlet)



/**
 * This will look for Textures which:
 *
 *   0) are probably specular (based off their name) and see if they have an LODBias of two
 *   1) have a negative LODBias
 *   2) have neverstream set
 *
 * All of the above are things which can be considered suspicious and probably should be changed
 * for best memory usage.  
 *
 * Specifically:  Specular with an LODBias of 2 was not noticeably different at the texture resolutions we used for gears; 512+
 *
 **/
struct TextureCheckerFunctor
{
	template< typename OBJECTYPE >
	void DoIt( UPackage* Package )
	{
		for( TObjectIterator<OBJECTYPE> It; It; ++It )
		{
			OBJECTYPE* Texture2D = *It;

			if( Texture2D->IsIn( Package ) == FALSE )
			{
				continue;
			}

			const FString&  TextureName = Texture2D->GetPathName();
			const INT		LODBias     = Texture2D->LODBias;

			//warnf( TEXT( " checking %s" ), *TextureName );

			// if this has been named as a specular texture
			if( ( ( TextureName.ToUpper().InStr( TEXT("SPEC" )) != INDEX_NONE )  // gears
				   || ( TextureName.ToUpper().InStr( TEXT("_S0" )) != INDEX_NONE )  // ut
				   || ( TextureName.ToUpper().InStr( TEXT("_S_" )) != INDEX_NONE )  // ut
				   || ( (TextureName.ToUpper().Right(2)).InStr( TEXT("_S" )) != INDEX_NONE )  // ut
				   )
				&& ( !( ( LODBias == 2 ) 
       				      || ( Texture2D->LODGroup == TEXTUREGROUP_WorldSpecular )
						  || ( Texture2D->LODGroup == TEXTUREGROUP_CharacterSpecular )
						  || ( Texture2D->LODGroup == TEXTUREGROUP_WeaponSpecular )
						  || ( Texture2D->LODGroup == TEXTUREGROUP_VehicleSpecular )
					   )
				    )
				)		 
			{
				warnf( TEXT("Specular LODBias of 2 Not Set for:  %s ( Currently has %d )  OR not set to a SpecularTextureGroup (Currently has %d)"), *TextureName, LODBias, Texture2D->LODGroup );
			}


			// if this has a negative LOD Bias that might be suspicious :-) (artists will often bump up the LODBias on their pet textures to make certain they are full full res)
			if( ( LODBias < 0 )
				)
			{
				warnf( TEXT("LODBias is negative for: %s ( Currently has %d )"), *TextureName, LODBias );
			}

			if( ( Texture2D->NeverStream == TRUE )
				&& ( TextureName.InStr( TEXT("TerrainWeightMapTexture" )) == INDEX_NONE )  // TerrainWeightMapTextures are neverstream so we don't care about them
				
				)
			{
				warnf( TEXT("NeverStream is set to true for: %s"), *TextureName );
			}

			// if this texture is a G8 it usually can be converted to a dxt without loss of quality.  Remember a 1024x1024 G* is always 1024!
			if( Texture2D->CompressionSettings == TC_Grayscale )
			{
				warnf( TEXT("G8 texture: %s"), *TextureName );
			}

		}
	}
};


/**
* This will find materials which are missing Physical Materials
**/
struct MaterialMissingPhysMaterialFunctor
{
	template< typename OBJECTYPE >
	void DoIt( UPackage* Package )
	{
		for( TObjectIterator<OBJECTYPE> It; It; ++It )
		{
			OBJECTYPE* Material = *It;

			if( Material->IsIn( Package ) == FALSE )
			{
				continue;
			}

			UBOOL bHasPhysicalMaterial = FALSE;

			if( Material->PhysMaterial != NULL )
			{
				// if we want to do some other logic such as looking at the textures
				// used and seeing if they are all of one type (e.g. all effects textures probably
				// imply that the material is used in an effect and probably doesn't need a physical material)
				//TArray<UTexture*> OutTextures;
				//Material->GetTextures( OutTextures, TRUE );
				//const FTextureLODGroup& LODGroup = TextureLODGroups[Texture->LODGroup];

				bHasPhysicalMaterial = TRUE;
			}

			if( bHasPhysicalMaterial == FALSE )
			{
				const FString& MaterialName = Material->GetPathName();
				warnf( TEXT("Lacking PhysicalMaterial:  %s"), *MaterialName  );
			}
		}
	}
};


/**
* This will find SoundCues which are missing Sound Groups.
**/
struct SoundCueMissingGroupsFunctor
{
	template< typename OBJECTYPE >
	void DoIt( UPackage* Package )
	{
		for( TObjectIterator<OBJECTYPE> It; It; ++It )
		{
			OBJECTYPE* TheSoundCue = *It;

			if( TheSoundCue->IsIn( Package ) == FALSE )
			{
				continue;
			}

			UBOOL bHasAGroup = FALSE;

			if( TheSoundCue->SoundGroup != NAME_None )
			{
				// if we want to do some other logic such as looking at the textures
				// used and seeing if they are all of one type (e.g. all effects textures probably
				// imply that the material is used in an effect and probably doesn't need a physical material)
				//TArray<UTexture*> OutTextures;
				//Material->GetTextures( OutTextures, TRUE );
				//const FTextureLODGroup& LODGroup = TextureLODGroups[Texture->LODGroup];

				bHasAGroup = TRUE;
			}

			if( bHasAGroup == FALSE )
			{
				const FString& TheSoundCueName = TheSoundCue->GetPathName();
				warnf( TEXT("Lacking a Group:  %s"), *TheSoundCueName );
			}
		}
	}
};

struct SetTextureLODGroupFunctor
{
	template< typename OBJECTYPE >
	void DoIt( UPackage* Package )
	{
		UBOOL bDirtyPackage = FALSE;

		const FName& PackageName = Package->GetFName(); 
		FString PackageFileName;
		GPackageFileCache->FindPackageFile( *PackageName.ToString(), NULL, PackageFileName );
		//warnf( NAME_Log, TEXT("  Loading2 %s"), *PackageFileName );

		for( TObjectIterator<OBJECTYPE> It; It; ++It )
		{
			OBJECTYPE* Texture2D = *It;

			if( Texture2D->IsIn( Package ) == FALSE )
			{
				continue;
			}

			if( Package->GetLinker() != NULL
				&& Package->GetLinker()->Summary.EngineVersion == 2904 )
			{
				warnf( NAME_Log, TEXT( "Already 2904" ) );
				continue;
			}

			UBOOL bDirty = FALSE;
			UBOOL bIsSpec = FALSE;
			UBOOL bIsNormal = FALSE;

			const FString& TextureName = Texture2D->GetPathName();

			warnf( NAME_Log, TEXT( "TextureName: %s" ), *TextureName );

			const BYTE OrigGroup = Texture2D->LODGroup;
			const INT OrigLODBias = Texture2D->LODBias;


			// due to enum fiasco 2007 now we need to find which "type" it is based off the package name
			enum EPackageType
			{
				PACKAGE_CHARACTER,
				PACKAGE_EFFECTS,
				PACKAGE_UI,
				PACKAGE_VEHICLE,
				PACKAGE_WEAPON,
				PACKAGE_WORLD,
			};


			EPackageType ThePackageType = PACKAGE_WORLD;

			if( PackageName.ToString().Left(3).InStr( TEXT("CH_") ) != INDEX_NONE )
			{
				ThePackageType = PACKAGE_CHARACTER;
				//Texture2D->LODGroup = TEXTUREGROUP_World;
			}
			else if( PackageName.ToString().Left(3).InStr( TEXT("VH_") ) != INDEX_NONE )
			{
				ThePackageType = PACKAGE_VEHICLE;
				//Texture2D->LODGroup = TEXTUREGROUP_Vehicle;
			}
			else if( PackageName.ToString().Left(3).InStr( TEXT("WP_") ) != INDEX_NONE )
			{
				ThePackageType = PACKAGE_WEAPON;
				//Texture2D->LODGroup = TEXTUREGROUP_Weapon;
			}
			else if( PackageFileName.InStr( TEXT("Effects") ) != INDEX_NONE )
			{
				ThePackageType = PACKAGE_EFFECTS;
				if( Texture2D->LODGroup != TEXTUREGROUP_Effects )
				{
					Texture2D->LODGroup = TEXTUREGROUP_Effects;
					bDirty = TRUE; 
				}

			}
			else if( PackageName.ToString().Left(3).InStr( TEXT("UI_") ) != INDEX_NONE )
			{
				ThePackageType = PACKAGE_UI;
				//Texture2D->LODGroup = TEXTUREGROUP_Effects;
			}
			else
			{
				ThePackageType = PACKAGE_WORLD;
				//Texture2D->LODGroup = TEXTUREGROUP_World;
			}


			// if this is a specular texture
			if( ( ( TextureName.ToUpper().InStr( TEXT("SPEC" )) != INDEX_NONE )  // gears
				|| ( TextureName.ToUpper().InStr( TEXT("_S0" )) != INDEX_NONE )  // ut
				|| ( TextureName.ToUpper().InStr( TEXT("_S_" )) != INDEX_NONE )  // ut
				|| ( (TextureName.ToUpper().Right(2)).InStr( TEXT("_S" )) != INDEX_NONE )  // ut
				)
				)
			{
				bIsSpec = TRUE;

				// all spec LOD should be 0 as we are going to be using a group now for setting it
				if( Texture2D->LODBias != 0 )
				{
					Texture2D->LODBias = 0; 
					bDirty = TRUE; 
				}

				if( ( ThePackageType == PACKAGE_WORLD ) && ( Texture2D->LODGroup != TEXTUREGROUP_WorldSpecular ) )
				{
					Texture2D->LODGroup = TEXTUREGROUP_WorldSpecular; 
					bDirty = TRUE; 
				}
				else if( ( ThePackageType == PACKAGE_CHARACTER ) && ( Texture2D->LODGroup != TEXTUREGROUP_CharacterSpecular ) )
				{
					Texture2D->LODGroup = TEXTUREGROUP_CharacterSpecular; 
					bDirty = TRUE; 
				}
				else if( ( ThePackageType == PACKAGE_WEAPON ) && ( Texture2D->LODGroup != TEXTUREGROUP_WeaponSpecular ) )
				{
					Texture2D->LODGroup = TEXTUREGROUP_WeaponSpecular; 
					bDirty = TRUE; 
				}
				else if( ( ThePackageType == PACKAGE_VEHICLE ) && ( Texture2D->LODGroup != TEXTUREGROUP_VehicleSpecular ) )
				{
					Texture2D->LODGroup = TEXTUREGROUP_VehicleSpecular; 
					bDirty = TRUE; 
				}
			}
		
			if( ( Texture2D->CompressionSettings == TC_Normalmap )
				)
			{
				bIsNormal = TRUE;

				if( ( ThePackageType == PACKAGE_WORLD ) && ( Texture2D->LODGroup != TEXTUREGROUP_WorldNormalMap ) )
				{
					Texture2D->LODGroup = TEXTUREGROUP_WorldNormalMap; 
					bDirty = TRUE; 
				}
				else if( ( ThePackageType == PACKAGE_CHARACTER ) && ( Texture2D->LODGroup != TEXTUREGROUP_CharacterNormalMap ))
				{
					Texture2D->LODGroup = TEXTUREGROUP_CharacterNormalMap; 
					bDirty = TRUE; 
				}
				else if( ( ThePackageType == PACKAGE_WEAPON ) && ( Texture2D->LODGroup != TEXTUREGROUP_WeaponNormalMap ))
				{
					Texture2D->LODGroup = TEXTUREGROUP_WeaponNormalMap; 
					bDirty = TRUE; 
				}
				else if( ( ThePackageType == PACKAGE_VEHICLE ) && ( Texture2D->LODGroup != TEXTUREGROUP_VehicleNormalMap ) )
				{
					Texture2D->LODGroup = TEXTUREGROUP_VehicleNormalMap; 
					bDirty = TRUE; 
				}
			}


			if( ( ThePackageType == PACKAGE_UI ) && ( Texture2D->LODGroup != TEXTUREGROUP_UI ) )
			{
				Texture2D->LODGroup = TEXTUREGROUP_UI; 
				bDirty = TRUE; 
			}

			if( ( ( PackageFileName.InStr( TEXT("EngineFonts") ) != INDEX_NONE )
				|| ( PackageFileName.InStr( TEXT("UI_Fonts") ) != INDEX_NONE )
				) 
				&& ( Texture2D->LODGroup != TEXTUREGROUP_UI )
				)
			{
				Texture2D->LODGroup = TEXTUREGROUP_UI;
				bDirty = TRUE;
			}


			// SO now if we have not already modified the texture above then we need to
			// do one more final check to see if the texture in the package is actually
			// classified correctly
			if( ( bDirty == FALSE ) && ( bIsSpec == FALSE ) && ( bIsNormal == FALSE ) )
			{
				if( ( ThePackageType == PACKAGE_CHARACTER ) && ( Texture2D->LODGroup != TEXTUREGROUP_Character ) )
				{
					Texture2D->LODGroup = TEXTUREGROUP_Character; 
					bDirty = TRUE; 
				}
				else if( ( ThePackageType == PACKAGE_WEAPON ) && ( Texture2D->LODGroup != TEXTUREGROUP_Weapon ) )
				{
					Texture2D->LODGroup = TEXTUREGROUP_Weapon; 
					bDirty = TRUE; 
				}
				else if( ( ThePackageType == PACKAGE_VEHICLE ) && ( Texture2D->LODGroup != TEXTUREGROUP_Vehicle ) )
				{
					Texture2D->LODGroup = TEXTUREGROUP_Vehicle; 
					bDirty = TRUE; 
				}
			}


			if( bDirty == TRUE )
			{
				bDirtyPackage = TRUE;

				warnf( TEXT("Changing LODBias from:  %d to %d   LODGroup from:  %d to %d   for %s "), OrigLODBias, Texture2D->LODBias, OrigGroup, Texture2D->LODGroup, *TextureName );
			}

			bDirty = FALSE;
			bIsNormal = FALSE;
			bIsSpec = FALSE;
		}
		



		if( bDirtyPackage == TRUE )
		{
			FSourceControlIntegration* SCC = new FSourceControlIntegration;

			/** if we should auto checkout packages that need to be saved**/
			const UBOOL bAutoCheckOut = ParseParam(appCmdLine(),TEXT("AutoCheckOutPackages"));

			// kk now we want to possible save the package
			const UBOOL bIsReadOnly = GFileManager->IsReadOnly( *PackageFileName);

			// check to see if we need to check this package out
			if( bIsReadOnly == TRUE && bAutoCheckOut == TRUE )
			{
				SCC->CheckOut(Package);
			}

			try
			{
				if( SavePackageHelper( Package, PackageFileName ) == TRUE )
				{
					warnf( NAME_Log, TEXT("Correctly saved:  [%s]."), *PackageFileName );
				}
			}
			catch( ... )
			{
				warnf( NAME_Log, TEXT("Lame Exception %s"), *PackageFileName );
			}


			delete SCC; // clean up our allocated SCC
			SCC = NULL;
		}

	}
	
};



/**
* This is our Functional "Do an Action to all Packages" Template.  Basically it handles all
* of the boilerplate code which normally gets copy pasted around.  So now we just pass in
* the OBJECTYPE  (e.g. Texture2D ) and then the Functor which will do the actual work.
*
* @see UFindMissingPhysicalMaterialsCommandlet
* @see UFindTexturesWhichLackLODBiasOfTwoCommandlet
**/
template< typename OBJECTYPE, typename FUNCTOR >
void DoActionToAllPackages( const FString& Params )
{
	// Parse command line.
	TArray<FString> Tokens;
	TArray<FString> Switches;

	const TCHAR* Parms = *Params;
	UCommandlet::ParseCommandLine(Parms, Tokens, Switches);

	const UBOOL bVerbose = Switches.FindItemIndex(TEXT("VERBOSE")) != INDEX_NONE;
	const UBOOL bLoadMaps = Switches.FindItemIndex(TEXT("LOADMAPS")) != INDEX_NONE;

	TArray<FString> FilesInPath;
	FilesInPath = GPackageFileCache->GetPackageFileList();

	INT GCIndex = 0;
	for( INT FileIndex = 0; FileIndex < FilesInPath.Num(); FileIndex++ )
	{
		const FFilename& Filename = FilesInPath(FileIndex);

		// we don't care about trying to wrangle the various shader caches so just skipz0r them
		if(	Filename.GetBaseFilename().InStr( TEXT("LocalShaderCache") )	!= INDEX_NONE
		||	Filename.GetBaseFilename().InStr( TEXT("RefShaderCache") )		!= INDEX_NONE 
		)
		{
			continue;
		}

		// if we don't want to load maps for this
		if( ( bLoadMaps == FALSE ) && ( Filename.GetExtension() == FURL::DefaultMapExt ) )
		{
			continue;
		}


		if( bVerbose == TRUE )
		{
			warnf( NAME_Log, TEXT("Loading %s"), *Filename );
		}

		// don't die out when we have a few bad packages, just keep on going so we get most of the data
		try
		{
			UPackage* Package = UObject::LoadPackage( NULL, *Filename, LOAD_None );
			if( Package != NULL )
			{
				FUNCTOR TheFunctor;
				TheFunctor.DoIt<OBJECTYPE>( Package );
			}
			else
			{
				warnf( NAME_Error, TEXT("Error loading %s!"), *Filename );
			}
		}
		catch ( ... )
		{
			warnf( NAME_Log, TEXT("Exception %s"), *Filename.GetBaseFilename() );
		}



		if( (++GCIndex % 10) == 0 )
		{
			UObject::CollectGarbage(RF_Native);
		}
	}
}



/*-----------------------------------------------------------------------------
FindMissingPhysicalMaterials commandlet.
-----------------------------------------------------------------------------*/

INT UFindTexturesWithMissingPhysicalMaterialsCommandlet::Main( const FString& Params )
{
	DoActionToAllPackages<UMaterial, MaterialMissingPhysMaterialFunctor>(Params);

	return 0;
}
IMPLEMENT_CLASS(UFindTexturesWithMissingPhysicalMaterialsCommandlet)


/*-----------------------------------------------------------------------------
FindQuestionableTextures Commandlet
-----------------------------------------------------------------------------*/

INT UFindQuestionableTexturesCommandlet::Main( const FString& Params )
{
	DoActionToAllPackages<UTexture2D, TextureCheckerFunctor>(Params);

	return 0;
}
IMPLEMENT_CLASS(UFindQuestionableTexturesCommandlet)


/*-----------------------------------------------------------------------------
SetTextureLODGroup Commandlet
-----------------------------------------------------------------------------*/
INT USetTextureLODGroupCommandlet::Main( const FString& Params )
{
	DoActionToAllPackages<UTexture2D, SetTextureLODGroupFunctor>(Params);

	return 0;
}
IMPLEMENT_CLASS(USetTextureLODGroupCommandlet)


/*-----------------------------------------------------------------------------
FindSoundCuesWithMissingGroups commandlet.
-----------------------------------------------------------------------------*/
INT UFindSoundCuesWithMissingGroupsCommandlet::Main( const FString& Params )
{
	DoActionToAllPackages<USoundCue, SoundCueMissingGroupsFunctor>(Params);

	return 0;
}
IMPLEMENT_CLASS(UFindSoundCuesWithMissingGroupsCommandlet)




/*-----------------------------------------------------------------------------
UListScriptReferencedContentCommandlet.
-----------------------------------------------------------------------------*/
/**
* Processes a value found by ListReferencedContent(), possibly recursing for inline objects
*
* @param	Value			the object to be processed
* @param	Property		the property where Value was found (for a dynamic array, this is the Inner property)
* @param	PropertyDesc	string printed as the property Value was assigned to (usually *Property->GetName(), except for dynamic arrays, where it's the array name and index)
* @param	Tab				string with a number of tabs for the current tab level of the output
*/
void UListScriptReferencedContentCommandlet::ProcessObjectValue(UObject* Value, UProperty* Property, const FString& PropertyDesc, const FString& Tab)
{
	if (Value != NULL)
	{
		// if it's an inline object, recurse over its properties
		if ((Property->PropertyFlags & CPF_NeedCtorLink) || (Property->PropertyFlags & CPF_Component))
		{
			ListReferencedContent(Value->GetClass(), (BYTE*)Value, FString(*Value->GetName()), Tab + TEXT("\t"));
		}
		else
		{
			// otherwise, print it as content that's being referenced
			warnf(TEXT("%s\t%s=%s'%s'"), *Tab, *PropertyDesc, *Value->GetClass()->GetName(), *Value->GetPathName());
		}
	}
}

/**
* Lists content referenced by the given data
*
* @param	Struct		the type of the Default data
* @param	Default		the data to look for referenced objects in
* @param	HeaderName	string printed before any content references found (only if the data might contain content references)
* @param	Tab			string with a number of tabs for the current tab level of the output
*/
void UListScriptReferencedContentCommandlet::ListReferencedContent(UStruct* Struct, BYTE* Default, const FString& HeaderName, const FString& Tab)
{
	UBOOL bPrintedHeader = FALSE;

	// iterate over all its properties
	for (UProperty* Property = Struct->PropertyLink; Property != NULL; Property = Property->PropertyLinkNext)
	{
		if ( !bPrintedHeader &&
			(Property->IsA(UObjectProperty::StaticClass()) || Property->IsA(UStructProperty::StaticClass()) || Property->IsA(UArrayProperty::StaticClass())) &&
			Property->ContainsObjectReference() )
		{
			// this class may contain content references, so print header with class/struct name
			warnf(TEXT("%s%s"), *Tab, *HeaderName);
			bPrintedHeader = TRUE;
		}
		// skip class properties and object properties of class Object
		UObjectProperty* ObjectProp = Cast<UObjectProperty>(Property);
		if (ObjectProp != NULL && ObjectProp->PropertyClass != UObject::StaticClass() && ObjectProp->PropertyClass != UClass::StaticClass())
		{
			if (ObjectProp->ArrayDim > 1)
			{
				for (INT i = 0; i < ObjectProp->ArrayDim; i++)
				{
					ProcessObjectValue(*(UObject**)(Default + Property->Offset + i * Property->ElementSize), Property, FString::Printf(TEXT("%s[%d]"), *Property->GetName(), i), Tab);
				}
			}
			else
			{
				ProcessObjectValue(*(UObject**)(Default + Property->Offset), Property, FString(*Property->GetName()), Tab);
			}
		}
		else if (Property->IsA(UStructProperty::StaticClass()))
		{
			if (Property->ArrayDim > 1)
			{
				for (INT i = 0; i < Property->ArrayDim; i++)
				{
					ListReferencedContent(((UStructProperty*)Property)->Struct, (Default + Property->Offset + i * Property->ElementSize), FString::Printf(TEXT("%s[%d]"), *Property->GetName(), i), Tab + TEXT("\t"));
				}
			}
			else
			{
				ListReferencedContent(((UStructProperty*)Property)->Struct, (Default + Property->Offset), FString(*Property->GetName()), Tab + TEXT("\t"));
			}
		}
		else if (Property->IsA(UArrayProperty::StaticClass()))
		{
			UArrayProperty* ArrayProp = (UArrayProperty*)Property;
			FArray* Array = (FArray*)(Default + Property->Offset);
			UObjectProperty* ObjectProp = Cast<UObjectProperty>(ArrayProp->Inner);
			if (ObjectProp != NULL && ObjectProp->PropertyClass != UObject::StaticClass() && ObjectProp->PropertyClass != UClass::StaticClass())
			{
				for (INT i = 0; i < Array->Num(); i++)
				{
					ProcessObjectValue(*(UObject**)((BYTE*)Array->GetData() + (i * ArrayProp->Inner->ElementSize)), ObjectProp, FString::Printf(TEXT("%s[%d]"), *ArrayProp->GetName(), i), Tab);
				}
			}
			else if (ArrayProp->Inner->IsA(UStructProperty::StaticClass()))
			{
				UStruct* InnerStruct = ((UStructProperty*)ArrayProp->Inner)->Struct;
				INT PropertiesSize = InnerStruct->GetPropertiesSize();
				for (INT i = 0; i < Array->Num(); i++)
				{
					ListReferencedContent(InnerStruct, (BYTE*)Array->GetData() + (i * ArrayProp->Inner->ElementSize), FString(*Property->GetName()), Tab + TEXT("\t"));
				}
			}
		}
	}
}

/** lists all content referenced in the default properties of script classes */
INT UListScriptReferencedContentCommandlet::Main(const FString& Params)
{
	warnf(TEXT("Loading EditPackages..."));
	// load all the packages in the EditPackages list
	BeginLoad();
	for (INT i = 0; i < GEditor->EditPackages.Num(); i++)
	{
		LoadPackage(NULL, *GEditor->EditPackages(i), 0);
	}
	EndLoad();

	// iterate over all classes
	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (!(It->ClassFlags & CLASS_Intrinsic))
		{
			ListReferencedContent(*It, (BYTE*)It->GetDefaultObject(), FString::Printf(TEXT("%s %s"), *It->GetFullName(), It->HasAnyFlags(RF_Native) ? TEXT("(native)") : TEXT("")));
		}
	}

	return 0;
}
IMPLEMENT_CLASS(UListScriptReferencedContentCommandlet);


/*-----------------------------------------------------------------------------
UAnalyzeContent commandlet.
-----------------------------------------------------------------------------*/

/**
* Helper structure to hold level/ usage count information.
*/
struct FLevelResourceStat
{
	/** Level package.						*/
	UObject*	LevelPackage;
	/** Usage count in above level package.	*/
	INT			Count;

	/**
	* Constructor, initializing Count to 1 and setting package.
	*
	* @param InLevelPackage	Level package to use
	*/
	FLevelResourceStat( UObject* InLevelPackage )
		: LevelPackage( InLevelPackage )
		, Count( 1 )
	{}
};
/**
* Helper structure containing usage information for a single resource and multiple levels.
*/
struct FResourceStat
{
	/** Resource object.											*/
	UObject*					Resource;
	/** Total number this resource is used across all levels.		*/
	INT							TotalCount;
	/** Array of detailed per level resource usage breakdown.		*/
	TArray<FLevelResourceStat>	LevelResourceStats;

	/**
	* Constructor
	*
	* @param	InResource		Resource to use
	* @param	LevelPackage	Level package to use
	*/
	FResourceStat( UObject* InResource, UObject* LevelPackage )
		:	Resource( InResource )
		,	TotalCount( 1 )
	{
		// Create initial stat entry.
		LevelResourceStats.AddItem( FLevelResourceStat( LevelPackage ) );
	}

	/**
	* Increment usage count by one
	*
	* @param	LevelPackage	Level package using the resource.
	*/
	void IncrementUsage( UObject* LevelPackage )
	{
		// Iterate over all level resource stats to find existing entry.
		UBOOL bFoundExisting = FALSE;
		for( INT LevelIndex=0; LevelIndex<LevelResourceStats.Num(); LevelIndex++ )
		{
			FLevelResourceStat& LevelResourceStat = LevelResourceStats(LevelIndex);
			// We found a match.
			if( LevelResourceStat.LevelPackage == LevelPackage )
			{
				// Increase its count and exit loop.
				LevelResourceStat.Count++;
				bFoundExisting = TRUE;
				break;
			}
		}
		// No existing entry has been found, add new one.
		if( !bFoundExisting )
		{
			LevelResourceStats.AddItem( FLevelResourceStat( LevelPackage ) );
		}
		// Increase total count.
		TotalCount++;
	}
};
/** Compare function used by sort. Sorts in descending order. */
IMPLEMENT_COMPARE_CONSTREF( FResourceStat, UnPackageUtilities, { return B.TotalCount - A.TotalCount; } );

/**
* Class encapsulating stats functionality.
*/
class FResourceStatContainer
{
public:
	/**
	* Constructor
	*
	* @param	InDescription	Description used for dumping stats.
	*/
	FResourceStatContainer( const TCHAR* InDescription )
		:	Description( InDescription )
	{}

	/** 
	* Function called when a resource is encountered in a level to increment count.
	*
	* @param	Resource		Encountered resource.
	* @param	LevelPackage	Level package resource was encountered in.
	*/
	void EncounteredResource( UObject* Resource, UObject* LevelPackage )
	{
		FResourceStat* ResourceStatPtr = ResourceToStatMap.Find( Resource );
		// Resource has existing stat associated.
		if( ResourceStatPtr != NULL )
		{
			ResourceStatPtr->IncrementUsage( LevelPackage );
		}
		// Associate resource with new stat.
		else
		{
			FResourceStat ResourceStat( Resource, LevelPackage );
			ResourceToStatMap.Set( Resource, ResourceStat );
		}
	}

	/**
	* Dumps all the stats information sorted to the log.
	*/
	void DumpStats()
	{
		// Copy TMap data into TArray so it can be sorted.
		TArray<FResourceStat> SortedList;
		for( TMap<UObject*,FResourceStat>::TIterator It(ResourceToStatMap); It; ++It )
		{
			SortedList.AddItem( It.Value() );
		}
		// Sort the list in descending order by total count.
		Sort<USE_COMPARE_CONSTREF(FResourceStat,UnPackageUtilities)>( SortedList.GetTypedData(), SortedList.Num() );

		warnf( NAME_Log, TEXT("") ); 
		warnf( NAME_Log, TEXT("") ); 
		warnf( NAME_Log, TEXT("Stats for %s."), *Description ); 
		warnf( NAME_Log, TEXT("") ); 

		// Iterate over all entries and dump info.
		for( INT i=0; i<SortedList.Num(); i++ )
		{
			const FResourceStat& ResourceStat = SortedList(i);
			warnf( NAME_Log, TEXT("%4i use%s%4i level%s for%s   %s"), 
				ResourceStat.TotalCount,
				ResourceStat.TotalCount > 1 ? TEXT("s in") : TEXT(" in "), 
				ResourceStat.LevelResourceStats.Num(), 
				ResourceStat.LevelResourceStats.Num() > 1 ? TEXT("s") : TEXT(""),
				ResourceStat.LevelResourceStats.Num() > 1 ? TEXT("") : TEXT(" "),
				*ResourceStat.Resource->GetFullName() );

			for( INT LevelIndex=0; LevelIndex<ResourceStat.LevelResourceStats.Num(); LevelIndex++ )
			{
				const FLevelResourceStat& LevelResourceStat = ResourceStat.LevelResourceStats(LevelIndex);
				warnf( NAME_Log, TEXT("    %4i use%s: %s"), 
					LevelResourceStat.Count, 
					LevelResourceStat.Count > 1 ? TEXT("s in") : TEXT("  in"), 
					*LevelResourceStat.LevelPackage->GetName() );
			}
		}
	}
private:
	/** Map from resource to stat helper structure. */
	TMap<UObject*,FResourceStat>	ResourceToStatMap;
	/** Description used for dumping stats.			*/
	FString							Description;
};

void UAnalyzeContentCommandlet::StaticInitialize()
{
	ShowErrorCount = FALSE;
}

INT UAnalyzeContentCommandlet::Main( const FString& Params )
{
	// Parse command line.
	TArray<FString> Tokens;
	TArray<FString> Switches;

	const TCHAR* Parms = *Params;
	ParseCommandLine(Parms, Tokens, Switches);

	// Retrieve all package file names and iterate over them, comparing them to tokens.
	TArray<FString> PackageFileList = GPackageFileCache->GetPackageFileList();		
	for( INT PackageIndex=0; PackageIndex<PackageFileList.Num(); PackageIndex++ )
	{
		// Tokens on the command line are package names.
		for( INT TokenIndex=0; TokenIndex<Tokens.Num(); TokenIndex++ )
		{
			// Compare the two and see whether we want to include this package in the analysis.
			FFilename PackageName = PackageFileList( PackageIndex );
			if( Tokens(TokenIndex) == PackageName.GetBaseFilename() )
			{
				UPackage* Package = UObject::LoadPackage( NULL, *PackageName, LOAD_None );
				if( Package != NULL )
				{
					warnf( NAME_Log, TEXT("Loading %s"), *PackageName );
					// Find the world and load all referenced levels.
					UWorld* World = FindObjectChecked<UWorld>( Package, TEXT("TheWorld") );
					if( World )
					{
						AWorldInfo* WorldInfo	= World->GetWorldInfo();
						// Iterate over streaming level objects loading the levels.
						for( INT LevelIndex=0; LevelIndex<WorldInfo->StreamingLevels.Num(); LevelIndex++ )
						{
							ULevelStreaming* StreamingLevel = WorldInfo->StreamingLevels(LevelIndex);
							if( StreamingLevel )
							{
								// Load package if found.
								FString Filename;
								if( GPackageFileCache->FindPackageFile( *StreamingLevel->PackageName.ToString(), NULL, Filename ) )
								{
									warnf(NAME_Log, TEXT("Loading sub-level %s"), *Filename);
									LoadPackage( NULL, *Filename, LOAD_None );
								}
							}
						}
					}
				}
				else
				{
					warnf( NAME_Error, TEXT("Error loading %s!"), *PackageName );
				}
			}
		}
	}

	// By now all objects are in memory.

	FResourceStatContainer StaticMeshMaterialStats( TEXT("materials applied to static meshes") );
	FResourceStatContainer StaticMeshStats( TEXT("static meshes placed in levels") );
	FResourceStatContainer BSPMaterialStats( TEXT("materials applied to BSP surfaces") );

	// Iterate over all static mesh components and add their materials and static meshes.
	for( TObjectIterator<UStaticMeshComponent> It; It; ++It )
	{
		UStaticMeshComponent*	StaticMeshComponent = *It;
		UPackage*				LevelPackage		= StaticMeshComponent->GetOutermost();

		// Only add if the outer is a map package.
		if( LevelPackage->ContainsMap() )
		{
			if( StaticMeshComponent->StaticMesh && StaticMeshComponent->StaticMesh->LODModels.Num() )
			{
				// Populate materials array, avoiding duplicate entries.
				TArray<UMaterial*> Materials;
				INT MaterialCount = StaticMeshComponent->StaticMesh->LODModels(0).Elements.Num();
				for( INT MaterialIndex=0; MaterialIndex<MaterialCount; MaterialIndex++ )
				{
					UMaterialInterface* MaterialInterface = StaticMeshComponent->GetMaterial( MaterialIndex );
					if( MaterialInterface && MaterialInterface->GetMaterial() )
					{
						Materials.AddUniqueItem( MaterialInterface->GetMaterial() );
					}
				}

				// Iterate over materials and create/ update associated stats.
				for( INT MaterialIndex=0; MaterialIndex<Materials.Num(); MaterialIndex++ )
				{
					UMaterial* Material = Materials(MaterialIndex);
					// Track materials applied to static meshes.			
					StaticMeshMaterialStats.EncounteredResource( Material, LevelPackage );
				}
			}

			// Track static meshes used by static mesh components.
			if( StaticMeshComponent->StaticMesh )
			{
				StaticMeshStats.EncounteredResource( StaticMeshComponent->StaticMesh, LevelPackage );
			}
		}
	}

	for( TObjectIterator<ABrush> It; It; ++It )
	{
		ABrush*		BrushActor		= *It;
		UPackage*	LevelPackage	= BrushActor->GetOutermost();

		// Only add if the outer is a map package.
		if( LevelPackage->ContainsMap() )
		{
			if( BrushActor->Brush && BrushActor->Brush->Polys )
			{
				UPolys* Polys = BrushActor->Brush->Polys;

				// Populate materials array, avoiding duplicate entries.
				TArray<UMaterial*> Materials;
				for( INT ElementIndex=0; ElementIndex<Polys->Element.Num(); ElementIndex++ )
				{
					const FPoly& Poly = Polys->Element(ElementIndex);
					if( Poly.Material && Poly.Material->GetMaterial() )
					{
						Materials.AddUniqueItem( Poly.Material->GetMaterial() );
					}
				}

				// Iterate over materials and create/ update associated stats.
				for( INT MaterialIndex=0; MaterialIndex<Materials.Num(); MaterialIndex++ )
				{
					UMaterial* Material = Materials(MaterialIndex);
					// Track materials applied to BSP.
					BSPMaterialStats.EncounteredResource( Material, LevelPackage );
				}
			}
		}
	}

	// Dump stat summaries.
	StaticMeshMaterialStats.DumpStats();
	StaticMeshStats.DumpStats();
	BSPMaterialStats.DumpStats();

	return 0;
}
IMPLEMENT_CLASS(UAnalyzeContentCommandlet)


/*-----------------------------------------------------------------------------
UAnalyzeReferencedContentCommandlet
-----------------------------------------------------------------------------*/

/** Constructor, initializing all members. */
UAnalyzeReferencedContentCommandlet::FStaticMeshStats::FStaticMeshStats( UStaticMesh* StaticMesh )
:	ResourceType(StaticMesh->GetClass()->GetName())
,	ResourceName(StaticMesh->GetPathName())
,	NumInstances(0)
,	NumTriangles(0)
,	NumSections(0)
,	bIsReferencedByScript(FALSE)
,	NumMapsUsedIn(0)
,	ResourceSize(StaticMesh->GetResourceSize())
{
	// Update triangle and section counts.
	for( INT ElementIndex=0; ElementIndex<StaticMesh->LODModels(0).Elements.Num(); ElementIndex++ )
	{
		const FStaticMeshElement& StaticMeshElement = StaticMesh->LODModels(0).Elements(ElementIndex);
		NumTriangles += StaticMeshElement.NumTriangles;
		NumSections++;
	}

	NumPrimitives = 0;
	if(StaticMesh->BodySetup)
	{
		NumPrimitives = StaticMesh->BodySetup->AggGeom.ConvexElems.Num();
	}
}

/**
* Stringifies gathered stats in CSV format.
*
* @return comma separated list of stats
*/
FString UAnalyzeReferencedContentCommandlet::FStaticMeshStats::ToCSV() const
{
	return FString::Printf(TEXT("%s,%s,%i,%i,%i,%i,%i,%i,%d%s"),
		*ResourceType,
		*ResourceName,
		NumInstances,
		NumTriangles,
		NumSections,
		NumPrimitives,
		NumMapsUsedIn,
		ResourceSize,
		UsedAtScales.Num(),
		LINE_TERMINATOR);
}

/**
* Returns a header row for CSV
*
* @return comma separated header row
*/
FString UAnalyzeReferencedContentCommandlet::FStaticMeshStats::GetCSVHeaderRow()
{
	return TEXT("ResourceType,ResourceName,NumInstances,NumTriangles,NumSections,CollisionPrims,NumMapsUsedIn,ResourceSize,ScalesUsed") LINE_TERMINATOR;
}


/** Constructor, initializing all members */
UAnalyzeReferencedContentCommandlet::FTextureStats::FTextureStats( UTexture* Texture )
:	ResourceType(Texture->GetClass()->GetName())
,	ResourceName(Texture->GetPathName())
,	bIsReferencedByScript(FALSE)
,	NumMapsUsedIn(0)
,	ResourceSize(Texture->GetResourceSize())
,	LODBias(Texture->LODBias)
,	LODGroup(Texture->LODGroup)
,	Format(TEXT("UNKOWN"))
{
	// Update format.
	UTexture2D* Texture2D = Cast<UTexture2D>(Texture);
	if( Texture2D )
	{
		Format = GPixelFormats[Texture2D->Format].Name;
	}
}

/**
* Stringifies gathered stats in CSV format.
*
* @return comma separated list of stats
*/
FString UAnalyzeReferencedContentCommandlet::FTextureStats::ToCSV() const
{
	return FString::Printf(TEXT("%s,%s,%i,%i,%i,%i,%i,%i,%s%s"),
		*ResourceType,
		*ResourceName,						
		MaterialsUsedBy.Num(),
		bIsReferencedByScript,
		NumMapsUsedIn,
		ResourceSize,
		LODBias,
		LODGroup,
		*Format,
		LINE_TERMINATOR);
}

/**
* Returns a header row for CSV
*
* @return comma separated header row
*/
FString UAnalyzeReferencedContentCommandlet::FTextureStats::GetCSVHeaderRow()
{
	return TEXT("ResourceType,ResourceName,NumMaterialsUsedBy,ScriptReferenced,NumMapsUsedIn,ResourceSize,LODBias,LODGroup,Format") LINE_TERMINATOR;
}

/**
* Static helper to return instructions used by shader type.
*
* @param	MeshShaderMap	Shader map to use to find shader of passed in type
* @param	ShaderType		Type of shader to query instruction count for
* @return	Instruction count if found, 0 otherwise
*/
static INT GetNumInstructionsForShaderType( const FMeshMaterialShaderMap* MeshShaderMap, FShaderType* ShaderType )
{
	INT NumInstructions = 0;
	const FShader* Shader = MeshShaderMap->GetShader(ShaderType);
	if( Shader )
	{
		NumInstructions = Shader->GetNumInstructions();
	}
	return NumInstructions;
}

/** Constructor, initializing all members */
UAnalyzeReferencedContentCommandlet::FMaterialStats::FMaterialStats( UMaterial* Material, UAnalyzeReferencedContentCommandlet* Commandlet )
:	ResourceType(Material->GetClass()->GetName())
,	ResourceName(Material->GetPathName())
,	NumBrushesAppliedTo(0)
,	NumStaticMeshInstancesAppliedTo(0)
,	bIsReferencedByScript(FALSE)
,	NumMapsUsedIn(0)
,	ResourceSizeOfReferencedTextures(0)
{
	// Keep track of unique textures and texture sample count.
	TArray<UTexture*> UniqueTextures;
	TArray<UTexture*> SampledTextures;
	Material->GetTextures( UniqueTextures );
	Material->GetTextures( SampledTextures, FALSE );

	// Update texture samplers count.
	NumTextureSamples = SampledTextures.Num();

	// Update dependency chain stats.
	check( Material->MaterialResources[MSP_SM3]);
	MaxTextureDependencyLength = Material->MaterialResources[MSP_SM3]->GetMaxTextureDependencyLength();

	// Update instruction counts.
	const FMaterialShaderMap* MaterialShaderMap = Material->MaterialResources[MSP_SM3]->GetShaderMap();
	if(MaterialShaderMap)
	{
		// Use the local vertex factory shaders.
		const FMeshMaterialShaderMap* MeshShaderMap = MaterialShaderMap->GetMeshShaderMap(&FLocalVertexFactory::StaticType);
		check(MeshShaderMap);

		// Get intrustion counts.
		NumInstructionsBasePassNoLightmap		= GetNumInstructionsForShaderType( MeshShaderMap, Commandlet->ShaderTypeBasePassNoLightmap	);
		NumInstructionsBasePassAndLightmap		= GetNumInstructionsForShaderType( MeshShaderMap, Commandlet->ShaderTypeBasePassAndLightmap );
		NumInstructionsPointLightWithShadowMap	= GetNumInstructionsForShaderType( MeshShaderMap, Commandlet->ShaderTypePointLightWithShadowMap );
	}

	// Iterate over unique texture refs and update resource size.
	for( INT TextureIndex=0; TextureIndex<UniqueTextures.Num(); TextureIndex++ )
	{
		UTexture* Texture = UniqueTextures(TextureIndex);
		ResourceSizeOfReferencedTextures += Texture->GetResourceSize();
		TexturesUsed.AddItem( Texture->GetFullName() );
	}
}

/**
* Stringifies gathered stats in CSV format.
*
* @return comma separated list of stats
*/
FString UAnalyzeReferencedContentCommandlet::FMaterialStats::ToCSV() const
{
	return FString::Printf(TEXT("%s,%s,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i%s"),
		*ResourceType,
		*ResourceName,
		NumBrushesAppliedTo,
		NumStaticMeshInstancesAppliedTo,
		StaticMeshesAppliedTo.Num(),
		bIsReferencedByScript,
		NumMapsUsedIn,
		TexturesUsed.Num(),
		NumTextureSamples,
		MaxTextureDependencyLength,
		NumInstructionsBasePassNoLightmap,
		NumInstructionsBasePassAndLightmap,
		NumInstructionsPointLightWithShadowMap,
		ResourceSizeOfReferencedTextures,
		LINE_TERMINATOR);
}

/**
* Returns a header row for CSV
*
* @return comma separated header row
*/
FString UAnalyzeReferencedContentCommandlet::FMaterialStats::GetCSVHeaderRow()
{
	return TEXT("ResourceType,ResourceName,NumBrushesAppliedTo,NumStaticMeshInstancesAppliedTo,NumStaticMeshesAppliedTo,ScriptReferenced,NumMapsUsedIn,NumTextures,NumTextureSamples,MaxTextureDependencyLength,BasePass,BasePassLightmap,PointLightShadowMap,ResourceSizeOfReferencedTextures") LINE_TERMINATOR;
}

/** Constructor, initializing all members */
UAnalyzeReferencedContentCommandlet::FParticleStats::FParticleStats( UParticleSystem* ParticleSystem )
:	ResourceType(ParticleSystem->GetClass()->GetName())
,	ResourceName(ParticleSystem->GetPathName())
,	bIsReferencedByScript(FALSE)
,	NumMapsUsedIn(0)
,	NumEmitters(0)
,	NumModules(0)
,	NumPeakActiveParticles(0)
{
	// Iterate over all sub- emitters and update stats.
	for( INT EmitterIndex=0; EmitterIndex<ParticleSystem->Emitters.Num(); EmitterIndex++ )
	{
		UParticleEmitter* ParticleEmitter = ParticleSystem->Emitters(EmitterIndex);
		if( ParticleEmitter )
		{
			NumEmitters++;
			NumModules				+= ParticleEmitter->Modules.Num();
			// Get peak active particles from LOD 0.
			UParticleLODLevel* LODLevel = ParticleEmitter->GetLODLevel(0);
			if( LODLevel )
			{	
				NumPeakActiveParticles += LODLevel->PeakActiveParticles;
			}
		}
	}
}

/**
* Stringifies gathered stats in CSV format.
*
* @return comma separated list of stats
*/
FString UAnalyzeReferencedContentCommandlet::FParticleStats::ToCSV() const
{
	return FString::Printf(TEXT("%s,%s,%i,%i,%i,%i,%i%s"),
		*ResourceType,
		*ResourceName,	
		bIsReferencedByScript,
		NumMapsUsedIn,
		NumEmitters,
		NumModules,
		NumPeakActiveParticles,
		LINE_TERMINATOR);
}




/**
* Returns a header row for CSV
*
* @return comma separated header row
*/
FString UAnalyzeReferencedContentCommandlet::FParticleStats::GetCSVHeaderRow()
{
	return TEXT("ResourceType,ResourceName,ScriptReferenced,NumMapsUsedIn,NumEmitters,NumModules,NumPeakActiveParticles") LINE_TERMINATOR;
}

/** Constructor, initializing all members */
UAnalyzeReferencedContentCommandlet::FAnimSequenceStats::FAnimSequenceStats( UAnimSequence* Sequence )
:	ResourceType(Sequence->GetClass()->GetName())
,	ResourceName(Sequence->GetPathName())
,	bIsReferencedByScript(FALSE)
,	NumMapsUsedIn(0)
,	CompressionScheme(FString(TEXT("")))
,	TranslationFormat(ACF_None)
,	RotationFormat(ACF_None)
,	AnimationSize(0)
{
	// The sequence object name is not very useful - strip and add the friendly name.
	FString Left, Right;
	ResourceName.Split(TEXT("."), &Left, &Right, TRUE);
	ResourceName = Left + TEXT(".") + Sequence->SequenceName.ToString();

	if(Sequence->CompressionScheme)
	{
		CompressionScheme = Sequence->CompressionScheme->GetClass()->GetName();
	}

	TranslationFormat = static_cast<AnimationCompressionFormat>(Sequence->TranslationCompressionFormat);
	RotationFormat = static_cast<AnimationCompressionFormat>(Sequence->RotationCompressionFormat);
	AnimationSize = Sequence->GetResourceSize();
}


static FString GetCompressionFormatString(AnimationCompressionFormat InFormat)
{
	switch(InFormat)
	{
	case ACF_None:
		return FString(TEXT("ACF_None"));
	case ACF_Float96NoW:
		return FString(TEXT("ACF_Float96NoW"));
	case ACF_Fixed48NoW:
		return FString(TEXT("ACF_Fixed48NoW"));
	case ACF_IntervalFixed32NoW:
		return FString(TEXT("ACF_IntervalFixed32NoW"));
	case ACF_Fixed32NoW:
		return FString(TEXT("ACF_Fixed32NoW"));
	case ACF_Float32NoW:
		return FString(TEXT("ACF_Float32NoW"));
	default:
		warnf( TEXT("AnimationCompressionFormat was not found:  %i"), static_cast<INT>(InFormat) );
	}

	return FString(TEXT("Unknown"));
}

/**
* Stringifies gathered stats in CSV format.
*
* @return comma separated list of stats
*/
FString UAnalyzeReferencedContentCommandlet::FAnimSequenceStats::ToCSV() const
{
	return FString::Printf(TEXT("%s,%s,%i,%i,%s,%s,%s,%i%s"),
		*ResourceType,
		*ResourceName,	
		bIsReferencedByScript,
		NumMapsUsedIn,
		*GetCompressionFormatString(TranslationFormat),
		*GetCompressionFormatString(RotationFormat),
		*CompressionScheme,
		AnimationSize,
		LINE_TERMINATOR);
}




/**
* Returns a header row for CSV
*
* @return comma separated header row
*/
FString UAnalyzeReferencedContentCommandlet::FAnimSequenceStats::GetCSVHeaderRow()
{
	return TEXT("ResourceType,ResourceName,ScriptReferenced,NumMapsUsedIn,TransFormat,RotFormat,CompressionScheme,AnimSize") LINE_TERMINATOR;
}

/**
* Retrieves/ creates material stats associated with passed in material.
*
* @warning: returns pointer into TMap, only valid till next time Set is called
*
* @param	Material	Material to retrieve/ create material stats for
* @return	pointer to material stats associated with material
*/
UAnalyzeReferencedContentCommandlet::FMaterialStats* UAnalyzeReferencedContentCommandlet::GetMaterialStats( UMaterial* Material )
{
	UAnalyzeReferencedContentCommandlet::FMaterialStats* MaterialStats = ResourceNameToMaterialStats.Find( *Material->GetFullName() );
	if( MaterialStats == NULL )
	{
		MaterialStats =	&ResourceNameToMaterialStats.Set( *Material->GetFullName(), UAnalyzeReferencedContentCommandlet::FMaterialStats( Material, this ) );
	}
	return MaterialStats;
}

/**
* Retrieves/ creates texture stats associated with passed in texture.
*
* @warning: returns pointer into TMap, only valid till next time Set is called
*
* @param	Texture		Texture to retrieve/ create texture stats for
* @return	pointer to texture stats associated with texture
*/
UAnalyzeReferencedContentCommandlet::FTextureStats* UAnalyzeReferencedContentCommandlet::GetTextureStats( UTexture* Texture )
{
	UAnalyzeReferencedContentCommandlet::FTextureStats* TextureStats = ResourceNameToTextureStats.Find( *Texture->GetFullName() );
	if( TextureStats == NULL )
	{
		TextureStats = &ResourceNameToTextureStats.Set( *Texture->GetFullName(), UAnalyzeReferencedContentCommandlet::FTextureStats( Texture ) );
	}
	return TextureStats;
}

/**
* Retrieves/ creates static mesh stats associated with passed in static mesh.
*
* @warning: returns pointer into TMap, only valid till next time Set is called
*
* @param	StaticMesh	Static mesh to retrieve/ create static mesh stats for
* @return	pointer to static mesh stats associated with static mesh
*/
UAnalyzeReferencedContentCommandlet::FStaticMeshStats* UAnalyzeReferencedContentCommandlet::GetStaticMeshStats( UStaticMesh* StaticMesh )
{
	UAnalyzeReferencedContentCommandlet::FStaticMeshStats* StaticMeshStats = ResourceNameToStaticMeshStats.Find( *StaticMesh->GetFullName() );

	if( StaticMeshStats == NULL )
	{
		StaticMeshStats = &ResourceNameToStaticMeshStats.Set( *StaticMesh->GetFullName(), UAnalyzeReferencedContentCommandlet::FStaticMeshStats( StaticMesh ) );
	}

	return StaticMeshStats;
}

/**
* Retrieves/ creates particle stats associated with passed in particle system.
*
* @warning: returns pointer into TMap, only valid till next time Set is called
*
* @param	ParticleSystem	Particle system to retrieve/ create static mesh stats for
* @return	pointer to particle system stats associated with static mesh
*/
UAnalyzeReferencedContentCommandlet::FParticleStats* UAnalyzeReferencedContentCommandlet::GetParticleStats( UParticleSystem* ParticleSystem )
{
	UAnalyzeReferencedContentCommandlet::FParticleStats* ParticleStats = ResourceNameToParticleStats.Find( *ParticleSystem->GetFullName() );
	if( ParticleStats == NULL )
	{
		ParticleStats = &ResourceNameToParticleStats.Set( *ParticleSystem->GetFullName(), UAnalyzeReferencedContentCommandlet::FParticleStats( ParticleSystem ) );
	}
	return ParticleStats;
}

/**
* Retrieves/ creates animation sequence stats associated with passed in animation sequence.
*
* @warning: returns pointer into TMap, only valid till next time Set is called
*
* @param	AnimSequence	Anim sequence to retrieve/ create anim sequence stats for
* @return	pointer to particle system stats associated with anim sequence
*/
UAnalyzeReferencedContentCommandlet::FAnimSequenceStats* UAnalyzeReferencedContentCommandlet::GetAnimSequenceStats( UAnimSequence* AnimSequence )
{
	UAnalyzeReferencedContentCommandlet::FAnimSequenceStats* AnimStats = ResourceNameToAnimStats.Find( *AnimSequence->GetFullName() );
	if( AnimStats == NULL )
	{
		AnimStats = &ResourceNameToAnimStats.Set( *AnimSequence->GetFullName(), UAnalyzeReferencedContentCommandlet::FAnimSequenceStats( AnimSequence ) );
	}
	return AnimStats;
}

void UAnalyzeReferencedContentCommandlet::StaticInitialize()
{
	ShowErrorCount = FALSE;
}

/**
* Handles encountered object, routing to various sub handlers.
*
* @param	Object			Object to handle
* @param	LevelPackage	Currently loaded level package, can be NULL if not a level
* @param	bIsScriptReferenced Whether object is handled because there is a script reference
*/
void UAnalyzeReferencedContentCommandlet::HandleObject( UObject* Object, UPackage* LevelPackage, UBOOL bIsScriptReferenced )
{
	// Disregard marked objects as they won't go away with GC.
	if( !Object->HasAnyFlags( RF_Marked ) )
	{
		// Whether the object is the passed in level package if it is != NULL.
		const UBOOL bIsInALevelPackage = LevelPackage && Object->IsIn( LevelPackage );

		if( Object->IsA(UParticleSystemComponent::StaticClass()) && bIsInALevelPackage )
		{
			HandleStaticMeshOnAParticleSystemComponent( (UParticleSystemComponent*) Object, LevelPackage, bIsScriptReferenced );
		}
		// Handle static mesh.
		else if( Object->IsA(UStaticMesh::StaticClass()) )
		{
			HandleStaticMesh( (UStaticMesh*) Object, LevelPackage, bIsScriptReferenced );
		}
		// Handles static mesh component if it's residing in the map package. LevelPackage == NULL for non map packages.
		else if( Object->IsA(UStaticMeshComponent::StaticClass()) && bIsInALevelPackage )
		{
			HandleStaticMeshComponent( (UStaticMeshComponent*) Object, LevelPackage, bIsScriptReferenced );
		}
		// Handle material.
		else if( Object->IsA(UMaterial::StaticClass()) )
		{
			HandleMaterial( (UMaterial*) Object, LevelPackage, bIsScriptReferenced );
		}
		// Handle texture.
		else if( Object->IsA(UTexture::StaticClass()) )
		{
			HandleTexture( (UTexture*) Object, LevelPackage, bIsScriptReferenced );
		}
		// Handles brush actor if it's residing in the map package. LevelPackage == NULL for non map packages.
		else if( Object->IsA(ABrush::StaticClass()) && bIsInALevelPackage )
		{
			HandleBrush( (ABrush*) Object, LevelPackage, bIsScriptReferenced );
		}
		// Handle particle system.
		else if( Object->IsA(UParticleSystem::StaticClass()) )
		{
			HandleParticleSystem( (UParticleSystem*) Object, LevelPackage, bIsScriptReferenced );
		}
		// Handle anim sequence.
		else if( Object->IsA(UAnimSequence::StaticClass()) )
		{
			HandleAnimSequence( (UAnimSequence*) Object, LevelPackage, bIsScriptReferenced );
		}
		// Handle level
		else if( Object->IsA(ULevel::StaticClass()) )
		{
			HandleLevel( (ULevel*)Object, LevelPackage, bIsScriptReferenced );
		}
	}
}

/**
* Handles gathering stats for passed in static mesh.
*
* @param StaticMesh	StaticMesh to gather stats for.
* @param LevelPackage	Currently loaded level package, can be NULL if not a level
* @param bIsScriptReferenced Whether object is handled because there is a script reference
*/
void UAnalyzeReferencedContentCommandlet::HandleStaticMesh( UStaticMesh* StaticMesh, UPackage* LevelPackage, UBOOL bIsScriptReferenced  )
{
	UAnalyzeReferencedContentCommandlet::FStaticMeshStats* StaticMeshStats = GetStaticMeshStats( StaticMesh );

	if( LevelPackage )
	{
		StaticMeshStats->NumMapsUsedIn++;
	}

	if( bIsScriptReferenced )
	{
		StaticMeshStats->bIsReferencedByScript = TRUE;
	}

	// Populate materials array, avoiding duplicate entries.
	TArray<UMaterial*> Materials;
	// @todo need to do foreach over all LODModels
	INT MaterialCount = StaticMesh->LODModels(0).Elements.Num();
	for( INT MaterialIndex=0; MaterialIndex<MaterialCount; MaterialIndex++ )
	{
		UMaterialInterface* MaterialInterface = StaticMesh->LODModels(0).Elements(MaterialIndex).Material;
		if( MaterialInterface && MaterialInterface->GetMaterial() )
		{
			Materials.AddUniqueItem( MaterialInterface->GetMaterial() );
		}
	}

	// Iterate over materials and create/ update associated stats.
	for( INT MaterialIndex=0; MaterialIndex<Materials.Num(); MaterialIndex++ )
	{
		UMaterial* Material	= Materials(MaterialIndex);	
		UAnalyzeReferencedContentCommandlet::FMaterialStats* MaterialStats = GetMaterialStats( Material );
		MaterialStats->StaticMeshesAppliedTo.Set( *StaticMesh->GetFullName(), TRUE );
	}
}

/**
* Handles gathering stats for passed in static mesh component.
*
* @param StaticMeshComponent	StaticMeshComponent to gather stats for
* @param LevelPackage	Currently loaded level package, can be NULL if not a level
* @param bIsScriptReferenced Whether object is handled because there is a script reference
*/
void UAnalyzeReferencedContentCommandlet::HandleStaticMeshComponent( UStaticMeshComponent* StaticMeshComponent, UPackage* LevelPackage, UBOOL bIsScriptReferenced )
{
	if( StaticMeshComponent->StaticMesh && StaticMeshComponent->StaticMesh->LODModels.Num() )
	{
		// Populate materials array, avoiding duplicate entries.
		TArray<UMaterial*> Materials;
		INT MaterialCount = StaticMeshComponent->StaticMesh->LODModels(0).Elements.Num();
		for( INT MaterialIndex=0; MaterialIndex<MaterialCount; MaterialIndex++ )
		{
			UMaterialInterface* MaterialInterface = StaticMeshComponent->GetMaterial( MaterialIndex );
			if( MaterialInterface && MaterialInterface->GetMaterial() )
			{
				Materials.AddUniqueItem( MaterialInterface->GetMaterial() );
			}
		}

		// Iterate over materials and create/ update associated stats.
		for( INT MaterialIndex=0; MaterialIndex<Materials.Num(); MaterialIndex++ )
		{
			UMaterial* Material	= Materials(MaterialIndex);	
			UAnalyzeReferencedContentCommandlet::FMaterialStats* MaterialStats = GetMaterialStats( Material );
			MaterialStats->NumStaticMeshInstancesAppliedTo++;
		}

		// Track static meshes used by static mesh components.
		if( StaticMeshComponent->StaticMesh )
		{

			const UBOOL bBelongsToAParticleSystemComponent = StaticMeshComponent->GetOuter()->IsA(UParticleSystemComponent::StaticClass());

			if( bBelongsToAParticleSystemComponent == FALSE )
			{
				UAnalyzeReferencedContentCommandlet::FStaticMeshStats* StaticMeshStats = GetStaticMeshStats( StaticMeshComponent->StaticMesh );
				StaticMeshStats->NumInstances++;

				//warnf( TEXT("HandleStaticMeshComponent SMC: %s   Outter: %s  %d"), *StaticMeshComponent->StaticMesh->GetFullName(), *StaticMeshComponent->GetOuter()->GetFullName(), StaticMeshStats->NumInstances );
			}	
		}
	}
}

/**
 * Handles special case for stats for passed in static mesh component who is part of a ParticleSystemComponent
 *
 * @param ParticleSystemComponent	ParticleSystemComponent to gather stats for
 * @param LevelPackage	Currently loaded level package, can be NULL if not a level
 * @param bIsScriptReferenced Whether object is handled because there is a script reference
 */
void UAnalyzeReferencedContentCommandlet::HandleStaticMeshOnAParticleSystemComponent( UParticleSystemComponent* ParticleSystemComponent, UPackage* LevelPackage, UBOOL bIsScriptReferenced )
{
	UParticleSystemComponent* PSC = ParticleSystemComponent;
	//warnf( TEXT("%d"), PSC->SMComponents.Num() );

	TArray<UStaticMesh*> SMCs;
	for( INT i = 0; i < PSC->SMComponents.Num(); ++i )
	{
		SMCs.AddUniqueItem( PSC->SMComponents(i)->StaticMesh );
	}

	for( INT i = 0; i < SMCs.Num(); ++i )
	{
		//warnf( TEXT("%s"), *SMCs(i)->GetFullName() );
		HandleStaticMesh( SMCs(i), LevelPackage, bIsScriptReferenced );

		UAnalyzeReferencedContentCommandlet::FStaticMeshStats* StaticMeshStats = GetStaticMeshStats( SMCs(i) );
		StaticMeshStats->NumInstances++;
	}
}


/**
* Handles gathering stats for passed in material.
*
* @param Material	Material to gather stats for
* @param LevelPackage	Currently loaded level package, can be NULL if not a level
* @param bIsScriptReferenced Whether object is handled because there is a script reference
*/
void UAnalyzeReferencedContentCommandlet::HandleMaterial( UMaterial* Material, UPackage* LevelPackage, UBOOL bIsScriptReferenced )
{
	UAnalyzeReferencedContentCommandlet::FMaterialStats* MaterialStats = GetMaterialStats( Material );	

	if( LevelPackage )
	{
		MaterialStats->NumMapsUsedIn++;
	}

	if( bIsScriptReferenced )
	{
		MaterialStats->bIsReferencedByScript = TRUE;
	}

	// Array of textures used by this material. No duplicates.
	TArray<UTexture*> TexturesUsed;
	Material->GetTextures(TexturesUsed);

	// Update textures used by this material.
	for( INT TextureIndex=0; TextureIndex<TexturesUsed.Num(); TextureIndex++ )
	{
		UTexture* Texture = TexturesUsed(TextureIndex);
		UAnalyzeReferencedContentCommandlet::FTextureStats* TextureStats = GetTextureStats(Texture);
		TextureStats->MaterialsUsedBy.Set( *Material->GetFullName(), TRUE );
	}
}

/**
* Handles gathering stats for passed in texture.
*
* @paramTexture	Texture to gather stats for
* @param LevelPackage	Currently loaded level package, can be NULL if not a level
* @param bIsScriptReferenced Whether object is handled because there is a script reference
*/
void UAnalyzeReferencedContentCommandlet::HandleTexture( UTexture* Texture, UPackage* LevelPackage, UBOOL bIsScriptReferenced )
{
	UAnalyzeReferencedContentCommandlet::FTextureStats* TextureStats = GetTextureStats( Texture );

	// Only handle further if we have a level package.
	if( LevelPackage )
	{
		TextureStats->NumMapsUsedIn++;
	}

	// Mark as being referenced by script.
	if( bIsScriptReferenced )
	{
		TextureStats->bIsReferencedByScript = TRUE;
	}
}

/**
* Handles gathering stats for passed in brush.
*
* @param BrushActor Brush actor to gather stats for
* @param LevelPackage	Currently loaded level package, can be NULL if not a level
* @param bIsScriptReferenced Whether object is handled because there is a script reference
*/
void UAnalyzeReferencedContentCommandlet::HandleBrush( ABrush* BrushActor, UPackage* LevelPackage, UBOOL bIsScriptReferenced )
{
	if( BrushActor->Brush && BrushActor->Brush->Polys )
	{
		UPolys* Polys = BrushActor->Brush->Polys;

		// Populate materials array, avoiding duplicate entries.
		TArray<UMaterial*> Materials;
		for( INT ElementIndex=0; ElementIndex<Polys->Element.Num(); ElementIndex++ )
		{
			const FPoly& Poly = Polys->Element(ElementIndex);
			if( Poly.Material && Poly.Material->GetMaterial() )
			{
				Materials.AddUniqueItem( Poly.Material->GetMaterial() );
			}
		}

		// Iterate over materials and create/ update associated stats.
		for( INT MaterialIndex=0; MaterialIndex<Materials.Num(); MaterialIndex++ )
		{
			UMaterial* Material = Materials(MaterialIndex);
			UAnalyzeReferencedContentCommandlet::FMaterialStats* MaterialStats = GetMaterialStats( Material );
			MaterialStats->NumBrushesAppliedTo++;
		}
	}
}

/**
* Handles gathering stats for passed in particle system.
*
* @param ParticleSystem	Particle system to gather stats for
* @param LevelPackage		Currently loaded level package, can be NULL if not a level
* @param bIsScriptReferenced Whether object is handled because there is a script reference
*/
void UAnalyzeReferencedContentCommandlet::HandleParticleSystem( UParticleSystem* ParticleSystem, UPackage* LevelPackage, UBOOL bIsScriptReferenced )
{
	UAnalyzeReferencedContentCommandlet::FParticleStats* ParticleStats = GetParticleStats( ParticleSystem );

	// Only handle further if we have a level package.
	if( LevelPackage )
	{
		ParticleStats->NumMapsUsedIn++;
	}

	// Mark object as being referenced by script.
	if( bIsScriptReferenced )
	{
		ParticleStats->bIsReferencedByScript = TRUE;
	}
}

/**
* Handles gathering stats for passed in animation sequence.
*
* @param AnimSequence		AnimSequence to gather stats for
* @param LevelPackage		Currently loaded level package, can be NULL if not a level
* @param bIsScriptReferenced Whether object is handled because there is a script reference
*/
void UAnalyzeReferencedContentCommandlet::HandleAnimSequence( UAnimSequence* AnimSequence, UPackage* LevelPackage, UBOOL bIsScriptReferenced )
{
	UAnalyzeReferencedContentCommandlet::FAnimSequenceStats* AnimStats = GetAnimSequenceStats( AnimSequence );

	// Only handle further if we have a level package.
	if( LevelPackage )
	{
		AnimStats->NumMapsUsedIn++;
	}

	// Mark object as being referenced by script.
	if( bIsScriptReferenced )
	{
		AnimStats->bIsReferencedByScript = TRUE;
	}
}

void UAnalyzeReferencedContentCommandlet::HandleLevel( ULevel* Level, UPackage* LevelPackage, UBOOL bIsScriptReferenced )
{
	for ( TMap<UStaticMesh*, FCachedPhysSMData>::TIterator MeshIt(Level->CachedPhysSMDataMap); MeshIt; ++MeshIt )
	{
		UStaticMesh* Mesh = MeshIt.Key();
		FVector Scale3D = MeshIt.Value().Scale3D;

		if(Mesh)
		{
			UAnalyzeReferencedContentCommandlet::FStaticMeshStats* StaticMeshStats = GetStaticMeshStats( Mesh );

			UBOOL bHaveScale = FALSE;
			for (INT i=0; i < StaticMeshStats->UsedAtScales.Num(); i++)
			{
				// Found a shape with the right scale
				if ((StaticMeshStats->UsedAtScales(i) - Scale3D).IsNearlyZero())
				{
					bHaveScale = TRUE;
					break;
				}
			}

			if(!bHaveScale)
			{
				StaticMeshStats->UsedAtScales.AddItem(Scale3D);
			}
		}
	}
}

INT UAnalyzeReferencedContentCommandlet::Main( const FString& Params )
{
	// Parse command line.
	TArray<FString> Tokens;
	TArray<FString> Switches;

	const TCHAR* Parms = *Params;
	ParseCommandLine(Parms, Tokens, Switches);

	// Whether to only deal with map files.
	const UBOOL bShouldOnlyLoadMaps	= Switches.FindItemIndex(TEXT("MAPSONLY")) != INDEX_NONE;
	// Whether to exclude script references.
	const UBOOL bExcludeScript		= Switches.FindItemIndex(TEXT("EXCLUDESCRIPT")) != INDEX_NONE;
    // Whether to load non native script packages (e.g. useful for seeing what will always be loaded)
	const UBOOL bExcludeNonNativeScript = Switches.FindItemIndex(TEXT("EXCLUDENONNATIVESCRIPT")) != INDEX_NONE;


	if( bExcludeNonNativeScript == FALSE )
	{
		// Load up all script files in EditPackages.
		const UEditorEngine* EditorEngine = CastChecked<UEditorEngine>(GEngine);
		for( INT i=0; i<EditorEngine->EditPackages.Num(); i++ )
		{
			LoadPackage( NULL, *EditorEngine->EditPackages(i), LOAD_NoWarn );
		}
	}

	// Mark loaded objects as they are part of the always loaded set and are not taken into account for stats.
	for( TObjectIterator<UObject> It; It; ++It )
	{
		UObject* Object = *It;
		// Script referenced asset.
		if( !bExcludeScript )
		{
			HandleObject( Object, NULL, TRUE );
		}
		// Mark object as always loaded so it doesn't get counted multiple times.
		Object->SetFlags( RF_Marked );
	}

	TArray<FString> FileList;
	
	// Build package file list from passed in command line if tokens are specified.
	if( Tokens.Num() )
	{
		for( INT TokenIndex=0; TokenIndex<Tokens.Num(); TokenIndex++ )
		{
			// Lookup token in file cache and add filename if found.
			FString OutFilename;
			if( GPackageFileCache->FindPackageFile( *Tokens(TokenIndex), NULL, OutFilename ) )
			{
				new(FileList)FString(OutFilename);
			}
		}
	}
	// Or use all files otherwise.
	else
	{
		FileList = GPackageFileCache->GetPackageFileList();
	}
	
	if( FileList.Num() == 0 )
	{
		warnf( NAME_Warning, TEXT("No packages found") );
		return 1;
	}

	// Find shader types.
	ShaderTypeBasePassNoLightmap		= FindShaderTypeByName(TEXT("TBasePassPixelShaderFNoLightMapPolicyNoSkyLight"));
	ShaderTypeBasePassAndLightmap		= FindShaderTypeByName(TEXT("TBasePassPixelShaderFDirectionalVertexLightMapPolicyNoSkyLight"));
	ShaderTypePointLightWithShadowMap	= FindShaderTypeByName(TEXT("TLightPixelShaderFPointLightPolicyFShadowTexturePolicy"));

	check( ShaderTypeBasePassNoLightmap	);
	check( ShaderTypeBasePassAndLightmap );
	check( ShaderTypePointLightWithShadowMap );

	// Iterate over all files, loading up ones that have the map extension..
	for( INT FileIndex=0; FileIndex<FileList.Num(); FileIndex++ )
	{
		const FFilename& Filename = FileList(FileIndex);		

		// Disregard filenames that don't have the map extension if we're in MAPSONLY mode.
		if( bShouldOnlyLoadMaps && (Filename.GetExtension() != FURL::DefaultMapExt) )
		{
			continue;
		}

		// Skip filenames with the script extension. @todo: don't hardcode .u as the script file extension
		if( (Filename.GetExtension() == TEXT("u")) )
		{
			continue;
		}

		warnf( NAME_Log, TEXT("Loading %s"), *Filename );
		UPackage* Package = UObject::LoadPackage( NULL, *Filename, LOAD_None );
		if( Package == NULL )
		{
			warnf( NAME_Error, TEXT("Error loading %s!"), *Filename );
		}
		else
		{
			// Figure out whether package is a map or content package.
			UBOOL bIsAMapPackage = FindObject<UWorld>(Package, TEXT("TheWorld")) != NULL;

			// Handle currently loaded objects.
			for( TObjectIterator<UObject> It; It; ++It )
			{
				UObject* Object = *It;
				HandleObject( Object, bIsAMapPackage ? Package : NULL, FALSE );
			}
		}

		// Collect garbage, going back to a clean slate.
		UObject::CollectGarbage( RF_Native );

		// Verify that everything we cared about got cleaned up correctly.
		UBOOL bEncounteredUnmarkedObject = FALSE;
		for( TObjectIterator<UObject> It; It; ++It )
		{
			UObject* Object = *It;
			if( !Object->HasAllFlags( RF_Marked ) && !Object->IsIn(UObject::GetTransientPackage()) )
			{
				bEncounteredUnmarkedObject = TRUE;
				debugf(TEXT("----------------------------------------------------------------------------------------------------"));
				debugf(TEXT("%s didn't get cleaned up!"),*Object->GetFullName());
				UObject::StaticExec(*FString::Printf(TEXT("OBJ REFS CLASS=%s NAME=%s"),*Object->GetClass()->GetName(),*Object->GetPathName()));
				TMap<UObject*,UProperty*>	Route		= FArchiveTraceRoute::FindShortestRootPath( Object, TRUE, RF_Native  );
				FString						ErrorString	= FArchiveTraceRoute::PrintRootPath( Route, Object );
				debugf(TEXT("%s"),*ErrorString);
			}
		}
		check(!bEncounteredUnmarkedObject);
	}


	// Get time as a string
	FString CurrentTime = appSystemTimeString();

	// Re-used helper variables for writing to CSV file.
	FString		CSVDirectory	= appGameLogDir() + TEXT("AssetStatsCSVs") PATH_SEPARATOR;
	FString		CSVFilename		= TEXT("");
	FArchive*	CSVFile			= NULL;

	// Create CSV folder in case it doesn't exist yet.
	GFileManager->MakeDirectory( *CSVDirectory );

	// CSV: Human-readable spreadsheet format.
	CSVFilename	= FString::Printf(TEXT("%sStaticMeshStats-%s-%i-%s.csv"), *CSVDirectory, GGameName, GEngineVersion, *CurrentTime);
	CSVFile		= GFileManager->CreateFileWriter( *CSVFilename );
	if( CSVFile )
	{	
		// Write out header row.
		const FString& HeaderRow = FStaticMeshStats::GetCSVHeaderRow();
		CSVFile->Serialize( TCHAR_TO_ANSI( *HeaderRow ), HeaderRow.Len() );

		// Write out each individual stats row.
		for( TMap<FString,FStaticMeshStats>::TIterator It(ResourceNameToStaticMeshStats); It; ++ It )
		{
			const FStaticMeshStats& StatsEntry = It.Value();
			const FString& Row = StatsEntry.ToCSV();
			CSVFile->Serialize( TCHAR_TO_ANSI( *Row ), Row.Len() );
		}

		// Close and delete archive.
		CSVFile->Close();
		delete CSVFile;
	}
	else
	{
		debugf(NAME_Warning,TEXT("Could not create CSV file %s for writing."), *CSVFilename);
	}

	CSVFilename	= FString::Printf(TEXT("%sTextureStats-%s-%i-%s.csv"), *CSVDirectory, GGameName, GEngineVersion, *CurrentTime);
	CSVFile		= GFileManager->CreateFileWriter( *CSVFilename );
	if( CSVFile )
	{	
		// Write out header row.
		const FString& HeaderRow = FTextureStats::GetCSVHeaderRow();
		CSVFile->Serialize( TCHAR_TO_ANSI( *HeaderRow ), HeaderRow.Len() );

		// Write out each individual stats row.
		for( TMap<FString,FTextureStats>::TIterator It(ResourceNameToTextureStats); It; ++ It )
		{
			const FTextureStats& StatsEntry = It.Value();
			const FString& Row = StatsEntry.ToCSV();
			CSVFile->Serialize( TCHAR_TO_ANSI( *Row ), Row.Len() );
		}

		// Close and delete archive.
		CSVFile->Close();
		delete CSVFile;
	}
	else
	{
		debugf(NAME_Warning,TEXT("Could not create CSV file %s for writing."), *CSVFilename);
	}

	CSVFilename	= FString::Printf(TEXT("%sMaterialStats-%s-%i-%s.csv"), *CSVDirectory, GGameName, GEngineVersion, *CurrentTime);
	CSVFile		= GFileManager->CreateFileWriter( *CSVFilename );
	if( CSVFile )
	{	
		// Write out header row.
		const FString& HeaderRow = FMaterialStats::GetCSVHeaderRow();
		CSVFile->Serialize( TCHAR_TO_ANSI( *HeaderRow ), HeaderRow.Len() );

		// Write out each individual stats row.
		for( TMap<FString,FMaterialStats>::TIterator It(ResourceNameToMaterialStats); It; ++ It )
		{
			const FMaterialStats& StatsEntry = It.Value();
			const FString& Row = StatsEntry.ToCSV();
			CSVFile->Serialize( TCHAR_TO_ANSI( *Row ), Row.Len() );
		}

		// Close and delete archive.
		CSVFile->Close();
		delete CSVFile;
	}
	else
	{
		debugf(NAME_Warning,TEXT("Could not create CSV file %s for writing."), *CSVFilename);
	}

	CSVFilename	= FString::Printf(TEXT("%sParticleStats-%s-%i-%s.csv"), *CSVDirectory, GGameName, GEngineVersion, *CurrentTime);
	CSVFile		= GFileManager->CreateFileWriter( *CSVFilename );
	if( CSVFile )
	{	
		// Write out header row.
		const FString& HeaderRow = FParticleStats::GetCSVHeaderRow();
		CSVFile->Serialize( TCHAR_TO_ANSI( *HeaderRow ), HeaderRow.Len() );

		// Write out each individual stats row.
		for( TMap<FString,FParticleStats>::TIterator It(ResourceNameToParticleStats); It; ++ It )
		{
			const FParticleStats& StatsEntry = It.Value();
			const FString& Row = StatsEntry.ToCSV();
			CSVFile->Serialize( TCHAR_TO_ANSI( *Row ), Row.Len() );
		}

		// Close and delete archive.
		CSVFile->Close();
		delete CSVFile;
	}
	else
	{
		debugf(NAME_Warning,TEXT("Could not create CSV file %s for writing."), *CSVFilename);
	}

	CSVFilename	= FString::Printf(TEXT("%sAnimStats-%s-%i-%s.csv"), *CSVDirectory, GGameName, GEngineVersion, *CurrentTime);
	CSVFile		= GFileManager->CreateFileWriter( *CSVFilename );
	if( CSVFile )
	{	
		// Write out header row.
		const FString& HeaderRow = FAnimSequenceStats::GetCSVHeaderRow();
		CSVFile->Serialize( TCHAR_TO_ANSI( *HeaderRow ), HeaderRow.Len() );

		// Write out each individual stats row.
		for( TMap<FString,FAnimSequenceStats>::TIterator It(ResourceNameToAnimStats); It; ++ It )
		{
			const FAnimSequenceStats& StatsEntry = It.Value();
			const FString& Row = StatsEntry.ToCSV();
			CSVFile->Serialize( TCHAR_TO_ANSI( *Row ), Row.Len() );
		}

		// Close and delete archive.
		CSVFile->Close();
		delete CSVFile;
	}
	else
	{
		debugf(NAME_Warning,TEXT("Could not create CSV file %s for writing."), *CSVFilename);
	}

#if 0
	debugf(TEXT("%s"),*FStaticMeshStats::GetCSVHeaderRow());
	for( TMap<FString,FStaticMeshStats>::TIterator It(ResourceNameToStaticMeshStats); It; ++ It )
	{
		const FStaticMeshStats& StatsEntry = It.Value();
		debugf(TEXT("%s"),*StatsEntry.ToCSV());
	}

	debugf(TEXT("%s"),*FTextureStats::GetCSVHeaderRow());
	for( TMap<FString,FTextureStats>::TIterator It(ResourceNameToTextureStats); It; ++ It )
	{
		const FTextureStats& StatsEntry	= It.Value();
		debugf(TEXT("%s"),*StatsEntry.ToCSV());
	}

	debugf(TEXT("%s"),*FMaterialStats::GetCSVHeaderRow());
	for( TMap<FString,FMaterialStats>::TIterator It(ResourceNameToMaterialStats); It; ++ It )
	{
		const FMaterialStats& StatsEntry = It.Value();
		debugf(TEXT("%s"),*StatsEntry.ToCSV());
	}
#endif

	return 0;
}
IMPLEMENT_CLASS(UAnalyzeReferencedContentCommandlet)


/** Constructor, initializing all members */
UAnalyzeFallbackMaterialsCommandlet::FMaterialStats::FMaterialStats( UMaterial* Material, UAnalyzeFallbackMaterialsCommandlet* Commandlet )
:	ResourceType(Material->GetClass()->GetName())
,	ResourceName(Material->GetPathName())
{
	// Update dependency chain stats.
	Material->CacheResourceShaders(FALSE, TRUE);
	FMaterialResource* MaterialResource = Material->GetMaterialResource(SP_PCD3D_SM2);
	check( MaterialResource);
	DroppedFallbackComponents = MaterialResource->GetDroppedFallbackComponents();
}

/**
* Stringifies gathered stats in CSV format.
*
* @return comma separated list of stats
*/
FString UAnalyzeFallbackMaterialsCommandlet::FMaterialStats::ToCSV() const
{
	FString EmissiveDropped = TEXT("");
	if (DroppedFallbackComponents & DroppedFallback_Emissive)
	{
		EmissiveDropped = TEXT("YES");
	}
	FString DiffuseDropped = TEXT("");
	if (DroppedFallbackComponents & DroppedFallback_Diffuse)
	{
		DiffuseDropped = TEXT("YES");
	}
	FString NormalDropped = TEXT("");
	if (DroppedFallbackComponents & DroppedFallback_Normal)
	{
		NormalDropped = TEXT("YES");
	}
	FString SpecularDropped = TEXT("");
	if (DroppedFallbackComponents & DroppedFallback_Specular)
	{
		SpecularDropped = TEXT("YES");
	}

	return FString::Printf(TEXT("%s,%s,%s,%s,%s,%s%s"),
		*ResourceType,
		*ResourceName,
		*EmissiveDropped,
		*DiffuseDropped,
		*NormalDropped,
		*SpecularDropped,
		LINE_TERMINATOR);
}

/**
* Returns a header row for CSV
*
* @return comma separated header row
*/
FString UAnalyzeFallbackMaterialsCommandlet::FMaterialStats::GetCSVHeaderRow()
{
	return TEXT("ResourceType,ResourceName,Emissive Dropped,Diffuse Dropped,Normal Dropped,Specular Dropped") LINE_TERMINATOR;
}

/**
* Retrieves/ creates material stats associated with passed in material.
*
* @warning: returns pointer into TMap, only valid till next time Set is called
*
* @param	Material	Material to retrieve/ create material stats for
* @return	pointer to material stats associated with material
*/
UAnalyzeFallbackMaterialsCommandlet::FMaterialStats* UAnalyzeFallbackMaterialsCommandlet::GetMaterialStats( UMaterial* Material )
{
	UAnalyzeFallbackMaterialsCommandlet::FMaterialStats* MaterialStats = ResourceNameToMaterialStats.Find( *Material->GetFullName() );
	if( MaterialStats == NULL )
	{
		MaterialStats =	&ResourceNameToMaterialStats.Set( *Material->GetFullName(), UAnalyzeFallbackMaterialsCommandlet::FMaterialStats( Material, this ) );
	}
	return MaterialStats;
}


/**
* Handles encountered object, routing to various sub handlers.
*
* @param	Object			Object to handle
* @param	LevelPackage	Currently loaded level package, can be NULL if not a level
* @param	bIsScriptReferenced Whether object is handled because there is a script reference
*/
void UAnalyzeFallbackMaterialsCommandlet::HandleObject( UObject* Object, UPackage* LevelPackage, UBOOL bIsScriptReferenced )
{
	// Disregard marked objects as they won't go away with GC.
	if( !Object->HasAnyFlags( RF_Marked ) )
	{
		// Whether the object is the passed in level package if it is != NULL.
		UBOOL bIsInALevelPackage = LevelPackage && Object->IsIn( LevelPackage );

		// Handle material.
		if( Object->IsA(UMaterial::StaticClass()) )
		{
			HandleMaterial( (UMaterial*) Object, LevelPackage, bIsScriptReferenced );
		}
	}
}

/**
* Handles gathering stats for passed in material.
*
* @param Material	Material to gather stats for
* @param LevelPackage	Currently loaded level package, can be NULL if not a level
* @param bIsScriptReferenced Whether object is handled because there is a script reference
*/
void UAnalyzeFallbackMaterialsCommandlet::HandleMaterial( UMaterial* Material, UPackage* LevelPackage, UBOOL bIsScriptReferenced )
{
	UAnalyzeFallbackMaterialsCommandlet::FMaterialStats* MaterialStats = GetMaterialStats( Material );	
}



INT UAnalyzeFallbackMaterialsCommandlet::Main( const FString& Params )
{
	// Parse command line.
	TArray<FString> Tokens;
	TArray<FString> Switches;

	const TCHAR* Parms = *Params;
	ParseCommandLine(Parms, Tokens, Switches);

	// Whether to only deal with map files.
	const UBOOL bShouldOnlyLoadMaps	= Switches.FindItemIndex(TEXT("MAPSONLY")) != INDEX_NONE;
	// Whether to exclude script references.
	const UBOOL bExcludeScript		= Switches.FindItemIndex(TEXT("EXCLUDESCRIPT")) != INDEX_NONE;

	// Load up all script files in EditPackages.
	UEditorEngine* EditorEngine = CastChecked<UEditorEngine>(GEngine);
	for( INT i=0; i<EditorEngine->EditPackages.Num(); i++ )
	{
		LoadPackage( NULL, *EditorEngine->EditPackages(i), LOAD_NoWarn );
	}

	// Mark loaded objects as they are part of the always loaded set and are not taken into account for stats.
	for( TObjectIterator<UObject> It; It; ++It )
	{
		UObject* Object = *It;
		// Script referenced asset.
		if( !bExcludeScript )
		{
			HandleObject( Object, NULL, TRUE );
		}
		// Mark object as always loaded so it doesn't get counted multiple times.
		Object->SetFlags( RF_Marked );
	}

	TArray<FString> FileList;

	// Build package file list from passed in command line if tokens are specified.
	if( Tokens.Num() )
	{
		for( INT TokenIndex=0; TokenIndex<Tokens.Num(); TokenIndex++ )
		{
			// Lookup token in file cache and add filename if found.
			FString OutFilename;
			if( GPackageFileCache->FindPackageFile( *Tokens(TokenIndex), NULL, OutFilename ) )
			{
				new(FileList)FString(OutFilename);
			}
		}
	}
	// Or use all files otherwise.
	else
	{
		FileList = GPackageFileCache->GetPackageFileList();
	}

	if( FileList.Num() == 0 )
	{
		warnf( NAME_Warning, TEXT("No packages found") );
		return 1;
	}

	// Iterate over all files, loading up ones that have the map extension..
	for( INT FileIndex=0; FileIndex<FileList.Num(); FileIndex++ )
	{
		const FFilename& Filename = FileList(FileIndex);		

		// Disregard filenames that don't have the map extension if we're in MAPSONLY mode.
		if( bShouldOnlyLoadMaps && (Filename.GetExtension() != FURL::DefaultMapExt) )
		{
			continue;
		}

		// Skip filenames with the script extension. @todo: don't hardcode .u as the script file extension
		if( (Filename.GetExtension() == TEXT("u")) )
		{
			continue;
		}

		warnf( NAME_Log, TEXT("Loading %s"), *Filename );
		UPackage* Package = UObject::LoadPackage( NULL, *Filename, LOAD_None );
		if( Package == NULL )
		{
			warnf( NAME_Error, TEXT("Error loading %s!"), *Filename );
		}
		else
		{
			// Figure out whether package is a map or content package.
			const UBOOL bIsAMapPackage = FindObject<UWorld>(Package, TEXT("TheWorld")) != NULL;

			// Handle currently loaded objects.
			for( TObjectIterator<UObject> It; It; ++It )
			{
				UObject* Object = *It;
				HandleObject( Object, bIsAMapPackage ? Package : NULL, FALSE );
			}
		}

		// Collect garbage, going back to a clean slate.
		UObject::CollectGarbage( RF_Native );

		// Verify that everything we cared about got cleaned up correctly.
		UBOOL bEncounteredUnmarkedObject = FALSE;
		for( TObjectIterator<UObject> It; It; ++It )
		{
			UObject* Object = *It;
			if( !Object->HasAllFlags( RF_Marked ) && !Object->IsIn(UObject::GetTransientPackage()) )
			{
				bEncounteredUnmarkedObject = TRUE;
				debugf(TEXT("----------------------------------------------------------------------------------------------------"));
				debugf(TEXT("%s didn't get cleaned up!"),*Object->GetFullName());
				UObject::StaticExec(*FString::Printf(TEXT("OBJ REFS CLASS=%s NAME=%s"),*Object->GetClass()->GetName(),*Object->GetPathName()));
				TMap<UObject*,UProperty*>	Route		= FArchiveTraceRoute::FindShortestRootPath( Object, TRUE, RF_Native  );
				const FString				ErrorString	= FArchiveTraceRoute::PrintRootPath( Route, Object );
				debugf(TEXT("%s"),*ErrorString);
			}
		}
		check(!bEncounteredUnmarkedObject);
	}


	// Create string with system time to create a unique filename.
	INT Year, Month, DayOfWeek, Day, Hour, Min, Sec, MSec;
	appSystemTime( Year, Month, DayOfWeek, Day, Hour, Min, Sec, MSec );
	const FString	CurrentTime = FString::Printf(TEXT("%i.%02i.%02i-%02i.%02i.%02i"), Year, Month, Day, Hour, Min, Sec );

	// Re-used helper variables for writing to CSV file.
	FString		CSVDirectory	= appGameLogDir() + TEXT("AssetStatsCSVs") PATH_SEPARATOR;
	FString		CSVFilename		= TEXT("");
	FArchive*	CSVFile			= NULL;

	// Create CSV folder in case it doesn't exist yet.
	GFileManager->MakeDirectory( *CSVDirectory );

	CSVFilename	= FString::Printf(TEXT("%sFallbackMaterialStats-%s-%i-%s.csv"), *CSVDirectory, GGameName, GEngineVersion, *CurrentTime);
	CSVFile		= GFileManager->CreateFileWriter( *CSVFilename );
	if( CSVFile )
	{	
		// Write out header row.
		const FString& HeaderRow = FMaterialStats::GetCSVHeaderRow();
		CSVFile->Serialize( TCHAR_TO_ANSI( *HeaderRow ), HeaderRow.Len() );

		// Write out each individual stats row.
		for( TMap<FString,FMaterialStats>::TIterator It(ResourceNameToMaterialStats); It; ++ It )
		{
			const FMaterialStats& StatsEntry = It.Value();
			//only print the stats row if any component was dropped
			if (StatsEntry.DroppedFallbackComponents != DroppedFallback_None)
			{
				const FString& Row = StatsEntry.ToCSV();
				CSVFile->Serialize( TCHAR_TO_ANSI( *Row ), Row.Len() );
			}
		}

		// Close and delete archive.
		CSVFile->Close();
		delete CSVFile;
	}
	else
	{
		debugf(NAME_Warning,TEXT("Could not create CSV file %s for writing."), *CSVFilename);
	}

	return 0;
}
IMPLEMENT_CLASS(UAnalyzeFallbackMaterialsCommandlet);


/*-----------------------------------------------------------------------------
UShowStylesCommandlet
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UShowStylesCommandlet);

INT UShowStylesCommandlet::Main(const FString& Params)
{
	INT Result = 0;

	// Parse command line args.
	TArray<FString> Tokens;
	TArray<FString> Switches;

	const TCHAR* Parms = *Params;
	ParseCommandLine(Parms, Tokens, Switches);

	for ( INT TokenIndex = 0; TokenIndex < Tokens.Num(); TokenIndex++ )
	{
		FString& PackageWildcard = Tokens(TokenIndex);

		TArray<FString> PackageFileNames;
		GFileManager->FindFiles( PackageFileNames, *PackageWildcard, 1, 0 );
		if( PackageFileNames.Num() == 0 )
		{
			// if no files were found, it might be an unqualified path; try prepending the .u output path
			// if one were going to make it so that you could use unqualified paths for package types other
			// than ".u", here is where you would do it
			GFileManager->FindFiles( PackageFileNames, *(appScriptOutputDir() * PackageWildcard), 1, 0 );

			if ( PackageFileNames.Num() == 0 )
			{
				TArray<FString> Paths;
				if ( GConfig->GetArray( TEXT("Core.System"), TEXT("Paths"), Paths, GEngineIni ) > 0 )
				{
					for ( INT i = 0; i < Paths.Num(); i++ )
					{
						GFileManager->FindFiles( PackageFileNames, *(Paths(i) * PackageWildcard), 1, 0 );
					}
				}
			}
			else
			{
				// re-add the path information so that GetPackageLinker finds the correct version of the file.
				FFilename WildcardPath = appScriptOutputDir() * PackageWildcard;
				for ( INT FileIndex = 0; FileIndex < PackageFileNames.Num(); FileIndex++ )
				{
					PackageFileNames(FileIndex) = WildcardPath.GetPath() * PackageFileNames(FileIndex);
				}
			}

			// Try finding package in package file cache.
			if ( PackageFileNames.Num() == 0 )
			{
				FString Filename;
				if( GPackageFileCache->FindPackageFile( *PackageWildcard, NULL, Filename ) )
				{
					new(PackageFileNames)FString(Filename);
				}
			}
		}
		else
		{
			// re-add the path information so that GetPackageLinker finds the correct version of the file.
			FFilename WildcardPath = PackageWildcard;
			for ( INT FileIndex = 0; FileIndex < PackageFileNames.Num(); FileIndex++ )
			{
				PackageFileNames(FileIndex) = WildcardPath.GetPath() * PackageFileNames(FileIndex);
			}
		}

		if ( PackageFileNames.Num() == 0 )
		{
			warnf(TEXT("No packages found using '%s'!"), *PackageWildcard);
			continue;
		}

		// reset the loaders for the packages we want to load so that we don't find the wrong version of the file
		// (otherwise, attempting to run pkginfo on e.g. Engine.xxx will always return results for Engine.u instead)
		for ( INT FileIndex = 0; FileIndex < PackageFileNames.Num(); FileIndex++ )
		{
			const FString& PackageName = FPackageFileCache::PackageFromPath(*PackageFileNames(FileIndex));
			UPackage* ExistingPackage = FindObject<UPackage>(NULL, *PackageName, TRUE);
			if ( ExistingPackage != NULL )
			{
				ResetLoaders(ExistingPackage);
			}
		}

		for( INT FileIndex = 0; FileIndex < PackageFileNames.Num(); FileIndex++ )
		{
			const FString &Filename = PackageFileNames(FileIndex);

			warnf( NAME_Log, TEXT("Loading %s"), *Filename );

			UObject* Package = UObject::LoadPackage( NULL, *Filename, LOAD_None );
			if( Package == NULL )
			{
				warnf( NAME_Error, TEXT("Error loading %s!"), *Filename );
			}
			else
			{
				for ( FObjectIterator It; It; ++It )
				{
					if ( It->IsIn(Package) && It->IsA(UUIStyle::StaticClass()) )
					{
						DisplayStyleInfo(Cast<UUIStyle>(*It));
					}
				}
			}
		}
	}

	return Result;
}

void UShowStylesCommandlet::DisplayStyleInfo( UUIStyle* Style )
{
	// display the info about this style
	GWarn->Log(TEXT("*************************"));
	GWarn->Logf(TEXT("%s"), *Style->GetPathName());
	GWarn->Logf(TEXT("\t Archetype: %s"), *Style->GetArchetype()->GetPathName());
	GWarn->Logf(TEXT("\t       Tag: %s"), *Style->GetStyleName());
	GWarn->Logf(TEXT("\tStyleClass: %s"), Style->StyleDataClass != NULL ? *Style->StyleDataClass->GetName() : TEXT("NULL"));
	GWarn->Log(TEXT("\tStyle Data:"));

	const INT Indent = GetStyleDataIndent(Style);
	for ( TMap<UUIState*,UUIStyle_Data*>::TIterator It(Style->StateDataMap); It; ++It )
	{
		UUIState* State = It.Key();
		UUIStyle_Data* StyleData = It.Value();

		GWarn->Logf(TEXT("\t%*s: %s"), Indent, *State->GetClass()->GetName(), *StyleData->GetPathName());
		UUIStyle_Combo* ComboStyleData = Cast<UUIStyle_Combo>(StyleData);
		if ( ComboStyleData != NULL )
		{
			UUIStyle_Data* CustomTextStyle = ComboStyleData->TextStyle.GetCustomStyleData();
			GWarn->Logf(TEXT("\t\t TextStyle: %s"), CustomTextStyle ? *CustomTextStyle->GetPathName() : TEXT("NULL"));

			UUIStyle_Data* CustomImageStyle = ComboStyleData->ImageStyle.GetCustomStyleData();
			GWarn->Logf(TEXT("\t\tImageStyle: %s"), CustomImageStyle ? *CustomImageStyle->GetPathName() : TEXT("NULL"));
		}
	}
}

INT UShowStylesCommandlet::GetStyleDataIndent( UUIStyle* Style )
{
	INT Result = 0;

	check(Style);
	for ( TMap<UUIState*,UUIStyle_Data*>::TIterator It(Style->StateDataMap); It; ++It )
	{
		FString StateClassName = It.Key()->GetClass()->GetName();
		Result = Max(StateClassName.Len(), Result);
	}

	return Result;
}


IMPLEMENT_CLASS(UTestWordWrapCommandlet);
INT UTestWordWrapCommandlet::Main(const FString& Params)
{
	INT Result = 0;

	// replace any \n strings with the real character code
	FString MyParams = Params.Replace(TEXT("\\n"), TEXT("\n"));
	const TCHAR* Parms = *MyParams;

	INT WrapWidth = 0;
	FString WrapWidthString;
	ParseToken(Parms, WrapWidthString, FALSE);
	WrapWidth = appAtoi(*WrapWidthString);

	// advance past the space between the width and the test string
	Parms++;
	warnf(TEXT("WrapWidth: %i  WrapText: '%s'"), WrapWidth, Parms);
	UFont* DrawFont = GEngine->GetTinyFont();

	FRenderParameters Parameters(0, 0, WrapWidth, 0, DrawFont);
	TArray<FWrappedStringElement> Lines;

	UUIString::WrapString(Parameters, 0, Parms, Lines, TEXT("\n"));

	warnf(TEXT("Result: %i lines"), Lines.Num());
	for ( INT LineIndex = 0; LineIndex < Lines.Num(); LineIndex++ )
	{
		FWrappedStringElement& Line = Lines(LineIndex);
		warnf(TEXT("Line %i): (X=%.2f,Y=%.2f) '%s'"), LineIndex, Line.LineExtent.X, Line.LineExtent.Y, *Line.Value);
	}

	return Result;
}

/** sets certain allowed package flags on the specified package(s) */
INT USetPackageFlagsCommandlet::Main(const FString& Params)
{
	TArray<FString> Tokens, Switches;

	ParseCommandLine(*Params, Tokens, Switches);

	if (Tokens.Num() < 2)
	{
		warnf(TEXT("Syntax: setpackageflags <package/wildcard> <flag1=value> <flag2=value>..."));
		warnf(TEXT("Supported flags: ServerSideOnly, ClientOptional, AllowDownload"));
		return 1;
	}

	// find all the files matching the specified filename/wildcard
	TArray<FString> FilesInPath;
	GFileManager->FindFiles(FilesInPath, *Tokens(0), 1, 0);
	if (FilesInPath.Num() == 0)
	{
		warnf(NAME_Error, TEXT("No packages found matching %s!"), *Tokens(0));
		return 2;
	}
	// get the directory part of the filename
	INT ChopPoint = Max(Tokens(0).InStr(TEXT("/"), 1) + 1, Tokens(0).InStr(TEXT("\\"), 1) + 1);
	if (ChopPoint < 0)
	{
		ChopPoint = Tokens(0).InStr( TEXT("*"), 1 );
	}
	FString PathPrefix = (ChopPoint < 0) ? TEXT("") : Tokens(0).Left(ChopPoint);

	// parse package flags
	DWORD PackageFlagsToAdd = 0, PackageFlagsToRemove = 0;
	for (INT i = 1; i < Tokens.Num(); i++)
	{
		DWORD NewFlag = 0;
		UBOOL bValue;
		if (ParseUBOOL(*Tokens(i), TEXT("ServerSideOnly="), bValue))
		{
			NewFlag = PKG_ServerSideOnly;
		}
		else if (ParseUBOOL(*Tokens(i), TEXT("ClientOptional="), bValue))
		{
			NewFlag = PKG_ClientOptional;
		}
		else if (ParseUBOOL(*Tokens(i), TEXT("AllowDownload="), bValue))
		{
			NewFlag = PKG_AllowDownload;
		}
		else
		{
			warnf(NAME_Warning, TEXT("Unknown package flag '%s' specified"), *Tokens(i));
		}
		if (NewFlag != 0)
		{
			if (bValue)
			{
				PackageFlagsToAdd |= NewFlag;
			}
			else
			{
				PackageFlagsToRemove |= NewFlag;
			}
		}
	}
	// process files
	for (INT i = 0; i < FilesInPath.Num(); i++)
	{
		const FString& PackageName = FilesInPath(i);
		// get the full path name to the file
		const FString FileName = PathPrefix + PackageName;
		// skip if read-only
		if (GFileManager->IsReadOnly(*FileName))
		{
			warnf(TEXT("Skipping %s (read-only)"), *FileName);
		}
		else
		{
			// load the package
			warnf(TEXT("Loading %s..."), *PackageName); 
			UPackage* Package = LoadPackage(NULL, *PackageName, LOAD_None);
			if (Package == NULL)
			{
				warnf(NAME_Error, TEXT("Failed to load package '%s'"), *PackageName);
			}
			else
			{
				// set flags
				Package->PackageFlags |= PackageFlagsToAdd;
				Package->PackageFlags &= ~PackageFlagsToRemove;
				// save the package
				warnf(TEXT("Saving %s..."), *PackageName);
				SavePackage(Package, NULL, RF_Standalone, *FileName, GWarn);
			}
			// GC the package
			warnf(TEXT("Cleaning up..."));
			CollectGarbage(RF_Native);
		}
	}
	return 0;
}
IMPLEMENT_CLASS(USetPackageFlagsCommandlet);


/* ==========================================================================================================
	UPerformMapCheckCommandlet
========================================================================================================== */
IMPLEMENT_COMPARE_CONSTREF(INT,ReferencedStaticActorCount,
{
	return B - A;
})

INT UPerformMapCheckCommandlet::Main( const FString& Params )
{
	TArray<FString> Tokens, Switches, Unused;
	ParseCommandLine(*Params, Tokens, Switches);

	// assume the first token is the map wildcard/pathname
	FString MapWildcard = Tokens.Num() > 0 ? Tokens(0) : FString(TEXT("*.")) + FURL::DefaultMapExt;
	TArray<FFilename> MapNames;
	NormalizePackageNames( Unused, MapNames, MapWildcard, NORMALIZE_ExcludeContentPackages);

	const UBOOL bStaticKismetRefs = Switches.ContainsItem(TEXT("STATICREFS"));
	const UBOOL bShowObjectNames = Switches.ContainsItem(TEXT("OBJECTNAMES"));
	const UBOOL bLogObjectNames = Switches.ContainsItem(TEXT("LOGOBJECTNAMES"));
	const UBOOL bShowReferencers = Switches.ContainsItem(TEXT("SHOWREFERENCERS"));

	INT GCIndex=0;
	INT TotalStaticMeshActors = 0, TotalStaticLightActors = 0;
	INT TotalReferencedStaticMeshActors = 0, TotalReferencedStaticLightActors = 0;
	INT TotalMapsChecked=0;
	TMap<INT, INT> ReferencedStaticMeshActorMap;
	TMap<INT, INT> ReferencedStaticLightActorMap;
	for ( INT MapIndex = 0; MapIndex < MapNames.Num(); MapIndex++ )
	{
		const FFilename& Filename = MapNames(MapIndex);
		warnf( TEXT("Loading  %s...  (%i / %i)"), *Filename, MapIndex, MapNames.Num() );

		UPackage* Package = UObject::LoadPackage( NULL, *Filename, LOAD_NoWarn );
		if ( Package == NULL )
		{
			warnf( NAME_Error, TEXT("Error loading %s!"), *Filename );
			continue;
		}

		// skip packages in the trashcan or PIE packages
		if ( (Package->PackageFlags&(PKG_Trash|PKG_PlayInEditor)) != 0 )
		{
			warnf(TEXT("Skipping %s (%s)"), *Filename, (Package->PackageFlags&PKG_Trash) != 0 ? TEXT("Trashcan Map") : TEXT("PIE Map"));
			UObject::CollectGarbage(RF_Native);
			continue;
		}

		TotalMapsChecked++;

		warnf(TEXT("Checking %s..."), *Filename);
		if ( !bStaticKismetRefs )
		{
			for ( TObjectIterator<AActor> It; It; ++It )
			{
				AActor* Actor = *It;
				if ( Actor->IsIn(Package) && !Actor->IsTemplate() )
				{
					Actor->CheckForErrors();
				}
			}
		}
		else
		{
			// find all StaticMeshActors and static Light actors which are referenced by something in the map
			UWorld* World = FindObject<UWorld>( Package, TEXT("TheWorld") );
			if ( World )
			{
				// make sure that the world's PersistentLevel is part of the levels array for the purpose of this test.
				World->Levels.AddUniqueItem(World->PersistentLevel);
				for ( INT LevelIndex = 0; LevelIndex < World->Levels.Num(); LevelIndex++ )
				{
					ULevel* Level = World->Levels(LevelIndex);

					// remove all StaticMeshActors from the level's Actor array so that we don't get false positives.
					TArray<AStaticMeshActor*> StaticMeshActors;
					if ( ContainsObjectOfClass<AActor>(Level->Actors, AStaticMeshActor::StaticClass(), FALSE, (TArray<AActor*>*)&StaticMeshActors) )
					{
						for ( INT i = 0; i < StaticMeshActors.Num(); i++ )
						{
							Level->Actors.RemoveItem(StaticMeshActors(i));
						}
					}

					// same for lights marked bStatic
					TArray<ALight*> Lights;
					if ( ContainsObjectOfClass<AActor>(Level->Actors, ALight::StaticClass(), FALSE, (TArray<AActor*>*)&Lights) )
					{
						for ( INT i = Lights.Num() - 1; i >= 0; i-- )
						{
							// only care about static lights - if the light is static, remove it from the level's Actors array
							// so that we don't get false positives; otherwise, remove it from the list of lights that we'll process
							if ( Lights(i)->bStatic )
							{
								Level->Actors.RemoveItem(Lights(i));
							}
							else
							{
								Lights.Remove(i);
							}
						}
					}

					// now use the object reference collector to find the static mesh actors that are still being referenced
					TArray<AStaticMeshActor*> ReferencedStaticMeshActors;
					TArchiveObjectReferenceCollector<AStaticMeshActor> SMACollector(&ReferencedStaticMeshActors, Package, FALSE, TRUE, TRUE, TRUE);
					Level->Serialize( SMACollector );

					if ( ReferencedStaticMeshActors.Num() > 0 )
					{
						warnf(TEXT("\t%i of %i StaticMeshActors referenced"), ReferencedStaticMeshActors.Num(), StaticMeshActors.Num());
						if ( bShowReferencers )
						{
							TFindObjectReferencers<AStaticMeshActor> StaticMeshReferencers(ReferencedStaticMeshActors, Package);

							for ( INT RefIndex = 0; RefIndex < ReferencedStaticMeshActors.Num(); RefIndex++ )
							{
								AStaticMeshActor* StaticMeshActor = ReferencedStaticMeshActors(RefIndex);
								debugf(TEXT("\t  %i) %s"), RefIndex, *StaticMeshActor->GetFullName());

								TArray<UObject*> Referencers;
								StaticMeshReferencers.MultiFind(StaticMeshActor, Referencers);

								INT Count=0;
								for ( INT ReferencerIndex = Referencers.Num() - 1; ReferencerIndex >= 0; ReferencerIndex-- )
								{
									if ( Referencers(ReferencerIndex) != StaticMeshActor->StaticMeshComponent )
									{
										debugf(TEXT("\t\t %i) %s"), Count, *Referencers(ReferencerIndex)->GetFullName());
										Count++;
									}
								}

								if ( Count == 0 )
								{
									debugf(TEXT("\t\t  (StaticMeshComponent referenced from external source)"));
								}

								debugf(TEXT(""));
							}

							debugf(TEXT("******"));
						}
						else if ( bShowObjectNames )
						{
							for ( INT RefIndex = 0; RefIndex < ReferencedStaticMeshActors.Num(); RefIndex++ )
							{
								warnf(TEXT("\t  %i) %s"), RefIndex, *ReferencedStaticMeshActors(RefIndex)->GetFullName());
							}

							warnf(TEXT(""));
						}
						else if ( bLogObjectNames )
						{
							for ( INT RefIndex = 0; RefIndex < ReferencedStaticMeshActors.Num(); RefIndex++ )
							{
								debugf(TEXT("\t  %i) %s"), RefIndex, *ReferencedStaticMeshActors(RefIndex)->GetFullName());
							}

							debugf(TEXT(""));
						}

						ReferencedStaticMeshActorMap.Set(MapIndex, ReferencedStaticMeshActors.Num());
						TotalReferencedStaticMeshActors += ReferencedStaticMeshActors.Num();
					}

					TArray<ALight*> ReferencedLights;
					TArchiveObjectReferenceCollector<ALight> LightCollector(&ReferencedLights, Package, FALSE, TRUE, TRUE, TRUE);
					Level->Serialize( LightCollector );

					for ( INT RefIndex = ReferencedLights.Num() - 1; RefIndex >= 0; RefIndex-- )
					{
						if ( !ReferencedLights(RefIndex)->bStatic )
						{
							ReferencedLights.Remove(RefIndex);
						}
					}
					if ( ReferencedLights.Num() > 0 )
					{
						warnf(TEXT("\t%i of %i static Light actors referenced"), ReferencedLights.Num(), Lights.Num());
						if ( bShowReferencers )
						{
							TFindObjectReferencers<ALight> StaticLightReferencers(ReferencedLights, Package);

							for ( INT RefIndex = 0; RefIndex < ReferencedLights.Num(); RefIndex++ )
							{
								ALight* StaticLightActor = ReferencedLights(RefIndex);
								debugf(TEXT("\t  %i) %s"), RefIndex, *StaticLightActor->GetFullName());

								TArray<UObject*> Referencers;
								StaticLightReferencers.MultiFind(StaticLightActor, Referencers);

								INT Count=0;
								UBOOL bShowSubobjects = FALSE;
LogLightReferencers:
								for ( INT ReferencerIndex = Referencers.Num() - 1; ReferencerIndex >= 0; ReferencerIndex-- )
								{
									if ( bShowSubobjects || !Referencers(ReferencerIndex)->IsIn(StaticLightActor) )
									{
										debugf(TEXT("\t\t %i) %s"), Count, *Referencers(ReferencerIndex)->GetFullName());
										Count++;
									}
								}

								if ( !bShowSubobjects && Count == 0 )
								{
									bShowSubobjects = TRUE;
									goto LogLightReferencers;
								}

								debugf(TEXT(""));
							}

							debugf(TEXT("******"));
						}
						else if ( bShowObjectNames )
						{
							for ( INT RefIndex = 0; RefIndex < ReferencedLights.Num(); RefIndex++ )
							{
								warnf(TEXT("\t  %i) %s"), RefIndex, *ReferencedLights(RefIndex)->GetFullName());
							}
							warnf(TEXT(""));
						}
						else if ( bLogObjectNames )
						{
							for ( INT RefIndex = 0; RefIndex < ReferencedLights.Num(); RefIndex++ )
							{
								debugf(TEXT("\t  %i) %s"), RefIndex, *ReferencedLights(RefIndex)->GetFullName());
							}

							debugf(TEXT(""));
						}

						ReferencedStaticLightActorMap.Set(MapIndex, ReferencedLights.Num());
						TotalReferencedStaticLightActors += ReferencedLights.Num();
					}

					if (!bShowObjectNames
					&&	(ReferencedStaticMeshActors.Num() > 0 || ReferencedLights.Num() > 0))
					{
						warnf(TEXT(""));
					}

					TotalStaticMeshActors += StaticMeshActors.Num();
					TotalStaticLightActors += Lights.Num();
				}
			}
		}

		// save shader caches incase we encountered some that weren't fully cached yet
		SaveLocalShaderCaches();

		// collecting garbage every 10 maps instead of every map makes the commandlet run much faster
		if( (++GCIndex % 10) == 0 )
		{
			UObject::CollectGarbage(RF_Native);
		}
	}

	// sort the list of maps by the number of referenceed static actors contained in them
	ReferencedStaticMeshActorMap.ValueSort<COMPARE_CONSTREF_CLASS(INT,ReferencedStaticActorCount)>();
	ReferencedStaticLightActorMap.ValueSort<COMPARE_CONSTREF_CLASS(INT,ReferencedStaticActorCount)>();

	warnf(LINE_TERMINATOR TEXT("Referenced StaticMeshActor Summary"));
	for ( TMap<INT,INT>::TIterator It(ReferencedStaticMeshActorMap); It; ++It )
	{
		warnf(TEXT("%-4i %s"), It.Value(), *MapNames(It.Key()));
	}

	warnf(LINE_TERMINATOR TEXT("Referenced Static Light Actor Summary"));
	for ( TMap<INT,INT>::TIterator It(ReferencedStaticLightActorMap); It; ++It )
	{
		warnf(TEXT("%-4i %s"), It.Value(), *MapNames(It.Key()));
	}

	warnf(LINE_TERMINATOR TEXT("Total static actors referenced across %i maps"), TotalMapsChecked);
	warnf(TEXT("StaticMeshActors: %i (of %i) across %i maps"), TotalReferencedStaticMeshActors, TotalStaticMeshActors, ReferencedStaticMeshActorMap.Num());
	warnf(TEXT("Static Light Actors: %i (of %i) across %i maps"), TotalReferencedStaticLightActors, TotalStaticLightActors, ReferencedStaticLightActorMap.Num());

	return 0;
}
IMPLEMENT_CLASS(UPerformMapCheckCommandlet);

// EOF




