#pragma once
// Native TU1.5 sound readiness, entered at 1718C0 (physics live support
// unit): whether every chunk of a sound's permutations is ready in the
// sound cache.
//
//   sound_readiness::ready   1718C0(sound) -> AL, RET 4
//
// The sound's definition (through the tag instances at 4F0414) has its
// permutation count (int8 +0xA) and its first permutation (int16 +8) among
// the sound globals' 12-byte permutations (+0x24 of [52D664]); each
// permutation its first chunk (int16 +8) and chunk count (int16 +0xA) among
// the 16-byte chunks (+0x2C), each chunk its sample (int16 +0xC) among the
// 12-byte samples (+0x44). Every chunk, in order, is requested of the sound
// cache (21F260: the sound, the sample, mode 8); the sound is ready while
// every answer carries bit 1. A sound without permutations is ready.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the original's frame (the definition,
// the counts) and the argument slot it reuses as the permutation's index;
// the boundary adapter (native_tu15_sound_readiness.hpp) maps the role to the
// original instruction and owns the ABI of the child (21F260). The values the
// original keeps in callee-saved registers across it are a Kept, refreshed
// from the registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_sound_readiness {
using native_object_update::at;
using native_object_update::signed_value;
using native_object_update::widen16;
using native_object_update::widen8;
namespace globals {inline constexpr std::uint32_t tag_instances=0x4f0414u,sound_globals=0x52d664u;}
inline constexpr std::uint32_t tag_size=16u,tag_data=8u,permutation_size=12u,chunk_size=16u,sample_size=12u,request_mode=8u,
    ready_bit=2u;
namespace definition {inline constexpr std::uint32_t first_permutation=8u,permutations=0xau;}
namespace sound_globals {inline constexpr std::uint32_t permutations=0x24u,chunks=0x2cu,samples=0x44u;}
namespace permutation {inline constexpr std::uint32_t first_chunk=8u,chunks=0xau;}
namespace chunk {inline constexpr std::uint32_t sample=0xcu;}
// The frame's locals, by offset from the return address.
namespace local {inline constexpr std::int32_t definition=-0xc,chunks=-8,permutations=-4;}

namespace sound_readiness {
enum class Access {instances,sound_argument,definition_address,permutation_count,definition_store,index_clear,count_store,
    sound_globals,definition_load,first_permutation,permutation_array,chunk_count,chunks_store,first_chunk,chunk_array,
    chunk_sample,sample_array,chunks_load,globals_again,index_load,count_load,index_store};
enum class Outcome {no_permutations,ready,not_ready};
// The original's BL (the answer), EBP (the sound), ESI (the chunk) and EDI
// (the permutation) across 21F260.
struct Kept {std::uint32_t ready,sound,chunk,permutation;};
// Frame:
//   argument(), local(offset)              the argument's slot and the frame's locals now
//   saving()                               ESI and EDI saved before the walk
//   request(sound, sample, mode, kept) -> AL   21F260(the sound, the sample, the mode pushed), refreshes kept
//   ready_set(kept)                        the answer's register set (MOV BL,1)
//   answered(kept)                         the answer's register after a request (TEST BL; TEST AL,2; MOV or XOR BL)
//   chunk_advanced(kept, count), index_advanced(index, total)   the loops' INC and CMP
template<class Memory,class Frame> Outcome ready(Memory &memory,Frame &frame) {
    using enum Access;
    const auto instances_address=memory.load32(instances,globals::tag_instances);
    Kept kept{};
    kept.sound=memory.load32(sound_argument,frame.argument());
    const auto definition_value=memory.load32(definition_address,tag_size*(kept.sound&0xffffu)+instances_address+tag_data);
    const auto permutations=widen8(memory.load8(permutation_count,at(definition_value,definition::permutations)));
    memory.store32(definition_store,frame.local(local::definition),definition_value);
    kept.ready=1u;
    frame.ready_set(kept);
    memory.store32(index_clear,frame.argument(),0u);
    memory.store32(count_store,frame.local(local::permutations),permutations);
    if(signed_value(permutations)<=0)return Outcome::no_permutations;
    auto globals_address=memory.load32(sound_globals,globals::sound_globals);
    frame.saving();
    std::uint32_t index=0u;
    for(bool first=true;;first=false) {
        const auto definition_now=first?definition_value:memory.load32(definition_load,frame.local(local::definition));
        const auto permutation_index=widen16(memory.load16(first_permutation,at(definition_now,definition::first_permutation)))+index;
        kept.permutation=memory.load32(permutation_array,at(globals_address,sound_globals::permutations))+permutation_size*permutation_index;
        const auto chunks=widen16(memory.load16(chunk_count,at(kept.permutation,permutation::chunks)));
        memory.store32(chunks_store,frame.local(local::chunks),chunks);
        if(signed_value(chunks)>0) {
            kept.chunk=0u;
            std::uint32_t chunks_now;
            do {
                const auto chunk_index=widen16(memory.load16(first_chunk,at(kept.permutation,permutation::first_chunk)))+kept.chunk;
                const auto chunks_address=memory.load32(chunk_array,at(globals_address,sound_globals::chunks));
                const auto sample=widen16(memory.load16(chunk_sample,chunks_address+chunk_size*chunk_index+chunk::sample));
                const auto samples=memory.load32(sample_array,at(globals_address,sound_globals::samples));
                const auto answer=frame.request(kept.sound,samples+sample_size*sample,request_mode,kept);
                kept.ready=((kept.ready&0xffu) && (answer&ready_bit))?1u:0u;
                frame.answered(kept);
                chunks_now=memory.load32(chunks_load,frame.local(local::chunks));
                globals_address=memory.load32(globals_again,globals::sound_globals);
                ++kept.chunk;
                frame.chunk_advanced(kept,chunks_now);
            } while(signed_value(kept.chunk)<signed_value(chunks_now));
            index=memory.load32(index_load,frame.argument());
        }
        const auto total=memory.load32(count_load,frame.local(local::permutations));
        ++index;
        frame.index_advanced(index,total);
        memory.store32(index_store,frame.argument(),index);
        if(!(signed_value(index)<signed_value(total)))break;
    }
    return kept.ready&0xffu?Outcome::ready:Outcome::not_ready;
}
} // namespace sound_readiness
} // namespace halo2::native_sound_readiness
