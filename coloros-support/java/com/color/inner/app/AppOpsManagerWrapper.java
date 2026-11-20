/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package com.color.inner.app;

public class AppOpsManagerWrapper {
    private static final String TAG = "AppOpsManagerWrapper";

    public static void setUidMode(android.app.AppOpsManager appOpsManager, String appOp, int uid, int mode) {
        try {
            appOpsManager.setUidMode(appOp, uid, mode);
        } catch (Throwable e) {
            android.util.Log.e(TAG, e.toString());
        }
    }
}
