#pragma once
#include <bit>
#include <cstdint>

namespace halo2 {
enum class NativeTu15DatumDeletePhase {rows,stride,debug_flag,clear_begin,clear_word,clear_tail,clear_byte,
    clear_end,bitmap,bitmap_word,bitmap_store,salt,cursor,cursor_store,extent,shrink_stride,shrink_extent,
    shrink_store,shrink_salt,occupancy,finish,occupancy_store,finished};
struct NativeTu15DatumDelete {
    std::uint32_t pool{},datum{},rows{},stride{},index{},record{},debug_flag{},clear_at{},word_count{},tail_count{},
        bitmap{},word_index{},bit{},mask{},old_word{},new_word{},cursor{},extent{},next_index{},
        shrink_stride{},moving_record{},shrink_extent{},new_extent{},previous_salt{},occupancy{},next_occupancy{};
};
// Shared16E8A0 DATA algorithm. Callers capture the actual pool/datum and
// provide their storage/ABI boundary adapter; no register bank is interpreted.
template<class Access>void native_tu15_delete_datum(Access &memory,NativeTu15DatumDelete &s,bool backwards=false){
    using P=NativeTu15DatumDeletePhase;
    const auto signed_word=[](auto value){return std::bit_cast<std::int32_t>(value);};
    const auto publish=[&](P phase,std::uint32_t at,std::uint32_t value,unsigned width=4){memory.write(phase,s,at,value,width);memory.complete(phase,s);};
    s.index=s.datum&0xffffu;
    s.rows=memory.read(P::rows,s,s.pool+0x44u,4);s.stride=memory.read(P::stride,s,s.pool+0x24u,4);
    s.record=s.rows+s.index*s.stride;s.debug_flag=memory.read(P::debug_flag,s,s.pool+0x2au,1);
    if(s.debug_flag&8u){
        s.clear_at=s.record;s.word_count=s.stride>>2u;s.tail_count=s.stride&3u;memory.prepare(P::clear_begin,s);
        while(s.word_count){memory.write(P::clear_word,s,s.clear_at,0xbabababau,4);s.clear_at+=backwards?0xfffffffcu:4u;--s.word_count;memory.complete(P::clear_word,s);}
        memory.prepare(P::clear_tail,s);
        while(s.tail_count){memory.write(P::clear_byte,s,s.clear_at,0xbau,1);s.clear_at+=backwards?0xffffffffu:1u;--s.tail_count;memory.complete(P::clear_byte,s);}
        memory.prepare(P::clear_end,s);
    }
    s.bitmap=memory.read(P::bitmap,s,s.pool+0x48u,4);s.word_index=s.index>>5u;s.bit=1u<<(s.index&31u);s.mask=~s.bit;
    s.old_word=memory.read(P::bitmap_word,s,s.bitmap+s.word_index*4u,4);s.new_word=s.old_word&s.mask;
    publish(P::bitmap_store,s.bitmap+s.word_index*4u,s.new_word);publish(P::salt,s.record,0,2);
    s.cursor=memory.read(P::cursor,s,s.pool+0x34u,4);
    if(signed_word(s.index)<signed_word(s.cursor))publish(P::cursor_store,s.pool+0x34u,s.index);
    s.extent=memory.read(P::extent,s,s.pool+0x38u,4);s.next_index=s.index+1u;
    if(s.next_index==s.extent){
        s.shrink_stride=memory.read(P::shrink_stride,s,s.pool+0x24u,4);s.moving_record=s.record;
        for(;;){
            s.shrink_extent=memory.read(P::shrink_extent,s,s.pool+0x38u,4);s.moving_record-=s.shrink_stride;s.new_extent=s.shrink_extent-1u;
            publish(P::shrink_store,s.pool+0x38u,s.new_extent);
            if(signed_word(s.new_extent)<=0)break;
            s.previous_salt=memory.read(P::shrink_salt,s,s.moving_record,2);if(s.previous_salt)break;
        }
    }
    s.occupancy=memory.read(P::occupancy,s,s.pool+0x3cu,4);s.next_occupancy=s.occupancy-1u;memory.prepare(P::finish,s);
    publish(P::occupancy_store,s.pool+0x3cu,s.next_occupancy);memory.complete(P::finished,s);
}
} // namespace halo2
