/*
 * Copyright (C) 2024 The LineageOS Project
 *               2024 Paranoid Android
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Fingerprint.h"

#include <android-base/logging.h>
#include <unistd.h>

namespace aidl::android::hardware::biometrics::fingerprint {

namespace {
constexpr int32_t kSensorId = 0;
constexpr int32_t kMaxEnrollmentsPerUser = 5;

struct OppoDeathRecipient : public ::android::hardware::hidl_death_recipient {
    void serviceDied(uint64_t /*cookie*/,
                     const ::android::wp<::android::hidl::base::V1_0::IBase>& /*who*/) override {
        // Our callback and active group died with it. The stock framework reconnected once
        // the new instance was up, exit and let init restart us against it.
        LOG(ERROR) << "vendor.oppo.hardware.biometrics.fingerprint@2.1 service died";
        _exit(EXIT_FAILURE);
    }
};

}  // namespace

Fingerprint::Fingerprint()
    : mDevice(IOppoBiometricsFingerprint::getService()),
      mClientCallback(new OppoClientCallback()) {
    CHECK(mDevice != nullptr) << "vendor.oppo.hardware.biometrics.fingerprint@2.1 unavailable";

    static sp<OppoDeathRecipient> sDeathRecipient = new OppoDeathRecipient();
    mDevice->linkToDeath(sDeathRecipient, 0 /* cookie */);

    mDevice->getDebugInfo([this](const auto& info) { mHealth.setPid(info.pid); });

    // A zero device id is how the stock framework detected a failed HAL open.
    HealthMonitor::Scope watch(mHealth, "setNotify");
    Return<uint64_t> id = mDevice->setNotify(mClientCallback);
    CHECK(id.isOk() && id != 0) << "Failed to open Fingerprint HAL";
}

ndk::ScopedAStatus Fingerprint::getSensorProps(std::vector<SensorProps>* out) {
    common::CommonProps commonProps = {
            .sensorId = kSensorId,
            .sensorStrength = common::SensorStrength::STRONG,
            .maxEnrollmentsPerUser = kMaxEnrollmentsPerUser,
    };

    *out = {{.commonProps = commonProps, .sensorType = FingerprintSensorType::REAR}};
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Fingerprint::createSession(int32_t /*sensorId*/, int32_t userId,
                                              const std::shared_ptr<ISessionCallback>& cb,
                                              std::shared_ptr<ISession>* out) {
    CHECK(mSession == nullptr || mSession->isClosed()) << "Open session already exists!";

    mSession = ndk::SharedRefBase::make<Session>(mDevice, mClientCallback, mHealth, userId, cb,
                                                 mLockoutTracker);
    // Route callbacks before setActiveGroup(), it replies with onSyncTemplates.
    mClientCallback->setSession(mSession);
    mSession->init();
    mSession->linkToDeath(cb->asBinder().get());

    *out = mSession;
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::biometrics::fingerprint
