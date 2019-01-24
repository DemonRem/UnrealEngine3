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

#include "FxUnrealSupport.h"

namespace OC3Ent
{

namespace Face
{

//------------------------------------------------------------------------------
// FUnrealFaceFXNode.
//------------------------------------------------------------------------------

#define kCurrentFUnrealFaceFXNodeVersion 0

FX_IMPLEMENT_CLASS(FUnrealFaceFXNode, kCurrentFUnrealFaceFXNodeVersion, FxGenericTargetNode)

FUnrealFaceFXNode::FUnrealFaceFXNode()
{
	_shouldLink = FxFalse;
}

FUnrealFaceFXNode::~FUnrealFaceFXNode()
{}

FxFaceGraphNode* FUnrealFaceFXNode::Clone()
{
	FUnrealFaceFXNode* pNode = new FUnrealFaceFXNode();
	CopyData(pNode);
	return pNode;
}

void FUnrealFaceFXNode::CopyData(FxFaceGraphNode* pOther)
{
	Super::CopyData(pOther);
}

void FUnrealFaceFXNode::SetShouldLink(FxBool shouldLink)
{
	_shouldLink = shouldLink;
}

void FUnrealFaceFXNode::
	ReplaceUserProperty(const FxFaceGraphNodeUserProperty& userProperty)
{
	Super::ReplaceUserProperty(userProperty);
	// Force a re-link during the next render.
	_shouldLink = FxTrue;
}

void FUnrealFaceFXNode::Serialize(FxArchive& arc)
{
	Super::Serialize(arc);
	arc.SerializeClassVersion("FUnrealFaceFXNode");
}

//------------------------------------------------------------------------------
// FUnrealFaceFXMorphNode.
//------------------------------------------------------------------------------

#define kCurrentFUnrealFaceFXMorphNodeVersion 0

FX_IMPLEMENT_CLASS(FUnrealFaceFXMorphNode, kCurrentFUnrealFaceFXMorphNodeVersion, FUnrealFaceFXNode)

FUnrealFaceFXMorphNode::FUnrealFaceFXMorphNode()
{
	FxFaceGraphNodeUserProperty targetNameProperty(UPT_String);
	targetNameProperty.SetName("Target Name");
	targetNameProperty.SetStringProperty(FxString("Target"));
	AddUserProperty(targetNameProperty);
}

FUnrealFaceFXMorphNode::~FUnrealFaceFXMorphNode()
{}

FxFaceGraphNode* FUnrealFaceFXMorphNode::Clone()
{
	FUnrealFaceFXMorphNode* pNode = new FUnrealFaceFXMorphNode();
	CopyData(pNode);
	return pNode;
}

void FUnrealFaceFXMorphNode::CopyData(FxFaceGraphNode* pOther)
{
	Super::CopyData(pOther);
}

void FUnrealFaceFXMorphNode::Serialize(FxArchive& arc)
{
	Super::Serialize(arc);
	arc.SerializeClassVersion("FUnrealFaceFXMorphNode");
}

//------------------------------------------------------------------------------
// FUnrealFaceFXMaterialParameterNode.
//------------------------------------------------------------------------------

#define kCurrentFUnrealFaceFXMaterialParameterNodeVersion 0

FX_IMPLEMENT_CLASS(FUnrealFaceFXMaterialParameterNode, kCurrentFUnrealFaceFXMaterialParameterNodeVersion, FUnrealFaceFXNode)

FUnrealFaceFXMaterialParameterNode::FUnrealFaceFXMaterialParameterNode()
{
	FxFaceGraphNodeUserProperty materialSlotProperty(UPT_Integer);
	materialSlotProperty.SetName("Material Slot Id");
	materialSlotProperty.SetIntegerProperty(0);
	AddUserProperty(materialSlotProperty);
	FxFaceGraphNodeUserProperty materialParameterNameProperty(UPT_String);
	materialParameterNameProperty.SetName("Parameter Name");
	materialParameterNameProperty.SetStringProperty(FxString("Param"));
	AddUserProperty(materialParameterNameProperty);
}

FUnrealFaceFXMaterialParameterNode::~FUnrealFaceFXMaterialParameterNode()
{}

FxFaceGraphNode* FUnrealFaceFXMaterialParameterNode::Clone()
{
	FUnrealFaceFXMaterialParameterNode* pNode = new FUnrealFaceFXMaterialParameterNode();
	CopyData(pNode);
	return pNode;
}

void FUnrealFaceFXMaterialParameterNode::CopyData(FxFaceGraphNode* pOther)
{
	Super::CopyData(pOther);
}

void FUnrealFaceFXMaterialParameterNode::Serialize(FxArchive& arc)
{
	Super::Serialize(arc);
	arc.SerializeClassVersion("FUnrealFaceFXMaterialParameterNode");
}

} // namespace Face

} // namespace OC3Ent
