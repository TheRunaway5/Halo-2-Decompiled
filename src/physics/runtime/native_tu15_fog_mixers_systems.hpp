#pragma once
// Native TU1.5 atmospheric fog mixers, entered at 131610 (campaign
// environment unit; the scenario fog evaluation 131AC0 calls it with the
// cluster distance flood's list, 131470).
//
//   fog_mixers::mix   131610  the atmospheric fog of each visible cluster,
//                             with its palette entry's mixers applied
//
// A structure BSP cluster names an entry of the scenario's atmospheric fog
// palette (s_scenario_atmospheric_fog_palette_entry, 0xF4 bytes). An entry
// may list mixers (at most two in the tag): each names another palette
// entry and a scenario interpolator. For every cluster in the list whose
// entry has mixers:
//   1. each mixer whose source and interpolator indices are valid gets a
//      weight, its interpolator's value (137AC0) clamped to [0, 1]. The
//      weights are summed, and `remaining` (the share the entry keeps) is
//      multiplied by (1 - weight) for each;
//   2. when the sum is positive, the mixers' sources are blended into a
//      zeroed entry, each source's fields scaled by weight / sum;
//   3. the entry is copied into the next slot of the output array, then its
//      mixed fields become blended * (1 - remaining) + own * remaining. The
//      list entry's `link` is set to the slot. Fields not mixed (the name,
//      the patchy fog tag, the mixers, bloom and camera immersion) are the
//      entry's own.
// The mixed fields are the atmospheric, secondary, planar override and
// patchy fog colors, densities and distances (MixedField below). The result
// is the number of slots written; nothing is written without a scenario or
// clusters.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores. Every access
// names its role (the Access enum); the boundary adapter
// (native_tu15_fog_mixers.hpp) maps the role to the original instruction for
// the diagnostic cursor. The interpolator call (137AC0), registers and flags
// belong to that adapter, reached through a Frame object. The mixer's locals
// (the field table, the weights, the blended entry, its cursors) live in the
// original's stack frame, so they are read and written as memory, with
// roles, in the original's order; Frame::local gives their addresses (the
// frame moves when a child does not restore the stack pointer).
//
// What stays exact, because translated neighbours observe it:
//   - memory accesses happen in the original order and width (the field
//     table is written in the original's order; the cursors are kept in the
//     frame and read again; a mixer's weight slot is indexed by its position,
//     with no bound);
//   - the single-precision arithmetic runs on the live MXCSR in the
//     original's order, and the clamp compares as COMISS does (a NaN weight
//     is kept), so rounding, NaN payloads and sticky exception flags match;
//   - the blend and the copy are the original's dword loops (REP STOSD and
//     REP MOVSD run downward when the caller leaves the direction flag set;
//     Frame::string_step gives the step); a field's dword count is
//     ((size - 1) >> 2) + 1, address arithmetic wraps at 32 bits;
//   - the values the original keeps in callee-saved registers across the
//     interpolator call (the scenario and the palette entry) are a Walk. A
//     conforming callee returns them unchanged; the adapter hands back what
//     the callee actually returned, so the mixing continues exactly as the
//     original does even when a callee does not preserve them. The entry is
//     also kept in the frame, and the original uses each copy where it does.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>
#include <xmmintrin.h>

