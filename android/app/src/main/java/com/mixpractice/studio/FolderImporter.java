package com.mixpractice.studio;

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.provider.DocumentsContract;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;

public class FolderImporter {

    public static String[] importAudioFromTree(Context context, String treeUriString, String destDirPath) {
        ArrayList<String> importedPaths = new ArrayList<>();
        if (context == null || treeUriString == null || destDirPath == null) {
            return new String[0];
        }
        try {
            Uri treeUri = Uri.parse(treeUriString);
            ContentResolver resolver = context.getContentResolver();
            String treeDocId = DocumentsContract.getTreeDocumentId(treeUri);
            File destDir = new File(destDirPath);
            if (!destDir.exists()) {
                destDir.mkdirs();
            }
            scanDirectory(resolver, treeUri, treeDocId, destDir, importedPaths);
        } catch (Throwable t) {
            android.util.Log.e("MixPracticeStudio", "Error importing tree: " + t.getMessage(), t);
        }
        return importedPaths.toArray(new String[0]);
    }

    public static String[] importAudioDocuments(Context context, String[] uriStrings, String destDirPath) {
        ArrayList<String> importedPaths = new ArrayList<>();
        if (context == null || uriStrings == null || destDirPath == null) {
            return new String[0];
        }
        try {
            ContentResolver resolver = context.getContentResolver();
            File destDir = new File(destDirPath);
            if (!destDir.exists()) {
                destDir.mkdirs();
            }
            for (String uriStr : uriStrings) {
                if (uriStr == null || uriStr.isEmpty()) continue;
                Uri uri = Uri.parse(uriStr);
                String name = null;
                String mimeType = null;
                try {
                    mimeType = resolver.getType(uri);
                } catch (Throwable ignored) {}

                Cursor c = null;
                try {
                    c = resolver.query(uri, new String[] { DocumentsContract.Document.COLUMN_DISPLAY_NAME }, null, null, null);
                    if (c != null && c.moveToFirst()) {
                        name = c.getString(0);
                    }
                } catch (Throwable ignored) {
                } finally {
                    if (c != null) try { c.close(); } catch (Throwable ignored) {}
                }
                if (name == null || name.isEmpty()) {
                    name = uri.getLastPathSegment();
                }
                String cleanName = sanitizeName(name, mimeType);
                File targetFile = new File(destDir, cleanName);
                if (copyDocToFile(resolver, uri, targetFile)) {
                    importedPaths.add(targetFile.getAbsolutePath());
                }
            }
        } catch (Throwable t) {
            android.util.Log.e("MixPracticeStudio", "Error importing docs: " + t.getMessage(), t);
        }
        return importedPaths.toArray(new String[0]);
    }

    private static void scanDirectory(ContentResolver resolver, Uri treeUri, String parentDocId, File destDir, ArrayList<String> importedPaths) {
        Uri childrenUri = DocumentsContract.buildChildDocumentsUriUsingTree(treeUri, parentDocId);
        String[] projection = new String[] {
            DocumentsContract.Document.COLUMN_DOCUMENT_ID,
            DocumentsContract.Document.COLUMN_DISPLAY_NAME,
            DocumentsContract.Document.COLUMN_MIME_TYPE
        };
        Cursor cursor = null;
        try {
            cursor = resolver.query(childrenUri, projection, null, null, null);
            if (cursor != null) {
                while (cursor.moveToNext()) {
                    String docId = cursor.getString(0);
                    String name = cursor.getString(1);
                    String mimeType = cursor.getString(2);

                    if (DocumentsContract.Document.MIME_TYPE_DIR.equals(mimeType)) {
                        scanDirectory(resolver, treeUri, docId, destDir, importedPaths);
                    } else if (isSupportedAudio(name, mimeType)) {
                        String cleanName = sanitizeName(name, mimeType);
                        File targetFile = new File(destDir, cleanName);
                        Uri docUri = DocumentsContract.buildDocumentUriUsingTree(treeUri, docId);
                        if (copyDocToFile(resolver, docUri, targetFile)) {
                            importedPaths.add(targetFile.getAbsolutePath());
                        }
                    }
                }
            }
        } catch (Throwable t) {
            android.util.Log.e("MixPracticeStudio", "Error scanning dir: " + t.getMessage(), t);
        } finally {
            if (cursor != null) {
                try { cursor.close(); } catch (Throwable ignored) {}
            }
        }
    }

    private static boolean isSupportedAudio(String name, String mimeType) {
        if (name != null) {
            String lower = name.toLowerCase();
            if (lower.endsWith(".wav") || lower.endsWith(".mp3") || lower.endsWith(".flac") ||
                lower.endsWith(".aif") || lower.endsWith(".aiff") || lower.endsWith(".ogg")) {
                return true;
            }
        }
        if (mimeType != null) {
            String lowerMime = mimeType.toLowerCase();
            if (lowerMime.startsWith("audio/") || lowerMime.contains("wav") || lowerMime.contains("mp3") ||
                lowerMime.contains("mpeg") || lowerMime.contains("flac") || lowerMime.contains("ogg")) {
                return true;
            }
        }
        return false;
    }

    private static String sanitizeName(String name, String mimeType) {
        if (name == null || name.trim().isEmpty()) {
            name = "track_" + System.currentTimeMillis();
        }
        name = name.replaceAll("[/\\\\:*?\"<>|]", "_").trim();
        String lower = name.toLowerCase();
        if (!lower.endsWith(".wav") && !lower.endsWith(".mp3") && !lower.endsWith(".flac") &&
            !lower.endsWith(".aif") && !lower.endsWith(".aiff") && !lower.endsWith(".ogg")) {
            if (mimeType != null) {
                String lm = mimeType.toLowerCase();
                if (lm.contains("wav")) name += ".wav";
                else if (lm.contains("mp3") || lm.contains("mpeg")) name += ".mp3";
                else if (lm.contains("flac")) name += ".flac";
                else if (lm.contains("ogg")) name += ".ogg";
                else if (lm.contains("aiff") || lm.contains("aif")) name += ".aiff";
                else name += ".wav";
            } else {
                name += ".wav";
            }
        }
        return name;
    }

    private static boolean copyDocToFile(ContentResolver resolver, Uri docUri, File destFile) {
        InputStream in = null;
        OutputStream out = null;
        try {
            in = resolver.openInputStream(docUri);
            if (in == null) return false;
            if (destFile.exists()) {
                destFile.delete();
            }
            out = new FileOutputStream(destFile);
            byte[] buf = new byte[65536];
            int read;
            while ((read = in.read(buf)) > 0) {
                out.write(buf, 0, read);
            }
            out.flush();
            return destFile.exists() && destFile.length() > 0;
        } catch (Throwable t) {
            android.util.Log.e("MixPracticeStudio", "Error copying file: " + t.getMessage(), t);
            if (destFile.exists()) destFile.delete();
            return false;
        } finally {
            try { if (in != null) in.close(); } catch (Throwable ignored) {}
            try { if (out != null) out.close(); } catch (Throwable ignored) {}
        }
    }
}
