// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/nimbox_case_view.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace xc::ui::nimbox {

namespace {

constexpr int kW = 1920;
constexpr Color kDeep = rgba(16, 34, 52);

// ---------------------------------------------------------------------------
// Case textures. The front, the side (with the title printed along it) and the
// floor reflection are composed once per game and size, then drawn as
// projected strips. Measures follow the approved HTML preview (286 x 389 case).
// ---------------------------------------------------------------------------
struct CaseTextures {
    Image front, side, reflect;
    uint64_t used = 0;
};

uint64_t fnv(uint64_t h, const void* data, size_t n) {
    const auto* p = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < n; ++i) { h ^= p[i]; h *= 1099511628211ull; }
    return h;
}

uint64_t keyFor(const CaseArt& art, int width, bool focused) {
    uint64_t h = 1469598103934665603ull;
    h = fnv(h, art.name.data(), art.name.size());
    h = fnv(h, art.platformName.data(), art.platformName.size());
    h = fnv(h, &width, sizeof width);
    h = fnv(h, &focused, sizeof focused);
    if (art.cover) {
        // The image cache may hand back a new image at the same address; a few
        // sampled pixels tell them apart.
        const Image& im = *art.cover;
        h = fnv(h, &im.w, sizeof im.w);
        h = fnv(h, &im.h, sizeof im.h);
        if (!im.px.empty()) {
            for (int i = 0; i < 16; ++i) {
                const uint32_t px = im.px[(im.px.size() - 1) * static_cast<size_t>(i) / 15];
                h = fnv(h, &px, sizeof px);
            }
        }
    } else {
        h ^= 0x9e3779b97f4a7c15ull;
    }
    return h;
}

Image toImage(const Canvas& c) {
    Image im;
    im.w = c.width();
    im.h = c.height();
    im.px.assign(c.data(), c.data() + static_cast<size_t>(im.w) * im.h);
    return im;
}

// Rotates 90 degrees counter-clockwise (text that read left to right now reads
// from the bottom to the top, as on a book spine).
Image rotateCcw(const Image& s) {
    Image r;
    r.w = s.h;
    r.h = s.w;
    r.px.assign(static_cast<size_t>(r.w) * r.h, 0);
    for (int y = 0; y < s.h; ++y)
        for (int x = 0; x < s.w; ++x)
            r.px[static_cast<size_t>(s.w - 1 - x) * r.w + y] = s.px[static_cast<size_t>(y) * s.w + x];
    return r;
}

Image composeFront(const Fonts& fonts, const CaseArt& art, int W, bool focused) {
    const float sc = static_cast<float>(W) / kCaseWidth;
    const int H = static_cast<int>(std::lround(kCaseHeight * sc));
    Canvas t(W, H);  // transparent
    auto S = [sc](float v) { return std::max(1, static_cast<int>(std::lround(v * sc))); };
    const int b = S(2), radius = S(4);
    t.fillRect({0, 0, W, H}, focused ? rgba(233, 255, 230) : rgba(180, 236, 206), radius);
    t.fillRect({b, b, W - 2 * b, H - 2 * b}, rgba(10, 38, 53), std::max(1, radius - b));

    // Header: green band with the platform mark and name.
    const int hh = S(43);
    t.gradientV({b, b, W - 2 * b, hh}, rgba(3, 137, 35), rgba(0, 106, 29));
    t.fillRect({b, b + hh - b, W - 2 * b, b}, rgba(116, 244, 132, 85));
    const float cy = b + (hh - b) * 0.5f, cx = b + S(12) + S(12);
    t.strokeCircle(cx, cy, S(12) - 1.0f * sc, 2.0f * sc, rgba(230, 255, 231));
    const float k = 4.5f * sc;
    t.line(cx - k, cy - k, cx + k, cy + k, 2.2f * sc, rgba(250, 255, 252));
    t.line(cx + k, cy - k, cx - k, cy + k, 2.2f * sc, rgba(250, 255, 252));
    const int px16 = std::max(8, static_cast<int>(std::lround(16 * sc)));
    fonts.bold.draw(t, art.platformName, static_cast<int>(cx) + S(12) + S(8),
                    fonts.bold.centeredY(b, hh - b, px16), px16, rgba(250, 255, 252));

    // Artwork, cropped to fill the area (the preview's background-size: cover).
    const int bottomH = S(10);
    const Rect area{b, b + hh, W - 2 * b, H - 2 * b - hh - bottomH};
    if (art.cover && art.cover->w > 0 && art.cover->h > 0) {
        const float imgAspect = static_cast<float>(art.cover->w) / art.cover->h;
        const float areaAspect = static_cast<float>(area.w) / area.h;
        float u0 = 0, u1 = 1, v0 = 0, v1 = 1;
        if (imgAspect > areaAspect) {
            const float keep = areaAspect / imgAspect;
            u0 = (1 - keep) * 0.5f; u1 = 1 - u0;
        } else {
            const float keep = imgAspect / areaAspect;
            v0 = (1 - keep) * 0.5f; v1 = 1 - v0;
        }
        const float ax = static_cast<float>(area.x), ay = static_cast<float>(area.y);
        const float ax1 = ax + area.w, ay1 = ay + area.h;
        t.drawImageQuadUV(*art.cover, {{ax, ay}, {ax1, ay}, {ax1, ay1}, {ax, ay1}}, u0, v0, u1, v1);
    } else {
        t.gradientV(area, rgba(58, 92, 108), rgba(34, 62, 76));
        const int px = std::max(10, static_cast<int>(std::lround(24 * sc)));
        const auto lines = fonts.bold.wrap(art.name, px, area.w - 2 * S(16), 4);
        const int lh = fonts.bold.lineHeight(px);
        int y = area.y + (area.h - lh * static_cast<int>(lines.size())) / 2;
        for (const auto& line : lines) {
            fonts.bold.draw(t, line, area.x + (area.w - fonts.bold.measure(line, px)) / 2, y, px,
                            rgba(235, 247, 245));
            y += lh;
        }
    }
    // Sheen across the cover (white on the left, shade on the right).
    const int edge = static_cast<int>(area.w * 0.23f);
    t.gradientH({area.x, area.y, edge, area.h}, rgba(255, 255, 255, 74), rgba(255, 255, 255, 0));
    t.gradientH({area.x + area.w - edge, area.y, edge, area.h}, rgba(0, 0, 0, 0), rgba(0, 0, 0, 112));
    // Thin base strip.
    t.fillRect({b, H - b - bottomH, W - 2 * b, bottomH}, rgba(12, 89, 44));
    t.fillRect({b, H - b - bottomH, W - 2 * b, std::max(1, S(1))}, rgba(149, 227, 186, 85));
    return toImage(t);
}

