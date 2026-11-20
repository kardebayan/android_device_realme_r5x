/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package com.color.zoomwindow;

public class ColorZoomWindowManager {
    private static final ColorZoomWindowManager sInstance = new ColorZoomWindowManager();

    public static ColorZoomWindowManager getInstance() {
        return sInstance;
    }

    public ColorZoomWindowInfo getCurrentZoomWindowState() {
        return new ColorZoomWindowInfo();
    }
}
