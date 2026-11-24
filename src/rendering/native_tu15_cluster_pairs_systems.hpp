#pragma once
// Native TU1.5 cluster pairs, entered at 118120 (scene device
// constructor unit): whether an object's cluster pairs touch a cluster
// bitmap.
//
//   cluster_pairs::test   118120(object, bitmap, seen*) -> AL, RET C
//
// An object without references (its int16 count +0x1DC not positive)
// answers 0. Otherwise it answers 1, and the seen byte becomes 1 when, for
// any of its references (int16 indices from +0x1DA, into the structure's
// index list, the +0xC of its +0x220 block), either cluster of the indexed
// 36-byte pair (int16 +0 and +2 of the structure's +0x60 pairs) has its bit
// set in the bitmap.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the argument slot the original reuses
// for the pairs' address and the local holding the references' end; the
// boundary adapter (native_tu15_cluster_pairs.hpp) maps the role to
// the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_cluster_pairs {
using native_object_update::at;
using native_object_update::signed_value;
using native_object_update::widen16;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u,structure=0x4ecc18u;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,pair_words=9u;
namespace object {inline constexpr std::uint32_t first_reference=0x1dau,references=0x1dcu;}
namespace structure {inline constexpr std::uint32_t pairs=0x60u,indices_block=0x220u;}
inline constexpr std::uint32_t index_list=0xcu;
namespace argument {inline constexpr std::uint32_t object=0u,bitmap=1u,seen=2u;}

namespace cluster_pairs {
enum class Access {object_argument,headers,datums,object_address,references,first_reference,structure_address,end_store,
    indices_block,pairs,bitmap_argument,index_list_address,pairs_store,pairs_load,reference_index,first_cluster,first_bit,
    second_cluster,second_bit,end_load,seen_argument,seen_store};
enum class Outcome {no_references,unseen,seen,empty_range};
// Frame: argument(k), local() the slots now; saving_esi(), saving_edi(), pairing() and
// restoring(seen) the original's saved registers.
template<class Memory,class Frame> Outcome test(Memory &memory,Frame &frame) {
    using enum Access;
    const auto handle=memory.load32(object_argument,frame.argument(argument::object));
    const auto datums_address=memory.load32(datums,at(memory.load32(headers,globals::object_headers),datum_data));
    const auto object_value=memory.load32(object_address,datums_address+header_size*(handle&0xffffu)+header_address);
    const auto count=widen16(memory.load16(references,at(object_value,object::references)));
    if(signed_value(count)<=0)return Outcome::no_references;
    frame.saving_esi();
    auto reference=widen16(memory.load16(first_reference,at(object_value,object::first_reference)));
    const auto end=count+reference;
    frame.saving_edi();
    const auto structure_value=memory.load32(structure_address,globals::structure);
    memory.store32(end_store,frame.local(),end);
    if(!(signed_value(reference)<signed_value(end)))return Outcome::empty_range;
    const auto block=memory.load32(indices_block,at(structure_value,structure::indices_block));
    const auto pairs_value=memory.load32(pairs,at(structure_value,structure::pairs));
    const auto bitmap=memory.load32(bitmap_argument,frame.argument(argument::bitmap));
    const auto indices=memory.load32(index_list_address,at(block,index_list));
    memory.store32(pairs_store,frame.argument(argument::object),pairs_value);
    frame.pairing();
    const auto bit_of=[&](Access cluster_role,Access bit_role,std::uint32_t at_address) {
        const auto cluster=widen16(memory.load16(cluster_role,at_address));
        return (memory.load32(bit_role,bitmap+4u*std::uint32_t(signed_value(cluster)>>5))&(1u<<(cluster&31u)))!=0u;
    };
    do {
        const auto pairs_now=memory.load32(pairs_load,frame.argument(argument::object));
        const auto pair=pairs_now+4u*pair_words*widen16(memory.load16(reference_index,indices+2u*(reference&0xffffu)));
        if(bit_of(first_cluster,first_bit,pair) || bit_of(second_cluster,second_bit,pair+2u)) {
            const auto seen_address=memory.load32(seen_argument,frame.argument(argument::seen));
            frame.restoring(true);
            memory.store8(seen_store,seen_address,1u);
            return Outcome::seen;
        }
        ++reference;
    } while(signed_value(reference)<signed_value(memory.load32(end_load,frame.local())));
    frame.restoring(false);
    return Outcome::unseen;
}
} // namespace cluster_pairs
} // namespace halo2::native_cluster_pairs
