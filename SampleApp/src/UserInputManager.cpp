/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <cctype>

#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/String/StringUtils.h>
#include "SampleApp/UserInputManager.h"
#include "SampleApp/ConsolePrinter.h"

namespace alexaClientSDK {
namespace sampleApp {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::softwareInfo;

static const char HOLD = 'h';
static const char TAP = 't';
static const char QUIT = 'q';
static const char INFO = 'i';
static const char MIC_TOGGLE = 'm';
static const char STOP = 's';
static const char PLAY = '1';
static const char PAUSE = '2';
static const char NEXT = '3';
static const char PREVIOUS = '4';
static const char SKIP_FORWARD = '5';
static const char SKIP_BACKWARD = '6';
static const char SHUFFLE = '7';
static const char LOOP = '8';
static const char REPEAT = '9';
static const char THUMBS_UP = '+';
static const char THUMBS_DOWN = '-';
static const char SETTINGS = 'c';
static const char SPEAKER_CONTROL = 'p';
static const char FIRMWARE_VERSION = 'f';
static const char ESP_CONTROL = 'e';
static const char RESET = 'k';
static const char REAUTHORIZE = 'z';
#ifdef ENABLE_COMMS
static const char COMMS_CONTROL = 'd';
#endif

#ifdef ENABLE_PCC
static const char PHONE_CONTROL = 'a';
#endif
static constexpr char ENABLE = 'E';
static constexpr char DISABLE = 'D';
static const unsigned int VENDOR_ID=0x0572;  
static const unsigned int PRODUCT_ID=0x1494; 

enum class SettingsValues : char { LOCALE = '1', DO_NOT_DISTURB = '2' };

static const std::unordered_map<char, std::string> LOCALE_VALUES({{'1', "en-US"},
                                                                  {'2', "en-GB"},
                                                                  {'3', "de-DE"},
                                                                  {'4', "en-IN"},
                                                                  {'5', "en-CA"},
                                                                  {'6', "ja-JP"},
                                                                  {'7', "en-AU"},
                                                                  {'8', "fr-FR"},
                                                                  {'9', "it-IT"},
                                                                  {'a', "es-ES"},
                                                                  {'b', "es-MX"},
                                                                  {'c', "fr-CA"}});

static const std::unordered_map<char, SpeakerInterface::Type> SPEAKER_TYPES(
    {{'1', SpeakerInterface::Type::AVS_SPEAKER_VOLUME}, {'2', SpeakerInterface::Type::AVS_ALERTS_VOLUME}});

static const int8_t INCREASE_VOLUME = 10;

static const int8_t DECREASE_VOLUME = -10;

/// Time to wait for console input
static const auto READ_CONSOLE_TIMEOUT = std::chrono::milliseconds(100);

/// String to identify log entries originating from this file.
static const std::string TAG("UserInputManager");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<UserInputManager> UserInputManager::create(
    std::shared_ptr<InteractionManager> interactionManager,
    std::shared_ptr<ConsoleReader> consoleReader) {
    if (!interactionManager) {
        ACSDK_CRITICAL(LX("Invalid InteractionManager passed to UserInputManager"));
        return nullptr;
    }

    if (!consoleReader) {
        ACSDK_CRITICAL(LX("Invalid ConsoleReader passed to UserInputManager"));
        return nullptr;
    }

    return std::unique_ptr<UserInputManager>(new UserInputManager(interactionManager, consoleReader));
}

void xfer_complete(struct libusb_transfer *xfer)
{
	switch (xfer->status) {
		case LIBUSB_TRANSFER_COMPLETED:
			//for(int i=0; i<xfer->actual_length; i++)
			//{
					//printf("0x%x ",xfer->buffer[i]);
			//}
			if (xfer->buffer[3] == 0x19) {
				*((char *)xfer->user_data) = TAP;
			}
            //option mute
             //else if ((xfer->buffer[3] == 0x1a)||(xfer->buffer[3] == 0x1b)) {
                //printf("MIC_TOGGLE");
				//*((char *)xfer->user_data) = MIC_TOGGLE;
			//}
			break;
		case LIBUSB_TRANSFER_TIMED_OUT:
			// This just means no buttons were pressed
			// Do nothing
			break;
		default:
			printf("Unknown transfer error!\n");
			break;
	}

	// Re-submit
	assert(libusb_submit_transfer(xfer) == 0);
}

void UserInputManager::init_usb(char *input)
{
    int r = 1; 

    int ret = libusb_init(NULL);

    if (ret < 0) {
        fprintf(stderr, "failed to initialise libusb\n");
        exit(1);
    }

    libusb_device **devs = NULL;

    devh = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);

