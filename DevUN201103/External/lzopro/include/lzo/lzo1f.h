/* lzo1f.h -- public interface of the LZO1F compression algorithm

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


#ifndef __LZO1F_H_INCLUDED
#define __LZO1F_H_INCLUDED

#ifndef __LZOCONF_H_INCLUDED
#include "lzoconf.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************
//
************************************************************************/

/* Memory required for the wrkmem parameter.
 * When the required size is 0, you can also pass a NULL pointer.
 */

#define LZO1F_MEM_COMPRESS      ((lzo_uint32) (16384L * lzo_sizeof_dict_t))
#define LZO1F_MEM_DECOMPRESS    (0)


/* decompression */
LZO_EXTERN(int)
lzo1f_decompress        ( const lzo_bytep src, lzo_uint  src_len,
                                lzo_bytep dst, lzo_uintp dst_len,
                                lzo_voidp wrkmem /* NOT USED */ );

/* safe decompression with overrun testing */
LZO_EXTERN(int)
lzo1f_decompress_safe   ( const lzo_bytep src, lzo_uint  src_len,
                                lzo_bytep dst, lzo_uintp dst_len,
                                lzo_voidp wrkmem /* NOT USED */ );


/***********************************************************************
//
************************************************************************/

LZO_EXTERN(int)
lzo1f_1_compress        ( const lzo_bytep src, lzo_uint  src_len,
                                lzo_bytep dst, lzo_uintp dst_len,
                                lzo_voidp wrkmem );


/***********************************************************************
// better compression ratio at the cost of more memory and time
************************************************************************/

#define LZO1F_999_MEM_COMPRESS  ((lzo_uint32) (5 * 16384L * sizeof(short)))

LZO_EXTERN(int)
lzo1f_999_compress      ( const lzo_bytep src, lzo_uint  src_len,
                                lzo_bytep dst, lzo_uintp dst_len,
                                lzo_voidp wrkmem );



#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* already included */


/* vim:set ts=4 et: */
