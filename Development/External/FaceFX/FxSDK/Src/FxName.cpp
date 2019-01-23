//------------------------------------------------------------------------------
// This class is responsible for managing the names of all the named objects
// in the system.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "FxName.h"
#include "FxUtil.h"
#include "FxList.h"
#include "FxRefString.h"
#include "FxAnim.h"
#include "FxActor.h"

#ifdef FX_TRACK_NAME_STATS
	#pragma message( "FX_TRACK_NAME_STATS is defined." )
#endif

#ifdef FX_DISALLOW_SPACES_IN_NAMES
	#pragma message( "FX_DISALLOW_SPACES_IN_NAMES is defined." )
#endif

namespace OC3Ent
{

namespace Face
{

// Some useful macros for the string hashing routine.
#define FxHashSize(x) ((FxUInt32)1<<(x))
#define FxHashMask(x) (FxHashSize(x)-1)

// The maximum number of characters a name may contain.  If a name contains
// more characters than this then the name is truncated before being added
// into the name system.
#define FX_MAX_NAME_LENGTH 512
// The mask used for masking out parts of the 32-bit hash value to prevent a 
// mod operation.  A value of 12 results in 4096 hash bins.  FX_NAME_HASH_SIZE
// will become 2^FX_NAME_HASH_MASK.  The hashing routines below are optimized
// to result in zero collisions in 2^32.  Since a hash table with 2^32 elements
// is completely out of our scope, we need to mask the 32-bit result to fit in
// our hash table.  As such, these routines will seem to produce more collisions
// than they should theoretically.
#define FX_NAME_HASH_MASK  12
// The size of the hash table (the number of hash bins - defaults to 4096 with a
// FX_NAME_HASH_MASK of 12).
#define FX_NAME_HASH_SIZE  FxHashSize(FX_NAME_HASH_MASK)

// Hashes the specified string of the specified length and automatically masks
// the result to FX_NAME_HASH_MASK bits ensuring that the result will be 
// between zero and FX_NAME_HASH_SIZE.  This string hashing function was 
// developed by Professor Daniel J. Bernstein and was first published to 
// comp.lang.c long ago.  It is highly regarded as one of the most efficient 
// hash functions ever published.
FX_INLINE FxUInt32 FxHashString( const FxChar* string, FxSize length )
{
	// DJB's magic hash constant.
	FxSize hash = 5381;
	for( FxSize i = 0; i < length; ++i )
	{
		// This is equivalent to hash * 33 + string[i].  33 is another one of
		// DJB's magic hash constants.
		hash = ((hash << 5) + hash) + string[i];
	}
	// Return a result masked by FX_NAME_HASH_MASK.
	return (hash & FxHashMask(FX_NAME_HASH_MASK));
}

// An entry in the name hash.  An entry in the hash is simply a pointer to 
// a pointer to a RefString.  This is the same pointer stored in an actual
// FxName representing the name.
struct FxNameHashEntry
{
	// Default constructor.
	FxNameHashEntry() : ppRefString(NULL) {}
	// Construct from a pointer to a pointer to a RefString.
	FxNameHashEntry( FxRefString** ppInRefString ) 
		: ppRefString(ppInRefString) {}
	// Equality operator.
	FX_INLINE FxBool operator==( const FxNameHashEntry& other ) const
	{
		if( ppRefString == other.ppRefString )
		{
			return FxTrue;
		}
		return FxFalse;
	}
	FxRefString** ppRefString;
};

// The name table type.
typedef FxList<FxRefString*> FxNameTable;
// A bin in the name hash.
// If the number of names in the hash becomes so high that lots of collisions 
// occur resulting in hash bins with many elements, and searching through each
// bin becomes a bottleneck (unlikely), the list could be replaced with a
// self-balancing binary tree structure.  The benefit there is that searching
// through a large hash bin would be much faster and the most commonly used
// names would migrate to the root over time.
typedef FxList<FxNameHashEntry> FxNameHashBin;

// The global name table.
static FxNameTable* NameTable;
// The global name hash.  The name hash is simply an auxiliary structure that 
// sits conceptually in front of the name table itself providing name lookup
// in O(1) time.  It is very fast to determine if a name is already in the 
// name table or not.  The only time any memory is allocated is when the name
// is not already in the table and therefore needs to be added.
static FxNameHashBin NameHash[FX_NAME_HASH_SIZE];
// Scratch space to avoid memory allocation for proofing names.
static FxChar ProofNameScratch[FX_MAX_NAME_LENGTH];
// Length of the string currently stored in ProofNameScratch.
static FxSize ProofNameScratchLength = 0;

// Name table system statistics that are only available when FX_TRACK_NAME_STATS
// is defined.
#ifdef FX_TRACK_NAME_STATS
	// The total number of collisions that have occurred in the name hash.
	static FxSize NumCollisions = 0;
	// The total number of names added to the name hash and table.
	static FxSize NumNamesAdded = 0;
	// The total number of names that did not need to be added to the name hash
	// and table because they were already there.
	static FxSize NumProbes = 0;
#endif // FX_TRACK_NAME_STATS

// Returns the "safe" version of a name, stripping '|' characters.  After this
// function the "safe" version of name will be stored in ProofNameScratch and
// the length of the "safe" version of name will be stored in 
// ProofNameScratchLength.  Note that if name contains more characters than
// FX_MAX_NAME_LENGTH it is simply truncated in ProofNameScratch and no assert
// is triggered.
FX_INLINE void ProofName( const FxChar* name )
{
	for( FxSize i = 0; i < FX_MAX_NAME_LENGTH; ++i )
	{
		if( name[i] == '\0' )
		{
			ProofNameScratchLength = i;
			break;
		}
		else if( name[i] == '|' )
		{
			ProofNameScratch[i] = '_';
		}
		else if( name[i] == '"' )
		{
			ProofNameScratch[i] = '\'';
		}
#ifdef FX_DISALLOW_SPACES_IN_NAMES
		else if( name[i] == ' ' )
		{
			ProofNameScratch[i] = '_';
		}
#endif
		else
		{
			ProofNameScratch[i] = name[i];
		}
	}
	ProofNameScratch[ProofNameScratchLength] = '\0';
}

// Returns a valid pointer to the pointer to FxRefString in the actual name 
// table if the given name is in the given hash bin or NULL otherwise.
FX_INLINE FxRefString** FindNameInHash( FxUInt32 hashBin, const FxChar* name )
{
	const FxNameHashBin& theBin = NameHash[hashBin];
	FxNameHashBin::Iterator curr = theBin.Begin();
	FxNameHashBin::Iterator end  = theBin.End();
	for( ; curr != end; ++curr )
	{
		const FxNameHashEntry& entry = (*curr);
		FxRefString** ppRefString = entry.ppRefString;
		// This is a very fast string comparison as the only time a true string
		// compare takes place is if the string in ppRefString and name happen 
		// to have the same length.  No memory is allocated for this operation.
		if( ppRefString && (*ppRefString) && (*ppRefString)->GetString() == name )
		{
			return ppRefString;
		}
	}
	return NULL;
}

#define kCurrentFxNameVersion 1

// FxName static initialization.
FxName FxName::NullName;

FxName::FxName()
	: _ppRefString(NULL)
{
	// Every named object should have a valid name, even if it is the "NULL" 
	// name.
	_ppRefString = NullName._ppRefString;
	if( _ppRefString )
	{
		(*_ppRefString)->AddReference();
	}
}

FxName::FxName( const FxChar* name )
	: _ppRefString(NULL)
{
	FxAssert(NameTable != NULL);
	_addNameToTable(name);
}

FxName::FxName( const FxName& other )
	: _ppRefString(other._ppRefString)
{
	FxAssert(NameTable != NULL);
	if( _ppRefString )
	{
		(*_ppRefString)->AddReference();
	}
}

FxName& FxName::operator=( const FxChar* other )
{
	FxAssert(NameTable != NULL);
	_safeRemoveRef();
	_addNameToTable(other);
	return *this;
}

FxName& FxName::operator=( const FxName& other )
{
	if( this == &other ) return *this;
	_safeRemoveRef();
	_ppRefString = other._ppRefString;
	if( _ppRefString )
	{
		(*_ppRefString)->AddReference();
	}
	return *this;
}

FxName::~FxName()
{
	// Only call _safeRemoveRef() if NameTable is valid.  The only way NameTable
	// can be invalid is if FxName::Shutdown() has already been called and it
	// has been destroyed.  In that case, all names are cleaned up already
	// anyway.
	if( NameTable )
	{
		_safeRemoveRef();
	}
}

void FX_CALL FxName::Startup( void )
{
	FxAssert(NameTable == NULL);
	if( !NameTable )
	{
		NameTable = static_cast<FxNameTable*>(FxAlloc(sizeof(FxNameTable), "Name Table"));
		FxDefaultConstruct(NameTable);
		ProofNameScratch[0] = '\0';
		NullName = "";
		FxAnimGroup::Default = "Default";
		FxActor::NewActor = "NewActor";
	}
}

void FX_CALL FxName::Shutdown( void )
{
	// Clear out the name hash.
	for( FxSize i = 0; i < FX_NAME_HASH_SIZE; ++i )
	{
		NameHash[i].Clear();
	}
	// Clear out the name table.
	FxAssert(NameTable != NULL);
	FxNameTable::Iterator curr = NameTable->Begin();
	FxNameTable::Iterator end  = NameTable->End();
	for( ; curr != end; ++curr )
	{
		if( *curr )
		{
			FxRefString* badPointer = *curr;
			FxDelete(*curr);
			FxNameTable::Iterator pos;
			while( (pos = NameTable->Find(badPointer)) != end )
			{
				(*pos) = NULL;
			}
		}
	}
	FxDelete(NameTable);
	NullName._ppRefString = NULL;
	FxAnimGroup::Default._ppRefString = NULL;
	FxActor::NewActor._ppRefString = NULL;
}

FxBool FxName::operator==( const FxChar* other ) const
{
	if( _ppRefString )
	{
		FxAssert(_ppRefString != NULL);
		return (*_ppRefString)->GetString() == other;
	}
	else
	{
		_ppRefString = NullName._ppRefString;
		FxAssert(_ppRefString != NULL);
		return (*_ppRefString)->GetString() == other;
	}
}

FxBool FxName::operator!=( const FxChar* other ) const
{
	if( _ppRefString )
	{
		FxAssert(_ppRefString != NULL);
		return (*_ppRefString)->GetString() != other;
	}
	else
	{
		_ppRefString = NullName._ppRefString;
		FxAssert(_ppRefString != NULL);
		return (*_ppRefString)->GetString() != other;
	}
}

void FxName::SetName( const FxChar* name )
{
	// If the current name is valid remove a reference from it.
	_safeRemoveRef();
	// Add the new name to the name table and name hash.
	_addNameToTable(name);
}

const FxString& FxName::GetAsString( void ) const
{
	if( !_ppRefString )
	{
		_ppRefString = NullName._ppRefString;
	}
	FxAssert(_ppRefString != NULL);
	return (*_ppRefString)->GetString();
}

const FxChar* FxName::GetAsCstr( void ) const
{
	if( !_ppRefString )
	{
		_ppRefString = NullName._ppRefString;
	}
	FxAssert(_ppRefString != NULL);
	return *((*_ppRefString)->GetString());
}

// Implementation note: FxName::Rename() does not need to be particularly fast 
// as it should never be called in-game.  It is strictly a FaceFX Studio-only 
// operation.
void FxName::Rename( const FxChar* name )
{
	FxAssert(_ppRefString != NULL);

	// Make sure that we aren't doing something stupid like renaming a
	// class name, link function name, link function parameter, or the null 
	// name.
	if( (*_ppRefString)->GetString() == NullName.GetAsString() )
	{
		return;
	}
	else
	{
		// Check for class name.
		FxSize numClasses = FxClass::GetNumClasses();
		for( FxSize i = 0; i < numClasses; ++i )
		{
			const FxClass* pClass = FxClass::GetClass(i);
			if( pClass )
			{
				if( (*_ppRefString)->GetString() == pClass->GetName().GetAsString() )
				{
					return;
				}
			}
		}
		// Check for link function.
		FxSize numLinkFunctions = FxLinkFn::GetNumLinkFunctions();
		for( FxSize i = 0; i < numLinkFunctions; ++i )
		{
			const FxLinkFn* pLinkFn = FxLinkFn::GetLinkFunction(i);
			if( pLinkFn )
			{
				if( (*_ppRefString)->GetString() == pLinkFn->GetNameAsString() )
				{
					return;
				}
				// Check for parameter name.
				FxSize numParams = pLinkFn->GetNumParams();
				for( FxSize j = 0; j < numParams; ++j )
				{
					if( (*_ppRefString)->GetString() == pLinkFn->GetParamName(j) )
					{
						return;
					}
				}
			}
		}
	}

	// If execution gets here we can actually rename the name.
	ProofName(name);
	const FxString& theString = (*_ppRefString)->GetString();
	if( theString != ProofNameScratch )
	{
		// Find the hash bin of the current name.
		FxUInt32 oldHashBin = FxHashString(theString.GetData(), theString.Length());
		// Find the hash bin of the new name.
		FxUInt32 newHashBin = FxHashString(ProofNameScratch, ProofNameScratchLength);
		// Remove the current name from the name hash.
		NameHash[oldHashBin].RemoveIfEqual(FxNameHashEntry(_ppRefString));
		// Attempt to find the new name in the name table.
		FxNameTable::Iterator curr = NameTable->Begin();
		FxNameTable::Iterator end  = NameTable->End();
		for( ; curr != end; ++curr )
		{
			if( (*curr)->GetString() == ProofNameScratch )
			{
				// The new name is in the name table so cache the number of 
				// references to the old name.
				FxSize numRefs = (*_ppRefString)->GetReferenceCount();
				// And remove that many references from it.  This will cause
				// the reference count to drop to zero at which point the
				// string will be cleaned up.
				for( FxSize i = 0; i < numRefs; ++i )
				{
					(*_ppRefString)->RemoveReference();
				}
				// Set the pointer in the linked list to point to the correct 
				// refstring that was already in the name table.
				(*_ppRefString) = *curr;
				// Add the correct number of references to the refstring.
				for( FxSize i = 0; i < numRefs; ++i )
				{
					(*_ppRefString)->AddReference();
				}
				// The new name should already be in the hash so assert if it
				// isn't.
				FxAssert(FindNameInHash(newHashBin, ProofNameScratch) != NULL);
				return;
			}
		}
		// The new name was not in the name table.  Add the new name to the 
		// name hash.
        NameHash[newHashBin].PushBack(FxNameHashEntry(_ppRefString));
		// Set the refstring directly to the new name.
		(*_ppRefString)->SetString(ProofNameScratch);
	}
}

FxSize FX_CALL FxName::GetNumNames( void )
{
	FxAssert(NameTable != NULL);
	return NameTable->Length();
}

FxSize FX_CALL FxName::Find( const FxChar* nameToFind )
{
	FxAssert(NameTable != NULL);
	FxSize numNames = NameTable->Length();
	for( FxSize i = 0; i < numNames; ++i )
	{
		const FxString* pString = GetNameAsString(i);
		if( pString )
		{
			if( *pString == nameToFind )
			{
				return i;
			}
		}
	}
	return FxInvalidIndex;
}

FxSize FX_CALL FxName::GetNameTableSize( void )
{
	FxAssert(NameTable != NULL);
	FxSize result = 0;
	FxSize numNames = NameTable->Length();
	for( FxSize i = 0; i < numNames; ++i )
	{
		const FxString* pString = GetNameAsString(i);
		if( pString )
		{
			result += pString->Length() * sizeof(FxChar);
		}
	}
	return result;
}

const FxString* FX_CALL FxName::GetNameAsString( FxSize index )
{
	FxAssert(NameTable != NULL);
	FxNameTable::Iterator pos = NameTable->Begin();
	pos.Advance(index);
	return &(*pos)->GetString();
}

FxSize FX_CALL FxName::GetNumReferences( FxSize index )
{
	FxAssert(NameTable != NULL);
	FxNameTable::Iterator pos = NameTable->Begin();
	pos.Advance(index);
	return (*pos)->GetReferenceCount();
}

void FX_CALL FxName::GetNameStats( FxSize& numCollisions, FxSize& numNamesAdded,
						           FxSize& numProbes, FxSize& numHashBins, 
						           FxSize& numUsedHashBins, FxSize& memUsed )
{
#ifdef FX_TRACK_NAME_STATS
	numCollisions   = NumCollisions;
	numNamesAdded   = NumNamesAdded;
	numProbes       = NumProbes;
	numHashBins     = FX_NAME_HASH_SIZE;
	numUsedHashBins = 0;
	memUsed         = 0;
	for( FxSize i = 0; i < FX_NAME_HASH_SIZE; ++i )
	{
		memUsed += sizeof(FxList<FxNameHashBin>) + (sizeof(FxNameHashEntry) * NameHash[i].Length());
		if( NameHash[i].Length() > 0 )
		{
			numUsedHashBins++;
		}
	}
#else
	numCollisions   = 0;
	numNamesAdded   = 0;
	numProbes       = 0;
	numHashBins     = 0;
	numUsedHashBins = 0;
	memUsed         = 0;
#endif
}

void FxName::_addNameToTable( const FxChar* name )
{
	FxAssert(NameTable != NULL);
	// Proof the string, make sure it has no '|' characters.
	ProofName(name);
	// Proofed name and its length are now stored in ProofNameScratch and
	// ProofNameScratchLength.

	// Hash ProofNameScratch.
	FxUInt32 hashBin = FxHashString(ProofNameScratch, ProofNameScratchLength);
	// Find ProofNameScratch in the name hash.
	FxRefString** ppRefString = FindNameInHash(hashBin, ProofNameScratch);
	if( ppRefString )
	{
		// ProofNameScratch is already in the table and name hash.
#ifdef FX_TRACK_NAME_STATS
		NumProbes++;
#endif // FX_TRACK_NAME_STATS
		_ppRefString = ppRefString;
	}
	else
	{
		// ProofNameScratch is not already in the table and should be added 
		// to both the name hash and the actual name table.
#ifdef FX_TRACK_NAME_STATS
		NumNamesAdded++;
		if( NameHash[hashBin].Length() > 0 )
		{
			// If there are entries in this hash bin already then we have
			// a collision in the name hash.
			NumCollisions++;
		}
#endif // FX_TRACK_NAME_STATS
		_ppRefString = &(*NameTable->Insert(new FxRefString(ProofNameScratch), NameTable->End()));
		NameHash[hashBin].PushBack(FxNameHashEntry(_ppRefString));
	}
	FxAssert(_ppRefString != NULL);
	// Add a reference to the string.
	(*_ppRefString)->AddReference();
}

FX_INLINE void FxName::_safeRemoveRef( void )
{
	FxAssert(NameTable != NULL);
	if( _ppRefString )
	{
		// Cache a pointer to the underlying refstring.
		FxRefString* pRefString = *_ppRefString;
		// If we're removing the last reference to the name remove it from the
		// name hash and the name table.
		if( pRefString && pRefString->GetReferenceCount() == 1 )
		{
			const FxString& theString = pRefString->GetString();
			FxUInt32 hashBin = FxHashString(theString.GetData(), theString.Length());
			NameHash[hashBin].RemoveIfEqual(FxNameHashEntry(_ppRefString));
			NameTable->RemoveIfEqual(pRefString);
			_ppRefString = NULL;
		}
		pRefString->RemoveReference();
	}
}

FxArchive& operator<<( FxArchive& arc, FxName& name )
{
	FxUInt16 version = kCurrentFxNameVersion;
	arc << version;

	FxAssert(NameTable != NULL);
	if( arc.IsSaving() )
	{
		FxString tempString;
		if( name._ppRefString )
		{
			tempString = (*name._ppRefString)->GetString();
		}
		FxSize nameIndex = arc.AddName(&tempString);
		arc << nameIndex;
	}
	else
	{
		// If we are loading and the reference counted string is valid,
		// we should decrement the reference count of the string before
		// we load into it.
		if( name._ppRefString )
		{
			name._safeRemoveRef();
		}
		if( version < 1 )
		{
			// Prior to version 1 names were stored in archives as strings
			// everywhere.
			FxString tempString;
			arc << tempString;
			name._addNameToTable(tempString.GetData());
		}
		else
		{
			// Starting with version 1 and up strings are stored in archives
			// as indices into the archive's internal name table structure
			// in the archive directory.
			FxSize nameIndex = FxInvalidIndex;
			arc << nameIndex;
			name._ppRefString = reinterpret_cast<FxRefString**>(arc.GetName(nameIndex));
			if( name._ppRefString )
			{
				(*name._ppRefString)->AddReference();
			}
		}
	}
	return arc;
}

} // namespace Face

} // namespace OC3Ent
