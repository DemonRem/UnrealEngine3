/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2003             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: SSE Function Library
 last mod: $Id: xmmlib.c,v 1.4 2005-07-08 15:00:00+09 blacksword Exp $

 ********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "xmmlib.h"

#if	defined(__SSE__)
_MM_ALIGN16 const unsigned long PCS_NNRR[4] = { 0x80000000, 0x80000000, 0x00000000, 0x00000000 };
_MM_ALIGN16 const unsigned long PCS_NRNR[4] = { 0x80000000, 0x00000000, 0x80000000, 0x00000000 };
_MM_ALIGN16 const unsigned long PCS_NRRN[4] = { 0x00000000, 0x80000000, 0x80000000, 0x00000000 };
_MM_ALIGN16 const unsigned long PCS_NRRR[4] = { 0x80000000, 0x80000000, 0x80000000, 0x00000000 };
_MM_ALIGN16 const unsigned long PCS_RNRN[4] = { 0x00000000, 0x80000000, 0x00000000, 0x80000000 };
_MM_ALIGN16 const unsigned long PCS_RRNN[4] = { 0x00000000, 0x00000000, 0x80000000, 0x80000000 };
_MM_ALIGN16 const unsigned long PCS_RNNR[4] = { 0x80000000, 0x00000000, 0x00000000, 0x80000000 };
_MM_ALIGN16 const unsigned long PCS_RRRR[4] = { 0x80000000, 0x80000000, 0x80000000, 0x80000000 };
_MM_ALIGN16 const unsigned long PCS_NNNR[4] = { 0x80000000, 0x00000000, 0x00000000, 0x00000000 };
_MM_ALIGN16 const unsigned long PABSMASK[4] = { 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF };

_MM_ALIGN16 const unsigned long PMASKTABLE[4 * 16] =
{
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000,
	0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0xFFFFFFFF, 0x00000000,
	0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
	0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF,
	0xFFFFFFFF, 0x00000000, 0x00000000, 0xFFFFFFFF,
	0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
	0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF,
	0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
};

const __m128 PFV_0    = { 0.0f, 0.0f, 0.0f, 0.0f };
const __m128 PFV_1    = { 1.0f, 1.0f, 1.0f, 1.0f };
const __m128 PFV_2    = { 2.0f, 2.0f, 2.0f, 2.0f };
const __m128 PFV_4    = { 4.0f, 4.0f, 4.0f, 4.0f };
const __m128 PFV_8    = { 8.0f, 8.0f, 8.0f, 8.0f };
const __m128 PFV_INIT = { 0.0f, 1.0f, 2.0f, 3.0f };
const __m128 PFV_0P5  = { 0.5f, 0.5f, 0.5f, 0.5f };
const __m128 PFV_M0P5 = {-0.5f,-0.5f,-0.5f,-0.5f };

#endif /* defined(__SSE__) */

const int bitCountTable[16] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };

