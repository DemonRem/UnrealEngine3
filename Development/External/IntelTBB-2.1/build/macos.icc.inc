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

CPLUS = icpc
COMPILE_ONLY = -c -MMD
PREPROC_ONLY = -E -x c
INCLUDE_KEY = -I
DEFINE_KEY = -D
OUTPUT_KEY = -o #
OUTPUTOBJ_KEY = -o #
PIC_KEY = -fPIC
WARNING_AS_ERROR_KEY = -Werror
WARNING_KEY = -w1
DYLIB_KEY = -dynamiclib
EXPORT_KEY = -Wl,-exported_symbols_list,
LIBDL = -ldl
export COMPILER_VERSION := $(shell icc -V </dev/null 2>&1 | grep 'Version')
#TODO: autodetection of arch from COMPILER_VERSION!!

OPENMP_FLAG = -openmp
LIBS = -lpthread
LINK_FLAGS = 
LIB_LINK_FLAGS = -dynamiclib -i-static
C_FLAGS = $(CPLUS_FLAGS) -x c

ifeq ($(cfg), release)
    CPLUS_FLAGS = -O2 -fno-omit-frame-pointer
else
    CPLUS_FLAGS = -g -O0 -DTBB_USE_DEBUG
endif

CPLUS_FLAGS += -DUSE_PTHREAD

ifneq (,$(codecov))
    CPLUS_FLAGS += -prof-genx
endif

ifeq (em64t,$(arch))
#    LINK_FLAGS += -m64
#    LIB_LINK_FLAGS += -m64
endif

#------------------------------------------------------------------------------
# Setting tbbmalloc data.
#------------------------------------------------------------------------------

M_CPLUS_FLAGS = $(CPLUS_FLAGS) -fno-rtti -fno-exceptions

#------------------------------------------------------------------------------
# End of setting tbbmalloc data.
#------------------------------------------------------------------------------
