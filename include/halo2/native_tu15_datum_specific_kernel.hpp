#pragma once
#include "halo2/native_tu15_datum_initialize_kernel.hpp"
#include <bit>

namespace halo2 {
enum class NativeTu15DatumSpecificPhase {capacity,rows,stride,record_tag,bitmap,occupancy_word,occupancy,used,extent,used_count,grown_extent,requested_salt};
enum class NativeTu15DatumSpecificFailure {none,negative,bounds,occupied};
struct NativeTu15DatumSpecificResume {std::uint32_t pool{},record{},index{},generation{};};
struct NativeTu15DatumSpecific:NativeTu15DatumRecordInitialization {
    std::uint32_t requested{},index{},generation{},capacity{},rows{},stride{},product{},tag{};
    std::uint32_t bitmap{},occupied_word{},mask{},old_word{},new_word{},used{},extent{},next_used{},next_index{};
    std::uint32_t issued_generation{},handle{};
    bool grow{},success{};NativeTu15DatumSpecificFailure failure{};
};
// Actual16E770's requested-slot DATA policy. The initializer is a genuine
// continuation: its restored frame may change the pool, record, index or
// requested generation, so none of those values is guessed after the call.
template<class Access,class Initialize>
void native_tu15_allocate_specific_datum(Access&memory,NativeTu15DatumSpecific&s,Initialize initialize){
    using P=NativeTu15DatumSpecificPhase;
    const auto signed_word=[](std::uint32_t value){return std::bit_cast<std::int32_t>(value);};
    const auto publish=[&](P phase,std::uint32_t address,std::uint32_t value,unsigned width=4){
        memory.write(phase,s,address,value,width);memory.complete(phase,s);
    };
    s.index=s.requested&0xffffu;s.generation=std::uint32_t(signed_word(s.requested)>>16u);
    if(signed_word(s.index)<0){s.failure=NativeTu15DatumSpecificFailure::negative;memory.finished(s);return;}
    s.capacity=memory.read(P::capacity,s,s.pool+0x20u,4);
    if(signed_word(s.index)>=signed_word(s.capacity)){s.failure=NativeTu15DatumSpecificFailure::bounds;memory.finished(s);return;}
    s.rows=memory.read(P::rows,s,s.pool+0x44u,4);s.stride=memory.read(P::stride,s,s.pool+0x24u,4);
    s.product=s.stride*s.index;s.record=s.rows+s.product;s.tag=memory.read(P::record_tag,s,s.record,2);
    if(s.tag){s.failure=NativeTu15DatumSpecificFailure::occupied;memory.finished(s);return;}
    s.bitmap=memory.read(P::bitmap,s,s.pool+0x48u,4);s.occupied_word=s.bitmap+(s.index>>5u)*4u;s.mask=1u<<(s.index&31u);
    s.old_word=memory.read(P::occupancy_word,s,s.occupied_word,4);s.new_word=s.old_word|s.mask;
    publish(P::occupancy,s.occupied_word,s.new_word);
    s.used=memory.read(P::used,s,s.pool+0x3cu,4);s.extent=memory.read(P::extent,s,s.pool+0x38u,4);
    s.next_used=s.used+1u;s.next_index=s.index+1u;s.grow=signed_word(s.index)>=signed_word(s.extent);
    publish(P::used_count,s.pool+0x3cu,s.next_used);
    if(s.grow)publish(P::grown_extent,s.pool+0x38u,s.next_index);
    const auto resumed=initialize(s);s.pool=resumed.pool;s.record=resumed.record;s.index=resumed.index;s.generation=resumed.generation;
    s.issued_generation=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(s.generation))));
    publish(P::requested_salt,s.record,s.generation,2);
    s.handle=(s.issued_generation<<16u)|s.index;s.success=true;memory.finished(s);
}
} // namespace halo2
