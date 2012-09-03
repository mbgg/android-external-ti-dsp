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
ifeq ($(OMAPES), 5.x)
DVSDK_INSTALL_DIR := $(ANDROID_ROOT_DIR)/external/ti-dsp/ti-dvsdk_dm3730-evm_04_03_00_06
else
DVSDK_INSTALL_DIR := $(ANDROID_ROOT_DIR)/external/ti-dsp/ti-dvsdk_omap3530-evm_4_01_00_09
endif
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
    -fuse-ld=bfd -lc -lstdc++ -lm -nostdlib -Bdynamic -Wl,-rpath-link=$(ANDROID_OUT_DIR)/obj/lib -L$(ANDROID_OUT_DIR)/obj/lib -Wl,-T,$(ANDROID_ROOT_DIR)/build/core/armelf.x -Wl,-dynamic-linker,/system/bin/linker -Wl,--gc-sections -Wl,-z,nocopyreloc $(ANDROID_ROOT_DIR)/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/../lib/gcc/arm-eabi/4.4.3/libgcc.a $(ANDROID_OUT_DIR)/obj/lib/crtend_android.o $(ANDROID_OUT_DIR)/obj/lib/crtbegin_dynamic.o -L$(ANDROID_ROOT_DIR)/out/target/product/$(TARGET_PRODUCT)/obj/STATIC_LIBRARIES/libasound_intermediates

ANDROID_CFLAGS=$(BIONIC_LIBC_INCS) $(ANDROID_CC_FLAGS)

export ANDROID_TOOLCHAIN_PATH ANDROID_TOOLCHAIN_PREFIX ANDROID_TOOLCHAIN_LONGNAME ANDROID_TOOLCHAIN BIONIC_LIBC_INCS ANDROID_ROOT_DIR ANDROID_CC_FLAGS ANDROID_LD_FLAGS ANDROID_CFLAGS

all:	dvsdk  install

clean:	dvsdk_clean 

dvsdk:
	make -C $(DVSDK_INSTALL_DIR) dsplink_arm
	make -C $(DVSDK_INSTALL_DIR) dsplink_dsp
	make -C $(DVSDK_INSTALL_DIR) dsplink_gpp_genpackage
	make -C $(DVSDK_INSTALL_DIR) dsplink_dsp_genpackage
	make -C $(DVSDK_INSTALL_DIR) lpm
	make -C $(DVSDK_INSTALL_DIR) CFLAGS_MODULE=-fno-pic cmem
	make -C $(DVSDK_INSTALL_DIR) CFLAGS_MODULE=-fno-pic sdma
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

distclean:
	rm -rf $(DVSDK_INSTALL_DIR) already_clean

# Install resulting binaries in the Android filesystem

install:
	mkdir $(DVSDK_TARGET_DIR) -p
	cp $(LINK_INSTALL_DIR)/dsplink/gpp/export/BIN/Linux/OMAP3530/RELEASE/dsplinkk.ko $(DVSDK_TARGET_DIR)
	cp $(LPM_INSTALL_DIR)/packages/ti/bios/power/modules/omap3530/lpm/lpm_omap3530.ko $(DVSDK_TARGET_DIR)
	cp $(CMEM_INSTALL_DIR)/packages/ti/sdo/linuxutils/cmem/src/module/cmemk.ko $(DVSDK_TARGET_DIR)
	cp $(LINUXUTILS_INSTALL_DIR)/packages/ti/sdo/linuxutils/sdma/src/module/sdmak.ko $(DVSDK_TARGET_DIR)
	cp $(CODEC_INSTALL_DIR)/packages/ti/sdo/server/cs/bin/cs.x64P $(DVSDK_TARGET_DIR)
