#define SDL_MAIN_HANDLED
#include "halo2/desktop_session.hpp"
#include "halo2/frontend.hpp"
#include "halo2/renderer.hpp"
#include "halo2/frontend_extension_catalog.hpp"
#include "halo2/multiplayer_kbm.hpp"
#include <SDL.h>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace halo2;
namespace {
unsigned checks{};
void check(bool condition, const char *message) {
  ++checks;
  if (!condition)
    throw std::runtime_error(message);
}
void choose(MenuState &state, unsigned row) {
  state.select(row);
  state.accept();
}
} // namespace
int main(int argc, char **argv) {
  try {
    if ((argc != 5 && argc != 6) ||
        !std::filesystem::is_regular_file(std::filesystem::path(argv[1]) /
                                          "mainmenu.map") ||
        !std::filesystem::is_regular_file(argv[2]))
      return 77;
    Cache cache(std::filesystem::path(argv[1]) / "mainmenu.map");
    auto content = cache.load_frontend();
    const auto originals = content.screens.size();
    const auto original_images = content.images.size();
    content.vista = load_vista_settings(argv[2]);
    // Supplied PC mainmenu scnr kind2 -> matg interface[0]+84 -> wgtz
    // E4D002AE+4 -> wigl E4D102AF+1BC/+1C0. Retained source/payload
    // inventory: analysis/frontend/vista-session-flow-audit/music-binding-20261011/assets.json.
    check(content.vista->music == 0xe53d031b &&
              content.vista->music_silence_ms == 5500,
          "Vista music lost its actual selected shared-UI owner");
    check(content.music != content.vista->music,
          "PC music datum was aliased into the original Xbox content owner");
    // Source-local wgit IDs verified against the supplied PC cache and the
    // extension catalog;44 is the original aim-assist screen E58F036D.
    constexpr std::array<unsigned, 39> selected_ids{
        5, 6, 13, 14, 16, 17, 19, 23, 38, 42, 44, 182, 185, 206, 209, 210, 242, 243, 245,
        253, 256, 257, 258, 263, 267, 268, 269, 271, 272, 273, 274,
        276, 285, 290, 293, 294, 295, 296, 297};
    check(content.vista->screens.size() == selected_ids.size(),
          "Vista settings subset inventory differs");
    for (const auto id : selected_ids)
      check(content.vista->screens.contains(id),
            "Vista settings subset omits a selected source-local screen");
    check(content.screens.size() == originals &&
              content.images.size() == original_images,
          "PC importer mutated original content");
    check(content.screens.at(19).panes.size() == 2,
          "original Settings layout displaced");
    const FrontendExtensionCatalog catalog(content);
    for (const auto &[id, s] : content.vista->screens)
      catalog.validate_definition(
          {FrontendMenuSource::vista, std::uint16_t(id)}, s);
    check(&content.screen(42) != &content.screen(vista_menu(42)),
          "shared sensitivity ID lost source namespace");
    const auto preferences = std::filesystem::path(argv[4]);
    std::filesystem::remove(preferences);
    MenuState state(content, preferences);
    state.accept();
    choose(state, 0);
    state.text("Vista Test");
    state.commit_text();
    check(state.frame().screen == 38, "profile creation missed customization");
    check(state.frame().entries.back().destination == int(vista_menu(258)),
          "keyboard menu unreachable from profile");
    state.open(19);
    choose(state, 2);
    check(state.frame().screen == vista_menu(256),
          "video menu unreachable from Settings");
    std::ifstream source(argv[3]);
    std::string line;
    std::getline(source, line);
    check(line == "H2VISTA_SETTINGS_SOURCE_V1",
          "source oracle fixture missing");
    unsigned oracle_rows = 0;
    while (std::getline(source, line)) {
      std::istringstream fields(line);
      std::string kind, hex;
      unsigned a{}, i{};
      fields >> kind >> a;
      if (kind == "choice")
        fields >> i;
      else
        i = a;
      fields >> hex;
      const auto sid = std::stoul(hex, nullptr, 16);
      unsigned menu = kind == "hub" ? 19 : kind == "audio" ? 257 : kind == "video" ? 256 : kind == "editor" ? 253 : a;
      MenuState view(content);
      view.set_pc_video_modes({{800, 600}, {1024, 768}}, 960, 720, false);
      view.open(vista_menu(menu));
      if(kind=="video" && i>=2)++i; // Native Internal Resolution follows Screen Resolution.
      if(kind=="video")i=i==8?13:i+1; // Native FOV, CRT, split and graphics controls precede Restore Defaults.
      if(menu==274) {
        // The three imported AA string owners remain intact, while the
        // requested native Upscaling submenu has its own choice coverage.
        check(i<3 && sid==0x0e00092bu+i && !content.vista->localized(content.vista->screens.at(menu),sid).empty(),"Original AA string owner changed");
        ++oracle_rows;continue;
      }
      check(i < view.frame().entries.size(),
            "source row missing from native menu");
      auto expected = content.vista->localized(content.vista->screens.at(menu), sid);
      if(kind=="video" && i==1)expected="Fullscreen Mode";
      if(menu==271)expected=i==0?"Fullscreen":"Windowed";
      if(menu==271)i=1-i;
      if(kind=="video" && i==6)expected="Upscaling";
      if (view.frame().entries[i].label != expected)
        std::cerr << "source mismatch menu=" << menu << " row=" << i
                  << " native=" << view.frame().entries[i].label
                  << " source=" << expected << '\n';
      check(view.frame().entries[i].label == expected,
            "native label/order differs from executed original lookup");
      ++oracle_rows;
    }
    check(oracle_rows == 92, "source choice coverage changed");
    state.open(vista_menu(253));
    check(state.frame().entries.size() == 43,
          "advanced editor lost original headers/actions");
    choose(state, 1);
    check(state.capturing_binding(), "Forward did not enter binding capture");
    state.bind_pc_input(SDL_SCANCODE_UP);
    check(state.pc_keyboard_settings().bindings[unsigned(PcAction::Forward)] ==
              SDL_SCANCODE_UP,
          "custom primary binding lost");
    state.auxiliary();
    check(state.capturing_binding(), "secondary capture unavailable");
    state.bind_pc_input(SDL_SCANCODE_I);
    check(state.pc_keyboard_settings().secondary[unsigned(PcAction::Forward)] ==
              SDL_SCANCODE_I,
          "custom secondary binding lost");
    state.accept();
    state.back();
    check(!state.capturing_binding() && state.frame().screen == vista_menu(253),
          "Escape closed editor instead of canceling capture");
    MenuState loaded(content, preferences);
    check(loaded.pc_keyboard_settings() == state.pc_keyboard_settings(),
          "profile bindings did not survive reload");
    MultiplayerKbm input(17, loaded.pc_keyboard_settings());
    SDL_Event key{};
    key.type = SDL_KEYDOWN;
    key.key.windowID = 17;
    key.key.keysym.scancode = SDL_SCANCODE_UP;
    input.handle_event(key);
    check(input.snapshot().controls.forward == 1,
          "custom binding did not reach actual gameplay adapter");
    input.release();
    key.key.keysym.scancode = SDL_SCANCODE_W;
    input.handle_event(key);
    check(input.snapshot().controls.forward == 0,
          "replaced binding remains active");
    input.release();
    key.key.keysym.scancode = SDL_SCANCODE_I;
    input.handle_event(key);
    check(input.snapshot().controls.forward == 1,
          "secondary binding did not reach gameplay adapter");
    state.open(vista_menu(42));
    choose(state, 7);
    check(state.pc_keyboard_settings().sensitivity == 8,
          "sensitivity not applied");
    state.open(vista_menu(294));
    choose(state, 1);
    check(state.pc_keyboard_settings().invert, "mouse inversion not applied");
    state.open(vista_menu(295));
    choose(state, 1);
    check(state.pc_keyboard_settings().invert, "cancel reset changed settings");
    state.open(vista_menu(295));
    choose(state, 0);
    check(state.pc_keyboard_settings() == PcKeyboardSettings{},
          "reset failed to restore native host defaults");
    std::vector<std::array<unsigned, 3>> applied;
    state.set_pc_video_adapter([&](unsigned w, unsigned h, FullscreenMode full) {
      applied.push_back({w, h, unsigned(full)});
      return std::string{};
    });
    state.set_pc_video_modes({{800, 600}, {1024, 768}, {0, 0}, {800, 600}}, 960,
                             720, false);
    state.ui_clock(20000);
    state.open(vista_menu(269));
    choose(state, 0);
    check(applied.back() == std::array<unsigned, 3>{800, 600, 0} &&
              state.frame().screen == vista_menu(293),
          "resolution preview/confirmation missing");
    check(state.frame().description.find("16 seconds") != std::string::npos,
          "source initial countdown differs");
    state.ui_clock(35999);
    check(state.frame().screen == vista_menu(293),
          "source countdown reverted too early");
    state.ui_clock(36000);
    check(state.frame().screen != vista_menu(293) &&
              applied.back() == std::array<unsigned, 3>{960, 720, 0},
          "source timeout did not restore previous resolution");
    state.open(vista_menu(271));
    choose(state, 1);
    state.back();
    check(applied.back() == std::array<unsigned, 3>{960, 720, 0},
          "Escape did not restore previous display mode");
    state.open(vista_menu(269));
    choose(state, 0);
    state.accept();
    check(state.frame().screen != vista_menu(293),
          "confirmation did not commit mode");
    MenuState reload_video(content, preferences);
    reload_video.set_pc_video_adapter([&](unsigned w, unsigned h, FullscreenMode full) {
      applied.push_back({w, h, unsigned(full)});
      return std::string{};
    });
    reload_video.set_pc_video_modes({{800, 600}}, 960, 720, false);
    check(applied.back() == std::array<unsigned, 3>{800, 600, 0},
          "confirmed resolution not restored after reload");
    state.set_pc_video_adapter(
        [](unsigned, unsigned, FullscreenMode) { return "driver rejected mode"; });
    state.open(vista_menu(271));
    choose(state, 1);
    check(state.frame().screen == 8,
          "failed video change was silently accepted");
    state.back();
    state.open(vista_menu(274),1);
    choose(state, 2);
    check(state.frame().screen == 8,
          "unsupported renderer option was silently accepted");
    state.back();
    DisplaySettings applied_display;
    state.set_pc_display_adapter([&](const DisplaySettings &v){applied_display=v;return std::string{};});
    state.open(vista_menu(274),1);choose(state,1);
    for(auto [id,row]:std::array<std::pair<unsigned,unsigned>,4>{{{290,2},{272,7},{273,4},{285,1}}}) {
      state.open(vista_menu(id));choose(state,row);
      check(state.frame().screen!=8,"native renderer setting was rejected");
    }
    check(applied_display.upscaler==Upscaler::FSR31 && applied_display.detail==2 &&
          applied_display.brightness==7 && applied_display.gamma==4 && applied_display.safe_area,
          "native settings not delivered to renderer adapter");
    state.open(vista_menu(256));choose(state,3);choose(state,1);
    check(applied_display.upscaling_quality==UpscalingQuality::UltraQuality && applied_display.internal_width==0,
          "automatic internal resolution did not lead to the active upscaler quality control");
    state.open(vista_menu(274),1);choose(state,0);
    state.open(vista_menu(256));choose(state,3);choose(state,1);
    check(applied_display.internal_width==800 && applied_display.internal_height==600,
          "internal resolution not independent of window mode");
    MenuState display_reload(content,preferences);
    check(display_reload.pc_display_settings()==applied_display,"render preferences did not persist");
    const auto low=native_detail_policy(2),high=native_detail_policy(0),split=native_detail_policy(2,4);
    check(low.ordinary_maximum==3 && low.coverage_multiplier==.8f && low.effect_scale==.3f &&
          high.ordinary_maximum==4 && split.ordinary_maximum==2,"PC detail policy or split-screen cap changed");
    state.open(19);
    check(state.frame().screen==vista_menu(19) && state.frame().entries.size()==6,"Vista settings hub incomplete");
    for(unsigned i=0;i<6;++i) {
      const auto &pane=content.vista->screens.at(19).panes.at(i);
      check(pane.bitmaps.size()==3 && content.vista->images.contains(pane.bitmaps.back().datum),
            "Vista category illustration missing");
    }
    state.open(vista_menu(257));choose(state,0);choose(state,0);
    check(applied_display.volume==0,"audio mute not delivered");
    state.open(vista_menu(276));choose(state,0);
    check(applied_display.internal_width==0 && applied_display.upscaler==Upscaler::Off &&
          applied_display.brightness==3 && applied_display.gamma==0 && applied_display.detail==0 &&
          !applied_display.safe_area,"native render defaults were not restored");
    state.open(vista_menu(256));
    check(state.frame().entries.at(9).label=="Field of View" &&
          state.frame().entries.at(10).label=="Vehicle Field of View" &&
          state.pc_value_label(state.frame(),9)=="78" &&
          state.pc_value_label(state.frame(),10)=="70","independent FOV video rows missing");
    choose(state,9);
    check(state.frame().context==video_fov_context && state.frame().selected==8 &&
          state.frame().entries.front().value==70 && state.frame().entries.back().value==120,
          "FOV range or saved selection differs");
    state.select(50);state.back();
    check(applied_display.field_of_view==78,"canceling FOV changed the camera");
    choose(state,9);choose(state,50);
    check(applied_display.field_of_view==120 && applied_display.vehicle_field_of_view==70 &&
          applied_display.brightness==3,"on-foot FOV changed another setting");
    choose(state,10);choose(state,20);
    check(applied_display.field_of_view==120 && applied_display.vehicle_field_of_view==90,
          "vehicle FOV was not independent");
    MenuState fov_reload(content,preferences);
    check(fov_reload.pc_display_settings()==applied_display,"FOV settings did not survive reload");
    fov_reload.open(vista_menu(256));
    check(fov_reload.pc_value_label(fov_reload.frame(),9)=="120" &&
          fov_reload.pc_value_label(fov_reload.frame(),10)=="90","video menu lost saved FOV settings");
    check(state.frame().entries.at(11).label=="CRT Filter" && state.pc_value_label(state.frame(),11)=="Off",
          "dedicated CRT video toggle missing or initially enabled");
    const auto video_depth=state.stack().size();
    choose(state,11);
    check(state.frame().screen==vista_menu(256) && state.frame().selected==11 &&
          state.stack().size()==video_depth && applied_display.crt_filter &&
          state.pc_value_label(state.frame(),11)=="On" && applied_display.field_of_view==120,
          "CRT did not toggle immediately in Video Settings or changed another setting");
    MenuState crt_reload(content,preferences);
    check(crt_reload.pc_display_settings().crt_filter,"CRT On did not persist");
    choose(state,11);
    check(!applied_display.crt_filter && !MenuState(content,preferences).pc_display_settings().crt_filter,
          "CRT Off did not apply and persist");
    check(state.frame().entries.at(12).label=="2-Player Split Screen" &&
          state.pc_value_label(state.frame(),12)=="Horizontal","horizontal two-player default missing");
    choose(state,12);
    check(state.frame().screen==vista_menu(256) && applied_display.two_player_split==TwoPlayerSplit::Vertical &&
          state.pc_value_label(state.frame(),12)=="Vertical" && !applied_display.crt_filter &&
          applied_display.field_of_view==120,"split selection changed another preference or left Video Settings");
    check(MenuState(content,preferences).pc_display_settings().two_player_split==TwoPlayerSplit::Vertical,
          "vertical two-player layout did not persist");
    choose(state,12);
    check(MenuState(content,preferences).pc_display_settings().two_player_split==TwoPlayerSplit::Horizontal,
          "horizontal two-player layout did not persist");
    check(state.frame().entries.at(0).label=="Graphics Mode" &&
          state.pc_value_label(state.frame(),0)=="Original","Original graphics default missing");
    choose(state,0);
    check(applied_display.graphics_mode==pstencil::GraphicsMode::pStencil &&
          state.pc_value_label(state.frame(),0)=="pStencil" &&
          MenuState(content,preferences).pc_display_settings().graphics_mode==pstencil::GraphicsMode::pStencil,
          "pStencil mode did not apply and persist");
    choose(state,0);
    check(applied_display.graphics_mode==pstencil::GraphicsMode::PathTracing &&
          state.pc_value_label(state.frame(),0)=="Path Tracing" &&
          MenuState(content,preferences).pc_display_settings().graphics_mode==pstencil::GraphicsMode::PathTracing,
          "Path Tracing mode did not apply and persist");
    check(applied_display.path_tracing_samples==1 && applied_display.path_tracing_bounces==1 &&
          state.frame().entries.at(14).label=="Path Tracing Quality",
          "Path Tracing did not expose conservative ray/bounce defaults in Video Settings");
    choose(state,14);
    check(state.frame().context==video_path_tracing_context && state.frame().entries.size()==2,
          "Path Tracing quality submenu missing");
    choose(state,0);
    check(state.frame().context==video_path_tracing_samples_context && state.frame().entries.size()==2,
          "Path Tracing ray cap exposed values beyond 1..2");
    choose(state,1);
    check(applied_display.path_tracing_samples==2 && applied_display.path_tracing_bounces==1 &&
          state.frame().context==video_path_tracing_context &&
          state.frame().entries.at(0).label=="Ray Samples Per Pixel: 2",
          "Path Tracing ray selection changed bounce depth or failed to return");
    choose(state,1);
    check(state.frame().context==video_path_tracing_bounces_context && state.frame().entries.size()==2,
          "Path Tracing bounce cap exposed values beyond 1..2");
    choose(state,1);
    check(applied_display.path_tracing_samples==2 && applied_display.path_tracing_bounces==2 &&
          applied_display.graphics_mode==pstencil::GraphicsMode::PathTracing &&
          MenuState(content,preferences).pc_display_settings()==applied_display,
          "Path Tracing quality did not apply independently and persist");
    state.back();
    choose(state,0);
    check(applied_display.graphics_mode==pstencil::GraphicsMode::Original && state.frame().entries.size()==14 &&
          applied_display.path_tracing_samples==2 && applied_display.path_tracing_bounces==2,
          "Original mode did not restore or inactive RT quality altered graphics mode");
    choose(state,0);choose(state,0);
    check(applied_display.graphics_mode==pstencil::GraphicsMode::PathTracing,
          "video defaults fixture did not select Path Tracing");
    choose(state,12);choose(state,11);choose(state,13);choose(state,0);
    check(applied_display.field_of_view==78 && applied_display.vehicle_field_of_view==70 && !applied_display.crt_filter &&
          applied_display.two_player_split==TwoPlayerSplit::Horizontal &&
          applied_display.graphics_mode==pstencil::GraphicsMode::Original &&
          applied_display.path_tracing_samples==1 && applied_display.path_tracing_bounces==1 &&
          state.frame().entries.size()==14,
          "video defaults did not reset FOV, CRT and split controls");
    {
      MenuState rejected(content);
      rejected.set_pc_display_adapter([](const DisplaySettings &value){return value.crt_filter?"CRT unavailable":std::string{};});
      rejected.open(vista_menu(256));choose(rejected,11);
      check(!rejected.pc_display_settings().crt_filter,"rejected CRT toggle modified preferences");
      MenuState rejected_path(content);
      DisplaySettings rejected_applied;
      rejected_path.set_pc_display_adapter([&](const DisplaySettings &value){
        if(value.graphics_mode!=pstencil::GraphicsMode::PathTracing)rejected_applied=value;
        return value.graphics_mode==pstencil::GraphicsMode::PathTracing?"Path Tracing unavailable":std::string{};
      });
      rejected_path.open(vista_menu(256));choose(rejected_path,0);choose(rejected_path,0);
      check(rejected_path.pc_display_settings().graphics_mode==pstencil::GraphicsMode::Original &&
            rejected_applied.graphics_mode==pstencil::GraphicsMode::Original &&
            rejected_path.frame().description=="Path Tracing unavailable",
            "unavailable Path Tracing trapped the graphics cycle instead of restoring Original");
      const auto invalid=preferences.string()+".invalid-crt";
      {std::ofstream file(invalid);file<<"halo2-native-menu-v1\npc_crt_filter 2\npc_two_player_split 2\npc_graphics_mode 99\n";}
      check(!MenuState(content,invalid).pc_display_settings().crt_filter,"invalid saved CRT value was accepted");
      check(MenuState(content,invalid).pc_display_settings().two_player_split==TwoPlayerSplit::Horizontal,
            "invalid saved split layout was accepted");
      check(MenuState(content,invalid).pc_display_settings().graphics_mode==pstencil::GraphicsMode::Original,
            "invalid saved graphics mode was accepted");
      for(const auto quality:std::array<std::array<unsigned,2>,4>{{{0,1},{1,0},{3,1},{1,3}}}) {
        {std::ofstream file(invalid);file<<"halo2-native-menu-v1\npc_path_tracing "<<quality[0]<<' '<<quality[1]<<'\n';}
        const auto saved=MenuState(content,invalid).pc_display_settings();
        check(saved.path_tracing_samples==1 && saved.path_tracing_bounces==1,
              "Invalid saved ray/bounce cap bypassed conservative defaults");
      }
      {std::ofstream file(invalid);file<<"halo2-native-menu-v1\npc_graphics_mode 2\n";}
      const auto legacy=MenuState(content,invalid).pc_display_settings();
      check(legacy.graphics_mode==pstencil::GraphicsMode::PathTracing &&
            legacy.path_tracing_samples==1 && legacy.path_tracing_bounces==1,
            "Older configuration did not retain conservative RT quality defaults");
      std::filesystem::remove(invalid);
    }
    Mesh mesh;
    FrontendPainter painter(content, std::filesystem::path(argv[1]) / "fonts",
                            mesh);
    for(const auto size:{std::array<unsigned,2>{640,480},{1280,720},{2560,720}}) {
      MenuState mouse(content);
      mouse.set_pc_display_adapter([](const DisplaySettings &){return std::string{};});
      const auto hit_entry=[&](unsigned index) {
        painter.draw(mouse,5.f,-1,10000,size);
        for(int y=0;y<480;y+=2)for(int x=0;x<640;x+=2)
          if(auto hit=painter.pointer_hit(x,y);hit && hit->action==FrontendPointerAction::Entry && hit->index==index)
            return *hit;
        throw std::runtime_error("FOV mouse target missing after list scroll");
      };
      for(unsigned row:{9u,10u}) {
        mouse.open(vista_menu(256));mouse.select(row);
        check(mouse.activate_pointer(hit_entry(row)),"FOV row mouse activation failed");
        check(mouse.frame().context==(row==9?video_fov_context:video_vehicle_fov_context),
              "mouse opened the wrong FOV control");
        mouse.select(50);
        const auto target=hit_entry(50);
        check(mouse.activate_pointer(target),"last FOV choice cannot be clicked");
        check(!mouse.activate_pointer(target),"FOV popup admitted a stale mouse target");
        check((row==9?mouse.pc_display_settings().field_of_view:mouse.pc_display_settings().vehicle_field_of_view)==120,
              "mouse FOV choice was not applied");
      }
      mouse.open(vista_menu(256));mouse.select(11);
      const auto crt_target=hit_entry(11);
      check(mouse.activate_pointer(crt_target) && mouse.pc_display_settings().crt_filter,
            "CRT row mouse activation failed after list scroll");
      check(!mouse.activate_pointer(crt_target) && mouse.pc_display_settings().crt_filter,
            "CRT toggle admitted a stale duplicate click");
      check(mouse.activate_pointer(hit_entry(11)) && !mouse.pc_display_settings().crt_filter,
            "CRT row mouse activation could not disable the filter");
      mouse.select(12);const auto split_target=hit_entry(12);
      check(mouse.activate_pointer(split_target) && mouse.pc_display_settings().two_player_split==TwoPlayerSplit::Vertical,
            "split row mouse activation failed after list scroll");
      check(!mouse.activate_pointer(split_target),"split toggle admitted a stale duplicate click");
      check(mouse.activate_pointer(hit_entry(12)) && mouse.pc_display_settings().two_player_split==TwoPlayerSplit::Horizontal,
            "split row mouse activation could not restore horizontal layout");
    }
    {
      MenuState leave(content);
      leave.open_vista_lobby();
      const auto old_lobby=leave.frame().instance;
      auto owner=std::make_shared<VistaLeaveTaskScreenOwner>();
      const auto task=leave.open_vista_leave_task(0,owner);
      check(owner->instance==task && leave.vista_session_screen()==14 &&
          leave.frame().screen==vista_menu(182),"task replaced the actual session channel");
      leave.accept();leave.back();
      check(leave.frame().instance==task,"uncancellable source task accepted native input");
      leave.set_vista_leave_task_status(task,0x0d000441);
      leave.open_native_mainmenu([]{});
      check(leave.frame().instance==task && leave.vista_session_screen()==6,
          "main-menu factory disposed the independent task allocation");
      const auto task_draw=painter.draw(leave,5.f);
      check(task_draw.scenes.size()==1 && task_draw.scenes.front().definition->objects==std::vector<std::string>{"halo_ring2"},
          "actual PC progress model was omitted by the painter");
      check(task_draw.scenes.front().before_vertex<=task_draw.vertices.size(),"PC model lost its draw ordering boundary");
      leave.close_vista_leave_task(task,3);
      check(leave.frame().screen==6 && leave.frame().instance!=old_lobby &&
          leave.frame().entries.size()==6,"Task close did not retain the native main menu");
      bool rejected{};try{leave.open(vista_menu(13));}catch(const FormatError&){rejected=true;}
      check(rejected && leave.frame().screen==6,"Retired PC13 was reachable through native navigation");

    }
    for (auto id : {19u,257u,263u,267u,268u,242u,296u,297u,258u, 243u, 253u, 256u, 269u, 271u, 272u, 273u, 274u, 276u,
                    285u, 290u, 293u, 294u, 295u}) {
      MenuState view(content);
      view.set_pc_video_modes({{800, 600}, {1024, 768}}, 960, 720, false);
      view.open(vista_menu(id));
      const auto draw = painter.draw(view, 5.f);
      check(!draw.vertices.empty(), "PC menu submitted no UI");
      for (const auto &v : draw.vertices)
        check(std::isfinite(v.position.x) && std::isfinite(v.position.y) &&
                  v.material < mesh.materials.size(),
              "PC draw lost native coordinate/material identity");
    }
    if (argc == 6) {
      check(std::string(argv[5]) == "--live-window", "unknown test option");
      DesktopSession desktop;
      auto *window = desktop.window(960, 720, true);
      MenuState live(content);
      live.set_pc_video_adapter([&](unsigned w, unsigned h, FullscreenMode full) {
        return desktop.apply_video(w, h, full);
      });
      live.set_pc_video_modes({{800, 600}, {1024, 768}}, 960, 720, false);
      auto size_is = [&](int w, int h) {
        int actual_w{}, actual_h{};
        SDL_GetWindowSize(window, &actual_w, &actual_h);
        return actual_w == w && actual_h == h;
      };
      live.open(vista_menu(269));
      choose(live, 0);
      check(size_is(800, 600), "live window did not enter preview resolution");
      live.ui_clock(16000);
      check(size_is(960, 720), "live window did not revert after countdown");
      live.open(vista_menu(269));
      choose(live, 0);
      live.back();
      check(size_is(960, 720), "live Escape rollback failed");
      live.open(vista_menu(269));
      choose(live, 0);
      live.accept();
      check(size_is(800, 600), "live confirmation did not retain resolution");
      check(!desktop.apply_video(0, 0, false).empty() && size_is(800, 600),
            "invalid mode damaged live window");
      check(desktop.apply_video(960, 720, false).empty(),
            "live test could not restore its original window");
      check(desktop.apply_video(960,720,true).empty() &&
            (SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN),"exclusive fullscreen failed");
      check(desktop.apply_video(960,720,false).empty(),"windowed restore after fullscreen failed");
      Renderer renderer(window,mesh,true);
      renderer.share_display_settings(desktop.display_settings);
      renderer.measure_frames(true);
      MenuState gpu_state(content);gpu_state.open(19);
      const auto ui=painter.draw(gpu_state,5.f);
      for(unsigned aa=0;aa<3;++aa) {
        auto &display=desktop.display_settings;
        display.internal_width=800;display.internal_height=600;
        display.upscaler=aa?Upscaler::FSR31:Upscaler::Off;display.upscaling_quality=aa==1?UpscalingQuality::NativeAA:UpscalingQuality::Performance;
        display.brightness=aa?7:3;display.gamma=aa?2:0;display.detail=aa;
        check(renderer.validate_display_settings(display).empty(),"supported native render settings rejected");
        const auto screenshot=preferences.string()+".aa"+std::to_string(aa)+".ppm";
        check(renderer.draw(Camera{},screenshot,{},false,{},{},ui.vertices),"native offscreen presentation failed");
        check(renderer.draw(Camera{}, {},{},false,{},{},ui.vertices),"reused native offscreen presentation failed");
        const auto timing=renderer.frame_timing();
        const auto expected=aa?upscaling_render_size(960,720,display.upscaler,display.upscaling_quality):std::pair{800u,600u};
        check(timing.width==expected.first && timing.height==expected.second,"Upscaling did not change the actual render extent");
        std::ifstream shot(screenshot,std::ios::binary);std::string magic;unsigned width{},height{};
        shot>>magic>>width>>height;
        check(magic=="P6" && width==960 && height==720,"capture used internal extent instead of output extent");
      }
      DisplaySettings excessive;excessive.internal_width=32768;excessive.internal_height=32768;
      check(!renderer.validate_display_settings(excessive).empty(),"oversized combined resolution accepted");
      desktop.display_settings={};
      check(renderer.draw(Camera{}, {},{},false,{},{},ui.vertices),"direct swapchain path failed after offscreen teardown");
      MenuState video_live(content);
      video_live.set_pc_display_adapter([&](const DisplaySettings &value) {
        const auto error=renderer.validate_display_settings(value);
        if(error.empty())desktop.display_settings=value;
        return error;
      });
      video_live.open(vista_menu(256));
      const auto capture_crt=[&](const char *name) {
        const auto path=preferences.string()+name+".ppm";
        check(renderer.draw(Camera{},path,{},false,{},{},ui.vertices),"video CRT capture failed");
        std::ifstream file(path,std::ios::binary);
        return std::vector<char>{std::istreambuf_iterator<char>(file),{}};
      };
      const auto crt_off=capture_crt(".crt-off");
      choose(video_live,11);
      check(desktop.display_settings.crt_filter,"video toggle did not reach the live shared renderer preference");
      const auto crt_on=capture_crt(".crt-on");
      check(crt_on!=crt_off,"video CRT toggle changed no rendered pixels");
      choose(video_live,11);
      check(!desktop.display_settings.crt_filter && capture_crt(".crt-disabled")==crt_off,
            "video CRT Off did not restore exact unfiltered pixels");
    }
    std::filesystem::remove(preferences);
    std::cout << checks
              << " Vista source/import/navigation/binding/video/painter checks "
                 "passed\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
