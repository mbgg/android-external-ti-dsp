insmod cmemk.ko phys_start=0x84000000 phys_end=0x85900000 pools=10x4096,2x8192,2x131072,1x3225600,6x1843200 allowOverlap=1
insmod dsplinkk.ko
insmod lpm_omap3530.ko
insmod sdmak.ko
