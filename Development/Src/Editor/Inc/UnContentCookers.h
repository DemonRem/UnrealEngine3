/*=============================================================================
	UnContentCookers.h: Content cooker helper objects.

	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/*-----------------------------------------------------------------------------
	Cooking.
-----------------------------------------------------------------------------*/

/**
 * Helper structure containing information needed for separating bulk data from it's archive
 */
struct FCookedBulkDataInfo
{
	/** From last saving or StoreInSeperateFile call: Serialized flags for bulk data									*/
	DWORD	SavedBulkDataFlags;
	/** From last saving or StoreInSeperateFile call: Number of elements in bulk data array								*/
	INT		SavedElementCount;
	/** From last saving or StoreInSeperateFile call: Offset of bulk data into file or INDEX_NONE if no association		*/
	INT		SavedBulkDataOffsetInFile;
	/** From last saving or StoreInSeperateFile call: Size of bulk data on disk or INDEX_NONE if no association			*/
	INT		SavedBulkDataSizeOnDisk;
	
	/**
	 * Serialize operator
	 *
	 * @param	Ar		Archive to serialize with
	 * @param	Info	Structure to serialize
	 * @return	Ar after having been used for serialization
	 */
	friend FArchive& operator<<( FArchive& Ar, FCookedBulkDataInfo& Info )
    {
        Ar << Info.SavedBulkDataFlags;
		Ar << Info.SavedElementCount;
		Ar << Info.SavedBulkDataOffsetInFile;
		Ar << Info.SavedBulkDataSizeOnDisk;
		return Ar;
    }
};

/**
 * Serialized container class used for mapping a unique name to bulk data info.
 */
class UCookedBulkDataInfoContainer : public UObject
{
	DECLARE_CLASS(UCookedBulkDataInfoContainer,UObject,CLASS_Intrinsic,Editor);

	/**
	 * Create an instance of this class given a filename. First try to load from disk and if not found
	 * will construct object and store the filename for later use during saving.
	 *
	 * @param	Filename	Filename to use for serialization
	 * @return	instance of the container associated with the filename
	 */
	static UCookedBulkDataInfoContainer* CreateInstance( const TCHAR* Filename );
	
	/**
	 * Saves the data to disk.
	 */
	void SaveToDisk();

	/**
	 * Serialize function.
	 *
	 * @param	Ar	Archive to serialize with.
	 */
	virtual void Serialize(FArchive& Ar);

	/**
	 * Stores the bulk data info after creating a unique name out of the object and name of bulk data
	 *
	 * @param	Object					Object used to create unique name
	 * @param	BulkDataName			Unique name of bulk data within object, e.g. MipLevel_3
	 * @param	CookedBulkDataInfo		Data to store
	 */
	void SetInfo( UObject* Object, const TCHAR* BulkDataName, const FCookedBulkDataInfo& CookedBulkDataInfo );
	
	/**
	 * Retrieves previously stored bulk data info data for object/ bulk data array name combination.
	 *
	 * @param	Object					Object used to create unique name
	 * @param	BulkDataName			Unique name of bulk data within object, e.g. MipLevel_3
	 * @return	cooked bulk data info if found, NULL otherwise
	 */
	FCookedBulkDataInfo* RetrieveInfo( UObject* Object, const TCHAR* BulkDataName );

	/**
	 * Gathers bulk data infos of objects in passed in Outer.
	 *
	 * @param	Outer	Outer to use for object traversal when looking for bulk data
	 */
	void GatherCookedInfos( UObject* Outer );

	/**
	 * Dumps all info data to the log.
	 */
	void DumpInfos();

protected:
	/** Map from unique name to bulk data info data */
	TMap<FString,FCookedBulkDataInfo> CookedBulkDataInfoMap;
	/** Filename to use for serialization */
	FString	Filename;
};

/*------------------------------------------------------------------------------
	The End.
------------------------------------------------------------------------------*/
