###########################################################################
#
# Copyright 2018 Samsung Electronics All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
# either express or implied. See the License for the specific
# language governing permissions and limitations under the License.
#
###########################################################################

-include $(TOPDIR)/.config
-include $(TOPDIR)/Make.defs

IOTJS_ROOT_DIR ?= $(TOPDIR)/$(EXTDIR)/iotjs
IOTJS_BUILD_OPTION ?=
ifeq ($(CONFIG_DEBUG),y)
  IOTJS_BUILDTYPE = debug
else
  IOTJS_BUILDTYPE = release
endif
IOTJS_LIB_DIR = $(IOTJS_ROOT_DIR)/build/arm-tizenrt/$(IOTJS_BUILDTYPE)/lib

all: build
.PHONY: depend clean distclean

build:
	$(Q) python $(IOTJS_ROOT_DIR)/tools/build.py --target-arch=$(CONFIG_ARCH) --target-os=tizenrt \
	--sysroot=$(TOPDIR) --target-board=$(CONFIG_ARCH_BOARD) --jerry-heaplimit=$(CONFIG_IOTJS_JERRY_HEAP) \
	--buildtype=$(IOTJS_BUILDTYPE) --no-init-submodule $(IOTJS_BUILD_OPTION) --external-include-dir=$(TOPDIR)/net/lwip/src/include
	$(Q) cp $(IOTJS_LIB_DIR)/*.a $(TOPDIR)/$(EXTDIR)/iotjs

depend:

clean:
	$(Q) $(call DELDIR, $(IOTJS_ROOT_DIR)/build)
	$(Q) $(call DELFILE, $(IOTJS_ROOT_DIR)/*.a)

distclean:
