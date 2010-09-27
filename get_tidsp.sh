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

if [ -e "$install_dir"/dvsdk_3_01_00_10 ]
then
	exit
fi

cd "$install_dir"
#### Downloading DVSDK files ####
echo "Downloading DVSDK files..."
# Checking dvsdk
if ! [ -e dvsdk_3_01_00_10_Setup.bin ]
then
	wget http://software-dl.ti.com/dsps/dsps_public_sw/sdo_sb/targetcontent/dvsdk/DVSDK_3_00/latest/exports/dvsdk_3_01_00_10_Setup.bin
fi

# Checking Code Server
if ! [ -e cs1omap3530_setupLinux_1_01_00-prebuilt-dvsdk3.01.00.10.bin ]
then
	wget http://software-dl.ti.com/dsps/dsps_public_sw/sdo_sb/targetcontent/dvsdk/DVSDK_3_00/latest/exports/cs1omap3530_setupLinux_1_01_00-prebuilt-dvsdk3.01.00.10.bin
fi

# Checking Code generation tools
if ! [ -e ti_cgt_c6000_6.1.12_setup_linux_x86.bin ]
then
	wget  http://software-dl.ti.com/dsps/dsps_public_sw/sdo_sb/targetcontent/dvsdk/DVSDK_3_00/latest/exports/ti_cgt_c6000_6.1.12_setup_linux_x86.bin
fi

# Checking mp3dec
#if ! [ -e c64xplus_mp3dec_1_31_001_production.bin ]
#then
#	wget http://software-dl.ti.com/dsps/dsps_public_sw/sdo_sb/targetcontent/dvsdk/DVSDK_3_00/3_00_02_44/exports/c64xplus_mp3dec_1_31_001_production.bin
#fi

# Checking ti gstreamer plugins
if ! [ -e gstreamer_ti ]
then
	svn checkout -r 506 --username anonymous --password "" -q https://gstreamer.ti.com/svn/gstreamer_ti/trunk/gstreamer_ti
fi

#### Checking md5 sums ####
echo "Checking md5 sums..."

if [ -e codec_engine_2_25_05_16.tar.gz ]
then
        if [[ `md5sum codec_engine_2_25_05_16.tar.gz | awk '{print$(1)}'` != `cat md5sum.list | grep codec_engine | awk '{print$(1)}'` ]]
        then
                echo "ERROR: codec_engine_2_25_05_16.tar.gz md5sum not matched!"
                exit
        fi
else
        echo 
        echo "#####################################################################################################"
        echo "# Please put codec_engine_2_25_05_16.tar.gz under the external/ti-dsp folder and then restart build #"
        echo "#####################################################################################################"
        exit
fi

if [[ `md5sum dvsdk_3_01_00_10_Setup.bin | awk '{print$(1)}'` != `cat md5sum.list | grep dvsdk_3_01 | awk '{print$(1)}'` ]]
then
	echo "ERROR: dvsdk_3_01_00_10_Setup.bin md5sum not matched!"
	exit
fi

if [[ `md5sum cs1omap3530_setupLinux_1_01_00-prebuilt-dvsdk3.01.00.10.bin | awk '{print$(1)}'` != `cat md5sum.list | grep cs1omap3530_setupLinux | awk '{print$(1)}'` ]]
then
	echo "ERROR: cs1omap3530_setupLinux_1_01_00-prebuilt-dvsdk3.01.00.10.bin md5sum not matched!"
	exit
fi

if [[ `md5sum ti_cgt_c6000_6.1.12_setup_linux_x86.bin | awk '{print$(1)}'` != `cat md5sum.list | grep ti_cgt_c6000 | awk '{print$(1)}'` ]]
then
	echo "ERROR: ti_cgt_c6000_6.1.12_setup_linux_x86.bin md5sum not matched!"
	exit
fi

#if [[ `md5sum c64xplus_mp3dec_1_31_001_production.bin | awk '{print$(1)}'` != `cat md5sum.list | grep c64xplus_mp3dec_1_31_001_production.bin | awk '{print$(1)}'` ]]
#then
#	echo "ERROR: c64xplus_mp3dec_1_31_001_production.bin md5sum not matched!"
#	exit
#fi

#### Changing bin files mode to 755 ####
chmod 755 *.bin

#### Installing DVSDK ####
echo "Installing DVSDK..."

./install_dvsdk.exp &>/dev/null
./install_cs1omap3530.exp &>/dev/null
./install_cgt.exp &> /dev/null
tar zxvf codec_engine_2_25_05_16.tar.gz -C "$install_dir"/dvsdk_3_01_00_10 &> /dev/null
cp -a gstreamer_ti "$install_dir"/dvsdk_3_01_00_10/
#./install_mp3dec.exp &> /dev/null
#tar xvf tmp/dm6446_mp3dec_1_31_001_production.tar -C tmp
#cp -a tmp/dm6446_mp3dec_1_31_001_production/packages "$install_dir"/dvsdk_3_00_02_44/cs1omap3530_1_00_01
#rm -rf tmp
cd -

#### Patching ####
echo "Patching..."
patch_dir="$install_dir"/patches
cd "$install_dir"/dvsdk_3_01_00_10
ln -s "../gt_dais.h" codec_engine_2_25_05_16/cetools/packages/ti/sdo/fc/utils/gtinfra/gt_dais.h
patch -p1 < "$patch_dir"/dmai.patch
patch -p1 < "$patch_dir"/dsplink.patch
patch -p1 < "$patch_dir"/codec_engine.patch
patch -p1 < "$patch_dir"/gst_ti.patch
patch -p1 < "$patch_dir"/0001-omx-base.patch
patch -p1 < "$patch_dir"/0002-omx-dsp-audio.patch
patch -p1 < "$patch_dir"/0003-omx-dsp-video.patch
patch -p1 < "$patch_dir"/0004-omx-dsp-interface.patch

cd -
