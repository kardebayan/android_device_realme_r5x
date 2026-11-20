/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package com.color.inner.hardware.display;

public class DisplayManagerWrapper {
    public static void setTemporaryAutoBrightnessAdjustment(
            android.hardware.display.DisplayManager displayManager, float adjustment) {
        displayManager.setTemporaryAutoBrightnessAdjustment(adjustment);
    }
}
