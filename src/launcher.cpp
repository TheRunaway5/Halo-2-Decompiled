#include <SDL.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <process.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace {
#ifdef HALO2_TITLE_FIRST
constexpr bool title_first = true;
#else
constexpr bool title_first = false;
#endif
#ifdef HALO2_XBOX_GAMEPLAY_HUD
constexpr bool original_weapon_hud = true;
#else
constexpr bool original_weapon_hud = false;
#endif
namespace fs = std::filesystem;
constexpr std::array shader_names{"world.vert.spv",   "world.frag.spv",
                                  "world_emblem.geom.spv", "world_emblem.frag.spv",
                                  "fog.vert.spv",     "fog.frag.spv",
                                  "water_copy.frag.spv", "bsp_patchy_noise.frag.spv", "bsp_patchy_composite.frag.spv",
                                  "patchy_noise.frag.spv", "patchy_composite.frag.spv",
                                  "bloom.vert.spv", "bloom.frag.spv", "crt_oled.frag.spv", "display.frag.spv",
                                  "overlay.vert.spv", "overlay.frag.spv",
                                  "ui.vert.spv",      "ui.frag.spv", "hud.frag.spv"};
#ifdef _WIN32
constexpr auto native_name = "halo2_native.exe";
#else
constexpr auto native_name = "halo2_native";
#endif
struct Installation {
  fs::path root, runtime, build;
};
bool current_build(const fs::path &root,const fs::path &build) {
  const auto cache=build/"CMakeCache.txt";
  if(!fs::exists(cache))return true;
  std::ifstream file(cache);
  std::string line,source,directory;
  while(std::getline(file,line)) {
    if(!line.empty() && line.back()=='\r')line.pop_back();
    const std::string source_key="CMAKE_HOME_DIRECTORY:INTERNAL=";
    const std::string directory_key="CMAKE_CACHEFILE_DIR:INTERNAL=";
    if(line.starts_with(source_key))source=line.substr(source_key.size());
    if(line.starts_with(directory_key))directory=line.substr(directory_key.size());
  }
  return !source.empty() && !directory.empty() &&
      fs::weakly_canonical(source)==root &&
      fs::weakly_canonical(directory)==fs::weakly_canonical(build);
}
Installation locate() {
  const std::unique_ptr<char, decltype(&SDL_free)> base(SDL_GetBasePath(),
                                                        SDL_free);
  if (!base)
    throw std::runtime_error("Could not locate the launcher: " +
                             std::string(SDL_GetError()));
  fs::path directory(base.get());
  // SDL includes a trailing separator. Remove that empty final component so
  // parent_path() checks the project above a bin directory, retaining roots.
  if (directory.filename().empty() && directory != directory.root_path())
    directory = directory.parent_path();
  const std::vector<fs::path> roots{directory, directory.parent_path(),
                                    HALO2_PROJECT_DIR};
  for (const auto &root : roots) {
    if (!fs::is_regular_file(root / "tools/setup_assets.py"))
      continue;
    const auto project = fs::canonical(root);
    std::vector<fs::path> binaries{directory / native_name,
                                  project / "build" / native_name};
    // Installed local launchers retain their configured build directory.
    // A relocated launcher only searches the project beside it.
    std::error_code configured_error;
    const auto configured_project = fs::canonical(HALO2_PROJECT_DIR, configured_error);
    auto build=project/"build";
    if (!configured_error && configured_project == project) {
      build=fs::path(HALO2_RUNTIME_DIR);
      if(build.is_relative())build=project/build;
      binaries.insert(binaries.begin() + 1, build / native_name);
    }
    for (const auto &binary : binaries) {
      if (!fs::is_regular_file(binary) || !current_build(project,binary.parent_path()))
        continue;
      const auto shader_dir = binary.parent_path() / "shaders";
      const bool shaders =
          std::all_of(std::begin(shader_names), std::end(shader_names),
                      [&](const char *name) {
                        return fs::is_regular_file(shader_dir / name);
                      });
      if (shaders)
        return {project, fs::canonical(binary),
            fs::is_regular_file(binary.parent_path()/"CMakeCache.txt")?binary.parent_path():build};
    }
    // First-run source selection also works before the runtime is built.
    return {project, {}, build};
  }
  throw std::runtime_error(
      "Setup tools were not found. Keep the launcher with the project folder, "
      "including tools/setup_assets.py and tools/setup_assets_ui.py.");
}
constexpr auto not_playable = "MCC content is cached. Native gameplay loading and complete Xbox TU1.5 conversion are still being completed.";
struct AssetSources {
  fs::path cache;
  bool playable = false;
  std::string problem;
  bool ready() const { return !cache.empty(); }
  fs::path maps() const { return cache/"halo2/h2_maps_win64_dx11"; }
};
AssetSources read_sources(const fs::path &root) {
  const auto registry=root/"assets/mcc-source.tsv";
  if(!fs::exists(registry))return {};
  try {
    std::ifstream file(registry);
    std::string header,row,extra;
    if(!std::getline(file,header) || header!="H2_MCC_ASSET_SOURCE_V1" ||
        !std::getline(file,row) || !row.starts_with("cache\t") || row.size()==6 ||
        row.find_first_of("\t\r\n\0",6,4)!=row.npos || std::getline(file,extra))
      throw std::runtime_error("Invalid MCC cache selection. Run --setup to scan MCC again.");
    fs::path selected=row.substr(6);
    if(selected.is_relative())selected=root/selected;
    if(fs::is_symlink(selected))throw std::runtime_error("The MCC cache must be an owned directory.");
    selected=fs::canonical(selected);
    if(selected.parent_path()!=root/"assets/imported" || selected.filename().string().starts_with("."))
      throw std::runtime_error("Select a completed private MCC import under assets/imported.");
    for(const auto *name:{"halo2/h2_maps_win64_dx11/mainmenu.map", "halo2/h2_maps_win64_dx11/shared.map",
        "halo2/h2_maps_win64_dx11/single_player_shared.map", "halo2/h2_maps_win64_dx11/lockout.map",
        "halo2/h2_maps_win64_dx11/textures.dat", "halo2/h2_fonts/font_table.txt",
        "bitmap-resources.json", "tag-conversion.json", "loading/mcc-loading-assets.json", "provenance.json"}) {
      const auto path=selected/name;
      if(!fs::is_regular_file(path) || fs::is_symlink(path) ||
          fs::canonical(path).lexically_relative(selected).string().starts_with(".."))
        throw std::runtime_error("MCC cache is incomplete: "+std::string(name));
    }
    std::ifstream provenance(selected/"provenance.json");
    const auto report=nlohmann::json::parse(provenance);
    if(report.value("schema",std::string{})!="H2_MCC_TU15_PREPARATION_V1" ||
        report.value("external_installation_requirement",std::string{})!="Halo MCC only")
      throw std::runtime_error("Selected cache is not an MCC-only preparation.");
    const auto enabled=[&](const char *name) {
      return report.contains(name) && report[name].is_boolean() && report[name].get<bool>();
    };
    return {selected,enabled("runtime_active") && enabled("full_tu15_parity"),{}};
  } catch(const std::exception &error) {
    return {{},false,error.what()};
  }
}
int run_tool(std::vector<std::string> arguments) {
  std::vector<char*> pointers;
  for(auto &value:arguments)pointers.push_back(value.data());
  pointers.push_back(nullptr);
  std::cout.flush();std::cerr.flush();
#ifdef _WIN32
  const auto status=_spawnvp(_P_WAIT,arguments.front().c_str(),pointers.data());
  if(status!=-1)return int(status);
#else
  const auto child=fork();
  if(child==0) {execvp(arguments.front().c_str(),pointers.data());_exit(127);}
  if(child>0) {
    int status{};
    while(waitpid(child,&status,0)<0) {
      if(errno!=EINTR)throw std::runtime_error("Could not wait for "+arguments.front());
    }
    return WIFEXITED(status)?WEXITSTATUS(status):1;
  }
#endif
  throw std::runtime_error("Could not start "+arguments.front());
}
int prepare_sources(const Installation &installation,const std::string &mcc,bool interactive) {
  std::vector<std::string> arguments{HALO2_PYTHON_EXECUTABLE,
      (installation.root/"tools/setup_assets.py").string(),"--root",installation.root.string()};
  if(mcc.empty() && interactive)arguments.push_back("--interactive");
  if(!mcc.empty())arguments.insert(arguments.end(),{"--mcc-root",mcc});
  return run_tool(std::move(arguments));
}
void preview_arguments(std::vector<std::string> &arguments,const Installation &installation,const AssetSources &sources) {
  for(std::size_t i=0;i<arguments.size();++i) {
    const auto option=arguments[i];
    if(option=="--hidden" || option=="--validation" || option=="--overview" || option=="--mute")continue;
    const auto count=option=="--viewport"?2u:
        (option=="--map" || option=="--bsp" || option=="--frames" || option=="--screenshot")?1u:0u;
    if(!count || arguments.size()-i-1<count)
      throw std::runtime_error("MCC content preview accepts only map, BSP, viewport and capture options; gameplay is still being completed.");
    if(option=="--map") {
      fs::path candidate=arguments[i+1];
      if(candidate.is_relative()) {
        if(fs::is_regular_file(installation.root/candidate))candidate=installation.root/candidate;
        else candidate=candidate.has_parent_path()?sources.cache/"halo2"/candidate:sources.maps()/candidate;
      }
      const auto owned=fs::canonical(sources.cache/"halo2");
      if(!fs::is_regular_file(candidate) || fs::is_symlink(candidate))
        throw std::runtime_error("Select an imported MCC map from the private cache.");
      candidate=fs::canonical(candidate);
      const auto relative=candidate.lexically_relative(owned);
      auto extension=candidate.extension().string();
      std::transform(extension.begin(),extension.end(),extension.begin(),[](unsigned char c){return char(std::tolower(c));});
      if(relative.empty() || *relative.begin()==".." || extension!=".map")
        throw std::runtime_error("MCC content preview requires a map inside the selected private MCC cache.");
      arguments[i+1]=candidate.string();
    }
    i+=count;
  }
}
fs::path launcher_executable(const fs::path &root) {
#if defined(__linux__)
  std::error_code error;
  auto path=fs::read_symlink("/proc/self/exe",error);
  if(!error && path.is_absolute())return path;
#elif defined(_WIN32)
  std::vector<wchar_t> name(32768);
  const auto size=GetModuleFileNameW(nullptr,name.data(),DWORD(name.size()));
  if(size && size<name.size())return fs::path(std::wstring(name.data(),size));
#endif
  return root/(title_first?"Halo2-Title-Launcher":"Halo2-Launcher");
}
} // namespace

