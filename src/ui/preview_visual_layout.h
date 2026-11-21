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
// Deprecated redundant Home CTA: an empty rectangle intentionally disables
// its legacy pointer hitbox without touching input handlers. The projected
// physical case is the ONLY Details entry point from the carousel.
constexpr Rect kDetailsCta{0, 0, 0, 0};

constexpr Color kSky = rgba(70, 61, 55);
constexpr Color kStudioLow = rgba(23, 22, 25);
constexpr Color kShelfBg = rgba(27, 26, 29);
constexpr Color kGlass = rgba(39, 37, 39, 240);
constexpr Color kGlassLine = rgba(233, 220, 195, 55);
constexpr Color kText = rgba(249, 246, 238);
constexpr Color kMuted = rgba(203, 197, 186);
constexpr Color kAccent = rgba(244, 194, 125);
constexpr Color kSelect = rgba(243, 230, 208);

inline void glassPanel(Canvas& c, Rect r, int radius = 18) {
    c.fillRect(r, kGlass, radius);
    c.strokeRect(r, kGlassLine, 1, radius);
}

inline void shelfBackdrop(Canvas& c) {
    c.gradientV({0, kHeroShelfBoundary, 1920, kFooterTop - kHeroShelfBoundary},
                kStudioLow, kShelfBg);
    c.line(84.0f, static_cast<float>(kHeroShelfBoundary),
           1836.0f, static_cast<float>(kHeroShelfBoundary),
           1.0f, kGlassLine);
}

inline void footerBackdrop(Canvas& c) {
    c.fillRect({0, kFooterTop, 1920, 1080 - kFooterTop},
               withAlpha(kStudioLow, 246));
    c.line(0.0f, static_cast<float>(kFooterTop), 1920.0f,
           static_cast<float>(kFooterTop), 1.0f, kGlassLine);
}

} // namespace xc::ui::preview31
