// SPDX-License-Identifier: GPL-3.0-or-later
// Native Nimbox navigation regression tests, included from test_main.cpp.
static void testNimboxNativeMenuPages() {
    ui::Fonts fonts;
    ui::ImageCache images([] {});
    ui::AppUi app(fonts, images);
    app.setSubscription("GamePass");

    ui::GameTile active;
    active.name = "Action Title";
    active.productId = "MENU_ACTION";
    active.titleId = "MENU_ACTION_TITLE";
    active.categories = {"Action", "Adventure"};
    active.friends.push_back({"Online Friend", ""});
    ui::GameTile puzzle;
    puzzle.name = "Puzzle Title";
    puzzle.productId = "MENU_PUZZLE";
    puzzle.titleId = "MENU_PUZZLE_TITLE";
    puzzle.categories = {"Puzzle"};
    ui::GameRow row;
    row.title = "Sample catalog";
    row.tiles = {active, puzzle};
    app.setRows({row});
    app.showHome();

    ui::NavInput next;
    next.r1 = true;
    app.handle(next);
    CHECK(app.tab() == ui::Tab::Library);
    app.handle(next);
    CHECK(app.tab() == ui::Tab::Consoles);
    app.handle(next);
    CHECK(app.tab() == ui::Tab::Genres);

    // Select the first real category and open a game through the native page.
    ui::NavInput accept;
    accept.accept = true;
    app.handle(accept); // genre -> game's grid
    app.handle(accept); // grid -> details
    CHECK(app.screen() == ui::Screen::Details);

    ui::NavInput back;
    back.back = true;
    app.handle(back);
    CHECK(app.screen() == ui::Screen::Home);

    app.handle(next);
    CHECK(app.tab() == ui::Tab::Friends);
    app.handle(accept);
    CHECK(app.screen() == ui::Screen::Details);
    CHECK(app.detailWanted(100) == "MENU_ACTION");

    app.handle(back);
    CHECK(app.screen() == ui::Screen::Home);

    // Square opens the real profile screen from the friends page.
    ui::NavInput profile;
    profile.square = true;
    app.handle(profile);
    CHECK(app.screen() == ui::Screen::Profile);
    app.handle(back);
    CHECK(app.screen() == ui::Screen::Home);

    app.handle(next);
    CHECK(app.tab() == ui::Tab::GamePass); // wrap-around preserves old Home

    // In the native header, avatar and Settings gear have different targets.
    app.onMouseDown(1760.0f, 72.0f, 1);
    CHECK(app.screen() == ui::Screen::Profile);
    app.handle(back);
    CHECK(app.screen() == ui::Screen::Home);
    app.onMouseDown(1850.0f, 72.0f, 1);
    CHECK(app.screen() == ui::Screen::Settings);
}
