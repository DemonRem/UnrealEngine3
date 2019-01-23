//------------------------------------------------------------------------------
// This is the base FaceFx object class.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "FxObject.h"

namespace OC3Ent
{

namespace Face
{

#define kCurrentFxObjectVersion 0

FX_IMPLEMENT_BASE_CLASS(FxObject, kCurrentFxObjectVersion)

FxObject::FxObject()
{
}

FxObject::~FxObject()
{
}

void FxObject::Serialize( FxArchive& arc )
{
	FxUInt16 version = FX_GET_CLASS_VERSION(FxObject);
	arc << version;
}

FxDataContainer::FxDataContainer()
	: _userData(NULL)
{
}

FxDataContainer::FxDataContainer( const FxDataContainer& other )
	: _userData(other._userData)
{
}

FxDataContainer& FxDataContainer::operator=( const FxDataContainer& other )
{
	_userData = other._userData;
	return *this;
}

void* FxDataContainer::GetUserData( void )
{
	return _userData;
}

void FxDataContainer::SetUserData( void* userData )
{
	_userData = userData;
}

} // namespace Face

} // namespace OC3Ent
