//------------------------------------------------------------------------------
// This class implements a FaceFx actor instance.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#ifndef FxActorInstance_H__
#define FxActorInstance_H__

#include "FxPlatform.h"
#include "FxArchive.h"
#include "FxArray.h"
#include "FxNamedObject.h"
#include "FxActor.h"
#include "FxFaceGraphNode.h"
#include "FxDeltaNode.h"
#include "FxAnimPlayer.h"

namespace OC3Ent
{

namespace Face
{

/// A FaceFX actor instance.
/// \note Due to the way the FaceFX actor instancing system works, an actor
/// instance should always be destroyed before it's actor.
class FxActorInstance : public FxNamedObject
{
	/// Declare the class.
	FX_DECLARE_CLASS_NO_DYNCREATE_NO_SERIALIZE(FxActorInstance, FxNamedObject)
public:
	/// FxActors are our friends.
	friend class FxActor;

	/// Construct the instance and set the actor.
	FxActorInstance( FxActor* actor = NULL );
	/// Destructor.
	virtual ~FxActorInstance();

	/// Sets the actor this instance refers to.  Only call this once.  If you
	/// have already called SetActor() with a valid actor and you try to call
	/// it again, it will assert in debug mode and do nothing in release mode.
	void SetActor( FxActor* actor );
	/// Returns the actor this instance refers to.
	FX_INLINE FxActor* GetActor( void ) { return _actor; }

	/// Sets whether or not Tick() does anything if there is no currently 
	/// playing animation.  This is useful for procedural %Face Graph effects,
	/// especially those based on time.
	/// \param allowNonAnimTick Set to \p FxTrue to allow ticks regardless of the state
	/// of the animation playback, \p FxFalse to only tick when there is an animation
	/// being played.
	void SetAllowNonAnimTick( FxBool allowNonAnimTick );
	/// Returns whether or not Tick() does anything if there is no currently 
	/// playing animation.  This is useful for procedural %Face Graph effects,
	/// especially those based on time.
	FX_INLINE FxBool GetAllowNonAnimTick( void ) const { return _allowNonAnimTick; }

	/// Begins a frame for this actor instance.
	/// Updates all node values in the %Face Graph with the result of their register
	/// value operations only if their registers have been set via client code calls to
	/// SetRegister().  This should only be called after FxActorInstance::Tick().
	virtual void BeginFrame( void );
	/// Ends a frame for this actor instance.
	/// Updates all register values to the node final values in the %Face Graph.  The
	/// values will be valid and accessible until the next call to BeginFrame().
	/// This should only be called after FxActorInstance::Tick() and FxActorInstance::BeginFrame().
	virtual void EndFrame( void );

	/// Updates all bone pose nodes in the %Face Graph.  This must be called
	/// after FxActorInstance::Tick() and FxActorInstance::BeginFrame() but before
	/// any calls to FxActorInstance::GetBone().
	void UpdateBonePoses( void );
	/// Returns the number of bones controlled by the actor instance.
	FxSize GetNumBones( void ) const;
	/// Returns the bone's index in the client animation system or FX_INT32_MAX
	/// if the bone's index in the client animation system was not set.
	/// \param index The index of the bone in the FxMasterBoneList.
	FxInt32 GetBoneClientIndex( FxSize index ) const;
	/// Returns the bone at index.
	/// \param index The index of the bone to blend in the FxMasterBoneList.
	/// \param bonePos The bone transformation's position component.
	/// \param boneRot The bone transformation's rotation component.
	/// \param boneScale The bone transformation's scale component.
	/// \param boneWeight The current weight of the bone.
	/// \note This call returns valid results only after Tick() and BeginFrame()
	/// and before EndFrame().
	void GetBone( FxSize index, FxVec3& bonePos, FxQuat& boneRot, 
		          FxVec3& boneScale, FxReal& boneWeight ) const;

	/// Ticks the actor instance and the actor instance's animation player
	/// (if the animation player is currently playing an animation).
	/// \param appTime The time (in seconds) as reckoned by the entire 
	/// application (should be ever increasing).
	/// \param audioTime The current playback time in the audio (in seconds) to 
	/// which the animation is synchronized.
	/// If \a audioTime is less than zero while the animation is in the 
	/// \p APS_Playing state, the animation is assumed to have no corresponding 
	/// audio, and thus \a appTime is used to calculate the offset into 
	/// the animation.  Tick() should be called each frame before the 
	/// FxActorInstance::BeginFrame() and FxActorInstance::EndFrame() pair.
	/// \return The current state of the animation.  This value should be
	/// monitored each frame for equality to \p APS_StartAudio, which indicates that
	/// the audio corresponding to the animation should be started.
	FxAnimPlayerState Tick( const FxDReal appTime, const FxReal audioTime );
	/// Forces a tick on the actor instance and the actor instance's animation 
	/// player with the specified animation at the specified time in the
	/// animation.
	/// \param animName The name of the animation to force tick.
	/// \param groupName The group containing the animation to force tick.
	/// \param forcedAnimationTime The time (in seconds) with which to force
	/// tick the animation.  This time should be between the animation's start
	/// and end times.
	/// ForceTick() should be called each frame before the 
	/// FxActorInstance::BeginFrame() and FxActorInstance::EndFrame() pair.
	void ForceTick( const FxName& animName, const FxName& groupName, 
		            const FxReal forcedAnimationTime );
	/// Returns the animation player for the actor instance.
	FX_INLINE FxAnimPlayer& GetAnimPlayer( void ) { return _animPlayer; }
	
