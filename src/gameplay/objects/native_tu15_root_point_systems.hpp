#pragma once
// Native TU1.5 root point, entered at BBF20 (physics live support unit): a
// point for an object, from its root's cluster or from BBE00.
//
//   root_point::find   BBF20(EAX = object handle, ECX = point*) -> EAX point*
//
// The object's root is found along its parents (+0x14) to the one without
// a parent (with the handle NONE, the root is NONE). The root's cluster
// (+0xB4, NONE for none) names an entry by its signed byte +0x18, valid
// below the cluster's count (+0x74) and not negative; the entry (0x60
// bytes at the cluster's +0x70) leads through its +0x40 and that record's
// +0x3C to the three words +0x70, +0x74, +0x78 copied into the point.
// Without a cluster or an entry, BBE00 fills the point.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_root_point.hpp) maps the role to the original instruction
// and owns the ABI of BBE00; the point the original keeps in ESI across it
// is a Kept, refreshed from the register it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_root_point {
using native_object_update::ClusterRecord;
using native_object_update::ObjectData;
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
using native_object_update::widen8;
namespace globals {
inline constexpr std::uint32_t object_headers=0x4ecbd0u;   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t clusters=0x52be5cu;         // DataArray* of ClusterRecord
}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u;
namespace cluster {inline constexpr std::uint32_t entry_index=0x18u,entries=0x70u,count=0x74u,entry_size=0x60u;}
inline constexpr std::uint32_t entry_link=0x40u,record_link=0x3cu,point=0x70u;

namespace root_point {
enum class Access {headers,header_datums,walk_object,parent,root_datums,root_object,cluster_handle,clusters,cluster_datums,entry_index,
    count,entries,entry_record,point_record,point_z,point_y,point_x,point_x_store,point_y_store,point_z_store};
enum class Outcome {no_cluster,no_entry,copied};
struct Kept {std::uint32_t point;};   // ESI
// Frame: saved() the saves after the table; walked(handle) each object the
// walk reaches; indexed(index) or unindexed() the entry tested;
// fallback(kept) BBE00, refreshes kept.
template<class Memory,class Frame> Outcome find(Memory &memory,Frame &frame,std::uint32_t handle,std::uint32_t out) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    frame.saved();
    auto root=none;
    if(handle!=none) {
        const auto datums=memory.load32(header_datums,at(table,datum_data));
        for(auto current=handle;current!=none;) {
            root=current;
            frame.walked(current);
            const auto object=memory.load32(walk_object,datums+header_size*(current&0xffffu)+header_address);
            current=memory.load32(parent,at(object,offsetof(ObjectData,parent)));
        }
    }
    Kept kept{out};
    const auto datums=memory.load32(root_datums,at(table,datum_data));
    const auto object=memory.load32(root_object,datums+header_size*(root&0xffffu)+header_address);
    const auto cluster_name=memory.load32(Access::cluster_handle,at(object,offsetof(ObjectData,cluster)));
    if(cluster_name==none){frame.fallback(kept);return Outcome::no_cluster;}
    const auto array=memory.load32(clusters,globals::clusters);
    const auto data=memory.load32(cluster_datums,at(array,datum_data));
    const auto record=std::uint32_t(sizeof(ClusterRecord))*(cluster_name&0xffffu)+data;
    const auto index=signed_value(widen8(memory.load8(Access::entry_index,at(record,cluster::entry_index))));
    if(index<0 || index>=signed_value(memory.load32(count,at(record,cluster::count)))) {
        frame.unindexed();frame.fallback(kept);return Outcome::no_entry;
    }
    frame.indexed(std::uint32_t(index));
    const auto list=memory.load32(entries,at(record,cluster::entries));
    const auto link=memory.load32(entry_record,cluster::entry_size*std::uint32_t(index)+list+entry_link);
    const auto source=memory.load32(point_record,at(link,record_link));
    const auto z=memory.load32(point_z,at(source,point+8u));
    const auto y=memory.load32(point_y,at(source,point+4u));
    memory.store32(point_x_store,kept.point,memory.load32(point_x,at(source,point)));
    memory.store32(point_y_store,at(kept.point,4u),y);
    memory.store32(point_z_store,at(kept.point,8u),z);
    return Outcome::copied;
}
} // namespace root_point
} // namespace halo2::native_root_point
