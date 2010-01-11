# Copyright (C) 2009 Andrei Dolnikov <dolnikov.andrei@gmail.com>
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

#############################################################################################
# Several global defines:
ANDROID_ROOT_DIR := $(PWD)
PATH_TO_SDK := $(ANDROID_ROOT_DIR)/external/ti-dsp/dvsdk_3_00_02_44
LINUXKERNEL_INSTALL_DIR := $(ANDROID_ROOT_DIR)/kernel
DVSDK_INSTALL_DIR:=$(ANDROID_ROOT_DIR)/out/target/product/$(TARGET_PRODUCT)/system/ti-dsp

ifeq ($(TARGET_PRODUCT),omap3evm)
	ANDROID_PLATFORM:=OMAP3530
endif
ifeq ($(TARGET_PRODUCT),beagleboard)
	ANDROID_PLATFORM:=OMAP3530
endif

ifeq ($(ANDROID_PLATFORM),OMAP3530)
	DVSDK_PLATFORM:=o3530_al
	DSPLINK_ARCH:=OMAP3530
	LPM_ARCH:=omap3530
	LPM_MODULE:=lpm_omap3530.ko
	GST_PLATFORM:=omap3530
	GST_XDC_PLATFORM:=evm3530
endif
#############################################################################################


#------------- DSPlink ----------------------
DSPLINK_INSTALL_DIR := $(PATH_TO_SDK)/dsplink_1_61_03
ifeq ($(ANDROID_PLATFORM),OMAP3530)
DSPLINK_CONFIG := --platform=OMAP3530 --nodsp=1 --dspcfg_0=OMAP3530SHMEM --dspos_0=DSPBIOS5XX --gppos=OMAPLSP --comps=ponslrm --trace=0
endif

#-------------- Codec server ----------------
ifeq ($(ANDROID_PLATFORM),OMAP3530)
CODEC_INSTALL_DIR := $(PATH_TO_SDK)/cs1omap3530_1_00_01
endif

#-------------- Codec engine ----------------
CE_INSTALL_DIR := $(PATH_TO_SDK)/codec_engine_2_24_01

#-------------- Framework components -----------
FC_INSTALL_DIR := $(CE_INSTALL_DIR)/cetools

#-------------- Local power manager -------------
LPM_INSTALL_DIR := $(PATH_TO_SDK)/local_power_manager_1_24

#------------------ XDAIS ----------------------
XDAIS_INSTALL_DIR := $(PATH_TO_SDK)/xdais_6_24

#------------------ Bios ----------------------
BIOS_INSTALL_DIR := $(PATH_TO_SDK)/bios_5_33_06

#-------- Code generation tools ----------------
CODEGEN_INSTALL_DIR := $(PATH_TO_SDK)/cg6x

#---------------- Bios utils --------------------
BIOSUTILS_INSTALL_DIR := $(PATH_TO_SDK)/biosutils_1_01_00

#-------------- Linux utils --------------------------
LINUXUTILS_INSTALL_DIR := $(CE_INSTALL_DIR)/cetools

#---------------------- DMAI --------------------------
DMAI_INSTALL_DIR := $(PATH_TO_SDK)/dmai_2_00_01_04

# ------------------ DVSDK demos -----------------------
DVSDK_DEMOS_INSTALL_DIR := $(PATH_TO_SDK)/dvsdk_demos_3_00_01_13

#-------------- XDC Tools -------------------
XDC_INSTALL_DIR := $(PATH_TO_SDK)/xdctools_3_15_01_59
XDCPATH := "$(PATH_TO_SDK);$(XDAIS_INSTALL_DIR)/packages;$(FC_INSTALL_DIR)/packages;$(BIOS_INSTALL_DIR)/packages;$(DSPLINK_INSTALL_DIR)/packages;$(CE_INSTALL_DIR)/packages;$(CE_INSTALL_DIR)/cetools/packages;$(BIOSUTILS_INSTALL_DIR)/packages;$(LPM_INSTALL_DIR)/packages"

