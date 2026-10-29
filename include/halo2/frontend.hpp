#pragma once
#include "halo2/source_player_profile.hpp"
#include "halo2/cache.hpp"
#include "halo2/scene.hpp"
#include "halo2/popup_blur.hpp"
#include "halo2/frontend_appearance.hpp"
#include "halo2/frontend_ui_assets.hpp"
#include "halo2/frontend_lobby.hpp"
#include "halo2/frontend_lobby_roster.hpp"
#include "halo2/frontend_lobby_team.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/frontend_vista.hpp"
#include "halo2/pc_settings.hpp"
#include "halo2/controller_settings.hpp"
#include "halo2/prompt_device.hpp"
#include "halo2/display_settings.hpp"
#include "halo2/discord_login.hpp"
#include "halo2/discord_social.hpp"
#include "halo2/online_directory.hpp"
#include "halo2/online_matchmaking.hpp"
#include "halo2/frontend_matchmaking_progress.hpp"
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
#include <utility>

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
// Native accepted-party notifications reuse authored UI sounds. Membership
// delta policy belongs to the replacement service, not the retired Live wire.
std::vector<FrontendUiSound> frontend_party_notification_sounds(
    const OnlineAccountSnapshot &, const OnlineAccountSnapshot &);
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
  bool matchmaking_map_preview{}; // Native binding of the original screen210 map-shot widget.
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
  // Authenticated hopper rows retain all304 bytes, including map-specific
  // options not recoverable from a display name. Ordinary menu presets omit it.
  std::optional<SourceGameVariant> source_settings;
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
  // Optional exact prepared custom-cache owner, bound before MenuState exists.
  // Empty retains the original stock scenario-leaf launch path.
  std::string cache_file;
  std::string shared_maps;
};
struct FrontendTypewriterSettings {
  std::int16_t interval_ms{};
  std::uint16_t cursor{};
  float scale{};
};
struct VistaLobbyMetadata;
class VistaWidgetCacheBank;
struct FrontendContent {
  bool original_matchmaking_layout{};
  bool gameplay_overlay{};
  // PC resource datums, skins, fonts and animations keep their own namespace.
  std::shared_ptr<const FrontendContent> vista;
  // Exact source-local PC map/rule/locale records for future dynamic owners.
  // This does not choose the 3D backdrop or enable a different menu route.
  std::shared_ptr<const VistaLobbyMetadata> vista_lobby_metadata;
  std::filesystem::path source_fonts;
  // Gameplay MCC maps own a different symbol table from mainmenu. Compiled
  // PC constants use an immutable exact-name join into that actual owner;
  // authored widget StringIDs remain untouched. Null retains the pinned
  // mainmenu namespace (or the original PC namespace for legacy resources).
  std::shared_ptr<const std::map<std::uint32_t,std::uint32_t>> pc_string_ids;
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
RetailFont decode_retail_font(std::span<const std::byte>);
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
  // Replacement-service receipt, separate from the original PCR result bytes.
  // False also identifies a disconnected source ending with no commit attempt.
  std::optional<bool> result_recorded;
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
  // Dialog98 Leave Party (40C00D -> 6151E2(0)) closes this report's session.
  std::function<void()> leave_session;
  // Detached PCR185 presentation of these actual results. Its callbacks own
  // result data only, never the completed match transport or its voice room.
  std::shared_ptr<const VistaPcrScreenOwner> lobby_review;
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
  // Original233423/23A0F3 screen lifetime uses a monotonic GetTickCount
  // service; pane changes and report allocation do not restart this clock.
  std::uint32_t pcr_open_clock{};
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
  int machines = 1; // Physical session peers, independently of couch seats.
  unsigned engine{}, difficulty{};
  const UiLobbyMap *map{};
  std::optional<FrontendAppearance> player_appearance;
  int player_team{};
  FrontendLobbyTeamEdit team_edit;
  std::array<FrontendLobbyTeamEdit,4> local_team_edits;
  std::array<int,4> local_team_session_indices{0,1,2,3};
  unsigned team_count = 1;
  bool allow_no_team{};
  std::array<std::optional<std::string>, 5> quick_options;
  std::optional<std::vector<std::string>> party_names;
  std::vector<unsigned> party_speakers;
  std::vector<FrontendLobbyRosterPlayer> party_players;
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
  std::filesystem::path shared_maps;
};
enum class FrontendLobbySessionAction { Start, Setup, Back, Auxiliary };
// Native pointer actions share the menu's existing command handlers. Prompt
// targets come from the actual clipped text layout, including Vista scaling.
enum class FrontendPointerAction { Entry, Accept, Back, Auxiliary, Players, CommitText, ClearBinding, InviteAccept, InviteDismiss, InviteTab, PcrTab, PriorStats };
struct FrontendInviteNotice { std::string id,name; bool accepting{},can_accept{}; };
enum class GamePauseAction { Resume, Revert, Restart, Quit };
inline constexpr unsigned campaign_game_pause_context = 1;
inline constexpr unsigned online_game_pause_context = 2;
inline constexpr unsigned team_game_pause_context = 3;
struct GamePauseTeams {
  std::int16_t current=-1;
  bool enabled{};
  std::uint8_t available{};
  std::array<std::string,8> names{"Red","Blue","Yellow","Green","Purple","Orange","Brown","Pink"};
};
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
  // Real mainmenu map initialization, not a menu open/resize. The source
  // global UI intro retains its frame-delta clock independently of PCR16.
  void initialize_mainmenu_map();
  std::optional<std::uint32_t> ui_map_intro_color() const;
  void open(unsigned screen, unsigned context = 0);
  void game_pause_teams(std::optional<GamePauseTeams> teams,std::function<std::string(std::int16_t)> change) {
    game_pause_teams_=std::move(teams);game_pause_change_team_=std::move(change);
  }
  void game_pause_error(const std::string &error) {unavailable(error);}
  void open_game_pause(bool campaign);
  void open_game_pause(bool campaign, bool online);
  void set_game_pause_action(std::function<std::string(GamePauseAction)> action) {
    game_pause_action_=std::move(action);
  }
  bool game_pause() const { return content_.gameplay_overlay; }
  void game_pause_objectives(std::string value) { game_pause_objectives_=std::move(value); }
  const std::string &game_pause_objectives() const { return game_pause_objectives_; }
  // Explicit real PCR factory boundary. Raw source16/185 only; no automatic
  // timer/completion route, synthetic report or reuse of an old lobby frame.
  void open_vista_pcr(unsigned,std::shared_ptr<const VistaPcrScreenOwner>);
  void set_pcr_clock(std::function<std::uint32_t()>);
  std::optional<std::uint32_t> pcr_intro_color(const MenuFrame&) const;
  // A completed source menu-return factory allocates a fresh lobby frame from
  // retained local selections. It never resumes a pre-match Back stack.
  void open_vista_lobby();
  void set_system_link_lobby(bool host,std::vector<FrontendLobbyRosterPlayer>,unsigned machines,std::span<const unsigned> local_slots={});
  void leave_system_link_lobby();
  void adopt_system_link_configuration(std::string_view map,const UiVariant&);
  void system_link_configuration_error(std::string message="The selected match settings are invalid."){
    unavailable(std::move(message));
  }
  void set_system_link_leave(std::function<void()> action){system_link_leave_=std::move(action);}
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
  bool switch_pcr_tab(int direction);
  bool has_online_prior_stats() const;
  bool review_online_postgame();
  void close_online_prior_stats();
  bool vista_pcr_event(std::uint32_t source_code);
  // ONLINE PCR16 services: 6156D2's host predicate is false for a party
  // member who is not the leader, whose Done opens dialog98 (40C00D).
  bool online_postgame_host() const;
  void vista_pcr_dialog(const std::array<std::uint32_t, 8> &);
  void move(int);
  void mouse_wheel(int vertical);
  void navigate(int horizontal, int vertical);
  void select(unsigned);
  void accept();
  void back();
  void auxiliary();
  void players();
  bool activate_pointer(FrontendPointerTarget);
  std::optional<FrontendInviteNotice> invite_notice() const;
  void poll_party_notices() { update_online_account(); }
  bool act_invite_notice(bool accept);
  bool switch_invite_tab(int direction);
  bool party_invite_picker() const;
  unsigned party_invite_tab() const { return party_invite_tab_; }
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
  bool online_friends_available() const;
  std::string online_friends_text(const MenuFrame &, unsigned) const;
  std::string online_friends_column(const MenuFrame &, unsigned, unsigned) const;
  void return_online(std::string message = {});
  // Leaving an ONLINE match (PCR Done, abandon, disconnect) returns the party
  // to its matchmaking pregame lobby, as TU1.5 does.
  void return_online_lobby(std::string message = {});
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
  std::vector<FrontendUiSound> take_ui_sounds();
  // Read-only observations of the actual menu-bound snapshot and queued
  // cues. A renderer observer must not resample a worker between pane/draw.
  const std::optional<OnlineParty> &online_party_observation() const { return account_.party; }
  const std::vector<FrontendUiSound> &ui_sound_observation() const { return pending_ui_sounds_; }
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
  std::string online_matchmaking_original_text(unsigned pane, unsigned text_index) const;
  std::optional<std::uint32_t> online_matchmaking_map_bitmap() const;
  std::optional<unsigned> online_matchmaking_game_icon() const;
  std::optional<unsigned> online_matchmaking_rank(unsigned row) const;
  std::optional<FrontendAppearance> online_matchmaking_player_appearance(unsigned row) const;
  std::optional<unsigned> online_matchmaking_speaker(unsigned row) const;
  unsigned online_matchmaking_pane() const;
  // TU1.5 screen210 state (0x199D7C), its four-pane selector (0x25142F) and
  // the start-match countdown getter (0x59590). online_matchmaking_pane()
  // remains the PC/MCC authored pane that hosts the presentation.
  MatchmakingProgressUi online_matchmaking_ui() const;
  unsigned online_matchmaking_progress_pane() const;
  std::int32_t online_matchmaking_countdown() const { return matchmaking_countdown_; }
  bool online_matchmaking_team_colors() const;
  // 0x2514FD row order of the admitted match (sorted only on pane3).
  std::vector<unsigned> online_matchmaking_roster_order() const;
  // A Back request refused by the screen (TU1.5 0x25217B) has already queued
  // its error cue; the caller must not also play the Back cue.
  bool take_back_refused() { return std::exchange(back_refused_, false); }
  // Matchmaking pregame lobby START GAME countdown (TU1.5 0x19A78E/0x6E360),
  // -1 while none runs.
  std::int32_t online_lobby_countdown() const { return online_lobby_countdown_; }
  std::string expand(const std::string &) const;
  std::string profile_name() const;
  const std::string &signed_in_profile() const { return signed_in_; }
  std::vector<FrontendLocalPlayer> local_players() const;
  bool local_team_event(unsigned controller,int type,int param);
  void adopt_gameplay_teams(std::span<const std::int8_t>,bool preserve_edits=false);
  void set_lobby_team_change(std::function<std::string(unsigned,std::int16_t)> change){lobby_team_change_=std::move(change);}
  // Resolve actual SDL identity before dispatch. An unbound A/Start in the
  // offline sign-in/lobby opens the existing profile picker for that user.
  std::optional<unsigned> controller_slot(std::int32_t instance,bool join);
  std::optional<unsigned> joining_controller() const { return joining_controller_?joining_controller_:online_local_options_; }
  bool custom_match_settings_frame(const MenuFrame &) const;
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
                         unsigned width, unsigned height, FullscreenMode fullscreen);
  void set_pc_video_modes(std::vector<std::pair<unsigned,unsigned>> modes,
                         unsigned width,unsigned height,bool fullscreen) {
    set_pc_video_modes(std::move(modes),width,height,fullscreen?FullscreenMode::Fullscreen:FullscreenMode::Windowed);
  }
  // A successful native display adapter returns an empty error string.
  void set_pc_video_adapter(std::function<std::string(unsigned,unsigned,FullscreenMode)> adapter) {
    pc_video_adapter_ = std::move(adapter);
  }
  void set_pc_video_query(std::function<std::array<unsigned,3>()> query) { pc_video_query_=std::move(query); }
  FrontendAppearance profile_appearance() const;
  std::string profile_service_tag() const;
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
  std::optional<GamePauseTeams> game_pause_teams_;
  std::function<std::string(std::int16_t)> game_pause_change_team_;
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
  FullscreenMode pc_fullscreen_mode_{};
  std::optional<std::array<unsigned,3>> saved_pc_video_, previous_pc_video_;
  std::uint32_t video_preview_clock_{};
  void revert_pc_video();
  std::function<std::string(unsigned,unsigned,FullscreenMode)> pc_video_adapter_;
  std::function<std::array<unsigned,3>()> pc_video_query_;
  std::function<void(const FrontendLaunch &)> multiplayer_map_validator_;
  std::function<bool(FrontendLobbySessionAction)> lobby_session_action_;
  std::function<void()> system_link_leave_;
  std::optional<bool> system_link_host_;
  std::vector<FrontendLobbyRosterPlayer> system_link_players_;
  unsigned system_link_machines_{};
  std::array<int,4> system_link_local_slots_{-1,-1,-1,-1};
  std::function<std::string(unsigned,std::int16_t)> lobby_team_change_;
  std::optional<std::uint8_t> vista_mainmenu_pbuffer_byte_;
  std::shared_ptr<DiscordLogin> discord_login_;
  std::array<std::shared_ptr<DiscordLogin>,4> local_discord_logins_{};
  std::array<std::shared_ptr<OnlineAccount>,4> local_profile_accounts_{};
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
  std::uint64_t friend_join_serial_{};
  void update_online_friend_join();
  std::string activity_target_;
  void populate_online_friends(MenuFrame &) const;
  bool accept_online_friends();
  void update_online_friends();
  bool native_friends() const;
  void populate_native_friends(MenuFrame &) const;
  bool accept_native_friends();
  std::shared_ptr<OnlineAccount> online_account_;
  std::shared_ptr<OnlineVoice> online_voice_;
  OnlineVoiceSnapshot voice_;
  bool voice_party_{}, party_voice_wanted_{};
  OnlineAccountSnapshot account_;
  std::vector<FrontendUiSound> pending_ui_sounds_;
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
  std::vector<std::string> dismissed_party_invites_;
  unsigned party_invite_tab_{};

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
  bool matchmaking_consent_pending_{};
  std::string selected_playlist_, matchmaking_name_;
  void refresh_online_matchmaking();
  void populate_online_matchmaking(MenuFrame &) const;
  void reset_online_matchmaking_progress();
  // Hands the counted-down admitted match to gameplay; false after a refusal.
  bool launch_online_match();
  // c_session_state_start_match (0x72700) clock and B's published seconds.
  std::optional<std::uint32_t> matchmaking_start_clock_;
  std::int32_t matchmaking_countdown_ = matchmaking_countdown_none;
  bool matchmaking_have_fun_{};
  // c_matchmaking_screen +0x618 current pane and +0x614 pane clock (0x251B39).
  unsigned matchmaking_progress_pane_{};
  int matchmaking_progress_title_state_{}; // Original c_matchmaking_screen+610, updated only on pane change.
  std::uint32_t matchmaking_pane_clock_{};
  bool back_refused_{};
  // Party A's member handles while a search it did not cancel locally runs;
  // empty otherwise (0x250804 session-state notices).
  std::vector<std::string> matchmaking_search_party_;
  // True when the screen changed: a member follows the leader's search into
  // screen210, or a party-wide end of the search returned to the lobby.
  bool update_online_party_search();
  // Party A's lobby after matchmaking keeps its hopper (TU1.5 value49a4=2,
  // +49C8): START GAME searches it again after the lobby countdown.
  bool online_lobby_matchmaking_{};
  bool online_lobby_playlist_pending_{};
  bool online_lobby_create_submitted_{};
  struct OnlinePrivacyPending {
    std::uint64_t frame_instance{},command_serial{},revision{};
    unsigned mode{};
    std::string context;
  };
  std::optional<OnlinePrivacyPending> online_privacy_pending_;
  void update_online_privacy();
  void prepare_online_matchmaking_lobby(bool quickmatch);
  std::string online_lobby_start_error() const;
  std::shared_ptr<const VistaPcrScreenOwner> online_prior_report_;
  std::int32_t online_lobby_countdown_ = -1, online_lobby_countdown_stage_ = -1;
  std::uint32_t online_lobby_countdown_clock_{};
  std::uint64_t online_lobby_countdown_consumed_{};
  std::vector<std::string> online_lobby_countdown_party_;
  bool online_lobby_leave_pending_{};
  void update_online_lobby_leave();
  void enter_online_matchmaking_lobby();
  void cancel_online_lobby_countdown();
  // True when the countdown reached zero and opened the search.
  bool update_online_lobby_countdown();
  // A party follower's report continues once the leader has left it.
  void update_online_postgame();
  unsigned pending_online_screen_ = 186, pending_online_context_{};
  bool awaiting_discord_login_{};
  bool saved_login_suppressed_{};
  std::string login_profile_;
  std::filesystem::path preferences_;
  std::vector<MenuFrame> stack_;
  std::uint32_t ui_clock_{};
  std::optional<std::uint32_t> ui_map_initialized_clock_;
  std::function<std::uint32_t()> pcr_clock_;
  std::optional<std::int32_t> session_countdown_;
  std::vector<std::string> profiles_;
  std::string signed_in_;
  std::array<std::string,4> local_profiles_{};
  std::array<std::int32_t,4> local_inputs_{-1,-1,-1,-1};
  std::optional<unsigned> joining_controller_;
  unsigned host_profile_{};
  void finish_local_signin();
  std::map<std::string, std::map<unsigned, int>> profile_values_;
  std::map<std::string,std::string> profile_service_tags_;
  std::map<std::string,std::string> profile_login_keys_;
  std::string profile_login_key(const std::string &);
  void bind_primary_login();
  GameVariantSettings variant_values_;
  std::optional<SourceGameVariant> variant_source_settings_;
  UiVariant current_variant() const;
  std::optional<UiParameter> variant_parameter(unsigned datum) const;
  std::vector<UiVariant> saved_variants_;
  int saved_variant_ = -1;
  unsigned profile_{}, engine_{}, level_{}, map_{}, variant_{}, difficulty_ = 1;
  unsigned browsed_engine_{};
  unsigned mode_{}, category_{}, appearance_part_{}, keyboard_purpose_{};
  int lobby_protocol_{};
  // The existing one-controller native session initially supplies team0.
  // Explicit local edits now update that input; source automatic assignment
  // remains a separate producer. Start carries the recovered profile team
  // byte to the native local/network session.
  std::int32_t lobby_team_{};
  FrontendLobbyTeamEdit lobby_team_edit_;
  std::array<std::int32_t,4> local_teams_{};
  std::array<FrontendLobbyTeamEdit,4> local_team_edits_;
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
  enum class Entry { initial, postgame_report, fresh_lobby, session_return, online_return, system_link_browser };
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
  // Read-only validation capture after the actual UI update. An empty path
  // leaves the ordinary draw unchanged; this cannot provide state or time.
  std::function<std::filesystem::path(const MenuState&)> frame_capture;
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
  // Capture-only warm-up: ordinary UI/session time and input remain live;
  // the finite frame budget starts once the actual new-map intro is clear.
  bool wait_intro_clear{};
  std::string movie;
  bool skip_intro{};
  bool hidden{}, validation{}, mute{}, resize_test{}, audit{}, freeze_scene{};
  std::optional<pstencil::GraphicsMode> initial_graphics_mode;
};
class FrontendPainter {
public:
  FrontendPainter(const FrontendContent &, const std::filesystem::path &fonts,
                  Mesh &, const std::filesystem::path &frame_edges = {},
                  const FrontendUiAssetPack &ui_assets = {},
                  const std::filesystem::path &sampled_fonts = {});
  ~FrontendPainter();
  void prompt_device(PromptDevice);
  UiDraw draw(const MenuState &, float seconds, int pane_override = -1,
              std::uint32_t ui_clock_ms = 0,
              std::array<unsigned, 2> viewport = {640, 480});
  UiDraw draw_invite_notice(const FrontendInviteNotice &,std::array<unsigned,2> viewport,bool append=false);
  std::optional<unsigned> hit(float x, float y) const;
  std::optional<FrontendPointerTarget> pointer_hit(float x, float y) const;

private:
  friend class FrontendNetworkTransitionPainter;
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
std::optional<FrontendLaunch> run_frontend(const FrontendOptions &);
} // namespace halo2
