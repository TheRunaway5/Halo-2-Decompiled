#pragma once
// Native TU1.5 permutation choice, entered at 1700E0 (physics live support
// unit): a model's permutation for each of its regions.
//
//   permutation_choice::choose   1700E0(model tag, variant, chosen bytes, records, held mask); RET 14
//
// The model's definition ([4F0414], 16-byte entries, the address at +8) has
// regions (+0x70 of them, 16 bytes each at +0x74). Each region's chosen byte
// (the third argument) starts NONE, and its 8-byte record (the fourth) a
// NONE marker, two zero bytes and a NONE link. Then for each region a list
// of up to 32 candidates (0x1C bytes on the frame: a permutation, a NONE
// marker, two zero fields, a NONE link, a zero field and the weight
// [46A0D0]) is gathered: with a variant (the second argument; 0x38 bytes
// each at +0x54) and a model tag, the variant's region byte (+4) other than
// NONE has 170930 (EDI the list, EDX the count; the variant's candidates,
// 20 bytes each at +0x18, NONE, two zeros, 1 and 32 pushed) fill it; when it
// finds none, each of the region's permutations (+8 of them, 8 bytes each at
// +0xC) without the flag bit 0 (+4) is listed, four at a time and then the
// rest, and a region whose permutations are all flagged lists its first.
// Unless the region's bit is held in the mask (the fifth argument), 170B40
// (EBX the list, EDX the count) chooses a candidate: its permutation is the
// region's chosen byte, and its marker and two fields the record's bytes.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_permutation_choice.hpp) maps the role to the original
// instruction, owns the ABI of the children and leaves through the
// original's epilogue. The values the original keeps in callee-saved
// registers (and XMM4) across the children are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_permutation_choice {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t definitions=0x4f0414u,weight=0x46a0d0u;}
inline constexpr std::uint32_t none=0xffffffffu,handle_mask=0xffffu,none_byte=0xffu,listed_capacity=0x20u,gather_mode=1u;
namespace definition_entry {inline constexpr std::uint32_t size=16u,address=8u;}
namespace definition {inline constexpr std::uint32_t variants=0x54u,region_count=0x70u,regions=0x74u;}
namespace region {inline constexpr std::uint32_t size=16u,permutation_count=8u,permutations=0xcu;}
namespace permutation {inline constexpr std::uint32_t size=8u,flags=4u;inline constexpr std::uint32_t flagged=1u;}
namespace variant {inline constexpr std::uint32_t size=0x38u,regions=4u,candidates=0x18u;inline constexpr std::uint32_t candidate_size=20u;}
// A candidate on the frame, and a region's record.
namespace candidate {inline constexpr std::uint32_t permutation=0u,marker=4u,first=8u,second=0xcu,link=0x10u,spare=0x14u,weight=0x18u,
    size=0x1cu;}
namespace record {inline constexpr std::uint32_t marker=0u,first=1u,second=2u,link=4u,size=8u;}
// Four candidates at a time while at least four permutations remain.
inline constexpr std::uint32_t unrolled=4u;
// The frame (offsets from the ESP after EBX, EBP, ESI and EDI are pushed).
namespace local {inline constexpr std::uint32_t weight_cursor=0x10u,count=0x14u,permutation=0x18u,permutation_cursor=0x1cu,
    region_index=0x20u,region_offset=0x24u,output_cursor=0x28u,definition=0x2cu,entry=0x30u,region=0x34u,unrolled_limit=0x38u,
    list=0x3cu,tag=0x3c0u,variant=0x3c4u,chosen=0x3c8u,records=0x3ccu,held=0x3d0u;}
inline constexpr std::uint32_t signed_byte(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(std::uint8_t(v))));}

