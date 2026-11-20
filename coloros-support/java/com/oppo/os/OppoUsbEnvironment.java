/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package com.oppo.os;

import android.content.Context;
import android.os.Environment;
import android.os.UserHandle;
import android.os.storage.DiskInfo;
import android.os.storage.StorageManager;
import android.os.storage.VolumeInfo;
import android.util.Log;

import java.io.File;

public class OppoUsbEnvironment {
    private static final String TAG = "OppoUsbEnvironment";

    public OppoUsbEnvironment() {}

    private static String findPath(Context context, boolean internal) {
        StorageManager sm = context.getSystemService(StorageManager.class);
        if (sm == null) {
            return null;
        }
        try {
            for (VolumeInfo vol : sm.getVolumes()) {
                String path = vol.path;
                if (path == null) {
                    continue;
                }
                if (vol.type == VolumeInfo.TYPE_EMULATED) {
                    if (internal) {
                        return path + "/" + UserHandle.myUserId();
                    }
                } else if (!internal) {
                    DiskInfo disk = vol.getDisk();
                    if (disk != null && disk.isSd()) {
                        return path;
                    }
                }
            }
        } catch (RuntimeException e) {
            Log.e(TAG, "getVolumes failed", e);
        }
        return null;
    }

    private static String getState(Context context, String path) {
        StorageManager sm = path != null ? context.getSystemService(StorageManager.class) : null;
        return sm != null ? sm.getVolumeState(path) : Environment.MEDIA_UNKNOWN;
    }

    public static File getInternalSdDirectory(Context context) {
        String path = findPath(context, true);
        return path != null ? new File(path) : null;
    }

    public static String getInternalSdState(Context context) {
        return getState(context, findPath(context, true));
    }

    public static File getExternalSdDirectory(Context context) {
        String path = findPath(context, false);
        return path != null ? new File(path) : null;
    }

    public static String getExternalSdState(Context context) {
        return getState(context, findPath(context, false));
    }
}
