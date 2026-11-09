#pragma once
#include <cstdint>

namespace halo2 {
enum class NativeTu15HalfwordBitQueryPhase {argument,word,finished};
struct NativeTu15HalfwordBitQuery {
    std::uint32_t index{},mask{},pointer{},word{},tested{};
    bool present{};
};
// Actual FA5D0 tests a 16-bit authored bitfield with the low five bits of
// the index. Bits 16..31 are therefore absent; the index never wraps at 16.
// Access owns live caller arguments and return lifetime, not query results.
template<class Access>bool native_tu15_halfword_contains_bit(Access&access,
    std::uint32_t index,std::uint32_t argument_address){
    NativeTu15HalfwordBitQuery data;data.index=index;data.mask=1u<<(index&31u);
    data.pointer=access.read(NativeTu15HalfwordBitQueryPhase::argument,data,argument_address,4);
    data.word=access.read(NativeTu15HalfwordBitQueryPhase::word,data,data.pointer,2);
    data.tested=data.word&data.mask;data.present=data.tested!=0;
    access.complete(data);return data.present;
}
} // namespace halo2
