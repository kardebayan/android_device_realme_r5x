/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Session.h"

#include <aidl/android/hardware/biometrics/common/OperationReason.h>

#include <endian.h>
#include <hardware/hw_auth_token.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>

namespace aidl::android::hardware::biometrics::fingerprint {

namespace {
// Same path the stock framework passed to setActiveGroup(), vold prepares it.
constexpr char kFpDataPath[] = "/data/vendor_de/%d/fpdata";
// Timeout used for enroll() by the stock framework.
constexpr uint32_t kEnrollTimeoutSec = 60;
// The stock framework never called enumerate(), it kept the list onSyncTemplates sent
// (setActiveGroup, enroll, remove). FPC doesn't report it on a plain setActiveGroup, so
// it is persisted here like the stock framework persisted its own list.
constexpr char kTemplatesFile[] = "r5x_templates";
// fpc do_enroll() reports FPC_ENROL_FAILED_ALREADY_ENROLED as remaining = 9999.
constexpr uint32_t kFpcRemainingAlreadyEnrolled = 9999;

static_assert(sizeof(hw_auth_token_t) == 69, "stock HAL expects a 69 byte hw_auth_token_t");

hw_auth_token_t toHwAuthToken(const HardwareAuthToken& authToken) {
    hw_auth_token_t hat = {};
    hat.challenge = authToken.challenge;
    hat.user_id = authToken.userId;
    hat.authenticator_id = authToken.authenticatorId;
    // these are in host order: translate to network order
    hat.authenticator_type = htobe32(static_cast<uint32_t>(authToken.authenticatorType));
    hat.timestamp = htobe64(authToken.timestamp.milliSeconds);
    std::copy_n(authToken.mac.begin(), std::min(authToken.mac.size(), sizeof(hat.hmac)),
                hat.hmac);
    return hat;
}

HardwareAuthToken toHardwareAuthToken(const hw_auth_token_t& hat) {
    HardwareAuthToken authToken;
    authToken.challenge = hat.challenge;
    authToken.userId = hat.user_id;
    authToken.authenticatorId = hat.authenticator_id;
    // these are in network order: translate to host
    authToken.authenticatorType =
            static_cast<keymaster::HardwareAuthenticatorType>(be32toh(hat.authenticator_type));
    authToken.timestamp.milliSeconds = be64toh(hat.timestamp);
    authToken.mac.assign(std::begin(hat.hmac), std::end(hat.hmac));
    return authToken;
}

bool isOk(const Return<OppoRequestStatus>& ret) {
    return ret.isOk() && static_cast<OppoRequestStatus>(ret) == OppoRequestStatus::SYS_OK;
}

void onClientDeath(void* cookie) {
    ALOGI("FingerprintService has died");
    Session* session = static_cast<Session*>(cookie);
    if (session && !session->isClosed()) {
        session->close();
    }
}
}  // namespace

Session::Session(sp<IOppoBiometricsFingerprint> device, sp<OppoClientCallback> clientCallback,
                 HealthMonitor& health, int32_t userId, std::shared_ptr<ISessionCallback> cb,
                 LockoutTracker& lockoutTracker)
    : mDevice(std::move(device)),
      mClientCallback(std::move(clientCallback)),
      mHealth(health),
      mUserId(userId),
      mCb(std::move(cb)),
      mLockoutTracker(lockoutTracker),
      mDeathRecipient(AIBinder_DeathRecipient_new(onClientDeath)) {}

void Session::init() {
    // The stock service rejects setActiveGroup() unless access(path, W_OK) succeeds.
    char path[64];
    snprintf(path, sizeof(path), kFpDataPath, mUserId);
    if (access(path, F_OK) != 0 && mkdir(path, 0700) != 0) {
        ALOGE("Failed to create %s: %s", path, strerror(errno));
    }

    mTemplatesFile = std::string(path) + "/" + kTemplatesFile;
    {
        std::lock_guard<std::mutex> lock(mTemplatesMutex);
        std::ifstream in(mTemplatesFile);
        for (uint32_t fid; in >> fid;) {
            if (fid != 0) mTemplates.insert(fid);
        }
    }

    mPath = path;
    connect();
}

// Same sequence as the stock FingerprintService::getFingerprintDaemon(), which it ran
// again after ERROR_HW_UNAVAILABLE.
void Session::connect() {
    {
        HealthMonitor::Scope watch(mHealth, "setActiveGroup");
        if (!isOk(mDevice->setActiveGroup(mUserId, mPath))) {
            ALOGE("setActiveGroup(%d, %s) failed", mUserId, mPath.c_str());
        }
    }
    HealthMonitor::Scope watch(mHealth, "getEnrollmentTotalTimes");
    mDevice->getEnrollmentTotalTimes();
}

void Session::reconnectIfNeeded() {
    if (!mReconnect.exchange(false)) return;
    ALOGW("Reconnecting after ERROR_HW_UNAVAILABLE");
    {
        HealthMonitor::Scope watch(mHealth, "setNotify");
        Return<uint64_t> id = mDevice->setNotify(mClientCallback);
        if (!id.isOk() || id == 0) {
            ALOGE("Failed to open Fingerprint HAL");
            mReconnect = true;
            return;
        }
    }
    connect();
}

bool Session::isClosed() {
    return mClosed;
}

binder_status_t Session::linkToDeath(AIBinder* binder) {
    return AIBinder_linkToDeath(binder, mDeathRecipient, this);
}

void Session::setState(State state) {
    std::lock_guard<std::mutex> lock(mStateMutex);
    mState = state;
}

// Report a failed start of an operation unless the stock service already ended it.
void Session::failOperation() {
    std::lock_guard<std::mutex> lock(mStateMutex);
    if (mState == State::IDLE) return;
    mState = State::IDLE;
    mCb->onError(Error::UNABLE_TO_PROCESS, 0 /* vendorCode */);
}

ndk::ScopedAStatus Session::generateChallenge() {
    reconnectIfNeeded();
    uint64_t challenge;
    {
        HealthMonitor::Scope watch(mHealth, "preEnroll");
        challenge = mDevice->preEnroll();
    }
    mCb->onChallengeGenerated(challenge);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::revokeChallenge(int64_t challenge) {
    {
        HealthMonitor::Scope watch(mHealth, "postEnroll");
        mDevice->postEnroll();
    }
    mCb->onChallengeRevoked(challenge);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::enroll(const HardwareAuthToken& hat,
                                   std::shared_ptr<ICancellationSignal>* out) {
    hw_auth_token_t authToken = toHwAuthToken(hat);
    ::android::hardware::hidl_array<uint8_t, sizeof(hw_auth_token_t)> hidlHat;
    memcpy(hidlHat.data(), &authToken, sizeof(authToken));

    reconnectIfNeeded();
    setState(State::ENROLLING);
    Return<OppoRequestStatus> ret = [&] {
        HealthMonitor::Scope watch(mHealth, "enroll");
        return mDevice->enroll(hidlHat, mUserId, kEnrollTimeoutSec);
    }();
    if (!isOk(ret)) {
        ALOGE("enroll failed");
        failOperation();
    }

    *out = ndk::SharedRefBase::make<CancellationSignal>(ref<Session>());
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::authenticate(int64_t operationId,
                                         std::shared_ptr<ICancellationSignal>* out) {
    *out = ndk::SharedRefBase::make<CancellationSignal>(ref<Session>());
    if (checkSensorLockout()) {
        return ndk::ScopedAStatus::ok();
    }

    // egis_hidl::authenticate() is an empty stub in the stock service, only
    // authenticateAsType() reaches the sensor (on FPC as well).
    reconnectIfNeeded();
    setState(State::AUTHENTICATING);
    Return<OppoRequestStatus> ret = [&] {
        HealthMonitor::Scope watch(mHealth, "authenticate");
        return mDevice->authenticateAsType(operationId, mUserId, mAuthType);
    }();
    if (!isOk(ret)) {
        ALOGE("authenticate failed");
        failOperation();
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::detectInteraction(std::shared_ptr<ICancellationSignal>* out) {
    // supportsDetectInteraction is false, the framework doesn't call this.
    mCb->onError(Error::UNABLE_TO_PROCESS, 0 /* vendorCode */);
    *out = ndk::SharedRefBase::make<CancellationSignal>(ref<Session>());
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::enumerateEnrollments() {
    std::vector<int32_t> enrollments;
    {
        std::lock_guard<std::mutex> lock(mTemplatesMutex);
        enrollments.assign(mTemplates.begin(), mTemplates.end());
    }
    mCb->onEnrollmentsEnumerated(enrollments);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::removeEnrollments(const std::vector<int32_t>& enrollmentIds) {
    if (enrollmentIds.empty()) {
        mCb->onEnrollmentsRemoved({});
        return ndk::ScopedAStatus::ok();
    }

    {
        std::lock_guard<std::mutex> lock(mRemoveMutex);
        mPendingRemovals = std::set<int32_t>(enrollmentIds.begin(), enrollmentIds.end());
        mRemoved.clear();
    }

    reconnectIfNeeded();
    bool failed = false;
    for (int32_t fid : enrollmentIds) {
        // fid 0 removes every template of the group on Egis, never pass it through.
        if (fid != 0) {
            HealthMonitor::Scope watch(mHealth, "remove");
            if (isOk(mDevice->remove(mUserId, fid))) continue;
        }

        ALOGE("remove(%d) failed", fid);
        failed = true;
        std::lock_guard<std::mutex> lock(mRemoveMutex);
        mPendingRemovals.erase(fid);
    }

    if (failed) {
        std::lock_guard<std::mutex> lock(mRemoveMutex);
        if (mPendingRemovals.empty()) {
            if (!mRemoved.empty()) {
                mCb->onEnrollmentsRemoved(mRemoved);
                mRemoved.clear();
            }
            mCb->onError(Error::UNABLE_TO_REMOVE, 0 /* vendorCode */);
        }
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::getAuthenticatorId() {
    reconnectIfNeeded();
    uint64_t id;
    {
        HealthMonitor::Scope watch(mHealth, "getAuthenticatorId");
        id = mDevice->getAuthenticatorId();
    }
    mCb->onAuthenticatorIdRetrieved(id);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::invalidateAuthenticatorId() {
    // The stock service has no way to rotate it.
    uint64_t id;
    {
        HealthMonitor::Scope watch(mHealth, "getAuthenticatorId");
        id = mDevice->getAuthenticatorId();
    }
    mCb->onAuthenticatorIdInvalidated(id);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::resetLockout(const HardwareAuthToken& /*hat*/) {
    clearLockout(true);
    std::lock_guard<std::mutex> lock(mLockoutTimerMutex);
    if (mIsLockoutTimerStarted) mIsLockoutTimerAborted = true;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::close() {
    mClosed = true;
    mCb->onSessionClosed();
    AIBinder_DeathRecipient_delete(mDeathRecipient);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::authenticateWithContext(int64_t operationId,
                                                    const OperationContext& context,
                                                    std::shared_ptr<ICancellationSignal>* out) {
    setScreenState(context);
    // Stock FingerprintAuthClient: TYPE_KEYGUARD for keyguard, TYPE_OTHER otherwise
    // (TYPE_PAY is for OPPO's fingerprint pay, which doesn't exist here).
    mAuthType = context.reason == common::OperationReason::KEYGUARD
                        ? OppoAuthType::TYPE_KEYGUARD
                        : OppoAuthType::TYPE_OTHER;
    return authenticate(operationId, out);
}

ndk::ScopedAStatus Session::enrollWithContext(const HardwareAuthToken& hat,
                                              const OperationContext& context,
                                              std::shared_ptr<ICancellationSignal>* out) {
    setScreenState(context);
    return enroll(hat, out);
}

ndk::ScopedAStatus Session::detectInteractionWithContext(
        const OperationContext& /*context*/, std::shared_ptr<ICancellationSignal>* out) {
    return detectInteraction(out);
}

ndk::ScopedAStatus Session::onContextChanged(const OperationContext& context) {
    setScreenState(context);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::cancel() {
    Return<OppoRequestStatus> ret = [&] {
        HealthMonitor::Scope watch(mHealth, "cancel");
        return mDevice->cancel();
    }();
    if (!isOk(ret)) {
        return ndk::ScopedAStatus::fromServiceSpecificError(
                ret.isOk() ? static_cast<int32_t>(static_cast<OppoRequestStatus>(ret))
                           : static_cast<int32_t>(OppoRequestStatus::SYS_UNKNOWN));
    }

    // Neither fingerprint_cancel() (Egis) nor fpc_cancel() (FPC) report ERROR_CANCELED.
    setState(State::IDLE);
    mCb->onError(Error::CANCELED, 0 /* vendorCode */);
    return ndk::ScopedAStatus::ok();
}

// Egis only counts a bad quality image while verifying as a failed attempt with the screen
// on, with the screen off (finger in pocket) it's dropped silently.
void Session::setScreenState(const OperationContext& context) {
    bool screenOff = context.isAod || context.displayState == common::DisplayState::NO_UI ||
                     context.displayState == common::DisplayState::AOD;
    HealthMonitor::Scope watch(mHealth, "setScreenState");
    mDevice->setScreenState(screenOff ? OppoScreenState::FINGERPRINT_SCREEN_OFF
                                      : OppoScreenState::FINGERPRINT_SCREEN_ON);
}

bool Session::checkSensorLockout() {
    switch (mLockoutTracker.getMode()) {
        case LockoutTracker::LockoutMode::kPermanent: {
            ALOGE("Fail: lockout permanent");
            mCb->onLockoutPermanent();
            std::lock_guard<std::mutex> lock(mLockoutTimerMutex);
            mIsLockoutTimerAborted = true;
            return true;
        }
        case LockoutTracker::LockoutMode::kTimed: {
            int64_t timeLeft = mLockoutTracker.getLockoutTimeLeft();
            ALOGE("Fail: lockout timed: %" PRId64, timeLeft);
            mCb->onLockoutTimed(timeLeft);
            startLockoutTimer(timeLeft);
            return true;
        }
        default:
            return false;
    }
}

void Session::clearLockout(bool clearAttemptCounter) {
    mLockoutTracker.reset(clearAttemptCounter);
    mCb->onLockoutCleared();
}

void Session::startLockoutTimer(int64_t timeout) {
    {
        std::lock_guard<std::mutex> lock(mLockoutTimerMutex);
        if (mIsLockoutTimerStarted) return;
        mIsLockoutTimerStarted = true;
    }

    auto self = ref<Session>();
    std::thread([self, timeout]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
        bool aborted;
        {
            std::lock_guard<std::mutex> lock(self->mLockoutTimerMutex);
            aborted = self->mIsLockoutTimerAborted;
            self->mIsLockoutTimerStarted = false;
            self->mIsLockoutTimerAborted = false;
        }
        if (!aborted) self->clearLockout(false);
    }).detach();
}

void Session::onEnrollResult(uint32_t fingerId, uint32_t remaining) {
    // Already reported as ACQUIRED_ALREADY_ENROLLED, the enrollment continues.
    if (remaining == kFpcRemainingAlreadyEnrolled) return;

    if (remaining == 0) {
        setState(State::IDLE);
        std::lock_guard<std::mutex> lock(mTemplatesMutex);
        mTemplates.insert(fingerId);
        saveTemplatesLocked();
    }
    mCb->onEnrollmentProgress(fingerId, remaining);
}

// Stock passes raw codes with vendorCode 0, including ACQUIRED_TOO_SIMILAR (1001) and
// ACQUIRED_ALREADY_ENROLLED (1002) -> VENDOR 1 and 2, see fingerprint_acquired_vendor.
void Session::onAcquired(OppoAcquiredInfo acquiredInfo, int32_t vendorCode) {
    const int32_t info = static_cast<int32_t>(acquiredInfo);
    const int32_t vendorBase = static_cast<int32_t>(OppoAcquiredInfo::ACQUIRED_VENDOR_BASE);
    AcquiredInfo result;

    if (info >= vendorBase) {
        result = AcquiredInfo::VENDOR;
        vendorCode = info - vendorBase;
    } else {
        switch (acquiredInfo) {
            case OppoAcquiredInfo::ACQUIRED_GOOD:
                result = AcquiredInfo::GOOD;
                break;
            case OppoAcquiredInfo::ACQUIRED_PARTIAL:
                result = AcquiredInfo::PARTIAL;
                break;
            case OppoAcquiredInfo::ACQUIRED_INSUFFICIENT:
                result = AcquiredInfo::INSUFFICIENT;
                break;
            case OppoAcquiredInfo::ACQUIRED_IMAGER_DIRTY:
                result = AcquiredInfo::SENSOR_DIRTY;
                break;
            case OppoAcquiredInfo::ACQUIRED_TOO_SLOW:
                result = AcquiredInfo::TOO_SLOW;
                break;
            case OppoAcquiredInfo::ACQUIRED_TOO_FAST:
                result = AcquiredInfo::TOO_FAST;
                break;
            case OppoAcquiredInfo::ACQUIRED_VENDOR:
                result = AcquiredInfo::VENDOR;
                break;
            default:
                ALOGE("Unknown acquired info: %d", info);
                result = AcquiredInfo::UNKNOWN;
                break;
        }
    }

    mCb->onAcquired(result, vendorCode);
}

void Session::onAuthenticated(uint32_t fingerId, const hidl_vec<uint8_t>& token) {
    {
        std::lock_guard<std::mutex> lock(mStateMutex);
        // Late result of an identify the framework already ended.
        if (mState != State::AUTHENTICATING) return;
    }

    if (fingerId == 0 || token.size() != sizeof(hw_auth_token_t)) {
        mCb->onAuthenticationFailed();
        mLockoutTracker.addFailedAttempt();
        if (checkSensorLockout()) {
            // The stock service keeps identifying until cancelled, which the
            // stock framework did on lockout.
            setState(State::IDLE);
            HealthMonitor::Scope watch(mHealth, "cancel");
            mDevice->cancel();
        }
        return;
    }

    hw_auth_token_t hat;
    memcpy(&hat, token.data(), sizeof(hat));
    setState(State::IDLE);
    mCb->onAuthenticationSucceeded(fingerId, toHardwareAuthToken(hat));
    mLockoutTracker.reset(true);
}

// The stock service already folds raw codes into ERROR_VENDOR + vendorCode
// (Egis: code - 8, FPC: code - 1000).
void Session::onError(OppoError error, int32_t vendorCode) {
    Error result;
    switch (error) {
        case OppoError::ERROR_NO_ERROR:
            return;
        case OppoError::ERROR_HW_UNAVAILABLE:
            result = Error::HW_UNAVAILABLE;
            mReconnect = true;
            break;
        case OppoError::ERROR_UNABLE_TO_PROCESS:
            result = Error::UNABLE_TO_PROCESS;
            break;
        case OppoError::ERROR_TIMEOUT:
            result = Error::TIMEOUT;
            break;
        case OppoError::ERROR_NO_SPACE:
            result = Error::NO_SPACE;
            break;
        case OppoError::ERROR_CANCELED:
            result = Error::CANCELED;
            break;
        case OppoError::ERROR_UNABLE_TO_REMOVE:
            result = Error::UNABLE_TO_REMOVE;
            break;
        case OppoError::ERROR_VENDOR:
            result = Error::VENDOR;
            break;
        default:
            ALOGE("Unknown error: %d", static_cast<int32_t>(error));
            result = Error::UNABLE_TO_PROCESS;
            vendorCode = 0;
            break;
    }

    {
        std::lock_guard<std::mutex> lock(mStateMutex);
        if (result == Error::CANCELED && mState == State::IDLE) {
            return;  // already reported by cancel()
        }
        if (result != Error::VENDOR) mState = State::IDLE;
    }

    mCb->onError(result, vendorCode);
}

void Session::onRemoved(uint32_t fingerId) {
    {
        std::lock_guard<std::mutex> lock(mTemplatesMutex);
        mTemplates.erase(fingerId);
        saveTemplatesLocked();
    }

    std::lock_guard<std::mutex> lock(mRemoveMutex);
    if (mPendingRemovals.erase(fingerId) == 0) return;
    mRemoved.push_back(fingerId);
    if (mPendingRemovals.empty()) {
        mCb->onEnrollmentsRemoved(mRemoved);
        mRemoved.clear();
    }
}

void Session::onSyncTemplates(const hidl_vec<uint32_t>& fingerIds) {
    {
        std::lock_guard<std::mutex> lock(mTemplatesMutex);
        mTemplates.clear();
        for (uint32_t fid : fingerIds) {
            if (fid != 0) mTemplates.insert(fid);
        }
        saveTemplatesLocked();
    }
}

void Session::saveTemplatesLocked() {
    if (mTemplatesFile.empty()) return;

    std::string tmp = mTemplatesFile + ".tmp";
    {
        std::ofstream out(tmp, std::ios::trunc);
        for (uint32_t fid : mTemplates) out << fid << "\n";
        if (!out.good()) {
            ALOGE("Failed to write %s", tmp.c_str());
            return;
        }
    }
    if (rename(tmp.c_str(), mTemplatesFile.c_str()) != 0) {
        ALOGE("Failed to rename %s: %s", tmp.c_str(), strerror(errno));
    }
}

}  // namespace aidl::android::hardware::biometrics::fingerprint
