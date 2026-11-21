// SPDX-License-Identifier: GPL-3.0-or-later
#include <set>
#include "ui/app_ui_internal.h"
#include "ui/nimbox_horizon_layout.h"

namespace xc::ui {
namespace {
constexpr Str profileActions[] = {Str::SocialFriends, Str::LocalNotices, Str::Achievements,
                                  Str::RedeemCode, Str::ManagePlan, Str::SignOut};
const char* presenceLabel(const std::string& status) {
    if (status == "Online") return tr(Str::OnlineNow);
    if (status == "Away") return tr(Str::AwayNow);
    if (status == "Offline") return tr(Str::OfflineNow);
    return tr(Str::NoStatus);
}
}

void AppUi::rememberNotice(const std::string& message) {
    if (message.empty()) return;
    if (notices_.empty() || notices_.front() != message) notices_.insert(notices_.begin(), message);
    if (notices_.size() > 32) notices_.resize(32);
    unreadNotices_ = true;
    if (!directory_.empty() && directory_.back().kind == Directory::Notices) {
        directory_.back().labels = notices_;
        unreadNotices_ = false;
    }
}

void AppUi::setSocial(std::vector<xcloud::FriendPresence> people, std::string presence, bool loaded) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (loaded) {
        std::stable_sort(people.begin(), people.end(), [](const auto& a, const auto& b) {
            if ((a.presenceState == "Online") != (b.presenceState == "Online")) return a.presenceState == "Online";
            return a.gamertag < b.gamertag;
        });
        friends_ = std::move(people);
        friendsLoaded_ = true;
    }
    ownPresence_ = std::move(presence);
    if (!directory_.empty() && directory_.back().kind == Directory::Friends) fillDirectory(directory_.back());
    dirty_ = true;
}

void AppUi::openDirectory(Directory kind, const std::string& title, const std::string& url) {
    DirectoryPage page;
    page.kind = kind;
    page.title = title;
    page.url = url;
    fillDirectory(page);
    if (kind == Directory::Notices) unreadNotices_ = false;
    directory_.push_back(std::move(page));
    dirty_ = true;
}

void AppUi::fillDirectory(DirectoryPage& page) {
    const std::string selected = page.focus < static_cast<int>(page.games.size()) ? page.games[page.focus].productId :
                                 page.focus < static_cast<int>(page.labels.size()) ? page.labels[page.focus] : "";
    page.labels.clear();
    page.games.clear();
    const auto kind = page.kind;
    if (kind == Directory::Explore) {
        page.labels = {tr(Str::BrowseAll), tr(Str::BrowseGenres), tr(Str::BrowseFeatured),
                       tr(Str::BrowsePass), tr(Str::FilterMouseKeyboard), tr(Str::BrowseOwn), "Ubisoft+ Classics"};
        for (const auto& row : rows_)
            if (!row.isGrid && row.title != tr(Str::HiddenSection)) page.labels.push_back(row.title);
    } else if (kind == Directory::Friends) {
        for (const auto& f : friends_) page.labels.push_back(f.gamertag);
    } else if (kind == Directory::Notices) {
        page.labels = notices_;
    } else if (kind == Directory::SignOut) {
        page.labels = {tr(Str::Cancel), tr(Str::SignOut)};
    } else if (kind != Directory::Link) {
        std::unordered_set<std::string> seen;
        std::set<std::string> genres;
        auto append = [&](const std::vector<GameTile>& pool) {
            for (const auto& tile : pool) {
                if (tile.name.empty() || tile.productId.empty() || tile.titleId.empty() ||
                    hidden_.count(tile.productId) || storeUnavailableLocked(tile)) continue;
                if (kind == Directory::Genres) {
                    for (const auto& category : tile.categories) if (!category.empty()) genres.insert(category);
                    continue;
                }
                if (kind == Directory::GenreGames && std::find(tile.categories.begin(), tile.categories.end(),
                    page.title) == tile.categories.end()) continue;
                if (kind == Directory::Keyboard && !tile.mouseKeyboard) continue;
                if (kind == Directory::Awards && tile.xboxTitleId.empty()) continue;
                if (kind == Directory::Featured && tile.heroUrl.empty()) continue;
                if (!seen.insert(tile.productId).second) continue;
                page.games.push_back(tile);
                if (kind == Directory::Featured && page.games.size() >= 8) break;
            }
        };
        if (kind == Directory::Shelf) {
            for (const auto& row : rows_) if (row.title == page.title) { append(row.tiles); break; }
        } else if (kind == Directory::Own) {
            append(owned_);
            append(purchasable_);
        } else if (kind == Directory::Featured) {
            for (const auto& row : rows_) if (row.catalogId == xcloud::sigl::kMostPopular) { append(row.tiles); break; }
            if (page.games.size() < 8) append(gamePassPool_);
        } else {
            for (const auto& row : rows_) if (row.catalogId == xcloud::sigl::kAllGames) append(row.tiles);
            append(gamePassPool_);
            if (kind != Directory::Pass) append(libraryPool_);
        }
        page.labels.assign(genres.begin(), genres.end());
    }
    const int count = static_cast<int>(std::max(page.labels.size(), page.games.size()));
    if (!selected.empty()) {
        for (size_t i = 0; i < page.games.size(); ++i)
            if (page.games[i].productId == selected) { page.focus = static_cast<int>(i); break; }
        for (size_t i = 0; i < page.labels.size(); ++i)
            if (page.labels[i] == selected) { page.focus = static_cast<int>(i); break; }
    }
    page.focus = std::clamp(page.focus, 0, std::max(0, count - 1));
}

