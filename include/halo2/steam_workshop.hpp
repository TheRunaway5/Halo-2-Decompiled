#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace halo2 {
struct WorkshopMission {
  std::filesystem::path item;
  std::string scenario, title, description;
  bool campaign{};
};
struct WorkshopMod {
  std::string id, title, description, error;
  std::filesystem::path directory;
  std::vector<WorkshopMission> missions;
};
struct WorkshopCatalog {
  std::vector<WorkshopMod> mods;
  std::vector<std::string> roots, errors;
};
struct WorkshopPreparedMission {
  std::filesystem::path map, shared_maps, prepared_root;
  std::vector<std::string> script_requirements;
  std::vector<std::string> rendering_requirements;
  std::string gameplay_unavailable_reason() const {
    if(!script_requirements.empty())
      return "This mission needs native support for " + std::to_string(script_requirements.size()) +
          " MCC script features. First: " + script_requirements.front() + ". Choose Explore Map to view the level.";
    if(!rendering_requirements.empty()) {
      const auto &name=rendering_requirements.front();const auto at=name.find_last_of('\\');
      return "Campaign rendering needs support for " + std::to_string(rendering_requirements.size()) +
          " model materials. First: " + name.substr(at==std::string::npos?0:at+1) +
          ". Choose Explore Map to view the level.";
    }
    return {};
  }
};
// Metadata/preparation helpers use argv without a shell. The Python tool shares
// Steam library discovery with onboarding and never writes into subscriptions.
WorkshopCatalog scan_steam_workshop();
WorkshopPreparedMission prepare_steam_workshop(const WorkshopMission &,
    const std::filesystem::path &base_cache, const std::filesystem::path &storage);
inline constexpr unsigned workshop_menu_context = 0x57530000u;
inline constexpr unsigned workshop_mission_context = 0x57540000u;
} // namespace halo2
