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
dm37x_dvsdk_version=dvsdk_dm3730-evm_04_03_00_06
omap35x_dvsdk_version=dvsdk_omap3530-evm_4_01_00_09

cd "$install_dir" || exit 1

if [ "$OMAPES" == "5.x" ]; then
    dvsdk_version=$dm37x_dvsdk_version
else
    dvsdk_version=$omap35x_dvsdk_version
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
    ./install_dvsdk4.exp $dvsdk_version

    echo "Patching DVSDK components for Android..."
    cd $install_dir/ti-$dvsdk_version
    check_status

    for file in $install_dir/patches/*; do
        if [ $(basename $file) == "0001-Android-modifications-to-DVSDK-build-system-dm37x.patch" -a "$OMAPES" != "5.x" ]; then
            continue
        fi
        if [ $(basename $file) == "0001-Android-modifications-to-DVSDK-build-system-omap35x.patch" -a "$OMAPES" == "5.x" ]; then
            continue
        fi
        patch -p1 < $file
        check_status
    done

    sed -i -e  "s~CSTOOL_DIR=.*$~CSTOOL_DIR=${TOOLS_DIR}~g" -e 's~CSTOOL_PREFIX=\$(CSTOOL_DIR)/bin/arm-arago-linux-gnueabi-~CSTOOL_PREFIX=\$\(CSTOOL_DIR\)/bin/arm-eabi-~g' ./Rules.make
    check_status
fi

cd $root_dir
exit 0
