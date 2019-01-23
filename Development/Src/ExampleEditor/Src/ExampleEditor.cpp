//=============================================================================
// Copyright � 1998-2007 Epic Games - All Rights Reserved.
// This is where you would extend the editor with game-specific dependencies
// and functionality
//=============================================================================

#include "UnrealEd.h"
#include "ExampleEditorClasses.h"

#define STATIC_LINKING_MOJO 1

// Register things.
#define NAMES_ONLY
#define AUTOGENERATE_NAME(name) FName EXAMPLEEDITOR_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name) IMPLEMENT_FUNCTION(cls,idx,name)
#include "ExampleEditorClasses.h"
#undef AUTOGENERATE_FUNCTION
#undef AUTOGENERATE_NAME
#undef NAMES_ONLY

// Register natives.
#define NATIVES_ONLY
#define NAMES_ONLY
#define AUTOGENERATE_NAME(name)
#define AUTOGENERATE_FUNCTION(cls,idx,name)
#include "ExampleEditorClasses.h"
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
void AutoInitializeRegistrantsExampleEditor( INT& Lookup )
{
	//AUTO_INITIALIZE_REGISTRANTS_EXAMPLEEDITOR;
}

/**
 * Auto generates names.
 */
void AutoGenerateNamesExampleEditor()
{
	#define NAMES_ONLY
	#define AUTOGENERATE_FUNCTION(cls,idx,name)
    #define AUTOGENERATE_NAME(name) EXAMPLEEDITOR_##name = FName(TEXT(#name));
	#include "ExampleEditorClasses.h"
	#undef AUTOGENERATE_FUNCTION
	#undef AUTOGENERATE_NAME
	#undef NAMES_ONLY
}
