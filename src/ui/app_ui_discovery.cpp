// SPDX-License-Identifier: GPL-3.0-or-later
#include <set>
#include <string_view>
#include "ui/app_ui_internal.h"
#include "ui/nimbox_horizon_layout.h"

namespace xc::ui {
namespace {
// Original line marks on a 48-unit canvas. Match localized Store category names;
// new categories keep their own initial and their original filtering label.
struct MarkPoint { int x, y; };
struct GenreMark { const char* stems; const MarkPoint* points; size_t size; Color tint; };

const GenreMark* genreMark(const std::string& title) {
    static constexpr MarkPoint compass[] = {{-22,0}, {-17,-14}, {-7,-22}, {7,-22}, {17,-14}, {22,0}, {17,14}, {7,22}, {-7,22}, {-17,14}, {-22,0}, {99,99}, {-10,11}, {-4,-4}, {11,-12}, {5,4}, {-10,11}};
    static constexpr MarkPoint cards[] = {{-22,10}, {-26,-15}, {-8,-19}, {99,99}, {-14,-16}, {15,-16}, {15,19}, {-14,19}, {-14,-16}, {99,99}, {0,-7}, {7,1}, {0,9}, {-7,1}, {0,-7}};
    static constexpr MarkPoint dice[] = {{-20,-17}, {19,-17}, {19,20}, {-20,20}, {-20,-17}, {99,99}, {-11,-8}, {-9,-8}, {99,99}, {0,1}, {2,1}, {99,99}, {9,11}, {11,11}};
    static constexpr MarkPoint arcade[] = {{-19,0}, {-19,-7}, {-12,-7}, {-12,-14}, {11,-14}, {11,-7}, {18,-7}, {18,0}, {24,0}, {24,14}, {17,14}, {17,7}, {-17,7}, {-17,14}, {-24,14}, {-24,0}, {-19,0}, {99,99}, {-11,-3}, {-8,-3}, {99,99}, {8,-3}, {11,-3}, {99,99}, {-12,-14}, {-17,-21}, {99,99}, {12,-14}, {17,-21}};
    static constexpr MarkPoint flag[] = {{-18,22}, {-18,-20}, {0,-23}, {18,-16}, {18,3}, {0,-4}, {-18,0}, {99,99}, {-9,-21}, {-9,-2}, {99,99}, {1,-21}, {1,-4}, {99,99}, {10,-19}, {10,-1}, {99,99}, {-17,-10}, {0,-14}, {17,-7}};
    static constexpr MarkPoint puzzle[] = {{-19,-15}, {-6,-15}, {-6,-20}, {-2,-24}, {3,-24}, {7,-20}, {7,-15}, {19,-15}, {19,-3}, {13,-3}, {9,1}, {9,5}, {13,9}, {19,9}, {19,20}, {-19,20}, {-19,-15}};
    static constexpr MarkPoint book[] = {{0,-12}, {-8,-17}, {-23,-19}, {-23,14}, {-9,16}, {0,21}, {9,16}, {23,14}, {23,-19}, {9,-17}, {0,-12}, {0,19}};
    static constexpr MarkPoint ball[] = {{0,-23}, {14,-18}, {23,-5}, {22,9}, {13,19}, {0,23}, {-14,18}, {-22,7}, {-22,-7}, {-14,-18}, {0,-23}, {99,99}, {0,-9}, {9,-3}, {6,8}, {-6,8}, {-9,-3}, {0,-9}, {0,-22}, {99,99}, {9,-3}, {21,-7}, {99,99}, {6,8}, {13,19}, {99,99}, {-6,8}, {-14,18}, {99,99}, {-9,-3}, {-21,-7}};
    static constexpr MarkPoint rook[] = {{-17,-21}, {-17,-8}, {-11,-3}, {-13,12}, {-18,16}, {-18,21}, {18,21}, {18,16}, {13,12}, {11,-3}, {17,-8}, {17,-21}, {9,-21}, {9,-14}, {4,-14}, {4,-21}, {-4,-21}, {-4,-14}, {-9,-14}, {-9,-21}, {-17,-21}, {99,99}, {-11,-3}, {11,-3}, {99,99}, {-13,12}, {13,12}};
    static constexpr MarkPoint family[] = {{-11,-19}, {-5,-16}, {-3,-10}, {-6,-5}, {-12,-3}, {-17,-6}, {-19,-12}, {-16,-17}, {-11,-19}, {99,99}, {11,-12}, {16,-10}, {18,-5}, {16,0}, {11,2}, {6,0}, {4,-5}, {6,-10}, {11,-12}, {99,99}, {-24,18}, {-22,7}, {-15,2}, {-7,2}, {-2,6}, {99,99}, {-1,19}, {0,12}, {5,7}, {16,7}, {22,12}, {23,19}};
    static constexpr MarkPoint tools[] = {{-20,14}, {3,-9}, {4,-18}, {10,-23}, {14,-23}, {10,-14}, {15,-9}, {24,-13}, {23,-6}, {17,0}, {9,1}, {-14,23}, {-20,18}, {-20,14}};
    static constexpr MarkPoint target[] = {{-15,-13}, {-8,-18}, {4,-18}, {14,-12}, {18,-4}, {18,5}, {12,14}, {3,18}, {-6,18}, {-15,12}, {-18,4}, {-18,-5}, {-15,-13}, {99,99}, {0,-24}, {0,-12}, {99,99}, {0,12}, {0,24}, {99,99}, {-24,0}, {-12,0}, {99,99}, {12,0}, {24,0}, {99,99}, {-2,0}, {2,0}};
    static constexpr MarkPoint bulb[] = {{-8,10}, {-9,5}, {-16,-3}, {-16,-10}, {-12,-17}, {-5,-21}, {5,-21}, {12,-17}, {16,-10}, {16,-3}, {9,5}, {8,10}, {-8,10}, {99,99}, {-7,15}, {7,15}, {99,99}, {-4,20}, {4,20}, {99,99}, {0,9}, {0,-2}, {-5,-7}, {99,99}, {0,-2}, {5,-7}};
    static constexpr MarkPoint glove[] = {{-14,6}, {-21,-3}, {-20,-12}, {-15,-14}, {-11,-9}, {-11,-19}, {-6,-22}, {-2,-19}, {2,-22}, {6,-19}, {10,-20}, {14,-16}, {18,-16}, {21,-11}, {19,1}, {11,11}, {9,19}, {-9,19}, {-14,6}, {99,99}, {-11,9}, {12,9}};
    static constexpr MarkPoint crown[] = {{-23,-12}, {-12,-2}, {0,-19}, {12,-2}, {23,-12}, {16,12}, {-16,12}, {-23,-12}, {99,99}, {-15,18}, {15,18}};
    static constexpr MarkPoint notes[] = {{-10,12}, {-10,-14}, {17,-20}, {17,6}, {99,99}, {-9,-8}, {17,-14}, {99,99}, {-10,8}, {-17,6}, {-23,10}, {-23,16}, {-18,19}, {-11,16}, {-10,12}, {99,99}, {17,3}, {10,1}, {4,5}, {4,11}, {9,14}, {15,11}, {17,6}};
    static constexpr MarkPoint sliders[] = {{-22,-12}, {-11,-12}, {99,99}, {-2,-12}, {22,-12}, {99,99}, {-10,-17}, {-2,-17}, {-2,-7}, {-10,-7}, {-10,-17}, {99,99}, {-22,9}, {5,9}, {99,99}, {14,9}, {22,9}, {99,99}, {5,4}, {14,4}, {14,14}, {5,14}, {5,4}};
    static constexpr MarkPoint steps[] = {{-24,18}, {-12,18}, {-12,9}, {2,9}, {2,0}, {16,0}, {16,-9}, {24,-9}, {99,99}, {-16,-5}, {-11,-15}, {-2,-18}, {6,-16}, {99,99}, {0,-22}, {7,-16}, {1,-11}};
    static constexpr MarkPoint network[] = {{-16,-14}, {0,16}, {16,-14}, {-16,-14}, {99,99}, {-22,-18}, {-13,-18}, {-13,-10}, {-22,-10}, {-22,-18}, {99,99}, {12,-18}, {21,-18}, {21,-10}, {12,-10}, {12,-18}, {99,99}, {-4,12}, {4,12}, {4,21}, {-4,21}, {-4,12}};
    static constexpr GenreMark marks[] = {
        {"acao|action|accion|azione|abenteuer|aventure|aventura|adventure", compass, sizeof(compass)/sizeof(MarkPoint), rgba(56, 104, 130)},
        {"cart|card|karten|board|tabuleiro|tavolo|brett|mesa|societe", cards, sizeof(cards)/sizeof(MarkPoint), rgba(100, 78, 143)},
        {"cassino|casino|glucksspiel", dice, sizeof(dice)/sizeof(MarkPoint), rgba(66, 121, 105)},
        {"classic|class|clasico|klassik|retro", arcade, sizeof(arcade)/sizeof(MarkPoint), rgba(119, 84, 101)},
        {"corrida|corsa|course|carreras|racing|renn|voo|flying", flag, sizeof(flag)/sizeof(MarkPoint), rgba(68, 105, 153)},
        {"desafio|puzzle|rompecabezas|casse|ratsel|enigma|quebra", puzzle, sizeof(puzzle)/sizeof(MarkPoint), rgba(94, 93, 141)},
        {"educ|lerne|lernen", book, sizeof(book)/sizeof(MarkPoint), rgba(56, 104, 130)},
        {"esport|sport|deport", ball, sizeof(ball)/sizeof(MarkPoint), rgba(100, 78, 143)},
        {"estrat|strat", rook, sizeof(rook)/sizeof(MarkPoint), rgba(66, 121, 105)},
        {"famil|kinder|kids|bambini|enfant|ninos", family, sizeof(family)/sizeof(MarkPoint), rgba(119, 84, 101)},
        {"ferrament|outil|tools|strument|herramient|werkzeug", tools, sizeof(tools)/sizeof(MarkPoint), rgba(68, 105, 153)},
        {"tiro|shooter|tir |de tir|sparatutto|dispar|schiess", target, sizeof(target)/sizeof(MarkPoint), rgba(94, 93, 141)},
        {"indie|independ|unabhang", bulb, sizeof(bulb)/sizeof(MarkPoint), rgba(56, 104, 130)},
        {"luta|lucha|briga|brawler|fight|combat|kampf|picchiaduro", glove, sizeof(glove)/sizeof(MarkPoint), rgba(100, 78, 143)},
        {"rpg|role|rolle|rol |de rol|ruolo", crown, sizeof(crown)/sizeof(MarkPoint), rgba(66, 121, 105)},
        {"music|musik|ritm|rhythm|rythm", notes, sizeof(notes)/sizeof(MarkPoint), rgba(119, 84, 101)},
        {"simula", sliders, sizeof(sliders)/sizeof(MarkPoint), rgba(68, 105, 153)},
        {"platform|plataform|plateform|jump", steps, sizeof(steps)/sizeof(MarkPoint), rgba(94, 93, 141)},
        {"moba|multipl|arena", network, sizeof(network)/sizeof(MarkPoint), rgba(56, 104, 130)}
    };
    const std::string key = fold(title);
    for (const auto& mark : marks) {
        std::string_view stems(mark.stems);
        while (!stems.empty()) {
            const size_t split = stems.find('|');
            if (key.find(stems.substr(0, split)) != std::string::npos) return &mark;
            if (split == std::string_view::npos) break;
            stems.remove_prefix(split + 1);
        }
    }
    return nullptr;
}

void drawGenreCard(Canvas& c, const Font& font, Rect r, const std::string& title, bool selected) {
    const auto* mark = genreMark(title);
    const Color tint = mark ? mark->tint : rgba(75, 99, 130);
    c.fillRect(r, rgba(18,31,43), 14);
    const Rect surface{r.x+6, r.y+6, r.w-12, r.h-12};
    c.gradientV(surface, rgba(20,33,46), tint);
    c.strokeRect(r, selected ? kWhite : withAlpha(tint, 190), selected ? 4 : 2, 14);
    const int cx = r.x+r.w/2, cy = r.y+r.h/2-14;
    if (mark) {
        for (size_t i = 1; i < mark->size; ++i) {
            const auto a = mark->points[i-1], b = mark->points[i];
            if (a.x == 99 || b.x == 99) continue;
            c.line(cx+a.x*1.55f, cy+a.y*1.55f, cx+b.x*1.55f, cy+b.y*1.55f, 4, kWhite);
        }
    } else {
        size_t end = 1;
        while (end < title.size() && (static_cast<unsigned char>(title[end]) & 0xC0) == 0x80) ++end;
        const auto initial = title.substr(0,end);
        c.strokeCircle(cx,cy,37,3,kWhite);
        font.draw(c,initial,cx-font.measure(initial,38)/2,cy-24,38,kWhite);
    }
    const auto lines = font.wrap(title, 25, r.w-44, 2);
    int y = r.y+r.h-32-static_cast<int>(lines.size())*30;
    for (const auto& line : lines) { font.draw(c,line,r.x+22,y,25,kWhite); y+=30; }
}

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
            if (!row.isGrid && row.catalogId != horizon::kGenreShelf && row.catalogId != xcloud::sigl::kUbisoftClassics && row.title != tr(Str::HiddenSection)) page.labels.push_back(row.title);
    } else if (kind == Directory::Friends) {
        for (const auto& f : friends_) page.labels.push_back(f.gamertag);
    } else if (kind == Directory::Notices) {
        page.labels = notices_;
    } else if (kind == Directory::SignOut) {
        page.labels = {tr(Str::Cancel), tr(Str::SignOut)};
    } else if (kind != Directory::Link && kind != Directory::Chat) {
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
        } else if (kind == Directory::Ubisoft) {
            for (const auto& row : rows_) if (row.catalogId == xcloud::sigl::kUbisoftClassics) { append(row.tiles); break; }
        } else if (kind == Directory::Own) {
            append(owned_);
            append(purchasable_);
        } else if (kind == Directory::Featured) {
            for (const auto& row : rows_) if (row.catalogId == xcloud::sigl::kMostPopular) { append(row.tiles); break; }
            if (page.games.size() < 8) append(gamePassPool_);
        } else if (kind == Directory::All || kind == Directory::Genres || kind == Directory::GenreGames) {
            // These three views browse exactly the catalog advertised by this shelf.
            const auto all = std::find_if(rows_.begin(), rows_.end(), [](const GameRow& r) {
                return r.catalogId == xcloud::sigl::kAllGames;
            });
            append(all == rows_.end() ? gamePassPool_ : all->tiles);
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

bool AppUi::onCatalogShelf() const {
    return tab_ == Tab::GamePass && focusRow_ >= 0 && focusRow_ < static_cast<int>(rows_.size()) &&
           (rows_[focusRow_].catalogId == xcloud::sigl::kAllGames || rows_[focusRow_].catalogId == horizon::kGenreShelf);
}

AppUi::DirectoryPage& AppUi::activeDirectory() {
    if (!directory_.empty()) return directory_.back();
    if (shelfCatalogDirty_) {
        shelfCatalog_.kind = Directory::All;
        shelfCatalog_.title = tr(Str::BrowseAll);
        fillDirectory(shelfCatalog_);
        shelfGenres_.kind = Directory::Genres;
        shelfGenres_.title = tr(Str::BrowseGenres);
        fillDirectory(shelfGenres_);
        shelfCatalogDirty_ = false;
    }
    return rows_[focusRow_].catalogId == horizon::kGenreShelf ? shelfGenres_ : shelfCatalog_;
}

void AppUi::backDirectory() {
    if (directory_.empty()) {
        if (focusRow_ > 0) --focusRow_;
        else if (rows_.size() > 1) ++focusRow_;
        else if (caps_.desktopWindow) { quitPrompt_ = true; promptFocus_ = 1; }
        retarget();
        dirty_ = true;
        return;
    }
    if (directory_.back().kind == Directory::Chat) { ++chatSession_; chat_ = {}; }
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
    const auto& p = activeDirectory();
    if ((p.kind == Directory::Link || (p.kind == Directory::Ubisoft && p.games.empty()))) {
        if (caps_.openUrl) { ev.action = Action::OpenUrl; ev.url = p.url; }
        return;
    }
    if (!p.games.empty()) {
        const bool awards = p.kind == Directory::Awards;
        openDetails(p.games[p.focus]);
        if (awards) openAchievements();
        return;
    }
    if (p.kind == Directory::Genres && (p.allButtonFocused || p.labels.empty())) {
        if (directory_.empty()) {
            shelfGenres_.allButtonFocused = false;
            ++focusRow_;
            retarget();
            dirty_ = true;
            return;
        }
        const bool returnToAll = directory_.size() == 1 ? onCatalogShelf() :
                                 directory_[directory_.size()-2].kind == Directory::All;
        if (returnToAll) backDirectory();
        else openDirectory(Directory::All, tr(Str::BrowseAll));
        return;
    }
    if (p.labels.empty()) return;
    const std::string label = p.labels[p.focus];
    if (p.kind == Directory::Friends && p.focus < int(friends_.size())) {
        if (friends_[p.focus].xuid.empty()) return;
        ++chatSession_;
        chat_ = {};
        chat_.peer = friends_[p.focus].xuid;
        openDirectory(Directory::Chat, label);
    } else if (p.kind == Directory::Genres) openDirectory(Directory::GenreGames, label);
    else if (p.kind == Directory::Explore) {
        const Directory kinds[] = {Directory::All, Directory::Genres, Directory::Featured, Directory::Pass,
                                   Directory::Keyboard, Directory::Own};
        if (p.focus < 6) openDirectory(kinds[p.focus], label);
        else if (p.focus == 6) openDirectory(Directory::Ubisoft, label,
            std::string("https://www.xbox.com/") + gameLocale() + "/xbox-game-pass/games#ubisoftclassics");
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
    if (page.kind == Directory::Genres) {
        constexpr int visible = 6, pitch = 284;
        const int first = std::clamp(page.focus - visible/2, 0, std::max(0, int(page.labels.size()) - visible));
        const int slot = index - first;
        if (slot < 0 || slot >= visible) return {};
        return {120 + slot*pitch, 296, 260, 392};
    }
    const int cols = directoryColumns(page);
    const int visibleRows = cols == 6 || page.kind == Directory::Featured ? 2 : cols == 3 ? 4 : 7;
    const int count = static_cast<int>(std::max(page.games.size(), page.labels.size()));
    const int first = std::clamp(page.focus / cols - visibleRows + 1, 0, std::max(0, (count + cols - 1) / cols - visibleRows));
    const int row = index / cols - first;
    if (row < 0 || row >= visibleRows) return {};
    if (cols == 6) return {180 + index % 6 * kCardPitch, 265 + row * 324, kCard, kCard};
    if (page.kind == Directory::Featured) return {102 + (index % 2) * 872, 242 + row * 350, 844, 308};
    if (page.kind == Directory::Friends) return {120, 245 + row*98, 820, 82};
    const int pitch = cols == 3 ? 566 : 1570;
    return {120 + (index % cols) * pitch, 245 + row * (cols == 3 ? 166 : 98), pitch - 24, cols == 3 ? 140 : 82};
}

namespace {
void drawFriendAvatar(Canvas& c, Rect r) {
    const int cx = r.x+r.w/2, cy = r.y+r.h/2;
    c.fillCircle(cx,cy,r.w/2,rgba(54,68,83));
    c.fillCircle(cx,cy-r.h/10,r.w/8,kWhite);
    c.strokeArc(cx,cy+r.h/4,r.w/5,float(r.w)/19,3.14f,3.14f,kWhite);
}
constexpr Rect kChatAction{1060, 610, 350, 76};
constexpr char kChatKeys[] = "abcdefghijklmnopqrstuvwxyz0123456789 .?!";
constexpr Rect kChatInput{120,836,1130,94}, kChatKeyboard{1268,836,220,94}, kChatSend{1506,836,294,94};
Rect chatKeyRect(int key) {
    if (key >= 40) return {520 + (key-40)*446,745,428,62};
    return {520 + key%10*89,475 + key/10*66,78,57};
}
}

bool AppUi::inChat() const {
    return screen_ == Screen::Home && !directory_.empty() && directory_.back().kind == Directory::Chat;
}

AppUi::ChatRequest AppUi::takeChatRequest(uint64_t nowMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!inChat() || chat_.busy || (!chat_.sendQueued && nowMs < chat_.nextRead)) return {};
    chat_.busy = true;
    chat_.sending = chat_.sendQueued;
    chat_.sendQueued = false;
    return {chatSession_, chat_.peer, chat_.sending ? chat_.draft : "",chatHistory_[chat_.peer].conversationId,chatAccount_};
}

void AppUi::finishChatRequest(const ChatRequest& req, const xcloud::ChatResponse& result, uint64_t nowMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (req.account != chatAccount_) return;
    const bool current = inChat() && req.session == chatSession_ && req.peer == chat_.peer;
    // A receipt can arrive after Back. Keep it with its recipient in this account.
    if (!current && req.text.empty()) return;
    auto& history = chatHistory_[req.peer];
    history.merge(result);
    if (!current) { dirty_ = true; return; }
    chat_.busy = chat_.sending = false;
    if (req.text.empty()) {
        chat_.readFailed = !result.ok;
        chat_.readStatus = result.status;
        chat_.loaded |= result.ok;
        chat_.historyOffset = std::clamp(chat_.historyOffset,0,std::max(0,int(history.messages.size())-1));
        chat_.nextRead = nowMs + (result.ok ? 3000 : 12000);
    } else {
        chat_.sendFailed = !result.ok;
        chat_.sendStatus = result.status;
        if (result.ok) { chat_.draft.clear(); chat_.historyOffset = 0; }
        chat_.nextRead = 0;
    }
    dirty_ = true;
}

void AppUi::editChat(std::string_view input) {
    if (chat_.sending || chat_.sendQueued) return;
    for (size_t i = 0; i < input.size();) {
        unsigned char c = input[i];
        if (c == '\b' && !chat_.draft.empty()) {
            while (!chat_.draft.empty() && (static_cast<unsigned char>(chat_.draft.back()) & 0xc0) == 0x80)
                chat_.draft.pop_back();
            if (!chat_.draft.empty()) chat_.draft.pop_back();
        } else if (c == '\n' || c == '\r') {
            pressChatKey(41);
            return;
        } else if (c >= 32) {
            size_t len = c < 128 ? 1 : c < 224 ? 2 : c < 240 ? 3 : 4;
            if (i+len > input.size() || chat_.draft.size()+len > 256) break;
            chat_.draft.append(input.data()+i, len);
            i += len;
            continue;
        }
        ++i;
    }
    dirty_ = true;
}

void AppUi::pressChatKey(int key) {
    if (key == 41) {
        if (!chat_.sending && !chat_.sendQueued && chat_.draft.find_first_not_of(' ') != std::string::npos) {
            chat_.sendQueued = true;
            chat_.historyOffset = 0;
            chat_.keyboardOpen = chat_.composing = false;
            chat_.sendFailed = false;
        }
    } else if (key == 40) editChat("\b");
    else if (key >= 0 && key < 40) editChat(std::string_view(kChatKeys + key, 1));
    dirty_ = true;
}

void AppUi::drawChat(Canvas& c, const std::string& name) {
    const auto& messages = chatHistory_[chat_.peer].messages;
    const bool pending = chat_.sendQueued || chat_.sending;
    const int top = 215;
    int bottom = chat_.keyboardOpen ? 449 : 799;
    c.setClip({120,top,1680,bottom-top});
    auto bubble = [&](const std::string& text,const std::string& label,bool mine) {
        const auto lines = fonts_.regular.wrap(text,26,870,20);
        int width = fonts_.semibold.measure(label,19);
        for (const auto& line : lines) width = std::max(width,fonts_.regular.measure(line,26));
        width = std::clamp(width+54,160,930);
        const int height = 62 + 36*int(lines.size());
        Rect r{mine ? 1800-width : 120,bottom-height,width,height};
        c.fillRect(r,mine ? rgba(34,69,87,239) : rgba(22,29,39,235),19);
        fonts_.semibold.draw(c,label,r.x+26,r.y+10,19,rgba(199,225,235));
        int y = r.y+41;
        for (const auto& line : lines) { fonts_.regular.draw(c,line,r.x+26,y,26,kWhite); y += 36; }
        bottom = r.y-14;
    };
    if (pending && !chat_.historyOffset) bubble(chat_.draft,tr(Str::ChatSending),true);
    for (int i=int(messages.size())-1-chat_.historyOffset;i>=0 && bottom>top;--i) {
        const auto& msg = messages[i];
        const bool mine = msg.sender != chat_.peer;
        bubble(msg.text.empty() ? tr(Str::ChatUnsupported) : msg.text,mine ? (msg.timestamp.empty() ? tr(Str::ChatUnconfirmed) : tr(Str::ChatSelf)) : name,mine);
    }
    c.clearClip();
    if (chat_.readFailed || (messages.empty() && !pending)) {
        const Str status = chat_.readFailed ? Str::ChatUnavailable : chat_.loaded ? Str::ChatEmpty : Str::ChatLoading;
        std::string label = tr(status);
        if (chat_.readFailed && chat_.readStatus) label += " (HTTP " + std::to_string(chat_.readStatus) + ")";
        fonts_.regular.draw(c,label,120,168,23,kWhite);
    }
    if (chat_.keyboardOpen) {
        c.fillRect({492,451,936,371},rgba(11,19,28,235),22);
        for (int i=0;i<42;++i) {
            Rect r = chatKeyRect(i);
            const bool focus = i == chat_.key;
            if (focus || i >= 40) c.fillRect(r,focus ? kWhite : withAlpha(kWhite,32),12);
            std::string label = i == 41 ? tr(Str::ChatSend) : i == 40 ? tr(Str::KeyDelete) :
                                i == 36 ? "_" : std::string(1,kChatKeys[i]);
            fonts_.semibold.draw(c,label,r.x+(r.w-fonts_.semibold.measure(label,23))/2,
                                 fonts_.semibold.centeredY(r.y,r.h,23),23,focus ? kBg : kWhite);
        }
    }
    c.fillRect(kChatInput,rgba(10,18,29,222),16);
    if (!chat_.keyboardOpen && !chat_.sendFocus) c.strokeRect(kChatInput,withAlpha(kWhite,160),2,16);
    const auto lines = fonts_.regular.wrap(chat_.draft,25,kChatInput.w-48,16);
    if (chat_.draft.empty()) fonts_.regular.draw(c,tr(Str::ChatCompose),144,864,25,kGray);
    else {
        int y = lines.size()>1 ? 845 : 864;
        for (int i=std::max(0,int(lines.size())-2);i<int(lines.size());++i) {
            fonts_.regular.draw(c,lines[i],144,y,25,kWhite); y+=34;
        }
    }
    c.fillRect(kChatKeyboard,rgba(23,33,46,221),16);
    c.fillRect(kChatSend,pending || chat_.draft.empty() ? rgba(53,64,74,223) : kWhite,16);
    if (chat_.sendFocus && !chat_.keyboardOpen) c.strokeRect(kChatSend,kWhite,3,16);
    auto button = [&](Rect r,Str id,Color ink) {
        const char* label = tr(id);
        fonts_.semibold.draw(c,label,r.x+(r.w-fonts_.semibold.measure(label,24))/2,865,24,ink);
    };
    button(kChatKeyboard,Str::ChatKeyboard,kWhite);
    button(kChatSend,pending ? Str::ChatSending : Str::ChatSend,pending || chat_.draft.empty() ? kWhite : kBg);
    if (chat_.sendFailed) {
        std::string status = tr(Str::ChatUnconfirmed);
        if (chat_.sendStatus) status += " (HTTP " + std::to_string(chat_.sendStatus) + ")";
        fonts_.regular.draw(c,fonts_.regular.fit(status,21,1660),120,944,21,kWhite);
    }
    hintRects_.clear();
    drawHints(c,{{kIconL1R1,tr(Str::ChatMessages)}, {kIconCircle,tr(Str::Back)}});
}

void AppUi::handleDirectory(const NavInput& in, UiEvent& ev) {
    auto& p = activeDirectory();
    if (in.back && p.kind == Directory::Chat && chat_.keyboardOpen) {
        chat_.keyboardOpen = chat_.composing = false; dirty_ = true; return;
    }
    if (in.back) { backDirectory(); return; }
    if (p.kind == Directory::Chat) {
        if (in.l1 || in.r1 || in.scrollRows || (!chat_.keyboardOpen && (in.up || in.down))) {
            const int count = int(chatHistory_[chat_.peer].messages.size());
            chat_.historyOffset = std::clamp(chat_.historyOffset + (in.l1 || in.up || in.scrollRows < 0 ? 1 : -1),
                                             0,std::max(0,count-1));
            dirty_ = true;
            return;
        }
        if (!chat_.keyboardOpen) {
            if (in.left || in.right) chat_.sendFocus = in.right;
            if (in.accept && chat_.sendFocus) pressChatKey(41);
            else if (in.accept) chat_.keyboardOpen = true;
            dirty_ = true; return;
        }
        int& key = chat_.key;
        if (in.accept) pressChatKey(key);
        else if (in.left) key = std::max(key >= 40 ? 40 : (key/10)*10, key-1);
        else if (in.right) key = std::min(key >= 40 ? 41 : (key/10)*10+9, key+1);
        else if (in.up) key = key >= 40 ? (key == 40 ? 32 : 37) : std::max(0,key-10);
        else if (in.down && key < 40) key = key >= 30 ? (key < 35 ? 40 : 41) : key+10;
        dirty_ = true;
        return;
    }
    if (p.kind == Directory::Genres) {
        const int last = static_cast<int>(p.labels.size())-1;
        if (in.accept) activateDirectory(ev);
        else if (directory_.empty() && (in.up || in.down || in.scrollRows)) {
            focusRow_ = std::clamp(focusRow_ + (in.up || in.scrollRows < 0 ? -1 : 1), 0, int(rows_.size())-1);
            retarget();
        }
        else if (in.down) p.allButtonFocused = true;
        else if (in.up) p.allButtonFocused = false;
        else if (last >= 0 && (in.left || in.right || in.scrollRows || in.scrollCards)) {
            p.allButtonFocused = false;
            const int step = in.scrollCards ? in.scrollCards : in.scrollRows ? in.scrollRows : in.right ? 1 : -1;
            p.focus = std::clamp(p.focus + step, 0, last);
        }
        dirty_ = true;
        return;
    }
    if (p.kind == Directory::All && in.square) { openDirectory(Directory::Genres, tr(Str::BrowseGenres)); return; }
    if (directory_.empty() && (in.up || in.scrollRows < 0) && p.focus < directoryColumns(p) && focusRow_ > 0) {
        backDirectory(); return;
    }
    if (directory_.empty() && (in.down || in.scrollRows > 0) && focusRow_+1 < static_cast<int>(rows_.size()) &&
        (p.games.empty() || p.focus / 6 == (static_cast<int>(p.games.size())-1)/6)) {
        ++focusRow_; retarget(); dirty_ = true; return;
    }
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
            else if (in.scrollCards) p.focus = std::clamp(p.focus + in.scrollCards, 0, count-1);
            else moveInGrid(p.focus, count, cols, in);
        }
    }
    dirty_ = true;
}

