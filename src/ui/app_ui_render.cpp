// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 RafaelNGP
#include <cstddef>
#include <cstdint>
#include "ui/app_ui_internal.h"
#include "ui/cloud_time.h"
#include "ui/nimbox_horizon_layout.h"
#include "ui/nimbox_detail_showcase.h"

#include <algorithm>

namespace xc::ui {

void AppUi::drawPads(Canvas& c) {
    // Bottom left, on the line of the button hints (which are right-aligned).
    drawPadRow(c, fonts_.bold, kMargin, kH - 26 - padIconHeight(48), 48, 16, pads_, kBg,
               fonts_.hasIcons ? &fonts_.icons : nullptr);
}

bool AppUi::needsRedraw(uint64_t nowMs) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (screen_ == Screen::Streaming) return false;
    if (dirty_ || animating_) return true;
    if (mouseCursorVisible_ && nowMs >= mouseCursorVisibleUntil_) return true;
    // A desktop's toast came or went (the PS5 sends system notifications).
    if (platform::toastsEnabled() && platform::toasts().version(nowMs) != toastsDrawn_) return true;
    // Spinners keep moving (about 30 fps is enough).
    bool spinner = screen_ == Screen::Splash || screen_ == Screen::SignIn || screen_ == Screen::Launching ||
                   screen_ == Screen::Updating ||
                   (screen_ == Screen::Settings && ((tester_ == Tester::SignInCheck && signInCheck_.running) ||
                                                    (tester_ == Tester::NetworkTest && networkTest_.running)));
    return spinner && nowMs - lastRender_ >= 33;
}

void AppUi::drawCentered(Canvas& c, const Font& f, const std::string& text, int y, int px, Color color) {
    f.draw(c, text, (kW - f.measure(text, px)) / 2, y, px, color);
}

void AppUi::drawBackground(Canvas& c) {
    c.gradientV({0, 0, kW, kH}, preview31::kStudioLow, kBg);
    c.gradientH({0, 0, kW, 520}, rgba(134, 98, 63, 30), withAlpha(kBg, 0));
}

void AppUi::drawLogo(Canvas& c, float cx, float cy, float r) {
    const int h = std::max(1, static_cast<int>(r * 2.0f));
    const int w = std::max(1, static_cast<int>(r * 3.7f));
    drawInternalBrand(c, static_cast<int>(cx - w * 0.5f),
                      static_cast<int>(cy - h * 0.5f), w, h);
}

void AppUi::drawGamerscoreBadge(Canvas& c, float cx, float cy, float r) {
    // A filled disc with a "G" cut out of it, drawn from shapes (a letter of
    // the font never sits in the middle of a circle): a ring open at the
    // upper right, and its bar from the centre to the ring.
    c.fillCircle(cx, cy, r, kGray);
    const float ring = r * 0.56f, t = r * 0.24f;
    const float kGap = 0.95f;  // radians left open, above 3 o'clock
    c.strokeArc(cx, cy, ring, t, 0.0f, 6.2832f - kGap, kBg);
    c.line(cx + r * 0.05f, cy, cx + ring + t * 0.5f, cy, t, kBg);
}

// The account's cloud time (Xbox limits it each month since November 2026),
// under the brand name: the left side, the account's own (name, score, plan)
// being on the right. What is left of the month's hours and a bar; orange
// from 2 h left, red at none. An unlimited account says so; nothing when it
// is not known.
void AppUi::drawCloudTime(Canvas& c, int x, int y) {
#if defined(__ANDROID__)
    constexpr int kPx = 22, kLineH = 28;
    constexpr float kClockR = 10.0f;
    constexpr int kBarH = 6;
    constexpr int kPad = 26;
#else
    constexpr int kPx = 15, kLineH = 20;
    constexpr float kClockR = 7.0f;
    constexpr int kBarH = 4;
    constexpr int kPad = 20;
#endif
    const int ty = fonts_.semibold.centeredY(y, kLineH, kPx);
    const float iconY = y + kLineH / 2.0f;
    // A free account plays in sessions of a fixed length: "unlimited" would be wrong for it.
    if (freeTierSessionSeconds_ > 0 && isFreeAccountLocked()) {
        drawClockIcon(c, x + kClockR, iconY, kClockR, kGray);
        fonts_.semibold.draw(c, trf(Str::CloudTimeFreeTier, std::to_string(freeTierSessionSeconds_ / 60)), x + kPad, ty, kPx, kGray);
        return;
    }
    if (playtimeUnlimited_) {
        drawClockIcon(c, x + kClockR, iconY, kClockR, kGray);
        fonts_.semibold.draw(c, tr(Str::CloudTimeUnlimited), x + kPad, ty, kPx, kGray);
        return;
    }
    if (playtimeRemaining_ < 0) return;
    const Color col = playtimeRemaining_ <= 0      ? rgba(240, 100, 100)
                      : playtimeRemaining_ < 7260 ? rgba(255, 160, 40)
                                                   : kGray;
    drawClockIcon(c, x + kClockR, iconY, kClockR, col);
    int end = fonts_.semibold.draw(c, trf(Str::CloudTimeLeft, formatCloudTime(playtimeRemaining_)), x + kPad, ty, kPx, col);
    if (playtimeMax_ <= 0) return;
    end = fonts_.semibold.draw(c, " " + trf(Str::CloudTimeOf, formatCloudTime(playtimeMax_)), end, ty, kPx, kDim);
    // As wide as the brand name, or the line when that is longer.
#if defined(__ANDROID__)
    constexpr int kBrandPx = 30;
#else
    constexpr int kBrandPx = 22;
#endif
    const int w = std::max(fonts_.semibold.measure("NIMBOPLAY", kBrandPx), end - x);
    const Rect bar{x, y + kLineH + 2, w, kBarH};
    c.fillRect(bar, rgba(255, 255, 255, 36), 2);
    const double f = std::clamp(static_cast<double>(playtimeRemaining_) / static_cast<double>(playtimeMax_), 0.0, 1.0);
    if (const int fw = static_cast<int>(w * f + 0.5); fw > 0) c.fillRect({x, bar.y, std::max(fw, 4), bar.h}, col, 2);
}

void AppUi::drawTopBar(Canvas& c, bool overArt) {
    if (!overArt) {
        c.fillRect({0, 0, kW, 122}, withAlpha(kBg, 231));
        c.line(50, 121, kW - 50, 121, 1, preview31::kGlassLine);
    }
    const bool hasCloudTime = playtimeUnlimited_ || playtimeRemaining_ >= 0;
#if defined(__ANDROID__)
    constexpr int kAvatar = 64;
    constexpr int kGamertagPx = 30;
    constexpr int kSubPlanPx = 16;
    constexpr int kSubBoxH = 24;
    constexpr int kSubGpPx = 18;
    constexpr int kLineY = 74, kLineH = 28, kLinePx = 22;
    constexpr float kBadgeR = 11.5f;
    constexpr int kBadgeMargin = 16;
#else
    constexpr int kAvatar = 52;
    constexpr int kGamertagPx = 22;
    constexpr int kSubPlanPx = 11;
    constexpr int kSubBoxH = 16;
    constexpr int kSubGpPx = 14;
    constexpr int kLineY = 76, kLineH = 22, kLinePx = 17;
    constexpr float kBadgeR = 8.5f;
    constexpr int kBadgeMargin = 13;
#endif

    // NimboPlay's complete transparent wordmark is the primary header brand.
    // No extra text beside it: the lettering is already included in the image.
    drawInternalBrand(c, 91, 20, 184, 100);
    if (hasCloudTime && !overArt) drawCloudTime(c, 292, 59);

    // Avatar and centered Settings touch button
    constexpr int kAvatarRight = kW - 120;
    int x = kAvatarRight - kAvatar;
    int avatarY = 72 - kAvatar / 2;
    if (auto pic = images_.get(gamerpicUrl_, kAvatar, kAvatar)) {
        c.drawImage(*pic, x, avatarY, 255, -1);
    } else {
        c.fillCircle(x + kAvatar / 2.0f, 72, kAvatar / 2.0f, kPanel);
    }
    // A deliberate, subtle affordance: the avatar is now a real action on Home.
    if (screen_ == Screen::Home && !searching_) {
        c.strokeCircle(x + kAvatar / 2.0f, 72.0f, kAvatar / 2.0f + 3.0f,
                       2.0f, rgba(182, 225, 242, 145));
    }

    // Dedicated Settings touch button, perfectly centered between avatar and screen right edge
    constexpr int kBtnSz = 52;
    Rect settingsBtn{kAvatarRight + (120 - kBtnSz) / 2, 46, kBtnSz, kBtnSz};
    if (!overArt) {
        c.fillRect(settingsBtn, rgba(255, 255, 255, 18), 26);
        c.strokeRect(settingsBtn, rgba(255, 255, 255, 45), 1, 26);
    }
    float scx = settingsBtn.x + 26.0f, scy = settingsBtn.y + 26.0f;
    for (int a = 0; a < 6; ++a) {
        float angle = a * (3.14159265f / 3.0f);
        float ca = std::cos(angle), sa = std::sin(angle);
        c.line(scx + 7.0f * ca, scy + 7.0f * sa, scx + 14.0f * ca, scy + 14.0f * sa, 3.5f, kWhite);
    }
    c.strokeCircle(scx, scy, 9.5f, 3.5f, kWhite);
    c.fillCircle(scx, scy, 4.0f, rgba(20, 20, 20));
    if (overArt) {
        drawSocialButtons(c);
        const auto name = fonts_.semibold.fit(gamertag_, kGamertagPx, 250);
        fonts_.semibold.draw(c, name, x - 16 - fonts_.semibold.measure(name, kGamertagPx),
                             fonts_.semibold.centeredY(46, 52, kGamertagPx), kGamertagPx, kWhite);
        return;
    }
    if (!gamertag_.empty()) {
        // The name; under it the gamerscore (its badge and the number, the
        // thousands grouped as the language writes them) and the Game Pass
        // plan, right-aligned to the picture as the Xbox app shows them.
        bool isFree = subscription_.empty() || (subscription_ != "GamePass" && xcloud::planTier(subscription_) == 0);
        bool second = !gamerscore_.empty() || !subscription_.empty() || isFree;
        int w = fonts_.semibold.measure(gamertag_, kGamertagPx);
        fonts_.semibold.draw(c, gamertag_, x - 16 - w, second ? (kGamertagPx == 30 ? 38 : 44) : 59, kGamertagPx, kWhite);
        int right = x - 16;
        const float midY = kLineY + kLineH / 2.0f;
        if (!subscription_.empty()) {
            // "GAME PASS", and the plan in an outlined box when it is known
            // ("GamePass": a member, the plan not known).
            int left = right;
            if (subscription_ != "GamePass") {
                std::string plan = subscription_;
                for (auto& ch : plan) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
                int pw = fonts_.bold.measure(plan, kSubPlanPx);
                Rect box{right - pw - 10, static_cast<int>(midY) - kSubBoxH / 2, pw + 10, kSubBoxH};
                c.strokeRect(box, kGray, 1, 2);
                fonts_.bold.draw(c, plan, box.x + 5, fonts_.bold.centeredY(box.y, box.h, kSubPlanPx), kSubPlanPx, kGray);
                left = box.x - 6;
            }
            const char* gp = "GAME PASS";
            int gw = fonts_.semibold.measure(gp, kSubGpPx);
            fonts_.semibold.draw(c, gp, left - gw, fonts_.semibold.centeredY(kLineY, kLineH, kSubGpPx), kSubGpPx, kGray);
            right = left - gw - 16;
        } else if (isFree) {
            // "XBOX GAME PASS" and FREE in its box, as a plan's name is shown.
            const char* freeLabel = "FREE";
            int fw = fonts_.bold.measure(freeLabel, kSubPlanPx);
            Rect box{right - fw - 12, static_cast<int>(midY) - kSubBoxH / 2, fw + 12, kSubBoxH};
            c.fillRect(box, rgba(255, 255, 255, 20), 4);
            c.strokeRect(box, rgba(160, 160, 175, 180), 1, 4);
            fonts_.bold.draw(c, freeLabel, box.x + 6, fonts_.bold.centeredY(box.y, box.h, kSubPlanPx), kSubPlanPx, rgba(220, 225, 235, 255));
            const char* gp = "XBOX GAME PASS";
            int gw = fonts_.semibold.measure(gp, kSubGpPx);
            fonts_.semibold.draw(c, gp, box.x - 6 - gw, fonts_.semibold.centeredY(kLineY, kLineH, kSubGpPx), kSubGpPx, kGray);
            right = box.x - 6 - gw - 16;
        }
        if (!gamerscore_.empty()) {
            std::string n;
            const char* sep = language() == Language::English ? "," : ".";
            for (size_t i = 0; i < gamerscore_.size(); ++i) {
                if (i && (gamerscore_.size() - i) % 3 == 0) n += sep;
                n += gamerscore_[i];
            }
            int nw = fonts_.semibold.measure(n, kLinePx);
            int nx = right - nw;
            fonts_.semibold.draw(c, n, nx, fonts_.semibold.centeredY(kLineY, kLineH, kLinePx), kLinePx, kGray);
            drawGamerscoreBadge(c, nx - kBadgeMargin, midY, kBadgeR);
            right = nx - static_cast<int>(kBadgeR * 2.0f + 9.0f);
        }
    }
}

