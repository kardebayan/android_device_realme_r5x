/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package com.color.inner.os;

public class TraceWrapper {
    private static final String TAG = "TraceWrapper";

    public static void traceBegin(long traceTag, String methodName) {
        try {
            android.os.Trace.traceBegin(traceTag, methodName);
        } catch (Throwable e) {
            android.util.Log.e(TAG, e.toString());
        }
    }

    public static void traceEnd(long traceTag) {
        try {
            android.os.Trace.traceEnd(traceTag);
        } catch (Throwable e) {
            android.util.Log.e(TAG, e.toString());
        }
    }
}
