package com.mixpractice.studio;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Build;
import android.util.Log;

import org.json.JSONObject;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;
import java.util.UUID;

/**
 * Direct Android Java Telemetry Reporter for Mix Practice Studio.
 * Ensures installation and active user metrics are reliably delivered to Firebase Firestore
 * on app startup, even if C++ JNI bridge is not yet initialized.
 */
public class TelemetryHelper {
    private static final String TAG = "MixPracticeTelemetry";

    private static final String FIREBASE_PROJECT_ID = "pad-mix";
    private static final String FIREBASE_API_KEY = "AIzaSyDrnDzAK1WLPgdKRUq__zYpHpag6vOFUQ8";

    public static void reportAppLaunchAsync(final Context context) {
        if (context == null) return;

        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    // Small delay to ensure network connectivity on cold launch
                    Thread.sleep(1500);
                    reportAppLaunchInternal(context.getApplicationContext());
                } catch (Throwable t) {
                    Log.e(TAG, "Error in telemetry background thread: " + t.getMessage(), t);
                }
            }
        }, "TelemetryReportThread").start();
    }

    private static void reportAppLaunchInternal(Context context) {
        try {
            // Load or initialize persistent telemetry state
            File appDir = new File(context.getFilesDir(), "MixPracticeStudio");
            if (!appDir.exists()) {
                appDir.mkdirs();
            }
            File stateFile = new File(appDir, "telemetry_state.json");

            String installId = "";
            String firstInstallTime = "";
            int launchCount = 0;

            SimpleDateFormat isoFormat = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSSXXX", Locale.US);
            isoFormat.setTimeZone(TimeZone.getTimeZone("UTC"));
            String nowIso = isoFormat.format(new Date());

            if (stateFile.exists()) {
                try (FileInputStream fis = new FileInputStream(stateFile)) {
                    byte[] data = new byte[(int) stateFile.length()];
                    fis.read(data);
                    JSONObject json = new JSONObject(new String(data, StandardCharsets.UTF_8));
                    installId = json.optString("installId", "");
                    firstInstallTime = json.optString("firstInstallTime", "");
                    launchCount = json.optInt("launchCount", 0);
                } catch (Throwable ignored) {}
            }

            SharedPreferences sp = context.getSharedPreferences("mps_telemetry", Context.MODE_PRIVATE);
            if (installId.isEmpty()) {
                installId = sp.getString("install_id", "");
            }
            if (installId.isEmpty()) {
                installId = UUID.randomUUID().toString();
                sp.edit().putString("install_id", installId).apply();
            }

            if (firstInstallTime.isEmpty()) {
                firstInstallTime = nowIso;
            }

            launchCount++;

            // Save updated local state
            try {
                JSONObject json = new JSONObject();
                json.put("installId", installId);
                json.put("firstInstallTime", firstInstallTime);
                json.put("lastSyncTime", nowIso);
                json.put("launchCount", launchCount);
                json.put("syncedFirstInstall", true);
                try (FileOutputStream fos = new FileOutputStream(stateFile)) {
                    fos.write(json.toString().getBytes(StandardCharsets.UTF_8));
                }
            } catch (Throwable ignored) {}

            // Build Firestore REST payload
            JSONObject fields = new JSONObject();
            fields.put("installId",        new JSONObject().put("stringValue", installId));
            fields.put("platform",         new JSONObject().put("stringValue", "Android"));
            fields.put("osName",           new JSONObject().put("stringValue", "Android " + Build.VERSION.RELEASE));
            fields.put("appVersion",       new JSONObject().put("stringValue", "1.0.0"));
            fields.put("deviceModel",      new JSONObject().put("stringValue", Build.MANUFACTURER + " " + Build.MODEL));
            fields.put("cpuModel",         new JSONObject().put("stringValue", Build.HARDWARE != null ? Build.HARDWARE : "arm64"));
            fields.put("firstInstallTime", new JSONObject().put("stringValue", firstInstallTime));
            fields.put("lastSeenTime",     new JSONObject().put("stringValue", nowIso));
            fields.put("launchCount",      new JSONObject().put("integerValue", String.valueOf(launchCount)));

            JSONObject root = new JSONObject();
            root.put("fields", fields);
            String payload = root.toString();

            String endpoint = "https://firestore.googleapis.com/v1/projects/"
                    + FIREBASE_PROJECT_ID
                    + "/databases/(default)/documents/installs/"
                    + installId
                    + "?key=" + FIREBASE_API_KEY;

            for (int attempt = 1; attempt <= 3; attempt++) {
                HttpURLConnection conn = null;
                try {
                    URL url = new URL(endpoint);
                    conn = (HttpURLConnection) url.openConnection();
                    conn.setRequestMethod("POST");
                    conn.setRequestProperty("Content-Type", "application/json");
                    conn.setRequestProperty("X-HTTP-Method-Override", "PATCH");
                    conn.setConnectTimeout(10000);
                    conn.setReadTimeout(10000);
                    conn.setDoOutput(true);

                    try (OutputStream os = conn.getOutputStream()) {
                        os.write(payload.getBytes(StandardCharsets.UTF_8));
                        os.flush();
                    }

                    int responseCode = conn.getResponseCode();
                    if (responseCode >= 200 && responseCode < 300) {
                        Log.i(TAG, "Successfully reported install telemetry to Firebase Firestore (HTTP " + responseCode + ")");
                        break;
                    } else {
                        Log.w(TAG, "Firestore returned HTTP " + responseCode + " on attempt " + attempt);
                    }
                } catch (Throwable t) {
                    Log.w(TAG, "Failed telemetry attempt " + attempt + ": " + t.getMessage());
                } finally {
                    if (conn != null) {
                        conn.disconnect();
                    }
                }

                if (attempt < 3) {
                    Thread.sleep(3000);
                }
            }
        } catch (Throwable t) {
            Log.e(TAG, "Failed to report telemetry: " + t.getMessage(), t);
        }
    }

    /**
     * Reports a Shopee Affiliate sponsor redirect event to Firebase Firestore.
     */
    public static void reportShopeeClickAsync(final Context context, final String targetUrl) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                HttpURLConnection conn = null;
                try {
                    SimpleDateFormat isoFormat = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSSXXX", Locale.US);
                    isoFormat.setTimeZone(TimeZone.getTimeZone("UTC"));
                    String nowIso = isoFormat.format(new Date());

                    JSONObject fields = new JSONObject();
                    fields.put("timestamp", new JSONObject().put("stringValue", nowIso));
                    fields.put("platform", new JSONObject().put("stringValue", "Android"));
                    fields.put("osName", new JSONObject().put("stringValue", "Android " + Build.VERSION.RELEASE));
                    fields.put("deviceModel", new JSONObject().put("stringValue", Build.MANUFACTURER + " " + Build.MODEL));
                    fields.put("targetUrl", new JSONObject().put("stringValue", targetUrl != null ? targetUrl : ""));

                    JSONObject root = new JSONObject();
                    root.put("fields", fields);
                    String payload = root.toString();

                    String endpoint = "https://firestore.googleapis.com/v1/projects/" + FIREBASE_PROJECT_ID
                            + "/databases/(default)/documents/shopee_clicks?key=" + FIREBASE_API_KEY;

                    URL url = new URL(endpoint);
                    conn = (HttpURLConnection) url.openConnection();
                    conn.setRequestMethod("POST");
                    conn.setRequestProperty("Content-Type", "application/json");
                    conn.setConnectTimeout(8000);
                    conn.setReadTimeout(8000);
                    conn.setDoOutput(true);

                    try (OutputStream os = conn.getOutputStream()) {
                        os.write(payload.getBytes(StandardCharsets.UTF_8));
                        os.flush();
                    }

                    int responseCode = conn.getResponseCode();
                    Log.i(TAG, "Shopee click reported to Firestore (HTTP " + responseCode + ")");
                } catch (Throwable t) {
                    Log.w(TAG, "Failed to report Shopee click: " + t.getMessage());
                } finally {
                    if (conn != null) {
                        conn.disconnect();
                    }
                }
            }
        }, "ShopeeReportThread").start();
    }

    /**
     * Safely opens an external web browser or Shopee app with complete exception handling
     * so it never crashes even if no browser is installed or intent fails.
     */
    public static boolean openBrowserSafely(final Context context, final String urlString) {
        if (context == null || urlString == null || urlString.trim().isEmpty()) {
            return false;
        }
        try {
            android.content.Intent intent = new android.content.Intent(
                android.content.Intent.ACTION_VIEW,
                android.net.Uri.parse(urlString.trim())
            );
            intent.addFlags(android.content.Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(intent);
            return true;
        } catch (Throwable t) {
            Log.e(TAG, "Failed to launch URL safely: " + t.getMessage(), t);
            return false;
        }
    }
}