void AppUi::drawHints(Canvas& c, const std::vector<std::pair<int, const char*>>& hints) {
    if (!input::hasGamepad() && !g_keyboardHints) return;
    // Right-aligned PlayStation button glyphs with labels.
    constexpr int kPx = 22, kR = 14, kGap = 36;
    int x = kW - kMargin;
    const int y = kH - 64;
    for (auto it = hints.rbegin(); it != hints.rend(); ++it) {
        int itemRightX = x;
        // With Circle confirming, the pad reports it as Cross (and the
        // other way round): the hints show the button to press.
        int icon = it->first;
        int w = fonts_.semibold.measure(it->second, kPx);
        x -= w;
        fonts_.semibold.draw(c, it->second, x, y, kPx, kGray);
        if (g_keyboardHints) {  // the keyboard's key, as a key cap
            const char* key = keyLabel(icon);
            int kw = fonts_.bold.measure(key, 15) + 18;
            Rect cap{x - 10 - kw, y + 1, kw, 26};
            c.fillRect(cap, rgba(255, 255, 255, 30), 6);
            c.strokeRect(cap, kGray, 2, 6);
            fonts_.bold.draw(c, key, cap.x + 9, fonts_.bold.centeredY(cap.y, cap.h, 15), 15, kWhite);
            hintRects_.push_back({it->first, Rect{cap.x - 12, y - 16, itemRightX - cap.x + 24, 60}});
            x = cap.x - kGap;
            continue;
        }
        if (circleConfirms_ && (icon == kIconCross || icon == kIconCircle))
            icon = icon == kIconCross ? kIconCircle : kIconCross;
        x -= 10 + 2 * kR;
        float cx = x + kR, cy = y + 13;
        const GlyphStyle style = g_glyphStyle;
        const bool xbox = style == GlyphStyle::Xbox;
        if (icon == kIconL2R2 || icon == kIconL1R1) {
            const char* l = buttonName(style, icon == kIconL1R1 ? PadButton::L1 : PadButton::L2);
            const char* r = buttonName(style, icon == kIconL1R1 ? PadButton::R1 : PadButton::R2);
            int lw = fonts_.bold.measure(l, 14) + 16;
            int rw = fonts_.bold.measure(r, 14) + 16;
            int sepW = fonts_.regular.measure("|", 14);
            constexpr int kPillGap = 6;
            int rightX = x + 2 * kR;
            Rect rPill{rightX - rw, static_cast<int>(cy) - 12, rw, 24};
            c.fillRect(rPill, rgba(255, 255, 255, 40), 8);
            fonts_.bold.draw(c, r, rPill.x + (rw - fonts_.bold.measure(r, 14)) / 2,
                             fonts_.bold.centeredY(rPill.y, rPill.h, 14), 14, kGray);
            int sepX = rPill.x - kPillGap - sepW;
            fonts_.regular.draw(c, "|", sepX, fonts_.regular.centeredY(static_cast<int>(cy) - 12, 24, 14), 14,
                                rgba(255, 255, 255, 120));
            Rect lPill{sepX - kPillGap - lw, static_cast<int>(cy) - 12, lw, 24};
            c.fillRect(lPill, rgba(255, 255, 255, 40), 8);
            fonts_.bold.draw(c, l, lPill.x + (lw - fonts_.bold.measure(l, 14)) / 2,
                             fonts_.bold.centeredY(lPill.y, lPill.h, 14), 14, kGray);
            x = lPill.x;
        } else if (icon == kIconDpadLR) {
            const char* l = "\xE2\x86\x90";
            const char* r = "\xE2\x86\x92";
            int lw = fonts_.bold.measure(l, 14) + 14;
            int rw = fonts_.bold.measure(r, 14) + 14;
            constexpr int kPillGap = 4;
            int rightX = x + 2 * kR;
            Rect rPill{rightX - rw, static_cast<int>(cy) - 12, rw, 24};
            c.fillRect(rPill, rgba(255, 255, 255, 40), 8);
            fonts_.bold.draw(c, r, rPill.x + (rw - fonts_.bold.measure(r, 14)) / 2,
                             fonts_.bold.centeredY(rPill.y, rPill.h, 14), 14, kGray);
            Rect lPill{rPill.x - kPillGap - lw, static_cast<int>(cy) - 12, lw, 24};
            c.fillRect(lPill, rgba(255, 255, 255, 40), 8);
            fonts_.bold.draw(c, l, lPill.x + (lw - fonts_.bold.measure(l, 14)) / 2,
                             fonts_.bold.centeredY(lPill.y, lPill.h, 14), 14, kGray);
            x = lPill.x;
        } else if (icon == kIconR3 || icon == kIconL3) {
            const char* label = buttonName(style, icon == kIconR3 ? PadButton::R3 : PadButton::L3);
            int pillW = fonts_.bold.measure(label, 14) + 16;
            int rightX = x + 2 * kR;
            Rect pill{rightX - pillW, static_cast<int>(cy) - 12, pillW, 24};
            c.fillRect(pill, rgba(255, 255, 255, 40), 8);
            fonts_.bold.draw(c, label, pill.x + (pillW - fonts_.bold.measure(label, 14)) / 2,
                             fonts_.bold.centeredY(pill.y, pill.h, 14), 14, kGray);
            x = pill.x;
        } else if (xbox && icon == kIconTouchpad) {  // View
            drawXboxViewButton(c, cx, cy, kR, kGray);
        } else if (xbox && icon == kIconOptions) {  // Menu
            drawXboxMenuButton(c, cx, cy, kR, kGray);
        } else if (icon == kIconTouchpad) {
            Rect pad{x - 18, static_cast<int>(cy) - 10, 2 * kR + 12, 20};
            c.fillRect(pad, rgba(255, 255, 255, 40), 6);
            c.strokeRect(pad, kGray, 2, 6);
            x -= 10;
        } else if (icon == kIconOptions) {
            c.fillRect({x - 4, static_cast<int>(cy) - 11, 2 * kR + 8, 22}, rgba(255, 255, 255, 40), 11);
            for (int k = -1; k <= 1; ++k) c.line(cx - 8, cy + k * 5, cx + 8, cy + k * 5, 2, kGray);
        } else {
            drawFaceButton(c, fonts_.bold, icon, cx, cy, kR);
        }
        hintRects_.push_back({it->first, Rect{x - 12, y - 16, itemRightX - x + 24, 60}});
        x -= kGap;
    }
}

void AppUi::drawSpinner(Canvas& c, float cx, float cy, float r, uint64_t nowMs) {
    float t = static_cast<float>(nowMs % 1000) / 1000.0f * 6.2832f;
    c.strokeArc(cx, cy, r, r * 0.18f, t, 4.4f, kWhite, /*fadeTail=*/true);
}

void AppUi::drawToast(Canvas& c, uint64_t nowMs) {
    if (toast_.empty()) return;
    if (toastUntil_ == 0) toastUntil_ = nowMs + 3500;
    if (nowMs >= toastUntil_) {
        toast_.clear();
        return;
    }
    int w = fonts_.semibold.measure(toast_, 24) + 64;
    Rect r{(kW - w) / 2, kH - 150, w, 56};
    c.fillRect(r, rgba(40, 40, 40, 235), 28);
    int ty = fonts_.semibold.centeredY(r.y, r.h, 24);
    fonts_.semibold.draw(c, toast_, r.x + 32, ty, 24, kWhite);
    animating_ = true;
}

bool AppUi::drawHero(Canvas& c, uint64_t nowMs, const std::string& url, bool details, bool fullScreen) {
    if (url != heroUrl_) {
        // The previous art may only cross-fade into a new one that is already
        // there; it is never kept up while the new one downloads.
        prevHeroUrl_ = heroShownAt_ ? heroUrl_ : std::string();
        heroUrl_ = url;
        heroSince_ = nowMs;
        heroShownAt_ = 0;
    }
    auto current = heroUrl_.empty() ? nullptr : images_.get(heroUrl_, kW, kH, fullScreen);
    if (current) {
        if (!heroShownAt_) heroShownAt_ = nowMs;
        uint64_t age = nowMs - heroShownAt_;
        // Arrived at once (cached): cross-fade from the previous art. Arrived
        // later: fade in from the plain background.
        bool crossFade = heroShownAt_ - heroSince_ < 150 && !prevHeroUrl_.empty();
        auto previous = crossFade && age < kHeroFadeMs ? images_.get(prevHeroUrl_, kW, kH, fullScreen) : nullptr;
        if (previous) c.drawImage(*previous, (kW - previous->w) / 2, (kH - previous->h) / 2);
        uint8_t alpha = age >= kHeroFadeMs ? 255 : static_cast<uint8_t>(age * 255 / kHeroFadeMs);
        c.drawImage(*current, (kW - current->w) / 2, (kH - current->h) / 2, alpha);
        if (alpha < 255) animating_ = true;
    } else {
        prevHeroUrl_.clear();  // nothing on screen to fade from any more
    }
    // The Horizon stage keeps art visible through all 1080 lines. Gradients
    // protect readability but never paint a hard band across the wallpaper.
    if (fullScreen) {
        c.gradientH({0, 0, 1300, kH}, rgba(2, 11, 23, 192), rgba(2, 11, 23, 0));
        c.gradientV({0, 610, kW, 470}, rgba(4, 13, 24, 0), rgba(4, 13, 24, 118));
    } else {
        c.gradientH({0, 0, details ? 1400 : 1200, kH}, rgba(0, 0, 0, details ? 240 : 225), rgba(0, 0, 0, 0));
        if (details) {
            c.gradientV({0, 640, kW, 440}, withAlpha(kBg, 0), withAlpha(kBg, 230));
        } else {
            c.gradientV({0, 360, kW, 420}, withAlpha(kBg, 0), kBg);
            c.fillRect({0, 780, kW, kH - 780}, kBg);
        }
    }
    c.gradientV({0, 0, kW, 160}, rgba(0, 0, 0, 150), rgba(0, 0, 0, 0));
    return current != nullptr;
}

void AppUi::drawSplash(Canvas& c, uint64_t nowMs) {
    drawBackground(c);
    drawInternalBrand(c, kW / 2 - 235, 283, 470, 255);
    drawCentered(c, fonts_.regular, status_, 590, 28, kGray);
    drawSpinner(c, kW / 2.0f, 720, 26, nowMs);
}

void AppUi::drawSignIn(Canvas& c, uint64_t nowMs) {
    drawBackground(c);
    c.gradientH({0, 80, kW, 920}, withAlpha(kBg, 115), rgba(95, 69, 49, 75));
    preview31::glassPanel(c, {139, 110, 890, 850}, 22);
    preview31::glassPanel(c, {1160, 190, 660, 690}, 22);
    c.gradientV({1160, 190, 660, 690}, withAlpha(preview31::kAccent, 14),
                rgba(5, 21, 38, 24));
    const int left = 180;
    drawInternalBrand(c, left - 12, 113, 208, 113);
    fonts_.bold.draw(c, tr(Str::SignInTitle), left, 260, 72, kWhite);
    fonts_.regular.draw(c, tr(Str::SignInStep1), left, 400, 32, kGray);
    std::string shortUrl = codeUrl_;
    for (const char* p : {"https://", "http://", "www."})
        if (shortUrl.rfind(p, 0) == 0) shortUrl = shortUrl.substr(std::string(p).size());
    fonts_.semibold.draw(c, shortUrl, left, 450, 44, kWhite);
    fonts_.regular.draw(c, tr(Str::SignInStep2), left, 540, 32, kGray);

    // The code, letter by letter, in a panel.
    constexpr int kPx = 96;
    int letters = static_cast<int>(code_.size());
    int cell = 82;
    Rect panel{left, 600, std::max(400, letters * cell + 64), 150};
    c.fillRect(panel, kPanel, 18);
    int x = panel.x + 32;
    for (char ch : code_) {
        std::string s(1, ch);
        fonts_.bold.draw(c, s, x + (cell - fonts_.bold.measure(s, kPx)) / 2, panel.y + 18, kPx, kWhite);
        x += cell;
    }
    if (caps_.openUrl && !code_.empty()) {
#if !defined(__ANDROID__)
        fonts_.regular.draw(c, tr(Str::SignInClickToCopy), left, 762, 22, kDim);
#endif
    }
    drawSpinner(c, left + 18, 885, 14, nowMs);
    fonts_.regular.draw(c, tr(Str::SignInWaiting), left + 48, 872, 26, kDim);

    if (!platform::dataDirWritable()) {
        std::string pathAndErr = platform::dataDir() + " (errno " + std::to_string(platform::dataDirErrno()) + ")";
        std::string warn1 = trf(Str::StorageNotWritable, pathAndErr);
        std::string warn2 = trf(Str::StorageFixHint, platform::dataDir());
        fonts_.semibold.draw(c, warn1, left, 935, 22, rgba(255, 160, 40));
        fonts_.regular.draw(c, warn2, left, 965, 20, kGray);
    }

#if defined(__ANDROID__)
    // Harmonized Xbox-styled primary action button for mobile browser sign-in
    Rect btnBox{1180, 480, 560, 124};
    c.fillRect(btnBox, rgba(27, 126, 141), 24);
    c.strokeRect(btnBox, rgba(128, 228, 238, 185), 2, 24);

    const char* btnTxt = tr(Str::SignInOpenBrowser);
    int bw = fonts_.bold.measure(btnTxt, 28);
    fonts_.bold.draw(c, btnTxt, btnBox.x + (btnBox.w - bw) / 2,
                     btnBox.y + 24, 28, kWhite);

    const char* subTxt = tr(Str::SignInOpenBrowserHint);
    int sw = fonts_.regular.measure(subTxt, 18);
    fonts_.regular.draw(c, subTxt, btnBox.x + (btnBox.w - sw) / 2,
                        btnBox.y + btnBox.h - 32, 18, rgba(220, 245, 225));

    // Under the code panel: clean copy affordance badge matching app palette
    Rect copyBadge{left, 765, panel.w, 46};
    c.fillRect(copyBadge, rgba(28, 28, 34, 220), 12);
    c.strokeRect(copyBadge, rgba(255, 255, 255, 30), 1, 12);
    const char* copyMsg = tr(Str::SignInClickToCopy);
    int cw = fonts_.semibold.measure(copyMsg, 20);
    fonts_.semibold.draw(c, copyMsg, copyBadge.x + (copyBadge.w - cw) / 2,
                         fonts_.semibold.centeredY(copyBadge.y, copyBadge.h, 20), 20, kGray);
#else
    // QR code for the sign-in page. Not "?otc=<code>": that prefilled link
    // goes through a consent flow that rejects this client id ("first party
    // application ... pre-authorization"), so the code is typed on the phone.
    // The plain link, the code typed by hand. With the code in it (microsoft.com/link?otc=CODE,
    // tried 2026-11-12) Microsoft answers "the application is a first party application... users
    // are not permitted to consent": the pre-filled path asks for a consent this app's client
    // id (a Microsoft one) cannot be given. It works for apps with a client id of their own.
    std::string target = codeUrl_.empty() ? "https://www.microsoft.com/link" : codeUrl_;
    uint8_t qr[qrcodegen_BUFFER_LEN_MAX], tmp[qrcodegen_BUFFER_LEN_MAX];
    if (qrcodegen_encodeText(target.c_str(), tmp, qr, qrcodegen_Ecc_MEDIUM, qrcodegen_VERSION_MIN,
                             qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true)) {
        int n = qrcodegen_getSize(qr);
        constexpr int kBox = 440;
        int module = (kBox - 60) / (n + 2);
        int size = module * (n + 2);
        Rect box{1280, 300, size + 40, size + 40};
        c.fillRect(box, kWhite, 24);
        int ox = box.x + 20 + module, oy = box.y + 20 + module;
        for (int yy = 0; yy < n; ++yy)
            for (int xx = 0; xx < n; ++xx)
                if (qrcodegen_getModule(qr, xx, yy)) c.fillRect({ox + xx * module, oy + yy * module, module, module}, kBg);
        int w = fonts_.regular.measure(tr(Str::SignInScan), 26);
        fonts_.regular.draw(c, tr(Str::SignInScan), box.x + (box.w - w) / 2, box.y + box.h + 24, 26, kGray);
    }
    if (caps_.openUrl) drawHints(c, {{kIconCross, tr(Str::OpenInBrowser)}});  // a desktop: the page in the browser
#endif
}

void AppUi::drawTabs(Canvas& c) {
    // Make the full tab clickable while drawing only its label and active underline.
    const char* labels[3] = {tr(Str::TabGamePass), tr(Str::YourGames), tr(Str::TabConsoles)};
    constexpr int kPx = 22, kPad = 26, kGap = 12, kH = 44, kY = 50;
    int widths[3], total = 0;
    for (int i = 0; i < 3; ++i) {
        widths[i] = fonts_.semibold.measure(labels[i], kPx) + 2 * kPad;
        total += widths[i] + (i ? kGap : 0);
    }
    int x = (kW - total) / 2;
    auto shoulder = [&](const char* name, int cx) {
        if (!input::hasGamepad() && !g_keyboardHints) return;
        Rect r{cx - 22, kY + 8, 44, 28};
        fonts_.bold.draw(c, name, cx - fonts_.bold.measure(name, 16) / 2, fonts_.bold.centeredY(r.y, r.h, 16), 16, kGray);
    };
    shoulder(g_keyboardHints ? "Q" : buttonName(g_glyphStyle, PadButton::L1), x - 40);
    for (int i = 0; i < 3; ++i) {
        bool on = static_cast<int>(tab_) == i;
        Rect r{x, kY, widths[i], kH};
        if (on) {
            c.fillRect({r.x + kPad, r.y + r.h - 3, r.w - 2 * kPad, 3},
                       kWhite, 1);
        }
        fonts_.semibold.draw(c, labels[i], x + kPad, fonts_.semibold.centeredY(kY, kH, kPx),
                             kPx, on ? kWhite : rgba(223, 226, 233));
        x += widths[i] + kGap;
    }
    shoulder(g_keyboardHints ? "E" : buttonName(g_glyphStyle, PadButton::R1), x - kGap + 40);
}

