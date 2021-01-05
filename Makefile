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

arch ?= $(shell uname -m)
ifeq ($(arch),aarch64)
	arch=arm64
endif

cross ?= n
installpath ?=
packagepath = $(shell pwd)/package

DLL_PATH = $(installpath)/usr/lib/vpe/
INC_PATH = $(installpath)/usr/local/include/vpe/
PKG_PATH = $(installpath)/usr/share/pkgconfig/
DRV_PATH = $(installpath)/lib/modules/`uname -r`/kernel/drivers/pci/pcie/solios-x/
FRM_PATH = $(installpath)/lib/firmware/
CFG_PATH = $(installpath)/etc/ld.so.conf.d/

.PHONY: all vpi drivers install clean help package

all: drivers vpi package

vpi:
	@echo "VPE build step - build VPI"

ifeq ($(DEBUG),y)
	@echo "Build debug VPE"
else
	@echo "Build release VPE"
endif
	make -C vpi CHECK_MEM_LEAK=y

drivers:
	make -C drivers all

package:

	$(shell if [ ! -d $(packagepath)/ ]; then mkdir $(packagepath); fi;)
	$(shell cp firmware/ZSP_FW_RP_V*.bin $(packagepath)/ )
	$(shell cp sdk_libs/$(arch)/*.so $(packagepath)/ )
	$(shell cp vpi/libvpi.so $(packagepath)/ )
	$(shell if [ ! -d $(packagepath)/vpe/ ]; then mkdir $(packagepath)/vpe/; fi;)
	$(shell cp $(PWD)/vpi/inc/*.h $(packagepath)/vpe )
	$(shell cp $(PWD)/drivers/transcoder_pcie.ko $(packagepath)/ )
	## libvpi.pc was generated to $(packagepath)

	@echo "Name: libvpi" >  $(packagepath)/libvpi.pc
	@echo "Description: VSI platform interface lib" >>  $(packagepath)/libvpi.pc
	@echo "Version: 1.0" >>  $(packagepath)/libvpi.pc
	@echo "Cflags: -I$(packagepath)/" >> $(packagepath)/libvpi.pc
	@echo "Libs: -L$(packagepath)/ -lvpi"  >> $(packagepath)/libvpi.pc
	## VPE libs, fw, .h were copied to $(packagepath)

install:
ifeq ($(cross),y)
ifeq ($(installpath),)
	@echo "!! WARNNING !! non-compatible VPE will be installed to your host server system folder. "
	@ERROR @echo "Please use ./configure --installpath to set target VPE installation path."
endif
endif

	$(shell if [ ! -d $(DLL_PATH) ]; then mkdir $(DLL_PATH) -p; fi;)
	$(shell if [ ! -d $(INC_PATH) ]; then mkdir $(INC_PATH) -p; fi;)
	$(shell if [ ! -d $(PKG_PATH) ]; then mkdir $(PKG_PATH) -p; fi;)
	$(shell if [ ! -d $(DRV_PATH) ]; then mkdir $(DRV_PATH) -p; fi;)
	$(shell if [ ! -d $(FRM_PATH) ]; then mkdir $(FRM_PATH) -p; fi;)
	$(shell if [ ! -d $(CFG_PATH) ]; then mkdir $(CFG_PATH) -p; fi;)

	$(shell cp sdk_libs/$(arch)/*.so $(DLL_PATH) )
	$(shell cp vpi/libvpi.so $(DLL_PATH) )

	$(shell cp vpi/inc/*.h $(INC_PATH) )
	$(shell cp build/libvpi.pc $(PKG_PATH) )
	$(shell rm $(DRV_PATH) -rf )
	$(shell mkdir -p $(DRV_PATH) )
	$(shell cp drivers/transcoder_pcie.ko $(DRV_PATH) )
	$(shell cp firmware/ZSP_FW_RP_V*.bin $(FRM_PATH)/transcoder_zsp_fw.bin )
	@echo "/usr/lib/vpe" > $(CFG_PATH)/vpe-$(arch).conf
	## VPE libs, fw, .h were installed to path:$(installpath)

ifeq ($(cross),n)
	@echo "Installing driver..."
	$(shell /sbin/ldconfig )
	$(shell rmmod transcoder_pcie )
	$(shell insmod drivers/transcoder_pcie.ko )
ifneq ($(shell lsmod | grep transcoder_pcie), "")
	@echo "driver was installed successfully!"
else
	@echo "driver was installed failed!"
endif
	$(shell depmod )
else
	@echo "cross compiling, skip driver installation"
endif
	## VPE installation finished!

uninstall:
	@echo "VPE build step - uninstall"
	$(shell if [ -d $(DLL_PATH) ]; then rm $(DLL_PATH) -rf; fi; )
	$(shell if [ -d $(INC_PATH) ]; then rm $(INC_PATH) -rf; fi; )
	$(shell rm $(FRM_PATH)/transcoder_zsp_fw.bin )
	$(shell rm $(PKG_PATH)/libvpi.pc )
	$(shell rm $(CFG_PATH)/vpe-$(ARCH).conf )
	$(shell rm $(DRV_PATH) -rf )

ifeq ($(cross),n)
	$(shell /sbin/ldconfig )
	$(shell rmmod transcoder_pcie )
	$(shell depmod )
endif
	## VPE uninstallation finished!

clean:
	make -C vpi clean
	make -C drivers clean
	$(shell if [ -d $(packagepath)/ ]; then rm $(packagepath)/ -rf; fi;)

help:
	@echo "  o make                - make VPI library and pcie driver"
	@echo "  o make clean          - cleaN VPE"
	@echo "  o make install        - install VPE"
	@echo "  o make: uninstall      - uninstall VPE"
