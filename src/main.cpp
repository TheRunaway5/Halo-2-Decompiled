#include "halo2/custom_slayer.hpp"
#include "halo2/audio.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/source_script_sound_admission_tu15.hpp"
#include "halo2/source_scripted_sound_point_tu15.hpp"
#include "halo2/campaign_audio_timeline.hpp"
#include "halo2/campaign_capture_boundary.hpp"
#include "halo2/cinematic_camera_interpolation.hpp"
#include "halo2/source_unit_speech_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/cache.hpp"
#include "halo2/frontend.hpp"
#include "halo2/frontend_session.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/math.hpp"
#include "halo2/model.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/appearance.hpp"
#include "halo2/world_material.hpp"
#include "halo2/pstencil/campaign_renderer.hpp"
#include "halo2/pstencil/campaign_capture.hpp"
#include "halo2/pstencil/campaign_materials.hpp"
#include "halo2/sky.hpp"
#include "halo2/source_model_transparent_order.hpp"
#include "halo2/renderer.hpp"
#include "halo2/debug_panel.hpp"
#include "halo2/game_pause_menu.hpp"
#include "halo2/campaign_text.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "halo2/training_controls.hpp"
#include "halo2/rumble.hpp"
#include "halo2/multiplayer_kbm.hpp"
#include "halo2/residency.hpp"
#include "halo2/simulation.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/native_action_prompt_hud.hpp"
#include "halo2/native_weapon_hud.hpp"
#include "halo2/campaign_source_runtime.hpp"
#include "halo2/source_campaign_model.hpp"
#include "halo2/source_campaign_weapons.hpp"
#include "halo2/model_node_interpolation.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include "halo2/unit_scope_camera.hpp"
#include "halo2/multiplayer_outcome.hpp"
#include "halo2/native_vista_flow.hpp"
#include "halo2/mcc_loading_inputs.hpp"
#include "halo2/online_postgame.hpp"
#include "halo2/transform.hpp"
#include "halo2/render_interpolation.hpp"
#include "halo2/source_character_shaders.hpp"
#include "halo2/source_object_colors.hpp"
#include "halo2/object_light_query.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_campaign_environment_view_tu15.hpp"
#include "halo2/source_campaign_screen_tu15.hpp"
#include "halo2/menu_world_depth.hpp"
#include "halo2/source_campaign_camera_lens_tu15.hpp"
#include "halo2/source_campaign_camera_tu15.hpp"
#include "halo2/gameplay_fov.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/source_campaign_view_tu15.hpp"
#include "halo2/source_lens_query_frame_tu15.hpp"
#include "halo2/source_lens_draw_frame_tu15.hpp"
#include "halo2/pstencil/campaign.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <algorithm>
#include <chrono>
#include <charconv>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <limits>
#include <memory>
#include <map>
#include <iterator>
#include <optional>
#include <set>
#include <string_view>
#include <unordered_map>

