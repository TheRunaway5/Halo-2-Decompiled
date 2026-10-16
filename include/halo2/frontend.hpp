#pragma once
#include "halo2/source_player_profile.hpp"
#include "halo2/cache.hpp"
#include "halo2/scene.hpp"
#include "halo2/popup_blur.hpp"
#include "halo2/frontend_appearance.hpp"
#include "halo2/frontend_ui_assets.hpp"
#include "halo2/frontend_lobby.hpp"
#include "halo2/frontend_lobby_team.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/frontend_vista.hpp"
#include "halo2/pc_settings.hpp"
#include "halo2/controller_settings.hpp"
#include "halo2/display_settings.hpp"
#include "halo2/discord_login.hpp"
#include "halo2/discord_social.hpp"
#include "halo2/online_directory.hpp"
#include "halo2/online_matchmaking.hpp"
#include "halo2/online_account.hpp"
#include "halo2/native_tu15_online_services.hpp"
#include "halo2/vista_pcr_screen.hpp"
#include "halo2/vista_pcr_appearance.hpp"
#include "halo2/vista_lobby_leave_task.hpp"
#include <chrono>
#include <map>
#include <functional>
#include <memory>
#include <unordered_map>

namespace halo2 {
inline constexpr unsigned online_server_browser_context = 1;
class DesktopSession;
class FrontendSession;
// Literal wigl sound-table indices used by original236299 / Vista61DD04.
// Slot11 has no identified sound and is intentionally unnamed.
enum class FrontendUiSound : unsigned {
  Cursor = 0, Forward = 1, Error = 2, Advance = 3, Back = 4,
  Transport = 5, KeyboardCursor = 6, KeyboardInsert = 7,
  ReceiveMessage = 8, Tab = 9, PregameCountdown = 10,
  MatchmakingAdvance = 12
};
inline constexpr unsigned frontend_ui_sound_slot_count = 13;
// Vista61DDEA/61DE1A both FLD1 before the source playback request.
inline constexpr float frontend_ui_sound_request_scale = 1.f;
struct UiRect {
  float top{}, left{}, bottom{}, right{};
};
struct UiColor {
  float alpha = 1, red = 1, green = 1, blue = 1;
};
struct UiKey {
  float alpha = 1;
  Vec3 position;
};
struct UiTrack {
  unsigned milliseconds{};
  std::vector<UiKey> keys;
};
struct UiAnimation {
  UiTrack intro, outro, ambient;
  int loop{};
};
struct UiText {
  unsigned flags{}, animation{}, delay{};
  int font = 1;
  UiColor color;
  UiRect bounds;
  std::uint32_t string{};
  int layer{}; // Signed authored text+40, or button+52, copied to widget+6A.
};
struct UiBitmap {
  unsigned flags{}, animation{}, delay{}, blend{};
  int frame{}, layer{};
  Vec2 position, scroll, scale;
  std::uint32_t datum = 0xffffffffu;
  std::uint32_t placeholder{};
  float fps{};
};
struct UiGrid {
  std::uint32_t skin = 0xffffffffu;
  Vec2 position, step;
  unsigned order{}, count{}, rows{}, columns{};
};
// Original type9 widget, 2B1111/2B11C9. The authored shader composes
// selected emblem pages; the dummy bitmap supplies the quad descriptor.
struct UiEmblem {
  unsigned flags{}, animation{}, delay{};
  int layer{}, page{};
  std::uint32_t bitmap = 0xffffffffu, shader = 0xffffffffu;
  UiRect bounds;
};
struct UiButton {
  UiText text;
  std::uint32_t bitmap = 0xffffffffu;
  Vec2 offset;
  // PC63FFF8 subtracts signed button+36 from hit-test top only. Draw
  // bounds remain the authored+1C rectangle (PC63F85B ->6116D2).
  std::int16_t vista_hit_top_offset{};
  // Full button+38 dword; PC63F85B copies bits0/1 to widget+F4.
  // Keep the remaining source bits without assigning unproven behavior.
  std::uint32_t vista_flags{};
};
struct UiList {
  unsigned flags{}, skin{}, visible{}, animation{}, delay{};
  Vec2 position;
  std::vector<std::uint32_t> labels;
};
struct UiScene {
  unsigned flags{}, animation{}, delay{};
  Vec3 scale, camera;
  float field_of_view{};
  UiRect viewport;
  std::vector<std::string> objects, lights;
};
struct UiPane {
  unsigned animation{};
  std::vector<UiText> texts;
  std::vector<UiBitmap> bitmaps;
  std::vector<UiButton> buttons;
  std::vector<UiList> lists;
  std::vector<UiScene> scenes;
  std::vector<UiGrid> grids;
  std::vector<UiEmblem> emblems;
};
using UiStrings = std::map<std::uint32_t, std::string>;
struct UiScreen {
  std::string name;
  unsigned id{}, flags{}, button_key{}, shape{};
  std::uint32_t header{};
  UiColor color;
  UiStrings strings;
  std::vector<std::uint32_t> bitmap_resources;
  std::vector<UiPane> panes;
};
struct UiSkin {
  std::string name;
  std::vector<UiText> texts;
  std::vector<UiBitmap> bitmaps;
  std::vector<UiTrack> animations;
  std::vector<UiGrid> grids;
  std::uint32_t arrows = 0xffffffffu;
  Vec2 arrow_up, arrow_down;
  std::vector<UiEmblem> emblems;
};
struct UiSprite {
  unsigned image{};
  float left{}, right = 1, top{}, bottom = 1;
  Vec2 registration;
};
struct UiImage {
  std::string name;
  std::vector<Texture> images;
  std::vector<std::vector<UiSprite>> sequences;
};
struct UiChoice {
  std::string label;
  int value{};
};
struct UiParameter {
  unsigned parameter{};
  int default_value{};
  std::string title, header, description;
  std::vector<UiChoice> choices;
};
struct UiVariant {
  std::string name;
  unsigned engine{};
  GameVariantSettings values;
};
struct UiDialog {
  unsigned screen = 7, selected{};
  std::string title, message, first, second;
};
struct UiLobbyMap {
  std::int32_t campaign_id = -1, map_id = -1;
  std::uint32_t bitmap = 0xffffffffu;
  std::string name, description, scenario_path;
  std::int32_t sort_order{};
  std::uint8_t flags{};
  std::array<std::uint8_t, 16> team_counts{};
};
struct FrontendTypewriterSettings {
  std::int16_t interval_ms{};
  std::uint16_t cursor{};
  float scale{};
};
struct VistaLobbyMetadata;
class VistaWidgetCacheBank;
struct FrontendContent {
  bool gameplay_overlay{};
  // PC resource datums, skins, fonts and animations keep their own namespace.
  std::shared_ptr<const FrontendContent> vista;
  // Exact source-local PC map/rule/locale records for future dynamic owners.
  // This does not choose the 3D backdrop or enable a different menu route.
  std::shared_ptr<const VistaLobbyMetadata> vista_lobby_metadata;
  std::filesystem::path source_fonts;
  const UiScreen &screen(unsigned id) const {
    return is_vista_menu(id) ? (vista ? *vista : *this).screens.at(id & 0xffffu)
                             : screens.at(id);
  }
  std::vector<Vec3> appearance_palette;
  std::map<unsigned, UiScreen> screens;
  std::vector<UiSkin> skins;
  std::map<std::uint32_t, UiSkin> skin_groups;
  std::vector<UiAnimation> animations;
  std::map<std::uint32_t, UiImage> images;
  std::map<std::uint32_t, Material> emblem_shaders;
  std::map<unsigned, std::vector<std::uint32_t>> categories;
  std::map<std::uint32_t, UiParameter> parameters;
  std::vector<UiVariant> variants;
  std::vector<UiVariant> bundled_variants;
  std::vector<UiLobbyMap> campaign_maps, multiplayer_maps;
  std::map<unsigned, UiDialog> dialogs;
  UiStrings global_strings;
  // Original19D220 follows matg+16C -> mulg universal+1C -> unic.
  UiStrings variant_strings;
  std::vector<std::string> button_keys;
  std::array<int, 4> header_fonts{};
  std::array<UiRect, 8> header_bounds{};
  // Actual wigl+48/+4A/+4C, populated by Cache::load_frontend.
  FrontendTypewriterSettings typewriter;
  UiColor color;
  UiColor overlay;
  std::optional<Vec3> scene_clear;
  std::uint32_t music = 0xffffffffu;
  unsigned music_silence_ms{};
  std::uint32_t keyboard_selection = 0xffffffffu,
                keyboard_caret = 0xffffffffu;
  std::array<std::uint32_t, frontend_ui_sound_slot_count> sounds{};
  std::string localized(const UiScreen &, std::uint32_t) const;
  // Actual PC matg+16C ->mulg universal+10 and the mainmenu interface
  // matg+110 ->wgtz+4 ->wigl+158 appearance owners. Xbox assets differ.
  std::vector<Vec3> vista_pcr_team_palette;
  // PC45C341: referenced mulg universal+0C team_names unic, ordinal0..8.
  std::array<std::string,9> vista_team_names;
  std::vector<VistaPcrRankRange> vista_pcr_rank_ranges;
  bool vista_pcr_rank_owner_present{};
  // Immutable original PC index/table/block/frame bytes; source32 placement is
  // explicitly chosen by the loaded widget owner, separately from host memory.
  std::shared_ptr<const VistaWidgetCacheBank> vista_widget_cache;
};
struct FontGlyph {
  unsigned x{}, y{}, width{}, height{};
  int advance{}, bearing_x{}, bearing_y{};
};
struct FontGlyphSample {
  unsigned x{}, y{}, width{}, height{};
};
struct RetailFont {
  Texture atlas;
  int ascent{}, descent{}, leading{};
  std::map<unsigned, FontGlyph> glyphs;
  std::map<std::pair<unsigned, unsigned>, int> kerning;
  // Exact source lookup records distinguish authored rows from aliases. These
  // fields never change the original glyph metrics or font-table selection.
  std::vector<std::uint32_t> glyph_record_indices;
  std::uint64_t file_fingerprint{}; // FNV-1a64 identity, not a security digest.
  std::map<unsigned, FontGlyphSample> sampled_glyphs;
};
RetailFont load_retail_font(const std::filesystem::path &);
Texture prepare_ui_bitmap(const Bytes &, unsigned width, unsigned height,
                          unsigned format, unsigned flags);
std::vector<std::uint16_t> decode_font_pixels(std::span<const std::byte>,
                                              std::size_t limit);

// UI triangle order is preserved for alpha compositing. Position.z carries
// alpha.
struct UiDraw {
  std::vector<Vertex> vertices;
  std::vector<UiBackdrop> backdrops;
  // Source widgets whose real rendering owner has not yet been implemented.
  // Retaining these boundaries makes an incomplete pass observable to callers.
  std::vector<std::string> rendering_gaps;
  bool scene_color_filter{};
  // Native layouts may use the surface outside the centered authored canvas.
  // Coordinates still use its uniform scale, including for mouse input.
  bool full_surface_bounds{};
  struct Scene {
    const UiScene *definition{};
    std::size_t before_vertex{};
    Vec3 animation_offset;
    std::optional<FrontendAppearance> appearance;
    bool vista_viewport{};
  };
  std::vector<Scene> scenes;
};
struct MenuEntry {
  std::string label;
  int destination = -1, value{};
};
// Actual40D0D7 ->620652/640F96 owner output for one retained PC player.
// Full sixteen appearance bytes survive until the PC emblem pass consumes
// them; the native renderer must not generate missing identity/color data.
struct VistaPcrPlayerPresentation {
  std::array<std::byte,16> appearance{};
  std::uint16_t team=0xffff,rank_range=0xffff;
  std::array<std::uint32_t,3> primary_color{},name_color{};
  bool primary_visible{},secondary_visible{},rank_visible{},emblem_visible{};
  unsigned primary_frame{},rank_frame{};
};
struct VistaPcrScreenOwner {
  // Native session provenance selects the ONLINE Vista asset bank; it does
  // not modify the original retained PCR result bytes or widget records.
  bool online{};
  std::shared_ptr<const VistaPcrResults> results;
  // Only team_color/selected_row/localized/format are external here. Native
  // child lookup/text/color/visibility callbacks are installed by the painter.
  VistaPcrRowServices rows;
  // Mandatory reached40D0D7 owner, including actual local-player services.
  std::function<VistaPcrPlayerPresentation(unsigned)> appearance;
  // Actual40C5DA genuine voice owner. An absent PC16 callback stays an
  // explicit rendering gap; PC185 independently hides bitmap0 in40C5C4.
  std::function<VistaPcrVoicePresentation(unsigned)> voice;
  // Native session audio lifetime/control; no original widget geometry changes.
  std::function<void(bool)> voice_update;
  std::function<void(unsigned,unsigned,unsigned)> voice_control; // action,pane,row
  VistaPcrInputServices input;
  std::function<std::uint32_t(std::uint32_t)> elapsed;
  std::function<void(unsigned)> select_row;
  VistaLoadingFloatingPoint floating_point;
  std::uint32_t controller{},creation_clock{},selected_row{};
  unsigned pane{};
};
// The native allocation retains the source-sized task tail. Only fields
// consumed by the verified Leave corridor represent original widget state;
// native rendering owns the ordinary children and animation lifetime.
struct VistaLeaveTaskScreenOwner {
  std::array<std::byte,vista_lobby_leave_task_size> storage{};
  std::uint32_t controller{},status_string=0x0d000441;
  std::uint64_t instance{};
};
struct MenuFrame {
  unsigned screen = 9, selected{}, context{};
  std::uint32_t parameter{};
  std::string title, description;
  std::vector<MenuEntry> entries;
  unsigned first_visible{};
  bool retained_lobby_backdrop{};
  // Base channel3 keeps its replaced Setup23 transiently; this is not Back
  // history. Original234EBC starts its type3 exit before incoming Map206.
  std::shared_ptr<const MenuFrame> previous_lobby_setup;
  std::shared_ptr<const VistaPcrScreenOwner> vista_pcr;
  std::shared_ptr<VistaLeaveTaskScreenOwner> vista_leave_task;
  // Native identities represent the original screen/widget allocation owner.
  // Copies retaining an outgoing channel retain these values; build() starts
  // a new lifetime. Recreated pane children have a separate generation.
  std::uint64_t instance{}, pane_generation{};
  std::uint32_t pane_created_clock{};
};
// Source constructors2306CF/237758/23768D/2BB477/2519F7+251AA6 use
// null layout containers and focus-driven230427 pane reconstruction.
// Screen1's container/lifecycle is not established by its definition flags.
constexpr bool frontend_text_widgets_recreated_on_focus(unsigned screen) {
  return screen == 19 || screen == 38 || screen == 178 ||
         screen == 12 || screen == 210 || screen == vista_menu(19) ||
         screen == vista_menu(38);
}
struct FrontendLobbyView {
  FrontendLobbyInput input;
  int protocol = -1;
  unsigned engine{}, difficulty{};
  const UiLobbyMap *map{};
  std::optional<FrontendAppearance> player_appearance;
  int player_team{};
  FrontendLobbyTeamEdit team_edit;
  unsigned team_count = 1;
  bool allow_no_team{};
  std::array<std::optional<std::string>, 5> quick_options;
  std::optional<std::vector<std::string>> party_names;
  std::vector<unsigned> party_speakers;
};
struct FrontendLocalPlayer {
  unsigned controller{};
  std::string name;
  FrontendAppearance appearance;
  int team{};
  std::int32_t input_instance=-1; // SDL instance; -1 is keyboard/mouse.
};
struct FrontendLaunch {
  std::filesystem::path map;
  bool campaign{};
  unsigned difficulty = 1;
  bool multiplayer{};
  unsigned local_players = 1;
  unsigned host_port{};
  UiVariant variant;
  // One converted profile team per actually selected local user in team play.
  std::vector<std::int8_t> selected_teams;
  std::vector<SourcePlayerConfiguration> selected_profiles;
  std::vector<std::int32_t> input_instances;
  std::shared_ptr<OnlineMatchSession> online_match;
  bool match_voice{};
  std::shared_ptr<NativeTu15OnlineServices> online_services;
};
enum class FrontendLobbySessionAction { Start, Setup, Back, Auxiliary };
// Native pointer actions share the menu's existing command handlers. Prompt
// targets come from the actual clipped text layout, including Vista scaling.
enum class FrontendPointerAction { Entry, Accept, Back, Auxiliary, Players, CommitText, ClearBinding };
enum class GamePauseAction { Resume, Revert, Restart, Quit };
struct FrontendPointerTarget {
  FrontendPointerAction action{};
  unsigned index{}, revision{};
  bool operator==(const FrontendPointerTarget &) const = default;
};
// Retail screen IDs and content define presentation; this host state owns local
// profiles/options and reports unavailable engine/network operations
// explicitly.
class MenuState {
public:
  explicit MenuState(const FrontendContent &,
                     const std::filesystem::path &preferences = {});
  const MenuFrame &frame() const { return stack_.back(); }
  // ONLINE discovery borrows PC13's presentation only. Its native219 route
  // retains directory authentication/history and cannot enter the old session
  // browser's Create/Back/Leave lifecycle.
  unsigned screen_layout(const MenuFrame &frame) const {
    return content_.vista && frame.screen == 219 &&
        frame.context == online_server_browser_context ? vista_menu(13) : frame.screen;
  }
  const std::vector<MenuFrame> &stack() const { return stack_; }
  // Explicit original UI clock input, supplied before navigation events.
  void ui_clock(std::uint32_t value);
  void open(unsigned screen, unsigned context = 0);
  void open_game_pause(bool campaign);
  void set_game_pause_action(std::function<std::string(GamePauseAction)> action) {
    game_pause_action_=std::move(action);
  }
  bool game_pause() const { return content_.gameplay_overlay; }
  void game_pause_objectives(std::string value) { game_pause_objectives_=std::move(value); }
  const std::string &game_pause_objectives() const { return game_pause_objectives_; }
  // Explicit real PCR factory boundary. Raw source16/185 only; no automatic
  // timer/completion route, synthetic report or reuse of an old lobby frame.
  void open_vista_pcr(unsigned,std::shared_ptr<const VistaPcrScreenOwner>);
  // A completed source menu-return factory allocates a fresh lobby frame from
  // retained local selections. It never resumes a pre-match Back stack.
  void open_vista_lobby();
  void open_local_title();
  // Native presentation of the completed source main-menu return. Allocate
  // the same menu as startup and publish presence before registering it.
  void open_native_mainmenu(const std::function<void()>& before_register);
  //Actual XLivePBufferGetByte output used by40A276. Missing ownership keeps
  //LIVE/NETWORK unavailable; it must not be inferred from profile sign-in.
  void set_vista_mainmenu_pbuffer_byte(std::optional<std::uint8_t> value) {
    if(vista_mainmenu_pbuffer_byte_!=value){vista_mainmenu_pbuffer_byte_=value;++revision;}
  }
  bool vista_mainmenu_row_enabled(unsigned datum) const;
  std::uint64_t open_vista_leave_task(std::uint32_t controller,
      std::shared_ptr<VistaLeaveTaskScreenOwner>);
  void set_vista_leave_task_status(std::uint64_t instance,std::uint32_t string_id);
  void close_vista_leave_task(std::uint64_t instance,std::uint32_t reason);
  unsigned vista_session_screen() const;
  FrontendLaunch selected_multiplayer_configuration() const;
  void vista_pcr_pane(unsigned);
  bool vista_pcr_event(std::uint32_t source_code);
  void move(int);
  void mouse_wheel(int vertical);
  void navigate(int horizontal, int vertical);
  void select(unsigned);
  void accept();
  void back();
  void auxiliary();
  void players();
  bool activate_pointer(FrontendPointerTarget);
  void text(const std::string &);
  void erase_text();
  void commit_text();
  void cursor(int);
  std::string keyboard_display() const;
  std::string keyboard_prefix() const;
  bool keyboard_selected() const;
  bool keyboard_key_visible(unsigned) const;
  bool keyboard_key_enabled(unsigned) const;
  bool keyboard_key_active(unsigned) const;
  void save() const;
  // Resolve selected user-owned map capabilities before handing off the lobby.
  void set_multiplayer_map_validator(std::function<void(const FrontendLaunch &)>);
  // Retained session service, independent of invocation-local render adapters.
  // True consumes the native action; false continues ordinary menu handling.
  void set_lobby_session_action(std::function<bool(FrontendLobbySessionAction)> action) {
    lobby_session_action_=std::move(action);
  }
  // Detach borrowed invocation resources without applying settings or calling
  // validators. Retained menu/configuration state is unchanged.
  void clear_runtime_adapters() noexcept;
  void set_discord_login(std::shared_ptr<DiscordLogin>);
  void set_local_discord_login_factory(std::function<std::shared_ptr<DiscordLogin>()>);
  void open_online_local_player(unsigned controller);
  bool online_couch_screen(const MenuFrame &) const;
  std::string online_couch_text(const MenuFrame &,unsigned) const;
  bool online_couch_mutable() const;
  void set_discord_social(std::shared_ptr<DiscordFriends>);
  bool online_friends_screen(const MenuFrame &) const;
  std::string online_friends_text(const MenuFrame &, unsigned) const;
  std::string online_friends_column(const MenuFrame &, unsigned, unsigned) const;
  void return_online(std::string message = {});
  void update_online_login();
  std::optional<DiscordIdentity> online_identity() const;
  void set_online_directory(std::shared_ptr<OnlineDirectory>);
  void update_online_directory();
  std::string online_directory_text(const MenuFrame &, unsigned text_index) const;
  std::string online_directory_column(const MenuFrame &, unsigned row, unsigned column) const;
  void set_online_account(std::shared_ptr<OnlineAccount>);
  void set_online_voice(std::shared_ptr<OnlineVoice>);
  void voice_push_to_talk(bool);
  void update_online_account();
  bool online_clan_screen(const MenuFrame &) const;
  std::string online_clan_text(const MenuFrame &, unsigned) const;
  std::string online_clan_column(const MenuFrame &, unsigned, unsigned) const;
  bool online_party_screen(const MenuFrame &) const;
  std::string online_party_text(const MenuFrame &, unsigned) const;
  std::string online_party_column(const MenuFrame &, unsigned, unsigned) const;
  std::vector<OnlinePartyMember> online_party_members() const;
  void set_online_matchmaking(std::shared_ptr<OnlineMatchmaking>);
  void update_online_matchmaking();
  std::string online_matchmaking_text(const MenuFrame &, unsigned text_index) const;
  unsigned online_matchmaking_pane() const;
  std::string expand(const std::string &) const;
  std::string profile_name() const;
  const std::string &signed_in_profile() const { return signed_in_; }
  std::vector<FrontendLocalPlayer> local_players() const;
  // Resolve actual SDL identity before dispatch. An unbound A/Start in the
  // offline sign-in/lobby opens the existing profile picker for that user.
  std::optional<unsigned> controller_slot(std::int32_t instance,bool join);
  std::optional<unsigned> joining_controller() const { return joining_controller_?joining_controller_:online_local_options_; }
  void leave_local_controller(unsigned);
  int profile_option(unsigned key, int fallback = 0) const;
  PcKeyboardSettings pc_keyboard_settings() const;
  ControllerSettings controller_settings() const;
  void adopt_controller_settings(const ControllerSettings &);
  const DisplaySettings &pc_display_settings() const { return pc_display_; }
  void adopt_graphics_mode(pstencil::GraphicsMode mode){pc_display_.graphics_mode=mode;}
  void adopt_pc_settings(const PcKeyboardSettings &,const DisplaySettings &,
                         std::array<unsigned,3> video);
  void set_pc_display_adapter(std::function<std::string(const DisplaySettings &)> adapter);
  std::string pc_value_label(const MenuFrame &, unsigned index, unsigned column = 1) const;
  bool capturing_binding() const { return binding_action_.has_value(); }
  void bind_pc_input(int);
  void set_pc_video_modes(std::vector<std::pair<unsigned,unsigned>> modes,
                         unsigned width, unsigned height, bool fullscreen);
  // A successful native display adapter returns an empty error string.
  void set_pc_video_adapter(std::function<std::string(unsigned,unsigned,bool)> adapter) {
    pc_video_adapter_ = std::move(adapter);
  }
  void set_pc_video_query(std::function<std::array<unsigned,3>()> query) { pc_video_query_=std::move(query); }
  FrontendAppearance profile_appearance() const;
  FrontendAppearance appearance() const;
  std::optional<FrontendLobbyView> lobby_view() const;
  void set_session_countdown(std::optional<std::int32_t> stage) { session_countdown_=stage; }
  const UiLobbyMap *focused_lobby_map(const MenuFrame &) const;
  std::optional<FrontendAppearance> profile_entry_appearance(const MenuFrame &, unsigned) const;
  std::optional<FrontendAppearance> focused_profile_appearance(const MenuFrame &) const;
  std::optional<int> focused_profile_control(const MenuFrame &, unsigned screen) const;
  std::optional<FrontendAppearance> model_appearance(const MenuFrame &) const;
  std::optional<int> focused_profile_species(const MenuFrame &) const;
  std::string keyboard_text;
  std::optional<FrontendLaunch> launch;
  bool quit{};
  unsigned revision{};

private:
  std::function<std::string(GamePauseAction)> game_pause_action_;
  std::string game_pause_objectives_;
  const FrontendContent &content_;
  MenuFrame build_pc(unsigned, unsigned) const;
  void accept_pc();
  std::optional<unsigned> binding_action_;
  unsigned binding_slot_{};
  std::vector<std::pair<unsigned,unsigned>> pc_video_modes_;
  DisplaySettings pc_display_;
  std::function<std::string(const DisplaySettings &)> pc_display_adapter_;
  unsigned pc_width_{}, pc_height_{};
  bool pc_fullscreen_{};
  std::optional<std::array<unsigned,3>> saved_pc_video_, previous_pc_video_;
  std::uint32_t video_preview_clock_{};
  void revert_pc_video();
  std::function<std::string(unsigned,unsigned,bool)> pc_video_adapter_;
  std::function<std::array<unsigned,3>()> pc_video_query_;
  std::function<void(const FrontendLaunch &)> multiplayer_map_validator_;
  std::function<bool(FrontendLobbySessionAction)> lobby_session_action_;
  std::optional<std::uint8_t> vista_mainmenu_pbuffer_byte_;
  std::shared_ptr<DiscordLogin> discord_login_;
  std::array<std::shared_ptr<DiscordLogin>,4> local_discord_logins_{};
  std::function<std::shared_ptr<DiscordLogin>()> local_discord_factory_;
  std::array<std::string,4> local_discord_subjects_{};
  std::optional<unsigned> online_local_options_;
  bool joining_online_{};
  std::uint64_t couch_revision_{},matchmaking_couch_revision_{};
  void populate_online_couch(MenuFrame &) const;
  bool accept_online_couch();
  void update_online_couch();
  void clear_online_couch();
  std::vector<OnlineLocalPlayer> online_local_players() const;
  std::shared_ptr<DiscordFriends> discord_social_;
  DiscordSocialSnapshot friends_;
  std::string friend_target_;
  std::string activity_target_;
  void populate_online_friends(MenuFrame &) const;
  bool accept_online_friends();
  void update_online_friends();
  std::shared_ptr<OnlineAccount> online_account_;
  std::shared_ptr<OnlineVoice> online_voice_;
  OnlineVoiceSnapshot voice_;
  bool voice_party_{}, party_voice_wanted_{};
  OnlineAccountSnapshot account_;
  std::string account_name_, clan_target_;
  unsigned clan_target_kind_{}, clan_message_page_{};
  std::optional<OnlineClanCommand> clan_confirmation_;
  void populate_online_clans(MenuFrame &) const;
  bool accept_online_clans();
  void clan_command(OnlineClanCommand);
  void confirm_clan(OnlineClanCommand, std::string);
  void populate_online_party(MenuFrame &) const;
  bool accept_online_party();
  void party_command(OnlinePartyCommand);
  std::string party_target_;
  unsigned party_target_kind_{};
  std::uint64_t party_target_version_{};

