//------------------------------------------------------------------------------
// Represents the transformations which may take place along links.
//
// Owner: Jamie Redmond
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

// Adding new link function types is easy:
// ---------------------------------------
// [1] Create a class for the new type.  Use the built-in functions as guides.
// [2] Increment FX_NUM_LINK_FUNCTIONS in FxLinkFn.cpp.
// [3] Initialize the new link function in FxLinkFn::Startup() in FxLinkFn.cpp
//     using the built-in functions as guides.

#ifndef FxLinkFn_H__
#define FxLinkFn_H__

#include "FxPlatform.h"
#include "FxNamedObject.h"
#include "FxArchive.h"
#include "FxArray.h"
#include "FxString.h"
#include "FxAnimCurve.h"

namespace OC3Ent
{

namespace Face
{

/// A name and default value for a FxLinkFnParameter.
/// \ingroup faceGraph
class FxLinkFnParameterDesc
{
public:
	FxLinkFnParameterDesc();
	FxLinkFnParameterDesc( const FxChar* name, FxReal defaultValue );
	FxLinkFnParameterDesc( const FxLinkFnParameterDesc& other );
	~FxLinkFnParameterDesc();

	FxName parameterName;
	FxReal parameterDefaultValue;

private:
	FxLinkFnParameterDesc& operator=( const FxLinkFnParameterDesc& other );
};

/// Macro to help define a link function.
/// \ingroup faceGraph
#define FX_DECLARE_LINKFN() \
	private: \
		FxString _fnDesc; \
		FxArray<FxLinkFnParameterDesc> _parameters; \
	public: \
		virtual const FxString& GetFnDesc( void ) const \
		{ \
			return _fnDesc; \
		} \
		virtual FxSize GetNumParams( void ) const \
		{ \
			return _parameters.Length(); \
		} \
		virtual const FxString& GetParamName( FxSize index ) const \
		{ \
			return _parameters[index].parameterName.GetAsString(); \
		} \
		virtual FxReal GetParamDefaultValue( FxSize index ) const \
		{ \
			return _parameters[index].parameterDefaultValue; \
		} 

/// The parameters to a FxLinkFn.
/// \ingroup faceGraph
class FxLinkFnParameters
{
public:
	FxLinkFnParameters();
	FxLinkFnParameters( const FxLinkFnParameters& other );
	FxLinkFnParameters& operator=( const FxLinkFnParameters& other );
	~FxLinkFnParameters();

	/// The parameter array.
	FxArray<FxReal> parameters;
	/// The custom curve.
	FxAnimCurve* pCustomCurve;

	/// Serializes the parameters to an archive.
	friend FxArchive& operator<<( FxArchive& arc, FxLinkFnParameters& key );
};

/// The function to modify a value as it flows through a FxFaceGraphNodeLink.
/// \ingroup faceGraph
class FxLinkFn : public FxNamedObject
{
public:
	/// Declare the class.
	FX_DECLARE_CLASS_NO_DYNCREATE_NO_SERIALIZE(FxLinkFn, FxNamedObject)
	/// Disable copy construction and assignment.
	FX_NO_COPY_OR_ASSIGNMENT(FxLinkFn)
	/// Declare it as a link function.
	FX_DECLARE_LINKFN()
	/// Constructor.
	FxLinkFn();
	/// Destructor.
	virtual ~FxLinkFn();

	/// Starts up the link function system.
	/// \note For internal use only.  Never call this directly.
	static void FX_CALL Startup( void );
	/// Shuts down the link function system.
	/// \note For internal use only.  Never call this directly.
	static void FX_CALL Shutdown( void );

	/// Evaluates the link function at x using the parameters and returns the
	/// result.
	virtual FxReal Evaluate( FxReal x, const FxLinkFnParameters& params ) const = 0;
	
	/// Returns the total number of link functions.
	static FxSize FX_CALL GetNumLinkFunctions( void );
	/// Returns the link function at index.
	static const FxLinkFn* FX_CALL GetLinkFunction( FxSize index );
	/// Find a specified link function.
	/// \param name The name of the link function to find.
	/// \return A pointer to the link function, or \p NULL if it was not found.
	static const FxLinkFn* FX_CALL FindLinkFunction( const FxName& name );
	/// Adds the specified link function.
	/// \param pLinkFn A pointer to the link function to add.
	static void FX_CALL AddLinkFunction( FxLinkFn* pLinkFn );

