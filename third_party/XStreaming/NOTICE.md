# XStreaming Android Surface playback — AGPL-3.0

Source: https://github.com/Geocld/XStreaming (branch main, accessed 2026-11-17).
Original files:
- android/app/src/main/java/com/xstreaming/nano/NanoMediaCodecHelper.java
- android/app/src/main/java/com/xstreaming/nano/NanoRustBridge.java
- android/app/src/main/java/com/xstreaming/nano/NanoStreamView.java

Nimbox imports and adapts the decoder-selection logic, low-latency MediaFormat
settings, MediaCodec input/output and Surface presentation from XStreaming.
These Java components are under GNU AGPL version 3; see LICENSE-AGPL-3.0.

Nimbox's existing code remains under its original GPL-3.0-or-later notices.
GPLv3 section 13 and AGPLv3 section 13 allow covered GPLv3 and AGPLv3 works
to be linked/combined, but the AGPLv3 section 13 network requirements apply
to the combination. Distribution must preserve copyright and license notices
and provide the corresponding source as required by both licenses.

This import intentionally excludes XStreaming's authentication, WebRTC,
controllers, audio, React Native UI and Rust streaming session. Nimbox's own
streaming session feeds the new Surface playback engine via JNI.

Known experimental limitation: the Android SurfaceView is a separate
SurfaceFlinger layer above SDL; additional UI compositing work is necessary
to display SDL's touch HUD and other overlays above video while it plays.
Do not mark this experimental mode as production-ready before verifying
Android layering, menus, touch input, rotation, lifecycle, and fallback.
