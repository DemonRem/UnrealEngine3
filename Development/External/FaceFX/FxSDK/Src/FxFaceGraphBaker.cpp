//------------------------------------------------------------------------------
// A special animation player that bakes out Face Graph nodes into animation
// curves.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "FxFaceGraphBaker.h"
#include "FxDeltaNode.h"

namespace OC3Ent
{

namespace Face
{

FxFaceGraphBaker::FxFaceGraphBaker( FxActor* pActor )
	: _pActor(pActor)
{
}

FxFaceGraphBaker::~FxFaceGraphBaker()
{
}

void FxFaceGraphBaker::SetActor( FxActor* pActor )
{
	_pActor = pActor;
}

FxActor* FxFaceGraphBaker::GetActor( void )
{
	return _pActor;
}

void FxFaceGraphBaker::SetAnim( const FxName& animGroupName, const FxName& animName )
{
	_animGroupName = animGroupName;
	_animName = animName;
}

const FxName& FxFaceGraphBaker::GetAnimGroupName( void ) const
{
	return _animGroupName;
}

const FxName& FxFaceGraphBaker::GetAnimName( void ) const
{
	return _animName;
}

FxBool FxFaceGraphBaker::Bake( const FxArray<FxName>& nodesToBake )
{
	// Always clear the results from a previous bake to keep the baker in
	// a consistent state.
	_resultingAnimCurves.Clear();
	if( _pActor && nodesToBake.Length() > 0 )
	{
		FxSize animGroupIndex = _pActor->FindAnimGroup(_animGroupName);
		if( FxInvalidIndex != animGroupIndex )
		{
			FxAnimGroup& animGroup = _pActor->GetAnimGroup(animGroupIndex);
			FxSize animIndex = animGroup.FindAnim(_animName);
			if( FxInvalidIndex != animIndex )
			{
				// The animation is valid, so grab a reference to it and the
				// Face Graph.
				const FxAnim& anim = animGroup.GetAnim(animIndex);
				FxFaceGraph& faceGraph = _pActor->GetFaceGraph();

				// Set up an array of Face Graph nodes based on nodesToBake.
				FxArray<FxFaceGraphNode*> faceGraphNodes;
				faceGraphNodes.Reserve(nodesToBake.Length());
				// Set up the delta nodes as well.
				FxArray<FxFaceGraphNode*> deltaNodes;
				FxSize numNodesToBake = nodesToBake.Length();
				deltaNodes.Reserve(numNodesToBake);
				for( FxSize i = 0; i < numNodesToBake; ++i )
				{
					// FindNode() returns NULL if the node cannot be found.
					FxFaceGraphNode* nodeToBake = faceGraph.FindNode(nodesToBake[i]);
					if( nodeToBake )
					{
						faceGraphNodes.PushBack(nodeToBake);
						// See if there is already a delta node.
						FxString deltaNodeNameStr = faceGraphNodes[i]->GetNameAsString();
						deltaNodeNameStr += "_$Baker_Auto_Delta$_";
						FxFaceGraphNode* deltaNode = faceGraph.FindNode(deltaNodeNameStr.GetData());
						if( !deltaNode )
						{
							// If not, create one and link it.
							deltaNode = new FxDeltaNode();
							deltaNode->SetName(deltaNodeNameStr.GetData());
							faceGraph.AddNode(deltaNode);
							faceGraph.Link(deltaNodeNameStr.GetData(), faceGraphNodes[i]->GetName(), "linear", FxLinkFnParameters());
						}
						deltaNodes.PushBack(deltaNode);
					}
				}

				// Initialize the last values and last derivatives arrays.
				FxArray<FxReal> lastValues;
				FxArray<FxReal> lastDerivatives;
				FxSize numNodes = faceGraphNodes.Length();
				lastValues.Reserve(numNodes);
				lastDerivatives.Reserve(numNodes);
				for( FxSize i = 0; i < numNodes; ++i )
				{
					lastDerivatives[i] = 0.0f;
					lastValues[i] = 0.0f;
				}

				// Set up the array of resulting curves.
				for( FxSize i = 0; i < numNodes; ++i )
				{
					if( faceGraphNodes[i] )
					{
						FxAnimCurve resultingAnimCurve;
						resultingAnimCurve.SetName(faceGraphNodes[i]->GetName());
						_resultingAnimCurves.PushBack(resultingAnimCurve);
					}
				}

				// Pad the start and end times so we don't miss anything important.
				FxReal timeStep = 0.0166666667f; // 60fps.
				FxReal startTime = anim.GetStartTime() - (timeStep * 3);
				FxReal endTime = anim.GetEndTime() + (timeStep * 3);
				for( FxReal time = startTime; time < endTime; time += timeStep )
				{
					faceGraph.ClearValues();
					faceGraph.SetTime(time);
					anim.Tick(time);
					// Go through each track and get its value and derivative.
					for( FxSize i = 0; i < numNodes; ++i )
					{
						FxReal derivative = deltaNodes[i]->GetValue();
						if( FxAbs(derivative) < FX_REAL_EPSILON )
						{
							derivative = 0.0f;
						}
						FxReal value = faceGraphNodes[i]->GetValue();

						FxBool lastDerivEqualsZero = FxRealEquality(lastDerivatives[i], 0.0f);
						FxBool thisDerivEqualsZero = FxRealEquality(derivative, 0.0f);
						// Insert a key if we had a zero derivative and now we don't.
						if( (lastDerivEqualsZero && !thisDerivEqualsZero) ||
							// Or, insert a key if we had a non-zero derivative and now we have a zero derivative.
							(!lastDerivEqualsZero && thisDerivEqualsZero) ||
							// Or, insert a key if we had a positive derivative and now we have negative.
							(lastDerivatives[i] > 0.0f && derivative < 0.0f) || 
							// Or, insert a key if we had a negative derivative and now we have positive.
							(lastDerivatives[i] < 0.0f && derivative > 0.0f) )
						{
							// Use the last value to make sure if we've started up from zero
							// that the keys are actually on zero.
							_resultingAnimCurves[i].InsertKey(FxAnimKey(time-(timeStep*0.5f), lastValues[i]));
						}

						lastValues[i] = value;
						lastDerivatives[i] = derivative;
					}
				}

				// Clean up.
				for( FxSize i = 0; i < numNodes; ++i )
				{
					if( faceGraphNodes[i] )
					{
						// Remove the delta node.
						FxString deltaNodeNameStr = faceGraphNodes[i]->GetNameAsString();
						deltaNodeNameStr += "_$Baker_Auto_Delta$_";
						faceGraph.RemoveNode(deltaNodeNameStr.GetData());
					}
				}
				return FxTrue;
			}
		}
	}
	return FxFalse;
}

FxSize FxFaceGraphBaker::GetNumResultAnimCurves( void ) const
{
	return _resultingAnimCurves.Length();
}

const FxAnimCurve& FxFaceGraphBaker::GetResultAnimCurve( FxSize index ) const
{
	return _resultingAnimCurves[index];
}

} // namespace Face

} // namespace OC3Ent
