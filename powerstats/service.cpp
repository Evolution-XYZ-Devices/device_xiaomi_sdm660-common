/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "android.hardware.power.stats@1.0-service.xiaomi_sdm660"
#include <android-base/properties.h>
#include <android/log.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <hidl/HidlTransportSupport.h>
#include <pixelpowerstats/AidlStateResidencyDataProvider.h>
#include <pixelpowerstats/GenericStateResidencyDataProvider.h>
#include <pixelpowerstats/PowerStats.h>
#include "RailDataProvider.h"
using android::OK;
using android::sp;
using android::status_t;
// libhwbinder:
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
// Generated HIDL files
using android::hardware::power::stats::V1_0::IPowerStats;
using android::hardware::power::stats::V1_0::PowerEntityInfo;
using android::hardware::power::stats::V1_0::PowerEntityStateSpace;
using android::hardware::power::stats::V1_0::PowerEntityType;
using android::hardware::power::stats::V1_0::implementation::PowerStats;
// Pixel specific
using android::hardware::google::pixel::powerstats::AidlStateResidencyDataProvider;
using android::hardware::google::pixel::powerstats::GenericStateResidencyDataProvider;
using android::hardware::google::pixel::powerstats::RailDataProvider;
int main(int /* argc */, char ** /* argv */) {
    ALOGI("power.stats service 1.0 is starting.");
    PowerStats *service = new PowerStats();
    // Add rail data provider
    service->setRailDataProvider(std::make_unique<RailDataProvider>());
    // Add Power Entities that require the Aidl data provider
    sp<AidlStateResidencyDataProvider> aidlSdp = new AidlStateResidencyDataProvider();
    uint32_t citadelId = service->addPowerEntity("Citadel", PowerEntityType::SUBSYSTEM);
    aidlSdp->addEntity(citadelId, "Citadel", {"Last-Reset", "Active", "Deep-Sleep"});
    auto serviceStatus = android::defaultServiceManager()->addService(
        android::String16("power.stats-vendor"), aidlSdp);
    if (serviceStatus != android::OK) {
        ALOGE("Unable to register power.stats-vendor service %d", serviceStatus);
        return 1;
    }
    sp<android::ProcessState> ps{android::ProcessState::self()};  // Create non-HW binder threadpool
    ps->startThreadPool();
    service->addStateResidencyDataProvider(aidlSdp);
    // Configure the threadpool
    configureRpcThreadpool(1, true /*callerWillJoin*/);
    status_t status = service->registerAsService();
    if (status != OK) {
        ALOGE("Could not register service for power.stats HAL Iface (%d), exiting.", status);
        return 1;
    }
    ALOGI("power.stats service is ready");
    joinRpcThreadpool();
    // In normal operation, we don't expect the thread pool to exit
    ALOGE("power.stats service is shutting down");
    return 1;
}
