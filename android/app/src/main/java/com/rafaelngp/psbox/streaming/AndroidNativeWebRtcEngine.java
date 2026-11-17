// SPDX-License-Identifier: GPL-3.0-or-later
package com.rafaelngp.psbox.streaming;

import android.content.Context;
import android.util.Log;

import org.webrtc.DataChannel;
import org.webrtc.EglBase;
import org.webrtc.IceCandidate;
import org.webrtc.MediaStream;
import org.webrtc.MediaStreamTrack;
import org.webrtc.PeerConnection;
import org.webrtc.PeerConnectionFactory;
import org.webrtc.RtpReceiver;
import org.webrtc.RtpTransceiver;
import org.webrtc.SdpObserver;
import org.webrtc.SessionDescription;
import org.webrtc.VideoFrame;
import org.webrtc.VideoSink;
import org.webrtc.VideoTrack;
import org.webrtc.audio.JavaAudioDeviceModule;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

/**
 * Independent Android libwebrtc session foundation for Nimbox.
 *
 * IMPORTANT: this is NOT connected to GSSV or the Nimbox playback screen yet.
 * It is not a selectable or operational xCloud streaming engine until the
 * platform bridge, xCloud handshakes, media rendering and lifecycle land.
 *
 * The shipping C++ libdatachannel + MediaCodec/FFmpeg path is unchanged.
 * This class deliberately uses only the standard WebRTC Android API, not
 * any XStreaming AGPL source code.
 */
public final class AndroidNativeWebRtcEngine implements AutoCloseable {
    private static final String TAG = "NimboxNativeRTC";
    private static boolean sInitialized = false;

    public interface Events {
        void onLocalOffer(String sdp);
        void onLocalIce(String sdpMid, int mLineIndex, String candidate);
        void onConnectionState(String state);
        void onDataMessage(String channel, byte[] data, boolean binary);
        void onVideoFrame(VideoFrame frame);
        void onError(String message);
    }

    private final Events events;
    private final EglBase eglBase;
    private final JavaAudioDeviceModule audioModule;
    private final PeerConnectionFactory factory;
    private PeerConnection peer;
    private VideoTrack videoTrack;
    private final List<DataChannel> channels = new ArrayList<>();
    private boolean closed;

    public AndroidNativeWebRtcEngine(Context context, Events events) {
        if (events == null) throw new IllegalArgumentException("Events required");
        this.events = events;
        synchronized (AndroidNativeWebRtcEngine.class) {
            if (!sInitialized) {
                PeerConnectionFactory.initialize(
                        PeerConnectionFactory.InitializationOptions
                                .builder(context.getApplicationContext())
                                .createInitializationOptions());
                sInitialized = true;
            }
        }
        eglBase = EglBase.create();
        audioModule = JavaAudioDeviceModule.builder(context.getApplicationContext())
                .createAudioDeviceModule();
        factory = PeerConnectionFactory.builder()
                .setAudioDeviceModule(audioModule)
                .setVideoDecoderFactory(new org.webrtc.DefaultVideoDecoderFactory(eglBase.getEglBaseContext()))
                .createPeerConnectionFactory();
        Log.i(TAG, "Independent Android libwebrtc factory initialized; no xCloud session started");
    }

    public synchronized boolean prepare(List<PeerConnection.IceServer> iceServers) {
        if (closed || peer != null) return false;
        PeerConnection.RTCConfiguration config = new PeerConnection.RTCConfiguration(
                iceServers == null ? new ArrayList<>() : iceServers);
        config.sdpSemantics = PeerConnection.SdpSemantics.UNIFIED_PLAN;
        peer = factory.createPeerConnection(config, new PeerConnection.Observer() {
            @Override public void onSignalingChange(PeerConnection.SignalingState state) {}
            @Override public void onIceConnectionChange(PeerConnection.IceConnectionState state) {
                events.onConnectionState("ICE: " + state.name());
            }
            @Override public void onIceConnectionReceivingChange(boolean receiving) {}
            @Override public void onIceGatheringChange(PeerConnection.IceGatheringState state) {}
            @Override public void onIceCandidate(IceCandidate candidate) {
                events.onLocalIce(candidate.sdpMid, candidate.sdpMLineIndex, candidate.sdp);
            }
            @Override public void onIceCandidatesRemoved(IceCandidate[] candidates) {}
            @Override public void onAddStream(MediaStream stream) {}
            @Override public void onRemoveStream(MediaStream stream) {}
            @Override public void onDataChannel(DataChannel channel) { observe(channel); }
            @Override public void onRenegotiationNeeded() {}
            @Override public void onAddTrack(RtpReceiver receiver, MediaStream[] streams) {
                if (receiver.track() instanceof VideoTrack) attach((VideoTrack) receiver.track());
            }
            @Override public void onConnectionChange(PeerConnection.PeerConnectionState state) {
                events.onConnectionState(state.name());
            }
            @Override public void onTrack(RtpTransceiver transceiver) {
                if (transceiver != null && transceiver.getReceiver().track() instanceof VideoTrack) {
                    attach((VideoTrack) transceiver.getReceiver().track());
                }
            }
        });
        if (peer == null) {
            events.onError("Unable to allocate independent Android WebRTC PeerConnection");
            return false;
        }

        // xCloud's existing client expects the same ordered channel labels.
        // Message/control handshakes are implemented in the C++ session and
        // must be bridged before this engine can be selected in gameplay.
        createChannel("chat", "chatV1", true, -1);
        createChannel("control", "controlV1", true, -1);
        createChannel("input", "1.0", false, 0);
        createChannel("message", "messageV1", true, -1);

        peer.addTransceiver(MediaStreamTrack.MediaType.MEDIA_TYPE_AUDIO,
                new RtpTransceiver.RtpTransceiverInit(RtpTransceiver.RtpTransceiverDirection.RECV_ONLY));
        peer.addTransceiver(MediaStreamTrack.MediaType.MEDIA_TYPE_VIDEO,
                new RtpTransceiver.RtpTransceiverInit(RtpTransceiver.RtpTransceiverDirection.RECV_ONLY));
        return true;
    }

