#include "halo2/frontend.hpp"
#include "halo2/frontend_online.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/frontend_lobby_game.hpp"
#include "halo2/frontend_lobby_setup.hpp"
#include "halo2/frontend_lobby_maps.hpp"
#include "halo2/frontend_lobby_ready.hpp"
#include "halo2/native_player_profile.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
namespace halo2 {
namespace {
// A painter may render several disposable MenuState objects. Its screen
// allocation identity must therefore be unique across those objects too.
std::atomic<std::uint64_t> next_menu_instance{1};
const UiVariant &builtin_variant(const FrontendContent &content,unsigned index) {
  return index<content.variants.size()?content.variants.at(index):content.bundled_variants.at(index-content.variants.size());
}
constexpr std::array<const char *, 15> campaign_names{
    "The Heretic", "The Armory",   "Cairo Station",    "Outskirts",
    "Metropolis",  "The Arbiter",  "The Oracle",       "Delta Halo",
    "Regret",      "Sacred Icon",  "Quarantine Zone",  "Gravemind",
    "Uprising",    "High Charity", "The Great Journey"};
constexpr std::array<const char *, 15> campaign_files{
    "00a_introduction",  "01a_tutorial",       "01b_spacestation",
    "03a_oldmombasa",    "03b_newmombasa",     "04a_gasgiant",
    "04b_floodlab",      "05a_deltaapproach",  "05b_deltatowers",
    "06a_sentinelwalls", "06b_floodzone",      "07a_highcharity",
    "08a_deltacliffs",   "07b_forerunnership", "08b_deltacontrol"};
constexpr std::array<const char *, 12> multiplayer_names{
    "Lockout",       "Ascension", "Midship",     "Beaver Creek",
    "Burial Mounds", "Colossus",  "Ivory Tower", "Headlong",
    "Waterworks",    "Zanzibar",  "Coagulation", "Foundation"};
constexpr std::array<const char *, 12> multiplayer_files{
    "lockout",       "ascension", "midship",     "beavercreek",
    "burial_mounds", "colossus",  "cyclotron",   "headlong",
    "waterworks",    "zanzibar",  "coagulation", "foundation"};
constexpr std::array<unsigned, 7> engine_order{0, 3, 1, 2, 4, 5, 6};
constexpr std::array<unsigned, 7> match_category{
    1, 2, 6, 3, 0, 8, 7}; // Indexed by actual engine type.
constexpr std::array<unsigned, 7> game_category{14, 15, 19, 16, 13, 21, 20};
constexpr std::array<unsigned, 7> team_category{32, 33, 37, 34, 31, 39, 38};
std::string by_index(const UiScreen &screen, unsigned low) {
  for (const auto &[id, text] : screen.strings)
    if ((id & 0xffff) == low)
      return frontend_online_text(text);
  return {};
}
bool appearance_window(unsigned screen) {
  return screen == 49 || screen == 50 || screen == 52 || screen == 233;
}
unsigned appearance_window_size(const FrontendContent &content,
                                const MenuFrame &frame) {
  const auto &screen = content.screens.at(frame.screen);
  return std::min(screen.panes.front().lists.front().visible,
                  unsigned(frame.entries.size()));
}
bool online_screen(unsigned screen) {
  // Actual mainmenu.map tag names: dialog_live_*, online_y_menu and
  // game_shell/xbox_live. Generic progress/error and Extras screens also
  // serve offline routes and must not acquire a Discord requirement.
  switch (screen) {
  case 21: case 26: case 27: case 31: case 34: case 35: case 36:
  case 182: case 186: case 210: case 211:
  case 215: case 216: case 217: case 219: case 236: case 237:
    return true;
  default: return false;
  }
}
} // namespace
MenuState::MenuState(const FrontendContent &content,
                     const std::filesystem::path &preferences)
    : content_(content), preferences_(preferences) {
  if (!preferences.empty() && std::filesystem::exists(preferences) &&
      std::filesystem::file_size(preferences) < 256 * 1024) {
    std::ifstream file(preferences);
    std::string signature;
    std::getline(file, signature);
    if (signature == "halo2-native-menu-v1") {
      std::string kind, name;
      std::string active;
      unsigned key;
      int value;
      std::vector<std::string> loaded;
      while (file >> kind) {
        if (kind == "active" && file >> std::quoted(name)) {
          if (!name.empty() && name.size() <= 48)
            active = name;
        } else if (kind == "variant_name" && file >> std::quoted(name) >> key) {
          if (!name.empty() && name.size() <= 64 && key < 7) {
            variant_name_ = name;
            engine_ = key;
          }
        } else if (kind == "saved_variant" &&
                   file >> std::quoted(name) >> key) {
          if (!name.empty() && name.size() <= 64 && key < 7 &&
              saved_variants_.size() < 32 &&
              std::none_of(
                  saved_variants_.begin(), saved_variants_.end(),
                  [&](const auto &v) { return v.name == name; }))
            saved_variants_.push_back({name, key, {}});
        } else if (kind == "saved_value" &&
                   file >> std::quoted(name) >> key >> value) {
          for (auto &v : saved_variants_)
            if (v.name == name)
              v.values.append(key, value);
        } else if (kind == "profile" && file >> std::quoted(name)) {
          if (!name.empty() && name.size() <= 48 && loaded.size() < 32 &&
              std::find(loaded.begin(), loaded.end(), name) == loaded.end())
            loaded.push_back(name);
        } else if (kind == "option" &&
                   file >> std::quoted(name) >> key >> value) {
          if (key < 65536 && value >= 0 && value <= 1000)
            profile_values_[name][key] = value;
        } else if (kind == "variant" && file >> key >> value) {
          variant_values_.append(key, value);
        } else if (kind == "pc_display") {
          DisplaySettings v;
          unsigned safe{};
          if (file >> v.internal_width >> v.internal_height >> v.anti_aliasing
                   >> v.brightness >> v.gamma >> v.detail >> safe >> v.volume >> v.subtitles &&
              ((v.internal_width == 0 && v.internal_height == 0) ||
               (v.internal_width >= 320 && v.internal_width <= 7680 &&
                v.internal_height >= 240 && v.internal_height <= 4320)) &&
              v.anti_aliasing <= 2 && v.brightness < 10 && v.gamma < 11 &&
              v.detail < 3 && safe < 2 && v.volume <= 10 && v.subtitles < 3) {
            v.safe_area = safe != 0;
            pc_display_ = v;
          }
        } else if(kind=="pc_speakers") {
          unsigned speakers{};if(file>>speakers && speakers<5)pc_display_.speakers=speakers;
        } else if(kind=="pc_two_player_split") {
          unsigned layout{};if(file>>layout && layout<2)pc_display_.two_player_split=TwoPlayerSplit(layout);
        } else if(kind=="pc_crt_filter") {
          unsigned enabled{};if(file>>enabled && enabled<2)pc_display_.crt_filter=enabled!=0;
        } else if(kind=="pc_fov") {
          unsigned on_foot{},vehicle{};
          if(file>>on_foot>>vehicle && valid_field_of_view(on_foot) && valid_field_of_view(vehicle)) {
            pc_display_.field_of_view=on_foot;pc_display_.vehicle_field_of_view=vehicle;
          }
        } else if (kind == "pc_video") {
          unsigned width{}, height{}, fullscreen{};
          if (file >> width >> height >> fullscreen && width >= 320 && height >= 240 &&
              width <= 16384 && height <= 16384 && fullscreen <= 1)
            saved_pc_video_ = std::array<unsigned,3>{width,height,fullscreen};
        } else {
          std::string discard;
          std::getline(file, discard);
        }
      }
      profiles_ = std::move(loaded);
      if (const auto it = std::find(profiles_.begin(), profiles_.end(), active);
          it != profiles_.end())
        profile_ = unsigned(it - profiles_.begin());
    }
  }
  for (unsigned i = 0; i < saved_variants_.size(); ++i)
    if (saved_variants_[i].name == variant_name_ &&
        saved_variants_[i].engine == engine_)
      saved_variant_ = int(i);
  stack_.push_back(build(9, 0));
}
std::string MenuState::profile_name() const {
  return profiles_.empty() ? std::string{} : profiles_.at(profile_);
}
void MenuState::set_multiplayer_map_validator(std::function<void(const FrontendLaunch &)> validator) {
  multiplayer_map_validator_=std::move(validator);
}
void MenuState::set_discord_login(std::shared_ptr<DiscordLogin> login) {
  discord_login_ = std::move(login);
}
void MenuState::return_online(std::string message) {
  // Allocate fresh ONLINE navigation after world teardown or local PCR Done.
  stack_.clear();launch.reset();quit=false;mode_=3;
  open(6);awaiting_discord_login_=true;open(186);
  if(!message.empty())unavailable(std::move(message));
}
std::optional<DiscordIdentity> MenuState::online_identity() const {
  if (online_account_ && !account_.authenticated) return std::nullopt;
  return discord_login_ ? discord_login_->snapshot().identity : std::nullopt;
}
void MenuState::update_online_login() {
  update_online_couch();
  update_online_friends();
  update_online_account();
  if (!discord_login_) return;
  const auto status = discord_login_->snapshot();
  if (frame().screen == 20) {
    if (awaiting_discord_login_ && online_identity()) {
      awaiting_discord_login_ = false;
      const auto screen = pending_online_screen_, context = pending_online_context_;
      stack_.pop_back();
      open(screen, context);
      return;
    }
    if (status.status == DiscordLoginStatus::Failed) awaiting_discord_login_ = false;
  } else if (!status.identity && std::any_of(stack_.begin(), stack_.end(), [](const auto &f) { return online_screen(f.screen); })) {
    if (online_matchmaking_) online_matchmaking_->sign_out();
    matchmaking_active_ = quickmatch_pending_ = false;
    while (stack_.size() > 1 && frame().screen != 6) stack_.pop_back();
    open(20);
  }
  // The account remains Discord-verified while its local screen name changes.
  // Refresh retained panels too, so Back never reveals the previous name.
  for (auto &retained : stack_) {
    if (retained.screen != 20 && retained.screen != 186) continue;
    auto updated = build(retained.screen, retained.context);
    if (retained.description == updated.description &&
        retained.entries.front().label == updated.entries.front().label) continue;
    updated.instance = retained.instance;
    updated.pane_created_clock = retained.pane_created_clock;
    updated.selected = std::min(retained.selected, unsigned(updated.entries.size() - 1));
    retained = std::move(updated);
    ++revision;
  }
}
int MenuState::profile_option(unsigned key, int fallback) const {
  // 148ADA keeps one edited settings copy; child-page focus producers update
  // that copy, while the saved profile changes only at 148B8C/148BFF.
  if (key >= 0x100 && key <= 0x103)
    return appearance().colors[key - 0x100];
  if (key == 52) return appearance().species;
  if (key == 49) return appearance().foreground;
  if (key == 233) return appearance().background;
  if (key == 0x104) return appearance().flags;
  const auto profile = profile_values_.find(profile_name());
  if (profile == profile_values_.end())
    return fallback;
  const auto value = profile->second.find(key);
  return value == profile->second.end() ? fallback : value->second;
}
FrontendAppearance MenuState::appearance_for_profile(const std::string &name) const {
  FrontendAppearance value;
  const auto found = profile_values_.find(name);
  if (found == profile_values_.end())
    return value;
  const auto option = [&](unsigned key, int fallback) {
    const auto at = found->second.find(key);
    return at == found->second.end() ? fallback : at->second;
  };
  for (unsigned i = 0; i < value.colors.size(); ++i)
    value.colors[i] = std::int8_t(option(0x100 + i, value.colors[i]));
  value.species = std::int8_t(option(52, 0));
  value.foreground = std::uint8_t(option(49, 0));
  value.background = std::uint8_t(option(233, 0));
  value.flags = std::uint8_t(option(0x104, 0));
  value.reserved = std::uint8_t(option(0x105, 0));
  return value;
}
FrontendAppearance MenuState::profile_appearance() const {
  return appearance_for_profile(profile_name());
}
FrontendAppearance MenuState::appearance() const {
  return appearance_edit_.value_or(profile_appearance());
}
void MenuState::begin_appearance_edit() {
  if (appearance_edit_ && appearance_profile_ == profile_name())
    return;
  commit_appearance_edit();
  appearance_profile_ = profile_name();
  appearance_edit_ = profile_appearance();
}
void MenuState::commit_appearance_edit() {
  if (!appearance_edit_)
    return;
  if (!appearance_profile_.empty()) {
    auto &values = profile_values_[appearance_profile_];
    const auto &value = *appearance_edit_;
    for (unsigned i = 0; i < value.colors.size(); ++i)
      values[0x100 + i] = value.colors[i];
    values[52] = value.species;
    values[49] = value.foreground;
    values[233] = value.background;
    values[0x104] = value.flags;
    values[0x105] = value.reserved;
  }
  appearance_edit_.reset();
  appearance_profile_.clear();
  appearance_snapshots_.clear();
  save();
}
void MenuState::preview_appearance_selection() {
  if (!appearance_edit_ || frame().selected >= frame().entries.size())
    return;
  preview_frontend_appearance(*appearance_edit_, frame().screen, frame().context,
                              frame().entries[frame().selected].value);
}
std::optional<FrontendAppearance> MenuState::profile_entry_appearance(
    const MenuFrame &frame, unsigned index) const {
  if ((frame.screen != 10 && frame.screen != 193) || index >= frame.entries.size())
    return {};
  const auto &entry = frame.entries[index];
  if (entry.destination != -3 || entry.value < 0 || unsigned(entry.value) >= profiles_.size())
    return {};
  return appearance_for_profile(profiles_[unsigned(entry.value)]);
}
std::optional<FrontendAppearance> MenuState::focused_profile_appearance(const MenuFrame &frame) const {
  return frame.screen == 193 ? profile_entry_appearance(frame, frame.selected)
                             : std::nullopt;
}
std::optional<int> MenuState::focused_profile_control(
    const MenuFrame &frame, unsigned screen) const {
  if (frame.screen != 193 || (screen != 40 && screen != 41) ||
      frame.selected >= frame.entries.size())
    return {};
  const auto &entry = frame.entries[frame.selected];
  if (entry.destination != -3 || entry.value < 0 || unsigned(entry.value) >= profiles_.size())
    return {};
  const auto profile = profile_values_.find(profiles_[unsigned(entry.value)]);
  if (profile == profile_values_.end())
    return 0; // Original1A0750 initializes both layout bytes to0.
  const auto value = profile->second.find(screen);
  return value == profile->second.end() ? 0 : value->second;
}
std::optional<FrontendAppearance> MenuState::model_appearance(const MenuFrame &frame) const {
  if (frame.screen == 193)
    return focused_profile_appearance(frame);
  if (frame.screen == 30)
    return signed_in_.empty() ? std::optional<FrontendAppearance>{}
                              : appearance_for_profile(signed_in_);
  if (frame.screen == 47 || frame.screen == 50)
    return appearance();
  return {};
}
std::optional<int> MenuState::focused_profile_species(const MenuFrame &frame) const {
  const auto value = focused_profile_appearance(frame);
  return value ? std::optional<int>{value->species} : std::optional<int>{};
}
MenuFrame MenuState::build(unsigned id, unsigned context) const {
  if(id==13 || id==vista_menu(13))
    throw FormatError("Retired screen13 is unavailable in native navigation");
  if (id == 19 && content_.vista) return build_pc(vista_menu(19), context);
  if (is_vista_menu(id)) return build_pc(id, context);
  if (!content_.screens.contains(id))
    throw FormatError("retail menu screen not found: " + std::to_string(id));
  const auto &s = content_.screens.at(id);
  MenuFrame f;
  f.instance = next_menu_instance.fetch_add(1, std::memory_order_relaxed);
  f.pane_created_clock = ui_clock_;
  f.screen = id;
  f.context = context;
  auto add = [&](unsigned low, int destination = -1, int value = 0) {
    auto label = by_index(s, low);
    if (label.empty())
      throw FormatError("retail menu label missing on screen " +
                        std::to_string(id) + ": " + std::to_string(low));
    f.entries.push_back({label, destination, value});
  };
  auto range = [&](unsigned first, unsigned last) {
    for (auto i = first; i <= last; ++i)
      add(i, -2, int(i - first));
  };
  switch (id) {
  case 6:
    add(0x10b, 218);
    add(0x283, 20);
    f.entries.back().label = "ONLINE";
    add(0x284, 30, 0);
    add(0x285, 30, 1);
    add(0x286, 19);
    break;
  case 19:
    add(0x196, 193);
    add(0x197, 60, 1);
    if (content_.vista) f.entries.push_back({content_.vista->global_strings.at(0x0e000820), int(vista_menu(256)), 0});
    break;
  case 10:
  case 193:
    if(id==10 && joining_online_)f.entries.push_back({"Guest",-80,0});
    add(0x195, 167, -1);
    for (unsigned i = 0; i < profiles_.size(); ++i) {
      if(id==10 && joining_controller_ &&
          (profiles_[i]==signed_in_ || std::find(local_profiles_.begin(),local_profiles_.end(),profiles_[i])!=local_profiles_.end()))continue;
      f.entries.push_back({profiles_[i], -3, int(i)});
    }
    break;
  case 38:
    add(0x2ef, 39);
    add(0x2f0, 47);
    add(0x302, 55);
    add(0x301, 54);
    add(0x2f2, 58);
    add(0x2f1, 242);
    if (content_.vista) f.entries.push_back({content_.vista->global_strings.at(0x16000853), int(vista_menu(258)), 0});
    break;
  case 39: {
    const std::array<unsigned, 6> ids{0x3dd, 0x3de, 0x3df, 0x3e0, 0x3e1, 0x3e2};
    const std::array<int, 6> targets{41, 40, 42, 43, 44, 183};
    for (unsigned i = 0; i < ids.size(); ++i)
      add(ids[i], targets[i]);
    break;
  }
  case 41:
    add(1, -2, 0);
    add(0x2f5, -2, 1);
    add(0x2f6, -2, 2);
    add(0x2f7, -2, 3);
    break;
  case 40:
    // Original2C647D admits four records;2C6512/2C6567 map0..3
    // directly to Default/Southpaw/Boxer/GreenThumb and the saved enum.
    add(1, -2, 0);
    add(0x3e4, -2, 1);
    add(0x3e5, -2, 2);
    add(0x3e6, -2, 3);
    break;
  case 42:
    range(0x3cf, 0x3d8);
    break;
  case 43:
  case 44:
    add(0x308, -2, 1);
    add(0x309, -2, 0);
    break;
  case 183:
    add(0x181, -2, 1);
    add(0x182, -2, 0);
    break;
  case 45:
    range(0x3d9, 0x3dc);
    break;
  case 47:
    add(0x2f8, 52);
    add(0x2f9, 50, 0);
    add(0x2fa, 50, 1);
    add(0x2fb, 50, 2);
    add(0x2fc, 50, 3);
    add(0x2fd, 49);
    add(0x2fe, 233);
    break;
  case 50:
    range(0x30a, 0x31b);
    break;
  case 49:
    range(0x30a, 0x349);
    break;
  case 233:
    range(0x30a, 0x329);
    break;
  case 52:
    add(0x2ff, -2, 0);
    add(0x300, -2, 1);
    break;
  case 53:
    add(0x301, 54);
    add(0x302, 55);
    break;
  case 54:
    add(0x21c, -2, 0);
    add(0x303, -2, 1);
    break;
  case 55:
    add(0x304, -2, 0);
    add(0x306, -2, 1);
    add(0x305, -2, 2);
    add(0x307, -2, 3);
    break;
  case 58:
    add(0x183, -2, 1);
    add(0x184, -2, 0);
    break;
  case 242:
    add(1, -2, 0);
    add(0x181, -2, 1);
    add(0x182, -2, 2);
    break;
  case 48:
    add(0x146, -5);
    add(0x147, -6);
    break;
  case 218:
    add(0x424, 12);
    add(0x425, -4);
    add(0x426, 11);
    add(0x427, 30, 2);
    break;
  case 1:
  case 12:
    add(0x28e, -2, 0);
    add(0xb8, -2, 1);
    add(0x28f, -2, 2);
    add(0x290, -2, 3);
    break;
  case 2:
  case 11:
    for (unsigned i = 0; i < campaign_names.size(); ++i)
      f.entries.push_back({campaign_names[i], -2, int(i)});
    break;
  case 17:
    for (unsigned i = 0; i < multiplayer_names.size(); ++i)
      f.entries.push_back({multiplayer_names[i], -2, int(i)});
    break;
  case 206: {
    // Native profiles have no imported campaign-completion keys. Original
    // new-profile keys are zero/NONE; Foundation's literal unlock flag applies.
    const auto list = frontend_lobby_maps(content_.multiplayer_maps, content_.campaign_maps,
                                         frontend_lobby_new_profile_map_progress);
    for (const auto index : list.catalog_indices) {
      const auto &map = content_.multiplayer_maps[index];
      const auto leaf = map.scenario_path.substr(map.scenario_path.find_last_of("\\/") + 1);
      for (unsigned i = 0; i < multiplayer_files.size(); ++i)
        if (leaf == multiplayer_files[i]) f.entries.push_back({map.name, -2, int(i)});
    }
    break;
  }
  case 60:
  case 207: {
    const std::array<unsigned, 7> ids{0x10f, 0x237, 0x110, 0x113,
                                      0x439, 0x10e, 0x115};
    for (unsigned i = 0; i < ids.size(); ++i)
      add(ids[i], id == 60 ? 15 : 208, int(engine_order[i]));
    break;
  }
  case 15:
  case 208:
    for (unsigned i = 0; i < content_.variants.size(); ++i)
      if (content_.variants[i].engine == engine_)
        f.entries.push_back({content_.variants[i].name, -7, int(i)});
    for (unsigned i=0;i<content_.bundled_variants.size();++i)
      if(content_.bundled_variants[i].engine==engine_)
        f.entries.push_back({content_.bundled_variants[i].name,-7,int(content_.variants.size()+i)});
    for (unsigned i = 0; i < saved_variants_.size(); ++i)
      if (saved_variants_[i].engine == engine_)
        f.entries.push_back({saved_variants_[i].name, -9, int(i)});
    break;
  case 178: {
    const std::array<unsigned, 6> ids{0x434, 0x430, 0x435, 0x42e, 0x433, 0x436};
    const std::array<unsigned, 6> categories{
        match_category.at(engine_), 9,  team_category.at(engine_),
        game_category.at(engine_),  11, 12};
    for (unsigned i = 0; i < ids.size(); ++i)
      add(ids[i], 221, int(categories[i]));
    break;
  }
  case 221:
  case 222: {
    const auto category = id == 222 ? 22 + match_category.at(engine_) : context;
    if (const auto it = content_.categories.find(category);
        it != content_.categories.end())
      for (auto datum : it->second)
        f.entries.push_back(
            {content_.parameters.at(datum).title, 223, int(datum)});
    if (id == 221) {
      const auto group = category == 9    ? 1
                         : category == 11 ? 4
                         : category == 12 ? 5
                         : category >= 31 ? 2
                         : category >= 13 ? 3
                                          : 0;
      f.title = by_index(s, 0x189e + group);
    }
    break;
  }
  case 223:
    if (const auto it = content_.parameters.find(context);
        it != content_.parameters.end()) {
      f.parameter = context;
      f.title = it->second.header;
      f.description = it->second.description;
      for (const auto &v : it->second.choices)
        f.entries.push_back({v.label, -2, v.value});
    }
    break;
  case 14:
    if(mode_==3){populate_online_party(f);break;}
    // The authored buttons provide the labels; their actions use retail order.
    if (!s.panes.empty())
      for (const auto &button : s.panes.front().buttons) {
        const auto label = content_.localized(s, button.text.string);
        f.entries.push_back({label, f.entries.empty() ? -4 : 23, 0});
      }
    break;
  case 23:
    if (const auto lobby = lobby_view())
      for (const auto &row : frontend_lobby_setup_rows(lobby->input.session_enum))
        f.entries.push_back({content_.localized(s, row.label), row.destination, row.semantic});
    break;
  case 25:
    add(0x201, -2, 0);
    add(0x202, -2, 1);
    add(0x203, -2, 2);
    break;
  case 30:
    f.entries = {{profile_name(), 14, 0}, {by_index(s, 0x28a), 10, 1}};
    break;
  case 20:
    f.title = "ONLINE";
    if (const auto identity = online_identity()) {
      f.entries = {{profile_name().empty() ? "Continue" : "Continue as " + profile_name(), 186, 0},
                   {"Change screen name", 167, 1}, {"Sign out of Discord", -12, 0}};
      f.description = "Screen name: " + profile_name() + "\nDiscord: " + identity->username;
    } else {
      const auto status = discord_login_ ? discord_login_->snapshot() : DiscordLoginSnapshot{};
      const bool busy = status.status == DiscordLoginStatus::Waiting || status.status == DiscordLoginStatus::Verifying;
      f.entries = {{busy ? "Waiting for Discord..." : "Sign in with Discord", -12, 0}};
      f.description = status.identity && online_account_ ? account_.message : status.message.empty() ? "Sign in with Discord to access ONLINE." : status.message;
      if (status.identity && online_account_) f.entries.front().label = account_.busy ? "Verifying ONLINE identity..." : "Retry ONLINE connection";
    }
    break;
  case 21:
    f.entries = {{"Continue", -4, 0}};
    break;
  case 186:
    f.title = "ONLINE";
    add(0x41f, 210);
    add(0x420, 219);
    f.entries.back().label = "Optimatch";
    add(0x421, 14);
    add(0x445, -4);
    f.entries.back().label = "Discord Account";
    f.entries.push_back({"Server Browser", 219, int(online_server_browser_context)});
    if (online_identity()) f.description = profile_name()+" | "+std::to_string(local_players().size())+" / 4 local | A: join | X: options";
    break;
  case 26:
    if (context == 1) populate_online_clans(f);
    else if(context==2)populate_online_party(f);
    else populate_online_friends(f);
    break;
  case 27:
    break; // The service supplies these lists; there are no online users
           // locally.
  case 216:
    if(context==43)populate_online_friends(f);else populate_online_party(f);
    break;
  case 217:
    populate_online_clans(f);
    break;
  case 31:
    if(online_couch_screen(f)){populate_online_couch(f);break;}
    if(context==43 || context==44){populate_online_friends(f);break;}
    if (context == 41) { populate_online_clans(f); break; }
    if(context==42){populate_online_party(f);break;}
    add(0x2a6, 215);
    add(0x2a7, 211);
    add(0x2ac, 34);
    add(0x2ae, -4);
    add(0x2b4, -4);
    add(0x2ba, -4);
    break;
  case 34:
    range(0x291, 0x297);
    break;
  case 35:
    add(0x1865, 167, 3);
    add(0x1866, 36);
    break;
  case 215:
    add(0x601, 167, 3);
    add(0x602, 36);
    add(0x2a6, -4);
    break;
  case 211:
    if (context == 41) { populate_online_clans(f); break; }
    add(0x2a6, 215);
    add(0x2a7, -4);
    break;
  case 36:
    f.entries = {{"Record", -4, 0}, {"Play", -4, 0}, {"Done", -5, 0}};
    break;
  case 236:
    populate_online_clans(f);
    break;
  case 177:
  case 184:
    f.entries = {{profile_name(), -4, 0}};
    break;
  case 194:
    add(0x19e, context ? 178 : 38);
    add(0x19f, 167, context ? 6 : 5);
    add(0x1a0, -12, context);
    break;
  case 210:
    populate_online_matchmaking(f);
    break;
  case 219:
  case 237:
    if (context == online_server_browser_context) populate_online_directory(f);
    else populate_online_matchmaking(f);
    break;
  case 239:
    range(0x6f6, 0x704);
    break;
  case 241:
    add(0x181, -2, 1);
    add(0x182, -2, 0);
    break;
  case 7:
  case 8:
  case 33:
  case 230:
  case 240:
    f.entries = {{"OK", -5, 0}};
    break;
  case 167:
    if (!s.panes.empty())
      for (unsigned i = 0;
           i < std::min(47u, unsigned(s.panes[0].buttons.size())); ++i)
        f.entries.push_back(
            {content_.localized(s, s.panes[0].buttons[i].text.string), -8,
             int(i)});
    f.title = by_index(s, context == 2 || context == 6 ? 0x44c
                          : context == 3 ? 0x453
                          : context == 4 ? 0x455
                                         : 0x449);
    f.description = by_index(s, context == 2 || context == 6 ? 0x44d
                                : context == 3 ? 0x454
                                : context == 4 ? 0x456
                                : context == 1 ? 0x44b
                                               : 0x44a);
    f.selected = 40;
    break;
  default:
    if (!s.panes.empty()) {
      for (const auto &list : s.panes.front().lists)
        for (auto label : list.labels)
          f.entries.push_back({content_.localized(s, label), -4, 0});
      for (const auto &button : s.panes.front().buttons)
        f.entries.push_back({content_.localized(s, button.text.string), -4, 0});
    }
  }
  if (!f.entries.empty() && id != 167) {
    int selected_value = 0;
    if (id == 223 && f.parameter) {
      const auto &p = content_.parameters.at(f.parameter);
      selected_value = source_game_variant_get(
          source_game_variant({variant_name_,engine_,variant_values_}),
          std::bit_cast<std::int32_t>(std::uint32_t(p.parameter)));
    } else if (id == 10 || id == 193)
      selected_value = int(profile_);
    else if (id == 11 || id == 2)
      selected_value = int(level_);
    else if (id == 1 || id == 12)
      selected_value = int(difficulty_);
    else if (id == 17 || id == 206)
      selected_value = int(map_);
    else if (id == 50)
      selected_value = appearance().colors.at(context);
    else if (id == 52)
      selected_value = appearance().species;
    else if (id == 49)
      selected_value = appearance().foreground;
    else if (id == 233)
      selected_value = appearance().background;
    else if (const auto v = profile_values_.find(profile_name());
             v != profile_values_.end()) {
      const auto key = id == 50 ? 0x100 + context : id;
      if (v->second.contains(key))
        selected_value = v->second.at(key);
    }
    for (unsigned i = 0; i < f.entries.size(); ++i)
      if (f.entries[i].value == selected_value) {
        f.selected = i;
        break;
      }
  }
  if (appearance_window(id) && !f.entries.empty()) {
    const auto visible = appearance_window_size(content_, f);
    // 24C515 selects the last authored cell when a datum is outside the
    // window; 24C461 then assigns that cell and its preceding siblings.
    f.first_visible = f.selected >= visible ? f.selected - visible + 1 : 0;
  }
  return f;
}
void MenuState::open(unsigned screen, unsigned context) {
  if(screen==vista_menu(16) || screen==vista_menu(185))
    throw FormatError("Vista PCR requires its retained source owner factory");
  if (screen == 21) screen = 20; // Legacy Xbox password entry is replaced by browser OAuth.
  if ((online_screen(screen) || (mode_ == 3 && (screen == 14 || screen == 204 || screen == 231))) && !online_identity()) {
    pending_online_screen_ = screen;
    pending_online_context_ = context;
    screen = 20; context = 0;
  } else if (screen == 20) {
    pending_online_screen_ = 186; pending_online_context_ = 0;
  }
  if (stack_.size() >= 64)
    throw FormatError("menu navigation stack limit");
  const bool appearance_child = screen == 50 || screen == 52 || screen == 49 || screen == 233;
  if (screen == 38 || screen == 47 || appearance_child)
    begin_appearance_edit();
  if (appearance_child)
    appearance_snapshots_.emplace_back(stack_.size(), appearance());
  stack_.push_back(build(screen, context));
  if(screen==14 && mode_==3) {
    if(online_account_ && !account_.party && !online_account_->snapshot().busy)
      online_account_->party_command({"create"});
    if(!matchmaking_active_)refresh_online_matchmaking();
  }
  if (screen == 219 && context == online_server_browser_context) refresh_online_directory();
  if (screen == 219 && context == 0) refresh_online_matchmaking();
  if (screen == 210) {
    quickmatch_pending_ = true;
    if (!matchmaking_active_) refresh_online_matchmaking();
    update_online_matchmaking();
  }
  preview_appearance_selection();
  if (screen == 167) {
    keyboard_purpose_ = context;
    keyboard_cursor_ = keyboard_text.size();
    keyboard_selected_ = !keyboard_text.empty();
    keyboard_flags_ = 0;
    if (context == 4) { stack_.back().title="CREATE CLAN"; stack_.back().description="Enter a clan name. Maximum 15 letters, numbers or spaces."; }
    if (context == 7) { stack_.back().title="DISCORD USERNAME"; stack_.back().description="Enter the Discord username of a player who has signed into ONLINE."; }
    if (context == 8) { stack_.back().title="MESSAGE CLAN"; stack_.back().description="Send a text message to your clan. Maximum 128 characters."; }
    if(context==9){stack_.back().title="INVITE TO PARTY";stack_.back().description="Enter the Discord username of a player who has signed into ONLINE.";}
    keyboard_labels();
  }
  ++revision;
}
void MenuState::open_game_pause(bool campaign) {
  if(!content_.gameplay_overlay || !content_.vista)
    throw FormatError("Pause menu requires its gameplay UI source");
  stack_.clear();
  stack_.push_back(build(vista_menu(18),unsigned(campaign)));
  ++revision;
}
void MenuState::open_vista_pcr(unsigned id,std::shared_ptr<const VistaPcrScreenOwner> owner) {
  if((id!=16 && id!=185) || !owner || !owner->results || owner->pane>=6 ||
      owner->results->player_count>16 || owner->results->team_count>16 ||
      !content_.vista || !content_.vista->screens.contains(id))
    throw FormatError("Vista PCR factory requires actual bounded owner and source16/185 definition");
  MenuFrame f;
  f.instance=next_menu_instance.fetch_add(1,std::memory_order_relaxed);
  f.pane_created_clock=ui_clock_;f.screen=vista_menu(id);f.context=owner->pane;
  f.vista_pcr=std::move(owner);
  const auto count=f.context?f.vista_pcr->results->player_count:f.vista_pcr->results->team_count;
  if(f.vista_pcr->selected_row!=0xffffffffu && f.vista_pcr->selected_row>=count)
    throw FormatError("Vista PCR initial focus is outside actual retained rows");
  f.selected=f.vista_pcr->selected_row;
  f.entries.resize(count);
  for(unsigned i=0;i<count;++i)f.entries[i].value=int(i);
  // A source PCR allocation owns a new postmatch channel lifetime. Retaining
  // an old match's lobby as Back history can accidentally resume that match.
  stack_.clear();launch.reset();quit=false;
  stack_.push_back(std::move(f));++revision;
}
void MenuState::vista_pcr_pane(unsigned pane) {
  auto& f=stack_.back();
  if(!f.vista_pcr || pane>=6)throw FormatError("Vista PCR pane requires its retained source owner");
  if(f.context==pane)return;
  const auto count=pane?f.vista_pcr->results->player_count:f.vista_pcr->results->team_count;
  const auto selected=f.selected>=count?(count?0u:0xffffffffu):f.selected;
  if(selected!=f.selected && !f.vista_pcr->select_row)
    throw FormatError("Vista PCR pane focus requires its selection owner");
  f.context=pane;
  f.entries.clear();f.entries.resize(count);
  for(unsigned i=0;i<count;++i)f.entries[i].value=int(i);
  // Keep the existing native clamp and the row callback's shared selection
  // owner synchronized, including NONE for an empty pane. This clamp is a
  // native adapter policy; it does not establish Vista's initial focus rule.
  if(selected!=f.selected){f.selected=selected;f.vista_pcr->select_row(selected);}
  ++f.pane_generation;f.pane_created_clock=ui_clock_;++revision;
}
void MenuState::open_vista_lobby() {
  // The retained fields are the actual selected profile, mode, map and game
  // options. build() allocates a new screen/pane identity from those owners.
  // No readiness, membership or lifecycle phase is assigned by this factory.
  auto lobby=build(14,0);
  stack_.clear();launch.reset();quit=false;
  stack_.push_back(std::move(lobby));++revision;
}
void MenuState::open_native_mainmenu(const std::function<void()>& before_register) {
  if(!before_register)throw FormatError("Native main-menu return requires its source presence publication owner");
  auto menu=build(6,0);
  before_register();
  std::vector<MenuFrame> tasks;
  for(auto& f:stack_)if(f.vista_leave_task)tasks.push_back(std::move(f));
  stack_.clear();stack_.push_back(std::move(menu));
  for(auto& f:tasks)stack_.push_back(std::move(f));
  launch.reset();quit=false;++revision;
}
bool MenuState::vista_mainmenu_row_enabled(unsigned datum) const {
  if(datum>=6)throw FormatError("Vista main-menu row requires source datum0..5");
  //40A1D6 is true only forLIVE/NETWORK.40A276 combines it with the byte
  //returned by the actual platform PBuffer getter. Missing native ownership
  //is an explicit unsupported boundary, not an original disabled verdict.
  return (datum!=1 && datum!=2) ||
      (vista_mainmenu_pbuffer_byte_ && *vista_mainmenu_pbuffer_byte_==0);
}
std::uint64_t MenuState::open_vista_leave_task(std::uint32_t controller,
    std::shared_ptr<VistaLeaveTaskScreenOwner> owner) {
  if(!owner || controller>=4 || owner->instance || !content_.vista ||
      !content_.vista->screens.contains(182))
    throw FormatError("Vista Leave task factory requires a fresh actual owner and source182 definition");
  (void)vista_lobby_leave_task_descriptor(std::int32_t(controller));
  //6106A2 ->611D81 zeroes the 34B constructor descriptor;6115FE copies
  // its word+0E into widget+42. No unproved flag from zeroed tail is read.
  owner->storage[0x42]=owner->storage[0x43]=std::byte{};
  owner->controller=controller;
  vista_lobby_leave_task_bind(owner->storage);
  auto f=build_pc(vista_menu(182),controller);
  owner->instance=f.instance;f.vista_leave_task=std::move(owner);
  stack_.push_back(std::move(f));++revision;return frame().instance;
}
void MenuState::set_vista_leave_task_status(std::uint64_t instance,std::uint32_t sid) {
  const auto it=std::find_if(stack_.begin(),stack_.end(),[&](const auto& f){return f.instance==instance && f.vista_leave_task;});
  if(it==stack_.end())throw FormatError("Vista Leave status requires its live task allocation");
  if(!content_.vista->screens.at(182).strings.contains(sid))
    throw FormatError("Vista Leave status string is absent from its actual screen");
  if(it->vista_leave_task->status_string!=sid){it->vista_leave_task->status_string=sid;++revision;}
}
void MenuState::close_vista_leave_task(std::uint64_t instance,std::uint32_t reason) {
  if(reason!=3)throw FormatError("Vista Leave task close requires source reason3");
  const auto it=std::find_if(stack_.begin(),stack_.end(),[&](const auto& f){return f.instance==instance && f.vista_leave_task;});
  if(it==stack_.end() || stack_.size()==1)throw FormatError("Vista Leave close requires its independent live allocation");
  stack_.erase(it);++revision;
}
unsigned MenuState::vista_session_screen() const {
  for(auto it=stack_.rbegin();it!=stack_.rend();++it)if(!it->vista_leave_task)return it->screen;
  throw FormatError("Vista session query requires its actual channel5 owner");
}
void MenuState::open_local_title() {
  stack_.clear();stack_.push_back(build(9,0));launch.reset();quit=false;++revision;
}
std::vector<FrontendLocalPlayer> MenuState::local_players() const {
  std::vector<FrontendLocalPlayer> players;
  if(signed_in_.empty())return players;
  players.push_back({0,signed_in_,appearance_for_profile(signed_in_),lobby_team_,local_inputs_[0]});
  for(unsigned i=1;i<local_profiles_.size();++i)if(!local_profiles_[i].empty())
    players.push_back({i,local_profiles_[i],appearance_for_profile(local_profiles_[i]),0,local_inputs_[i]});
  return players;
}
std::optional<unsigned> MenuState::controller_slot(std::int32_t instance,bool join) {
  if(instance<0)return {};
  for(unsigned i=0;i<local_inputs_.size();++i)if(local_inputs_[i]==instance)return i;
  if(joining_controller_ || online_local_options_)return {};
  const bool online_join=online_couch_mutable() && (frame().screen==186 ||
      (frame().screen==14 && mode_==3) || (frame().screen==26 && frame().context==2));
  // Title sign-out clears every input slot. Rebind the selected host before
  // interpreting a new controller as an additional Split Screen player.
  if(join && local_inputs_[0]==-1 && !online_join) {local_inputs_[0]=instance;return 0;}
  if((((frame().screen==30 || frame().screen==14) && mode_==1) || online_join) && join && !signed_in_.empty()) {
    for(unsigned i=1;i<local_profiles_.size();++i)if(local_profiles_[i].empty()) {
      local_inputs_[i]=instance;joining_controller_=i;host_profile_=profile_;
      joining_online_=online_join;
      // A roster change cancels the existing countdown through its real owner.
      if(session_countdown_ && *session_countdown_>=0)back();
      open(10);return i;
    }
    return {};
  }
  return {};
}
void MenuState::finish_local_signin() {
  if(!joining_controller_)return;
  const auto controller=*joining_controller_;const bool online=joining_online_;
  local_profiles_[controller]=profile_name();
  joining_controller_.reset();profile_=host_profile_;++revision;
  joining_online_=false;
  if(online){++couch_revision_;open_online_local_player(controller);}
}
void MenuState::leave_local_controller(unsigned controller) {
  if(controller==0 || controller>=4 || joining_controller_)return;
  if(mode_==3 && !online_couch_mutable())return;
  if(session_countdown_ && *session_countdown_>=0)back();
  local_profiles_[controller].clear();local_inputs_[controller]=-1;++revision;
  if(local_discord_logins_[controller])local_discord_logins_[controller]->cancel();
  local_discord_logins_[controller].reset();local_discord_subjects_[controller].clear();++couch_revision_;
}
FrontendLaunch MenuState::selected_multiplayer_configuration() const {
  if(mode_!=1 && mode_!=2)throw FormatError("Native multiplayer configuration requires its selected local mode");
  UiVariant variant{variant_name_,engine_,variant_values_};
  const auto rules=slayer_rules(variant,true);
  if(rules.custom==CustomSlayer::none && (map_==2 || map_==8))throw FormatError("Midship and Waterworks start with energy weapons; their battery combat is unfinished. Choose an SMG-start map.");
  FrontendLaunch result;result.map=std::string(multiplayer_files.at(map_))+".map";
  result.multiplayer=true;
  const auto players=local_players();result.local_players=unsigned(players.size());
  if(mode_==1 && rules.custom==CustomSlayer::infection && players.size()<2)throw FormatError("Infection requires at least two local players or a System Link/ONLINE match.");
  for(const auto& player:players) {
    result.selected_profiles.push_back(native_selected_player_configuration(
        player.name,std::span<const std::uint8_t,9>(
            reinterpret_cast<const std::uint8_t*>(&player.appearance),9),player.team));
    native_player_configuration_name(result.selected_profiles.back());
    result.input_instances.push_back(player.input_instance);
    if(rules.team_mode) {
      const auto team=source_local_profile_team(player.team);
      if(team<0 || team>7)throw FormatError("The native Slayer session requires a selected playing team 0..7.");
      result.selected_teams.push_back(team);
    }
  }
  result.host_port=mode_==2?2002:0;result.variant=std::move(variant);
  if(multiplayer_map_validator_)multiplayer_map_validator_(result);
  else if(rules.primary_light_vehicle)
    throw FormatError("The selected map's Primary Light Vehicle setting cannot be validated.");
  return result;
}
bool MenuState::vista_pcr_event(std::uint32_t code) {
  const auto owner=frame().vista_pcr;
  if(!owner)return false;
  return vista_pcr_input(frame().screen==vista_menu(185),code,owner->controller,owner->input);
}
void MenuState::pane_selection_changed(unsigned previous) {
  auto &f = stack_.back();
  // These actual constructors use a null layout container:2306CF(19),
  //237758(38),23768D(178),2BB477(12),2519F7/251AA6(210).
  //22F6CA registers focus notifications;24C461->230427 deletes ordinary
  //pane children via22FBA9 and recreates them via22FC08/22F9BF/253765.
  //Screen1's constructor and container policy remain unproved.
  if (!frontend_text_widgets_recreated_on_focus(f.screen))
    return;
  const auto count = content_.screen(f.screen).panes.size();
  if (!count || std::min<std::size_t>(previous, count - 1) ==
                    std::min<std::size_t>(f.selected, count - 1))
    return;
  ++f.pane_generation;
  f.pane_created_clock = ui_clock_;
}
void MenuState::move(int delta) {
  if (binding_action_) return;
  auto &f = stack_.back();
  if (f.entries.empty())
    return;
  if(f.vista_pcr) {
    const auto count=unsigned(f.entries.size());
    const auto previous=f.selected==0xffffffffu?0:f.selected;
    select(unsigned((int(previous)+delta%int(count)+int(count))%int(count)));
    return;
  }
  const auto previous = f.selected;
  if (appearance_window(f.screen)) {
    const auto count = unsigned(f.entries.size());
    const auto visible = appearance_window_size(content_, f);
    // 24C1C5 advances focus within the existing child widgets. It replaces
    // their datums only after focus crosses an edge, including wraparound.
    const auto steps = delta % int(count);
    for (int i = 0; i != steps; i += steps > 0 ? 1 : -1) {
      f.selected = (f.selected + (steps > 0 ? 1 : count - 1)) % count;
      if (f.selected < f.first_visible)
        f.first_visible = f.selected;
      else if (f.selected >= f.first_visible + visible)
        f.first_visible = f.selected - visible + 1;
    }
  } else {
    f.selected = unsigned((int(f.selected) + delta % int(f.entries.size()) +
                           int(f.entries.size())) %
                          int(f.entries.size()));
    if (is_vista_settings(f.screen, 253))
      while (f.entries[f.selected].value == -2)
        f.selected = unsigned((int(f.selected) + (delta < 0 ? -1 : 1) +
                               int(f.entries.size())) % int(f.entries.size()));
  }
  pane_selection_changed(previous);
  preview_appearance_selection();
  ++revision;
}
void MenuState::mouse_wheel(int vertical) {
  // PC607B3E/607DDC dispatch wheel actions28/29 as events10/11.
  // The source list's614CA9 table sends those through614537, the same
  // single-row (including wrap) navigation as events1/3. Raw device events9
  // remain separate for binding capture in60780B/636277.
  if (vertical && frame().screen != 167)
    move(vertical > 0 ? -1 : 1);
}
void MenuState::select(unsigned index) {
  if (binding_action_) return;
  auto &f = stack_.back();
  if (is_vista_settings(f.screen, 253) && index < f.entries.size() &&
      f.entries[index].value == -2) return;
  if (f.screen == 167 && !keyboard_key_enabled(index))
    return;
  if (index < f.entries.size() && f.selected != index) {
    if(f.vista_pcr) {
      if(!f.vista_pcr->select_row)throw FormatError("Vista PCR focus requires its actual selection owner");
      f.vista_pcr->select_row(index);
    }
    const auto previous = f.selected;
    if (appearance_window(f.screen)) {
      const auto visible = appearance_window_size(content_, f);
      if (index < f.first_visible || index >= f.first_visible + visible)
        f.first_visible = index >= visible ? index - visible + 1 : 0;
    }
    f.selected = index;
    pane_selection_changed(previous);
    preview_appearance_selection();
    ++revision;
  }
}
void MenuState::unavailable(const std::string &description) {
  open(content_.gameplay_overlay?vista_menu(8):8);
  stack_.back().title = "UNAVAILABLE";
  stack_.back().description = description;
  if(content_.gameplay_overlay)stack_.back().entries={{"OK",-104,0}};
}
void MenuState::dialog(unsigned id, int action, bool single) {
  const auto &d = content_.dialogs.at(id);
  open(d.screen);
  auto &f = stack_.back();
  f.title = d.title;
  f.description = d.message;
  f.entries = {{d.first, action, 0}};
  if (!single)
    f.entries.push_back({d.second, -5, 0});
  f.selected = single ? 0 : d.selected;
}
void MenuState::accept() {
  if(content_.gameplay_overlay && !frame().entries.empty() &&
      frame().entries.at(frame().selected).destination<=-100) {
    const auto action=frame().entries.at(frame().selected).destination;
    if(action==-104) {back();return;}
    if(action==-103 || action==-102) {
      // Original23217B/2321FC confirm restart/quit before dispatch.
      const auto &d=content_.dialogs.at(action==-102?0xaa:0xa8);
      open(vista_menu(7));
      auto &f=stack_.back();f.title=d.title;f.description=d.message;
      if(action==-103) {
        f.title=content_.vista->localized(content_.screen(vista_menu(18)),0x040002ed);
        f.description="Quit the current game? Unsaved progress will be lost.";
      }
      f.entries={{action==-102?d.first:f.title,action==-102?-112:-113,0},{d.second,-104,0}};
      f.selected=1;++revision;return;
    }
    const auto requested=action==-100?GamePauseAction::Resume:action==-101?GamePauseAction::Revert:
        action==-112?GamePauseAction::Restart:GamePauseAction::Quit;
    const auto error=game_pause_action_?game_pause_action_(requested):"No live game owns this action.";
    if(!error.empty())unavailable(error);
    return;
  }
  if(frame().vista_leave_task)return; //Actual task cancel callback is null.
  if(frame().vista_pcr)return; // Source Done is codes1/13, not native Accept.
  update_online_login();
  update_online_directory();
  update_online_matchmaking();
  if(accept_online_couch())return;
  if (accept_online_clans()) return;
  if(accept_online_friends())return;
  if(accept_online_party())return;
  if (is_vista_menu(frame().screen)) { if (!binding_action_) binding_slot_=0; accept_pc(); return; }
  if (frame().screen == 14 && lobby_team_edit_.valid && lobby_team_event(5, 0))
    return;
  const auto f = frame();
  if(f.screen==14 && f.selected<2 && lobby_session_action_ &&
      lobby_session_action_(f.selected?FrontendLobbySessionAction::Setup:FrontendLobbySessionAction::Start))return;
  if (f.screen == 210) return;
  if (f.screen == 219 && f.context == 0) {
    if (f.selected < matchmaking_.playlists.size()) {
      selected_playlist_ = matchmaking_.playlists[f.selected].id; open(237);
    } else refresh_online_matchmaking();
    return;
  }
  if (f.screen == 237 && f.context == 0) {
    if(f.selected==1) {match_voice_=!match_voice_;stack_.back().entries.clear();populate_online_matchmaking(stack_.back());++revision;return;}
    const auto selected = std::find_if(matchmaking_.playlists.begin(), matchmaking_.playlists.end(), [&](const auto &p) { return p.id == selected_playlist_; });
    if (selected != matchmaking_.playlists.end() && selected->eligible) open(210);
    return;
  }
  if (f.screen == 219 && f.context == online_server_browser_context) {
    if (f.selected < online_servers_.servers.size()) {
      online_detail_id_ = online_servers_.servers[f.selected].id;
      open(237, online_server_browser_context);
    } else refresh_online_directory();
    return;
  }
  if (f.screen == 237) { if (f.entries.size() == 1) back(); return; }
  if (f.screen == 20) {
    if (online_identity()) {
      if (f.selected == 0) open(186);
      else if (f.selected == 1) {
        keyboard_text = profile_name();
        open(167, keyboard_text.empty() ? 0 : 1);
      }
      else {
        if (online_matchmaking_) online_matchmaking_->sign_out();
        matchmaking_active_ = quickmatch_pending_ = false;
        if (online_account_) online_account_->sign_out();
        if (online_voice_) online_voice_->leave();
        party_voice_wanted_=false;
        if(discord_social_)discord_social_->disconnect();
        discord_login_->cancel();
        clear_online_couch();
        if (online_directory_) online_directory_->cancel();
        online_directory_active_ = false;
        online_servers_ = {};
        online_detail_id_.clear();
        while (stack_.size() > 1 && frame().screen != 6) stack_.pop_back();
        open(20);
      }
    } else if (discord_login_) {
      if (discord_login_->snapshot().identity && online_account_) { online_account_->connect(profile_name()); awaiting_discord_login_ = true; update_online_login(); return; }
      const auto status = discord_login_->snapshot().status;
      if (status != DiscordLoginStatus::Waiting && status != DiscordLoginStatus::Verifying) {
        discord_login_->start();
        awaiting_discord_login_ = true;
        update_online_login();
      }
    } else unavailable("Discord sign-in is unavailable in this preview. Start the game normally to sign in.");
    return;
  }
  if (f.screen == 186 && f.selected == 3) { open(20); return; }
  if (f.screen == 9) {
    open(10);
    return;
  }
  if (f.screen == 167) {
    const auto key = f.selected;
    if (!keyboard_key_enabled(key))
      return;
    if (key < 36)
      text(f.entries.at(key).label);
    else if (key == 36)
      erase_text();
    else if (key == 37)
      text(" ");
    else if (key == 38 || key == 39)
      cursor(key == 38 ? -1 : 1);
    else if (key == 40)
      commit_text();
    else if (key >= 41 && key <= 44) {
      keyboard_flags_ ^= 1u << (key - 41);
      if (key == 41)
        keyboard_flags_ &= ~(2u | 8u);
      else if (key == 42)
        keyboard_flags_ &= ~(1u | 4u | 8u);
      else if (key == 43)
        keyboard_flags_ &= ~(2u | 8u);
      else
        keyboard_flags_ &= ~7u;
      keyboard_labels();
      ++revision;
    }
    return;
  }
  if (f.entries.empty()) {
    if (f.screen == 219 || f.screen == 237 || f.screen == 20 ||
        f.screen == 210 || f.screen == 182 || f.screen == 204 ||
        f.screen == 231)
      unavailable(
          "Network game services are not implemented by the native runtime.");
    return;
  }
  const auto entry = f.entries.at(f.selected);
  if (f.screen == 6) {
    if (f.selected == 0) {
      mode_ = 0;
      level_ = 0;
    }
    if (f.selected == 1)
      mode_ = 3;
    if (f.selected == 2)
      mode_ = 1;
    if (f.selected == 3)
      mode_ = 2;
  }
  if (f.screen == 30 && f.context == 2)
    { mode_ = 4; lobby_protocol_ = 0; }
  if (f.screen == 23) {
    const auto lobby = lobby_view();
    if (!lobby) return;
    const auto rows = frontend_lobby_setup_rows(lobby->input.session_enum);
    if (f.selected >= rows.size()) return;
    const auto row = rows[f.selected];
    if (row.action == FrontendLobbySetupAction::AlternateSession) {
      unavailable("This network session mode is not implemented by the native runtime.");
      return;
    }
    // Setup and its children occupy original channel3/window4. This channel
    // replaces its current screen; closing a child reveals retained lobby14.
    const auto previous_setup = row.destination == 206
        ? std::make_shared<const MenuFrame>(f) : nullptr;
    stack_.pop_back();
    if (row.action == FrontendLobbySetupAction::SwitchCoop) {
      lobby_protocol_ = lobby->protocol;
      mode_ = 4;
      // Original199A03(0)->1910D9 clamps an absent campaign save to map105.
      // Native profiles currently have no campaign progress bank.
      for (unsigned i = 0; i < campaign_files.size(); ++i)
        for (const auto &map : content_.campaign_maps)
          if (map.map_id == 105 && map.scenario_path.ends_with(campaign_files[i])) level_ = i;
      stack_.back() = build(14, 0);
      ++revision;
    } else if (row.action == FrontendLobbySetupAction::SwitchCustom) {
      mode_ = lobby->protocol == 1 ? 2 : 1;
      stack_.back() = build(14, 0);
      ++revision;
    } else {
      if (row.semantic == 1) category_ = 0;
      open(unsigned(row.destination), row.destination == 11 || row.destination == 12 ? 1 : 0);
      // 234F8C sums outgoing23 and incoming206's equal 250ms tracks.
      // The shared channel's backdrop stays at full strength on replacement.
      stack_.back().retained_lobby_backdrop = row.destination == 206;
      stack_.back().previous_lobby_setup = previous_setup;
    }
    return;
  }
  if (f.screen == 47)
    appearance_part_ = unsigned(entry.value);
  if (f.screen == 60 || f.screen == 207)
    engine_ = unsigned(entry.value);
  if (entry.destination >= 0) {
    if (entry.destination == 167) {
      keyboard_purpose_ = entry.value < 0 ? 0u : unsigned(entry.value);
      keyboard_text = keyboard_purpose_ == 1 || keyboard_purpose_ == 5
                          ? profile_name()
                      : keyboard_purpose_ == 2 || keyboard_purpose_ == 6
                          ? variant_name_
                          : "";
      if (keyboard_purpose_ == 0) {
        // 148C3E -> 215F40 supplies the first unused Halo%04d profile name.
        // 2381DE then selects it and places the cursor at its end on entry.
        for (unsigned suffix = 1; suffix <= 4096; ++suffix) {
          std::ostringstream candidate;
          candidate << "Halo" << std::setfill('0') << std::setw(4) << suffix;
          if (std::find(profiles_.begin(), profiles_.end(), candidate.str()) ==
              profiles_.end()) {
            keyboard_text = candidate.str();
            break;
          }
        }
      }
    }
    if (entry.destination == 178 || entry.destination == 60)
      category_ = unsigned(entry.value);
    // Parameter contexts are full 32-bit tag datums, including high-bit IDs.
    const auto context = entry.destination == 223
                             ? unsigned(entry.value)
                             : entry.value < 0 ? 0u : unsigned(entry.value);
    open(unsigned(entry.destination), context);
    return;
  }
  if (entry.destination == -3) {
    profile_ = unsigned(entry.value);
    save();
    if (f.screen == 193)
      open(194, 0);
    else {
      if(joining_controller_) {
        stack_.pop_back();finish_local_signin();save();return;
      }
      signed_in_ = profile_name();
      back();
      if (frame().screen == 9)
        open(6);
      else if (frame().screen == 30) {
      } else
        open(6);
    }
  } else if (entry.destination == -10) {
    clear_online_couch();
    signed_in_.clear();local_profiles_={};local_inputs_={-1,-1,-1,-1};
    open_local_title();
  } else if (entry.destination == -11) {
    while (stack_.size() > 1 && frame().screen != 6)
      stack_.pop_back();
    ++revision;
  } else if (entry.destination == -12) {
    if (f.context == 1) {
      // 2BAA60 rejects built-ins with the UI error sound and stays here.
      if (saved_variant_ < 0)
        return;
      dialog(0xb, -6);
      stack_.back().context = 1;
    } else if (profile_name() == signed_in_ && !signed_in_.empty())
      dialog(0x57, -5, true);
    else
      dialog(0x70, -6);
  } else if (entry.destination == -7 || entry.destination == -9) {
    if (entry.destination == -7) {
      saved_variant_ = -1;
      variant_ = unsigned(entry.value);
      variant_name_ = builtin_variant(content_,variant_).name;
      variant_values_ = builtin_variant(content_,variant_).values;
    } else {
      saved_variant_ = entry.value;
      variant_name_ = saved_variants_.at(saved_variant_).name;
      variant_values_ = saved_variants_.at(saved_variant_).values;
    }
    if (category_ == 1)
      open(194, 1);
    else {
      while (stack_.size() > 1 && frame().screen != 14)
        stack_.pop_back();
      if (frame().screen != 14)
        open(14);
    }
  } else if (entry.destination == -2) {
    if (f.screen == 11 || f.screen == 2) {
      level_ = unsigned(entry.value);
      if (f.screen == 11 && f.context != 1)
        open(12);
      else
        back();
    } else if (f.screen == 12) {
      difficulty_ = unsigned(entry.value);
      if (f.context == 1) back();
      else launch = FrontendLaunch{std::string(campaign_files.at(level_)) + ".map",
                                   true, difficulty_};
    } else if (f.screen == 1) {
      difficulty_ = unsigned(entry.value);
      back();
    } else if (f.screen == 17 || f.screen == 206) {
      map_ = unsigned(entry.value);
      back();
    } else if (f.screen == 223 && f.parameter) {
      variant_values_.set(content_.parameters.at(f.parameter).parameter, entry.value);
      if (saved_variant_ >= 0)
        saved_variants_.at(saved_variant_).values = variant_values_;
      save();
      back();
    } else if (f.screen == 34)
      unavailable(
          "ONLINE feedback is not implemented by the native runtime.");
    else if (f.screen == 239) {
      open(241, entry.value);
    } else if (f.screen == 50 || f.screen == 52 || f.screen == 49 || f.screen == 233) {
      // Accept keeps the original working settings. B/back instead runs the
      // child snapshot restoration proven by original2C87FE.
      preview_appearance_selection();
      if (!appearance_snapshots_.empty() && appearance_snapshots_.back().first == stack_.size() - 1)
        appearance_snapshots_.pop_back();
      back();
    } else {
      profile_values_[profile_name()][f.screen == 50 ? 0x100 + appearance_part_
                                                     : f.screen] = entry.value;
      save();
      back();
    }
  } else if (entry.destination == -5)
    back();
  else if (entry.destination == -6) {
    if (f.context == 1) {
      if (saved_variant_ < 0) {
        unavailable("Built-in game variants cannot be deleted.");
        return;
      }
      saved_variants_.erase(saved_variants_.begin() + saved_variant_);
      saved_variant_ = -1;
      const auto &original = builtin_variant(content_,variant_);
      variant_name_ = original.name;
      variant_values_ = original.values;
      save();
      while (stack_.size() > 1 && frame().screen != 60)
        stack_.pop_back();
      return;
    }
    profile_values_.erase(profile_name());
    profiles_.erase(profiles_.begin() + profile_);
    profile_ = 0;
    save();
    while (stack_.size() > 1 && frame().screen != 193)
      stack_.pop_back();
    stack_.back() = build(193, 0);
  } else if (entry.destination == -4) {
    if (f.screen == 14) {
      if (mode_ == 3) { open(219); return; }
      // Original250FB3 keeps Start focus independent of readiness, but its
      // activation calls25106C only for a signed-in host with status9.
      if (const auto lobby = lobby_view(); lobby &&
          (!lobby->player_appearance || !lobby->input.host || lobby->input.status != 9)) return;
      if (mode_ == 0)
        launch = FrontendLaunch{std::string(campaign_files.at(level_)) + ".map",
                                true, difficulty_};
      else if(mode_==1 || mode_==2) {
        try {
          launch=selected_multiplayer_configuration();
        } catch(const FormatError &e) {unavailable(e.what());}
      } else unavailable("This ONLINE service is not implemented by the native runtime.");
    } else if (f.screen == 218)
      unavailable("Campaign save and resume are not implemented by the native "
                  "runtime.");
    else
      unavailable(
          "This ONLINE service is not implemented by the native runtime.");
  }
  ++revision;
}
void MenuState::back() {
  if(online_couch_screen(frame())) {
    if(online_local_options_) {
      const auto login=local_discord_logins_[*online_local_options_];
      if(login && !login->authorization()){login->cancel();local_discord_logins_[*online_local_options_].reset();}
    }
    online_local_options_.reset();stack_.pop_back();++couch_revision_;++revision;return;
  }
  if(content_.gameplay_overlay && stack_.size()==1 && game_pause_action_) {
    const auto error=game_pause_action_(GamePauseAction::Resume);
    if(!error.empty())unavailable(error);
    return;
  }
  if(joining_controller_ && frame().screen==10) {
    local_inputs_[*joining_controller_]=-1;joining_controller_.reset();profile_=host_profile_;joining_online_=false;
    stack_.pop_back();++revision;return;
  }
  if(frame().vista_leave_task)return;
  if(frame().vista_pcr){vista_pcr_event(1);return;} // Actual PC component1/B.
  if (frame().screen == 210) {
    quickmatch_pending_ = false;
    if (online_matchmaking_) online_matchmaking_->cancel_queue();
  }
  if (frame().screen == 20 && awaiting_discord_login_) {
    if (discord_login_) discord_login_->cancel();
    awaiting_discord_login_ = false;
  }
  if (binding_action_) { binding_action_.reset(); binding_slot_=0; ++revision; return; }
  if (frame().screen == vista_menu(293)) { revert_pc_video(); return; }
  if (frame().screen == 14 && lobby_team_edit_.valid && lobby_team_event(5, 1))
    return;
  if(frame().screen==14 && lobby_session_action_ &&
      lobby_session_action_(FrontendLobbySessionAction::Back))return;
  if (!appearance_snapshots_.empty() && appearance_snapshots_.back().first == stack_.size() - 1) {
    appearance_edit_ = appearance_snapshots_.back().second;
    appearance_snapshots_.pop_back();
  }
  if (frame().screen == 38 || (frame().screen == 47 &&
      std::none_of(stack_.begin(), stack_.end(), [](const auto &f) { return f.screen == 38; })))
    commit_appearance_edit();
  if (frame().screen == 167 && keyboard_purpose_ == 0 &&
      stack_.size() >= 3 && stack_[stack_.size() - 2].screen == 10 &&
      stack_[stack_.size() - 3].screen == 9) {
    // 23784F disposes the picker channel before opening Profile Name.
    // 237F2A cancellation closes the keyboard; the retained title is exposed.
    // Keep this source-proven route distinct from successful name submission.
    stack_.pop_back();
    stack_.pop_back();
    ++revision;
    return;
  }
  if (frame().screen == 6) {
    dialog(0x30, -10);
    return;
  }
  if (frame().screen == 14 || frame().screen == 30) {
    dialog(frame().screen == 14 ? 0x8b : 0x37, -11);
    return;
  }
  if (frame().screen == 38 &&
      (frame().context == 10 || frame().context == 193)) {
    const auto parent = frame().context;
    stack_.pop_back();
    if (parent == 10) {
      if(joining_controller_) {
        while(stack_.size()>1 && frame().screen!=10)stack_.pop_back();
        if(frame().screen==10)stack_.pop_back();
        finish_local_signin();save();return;
      }
      signed_in_ = profile_name();
      while (stack_.size() > 1)
        stack_.pop_back();
      open(6);
    } else {
      while (stack_.size() > 1 && frame().screen != 19)
        stack_.pop_back();
      open(193);
    }
    return;
  }
  if (stack_.size() > 1) {
    stack_.pop_back();
    if (!frame().entries.empty()) {
      const auto previous = frame().selected;
      stack_.back().selected =
          std::min(frame().selected, unsigned(frame().entries.size() - 1));
      pane_selection_changed(previous);
    }
  }
  update_online_directory();
  update_online_matchmaking();
  ++revision;
}
void MenuState::auxiliary() {
  if(frame().vista_pcr)return; // Source details is component3/Y, not X.
  if(frame().screen==14 && lobby_session_action_ &&
      lobby_session_action_(FrontendLobbySessionAction::Auxiliary))return;
  if (frame().screen == 211 && frame().context == 41) { ++clan_message_page_; ++revision; return; }
  update_online_login();
  if(frame().screen==14 && mode_==3){open(216);return;}
  if (frame().screen == 219 && online_identity()) {
    if (frame().context == online_server_browser_context) refresh_online_directory();
    else refresh_online_matchmaking();
    return;
  }
  if (is_vista_settings(frame().screen, 243)) { open(vista_menu(content_.gameplay_overlay?254:253)); return; }
  if (is_vista_settings(frame().screen, 253) && !frame().entries.empty()) {
    binding_slot_ = 1;
    accept_pc();
    return;
  }
  if (frame().screen == 49 && appearance_edit_) {
    // Original2CB1D4: X toggles bit0 only on foreground mode0.
    appearance_edit_->flags ^= 1;
    ++revision;
  } else if (frame().screen == 23) {
    const auto lobby = lobby_view();
    if (lobby && lobby->input.session_enum != 0 && lobby->input.session_enum != 1) {
      // Original2BBED0 X/param2 opens privacy on non-local-only sessions.
      stack_.pop_back();
      open(25);
    }
  } else if (frame().screen == 14)
    lobby_team_event(5, 2);
  else if (frame().screen == 193)
    open(194);
  else if (frame().screen == 38) {
    keyboard_purpose_ = 1;
    keyboard_text = profile_name();
    open(167, 1);
  } else if (frame().screen == 15 || frame().screen == 208)
    open(178);
  else if (frame().screen == 26)
    open(frame().context == 1 ? 217 : 216,frame().context==0?43:0);
  else if (frame().screen == 167)
    erase_text();
}
void MenuState::players() {
  if(frame().vista_pcr){vista_pcr_event(3);return;}
  if (frame().screen == 167)
    text(" ");
  else if (frame().screen != 9 && frame().screen != 26)
    open(26,(account_.party || !account_.party_invitations.empty())?2:0);
  else if (frame().screen == 26)
    back();
}
bool MenuState::activate_pointer(FrontendPointerTarget target) {
  // Events queued across a screen/focus change cannot activate old geometry.
  if (target.revision != revision) return false;
  using Action = FrontendPointerAction;
  if (binding_action_ && target.action != Action::Back && target.action != Action::ClearBinding) return false;
  switch (target.action) {
  case Action::Entry:
    if (target.index >= frame().entries.size() ||
        (frame().screen == 167 && !keyboard_key_enabled(target.index)) ||
        (is_vista_settings(frame().screen, 253) && frame().entries[target.index].value == -2))
      return false;
    select(target.index);
    accept();
    break;
  case Action::Accept: accept(); break;
  case Action::Back: back(); break;
  case Action::Auxiliary: auxiliary(); break;
  case Action::Players: players(); break;
  case Action::CommitText: commit_text(); break;
  case Action::ClearBinding:
    if (binding_action_) { bind_pc_input(0); break; }
    if (!is_vista_settings(frame().screen, 253) || frame().entries.empty() ||
        frame().entries[frame().selected].value < 0) return false;
    accept();
    bind_pc_input(0);
    break;
  }
  return true;
}
void MenuState::commit_text() {
  if(frame().screen==167 && keyboard_purpose_==9) {
    if(keyboard_text.empty()){unavailable("Enter a Discord username first.");return;}
    party_command({"invite",keyboard_text});return;
  }
  if (frame().screen == 167 && (keyboard_purpose_ == 4 || keyboard_purpose_ == 7 || keyboard_purpose_ == 8)) {
    if (keyboard_text.empty()) { unavailable("Enter a name or message first."); return; }
    const char *action = keyboard_purpose_ == 4 ? "create" : keyboard_purpose_ == 7 ? "invite" : "broadcast";
    clan_command({action, keyboard_text}); return;
  }
  if (frame().screen != 167)
    return;
  if (keyboard_text.empty() ||
      keyboard_text.find_first_not_of(" \t") == std::string::npos) {
    dialog(keyboard_purpose_ == 2 || keyboard_purpose_ == 6 ? 0x59 : 0x58,
           -5, true);
    return;
  }
  if (keyboard_purpose_ <= 1 || keyboard_purpose_ == 5) {
    if (std::find(profiles_.begin(), profiles_.end(), keyboard_text) !=
            profiles_.end() &&
        (keyboard_purpose_ != 1 || keyboard_text != profile_name())) {
      dialog(0x46, -5, true);
      return;
    }
    if (keyboard_purpose_ == 0 || keyboard_purpose_ == 5) {
      if (profiles_.size() >= 32) {
        unavailable("The profile list is full.");
        return;
      }
      if (keyboard_purpose_ == 5)
        profile_values_[keyboard_text] = profile_values_[profile_name()];
      profiles_.push_back(keyboard_text);
      profile_ = unsigned(profiles_.size() - 1);
    } else {
      // Original238742 ends the profile edit after changing its name. Keep
      // its accepted appearance fields with the profile before moving the
      // host preference map to that new name.
      commit_appearance_edit();
      const auto previous_name = profile_name();
      auto values = profile_values_[profile_name()];
      profile_values_.erase(profile_name());
      profiles_[profile_] = keyboard_text;
      profile_values_[profile_name()] = std::move(values);
      if (signed_in_ == previous_name) signed_in_ = profile_name();
      // 238742 ->148BFF clears the edited profile index.24A01F then asks
      // 24A150 to rebuild retained profile names while keeping their focused
      // datum slot. Rename keeps the native slot/count, so refresh its copied
      // labels without replacing the selection or visible window.
      for (auto &retained : stack_)
        if (retained.screen == 10 || retained.screen == 193)
          for (auto &entry : retained.entries)
            if (entry.destination == -3 && entry.value == int(profile_))
              entry.label = profile_name();
    }
  } else if (keyboard_purpose_ == 2 || keyboard_purpose_ == 6) {
    for (unsigned i = 0; i < saved_variants_.size(); ++i)
      if (saved_variants_[i].name == keyboard_text &&
          (keyboard_purpose_ == 6 || int(i) != saved_variant_)) {
        dialog(0x47, -5, true);
        return;
      }
    if (saved_variant_ < 0 || keyboard_purpose_ == 6) {
      if (saved_variants_.size() >= 32) {
        unavailable("The game variant list is full.");
        return;
      }
      saved_variant_ = int(saved_variants_.size());
      saved_variants_.push_back({keyboard_text, engine_, variant_values_});
    } else
      saved_variants_.at(saved_variant_).name = keyboard_text;
    variant_name_ = keyboard_text;
  } else {
    unavailable(
        "ONLINE messaging is not implemented by the native runtime.");
    return;
  }
  save();
  // Submission resumes the creation callback's parent context. Cancellation
  // instead exposes the retained original channel (see back()).
  if (stack_.size() > 1)
    stack_.pop_back();
  ++revision;
  if (keyboard_purpose_ == 5 && frame().screen == 194) {
    stack_.pop_back();
    if (frame().screen == 193)
      stack_.back() = build(193, 0);
  }
  if (keyboard_purpose_ == 0 &&
      (frame().screen == 10 || frame().screen == 193)) {
    open(38, frame().screen);
  }
  if (frame().screen == 20 && keyboard_purpose_ == 0)
    signed_in_ = profile_name();
  update_online_login();
  return;
}
void MenuState::keyboard_labels() {
  if (frame().screen != 167)
    return;
  const auto &screen = content_.screens.at(167);
  for (unsigned i = 0; i < 36; ++i) {
    unsigned id = i < 10 ? 0x463 + i : 0x46d + i - 10;
    if (keyboard_flags_ & 8)
      id = 0x4f3 + i;
    else if (keyboard_flags_ & 4)
      id = ((keyboard_flags_ & 1) ? 0x4cf : 0x4ab) + i;
    else if (keyboard_flags_ & 3)
      id = i < 10 ? (keyboard_flags_ & 1 ? 0x487 : 0x463) + i : 0x491 + i - 10;
    stack_.back().entries.at(i).label = by_index(screen, id);
  }
}
bool MenuState::keyboard_key_visible(unsigned key) const {
  // 237998 and g_44A9F8: all currently exposed profile, variant and
  // gamertag purposes use type flags 2 or 8. Their active Symbols/Accents
  // keys (43/44) are replaced by the noninteractive artwork at 45/46.
  return key < 47 && key != 43 && key != 44;
}
bool MenuState::keyboard_key_enabled(unsigned key) const {
  return key < 43;
}
bool MenuState::keyboard_key_active(unsigned key) const {
  return key >= 41 && key <= 44 && (keyboard_flags_ & (1u << (key - 41)));
}
void MenuState::navigate(int horizontal, int vertical) {
  if(frame().vista_pcr) {
    if(vertical)move(vertical);
    // Horizontal pane changes belong to the source tab owner. The explicit
    // vista_pcr_pane entry accepts its actual pane; do not turn them into rows.
    return;
  }
  if (frame().screen == 14 && horizontal &&
      lobby_team_event(horizontal < 0 ? 2 : 4, 0))
    return;
  if (frame().screen == 26 && horizontal) {
    const auto tab = unsigned((int(frame().context) + horizontal + 3) % 3);
    back();
    open(26, tab);
    return;
  }
  if (frame().screen != 167) {
    move(vertical ? vertical : horizontal);
    return;
  }
  // Retail focus grid at 0x44A9BC; wide keys occupy multiple cells.
  constexpr unsigned grid[5][11] = {
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 44},
      {10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 41},
      {20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 42},
      {30, 31, 32, 33, 34, 35, 36, 36, 36, 36, 43},
      {37, 37, 37, 37, 37, 37, 38, 38, 39, 39, 40}};
  int row = 0, column = 0;
  bool found = false;
  for (int r = 0; r < 5 && !found; ++r)
    for (int c = 0; c < 11; ++c)
      if (grid[r][c] == frame().selected) {
        row = r;
        column = c;
        found = true;
        break;
      }
  const auto old = frame().selected;
  for (int i = 0; i < 11; ++i) {
    row = (row + vertical + 5) % 5;
    column = (column + horizontal + 11) % 11;
    if (grid[row][column] != old && keyboard_key_enabled(grid[row][column])) {
      select(grid[row][column]);
      break;
    }
  }
}
void MenuState::text(const std::string &value) {
  if (frame().screen != 167)
    return;
  if (std::any_of(value.begin(), value.end(),
                  [](unsigned char c) { return c < 32 || c == 127; }))
    return;
  const auto count = [](const std::string &s) {
    unsigned n = 0;
    for (unsigned char c : s)
      if ((c & 0xc0) != 0x80)
        n += c >= 0xf0 ? 2 : 1;
    return n;
  };
  if (keyboard_selected_) {
    keyboard_text.clear();
    keyboard_cursor_ = 0;
    keyboard_selected_ = false;
  }
  if (count(keyboard_text) + count(value) > (keyboard_purpose_ == 4 ? 15 : keyboard_purpose_ == 7 || keyboard_purpose_==9 ? 32 : keyboard_purpose_ == 8 ? 128 : 16))
    return;
  keyboard_text.insert(keyboard_cursor_, value);
  keyboard_cursor_ += value.size();
  ++revision;
}
void MenuState::cursor(int direction) {
  if (frame().screen != 167)
    return;
  keyboard_selected_ = false;
  if (direction < 0 && keyboard_cursor_) {
    --keyboard_cursor_;
    while (keyboard_cursor_ &&
           (static_cast<unsigned char>(keyboard_text[keyboard_cursor_]) &
            0xc0) == 0x80)
      --keyboard_cursor_;
  } else if (direction > 0 && keyboard_cursor_ < keyboard_text.size()) {
    ++keyboard_cursor_;
    while (keyboard_cursor_ < keyboard_text.size() &&
           (static_cast<unsigned char>(keyboard_text[keyboard_cursor_]) &
            0xc0) == 0x80)
      ++keyboard_cursor_;
  }
  ++revision;
}
std::string MenuState::keyboard_display() const {
  return keyboard_text;
}
std::string MenuState::keyboard_prefix() const {
  return keyboard_text.substr(0, std::min(keyboard_cursor_, keyboard_text.size()));
}
bool MenuState::keyboard_selected() const {
  return keyboard_selected_;
}
void MenuState::erase_text() {
  if (frame().screen == 167 && keyboard_selected_) {
    keyboard_text.clear();
    keyboard_cursor_ = 0;
    keyboard_selected_ = false;
    ++revision;
    return;
  }
  if (frame().screen != 167 || !keyboard_cursor_)
    return;
  const auto end = keyboard_cursor_;
  cursor(-1);
  keyboard_text.erase(keyboard_cursor_, end - keyboard_cursor_);
  ++revision;
}
void MenuState::save() const {
  if (preferences_.empty())
    return;
  if (!preferences_.parent_path().empty())
    std::filesystem::create_directories(preferences_.parent_path());
  const auto temporary = preferences_.string() + ".tmp";
  std::ofstream file(temporary, std::ios::trunc);
  if (!file)
    throw FormatError("cannot save menu preferences");
  file << "halo2-native-menu-v1\n";
  file << "pc_display " << pc_display_.internal_width << ' ' << pc_display_.internal_height
       << ' ' << pc_display_.anti_aliasing << ' ' << pc_display_.brightness
       << ' ' << pc_display_.gamma << ' ' << pc_display_.detail << ' ' << pc_display_.safe_area
       << ' ' << pc_display_.volume << ' ' << pc_display_.subtitles << '\n';
  file << "pc_speakers " << pc_display_.speakers << '\n';
  file << "pc_fov " << pc_display_.field_of_view << ' ' << pc_display_.vehicle_field_of_view << '\n';
  file << "pc_crt_filter " << pc_display_.crt_filter << '\n';
  file << "pc_two_player_split " << unsigned(pc_display_.two_player_split) << '\n';
  if (saved_pc_video_)
    file << "pc_video " << (*saved_pc_video_)[0] << ' ' << (*saved_pc_video_)[1]
         << ' ' << (*saved_pc_video_)[2] << '\n';
  file << "active " << std::quoted(profile_name()) << '\n'
       << "variant_name " << std::quoted(variant_name_) << ' ' << engine_
       << '\n';
  for (const auto &name : profiles_)
    file << "profile " << std::quoted(name) << '\n';
  for (const auto &[name, values] : profile_values_)
    for (const auto &[key, value] : values)
      file << "option " << std::quoted(name) << ' ' << key << ' ' << value
           << '\n';
  for (const auto &[key, value] : variant_values_)
    file << "variant " << key << ' ' << value << '\n';
  for (const auto &v : saved_variants_) {
    file << "saved_variant " << std::quoted(v.name) << ' ' << v.engine << '\n';
    for (const auto &[key, value] : v.values)
      file << "saved_value " << std::quoted(v.name) << ' ' << key << ' '
           << value << '\n';
  }
  file.close();
  if (!file)
    throw FormatError("menu preference write failed");
  std::filesystem::rename(temporary, preferences_);
}
const UiLobbyMap *MenuState::focused_lobby_map(const MenuFrame &frame) const {
  if (frame.screen != 206 || frame.selected >= frame.entries.size()) return nullptr;
  const auto list = frontend_lobby_maps(content_.multiplayer_maps, content_.campaign_maps,
                                       frontend_lobby_new_profile_map_progress);
  const auto preview = frontend_lobby_map_preview(content_.multiplayer_maps, list, frame.selected);
  return preview ? preview->map : nullptr;
}
std::optional<FrontendLobbyView> MenuState::lobby_view() const {
  if(mode_==3) {
    FrontendLobbyView view;view.protocol=2;view.input.session_enum=6;
    view.input.host=account_.party && account_.party->leader;
    view.input.start_permission=view.input.host;
    view.input.max_players=16;view.input.player_count=int(online_party_members().size());
    view.party_names=std::vector<std::string>{};
    for(const auto &m:online_party_members()) {
      view.party_names->push_back(m.name);
      const bool talking=voice_party_ && voice_.joined && ((m.self && voice_.transmitting) ||
          std::find(voice_.talking.begin(),voice_.talking.end(),m.handle)!=voice_.talking.end());
      const bool muted=std::find(voice_.muted.begin(),voice_.muted.end(),m.handle)!=voice_.muted.end();
      // Remote idle device state is not supplied by this relay. Only actual
      // reception, the local mute choice and our own device state are known.
      view.party_speakers.push_back(!m.self && !talking && !muted?3:
          voice_party_ && voice_.joined && !muted && !(m.self && voice_.microphone_muted)?!talking:2);
    }
    return view;
  }
  // These are the native local-host routes. Live/reference session0 and1
  // establish the Split Screen/System Link mapping; no remote party is
  // manufactured for an unsupported online route or a direct screen preview.
  if (mode_ != 1 && mode_ != 2 && mode_ != 4) return {};
  FrontendLobbyView view;
  view.protocol = mode_ == 2 ? 1 : mode_ == 4 ? lobby_protocol_ : 0;
  view.engine = engine_;
  view.difficulty = difficulty_;
  const bool campaign = mode_ == 4;
  const auto &maps = campaign ? content_.campaign_maps : content_.multiplayer_maps;
  const std::string file = campaign ? campaign_files.at(level_) : multiplayer_files.at(map_);
  for (const auto &map : maps) {
    const auto slash = map.scenario_path.find_last_of("\\/");
    if (map.scenario_path.substr(slash == std::string::npos ? 0 : slash + 1) == file) {
      view.map = &map;
      break;
    }
  }
  view.input.session_enum = frontend_lobby_session_enum({view.protocol, 1,
      campaign && view.map ? view.map->campaign_id : -1,
      frontend_lobby_variant_type(engine_), view.map != nullptr});
  view.input.host = true;
  view.input.party_privacy = profile_option(25, 0);
  const auto settings=source_game_variant({variant_name_,engine_,variant_values_});
  view.input.team_mode = !campaign && source_game_variant_get(settings,91) != 0;
  // Original flag5/parameter13 controls NONE in the cycle. The retail assets
  // have no authored rule selector for this parameter; do not add one.
  view.allow_no_team = !campaign && source_game_variant_get(settings,13) != 0;
  view.player_team = lobby_team_;
  view.team_edit = lobby_team_edit_;
  view.input.local_team_selection = lobby_team_edit_.valid;
  view.team_count = frontend_lobby_team_count(view.map ?
      std::span<const std::uint8_t>(view.map->team_counts) : std::span<const std::uint8_t>{},
      frontend_lobby_variant_type(engine_).value_or(-1), view.map != nullptr);
  view.input.team_count = int(view.team_count);
  view.input.player_count = int(local_players().size());
  if (!signed_in_.empty()) view.player_appearance = appearance_for_profile(signed_in_);
  view.input.max_players = campaign ? 2 : 16;
  // Original19A2CE's established-local-host branch, frozen separately from
  // online/loading states. Current native roster contains one ready local
  // player;19A2CE excludes NONE from the distinct-team count. Co-op requires
  // two present and ready players. Preview edits do not replace membership.
  if (view.map && view.input.player_count > 0)
    view.input.status = frontend_lobby_ready_status({view.input.session_enum,
        view.input.player_count, view.input.player_count, view.input.team_mode,
        lobby_team_ >= 0 && lobby_team_ < 16 ? 1 : 0, int(view.team_count)});
  // 63F00 checks local host/session ownership, independently of19A2CE.
  view.input.start_permission = view.input.host;
  // Actual native session countdown, rather than an immediate Start launch.
  // This is the existing Xbox lobby presentation; PC lobby14 remains separate.
  if(session_countdown_ && *session_countdown_>=0) {
    view.input.countdown=true;
    view.input.start_permission=false;
    // Xbox24FFC9 binds status10 to the live countdown (E413). Status11
    // selects the unrelated matchmaking "You need to be in a party" text.
    view.input.status=10;
  }
  if (!campaign) {
    std::map<unsigned,int> queries;
    for (unsigned id=0;id<112;++id)queries.emplace(id,source_game_variant_get(settings,int(id)));
    view.quick_options = frontend_lobby_quick_options(content_, engine_, queries,
        content_.localized(content_.screens.at(14), 0x1a001820));
  }
  return view;
}
bool MenuState::lobby_team_event(int type, int param) {
  const auto lobby = lobby_view();
  if (!lobby) return false;
  FrontendLobbyTeamEvent input;
  input.type = type;
  input.param = param;
  input.signed_in = !signed_in_.empty();
  input.available = frontend_lobby_team_available(lobby->input.session_enum,
                                                  lobby->input.team_mode ? 1 : 0);
  if (input.signed_in) input.member = std::int8_t(lobby_team_);
  input.team_count = lobby->team_count;
  input.allow_none = lobby->allow_no_team;
  const auto result = frontend_lobby_team_event(lobby_team_edit_, lobby_team_, input);
  // This local session has no external original profile-update transport.
  // Its committed membership is the native session input exposed above.
  if (result.consumed) ++revision;
  return result.consumed;
}
std::string MenuState::expand(const std::string &source) const {
  std::string result;
  for (std::size_t i = 0; i < source.size();) {
    if (i + 2 < source.size() &&
        static_cast<unsigned char>(source[i]) == 0xee &&
        (static_cast<unsigned char>(source[i + 1]) & 0xc0) == 0x80 &&
        (static_cast<unsigned char>(source[i + 2]) & 0xc0) == 0x80) {
      const unsigned code =
          ((static_cast<unsigned char>(source[i]) & 15) << 12) |
          ((static_cast<unsigned char>(source[i + 1]) & 63) << 6) |
          (static_cast<unsigned char>(source[i + 2]) & 63);
      if (code >= 0xe400 && code <= 0xe45f) {
        switch (code) {
        case 0xe40f:
        case 0xe420:
        case 0xe431:
        case 0xe416:
          result += profile_name();
          break;
        case 0xe408:
          result += mode_ == 0 || mode_ == 4 ? campaign_names.at(level_)
                               : multiplayer_names.at(map_);
          break;
        case 0xe409:
        case 0xe40a:
          result += variant_name_;
          break;
        case 0xe41f:
          if (const auto lobby = lobby_view())
            result += content_.localized(content_.screens.at(14),
                frontend_lobby_protocol_string(lobby->input.session_enum));
          break;
        case 0xe425:
          if (const auto lobby = lobby_view()) {
            const auto id = frontend_lobby_player_count_string(lobby->input.player_count);
            if (id) result += content_.localized(content_.screens.at(14), *id);
          }
          break;
        case 0xe426:
        case 0xe427:
          if (const auto lobby = lobby_view(); lobby && lobby->map)
            result += code == 0xe426 ? lobby->map->name : lobby->map->description;
          break;
        case 0xe428:
          result += content_.localized(content_.screens.at(14),
              frontend_lobby_difficulty_string(std::int16_t(difficulty_)));
          break;
        case 0xe413:
          result += session_countdown_?std::to_string(*session_countdown_):"0";
          break;
        case 0xe424:
          result += "Player";
          break;
        case 0xe43a:
          result += "Rumble Pit";
          break;
        default:
          break;
        }
        i += 3;
        continue;
      }
    }
    result += source[i++];
  }
  return frontend_online_text(result);
}
} // namespace halo2
