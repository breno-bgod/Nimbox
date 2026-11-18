// SPDX-License-Identifier: GPL-3.0-or-later
// Nimbox Horizon: single geometry source for painting and pointer hit testing.
#pragma once
#include "ui/canvas.h"
#include <algorithm>
#include <cmath>
#include <vector>
namespace xc::ui::horizon {
constexpr int kCenterX = 960, kPitch = 210, kSideItems = 4;
constexpr int kSelectedSide = 230, kNeighborSide = 174, kTileCenterY = 731;
constexpr Rect kPlayButton{104, 485, 230, 62};
constexpr Rect kDetailsButton{350, 485, 244, 62};
inline Rect tileRect(float offset) {
    if (!std::isfinite(offset)) offset = 0.0f;
    offset = std::clamp(offset, -5.0f, 5.0f);
    const float scale = std::clamp(1.0f - std::fabs(offset), 0.0f, 1.0f);
    const int side = int(std::lround(kNeighborSide + (kSelectedSide - kNeighborSide) * scale));
    const int cx = kCenterX + int(std::lround(offset * kPitch));
    return {cx - side / 2, kTileCenterY - side / 2, side, side};
}
// Only one visual collection exists. These utilities are intentionally
// independent of the removed physical 3D case geometry and its renderer.
inline int indexForOffset(int selected, int count, int offset) {
    if (count <= 0 || offset < -count / 2 || offset > (count - 1) / 2) return -1;
    return ((selected + offset) % count + count) % count;
}
inline std::vector<int> drawOrder(int selected, int count, float slide) {
    std::vector<int> order;
    if (count <= 0) return order;
    if (!std::isfinite(slide)) slide = 0.0f;
    slide = std::clamp(slide, -float(kSideItems), float(kSideItems));
    for (int offset = -kSideItems; offset <= kSideItems; ++offset)
        if (indexForOffset(selected, count, offset) >= 0) order.push_back(offset);
    std::stable_sort(order.begin(), order.end(), [slide](int a, int b) {
        return std::fabs(float(a) + slide) > std::fabs(float(b) + slide);
    });
    return order;
}
inline bool contains(Rect r, float x, float y) {
    return r.w > 0 && r.h > 0 && x >= r.x && x < r.x+r.w && y >= r.y && y < r.y+r.h;
}
} // namespace xc::ui::horizon
