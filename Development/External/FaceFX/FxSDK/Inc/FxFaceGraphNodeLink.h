//------------------------------------------------------------------------------
// This class defines a link between two nodes in an FxFaceGraph.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#ifndef FxFaceGraphNodeLink_H__
#define FxFaceGraphNodeLink_H__

#include "FxPlatform.h"
#include "FxObject.h"
#include "FxLinkFn.h"
#include "FxName.h"

namespace OC3Ent
{

namespace Face
{

// Forward declare FxFaceGraphNode.
class FxFaceGraphNode;

/// A link between two nodes in the %Face Graph.
/// \ingroup faceGraph
class FxFaceGraphNodeLink : public FxObject, public FxDataContainer
{
	/// Declare the class.
	FX_DECLARE_CLASS(FxFaceGraphNodeLink, FxObject)
public:
	/// Default constructor.
	FxFaceGraphNodeLink();

	/// Constructor.
	/// \param inputName The name of the node to which to link.
	/// \param inputNode The node to which to link.
	/// \param linkFunctionName The name of the link function to use.
	/// \param linkFunction A pointer to the function to execute on the value 
	/// coming from \a inputNode.
	/// \param linkFunctionParams The parameters of the link function.
	FxFaceGraphNodeLink( const FxName& inputName, FxFaceGraphNode* inputNode, 
					     const FxName& linkFunctionName, 
					     const FxLinkFn* linkFunction,
					     const FxLinkFnParameters& linkFunctionParams );

	/// Copy constructor.
	FxFaceGraphNodeLink( const FxFaceGraphNodeLink& other );
	/// Assignment operator.
	FxFaceGraphNodeLink& operator=( const FxFaceGraphNodeLink& other );
	/// Destructor.
	virtual ~FxFaceGraphNodeLink();

	/// Takes the value from the linked node, transforms it by the link function
	/// and returns it.
	FxReal GetTransformedValue( void ) const;
	/// Returns the value to use for correction.  Only valid if IsCorrective().
	FxReal GetCorrectiveValue( FxReal formerCorrectiveValue ) const;

	/// Returns the name of the linked node.
	FX_INLINE const FxName& GetNodeName( void ) const { return _inputName; }
	
	/// Returns a pointer to the linked node.
	FX_INLINE const FxFaceGraphNode* GetNode( void ) const { return _input; }
	/// Sets the linked node.
	void SetNode( FxFaceGraphNode* inputNode );

	/// Returns the link function algorithm name.
	FxName GetLinkFnName( void ) const;
	/// Sets the link function algorithm name.
	void SetLinkFnName( const FxName& linkFn );

	/// Returns a pointer to the link function.
	FX_INLINE const FxLinkFn* GetLinkFn( void ) const { return _linkFn; }
	/// Sets the link function pointer.
	void SetLinkFn( const FxLinkFn* linkFn );

	/// Returns the link function parameters.
	FxLinkFnParameters GetLinkFnParams( void ) const;
	/// Sets the link function parameters.
	void SetLinkFnParams( const FxLinkFnParameters& linkFnParams );

	/// Returns FxTrue if the link is corrective.
	FxBool IsCorrective( void ) const;

	/// Returns the correction factor.
	FxReal GetCorrectionFactor( void ) const;
	/// Sets the correction factor.
	void SetCorrectionFactor( FxReal newValue );

	/// Serialization.
	void Serialize( FxArchive& arc );

private:
	/// The name of the node we're linked to.
	FxName _inputName;
	/// The node we're linked to.
	FxFaceGraphNode* _input;
	/// The function name.
	FxName _linkFnName;
	/// The function with which to transform this input's value.
	const FxLinkFn* _linkFn;
	/// The parameters to the link function.
	FxLinkFnParameters _linkFnParams;
};

FX_INLINE FxBool FxFaceGraphNodeLink::IsCorrective( void ) const
{
	FxAssert(_linkFn);
	return _linkFn->IsCorrective();
}

FX_INLINE FxReal FxFaceGraphNodeLink::GetCorrectionFactor( void ) const
{
	FxAssert(_linkFn);
	if( _linkFnParams.parameters.Length() )
	{
		return _linkFnParams.parameters[0];
	}
	return _linkFn->GetParamDefaultValue(0);
}

} // namespace Face

} // namespace OC3Ent

#endif
