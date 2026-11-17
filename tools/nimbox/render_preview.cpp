// SPDX-License-Identifier: GPL-3.0-or-later
// Offline preview of the Nimbox shelf: runs the same drawing code as the app
// (src/ui/nimbox_case_view.cpp) on synthetic artwork and writes a PPM, so a
// visual change can be looked at without a device.
//   render_preview <out.ppm> [selected=0] [slideMilli=0] [fontsDir=assets/fonts] [benchFrames=0]
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "platform/platform.h"
#include "ui/nimbox_case_view.h"
#include "util/log.h"

// --- minimal link stubs for the pieces of the app the drawing code touches ---
namespace xc::log { void write(Level, const char*, ...) {} }
namespace xc::platform {
bool readFile(const std::string& path, std::string& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    out.assign(std::istreambuf_iterator<char>(f), {});
    return true;
}
}  // namespace xc::platform
extern "C++" {
extern const unsigned char xc_builtin_font_Bold[] = {0};
extern const std::size_t xc_builtin_font_Bold_size = 0;
extern const unsigned char xc_builtin_font_Regular[] = {0};
extern const std::size_t xc_builtin_font_Regular_size = 0;
extern const unsigned char xc_builtin_font_SemiBold[] = {0};
extern const std::size_t xc_builtin_font_SemiBold_size = 0;
}

using namespace xc::ui;

static Image makeArt(int seed, int size) {
    Image img;
    img.w = img.h = size;
    img.opaque = true;
    img.px.resize(static_cast<size_t>(size) * size);
    const float h1 = std::fmod(seed * 47.0f, 360.0f), h2 = std::fmod(h1 + 70.0f, 360.0f);
    auto hsv = [](float h, float s, float v) {
        const float c = v * s, x = c * (1 - std::fabs(std::fmod(h / 60.0f, 2.0f) - 1)), m = v - c;
        float r = 0, g = 0, b = 0;
        if (h < 60) { r = c; g = x; } else if (h < 120) { r = x; g = c; }
        else if (h < 180) { g = c; b = x; } else if (h < 240) { g = x; b = c; }
        else if (h < 300) { r = x; b = c; } else { r = c; b = x; }
        return rgba(uint8_t((r + m) * 255), uint8_t((g + m) * 255), uint8_t((b + m) * 255));
    };
    const Color a = hsv(h1, 0.75f, 0.95f), b = hsv(h2, 0.85f, 0.45f);
    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x) {
            const float t = (x + y) / float(2 * size);
            auto mix = [&](int s) { return uint8_t(((a >> s) & 255) * (1 - t) + ((b >> s) & 255) * t); };
            Color c = rgba(mix(0), mix(8), mix(16));
            const float dx = x - size * 0.5f, dy = y - size * 0.42f;
            if (std::sqrt(dx * dx + dy * dy) < size * 0.27f) c = rgba(255, 255, 255, 255);
            if (y > size * 0.78f) c = rgba(uint8_t(((c) & 255) * 0.45f), uint8_t(((c >> 8) & 255) * 0.45f),
                                          uint8_t(((c >> 16) & 255) * 0.45f));
            img.px[static_cast<size_t>(y) * size + x] = c;
        }
    return img;
}

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "usage: render_preview out.ppm [selected] [slideMilli] [fontsDir]\n"); return 2; }
    const int selected0 = argc > 2 ? std::atoi(argv[2]) : 0;
    const float slide = argc > 3 ? static_cast<float>(std::atof(argv[3])) : 0.0f;
    const std::string fontsDir = argc > 4 ? argv[4] : "assets/fonts";

    Fonts fonts;
    fonts.load(fontsDir);
    const std::vector<std::string> names = {
        "Injustice 2", "Hollow Knight: Silksong", "Crash Bandicoot N. Sane", "Tom Clancy's Ghost Recon",
        "Stardew Valley", "Far Cry 5", "Fortnite", "Stumble Guys", "Call of Duty: Warzone",
        "Mortal Kombat 11", "Cyberpunk 2077", "Asphalt Legends", "Gang Beasts"};
    const int count = static_cast<int>(names.size());
    std::vector<Image> art;
    for (int i = 0; i < count; ++i) art.push_back(makeArt(i + 3, 512));
    const int selected = ((selected0 % count) + count) % count;

    Canvas c(1920, 1080);
    const int bench = argc > 5 ? std::atoi(argv[5]) : 0;
    const auto t0 = std::chrono::steady_clock::now();
    for (int frame = 0; frame <= bench; ++frame) {
    if (frame == 1) {
        const auto t1 = std::chrono::steady_clock::now();
        (void)t1;
    }
    nimbox::drawStage(c, rgba(66, 112, 136));

    std::vector<int> order;
    for (int d = -nimbox::kSideCases; d <= nimbox::kSideCases; ++d)
        if (nimbox::indexForOffset(selected, count, d) >= 0) order.push_back(d);
    const float shift = slide / 1000.0f;
    std::stable_sort(order.begin(), order.end(), [shift](int a, int b) {
        return std::fabs(a + shift) > std::fabs(b + shift);
    });
    for (int offset : order) {
        const int idx = nimbox::indexForOffset(selected, count, offset);
        nimbox::CaseArt ca;
        ca.name = names[static_cast<size_t>(idx)];
        ca.platformName = "XBOX SERIES";
        ca.xboxPlatform = true;
        ca.cover = &art[static_cast<size_t>(idx)];
        nimbox::drawCase(c, fonts, offset, slide, ca);
    }
    nimbox::drawFloorFade(c);
    const std::string& cur = names[static_cast<size_t>(selected)];
    const int tw = fonts.bold.measure(cur, 38);
    fonts.bold.draw(c, cur, (1920 - tw) / 2, 805, 38, rgba(255, 255, 255));
    const std::string meta = "Epic Games Inc.  -  Action & adventure";
    fonts.regular.draw(c, meta, (1920 - fonts.regular.measure(meta, 20)) / 2, 858, 20, rgba(170, 180, 190));
    }
    if (bench > 0) {
        const auto t2 = std::chrono::steady_clock::now();
        std::printf("%.2f ms/frame (incl. first frame's texture build)\n",
                    std::chrono::duration<double, std::milli>(t2 - t0).count() / (bench + 1));
    }

    std::FILE* f = std::fopen(argv[1], "wb");
    if (!f) return 1;
    std::fprintf(f, "P6\n%d %d\n255\n", c.width(), c.height());
    for (int i = 0; i < c.width() * c.height(); ++i) {
        const uint32_t p = c.data()[i];
        const unsigned char rgb[3] = {uint8_t(p), uint8_t(p >> 8), uint8_t(p >> 16)};
        std::fwrite(rgb, 1, 3, f);
    }
    std::fclose(f);
    return 0;
}