#-------------- Gstreamer PLugins ------------
GST_INSTALL_DIR := $(PATH_TO_SDK)/gstreamer_ti

#-------------- Bionic tools -----------------
ANDROID_TOOLCHAIN := $(ANDROID_ROOT_DIR)/$($(combo_target)TOOLS_PREFIX)
ANDROID_TOOLCHAIN_PREFIX := $(shell echo $(ANDROID_TOOLCHAIN) | sed -n 's/.*\///;p')
ANDROID_TOOLCHAIN_PATH := $(shell echo $(ANDROID_TOOLCHAIN) | sed 's/[a-zA-Z\-]*$$//' | sed 's/[\/]*$$//' | sed 's/[a-zA-Z\-]*$$//')
ANDROID_TOOLCHAIN_LONGNAME := bin/$(ANDROID_TOOLCHAIN_PREFIX)gcc

BIONIC_PATH := $(ANDROID_ROOT_DIR)/bionic
BIONIC_LIBC_INCS := -I$(BIONIC_PATH)/libc/arch-arm/include -I$(BIONIC_PATH)/libc/include \
	-I$(BIONIC_PATH)/libstdc++/include \
	-I$(BIONIC_PATH)/libc/kernel/common -I$(BIONIC_PATH)/libc/kernel/arch-arm \
	-I$(BIONIC_PATH)/libm/include \
	-I$(BIONIC_PATH)/libm/arch/arm/include \
	-I$(BIONIC_PATH)/libthread_db/include

ANDROID_PRODUCT_PATH=$(ANDROID_ROOT_DIR)/out/target/product
ANDROID_OUT_DIR=$(ANDROID_PRODUCT_PATH)/$(TARGET_PRODUCT)

ANDROID_CC_FLAGS := \
	-fno-exceptions \
	-Wno-multichar \
	-march=armv5te \
	-mtune=xscale \
	-msoft-float \
	-fpic \
	-mthumb-interwork \
	-ffunction-sections \
	-funwind-tables \
	-fstack-protector \
	-fno-short-enums \
	-D__ARM_ARCH_5__ \
	-D__ARM_ARCH_5T__ \
	-D__ARM_ARCH_5E__ \
	-D__ARM_ARCH_5TE__ \
	-fmessage-length=0 \
	-DUSE_ARM_TLS_REG \
	-finline-functions \
	-fno-inline-functions-called-once \
	-fgcse-after-reload \
	-frerun-cse-after-loop \
	-frename-registers \
	-mthumb \
	-fomit-frame-pointer \
	-fno-strict-aliasing \
	-finline-limit=64 \
	-D_ANDROID_

ANDROID_LD_FLAGS := \
	-lc -lstdc++ -lm -nostdlib -Bdynamic -Wl,-rpath-link=$(ANDROID_OUT_DIR)/obj/lib -L$(ANDROID_OUT_DIR)/obj/lib -Wl,-T,$(ANDROID_ROOT_DIR)/build/core/armelf.x -Wl,-dynamic-linker,/system/bin/linker -Wl,--gc-sections -Wl,-z,nocopyreloc $(ANDROID_ROOT_DIR)/prebuilt/linux-x86/toolchain/arm-eabi-4.3.1/lib/gcc/arm-eabi/4.3.1/interwork/libgcc.a $(ANDROID_OUT_DIR)/obj/lib/crtend_android.o $(ANDROID_OUT_DIR)/obj/lib/crtbegin_dynamic.o 
 	