bool AppUi::directoryPointer(float x, float y, int button, UiEvent& ev) {
    auto& p = activeDirectory();
    const bool inlineShelf = directory_.empty();
    if (button == 3 || (button == 1 && horizon::contains(horizon::directoryHeading(horizon::kBrowseBack,inlineShelf), x, y))) {
        backDirectory();
        return true;
    }
    if (button != 1) return true;
    if (p.kind == Directory::Chat) {
        for (const auto& [icon,r] : hintRects_) {
            if (icon == kIconCircle && horizon::contains(r,x,y)) { backDirectory(); return true; }
        }
        if (horizon::contains(kChatSend,x,y)) { pressChatKey(41); return true; }
        if (horizon::contains(kChatKeyboard,x,y)) {
            chat_.keyboardOpen = !chat_.keyboardOpen;
            chat_.composing = false;
            dirty_ = true; return true;
        }
        if (horizon::contains(kChatInput,x,y)) {
            chat_.keyboardOpen = false; chat_.composing = true; chat_.sendFocus = false;
            dirty_ = true; return true;
        }
        for (int i=0;chat_.keyboardOpen && i<42;++i) if (horizon::contains(chatKeyRect(i),x,y)) {
            chat_.key = i; pressChatKey(i); break;
        }
        return true;
    }
    if (p.kind == Directory::Friends && horizon::contains(kChatAction,x,y)) {
        activateDirectory(ev); return true;
    }
    if (p.kind == Directory::All && horizon::contains(horizon::directoryHeading(horizon::kCatalogGenres,inlineShelf),x,y)) {
        openDirectory(Directory::Genres,tr(Str::BrowseGenres)); return true;
    }
    if (p.kind == Directory::Genres) {
        if (horizon::contains(horizon::kGenreAll,x,y)) {
            p.allButtonFocused = true; activateDirectory(ev); return true;
        }
        if (horizon::contains(horizon::kGenrePrev,x,y) || horizon::contains(horizon::kGenreNext,x,y)) {
            NavInput move;
            move.scrollCards = x < kW/2 ? -1 : 1;
            handleDirectory(move,ev); return true;
        }
    }
    for (const auto& [icon, rect] : hintRects_) {
        if (!horizon::contains(rect, x, y)) continue;
        NavInput input;
        if (icon == kIconSquare) input.square = true;
        else if (icon == kIconCircle) input.back = true;
        else continue;
        handleDirectory(input,ev); return true;
    }
    if (!p.games.empty() && horizon::contains(horizon::directoryHeading(horizon::kBrowseSearch,inlineShelf), x, y)) {
        NavInput search;
        search.triangle = true;
        handleDirectory(search, ev);
        return true;
    }
    if ((p.kind == Directory::Link || (p.kind == Directory::Ubisoft && p.games.empty())) && horizon::contains({740, 685, 440, 74}, x, y)) {
        activateDirectory(ev);
        return true;
    }
    const int count = static_cast<int>(std::max(p.labels.size(), p.games.size()));
    for (int i = 0; i < count; ++i) {
        if (!horizon::contains(directoryCell(p, i), x, y)) continue;
        p.focus = i;
        p.allButtonFocused = false;
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
    const auto& p = activeDirectory();
    drawCollectionBackdrop(c, lastRender_, focusedTile());
    const bool inlineShelf = directory_.empty();
    if (inlineShelf) {
        drawTopBar(c,true);
        drawTabs(c);
    }
    const Rect back = horizon::directoryHeading(horizon::kBrowseBack,inlineShelf);
    const int titleY = back.y - 5;
    c.line(back.x + 36, back.y + 20, back.x + 24, back.y + 32, 3, kWhite);
    c.line(back.x + 24, back.y + 32, back.x + 36, back.y + 44, 3, kWhite);
    fonts_.bold.draw(c, fonts_.bold.fit(p.title, 52, 1190), 184, titleY, 52, kWhite);
    if (p.kind == Directory::Chat) { drawChat(c,p.title); return; }
    const int count = static_cast<int>(std::max(p.games.size(), p.labels.size()));
    if (p.kind == Directory::All) {
        const Rect r = horizon::directoryHeading(horizon::kCatalogGenres,inlineShelf);
        c.fillRect(r, rgba(26,44,59,215), 15);
        const auto name = tr(Str::BrowseGenres);
        fonts_.semibold.draw(c, name, r.x+(r.w-fonts_.semibold.measure(name,24))/2,
                             fonts_.semibold.centeredY(r.y,r.h,24),24,kWhite);
    }
    if (!p.games.empty()) {
        fonts_.regular.draw(c, gamesCount(count), 186, titleY+77, 23, kGray);
        fonts_.semibold.draw(c, tr(Str::TabSearch), horizon::kBrowseSearch.x + 22, titleY+24, 24, kWhite);
    }
    if ((p.kind == Directory::Link || (p.kind == Directory::Ubisoft && p.games.empty()))) {
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
        const bool selected = i == p.focus && !p.allButtonFocused;
        if (p.kind == Directory::Genres) {
            drawGenreCard(c,fonts_.semibold,r,p.labels[i],selected);
            continue;
        }
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
            if (selected) {
                c.fillRect(r, withAlpha(kWhite,32), 12);
                c.fillRect({r.x,r.y+18,3,r.h-36}, kWhite, 2);
            }
            int x = r.x+28;
            if (p.kind == Directory::Friends && i < int(friends_.size())) {
                const auto& who = friends_[i];
                if (auto avatar = images_.get(who.gamerpicUrl, 52, 52)) c.drawImage(*avatar,x,r.y+15,255,-1);
                else drawFriendAvatar(c,{x,r.y+15,52,52});
                x += 72;
                c.fillCircle(x-25,r.y+56,6,who.presenceState == "Online" ? rgba(139,232,174) : kGray);
            }
            fonts_.semibold.draw(c, fonts_.semibold.fit(p.labels[i], 28, p.kind == Directory::Friends ? r.w-125 : r.w-56),
                                 x, fonts_.semibold.centeredY(r.y,r.h,28), 28, kWhite);
        }
    }
    if (p.kind == Directory::Friends && p.focus < int(friends_.size())) {
        const auto& who = friends_[p.focus];
        if (auto pic = images_.get(who.gamerpicUrl,128,128)) c.drawImage(*pic,1060,248,255,-1);
        else drawFriendAvatar(c,{1060,248,128,128});
        fonts_.bold.draw(c,fonts_.bold.fit(who.gamertag,49,715),1060,410,49,kWhite);
        fonts_.semibold.draw(c,presenceLabel(who.presenceState),1060,481,24,kWhite);
        int y = 525;
        for (const auto& line : fonts_.regular.wrap(who.presenceText,22,720,2)) {
            fonts_.regular.draw(c,line,1060,y,22,kWhite); y += 28;
        }
        if (!who.xuid.empty()) {
            c.fillRect(kChatAction,kWhite,16);
            fonts_.semibold.draw(c,tr(Str::ChatOpen),kChatAction.x+28,kChatAction.y+21,27,kBg);
        }
    }
    if (p.kind == Directory::Genres) {
        const auto chevron = [&](Rect r,int dir) {
            const int cx = r.x+r.w/2, cy = r.y+r.h/2;
            c.line(cx-5*dir,cy-10,cx+5*dir,cy,3,kWhite);
            c.line(cx+5*dir,cy,cx-5*dir,cy+10,3,kWhite);
        };
        if (p.focus > 0) chevron(horizon::kGenrePrev,-1);
        if (p.focus+1 < count) chevron(horizon::kGenreNext,1);
        const Rect all = horizon::kGenreAll;
        const bool focused = p.allButtonFocused || !count;
        c.fillRect(all, focused ? kWhite : rgba(37,50,64,245), 12);
        const auto label = tr(Str::ViewAllGames);
        fonts_.semibold.draw(c,label,all.x+(all.w-fonts_.semibold.measure(label,23))/2,
                             fonts_.semibold.centeredY(all.y,all.h,23),23,focused ? kBg : kWhite);
    }
    hintRects_.clear();
    if (p.kind == Directory::All) drawHints(c, {{kIconSquare,tr(Str::BrowseGenres)}, {kIconCircle,tr(Str::Back)}});
    else drawHints(c, {{kIconCircle, tr(Str::Back)}});
}

} // namespace xc::ui
