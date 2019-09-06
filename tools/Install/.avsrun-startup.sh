#!/bin/bash
CONFIG_JSON_FILE="/home/pi/sdk-folder/sdk-build/Integration/AlexaClientSDKConfig.json"
if [ -f $CONFIG_JSON_FILE ]; then
    /home/pi/sdk-folder/sdk-build/SampleApp/src/SampleApp $CONFIG_JSON_FILE /home/pi/sdk-folder/third-party/alexa-rpi/models
else
    echo "AVS setup is not complete, follow the steps below:"
    echo ""
    echo ""
    echo "1. Register Alexa with AVS and save a *config.json* file by following https://github.com/alexa/avs-device-sdk/wiki/Create-Security-Profile."
    echo ""
    echo ""
    echo "2. Copy the *config.json* into the directory \`vocalfusion_3510_avs_setup\`"
    echo ""
    echo ""
    echo "3. Go to the directory \`vocalfusion_3510_avs_setup\` and type:"
    echo "    \`avssetup config.json\`"
    echo ""
    echo ""
    echo "4. Read and accept the Sensory and the AVS Device SDK license agreements."
    echo ""
    echo ""
    echo "5. Complete the setup by following the steps from 8 onward here: https://github.com/xmos/vocalfusion_3510_avs_setup"
    echo ""
    echo ""
fi
$SHELL
