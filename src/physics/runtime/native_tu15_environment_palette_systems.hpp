#pragma once
// Native TU1.5 campaign environment system: the sky of a structure BSP
// cluster, entered at 133A70 (campaign environment unit).
//
//   cluster_sky::select     133A70  which of the scenario's skies a cluster
//                                   shows, and that sky's tag
//
// A cluster names up to two of the scenario's skies: its own sky and the sky
// visible from it. Its own sky wins whenever it names a valid entry of the
// scenario's sky list, even when that entry has no tag. Otherwise the visible
// sky is used, unless it has no tag or its sky definition has flag bit 4 set
// (the bit the renderer's atmosphere code, bsp_atmosphere.cpp, also tests).
// Otherwise the current structure BSP's default sky is used, when the BSP
// reference enables one and it names a valid entry with a tag. The chosen
// sky's index is written to the caller's output (-1 when none applies) and
// its tag index is returned (NONE when none applies).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores. Every access
// names its role (the Access enum); the boundary adapter
// (native_tu15_environment_palette.hpp) maps the role to the original
// instruction for the diagnostic cursor and keeps the original's registers
// and flags. Registers, flags and stack slots belong to the adapter, not to
// this file.
//
// What stays exact, because translated neighbours observe it: the memory
// accesses happen in the original order and width (the output is cleared
// first; the original reads the cluster's sky byte and the chosen entry's
// tag twice), and address arithmetic is 32-bit and wraps as the original's
// does.
#include <cstddef>
#include <cstdint>

namespace halo2::native_campaign_environment {

// ------------------------------------------------------------------ helpers
constexpr std::uint32_t at(std::uint32_t base,std::size_t offset){return base+std::uint32_t(offset);}
constexpr std::uint32_t element(std::uint32_t base,std::uint32_t index,std::size_t size){return base+index*std::uint32_t(size);}
constexpr std::int32_t signed_value(std::uint32_t x){return std::int32_t(x);}
// MOVSX of a byte or a word.
constexpr std::uint32_t widen8(std::uint32_t byte){return std::uint32_t(std::int32_t(std::int8_t(std::uint8_t(byte))));}
constexpr std::uint32_t widen16(std::uint32_t word){return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(word))));}
// NONE: the empty tag index and the empty output.
inline constexpr std::uint32_t none=0xffffffffu;
// A tag index's instance row is its low word.
constexpr std::uint32_t tag_row(std::uint32_t tag){return tag&0xffffu;}

// ------------------------------------------------------------- game layouts
// Globals at fixed TU1.5 addresses.
namespace globals {
inline constexpr std::uint32_t scenario=0x4ecc20u;             // Scenario*: the scenario tag's data
inline constexpr std::uint32_t tag_instances=0x4f0414u;        // TagInstance[]: the loaded tags
inline constexpr std::uint32_t structure_bsp_index=0x474c24u;  // int16: the current structure BSP
}

// A tag block: a count and the address of its elements.
struct TagBlock {
    std::int32_t count;
    std::uint32_t address;
};
static_assert(sizeof(TagBlock)==8);

// The scenario's tag data: the fields this system reads.
struct Scenario {
    std::uint8_t unused_here_00[8];
    TagBlock skies;                    // +0x08: SkyReference
    std::uint8_t unused_here_10[0x200];
    TagBlock structure_bsps;           // +0x210: StructureBspReference
};
static_assert(offsetof(Scenario,skies)==8 && offsetof(Scenario,structure_bsps)==0x210);

// One entry of the scenario's sky list: a tag reference.
struct SkyReference {
    std::uint32_t group;               // 'sky '
    std::uint32_t sky;                 // +4: the sky's tag index, or NONE
};
static_assert(sizeof(SkyReference)==8 && offsetof(SkyReference,sky)==4);

// One entry of the scenario's structure BSP list (0x44 bytes).
struct StructureBspReference {
    std::uint8_t unused_here_00[0x3c];
    std::uint16_t flags;               // +0x3C: structure_bsp_flags
    std::uint8_t unused_here_3e[2];
    std::int16_t default_sky;          // +0x40: index in the scenario's sky list, or -1
    std::uint8_t unused_here_42[2];
};
static_assert(sizeof(StructureBspReference)==0x44 && offsetof(StructureBspReference,flags)==0x3c &&
              offsetof(StructureBspReference,default_sky)==0x40);
namespace structure_bsp_flags {
inline constexpr std::uint32_t default_sky=0x01u;   // the BSP has a default sky
}

// A loaded tag's instance row (16 bytes).
struct TagInstance {
    std::uint32_t group;
    std::uint32_t identifier;
    std::uint32_t definition;          // +8: the tag's data
    std::uint32_t size;
};
static_assert(sizeof(TagInstance)==16 && offsetof(TagInstance,definition)==8);

// A sky definition: the field this system reads.
struct SkyDefinition {
    std::uint8_t unused_here_00[0x10];
    std::uint32_t flags;               // +0x10: sky_flags
};
static_assert(offsetof(SkyDefinition,flags)==0x10);
namespace sky_flags {
inline constexpr std::uint32_t not_visible_from_other_clusters=0x10u;  // bit 4: a cluster that only sees this
                                                                       // sky does not take it. The tag's own
                                                                       // name for the bit is not established.
}

