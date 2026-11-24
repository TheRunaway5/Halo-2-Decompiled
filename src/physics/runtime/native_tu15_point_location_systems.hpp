#pragma once
// Native TU1.5 point location, entered at 11E600 (physics live support
// unit): the location of a point in the loaded structure BSP.
//
//   point_location::locate   11E600(ESI = location*; the point as 14CDB0 takes it)
//
// A location is the collision BSP leaf (+0), the structure cluster (+4) and
// the structure BSP index (+6). With no structure BSP loaded ([474C24] the
// word NONE) the location is NONE throughout. Otherwise 14CDB0 finds the
// leaf in the collision BSP ([4ECC0C]); a leaf names its cluster (the word
// at +0 of the structure's 8-byte leaves, +0x30), and a point in no leaf
// has the cluster NONE.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_point_location.hpp) maps the role to the original
// instruction and owns the ABI of 14CDB0; the location and the BSP index the
// original keeps in ESI and EDI across it are a Kept, refreshed from the
// registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_point_location {
using native_object_update::at;
using native_object_update::none;
using native_object_update::widen16;
namespace globals {
inline constexpr std::uint32_t structure_bsp_index=0x474c24u;   // word: the loaded structure BSP, NONE for none
inline constexpr std::uint32_t collision_bsp=0x4ecc0cu;         // the loaded collision BSP
inline constexpr std::uint32_t structure_bsp=0x4ecc18u;         // the loaded structure BSP
}
struct Location {std::uint32_t leaf;std::uint16_t cluster,bsp;};
static_assert(sizeof(Location)==8 && offsetof(Location,cluster)==4 && offsetof(Location,bsp)==6);
inline constexpr std::uint32_t structure_leaves=0x30u,leaf_size=8u;
inline constexpr std::uint32_t no_word=0xffffu;

namespace point_location {
enum class Access {bsp_index,empty_cluster,empty_bsp,empty_leaf,collision,leaf_store,structure,leaves,cluster,
    bsp_store,cluster_store,outside_bsp_store,outside_cluster_store};
enum class Outcome {no_bsp,located,outside};
struct Kept {std::uint32_t location,bsp;};   // ESI, EDI (its low word)
// Frame: leaf(kept, collision BSP) 14CDB0; refreshes kept; returns EAX.
template<class Memory,class Frame> Outcome locate(Memory &memory,Frame &frame,std::uint32_t out) {
    using enum Access;
    Kept kept{out,memory.load16(bsp_index,globals::structure_bsp_index)&no_word};
    if(kept.bsp==no_word) {
        memory.store16(empty_cluster,at(kept.location,offsetof(Location,cluster)),kept.bsp);
        memory.store16(empty_bsp,at(kept.location,offsetof(Location,bsp)),kept.bsp);
        memory.store32(empty_leaf,at(kept.location,offsetof(Location,leaf)),none);
        return Outcome::no_bsp;
    }
    const auto leaf=frame.leaf(kept,memory.load32(collision,globals::collision_bsp));
    memory.store32(leaf_store,at(kept.location,offsetof(Location,leaf)),leaf);
    if(leaf==none) {
        memory.store16(outside_bsp_store,at(kept.location,offsetof(Location,bsp)),kept.bsp);
        memory.store16(outside_cluster_store,at(kept.location,offsetof(Location,cluster)),no_word);
        return Outcome::outside;
    }
    const auto structure_address=memory.load32(structure,globals::structure_bsp);
    const auto leaf_table=memory.load32(leaves,at(structure_address,structure_leaves));
    const auto cluster_index=memory.load16(Access::cluster,leaf_table+leaf_size*leaf);
    memory.store16(bsp_store,at(kept.location,offsetof(Location,bsp)),kept.bsp);
    memory.store16(cluster_store,at(kept.location,offsetof(Location,cluster)),cluster_index);
    return Outcome::located;
}
} // namespace point_location
} // namespace halo2::native_point_location
