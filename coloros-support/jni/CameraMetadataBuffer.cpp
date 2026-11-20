/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "ColorOSCameraMetadata"

#include <memory>

#include <camera/CameraMetadata.h>
#include <jni.h>
#include <log/log.h>
#include <system/camera_metadata.h>

using android::CameraMetadata;

namespace {
jfieldID gMetadataPtr;

CameraMetadata* getMetadata(JNIEnv* env, jobject metadataNative) {
    if (metadataNative == nullptr || gMetadataPtr == nullptr) {
        return nullptr;
    }
    auto ptr = reinterpret_cast<std::shared_ptr<CameraMetadata>*>(
            env->GetLongField(metadataNative, gMetadataPtr));
    if (ptr == nullptr || *ptr == nullptr) {
        return nullptr;
    }
    return ptr->get();
}

size_t metadataSize(const camera_metadata_t* raw) {
    return calculate_camera_metadata_size(get_camera_metadata_entry_count(raw),
                                          get_camera_metadata_data_count(raw));
}

jint nativeGetBufSize(JNIEnv* env, jclass, jobject metadataNative) {
    CameraMetadata* metadata = getMetadata(env, metadataNative);
    if (metadata == nullptr) {
        return 0;
    }
    const camera_metadata_t* raw = metadata->getAndLock();
    jint size = raw != nullptr ? static_cast<jint>(metadataSize(raw)) : 0;
    metadata->unlock(raw);
    return size;
}

jint nativeCopyBuf(JNIEnv* env, jclass, jobject metadataNative, jlong address) {
    CameraMetadata* metadata = getMetadata(env, metadataNative);
    if (metadata == nullptr || address == 0) {
        return -1;
    }
    const camera_metadata_t* raw = metadata->getAndLock();
    jint ret = -1;
    if (raw != nullptr) {
        copy_camera_metadata(reinterpret_cast<void*>(address), metadataSize(raw), raw);
        ret = 0;
    }
    metadata->unlock(raw);
    return ret;
}

const JNINativeMethod gMethods[] = {
        {"nativeGetBufSize", "(Ljava/lang/Object;)I", reinterpret_cast<void*>(nativeGetBufSize)},
        {"nativeCopyBuf", "(Ljava/lang/Object;J)I", reinterpret_cast<void*>(nativeCopyBuf)},
};
}  // namespace

jint JNI_OnLoad(JavaVM* vm, void*) {
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    jclass metadataClass = env->FindClass("android/hardware/camera2/impl/CameraMetadataNative");
    if (metadataClass == nullptr) {
        env->ExceptionClear();
        ALOGE("CameraMetadataNative not found");
        return JNI_ERR;
    }
    gMetadataPtr = env->GetFieldID(metadataClass, "mMetadataPtr", "J");
    if (gMetadataPtr == nullptr) {
        env->ExceptionClear();
        ALOGE("CameraMetadataNative.mMetadataPtr not found");
        return JNI_ERR;
    }

    jclass wrapperClass =
            env->FindClass("com/color/inner/hardware/camera2/impl/CameraMetadataNativeWrapper");
    if (wrapperClass == nullptr ||
        env->RegisterNatives(wrapperClass, gMethods, std::size(gMethods)) != JNI_OK) {
        env->ExceptionClear();
        ALOGE("Failed to register natives");
        return JNI_ERR;
    }
    return JNI_VERSION_1_6;
}
