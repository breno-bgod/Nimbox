// SPDX-License-Identifier: GPL-3.0-or-later
package com.rafaelngp.psbox.streaming;

import android.app.Activity;
import android.util.Log;

import org.webrtc.EglBase;
import org.webrtc.EncodedImage;
import org.webrtc.HardwareVideoDecoderFactory;
import org.webrtc.PeerConnectionFactory;
import org.webrtc.VideoCodecInfo;
import org.webrtc.VideoCodecStatus;
import org.webrtc.VideoDecoder;
import org.webrtc.VideoFrame;

import java.nio.ByteBuffer;
import java.util.ArrayDeque;

/**
 * Video-only adapter: takes complete H.264 access units from Nimbox's
 * EXISTING GSSV/libdatachannel session, decodes with libwebrtc's Android
 * hardware decoder, and returns YUV420 frames to Nimbox's existing renderer.
 *
 * No PeerConnection, authentication, audio, gamepad or extra network session.
 * The initial bridge copies decoded YUV once. Zero-copy EGL/UI compositing
 * is a separate optimization, not required to make the backend playable.
 */
public final class AndroidWebRtcVideoPlayer implements AutoCloseable {
    private static final String TAG = "NimboxWebRtcVideo";
    private static boolean nativeInitialized;
    private static final int MAX_QUEUED_FRAMES = 3;

    private final Object lock = new Object();
    private final ArrayDeque<byte[]> frames = new ArrayDeque<>();
    private final EglBase eglBase;
    private final HardwareVideoDecoderFactory factory;
    private VideoDecoder decoder;
    private boolean active;
    private long submitted;
    private long decoded;

    public AndroidWebRtcVideoPlayer(Activity activity) {
        synchronized (AndroidWebRtcVideoPlayer.class) {
            if (!nativeInitialized) {
                PeerConnectionFactory.initialize(
                    PeerConnectionFactory.InitializationOptions
                        .builder(activity.getApplicationContext()).createInitializationOptions());
                nativeInitialized = true;
            }
        }
        eglBase = EglBase.create();
        factory = new HardwareVideoDecoderFactory(eglBase.getEglBaseContext());
    }

    public boolean start(int width, int height) {
        if (width < 16 || height < 16 || width > 4096 || height > 4096) return false;
        synchronized (lock) {
            if (active) return false;
            VideoDecoder candidate = null;
            for (VideoCodecInfo info : factory.getSupportedCodecs()) {
                if ("H264".equalsIgnoreCase(info.name)) {
                    candidate = factory.createDecoder(info);
                    if (candidate != null) break;
                }
            }
            if (candidate == null) {
                Log.w(TAG, "WebRTC Android hardware H264 decoder unavailable");
                return false;
            }
            VideoCodecStatus code = candidate.initDecode(
                new VideoDecoder.Settings(1, width, height),
                (frame, decodeTimeMs, qp) -> enqueue(frame));
            if (code != VideoCodecStatus.OK) {
                Log.w(TAG, "H264 decoder initialization failed: " + code);
                candidate.release();
                return false;
            }
            decoder = candidate;
            active = true;
            submitted = decoded = 0;
            frames.clear();
            Log.i(TAG, "Nimbox WebRTC video decoder active: " + candidate.getImplementationName());
            return true;
        }
    }

    private static void putIntLE(byte[] data, int offset, int value) {
        data[offset] = (byte) value;
        data[offset + 1] = (byte) (value >>> 8);
        data[offset + 2] = (byte) (value >>> 16);
        data[offset + 3] = (byte) (value >>> 24);
    }

    private static void copyPlane(ByteBuffer src, int stride, byte[] dst,
                                  int offset, int width, int rows) {
        ByteBuffer data = src.duplicate();
        for (int y = 0; y < rows; ++y) {
            data.position(y * stride);
            data.get(dst, offset + y * width, width);
        }
    }

    private void enqueue(VideoFrame frame) {
        final VideoFrame.I420Buffer i420 = frame.getBuffer().toI420();
        try {
            final int width = i420.getWidth(), height = i420.getHeight();
            if (width < 16 || height < 16 || width > 4096 || height > 4096) return;
            final int cw = (width + 1) / 2, ch = (height + 1) / 2;
            final long payload = (long) width * height + 2L * cw * ch;
            if (payload > 32L * 1024 * 1024) return;
            // 16-byte LE header: RTP tag (low 32 bits), width, height, 0.
            final byte[] packed = new byte[16 + (int) payload];
            // Hardware codecs may quantize timestamps to microseconds.
            // Encode the original 32-bit RTP tag as whole microseconds, then
            // reconstruct it exactly when the YUV frame returns to C++.
            putIntLE(packed, 0, (int) (frame.getTimestampNs() / 1000L));
            putIntLE(packed, 4, width);
            putIntLE(packed, 8, height);
            putIntLE(packed, 12, 0);
            int uvStart = 16 + width * height;
            copyPlane(i420.getDataY(), i420.getStrideY(), packed, 16, width, height);
            copyPlane(i420.getDataU(), i420.getStrideU(), packed, uvStart, cw, ch);
            copyPlane(i420.getDataV(), i420.getStrideV(), packed, uvStart + cw * ch, cw, ch);
            synchronized (lock) {
                if (!active) return;
                while (frames.size() >= MAX_QUEUED_FRAMES) frames.removeFirst();
                frames.addLast(packed);
                ++decoded;
            }
        } finally {
            i420.release();
        }
    }

    /**
     * Feed one Annex-B access unit (NOT raw RTP) and return the newest
     * completed I420 frame, possibly from an earlier packet.
     * null = codec still decoding. Empty byte[] = decode failure.
     */
    public byte[] decode(byte[] annexB, long rtpTag, boolean keyFrame) {
        if (annexB == null || annexB.length == 0) return new byte[0];
        VideoDecoder current;
        synchronized (lock) {
            if (!active || decoder == null) return new byte[0];
            current = decoder;
        }
        ByteBuffer encodedData = ByteBuffer.allocateDirect(annexB.length);
        encodedData.put(annexB);
        encodedData.flip();
        EncodedImage encoded = EncodedImage.builder()
            .setBuffer(encodedData, null)
            .setEncodedWidth(1920).setEncodedHeight(1088)
            .setCaptureTimeNs((rtpTag & 0xffffffffL) * 1000L)
            .setFrameType(keyFrame ? EncodedImage.FrameType.VideoFrameKey
                                  : EncodedImage.FrameType.VideoFrameDelta)
            .setRotation(0).createEncodedImage();
        try {
            VideoCodecStatus status = current.decode(
                encoded, new VideoDecoder.DecodeInfo(false, 0));
            if (status != VideoCodecStatus.OK && status != VideoCodecStatus.NO_OUTPUT) {
                Log.w(TAG, "H264 decoder status " + status);
                return new byte[0];
            }
            ++submitted;
        } finally {
            encoded.release();
        }
        synchronized (lock) {
            if (frames.isEmpty()) return null;
            // Never show a stale frame when the native presentation loop lags.
            while (frames.size() > 1) frames.removeFirst();
            return frames.removeFirst();
        }
    }

    public void stop() {
        VideoDecoder old;
        synchronized (lock) {
            active = false;
            old = decoder;
            decoder = null;
            frames.clear();
        }
        if (old != null) old.release();
        Log.i(TAG, "Nimbox WebRTC video player stopped; packets=" + submitted + " frames=" + decoded);
    }

    @Override public void close() {
        stop();
        eglBase.release();
    }
}
