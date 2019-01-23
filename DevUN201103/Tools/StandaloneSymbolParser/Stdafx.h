// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#include <vcclr.h>
#include <atlbase.h>
#include <atlcom.h>
#include <Windows.h>
#include <diacreate.h>
#include <dia2.h>
#include <ImageHlp.h>

#ifdef GetEnvironmentVariable
#undef GetEnvironmentVariable
#endif

#ifdef GetCurrentDirectory
#undef GetCurrentDirectory
#endif