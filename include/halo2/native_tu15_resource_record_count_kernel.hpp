#pragma once
#include "halo2/native_tu15_bitmap_scan_kernel.hpp"

namespace halo2 {
enum class NativeTu15ResourceCountExit {bound,null_record,negative_index};
struct NativeTu15ResourceCount {
    std::uint32_t count{},index{},escaped{},limit{},offset{},records{};
    NativeTu15ResourceCountExit exit{NativeTu15ResourceCountExit::bound};
};
template<class Memory>inline std::uint32_t native_resource_count_value(const Memory&memory,std::uint32_t at,unsigned width=4) {
    std::array<std::byte,4>bytes{};memory.read(at,std::span(bytes).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8*n);return value;
}
// Count occupied records with a zero activity byte. Each occupied candidate
// starts another descriptor scan; empty candidates share that scan's bound
// and bitmap base. A wrapped null record address terminates before its byte
// is read, as in the original pool traversal.
template<class Memory>inline NativeTu15ResourceCount native_tu15_count_inactive_resource_records(
    const Memory&memory,std::uint32_t pool,std::uint32_t incoming_ecx) {
    NativeTu15ResourceCount result;result.escaped=incoming_ecx;
    for(;;) {
        if(std::bit_cast<std::int32_t>(result.index)<0){result.exit=NativeTu15ResourceCountExit::negative_index;return result;}
        result.limit=native_resource_count_value(memory,pool+0x38u);
        if(std::bit_cast<std::int32_t>(result.index)>=std::bit_cast<std::int32_t>(result.limit))return result;
        const auto bitmap=native_resource_count_value(memory,pool+0x48u);
        const auto slot=native_tu15_scan_bitmap_slots(memory,bitmap,result.index,result.limit);
        result.index=slot.index;if(slot.probed)result.escaped=slot.word_index;
        if(!slot.found)return result;
        const auto stride=native_resource_count_value(memory,pool+0x24u);
        result.records=native_resource_count_value(memory,pool+0x44u);
        result.offset=stride*result.index;result.escaped=result.records+result.offset;
        if(!result.escaped){result.exit=NativeTu15ResourceCountExit::null_record;return result;}
        if(native_resource_count_value(memory,result.escaped+4u,1)==0)++result.count;
        ++result.index;
    }
}
} // namespace halo2
