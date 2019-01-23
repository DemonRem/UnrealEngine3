//------------------------------------------------------------------------------
// The curve command.
// 
// Owner: John Briggs
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "stdwx.h"

#include "FxCurveCommand.h"
#include "FxVM.h"
#include "FxSessionProxy.h"

namespace OC3Ent
{

namespace Face
{

FX_IMPLEMENT_CLASS(FxCurveCommand, 0, FxCommand);

FxCurveCommand::FxCurveCommand()
	: _cachedAnimCurve( NULL )
	, _cachedOwner( FxFalse )
	, _cachedInterp( IT_Hermite )
	, _mode( NONE )
{
	_isUndoable = FxTrue;
}

FxCurveCommand::~FxCurveCommand()
{
	if( _cachedAnimCurve )
	{
		delete _cachedAnimCurve;
		_cachedAnimCurve = NULL;
	}
}

FxCommandSyntax FxCurveCommand::CreateSyntax( void )
{
	FxCommandSyntax syntax;
	syntax.AddArgDesc( FxCommandArgumentDesc("-an", "-anim", CAT_String) );
	syntax.AddArgDesc( FxCommandArgumentDesc("-gr", "-group", CAT_String) );
	syntax.AddArgDesc( FxCommandArgumentDesc("-a", "-add", CAT_Flag) );
	syntax.AddArgDesc( FxCommandArgumentDesc("-r", "-remove", CAT_Flag) );
	syntax.AddArgDesc( FxCommandArgumentDesc("-e", "-edit", CAT_Flag) );
	syntax.AddArgDesc( FxCommandArgumentDesc("-n", "-name", CAT_String) );
	syntax.AddArgDesc( FxCommandArgumentDesc("-o", "-owner", CAT_String) );
	syntax.AddArgDesc( FxCommandArgumentDesc("-i", "-interpolation", CAT_String) );
	return syntax;
}

#define FAIL(msg) FxVM::DisplayError((msg)); return CE_Failure;

FxCommandError FxCurveCommand::Execute( const FxCommandArgumentList& argList )
{
	// Get the group, anim, and curve names. All three must exist.
	if( !argList.GetArgument("-group", _group) )
	{
		FAIL("Must specify -group \"group name\"");
	}
	if( !argList.GetArgument("-anim", _anim) )
	{
		FAIL("Must specify -anim \"animation name\"");
	}
	if( !argList.GetArgument("-name", _curve) )
	{
		FAIL("Must specify -name \"curve name\"");
	}
	
	// Get the action - add, remove or edit.  One of three must exist.
	if( argList.GetArgument("-add") )
	{
		_mode = ADD;
	}
	else if( argList.GetArgument("-remove") )
	{
		// Cache the animation curve.
		FxAnimCurve* pCurve = NULL;
		FxSessionProxy::GetCurve(_group, _anim, _curve, &pCurve);
		if( !FxSessionProxy::GetSelectedAnimCurves(_cachedSelectedCurves) )
		{
			FAIL("Could not cache selected curves.");
		}
		if( pCurve )
		{
			_cachedAnimCurve = new FxAnimCurve(*pCurve);
			// Set the user data to NULL or Bad Things will happen when undoing
			// and redoing this operation.
			if( _cachedAnimCurve )
			{
				for( FxSize i = 0; i < _cachedAnimCurve->GetNumKeys(); ++i )
				{
					FxAnimKey& key = _cachedAnimCurve->GetKeyM(i);
					key.SetUserData(NULL);
				}
				_cachedAnimCurve->SetUserData(NULL);
			}

			if( FxSessionProxy::IsAnimCurveSelected(_curve) )
			{
				FxSessionProxy::GetSelectedKeys(_cachedCurveIndices, _cachedKeyIndices);
			}
		}
		_mode = REMOVE;
	}
	
	if( _mode == ADD || (argList.GetArgument("-edit") && !argList.GetArgument("-add") && !argList.GetArgument("-remove")) )
	{
		FxString owner;
		if( argList.GetArgument("-owner", owner) )
		{
			if( !FxSessionProxy::GetCurveOwner(_group, _anim, _curve, _cachedOwner) && !(_mode & ADD) )
			{
				FAIL("Could not find specified group, anim or curve.");
			}

			if( owner == "user" || owner == "USER" || owner == "User" )
			{
				_requestedOwner = FxFalse;
			}
			else if( owner == "analysis" || owner == "ANALYSIS" || owner == "Analysis" )
			{
				_requestedOwner = FxTrue;
			}
			else
			{
				FAIL("-owner must be either \"user\" or \"analysis\"");
			}

			// We only want to do this if the operation is "-edit" only so that
			// we don't incorrectly mark -add
			if( !(_mode & ADD) )
			{
				_mode = static_cast<FxCurveCommandMode>(static_cast<FxInt32>(_mode) | EDITOWNER);
				_isUndoable = FxFalse;
				_shouldFlushUndo = FxTrue;
			}
		}

		FxString interp;
		if( argList.GetArgument("-interpolation", interp) )
		{
			if(! FxSessionProxy::GetCurveInterp(_group, _anim, _curve, _cachedInterp) && !(_mode & ADD) )
			{
				FAIL("Could not find specified group, anim or curve.");
			}

			if( interp == "linear" || interp == "Linear" || interp == "LINEAR" )
			{
				_requestedInterp = IT_Linear;
			}
			else if( interp == "hermite" || interp == "Hermite" || interp == "HERMITE" )
			{
				_requestedInterp = IT_Hermite;
			}
			else
			{
				FAIL("-interpolation must be either \"linear\" or \"hermite\"");
			}
			_mode = static_cast<FxCurveCommandMode>(static_cast<FxInt32>(_mode) | EDITINTERP);
		}
	}
	else if( _mode == NONE )
	{
		FAIL("Must specify -add, -remove, or -edit.");
	}

	// We've made it thus far - perform the action.
	_isValidOperation = FxTrue;
	return Redo();
}

FxCommandError FxCurveCommand::Undo( void )
{
	if( _mode & REMOVE )
	{
		if( _isValidOperation )
		{
			if( _cachedAnimCurve )
			{
				if( FxSessionProxy::AddCurve(_group, _anim, *_cachedAnimCurve, _oldIndex) )
				{
					FxSessionProxy::SetSelectedAnimCurves(_cachedSelectedCurves);

					if( _cachedCurveIndices.Length() )
					{
						FxSessionProxy::SetSelectedKeys(_cachedCurveIndices, _cachedKeyIndices);
					}
					return CE_Success;
				}
			}
			return CE_Failure;
		}
		else
		{
			FxString msg = "Animation <b>";
			msg += _anim;
			msg += "</b> in group <b>";
			msg += _group;
			msg += "</b> did not contain curve <b>";
			msg += _curve;
			msg += "</b> - skipping...";
			FxVM::DisplayWarning(msg);
		}
		return CE_Success;
	}

	FxBool goodResult = FxTrue;
	if( _mode & EDITINTERP )
	{
		goodResult &= FxSessionProxy::SetCurveInterp(_group, _anim, _curve, _cachedInterp);
	}
	if( goodResult && _mode & EDITOWNER )
	{
		goodResult &= FxSessionProxy::SetCurveOwner(_group, _anim, _curve, _cachedOwner);
	}
	if( goodResult && _mode & ADD )
	{
		if( _isValidOperation )
		{
			// Caching the old index isn't really needed here, but is required by
			// the proxy interface.  Since the command mode is add, this should be
			// harmless.
			goodResult &= FxSessionProxy::RemoveCurve(_group, _anim, _curve, _oldIndex);
		}
		else
		{
			FxString msg = "Animation <b>";
			msg += _anim;
			msg += "</b> in group <b>";
			msg += _group;
			msg += "</b> already contained curve <b>";
			msg += _curve;
			msg += "</b> - skipping...";
			FxVM::DisplayWarning(msg);
		}
	}

	return goodResult ? CE_Success : CE_Failure;
}

FxCommandError FxCurveCommand::Redo( void )
{
	if( _mode & REMOVE )
	{
		FxAnimCurve* pCurve = NULL;
		if( FxSessionProxy::GetCurve(_group, _anim, _curve, &pCurve) && pCurve )
		{
			if( FxSessionProxy::RemoveCurve(_group, _anim, _curve, _oldIndex) )
			{
				return CE_Success;
			}
			return CE_Failure;
		}
		else
		{
			_isValidOperation = FxFalse;
			FxString msg = "Animation <b>";
			msg += _anim;
			msg += "</b> in group <b>";
			msg += _group;
			msg += "</b> does not contain curve <b>";
			msg += _curve;
			msg += "</b> - skipping...";
			FxVM::DisplayWarning(msg);
		}
	}

	FxBool goodResult = FxTrue;
	if( _mode & ADD )
	{
		FxAnimCurve* pCurve = NULL;
		if( FxSessionProxy::GetCurve(_group, _anim, _curve, &pCurve) && pCurve )
		{
			_isValidOperation = FxFalse;
			FxString msg = "Animation <b>";
			msg += _anim;
			msg += "</b> in group <b>";
			msg += _group;
			msg += "</b> already contains curve <b>";
			msg += _curve;
			msg += "</b> - skipping...";
			FxVM::DisplayWarning(msg);
		}
		else
		{
			goodResult &= FxSessionProxy::AddCurve(_group, _anim, _curve);
		}
	}
	if( goodResult && _mode & EDITINTERP )
	{
		goodResult &= FxSessionProxy::SetCurveInterp(_group, _anim, _curve, _requestedInterp);
	}
	if( goodResult && _mode & EDITOWNER )
	{
		goodResult &= FxSessionProxy::SetCurveOwner(_group, _anim, _curve, _requestedOwner);
	}

	return goodResult ? CE_Success : CE_Failure;
}

} // namespace Face

} // namespace OC3Ent
