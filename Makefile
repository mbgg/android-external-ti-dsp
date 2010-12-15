# Copyright (C) 2010 Texas Instruments ITC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

ANDROID_ROOT_DIR := $(PWD)
DVSDK_INSTALL_DIR := $(ANDROID_ROOT_DIR)/external/ti-dsp/ti-dvsdk_dm3730-evm_4_00_00_22
DVSDK_TARGET_DIR:=$(ANDROID_ROOT_DIR)/out/target/product/$(TARGET_PRODUCT)/system/ti-dsp

include $(DVSDK_INSTALL_DIR)/Rules.make

#-------------- Bionic tools -----------------

ANDROID_TOOLCHAIN := $(ANDROID_ROOT_DIR)/$($(combo_target)TOOLS_PREFIX)
ANDROID_TOOLCHAIN_PREFIX := $(shell echo $(ANDROID_TOOLCHAIN) | sed -n 's/.*\///;p')
ANDROID_TOOLCHAIN_PATH := $(shell echo $(ANDROID_TOOLCHAIN) | sed 's/[a-zA-Z\-]*$$//' | sed 's/[\/]*$$//' | sed 's/[a-zA-Z\-]*$$//')
ANDROID_TOOLCHAIN_LONGNAME := bin/$(ANDROID_TOOLCHAIN_PREFIX)gcc

BIONIC_PATH := $(ANDROID_ROOT_DIR)/bionic
BIONIC_LIBC_INCS := \
	-I$(BIONIC_PATH)/libc/arch-arm/include	  \
	-I$(BIONIC_PATH)/libc/include		  \
	-I$(BIONIC_PATH)/libstdc++/include	  \
	-I$(BIONIC_PATH)/libc/kernel/common	  \
	-I$(BIONIC_PATH)/libc/kernel/arch-arm	  \
	-I$(BIONIC_PATH)/libm/include		  \
	-I$(BIONIC_PATH)/libm/arch/arm/include	  \
	-I$(BIONIC_PATH)/libthread_db/include

ANDROID_PRODUCT_PATH=$(ANDROID_ROOT_DIR)/out/target/product
ANDROID_OUT_DIR=$(ANDROID_PRODUCT_PATH)/$(TARGET_PRODUCT)

ANDROID_CC_FLAGS :=			      \
	-DHAVE_ARM_TLS_REGISTER		      \
	-DANDROID			      \
	-D_ANDROID_			      \
	-DSK_RELEASE			      \
	-DNDEBUG			      \
	-UDEBUG				      \
	-fpic				      \
	-ffunction-sections		      \
	-funwind-tables			      \
	-fstack-protector		      \
	-fno-short-enums		      \
	-finline-functions		      \
	-fno-inline-functions-called-once     \
	-fgcse-after-reload		      \
	-frerun-cse-after-loop		      \
	-frename-registers		      \
	-fomit-frame-pointer		      \
	-fno-strict-aliasing		      \
	-finline-limit=64		      \
	-fno-exceptions			      \
	-fmessage-length=0		      \
	-march=armv7-a			      \
	-mfloat-abi=softfp		      \
	-msoft-float			      \
	-mfpu=neon			      \
	-mthumb				      \
	-mthumb-interwork		      \
	-W				      \
	-Wall				      \
	-Wno-unused			      \
	-Winit-self			      \
	-Wpointer-arith			      \
	-Werror=return-type		      \
	-Werror=non-virtual-dtor	      \
	-Werror=address			      \
	-Werror=sequence-point		      \
	-Wstrict-aliasing=2		      \
	-Wno-multichar			      \
	-Wno-missing-field-initializers       \
	-Os				      \
	-g

ANDROID_CPP_FLAGS :=			      \
	$(ANDROID_CC_FLAGS)		      \
	-fvisibility-inlines-hidden	      \
	-fno-rtti			      \
	-Wsign-promo

ANDROID_LD_FLAGS := \
	-lc -lstdc++ -lm -nostdlib -Bdynamic -Wl,-rpath-link=$(ANDROID_OUT_DIR)/obj/lib -L$(ANDROID_OUT_DIR)/obj/lib -Wl,-T,$(ANDROID_ROOT_DIR)/build/core/armelf.x -Wl,-dynamic-linker,/system/bin/linker -Wl,--gc-sections -Wl,-z,nocopyreloc $(ANDROID_ROOT_DIR)/prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin/../lib/gcc/arm-eabi/4.4.0/interwork/libgcc.a $(ANDROID_OUT_DIR)/obj/lib/crtend_android.o $(ANDROID_OUT_DIR)/obj/lib/crtbegin_dynamic.o -L$(ANDROID_ROOT_DIR)/out/target/product/$(TARGET_PRODUCT)/obj/STATIC_LIBRARIES/libasound_intermediates

ANDROID_CFLAGS=$(BIONIC_LIBC_INCS) $(ANDROID_CC_FLAGS)