namespace {
void usage() {
  std::cout
      << "Halo 2 native port workbench (subsystem development; incomplete game "
         "port)\n"
      << "  halo2_native [--map PATH] [--bsp N] [--frames N] [--screenshot "
         "PATH.ppm]\n"
      << "               [--overview] [--hidden] [--validation] "
         "[--resize-test] [--diagnostic]\n"
      << "               [--play] [--weapon NAME] [--place-squad NAME] "
         "[--script NAME] [--sound NAME] [--mute]\n"
      << "  halo2_native --inspect PATH\n"
      << "  halo2_native --mcc-preview --map PATH previews MCC map assets without gameplay admission.\n"
      << "  halo2_native --multiplayer [--map PATH] [--local-players 1..4]\n"
      << "  --graphics-mode Original|pStencil selects initial graphics; F1 or Video Settings switches live.\n"
      << "               [--host PORT | --join ADDRESS:PORT] [--dedicated]\n"
      << "               [--custom-slayer fiesta|infection]\n"
      << "               [--mp-starting-weapon 0|5] (map default or authored SMG)\n"
      << "               [--mp-rule PARAM VALUE] [--mp-team 0..7] (repeat per local player)\n"
      << "  --mp-reference-camera PX PY PZ FX FY FZ UX UY UZ replays a captured source view.\n"
      << "  --mp-reference-projection ANGLE SCALE retains its vertical radians and lighting scale.\n"
      << "  --mp-reference-root X Y Z VX VY VZ supplies its root lighting inputs.\n"
      << "  --mp-reference-player PATH replays retained original player/camera/shader inputs.\n"
      << "  --mp-reference-render-fraction 0..1 fixes diagnostic interpolation for finite local tick captures.\n"
      << "  --mp-reference-auxiliary-seed N --mp-reference-tick-cadence controls finite input-script capture RNG and one-frame-per-tick timing.\n"
      << "               [--shared-maps PATH] (shared resources for external maps)\n"
      << "               [--gameplay-xbe PATH] (TU1.5 automatically enables its physics/death world)\n"
      << "  --mp-source-object-world DIRECTORY explicitly selects retained-world savegame storage.\n"
      << "  halo2_native --menu [--menu-screen ID] [--menu-audit]\n"
      << "  --menu-ui-assets DIRECTORY optionally replaces sampled 2D art; original layout and backdrop remain.\n"
      << "  --pause-ui-assets DIRECTORY selects imported menu art/fonts for gameplay pause screens.\n"
      << "  --menu-vista-settings PATH imports the reference Vista mainmenu.map settings, icons and keyboard/video menus.\n"
      << "  Loading images use MCC-owned inputs; unavailable presentation roles are reported separately.\n"
      << "  --hud-ui-assets DIRECTORY samples imported Vista/MCC Classic HUD art/fonts with original layout.\n"
      << "  --skip-intro skips boot playback; --menu-movie intro|attract|credits previews a retail movie.\n"
      << "  --movie-frame N previews a movie frame; --menu-tick N advances the authored flythrough before rendering.\n"
      << "  --freeze-menu holds that camera/animation tick for visual captures.\n"
      << "  --menu-action accept|back|select:N|text:VALUE|commit|wheel-up|wheel-down|click:X:Y repeats a disposable capture route.\n"
      << "  --menu-ui-ms N samples UI animation time in --frames captures only.\n"
      << "  --menu-ui-step-ms N advances that capture clock once per presented frame.\n"
      << "  halo2_native --menu-sweep DIRECTORY renders all authored menu panes and variant dialogs.\n"
      << "  halo2_native --inspect-systems PATH [--bsp N]\n"
      << "  halo2_native --validate-systems DIRECTORY\n"
      << "  halo2_native --map PATH --simulate TICKS [--weapon NAME] "
         "[--place-squad NAME] [--script NAME]\n"
      << "  --campaign activates authored startup/dormant/continuous scripts; "
         "unsupported mission services report faults.\n"
      << "  --bsp-replay TARGET --frames 6 exercises native streaming and checkpoint restore.\n"
      << "  --loop-sound NAME plays an authored looping-sound track.\n"
      << "  --camera-animation GRAPH TRACK FLAG_INDEX previews an authored "
         "camera track.\n"
      << "  --first-person-pose ACTION FRAME freezes an authored weapon pose; "
         "--viewport WIDTH HEIGHT sets its capture size.\n"
      << "  halo2_native --map PATH --decode-sound NAME --audio-output "
         "PATH.wav\n"
      << "  halo2_native --validate-maps DIRECTORY\n"
      << "WASD move, mouse look (click to capture), Q/E down/up, Shift "
         "faster,\n"
      << "Tab next player start, R overview, Escape release mouse or quit.\n"
      << "With --play: Space jump, Shift crouch/sneak, mouse fire, R reload; F2 "
         "toggles free flight.\n";
}
halo2::Camera overview(const halo2::Mesh &mesh) {
  const auto a = mesh.minimum, b = mesh.maximum;
  const float size = std::max({b.x - a.x, b.y - a.y, b.z - a.z, 1.f});
  return {{(a.x + b.x) * 0.5f + size * 0.8f, (a.y + b.y) * 0.5f - size * 0.8f,
           (a.z + b.z) * 0.5f + size * 0.65f},
          2.35619449f,
          -0.522f};
}
int validate_maps(const std::filesystem::path &directory) {
  std::vector<std::filesystem::path> paths;
  for (const auto &file : std::filesystem::directory_iterator(directory))
    if (file.is_regular_file() && file.path().extension() == ".map")
      paths.push_back(file.path());
  std::sort(paths.begin(), paths.end());
  if (paths.empty())
    throw halo2::FormatError("no .map files found");
  std::size_t failures = 0, bsps = 0, triangles = 0;
  for (const auto &path : paths) {
    try {
      halo2::Cache cache(path);
      std::cout << cache.describe() << '\n';
      for (std::size_t i = 0; i < cache.bsps().size(); ++i) {
        const auto mesh = cache.load_bsp(i);
        if (mesh.vertices.empty())
          throw halo2::FormatError("BSP has no renderable geometry");
        ++bsps;
        triangles += mesh.vertices.size() / 3;
        for (const auto &warning : mesh.material_warnings)
          std::cout << "  Rendering gap: " << warning << '\n';
        std::cout << "  BSP " << i << ": " << mesh.clusters << " clusters, "
                  << mesh.instances << " instances, "
                  << mesh.vertices.size() / 3 << " triangles\n";
      }
    } catch (const std::exception &e) {
      ++failures;
      std::cerr << path.filename().string() << ": " << e.what() << '\n';
    }
  }
  std::cout << "Validated " << paths.size() << " maps, " << bsps << " BSPs, "
            << triangles << " triangles; failures=" << failures << '\n';
  return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
} // namespace

int main(int argc, char **argv) {
  try {
    using Clock = std::chrono::steady_clock;
    const auto campaign_application_started=Clock::now();
    std::ofstream campaign_weapon_gpu_trace;
    if(const auto path=std::getenv("HALO2_TEST_CAMPAIGN_WEAPON_GPU_REPORT")) {
      campaign_weapon_gpu_trace.open(path);
      if(!campaign_weapon_gpu_trace)throw halo2::FormatError("Campaign weapon draw report could not be opened");
    }
    const auto trace_campaign_weapon=[&](nlohmann::json record) {
      if(!campaign_weapon_gpu_trace.is_open())return;
      record["application_elapsed_seconds"]=std::chrono::duration<double>(Clock::now()-campaign_application_started).count();
      campaign_weapon_gpu_trace<<record.dump()<<'\n';campaign_weapon_gpu_trace.flush();
    };
    if(campaign_weapon_gpu_trace.is_open())trace_campaign_weapon({{"event","timing"},{"stage","application_start"}});
    std::filesystem::path map = "assets/disc/maps/lockout.map", screenshot;
    std::optional<std::filesystem::path> inspect, validate, inspect_systems,
        validate_systems;
    std::string weapon_name, squad_name, script_name, sound_name,
        play_sound_name, loop_sound_name, camera_graph, camera_track;
    int camera_anchor = -1;
    std::filesystem::path audio_output;
    unsigned simulate = 0;
    std::optional<unsigned> bsp_replay;
    bool play = false, mute = false, campaign = false, debug_panel = false;
    bool menu = argc == 1;
    unsigned menu_difficulty=1;
    halo2::FrontendOptions frontend;
    halo2::MultiplayerOptions multiplayer_options;
    const auto desktop = std::make_shared<halo2::DesktopSession>();
    bool multiplayer=false,mcc_preview=false,map_supplied=false;
    std::size_t bsp = 0;
    unsigned frame_limit = 0;
    std::string first_person_action;
    unsigned first_person_frame = 0, viewport_width = 1280, viewport_height = 720;
    bool hidden = false, validation = false, use_overview = false,
         resize_test = false, diagnostic = false, viewport_requested = false;
    for (int i = 1; i < argc; ++i) {
      const std::string arg(argv[i]);
      auto next = [&]() -> std::string {
        if (++i >= argc)
          throw std::runtime_error("missing value for " + arg);
        return argv[i];
      };
      auto number = [&]() {
        const auto value = next();
        std::size_t end{};
        const auto parsed = std::stoul(value, &end);
        if (value.empty() || value[0] == '-' || end != value.size() ||
            parsed > 1000000)
          throw std::runtime_error("invalid value for " + arg);
        return static_cast<unsigned>(parsed);
      };
      if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      }
      if(arg=="--mcc-preview")mcc_preview=true;
      else if(arg=="--graphics-mode") {
        const auto mode=next();
        if(mode=="Original")frontend.initial_graphics_mode=halo2::pstencil::GraphicsMode::Original;
        else if(mode=="pStencil")frontend.initial_graphics_mode=halo2::pstencil::GraphicsMode::pStencil;
        else throw std::runtime_error("graphics mode must be Original or pStencil");
        desktop->display_settings.graphics_mode=*frontend.initial_graphics_mode;
      }
      else if (arg == "--map") {
        map = next();map_supplied=true;
      }
      else if (arg == "--shared-maps")
        multiplayer_options.shared_maps = next();
      else if (arg == "--gameplay-xbe")
        multiplayer_options.gameplay_xbe = next();
      else if (arg == "--mp-source-object-world") {
        multiplayer_options.source_object_world=true;
        multiplayer_options.source_object_savegame_directory=next();
      }
      else if (arg == "--hud-ui-assets")
        multiplayer_options.ui_assets = next();
      else if (arg == "--pause-ui-assets")
        multiplayer_options.menu_ui_assets = next();
      else if (arg == "--menu")
        menu = true;
      else if (arg == "--menu-ui-assets") {
        frontend.ui_assets = next();
        menu = true;
      }
      else if (arg == "--menu-vista-settings") {
        frontend.vista_settings = next();
        menu = true;
      }
      else if (arg == "--menu-loading-assets") {
        throw std::runtime_error("Executable loading banks are offline/reference inputs; production loading uses MCC-owned assets");
      }
      else if (arg == "--skip-intro")
        frontend.skip_intro = menu = true;
      else if (arg == "--menu-movie") {
        frontend.movie = next();
        if (frontend.movie != "intro" && frontend.movie != "attract" && frontend.movie != "credits")
          throw std::runtime_error("unknown retail movie");
        menu = true;
      } else if (arg == "--menu-tick") {
        frontend.scene_tick = number();
        menu = true;
      } else if (arg == "--freeze-menu") {
        frontend.freeze_scene = menu = true;
      } else if (arg == "--menu-ui-ms") {
        frontend.capture_ui_ms = number();
        menu = true;
      } else if (arg == "--menu-ui-step-ms") {
        frontend.capture_ui_step_ms = number();
        menu = true;
      } else if (arg == "--menu-action") {
        const auto value = next();
        halo2::FrontendCaptureAction action;
        using Kind = halo2::FrontendCaptureAction::Kind;
        if (value == "accept") action.kind = Kind::Accept;
        else if (value == "back") action.kind = Kind::Back;
        else if (value == "commit") action.kind = Kind::Commit;
        else if (value == "wheel-up") action.kind = Kind::WheelUp;
        else if (value == "wheel-down") action.kind = Kind::WheelDown;
        else if (value.starts_with("click:")) {
          action.kind = Kind::Click;
          const auto coordinates = std::string_view(value).substr(6);
          const auto separator = coordinates.find(':');
          if (separator == std::string_view::npos) throw std::runtime_error("menu click requires X:Y window coordinates");
          auto parse = [](std::string_view digits, int &coordinate) {
            const auto result = std::from_chars(digits.data(), digits.data() + digits.size(), coordinate);
            if (result.ec != std::errc{} || result.ptr != digits.data() + digits.size() ||
                coordinate < 0 || coordinate > 16384)
              throw std::runtime_error("invalid menu click coordinate");
          };
          parse(coordinates.substr(0, separator), action.x);
          parse(coordinates.substr(separator + 1), action.y);
        }
        else if (value.starts_with("text:")) {
          action.kind = Kind::Text;
          action.text = value.substr(5);
        } else if (value.starts_with("select:")) {
          action.kind = Kind::Select;
          const auto digits = std::string_view(value).substr(7);
          const auto parsed = std::from_chars(digits.data(), digits.data() + digits.size(), action.index);
          if (parsed.ec != std::errc{} || parsed.ptr != digits.data() + digits.size() ||
              action.index > 1000000)
            throw std::runtime_error("invalid menu capture selection");
        } else throw std::runtime_error("unknown menu capture action");
        frontend.capture_actions.push_back(std::move(action));
        menu = true;
      } else if (arg == "--movie-frame") {
        frontend.movie_frame = number();
        menu = true;
      }
      else if (arg == "--menu-screen") {
        frontend.screen = number();
        menu = true;
      } else if (arg == "--menu-audit") {
        frontend.audit = menu = true;
      } else if (arg == "--menu-sweep") {
        frontend.sweep = next();
        menu = true;
      } else if (arg == "--menu-maps") {
        frontend.maps = next();
        menu = true;
      }
      else if (arg == "--inspect")
        inspect = next();
      else if (arg == "--validate-maps")
        validate = next();
      else if (arg == "--inspect-systems")
        inspect_systems = next();
      else if (arg == "--validate-systems")
        validate_systems = next();
      else if (arg == "--simulate")
        simulate = number();
      else if (arg == "--bsp-replay") {
        bsp_replay = number(); play=true;
      }
      else if (arg == "--play")
        play = true;
      else if (arg == "--debug-panel")
        debug_panel = true;
      else if (arg == "--campaign")
        campaign = play = true;
      else if (arg == "--multiplayer") multiplayer=true;
      else if(arg=="--custom-slayer") {
        const auto requested=next();const auto variants=halo2::bundled_slayer_variants();
        const auto found=std::find_if(variants.begin(),variants.end(),[&](const auto &v){return v.name==requested || (requested=="fiesta" && halo2::custom_slayer(v)==halo2::CustomSlayer::fiesta) || (requested=="infection" && halo2::custom_slayer(v)==halo2::CustomSlayer::infection);});
        if(found==variants.end())throw std::runtime_error("Bundled custom Slayer variant unavailable");
        multiplayer_options.variant=*found;multiplayer=true;
      }
      else if (arg == "--mp-starting-weapon") {multiplayer_options.starting_weapon=number();multiplayer=true;}
      else if (arg == "--mp-rule") {
        const auto parameter=number();
        const auto value=next();std::size_t end{};const auto parsed=std::stoll(value,&end);
        if(parameter>=112 || end!=value.size() || parsed<std::numeric_limits<std::int32_t>::min() ||
           parsed>std::numeric_limits<std::int32_t>::max())
          throw std::runtime_error("--mp-rule requires a parameter 0..111 and signed32-bit value");
        multiplayer_options.variant.values.append(parameter,std::int32_t(parsed));multiplayer=true;
      }
      else if (arg == "--mp-team") {
        const auto team=number();
        if(team>7 || multiplayer_options.selected_teams.size()==4)
          throw std::runtime_error("--mp-team requires one team 0..7 per local player, at most four");
        multiplayer_options.selected_teams.push_back(std::int8_t(team));multiplayer=true;
      }
      else if (arg == "--mp-reference-player") {multiplayer_options.reference_player=next();multiplayer=true;}
      else if(arg=="--mp-reference-projection") {
        const auto real=[&](){const auto value=next();std::size_t end{};const float v=std::stof(value,&end);
          if(end!=value.size() || !std::isfinite(v))throw std::runtime_error("invalid reference projection float");return v;};
        multiplayer_options.capture_projection=halo2::Vec2{real(),real()};multiplayer=true;
      }
      else if(arg=="--mp-reference-camera" || arg=="--mp-reference-root") {
        const auto real=[&]() {
          const auto value=next();std::size_t end{};const float v=std::stof(value,&end);
          if(end!=value.size() || !std::isfinite(v))throw std::runtime_error("invalid reference float for "+arg);
          return v;
        };
        const auto vector=[&]() {return halo2::Vec3{real(),real(),real()};};
        if(arg=="--mp-reference-camera") {
          halo2::Camera view;view.position=vector();view.forward=vector();view.up=vector();view.oriented=true;
          multiplayer_options.capture_camera=view;
        } else multiplayer_options.capture_root=std::array{vector(),vector()};
        multiplayer=true;
      }
      else if (arg == "--host") {multiplayer_options.host_port=number();multiplayer=true;}
      else if (arg == "--join") {multiplayer_options.join=next();multiplayer=true;}
      else if (arg == "--local-players") {multiplayer_options.local_players=number();multiplayer=true;}
      else if (arg == "--dedicated") {multiplayer_options.dedicated=multiplayer=true;multiplayer_options.local_players=0;}
      else if (arg == "--mp-replay") {multiplayer_options.replay=multiplayer=true;}
      else if (arg == "--mp-input-script") {multiplayer_options.input_script=next(); multiplayer=true;}
      else if (arg == "--mp-capture-tick") {multiplayer_options.capture_tick=number(); multiplayer=true;}
      else if(arg=="--mp-reference-auxiliary-seed") {
        const auto value=next();std::uint32_t seed{};
        const auto parsed=std::from_chars(value.data(),value.data()+value.size(),seed);
        if(parsed.ec!=std::errc{} || parsed.ptr!=value.data()+value.size())
          throw std::runtime_error("invalid reference auxiliary seed");
        multiplayer_options.reference_auxiliary_seed=seed;multiplayer=true;
      }
      else if(arg=="--mp-reference-tick-cadence") {multiplayer_options.reference_tick_cadence=true;multiplayer=true;}
      else if (arg == "--mp-reference-render-fraction") {
        const auto value=next();std::size_t end{};const float fraction=std::stof(value,&end);
        if(end!=value.size() || !std::isfinite(fraction) || fraction<0 || fraction>1)
          throw std::runtime_error("invalid reference render fraction");
        multiplayer_options.reference_render_fraction=fraction;multiplayer=true;
      }
      else if (arg == "--mp-capture-players") {multiplayer_options.capture_players=number();multiplayer=true;}
      else if (arg == "--mute")
        mute = true;
      else if (arg == "--weapon")
        weapon_name = next();
      else if (arg == "--first-person-pose") {
        first_person_action = next();
        first_person_frame = number();
        play = true;
      } else if (arg == "--viewport") {
        viewport_requested = true;
        viewport_width = number();
        viewport_height = number();
        if (viewport_width < 64 || viewport_width > 4096 ||
            viewport_height < 64 || viewport_height > 4096)
          throw std::runtime_error("viewport dimensions must be 64..4096");
      }
      else if (arg == "--place-squad")
        squad_name = next();
      else if (arg == "--script")
        script_name = next();
      else if (arg == "--camera-animation") {
        camera_graph = next();
        camera_track = next();
        camera_anchor = int(number());
        play = true;
      } else if (arg == "--decode-sound")
        sound_name = next();
      else if (arg == "--sound")
        play_sound_name = next();
      else if (arg == "--loop-sound")
        loop_sound_name = next();
      else if (arg == "--audio-output")
        audio_output = next();
      else if (arg == "--bsp")
        bsp = number();
      else if (arg == "--frames")
        frame_limit = number();
      else if (arg == "--screenshot")
        screenshot = next();
      else if (arg == "--hidden")
        hidden = true;
      else if (arg == "--validation")
        validation = true;
      else if (arg == "--overview")
        use_overview = true;
      else if (arg == "--resize-test")
        resize_test = true;
      else if (arg == "--diagnostic")
        diagnostic = true;
      else
        throw std::runtime_error("unknown option: " + arg);
    }
    if(mcc_preview && !map_supplied)
      throw halo2::FormatError("MCC content preview requires --map with an imported MCC map");
    if(mcc_preview && (menu || play || campaign || multiplayer || simulate || inspect || validate ||
        inspect_systems || validate_systems || bsp_replay || !weapon_name.empty() || !squad_name.empty() ||
        !script_name.empty() || !camera_graph.empty() || !first_person_action.empty() || !sound_name.empty() ||
        !play_sound_name.empty() || !loop_sound_name.empty()))
      throw halo2::FormatError("MCC content preview is an asset viewer; native gameplay conversion is still incomplete");
    if (!first_person_action.empty() && menu)
      throw std::runtime_error("first person pose fixtures require a level scene");
    if (validate_systems) {
      std::size_t maps = 0, bsps = 0, errors = 0;
      for (const auto &file :
           std::filesystem::directory_iterator(*validate_systems))
        if (file.path().extension() == ".map") {
          try {
            halo2::Cache cache(file.path());
            ++maps;
            for (std::size_t b = 0; b < cache.bsps().size(); ++b) {
              const auto content = cache.load_content(b);
              halo2::ScriptVm vm(content.scripts,
                                 [](const auto &, const auto &) {
                                   return halo2::ScriptValue{};
                                 });
              const auto audit = vm.audit();
              std::cout << cache.name() << " bsp=" << b << " "
                        << content.describe()
                        << " active_nodes=" << audit.active
                        << " builtins=" << audit.builtins.size() << '\n';
              if (!content.limitations.empty())
                throw halo2::FormatError(content.limitations.front());
              ++bsps;
            }
          } catch (const std::exception &e) {
            ++errors;
            std::cerr << file.path().filename() << ": " << e.what() << '\n';
          }
        }
      std::cout << "Systems maps=" << maps << " BSPs=" << bsps
                << " failures=" << errors << '\n';
      return errors || !maps ? 1 : 0;
    }
    if (inspect_systems) {
      halo2::Cache cache(*inspect_systems);
      const auto content = cache.load_content(bsp);
      halo2::ScriptVm vm(content.scripts, [](const auto &, const auto &) {
        return halo2::ScriptValue{};
      });
      const auto audit = vm.audit();
      std::cout << cache.describe() << '\n' << content.describe() << '\n';
      std::cout << "Player height=" << content.player.height
                << " radius=" << content.player.radius
                << " jump=" << content.player.jump_velocity
                << " speed=" << content.movement.forward << '\n';
      for (const auto &s : content.squads)
        std::cout << "Squad: " << s.name << " count=" << s.count
                  << " starts=" << s.starts.size() << " team=" << s.team
                  << " bsp=" << s.bsp << '\n';
      for (const auto &s : content.scripts.scripts)
        std::cout << "Script: " << s.name << " type=" << s.type << '\n';
      for (const auto &[name, count] : audit.builtins)
        std::cout << "Builtin: " << name << " calls=" << count << '\n';
      for (const auto &warning : content.limitations)
        std::cerr << warning << '\n';
      return content.limitations.empty() ? 0 : 1;
    }
    if (validate)
      return validate_maps(*validate);
    if (inspect) {
      std::cout << halo2::Cache(*inspect).describe() << '\n';
      return 0;
    }
    if (!screenshot.empty() && frame_limit == 0 && !multiplayer_options.capture_tick)
      frame_limit = 3;
    if (menu) {
      // MCC supplies five checked map-owned images. Keep those pixels owned
      // across the menu/game handoff, without reading a Vista executable bank.
      // The full existing draw stream also requires a separately authored mask;
      // do not invent that texture, omit its draw, or substitute loading audio.
      std::optional<halo2::MccLoadingBitmapInputs> mcc_loading_inputs;
      {
        const halo2::Cache loading_menu(frontend.maps / "mainmenu.map");
        if (loading_menu.is_mcc()) {
          try {
            mcc_loading_inputs=halo2::load_mcc_loading_bitmap_inputs(loading_menu);
            std::cout << "MCC loading: five owned bitmap inputs prepared; full presentation "
                         "awaits a verified logo mask and ambience producer\n";
          } catch (const halo2::FormatError& error) {
            std::cerr << "MCC loading presentation unavailable: " << error.what() << '\n';
          }
        }
      }
      // Retain configuration/content; actual source lifecycle and UI factories
      // own each subsequent screen, never an implicit old-stack resume.
      frontend.session = std::make_shared<halo2::FrontendSession>();
      frontend.debug_panel=debug_panel;
      frontend.desktop = desktop;
      frontend.frames = frame_limit;
      frontend.screenshot = screenshot;
      frontend.hidden = hidden;
      frontend.validation = validation;
      frontend.mute = mute;
      frontend.resize_test = resize_test;
      if(viewport_requested) {
        frontend.width=viewport_width;frontend.height=viewport_height;
      }
      std::unique_ptr<halo2::NativeVistaOfflineFlow> vista_flow;
      std::string online_return_message;
      auto runtime_defaults=multiplayer_options;
      runtime_defaults.debug_panel=debug_panel;runtime_defaults.desktop=desktop;
      runtime_defaults.bsp=bsp;runtime_defaults.frames=frame_limit;runtime_defaults.ticks=simulate;
      runtime_defaults.screenshot=screenshot;runtime_defaults.width=viewport_width;
      runtime_defaults.height=viewport_height;runtime_defaults.hidden=hidden;
      runtime_defaults.validation=validation;runtime_defaults.mute=mute;
      runtime_defaults.menu_audio_update=[session=frontend.session]{session->world_audio_update();};
      frontend.session_update=[&](halo2::MenuState& state,std::uint32_t clock,
          const std::function<void(halo2::FrontendUiSound)>& sound) {
        if(!online_return_message.empty()) {
          state.return_online(std::move(online_return_message));online_return_message.clear();
        }
        if(!vista_flow && !frame_limit && !simulate && state.launch &&
            frontend.session->content().vista &&
            halo2::NativeVistaOfflineFlow::supports(*state.launch))
          vista_flow=std::make_unique<halo2::NativeVistaOfflineFlow>(*frontend.session,frontend.maps,
              runtime_defaults,[&]{desktop->begin_loading();});
        if(vista_flow) {
          vista_flow->update_frontend(state,clock,sound);
          // A disposed match session must not observe later title/profile
          // screens or service another Start. The next request gets a fresh owner.
          if(vista_flow->finished())vista_flow.reset();
        }
        return vista_flow && vista_flow->launch_ready();
      };
      frontend.session_mainmenu_ready=[&](halo2::MenuState&) {
        if(!vista_flow)throw halo2::FormatError("Native mainmenu return lost its real Vista session");
        vista_flow->mainmenu_loaded();
      };
      std::optional<halo2::FrontendLaunch> launch;
      // Keep an owning validated recovery cut across runtime teardown. It is
      // separate from PCR/display state and needs a certified host decision
      // before a later continuation may consume it.
      std::shared_ptr<const halo2::SourceAuthorityRetainedWorld> online_recovery;
      for(;;) {
        launch=halo2::run_frontend(frontend);
        if(launch && launch->online_match) {
          // Consume the frontend request. Its retained MenuState must not keep
          // a departed room alive while the next menu world loads.
          frontend.session->state().launch.reset();
          vista_flow.reset();
          auto runtime=runtime_defaults;
          runtime.map=launch->map;runtime.local_players=launch->local_players;
          runtime.variant=launch->variant;runtime.selected_profiles=launch->selected_profiles;
          runtime.selected_teams=launch->selected_teams;runtime.input_instances=launch->input_instances;runtime.online_match=launch->online_match;
          runtime.match_voice=launch->match_voice;
          halo2::MultiplayerRuntimeOutcome outcome;
          const auto result=halo2::run_multiplayer(runtime,outcome,online_recovery);
          if(result || outcome.reason==halo2::MultiplayerExitReason::window_quit || frame_limit || simulate)return result;
          if(outcome.reason==halo2::MultiplayerExitReason::online_postgame) {
            frontend.postgame_report=halo2::online_postgame_screen(*frontend.session,launch->online_match,
                [&]{frontend.session->state().return_online();});
            frontend.entry=halo2::FrontendOptions::Entry::postgame_report;
          } else {
            if(outcome.reason==halo2::MultiplayerExitReason::online_disconnected) {
              online_return_message=launch->online_match->error();
              if(online_return_message.empty())online_return_message="Match connection failed. Search again.";
            }
            frontend.postgame_report.reset();frontend.entry=halo2::FrontendOptions::Entry::online_return;
          }
          launch.reset();runtime.online_match.reset();frontend.skip_intro=true;continue;
        }
        if(!vista_flow || !vista_flow->launch_ready())break;
        launch=vista_flow->take_launch();
        online_recovery.reset();
        auto runtime=runtime_defaults;
        runtime.map=launch->map;runtime.local_players=launch->local_players;
        runtime.variant=launch->variant;runtime.selected_profiles=launch->selected_profiles;
        runtime.selected_teams=launch->selected_teams;runtime.input_instances=launch->input_instances;
        runtime.prepared_world=vista_flow->prepared_world();
        runtime.session_random_seed=runtime.prepared_world->rules.random_seed;
        runtime.session_world_loaded=[&](const auto& match,auto slots){vista_flow->world_loaded(match,slots);};
        runtime.session_world_update=[&](const auto& match){return vista_flow->world_update(match);};
        runtime.session_view_black=[&](unsigned view){return vista_flow->world_view_black(view);};
        runtime.session_audio_update=[&](float volume,const auto& gains){vista_flow->world_audio_update(volume,gains);};
        halo2::MultiplayerRuntimeOutcome outcome;
        const auto result=halo2::run_multiplayer(runtime,outcome);
        if(result || outcome.reason!=halo2::MultiplayerExitReason::session_world_close)return result;
        runtime.prepared_world.reset();vista_flow->world_unloaded();
        frontend.entry=halo2::FrontendOptions::Entry::session_return;
        frontend.skip_intro=true;
      }
      if (!launch) return 0;
      map = launch->map;
      campaign = launch->campaign;
      menu_difficulty=launch->difficulty;
      if(launch->multiplayer) {
        multiplayer=true;multiplayer_options.local_players=launch->local_players;
        multiplayer_options.host_port=launch->host_port;multiplayer_options.variant=launch->variant;
        multiplayer_options.selected_teams=launch->selected_teams;
        multiplayer_options.selected_profiles=launch->selected_profiles;
        multiplayer_options.input_instances=launch->input_instances;
        multiplayer_options.online_match=launch->online_match;
        multiplayer_options.match_voice=launch->match_voice;
      }
      play = true;
    }
    if(multiplayer) {
      multiplayer_options.debug_panel=debug_panel;
      multiplayer_options.desktop = desktop;
      if(campaign)throw std::runtime_error("Choose either campaign or multiplayer");
      multiplayer_options.map=map;multiplayer_options.bsp=bsp;multiplayer_options.frames=frame_limit;
      multiplayer_options.ticks=simulate;multiplayer_options.screenshot=screenshot;
      multiplayer_options.width=viewport_width;multiplayer_options.height=viewport_height;
      multiplayer_options.hidden=hidden;multiplayer_options.validation=validation;multiplayer_options.mute=mute;
      // Own the actual returned native banks outside the world/renderer
      // lifetime. Their source identity/order still needs the Vista PCR owner;
      // retaining them alone does not admit a menu or lifecycle transition.
      halo2::MultiplayerRuntimeOutcome outcome;
      return halo2::run_multiplayer(multiplayer_options, outcome);
    }
    if(campaign_weapon_gpu_trace.is_open())trace_campaign_weapon({{"event","timing"},{"stage","cache_begin"}});
    const halo2::Cache cache(map,multiplayer_options.shared_maps,
        multiplayer_options.gameplay_xbe.empty()?std::shared_ptr<const halo2::OriginalData>{}:
            halo2::OriginalData::load_xbe(multiplayer_options.gameplay_xbe));
    if(campaign_weapon_gpu_trace.is_open())trace_campaign_weapon({{"event","timing"},{"stage","cache_end"}});
    if(mcc_preview && !cache.is_mcc())
      throw halo2::FormatError("MCC content preview requires an imported Halo MCC map");
    if (!sound_name.empty()) {
      if (audio_output.empty())
        throw std::runtime_error("--decode-sound requires --audio-output");
      const halo2::Tag *match = nullptr;
      for (const auto &t : cache.select_tags())
        if (t.group == "snd!" && t.name.find(sound_name) != std::string::npos) {
          if (t.name == sound_name) {
            match = &t;
            break;
          }
          if (match)
            throw std::runtime_error(
                "sound name is ambiguous; use its complete tag name");
          match = &t;
        }
      if (!match)
        throw std::runtime_error("sound tag not found");
      const auto encoded = cache.load_sound(match->datum);
      const auto pcm = halo2::decode_sound(encoded);
      halo2::write_wav(pcm, audio_output);
      std::cout << "Decoded " << pcm.name << ": " << pcm.frames()
                << " stereo frames at " << pcm.rate << " Hz from codec "
                << encoded.codec << '\n';
      return 0;
    }
    halo2::NativeResidency residency(cache);
    std::optional<halo2::LevelContent> content;
    std::unique_ptr<halo2::Simulation> simulation;
    std::uint32_t scripted_audio_random=multiplayer_options.session_random_seed.value_or(0x12345678u);
    std::unique_ptr<halo2::MultiplayerAudio> scripted_audio;
    std::map<halo2::MultiplayerAudio::Voice,halo2::MultiplayerAudioSubmission> scripted_speech;
    std::map<halo2::MultiplayerAudio::Voice,std::int32_t> traced_scripted_voice_volumes;
    const auto campaign_source=[&](std::size_t selected) -> std::shared_ptr<halo2::CampaignSourceRuntime> {
      if(!play || !content || content->multiplayer_spawn ||
          cache.original_data()->image!=halo2::OriginalData::Image::TitleUpdate15)return {};
      if(cache.is_mcc())return std::make_shared<halo2::CampaignSourceRuntime>(cache,
          selected,menu_difficulty,multiplayer_options.session_random_seed.value_or(0x12345678u));
      return std::make_shared<halo2::CampaignSourceRuntime>(cache,cache.original_data()->source,
          selected,menu_difficulty,multiplayer_options.session_random_seed.value_or(0x12345678u));
    };
    std::optional<halo2::CampaignTextContent> campaign_text_content;
    const auto bind_cinematic_text=[&] {
      if(!simulation)return;
      if(!campaign_text_content)campaign_text_content=halo2::load_campaign_text(cache,map);
      simulation->training_definitions(campaign_text_content->training_events);
      simulation->cinematic_text_services([&](std::uint32_t id){return campaign_text_content->subtitles.contains(id);},[&](std::uint32_t id){
        const auto sound=cache.load_sound(id);return float(sound.samples)/sound.sample_rate;
      });
    };
    std::vector<halo2::Spawn> spawns;
    for (const auto &spawn : cache.spawns())
      if (spawn.bsp == bsp)
        spawns.push_back(spawn);
    if (play || simulate || !weapon_name.empty() || !squad_name.empty() ||
        !script_name.empty()) {
      content = cache.load_content(bsp);
      if (spawns.empty())
        throw std::runtime_error(
            "BSP has no authored player start for simulation");
      std::cout << content->describe() << '\n';
      simulation = std::make_unique<halo2::Simulation>(
          *content, halo2::BspContent{std::move(content->collision),content->navigation},
          bsp, spawns.front(), [&](auto graph, auto name) {
            return cache.load_named_animation(graph, name);
          }, [&](std::size_t target) { return cache.load_bsp_content(target); },
          [&](const halo2::Simulation::Prediction &p) {
            if (p.kind == halo2::Simulation::Prediction::Barrier) residency.barrier();
            else if (p.kind == halo2::Simulation::Prediction::Sound) {
              if(scripted_audio)scripted_audio->prepare(p.datum);
              else residency.predict_sound(p.datum);
            }
            else if (p.kind == halo2::Simulation::Prediction::Model) residency.model(p.datum,p.section);
            else if (p.kind == halo2::Simulation::Prediction::Object) residency.object(p.datum);
            else residency.structure(p.datum,p.section,p.instances);
          },std::nullopt,std::nullopt,halo2::SourceSceneObjectOwner{},campaign_source(bsp));
      simulation->controller_look_settings([&]{return desktop->controller_settings.invert;},[&](bool value){desktop->controller_settings.invert=value;});
      simulation->cinematic_animation_assets(std::make_shared<halo2::CinematicAnimationAssets>(cache));
      bind_cinematic_text();
      simulation->set_difficulty(menu_difficulty);
      if (bsp_replay) {
        if (*bsp_replay >= cache.bsps().size() || *bsp_replay == bsp)
          throw halo2::FormatError("BSP replay needs a different valid target");
        simulation->request_checkpoint();
      }
      if (campaign)
        simulation->start_level();
      if (!camera_graph.empty()) {
        const auto graph = cache.query_tag({.group=halo2::tag_groups::model_animation_graph, .name=camera_graph});
        if (!graph)
          throw halo2::FormatError("camera graph tag not found");
        simulation->preview_camera(
            graph->datum, cache.animation_name(graph->datum, camera_track),
            camera_anchor);
      }
      if (!weapon_name.empty()) {
        const halo2::WeaponDefinition *match = nullptr;
        for (const auto &[id, w] : content->weapons)
          if (w.name.find(weapon_name) != std::string::npos) {
            if (w.name == weapon_name) {
              match = &w;
              break;
            }
            if (match)
              throw std::runtime_error("ambiguous weapon name");
            match = &w;
          }
        if (!match)
          throw std::runtime_error("weapon not found");
        simulation->equip(match->datum);
      }
      if (!squad_name.empty())
        std::cout << "Placed " << simulation->place_squad(squad_name)
                  << " actors from " << squad_name << '\n';
      if (!script_name.empty())
        simulation->run_script(script_name);
      if (simulate) {
        halo2::Controls input;
        input.yaw = spawns.front().facing;
        const auto report_simulation=[&] {
          std::cout << simulation->status() << '\n';
          for (const auto &fault : simulation->scripts().diagnostics())
            std::cerr << "Script fault: " << fault << '\n';
        };
        try {
          for (unsigned i = 0; i < simulate; ++i) {
            const auto before=simulation->ticks();
            simulation->step(input);
            // Headless simulation has one native outer iteration per requested
            // game tick. Advance retained effect owners without a GPU consumer.
            if(auto *source=simulation->campaign_source())
              source->objects().advance_effect_frame(simulation->ticks()==before+1u?halo2::tick_seconds:0.f);
          }
        } catch (...) {
          report_simulation();
          throw;
        }
        report_simulation();
        return simulation->scripts().diagnostics().empty() ? 0 : 1;
      }
      play = true;
    }
    auto mesh = cache.load_bsp(bsp, !diagnostic);
    std::optional<halo2::SourceCampaignViewTu15> source_view;
    if(simulation && simulation->campaign_source() && !diagnostic)source_view.emplace(cache,bsp);
    std::size_t geometry_materials = mesh.materials.size();
    struct Visual {
      halo2::RenderModel model;
      std::optional<halo2::AnimationClip> idle;
      std::uint32_t material_base{};
      std::unique_ptr<halo2::SourceCharacterShaders> character_shaders;
      std::unique_ptr<halo2::SourceCampaignModel> source_actor_model;
      std::unique_ptr<halo2::SourceCharacterShaderInputs> source_actor_inputs;
      std::optional<halo2::SourceObjectColorDefinition> vehicle_colors;
      std::uint32_t vehicle_variant{};
      std::unique_ptr<halo2::SourceDynamicModelShaders> cinematic_shaders;
      std::unique_ptr<halo2::SourceCampaignModel> source_cinematic_model;
      bool source_forward_only{};
      bool source_lit_scene{};
      std::uint32_t cinematic_material_base{};
      std::optional<halo2::ShadowCasterDefinition> cinematic_bounds;
      std::optional<halo2::SourceObjectColorDefinition> cinematic_colors;
      std::uint32_t cinematic_variant{};
      std::vector<halo2::ObjectFunction> cinematic_functions;
      std::string cinematic_failure;
    };
    std::unordered_map<std::uint32_t, Visual> actor_visuals;
    std::set<std::pair<std::uint32_t,std::uint32_t>> reported_actor_overrides;
    std::set<std::pair<std::uint32_t,unsigned>> reported_actor_unresident;
    std::unordered_map<std::uint64_t, Visual> object_visuals;
    std::unique_ptr<halo2::SourceCampaignWeapons> source_weapon_models;
    std::set<std::pair<std::uint32_t,std::uint32_t>> reported_weapon_overrides;
    std::optional<halo2::ObjectLightingDefinition> character_lighting_definition;
    std::optional<halo2::BspSurfaceLighting> character_surface_lighting;
    std::optional<halo2::BspAtmosphere> character_atmosphere;
    const auto cinematic_function_tables=simulation?cache.shader_function_tables():nullptr;
    std::vector<std::optional<halo2::ObjectSkyLightingDefinition>> character_sky_lighting;
    std::unordered_map<std::uint32_t,halo2::ObjectLightingCache> character_light_cache;
    const auto reload_character_lighting=[&](std::size_t selected_bsp) {
      if(!simulation || diagnostic)return;
      character_lighting_definition=cache.load_object_lighting_definition();
      character_surface_lighting=cache.load_bsp_surface_lighting(selected_bsp);
      character_atmosphere=cache.load_bsp_atmosphere(selected_bsp);
      character_sky_lighting.clear();
      for(unsigned i=0;i<character_atmosphere->skies.size();++i)
        character_sky_lighting.push_back(cache.load_object_sky_lighting(i));
      character_light_cache.clear();
    };
    reload_character_lighting(bsp);
    std::optional<Visual> arms, gun;
    struct CampaignFirstPerson {
      std::optional<Visual> gun;
      std::optional<halo2::AnimationClip> idle,fire,reload,reload_empty;
    };
    std::unordered_map<std::uint32_t,CampaignFirstPerson> campaign_first_person;
    std::uint32_t active_fp_definition=halo2::null_datum;
    std::optional<Visual> sky_visual;
    std::optional<halo2::SkyDefinition> sky_definition;
    std::optional<halo2::BspSkyModels> cluster_sky_models;
    std::optional<halo2::AnimationClip> fp_idle, fp_fire, fp_reload,
        fp_reload_empty, fp_fixture;
    auto add_model = [&](halo2::RenderModel model) {
      Visual v;
      v.material_base = static_cast<std::uint32_t>(mesh.materials.size());
      for (auto &m : model.mesh.materials)
        mesh.materials.push_back(std::move(m));
      v.model = std::move(model);
      return v;
    };
    if (simulation) {
      if(simulation->campaign_source() && !diagnostic)
        source_weapon_models=std::make_unique<halo2::SourceCampaignWeapons>(cache,*content,mesh);
      for (const auto &[id, character] : content->characters) {
        const auto biped = character.biped.datum;
        if (biped == halo2::null_datum || actor_visuals.contains(biped))
          continue;
        try {
          if(simulation->campaign_source() && !diagnostic) {
            Visual visual;
            visual.material_base=std::uint32_t(mesh.materials.size());
            visual.model=cache.load_model_hierarchy(biped);
            visual.source_actor_model=std::make_unique<halo2::SourceCampaignModel>(cache,biped);
            visual.source_actor_inputs=std::make_unique<halo2::SourceCharacterShaderInputs>(cache,biped);
            visual.source_actor_model->prepare_materials(cache,mesh);
            actor_visuals.emplace(biped,std::move(visual));
            continue;
          }
          auto model = cache.load_model(biped, !diagnostic);
          std::optional<halo2::AnimationClip> idle;
          try {
            idle = cache.load_action(model.animation, "combat", "missile",
                                     "any", "idle");
            halo2::pose_model(model, &*idle);
          } catch (const halo2::FormatError &e) {
            idle.reset();
            std::cerr << "Animation unavailable " << model.name << ": "
                      << e.what() << '\n';
          }
          std::unique_ptr<halo2::SourceCharacterShaders> shaders;
          if(!diagnostic)shaders=std::make_unique<halo2::SourceCharacterShaders>(cache,model,biped);
          auto visual = add_model(std::move(model));
          visual.character_shaders=std::move(shaders);
          visual.idle = std::move(idle);
          actor_visuals.emplace(biped, std::move(visual));
        } catch (const halo2::FormatError &e) {
          std::cerr << "Actor model unavailable: " << e.what() << '\n';
        }
      }
      const auto cinematic_definitions=halo2::cinematic_lighting_definitions(*content);
      for (const auto &object : content->objects) {
        const auto key =
            (std::uint64_t(object.variant) << 32) | object.definition;
        if (object.definition == halo2::null_datum ||
            object_visuals.contains(key) || object.type == 10)
          continue;
        if(source_weapon_models && source_weapon_models->owns_definition(object.definition))continue;
        try {
          auto model=cache.load_model(object.definition,!diagnostic,object.variant);
          std::optional<halo2::RenderModel> cinematic_model;
          std::unique_ptr<halo2::SourceDynamicModelShaders> cinematic_shaders;
          std::unique_ptr<halo2::SourceCampaignModel> source_cinematic_model;
          std::optional<halo2::ShadowCasterDefinition> cinematic_bounds;
          std::optional<halo2::SourceObjectColorDefinition> cinematic_colors;
          std::uint32_t cinematic_variant{};
          std::string cinematic_failure;
          // A layer15-only model has no object-lighting classification to
          // choose. Its original transparent implementation0 still needs
          // the source collector, vertex program and authored blend state.
          const bool source_forward_only=simulation->campaign_source() && object.type==6 &&
              std::ranges::any_of(model.mesh.materials,[](const auto &m){return !m.lods.empty();}) &&
              std::ranges::all_of(model.mesh.materials,[](const auto &m){
                return std::ranges::all_of(m.lods,[](const auto &lod){return lod.available_layers==(1u<<15);});
              });
          bool source_lit_scene=!diagnostic && simulation->campaign_source() &&
              halo2::source_campaign_lit_scenery(model,object);
          if(!diagnostic && (cinematic_definitions.contains(object.definition) || source_forward_only || source_lit_scene)) {
            try {
            if(simulation->campaign_source()) {
              source_cinematic_model=std::make_unique<halo2::SourceCampaignModel>(cache,object);
              cinematic_bounds=cache.load_object_shadow_definition(object.definition);
              cinematic_colors=cache.load_source_object_color_definition(object.definition);
              source_cinematic_model->prepare_materials(cache,mesh,source_lit_scene);
            } else {
            cinematic_model=model;
            for(auto &material:cinematic_model->mesh.materials) {
              const auto colors=material.object_render_colors;
              if(!material.lods.empty() && !halo2::prepare_dynamic_material(cache,material,{},true,0.f))
                throw halo2::FormatError("Campaign cinematic model shader is unresolved: "+material.name);
              if(colors)halo2::apply_object_render_colors(material,*colors);
            }
            const auto initial=cache.load_initial_model_definition(object.definition);
            cinematic_shaders=std::make_unique<halo2::SourceDynamicModelShaders>(cache,*cinematic_model,initial.render_model,2,0.f);
            cinematic_bounds=cache.load_object_shadow_definition(object.definition);
            const auto selected=cache.load_initial_model_selection(object.definition,object.variant);
            if(!selected.resolved())throw halo2::FormatError("Campaign cinematic variant requires its source selection owner");
            cinematic_variant=selected.variant_index<0?0u:initial.variants.at(unsigned(selected.variant_index)).name;
            cinematic_colors=cache.load_source_object_color_definition(object.definition);
            }
            } catch(const halo2::FormatError &e) {
              cinematic_failure=e.what();cinematic_model.reset();cinematic_shaders.reset();source_cinematic_model.reset();
              source_lit_scene=false;
              std::cerr<<"Cinematic shader admission unresolved: "<<cinematic_failure<<'\n';
            }
          }
          std::unique_ptr<halo2::SourceCharacterShaders> vehicle_shaders;
          if(object.type==1 && !diagnostic) {
            // Admission is transactional: an unresolved authored PRT owner
            // must not leave a partially prepared model in the scene bank.
            auto candidate=model;
            try {
              vehicle_shaders=std::make_unique<halo2::SourceCharacterShaders>(cache,candidate,object.definition);
              model=std::move(candidate);
            }catch(const halo2::FormatError& e){std::cerr<<"Vehicle shader owner unresolved: "<<e.what()<<'\n';}
          }
          if(!diagnostic && !source_cinematic_model && !cinematic_model && !vehicle_shaders)
            for(auto &material:model.mesh.materials)
              halo2::prepare_independent_object_material(cache,material);
          auto v=add_model(std::move(model));v.character_shaders=std::move(vehicle_shaders);
          v.source_forward_only=source_forward_only;
          v.source_lit_scene=source_lit_scene;
          v.cinematic_failure=std::move(cinematic_failure);
          if(cinematic_model) {
            v.cinematic_material_base=std::uint32_t(mesh.materials.size());
            for(auto &material:cinematic_model->mesh.materials)mesh.materials.push_back(std::move(material));
            v.cinematic_shaders=std::move(cinematic_shaders);v.cinematic_bounds=std::move(cinematic_bounds);
            v.cinematic_colors=std::move(cinematic_colors);v.cinematic_variant=cinematic_variant;
            v.cinematic_functions=cache.load_object_functions(object.definition);
          }
          if(source_cinematic_model) {
            v.source_cinematic_model=std::move(source_cinematic_model);
            v.cinematic_bounds=std::move(cinematic_bounds);v.cinematic_colors=std::move(cinematic_colors);
            v.cinematic_functions=cache.load_object_functions(object.definition);
          }
          if(v.character_shaders) {
            const auto initial=cache.load_initial_model_definition(object.definition);
            const auto selected=cache.load_initial_model_selection(object.definition,object.variant);
            if(!selected.resolved())throw halo2::FormatError("Campaign vehicle variant requires its source selection owner");
            v.vehicle_variant=selected.variant_index<0?0u:initial.variants.at(unsigned(selected.variant_index)).name;
            v.vehicle_colors=cache.load_source_object_color_definition(object.definition);
          }
          object_visuals.emplace(key,std::move(v));
        } catch (const halo2::FormatError &e) {
          const auto tag = cache.query_tag({.handle=halo2::TagHandle{object.definition}});
          std::cerr << "Scene model unavailable "
                    << (!tag
                            ? std::to_string(object.definition)
                            : tag->name)
                    << ": " << e.what() << '\n';
        }
      }
      const auto equipped =
          content->weapons.find(simulation->player().weapon.definition);
      if (equipped != content->weapons.end() &&
          content->first_person_hands != halo2::null_datum) {
        const auto &w = equipped->second;
        try {
          auto hands =
              cache.load_model(content->first_person_hands, !diagnostic);
          if (!diagnostic) {
            const auto colors = cache.load_object_colors(content->player.datum);
            for (auto &material : hands.mesh.materials)
              halo2::apply_object_colors(material,colors);
          }
          auto weapon = cache.load_model(w.first_person_model, !diagnostic);
          fp_idle = cache.load_action(w.first_person_animation, "any", "any",
                                      "any", "idle");
          halo2::pose_first_person_models(weapon, hands, *fp_idle, 0,
                                          w.first_person_offset);
          arms = add_model(std::move(hands));
          gun = add_model(std::move(weapon));
          auto action = [&](const std::string &name, auto &destination) {
            try {
              destination = cache.load_action(w.first_person_animation, "any",
                                              "any", "any", name);
            } catch (const halo2::FormatError &e) {
              std::cerr << "Weapon action unavailable: " << e.what() << '\n';
            }
          };
          action("fire_1", fp_fire);
          action("reload_full", fp_reload);
          action("reload_empty", fp_reload_empty);
          if (!first_person_action.empty()) {
            fp_fixture = cache.load_action(w.first_person_animation, "any",
                                            "any", "any", first_person_action);
            if (first_person_frame >= fp_fixture->frame_count)
              throw halo2::FormatError("first person fixture frame outside clip");
            std::cout << "First person pose fixture: " << first_person_action
                      << " frame=" << first_person_frame
                      << " horizontal_fov_radians="
                      << content->player.camera_field_of_view << '\n';
          }
        } catch (const halo2::FormatError &e) {
          fp_idle.reset();
          arms.reset();
          gun.reset();
          std::cerr << "First person model unavailable: " << e.what() << '\n';
          if (!first_person_action.empty())
            throw;
        }
      }
      // Prepare every player-usable campaign pickup before Vulkan uploads.
      // A later inventory transfer selects resident geometry and its own graph.
      active_fp_definition=simulation->player().weapon.definition;
      if(!arms && content->first_person_hands!=halo2::null_datum)try {
        auto hands=cache.load_model(content->first_person_hands,!diagnostic);
        if(!diagnostic)for(auto &material:hands.mesh.materials)
          halo2::apply_object_colors(material,cache.load_object_colors(content->player.datum));
        arms=add_model(std::move(hands));
      } catch(const halo2::FormatError &e) {
        std::cerr<<"Campaign arms unavailable: "<<e.what()<<'\n';
      }
      for(const auto &[id,w]:content->weapons) {
        if(w.first_person_model==halo2::null_datum || w.first_person_animation==halo2::null_datum)continue;
        if(!source_weapon_models && !object_visuals.contains(id))try {
          object_visuals.emplace(id,add_model(cache.load_model(id,!diagnostic)));
        } catch(const halo2::FormatError &e) {
          std::cerr<<"Campaign item model unavailable "<<w.name<<": "<<e.what()<<'\n';
        }
        if(id==active_fp_definition)continue;
        try {
          CampaignFirstPerson visual;
          visual.idle=cache.load_action(w.first_person_animation,"any","any","any","idle");
          visual.gun=add_model(cache.load_model(w.first_person_model,!diagnostic));
          const auto action=[&](const char *name,auto &destination) {
            try {destination=cache.load_action(w.first_person_animation,"any","any","any",name);}
            catch(const halo2::FormatError &) {}
          };
          action("fire_1",visual.fire);action("reload_full",visual.reload);action("reload_empty",visual.reload_empty);
          campaign_first_person.emplace(id,std::move(visual));
        } catch(const halo2::FormatError &e) {
          std::cerr<<"Campaign first person unavailable "<<w.name<<": "<<e.what()<<'\n';
        }
      }
      std::cout << "Actor models=" << actor_visuals.size()
                << " first_person=" << bool(gun) << '\n';
    }
    if (!first_person_action.empty() && (!fp_fixture || !gun || !arms))
      throw std::runtime_error("first person fixture requires authored arms, weapon and animation");
    const std::size_t common_material_count=mesh.materials.size()-geometry_materials;
    std::unique_ptr<halo2::pstencil::CampaignRenderer> pstencil_campaign_renderer;
    halo2::pstencil::CampaignMaterialUpdates pstencil_campaign_materials;
    const auto load_pstencil_campaign_renderer=[&] {
      pstencil_campaign_renderer.reset();
      if(source_view)pstencil_campaign_renderer=std::make_unique<halo2::pstencil::CampaignRenderer>(cache,mesh,unsigned(bsp));
    };
    auto load_sky = [&]() {
      sky_visual.reset(); sky_definition.reset();cluster_sky_models.reset();
      if(!diagnostic && character_atmosphere) {
        cluster_sky_models.emplace(cache,*character_atmosphere,mesh);
        return;
      }
    if (!diagnostic) {
      try {
        sky_definition=cache.load_visible_sky(bsp);
        if(sky_definition) {
          auto model=cache.load_model(sky_definition->model);
          halo2::prepare_forward_materials(cache,model.mesh);
          sky_visual=add_model(std::move(model));
          std::cout << "Visible retail sky: " << sky_definition->name << '\n';
        }
      } catch(const halo2::FormatError &e) {
        sky_definition.reset();
        std::cerr << "Sky unavailable: " << e.what() << '\n';
      }
    }
    };
    load_sky();
    if(source_view) {
      // Original191E03..191E0B follows ordinary scenario births. Keep this
      // once-per-map producer outside view collection and mode switching.
      auto& world=simulation->campaign_source()->objects().tu15_world();
      const auto& memory=world.memory();
      const auto game=halo2::Bytes(memory.view(0x4f3218u,4)).u32(0);
      auto call=world.prepare_source_call();call.gp[0]=halo2::Bytes(memory.view(game+8u,4)).u32(0);
      world.run_actual(0xc29f0u,call);
    }
    load_pstencil_campaign_renderer();
    auto debug_material =
        static_cast<std::uint32_t>(mesh.materials.size());
    mesh.materials.push_back({});
    if (mesh.vertices.empty())
      throw halo2::FormatError("selected BSP contains no geometry");
    std::cout << cache.describe() << '\n'
              << "BSP " << bsp << ": " << mesh.vertices.size() / 3
              << " triangles\n";
    std::cout << "Diffuse textures: "
              << std::count_if(
                     mesh.materials.begin(), mesh.materials.end(),
                     [](const auto &m) { return !m.diffuse.pixels.empty(); })
              << "/" << mesh.materials.size() - 1 << '\n';
    std::cout << "Static retail lightmap passes: "
              << std::count_if(mesh.materials.begin(),mesh.materials.end(),
                               [](const auto& m){return m.static_lightmap_pass;})
              << "; baked lightmap images: " << mesh.lightmaps.size() << '\n';
    for (const auto &warning : mesh.material_warnings)
      std::cerr << "Rendering gap: " << warning << '\n';
    auto *window = desktop->window(viewport_width, viewport_height, hidden);
    if(mcc_preview)SDL_SetWindowTitle(window,"Halo 2 — MCC content preview");
    halo2::MultiplayerKbm gameplay_keyboard(SDL_GetWindowID(window),desktop->keyboard_settings);
    const auto initial_bsp=bsp;
    const auto initial_spawn=spawns.empty()?halo2::Spawn{}:spawns.front();
    bool restart_level{};
    std::unique_ptr<halo2::GamePauseMenu> pause_menu;
    const auto objective_strings=play && simulation?halo2::load_campaign_pause_objectives(cache,map):halo2::UiStrings{};
    const auto prepare_pause=[&] {
      if(!play || !simulation || multiplayer_options.ui_assets.empty())return;
      std::unique_ptr<char,decltype(&SDL_free)> pref(SDL_GetPrefPath("Halo2Native","Halo2"),SDL_free);
      pause_menu=std::make_unique<halo2::GamePauseMenu>(cache.shared_directory(),multiplayer_options.ui_assets,
          mesh,true,pref?std::filesystem::path(pref.get())/"menu.cfg":std::filesystem::path{},multiplayer_options.menu_ui_assets);
      pause_menu->command([&](halo2::GamePauseAction action) {
        try {
          if(action==halo2::GamePauseAction::Revert)simulation->request_revert();
          if(action==halo2::GamePauseAction::Restart)restart_level=true;
          return std::string{};
        } catch(const std::exception &error) {return std::string(error.what());}
      });
    };
    const auto bind_training_settings=[&] {
      if(simulation)simulation->controller_look_settings([&]{return desktop->controller_settings.invert;},[&](bool value){
        desktop->controller_settings.invert=value;
        if(pause_menu){pause_menu->state().adopt_controller_settings(desktop->controller_settings);pause_menu->state().save();}
      });
    };
    std::unique_ptr<halo2::NativeActionPromptHud> action_hud;
    std::unique_ptr<halo2::NativeWeaponHud> weapon_hud;
    const auto prepare_action_hud=[&] {
      action_hud.reset();
      weapon_hud.reset();
      if(simulation)action_hud=std::make_unique<halo2::NativeActionPromptHud>(cache,
          content->player.datum,halo2::gameplay_font_directory(cache),mesh,multiplayer_options.ui_assets);
      if(simulation && play)weapon_hud=std::make_unique<halo2::NativeWeaponHud>(cache,*content,mesh,
          halo2::gameplay_font_directory(cache),multiplayer_options.ui_assets);
    };
    prepare_pause();bind_training_settings();prepare_action_hud();
    std::unique_ptr<halo2::CampaignTextRenderer> campaign_text;
    if(play && simulation && campaign_text_content)campaign_text=std::make_unique<halo2::CampaignTextRenderer>(
        *campaign_text_content,halo2::gameplay_font_directory(cache),multiplayer_options.menu_ui_assets.empty()?multiplayer_options.ui_assets:multiplayer_options.menu_ui_assets,mesh);
    halo2::RumblePlayer rumble;halo2::RumbleOutput rumble_output;
    std::map<std::uint32_t,std::shared_ptr<const halo2::RumbleDefinition>> rumble_definitions;
    const auto rumble_tables=cache.shader_function_tables();
    halo2::NativeInteractionHold pad_pickup_hold,pad_dual_hold;
    halo2::ControllerInput pad_previous,pad_pending;
    std::int16_t campaign_view_zoom=-1;
    std::uint32_t campaign_zoom_reset{};
    halo2::UnitScopeCameraProducer campaign_scope_producer;
    std::optional<halo2::UnitScopeCameraState> campaign_scope;
    if(content && content->original_data->unit_scope_camera_constants)
      campaign_scope=halo2::source_scope_camera_initial(*content->original_data->unit_scope_camera_constants);
    std::optional<halo2::NativeVehicleCamera> campaign_vehicle_camera;
    const auto reset_campaign_camera=[&] {
      campaign_vehicle_camera.reset();
      if(simulation && simulation->campaign_source())
        campaign_vehicle_camera.emplace(simulation->campaign_source()->objects(),*content->original_data);
    };
    reset_campaign_camera();
    // Source bitmap metadata/material identities must precede Renderer upload.
    std::unique_ptr<halo2::SourceLensDrawFrameTu15> campaign_lens_draws;
    if(source_view && !cache.is_vista())
      campaign_lens_draws=std::make_unique<halo2::SourceLensDrawFrameTu15>(cache,mesh);
    if(campaign_weapon_gpu_trace.is_open())trace_campaign_weapon({{"event","timing"},{"stage","renderer_begin"}});
    auto renderer = std::make_unique<halo2::Renderer>(window, mesh, validation);
    if(campaign_weapon_gpu_trace.is_open())trace_campaign_weapon({{"event","timing"},{"stage","renderer_end"}});
    // Retire query GPU resources before the borrowed Renderer/device owner.
    std::unique_ptr<halo2::SourceLensQueryFrameTu15> campaign_lens_queries;
    // Original checkpoint storage excludes the low lens/render BSS. Register
    // the actual live consumer so campaign restore retains that external state.
    // Declared after query resources: unregister before their/device teardown.
    struct CampaignLensRegistration {
      halo2::CampaignSourceRuntime *owner{};
      void clear() {
        if(owner)owner->live_lens_renderer(false);
        owner=nullptr;
      }
      void bind(halo2::CampaignSourceRuntime &next) {
        next.live_lens_renderer(true);owner=&next;
      }
      ~CampaignLensRegistration() {
        if(owner)try {clear();}
        catch(const std::exception &error) {
          std::cerr<<"Campaign lens cleanup: "<<error.what()<<'\n';
        }
      }
    } campaign_lens_registration;
    const auto bind_campaign_lenses=[&] {
      if(!campaign_lens_draws)return;
      campaign_lens_queries=std::make_unique<halo2::SourceLensQueryFrameTu15>(*renderer);
      campaign_lens_draws->bind(*renderer);
      if(!simulation || !simulation->campaign_source())
        throw halo2::FormatError("Campaign lens consumer has no original source owner");
      campaign_lens_registration.bind(*simulation->campaign_source());
    };
    bind_campaign_lenses();
    if(simulation) {
      renderer->campaign_cinematic_crossfade({});
      simulation->cinematic_screen_consumer(true);
    }
    if(character_atmosphere)renderer->scene_bsp_atmosphere(*character_atmosphere);
    if(pause_menu)pause_menu->bind(window,*desktop,*renderer);
    renderer->debug_panel().share_display_settings(desktop->display_settings);
    if(debug_panel)renderer->debug_panel().set_visible(true);
    if(sky_definition && sky_definition->clear)
      renderer->scene_clear_color(*sky_definition->clear);
    std::cout << "Vulkan device: " << renderer->device_name() << '\n';
    std::unique_ptr<SDL_GameController, decltype(&SDL_GameControllerClose)>
        controller(nullptr, SDL_GameControllerClose);
    auto open_controller = [&]() {
      if (controller)
        return;
      for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
          controller.reset(SDL_GameControllerOpen(i));
          break;
        }
      }
    };
    open_controller();
    halo2::Camera camera = overview(mesh);
    halo2::AutoCenterState auto_center;float prior_input_pitch=camera.pitch;

    std::size_t spawn_index = 0;
    auto spawn_camera = [&]() {
      if (spawns.empty()) {
        camera = overview(mesh);
        return;
      }
      const auto &start = spawns[spawn_index];
      camera = {start.position, start.facing, 0};
      camera.position.z += 0.65f;
      if (simulation)
        simulation->respawn(start);
    };
    if (!use_overview) {
      if(simulation && simulation->campaign_source()) {
        const auto &player=simulation->player();camera={player.body.position,player.yaw,simulation->player_pitch()};
        camera.position.z+=player.definition.camera_height;
      } else spawn_camera();
    }
    std::unique_ptr<halo2::AudioEngine> audio;
    std::unordered_map<std::uint32_t, std::shared_ptr<const halo2::PcmSound>>
        sounds;
    if ((simulation || !play_sound_name.empty() || !loop_sound_name.empty()) &&
        !mute) {
      try {
        audio = std::make_unique<halo2::AudioEngine>();
        audio->master_volume(float(desktop->display_settings.volume)/10.f);
        if(auto error=audio->speaker_layout(desktop->display_settings.speakers);!error.empty())std::cerr<<"Audio layout: "<<error<<'\n';
      } catch (const std::exception &e) {
        if (!play_sound_name.empty() || !loop_sound_name.empty())
          throw;
        std::cerr << "Audio unavailable: " << e.what() << '\n';
      }
    }
    // The existing authored adapter consumes playback/scale/class/permutation
    // gain, pitch and original distance kernels. One shared path serves the
    // campaign and multiplayer; AudioEngine still owns UI and loop previews.
    if(audio && simulation && !cache.is_vista()) {
      scripted_audio=std::make_unique<halo2::MultiplayerAudio>(cache,scripted_audio_random,
          halo2::SdlSoundPlatformConfiguration{halo2::SdlSoundPlatformConfiguration::Delivery::DeviceCallback,1024,{}},
          [&](const halo2::MultiplayerAudioSubmission &cue) {
            if(cue.owner!=halo2::null_datum && simulation->campaign_source())
              scripted_speech.insert_or_assign(cue.voice,cue);
          },&scripted_audio_random);
      if(auto error=scripted_audio->speaker_layout(desktop->display_settings.speakers);!error.empty())
        std::cerr<<"Scripted audio layout: "<<error<<'\n';
    }
    if(pause_menu)pause_menu->audio(audio.get());
    if (audio)
      std::cout << "SDL audio driver: " << SDL_GetCurrentAudioDriver() << '\n';
    struct LoopPlayback {
      halo2::LoopingSound definition;
      std::vector<halo2::AudioEngine::VoiceId> voices;
    };
    std::unordered_map<std::uint32_t, LoopPlayback> loops;
    std::unordered_map<std::uint32_t, std::vector<halo2::AudioEngine::VoiceId>>
        impulses;
    std::map<halo2::MultiplayerAudio::Voice,std::uint32_t> scripted_emitters;
    halo2::CampaignAudioTimeline scripted_audio_timeline;
    const auto update_scripted_speech = [&] {
      if(!scripted_audio || !simulation->campaign_source())return;
      auto &world=simulation->campaign_source()->objects().tu15_world();
      for(auto it=scripted_speech.begin();it!=scripted_speech.end();) {
        const auto &cue=it->second;
        if(!simulation->object_lifetimes().contains(cue.owner)) {
          traced_scripted_voice_volumes.erase(it->first);
          it=scripted_speech.erase(it);continue;
        }
        if(!scripted_audio->voice_active(it->first)) {
          halo2::source_unit_speech_stop_tu15(world,cue.owner,cue.sound);
          traced_scripted_voice_volumes.erase(it->first);
          it=scripted_speech.erase(it);continue;
        }
        const auto consumed=scripted_audio->playback_position(it->first);
        if(std::getenv("HALO2_SCRIPT_CALL_TRACE") && consumed.first_device_frame) {
          const auto parameters=scripted_audio->voice_parameters(it->first);
          const auto [entry,first]=traced_scripted_voice_volumes.try_emplace(it->first,
              parameters.volume_hundredths_db);
          if(first || entry->second!=parameters.volume_hundredths_db) {
            std::cout<<"Script voice: tick="<<std::to_string(simulation->ticks())
                <<" sound="<<std::to_string(cue.sound)<<" owner="<<std::to_string(cue.owner)
                <<" voice="<<std::to_string(it->first)
                <<" volume_hundredths_db="<<std::to_string(parameters.volume_hundredths_db)
                <<" packet_frames="<<std::to_string(cue.frames)
                <<" source_frames="<<std::to_string(consumed.source_frames)
                <<" first_device_frame="<<std::to_string(*consumed.first_device_frame)<<'\n';
            entry->second=parameters.volume_hundredths_db;
          }
        }
        if(consumed.first_device_frame)
          halo2::source_unit_speech_tu15(world,cue.owner,cue.sound,cue.pitch_range,
              cue.permutation,float(consumed.source_frames/48000.0));
        ++it;
      }
    };
    auto pcm = [&](std::uint32_t id) -> std::shared_ptr<const halo2::PcmSound> {
      if (id == halo2::null_datum)
        return {};
      if (!sounds.contains(id)) sounds.emplace(id,residency.sound(id));
      return sounds.at(id);
    };
    auto loop_event = [&](const halo2::SoundEvent &event) {
      using Event = halo2::SoundEvent;
      if (event.action == Event::Reset) {
        audio->stop();
        if(scripted_audio)scripted_audio->stop();
        scripted_speech.clear();traced_scripted_voice_volumes.clear();
        scripted_emitters.clear();
        if(scripted_audio)scripted_audio_timeline.anchor(simulation->ticks(),scripted_audio->presentation_frames());
        loops.clear();
        impulses.clear();
        return;
      }
      if (event.action == Event::ImpulseStop) {
        for (auto id : impulses[event.datum])
          if(scripted_audio)scripted_audio->stop_voice(id);
          else audio->stop(id, true);
        impulses.erase(event.datum);
        return;
      }
      if (event.action == Event::LoopStop) {
        if (const auto found = loops.find(event.datum); found != loops.end()) {
          for (auto id : found->second.voices)
            audio->stop(id);
          loops.erase(found);
        }
        return;
      }
      if (event.action == Event::LoopAlternate) {
        if (const auto found = loops.find(event.datum); found != loops.end())
          for (std::size_t i = 0; i < found->second.voices.size(); ++i) {
            const auto &track = found->second.definition.tracks[i];
            const auto id = event.alternate ? track.alternate : track.loop;
            if (id != halo2::null_datum)
              audio->next_loop(found->second.voices[i], pcm(id));
          }
        return;
      }
      auto definition = cache.load_looping_sound(event.datum);
      if (definition.flags & 4) {
        for (auto &[datum, loop] : loops)
          for (auto id : loop.voices)
            audio->stop(id, true);
        loops.clear();
      }
      if (const auto found = loops.find(event.datum); found != loops.end()) {
        for (auto id : found->second.voices)
          audio->stop(id, true);
        loops.erase(found);
      }
      LoopPlayback playback{definition, {}};
      try {
        for (const auto &track : definition.tracks) {
          const auto start = pcm(track.start), body = pcm(track.loop),
                     end = pcm(track.end);
          const float gain = event.gain * std::pow(10.f, track.gain_db / 20.f);
          if (definition.flags & 2) {
            auto joined = std::make_shared<halo2::PcmSound>();
            joined->name = definition.name;
            for (auto clip : {start, body, end})
              if (clip) {
                const auto base=joined->frames();
                joined->stereo.insert(joined->stereo.end(),
                                      clip->stereo.begin(), clip->stereo.end());
                if (clip->class_spans.empty())
                  joined->class_spans.push_back({joined->frames(),clip->sound_class});
                else
                  for (const auto &span:clip->class_spans)
                    joined->class_spans.push_back({base+span.end_frame,span.sound_class});
              }
            playback.voices.push_back(audio->play(joined, gain));
          } else
            playback.voices.push_back(
                audio->play_sequence(start, body, end, gain));
        }
      } catch (...) {
        for (auto id : playback.voices)
          audio->stop(id, true);
        throw;
      }
      std::cout << "Looping sound: " << definition.name
                << " tracks=" << playback.voices.size() << '\n';
      loops.emplace(event.datum, std::move(playback));
    };
    auto submit_script_sound = [&](const halo2::SoundEvent &sound) {
      try {
        audio->class_gains(simulation->sound_classes().gains());
        scripted_audio->class_gains(simulation->sound_classes().gains());
        if(sound.action!=halo2::SoundEvent::Impulse &&
            sound.action!=halo2::SoundEvent::NonSpatialImpulse) {
          loop_event(sound);return;
        }
        auto *world=simulation->campaign_source()?&simulation->campaign_source()->objects().tu15_world():nullptr;
        std::uint32_t auxiliary{};
        if(world) {
          auxiliary=halo2::Bytes(world->memory().view(0x4f3cd8u,4)).u32(0)+4u;
          scripted_audio_random=halo2::Bytes(world->memory().view(auxiliary,4)).u32(0);
        }
        const auto publish_random=[&] {
          if(world)world->write_source_call_input(auxiliary,std::as_bytes(std::span(&scripted_audio_random,1)));
        };
        std::optional<halo2::MultiplayerAudio::Voice> voice;
        try {
          if(simulation->campaign_source()) {
            const auto due=scripted_audio_timeline.frame(simulation->ticks());
            voice=sound.action==halo2::SoundEvent::Impulse?
                scripted_audio->schedule_impulse_at(sound.datum,sound.position,due,sound.gain,halo2::MultiplayerAudioKind::Impulse,sound.owner):
                scripted_audio->schedule_impulse(sound.datum,due,sound.gain,halo2::MultiplayerAudioKind::Impulse,sound.owner);
          } else voice=sound.action==halo2::SoundEvent::Impulse?
              scripted_audio->play_impulse_at(sound.datum,sound.position,sound.gain,halo2::MultiplayerAudioKind::Impulse,sound.owner):
              scripted_audio->play_impulse(sound.datum,sound.gain,halo2::MultiplayerAudioKind::Impulse,sound.owner);
        } catch(...) {publish_random();throw;}
        publish_random();
        if(voice) {
          if(sound.action==halo2::SoundEvent::Impulse && simulation->campaign_source())
            scripted_emitters.emplace(*voice,sound.owner);
          auto &voices=impulses[sound.datum];
          std::erase_if(voices,[&](auto id){return !scripted_audio->voice_active(id);});
          voices.push_back(*voice);
        }
        return;
      } catch(const std::exception &error) {
        std::cerr<<"Audio fault: "<<error.what()<<'\n';
      }
    };
    if (!loop_sound_name.empty()) {
      if (!audio)
        throw std::runtime_error("looping sound requires audio");
      const halo2::Tag *match = nullptr;
      for (const auto &t : cache.select_tags())
        if (t.group == "lsnd" &&
            t.name.find(loop_sound_name) != std::string::npos) {
          if (t.name == loop_sound_name) {
            match = &t;
            break;
          }
          if (match)
            throw std::runtime_error("ambiguous looping sound name");
          match = &t;
        }
      if (!match)
        throw std::runtime_error("looping sound not found");
      loop_event({match->datum, {}, .15f, halo2::SoundEvent::LoopStart});
    }
    if (!play_sound_name.empty()) {
      if (mute)
        throw std::runtime_error("--sound and --mute cannot be combined");
      const halo2::Tag *match = nullptr;
      for (const auto &t : cache.select_tags())
        if (t.group == "snd!" &&
            t.name.find(play_sound_name) != std::string::npos) {
          if (t.name == play_sound_name) {
            match = &t;
            break;
          }
          if (match)
            throw std::runtime_error(
                "sound name is ambiguous; use its complete tag name");
          match = &t;
        }
      if (!match)
        throw std::runtime_error("sound tag not found");
      const auto pcm = std::make_shared<halo2::PcmSound>(
          halo2::decode_sound(cache.load_sound(match->datum)));
      audio->play(pcm, 0.15f);
      std::cout << "Playing " << pcm->name << " frames=" << pcm->frames()
                << '\n';
    }
    std::uint64_t observed_bsp_revision = simulation ? simulation->bsp_revision() : 0;
    auto update_bsp = [&]() {
      if (!simulation || observed_bsp_revision == simulation->bsp_revision())
        return;
      const auto target = simulation->bsp();
      // Previous submissions and query generations belong to the old target
      // owner. Recreate bitmap material identities with the new BSP mesh.
      campaign_lens_registration.clear();
      campaign_lens_queries.reset();campaign_lens_draws.reset();
      reset_campaign_camera();
      auto next_mesh = cache.load_bsp(target, !diagnostic);
      if (next_mesh.vertices.empty())
        throw halo2::FormatError("campaign BSP contains no render geometry");
      const auto next_geometry_materials = next_mesh.materials.size();
      // The current mesh already owns these resident model images. Transfer
      // them across a BSP switch instead of retaining a second full image bank
      // for the entire campaign. Sky/UI/debug materials are rebuilt below.
      next_mesh.materials.insert(next_mesh.materials.end(),
          std::make_move_iterator(mesh.materials.begin()+geometry_materials),
          std::make_move_iterator(mesh.materials.begin()+geometry_materials+common_material_count));
      auto rebase = [&](Visual &v) {
        if(v.source_actor_model)v.source_actor_model->rebase_materials(unsigned(geometry_materials),unsigned(next_geometry_materials));
        if(v.cinematic_shaders)v.cinematic_material_base=std::uint32_t(v.cinematic_material_base-geometry_materials+next_geometry_materials);
        if(v.source_cinematic_model)v.source_cinematic_model->rebase_materials(unsigned(geometry_materials),unsigned(next_geometry_materials));
        v.material_base = std::uint32_t(v.material_base - geometry_materials +
                                        next_geometry_materials);
      };
      for (auto &[id,v] : actor_visuals) rebase(v);
      for (auto &[id,v] : object_visuals) rebase(v);
      if(source_weapon_models)source_weapon_models->rebase_materials(unsigned(geometry_materials),unsigned(next_geometry_materials));
      if (arms) rebase(*arms);
      if (gun) rebase(*gun);
      for(auto &[id,view]:campaign_first_person)if(view.gun)rebase(*view.gun);
      geometry_materials = next_geometry_materials;
      mesh = std::move(next_mesh);
      bsp = target;
      if(source_view)source_view.emplace(cache,bsp);
      reload_character_lighting(bsp);
      load_sky();
      load_pstencil_campaign_renderer();
      debug_material = std::uint32_t(mesh.materials.size());
      mesh.materials.push_back({});
      // Release old Vulkan resources after waiting for their submissions.
      // Static geometry, lightmap descriptors and draw ranges change together.
      const auto debug_settings=renderer->debug_panel().game_settings();
      const bool debug_visible=renderer->debug_panel().visible();
      renderer.reset();
      pause_menu.reset();prepare_pause();prepare_action_hud();
      campaign_text.reset();
      if(play && simulation && campaign_text_content)campaign_text=std::make_unique<halo2::CampaignTextRenderer>(
          *campaign_text_content,halo2::gameplay_font_directory(cache),multiplayer_options.menu_ui_assets.empty()?multiplayer_options.ui_assets:multiplayer_options.menu_ui_assets,mesh);
      if(source_view && !cache.is_vista())
      campaign_lens_draws=std::make_unique<halo2::SourceLensDrawFrameTu15>(cache,mesh);
      renderer = std::make_unique<halo2::Renderer>(window,mesh,validation);
      bind_campaign_lenses();
      if(simulation) {
        renderer->campaign_cinematic_crossfade({});
        simulation->cinematic_screen_consumer(true);
      }
      if(character_atmosphere)renderer->scene_bsp_atmosphere(*character_atmosphere);
      if(pause_menu)pause_menu->bind(window,*desktop,*renderer);
      renderer->debug_panel().share_display_settings(desktop->display_settings);
      renderer->debug_panel().game_settings(debug_settings);
      renderer->debug_panel().set_visible(debug_visible);
      if (sky_definition && sky_definition->clear)
        renderer->scene_clear_color(*sky_definition->clear);
      spawns.clear();
      for (const auto &spawn : cache.spawns())
        if (spawn.bsp == bsp) spawns.push_back(spawn);
      spawn_index = 0;
      observed_bsp_revision = simulation->bsp_revision();
      std::cout << "Campaign BSP committed: " << bsp << " triangles="
                << mesh.vertices.size()/3 << '\n';
    };
    auto last = Clock::now();
    bool running = true;
    unsigned frames = 0;
    std::size_t reported_faults = 0;
    std::uint64_t observed_restores = 0;
    halo2::SceneInterpolation scene_interpolation;
    halo2::CinematicCameraInterpolation cinematic_camera_interpolation{
        std::getenv("HALO2_REFERENCE_TICK_CAMERA")!=nullptr};
    const auto *campaign_frame_trace=std::getenv("HALO2_PSTENCIL_CAMPAIGN_TRACE");
    std::map<std::uint32_t,halo2::ModelNodeInterpolation> cinematic_render_interpolation;
    std::vector<halo2::Unit> previous_actors;
    std::vector<halo2::NativeWeaponProjectile> previous_projectiles;
    halo2::Unit previous_player;
    bool presentation_paired=false;
    halo2::CampaignCaptureCarry capture_carry;
    if(simulation)scene_interpolation.reset(simulation->ticks(),simulation->scene().objects());cinematic_render_interpolation.clear();
    bool skip_requested = false;
    bool replay_streamed=false, replay_reverted=false;
    bool prompt_gamepad=false;
    // Immediate submissions copy these CPU streams before returning. Retain
    // capacity across frames so appending objects does not repeatedly relocate
    // the complete accumulated vertex bank.
    std::vector<halo2::Vertex> debug,foreground,background,campaign_world;
    while (running && (!frame_limit || frames < frame_limit)) {
      if(campaign_weapon_gpu_trace.is_open())trace_campaign_weapon({{"event","timing"},{"stage","frame_begin"},{"frame",frames}});
      const auto now = Clock::now();
      const float audio_dt=std::chrono::duration<float>(now-last).count();
      const float dt = std::min(audio_dt, 0.05f);
      last = now;
      SDL_Event event;
      while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT)
          running = false;
        if (event.type == SDL_WINDOWEVENT &&
            (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
             event.window.event == SDL_WINDOWEVENT_RESTORED))
          renderer->resized();
        if(renderer->debug_panel().process_event(event))continue;
        if(pause_menu && pause_menu->event(event)) {gameplay_keyboard.release();continue;}
        if((event.type==SDL_KEYDOWN && event.key.windowID==SDL_GetWindowID(window)) ||
            (event.type==SDL_MOUSEBUTTONDOWN && event.button.windowID==SDL_GetWindowID(window)) ||
            (event.type==SDL_MOUSEMOTION && event.motion.windowID==SDL_GetWindowID(window) &&
             (event.motion.xrel || event.motion.yrel)))prompt_gamepad=false;
        if(controller && (event.type==SDL_CONTROLLERBUTTONDOWN ||
            (event.type==SDL_CONTROLLERAXISMOTION && std::abs(float(event.caxis.value)/32767.f)>=.15f))) {
          const auto instance=event.type==SDL_CONTROLLERBUTTONDOWN?event.cbutton.which:event.caxis.which;
          if(SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller.get()))==instance)prompt_gamepad=true;
        }
        gameplay_keyboard.handle_event(event);
        if (event.type == SDL_MOUSEBUTTONDOWN &&
            event.button.button == SDL_BUTTON_LEFT)
          SDL_SetRelativeMouseMode(SDL_TRUE);
        if (event.type == SDL_MOUSEMOTION && SDL_GetRelativeMouseMode() &&
            (!play || !simulation ||
             simulation->presentation().player_camera_enabled)) {
          const auto &settings=desktop->keyboard_settings;
          const float gain=.003f*float(settings.sensitivity)/3.f;
          camera.yaw -= float(event.motion.xrel) * gain;
          camera.pitch -= float(event.motion.yrel) * gain*(settings.invert?-1.f:1.f);
          if(!simulation || !simulation->campaign_source())camera.pitch=std::clamp(camera.pitch,-1.5f,1.5f);
        }
        if (event.type == SDL_KEYDOWN && !event.key.repeat) {
          if (event.key.keysym.sym == SDLK_ESCAPE) {
            if (play && simulation && simulation->cinematic_skippable())
              skip_requested = true;
            else if (SDL_GetRelativeMouseMode())
              SDL_SetRelativeMouseMode(SDL_FALSE);
            else
              running = false;
          }
          if (event.key.keysym.sym == SDLK_F2 && simulation)
            play = !play;
          if (event.key.keysym.sym == SDLK_r && !play)
            camera = overview(mesh);
          if (event.key.keysym.sym == SDLK_TAB) {
            if (!spawns.empty())
              spawn_index = (spawn_index + 1) % spawns.size();
            spawn_camera();
          }
        }
        if (event.type == SDL_CONTROLLERDEVICEADDED)
          open_controller();
        if (event.type == SDL_CONTROLLERDEVICEREMOVED && controller &&
            event.cdevice.which ==
                SDL_JoystickInstanceID(
                    SDL_GameControllerGetJoystick(controller.get())))
          controller.reset();
      }
      if(pause_menu && pause_menu->quit_requested())running=false;
      if (!running)
        break;
      if(restart_level) {
        capture_carry.clear();
        campaign_lens_registration.clear();
        campaign_vehicle_camera.reset();
        if(audio)audio->stop();
        if(scripted_audio)scripted_audio->stop();
        scripted_speech.clear();traced_scripted_voice_volumes.clear();
        scripted_emitters.clear();
        scripted_audio_timeline.clear();
        loops.clear();impulses.clear();reported_faults=0;
        character_light_cache.clear();
        simulation=std::make_unique<halo2::Simulation>(*content,cache.load_bsp_content(initial_bsp),initial_bsp,initial_spawn,
            [&](auto graph,auto name){return cache.load_named_animation(graph,name);},
            [&](std::size_t target){return cache.load_bsp_content(target);},
            [&](const halo2::Simulation::Prediction &p) {
              if(p.kind==halo2::Simulation::Prediction::Barrier)residency.barrier();
              else if(p.kind==halo2::Simulation::Prediction::Sound) {
                if(scripted_audio)scripted_audio->prepare(p.datum);
                else residency.predict_sound(p.datum);
              }
              else if(p.kind==halo2::Simulation::Prediction::Model)residency.model(p.datum,p.section);
              else if(p.kind==halo2::Simulation::Prediction::Object)residency.object(p.datum);
              else residency.structure(p.datum,p.section,p.instances);
            },std::nullopt,std::nullopt,halo2::SourceSceneObjectOwner{},campaign_source(initial_bsp));
        simulation->cinematic_animation_assets(std::make_shared<halo2::CinematicAnimationAssets>(cache));
        simulation->set_difficulty(menu_difficulty);bind_training_settings();bind_cinematic_text();rumble.clear();rumble_output={};if(campaign)simulation->start_level();
        observed_bsp_revision=std::numeric_limits<std::uint64_t>::max();update_bsp();
        camera.yaw=simulation->player().yaw;camera.pitch=simulation->player_pitch();
        campaign_view_zoom=-1;campaign_zoom_reset=simulation->player().zoom_reset_serial;
        campaign_scope_producer={};
        if(campaign_scope)campaign_scope=halo2::source_scope_camera_initial(*content->original_data->unit_scope_camera_constants);
        presentation_paired=false;restart_level=false;observed_restores=0;
        cinematic_camera_interpolation.clear();
        scene_interpolation.reset(simulation->ticks(),simulation->scene().objects());cinematic_render_interpolation.clear();
        if(source_weapon_models)source_weapon_models->reset();
        previous_actors.clear();previous_projectiles.clear();
      }
      const auto keys = SDL_GetKeyboardState(nullptr);
      const bool game_paused=pause_menu && pause_menu->visible();
      const bool debug_capture=renderer->debug_panel().captures_game_input() ||
          (pause_menu && pause_menu->captures_game_input());
      gameplay_keyboard.settings(desktop->keyboard_settings);
      if(debug_capture)gameplay_keyboard.release();
      const auto host_snapshot=gameplay_keyboard.snapshot(camera.yaw,camera.pitch);
      const auto &host_input=host_snapshot.controls;
      if(audio) {audio->paused(game_paused);audio->master_volume(float(desktop->display_settings.volume)/10.f);}
      auto pad_input=debug_capture?halo2::ControllerInput{}:halo2::controller_input(halo2::sample_controller(controller.get()),desktop->controller_settings);
      const auto pad_now=SDL_GetTicks();
      if(debug_capture){pad_pickup_hold={};pad_dual_hold={};pad_previous={};pad_pending={};}
      pad_pickup_hold.set(pad_input.pickup,pad_now);pad_dual_hold.set(pad_input.dual_wield,pad_now);
      const auto pad_pickup=pad_pickup_hold.sample(pad_now),pad_dual=pad_dual_hold.sample(pad_now);
      pad_pending.switch_weapon|=pad_input.switch_weapon&&!pad_previous.switch_weapon;
      pad_pending.grenade|=pad_input.grenade&&!pad_previous.grenade;
      pad_pending.flashlight|=pad_input.flashlight&&!pad_previous.flashlight;
      pad_pending.melee|=pad_input.melee&&!pad_previous.melee;
      pad_pending.switch_grenade|=pad_input.switch_grenade&&!pad_previous.switch_grenade;
      pad_pending.zoom|=pad_input.zoom&&!pad_previous.zoom;
      pad_previous=pad_input;pad_input.switch_weapon=pad_pending.switch_weapon;pad_input.grenade=pad_pending.grenade;
      pad_input.flashlight=pad_pending.flashlight;pad_input.melee=pad_pending.melee;pad_input.switch_grenade=pad_pending.switch_grenade;
      if(!debug_capture && (!play || !simulation || simulation->presentation().player_camera_enabled))
        halo2::controller_look(pad_input,desktop->controller_settings,dt,camera.yaw,camera.pitch,
            !simulation || !simulation->campaign_source());
      const float forward=debug_capture?0:(play?host_input.forward:float(keys[SDL_SCANCODE_W]-keys[SDL_SCANCODE_S]))+pad_input.forward;
      if(!debug_capture && simulation && play && simulation->presentation().player_camera_enabled) {
        const bool center=controller && (pad_input.forward || pad_input.right || pad_input.look_pitch || pad_input.look_yaw)
            ?desktop->controller_settings.auto_center:desktop->keyboard_settings.auto_center;
        //189BF7 clears generic auto-centering for a parented unit. Original
        //18A300 owns its authored seated pitch on the campaign command tick.
        if(simulation->player().source_parent!=halo2::null_datum)auto_center={};
        else halo2::auto_center_look(auto_center,center,forward,camera.pitch-prior_input_pitch,
            host_input.crouch||pad_input.crouch,halo2::length(simulation->player().body.velocity),dt,
            content->auto_level_scale,content->auto_level_minimum_ticks,camera.pitch);
      } else auto_center={};
      prior_input_pitch=camera.pitch;
      const float right=debug_capture?0:(play?host_input.right:float(keys[SDL_SCANCODE_D]-keys[SDL_SCANCODE_A]))+pad_input.right;
      const float vertical = debug_capture?0:float(keys[SDL_SCANCODE_E] - keys[SDL_SCANCODE_Q]);
      const float speed = (keys[SDL_SCANCODE_LSHIFT] ? 16.f : 4.f) * dt;
      if (simulation) {
        std::optional<halo2::SourceScriptSoundAdmissionBindingTu15> sound_admission;
        if(audio && scripted_audio) {
          scripted_audio->paused(game_paused);
          scripted_audio->master_volume(float(desktop->display_settings.volume)/10.f);
          if(!scripted_audio_timeline.anchored())
            scripted_audio_timeline.anchor(simulation->ticks(),scripted_audio->presentation_frames());
          sound_admission.emplace(*simulation,submit_script_sound);
        }
        auto& panel=renderer->debug_panel();
        simulation->debug_settings(panel.game_settings());
        while(const auto request=panel.take_request()) {
          try {
            halo2::apply_debug_request(*simulation,*request,spawns);
            if(request->kind==halo2::DebugGameRequest::Teleport || request->kind==halo2::DebugGameRequest::LoadSnapshot) {
              if(request->kind==halo2::DebugGameRequest::LoadSnapshot)capture_carry.clear();
              camera.yaw=simulation->player().yaw;camera.pitch=simulation->player_pitch();camera.oriented=false;
              presentation_paired=false;scene_interpolation.reset(simulation->ticks(),simulation->scene().objects());cinematic_render_interpolation.clear();
              previous_actors.clear();previous_projectiles.clear();
            }
            panel.status("Debug request applied.");
          } catch(const std::exception& e){panel.status(e.what());}
        }
        if(campaign_weapon_gpu_trace.is_open())trace_campaign_weapon({{"event","timing"},{"stage","simulation_begin"},{"frame",frames},{"tick",simulation->ticks()}});
        if(!game_paused)simulation->advance_audio(audio_dt);
        const float caller_frame_interval=bsp_replay?halo2::tick_seconds:audio_dt;
        const float source_frame_interval=simulation->campaign_source()?
            halo2::source_campaign_frame_interval_tu15(caller_frame_interval):caller_frame_interval;
        const auto frame_time=capture_carry.admit(
            simulation->advance_frame_time(source_frame_interval,game_paused),game_paused);
        const auto capture_pending=[&] {
          const auto *state=simulation->cinematic_state();
          return state && state->screen_consumer && state->crossfade.active &&
              (state->crossfade.pending || (state->crossfade.capture && state->crossfade.alpha>0.f));
        };
        int steps_due=frame_time.ticks;
        int completed_ticks=0;
        halo2::Controls input{forward,
                              right,
                              camera.yaw,
                              camera.pitch,
                              bool(!debug_capture && (host_input.jump||pad_input.jump)),
                              bool(!debug_capture && (host_input.crouch||pad_input.crouch)),
                              bool(!debug_capture && SDL_GetRelativeMouseMode() && (host_input.fire||pad_input.fire)),
                              bool(!debug_capture && (host_input.reload||pad_input.reload))};
        input.pickup=host_input.pickup||pad_pickup.first;input.dual_wield=host_input.dual_wield||pad_dual.first;
        input.pickup_milliseconds=std::max(host_input.pickup_milliseconds,pad_pickup.second);input.dual_wield_milliseconds=std::max(host_input.dual_wield_milliseconds,pad_dual.second);
        input.switch_weapon=host_input.switch_weapon||pad_input.switch_weapon;input.grenade=host_input.grenade||pad_input.grenade;
        input.fire_secondary=host_input.fire_secondary||pad_input.secondary;
        input.accept=pad_input.accept||host_input.accept;
        input.cancel=pad_input.cancel||host_input.cancel;input.melee=pad_input.melee||host_input.melee;input.flashlight=pad_input.flashlight||host_input.flashlight;
        input.switch_grenade=pad_input.switch_grenade||host_input.switch_grenade;input.look_stick=pad_input.look_pitch;
        bool scope_pressed=host_snapshot.actions.scope_pressed || pad_pending.zoom;
        halo2::campaign_frame_ticks(steps_due,
            [&]{return running && !restart_level && !game_paused;},[&] {
          const auto &unit=simulation->player();
          if(campaign_zoom_reset!=unit.zoom_reset_serial) {
            campaign_view_zoom=-1;campaign_zoom_reset=unit.zoom_reset_serial;
          }
          if(play && content->binocular_zoom) {
            const auto found=content->weapons.find(unit.weapon.definition);
            if(found!=content->weapons.end()) {
              const auto &weapon=found->second;
              const std::array<std::int16_t,1> modes{weapon.smg?unit.weapon.smg.magazine_mode:unit.weapon.magazine.mode};
              const halo2::ZoomWeaponView primary{weapon.zoom,weapon.magazine?
                  std::span(modes):std::span<const std::int16_t>{}};
              const auto actions=simulation->campaign_source()?unit.grenade_action.active_actions():unit.source_action_word;
              if(!halo2::source_zoom_admitted({unit.source_parent,actions,{&primary,nullptr}}))
                campaign_view_zoom=-1;
              else if(scope_pressed && !halo2::native_hand_weapon(unit,1))
                campaign_view_zoom=halo2::source_zoom_next(campaign_view_zoom,&primary,*content->binocular_zoom);
            }
          }
          input.zoom=campaign_view_zoom;input.zoom_reset_serial=campaign_zoom_reset;scope_pressed=false;
          if (bsp_replay && simulation->ticks()==1 && !replay_streamed) {
            simulation->request_bsp(int(*bsp_replay)); replay_streamed=true;
          }
          if (bsp_replay && simulation->ticks()>=3 && !replay_reverted) {
            simulation->request_revert(); replay_reverted=true;
          }
          input.skip_cinematic = skip_requested;
          previous_player=simulation->player();previous_actors=simulation->actors();
          previous_projectiles=simulation->presentation_projectiles();
          const auto prior_tick=simulation->ticks(),prior_bsp=simulation->bsp_revision(),prior_restores=simulation->checkpoint_restores();
          try {
            simulation->step(
                play ? input : halo2::Controls{0, 0, camera.yaw, camera.pitch});
          } catch (...) {
            std::cerr << simulation->status() << '\n';
            for(const auto &fault:simulation->scripts().diagnostics())
              std::cerr << "Script fault: " << fault << '\n';
            throw;
          }
          if(simulation->checkpoint_restores()==prior_restores && simulation->ticks()==prior_tick+1)
          {
            ++completed_ticks;
            // Retain original30Hz facial smoothing. Render-only frames must
            // not advance it again; progress comes from actually consumed PCM.
            update_scripted_speech();
          }
          if(simulation->reset_requested())restart_level=true;
          if(simulation->level_finished())running=false;
          if(simulation->checkpoint_restores()!=prior_restores) {
            // Remaining ticks belong to the discarded timeline. The saved
            // frame fraction is restored with the coupled campaign state.
            steps_due=0;
            capture_carry.clear();
            rumble.clear();
            // A restored salted actor identity can carry a different source
            // light-query history. Begin its cache with that restored lifetime.
            character_light_cache.clear();
          }
          for(const auto &event:simulation->take_rumble())if(event.owner==-1) {
            auto &definition=rumble_definitions[event.definition];
            if(!definition)definition=std::make_shared<const halo2::RumbleDefinition>(halo2::load_rumble_definition(cache,event.definition));
            rumble.play(definition,event.shielded,event.scale);
          }
          rumble_output=rumble.evaluate(*rumble_tables);rumble.advance(halo2::tick_seconds);
          if(simulation->training().suppressed || (simulation->training().actions&0x600000u)) {
            camera.yaw=input.yaw=simulation->player().yaw;camera.pitch=input.pitch=simulation->player_pitch();
          }
          gameplay_keyboard.consume(camera.yaw,camera.pitch);
          pad_pending={};pad_pickup_hold.consume();pad_dual_hold.consume();
          input.switch_weapon=input.grenade=input.melee=input.flashlight=input.switch_grenade=false;
          if (observed_restores != simulation->checkpoint_restores()) {
            observed_restores = simulation->checkpoint_restores();
            camera.yaw = input.yaw = simulation->player().yaw;
            camera.pitch = input.pitch = simulation->player_pitch();
            camera.oriented = false;
            campaign_view_zoom=simulation->player().source_zoom.current;
            campaign_zoom_reset=simulation->player().zoom_reset_serial;
            campaign_scope_producer={};
            reset_campaign_camera();
          }
          presentation_paired=simulation->ticks()==prior_tick+1 && simulation->bsp_revision()==prior_bsp &&
              prior_restores==simulation->checkpoint_restores();
          if(presentation_paired)scene_interpolation.capture(simulation->ticks(),simulation->scene().objects());
          else {scene_interpolation.reset(simulation->ticks(),simulation->scene().objects());cinematic_render_interpolation.clear();previous_actors.clear();previous_projectiles.clear();if(source_weapon_models)source_weapon_models->reset();}
          skip_requested = false;
        },capture_pending);
        if(!game_paused) {
          if(running && !restart_level && steps_due>0 && capture_pending())
            capture_carry.defer(frame_time,completed_ticks,steps_due);
          else capture_carry.clear();
        }
        simulation->advance_cinematic_render_time(frame_time.elapsed,frame_time.ticks,completed_ticks);
        simulation->advance_cinematic_camera(source_frame_interval);
        update_bsp();
        if (audio) audio->class_gains(simulation->sound_classes().gains());
        const auto &player = simulation->player();
        const float fraction=presentation_paired?std::clamp(simulation->frame_fraction(),0.f,1.f):1.f;
        if (play) {
          camera.field_of_view = float(desktop->display_settings.field_of_view);
          if(campaign_scope && content->binocular_zoom) {
            const auto found=content->weapons.find(player.weapon.definition);
            if(found!=content->weapons.end()) {
              const auto &weapon=found->second;
              const std::array<std::int16_t,1> modes{weapon.smg?player.weapon.smg.magazine_mode:player.weapon.magazine.mode};
              const halo2::ZoomWeaponView primary{weapon.zoom,weapon.magazine?
                  std::span(modes):std::span<const std::int16_t>{}};
              if(campaign_zoom_reset!=player.zoom_reset_serial || player.vitality.dead)campaign_view_zoom=-1;
              //105240 consumes the admitted unit selector. The local input
              //request may still be waiting for, or rejected by, that owner.
              const auto current_zoom=player.vitality.dead?std::int8_t(-1):player.source_zoom.current;
              const auto magnification=halo2::source_zoom_magnification(current_zoom,&primary,*content->binocular_zoom);
              if(!magnification)throw halo2::FormatError("Campaign optic requires unavailable general power consumer");
              const auto &constants=*content->original_data->unit_scope_camera_constants;
              halo2::UnitScopeCameraPacket packet{1,0,0,0,0};
              halo2::source_scope_camera_target(campaign_scope_producer,packet,
                  halo2::source_scope_camera_fov(player.definition.camera_field_of_view,*magnification,constants),constants);
              halo2::source_scope_camera_register(*campaign_scope,packet);
              halo2::source_scope_camera_adopt(*campaign_scope,packet,constants);
              if(!game_paused && halo2::source_scope_camera_motion(*campaign_scope,
                  halo2::source_scope_camera_dt(dt,constants),constants))
                throw halo2::FormatError("Campaign scope requires coupled observer cancellation owner");
              camera.field_of_view=halo2::gameplay_observer_field_of_view(
                  halo2::source_scope_camera_render_fov(*campaign_scope,constants),player.definition.camera_field_of_view,
                  desktop->display_settings.field_of_view)*57.295779513f;
            }
          }
          camera.horizontal_field_of_view = true;
          camera.position = presentation_paired?halo2::render_lerp(previous_player.body.position,player.body.position,fraction):player.body.position;
          camera.position.z += player.definition.camera_height +
                               (player.definition.crouch_camera_height -
                                player.definition.camera_height) *
                                   (presentation_paired?std::lerp(previous_player.crouch,player.crouch,fraction):player.crouch);
        }
        const auto &faults = simulation->scripts().diagnostics();
        if (reported_faults > faults.size())
          reported_faults = 0;
        while (reported_faults < faults.size())
          std::cerr << "Script fault: " << faults[reported_faults++] << '\n';
        if (frames % 15 == 0)
          SDL_SetWindowTitle(window,
                             ("Halo 2 native subsystem workbench | " +
                              simulation->status() + " | F2 fly | F reload")
                                 .c_str());
      }
      if(campaign_weapon_gpu_trace.is_open() && simulation)trace_campaign_weapon({{"event","timing"},{"stage","simulation_end"},{"frame",frames},{"tick",simulation->ticks()}});
      if (!play) {
        camera.position.x +=
            (std::cos(camera.yaw) * forward + std::sin(camera.yaw) * right) *
            speed;
        camera.position.y +=
            (std::sin(camera.yaw) * forward - std::cos(camera.yaw) * right) *
            speed;
        camera.position.z += vertical * speed;
      }
      if (resize_test && frames == 2) {
        SDL_SetWindowSize(window, 960, 600);
        renderer->resized();
      }
      const bool capture = !screenshot.empty() && frames + 1 == frame_limit;
      auto render_camera = camera;
      std::optional<halo2::SourceCampaignCameraViewTu15> diagnostic_source_camera;
      std::optional<halo2::SourceCameraAngles> campaign_scripted_angles;
      std::optional<float> campaign_scripted_horizontal_angle;
      if(simulation && play && simulation->campaign_source() && simulation->presentation().scripted) {
        auto completed=halo2::source_campaign_camera_view_tu15(
            simulation->campaign_source()->objects().memory(),0);
        if(!completed)throw halo2::FormatError("Scripted source camera lacks a completed registered observer");
        if(campaign_frame_trace)diagnostic_source_camera=*completed;
        *completed=cinematic_camera_interpolation.sample({simulation->ticks(),simulation->cinematic_camera_discontinuity(),
            simulation->checkpoint_restores(),simulation->bsp_revision(),presentation_paired,
            presentation_paired?std::clamp(simulation->frame_fraction(),0.f,1.f):1.f,*completed});
        render_camera.position=completed->position;render_camera.forward=completed->forward;render_camera.up=completed->up;
        render_camera.oriented=true;render_camera.horizontal_field_of_view=true;
        render_camera.field_of_view=completed->horizontal_field_of_view*57.295779513f;
        campaign_scripted_horizontal_angle=completed->horizontal_field_of_view;
        campaign_scripted_angles=halo2::source_camera_angles(completed->horizontal_field_of_view,completed->aspect);
        render_camera.projection_vertical_angle=campaign_scripted_angles->projection_angle;
      } else if(simulation && play) {
        cinematic_camera_interpolation.clear();
        render_camera=simulation->presentation().camera(camera,
            presentation_paired?std::clamp(simulation->frame_fraction(),0.f,1.f):0.f);
      } else cinematic_camera_interpolation.clear();
      std::optional<halo2::MultiplayerState::Player> campaign_hud_snapshot;
      bool campaign_vehicle_view=false;
      std::optional<halo2::SourceCameraAngles> campaign_vehicle_angles;
      float campaign_aim_displacement{};
      const bool gameplay_camera=simulation && play && !simulation->presentation().scripted && !simulation->presentation().cinematic;
      if(gameplay_camera) {
        if(simulation->campaign_source()) {
          const auto processed=simulation->campaign_source()->hud_player(*simulation);
          render_camera.yaw=processed.yaw;render_camera.pitch=processed.pitch;
          render_camera.oriented=false;
        }
        const auto forward=render_camera.oriented?render_camera.forward:halo2::direction(render_camera.yaw,render_camera.pitch);
        const auto up=render_camera.oriented?render_camera.up:halo2::cross(forward,{-std::sin(render_camera.yaw),std::cos(render_camera.yaw),0});
        const auto aim=halo2::source_observer_aim(forward,up,render_camera.field_of_view*.01745329238474369f,
            content->player_screen_offset,1.3333333730697632f,halo2::gameplay_camera_angle_scale);
        render_camera.forward=aim.forward;render_camera.up=aim.up;render_camera.oriented=true;
        campaign_aim_displacement=aim.displacement;
      }
      if(simulation && simulation->campaign_source()) {
        campaign_hud_snapshot=simulation->campaign_source()->hud_player(*simulation);
        if(play && campaign_vehicle_camera &&
            !simulation->presentation().scripted && !simulation->presentation().cinematic) {
          if(const auto seated=campaign_vehicle_camera->update_player(0,campaign_hud_snapshot->source_player,
              campaign_hud_snapshot->source_flags,game_paused?0.f:dt)) {
            render_camera.position=seated->position;render_camera.forward=seated->forward;render_camera.up=seated->up;
            render_camera.oriented=true;campaign_vehicle_view=true;
            campaign_aim_displacement=seated->aim_displacement;
            const auto baseline=content->original_data->death_camera_constants->default_fov;
            render_camera.field_of_view=halo2::vehicle_observer_field_of_view(seated->horizontal_fov,
                baseline,desktop->display_settings.vehicle_field_of_view)*57.295779513f;
            campaign_vehicle_angles=halo2::vehicle_camera_angles(seated->horizontal_fov,
                baseline,desktop->display_settings.vehicle_field_of_view);
          }
        }
      }
      if(simulation && simulation->campaign_source())
        simulation->campaign_source()->objects().publish_native_observer_position(0,render_camera.position);
      if(simulation && play && !simulation->presentation().scripted &&
          !simulation->presentation().cinematic) {
        int width{},height{};SDL_Vulkan_GetDrawableSize(window,&width,&height);
        if(width>0 && height>0) {
          const float fov=render_camera.field_of_view*.01745329238474369f;
          render_camera.projection_vertical_angle=campaign_vehicle_angles?campaign_vehicle_angles->projection_angle:
              halo2::gameplay_camera_angles(fov,halo2::gameplay_field_of_view(desktop->display_settings.field_of_view),
                  float(width)/height).projection_angle;
          const auto &safe=halo2::hud_viewport(1,0).safe;
          const auto physical_safe=halo2::full_surface_viewport({safe.left,safe.top,
              safe.right-safe.left,safe.bottom-safe.top},unsigned(width),unsigned(height));
          const auto lens=halo2::source_camera_projection(*render_camera.projection_vertical_angle,
              {0,0,float(width),float(height)},physical_safe,false);
          render_camera.projection_clip_scale=lens.clip_scale;
          render_camera.projection_clip_offset=lens.clip_offset;
        }
      }
      if(scripted_audio) {
        scripted_audio->paused(game_paused);
        scripted_audio->master_volume(float(desktop->display_settings.volume)/10.f);
        scripted_audio->class_gains(simulation->sound_classes().gains());
        if(auto source=simulation->campaign_source()) {
          auto &world=source->objects().tu15_world();
          for(auto it=scripted_emitters.begin();it!=scripted_emitters.end();) {
            if(!scripted_audio->voice_active(it->first) ||
                !simulation->object_lifetimes().contains(it->second)) {
              // Keep existing playback policy when an owner retires, and
              // never rebind its voice to a recycled object index.
              it=scripted_emitters.erase(it);continue;
            }
            scripted_audio->emitter(it->first,
                halo2::source_scripted_sound_point_tu15(world,it->second));
            ++it;
          }
        }
        scripted_audio->listeners(std::span(&render_camera.position,1));
        scripted_audio->update();
      }
      // The cinematic/vehicle observer is the listener. Its completed pose
      // must exist before queued script impulses are submitted to the mixer.
      if(simulation)
        for (const auto &sound : simulation->take_sounds())
          if (audio) {
            try {
              if (sound.action != halo2::SoundEvent::Impulse &&
                  sound.action != halo2::SoundEvent::NonSpatialImpulse) {
                loop_event(sound);
                continue;
              }
              if(scripted_audio) {submit_script_sound(sound);continue;}
              if (!sounds.contains(sound.datum))
                sounds.emplace(
                    sound.datum,
                    residency.sound(sound.datum));
              const auto delta = sound.position - render_camera.position;
              const float distance = std::sqrt(
                  delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
              const bool spatial=sound.action==halo2::SoundEvent::Impulse;
              const auto forward=render_camera.oriented?render_camera.forward:
                  halo2::direction(render_camera.yaw,render_camera.pitch);
              const auto up=render_camera.oriented?render_camera.up:halo2::Vec3{0,0,1};
              const auto right=halo2::normalized(halo2::cross(forward,up));
              const float pan=spatial && distance>.001f?halo2::dot(delta,right)/distance:0.f;
              auto &voices = impulses[sound.datum];
              if (voices.size() >= 32)
                voices.erase(voices.begin());
              voices.push_back(audio->play(sounds.at(sound.datum),
                                           spatial?sound.gain / (1.f + distance * .1f):sound.gain,
                                           pan));
            } catch (const std::exception &e) {
              std::cerr << "Audio fault: " << e.what() << '\n';
            }
          }
      debug.clear();foreground.clear();background.clear();
      std::optional<halo2::SourceCampaignViewFrameTu15> source_frame;
      std::vector<halo2::SourceLensQueryDraw> campaign_lens_query_draws;
      std::vector<halo2::SourceLensDraw> campaign_lens_final_draws;
      bool campaign_lens_frame{};
      const auto *cinematic=simulation?simulation->cinematic_state():nullptr;
      renderer->campaign_cinematic_bloom(simulation && simulation->presentation().cinematic,
          cinematic?cinematic->bloom:halo2::CinematicBloomOverride{});
      if(simulation)renderer->campaign_cinematic_crossfade(simulation->cinematic_crossfade_frame());
      std::vector<std::vector<halo2::Vertex>> character_poses;
      std::vector<halo2::ModelObjectDraw> character_draws;
      std::map<std::size_t,std::array<std::uint32_t,3>> campaign_weapon_draw_owners;
      std::vector<halo2::SourceModelTransparentDraw> campaign_transparent_models;
      unsigned campaign_weapon_gpu_draws{};
      const bool pstencil_campaign=renderer->debug_panel().display_settings().graphics_mode==halo2::pstencil::GraphicsMode::pStencil;
      halo2::pstencil::CampaignShadows pstencil_campaign_shadows;
      std::optional<halo2::SourceViewProjectionBank> pstencil_campaign_camera;
      std::array<std::int16_t,4> pstencil_campaign_viewport{};
      std::optional<halo2::pstencil::CampaignLightingFrame> pstencil_campaign_lighting;
      std::optional<halo2::SourceModelShaderView> character_shader_view;
      std::optional<halo2::SourceViewShaderConstants> character_camera_constants;
      std::optional<halo2::SourceCampaignEnvironmentViewTu15> character_environment;
      std::optional<halo2::BspFogSample> character_fog;
      std::optional<halo2::Vec2> character_clip_planes;
      float character_w_distance=1024.f;
      halo2::Camera character_camera=render_camera;
      float character_distance_scale{};
      float character_time{};
      int character_width{},character_height{};
      const auto add_pstencil_campaign_shadow=[&](std::uint32_t owner,std::span<const halo2::Vertex> vertices) {
        if(!pstencil_campaign || !simulation || !simulation->campaign_source() || diagnostic)return;
        auto& objects=simulation->campaign_source()->objects();
        const auto sphere=objects.object_sphere(owner);
        const auto metric=halo2::source_geometry_lod_distance(sphere.center,render_camera.position,character_distance_scale);
        pstencil_campaign_shadows.add(objects.tu15_world(),owner,metric,vertices);
      };
      if(character_atmosphere) {
        SDL_Vulkan_GetDrawableSize(window,&character_width,&character_height);
        if(character_width<=0 || character_height<=0)continue;
        if(character_width>32767 || character_height>32767)
          throw halo2::FormatError("Campaign source camera rectangle exceeds signed-short storage");
        const auto angles=campaign_vehicle_angles?*campaign_vehicle_angles:
            campaign_scripted_angles?*campaign_scripted_angles:
            render_camera.projection_vertical_angle && simulation && play &&
            !simulation->presentation().scripted && !simulation->presentation().cinematic?
            halo2::gameplay_camera_angles(render_camera.field_of_view*.01745329238474369f,
                halo2::gameplay_field_of_view(campaign_vehicle_view?desktop->display_settings.vehicle_field_of_view:
                    desktop->display_settings.field_of_view),float(character_width)/character_height):
            halo2::source_camera_angles(render_camera.field_of_view*.01745329238474369f);
        character_distance_scale=angles.lighting_distance_scale;
        halo2::SourceFrustumProjectionInput projection;
        projection.projection_angle=render_camera.projection_vertical_angle?*render_camera.projection_vertical_angle:
            render_camera.horizontal_field_of_view?angles.projection_angle:
            halo2::camera_vertical_field_of_view(render_camera,float(character_width)/character_height);
        projection.outer=projection.safe={0,0,std::int16_t(character_height),std::int16_t(character_width)};
        if(render_camera.projection_vertical_angle && simulation && play &&
            !simulation->presentation().scripted && !simulation->presentation().cinematic) {
          const auto &safe=halo2::hud_viewport(1,0).safe;
          const auto physical=halo2::full_surface_viewport({safe.left,safe.top,
              safe.right-safe.left,safe.bottom-safe.top},unsigned(character_width),unsigned(character_height));
          projection.safe={std::int16_t(physical.top),std::int16_t(physical.left),
              std::int16_t(physical.top+physical.height),std::int16_t(physical.left+physical.width)};
        }
        if(simulation && play &&
            (simulation->presentation().scripted || simulation->presentation().cinematic)) {
          const auto &safe=halo2::hud_viewport(1,0).safe;
          const auto lens=halo2::source_campaign_camera_lens_tu15(projection.projection_angle,
              unsigned(character_width),unsigned(character_height),
              {safe.left,safe.top,safe.right-safe.left,safe.bottom-safe.top});
          projection=lens.source;
          lens.apply(render_camera);lens.apply(character_camera);
        }
        const auto forward=render_camera.oriented?render_camera.forward:halo2::direction(render_camera.yaw,render_camera.pitch);
        const auto up=render_camera.oriented?render_camera.up:halo2::Vec3{0,0,1};
        halo2::Vec2 configured_clip{.015625f,1024.f};
        if(source_view) {
          auto& world=simulation->campaign_source()->objects().tu15_world();
          configured_clip=halo2::source_campaign_clip_planes_tu15(world);
          character_environment=halo2::source_campaign_environment_view_tu15(world,
              *character_atmosphere,render_camera.position,forward,configured_clip.x,configured_clip.y);
          character_fog=character_environment->fog;
          character_clip_planes=halo2::Vec2{configured_clip.x,character_environment->source.far_distance};
          character_w_distance=configured_clip.y;
        } else character_fog=character_atmosphere->sample(render_camera.position,forward);
        const auto clip=character_clip_planes.value_or(configured_clip);
        //2B888 clamps camera+50 before the camera projection and clipping
        //owners.491E7C then owns shader-bank/fog projection normalization;
        //configured4922F8 independently owns integer-W clip controls.
        projection.scenario_present=true;projection.scenario_type=0;
        const auto projected=halo2::source_frustum_projection(projection);
        const auto bank=halo2::source_view_projection({render_camera.position,forward,
            up,clip.x,clip.y,{},projected.projection_program});
        pstencil_campaign_camera=bank;
        pstencil_campaign_viewport=projection.outer;
        if(source_view) {
          auto &world=simulation->campaign_source()->objects().tu15_world();
          const auto frustum=halo2::source_frustum_camera({render_camera.position,forward,up,
              clip.x,clip.y,projected.clipping_rectangle()});
          halo2::MuzzleEffectView particles;
          particles.camera=halo2::source_view_shader_bank(bank,clip.y,render_camera.position,
              {0,0,float(character_width),float(character_height)},false);
          particles.fog=halo2::bsp_fog_constants(*character_fog,render_camera.position,forward);
          const auto& basis=bank.camera_to_world.basis;
          particles.orientation={basis[0].x,basis[0].y,basis[0].z,basis[1].x,basis[1].y,basis[1].z,
              basis[2].x,basis[2].y,basis[2].z};
          particles.position=render_camera.position;particles.forward=forward;particles.up=up;
          const auto depth=halo2::menu_world_depth_clip_planes({configured_clip.x,configured_clip.y,
              halo2::MenuWorldDepthBoundary::none,0,false});
          particles.clip_range={0,16777215.f,depth.minimum_w,depth.maximum_w};
          particles.width=unsigned(character_width);particles.height=unsigned(character_height);
          halo2::SourceCampaignViewInputTu15 input{bank,frustum,render_camera.position,forward,
              projection.outer,angles.lighting_distance_scale,
              halo2::source_geometry_lod_policy(1).coverage_multiplier,projected.view_modifier};
          input.particles=halo2::SourceCampaignParticleViewTu15{particles,projection.projection_angle,
              campaign_scripted_horizontal_angle.value_or(render_camera.field_of_view*.01745329238474369f)};
          input.environment=&character_environment->source;
          if(campaign_lens_queries) {
            campaign_lens_queries->begin(world);campaign_lens_draws->begin();
            campaign_lens_frame=true;
            // 2B68C precedes actual object/effect request production. The
            // collector invokes this after its frame/view reset, exactly once.
            input.prepare_lens_view=[&](auto &owner,unsigned view) {
              campaign_lens_queries->prepare_view(owner,view);
            };
          }
          source_frame=source_view->collect(simulation->campaign_source()->objects().tu15_world(),
              input,*character_atmosphere,*character_fog);
          if(campaign_weapon_gpu_trace.is_open())trace_campaign_weapon({{"event","source_frame"},{"frame",frames},
              {"tick",simulation->ticks()},{"eye",{render_camera.position.x,render_camera.position.y,render_camera.position.z}},
              {"forward",{forward.x,forward.y,forward.z}},{"up",{up.x,up.y,up.z}},
              {"projection_angle",projection.projection_angle},{"distance_scale",angles.lighting_distance_scale},
              {"coverage_multiplier",input.coverage_multiplier},{"viewport",projection.outer},
              {"view_modifier",projected.view_modifier},{"main_commands_completed",source_frame->main_commands_completed},
              {"source_particle_commands",source_frame->source_particle_commands},{"main_units",source_frame->base_draws.size()}});
          if(campaign_lens_frame) {
            campaign_lens_query_draws=campaign_lens_queries->view(world,0,render_camera.position,
                forward,up,projection.projection_angle,bank,projection.outer,
                unsigned(character_width),unsigned(character_height),clip);
            campaign_lens_final_draws=campaign_lens_draws->view(world,0,render_camera.position,
                forward,up,projection.projection_angle,bank,projection.outer,
                unsigned(character_width),unsigned(character_height),clip);
            campaign_lens_queries->finish_view(world,0);
          }
          if(pstencil_campaign)halo2::pstencil::prepare_native_incident_geometry_tu15(
              simulation->campaign_source()->objects().tu15_world(),source_frame->bsp);
        }
        const auto policy=halo2::source_geometry_lod_policy(1);
        character_shader_view=halo2::SourceModelShaderView{halo2::source_shader_coverage_camera(bank),
            halo2::source_shader_lod_view(0,std::int16_t(character_height),bool(simulation),
                simulation && simulation->presentation().cinematic),
            policy.coverage_multiplier,policy.effect_scale,projected.view_modifier};
        character_camera_constants.emplace();
        character_camera_constants->world=halo2::source_view_shader_constants(bank,clip.y,false);
        character_camera_constants->reflection=character_camera_constants->world;
        auto foreground_bank=bank;
        if(render_camera.projection_vertical_angle && simulation && play &&
            !simulation->presentation().scripted && !simulation->presentation().cinematic) {
          auto foreground_projection=projection;
          foreground_projection.projection_angle=halo2::gameplay_first_person_projection_angle;
          foreground_bank=halo2::source_view_projection({render_camera.position,forward,
              render_camera.oriented?render_camera.up:halo2::Vec3{0,0,1},clip.x,clip.y,{},
              halo2::source_frustum_projection(foreground_projection).projection_program});
        }
        const auto fp=halo2::source_view_shader_constants(foreground_bank,clip.y,true);
        std::copy_n(fp.begin(),4,character_camera_constants->foreground.begin());
        character_camera.projection_vertical_angle=projection.projection_angle;
      }
      if(cluster_sky_models) {
        if(!character_fog)throw halo2::FormatError("Campaign sky has no completed environment owner");
        background=cluster_sky_models->pose(*character_fog,render_camera.position);
      } else if(sky_visual) {
        background=halo2::pose_sky(sky_visual->model,*sky_definition,render_camera.position);
        for(auto &v:background)v.material+=sky_visual->material_base;
      }
      auto append_pose = [&](std::vector<halo2::Vertex> &out, const Visual &v,
                             const halo2::AnimationClip *clip, float seconds,
                             halo2::Vec3 position, float yaw, float pitch,
                             const halo2::RenderModel *companion = nullptr) {
        auto vertices = halo2::pose_model(v.model, clip, seconds, companion);
        const auto forward = halo2::direction(yaw, pitch),
                   left = halo2::Vec3{-std::sin(yaw), std::cos(yaw), 0},
                   up = halo2::cross(forward, left);
        for (auto &vertex : vertices) {
          const auto p = vertex.position;
          vertex.position = position + forward * p.x + left * p.y + up * p.z;
          const auto rotate=[&](halo2::Vec3 v){return forward*v.x+left*v.y+up*v.z;};
          vertex.normal=rotate(vertex.normal);vertex.tangent=rotate(vertex.tangent);
          vertex.binormal=rotate(vertex.binormal);
          vertex.material += v.material_base;
        }
        out.insert(out.end(), vertices.begin(), vertices.end());
      };
      if (simulation) {
        const float fraction=presentation_paired?std::clamp(simulation->frame_fraction(),0.f,1.f):1.f;
        const double render_tick=presentation_paired?double(simulation->ticks()-1)+fraction:double(simulation->ticks());
        character_time=float(render_tick)*halo2::tick_seconds;
        auto *source=simulation->campaign_source()?&simulation->campaign_source()->objects():nullptr;
        if(source && !diagnostic) {
          if(!source_frame)throw halo2::FormatError("Campaign source objects have no actual view admission");
          std::erase_if(cinematic_render_interpolation,[&](const auto &entry) {
            if(!source_frame->drawn.contains(entry.first))return true;
            return std::ranges::none_of(simulation->scene().objects(),[&](const auto &object){
                return object.handle==entry.first && object.visible && !object.bsp_hidden;
              }) && std::ranges::none_of(simulation->actors(),[&](const auto &actor){
                return actor.source_datum==entry.first && actor.exists && !actor.vitality.dead &&
                    actor.visible && !actor.bsp_hidden;
              });
          });
        }
        for (const auto &object : simulation->scene().objects()) {
          if (object.handle == halo2::null_datum || !object.visible)
            continue;
          if(std::any_of(simulation->weapon_pickups().begin(),simulation->weapon_pickups().end(),
              [&](const auto &p){return p.handle==object.handle;}))continue;
          if(source && !diagnostic && !source_frame->drawn.contains(object.handle))continue;
          const auto &placement = content->objects[object.placement];
          if(source && !diagnostic && source_weapon_models && source_weapon_models->owns_definition(placement.definition))continue;
          const auto visual = object_visuals.find(
              (std::uint64_t(placement.variant) << 32) | placement.definition);
          if (visual == object_visuals.end())
            continue;
          const auto *animation=simulation->object_animation(object.handle);
          float object_time=animation?std::max(0.f,float(render_tick-animation->tick)*halo2::tick_seconds):0;
          if(animation && !animation->loop)object_time=std::min(object_time,float(animation->clip->frame_count-1)*halo2::tick_seconds);
          const auto root=scene_interpolation.sample(object,fraction);
          const bool cinematic_lit=cinematic && cinematic->lit_objects.contains(object.handle);
          if((cinematic_lit || (cinematic && visual->second.source_forward_only) || visual->second.source_lit_scene) && !diagnostic) {
            const auto &v=visual->second;
            if((!v.cinematic_shaders && !v.source_cinematic_model) || !character_shader_view || !v.cinematic_bounds || !v.cinematic_colors)
              throw halo2::FormatError("Live cinematic lighting has no resident model shader owner: "+v.cinematic_failure);
            std::vector<halo2::Vertex> vertices;auto variant=v.cinematic_variant;
            std::optional<halo2::ObjectLightingState> lighting=cinematic?
                std::optional{cinematic->lighting}:std::nullopt;
            const halo2::SourceLiveModelBaseDrawTu15 *source_scene_draw=nullptr;
            if(v.source_cinematic_model) {
              if(!source)throw halo2::FormatError("Campaign source geometry lost its live object world");
              const auto sphere=source->object_sphere(object.handle);
              const auto shader=halo2::source_model_shader_collector(sphere.center,sphere.radius,*character_shader_view);
              const auto selected=source_frame->built.at(object.handle);
              if(halo2::Bytes(selected.bytes).u16(0x166)) {
                const auto nodes=source->rendered_world_nodes(object.handle);
                if(!nodes)throw halo2::FormatError("Campaign source model did not publish its deferred render pose");
                auto &history=cinematic_render_interpolation[object.handle];history.capture_render(simulation->ticks(),*nodes);
                const auto pose=history.sample_render(v.source_cinematic_model->hierarchy(),fraction,{},v.source_cinematic_model->scale_resets());
                if(v.source_lit_scene && !cinematic_lit) {
                  std::vector<halo2::SourceLiveModelBaseDrawTu15> ordinary;
                  for(const auto &draw:source_frame->base_draws)if(draw.object==object.handle) {
                    if(draw.override_slot!=255 || (draw.lighting_class!=1 && draw.lighting_class!=2) || !(draw.unit.words[0]&1u)) {
                      if(campaign_weapon_gpu_trace.is_open())trace_campaign_weapon({{"event","scenery_rejected"},
                          {"frame",frames},{"tick",simulation->ticks()},{"owner",object.handle},
                          {"definition",placement.definition},{"placement",object.placement},{"ordinal",draw.ordinal},
                          {"lighting_class",draw.lighting_class},{"override_slot",draw.override_slot},
                          {"override_shader",draw.shader_override},{"source_unit_words",draw.unit.words}});
                      throw halo2::FormatError("Campaign scenery requires its separate source lighting or override owner: object="+
                          std::to_string(object.handle)+" definition="+std::to_string(placement.definition)+
                          " ordinal="+std::to_string(draw.ordinal)+" class="+std::to_string(draw.lighting_class)+
                          " override="+std::to_string(draw.override_slot)+" shader="+std::to_string(draw.shader_override)+
                          " flags="+std::to_string(draw.unit.words[0]));
                    }
                    if(!source_scene_draw)source_scene_draw=&draw;
                    ordinary.push_back(draw);
                  }
                  if(ordinary.empty())continue;
                  lighting=source_scene_draw->lighting;
                  vertices=v.source_cinematic_model->draw(object.handle,ordinary,pose,*character_shader_view,
                      character_surface_lighting?&character_surface_lighting->lighting:nullptr);
                } else vertices=v.source_cinematic_model->draw(object.handle,selected,pose,shader,*character_shader_view);
              }
              const auto regions=source->object_model_regions(object.handle);
              variant=v.source_cinematic_model->variant_name(regions.selection.variant_index);
            } else {
              const auto nodes=object.solved_nodes?scene_interpolation.sample_nodes(object,fraction):
                  halo2::pose_model_nodes_world(v.model,root,animation?animation->clip.get():nullptr,object_time);
              if(nodes.empty())throw halo2::FormatError("Cinematic lighting requires the current WORLD node0");
              const auto sphere=halo2::source_shadow_sphere(*v.cinematic_bounds,nodes.front(),root.scale);
              vertices=halo2::skin_model_world_nodes(v.model,nodes);
              v.cinematic_shaders->select(vertices,halo2::source_model_shader_collector(sphere.center,sphere.radius,*character_shader_view),*character_shader_view);
              for(auto &vertex:vertices)vertex.material+=v.cinematic_material_base;
            }
            auto colors=halo2::source_object_initial_colors(*v.cinematic_colors,placement.position,variant);
            if(source_scene_draw)colors=halo2::source_live_model_base_colors_tu15(*source_scene_draw,colors);
            const int function_tick=int(simulation->ticks()?simulation->ticks()-1:0);
            const halo2::ShaderInput input=[&,handle=object.handle,function_tick](std::uint32_t name) {
              return halo2::evaluate_object_function(v.cinematic_functions,*cinematic_function_tables,name,
                  float(function_tick)*halo2::tick_seconds,[&](std::uint32_t variable) {
                    return simulation->scene().function_variable(handle,variable,function_tick);
                  }).value;
            };
            add_pstencil_campaign_shadow(object.handle,vertices);
            if(source_frame && v.source_forward_only && !vertices.empty()) {
              const auto &first=vertices.front();
              if(first.source_section<=511 && first.source_part<=255 &&
                 std::ranges::all_of(vertices,[&](const auto &vertex) {
                   return vertex.source_section==first.source_section && vertex.source_part==first.source_part &&
                       vertex.material==first.material;
                 }))campaign_transparent_models.push_back({object.handle,placement.definition,
                     first.source_section,first.source_part,mesh.materials.at(first.material).source_shader_datum,
                     character_draws.size()});
            }
            character_poses.push_back(std::move(vertices));
            character_draws.push_back({character_poses.back(),lighting,colors,input});
            continue;
          }
          if(visual->second.character_shaders && character_shader_view) {
            const auto nodes=object.solved_nodes?scene_interpolation.sample_nodes(object,fraction):
                halo2::pose_model_nodes_world(visual->second.model,root,animation?animation->clip.get():nullptr,object_time);
            if(nodes.empty())throw halo2::FormatError("Campaign vehicle has no current WORLD node0");
            const auto sphere=visual->second.character_shaders->sphere(nodes.front(),root.scale);
            auto vertices=halo2::skin_model_world_nodes(visual->second.model,nodes);
            visual->second.character_shaders->select(vertices,sphere,*character_shader_view);
            for(auto& vertex:vertices)vertex.material+=visual->second.material_base;
            auto& light=character_light_cache[object.handle];const auto frame=std::int32_t(frames);
            const auto distance=halo2::source_geometry_lod_distance(sphere.center,render_camera.position,character_distance_scale);
            const bool due=halo2::object_lighting_query_due(light,frame,sphere.center,distance,false);
            auto context=visual->second.character_shaders->light_query_context();
            context.bank_valid=halo2::source_object_light_query_bank_valid(character_surface_lighting->lighting);
            const auto fog=character_atmosphere->sample(render_camera.position,
                render_camera.oriented?render_camera.forward:halo2::direction(render_camera.yaw,render_camera.pitch));
            const halo2::ObjectSkyLightingDefinition* sky=nullptr;
            if(fog.visible_sky>=0 && std::size_t(fog.visible_sky)<character_sky_lighting.size() && character_sky_lighting[fog.visible_sky])sky=&*character_sky_lighting[fog.visible_sky];
            const auto query=due?halo2::source_object_light_query(context,[&](unsigned mode){
                return halo2::query_object_lightmap(*character_surface_lighting,simulation->collision(),sphere.center,
                    halo2::ObjectLightMapQueryContext{mode,!(light.sample_valid && light.lighting_valid),1.f,light.target});
            },[&]{return halo2::query_object_sky_lightmap(*character_surface_lighting,sky,sphere.center);}):halo2::ObjectLightQueryAdmission{};
            // Campaign scene vehicles still use the native placement/script
            // pose owner. Original moving-body velocity/cache routing is a
            // separate integration; no physics velocity is inferred here.
            halo2::update_object_lighting_cache(light,*character_lighting_definition,1,frame,sphere.center,{},due,
                query.accepted?query.query.sample:std::optional<halo2::ObjectLightmapSample>{});
            const auto colors=halo2::source_object_initial_colors(*visual->second.vehicle_colors,placement.position,visual->second.vehicle_variant);
            add_pstencil_campaign_shadow(object.handle,vertices);
            character_poses.push_back(std::move(vertices));character_draws.push_back({character_poses.back(),light.lighting,colors,{}});
            continue;
          }
          auto vertices = object.solved_nodes?halo2::skin_model_world_nodes(visual->second.model,
              scene_interpolation.sample_nodes(object,fraction)):
              halo2::pose_model(visual->second.model,animation?animation->clip.get():nullptr,object_time);
          for (auto &vertex : vertices) {
            const auto p=vertex.position;
            if(!object.solved_nodes)vertex.position=root.position+(root.basis[0]*p.x+root.basis[1]*p.y+root.basis[2]*p.z)*root.scale;
            const auto rotate=[&](halo2::Vec3 v) {
              return root.basis[0]*v.x+root.basis[1]*v.y+root.basis[2]*v.z;
            };
            if(!object.solved_nodes) {
              vertex.normal=rotate(vertex.normal);vertex.tangent=rotate(vertex.tangent);
              vertex.binormal=rotate(vertex.binormal);
            }
            vertex.material += visual->second.material_base;
          }
          add_pstencil_campaign_shadow(object.handle,vertices);
          debug.insert(debug.end(), vertices.begin(), vertices.end());
        }
        if(!source || diagnostic)for(const auto &item:simulation->weapon_pickups()) {
          if(!item.visible || !item.weapon)continue;
          const auto v=object_visuals.find(item.definition);if(v==object_visuals.end())continue;
          halo2::Transform root;root.position=item.position;
          root.basis={item.forward,halo2::cross(item.up,item.forward),item.up};
          auto vertices=halo2::pose_model_world(v->second.model,root);
          for(auto &vertex:vertices)vertex.material+=v->second.material_base;
          debug.insert(debug.end(),vertices.begin(),vertices.end());
        }
        if(active_fp_definition!=simulation->player().weapon.definition) {
          campaign_first_person[active_fp_definition]={std::move(gun),std::move(fp_idle),std::move(fp_fire),
              std::move(fp_reload),std::move(fp_reload_empty)};
          active_fp_definition=simulation->player().weapon.definition;
          auto &next=campaign_first_person[active_fp_definition];
          gun=std::move(next.gun);fp_idle=std::move(next.idle);fp_fire=std::move(next.fire);
          fp_reload=std::move(next.reload);fp_reload_empty=std::move(next.reload_empty);
          fp_fixture.reset();
        }
        for (const auto &actor : simulation->actors()) {
          if (!actor.exists || actor.vitality.dead || actor.bsp_hidden || !actor.visible)
            continue;
          const auto v = actor_visuals.find(actor.definition.datum);
          if (v == actor_visuals.end())
            continue;
          auto position=actor.body.position;auto body_yaw=actor.yaw;
          const auto index=std::size_t(&actor-simulation->actors().data());
          if(presentation_paired && index<previous_actors.size()) {
            const auto &old=previous_actors[index];
            if(old.exists && !old.vitality.dead && old.visible && !old.bsp_hidden &&
               old.source_datum==actor.source_datum && old.definition.datum==actor.definition.datum && old.bsp==actor.bsp) {
              position=halo2::render_lerp(old.body.position,position,fraction);
              body_yaw=halo2::render_angle(old.yaw,body_yaw,fraction);
            }
          }
          const auto *clip=actor.custom_animation?actor.custom_animation.get():v->second.idle?&*v->second.idle:nullptr;
          float time=std::max(0.f,float(render_tick-(actor.custom_animation?actor.custom_animation_tick:0))*halo2::tick_seconds);
          if(actor.custom_animation && !actor.custom_animation_loop)time=std::min(time,float(clip->frame_count-1)*halo2::tick_seconds);
          if((v->second.source_actor_model || v->second.character_shaders) && character_shader_view) {
            const halo2::SourceCharacterShaderInputs* inputs=v->second.source_actor_inputs?
                v->second.source_actor_inputs.get():v->second.character_shaders.get();
            halo2::ShadowCasterSphere sphere;
            std::vector<halo2::Vertex> vertices;
            if(v->second.source_actor_model) {
              if(!source || !source_frame)
                throw halo2::FormatError("Campaign actor lost its actual source view owner");
              if(!source_frame->drawn.contains(actor.source_datum))continue;
              std::vector<halo2::SourceLiveModelBaseDrawTu15> ordinary;
              for(const auto &draw:source_frame->base_draws)if(draw.object==actor.source_datum) {
                if(draw.override_slot!=255) {
                  if(reported_actor_overrides.emplace(actor.source_datum,draw.shader_override).second)
                    std::cerr<<"Campaign actor overlay pending: owner="<<draw.object
                        <<" ordinal="<<draw.ordinal<<" class="<<draw.lighting_class
                        <<" slot="<<unsigned(draw.override_slot)<<" shader="<<draw.shader_override<<'\n';
                  continue;
                }
                if(draw.lighting_class!=2 || !(draw.unit.words[0]&1u))
                  throw halo2::FormatError("Campaign actor ordinary main unit requires an unresolved lighting class");
                if(!draw.geometry.geometry.resident) {
                  const auto section=draw.unit.words[3]>>9&511u;
                  if(reported_actor_unresident.emplace(actor.source_datum,section).second)
                    std::cerr<<"Campaign actor source section unresident: owner="<<draw.object
                        <<" ordinal="<<draw.ordinal<<" model="<<draw.unit.words[1]
                        <<" lod="<<draw.lod<<" section="<<section<<'\n';
                  continue;
                }
                ordinary.push_back(draw);
              }
              // An override-only presentation (including a camouflage clone)
              // does not authorize an ordinary opaque body submission.
              if(ordinary.empty())continue;
              const auto selected=source_frame->built.find(actor.source_datum);
              if(selected==source_frame->built.end())
                throw halo2::FormatError("Campaign actor has no completed source model collector");
              const auto nodes=source->rendered_world_nodes(actor.source_datum);
              if(!nodes)throw halo2::FormatError("Campaign actor did not publish its deferred rendered WORLD pose");
              sphere=source->object_sphere(actor.source_datum);
              auto &history=cinematic_render_interpolation[actor.source_datum];
              history.capture_render(simulation->ticks(),*nodes);
              vertices=v->second.source_actor_model->draw(actor.source_datum,ordinary,
                  history.sample_render(v->second.source_actor_model->hierarchy(),fraction,{},v->second.source_actor_model->scale_resets()),*character_shader_view);
              if(frame_limit) {
                constexpr char hex[]="0123456789abcdef";
                std::string clock;
                for(const auto byte:source->memory().view(source->source_clock(),0x24)) {
                  const auto value=std::to_integer<unsigned>(byte);
                  clock.push_back(hex[value>>4]);clock.push_back(hex[value&15u]);
                }
                std::cout<<"Campaign source actor frame="<<frames<<" owner="<<actor.source_datum
                    <<" definition="<<source->object_definition(actor.source_datum)
                    <<" builder_rows="<<halo2::Bytes(selected->second.bytes).u16(0x166)
                    <<" WORLD_nodes="<<nodes->size()<<" selected_vertices="<<vertices.size()
                    <<" clock_24_hex="<<clock<<" ordinary_units=";
                for(const auto &draw:ordinary)
                  std::cout<<'['<<draw.ordinal<<','<<draw.unit.words[1]<<','
                      <<(draw.unit.words[3]>>9&511u)<<','<<draw.lod<<','<<unsigned(draw.coverage)<<']';
                std::cout<<'\n';
              }
            } else {
              const auto basis=halo2::authored_basis({body_yaw,0,0});
              const auto root=halo2::source_object_transform(position,basis.forward,basis.up,1,false);
              const auto nodes=halo2::pose_model_nodes_world(v->second.model,root,clip,time);
              if(nodes.empty())throw halo2::FormatError("Campaign character has no source node0");
              sphere=v->second.character_shaders->sphere(nodes.front(),1);
              vertices=halo2::skin_model_world_nodes(v->second.model,nodes);
              v->second.character_shaders->select(vertices,sphere,*character_shader_view);
              for(auto& vertex:vertices)vertex.material+=v->second.material_base;
            }
            auto& light=character_light_cache[actor.source_datum];
            const auto frame=std::int32_t(frames);
            const auto distance=halo2::source_geometry_lod_distance(sphere.center,render_camera.position,character_distance_scale);
            const bool due=halo2::object_lighting_query_due(light,frame,sphere.center,distance,false);
            const auto fog=character_atmosphere->sample(render_camera.position,
                render_camera.oriented?render_camera.forward:halo2::direction(render_camera.yaw,render_camera.pitch));
            const halo2::ObjectSkyLightingDefinition* sky=nullptr;
            if(fog.visible_sky>=0 && std::size_t(fog.visible_sky)<character_sky_lighting.size() && character_sky_lighting[fog.visible_sky])
              sky=&*character_sky_lighting[fog.visible_sky];
            auto context=inputs->light_query_context(actor);
            context.bank_valid=halo2::source_object_light_query_bank_valid(character_surface_lighting->lighting);
            const auto query=due?halo2::source_object_light_query(context,
                [&](unsigned mode){return halo2::query_object_lightmap(*character_surface_lighting,simulation->collision(),sphere.center,
                    halo2::ObjectLightMapQueryContext{mode,!(light.sample_valid && light.lighting_valid),1.f,light.target});},
                [&]{return halo2::query_object_sky_lightmap(*character_surface_lighting,sky,sphere.center);}):halo2::ObjectLightQueryAdmission{};
            halo2::update_object_lighting_cache(light,*character_lighting_definition,
                inputs->object_type(),frame,sphere.center,actor.body.velocity,due,
                query.accepted?query.query.sample:std::optional<halo2::ObjectLightmapSample>{});
            add_pstencil_campaign_shadow(actor.source_datum,vertices);
            character_poses.push_back(std::move(vertices));
            character_draws.push_back({character_poses.back(),light.lighting,{},
                inputs->input(actor,float(render_tick)*halo2::tick_seconds)});
          } else append_pose(debug,v->second,clip,time,position,body_yaw,0);
        }
        if(source_weapon_models && source && source_frame && character_shader_view) {
          auto weapon_frame=source_weapon_models->draw(*source,*source_frame,*character_shader_view,
              simulation->ticks(),fraction,float(render_tick)*halo2::tick_seconds);
          for(const auto &pending:weapon_frame.pending_overrides)
            if(reported_weapon_overrides.emplace(pending.object,pending.shader).second)
              std::cerr<<"Campaign weapon overlay pending: owner="<<pending.object<<" ordinal="<<pending.ordinal
                  <<" class="<<pending.lighting_class<<" shader="<<pending.shader<<'\n';
          for(auto &draw:weapon_frame.draws) {
            if(frame_limit)std::cout<<"Campaign source weapon frame="<<frames<<" owner="<<draw.object
                <<" definition="<<draw.definition<<" ordinal="<<draw.ordinal<<" vertices="<<draw.vertices.size()<<'\n';
            add_pstencil_campaign_shadow(draw.object,draw.vertices);
            if(campaign_weapon_gpu_trace.is_open()) {
              campaign_weapon_draw_owners.emplace(character_draws.size(),std::array{draw.object,draw.definition,std::uint32_t(draw.ordinal)});
              trace_campaign_weapon({{"event","source_weapon"},{"frame",frames},{"tick",simulation->ticks()},
                  {"object_index",character_draws.size()},{"owner",draw.object},{"definition",draw.definition},
                  {"ordinal",draw.ordinal},{"vertices",draw.vertices.size()}});
            }
            character_poses.push_back(std::move(draw.vertices));
            character_draws.push_back({character_poses.back(),draw.lighting,draw.colors,std::move(draw.input)});
          }
        }
        if (play && gun && arms && !simulation->player().vitality.dead &&
            (!campaign_hud_snapshot || campaign_hud_snapshot->source_vehicle_hud.ownership.handheld_visible) &&
            simulation->player().visible &&
            !simulation->presentation().cinematic &&
            !simulation->presentation().scripted) {
          const auto &state = simulation->player().weapon;
          const auto &w = content->weapons.at(state.definition);
          // FF5F0 supplies the magazine digits to each weapon-owned shader;
          // 17550 selects resident number textures independently of the pose.
          for(std::size_t i=0;i<gun->model.mesh.materials.size();++i) {
            auto &material=mesh.materials[gun->material_base+i];
            halo2::animate_material_bitmaps(material,
                float(render_tick)*halo2::tick_seconds,[&](std::uint32_t name) {
                  return halo2::weapon_ammunition_input(name,state.loaded).value_or(0.f);
                });
          }
          pstencil_campaign_materials.request(*renderer,mesh.materials);
          const halo2::AnimationClip *clip = &*fp_idle;
          float seconds = float(render_tick) * halo2::tick_seconds;
          if (state.reload > 0 &&
              (state.reload_empty ? fp_reload_empty.has_value()
                                  : fp_reload.has_value())) {
            clip = state.reload_empty ? &*fp_reload_empty : &*fp_reload;
            seconds = std::min(float(std::max(0.0,render_tick-double(state.reload_started))) *
                                   halo2::tick_seconds,
                               (clip->frame_count - 1) * halo2::tick_seconds);
          } else if (state.last_shot && fp_fire &&
                     simulation->ticks() - state.last_shot <
                         fp_fire->frame_count) {
            clip = &*fp_fire;
            seconds =
                float(std::max(0.0,render_tick-double(state.last_shot))) * halo2::tick_seconds;
          }
          if (fp_fixture) {
            clip = &*fp_fixture;
            seconds = float(first_person_frame) * halo2::tick_seconds;
          }
          auto pose = halo2::pose_first_person_models(
              gun->model, arms->model, *clip, seconds, w.first_person_offset);
          const auto f = render_camera.oriented
                             ? render_camera.forward
                             : halo2::direction(render_camera.yaw,
                                                render_camera.pitch);
          const auto u = render_camera.oriented
                             ? render_camera.up
                             : halo2::cross(
                                   f, {-std::sin(render_camera.yaw),
                                       std::cos(render_camera.yaw), 0});
          const auto view_basis =
              halo2::camera_transform(render_camera.position, f, u);
          const auto append_first_person = [&](auto &vertices, unsigned material_base) {
            for (auto &vertex : vertices) {
              const auto p = vertex.position;
              vertex.position =
                  (view_basis.basis[0] * p.x + view_basis.basis[1] * p.y) +
                  view_basis.basis[2] * p.z + view_basis.position;
              vertex.material += material_base;
              const auto rotate = [&](halo2::Vec3 v) {
                return (view_basis.basis[0] * v.x + view_basis.basis[1] * v.y) +
                       view_basis.basis[2] * v.z;
              };
              vertex.normal = rotate(vertex.normal);
              vertex.tangent = rotate(vertex.tangent);
              vertex.binormal = rotate(vertex.binormal);
            }
            foreground.insert(foreground.end(), vertices.begin(), vertices.end());
          };
          append_first_person(pose.arms, arms->material_base);
          append_first_person(pose.weapon, gun->material_base);
        }
        std::vector<std::uint32_t> rendered_models;
        for (const auto &[id, v] : actor_visuals)
          rendered_models.push_back(id);
        auto proxies = simulation->debug_mesh(rendered_models,previous_actors,previous_projectiles,fraction);
        for (auto &vertex : proxies)
          vertex.material = debug_material;
        debug.insert(debug.end(), proxies.begin(), proxies.end());
      }
      auto effects = simulation && play
                               ? simulation->presentation().effects(presentation_paired
                                     ? std::clamp(simulation->frame_fraction(),0.f,1.f) : 0.f)
                               : halo2::ScreenEffects{};
      if(campaign && simulation && (simulation->ticks()==0 || simulation->level_finished() ||
          (cinematic && cinematic->awaiting_camera))) {
        effects.fade_color={};effects.fade=1.f;effects.hud=0.f;
      }
      if(controller) {
        halo2::send_controller_rumble(controller.get(),rumble_output,desktop->controller_settings.vibration,game_paused,
            SDL_GetWindowFlags(window)&SDL_WINDOW_INPUT_FOCUS);
      }
      if(simulation)renderer->debug_panel().observe(halo2::observe_debug_game(*simulation,spawns));
      halo2::UiDraw pause_draw;
      if(weapon_hud && play && simulation && !game_paused && effects.hud>0) {
        int w{},h{};SDL_Vulkan_GetDrawableSize(window,&w,&h);
        const auto hud_player=campaign_hud_snapshot?*campaign_hud_snapshot:
            halo2::native_weapon_hud_player(simulation->player(),*content);
        const halo2::SourceHudAimCamera hud_camera{
                render_camera.position,render_camera.oriented?render_camera.forward:halo2::direction(render_camera.yaw,render_camera.pitch),
                render_camera.oriented?render_camera.up:halo2::cross(halo2::direction(render_camera.yaw,render_camera.pitch),
                    {-std::sin(render_camera.yaw),std::cos(render_camera.yaw),0}),
                render_camera.projection_vertical_angle.value_or(halo2::camera_vertical_field_of_view(render_camera,float(std::max(1,w))/std::max(1,h))),false};
        const auto layout=halo2::hud_viewport(1,0);
        const auto physical=[&](halo2::UiRect r){return halo2::full_surface_viewport(
            {r.left,r.top,r.right-r.left,r.bottom-r.top},unsigned(std::max(1,w)),unsigned(std::max(1,h)));};
        const auto aim_anchor=halo2::source_hud_aim_anchor(hud_camera,physical(layout.outer),physical(layout.safe),
            campaign_aim_displacement,gameplay_camera && !hud_player.dead);
        pause_draw.vertices=weapon_hud->frame(hud_player,std::uint32_t(simulation->ticks()),
            unsigned(std::max(1,w)),unsigned(std::max(1,h)),aim_anchor,hud_camera);
        pstencil_campaign_materials.request(*renderer,mesh.materials);
      }
      if(action_hud && play && simulation && !game_paused && effects.hud>0) {
        int w{},h{};SDL_Vulkan_GetDrawableSize(window,&w,&h);
        auto prompt=action_hud->frame(simulation->action_prompt(),desktop->keyboard_settings,
            desktop->controller_settings,prompt_gamepad,unsigned(std::max(1,w)),unsigned(std::max(1,h))).vertices;
        pause_draw.vertices.insert(pause_draw.vertices.end(),prompt.begin(),prompt.end());
      }
      if(campaign_text && simulation) {
        auto text=campaign_text->draw(simulation->training(),simulation->subtitle(),
            desktop->display_settings.subtitles,viewport_width,viewport_height,effects.letterbox,[&](const std::string &value){return halo2::training_control_text(value,desktop->controller_settings,desktop->keyboard_settings,prompt_gamepad);},simulation->titles());
        pause_draw.vertices.insert(pause_draw.vertices.end(),text.begin(),text.end());
      }
      if(pause_menu && pause_menu->visible()) {
        std::string objectives;
        const auto &states=simulation->objectives();
        for(unsigned i=0;i<states.size();++i)if(states[i]) {
          const auto text=objective_strings.find(0x0100030b+i);
          if(text!=objective_strings.end())objectives+=(states[i]==1?"\xe2\x98\x90\t":"\xe2\x98\x91\t")+text->second+"\r\n";
        }
        pause_menu->state().game_pause_objectives(std::move(objectives));
        int w{},h{};SDL_GetWindowSize(window,&w,&h);
        auto menu=pause_menu->draw(unsigned(std::max(1,w)),unsigned(std::max(1,h)));
        for(auto &backdrop:menu.backdrops)backdrop.before_vertex+=pause_draw.vertices.size();
        pause_draw.vertices.insert(pause_draw.vertices.end(),menu.vertices.begin(),menu.vertices.end());pause_draw.backdrops=std::move(menu.backdrops);
      }
      if(pstencil_campaign && source_frame && pstencil_campaign_camera && pstencil_campaign_renderer) {
        if(!character_shader_view || !character_fog)
          throw halo2::FormatError("pStencil campaign view lacks its completed camera/shader/fog inputs");
        const auto forward=character_camera.oriented?character_camera.forward:
            halo2::direction(character_camera.yaw,character_camera.pitch);
        pstencil_campaign_lighting=pstencil_campaign_renderer->draw(
            simulation->campaign_source()->objects().tu15_world(),*source_frame,
            {*pstencil_campaign_camera,*character_shader_view,*character_fog,character_camera.position,
             forward,character_distance_scale,pstencil_campaign_viewport,
             {std::uint32_t(character_width),std::uint32_t(character_height)},
             std::bit_cast<std::int32_t>(std::uint32_t(frames)+1),simulation->presentation().cinematic});
      }
      campaign_world.clear();
      std::optional<halo2::ModelView> campaign_view;
      if(character_atmosphere) {
        campaign_view.emplace();auto& view=*campaign_view;
        view.camera=character_camera;view.width=640;view.height=480;view.full_surface=true;
        // The BSP mesh survives this immediate submission. With no appended
        // objects, borrow its unchanged stream instead of copying every vertex.
        const auto* copy_world=std::getenv("HALO2_REFERENCE_COPY_CAMPAIGN_WORLD");
        if(debug.empty() && !(copy_world && *copy_world))view.vertices=mesh.vertices;
        else {
          campaign_world.reserve(mesh.vertices.size()+debug.size());
          campaign_world.insert(campaign_world.end(),mesh.vertices.begin(),mesh.vertices.end());
          campaign_world.insert(campaign_world.end(),debug.begin(),debug.end());
          view.vertices=campaign_world;
        }
        if(source_frame && source_frame->main_commands_completed && !campaign_transparent_models.empty()) {
          if(const auto order=halo2::source_model_transparent_order_tu15(source_frame->transparent_commands,
              source_frame->base_draws,campaign_transparent_models,render_camera.position)) {
            std::vector<halo2::ModelObjectDraw> ordered;ordered.reserve(order->size());
            for(const auto index:*order)ordered.push_back(std::move(character_draws.at(index)));
            for(std::size_t i=0;i<ordered.size();++i)
              character_draws.at(campaign_transparent_models[i].draw)=std::move(ordered[i]);
          }
        }
        view.objects=character_draws;
        if(campaign_weapon_gpu_trace.is_open())view.completed_object_draw_observer=[&](const halo2::RendererObservedObjectDraw &draw) {
          const auto owner=campaign_weapon_draw_owners.find(draw.object_index);
          if(owner==campaign_weapon_draw_owners.end())return;
          ++campaign_weapon_gpu_draws;
          trace_campaign_weapon({{"event","gpu_draw"},{"frame",frames},{"object_index",draw.object_index},
              {"owner",owner->second[0]},{"definition",owner->second[1]},{"ordinal",owner->second[2]},
              {"phase",unsigned(draw.phase)},{"source_material",draw.source_material},
              {"material",draw.material_slot},{"parent_material",draw.parent_material},
              {"pipeline",draw.pipeline},{"descriptor",draw.descriptor},{"vertex_buffer",draw.vertex_buffer},
              {"first",draw.first},{"count",draw.count},{"owner_constants",draw.owner_constants}});
        };
        if(source_frame) {
          view.source_bsp_admission=&source_frame->bsp;
          view.world_particles=source_frame->world_particles;
        }
        view.lens_queries=campaign_lens_query_draws;
        view.lens_draws=campaign_lens_final_draws;
        view.projection_clip_planes=character_clip_planes;
        view.source_w_distance=character_w_distance;
        view.pstencil_lightmap_casters=pstencil_campaign_shadows.casters();
        if(pstencil_campaign_lighting) {
          view.raw_effect_lights=pstencil_campaign_lighting->draws;
          view.pstencil_point_casters=pstencil_campaign_lighting->casters;
          view.pstencil_native_point_casters=true;
        }
        view.foreground=foreground;view.background=background;view.effect_time=character_time;
        view.source_render_policy=halo2::source_geometry_lod_policy(1);
        view.source_view_shader_constants=character_camera_constants;
        if(render_camera.projection_vertical_angle && simulation && play &&
            !simulation->presentation().scripted && !simulation->presentation().cinematic)
          view.foreground_projection_vertical_angle=halo2::gameplay_first_person_projection_angle;
        view.atmosphere=character_fog;
        view.atmosphere_source=&*character_atmosphere;
      }
      pstencil_campaign_materials.flush(*renderer,mesh.materials,
          {campaign_view?std::span<const halo2::Vertex>{}:std::span<const halo2::Vertex>{mesh.vertices},
           campaign_view?std::span<const halo2::Vertex>{}:std::span<const halo2::Vertex>{debug},
           campaign_view?std::span<const halo2::Vertex>{}:std::span<const halo2::Vertex>{foreground},
           campaign_view?std::span<const halo2::Vertex>{}:std::span<const halo2::Vertex>{background},pause_draw.vertices,
           campaign_view?std::span<const halo2::ModelView>{&*campaign_view,1}:std::span<const halo2::ModelView>{}},frames);
      if(const auto* trace=campaign_frame_trace) {
        static std::ofstream out(trace);
        out<<"{\"frame\":"<<frames<<",\"tick\":"<<(simulation?simulation->ticks():0)
           <<",\"bsp\":"<<bsp<<",\"pstencil\":"<<pstencil_campaign
           <<",\"attempted\":"<<pstencil_campaign_shadows.attempted()<<",\"owners\":[";
        bool comma=false;for(const auto& owner:pstencil_campaign_shadows.owners()) {
          if(comma)out<<',';comma=true;out<<"{\"object\":"<<owner.object<<",\"sample_owner\":"<<owner.sample_owner
              <<",\"triangles\":"<<owner.triangles<<",\"incident\":["<<owner.incident.x<<','<<owner.incident.y<<','<<owner.incident.z<<"]}";
        }out<<']';
        if(simulation) {
          const auto crossfade=simulation->cinematic_crossfade_frame();
          out<<",\"crossfade_capture\":"<<crossfade.capture
             <<",\"crossfade_composite\":"<<bool(crossfade.composite);
        }
        if(diagnostic_source_camera) {
          out<<",\"camera_command\":"<<simulation->cinematic_camera_discontinuity()
             <<",\"camera_restores\":"<<simulation->checkpoint_restores()
             <<",\"camera_paired\":"<<presentation_paired
             <<",\"camera_fraction\":"<<simulation->frame_fraction();
          const auto pose_bits=[&](const auto &position,const auto &forward,const auto &up) {
            out<<'[';bool separator=false;
            for(const auto &v:{position,forward,up})for(float value:{v.x,v.y,v.z}) {
              if(separator)out<<',';separator=true;out<<std::bit_cast<std::uint32_t>(value);
            }out<<']';
          };
          out<<",\"source_camera_bits\":";
          pose_bits(diagnostic_source_camera->position,diagnostic_source_camera->forward,diagnostic_source_camera->up);
          out<<",\"render_camera_bits\":";pose_bits(render_camera.position,render_camera.forward,render_camera.up);
        }
        out<<"}\n";out.flush();
      }
      const auto pstencil_capture=simulation?halo2::pstencil::campaign_capture_path(simulation->ticks()):std::filesystem::path{};
      const bool campaign_controls=campaign_view && !pstencil_capture.empty() &&
          (!simulation || (!simulation->cinematic_crossfade_frame().capture && !simulation->cinematic_crossfade_frame().composite));
      const bool pstencil_controls=campaign_controls && pstencil_campaign;
      const auto draw_pstencil_capture=[&](const halo2::ModelView& view,const std::filesystem::path& path) {
            // Repeated diagnostic submissions cannot reissue this frame's actual
            // query identities or publish additional source display stamps.
            auto diagnostic_view=view;diagnostic_view.lens_queries={};diagnostic_view.lens_draws={};
            renderer->draw(render_camera,path,{},play && !(simulation && simulation->presentation().cinematic),{},
                effects,pause_draw.vertices,std::span<const halo2::ModelView>{&diagnostic_view,1},{},pause_draw.backdrops);
      };
      if(pstencil_controls)halo2::pstencil::campaign_capture_controls(*renderer,*campaign_view,
          pstencil_capture,draw_pstencil_capture);
      const bool campaign_submitted=renderer->draw(
              render_camera, !pstencil_capture.empty()?pstencil_capture:capture ? screenshot : std::filesystem::path{},
              campaign_view?std::span<const halo2::Vertex>{}:std::span<const halo2::Vertex>{debug},
              play && !(simulation && simulation->presentation().cinematic),
              campaign_view?std::span<const halo2::Vertex>{}:std::span<const halo2::Vertex>{foreground},
              effects,pause_draw.vertices,campaign_view?std::span<const halo2::ModelView>{&*campaign_view,1}:std::span<const halo2::ModelView>{},
              campaign_view?std::span<const halo2::Vertex>{}:std::span<const halo2::Vertex>{background},pause_draw.backdrops);
      if(campaign_weapon_gpu_trace.is_open())trace_campaign_weapon({{"event","frame"},{"frame",frames},
          {"submitted",campaign_submitted},{"source_weapon_units",campaign_weapon_draw_owners.size()},
          {"weapon_gpu_draws",campaign_weapon_gpu_draws},
          {"lens_query_draws",campaign_lens_query_draws.size()},
          {"lens_final_draws",campaign_lens_final_draws.size()},
          {"lens_completed_results",campaign_lens_frame&&campaign_lens_queries ? campaign_lens_queries->completed().size():0}});
      if(campaign_lens_frame) {
        auto& world=simulation->campaign_source()->objects().tu15_world();
        campaign_lens_queries->finish(world,campaign_submitted);
        campaign_lens_draws->finish(campaign_submitted);
      }
      if(campaign_submitted)
        ++frames;
      else
        SDL_Delay(10);
      if(campaign_controls)
        halo2::pstencil::campaign_material_capture_control(*renderer,mesh.materials,*campaign_view,
            pstencil_capture,[&](const halo2::ModelView& view,const std::filesystem::path& path) {
              // Queries have source-owned identities and cannot be repeated.
              // Lens color quads are completed immutable draw packets; retain
              // them so the full-bank control contains the same final pixels.
              auto control=view;control.lens_queries={};
              renderer->draw(render_camera,path,{},play && !(simulation && simulation->presentation().cinematic),{},
                  effects,pause_draw.vertices,std::span<const halo2::ModelView>{&control,1},{},pause_draw.backdrops);
            });
      if(pstencil_controls) {
        halo2::pstencil::campaign_capture_controls(*renderer,*campaign_view,
            pstencil_capture,draw_pstencil_capture,true);
      }
      if(simulation && renderer->cinematic_crossfade_captured())simulation->cinematic_crossfade_capture_complete();
    }
    std::cout << "Rendered " << frames << " frames\n";
    if(controller)SDL_GameControllerRumble(controller.get(),0,0,0);
    if (simulation)
      std::cout << simulation->status() << " level_finished=" << simulation->level_finished() << '\n';
    if (bsp_replay && (!replay_streamed || !replay_reverted ||
                      simulation->checkpoint_restores()!=1 || simulation->bsp_revision()!=2))
      throw halo2::FormatError("BSP replay did not complete streaming and restore");
    if (simulation)
      for (const auto &fault : simulation->scripts().diagnostics())
        std::cerr << "Script fault: " << fault << '\n';
    if (audio)
      std::cout << "Audio mixed_frames=" << audio->mixed_frames()
                << " peak=" << audio->peak() << '\n';
    if(scripted_audio)std::cout<<"Authored scripted audio mixed_frames="<<scripted_audio->mixed_frames()<<'\n';
    return simulation && !simulation->scripts().diagnostics().empty()
               ? EXIT_FAILURE
               : EXIT_SUCCESS;
  } catch (const std::exception &error) {
    std::cerr << "error: " << error.what() << '\n';
    return EXIT_FAILURE;
  }
}
