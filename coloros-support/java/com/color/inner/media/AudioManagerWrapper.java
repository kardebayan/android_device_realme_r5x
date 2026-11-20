/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package com.color.inner.media;

public class AudioManagerWrapper {
    private static final String TAG = "AudioManagerWrapper";

    public static int getRingerModeInternal(android.media.AudioManager audioManager) {
        try {
            return audioManager.getRingerModeInternal();
        } catch (Throwable e) {
            android.util.Log.e(TAG, e.toString());
            return 2;
        }
    }

    public static void setRingerModeInternal(android.media.AudioManager audioManager, int ringerMode) {
        try {
            audioManager.setRingerModeInternal(ringerMode);
        } catch (Throwable e) {
            android.util.Log.e(TAG, e.toString());
        }
    }
}