void AppUi::drawCard(Canvas& c, const GameTile& t, int x, int y, bool focused, bool gamePassBadge) {
    if (auto img = images_.get(t.tileUrl, kCard, kCard)) {
        c.drawImage(*img, x, y, 255, 10);
    } else {
        c.fillRect({x, y, kCard, kCard}, kPlaceholder, 10);
        auto lines = fonts_.semibold.wrap(t.name, 22, kCard - 32, 3);
        for (size_t k = 0; k < lines.size(); ++k)
            fonts_.semibold.draw(c, lines[k], x + 16, y + 16 + static_cast<int>(k) * 30, 22, kGray);
    }
    // Top-left: time played on this title, when recorded.
    uint64_t cardPlayedSec = 0;
    if (!t.productId.empty()) {
        auto it = settings_.titlePlaytime.find(t.productId);
        if (it != settings_.titlePlaytime.end()) cardPlayedSec = it->second;
    }
    if (!cardPlayedSec && !t.titleId.empty()) {
        auto it = settings_.titlePlaytime.find(t.titleId);
        if (it != settings_.titlePlaytime.end()) cardPlayedSec = it->second;
    }
    if (cardPlayedSec > 0) {
        std::string ptLabel = formatPlaytime(cardPlayedSec);
        int pw = std::min(kCard - 56, fonts_.bold.measure(ptLabel, 11) + 14);
        Rect ptBadge{x + 10, y + 10, pw, 22};
        c.fillRect(ptBadge, rgba(0, 0, 0, 215), 4);
        fonts_.bold.draw(c, ptLabel, ptBadge.x + (ptBadge.w - fonts_.bold.measure(ptLabel, 11)) / 2,
                         fonts_.bold.centeredY(ptBadge.y, ptBadge.h, 11), 11, kWhite);
    }
    // Top-right: Wishlist blue star when marked (or lock if unplayable and not wishlisted)
    bool isWishlisted = !t.productId.empty() && wishlist_.count(t.productId) > 0;
    if (isWishlisted) {
        c.fillCircle(x + kCard - 24, y + 24, 14, rgba(10, 20, 35, 230));
        c.strokeCircle(x + kCard - 24, y + 24, 14, 1.5f, rgba(40, 140, 255, 180));
        drawStar(c, x + kCard - 24, y + 24, 7.5f, rgba(50, 160, 255));
    }

    // Bottom-right: console badge and input badge.
    // Calculate rightBadgeLeft so bottom-left badges never overlap them.
    int rightEdge = x + kCard - 10;
    int rightBadgeLeft = rightEdge;
    if (const char* p = platformLabel(t.platform)) {
        int pTextW = fonts_.bold.measure(p, 11);
        int pBadgeW = std::clamp(pTextW + 12, 54, 66);
        Rect badge{rightBadgeLeft - pBadgeW, y + kCard - 34, pBadgeW, 24};
        c.fillRect(badge, rgba(0, 0, 0, 220), 4);
        fonts_.bold.draw(c, p, badge.x + (badge.w - pTextW) / 2,
                         fonts_.bold.centeredY(badge.y, badge.h, 11), 11, kWhite);
        rightBadgeLeft = badge.x - 4;
    }
    if (t.mouseKeyboard || fonts_.hasIcons) {
        constexpr int kInputW = 28;
        Rect k{rightBadgeLeft - kInputW, y + kCard - 34, kInputW, 24};
        c.fillRect(k, rgba(0, 0, 0, 220), 4);
        const char* glyph = t.mouseKeyboard ? kIconKeyboardMouse : kIconGamepad;
        if (fonts_.hasIcons) {
            int gw = fonts_.icons.measure(glyph, 18);
            int gx = k.x + (k.w - gw) / 2;
            fonts_.icons.draw(c, glyph, gx, k.y + 3, 18, kWhite);
        } else {
            int kw = 16, kh = kw * 10 / 24;
            int kx = k.x + (k.w - kw) / 2;
            int ky = k.y + (k.h - kh) / 2;
            drawKeyboardGlyph(c, kx, ky, kw, kWhite);
        }
        rightBadgeLeft = k.x - 6;
    }

    // Bottom-left: price badge, plan badge, or friends playing; bounded by rightBadgeLeft.
    int maxLeftW = std::max(40, rightBadgeLeft - (x + 10) - 4);
    const char* planBadge = planBadgeLabel(t.plan);
    uint8_t myPlan = xcloud::planTier(subscription_);
    bool otherPlan = planBadge && (!t.playable || (myPlan > 0 && !xcloud::planIncludes(myPlan, t.plan, isFreeGame(t))));
    bool showPlan = otherPlan || (planBadge && searching_ && tab_ == Tab::GamePass && filterPlan_ > 0 && t.friends.empty());
    bool isF2P = isFreeGame(t);
    bool effectivePlayable = isEffectivePlayableLocked(t);
    if (!effectivePlayable && !t.playable && (isF2P || t.purchasable)) {
        c.fillRect({x, y, kCard, kCard}, rgba(0, 0, 0, 70), 10);
        auto price = prices_.find(t.productId);
        bool free = isFreeGame(t);
        std::string label = free                               ? tr(Str::Free)
                            : price != prices_.end()           ? price->second.now
                                                               : tr(Str::BuyBadge);
        int tw = fonts_.bold.measure(label, 11);
        int w = std::min(maxLeftW, 20 + tw + 8);
        if (w < 20 + tw + 8) {
            label = fonts_.bold.fit(label, 11, w - 22);
        }
        Rect badge{x + 10, y + kCard - 34, w, 24};
        c.fillRect(badge, rgba(16, 124, 16, 235), 4);
        drawBag(c, badge.x + 10, badge.y + badge.h / 2, kWhite, 0.52f);
        fonts_.bold.draw(c, label, badge.x + 18, fonts_.bold.centeredY(badge.y, badge.h, 11), 11, kWhite);
    } else if (!effectivePlayable) {
        c.fillRect({x, y, kCard, kCard}, rgba(0, 0, 0, 150), 10);
        drawLock(c, isWishlisted ? x + kCard - 54 : x + kCard - 34, isWishlisted ? y + 24 : y + 30);
        if (planBadge) {
            int w = std::min(maxLeftW, fonts_.bold.measure(planBadge, 11) + 14);
            Rect badge{x + 10, y + kCard - 34, w, 24};
            c.fillRect(badge, rgba(0, 0, 0, 220), 4);
            fonts_.bold.draw(c, planBadge, badge.x + (badge.w - fonts_.bold.measure(planBadge, 11)) / 2,
                             fonts_.bold.centeredY(badge.y, badge.h, 11), 11, kWhite);
        }
    } else if (showPlan) {
        int w = std::min(maxLeftW, fonts_.bold.measure(planBadge, 11) + 14);
        Rect badge{x + 10, y + kCard - 34, w, 24};
        c.fillRect(badge, rgba(0, 0, 0, 220), 4);
        fonts_.bold.draw(c, planBadge, badge.x + (badge.w - fonts_.bold.measure(planBadge, 11)) / 2,
                         fonts_.bold.centeredY(badge.y, badge.h, 11), 11, kWhite);
    } else if (!t.friends.empty()) {
        int numAvatars = std::min<int>(static_cast<int>(t.friends.size()), 2);
        int extraCount = static_cast<int>(t.friends.size()) - numAvatars;
        constexpr int kAvatarSz = 20;
        constexpr int kAvatarOverlap = 13;
        constexpr int kPillH = 26;
        constexpr int kPadX = 4;
        constexpr int kPadY = 3;

        int avatarsW = numAvatars == 1 ? kAvatarSz : (kAvatarSz + (numAvatars - 1) * kAvatarOverlap);
        std::string tagText;
        if (t.friends.size() == 1) {
            tagText = t.friends[0].gamertag;
        } else if (extraCount > 0) {
            tagText = "+" + std::to_string(extraCount);
        } else if (t.friends.size() == 2) {
            tagText = t.friends[0].gamertag;
        }

        int maxPillW = maxLeftW;
        int maxTextW = maxPillW - avatarsW - kPadX * 2 - 8;
        int textW = !tagText.empty() ? fonts_.semibold.measure(tagText, 12) + 6 : 0;
        if (textW > maxTextW && maxTextW > 15) {
            while (!tagText.empty() && fonts_.semibold.measure(tagText + "…", 12) + 6 > maxTextW) {
                tagText.pop_back();
            }
            tagText += "…";
            textW = fonts_.semibold.measure(tagText, 12) + 6;
        }

        int pillW = kPadX * 2 + avatarsW + textW;
        Rect pill{x + 10, y + kCard - 10 - kPillH, pillW, kPillH};
        c.fillRect(pill, rgba(0, 0, 0, 215), kPillH / 2);
        c.strokeRect(pill, rgba(255, 255, 255, 50), 1, kPillH / 2);

        int curX = pill.x + kPadX;
        int curY = pill.y + kPadY;
        for (int i = 0; i < numAvatars; ++i) {
            int ax = curX + i * kAvatarOverlap;
            const auto& friendInfo = t.friends[static_cast<size_t>(i)];
            if (auto pic = images_.get(friendInfo.gamerpicUrl, kAvatarSz, kAvatarSz)) {
                c.drawImage(*pic, ax, curY, 255, -1);
            } else {
                c.fillCircle(ax + kAvatarSz / 2.0f, curY + kAvatarSz / 2.0f, kAvatarSz / 2.0f, rgba(48, 56, 68, 255));
                if (!friendInfo.gamertag.empty()) {
                    std::string initial(1, static_cast<char>(std::toupper(static_cast<unsigned char>(friendInfo.gamertag[0]))));
                    int iw = fonts_.bold.measure(initial, 11);
                    int iy = fonts_.bold.centeredY(curY, kAvatarSz, 11);
                    fonts_.bold.draw(c, initial, ax + (kAvatarSz - iw) / 2, iy, 11, kWhite);
                }
            }
            c.strokeCircle(ax + kAvatarSz / 2.0f, curY + kAvatarSz / 2.0f, kAvatarSz / 2.0f, 1.2f, rgba(255, 255, 255, 200));
        }

        if (!tagText.empty()) {
            int tx = curX + avatarsW + 4;
            int ty = fonts_.semibold.centeredY(pill.y, pill.h, 12);
            if (extraCount > 0 && numAvatars == 2) {
                fonts_.bold.draw(c, tagText, tx, ty, 12, rgba(16, 124, 16, 255));
            } else {
                fonts_.semibold.draw(c, tagText, tx, ty, 12, kWhite);
            }
        }
    }
    if (focused) {
        // A legible focus marker on gamepad, mouse and touch.
        c.strokeRect({x - 8, y - 8, kCard + 16, kCard + 16},
                     withAlpha(kWhite, 52), 7, 17);
        c.strokeRect({x - 6, y - 6, kCard + 12, kCard + 12},
                     kWhite, 3, 16);
    }
}

void AppUi::drawGrid(Canvas& c, const std::vector<GameTile>& tiles, int x0, int y0, int cols, float scroll, int focus,
                     int clipTop) {
    int offset = static_cast<int>(std::lround(scroll));
    c.setClip({0, clipTop, kW, kH - clipTop});
    int firstRow = std::max(0, (clipTop - y0 + offset - kCard - 40) / kGridPitchY);
    size_t startIdx = static_cast<size_t>(firstRow) * static_cast<size_t>(cols);
    for (size_t i = startIdx; i < tiles.size(); ++i) {
        int col = static_cast<int>(i) % cols, row = static_cast<int>(i) / cols;
        int x = x0 + col * kCardPitch;
        int y = y0 + row * kGridPitchY - offset;
        if (y + kCard + 40 < clipTop) continue;
        if (y > kH) break;
        bool focused = static_cast<int>(i) == focus;
        drawCard(c, tiles[i], x, y, focused, false);
        auto name = fonts_.semibold.wrap(tiles[i].name, 20, kCard, 1);
        if (!name.empty()) fonts_.semibold.draw(c, name[0], x, y + kCard + 12, 20, focused ? kWhite : kGray);
    }
    c.clearClip();
    c.fillRect({0, 0, kW, clipTop}, kBg);
    c.gradientV({0, clipTop, kW, 12}, kBg, withAlpha(kBg, 0));
}

void AppUi::drawCollectionBackdrop(Canvas& c, uint64_t nowMs, const GameTile* game) {
    c.gradientV({0, 0, kW, kH}, rgba(38, 54, 72), rgba(8, 15, 26));
    if (game && !game->heroUrl.empty()) drawHero(c, nowMs, game->heroUrl, true, true);
    c.gradientV({0, 0, kW, kH}, rgba(6, 12, 20, 106), rgba(6, 12, 20, 221));
}

void AppUi::drawLibrary(Canvas& c, uint64_t nowMs) {
    const GameTile* focus = libraryTile(gridFocus_);
    drawCollectionBackdrop(c, nowMs, focus && !focus->heroUrl.empty() ? focus : focusedTile());
    constexpr int kGridW = kLibraryCols * kCardPitch - kCardGap;
    const int left = (kW - kGridW) / 2;
    const int clipTop = kLibraryTop - 10;
    const LibraryLayout& L = libraryLayout();
    int offset = static_cast<int>(std::lround(gridScroll_.value));
    c.setClip({0, clipTop, kW, kH - 80 - clipTop});
    // Each section has its heading in the grid: a title, the count, a line
    // of explanation (the games to buy have a bag).
    auto heading = [&](int section, const char* title, size_t count, const char* line, bool bag) {
        int hy = kLibraryTop + L.headerY[section] - offset;
        if (hy < clipTop - kSectionHeaderH || hy > kH) return;
        int x = left;
        if (bag) {
            drawBag(c, left + 14, hy + 30, kWhite);
            x += 40;
        }
        fonts_.bold.draw(c, title, x, hy + 8, 34, kWhite);
        int w = fonts_.bold.measure(title, 34);
        fonts_.semibold.draw(c, gamesCount(count), x + w + 20, hy + 20, 22, kDim);
        if (line) fonts_.regular.draw(c, line, x, hy + 52, 22, kGray);
    };
    if (!purchasable_.empty()) heading(1, tr(Str::AvailableToBuy), purchasable_.size(), tr(Str::BuyToPlay), true);
    if (!hiddenTiles_.empty()) heading(2, tr(Str::HiddenSection), hiddenTiles_.size(), tr(Str::HiddenHint), false);
    int minRowY = clipTop - kLibraryTop + offset - kCard - 40;
    auto rowIt = std::lower_bound(L.rowY.begin(), L.rowY.end(), minRowY);
    int firstVisRow = static_cast<int>(rowIt - L.rowY.begin());
    auto tileIt = std::lower_bound(L.rowOf.begin(), L.rowOf.end(), firstVisRow);
    for (size_t i = static_cast<size_t>(tileIt - L.rowOf.begin()); i < L.rowOf.size(); ++i) {
        int y = kLibraryTop + L.rowY[static_cast<size_t>(L.rowOf[i])] - offset;
        if (y + kCard + 40 < clipTop) continue;
        if (y > kH) break;
        int x = left + L.colOf[i] * kCardPitch;
        const GameTile* t = libraryTile(static_cast<int>(i));
        bool focused = static_cast<int>(i) == gridFocus_;
        drawCard(c, *t, x, y, focused, false);
        if (hidden_.count(t->productId)) c.fillRect({x, y, kCard, kCard}, rgba(0, 0, 0, 120), 10);
        auto name = fonts_.semibold.wrap(t->name, 20, kCard, 1);
        if (!name.empty()) fonts_.semibold.draw(c, name[0], x, y + kCard + 12, 20, focused ? kWhite : kGray);
    }
    c.clearClip();
    drawTopBar(c, true);
    drawTabs(c);
    const char* title = tr(Str::YourGames);
    fonts_.bold.draw(c, title, left, 157, 52, kWhite);
    if (ownedKnown_) {
        fonts_.semibold.draw(c, gamesCount(owned_.size()),
                             left + fonts_.bold.measure(title, 52) + 24, 184, 22, kGray);
    }
    if (owned_.empty() && purchasable_.empty() && hiddenTiles_.empty()) {
        if (!ownedKnown_) {
            drawSpinner(c, kW / 2.0f, 470, 26, nowMs);
            drawCentered(c, fonts_.semibold, tr(Str::LoadingGames), 530, 28, kGray);
            animating_ = true;
        } else {
            drawCentered(c, fonts_.semibold, tr(Str::NoGames), 500, 30, kGray);
        }
    }
    {
        drawLoadingBar(c, left, kLibraryTop - 24, 420, nowMs);
        // The order (the whole tab's), right-aligned under the tabs, with its button.
        Str sortName = librarySort_ == LibrarySort::AZ        ? Str::SortAZ
                       : librarySort_ == LibrarySort::Console ? Str::SortConsole
                                                              : Str::SortRecent;
        std::string label = trf(Str::SortLabel, tr(sortName));
        label = fonts_.semibold.fit(label, 22, kLibrarySortTarget.w - 24);
        const int right = kLibrarySortTarget.x + kLibrarySortTarget.w;
        fonts_.semibold.draw(c, label, right - 24 - fonts_.semibold.measure(label, 22),
                             fonts_.semibold.centeredY(kLibrarySortTarget.y, kLibrarySortTarget.h, 22), 22, kWhite);
        c.line(right - 14, 188, right - 9, 193, 2, kWhite);
        c.line(right - 9, 193, right - 4, 188, 2, kWhite);
    }
    c.gradientV({0, kH - 190, kW, 110}, withAlpha(kBg, 0), withAlpha(kBg, 245));
    c.fillRect({0, kH - 80, kW, 80}, withAlpha(kBg, 245));
    bool focusHidden = focus && hidden_.count(focus->productId);
    drawHints(c, {{kIconCross, tr(Str::Select)},
                  {kIconSquare, tr(focusHidden ? Str::Unhide : Str::Hide)},
                  {kIconTriangle, tr(Str::TabSearch)},
                  {kIconL3, tr(Str::RefreshLibrary)},
                  {kIconR3, tr(Str::SortHint)},
                  {kIconL2R2, tr(Str::Sections)},
                  {kIconOptions, tr(Str::Settings)}});
    drawPads(c);
    drawToast(c, nowMs);
}

