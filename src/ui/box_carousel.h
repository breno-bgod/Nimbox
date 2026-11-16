// SPDX-License-Identifier: GPL-3.0-or-later
// Nimbox 3D Library — shared geometry for native rendering and touch hit testing.
#pragma once

#include "ui/canvas.h"
#include <algorithm>
#include <cmath>

namespace xc::ui::nimbox {

constexpr int kSideCases = 6;

struct CaseGeometry {
    Quad front;
    Quad spine;
    Rect bounds;
    int distance = 0;
    float x = 0, y = 0, w = 0, h = 0;
};

inline Point between(Point a, Point b, float t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

// Extract a vertical slice of the projected front for the packaging banner,
// square game artwork and printed title plate.
inline Quad verticalSlice(const Quad& q, float top, float bottom) {
    return {between(q.tl, q.bl, top), between(q.tr, q.br, top),
            between(q.tr, q.br, bottom), between(q.tl, q.bl, bottom)};
}

inline CaseGeometry layout(int offset) {
    const int d = std::abs(offset);
    CaseGeometry g;
    g.distance = d;
    if (d == 0) {
        g.x = 805.0f; g.y = 230.0f; g.w = 310.0f; g.h = 460.0f;
        g.front = {{g.x, g.y}, {g.x + g.w, g.y}, {g.x + g.w, g.y + g.h}, {g.x, g.y + g.h}};
        g.spine = {{g.x - 22, g.y + 6}, g.front.tl, g.front.bl, {g.x - 22, g.y + g.h - 6}};
    } else {
        // Close-packed cases create the staggered physical shelf seen in the
        // reference. A shallow trapezoid turns each neighbor toward the viewer.
        const int dir = offset < 0 ? -1 : 1;
        g.w = static_cast<float>(std::max(126, 236 - (d - 1) * 18));
        g.h = static_cast<float>(std::max(285, 416 - (d - 1) * 25));
        g.x = 960.0f + dir * (300.0f + (d - 1) * 122.0f) - g.w * 0.5f;
        g.y = 294.0f + (d - 1) * 13.0f;
        const float inset = std::min(47.0f, 25.0f + d * 4.0f);
        if (dir < 0) {
            g.front = {{g.x, g.y + inset}, {g.x + g.w, g.y}, {g.x + g.w, g.y + g.h},
                       {g.x, g.y + g.h - inset}};
            const float sw = 13.0f + d * 1.2f;
            g.spine = {{g.x - sw, g.y + inset - 3}, g.front.tl, g.front.bl,
                       {g.x - sw, g.y + g.h - inset - 3}};
        } else {
            g.front = {{g.x, g.y}, {g.x + g.w, g.y + inset},
                       {g.x + g.w, g.y + g.h - inset}, {g.x, g.y + g.h}};
            const float sw = 13.0f + d * 1.2f;
            g.spine = {g.front.tr, {g.x + g.w + sw, g.y + inset - 3},
                       {g.x + g.w + sw, g.y + g.h - inset - 3}, g.front.br};
        }
    }
    // Broad-phase hit testing only; exact projected polygon follows.
    const float minX = std::min({g.front.tl.x, g.front.bl.x, g.spine.tl.x, g.spine.bl.x});
    const float maxX = std::max({g.front.tr.x, g.front.br.x, g.spine.tr.x, g.spine.br.x});
    const float minY = std::min({g.front.tl.y, g.front.tr.y, g.spine.tl.y, g.spine.tr.y});
    const float maxY = std::max({g.front.bl.y, g.front.br.y, g.spine.bl.y, g.spine.br.y});
    g.bounds = {static_cast<int>(std::floor(minX)), static_cast<int>(std::floor(minY)),
                static_cast<int>(std::ceil(maxX - minX)), static_cast<int>(std::ceil(maxY - minY))};
    return g;
}

inline bool insideQuad(const Quad& q, float x, float y) {
    const Point pts[4] = {q.tl, q.tr, q.br, q.bl};
    int sign = 0;
    for (int i = 0; i < 4; ++i) {
        const Point a = pts[i], b = pts[(i + 1) % 4];
        const float cross = (b.x - a.x) * (y - a.y) - (b.y - a.y) * (x - a.x);
        if (std::fabs(cross) < 0.01f) continue;
        const int edgeSign = cross > 0 ? 1 : -1;
        if (sign != 0 && edgeSign != sign) return false;
        sign = edgeSign;
    }
    return true;
}

inline bool hit(const CaseGeometry& g, float x, float y) {
    if (x < g.bounds.x || x >= g.bounds.x + g.bounds.w ||
        y < g.bounds.y || y >= g.bounds.y + g.bounds.h) return false;
    return insideQuad(g.front, x, y) || insideQuad(g.spine, x, y);
}

// When a selected game is at the beginning/end of the list, wrap
// neighbors so both shelves stay filled. Limit neighbors for short lists.
inline int indexForOffset(int selected, int count, int offset) {
    if (count <= 0 || std::abs(offset) > (count - 1) / 2) return -1;
    return ((selected + offset) % count + count) % count;
}

} // namespace xc::ui::nimbox
