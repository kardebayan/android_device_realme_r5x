/*
 * Copyright (C) 2022 The Android Open Source Project
 *               2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>

namespace aidl::android::hardware::biometrics::fingerprint {

class LockoutTracker {
  public:
    enum class LockoutMode : int8_t { kNone = 0, kTimed, kPermanent };

    void reset(bool clearAttemptCounter);
    LockoutMode getMode();
    void addFailedAttempt();
    int64_t getLockoutTimeLeft();

  private:
    using Clock = std::chrono::steady_clock;

    LockoutMode getModeLocked();

    std::mutex mMutex;
    int32_t mFailedCount = 0;
    bool mTimedLockoutCleared = true;
    Clock::time_point mLockoutTimedStart;
};

}  // namespace aidl::android::hardware::biometrics::fingerprint