void AppUi::drawSearch(Canvas& c, uint64_t nowMs) {
    drawBackground(c);
    // Results on the right.
    drawGrid(c, results_, kResultsX, kGridTop, kResultCols, resultScroll_.value, searchOnKeys_ ? -1 : resultFocus_,
             kGridTop - 20);
    drawTopBar(c);
    drawTabs(c);
    const char* scope = tab_ == Tab::GamePass ? tr(Str::TabGamePass) : tr(Str::YourGames);
    bool filtering = anyFilter();
    if (query_.empty() && !filtering) {
        fonts_.semibold.draw(c, tr(Str::SearchHint), kResultsX, 150, 26, kDim);
    } else if (results_.empty()) {
        const char* emptyMsg = (tab_ == Tab::Library && filterWishlist_) ? tr(Str::WishlistEmpty) : tr(Str::NoResults);
        fonts_.semibold.draw(c, emptyMsg, kResultsX, 150, 26, kGray);
    } else {
        fonts_.semibold.draw(c, trf(Str::ResultsCount, std::to_string(results_.size())), kResultsX, 150, 26, kGray);
    }
    drawLoadingBar(c, kResultsX + 420, 150, 420, nowMs);

    // The query.
    const int kbW = kKeyCols * (kKeyW + kKeyGap) - kKeyGap;
    Rect box{kMargin, 140, kbW, 76};
    c.fillRect(box, kPanel, 14);
    if (searchOnKeys_) c.strokeRect(box, rgba(255, 255, 255, 90), 2, 14);
    int tx = box.x + 24;
    if (query_.empty()) {
        fonts_.regular.draw(c, trf(Str::SearchIn, scope), tx, box.y + 22, 26, kDim);
    } else {
        fonts_.semibold.draw(c, query_, tx, box.y + 20, 30, kWhite);
        tx += fonts_.semibold.measure(query_, 30) + 4;
    }
    if (searchOnKeys_ && (nowMs / 530) % 2 == 0) c.fillRect({query_.empty() ? box.x + 20 : tx, box.y + 18, 3, 40}, kWhite);
    if (searchOnKeys_) animating_ = true;  // the caret blinks

    // The keys.
    for (int r = 0; r <= kKeyRows; ++r) {
        int n = r == kKeyRows ? 3 : kKeyCols;
        for (int k = 0; k < n; ++k) {
            int col = r == kKeyRows ? kWideCols[k] : k;
            int span = r == kKeyRows ? kWideSpan[k] : 1;
            Rect key{kMargin + col * (kKeyW + kKeyGap), kKeysY + r * (kKeyH + kKeyGap),
                     span * kKeyW + (span - 1) * kKeyGap, kKeyH};
            bool focused = searchOnKeys_ && keyRow_ == r && keyCol_ == k;
            c.fillRect(key, focused ? kWhite : kPanel, 10);
            std::string label = r == kKeyRows ? tr(k == 0 ? Str::KeySpace : k == 1 ? Str::KeyDelete : Str::KeyClear)
                                               : std::string(1, static_cast<char>(std::toupper(kKeys[r * kKeyCols + k])));
            int px = r == kKeyRows ? 22 : 28;
            int w = fonts_.semibold.measure(label, px);
            fonts_.semibold.draw(c, label, key.x + (key.w - w) / 2, fonts_.semibold.centeredY(key.y, kKeyH, px), px,
                                 focused ? kBg : kWhite);
        }
    }
    c.gradientV({0, kH - 190, kW, 110}, withAlpha(kBg, 0), withAlpha(kBg, 245));
    c.fillRect({0, kH - 80, kW, 80}, withAlpha(kBg, 245));
    // The filters, 3x3 grid under the keys: green when on; the lists with a chevron.
    const int gap = kChipGap, px = 18, fh = 48;
    const int fy0 = kKeysY + (kKeyRows + 1) * (kKeyH + kKeyGap) + 8;
    constexpr int kCols = 3;
    const int chipW = (kKeysW - gap * (kCols - 1)) / kCols;
    Rect openChip{};
    for (int row = 0; row < 3; ++row) {
        auto chips = filterRow(row);
        if (chips.empty()) continue;
        int fy = fy0 + row * (fh + 8);
        for (size_t k = 0; k < chips.size(); ++k) {
            Filter f = chips[k];
            bool on = filterOn(f);
            bool focused = searchOnKeys_ && keyRow_ == kKeyRows + 1 + row && keyCol_ == static_cast<int>(k);
            Rect chip{kMargin + static_cast<int>(k) * (chipW + gap), fy, chipW, fh};
            if (filterList_ == static_cast<int>(f)) openChip = chip;
            c.fillRect(chip, focused ? kWhite : on ? rgba(16, 124, 16, 255) : kPanel, fh / 2);
            if (on && focused) c.strokeRect({chip.x + 3, chip.y + 3, chip.w - 6, chip.h - 6}, kGreen, 3, fh / 2 - 3);
            Color text = focused ? kBg : kWhite;
            int room = chip.w - (isList(f) ? 56 : 20);
            int chipPx = px;
            while (chipPx > 13 && fonts_.semibold.measure(filterText(f), chipPx) > room) --chipPx;
            std::string label = fonts_.semibold.fit(filterText(f), chipPx, room);
            int tw = fonts_.semibold.measure(label, chipPx);
            int tx = chip.x + (chip.w - tw) / 2;
            fonts_.semibold.draw(c, label, tx, fonts_.semibold.centeredY(chip.y, chip.h, chipPx), chipPx, text);
            if (isList(f)) {  // a list: a chevron at the right
                float cx = static_cast<float>(chip.x + chip.w - 20), cy = static_cast<float>(chip.y + chip.h / 2);
                c.line(cx - 5, cy - 3, cx, cy + 2, 2.2f, text);
                c.line(cx, cy + 2, cx + 5, cy - 3, 2.2f, text);
            }
        }
    }
    if (filterList_ >= 0) {
        // The open list, above its chip (they are near the bottom); scrolls
        // past kVisible choices.
        Filter f = static_cast<Filter>(filterList_);
        auto options = filterOptions(f);
        int sel = filterSelected(f);
        constexpr int kItemH = 54, kVisible = 9;
        int n = static_cast<int>(options.size()), visible = std::min(n, kVisible);
        if (filterListIndex_ < filterListTop_) filterListTop_ = filterListIndex_;
        if (filterListIndex_ >= filterListTop_ + visible) filterListTop_ = filterListIndex_ - visible + 1;
        int listW = std::max(openChip.w, 460);
        Rect panel{openChip.x, openChip.y - 10 - (visible * kItemH + 16), listW, visible * kItemH + 16};
        panel.y = std::max(120, panel.y);
        c.fillRect({panel.x + 6, panel.y + 10, panel.w, panel.h}, rgba(0, 0, 0, 120), 18);  // shadow
        c.fillRect(panel, rgba(38, 38, 38, 250), 18);
        c.strokeRect(panel, rgba(255, 255, 255, 50), 2, 18);
        for (int k = 0; k < visible; ++k) {
            int i = filterListTop_ + k;
            Rect item{panel.x + 8, panel.y + 8 + k * kItemH, panel.w - 16, kItemH};
            bool on = i == filterListIndex_;
            if (on) c.fillRect(item, kWhite, 12);
            Color text = on ? kBg : (i == sel ? kWhite : kGray);
            std::string label = fonts_.semibold.fit(options[static_cast<size_t>(i)], 24, item.w - 70);
            fonts_.semibold.draw(c, label, item.x + 20, fonts_.semibold.centeredY(item.y, item.h, 24), 24, text);
            if (i == sel) {  // a check mark on the current choice
                float cx = static_cast<float>(item.x + item.w - 30), cy = static_cast<float>(item.y + item.h / 2);
                c.line(cx - 9, cy, cx - 3, cy + 6, 3, on ? kBg : kGreen);
                c.line(cx - 3, cy + 6, cx + 9, cy - 6, 3, on ? kBg : kGreen);
            }
        }
        if (filterListTop_ > 0) {  // more above / below
            float cx = static_cast<float>(panel.x + panel.w / 2), cy = static_cast<float>(panel.y - 12);
            c.line(cx - 9, cy + 4, cx, cy - 4, 3, kGray);
            c.line(cx, cy - 4, cx + 9, cy + 4, 3, kGray);
        }
        if (filterListTop_ + visible < n) {
            float cx = static_cast<float>(panel.x + panel.w / 2), cy = static_cast<float>(panel.y + panel.h + 12);
            c.line(cx - 9, cy - 4, cx, cy + 4, 3, kGray);
            c.line(cx, cy + 4, cx + 9, cy - 4, 3, kGray);
        }
    }
    if (filterList_ >= 0)
        drawHints(c, {{kIconCross, tr(Str::Select)}, {kIconCircle, tr(Str::Back)}});
    else if (searchOnKeys_)
        drawHints(c, {{kIconCross, tr(Str::KeyType)}, {kIconSquare, tr(Str::KeyDelete)}, {kIconCircle, tr(Str::Back)}});
    else
        drawHints(c, {{kIconCross, tr(Str::Select)}, {kIconSquare, tr(Str::KeyDelete)}, {kIconCircle, tr(Str::Back)}});
    drawToast(c, nowMs);
}

void AppUi::drawConsoles(Canvas& c, uint64_t nowMs) {
    drawCollectionBackdrop(c, nowMs, focusedTile());
    drawTopBar(c, true);
    drawTabs(c);
    constexpr int left = (kW - (kLibraryCols * kCardPitch - kCardGap)) / 2;
    drawLoadingBar(c, left, 296, 420, nowMs);
    fonts_.bold.draw(c, tr(Str::TabConsoles), left, 157, 52, kWhite);
    fonts_.regular.draw(c, tr(Str::ConsolesHint), left, 241, 25, kGray);
    if (consoles_.empty()) {
        if (!consolesKnown_) {
            drawSpinner(c, kW / 2.0f, 520, 26, nowMs);
            drawCentered(c, fonts_.semibold, tr(Str::ConsolesLoading), 580, 28, kGray);
            animating_ = true;
            drawHints(c, {{kIconOptions, tr(Str::Settings)}, {kIconTouchpad, tr(Str::HoldSignOut)}});
        } else {
            // How a console shows up: three numbered steps, and Microsoft's
            // help on the phone.
            fonts_.bold.draw(c, tr(Str::NoConsoles), left, 355, 36, kWhite);
            int y = 445;
            const Str steps[] = {Str::ConsoleStep1, Str::ConsoleStep2, Str::ConsoleStep3};
            for (int i = 0; i < 3; ++i) {
                const std::string number = "0" + std::to_string(i + 1);
                fonts_.semibold.draw(c, number, left, y + 3, 23, rgba(182, 206, 221));
                for (const auto& line : fonts_.regular.wrap(tr(steps[i]), 26, 910, 3)) {
                    fonts_.regular.draw(c, line, left + 64, y, 26, kWhite);
                    y += 36;
                }
                if (i < 2) c.fillRect({left + 64, y + 17, 910, 1}, withAlpha(kWhite, 46));
                y += 43;
            }
            Rect box = drawQr(c, "https://support.xbox.com/help/games-apps/game-setup-and-play/how-to-use-remote-play",
                              kW - left - 56, 401, 260);
            if (box.w > 0) {
                int captionY = box.y + box.h + 22;
                for (const auto& line : fonts_.semibold.wrap(tr(Str::ScanForHelp), 22, 380, 2)) {
                    fonts_.semibold.draw(c, line, box.x + (box.w - fonts_.semibold.measure(line, 22)) / 2,
                                         captionY, 22, kWhite);
                    captionY += 30;
                }
            }
            const Rect search = kConsoleSearchTarget;
            c.fillRect(search, withAlpha(kWhite, 230), 12);
            const auto label = fonts_.semibold.fit(tr(Str::SearchAgain), 24, search.w - 36);
            fonts_.semibold.draw(c, label, search.x + (search.w - fonts_.semibold.measure(label, 24)) / 2,
                                 fonts_.semibold.centeredY(search.y, search.h, 24), 24, kBg);
            drawHints(c, {{kIconCross, tr(Str::SearchAgain)},
                          {kIconOptions, tr(Str::Settings)},
                          {kIconTouchpad, tr(Str::HoldSignOut)}});
        }
        drawPads(c);
        return;
    }
    // One card per console: a drawn Xbox, its name, model and state.
    for (size_t i = 0; i < consoles_.size(); ++i) {
        const ConsoleTile& con = consoles_[i];
        const Rect card = consoleCardRect(static_cast<int>(i), consoleFocus_);
        if (card.x + card.w <= 0) continue;
        if (card.x >= kW) break;
        const int x = card.x, kTop = card.y, kCardW = card.w;
        bool focused = static_cast<int>(i) == consoleFocus_;
        if (focused) c.strokeRect({card.x - 7, card.y - 7, card.w + 14, card.h + 14}, kWhite, 4, 22);
        c.fillRect(card, focused ? rgba(25, 40, 55, 221) : rgba(17, 28, 40, 177), 16);
        c.strokeRect(card, withAlpha(kWhite, focused ? 112 : 45), 1, 16);
        c.fillRect({x + 24, kTop + 222, kCardW - 48, 1}, preview31::kGlassLine);
        // The console: a Series X stands as a tower, the others lie flat.
        bool tower = con.type == "XboxSeriesX";
        Rect box = tower ? Rect{x + kCardW / 2 - 55, kTop + 40, 110, 170} : Rect{x + kCardW / 2 - 120, kTop + 90, 240, 70};
        c.fillRect(box, rgba(20, 20, 20), 12);
        c.strokeRect(box, rgba(70, 70, 70), 2, 12);
        bool on = con.power == "On";
        c.fillCircle(static_cast<float>(box.x + (tower ? box.w / 2 : 24)), static_cast<float>(box.y + (tower ? 22 : box.h / 2)),
                     tower ? 16.0f : 9.0f, on ? kGreen : rgba(90, 90, 90));
        std::string name = con.name.empty() ? "Xbox" : con.name;
        auto lines = fonts_.semibold.wrap(name, 30, kCardW - 48, 1);
        if (!lines.empty()) fonts_.semibold.draw(c, lines[0], x + 24, kTop + 236, 30, kWhite);
        std::string model = con.type == "XboxSeriesX"   ? "Xbox Series X"
                            : con.type == "XboxSeriesS" ? "Xbox Series S"
                            : con.type == "XboxOneX"    ? "Xbox One X"
                            : con.type == "XboxOneS"    ? "Xbox One S"
                            : con.type == "XboxOne"     ? "Xbox One"
                                                        : con.type;
        fonts_.regular.draw(c, model, x + 24, kTop + 284, 24, kGray);
        const char* state = on ? tr(Str::ConsoleOn)
                            : con.power == "ConnectedStandby" ? tr(Str::ConsoleSleeping)
                                                              : tr(Str::ConsoleOff);
        c.fillCircle(static_cast<float>(x + 32), static_cast<float>(kTop + 336), 6, on ? rgba(60, 200, 60) : kDim);
        fonts_.semibold.draw(c, state, x + 48, kTop + 322, 24, on ? kWhite : kGray);
    }
    drawHints(c, {{kIconCross, tr(Str::Play)},
                  {kIconTriangle, tr(Str::SearchAgain)},
                  {kIconOptions, tr(Str::Settings)},
                  {kIconTouchpad, tr(Str::HoldSignOut)}});
    drawPads(c);
    drawToast(c, nowMs);
}

