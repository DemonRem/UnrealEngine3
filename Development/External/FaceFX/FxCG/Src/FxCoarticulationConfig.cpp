//------------------------------------------------------------------------------
// The parameters for the coarticulation.
// 
// Owner: John Briggs
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "FxCoarticulationConfig.h"
#include "FxPhonemeEnum.h"
#include "FxSimplePhonemeList.h"
#include "FxVersionInfo.h"
#include "FxArchiveStoreFileFast.h"
#include "FxArchiveStoreMemory.h"
#include "FxArchiveStoreNull.h"
#include "FxArchiveStoreMemoryNoCopy.h"

namespace OC3Ent
{

namespace Face
{

#define kCurrentFxCoarticulationConfigVersion 1

FX_IMPLEMENT_CLASS(FxCoarticulationConfig,kCurrentFxCoarticulationConfigVersion,FxNamedObject);

FxCoarticulationConfig::FxCoarticulationConfig()
	: FxNamedObject("Default")
	, leadInTime(0.25f)
	, leadOutTime(0.30f)
	, shortSilenceDuration(0.27f)
	, suppressionWindowMax(0.02f)
	, suppressionWindowMaxSlope(0.13f)
	, suppressionWindowMin(0.0f)
	, suppressionWindowMinSlope(0.03f)
	, splitDiphthongs(FxTrue)
	, diphthongBoundary(0.05f)
	, convertToFlaps(FxTrue)
	, description("A generic coarticulation configuration, suitable for a wide range of characters.")

{
	// Add the default per-phone information
	phoneInfo.Reserve(NUM_PHONEMES);
	for( FxSize i = 0; i < NUM_PHONEMES; ++i )
	{
		phoneInfo.PushBack(FxPerPhoneCoarticulationInfo());
	}

	phoneInfo[PHONEME_IY] = FxPerPhoneCoarticulationInfo(0.50f);
	phoneInfo[PHONEME_IH] = FxPerPhoneCoarticulationInfo(0.40f);
	phoneInfo[PHONEME_EH] = FxPerPhoneCoarticulationInfo(0.50f);
	phoneInfo[PHONEME_EY] = FxPerPhoneCoarticulationInfo(0.50f);
	phoneInfo[PHONEME_AE] = FxPerPhoneCoarticulationInfo(0.50f);
	phoneInfo[PHONEME_AA] = FxPerPhoneCoarticulationInfo(0.50f);
	phoneInfo[PHONEME_AW] = FxPerPhoneCoarticulationInfo(0.50f);
	phoneInfo[PHONEME_AY] = FxPerPhoneCoarticulationInfo(0.50f);
	phoneInfo[PHONEME_AH] = FxPerPhoneCoarticulationInfo(0.50f);
	phoneInfo[PHONEME_AO] = FxPerPhoneCoarticulationInfo(0.50f);
	phoneInfo[PHONEME_OY] = FxPerPhoneCoarticulationInfo(0.50f);
	phoneInfo[PHONEME_OW] = FxPerPhoneCoarticulationInfo(0.50f);
	phoneInfo[PHONEME_UH] = FxPerPhoneCoarticulationInfo(0.50f);
	phoneInfo[PHONEME_UW] = FxPerPhoneCoarticulationInfo(0.60f);
	phoneInfo[PHONEME_ER] = FxPerPhoneCoarticulationInfo(0.50f);
	phoneInfo[PHONEME_AX] = FxPerPhoneCoarticulationInfo(0.50f);
	phoneInfo[PHONEME_S]  = FxPerPhoneCoarticulationInfo(0.20f);
	phoneInfo[PHONEME_SH] = FxPerPhoneCoarticulationInfo(0.75f);
	phoneInfo[PHONEME_Z]  = FxPerPhoneCoarticulationInfo(0.60f);
	phoneInfo[PHONEME_ZH] = FxPerPhoneCoarticulationInfo(0.75f);
	phoneInfo[PHONEME_F]  = FxPerPhoneCoarticulationInfo(0.95f, 0.06f);
	phoneInfo[PHONEME_TH] = FxPerPhoneCoarticulationInfo(0.95f);
	phoneInfo[PHONEME_V]  = FxPerPhoneCoarticulationInfo(0.95f, 0.05f);
	phoneInfo[PHONEME_DH] = FxPerPhoneCoarticulationInfo(0.95f);
	phoneInfo[PHONEME_M]  = FxPerPhoneCoarticulationInfo(1.00f, 0.06f);
	phoneInfo[PHONEME_N]  = FxPerPhoneCoarticulationInfo(0.35f);
	phoneInfo[PHONEME_NG] = FxPerPhoneCoarticulationInfo(0.45f);
	phoneInfo[PHONEME_L]  = FxPerPhoneCoarticulationInfo(0.40f);
	phoneInfo[PHONEME_R]  = FxPerPhoneCoarticulationInfo(0.65f);
	phoneInfo[PHONEME_W]  = FxPerPhoneCoarticulationInfo(0.85f);
	phoneInfo[PHONEME_Y]  = FxPerPhoneCoarticulationInfo(0.65f);
	phoneInfo[PHONEME_HH] = FxPerPhoneCoarticulationInfo(0.10f);
	phoneInfo[PHONEME_B]  = FxPerPhoneCoarticulationInfo(1.00f, 0.06f);
	phoneInfo[PHONEME_D]  = FxPerPhoneCoarticulationInfo(0.45f, 0.06f);
	phoneInfo[PHONEME_JH] = FxPerPhoneCoarticulationInfo(0.75f, 0.05f);
	phoneInfo[PHONEME_G]  = FxPerPhoneCoarticulationInfo(0.10f);
	phoneInfo[PHONEME_P]  = FxPerPhoneCoarticulationInfo(1.00f, 0.06f);
	phoneInfo[PHONEME_T]  = FxPerPhoneCoarticulationInfo(0.50f, 0.06f);
	phoneInfo[PHONEME_K]  = FxPerPhoneCoarticulationInfo(0.10f);
	phoneInfo[PHONEME_CH] = FxPerPhoneCoarticulationInfo(0.75f);
	phoneInfo[PHONEME_SIL] = FxPerPhoneCoarticulationInfo(0.05f);
	phoneInfo[PHONEME_SHORTSIL] = FxPerPhoneCoarticulationInfo(0.10f);
	phoneInfo[PHONEME_FLAP] = FxPerPhoneCoarticulationInfo(0.50f);
}

// Serialize an FxPerPhoneCoarticulationInfo
FxArchive& operator<<( FxArchive& arc, FxPerPhoneCoarticulationInfo& obj )
{
	arc << obj.dominance << obj.anticipation;
	return arc;
}

// Serialize the configuration
void FxCoarticulationConfig::Serialize( FxArchive& arc )
{
	Super::Serialize(arc);

	FxUInt16 version = FX_GET_CLASS_VERSION(FxCoarticulationConfig);
	arc << version;

	arc << leadInTime
		<< leadOutTime
		<< shortSilenceDuration
		<< suppressionWindowMax
	    << suppressionWindowMaxSlope
	    << suppressionWindowMin
	    << suppressionWindowMinSlope
		<< splitDiphthongs
		<< diphthongBoundary
		<< convertToFlaps
		<< phoneInfo
		<< description;

	if( arc.IsLoading() && version < 1 )
	{
		// As of version 1 of FxCoarticulationConfig these values have new 
		// defaults and loading old configurations needs to be patched up to 
		// reflect them.
		suppressionWindowMax      = 0.02f;
		suppressionWindowMaxSlope = 0.13f;
		suppressionWindowMin      = 0.0f;
		suppressionWindowMinSlope = 0.03f;
	}
}

FxBool FX_CALL FxLoadCoarticulationConfigFromFile( FxCoarticulationConfig& config, const FxChar* filename, const FxBool bUseFastMethod, void(FX_CALL *callbackFunction)(FxReal) /*= NULL*/, FxReal updateFrequency /*= 0.01f */ )
{
	FxArchiveStore* pStore = bUseFastMethod ? FxArchiveStoreFileFast::Create(filename)
		: FxArchiveStoreFile::Create(filename);
	FxArchive configArchive(pStore, FxArchive::AM_LinearLoad);
	configArchive.Open();
	if( configArchive.IsValid() )
	{
		configArchive.RegisterProgressCallback(callbackFunction, updateFrequency);
		configArchive << config;
		return FxTrue;
	}
	return FxFalse;
}

FxBool FX_CALL FxSaveCoarticulationConfigToFile( FxCoarticulationConfig& config, const FxChar* filename, FxArchive::FxArchiveByteOrder byteOrder /*= FxArchive::ABO_LittleEndian*/, void(FX_CALL *callbackFunction)(FxReal) /*= NULL*/, FxReal updateFrequency /*= 0.01f */ )
{
#ifndef NO_SAVE_VERSION
	FxArchive directoryCreater(FxArchiveStoreNull::Create(), FxArchive::AM_CreateDirectory);
	directoryCreater.Open();
	directoryCreater << config;
	FxArchive configArchive(FxArchiveStoreFile::Create(filename), FxArchive::AM_Save, byteOrder);
	configArchive.Open();
	if( configArchive.IsValid() )
	{
		configArchive.SetInternalDataState(directoryCreater.GetInternalData());
		configArchive.RegisterProgressCallback(callbackFunction, updateFrequency);
		configArchive << config;
		return FxTrue;
	}
#else
	config; filename; byteOrder; callbackFunction; updateFrequency;
#endif
	return FxFalse;
}

FxBool FX_CALL FxLoadCoarticulationConfigFromMemory( FxCoarticulationConfig& config, const FxByte* pMemory, const FxSize numBytes, void(FX_CALL *callbackFunction)(FxReal) /*= NULL*/, FxReal updateFrequency /*= 0.01f */ )
{
	if( pMemory )
	{
		FxArchive configArchive(FxArchiveStoreMemoryNoCopy::Create(pMemory, numBytes), FxArchive::AM_LinearLoad);
		configArchive.Open();
		if( configArchive.IsValid() )
		{
			configArchive.RegisterProgressCallback(callbackFunction, updateFrequency);
			configArchive << config;
			return FxTrue;
		}
	}
	return FxFalse;
}

FxBool FX_CALL FxSaveCoarticulationConfigToMemory( FxCoarticulationConfig& config, FxByte*& pMemory, FxSize& numBytes, FxArchive::FxArchiveByteOrder byteOrder /*= FxArchive::ABO_LittleEndian*/, void(FX_CALL *callbackFunction)(FxReal) /*= NULL*/, FxReal updateFrequency /*= 0.01f */ )
{
#ifndef NO_SAVE_VERSION
	FxAssert(pMemory == NULL);
	if( !pMemory )
	{
		FxArchive directoryCreater(FxArchiveStoreNull::Create(), FxArchive::AM_CreateDirectory);
		directoryCreater.Open();
		directoryCreater << config;
		FxArchiveStoreMemory* pStore = FxArchiveStoreMemory::Create(NULL, 0);
		FxArchive configArchive(pStore, FxArchive::AM_Save, byteOrder);
		configArchive.Open();
		if( configArchive.IsValid() )
		{
			configArchive.SetInternalDataState(directoryCreater.GetInternalData());
			configArchive.RegisterProgressCallback(callbackFunction, updateFrequency);
			configArchive << config;
			numBytes = pStore->GetSize();
			pMemory = static_cast<FxByte*>(FxAlloc(numBytes, "FxSaveCoarticulationConfigToMemory"));
			FxMemcpy(pMemory, pStore->GetMemory(), numBytes);
			return FxTrue;
		}
	}
#else
	config; pMemory; numBytes; byteOrder; callbackFunction; updateFrequency;
#endif
	return FxFalse;
}

} // namespace Face

} // namespace OC3Ent