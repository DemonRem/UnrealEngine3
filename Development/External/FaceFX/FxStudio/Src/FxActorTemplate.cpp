//------------------------------------------------------------------------------
// Actor template support.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "stdwx.h"

#include "FxActorTemplate.h"

namespace OC3Ent
{

namespace Face
{

#define kCurrentFxActorTemplateVersion 0

FX_IMPLEMENT_CLASS(FxActorTemplate,kCurrentFxActorTemplateVersion,FxNamedObject);

FxActorTemplate::FxActorTemplate()
{
}

FxActorTemplate::~FxActorTemplate()
{
}

void FxActorTemplate::SetFaceGraph( const FxFaceGraph& faceGraph )
{
	_faceGraph = faceGraph;
}

FxFaceGraph& FxActorTemplate::GetFaceGraph( void )
{
	return _faceGraph;
}

void FxActorTemplate::SetPhonemeMap( const FxPhonemeMap& phonemeMap )
{
	_phonemeMap = phonemeMap;
}

const FxPhonemeMap& FxActorTemplate::GetPhonemeMap( void ) const
{
	return _phonemeMap;
}

void FxActorTemplate::SetNodeGroups( const FxArray<FxFaceGraphNodeGroup>& nodeGroups )
{
	_nodeGroups = nodeGroups;
}

const FxArray<FxFaceGraphNodeGroup>& FxActorTemplate::GetNodeGroups( void ) const
{
	return _nodeGroups;
}

void FxActorTemplate::SetCameras( const FxArray<FxViewportCamera>& cameras )
{
	_cameras = cameras;
}

const FxArray<FxViewportCamera>& FxActorTemplate::GetCameras( void ) const
{
	return _cameras;
}

void FxActorTemplate::SetWorkspaces( const FxArray<FxWorkspace>& workspaces )
{
	_workspaces = workspaces;
}

const FxArray<FxWorkspace>& FxActorTemplate::GetWorkspaces( void ) const
{
	return _workspaces;
}

void FxActorTemplate::Serialize( FxArchive& arc )
{
	Super::Serialize(arc);

	FxUInt16 version = FX_GET_CLASS_VERSION(FxActorTemplate);
	arc << version;

	arc << _faceGraph << _phonemeMap << _nodeGroups << _cameras << _workspaces;
}

} // namespace Face

} // namespace OC3Ent
