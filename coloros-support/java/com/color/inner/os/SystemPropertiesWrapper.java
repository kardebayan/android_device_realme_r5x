/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package com.color.inner.os;

public class SystemPropertiesWrapper {
    private static final String TAG = "SystemPropertiesWrapper";

    public static String get(String key) {
        try {
            return android.os.SystemProperties.get(key);
        } catch (Throwable e) {
            android.util.Log.e(TAG, e.toString());
            return null;
        }
    }

    public static String get(String key, String def) {
        try {
            return android.os.SystemProperties.get(key, def);
        } catch (Throwable e) {
            android.util.Log.e(TAG, e.toString());
            return def;
        }
    }

    public static boolean getBoolean(String key, boolean def) {
        try {
            return android.os.SystemProperties.getBoolean(key, def);
        } catch (Throwable e) {
            android.util.Log.e(TAG, e.toString());
            return def;
        }
    }

    public static int getInt(String key, int def) {
        try {
            return android.os.SystemProperties.getInt(key, def);
        } catch (Throwable e) {
            android.util.Log.e(TAG, e.toString());
            return def;
        }
    }

    public static void set(String key, String val) {
        try {
            android.os.SystemProperties.set(key, val);
        } catch (Throwable e) {
            android.util.Log.e(TAG, e.toString());
        }
    }
}
