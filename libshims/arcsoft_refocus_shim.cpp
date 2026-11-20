/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdlib.h>

namespace {
constexpr size_t kPad = 64;

bool padded(size_t size, size_t* out) {
    return !__builtin_add_overflow(size, kPad, out);
}
}  // namespace

extern "C" void* MMemAlloc(void* handle, size_t size);

extern "C" void* r5xMAlloc(void* handle, size_t size) {
    size_t total;
    if (handle != nullptr || !padded(size, &total)) {
        return MMemAlloc(handle, size);
    }
    return MMemAlloc(handle, total);
}

extern "C" void* r5xmlc(size_t size) {
    size_t total;
    return padded(size, &total) ? malloc(total) : nullptr;
}

extern "C" void* r5xclc(size_t count, size_t size) {
    size_t bytes, total;
    if (__builtin_mul_overflow(count, size, &bytes) || !padded(bytes, &total)) {
        return nullptr;
    }
    return calloc(1, total);
}

extern "C" void* r5xrllc(void* ptr, size_t size) {
    size_t total;
    if (size == 0 || !padded(size, &total)) {
        return realloc(ptr, size);
    }
    return realloc(ptr, total);
}

extern "C" int r5x_posix_mema(void** out, size_t alignment, size_t size) {
    size_t total;
    if (!padded(size, &total)) {
        return posix_memalign(out, alignment, size);
    }
    return posix_memalign(out, alignment, total);
}
