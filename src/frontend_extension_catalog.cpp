#include "halo2/frontend_extension_catalog.hpp"
#include "halo2/frontend.hpp"
#include <iterator>
#include <limits>
#include <utility>

namespace halo2 {
namespace {
struct VistaDefinition {
  std::uint16_t id;
  std::uint32_t datum;
  unsigned panes;
  bool variant;
  const char *path;
};
// Generated from actual wgit root+4 and root+32 records, not an inferred menu
// enum. Source map SHA256 eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44.
// Reproduce with analysis/frontend/hd-ui-followup-20261009/audit-vista-menu-extension.py.
// Common paths are variants, never overrides of the original Xbox registry.
constexpr VistaDefinition vista_definitions[]{
  {1, 0xe6280406u, 4, true, "ui\\screens\\game_shell\\difficulty\\difficulty_lobby"},
  {2, 0xe6270405u, 1, true, "ui\\screens\\game_shell\\load_level\\load_level_lobby"},
  {5, 0xe551032fu, 1, true, "ui\\screens\\game_shell\\main_menu_screen\\game_shell_background"},
  {6, 0xe540031eu, 1, true, "ui\\screens\\game_shell\\main_menu_screen\\main_menu"},
  {7, 0xe59b0379u, 1, true, "ui\\screens\\misc\\error_dialog_ok_cancel"},
  {8, 0xe59a0378u, 1, true, "ui\\screens\\misc\\error_dialog_ok"},
  {9, 0xe5c0039eu, 1, true, "ui\\screens\\game_shell\\start_screen\\start_screen"},
  {10, 0xe5b40392u, 1, true, "ui\\screens\\game_shell\\dialog_profile_select\\dialog_profile"},
  {11, 0xe5ae038cu, 1, true, "ui\\screens\\game_shell\\load_level\\load_level"},
  {12, 0xe5a80386u, 4, true, "ui\\screens\\game_shell\\difficulty\\difficulty"},
  {13, 0xe59d037bu, 1, true, "ui\\screens\\game_shell\\network_squad_browser_screen\\network_squad_browser"},
  {14, 0xe5ca03a8u, 3, true, "ui\\screens\\game_shell\\pregame_lobby\\pregame_lobby"},
  {15, 0xe5e103bfu, 1, true, "ui\\screens\\game_shell\\mp_variant_select\\mp_variant_select"},
  {16, 0xe5b80396u, 6, true, "ui\\screens\\game_shell\\postgame_statistics\\pcr"},
  {17, 0xe5e303c1u, 1, true, "ui\\screens\\game_shell\\mp_map_select\\mp_map_select"},
  {19, 0xe55c033au, 6, true, "ui\\screens\\game_shell\\settings_screen\\settings"},
  {20, 0xe5b60394u, 1, true, "ui\\screens\\game_shell\\dialog_live_signin\\dialog_live_signin"},
  {21, 0xe5e703c5u, 1, true, "ui\\screens\\game_shell\\dialog_live_password\\dialog_live_password"},
  {23, 0xe5dc03bau, 1, true, "ui\\screens\\game_shell\\pregame_lobby\\squad_settings_dialog"},
  {25, 0xe5df03bdu, 1, true, "ui\\screens\\game_shell\\pregame_lobby\\squad_privacy_settings"},
  {26, 0xe5ea03c8u, 3, true, "ui\\screens\\game_shell\\online_y_menu\\online_y_menu"},
  {27, 0xe5ef03cdu, 1, true, "ui\\screens\\game_shell\\online_y_menu\\online_y_menu_collapsed"},
  {30, 0xe5f203d0u, 1, true, "ui\\screens\\game_shell\\4way_multiplayer_signin\\4way_signin_main"},
  {31, 0xe5f003ceu, 1, true, "ui\\screens\\game_shell\\online_y_menu\\player_selected_dialog"},
  {32, 0xe5f903d7u, 1, true, "ui\\screens\\game_shell\\player_selected\\player_selected_dialog"},
  {33, 0xe5fb03d9u, 1, true, "ui\\screens\\misc\\confirmation_dialog"},
  {34, 0xe5fc03dau, 1, true, "ui\\screens\\game_shell\\player_selected\\live_feedback_dialog"},
  {35, 0xe5fe03dcu, 1, true, "ui\\screens\\game_shell\\player_selected\\message_type_dialog"},
  {36, 0xe6380416u, 1, true, "ui\\screens\\game_shell\\xbox_live\\voice_mail_record\\voice_mail_record"},
  {38, 0xe561033fu, 3, true, "ui\\screens\\game_shell\\settings_screen\\player_profile\\edit_profile_menu"},
  {39, 0xe5680346u, 1, true, "ui\\screens\\game_shell\\settings_screen\\player_profile\\controller_settings"},
  {40, 0xe58c036au, 8, true, "ui\\screens\\game_shell\\settings_screen\\player_profile\\button_settings"},
  {41, 0xe5870365u, 8, true, "ui\\screens\\game_shell\\settings_screen\\player_profile\\thumbstick_settings"},
  {42, 0xe5930371u, 1, true, "ui\\screens\\game_shell\\settings_screen\\player_profile\\look_sensitivity_settings"},
  {43, 0xe591036fu, 1, true, "ui\\screens\\game_shell\\settings_screen\\player_profile\\invert_look"},
  {44, 0xe58f036du, 1, true, "ui\\screens\\game_shell\\settings_screen\\player_profile\\aim_assist"},
  {45, 0xe56c034au, 1, true, "ui\\screens\\game_shell\\settings_screen\\player_profile\\handicap_settings"},
  {47, 0xe5720350u, 1, true, "ui\\screens\\game_shell\\settings_screen\\player_profile\\multiplayer_appearance_settings"},
  {48, 0xe5850363u, 1, true, "ui\\screens\\game_shell\\settings_screen\\player_profile\\delete_profile_confirmation"},
  {49, 0xe5760354u, 1, true, "ui\\screens\\game_shell\\settings_screen\\player_profile\\choose_emblem"},
  {50, 0xe5790357u, 1, true, "ui\\screens\\game_shell\\settings_screen\\player_profile\\choose_primary_color"},
  {52, 0xe57b0359u, 1, true, "ui\\screens\\game_shell\\settings_screen\\player_profile\\choose_player_model"},
  {53, 0xe57d035bu, 1, true, "ui\\screens\\game_shell\\settings_screen\\player_profile\\voice_settings_menu"},
  {54, 0xe57f035du, 1, true, "ui\\screens\\game_shell\\settings_screen\\player_profile\\voice_mask"},
  {55, 0xe581035fu, 1, true, "ui\\screens\\game_shell\\settings_screen\\player_profile\\voice_through_tv"},
  {58, 0xe5830361u, 1, true, "ui\\screens\\game_shell\\settings_screen\\player_profile\\xbox_live_appear_offline"},
  {60, 0xe5950373u, 7, true, "ui\\screens\\game_shell\\mp_game_select\\game_engine_category_listing"},
  {167, 0xe60003deu, 1, true, "ui\\screens\\game_shell\\virtual_keyboard\\virtual_keyboard"},
  {177, 0xe60203e0u, 1, true, "ui\\screens\\game_shell\\pregame_lobby\\squad_leader_select"},
  {178, 0xe60403e2u, 6, true, "ui\\screens\\game_shell\\settings_screen\\variant_settings\\variant_editing_options"},
  {182, 0xe60b03e9u, 1, true, "ui\\screens\\game_shell\\xbox_live\\task_progress_dialog\\task_progress_dialog"},
  {183, 0xe60903e7u, 1, true, "ui\\screens\\game_shell\\settings_screen\\player_profile\\vibration"},
  {184, 0xe60d03ebu, 1, true, "ui\\screens\\game_shell\\pregame_lobby\\boot_player"},
  {185, 0xe5be039cu, 6, true, "ui\\screens\\game_shell\\postgame_statistics\\pcr_lobby"},
  {186, 0xe60f03edu, 4, true, "ui\\screens\\game_shell\\xbox_live\\xbox_live_main_menu\\xbox_live_main_menu"},
  {192, 0xe61903f7u, 1, true, "ui\\screens\\game_shell\\xbox_live\\xbox_live_notification_beeper\\online_notification_beeper"},
  {193, 0xe61303f1u, 1, true, "ui\\screens\\game_shell\\profile_picker\\profile_picker"},
  {194, 0xe61b03f9u, 1, true, "ui\\screens\\game_shell\\selected_saved_game_file_actions\\saved_game_file_actions"},
  {204, 0xe61d03fbu, 1, true, "ui\\screens\\misc\\squad_join_progress_dialog\\squad_join_progress_dialog"},
  {206, 0xe61f03fdu, 1, true, "ui\\screens\\game_shell\\mp_map_select\\mp_map_select_lobby"},
  {207, 0xe62003feu, 7, true, "ui\\screens\\game_shell\\mp_game_select\\game_engine_category_listing_lobby"},
  {208, 0xe6220400u, 1, true, "ui\\screens\\game_shell\\mp_variant_select\\mp_variant_select_lobby"},
  {209, 0xe6230401u, 1, true, "ui\\screens\\misc\\loading_screen\\loading_screen"},
  {210, 0xe62d040bu, 3, true, "ui\\screens\\game_shell\\xbox_live\\matchmaking_progress_dialog\\matchmaking_progress_screen"},
  {211, 0xe6330411u, 1, true, "ui\\screens\\game_shell\\xbox_live\\message_display\\message_display"},
  {212, 0xe64d042bu, 1, true, "ui\\screens\\misc\\texture_geometry_cache_fade_in\\fade_from_black"},
  {215, 0xe6360414u, 1, true, "ui\\screens\\game_shell\\xbox_live\\message_send\\message_send"},
  {216, 0xe63e041cu, 1, true, "ui\\screens\\game_shell\\online_y_menu\\friends_options_dialog"},
  {217, 0xe641041fu, 1, true, "ui\\screens\\game_shell\\online_y_menu\\clan_options_dialog"},
  {218, 0xe63f041du, 1, true, "ui\\screens\\game_shell\\campaign_options\\campaign_options_dialog"},
  {219, 0xe6290407u, 1, true, "ui\\screens\\game_shell\\xbox_live\\matchmaking\\optimatch_listing"},
  {221, 0xe6440422u, 1, true, "ui\\screens\\game_shell\\settings_screen\\variant_settings\\editing_format_screens\\top_level_settings"},
  {222, 0xe6420420u, 1, true, "ui\\screens\\game_shell\\settings_screen\\variant_settings\\editing_format_screens\\quick_options"},
  {223, 0xe6460424u, 1, true, "ui\\screens\\game_shell\\settings_screen\\variant_settings\\editing_format_screens\\low_level_settings"},
  {230, 0xe6470425u, 1, true, "ui\\screens\\misc\\multiple_choice_dialog"},
  {231, 0xe6490427u, 1, true, "ui\\screens\\game_shell\\network_transition_progress\\network_transition_progress_screen"},
  {233, 0xe64b0429u, 1, true, "ui\\screens\\game_shell\\settings_screen\\player_profile\\choose_emblem_background"},
  {236, 0xe64e042cu, 4, true, "ui\\screens\\game_shell\\xbox_live\\edit_clan\\clan_permissions"},
  {237, 0xe62b0409u, 1, true, "ui\\screens\\game_shell\\xbox_live\\matchmaking\\optimatch_listing_lobby"},
  {238, 0xe650042eu, 1, true, "ui\\screens\\misc\\tcr_dialog"},
  {239, 0xe6540432u, 1, true, "ui\\screens\\game_shell\\settings_screen\\extras\\extras_settings"},
  {240, 0xe59c037au, 1, true, "ui\\screens\\misc\\error_dialog_large"},
  {241, 0xe6560434u, 1, true, "ui\\screens\\game_shell\\settings_screen\\extras\\extras_enabled_dialog"},
  {242, 0xe6520430u, 1, true, "ui\\screens\\game_shell\\settings_screen\\player_profile\\subtitles"},
  {243, 0xe66d044bu, 4, false, "ui\\screens\\game_shell\\settings_screen\\player_profile\\keyboard_settings"},
  {245, 0xe670044eu, 1, false, "ui\\screens\\game_shell\\settings_screen\\player_profile\\invert_dual_wield"},
  {246, 0xe6580436u, 2, false, "ui\\screens\\game_shell\\settings_screen\\system_settings"},
  {247, 0xe65a0438u, 4, false, "ui\\screens\\game_shell\\bungie_news\\bungie_news"},
  {248, 0xe65d043bu, 1, false, "ui\\screens\\game_shell\\live_game_browser\\filter_select"},
  {249, 0xe65f043du, 1, false, "ui\\screens\\game_shell\\live_game_browser\\live_game_browser"},
  {250, 0xe6680446u, 1, false, "ui\\screens\\game_shell\\live_game_browser\\game_details"},
  {251, 0xe66c044au, 1, false, "ui\\screens\\game_shell\\mp_map_select\\mp_custom_map_select_lobby"},
  {252, 0xe66a0448u, 2, false, "ui\\screens\\game_shell\\mp_map_select\\mp_base_map_select"},
  {253, 0xe6720450u, 1, false, "ui\\screens\\game_shell\\settings_screen\\player_profile\\advanced_keyboard_settings"},
  {255, 0xe6740452u, 1, false, "ui\\screens\\game_shell\\settings_screen\\player_profile\\restore_default_controller_settings"},
  {256, 0xe6760454u, 1, false, "ui\\screens\\game_shell\\settings_screen\\video_settings\\video_settings"},
  {257, 0xe6780456u, 1, false, "ui\\screens\\game_shell\\settings_screen\\audio_settings\\audio_settings"},
  {258, 0xe67a0458u, 1, false, "ui\\screens\\game_shell\\settings_screen\\player_profile\\keyboard_settings_menu"},
  {263, 0xe67c045au, 1, false, "ui\\screens\\game_shell\\settings_screen\\audio_settings\\volume_settings"},
  {264, 0xe67e045cu, 1, false, "ui\\screens\\game_shell\\settings_screen\\audio_settings\\sound_quality"},
  {265, 0xe680045eu, 1, false, "ui\\screens\\game_shell\\settings_screen\\audio_settings\\eax"},
  {266, 0xe6820460u, 1, false, "ui\\screens\\game_shell\\settings_screen\\audio_settings\\audio_hardware_3d"},
  {267, 0xe6840462u, 1, false, "ui\\screens\\game_shell\\settings_screen\\audio_settings\\speaker_config"},
  {268, 0xe6860464u, 1, false, "ui\\screens\\game_shell\\settings_screen\\audio_settings\\restore_audio_defaults"},
  {269, 0xe6880466u, 1, false, "ui\\screens\\game_shell\\settings_screen\\video_settings\\resolution"},
  {270, 0xe68a0468u, 1, false, "ui\\screens\\game_shell\\settings_screen\\video_settings\\aspect_ratio"},
  {271, 0xe68c046au, 1, false, "ui\\screens\\game_shell\\settings_screen\\video_settings\\display_mode"},
  {272, 0xe68e046cu, 1, false, "ui\\screens\\game_shell\\settings_screen\\video_settings\\brightness_level"},
  {273, 0xe690046eu, 1, false, "ui\\screens\\game_shell\\settings_screen\\video_settings\\gamma_setting"},
  {274, 0xe6920470u, 1, false, "ui\\screens\\game_shell\\settings_screen\\video_settings\\anti_aliasing"},
  {275, 0xe6940472u, 1, false, "ui\\screens\\game_shell\\settings_screen\\video_settings\\resize_hud"},
  {276, 0xe6960474u, 1, false, "ui\\screens\\game_shell\\settings_screen\\video_settings\\restore_video_defaults"},
  {277, 0xe6980476u, 1, false, "ui\\screens\\game_shell\\live_game_browser\\search_option_maps"},
  {278, 0xe69a0478u, 1, false, "ui\\screens\\game_shell\\live_game_browser\\search_option_gametype"},
  {279, 0xe69c047au, 1, false, "ui\\screens\\game_shell\\live_game_browser\\search_option_variant"},
  {280, 0xe69e047cu, 1, false, "ui\\screens\\game_shell\\live_game_browser\\search_option_gold_only"},
  {281, 0xe6a0047eu, 1, false, "ui\\screens\\game_shell\\live_game_browser\\search_option_dedicated_server"},
  {282, 0xe6a20480u, 1, false, "ui\\screens\\game_shell\\live_game_browser\\search_option_max_players"},
  {283, 0xe6a40482u, 1, false, "ui\\screens\\game_shell\\live_game_browser\\search_option_favorites"},
  {284, 0xe6a60484u, 1, false, "ui\\screens\\game_shell\\live_game_browser\\search_option_show_full_games"},
  {285, 0xe6a80486u, 1, false, "ui\\screens\\game_shell\\settings_screen\\video_settings\\safe_area"},
  {286, 0xe6aa0488u, 1, false, "ui\\screens\\game_shell\\pregame_lobby\\find_game_menu"},
  {287, 0xe6ac048au, 1, false, "ui\\screens\\game_shell\\live_game_browser\\search_options"},
  {290, 0xe6ae048cu, 1, false, "ui\\screens\\game_shell\\settings_screen\\video_settings\\lod_setting"},
  {291, 0xe6b0048eu, 1, false, "ui\\screens\\game_shell\\settings_screen\\video_settings\\refresh"},
  {292, 0xe6b20490u, 1, false, "ui\\screens\\game_shell\\start_screen\\esrb_warning"},
  {293, 0xe6b40492u, 1, false, "ui\\screens\\misc\\resolution_confirmation"},
  {294, 0xe6b60494u, 1, false, "ui\\screens\\game_shell\\settings_screen\\player_profile\\invert_kb_look"},
  {295, 0xe6b80496u, 1, false, "ui\\screens\\game_shell\\settings_screen\\player_profile\\restore_default_keyboard_settings"},
  {296, 0xe6ba0498u, 1, false, "ui\\screens\\game_shell\\settings_screen\\network_settings\\network_adapter"},
  {297, 0xe6bc049au, 1, false, "ui\\screens\\game_shell\\settings_screen\\about\\about_dialog"},
};
static_assert(std::size(vista_definitions) == 131);
} // namespace

FrontendExtensionCatalog::FrontendExtensionCatalog(const FrontendContent &original) {
  entries_.reserve(original.screens.size() + std::size(vista_definitions));
  for (const auto &[id, screen] : original.screens) {
    if (id != screen.id || id > std::numeric_limits<std::uint16_t>::max())
      throw FormatError("original menu catalog id differs from definition");
    add({{FrontendMenuSource::original_xbox, std::uint16_t(id)}, screen.name,
         unsigned(screen.panes.size()), FrontendMenuRelation::original,
         std::nullopt, std::nullopt});
  }
  for (const auto &source : vista_definitions) {
    std::optional<FrontendMenuKey> parent;
    if (const auto *entry = find(FrontendMenuSource::original_xbox, source.path))
      parent = entry->key;
    add({{FrontendMenuSource::vista, source.id}, source.path, source.panes,
         source.variant ? FrontendMenuRelation::vista_variant
                        : FrontendMenuRelation::vista_addition,
         source.datum, parent});
  }
}

void FrontendExtensionCatalog::add(FrontendMenuCatalogEntry entry) {
  const auto path = std::pair{entry.key.source, entry.path};
  if (entry.path.empty() || keys_.contains(entry.key) || paths_.contains(path))
    throw FormatError("duplicate or empty source-qualified menu catalog identity");
  const auto index = entries_.size();
  keys_.emplace(entry.key, index);
  paths_.emplace(path, index);
  entries_.push_back(std::move(entry));
}

std::span<const FrontendMenuCatalogEntry> FrontendExtensionCatalog::entries() const noexcept {
  return entries_;
}
const FrontendMenuCatalogEntry *FrontendExtensionCatalog::find(FrontendMenuKey key) const noexcept {
  const auto it = keys_.find(key);
  return it == keys_.end() ? nullptr : &entries_[it->second];
}
const FrontendMenuCatalogEntry *FrontendExtensionCatalog::find(
    FrontendMenuSource source, std::string_view path) const noexcept {
  // Avoid allocating an owning path merely to perform a heterogeneous lookup.
  // The bounded catalogue contains only the supplied131 PC definitions plus
  // original definitions, and entry addresses remain stable after construction.
  for (const auto &entry : entries_)
    if (entry.key.source == source && entry.path == path)
      return &entry;
  return nullptr;
}
bool FrontendExtensionCatalog::reserved(FrontendMenuKey key) const noexcept {
  return find(key) != nullptr;
}
void FrontendExtensionCatalog::validate_definition(FrontendMenuKey key,
                                                   const UiScreen &screen) const {
  const auto *entry = find(key);
  if (!entry || screen.id != key.id || screen.name != entry->path ||
      screen.panes.size() != entry->pane_count)
    throw FormatError("menu definition differs from source-qualified reservation");
}
std::string_view frontend_vista_menu_catalog_source_sha256() noexcept {
  return "eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44";
}
} // namespace halo2
