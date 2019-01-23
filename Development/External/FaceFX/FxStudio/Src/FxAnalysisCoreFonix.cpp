//------------------------------------------------------------------------------
// The Fonix analysis core.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "stdwx.h"

#ifndef __APPLE__

#include "FxAnalysisCoreFonix.h"
#include "FxConsole.h"
#include "FxVM.h"
#include "FxStudioApp.h"
#include "FxProgressDialog.h"
#include "FxCurveUserData.h"
#include "FxSessionProxy.h"
#include "FxCGConfigManager.h"

#pragma pack( push, 8 )
	#include "FxAnalysisFonix.h"
#pragma pack( pop )

// Uncomment this to force using the debug analysis library.
// Note to licensees: do not turn this on because only OC3 has the debug library.
//#define FX_USE_DEBUG_FONIX_LIBRARY

#ifndef __UNREAL__
	#if defined(FX_DEBUG) && defined(FX_USE_DEBUG_FONIX_LIBRARY)
		#pragma message("Linking Debug FxAnalysisFonix...")
		#pragma comment(lib, "FxAnalysisFonix-DEBUG.lib")
	#else
		#pragma message("Linking Release FxAnalysisFonix...")
		#pragma comment(lib, "FxAnalysisFonix.lib")
	#endif
#endif

