/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package android.app;

import android.content.Context;

import java.util.ArrayList;

public class OppoWhiteListManager {
    public OppoWhiteListManager(Context context) {}

    public void addStageProtectInfo(String pkg, long timeout) {}

    public void removeStageProtectInfo(String pkg) {}

    public ArrayList<String> getStageProtectListFromPkg(String pkg, int type) {
        return new ArrayList<>();
    }
}
