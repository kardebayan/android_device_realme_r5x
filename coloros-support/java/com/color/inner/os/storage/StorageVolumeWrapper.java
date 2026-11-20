/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package com.color.inner.os.storage;

public class StorageVolumeWrapper {
    private static final String TAG = "StorageVolumeWrapper";

    public static String getPath(android.os.storage.StorageVolume storageVolume) {
        try {
            return storageVolume.getPath();
        } catch (Throwable e) {
            android.util.Log.e(TAG, e.toString());
            return null;
        }
    }

    public static int getReadOnlyType(android.os.storage.StorageVolume storageVolume) {
        return -1;
    }
}
