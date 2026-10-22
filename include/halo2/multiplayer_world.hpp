#pragma once
#include "halo2/multiplayer.hpp"
#include "halo2/original_data.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/tags.hpp"
#include <future>
#include <thread>

namespace halo2 {
class GameplayFontBank;
class SourceRuntimeResourcePreparation;
// Immutable map resources only: no Match, source executor, players, clocks or
// RNG. A countdown job can prepare these before the original world boundary.
class MultiplayerMapAssets {
public:
  explicit MultiplayerMapAssets(const MultiplayerOptions&);
  ~MultiplayerMapAssets();
  bool matches(const MultiplayerOptions&) const;
  std::shared_ptr<const Cache> cache_for(const MultiplayerOptions&) const;
  LevelContent take_content(const MultiplayerOptions&,std::optional<ScriptProgram>&);
  Mesh take_bsp_mesh(const MultiplayerOptions&);
  HudContent take_hud(std::uint32_t biped,std::uint32_t weapon);
  float hud_sensor_range(std::uint32_t biped,std::uint32_t weapon) const;
  std::shared_ptr<GameplayFontBank> fonts;
private:
  std::shared_ptr<const Cache> cache_;
  std::shared_ptr<SourceRuntimeResourcePreparation> resources_;
  std::optional<LevelContent> content_;
  std::optional<Mesh> mesh_;
  struct Huds;
  std::unique_ptr<Huds> huds_;
  std::filesystem::path map_,shared_,xbe_;
  std::size_t bsp_{};
  SourceGameVariant variant_;
  std::uint64_t custom_fingerprint_{};
  std::optional<std::int16_t> campaign_difficulty_;
};
// One application-owned worker and at most one queued request. Abandoning a
// countdown future never joins on the UI thread; application teardown joins
// before its resource owners disappear. Obsolete queued requests are dropped.
class MultiplayerAssetLoader {
public:
  MultiplayerAssetLoader();
  ~MultiplayerAssetLoader();
  std::shared_future<std::shared_ptr<MultiplayerMapAssets>> request(const MultiplayerOptions&);
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
// Actual native world initialization, shared by the synchronous session load
// service and the later runtime. No snapshot or replacement match is created
// when rendering begins. Network transports remain runtime-owned.
class MultiplayerPreparedWorld {
  // Pin the actual constructor owner before its public slot can escape.
  // Native borrowers continue to require this exact Cache/control block.
  const std::thread::id cache_executor_=std::this_thread::get_id();
  const std::shared_ptr<const Cache> constructor_cache_;
public:
  explicit MultiplayerPreparedWorld(const MultiplayerOptions&);
  MultiplayerPreparedWorld(const MultiplayerOptions&,std::shared_ptr<MultiplayerMapAssets>);
  // Declared local script input is admitted before the VM/match constructor.
  // It supplies commands only; network and physical results remain owned by
  // the actual native authority. Ordinary construction retains map scripts.
  MultiplayerPreparedWorld(const MultiplayerOptions&,std::optional<ScriptProgram>);
  // The same native initialization with an owned external clock source.
  // Default construction still uses the platform clock.
  MultiplayerPreparedWorld(const MultiplayerOptions&,std::optional<ScriptProgram>,
      std::shared_ptr<PhysicsLiveClockSource>);
  enum class SourceShadowViews { LocalPeer, IncomingHost };
  // Separate, quiescent TU1.5 executor for an accepted direct ONLINE roster.
  // It owns private cache/files/physics and all admitted prototype players.
  // Native identities must still come from the independent peer admission;
  // construction alone does not make this a validated or promoted world.
  static std::unique_ptr<MultiplayerPreparedWorld> source_shadow(
      const MultiplayerOptions&,SourceShadowViews,
      std::shared_ptr<PhysicsLiveClockSource> = {});
  // Complete native view-definition bank without renderer/device ownership.
  // Called after the accepted players have been born in this source world.
  void bind_source_local_views(std::span<const unsigned>);

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
  std::shared_ptr<GameplayFontBank> gameplay_fonts;
  std::shared_ptr<MultiplayerMapAssets> map_assets;
private:
  friend class MultiplayerRuntimeAuthority;
  friend class SourcePeerHostTakeover;
  struct SourceShadowSetup;
  std::shared_ptr<const Cache> original_cache() const;
  // Only same-creator incoming takeover construction may reuse the resident's
  // genuine original Cache. Replay workers retain the public private-Cache path.
  static std::unique_ptr<MultiplayerPreparedWorld> incoming_source_shadow(
      const MultiplayerOptions&,const MultiplayerPreparedWorld&,
      std::shared_ptr<PhysicsLiveClockSource>);
  static std::unique_ptr<MultiplayerPreparedWorld> source_shadow(
      const MultiplayerOptions&,SourceShadowViews,
      std::shared_ptr<PhysicsLiveClockSource>,std::shared_ptr<const Cache>);
  MultiplayerPreparedWorld(const MultiplayerOptions&,std::optional<ScriptProgram>,const SourceShadowSetup*,
      std::shared_ptr<PhysicsLiveClockSource>,std::shared_ptr<MultiplayerMapAssets> = {},
      std::shared_ptr<const Cache> = {});
  std::filesystem::path map_,shared_,xbe_;
  std::size_t bsp_{};
  unsigned players_{},starting_weapon_{};
  SourceGameVariant variant_;
  std::uint64_t custom_fingerprint_{};
  std::vector<std::int8_t> teams_;
  std::vector<SourcePlayerConfiguration> profiles_;
  std::optional<std::uint32_t> seed_;
  bool source_object_world_{};
  std::optional<std::int16_t> campaign_difficulty_;
  std::filesystem::path source_object_savegame_directory_;
  std::vector<Spawn> native_spawn_inputs_;
  bool shadow_{};
};
} // namespace halo2
