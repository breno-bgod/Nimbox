// SPDX-License-Identifier: GPL-3.0-or-later
// Colecao Viva — Xbox-style physical case finish on the EXISTING projected
// eight-vertex cuboid. This file owns *materials only*, never camera/poses.
// No network, texture asset, shader or additional platform dependency.
#pragma once

#include "ui/box_carousel.h"
#include <algorithm>
#include <cmath>

namespace xc::ui::nimbox::finish {

// Material tokens derive from the green translucent Xbox One game case
// supplied as visual reference; Xbox branding is packaging, not Nimbox branding.
constexpr Color kPlasticDark = rgba(8, 83, 25);
constexpr Color kPlasticSide = rgba(13, 99, 31);
constexpr Color kPlasticTop = rgba(37, 173, 47);
constexpr Color kPlasticHighlight = rgba(122, 227, 86, 112);
constexpr Color kBanner = rgba(35, 168, 28);
constexpr Color kBannerBright = rgba(113, 230, 55, 126);
constexpr Color kSleeveWell = rgba(7, 34, 27);
constexpr Color kPrintFooter = rgba(14, 51, 31);
constexpr Color kPrintWhite = rgba(248, 255, 246);

constexpr float kBannerTop = 0.012f;
constexpr float kBannerBottom = 0.132f;
constexpr float kArtTop = 0.150f;
constexpr float kArtBottom = 0.856f;
constexpr float kFooterTop = 0.865f;
constexpr float kFooterBottom = 0.978f;

inline Quad horizontalSlice(const Quad& q, float left, float right) {
    return {between(q.tl, q.tr, left), between(q.tl, q.tr, right),
            between(q.bl, q.br, right), between(q.bl, q.br, left)};
}

// UV fractions follow the physical front face, not the screen AABB.
// The cached xCloud art remains untouched; this quad is its printed sleeve.
inline Quad insetFront(const Quad& q, float u0, float v0, float u1, float v1) {
    return verticalSlice(horizontalSlice(q, u0, u1), v0, v1);
}

inline Quad imageSlot(const CaseGeometry& g) {
    return insetFront(g.front, 0.045f, kArtTop, 0.955f, kArtBottom);
}

inline void drawShell(Canvas& c, const CaseGeometry& g, bool selected) {
    // Physical 34 mm-deep cuboid, already projected by box_carousel.h.
    // The front surface reads as green plastic, not a recoloured game image.
    c.fillQuad(g.top, kPlasticTop);
    c.fillQuad(horizontalSlice(g.top, 0.03f, 0.97f),
               rgba(134, 242, 95, selected ? 82 : 48));
    c.fillQuad(g.spine, kPlasticSide);
    c.fillQuad(horizontalSlice(g.spine, 0.02f, 0.16f),
               rgba(165, 249, 132, selected ? 111 : 70));
    c.fillQuad(horizontalSlice(g.spine, 0.84f, 0.97f),
               rgba(2, 42, 21, 120));
    c.fillQuad(g.front, kPlasticDark);
    // Green moulded plastic around the artwork, plus the official-style band.
    c.fillQuad(verticalSlice(g.front, kBannerTop, kBannerBottom), kBanner);
    c.fillQuad(verticalSlice(g.front, kBannerTop, 0.036f), kBannerBright);
    c.fillQuad(verticalSlice(g.front, 0.117f, kBannerBottom),
               rgba(1, 70, 23, 107));
    // Inset black-green print bed creates a shallow inner lip beneath the art.
    c.fillQuad(insetFront(g.front, 0.028f, 0.141f, 0.972f, 0.865f), kSleeveWell);
    c.fillQuad(verticalSlice(g.front, kFooterTop, kFooterBottom), kPrintFooter);
}

inline void drawXboxOrb(Canvas& c, float cx, float cy, float radius) {
    // Vector interpretation of the white Xbox sphere with an X-shaped split.
    // No downloaded asset and no separate brand treatment of the Nimbox app.
    const Color green = rgba(20, 150, 37);
    c.fillCircle(cx, cy, radius, kPrintWhite);
    const float t = std::max(2.0f, radius * 0.26f);
    c.line(cx - radius * 0.60f, cy - radius * 0.69f,
           cx + radius * 0.60f, cy + radius * 0.67f, t, green);
    c.line(cx + radius * 0.60f, cy - radius * 0.69f,
           cx - radius * 0.60f, cy + radius * 0.67f, t, green);
    c.fillCircle(cx, cy - radius * 0.07f, radius * 0.12f, green);
}

inline void drawGlaze(Canvas& c, const CaseGeometry& g, bool selected) {
    // Subtle translucent acetate, seams and green rim; constrained to the
    // projected front so the actual xCloud artwork still dominates.
    c.fillQuad(insetFront(g.front, 0.074f, kArtTop, 0.115f, kArtBottom),
               rgba(249, 255, 242, selected ? 31 : 17));
    c.fillQuad(insetFront(g.front, 0.118f, kArtTop, 0.135f, kArtBottom),
               rgba(231, 255, 223, selected ? 17 : 9));
    c.fillQuad(insetFront(g.front, 0.905f, kArtTop, 0.953f, kArtBottom),
               rgba(0, 34, 16, 42));
    c.fillQuad(horizontalSlice(g.front, 0.008f, 0.029f),
               rgba(71, 198, 53, selected ? 166 : 120));
    c.fillQuad(horizontalSlice(g.front, 0.971f, 0.991f),
               rgba(40, 153, 38, selected ? 183 : 143));
    c.fillQuad(verticalSlice(g.front, 0.978f, 0.994f),
               rgba(23, 142, 33, 223));

    const Color edge = rgba(160, 250, 113, selected ? 205 : 142);
    c.line(g.front.tl.x, g.front.tl.y, g.front.tr.x, g.front.tr.y,
           selected ? 2.6f : 1.6f, edge);
    c.line(g.front.tl.x, g.front.tl.y, g.front.bl.x, g.front.bl.y,
           selected ? 2.7f : 1.7f, edge);
    c.line(g.front.tr.x, g.front.tr.y, g.front.br.x, g.front.br.y,
           selected ? 2.3f : 1.5f, rgba(17, 119, 41, 230));
    c.line(g.front.bl.x, g.front.bl.y, g.front.br.x, g.front.br.y,
           2.3f, rgba(94, 213, 63, 181));
    c.line(g.top.tl.x, g.top.tl.y, g.top.tr.x, g.top.tr.y,
           1.7f, rgba(179, 254, 128, 184));
    c.line(g.spine.tl.x, g.spine.tl.y, g.spine.bl.x, g.spine.bl.y,
           1.8f, rgba(148, 236, 121, 139));
}

} // namespace xc::ui::nimbox::finish
