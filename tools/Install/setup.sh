#!/usr/bin/env bash

#
# Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License").
# You may not use this file except in compliance with the License.
# A copy of the License is located at
#
#  http://aws.amazon.com/apache2.0
#
# or in the "license" file accompanying this file. This file is distributed
# on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
# express or implied. See the License for the specific language governing
# permissions and limitations under the License.
#

#
# Modified by XMOS Ltd
# https://github.com/xmos/avs-device-sdk
#

set -o errexit  # Exit the script if any statement fails.
set -o nounset  # Exit the script if any uninitialized variable is used.

CLONE_URL=${CLONE_URL:- 'https://github.com/xmos/avs-device-sdk.git'}

PORT_AUDIO_FILE="pa_stable_v190600_20161030.tgz"
PORT_AUDIO_DOWNLOAD_URL="http://www.portaudio.com/archives/$PORT_AUDIO_FILE"


BUILD_TESTS=${BUILD_TESTS:-'true'}

pushd "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null
CURRENT_DIR="$( pwd )"
THIS_SCRIPT="$CURRENT_DIR/setup.sh"
popd > /dev/null

INSTALL_BASE=${INSTALL_BASE:-"$HOME/sdk-folder"}
SOURCE_FOLDER=${SDK_LOC:-''}
THIRD_PARTY_FOLDER=${THIRD_PARTY_LOC:-'third-party'}
BUILD_FOLDER=${BUILD_FOLDER:-'sdk-build'}
SOUNDS_FOLDER=${SOUNDS_FOLDER:-'sounds'}
DB_FOLDER=${DB_FOLDER:-'db'}

SOURCE_PATH="$INSTALL_BASE/$SOURCE_FOLDER"
THIRD_PARTY_PATH="$INSTALL_BASE/$THIRD_PARTY_FOLDER"
BUILD_PATH="$INSTALL_BASE/$BUILD_FOLDER"
SOUNDS_PATH="$INSTALL_BASE/$SOUNDS_FOLDER"
DB_PATH="$INSTALL_BASE/$DB_FOLDER"
CONFIG_DB_PATH="$DB_PATH"
UNIT_TEST_MODEL_PATH="$INSTALL_BASE/avs-device-sdk/KWD/inputs/SensoryModels/"
UNIT_TEST_MODEL="$THIRD_PARTY_PATH/alexa-rpi/models/spot-alexa-rpi-31000.snsr"
INPUT_CONFIG_FILE="$SOURCE_PATH/avs-device-sdk/Integration/AlexaClientSDKConfig.json"
OUTPUT_CONFIG_FILE="$BUILD_PATH/Integration/AlexaClientSDKConfig.json"
TEMP_CONFIG_FILE="$BUILD_PATH/Integration/tmp_AlexaClientSDKConfig.json"
TEST_SCRIPT="$INSTALL_BASE/test.sh"
LIB_SUFFIX="a"
ANDROID_CONFIG_FILE=""
PATH_FILES_DIR="$HOME/.config/"
VOCALFUSION_3510_SALES_DEMO_PATH_FILE="$PATH_FILES_DIR/vocalfusion_3510_sales_demo_path"
VOCALFUSION_3510_AVS_SETUP_PATH_FILE="$PATH_FILES_DIR/vocalfusion_3510_avs_setup_path"

PI_HAT_CTRL_PATH="$THIRD_PARTY_PATH/pi_hat_ctrl"
ALIASES="$HOME/.bash_aliases"

# Default value for XMOS device
XMOS_DEVICE="xvf3510"

# Default device serial number if nothing is specified
DEVICE_SERIAL_NUMBER="123456"

# Default device manufacturer name
DEVICE_MANUFACTURER_NAME=${DEVICE_MANUFACTURER_NAME:-"Test Manufacturer"}

# Default device description
DEVICE_DESCRIPTION=${DEVICE_DESCRIPTION:-"Test Device"}

GSTREAMER_AUDIO_SINK="autoaudiosink"

build_port_audio() {
  # build port audio
  echo
  echo "==============> BUILDING PORT AUDIO =============="
  echo
  pushd $THIRD_PARTY_PATH
  wget -c $PORT_AUDIO_DOWNLOAD_URL
  tar zxf $PORT_AUDIO_FILE

  pushd portaudio
  ./configure --without-jack
  make
  popd
  popd
}