export ANDROID_TOOLCHAIN_PATH ANDROID_TOOLCHAIN_PREFIX ANDROID_TOOLCHAIN_LONGNAME ANDROID_TOOLCHAIN BIONIC_LIBC_INCS ANDROID_ROOT_DIR ANDROID_CC_FLAGS ANDROID_LD_FLAGS ANDROID_CFLAGS

all:	dvsdk omx_ti install

clean:	dvsdk_clean omx_ti_clean

dvsdk:
	make -C $(DVSDK_INSTALL_DIR) dsplink_arm
	make -C $(DVSDK_INSTALL_DIR) dsplink_dsp
	make -C $(DVSDK_INSTALL_DIR) dsplink_gpp_genpackage
	make -C $(DVSDK_INSTALL_DIR) dsplink_dsp_genpackage
	make -C $(DVSDK_INSTALL_DIR) lpm
	make -C $(DVSDK_INSTALL_DIR) cmem
	make -C $(DVSDK_INSTALL_DIR) sdma
	make -C $(DVSDK_INSTALL_DIR) ce
#	make -C $(DVSDK_INSTALL_DIR) ce_examples
	make -C $(DVSDK_INSTALL_DIR) codecs
	make -C $(DVSDK_INSTALL_DIR) dmai
	make -C $(DVSDK_INSTALL_DIR) c6accel

dvsdk_clean:
	make -C $(DVSDK_INSTALL_DIR) dsplink_clean
	make -C $(DVSDK_INSTALL_DIR) lpm_clean
	make -C $(DVSDK_INSTALL_DIR) cmem_clean
	make -C $(DVSDK_INSTALL_DIR) sdma_clean
	make -C $(DVSDK_INSTALL_DIR) ce_clean
	make -C $(DVSDK_INSTALL_DIR) codecs_clean
	make -C $(DVSDK_INSTALL_DIR) dmai_clean
	make -C $(DVSDK_INSTALL_DIR) c6accel_clean

#-------------- OpenMax components -----------------

OMX_INSTALL_DIR := $(ANDROID_ROOT_DIR)/external/ti-dsp/omx_ti
OMX_VIDEO_INSTALL_DIR := $(OMX_INSTALL_DIR)/video
OMX_AUDIO_INSTALL_DIR := $(OMX_INSTALL_DIR)/audio
OMX_IFACE_INSTALL_DIR := $(OMX_INSTALL_DIR)/interface

OMX_CFLAGS = "-I.. \
	$(BIONIC_LIBC_INCS)	\
	-I$(ANDROID_ROOT_DIR)/frameworks/base/include \
	-I$(ANDROID_ROOT_DIR)/system/core/include \
	-I$(ANDROID_ROOT_DIR)/external/opencore/codecs_v2/omx/omx_common/include \
	-I$(ANDROID_ROOT_DIR)/external/opencore/codecs_v2/omx/omx_queue/src \
	-I$(ANDROID_ROOT_DIR)/external/opencore/codecs_v2/omx/omx_proxy/src \
	-I$(ANDROID_ROOT_DIR)/external/opencore/extern_libs_v2/khronos/openmax/include \
	-I$(ANDROID_ROOT_DIR)/external/opencore/codecs_v2/omx/omx_baseclass/include \
	-I$(ANDROID_ROOT_DIR)/external/opencore/oscl/oscl/osclmemory/src \
	-I$(ANDROID_ROOT_DIR)/external/opencore/oscl/oscl/osclbase/src \
	-I$(ANDROID_ROOT_DIR)/external/opencore/oscl/oscl/osclerror/src \
	-I$(ANDROID_ROOT_DIR)/external/opencore/oscl/oscl/osclproc/src \
	-I$(ANDROID_ROOT_DIR)/external/opencore/oscl/oscl/osclutil/src \
	-I$(ANDROID_ROOT_DIR)/external/opencore/oscl/oscl/oscllib/src \
	-I$(ANDROID_ROOT_DIR)/external/opencore/oscl/pvlogger/src \
	-I$(ANDROID_ROOT_DIR)/external/opencore/android \
	-I$(ANDROID_ROOT_DIR)/external/opencore/engines/common/include \
	-I$(ANDROID_ROOT_DIR)/external/opencore/engines/player/config/core \
	-I$(ANDROID_ROOT_DIR)/external/opencore/engines/player/include \
	-I$(ANDROID_ROOT_DIR)/external/opencore/extern_libs_v2/khronos/openmax/include \
	-I$(ANDROID_ROOT_DIR)/external/opencore/nodes/pvmediaoutputnode/include \
	-I$(ANDROID_ROOT_DIR)/external/opencore/nodes/pvdownloadmanagernode/config/opencore \
	-I$(ANDROID_ROOT_DIR)/external/opencore/pvmi/pvmf/include \
	-I$(ANDROID_ROOT_DIR)/external/opencore/fileformats/mp4/parser/config/opencore \
	-I$(ANDROID_ROOT_DIR)/external/opencore/oscl/oscl/config/android \
	-I$(ANDROID_ROOT_DIR)/external/opencore/oscl/oscl/config/shared \
	-I$(ANDROID_ROOT_DIR)/external/opencore/engines/author/include \
	-I$(ANDROID_ROOT_DIR)/external/opencore/android/drm/oma1/src \
	-I$(ANDROID_ROOT_DIR)/external/opencore/build_config/opencore_dynamic \
	$(ANDROID_CPP_FLAGS) \
	-DENABLE_SHAREDFD_PLAYBACK -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DUSE_CML2_CONFIG -DHAS_OSCL_LIB_SUPPORT -MD"

