//------------------------------------------------------------------------------
// An animation player.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "FxAnimPlayer.h"
#include "FxUtil.h"
#include "FxMath.h"

namespace OC3Ent
{

namespace Face
{

FxAnimPlayer::FxAnimPlayer( FxActor* actor )
	: _actor(actor)
	, _currentAnim(NULL)
	, _startTime(FxInvalidValue)
	, _blendInStartTime(FxInvalidValue)
	, _blendOutStartTime(FxInvalidValue)
	, _currentTime(FxInvalidValue)
	, _leadinDuration(FxInvalidValue)
	, _hasStartedAudio(FxFalse)
	, _wantBlendInAndOut(FxFalse)
	, _ignoreNegativeTime(FxFalse)
	, _state(APS_Stopped)
{
}

FxAnimPlayer::~FxAnimPlayer()
{
}

void FxAnimPlayer::SetActor( FxActor* actor )
{
	_actor = actor;
}

FxAnimPlayerState 
FxAnimPlayer::Tick( const FxDReal appTime, const FxReal audioTime )
{
	if( _actor && _currentAnim )
	{
		// Clear the face graph.
		FxFaceGraph& faceGraph = _actor->GetFaceGraph();
		faceGraph.ClearValues();
		// Reset the reference bone weights.
		FxMasterBoneList& masterBoneList = _actor->GetMasterBoneList();
		masterBoneList.ResetRefBoneWeights();
		// Set the reference bone weights for the current animation.
		FxSize numBoneWeights = _currentAnim->GetNumBoneWeights();
		for( FxSize i = 0; i < numBoneWeights; ++i )
		{
			const FxAnimBoneWeight& boneWeight = _currentAnim->GetBoneWeight(i);
			masterBoneList.SetRefBoneCurrentWeight(boneWeight.boneName, boneWeight.boneWeight);
		}
		// Set the current time in the face graph.
		faceGraph.SetTime(static_cast<FxReal>(appTime));
		
		if( _state == APS_Playing )
		{
			// Start the animation if it hasn't started yet.
			if( FxDRealEquality(_startTime, static_cast<FxDReal>(FxInvalidValue)) )
			{
				// Check to see if we have negative time to play through.
				FxReal animStartTime = _currentAnim->GetStartTime();
				if( animStartTime < 0.0f && !_ignoreNegativeTime )
				{
					_leadinDuration = FxAbs(animStartTime);
				}
				else
				{
					_leadinDuration = 0.0f;
				}
				_startTime = appTime;
			}

			// See if there is audio playing (if not, audioTime should be less than 0.0)
			// and update _currentTime accordingly.
			if( audioTime < 0.0f )
			{
				_currentTime = static_cast<FxReal>(appTime - _startTime) - _leadinDuration;
			}
			else
			{
				_currentTime = audioTime;
				// Keep appTime and audioTime from diverging.  This prevents situations
				// where appTime is slightly behind audioTime, so when the audio playback ends,
				// we don't go backwards in time when picking up at appTime where audioTime 
				// left off.
				_startTime = appTime - audioTime - _leadinDuration;
			}

			// Tick the animation.
			if( !_currentAnim->Tick(_currentTime) && audioTime < 0.0f )
			{
				// The animation is over.  If we want blend in and out operations, set _state to
				// APS_BlendingOut.
				if( _wantBlendInAndOut )
				{
					_state = APS_BlendingOut;
				}
				else
				{
					// Otherwise, we're stopped.
					Stop();
				}
			}

			// If we've reached the time when we should start the audio,
			// tell the client and quit early.
			if( !_hasStartedAudio && _currentTime >= 0.0f )
			{
				_hasStartedAudio = FxTrue;
				return APS_StartAudio;
			}
		}
		else if( _state == APS_StartAudio )
		{
			// If not, set _state to the next valid state.
			_state = APS_Playing;

			// If we don't want blending operations, the first state is APS_StartAudio.
			// We need to make sure that this is the state passed back from this function
			// so that the application knows to start the audio.
			if( !_wantBlendInAndOut )
			{
				_hasStartedAudio = FxTrue;
				return APS_StartAudio;
			}
		}
		else if( _state == APS_Stopped )
		{
			// Nothing to do here for now.
		}
		else if( _state == APS_BlendingIn )
		{
			//@todo Blending.
			_state = APS_Playing;
		}
		else if( _state == APS_BlendingOut )
		{
			//@todo Blending.
			_state = APS_Stopped;
		}
		else
		{
			// Should never get here.
			FxAssert( !"Unknown state in FxAnimPlayer::Tick()!" );
		}
	}

	// Return the current state of this instance.
	return _state;
}

void FxAnimPlayer::Tick( const FxReal animationTime )
{
	if( _actor && _currentAnim )
	{
		// Clear the face graph.
		_actor->GetFaceGraph().ClearValues();
		// Reset the reference bone weights.
		FxMasterBoneList& masterBoneList = _actor->GetMasterBoneList();
		masterBoneList.ResetRefBoneWeights();
		// Set the reference bone weights for the current animation.
		FxSize numBoneWeights = _currentAnim->GetNumBoneWeights();
		for( FxSize i = 0; i < numBoneWeights; ++i )
		{
			const FxAnimBoneWeight& boneWeight = _currentAnim->GetBoneWeight(i);
			masterBoneList.SetRefBoneCurrentWeight(boneWeight.boneName, boneWeight.boneWeight);
		}
		// Tick the animation to the specified time.
		_currentAnim->Tick(animationTime);
	}
}

FxBool FxAnimPlayer::Play( const FxName& animName, 
			               const FxName& groupName,
						   FxBool ignoreNegativeTime )
{
	if( _actor )
	{
		Stop();
		_currentAnim = _actor->GetAnimPtr(groupName, animName);
		if( _currentAnim )
		{
			// Check if the animation has been linked.  If it hasn't, do so.
			if( !_currentAnim->IsLinked() )
			{
				_currentAnim->Link(_actor->GetFaceGraph());
			}

			// Set the initial state.
			_currentAnimGroupName = groupName;
			_currentAnimName      = animName;
			_ignoreNegativeTime   = ignoreNegativeTime;
			if( _wantBlendInAndOut )
			{
				_state = APS_BlendingIn;
			}
			else
			{
				_state = APS_Playing;
			}
			return FxTrue;
		}
	}
	return FxFalse;
}

void FxAnimPlayer::Stop( void )
{
	if( _actor )
	{
		// Set the timing variables to some known state.
		_currentAnimGroupName = FxName::NullName;
		_currentAnimName      = FxName::NullName;
		_currentAnim          = NULL;
		_startTime            = FxInvalidValue;
		_blendInStartTime     = FxInvalidValue;
		_blendOutStartTime    = FxInvalidValue;
		_currentTime          = FxInvalidValue;
		_leadinDuration       = FxInvalidValue;
		_hasStartedAudio      = FxFalse;
		_ignoreNegativeTime   = FxFalse;
		_state	              = APS_Stopped;
	}
}

void FxAnimPlayer::SetWantBlendInAndOut( FxBool wantBlendInAndOut )
{
	_wantBlendInAndOut = wantBlendInAndOut;
}

} // namespace Face

} // namespace OC3Ent
