//------------------------------------------------------------------------------
// An enumeration of the valid phonemes.
//
// Owner: John Briggs
//
// Copyright (c) 2002-2006 OC3 Entertainment, Inc.
//------------------------------------------------------------------------------

#ifndef FxPhonemeEnum_H__
#define FxPhonemeEnum_H__

namespace OC3Ent
{

namespace Face
{

/// The phonemes supported by FaceFX.
enum FxPhoneme
{
	PHONEME_INVALID = -1,
	PHONEME_FIRST = 0,
	PHONEME_IY = PHONEME_FIRST,          // 0
	PHONEME_IH,                          // 1
	PHONEME_EH,                          // 2
	PHONEME_EY,                          // 3
	PHONEME_AE,                          // 4
	PHONEME_AA,                          // 5
	PHONEME_AW,                          // 6
	PHONEME_AY,                          // 7
	PHONEME_AH,                          // 8
	PHONEME_AO,                          // 9
	PHONEME_OY,                          // 10
	PHONEME_OW,                          // 11
	PHONEME_UH,                          // 12
	PHONEME_UW,                          // 13
	PHONEME_ER,                          // 14
	PHONEME_AX,                          // 15
	PHONEME_S,                           // 16
	PHONEME_SH,                          // 17
	PHONEME_Z,                           // 18
	PHONEME_ZH,                          // 19
	PHONEME_F,                           // 20
	PHONEME_TH,                          // 21
	PHONEME_V,                           // 22
	PHONEME_DH,                          // 23
	PHONEME_M,                           // 24
	PHONEME_N,                           // 25
	PHONEME_NG,                          // 26
	PHONEME_L,                           // 27
	PHONEME_R,                           // 28
	PHONEME_W,                           // 29
	PHONEME_Y,                           // 30
	PHONEME_HH,                          // 31
	PHONEME_B,                           // 32
	PHONEME_D,                           // 33
	PHONEME_JH,                          // 34
	PHONEME_G,                           // 35
	PHONEME_P,                           // 36
	PHONEME_T,                           // 37
	PHONEME_K,                           // 38
	PHONEME_CH,                          // 39
	PHONEME_SIL,                         // 40
	PHONEME_SHORTSIL,					 // 41
	PHONEME_FLAP,						 // 42
	PHONEME_LAST = PHONEME_FLAP,         // 42
	NUM_PHONEMES                         // 43 (0..42)
};

} // namespace Face

} // namespace OC3Ent

#endif