	/// Sets a register to a specific value.
	/// The operation specified by \a regOp will
	/// be used to put the register value into the %Face Graph.
	/// Registers are updated during BeginFrame().
	/// \param regName The name of the register.  This must be the name of a %Face Graph node.
	/// \param newValue The desired value of the register.  If newValue is FxInvalidValue the
	/// register is temporarily shut off until the next call to SetRegister(), SetRegisterEx(),
	/// or SetAllRegisters().
	/// \param regOp The operation to perform with \a newValue and the value of the node.
	/// \param interpDuration The time (in seconds) of the interpolation to \a newValue.
	/// \return \p FxTrue if the operation succeeded, or \p FxFalse if the register was not
	/// found and could not be created.
	/// \note SetRegister() must be called outside of a BeginFrame() / EndFrame() pair.
	/// If \a interpDuration is 0.0f, the register value is immediately set to \a newValue.
	/// Otherwise, Hermite interpolation is performed from the current value to \a newValue
	/// over \a interpDuration seconds.
	FxBool SetRegister( const FxName& regName, FxReal newValue, 
						FxValueOp regOp, FxReal interpDuration = 0.0f );
	/// Sets a register to a specific value.
	/// The operation specified by \a regOp will
	/// be used to put the register value into the %Face Graph.
	/// Registers are updated during BeginFrame().
	/// \param regName The name of the register.  This must be the name of a %Face Graph node.
	/// \param regOp The operation to perform with \a newValue and the value of the node.
	/// \param firstValue The desired first value of the register.  If firstValue is FxInvalidValue
	/// the register is temporarily shut off until the next call to SetRegister(), SetRegisterEx(),
	/// or SetAllRegisters().
	/// \param firstInterpDuration The time (in seconds) of the interpolation to \a firstValue.
	/// \param nextValue The desired next value of the register.
	/// \param nextInterpDuration The time (in seconds) of the interpolation to \a nextValue.
	/// \return \p FxTrue if the operation succeeded, or \p FxFalse if the register was not
	/// found and could not be created.
	/// \note SetRegisterEx() must be called outside of a BeginFrame() / EndFrame() pair.
	/// If \a firstInterpDuration is 0.0f, the register value is immediately set to \a firstValue.
	/// Otherwise, Hermite interpolation is performed from the current value to \a firstValue
	/// over \a firstInterpDuration seconds after which Hermite interpolation is performed from
	/// that value to \a nextValue over \a nextInterpDuration seconds.
	FxBool SetRegisterEx( const FxName& regName, FxValueOp regOp, 
		                  FxReal firstValue, FxReal firstInterpDuration, 
						  FxReal nextValue, FxReal nextInterpDuration );
	/// Sets all registers to a specific value.
	/// The operation specified by \a regOp will be used to put all register 
	/// values into the %Face Graph.
	/// Registers are updated during BeginFrame().
	/// \param regOp The operation to perform with \a newValue and the value of the nodes.
	/// \param newValue The desired value of all registers.  If newValue is FxInvalidValue all
	/// registers are temporarily shut off until the next call to SetRegister(), SetRegisterEx(),
	/// or SetAllRegisters().
	/// \param interpDuration The time (in seconds) of the interpolation to \a newValue.
	/// \note SetAllRestiers() must be called outside of a BeginFrame() / EndFrame() pair.
	/// If \a interpDuration is 0.0f, the register values are immediately set to \a newValue.
	/// Otherwise, Hermite interpolation is performed from the current values to \a newValue
	/// over \a interpDuration seconds.
	void SetAllRegisters( FxValueOp regOp, FxReal newValue, FxReal interpDuration = 0.0f );
	/// Returns the value of a register.
	/// \param regName The name of the register.  This must be the name of a %Face Graph node.
	/// \return The value of the register, or 0.0f if the register was not found and
	/// could not be created.
	/// \note GetRegister() should only be called outside of a FxActorInstance::BeginFrame() and 
	/// FxActorInstance::EndFrame() pair.
	FxReal GetRegister( const FxName& regName );

protected:
	/// A specialized structure for caching all FxDeltaNode values on a 
	/// per-FxActorInstance basis across %Face Graph evaluations.
	class FxActorInstanceDeltaNodeCacheEntry
	{
	public:
		/// Constructor.
		FxActorInstanceDeltaNodeCacheEntry( FxDeltaNode* deltaNode = NULL )
			: pDeltaNode(deltaNode)
			, deltaNodePreviousFirstInputValue(FxInvalidValue)
		{
		}
		/// Destructor.
		~FxActorInstanceDeltaNodeCacheEntry()
		{
		}
		/// The FxDeltaNode contained in the cache entry.
		FxDeltaNode* pDeltaNode;
		/// The cached value of the first input link to the delta node the last 
		/// time the delta node was evaluated.
		FxReal deltaNodePreviousFirstInputValue;
	};