#used both for building dsplink and samples
BUILD_OPTIONS= \
	BASE_BUILDOS='$(LINUXKERNEL_INSTALL_DIR)' KERNEL_DIR='$(LINUXKERNEL_INSTALL_DIR)' BASE_TOOLCHAIN='$(ANDROID_TOOLCHAIN_PATH)' \
	CROSS_COMPILE=$(ANDROID_TOOLCHAIN_PREFIX) \
	MAKE_OPTS='ARCH=arm CROSS_COMPILE=$(ANDROID_TOOLCHAIN)' \
	USR_CC_FLAGS='$(ANDROID_CC_FLAGS) $(BIONIC_LIBC_INCS)' \
	USR_LD_FLAGS='' \
	EXE_LD_FLAGS='$(ANDROID_LD_FLAGS)' \
	COMPILER='$(ANDROID_TOOLCHAIN)gcc' \
	LD='$(ANDROID_TOOLCHAIN)ld' \
	ARCHIVER='$(ANDROID_TOOLCHAIN)ar' \
	LINKER='$(ANDROID_TOOLCHAIN)gcc'

export ANDROID_TOOLCHAIN_PATH ANDROID_TOOLCHAIN_PREFIX ANDROID_TOOLCHAIN_LONGNAME ANDROID_TOOLCHAIN BIONIC_LIBC_INCS

ifeq ($(ANDROID_PLATFORM),OMAP3530)
all: dsplink_config dsplink_build dsplink_dsp cmem_build sdma_build ce_build fc_build cs_build dmai_build lpm_build ti_gst install
else
$(error "Not supported platform!")
endif

clean: dsplink_clean dsplink_dsp_clean cmem_clean cs_clean dmai_clean lpm_clean fc_clean ce_clean sdma_clean ti_gst_clean


# DSPlink
dsplink_clean:
	DSPLINK=$(DSPLINK_INSTALL_DIR)/packages/dsplink $(XDC_INSTALL_DIR)/gmake -C $(DSPLINK_INSTALL_DIR)/packages/dsplink/gpp/src clean

dsplink_config:
	DSPLINK=$(DSPLINK_INSTALL_DIR)/packages/dsplink perl $(DSPLINK_INSTALL_DIR)/packages/dsplink/config/bin/dsplinkcfg.pl $(DSPLINK_CONFIG)

dsplink_build:
	DSPLINK=$(DSPLINK_INSTALL_DIR)/packages/dsplink $(XDC_INSTALL_DIR)/gmake -C $(DSPLINK_INSTALL_DIR)/packages/dsplink/gpp/src \
	$(BUILD_OPTIONS) debug
	DSPLINK=$(DSPLINK_INSTALL_DIR)/packages/dsplink $(XDC_INSTALL_DIR)/gmake -C $(DSPLINK_INSTALL_DIR)/packages/dsplink/gpp/src \
	$(BUILD_OPTIONS) release

dsplink_dsp_clean:
	DSPLINK=$(DSPLINK_INSTALL_DIR)/packages/dsplink $(XDC_INSTALL_DIR)/gmake -C $(DSPLINK_INSTALL_DIR)/packages/dsplink/dsp/src clean

dsplink_dsp:
	DSPLINK=$(DSPLINK_INSTALL_DIR)/packages/dsplink $(XDC_INSTALL_DIR)/gmake -C $(DSPLINK_INSTALL_DIR)/packages/dsplink/dsp/src \
	BASE_SABIOS=$(BIOS_INSTALL_DIR) XDCTOOLS_DIR=$(XDC_INSTALL_DIR) BASE_CGTOOLS=$(CODEGEN_INSTALL_DIR)

# Build cmem
cmem_build:
	$(XDC_INSTALL_DIR)/gmake -C $(LINUXUTILS_INSTALL_DIR)/packages/ti/sdo/linuxutils/cmem/ \
	LINUXKERNEL_INSTALL_DIR=$(LINUXKERNEL_INSTALL_DIR) \
	UCTOOL_PREFIX=$(ANDROID_TOOLCHAIN) \
	MVTOOL_PREFIX=$(ANDROID_TOOLCHAIN) \
	C_FLAGS="$(ANDROID_CC_FLAGS) $(BIONIC_LIBC_INCS) -I. -I../../include" \
	LD_FLAGS="$(ANDROID_LD_FLAGS)"

cmem_clean:
	$(XDC_INSTALL_DIR)/gmake -C $(LINUXUTILS_INSTALL_DIR)/packages/ti/sdo/linuxutils/cmem clean