  std::shared_ptr<OnlineDirectory> online_directory_;
  OnlineDirectorySnapshot online_servers_;
  bool online_directory_active_{};
  std::string online_detail_id_;
  std::chrono::steady_clock::time_point online_refresh_at_;
  void refresh_online_directory();
  void populate_online_directory(MenuFrame &) const;
  std::shared_ptr<OnlineMatchmaking> online_matchmaking_;
  OnlineMatchmakingSnapshot matchmaking_;
  bool matchmaking_active_{}, quickmatch_pending_{}, match_voice_{};
  std::string selected_playlist_, matchmaking_name_;
  void refresh_online_matchmaking();
  void populate_online_matchmaking(MenuFrame &) const;
  unsigned pending_online_screen_ = 186, pending_online_context_{};
  bool awaiting_discord_login_{};
  std::filesystem::path preferences_;
  std::vector<MenuFrame> stack_;
  std::uint32_t ui_clock_{};
  std::optional<std::int32_t> session_countdown_;
  std::vector<std::string> profiles_;
  std::string signed_in_;
  std::array<std::string,4> local_profiles_{};
  std::array<std::int32_t,4> local_inputs_{-1,-1,-1,-1};
  std::optional<unsigned> joining_controller_;
  unsigned host_profile_{};
  void finish_local_signin();
  std::map<std::string, std::map<unsigned, int>> profile_values_;
  GameVariantSettings variant_values_;
  std::vector<UiVariant> saved_variants_;
  int saved_variant_ = -1;
  unsigned profile_{}, engine_{}, level_{}, map_{}, variant_{}, difficulty_ = 1;
  unsigned mode_{}, category_{}, appearance_part_{}, keyboard_purpose_{};
  int lobby_protocol_{};
  // The existing one-controller native session initially supplies team0.
  // Explicit local edits now update that input; source automatic assignment
  // remains a separate producer. Start carries the recovered profile team
  // byte to the native local/network session.
  std::int32_t lobby_team_{};
  FrontendLobbyTeamEdit lobby_team_edit_;
  std::string variant_name_ = "Slayer";
  std::size_t keyboard_cursor_{};
  unsigned keyboard_flags_{};
  bool keyboard_selected_{};
  std::optional<FrontendAppearance> appearance_edit_;
  std::string appearance_profile_;
  std::vector<std::pair<std::size_t, FrontendAppearance>> appearance_snapshots_;
  FrontendAppearance appearance_for_profile(const std::string &) const;
  void begin_appearance_edit();
  void commit_appearance_edit();
  void preview_appearance_selection();
  bool lobby_team_event(int type, int param);
  MenuFrame build(unsigned, unsigned) const;
  void pane_selection_changed(unsigned previous);
  void keyboard_labels();
  void unavailable(const std::string &);
  void dialog(unsigned id, int action, bool single = false);
};
struct FrontendCaptureAction {
  enum class Kind { Accept, Back, Select, Text, Commit, WheelUp, WheelDown, Click };
  Kind kind{};
  unsigned index{};
  std::string text;
  int x{}, y{}; // Diagnostic SDL window coordinates, independent of DPI.
};
struct FrontendOptions {
  std::shared_ptr<DesktopSession> desktop;
  // Retained configuration/content. A later invocation requires an explicit
  // source factory request; the old navigation stack cannot resume implicitly.
  std::shared_ptr<FrontendSession> session;
  enum class Entry { initial, postgame_report, fresh_lobby, session_return, online_return };
  Entry entry=Entry::initial;
  std::shared_ptr<const VistaPcrScreenOwner> postgame_report;
  // Called after actual mainmenu Cache/GPU initialization and before any old
  // frame is presented. The live source observer must request a new factory.
  std::function<void(MenuState&)> session_mainmenu_ready;
  // Real application session update after native input/UI clock delivery.
  // May allocate an explicitly admitted new frame through MenuState. Its
  // return requests this invocation to yield; no quit/launch is manufactured.
  std::function<bool(MenuState&,std::uint32_t,
      const std::function<void(FrontendUiSound)>&)> session_update;
  std::filesystem::path maps = "assets/disc/maps", screenshot, sweep;
  // Optional sampled 2D art; original content retains every logical extent.
  std::filesystem::path ui_assets;
  std::filesystem::path vista_settings;
  unsigned frames{}, screen = 9, movie_frame{}, scene_tick{};
  unsigned width=960,height=720;
  // Diagnostic routes use ordinary MenuState operations and disposable profiles.
  std::vector<FrontendCaptureAction> capture_actions;
  std::optional<unsigned> capture_ui_ms;
  // Diagnostic frame-by-frame clock progression for retained widget state.
  // Used only with --frames and --menu-ui-ms; ordinary UI uses its live clock.
  unsigned capture_ui_step_ms{};
  std::string movie;
  bool skip_intro{};
  bool hidden{}, validation{}, mute{}, resize_test{}, audit{}, freeze_scene{};
  bool debug_panel{}; // Optional F1 window initially open; ordinary startup remains clean.
  std::optional<pstencil::GraphicsMode> initial_graphics_mode;
};
class FrontendPainter {
public:
  FrontendPainter(const FrontendContent &, const std::filesystem::path &fonts,
                  Mesh &, const std::filesystem::path &frame_edges = {},
                  const FrontendUiAssetPack &ui_assets = {},
                  const std::filesystem::path &sampled_fonts = {});
  ~FrontendPainter();
  UiDraw draw(const MenuState &, float seconds, int pane_override = -1,
              std::uint32_t ui_clock_ms = 0,
              std::array<unsigned, 2> viewport = {640, 480});
  std::optional<unsigned> hit(float x, float y) const;
  std::optional<FrontendPointerTarget> pointer_hit(float x, float y) const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
std::optional<FrontendLaunch> run_frontend(const FrontendOptions &);
} // namespace halo2
