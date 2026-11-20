/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package r5x.coloros;

import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.CaptureResult;
import android.hardware.camera2.impl.CameraMetadataNative;
import android.util.Log;

import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

public final class CameraKeys {
    private static final String TAG = "CameraKeys";

    private CameraKeys() {}

    public static List<CaptureRequest.Key<?>> getRequestKeys(CaptureRequest request) {
        List<CaptureRequest.Key<?>> keys = request.getKeys();
        try {
            return merge(keys, request.getNativeCopy().getAllVendorKeys(CaptureRequest.Key.class));
        } catch (RuntimeException e) {
            Log.w(TAG, "Failed to list vendor request keys", e);
            return keys;
        }
    }

    public static List<CaptureResult.Key<?>> getResultKeys(CaptureResult result) {
        List<CaptureResult.Key<?>> keys = result.getKeys();
        try {
            return merge(keys, result.getNativeCopy().getAllVendorKeys(CaptureResult.Key.class));
        } catch (RuntimeException e) {
            Log.w(TAG, "Failed to list vendor result keys", e);
            return keys;
        }
    }

    @SuppressWarnings({"rawtypes", "unchecked"})
    private static <K> List<K> merge(List<K> keys, ArrayList vendorKeys) {
        if (vendorKeys == null || vendorKeys.isEmpty()) {
            return keys;
        }
        Set<K> all = new LinkedHashSet<>(keys);
        all.addAll((List<K>) vendorKeys);
        return new ArrayList<>(all);
    }
}
