/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package com.color.inner.os;

public class FileUtilsWrapper {
    private static final String TAG = "FileUtilsWrapper";

    public static void copyFileOrThrow(java.io.File srcFile, java.io.File destFile) {
        try {
            android.os.FileUtils.copyFileOrThrow(srcFile, destFile);
        } catch (Throwable e) {
            android.util.Log.e(TAG, e.toString());
        }
    }
}