Image composeSide(const Fonts& fonts, const CaseArt& art, int W) {
    const float sc = static_cast<float>(W) / kCaseWidth;
    const int L = static_cast<int>(std::lround(kCaseHeight * sc));
    const int T = std::max(6, static_cast<int>(std::lround(kCaseThickness * sc)));
    Canvas s(T, L);
    const int half = T / 2;
    s.gradientH({0, 0, half, L}, rgba(0, 85, 27), rgba(11, 154, 72));
    s.gradientH({half, 0, T - half, L}, rgba(11, 154, 72), rgba(11, 90, 53));
    s.strokeRect({0, 0, T, L}, rgba(192, 234, 216, 210), std::max(1, static_cast<int>(std::lround(sc))));
    // Title along the spine, reading from the bottom to the top.
    const int px = std::max(8, static_cast<int>(std::lround(14 * sc)));
    const int maxLen = static_cast<int>(L * 0.80f);
    const std::string text = fonts.bold.fit(art.name, px, maxLen);
    const int tw = std::max(1, fonts.bold.measure(text, px));
    const int th = fonts.bold.lineHeight(px) + 2;
    Canvas line(tw + 2, th);
    fonts.bold.draw(line, text, 2, 2, px, rgba(0, 18, 14, 150));
    fonts.bold.draw(line, text, 1, 1, px, rgba(250, 255, 252));
    const Image rotated = rotateCcw(toImage(line));
    s.drawImage(rotated, (T - rotated.w) / 2, (L - rotated.h) / 2);
    return toImage(s);
}

Image composeReflection(const Image& front, bool focused) {
    const int H = front.h;
    const int Hr = std::max(2, static_cast<int>(std::lround(H * 154.0f / kCaseHeight)));
    Image r;
    r.w = front.w;
    r.h = Hr;
    r.px.assign(static_cast<size_t>(r.w) * r.h, 0);
    const float strength = focused ? 0.44f : 0.35f;
    for (int y = 0; y < Hr; ++y) {
        // Mirror of the bottom of the case, fading out with distance.
        const float fade = std::clamp(1.0f - static_cast<float>(y) / (0.94f * Hr), 0.0f, 1.0f);
        const float a = strength * fade;
        const uint32_t* src = &front.px[static_cast<size_t>(H - 1 - y) * front.w];
        uint32_t* dst = &r.px[static_cast<size_t>(y) * r.w];
        for (int x = 0; x < r.w; ++x) {
            const uint32_t p = src[x];
            const uint32_t alpha = static_cast<uint32_t>((p >> 24) * a);
            dst[x] = (p & 0x00FFFFFFu) | (alpha << 24);
        }
    }
    return r;
}

