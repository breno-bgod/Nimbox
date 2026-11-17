// SPDX-License-Identifier: GPL-3.0-or-later
// Nimbox 3D Library — shared geometry for native rendering and touch hit testing.
//
// Each case is a real 3D box (front face + one side face) placed with the
// poses of the approved HTML preview (nimbox_pose_core.h) and projected with
// the same perspective, so the native shelf and the preview share one layout.
#pragma once

#include "ui/canvas.h"
#include "ui/nimbox_pose_core.h"
#include <algorithm>
#include <cmath>

namespace xc::ui::nimbox {

constexpr int kSideCases = 6;

// Preview stage (1600x900) -> 1920x1080 menu. The scale keeps the selected
// case about as tall as before (the title and hints below were laid out for it).
constexpr float kAppScale = 1.06f;
constexpr float kAppCenterX = 960.0f;
constexpr float kAppCenterY = 458.0f;
constexpr float kSelectedZ = 160.0f;
// Stage Y of the selected case's centre after perspective (the anchor above).
constexpr float kSelectedCenterY =
    kPerspectiveOriginY + (kCaseCenterY - kPerspectiveOriginY) * kPerspective / (kPerspective - kSelectedZ);
constexpr float kHalfW = kCaseWidth * 0.5f;
constexpr float kHalfH = kCaseHeight * 0.5f;
constexpr float kHalfD = kCaseThickness * 0.5f;

struct Vec3 {
    float x = 0, y = 0, z = 0;
};

inline Vec3 lerp3(Vec3 a, Vec3 b, float t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
}

struct CasePose {
    float x = 0, y = 0, z = 0, rotation = 0, scale = 1;
};

// Poses of the approved preview for |offset| <= 4. The menu is wider than the
// preview stage, so the same progression continues for the farther cases.
inline CasePose poseFor(int offset) {
    const int d = offset < 0 ? -offset : offset;
    if (d <= kMaxVisibleOffset) {
        const PreviewPose p = previewPose(offset);
        return {p.x, p.y, p.z, p.rotationY, p.scale};
    }
    const float side = offset < 0 ? -1.0f : 1.0f;
    const float e = static_cast<float>(d - kMaxVisibleOffset);
    return {side * (690.0f + 95.0f * e), d * 10.0f + 25.0f, d * -58.0f,
            -side * (67.0f + e), std::max(0.5f, 0.65f - 0.045f * e)};
}

// CSS order: translate3d(x,y,z) rotateY(a) scale(s), perspective on the stage.
struct Projector {
    CasePose pose;
    float cs = 1, sn = 0;

