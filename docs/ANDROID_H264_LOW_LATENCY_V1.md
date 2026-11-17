# Nimbox Android — H.264 low-latency decoder selection (experimental)

## Scope

Only the Android MediaCodec path changes. This branch is based directly on `main` to keep the PS5/PS4/Windows/Linux renderer, WebRTC signaling, network transport, input and UI unchanged. Do not merge into a release without validating a physical Android device.

## XStreaming reference (analysis, not a code import)

The public XStreaming project (`Geocld/XStreaming`, AGPL-3.0) uses Android `MediaCodecList`, identifies hardware decoders, checks optional low-latency capability, and has separate WebRTC/FSR/FrameGen and Rust Nano pipelines. **No AGPL implementation was transplanted into Nimbox**. The change here is a small original implementation of the publicly documented Android MediaCodec API.

## Implemented

1. `MainActivity.preferredAvcDecoder()` enumerates AVC decoders; skips encoders, aliases and software-only components; ranks decoders advertising Android's low-latency feature above other hardware components. Returns an empty string when enumeration cannot identify one.
2. The Android-only C++ `HwDecoder` queries that method through the existing SDL/JNI runtime once during startup and attempts `AMediaCodec_createCodecByName`. Falls back to `AMediaCodec_createDecoderByType("video/avc")`, retaining existing format and FFmpeg fallback.
3. Runtime logs report the **actual codec component name** (API 28+) as well as whether the preferred or default decoder was requested.
4. Handles empty codec buffers, logs when a queue backlog forces a stale frame to be discarded, and checks `AMediaCodecBufferInfo.size` rather than trusting `AMediaCodec_getOutputBuffer`'s output size (not reliable through Android API 35).
5. Corrects the platform-neutral log that mislabelled Android's MediaCodec as the PS5's Videodec2 decoder.

### What this does not do

- No new codec (stream remains H.264); no new streaming engine.
- No zero-copy GPU SurfaceView pipeline, FSR replacement, new Rust client or WebRTC protocol migration.
- No guaranteed resolution of stalls: RTP loss, decoder queueing, YUV CPU copies and Vulkan upload are separate possible bottlenecks.
- No PS5 or Linux behavior change except more accurate logging in the shared decoder.

## A/B procedure (required)

1. Use **the same Android device**, Wi-Fi network, xCloud region, game, resolution, bitrate and decoder preference. Record 60 seconds on the current main branch and on this experimental branch.
2. Capture `adb logcat | grep -E '(hw_decoder_android|h264 decoder:|stream: video decoder|stream: .*decode)'`.
3. Verify selected MediaCodec component on test build; compare frame decode mean/max, queued/late video, dropped RTP, RTT, reported FPS and subjective presentation stutter.
4. Test H.264 Hardware first; if the device lacks a working hardware codec, confirm the fallback works. Test Software mode unchanged.
5. Test on Android 8 (API 26) compatibility fallback and newer Android; check both Snapdragon and non-Snapdragon devices if possible.
6. Verify video aspect/crop, sound sync, gamepad responsiveness, orientation/rotation, stream reconnect and app resume.

Acceptance: no crash, no worse dropped-frame/latency metrics and a measurable improvement (or clear evidence of parity); no automatic merge until user approves. If no improvement, revert this isolated Android branch rather than layering speculative fixes.

## Later architecture candidate

XStreaming's native render path is closer to hardware zero-copy and FSR in `RTCFsrVideoView.java` and its Nano backend. The current Nimbox Android MediaCodec explicitly operates in CPU-readable ByteBuffer mode and SDL/Vulkan uploads YUV frames. Integrating a Surface/SurfaceTexture render path would require a separate design with frame ownership, Vulkan import, crop/rotation and synchronization, and needs Android-only performance testing before adoption.