// Nimbox Horizon — console-first view. Reuses the existing catalog and image cache.
void AppUi::drawNimboxHorizon(Canvas& c, uint64_t nowMs) {
    // Premium PS5 presentation, entirely in the existing software Canvas.
    // No HTML, GPU backend changes or streaming/input pipeline dependencies.
    const auto& env = nimbox_theme::palette(settings_.nimboxTheme);
    const Color deep = kBg;
    const Color soft = preview31::kMuted;
    const Color accent = env.accent;
    c.clear(deep);
    const GameTile* focus = focusedTile();
    std::string hero = focus ? focus->heroUrl : std::string();
    if (hero.empty() && focus && !focus->productId.empty()) {
        auto it = detailInfo_.find(focus->productId);
        if (it != detailInfo_.end()) hero = it->second.heroUrl;
    }
    const bool wideArt = !hero.empty() && drawHero(c, nowMs, hero, true, true);
    if (!wideArt) {
        c.gradientV({0, 0, kW, kH}, env.low, deep);
        c.gradientV({0, 0, kW, 200}, withAlpha(deep, 150), withAlpha(deep, 0));
        if (focus && !focus->tileUrl.empty()) {
            if (auto pic = images_.get(focus->tileUrl, 480, 480))
                c.drawImage(*pic, 1340, 185, 86, 20);
        }
    }

    c.gradientV({0, 480, kW, 600}, withAlpha(deep, 0), withAlpha(deep, 218));
    drawTopBar(c, true);
    drawTabs(c);

    if (!focus || rows_.empty()) {
        fonts_.semibold.draw(c, tr(Str::NoGames), 104, 352, 38, kWhite);
        drawHints(c, {{kIconOptions, tr(Str::Settings)}, {kIconTouchpad, tr(Str::HoldSignOut)}});
        drawToast(c, nowMs);
        return;
    }
    const int rowIndex = std::clamp(focusRow_, 0, int(rows_.size()) - 1);
    const GameRow& row = rows_[size_t(rowIndex)];
    const auto entry = [&](Rect r, const char* label, bool primary) {
        if (primary) c.fillRect(r, preview31::kSelect, 14);
        else c.fillRect(r, rgba(6, 13, 24, 130), 14);
        const auto text = fonts_.semibold.fit(label, 23, r.w - 30);
        fonts_.semibold.draw(c, text, r.x + (r.w - fonts_.semibold.measure(text, 23)) / 2,
                             fonts_.semibold.centeredY(r.y, r.h, 23), 23, primary ? kBg : kWhite);
    };
    const std::string exploreKey = (g_keyboardHints ? "Z" : buttonName(g_glyphStyle, PadButton::L2)) +
                                   std::string("  ") + tr(Str::BrowseHome);
    entry(horizon::kExplore, exploreKey.c_str(), false);
    if (row.catalogId == xcloud::sigl::kAllGames) {
        fonts_.bold.draw(c, tr(Str::BrowseAll), 104, 248, 70, kWhite);
        fonts_.regular.draw(c, gamesCount(row.tiles.size()), 106, 348, 26, kGray);
        entry(horizon::kPlayButton, tr(Str::OpenCollection), true);
        entry(horizon::kDetailsButton, tr(Str::BrowseGenres), false);
        fonts_.regular.draw(c, tr(Str::HorizonVerticalHint), 104, 950, 21, kGray);
        drawHints(c, {{kIconCross, tr(Str::OpenCollection)}, {kIconTriangle, tr(Str::TabSearch)}, {kIconOptions, tr(Str::Settings)}});
        drawnCollection_ = outgoingCollection_ = -1;
        return;
    }
    entry(horizon::kViewShelf, tr(Str::OpenCollection), false);
    const int count = int(row.tiles.size());
    if (count <= 0) {
        fonts_.semibold.draw(c, tr(Str::NoGames), 104, 352, 38, kWhite);
        drawHints(c, {{kIconOptions, tr(Str::Settings)}, {kIconTouchpad, tr(Str::HoldSignOut)}});
        drawToast(c, nowMs);
        return;
    }
    const int selected = std::clamp(focusCol_[size_t(rowIndex)], 0, count - 1);

    if (settings_.horizonShowcase) {
        const Rect cover = horizon::kShowcaseCover;
        c.fillRect({cover.x + 4, cover.y + 8, cover.w, cover.h}, rgba(0, 0, 0, 96), 18);
        c.fillRect(cover, kPanel, 16);
        if (auto art = images_.get(focus->tileUrl, cover.w, cover.h)) {
            c.drawImage(*art, cover.x, cover.y, 255, 16);
        } else {
            const auto lines = fonts_.semibold.wrap(focus->name, 28, cover.w - 48, 3);
            int y = cover.y + 30;
            for (const auto& line : lines) {
                fonts_.semibold.draw(c, line, cover.x + 24, y, 28, kWhite);
                y += 38;
            }
        }
        c.strokeRect(cover, withAlpha(kWhite, 104), 1, 16);
    }
    const std::string collection = row.title.empty() ? tr(Str::TabGamePass) : row.title;
    fonts_.bold.draw(c, fonts_.bold.fit(focus->name, 78, 1120), 100, 249, 78, preview31::kText);
    std::string meta = focus->publisher;
    if (!focus->categories.empty()) {
        if (!meta.empty()) meta += "  -  ";
        meta += focus->categories.front();
    }
    if (!meta.empty())
        fonts_.regular.draw(c, fonts_.regular.fit(meta, 23, 1040), 106, 354, 23, soft);

    const bool canPlay = isEffectivePlayableLocked(*focus);
    const Color primary = preview31::kText;
    c.fillRect({horizon::kPlayButton.x, horizon::kPlayButton.y + 5,
                horizon::kPlayButton.w, horizon::kPlayButton.h}, rgba(2, 9, 20, 100), 17);
    c.fillRect(horizon::kPlayButton, preview31::kSelect, 16);
    c.strokeRect(horizon::kPlayButton, rgba(255, 255, 255, 175), 1, 16);
    fonts_.semibold.draw(c, canPlay ? tr(Str::Play) : tr(Str::OpenDetails),
        horizon::kPlayButton.x + 27,
        fonts_.semibold.centeredY(horizon::kPlayButton.y, horizon::kPlayButton.h, 26),
        26, rgba(12, 35, 51));

    if (canPlay) {
        fonts_.semibold.draw(c, tr(Str::OpenDetails), horizon::kDetailsButton.x + 37,
            fonts_.semibold.centeredY(horizon::kDetailsButton.y, horizon::kDetailsButton.h, 26),
            26, primary);
    }

    if (horizonBoundary_.direction && nowMs >= horizonBoundary_.startedMs &&
        nowMs - horizonBoundary_.startedMs <= 1600) {
        fonts_.semibold.draw(c, fonts_.semibold.fit(
            tr(horizonBoundary_.direction > 0 ? Str::HorizonEndHint : Str::HorizonStartHint),
            20, 1190), 104, 511, 20, accent);
        animating_ = true;
    }

    if (drawnCollection_ != rowIndex) {
        outgoingCollection_ = drawnCollection_;
        drawnCollection_ = rowIndex;
        collectionChangedAt_ = nowMs;
    }
    const float elapsed = std::min(1.0f, float(nowMs - collectionChangedAt_) / 280.0f);
    const bool moving = !nimboxReducedMotion_ && outgoingCollection_ >= 0 &&
                        outgoingCollection_ < int(rows_.size()) && elapsed < 1.0f;
    const float ease = elapsed * elapsed * (3.0f - 2.0f * elapsed);
    const int travel = rowIndex > outgoingCollection_ ? 310 : -310;
    collectionOffsetY_ = moving ? int(std::lround(travel * (1.0f - ease))) : 0;
    const auto drawShelf = [&](int shelfIndex, int choice, float slide, int shift) {
        const GameRow& shelf = rows_[size_t(shelfIndex)];
        const auto order = horizon::drawOrder(choice, int(shelf.tiles.size()), slide);
        for (int offset : order) {
            const int index = choice + offset;
            const GameTile& tile = shelf.tiles[size_t(index)];
            const Rect card = horizon::homeCoverRect(float(offset) + slide, shift, settings_.horizonShowcase);
            const bool center = offset == 0;
            if (center) c.fillRect({card.x - 10, card.y - 10, card.w + 20, card.h + 20},
                                   withAlpha(preview31::kAccent, 42), 22);
            c.fillRect({card.x - 5, card.y + 13, card.w + 10, card.h + 8},
                       rgba(0, 5, 12, center ? 145 : 85), 18);
            c.fillRect(card, kPlaceholder, 15);
            if (!tile.tileUrl.empty()) {
                if (auto art = images_.get(tile.tileUrl, card.w, card.h))
                    c.drawImage(*art, card.x, card.y, center ? 255 : 211, 15);
            }
            if (center) {
                c.strokeRect({card.x - 5, card.y - 5, card.w + 10, card.h + 10},
                             preview31::kText, 3, 19);
                c.strokeRect({card.x - 8, card.y - 8, card.w + 16, card.h + 16},
                             withAlpha(preview31::kAccent, 170), 1, 21);
            } else {
                c.fillRect(card, rgba(3, 13, 25, 28), 15);
            }
        }
    };
    c.setClip({96, 610, 1728, 282});
    if (moving) {
        drawShelf(outgoingCollection_, focusCol_[size_t(outgoingCollection_)], 0, collectionOffsetY_ - travel);
        animating_ = true;
    }
    drawShelf(rowIndex, selected, nimboxSlide_.value / 1000.0f, collectionOffsetY_);
    c.resetClip();
    fonts_.semibold.draw(c, fonts_.semibold.fit(collection, 34, 1140), 104, 553, 34, kWhite);
    const std::string collectionPos = trf(Str::HorizonCollectionPosition,
        std::to_string(rowIndex + 1), std::to_string(rows_.size()));
    fonts_.semibold.draw(c, collectionPos, 1816 - fonts_.semibold.measure(collectionPos, 20), 568, 20, soft);
    const std::string position = trf(Str::HorizonGamePosition,
        std::to_string(selected + 1), std::to_string(count));
    fonts_.semibold.draw(c, position, (kW - fonts_.semibold.measure(position, 20)) / 2,
                         906, 20, rgba(221, 239, 246));
    const Rect track{771, 943, 378, 3};
    c.fillRect(track, rgba(214, 236, 245, 53), 2);
    const int progress = std::max(5, (track.w * (selected + 1)) / count);
    c.fillRect({track.x, track.y, progress, track.h}, accent, 2);

    fonts_.regular.draw(c, tr(Str::HorizonHorizontalHint), 104, 972, 18, soft);
    const char* directions = tr(Str::HorizonVerticalHint);
    fonts_.regular.draw(c, directions, 1816 - fonts_.regular.measure(directions, 18),
                        972, 18, soft);

    c.gradientV({0, kH - 82, kW, 82}, rgba(3, 10, 20, 0), rgba(3, 10, 20, 128));
    // On a console, show an understated status only for connected pads.
    const int connected = static_cast<int>(std::count_if(pads_.begin(), pads_.end(),
        [](const PadSlot& p) { return p.connected; }));
    if (connected > 0) {
        c.fillCircle(111.0f, 1028.0f, 6.0f, accent);
        const std::string countLabel = std::to_string(connected) + " / " +
            std::to_string(pads_.size());
        fonts_.semibold.draw(c, countLabel, 132, 1017, 20, soft);
    }
    drawHints(c, {{kIconCross, tr(Str::OpenDetails)}, {kIconR3, tr(Str::Profile)},
                  {kIconTriangle, tr(Str::TabSearch)}, {kIconOptions, tr(Str::Settings)},
                  {kIconTouchpad, tr(Str::HoldSignOut)}});
    drawToast(c, nowMs);
}

void AppUi::drawProfilePanel(Canvas& c) {
    const Rect panel = horizon::kProfilePanel;
    const int x = panel.x + 44, right = panel.x + panel.w - 44;
    c.gradientH({0, 0, panel.x, kH}, rgba(8, 13, 21, 105), rgba(8, 13, 21, 193));
    c.gradientH({panel.x, 0, kW - panel.x, kH}, rgba(8, 13, 21, 193), rgba(8, 13, 21, 235));
    c.fillRect({panel.x, panel.y + 36, 1, panel.h - 84}, withAlpha(kWhite, 56));
    const Color accent = nimbox_theme::palette(settings_.nimboxTheme).accent;
    fonts_.semibold.draw(c, tr(Str::Profile), x, panel.y + 36, 23, kGray);
    const Rect close = horizon::kProfileClose;
    c.line(close.x + 17, close.y + 17, close.x + 31, close.y + 31, 2, kWhite);
    c.line(close.x + 31, close.y + 17, close.x + 17, close.y + 31, 2, kWhite);

    const int cy = panel.y + 155;
    c.fillCircle(x + 52, cy, 50, withAlpha(accent, 45));
    // The header already requests this exact cache size on each platform.
#if defined(__ANDROID__)
    constexpr int avatar = 64;
#else
    constexpr int avatar = 52;
#endif
    auto image = images_.get(gamerpicUrl_, 94, 94);
    if (!image) image = images_.get(gamerpicUrl_, avatar, avatar);
    if (image) {
        c.drawImage(*image, x + 52 - image->w / 2, cy - image->h / 2, 255, -1);
    } else {
        c.fillCircle(x + 52, cy - 9, 12, accent);
        c.fillRect({x + 29, cy + 8, 46, 19}, accent, 9);
    }
    const std::string name = gamertag_.empty() ? tr(Str::Profile) : gamertag_;
    fonts_.bold.draw(c, fonts_.bold.fit(name, 45, right - x - 126), x + 126, cy - 44, 45, kWhite);
    const std::string plan = subscription_ == "GamePass" ? "Game Pass" : subscription_;
    fonts_.semibold.draw(c, fonts_.semibold.fit(plan.empty() ? "Xbox" : plan, 22, 408),
                         x + 128, cy + 22, 22, accent);

    drawProfileActions(c, x, right);
    // Hide base-screen touch targets while this native modal has focus.
    hintRects_.clear();
    c.gradientV({0, kH - 120, kW, 40}, rgba(8, 13, 21, 0), rgba(8, 13, 21));
    c.fillRect({0, kH - 80, kW, 80}, rgba(8, 13, 21));
    drawHints(c, {{kIconCircle, tr(Str::Back)}});
}

void AppUi::drawHome(Canvas& c, uint64_t nowMs) {
    if (searching_) return drawSearch(c, nowMs);
    if (!directory_.empty()) return drawDirectory(c);
    if (tab_ == Tab::Library) return drawLibrary(c, nowMs);
    if (tab_ == Tab::Consoles) return drawConsoles(c, nowMs);
    drawNimboxHorizon(c, nowMs);
}

