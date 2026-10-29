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
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef __APPLE__
#include "halo2/macos/executable.hpp"
#include "halo2/macos/application.hpp"
#endif
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace {
// Keep the legacy executable names for shortcuts and release forwarding.
// Both entry points now play the intro before the title screen.
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
// Retained legacy selection fixtures; unselected candidates require the AOT
// package verifier below instead of presence checks on this historical bank.
[[maybe_unused]] constexpr std::array shader_names{"world.vert.spv",   "world.frag.spv",
                                  "world_emblem.geom.spv", "world_emblem.frag.spv",
                                  "fog.vert.spv",     "fog.frag.spv",
                                  "water_copy.frag.spv", "bsp_patchy_noise.frag.spv", "bsp_patchy_composite.frag.spv",
                                  "patchy_noise.frag.spv", "patchy_composite.frag.spv",
                                  "bloom.vert.spv", "bloom.frag.spv", "crt_oled.frag.spv", "display.frag.spv",
                                  "overlay.vert.spv", "overlay.frag.spv",
                                  "ui.vert.spv",      "ui.frag.spv", "hud.frag.spv"};
#ifdef _WIN32
constexpr auto native_name = "halo2_native.exe";
constexpr auto launcher_name = "Halo2-Launcher.exe";
constexpr auto title_launcher_name = "Halo2-Title-Launcher.exe";
constexpr auto foreign_native_name = "halo2_native";
int windows_environment(const char* name,const std::string& value);
#else
constexpr auto native_name = "halo2_native";
constexpr auto launcher_name = "Halo2-Launcher";
constexpr auto title_launcher_name = "Halo2-Title-Launcher";
constexpr auto foreign_native_name = "halo2_native.exe";
#endif
struct Installation {
  fs::path root, runtime, build, release_launcher;
  fs::path resources=root;
  bool packaged=false;
  bool qualified_multiplayer=false;
  std::string multiplayer_map;
  unsigned multiplayer_players=2;
  std::string multiplayer_video_driver;
  fs::path tool(const char *name) const {return resources/"tools"/name;}
  std::string python() const {
#ifdef _WIN32
    const auto interpreter=resources/"python/python.exe";
    if(!packaged) {
      if(fs::is_regular_file(interpreter))return interpreter.string();
      const fs::path configured=HALO2_PYTHON_EXECUTABLE;
      // Cross configuration uses a Linux host Python for code generation.
      // Source launchers on Windows instead use an installed Windows Python.
      return configured.extension()==".exe"?configured.string():"python";
    }
#else
    if(!packaged)return HALO2_PYTHON_EXECUTABLE;
    const auto interpreter=resources/"python/bin/python3";
#endif
    if(!fs::is_regular_file(interpreter))
      throw std::runtime_error("The launcher bundle is missing its Python setup runtime");
    return interpreter.string();
  }
};
void bind_installation(const Installation &installation) {
  const auto value=nlohmann::json{{"schema","H2_INSTALLATION_LAYOUT_V1"},
      {"data",installation.root.string()},{"resources",installation.resources.string()}}.dump();
#ifdef _WIN32
  auto failed=windows_environment("H2_INSTALLATION_LAYOUT",value);
  if(fs::is_regular_file(installation.resources/"python/python.exe")) {
    // The portable native interpreter and Tcl/Tk must resolve beside this
    // installation after relocation, rather than from their SDK build prefix.
    failed|=windows_environment("PYTHONHOME",(installation.resources/"python").string());
    failed|=windows_environment("TCL_LIBRARY",(installation.resources/"python/lib/tcl8.6").string());
    failed|=windows_environment("TK_LIBRARY",(installation.resources/"python/lib/tk8.6").string());
    failed|=windows_environment("PYTHONDONTWRITEBYTECODE","1");
  }
  if(installation.packaged)failed|=windows_environment("PYTHONDONTWRITEBYTECODE","1");
#else
  auto failed=setenv("H2_INSTALLATION_LAYOUT",value.c_str(),1);
  // A writable installed app still owns sealed code resources. Python must
  // not add __pycache__ files when importing those bundled helpers.
  if(installation.packaged)failed|=setenv("PYTHONDONTWRITEBYTECODE","1",1);
#endif
  if(failed)throw std::runtime_error("Could not bind installation resources to user storage");
}
fs::path current_executable() {
#if defined(__linux__)
  std::error_code error;
  const auto path=fs::read_symlink("/proc/self/exe",error);
  if(!error && path.is_absolute())return path;
#elif defined(_WIN32)
  std::vector<wchar_t> name(32768);
  const auto size=GetModuleFileNameW(nullptr,name.data(),DWORD(name.size()));
  if(size && size<name.size())return fs::path(std::wstring(name.data(),size));
#elif defined(__APPLE__)
  return halo2::macos_executable_path();
#endif
  return {};
}
#ifdef _WIN32
std::vector<std::string> windows_arguments(const std::vector<std::string>& arguments) {
  std::vector<std::string> quoted;
  quoted.reserve(arguments.size());
  for(const auto& argument:arguments) {
    // The child CRT parses one Windows command line. Preserve spaces,
    // embedded quotes and trailing backslashes through that round trip.
    std::string value="\"";
    std::size_t slashes=0;
    for(char c:argument) {
      if(c=='\\'){++slashes;continue;}
      value.append(slashes*(c=='\"'?2u:1u), '\\');slashes=0;
      if(c=='\"')value+='\\';
      value+=c;
    }
    value.append(slashes*2u,'\\');value+='\"';
    quoted.push_back(std::move(value));
  }
  return quoted;
}
std::wstring windows_text(const std::string& value) {
  if(value.empty())return {};
  const auto count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,
      value.data(),int(value.size()),nullptr,0);
  if(!count)throw std::runtime_error("Invalid UTF-8 in launcher process arguments");
  std::wstring result(std::size_t(count),L'\0');
  if(MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),int(value.size()),
      result.data(),count)!=count)
    throw std::runtime_error("Could not encode launcher process arguments");
  return result;
}
int windows_environment(const char* name,const std::string& value) {
  return _wputenv_s(windows_text(name).c_str(),windows_text(value).c_str());
}
std::string windows_utf8(const std::wstring& value) {
  if(value.empty())return {};
  const auto count=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,
      value.data(),int(value.size()),nullptr,0,nullptr,nullptr);
  if(!count)throw std::runtime_error("Invalid Unicode in launcher process arguments");
  std::string result(std::size_t(count),'\0');
  if(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),int(value.size()),
      result.data(),count,nullptr,nullptr)!=count)
    throw std::runtime_error("Could not decode launcher process arguments");
  return result;
}
int windows_process(const std::vector<std::string>& arguments,bool search_path) {
  if(arguments.empty())throw std::runtime_error("Missing launcher process arguments");
  std::wstring command;
  for(const auto& argument:windows_arguments(arguments)) {
    if(!command.empty())command+=L' ';
    command+=windows_text(argument);
  }
  const auto executable=windows_text(arguments.front());
  STARTUPINFOW startup{};startup.cb=sizeof(startup);
  const auto input=GetStdHandle(STD_INPUT_HANDLE);
  const auto output=GetStdHandle(STD_OUTPUT_HANDLE);
  const auto error_output=GetStdHandle(STD_ERROR_HANDLE);
  if(input && input!=INVALID_HANDLE_VALUE && output && output!=INVALID_HANDLE_VALUE &&
      error_output && error_output!=INVALID_HANDLE_VALUE) {
    startup.dwFlags|=STARTF_USESTDHANDLES;
    startup.hStdInput=input;startup.hStdOutput=output;startup.hStdError=error_output;
  }
  PROCESS_INFORMATION process{};
  // Setup and home are native GUI windows. Their Python interpreter must not
  // open a console, while inherited handles still support explicit CLI use
  // and the setup child's redirected progress/ownership pipes.
  if(!CreateProcessW(search_path?nullptr:executable.c_str(),command.data(),nullptr,
      nullptr,TRUE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&process))
    throw std::runtime_error("Could not start "+arguments.front()+
        " (Windows error "+std::to_string(GetLastError())+")");
  CloseHandle(process.hThread);
  const auto waited=WaitForSingleObject(process.hProcess,INFINITE);
  DWORD status{};
  const bool complete=waited==WAIT_OBJECT_0 && GetExitCodeProcess(process.hProcess,&status);
  const auto error=complete?ERROR_SUCCESS:GetLastError();
  CloseHandle(process.hProcess);
  if(!complete)throw std::runtime_error("Could not wait for "+arguments.front()+
      " (Windows error "+std::to_string(error)+")");
  return int(status);
}
#endif
int run_tool(std::vector<std::string> arguments);
bool native_shader_package(const Installation &installation,
                           const fs::path &binary, const fs::path &directory) {
  const auto helper=installation.tool("native_renderer_shaders.py");
  if(!fs::is_regular_file(helper))return false;
  return run_tool({installation.python(),"-B",helper.string(),
      "--verify-aot-package","--quiet","--directory",directory.string(),
      "--executable",binary.string()})==0;
}
std::optional<nlohmann::json> local_gameplay(const Installation &installation) {
  const auto path=installation.root/"gameplay-runtime-selection.json";
  if(!fs::exists(path) && !fs::is_symlink(path))return {};
  if(fs::is_symlink(path) || !fs::is_regular_file(path) || fs::file_size(path)>65536)
    throw std::runtime_error("Invalid local gameplay selection");
  std::ifstream input(path);const auto record=nlohmann::json::parse(input);
  const auto runtime_name=fs::path(record.at("runtime").get<std::string>()).filename();
  // A dual-platform source checkout can retain the other platform's selected
  // engine. Leave its selection untouched; it grants this launcher no runtime
  // or gameplay qualification.
  if(runtime_name==foreign_native_name)return {};
  if(runtime_name!=native_name)
    throw std::runtime_error("Invalid local gameplay selection runtime");
  const auto helper=installation.tool("gameplay_runtime_selection.py");
  if(!fs::is_regular_file(helper) || run_tool({installation.python(),helper.string(),
      "--root",installation.root.string()})!=0)
    throw std::runtime_error("The selected MCC multiplayer runtime changed or lost its qualification");
  std::ifstream final_input(path);
  if(nlohmann::json::parse(final_input)!=record)
    throw std::runtime_error("The MCC multiplayer selection changed during admission");
  return record;
}
fs::path selected_prebuilt_runtime(const Installation &installation) {
  const auto &root=installation.root;
  const auto selection=root/"runtime-selection.json";
  const auto helper=installation.tool("update_runtime.py");
  if(!fs::is_regular_file(selection) || fs::is_symlink(selection) ||
      fs::file_size(selection)>65536 || !fs::is_regular_file(helper))return {};
  try {
    std::ifstream stream(selection);
    const auto report=nlohmann::json::parse(stream);
    if(report.value("schema",std::string{})!="H2_PREBUILT_RUNTIME_SELECTION_V1")return {};
    const fs::path relative=report.at("runtime").get<std::string>();
    if(relative.is_absolute() || relative.lexically_normal()!=relative)return {};
    const auto release=root/"bin/releases"/report.at("release_id").get<std::string>();
    const auto runtime=root/relative;
    if(runtime.parent_path()!=release || runtime.filename()!=native_name)return {};
    auto owner=root;
    for(const auto &part:relative) {
      if(part==".." || part==".")return {};
      owner/=part;
      if(fs::is_symlink(owner))return {};
    }
    if(!fs::is_regular_file(runtime))return {};
    // The update helper authenticates every installed manifest file and the
    // host platform before the new release takes precedence over local builds.
    if(run_tool({installation.python(),helper.string(),"--root",root.string(),
                 "--verify-selection"})!=0)return {};
    return fs::canonical(runtime);
  } catch(const std::exception &) { return {}; }
}
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
#ifdef __APPLE__
  if(const auto layout=halo2::macos_application_layout()) {
    Installation installed{layout->data,{},layout->data/"build",{},layout->resources,true};
    bind_installation(installed);
    if(const auto selected=selected_prebuilt_runtime(installed);!selected.empty()) {
      installed.runtime=selected;
    } else {
      const auto binary=layout->resources.parent_path()/"MacOS"/native_name;
      if(fs::is_regular_file(binary) &&
          native_shader_package(installed,binary,layout->resources/"shaders"))
        installed.runtime=fs::canonical(binary);
    }
    return installed;
  }
