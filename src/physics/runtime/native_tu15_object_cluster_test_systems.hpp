#pragma once
// Native TU1.5 object cluster test, entered at B8D90 (physics live support
// unit): whether an object stands in a set of clusters.
//
//   object_cluster_test::in   B8D90(EAX = object handle; clusters*) -> AL
//
// An object flagged by bit 1 of its +4 word is always in. A machine (header
// type 7) asks its type chain (10DDE0, the extra argument pointing at the
// result byte the original keeps in its argument's slot, cleared first);
// when a type answers, that byte is the result. Otherwise the object's
// cluster (the header's signed word +4) is looked up in the cluster bit
// set; an object without a cluster (NONE) is in when the result byte says
// so (it is still clear).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_object_cluster_test.hpp) maps the role to the original
// instruction and owns the ABI of 10DDE0; the header and the cluster set
// the original keeps in callee-saved registers across it are a Kept,
// refreshed from the registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_object_cluster_test {
using native_object_update::at;
using native_object_update::widen16;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u;}   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_type=3u,header_cluster=4u,header_address=8u;
inline constexpr std::uint32_t object_flags=4u,always_bit=1u,machine_type=7u;
inline constexpr std::uint16_t no_cluster=0xffffu;

namespace object_cluster_test {
enum class Access {headers,header_datums,clusters_argument,object_address,result_clear,object_flags,type,cluster_check,
    cluster,cluster_word,result_read};
enum class Outcome {always,answered,no_cluster,looked_up};
struct Kept {std::uint32_t header;std::uint32_t clusters;};   // ESI, EBP
// Frame:
//   argument()          the cluster set's slot (also the result byte)
//   ask(kept)           10DDE0(EDI the handle, EBX the result byte, the
//                       cluster set pushed); refreshes kept; returns AL
template<class Memory,class Frame> Outcome in(Memory &memory,Frame &frame,std::uint32_t handle,std::uint32_t &result) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    Kept kept{0u,memory.load32(clusters_argument,frame.argument())};
    kept.header=datums+header_size*(handle&0xffffu);
    const auto address=memory.load32(object_address,at(kept.header,header_address));
    memory.store8(result_clear,frame.argument(),0u);
    if((memory.load32(Access::object_flags,at(address,native_object_cluster_test::object_flags))>>always_bit)&1u) {
        result=1u;return Outcome::always;
    }
    const auto type_value=memory.load8(type,at(kept.header,header_type));
    if(((1u<<(type_value&31u))&0xffu)==(1u<<machine_type) && frame.ask(kept)) {
        result=memory.load8(result_read,frame.argument());return Outcome::answered;
    }
    if(memory.load16(cluster_check,at(kept.header,header_cluster))==no_cluster) {
        result=memory.load8(result_read,frame.argument());return Outcome::no_cluster;
    }
    const auto cluster_index=widen16(memory.load16(cluster,at(kept.header,header_cluster)));
    const auto word=memory.load32(cluster_word,kept.clusters+4u*std::uint32_t(std::int32_t(cluster_index)>>5));
    result=(word&(1u<<(cluster_index&31u)))!=0;
    return Outcome::looked_up;
}
} // namespace object_cluster_test
} // namespace halo2::native_object_cluster_test
