/*=============================================================================
	Core utility archive classes.  Must be seperate from UnArc.h since UnTemplate.h
	references FArchive implementation.
				
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __UNARCHIVE_H__
#define __UNARCHIVE_H__

/**
 * Archive for counting memory usage.
 */
class FArchiveCountMem : public FArchive
{
public:
	FArchiveCountMem( UObject* Src )
	:	Num(0)
	,	Max(0)
	{
		ArIsCountingMemory = TRUE;
		Src->Serialize( *this );
	}
	SIZE_T GetNum()
	{
		return Num;
	}
	SIZE_T GetMax()
	{
		return Max;
	}
	void CountBytes( SIZE_T InNum, SIZE_T InMax )
	{
		Num += InNum;
		Max += InMax;
	}
	/**
  	 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
	 * is in when a loading error occurs.
	 *
	 * This is overridden for the specific Archive Types
	 **/
	virtual FName GetArchiveName() const { return TEXT("FArchiveCountMem"); }

protected:
	SIZE_T Num, Max;
};

/**
 * Base class for serializing arbitrary data in memory.
 */
class FMemoryArchive : public FArchive
{
public:
	FMemoryArchive( TArray<BYTE>& InBytes )
	: FArchive(), Offset(0), Bytes(InBytes)
	{
	}
	/**
  	 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
	 * is in when a loading error occurs.
	 *
	 * This is overridden for the specific Archive Types
	 **/
	virtual FName GetArchiveName() const { return TEXT("FMemoryArchive"); }

	void Seek( INT InPos )
	{
		Offset = InPos;
	}
	INT Tell()
	{
		return Offset;
	}
	INT TotalSize()
	{
		return Bytes.Num();
	}

protected:
	INT				Offset;
	TArray<BYTE>&	Bytes;
};

/**
 * Archive for storing arbitrary data to the specified memory location
 */
class FMemoryWriter : public FMemoryArchive
{
public:
	FMemoryWriter( TArray<BYTE>& InBytes, UBOOL bIsPersistent = FALSE )
	: FMemoryArchive(InBytes)
	{
		ArIsSaving		= TRUE;
		ArIsPersistent	= bIsPersistent;
	}

	void Serialize( void* Data, INT Num )
	{
		INT NumBytesToAdd = Offset + Num - Bytes.Num();
		if( NumBytesToAdd > 0 )
		{
			Bytes.Add( NumBytesToAdd );
		}
		if( Num )
		{
			appMemcpy( &Bytes(Offset), Data, Num );
			Offset+=Num;
		}
	}
	/**
  	 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
	 * is in when a loading error occurs.
	 *
	 * This is overridden for the specific Archive Types
	 **/
	virtual FName GetArchiveName() const { return TEXT("FMemoryWriter"); }
};


/**
 * Buffer archiver.
 */
class FBufferArchive : public FMemoryWriter, public TArray<BYTE>
{
public:
	FBufferArchive( UBOOL bIsPersistent=FALSE )
	: FMemoryWriter( (TArray<BYTE>&)*this, bIsPersistent )
	{}
	/**
  	 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
	 * is in when a loading error occurs.
	 *
	 * This is overridden for the specific Archive Types
	 **/
	virtual FName GetArchiveName() const { return TEXT("FBufferArchive"); }
};

/**
 * Archive for reading arbitrary data from the specified memory location
 */
class FMemoryReader : public FMemoryArchive
{
public:
	/**
  	 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
	 * is in when a loading error occurs.
	 *
	 * This is overridden for the specific Archive Types
	 **/
	virtual FName GetArchiveName() const { return TEXT("FMemoryReader"); }
	void Seek(INT InPos )
	{
		check(InPos<=Bytes.Num());
		FMemoryArchive::Seek(InPos);
	}

	void Serialize( void* Data, INT Num )
	{
		if (Num)
		{
			check(Offset+Num<=Bytes.Num());
			appMemcpy( Data, &Bytes(Offset), Num );
			Offset += Num;
		}
	}
	FMemoryReader( TArray<BYTE>& InBytes, UBOOL bIsPersistent = FALSE )
	: FMemoryArchive(InBytes)
	{
		ArIsLoading		= TRUE;
		ArIsPersistent	= bIsPersistent;
	}
};

/**
 * Similar to FMemoryReader, but able to internally
 * manage the memory for the buffer.
 */
class FBufferReader : public FArchive
{
public:
	/**
	 * Constructor
	 * 
	 * @param Data Buffer to use as the source data to read from
	 * @param Size Size of Data
	 * @param bInFreeOnClose If TRUE, Data will be appFree'd when this archive is closed
	 * @param bIsPersistent Uses this value for ArIsPersistent
	 * @param bInSHAVerifyOnClose It TRUE, an async SHA verification will be done on the Data buffer (bInFreeOnClose will be passed on to the async task)
	 */
	FBufferReader( void* Data, INT Size, UBOOL bInFreeOnClose, UBOOL bIsPersistent = FALSE )
	:	ReaderData			( Data )
	,	ReaderPos 			( 0 )
	,	ReaderSize			( Size )
	,	bFreeOnClose		( bInFreeOnClose )
	{
		ArIsLoading		= TRUE;
		ArIsPersistent	= bIsPersistent;
	}

	~FBufferReader()
	{
		Close();
	}
	UBOOL Close()
	{
		if( bFreeOnClose )
		{
			appFree( ReaderData );
			ReaderData = NULL;
		}
		return !ArIsError;
	}
	void Serialize( void* Data, INT Num )
	{
		check( ReaderPos >=0 );
		check( ReaderPos+Num <= ReaderSize );
		appMemcpy( Data, (BYTE*)ReaderData + ReaderPos, Num );
		ReaderPos += Num;
	}
	INT Tell()
	{
		return ReaderPos;
	}
	INT TotalSize()
	{
		return ReaderSize;
	}
	void Seek( INT InPos )
	{
		check( InPos >= 0 );
		check( InPos <= ReaderSize );
		ReaderPos = InPos;
	}
	UBOOL AtEnd()
	{
		return ReaderPos >= ReaderSize;
	}
	/**
  	 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
	 * is in when a loading error occurs.
	 *
	 * This is overridden for the specific Archive Types
	 **/
	virtual FName GetArchiveName() const { return TEXT("FBufferReader"); }
protected:
	void*	ReaderData;
	INT		ReaderPos;
	INT		ReaderSize;
	UBOOL	bFreeOnClose;
};

/**
 * Similar to FBufferReader, but will verify the contents of the buffer on close (on close to that 
 * we know we don't need the data anymore)
 */
