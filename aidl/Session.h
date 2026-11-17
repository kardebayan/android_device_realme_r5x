/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <aidl/android/hardware/biometrics/common/BnCancellationSignal.h>
#include <aidl/android/hardware/biometrics/fingerprint/BnSession.h>
#include <aidl/android/hardware/biometrics/fingerprint/ISessionCallback.h>
#include <log/log.h>

#include <atomic>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include "HealthMonitor.h"
#include "LockoutTracker.h"
#include "OppoFingerprint.h"

namespace aidl::android::hardware::biometrics::fingerprint {

using ::aidl::android::hardware::biometrics::common::BnCancellationSignal;
using ::aidl::android::hardware::biometrics::common::ICancellationSignal;
using ::aidl::android::hardware::biometrics::common::OperationContext;
using ::aidl::android::hardware::keymaster::HardwareAuthToken;

class Session : public BnSession {
  public:
    Session(sp<IOppoBiometricsFingerprint> device, sp<OppoClientCallback> clientCallback,
            HealthMonitor& health, int32_t userId, std::shared_ptr<ISessionCallback> cb,
            LockoutTracker& lockoutTracker);

    // Selects the user's template group, callbacks must already be routed to this session.
    void init();
    bool isClosed();
    binder_status_t linkToDeath(AIBinder* binder);
    ndk::ScopedAStatus cancel();

    // ISession
    ndk::ScopedAStatus generateChallenge() override;
    ndk::ScopedAStatus revokeChallenge(int64_t challenge) override;
    ndk::ScopedAStatus enroll(const HardwareAuthToken& hat,
                              std::shared_ptr<ICancellationSignal>* out) override;
    ndk::ScopedAStatus authenticate(int64_t operationId,
                                    std::shared_ptr<ICancellationSignal>* out) override;
    ndk::ScopedAStatus detectInteraction(std::shared_ptr<ICancellationSignal>* out) override;
    ndk::ScopedAStatus enumerateEnrollments() override;
    ndk::ScopedAStatus removeEnrollments(const std::vector<int32_t>& enrollmentIds) override;
    ndk::ScopedAStatus getAuthenticatorId() override;
    ndk::ScopedAStatus invalidateAuthenticatorId() override;
    ndk::ScopedAStatus resetLockout(const HardwareAuthToken& hat) override;
    ndk::ScopedAStatus close() override;
    ndk::ScopedAStatus authenticateWithContext(int64_t operationId, const OperationContext& context,
                                               std::shared_ptr<ICancellationSignal>* out) override;
    ndk::ScopedAStatus enrollWithContext(const HardwareAuthToken& hat,
                                         const OperationContext& context,
                                         std::shared_ptr<ICancellationSignal>* out) override;
    ndk::ScopedAStatus detectInteractionWithContext(
            const OperationContext& context, std::shared_ptr<ICancellationSignal>* out) override;
    ndk::ScopedAStatus onContextChanged(const OperationContext& context) override;

    // Under-display sensor only, r5x has a rear sensor.
    ndk::ScopedAStatus onPointerDown(int32_t, int32_t, int32_t, float, float) override {
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus onPointerUp(int32_t) override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus onUiReady() override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus onPointerDownWithContext(const PointerContext&) override {
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus onPointerUpWithContext(const PointerContext&) override {
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus onPointerCancelWithContext(const PointerContext&) override {
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus setIgnoreDisplayTouches(bool) override { return ndk::ScopedAStatus::ok(); }

    // Callbacks from the stock vendor.oppo service
    void onEnrollResult(uint32_t fingerId, uint32_t remaining);
    void onAcquired(OppoAcquiredInfo acquiredInfo, int32_t vendorCode);
    void onAuthenticated(uint32_t fingerId, const hidl_vec<uint8_t>& token);
    void onError(OppoError error, int32_t vendorCode);
    void onRemoved(uint32_t fingerId);
    void onSyncTemplates(const hidl_vec<uint32_t>& fingerIds);

  private:
    enum class State { IDLE, ENROLLING, AUTHENTICATING };

    void setState(State state);
    void failOperation();
    bool checkSensorLockout();
    void clearLockout(bool clearAttemptCounter);
    void startLockoutTimer(int64_t timeout);
    void setScreenState(const OperationContext& context);
    void saveTemplatesLocked();
    void connect();
    void reconnectIfNeeded();

    sp<IOppoBiometricsFingerprint> mDevice;
    sp<OppoClientCallback> mClientCallback;
    HealthMonitor& mHealth;
    std::string mPath;
    std::atomic<bool> mReconnect = false;
    OppoAuthType mAuthType = OppoAuthType::TYPE_OTHER;
    const int32_t mUserId;
    std::shared_ptr<ISessionCallback> mCb;
    LockoutTracker& mLockoutTracker;
    AIBinder_DeathRecipient* mDeathRecipient;
    bool mClosed = false;

    std::mutex mLockoutTimerMutex;
    bool mIsLockoutTimerStarted = false;
    bool mIsLockoutTimerAborted = false;

    // Used to drop the stock service's late ERROR_CANCELED after we already reported one.
    std::mutex mStateMutex;
    State mState = State::IDLE;

    // Template list from onSyncTemplates (the stock service never sends onEnumerate),
    // persisted in mTemplatesFile like the stock framework persisted its list.
    std::string mTemplatesFile;
    std::mutex mTemplatesMutex;
    std::set<uint32_t> mTemplates;

    // Pending removeEnrollments()
    std::mutex mRemoveMutex;
    std::set<int32_t> mPendingRemovals;
    std::vector<int32_t> mRemoved;
};

class CancellationSignal : public BnCancellationSignal {
  public:
    explicit CancellationSignal(std::shared_ptr<Session> session) : mSession(std::move(session)) {}
    ndk::ScopedAStatus cancel() override { return mSession->cancel(); }

  private:
    std::shared_ptr<Session> mSession;
};

}  // namespace aidl::android::hardware::biometrics::fingerprint