// Nimbox Showcase: an editorial, console-first detail page. The game image
// occupies the stage; focusable actions use the same native controller/touch
// events as before. No extra service or HTML view is involved.
void AppUi::drawDetails(Canvas& c, uint64_t nowMs) {
    const GameTile* g = &detail_;
    playRect_ = {};
    achRect_ = {};
    claimRect_ = {};
    c.clear(kBg);
    if (g->productId.empty() && g->titleId.empty()) return;

    const bool isF2P = isFreeGame(*g);
    const bool effectivePlayable = isEffectivePlayableLocked(*g);
    const bool needsPurchase = !effectivePlayable && !g->playable && (isF2P || g->purchasable);
    const char* planName = planDisplayName(g->plan);

    if (!drawHero(c, nowMs, g->heroUrl, true)) {
        drawBackground(c);
        // Never reuse the previous game's hero when this one has no art.
        if (auto cover = images_.get(g->tileUrl, 900, 900))
            c.drawImage(*cover, 900, 70, 160, 18);
    }

    // A quiet editorial stage, not a large opaque "app card". The artwork
    // remains visible between the text and the narrow information rail.
    c.gradientH({0, 160, 1130, 750}, rgba(4, 13, 24, 174), rgba(4, 13, 24, 0));
    c.gradientH({1320, 162, 600, 762}, rgba(3, 11, 20, 0), rgba(3, 11, 20, 190));
    c.gradientV({0, 745, kW, 210}, rgba(5, 13, 23, 0), rgba(5, 13, 23, 205));
    c.fillRect(showcase::kFrame, rgba(8, 20, 35, 28), 20);
    c.strokeRect(showcase::kFrame, rgba(216, 232, 247, 76), 1, 20);
    // Fine section markers and a PlayStation-style cool-white focused state.
    c.fillRect({showcase::kTextX, 233, 48, 3}, preview31::kAccent, 2);
    fonts_.semibold.draw(c, tr(Str::DetailsShowcase), showcase::kTextX, 248, 19,
                         preview31::kMuted);

    drawTopBar(c);
    // The gear is also a touch target; L2 opens these system settings from a
    // game detail without replacing the existing Options/game profile menu.
    fonts_.regular.draw(c, tr(Str::SettingsDetailsShortcut), 1584, 141, 18,
                        rgba(197, 220, 235));

    // Main editorial column.
    int titleY = 288;
    const auto titleLines = fonts_.bold.wrap(g->name, 63, showcase::kTextW, 2);
    for (const auto& line : titleLines) {
        fonts_.bold.draw(c, line, showcase::kTextX, titleY, 63, kWhite);
        titleY += 78;
    }
    int byY = titleY + 13;
    std::string byLine;
    if (!g->publisher.empty()) byLine = trf(Str::PublisherBy, g->publisher);
    if (planName && g->playable) {
        if (!byLine.empty()) byLine += "   \xE2\x80\xA2   ";
        byLine += trf(Str::IncludedWithPlan, planName);
    }
    if (!byLine.empty()) {
        fonts_.semibold.draw(c, fonts_.semibold.fit(byLine, 23, showcase::kTextW),
                             showcase::kTextX, byY, 23, rgba(210, 224, 232));
    }

    // A single calm metadata row rather than unstructured text over the image.
    std::string metadata;
    if (!g->ratingAge.empty()) metadata = g->ratingAge;
    if (const char* platform = platformLabel(g->platform)) {
        if (!metadata.empty()) metadata += "   \xE2\x80\xA2   ";
        metadata += platform;
    }
    for (size_t i = 0; i < g->categories.size() && i < 3; ++i) {
        if (!metadata.empty()) metadata += "   \xE2\x80\xA2   ";
        metadata += g->categories[i];
    }
    if (g->mouseKeyboard) {
        if (!metadata.empty()) metadata += "   \xE2\x80\xA2   ";
        metadata += tr(Str::MouseKeyboardSupport);
    }
    if (!metadata.empty()) {
        fonts_.regular.draw(c, fonts_.regular.fit(metadata, 21, showcase::kTextW),
                            showcase::kTextX, byY + 42, 21, rgba(191, 213, 222));
    }

    // About the game. Keep full descriptions in the model, trim ONLY the
    // presentation to leave the action row fixed for all title lengths.
    c.line(showcase::kTextX, 559, showcase::kTextX + 625, 559, 1,
           rgba(208, 232, 239, 90));
    fonts_.semibold.draw(c, tr(Str::DetailsAbout), showcase::kTextX, 580, 24, kWhite);
    int descY = 622;
    for (const auto& line : fonts_.regular.wrap(g->description, 23, 905, needsPurchase ? 2 : 4)) {
        fonts_.regular.draw(c, line, showcase::kTextX, descY, 23,
                            rgba(207, 222, 231));
        descY += 34;
    }

    // Right-hand rail: real Xbox data, with no invented trailers or awards.
    auto panel = [&](Rect bounds) {
        c.fillRect(bounds, rgba(5, 15, 27, 181), 16);
        c.strokeRect(bounds, rgba(209, 231, 241, 72), 1, 16);
    };
    panel(showcase::kAchievementsPanel);
    const int rx = showcase::kRailX + 25;
    fonts_.semibold.draw(c, tr(Str::Achievements), rx, 281, 23, kWhite);
    int unlocked = 0, total = 0, score = 0, possibleScore = 0;
    if (auto it = achievements_.find(g->xboxTitleId); it != achievements_.end()) {
        unlocked = it->second.unlockedCount;
        total = it->second.totalCount;
        score = it->second.unlockedGamerscore;
        possibleScore = it->second.totalGamerscore;
    }
    if (!g->xboxTitleId.empty()) {
        std::string progress = total > 0 ? std::to_string(unlocked) + " / " + std::to_string(total) : "--";
        fonts_.bold.draw(c, progress, rx, 319, 36, kWhite);
        std::string gs = total > 0 ? std::to_string(score) + " / " + std::to_string(possibleScore) + " G" : "";
        fonts_.regular.draw(c, gs, rx + 185, 331, 20, kGray);
        c.fillRect({rx, 392, 354, 8}, rgba(210, 227, 237, 43), 4);
        if (total > 0) {
            int progressW = static_cast<int>(354.0 * std::clamp(unlocked, 0, total) / total);
            if (progressW > 0)
                c.fillRect({rx, 392, progressW, 8}, rgba(226, 248, 255), 4);
        }
    } else {
        fonts_.regular.draw(c, tr(Str::NoAchievements), rx, 334, 22, kGray);
    }

    uint64_t playedSeconds = 0;
    if (!g->productId.empty()) {
        if (auto it = settings_.titlePlaytime.find(g->productId); it != settings_.titlePlaytime.end())
            playedSeconds = it->second;
    }
    if (!playedSeconds && !g->titleId.empty()) {
        if (auto it = settings_.titlePlaytime.find(g->titleId); it != settings_.titlePlaytime.end())
            playedSeconds = it->second;
    }

    panel(showcase::kSessionPanel);
    fonts_.semibold.draw(c, tr(Str::DetailsSession), rx, 489, 22, kWhite);
    int lineY = 527;
    if (queueTitle_ == g->titleId && queueSeconds_ >= 0) {
        const std::string queue = queueSeconds_ < 60
            ? trf(Str::QueueSeconds, std::to_string(queueSeconds_))
            : trf(Str::QueueMinutes, std::to_string((queueSeconds_ + 59) / 60));
        fonts_.regular.draw(c, fonts_.regular.fit(queue, 21, 353), rx, lineY, 21, kGray);
        lineY += 35;
    }
    if (playedSeconds > 0) {
        const std::string played = trf(Str::PlaytimePlayed, formatPlaytime(playedSeconds));
        fonts_.regular.draw(c, fonts_.regular.fit(played, 21, 353), rx, lineY, 21, kGray);
        lineY += 35;
    }
    if (lineY == 527 && planName)
        fonts_.regular.draw(c, planName, rx, lineY, 21, kGray);
    if (lineY == 527 && !planName && !g->publisher.empty())
        fonts_.regular.draw(c, fonts_.regular.fit(g->publisher, 21, 353), rx, lineY, 21, kGray);

    panel(showcase::kArtworkPanel);
    if (!needsPurchase || isF2P) {
    const Rect artwork{showcase::kRailX + 24, 659, 174, 194};
    c.fillRect(artwork, rgba(31, 52, 65), 12);
    if (auto image = images_.get(g->tileUrl, artwork.w, artwork.h))
        c.drawImage(*image, artwork.x, artwork.y, 255, 12);
    int mediaX = artwork.x + artwork.w + 20;
    fonts_.semibold.draw(c, tr(Str::DetailsArtwork), mediaX, 678, 21, kWhite);
    if (planName && g->playable) {
        fonts_.regular.draw(c, fonts_.regular.fit(planName, 18, 171),
                            mediaX, 715, 18, kGray);
    }
    if (!g->ratingSystem.empty())
        fonts_.regular.draw(c, fonts_.regular.fit(g->ratingSystem, 18, 170),
                            mediaX, 751, 18, kGray);
    if (g->mouseKeyboard)
        fonts_.regular.draw(c, fonts_.regular.fit(tr(Str::MouseKeyboardSupport), 18, 170),
                            mediaX, 790, 18, kGray);

    }

    // Premium action states retain the exact account entitlement logic and
    // event mapping already implemented by the Nimbox controller handler.
    if (needsPurchase) {
        if (isF2P) {
            fonts_.semibold.draw(c, tr(Str::FreeToPlay), showcase::kTextX, 718, 22, kWhite);
            const auto hint = fonts_.regular.wrap(tr(Str::FreeAcquireHint), 20, 880, 2);
            int hy = 753;
            for (const auto& line : hint) {
                fonts_.regular.draw(c, line, showcase::kTextX, hy, 20, kGray);
                hy += 27;
            }
            claimRect_ = showcase::kClaimButton;
            c.fillRect(claimRect_, rgba(235, 245, 252), 26);
            if (detailButtonFocus_ == 0)
                c.strokeRect({claimRect_.x - 6, claimRect_.y - 6,
                              claimRect_.w + 12, claimRect_.h + 12}, kWhite, 3, 30);
            const char* label = tr(Str::ClaimFree);
            fonts_.semibold.draw(c, label,
                                 claimRect_.x + (claimRect_.w - fonts_.semibold.measure(label, 28)) / 2,
                                 claimRect_.y + 23, 28, rgba(13, 30, 46));
        } else {
            if (auto it = prices_.find(g->productId); it != prices_.end())
                fonts_.bold.draw(c, it->second.now, showcase::kTextX, 759, 34, kWhite);
            const char* label = tr(Str::BuyToPlay);
            fonts_.semibold.draw(c, label, showcase::kTextX, 832, 25, kWhite);
            std::string storeLocale;
            if (settings_.storeCountry == "br") storeLocale = "pt-BR/";
            else if (settings_.storeCountry == "jp") storeLocale = "ja-JP/";
            else if (settings_.storeCountry == "kr") storeLocale = "ko-KR/";
            else if (settings_.storeCountry == "pl") storeLocale = "pl-PL/";
            else if (settings_.storeCountry == "us") storeLocale = "en-US/";
            Rect qr = drawQr(c, "https://www.xbox.com/" + storeLocale + "games/store/p/" + g->productId,
                             showcase::kRailX + showcase::kRailW - 25, 644, 185);
            if (qr.w > 0) {
                const char* scan = tr(Str::ScanToBuy);
                fonts_.regular.draw(c, fonts_.regular.fit(scan, 16, 205), qr.x, qr.y + qr.h + 10, 16, kWhite);
            }
        }
    } else if (!effectivePlayable) {
        Rect note = showcase::kLockedNotice;
        c.fillRect(note, rgba(8, 22, 36, 215), 26);
        c.strokeRect(note, rgba(228, 239, 245, 110), 1, 26);
        drawLock(c, note.x + 39, note.y + 41);
        const std::string status = isFreeAccountLocked() && !isF2P ? tr(Str::GamePassRequired)
            : planName ? trf(Str::IncludedWithPlan, planName) : std::string(tr(Str::NotPlayable));
        fonts_.semibold.draw(c, fonts_.semibold.fit(status, 24, note.w - 110),
                             note.x + 76, note.y + 25, 24, kWhite);
    } else {
        playRect_ = showcase::kPlayButton;
        const bool playFocused = detailButtonFocus_ == 0;
        c.fillRect(playRect_, playFocused ? rgba(243, 247, 253) : rgba(232, 244, 251, 185), 26);
        if (playFocused)
            c.strokeRect({playRect_.x - 6, playRect_.y - 6,
                          playRect_.w + 12, playRect_.h + 12}, kWhite, 3, 30);
        const Color ink = rgba(11, 30, 48);
        c.line(playRect_.x + 42, playRect_.y + 27, playRect_.x + 42, playRect_.y + 53, 3, ink);
        c.line(playRect_.x + 42, playRect_.y + 27, playRect_.x + 65, playRect_.y + 40, 3, ink);
        c.line(playRect_.x + 42, playRect_.y + 53, playRect_.x + 65, playRect_.y + 40, 3, ink);
        fonts_.semibold.draw(c, tr(Str::Play), playRect_.x + 93, playRect_.y + 23, 31, ink);

        if (!g->xboxTitleId.empty()) {
            achRect_ = showcase::kAchievementsButton;
            const bool focused = detailButtonFocus_ == 1;
            c.fillRect(achRect_, focused ? rgba(243, 247, 253) : rgba(12, 27, 43, 210), 26);
            c.strokeRect(achRect_, rgba(213, 233, 243, 130), focused ? 3 : 1, 26);
            if (focused)
                c.strokeRect({achRect_.x - 6, achRect_.y - 6,
                              achRect_.w + 12, achRect_.h + 12}, kWhite, 3, 30);
            drawGamerscoreBadge(c, static_cast<float>(achRect_.x + 44),
                                static_cast<float>(achRect_.y + 39), 13);
            fonts_.semibold.draw(c, fonts_.semibold.fit(tr(Str::Achievements), 24, achRect_.w - 90),
                                 achRect_.x + 74, achRect_.y + 27, 24,
                                 focused ? ink : kWhite);
        }
    }

    preview31::footerBackdrop(c);
    std::vector<Hint> hints;
    if (claimRect_.w > 0) hints.push_back({kIconCross, tr(Str::ClaimFree)});
    else if (playRect_.w > 0) hints.push_back({kIconCross, tr(detailButtonFocus_ == 1 && achRect_.w > 0 ? Str::Achievements : Str::Play)});
    hints.push_back({kIconOptions, tr(Str::GameSettings)});
    hints.push_back({kIconTriangle, tr(favorites_.count(g->productId) ? Str::RemoveFavorite : Str::AddFavorite)});
    hints.push_back({kIconSquare, tr(hidden_.count(g->productId) ? Str::Unhide : Str::Hide)});
    if (!isAcquired(*g)) hints.push_back({kIconR3, tr(wishlist_.count(g->productId) ? Str::RemoveWishlist : Str::AddWishlist)});
    if (!g->xboxTitleId.empty() && achRect_.w == 0) hints.push_back({kIconL3, tr(Str::Achievements)});
    hints.push_back({kIconCircle, tr(Str::Back)});
    drawHints(c, hints);
    drawToast(c, nowMs);
}

