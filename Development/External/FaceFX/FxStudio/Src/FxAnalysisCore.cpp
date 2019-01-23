//------------------------------------------------------------------------------
// The base class for all analysis cores.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "stdwx.h"
#include "FxAnalysisCore.h"
#include "FxMemFnCallback.h"
#include "FxConsole.h"

namespace OC3Ent
{

namespace Face
{

FxFunctor* g_pProgressFunctor = NULL;
void g_ProgessCallback( FxDReal percentComplete )
{
	static FxReal kOneOverOneHundred = 1.0f / 100.0f;
	if( g_pProgressFunctor )
	{
		FxReal pct = static_cast<FxReal>(percentComplete) * kOneOverOneHundred;
		(*g_pProgressFunctor)(pct);
	}
}

FxAnalysisCoreLicenseeInfo::FxAnalysisCoreLicenseeInfo()
{
}

FxAnalysisCoreLicenseeInfo::~FxAnalysisCoreLicenseeInfo()
{
}

const FxString& FxAnalysisCoreLicenseeInfo::GetLicenseeName( void ) const
{
	return _licenseeName;
}

const FxString& FxAnalysisCoreLicenseeInfo::GetLicenseeProject( void ) const
{
	return _licenseeProject;
}

const FxString& FxAnalysisCoreLicenseeInfo::GetExpirationDate( void ) const
{
	return _expirationDate;
}

void FxAnalysisCoreLicenseeInfo::SetLicenseeInfo( const FxChar* licenseeName,
					                              const FxChar* licenseeProject,
					                              const FxChar* expirationDate )
{
	_licenseeName    = licenseeName;
	_licenseeProject = licenseeProject;
	_expirationDate  = expirationDate;
}

#define kCurrentFxAnalysisCoreVersion 0

FX_IMPLEMENT_CLASS(FxAnalysisCore, kCurrentFxAnalysisCoreVersion, FxObject);

FxAnalysisCore::FxAnalysisCore()
	: _isExpired(FxFalse)
	, _speechDetectionState(SDS_Automatic)
{
}

FxAnalysisCore::~FxAnalysisCore()
{
}

FxBool FxAnalysisCore::IsExpired( void ) const
{
	return _isExpired;
}

const FxAnalysisCoreLicenseeInfo& FxAnalysisCore::GetLicenseeInfo( void ) const
{
	return _licenseeInfo;
}

const FxArray<FxLanguageInfo>& FxAnalysisCore::GetSupportedLanguages( void ) const
{
	return _languages;
}

FxBool FxAnalysisCore::ChangeAudioMinimum( FxReal FxUnused(audioMinimum) )
{
	return FxFalse;
}

FxBool FxAnalysisCore::ChangeAudioMaximum( FxReal FxUnused(audioMaximum) )
{
	return FxFalse;
}

FxBool FxAnalysisCore::GetAudioMinimum( FxReal& FxUnused(audioMinimum) )
{
	return FxFalse;
}

FxBool FxAnalysisCore::GetAudioMaximum( FxReal& FxUnused(audioMaximum) )
{
	return FxFalse;
}

FxBool FxAnalysisCore::ChangeAnim( FxAnim* FxUnused(pAnim) )
{
	return FxFalse;
}

void FxAnalysisCore::SetSpeechDetection( SpeechDetectionState state )
{
	_speechDetectionState = state;
}

FxAnalysisCore::SpeechDetectionState FxAnalysisCore::GetSpeechDetection()
{
	return _speechDetectionState;
}

FxBool FxAnalysisCore::
ReanalyzeRange( FxAnim* FxUnused(pAnim), FxReal FxUnused(rangeStart), 
			    FxReal FxUnused(rangeEnd), const FxWString& FxUnused(strippedText) )
{
	FxError("<b>FxAnalysisCore::ReanalyzeRange - the current analysis core does not support reanalyze range!</b>");
	return FxFalse;
}

} // namespace Face

} // namespace OC3Ent