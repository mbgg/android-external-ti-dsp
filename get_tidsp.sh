#!/bin/bash

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

install_dir=`pwd`/external/ti-dsp

if [ -e "$install_dir"/dvsdk_3_00_02_44 ]
then
	exit
fi

cd "$install_dir"
#### Downloading DVSDK files ####
echo "Downloading DVSDK files..."
# Checking dvsdk
if ! [ -e dvsdk_setuplinux_3_00_02_44.bin ]
then
	wget http://software-dl.ti.com/dsps/dsps_public_sw/sdo_sb/targetcontent/dvsdk/DVSDK_3_00/latest/exports/dvsdk_setuplinux_3_00_02_44.bin
fi

# Checking Code Server
if ! [ -e cs1omap3530_setuplinux_1_00_01-44.bin ]
then
	wget http://software-dl.ti.com/dsps/dsps_public_sw/sdo_sb/targetcontent/dvsdk/DVSDK_3_00/latest/exports/cs1omap3530_setuplinux_1_00_01-44.bin
fi

# Checking xdctools
if ! [ -e xdctools_setuplinux_3_15_01_59.bin ]
then
	wget http://software-dl.ti.com/dsps/dsps_public_sw/sdo_sb/targetcontent/dvsdk/DVSDK_3_00/latest/exports/xdctools_setuplinux_3_15_01_59.bin
fi

# Checking bios
if ! [ -e bios_setuplinux_5_33_06.bin ]
then
	wget http://software-dl.ti.com/dsps/dsps_public_sw/sdo_sb/targetcontent/dvsdk/DVSDK_3_00/latest/exports/bios_setuplinux_5_33_06.bin
fi

# Checking Code generation tools
if ! [ -e TI-C6x-CGT-v6.0.16.1.bin ]
then
	wget http://software-dl.ti.com/dsps/dsps_public_sw/sdo_sb/targetcontent/dvsdk/DVSDK_3_00/latest/exports/TI-C6x-CGT-v6.0.16.1.bin
fi

# Checking mp3dec
if ! [ -e c64xplus_mp3dec_1_31_001_production.bin ]
then
	wget http://software-dl.ti.com/dsps/dsps_public_sw/sdo_sb/targetcontent/dvsdk/DVSDK_3_00/latest/exports/c64xplus_mp3dec_1_31_001_production.bin
fi

# Checking ti gstreamer plugins
if ! [ -e gstreamer_ti ]
then
	svn checkout -r 506 --username anonymous --password "" -q https://gstreamer.ti.com/svn/gstreamer_ti/trunk/gstreamer_ti
fi

#### Checking md5 sums ####
echo "Checking md5 sums..."

if [ -e codec_engine_2_24_01.tar.gz ]
then
	if [[ `md5sum codec_engine_2_24_01.tar.gz | awk '{print$(1)}'` != `cat md5sum.list | grep codec_engine | awk '{print$(1)}'` ]]
	then
		echo "ERROR: codec_engine_2_24_01.tar.gz md5sum not matched!"
		exit
	fi
else
	echo "Please put codec_engine_2_24_01.tar.gz under the external/ti-dsp folder and then restart build"
	exit
fi

if [[ `md5sum dvsdk_setuplinux_3_00_02_44.bin | awk '{print$(1)}'` != `cat md5sum.list | grep dvsdk_setuplinux | awk '{print$(1)}'` ]]
then
	echo "ERROR: dvsdk_setuplinux_3_00_02_44.bin md5sum not matched!"
	exit
fi

if [[ `md5sum cs1omap3530_setuplinux_1_00_01-44.bin | awk '{print$(1)}'` != `cat md5sum.list | grep cs1omap3530_setuplinux | awk '{print$(1)}'` ]]
then
	echo "ERROR: cs1omap3530_setuplinux_1_00_01-44.bin md5sum not matched!"
	exit
fi

if [[ `md5sum xdctools_setuplinux_3_15_01_59.bin | awk '{print$(1)}'` != `cat md5sum.list | grep xdctools_setuplinux | awk '{print$(1)}'` ]]
then
	echo "ERROR: xdctools_setuplinux_3_15_01_59.bin md5sum not matched!"
	exit
fi

if [[ `md5sum bios_setuplinux_5_33_06.bin | awk '{print$(1)}'` != `cat md5sum.list | grep bios_setuplinux | awk '{print$(1)}'` ]]
then
	echo "ERROR: bios_setuplinux_5_33_06.bin md5sum not matched!"
	exit
fi

if [[ `md5sum TI-C6x-CGT-v6.0.16.1.bin | awk '{print$(1)}'` != `cat md5sum.list | grep TI-C6x-CGT | awk '{print$(1)}'` ]]
then
	echo "ERROR: TI-C6x-CGT-v6.0.16.1.bin md5sum not matched!"
	exit
fi

if [[ `md5sum c64xplus_mp3dec_1_31_001_production.bin | awk '{print$(1)}'` != `cat md5sum.list | grep c64xplus_mp3dec_1_31_001_production.bin | awk '{print$(1)}'` ]]
then
	echo "ERROR: c64xplus_mp3dec_1_31_001_production.bin md5sum not matched!"
	exit
fi

#### Changing bin files mode to 755 ####
chmod 755 *.bin

#### Installing DVSDK ####
echo "Installing DVSDK..."

./dvsdk_setuplinux_3_00_02_44.bin --S --prefix "$install_dir"
./cs1omap3530_setuplinux_1_00_01-44.bin --S --prefix "$install_dir"
./xdctools_setuplinux_3_15_01_59.bin --S --prefix "$install_dir"/dvsdk_3_00_02_44
./bios_setuplinux_5_33_06.bin --S --prefix "$install_dir"/dvsdk_3_00_02_44
./install_cg6x.exp &> /dev/null
tar zxvf codec_engine_2_24_01.tar.gz -C "$install_dir"/dvsdk_3_00_02_44 &> /dev/null
mv gstreamer_ti "$install_dir"/dvsdk_3_00_02_44
./install_mp3dec.exp &> /dev/null
tar xvf tmp/dm6446_mp3dec_1_31_001_production.tar -C tmp
cp -a tmp/dm6446_mp3dec_1_31_001_production/packages "$install_dir"/dvsdk_3_00_02_44/cs1omap3530_1_00_01
rm -rf tmp
cd -

#### Patching ####
echo "Patching..."
patch_dir="$install_dir"/patches
cd "$install_dir"/dvsdk_3_00_02_44
ln -s "../gt_dais.h" codec_engine_2_24_01/cetools/packages/ti/sdo/fc/utils/gtinfra/gt_dais.h
patch -p1 < "$patch_dir"/dmai.patch
patch -p1 < "$patch_dir"/dsplink.patch
patch -p1 < "$patch_dir"/codec_engine.patch
patch -p1 < "$patch_dir"/gst_ti.patch
patch -p1 < "$patch_dir"/cs.patch
cd -