namespace OC3Ent
{

namespace Face
{

// Checks for FaceFX Fonix Analysis Library errors and dumps the specified 
// message upon success.
#define CHECK_FXAL_ERROR_NOTIFY_SUCCESS(errorCode, successMsg) \
	if( FX_ANALYSIS_FONIX_NOERROR != errorCode ) \
	{ \
		FxInt32 errorStringLength = 1023; \
		FxWChar  errorString[1024] = {0}; \
		FxAnalysisFonixGetErrorString(errorCode, errorStringLength, errorString); \
		FxString converter(errorString); \
		FxCritical( "<b>Fonix core error detected: <i>%s</i></b>", \
		converter.GetData() ); \
	} \
	else \
	{ \
		FxMsg( "Fonix core %s", #successMsg ); \
	}

// Checks for FaceFX Fonix Analysis Library errors and does nothing upon success.
#define CHECK_FXAL_ERROR(errorCode) \
	if( FX_ANALYSIS_FONIX_NOERROR != errorCode ) \
	{ \
		FxInt32 errorStringLength = 1023; \
		FxWChar  errorString[1024] = {0}; \
		FxAnalysisFonixGetErrorString(errorCode, errorStringLength, errorString); \
		FxString converter(errorString); \
		FxCritical( "<b>Fonix core error detected: <i>%s</i></b>", \
		converter.GetData() ); \
	}

#define kCurrentFxAnalysisCoreFonixVersion 0

FX_IMPLEMENT_CLASS(FxAnalysisCoreFonix, kCurrentFxAnalysisCoreFonixVersion, FxAnalysisCore);

void FxAnalysisFonixLogOutputCallback( const FxWChar* msg )
{
	FxMsgW(msg);
}

FxAnalysisCoreFonix::FxAnalysisCoreFonix()
	: a_audiomin(FxVM::FindConsoleVariable("a_audiomin"))
	, a_audiomax(FxVM::FindConsoleVariable("a_audiomax"))
{
	FxAssert(a_audiomin);
	FxAssert(a_audiomax);
}

FxAnalysisCoreFonix::~FxAnalysisCoreFonix()
{
}

FxBool FxAnalysisCoreFonix::Startup( FxAnimController* pAnimController )
{
	_pAnimController = pAnimController;
	_minimumPhonemeLength = 0.01f;
	FxMsg( "Starting up Fonix core..." );
	
	wxFileName dataPath(FxStudioApp::GetAppDirectory());
	dataPath.AppendDir(wxT("FonixData"));
	dataPath.AppendDir(wxT("fre"));
	dataPath.AppendDir(wxT("fd01"));
	FxWString dataDirectory(dataPath.GetFullPath().GetData());
	FxMsg( "Starting Fonix core with data directory %s...",
		dataDirectory.GetCstr() );
	FxAnalysisFonixError stat = FxAnalysisFonixStartup(dataDirectory.GetData());
	CHECK_FXAL_ERROR_NOTIFY_SUCCESS(stat, started successfully.);
	if( FX_ANALYSIS_FONIX_LICENSE_EXPIRED == stat )
	{
		_isExpired = FxTrue;
	}

	// Get the minimum phoneme length from Fonix.
	FxAnalysisFonixGetMinimumPhonemeLength(_minimumPhonemeLength);

	FxString versionInfo = "Using Fonix version ";
	FxInt32 fonixVersion = 0;
	FxAnalysisFonixGetVersion(fonixVersion);
	FxChar versionNumber[32] = {0};
	FxItoa(fonixVersion , versionNumber);
	versionInfo = FxString::Concat(versionInfo, versionNumber);
	FxMsg(versionInfo.GetData());

	// Display the licensee information.
	FxAnalysisFonixLicenseeInfo licenseeInfo;
	FxAnalysisFonixGetLicenseeInfo(licenseeInfo);
	wxString licensedTo = wxString::Format(wxT("Licensed to: %s"), wxString::FromAscii(licenseeInfo.licenseeName));
	wxString project = wxString::Format(wxT("Project: %s"),  wxString::FromAscii(licenseeInfo.licenseeProject));
	wxString expiration = wxString::Format(wxT("Expires: %s"),  wxString::FromAscii(licenseeInfo.expirationDate));
	FxMsg(licensedTo.mb_str(wxConvLibc));
	FxMsg(project.mb_str(wxConvLibc));
	FxMsg(expiration.mb_str(wxConvLibc));

	// Cache the licensee information.
	_licenseeInfo.SetLicenseeInfo(licenseeInfo.licenseeName, licenseeInfo.licenseeProject, licenseeInfo.expirationDate);

	// Display and cache the supported languages information.
	_languages.Clear();
	FxInt32 numSupportedLanguages = 0;
	FxAnalysisFonixGetNumSupportedLanguages(numSupportedLanguages);
	FxMsg("%d supported languages:", numSupportedLanguages);

	FxAnalysisFonixLanguageInfo* pSupportedLanguages = new FxAnalysisFonixLanguageInfo[numSupportedLanguages];
	FxAnalysisFonixGetSupportedLanguages(pSupportedLanguages);
	for( FxInt32 i = 0; i < numSupportedLanguages; ++i )
	{
		// Cache the language.
		FxLanguageInfo newLanguage;
		newLanguage.languageCode = pSupportedLanguages[i].languageCode;
		newLanguage.languageName = pSupportedLanguages[i].languageName;
		newLanguage.nativeLanguageName = pSupportedLanguages[i].nativeLanguageName;
		newLanguage.isDefault = (1 == pSupportedLanguages[i].isDefault) ? FxTrue : FxFalse;
		_languages.PushBack(newLanguage);

		// Print the information to the console.
		wxString languageName(_languages.Back().languageName.GetWCstr());
		if( _languages.Back().isDefault )
		{
			FxMsg("%s (Default)", languageName.mb_str(wxConvLibc));
		}
		else
		{
			FxMsg("%s", languageName.mb_str(wxConvLibc));
		}
	}
	delete [] pSupportedLanguages;
	pSupportedLanguages = NULL;

	FxMsg( "Starting up FxCG..." );
	FxCGStartup();
	FxMsg( "Using FxCG version %f", static_cast<FxReal>(FxCGGetVersion()/1000.0f) );

	// We intentionally do not fail here if the license was expired.
	if( FX_ANALYSIS_FONIX_NOERROR == stat || FX_ANALYSIS_FONIX_LICENSE_EXPIRED == stat )
	{
		return FxTrue;
	}
	return FxFalse;
}

FxBool FxAnalysisCoreFonix::Shutdown( void )
{
	FxMsg( "Shutting down Fonix core..." );
	FxAnalysisFonixError stat = FxAnalysisFonixShutdown();
	CHECK_FXAL_ERROR_NOTIFY_SUCCESS(stat, shutdown successfully.);
	FxMsg( "Shutting down FxCG..." );
	FxCGShutdown();
	FxMsg( "FxCG shutdown successfully." );
	return FxTrue;
}

FxBool FxAnalysisCoreFonix::ChangeAudioMinimum( FxReal audioMinimum )
{
	FxAnalysisFonixSetAudioMinimum(audioMinimum);
	return FxTrue;
}

FxBool FxAnalysisCoreFonix::ChangeAudioMaximum( FxReal audioMaximum )
{
	FxAnalysisFonixSetAudioMaximum(audioMaximum);
	return FxTrue;
}

FxBool FxAnalysisCoreFonix::GetAudioMinimum( FxReal& audioMinimum )
{
	FxAnalysisFonixGetAudioMinimum(audioMinimum);
	return FxTrue;
}

FxBool FxAnalysisCoreFonix::GetAudioMaximum( FxReal& audioMaximum )
{
	FxAnalysisFonixGetAudioMaximum(audioMaximum);
	return FxTrue;
}

FxBool FxAnalysisCoreFonix::ChangeAnim( FxAnim* pAnim )
{
	if( pAnim )
	{
		FxAnimUserData* pAnimUserData = reinterpret_cast<FxAnimUserData*>(pAnim->GetUserData());
		if( pAnimUserData )
		{
			// Set the configurations.
			FxCoarticulationConfig* pCoarticulationConfig = 
				FxCGConfigManager::GetGenericCoarticulationConfig(pAnimUserData->GetCoarticulationConfig().GetData());
			FxGestureConfig* pGestureConfig = 
				FxCGConfigManager::GetGestureConfig(pAnimUserData->GetGestureConfig().GetData());
			//@todo Fix this at some point.
			// The previous call to FxCGConfigManager::GetGestureConfig() could have caused all of the
			// configurations to be rescanned in the rare case that the gesture configuration could not
			// be found.  Since those are stored as arrays, the previous pointer is no longer valid, so
			// cache it again.
            pCoarticulationConfig = 
				FxCGConfigManager::GetGenericCoarticulationConfig(pAnimUserData->GetCoarticulationConfig().GetData());

			if( pCoarticulationConfig )
			{
				_coarticulationObj.SetConfig(*pCoarticulationConfig);
			}
			if( pGestureConfig )
			{
				_speechGesturesObj.SetConfig(*pGestureConfig);
			}

			FxDigitalAudio* pDigitalAudio = pAnimUserData->GetDigitalAudioPointer();
			if( pDigitalAudio )
			{
				_speechGesturesObj.Initialize(pDigitalAudio->GetSampleBuffer(), 
					                          pDigitalAudio->GetSampleCount(), 
											  pDigitalAudio->GetSampleRate());
			}
		}
	}
	return FxTrue;
}

FxBool FxAnalysisCoreFonix::CreateAnim( FxAnim* pAnim, FxPhonemeMap* pPhonemeMap, const FxWString& strippedText )
{
	if( pAnim && pPhonemeMap )
	{
		FxAnimUserData* pAnimUserData = reinterpret_cast<FxAnimUserData*>(pAnim->GetUserData());
		if( pAnimUserData )
		{
			// If the digital audio pointer is NULL, nothing happens here as it is
			// assumed that the animation is a "silent" animation.
			FxDigitalAudio* pDigitalAudio = pAnimUserData->GetDigitalAudioPointer();
			if( pDigitalAudio )
			{
				// Create the curves that should be owned by the analysis.
				for( FxSize i = 0; i < pPhonemeMap->GetNumTargets(); ++i )
				{
					FxAnimCurve newAnimCurve;
					newAnimCurve.SetName(pPhonemeMap->GetTargetName(i));
					pAnim->AddAnimCurve(newAnimCurve);
					pAnim->GetAnimCurveM(pAnim->FindAnimCurve(newAnimCurve.GetName())).SetUserData(new FxCurveUserData());
					pAnimUserData->SetCurveOwnedByAnalysis(newAnimCurve.GetName(), FxTrue);
				}
				for( FxSize i = 0; i < FxCGGetNumSpeechGestureTracks(); ++i )
				{
					FxAnimCurve newAnimCurve;
					newAnimCurve.SetName(FxCGGetSpeechGestureTrackName(i));
					pAnim->AddAnimCurve(newAnimCurve);
					pAnim->GetAnimCurveM(pAnim->FindAnimCurve(newAnimCurve.GetName())).SetUserData(new FxCurveUserData());
					pAnimUserData->SetCurveOwnedByAnalysis(newAnimCurve.GetName(), FxTrue);
				}

				SpeechDetectionState localSpeechDetectionState = _speechDetectionState;
				if( localSpeechDetectionState == SDS_Automatic )
				{
					localSpeechDetectionState = SDS_On;
				}

				// Set up for analysis.
				FxByte* pSampleBuffer = pDigitalAudio->ExportSampleBuffer(pDigitalAudio->GetBitsPerSample());
				FxProgressDialog progress(_("Analyzing..."), FxStudioApp::GetMainWindow());
				g_pProgressFunctor = new FxProgressCallback<FxProgressDialog>(&progress, &FxProgressDialog::Update);
				//@todo This will always fail until Fonix supports progress callbacks.
				FxAnalysisFonixRegisterAnalysisCallback(g_ProgessCallback);

				// First analysis attempt.
				FxAnalysisFonixError stat = FxAnalysisFonixAnalyze(pDigitalAudio->GetBitsPerSample(),
																   pDigitalAudio->GetSampleRate(),
																   pDigitalAudio->GetChannelCount(),
																   pDigitalAudio->GetSampleCount(),
																   reinterpret_cast<const FxChar*>(pSampleBuffer),
																   strippedText.Length(),
																   strippedText.GetData(),
																   GetLanguageCode(pAnimUserData->GetLanguage()),
																   localSpeechDetectionState == SDS_On);
				
				// Check for the need to chunk audio.
				if( FX_ANALYSIS_FONIX_TOO_MUCH_AUDIO == stat )
				{
					progress.Destroy();
					if( pSampleBuffer )
					{
						delete [] pSampleBuffer;
						pSampleBuffer = NULL;
					}
					if( g_pProgressFunctor )
					{
						delete g_pProgressFunctor;
						g_pProgressFunctor = NULL;
					}

					// Create an empty phoneme / word list.
					FxPhonWordList* pPhonemeWordList = new FxPhonWordList();
					pPhonemeWordList->SetMinPhonemeLength(_minimumPhonemeLength);
					// Set the phoneme list.
					pPhonemeWordList->ClearDirtyFlag();
					pAnimUserData->SetPhonemeWordList(pPhonemeWordList);
					delete pPhonemeWordList;
					pPhonemeWordList = NULL;

					FxWarn( "FxAnalysisCoreFonix::CreateAnim - the specified audio was too long to safely analyze; chunking and using no text..." );
					
					// Determine the appropriate chunk length.  The limit for Fonix is 90 seconds,
					// but to be safe we first attempt 60 second chunks so that any left over
					// audio should never be greater than 90 seconds.  Taking two thirds of the
					// audio max (defaulted to 90 seconds) gives 60 seconds as the result.  Taking
					// two thirds of the audio max in this way should allow for arbitrary audio max
					// values with the same basic algorithm.
					FxDReal chunkSize     = static_cast<FxReal>(*a_audiomax) * (2.0 / 3.0);
					FxDReal audioDuration = pDigitalAudio->GetDuration();
					FxSize  numChunks     = static_cast<FxSize>(audioDuration / chunkSize);
					FxDReal leftOverAudio = audioDuration - (numChunks * chunkSize);
					numChunks += (0.0 == leftOverAudio) ? 0 : 1;

					FxMsg( "audioDuration: %f  leftOverAudio: %f  chunkSize: %fs  numChunks: %d",
						audioDuration, leftOverAudio, chunkSize, numChunks );

					// More often than not straight 60 second chunks should work,
					// but if they don't we must converge on a chunk size that will work.
					if( 0.0 != leftOverAudio && leftOverAudio < static_cast<FxReal>(*a_audiomin) )
					{
						FxDReal slack = leftOverAudio * 10.0;
						while( slack <= static_cast<FxReal>(*a_audiomin) )
						{
							slack *= 10.0;
						}
						chunkSize -= slack;
						numChunks = static_cast<FxSize>(audioDuration / chunkSize);
						leftOverAudio = audioDuration - (numChunks * chunkSize);
						numChunks += (0.0 == leftOverAudio) ? 0 : 1;
						FxMsg( "convergence:  audioDuration: %f  leftOverAudio: %f  chunkSize: %fs  numChunks: %d",
							audioDuration, leftOverAudio, chunkSize, numChunks );
					}
					
					FxReal chunkStart = 0.0f;
					FxReal chunkEnd   = chunkSize;
					for( FxSize chunk = 0; chunk < numChunks; ++chunk )
					{
						FxMsg( "Analyzing chunk %d of %d: [%f, %f]...", chunk+1, numChunks, chunkStart, chunkEnd );
						if( !ReanalyzeRange(pAnim, chunkStart, chunkEnd, L"") )
						{
							FxError( "FxAnalysisCoreFonix::ReanalyzeRange failed for chunk %d of %d.", chunk+1, numChunks );
						}
						chunkStart = chunkEnd;
						chunkEnd += chunkSize;
						chunkEnd = FxClamp(0.0f, chunkEnd, static_cast<FxReal>(audioDuration));
					}

					stat = FX_ANALYSIS_FONIX_NOERROR;
				}
				else
				{
					// In this case FxAnalysisFonixAnalyze() returned an error
					// case that was not handled by the FX_ANALYSIS_FONIX_TOO_MUCH_AUDIO
					// block above, so we issue a warning to the console and 
					// reanalyze without text and with speech detection disabled.
					// Then the rest of the code flows just as normal.  This way
					// we should be able to trap all errors and analyze all audio
					// files and produce some results in all cases.
					FxBool usedText = FxTrue;
					if( FX_ANALYSIS_FONIX_NOERROR != stat )
					{
						usedText = FxFalse;
						FxWarn( "First analysis attempt failed.  Trying again with no text and speech detection disabled." );
						// Second analysis attempt.
						stat = FxAnalysisFonixAnalyze(pDigitalAudio->GetBitsPerSample(),
													  pDigitalAudio->GetSampleRate(),
													  pDigitalAudio->GetChannelCount(),
													  pDigitalAudio->GetSampleCount(),
													  reinterpret_cast<const FxChar*>(pSampleBuffer),
													  0,
													  NULL,
												      GetLanguageCode(pAnimUserData->GetLanguage()),
													  SDS_Off);
					}

					CHECK_FXAL_ERROR_NOTIFY_SUCCESS(stat, analysis complete.);

					if( strippedText.Length() && usedText )
					{
						FxMsgW( L"Used text: %s", strippedText.GetWCstr() );
					}

					// Create the phoneme / word list based on the analysis results.
					FxPhonWordList* pPhonemeWordList = new FxPhonWordList();
					pPhonemeWordList->SetMinPhonemeLength(_minimumPhonemeLength);
					FxInt32 numPhonemes = 0;
					stat = FxAnalysisFonixGetNumPhonemes(numPhonemes);
					CHECK_FXAL_ERROR(stat);
					// Get all of the phonemes.
					for( FxInt32 i = 0; i < numPhonemes; ++i )
					{
						int phonemeType = 0;
						FxDReal phonemeStartTime = 0.0;
						FxDReal phonemeEndTime   = 0.0;
						stat = FxAnalysisFonixGetPhonemeInfo(i, phonemeType, phonemeStartTime, phonemeEndTime);
						CHECK_FXAL_ERROR(stat);
						pPhonemeWordList->AppendPhoneme(static_cast<FxPhoneme>(phonemeType), phonemeStartTime, phonemeEndTime);
					}

					FxInt32 numWords = 0;
					stat = FxAnalysisFonixGetNumWords(numWords);
					CHECK_FXAL_ERROR(stat);
					FxInt32 wordLength = 63;
					FxWChar word[64];
					FxDReal wordStartTime = 0.0;
					FxDReal wordEndTime   = 0.0;
					// Get all of the words and group them to the correct phonemes.
					for( FxInt32 i = 0; i < numWords; ++i )
					{
						stat = FxAnalysisFonixGetWordInfo(i, wordLength, word, wordStartTime, wordEndTime);
						CHECK_FXAL_ERROR(stat);
						pPhonemeWordList->GroupToWord(wxString(word), 
							static_cast<FxReal>(wordStartTime), static_cast<FxReal>(wordEndTime));
					}

					// Set the configurations.
					FxCoarticulationConfig* pCoarticulationConfig = 
						FxCGConfigManager::GetGenericCoarticulationConfig(pAnimUserData->GetCoarticulationConfig().GetData());
					FxGestureConfig* pGestureConfig = 
						FxCGConfigManager::GetGestureConfig(pAnimUserData->GetGestureConfig().GetData());
					//@todo Fix this at some point.
					// The previous call to FxCGConfigManager::GetGestureConfig() could have caused all of the
					// configurations to be rescanned in the rare case that the gesture configuration could not
					// be found.  Since those are stored as arrays, the previous pointer is no longer valid, so
					// cache it again.
					pCoarticulationConfig = 
						FxCGConfigManager::GetGenericCoarticulationConfig(pAnimUserData->GetCoarticulationConfig().GetData());

					if( pCoarticulationConfig )
					{
						_coarticulationObj.SetConfig(*pCoarticulationConfig);
					}
					if( pGestureConfig )
					{
						_speechGesturesObj.SetConfig(*pGestureConfig);
					}

					// Set up for speech gesture generation.
					_speechGesturesObj.Initialize(pDigitalAudio->GetSampleBuffer(), 
						                          pDigitalAudio->GetSampleCount(), 
						                          pDigitalAudio->GetSampleRate());
					// Update the animation tracks.
					ChangePhonemeList(pAnim, NULL, pPhonemeWordList);

					// Set the phoneme list.
					pPhonemeWordList->ClearDirtyFlag();
					pAnimUserData->SetPhonemeWordList(pPhonemeWordList);
					delete pPhonemeWordList;
					pPhonemeWordList = NULL;

					progress.Destroy();
					if( pSampleBuffer )
					{
						delete [] pSampleBuffer;
						pSampleBuffer = NULL;
					}
					if( g_pProgressFunctor )
					{
						delete g_pProgressFunctor;
						g_pProgressFunctor = NULL;
					}
				}

				if( FX_ANALYSIS_FONIX_NOERROR != stat )
				{
					return FxFalse;
				}
			}
			else
			{
				FxMsg( "FxAnalysisCoreFonix::CreateAnim - audio pointer is NULL; assuming silent animation." );
			}
			return FxTrue;
		}
	}
	return FxFalse;
}

FxBool FxAnalysisCoreFonix::
ReanalyzeRange( FxAnim* pAnim, FxReal rangeStart, FxReal rangeEnd, const FxWString& strippedText )
{
	if( pAnim )
	{
		FxReal duration = rangeEnd - rangeStart;
        if( duration > static_cast<FxReal>(*a_audiomax) )
		{
			FxError( "FxAnalysisCoreFonix::ReanalyzeRange - range is greater than %f seconds!", 
				static_cast<FxReal>(*a_audiomax) );
			return FxFalse;
		}
		FxAnimUserData* pAnimUserData = reinterpret_cast<FxAnimUserData*>(pAnim->GetUserData());
		if( pAnimUserData )
		{
			// If the digital audio pointer is NULL, nothing can happen here.
			FxDigitalAudio* pDigitalAudio = pAnimUserData->GetDigitalAudioPointer();
			if( pDigitalAudio )
			{
				// Convert everything into samples and sample counts.
				FxUInt32 startSample = rangeStart * pDigitalAudio->GetSampleRate();
				FxSize startSampleByteOffset = (startSample * pDigitalAudio->GetBitsPerSample()) / FX_CHAR_BIT;
				FxUInt32 sampleRange = (rangeEnd - rangeStart) * pDigitalAudio->GetSampleRate();

				// Make a new digital audio object.
				FxByte* pTempSampleBuffer = pDigitalAudio->ExportSampleBuffer(pDigitalAudio->GetBitsPerSample());
				if( pTempSampleBuffer )
				{
					FxDigitalAudio audioChunk(pDigitalAudio->GetBitsPerSample(),
										      pDigitalAudio->GetSampleRate(),
											  pDigitalAudio->GetChannelCount(),
											  sampleRange,
											  pTempSampleBuffer + startSampleByteOffset);

					delete [] pTempSampleBuffer;
					pTempSampleBuffer = NULL;

					SpeechDetectionState localSpeechDetectionState = _speechDetectionState;
					if( localSpeechDetectionState == SDS_Automatic )
					{
						localSpeechDetectionState = SDS_On;
					}

					// Set up for analysis.
					FxByte* pSampleBuffer = audioChunk.ExportSampleBuffer(audioChunk.GetBitsPerSample());
					FxProgressDialog progress(_("Analyzing..."), FxStudioApp::GetMainWindow());
					g_pProgressFunctor = new FxProgressCallback<FxProgressDialog>(&progress, &FxProgressDialog::Update);
					//@todo This will always fail until Fonix supports progress callbacks.
					FxAnalysisFonixRegisterAnalysisCallback(g_ProgessCallback);
					FxAnalysisFonixError stat = FxAnalysisFonixAnalyze(audioChunk.GetBitsPerSample(),
																		     audioChunk.GetSampleRate(),
																			 audioChunk.GetChannelCount(),
																			 audioChunk.GetSampleCount(),
																			 reinterpret_cast<const FxChar*>(pSampleBuffer),
																		     strippedText.Length(),
																			 strippedText.GetData(),
																			 GetLanguageCode(pAnimUserData->GetLanguage()),
																			 localSpeechDetectionState == SDS_On);
					CHECK_FXAL_ERROR_NOTIFY_SUCCESS(stat, analysis complete.);

					if( strippedText.Length() )
					{
						FxMsgW( L"Used text: %s", strippedText.GetWCstr() );
					}

					// Create the phoneme / word list based on the analysis results.
					FxPhonWordList* pPhonemeWordList = pAnimUserData->GetPhonemeWordListPointer();
					if( pPhonemeWordList )
					{
						FxPhonWordList tempPhonemeWordList;
						tempPhonemeWordList.SetMinPhonemeLength(_minimumPhonemeLength);
						
						FxInt32 numPhonemes = 0;
						stat = FxAnalysisFonixGetNumPhonemes(numPhonemes);
						CHECK_FXAL_ERROR(stat);
						// Get all of the phonemes.
						for( FxInt32 i = 0; i < numPhonemes; ++i )
						{
							int phonemeType = 0;
							FxDReal phonemeStartTime = 0.0;
							FxDReal phonemeEndTime   = 0.0;
							stat = FxAnalysisFonixGetPhonemeInfo(i, phonemeType, phonemeStartTime, phonemeEndTime);
							CHECK_FXAL_ERROR(stat);
							tempPhonemeWordList.AppendPhoneme(static_cast<FxPhoneme>(phonemeType), phonemeStartTime, phonemeEndTime);
						}

						FxInt32 numWords = 0;
						stat = FxAnalysisFonixGetNumWords(numWords);
						CHECK_FXAL_ERROR(stat);
						FxInt32 wordLength = 63;
						FxWChar word[64];
						FxDReal wordStartTime = 0.0;
						FxDReal wordEndTime   = 0.0;
						// Get all of the words and group them to the correct phonemes.
						for( FxInt32 i = 0; i < numWords; ++i )
						{
							stat = FxAnalysisFonixGetWordInfo(i, wordLength, word, wordStartTime, wordEndTime);
							CHECK_FXAL_ERROR(stat);
							tempPhonemeWordList.GroupToWord(wxString(word), 
								static_cast<FxReal>(wordStartTime), 
								static_cast<FxReal>(wordEndTime));
						}

						PostProcessPhonemeList( &tempPhonemeWordList, 
							static_cast<FxReal>(audioChunk.GetDuration()) );

						FxSize startIndex = pPhonemeWordList->FindPhoneme(rangeStart);
						FxSize endIndex = pPhonemeWordList->FindPhoneme(rangeEnd);
						pPhonemeWordList->Splice(tempPhonemeWordList, startIndex, endIndex);

						// Set the configurations.
						FxCoarticulationConfig* pCoarticulationConfig = 
							FxCGConfigManager::GetGenericCoarticulationConfig(pAnimUserData->GetCoarticulationConfig().GetData());
						FxGestureConfig* pGestureConfig = 
							FxCGConfigManager::GetGestureConfig(pAnimUserData->GetGestureConfig().GetData());
						//@todo Fix this at some point.
						// The previous call to FxCGConfigManager::GetGestureConfig() could have caused all of the
						// configurations to be rescanned in the rare case that the gesture configuration could not
						// be found.  Since those are stored as arrays, the previous pointer is no longer valid, so
						// cache it again.
						pCoarticulationConfig = 
							FxCGConfigManager::GetGenericCoarticulationConfig(pAnimUserData->GetCoarticulationConfig().GetData());

						if( pCoarticulationConfig )
						{
							_coarticulationObj.SetConfig(*pCoarticulationConfig);
						}
						if( pGestureConfig )
						{
							_speechGesturesObj.SetConfig(*pGestureConfig);
						}

						// Set up for speech gesture generation.
						_speechGesturesObj.Initialize(pDigitalAudio->GetSampleBuffer(), 
													  pDigitalAudio->GetSampleCount(), 
													  pDigitalAudio->GetSampleRate());
						// Update the animation tracks.
						ChangePhonemeList(pAnim, NULL, pPhonemeWordList);

						// Set the phoneme list.
						pPhonemeWordList->ClearDirtyFlag();
					}
					
					progress.Destroy();
					if( pSampleBuffer )
					{
						delete [] pSampleBuffer;
						pSampleBuffer = NULL;
					}
					if( g_pProgressFunctor )
					{
						delete g_pProgressFunctor;
						g_pProgressFunctor = NULL;
					}

					if( FX_ANALYSIS_FONIX_NOERROR != stat )
					{
						return FxFalse;
					}
				}
			}
			else
			{
				FxWarn( "FxAnalysisCoreFonix::ReanalyzeRange - audio pointer is NULL." );
			}
			return FxTrue;
		}
	}
	return FxFalse;
}

FxBool FxAnalysisCoreFonix::ChangePhonemeMapping( FxAnim* pAnim, FxPhonemeMap* pPhonemeMap )
{
	if( pPhonemeMap )
	{
		// Set the new mapping.
		_coarticulationObj.BeginMapping();
		for( FxSize i = 0; i < pPhonemeMap->GetNumMappingEntries(); ++i ) 
		{
			const FxPhonToNameMap& entry = pPhonemeMap->GetMappingEntry(i);
			if( !_coarticulationObj.AddMappingEntry(entry.phoneme, entry.name, entry.amount) )
			{
				// Clean up - don't leave the coarticulation object in an invalid state.
				_coarticulationObj.EndMapping();
				return FxFalse;
			}
		}
		_coarticulationObj.EndMapping();
		// Update the animation tracks.
		if( pAnim )
		{
			FxAnimUserData* pAnimUserData = reinterpret_cast<FxAnimUserData*>(pAnim->GetUserData());
			if( pAnimUserData )
			{
				return ChangePhonemeList(pAnim, NULL, pAnimUserData->GetPhonemeWordListPointer());
			}
		}
	}
	return FxTrue;
}

FxBool FxAnalysisCoreFonix::ChangePhonemeList( FxAnim* pAnim, FxPhonemeMap* FxUnused(pPhonemeMap), FxPhonWordList* pPhonemeWordList )
{
	if( pAnim && pPhonemeWordList && _pAnimController )
	{
		// Get the animation user data.
		FxAnimUserData* pAnimUserData = reinterpret_cast<FxAnimUserData*>(pAnim->GetUserData());

		// Set the new phoneme list.
		_coarticulationObj.BeginPhonemeList(pPhonemeWordList->GetNumPhonemes());
		for( FxSize i = 0; i < pPhonemeWordList->GetNumPhonemes(); ++i )
		{
			_coarticulationObj.AppendPhoneme(pPhonemeWordList->GetPhonemeEnum(i),
				                             pPhonemeWordList->GetPhonemeStartTime(i),
				                             pPhonemeWordList->GetPhonemeEndTime(i));
		}
		_coarticulationObj.EndPhonemeList();
		
		// Run the coarticulation.
		_coarticulationObj.Process();
		
		// Update the animation tracks.
		FxBool addedCurves = FxFalse;
		for( FxSize i = 0; i < _coarticulationObj.GetNumCurves(); ++i )
		{
			const FxAnimCurve& curve = _coarticulationObj.GetCurve(i);
			FxSize index = pAnim->FindAnimCurve(curve.GetName());

			// Check if the curve is in the animation and owned by the analysis.
			if( index != FxInvalidIndex &&
				_pAnimController->IsCurveOwnedByAnalysis(index) )
			{
				// Clean up the user data on the keys.
				_pAnimController->DestroyCurveUserData(index);
				FxAnimCurve& curveInAnim = pAnim->GetAnimCurveM(index);
				curveInAnim.RemoveAllKeys();

				// Insert keys in reverse order for a speed up on key insertion.
				for( FxInt32 currentKey = curve.GetNumKeys() - 1; currentKey >= 0; --currentKey )
				{
					curveInAnim.InsertKey(curve.GetKey(currentKey));
				}
			}
		}

		// Get the random seed.
		FxInt32 randomSeed = 0;
		if( pAnimUserData )
		{
			randomSeed = pAnimUserData->GetRandomSeed();
		}

		// Run the gestures processing.
		FxPhonemeList genericPhonList;
		pPhonemeWordList->ToGenericFormat(genericPhonList);
		_speechGesturesObj.Align(&genericPhonList, randomSeed);

		for( FxSize i = GT_First; i <= GT_Last; ++i )
		{
			const FxAnimCurve& curve = _speechGesturesObj.GetTrack(static_cast<FxGestureTrack>(i));
			FxSize index = pAnim->FindAnimCurve(curve.GetName());

			// Check if the curve is in the animation and owned by the analysis.
			if( index != FxInvalidIndex && 
				_pAnimController->IsCurveOwnedByAnalysis(index))
			{
				// Clean up the user data on the keys.
				_pAnimController->DestroyCurveUserData(index);
				FxAnimCurve& curveInAnim = pAnim->GetAnimCurveM(index);
				curveInAnim.RemoveAllKeys();

				// Insert keys in reverse order for a speed up on key insertion.
				for( FxInt32 currentKey = curve.GetNumKeys() - 1; currentKey >= 0; --currentKey )
				{
					curveInAnim.InsertKey(curve.GetKey(currentKey));
				}
			}
		}

		// Finish up.
		_pAnimController->ProofUserData();
		if( addedCurves )
		{
			// The curves have changed.
			FxSessionProxy::ActorDataChanged(ADCF_CurvesChanged);
		}
	}
	return FxTrue;
}

void FxAnalysisCoreFonix::OnDebugFonixUpdated( FxConsoleVariable& cvar )
{
	FxBool bToggle = static_cast<FxBool>(cvar);
	if( bToggle )
	{
			FxAnalysisFonixSetLogOutputFunction(FxAnalysisFonixLogOutputCallback);
	}
	else
	{
			FxAnalysisFonixSetLogOutputFunction(NULL);
	}
}

FxInt32 FxAnalysisCoreFonix::GetLanguageCode( const FxString& languageName )
{
	for( FxSize i = 0; i < _languages.Length(); ++i )
	{
		if( _languages[i].languageName == languageName )
		{
			return _languages[i].languageCode;
		}
	}
	// -1 is the invalid language code.
	FxWarn("WARNING: invalid language specified!");
	return -1;
}

void FxAnalysisCoreFonix::PostProcessPhonemeList( FxPhonWordList* pPhonList, FxReal audioEndTime )
{
	for( FxSize i = 0; i < pPhonList->GetNumPhonemes(); ++i )
	{
		// Remove any phoneme entirely before 0.
		if( pPhonList->GetPhonemeEndTime(i) < 0.f )
		{
			pPhonList->RemovePhoneme(i);
		}

		// Remove any phonemes entirely after the end time of the audio.
		if( pPhonList->GetPhonemeStartTime(i) > audioEndTime )
		{
			pPhonList->RemovePhoneme(i);
		}
	}

	if( pPhonList->GetNumPhonemes() )
	{
		// Truncate first phoneme to start at 0.
		FxReal newStartTime = pPhonList->ModifyPhonemeStartTime(0, 0.f);
		//@todo hack - this will only work if the next phoneme can give away
		// min phoneme length worth of it's own duration.  otherwise, it's still
		// going to yield some busted phoneme lists once we start splicing the
		// results of reanalyze range into pre-existing phoneme list.
		if( newStartTime < 0.0f )
		{
			pPhonList->ModifyPhonemeEndTime(0, 0.f + pPhonList->GetMinPhonemeLength());
			pPhonList->ModifyPhonemeStartTime(0, 0.f);
		}

		// Truncate the last phoneme to end at the audio's end time.
		pPhonList->ModifyPhonemeEndTime(pPhonList->GetNumPhonemes() - 1, audioEndTime);
	}
}

} // namespace Face

} // namespace OC3Ent

#endif // __APPLE__
