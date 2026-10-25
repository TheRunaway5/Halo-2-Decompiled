#include "halo2/frontend_custom_maps.hpp"
#include "halo2/frontend.hpp"
#include "halo2/frontend_multiplayer_maps.hpp"
#include "halo2/tags.hpp"
#include <bit>
#include <fstream>
#include <nlohmann/json.hpp>
#include <set>

namespace halo2 {
void bind_frontend_custom_maps(FrontendContent &content,
                              const std::filesystem::path &maps) {
  namespace fs = std::filesystem;
  const auto root = fs::canonical(maps).parent_path().parent_path();
  const auto manifest = root / "frontend-map-bindings.json";
  if (!fs::exists(manifest)) return;
  if (fs::is_symlink(manifest) || !fs::is_regular_file(manifest) ||
      fs::file_size(manifest) > 65536)
    throw FormatError("Invalid frontend custom-map binding owner");
  std::ifstream stream(manifest);
  const auto data = nlohmann::json::parse(stream);
  if (data.value("schema", "") != "H2_FRONTEND_MAP_BINDINGS_V1" ||
      !data.at("maps").is_array() || data.at("maps").size() > 64)
    throw FormatError("Invalid frontend custom-map binding schema");
  std::set<std::int32_t> ids;
  std::set<fs::path> paths;
  struct Binding { std::size_t index; std::string path, shared; };
  std::vector<Binding> bindings;
  for (const auto &row : data.at("maps")) {
    const auto id = row.at("map_id").get<std::int32_t>();
    const auto scenario = row.at("scenario_path").get<std::string>();
    const auto text = row.at("cache_file").get<std::string>();
    const auto relative_path = [](const std::string &text) {
      const fs::path relative(text);
      if (relative.empty() || relative.is_absolute() || relative.generic_string() != text ||
          text.find_first_of("\\:\r\n\t") != text.npos || text.find('\0') != text.npos)
        throw FormatError("Custom map requires an exact relative cache path");
      for (const auto &part : relative)
        if (part.empty() || part == "." || part == "..")
          throw FormatError("Custom map requires an exact relative cache path");
      return relative;
    };
    const auto relative = relative_path(text);
    const auto root_relative = relative_path(row.at("cache_root").get<std::string>());
    const auto shared_relative = relative_path(row.at("shared_maps").get<std::string>());
    if (id == -1 || !ids.insert(id).second || !paths.insert(relative).second)
      throw FormatError("Duplicate or absent frontend custom-map identity");
    std::size_t index = content.multiplayer_maps.size();
    for (std::size_t i = 0; i < content.multiplayer_maps.size(); ++i)
      if (content.multiplayer_maps[i].map_id == id) {
        if (index != content.multiplayer_maps.size())
          throw FormatError("Ambiguous frontend custom-map catalog identity");
        index = i;
      }
    if (index == content.multiplayer_maps.size() ||
        content.multiplayer_maps[index].scenario_path != scenario ||
        !content.multiplayer_maps[index].cache_file.empty())
      throw FormatError("Custom map differs from its authored menu catalog");
    const auto &map = content.multiplayer_maps[index];
    if (relative.filename() != frontend_multiplayer_map_file(map))
      throw FormatError("Custom cache filename differs from its authored scenario");
    const auto contained = [&](const fs::path &relative) {
      auto path = root;
      for (const auto &part : relative) {
        path /= part;
        if (fs::is_symlink(path))
          throw FormatError("Custom map cache binding contains a symlink");
      }
      if (fs::canonical(path) != path)
        throw FormatError("Custom map cache binding lost its exact owner");
      return path;
    };
    const auto path = contained(relative);
    const auto prepared = contained(root_relative);
    const auto shared = contained(shared_relative);
    const auto local = path.lexically_relative(prepared / "halo2/Mods");
    if (local.empty() || *local.begin() == ".." ||
        shared != prepared / "halo2/h2_maps_win64_dx11")
      throw FormatError("Custom map lost its separate prepared map/resource owner");
    if (!fs::is_regular_file(path) || fs::canonical(path) != path)
      throw FormatError("Custom map cache binding has no exact regular owner");
    const Cache cache(path, shared, {}, prepared);
    if (!cache.is_custom_mcc())
      throw FormatError("Frontend custom map requires its admitted custom cache owner");
    const auto *tag = cache.query_tag({.group = tag_groups::scenario, .name = scenario});
    if (!tag || tag->size < 0x3a0)
      throw FormatError("Custom map cache has a different scenario owner");
    const Bytes level(cache.read_tag_bytes(tag->address + 0x398, 8));
    if (level.u32(0) != 1)
      throw FormatError("Custom map requires one authored level-data block");
    const Bytes header(cache.read_tag_bytes(level.u32(4), 24));
    if (header.u32(16) != 1)
      throw FormatError("Custom map requires one authored multiplayer definition");
    const Bytes definition(cache.read_tag_bytes(header.u32(20), 0xc64));
    if (std::bit_cast<std::int32_t>(definition.u32(0)) != id ||
        definition.string(0xb4c, 256) != scenario)
      throw FormatError("Custom map's actual level definition differs from its menu");
    bindings.push_back({index, path.generic_string(), shared.generic_string()});
  }
  for (auto &binding : bindings) {
    auto &map = content.multiplayer_maps[binding.index];
    map.cache_file = std::move(binding.path);
    map.shared_maps = std::move(binding.shared);
  }
}
}