// A structure BSP cluster: the fields this system reads.
struct Cluster {
    std::uint8_t unused_here_00[0x6c];
    std::int8_t sky;                   // +0x6C: index in the scenario's sky list, or -1
    std::int8_t unused_here_6d;
    std::int8_t visible_sky;           // +0x6E: index in the scenario's sky list, or -1
};
static_assert(offsetof(Cluster,sky)==0x6c && offsetof(Cluster,visible_sky)==0x6e);
// The byte of an empty sky index (-1).
inline constexpr std::uint32_t no_sky=0xffu;
constexpr bool negative_byte(std::uint32_t byte){return (byte&0x80u)!=0;}

// -------------------------------------------------------------- cluster sky
// 133A70. The cluster's sky (see the top of this file).
namespace cluster_sky {
enum class Access {
    scenario_data,output_clear,
    // the cluster's own sky
    own_sky,own_count,own_again,own_list,own_output,own_tag,
    // the sky visible from the cluster
    visible_sky,visible_count,visible_list,visible_tag,tag_instances,visible_definition,visible_flags,
    visible_output,visible_tag_again,
    // the structure BSP's default sky
    bsp_index,bsp_list,bsp_flags,default_index,default_count,default_list,default_tag,default_output,default_tag_again
};
enum class Choice {own_sky,visible_sky,default_sky,no_sky};
struct Result {
    Choice choice;
    std::uint32_t sky_tag;   // the chosen entry's tag index; NONE without a choice
};
// The last candidate: the current structure BSP's default sky, when its
// reference enables one and it is in the sky list with a tag.
template<class Memory> Result structure_bsp_sky(Memory &memory,std::uint32_t scenario,std::uint32_t output) {
    using enum Access;
    const auto current=widen16(memory.load16(bsp_index,globals::structure_bsp_index));
    const auto bsp=element(memory.load32(bsp_list,at(scenario,offsetof(Scenario,structure_bsps.address))),current,
                           sizeof(StructureBspReference));
    if(!(memory.load8(bsp_flags,at(bsp,offsetof(StructureBspReference,flags)))&structure_bsp_flags::default_sky))
        return {Choice::no_sky,none};
    const auto index=memory.load16(default_index,at(bsp,offsetof(StructureBspReference,default_sky)));
    if(index==0xffffu || (index&0x8000u))return {Choice::no_sky,none};
    if(signed_value(widen16(index))>=signed_value(memory.load32(default_count,at(scenario,offsetof(Scenario,skies.count)))))
        return {Choice::no_sky,none};
    const auto entry=element(memory.load32(default_list,at(scenario,offsetof(Scenario,skies.address))),widen16(index),
                             sizeof(SkyReference));
    const auto tag=memory.load32(default_tag,at(entry,offsetof(SkyReference,sky)));
    if(tag==none)return {Choice::no_sky,none};
    memory.store32(default_output,output,widen16(index));
    return {Choice::default_sky,memory.load32(default_tag_again,at(entry,offsetof(SkyReference,sky)))};
}
// cluster: the Cluster; output: where the chosen sky index goes.
template<class Memory> Result select(Memory &memory,std::uint32_t cluster,std::uint32_t output) {
    using enum Access;
    const auto scenario=memory.load32(scenario_data,globals::scenario);
    memory.store32(output_clear,output,none);

    // The cluster's own sky: taken when it is in the sky list, whether or
    // not its entry names a tag. The original reads the index again.
    const auto own=memory.load8(own_sky,at(cluster,offsetof(Cluster,sky)));
    if(own!=no_sky) {
        if(negative_byte(own) ||
           signed_value(widen8(own))>=signed_value(memory.load32(own_count,at(scenario,offsetof(Scenario,skies.count)))))
            return structure_bsp_sky(memory,scenario,output);
        const auto index=widen8(memory.load8(own_again,at(cluster,offsetof(Cluster,sky))));
        const auto entry=element(memory.load32(own_list,at(scenario,offsetof(Scenario,skies.address))),index,sizeof(SkyReference));
        memory.store32(own_output,output,index);
        return {Choice::own_sky,memory.load32(own_tag,at(entry,offsetof(SkyReference,sky)))};
    }

    // The sky visible from the cluster: it must be in the sky list and name
    // a tag, and that sky must not keep to its own clusters.
    const auto visible=memory.load8(visible_sky,at(cluster,offsetof(Cluster,visible_sky)));
    if(visible==no_sky || negative_byte(visible))return structure_bsp_sky(memory,scenario,output);
    const auto index=widen8(visible);
    if(signed_value(index)>=signed_value(memory.load32(visible_count,at(scenario,offsetof(Scenario,skies.count)))))
        return structure_bsp_sky(memory,scenario,output);
    const auto entry=element(memory.load32(visible_list,at(scenario,offsetof(Scenario,skies.address))),index,sizeof(SkyReference));
    const auto tag=memory.load32(visible_tag,at(entry,offsetof(SkyReference,sky)));
    if(tag==none)return structure_bsp_sky(memory,scenario,output);
    const auto instances=memory.load32(tag_instances,globals::tag_instances);
    const auto definition=memory.load32(visible_definition,
        at(element(instances,tag_row(tag),sizeof(TagInstance)),offsetof(TagInstance,definition)));
    if(memory.load8(visible_flags,at(definition,offsetof(SkyDefinition,flags)))&sky_flags::not_visible_from_other_clusters)
        return structure_bsp_sky(memory,scenario,output);
    memory.store32(visible_output,output,index);
    return {Choice::visible_sky,memory.load32(visible_tag_again,at(entry,offsetof(SkyReference,sky)))};
}
} // namespace cluster_sky
} // namespace halo2::native_campaign_environment
