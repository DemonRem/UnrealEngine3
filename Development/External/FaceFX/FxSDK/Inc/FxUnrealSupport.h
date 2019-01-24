//------------------------------------------------------------------------------
// This file contains definitions for Unreal Engine specific Face Graph node
// types. These types no longer depend on any Unreal Engine data types so they
// are safe to pull back into our SDK to reduce the number of build 
// configurations for tools.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2009 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#ifndef FxUnrealSupport_H__
#define FxUnrealSupport_H__

#include "FxPlatform.h"
#include "FxGenericTargetNode.h"

namespace OC3Ent
{

namespace Face
{

// Note these classes don't use Doxygen style comments intentionally so that
// Unreal specific stuff doesn't show up in our normal documentation.

// An Unreal-specific FaceFX Face Graph node.
class FUnrealFaceFXNode : public FxGenericTargetNode
{
	// Declare the class.
	FX_DECLARE_CLASS(FUnrealFaceFXNode, FxGenericTargetNode)
	// Disable copy construction and assignment.
	FX_NO_COPY_OR_ASSIGNMENT(FUnrealFaceFXNode)
public:
	// Constructor.
	FUnrealFaceFXNode();
	// Destructor.
	virtual ~FUnrealFaceFXNode();

	// Clones the node.
	virtual FxFaceGraphNode* Clone();

	// Copies the data from this object into the other object.
	virtual void CopyData(FxFaceGraphNode* pOther);

	// Returns FxTrue if the engine should perform a linkup during the next
	// render.
	FX_INLINE FxBool ShouldLink() const { return _shouldLink; }
	// Sets whether or not the engine should perform a linkup during the next
	// render.
	void SetShouldLink(FxBool shouldLink);

	// Replaces any user property with the same name as the supplied user 
	// property with the values in the supplied user property.  If the
	// user property does not exist, nothing happens.
	virtual void ReplaceUserProperty(const FxFaceGraphNodeUserProperty& userProperty);

	// Serializes an FUnrealFaceFXNode to an archive.
	virtual void Serialize(FxArchive& arc);

protected:
	// FxTrue if the engine should perform a linkup during the next render.
	FxBool _shouldLink;
};

#define FX_UNREAL_MORPH_NODE_TARGET_NAME_INDEX 0

// An Unreal morph target FaceFX Face Graph node.
class FUnrealFaceFXMorphNode : public FUnrealFaceFXNode
{
	// Declare the class.
	FX_DECLARE_CLASS(FUnrealFaceFXMorphNode, FUnrealFaceFXNode)
	// Disable copy construction and assignment.
	FX_NO_COPY_OR_ASSIGNMENT(FUnrealFaceFXMorphNode)
public:
	// Constructor.
	FUnrealFaceFXMorphNode();
	// Destructor.
	virtual ~FUnrealFaceFXMorphNode();

	// Clones the material parameter node.
	virtual FxFaceGraphNode* Clone();

	// Copies the data from this object into the other object.
	virtual void CopyData(FxFaceGraphNode* pOther);

	// Serializes an FUnrealFaceFXMorphNode to an archive.
	virtual void Serialize(FxArchive& arc);
};

#define FX_UNREAL_MATERIAL_PARAMETER_NODE_MATERIAL_SLOT_ID_INDEX 0
#define FX_UNREAL_MATERIAL_PARAMETER_NODE_PARAMETER_NAME_INDEX   1

// An Unreal scalar material parameter FaceFX Face Graph node.
class FUnrealFaceFXMaterialParameterNode : public FUnrealFaceFXNode
{
	// Declare the class.
	FX_DECLARE_CLASS(FUnrealFaceFXMaterialParameterNode, FUnrealFaceFXNode)
	// Disable copy construction and assignment.
	FX_NO_COPY_OR_ASSIGNMENT(FUnrealFaceFXMaterialParameterNode)
public:
	// Constructor.
	FUnrealFaceFXMaterialParameterNode();
	// Destructor.
	virtual ~FUnrealFaceFXMaterialParameterNode();

	// Clones the material parameter node.
	virtual FxFaceGraphNode* Clone();

	// Copies the data from this object into the other object.
	virtual void CopyData(FxFaceGraphNode* pOther);

	// Serializes an FUnrealFaceFXMaterialParameterNode to an archive.
	virtual void Serialize(FxArchive& arc);
};

} // namespace Face

} // namespace OC3Ent

#endif // FxUnrealSupport_H__