get_platform() {
  uname_str=`uname -a`
  result=""

  if [[ "$uname_str" ==  "Linux "* ]] && [[ -f /etc/os-release ]]
  then
    sys_id=`cat /etc/os-release | grep "^ID="`
    if [[ "$sys_id" == "ID=raspbian" ]]
    then
      echo "Raspberry pi"
    fi
  elif [[ "$uname_str" ==  "MINGW64"* ]]
  then
    echo "Windows mingw64"
  fi
}

show_help() {
  echo  'Usage: setup.sh <config-json-file> <xmos-tag> [OPTIONS]'
  echo  'The <config-json-file> can be downloaded from developer portal and must contain the following:'
  echo  '   "clientId": "<OAuth client ID>"'
  echo  '   "productId": "<your product name for device>"'
  echo  ' The  <xmos-tag> is the tag in the GIT repository xmos/avs-device-sdk'
  echo  ''
  echo  'Optional parameters'
  echo  '  -s <serial-number>    If nothing is provided, the default device serial number is 123456'
  echo  '  -a <file-name>        The file that contains Android installation configurations (e.g. androidConfig.txt)'
  echo  '  -d <description>      The description of the device.'
  echo  '  -m <manufacturer>     The device manufacturer name.'
  echo  '  -x <xmos-device-type> XMOS device to setup: default xvf3510, possible value xvf3500'
  echo  '  -h                    Display this help and exit'
}

if [[ $# -lt 2 ]]; then
    show_help
    exit 1
fi

CONFIG_JSON_FILE=$1
if [ ! -f "$CONFIG_JSON_FILE" ]; then
    echo "Config json file not found!"
    show_help
    exit 1
fi

XMOS_TAG=$2

shift 2

OPTIONS=s:a:m:d:hx:
while getopts "$OPTIONS" opt ; do
    case $opt in
        s )
            DEVICE_SERIAL_NUMBER="$OPTARG"
            ;;
        a )
            ANDROID_CONFIG_FILE="$OPTARG"
            if [ ! -f "$ANDROID_CONFIG_FILE" ]; then
                echo "Android config file is not found!"
                exit 1
            fi
            source $ANDROID_CONFIG_FILE
            ;;
        d )
            DEVICE_DESCRIPTION="$OPTARG"
            ;;
        m )
            DEVICE_MANUFACTURER_NAME="$OPTARG"
            ;;
        x )
            XMOS_DEVICE="$OPTARG"
            ;;
        h )
            show_help
            exit 1
            ;;
    esac
done

if [[ ! "$DEVICE_SERIAL_NUMBER" =~ [0-9a-zA-Z_]+ ]]; then
   echo 'Device serial number is invalid!'
   exit 1
fi

# The target platform for the build.
PLATFORM=${PLATFORM:-$(get_platform)}

if [ "$PLATFORM" == "Raspberry pi" ]
then
  source pi.sh
elif [ "$PLATFORM" == "Windows mingw64" ]
then
  source mingw.sh
  else
    PLATFORM_LOWER=$(echo "${PLATFORM}" | tr '[:upper:]' '[:lower:]')
    if [ "$PLATFORM_LOWER" == "android" ]
    then
      PLATFORM="Android"
      source android.sh
    else
      echo "The installation script doesn't support current system. (System: $(uname -a))"
      exit 1
    fi
fi

echo "################################################################################"
echo "################################################################################"
echo ""
echo ""
echo "AVS Device SDK $PLATFORM Script - Terms and Agreements"
echo ""
echo ""
echo "The AVS Device SDK is dependent on several third-party libraries, environments, "
echo "and/or other software packages that are installed using this script from "
echo "third-party sources (\"External Dependencies\"). These are terms and conditions "
echo "associated with the External Dependencies "
echo "(available at https://github.com/alexa/avs-device-sdk/wiki/Dependencies) that "
echo "you need to agree to abide by if you choose to install the External Dependencies."
echo ""
echo ""
echo "If you do not agree with every term and condition associated with the External "
echo "Dependencies, enter \"QUIT\" in the command line when prompted by the installer."
echo "Else enter \"AGREE\"."
echo ""
echo ""
echo "################################################################################"
echo "################################################################################"

read input
input=$(echo $input | awk '{print tolower($0)}')
if [ $input == 'quit' ]
then
  exit 1
elif [ $input == 'agree' ]
then
  echo "################################################################################"
  echo "Proceeding with installation"
  echo "################################################################################"
else
  echo "################################################################################"
  echo 'Unknown option'
  echo "################################################################################"
  exit 1
