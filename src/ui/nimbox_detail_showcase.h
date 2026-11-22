// SPDX-License-Identifier: GPL-3.0-or-later
// Nimbox native console-style game detail page. Shared geometry for
// rendering, pointer hit testing and no-network UI selftests.
#pragma once
#include "ui/canvas.h"

namespace xc::ui::showcase {
inline constexpr Rect kFrame{64, 162, 1792, 762};
inline constexpr int kTextX = 120;
inline constexpr int kTextW = 950;
inline constexpr int kRailX = 1384;
inline constexpr int kRailW = 410;
inline constexpr Rect kAchievementsPanel{kRailX, 254, kRailW, 196};
inline constexpr Rect kSessionPanel{kRailX, 466, kRailW, 154};
inline constexpr Rect kArtworkPanel{kRailX, 637, kRailW, 241};
inline constexpr Rect kPlayButton{kTextX, 801, 310, 80};
inline constexpr Rect kAchievementsButton{452, 801, 312, 80};
inline constexpr Rect kGameProfileButton{kTextX,902,900,65};
inline constexpr Rect kClaimButton{kTextX, 801, 360, 80};
inline constexpr Rect kLockedNotice{kTextX, 801, 730, 80};

inline constexpr bool contains(Rect r, float x, float y) {
    return r.w > 0 && r.h > 0 &&
           x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}
} // namespace xc::ui::showcase
