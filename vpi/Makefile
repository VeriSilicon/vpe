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

PWD = $(shell pwd)

VC8000D = $(PWD)/../sdk_inc/VC8000D/software
VC8000E = $(PWD)/../sdk_inc/VC8000E/software
BIGSEA = $(PWD)/../sdk_inc/Bigsea/software
COMMON = $(PWD)/../sdk_inc/common
DRIVERS = $(PWD)/../drivers

ifeq ($(ARCH), )
ARCH = x86_64
endif

CC  ?= $(CROSS_COMPILE)gcc
RM  ?= rm -f
AR  ?= $(CROSS_COMPILE)ar rcs

FBIP_ENABLE = y
PERFORMANCE = y

DEBFLAGS += -Werror -fPIC -Wno-unused -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-strict-overflow -Wno-shift-negative-value

ifeq ($(DEBUG),y)
	DEBFLAGS += -O0 -g -DDEBUG
else
	DEBFLAGS += -O2 -g -DNDEBUG
endif

CFLAGS  = -Wall -D_GNU_SOURCE -D_REENTRANT -D_THREAD_SAFE \
           $(DEBFLAGS) $(INCLUDE) -fPIC
           CFLAGS += -Wmissing-field-initializers -std=gnu99

ifeq ($(ERROR_TEST),y)
	CFLAGS += -DERROR_TEST
endif

ifeq ($(FBIP_ENABLE),y)
	CFLAGS += -DSUPPORT_TCACHE -DSUPPORT_DEC400 -DSUPPORT_CACHE -DSUPPORT_L2CACHE -DDRV_NEW_ARCH -DFPGA
endif

ifeq ($(PERFORMANCE),y)
	CFLAGS += -DFB_PERFORMANCE_STATIC
endif

ifeq ($(CHECK_MEM_LEAK), y)
	CFLAGS += -DCHECK_MEM_LEAK_TRANS
endif

LDFLAGS := -L./../sdk_libs/$(ARCH)/

OBJS = $(SRCS:.c=.o)

INCLUDE =  	-I$(VC8000E) \
		-I$(VC8000E)/inc \
		-I$(VC8000E)/source/hevc \
		-I$(VC8000E)/source/vp9 \
		-I$(VC8000E)/source/jpeg \
		-I$(VC8000E)/source/common \
		-I$(VC8000E)/linux_reference/ewl \
		-I$(PWD)/inc \
		-I$(PWD)/utils/inc \
		-I$(PWD)/src \
		-I$(PWD)/src/dec \
		-I$(PWD)/src/enc \
		-I$(PWD)/src/filter \
		-I$(VC8000D)/source/inc \
		-I$(VC8000D)/source/pp \
		-I$(VC8000D)/source/config \
		-I$(VC8000D)/source/h264high \
		-I$(VC8000D)/source/h264high/legacy \
		-I$(VC8000D)/linux/h264high \
		-I$(VC8000D)/source/common \
		-I$(COMMON)/inc \
		-I$(DRIVERS)/transcoder-pcie \
		-I$(BIGSEA)/inc

#INCLUDE += -Ikernel_module -Imemalloc

vpath %.c src \
          src/dec \
          utils/src \
          src/enc \
	  src/filter

SRCS := src/vpi.c \
        src/dec/vpi_video_dec.c \
	src/dec/vpi_video_dec_buffer.c \
	src/dec/vpi_video_dec_cfg.c \
	src/dec/vpi_video_dec_info.c \
	src/dec/vpi_video_dec_picture_consume.c \
	src/dec/vpi_video_dec_pp.c \
	src/dec/vpi_video_dec_tb_defs.c \
	src/dec/vpi_video_h264dec.c \
	src/dec/vpi_video_hevcdec.c \
	src/dec/vpi_video_vp9dec.c \
	utils/src/log_manager.c \
	src/enc/vpi_video_h26xenc.c \
	src/enc/vpi_video_h26xenc_cfg.c \
	src/enc/vpi_video_h26xenc_utils.c \
	src/enc/vpi_video_h26xenc_options.c \
	src/enc/vpi_video_enc_common.c \
	src/enc/vpi_video_vp9enc.c \
	src/enc/vpi_video_vp9enc_utils.c \
	src/filter/vpi_video_prc.c \
	src/filter/vpi_video_hwdwprc.c \
	src/filter/vpi_video_pp.c \
	src/filter/vpi_video_hwulprc.c

LIBS += -lg2h264 -lg2hevc -lg2vp9 -lpp -lg2common -ldwlg2 -lhugetlbfs -lh2enc -lenc -lcwl -lhal -lsyslog -lpthread -lm

TARGET = libvpi.so

all: $(OBJS)
	  $(CC) $(DEBFLAGS) -shared -o $(TARGET) $(OBJS) $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(ENVSET) $(CONFFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) *.o *.so
	$(RM) src/*.o
	$(RM) src/dec/*.o
	$(RM) src/enc/*.o
	$(RM) src/filter/*.o
	$(RM) utils/src/*.o
