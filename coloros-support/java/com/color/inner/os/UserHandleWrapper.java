/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package com.color.inner.os;

public class UserHandleWrapper {
    private static final String TAG = "UserHandleWrapper";

    public static android.os.UserHandle createUserHandle(int h) {
        return new android.os.UserHandle(h);
    }

    public static int myUserId() {
        try {
            return android.os.UserHandle.myUserId();
        } catch (Throwable e) {
            android.util.Log.e(TAG, e.toString());
            return -1;
        }
    }
}