	/// Checks if the link function is the corrective link function.
	FX_INLINE const FxBool IsCorrective() const { return _isCorrective; }

protected:
	/// Whether or not the link function is the corrective link function.
	FxBool _isCorrective;

private:
	/// The list of all link functions in the system.
	static FxArray<FxLinkFn*>* _linkFnTable;
};

//------------------------------------------------------------------------------
// Null.
//------------------------------------------------------------------------------
/// The Null link function.  It is used to denote deprecated link functions and
/// always returns 0.0f in its Evaluate() function.
/// \ingroup faceGraph
class FxNullLinkFn : public FxLinkFn
{
public:
	/// Declare the class.
	FX_DECLARE_CLASS_NO_DYNCREATE_NO_SERIALIZE(FxNullLinkFn, FxLinkFn)
	/// Disable copy construction and assignment.
	FX_NO_COPY_OR_ASSIGNMENT(FxNullLinkFn)
	/// Declare it as a link function.
	FX_DECLARE_LINKFN()
	/// Constructor.  The Null link function must be supplied a name upon
	/// construction.  The name should be the name of the link function being
	/// deprecated.
	FxNullLinkFn( const FxChar* name );
	/// Destructor.
	virtual ~FxNullLinkFn();

	/// Always returns 0.0f.
	FX_INLINE FxReal Evaluate( FxReal FxUnused(x), const FxLinkFnParameters& FxUnused(params) ) const
	{
		return 0.0f;
	}
};

//------------------------------------------------------------------------------
// Linear.
//------------------------------------------------------------------------------
/// The linear link function.
/// \ingroup faceGraph
class FxLinearLinkFn : public FxLinkFn
{
public:
	/// Declare the class.
	FX_DECLARE_CLASS_NO_DYNCREATE_NO_SERIALIZE(FxLinearLinkFn, FxLinkFn)
	/// Disable copy construction and assignment.
	FX_NO_COPY_OR_ASSIGNMENT(FxLinearLinkFn)
	/// Declare it as a link function.
	FX_DECLARE_LINKFN()
	/// Constructor.
	FxLinearLinkFn();
	/// Destructor.
	virtual ~FxLinearLinkFn();

	/// Evaluates the linear function at x using the parameters and returns the
	/// result.
	FX_INLINE FxReal Evaluate( FxReal x, const FxLinkFnParameters& params ) const
	{
		if( params.parameters.Length() == 2 )
		{
			return x * params.parameters[0] + params.parameters[1];
		}
		else if( params.parameters.Length() == 1 )
		{
			return x * params.parameters[0];
		}
		else if( params.parameters.IsEmpty() )
		{
			return x;
		}
		return x;
	}
};

//------------------------------------------------------------------------------
// Quadratic.
//------------------------------------------------------------------------------
/// The quadratic link function.
/// \ingroup faceGraph
class FxQuadraticLinkFn : public FxLinkFn
{
public:
	/// Declare the class.
	FX_DECLARE_CLASS_NO_DYNCREATE_NO_SERIALIZE(FxQuadraticLinkFn, FxLinkFn)
	/// Disable copy construction and assignment.
	FX_NO_COPY_OR_ASSIGNMENT(FxQuadraticLinkFn)
	/// Declare it as a link function.
	FX_DECLARE_LINKFN()
	/// Constructor.
	FxQuadraticLinkFn();
	/// Destructor.
	virtual ~FxQuadraticLinkFn();

	/// Evaluates the quadratic function at x using the parameters and returns the
	/// result.
	FX_INLINE FxReal Evaluate( FxReal x, const FxLinkFnParameters& params ) const
	{
		if( params.parameters.Length() == 1 )
		{
			return (x<0.0f ? -1.0f : 1.0f)*(x*x) * params.parameters[0];
		}
		return (x<0.0f ? -1.0f : 1.0f)*(x*x);
	}
};

//------------------------------------------------------------------------------
// Cubic.
//------------------------------------------------------------------------------
/// The cubic link function.
/// \ingroup faceGraph
class FxCubicLinkFn : public FxLinkFn
{
public:
	/// Declare the class.
	FX_DECLARE_CLASS_NO_DYNCREATE_NO_SERIALIZE(FxCubicLinkFn, FxLinkFn)
	/// Disable copy construction and assignment.
	FX_NO_COPY_OR_ASSIGNMENT(FxCubicLinkFn)
	/// Declare it as a link function.
	FX_DECLARE_LINKFN()
	/// Constructor.
	FxCubicLinkFn();
	/// Destructor.
	virtual ~FxCubicLinkFn();

	/// Evaluates the cubic function at x using the parameters and returns the
	/// result.
	FX_INLINE FxReal Evaluate( FxReal x, const FxLinkFnParameters& params ) const
	{
		if( params.parameters.Length() == 1 )
		{
			return (x * x * x) * params.parameters[0];
		}
		return (x * x * x);
	}
};

//------------------------------------------------------------------------------
// Square Root.
//------------------------------------------------------------------------------
/// The square root link function.
/// \ingroup faceGraph
class FxSqrtLinkFn : public FxLinkFn
{
public:
	/// Declare the class.
	FX_DECLARE_CLASS_NO_DYNCREATE_NO_SERIALIZE(FxSqrtLinkFn, FxLinkFn)
	/// Disable copy construction and assignment.
	FX_NO_COPY_OR_ASSIGNMENT(FxSqrtLinkFn)
	/// Declare it as a link function.
	FX_DECLARE_LINKFN()
	/// Constructor.
	FxSqrtLinkFn();
	/// Destructor.
	virtual ~FxSqrtLinkFn();

