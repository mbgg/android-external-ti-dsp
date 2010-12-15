PLATFORM = omap3530
XDC_CONFIG_BASENAME = omx_dsp_iface_$(PLATFORM)
CONFIGURO = $(XDC_INSTALL_DIR)/xs xdc.tools.configuro
CXX = $(ANDROID_TOOLCHAIN)g++

engineiface_objs = \
	iface.o

xdc_config:
	XDCPATH="$(OMX_XDCPATH)" $(CONFIGURO) -c $(ANDROID_TOOLCHAIN_PATH) -o $(XDC_CONFIG_BASENAME) -t gnu.targets.arm.GCArmv5T -p ti.platforms.evm3530 -b ../config.bld omx_dsp_iface_$(PLATFORM).cfg

INCLUDES = $(shell cat $(XDC_CONFIG_BASENAME)/compiler.opt) -I../include

all: xdc_config
	$(CXX) -DHAVE_CONFIG_H $(CFLAGS) $(INCLUDES) -c -o iface.o iface.cpp
	$(CXX) $(LDFLAGS) $(engineiface_objs) -T $(XDC_CONFIG_BASENAME)/linker.cmd -Wl,-soname -Wl,libdspengineiface.so -o libdspengineiface.so

clean:
	rm -rf *.o *.so omx_dsp_iface_$(PLATFORM)
