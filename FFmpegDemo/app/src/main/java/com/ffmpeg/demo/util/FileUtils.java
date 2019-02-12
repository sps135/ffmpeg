package com.ffmpeg.demo.util;


import android.content.Context;
import android.content.res.AssetManager;
import android.os.Environment;
import android.text.TextUtils;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Utilities for dealing with assets.
 */
public class FileUtils {

    private static final String TAG = FileUtils.class.getSimpleName();

    private static final int BYTE_BUF_SIZE = 2048;

    private static final String PATH = Environment.getExternalStorageDirectory().getPath();

    public static final String TARGET_NAME = "hello.mp4";

    public static final String PART_NAME = "part.mp4";

    public static final String SCREENSHOT_NAME = "shot.png";

    public static final String TRANSFORM_NAME = "transformVideo.flv";

    /**
     * Copies a file from assets.
     *
     * @param context    application context used to discover assets.
     * @param assetName  the relative file name within assets.
     * @param targetName the target file name, always over write the existing file.
     * @throws IOException if operation fails.
     */
    public static void copy(Context context, String assetName, String targetName) throws IOException {

        Log.d(TAG, "creating file " + targetName + " from " + assetName);

        File targetFile = null;
        InputStream inputStream = null;
        FileOutputStream outputStream = null;

        try {
            AssetManager assets = context.getAssets();
            targetFile = new File(String.format("%s%s%s", PATH, File.separator, targetName));
            if (!targetFile.getParentFile().exists()) {
                targetFile.getParentFile().mkdirs();
            }
            if (!targetFile.exists()) {
                targetFile.createNewFile();
            }
            inputStream = assets.open(assetName);
            outputStream = new FileOutputStream(targetFile, false /* append */);
            copy(inputStream, outputStream);
        } finally {
            if (outputStream != null) {
                outputStream.close();
            }
            if (inputStream != null) {
                inputStream.close();
            }
        }
    }

    private static void copy(InputStream from, OutputStream to) throws IOException {
        byte[] buf = new byte[BYTE_BUF_SIZE];
        while (true) {
            int r = from.read(buf);
            if (r == -1) {
                break;
            }
            to.write(buf, 0, r);
        }
    }

    public static boolean checkFileExist(String path) {
        if (TextUtils.isEmpty(path)) {
            return false;
        }
        File file = new File(path);
        if (!file.exists()) {
            return false;
        }
        return true;
    }

    public static boolean checkTargetFile(String fileName) {
        return checkFileExist(String.format("%s%s%s", PATH, File.separator, fileName));
    }

    public static String filePath(String fileName) {
        return String.format("%s%s%s", PATH, File.separator, fileName);
    }
}
