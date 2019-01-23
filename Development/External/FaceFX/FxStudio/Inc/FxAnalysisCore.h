//------------------------------------------------------------------------------
// The base class for all analysis cores.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#ifndef FxAnalysisCore_H__
#define FxAnalysisCore_H__

#include "FxPlatform.h"
#include "FxObject.h"
#include "FxAnimController.h"
#include "FxAnim.h"
#include "FxPhonemeMap.h"
#include "FxPhonWordList.h"
#include "FxWidget.h" // Needed for FxLanguageInfo.

namespace OC3Ent
{

namespace Face
{

// The base analysis core licensee information.
class FxAnalysisCoreLicenseeInfo
{
public:
	// Constructor.
	FxAnalysisCoreLicenseeInfo();
	// Destructor.
	~FxAnalysisCoreLicenseeInfo();

	// Returns the licensee name.
	const FxString& GetLicenseeName( void ) const;
	// Returns the licensee project name.
	const FxString& GetLicenseeProject( void ) const;
	// Returns the expiration date as a string.
	const FxString& GetExpirationDate( void ) const;

	// Sets the licensee information.  Internal use only.
	void SetLicenseeInfo( const FxChar* licenseeName,
		                  const FxChar* licenseeProject,
		                  const FxChar* expirationDate );

private:
	// The licensee name.
	FxString _licenseeName;
	// The licensee project name.
	FxString _licenseeProject;
	// The expiration date.
	FxString _expirationDate;
};

// The progress callback.
class FxFunctor;
extern FxFunctor* g_pProgressFunctor;
void g_ProgessCallback( FxDReal percentComplete );

// The base analysis core.
class FxAnalysisCore : public FxObject
{
	// Declare the class.
	FX_DECLARE_CLASS_NO_DYNCREATE_NO_SERIALIZE(FxAnalysisCore, FxObject)
	// Disable copy construction and assignment.
	FX_NO_COPY_OR_ASSIGNMENT(FxAnalysisCore)
public:
	// Constructor.
	FxAnalysisCore();
	// Destructor.
	virtual ~FxAnalysisCore();

	// Returns whether or not the core is expired.
	FxBool IsExpired( void ) const;
	// Returns the core licensee information.
	const FxAnalysisCoreLicenseeInfo& GetLicenseeInfo( void ) const;

	// Returns the languages supported by the analysis core.
	const FxArray<FxLanguageInfo>& GetSupportedLanguages( void ) const;

	// Startup the core.
	virtual FxBool Startup( FxAnimController* pAnimController ) = 0;
	// Shutdown the core.
	virtual FxBool Shutdown( void ) = 0;

	// Called when the audio minimum has changed.
	virtual FxBool ChangeAudioMinimum( FxReal audioMinimum );
	// Called when the audio maximum has changed.
	virtual FxBool ChangeAudioMaximum( FxReal audioMaximum );
	// Called to retrieve the audio minimum.
	virtual FxBool GetAudioMinimum( FxReal& audioMinimum );
	// Called to retrieve the audio maximum.
	virtual FxBool GetAudioMaximum( FxReal& audioMaximum );

	enum SpeechDetectionState
	{
		SDS_Automatic = -1,
		SDS_Off = 0,
		SDS_On = 1
	};
	// Called to set the speech detection state.
	virtual void SetSpeechDetection(SpeechDetectionState state);
	// Called to get the speech detection state.
	virtual SpeechDetectionState GetSpeechDetection();

	// Called when the current animation has changed.
	virtual FxBool ChangeAnim( FxAnim* pAnim );
    
	// Create a new animation.
	virtual FxBool CreateAnim( FxAnim* pAnim, FxPhonemeMap* pPhonemeMap, const FxWString& strippedText ) = 0;
	// Reanalyze a range of an existing animation.
	virtual FxBool ReanalyzeRange( FxAnim* pAnim, FxReal rangeStart, FxReal rangeEnd, const FxWString& strippedText );
	// Change the phoneme mapping.
	virtual FxBool ChangePhonemeMapping( FxAnim* pAnim, FxPhonemeMap* pPhonemeMap ) = 0;
	// Change the phoneme list.
	virtual FxBool ChangePhonemeList( FxAnim* pAnim, FxPhonemeMap* pPhonemeMap, FxPhonWordList* pPhonemeWordList ) = 0;

protected:
	// FxTrue if the core is expired.
	FxBool _isExpired;
	// The core licensee information.
	FxAnalysisCoreLicenseeInfo _licenseeInfo;
	// The core language information.
	FxArray<FxLanguageInfo> _languages;
	// The speech detection state.
	SpeechDetectionState _speechDetectionState;
};

} // namespace Face

} // namespace OC3Ent

#endif