# Build sdma
sdma_build:
	$(XDC_INSTALL_DIR)/gmake -C $(LINUXUTILS_INSTALL_DIR)/packages/ti/sdo/linuxutils/sdma/ \
	LINUXKERNEL_INSTALL_DIR=$(LINUXKERNEL_INSTALL_DIR) \
	UCTOOL_PREFIX=$(ANDROID_TOOLCHAIN) \
	MVTOOL_PREFIX=$(ANDROID_TOOLCHAIN) \
	C_FLAGS="$(ANDROID_CC_FLAGS) $(BIONIC_LIBC_INCS) -I. -I../../include" \
	LD_FLAGS="$(ANDROID_LD_FLAGS)"

sdma_clean:
	$(XDC_INSTALL_DIR)/gmake -C $(LINUXUTILS_INSTALL_DIR)/packages/ti/sdo/linuxutils/sdma clean

# Codec server
cs_build:
	$(XDC_INSTALL_DIR)/gmake -C $(CODEC_INSTALL_DIR) \
	DVSDK_INSTALL_DIR=$(PATH_TO_SDK) \
	BIOS_INSTALL_DIR=$(BIOS_INSTALL_DIR) \
	XDC_INSTALL_DIR=$(XDC_INSTALL_DIR) \
	CE_INSTALL_DIR=$(CE_INSTALL_DIR) \
	FC_INSTALL_DIR=$(FC_INSTALL_DIR) \
	CMEM_INSTALL_DIR=$(LINUXUTILS_INSTALL_DIR) \
	CODEC_INSTALL_DIR=$(CODEC_INSTALL_DIR) \
	BIOSUTILS_INSTALL_DIR=$(BIOSUTILS_INSTALL_DIR) \
	XDAIS_INSTALL_DIR=$(XDAIS_INSTALL_DIR) \
	LINK_INSTALL_DIR=$(DSPLINK_INSTALL_DIR) \
	CODEGEN_INSTALL_DIR=$(CODEGEN_INSTALL_DIR)
	XDCARGS=\"prod\"

cs_clean:
	$(XDC_INSTALL_DIR)/gmake -C $(CODEC_INSTALL_DIR) \
	XDC_INSTALL_DIR=$(XDC_INSTALL_DIR) \
	clean

# Framework components
fc_build:
	XDCPATH=$(XDCPATH) XDCOPTIONS=v XDCARGS="prod" XDCBUILDCFG=$(CE_INSTALL_DIR)/config.bld \
	ANDROID_CFLAGS="$(BIONIC_LIBC_INCS) $(ANDROID_CC_FLAGS)" \
	ANDROID_LDFLAGS="$(ANDROID_LD_FLAGS)" \
	CODEGEN_INSTALL_DIR=$(CODEGEN_INSTALL_DIR) \
	ANDROID_TOOLCHAIN="$(ANDROID_TOOLCHAIN_PATH)" \
	$(XDC_INSTALL_DIR)/xdc -PR $(CE_INSTALL_DIR)/cetools/packages

fc_clean:
	XDCPATH=$(XDCPATH) XDCOPTIONS=v XDCARGS="prod" XDCBUILDCFG=$(CE_INSTALL_DIR)/config.bld \
	ANDROID_CFLAGS="$(BIONIC_LIBC_INCS) $(ANDROID_CC_FLAGS)" \
	ANDROID_LDFLAGS="$(ANDROID_LD_FLAGS)" \
	CODEGEN_INSTALL_DIR=$(CODEGEN_INSTALL_DIR) \
	$(XDC_INSTALL_DIR)/xdc clean -PR $(CE_INSTALL_DIR)/cetools/packages

