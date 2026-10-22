#pragma once
#include "halo2/frontend.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <optional>
#include <string_view>

namespace halo2::detail {
namespace mcc_model_viewport_profile {
// Numeric/name metadata only. MCC source56a52889598d49c2...; original source
// 1b5de78212ee7439... and original2B0BAA/2C560 matching scene cases.
// Reproduction: .local/mcc-model-view-diagnosis-20261019/viewport-profile-v1.
// These are viewport-only projections: camera/FOV/animation/pose stay authored.
struct Profile {
  unsigned screen;
  std::string_view screen_name, object;
  unsigned light_count;
  std::array<std::string_view,3> lights;
  unsigned flags, animation, delay;
  std::array<std::uint32_t,11> source_words; // scale3,camera3,FOV,viewport4.
  std::array<std::int16_t,4> canonical; // Original(top,left,bottom,right).
};
inline constexpr Profile profiles[]{
  {47u, "ui\\screens\\game_shell\\settings_screen\\player_profile\\multiplayer_appearance_settings", "ui_player1c", 1u, {"ui_basic9", "", ""}, 0u, 11u, 250u,
    {0x3ac49ba6u, 0x3ac49ba6u, 0x3ac49ba6u, 0x41accccdu, 0x4261999au, 0xc247851fu, 0x425c0000u, 0x43eb0000u, 0xc41b0000u, 0xc3e10000u, 0x43ff0000u},
    {240, -320, -240, 320}},
  {47u, "ui\\screens\\game_shell\\settings_screen\\player_profile\\multiplayer_appearance_settings", "ui_player1d", 1u, {"ui_basic10", "", ""}, 0u, 11u, 250u,
    {0x3ac49ba6u, 0x3ac49ba6u, 0x3ac49ba6u, 0x41fbd70au, 0x4260cccdu, 0xc247999au, 0x425c0000u, 0x43eb0000u, 0xc42f0000u, 0xc3e10000u, 0x44160000u},
    {240, -320, -240, 320}},
  {30u, "ui\\screens\\game_shell\\4way_multiplayer_signin\\4way_signin_main", "ui_player1", 1u, {"ui_basic1", "", ""}, 0u, 11u, 100u,
    {0x3ba3d70au, 0x3ba3d70au, 0x3ba3d70au, 0xc263f5c3u, 0x4260cccdu, 0xc246cccdu, 0x428c0000u, 0x43d20000u, 0xc3fa0000u, 0x41a00000u, 0x44098000u},
    {240, -320, 0, 320}},
  {30u, "ui\\screens\\game_shell\\4way_multiplayer_signin\\4way_signin_main", "ui_player2", 1u, {"ui_basic3", "", ""}, 0u, 11u, 120u,
    {0x3ba3d70au, 0x3ba3d70au, 0x3ba3d70au, 0xc2193333u, 0x4260cccdu, 0xc246cccdu, 0x428c0000u, 0x43700000u, 0xc3a00000u, 0x00000000u, 0x43a00000u},
    {240, -320, 0, 320}},
  {30u, "ui\\screens\\game_shell\\4way_multiplayer_signin\\4way_signin_main", "ui_player3", 1u, {"ui_basic5", "", ""}, 0u, 11u, 90u,
    {0x3ba3d70au, 0x3ba3d70au, 0x3ba3d70au, 0xc187eb85u, 0x4260cccdu, 0xc2480000u, 0x428c0000u, 0x00000000u, 0xc3a00000u, 0xc3700000u, 0x43a00000u},
    {0, -320, -240, 320}},
  {30u, "ui\\screens\\game_shell\\4way_multiplayer_signin\\4way_signin_main", "ui_player4", 1u, {"ui_basic7", "", ""}, 0u, 11u, 110u,
    {0x3ba3d70au, 0x3ba3d70au, 0x3ba3d70au, 0x3fd9999au, 0x4260cccdu, 0xc2480000u, 0x428c0000u, 0x00000000u, 0xc3a00000u, 0xc3700000u, 0x43a00000u},
    {0, -320, -240, 320}},
  {30u, "ui\\screens\\game_shell\\4way_multiplayer_signin\\4way_signin_main", "ui_player1b", 1u, {"ui_basic2", "", ""}, 0u, 11u, 100u,
    {0x3ba3d70au, 0x3ba3d70au, 0x3ba3d70au, 0xc23c3333u, 0x4260cccdu, 0xc2470000u, 0x428c0000u, 0x43700000u, 0xc3a00000u, 0x00000000u, 0x43a00000u},
    {240, -320, 0, 320}},
  {30u, "ui\\screens\\game_shell\\4way_multiplayer_signin\\4way_signin_main", "ui_player2b", 1u, {"ui_basic4", "", ""}, 0u, 11u, 90u,
    {0x3ba3d70au, 0x3ba3d70au, 0x3ba3d70au, 0xc1e26666u, 0x4260cccdu, 0xc2470000u, 0x428c0000u, 0x43700000u, 0xc3a00000u, 0x00000000u, 0x43a00000u},
    {240, -320, 0, 320}},
  {30u, "ui\\screens\\game_shell\\4way_multiplayer_signin\\4way_signin_main", "ui_player3b", 1u, {"ui_basic6", "", ""}, 0u, 11u, 120u,
    {0x3ba3d70au, 0x3ba3d70au, 0x3ba3d70au, 0xc0e1999au, 0x4260cccdu, 0xc2483333u, 0x428c0000u, 0x00000000u, 0xc3a00000u, 0xc3700000u, 0x43a00000u},
    {0, -320, -240, 320}},
  {30u, "ui\\screens\\game_shell\\4way_multiplayer_signin\\4way_signin_main", "ui_player4b", 1u, {"ui_basic8", "", ""}, 0u, 11u, 110u,
    {0x3ba3d70au, 0x3ba3d70au, 0x3ba3d70au, 0x413b3333u, 0x4260cccdu, 0xc2483333u, 0x428c0000u, 0x00000000u, 0xc3a00000u, 0xc3700000u, 0x43a00000u},
    {0, -320, -240, 320}},
  {182u, "ui\\screens\\game_shell\\xbox_live\\task_progress_dialog\\task_progress_dialog", "halo_ring2", 1u, {"ui_basic1", "", ""}, 0u, 59u, 0u,
    {0x00000000u, 0x00000000u, 0x00000000u, 0xc0b80000u, 0xc18f999au, 0xc1b6147bu, 0x43700000u, 0x421c0000u, 0xc25c0000u, 0xc2ae0000u, 0x425c0000u},
    {39, -55, -87, 55}},
  {193u, "ui\\screens\\game_shell\\profile_picker\\profile_picker", "ui_player1", 1u, {"ui_basic1", "", ""}, 0u, 16u, 50u,
    {0x3b449ba6u, 0x3b449ba6u, 0x3b449ba6u, 0xc26947aeu, 0x4260cccdu, 0xc24778d5u, 0x428c0000u, 0x44020000u, 0xc4174000u, 0xc3fa0000u, 0x44250000u},
    {240, -320, -240, 320}},
  {193u, "ui\\screens\\game_shell\\profile_picker\\profile_picker", "ui_player1b", 3u, {"ui_basic2", "ui_player1_direct", "ui_player1_backlight"}, 0u, 11u, 250u,
    {0x3ac49ba6u, 0x3ac49ba6u, 0x3ac49ba6u, 0xc2411eb8u, 0x42611eb8u, 0xc2478f5cu, 0x428c0000u, 0x43fa0000u, 0xc40fc000u, 0xc3e60000u, 0x441d8000u},
    {240, -320, -240, 320}},
};
} // namespace mcc_model_viewport_profile

// Call on the original immutable content scene. A returned rectangle belongs
// in a private UiScene copy with the existing Xbox viewport route(false).
// Every exposed scene field is qualified; unknown/custom tuples are untouched.
// This does not import an original asset, mutate a cache or choose a shader.
// Matchmaking210 retains its complete PC layout, including the ring viewport.
// Projecting that ring alone into original units separates it from its stages.
inline std::optional<UiRect> frontend_mcc_model_viewport(
    const FrontendContent& content,const UiScreen& screen,const UiScene& scene) {
  if(content.source_fonts.filename()!="h2_fonts" || scene.objects.size()!=1 || scene.lights.empty() || scene.lights.size()>3)
    return {};
  const std::array values{scene.scale.x,scene.scale.y,scene.scale.z,
      scene.camera.x,scene.camera.y,scene.camera.z,scene.field_of_view,
      scene.viewport.top,scene.viewport.left,scene.viewport.bottom,scene.viewport.right};
  std::array<std::uint32_t,11> words{};
  for(unsigned i=0;i<words.size();++i)words[i]=std::bit_cast<std::uint32_t>(values[i]);
  for(const auto& row:mcc_model_viewport_profile::profiles) {
    if(screen.id!=row.screen || screen.name!=row.screen_name ||
        scene.objects.front()!=row.object || scene.lights.size()!=row.light_count ||
        !std::equal(scene.lights.begin(),scene.lights.end(),row.lights.begin()) ||
        scene.flags!=row.flags || scene.animation!=row.animation || scene.delay!=row.delay ||
        words!=row.source_words)continue;
    return UiRect{float(row.canonical[0]),float(row.canonical[1]),
        float(row.canonical[2]),float(row.canonical[3])};
  }
  return {};
}
} // namespace halo2::detail
