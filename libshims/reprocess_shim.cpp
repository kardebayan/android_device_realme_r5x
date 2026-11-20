/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "r5x_reprocess_shim"

#include <mutex>

#include <log/log.h>
#include <system/camera_metadata.h>

namespace {
constexpr uint32_t kReprocessFlagsTag = 0x800d0000;
constexpr size_t kDeferredCount = 4;

std::mutex gLock;
camera_metadata_t* gDeferred[kDeferredCount];
size_t gNext;

bool isReprocessSettings(const camera_metadata_t* metadata) {
    camera_metadata_ro_entry_t entry;
    return find_camera_metadata_ro_entry(metadata, kReprocessFlagsTag, &entry) == 0;
}

}  // namespace

extern "C" void r5x_reprocess_mdfree(camera_metadata_t* metadata) {
    if (metadata == nullptr) {
        return;
    }
    if (!isReprocessSettings(metadata)) {
        free_camera_metadata(metadata);
        return;
    }

    camera_metadata_t* oldest;
    {
        std::lock_guard<std::mutex> lock(gLock);
        oldest = gDeferred[gNext];
        gDeferred[gNext] = metadata;
        gNext = (gNext + 1) % kDeferredCount;
    }
    if (oldest != nullptr) {
        free_camera_metadata(oldest);
    }
}
