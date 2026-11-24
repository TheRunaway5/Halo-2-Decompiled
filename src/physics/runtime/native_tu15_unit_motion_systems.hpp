#pragma once
// Native TU1.5 unit motion check, entered at E0500 (physics live support
// unit): whether a unit counts as moving.
//
//   unit_motion::moving   E0500(ESI = unit handle) -> AL
//
// A unit whose byte +0x34B is set does not. Otherwise its support is
// whether it stands in a cluster (+0xB4, NONE for none; 0xA0-byte clusters
// of [52BE5C]) whose surface (+0x70) is flagged (+0x44) and whose bit 18 of
// +4 is clear. A unit in state 6 (+0x3DC) moves; one in state 3 moves as
// its support says; otherwise it moves when 46A0EC is not above its squared
// speed (the velocity +0x1B0, on the x87; or they are unordered), in states
// 4 and 5, or when E65A0
// reports it ready; else as its support says.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_unit_motion.hpp) maps the role to the original instruction,
// runs the speed on the logical x87 and owns the ABI of the child E65A0.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_unit_motion {
using native_object_update::at;
using native_object_update::none;
using native_object_update::object_of;
namespace globals {
inline constexpr std::uint32_t clusters=0x52be5cu;        // DataArray* of 0xA0-byte clusters
inline constexpr std::uint32_t still_speed=0x46a0ecu;     // float
}
inline constexpr std::uint32_t cluster_size=0xa0u,datum_data=0x44u;
namespace unit {
inline constexpr std::uint32_t frozen=0x34bu,cluster=0xb4u,velocity=0x1b0u,state=0x3dcu;
inline constexpr std::uint32_t moving_state=6u,supported_state=3u,first_active_state=4u,second_active_state=5u;
}
namespace cluster {inline constexpr std::uint32_t surface=0x70u,flags=4u,detached_bit=18u;}
inline constexpr std::uint32_t surface_flagged=0x44u;

namespace unit_motion {
enum class Access {object_table,object_datums,object_address,frozen,cluster,clusters,cluster_data,surface,flagged,
    cluster_flags,state,velocity_z,velocity_y,velocity_x,still_speed};
// The result: moving (1), or as the support says.
enum class Exit {moving,by_support};
// Frame:
//   supported()           the support is set (BL)
//   faster_than_still()   the x87 speed test (the loads ran it): whether
//                         the still speed is not above the squared speed
//   ready()               E65A0(EAX the handle); returns AL
template<class Memory,class Frame> Exit moving(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto unit_data=object_of(memory,handle,object_table,object_datums,object_address);
    if(memory.load8(frozen,at(unit_data,unit::frozen)))return Exit::by_support;
    bool support=false;
    const auto cluster_handle=memory.load32(cluster,at(unit_data,unit::cluster));
    if(cluster_handle!=none) {
        const auto clusters_array=memory.load32(clusters,globals::clusters);
        const auto cluster_address=(cluster_handle&0xffffu)*cluster_size+memory.load32(cluster_data,at(clusters_array,datum_data));
        const auto surface_address=memory.load32(surface,at(cluster_address,cluster::surface));
        if(memory.load8(flagged,at(surface_address,surface_flagged))!=0) {
            support=!((memory.load32(cluster_flags,at(cluster_address,cluster::flags))>>cluster::detached_bit)&1u);
            if(support)frame.supported();
        }
    }
    const auto state_value=memory.load8(state,at(unit_data,unit::state));
    if(state_value==unit::moving_state)return Exit::moving;
    if(state_value==unit::supported_state)return Exit::by_support;
    (void)memory.load32(velocity_z,at(unit_data,unit::velocity+8u));
    (void)memory.load32(velocity_y,at(unit_data,unit::velocity+4u));
    (void)memory.load32(velocity_x,at(unit_data,unit::velocity));
    (void)memory.load32(still_speed,globals::still_speed);
    if(frame.faster_than_still())return Exit::moving;
    if(state_value==unit::first_active_state || state_value==unit::second_active_state)return Exit::moving;
    return frame.ready()&0xffu?Exit::moving:Exit::by_support;
}
} // namespace unit_motion
} // namespace halo2::native_unit_motion
