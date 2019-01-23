//=============================================================================
// Copyright © 1998-2007 Epic Games - All Rights Reserved.
//=============================================================================

#include "UnrealEd.h"
#include "UTEditor.h"
#include "UTEditorFactories.h"

#define STATIC_LINKING_MOJO 1

// Register things.
#define NAMES_ONLY
#define AUTOGENERATE_NAME(name) FName UTEDITOR_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name) IMPLEMENT_FUNCTION(cls,idx,name)
#include "UTEditorClasses.h"
#undef AUTOGENERATE_FUNCTION
#undef AUTOGENERATE_NAME
#undef NAMES_ONLY

// Register natives.
#define NATIVES_ONLY
#define NAMES_ONLY
#define AUTOGENERATE_NAME(name)
#define AUTOGENERATE_FUNCTION(cls,idx,name)
#include "UTEditorClasses.h"
#undef AUTOGENERATE_FUNCTION
#undef AUTOGENERATE_NAME
#undef NATIVES_ONLY
#undef NAMES_ONLY

/**
 * Initialize registrants, basically calling StaticClass() to create the class and also 
 * populating the lookup table.
 *
 * @param	Lookup	current index into lookup table
 */
void AutoInitializeRegistrantsUTEditor( INT& Lookup )
{
	AUTO_INITIALIZE_REGISTRANTS_UTEDITOR;
}

/**
 * Auto generates names.
 */
void AutoGenerateNamesUTEditor()
{
	#define NAMES_ONLY
	#define AUTOGENERATE_FUNCTION(cls,idx,name)
    #define AUTOGENERATE_NAME(name) UTEDITOR_##name = FName(TEXT(#name));
	#include "UTEditorClasses.h"
	#undef AUTOGENERATE_FUNCTION
	#undef AUTOGENERATE_NAME
	#undef NAMES_ONLY
}


/*------------------------------------------------------------------------------
     UUTMapMusicInfoFactoryNew.
------------------------------------------------------------------------------*/

void UUTMapMusicInfoFactoryNew::StaticConstructor()
{
	new(GetClass()->HideCategories) FName(NAME_Object);
}

/**
 * Initializes property values for intrinsic classes.  It is called immediately after the class default object
 * is initialized against its archetype, but before any objects of this class are created.
 */
void UUTMapMusicInfoFactoryNew::InitializeIntrinsicPropertyValues()
{
	SupportedClass		= UUTMapMusicInfo::StaticClass();
	bCreateNew			= TRUE;
	bEditAfterNew		= TRUE;
	Description			= TEXT("UT Map Music");
}

UObject* UUTMapMusicInfoFactoryNew::FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn)
{
	return StaticConstructObject(Class,InParent,Name,Flags);
}

IMPLEMENT_CLASS(UUTMapMusicInfoFactoryNew);



void UGenericBrowserType_UTMapMusicInfo::Init()
{
	SupportInfo.AddItem(FGenericBrowserTypeInfo(UUTMapMusicInfo::StaticClass(), FColor(200,128,128), NULL, 0, this));
}

IMPLEMENT_CLASS(UGenericBrowserType_UTMapMusicInfo);
