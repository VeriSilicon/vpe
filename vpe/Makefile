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

.PHONY: all vpi drivers install clean help

all: prepare vpi drivers

ifeq ($(DEBUG),y)
	@echo "Build debug VPE"
else
	@echo "Build release VPE"
endif

prepare:
	$(shell if [ ! -d $(DLL_PATH) ]; then sudo mkdir $(DLL_PATH); fi;)
	$(shell if [ ! -d $(INC_PATH) ]; then sudo mkdir $(INC_PATH); fi;)
	sudo cp $(PWD)/sdk_libs/*.so $(DLL_PATH)
	sudo cp $(PWD)/vpi/inc/*.h $(INC_PATH)
	sudo cp $(PWD)/build/libvpi.pc $(PKG_PATH)

vpi: prepare
	make -C vpi DEBUG=$(DEBUG)

drivers:
	make -C drivers/transcoder-pcie clean
	make -C drivers/transcoder-pcie all

install:
	$(shell if [ ! -d $(DLL_PATH) ]; then mkdir $(DLL_PATH); fi;)
	$(shell if [ ! -d $(INC_PATH) ]; then mkdir $(INC_PATH); fi;)
	cp $(PWD)/firmware/ZSP_FW_RP_V*.bin /lib/firmware/transcoder_zsp_fw.bin
	cp $(PWD)/sdk_libs/*.so $(DLL_PATH)
	cp $(PWD)/vpi/libvpi.so $(DLL_PATH)
	echo "/usr/lib/vpe" >  /etc/ld.so.conf.d/vpe-x86_64.conf
	/sbin/ldconfig
	cp $(PWD)/vpi/inc/*.h $(INC_PATH)
	$(shell rmmod transcoder_pcie;)
	$(shell insmod $(PWD)/drivers/transcoder-pcie/transcoder_pcie.ko;)

clean:
	make -C vpi clean
	make -C drivers/transcoder-pcie clean

help:
	@echo ""
	@echo "  o make                - make VPI library and pcie driver"
	@echo "  o make vpi            - make VPI library"
	@echo "  o make drivers        - make pcie driver"
	@echo "  o make install        - copy the vpe sdk libs to default search path"
	@echo "                          install the pcie driver"
	@echo "                          run "sudo make install" if permission denied"
	@echo "  o make clean          - make clean VPI and drivers"
	@echo ""
	@echo "  o How to make FFmpeg:"
	@echo "   cd ffmpeg"
	@echo "   ./configure --enable-vpe  --extra-ldflags="-L/lib/vpe" --extra-libs="-lvpi""
	@echo "    make"