class FBufferReaderWithSHA : public FBufferReader
{
public:
	/**
	 * Constructor
	 * 
	 * @param Data Buffer to use as the source data to read from
	 * @param Size Size of Data
	 * @param bInFreeOnClose If TRUE, Data will be appFree'd when this archive is closed
	 * @param bIsPersistent Uses this value for ArIsPersistent
	 * @param SHASourcePathname Path to the file to use to lookup the SHA hash value
	 */
	FBufferReaderWithSHA( void* Data, INT Size, UBOOL bInFreeOnClose, const TCHAR* SHASourcePathname, UBOOL bIsPersistent = FALSE )
	// we force the base class to NOT free buffer on close, as we will let the SHA task do it if needed
	: FBufferReader(Data, Size, bInFreeOnClose, bIsPersistent)
	, SourcePathname(SHASourcePathname)
	{
	}

	~FBufferReaderWithSHA()
	{
		Close();
	}

	UBOOL Close()
	{
		// don't redo if we were already closed
		if (ReaderData)
		{
			// kick off an SHA verification task to verify. this will handle any errors we get
			GThreadPool->AddQueuedWork(new FAsyncSHAVerify(ReaderData, ReaderSize, bFreeOnClose, *SourcePathname, FALSE));
			ReaderData = NULL;
		}
		
		// note that we don't allow the base class CLose to happen, as the FAsyncSHAVerify will free the buffer if needed
		return !ArIsError;
	}
	/**
  	 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
	 * is in when a loading error occurs.
	 *
	 * This is overridden for the specific Archive Types
	 **/
	virtual FName GetArchiveName() const { return TEXT("FBufferReaderWithSHA"); }

protected:
	/** Path to the file to use to lookup the SHA hash value */
	FString SourcePathname;
};

class FObjectWriter : public FMemoryWriter
{
public:
	FObjectWriter(UObject* Obj, TArray<BYTE>& InBytes)
	: FMemoryWriter(InBytes)
	{
		Obj->Serialize(*this);
	}

	virtual FArchive& operator<<( class FName& N )
	{
		NAME_INDEX Name = N.GetIndex();
		INT Number = N.GetNumber();
		ByteOrderSerialize(&Name, sizeof(Name));
		ByteOrderSerialize(&Number, sizeof(Number));
		return *this;
	}
	virtual FArchive& operator<<( class UObject*& Res )
	{
		ByteOrderSerialize(&Res, sizeof(Res));
		return *this;
	}
	/**
  	 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
	 * is in when a loading error occurs.
	 *
	 * This is overridden for the specific Archive Types
	 **/
	virtual FName GetArchiveName() const { return TEXT("FObjectWriter"); }
};

class FObjectReader : public FMemoryReader
{
public:
	FObjectReader(UObject* Obj, TArray<BYTE>& InBytes)
	: FMemoryReader(InBytes)
	{
		Obj->Serialize(*this);
	}

	virtual FArchive& operator<<( class FName& N )
	{
		NAME_INDEX Name;
		INT Number;
		ByteOrderSerialize(&Name, sizeof(Name));
		ByteOrderSerialize(&Number, sizeof(Number));
		// copy over the name with a name made from the name index and number
		N = FName((EName)Name, Number);
		return *this;
	}
	virtual FArchive& operator<<( class UObject*& Res )
	{
		ByteOrderSerialize(&Res, sizeof(Res));
		return *this;
	}
	/**
  	 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
	 * is in when a loading error occurs.
	 *
	 * This is overridden for the specific Archive Types
	 **/
	virtual FName GetArchiveName() const { return TEXT("FObjectReader"); }
};

/**
 * Archive for reloading UObjects without requiring the UObject to be completely unloaded.
 * Used for easily repropagating defaults to UObjects after class data has been changed.
 * Aggregates FMemoryReader and FMemoryWriter to encapsulate management and coordination 
 * of the UObject data being saved/loaded.
 * <p>
 * UObject references are not serialized directly into the memory archive.  Instead, we use
 * a system similar to the Export/ImportMap of ULinker - the pointer to the UObject is added
 * to a persistent (from the standpoint of the FReloadObjectArc) array.  The location into
 * this array is what is actually stored in the archive's buffer.
 * <p>
 * This is primarily necessary for loading UObject references from the memory archive.  The
 * UObject pointer ref. passed into the UObject overloaded serialization operator is not
 * guaranteed to be valid when reading data from the memory archive.  Since we need to determine
 * whether we *should* serialize the object before we actually do, we must introduce a level of
 * indirection.  Using the index retrieved from the archive's buffer, we can look at the UObject
 * before we attempt to serialize it.
 */
class FReloadObjectArc : public FArchive
{
public:

	/**
	 * Changes this memory archive to "read" mode, for reading UObject
	 * data from the temporary location back into the UObjects.
	 */
	void ActivateReader()
	{
		ArIsSaving = FALSE;
		ArIsLoading = TRUE;
	}

	/**
	 * Changes this memory archive to "write" mode, for storing UObject
	 * data to the temporary location.
	 *
	 * @note: called from ctors in child classes - should never be made virtual
	 */
	void ActivateWriter()
	{
		ArIsSaving = TRUE;
		ArIsLoading = FALSE;
	}

	/**
	 * Begin serializing a UObject into the memory archive.
	 *
	 * @param	Obj		the object to serialize
	 */
	void SerializeObject( UObject* Obj );

	/**
	 * Resets the archive so that it can be loaded from again from scratch
	 * as if it was never serialized as a Reader
	 */
	void Reset();

	/** FArchive Interface */
	INT TotalSize()
	{
		return Bytes.Num();
	}
	void Seek( INT InPos )
	{
		if ( IsLoading() )
			Reader.Seek(InPos);
		else if ( IsSaving() )
			Writer.Seek(InPos);
	}
	INT Tell()
	{
		return IsLoading() ? Reader.Tell() : Writer.Tell();
	}
	FArchive& operator<<( class FName& Name );
	FArchive& operator<<(class UObject*& Obj);

	/** Constructor */
	FReloadObjectArc();

	/** Destructor */
	virtual ~FReloadObjectArc();

	/**
  	 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
	 * is in when a loading error occurs.
	 *
	 * This is overridden for the specific Archive Types
	 **/
	virtual FName GetArchiveName() const { return TEXT("FReloadObjectArc"); }

protected:

	/**
	 * Raw I/O function.  Routes the call to the appropriate archive, depending on whether
	 * we're loading/saving.
	 */
	void Serialize( void* Data, INT Num )
	{
		if ( IsLoading() )
		{
			Reader.Serialize(Data, Num);
		}
		else if ( IsSaving() )
		{
			Writer.Serialize(Data,Num);
		}
	}

