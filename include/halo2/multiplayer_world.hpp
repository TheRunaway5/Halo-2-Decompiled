#pragma once
#include "halo2/multiplayer.hpp"
#include "halo2/original_data.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/tags.hpp"

namespace halo2 {
// Actual native world initialization, shared by the synchronous session load
// service and the later runtime. No snapshot or replacement match is created
// when rendering begins. Network transports remain runtime-owned.
class MultiplayerPreparedWorld {
public:
  explicit MultiplayerPreparedWorld(const MultiplayerOptions&);
  bool matches(const MultiplayerOptions&) const;
  std::shared_ptr<const Cache> cache;
  TagSystem tags;
  LevelContent content;
  SlayerRules rules;
  // Session-private source scratch files; destroyed after the world closes.
  std::shared_ptr<const std::filesystem::path> source_savegames;
  // Must outlive Match's source pool view; Cache must outlive its native I/O.
  std::unique_ptr<MultiplayerObjectRuntime> source_objects;
  MultiplayerMatch match;
  std::vector<unsigned> local_slots;
private:
  std::filesystem::path map_,shared_,xbe_;
  std::size_t bsp_{};
  unsigned players_{},starting_weapon_{};
  SourceGameVariant variant_;
  std::vector<std::int8_t> teams_;
  std::vector<SourcePlayerConfiguration> profiles_;
  std::optional<std::uint32_t> seed_;
  bool source_object_world_{};
  std::filesystem::path source_object_savegame_directory_;
  std::vector<Spawn> native_spawn_inputs_;
};
} // namespace halo2
