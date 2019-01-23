//------------------------------------------------------------------------------
// Provides generic phoneme-to-target style coarticulation.
// 
// Owner: John Briggs
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "FxCoarticulationGeneric.h"
#include "FxFaceGraph.h"
#include "FxDeltaNode.h"

namespace OC3Ent
{

namespace Face
{

// Constructor.
FxCoarticulationGeneric::FxCoarticulationGeneric()
	: _config(FxCoarticulationConfig()) // Default configuration.
	, _lastPhonEndTime(FxInvalidValue)
	, _mappingGraph(NULL)
	, _startTime(FX_REAL_MAX)
	, _endTime(FX_REAL_MIN)
{
}

// Destructor.
FxCoarticulationGeneric::~FxCoarticulationGeneric()
{
	if( _mappingGraph )
	{
		delete _mappingGraph;
		_mappingGraph = NULL;
	}
}

// Set the coarticulation configuration.
void FxCoarticulationGeneric::SetConfig( const FxCoarticulationConfig& config )
{
	_config = config;
}

// Begin the phoneme list.
void FxCoarticulationGeneric::BeginPhonemeList(FxSize numPhones)
{
	ClearPhonemeList();
	_phonData.Reserve(numPhones);
}

// Append a phoneme.
void FxCoarticulationGeneric::AppendPhoneme(FxPhoneme phonEnum, FxReal phonStart, FxReal phonEnd)
{
	// Check if we should split a diphthong.
	FxPhoneme firstPhoneme = phonEnum;
	FxPhoneme secondPhoneme = PHONEME_SIL;
	switch( phonEnum )
	{
	case PHONEME_EY:
		firstPhoneme = PHONEME_EH;
		secondPhoneme = PHONEME_IY;
		break;
	case PHONEME_AW:
		firstPhoneme = PHONEME_AE;
		secondPhoneme = PHONEME_UH;
		break;
	case PHONEME_AY:
		firstPhoneme = PHONEME_AE;
		secondPhoneme = PHONEME_IY;
		break;
	case PHONEME_OY:
		firstPhoneme = PHONEME_AO;
		secondPhoneme = PHONEME_IY;
		break;
	case PHONEME_OW:
		firstPhoneme = PHONEME_AO;
		secondPhoneme = PHONEME_UW; // UH?
		break;
	default:
		break;
	};

	// Undo the changes if the diphthong is too short.
	if( !_config.splitDiphthongs || (secondPhoneme != PHONEME_SIL && phonEnd - phonStart < _config.diphthongBoundary) )
	{
		firstPhoneme = phonEnum;
		secondPhoneme = PHONEME_SIL;
	}

	// Append the phoneme.
	_phonData.PushBack(FxCGPhonemeData(firstPhoneme, phonStart - _config.phoneInfo[firstPhoneme].anticipation ));	
	if( secondPhoneme == PHONEME_SIL )
	{
		// Set the end of the list.
		_lastPhonEndTime = phonEnd - _config.phoneInfo[firstPhoneme].anticipation;
	}
	else
	{
		// Append the second phoneme, if necessary
		_phonData.PushBack(FxCGPhonemeData(secondPhoneme, (phonEnd - phonStart) * 0.5f + phonStart - _config.phoneInfo[secondPhoneme].anticipation));
		// Set the end of the list.
		_lastPhonEndTime = phonEnd - _config.phoneInfo[secondPhoneme].anticipation;
	}

	// Make sure there are enough phonemes in the list before proceeding.
	if( (secondPhoneme == PHONEME_SIL && _phonData.Length() > 1) ||
		(secondPhoneme != PHONEME_SIL && _phonData.Length() > 2) )
	{
		FxSize i = _phonData.Length() - (secondPhoneme == PHONEME_SIL ? 1 : 2);
		FxSize iminus1 = _phonData.Length() - (secondPhoneme == PHONEME_SIL ? 2 : 3);
		// If we should convert to flaps, and
		if( _config.convertToFlaps && 
			// the previous phoneme is convertible to a flap, and
			(_phonData[iminus1].phoneme == PHONEME_T || _phonData[iminus1].phoneme == PHONEME_D) &&
			// there are enough phonemes in the list.
			((secondPhoneme == PHONEME_SIL && _phonData.Length() > 2) ||
			(secondPhoneme != PHONEME_SIL && _phonData.Length() > 3)) )
		{
			// Check if we should convert a T or D to a flap
			FxSize iminus2 = _phonData.Length() - (secondPhoneme == PHONEME_SIL ? 3 : 4);
			if( _phonData[iminus2].phoneme <= PHONEME_AX && _phonData[i].phoneme <= PHONEME_AX )
			{
				_phonData[iminus1].phoneme = PHONEME_FLAP;
			}
		}

		// Check if we should merge an HH into the succeeding phoneme.
		if( _phonData[iminus1].phoneme == PHONEME_HH )
		{
			_phonData[iminus1].phoneme = firstPhoneme;
			_phonData.PopBack();
		}
		// Check if the phoneme is the same as the preceding phoneme, and merge.
		else if( _phonData[iminus1].phoneme == _phonData[i].phoneme )
		{
			_phonData.PopBack();
		}
	}
}

// End the phoneme list.
void FxCoarticulationGeneric::EndPhonemeList()
{
	// Intentionally blank... not a necessary function call, but it looks nicer
	// if the code flows like
	// 
	// BeginPhonemeList();
	// AppendPhoneme(); // num phoneme times
	// EndPhonemeList();
	// 
	// ... just so that you don't think I forget to write the function body.
}

// Begin phoneme to track mapping input.
void FxCoarticulationGeneric::BeginMapping()
{
	ClearMapping();

	if( !_mappingGraph )
	{
		_mappingGraph = new FxFaceGraph();
	}
	
	// Add the phoneme nodes.
	_phonNodeMap.Reserve(NUM_PHONEMES);
	for( FxInt32 phoneme = 0; phoneme < NUM_PHONEMES; ++phoneme )
	{
		FxCombinerNode* phonNode = new FxCombinerNode;
		phonNode->SetName(GetPhonemeNodeName(static_cast<FxPhoneme>(phoneme)));
		_mappingGraph->AddNode(phonNode);
		_phonNodeMap.PushBack(phonNode);
	}
}

// Add a mapping entry to the phoneme to track mapping.
FxBool FxCoarticulationGeneric::AddMappingEntry(FxPhoneme phonEnum, FxName track, FxReal amount)
{
	FxAssert(_mappingGraph);

	// Ensure a node for the track exists, as well as its corresponding delta node.
	FxFaceGraphNode* trackNode = _mappingGraph->FindNode(track);
	if( !trackNode )
	{
		// Create the combiner node and add it to the graph.
		trackNode = new FxCombinerNode;
		trackNode->SetName(track);
		if( amount > 0.f )
		{
			trackNode->SetMax(amount);
		}
		else
		{
			trackNode->SetMin(amount);
			trackNode->SetMax(0.f);
		}
		_mappingGraph->AddNode(trackNode);

		// Create the corresponding delta node and add it to the graph.
		FxDeltaNode* trackDeltaNode = new FxDeltaNode;
		FxString trackDeltaNodeName(FxString::Concat(track.GetAsCstr(), "_Delta"));
		trackDeltaNode->SetName(trackDeltaNodeName.GetData());
		_mappingGraph->AddNode(trackDeltaNode);

		// Link the track combiner node to the track delta node.
		_mappingGraph->Link(trackDeltaNode->GetName(), trackNode->GetName(), "linear", FxLinkFnParameters());

		// Cache the track name.
		_trackNames.PushBack(track);
	}
	else
	{
		// Need to update bounds
		if( amount < trackNode->GetMin() )
		{
			trackNode->SetMin(amount);
		}
		else if( amount > trackNode->GetMax() )
		{
			trackNode->SetMax(amount);
		}
	}

	// Link the phoneme to the track with the correct weight.
	FxLinkFnParameters params;
	params.parameters.PushBack(amount);
	FxLinkErrorCode err = _mappingGraph->Link(trackNode->GetName(), GetPhonemeNodeName(phonEnum), "linear", params);
	return err == LEC_None || err == LEC_Duplicate; // Duplicate links are OK.
}

// End phoneme to track mapping input.
void FxCoarticulationGeneric::EndMapping()
{
	FxAssert(_mappingGraph);

	FxSize numTargets = _trackNames.Length();

	// Clear the curves in preparation for generating new ones.
	_curves.Clear();
	_curves.Reserve(_trackNames.Length());

	_targetNodeMap.Reserve(numTargets);
	_deltaNodeMap.Reserve(numTargets);
	_lastDerivatives.Reserve(numTargets);
	_lastValues.Reserve(numTargets);

	for( FxSize i = 0; i < numTargets; ++i )
	{
		FxAnimCurve curve;
		curve.SetName(_trackNames[i]);
		_curves.PushBack(curve);

		_targetNodeMap.PushBack(_mappingGraph->FindNode(_trackNames[i]));
		FxString trackDeltaNodeName(FxString::Concat(_trackNames[i].GetAsString(), "_Delta"));
		_deltaNodeMap.PushBack(_mappingGraph->FindNode(trackDeltaNodeName.GetData()));

		_lastDerivatives.PushBack(0.0f);
		_lastValues.PushBack(0.0f);
	}

}

// Clear the phoneme list.
void FxCoarticulationGeneric::ClearPhonemeList()
{
	_phonData.Clear();
	_lastPhonEndTime = FxInvalidValue;
}

// Clear the phoneme to track mapping.
void FxCoarticulationGeneric::ClearMapping()
{
	if( _mappingGraph )
	{
		_mappingGraph->Clear();
	}

	_trackNames.Clear();
	_targetNodeMap.Clear();
	_deltaNodeMap.Clear();
	_phonNodeMap.Clear();
}

// Does the coarticulation calculations and phoneme to track mapping.
void FxCoarticulationGeneric::Process()
{
	FxAssert(_mappingGraph);

	// Coarticulate the phoneme list, filling out the per-phoneme curves.
	DoCoarticulation();
	// Merge the per-phoneme curves into per-track curves based on the mapping.
	DoMapping();
}

// Gets the number of curves that were generated.
FxSize FxCoarticulationGeneric::GetNumCurves()
{
	return _curves.Length();
}

// Returns a curve that was generated.
const FxAnimCurve& FxCoarticulationGeneric::GetCurve( FxSize index )
{
	return _curves[index];
}

// Returns the name of a phoneme node.
FxName FxCoarticulationGeneric::GetPhonemeNodeName(FxPhoneme phon)
{
	FxChar tempBuffer[255];
	return FxItoa(phon, tempBuffer);
}

// Do the coarticulation.
void FxCoarticulationGeneric::DoCoarticulation()
{
	// Clear the start time, end time and phoneme data.
	FxSize numPhones = _phonData.Length();
	_startTime = FX_REAL_MAX;
	_endTime = FX_REAL_MIN;

	// The amount that the last phoneme was suppressed.
	// This prevents a suppressed phoneme from over-suppressing it's neighbor.
	FxReal cachedSuppression = 0.0f;

	// The amount the previous phoneme is suppressing the current phoneme.
	FxReal prevSuppression = 0.0f;
	// The amount the next phoneme is suppressing the current phoneme.
	FxReal nextSuppression = 0.0f;

	// For each phoneme in the list, 
	// - Calculate the suppression of the phoneme
	// - Insert the default triplet
	for( FxSize i = 0; i < numPhones; ++i )
	{
		if( i != 0 )
		{
			// Calculate the suppression that the previous phoneme has on the current phoneme.
			prevSuppression = CalculateSuppression(_phonData[i].phoneme, _phonData[i-1].phoneme, GetPhonemeDuration(i-1), cachedSuppression);

			if( _phonData[i-1].phoneme != PHONEME_SIL )
			{
				// If we weren't preceded by a silence, use the preceding phoneme's start time.
				_phonData[i].leadIn.SetTime(_phonData[i-1].startTime);
			}
			else
			{
				if( GetPhonemeDuration(i-1) > _config.shortSilenceDuration || i < 2 )
				{
					// If we have a "longer" silence, ramp in.
					_phonData[i].leadIn.SetTime(_phonData[i].startTime - _config.leadInTime);
				}
				else
				{
					// If we have a "short" silence, blend over it completely.
					_phonData[i].leadIn.SetTime(_phonData[i-2].startTime);
				}
			}
		}
		else
		{
			// If we're the first phoneme in the animation, ramp in.
			_phonData[i].leadIn.SetTime(_phonData[0].startTime - _config.leadInTime);
		}

		if( i != numPhones - 1 )
		{
			// Calculate the suppression that the next phoneme has on the current phoneme.
			// We pass in 0.0f as the prevSuppression parameter since it doesn't apply
			// to calculations dealing with the next phoneme.
			nextSuppression = CalculateSuppression(_phonData[i].phoneme, _phonData[i+1].phoneme, GetPhonemeDuration(i), 0.0f);

			if( _phonData[i+1].phoneme != PHONEME_SIL )
			{
				// If we're not succeeded by a silence, use the phoneme's end time.
				_phonData[i].leadOut.SetTime(GetPhonemeEndTime(i));
			}
			else
			{
				if( GetPhonemeDuration(i+1) > _config.shortSilenceDuration || i > numPhones - 2)
				{
					// If we have a "longer" silence, ramp out.
					_phonData[i].leadOut.SetTime(_phonData[i].startTime + _config.leadOutTime);
				}
				else
				{
					// If we have a "short" silence, blend over it completely.
					_phonData[i].leadOut.SetTime(GetPhonemeEndTime(i+1));
				}
			}
		}
		else
		{
			// If we're the last phoneme in the animation, ramp out.
			_phonData[i].leadOut.SetTime(_lastPhonEndTime + _config.leadOutTime);

			nextSuppression = 0.0f;
		}

		// We want to use the greater of the two suppression values.  The lesser suppression 
		// doesn't impact the mouth shape at all because the target was already suppressed
		// beyond it's influence.
		_phonData[i].suppression = FxMax(prevSuppression,nextSuppression);
		cachedSuppression = _phonData[i].suppression;

		// Set the time of the peak key
		_phonData[i].peak.SetTime(_phonData[i].startTime);
		// Set the value of the peak key
		_phonData[i].peak.SetValue(1.0f - _phonData[i].suppression);

		// Update the animation start and end times
		if( _phonData[i].leadIn.GetTime() < _startTime )
		{
			_startTime = _phonData[i].leadIn.GetTime();
		}
		if( _phonData[i].leadOut.GetTime() > _endTime )
		{
			_endTime = _phonData[i].leadOut.GetTime();
		}
	}

	// For each phoneme in the list, fudge around the neighboring keys according
	// to how much the phoneme was being suppressed.
	for( FxSize i = 0; i < numPhones; ++i )
	{
		if( _phonData[i].phoneme != PHONEME_SIL )
		{
			FxSize prevIndex = i-1;
			FxSize nextIndex = i+1;

			if( _phonData[i].suppression != 0.0f )
			{
				if( i != 0 )
				{
					FxReal duration = GetPhonemeDuration(i);
					FxReal timeChange = _phonData[i].suppression * duration;

					_phonData[prevIndex].leadOut.SetTime(_phonData[prevIndex].leadOut.GetTime() + timeChange);
				}

				if( i != numPhones - 1 )
				{
					FxReal duration = GetPhonemeDuration(i);
					if( i != 0 )
					{
						if( _phonData[prevIndex].phoneme == PHONEME_SIL && 
							GetPhonemeDuration(prevIndex) > _config.shortSilenceDuration )

						{
							duration += _config.leadInTime;
						}
						else
						{
							duration += GetPhonemeDuration(prevIndex);
						}
					}
					FxReal timeChange = _phonData[i].suppression * duration;

					_phonData[nextIndex].leadIn.SetTime(_phonData[nextIndex].leadIn.GetTime() - timeChange);
				}

			}
		}
	}
}

// Do the phoneme to track mapping.
void FxCoarticulationGeneric::DoMapping()
{
	FxSize numTargets = _trackNames.Length();

	// Initialize the arrays.
	for( FxSize i = 0; i < numTargets; ++i )
	{
		_lastDerivatives[i] = 0.0f;
		_lastValues[i] = 0.0f;
		_curves[i].RemoveAllKeys();
	}

	// Pad the start and end times so we don't miss anything important.
	FxReal timeStep = 0.0166666667f; // 60fps
	FxReal startTime = _startTime - (timeStep * 3);
	FxReal endTime = _endTime + (timeStep * 3);
	for( FxReal time = startTime; time < endTime; time += timeStep )
	{
		// Evaluate the phoneme curves at the time.
		_mappingGraph->ClearValues();
		for( FxSize i = 0; i < _phonData.Length(); ++i )
		{
			FxReal value = EvaluateCurve(
				_phonData[i].leadIn,
				_phonData[i].peak,
				_phonData[i].leadOut,
				time);
			if( value != 0.0f )
			{
				// Insert the value into the curves.
				FxFaceGraphNode* node = _phonNodeMap[_phonData[i].phoneme];
				node->SetTrackValue( node->GetTrackValue() == FxInvalidValue ? 
					value : node->GetTrackValue() + value );
			}
		}

		// Go through each track and get its value and derivative.
		for( FxSize i = 0; i < numTargets; ++i )
		{
			FxReal derivative = _deltaNodeMap[i]->GetValue();
			if( FxAbs(derivative) < FX_REAL_EPSILON )
			{
				derivative = 0.0f;
			}
			FxReal value = _targetNodeMap[i]->GetValue();

			FxBool lastDerivEqualsZero = FxRealEquality(_lastDerivatives[i], 0.0f);
			FxBool thisDerivEqualsZero = FxRealEquality(derivative, 0.0f);
				// Insert a key if we had a zero derivative and now we don't
			if( (lastDerivEqualsZero && !thisDerivEqualsZero) ||
				// Or, insert a key if we had a non-zero derivative and now we have a zero derivative
				(!lastDerivEqualsZero && thisDerivEqualsZero) ||
				// Or, insert a key if we had a positive derivative and now we have negative
				(_lastDerivatives[i] > 0.0f && derivative < 0.0f) || 
				// Or, insert a key if we had a negative derivative and now we have positive
				(_lastDerivatives[i] < 0.0f && derivative > 0.0f) )
			{
				// Use the last value to make sure if we've started up from zero
				// that the keys are actually on zero.
				_curves[i].InsertKey(FxAnimKey(time-(timeStep*0.5f), _lastValues[i]));
			}

			_lastValues[i] = value;
			_lastDerivatives[i] = derivative;
		}
	}
}

// Calculates the suppression of a phoneme pair.
FxReal FxCoarticulationGeneric::CalculateSuppression( FxPhoneme currPhoneme, FxPhoneme otherPhoneme, FxReal phonemeDuration, FxReal cachedSuppression )
{
	// Calculate the dominance ratio between the phonemes.  A value of 0.0f
	// indicates that currPhoneme is very dominant.
	FxReal dominanceRatio = (1.0f - _config.phoneInfo[currPhoneme].dominance) * _config.phoneInfo[otherPhoneme].dominance;

	// Any phoneme lengths greater than this value will not result in any suppression.
	FxReal zeroSuppressionCutoff = _config.suppressionWindowMax + dominanceRatio * _config.suppressionWindowMaxSlope;
	// The length at which the phoneme will be fully suppressed.
	FxReal fullSuppressionCutoff = _config.suppressionWindowMin + dominanceRatio * _config.suppressionWindowMinSlope;

	// Clamp the separation so it is between zeroSuppressionCutoff and fullSuppressionCutoff.
	FxReal separation = FxClamp(fullSuppressionCutoff, phonemeDuration, zeroSuppressionCutoff);

	// Calculate the suppression.
	FxReal suppression = (zeroSuppressionCutoff - separation) / (zeroSuppressionCutoff - fullSuppressionCutoff);
	// Take into account that the previous phoneme may have been suppressed and 
	// therefore does not suppress the current phoneme to the same extent.
	return suppression - (cachedSuppression * suppression);
}

// Calculates the duration of a phoneme
FxReal FxCoarticulationGeneric::GetPhonemeDuration(FxSize i)
{
	if( i < _phonData.Length() - 1 )
	{
		return _phonData[i+1].startTime - _phonData[i].startTime;
	}
	else
	{
		return _lastPhonEndTime - _phonData.Back().startTime;
	}
}

// Calculates the end time of a phoneme.
FxReal FxCoarticulationGeneric::GetPhonemeEndTime(FxSize i)
{
	if( i < _phonData.Length() - 1 )
	{
		return _phonData[i+1].startTime;
	}
	else
	{
		return _lastPhonEndTime;
	}
}

// Performs Hermite evaluation on three keys
FxReal FxCoarticulationGeneric::EvaluateCurve( const FxAnimKey& leadIn, 
											   const FxAnimKey& peak, 
											   const FxAnimKey& leadOut, 
											   FxReal time )
{
	if( time > leadIn.GetTime() && time < leadOut.GetTime() )
	{
		if( time < peak.GetTime() )
		{
			return FxHermiteInterpolate(leadIn, peak, time);
		}
		else
		{
			return FxHermiteInterpolate(peak, leadOut, time);
		}
	}

	return 0.0f;
}

} // namespace Face

} // namespace OC3Ent
