/* lzo1b_cc.h -- definitions for the the LZO1B compression driver

   This file is part of the LZO-Professional data compression library.

   Copyright (C) 2006 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2005 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2004 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2003 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2002 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2001 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2000 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1999 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1998 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1997 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1996 Markus Franz Xaver Johannes Oberhumer
   All Rights Reserved.

   CONFIDENTIAL & PROPRIETARY SOURCE CODE.

   ANY USAGE OF THIS FILE IS SUBJECT TO YOUR LICENSE AGREEMENT.

   Markus F.X.J. Oberhumer
   <markus@oberhumer.com>
   http://www.oberhumer.com/products/lzo-professional/
 */


/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the library and is subject
   to change.
 */


#ifndef __LZO1B_CC_H
#define __LZO1B_CC_H


/***********************************************************************
//
************************************************************************/

extern const lzo_compress_t _lzo1b_1_compress_func;
extern const lzo_compress_t _lzo1b_2_compress_func;
extern const lzo_compress_t _lzo1b_3_compress_func;
extern const lzo_compress_t _lzo1b_4_compress_func;
extern const lzo_compress_t _lzo1b_5_compress_func;
extern const lzo_compress_t _lzo1b_6_compress_func;
extern const lzo_compress_t _lzo1b_7_compress_func;
extern const lzo_compress_t _lzo1b_8_compress_func;
extern const lzo_compress_t _lzo1b_9_compress_func;

extern const lzo_compress_t _lzo1b_99_compress_func;


/***********************************************************************
//
************************************************************************/

LZO_EXTERN(lzo_bytep )
_lzo1b_store_run ( lzo_bytep const oo, const lzo_bytep const ii,
                   lzo_uint r_len);

#define STORE_RUN   _lzo1b_store_run


lzo_compress_t _lzo1b_get_compress_func(int clevel);

int _lzo1b_do_compress   ( const lzo_bytep in,  lzo_uint  in_len,
                                 lzo_bytep out, lzo_uintp out_len,
                                 lzo_voidp wrkmem,
                                 lzo_compress_t func );


#endif /* already included */

/*
vi:ts=4:et
*/


