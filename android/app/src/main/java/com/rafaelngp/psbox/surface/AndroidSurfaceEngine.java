// SPDX-License-Identifier: AGPL-3.0-only
// Android Surface MediaCodec playback adapter for Nimbox.
// Adapted from Geocld/XStreaming NanoRustBridge.java, especially queueVideoFrame,
// ensureVideoDecoderLocked, drainVideoDecoderOutput, and applyLowLatencyVideoFormat.
// Copyright of original code remains with its respective contributors.
// https://github.com/Geocld/XStreaming
package com.rafaelngp.psbox.surface;

import android.media.MediaCodec;
import android.media.MediaFormat;
import android.os.Build;
import android.os.Process;
import android.util.Log;
import android.view.Surface;
import java.nio.ByteBuffer;
import java.util.Locale;

public final class AndroidSurfaceEngine {
    private static final String TAG = "NimboxSurface";
    private volatile MediaCodec videoDecoder;
    private volatile boolean videoOutputDrainRunning;
    private Thread videoOutputThread;
    private long firstVideoPtsUs = Long.MIN_VALUE;
    private long lastVideoPtsUs;
    private long queued;
    private volatile long rendered;

    public synchronized boolean start(Surface surface, int width, int height) {
        if (surface == null || !surface.isValid() || videoDecoder != null) return false;
        MediaCodec codec = null;
        try {
            final String mime = "video/avc";
            final String name = NanoMediaCodecHelper.getPreferredDecoderName(mime);
            codec = name == null ? MediaCodec.createDecoderByType(mime) : MediaCodec.createByCodecName(name);
            MediaFormat format = MediaFormat.createVideoFormat(mime, width, height);
            format.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, Math.max(4 * 1024 * 1024, width * height));
            applyLowLatencyVideoFormat(format, name, mime);
            codec.configure(format, surface, null, 0);
            codec.start();
            videoDecoder = codec;
            videoOutputDrainRunning = true;
            videoOutputThread = new Thread(this::drainVideoDecoderOutput, "NimboxSurfaceOutput");
            videoOutputThread.start();
            Log.i(TAG, "Surface decoder started " + name + " " + width + "x" + height);
            return true;
        } catch (Exception error) {
            Log.e(TAG, "Surface decoder start failed", error);
            if (codec != null) {
                try { codec.release(); } catch (Exception ignored) {}
            }
            videoDecoder = null;
            return false;
        }
    }

    public synchronized boolean queueVideoFrame(ByteBuffer data, long ptsUs, boolean keyframe) {
        MediaCodec codec = videoDecoder;
        if (codec == null || !videoOutputDrainRunning || data == null) return false;
        try {
            // As in XStreaming: feed the compressed access unit, never decoded CPU YUV.
            int index = codec.dequeueInputBuffer(0L);
            if (index < 0) return true; // nonblocking: skip this AU to preserve low latency
            ByteBuffer target = codec.getInputBuffer(index);
            if (target == null) return false;
            ByteBuffer src = data.duplicate();
            src.position(0);
            target.clear();
            if (src.remaining() > target.remaining()) {
                codec.queueInputBuffer(index, 0, 0, Math.max(0L, ptsUs), 0);
                Log.w(TAG, "Access unit larger than input buffer: " + src.remaining());
                return false;
            }
            int len = src.remaining();
            target.put(src);
            if (firstVideoPtsUs == Long.MIN_VALUE) firstVideoPtsUs = ptsUs;
            long pts = Math.max(0L, ptsUs - firstVideoPtsUs);
            if (pts <= lastVideoPtsUs) pts = lastVideoPtsUs + 1;
            lastVideoPtsUs = pts;
            codec.queueInputBuffer(index, 0, len, pts, keyframe ? MediaCodec.BUFFER_FLAG_KEY_FRAME : 0);
            queued++;
            return true;
        } catch (Exception error) {
            Log.e(TAG, "Surface input failed", error);
            return false;
        }
    }

    private void drainVideoDecoderOutput() {
        Process.setThreadPriority(Process.THREAD_PRIORITY_DISPLAY);
        MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
        while (videoOutputDrainRunning) {
            MediaCodec codec = videoDecoder;
            if (codec == null) break;
            try {
                int index = codec.dequeueOutputBuffer(info, 5000L);
                if (index >= 0) {
                    // Release directly to Android Surface (no YUV download or SDL blit).
                    codec.releaseOutputBuffer(index, true);
                    rendered++;
                } else if (index == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                    Log.i(TAG, "Surface output format: " + codec.getOutputFormat());
                }
            } catch (Exception error) {
                Log.e(TAG, "Surface output failed", error);
                videoOutputDrainRunning = false;
                break;
            }
        }
    }

    public void stop() {
        videoOutputDrainRunning = false;
        Thread thread;
        MediaCodec codec;
        synchronized (this) {
            thread = videoOutputThread;
            videoOutputThread = null;
            codec = videoDecoder;
            videoDecoder = null;
        }
        if (thread != null && thread != Thread.currentThread()) {
            try { thread.join(250L); } catch (InterruptedException ex) { Thread.currentThread().interrupt(); }
        }
        if (codec != null) {
            try { codec.stop(); } catch (Exception ignored) {}
            try { codec.release(); } catch (Exception ignored) {}
        }
        Log.i(TAG, "Surface playback stopped: queued=" + queued + " released=" + rendered);
    }

    private static void applyLowLatencyVideoFormat(MediaFormat format, String codecName, String mime) {
        // Direct adaptation of XStreaming's low-latency MediaFormat policy.
        format.setInteger(MediaFormat.KEY_OPERATING_RATE, 32767);
        format.setInteger("priority", 0);
        format.setInteger("allow-frame-drop", 1);
        format.setInteger("low-latency", 1);
        format.setInteger("vdec-lowlatency", 1);
        format.setInteger("vendor.low-latency.enable", 1);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R &&
                NanoMediaCodecHelper.decoderSupportsLowLatency(codecName, mime)) {
            format.setInteger(MediaFormat.KEY_LOW_LATENCY, 1);
        }
        String name = codecName == null ? "" : codecName.toLowerCase(Locale.US);
        if (name.contains("qcom") || name.contains("qti")) {
            format.setInteger("vendor.qti-ext-dec-picture-order.enable", 1);
            format.setInteger("vendor.qti-ext-dec-low-latency.enable", 1);
            format.setInteger("vendor.rtc-ext-dec-low-latency.enable", 1);
        }
    }
}
