/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2002             *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: toplevel libogg include
 last mod: $Id: ogg.h 7188 2004-07-20 07:26:04Z xiphmont $

 ********************************************************************/
#ifndef _OGG_H
#define _OGG_H

#ifdef __cplusplus
extern "C" {
#endif


#ifdef OGG_DLL
	#define OGG_DLL_FUNCTION __declspec(dllexport)
#else
	#define OGG_DLL_FUNCTION __declspec(dllimport)
#endif /* OGG_DLL */


#include <ogg/os_types.h>

typedef struct 
{
	long			endbyte;
	int				endbit;

	unsigned char*	buffer;
	unsigned char*	ptr;
	long			storage;
} oggpack_buffer;

/* ogg_page is used to encapsulate the data in one Ogg bitstream page *****/

typedef struct 
{
	unsigned char*	header;
	long			header_len;
	unsigned char*	body;
	long			body_len;
} ogg_page;

/* ogg_stream_state contains the current encode/decode state of a logical
   Ogg bitstream **********************************************************/

typedef struct 
{
	unsigned char*	body_data;				/* bytes from packet bodies */
	long			body_storage;			/* storage elements allocated */
	long			body_fill;				/* elements stored; fill mark */
	long			body_returned;			/* elements of fill returned */

	int*			lacing_vals;			/* The values that will go to the segment table */
	ogg_int64_t*	granule_vals;			/* granulepos values for headers. Not compact this way, but it is simple coupled to the	lacing fifo */
	long			lacing_storage;
	long			lacing_fill;
	long			lacing_packet;
	long			lacing_returned;

	unsigned char	header[282];			/* working space for header encode */
	int				header_fill;

	int				e_o_s;					/* set when we have buffered the last packet in the logical bitstream */
	int				b_o_s;					/* set after we've written the initial page of a logical bitstream */
	long			serialno;
	long			pageno;
	ogg_int64_t		packetno;				/* sequence number for decode; the framing knows where there's a hole in the data,
											   but we need coupling so that the codec (which is in a seperate abstraction
											   layer) also knows about the gap */
	ogg_int64_t		granulepos;
} ogg_stream_state;

/* ogg_packet is used to encapsulate the data and metadata belonging
   to a single raw Ogg/Vorbis packet *************************************/

typedef struct 
{
	unsigned char*	packet;
	long			bytes;
	long			b_o_s;
	long			e_o_s;

	ogg_int64_t		granulepos;

	ogg_int64_t		packetno;				/* sequence number for decode; the framing knows where there's a hole in the data,
											   but we need coupling so that the codec (which is in a seperate abstraction
											   layer) also knows about the gap */
} ogg_packet;

typedef struct 
{
	unsigned char*	data;
	int				storage;
	int				fill;
	int				returned;

	int				unsynced;
	int				headerbytes;
	int				bodybytes;
} ogg_sync_state;

/* Ogg BITSTREAM PRIMITIVES: bitstream ************************/

OGG_DLL_FUNCTION void oggpack_writeinit( oggpack_buffer* b );
OGG_DLL_FUNCTION void oggpack_writetrunc( oggpack_buffer* b, long bits );
OGG_DLL_FUNCTION void oggpack_writealign( oggpack_buffer* b );
OGG_DLL_FUNCTION void oggpack_writecopy( oggpack_buffer* b, void* source, long bits );
OGG_DLL_FUNCTION void oggpack_reset( oggpack_buffer* b );
OGG_DLL_FUNCTION void oggpack_writeclear( oggpack_buffer* b );
OGG_DLL_FUNCTION void oggpack_readinit( oggpack_buffer* b, unsigned char* buf, int bytes );
OGG_DLL_FUNCTION void oggpack_write( oggpack_buffer* b, unsigned long value, int bits );
OGG_DLL_FUNCTION long oggpack_look( oggpack_buffer* b, int bits );
OGG_DLL_FUNCTION long oggpack_look1( oggpack_buffer* b );
OGG_DLL_FUNCTION void oggpack_adv( oggpack_buffer* b, int bits );
OGG_DLL_FUNCTION void oggpack_adv1( oggpack_buffer* b );
OGG_DLL_FUNCTION long oggpack_read( oggpack_buffer* b, int bits );
OGG_DLL_FUNCTION long oggpack_read1( oggpack_buffer* b );
OGG_DLL_FUNCTION long oggpack_bytes( oggpack_buffer* b );
OGG_DLL_FUNCTION long oggpack_bits( oggpack_buffer* b );
OGG_DLL_FUNCTION unsigned char* oggpack_get_buffer( oggpack_buffer* b );

OGG_DLL_FUNCTION void oggpackB_writeinit( oggpack_buffer* b );
OGG_DLL_FUNCTION void oggpackB_writetrunc( oggpack_buffer* b, long bits );
OGG_DLL_FUNCTION void oggpackB_writealign( oggpack_buffer* b );
OGG_DLL_FUNCTION void oggpackB_writecopy( oggpack_buffer* b, void* source, long bits );
OGG_DLL_FUNCTION void oggpackB_reset( oggpack_buffer* b );
OGG_DLL_FUNCTION void oggpackB_writeclear( oggpack_buffer* b );
OGG_DLL_FUNCTION void oggpackB_readinit( oggpack_buffer* b, unsigned char* buf, int bytes );
OGG_DLL_FUNCTION void oggpackB_write( oggpack_buffer* b, unsigned long value, int bits );
OGG_DLL_FUNCTION long oggpackB_look( oggpack_buffer* b, int bits );
OGG_DLL_FUNCTION long oggpackB_look1( oggpack_buffer* b );
OGG_DLL_FUNCTION void oggpackB_adv( oggpack_buffer* b, int bits );
OGG_DLL_FUNCTION void oggpackB_adv1( oggpack_buffer* b );
OGG_DLL_FUNCTION long oggpackB_read( oggpack_buffer* b, int bits );
OGG_DLL_FUNCTION long oggpackB_read1( oggpack_buffer* b );
OGG_DLL_FUNCTION long oggpackB_bytes( oggpack_buffer* b );
OGG_DLL_FUNCTION long oggpackB_bits( oggpack_buffer* b );
OGG_DLL_FUNCTION unsigned char* oggpackB_get_buffer( oggpack_buffer* b );

/* Ogg BITSTREAM PRIMITIVES: encoding **************************/

OGG_DLL_FUNCTION int ogg_stream_packetin( ogg_stream_state* os, ogg_packet* op );
OGG_DLL_FUNCTION int ogg_stream_pageout( ogg_stream_state* os, ogg_page* og );
OGG_DLL_FUNCTION int ogg_stream_flush( ogg_stream_state* os, ogg_page* og );

/* Ogg BITSTREAM PRIMITIVES: decoding **************************/

OGG_DLL_FUNCTION int ogg_sync_init( ogg_sync_state* oy );
OGG_DLL_FUNCTION int ogg_sync_clear( ogg_sync_state* oy );
OGG_DLL_FUNCTION int ogg_sync_reset( ogg_sync_state* oy );
OGG_DLL_FUNCTION int ogg_sync_destroy( ogg_sync_state* oy );

OGG_DLL_FUNCTION char* ogg_sync_buffer( ogg_sync_state* oy, long size );
OGG_DLL_FUNCTION int ogg_sync_wrote( ogg_sync_state* oy, long bytes );
OGG_DLL_FUNCTION long ogg_sync_pageseek( ogg_sync_state* oy, ogg_page* og );
OGG_DLL_FUNCTION int ogg_sync_pageout( ogg_sync_state* oy, ogg_page* og );
OGG_DLL_FUNCTION int ogg_stream_pagein( ogg_stream_state* os, ogg_page* og );
OGG_DLL_FUNCTION int ogg_stream_packetout( ogg_stream_state* os, ogg_packet* op );
OGG_DLL_FUNCTION int ogg_stream_packetpeek( ogg_stream_state* os, ogg_packet* op );

/* Ogg BITSTREAM PRIMITIVES: general ***************************/

OGG_DLL_FUNCTION int ogg_stream_init( ogg_stream_state* os, int serialno );
OGG_DLL_FUNCTION int ogg_stream_clear( ogg_stream_state* os );
OGG_DLL_FUNCTION int ogg_stream_reset( ogg_stream_state* os );
OGG_DLL_FUNCTION int ogg_stream_reset_serialno( ogg_stream_state* os, int serialno );
OGG_DLL_FUNCTION int ogg_stream_destroy( ogg_stream_state* os );
OGG_DLL_FUNCTION int ogg_stream_eos( ogg_stream_state* os );

OGG_DLL_FUNCTION void ogg_page_checksum_set( ogg_page* og );

OGG_DLL_FUNCTION int ogg_page_version( ogg_page* og );
OGG_DLL_FUNCTION int ogg_page_continued( ogg_page* og );
OGG_DLL_FUNCTION int ogg_page_bos( ogg_page* og );
OGG_DLL_FUNCTION int ogg_page_eos( ogg_page* og );
OGG_DLL_FUNCTION ogg_int64_t ogg_page_granulepos( ogg_page* og );
OGG_DLL_FUNCTION int ogg_page_serialno( ogg_page* og );
OGG_DLL_FUNCTION long ogg_page_pageno( ogg_page* og );
OGG_DLL_FUNCTION int ogg_page_packets( ogg_page* og );

OGG_DLL_FUNCTION void ogg_packet_clear( ogg_packet* op );

#ifdef __SSE__												/* SSE Optimize */
#undef _ogg_malloc
#undef _ogg_calloc
#undef _ogg_realloc
#undef _ogg_free
#undef _ogg_alloca

#define _ogg_malloc( x ) _aligned_malloc( ( x ), 16 )
#define _ogg_realloc( x, y ) _aligned_realloc( ( x ), ( y ), 16 )
#define _ogg_free( x ) _aligned_free( ( x ) )
#endif														/* SSE Optimize */

#ifdef __cplusplus
}
#endif

#endif  /* _OGG_H */






