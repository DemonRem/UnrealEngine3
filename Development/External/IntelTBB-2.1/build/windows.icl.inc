# Copyright 2005-2009 Intel Corporation.  All Rights Reserved.
#
# The source code contained or described herein and all documents related
# to the source code ("Material") are owned by Intel Corporation or its
# suppliers or licensors.  Title to the Material remains with Intel
# Corporation or its suppliers and licensors.  The Material is protected
# by worldwide copyright laws and treaty provisions.  No part of the
# Material may be used, copied, reproduced, modified, published, uploaded,
# posted, transmitted, distributed, or disclosed in any way without
# Intel's prior express written permission.
#
# No license under any patent, copyright, trade secret or other
# intellectual property right is granted to or conferred upon you by
# disclosure or delivery of the Materials, either expressly, by
# implication, inducement, estoppel or otherwise.  Any license under such
# intellectual property rights must be express and approved by Intel in
# writing.

#------------------------------------------------------------------------------
# Define compiler-specific variables.
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# Setting default configuration to release.
#------------------------------------------------------------------------------
cfg ?= release
#------------------------------------------------------------------------------
# End of setting default configuration to release.
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# Setting compiler flags.
#------------------------------------------------------------------------------
CPLUS = icl /nologo $(VCCOMPAT_FLAG)
LINK_FLAGS = /link /nologo
LIB_LINK_FLAGS= /link /nologo /DLL /MAP /DEBUG /fixed:no /INCREMENTAL:NO
MS_CRT_KEY = /MD$(if $(findstring debug,$(cfg)),d)
EH_FLAGS = /EHsc /GR

ifeq ($(cfg), release)
    CPLUS_FLAGS = $(MS_CRT_KEY) /O2 /Zi $(EH_FLAGS) /Zc:forScope /Zc:wchar_t
    ASM_FLAGS =
ifeq (ia32,$(arch))
    CPLUS_FLAGS += /Oy
endif
endif
ifeq ($(cfg), debug)
    CPLUS_FLAGS = $(MS_CRT_KEY) /Od /Ob0 /Zi $(EH_FLAGS) /Zc:forScope /Zc:wchar_t /DTBB_USE_DEBUG
    LINK_FLAGS += libmmds.lib /NODEFAULTLIB:libmmdd.lib
    ASM_FLAGS = /DUSE_FRAME_POINTER
endif


COMPILE_ONLY = /c /QMMD
PREPROC_ONLY = /EP /Tc
INCLUDE_KEY = /I
DEFINE_KEY = /D
OUTPUT_KEY = /Fe
OUTPUTOBJ_KEY = /Fo
WARNING_AS_ERROR_KEY = /WX
WARNING_KEY = /W3
DYLIB_KEY = /DLL
EXPORT_KEY = /DEF:

ifeq (em64t,$(arch))
    CPLUS_FLAGS += /GS-
endif

ifneq (,$(codecov))
    CPLUS_FLAGS += /Qprof-genx
else
    CPLUS_FLAGS += /DDO_ITT_NOTIFY
endif

OPENMP_FLAG = /Qopenmp
CPLUS_FLAGS += /DUSE_WINTHREAD /D_CRT_SECURE_NO_DEPRECATE \
               /D_WIN32_WINNT=$(_WIN32_WINNT)

ifeq ($(runtime),vc8)
        CPLUS_FLAGS += /D_USE_RTM_VERSION
endif

C_FLAGS = $(CPLUS_FLAGS)

VCVERSION:=$(runtime)
VCCOMPAT_FLAG := $(if $(findstring vc7.1, $(VCVERSION)),/Qvc7.1)
ifeq ($(VCCOMPAT_FLAG),)
        VCCOMPAT_FLAG := $(if $(findstring vc8, $(VCVERSION)),/Qvc8)
endif
ifeq ($(VCCOMPAT_FLAG),)
        VCCOMPAT_FLAG := $(if $(findstring vc9, $(VCVERSION)),/Qvc9)
endif
ifeq ($(VCCOMPAT_FLAG),)
        $(error VC version not detected correctly: $(VCVERSION) )
endif
export VCCOMPAT_FLAG
#------------------------------------------------------------------------------
# End of setting compiler flags.
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# Setting assembler data.
#------------------------------------------------------------------------------
ASSEMBLY_SOURCE=$(arch)-masm
ifeq (em64t,$(arch))
    ASM=ml64
    ASM_FLAGS += /DEM64T=1 /c /Zi
    TBB_ASM.OBJ = atomic_support.obj
else
    ASM=ml
    ASM_FLAGS += /c /coff /Zi
    TBB_ASM.OBJ = atomic_support.obj lock_byte.obj
endif
#------------------------------------------------------------------------------
# End of setting assembler data.
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# Setting tbbmalloc data.
#------------------------------------------------------------------------------
M_CPLUS_FLAGS = $(subst $(EH_FLAGS),/EHs-,$(CPLUS_FLAGS))
#------------------------------------------------------------------------------
# End of setting tbbmalloc data.
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# End of define compiler-specific variables.
#------------------------------------------------------------------------------
