#pragma once
#include "halo2/multiplayer_network.hpp"
#include "halo2/native_vehicle_camera.hpp"
#include "multiplayer_snapshot_origin_data.hpp"
namespace halo2 {
struct LanHost::Continuation::Data {
  struct Peer {
    unsigned endpoint{};
    std::uint64_t nonce{},cookie{};
    std::vector<unsigned> ids;
    std::vector<std::int8_t> selected_teams;
    std::vector<std::optional<SourcePlayerConfiguration>> profiles;
    std::uint32_t sequence{},snapshot_sequence{};
    std::optional<MultiplayerSnapshotStamp> last_snapshot;
    bool seen{};
    std::chrono::nanoseconds idle{};
    std::array<Controls,4> controls{};
    std::array<std::array<std::uint32_t,10>,4> received_presses{},delivered_presses{};
    std::array<std::array<std::uint16_t,2>,4> completed{};
    std::array<std::optional<SourcePlayerActionTarget>,4> completed_vehicle{};
    std::optional<NativeVehicleCamera::Checkpoint> camera;
    std::optional<std::uint32_t> camera_tick;
    std::array<std::optional<NativeDeathCameraView>,4> vehicle_views,death_views;
    std::array<std::uint32_t,4> camera_jump_serial{};
  };
  MultiplayerMatch::AuthorityDigest binding{};
  std::uint64_t fingerprint{},tick{};
  std::vector<std::byte> configuration;
  std::array<std::uint32_t,4> local_players{};
  std::vector<OnlineAdmission> admissions;
  std::vector<SourcePlayerAdmission> roster;
  std::vector<Peer> peers;
};
}