	/**
	 * Sets the root object for this memory archive.
	 * 
	 * @param	NewRoot		the UObject that should be the new root
	 */
	void SetRootObject( UObject* NewRoot );

	/** moves UObject data from storage into UObject address space */
	FMemoryReader		Reader;

	/** stores UObject data in a temporary location for later retrieval */
	FMemoryWriter		Writer;

	/** the raw UObject data contained by this archive */
	TArray<BYTE>		Bytes;

	/** UObjects for which all data is stored in the memory archive */
	TArray<UObject*>	CompleteObjects;

	/** UObjects for which only a reference to the object is stored in the memory archive */
	TArray<UObject*>	ReferencedObjects;

	/**
	 * List of top-level objects that have saved into the memory archive.  Used to prevent objects
	 * from being serialized into storage multiple times.
	 */
	TArray<UObject*>	SavedObjects;

	/**
	 * List of top-level objects that have been loaded using the memory archive.  Used to prevent
	 * objects from being serialized multiple times from the same memory archive.
	 */
	TArray<UObject*>	LoadedObjects;

	/** the objects stored in this archive */
	TMap<UObject*,INT>	ObjectMap;

	/**
	 * This is the current top-level object.  For any UObjects contained
	 * within this object, the complete UObject data will be stored in the
	 * archive's buffer
	 */
	UObject*			RootObject;

	/**
	 * Used for tracking the subobjects and components that are instanced during this object reload.
	 */
	struct FObjectInstancingGraph*	InstanceGraph;

	/**
	 * Indicates whether this archive will serialize references to objects with the RF_Transient flag. (defaults to FALSE)
	 */
	UBOOL				bAllowTransientObjects;

	/**
	 * Indicates whether this archive should call InstanceSubobjectTemplates/InstanceComponents on objects that it re-initializes.
	 * Specify FALSE if the object needs special handling before calling InstanceSubobjectTemplates (like remapping archetype refs
	 * to instance refs in the case of prefabs)
	 */
	UBOOL				bInstanceSubobjectsOnLoad;
};


/*----------------------------------------------------------------------------
	FArchetypePropagationArc.
----------------------------------------------------------------------------*/
/**
 * This specialized version of the FReloadObjectArc is designed to be used for safely propagating property changes in object archetypes to
 * instances of that archetype.  Primarily a convenience class (for now), it handles setting the appropriate flags, etc.
 */
class FArchetypePropagationArc : public FReloadObjectArc
{
public:
	/** Constructor */
	FArchetypePropagationArc();

	/**
  	 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
	 * is in when a loading error occurs.
	 *
	 * This is overridden for the specific Archive Types
	 **/
	virtual FName GetArchiveName() const { return TEXT("FArchetypePropagationArc"); }
};

/*----------------------------------------------------------------------------
	FArchiveShowReferences.
----------------------------------------------------------------------------*/
/**
 * Archive for displaying all objects referenced by a particular object.
 */
class FArchiveShowReferences : public FArchive
{
	/**
	 * I/O function.  Called when an object reference is encountered.
	 *
	 * @param	Obj		a pointer to the object that was encountered
	 */
	FArchive& operator<<( UObject*& Obj );

	/** the object to display references to */
	UObject* SourceObject;

	/** ignore references to objects have the same Outer as our Target */
	UObject* SourceOuter;

	/** output device for logging results */
	FOutputDevice& OutputAr;

	/**
	 * list of Outers to ignore;  any objects encountered that have one of
	 * these objects as an Outer will also be ignored
	 */
	class TArray<UObject*>& Exclude;

	/** list of objects that have been found */
	class TArray<UObject*> Found;

	UBOOL DidRef;

public:

	/**
	 * Constructor
	 * 
	 * @param	inOutputAr		archive to use for logging results
	 * @param	inOuter			only consider objects that do not have this object as its Outer
	 * @param	inSource		object to show references for
	 * @param	inExclude		list of objects that should be ignored if encountered while serializing SourceObject
	 */
	FArchiveShowReferences( FOutputDevice& inOutputAr, UObject* inOuter, UObject* inSource, TArray<UObject*>& InExclude );

	/**
  	 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
	 * is in when a loading error occurs.
	 *
	 * This is overridden for the specific Archive Types
	 **/
	virtual FName GetArchiveName() const { return TEXT("FArchiveShowReferences"); }
};


/*----------------------------------------------------------------------------
	FFindReferencersArchive.
----------------------------------------------------------------------------*/
/**
 * Archive for mapping out the referencers of a collection of objects.
 */
class FFindReferencersArchive : public FArchive
{
public:
	/**
	 * Constructor
	 *
	 * @param	PotentialReferencer		the object to serialize which may contain references to our target objects
	 * @param	InTargetObjects			array of objects to search for references to
	 */
	FFindReferencersArchive( class UObject* PotentialReferencer, TArray<class UObject*> InTargetObjects );

	/**
	 * Retrieves the number of references from PotentialReferencer to the object specified.
	 *
	 * @param	TargetObject	the object to might be referenced
	 * @param	out_ReferencingProperties
	 *							receives the list of properties which were holding references to TargetObject
	 *
	 * @return	the number of references to TargetObject which were encountered when PotentialReferencer
	 *			was serialized.
	 */
	INT GetReferenceCount( class UObject* TargetObject, TArray<class UProperty*>* out_ReferencingProperties=NULL ) const;

	/**
	 * Retrieves the number of references from PotentialReferencer list of TargetObjects
	 *
	 * @param	out_ReferenceCounts		receives the number of references to each of the TargetObjects
	 *
	 * @return	the number of objects which were referenced by PotentialReferencer.
	 */
	INT GetReferenceCounts( TMap<class UObject*, INT>& out_ReferenceCounts ) const;

	/**
	 * Retrieves the number of references from PotentialReferencer list of TargetObjects
	 *
	 * @param	out_ReferenceCounts			receives the number of references to each of the TargetObjects
	 * @param	out_ReferencingProperties	receives the map of properties holding references to each referenced object.
	 *
	 * @return	the number of objects which were referenced by PotentialReferencer.
	 */
	INT GetReferenceCounts( TMap<class UObject*, INT>& out_ReferenceCounts, TMultiMap<class UObject*,class UProperty*>& out_ReferencingProperties ) const;

	/**
  	 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
	 * is in when a loading error occurs.
	 *
	 * This is overridden for the specific Archive Types
	 **/
	virtual FName GetArchiveName() const { return TEXT("FFindReferencersArchive"); }

protected:
	TMap<class UObject*,INT>	TargetObjects;

