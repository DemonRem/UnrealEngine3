/*=============================================================================
	SpeedTreeBrowserType.cpp: SpeedTree generic browser implementation
	Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "UnrealEd.h"
#include "GenericBrowserContextMenus.h"
#include "SpeedTree.h"

IMPLEMENT_CLASS(UGenericBrowserType_SpeedTree);

#if WITH_SPEEDTREE
class WxMBGenericBrowserContext_SpeedTree : public WxMBGenericBrowserContext
{
public:
	WxMBGenericBrowserContext_SpeedTree(void)
	{
		Append(IDMN_ObjectContext_Editor, *LocalizeUnrealEd("SpeedTreeEditor"), TEXT(""));
		AppendObjectMenu( );
	}
};
#endif

void UGenericBrowserType_SpeedTree::Init( )
{
#if WITH_SPEEDTREE
	SupportInfo.AddItem(FGenericBrowserTypeInfo(USpeedTree::StaticClass( ), FColor(100, 255, 100), new WxMBGenericBrowserContext_SpeedTree));
#endif
}

UBOOL UGenericBrowserType_SpeedTree::ShowObjectEditor(UObject* InObject)
{
#if WITH_SPEEDTREE
	WxSpeedTreeEditor* SpeedTreeEditor = new WxSpeedTreeEditor(GApp->EditorFrame, -1, CastChecked<USpeedTree>(InObject));
	SpeedTreeEditor->Show(1);
	return TRUE;
#else
	return FALSE;
#endif
}

/**
* Returns a list of commands that this object supports (or the object type supports, if InObject is NULL)
*
* @param	InObjects		The objects to query commands for (if NULL, query commands for all objects of this type.)
* @param	OutCommands		The list of custom commands to support
*/
void UGenericBrowserType_SpeedTree::QuerySupportedCommands( USelection* InObjects, TArray< FObjectSupportedCommandType >& OutCommands ) const
{
#if WITH_SPEEDTREE
	const UBOOL bAnythingCooked = AnyObjectsAreCooked( InObjects );
	OutCommands.AddItem( FObjectSupportedCommandType( IDMN_ObjectContext_Editor, *LocalizeUnrealEd( "ObjectContext_EditUsingSpeedTreeEditor" ) ) );
#endif
}