namespace halo2::native_atmospheric_fog {
using native_object_update::at;
using native_object_update::element;
using native_object_update::none;
using native_object_update::signed_value;
using native_object_update::widen8;
using native_object_update::widen16;

// ------------------------------------------------------- single precision
// The original's scalar SSE on the live MXCSR, on bit patterns; the left
// operand is the destination register.
namespace sse {
inline __m128 scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline std::uint32_t bits(__m128 x){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x)));}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline std::uint32_t add(std::uint32_t a,std::uint32_t b){auto x=scalar(a);asm volatile("addss %1,%0":"+x"(x):"x"(scalar(b)));return bits(x);}
inline std::uint32_t subtract(std::uint32_t a,std::uint32_t b){auto x=scalar(a);asm volatile("subss %1,%0":"+x"(x):"x"(scalar(b)));return bits(x);}
inline std::uint32_t multiply(std::uint32_t a,std::uint32_t b){auto x=scalar(a);asm volatile("mulss %1,%0":"+x"(x):"x"(scalar(b)));return bits(x);}
inline std::uint32_t divide(std::uint32_t a,std::uint32_t b){auto x=scalar(a);asm volatile("divss %1,%0":"+x"(x):"x"(scalar(b)));return bits(x);}
// COMISS a,b; JA: a > b, ordered.
inline bool above(std::uint32_t a,std::uint32_t b) {
    unsigned char result;asm volatile("comiss %2,%1\n\tseta %0":"=qm"(result):"x"(scalar(a)),"x"(scalar(b)):"cc");return result;
}
#else
inline std::uint32_t add(std::uint32_t a,std::uint32_t b){return bits(_mm_add_ss(scalar(a),scalar(b)));}
inline std::uint32_t subtract(std::uint32_t a,std::uint32_t b){return bits(_mm_sub_ss(scalar(a),scalar(b)));}
inline std::uint32_t multiply(std::uint32_t a,std::uint32_t b){return bits(_mm_mul_ss(scalar(a),scalar(b)));}
inline std::uint32_t divide(std::uint32_t a,std::uint32_t b){return bits(_mm_div_ss(scalar(a),scalar(b)));}
inline bool above(std::uint32_t a,std::uint32_t b){return _mm_comigt_ss(scalar(a),scalar(b));}
#endif
inline constexpr std::uint32_t zero=0u;   // XORPS: +0.0
} // namespace sse

// ------------------------------------------------------------- game layouts
// Globals at fixed TU1.5 addresses.
namespace globals {
inline constexpr std::uint32_t structure_bsp=0x4ecc18u;   // the loaded structure BSP
inline constexpr std::uint32_t scenario=0x4ecc20u;        // the scenario tag's data
inline constexpr std::uint32_t interpolators=0x4f3010u;   // the scenario interpolators' runtime state (0x20 bytes each)
inline constexpr std::uint32_t one=0x46a0d0u;             // float 1.0
}

// The scenario's fields this system reads.
struct Scenario {
    std::uint8_t unused_here_000[0x340];
    std::int32_t atmospheric_fog_count;          // +0x340: the atmospheric fog palette
    std::uint32_t atmospheric_fog;               // +0x344: FogEntry[]
    std::uint8_t unused_here_348[0x78];
    std::int32_t interpolator_count;             // +0x3C0: the scenario interpolators
};
static_assert(offsetof(Scenario,atmospheric_fog_count)==0x340 && offsetof(Scenario,atmospheric_fog)==0x344 &&
              offsetof(Scenario,interpolator_count)==0x3c0);

// The structure BSP's cluster block, and a cluster's atmospheric fog.
struct StructureBsp {
    std::uint8_t unused_here_00[0xa0];
    std::uint32_t clusters;                      // +0xA0: Cluster[] (0xB0 bytes each)
};
static_assert(offsetof(StructureBsp,clusters)==0xa0);
struct Cluster {
    std::uint8_t unused_here_00[0x6f];
    std::int8_t atmospheric_fog;                 // +0x6F: palette index, or -1
    std::uint8_t unused_here_70[0x40];
};
static_assert(sizeof(Cluster)==0xb0 && offsetof(Cluster,atmospheric_fog)==0x6f);
inline constexpr std::uint32_t no_fog=0xffu;

// One entry of the cluster distance flood's list (131470).
struct ClusterDistance {
    std::int32_t cluster;
    std::uint32_t distance;                      // float
    std::uint8_t unused_here_08[12];             // 131AC0's fog choice
    std::int32_t link;                           // +0x14: the cluster's mixed fog slot
};
static_assert(sizeof(ClusterDistance)==0x18 && offsetof(ClusterDistance,link)==0x14);

