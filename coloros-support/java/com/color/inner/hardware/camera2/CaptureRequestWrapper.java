/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package com.color.inner.hardware.camera2;

import android.hardware.camera2.CaptureRequest;
import android.util.Log;

public class CaptureRequestWrapper {
    private static final String TAG = "CaptureRequestWrapper";

    public static <T> CaptureRequest.Key<T> captureRequestKey(String name, Class<T> type) {
        try {
            return new CaptureRequest.Key<>(name, type);
        } catch (Throwable e) {
            Log.e(TAG, e.toString());
            return null;
        }
    }
}
