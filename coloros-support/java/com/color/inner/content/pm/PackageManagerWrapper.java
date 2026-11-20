/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package com.color.inner.content.pm;

public class PackageManagerWrapper {
    private static final String TAG = "PackageManagerWrapper";

    public static void grantRuntimePermission(android.content.pm.PackageManager pm, String packageName, String permissionName, android.os.UserHandle user) {
        try {
            pm.grantRuntimePermission(packageName, permissionName, user);
        } catch (Throwable e) {
            android.util.Log.e(TAG, e.toString());
        }
    }
}
