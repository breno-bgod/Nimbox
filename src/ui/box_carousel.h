// SPDX-License-Identifier: GPL-3.0-or-later
// Nimbox Colecao Viva: 3D case geometry projected into the existing Canvas.
// Camera, poses and physical case dimensions come from the approved C++
// Preview 3.1 contract. No platform-specific GPU API is required at this stage.
#pragma once

#include "ui/canvas.h"
#include "ui/nimbox_pose_core.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace xc::ui::nimbox {

// The HTML stage shows at most four cases on either side of the selection.
constexpr int kSideCases = kMaxVisibleOffset;
constexpr float kCanvasScale = 1920.0f / kStageWidth;
constexpr float kPi3D = 3.14159265358979323846f;

struct CaseGeometry {
    Quad front;             // printed artwork on the near Z face
    Quad spine;             // visible physical side face
    Quad top;               // physical top face (depth = kCaseThickness)
    Rect bounds;            // union of all 8 projected box corners
    int distance = 0;
    float x = 0, y = 0, w = 0, h = 0; // front-face screen AABB for existing labels
};

inline Point between(Point a, Point b, float t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

inline Quad verticalSlice(const Quad& q, float top, float bottom) {
    return {between(q.tl, q.bl, top), between(q.tr, q.br, top),
            between(q.tr, q.br, bottom), between(q.tl, q.bl, bottom)};
}

struct CasePoint3D {
    float x, y, z;
};

// Interpolate actual 3D pose values, not the corners of unrelated 2D trapezoids.
// This keeps the box rigid while it turns towards/away from the camera.
inline PreviewPose animatedPose(float offset) {
    if (!std::isfinite(offset)) return previewPose(0);
    offset = std::clamp(offset, -float(kMaxVisibleOffset), float(kMaxVisibleOffset));
    const int lo = static_cast<int>(std::floor(offset));
    const int hi = std::min(lo + 1, kMaxVisibleOffset);
    const float t = std::clamp(offset - float(lo), 0.0f, 1.0f);
    const PreviewPose a = previewPose(lo), b = previewPose(hi);
    const auto mix = [t](float x, float y) { return x + (y - x) * t; };
    return {mix(a.x, b.x), mix(a.y, b.y), mix(a.z, b.z),
            mix(a.rotationY, b.rotationY), mix(a.scale, b.scale),
            mix(a.opacity, b.opacity), a.zIndex};
}

inline Point projectCasePoint(const CasePoint3D& local, const PreviewPose& pose) {
    const float r = pose.rotationY * kPi3D / 180.0f;
    const float cosine = std::cos(r), sine = std::sin(r);
    const float worldX = kPerspectiveOriginX + pose.x +
                         pose.scale * (local.x * cosine + local.z * sine);
    const float worldY = kCaseCenterY + pose.y + pose.scale * local.y;
    const float worldZ = pose.z + pose.scale * (-local.x * sine + local.z * cosine);
    // The HTML camera uses perspective:1250px and a shifted perspective-origin.
    // Clamp the near plane so a malformed transient cannot produce infinities.
    const float factor = kPerspective / std::max(32.0f, kPerspective - worldZ);
    return {(kPerspectiveOriginX + (worldX - kPerspectiveOriginX) * factor) * kCanvasScale,
            (kPerspectiveOriginY + (worldY - kPerspectiveOriginY) * factor) * kCanvasScale};
}

inline CaseGeometry makeCase(const PreviewPose& pose, float offset) {
    constexpr float hw = kCaseWidth * 0.5f;
    constexpr float hh = kCaseHeight * 0.5f;
    constexpr float hz = kCaseThickness * 0.5f;
    // Eight corners of a rigid rectangular prism (front at positive Z).
    const Point ftl = projectCasePoint({-hw,-hh, hz}, pose);
    const Point ftr = projectCasePoint({ hw,-hh, hz}, pose);
    const Point fbr = projectCasePoint({ hw, hh, hz}, pose);
    const Point fbl = projectCasePoint({-hw, hh, hz}, pose);
    const Point btl = projectCasePoint({-hw,-hh,-hz}, pose);
    const Point btr = projectCasePoint({ hw,-hh,-hz}, pose);
    const Point bbr = projectCasePoint({ hw, hh,-hz}, pose);
    const Point bbl = projectCasePoint({-hw, hh,-hz}, pose);
    CaseGeometry g;
    g.distance = static_cast<int>(std::round(std::fabs(offset)));
    g.front = {ftl, ftr, fbr, fbl};
    // With a positive Y angle the left edge is closer, with negative the
    // right edge is closer. Display exactly that side of the real box.
    g.spine = pose.rotationY >= 0.0f
              ? Quad{btl, ftl, fbl, bbl}
              : Quad{ftr, btr, bbr, fbr};
    g.top = {btl, btr, ftr, ftl};
    const Point all[] = {ftl,ftr,fbr,fbl,btl,btr,bbr,bbl};
    float minX = all[0].x, maxX = all[0].x;
    float minY = all[0].y, maxY = all[0].y;
    for (const Point& p : all) {
        minX = std::min(minX, p.x); maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y); maxY = std::max(maxY, p.y);
    }
    g.bounds = {static_cast<int>(std::floor(minX)), static_cast<int>(std::floor(minY)),
                static_cast<int>(std::ceil(maxX) - std::floor(minX)),
                static_cast<int>(std::ceil(maxY) - std::floor(minY))};
    const float faceMinX = std::min({ftl.x, ftr.x, fbr.x, fbl.x});
    const float faceMaxX = std::max({ftl.x, ftr.x, fbr.x, fbl.x});
    const float faceMinY = std::min({ftl.y, ftr.y, fbr.y, fbl.y});
    const float faceMaxY = std::max({ftl.y, ftr.y, fbr.y, fbl.y});
    g.x = faceMinX; g.y = faceMinY;
    g.w = faceMaxX - faceMinX; g.h = faceMaxY - faceMinY;
    return g;
}

inline CaseGeometry layout(int offset) {
    return makeCase(previewPose(std::clamp(offset, -kMaxVisibleOffset, kMaxVisibleOffset)),
                    float(offset));
}

inline CaseGeometry layoutAnimated(float offset) {
    if (!std::isfinite(offset)) offset = 0.0f;
    return makeCase(animatedPose(offset), offset);
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
    return insideQuad(g.front, x, y) ||
           insideQuad(g.spine, x, y) ||
           insideQuad(g.top, x, y);
}

inline int indexForOffset(int selected, int count, int offset) {
    if (count <= 0 || offset < -count / 2 || offset > (count - 1) / 2) return -1;
    return ((selected + offset) % count + count) % count;
}

// Painter order is shared by rendering and pointer hit-testing. Far cases are
// drawn first, while input walks this vector backward (frontmost first).
// This matters when physical boxes overlap in a cascading arc or animate
// between selected positions.
inline std::vector<int> caseDrawOrder(int selected, int count, float slide) {
    std::vector<int> order;
    if (!std::isfinite(slide)) slide = 0.0f;
    slide = std::clamp(slide, -1.0f, 1.0f);
    for (int offset = -kSideCases; offset <= kSideCases; ++offset) {
        if (indexForOffset(selected, count, offset) >= 0) order.push_back(offset);
    }
    std::stable_sort(order.begin(), order.end(), [slide](int a, int b) {
        return std::fabs(static_cast<float>(a) + slide) >
               std::fabs(static_cast<float>(b) + slide);
    });
    return order;
}

} // namespace xc::ui::nimbox
