#pragma once
#include <cstdint>
#include <stdexcept>
#include <type_traits>

namespace halo2::tu15_sound {
// One C++ object type for shared native effect buffers. The value is a raw
// unsigned24-bit word; each effect retains its own signed arithmetic rules.
// This is sample storage, not a serialized DSP address space or program bank.
struct Word24 {
    std::uint32_t bits{};
    Word24()=default;
    explicit Word24(std::uint32_t raw):bits(raw) {
        if(raw>0xffffffu)throw std::invalid_argument("Data word exceeds 24 bits");
    }
    bool operator==(const Word24&)const=default;
};
static_assert(sizeof(Word24)==sizeof(std::uint32_t));
static_assert(alignof(Word24)==alignof(std::uint32_t));
static_assert(std::is_standard_layout_v<Word24>);
static_assert(std::is_trivially_copyable_v<Word24>);
} // namespace halo2::tu15_sound
