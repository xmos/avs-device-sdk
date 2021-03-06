/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKPLAYBACKROUTER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKPLAYBACKROUTER_H_

#include <gmock/gmock.h>

#include <AVSCommon/SDKInterfaces/PlaybackRouterInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace test {

/**
 * Mock class implementing @c PlaybackRouterInterface
 */
class MockPlaybackRouter : public PlaybackRouterInterface {
public:
    MOCK_METHOD1(buttonPressed, void(avsCommon::avs::PlaybackButton button));
    MOCK_METHOD2(togglePressed, void(avsCommon::avs::PlaybackToggle toggle, bool action));
    MOCK_METHOD1(setHandler, void(std::shared_ptr<avsCommon::sdkInterfaces::PlaybackHandlerInterface> handler));
    MOCK_METHOD0(switchToDefaultHandler, void());
    MOCK_METHOD2(
        setHandler,
        void(
            std::shared_ptr<avsCommon::sdkInterfaces::PlaybackHandlerInterface> handler,
            std::shared_ptr<avsCommon::sdkInterfaces::LocalPlaybackHandlerInterface> localHandler));
    MOCK_METHOD1(
        useDefaultHandlerWith,
        void(std::shared_ptr<avsCommon::sdkInterfaces::LocalPlaybackHandlerInterface> localHandler));
    MOCK_METHOD1(localOperation, bool(avsCommon::sdkInterfaces::LocalPlaybackHandlerInterface::PlaybackOperation op));
    MOCK_METHOD2(localSeekTo, bool(std::chrono::milliseconds location, bool fromStart));
};

}  // namespace test
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKPLAYBACKROUTER_H_
