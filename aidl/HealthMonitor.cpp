/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "HealthMonitor.h"

#include <signal.h>

#include <log/log.h>

namespace aidl::android::hardware::biometrics::fingerprint {

namespace {
constexpr auto kBinderCallTimeout = std::chrono::milliseconds(10000);
}  // namespace

HealthMonitor::Scope::Scope(HealthMonitor& monitor, const char* api)
    : mMonitor(monitor), mId(monitor.start(api)) {}

HealthMonitor::Scope::~Scope() {
    mMonitor.stop(mId);
}

HealthMonitor::HealthMonitor() : mThread([this] { run(); }) {
    mThread.detach();
}

void HealthMonitor::setPid(pid_t pid) {
    std::lock_guard<std::mutex> lock(mMutex);
    mPid = pid;
}

uint64_t HealthMonitor::start(const char* api) {
    std::lock_guard<std::mutex> lock(mMutex);
    uint64_t id = mNextId++;
    mCalls[id] = {api, Clock::now() + kBinderCallTimeout};
    mCv.notify_all();
    return id;
}

void HealthMonitor::stop(uint64_t id) {
    std::lock_guard<std::mutex> lock(mMutex);
    mCalls.erase(id);
    mCv.notify_all();
}

void HealthMonitor::run() {
    std::unique_lock<std::mutex> lock(mMutex);
    while (true) {
        if (mCalls.empty()) {
            mCv.wait(lock);
            continue;
        }

        auto earliest = mCalls.begin();
        for (auto it = mCalls.begin(); it != mCalls.end(); ++it) {
            if (it->second.deadline < earliest->second.deadline) earliest = it;
        }
        if (Clock::now() < earliest->second.deadline) {
            mCv.wait_until(lock, earliest->second.deadline);
            continue;
        }

        ALOGE("%s blocked for %lldms, resetting the fingerprint service (pid %d)",
              earliest->second.api, static_cast<long long>(kBinderCallTimeout.count()), mPid);
        if (mPid > 0) kill(mPid, SIGQUIT);
        mCalls.clear();
    }
}

}  // namespace aidl::android::hardware::biometrics::fingerprint
