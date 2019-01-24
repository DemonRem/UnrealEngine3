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

 function: vorbis encode-engine setup

 ********************************************************************/

#ifndef _OV_ENC_H_
#define _OV_ENC_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include "codec.h"


VORBIS_DLL_FUNCTION int vorbis_encode_init( vorbis_info* vi, long channels, long rate, long max_bitrate, long nominal_bitrate, long min_bitrate );
VORBIS_DLL_FUNCTION int vorbis_encode_setup_managed( vorbis_info* vi, long channels, long rate, long max_bitrate, long nominal_bitrate, long min_bitrate );
/* quality level from 0. (lo) to 1. (hi) */
VORBIS_DLL_FUNCTION int vorbis_encode_setup_vbr( vorbis_info* vi, long channels, long rate, float quality );
/* quality level from 0. (lo) to 1. (hi) */
VORBIS_DLL_FUNCTION int vorbis_encode_init_vbr( vorbis_info* vi, long channels, long rate, float base_quality );
VORBIS_DLL_FUNCTION int vorbis_encode_setup_init( vorbis_info* vi );
VORBIS_DLL_FUNCTION int vorbis_encode_ctl( vorbis_info* vi, int number, void* arg );

/* new rate setup */
#define OV_ECTL_RATEMANAGE2_GET      0x14
#define OV_ECTL_RATEMANAGE2_SET      0x15

struct ovectl_ratemanage2_arg 
{
	int		management_active;

	long	bitrate_limit_min_kbps;
	long	bitrate_limit_max_kbps;
	long	bitrate_limit_reservoir_bits;
	double	bitrate_limit_reservoir_bias;

	long	bitrate_average_kbps;
	double	bitrate_average_damping;
};

#define OV_ECTL_LOWPASS_GET          0x20
#define OV_ECTL_LOWPASS_SET          0x21

#define OV_ECTL_IBLOCK_GET           0x30
#define OV_ECTL_IBLOCK_SET           0x31

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif


