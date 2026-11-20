/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package com.color.util;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.provider.Settings;
import android.telephony.TelephonyManager;
import android.util.Log;

public class ColorNetworkUtil {
    private static final String TAG = "ColorNetworkUtil";
    private static final String KEY_NETWORK_MONITOR_AVAILABLE = "oppo.comm.network.monitor.available";
    private static final String KEY_NETWORK_MONITOR_PORTAL = "oppo.comm.network.monitor.portal";
    private static final String KEY_NETWORK_MONITOR_SSID = "oppo.comm.network.monitor.ssid";

    private static String sCurrSSID;

    public static boolean isWifiConnected(Context context) {
        ConnectivityManager cm = context.getSystemService(ConnectivityManager.class);
        NetworkInfo info = cm != null ? cm.getNetworkInfo(ConnectivityManager.TYPE_WIFI) : null;
        if (info != null && info.getState() == NetworkInfo.State.CONNECTED) {
            sCurrSSID = info.getExtraInfo();
            return true;
        }
        return false;
    }

    public static boolean isMobileDataConnected(Context context) {
        ConnectivityManager cm = context.getSystemService(ConnectivityManager.class);
        NetworkInfo info = cm != null ? cm.getNetworkInfo(ConnectivityManager.TYPE_MOBILE) : null;
        return info != null && info.getState() == NetworkInfo.State.CONNECTED;
    }

    public static int getErrorString(Context context, String url) {
        boolean btTether = isBluetoothTetherConnected(context);
        boolean airplane = Settings.Global.getInt(
                context.getContentResolver(), Settings.Global.AIRPLANE_MODE_ON, 0) != 0;
        if (airplane && !isWifiConnected(context) && !btTether) {
            return 0;
        }
        if (isWifiConnected(context) || btTether) {
            return getCaptivePortalStr(context);
        }
        if (hasSimCard(context)) {
            return isMobileDataConnected(context) ? -1 : 1;
        }
        return 3;
    }

    private static int getCaptivePortalStr(Context context) {
        String ssid = Settings.Global.getString(
                context.getContentResolver(), KEY_NETWORK_MONITOR_SSID);
        if (sCurrSSID == null || ssid == null || !sCurrSSID.equals(ssid)) {
            return 3;
        }
        if (Settings.Global.getInt(context.getContentResolver(),
                KEY_NETWORK_MONITOR_AVAILABLE, 0) == 1) {
            return -1;
        }
        return Settings.Global.getInt(context.getContentResolver(),
                KEY_NETWORK_MONITOR_PORTAL, 0) == 1 ? 2 : 3;
    }

    private static boolean hasSimCard(Context context) {
        try {
            TelephonyManager tm = context.getSystemService(TelephonyManager.class);
            return tm != null && (tm.hasIccCard(0) || tm.hasIccCard(1));
        } catch (Exception e) {
            Log.e(TAG, "", e);
            return false;
        }
    }

    private static boolean isBluetoothTetherConnected(Context context) {
        ConnectivityManager cm = context.getSystemService(ConnectivityManager.class);
        if (cm == null) {
            return false;
        }
        Network network = cm.getActiveNetwork();
        NetworkCapabilities caps = network != null ? cm.getNetworkCapabilities(network) : null;
        return caps != null && caps.hasTransport(NetworkCapabilities.TRANSPORT_BLUETOOTH);
    }
}