int main(int argc, char **argv) {
  try {
    auto installation = locate();
    std::vector<std::string> supplied;
    std::string mcc_source;
    bool setup=false,setup_only=false,update=false,launch_game=false,prepare=false,preview=false;
    for(int i=1;i<argc;++i) {
      const std::string argument=argv[i];
      if(argument=="--setup" || argument=="--setup-only") {
        setup=true;setup_only|=argument=="--setup-only";
      } else if(argument=="--mcc-source") {
        if(++i>=argc || !*argv[i] || std::string(argv[i]).starts_with("--"))
          throw std::runtime_error("missing value for --mcc-source (expected the MCC installation or halo2 folder)");
        mcc_source=argv[i];setup=true;
      } else if(argument=="--vista-source" || argument=="--xbox-source" ||
          argument=="--gameplay-xbe" || argument=="--menu-vista-settings" ||
          argument=="--import-mcc-hud" || argument=="--import-mcc-menus") {
        throw std::runtime_error(argument+" is no longer a launcher option. Use --setup [--mcc-source PATH] to import MCC content.");
      } else if(argument=="--update")update=true;
      else if(argument=="--launch-game")launch_game=true;
      else if(argument=="--preview-mcc")preview=true;
      else if(argument=="--prepare-mcc-gameplay") {prepare=true;setup=true;setup_only=true;}
      else supplied.push_back(argument);
    }
    auto has = [&](const char *option) {
      return std::find(supplied.begin(), supplied.end(), option) != supplied.end();
    };
    if(prepare && (update || launch_game || preview || !supplied.empty()))
      throw std::runtime_error("Use --prepare-mcc-gameplay [--mcc-source PATH] [--setup-only] by itself");
    if(preview && (update || launch_game || setup_only))
      throw std::runtime_error("Use --preview-mcc [--map NAME] with preview options.");
    auto sources=read_sources(installation.root);
    const bool help=has("--help") || has("-h");
    if(has("--launcher-info")) {
      std::cout << "Project: " << installation.root.string() << '\n'
                << "Runtime: " << (installation.runtime.empty()?"not built":installation.runtime.string()) << '\n'
                << (title_first?"Default: title screen (skip boot intro) and menus\n":"Default: startup intro, title screen and menus\n")
                << "Asset source: Halo MCC only\n"
                << "MCC cache prepared: " << (sources.ready()?"yes":"no — scan MCC with --setup") << '\n'
                << "MCC cache: " << sources.cache.string() << '\n'
                << "MCC maps: " << (sources.ready()?sources.maps().string():"") << '\n'
                << "Native MCC gameplay ready: " << (sources.playable?"yes":"no") << '\n'
                << "MCC content preview: " << (sources.ready()?"available":"import MCC first") << '\n'
                << "Status: " << (!sources.problem.empty()?sources.problem:
                    !sources.ready()?"MCC has not been imported yet. Run --setup.":
                    !sources.playable?not_playable:"MCC detection and private-cache import selected") << '\n'
                << "Shaders: existing native Vulkan shaders\n";
      return 0;
    }
    if(help) {
      std::cout << "MCC setup: --setup [--mcc-source PATH] [--setup-only]\n"
          "First launch scans installed MCC libraries. Browse to the MCC or halo2 folder if needed.\n"
          "Content is copied into the private PC-port cache; existing shaders are preserved.\n"
          "--prepare-mcc-gameplay [--mcc-source PATH] prepares and selects that cache, then exits.\n"
          "--launcher-info reports cache and native-gameplay readiness.\n"
          "--preview-mcc [--map NAME] previews imported map assets with the existing renderer.\n"
          "--update downloads changes and rebuilds; --launch-game skips the launcher home.\n";
      return 0;
    }
    if(update) {
      if(setup || launch_game || !supplied.empty())
        throw std::runtime_error("Use --update by itself to download changes and rebuild the game");
      return run_tool({HALO2_PYTHON_EXECUTABLE,
          (installation.root/"tools/update_runtime.py").string(),"--root",
          installation.root.string(),"--build-dir",installation.build.string()});
    }
    if(setup || !sources.ready()) {
      const auto status=prepare_sources(installation,mcc_source,!prepare);
      if(status)return status;
      sources=read_sources(installation.root);
      if(!sources.ready())throw std::runtime_error("MCC setup did not publish a complete private cache. "+sources.problem);
    }
    if(setup_only)return 0;
    const auto update_pending=installation.root/"launcher-update.pending";
    if(!launch_game && !preview && supplied.empty()) {
      std::vector<std::string> arguments{HALO2_PYTHON_EXECUTABLE,
          (installation.root/"tools/launcher_ui.py").string(),"--root",
          installation.root.string(),"--build-dir",installation.build.string(),
          "--launcher-executable",launcher_executable(installation.root).string()};
      if(!sources.playable)arguments.insert(arguments.end(),{"--runtime-unavailable",not_playable});
      const auto status=run_tool(std::move(arguments));
      if(status==20)return 0;
      if(status)return status;
      installation=locate();sources=read_sources(installation.root);
    }
    if(!sources.ready())throw std::runtime_error("The MCC cache needs preparation. Run --setup. "+sources.problem);
    if(!sources.playable && !preview)throw std::runtime_error(not_playable);
    if(preview)preview_arguments(supplied,installation,sources);
    if(installation.runtime.empty() || fs::exists(update_pending)) {
      const auto tool=installation.root/"tools/build_runtime.py";
      if(!fs::is_regular_file(tool))throw std::runtime_error("Automatic build tool is missing: "+tool.string());
      const auto status=run_tool({HALO2_PYTHON_EXECUTABLE,tool.string(),"--root",
          installation.root.string(),"--build-dir",installation.build.string()});
      if(status)return status;
      installation=locate();
      if(!installation.runtime.empty())fs::remove(update_pending);
    }
    if(installation.runtime.empty())throw std::runtime_error(
        "The automatic build finished, but the runtime or required shaders are missing. See "+
        (installation.build/"launcher-build.log").string());
    std::vector<std::string> arguments{installation.runtime.string()};
    if(preview) {
      arguments.push_back("--mcc-preview");
    }
    const bool custom =
        preview || has("--menu") || has("--menu-screen") || has("--menu-audit") ||
        has("--menu-sweep") || has("--menu-maps") || has("--menu-movie") || has("--play") ||
        has("--campaign") || has("--simulate") || has("--map") ||
        has("--multiplayer") || has("--custom-slayer") || has("--host") || has("--join") || has("--local-players") || has("--dedicated") ||
        has("--help") || has("-h") || has("--inspect") ||
        has("--inspect-systems") || has("--validate-maps") ||
        has("--validate-systems") || has("--decode-sound");
    if (!custom) {
      arguments.push_back("--menu-screen");
      arguments.push_back("9");
      if (title_first)
        arguments.push_back("--skip-intro");
    }
    const bool menu_route = !custom || has("--menu") || has("--menu-screen") ||
                            has("--menu-sweep") || has("--menu-audit") ||
                            has("--menu-maps") || has("--menu-movie");
    if(menu_route && !has("--menu-maps"))
      arguments.insert(arguments.end(),{"--menu-maps",sources.maps().string()});
    if(!has("--map"))arguments.insert(arguments.end(),{"--map",(sources.maps()/"lockout.map").string()});
    if(!has("--shared-maps"))arguments.insert(arguments.end(),{"--shared-maps",sources.maps().string()});
    // Future decoded UI packs must be owned by this selected MCC import.
    // Do not read the historical per-platform source registries.
    const auto ui=sources.cache/"ui";
    if(original_weapon_hud && !has("--hud-ui-assets"))arguments.insert(arguments.end(),{"--hud-ui-assets",""});
    for(const auto &[option,manifest]:std::array{
        std::pair{"--menu-ui-assets","pack.tsv"},std::pair{"--hud-ui-assets","hud.tsv"},
        std::pair{"--pause-ui-assets","pause.tsv"}})
      if(!(original_weapon_hud && std::string(option)=="--hud-ui-assets") && !has(option) && !fs::is_symlink(ui) && fs::is_regular_file(ui/manifest) &&
          !fs::is_symlink(ui/manifest) &&
          !fs::canonical(ui/manifest).lexically_relative(sources.cache).string().starts_with(".."))
        arguments.insert(arguments.end(),{option,ui.string()});
    arguments.insert(arguments.end(), supplied.begin(), supplied.end());
    // The native runtime's map/shared-resource paths are relative to the
    // project. Process arguments go directly to the runtime; no shell is
    // involved.
    fs::current_path(installation.root);
    // Discord must relaunch through this launcher so the selected MCC cache
    // and runtime readiness are resolved again. No OAuth token or
    // party join secret is placed in the launch command or process arguments.
#ifdef _WIN32
    std::vector<wchar_t> launcher_name(32768);
    const auto launcher_size=GetModuleFileNameW(nullptr,launcher_name.data(),DWORD(launcher_name.size()));
    if(launcher_size && launcher_size<launcher_name.size())
      _putenv_s("H2_DISCORD_LAUNCHER",fs::path(std::wstring(launcher_name.data(),launcher_size)).string().c_str());
#elif defined(__linux__)
    std::error_code launcher_error;
    const auto launcher=fs::read_symlink("/proc/self/exe",launcher_error);
    if(!launcher_error && launcher.is_absolute())setenv("H2_DISCORD_LAUNCHER",launcher.c_str(),1);
#endif
    std::vector<char *> pointers;
    for (auto &argument : arguments)
      pointers.push_back(argument.data());
    pointers.push_back(nullptr);
    std::cout.flush();
    std::cerr.flush();
#ifdef _WIN32
    const auto status = _spawnv(_P_WAIT, installation.runtime.string().c_str(),
                                pointers.data());
    if (status != -1)
      return int(status);
#else
    execv(installation.runtime.c_str(), pointers.data());
#endif
    throw std::runtime_error("Could not start the game: " +
                             std::string(std::strerror(errno)));
  } catch (const std::exception &error) {
    std::cerr << "Halo 2 launcher: " << error.what() << '\n';
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Halo 2 native launcher",
                             error.what(), nullptr);
    return 1;
  }
}
