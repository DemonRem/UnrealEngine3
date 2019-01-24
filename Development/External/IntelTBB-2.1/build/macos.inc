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

####### Detections and Commands ###############################################
ifndef arch
 ifeq ($(shell /usr/sbin/sysctl -n hw.machine),Power Macintosh)
   ifeq ($(shell /usr/sbin/sysctl -n hw.optional.64bitops),1)
     export arch:=ppc64
   else
     export arch:=ppc32
   endif
 else
   ifeq ($(shell /usr/sbin/sysctl -n hw.optional.x86_64 2>/dev/null),1)
     export arch:=em64t
   else
     export arch:=ia32
   endif
 endif
endif

ifndef runtime
 #gcc_version:=$(shell gcc -v 2>&1 | grep 'gcc version' | sed -e 's/^gcc version //' | sed -e 's/ .*$$//' )
 gcc_version_full=$(shell gcc --version | grep 'gcc'| egrep -o ' [0-9]+\.[0-9]+\.[0-9]+.*' | sed -e 's/^\ //')
 gcc_version=$(shell echo "$(gcc_version_full)" | egrep -o '^[0-9]+\.[0-9]+\.[0-9]+\s*' | head -n 1 | sed -e 's/ *//g')
 os_version:=$(shell /usr/bin/sw_vers -productVersion)
 export runtime:=cc$(gcc_version)_os$(os_version)
endif

native_compiler := gcc
export compiler ?= gcc
debugger ?= gdb

CMD=$(SHELL) -c
CWD=$(shell pwd)
RM?=rm -f
RD?=rmdir
MD?=mkdir -p
NUL= /dev/null
SLASH=/
MAKE_VERSIONS=sh $(tbb_root)/build/version_info_macos.sh $(CPLUS) $(CPLUS_FLAGS) $(INCLUDES) >version_string.tmp
MAKE_TBBVARS=sh $(tbb_root)/build/generate_tbbvars.sh

####### Build settings ########################################################

OBJ=o
DLL=dylib
LIBEXT=dylib

def_prefix = $(if $(findstring 32,$(arch)),mac32,mac64)

TBB.DEF = $(tbb_root)/src/tbb/$(def_prefix)-tbb-export.def
TBB.DLL = libtbb$(DEBUG_SUFFIX).$(DLL)
TBB.LIB = $(TBB.DLL)
LINK_TBB.LIB = $(TBB.LIB)

MALLOC.DEF = $(MALLOC_ROOT)/$(def_prefix)-tbbmalloc-export.def
MALLOC.DLL = libtbbmalloc$(DEBUG_SUFFIX).$(DLL)
MALLOC.LIB = $(MALLOC.DLL)

TBB_NOSTRICT=1

TEST_LAUNCHER=sh $(tbb_root)/build/test_launcher.sh
