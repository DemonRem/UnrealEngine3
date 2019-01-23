//------------------------------------------------------------------------------
// A map from phonemes to tracks.
//
// Owner: John Briggs
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "FxPhonemeMap.h"
#include "FxMath.h"

namespace OC3Ent
{

namespace Face
{

#define kCurrentFxPhonemeMapVersion 0
FX_IMPLEMENT_CLASS(FxPhonemeMap, kCurrentFxPhonemeMapVersion, FxObject)

FxPhonemeMap::FxPhonemeMap()
{
}

FxPhonemeMap::~FxPhonemeMap()
{
}

// Returns the phoneme mapping information for a phoneme/nameIndex pair.
FxReal FxPhonemeMap::GetMappingAmount( const FxPhoneme& phon, 
									   const FxName& name ) const
{
	FxSize numMappingEntries = _mapping.Length();
	for( FxSize i = 0; i < numMappingEntries; ++i )
	{
		if( _mapping[i].name == name && _mapping[i].phoneme == phon )
		{
			return _mapping[i].amount;
		}
	}
	return 0.0f;
}

FxReal FxPhonemeMap::GetMappingAmount( const FxPhoneme& phon, FxSize targetIndex ) const
{
	return GetMappingAmount(phon, GetTargetName(targetIndex));
}

// Sets the phoneme mapping information for a phoneme/nameIndex pair.
void FxPhonemeMap::SetMappingAmount( const FxPhoneme& phon, 
									 const FxName& name, FxReal amount )
{
	FxSize numMappingEntries = _mapping.Length();
	for( FxSize i = 0; i < numMappingEntries; ++i )
	{
		if( _mapping[i].name == name && _mapping[i].phoneme == phon )
		{
			if( !FxRealEquality(amount, 0.0f) )
			{
				// If we're not mapping to zero, update the entry.
				_mapping[i].amount = amount;
			}
			else
			{
				// If we're mapping an existing entry back to zero, remove it 
				// from the mapping to save memory.
				_mapping.Remove(i);
			}
			return;
		}
	}

	// If we're mapping a phoneme to a new target name, add it to the targets.
	FxBool newName = FxTrue;
	FxSize numTargetNames = _targetNames.Length();
	for( FxSize i = 0; i < numTargetNames; ++i )
	{
		newName &= _targetNames[i] != name;
	}
	if( newName )
	{
		_targetNames.PushBack(name);
	}

	// We don't need to add a mapping entry when the amount is 0 since any 
	// missing entry is assumed to be 0.
	if( !FxRealEquality(amount, 0.0f) )
	{
		FxPhonToNameMap newMappingInfo;
		newMappingInfo.phoneme = phon;
		newMappingInfo.name    = name;
		newMappingInfo.amount  = amount;
		_mapping.PushBack(newMappingInfo);
	}
}

void FxPhonemeMap::SetMappingAmount( const FxPhoneme& phon, const FxChar* name, FxReal amount )
{
	// Note the explicit FxName construction here so that the compiler does
	// not get confused now that FxNames are constructed from const FxChar*.
	SetMappingAmount(phon, FxName(name), amount);
}

void FxPhonemeMap::SetMappingAmount( const FxPhoneme& phon, FxSize targetIndex, FxReal amount )
{
	if( targetIndex < _targetNames.Length() )
	{
		SetMappingAmount(phon, GetTargetName(targetIndex), amount);
	}
}

// Removes all mapping entries associated with the given phoneme.
void FxPhonemeMap::RemovePhonemeMappingEntries( const FxPhoneme& phon )
{
	for( FxSize i = 0; i < _mapping.Length(); ++i )
	{
		if( _mapping[i].phoneme == phon )
		{
			_mapping.Remove(i--);
		}
	}
}

// Gets the number of non-zero mapping entries
FxSize FxPhonemeMap::GetNumMappingEntries( void ) const
{
	return _mapping.Length();
}

// Gets a requested mapping entry
const FxPhonToNameMap& FxPhonemeMap::GetMappingEntry( FxSize index ) const
{
	return _mapping[index];
}

// Adds a target to the mapping.
void FxPhonemeMap::AddTarget( const FxName& targetName )
{
	_targetNames.PushBack(targetName);
}

// Removes a target from the mapping.
void FxPhonemeMap::RemoveTarget( const FxName& targetName )
{
	for( FxSize i = 0; i < _mapping.Length(); ++i )
	{
		if( _mapping[i].name == targetName )
		{
			_mapping.Remove(i--);
		}
	}
	_targetNames.Remove(GetTargetIndex(targetName));
}

// Returns the number of targets.
FxSize FxPhonemeMap::GetNumTargets( void ) const
{
	return _targetNames.Length();
}

// Returns the nameIndex of the nth target.
const FxName& FxPhonemeMap::GetTargetName( FxSize n ) const
{
	return _targetNames[n];
}

// Returns the index of the target.
FxSize FxPhonemeMap::GetTargetIndex( const FxName& nameIndex ) const
{
	FxSize numTargetNames = _targetNames.Length();
	for( FxSize i = 0; i < numTargetNames; ++i )
	{
		if( _targetNames[i] == nameIndex ) return i;
	}
	return FxInvalidIndex;
}

// Clears the phoneme mapping.
void FxPhonemeMap::Clear( void )
{
	_targetNames.Clear();
	_mapping.Clear();
}

// Generates the "default" mapping.
void FxPhonemeMap::MakeDefaultMapping( void )
{
	Clear();
	SetMappingAmount(PHONEME_IY, "Eat",		0.85f );
	SetMappingAmount(PHONEME_IH, "If",		0.85f );
	SetMappingAmount(PHONEME_EH, "If",		0.85f );
	SetMappingAmount(PHONEME_EY, "If",		0.85f );
	SetMappingAmount(PHONEME_AE, "If",		0.85f );
	SetMappingAmount(PHONEME_AA, "Ox",		0.85f );
	SetMappingAmount(PHONEME_AW, "If",		0.85f );
	SetMappingAmount(PHONEME_AY, "If",		0.85f );
	SetMappingAmount(PHONEME_AH, "If",		0.85f );
	SetMappingAmount(PHONEME_AO, "Ox",		0.85f );
	SetMappingAmount(PHONEME_OY, "Oat",		0.85f );
	SetMappingAmount(PHONEME_OW, "Oat",		0.85f );
	SetMappingAmount(PHONEME_UH, "Oat",		0.85f );
	SetMappingAmount(PHONEME_UW, "Oat",		0.85f );
	SetMappingAmount(PHONEME_ER, "Earth",	0.85f );
	SetMappingAmount(PHONEME_AX, "If",		0.85f );
	SetMappingAmount(PHONEME_S,  "Size",	0.85f );
	SetMappingAmount(PHONEME_SH, "Church",	0.85f );
	SetMappingAmount(PHONEME_Z,  "Size",	0.85f );
	SetMappingAmount(PHONEME_ZH, "Church",	0.85f );
	SetMappingAmount(PHONEME_F,  "Fave",	0.70f );
	SetMappingAmount(PHONEME_TH, "Though",	0.85f );
	SetMappingAmount(PHONEME_V,  "Fave",	0.70f );
	SetMappingAmount(PHONEME_DH, "Though",  0.85f );
	SetMappingAmount(PHONEME_M,  "Bump",	0.85f );
	SetMappingAmount(PHONEME_N,  "New",		0.85f );
	SetMappingAmount(PHONEME_NG, "New",		0.85f );
	SetMappingAmount(PHONEME_L,  "Told",	0.85f );
	SetMappingAmount(PHONEME_R,  "Roar",	0.85f );
	SetMappingAmount(PHONEME_W,  "Wet",		0.85f );
	SetMappingAmount(PHONEME_Y,  "Wet",		0.85f );
	SetMappingAmount(PHONEME_HH, "If",		0.85f );
	SetMappingAmount(PHONEME_B,  "Bump",	0.85f );
	SetMappingAmount(PHONEME_D,  "Told",	0.85f );
	SetMappingAmount(PHONEME_JH, "Church",	0.85f );
	SetMappingAmount(PHONEME_G,  "Cage",	0.20f );
	SetMappingAmount(PHONEME_P,	 "Bump",	0.85f );
	SetMappingAmount(PHONEME_T,  "Told",	0.85f );
	SetMappingAmount(PHONEME_K,  "Cage",	0.20f );
	SetMappingAmount(PHONEME_CH, "Church",	0.85f );
	SetMappingAmount(PHONEME_SHORTSIL, "If", 0.20f );
	SetMappingAmount(PHONEME_FLAP, "Told", 0.50f );
}

FxArchive& operator<<( FxArchive& arc, FxPhonToNameMap& obj )
{
	FxInt32 tempPhon = static_cast<FxInt32>(obj.phoneme);
	arc << tempPhon << obj.name << obj.amount;
	obj.phoneme = static_cast<FxPhoneme>(tempPhon);
	return arc;
}

void FxPhonemeMap::Serialize( FxArchive& arc )
{
	Super::Serialize( arc );

	FxUInt16 version = FX_GET_CLASS_VERSION(FxPhonemeMap);
	arc << version;

	arc << _mapping << _targetNames;
}

} // namespace Face

} // namespace OC3Ent