	/** a mapping of target object => the properties in PotentialReferencer that hold the reference to target object */
	TMultiMap<class UObject*,class UProperty*> ReferenceMap;

private:

	/**
	 * Serializer - if Obj is one of the objects we're looking for, increments the reference count for that object
	 */
	FArchive& operator<<( class UObject*& Obj );
};


/**
 * This class is used to find which objects reference any element from a list of "TargetObjects".  When invoked,
 * it will generate a mapping of each target object to an array of objects referencing that target object.
 *
 * Each key corresponds to an element of the input TargetObjects array which was referenced
 * by some other object.  The values for these keys are the objects which are referencing them.
 */
template< class T >
class TFindObjectReferencers : public TMultiMap<T*, UObject*>
{
public:

	/**
	 * Default constructor
	 *
	 * @param	TargetObjects	the list of objects to find references to
	 * @param	PackageToCheck	if specified, only objects contained in this package will be searched
	 *							for references to 
	 */
	TFindObjectReferencers( TArray< T* > TargetObjects, UPackage* PackageToCheck=NULL )
	: TMultiMap< T*, UObject* >()
	{
		for ( TObjectIterator<UObject> It; It; ++It )
		{
			UObject* PotentialReferencer = *It;
			if (!TargetObjects.ContainsItem(Cast<T>(PotentialReferencer))
			&&	(PackageToCheck == NULL || PotentialReferencer->IsIn(PackageToCheck))
			&&	!PotentialReferencer->IsTemplate() )
			{
				FFindReferencersArchive FindReferencerAr(PotentialReferencer, (TArray<UObject*>&)TargetObjects);

				TMap<UObject*,INT> ReferenceCounts;
				if ( FindReferencerAr.GetReferenceCounts(ReferenceCounts) > 0 )
				{
					// here we don't really care about the number of references from PotentialReferencer to the target object...just that he's a referencer
					TArray<UObject*> ReferencedObjects;
					ReferenceCounts.GenerateKeyArray(ReferencedObjects);
					for ( INT RefIndex = 0; RefIndex < ReferencedObjects.Num(); RefIndex++ )
					{
						Add(static_cast<T*>(ReferencedObjects(RefIndex)), PotentialReferencer);
					}
				}
			}
		}
	}

private:
	/**
	 * This is a mapping of TargetObjects to the list of objects which references each one combined with
	 * the list of properties which are holding the reference to the TargetObject in that referencer.
	 *
	 * @todo - not yet implemented
	 */
//	TMap< T*, TMultiMap<UObject*, UProperty*> >		ReferenceProperties;
};

/*----------------------------------------------------------------------------
	FArchiveFindCulprit.
----------------------------------------------------------------------------*/
/**
 * Archive for finding who references an object.
 */
class FArchiveFindCulprit : public FArchive
{
public:
	/**
	 * Constructor
	 *
	 * @param	InFind	the object that we'll be searching for references to
	 * @param	Src		the object to serialize which may contain a reference to InFind
	 * @param	InPretendSaving		if TRUE, marks the archive as saving and persistent, so that a different serialization codepath is followed
	 */
	FArchiveFindCulprit( UObject* InFind, UObject* Src, UBOOL InPretendSaving );

	INT GetCount() const
	{
		return Count;
	}
	INT GetCount( TArray<UProperty*>& Properties )
	{
		Properties = Referencers;
		return Count;
	}

	/**
  	 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
	 * is in when a loading error occurs.
	 *
	 * This is overridden for the specific Archive Types
	 **/
	virtual FName GetArchiveName() const { return TEXT("FArchiveFindCulprit"); }

protected:
	UObject*			Find;
	INT					Count;
	UBOOL				PretendSaving;
	class TArray<UProperty*>	Referencers;

private:
	FArchive& operator<<( class UObject*& Obj );
};

struct FTraceRouteRecord
{
	INT Depth;
	UObject* Referencer;
	UProperty* ReferencerProperty;

	FTraceRouteRecord( INT InDepth, UObject* InReferencer )
	: Depth(InDepth), Referencer(InReferencer), ReferencerProperty(GSerializedProperty)
	{}
};


/*----------------------------------------------------------------------------
	FArchiveFindCulprit.
----------------------------------------------------------------------------*/

/**
 * Archive for finding shortest path from root to a particular object.
 * Depth-first search.
 */
class FArchiveTraceRoute : public FArchive
{
public:
	static TMap<UObject*,UProperty*> FindShortestRootPath( UObject* Object, UBOOL bIncludeTransients, EObjectFlags KeepFlags );

	/**
	 * Retuns path to root created by e.g. FindShortestRootPath via a string.
	 *
	 * @param TargetObject	object marking one end of the route
	 * @param Route			route to print to log.
	 * @param String of root path
	 */
	static FString PrintRootPath( const TMap<UObject*,UProperty*>& Route, const UObject* TargetObject );

	/**
  	 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
	 * is in when a loading error occurs.
	 *
	 * This is overridden for the specific Archive Types
	 **/
	virtual FName GetArchiveName() const { return TEXT("FArchiveTraceRoute"); }

private:
	FArchiveTraceRoute( TMap<UObject*,FTraceRouteRecord>& InRoutes, UBOOL bShouldIncludeTransients, EObjectFlags KeepFlags );
	FArchive& operator<<( class UObject*& Obj );

	TMap<UObject*,FTraceRouteRecord>& Routes;
	INT Depth;

	/** The object through which we reached the objects we're serializing */
	UObject* Referencer;

	/** TRUE if we should serialize objects marked RF_Transient */
	UBOOL bIncludeTransients;
};

/*----------------------------------------------------------------------------
	FDuplicateDataReader.
----------------------------------------------------------------------------*/
/**
 * Stores a copy of an object along with its component instances.
 */
struct FDuplicatedObjectInfo
{
	UObject*						DupObject;
	TMap<UComponent*,UComponent*>	ComponentInstanceMap;
};

/**
 * Reads duplicated objects from a memory buffer, replacing object references to duplicated objects.
 */
class FDuplicateDataReader : public FArchive
{
private:

	const TMap<UObject*,FDuplicatedObjectInfo*>&	DuplicatedObjects;
	const TArray<BYTE>&								ObjectData;
	INT												Offset;

	// FArchive interface.

	virtual FArchive& operator<<(FName& N)
	{
		NAME_INDEX Name;
		INT Number;
		ByteOrderSerialize(&Name, sizeof(Name));
		ByteOrderSerialize(&Number, sizeof(Number));
		// copy over the name with a name made from the name index and number
		N = FName((EName)Name, Number);
		return *this;
	}

	virtual FArchive& operator<<(UObject*& Object);

