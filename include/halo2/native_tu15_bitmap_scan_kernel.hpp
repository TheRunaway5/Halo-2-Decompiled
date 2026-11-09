#pragma once
#include "halo2/physics_contact_record.hpp"
#include <bit>

namespace halo2 {
struct NativeTu15BitmapScan {
    std::uint32_t index{},word_index{},selected_bit{};
    bool found{},probed{};
};
// Actual16EA60 occupied-slot DATA scan. Keep one bitmap DWORD read per slot:
// repeating a word is observable and cannot be replaced by a cached word/CTZ.
// The caller captures descriptor fields at its own storage/stack boundaries.
template<class Memory>inline NativeTu15BitmapScan native_tu15_scan_bitmap_slots(const Memory&memory,
    std::uint32_t bitmap,std::uint32_t first,std::uint32_t limit) {
    NativeTu15BitmapScan result;result.index=first;
    if(std::bit_cast<std::int32_t>(first)<0)return result;
    while(std::bit_cast<std::int32_t>(result.index)<std::bit_cast<std::int32_t>(limit)) {
        result.word_index=result.index>>5u;result.probed=true;
        std::array<std::byte,4>bytes{};memory.read(bitmap+result.word_index*4u,bytes);
        std::uint32_t word{};for(unsigned n=0;n<4;++n)word|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
        result.selected_bit=word&(1u<<(result.index&31u));
        if(result.selected_bit){result.found=true;break;}
        ++result.index;
    }
    return result;
}
} // namespace halo2