	/// A register is for interacting with an actor instance's %Face Graph.
	/// A register is used for setting and retrieving results from 
	/// FxFaceGraphNode objects contained in the FxActor this instance
	/// refers to.
	class FxActorInstanceRegister
	{
	public:
		/// Constructor.
		FxActorInstanceRegister()
			: pBinding(NULL)
			, value(0.0f)
			, interpStartValue(0.0f)
			, interpEndValue(0.0f)
			, interpNextEndValue(0.0f)
			, interpLastValue(0.0f)
			, interpInverseDuration(0.0f)
			, interpNextInverseDuration(0.0f)
			, interpStartTime(FxInvalidValue)
			, regOp(VO_Add)
		{
		}
		/// Constructor.
		FxActorInstanceRegister( FxFaceGraphNode* registerDef )
			: pBinding(registerDef)
			, value(0.0f)
			, interpStartValue(0.0f)
			, interpEndValue(0.0f)
			, interpNextEndValue(0.0f)
			, interpLastValue(0.0f)
			, interpInverseDuration(0.0f)
			, interpNextInverseDuration(0.0f)
			, interpStartTime(FxInvalidValue)
			, regOp(VO_Add)
		{
		}
		/// Destructor.
		~FxActorInstanceRegister()
		{
		}
		/// The FxFaceGraphNode this register is bound to.
		FxFaceGraphNode* pBinding;
		/// The value of the register.
		FxReal value;
		/// The start value of the interpolation.
		FxReal interpStartValue;
		/// The end value of the interpolation.
		FxReal interpEndValue;
		/// The end value of the next interpolation.
		FxReal interpNextEndValue;
		/// The last known interpolation value.
		FxReal interpLastValue;
		/// The inverse duration of the interpolation.
		FxReal interpInverseDuration;
		/// The inverse duration of the next interpolation.
		FxReal interpNextInverseDuration;
		/// The start time of the interpolation.
		FxReal interpStartTime;
		/// The register operation to perform when setting the value of the
		/// register binding.
		FxValueOp regOp;
	};

	/// The actor this instance refers to.
	FxActor* _actor;
	
	/// Determines whether or not calling Tick() does anything if there is no
	/// currently playing animation.  This is useful for procedural %Face Graph 
	/// effects, especially those based on time.  Defaults to \p FxFalse.
	FxBool _allowNonAnimTick;

	/// The animation player.
	FxAnimPlayer _animPlayer;

	/// The FxDeltaNode cache.
	FxArray<FxActorInstanceDeltaNodeCacheEntry> _deltaNodeCache;

	/// Registers.
	FxArray<FxActorInstanceRegister> _registers;

	/// Adds a register to the actor instance.
	FxBool _addRegister( FxFaceGraphNode* registerDef );
	/// Removes a register from the actor instance.
	FxBool _removeRegister( FxFaceGraphNode* registerDef );
};

FX_INLINE void FxActorInstance::UpdateBonePoses( void )
{
	FxAssert(_actor != NULL);
	_actor->_masterBoneList.UpdateBonePoses(_actor->_faceGraph);
}

FX_INLINE FxSize FxActorInstance::GetNumBones( void ) const
{
	FxAssert(_actor != NULL);
	return _actor->_masterBoneList.GetNumRefBones();
}

FX_INLINE FxInt32 FxActorInstance::GetBoneClientIndex( FxSize index ) const
{
	FxAssert(_actor != NULL);
	return _actor->_masterBoneList.GetRefBoneClientIndex(index);
}

FX_INLINE void FxActorInstance::GetBone( FxSize index, FxVec3& bonePos, 
							             FxQuat& boneRot, FxVec3& boneScale, 
							             FxReal& boneWeight ) const
{
	FxAssert(_actor != NULL);
	return _actor->_masterBoneList.GetBlendedBone(index, bonePos, boneRot, boneScale, boneWeight);
}

FX_INLINE FxBool FxActorInstance::SetRegister( const FxName& regName, 
									           FxReal newValue, FxValueOp regOp, 
									           FxReal interpDuration )
{
	return SetRegisterEx(regName, regOp, newValue, interpDuration, FxInvalidValue, 0.0f);
}

} // namespace Face

} // namespace OC3Ent

#endif