// An atmospheric fog palette entry: the fields this system reads.
struct FogMixer {
    std::uint32_t unused_here_00[3];             // pad and the two names
    std::int16_t source;                         // +0xC: palette index of the mixed entry
    std::int16_t interpolator;                   // +0xE: scenario interpolator index
};
static_assert(sizeof(FogMixer)==0x10 && offsetof(FogMixer,source)==0xc && offsetof(FogMixer,interpolator)==0xe);
struct FogEntry {
    std::uint8_t unused_here_00[0xd8];           // the mixed fields (MixedField), the patchy fog tag
    std::int32_t mixer_count;                    // +0xD8
    std::uint32_t mixers;                        // +0xDC: FogMixer[]
    std::uint8_t unused_here_e0[0x14];           // bloom and camera immersion overrides
};
static_assert(sizeof(FogEntry)==0xf4 && offsetof(FogEntry,mixer_count)==0xd8 && offsetof(FogEntry,mixers)==0xdc);
inline constexpr std::uint32_t entry_dwords=std::uint32_t(sizeof(FogEntry)/4);

// A scenario interpolator's runtime state; 137AC0 evaluates it.
struct InterpolatorState {
    std::uint32_t unused_here_00;
    std::uint32_t value;                         // +4: handed to 137AC0 with the index
    std::uint8_t unused_here_08[0x18];
};
static_assert(sizeof(InterpolatorState)==0x20 && offsetof(InterpolatorState,value)==4);

// The fields a mixer blends, as byte spans of a FogEntry.
struct MixedField {
    std::uint32_t offset,size;
};
inline constexpr MixedField mixed_fields[]{
    {0x04,12},   // atmospheric fog color
    {0x10,4},    // spread distance
    {0x18,4},    // maximum density
    {0x1c,4},    // start distance
    {0x20,4},    // opaque distance
    {0x24,12},   // secondary fog color
    {0x34,4},    // secondary maximum density
    {0x38,4},    // secondary start distance
    {0x3c,4},    // secondary opaque distance
    {0x44,12},   // planar fog override color
    {0x50,4},    // planar maximum density
    {0x54,4},    // planar override amount
    {0x58,4},    // planar minimum distance bias
    {0x88,24},   // patchy fog color (and its padding)
    {0xa0,4},    // patchy fog density
    {0xa4,4},
    {0xa8,4},    // patchy fog distance
    {0xac,4},
};
inline constexpr std::uint32_t mixed_field_count=std::uint32_t(std::size(mixed_fields));
static_assert(mixed_field_count==18u);

// The mixer's locals (the original's frame below its saved registers).
struct Locals {
    std::uint32_t saved[4];                      // EDI, ESI, EBX saved; one alignment word
    std::uint32_t mixer_offset;                  // +0x10: the mixer's byte offset in the mixer list
    std::uint32_t mixer_index;                   // +0x14
    std::uint32_t list_entry;                    // +0x18: the ClusterDistance being mixed
    std::uint32_t slots_written;                 // +0x1C: the result
    std::uint32_t output_slot;                   // +0x20: the next FogEntry of the output
    std::uint32_t weight_sum;                    // +0x24: float
    std::uint32_t fog_entry;                     // +0x28: the cluster's FogEntry
    std::uint32_t remaining;                     // +0x2C: float, the share the entry keeps
    std::uint32_t clusters_left;                 // +0x30
    std::uint32_t mixer_count;                   // +0x34: for the blend
    std::uint32_t weights[2];                    // +0x38: float, by mixer position (not bounded)
    MixedField fields[18];                       // +0x40
    std::uint32_t blended[entry_dwords];         // +0xD0: a FogEntry
    std::uint32_t alignment;
};
static_assert(sizeof(Locals)==0x1c8 && offsetof(Locals,mixer_offset)==0x10 && offsetof(Locals,weights)==0x38 &&
              offsetof(Locals,fields)==0x40 && offsetof(Locals,blended)==0xd0);