void AppUi::drawLaunching(Canvas& c, uint64_t nowMs) {
    c.clear(kBg);
    if (auto hero = images_.get(launching_.heroUrl, kW, kH)) c.drawImage(*hero, 0, 0);
    c.fillRect({0, 0, kW, kH}, rgba(0, 0, 0, 175));
    drawSpinner(c, kW / 2.0f, 420, 40, nowMs);
    drawCentered(c, fonts_.bold, launching_.name, 500, 48, kWhite);
    drawCentered(c, fonts_.semibold, tr(Str::GettingReady), 576, 30, kGray);
    drawCentered(c, fonts_.regular, status_, 630, 26, kDim);
    // Helpful tips rotating every ~7 seconds from when the launching screen opened.
    static constexpr Str kTips[] = {
        Str::TipTouchpadMenu,
        Str::TipKeyboardMouse,
        Str::TipFreeStore,
        Str::TipBugReportQr,
        Str::TipSupportKofi,
        Str::Tip120HzVideo,
        Str::TipAnalogDeadzone,
        Str::TipDualSenseMic,
        Str::TipTouchpadMouse,
        Str::TipGyroAiming,
        Str::TipFSRUpscaling,
        Str::TipCustomBitrate,
        Str::TipNativeTrophies,
        Str::TipPerGameTrophies,
        Str::TipSmartGlassWebUI,
        Str::TipClaimFreeStore,
        Str::TipButtonRemap,
        Str::TipInstantResume,
        Str::TipInGameMenu,
        Str::TipBackgroundSync,
    };
    constexpr size_t kTipCount = sizeof(kTips) / sizeof(kTips[0]);
    if (!launchingStartMs_) {
        launchingStartMs_ = nowMs;
        uint32_t titleHash = 2166136261u;
        for (char ch : launching_.name) {
            titleHash = (titleHash ^ static_cast<uint8_t>(ch)) * 16777619u;
        }
        launchingSeed_ = static_cast<size_t>(nowMs ^ titleHash);
    }
    uint64_t elapsedMs = nowMs >= launchingStartMs_ ? (nowMs - launchingStartMs_) : 0;
    size_t tipIndex = (launchingSeed_ + (elapsedMs / 7000)) % kTipCount;
    drawCentered(c, fonts_.regular, tr(kTips[tipIndex]), 900, 22, kDim);
    drawHints(c, {{kIconCircle, tr(Str::Cancel)}});
}

void AppUi::drawError(Canvas& c) {
    drawBackground(c);
    int panelW = errorUrl_.empty() ? 1040 : 1300;
    int maxTextW = errorUrl_.empty() ? (panelW - 112) : (panelW - 460);
    auto lines = fonts_.regular.wrap(error_, 26, maxTextW, 8);
    int h = 132 + static_cast<int>(lines.size()) * 38 + 56;
    if (!errorUrl_.empty() && h < 460) h = 460;
    Rect panel{(kW - panelW) / 2, (kH - h) / 2 - 40, panelW, h};
    c.fillRect(panel, kPanel, 24);
    fonts_.bold.draw(c, tr(Str::ErrorTitle), panel.x + 56, panel.y + 52, 44, kWhite);
    int y = panel.y + 132;
    for (const auto& line : lines) {
        fonts_.regular.draw(c, line, panel.x + 56, y, 26, kGray);
        y += 38;
    }
    if (!errorUrl_.empty()) {
        y += 16;
        for (const auto& line : fonts_.regular.wrap(tr(Str::ScanQrToResolve), 24, maxTextW, 4)) {
            fonts_.regular.draw(c, line, panel.x + 56, y, 24, rgba(220, 220, 255));
            y += 34;
        }
        drawQr(c, errorUrl_, panel.x + panel.w - 56, panel.y + 56, 320, qrcodegen_Ecc_LOW);
    }
    if (!errorGame_.titleId.empty() || !errorGame_.productId.empty()) {
        bool free = (isFreeGame(errorGame_) || errorGame_.freeInStore) && !errorGame_.playable;
        const char* acceptLabel = free ? tr(Str::ClaimFree) : tr(Str::TryAgain);
        drawHints(c, {{kIconCross, acceptLabel}, {kIconTriangle, tr(Str::ReportProblem)}, {kIconCircle, tr(Str::Back)}});
    }
    else if (regionRefused_)
        drawHints(c, {{kIconCross, tr(Str::TryAgain)}, {kIconSquare, tr(Str::TryOtherCountry)},
                      {kIconOptions, tr(Str::Settings)}, {kIconTriangle, tr(Str::ReportProblem)},
                      {kIconTouchpad, tr(Str::HoldSignOut)}});
    else if (!rows_.empty() || !owned_.empty())
        drawHints(c, {{kIconCross, tr(Str::TryAgain)}, {kIconSquare, tr(Str::OfflineMode)},
                      {kIconTriangle, tr(Str::ReportProblem)}, {kIconTouchpad, tr(Str::HoldSignOut)}});
    else
        drawHints(c, {{kIconCross, tr(Str::TryAgain)}, {kIconTriangle, tr(Str::ReportProblem)},
                      {kIconTouchpad, tr(Str::HoldSignOut)}});
}

void AppUi::drawReport(Canvas& c) {
    // A panel over the screen: the heading and what the QR code does on the
    // left, the code on the right (dense: about 1.8 KB, so large).
    c.fillRect({0, 0, kW, kH}, rgba(0, 0, 0, 170));
    constexpr int kPanelW = 1500, kPanelH = 720;
    Rect panel{(kW - kPanelW) / 2, (kH - kPanelH) / 2 - 30, kPanelW, kPanelH};
    c.fillRect({panel.x + 8, panel.y + 12, panel.w, panel.h}, rgba(0, 0, 0, 120), 26);
    c.fillRect(panel, kPanel, 24);
    c.strokeRect(panel, rgba(255, 255, 255, 50), 2, 24);
    Rect qr = drawQr(c, reportUrl_, panel.x + panel.w - 48, panel.y + 48, 600, qrcodegen_Ecc_LOW);
    int textW = (qr.w ? qr.x : panel.x + panel.w) - panel.x - 112;
    int y = panel.y + 56;
    for (const auto& line : fonts_.bold.wrap(reportHeading_, 40, textW, 3)) {
        fonts_.bold.draw(c, line, panel.x + 56, y, 40, kWhite);
        y += 54;
    }
    y += 24;
    for (Str s : {Str::ReportScan, Str::ReportIncludes}) {
        for (const auto& line : fonts_.regular.wrap(tr(s), 26, textW, 4)) {
            fonts_.regular.draw(c, line, panel.x + 56, y, 26, kGray);
            y += 38;
        }
        y += 20;
    }
    if (!reportDetail_.empty()) {
        y += 8;
        for (const auto& line : fonts_.regular.wrap(reportDetail_, 22, textW, 3)) {
            fonts_.regular.draw(c, line, panel.x + 56, y, 22, kWhite);
            y += 32;
        }
    }

    // Touch / visual buttons
    Rect closeBtn{panel.x + panel.w - 64, panel.y + 16, 48, 48};
    c.fillRect(closeBtn, rgba(255, 255, 255, 30), 12);
    int cw = fonts_.bold.measure("✕", 24);
    fonts_.bold.draw(c, "✕", closeBtn.x + (closeBtn.w - cw) / 2, fonts_.bold.centeredY(closeBtn.y, closeBtn.h, 24), 24, kWhite);

    Rect backBtn{panel.x + 56, panel.y + panel.h - 80, 220, 54};
    c.fillRect(backBtn, rgba(60, 60, 60, 220), 14);
    c.strokeRect(backBtn, rgba(255, 255, 255, 50), 1, 14);
    int bw = fonts_.semibold.measure(tr(Str::Back), 22);
    fonts_.semibold.draw(c, tr(Str::Back), backBtn.x + (backBtn.w - bw) / 2,
                         fonts_.semibold.centeredY(backBtn.y, backBtn.h, 22), 22, kWhite);

    if (caps_.openUrl
#if defined(__ANDROID__)
        || true
#endif
    ) {
        Rect urlBtn{backBtn.x + backBtn.w + 20, backBtn.y, 340, 54};
        c.fillRect(urlBtn, rgba(16, 124, 65, 220), 14);
        c.strokeRect(urlBtn, rgba(30, 180, 90, 200), 1, 14);
        int uw = fonts_.semibold.measure(tr(Str::OpenInBrowser), 22);
        fonts_.semibold.draw(c, tr(Str::OpenInBrowser), urlBtn.x + (urlBtn.w - uw) / 2,
                             fonts_.semibold.centeredY(urlBtn.y, urlBtn.h, 22), 22, kWhite);
    }

    c.fillRect({0, kH - 80, kW, 80}, kBg);
    if (caps_.openUrl)  // a desktop: Cross opens the form in the browser
        drawHints(c, {{kIconCross, tr(Str::OpenInBrowser)}, {kIconCircle, tr(Str::Back)}});
    else
        drawHints(c, {{kIconCircle, tr(Str::Back)}});
}

void AppUi::drawCountryPicker(Canvas& c) {
    // Over the refused sign-in: the warning, then the countries.
    constexpr int kPanelW = 1100, kItemH = 58;
    auto warning = fonts_.regular.wrap(tr(Str::LoginCountryWarning), 26, kPanelW - 112, 6);
    int n = 1 + static_cast<int>(std::size(kLoginCountries));
    int listY = 124 + static_cast<int>(warning.size()) * 36 + 24;
    Rect panel = drawModal(c, kPanelW, listY + n * kItemH + 40, tr(Str::TryOtherCountry));
    int y = panel.y + 124;
    for (const auto& line : warning) {
        fonts_.regular.draw(c, line, panel.x + 56, y, 26, kGray);
        y += 36;
    }
    int sel = settingSelected(8);
    for (int i = 0; i < n; ++i) {
        Rect item{panel.x + 48, panel.y + listY + i * kItemH, panel.w - 96, kItemH};
        bool on = i == countryIndex_;
        if (on) c.fillRect(item, kWhite, 12);
        std::string label = i ? loginCountryName(kLoginCountries[i - 1]) : loginCountryOffText();
        fonts_.semibold.draw(c, label, item.x + 24, fonts_.semibold.centeredY(item.y, item.h, 26), 26,
                             on ? kBg : (i == sel ? kWhite : kGray));
        if (i == sel) {  // a check mark on the current choice
            float cx = static_cast<float>(item.x + item.w - 36), cy = static_cast<float>(item.y + item.h / 2);
            c.line(cx - 10, cy, cx - 3, cy + 7, 3, on ? kBg : kGreen);
            c.line(cx - 3, cy + 7, cx + 10, cy - 7, 3, on ? kBg : kGreen);
        }
    }
}

void AppUi::drawQuitPrompt(Canvas& c) {
    // As the update pop-up: the question, then Close and Cancel as buttons.
    constexpr int kButtonW = 300, kButtonH = 72, kPanelW = 2 * kButtonW + 24 + 112, kPanelH = 250;
    Rect panel = drawModal(c, kPanelW, kPanelH, tr(Str::QuitTitle));
    const char* labels[2] = {tr(Str::QuitConfirm), tr(Str::Cancel)};
    for (int i = 0; i < 2; ++i) {
        Rect b{panel.x + 56 + i * (kButtonW + 24), panel.y + panel.h - kButtonH - 44, kButtonW, kButtonH};
        bool on = i == promptFocus_;
        c.fillRect(b, on ? kWhite : rgba(255, 255, 255, 30), 14);
        int tw = fonts_.semibold.measure(labels[i], 28);
        fonts_.semibold.draw(c, labels[i], b.x + (b.w - tw) / 2, fonts_.semibold.centeredY(b.y, b.h, 28), 28,
                             on ? kBg : kWhite);
    }
    c.fillRect({0, kH - 80, kW, 80}, kBg);  // its own hints over drawModal's
    drawHints(c, {{kIconCross, tr(Str::Select)}, {kIconCircle, tr(Str::Cancel)}});
}

void AppUi::drawUpdatePrompt(Canvas& c) {
    c.fillRect({0, 0, kW, kH}, rgba(0, 0, 0, 160));
    constexpr int kPanelW = 960, kPanelH = 340, kButtonW = 260, kButtonH = 72;
    Rect panel{(kW - kPanelW) / 2, (kH - kPanelH) / 2 - 40, kPanelW, kPanelH};
    c.fillRect({panel.x + 8, panel.y + 12, panel.w, panel.h}, rgba(0, 0, 0, 120), 26);  // shadow
    c.fillRect(panel, kPanel, 24);
    c.strokeRect(panel, rgba(255, 255, 255, 50), 2, 24);
    fonts_.bold.draw(c, trf(Str::UpdateAvailable, updateAvailable_), panel.x + 56, panel.y + 48, 44, kWhite);
    auto lines = fonts_.regular.wrap(tr(Str::UpdatePrompt), 26, kPanelW - 112, 2);
    for (size_t i = 0; i < lines.size(); ++i)
        fonts_.regular.draw(c, lines[i], panel.x + 56, panel.y + 124 + static_cast<int>(i) * 36, 26, kGray);

    // 3 buttons: Update now (0), What's new (1), Not now (2)
    const char* labels[3] = {tr(Str::UpdateNow), tr(Str::WhatsNew), tr(Str::NotNow)};
    constexpr int kGap = (kPanelW - 112 - 3 * kButtonW) / 2;
    for (int i = 0; i < 3; ++i) {
        Rect b{panel.x + 56 + i * (kButtonW + kGap), panel.y + panel.h - kButtonH - 44, kButtonW, kButtonH};
        bool on = i == promptFocus_;
        c.fillRect(b, on ? kWhite : rgba(255, 255, 255, 30), 14);
        int tw = fonts_.semibold.measure(labels[i], 24);
        fonts_.semibold.draw(c, labels[i], b.x + (b.w - tw) / 2, fonts_.semibold.centeredY(b.y, b.h, 24), 24,
                             on ? kBg : kWhite);
    }
    c.fillRect({0, kH - 80, kW, 80}, kBg);  // the home screen's hints, under the pop-up's
    drawPads(c);
    drawHints(c, {
        {kIconCross, tr(Str::Select)},
        {kIconSquare, tr(Str::WhatsNew)},
        {kIconCircle, tr(Str::NotNow)}
    });
}

void AppUi::formatUpdateNotes() {
    updateNotesLines_.clear();
    std::string lang = languageCode(language());
    updateNotesLang_ = lang;
    std::string raw = xc::app::filterReleaseNotes(updateNotes_, lang);
    if (raw.empty()) raw = updateNotes_;
    if (raw.empty()) raw = "No release notes provided.";

    constexpr int kModalW = 1200, kModalH = 740;
    int textW = kModalW - 128 - 28;
    int textH = kModalH - 175;

    size_t pos = 0;
    while (pos < raw.size()) {
        size_t next = raw.find('\n', pos);
        std::string rawLine = raw.substr(pos, next == std::string::npos ? std::string::npos : next - pos);
        if (!rawLine.empty() && rawLine.back() == '\r') rawLine.pop_back();

        if (rawLine.rfind("### ", 0) == 0) {
            std::string sub = rawLine.substr(4);
            auto wrapped = fonts_.semibold.wrap(sub, 22, textW, 4);
            for (const auto& w : wrapped) updateNotesLines_.push_back({w, false, true, false, false});
        } else if (rawLine.rfind("## ", 0) == 0) {
            std::string hdr = rawLine.substr(3);
            auto wrapped = fonts_.bold.wrap(hdr, 26, textW, 4);
            for (const auto& w : wrapped) updateNotesLines_.push_back({w, true, false, false, false});
        } else if (rawLine.rfind("# ", 0) == 0) {
            std::string hdr = rawLine.substr(2);
            auto wrapped = fonts_.bold.wrap(hdr, 28, textW, 4);
            for (const auto& w : wrapped) updateNotesLines_.push_back({w, true, false, false, false});
        } else if (rawLine.rfind("- ", 0) == 0 || rawLine.rfind("* ", 0) == 0) {
            std::string bullet = "•  " + rawLine.substr(2);
            auto wrapped = fonts_.regular.wrap(bullet, 22, textW, 6);
            for (size_t wi = 0; wi < wrapped.size(); ++wi) {
                updateNotesLines_.push_back({wrapped[wi], false, false, wi == 0, wi > 0});
            }
        } else if (!rawLine.empty()) {
            auto wrapped = fonts_.regular.wrap(rawLine, 22, textW, 8);
            for (const auto& w : wrapped) updateNotesLines_.push_back({w, false, false, false, false});
        } else {
            updateNotesLines_.push_back({"", false, false, false, false});
        }

        if (next == std::string::npos) break;
        pos = next + 1;
    }

    constexpr int kLineH = 32;
    int visibleLines = textH / kLineH;
    int totalLines = static_cast<int>(updateNotesLines_.size());
    updateNotesMaxScroll_ = std::max(0, totalLines - visibleLines);
    updateNotesScroll_ = std::clamp(updateNotesScroll_, 0, updateNotesMaxScroll_);
}