/*---------------------------------------------------------------------------
// 16Byte Allignment calloc
//-------------------------------------------------------------------------*/
void* xmm_calloc( size_t nitems, size_t size )
{
	unsigned char* t_RetPtr = ( unsigned char* )_aligned_malloc( nitems * size, 16 );

	if( t_RetPtr )
	{
#ifdef	__SSE__
		size_t	i,j, k;
		__m128	XMM0, XMM1, XMM2, XMM3;
		unsigned char* Work = t_RetPtr;

		XMM0 = _mm_setzero_ps();
		XMM1 = _mm_setzero_ps();
		XMM2 = _mm_setzero_ps();
		XMM3 = _mm_setzero_ps();
		k = nitems * size;
		j = k & ( ~127 );

		for( i = 0; i < j; i += 128 )
		{
			_mm_stream_ps( ( float* )( Work ), XMM0 );
			_mm_stream_ps( ( float* )( Work + 16 ), XMM1 );
			_mm_stream_ps( ( float* )( Work + 32 ), XMM2 );
			_mm_stream_ps( ( float* )( Work + 48 ), XMM3 );

			_mm_stream_ps( ( float* )( Work + 64 ), XMM0 );
			_mm_stream_ps( ( float* )( Work + 80 ), XMM1 );
			_mm_stream_ps( ( float* )( Work + 96 ), XMM2 );
			_mm_stream_ps( ( float* )( Work + 112 ), XMM3 );

			Work += 128;
		}

		k &= 127;
		if( k & ( ~63 ) )
		{
			_mm_stream_ps( ( float* )( Work ), XMM0 );
			_mm_stream_ps( ( float* )( Work + 16 ), XMM1 );
			_mm_stream_ps( ( float* )( Work + 32 ), XMM2 );
			_mm_stream_ps( ( float* )( Work + 48 ), XMM3 );

			Work += 64;
		}

		k &= 63;
		if( k & ( ~31 ) )
		{
			_mm_stream_ps( ( float* )( Work ), XMM0 );
			_mm_stream_ps( ( float* )( Work + 16 ), XMM1 );

			Work += 32;
		}

		k &= 31;
		if( k & ( ~15 ) )
		{
			_mm_stream_ps( ( float* )( Work ), XMM2 );

			Work += 16;
		}

		k &= 15;
		memset( Work, 0, k );

		_mm_sfence();
#else
		memset( t_RetPtr, 0, nitems * size );
#endif
	}
	return(	( void* )t_RetPtr );
}

void* xmm_align( void* address )
{
	long work = ( long )address;
	work = ( work + 15 ) & ~15;
	address = ( void* )work;

	return( address );
}

void xmm_copy_forward( float* d, float* s, int count )
{
	int	i;

	for( i = 0; i < count; i += 32 )
	{
		__m128	XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7;

		XMM0 = _mm_load_ps( s );
		XMM1 = _mm_load_ps( s + 4 );
		XMM2 = _mm_load_ps( s + 8 );
		XMM3 = _mm_load_ps( s + 12 );
		XMM4 = _mm_load_ps( s + 16 );
		XMM5 = _mm_load_ps( s + 20 );
		XMM6 = _mm_load_ps( s + 24 );
		XMM7 = _mm_load_ps( s + 28 );
		_mm_store_ps( d, XMM0 );
		_mm_store_ps( d + 4, XMM1 );
		_mm_store_ps( d + 8, XMM2 );
		_mm_store_ps( d + 12, XMM3 );
		_mm_store_ps( d + 16, XMM4 );
		_mm_store_ps( d + 20, XMM5 );
		_mm_store_ps( d + 24, XMM6 );
		_mm_store_ps( d + 28, XMM7 );

		_mm_prefetch( ( const char* )( s + 64 ), _MM_HINT_NTA );
		s += 32;
		d += 32;
	}
}

void xmm_copy_backward( float* d, float* s, int count )
{
	int	i;

	for( i = 0; i < count; i += 32 )
	{
		__m128	XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7;

		XMM0 = _mm_load_ps( s - 32 );
		XMM1 = _mm_load_ps( s - 28 );
		XMM2 = _mm_load_ps( s - 24 );
		XMM3 = _mm_load_ps( s - 20 );
		XMM4 = _mm_load_ps( s - 16 );
		XMM5 = _mm_load_ps( s - 12 );
		XMM6 = _mm_load_ps( s - 8 );
		XMM7 = _mm_load_ps( s - 4 );
		_mm_store_ps( d - 32, XMM0 );
		_mm_store_ps( d - 28, XMM1 );
		_mm_store_ps( d - 24, XMM2 );
		_mm_store_ps( d - 20, XMM3 );
		_mm_store_ps( d - 16, XMM4 );
		_mm_store_ps( d - 12, XMM5 );
		_mm_store_ps( d - 8, XMM6 );
		_mm_store_ps( d - 4, XMM7 );

		_mm_prefetch( ( const char* )( s - 64 ), _MM_HINT_NTA );
		s -= 32;
		d -= 32;
	}
}
