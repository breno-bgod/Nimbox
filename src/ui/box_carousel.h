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
        // The selected object is frontal and full-size. Other cases
        // will rotate naturally into this geometry when they are selected.
        g.x = 805.0f; g.y = 230.0f; g.w = 310.0f; g.h = 460.0f;
        g.front = {{g.x, g.y}, {g.x + g.w, g.y},
                   {g.x + g.w, g.y + g.h}, {g.x, g.y + g.h}};
        g.spine = {{g.x - 21, g.y + 7}, g.front.tl,
                   g.front.bl, {g.x - 21, g.y + g.h - 7}};
    } else {
        // Physical-library layout: the spine faces the camera, only a slim
        // slice of the printed cover is visible (approximately 67-80 deg).
        // Near boxes overlap far boxes; not a row of frontal posters.
        const int side = offset < 0 ? -1 : 1;
        g.w = static_cast<float>(std::max(112, 214 - (d - 1) * 20));
        g.h = static_cast<float>(430 - (d - 1) * 11);
        g.y = static_cast<float>(268 + (d - 1) * 7);
        const float shelf = 185.0f + (d - 1) * 113.0f;
        g.x = side < 0 ? 960.0f - shelf - g.w : 960.0f + shelf;
        const float tilt = static_cast<float>(49 + std::min(d, 5) * 4);
        const float sw = static_cast<float>(26 + std::min(d, 5));
        if (side < 0) {
            g.front = {{g.x, g.y + tilt}, {g.x + g.w, g.y},
                       {g.x + g.w, g.y + g.h},
                       {g.x, g.y + g.h - tilt}};
            g.spine = {{g.x - sw, g.y + tilt - 4}, g.front.tl,
                       g.front.bl, {g.x - sw, g.y + g.h - tilt - 4}};
        } else {
            g.front = {{g.x, g.y}, {g.x + g.w, g.y + tilt},
                       {g.x + g.w, g.y + g.h - tilt},
                       {g.x, g.y + g.h}};
            g.spine = {g.front.tr, {g.x + g.w + sw, g.y + tilt - 4},
                       {g.x + g.w + sw, g.y + g.h - tilt - 4},
                       g.front.br};
        }
    }
    const float minX = std::min({g.front.tl.x, g.front.bl.x, g.spine.tl.x, g.spine.bl.x});
    const float maxX = std::max({g.front.tr.x, g.front.br.x, g.spine.tr.x, g.spine.br.x});
    const float minY = std::min({g.front.tl.y, g.front.tr.y, g.spine.tl.y, g.spine.tr.y});
    const float maxY = std::max({g.front.bl.y, g.front.br.y, g.spine.bl.y, g.spine.br.y});
    g.bounds = {static_cast<int>(std::floor(minX)), static_cast<int>(std::floor(minY)),
                static_cast<int>(std::ceil(maxX - minX)), static_cast<int>(std::ceil(maxY - minY))};
    return g;
}

// Continuous case placement for the animated navigation. The artwork remains
// in 2D, but all four corners interpolate to convey physical movement.
inline CaseGeometry layoutAnimated(float offset) {
    if (!std::isfinite(offset)) return layout(0);
    const int lower = static_cast<int>(std::floor(offset));
    const int upper = lower + 1;
    const float t = std::clamp(offset - static_cast<float>(lower), 0.0f, 1.0f);
    if (t < 0.0001f) return layout(lower);
    const CaseGeometry a = layout(lower), b = layout(upper);
    CaseGeometry g;
    g.distance = static_cast<int>(std::round(std::fabs(offset)));
    g.x = a.x + (b.x - a.x) * t;
    g.y = a.y + (b.y - a.y) * t;
    g.w = a.w + (b.w - a.w) * t;
    g.h = a.h + (b.h - a.h) * t;
    auto mixQuad = [t](Quad x, Quad y) {
        return Quad{between(x.tl, y.tl, t), between(x.tr, y.tr, t),
                    between(x.br, y.br, t), between(x.bl, y.bl, t)};
    };
    g.front = mixQuad(a.front, b.front);
    g.spine = mixQuad(a.spine, b.spine);
    const float minX = std::min({g.front.tl.x, g.front.bl.x, g.front.tr.x, g.front.br.x,
                                 g.spine.tl.x, g.spine.bl.x, g.spine.tr.x, g.spine.br.x});
    const float maxX = std::max({g.front.tl.x, g.front.bl.x, g.front.tr.x, g.front.br.x,
                                 g.spine.tl.x, g.spine.bl.x, g.spine.tr.x, g.spine.br.x});
    const float minY = std::min({g.front.tl.y, g.front.bl.y, g.front.tr.y, g.front.br.y,
                                 g.spine.tl.y, g.spine.bl.y, g.spine.tr.y, g.spine.br.y});
    const float maxY = std::max({g.front.tl.y, g.front.bl.y, g.front.tr.y, g.front.br.y,
                                 g.spine.tl.y, g.spine.bl.y, g.spine.tr.y, g.spine.br.y});
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
    if (count <= 0 || offset < -count / 2 || offset > (count - 1) / 2) return -1;
    return ((selected + offset) % count + count) % count;
}

} // namespace xc::ui::nimbox
