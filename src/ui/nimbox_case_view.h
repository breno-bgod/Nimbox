// SPDX-License-Identifier: GPL-3.0-or-later
// Nimbox 3D Library — drawing of one game case and of the stage behind the
// shelf. Presentation only: no catalog, login, input or streaming state is
// read here. AppUi hands in what to draw, and the offline preview tool
// (tools/nimbox/render_preview.cpp) calls exactly the same functions.
#pragma once

#include <string>

#include "ui/box_carousel.h"
#include "ui/canvas.h"
#include "ui/font.h"

namespace xc::ui::nimbox {

struct CaseArt {
    const Image* cover = nullptr;  // square catalog artwork, may be null while loading
    std::string name;
    std::string platformName;      // printed on the case header ("XBOX SERIES")
    bool xboxPlatform = false;
};

// Draws the case at shelf position `offset` (0 = selected), shifted by the
// navigation animation `slide` (thousandths of a position, as AppUi keeps it).
void drawCase(Canvas& c, const Fonts& fonts, int offset, float slide, const CaseArt& art);

// Background of the shelf screen: colour wash from the selected artwork,
// ambient light and the floor falloff. `topShade` is the tinted top colour.
void drawStage(Canvas& c, Color topShade);

// Fade drawn over the reflections, after all the cases.
void drawFloorFade(Canvas& c);

}  // namespace xc::ui::nimbox
