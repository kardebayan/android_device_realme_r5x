/*
 * Copyright (C) 2022 The Android Open Source Project
 *               2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "LockoutTracker.h"

#include <algorithm>

namespace aidl::android::hardware::biometrics::fingerprint {

// Same policy as the stock FingerprintService (getLockoutMode/handleFailedAttempt):
// timed lockout on every 5th failure, cleared by a 30s alarm without resetting the
// counter, permanent lockout at 20 failures.
namespace {
constexpr int32_t kLockoutTimedThreshold = 5;
constexpr int32_t kLockoutPermanentThreshold = 20;
constexpr auto kLockoutTimedDuration = std::chrono::milliseconds(30000);
}  // namespace

void LockoutTracker::reset(bool clearAttemptCounter) {
    std::lock_guard<std::mutex> lock(mMutex);
    if (clearAttemptCounter) {
        mFailedCount = 0;
    }
    mTimedLockoutCleared = true;
}

void LockoutTracker::addFailedAttempt() {
    std::lock_guard<std::mutex> lock(mMutex);
    mFailedCount++;
    mTimedLockoutCleared = false;
    if (mFailedCount < kLockoutPermanentThreshold && mFailedCount % kLockoutTimedThreshold == 0) {
        mLockoutTimedStart = Clock::now();
    }
}

LockoutTracker::LockoutMode LockoutTracker::getMode() {
    std::lock_guard<std::mutex> lock(mMutex);
    return getModeLocked();
}

LockoutTracker::LockoutMode LockoutTracker::getModeLocked() {
    if (mFailedCount >= kLockoutPermanentThreshold) {
        return LockoutMode::kPermanent;
    }
    if (mFailedCount > 0 && !mTimedLockoutCleared &&
        mFailedCount % kLockoutTimedThreshold == 0) {
        if (Clock::now() - mLockoutTimedStart < kLockoutTimedDuration) {
            return LockoutMode::kTimed;
        }
        mTimedLockoutCleared = true;
    }
    return LockoutMode::kNone;
}

int64_t LockoutTracker::getLockoutTimeLeft() {
    std::lock_guard<std::mutex> lock(mMutex);
    if (getModeLocked() != LockoutMode::kTimed) return 0;

    auto left = kLockoutTimedDuration - std::chrono::duration_cast<std::chrono::milliseconds>(
                                                Clock::now() - mLockoutTimedStart);
    return std::max<int64_t>(left.count(), 0);
}

}  // namespace aidl::android::hardware::biometrics::fingerprint
