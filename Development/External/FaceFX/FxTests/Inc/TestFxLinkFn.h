#ifndef TestFxLinkFn_H__
#define TestFxLinkFn_H__

#include "FxLinkFn.h"

UNITTEST( FxLinkFn, Evaluate )
{
	const FxLinkFn* testLinearFn = FxLinkFn::FindLinkFunction( "linear" );
	CHECK( testLinearFn );
	FxLinkFnParameters params;
	CHECK( FLOAT_EQUALITY( testLinearFn->Evaluate( 0.0f, params ), 0.0f ) );
	CHECK( FLOAT_EQUALITY( testLinearFn->Evaluate( 0.5f, params ), 0.5f ) );
	CHECK( FLOAT_EQUALITY( testLinearFn->Evaluate( 1.0f, params ), 1.0f ) );

	const FxLinkFn* testCubicFn = FxLinkFn::FindLinkFunction( "cubic" );
	CHECK( testCubicFn );
	CHECK( FLOAT_EQUALITY( testCubicFn->Evaluate( 0.0f, params ), 0.0f ) );
	CHECK( FLOAT_EQUALITY( testCubicFn->Evaluate( 0.5f, params ), 0.125f ) );
	CHECK( FLOAT_EQUALITY( testCubicFn->Evaluate( 1.0f, params ), 1.0f ) );

	FxLinkFnParameters params2;
	params2.parameters.PushBack( 2.0f );
	CHECK( FLOAT_EQUALITY( testLinearFn->Evaluate( 0.0f, params2 ), 0.0f ) );
	CHECK( FLOAT_EQUALITY( testLinearFn->Evaluate( 0.5f, params2 ), 1.0f ) );
	CHECK( FLOAT_EQUALITY( testLinearFn->Evaluate( 1.0f, params2 ), 2.0f ) );

	CHECK( FLOAT_EQUALITY( testCubicFn->Evaluate( 0.0f, params2 ), 0.0f ) );
	CHECK( FLOAT_EQUALITY( testCubicFn->Evaluate( 0.5f, params2 ), 0.25f ) );
	CHECK( FLOAT_EQUALITY( testCubicFn->Evaluate( 1.0f, params2 ), 2.0f ) );

	const FxLinkFn* testNegateFn = FxLinkFn::FindLinkFunction( "negate" );
	CHECK( testNegateFn );
	FxLinkFnParameters negateParams;
	negateParams.parameters.PushBack( 1.0f );
	CHECK( FLOAT_EQUALITY( testNegateFn->Evaluate( 1.0f, negateParams ), -1.0f ) );
	FxLinkFnParameters negateParams2;
	negateParams2.parameters.PushBack( -1.0f );
	CHECK( FLOAT_EQUALITY( testNegateFn->Evaluate( -1.0f, negateParams2 ), 1.0f ) );
	FxLinkFnParameters negateParams3;
	negateParams3.parameters.PushBack( 0.0f );
	CHECK( FLOAT_EQUALITY( testNegateFn->Evaluate( 0.0f, negateParams3 ), 0.0f ) );

	const FxLinkFn* testInverseFn = FxLinkFn::FindLinkFunction( "inverse" );
	CHECK( testInverseFn );
	FxLinkFnParameters inverseParams;
	inverseParams.parameters.PushBack( 0.0f );
	CHECK( FLOAT_EQUALITY( testInverseFn->Evaluate( 0.0, inverseParams ), 0.0f ) );
	FxLinkFnParameters inverseParams2;
	inverseParams2.parameters.PushBack( 1.0f );
	CHECK( FLOAT_EQUALITY( testInverseFn->Evaluate( 1.0, inverseParams2 ), (1.0f / 1.0f) ) );
	FxLinkFnParameters inverseParams3;
	inverseParams3.parameters.PushBack( -1.0f );
	CHECK( FLOAT_EQUALITY( testInverseFn->Evaluate( -1.0, inverseParams3 ), (1.0f / -1.0f) ) );

	const FxLinkFn* testOneClampFn = FxLinkFn::FindLinkFunction( "oneclamp" );
	CHECK( testOneClampFn );
	FxLinkFnParameters oneClampParams;
	oneClampParams.parameters.PushBack( 0.0f );
	CHECK( FLOAT_EQUALITY( testOneClampFn->Evaluate( 0.0f, oneClampParams ), (1.0f / 1.0f) ) );
	FxLinkFnParameters oneClampParams2;
	oneClampParams2.parameters.PushBack( -1.0f );
	CHECK( FLOAT_EQUALITY( testOneClampFn->Evaluate( -1.0f, oneClampParams2 ), (1.0f / 1.0f) ) );
	FxLinkFnParameters oneClampParams3;
	oneClampParams3.parameters.PushBack( 1.0f );
	CHECK( FLOAT_EQUALITY( testOneClampFn->Evaluate( 1.0f, oneClampParams3 ), (1.0f / 1.0f) ) );
	FxLinkFnParameters oneClampParams4;
	oneClampParams4.parameters.PushBack( 2.0f );
	CHECK( FLOAT_EQUALITY( testOneClampFn->Evaluate( 2.0f, oneClampParams4 ), (1.0f / 2.0f) ) );

	const FxLinkFn* constLinkFn = FxLinkFn::FindLinkFunction( "constant" );
	CHECK( constLinkFn );
	FxLinkFnParameters constParams;
	constParams.parameters.PushBack( 3.0f );
	CHECK( FLOAT_EQUALITY( constLinkFn->Evaluate( -1.0f, constParams ), 3.0f ) );
	CHECK( FLOAT_EQUALITY( constLinkFn->Evaluate( 0.0f, constParams ), 3.0f ) );
	CHECK( FLOAT_EQUALITY( constLinkFn->Evaluate( 1.0f, constParams ), 3.0f ) );
	CHECK( FLOAT_EQUALITY( constLinkFn->Evaluate( 10.0f, constParams ), 3.0f ) );
}

#endif