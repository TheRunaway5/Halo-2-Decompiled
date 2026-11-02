#include "halo2/steam_workshop.hpp"
#include "halo2/cache.hpp"
#include "halo2/mcc_script_layout.hpp"
#include "halo2/native_mcc_script_builtin.hpp"
#include "halo2/content.hpp"
#include "halo2/model_selection.hpp"
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>
#include <random>
#include <set>
#ifdef _WIN32
#include <process.h>
#include <windows.h>
#else
#include <cerrno>
#include <spawn.h>
#include <sys/wait.h>
extern char **environ;
#endif

namespace halo2 {
namespace {
namespace fs = std::filesystem;
using Json = nlohmann::json;
std::string utf8(const fs::path &path) {
  const auto value = path.u8string();
  return {value.begin(), value.end()};
}
struct ReportDirectory {
  fs::path path;
  ReportDirectory() {
    const auto *scratch = std::getenv("HALO2_TEST_SCRATCH");
    const auto parent = scratch ? fs::path(scratch) : fs::temp_directory_path();
    std::random_device random;
    for (unsigned attempt = 0; attempt < 32; ++attempt) {
      path = parent / ("halo2-workshop-" + std::to_string(random()) + "-" + std::to_string(random()));
      if (fs::create_directory(path)) return;
    }
    throw FormatError("Could not create Workshop metadata workspace");
  }
  ~ReportDirectory() { std::error_code error; fs::remove_all(path, error); }
};
fs::path resources() {
  if (const auto *raw = std::getenv("H2_INSTALLATION_LAYOUT")) {
    const auto layout = Json::parse(raw);
    if (layout.value("schema", "") != "H2_INSTALLATION_LAYOUT_V1")
      throw FormatError("Invalid installation resource binding for Workshop");
    return fs::u8path(layout.at("resources").get<std::string>());
  }
  // Source builds/frozen source cohorts keep their tools with the source root.
  return fs::path(HALO2_WORKSHOP_TOOL_ROOT);
}
int process(std::vector<std::string> arguments) {
#ifdef _WIN32
  std::vector<std::wstring> encoded;
  for (const auto &value : arguments) {
    const auto size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        value.data(), int(value.size()), nullptr, 0);
    if (!size) throw FormatError("Invalid UTF-8 in Workshop process argument");
    std::wstring text(size, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), int(value.size()), text.data(), size);
    // _wspawnvp follows the CRT command-line parser; quote spaces and escape
    // quotes/backslashes so a path remains exactly one argument.
    std::wstring quoted = L"\"";
    unsigned slashes{};
    for (auto c : text) {
      if (c == L'\\') { ++slashes; continue; }
      quoted.append(c == L'"' ? slashes * 2 + 1 : slashes, L'\\');
      slashes = 0; quoted += c;
    }
    quoted.append(slashes * 2, L'\\'); quoted += L'"';
    encoded.push_back(std::move(quoted));
  }
  std::vector<const wchar_t *> pointers;
  for (const auto &value : encoded) pointers.push_back(value.c_str());
  pointers.push_back(nullptr);
  const auto executable = fs::u8path(arguments.front()).wstring();
  const auto result = _wspawnvp(_P_WAIT, executable.c_str(), pointers.data());
  return result < 0 ? 127 : int(result);
#else
  std::vector<char *> pointers;
  for (auto &argument : arguments) pointers.push_back(argument.data());
  pointers.push_back(nullptr);
  pid_t child{};
  if (posix_spawnp(&child, pointers.front(), nullptr, nullptr, pointers.data(), environ)) return 127;
  int status{};
  while (waitpid(child, &status, 0) < 0)
    if (errno != EINTR) throw FormatError("Could not wait for Workshop preparation");
  return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
#endif
}
Json request(std::vector<std::string> options) {
  const auto root = resources();
  const auto tool = root / "tools/steam_workshop.py";
  if (!fs::is_regular_file(tool)) throw FormatError("Workshop discovery tool is missing from this installation");
#ifdef _WIN32
  auto python = root / "python/python.exe";
  std::string interpreter = fs::is_regular_file(python) ? utf8(python) : "python";
#else
  auto python = root / "python/bin/python3";
  std::string interpreter = fs::is_regular_file(python) ? utf8(python) : "python3";
#endif
  if (const auto *configured = std::getenv("HALO2_PYTHON")) interpreter = configured;
  ReportDirectory owner;
  const auto report = owner.path / "result.json";
  std::vector<std::string> arguments{interpreter, "-B", utf8(tool)};
  arguments.insert(arguments.end(), options.begin(), options.end());
  arguments.insert(arguments.end(), {"--report", utf8(report)});
  const auto status = process(std::move(arguments));
  if (!fs::is_regular_file(report) || fs::file_size(report) > 16 * 1024 * 1024)
    throw FormatError("Workshop helper did not return metadata; check the installation's Python runtime");
  std::ifstream in(report);
  const auto result = Json::parse(in);
  if (status || result.contains("error"))
    throw FormatError(result.value("error", "Workshop preparation failed"));
  if (result.value("schema", "") != "H2_STEAM_WORKSHOP_V1")
    throw FormatError("Workshop helper returned an unsupported catalog");
  return result;
}
}
WorkshopCatalog scan_steam_workshop() {
  const auto value = request({"--scan"});
  WorkshopCatalog catalog;
  catalog.roots = value.at("roots").get<std::vector<std::string>>();
  catalog.errors = value.at("errors").get<std::vector<std::string>>();
  for (const auto &row : value.at("mods")) {
    WorkshopMod mod;
    mod.id = row.at("id"); mod.title = row.at("title");
    mod.description = row.at("description"); mod.error = row.at("error");
    mod.directory = fs::u8path(row.at("directory").get<std::string>());
    for (const auto &mission : row.at("missions"))
      mod.missions.push_back({fs::u8path(mission.at("item").get<std::string>()), mission.at("scenario"),
          mission.at("title"), mission.at("description"), mission.at("campaign").get<bool>()});
    catalog.mods.push_back(std::move(mod));
  }
  return catalog;
}
WorkshopPreparedMission prepare_steam_workshop(const WorkshopMission &mission,
    const fs::path &base_cache, const fs::path &storage) {
  const auto result = request({"--item", utf8(mission.item), "--scenario", mission.scenario,
      "--base-cache", utf8(base_cache), "--storage", utf8(fs::absolute(storage))});
  if (result.at("campaign").get<bool>() != mission.campaign)
    throw FormatError("Workshop mission type changed before launch");
  WorkshopPreparedMission prepared{fs::u8path(result.at("map").get<std::string>()), fs::u8path(result.at("shared_maps").get<std::string>()),
                                  fs::u8path(result.at("prepared_root").get<std::string>()), {}};
  const Cache cache(prepared.map, prepared.shared_maps, {}, prepared.prepared_root);
  const auto *scenario = cache.find_tag(cache.scenario_tag());
  if (!scenario) throw FormatError("Workshop mission has no scenario");
  const Bytes definition(cache.read_tag_bytes(scenario->address, scenario->size));
  const auto block = [&](unsigned field, unsigned stride) {
    const auto count = definition.u32(field);
    if (count > (stride == 1 ? 16u * 1024u * 1024u : 65536u))
      throw FormatError("Workshop script bank exceeds its native limit");
    return count ? cache.read_tag_bytes(definition.u32(field + 4), std::size_t(count) * stride)
                 : std::span<const std::byte>{};
  };
  const auto strings = block(0x1b0, 1);
  const auto syntax = block(0x238,20);
  const auto layout = project_mcc_script_layout(cache, syntax, strings, block(0x1b8, 0x28), {true});
  const NativeMccScriptBuiltinProfile owned(layout, strings, true);
  std::set<std::string> missing;
  for (const auto &need : layout.requirements)
    if (!owned.owns(need)) missing.insert(need.name);
  prepared.script_requirements.assign(missing.begin(), missing.end());
  // The campaign renderer prepares every material in each placed model's
  // table. A valid script signature alone cannot admit a model whose cooked
  // material has no native passes. Preview keeps its existing gap reporting.
  if(mission.campaign && missing.empty()) {
    std::set<std::uint32_t> definitions,models,shaders;
    std::set<std::string> unresolved;
    for(const auto &object:cache.load_content(0,false).objects) {
      if(object.definition==0xffffffffu || !definitions.insert(object.definition).second)continue;
      const auto model=cache.load_initial_model_definition(object.definition).render_model;
      if(model==0xffffffffu || !models.insert(model).second)continue;
      const auto *tag=cache.find_tag({model});
      if(!tag || tag->group!="mode")throw FormatError("Workshop placed object has no render-model owner");
      const Bytes root(cache.read_tag_bytes(tag->address,0x68));
      const auto count=root.u32(0x60);
      if(count>65536u)throw FormatError("Workshop model material bank exceeds its native limit");
      const Bytes materials(count?cache.read_tag_bytes(root.u32(0x64),std::size_t(count)*32):std::span<const std::byte>{});
      for(unsigned row=0;row<count;++row) {
        const auto shader=materials.u32(row*32+12);
        if(shader==0xffffffffu || !shaders.insert(shader).second)continue;
        const auto *instance=cache.find_tag({shader});
        if(!instance || instance->group!="shad")throw FormatError("Workshop model material has no shader owner");
        const auto material=cache.load_material(shader,false,false);
        if(!material.lods.empty() && material.passes.empty())unresolved.insert(instance->name);
      }
    }
    prepared.rendering_requirements.assign(unresolved.begin(),unresolved.end());
  }
  return prepared;
}
} // namespace halo2
