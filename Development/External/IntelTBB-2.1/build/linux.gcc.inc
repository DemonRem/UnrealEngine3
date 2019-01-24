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

COMPILE_ONLY = -c -MMD
PREPROC_ONLY = -E -x c
INCLUDE_KEY = -I
DEFINE_KEY = -D
OUTPUT_KEY = -o #
OUTPUTOBJ_KEY = -o #
PIC_KEY = -fPIC
WARNING_AS_ERROR_KEY = -Werror
WARNING_KEY = -Wall
WARNING_SUPPRESS = -Wno-parentheses
DYLIB_KEY = -shared
LIBDL = -ldl

TBB_NOSTRICT = 1

CPLUS = g++ 
CONLY = gcc
LIB_LINK_FLAGS = -shared -Wl,-soname=$(BUILDING_LIBRARY)
LIBS = -lpthread -lrt 
C_FLAGS = $(CPLUS_FLAGS)

ifeq ($(cfg), release)
        CPLUS_FLAGS = -DDO_ITT_NOTIFY -O2 -DUSE_PTHREAD
endif
ifeq ($(cfg), debug)
        CPLUS_FLAGS = -DTBB_USE_DEBUG -DDO_ITT_NOTIFY -g -O0 -DUSE_PTHREAD
endif

ASM=
ASM_FLAGS=

TBB_ASM.OBJ=

ifeq (itanium,$(arch))
# Position-independent code (PIC) is a must on IA-64, even for regular (not shared) executables
    CPLUS_FLAGS += $(PIC_KEY)
endif 

ifeq (em64t,$(arch))
    CPLUS_FLAGS += -m64
    LIB_LINK_FLAGS += -m64
endif 

ifeq (ia32,$(arch))
    CPLUS_FLAGS += -m32
    LIB_LINK_FLAGS += -m32
endif 

# for some gcc versions on Solaris, -m64 may imply V9, but perhaps not everywhere (TODO: verify)
ifeq (sparc,$(arch))
    CPLUS_FLAGS    += -mcpu=v9 -m64
    LIB_LINK_FLAGS += -mcpu=v9 -m64
endif 

#------------------------------------------------------------------------------
# Setting assembler data.
#------------------------------------------------------------------------------
ASSEMBLY_SOURCE=$(arch)-gas
ifeq (itanium,$(arch))
    ASM=as
    ASM_FLAGS += -xexplicit
    TBB_ASM.OBJ = atomic_support.o lock_byte.o log2.o pause.o
endif 
#------------------------------------------------------------------------------
# End of setting assembler data.
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# Setting tbbmalloc data.
#------------------------------------------------------------------------------

M_CPLUS_FLAGS = $(CPLUS_FLAGS) -fno-rtti -fno-exceptions -fno-schedule-insns2

#------------------------------------------------------------------------------
# End of setting tbbmalloc data.
#------------------------------------------------------------------------------
