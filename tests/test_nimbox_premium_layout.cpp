// SPDX-License-Identifier: GPL-3.0-or-later
// Standalone PS5 Horizon geometry regression test:
//   c++ -std=c++20 -Isrc tests/test_nimbox_premium_layout.cpp -o /tmp/nimbox_premium_layout
//   /tmp/nimbox_premium_layout
#include "ui/nimbox_horizon_layout.h"
#include <cassert>

int main() {
    namespace h = xc::ui::horizon;
    // Static shelf layout respects a 96px horizontal 1080p TV safe area.
    for (int off = -h::kSideItems; off <= h::kSideItems; ++off) {
        const xc::ui::Rect r = h::tileRect(float(off));
        assert(r.x >= 96);
        assert(r.x + r.w <= 1920 - 96);
        assert(r.y >= 600);
        assert(r.y + r.h <= 920);
    }
    // Hero CTAs remain separate and entirely above the selected cover.
    const auto cover = h::tileRect(0.0f);
    assert(h::kPlayButton.x + h::kPlayButton.w < h::kDetailsButton.x);
    assert(h::kPlayButton.y + h::kPlayButton.h < cover.y);
    assert(h::kDetailsButton.y + h::kDetailsButton.h < cover.y);
    assert(h::contains(h::kPlayButton, h::kPlayButton.x + 5, h::kPlayButton.y + 5));
    assert(!h::contains(h::kPlayButton, h::kDetailsButton.x + 5, h::kDetailsButton.y + 5));
    // Profile surface stays within safe area and doesn't block the gear.
    assert(h::kProfilePanel.x >= 96);
    assert(h::kProfilePanel.x + h::kProfilePanel.w <= 1920 - 96);
    assert(h::contains(h::kProfilePanel, h::kProfileClose.x + 8, h::kProfileClose.y + 8));
    assert(h::kProfileAvatarTap.x + h::kProfileAvatarTap.w < 1834);

    // A first push at a carousel edge does not wrap automatically.
    int selection = 0;
    h::BoundaryGate gate;
    assert(h::navigate(selection, 10, -1, 1000, gate) == h::MoveResult::Boundary);
    assert(selection == 0);
    gate.noteNeutral();
    assert(h::navigate(selection, 10, -1, 1400, gate) == h::MoveResult::Wrapped);
    assert(selection == 9);
    assert(h::indexForOffset(0, 10, -1) == -1);
    assert(h::indexForOffset(9, 10, +1) == -1);

    const auto order = h::drawOrder(4, 10, 0.0f);
    assert(!order.empty() && order.back() == 0); // selected tile is drawn on top
    return 0;
}
