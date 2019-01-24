//------------------------------------------------------------------------------
// The static global data maintained by the FaceFx MotionBuilder plug-in.
//
// Owner: Doug Perkowski
//
// Copyright (c) 2002-2009 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#ifndef FxMBData_H__
#define FxMBData_H__

#include "FxToolMB.h"

namespace OC3Ent
{

namespace Face
{
	
// Static global data for the Max plug-in.
class FxMBData
{
public:
	// The Max plug-in interface.
	static FxToolMB mbInterface;
};

} // namespace Face

} // namespace OC3Ent

#endif
