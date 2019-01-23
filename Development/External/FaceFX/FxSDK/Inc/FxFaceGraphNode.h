//------------------------------------------------------------------------------
// This abstract class implements a node in an FxFaceGraph.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#ifndef FxFaceGraphNode_H__
#define FxFaceGraphNode_H__

#include "FxPlatform.h"
#include "FxArchive.h"
#include "FxNamedObject.h"
#include "FxArray.h"
#include "FxFaceGraphNodeLink.h"

namespace OC3Ent
{

namespace Face
{

/// The available %Face Graph node user property types.
/// \ingroup faceGraph
enum FxFaceGraphNodeUserPropertyType
{
	UPT_Integer = 0, ///< An integer property.
	UPT_Bool    = 1, ///< A boolean property.
	UPT_Real    = 2, ///< A floating-point property.
	UPT_String  = 3, ///< A string property.
	UPT_Choice  = 4  ///< A choice property.
};

/// A named %Face Graph node property.
/// \ingroup faceGraph
class FxFaceGraphNodeUserProperty : public FxNamedObject
{
	/// Declare the class.
	FX_DECLARE_CLASS(FxFaceGraphNodeUserProperty, FxNamedObject)
public:
	/// Constructor.
	FxFaceGraphNodeUserProperty( FxFaceGraphNodeUserPropertyType propertyType = UPT_Integer );
	/// Copy constructor.
	FxFaceGraphNodeUserProperty( const FxFaceGraphNodeUserProperty& other );
	/// Assignment.
	FxFaceGraphNodeUserProperty& operator=( const FxFaceGraphNodeUserProperty& other );
	/// Destructor.
	virtual ~FxFaceGraphNodeUserProperty();

	/// Returns the user property type.
	FxFaceGraphNodeUserPropertyType GetPropertyType( void ) const;

	/// Returns the integer property.  The return value is only valid if the 
	/// type of user property is Face::UPT_Integer.
	FxInt32 GetIntegerProperty( void ) const;
	/// Sets the integer property if the type of user property is Face::UPT_Integer.
	void SetIntegerProperty( FxInt32 integerProperty );

	/// Returns the bool property.  The return value is only valid if the
	/// type of user property is Face::UPT_Bool.
	FxBool GetBoolProperty( void ) const;
	/// Sets the bool property if the type of user property is Face::UPT_Bool.
	void SetBoolProperty( FxBool boolProperty );

	/// Returns the real property.  The return value is only valid if the type 
	/// of user property is Face::UPT_Real.
	FxReal GetRealProperty( void ) const;
	/// Sets the real property if the type of user property is Face::UPT_Real.
	void SetRealProperty( FxReal realProperty );

	/// Returns the string property.  The return value is only valid if the 
	/// type of user property is Face::UPT_String.
	const FxString& GetStringProperty( void ) const;
	/// Sets the string property if the type of user property is Face::UPT_String.
	void SetStringProperty( const FxString& stringProperty );

	/// Returns the choice property (current choice).  The return value is only
	/// valid if the type of user property is Face::UPT_Choice.
	FxString GetChoiceProperty( void ) const;
	/// Sets the choice property (current choice) if the type of user property
	/// is Face::UPT_Choice.
	void SetChoiceProperty( const FxString& choiceProperty );

	/// Returns the number of choices if the type of user property is 
	/// Face::UPT_Choice.
	FxSize GetNumChoices( void ) const;
	/// Returns the choice at index if the type of user property is Face::UPT_Choice.
	const FxString& GetChoice( FxSize index ) const;
	/// Adds a choice if the type of user property is Face::UPT_Choice.
	void AddChoice( const FxString& choice );

