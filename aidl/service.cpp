/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Fingerprint.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <hidl/HidlTransportSupport.h>

#include <thread>

using ::aidl::android::hardware::biometrics::fingerprint::Fingerprint;

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    // Callbacks from the stock vendor.oppo service arrive over hwbinder, nothing starts that
    // threadpool for a pure HIDL client, so serve it from a dedicated thread.
    ::android::hardware::configureRpcThreadpool(1, true /* callerWillJoin */);
    std::thread([] { ::android::hardware::joinRpcThreadpool(); }).detach();

    std::shared_ptr<Fingerprint> fingerprint = ndk::SharedRefBase::make<Fingerprint>();

    const std::string instance = std::string() + Fingerprint::descriptor + "/default";
    binder_status_t status =
            AServiceManager_addService(fingerprint->asBinder().get(), instance.c_str());
    CHECK(status == STATUS_OK);

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
