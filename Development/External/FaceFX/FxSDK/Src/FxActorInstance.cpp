//------------------------------------------------------------------------------
// This class implements a FaceFx actor instance.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "FxActorInstance.h"
#include "FxUtil.h"
#include "FxMath.h"

namespace OC3Ent
{

namespace Face
{

#define kCurrentFxActorInstanceVersion 0

FX_IMPLEMENT_CLASS(FxActorInstance, kCurrentFxActorInstanceVersion, FxNamedObject)

FxActorInstance::FxActorInstance( FxActor* actor )
	: _actor(NULL)
	, _allowNonAnimTick(FxFalse)
{
	SetActor(actor);
}

FxActorInstance::~FxActorInstance()
{
	// Find this instance in the actor's instance list and remove it.
	if( _actor )
	{
		FxList<FxActorInstance*>::Iterator curr = _actor->_instanceList.Begin();
		FxList<FxActorInstance*>::Iterator end  = _actor->_instanceList.End();
		for( ; curr != end; ++curr )
		{
			if( (*curr) == this )
			{
				_actor->_instanceList.Remove(curr);
				break;
			}
		}
	}
}

void FxActorInstance::SetActor( FxActor* actor )
{
	FxAssert(_actor == NULL);
	if( !_actor && actor )
	{
		_actor = actor;
		_deltaNodeCache.Clear();
		_registers.Clear();
		_animPlayer.SetActor(_actor);
		// Add this instance to the instance list of the actor.
		_actor->_instanceList.PushBack(this);
		// Set up the FxDeltaNode cache.
		FxFaceGraph& faceGraph = _actor->GetFaceGraph();
		_deltaNodeCache.Reserve(faceGraph.CountNodes(FxDeltaNode::StaticClass()));
		FxFaceGraph::Iterator deltaNodeIter    = faceGraph.Begin(FxDeltaNode::StaticClass());
		FxFaceGraph::Iterator deltaNodeEndIter = faceGraph.End(FxDeltaNode::StaticClass());
		for( ; deltaNodeIter != deltaNodeEndIter; ++deltaNodeIter )
		{
			FxDeltaNode* pDeltaNode = FxCast<FxDeltaNode>(deltaNodeIter.GetNode());
			if( pDeltaNode )
			{
				_deltaNodeCache.PushBack(FxActorInstanceDeltaNodeCacheEntry(pDeltaNode));
			}
		}
		// Add all the registers.
		FxSize numRegisterDefs = _actor->_registerDefs.Length();
		_registers.Reserve(numRegisterDefs);
		for( FxSize i = 0; i < numRegisterDefs; ++i )
		{
			_addRegister(_actor->_registerDefs[i]);
		}
	}
}

void FxActorInstance::SetAllowNonAnimTick( FxBool allowNonAnimTick )
{
	_allowNonAnimTick = allowNonAnimTick;
}

void FxActorInstance::BeginFrame( void )
{	
	// Update all of the FxDeltaNodes.
	FxSize numDeltaNodesInCache = _deltaNodeCache.Length();
	for( FxSize i = 0; i < numDeltaNodesInCache; ++i )
	{
		FxActorInstanceDeltaNodeCacheEntry& cacheEntry  = _deltaNodeCache[i];
		cacheEntry.pDeltaNode->_previousFirstInputValue = cacheEntry.deltaNodePreviousFirstInputValue;
	}
	// Update all nodes with register values set by client code.
	FxSize numRegisters = _registers.Length();
	for( FxSize i = 0; i < numRegisters; ++i )
	{
		FxAssert(_registers[i].pBinding);
		_registers[i].pBinding->SetUserValue(_registers[i].interpLastValue,
										     _registers[i].regOp);
	}
}

void FxActorInstance::EndFrame( void )
{
	// Cache all of the FxDeltaNodes.
	FxSize numDeltaNodesInCache = _deltaNodeCache.Length();
	for( FxSize i = 0; i < numDeltaNodesInCache; ++i )
	{
		FxActorInstanceDeltaNodeCacheEntry& cacheEntry = _deltaNodeCache[i];
		cacheEntry.deltaNodePreviousFirstInputValue    = cacheEntry.pDeltaNode->_previousFirstInputValue;
	}
	// Set all register values to the values of their bindings.
	FxSize numRegisters = _registers.Length();
	for( FxSize i = 0; i < numRegisters; ++i )
	{
		FxAssert(_registers[i].pBinding);
		_registers[i].value = _registers[i].pBinding->GetValue();
	}
}

FxAnimPlayerState 
FxActorInstance::Tick( const FxDReal appTime, const FxReal audioTime )
{
	FxAssert(_actor != NULL);
	FxAnimPlayerState currentState = APS_Stopped;
	FxReal appTimeReal = static_cast<FxReal>(appTime);
	FxSize numRegisters = _registers.Length();
	for( FxSize i = 0; i < numRegisters; ++i )
	{
		// Only do register processing if the register is not in the shut off
		// state.
		if( FxInvalidValue != _registers[i].interpEndValue )
		{
			if( FxInvalidValue == _registers[i].interpStartTime )
			{
				_registers[i].interpStartTime = appTimeReal;
			}
			if( 0.0f == _registers[i].interpInverseDuration )
			{
				_registers[i].interpLastValue = _registers[i].interpEndValue;
			}
			else
			{
				FxReal parametricTime = (appTimeReal - _registers[i].interpStartTime) * 
					_registers[i].interpInverseDuration;
				// If there is a "next" interpolation operation and it is time to
				// start it, promote the "next" interpolation parameters up to the
				// "current" interpolation parameters.
				if( parametricTime >= 1.0f && 
					FxInvalidValue != _registers[i].interpNextEndValue )
				{
					_registers[i].interpStartTime       = appTimeReal;
					_registers[i].interpStartValue      = _registers[i].interpEndValue;
					_registers[i].interpEndValue        = _registers[i].interpNextEndValue;
					_registers[i].interpInverseDuration = _registers[i].interpNextInverseDuration;
					_registers[i].interpNextEndValue    = FxInvalidValue;
					parametricTime                      = 0.0f;
				}
				_registers[i].interpLastValue = 
					FxHermiteInterpolate(_registers[i].interpStartValue, 
					_registers[i].interpEndValue, 
					parametricTime);
			}
		}
	}
	if( _animPlayer.IsPlaying() )
	{
		currentState = _animPlayer.Tick(appTime, audioTime);
	}
	else if( _allowNonAnimTick )
	{
		// Clear the face graph.
		FxFaceGraph& faceGraph = _actor->GetFaceGraph();
		faceGraph.ClearValues();
		// Set the current time in the face graph.
		faceGraph.SetTime(appTimeReal);
		currentState = APS_Playing;
	}

	return currentState;
}

void FxActorInstance::ForceTick( const FxName& animName, const FxName& groupName, 
							     const FxReal forcedAnimationTime )
{
	// Need to set the animation as playing to make it valid inside the player.
	_animPlayer.Play(animName, groupName);
	// Force tick it.
	_animPlayer.Tick(forcedAnimationTime);
	// Stop it to clear it out of the player.
	_animPlayer.Stop();
}

FxBool 
FxActorInstance::SetRegisterEx( const FxName& regName, FxValueOp regOp, 
							    FxReal firstValue, FxReal firstInterpDuration, 
								FxReal nextValue, FxReal nextInterpDuration )
{
	FxAssert(_actor != NULL);
	// Find the register and set the value.
	FxSize numRegisters = _registers.Length();
	for( FxSize i = 0; i < numRegisters; ++i )
	{
		FxAssert(_registers[i].pBinding);
		if( _registers[i].pBinding->GetName() == regName )
		{
			// If firstValue is FxInvalidValue, this indicates that the 
			// register should be shut off.
			if( FxInvalidValue == firstValue )
			{
				_registers[i].interpLastValue = FxInvalidValue;
			}
			else if( FxInvalidValue == _registers[i].interpLastValue )
			{
				_registers[i].interpLastValue = 0.0f;
			}
			_registers[i].interpStartValue   = _registers[i].interpLastValue;
			_registers[i].interpEndValue     = firstValue;
			_registers[i].interpNextEndValue = nextValue;
			if( 0.0f == firstInterpDuration )
			{
				_registers[i].interpInverseDuration = 0.0f;
			}
			else
			{
				_registers[i].interpInverseDuration = 1.0f / firstInterpDuration;
			}
			if( 0.0f == nextInterpDuration )
			{
				_registers[i].interpNextInverseDuration = 0.0f;
			}
			else
			{
				_registers[i].interpNextInverseDuration = 1.0f / nextInterpDuration;
			}
			_registers[i].interpStartTime = FxInvalidValue;
			_registers[i].regOp           = regOp;
			return FxTrue;
		}
	}
	// The register was not found.
	// This will add the register to all instances, including this one.
	if( _actor->AddRegister(regName) )
	{
		// The register will now be at the end of the array, so we don't have
		// to search for it again.
		_registers.Back().interpEndValue     = firstValue;
		_registers.Back().interpNextEndValue = nextValue;
		if( 0.0f == firstInterpDuration )
		{
			_registers.Back().interpInverseDuration = 0.0f;
		}
		else
		{
			_registers.Back().interpInverseDuration = 1.0f / firstInterpDuration;
		}
		if( 0.0f == nextInterpDuration )
		{
			_registers.Back().interpNextInverseDuration = 0.0f;
		}
		else
		{
			_registers.Back().interpNextInverseDuration = 1.0f / nextInterpDuration;
		}
		_registers.Back().regOp = regOp;
		// If firstValue is FxInvalidValue, this indicates that the 
		// register should be shut off.
		if( FxInvalidValue == firstValue )
		{
			_registers.Back().interpLastValue = FxInvalidValue;
		}
		return FxTrue;
	}
	return FxFalse;
}

void FxActorInstance::SetAllRegisters( FxValueOp regOp, FxReal newValue, 
									   FxReal interpDuration )
{
	FxSize numRegisters = _registers.Length();
	for( FxSize i = 0; i < numRegisters; ++i )
	{
		FxAssert(_registers[i].pBinding);
		// If newValue is FxInvalidValue, this indicates that the 
		// register should be shut off.
		if( FxInvalidValue == newValue )
		{
			_registers[i].interpLastValue = FxInvalidValue;
		}
		else if( FxInvalidValue == _registers[i].interpLastValue )
		{
			_registers[i].interpLastValue = 0.0f;
		}
		_registers[i].interpStartValue   = _registers[i].interpLastValue;
		_registers[i].interpEndValue     = newValue;
		_registers[i].interpNextEndValue = FxInvalidValue;
		if( 0.0f == interpDuration )
		{
			_registers[i].interpInverseDuration = 0.0f;
		}
		else
		{
			_registers[i].interpInverseDuration = 1.0f / interpDuration;
		}
		_registers[i].interpNextInverseDuration = 0.0f;
		_registers[i].interpStartTime           = FxInvalidValue;
		_registers[i].regOp                     = regOp;
	}
}

FxReal FxActorInstance::GetRegister( const FxName& regName )
{
	FxAssert(_actor != NULL);
	// Find the register and get the value.
	FxSize numRegisters = _registers.Length();
	for( FxSize i = 0; i < numRegisters; ++i )
	{
		FxAssert(_registers[i].pBinding);
		if( _registers[i].pBinding->GetName() == regName )
		{
			return _registers[i].value;
		}
	}
	// The register was not found.
	// This will add the register to all instances, including this one.
	if( _actor->AddRegister(regName) )
	{
		// The register will now be at the end of the array, so we don't have
		// to search for it again.  The new registers value will be zero
		// by default.
		return _registers.Back().value;
	}
	return 0.0f;
}

FxBool FxActorInstance::_addRegister( FxFaceGraphNode* registerDef )
{
	// Make sure this isn't a duplicate.
	FxSize numRegisters = _registers.Length();
	for( FxSize i = 0; i < numRegisters; ++i )
	{
		if( _registers[i].pBinding == registerDef )
		{
			return FxFalse;
		}
	}
	_registers.PushBack(FxActorInstanceRegister(registerDef));
	return FxTrue;
}

FxBool FxActorInstance::_removeRegister( FxFaceGraphNode* registerDef )
{
	// Find this register.
	FxSize numRegisters = _registers.Length();
	for( FxSize i = 0; i < numRegisters; ++i )
	{
		if( _registers[i].pBinding == registerDef )
		{
			_registers.Remove(i);
			return FxTrue;
		}
	}
	return FxFalse;
}

} // namespace Face

} // namespace OC3Ent
