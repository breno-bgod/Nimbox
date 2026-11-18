// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 RafaelNGP
// PSBox Cloud Gaming's mark: a white sphere crossed by two curved swooshes
// that pinch at the centre, on Xbox green. Original artwork (not the Xbox
// logo); drawn with the Canvas so the in-app logo and the PS5 home-screen
// icon (ps5/sce_sys/icon0.png, see `xcloud-cli render-icon`) always match.
#pragma once

#include "ui/canvas.h"
#include "ui/font.h"

namespace xc::ui {

constexpr Color kBrandGreen = rgba(16, 124, 16);

// User-provided NimboPlay wordmark; embedded PNG, transparent and platform-neutral.
// Draws to a contained rectangle without stretching, cropping or network access.
void drawInternalBrand(Canvas& c, int x, int y, int w, int h);

// The old technical mark is retained solely for the existing external
// launcher / PS5 sce_sys icon generation until a separate icon is approved.
void drawBrandMark(Canvas& c, float cx, float cy, float r, Color color = rgba(255, 255, 255));

// User-provided Nimble Play cloud + wordmark. INTERNAL menus only; embedded
// as PNG in the native executable and drawn without any network request.
// The external launcher icon is intentionally separate.
void drawInternalBrand(Canvas& c, Rect bounds);

// The square app icon (green field, mark, "PSBox" wordmark).
void drawAppIcon(Canvas& c, const Fonts& fonts);

// PS5 home-screen artwork, any 16:9 size (3840x2160 for sce_sys):
// `launch` false: pic0, the selected-app background (mark on the right, the
// left kept dark for the Shell's title and Play button);
// `launch` true: pic1, the launch transition (mark and name centred).
void drawHomeArt(Canvas& c, const Fonts& fonts, bool launch);

}  // namespace xc::ui
