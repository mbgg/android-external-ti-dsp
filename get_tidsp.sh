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

check_status() {
    if [ "$?" -ne "0" ]; then
        echo "Failed get_tidsp.sh, aborting.."
        exit 1
    fi
}

root_dir=`pwd`
install_dir=$root_dir/external/ti-dsp
dvsdk_version=dvsdk_dm3730-evm_4_00_00_22

cd "$install_dir"

# Download and install the CodeSourcery toolchain as it's required by DVSDK 4.00
if ! [ -d "arm-2009q1" ]; then
    if ! [ -e arm-2009q1-203-arm-none-linux-gnueabi-i686-pc-linux-gnu.tar.bz2 ]
    then
        echo "Downloading CodeSourcery toolchain..."
        wget http://www.codesourcery.com/sgpp/lite/arm/portal/package4571/public/arm-none-linux-gnueabi/arm-2009q1-203-arm-none-linux-gnueabi-i686-pc-linux-gnu.tar.bz2
        check_status
    fi

    if [[ `md5sum arm-2009q1-203-arm-none-linux-gnueabi-i686-pc-linux-gnu.tar.bz2 | awk '{print$(1)}'` != `cat md5sum.list | grep arm-2009q1-203-arm-none-linux-gnueabi-i686-pc-linux-gnu.tar.bz2 | awk '{print$(1)}'` ]]
    then
        echo "ERROR: arm-2009q1-203-arm-none-linux-gnueabi-i686-pc-linux-gnu.tar.bz2 md5sum not matched!"
        exit 1
    fi

    echo "Installing CodeSourcery..."
    tar xjf arm-2009q1-203-arm-none-linux-gnueabi-i686-pc-linux-gnu.tar.bz2 -C "$install_dir"
    check_status
fi

# Install the DVSDK using the download DVSDK installer
if ! [ -d "ti-$dvsdk_version" ]; then
    if ! [ -e "${dvsdk_version}_setuplinux" ]; then
        echo "Didn't find ${dvsdk_version}_setuplinux under external/ti-dsp, aborting.."
        exit 1
    fi

    if [[ `md5sum ${dvsdk_version}_setuplinux | awk '{print$(1)}'` != `cat md5sum.list | grep ${dvsdk_version}_setuplinux | awk '{print$(1)}'` ]]
    then
        echo "ERROR: ${dvsdk_version}_setuplinux md5sum not matched!"
        exit 1
    fi

    if ! [ -x "${dvsdk_version}_setuplinux" ]; then
        chmod 755 ${dvsdk_version}_setuplinux
        check_status
    fi

    echo "Installing DVSDK..."
    ./install_dvsdk4.exp

    echo "Patching DVSDK components for Android..."
    cd $install_dir/ti-$dvsdk_version
    check_status

    for file in $install_dir/patches/*; do
        patch -p1 < $file
        check_status
    done
fi

cd $root_dir
exit 0
