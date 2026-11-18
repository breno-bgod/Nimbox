// SPDX-License-Identifier: GPL-3.0-or-later
// Nimbox experimental visual identity; the original project's GPL credit is preserved.
#include <cstddef>
#include <cstdint>
#include "ui/brand.h"

#include <algorithm>
#include <cmath>

namespace xc::ui {

// An original geometric N mark, replacing the PSBox swoosh icon in the
// experimental UI. Uses Canvas primitives so it works on every platform.
void drawBrandMark(Canvas& c, float cx, float cy, float r, Color color) {
    c.fillCircle(cx, cy, r, rgba(13, 32, 48));
    c.strokeCircle(cx, cy, r * 0.87f, std::max(1.0f, r * 0.075f), rgba(92, 212, 229));
    const float left = cx - r * 0.33f;
    const float right = cx + r * 0.33f;
    const float top = cy - r * 0.42f;
    const float bottom = cy + r * 0.42f;
    const float thickness = std::max(1.5f, r * 0.135f);
    c.line(left, bottom, left, top, thickness, color);
    c.line(left, top, right, bottom, thickness, color);
    c.line(right, bottom, right, top, thickness, color);
}

void drawAppIcon(Canvas& c, const Fonts& fonts) {
    const int s = c.width();
    c.gradientV({0, 0, s, s}, rgba(22, 71, 94), rgba(5, 18, 31));
    drawBrandMark(c, s * 0.5f, s * 0.39f, s * 0.27f);
    const char* word = "NIMBOX";
    const int px = s / 10;
    const int tw = fonts.bold.measure(word, px);
    fonts.bold.draw(c, word, (s - tw) / 2, static_cast<int>(s * 0.76f), px, rgba(236, 249, 253));
}

void drawHomeArt(Canvas& c, const Fonts& fonts, bool launch) {
    const float w = static_cast<float>(c.width()), h = static_cast<float>(c.height());
    const float cx = launch ? w * 0.5f : w * 0.70f;
    const float cy = launch ? h * 0.42f : h * 0.47f;
    const float r = h * (launch ? 0.17f : 0.22f);
    c.gradientH({0, 0, c.width(), c.height()}, rgba(5, 12, 22), rgba(12, 63, 80));
    for (int i = 28; i > 0; --i)
        c.fillCircle(cx, cy, r * (1.0f + i * 0.085f), rgba(70, 182, 214, 4));
    for (int i = 0; i < 9; ++i) {
        const float y0 = h * (0.08f + 0.11f * i);
        const float tilt = h * 0.22f;
        c.line(cx - w * 0.55f, y0 + tilt, cx + w * 0.55f, y0 - tilt,
               h * (0.002f + 0.0015f * (i % 3)),
               rgba(111, 213, 235, static_cast<uint8_t>(10 + 6 * (i % 3))));
    }
    c.gradientV({0, 0, c.width(), static_cast<int>(h * 0.25f)}, rgba(0, 0, 0, 120), rgba(0, 0, 0, 0));
    c.gradientV({0, static_cast<int>(h * 0.7f), c.width(), static_cast<int>(h * 0.3f) + 1},
                rgba(0, 0, 0, 0), rgba(0, 0, 0, 150));
    if (!launch)
        c.gradientH({0, 0, static_cast<int>(w * 0.45f), c.height()},
                    rgba(0, 0, 0, 140), rgba(0, 0, 0, 0));
    drawBrandMark(c, cx, cy, r);
    if (launch) {
        const char* name = "NIMBOX";
        const int px = static_cast<int>(h * 0.065f);
        const int tw = fonts.bold.measure(name, px);
        fonts.bold.draw(c, name, (c.width() - tw) / 2,
                        static_cast<int>(cy + r * 1.45f), px, rgba(255, 255, 255));
    }
}

}  // namespace xc::ui
