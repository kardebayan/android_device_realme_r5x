/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package com.color.inner.content;

public class ContextWrapper {
    private static final String TAG = "ContextWrapper";

    public static android.view.Display getDisplay(android.content.Context context) {
        try {
            return context.getDisplay();
        } catch (Throwable e) {
            android.util.Log.e(TAG, e.toString());
            return null;
        }
    }
}
