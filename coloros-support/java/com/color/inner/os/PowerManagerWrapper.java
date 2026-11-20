/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package com.color.inner.os;

public class PowerManagerWrapper {
    private static final String TAG = "PowerManagerWrapper";

    public static int getMaximumScreenBrightnessSetting(android.os.PowerManager powerManager) {
        try {
            return powerManager.getMaximumScreenBrightnessSetting();
        } catch (Throwable e) {
            android.util.Log.e(TAG, e.toString());
            return -1;
        }
    }
}
