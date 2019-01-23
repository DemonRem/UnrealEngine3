//------------------------------------------------------------------------------
// An animation player.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#ifndef FxAnimPlayer_H__
#define FxAnimPlayer_H__

#include "FxPlatform.h"
#include "FxActor.h"
#include "FxAnim.h"

namespace OC3Ent
{

namespace Face
{

/// The various states of the animation player.
enum FxAnimPlayerState
{
	APS_BlendingIn,	 ///< The animation is blending in.
	APS_StartAudio,	 ///< The audio should be started at this time.
	APS_Playing,	 ///< The animation is playing.
	APS_BlendingOut, ///< The animation is blending out.
	APS_Stopped,     ///< The animation has finished (including blending out).
};

/// An animation player.
/// Each FxActorInstance has its own animation player.
class FxAnimPlayer
{
public:
	/// Constructor.
	FxAnimPlayer( FxActor* actor = NULL );
	/// Destructor.
	virtual ~FxAnimPlayer();

	/// Connects the animation player to an actor.
	void SetActor( FxActor* actor );
	/// Returns the actor this animation player is connected to.
	FX_INLINE FxActor* GetActor( void ) { return _actor; }
	
	/// Ticks the animation player causing the animation state to be updated.
	/// \param appTime The time (in seconds) as reckoned by the entire 
	/// application (should be ever increasing)
	/// \param audioTime The time in the audio (in seconds) to which the 
	/// animation is synchronized.
	/// If \a audioTime is less than zero while the animation is in the 
	/// \p APS_Playing state, the animation is assumed to have no corresponding 
	/// audio, and thus \a appTime is used to calculate the offset into 
	/// the animation. 
	/// \return The current state of the animation.  This value should be
	/// monitored each frame for equality to \p APS_StartAudio, which indicates that
	/// the audio corresponding to the animation should be started.  Note that
	/// this will only return APS_Stopped if the animation is ticked to a time
	/// that is beyond the end of the animation data AND audioTime is less than
	/// zero.  Therefore code passing in a valid audioTime should be constructed
	/// carefully such that when the audio actually stops audioTime is correctly
	/// passed in as less than zero.
	FxAnimPlayerState Tick( const FxDReal appTime, const FxReal audioTime );

	/// Ticks the animation causing the animation state to be updated.
	/// This version of the function is intended for tool usage, when you know
	/// exactly what frame to display and do not need the additional services
	/// provided by the other version of Tick().  Note that appTime is \b not used
	/// here and therefore current time nodes in the %Face Graph will \b not be 
	/// updated when calling this version of Tick().
	/// \param animationTime The time (in seconds) of the desired frame of the animation.
	void Tick( const FxReal animationTime );

	/// Plays the animation 'animName' in the animation group 'groupName'.
	/// \param animName The name of the animation to play.
	/// \param groupName The name of the group in which the animation resides.
	/// \param ignoreNegativeTime If \p FxTrue, the player will skip the part of
	/// the animation before 0.0s.  This is only useful in certain circumstances,
	/// where the animation begins almost immediately at the start of the audio,
	/// and the ramp-in is undesired because of synching issues with in-game
	/// events.  It defaults to \p FxFalse, meaning the negative time will be 
	/// treated normally, and played through before sending the signal to start
	/// the audio.
	/// \return \p FxTrue if the animation started playing, \p FxFalse otherwise.
	FxBool Play( const FxName& animName, 
				 const FxName& groupName = FxAnimGroup::Default,
				 FxBool ignoreNegativeTime = FxFalse );
	/// Stops playing an animation.
	void Stop( void );

	/// Returns whether or not blending in and blending out operations 
	/// should occur.  Note that blending into and out of animations is not
	/// supported as of 1.1.
	FX_INLINE FxBool GetWantBlendInAndOut( void ) const { return _wantBlendInAndOut; }
	/// Sets whether or not blending in and blending out operations 
	/// should occur.  Note that blending into and out of animations is not
	/// supported as of 1.1.
	/// \param wantBlendInAndOut \p FxTrue if blending should be performed, 
	/// \p FxFalse otherwise.
	void SetWantBlendInAndOut( FxBool wantBlendInAndOut );

	/// Returns the state of the animation player.
	FX_INLINE FxAnimPlayerState GetState( void ) const { return _state; }
	
	/// Returns \p FxTrue if the animation player is currently playing, \p FxFalse
	/// otherwise.
	FX_INLINE FxBool IsPlaying( void ) const { return _state != APS_Stopped; }
	/// Returns the current time in the animation player.  If no animation is
	/// currently playing, -1.0f is returned.
	FX_INLINE FxReal GetCurrentAnimTime( void ) const { return _currentTime; }
	
	/// Returns the name of the animation group that contains the currently
	/// playing animation or FxName::NullName if nothing is playing.
	FX_INLINE const FxName& GetCurrentAnimGroupName( void ) const { return _currentAnimGroupName; }
	/// Returns the name of the currently playing animation or FxName::NulName
	/// if nothing is playing.
	FX_INLINE const FxName& GetCurrentAnimName( void ) const { return _currentAnimName; }
	/// Returns a pointer to the currently playing animation or \p NULL if none.
	FX_INLINE const FxAnim* GetCurrentAnim( void ) const { return _currentAnim; }

private:
	/// The actor associated with the player.
	FxActor* _actor;
	/// The name of the animation group that contains the currently playing
	/// animation or FxName::NullName if nothing is playing.
	FxName _currentAnimGroupName;
	/// The name of the currently playing animation or FxName::NullName if 
	/// nothing is playing.
	FxName _currentAnimName;
	/// The animation loaded in the player.
	FxAnim* _currentAnim;

	/// The start time of the animation in app time.
	/// (Used for soundless animations)
	FxDReal _startTime;
	/// The start time of the blend in operation.
	FxDReal _blendInStartTime;
	/// The start time of the blend out operation.
	FxDReal _blendOutStartTime;
	/// The current time of the animation player.
	FxReal _currentTime;
	/// The lead-in duration of the animation
	FxReal _leadinDuration;
	/// Whether or not we've triggered an audio start for this animation.
	FxBool _hasStartedAudio;
	/// Determines whether or not blending in and blending out operations 
	/// should occur.
	FxBool	_wantBlendInAndOut;
	/// Whether to ignore the negative time in an animation.
	FxBool _ignoreNegativeTime;
	/// The current state of this instance.
	FxAnimPlayerState _state;
};

} // namespace Face

} // namespace OC3Ent

#endif
