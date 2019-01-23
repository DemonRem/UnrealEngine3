//------------------------------------------------------------------------------
// The Fonix analysis core.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#ifndef FxAnalysisCoreFonix_H__
#define FxAnalysisCoreFonix_H__

#include "stdwx.h"

#include "FxPlatform.h"
#include "FxAnalysisCore.h"
#include "FxConsoleVariable.h"
#include "FxAnimController.h"
#include "FxCG.h"
#include "FxCoarticulationGeneric.h"
#include "FxSpeechGestures.h"

namespace OC3Ent
{

namespace Face
{

// The Fonix analysis core.
class FxAnalysisCoreFonix : public FxAnalysisCore
{
	// Declare the class.
	FX_DECLARE_CLASS_NO_SERIALIZE(FxAnalysisCoreFonix, FxAnalysisCore)
	// Disable copy construction and assignment.
	FX_NO_COPY_OR_ASSIGNMENT(FxAnalysisCoreFonix)
public:
	// Constructor.
	FxAnalysisCoreFonix();
	// Destructor.
	virtual ~FxAnalysisCoreFonix();

	// Startup the Fonix core.
	virtual FxBool Startup( FxAnimController* pAnimController );
	// Shutdown the Fonix core.
	virtual FxBool Shutdown( void );

	// Called when the audio minimum has changed.
	virtual FxBool ChangeAudioMinimum( FxReal audioMinimum );
	// Called when the audio maximum has changed.
	virtual FxBool ChangeAudioMaximum( FxReal audioMaximum );
	// Called to retrieve the audio minimum.
	virtual FxBool GetAudioMinimum( FxReal& audioMinimum );
	// Called to retrieve the audio maximum.
	virtual FxBool GetAudioMaximum( FxReal& audioMaximum );

	// Called when the current animation has changed.
	virtual FxBool ChangeAnim( FxAnim* pAnim );

	// Create a new animation.
	virtual FxBool CreateAnim( FxAnim* pAnim, FxPhonemeMap* pPhonemeMap, const FxWString& strippedText );
	// Reanalyze a range of an existing animation.
	virtual FxBool ReanalyzeRange( FxAnim* pAnim, FxReal rangeStart, FxReal rangeEnd, const FxWString& strippedText );
	// Change the phoneme mapping.
	virtual FxBool ChangePhonemeMapping( FxAnim* pAnim, FxPhonemeMap* pPhonemeMap );
	// Change the phoneme list.
	virtual FxBool ChangePhonemeList( FxAnim* pAnim, FxPhonemeMap* pPhonemeMap, FxPhonWordList* pPhonemeWordList );

	// Callback for when the debug Fonix console variable changes.
	static void OnDebugFonixUpdated( FxConsoleVariable& cvar );

protected:
	FxInt32 GetLanguageCode( const FxString& languageName );

	void PostProcessPhonemeList( FxPhonWordList* pPhonList, FxReal audioEndTime );

	FxConsoleVariable* a_audiomin;
	FxConsoleVariable* a_audiomax;
	// The animation controller.
	FxAnimController* _pAnimController;
	// The minimum phoneme length;
	FxReal _minimumPhonemeLength;
	// The coarticulation component.
	FxCoarticulationGeneric _coarticulationObj;
	// The speech gestures component.
	FxSpeechGestures _speechGesturesObj;
};

} // namespace Face

} // namespace OC3Ent

#endif