	/// Evaluates the sqrt function at x using the parameters and returns the
	/// result.
	FX_INLINE FxReal Evaluate( FxReal x, const FxLinkFnParameters& params ) const
	{
		if( x == 0.0f ) return 0.0f;
		if( params.parameters.Length() == 1 )
		{
			return (x<0.0f ? -1.0f : 1.0f)*FxSqrt(FxAbs(x)) * params.parameters[0];
		}
		return (x<0.0f ? -1.0f : 1.0f)*FxSqrt(FxAbs(x));
	}
};

//------------------------------------------------------------------------------
// Negate.
//------------------------------------------------------------------------------
/// The negate link function.
/// \ingroup faceGraph
class FxNegateLinkFn : public FxLinkFn
{
public:
	/// Declare the class.
	FX_DECLARE_CLASS_NO_DYNCREATE_NO_SERIALIZE(FxNegateLinkFn, FxLinkFn)
	/// Disable copy construction and assignment.
	FX_NO_COPY_OR_ASSIGNMENT(FxNegateLinkFn)
	/// Declare it as a link function.
	FX_DECLARE_LINKFN()
	/// Constructor.
	FxNegateLinkFn();
	/// Destructor.
	virtual ~FxNegateLinkFn();

	/// Evaluates the cubic function at x using the parameters and returns the
	/// result.
	FX_INLINE FxReal Evaluate( FxReal x, const FxLinkFnParameters& FxUnused(params) ) const
	{
		return -x;
	}
};

//------------------------------------------------------------------------------
// Inverse.
//------------------------------------------------------------------------------
/// The inverse link function.
/// \ingroup faceGraph
class FxInverseLinkFn : public FxLinkFn
{
public:
	/// Declare the class.
	FX_DECLARE_CLASS_NO_DYNCREATE_NO_SERIALIZE(FxInverseLinkFn, FxLinkFn)
	/// Disable copy construction and assignment.
	FX_NO_COPY_OR_ASSIGNMENT(FxInverseLinkFn)
	/// Declare it as a link function.
	FX_DECLARE_LINKFN()
	/// Constructor.
	FxInverseLinkFn();
	/// Destructor.
	virtual ~FxInverseLinkFn();

	/// Evaluates the cubic function at x using the parameters and returns the
	/// result.
	FX_INLINE FxReal Evaluate( FxReal x, const FxLinkFnParameters& FxUnused(params) ) const
	{
		if( x != 0.0f )
		{
			return 1.0f / x;
		}
		return 0.0f;
	}
};

//------------------------------------------------------------------------------
// One Clamp.
//------------------------------------------------------------------------------
/// The one clamp link function.
/// The one clamp link function is a special link function used in the implementation
/// of our old expressions algorithm.  It returns 1 if the value of the node is
/// less than or equal to one, or (1/node value) if the node value is greater than one.
/// \ingroup faceGraph
class FxOneClampLinkFn : public FxLinkFn
{
public:
	/// Declare the class.
	FX_DECLARE_CLASS_NO_DYNCREATE_NO_SERIALIZE(FxOneClampLinkFn, FxLinkFn)
	/// Disable copy construction and assignment.
	FX_NO_COPY_OR_ASSIGNMENT(FxOneClampLinkFn)
	/// Declare it as a link function.
	FX_DECLARE_LINKFN()
	/// Constructor.
	FxOneClampLinkFn();
	/// Destructor.
	virtual ~FxOneClampLinkFn();

	/// Evaluates the cubic function at x using the parameters and returns the
	/// result.
	FX_INLINE FxReal Evaluate( FxReal x, const FxLinkFnParameters& FxUnused(params) ) const
	{
		return (x <= 1.0f) ? (1.0f) : (1.0f / x);
	}
};

//------------------------------------------------------------------------------
// Constant.
//------------------------------------------------------------------------------
/// The constant link function.
/// \ingroup faceGraph
class FxConstantLinkFn : public FxLinkFn
{
public:
	/// Declare the class.
	FX_DECLARE_CLASS_NO_DYNCREATE_NO_SERIALIZE(FxConstantLinkFn, FxLinkFn)
	/// Disable copy construction and assignment.
	FX_NO_COPY_OR_ASSIGNMENT(FxConstantLinkFn)
	/// Declare it as a link function.
	FX_DECLARE_LINKFN()
	/// Constructor.
	FxConstantLinkFn();
	/// Destructor.
	virtual ~FxConstantLinkFn();

