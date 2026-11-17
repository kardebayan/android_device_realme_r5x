/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <sys/types.h>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <thread>

namespace aidl::android::hardware::biometrics::fingerprint {

// Same watchdog the stock FingerprintService (tool/HealthMonitor) ran around every call
// into the vendor.oppo service: a call blocking for 10s gets the service SIGQUIT, so
// init restarts it (and us with it, through the death recipient).
class HealthMonitor {
  public:
    class Scope {
      public:
        Scope(HealthMonitor& monitor, const char* api);
        ~Scope();

      private:
        HealthMonitor& mMonitor;
        uint64_t mId;
    };

    HealthMonitor();
    void setPid(pid_t pid);

  private:
    using Clock = std::chrono::steady_clock;
    struct Call {
        const char* api;
        Clock::time_point deadline;
    };

    uint64_t start(const char* api);
    void stop(uint64_t id);
    void run();

    std::mutex mMutex;
    std::condition_variable mCv;
    std::map<uint64_t, Call> mCalls;
    uint64_t mNextId = 0;
    pid_t mPid = -1;
    std::thread mThread;
};

}  // namespace aidl::android::hardware::biometrics::fingerprint
