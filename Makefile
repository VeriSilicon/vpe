#/*
# * Copyright 2019 VeriSilicon, Inc.
# *
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *      http://www.apache.org/licenses/LICENSE-2.0
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# */

DLL_PATH  := "/usr/lib/vpe"
INC_PATH  := "/usr/local/include/vpe"
PKG_PATH  := "/usr/share/pkgconfig"
PWD  := $(shell pwd)
DRV_PATH := "/lib/modules/$(shell uname -r)/kernel/drivers/pci/pcie/solios-x"

.PHONY: all vpi drivers install clean help

-include config.mk

all: vpi drivers

ARCH ?= $(shell uname -m)

vpi:
	@echo "VPE build step - build VPI"
ifeq ($(DEBUG),y)
	@echo "Build debug VPE"
else
	@echo "Build release VPE"
endif
	make -C vpi CHECK_MEM_LEAK=y DEBUG=$(DEBUG) ARCH=$(ARCH)

drivers:
	make -C drivers all

install:
	@echo "VPE build step - install"
	$(shell if [ ! -d $(DLL_PATH) ]; then mkdir $(DLL_PATH); fi;)
	$(shell if [ ! -d $(INC_PATH) ]; then mkdir $(INC_PATH); fi;)
	cp $(PWD)/firmware/ZSP_FW_RP_V*.bin /lib/firmware/transcoder_zsp_fw.bin
	cp $(PWD)/sdk_libs/$(ARCH)/*.so $(DLL_PATH)
	cp $(PWD)/vpi/libvpi.so $(DLL_PATH)
	cp $(PWD)/build/libvpi.pc $(PKG_PATH)
	echo "/usr/lib/vpe" >  /etc/ld.so.conf.d/vpe-$(ARCH).conf
	/sbin/ldconfig
	cp $(PWD)/vpi/inc/*.h $(INC_PATH)
	$(shell rmmod transcoder_pcie)
	insmod drivers/transcoder_pcie.ko
	rm $(DRV_PATH) -rf
	mkdir -p $(DRV_PATH)
	cp drivers/transcoder_pcie.ko $(DRV_PATH)
	depmod
	@echo "VPE installation finished!"

uninstall:
	@echo "VPE build step - uninstall"
	$(shell if [ -d $(DLL_PATH) ]; then rm $(DLL_PATH) -rf; fi;)
	$(shell if [ -d $(INC_PATH) ]; then rm $(INC_PATH) -rf; fi;)
	$(shell rm /lib/firmware/transcoder_zsp_fw.bin)
	$(shell rm 	$(PKG_PATH)/libvpi.pc)
	$(shell rm /etc/ld.so.conf.d/vpe-$(ARCH).conf)
	/sbin/ldconfig
	$(shell rmmod transcoder_pcie)
	$(shell rm $(DRV_PATH) -rf )
	depmod
	@echo "VPE uninstallation finished!"

clean:
	make -C vpi clean
	make -C drivers clean

help:
	@echo ""
	@echo "  o make                - make VPI library and pcie driver"
	@echo "  o make clean          - cleaN VPE"
	@echo "  o make install        - install VPE"
	@echo "  o make: uninstall      - uninstall VPE"