	virtual void Serialize(void* Data,INT Num)
	{
		if(Num)
		{
			check(Offset + Num <= ObjectData.Num());
			appMemcpy(Data,&ObjectData(Offset),Num);
			Offset += Num;
		}
	}

	virtual void Seek(INT InPos)
	{
		Offset = InPos;
	}

public:
	/**
  	 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
	 * is in when a loading error occurs.
	 *
	 * This is overridden for the specific Archive Types
	 **/
	virtual FName GetArchiveName() const { return TEXT("FDuplicateDataReader"); }

	virtual INT Tell()
	{
		return Offset;
	}
	virtual INT TotalSize()
	{
		return ObjectData.Num();
	}

	/**
	 * Constructor
	 * 
	 * @param	InDuplicatedObjects		map of original object to copy of that object
	 * @param	InObjectData			object data to read from
	 */
	FDuplicateDataReader(const TMap<UObject*,FDuplicatedObjectInfo*>& InDuplicatedObjects,const TArray<BYTE>& InObjectData);
};

/*----------------------------------------------------------------------------
	FDuplicateDataWriter.
----------------------------------------------------------------------------*/
/**
 * Writes duplicated objects to a memory buffer, duplicating referenced inner objects and adding the duplicates to the DuplicatedObjects map.
 */
class FDuplicateDataWriter : public FArchive
{
private:

	TMap<UObject*,FDuplicatedObjectInfo*>&	DuplicatedObjects;
	TArray<BYTE>&							ObjectData;
	INT										Offset;
	EObjectFlags							FlagMask;
	EObjectFlags							ApplyFlags;

	/**
	 * This is used to prevent object & component instancing resulting from the calls to StaticConstructObject(); instancing subobjects and components is pointless,
	 * since we do that manually and replace the current value with our manually created object anyway.
	 */
	struct FObjectInstancingGraph*			InstanceGraph;

	// FArchive interface.

	virtual FArchive& operator<<(FName& N)
	{
		NAME_INDEX Name = N.GetIndex();
		INT Number = N.GetNumber();
		ByteOrderSerialize(&Name, sizeof(Name));
		ByteOrderSerialize(&Number, sizeof(Number));
		return *this;
	}

	virtual FArchive& operator<<(UObject*& Object);

	virtual void Serialize(void* Data,INT Num)
	{
		// Don't try to add/memcpy zero sized items
		if (Data != NULL && Num > 0)
		{
			if(Offset == ObjectData.Num())
			{
				ObjectData.Add(Num);
			}
			appMemcpy(&ObjectData(Offset),Data,Num);
			Offset += Num;
		}
	}

	virtual void Seek(INT InPos)
	{
		Offset = InPos;
	}

	/**
	 * Places a new duplicate in the DuplicatedObjects map as well as the UnserializedObjects list
	 * 
	 * @param	SourceObject	the original version of the object
	 * @param	DuplicateObject	the copy of the object
	 *
	 * @return	a pointer to the copy of the object
	 */
	UObject* AddDuplicate(UObject* SourceObject,UObject* DuplicateObject);

public:
	/**
  	 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
	 * is in when a loading error occurs.
	 *
	 * This is overridden for the specific Archive Types
	 **/
	virtual FName GetArchiveName() const { return TEXT("FDuplicateDataWriter"); }

	virtual INT Tell()
	{
		return Offset;
	}

	virtual INT TotalSize()
	{
		return ObjectData.Num();
	}
	TArray<UObject*>	UnserializedObjects;

	/**
	 * Returns a pointer to the duplicate of a given object, creating the duplicate object if necessary.
	 * 
	 * @param	Object	the object to find a duplicate for
	 *
	 * @return	a pointer to the duplicate of the specified object
	 */
	UObject* GetDuplicatedObject(UObject* Object);

	/**
	 * Constructor
	 * 
	 * @param	InDuplicatedObjects		will contain the original object -> copy mappings
	 * @param	InObjectData			will store the serialized data
	 * @param	SourceObject			the object to copy
	 * @param	DestObject				the object to copy to
	 * @param	InFlagMask				the flags that should be copied when the object is duplicated
	 * @param	InApplyFlags			the flags that should always be set on the duplicated objects (regardless of whether they're set on the source)
	 * @param	InInstanceGraph			the instancing graph to use when creating the duplicate objects.
	 */
	FDuplicateDataWriter(TMap<UObject*,FDuplicatedObjectInfo*>& InDuplicatedObjects,TArray<BYTE>& InObjectData,UObject* SourceObject,UObject* DestObject,EObjectFlags InFlagMask,EObjectFlags InApplyMask,struct FObjectInstancingGraph* InInstanceGraph);
};


/*----------------------------------------------------------------------------
	Saving Packages.
----------------------------------------------------------------------------*/
/**
 * Archive for tagging objects and names that must be exported
 * to the file.  It tags the objects passed to it, and recursively
 * tags all of the objects this object references.
 */
class FArchiveSaveTagExports : public FArchive
{
public:
	/**
	 * Constructor
	 * 
	 * @param	InOuter		the package to save
	 */
	FArchiveSaveTagExports( UObject* InOuter )
	: Outer(InOuter)
	{
		ArIsSaving				= TRUE;
		ArIsPersistent			= TRUE;
		ArShouldSkipBulkData	= TRUE;
	}
	FArchive& operator<<( UObject*& Obj );

	/**
	 * Package we're currently saving.  Only objects contained
	 * within this package will be tagged for serialization.
	 */
	UObject* Outer;

	/**
  	 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
	 * is in when a loading error occurs.
	 *
	 * This is overridden for the specific Archive Types
	 **/
	virtual FName GetArchiveName() const;
};

/**
 * Archive for tagging objects and names that must be listed in the
 * file's imports table.
 */
class FArchiveSaveTagImports : public FArchive
{
public:
	ULinkerSave* Linker;
	EObjectFlags ContextFlags;
	TArray<UObject*> Dependencies;

	FArchiveSaveTagImports( ULinkerSave* InLinker, EObjectFlags InContextFlags )
	: Linker( InLinker ), ContextFlags( InContextFlags )
	{
		ArIsSaving				= TRUE;
		ArIsPersistent			= TRUE;
		ArShouldSkipBulkData	= TRUE;
		if ( Linker != NULL )
		{
			ArPortFlags = ((FArchive*)Linker)->GetPortFlags();
		}
	}
	FArchive& operator<<( UObject*& Obj );
	FArchive& operator<<( FName& Name )
	{
		Name.SetFlags( RF_TagExp | ContextFlags );
		return *this;
	}

	/**
  	 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
	 * is in when a loading error occurs.
	 *
	 * This is overridden for the specific Archive Types
	 **/
	virtual FName GetArchiveName() const;
};

/*----------------------------------------------------------------------------
	FArchiveReplaceObjectRef.
----------------------------------------------------------------------------*/
/**
 * Archive for replacing a reference to an object. This classes uses
 * serialization to replace all references to one object with another.
 * Note that this archive will only traverse objects with an Outer
 * that matches InSearchObject.
 *
 * NOTE: The template type must be a child of UObject or this class will not compile.
 */
template< class T >
class FArchiveReplaceObjectRef : public FArchive
{
public:
	/**
	 * Initializes variables and starts the serialization search
	 *
	 * @param InSearchObject		The object to start the search on
	 * @param ReplacementMap		Map of objects to find -> objects to replace them with (null zeros them)
	 * @param bNullPrivateRefs		Whether references to non-public objects not contained within the SearchObject
	 *								should be set to null
	 * @param bIgnoreOuterRef		Whether we should replace Outer pointers on Objects.
	 * @param bIgnoreArchetypeRef	Whether we should replace the ObjectArchetype reference on Objects.
	 * @param bDelayStart			Specify TRUE to prevent the constructor from starting the process.  Allows child classes' to do initialization stuff in their ctor
	 */
	FArchiveReplaceObjectRef
	(
		UObject* InSearchObject,
		const TMap<T*,T*>& inReplacementMap,
		UBOOL bNullPrivateRefs,
		UBOOL bIgnoreOuterRef,
		UBOOL bIgnoreArchetypeRef,
		UBOOL bDelayStart=FALSE
	)
	: SearchObject(InSearchObject), ReplacementMap(inReplacementMap)
	, Count(0), bNullPrivateReferences(bNullPrivateRefs)
	{
		ArIsObjectReferenceCollector = TRUE;
		ArIgnoreArchetypeRef = bIgnoreArchetypeRef;
		ArIgnoreOuterRef = bIgnoreOuterRef;

		if ( !bDelayStart )
		{
			SerializeSearchObject();
		}
	}

	/**
	 * Starts the serialization of the root object
	 */
	void SerializeSearchObject()
	{
		if (SearchObject != NULL && !SerializedObjects.HasKey(SearchObject)
		&&	(ReplacementMap.Num() > 0 || bNullPrivateReferences))
		{
			// start the initial serialization
			SerializedObjects.AddItem(SearchObject);

			// serialization for class default objects must be deterministic (since class 
			// default objects may be serialized during script compilation while the script
			// and C++ versions of a class are not in sync), so use SerializeTaggedProperties()
			// rather than the native Serialize() function
			if ( SearchObject->HasAnyFlags(RF_ClassDefaultObject) )
			{
				UClass* ObjectClass = SearchObject->GetClass();
				StartSerializingDefaults();
				if ( !IsTransacting() && (IsLoading() || IsSaving()) )
				{
					ObjectClass->SerializeTaggedProperties(*this, (BYTE*)SearchObject, ObjectClass, NULL);
				}
				else
				{
					ObjectClass->SerializeBin(*this, (BYTE*)SearchObject, 0);
				}
				StopSerializingDefaults();
			}
			else
			{
				SearchObject->Serialize(*this);
			}
		}
	}

	/**
	 * Returns the number of times the object was referenced
	 */
	INT GetCount()
	{
		return Count;
	}

	/**
	 * Returns a reference to the object this archive is operating on
	 */
	const UObject* GetSearchObject() const { return SearchObject; }

	/**
	 * Serializes the reference to the object
	 */
	FArchive& operator<<( UObject*& Obj )
	{
		if (Obj != NULL)
		{
			// If these match, replace the reference
			T* const* ReplaceWith = (T*const*)((const TMap<UObject*,UObject*>*)&ReplacementMap)->Find(Obj);
			if ( ReplaceWith != NULL )
			{
				Obj = *ReplaceWith;
				Count++;
			}
			// A->IsIn(A) returns FALSE, but we don't want to NULL that reference out, so extra check here.
			else if ( Obj == SearchObject || Obj->IsIn(SearchObject) )
			{
#if 0
				// DEBUG: Log when we are using the A->IsIn(A) path here.
				if(Obj == SearchObject)
				{
					FString ObjName = Obj->GetPathName();
					debugf( TEXT("FArchiveReplaceObjectRef: Obj == SearchObject : '%s'"), *ObjName );
				}
#endif

				if ( !SerializedObjects.HasKey(Obj) )
				{
					// otherwise recurse down into the object if it is contained within the initial search object
					SerializedObjects.AddItem(Obj);
	
					// serialization for class default objects must be deterministic (since class 
					// default objects may be serialized during script compilation while the script
					// and C++ versions of a class are not in sync), so use SerializeTaggedProperties()
					// rather than the native Serialize() function
					if ( Obj->HasAnyFlags(RF_ClassDefaultObject) )
					{
						UClass* ObjectClass = Obj->GetClass();
						StartSerializingDefaults();
						if ( !IsTransacting() && (IsLoading() || IsSaving()) )
						{
							ObjectClass->SerializeTaggedProperties(*this, (BYTE*)Obj, ObjectClass, NULL);
						}
						else
						{
							ObjectClass->SerializeBin(*this, (BYTE*)Obj, 0);
						}
						StopSerializingDefaults();
					}
					else
					{
						Obj->Serialize(*this);
					}
				}
			}
			else if ( bNullPrivateReferences && !Obj->HasAnyFlags(RF_Public) )
			{
				Obj = NULL;
			}
		}
		return *this;
	}

	/**
  	 * Returns the name of this archive.
	 **/
	virtual FName GetArchiveName() const { return FName(TEXT("ReplaceObjectRef")); }

protected:
	/** Initial object to start the reference search from */
	UObject* SearchObject;

	/** Map of objects to find references to -> object to replace references with */
	const TMap<T*,T*>& ReplacementMap;
	
	/** The number of times encountered */
	INT Count;

	/** List of objects that have already been serialized */
	TLookupMap<UObject*> SerializedObjects;

	/**
	 * Whether references to non-public objects not contained within the SearchObject
	 * should be set to null
	 */
	UBOOL bNullPrivateReferences;
};

/*----------------------------------------------------------------------------
	FArchiveObjectReferenceCollector.
----------------------------------------------------------------------------*/
template < class T >
class TArchiveObjectReferenceCollector : public FArchive
{
public:

