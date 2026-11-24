#pragma once
// Native TU1.5 object connect, entered at BA630 (physics live support unit):
// an object (re)registered with the structure's clusters.
//
//   object_connect::connect   BA630(object, location*) -> RET 8
//
// The object (through the object headers at [4ECBD0]: its header's word +4
// is its cluster, NONE while unplaced) takes the given location, or the one
// 11E600 finds at its center (+0x40; EDX the point, ESI the location built
// in the frame), or failing that at its position (+0x64). A location with a
// cluster (+4) becomes the object's (+0x28, +0x2C; the header's cluster) and
// clears the object's unplaced bit (bit 18 of +4); one without sets it. An
// object flagged for every cluster (bit 21) passes a cluster set of all
// clusters (the structure's count +0x9C at [4ECC18], one bit each, filled
// with ones in the frame) in place of the location argument. Then:
//   - BA910 summarizes it (EDX the object, EDI a summary in the frame);
//   - 1D2F10 (EAX the cluster set) inserts it into its partition (4EABB0
//     under bit 9 of +4, else 4EABA4) with its head (+0x60), center, radius
//     (+0x4C), location, the summary (20 bytes) and the object's argument
//     slot as the overflow flag;
//   - the object is marked connected (bit 8 of +4) and its header registered
//     (bit 6 of byte +2); C0F60 hears of it (the object, 0, 1, 0, 0);
//   - B8D90 (EAX the object; the game's active clusters [4F3218]+0x11B8)
//     decides: active objects are activated (B92C0, EDI the object); others
//     flagged bit 17 that B1870 (EAX the object) does not keep are released
//     (BA570, the object pushed); the rest are parked (B9330, EDX the object).
// An object that was unplaced and now has a cluster finally goes through
// 1496F0, 1CC290 (the object twice) and 282C10, and 1496F0 again.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the original's frame (the unplaced
// byte, the location, the summary, the cluster set) and the argument slots
// it reuses; the boundary adapter (native_tu15_object_connect.hpp) maps the
// role to the original instruction and owns the ABI of the children. The
// values the original keeps in callee-saved registers across them are a
// Kept, refreshed from the registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_object_connect {
using native_object_update::at;
using native_object_update::signed_value;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u,structure=0x4ecc18u,game=0x4f3218u,
    active_partition=0x4eabb0u,static_partition=0x4eaba4u;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,no_cluster=0xffffu,summary_size=0x14u;
namespace header {inline constexpr std::uint32_t flags=2u,cluster=4u;inline constexpr std::uint32_t registered=0x40u;}
namespace object {inline constexpr std::uint32_t flags=4u,location=0x28u,location_cluster=0x2cu,center=0x40u,radius=0x4cu,head=0x60u,
    position=0x64u;}
namespace bit {inline constexpr std::uint32_t connected=0x100u,active_partition=9u,releasable=17u,unplaced=0x40000u,every_cluster=21u;}
namespace structure {inline constexpr std::uint32_t clusters=0x9cu;}
namespace game {inline constexpr std::uint32_t active_clusters=0x11b8u;}
namespace argument {inline constexpr std::uint32_t object=0u,location=1u;}
// The frame's slots, by offset from the return address.
namespace local {inline constexpr std::int32_t unplaced=-0x5d,location=-0x5c,location_cluster=-0x58,summary=-0x54,cluster_set=-0x40;}

