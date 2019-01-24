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

ifndef tbb_os
 ifeq ($(OS), Windows_NT)
  export tbb_os=windows
 else
  OS:=$(shell uname)
  ifeq ($(OS),)
   $(error "$(OS) is not supported")
  else
   export tbb_os=$(OS)
   ifeq ($(OS), Linux)
    export tbb_os=linux
   endif
   ifeq ($(OS), Darwin)
    export tbb_os=macos
   endif
  endif # OS successfully detected
 endif # !Windows
endif # !tbb_os

ifeq ($(wildcard $(tbb_root)/build/$(tbb_os).inc),)
  $(error "$(tbb_os)" is not supported. Add build/$(tbb_os).inc file with os-specific settings )
endif

# detect arch and runtime versions, provide common os-specific definitions
include $(tbb_root)/build/$(tbb_os).inc

ifeq ($(arch),)
 $(error Architecture not detected)
endif
ifeq ($(runtime),)
 $(error Runtime version not detected)
endif
ifeq ($(wildcard $(tbb_root)/build/$(tbb_os).$(compiler).inc),)
  $(error Compiler "$(compiler)" is not supported on $(tbb_os). Add build/$(tbb_os).$(compiler).inc file with compiler-specific settings )
endif

# Support for running debug tests to release library and vice versa
flip_cfg=$(subst _flipcfg,_release,$(subst _release,_debug,$(subst _debug,_flipcfg,$(1))))
cross_cfg = $(if $(crosstest),$(call flip_cfg,$(1)),$(1))

ifdef BUILDING_PHASE
 # Setting default configuration to release
 cfg?=release
  # include compiler-specific build configurations
 -include $(tbb_root)/build/$(tbb_os).$(compiler).inc
 ifdef extra_inc
  -include $(tbb_root)/build/$(extra_inc)
 endif
endif
ifneq ($(BUILDING_PHASE),1)
 # definitions for top-level Makefiles
 origin_build_dir:=$(origin tbb_build_dir)
 tbb_build_dir?=$(tbb_root)$(SLASH)build
 tbb_build_prefix?=$(tbb_os)_$(arch)_$(compiler)_$(runtime)
 work_dir=$(tbb_build_dir)$(SLASH)$(tbb_build_prefix)
 ifneq ($(BUILDING_PHASE),0)
  work_dir:=$(work_dir)
  # assign new value for tbb_root if path is not absolute (the filter keeps only /* paths)
  ifeq ($(filter /% $(SLASH)%, $(subst :, ,$(tbb_root)) ),)
   ifeq ($(origin_build_dir),undefined)
    override tbb_root:=../..
   else
    override tbb_root:=$(CWD)/$(tbb_root)
   endif
  endif
  export tbb_root
 endif # BUILDING_PHASE != 0
endif  # BUILDING_PHASE != 1
