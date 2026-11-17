// SPDX-License-Identifier: GPL-3.0-or-later
// Nimbox visual-only environment presets from the approved HTML Preview 3.1.
// No effect on cloud streaming, decoder, network, gamepad or Xbox sessions.
#pragma once
#include "ui/canvas.h"
#include <algorithm>

namespace xc::ui::nimbox_theme {

struct Palette {
    const char* name;
    Color sky, horizon, mid, low, accent;
};

// Source: #stage[data-theme] in nimbox-preview-3.1.html.
// The actual Home uses restrained alpha grading to preserve the readable
// Xbox artwork rather than copying raw CSS colors over it.
constexpr Palette kPalettes[5] = {
    {"Aurora",         rgba(140,190,213), rgba(167,213,232), rgba(79,130,167), rgba(28,71,100), rgba(125,199,212)},
    {"Infinity Light", rgba(222,238,241), rgba(215,237,239), rgba(160,198,206), rgba(69,115,141), rgba(172,222,222)},
    {"Nimbox Blue",    rgba(128,182,215), rgba(121,190,218), rgba(54,123,166), rgba(22,63,106), rgba(105,190,231)},
    {"Emerald",        rgba(142,207,194), rgba(180,227,216), rgba(74,151,143), rgba(34,86,93), rgba(137,211,177)},
    {"Cinema Dark",    rgba(78,112,133), rgba(85,124,142), rgba(35,71,94), rgba(8,29,50), rgba(112,171,188)}
};
constexpr int kThemeCount = 5;
inline const Palette& palette(int index) { return kPalettes[std::clamp(index, 0, kThemeCount - 1)]; }
constexpr Rect kThemeCards[5] = {
    {1329, 297, 228, 55}, {1572, 297, 228, 55},
    {1329, 362, 228, 55}, {1572, 362, 228, 55},
    {1329, 427, 228, 55}
};
constexpr Rect kReflectionTrack{1334, 544, 452, 12};
constexpr Rect kReducedMotionButton{1580, 589, 205, 44};
constexpr int kAppearanceFirst = 28;
constexpr int kAppearanceReflection = 29;
constexpr int kAppearanceMotion = 30;
constexpr int kMinReflection = 0;
constexpr int kMaxReflection = 100;

inline int clampReflection(int value) { return std::clamp(value, kMinReflection, kMaxReflection); }
inline int reflectionFromX(float x) {
    const float f = (x - kReflectionTrack.x) / static_cast<float>(kReflectionTrack.w);
    return clampReflection(static_cast<int>(std::clamp(f, 0.0f, 1.0f) * 100 + 0.5f));
}
inline bool contains(Rect r, float x, float y) {
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}
}  // namespace xc::ui::nimbox_theme