const CaseTextures& texturesFor(const Fonts& fonts, const CaseArt& art, int width, bool focused) {
    static std::mutex mutex;
    static std::unordered_map<uint64_t, CaseTextures> cache;
    static uint64_t tick = 0;
    std::lock_guard<std::mutex> lock(mutex);
    const uint64_t key = keyFor(art, width, focused);
    auto it = cache.find(key);
    if (it == cache.end()) {
        if (cache.size() >= 40) {
            auto oldest = cache.begin();
            for (auto i = cache.begin(); i != cache.end(); ++i)
                if (i->second.used < oldest->second.used) oldest = i;
            cache.erase(oldest);
        }
        CaseTextures t;
        t.front = composeFront(fonts, art, width, focused);
        t.side = composeSide(fonts, art, width);
        t.reflect = composeReflection(t.front, focused);
        it = cache.emplace(key, std::move(t)).first;
    }
    it->second.used = ++tick;
    return it->second;  // entries are only dropped by a later call on this thread
}

// A planar face drawn as `n` strips whose corners are projected one by one, so
// the texture keeps its perspective instead of shearing.
void drawFace(Canvas& c, const Image& tex, const Projector& pr, const Face3& f, int n,
              uint8_t opacity = 255, uint8_t shade = 255) {
    for (int i = 0; i < n; ++i) {
        const float t0 = static_cast<float>(i) / n, t1 = static_cast<float>(i + 1) / n;
        const Quad q{pr.project(lerp3(f.tl, f.tr, t0)), pr.project(lerp3(f.tl, f.tr, t1)),
                     pr.project(lerp3(f.bl, f.br, t1)), pr.project(lerp3(f.bl, f.br, t0))};
        c.drawImageQuadUV(tex, q, t0, 0.0f, t1, 1.0f, opacity, shade);
    }
}

Quad shifted(const Quad& q, float dx, float dy) {
    return {{q.tl.x + dx, q.tl.y + dy}, {q.tr.x + dx, q.tr.y + dy},
            {q.br.x + dx, q.br.y + dy}, {q.bl.x + dx, q.bl.y + dy}};
}

}  // namespace

void drawStage(Canvas& c, Color topShade) {
    c.clear(kDeep);
    c.gradientV({0, 136, kW, 700}, topShade, kDeep);
    // Lighter infinity-room atmosphere; the cases remain the main focus.
    c.gradientV({0, 380, kW, 310}, rgba(108, 163, 190, 42), rgba(23, 49, 71, 33));
    c.gradientH({0, 220, kW, 490}, rgba(11, 30, 43, 110), rgba(15, 70, 75, 60));
    // No hard horizontal shelf line. Subtle ambient falloff behind reflected art.
    c.gradientV({0, 695, kW, 240}, rgba(79, 133, 165, 45), rgba(11, 30, 51, 95));
}

void drawFloorFade(Canvas& c) {
    c.gradientV({0, 738, kW, 150}, rgba(17, 39, 60, 0), rgba(17, 39, 60, 174));
}

void drawCase(Canvas& c, const Fonts& fonts, int offset, float slide, const CaseArt& art) {
    const CaseGeometry g = layoutAnimated(static_cast<float>(offset) + slide / 1000.0f);
    const int d = std::abs(offset);
    const bool focused = offset == 0;
    const int width = focused ? 344 : (d <= 2 ? 258 : 172);
    const CaseTextures& tex = texturesFor(fonts, art, width, focused);

    // Light and shadow around the case.
    if (focused) {
        const Rect r{static_cast<int>(g.x), static_cast<int>(g.y), static_cast<int>(g.w),
                     static_cast<int>(g.h)};
        for (int i = 1; i <= 7; ++i) {  // soft cold shadow below
            const int grow = i * 4;
            c.strokeRect({r.x - grow + 4, r.y - grow + 20, r.w + 2 * grow - 8, r.h + 2 * grow - 8},
                         rgba(6, 49, 75, static_cast<uint8_t>(34 - i * 4)), 4, 10 + grow);
        }
        for (int i = 1; i <= 8; ++i) {  // cyan glow
            const int grow = i * 2;
            c.strokeRect({r.x - grow, r.y - grow, r.w + 2 * grow, r.h + 2 * grow},
                         rgba(97, 238, 255, static_cast<uint8_t>(30 - i * 3)), 3, 4 + grow);
        }
    } else {
        c.fillQuad(shifted(g.front, 8, 18), rgba(8, 30, 44, 52));
    }

    // Floor reflection first, so the case stands on top of it.
    if (d <= 2 && art.cover) {
        const float top = kHalfH + 10.0f, bottom = top + 154.0f;
        const Face3 mirror{{-kHalfW, top, 0}, {kHalfW, top, 0}, {kHalfW, bottom, 0}, {-kHalfW, bottom, 0}};
        drawFace(c, tex.reflect, g.proj, mirror, 4,
                 focused ? 255 : static_cast<uint8_t>(d == 1 ? 215 : 150));
    }

    // Slightly dimmer with distance (the preview's brightness filter).
    const uint8_t shade = d > 1 ? static_cast<uint8_t>(std::max(160, 232 - (d - 2) * 14)) : 255;
    if (g.sideVisible) drawFace(c, tex.side, g.proj, sideFace(g.sideLeft), 4, 255, shade);
    drawFace(c, tex.front, g.proj, frontFace(), 14, 255, shade);
}

}  // namespace xc::ui::nimbox
