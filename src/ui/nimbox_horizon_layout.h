// SPDX-License-Identifier: GPL-3.0-or-later
// Nimbox Horizon: single geometry source for painting and pointer hit testing.
#pragma once
#include "ui/canvas.h"
#include <algorithm>
#include <cmath>
namespace xc::ui::horizon {
constexpr int kCenterX = 960, kPitch = 210, kSideItems = 4;
constexpr int kSelectedSide = 230, kNeighborSide = 174, kTileCenterY = 731;
constexpr Rect kPlayButton{104, 485, 230, 62};
constexpr Rect kDetailsButton{350, 485, 244, 62};
constexpr Rect kModeSwitch{1514, 242, 294, 53};
inline Rect tileRect(float offset) {
    if (!std::isfinite(offset)) offset = 0.0f;
    offset = std::clamp(offset, -5.0f, 5.0f);
    const float scale = std::clamp(1.0f - std::fabs(offset), 0.0f, 1.0f);
    const int side = int(std::lround(kNeighborSide + (kSelectedSide - kNeighborSide) * scale));
    const int cx = kCenterX + int(std::lround(offset * kPitch));
    return {cx - side / 2, kTileCenterY - side / 2, side, side};
}
inline bool contains(Rect r, float x, float y) {
    return r.w > 0 && r.h > 0 && x >= r.x && x < r.x+r.w && y >= r.y && y < r.y+r.h;
}
} // namespace xc::ui::horizon