# Codec engine
ce_build:
	XDCPATH=$(XDCPATH) XDCOPTIONS=v XDCARGS="prod" XDCBUILDCFG=$(CE_INSTALL_DIR)/config.bld \
	CODEGEN_INSTALL_DIR=$(CODEGEN_INSTALL_DIR) \
	ANDROID_CFLAGS="$(BIONIC_LIBC_INCS) $(ANDROID_CC_FLAGS)" \
	ANDROID_LDFLAGS="$(ANDROID_LD_FLAGS)" \
	ANDROID_TOOLCHAIN="$(ANDROID_TOOLCHAIN_PATH)" \
	ANDROID_TOOLCHAIN_LONGNAME=$(ANDROID_TOOLCHAIN_LONGNAME) \
	$(XDC_INSTALL_DIR)/xdc -PR $(CE_INSTALL_DIR)/packages/

ce_clean:
	XDCPATH=$(XDCPATH) XDCOPTIONS=v XDCARGS="prod" XDCBUILDCFG=$(CE_INSTALL_DIR)/config.bld \
	CODEGEN_INSTALL_DIR=$(CODEGEN_INSTALL_DIR) \
	ANDROID_CFLAGS="$(BIONIC_LIBC_INCS) $(ANDROID_CC_FLAGS)" \
	ANDROID_LDFLAGS="$(ANDROID_LD_FLAGS)" \
	$(XDC_INSTALL_DIR)/xdc clean -PR $(CE_INSTALL_DIR)/packages/

ce_examples_build:
	XDCPATH=$(XDCPATH) XDCOPTIONS=v XDCARGS="prod" XDCBUILDCFG=$(CE_INSTALL_DIR)/config.bld \
	CODEGEN_INSTALL_DIR=$(CODEGEN_INSTALL_DIR) $(XDC_INSTALL_DIR)/xdc \
	-PR $(CE_INSTALL_DIR)/examples/

# DMAI
dmai_build:
	$(XDC_INSTALL_DIR)/gmake -C $(DMAI_INSTALL_DIR)  PLATFORM=$(DVSDK_PLATFORM) \
        CE_INSTALL_DIR_$(DVSDK_PLATFORM)=$(CE_INSTALL_DIR) \
        CODEC_INSTALL_DIR_$(DVSDK_PLATFORM)=$(CODEC_INSTALL_DIR) \
        LINK_INSTALL_DIR_$(DVSDK_PLATFORM)=$(DSPLINK_INSTALL_DIR) \
        CMEM_INSTALL_DIR_$(DVSDK_PLATFORM)=$(LINUXUTILS_INSTALL_DIR) \
        FC_INSTALL_DIR_$(DVSDK_PLATFORM)=$(FC_INSTALL_DIR) \
        LPM_INSTALL_DIR_$(DVSDK_PLATFORM)=$(LPM_INSTALL_DIR) \
        XDAIS_INSTALL_DIR_$(DVSDK_PLATFORM)=$(XDAIS_INSTALL_DIR) \
        BIOS_INSTALL_DIR_$(DVSDK_PLATFORM)=$(BIOS_INSTALL_DIR) \
        LINUXKERNEL_INSTALL_DIR_$(DVSDK_PLATFORM)=$(LINUXKERNEL_INSTALL_DIR) \
        CROSS_COMPILE_$(DVSDK_PLATFORM)=$(ANDROID_TOOLCHAIN) \
        XDC_INSTALL_DIR_$(DVSDK_PLATFORM)=$(XDC_INSTALL_DIR) \
        ANDROID_CFLAGS="$(BIONIC_LIBC_INCS) $(ANDROID_CC_FLAGS) -I$(ANDROID_ROOT_DIR)/external/alsa-lib/include" \
	ANDROID_LDFLAGS="$(ANDROID_LD_FLAGS) -L$(ANDROID_ROOT_DIR)/out/target/product/$(TARGET_PRODUCT)/obj/STATIC_LIBRARIES/libasound_intermediates" \
        VERBOSE=true all