// ------------------------------------------------------------ fog mixers
// 131610. Frame:
//   local(Local)                       the address of one of the mixer's locals
//   argument(Argument)                 the address of one of the caller's arguments
//   string_step()                      the REP string instructions' step (4, or -4 with DF set)
//   interpolate(walk, index, value)    137AC0: the interpolator's value (XMM0)
//   weighed(clamp, weight)             the clamped weight, and which bound replaced it
//   blend_scale(scale)                 1 / the weight sum
//   mixed(step)                        a dword of the output
namespace fog_mixers {
enum class Argument {list,output};
enum class Local {mixer_offset,mixer_index,list_entry,slots_written,output_slot,weight_sum,fog_entry,remaining,
                  clusters_left,mixer_count,weights,fields,blended};
enum class Access {
    field_offset_early,field_size_early,scenario,slots_clear,field_store,
    output_argument,list_argument,output_store,list_store,clusters_left_store,
    // a cluster
    list_entry,entry_cluster,structure_bsp,clusters,fog_index,fog_count,fog_palette,fog_entry_store,mixer_count,
    unit_weight,weight_0_clear,weight_sum_clear,remaining_clear,weight_1_clear,mixer_index_clear,mixer_offset_clear,
    // a mixer's weight
    mixers,mixer_offset,mixer_source,palette_count,mixer_interpolator,interpolator_count,interpolators,
    interpolator_value,remaining_reload,unit_weight_reload,mixer_index_reload,weight_sum,weight_sum_store,weight_store,
    remaining_store,mixer_index_next,mixer_offset_next,mixer_count_next,mixer_index_store,mixer_offset_store,
    // the blend
    weight_total,blended_clear,blend_mixer_count,blend_index_clear,blend_count_store,blend_offset_clear,
    blend_weight,blend_mixers,blend_offset,blend_source,blend_scenario,blend_palette_count,blend_palette,
    blend_field_size,blend_field_offset,blend_source_field,blend_accumulator,blend_accumulator_store,blend_entry_reload,
    blend_count_reload,blend_index_reload,blend_offset_reload,blend_index_store,blend_offset_store,
    // the output
    output_count,output_entry,output_slot,link_store,copy,mix_field_size,mix_field_offset,mix_output,mix_entry,
    mix_original,mix_blended,mix_store,mix_entry_reload,output_count_reload,output_slot_reload,scenario_reload,
    output_count_store,output_slot_store,
    // the next cluster
    next_entry,clusters_left,next_entry_store,clusters_left_next,
    result
};
enum class Exit {no_scenario,no_clusters,mixed};
// Which bound replaced a weight (COMISS against 0, then 1).
enum class Clamp {kept,zero,one};
// Kept across the interpolator call.
struct Walk {
    std::uint32_t scenario;
    std::uint32_t entry;        // the cluster's FogEntry
};
// One dword of the output: blended * complement + own * remaining.
struct MixStep {
    std::uint32_t complement;   // 1 - remaining
    std::uint32_t own_share;    // own * remaining
    std::uint32_t result;
};
// A field's dword count, as the original computes it.
constexpr std::uint32_t field_dwords(std::uint32_t size){return ((size-1u)>>2)+1u;}

// The field table, in the original's store order: two entries' words come
// first (they share a register), then the rest in order.
template<class Memory,class Frame> std::uint32_t write_field_table(Memory &memory,Frame &frame) {
    using enum Access;
    const auto field=[&](std::uint32_t index,bool size) {
        return element(frame.local(Local::fields),index,sizeof(MixedField))+(size?4u:0u);
    };
    memory.store32(field_offset_early,field(2,false),mixed_fields[2].offset);
    memory.store32(field_size_early,field(13,true),mixed_fields[13].size);
    const auto scenario_data=memory.load32(Access::scenario,globals::scenario);
    memory.store32(slots_clear,frame.local(Local::slots_written),0);
    for(std::uint32_t index=0;index<mixed_field_count;++index) {
        if(index!=2)memory.store32(field_store,field(index,false),mixed_fields[index].offset);
        if(index!=13)memory.store32(field_store,field(index,true),mixed_fields[index].size);
    }
    return scenario_data;
}

// Step 1: the weight of the mixer at mixer_offset, or nothing when its
// source or interpolator index is out of range.
template<class Memory,class Frame> void weigh(Memory &memory,Frame &frame,Walk &walk,std::uint32_t &one,
                                              std::uint32_t &remaining) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    const auto list=memory.load32(mixers,at(walk.entry,offsetof(FogEntry,mixers)));
    const auto mixer=list+memory.load32(mixer_offset,local(Local::mixer_offset));
    const auto source=memory.load16(mixer_source,at(mixer,offsetof(FogMixer,source)));
    // The index clamped to the palette must be the index itself.
    std::uint32_t clamped=0;
    if(!(source&0x8000u)) {
        const auto last=memory.load32(palette_count,at(walk.scenario,offsetof(Scenario,atmospheric_fog_count)))-1u;
        clamped=signed_value(widen16(source))>signed_value(last)?last:widen16(source);
    }
    if(clamped!=widen16(source))return;
    const auto interpolator=memory.load16(mixer_interpolator,at(mixer,offsetof(FogMixer,interpolator)));
    clamped=0;
    if(!(interpolator&0x8000u)) {
        const auto last=memory.load32(interpolator_count,at(walk.scenario,offsetof(Scenario,interpolator_count)))-1u;
        clamped=signed_value(widen16(interpolator))>signed_value(last)?last:widen16(interpolator);
    }
    const auto index=widen16(interpolator);
    if(clamped!=index)return;
    // The interpolator's state; a null state weighs nothing.
    const auto state=at(element(memory.load32(interpolators,globals::interpolators),index,sizeof(InterpolatorState)),
                        offsetof(InterpolatorState,value));
    std::uint32_t weight=sse::zero;
    if(state) {
        weight=frame.interpolate(walk,index,memory.load32(interpolator_value,state));
        remaining=memory.load32(remaining_reload,local(Local::remaining));
        one=memory.load32(unit_weight_reload,globals::one);
    }
    auto clamp=Clamp::kept;
    if(sse::above(sse::zero,weight))clamp=Clamp::zero,weight=sse::zero;
    else if(sse::above(weight,one))clamp=Clamp::one,weight=one;
    frame.weighed(clamp,weight);
    const auto position=memory.load32(mixer_index_reload,local(Local::mixer_index));
    const auto sum=sse::add(weight,memory.load32(weight_sum,local(Local::weight_sum)));
    memory.store32(weight_sum_store,local(Local::weight_sum),sum);
    remaining=sse::multiply(sse::subtract(one,weight),remaining);
    memory.store32(weight_store,element(local(Local::weights),position,4),weight);
    memory.store32(remaining_store,local(Local::remaining),remaining);
}

