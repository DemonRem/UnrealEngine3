/*=============================================================================
	UStripSourceCommandlet.cpp: Load a .u file and remove the script text from
	all classes.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EditorPrivate.h"

/*-----------------------------------------------------------------------------
	UStripSourceCommandlet
-----------------------------------------------------------------------------*/

INT UStripSourceCommandlet::Main( const FString& Params )
{
	const TCHAR* Parms = *Params;

	FString PackageName, FullPackageName;
	if( !ParseToken(Parms, PackageName, 0) )
		appErrorf( TEXT("A .u package file must be specified.") );

	warnf( TEXT("Loading package %s..."), *PackageName );
	warnf(TEXT(""));

	// resolve the package name so that it is saved into the correct location
	GPackageFileCache->FindPackageFile(*PackageName, NULL, FullPackageName);

	UPackage* Package = LoadPackage( NULL, *FullPackageName, LOAD_NoWarn );
	if( !Package )
	{
		appErrorf( TEXT("Unable to load %s"), *PackageName );
	}

	for( TObjectIterator<UClass> It; It; ++It )
	{
		if( It->GetOuter() == Package && It->ScriptText )
		{
			warnf( TEXT("  Stripping source code from class %s"), *It->GetName() );
			It->ScriptText->Text = FString(TEXT(" "));
			It->ScriptText->Pos = 0;
			It->ScriptText->Top = 0;
		}
	}

	warnf(TEXT(""));
	warnf(TEXT("Saving %s..."), *FullPackageName );
	SavePackage( Package, NULL, RF_Standalone, *FullPackageName, GWarn );

	GIsRequestingExit=1;
	return 0;
}
IMPLEMENT_CLASS(UStripSourceCommandlet)

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
