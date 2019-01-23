/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2007             *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: linear scale -> dB, Bark and Mel scales

 ********************************************************************/

#ifndef _V_SCALES_H_
#define _V_SCALES_H_

#include <math.h>
#include "os.h"

/* 20log10(x) */
#define VORBIS_IEEE_FLOAT32 1
#ifdef VORBIS_IEEE_FLOAT32

STIN float unitnorm(float x)
{
	union 
	{
		ogg_uint32_t i;
		float f;
	} ix;

	ix.f = x;
	ix.i = (ix.i & 0x80000000U) | (0x3f800000U);
	return( ix.f );
}

/* Segher was off (too high) by ~ .3 decibel.  Center the conversion correctly. */
STIN float todB(const float* x)
{
	union 
	{
		ogg_uint32_t i;
		float f;
	} ix;

	ix.f = *x;
	ix.i = ix.i & 0x7fffffff;
	return( ( float )( ix.i * 7.17711438e-7f - 764.6161886f ) );
}

#define todB_nn( x ) todB( x )

#else

static float unitnorm( float x )
{
	if( x < 0.0f )
	{
		return( -1.0f );
	}
	return( 1.0f );
}

#define todB( x )		( *( x ) == 0.0f ? -400.0f : log( *( x ) * *( x ) ) * 4.34294480f )
#define todB_nn( x )	( *( x ) == 0.0f ? -400.0f : log( *( x ) ) * 8.6858896f )

#endif 

#define fromdB( x ) ( ( float )( exp( ( x ) * 0.11512925f ) ) ) 

/* The bark scale equations are approximations, since the original table was somewhat hand rolled.  The below are chosen to have the
   best possible fit to the rolled tables, thus their somewhat odd appearance (these are more accurate and over a longer range than
   the oft-quoted bark equations found in the texts I have).  The approximations are valid from 0 - 30kHz (nyquist) or so.
   all f in Hz, z in Bark */

#define toBARK( n )		( 13.1f * atanf( 0.00074f * ( n ) ) + 2.24f * atanf( ( n ) * ( n ) * 1.85e-8f ) + 1e-4f * ( n ) )
#define fromBARK( z )	( 102.0f * ( z ) - 2.0f * powf( z, 2.0f ) + 0.4f * powf( z, 3.0f ) + powf( 1.46f, z ) - 1.0f )
#define toMEL( n )		( ( float )( log( 1.0f + ( n ) * 0.001f ) * 1442.695f ) )
#define fromMEL( m )	( ( float )( 1000.0f * exp( ( m ) / 1442.695f ) - 1000.0f ) )

/* Frequency to octave.  We arbitrarily declare 63.5 Hz to be octave 0.0 */
/* Note: These must be log and exp, not logf and expf for proper operation on x64 */
#define toOC( n )		( ( float )( log( n ) * 1.442695f - 5.965784f ) )
#define fromOC( o )		( ( float )( exp( ( ( o ) + 5.965784f ) * 0.693147f ) ) )

#endif