fi

echo
echo "==============> CREATING FILE WITH AVS SETUP PATH ============"
echo
echo "$CURRENT_DIR" > $VOCALFUSION_3510_AVS_SETUP_PATH_FILE

echo
echo "==============> CREATING AUTOSTART SCRIPT ============"
echo


# Create autostart script
AUTOSTART_SESSION="avsrun"
AUTOSTART_DIR=$HOME/.config/lxsession/LXDE-pi
AUTOSTART=$AUTOSTART_DIR/autostart
AVSRUN_CMD="lxterminal -t avsrun -e \"$BUILD_PATH/SampleApp/src/SampleApp $OUTPUT_CONFIG_FILE $THIRD_PARTY_PATH/alexa-rpi/models NONE 12 \$*\" &" #$* is for passing any extra arguments to Sampleapp through .avsrun-startup.sh shell script
STARTUP_SCRIPT=$CURRENT_DIR/.avsrun-startup.sh
if [ ! -f $AUTOSTART ]; then
  mkdir -p $AUTOSTART_DIR
  cp /etc/xdg/lxsession/LXDE-pi/autostart $AUTOSTART
fi
cat << EOF > "$STARTUP_SCRIPT"
#!/bin/bash
$AVSRUN_CMD
EOF

# If vocalfusion_3510_sales_demo is present, modify .avsrun-startup.sh to start sales demo along with AVS
if [ -f $VOCALFUSION_3510_SALES_DEMO_PATH_FILE ]; then
  SALES_DEMO_PATH=`cat $VOCALFUSION_3510_SALES_DEMO_PATH_FILE`
  sed -i '/'"offline_demo"'/d' $STARTUP_SCRIPT
  echo "lxterminal -t offline_demo -e \"$SALES_DEMO_PATH/"run_demo.sh" --with-avs\" &" >> $STARTUP_SCRIPT
fi

chmod a+rx $STARTUP_SCRIPT

while true; do
  read -p "Automatically run AVS SDK at startup (y/n)? " ANSWER
  case ${ANSWER:0:1} in
    N|n )
      if grep $AUTOSTART_SESSION $AUTOSTART; then 
        # Remove startup script from autostart file
        sed -i '/'"$AUTOSTART_SESSION"'/d' $AUTOSTART
      fi
      break
      ;;
    Y|y )
      if [ ! -f $VOCALFUSION_3510_SALES_DEMO_PATH_FILE ]; then # Sales demo is not present
        # Remove existing lines containing avs
        sed -i '/'"$AUTOSTART_SESSION"'/d' $AUTOSTART
        # Append avs startup script in autostart file
        echo "@$STARTUP_SCRIPT" >> $AUTOSTART
      else # Vocalfusion demo present and we're adding avs so update the startup.py call in $AUTOSTART
        SALES_DEMO_PATH=`cat $VOCALFUSION_3510_SALES_DEMO_PATH_FILE`
        sales_demo_startup_string="@lxterminal -t startup -e python3 $SALES_DEMO_PATH/startup.py --avsrun-startup-file $STARTUP_SCRIPT"
        sed -i '/'"vocalfusion_3510_sales_demo"'/d' $AUTOSTART
        sed -i '/'"$AUTOSTART_SESSION"'/d' $AUTOSTART
        echo "$sales_demo_startup_string" >> $AUTOSTART
      fi
      break
      ;;
  esac
done

SENSORY_OP_POINT_FLAG="-DSENSORY_OP_POINT=ON"
XMOS_AVS_TESTS_FLAG="-DXMOS_AVS_TESTS=ON"
if [ $XMOS_DEVICE = "xvf3510" ]
then
  PI_HAT_FLAG="-DPI_HAT_CTRL=ON"
else
  PI_HAT_FLAG=""
fi