// Step 2: each weighed mixer's source, scaled, into the blended entry.
template<class Memory,class Frame> void blend(Memory &memory,Frame &frame,Walk &walk,std::uint32_t scale,
                                              std::uint32_t &position,std::uint32_t count) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    do {
        const auto weight=memory.load32(blend_weight,element(local(Local::weights),position,4));
        if(sse::above(weight,sse::zero)) {
            const auto list=memory.load32(blend_mixers,at(walk.entry,offsetof(FogEntry,mixers)));
            const auto offset=memory.load32(blend_offset,local(Local::mixer_offset));
            const auto index=widen16(memory.load16(blend_source,at(offset+list,offsetof(FogMixer,source))));
            std::uint32_t source=0;
            if(signed_value(index)>=0) {
                const auto scenario=memory.load32(blend_scenario,globals::scenario);
                if(signed_value(index)<signed_value(memory.load32(blend_palette_count,
                                                                  at(scenario,offsetof(Scenario,atmospheric_fog_count)))))
                    source=element(memory.load32(blend_palette,at(scenario,offsetof(Scenario,atmospheric_fog))),index,
                                   sizeof(FogEntry));
            }
            for(std::uint32_t field=0;field<mixed_field_count;++field) {
                const auto size=memory.load32(blend_field_size,element(local(Local::fields),field,sizeof(MixedField))+4u);
                if(signed_value(size)<=0)continue;
                const auto offset_in_entry=memory.load32(blend_field_offset,
                                                        element(local(Local::fields),field,sizeof(MixedField)));
                auto accumulator=local(Local::blended)+offset_in_entry;
                auto from=source+offset_in_entry;
                for(auto dwords=field_dwords(size);dwords;--dwords,accumulator+=4u,from+=4u) {
                    auto value=sse::multiply(memory.load32(blend_source_field,from),weight);
                    value=sse::multiply(value,scale);
                    value=sse::add(value,memory.load32(blend_accumulator,accumulator));
                    memory.store32(blend_accumulator_store,accumulator,value);
                }
                walk.entry=memory.load32(blend_entry_reload,local(Local::fog_entry));
            }
            count=memory.load32(blend_count_reload,local(Local::mixer_count));
            position=memory.load32(blend_index_reload,local(Local::mixer_index));
        }
        const auto offset=memory.load32(blend_offset_reload,local(Local::mixer_offset))+std::uint32_t(sizeof(FogMixer));
        ++position;
        memory.store32(blend_index_store,local(Local::mixer_index),position);
        memory.store32(blend_offset_store,local(Local::mixer_offset),offset);
    } while(signed_value(position)<signed_value(count));
}

