// SPDX-License-Identifier: GPL-3.0-or-later
package com.rafaelngp.psbox.streaming;

import android.app.Activity;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import org.webrtc.EglBase;
import org.webrtc.EncodedImage;
import org.webrtc.HardwareVideoDecoderFactory;
import org.webrtc.PeerConnectionFactory;
import org.webrtc.RendererCommon;
import org.webrtc.SurfaceViewRenderer;
import org.webrtc.VideoCodecInfo;
import org.webrtc.VideoCodecStatus;
import org.webrtc.VideoDecoder;
import org.webrtc.VideoFrame;

import java.nio.ByteBuffer;
import java.util.HashMap;

/**
 * Android-only H.264 playback adapter using libwebrtc's hardware video decoder
 * and EGL/SurfaceView renderer.
 *
 * It intentionally does NOT create PeerConnection, SDP, ICE, data channels,
 * audio, authentication or input. Those already belong to Nimbox's C++
 * libdatachannel/GSSV stream. The eventual JNI bridge will pass complete
 * H.264 Annex-B access units from StreamPlayer's existing video callback.
 *
 * The renderer is an independent Android SurfaceView above SDL. It must not
 * be enabled until video/UI composition and touch overlay coexistence are
 * solved and tested on a device. The shipping decoder path is unaffected.
 */
public final class AndroidWebRtcVideoPlayer implements AutoCloseable {
    private static final String TAG = "NimboxWebRtcVideo";
    private static boolean nativeInitialized;

    private final Activity activity;
    private final Object stateLock = new Object();
    private final EglBase eglBase;
    private final HardwareVideoDecoderFactory decoderFactory;
    private volatile SurfaceViewRenderer renderer;

    private VideoDecoder decoder;
    private boolean playing;
    private boolean closed;
    private int width;
    private int height;
    private long submitted;
    private long rendered;

    public AndroidWebRtcVideoPlayer(Activity activity) {
        if (activity == null) throw new IllegalArgumentException("Activity required");
        this.activity = activity;
        // This initializes libwebrtc's native runtime, NOT a PeerConnection.
        synchronized (AndroidWebRtcVideoPlayer.class) {
            if (!nativeInitialized) {
                PeerConnectionFactory.initialize(
                        PeerConnectionFactory.InitializationOptions
                                .builder(activity.getApplicationContext())
                                .createInitializationOptions());
                nativeInitialized = true;
            }
        }
        eglBase = EglBase.create();
        decoderFactory = new HardwareVideoDecoderFactory(eglBase.getEglBaseContext());
    }

    /**
     * Prepare only video decoding. This is called from the existing Nimbox
     * video thread; no new xCloud transport or audio session is started.
     * Returns false when a libwebrtc hardware H.264 decoder is unavailable.
     */
    public boolean start(int videoWidth, int videoHeight) {
        if (videoWidth < 16 || videoHeight < 16
                || videoWidth > 4096 || videoHeight > 4096) return false;
        synchronized (stateLock) {
            if (closed || playing) return false;
            VideoCodecInfo codec = new VideoCodecInfo("H264", new HashMap<>());
            VideoDecoder candidate = decoderFactory.createDecoder(codec);
            if (candidate == null) {
                Log.w(TAG, "No libwebrtc H.264 hardware decoder available");
                return false;
            }
            VideoCodecStatus status = candidate.initDecode(
                    new VideoDecoder.Settings(
                            Math.max(1, Runtime.getRuntime().availableProcessors()),
                            videoWidth, videoHeight),
                    (frame, decodeMs, qp) -> {
                        // libwebrtc may invoke this callback on another thread.
                        SurfaceViewRenderer sink = renderer;
                        if (sink != null) {
                            sink.onFrame(frame);
                            synchronized (stateLock) {
                                ++rendered;
                            }
                        }
                    });
            if (status != VideoCodecStatus.OK) {
                Log.w(TAG, "WebRTC H.264 decoder init failed: " + status);
                candidate.release();
                return false;
            }
            decoder = candidate;
            width = videoWidth;
            height = videoHeight;
            submitted = 0;
            rendered = 0;
            playing = true;
            Log.i(TAG, "Video-only libwebrtc H.264 decoder: "
                    + candidate.getImplementationName() + " " + width + "x" + height);
            return true;
        }
    }

