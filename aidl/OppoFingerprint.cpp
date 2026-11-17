/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "OppoFingerprint.h"
#include "Session.h"

namespace aidl::android::hardware::biometrics::fingerprint {

void OppoClientCallback::setSession(std::shared_ptr<Session> session) {
    std::lock_guard<std::mutex> lock(mMutex);
    mSession = std::move(session);
}

std::shared_ptr<Session> OppoClientCallback::getSession() {
    std::lock_guard<std::mutex> lock(mMutex);
    if (mSession == nullptr || mSession->isClosed()) {
        ALOGE("Receiving callbacks without an open session");
        return nullptr;
    }
    return mSession;
}

Return<void> OppoClientCallback::onEnrollResult(uint64_t /*deviceId*/, uint32_t fingerId,
                                                uint32_t /*groupId*/, uint32_t remaining) {
    if (auto session = getSession()) session->onEnrollResult(fingerId, remaining);
    return Void();
}

Return<void> OppoClientCallback::onAcquired(uint64_t /*deviceId*/, OppoAcquiredInfo acquiredInfo,
                                            int32_t vendorCode) {
    if (auto session = getSession()) session->onAcquired(acquiredInfo, vendorCode);
    return Void();
}

Return<void> OppoClientCallback::onAuthenticated(uint64_t /*deviceId*/, uint32_t fingerId,
                                                 uint32_t /*groupId*/,
                                                 const hidl_vec<uint8_t>& token) {
    if (auto session = getSession()) session->onAuthenticated(fingerId, token);
    return Void();
}

Return<void> OppoClientCallback::onError(uint64_t /*deviceId*/, OppoError error,
                                         int32_t vendorCode) {
    if (auto session = getSession()) session->onError(error, vendorCode);
    return Void();
}

Return<void> OppoClientCallback::onRemoved(uint64_t /*deviceId*/, uint32_t fingerId,
                                           uint32_t /*groupId*/, uint32_t /*remaining*/) {
    if (auto session = getSession()) session->onRemoved(fingerId);
    return Void();
}

Return<void> OppoClientCallback::onSyncTemplates(uint64_t /*deviceId*/,
                                                 const hidl_vec<uint32_t>& fingerId,
                                                 uint32_t /*remaining: group id on stock*/) {
    if (auto session = getSession()) session->onSyncTemplates(fingerId);
    return Void();
}

}  // namespace aidl::android::hardware::biometrics::fingerprint
