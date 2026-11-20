/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package com.color.inner.hardware.camera2.impl;

import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CaptureResult;
import android.hardware.camera2.impl.CameraMetadataNative;
import android.util.Log;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;

public class CameraMetadataNativeWrapper {
    private static final String TAG = "CameraMetadataNativeWrapper";

    static {
        System.loadLibrary("coloros_support_jni");
    }

    private static native int nativeGetBufSize(Object metadataNative);
    private static native int nativeCopyBuf(Object metadataNative, long address);

    private static CameraMetadataNative getCameraMetadataNativeObj(Object obj) {
        if (obj == null) {
            return null;
        }
        try {
            Field field;
            if (obj instanceof CameraCharacteristics) {
                field = CameraCharacteristics.class.getDeclaredField("mProperties");
            } else if (obj instanceof CaptureResult) {
                field = CaptureResult.class.getDeclaredField("mResults");
            } else {
                return null;
            }
            field.setAccessible(true);
            return (CameraMetadataNative) field.get(obj);
        } catch (ReflectiveOperationException | RuntimeException e) {
            Log.e(TAG, e.toString());
            return null;
        }
    }

    public static int copyBuf(Object obj, long address) {
        CameraMetadataNative metadata = getCameraMetadataNativeObj(obj);
        if (metadata == null) {
            return -1;
        }
        return nativeCopyBuf(metadata, address);
    }

    public static int getBufSize(Object obj) {
        CameraMetadataNative metadata = getCameraMetadataNativeObj(obj);
        if (metadata == null) {
            return 0;
        }
        return nativeGetBufSize(metadata);
    }

    public static ConcurrentHashMap<CaptureResult.Key<?>, Integer> getVendorTagId(
            CaptureResult result) {
        if (result == null) {
            Log.e(TAG, "getVendorTagId, CaptureResult is null, return");
            return null;
        }
        CameraMetadataNative metadata = getCameraMetadataNativeObj(result);
        if (metadata == null) {
            return null;
        }
        ConcurrentHashMap<CaptureResult.Key<?>, Integer> vendorKeyMap = new ConcurrentHashMap<>();
        try {
            Field ptrField = CameraMetadataNative.class.getDeclaredField("mMetadataPtr");
            ptrField.setAccessible(true);
            long ptr = ptrField.getLong(metadata);
            Method getTag = CameraMetadataNative.class.getDeclaredMethod(
                    "nativeGetTagFromKeyLocal", long.class, String.class);
            getTag.setAccessible(true);
            HashMap<CaptureResult.Key<?>, Integer> tags = new HashMap<>();
            List<CaptureResult.Key<?>> keys = r5x.coloros.CameraKeys.getResultKeys(result);
            for (CaptureResult.Key<?> key : keys) {
                if (key.getName().contains("android")) {
                    continue;
                }
                tags.put(key, (Integer) getTag.invoke(null, ptr, key.getName()));
            }
            vendorKeyMap.putAll(tags);
            return vendorKeyMap;
        } catch (ReflectiveOperationException | RuntimeException e) {
            Log.e(TAG, "getVendorTagId error", e);
            return null;
        }
    }
}