    private void createChannel(String label, String protocol, boolean ordered, int maxRetransmits) {
        if (peer == null) return;
        DataChannel.Init init = new DataChannel.Init();
        init.protocol = protocol;
        init.ordered = ordered;
        if (maxRetransmits >= 0) init.maxRetransmits = maxRetransmits;
        DataChannel channel = peer.createDataChannel(label, init);
        if (channel != null) observe(channel);
    }

    private void observe(DataChannel channel) {
        channels.add(channel);
        channel.registerObserver(new DataChannel.Observer() {
            @Override public void onBufferedAmountChange(long amount) {}
            @Override public void onStateChange() {
                Log.i(TAG, channel.label() + " state: " + channel.state());
            }
            @Override public void onMessage(DataChannel.Buffer buffer) {
                ByteBuffer source = buffer.data.duplicate();
                byte[] bytes = new byte[source.remaining()];
                source.get(bytes);
                events.onDataMessage(channel.label(), bytes, buffer.binary);
            }
        });
    }

    private final VideoSink frameSink = frame -> {
        // Retaining a frame or presenting it is the Android renderer's job.
        // The callback is synchronous; consumer must retain/release correctly.
        events.onVideoFrame(frame);
    };

    private void attach(VideoTrack track) {
        if (videoTrack == track) return;
        if (videoTrack != null) videoTrack.removeSink(frameSink);
        videoTrack = track;
        track.addSink(frameSink);
    }

    public synchronized void createLocalOffer() {
        if (closed || peer == null) { events.onError("Peer not prepared"); return; }
        peer.createOffer(new SdpObserver() {
            @Override public void onCreateSuccess(SessionDescription offer) {
                peer.setLocalDescription(new SdpObserver() {
                    @Override public void onCreateSuccess(SessionDescription d) {}
                    @Override public void onSetSuccess() { events.onLocalOffer(offer.description); }
                    @Override public void onCreateFailure(String e) { events.onError(e); }
                    @Override public void onSetFailure(String e) { events.onError(e); }
                }, offer);
            }
            @Override public void onSetSuccess() {}
            @Override public void onCreateFailure(String e) { events.onError(e); }
            @Override public void onSetFailure(String e) { events.onError(e); }
        }, new org.webrtc.MediaConstraints());
    }

    public synchronized void setRemoteAnswer(String sdp) {
        if (closed || peer == null || sdp == null || sdp.isEmpty()) return;
        peer.setRemoteDescription(new SdpObserver() {
            @Override public void onCreateSuccess(SessionDescription d) {}
            @Override public void onSetSuccess() { Log.i(TAG, "Remote SDP answer applied"); }
            @Override public void onCreateFailure(String e) { events.onError(e); }
            @Override public void onSetFailure(String e) { events.onError(e); }
        }, new SessionDescription(SessionDescription.Type.ANSWER, sdp));
    }

    public synchronized boolean addRemoteIce(String mid, int mLine, String candidate) {
        if (closed || peer == null || candidate == null || candidate.isEmpty()) return false;
        return peer.addIceCandidate(new IceCandidate(mid, mLine, candidate));
    }

    public synchronized boolean send(String label, byte[] payload, boolean binary) {
        if (closed || payload == null) return false;
        for (DataChannel channel : channels) {
            if (channel.label().equals(label) && channel.state() == DataChannel.State.OPEN) {
                return channel.send(new DataChannel.Buffer(ByteBuffer.wrap(payload), binary));
            }
        }
        return false;
    }

    @Override public synchronized void close() {
        if (closed) return;
        closed = true;
        if (videoTrack != null) {
            videoTrack.removeSink(frameSink);
            videoTrack = null;
        }
        for (DataChannel channel : channels) {
            channel.close();
            channel.dispose();
        }
        channels.clear();
        if (peer != null) {
            peer.close();
            peer.dispose();
            peer = null;
        }
        factory.dispose();
        audioModule.release();
        eglBase.release();
    }
}