    /**
     * The existing Nimbox RTP depacketizer supplies one complete Annex-B
     * access unit (including SPS/PPS before IDR). It is NOT raw RTP.
     *
     * The initial JNI transport will copy only the *compressed* bytes to a
     * Java byte[]; texture-backed decoded output stays inside libwebrtc's
     * EGL renderer without YUV CPU roundtrip.
     */
    public boolean submitH264(byte[] annexB, long captureTimeNs, boolean keyFrame) {
        if (annexB == null || annexB.length == 0) return false;
        synchronized (stateLock) {
            if (closed || !playing || decoder == null) return false;
            EncodedImage encoded = EncodedImage.builder()
                    .setBuffer(ByteBuffer.wrap(annexB), null)
                    .setEncodedWidth(width)
                    .setEncodedHeight(height)
                    .setCaptureTimeNs(captureTimeNs > 0 ? captureTimeNs : System.nanoTime())
                    .setFrameType(keyFrame
                            ? EncodedImage.FrameType.VideoFrameKey
                            : EncodedImage.FrameType.VideoFrameDelta)
                    .setRotation(0)
                    .createEncodedImage();
            try {
                VideoCodecStatus status = decoder.decode(
                        encoded, new VideoDecoder.DecodeInfo(false, 0));
                if (status != VideoCodecStatus.OK && status != VideoCodecStatus.NO_OUTPUT) {
                    Log.w(TAG, "WebRTC video decode result: " + status);
                    return false;
                }
                ++submitted;
                return true;
            } finally {
                encoded.release();
            }
        }
    }

    /**
     * Optional Android surface attachment. Must be activated only after the
     * SDL overlay/menu composition problem is solved; otherwise the video
     * SurfaceView would cover Nimbox's UI and touch controls.
     *
     * Not invoked by MainActivity or StreamPlayer in this experimental PR.
     */
    public void attachVideoSurface() {
        activity.runOnUiThread(() -> {
            synchronized (stateLock) {
                if (closed || renderer != null) return;
                ViewGroup root = activity.findViewById(android.R.id.content);
                if (root == null) {
                    Log.w(TAG, "Cannot find Android content root for video playback");
                    return;
                }
                SurfaceViewRenderer view = new SurfaceViewRenderer(activity);
                view.init(eglBase.getEglBaseContext(), null);
                view.setScalingType(RendererCommon.ScalingType.SCALE_ASPECT_FIT);
                view.setMirror(false);
                view.setEnableHardwareScaler(true);
                view.setZOrderMediaOverlay(true);
                view.setClickable(false);
                view.setFocusable(false);
                view.setVisibility(View.VISIBLE);
                root.addView(view, new FrameLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.MATCH_PARENT));
                renderer = view;
            }
        });
    }

    public long submittedFrames() {
        synchronized (stateLock) { return submitted; }
    }

    public long renderedFrames() {
        synchronized (stateLock) { return rendered; }
    }

    /** Stop the codec; no network, input, audio or account state is modified. */
    public void stop() {
        synchronized (stateLock) {
            playing = false;
            if (decoder != null) {
                decoder.release();
                decoder = null;
            }
        }
        activity.runOnUiThread(() -> {
            SurfaceViewRenderer old = renderer;
            renderer = null;
            if (old != null) {
                ViewGroup parent = (ViewGroup) old.getParent();
                if (parent != null) parent.removeView(old);
                old.release();
            }
        });
    }

    @Override public void close() {
        synchronized (stateLock) {
            if (closed) return;
            closed = true;
        }
        stop();
        // Renderer.release() is posted to the UI thread. eglBase.release()
        // must happen after the view's renderer is finished with its context.
        activity.runOnUiThread(eglBase::release);
    }
}
