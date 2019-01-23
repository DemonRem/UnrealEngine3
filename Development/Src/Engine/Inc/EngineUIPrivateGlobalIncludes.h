/**
 *
 * Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
 */


/*===========================================================================
    Class and struct declarations which are coupled to
	EngineUIPrivateClasses.h but shouldn't be declared inside a class
===========================================================================*/

#ifndef NAMES_ONLY

#include "UnPrefab.h"	// Required for access to FPrefabUpdateArc

#ifndef __ENGINEUIPRIVATEGLOBALINCLUDES_H__
#define __ENGINEUIPRIVATEGLOBALINCLUDES_H__

/**
 * Specialized version of FPrefabUpdateArc used for handling UIPrefabs.
 */
class FUIPrefabUpdateArc : public FPrefabUpdateArc
{
	/** Allow UIPrefabInstance to access all our personals */
	friend class UUIPrefabInstance;

public:
	FUIPrefabUpdateArc() : FPrefabUpdateArc()
	{
		bInstanceSubobjectsOnLoad = FALSE;
	}
};

/**
 * Specialized version of FArchiveReplaceObjectRef used for handling UIPrefabs.  It extends the functionality of FArchiveReplaceObjectRef in two ways:
 *	- allows the caller to specify a set of objects which should NOT be recursively serialized by this archive
 */
class FUIPrefabReplaceObjectRefArc : public FArchiveReplaceObjectRef<UObject>
{
public:
	/**
	 * Initializes variables and starts the serialization search
	 *
	 * @param InSearchObject		The object to start the search on
	 * @param ReplacementMap		Map of objects to find -> objects to replace them with (null zeros them)
	 * @param inSerializeExclusionMap
	 *								Map of objects that this archive should NOT call Serialize on.  They will be added to the SerializedObjects map
	 *								before the process begins.
	 * @param bNullPrivateRefs		Whether references to non-public objects not contained within the SearchObject
	 *								should be set to null
	 * @param bIgnoreOuterRef		Whether we should replace Outer pointers on Objects.
	 * @param bIgnoreArchetypeRef	Whether we should replace the ObjectArchetype reference on Objects.
	 */
	FUIPrefabReplaceObjectRefArc
	(
		UObject* InSearchObject,
		const TMap<UObject*,UObject*>& inReplacementMap,
		const TLookupMap<UObject*>* inSerializeExclusionMap,
		UBOOL bNullPrivateRefs,
		UBOOL bIgnoreOuterRef,
		UBOOL bIgnoreArchetypeRef
	)
	: FArchiveReplaceObjectRef<UObject>(InSearchObject,inReplacementMap,bNullPrivateRefs,bIgnoreOuterRef,bIgnoreArchetypeRef,TRUE)
	{
		if ( inSerializeExclusionMap != NULL )
		{
			for ( INT ExclusionIndex = 0; ExclusionIndex < inSerializeExclusionMap->Num(); ExclusionIndex++ )
			{
				UObject* ExcludedObject = (*inSerializeExclusionMap)(ExclusionIndex);
				if ( ExcludedObject != NULL && ExcludedObject != SearchObject )
				{
					SerializedObjects.AddItem(ExcludedObject);
				}
			}
		}

		SerializeSearchObject();
	}
};

enum EUISceneTickStats
{
	STAT_UISceneTickTime = STAT_UIDrawingTime+1,
	STAT_UISceneUpdateTime,
	STAT_UIPreRenderCallbackTime,
	STAT_UIRefreshFormatting,
	STAT_UIRebuildDockingStack,
	STAT_UIResolveScenePositions,
	STAT_UIRebuildNavigationLinks,
	STAT_UIRefreshWidgetStyles,

	STAT_UIAddDockingNode,
	STAT_UIAddDockingNode_String,

	STAT_UIResolvePosition,
	STAT_UIResolvePosition_String,
	STAT_UIResolvePosition_List,
	STAT_UIResolvePosition_AutoAlignment,

	STAT_UIGetStringFormatParms,
	STAT_UIApplyStringFormatting,

	STAT_UIParseString,

	STAT_UISetWidgetPosition,
	STAT_UIGetWidgetPosition,
	STAT_UICalculateBaseValue,
	STAT_UIGetPositionValue,
	STAT_UISetPositionValue,
};

#endif	// __ENGINEUIPRIVATEGLOBALINCLUDES_H__
#endif	// NAMES_ONLY
