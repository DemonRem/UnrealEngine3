//------------------------------------------------------------------------------
// A widget that uses the FaceFx Analysis Library to produce animation
// results.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "stdwx.h"
#include "FxAnalysisWidget.h"
#include "FxVM.h"
#include "FxMath.h"
#include "FxAnimUserData.h"
#include "FxUndoRedoManager.h"
#include "FxSessionProxy.h"
#include "FxConsole.h"

#include "FxAnalysisCoreFonix.h"

namespace OC3Ent
{

namespace Face
{

FxReal FxAnalysisWidget::_audioMinimum = 0.5f;
FxReal FxAnalysisWidget::_audioMaximum = 90.0f;

FxAnalysisWidget::FxAnalysisWidget( FxWidgetMediator* mediator )
	: FxWidget(mediator)
	, _pAnimController(NULL)
	, _pActor(NULL)
	, _pAnim(NULL)
	, _pPhonemeList(NULL)
	, _pPhonemeMap(NULL)
	, _pAnalysisCore(NULL)
{
}

FxAnalysisWidget::~FxAnalysisWidget()
{
	if( _pAnalysisCore )
	{
		delete _pAnalysisCore;
		_pAnalysisCore = NULL;
	}
}

void FxAnalysisWidget::SetAnimControllerPointer( FxAnimController* pAnimController )
{
	_pAnimController = pAnimController;
}

FxBool FxAnalysisWidget::IsExpired( void ) const
{
	if( _pAnalysisCore )
	{
		return _pAnalysisCore->IsExpired();
	}
	return FxTrue;
}

FxAnalysisCoreLicenseeInfo FxAnalysisWidget::GetLicenseeInfo( void ) const
{
	if( _pAnalysisCore )
	{
		return _pAnalysisCore->GetLicenseeInfo();
	}
	return FxAnalysisCoreLicenseeInfo();
}

void FxAnalysisWidget::OnAppStartup( FxWidget* FxUnused(sender) )
{
#ifndef __APPLE__
	FxAnalysisCore::StaticClass();
	FxAnalysisCoreFonix::StaticClass();
	// See if we can startup the Fonix core.
	_pAnalysisCore = new FxAnalysisCoreFonix();
	if( _pAnalysisCore && _pAnalysisCore->Startup(_pAnimController) )
	{
		// Fonix core started successfully.
		FxMsg( "Running Fonix core." );
		// Print a message if the Fonix core has expired.
		if( _pAnalysisCore->IsExpired() )
		{
			FxError( "<b>Fonix core expired!</b>" );
		}
	}
	else
	{
		// Fonix didn't start for some reason.
		delete _pAnalysisCore;
		_pAnalysisCore = NULL;
		FxError( "<b>FxAnalysisWidget: no valid analysis core found!</b>" );
	}

	// Read the audio minimum and maximum from the core object.
	if( _pAnalysisCore )
	{
		FxReal audioMin, audioMax;
		if( _pAnalysisCore->GetAudioMinimum(audioMin) &&
			_pAnalysisCore->GetAudioMaximum(audioMax) )
		{
			_audioMinimum = audioMin;
			_audioMaximum = audioMax;

			FxConsoleVariable* a_audiomin = FxVM::FindConsoleVariable("a_audiomin");
			FxConsoleVariable* a_audiomax = FxVM::FindConsoleVariable("a_audiomax");
			if( a_audiomin && a_audiomax )
			{
				a_audiomin->SetFloat(_audioMinimum);
				a_audiomax->SetFloat(_audioMaximum);
			}
		}
	}
#endif
}

void FxAnalysisWidget::OnAppShutdown( FxWidget* FxUnused(sender) )
{
	if( _pAnalysisCore )
	{
		_pAnalysisCore->Shutdown();
	}
}

void FxAnalysisWidget::OnActorChanged( FxWidget* FxUnused(sender), FxActor* actor )
{
	_pActor = actor;
}

void FxAnalysisWidget::
OnQueryLanguageInfo( FxWidget* FxUnused(sender), FxArray<FxLanguageInfo>& languages )
{
	if( _pAnalysisCore )
	{
		languages = _pAnalysisCore->GetSupportedLanguages();
	}
}

void FxAnalysisWidget::
OnCreateAnim( FxWidget* FxUnused(sender), const FxName& FxUnused(animGroupName), FxAnim* anim )
{
	_pAnim = anim;
	if( _pAnalysisCore )
	{
		if( _pActor && _pAnim )
		{
			_setSpeechDetectionState();

			// The sender of this message should have verified that the animation
			// group exists, the animation will not be a duplicate, and that the 
			// animation user data was created with a valid digital audio pointer
			// (including resampling) and with valid analysis text (if any).
			FxAnimUserData* pUserData = reinterpret_cast<FxAnimUserData*>(_pAnim->GetUserData());
			if( pUserData && pUserData->ShouldContainAnalysisResults() )
			{
				FxUnicodeTextTagParser textTagParser;
				FxWString strippedText = L"";
				FxWString analysisText = pUserData->GetAnalysisText();
				if( analysisText.Length() > 0 )
				{
					// Parse the text tags.
					textTagParser.InitializeParser(analysisText);
					// The stripped string is the text with all of the text tags
					// stripped (as well as punctuation).
					strippedText = textTagParser.GetStrippedText();
				}

				if( _pAnalysisCore->CreateAnim(_pAnim, _pPhonemeMap, strippedText) )
				{
					// Get the phoneme list.
					_pPhonemeList = pUserData->GetPhonemeWordListPointer();

					// Process the text tags.
					textTagParser.Parse(_pPhonemeList);
					_processTextTags(textTagParser);

					// Find the bounds of the animation.
					_pAnim->FindBounds();
				}
				else
				{
					FxError( "<b>FxAnalysisWidget: failed to create animation!</b>" );
				}
			}		
		}
		else
		{
			FxWarn( "<b>FxAnalysisWidget::OnCreateAnim - animation or actor pointer is NULL!</b>" );
		}
	}
	else
	{
		FxError( "<b>FxAnalysisWidget: no valid analysis core!</b>" );
	}
}

void FxAnalysisWidget::
OnAudioMinMaxChanged( FxWidget* FxUnused(sender), FxReal audioMin, FxReal audioMax )
{
	if( _pAnalysisCore )
	{
		_pAnalysisCore->ChangeAudioMinimum(audioMin);
		_pAnalysisCore->ChangeAudioMaximum(audioMax);

		FxReal newAudioMin, newAudioMax;
		if( _pAnalysisCore->GetAudioMinimum(newAudioMin) &&
			_pAnalysisCore->GetAudioMaximum(newAudioMax) )
		{
			_audioMinimum = newAudioMin;
			_audioMaximum = newAudioMax;

			FxConsoleVariable* a_audiomin = FxVM::FindConsoleVariable("a_audiomin");
			FxConsoleVariable* a_audiomax = FxVM::FindConsoleVariable("a_audiomax");
			if( a_audiomin && a_audiomax )
			{
				a_audiomin->SetFloat(_audioMinimum);
				a_audiomax->SetFloat(_audioMaximum);
			}
		}
	}
}

void FxAnalysisWidget::
OnReanalyzeRange( FxWidget* FxUnused(sender), FxAnim* anim, FxReal rangeStart, 
				  FxReal rangeEnd, const FxWString& analysisText )
{
	if( _pAnalysisCore )
	{
		if( _pActor && anim && anim == _pAnim )
		{
			_setSpeechDetectionState();

			FxAnimUserData* pUserData = reinterpret_cast<FxAnimUserData*>(anim->GetUserData());
			if( pUserData && pUserData->ShouldContainAnalysisResults() )
			{
				FxUnicodeTextTagParser textTagParser;
				FxWString strippedText = L"";
				if( analysisText.Length() > 0 )
				{
					// Parse the text tags.
					textTagParser.InitializeParser(analysisText);
					// The stripped string is the text with all of the text tags
					// stripped (as well as punctuation).
					strippedText = textTagParser.GetStrippedText();
				}

				if( _pAnalysisCore->ReanalyzeRange(_pAnim, rangeStart, rangeEnd, strippedText) )
				{
					// This is not an undoable operation, so the undo / redo buffers should
					// be flushed.
					FxUndoRedoManager::Flush();
					// The session should also be flagged as forced dirty.
					FxSessionProxy::SetIsForcedDirty(FxTrue);

					// Find the bounds of the animation.
					_pAnim->FindBounds();

					// The phoneme list has changed and all of the widgets should
					// be notified.
					FxAnimUserData* pUserData = reinterpret_cast<FxAnimUserData*>(_pAnim->GetUserData());
					if( pUserData )
					{
						// Get the phoneme list.
						FxPhonWordList* pPhonemeList = pUserData->GetPhonemeWordListPointer();

						// The analysis text could have changed, so we should update 
						// it with the words from the phoneme word list here.
						if( pPhonemeList )
						{
							wxString newAnalysisText;
							for( FxSize i = 0; i < pPhonemeList->GetNumWords(); ++i )
							{
								newAnalysisText.Append(pPhonemeList->GetWordText(i));
								newAnalysisText.Append(wxT(" "));
							}
							// Remove that last space if it's been appended.
							if( newAnalysisText != wxEmptyString )
							{
								newAnalysisText.RemoveLast();
							}
							pUserData->SetAnalysisText(FxWString(newAnalysisText.GetData()));
							pUserData->SetIsSafeToUnload(FxFalse);
						}

						if( _mediator )
						{
							_mediator->OnAnimPhonemeListChanged(this, _pAnim, pPhonemeList);
						}
					}
				}
			}
			else
			{
				FxError( "<b>FxAnalysisWidget: failed to reanalyze range!</b>" );
			}
		}
		else
		{
			FxWarn( "<b>FxAnalysisWidget::OnReanalyzeRange - animation or actor pointer is NULL or animation pointer mismatch!</b>" );
		}
	}
	else
	{
		FxError( "<b>FxAnalysisWidget: no valid analysis core!</b>" );
	}
}

void FxAnalysisWidget::
OnAnimChanged( FxWidget* FxUnused(sender), const FxName& FxUnused(animGroupName), FxAnim* anim )
{
	_pAnim = anim;
	if( _pAnalysisCore )
	{
		_pAnalysisCore->ChangeAnim(_pAnim);
	}
}

void FxAnalysisWidget::
OnAnimPhonemeListChanged( FxWidget* FxUnused(sender), FxAnim* anim, FxPhonWordList* phonemeList )
{
	_pPhonemeList = phonemeList;
	if( _pAnim == anim )
	{
		if( _pAnim && _pPhonemeList && _pAnalysisCore )
		{
			FxAnimUserData* pUserData = reinterpret_cast<FxAnimUserData*>(_pAnim->GetUserData());
			if( pUserData && pUserData->ShouldContainAnalysisResults() )
			{
				if( !_pAnalysisCore->ChangePhonemeList(_pAnim, _pPhonemeMap, _pPhonemeList) )
				{
					FxError( "FxAnalysisWidget: analysis core change phoneme list failed!" );
				}
			}
		}
	}
}

void FxAnalysisWidget::OnPhonemeMappingChanged( FxWidget* FxUnused(sender), FxPhonemeMap* phonemeMap )
{
	_pPhonemeMap = phonemeMap;
	if( _pPhonemeMap && _pAnimController && _pAnalysisCore )
	{
		if( !_pAnalysisCore->ChangePhonemeMapping(_pAnim, _pPhonemeMap) )
		{
			FxError( "FxAnalysisWidget: analysis core change phoneme mapping failed!" );
		}
		// Force the animation curves owned by the analysis to stay synchronized
		// with the new mapping.
		OnAnimPhonemeListChanged(NULL, _pAnim, _pPhonemeList);
	}
}

void FxAnalysisWidget::OnGenericCoarticulationConfigChanged( FxWidget* FxUnused(sender), FxCoarticulationConfig* FxUnused(config) )
{
	if( _pAnalysisCore )
	{
		// Rather than exposing FxCG to FxAnalysisLIPSinc, we hijack the ChangeAnim() 
		// interface to force FxAnalysisFonix to reload the current configuration.
		_pAnalysisCore->ChangeAnim(_pAnim);
	}
}

void FxAnalysisWidget::OnGestureConfigChanged( FxWidget* FxUnused(sender), FxGestureConfig* FxUnused(config) )
{
	if( _pAnalysisCore )
	{
		// Rather than exposing FxCG to FxAnalysisLIPSinc, we hijack the ChangeAnim() 
		// interface to force FxAnalysisFonix to reload the current configuration.
		_pAnalysisCore->ChangeAnim(_pAnim);
	}
}

void FxAnalysisWidget::OnAudioMinimumUpdated( FxConsoleVariable& cvar )
{
	_audioMinimum = static_cast<FxReal>(cvar);
	FxSessionProxy::ChangeAudioMinMax(_audioMinimum, _audioMaximum);
}

void FxAnalysisWidget::OnAudioMaximumUpdated( FxConsoleVariable& cvar )
{
	_audioMaximum = static_cast<FxReal>(cvar);
	FxSessionProxy::ChangeAudioMinMax(_audioMinimum, _audioMaximum);
}

void FxAnalysisWidget::_processTextTags( FxUnicodeTextTagParser& textTagParser )
{
	if( _pAnim )
	{
		FxAnimUserData* pUserData = reinterpret_cast<FxAnimUserData*>(_pAnim->GetUserData());
		if( pUserData )
		{
			// Process the text tags.
			for( FxSize currentTag = 0; currentTag < textTagParser.GetNumTextTags(); ++currentTag )
			{
				const FxUnicodeTextTagInfo& tag = textTagParser.GetTextTag(currentTag);

				// Debugging stuff.
#ifdef FX_DEBUG_TEXT_TAGGING
				FxString tagCurveType;
				switch( tag.GetCurveType() )
				{
				case FxUnicodeTextTagInfo::CT_Quadruplet:
					tagCurveType = "quadruplet";
					break;
				case FxUnicodeTextTagInfo::CT_LeadingTriplet:
					tagCurveType = "leading_triplet";
					break;
				case FxUnicodeTextTagInfo::CT_CenterTriplet:
					tagCurveType = "center_triplet";
					break;
				case FxUnicodeTextTagInfo::CT_TrailingTriplet:
					tagCurveType = "trailing_triplet";
					break;
				default:
					break;
				}
				FxDebugTextTaggingLevelOne( "<b><i>Applying text tag %d: name=%s type=%s duration=%f timeshift=%f easein=%f easeout=%f v1=%f v2=%f v3=%f v4=%f startWord=%d endWord=%d</i></b>",
					currentTag, tag.GetCurveName().GetAsString().GetData(), tagCurveType.GetData(), tag.GetDuration(), tag.GetTimeShift(), tag.GetEaseIn(), 
					tag.GetEaseOut(), tag.GetV1(), tag.GetV2(), tag.GetV3(), tag.GetV4(), tag.GetStartWord(), tag.GetEndWord() );
#endif
				// End debugging stuff.

				FxSize curveIndex = _pAnim->FindAnimCurve(tag.GetCurveName().GetData());
				if( curveIndex == FxInvalidIndex )
				{
					// Add a new curve.
					FxAnimCurve newCurve;
					newCurve.SetName(tag.GetCurveName().GetData());
					_pAnim->AddAnimCurve(newCurve);
					curveIndex = _pAnim->FindAnimCurve(tag.GetCurveName().GetData());
					FxAssert( curveIndex != FxInvalidIndex );
				}
				// Modify an existing curve.
				FxAnimCurve& animCurve = _pAnim->GetAnimCurveM(curveIndex);

				// Line up the keys with word timings.
				FxPhonWordList* pWordList = pUserData->GetPhonemeWordListPointer();
				if( pWordList )
				{
					FxReal v2Time = pWordList->GetWordStartTime(tag.GetStartWord()) + tag.GetTimeShift();
					FxReal v3Time = pWordList->GetWordEndTime(tag.GetEndWord()) + tag.GetTimeShift();

					// Handle quadruplets.
					if( tag.GetCurveType() == FxUnicodeTextTagInfo::CT_Quadruplet )
					{
						FxAnimKey v1(v2Time - tag.GetEaseIn(), tag.GetV1());
						animCurve.InsertKey(v1);

						if( tag.GetDuration() != FxInvalidValue )
						{
							v3Time = v2Time + tag.GetDuration();
						}
						FxAnimKey v2(v2Time, tag.GetV2());
						animCurve.InsertKey(v2);
						FxAnimKey v3(v3Time, tag.GetV3());
						animCurve.InsertKey(v3);

						if( !FxRealEquality(tag.GetV4(), tag.GetV3()) )
						{
							FxAnimKey v4(v3Time + tag.GetEaseOut(), tag.GetV4());
							animCurve.InsertKey(v4);
						}
					}
					else
					{
						// Handle all triplets.
						FxReal centerTime = 0.0f;
						if( tag.GetCurveType() == FxUnicodeTextTagInfo::CT_LeadingTriplet )
						{
							centerTime = v2Time;
						}
						else if( tag.GetCurveType() == FxUnicodeTextTagInfo::CT_CenterTriplet )
						{
							centerTime = v2Time + ((v3Time - v2Time) * 0.5f);
						}
						else if( tag.GetCurveType() == FxUnicodeTextTagInfo::CT_TrailingTriplet )
						{
							centerTime = v3Time;
						}

						FxAnimKey v1(v2Time - tag.GetEaseIn(), tag.GetV1());
						animCurve.InsertKey(v1);

						FxAnimKey centerKey(centerTime, tag.GetV2());
						animCurve.InsertKey(centerKey);

						FxAnimKey v3(v3Time + tag.GetEaseOut(), tag.GetV3());
						animCurve.InsertKey(v3);
					}
				}
			}
		}
	}
}

void FxAnalysisWidget::_setSpeechDetectionState()
{
	FxConsoleVariable* cvar = FxVM::FindConsoleVariable("a_detectspeech");
	if( cvar && _pAnalysisCore )
	{
		FxString val = cvar->GetString();
		if( val == "yes" || val == "true" || val == "on" || val == "1" || 
		    val == "YES" || val == "TRUE" || val == "ON" )              
		{
			_pAnalysisCore->SetSpeechDetection(FxAnalysisCore::SDS_On);
		}
		else if( val == "no" || val == "false" || val == "off" || val == "0" ||
			     val == "NO" || val == "FALSE" || val == "OFF" )
		{
			_pAnalysisCore->SetSpeechDetection(FxAnalysisCore::SDS_Off);
		}
		else
		{
			_pAnalysisCore->SetSpeechDetection(FxAnalysisCore::SDS_Automatic);
		}
	}
}

} // namespace Face

} // namespace OC3Ent