// Step 3: the entry into the next output slot, with its mixed fields.
template<class Memory,class Frame> void write_slot(Memory &memory,Frame &frame,Walk &walk,std::uint32_t one,
                                                   std::uint32_t remaining) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    const auto slot_index=memory.load32(output_count,local(Local::slots_written));
    const auto entry=memory.load32(output_entry,local(Local::list_entry));
    const auto slot=memory.load32(output_slot,local(Local::output_slot));
    memory.store32(link_store,at(entry,offsetof(ClusterDistance,link)),slot_index);
    const auto step=frame.string_step();
    for(std::uint32_t n=0;n<entry_dwords;++n)
        memory.store32(copy,slot+n*step,memory.load32(copy,walk.entry+n*step));
    for(std::uint32_t field=0;field<mixed_field_count;++field) {
        const auto size=memory.load32(mix_field_size,element(local(Local::fields),field,sizeof(MixedField))+4u);
        if(signed_value(size)<=0)continue;
        const auto offset_in_entry=memory.load32(mix_field_offset,element(local(Local::fields),field,sizeof(MixedField)));
        const auto output=memory.load32(mix_output,local(Local::output_slot));
        const auto relative=output-memory.load32(mix_entry,local(Local::fog_entry));
        MixStep step_values{sse::subtract(one,remaining),0,0};
        auto own=walk.entry+offset_in_entry;
        auto blended=local(Local::blended)+offset_in_entry;
        for(auto dwords=field_dwords(size);dwords;--dwords,own+=4u,blended+=4u) {
            const auto own_value=memory.load32(mix_original,own);
            const auto mixed_value=sse::multiply(memory.load32(mix_blended,blended),step_values.complement);
            step_values.own_share=sse::multiply(own_value,remaining);
            step_values.result=sse::add(mixed_value,step_values.own_share);
            memory.store32(mix_store,own+relative,step_values.result);
            frame.mixed(step_values);
        }
        walk.entry=memory.load32(mix_entry_reload,local(Local::fog_entry));
    }
    const auto written=memory.load32(output_count_reload,local(Local::slots_written))+1u;
    const auto next_slot=memory.load32(output_slot_reload,local(Local::output_slot))+std::uint32_t(sizeof(FogEntry));
    walk.scenario=memory.load32(scenario_reload,globals::scenario);
    memory.store32(output_count_store,local(Local::slots_written),written);
    memory.store32(output_slot_store,local(Local::output_slot),next_slot);
}

