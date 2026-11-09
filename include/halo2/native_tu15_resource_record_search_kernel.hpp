#pragma once
#include "halo2/native_tu15_bitmap_scan_kernel.hpp"

namespace halo2 {
enum class NativeTu15ResourceSearchExit {outer_bound,inner_bound,found,negative_index};
struct NativeTu15ResourceSearch {
    std::uint32_t datum{0xffffffffu},index{},escaped{0xffffffffu},limit{},last_owner{};
    NativeTu15ResourceSearchExit exit{NativeTu15ResourceSearchExit::outer_bound};
};
template<class Memory>inline std::uint32_t native_resource_search_value(const Memory&memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8*n);return value;
}
// Find the occupied resource record whose owner is the requested entry. The
// pool can change during reads: the empty-slot loop retains its bitmap base,
// but obtains a fresh bound before advancing; a nonmatching occupied record
// starts a new scan and captures both descriptor fields again.
template<class Memory>inline NativeTu15ResourceSearch native_tu15_find_resource_record(
    const Memory&memory,std::uint32_t pool,std::uint32_t entry) {
    NativeTu15ResourceSearch result;
    for(;;) {
        if(std::bit_cast<std::int32_t>(result.index)<0){result.exit=NativeTu15ResourceSearchExit::negative_index;return result;}
        result.limit=native_resource_search_value(memory,pool+0x38u);
        if(std::bit_cast<std::int32_t>(result.index)>=std::bit_cast<std::int32_t>(result.limit))return result;
        const auto bitmap=native_resource_search_value(memory,pool+0x48u);
        while(!native_tu15_scan_bitmap_slots(memory,bitmap,result.index,result.index+1u).found) {
            result.limit=native_resource_search_value(memory,pool+0x38u);++result.index;
            if(std::bit_cast<std::int32_t>(result.index)>=std::bit_cast<std::int32_t>(result.limit)) {
                result.escaped=result.limit;result.exit=NativeTu15ResourceSearchExit::inner_bound;return result;
            }
        }
        const auto stride=native_resource_search_value(memory,pool+0x24u);
        const auto records=native_resource_search_value(memory,pool+0x44u);
        const auto record=records+stride*result.index;
        result.last_owner=native_resource_search_value(memory,record+8u);
        result.escaped=record;
        const auto generation=native_resource_search_value(memory,record,2);
        if(result.last_owner==entry) {
            result.datum=(generation<<16u)|result.index;result.exit=NativeTu15ResourceSearchExit::found;return result;
        }
        ++result.index;
    }
}
} // namespace halo2