namespace object_connect {
enum class Access {object_argument,headers,datums,header_cluster,object_address,location_argument,unplaced_store,local_cluster,
    location_cluster,location_first,location_store,location_second,location_cluster_store,header_cluster_store,flags_placed,
    flags_unplaced,flags_store,set_clear,structure_address,cluster_count,set_fill,set_fill_byte,set_store,object_again,
    partition_flags,radius,set_argument,connected_flags,connected_store,header_flags,header_flags_store,game_address,
    release_flags,unplaced_load,header_cluster_again};
enum class Outcome {activated,released,parked};
// The original's EBX (the header), EBP (the object's address), ESI (the location, then the object) and EDI (the
// location in use) across the children.
struct Kept {std::uint32_t header,object,source,location;};
// Frame:
//   argument(k), local(offset)                   the argument slots and the frame's slots now
//   indexing(handle), saving()                   PUSH EBX; AND; PUSH EBP; LEA; PUSH ESI; PUSH EDI
//   unplaced_tested(cluster), location_tested(location)        the header's CMP and TEST EDI,EDI
//   locate(point, out, kept)                     11E600(EDX = point, ESI = out), refreshes kept
//   cleared(), cluster_tested(cluster), flagged(flags)          OR EAX,-1; the CMPs; SHR EDX,15; TEST DL,1
//   filling(clusters), filling_rest(), step(width)              the cluster set's REP STOSD and STOSB
//   summarize(kept)                              BA910(EDX = object, EDI = the summary), refreshes kept
//   partition_tested(flags, partition)           SHR ECX,9; TEST CL,1; MOV EAX,partition
//   insert_arguments(overflow, payload), insert_object(kept, radius, partition), insert(set, kept)
//                                                1D2F10's nine arguments and the call (EAX the set), refreshes kept
//   connecting(flags), registering(flags, kept)  the pushes for C0F60 between the flag updates
//   notify(kept)                                 C0F60(the object, 0, 1, 0, 0), refreshes kept
//   in_active(clusters, kept) -> AL, activate(kept), keeps(kept) -> AL, release(kept), park(kept)   B8D90, B92C0, B1870, BA570, B9330
//   relink(kept)                                 1496F0, 1CC290(the object twice), 282C10, 1496F0, refresh kept
template<class Memory,class Frame> Outcome connect(Memory &memory,Frame &frame) {
    using enum Access;
    Kept kept{};
    const auto handle=memory.load32(object_argument,frame.argument(argument::object));
    const auto datums_address=memory.load32(datums,at(memory.load32(headers,globals::object_headers),datum_data));
    frame.indexing(handle);
    kept.header=datums_address+header_size*(handle&0xffffu);
    const auto cluster=memory.load16(header_cluster,at(kept.header,header::cluster));
    frame.unplaced_tested(cluster);
    kept.object=memory.load32(object_address,at(kept.header,header_address));
    frame.saving();
    kept.location=memory.load32(location_argument,frame.argument(argument::location));
    memory.store8(unplaced_store,frame.local(local::unplaced),cluster==no_cluster?1u:0u);
    frame.location_tested(kept.location);
    if(!kept.location) {
        frame.locate(at(kept.object,object::center),frame.local(local::location),kept);
        const auto found=memory.load16(local_cluster,frame.local(local::location_cluster));
        frame.cluster_tested(found);
        kept.location=kept.source;
        if(found==no_cluster)frame.locate(at(kept.object,object::position),kept.source,kept);
    }
    frame.cleared();
    const auto location_cluster_value=memory.load16(location_cluster,at(kept.location,4u));
    frame.cluster_tested(location_cluster_value);
    std::uint32_t flags;
    if(location_cluster_value!=no_cluster) {
        memory.store32(location_store,at(kept.object,object::location),memory.load32(location_first,kept.location));
        const auto second=memory.load32(location_second,at(kept.location,4u));
        memory.store32(location_cluster_store,at(kept.object,object::location_cluster),second);
        memory.store16(header_cluster_store,at(kept.header,header::cluster),second&0xffffu);
        flags=memory.load32(flags_placed,at(kept.object,object::flags))&~bit::unplaced;
    } else {
        flags=memory.load32(flags_unplaced,at(kept.object,object::flags))|bit::unplaced;
    }
    frame.flagged(flags);
    memory.store32(flags_store,at(kept.object,object::flags),flags);
    memory.store32(set_clear,frame.argument(argument::location),0u);
    if((flags>>bit::every_cluster)&1u) {
        const auto clusters=memory.load32(cluster_count,at(memory.load32(structure_address,globals::structure),structure::clusters));
        const auto bytes=std::uint32_t(signed_value(clusters+0x1fu)>>5)<<2;
        frame.filling(clusters);
        const auto word_step=frame.step(4u);
        auto to=frame.local(local::cluster_set);
        for(std::uint32_t k=0;k<(bytes>>2);++k,to+=word_step)memory.store32(set_fill,to,0xffffffffu);
        frame.filling_rest();
        const auto byte_step=frame.step(1u);
        for(std::uint32_t k=0;k<(bytes&3u);++k,to+=byte_step)memory.store8(set_fill_byte,to,0xffu);
        memory.store32(set_store,frame.argument(argument::location),frame.local(local::cluster_set));
    }
    kept.source=memory.load32(object_again,frame.argument(argument::object));
    frame.summarize(kept);
    const auto partition_bits=memory.load32(partition_flags,at(kept.object,object::flags));
    const auto partition=((partition_bits>>bit::active_partition)&1u)?globals::active_partition:globals::static_partition;
    frame.partition_tested(partition_bits,partition);
    frame.insert_arguments(frame.argument(argument::object),frame.local(local::summary));
    const auto radius_value=memory.load32(radius,at(kept.object,object::radius));
    frame.insert_object(kept,radius_value,partition);
    frame.insert(memory.load32(set_argument,frame.argument(argument::location)),kept);
    const auto connected=memory.load32(connected_flags,at(kept.object,object::flags))|bit::connected;
    frame.connecting(connected);
    memory.store32(connected_store,at(kept.object,object::flags),connected);
    const auto registered=memory.load8(header_flags,at(kept.header,header::flags))|header::registered;
    frame.registering(registered,kept);
    memory.store8(header_flags_store,at(kept.header,header::flags),registered);
    frame.notify(kept);
    auto outcome=Outcome::parked;
    const auto game_value=memory.load32(game_address,globals::game);
    if(frame.in_active(at(game_value,game::active_clusters),kept)&0xffu) {
        frame.activate(kept);
        outcome=Outcome::activated;
    } else {
        const auto release=memory.load32(release_flags,at(kept.object,object::flags));
        frame.release_tested(release);
        if(((release>>bit::releasable)&1u) && !(frame.keeps(kept)&0xffu)) {
            frame.release(kept);
            outcome=Outcome::released;
        } else frame.park(kept);
    }
    const auto unplaced_now=memory.load8(unplaced_load,frame.local(local::unplaced));
    frame.unplaced_now(unplaced_now);
    if(unplaced_now) {
        const auto cluster_now=memory.load16(header_cluster_again,at(kept.header,header::cluster));
        frame.placed_tested(cluster_now);
        if(cluster_now!=no_cluster)frame.relink(kept);
    }
    return outcome;
}
} // namespace object_connect
} // namespace halo2::native_object_connect
