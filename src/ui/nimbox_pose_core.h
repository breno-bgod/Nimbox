// SPDX-License-Identifier: GPL-3.0-or-later
// Geometry source of truth for Nimbox Preview 3.0/3.1 and native builds.
// No browser, SDL, Android, PS4 or PS5 dependency; also compiles to WebAssembly.
#pragma once

namespace xc::ui::nimbox {

struct PreviewPose {
    float x, y, z, rotationY, scale, opacity;
    int zIndex;
};

// Values measured from the approved HTML (1600x900 logical stage).
constexpr int kMaxVisibleOffset = 4;
constexpr float kStageWidth = 1600.0f;
constexpr float kStageHeight = 900.0f;
constexpr float kCaseWidth = 286.0f;
constexpr float kCaseHeight = 389.0f;
constexpr float kCaseThickness = 34.0f;
constexpr float kPerspective = 1250.0f;
constexpr float kPerspectiveOriginX = 800.0f;
constexpr float kPerspectiveOriginY = 80.0f + 62.0f + 427.0f * 0.37f;
constexpr float kCaseCenterY = 80.0f + 62.0f + 12.0f + kCaseHeight * 0.5f;

constexpr PreviewPose previewPose(int offset) {
    const int distance = offset < 0 ? -offset : offset;
    if (distance > kMaxVisibleOffset) return {0, 0, -1000, 0, 0, 0, 0};
    if (distance == 0) return {0, 0, 160, 0, 1, 1, 20};
    constexpr float xOffsets[] = {0, 280, 450, 580, 690};
    constexpr float angles[] = {0, 48, 57, 64, 67};
    constexpr float scales[] = {1, 0.90f, 0.80f, 0.72f, 0.65f};
    const float side = offset < 0 ? -1.0f : 1.0f;
    return {side * xOffsets[distance], distance * 10.0f + 25.0f,
            distance * -58.0f, -side * angles[distance], scales[distance],
            1.0f, 20 - distance};
}

// Same wrap convention as the approved preview, including even-sized lists.
constexpr int previewRelativeOffset(int item, int selected, int count) {
    if (count <= 0) return 0;
    int d = item - selected;
    if (d > count / 2) d -= count;
    if (d < -(count / 2)) d += count;
    return d;
}

} // namespace xc::ui::nimbox

// Simple ABI: the same compiled C++ runs natively and as a browser .wasm.
extern "C" float nimbox_pose_value(int offset, int component);
extern "C" int nimbox_relative_offset(int item, int selected, int count);
