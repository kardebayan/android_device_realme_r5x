/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package com.color.inner.view;

public class WindowManagerWrapper {
    private static final String TAG = "WindowManagerWrapper";

    public static int getInitialDisplayDensity(int displayId) {
        try {
            return android.view.WindowManagerGlobal.getWindowManagerService().getInitialDisplayDensity(displayId);
        } catch (Exception e) {
            android.util.Log.w(TAG, e.toString());
            return -1;
        }
    }

    public static boolean hasNavigationBar(int displayId) {
        try {
            return android.view.WindowManagerGlobal.getWindowManagerService().hasNavigationBar(displayId);
        } catch (Exception e) {
            android.util.Log.w(TAG, e.toString());
            return false;
        }
    }
}
