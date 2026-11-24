#pragma once
// Native TU1.5 aim choice, entered at 1ADF60 (combat action unit): the
// target an aim chooses among the candidates its search finds.
//
//   aim_choice::choose   1ADF60(ECX ray; first, second, third, fourth, chosen) -> AL; RET 14
//
// The ray must lie in a leaf (14CDB0: ECX the structure query [4ECC0C],
// EDX the ray, EAX 0) whose cluster (the word at its 8-byte entry in the
// structure BSP's +0x30) is named. The aim search 1AE090 (the first
// argument, the cluster, the ray, the second, third and fourth arguments
// and the list pushed) lists up to its count of 0x60-byte candidates on
// the frame; qsort (32B560) orders them with 1AF2E0, the first argument in
// [52BE40]. The first candidate 1AF3E0 accepts (ECX the third argument, EBX
// the candidate's +8, EAX its +0) is copied whole into the chosen record,
// and the answer is 1; otherwise the answer stays 0.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_aim_choice.hpp) maps the role to the original instruction,
// owns the ABI of the children (the stack probe 32A660 among them) and
// leaves through the original's epilogue for each exit. The values the
// original keeps in callee-saved registers across the children are a Kept,
// refreshed from the registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_aim_choice {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t structure_query=0x4ecc0cu,structure_bsp=0x4ecc18u,sort_context=0x52be40u;}
inline constexpr std::uint32_t none=0xffffffffu,no_cluster=0xffffu,leaf_size=8u,candidate_size=0x60u,candidate_words=0x18u,
    frame_size=0x1808u,comparator=0x1af2e0u;
namespace bsp {inline constexpr std::uint32_t leaves=0x30u;}
// A candidate: the key 1AF3E0 takes in EAX, and the part it takes in EBX.
namespace candidate {inline constexpr std::uint32_t key=0u,part=8u;}
// The frame (offsets from the ESP after EBX, EBP, EDI and ESI are pushed):
// the answer byte, the cursor (on each candidate's +0x14), the list, and
// the five arguments.
namespace local {inline constexpr std::uint32_t answer=0x13u,cursor=0x14u,list=0x18u,first=0x181cu,second=0x1820u,third=0x1824u,
    fourth=0x1828u,chosen=0x182cu;}
inline constexpr std::uint32_t cursor_offset=0x14u;

namespace aim_choice {
enum class Access {first_argument,query_load,answer_clear,structure_load,leaves_load,leaf_cluster,fourth_argument,third_argument,
    second_argument,sort_context_store,cursor_store,cursor_load,third_again,candidate_key,cursor_next_load,cursor_next_store,
    answer_load,chosen_argument,copy_load,copy_store};
// The original's exits, each with its own epilogue: no leaf or cluster
// (before ESI is saved), no candidate, none accepted, and one chosen.
enum class Exit {early,unlisted,exhausted,chosen};
// The original's EBX (the answer in BL, later each candidate's part), EBP
// (the first argument, later the candidate counter), ESI (the count) and
// EDI (the ray).
struct Kept {std::uint32_t answer,base,count,ray;};

// Frame: see native_tu15_aim_choice.hpp.
template<class Memory,class Frame> Exit choose(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    // The ray's leaf and cluster.
    frame.probed();
    frame.saving(1u);
    frame.saving(2u);
    kept.base=memory.load32(first_argument,frame.local(local::first));
    frame.saving(3u);
    kept.ray=frame.ray();
    const auto query=memory.load32(query_load,globals::structure_query);
    kept.answer=frame.unanswered();
    frame.leaf_arguments(kept.ray);
    memory.store8(answer_clear,frame.local(local::answer),kept.answer);
    const auto leaf=frame.leaf(kept,query);
    if(!frame.named(leaf))return Exit::early;
    const auto structure=memory.load32(structure_load,globals::structure_bsp);
    const auto leaves=memory.load32(leaves_load,at(structure,bsp::leaves));
    if(!frame.cluster_named(memory.load16(leaf_cluster,leaves+leaf_size*leaf)))return Exit::early;

    // The candidates, searched and sorted.
    const auto fourth=memory.load32(fourth_argument,frame.local(local::fourth));
    frame.saving(4u);
    frame.list_pushed();
    const auto third=memory.load32(third_argument,frame.local(local::third));
    frame.fourth_pushed(fourth);
    const auto second=memory.load32(second_argument,frame.local(local::second));
    kept.count=frame.searched(kept,third,second);
    if(!frame.positive(kept.count))return Exit::unlisted;
    frame.sort_pushed(kept.count);
    memory.store32(sort_context_store,globals::sort_context,kept.base);
    frame.sorted(kept);
    kept.base=frame.first_candidate();
    if(!frame.positive(kept.count))return Exit::unlisted;

    // The first candidate 1AF3E0 accepts.
    auto cursor=frame.cursor_start();
    memory.store32(cursor_store,frame.local(local::cursor),cursor);
    for(bool first=true;;first=false) {
        if(!first)cursor=memory.load32(cursor_load,frame.local(local::cursor));
        const auto third_now=memory.load32(third_again,frame.local(local::third));
        const auto candidate_address=cursor-cursor_offset;
        kept.answer=frame.candidate_part(candidate_address+candidate::part);
        const auto key=memory.load32(candidate_key,at(candidate_address,candidate::key));
        if(frame.accepted(kept,third_now,key)) {
            const auto chosen=memory.load32(chosen_argument,frame.local(local::chosen));
            auto from=frame.copying(kept.base,chosen);
            auto to=chosen;
            const auto step=frame.step();
            for(std::uint32_t word=0;word<candidate_words;++word,from+=step,to+=step)
                memory.store32(copy_store,to,memory.load32(copy_load,from));
            return Exit::chosen;
        }
        const auto cursor_now=memory.load32(cursor_next_load,frame.local(local::cursor));
        kept.base=frame.next_candidate(kept.base);
        const auto cursor_next=frame.next_cursor(cursor_now);
        const bool more=frame.below(kept.base,kept.count);
        memory.store32(cursor_next_store,frame.local(local::cursor),cursor_next);
        if(!more) {
            memory.load8(answer_load,frame.local(local::answer));
            return Exit::exhausted;
        }
    }
}
} // namespace aim_choice
} // namespace halo2::native_aim_choice
