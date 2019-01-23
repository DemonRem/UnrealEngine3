//------------------------------------------------------------------------------
// Represents the transformations which may take place along links.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#include "FxLinkFn.h"
#include "FxMath.h"
#include "FxUtil.h"

namespace OC3Ent
{

namespace Face
{

FxLinkFnParameterDesc::FxLinkFnParameterDesc()
	: parameterDefaultValue(0.0f)
{
}

FxLinkFnParameterDesc::
FxLinkFnParameterDesc( const FxChar* name, FxReal defaultValue )
	: parameterName(name)
	, parameterDefaultValue(defaultValue)
{
}

FxLinkFnParameterDesc::
FxLinkFnParameterDesc( const FxLinkFnParameterDesc& other )
	: parameterName(other.parameterName)
	, parameterDefaultValue(other.parameterDefaultValue)
{
}

FxLinkFnParameterDesc::~FxLinkFnParameterDesc()
{
}

#define kCurrentFxLinkFnParametersVersion 0

FxLinkFnParameters::FxLinkFnParameters()
	: pCustomCurve(NULL)
{
}

FxLinkFnParameters::FxLinkFnParameters( const FxLinkFnParameters& other )
	: parameters(other.parameters)
	, pCustomCurve(NULL)
{
	if( other.pCustomCurve )
	{
		pCustomCurve = new FxAnimCurve(*other.pCustomCurve);
	}
}

FxLinkFnParameters& FxLinkFnParameters::operator=( const FxLinkFnParameters& other )
{
	if( this == &other ) return *this;
	
	parameters = other.parameters;
	if( pCustomCurve )
	{
		delete pCustomCurve;
		pCustomCurve = NULL;
	}
	if( other.pCustomCurve )
	{
		pCustomCurve = new FxAnimCurve(*other.pCustomCurve);
	}

	return *this;
}

FxLinkFnParameters::~FxLinkFnParameters()
{
	if( pCustomCurve )
	{
		delete pCustomCurve;
	}
}

FxArchive& operator<<( FxArchive& arc, FxLinkFnParameters& linkFnParams )
{
	FxUInt16 version = kCurrentFxLinkFnParametersVersion;
	arc << version;
	arc << linkFnParams.parameters << linkFnParams.pCustomCurve;
	return arc;
}

#define kCurrentFxLinkFnVersion 0

FX_IMPLEMENT_CLASS(FxLinkFn, kCurrentFxLinkFnVersion, FxNamedObject)

FxArray<FxLinkFn*>* FxLinkFn::_linkFnTable = NULL;

FxLinkFn::FxLinkFn()
{
	_isCorrective = FxFalse;
}

FxLinkFn::~FxLinkFn()
{
}

// Update this when adding new link function types.
#define FX_NUM_LINK_FUNCTIONS 12
void FX_CALL FxLinkFn::Startup( void )
{
	if( !_linkFnTable )
	{
		FxLinkFn::StaticClass();

		_linkFnTable = static_cast<FxArray<FxLinkFn*>*>(FxAlloc(sizeof(FxArray<FxLinkFn*>), "LinkFn List"));
		FxDefaultConstruct(_linkFnTable);
		_linkFnTable->Reserve(FX_NUM_LINK_FUNCTIONS);

//------------------------------------------------------------------------------
// Initialize all link functions here.
//------------------------------------------------------------------------------
		FxNullLinkFn::StaticClass();

		FxLinearLinkFn::StaticClass();
		FxLinearLinkFn* linearLinkFn = new FxLinearLinkFn();
		FxLinkFn::AddLinkFunction(linearLinkFn);

		FxQuadraticLinkFn::StaticClass();
		FxQuadraticLinkFn* quadraticLinkFn = new FxQuadraticLinkFn();
		FxLinkFn::AddLinkFunction(quadraticLinkFn);

		FxCubicLinkFn::StaticClass();
		FxCubicLinkFn* cubicLinkFn = new FxCubicLinkFn();
		FxLinkFn::AddLinkFunction(cubicLinkFn);

		FxSqrtLinkFn::StaticClass();
		FxSqrtLinkFn* sqrtLinkFn = new FxSqrtLinkFn();
		FxLinkFn::AddLinkFunction(sqrtLinkFn);

		FxNegateLinkFn::StaticClass();
		FxNegateLinkFn* negateLinkFn = new FxNegateLinkFn();
		FxLinkFn::AddLinkFunction(negateLinkFn);

		FxInverseLinkFn::StaticClass();
		FxInverseLinkFn* inverseLinkFn = new FxInverseLinkFn();
		FxLinkFn::AddLinkFunction(inverseLinkFn);

		FxOneClampLinkFn::StaticClass();
		FxOneClampLinkFn* oneClampLinkFn = new FxOneClampLinkFn();
		FxLinkFn::AddLinkFunction(oneClampLinkFn);

		FxConstantLinkFn::StaticClass();
		FxConstantLinkFn* constantLinkFn = new FxConstantLinkFn();
		FxLinkFn::AddLinkFunction(constantLinkFn);

		FxCorrectiveLinkFn::StaticClass();
		FxCorrectiveLinkFn* correctiveLinkFn = new FxCorrectiveLinkFn();
		FxLinkFn::AddLinkFunction(correctiveLinkFn);

		FxCustomLinkFn::StaticClass();
		FxCustomLinkFn* customLinkFn = new FxCustomLinkFn();
		FxLinkFn::AddLinkFunction(customLinkFn);

		// Deprecated the Perlin Noise link function.  In FaceFX 1.5+, all
		// Perlin Noise link functions are re-routed through the "null" link
		// function.
		FxNullLinkFn* deprecatedPerlinNoiseLinkFn = new FxNullLinkFn("perlin noise");
		FxLinkFn::AddLinkFunction(deprecatedPerlinNoiseLinkFn);

		FxClampedLinearLinkFn::StaticClass();
		FxClampedLinearLinkFn* clampedLinearLinkFn = new FxClampedLinearLinkFn();
		FxLinkFn::AddLinkFunction(clampedLinearLinkFn);
	}
}

void FX_CALL FxLinkFn::Shutdown( void )
{
	if( _linkFnTable )
	{
		// Delete all link functions in the link function table.
		FxSize numLinkFns = _linkFnTable->Length();
		for( FxSize i = 0; i < numLinkFns; ++i )
		{
			delete (*_linkFnTable)[i];
		}
		_linkFnTable->Clear();
		FxDelete(_linkFnTable);
        _linkFnTable = NULL;
	}
}

FxSize FX_CALL FxLinkFn::GetNumLinkFunctions( void )
{
	if( _linkFnTable )
	{
		return _linkFnTable->Length();
	}
	return 0;
}

const FxLinkFn* FX_CALL FxLinkFn::GetLinkFunction( FxSize index )
{
	if( _linkFnTable )
	{
		return (*_linkFnTable)[index];
	}
	return NULL;
}

const FxLinkFn* FX_CALL FxLinkFn::FindLinkFunction( const FxName& name )
{
	if( _linkFnTable )
	{
		FxSize numLinkFns = _linkFnTable->Length();
		for( FxSize i = 0; i < numLinkFns; ++i )
		{
			if( (*_linkFnTable)[i]->GetName() == name )
			{
				return (*_linkFnTable)[i];
			}
		}
	}
	return NULL;
}

void FX_CALL FxLinkFn::AddLinkFunction( FxLinkFn* pLinkFn )
{
	if( pLinkFn )
	{
		if( !_linkFnTable )
		{
			FxAssert(!"Attempt to add FxLinkFn before FxLinkFn::Startup() called!");
		}
		const FxLinkFn* duplicateFn = FindLinkFunction(pLinkFn->GetName());
		if( duplicateFn != NULL )
		{
			FxAssert(!"Attempt to add duplicate FxLinkFn!");
		}
		_linkFnTable->PushBack(pLinkFn);
	}
}

//------------------------------------------------------------------------------
// Null.
//------------------------------------------------------------------------------
FX_IMPLEMENT_CLASS(FxNullLinkFn, kCurrentFxLinkFnVersion, FxLinkFn)

FxNullLinkFn::FxNullLinkFn( const FxChar* name )
	: _fnDesc(" This link function type has been deprecated. ")
{
	SetName(name);
}

FxNullLinkFn::~FxNullLinkFn()
{
}

//------------------------------------------------------------------------------
// Linear.
//------------------------------------------------------------------------------
FX_IMPLEMENT_CLASS(FxLinearLinkFn, kCurrentFxLinkFnVersion, FxLinkFn)

FxLinearLinkFn::FxLinearLinkFn()
	: _fnDesc(" y = m * x + b ")
{
	SetName("linear");
	_parameters.Reserve(2);
	_parameters.PushBack(FxLinkFnParameterDesc("m", 1.0f));
	_parameters.PushBack(FxLinkFnParameterDesc("b", 0.0f));
}

FxLinearLinkFn::~FxLinearLinkFn()
{
}

//------------------------------------------------------------------------------
// Quadratic.
//------------------------------------------------------------------------------
FX_IMPLEMENT_CLASS(FxQuadraticLinkFn, kCurrentFxLinkFnVersion, FxLinkFn)

FxQuadraticLinkFn::FxQuadraticLinkFn()
	: _fnDesc(" y = a * x^2 ")
{
	SetName("quadratic");
	_parameters.PushBack(FxLinkFnParameterDesc("a", 1.0f));
}

FxQuadraticLinkFn::~FxQuadraticLinkFn()
{
}

//------------------------------------------------------------------------------
// Cubic.
//------------------------------------------------------------------------------
FX_IMPLEMENT_CLASS(FxCubicLinkFn, kCurrentFxLinkFnVersion, FxLinkFn)

FxCubicLinkFn::FxCubicLinkFn()
	: _fnDesc(" y = a * x^3 ")
{
	SetName("cubic");
	_parameters.PushBack(FxLinkFnParameterDesc("a", 1.0f));
}

FxCubicLinkFn::~FxCubicLinkFn()
{
}

//------------------------------------------------------------------------------
// Square Root.
//------------------------------------------------------------------------------
FX_IMPLEMENT_CLASS(FxSqrtLinkFn, kCurrentFxLinkFnVersion, FxLinkFn)

FxSqrtLinkFn::FxSqrtLinkFn()
	: _fnDesc(" y = a * sqrt(x) ")
{
	SetName("square root");
	_parameters.PushBack(FxLinkFnParameterDesc("a", 1.0f));
}

FxSqrtLinkFn::~FxSqrtLinkFn()
{
}

//------------------------------------------------------------------------------
// Negate.
//------------------------------------------------------------------------------
FX_IMPLEMENT_CLASS(FxNegateLinkFn, kCurrentFxLinkFnVersion, FxLinkFn)

FxNegateLinkFn::FxNegateLinkFn()
	: _fnDesc(" y = -x ")
{
	SetName("negate");
}

FxNegateLinkFn::~FxNegateLinkFn()
{
}

//------------------------------------------------------------------------------
// Inverse.
//------------------------------------------------------------------------------
FX_IMPLEMENT_CLASS(FxInverseLinkFn, kCurrentFxLinkFnVersion, FxLinkFn)

FxInverseLinkFn::FxInverseLinkFn()
	: _fnDesc(" y = 1 / x ")
{
	SetName("inverse");
}

FxInverseLinkFn::~FxInverseLinkFn()
{
}

//------------------------------------------------------------------------------
// One Clamp.
//------------------------------------------------------------------------------
FX_IMPLEMENT_CLASS(FxOneClampLinkFn, kCurrentFxLinkFnVersion, FxLinkFn)

FxOneClampLinkFn::FxOneClampLinkFn()
	: _fnDesc(" y = [1 if x <= 1] or [1 / x if x > 1) ")
{
	SetName("oneclamp");
}

FxOneClampLinkFn::~FxOneClampLinkFn()
{
}

//------------------------------------------------------------------------------
// Constant.
//------------------------------------------------------------------------------
FX_IMPLEMENT_CLASS(FxConstantLinkFn, kCurrentFxLinkFnVersion, FxLinkFn)

FxConstantLinkFn::FxConstantLinkFn()
	: _fnDesc(" y = c ")
{
	SetName("constant");
	_parameters.PushBack(FxLinkFnParameterDesc("c", 1.0f));
}

FxConstantLinkFn::~FxConstantLinkFn()
{
}

//------------------------------------------------------------------------------
// Corrective.
//------------------------------------------------------------------------------
FX_IMPLEMENT_CLASS(FxCorrectiveLinkFn, kCurrentFxLinkFnVersion, FxLinkFn)

FxCorrectiveLinkFn::FxCorrectiveLinkFn()
	: _fnDesc(" x corrects for y ")	
{
	SetName("corrective");
	_parameters.PushBack(FxLinkFnParameterDesc("Correction Factor", 1.0f));
	_isCorrective = FxTrue;
}

FxCorrectiveLinkFn::~FxCorrectiveLinkFn()
{
}

//------------------------------------------------------------------------------
// Custom.
//------------------------------------------------------------------------------
FX_IMPLEMENT_CLASS(FxCustomLinkFn, kCurrentFxLinkFnVersion, FxLinkFn)

FxCustomLinkFn::FxCustomLinkFn()
	: _fnDesc(" y = custom_function(x) ")	
{
	SetName("custom");
}

FxCustomLinkFn::~FxCustomLinkFn()
{
}

//------------------------------------------------------------------------------
// Clamped Linear.
//------------------------------------------------------------------------------
FX_IMPLEMENT_CLASS(FxClampedLinearLinkFn, kCurrentFxLinkFnVersion, FxLinkFn)

FxClampedLinearLinkFn::FxClampedLinearLinkFn()
	: _fnDesc(" y = m * x ")
{
	SetName("clamped linear");
	_parameters.Reserve(4);
	_parameters.PushBack(FxLinkFnParameterDesc("m", 1.0f));
	_parameters.PushBack(FxLinkFnParameterDesc("clampx", 0.0f));
	_parameters.PushBack(FxLinkFnParameterDesc("clampy", 0.0f));
	_parameters.PushBack(FxLinkFnParameterDesc("clampdir", 1.0f));
}

FxClampedLinearLinkFn::~FxClampedLinearLinkFn()
{
}

//------------------------------------------------------------------------------
// Add new link function types here.
//------------------------------------------------------------------------------

} // namespace Face

} // namespace OC3Ent