dmai_clean:
	$(XDC_INSTALL_DIR)/gmake -C $(DMAI_INSTALL_DIR)  PLATFORM=$(DVSDK_PLATFORM) \
        CE_INSTALL_DIR_$(DVSDK_PLATFORM)=$(CE_INSTALL_DIR) \
        CODEC_INSTALL_DIR_$(DVSDK_PLATFORM)=$(CODEC_INSTALL_DIR) \
        LINK_INSTALL_DIR_$(DVSDK_PLATFORM)=$(DSPLINK_INSTALL_DIR) \
        CMEM_INSTALL_DIR_$(DVSDK_PLATFORM)=$(LINUXUTILS_INSTALL_DIR) \
        FC_INSTALL_DIR_$(DVSDK_PLATFORM)=$(FC_INSTALL_DIR) \
        LPM_INSTALL_DIR_$(DVSDK_PLATFORM)=$(LPM_INSTALL_DIR) \
        XDAIS_INSTALL_DIR_$(DVSDK_PLATFORM)=$(XDAIS_INSTALL_DIR) \
        BIOS_INSTALL_DIR_$(DVSDK_PLATFORM)=$(BIOS_INSTALL_DIR) \
        LINUXKERNEL_INSTALL_DIR_$(DVSDK_PLATFORM)=$(LINUXKERNEL_INSTALL_DIR) \
        CROSS_COMPILE_$(DVSDK_PLATFORM)=$(ANDROID_TOOLCHAIN) \
        XDC_INSTALL_DIR_$(DVSDK_PLATFORM)=$(XDC_INSTALL_DIR) \
        ANDROID_CFLAGS="$(BIONIC_LIBC_INCS) $(ANDROID_CC_FLAGS) -I$(ANDROID_ROOT_DIR)/external/alsa-lib/include" \
	ANDROID_LDFLAGS="$(ANDROID_LD_FLAGS) -L$(ANDROID_ROOT_DIR)/out/target/product/$(TARGET_PRODUCT)/obj/STATIC_LIBRARIES/libasound_intermediates" \
        VERBOSE=true clean

# LPM
lpm_build:
	$(XDC_INSTALL_DIR)/gmake -C $(LPM_INSTALL_DIR)/packages/ti/bios/power/modules/$(LPM_ARCH)/lpm \
		LINUXKERNEL_INSTALL_DIR=$(LINUXKERNEL_INSTALL_DIR) \
		MVTOOL_PREFIX=$(ANDROID_TOOLCHAIN) \
		DSPLINK_REPO=$(DSPLINK_INSTALL_DIR)/packages

lpm_clean:
	$(XDC_INSTALL_DIR)/gmake -C $(LPM_INSTALL_DIR)/packages/ti/bios/power/modules/$(LPM_ARCH)/lpm \
	LINUXKERNEL_INSTALL_DIR=$(LINUXKERNEL_INSTALL_DIR) MVTOOL_PREFIX=$(CSTOOL_PREFIX) \
	DSPLINK_REPO=$(DSPLINK_INSTALL_DIR)/packages clean

