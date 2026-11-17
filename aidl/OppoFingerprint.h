/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <mutex>

#include <vendor/oppo/hardware/biometrics/fingerprint/2.1/IBiometricsFingerprint.h>
#include <vendor/oppo/hardware/biometrics/fingerprint/2.1/IBiometricsFingerprintClientCallback.h>

namespace aidl::android::hardware::biometrics::fingerprint {

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

namespace oppo_fp = ::vendor::oppo::hardware::biometrics::fingerprint::V2_1;
using IOppoBiometricsFingerprint = oppo_fp::IBiometricsFingerprint;
using OppoAcquiredInfo = oppo_fp::FingerprintAcquiredInfo;
using OppoAuthType = oppo_fp::FingerprintAuthType;
using OppoError = oppo_fp::FingerprintError;
using OppoRequestStatus = oppo_fp::RequestStatus;
using OppoScreenState = oppo_fp::FingerprintScreenState;

class Session;

// The stock vendor.oppo service takes a single callback via setNotify(), the AIDL framework
// opens a new session per user, so this is registered once and pointed at the current session.
class OppoClientCallback : public oppo_fp::IBiometricsFingerprintClientCallback {
  public:
    void setSession(std::shared_ptr<Session> session);

    Return<void> onEnrollResult(uint64_t deviceId, uint32_t fingerId, uint32_t groupId,
                                uint32_t remaining) override;
    Return<void> onAcquired(uint64_t deviceId, OppoAcquiredInfo acquiredInfo,
                            int32_t vendorCode) override;
    Return<void> onAuthenticated(uint64_t deviceId, uint32_t fingerId, uint32_t groupId,
                                 const hidl_vec<uint8_t>& token) override;
    Return<void> onError(uint64_t deviceId, OppoError error, int32_t vendorCode) override;
    Return<void> onRemoved(uint64_t deviceId, uint32_t fingerId, uint32_t groupId,
                           uint32_t remaining) override;
    Return<void> onSyncTemplates(uint64_t deviceId, const hidl_vec<uint32_t>& fingerId,
                                 uint32_t remaining) override;

    // Never sent by the stock service, or not relevant for a rear sensor.
    Return<void> onEnumerate(uint64_t, uint32_t, uint32_t, uint32_t) override { return Void(); }
    Return<void> onTouchDown(uint64_t) override { return Void(); }
    Return<void> onTouchUp(uint64_t) override { return Void(); }
    Return<void> onMonitorEventTriggered(uint32_t, const hidl_string&) override { return Void(); }
    Return<void> onImageInfoAcquired(uint32_t, uint32_t, uint32_t) override { return Void(); }
    Return<void> onEngineeringInfoUpdated(uint32_t, const hidl_vec<uint32_t>&,
                                          const hidl_vec<hidl_string>&) override {
        return Void();
    }
    Return<void> onFingerprintCmd(int32_t, const hidl_vec<int8_t>&, uint32_t) override {
        return Void();
    }

  private:
    std::shared_ptr<Session> getSession();

    std::mutex mMutex;
    std::shared_ptr<Session> mSession;
};

}  // namespace aidl::android::hardware::biometrics::fingerprint
