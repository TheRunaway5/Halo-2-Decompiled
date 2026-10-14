#define SDL_MAIN_HANDLED
#include "halo2/desktop_session.hpp"
#include "halo2/frontend.hpp"
#include "halo2/renderer.hpp"
#include "halo2/debug_panel.hpp"
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
      check(i < view.frame().entries.size(),
            "source row missing from native menu");
      auto expected = content.vista->localized(content.vista->screens.at(menu), sid);
      if(menu==274 && i>0)expected=i==1?"2x SSAA":"4x SSAA";
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
    state.set_pc_video_adapter([&](unsigned w, unsigned h, bool full) {
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
    choose(state, 0);
    state.back();
    check(applied.back() == std::array<unsigned, 3>{960, 720, 0},
          "Escape did not restore previous display mode");
    state.open(vista_menu(269));
    choose(state, 0);
    state.accept();
    check(state.frame().screen != vista_menu(293),
          "confirmation did not commit mode");
    MenuState reload_video(content, preferences);
    reload_video.set_pc_video_adapter([&](unsigned w, unsigned h, bool full) {
      applied.push_back({w, h, unsigned(full)});
      return std::string{};
    });
    reload_video.set_pc_video_modes({{800, 600}}, 960, 720, false);
    check(applied.back() == std::array<unsigned, 3>{800, 600, 0},
          "confirmed resolution not restored after reload");
    state.set_pc_video_adapter(
        [](unsigned, unsigned, bool) { return "driver rejected mode"; });
    state.open(vista_menu(271));
    choose(state, 0);
    check(state.frame().screen == 8,
          "failed video change was silently accepted");
    state.back();
    state.open(vista_menu(274));
    choose(state, 2);
    check(state.frame().screen == 8,
          "unsupported renderer option was silently accepted");
    state.back();
    DisplaySettings applied_display;
    state.set_pc_display_adapter([&](const DisplaySettings &v){applied_display=v;return std::string{};});
    for(auto [id,row]:std::array<std::pair<unsigned,unsigned>,5>{{{274,1},{290,2},{272,7},{273,4},{285,1}}}) {
      state.open(vista_menu(id));choose(state,row);
      check(state.frame().screen!=8,"native renderer setting was rejected");
    }
    check(applied_display.anti_aliasing==1 && applied_display.detail==2 &&
          applied_display.brightness==7 && applied_display.gamma==4 && applied_display.safe_area,
          "native settings not delivered to renderer adapter");
    state.open(vista_menu(256));choose(state,2);choose(state,1);
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
    check(applied_display.internal_width==0 && applied_display.anti_aliasing==0 &&
          applied_display.brightness==3 && applied_display.gamma==0 && applied_display.detail==0 &&
          !applied_display.safe_area,"native render defaults were not restored");
    Mesh mesh;
    FrontendPainter painter(content, std::filesystem::path(argv[1]) / "fonts",
                            mesh);
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
          leave.frame().entries.size()==5,"Task close did not retain the native main menu");
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
      live.set_pc_video_adapter([&](unsigned w, unsigned h, bool full) {
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
            (SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN),"fullscreen desktop fallback failed");
      check(desktop.apply_video(960,720,false).empty(),"windowed restore after fullscreen failed");
      Renderer renderer(window,mesh,true);
      renderer.debug_panel().share_display_settings(desktop.display_settings);
      renderer.measure_frames(true);
      MenuState gpu_state(content);gpu_state.open(19);
      const auto ui=painter.draw(gpu_state,5.f);
      for(unsigned aa=0;aa<3;++aa) {
        auto &display=desktop.display_settings;
        display.internal_width=800;display.internal_height=600;display.anti_aliasing=aa;
        display.brightness=aa?7:3;display.gamma=aa?2:0;display.detail=aa;
        check(renderer.validate_display_settings(display).empty(),"supported native render settings rejected");
        const auto screenshot=preferences.string()+".aa"+std::to_string(aa)+".ppm";
        check(renderer.draw(Camera{},screenshot,{},false,{},{},ui.vertices),"native offscreen presentation failed");
        check(renderer.draw(Camera{}, {},{},false,{},{},ui.vertices),"reused native offscreen presentation failed");
        const auto timing=renderer.frame_timing();
        check(timing.width==(800u<<aa) && timing.height==(600u<<aa),"SSAA did not change the actual render extent");
        std::ifstream shot(screenshot,std::ios::binary);std::string magic;unsigned width{},height{};
        shot>>magic>>width>>height;
        check(magic=="P6" && width==960 && height==720,"capture used internal extent instead of output extent");
      }
      DisplaySettings excessive;excessive.internal_width=7680;excessive.internal_height=4320;excessive.anti_aliasing=2;
      check(!renderer.validate_display_settings(excessive).empty(),"oversized combined resolution accepted");
      desktop.display_settings={};
      check(renderer.draw(Camera{}, {},{},false,{},{},ui.vertices),"direct swapchain path failed after offscreen teardown");

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
