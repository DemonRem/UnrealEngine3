//------------------------------------------------------------------------------
// A command for importing or exporting FxActor data in XML format.
//
// Owner: Doug Perkowski
//
// Copyright (c) 2002-2009 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------
#ifndef FxActorXMLCommand_H__
#define FxActorXMLCommand_H__

#include "FxCommand.h"

namespace OC3Ent
{
namespace Face
{

// The FxActorXML command.
class FxActorXMLCommand : public FxCommand
{

	// Declare the class.
	FX_DECLARE_CLASS(FxActorXMLCommand, FxCommand);
public:
	// Constructor.
	FxActorXMLCommand();
	// Destructor.
	virtual ~FxActorXMLCommand();

	// Sets up the argument syntax.
	static FxCommandSyntax CreateSyntax( void );

	// Execute the command.
	virtual FxCommandError Execute( const FxCommandArgumentList& argList );

protected:
	// Cached anim group selection
	FxString _cachedAnimGroup;
	// Cached anim selection
	FxString _cachedAnim;
	// Cached curve selection
	FxArray<FxString> _cachedCurves;
	// Cached key selection
	FxArray<FxSize> _cachedCurveIndices;
	FxArray<FxSize> _cachedKeyIndices;
};


} // namespace Face

} // namespace OC3Ent

#endif
