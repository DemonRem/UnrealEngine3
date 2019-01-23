/*=============================================================================
	UnCodecs.h: Movie codec definitions.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef CODECS_H
#define CODECS_H

#ifndef USE_BINK_CODEC
#error UnBuild.h must be included before including this file (UnCodecs.h).
#endif

#if USE_BINK_CODEC && !USE_NULL_RHI
#include "../Bink/Src/UnCodecBink.h"
#else
class UCodecMovieBink : public UCodecMovie
{
	DECLARE_CLASS(UCodecMovieBink,UCodecMovie,CLASS_Intrinsic|CLASS_Transient,Engine)
};
#endif //USE_BINK_CODEC

#endif //CODECS_H




