PLATFORM = omap3530
XDC_CONFIG_BASENAME = omx_dsp_video_$(PLATFORM)
CONFIGURO = $(XDC_INSTALL_DIR)/xs xdc.tools.configuro
CXX = $(ANDROID_TOOLCHAIN)g++

openmaxticomponent_objs = \
	omx-dsp-video.o		\
	omx-dsp-mpeg4.o		\
	omx-dsp-h264.o 

xdc_config:
	XDCPATH="$(OMX_XDCPATH)" $(CONFIGURO) -c $(ANDROID_TOOLCHAIN_PATH) -o $(XDC_CONFIG_BASENAME) -t gnu.targets.arm.GCArmv5T -p ti.platforms.evm3530 -b ../config.bld omx_dsp_video_$(PLATFORM).cfg

INCLUDES = $(shell cat $(XDC_CONFIG_BASENAME)/compiler.opt) -I../include

all: xdc_config
	$(CXX) -DHAVE_CONFIG_H $(CFLAGS) $(INCLUDES) -c -o omx-dsp-video.o omx-dsp-video.cpp
	$(CXX) -DHAVE_CONFIG_H $(CFLAGS) $(INCLUDES) -c -o omx-dsp-mpeg4.o omx-dsp-mpeg4.cpp
	$(CXX) -DHAVE_CONFIG_H $(CFLAGS) $(INCLUDES) -c -o omx-dsp-h264.o omx-dsp-h264.cpp
	$(CXX) $(LDFLAGS) $(openmaxticomponent_objs) -T $(XDC_CONFIG_BASENAME)/linker.cmd -Wl,-soname -Wl,libomx_dsp_video_sharedlibrary.so -o libomx_dsp_video_sharedlibrary.so

clean:
	rm -rf *.o *.so omx_dsp_video_$(PLATFORM)