// One list entry's cluster (steps 1 to 3).
template<class Memory,class Frame> void mix_cluster(Memory &memory,Frame &frame,Walk &walk) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    const auto entry=memory.load32(list_entry,local(Local::list_entry));
    const auto cluster_index=memory.load32(entry_cluster,at(entry,offsetof(ClusterDistance,cluster)));
    const auto bsp=memory.load32(structure_bsp,globals::structure_bsp);
    const auto cluster=cluster_index*std::uint32_t(sizeof(Cluster))+memory.load32(clusters,at(bsp,offsetof(StructureBsp,clusters)));
    const auto fog=memory.load8(fog_index,at(cluster,offsetof(Cluster,atmospheric_fog)));
    if(fog==no_fog)return;
    const auto index=widen8(fog);
    if(signed_value(index)<0)return;
    if(signed_value(index)>=signed_value(memory.load32(fog_count,at(walk.scenario,offsetof(Scenario,atmospheric_fog_count)))))
        return;
    walk.entry=element(memory.load32(fog_palette,at(walk.scenario,offsetof(Scenario,atmospheric_fog))),index,sizeof(FogEntry));
    memory.store32(fog_entry_store,local(Local::fog_entry),walk.entry);
    if(!walk.entry)return;
    const auto count=memory.load32(mixer_count,at(walk.entry,offsetof(FogEntry,mixer_count)));
    if(signed_value(count)<=0)return;

    // Step 1.
    auto one=memory.load32(unit_weight,globals::one);
    auto remaining=one;
    memory.store32(weight_0_clear,local(Local::weights),0);
    memory.store32(weight_sum_clear,local(Local::weight_sum),sse::zero);
    memory.store32(remaining_clear,local(Local::remaining),remaining);
    memory.store32(weight_1_clear,local(Local::weights)+4u,0);
    memory.store32(mixer_index_clear,local(Local::mixer_index),0);
    memory.store32(mixer_offset_clear,local(Local::mixer_offset),0);
    std::uint32_t position=0,limit=0;
    do {
        weigh(memory,frame,walk,one,remaining);
        position=memory.load32(mixer_index_next,local(Local::mixer_index))+1u;
        const auto offset=memory.load32(mixer_offset_next,local(Local::mixer_offset))+std::uint32_t(sizeof(FogMixer));
        limit=memory.load32(mixer_count_next,at(walk.entry,offsetof(FogEntry,mixer_count)));
        memory.store32(mixer_index_store,local(Local::mixer_index),position);
        memory.store32(mixer_offset_store,local(Local::mixer_offset),offset);
    } while(signed_value(position)<signed_value(limit));
    const auto total=memory.load32(weight_total,local(Local::weight_sum));
    if(!sse::above(total,sse::zero))return;

    // Step 2.
    const auto step=frame.string_step();
    for(std::uint32_t n=0;n<entry_dwords;++n)memory.store32(blended_clear,local(Local::blended)+n*step,0);
    const auto count_now=memory.load32(blend_mixer_count,at(walk.entry,offsetof(FogEntry,mixer_count)));
    const auto scale=sse::divide(one,total);
    frame.blend_scale(scale);
    memory.store32(blend_index_clear,local(Local::mixer_index),0);
    memory.store32(blend_count_store,local(Local::mixer_count),count_now);
    if(signed_value(count_now)>0) {
        memory.store32(blend_offset_clear,local(Local::mixer_offset),0);
        position=0;
        blend(memory,frame,walk,scale,position,count_now);
    }

    // Step 3.
    write_slot(memory,frame,walk,one,remaining);
}

template<class Memory,class Frame> Exit mix(Memory &memory,Frame &frame,std::uint32_t cluster_count) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    Walk walk{write_field_table(memory,frame),0};
    if(!walk.scenario)return Exit::no_scenario;
    if(signed_value(cluster_count)<=0)return Exit::no_clusters;
    const auto output=memory.load32(output_argument,frame.argument(Argument::output));
    const auto list=memory.load32(list_argument,frame.argument(Argument::list));
    memory.store32(output_store,local(Local::output_slot),output);
    memory.store32(list_store,local(Local::list_entry),list);
    memory.store32(clusters_left_store,local(Local::clusters_left),cluster_count);
    std::uint32_t left=0;
    do {
        mix_cluster(memory,frame,walk);
        const auto next=memory.load32(next_entry,local(Local::list_entry))+std::uint32_t(sizeof(ClusterDistance));
        left=memory.load32(Access::clusters_left,local(Local::clusters_left))-1u;
        memory.store32(next_entry_store,local(Local::list_entry),next);
        memory.store32(clusters_left_next,local(Local::clusters_left),left);
    } while(left);
    memory.load32(result,local(Local::slots_written));
    return Exit::mixed;
}
} // namespace fog_mixers
} // namespace halo2::native_atmospheric_fog
