#pragma once
#include "halo2/physics_contact_record.hpp"

namespace halo2 {
struct NativeTu15MillisecondClockSample {std::uint32_t original_cell{},sampled_cell{},milliseconds{};};
template<class Memory>inline std::uint32_t native_millisecond_clock_word(const Memory&memory,std::uint32_t at) {
    std::array<std::byte,4>bytes{};memory.read(at,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8*n);return value;
}
// The source publishes a clock-cell pointer before its real platform refresh.
// Refresh owns that operation and returns the actual cell selected afterward;
// this kernel reads the backed counter and never supplies a host-time value.
template<class Memory,class Refresh>inline NativeTu15MillisecondClockSample native_tu15_sample_millisecond_clock(
    const Memory&memory,Refresh refresh) {
    NativeTu15MillisecondClockSample sample;sample.original_cell=native_millisecond_clock_word(memory,0x41b774u);
    sample.sampled_cell=refresh(sample.original_cell);
    sample.milliseconds=native_millisecond_clock_word(memory,sample.sampled_cell);return sample;
}
} // namespace halo2
