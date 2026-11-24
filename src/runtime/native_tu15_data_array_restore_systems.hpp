#pragma once
// Native TU1.5 data array restore, entered at 16E510 (campaign runtime unit):
// a data array's bookkeeping rebuilt over the elements it holds.
//
//   data_array_restore::restore   16E510(ESI = array, EAX = count, ECX = data)
//
// The array loses its flag bit 1 (+0x2A), takes its count (+0x20, and its
// first free +0x34) and data (+0x44), is marked valid (+0x29) with no used
// elements (+0x38 the last used plus one, +0x3C the used count), and its
// signature (+0x40) is the first two bytes of its name (strncpy through the
// dispatch) with bit 15 set. Then each element (the size +0x24 apart), used
// when its first word is not 0: a used element sets its bit in the bitmap
// (+0x48), moves the last used past it and counts; an unused one clears its
// bit (in fill mode, flag bit 3, after its bytes are filled with BA, REP
// STOSD then REP STOSB in the string direction flag's direction, and its
// first word zeroed) and lowers the first free to it.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_data_array_restore.hpp) maps the role to the original
// instruction and owns the ABI of strncpy. The values the original keeps in
// callee-saved registers across it are a Kept, refreshed from the registers
// it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_data_array_restore {
using native_object_update::at;
namespace array {inline constexpr std::uint32_t count=0x20u,size=0x24u,valid=0x29u,flags=0x2au,first_free=0x34u,last_used=0x38u,
    used=0x3cu,signature=0x40u,data=0x44u,bitmap=0x48u;inline constexpr std::uint32_t cleared_flag=0xfdu,fill_bit=3u;}
inline constexpr std::uint32_t signature_bytes=2u,signature_bit=0x8000u,fill=0xbabababau;

namespace data_array_restore {
enum class Access {flags,flags_store,count_store,data_store,valid_store,last_used_store,used_store,first_free_store,count_load,
    signature_load,signature_store,size,data,in_use,mode,fill_word,fill_byte,unused_store,bitmap,bit_load,bit_store,first_free,
    first_free_lower,count_again,last_used_set,used_load,used_count};
enum class Outcome {empty,restored};
// The original's EBP (the element's index), ESI (the array) and EDI (the signature) across strncpy.
struct Kept {std::uint32_t index,array,signature;};
// Frame: saving(n), clearing(), signed_name(kept) [strncpy], counted(count, index), element(size, index, data),
// tested(...), filling(size, element), step(), bytes(size), bit(...), lowered(...), advanced(index, count), restoring(n).
template<class Memory,class Frame> Outcome restore(Memory &memory,Frame &frame,Kept kept,std::uint32_t count_value,std::uint32_t data_address) {
    using enum Access;
    memory.store8(flags_store,at(kept.array,array::flags),frame.anded(memory.load8(Access::flags,at(kept.array,array::flags)),array::cleared_flag));
    kept.index=0u;
    kept.signature=kept.array+array::signature;
    frame.saving(0u,kept);
    memory.store32(count_store,at(kept.array,array::count),count_value);
    memory.store32(data_store,at(kept.array,array::data),data_address);
    memory.store8(valid_store,at(kept.array,array::valid),1u);
    memory.store32(last_used_store,at(kept.array,array::last_used),0u);
    memory.store32(used_store,at(kept.array,array::used),0u);
    memory.store32(first_free_store,at(kept.array,array::first_free),count_value);
    frame.signed_name(kept);
    const auto count_now=memory.load32(count_load,at(kept.array,array::count));
    memory.store16(signature_store,kept.signature,frame.ored(memory.load16(signature_load,kept.signature),signature_bit));
    auto index=kept.index;
    frame.counted(count_now,index);
    if(std::int32_t(count_now)<=std::int32_t(index)){frame.restoring(0u);return Outcome::empty;}
    frame.saving(1u,kept);
    for(std::uint32_t limit=count_now;;) {
        const auto size_value=memory.load32(size,at(kept.array,array::size));
        const auto data_value=memory.load32(Access::data,at(kept.array,array::data));
        const auto element=frame.element(size_value,index,data_value);
        const auto first_word=memory.load16(in_use,element);
        const auto mode_value=memory.load8(mode,at(kept.array,array::flags));
        const bool used=frame.tested(first_word,mode_value);
        const bool fill_mode=(mode_value>>array::fill_bit)&1u;
        if(used) {
            const auto bitmap_value=memory.load32(bitmap,at(kept.array,array::bitmap));
            const auto word=frame.bit_word(bitmap_value,index,true);
            const auto bits=memory.load32(bit_load,word);
            memory.store32(bit_store,word,frame.bit_set(bits,index,used));
            memory.store32(last_used_set,at(kept.array,array::last_used),index+1u);
            memory.store32(used_count,at(kept.array,array::used),frame.incremented(memory.load32(used_load,at(kept.array,array::used))));
        } else {
            if(fill_mode) {
                auto to=frame.filling(size_value,element);
                const auto step=frame.step();
                for(std::uint32_t w=0;w<(size_value>>2);++w,to+=4u*step)memory.store32(fill_word,to,fill);
                frame.bytes(size_value);
                for(std::uint32_t b=0;b<(size_value&3u);++b,to+=step)memory.store8(fill_byte,to,fill&0xffu);
                memory.store16(unused_store,element,0u);
            }
            const auto bitmap_value=memory.load32(bitmap,at(kept.array,array::bitmap));
            const auto word=frame.bit_word(bitmap_value,index,false);
            const auto bits=memory.load32(bit_load,word);
            memory.store32(bit_store,word,frame.bit_cleared(bits,index));
            const auto first=memory.load32(first_free,at(kept.array,array::first_free));
            frame.lowered(index,first);
            if(std::int32_t(index)<std::int32_t(first))memory.store32(first_free_lower,at(kept.array,array::first_free),index);
        }
        limit=memory.load32(count_again,at(kept.array,array::count));
        ++index;
        frame.advanced(index,limit);
        if(!(std::int32_t(index)<std::int32_t(limit)))break;
    }
    frame.restoring(1u);
    return Outcome::restored;
}
} // namespace data_array_restore
} // namespace halo2::native_data_array_restore
