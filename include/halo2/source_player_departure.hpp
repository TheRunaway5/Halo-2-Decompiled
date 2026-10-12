#pragma once
#include "halo2/source_player_profile.hpp"
#include <cstddef>
#include <functional>

namespace halo2 {
// Original allocated player record. Departure retains salt, identity, profile
// and every unidentified byte. Pool deletion (14C540) is a different owner.
struct SourceDeparturePlayer {
  std::uint16_t salt{},flags{};
  SourcePlayerIdentity identity{};
  std::uint32_t leave_tick{};
  SourceMachineIdentity machine{};
  std::int16_t machine_index=-1,machine_user=-1;
  std::array<std::uint8_t,2> reserved_1e{};
  std::int32_t machine_controller=-1,controller_index=-1;
  std::int16_t local_index=-1;
  std::array<std::uint8_t,2> reserved_2a{};
  std::uint32_t current_unit=0xffffffffu,previous_unit=0xffffffffu;
  std::array<std::uint8_t,0x10> reserved_34{};
  SourcePlayerConfiguration configuration{};
  std::array<std::uint8_t,0x148> remaining{};
};
static_assert(sizeof(SourceDeparturePlayer)==0x21c);
static_assert(offsetof(SourceDeparturePlayer,identity)==4);
static_assert(offsetof(SourceDeparturePlayer,leave_tick)==0x10);
static_assert(offsetof(SourceDeparturePlayer,machine)==0x14);
static_assert(offsetof(SourceDeparturePlayer,controller_index)==0x24);
static_assert(offsetof(SourceDeparturePlayer,local_index)==0x28);
static_assert(offsetof(SourceDeparturePlayer,current_unit)==0x2c);
static_assert(offsetof(SourceDeparturePlayer,configuration)==0x44);
struct SourceDepartureCensus {
  std::int32_t count{};
  std::array<std::uint8_t,4> reserved_04{};
  std::int16_t user_count{},controller_count{};
  std::array<std::uint32_t,4> users{},controllers{};
};
static_assert(sizeof(SourceDepartureCensus)==0x2c);
static_assert(offsetof(SourceDepartureCensus,users)==0xc);
struct SourceDepartureContext {
  std::uint32_t tick{};
  std::uint8_t raw_mode{};
  bool world_present{},world_flag{},engine_present{},engine_team_mode{};
};
struct SourceDepartureServices {
  // Complete 152DF0 unit+10A|400/B7360 and 14CAD0 owners, respectively.
  // B7360 marks header mask2 up the parent chain; it does not itself delete
  // an object. Callers must not substitute a death or weapon drop here.
  std::function<void(std::uint32_t)> mark_current_unit;
  std::function<void(std::uint32_t)> mode4_clear_unit_backlinks;
  std::function<void(std::uint32_t)> unlink_current_unit,simulation_leave;
  std::function<void(std::int32_t,std::uint32_t)> control_set_unit;
  std::function<void(std::uint32_t,std::int32_t,std::int32_t)> first_person_set_unit;
  std::function<void()> refresh_team_masks,remap_results;
  std::function<void(std::uint32_t)> engine_leave;
  // Original event category0/subtype11, with actual cause player/team.
  std::function<void(std::uint32_t,std::int8_t)> publish_leave_event;
  std::function<void(std::int16_t,std::uint32_t)> dirty_player;
};
// Whole 84F30 gate followed by 152DF0/157DE0. Input is a low slot and a
// 12-byte identity, not an incoming salted datum. The span ends at the real
// pool high-water mark; zero salt means unallocated. Services are mandatory
// when reached, and retain their original order. No native identity is made.
bool source_simulation_player_depart(std::span<SourceDeparturePlayer>,
    std::int32_t slot,const SourcePlayerIdentity&,SourceDepartureCensus&,
    const SourceDepartureContext&,const SourceDepartureServices&);
}
