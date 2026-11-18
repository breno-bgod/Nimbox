// SPDX-License-Identifier: GPL-3.0-or-later
// NimboPlay internal visual identity; original project GPL credit preserved.
#include "ui/brand.h"
#include "ui/image_cache.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <mutex>
#include <string>
#include <utility>

// Generated from user-provided internal logo by cmake/embed_file.cmake.
// External OS launcher and PS5 shell artwork remain separate.
extern const unsigned char xc_internal_nimboplay_brand[];
extern const std::size_t xc_internal_nimboplay_brand_size;
// Separate, approved EXTERNAL square launcher icon.
extern const unsigned char xc_nimboplay_external_launcher[];
extern const std::size_t xc_nimboplay_external_launcher_size;

namespace xc::ui {

// Decode the bundled transparent wordmark only once. Each requested display
// size is fitted and cached so there is no repeated PNG decoding per frame.
void drawInternalBrand(Canvas& c, Rect bounds) {
    if (bounds.w <= 0 || bounds.h <= 0) return;
    static const Image source = [] {
        Image logo;
        if (xc_internal_nimboplay_brand_size > 1) {
            const std::string bytes(reinterpret_cast<const char*>(xc_internal_nimboplay_brand),
                                    xc_internal_nimboplay_brand_size - 1);
            decodeImage(bytes, logo);
        }
        return logo;
    }();
    if (source.w <= 0 || source.h <= 0) return;
    const double aspect = static_cast<double>(source.w) / source.h;
    const int w = std::max(1, std::min(bounds.w, static_cast<int>(bounds.h * aspect + 0.5)));
    const int h = std::max(1, std::min(bounds.h, static_cast<int>(w / aspect + 0.5)));
    const int x = bounds.x + (bounds.w - w) / 2;
    const int y = bounds.y + (bounds.h - h) / 2;

    static std::mutex mutex;
    static std::map<std::pair<int, int>, Image> scaled;
    std::lock_guard<std::mutex> lock(mutex);
    const auto key = std::make_pair(w, h);
    auto it = scaled.find(key);
    if (it == scaled.end())
        it = scaled.emplace(key, coverResize(source, w, h)).first;
    c.drawImage(it->second, x, y, 255, 0);
}

void drawInternalBrand(Canvas& c, int x, int y, int w, int h) {
    drawInternalBrand(c, Rect{x, y, w, h});
}

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

// Launcher image in the binary, independent of the in-app cloud wordmark.
static const Image& externalLauncherSource() {
    static const Image source = [] {
        Image icon;
        if (xc_nimboplay_external_launcher_size > 1) {
            const std::string bytes(
                reinterpret_cast<const char*>(xc_nimboplay_external_launcher),
                xc_nimboplay_external_launcher_size - 1);
            decodeImage(bytes, icon);
        }
        return icon;
    }();
    return source;
}

void drawAppIcon(Canvas& c, const Fonts& fonts) {
    const Image& source = externalLauncherSource();
    if (source.w > 0 && source.h > 0) {
        // CLI render-icon uses this C++ code for every requested output size.
        const Image fitted = coverResize(source, c.width(), c.height());
        c.drawImage(fitted, 0, 0);
        return;
    }
    // Rendering fallback only; CI tests fail if the asset is missing.
    const int s = c.width();
    c.gradientV({0, 0, s, s}, rgba(22, 71, 94), rgba(5, 18, 31));
    drawBrandMark(c, s * 0.5f, s * 0.5f, s * 0.27f);
    (void)fonts;
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
    // PS5 home art uses the same approved external square icon.
    // The icon already contains the wordmark, so no duplicate title is drawn.
    const auto& launcher = externalLauncherSource();
    if (launcher.w > 0 && launcher.h > 0) {
        const int size = std::max(1, static_cast<int>(r * 2.0f));
        const Image fitted = coverResize(launcher, size, size);
        c.drawImage(fitted, static_cast<int>(cx - size * 0.5f),
                    static_cast<int>(cy - size * 0.5f), 255, 40);
    } else {
        drawBrandMark(c, cx, cy, r);
    }
    (void)fonts;
}

}  // namespace xc::ui
