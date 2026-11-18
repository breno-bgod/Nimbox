// SPDX-License-Identifier: AGPL-3.0-only
// Adapted from Geocld/XStreaming NanoRustBridge.java (MediaCodec/Surface output,
// decoder selection and low latency video presentation). See third_party/XStreaming.
// Copyright remains with original XStreaming contributors.
package com.rafaelngp.psbox.surface;

import android.media.MediaCodec;
import android.media.MediaFormat;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.Process;
import android.util.Log;
import android.view.Surface;
import java.nio.ByteBuffer;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;

public final class AndroidSurfaceEngine {
    private static final String TAG = "NimboxSurface";
    // 1 = accepted, 0 = input backpressure (caller must recover), -1 = decoder failure.
    public static final int QUEUED = 1;
    public static final int BUSY = 0;
    public static final int ERROR = -1;
    private static final long INPUT_WAIT_US = 1500;
    private static final long START_DELAY_NS = 8_000_000L;
    private static final long MAX_FUTURE_NS = 35_000_000L;
    private static final long LATE_THRESHOLD_NS = 12_000_000L;

    private static final class Session {
        final MediaCodec codec;
        final AtomicBoolean running = new AtomicBoolean(true);
        final AtomicLong submitted = new AtomicLong();
        final AtomicLong released = new AtomicLong();
        final AtomicLong rendered = new AtomicLong();
        final AtomicLong busy = new AtomicLong();
        final AtomicLong errors = new AtomicLong();
        Thread outputThread;
        long firstPtsUs = Long.MIN_VALUE;
        long lastPtsUs = -1;
        long firstRenderPtsUs = Long.MIN_VALUE;
        long firstRenderNs = Long.MIN_VALUE;

        Session(MediaCodec decoder) { codec = decoder; }
    }

    private volatile Session session;