namespace permutation_choice {
enum class Access {
    tag_argument,definitions_load,output_argument,definition_load,entry_store,region_count,definition_store,chosen_argument,
    chosen_clear,output_marker_clear,output_first_clear,output_second_clear,output_link_clear,region_count_again,regions_test,
    region_index_store,weight_load,region_offset_store,output_cursor_store,region_offset_load,definition_reload,regions_load,
    variant_argument,region_store,count_store,variants_load,tag_test,entry_load,entry_definition,entry_variants,region_index_load,
    variant_permutation,variant_candidates,gathered_count,permutation_count,permutation_store,permutations_load,
    unrolled_limit_store,weight_cursor_store,permutation_cursor_store,permutation_cursor_load,flags_0,index_0,index_store_0,
    weight_cursor_0,marker_store_0,first_store_0,second_store_0,link_store_0,spare_store_0,weight_store_0,count_load_0,
    count_store_0,weight_cursor_again_0,weight_cursor_next_0,permutation_cursor_0,flags_1,index_1,index_store_1,weight_cursor_1,
    marker_store_1,first_store_1,second_store_1,link_store_1,spare_store_1,weight_store_1,count_load_1,count_store_1,
    weight_cursor_again_1,weight_cursor_next_1,permutation_cursor_1,flags_2,index_2,index_store_2,weight_cursor_2,marker_store_2,
    first_store_2,second_store_2,link_store_2,spare_store_2,weight_store_2,count_load_2,count_store_2,weight_cursor_again_2,
    weight_cursor_next_2,permutation_cursor_2,flags_3,index_3,index_store_3,weight_cursor_3,marker_store_3,first_store_3,
    second_store_3,link_store_3,spare_store_3,weight_store_3,count_load_3,count_store_3,weight_cursor_again_3,weight_cursor_next_3,
    permutation_index_load,permutation_index_store,permutation_cursor_add_load,permutation_cursor_add_store,unrolled_limit_load,
    permutation_index_test,permutation_index_after,region_reload,count_after,permutation_total,permutations_rest,flags_rest,
    index_rest,marker_rest,first_rest,second_rest,link_rest,spare_rest,weight_rest,default_marker,default_link,default_index,
    default_first,default_second,default_spare,default_weight,mask_region,mask_argument,chosen_region,chosen_argument_again,
    chosen_permutation,chosen_store,chosen_marker,output_cursor_load,output_marker_store,chosen_second,chosen_first,
    output_second_store,output_first_store,output_cursor_next,region_index_next,region_offset_next,output_cursor_advance,
    definition_next,region_count_next,region_index_advance,region_offset_advance};
// The roles of the four listings of an unrolled pass (the last reloads no permutation cursor).
struct Listing {Access flags,index,index_store,weight_cursor,marker,first,second,link,spare,weight,count_load,count_store,
    weight_cursor_again,weight_cursor_next,permutation_cursor;};
inline constexpr Listing listings[unrolled]{
    {Access::flags_0,Access::index_0,Access::index_store_0,Access::weight_cursor_0,Access::marker_store_0,Access::first_store_0,
        Access::second_store_0,Access::link_store_0,Access::spare_store_0,Access::weight_store_0,Access::count_load_0,
        Access::count_store_0,Access::weight_cursor_again_0,Access::weight_cursor_next_0,Access::permutation_cursor_0},
    {Access::flags_1,Access::index_1,Access::index_store_1,Access::weight_cursor_1,Access::marker_store_1,Access::first_store_1,
        Access::second_store_1,Access::link_store_1,Access::spare_store_1,Access::weight_store_1,Access::count_load_1,
        Access::count_store_1,Access::weight_cursor_again_1,Access::weight_cursor_next_1,Access::permutation_cursor_1},
    {Access::flags_2,Access::index_2,Access::index_store_2,Access::weight_cursor_2,Access::marker_store_2,Access::first_store_2,
        Access::second_store_2,Access::link_store_2,Access::spare_store_2,Access::weight_store_2,Access::count_load_2,
        Access::count_store_2,Access::weight_cursor_again_2,Access::weight_cursor_next_2,Access::permutation_cursor_2},
    {Access::flags_3,Access::index_3,Access::index_store_3,Access::weight_cursor_3,Access::marker_store_3,Access::first_store_3,
        Access::second_store_3,Access::link_store_3,Access::spare_store_3,Access::weight_store_3,Access::count_load_3,
        Access::count_store_3,Access::weight_cursor_again_3,Access::weight_cursor_next_3,Access::flags_3},
};
// The original's EBX (zero, as the original keeps it), EBP (the records,
// then each region) and XMM4's low lane (the weight).
struct Kept {std::uint32_t zero,region,weight;};

// Frame: see native_tu15_permutation_choice.hpp.
template<class Memory,class Frame> void choose(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    // The definition; each region's chosen byte and record cleared.
    frame.framing();
    const auto tag=memory.load32(tag_argument,frame.local(local::tag));
    const auto definitions=memory.load32(definitions_load,globals::definitions);
    const auto tag_index=frame.tag_index(tag);
    frame.saving(1u);
    const auto tag_offset=frame.tag_offset(tag_index);
    frame.saving(2u);
    const auto records=memory.load32(output_argument,frame.local(local::records));
    const auto entry=frame.entry_address(definitions+tag_offset+definition_entry::address);
    frame.saving(3u);
    auto definition_address=memory.load32(definition_load,entry);
    memory.store32(entry_store,frame.local(local::entry),entry);
    const auto regions_total=memory.load32(region_count,at(definition_address,definition::region_count));
    kept.zero=frame.zeroed();
    std::uint32_t index=frame.first_cleared();
    const bool any=frame.above(regions_total,kept.zero);
    frame.saving(4u);
    memory.store32(definition_store,frame.local(local::definition),definition_address);
    if(any) {
        auto cursor=frame.record_cursor(records);
        for(;;) {
            const auto chosen=memory.load32(chosen_argument,frame.local(local::chosen));
            const auto none_value=frame.none_value();
            memory.store8(chosen_clear,chosen+index,none_value);
            memory.store8(output_marker_clear,cursor-2u+record::marker,none_value);
            memory.store8(output_first_clear,cursor-2u+record::first,0u);
            memory.store8(output_second_clear,cursor-2u+record::second,0u);
            memory.store32(output_link_clear,cursor-2u+record::link,none_value);
            const auto total=memory.load32(region_count_again,at(definition_address,definition::region_count));
            index=frame.next_cleared(index);
            cursor=frame.next_record(cursor);
            if(!frame.below(index,total))break;
        }
    }

    // Each region.
    const bool regions_present=frame.above(memory.load32(regions_test,at(definition_address,definition::region_count)),kept.zero);
    memory.store32(region_index_store,frame.local(local::region_index),kept.zero);
    if(!regions_present)return;
    kept.weight=memory.load32(weight_load,globals::weight);
    auto region_offset=frame.first_region();
    kept.region=frame.output_cursor(records);
    memory.store32(region_offset_store,frame.local(local::region_offset),region_offset);
    memory.store32(output_cursor_store,frame.local(local::output_cursor),kept.region);
    for(bool first=true;;first=false) {
        if(!first) {
            region_offset=memory.load32(region_offset_load,frame.local(local::region_offset));
            definition_address=memory.load32(definition_reload,frame.local(local::definition));
        }
        const auto regions_address=memory.load32(regions_load,at(definition_address,definition::regions));
        const auto variant_index=memory.load32(variant_argument,frame.local(local::variant));
        kept.region=frame.region_address(regions_address,region_offset);
        auto count=frame.count_cleared();
        const bool varied=frame.named(variant_index);
        memory.store32(region_store,frame.local(local::region),kept.region);
        memory.store32(count_store,frame.local(local::count),count);
        bool gathered=false;
        if(varied) {
            // The variant's candidates for the region.
            const auto variants=memory.load32(variants_load,at(definition_address,definition::variants));
            const auto variant_offset=frame.variant_offset(variant_index);
            const auto model=memory.load32(tag_test,frame.local(local::tag));
            const auto variant_address=frame.variant_address(variants,variant_offset);
            if(frame.named(model) && frame.region_listed(region_offset)) {
                const auto entry_now=memory.load32(entry_load,frame.local(local::entry));
                const auto definition_now=memory.load32(entry_definition,entry_now);
                const auto variants_now=memory.load32(entry_variants,at(definition_now,definition::variants));
                const auto variant_now=frame.variant_again(variants_now,variant_offset);
                const auto region_index=memory.load32(region_index_load,frame.local(local::region_index));
                const auto listed=signed_byte(memory.load8(variant_permutation,variant_now+region_index+variant::regions));
                if(frame.named(listed)) {
                    frame.gather_pushed(kept,listed);
                    const auto candidates=memory.load32(variant_candidates,at(variant_address,variant::candidates));
                    frame.gather(kept,candidates+variant::candidate_size*listed);
                    count=memory.load32(gathered_count,frame.local(local::count));
                    gathered=frame.gathered(count,kept.zero);
                }
            }
        }
        if(!gathered) {
            // The region's unflagged permutations, four at a time while four remain.
            const auto permutations_total=memory.load32(permutation_count,at(kept.region,region::permutation_count));
            auto permutation_index=frame.first_permutation();
            const bool four=frame.unrolled(permutations_total);
            memory.store32(permutation_store,frame.local(local::permutation),permutation_index);
            if(four) {
                const auto permutations=memory.load32(permutations_load,at(kept.region,region::permutations));
                const auto limit=frame.unrolled_limit(count,permutations_total);
                memory.store32(unrolled_limit_store,frame.local(local::unrolled_limit),limit);
                memory.store32(weight_cursor_store,frame.local(local::weight_cursor),frame.weight_cursor());
                // The cursor on the second permutation's flags of each pass.
                auto flags_cursor=frame.flags_cursor(permutations);
                memory.store32(permutation_cursor_store,frame.local(local::permutation_cursor),flags_cursor);
                auto candidate_address=frame.local(local::list)+candidate::size*count;
                for(bool pass_first=true;;pass_first=false) {
                    if(!pass_first)flags_cursor=memory.load32(permutation_cursor_load,frame.local(local::permutation_cursor));
                    for(std::uint32_t k=0;k<unrolled;++k) {
                        const auto &l=listings[k];
                        const auto flags=memory.load8(l.flags,flags_cursor-permutation::size+k*permutation::size);
                        if(frame.flagged(flags))continue;
                        const auto listed_index=frame.listed_index(memory.load32(l.index,frame.local(local::permutation)),k);
                        memory.store32(l.index_store,candidate_address+candidate::permutation,listed_index);
                        const auto weight_at=memory.load32(l.weight_cursor,frame.local(local::weight_cursor));
                        memory.store32(l.marker,candidate_address+candidate::marker,none);
                        memory.store32(l.first,candidate_address+candidate::first,0u);
                        memory.store32(l.second,candidate_address+candidate::second,0u);
                        memory.store32(l.link,candidate_address+candidate::link,none);
                        memory.store32(l.spare,candidate_address+candidate::spare,0u);
                        memory.store32(l.weight,weight_at,kept.weight);
                        memory.store32(l.count_store,frame.local(local::count),frame.incremented(memory.load32(l.count_load,frame.local(local::count))));
                        const auto weight_next=frame.listed(memory.load32(l.weight_cursor_again,frame.local(local::weight_cursor)));
                        memory.store32(l.weight_cursor_next,frame.local(local::weight_cursor),weight_next);
                        candidate_address+=candidate::size;
                        if(k+1u<unrolled)flags_cursor=memory.load32(l.permutation_cursor,frame.local(local::permutation_cursor));
                    }
                    memory.store32(permutation_index_store,frame.local(local::permutation),
                        frame.added(memory.load32(permutation_index_load,frame.local(local::permutation)),unrolled));
                    memory.store32(permutation_cursor_add_store,frame.local(local::permutation_cursor),
                        frame.added(memory.load32(permutation_cursor_add_load,frame.local(local::permutation_cursor)),unrolled*permutation::size));
                    const auto limit_now=memory.load32(unrolled_limit_load,frame.local(local::unrolled_limit));
                    if(!frame.below(memory.load32(permutation_index_test,frame.local(local::permutation)),limit_now))break;
                }
                permutation_index=memory.load32(permutation_index_after,frame.local(local::permutation));
                kept.region=memory.load32(region_reload,frame.local(local::region));
                count=memory.load32(count_after,frame.local(local::count));
                kept.zero=frame.zeroed();
            }
            // The rest, one at a time.
            const auto total=memory.load32(permutation_total,at(kept.region,region::permutation_count));
            if(frame.below(permutation_index,total)) {
                auto candidate_address=frame.rest_cursor(count);
                const auto permutations=memory.load32(permutations_rest,at(kept.region,region::permutations));
                auto flags_address=frame.rest_flags(permutations+permutation::size*permutation_index+permutation::flags);
                for(;;) {
                    if(!frame.flagged(memory.load8(flags_rest,flags_address))) {
                        memory.store32(index_rest,candidate_address+candidate::permutation,permutation_index);
                        memory.store32(marker_rest,candidate_address+candidate::marker,none);
                        memory.store32(first_rest,candidate_address+candidate::first,kept.zero);
                        memory.store32(second_rest,candidate_address+candidate::second,kept.zero);
                        memory.store32(link_rest,candidate_address+candidate::link,none);
                        memory.store32(spare_rest,candidate_address+candidate::spare,kept.zero);
                        memory.store32(weight_rest,candidate_address+candidate::weight,kept.weight);
                        count=frame.rest_listed(count);
                        candidate_address+=candidate::size;
                    }
                    permutation_index=frame.next_permutation(permutation_index);
                    flags_address+=permutation::size;
                    if(!frame.below(permutation_index,total))break;
                }
            }
            // A region with every permutation flagged lists its first.
            if(!frame.different(count,kept.zero) && frame.above(total,kept.zero)) {
                const auto none_value=frame.none_count();
                const auto list=frame.local(local::list);
                memory.store32(default_marker,list+candidate::marker,none_value);
                memory.store32(default_link,list+candidate::link,none_value);
                memory.store32(default_index,list+candidate::permutation,kept.zero);
                memory.store32(default_first,list+candidate::first,kept.zero);
                memory.store32(default_second,list+candidate::second,kept.zero);
                memory.store32(default_spare,list+candidate::spare,kept.zero);
                memory.store32(default_weight,list+candidate::weight,kept.weight);
                count=frame.one_listed();
            }
        }

        // The choice, unless the region is held.
        const auto region_index=memory.load32(mask_region,frame.local(local::region_index));
        if(!frame.held(memory.load32(mask_argument,frame.local(local::held)),region_index)) {
            const auto choice=frame.chosen(kept,count);
            if(frame.named(choice)) {
                const auto region_now=memory.load32(chosen_region,frame.local(local::region_index));
                const auto choice_offset=frame.choice_offset(choice);
                const auto chosen=memory.load32(chosen_argument_again,frame.local(local::chosen));
                const auto picked=frame.choice_address(choice_offset);
                memory.store8(chosen_store,region_now+chosen,memory.load8(chosen_permutation,at(picked,candidate::permutation)));
                const auto marker=memory.load8(chosen_marker,at(picked,candidate::marker));
                const auto output=memory.load32(output_cursor_load,frame.local(local::output_cursor));
                memory.store8(output_marker_store,output-1u,marker);
                const auto second=memory.load8(chosen_second,at(picked,candidate::second));
                const auto first_value=memory.load8(chosen_first,at(picked,candidate::first));
                memory.store8(output_second_store,output+1u,second);
                memory.store8(output_first_store,output,first_value);
            }
            kept.zero=frame.zeroed();
        }

        // The next region.
        const auto output=memory.load32(output_cursor_next,frame.local(local::output_cursor));
        const auto next_index=memory.load32(region_index_next,frame.local(local::region_index));
        const auto next_offset=memory.load32(region_offset_next,frame.local(local::region_offset));
        memory.store32(output_cursor_advance,frame.local(local::output_cursor),frame.next_output(output));
        const auto definition_now=memory.load32(definition_next,frame.local(local::definition));
        const auto regions_now=memory.load32(region_count_next,at(definition_now,definition::region_count));
        const auto index_after=frame.next_region(next_index);
        const auto offset_after=frame.next_offset(next_offset);
        const bool more=frame.below(index_after,regions_now);
        memory.store32(region_index_advance,frame.local(local::region_index),index_after);
        memory.store32(region_offset_advance,frame.local(local::region_offset),offset_after);
        if(!more)return;
    }
}
} // namespace permutation_choice
} // namespace halo2::native_permutation_choice
