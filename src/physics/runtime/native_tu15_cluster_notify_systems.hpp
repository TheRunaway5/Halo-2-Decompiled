#pragma once
// Native TU1.5 cluster notice, entered at 1CD620 (physics live support
// unit): an object's cluster told of a change, unless its flags hold it back.
//
//   cluster_notify::notify   1CD620(EAX = object handle; first, second) -> RET 8
//
// An object in a cluster (+0xB4, NONE for none) has the cluster's flags
// (+4 of the 0xA0-byte cluster) looked at:
//   - bit 1 with the first argument set holds the notice back;
//   - bit 8 holds it back (the second argument is read first);
//   - bit 7 holds it back unless the second argument is set;
// otherwise 1DC270 gets the cluster handle.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_cluster_notify.hpp) maps the role to the original instruction
// and owns the ABI of 1DC270.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_cluster_notify {
using native_object_update::ClusterRecord;
using native_object_update::ObjectData;
using native_object_update::at;
using native_object_update::none;
namespace globals {
inline constexpr std::uint32_t object_headers=0x4ecbd0u;   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t clusters=0x52be5cu;         // DataArray* of ClusterRecord
}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u;
namespace cluster_flags {inline constexpr std::uint32_t first_gate=1u<<1,second_gate=1u<<7,sealed=1u<<8;}

namespace cluster_notify {
enum class Access {headers,header_datums,object_address,cluster,clusters,cluster_datums,flags,first_argument,second_argument};
enum class Outcome {no_cluster,first_held,sealed,second_held,notified,second_notified};
// Frame: argument(k) the k-th argument's slot; examined(flags, sealed) the
// bit tests past the first gate; notify(cluster) 1DC270.
template<class Memory,class Frame> Outcome notify(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    const auto object=memory.load32(object_address,datums+header_size*(handle&0xffffu)+header_address);
    const auto cluster_handle=memory.load32(Access::cluster,at(object,offsetof(ObjectData,cluster)));
    if(cluster_handle==none)return Outcome::no_cluster;
    const auto array=memory.load32(Access::clusters,globals::clusters);
    const auto data=memory.load32(cluster_datums,at(array,datum_data));
    const auto flags_value=memory.load32(Access::flags,
        std::uint32_t(sizeof(ClusterRecord))*(cluster_handle&0xffffu)+data+offsetof(ClusterRecord,flags));
    if((flags_value&cluster_flags::first_gate) && (memory.load8(first_argument,frame.argument(0))&0xffu))return Outcome::first_held;
    const bool sealed=(flags_value&cluster_flags::sealed)!=0;
    frame.examined(flags_value,sealed);
    if(sealed || (flags_value&cluster_flags::second_gate)) {
        const bool second=(memory.load8(second_argument,frame.argument(1))&0xffu)!=0;
        if(sealed)return Outcome::sealed;
        if(!second)return Outcome::second_held;
        frame.notify(cluster_handle);
        return Outcome::second_notified;
    }
    frame.notify(cluster_handle);
    return Outcome::notified;
}
} // namespace cluster_notify
} // namespace halo2::native_cluster_notify
