// SPDX-License-Identifier: GPL-3.0-or-later
// Nimbox Preview 3.0/3.1 native VISUAL layout tokens.
// Rendering geometry only. Do not introduce actions, fake navigation,
// catalog mutations, authentication, or streaming changes here.
#pragma once

#include "ui/canvas.h"

namespace xc::ui::preview31 {

// The approved HTML stage is 1600x900. The native canvas is 1920x1080.
// Keep the 3D case projection and physical finish untouched. The studio
// hero occupies the upper stage and a real catalog rail starts beneath it.
constexpr int kHeroShelfBoundary = 869;
constexpr int kShelfHeaderY = 886;
constexpr int kShelfTileY = 925;
constexpr int kShelfTileW = 266;
constexpr int kShelfTileH = 96;
constexpr int kShelfGap = 18;
constexpr int kShelfLeft = 96;
constexpr int kShelfBottom = 1016;
constexpr int kFooterTop = 1018;
constexpr int kFocusNameY = 711;
constexpr int kFocusMetaY = 765;
constexpr int kPageMargin = 96;
// Existing CTA and its pointer hitbox share this coordinate. Only move
// together; never draw an inert button elsewhere.
constexpr Rect kDetailsCta{756, 806, 408, 52};

constexpr Color kSky = rgba(15, 48, 72);
constexpr Color kStudioLow = rgba(8, 32, 53);
constexpr Color kShelfBg = rgba(9, 30, 48);
constexpr Color kGlass = rgba(11, 37, 56, 194);
constexpr Color kGlassLine = rgba(159, 220, 238, 66);
constexpr Color kText = rgba(238, 249, 255);
constexpr Color kMuted = rgba(180, 205, 220);
constexpr Color kAccent = rgba(125, 226, 249);
constexpr Color kSelect = rgba(177, 251, 243);

inline void glassPanel(Canvas& c, Rect r, int radius = 18) {
    c.fillRect(r, kGlass, radius);
    c.strokeRect(r, kGlassLine, 1, radius);
}

inline void shelfBackdrop(Canvas& c) {
    c.gradientV({0, kHeroShelfBoundary, 1920, kFooterTop - kHeroShelfBoundary},
                rgba(18, 61, 80), kShelfBg);
    c.line(84.0f, static_cast<float>(kHeroShelfBoundary),
           1836.0f, static_cast<float>(kHeroShelfBoundary),
           1.0f, rgba(179, 232, 245, 48));
}

inline void footerBackdrop(Canvas& c) {
    c.fillRect({0, kFooterTop, 1920, 1080 - kFooterTop},
               rgba(5, 23, 37, 246));
    c.line(0.0f, static_cast<float>(kFooterTop), 1920.0f,
           static_cast<float>(kFooterTop), 1.0f, rgba(149, 210, 232, 72));
}

} // namespace xc::ui::preview31