	/// Serializes an FxFaceGraphNodeUserProperty to an archive.
	virtual void Serialize( FxArchive& arc );

protected:
	/// The type of user property.
	FxFaceGraphNodeUserPropertyType _propertyType;
	/// The integer property.  If the type is Face::UPT_Choice, this is used as an
	/// index into the \a _choices array.  If the type is Face::UPT_Bool, this is used
	/// as a boolean value where zero is false and non-zero is true.
	FxInt32 _integerProperty;
	/// The real property.
	FxReal _realProperty;
	/// The choices.  If the type is Face::UPT_String, the first element of \a _choices
	/// is used as the string property.
	FxArray<FxString> _choices;
};

/// The available operations for use with the node inputs.
/// \ingroup faceGraph
enum FxInputOp
{
	IO_Sum      = 0, ///< Sum the inputs.
	IO_Multiply = 1	 ///< Multiply the inputs together.
};

/// The available operations for use with the SetUserValue() call.
/// \ingroup faceGraph
enum FxValueOp
{
	VO_Add      = IO_Sum,      ///< Add the user value into the node value.
	VO_Multiply = IO_Multiply, ///< Multiply the user value with the node value.
	VO_Replace  = 2            ///< Replace the node value with the user value.
};

/// The base class for a node in an FxFaceGraph.
/// \ingroup faceGraph
class FxFaceGraphNode : public FxNamedObject, public FxDataContainer
{
	/// Declare the class.
	FX_DECLARE_CLASS_NO_DYNCREATE(FxFaceGraphNode, FxNamedObject)
	/// Disable copy construction and assignment.
	FX_NO_COPY_OR_ASSIGNMENT(FxFaceGraphNode)
public:
	/// Face Graphs are our friends.
	friend class FxFaceGraph;

	/// Constructor.
	FxFaceGraphNode();
	/// Destructor.
	virtual ~FxFaceGraphNode() = 0;

	/// Clones the %Face Graph node.  All data, including inherited data, must
	/// be cloned.
	virtual FxFaceGraphNode* Clone( void ) = 0;
	/// Copies the data from this object into the other object.
	virtual void CopyData( FxFaceGraphNode* pOther );

	/// Resets the node's values.
	void ClearValues( void );
	/// Returns the value of the node.  This will cause the portion of the %Face Graph
	/// logically "above" the node to be evaluated to ensure the value is 
	/// correct.
	virtual FxReal GetValue( void );
	
	/// Returns the minimum value this node is allowed to assume.
	FX_INLINE FxReal GetMin( void ) const { return _min; }
	/// Sets the minimum value this node is allowed to assume.
	void SetMin( FxReal iMin );

	/// Returns the maximum value this node is allowed to assume.
	FX_INLINE FxReal GetMax( void ) const { return _max; }
	/// Sets the maximum value this node is allowed to assume.
	void SetMax( FxReal iMax );

	/// Returns the operation performed on inputs.
	FX_INLINE FxInputOp GetNodeOperation( void ) const { return _inputOperation; }
	/// Sets the operation performed on inputs.
	void SetNodeOperation( FxInputOp iNodeOperation );

	/// Returns the value of the corresponding animation track.  This is mostly
	/// for internal use, but advanced users might find this useful.  If you
	/// just want the current value of the node, call GetValue().  This could
	/// return \p FxInvalidValue.
	FX_INLINE FxReal GetTrackValue( void ) const { return _trackValue; }
	/// Returns the user value of the node.  This is mostly for internal use, 
	/// but advanced users might find this useful.  If you just want the 
	/// current value of the node, call GetValue().  This could 
	/// return \p FxInvalidValue.
	FX_INLINE FxReal GetUserValue( void ) const { return _userValue; }
	/// This resets all values except the user value.  This is for internal
	/// use only and should not be used as a substitute for ClearValues().
	void ClearAllButUserValue( void );

	/// Sets the value of the corresponding animation track.
	void SetTrackValue( FxReal trackValue );
	/// Sets the user value of the node.
	void SetUserValue( FxReal userValue, FxValueOp userValueOp );

	/// Adds an input link to this %Face Graph node.
	/// \param newInputLink The link to add to the node.
	/// \return \p FxTrue if the link was added, \p FxFalse otherwise.
	virtual FxBool AddInputLink( const FxFaceGraphNodeLink& newInputLink );
	/// Modifies the input link at index, setting it equal to \a inputLink.
	void ModifyInputLink( FxSize index, const FxFaceGraphNodeLink& inputLink );
	/// Removes the input link at the specified index.
	void RemoveInputLink( FxSize index );
	/// Returns the number of inputs to this %Face Graph node.
	FX_INLINE FxSize GetNumInputLinks( void ) const { return _inputs.Length(); }
	/// Returns a const ref to the input link at the specified index.
	FX_INLINE const FxFaceGraphNodeLink& GetInputLink( FxSize index ) const { return _inputs[index]; }
	
