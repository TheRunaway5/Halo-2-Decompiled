#pragma once
#include "halo2/native_tu15_retained_store_observation.hpp"
#include <bit>

namespace halo2 {
// Ordinary16-byte iterator DATA. Byte5 is padding and remains untouched.
struct NativeTu15ObjectScanState {std::uint32_t identity{};};
// Capture at the caller's own storage boundaries. The numerical/DATA loop
// accepts no GP image, flags object, executable dispatcher or instruction hook.
struct NativeTu15ObjectScanInput {
    NativeTu15ObjectScanState scan;
    std::uint32_t rows{},cursor{},limit{};
};
enum class NativeTu15ObjectScanExit {initial_bound,exhausted,matched};
enum class NativeTu15ObjectScanStores {retained_data,source_abi};
struct NativeTu15ObjectScanResult {
    std::uint32_t object{},index{},datum{},kind{},selected_bit{};
    NativeTu15ObjectScanExit exit{NativeTu15ObjectScanExit::initial_bound};
};
inline NativeTu15ObjectScanResult native_tu15_scan_objects(SourcePhysicsContactMemory&memory,
    NativeTu15ObjectScanInput input,const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15ObjectScanStores stores=NativeTu15ObjectScanStores::retained_data) {
    const auto word=[&](std::uint32_t at,unsigned count=4) {
        std::array<std::byte,4>bytes{};memory.read(at,std::span(bytes).first(count));std::uint32_t value{};
        for(unsigned n=0;n<count;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);return value;
    };
    const auto publish=[&](std::uint32_t pc,std::uint32_t at,std::uint32_t value,unsigned count) {
        // Source-ABI entries retain all preimages/events, even when the caller
        // memory also supports bounded private association-DATA suppression.
        const bool observed=stores==NativeTu15ObjectScanStores::source_abi?bool(observe):
            native_tu15_observes_retained_store(memory,observe,pc,at,count);
        SourcePhysicsContactRecordStore event{pc,at,{},{}};
        if(observed){event.before.resize(count);memory.read(at,event.before);}
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<count;++n)bytes[n]=std::byte(value>>(8u*n));
        if(observed)event.written.assign(bytes.begin(),bytes.begin()+count);
        memory.write(at,std::span(bytes).first(count));if(observed)observe(event);
    };
    const auto signed_word=[](std::uint32_t value){return std::bit_cast<std::int32_t>(value);};
    const auto extend_half=[](std::uint32_t value){return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));};
    NativeTu15ObjectScanResult result;const auto p=input.scan.identity,limit=input.limit;auto cursor=input.cursor;
    auto index=extend_half(cursor),row=input.rows+index*12u;result.index=index;
    if(signed_word(index)>=signed_word(limit)){publish(0xbcf5bu,p+6u,cursor,2);return result;}
    do {
        const auto generation=word(row,2),datum=(extend_half(generation)<<16u)|index;result.datum=datum;
        cursor=(cursor+1u)&0xffffu;
        if(generation) {
            const auto requested=word(p+4u,1),flags=word(row+2u,1);
            if((flags&requested)==requested) {
                const auto kind=word(row+3u,1),bit=1u<<(kind&31u),selected=word(p)&bit;
                if(selected) {
                    publish(0xbcf4au,p+8u,datum,4);result.object=word(row+8u);
                    publish(0xbcf50u,p+6u,cursor,2);
                    result.index=index;result.kind=kind;result.selected_bit=selected;
                    result.exit=NativeTu15ObjectScanExit::matched;return result;
                }
            }
        }
        index=extend_half(cursor);row+=12u;
    }while(signed_word(index)<signed_word(limit));
    publish(0xbcf3du,p+6u,cursor,2);result.index=index;
    result.exit=NativeTu15ObjectScanExit::exhausted;return result;
}
} // namespace halo2