	/// Evaluates the cubic function at x using the parameters and returns the
	/// result.
	FX_INLINE FxReal Evaluate( FxReal FxUnused(x), const FxLinkFnParameters& params ) const
	{
		if( params.parameters.Length() > 0 )
		{
			return params.parameters[0];
		}
		return 1.0f;
	}
};

//------------------------------------------------------------------------------
// Corrective.
//------------------------------------------------------------------------------
/// The corrective link function.
/// \ingroup faceGraph
class FxCorrectiveLinkFn : public FxLinkFn
{
public:
	/// Declare the class.
	FX_DECLARE_CLASS_NO_DYNCREATE_NO_SERIALIZE(FxCorrectiveLinkFn, FxLinkFn)
	/// Disable copy construction and assignment.
	FX_NO_COPY_OR_ASSIGNMENT(FxCorrectiveLinkFn)
	/// Declare it as a link function.
	FX_DECLARE_LINKFN()
	/// Constructor.
	FxCorrectiveLinkFn();
	/// Destructor.
	virtual ~FxCorrectiveLinkFn();

	/// Evaluates the cubic function at x using the parameters and returns the
	/// result.
	FX_INLINE FxReal Evaluate( FxReal FxUnused(x), const FxLinkFnParameters& params ) const
	{
		// This does nothing on purpose.  We need more information than we have at
		// this level to compute the corrective effect.
		if( params.parameters.Length() > 0 )
		{
			return params.parameters[0];
		}
		return 0.0f;
	}
};

//------------------------------------------------------------------------------
// Custom.
//------------------------------------------------------------------------------
/// The custom link function.
/// \ingroup faceGraph
class FxCustomLinkFn : public FxLinkFn
{
public:
	/// Declare the class.
	FX_DECLARE_CLASS_NO_DYNCREATE_NO_SERIALIZE(FxCustomLinkFn, FxLinkFn)
	/// Disable copy construction and assignment.
	FX_NO_COPY_OR_ASSIGNMENT(FxCustomLinkFn)
	/// Declare it as a link function.
	FX_DECLARE_LINKFN()
	/// Constructor.
	FxCustomLinkFn();
	/// Destructor.
	virtual ~FxCustomLinkFn();

	/// Evaluates the cubic function at x using the parameters and returns the
	/// result.
	FX_INLINE FxReal Evaluate( FxReal x, const FxLinkFnParameters& params ) const
	{
		if( params.pCustomCurve )
		{
			return params.pCustomCurve->EvaluateAt(x);
		}
		return 0.0f;
	}
};

//------------------------------------------------------------------------------
// ClampedLinear
//------------------------------------------------------------------------------
/// The clamped linear link function.
/// \ingroup faceGraph
class FxClampedLinearLinkFn : public FxLinkFn
{
public:
	/// Declare the class.
	FX_DECLARE_CLASS_NO_DYNCREATE_NO_SERIALIZE(FxClampedLinearLinkFn, FxLinkFn)
	/// Disable copy construction and assignment.
	FX_NO_COPY_OR_ASSIGNMENT(FxClampedLinearLinkFn)
	/// Declare it as a link function.
	FX_DECLARE_LINKFN()
	/// Constructor.
	FxClampedLinearLinkFn();
	/// Destructor.
	virtual ~FxClampedLinearLinkFn();

	#define FX_SLOPE params.parameters[0]
	#define FX_CLAMP_X params.parameters[1]
	#define FX_CLAMP_Y params.parameters[2]
	#define FX_CLAMP_DIR params.parameters[3]
	/// Evaluates the clamped linear function for x.
	FX_INLINE FxReal Evaluate( FxReal x, const FxLinkFnParameters& params ) const
	{
		if( params.parameters.Length() == 4 )
		{
			if( FX_CLAMP_DIR > 0.0f && x > FX_CLAMP_X ||
				FX_CLAMP_DIR <= 0.0f && x < FX_CLAMP_X )
			{
				return x * FX_SLOPE + (-1.0f * FX_SLOPE * FX_CLAMP_X + FX_CLAMP_Y);
			}
			return FX_CLAMP_Y;
		}
		else if( params.parameters.Length() == 1 )
		{
			if( x > 0.0f )
			{
				return x * FX_SLOPE;
			}
			return 0.0f;
		}
		return x;
	}
	#undef FX_SLOPE
	#undef FX_CLAMP_X
	#undef FX_CLAMP_Y
	#undef FX_CLAMP_DIR
};

//------------------------------------------------------------------------------
// Add new link function types here.
//------------------------------------------------------------------------------

} // namespace Face

} // namespace OC3Ent

#endif