	/// Returns the number of outputs from this node.
	/// \return The number of outputs from this node.
	/// \note For internal use only.
	FX_INLINE FxSize GetNumOutputs( void ) const { return _outputs.Length(); }
	/// Returns the output at index.
	/// \param index The index of the desired output.
	/// \return The output at \a index.
	/// \note For internal use only.
	FX_INLINE FxFaceGraphNode* GetOutput( FxSize index ) { return _outputs[index]; }
	/// Adds an output to the node.
	/// \param pOutput A pointer to the output node.
	/// \note For internal use only.
	void AddOutput( FxFaceGraphNode* pOutput );
	/// Checks if this node type is "placeable" in the tools.
	/// \return FxTrue if the node is "placeable", FxFalse otherwise.
	/// \note For internal use only.
	FxBool IsPlaceable( void ) const;

	/// Adds a user property to the node.  If a user property with the same
	/// name already exists, the user property is not added.  This should only 
	/// be called in the constructor of the new node type to add the same 
	/// properties to all nodes of that type.
	/// \param userProperty The property to add.
	void AddUserProperty( const FxFaceGraphNodeUserProperty& userProperty );
	/// Returns the number of user properties associated with the node.
	FxSize GetNumUserProperties( void ) const;
	/// Returns the index of the user property named 'userPropertyName' or 
	/// FxInvalidIndex if it does not exist.
	FxSize FindUserProperty( const FxName& userPropertyName ) const;
	/// Returns the user property at index.  For performance reasons, if access
	/// to the user property is needed on a regular basis at run-time it is 
	/// best to define or cache the index for each property rather than calling
	/// FindUserProperty() before GetUserProperty().
	const FxFaceGraphNodeUserProperty& GetUserProperty( FxSize index ) const;
	/// Replaces any user property with the same name as the supplied user 
	/// property with the values in the supplied user property.  If the
	/// user property does not exist, nothing happens.  Overload this function
	/// to receive indications that a user property has changed from within
	/// FaceFX Studio and then perform any required proxy re-linking steps.
	/// \param userProperty The property with new data.
	virtual void ReplaceUserProperty( const FxFaceGraphNodeUserProperty& userProperty );
	
	/// Serializes an FxFaceGraphNode to an archive.
	virtual void Serialize( FxArchive& arc );

protected:
	/// The current value of this node.
	/// \note This is mutable so we can change it in _hasCycles().
	mutable FxReal _value;
	/// The value of the track 'driving' this node.
	FxReal _trackValue;
	/// The minimum value this node is allowed to assume.
	FxReal _min;
	/// The maximum value this node is allowed to assume.
	FxReal _max;

	/// The value of the node as set by the user.
	FxReal _userValue;
	/// The operation used to move the value into the node.
	FxValueOp _userValueOperation;

	/// Defines what to do with the inputs.
	FxInputOp _inputOperation;
	/// The inputs to this node.
	FxArray<FxFaceGraphNodeLink> _inputs;

	/// The outputs from this node.
	/// \note Outputs are strictly for internal bookkeeping usage.
	FxArray<FxFaceGraphNode*> _outputs;

	/// FxTrue if this node type is "placeable" by the user in editing tools.
	/// \note This is for internal usage only.
	FxBool _isPlaceable;

	/// The user properties associated with this node.
	FxArray<FxFaceGraphNodeUserProperty> _userProperties;
	
	/// Returns FxTrue if any input of the node has cycles.
	FxBool _hasCycles( void ) const;
};

FX_INLINE void FxFaceGraphNode::ClearValues( void )
{
	_value      = FxInvalidValue;
	_trackValue = FxInvalidValue;
	_userValue  = FxInvalidValue;
}

FX_INLINE void FxFaceGraphNode::ClearAllButUserValue( void )
{
	_value      = FxInvalidValue;
	_trackValue = FxInvalidValue;
}

} // namespace Face

} // namespace OC3Ent

#endif
