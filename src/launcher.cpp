#include <SDL.h>
#include <algorithm>
#include <array>
#include <cerrno>
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
        return {project, fs::canonical(binary), binary.parent_path()};
    }
    // First-run source selection also works before the runtime is built.
    return {project, {}, build};
  }
  throw std::runtime_error(
      "Setup tools were not found. Keep the launcher with the project folder, "
      "including tools/setup_assets.py and tools/setup_assets_ui.py.");
}
struct AssetSources {
  fs::path xbox, vista;
  bool ready() const {
    return !xbox.empty() && !vista.empty() &&
        fs::is_regular_file(xbox/"default.xbe") &&
        fs::is_regular_file(xbox/"maps/mainmenu.map") &&
        fs::is_regular_file(xbox/"maps/shared.map") &&
        fs::is_regular_file(xbox/"maps/lockout.map") &&
        fs::is_regular_file(xbox/"maps/fonts/font_table.txt") &&
        fs::is_regular_file(vista/"pack.tsv") && fs::is_regular_file(vista/"hud.tsv") &&
        fs::is_regular_file(vista/"maps/mainmenu.map") &&
        fs::is_regular_file(vista/"maps/shared.map") &&
        fs::is_regular_file(vista/"maps/lockout.map") && fs::is_directory(vista/"fonts");
  }
};
AssetSources read_sources(const fs::path &root) {
  std::ifstream file(root/"assets/asset-sources.tsv");
  std::string line;
  if(!std::getline(file,line) || line!="H2_ASSET_SOURCES_V1")return {};
  AssetSources result;
  while(std::getline(file,line)) {
    const auto tab=line.find('\t');
    if(tab==std::string::npos || line.find('\t',tab+1)!=std::string::npos ||
        line.find_first_of("\r\n\0",0,3)!=std::string::npos)return {};
    const auto name=line.substr(0,tab);
    fs::path value=line.substr(tab+1);
    if(value.empty())return {};
    if(value.is_relative())value=root/value;
    if(name=="xbox" && result.xbox.empty())result.xbox=value;
    else if(name=="vista" && result.vista.empty())result.vista=value;
    else return {};
  }
  return result;
}
fs::path read_hud_source(const fs::path &root,const AssetSources &sources) {
  const auto registry=root/"assets/hud-source.tsv";
  if(!fs::exists(registry))return sources.vista;
  std::ifstream file(registry);std::string header,line,extra;
  if(!std::getline(file,header) || header!="H2_HUD_SOURCE_V1" ||
      !std::getline(file,line) || !line.starts_with("mcc\t") || line.size()==4 ||
      line.find_first_of("\t\r\n\0",4,4)!=line.npos || std::getline(file,extra))
    throw std::runtime_error("Invalid selected HUD source: "+registry.string());
  fs::path path=line.substr(4);if(path.is_relative())path=root/path;
  for(const auto *name:{"hud.tsv","hud-source.tsv","scoreboard.tsv","fonts/font_table.txt"})
    if(!fs::is_regular_file(path/name))
      throw std::runtime_error("Selected MCC Classic HUD import is incomplete: "+path.string());
  return fs::canonical(path);
}
fs::path read_menu_source(const fs::path &root,const AssetSources &sources) {
  const auto registry=root/"assets/menu-source.tsv";
  if(!fs::exists(registry))return sources.vista;
  std::ifstream file(registry);std::string header,line,extra;
  if(!std::getline(file,header) || header!="H2_MENU_SOURCE_V1" ||
      !std::getline(file,line) || !line.starts_with("mcc\t") || line.size()==4 ||
      line.find_first_of("\t\r\n\0",4,4)!=line.npos || std::getline(file,extra))
    throw std::runtime_error("Invalid selected menu source: "+registry.string());
  fs::path path=line.substr(4);if(path.is_relative())path=root/path;
  for(const auto *name:{"pack.tsv","vista.tsv","pause.tsv","menu-source.tsv","menu-ui-source.tsv","fonts/font_table.txt"})
    if(!fs::is_regular_file(path/name))
      throw std::runtime_error("Selected MCC Classic menu import is incomplete: "+path.string());
  return fs::canonical(path);
}
fs::path read_gameplay_source(const fs::path &root) {
  const auto registry=root/"assets/gameplay-source.tsv";
  if(!fs::exists(registry))return {};
  std::ifstream file(registry);
  std::string header,line,extra;
  if(!std::getline(file,header) || header!="H2_GAMEPLAY_SOURCE_V1" ||
      !std::getline(file,line) || !line.starts_with("xbe\t") ||
      line.size()==4 || line.find_first_of("\t\r\n\0",4,4)!=std::string::npos ||
      std::getline(file,extra))
    throw std::runtime_error("Invalid gameplay source selection: "+registry.string());
  fs::path selected=line.substr(4);
  if(selected.is_relative())selected=root/selected;
  if(!fs::is_regular_file(selected))
    throw std::runtime_error("Selected gameplay executable is missing: "+selected.string()+
        ". Select it again with tools/select_gameplay_source.py.");
  return fs::canonical(selected);
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
int prepare_sources(const Installation &installation,const std::string &vista,
                    const std::string &xbox) {
  std::vector<std::string> arguments{HALO2_PYTHON_EXECUTABLE,
      (installation.root/"tools/setup_assets.py").string(),"--root",installation.root.string()};
  if(!vista.empty())arguments.insert(arguments.end(),{"--vista",vista});
  if(!xbox.empty())arguments.insert(arguments.end(),{"--xbox",xbox});
  if(vista.empty() || xbox.empty())arguments.push_back("--interactive");
  return run_tool(std::move(arguments));
}
int prepare_mcc_hud(const Installation &installation,const std::string &source) {
  std::vector<std::string> arguments{HALO2_PYTHON_EXECUTABLE,
      (installation.root/"tools/import_mcc_hud.py").string(),"--root",installation.root.string(),"--activate"};
  if(!source.empty())arguments.insert(arguments.end(),{"--mcc-root",source});
  return run_tool(std::move(arguments));
}
int prepare_mcc_menus(const Installation &installation,const std::string &source) {
  std::vector<std::string> arguments{HALO2_PYTHON_EXECUTABLE,
      (installation.root/"tools/import_mcc_menus.py").string(),"--root",installation.root.string(),"--activate"};
  if(!source.empty())arguments.insert(arguments.end(),{"--mcc-root",source});
  return run_tool(std::move(arguments));
}
} // namespace