# Gstreamer plugin
ti_gst:
	make -C $(GST_INSTALL_DIR)/ti_build/ticodecplugin/src PLATFORM=$(GST_PLATFORM) \
	XDC_TARGET=gnu.targets.arm.GCArmv5T \
	XDC_PLATFORM=ti.platforms.$(GST_XDC_PLATFORM) \
	PLATFORM_XDC=${XDC_PLATFORM} \
	MVTOOL_DIR=$(ANDROID_TOOLCHAIN_PATH) \
	ANDROID_TOOLCHAIN_LONGNAME=$(ANDROID_TOOLCHAIN_LONGNAME) \
	CC=$(ANDROID_TOOLCHAIN)gcc \
	CXX=$(ANDROID_TOOLCHAIN)g++ \
	CE_INSTALL_DIR=$(CE_INSTALL_DIR) \
	CODEC_INSTALL_DIR=$(CODEC_INSTALL_DIR) \
	LINK_INSTALL_DIR=$(DSPLINK_INSTALL_DIR) \
	CMEM_INSTALL_DIR=$(LINUXUTILS_INSTALL_DIR) \
	FC_INSTALL_DIR=$(FC_INSTALL_DIR) \
	LPM_INSTALL_DIR=$(LPM_INSTALL_DIR) \
	XDAIS_INSTALL_DIR=$(XDAIS_INSTALL_DIR) \
	BIOS_INSTALL_DIR=$(BIOS_INSTALL_DIR) \
	LINUXKERNEL_INSTALL_DIR=$(LINUXKERNEL_INSTALL_DIR) \
	DMAI_INSTALL_DIR=$(DMAI_INSTALL_DIR) \
	XDC_INSTALL_DIR=$(XDC_INSTALL_DIR) \
	GST_INSTALL_DIR=$(GST_INSTALL_DIR)/ti_build \
	ANDROID_CFLAGS="$(BIONIC_LIBC_INCS) $(ANDROID_CC_FLAGS)" \
	CFLAGS="-DDAVINCI_LSP_WORKAROUND -DPlatform_$(GST_PLATFORM) -I.. \
		$(BIONIC_LIBC_INCS) $(ANDROID_CC_FLAGS) \
		-I$(ANDROID_ROOT_DIR)/external/gstreamer \
		-I$(ANDROID_ROOT_DIR)/external/glib/glib \
		-I$(ANDROID_ROOT_DIR)/external/glib \
		-I$(ANDROID_ROOT_DIR)/external/glib/android \
		-I$(ANDROID_ROOT_DIR)/external/gstreamer/gst/android \
		-I$(ANDROID_ROOT_DIR)/external/glib/gmodule \
		-I$(ANDROID_ROOT_DIR)/external/gst-plugins-base/gst-libs \
		-I$(ANDROID_ROOT_DIR)/external/gstreamer/libs \
		-I/$(ANDROID_ROOT_DIR)/external/gst-plugins-base/gst-libs/gst/video/android" \
	LDFLAGS="-nostdlib -Wl,-soname,libagl.so -Wl,-T,$(ANDROID_ROOT_DIR)/build/core/armelf.xsc \
		-Wl,--gc-sections -Wl,-shared,-Bsymbolic -Wl,--whole-archive -Wl,--no-whole-archive \
		-lc -lstdc++ -lm -Wl,--no-undefined \
		-L$(ANDROID_ROOT_DIR)/out/target/product/$(TARGET_PRODUCT)/obj/lib \
		-lgstreamer-0.10 -lgstaudio-0.10 -lgstvideo-0.10 -lgstbase-0.10 -lgsttag-0.10 \
		-lglib-2.0 -lgthread-2.0 -lgmodule-2.0 -lgobject-2.0" \
	all

ti_gst_clean:
	make -C $(GST_INSTALL_DIR)/ti_build/ticodecplugin/src clean PLATFORM=$(GST_PLATFORM)

install:
	mkdir $(DVSDK_INSTALL_DIR) -p
	cp $(DSPLINK_INSTALL_DIR)/packages/dsplink/gpp/export/BIN/Linux/$(DSPLINK_ARCH)/RELEASE/dsplinkk.ko $(DVSDK_INSTALL_DIR)
	cp $(LPM_INSTALL_DIR)/packages/ti/bios/power/modules/$(LPM_ARCH)/lpm/$(LPM_MODULE) $(DVSDK_INSTALL_DIR)
	cp $(CE_INSTALL_DIR)/cetools/packages/ti/sdo/linuxutils/cmem/src/module/cmemk.ko $(DVSDK_INSTALL_DIR)
	cp $(CE_INSTALL_DIR)/cetools/packages/ti/sdo/linuxutils/sdma/src/module/sdmak.ko $(DVSDK_INSTALL_DIR)
	cp $(CODEC_INSTALL_DIR)/packages/ti/sdo/server/cs/bin/cs.x64P $(DVSDK_INSTALL_DIR)
	cp $(GST_INSTALL_DIR)/ti_build/ticodecplugin/src/libgstticodecplugin.so $(ANDROID_ROOT_DIR)/out/target/product/$(TARGET_PRODUCT)/system/plugins