	/**
	 * Constructor
	 *
	 * @param	InObjectArray			Array to add object references to
	 * @param	InOuter					value for LimitOuter
	 * @param	bInRequireDirectOuter	value for bRequireDirectOuter
	 * @param	bShouldIgnoreArchetype	whether to disable serialization of ObjectArchetype references
	 * @param	bInSerializeRecursively	only applicable when LimitOuter != NULL && bRequireDirectOuter==TRUE;
	 *									serializes each object encountered looking for subobjects of referenced
	 *									objects that have LimitOuter for their Outer (i.e. nested subobjects/components)
	 * @param	bShouldIgnoreTransient	TRUE to skip serialization of transient properties
	 */
	TArchiveObjectReferenceCollector( TArray<T*>* InObjectArray, UObject* InOuter=NULL, UBOOL bInRequireDirectOuter=TRUE, UBOOL bShouldIgnoreArchetype=FALSE, UBOOL bInSerializeRecursively=FALSE, UBOOL bShouldIgnoreTransient=FALSE )
	:	ObjectArray( InObjectArray )
	,	LimitOuter(InOuter)
	,	bRequireDirectOuter(bInRequireDirectOuter)
	{
		ArIsObjectReferenceCollector = TRUE;
		ArIsPersistent = bShouldIgnoreTransient;
		ArIgnoreArchetypeRef = bShouldIgnoreArchetype;
		bSerializeRecursively = bInSerializeRecursively && LimitOuter != NULL/* && bRequireDirectOuter*/; // hmmm, why was I previously requiring that bRequireDirectOuter be set in order to do a deep search
	}
protected:

	/** 
	 * UObject serialize operator implementation
	 *
	 * @param Object	reference to Object reference
	 * @return reference to instance of this class
	 */
	FArchive& operator<<( UObject*& Object )
	{
		// Avoid duplicate entries.
		if ( Object != NULL )
		{
			if ( LimitOuter == NULL || (Object->GetOuter() == LimitOuter || (!bRequireDirectOuter && Object->IsIn(LimitOuter))) )
			{
				// do not attempt to serialize objects that have already been 
				if ( Object->IsA(T::StaticClass()) && ObjectArray->FindItemIndex((T*)Object) == INDEX_NONE )
				{
					ObjectArray->AddItem( (T*)Object );
				}

				// check this object for any potential object references
				if ( bSerializeRecursively == TRUE && SerializedObjects.FindItemIndex(Object) == INDEX_NONE )
				{
					SerializedObjects.AddItem(Object);
					Object->Serialize(*this);
				}
			}
		}
		
		return *this;
	}

	/** Stored pointer to array of objects we add object references to */
	TArray<T*>*		ObjectArray;

	/** List of objects that have been recursively serialized */
	TArray<UObject*> SerializedObjects;

	/** only objects within this outer will be considered, NULL value indicates that outers are disregarded */
	UObject*		LimitOuter;

	/** determines whether nested objects contained within LimitOuter are considered */
	UBOOL			bRequireDirectOuter;

	/** determines whether we serialize objects that are encounterd by this archive */
	UBOOL			bSerializeRecursively;
};

/*----------------------------------------------------------------------------
	FArchiveObjectReferenceCollector.
----------------------------------------------------------------------------*/
/**
 * Helper implementation of FArchive used to collect object references, avoiding duplicate entries.
 */
class FArchiveObjectReferenceCollector : public TArchiveObjectReferenceCollector<UObject>
{
public:
	/**
	 * Constructor
	 *
	 * @param	InObjectArray			Array to add object references to
	 * @param	InOuter					value for LimitOuter
	 * @param	bInRequireDirectOuter	value for bRequireDirectOuter
	 * @param	bShouldIgnoreArchetype	whether to disable serialization of ObjectArchetype references
	 * @param	bInSerializeRecursively	only applicable when LimitOuter != NULL && bRequireDirectOuter==TRUE;
	 *									serializes each object encountered looking for subobjects of referenced
	 *									objects that have LimitOuter for their Outer (i.e. nested subobjects/components)
	 * @param	bShouldIgnoreTransient	TRUE to skip serialization of transient properties
	 */
	FArchiveObjectReferenceCollector( TArray<UObject*>* InObjectArray, UObject* InOuter=NULL, UBOOL bInRequireDirectOuter=TRUE, UBOOL bShouldIgnoreArchetypes=FALSE, UBOOL bInSerializeRecursively=FALSE, UBOOL bShouldIgnoreTransient=FALSE )
	:	TArchiveObjectReferenceCollector<UObject>( InObjectArray, InOuter, bInRequireDirectOuter, bShouldIgnoreArchetypes, bInSerializeRecursively, bShouldIgnoreTransient )
	{
	}

private:
	/** 
	 * UObject serialize operator implementation
	 *
	 * @param Object	reference to Object reference
	 * @return reference to instance of this class
	 */
	FArchive& operator<<( UObject*& Object );
};

/*----------------------------------------------------------------------------
	FArchiveObjectPropertyMapper.
----------------------------------------------------------------------------*/

/**
 * Class for collecting references to objects, along with the properties that
 * references that object.
 */
class FArchiveObjectPropertyMapper : public FArchive
{
public:
	/**
	 * Constructor
	 *
	 * @param	InObjectArray			Array to add object references to
	 * @param	InBase					only objects with this outer will be considered, or NULL to disregard outers
	 * @param	InLimitClass			only objects of this class (and children) will be considered, or null to disregard object class
	 * @param	bInRequireDirectOuter	determines whether objects contained within 'InOuter', but that do not have an Outer
	 *									of 'InOuter' are included.  i.e. for objects that have GetOuter()->GetOuter() == InOuter.
	 *									If InOuter is NULL, this parameter is ignored.
	 * @param	bInSerializeRecursively	only applicable when LimitOuter != NULL && bRequireDirectOuter==TRUE;
	 *									serializes each object encountered looking for subobjects of referenced
	 *									objects that have LimitOuter for their Outer (i.e. nested subobjects/components)
	 */
	FArchiveObjectPropertyMapper( TMap<UProperty*,UObject*>* InObjectGraph, UObject* InOuter=NULL, UClass* InLimitClass=NULL, UBOOL bInRequireDirectOuter=TRUE, UBOOL bInSerializeRecursively=TRUE )
	:	ObjectGraph( InObjectGraph ), LimitOuter(InOuter), LimitClass(InLimitClass), bRequireDirectOuter(bInRequireDirectOuter), bSerializeRecursively(bInSerializeRecursively)
	{
		ArIsObjectReferenceCollector = TRUE;
		bSerializeRecursively = bInSerializeRecursively && LimitOuter != NULL;
	}
private:
	/** 
	 * UObject serialize operator implementation
	 *
	 * @param Object	reference to Object reference
	 * @return reference to instance of this class
	 */
	FArchive& operator<<( UObject*& Object )
	{
		// Avoid duplicate entries.
		if ( Object != NULL )
		{
			if ((LimitClass == NULL || Object->IsA(LimitClass)) &&
				(LimitOuter == NULL || (Object->GetOuter() == LimitOuter || (!bRequireDirectOuter && Object->IsIn(LimitOuter)))) )
			{
				ObjectGraph->Set(GSerializedProperty, Object);
				if ( bSerializeRecursively && !ObjectArray.ContainsItem(Object) )
				{
					ObjectArray.AddItem( Object );

					// check this object for any potential object references
					Object->Serialize(*this);
				}
			}
		}

		return *this;
	}