int main(int argc, char **argv) {
  try {
    auto installation = locate();
    std::vector<std::string> supplied;
    std::string vista_source,xbox_source,mcc_source;
    bool setup=false,setup_only=false,import_mcc=false,import_menus=false,prepared_sources=false;
    for(int i=1;i<argc;++i) {
      const std::string argument=argv[i];
      if(argument=="--setup" || argument=="--setup-only") {
        setup=true;setup_only|=argument=="--setup-only";
      } else if(argument=="--vista-source" || argument=="--xbox-source" || argument=="--mcc-source") {
        if(++i>=argc || !*argv[i] || std::string(argv[i]).starts_with("--"))
          throw std::runtime_error("missing value for "+argument+" (expected a file or folder path)");
        if(argument=="--mcc-source")mcc_source=argv[i];
        else {
          (argument=="--vista-source"?vista_source:xbox_source)=argv[i];
          setup=true;
        }
      } else if(argument=="--import-mcc-hud")import_mcc=true;
      else if(argument=="--import-mcc-menus")import_menus=true;
      else supplied.push_back(argument);
    }
    if(!mcc_source.empty() && !import_mcc && !import_menus)import_mcc=true;
    auto has = [&](const char *option) {
      return std::find(supplied.begin(), supplied.end(), option) !=
             supplied.end();
    };
    auto sources=read_sources(installation.root);
    const bool help=has("--help") || has("-h");
    const auto gameplay=(!help && !has("--gameplay-xbe"))?
        read_gameplay_source(installation.root):fs::path{};
    if (has("--launcher-info")) {
      std::cout << "Project: " << installation.root.string() << '\n'
                << "Runtime: " << (installation.runtime.empty() ? "not built" : installation.runtime.string()) << '\n'
                << (title_first ? "Default: original title screen (skip boot intro) and menus\n"
                                : "Default: startup intro, original title screen and menus\n")
                << "Game files ready: " << (sources.ready()?"yes":"no — select both sources on launch") << '\n'
                << "Xbox game files: " << sources.xbox.string() << '\n'
                << "Vista HUD/GUI: " << sources.vista.string() << '\n'
                << "Selected HUD/fonts: " << read_hud_source(installation.root,sources).string() << '\n'
                << "Selected menus/fonts: " << read_menu_source(installation.root,sources).string() << '\n'
                << "ONLINE/clan UI/fonts: " << sources.vista.string() << '\n'
                << "Default pause UI/fonts: " << sources.vista.string() << '\n'
                << "Vista loading assets: " << (installation.root/"assets/menu-ui/vista/loading").string() << '\n'
                << "Default gameplay XBE: " << (gameplay.empty()?
                    "disc default.xbe (or explicit --gameplay-xbe override)":gameplay.string()) << '\n'
                << "Shaders: native Vulkan GLSL; Xbox material behavior reference\n";
      return 0;
    }
    if(help)std::cout<<"Launcher setup: --setup [--vista-source PATH --xbox-source PATH] [--setup-only]\n";
    if(help)std::cout<<"Normal launch prepares game files, builds the runtime when needed, and starts the game. --setup-only prepares files and exits.\n";
    if(help)std::cout<<"MCC Classic HUD: --import-mcc-hud [--mcc-source PATH] [--setup-only]\n";
    if(help)std::cout<<"MCC Classic menus: --import-mcc-menus [--mcc-source PATH] [--setup-only]\n";
    if(help && installation.runtime.empty())return 0;
    if(!help && ((setup && !import_mcc && !import_menus) || !sources.ready() || !vista_source.empty() || !xbox_source.empty())) {
      const auto status=prepare_sources(installation,vista_source,xbox_source);
      if(status)return status;
      sources=read_sources(installation.root);
      if(!sources.ready())throw std::runtime_error("Setup did not provide both Vista UI and Xbox game files");
      prepared_sources=true;
    }
    if(!help && import_mcc) {
      const auto status=prepare_mcc_hud(installation,mcc_source);
      if(status)return status;
    }
    if(!help && import_menus) {
      const auto status=prepare_mcc_menus(installation,mcc_source);
      if(status)return status;
    }
    if(setup_only)return 0;
    if(!help && (prepared_sources || installation.runtime.empty())) {
      const auto tool=installation.root/"tools/build_runtime.py";
      if(!fs::is_regular_file(tool))throw std::runtime_error("Automatic build tool is missing: "+tool.string());
      const auto status=run_tool({HALO2_PYTHON_EXECUTABLE,tool.string(),"--root",
          installation.root.string(),"--build-dir",installation.build.string()});
      if(status)return status;
      installation=locate();
    }
    if(installation.runtime.empty())throw std::runtime_error(
        "The automatic build finished, but the runtime or required shaders are missing. See "+
        (installation.build/"launcher-build.log").string());
    std::vector<std::string> arguments{installation.runtime.string()};
    if(!gameplay.empty())arguments.insert(arguments.end(),{"--gameplay-xbe",gameplay.string()});
    const bool custom =
        has("--menu") || has("--menu-screen") || has("--menu-audit") ||
        has("--menu-sweep") || has("--menu-maps") || has("--menu-movie") || has("--play") ||
        has("--campaign") || has("--simulate") || has("--map") ||
        has("--multiplayer") || has("--host") || has("--join") || has("--local-players") || has("--dedicated") ||
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
    if (menu_route && !has("--menu-maps")) {
      arguments.push_back("--menu-maps");
      arguments.push_back((sources.xbox/"maps").string());
    }
    if (menu_route && !has("--menu-ui-assets")) {
      arguments.push_back("--menu-ui-assets");
      arguments.push_back(read_menu_source(installation.root,sources).string());
    }
    if(menu_route && !has("--menu-loading-assets")) {
      arguments.push_back("--menu-loading-assets");
      arguments.push_back((installation.root/"assets/menu-ui/vista/loading").string());
    }
    const auto vista_settings=sources.vista/"maps/mainmenu.map";
    if(menu_route && fs::is_regular_file(vista_settings) && !has("--menu-vista-settings")) {
      arguments.push_back("--menu-vista-settings");
      arguments.push_back(vista_settings.string());
    }
    if(!help && !has("--hud-ui-assets")) {
      arguments.push_back("--hud-ui-assets");
      arguments.push_back(read_hud_source(installation.root,sources).string());
    }
    if(!help && !has("--pause-ui-assets")) {
      arguments.push_back("--pause-ui-assets");
      arguments.push_back(sources.vista.string());
    }
    if(!help && !menu_route && !has("--map") &&
        (has("--multiplayer") || has("--host") || has("--join") || has("--local-players") ||
         has("--dedicated") || has("--play") || has("--campaign") || has("--simulate"))) {
      arguments.push_back("--map");
      arguments.push_back((sources.xbox/"maps/lockout.map").string());
    }
    arguments.insert(arguments.end(), supplied.begin(), supplied.end());
    // The native runtime's map/shared-resource paths are relative to the
    // project. Process arguments go directly to the runtime; no shell is
    // involved.
    fs::current_path(installation.root);
    // Discord must relaunch through this launcher so that the selected Vista
    // assets and gameplay executable are resolved again. No OAuth token or
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