void AppUi::drawUpdateNotesModal(Canvas& c) {
    std::string currentLang = languageCode(language());
    if (updateNotesLines_.empty() || updateNotesLang_ != currentLang) {
        formatUpdateNotes();
    }

    std::string title = tr(Str::WhatsNewTitle);
    if (!updateAvailable_.empty()) {
        title += " - " + updateAvailable_;
    }
    constexpr int kModalW = 1200, kModalH = 740;
    Rect panel = drawModal(c, kModalW, kModalH, title);

    int textX = panel.x + 64;
    int textY = panel.y + 115;
    int textH = panel.h - 175;

    constexpr int kLineH = 32;
    int visibleLines = textH / kLineH;
    int totalLines = static_cast<int>(updateNotesLines_.size());
    updateNotesScroll_ = std::clamp(updateNotesScroll_, 0, updateNotesMaxScroll_);

    c.fillRect({panel.x + 44, textY - 12, panel.w - 88, textH + 24}, rgba(18, 18, 18, 220), 12);
    c.strokeRect({panel.x + 44, textY - 12, panel.w - 88, textH + 24}, rgba(255, 255, 255, 25), 1, 12);

    int curY = textY;
    int endIdx = std::min(totalLines, updateNotesScroll_ + visibleLines);
    for (int i = updateNotesScroll_; i < endIdx; ++i) {
        const auto& fl = updateNotesLines_[static_cast<size_t>(i)];
        if (!fl.text.empty()) {
            if (fl.isHeader) {
                fonts_.bold.draw(c, fl.text, textX, curY, 26, kWhite);
            } else if (fl.isSubHeader) {
                fonts_.semibold.draw(c, fl.text, textX, curY, 22, rgba(120, 220, 120));
            } else if (fl.isBullet) {
                fonts_.regular.draw(c, fl.text, textX + 6, curY, 22, rgba(235, 235, 235));
            } else if (fl.isBulletContinuation) {
                fonts_.regular.draw(c, fl.text, textX + 28, curY, 22, rgba(235, 235, 235));
            } else {
                fonts_.regular.draw(c, fl.text, textX, curY, 22, kGray);
            }
        }
        curY += kLineH;
    }

    if (totalLines > visibleLines) {
        int barTrackX = panel.x + panel.w - 60;
        int barTrackY = textY - 4;
        int barTrackH = textH + 8;
        c.fillRect({barTrackX, barTrackY, 6, barTrackH}, rgba(255, 255, 255, 30), 3);
        float ratio = static_cast<float>(visibleLines) / totalLines;
        int thumbH = std::max(32, static_cast<int>(barTrackH * ratio));
        float scrollPct = updateNotesMaxScroll_ > 0 ? static_cast<float>(updateNotesScroll_) / updateNotesMaxScroll_ : 0.0f;
        int thumbY = barTrackY + static_cast<int>((barTrackH - thumbH) * scrollPct);
        c.fillRect({barTrackX, thumbY, 6, thumbH}, kWhite, 3);
    }

    c.fillRect({0, kH - 80, kW, 80}, kBg);
    drawPads(c);
    drawHints(c, {
        {kIconCircle, tr(Str::Back)}
    });
}

void AppUi::drawAchievementsModal(Canvas& c, uint64_t nowMs) {
    Rect panel = drawModal(c, 1180, 720, tr(Str::Achievements));

    auto it = achievements_.find(detail_.xboxTitleId);
    if (it == achievements_.end()) {
        drawSpinner(c, static_cast<float>(panel.x + panel.w / 2), static_cast<float>(panel.y + panel.h / 2), 24.0f, nowMs);
        animating_ = true;
        c.fillRect({0, kH - 80, kW, 80}, kBg);
        drawHints(c, {{kIconCircle, tr(Str::Back)}});
        return;
    }

    const auto& ach = it->second;
    if (ach.totalCount == 0) {
        std::string emptyStr = tr(Str::NoAchievements);
        int tw = fonts_.regular.measure(emptyStr, 28);
        fonts_.regular.draw(c, emptyStr, panel.x + (panel.w - tw) / 2, panel.y + panel.h / 2, 28, kDim);
        c.fillRect({0, kH - 80, kW, 80}, kBg);
        drawHints(c, {{kIconCircle, tr(Str::Back)}});
        return;
    }

    std::string summary = std::to_string(ach.unlockedCount) + " / " + std::to_string(ach.totalCount) +
                          " (" + std::to_string(ach.unlockedGamerscore) + " / " + std::to_string(ach.totalGamerscore) + " G)";
    int sw = fonts_.semibold.measure(summary, 24);
    fonts_.semibold.draw(c, summary, panel.x + panel.w - 56 - sw, panel.y + 56, 24, kWhite);

    int barX = panel.x + 56;
    int barY = panel.y + 104;
    int barW = panel.w - 112;
    int barH = 10;
    c.fillRect({barX, barY, barW, barH}, rgba(255, 255, 255, 30), 5);
    if (ach.totalCount > 0 && ach.unlockedCount > 0) {
        int fillW = std::clamp(static_cast<int>(barW * static_cast<float>(ach.unlockedCount) / ach.totalCount), 10, barW);
        c.fillRect({barX, barY, fillW, barH}, kGreen, 5);
    }

    constexpr int kRowH = 80;
    constexpr int kGap = 8;
    constexpr int kVisibleCount = 5;
    int y = barY + 22;

    int total = static_cast<int>(ach.items.size());
    int startIdx = std::clamp(achScroll_, 0, std::max(0, total - kVisibleCount));

    for (int i = startIdx; i < total && i < startIdx + kVisibleCount; ++i) {
        const auto& item = ach.items[i];
        Rect r{panel.x + 56, y, panel.w - 112, kRowH};
        c.fillRect(r, rgba(255, 255, 255, 14), 14);

        float cx = static_cast<float>(r.x + 36), cy = static_cast<float>(r.y + r.h / 2);
        if (item.unlocked) {
            c.fillCircle(cx, cy, 18.0f, kGreen);
            c.line(cx - 7, cy, cx - 2, cy + 6, 3.0f, kWhite);
            c.line(cx - 2, cy + 6, cx + 9, cy - 6, 3.0f, kWhite);
        } else {
            c.fillCircle(cx, cy, 18.0f, rgba(255, 255, 255, 20));
            c.strokeCircle(cx, cy - 3.0f, 4.0f, 2.0f, kDim);
            c.fillRect({static_cast<int>(cx) - 6, static_cast<int>(cy) - 2, 12, 10}, kDim, 2);
        }

        int textX = r.x + 72;
        int maxTextW = r.w - 200;
        if (item.unlocked) {
            fonts_.semibold.draw(c, item.name, textX, r.y + 14, 24, kWhite);
            std::string desc = fonts_.regular.fit(item.description, 20, maxTextW);
            fonts_.regular.draw(c, desc, textX, r.y + 44, 20, kGray);
        } else if (item.isSecret) {
            fonts_.semibold.draw(c, tr(Str::SecretAchievement), textX, r.y + 14, 24, kDim);
            fonts_.regular.draw(c, tr(Str::AchievementLocked), textX, r.y + 44, 20, rgba(255, 255, 255, 90));
        } else {
            fonts_.semibold.draw(c, item.name, textX, r.y + 14, 24, rgba(230, 230, 230));
            std::string desc = item.lockedDescription.empty() ? item.description : item.lockedDescription;
            desc = fonts_.regular.fit(desc, 20, maxTextW);
            fonts_.regular.draw(c, desc, textX, r.y + 44, 20, kDim);
        }

        std::string score = std::to_string(item.gamerscore) + " G";
        int scw = fonts_.semibold.measure(score, 22);
        Color scCol = item.unlocked ? rgba(110, 225, 120) : kDim;
        fonts_.semibold.draw(c, score, r.x + r.w - 28 - scw, fonts_.semibold.centeredY(r.y, r.h, 22), 22, scCol);

        y += kRowH + kGap;
    }

    if (total > kVisibleCount) {
        int trackH = kVisibleCount * (kRowH + kGap) - kGap;
        int trackY = barY + 22;
        int trackX = panel.x + panel.w - 40;
        c.fillRect({trackX, trackY, 4, trackH}, rgba(255, 255, 255, 20), 2);
        int thumbH = std::max(20, trackH * kVisibleCount / total);
        int thumbY = trackY + (trackH - thumbH) * startIdx / (total - kVisibleCount);
        c.fillRect({trackX, thumbY, 4, thumbH}, rgba(255, 255, 255, 120), 2);
    }

    c.fillRect({0, kH - 80, kW, 80}, kBg);
    drawHints(c, {{kIconCircle, tr(Str::Back)}});
}

// Apply a separable binomial blur to the small snapshot before scaling it back up.
static Image frostSettingsBackdrop(const Image& capture) {
    Image small = coverResize(capture, 240, 135);
    Image pass = small;
    constexpr int weights[] = {1, 6, 15, 20, 15, 6, 1};
    for (int axis = 0; axis < 2; ++axis) {
        for (int y = 0; y < small.h; ++y) {
            for (int x = 0; x < small.w; ++x) {
                int red = 0, green = 0, blue = 0;
                for (int offset = -3; offset <= 3; ++offset) {
                    const int sx = axis == 0 ? std::clamp(x + offset, 0, small.w - 1) : x;
                    const int sy = axis == 1 ? std::clamp(y + offset, 0, small.h - 1) : y;
                    const uint32_t pixel = small.px[sy * small.w + sx];
                    const int weight = weights[offset + 3];
                    red += (pixel & 255) * weight;
                    green += ((pixel >> 8) & 255) * weight;
                    blue += ((pixel >> 16) & 255) * weight;
                }
                pass.px[y * small.w + x] = 0xff000000u | ((red + 32) / 64) |
                    (((green + 32) / 64) << 8) | (((blue + 32) / 64) << 16);
            }
        }
        small.px.swap(pass.px);
    }
    return coverResize(small, kW, kH);
}

void AppUi::render(Canvas& c, uint64_t nowMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    hintRects_.clear();
    float dt = lastRender_ ? std::min(0.1f, (nowMs - lastRender_) / 1000.0f) : 0.0f;
    lastRender_ = nowMs;
    animating_ = rowY_.step(dt);
    for (auto& a : rowScroll_) animating_ |= a.step(dt);
    animating_ |= gridScroll_.step(dt);
    animating_ |= resultScroll_.step(dt);
    // Horizon's cover navigation uses a framerate-independent ease-out.
    // Reduced-motion preference eliminates the transition.
    if (nimboxReducedMotion_) {
        nimboxSlide_.value = nimboxSlide_.target;
    } else {
        const float remaining = nimboxSlide_.target - nimboxSlide_.value;
        if (std::fabs(remaining) < 0.5f) {
            nimboxSlide_.value = nimboxSlide_.target;
        } else {
            const float blend = 1.0f - std::exp(-6.7f * dt);
            nimboxSlide_.value += remaining * std::clamp(blend, 0.0f, 1.0f);
            animating_ = true;
        }
    }
    dirty_ = false;
    switch (screen_) {
        case Screen::Splash: drawSplash(c, nowMs); break;
        case Screen::SignIn: drawSignIn(c, nowMs); break;
        case Screen::Home: drawHome(c, nowMs); break;
        case Screen::Details:
            drawDetails(c, nowMs);
            if (gameSettingsOpen_) drawGameSettings(c);
            if (achievementsOpen_) drawAchievementsModal(c, nowMs);
            break;
        case Screen::Launching: drawLaunching(c, nowMs); break;
        case Screen::Error: drawError(c); break;
        case Screen::Settings: {
            // One-shot software frost: render the screen that opened Settings,
            // and filter its snapshot. No per-frame 1080p blur cost and no
            // GPU/MediaCodec dependency on Android, desktop or PS5.
            if (!settingsBackdropReady_) {
                Canvas base(kW, kH, rgba(4, 12, 23));
                if (settingsReturnScreen_ == Screen::Details)
                    drawDetails(base, nowMs);
                else if (settingsReturnScreen_ == Screen::Home)
                    drawHome(base, nowMs);
                else
                    drawError(base);
                Image capture;
                capture.w = kW;
                capture.h = kH;
                capture.opaque = true;
                capture.px.assign(base.data(), base.data() + static_cast<size_t>(kW) * kH);
                settingsFrostedBackdrop_ = frostSettingsBackdrop(capture);
                settingsBackdropReady_ = true;
            }
            if (settingsBackdropReady_) c.drawImage(settingsFrostedBackdrop_, 0, 0);
            else drawBackground(c);
            hintRects_.clear();
            drawSettings(c);
            if (settingsQrExpanded_) drawSettingsQrOverlay(c);
            break;
        }
        case Screen::Updating: drawUpdating(c, nowMs); break;
        case Screen::Streaming: break;
    }
    if (screen_ == Screen::Home && profileOpen_) drawProfilePanel(c);
    if (screen_ == Screen::Home && updatePrompt_) {
        if (showingUpdateNotes_) drawUpdateNotesModal(c);
        else drawUpdatePrompt(c);
    }
    if (screen_ == Screen::Home && quitPrompt_) drawQuitPrompt(c);
    if (screen_ == Screen::Error && countryPickerOpen_) drawCountryPicker(c);
    if (reportOpen_) drawReport(c);
    if ((screen_ == Screen::Home && !updatePrompt_ && !profileOpen_) || screen_ == Screen::Error) drawSignOutHold(c, nowMs);
    if (!toast_.empty()) animating_ = true;  // to expire it
    if (platform::toastsEnabled()) {
        // platform::notify() on a desktop: over everything, under the top bar's right side.
        toastsDrawn_ = platform::toasts().version(nowMs);
        drawToasts(c, fonts_, platform::toasts().visible(nowMs), kW - kMargin, 132);
    }
    if (mouseCursorVisible_ && nowMs >= mouseCursorVisibleUntil_) {
        mouseCursorVisible_ = false;
    }
    if (mouseCursorVisible_ && screen_ != Screen::Streaming && screen_ != Screen::Splash) {
        drawMouseCursor(c, mouseCursorX_, mouseCursorY_);
    }
}

static const char* const kCursorRows[23] = {
    "#...............",
    "##..............",
    "#W#.............",
    "#WW#............",
    "#WWW#...........",
    "#WWWW#..........",
    "#WWWWW#.........",
    "#WWWWWW#........",
    "#WWWWWWW#.......",
    "#WWWWWWWW#......",
    "#WWWWWWWWW#.....",
    "#WWWWWWWWWW#....",
    "#WWWWWWWWWWW#...",
    "#WWWWWW######...",
    "#WWWW##WW#......",
    "#WW##.#WW#......",
    "#W#...#WW#......",
    "##.....#WW#.....",
    "#......#WW#.....",
    "........#WW#....",
    "........#WW#....",
    ".........#WW#...",
    ".........####..."
};

void AppUi::drawMouseCursor(Canvas& c, float cursorX, float cursorY) {
    static bool s_inited = false;
    static uint8_t s_outline[23 * 16];
    static uint8_t s_fill[23 * 16];
    static uint8_t s_shadow[23 * 16];
    if (!s_inited) {
        s_inited = true;
        for (int y = 0; y < 23; ++y) {
            for (int x = 0; x < 16; ++x) {
                char ch = kCursorRows[y][x];
                s_shadow[y * 16 + x] = (ch == '#' || ch == 'W') ? 255 : 0;
                s_outline[y * 16 + x] = (ch == '#') ? 255 : 0;
                s_fill[y * 16 + x] = (ch == 'W') ? 255 : 0;
            }
        }
    }
    int ix = static_cast<int>(std::round(cursorX));
    int iy = static_cast<int>(std::round(cursorY));
    // Sombra sutil
    c.drawMask(s_shadow, 16, 23, 16, ix + 2, iy + 2, rgba(0, 0, 0, 80));
    // Borda preta fina
    c.drawMask(s_outline, 16, 23, 16, ix, iy, rgba(0, 0, 0, 240));
    // Seta branca nítida
    c.drawMask(s_fill, 16, 23, 16, ix, iy, rgba(255, 255, 255, 255));
}


}  // namespace xc::ui