	/** Tracks the objects which have been serialized by this archive, to prevent recursion */
	TArray<UObject*>			ObjectArray;

	/** Stored pointer to array of objects we add object references to */
	TMap<UProperty*,UObject*>*	ObjectGraph;

	/** only objects with this outer will be considered, NULL value indicates that outers are disregarded */
	UObject*			LimitOuter;

	/** only objects of this type will be considered, NULL value indicates that all classes are considered */
	UClass*				LimitClass;

	/** determines whether nested objects contained within LimitOuter are considered */
	UBOOL				bRequireDirectOuter;

	/** determines whether we serialize objects that are encounterd by this archive */
	UBOOL				bSerializeRecursively;
};


/*----------------------------------------------------------------------------
	FArchiveAsync.
----------------------------------------------------------------------------*/

/**
 * Rough and basic version of async archive. The code relies on Serialize only ever to be called on the last
 * precached region.
 */
class FArchiveAsync : public FArchive
{
public:
	/**
 	 * Constructor, initializing member variables.
	 */
	FArchiveAsync( const TCHAR* InFileName );

	/**
	 * Virtual destructor cleaning up internal file reader.
	 */
	virtual ~FArchiveAsync();

	/**
	 * Close archive and return whether there has been an error.
	 *
	 * @return	TRUE if there were NO errors, FALSE otherwise
	 */
	virtual UBOOL Close();

	/**
	 * Sets mapping from offsets/ sizes that are going to be used for seeking and serialization to what
	 * is actually stored on disk. If the archive supports dealing with compression in this way it is 
	 * going to return TRUE.
	 *
	 * @param	CompressedChunks	Pointer to array containing information about [un]compressed chunks
	 * @param	CompressionFlags	Flags determining compression format associated with mapping
	 *
	 * @return TRUE if archive supports translating offsets & uncompressing on read, FALSE otherwise
	 */
	virtual UBOOL SetCompressionMap( TArray<FCompressedChunk>* CompressedChunks, ECompressionFlags CompressionFlags );

	/**
	 * Hint the archive that the region starting at passed in offset and spanning the passed in size
	 * is going to be read soon and should be precached.
	 *
	 * The function returns whether the precache operation has completed or not which is an important
	 * hint for code knowing that it deals with potential async I/O. The archive is free to either not 
	 * implement this function or only partially precache so it is required that given sufficient time
	 * the function will return TRUE. Archives not based on async I/O should always return TRUE.
	 *
	 * This function will not change the current archive position.
	 *
	 * @param	PrecacheOffset	Offset at which to begin precaching.
	 * @param	PrecacheSize	Number of bytes to precache
	 * @return	FALSE if precache operation is still pending, TRUE otherwise
	 */
	virtual UBOOL Precache( INT PrecacheOffset, INT PrecacheSize );

	/**
	 * Serializes data from archive.
	 *
	 * @param	Data	Pointer to serialize to
	 * @param	Num		Number of bytes to read
	 */
	virtual void Serialize( void* Data, INT Num );

	/**
	 * Returns the current position in the archive as offset in bytes from the beginning.
	 *
	 * @return	Current position in the archive (offset in bytes from the beginning)
	 */
	virtual INT Tell();
	/**
	 * Returns the total size of the archive in bytes.
	 *
	 * @return total size of the archive in bytes
	 */
	virtual INT TotalSize();

	/**
	 * Sets the current position.
	 *
	 * @param InPos	New position (as offset from beginning in bytes)
	 */
	virtual void Seek( INT InPos );

private:
	/**
	 * Flushes cache and frees internal data.
	 */
	void FlushCache();

	/**
	 * Swaps current and next buffer. Relies on calling code to ensure that there are no outstanding
	 * async read operations into the buffers.
	 */
	void BufferSwitcheroo();

	/**
	 * Whether the current precache buffer contains the passed in request.
	 *
	 * @param	RequestOffset	Offset in bytes from start of file
	 * @param	RequestSize		Size in bytes requested
	 *
	 * @return TRUE if buffer contains request, FALSE othwerise
	 */
	FORCEINLINE UBOOL PrecacheBufferContainsRequest( INT RequestOffset, INT RequestSize );

	/**
	 * Finds and returns the compressed chunk index associated with the passed in offset.
	 *
	 * @param	RequestOffset	Offset in file to find associated chunk index for
	 *
	 * @return Index into CompressedChunks array matching this offset
	 */
	INT FindCompressedChunkIndex( INT RequestOffset );

	/**
	 * Precaches compressed chunk of passed in index using buffer at passed in index.
	 *
	 * @param	ChunkIndex	Index of compressed chunk
	 * @param	BufferIndex	Index of buffer to precache into	
	 */
	void PrecacheCompressedChunk( INT ChunkIndex, INT BufferIndex );

	/** Anon enum used to index precache data. */
	enum
	{
		CURRENT = 0,
		NEXT	= 1,
	};

	/** Cached filename for debugging.												*/
	FString							FileName;
	/** Cached file size															*/
	INT								FileSize;
	/** Cached uncompressed file size (!= FileSize for compressed packages.			*/
	INT								UncompressedFileSize;
	/** Current position of archive.												*/
	INT								CurrentPos;

	/** Start position of current precache request.									*/
	INT								PrecacheStartPos[2];
	/** End position (exclusive) of current precache request.						*/
	INT								PrecacheEndPos[2];
	/** Buffer containing precached data.											*/
	BYTE*							PrecacheBuffer[2];
	/** Status of pending read, a value of 0 means no outstanding reads.			*/
	FThreadSafeCounter				PrecacheReadStatus[2];
	
	/** Mapping of compressed <-> uncompresses sizes and offsets, NULL if not used.	*/
	TArray<FCompressedChunk>*		CompressedChunks;
	/** Current index into compressed chunks array.									*/
	INT								CurrentChunkIndex;
	/** Compression flags determining compression of CompressedChunks.				*/
	ECompressionFlags				CompressionFlags;
};


#endif	// __UNARCHIVE_H__