void AppUi::backDirectory() {
    directory_.pop_back();
    if (!directory_.empty()) fillDirectory(directory_.back());
    if (directory_.empty() && directoryFromProfile_) {
        profileOpen_ = true;
        directoryFromProfile_ = false;
    }
    dirty_ = true;
}

void AppUi::activateProfile(UiEvent& ev) {
    const Directory kinds[] = {Directory::Friends, Directory::Notices, Directory::Awards,
                               Directory::Link, Directory::Link, Directory::SignOut};
    const Str label = profileActions[profileFocus_];
    const std::string url = profileFocus_ == 3 ? "https://redeem.microsoft.com/" :
                            profileFocus_ == 4 ? "https://account.microsoft.com/services" : "";
    profileOpen_ = false;
    directoryFromProfile_ = true;
    openDirectory(kinds[profileFocus_], tr(profileFocus_ == 5 ? Str::ConfirmLogOut : label), url);
    (void)ev;
}

void AppUi::activateDirectory(UiEvent& ev) {
    const auto& p = directory_.back();
    if (p.kind == Directory::Link) {
        if (caps_.openUrl) { ev.action = Action::OpenUrl; ev.url = p.url; }
        return;
    }
    if (!p.games.empty()) {
        const bool awards = p.kind == Directory::Awards;
        openDetails(p.games[p.focus]);
        if (awards) openAchievements();
        return;
    }
    if (p.labels.empty()) return;
    const std::string label = p.labels[p.focus];
    if (p.kind == Directory::Genres) openDirectory(Directory::GenreGames, label);
    else if (p.kind == Directory::Explore) {
        const Directory kinds[] = {Directory::All, Directory::Genres, Directory::Featured, Directory::Pass,
                                   Directory::Keyboard, Directory::Own};
        if (p.focus < 6) openDirectory(kinds[p.focus], label);
        else if (p.focus == 6) openDirectory(Directory::Link, label,
            "https://store.ubisoft.com/ubisoftplus/games?access=xbox&offer=classics");
        else openDirectory(Directory::Shelf, label);
    } else if (p.kind == Directory::SignOut) {
        if (p.focus == 0) backDirectory();
        else { directory_.clear(); directoryFromProfile_ = false; ev.action = Action::SignOut; }
    }
}

int AppUi::directoryColumns(const DirectoryPage& page) const {
    if (page.kind == Directory::Featured) return 2;
    if (!page.games.empty()) return 6;
    if (page.kind == Directory::Explore || page.kind == Directory::Genres) return 3;
    return 1;
}

Rect AppUi::directoryCell(const DirectoryPage& page, int index) const {
    const int cols = directoryColumns(page);
    const int visibleRows = cols == 6 || page.kind == Directory::Featured ? 2 : cols == 3 ? 4 : 7;
    const int count = static_cast<int>(std::max(page.games.size(), page.labels.size()));
    const int first = std::clamp(page.focus / cols - visibleRows + 1, 0, std::max(0, (count + cols - 1) / cols - visibleRows));
    const int row = index / cols - first;
    if (row < 0 || row >= visibleRows) return {};
    if (cols == 6) return {180 + index % 6 * kCardPitch, 265 + row * 324, kCard, kCard};
    if (page.kind == Directory::Featured) return {102 + (index % 2) * 872, 242 + row * 350, 844, 308};
    const int pitch = cols == 3 ? 566 : 1570;
    return {120 + (index % cols) * pitch, 245 + row * (cols == 3 ? 166 : 98), pitch - 24, cols == 3 ? 140 : 82};
}

