#pragma once
#include "halo2/multiplayer_network.hpp"
#include "halo2/native_vehicle_camera.hpp"
#include "network/multiplayer_snapshot_origin_data.hpp"
#include "halo2/nonhost_motion_adoption.hpp"
#include <deque>
#include <algorithm>
#include <bit>
namespace halo2 {
template<class Peer>void validate_source_host_input_continuation(const Peer& p) {
  const auto require=[](bool valid){if(!valid)throw FormatError("Invalid host queued-input continuation");};
  const auto newer=[](std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a-b)>0;};
  const auto view=[&](const auto& value){require(!value || value->fraction!=netcode_view_absent_fraction);};
  const auto motion=[&](const auto& value){if(value)for(auto position:value->position)
    require(position>=-native_motion_position_limit-1 && position<=native_motion_position_limit);};
  require(p.ids.size()<=4 && p.pending.size()<=3 && (p.consumed_known || !p.consumed));
  if(p.consumed_known)require(p.seen && (p.consumed==p.sequence || newer(p.sequence,p.consumed)));
  view(p.view);
  for(unsigned i=0;i<4;++i){motion(p.consumed_motion[i]);
    if(i>=p.ids.size())require(!p.consumed_motion[i] && !p.report_steps[i] &&
      std::ranges::all_of(p.latest_presses[i],[](auto serial){return serial==0;}));
    for(unsigned j=0;j<10;++j)require(!newer(p.received_presses[i][j],p.latest_presses[i][j]) &&
      !newer(p.delivered_presses[i][j],p.received_presses[i][j]));
  }
  std::optional<std::uint32_t> previous;
  for(const auto& q:p.pending){
    require(p.seen && (!previous || newer(q.sequence,*previous)) &&
      (!p.consumed_known || newer(q.sequence,p.consumed)) &&
      (q.sequence==p.sequence || newer(p.sequence,q.sequence)));
    previous=q.sequence;view(q.view);
    for(unsigned i=0;i<4;++i){motion(q.motion[i]);
      if(i>=p.ids.size())require(!q.motion[i] && !q.completed_vehicle[i] &&
        q.completed[i]==std::array<std::uint16_t,2>{} &&
        std::ranges::all_of(q.presses[i],[](auto serial){return serial==0;}));
      for(unsigned j=0;j<10;++j)require(!newer(q.presses[i][j],p.latest_presses[i][j]));
    }
  }
}
struct LanHost::Continuation::Data {
  struct QueuedInput {
    std::uint32_t sequence{};
    std::array<Controls,4> controls{};
    std::array<std::optional<NativeMotionReport>,4> motion{};
    std::optional<NetcodeViewTime> view;
    std::array<std::array<std::uint32_t,10>,4> presses{};
    std::array<std::array<std::uint16_t,2>,4> completed{};
    std::array<std::optional<SourcePlayerActionTarget>,4> completed_vehicle{};
  };
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
    std::array<std::optional<NativeDeathCameraView>,4> player_views;
    std::array<std::optional<NativeDeathCameraView>,4> vehicle_views,death_views;
    std::array<std::uint32_t,4> camera_jump_serial{};
    std::optional<NetcodeViewTime> view;
    std::array<std::array<std::uint32_t,10>,4> latest_presses{};
    std::deque<QueuedInput> pending;
    bool consumed_known{};
    std::uint32_t consumed{};
    std::array<std::optional<NativeMotionReport>,4> consumed_motion{};
    std::array<unsigned,4> report_steps{};
    std::uint64_t consumed_inputs{},skipped_inputs{},repeated_ticks{};
  };
  MultiplayerMatch::AuthorityDigest binding{};
  std::uint64_t fingerprint{},tick{};
  std::vector<std::byte> configuration;
  std::array<std::uint32_t,4> local_players{};
  std::vector<OnlineAdmission> admissions;
  std::vector<SourcePlayerAdmission> roster;
  std::vector<Peer> peers;
  std::optional<NonHostMotionAdopter::Checkpoint> motion_adoption;
};
}
