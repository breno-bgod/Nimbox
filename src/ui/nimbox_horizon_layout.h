// SPDX-License-Identifier: GPL-3.0-or-later
// NimboPlay Horizon: single geometry source for painting and pointer hit testing.
#pragma once
#include "ui/canvas.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
namespace xc::ui::horizon {
// 1920x1080 PS5-safe stage: all visible covers stay within 96px side margins.
constexpr int kCenterX = 960, kPitch = 195, kSideItems = 4;
constexpr int kSelectedSide = 244, kNeighborSide = 176, kTileCenterY = 746;
// Shared between Canvas rendering and native pointer hit testing.
constexpr Rect kPlayButton{104, 500, 248, 66};
constexpr Rect kDetailsButton{370, 500, 245, 66};
constexpr Rect kProfileAvatarTap{1718, 30, 107, 84};
constexpr Rect kProfilePanel{510, 218, 900, 640};
constexpr Rect kProfileClose{1332, 244, 48, 48};
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
// The carousel must communicate real collection boundaries. No duplicate
// cover is drawn on the other side of the list.
inline int indexForOffset(int selected, int count, int offset) {
    const int index = selected + offset;
    return count > 0 && index >= 0 && index < count ? index : -1;
}

enum class MoveResult { Unchanged, Moved, Boundary, Wrapped };
struct BoundaryGate {
    int direction = 0; // -1 left / +1 right; 0 = not armed
    uint64_t startedMs = 0;
    bool released = false; // only a neutral input may arm a second edge press
    void noteNeutral() { if (direction) released = true; }
    void reset() { direction = 0; startedMs = 0; released = false; }
};

// Require a separate deliberate outward action at an edge before wrapping.
// The minimum 320ms avoids a high-frequency stick/wheel repeat immediately
// wrapping from a single edge hit; time window expires after 1600ms.
// Real motion within the collection always clears this gate.
inline MoveResult navigate(int& selected, int count, int direction, uint64_t nowMs,
                           BoundaryGate& gate) {
    if (count <= 1 || direction == 0) { gate.reset(); return MoveResult::Unchanged; }
    const int next = selected + (direction > 0 ? 1 : -1);
    if (next >= 0 && next < count) {
        selected = next;
        gate.reset();
        return MoveResult::Moved;
    }
    if (gate.direction == direction && gate.released &&
        nowMs >= gate.startedMs &&
        nowMs - gate.startedMs >= 320 && nowMs - gate.startedMs <= 1600) {
        selected = direction > 0 ? 0 : count - 1;
        gate.reset();
        return MoveResult::Wrapped;
    }
    if (gate.direction != direction || nowMs < gate.startedMs ||
        nowMs - gate.startedMs > 1600) {
        gate.direction = direction;
        gate.startedMs = nowMs;
        gate.released = false;
    }
    return MoveResult::Boundary;
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
