/*
 * Copyright (C) 2024 The LineageOS Project
 *               2024 Paranoid Android
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <aidl/android/hardware/biometrics/fingerprint/BnFingerprint.h>

#include "HealthMonitor.h"
#include "LockoutTracker.h"
#include "OppoFingerprint.h"
#include "Session.h"

namespace aidl::android::hardware::biometrics::fingerprint {

class Fingerprint : public BnFingerprint {
  public:
    Fingerprint();

    ndk::ScopedAStatus getSensorProps(std::vector<SensorProps>* out) override;
    ndk::ScopedAStatus createSession(int32_t sensorId, int32_t userId,
                                     const std::shared_ptr<ISessionCallback>& cb,
                                     std::shared_ptr<ISession>* out) override;

  private:
    sp<IOppoBiometricsFingerprint> mDevice;
    sp<OppoClientCallback> mClientCallback;
    std::shared_ptr<Session> mSession;
    LockoutTracker mLockoutTracker;
    HealthMonitor mHealth;
};

}  // namespace aidl::android::hardware::biometrics::fingerprint
