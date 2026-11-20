/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package r5x.coloros;

import android.app.Activity;
import android.app.ActivityThread;
import android.app.KeyguardManager;
import android.content.ActivityNotFoundException;
import android.content.ClipData;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.media.CamcorderProfile;
import android.media.MediaRecorder;
import android.net.Uri;
import android.os.Process;
import android.os.SystemProperties;
import android.provider.MediaStore;
import android.provider.Settings;
import android.util.Log;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.List;

public final class CameraClient {
    private static final String TAG = "CameraClient";
    private static final String PROP_PACKNAME = "oppo.camera.packname";
    private static final String PROP_PID = "oppo.camera.pid";
    private static final String PACKAGE = "com.oppo.camera";

    private CameraClient() {}

    public static void onOpening() {
        try {
            SystemProperties.set(PROP_PID, Integer.toString(Process.myPid()));
            SystemProperties.set(PROP_PACKNAME, PACKAGE);
        } catch (RuntimeException e) {
            Log.w(TAG, "Failed to report camera client", e);
        }
    }

    public static void onReleased() {
        try {
            SystemProperties.set(PROP_PACKNAME, "");
        } catch (RuntimeException e) {
            Log.w(TAG, "Failed to clear camera client", e);
        }
    }

    public static boolean putSystemInt(ContentResolver resolver, String name, int value) {
        try {
            return Settings.System.putInt(resolver, name, value);
        } catch (IllegalArgumentException | SecurityException e) {
            Log.d(TAG, "Not writing System setting " + name + ": " + e.getMessage());
            return false;
        }
    }

    public static Object getLinearmotorVibrator(Context context, String name) {
        Object service = context.getSystemService(name);
        return service instanceof com.oppo.os.LinearmotorVibrator ? service : null;
    }

    private static final ArrayDeque<Uri> sSecureMedia = new ArrayDeque<>();

    public static Uri addLockedImage(ContentResolver resolver, Uri lockedUri, ContentValues values) {
        return addCapturedMedia(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, values);
    }

    public static Uri addLockedVideo(ContentResolver resolver, Uri lockedUri, ContentValues values) {
        return addCapturedMedia(MediaStore.Video.Media.EXTERNAL_CONTENT_URI, values);
    }

    private static Uri addCapturedMedia(Uri collection, ContentValues values) {
        Long id = values != null ? values.getAsLong("media_id") : null;
        if (id == null) {
            return null;
        }
        Uri uri = ContentUris.withAppendedId(collection, id);
        Context context = ActivityThread.currentApplication();
        synchronized (sSecureMedia) {
            if (context == null || !isKeyguardLocked(context)) {
                sSecureMedia.clear();
            } else {
                sSecureMedia.remove(uri);
                sSecureMedia.addFirst(uri);
            }
        }
        return uri;
    }

    public static void clearSecureMedia() {
        synchronized (sSecureMedia) {
            sSecureMedia.clear();
        }
    }

    public static boolean hasSecureMedia(Context context) {
        return !getSecureMedia(context).isEmpty();
    }

    private static List<Uri> getSecureMedia(Context context) {
        List<Uri> uris;
        synchronized (sSecureMedia) {
            if (!isKeyguardLocked(context)) {
                sSecureMedia.clear();
            }
            uris = new ArrayList<>(sSecureMedia);
        }
        List<Uri> gone = new ArrayList<>();
        for (Uri uri : uris) {
            if (!exists(context.getContentResolver(), uri)) {
                gone.add(uri);
            }
        }
        if (!gone.isEmpty()) {
            synchronized (sSecureMedia) {
                sSecureMedia.removeAll(gone);
            }
            uris.removeAll(gone);
        }
        return uris;
    }

    private static boolean exists(ContentResolver resolver, Uri uri) {
        try (Cursor cursor = resolver.query(uri, new String[] {MediaStore.MediaColumns._ID},
                null, null, null)) {
            return cursor != null && cursor.moveToFirst();
        } catch (RuntimeException e) {
            return false;
        }
    }

    private static boolean isKeyguardLocked(Context context) {
        KeyguardManager keyguardManager = context.getSystemService(KeyguardManager.class);
        return keyguardManager != null && keyguardManager.isKeyguardLocked();
    }

    public static void startReview(Activity activity, Intent request) {
        Uri uri = request.getData();
        int extraFlags = request.getFlags() & Intent.FLAG_ACTIVITY_NEW_TASK;

        if (isKeyguardLocked(activity)) {
            List<Uri> secure = getSecureMedia(activity);
            if (!secure.isEmpty()) {
                Intent intent = new Intent(MediaStore.ACTION_REVIEW_SECURE, secure.get(0));
                intent.setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | extraFlags);
                if (secure.size() > 1) {
                    ClipData clipData = ClipData.newUri(
                            activity.getContentResolver(), null, secure.get(1));
                    for (int i = 2; i < secure.size(); i++) {
                        clipData.addItem(activity.getContentResolver(),
                                new ClipData.Item(secure.get(i)));
                    }
                    intent.setClipData(clipData);
                }
                try {
                    activity.startActivity(intent);
                    return;
                } catch (ActivityNotFoundException e) {
                    Log.w(TAG, "No secure review activity", e);
                }
            }
            KeyguardManager keyguardManager = activity.getSystemService(KeyguardManager.class);
            keyguardManager.requestDismissKeyguard(activity,
                    new KeyguardManager.KeyguardDismissCallback() {
                        @Override
                        public void onDismissSucceeded() {
                            openReview(activity, uri, extraFlags);
                        }
                    });
            return;
        }

        openReview(activity, uri, extraFlags);
    }

    private static void openReview(Activity activity, Uri uri, int extraFlags) {
        if (uri == null) {
            return;
        }
        for (String action : new String[] {MediaStore.ACTION_REVIEW, Intent.ACTION_VIEW}) {
            Intent intent = new Intent(action, uri);
            intent.setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | extraFlags);
            try {
                activity.startActivity(intent);
                return;
            } catch (ActivityNotFoundException e) {
            }
        }
        Log.w(TAG, "No activity to review " + uri);
    }

    public static void setRecorderProfile(MediaRecorder recorder, CamcorderProfile profile) {
        recorder.setOutputFormat(profile.fileFormat);
        recorder.setVideoFrameRate(profile.videoFrameRate);
        recorder.setVideoSize(profile.videoFrameWidth, profile.videoFrameHeight);
        recorder.setVideoEncodingBitRate(profile.videoBitRate);
        recorder.setVideoEncoder(profile.videoCodec);
        boolean timeLapse = profile.quality >= CamcorderProfile.QUALITY_TIME_LAPSE_LOW
                && profile.quality <= CamcorderProfile.QUALITY_TIME_LAPSE_QVGA;
        if (!timeLapse && profile.audioCodec >= 0) {
            recorder.setAudioEncodingBitRate(profile.audioBitRate);
            recorder.setAudioChannels(profile.audioChannels);
            recorder.setAudioSamplingRate(profile.audioSampleRate);
            recorder.setAudioEncoder(profile.audioCodec);
        }
    }
}
