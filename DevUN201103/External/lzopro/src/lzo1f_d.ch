/* lzo1f_d.ch -- implementation of the LZO1F decompression algorithm

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


#include "lzo1_d.ch"


/***********************************************************************
// decompress a block of data.
************************************************************************/

LZO_PUBLIC(int)
DO_DECOMPRESS  ( const lzo_bytep in , lzo_uint  in_len,
                       lzo_bytep out, lzo_uintp out_len,
                       lzo_voidp wrkmem )
{
    register lzo_bytep op;
    register const lzo_bytep ip;
    register lzo_uint t;
    register const lzo_bytep m_pos;

    const lzo_bytep const ip_end = in + in_len;
#if defined(HAVE_ANY_OP)
    lzo_bytep const op_end = out + *out_len;
#endif

    LZO_UNUSED(wrkmem);

    *out_len = 0;

    op = out;
    ip = in;

    while (TEST_IP && TEST_OP)
    {
        t = *ip++;
        if (t > 31)
            goto match;

        /* a literal run */
        if (t == 0)
        {
            NEED_IP(1);
            while (*ip == 0)
            {
                t += 255;
                ip++;
                NEED_IP(1);
            }
            t += 31 + *ip++;
        }
        /* copy literals */
        assert(t > 0); NEED_OP(t); NEED_IP(t+1);
#if defined(LZO_UNALIGNED_OK_4)
        if (t >= 4)
        {
            do {
                COPY4(op, ip);
                op += 4; ip += 4; t -= 4;
            } while (t >= 4);
            if (t > 0) do *op++ = *ip++; while (--t > 0);
        }
        else
#endif
        do *op++ = *ip++; while (--t > 0);

        t = *ip++;

        while (TEST_IP && TEST_OP)
        {
            /* handle matches */
            if (t < 32)
            {
                m_pos = op - 1 - 0x800;
                m_pos -= (t >> 2) & 7;
                m_pos -= *ip++ << 3;
                TEST_LB(m_pos); NEED_OP(3);
                *op++ = *m_pos++; *op++ = *m_pos++; *op++ = *m_pos++;
            }
            else
            {
match:
                if (t < M3_MARKER)
                {
                    m_pos = op - 1;
                    m_pos -= (t >> 2) & 7;
                    m_pos -= *ip++ << 3;
                    t >>= 5;
                    TEST_LB(m_pos); assert(t > 0); NEED_OP(t+3-1);
                    goto copy_match;
                }
                else
                {
                    t &= 31;
                    if (t == 0)
                    {
                        NEED_IP(1);
                        while (*ip == 0)
                        {
                            t += 255;
                            ip++;
                            NEED_IP(1);
                        }
                        t += 31 + *ip++;
                    }
                    NEED_IP(2);
                    m_pos = op;
#if defined(LZO_UNALIGNED_OK_2) && defined(LZO_ABI_LITTLE_ENDIAN)
                    m_pos -= (* (const lzo_ushortp) ip) >> 2;
                    ip += 2;
#else
                    m_pos -= *ip++ >> 2;
                    m_pos -= *ip++ << 6;
#endif
                    if (m_pos == op)
                        goto eof_found;
                }

                /* copy match */
                TEST_LB(m_pos); assert(t > 0); NEED_OP(t+3-1);
#if defined(LZO_UNALIGNED_OK_4)
                if (t >= 2 * 4 - (3 - 1) && (op - m_pos) >= 4)
                {
                    COPY4(op, m_pos);
                    op += 4; m_pos += 4; t -= 4 - (3 - 1);
                    do {
                        COPY4(op, m_pos);
                        op += 4; m_pos += 4; t -= 4;
                    } while (t >= 4);
                    if (t > 0) do *op++ = *m_pos++; while (--t > 0);
                }
                else
#endif
                {
copy_match:
                *op++ = *m_pos++; *op++ = *m_pos++;
                do *op++ = *m_pos++; while (--t > 0);
                }
            }
            t = ip[-2] & 3;
            if (t == 0)
                break;

            /* copy literals */
            assert(t > 0); NEED_OP(t); NEED_IP(t+1);
            do *op++ = *ip++; while (--t > 0);
            t = *ip++;
        }
    }

#if defined(HAVE_TEST_IP) || defined(HAVE_TEST_OP)
    /* no EOF code was found */
    *out_len = pd(op, out);
    return LZO_E_EOF_NOT_FOUND;
#endif

eof_found:
    assert(t == 1);
    *out_len = pd(op, out);
    return (ip == ip_end ? LZO_E_OK :
           (ip < ip_end  ? LZO_E_INPUT_NOT_CONSUMED : LZO_E_INPUT_OVERRUN));


#if defined(HAVE_NEED_IP)
input_overrun:
    *out_len = pd(op, out);
    return LZO_E_INPUT_OVERRUN;
#endif

#if defined(HAVE_NEED_OP)
output_overrun:
    *out_len = pd(op, out);
    return LZO_E_OUTPUT_OVERRUN;
#endif

#if defined(LZO_TEST_OVERRUN_LOOKBEHIND)
lookbehind_overrun:
    *out_len = pd(op, out);
    return LZO_E_LOOKBEHIND_OVERRUN;
#endif
}


/*
vi:ts=4:et
*/