OMX_LDFLAGS = -nostdlib -Wl,-soname,libagl.so \
	-Wl,-T,$(ANDROID_ROOT_DIR)/build/core/armelf.xsc \
	-Wl,--gc-sections -Wl,-shared,-Bsymbolic -Wl,--no-whole-archive \
	-L$(ANDROID_ROOT_DIR)/out/target/product/$(TARGET_PRODUCT)/obj/lib \
	-lstdc++ -lc -lm

OMX_IFACE_LDFLAGS = "$(OMX_LDFLAGS) -Wl,--no-undefined -llog"

OMX_EXT_LDFLAGS = "$(OMX_LDFLAGS) -lopencore_common -lomx_sharedlibrary -ldspengineiface -llog"

OMX_XDCPATH = ".;$(XDC_INSTALL_DIR)/packages;$(LINK_INSTALL_DIR);$(FC_INSTALL_DIR)/packages;$(CE_INSTALL_DIR)/packages;$(XDAIS_INSTALL_DIR)/packages;$(CODEC_INSTALL_DIR)/packages;$(CMEM_INSTALL_DIR)/packages;$(DMAI_INSTALL_DIR)/packages;$(LPM_INSTALL_DIR)/packages;$(XDC_USER_PATH);$(EDMA3_LLD_INSTALL_DIR)/packages;$(C6ACCEL_INSTALL_DIR)/soc/packages"

.PHONY:	omx_ti
omx_ti:
	make -C $(OMX_IFACE_INSTALL_DIR) \
		OMX_XDCPATH=$(OMX_XDCPATH) \
		XDC_INSTALL_DIR=$(XDC_INSTALL_DIR) \
		CFLAGS=$(OMX_CFLAGS) \
		LDFLAGS=$(OMX_IFACE_LDFLAGS) \
		all
	cp $(OMX_IFACE_INSTALL_DIR)/libdspengineiface.so \
		$(ANDROID_ROOT_DIR)/out/target/product/$(TARGET_PRODUCT)/obj/lib
	make -C $(OMX_VIDEO_INSTALL_DIR) \
		OMX_XDCPATH=$(OMX_XDCPATH) \
		XDC_INSTALL_DIR=$(XDC_INSTALL_DIR) \
		CFLAGS=$(OMX_CFLAGS) \
		LDFLAGS=$(OMX_EXT_LDFLAGS) \
		all
	make -C $(OMX_AUDIO_INSTALL_DIR) \
		OMX_XDCPATH=$(OMX_XDCPATH) \
		XDC_INSTALL_DIR=$(XDC_INSTALL_DIR) \
		CFLAGS=$(OMX_CFLAGS) \
		LDFLAGS=$(OMX_EXT_LDFLAGS) \
		all

omx_ti_clean:
	make -C $(OMX_IFACE_INSTALL_DIR) clean
	make -C $(OMX_VIDEO_INSTALL_DIR) clean
	make -C $(OMX_AUDIO_INSTALL_DIR) clean

# Install resulting binaries in the Android filesystem

install:
	mkdir $(DVSDK_TARGET_DIR) -p
	cp $(LINK_INSTALL_DIR)/dsplink/gpp/export/BIN/Linux/OMAP3530/RELEASE/dsplinkk.ko $(DVSDK_TARGET_DIR)
	cp $(LPM_INSTALL_DIR)/packages/ti/bios/power/modules/omap3530/lpm/lpm_omap3530.ko $(DVSDK_TARGET_DIR)
	cp $(CMEM_INSTALL_DIR)/packages/ti/sdo/linuxutils/cmem/src/module/cmemk.ko $(DVSDK_TARGET_DIR)
	cp $(LINUXUTILS_INSTALL_DIR)/packages/ti/sdo/linuxutils/sdma/src/module/sdmak.ko $(DVSDK_TARGET_DIR)
	cp $(CODEC_INSTALL_DIR)/packages/ti/sdo/server/cs/bin/cs.x64P $(DVSDK_TARGET_DIR)
	cp $(OMX_VIDEO_INSTALL_DIR)/libomx_dsp_video_sharedlibrary.so $(ANDROID_ROOT_DIR)/out/target/product/$(TARGET_PRODUCT)/system/lib
	cp $(OMX_AUDIO_INSTALL_DIR)/libomx_dsp_audio_sharedlibrary.so $(ANDROID_ROOT_DIR)/out/target/product/$(TARGET_PRODUCT)/system/lib
	cp $(OMX_IFACE_INSTALL_DIR)/libdspengineiface.so $(ANDROID_ROOT_DIR)/out/target/product/$(TARGET_PRODUCT)/system/lib
