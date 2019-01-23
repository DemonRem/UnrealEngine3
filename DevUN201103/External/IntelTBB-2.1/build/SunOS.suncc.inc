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

COMPILE_ONLY = -c -MMD -errtags
PREPROC_ONLY = -E -MMD
INCLUDE_KEY = -I
DEFINE_KEY = -D
OUTPUT_KEY = -o #
OUTPUTOBJ_KEY = -o #
PIC_KEY = -KPIC
DYLIB_KEY = -G
LIBDL = -ldl
# WARNING_AS_ERROR_KEY = -errwarn=%all
WARNING_AS_ERROR_KEY = Warning as error
WARNING_SUPPRESS = -erroff=unassigned,attrskipunsup,badargtype2w,badbinaryopw,wbadasg,wvarhidemem
tbb_strict=0

TBB_NOSTRICT = 1

CPLUS = CC
CONLY = cc
LIB_LINK_FLAGS = -G -R . -M$(tbb_root)/build/suncc.map.pause
LINK_FLAGS += -M$(tbb_root)/build/suncc.map.pause
LIBS = -lpthread -lrt -R .
C_FLAGS = $(CPLUS_FLAGS)

ifeq ($(cfg), release)
        CPLUS_FLAGS = -mt -xO2 -library=stlport4 -DUSE_PTHREAD $(WARNING_SUPPRESS)
endif
ifeq ($(cfg), debug)
        CPLUS_FLAGS = -mt -DTBB_USE_DEBUG -g -library=stlport4 -DUSE_PTHREAD $(WARNING_SUPPRESS)
endif

ASM=
ASM_FLAGS=

TBB_ASM.OBJ=

ifeq (em64t,$(arch))
    CPLUS_FLAGS += -m64
    ASM_FLAGS += -m64
    LIB_LINK_FLAGS += -m64
endif 

ifeq (ia32,$(arch))
    CPLUS_FLAGS += -m32
    LIB_LINK_FLAGS += -m32
endif 

# TODO: verify whether -m64 implies V9 on relevant Sun Studio versions
#       (those that handle gcc assembler syntax)
ifeq (sparc,$(arch))
    CPLUS_FLAGS    += -m64
    LIB_LINK_FLAGS += -m64
endif 

#------------------------------------------------------------------------------
# Setting assembler data.
#------------------------------------------------------------------------------
ASSEMBLY_SOURCE=$(arch)-fbe
#------------------------------------------------------------------------------
# End of setting assembler data.
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# Setting tbbmalloc data.
#------------------------------------------------------------------------------
M_INCLUDES = $(INCLUDES) -I$(MALLOC_ROOT) -I$(MALLOC_SOURCE_ROOT)
M_CPLUS_FLAGS = $(CPLUS_FLAGS)
#------------------------------------------------------------------------------
# End of setting tbbmalloc data.
#------------------------------------------------------------------------------