    Point project(Vec3 local) const {
        const float px = local.x * pose.scale, py = local.y * pose.scale, pz = local.z * pose.scale;
        const float rx = px * cs + pz * sn;
        const float rz = -px * sn + pz * cs;
        const float X = kStageWidth * 0.5f + pose.x + rx;
        const float Y = kCaseCenterY + pose.y + py;
        const float Z = pose.z + rz;
        const float f = kPerspective / std::max(1.0f, kPerspective - Z);
        const float sx = kPerspectiveOriginX + (X - kPerspectiveOriginX) * f;
        const float sy = kPerspectiveOriginY + (Y - kPerspectiveOriginY) * f;
        return {kAppCenterX + (sx - kPerspectiveOriginX) * kAppScale,
                kAppCenterY + (sy - kSelectedCenterY) * kAppScale};
    }
};

inline Projector makeProjector(const CasePose& pose) {
    const float rad = pose.rotation * 3.14159265358979f / 180.0f;
    return {pose, std::cos(rad), std::sin(rad)};
}

// Four 3D corners of a face, in the same order as a Quad (tl, tr, br, bl).
struct Face3 {
    Vec3 tl, tr, br, bl;
};

inline Face3 frontFace() {
    return {{-kHalfW, -kHalfH, kHalfD}, {kHalfW, -kHalfH, kHalfD},
            {kHalfW, kHalfH, kHalfD}, {-kHalfW, kHalfH, kHalfD}};
}

// The side that turns toward the camera. Texture u runs from the back edge to
// the front edge on the left face, and from the front edge to the back edge on
// the right face, so printed text reads the same way on both.
inline Face3 sideFace(bool left) {
    if (left) {
        return {{-kHalfW, -kHalfH, -kHalfD}, {-kHalfW, -kHalfH, kHalfD},
                {-kHalfW, kHalfH, kHalfD}, {-kHalfW, kHalfH, -kHalfD}};
    }
    return {{kHalfW, -kHalfH, kHalfD}, {kHalfW, -kHalfH, -kHalfD},
            {kHalfW, kHalfH, -kHalfD}, {kHalfW, kHalfH, kHalfD}};
}

inline Quad projectFace(const Projector& pr, const Face3& f) {
    return {pr.project(f.tl), pr.project(f.tr), pr.project(f.br), pr.project(f.bl)};
}

struct CaseGeometry {
    Quad front;
    Quad spine;               // the visible side face (left or right of the front)
    Rect bounds;
    int distance = 0;
    float x = 0, y = 0, w = 0, h = 0;  // bounding box of the front face
    Projector proj;
    bool sideLeft = true;     // which side face `spine` is
    bool sideVisible = false; // false when the case faces the camera squarely
};

inline void finishGeometry(CaseGeometry& g) {
    const Point pts[8] = {g.front.tl, g.front.tr, g.front.br, g.front.bl,
                          g.spine.tl, g.spine.tr, g.spine.br, g.spine.bl};
    float minX = pts[0].x, maxX = pts[0].x, minY = pts[0].y, maxY = pts[0].y;
    for (const Point& p : pts) {
        minX = std::min(minX, p.x); maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y); maxY = std::max(maxY, p.y);
    }
    g.bounds = {static_cast<int>(std::floor(minX)), static_cast<int>(std::floor(minY)),
                static_cast<int>(std::ceil(maxX - minX)), static_cast<int>(std::ceil(maxY - minY))};
    float fx0 = g.front.tl.x, fx1 = fx0, fy0 = g.front.tl.y, fy1 = fy0;
    for (const Point& p : {g.front.tl, g.front.tr, g.front.br, g.front.bl}) {
        fx0 = std::min(fx0, p.x); fx1 = std::max(fx1, p.x);
        fy0 = std::min(fy0, p.y); fy1 = std::max(fy1, p.y);
    }
    g.x = fx0; g.y = fy0; g.w = fx1 - fx0; g.h = fy1 - fy0;
}

inline CaseGeometry layoutPose(const CasePose& pose, int distance) {
    CaseGeometry g;
    g.distance = distance;
    g.proj = makeProjector(pose);
    g.front = projectFace(g.proj, frontFace());
    // rotateY(+) brings the left face toward the viewer (cases left of the selection).
    g.sideLeft = pose.rotation >= 0.0f;
    g.sideVisible = std::fabs(pose.rotation) > 2.0f;
    g.spine = projectFace(g.proj, sideFace(g.sideLeft));
    finishGeometry(g);
    return g;
}

inline CaseGeometry layout(int offset) {
    return layoutPose(poseFor(offset), std::abs(offset));
}

// Continuous case placement for the animated navigation: the pose itself is
// interpolated, so the cases turn and travel as in the preview.
inline CaseGeometry layoutAnimated(float offset) {
    if (!std::isfinite(offset)) return layout(0);
    const int lower = static_cast<int>(std::floor(offset));
    const int upper = lower + 1;
    const float t = std::clamp(offset - static_cast<float>(lower), 0.0f, 1.0f);
    if (t < 0.0001f) return layout(lower);
    const CasePose a = poseFor(lower), b = poseFor(upper);
    auto mix = [t](float p, float q) { return p + (q - p) * t; };
    const CasePose pose{mix(a.x, b.x), mix(a.y, b.y), mix(a.z, b.z),
                        mix(a.rotation, b.rotation), mix(a.scale, b.scale)};
    return layoutPose(pose, static_cast<int>(std::round(std::fabs(offset))));
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
    return insideQuad(g.front, x, y) || (g.sideVisible && insideQuad(g.spine, x, y));
}

// When a selected game is at the beginning/end of the list, wrap
// neighbors so both shelves stay filled. Limit neighbors for short lists.
inline int indexForOffset(int selected, int count, int offset) {
    if (count <= 0 || offset < -count / 2 || offset > (count - 1) / 2) return -1;
    return ((selected + offset) % count + count) % count;
}

} // namespace xc::ui::nimbox
