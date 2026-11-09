#pragma once
#include "halo2/native_tu15_datum_initialize_kernel.hpp"
#include <bit>
#include <cstdint>

namespace halo2 {
enum class NativeTu15DatumAllocatePhase {
    scan_bitmap,scan_word,scan_extent,capacity,stride,rows,publish_bitmap,occupancy_word,occupancy,
    used,extent,used_count,cursor,grown_extent,clear_stride,clear_word,clear_byte,
    salt,salt_record,salt_increment,salt_check,salt_reset,record_salt,finished
};
enum class NativeTu15DatumSelection {initial,scanning,hole,invalid_hole,exhausted};
enum class NativeTu15DatumFailure {none,capacity,sentinel};
struct NativeTu15DatumAllocation:NativeTu15DatumRecordInitialization {
    std::uint32_t cursor{},extent{};
    NativeTu15DatumSelection selection{NativeTu15DatumSelection::initial};
    NativeTu15DatumFailure failure{};
    bool success{},selected{};
    std::uint32_t bitmap{},word_index{},bit{},word{},scan_extent{},capacity{},index{};
    std::uint32_t stride{},rows{},product{},publish_bitmap{},occupied{},old_word{},new_word{};
    std::uint32_t used{},next_used{},current_extent{},next_index{};
    std::uint32_t record_salt{},handle{};
};
// Whole16E6A0 DATA algorithm. Storage boundaries carry typed continuations;
// the separate ABI adapter owns registers, flags, source PCs and callbacks.
// Access::write commits bytes; complete observes only after this instruction's
// numerical counters have advanced (notably repeated DWORD/byte clears).
template<class Access>void native_tu15_allocate_datum(Access&memory,NativeTu15DatumAllocation&s,bool backwards=false) {
    using P=NativeTu15DatumAllocatePhase;using S=NativeTu15DatumSelection;
    const auto signed_word=[](std::uint32_t value){return std::bit_cast<std::int32_t>(value);};
    const auto publish=[&](P phase,std::uint32_t at,std::uint32_t value,unsigned bytes=4){
        memory.write(phase,s,at,value,bytes,0,false);memory.complete(phase,s);
    };
    if(signed_word(s.cursor)<signed_word(s.extent)) {
        s.bitmap=memory.read(P::scan_bitmap,s,s.pool+0x48u,4);s.selection=S::scanning;
        for(;;) {
            s.word_index=std::uint32_t(signed_word(s.cursor)>>5u);s.bit=1u<<(s.cursor&31u);
            s.word=memory.read(P::scan_word,s,s.bitmap+s.word_index*4u,4);
            if(!(s.word&s.bit)) {
                s.index=s.cursor;s.selection=s.cursor==0xffffffffu?S::invalid_hole:S::hole;break;
            }
            s.scan_extent=memory.read(P::scan_extent,s,s.pool+0x38u,4);++s.cursor;
            if(signed_word(s.cursor)>=signed_word(s.scan_extent)){s.selection=S::exhausted;break;}
        }
    }
    if(s.selection!=S::hole) {
        s.capacity=memory.read(P::capacity,s,s.pool+0x20u,4);
        if(signed_word(s.extent)>=signed_word(s.capacity)){s.failure=NativeTu15DatumFailure::capacity;return;}
        s.index=s.extent;
        if(s.index==0xffffffffu){s.failure=NativeTu15DatumFailure::sentinel;return;}
    }
    s.selected=true;
    s.stride=memory.read(P::stride,s,s.pool+0x24u,4);s.rows=memory.read(P::rows,s,s.pool+0x44u,4);
    s.product=s.stride*s.index;
    s.publish_bitmap=memory.read(P::publish_bitmap,s,s.pool+0x48u,4);s.record=s.rows+s.product;
    s.occupied=s.publish_bitmap+std::uint32_t(signed_word(s.index)>>5u)*4u;
    s.old_word=memory.read(P::occupancy_word,s,s.occupied,4);s.new_word=s.old_word|(1u<<(s.index&31u));
    publish(P::occupancy,s.occupied,s.new_word);
    s.used=memory.read(P::used,s,s.pool+0x3cu,4);s.current_extent=memory.read(P::extent,s,s.pool+0x38u,4);
    s.next_used=s.used+1u;s.next_index=s.index+1u;
    const bool grow=signed_word(s.current_extent)<=signed_word(s.index);
    publish(P::used_count,s.pool+0x3cu,s.next_used);publish(P::cursor,s.pool+0x34u,s.next_index);
    if(grow)publish(P::grown_extent,s.pool+0x38u,s.next_index);
    native_tu15_initialize_datum_record<P>(memory,s,backwards);
    s.record_salt=memory.read(P::record_salt,s,s.record,2);s.handle=(s.record_salt<<16u)|s.index;s.success=true;
}
} // namespace halo2