    public synchronized boolean start(Surface surface, int width, int height) {
        if (session != null || surface == null || !surface.isValid()
                || width <= 0 || height <= 0) return false;
        MediaCodec codec = null;
        try {
            final String mime = "video/avc";
            String name = NanoMediaCodecHelper.getPreferredDecoderName(mime);
            codec = name == null ? MediaCodec.createDecoderByType(mime)
                    : MediaCodec.createByCodecName(name);
            MediaFormat format = MediaFormat.createVideoFormat(mime, width, height);
            format.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE,
                    Math.max(4 * 1024 * 1024, width * height));
            applyLowLatencyVideoFormat(format, name, mime);
            codec.configure(format, surface, null, 0);
            Session s = new Session(codec);
            codec.setOnFrameRenderedListener((ignored, ptsUs, systemNano) -> {
                if (s.running.get()) s.rendered.incrementAndGet();
            }, new Handler(Looper.getMainLooper()));
            codec.start();
            session = s;
            s.outputThread = new Thread(() -> drainVideoDecoderOutput(s), "NimboxSurfaceOutput");
            s.outputThread.start();
            Log.i(TAG, "Surface initialized: " + name + " " + width + "x" + height);
            return true;
        } catch (Exception ex) {
            Log.e(TAG, "Surface initialization failed", ex);
            if (codec != null) {
                try { codec.stop(); } catch (Exception ignored) { }
                try { codec.release(); } catch (Exception ignored) { }
            }
            return false;
        }
    }

    // Called synchronously from JNI; the direct ByteBuffer points to a native AU.
    // A busy MediaCodec is NOT a successful decode: caller retries, then requests IDR.
    public synchronized int queueVideoFrame(ByteBuffer data, long ptsUs, boolean keyframe) {
        Session s = session;
        if (s == null || !s.running.get() || data == null) return ERROR;
        try {
            int index = s.codec.dequeueInputBuffer(INPUT_WAIT_US);
            if (index < 0) {
                s.busy.incrementAndGet();
                return BUSY;
            }
            ByteBuffer dst = s.codec.getInputBuffer(index);
            if (dst == null) {
                s.errors.incrementAndGet();
                return ERROR;
            }
            ByteBuffer src = data.duplicate();
            src.position(0);
            dst.clear();
            if (src.remaining() > dst.remaining()) {
                s.codec.queueInputBuffer(index, 0, 0, Math.max(0L, ptsUs), 0);
                s.errors.incrementAndGet();
                Log.w(TAG, "Compressed AU exceeds codec input buffer");
                return ERROR;
            }
            int size = src.remaining();
            dst.put(src);
            if (s.firstPtsUs == Long.MIN_VALUE) s.firstPtsUs = ptsUs;
            long normalized = Math.max(0L, ptsUs - s.firstPtsUs);
            if (normalized <= s.lastPtsUs) normalized = s.lastPtsUs + 1L;
            s.lastPtsUs = normalized;
            s.codec.queueInputBuffer(index, 0, size, normalized,
                    keyframe ? MediaCodec.BUFFER_FLAG_KEY_FRAME : 0);
            s.submitted.incrementAndGet();
            return QUEUED;
        } catch (Exception ex) {
            s.errors.incrementAndGet();
            s.running.set(false);
            Log.e(TAG, "Surface input failure", ex);
            return ERROR;
        }
    }

    private void drainVideoDecoderOutput(Session s) {
        Process.setThreadPriority(Process.THREAD_PRIORITY_DISPLAY);
        MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
        while (s.running.get()) {
            try {
                int index = s.codec.dequeueOutputBuffer(info, 5000);
                if (index >= 0) {
                    long renderNs = getRenderTimeNs(s, info.presentationTimeUs);
                    if (renderNs > 0) s.codec.releaseOutputBuffer(index, renderNs);
                    else s.codec.releaseOutputBuffer(index, true);
                    s.released.incrementAndGet();
                } else if (index == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                    Log.i(TAG, "Decoder format: " + s.codec.getOutputFormat());
                }
            } catch (Exception ex) {
                if (s.running.get()) {
                    s.errors.incrementAndGet();
                    Log.e(TAG, "Surface output failure", ex);
                }
                s.running.set(false);
                break;
            }
        }
    }

    // XStreaming's bounded presentation policy prevents extreme future/late scheduling.
    private static long getRenderTimeNs(Session s, long ptsUs) {
        long now = System.nanoTime();
        if (s.firstRenderPtsUs == Long.MIN_VALUE) {
            s.firstRenderPtsUs = ptsUs;
            s.firstRenderNs = now + START_DELAY_NS;
            return s.firstRenderNs;
        }
        long target = s.firstRenderNs + Math.max(0L, ptsUs - s.firstRenderPtsUs) * 1000L;
        if (target - now > MAX_FUTURE_NS) {
            s.firstRenderPtsUs = ptsUs;
            s.firstRenderNs = now + START_DELAY_NS;
            return s.firstRenderNs;
        }
        if (now - target > LATE_THRESHOLD_NS) {
            s.firstRenderPtsUs = ptsUs;
            s.firstRenderNs = now;
            return 0L;
        }
        return target;
    }

    // Actual render counts come from MediaCodec.OnFrameRenderedListener, not input queue.
    public long[] stats() {
        Session s = session;
        if (s == null) return new long[] {0, 0, 0, 0, 0};
        return new long[] {s.submitted.get(), s.released.get(), s.rendered.get(),
                s.busy.get(), s.errors.get()};
    }

    public void stop() {
        Session s;
        synchronized (this) {
            s = session;
            session = null;
        }
        if (s == null) return;
        s.running.set(false);
        if (s.outputThread != null && Thread.currentThread() != s.outputThread) {
            try {
                s.outputThread.join(500);
            } catch (InterruptedException ex) {
                Thread.currentThread().interrupt();
            }
        }
        try { s.codec.stop(); } catch (Exception ignored) { }
        try { s.codec.release(); } catch (Exception ignored) { }
        Log.i(TAG, "Surface released submitted=" + s.submitted.get() +
                " queuedForDisplay=" + s.released.get() +
                " onScreen=" + s.rendered.get() + " busy=" + s.busy.get());
    }

    private static void applyLowLatencyVideoFormat(MediaFormat format, String codecName, String mime) {
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