void AppUi::handleDirectory(const NavInput& in, UiEvent& ev) {
    auto& p = directory_.back();
    if (in.back) { backDirectory(); return; }
    if (in.triangle && !p.games.empty()) {
        searching_ = searchOnKeys_ = true;
        query_.clear();
        runSearch();
    } else if (in.accept) activateDirectory(ev);
    else {
        const int count = static_cast<int>(std::max(p.labels.size(), p.games.size()));
        if (count) {
            const int cols = directoryColumns(p);
            if (in.scrollRows) p.focus = std::clamp(p.focus + cols * in.scrollRows, 0, count - 1);
            else moveInGrid(p.focus, count, cols, in);
        }
    }
    dirty_ = true;
}

bool AppUi::directoryPointer(float x, float y, int button, UiEvent& ev) {
    auto& p = directory_.back();
    if (button == 3 || (button == 1 && horizon::contains(horizon::kBrowseBack, x, y))) {
        backDirectory();
        return true;
    }
    if (button != 1) return true;
    if (!p.games.empty() && horizon::contains(horizon::kBrowseSearch, x, y)) {
        NavInput search;
        search.triangle = true;
        handleDirectory(search, ev);
        return true;
    }
    if (p.kind == Directory::Link && horizon::contains({740, 685, 440, 74}, x, y)) {
        activateDirectory(ev);
        return true;
    }
    const int count = static_cast<int>(std::max(p.labels.size(), p.games.size()));
    for (int i = 0; i < count; ++i) {
        if (!horizon::contains(directoryCell(p, i), x, y)) continue;
        p.focus = i;
        activateDirectory(ev);
        break;
    }
    return true;
}

void AppUi::drawProfileActions(Canvas& c, int x, int right) {
    const int y = horizon::kProfilePanel.y + 230;
    const std::string score = gamerscore_.empty() ? "—" : gamerscore_;
    fonts_.semibold.draw(c, presenceLabel(ownPresence_), x, y, 23, kWhite);
    const auto stat = std::string(tr(Str::GamerscoreLabel)) + "   " + score;
    fonts_.regular.draw(c, stat, right - fonts_.regular.measure(stat, 22), y, 22, kGray);
    drawCloudTime(c, x, y + 36);
    const std::string counts = std::string(tr(Str::YourGames)) + "  " + (ownedKnown_ ? std::to_string(owned_.size()) : "—") +
        "     " + tr(Str::Favorites) + "  " + std::to_string(favorites_.size()) +
        "     " + tr(Str::TabConsoles) + "  " + (consolesKnown_ ? std::to_string(consoles_.size()) : "—");
    fonts_.regular.draw(c, fonts_.regular.fit(counts, 20, right-x), x, y+77, 20, kGray);
    for (int i = 0; i < 6; ++i) {
        const Rect r = horizon::profileAction(i);
        if (i == profileFocus_) c.fillRect(r, withAlpha(kWhite, 40), 9);
        fonts_.semibold.draw(c, tr(profileActions[i]), r.x+18, r.y+13, 25, kWhite);
    }
}

void AppUi::drawSocialButtons(Canvas& c) {
    const Rect f = horizon::kFriends, b = horizon::kBell;
    c.strokeCircle(f.x + 19, f.y + 19, 6, 2, kWhite);
    c.strokeArc(f.x + 19, f.y + 37, 11, 2, 3.15f, 3.13f, kWhite);
    c.strokeCircle(f.x + 33, f.y + 20, 4, 2, kWhite);
    c.strokeArc(b.x + 24, b.y + 24, 10, 2, 3.14f, 3.14f, kWhite);
    c.line(b.x + 14, b.y + 24, b.x + 11, b.y + 33, 2, kWhite);
    c.line(b.x + 34, b.y + 24, b.x + 37, b.y + 33, 2, kWhite);
    c.line(b.x + 11, b.y + 33, b.x + 37, b.y + 33, 2, kWhite);
    c.fillCircle(b.x + 24, b.y + 37, 2.5f, kWhite);
    if (unreadNotices_) c.fillCircle(b.x + 39, b.y + 13, 4, preview31::kAccent);
}