#endif
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
  std::vector<fs::path> roots{directory, directory.parent_path()};
  // Release-slot launchers live under bin/releases/<id> (or its onboarding
  // directory), so find their owning installation after relocation too.
  auto ancestor=directory.parent_path();
  for(unsigned i=0;i<4 && ancestor!=ancestor.root_path();++i) {
    ancestor=ancestor.parent_path();
    roots.push_back(ancestor);
  }
  roots.emplace_back(HALO2_PROJECT_DIR);
  for (const auto &root : roots) {
    if (!fs::is_regular_file(root / "tools/setup_assets.py"))
      continue;
    const auto project = fs::canonical(root);
    std::vector<fs::path> binaries{directory / native_name,
                                  project / native_name,
                                  project / "bin" / native_name,
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
    const Installation source_installation{project,{},build};
    bind_installation(source_installation);
    const auto selected=selected_prebuilt_runtime(source_installation);
    if(!selected.empty())binaries.insert(binaries.begin(),selected);
    const auto gameplay=local_gameplay(source_installation);
    const auto gameplay_runtime=gameplay?project/gameplay->at("runtime").get<std::string>():fs::path{};
    if(gameplay)binaries.insert(binaries.begin(),gameplay_runtime);
    for (const auto &binary : binaries) {
      if (!fs::is_regular_file(binary) || !current_build(project,binary.parent_path()))
        continue;
      const auto shader_dir = binary.parent_path() / "shaders";
      const bool shaders = binary==selected || (gameplay && binary==gameplay_runtime) ||
          native_shader_package(source_installation,binary,shader_dir);
      if (shaders) {
        const auto launcher=selected.empty()?fs::path{}:
            selected.parent_path()/(title_first?title_launcher_name:launcher_name);
        Installation result{project, fs::canonical(binary),
            fs::is_regular_file(binary.parent_path()/"CMakeCache.txt")?binary.parent_path():build,
            binary==selected && fs::is_regular_file(launcher)?launcher:fs::path{}};
        if(binary==gameplay_runtime) {
          result.qualified_multiplayer=true;
          result.multiplayer_map=gameplay->at("default_map").get<std::string>();
          result.multiplayer_players=gameplay->at("local_players").get<unsigned>();
          result.multiplayer_video_driver=gameplay->value("video_driver",std::string{});
        }
        return result;
      }
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
#ifdef _WIN32
  std::cout.flush();std::cerr.flush();
  return windows_process(arguments,true);
#else
  auto& encoded=arguments;
  std::vector<char*> pointers;
  for(auto &value:encoded)pointers.push_back(value.data());
  pointers.push_back(nullptr);
  std::cout.flush();std::cerr.flush();
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
  std::vector<std::string> arguments{installation.python(),
      installation.tool("setup_assets.py").string(),"--root",installation.root.string()};
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
bool runtime_supports_preview(const Installation &installation,const fs::path &runtime) {
  // Check the selected executable itself. A cache from an earlier build can
  // exist with every shader present while lacking the new preview option.
  return run_tool({installation.python(),"-c",
      "import subprocess,sys\n"
      "try:\n"
      " result=subprocess.run([sys.argv[1],'--mcc-preview','--help'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=10)\n"
      "except (OSError,subprocess.TimeoutExpired):\n"
      " sys.exit(1)\n"
      "sys.exit(result.returncode!=0)\n",runtime.string()})==0;
}
bool runtime_supports_prepared_preview(const Installation &installation,const fs::path &runtime) {
  // Query the executable being launched, not its directory or cache metadata.
  // Older runtimes reject this read-only query without receiving the new option.
  return run_tool({installation.python(),"-c",
      "import json,subprocess,sys\n"
      "try:\n"
      " result=subprocess.run([sys.argv[1],'--mcc-preview-capabilities'],stdout=subprocess.PIPE,stderr=subprocess.DEVNULL,timeout=10)\n"
      " if result.returncode or len(result.stdout)>4096: sys.exit(1)\n"
      " value=json.loads(result.stdout)\n"
      "except (OSError,subprocess.TimeoutExpired,UnicodeError,ValueError):\n"
      " sys.exit(1)\n"
      "sys.exit(not (type(value) is dict and set(value)=={'schema','prepared_cache_root'} and value['schema']=='H2_MCC_PREVIEW_CAPABILITIES_V1' and value['prepared_cache_root'] is True))\n",
      runtime.string()})==0;
}
bool native_gameplay_test_available(const Installation &installation,const AssetSources &sources) {
  const auto helper=installation.tool("native_gameplay_test.py");
  if(installation.runtime.empty() || !sources.ready() || !fs::is_regular_file(helper))return false;
  return run_tool({installation.python(),helper.string(),"--root",installation.root.string(),
      "--runtime",installation.runtime.string(),"--cache",sources.cache.string()})==0;
}
bool has_frontend_selection(const fs::path &root) {
  const auto path=root/"frontend-runtime-selection.json";
  return fs::exists(path) || fs::is_symlink(path);
}
fs::path frontend_runtime(const Installation &installation,const AssetSources &sources) {
  const auto helper=installation.tool("native_gameplay_test.py");
  if(!sources.ready() || !fs::is_regular_file(helper))return {};
  auto runtime=installation.runtime;
  if(has_frontend_selection(installation.root)) {
    // Python authenticates the complete bounded registry, direct path owners,
    // runtime/capability hashes and existing frontend resource admission.
    try {
      const auto selection=installation.root/"frontend-runtime-selection.json";
      if(fs::is_symlink(selection) || !fs::is_regular_file(selection) || fs::file_size(selection)>65536)return {};
      std::ifstream stream(selection);
      const auto report=nlohmann::json::parse(stream);
      if(report.value("schema",std::string{})!="H2_LOCAL_FRONTEND_RUNTIME_SELECTION_V1")return {};
      const fs::path relative=report.at("runtime").get<std::string>();
      if(relative.empty() || relative.is_absolute() || relative.lexically_normal()!=relative ||
          relative.filename()!=native_name)return {};
      runtime=installation.root/relative;
    } catch(const std::exception &) {return {};}
  }
  if(runtime.empty())return {};
  return run_tool({installation.python(),helper.string(),"--root",installation.root.string(),
      "--runtime",runtime.string(),"--cache",sources.cache.string(),"--frontend","--selected-frontend"})==0?runtime:fs::path{};
}
void native_test_arguments(std::vector<std::string> &arguments,const AssetSources &sources) {
  // This validation covers one local MCC Lockout source-object-world test.
  // Other gameplay modes/maps remain available through their existing routes
  // only when full readiness is separately admitted.
  for(std::size_t i=0;i<arguments.size();++i) {
    const auto option=arguments[i];
    if(option=="--multiplayer" || option=="--hidden" || option=="--mute" || option=="--validation")continue;
    const auto count=option=="--viewport"?2u:
        (option=="--map" || option=="--shared-maps" || option=="--frames" || option=="--screenshot")?1u:0u;
    if(!count || arguments.size()-i-1<count)
      throw std::runtime_error("The native gameplay test supports local MCC Lockout with viewport and capture options. Other gameplay modes remain in progress.");
    if(option=="--map" || option=="--shared-maps") {
      const auto expected=option=="--map"?sources.maps()/"lockout.map":sources.maps();
      auto requested=fs::path(arguments[i+1]);
      if(requested.is_relative())requested=sources.maps()/requested;
      if(fs::canonical(requested)!=fs::canonical(expected))
        throw std::runtime_error("The validated native gameplay test requires the selected MCC Lockout map and shared-resource directory.");
      arguments[i+1]=expected.string();
    }
    i+=count;
  }
}
fs::path native_test_save_directory(const fs::path &root) {
  const auto parent=root/"saves",directory=parent/"native-mcc-test";
  if(fs::is_symlink(parent) || fs::is_symlink(directory))
    throw std::runtime_error("The native gameplay test save directory must be owned by this port.");
  fs::create_directories(directory);
  if(fs::canonical(parent).parent_path()!=root || fs::canonical(directory).parent_path()!=fs::canonical(parent))
    throw std::runtime_error("The native gameplay test save directory escaped its port owner.");
  return directory;
}
fs::path launcher_executable(const fs::path &root) {
#ifndef __APPLE__
  if(fs::is_regular_file(root/"PRECOMPILED_TEST_BUILD.json"))
    return root/(title_first?title_launcher_name:launcher_name);
#endif
  if(const auto path=current_executable();!path.empty())return path;
  return root/(title_first?title_launcher_name:launcher_name);
}
std::optional<int> forward_to_release_launcher(const Installation &installation,
                                              std::vector<std::string> arguments) {
  if(installation.release_launcher.empty())return {};
  const auto current=current_executable();
  if(current.empty() || fs::equivalent(current,installation.release_launcher))return {};
  arguments.insert(arguments.begin(),installation.release_launcher.string());
  return run_tool(std::move(arguments));
}
} // namespace

int main(int argc, char **argv) {
#ifdef SDL_MAIN_HANDLED
  SDL_SetMainReady();
#endif
  try {
#ifdef _WIN32
    // The narrow CRT argv uses the active Windows code page. Keep Unicode
    // installation and MCC paths intact when passing them to native Python.
    const std::unique_ptr<wchar_t*,decltype(&LocalFree)> wide_arguments(
        CommandLineToArgvW(GetCommandLineW(),&argc),LocalFree);
    if(!wide_arguments)throw std::runtime_error("Could not read launcher process arguments");
    std::vector<std::string> utf8_arguments;
    std::vector<char*> utf8_pointers;
    utf8_arguments.reserve(argc);utf8_pointers.reserve(argc+1);
    for(int i=0;i<argc;++i)utf8_arguments.push_back(windows_utf8(wide_arguments.get()[i]));
    for(auto& argument:utf8_arguments)utf8_pointers.push_back(argument.data());
    utf8_pointers.push_back(nullptr);argv=utf8_pointers.data();
#endif
    for(int i=1;i<argc;++i) {
      const std::string argument=argv[i];
      if(argument=="--prepared-mcc-cache-root" || argument.starts_with("--prepared-mcc-cache-root="))
        throw std::runtime_error("The prepared MCC cache root is authored by the launcher, not a user option.");
    }
    auto installation = locate();
    if(argc==2 && std::string(argv[1])=="--build-engine") {
      const auto helper=installation.tool("build_launcher_engine.py");
      if(installation.packaged || !fs::is_regular_file(installation.root/"CMakeLists.txt") || !fs::is_regular_file(helper))
        throw std::runtime_error("Building the engine requires a local source checkout");
      return run_tool({installation.python(),"-B",helper.string(),"--root",installation.root.string()});
    }
    if(!has_frontend_selection(installation.root))
      if(const auto forwarded=forward_to_release_launcher(installation,
          std::vector<std::string>(argv+1,argv+argc));forwarded)return *forwarded;
    std::vector<std::string> supplied;
    std::string mcc_source;
    bool setup=false,setup_only=false,update=false,launch_game=false,prepare=false,preview=false,native_requested=false;
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
      else if(argument=="--native-test") {native_requested=true;launch_game=true;}
      else if(argument=="--prepare-mcc-gameplay") {prepare=true;setup=true;setup_only=true;}
      else if(argument=="--build-engine")throw std::runtime_error("Use --build-engine by itself");
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
    // Only an explicit qualified route can skip these unused readiness probes.
    // UI/setup paths still refresh them after their arguments or cache change.
    const bool qualified_multiplayer_cli=installation.qualified_multiplayer && sources.ready() &&
        !setup && !update && !preview && !help && !has("--launcher-info") &&
        !(has("--menu") || has("--menu-screen") || has("--menu-audit") ||
          has("--menu-sweep") || has("--menu-maps") || has("--menu-movie")) &&
        (native_requested || has("--multiplayer") || has("--play") || has("--campaign") ||
         has("--simulate") || has("--host") || has("--join") || has("--dedicated"));
    bool native_test=!qualified_multiplayer_cli && native_gameplay_test_available(installation,sources);
    auto frontend_owner=qualified_multiplayer_cli?fs::path{}:frontend_runtime(installation,sources);
    bool frontend=!frontend_owner.empty();
    if(has("--launcher-info")) {
      const bool stale_frontend=has_frontend_selection(installation.root) && !frontend;
      std::cout << "Project: " << installation.root.string() << '\n'
                << "Resources: " << installation.resources.string() << '\n'
                << "Runtime: " << (installation.runtime.empty()?"not built":installation.runtime.string()) << '\n'
                << "Frontend runtime: " << (frontend_owner.empty()?"unavailable":frontend_owner.string()) << '\n'
                << "Default: startup intro, title screen and menus\n"
                << "Asset source: Halo MCC only\n"
                << "MCC cache prepared: " << (sources.ready()?"yes":"no — scan MCC with --setup") << '\n'
                << "MCC cache: " << sources.cache.string() << '\n'
                << "MCC maps: " << (sources.ready()?sources.maps().string():"") << '\n'
                << "Native MCC gameplay ready: " << (sources.playable?"yes":"no") << '\n'
                << "Native MCC gameplay test: " << (native_test?"available (Lockout, retained object world)":"unavailable") << '\n'
                << "Native MCC frontend: " << (frontend?"available":"unavailable") << '\n'
                << "Qualified MCC multiplayer: " << (installation.qualified_multiplayer?"available":"unavailable") << '\n'
                << "Gameplay default video driver: " << (installation.multiplayer_video_driver.empty()?"SDL default":installation.multiplayer_video_driver) << '\n'
                << "Play: " << (frontend?"MCC intro, title screen and menus":stale_frontend?"unavailable (selected independent MCC frontend is unavailable or changed)":"unavailable (game menu needs validation)") << '\n'
                << "MCC content preview: " << (sources.ready()?"available":"import MCC first") << '\n'
                << "Status: " << (frontend?"Play opens the intro, title screen and main menu.":stale_frontend?"The selected independent MCC menu runtime is unavailable or changed. Repair its selection to enable Play.":!sources.problem.empty()?sources.problem:
                    !sources.ready()?"MCC has not been imported yet. Run --setup.":
                    !sources.playable?(native_test?"Interim native gameplay test; full Xbox TU1.5 gameplay, visual and audio parity remain incomplete.":not_playable):"MCC detection and private-cache import selected") << '\n'
                << "Shaders: existing native Vulkan shaders\n" << std::flush;
      const auto engine_status=installation.tool("build_launcher_engine.py");
      if(!installation.runtime.empty() && fs::is_regular_file(engine_status))
        run_tool({installation.python(),"-B",engine_status.string(),"--root",installation.root.string(),
            "--status","--runtime",installation.runtime.string()});
      return 0;
    }
    if(help) {
      std::cout << "MCC setup: --setup [--mcc-source PATH] [--setup-only]\n"
          "First launch scans installed MCC libraries. Browse to the MCC or halo2 folder if needed.\n"
          "Content is copied into the private PC-port cache; existing shaders are preserved.\n"
          "--prepare-mcc-gameplay [--mcc-source PATH] prepares and selects that cache, then exits.\n"
          "--launcher-info reports cache and native-gameplay readiness.\n"
          "--build-engine builds current local engine sources, checks MCC frame submission, and selects the passing runtime.\n"
          "An exact validated native-test build can expose local Lockout testing independently of full TU1.5 parity.\n"
          "Play opens the intro, title screen and main menu; --native-test opens Lockout.\n"
          "--preview-mcc [--map NAME] previews imported map assets with the existing renderer.\n"
          "--update downloads verified prebuilt binaries for this platform; --launch-game skips the launcher home.\n"
          "Play requires a validated menu runtime. Diagnostic modes remain explicit command-line options.\n";
      return 0;
    }
    if(update) {
      if(setup || launch_game || !supplied.empty())
        throw std::runtime_error("Use --update by itself to download the latest prebuilt game binaries");
      return run_tool({installation.python(),
          installation.tool("update_runtime.py").string(),"--root",
          installation.root.string(),"--build-dir",installation.build.string()});
    }
    if(setup || !sources.ready()) {
      const auto status=prepare_sources(installation,mcc_source,!prepare);
      if(status)return status;
      sources=read_sources(installation.root);
      native_test=native_gameplay_test_available(installation,sources);
      frontend_owner=frontend_runtime(installation,sources);frontend=!frontend_owner.empty();
      if(!sources.ready())throw std::runtime_error("MCC setup did not publish a complete private cache. "+sources.problem);
    }
    if(setup_only)return 0;
    const auto update_pending=installation.root/"launcher-update.pending";
    if(!launch_game && !preview && supplied.empty()) {
      std::vector<std::string> arguments{installation.python(),
          installation.tool("launcher_ui.py").string(),"--root",
          installation.root.string(),"--build-dir",installation.build.string(),
          "--launcher-executable",launcher_executable(installation.root).string()};
      if(!sources.playable)arguments.insert(arguments.end(),{"--runtime-unavailable",not_playable});
      if(!installation.runtime.empty())arguments.insert(arguments.end(),{"--runtime-executable",installation.runtime.string()});
      const auto status=run_tool(std::move(arguments));
      if(status==20)return 0;
      if(status)return status;
      installation=locate();sources=read_sources(installation.root);
      auto next=supplied;
      next.insert(next.begin(),"--launch-game");
      if(!has_frontend_selection(installation.root))
        if(const auto forwarded=forward_to_release_launcher(installation,std::move(next));forwarded)return *forwarded;
      native_test=native_gameplay_test_available(installation,sources);
      frontend_owner=frontend_runtime(installation,sources);frontend=!frontend_owner.empty();
    }
    if(!sources.ready())throw std::runtime_error("The MCC cache needs preparation. Run --setup. "+sources.problem);
    if(native_requested && ((!native_test && !installation.qualified_multiplayer) || preview))
      throw std::runtime_error("The selected build has no validated native MCC Lockout test.");
    const bool gameplay_requested=has("--multiplayer") || has("--play") || has("--campaign") ||
        has("--simulate") || has("--host") || has("--join") || has("--dedicated");
    const bool menu_requested=has("--menu") || has("--menu-screen") || has("--menu-audit") ||
        has("--menu-sweep") || has("--menu-maps") || has("--menu-movie");
    const bool multiplayer_route=installation.qualified_multiplayer && !preview && !menu_requested && (gameplay_requested || native_requested);
    const bool frontend_route=frontend && !preview && !native_requested && !gameplay_requested && !multiplayer_route;
    if(has_frontend_selection(installation.root) && !frontend && !preview && !native_requested && !gameplay_requested && !multiplayer_route)
      throw std::runtime_error("The selected independent MCC frontend is unavailable or changed. The native Lockout test remains available through --native-test.");
    if(!preview && !native_requested && !gameplay_requested && !frontend && !sources.playable)
      throw std::runtime_error("The game menu is unavailable. Update or repair the installed game files.");
    if(multiplayer_route) {
      std::vector<std::string> validation{installation.python(),installation.tool("gameplay_runtime_selection.py").string(),
          "--root",installation.root.string(),"--runtime",installation.runtime.string(),"--"};
      validation.insert(validation.end(),supplied.begin(),supplied.end());
      if(run_tool(std::move(validation))!=0)throw std::runtime_error("Selected MCC multiplayer arguments are outside their qualification");
      for(std::size_t i=0;i+1<supplied.size();++i)if((supplied[i]=="--map" || supplied[i]=="--shared-maps") && fs::path(supplied[i+1]).is_relative())
        supplied[i+1]=(sources.maps()/supplied[i+1]).string();
    }
    if(!sources.playable && !preview && !frontend_route && !multiplayer_route) {
      if(native_test) {
        native_test_arguments(supplied,sources);
        std::cout<<"Launching interim native MCC Lockout gameplay test. Full Xbox TU1.5 gameplay, visual and audio parity remain incomplete.\n";
      } else {
        if((!launch_game && !supplied.empty()) || has("--multiplayer") || has("--play") ||
            has("--campaign") || has("--simulate") || has("--host") || has("--join") || has("--dedicated"))
          throw std::runtime_error(not_playable);
        // Missing or stale native-test validation keeps the existing viewer
        // fallback without changing import/conversion readiness flags.
        preview=true;
        std::cout<<"Launching MCC content test build. Native gameplay and Xbox TU1.5 conversion remain in progress.\n";
      }
    }
    if(preview) {
      preview_arguments(supplied,installation,sources);
      const auto status=run_tool({installation.python(),
          installation.tool("repair_mcc_shaders.py").string(),"--root",installation.root.string()});
      if(status)return status;
    }
    const bool precompiled=installation.packaged || fs::is_regular_file(installation.root/"PRECOMPILED_TEST_BUILD.json");
    // A packaged runtime is pinned and feature-checked when the ZIP is made.
    // It has no source/build dependency and must never enter an auto-build.
    const bool outdated_preview_runtime=preview && !precompiled && !installation.runtime.empty() && !runtime_supports_preview(installation,installation.runtime);
    bool rebuilt=false;
    if(outdated_preview_runtime)std::cout<<"Updating the runtime for MCC content preview…\n"<<std::flush;
    // A legacy source-update marker must not turn a binary update into a
    // source rebuild. Explicit development builds remain a separate tool.
    if((!frontend_route && installation.runtime.empty()) || outdated_preview_runtime) {
      if(precompiled)throw std::runtime_error("The precompiled runtime is missing or incomplete. Extract the complete test ZIP again.");
      const auto tool=installation.tool("build_runtime.py");
      if(!fs::is_regular_file(tool))throw std::runtime_error("Automatic build tool is missing: "+tool.string());
      const auto status=run_tool({installation.python(),tool.string(),"--root",
          installation.root.string(),"--build-dir",installation.build.string()});
      if(status)return status;
      installation=locate();rebuilt=true;
      if(!installation.runtime.empty())fs::remove(update_pending);
    }
    if(!frontend_route && installation.runtime.empty())throw std::runtime_error(
        "The automatic build finished, but the runtime or required shaders are missing. See "+
        (installation.build/"launcher-build.log").string());
    if(preview && rebuilt && !runtime_supports_preview(installation,installation.runtime))
      throw std::runtime_error("The built runtime does not support MCC content preview. Update the source and rebuild this installation.");
    if(native_test && !sources.playable && !preview && !frontend_route && !native_gameplay_test_available(installation,sources))
      throw std::runtime_error("Native gameplay test validation changed. Relaunch to use the MCC content preview or validate the updated build.");
    if(frontend_route && frontend_runtime(installation,sources)!=frontend_owner)
      throw std::runtime_error("Independent MCC frontend validation changed before launch.");
    const auto execution_runtime=frontend_route?frontend_owner:installation.runtime;
    std::vector<std::string> arguments{execution_runtime.string()};
    if(multiplayer_route) {
      if(!has("--multiplayer"))arguments.push_back("--multiplayer");
      if(!has("--local-players"))arguments.insert(arguments.end(),{"--local-players",std::to_string(installation.multiplayer_players)});
      if(!has("--map"))arguments.insert(arguments.end(),{"--map",(sources.maps()/installation.multiplayer_map).string()});
    }
    if(preview) {
      arguments.push_back("--mcc-preview");
      if(runtime_supports_prepared_preview(installation,execution_runtime))
        arguments.insert(arguments.end(),{"--prepared-mcc-cache-root",sources.cache.string()});
      else for(std::size_t i=0;i+1<supplied.size();++i)
        if(supplied[i]=="--map" && fs::path(supplied[i+1]).parent_path()!=sources.maps())
          throw std::runtime_error("This runtime does not support prepared-cache ownership for nested MCC map previews. Use a coherent updated runtime.");
    }
    const bool native_test_route=native_test && !sources.playable && !preview && !frontend_route && !multiplayer_route;
    if(native_test_route) {
      if(!has("--multiplayer"))arguments.push_back("--multiplayer");
      arguments.insert(arguments.end(),{"--mp-source-object-world",native_test_save_directory(installation.root).string()});
    }
    const bool custom =
        preview || native_test_route || multiplayer_route || has("--menu") || has("--menu-screen") || has("--menu-audit") ||
        has("--menu-sweep") || has("--menu-maps") || has("--menu-movie") || has("--play") ||
        has("--campaign") || has("--simulate") || has("--map") ||
        has("--multiplayer") || has("--custom-slayer") || has("--custom-mode") || has("--host") || has("--join") || has("--local-players") || has("--dedicated") ||
        has("--help") || has("-h") || has("--inspect") ||
        has("--inspect-systems") || has("--validate-maps") ||
        has("--validate-systems") || has("--decode-sound");
    if (frontend_route && !has("--menu-screen") && !has("--menu-audit") &&
        !has("--menu-sweep") && !has("--menu-movie")) {
      arguments.insert(arguments.end(),{"--menu-screen","9"});
    } else if (!custom) {
      arguments.push_back("--menu-screen");
      arguments.push_back("9");
    }
    const bool menu_route = frontend_route || !custom || has("--menu") || has("--menu-screen") ||
                            has("--menu-sweep") || has("--menu-audit") ||
                            has("--menu-maps") || has("--menu-movie");
    if(menu_route && !has("--menu-maps"))
      arguments.insert(arguments.end(),{"--menu-maps",sources.maps().string()});
    if(menu_route && has("--menu-maps")) {
      if(std::count(supplied.begin(),supplied.end(),"--menu-maps")!=1)
        throw std::runtime_error("Specify --menu-maps only once for the selected imported MCC cache.");
      const auto option=std::find(supplied.begin(),supplied.end(),"--menu-maps");
      if(std::next(option)==supplied.end() || fs::canonical(*std::next(option))!=fs::canonical(sources.maps()))
        throw std::runtime_error("The frontend requires the selected imported MCC map cache.");
    }
    if(frontend_route) {
      for(const auto *name:{"--menu-ui-assets","--hud-ui-assets","--pause-ui-assets"}) {
        const auto option=std::find(supplied.begin(),supplied.end(),name);
        if(option==supplied.end())continue;
        if(std::count(supplied.begin(),supplied.end(),name)!=1)
          throw std::runtime_error(std::string("Specify ")+name+" only once for the selected imported MCC cache.");
        if(std::next(option)==supplied.end() || std::next(option)->empty() ||
            fs::canonical(*std::next(option))!=fs::canonical(sources.cache/"ui"))
          throw std::runtime_error("The frontend UI assets must belong to the selected imported MCC cache.");
      }
    }
    if(!has("--map") && !multiplayer_route)arguments.insert(arguments.end(),{"--map",(sources.maps()/"lockout.map").string()});
    if(!has("--shared-maps"))arguments.insert(arguments.end(),{"--shared-maps",sources.maps().string()});
    // Future decoded UI packs must be owned by this selected MCC import.
    // Do not read the historical per-platform source registries.
    const auto ui=sources.cache/"ui";
    if(!multiplayer_route && original_weapon_hud && !has("--hud-ui-assets"))arguments.insert(arguments.end(),{"--hud-ui-assets",""});
    for(const auto &[option,manifest]:std::array{
        std::pair{"--menu-ui-assets","pack.tsv"},std::pair{"--hud-ui-assets","hud.tsv"},
        std::pair{"--pause-ui-assets","pause.tsv"}})
      if(!multiplayer_route && !(original_weapon_hud && std::string(option)=="--hud-ui-assets") && !has(option) && !fs::is_symlink(ui) && fs::is_regular_file(ui/manifest) &&
          !fs::is_symlink(ui/manifest) &&
          !fs::canonical(ui/manifest).lexically_relative(sources.cache).string().starts_with(".."))
        arguments.insert(arguments.end(),{option,ui.string()});
    arguments.insert(arguments.end(), supplied.begin(), supplied.end());
    // The native runtime's map/shared-resource paths are relative to the
    // project. Process arguments go directly to the runtime; no shell is
    // involved.
    fs::current_path(installation.root);
#ifdef __linux__
    // The selected qualification can name its admitted host display backend.
    // Preserve an explicit user override; this does not alter gameplay inputs.
    if(multiplayer_route && !installation.multiplayer_video_driver.empty() && !std::getenv("SDL_VIDEODRIVER"))
      if(setenv("SDL_VIDEODRIVER",installation.multiplayer_video_driver.c_str(),0)!=0)
        throw std::runtime_error("Could not select the qualified gameplay video driver");
#endif
    // Discord must relaunch through this launcher so the selected MCC cache
    // and runtime readiness are resolved again. No OAuth token or
    // party join secret is placed in the launch command or process arguments.
    if(const auto launcher=current_executable();!launcher.empty()) {
#ifdef _WIN32
      windows_environment("H2_DISCORD_LAUNCHER",launcher.string());
#else
      setenv("H2_DISCORD_LAUNCHER",launcher.c_str(),1);
#endif
    }
#ifdef _WIN32
    std::cout.flush();std::cerr.flush();
    return windows_process(arguments,false);
#else
    auto& encoded=arguments;
    std::vector<char *> pointers;
    for (auto &argument : encoded)
      pointers.push_back(argument.data());
    pointers.push_back(nullptr);
    std::cout.flush();
    std::cerr.flush();
    execv(execution_runtime.c_str(), pointers.data());
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