if [ ! -d "$BUILD_PATH" ]
then

  # Make sure required packages are installed
  echo "==============> INSTALLING REQUIRED TOOLS AND PACKAGE ============"
  echo

  install_dependencies

  # create / paths
  echo
  echo "==============> CREATING PATHS AND GETTING SOUND FILES ============"
  echo

  mkdir -p $SOURCE_PATH
  mkdir -p $THIRD_PARTY_PATH
  mkdir -p $SOUNDS_PATH
  mkdir -p $DB_PATH

  run_os_specifics

  if [ ! -d "${SOURCE_PATH}/avs-device-sdk" ]
  then
    #get sdk
    echo
    echo "==============> CLONING SDK =============="
    echo

    cd $SOURCE_PATH
    git clone -b $XMOS_TAG $CLONE_URL
    if [ $XMOS_DEVICE = "xvf3510" ]
    then
      echo
      echo "==============> BUILDING PI HAT CONTROL =============="
      echo

      mkdir -p $PI_HAT_CTRL_PATH
      pushd $SOURCE_PATH/avs-device-sdk/ThirdParty/pi_hat_ctrl > /dev/null
      gcc pi_hat_ctrl.c -o $PI_HAT_CTRL_PATH/pi_hat_ctrl -lm
      popd > /dev/null
    fi
  fi

  # make the SDK
  echo
  echo "==============> BUILDING SDK =============="
  echo
  mkdir -p $BUILD_PATH
  cd $BUILD_PATH
  cmake "$SOURCE_PATH/avs-device-sdk" \
      $SENSORY_OP_POINT_FLAG \
      $PI_HAT_FLAG \
      $XMOS_AVS_TESTS_FLAG \
      -DCMAKE_BUILD_TYPE=DEBUG \
      "${CMAKE_PLATFORM_SPECIFIC[@]}"

  cd $BUILD_PATH
# remove -j2 option to allow building in Raspberry Pi3
  make SampleApp
  make PreviewAlexaClient

else
  cd $BUILD_PATH
# remove -j2 option to allow building in Raspberry Pi3
  make SampleApp
  make PreviewAlexaClient
fi

echo
echo "==============> SAVING CONFIGURATION FILE =============="
echo

# Create configuration file with audioSink configuration at the beginning of the file
cat << EOF > "$OUTPUT_CONFIG_FILE"
 {
    "gstreamerMediaPlayer":{
        "audioSink":"$GSTREAMER_AUDIO_SINK"
    },
EOF

cd $CURRENT_DIR
bash genConfig.sh config.json $DEVICE_SERIAL_NUMBER $CONFIG_DB_PATH $SOURCE_PATH/avs-device-sdk $TEMP_CONFIG_FILE \
  -DSDK_CONFIG_MANUFACTURER_NAME="$DEVICE_MANUFACTURER_NAME" -DSDK_CONFIG_DEVICE_DESCRIPTION="$DEVICE_DESCRIPTION"

# Delete first line from temp file to remove opening bracket
sed -i -e "1d" $TEMP_CONFIG_FILE

# Append temp file to configuration file
cat $TEMP_CONFIG_FILE >> $OUTPUT_CONFIG_FILE

# Enable the suggestedLatency parameter for portAudio
sed -i -e '/displayCardsSupported/s/$/,/' $OUTPUT_CONFIG_FILE
sed -i -e '/portAudio/s/\/\///' $OUTPUT_CONFIG_FILE
# the sed command below will remove the // on two consecutive lines
sed -i -e '/suggestedLatency/{N;s/\/\///g;}' $OUTPUT_CONFIG_FILE

# Delete temp file
rm $TEMP_CONFIG_FILE

echo
echo "==============> FINAL CONFIGURATION AND ALIASES =============="
echo
cat $OUTPUT_CONFIG_FILE

generate_start_script

generate_test_script

if [ ! -f $ALIASES ] ; then
echo "Create .bash_aliases file"
cat << EOF > "$ALIASES"
EOF
fi
echo "Delete any existing avs aliases and rewrite them"
sed -i '/avsrun/d' $ALIASES > /dev/null
sed -i '/avsunit/d' $ALIASES > /dev/null
sed -i '/avssetup/d' $ALIASES > /dev/null
sed -i '/avsauth/d' $ALIASES > /dev/null
sed -i '/AVS/d' $ALIASES > /dev/null
sed -i '/AlexaClientSDKConfig.json/d' $ALIASES > /dev/null
sed -i '/Remove/d' $ALIASES > /dev/null

echo "alias avsrun=\"$STARTUP_SCRIPT\"" >> $ALIASES
echo "echo \"Available AVS aliases: \"" >> $ALIASES
echo "echo \"avsrun \"" >> $ALIASES
echo "echo \"If authentication fails, please check $BUILD_PATH/Integration/AlexaClientSDKConfig.json\"" >> $ALIASES
echo "echo \"Remove .bash_aliases and open a new terminal to remove bindings\"" >> $ALIASES

echo " **** Completed Configuration/Build ***"