void AppUi::drawDirectory(Canvas& c) {
    const auto& p = directory_.back();
    drawCollectionBackdrop(c, lastRender_, focusedTile());
    c.fillRect({0,0,kW,kH}, rgba(6, 14, 24, 158));
    const Rect back = horizon::kBrowseBack;
    c.line(back.x + 36, back.y + 20, back.x + 24, back.y + 32, 3, kWhite);
    c.line(back.x + 24, back.y + 32, back.x + 36, back.y + 44, 3, kWhite);
    fonts_.bold.draw(c, fonts_.bold.fit(p.title, 52, 1190), 184, 91, 52, kWhite);
    const int count = static_cast<int>(std::max(p.games.size(), p.labels.size()));
    if (!p.games.empty()) {
        fonts_.regular.draw(c, gamesCount(count), 186, 168, 23, kGray);
        fonts_.semibold.draw(c, tr(Str::TabSearch), horizon::kBrowseSearch.x + 22, 115, 24, kWhite);
    }
    if (p.kind == Directory::Link) {
        drawQr(c, p.url, 1130, 278, 340);
        if (caps_.openUrl) {
            c.fillRect({740, 685, 440, 74}, withAlpha(kWhite, 30), 12);
            drawCentered(c, fonts_.semibold, tr(Str::OpenInBrowser), 702, 27, kWhite);
        }
    } else if (!count) {
        const Str empty = p.kind == Directory::Friends ? (friendsLoaded_ ? Str::NoFriends : Str::SocialUnavailable) :
                          p.kind == Directory::Notices ? Str::NoNotices : Str::NoGames;
        drawCentered(c, fonts_.regular, tr(empty), 468, 30, kGray);
    }
    const int cols = directoryColumns(p);
    const int begin = std::max(0, p.focus / cols - 7) * cols;
    for (int i = begin; i < std::min(count, begin + 16 * cols); ++i) {
        const Rect r = directoryCell(p, i);
        if (!r.w) continue;
        const bool selected = i == p.focus;
        if (!p.games.empty()) {
            const auto& tile = p.games[i];
            if (p.kind == Directory::Featured) {
                c.fillRect(r, kPanel, 18);
                if (auto art = images_.get(tile.heroUrl, r.w, r.h)) c.drawImage(*art, r.x, r.y, 255, 18);
                c.gradientV({r.x,r.y+r.h/2,r.w,r.h/2}, rgba(6,10,19,0), rgba(6,10,19,225));
                fonts_.bold.draw(c, fonts_.bold.fit(tile.name, 32, r.w-56), r.x+28, r.y+r.h-66, 32, kWhite);
                if (selected) c.strokeRect(r, kWhite, 3, 18);
            } else {
                drawCard(c, tile, r.x, r.y, selected, false);
                fonts_.semibold.draw(c, fonts_.semibold.fit(tile.name, 21, r.w), r.x, r.y+r.h+12, 21, kWhite);
                if (p.kind == Directory::Own) fonts_.regular.draw(c,
                    tr(tile.purchasable && !tile.playable ? Str::BuyToPlay : Str::Play), r.x, r.y+r.h+40, 18, kGray);
            }
        } else {
            c.fillRect(r, selected ? rgba(61, 83, 103, 220) : rgba(22, 36, 51, 193), 12);
            if (selected) c.strokeRect(r, kWhite, 2, 12);
            int x = r.x+28;
            if (p.kind == Directory::Friends && i < int(friends_.size())) {
                const auto& who = friends_[i];
                if (auto avatar = images_.get(who.gamerpicUrl, 52, 52)) c.drawImage(*avatar,x,r.y+15,255,-1);
                x += 72;
                const auto activity = who.presenceText.empty() ? presenceLabel(who.presenceState) : who.presenceText;
                fonts_.regular.draw(c, fonts_.regular.fit(activity, 22, r.w/2), r.x+r.w/2, r.y+25, 22, kGray);
            }
            fonts_.semibold.draw(c, fonts_.semibold.fit(p.labels[i], 28, p.kind == Directory::Friends ? r.w/2-122 : r.w-56),
                                 x, fonts_.semibold.centeredY(r.y,r.h,28), 28, kWhite);
        }
    }
    hintRects_.clear();
    drawHints(c, {{kIconCircle, tr(Str::Back)}});
}

} // namespace xc::ui