    if (devh == NULL) {
        fprintf(stderr, "failed to open device. Ensure adequate permissions, error = %d\n", r);
        exit(1);
    }

    libusb_free_device_list(devs, 1);

    if (libusb_kernel_driver_active(devh, 3)) {
        r = libusb_detach_kernel_driver(devh, 3);
    }
    if (libusb_kernel_driver_active(devh, 4)) {
        r = libusb_detach_kernel_driver(devh, 4);      
    }
    if(r < 0){
        fprintf(stderr, "libusb_detach_kernel_driver error %d\n", r); 
    }

    int config; 
    r = libusb_get_configuration(devh, &config);
    if (r < 0) { 
        fprintf(stderr, "libusb_get_configuration error %d\n", r); 
    } 

    r = libusb_claim_interface(devh, 3); 
    if (r < 0) { 
        fprintf(stderr, "libusb_claim_interface 3 error %d\n", r); 
    } 
    r = libusb_claim_interface(devh, 4); 
    if (r < 0) { 
        fprintf(stderr, "libusb_claim_interface 4 error %d\n", r); 
    } 
    
    xfer = libusb_alloc_transfer(0);
    assert(xfer != NULL);
    libusb_fill_interrupt_transfer(xfer, devh, 0x83, recv_packet, 64, &xfer_complete, (void *)input, 20);
    assert(libusb_submit_transfer(xfer) == 0);

}

UserInputManager::UserInputManager(
    std::shared_ptr<InteractionManager> interactionManager,
    std::shared_ptr<ConsoleReader> consoleReader) :
        m_interactionManager{interactionManager},
        m_consoleReader{consoleReader},
        m_limitedInteraction{false},
        m_restart{false} {
}


void UserInputManager::readButtonInput(char &Button_mute_state,char &Button_vl_dn_state,char &Button_vl_up_state,char &Button_action_state) {

#ifdef PI_HAT_CTRL
    int get_button_mute_ret = system("pi_hat_ctrl GET_BUT_MUTE ");
    if (get_button_mute_ret==0) {
        if (Button_mute_state == (char)ButtonState::UNPUSH) {
            Button_mute_state = (char)ButtonState::PUSH ;
            m_interactionManager->microphoneToggle();
        }
        
    } else if (get_button_mute_ret==256) {
        if (Button_mute_state == (char)ButtonState::PUSH){
            Button_mute_state = (char)ButtonState::UNPUSH;
        }
    } 
#endif

#ifdef PI_HAT_CTRL
    //int get_button_vl_dn = system("/home/pi/avs-device-sdk/ThirdParty/pi_hat_ctrl/pi_hat_ctrl GET_BUT_VOL_DN ");
    int get_button_vl_dn = system("pi_hat_ctrl GET_BUT_VOL_DN ");

    if (get_button_vl_dn==0) {
        if (Button_vl_dn_state == (char)ButtonState::UNPUSH) {
            Button_vl_dn_state = (char)ButtonState::PUSH;
            controlSpeakerDecreaseVolumeButton();
        }
        
    }else if (get_button_vl_dn==256) {
        if (Button_vl_dn_state == (char)ButtonState::PUSH){
            Button_vl_dn_state = (char)ButtonState::UNPUSH;
        }  
    } 
#endif

#ifdef PI_HAT_CTRL
    int get_button_vl_up = system("pi_hat_ctrl GET_BUT_VOL_UP ");
    if (get_button_vl_up==0) {
        if (Button_vl_up_state == (char)ButtonState::UNPUSH) {
            Button_vl_up_state = (char)ButtonState::PUSH;
            controlSpeakerIncreaseVolumeButton();
        }
        
    }else if (get_button_vl_up==256) {
        if (Button_vl_up_state == (char)ButtonState::PUSH){
            Button_vl_up_state = (char)ButtonState::UNPUSH;
        }  
    } 
#endif

#ifdef PI_HAT_CTRL
    int get_button_action = system("pi_hat_ctrl GET_BUT_ACTION ");
    if (get_button_action==0) {
        if (Button_action_state == (char)ButtonState::UNPUSH) {
            Button_action_state = (char)ButtonState::PUSH;
            m_interactionManager->tap();
        }
        
    }else if (get_button_action==256) {
        if (Button_action_state == (char)ButtonState::PUSH){
            Button_action_state = (char)ButtonState::UNPUSH;
        }  
    } 
#endif

}


bool UserInputManager::readConsoleInput(char* input) {

    while (input && !m_restart) {

        readButtonInput(Button_mute_state,Button_vl_dn_state,Button_vl_up_state,Button_action_state);

        *input = '\0';
        if(libusb_handle_events_completed(NULL, NULL) != 0)
        {
            printf("you're dead\n");
            break;
        }
        if (*input != '\0') {
            return true;
        }
        
        if (m_consoleReader->read(READ_CONSOLE_TIMEOUT, input)) {
            return true;
        }
    }
    return false;
}

SampleAppReturnCode UserInputManager::run() {
    bool userTriggeredLogout = false;
    m_interactionManager->begin();
    char x;
    init_usb(&x);
    while (true) {
        if (!readConsoleInput(&x)) {
            break;
        }
        x = ::tolower(x);
        if (x == QUIT) {
            break;
        } else if (x == RESET) {
            if (confirmReset()) {
                userTriggeredLogout = true;
            }
        } else if (x == REAUTHORIZE) {
            confirmReauthorizeDevice();
        } else if (x == MIC_TOGGLE) {
            m_interactionManager->microphoneToggle();
        } else if (x == STOP) {
            m_interactionManager->stopForegroundActivity();
        } else if (x == SPEAKER_CONTROL) {
            controlSpeaker();
#ifdef ENABLE_PCC
        } else if (x == PHONE_CONTROL) {
            controlPhone();
#endif
        } else if (x == SETTINGS) {
            settingsMenu();
        } else if (x == INFO) {
            if (m_limitedInteraction) {
                m_interactionManager->limitedHelp();
            } else {
                m_interactionManager->help();
            }
        } else if (m_limitedInteraction) {
            m_interactionManager->errorValue();
            // ----- Add a new interaction bellow if the action is available only in 'unlimited interaction mode'.
        } else if (x == HOLD) {
            m_interactionManager->holdToggled();
        } else if (x == TAP) {
            m_interactionManager->tap();
        } else if (x == PLAY) {
            m_interactionManager->playbackPlay();
        } else if (x == PAUSE) {
            m_interactionManager->playbackPause();
        } else if (x == NEXT) {
            m_interactionManager->playbackNext();
        } else if (x == PREVIOUS) {
            m_interactionManager->playbackPrevious();
        } else if (x == SKIP_FORWARD) {
            m_interactionManager->playbackSkipForward();
        } else if (x == SKIP_BACKWARD) {
            m_interactionManager->playbackSkipBackward();
        } else if (x == SHUFFLE) {
            m_interactionManager->playbackShuffle();
        } else if (x == LOOP) {
            m_interactionManager->playbackLoop();
        } else if (x == REPEAT) {
            m_interactionManager->playbackRepeat();
        } else if (x == THUMBS_UP) {
            m_interactionManager->playbackThumbsUp();
        } else if (x == THUMBS_DOWN) {
            m_interactionManager->playbackThumbsDown();
        } else if (x == FIRMWARE_VERSION) {
            std::string text;
            std::getline(std::cin, text);
            // If text is empty the user entered newline right after the command key.
            // Prompt for the version number and get the version from the next line.
            if (text.empty()) {
                m_interactionManager->firmwareVersionControl();
                std::getline(std::cin, text);
            }
            int version;
            const long long int maxValue = MAX_FIRMWARE_VERSION;
            if (avsCommon::utils::string::stringToInt(text, &version) && version > 0 && version <= maxValue) {
                m_interactionManager->setFirmwareVersion(static_cast<FirmwareVersion>(version));
            } else {
                m_interactionManager->errorValue();
            }
        } else if (x == ESP_CONTROL) {
            m_interactionManager->espControl();
            char espChoice;
            std::string tempString;
            bool continueWhileLoop = true;
            while (continueWhileLoop) {
                std::cin >> espChoice;
                switch (espChoice) {
                    case '1':
                        m_interactionManager->toggleESPSupport();
                        m_interactionManager->espControl();
                        break;
                    case '2':
                        std::cin >> tempString;
                        m_interactionManager->setESPVoiceEnergy(tempString);
                        m_interactionManager->espControl();
                        break;
                    case '3':
                        std::cin >> tempString;
                        m_interactionManager->setESPAmbientEnergy(tempString);
                        m_interactionManager->espControl();
                        break;
                    case 'q':
                        m_interactionManager->help();
                        continueWhileLoop = false;
                        break;
                    default:
                        m_interactionManager->errorValue();
                        m_interactionManager->espControl();
                        break;
                }
            }
#ifdef ENABLE_COMMS
        } else if (x == COMMS_CONTROL) {
            m_interactionManager->commsControl();
            char commsChoice;
            bool continueWhileLoop = true;
            while (continueWhileLoop) {
                std::cin >> commsChoice;
                switch (commsChoice) {
                    case 'a':
                    case 'A':
                        m_interactionManager->acceptCall();
                        break;
                    case 's':
                    case 'S':
                        m_interactionManager->stopCall();
                        break;
                    case 'q':
                        m_interactionManager->help();
                        continueWhileLoop = false;
                        break;
                    default:
                        m_interactionManager->errorValue();
                        continueWhileLoop = false;
                        break;
                }
            }
#endif
        } else {
            m_interactionManager->errorValue();
        }
    }
    if (!userTriggeredLogout && m_restart) {
        return SampleAppReturnCode::RESTART;
    }
    return SampleAppReturnCode::OK;
}

void UserInputManager::onLogout() {
    m_restart = true;
}

void UserInputManager::onAuthStateChange(AuthObserverInterface::State newState, AuthObserverInterface::Error newError) {
    m_limitedInteraction = m_limitedInteraction || (newState == AuthObserverInterface::State::UNRECOVERABLE_ERROR);
}

void UserInputManager::onCapabilitiesStateChange(
    CapabilitiesObserverInterface::State newState,
    CapabilitiesObserverInterface::Error newError) {
    m_limitedInteraction = m_limitedInteraction || (newState == CapabilitiesObserverInterface::State::FATAL_ERROR);
}

void UserInputManager::controlSpeaker() {
    m_interactionManager->speakerControl();
    char speakerChoice;
    std::cin >> speakerChoice;
    if (SPEAKER_TYPES.count(speakerChoice) == 0) {
        m_interactionManager->errorValue();
    } else {
        m_interactionManager->volumeControl();
        SpeakerInterface::Type speaker = SPEAKER_TYPES.at(speakerChoice);
        char volume;
        while (std::cin >> volume && volume != 'q') {
            switch (volume) {
                case '1':
                    m_interactionManager->adjustVolume(speaker, INCREASE_VOLUME);
                    break;
                case '2':
                    m_interactionManager->adjustVolume(speaker, DECREASE_VOLUME);
                    break;
                case '3':
                    m_interactionManager->setMute(speaker, true);
                    break;
                case '4':
                    m_interactionManager->setMute(speaker, false);
                    break;
                case 'i':
                    m_interactionManager->volumeControl();
                    break;
                default:
                    m_interactionManager->errorValue();
                    break;
            }
        }
    }
}


void UserInputManager::controlSpeakerIncreaseVolumeButton() {
    char speakerChoice = '1';
    if (SPEAKER_TYPES.count(speakerChoice) == 0) {
        m_interactionManager->errorValue();
    } else {
        SpeakerInterface::Type speaker = SPEAKER_TYPES.at(speakerChoice);
        m_interactionManager->adjustVolume(speaker, INCREASE_VOLUME);
    }
}

void UserInputManager::controlSpeakerDecreaseVolumeButton() {
    char speakerChoice = '1';
    if (SPEAKER_TYPES.count(speakerChoice) == 0) {
        m_interactionManager->errorValue();
    } else {
        SpeakerInterface::Type speaker = SPEAKER_TYPES.at(speakerChoice);
        m_interactionManager->adjustVolume(speaker, DECREASE_VOLUME);
    }
}


#ifdef ENABLE_PCC
void UserInputManager::controlPhone() {
    std::string callerId;
    m_interactionManager->callId();
    std::string callId;
    std::cin >> callId;
    m_interactionManager->phoneControl();
    char phoneChoice;
    std::cin >> phoneChoice;
    switch (phoneChoice) {
        case '1':
            m_interactionManager->sendCallActivated(callId);
            break;
        case '2':
            m_interactionManager->sendCallTerminated(callId);
            break;
        case '3':
            m_interactionManager->sendCallFailed(callId);
            break;
        case '4':
            m_interactionManager->callerId();
            std::cin >> callerId;
            m_interactionManager->sendCallReceived(callId, callerId);
            break;
        case '5':
            m_interactionManager->callerId();
            std::cin >> callerId;
            m_interactionManager->sendCallerIdReceived(callId, callerId);
            break;
        case '6':
            m_interactionManager->sendInboundRingingStarted(callId);
            break;
        case '7':
            m_interactionManager->sendOutboundCallRequested(callId);
            break;
        case '8':
            m_interactionManager->sendOutboundRingingStarted(callId);
            break;
        case '9':
            m_interactionManager->sendSendDtmfSucceeded(callId);
            break;
        case '0':
            m_interactionManager->sendSendDtmfFailed(callId);
            break;
        case 'q':
            m_interactionManager->help();
            break;
        case 'i':
            m_interactionManager->phoneControl();
            break;
        default:
            m_interactionManager->errorValue();
            break;
    }
}

#endif

bool UserInputManager::confirmReset() {
    m_interactionManager->confirmResetDevice();
    char y;
    do {
        std::cin >> y;
        // Check the Setting which has to be changed.
        switch (y) {
            case 'Y':
            case 'y':
                m_interactionManager->resetDevice();
                return true;
            case 'N':
            case 'n':
                return false;
            default:
                m_interactionManager->errorValue();
                m_interactionManager->confirmResetDevice();
                break;
        }
    } while (true);

    return false;
}

bool UserInputManager::confirmReauthorizeDevice() {
    m_interactionManager->confirmReauthorizeDevice();
    char y;
    do {
        std::cin >> y;
        // Check the Setting which has to be changed.
        switch (y) {
            case 'Y':
            case 'y':
                m_interactionManager->resetDevice();
                return true;
            case 'N':
            case 'n':
                return false;
            default:
                m_interactionManager->errorValue();
                m_interactionManager->confirmReauthorizeDevice();
                break;
        }
    } while (true);

    return false;
}

void boolSettingMenu(std::function<void(bool)> setFunction) {
    char y;
    std::cin >> y;
    switch (y) {
        case ENABLE:
            setFunction(true);
            return;
        case DISABLE:
            setFunction(false);
            return;
    }
}

void UserInputManager::settingsMenu() {
    m_interactionManager->settings();
    char y;
    std::cin >> y;
    // Check the Setting which has to be changed.
    switch (y) {
        case (char)SettingsValues::LOCALE: {
            char localeValue;
            m_interactionManager->locale();
            std::cin >> localeValue;
            auto searchLocale = LOCALE_VALUES.find(localeValue);
            if (searchLocale != LOCALE_VALUES.end()) {
                m_interactionManager->changeSetting("locale", searchLocale->second);
            } else {
                m_interactionManager->errorValue();
            }
            return;
        }
        case (char)SettingsValues::DO_NOT_DISTURB: {
            m_interactionManager->doNotDisturb();
            boolSettingMenu([this](bool enable) { m_interactionManager->setDoNotDisturbMode(enable); });
            return;
        }
    }
    m_interactionManager->help();
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
