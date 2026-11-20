/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package com.color.inner.os.storage;

public class StorageManagerWrapper {
    private static final String TAG = "StorageManagerWrapper";

    public static android.os.storage.StorageVolume[] getVolumeList(int userId, int flags) {
        try {
            return android.os.storage.StorageManager.getVolumeList(userId, flags);
        } catch (Throwable e) {
            android.util.Log.e(TAG, e.toString());
            return null;
        }
    }
}
