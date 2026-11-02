#pragma once
#include "halo2/native_tu15_sound_gp_output.hpp"
namespace halo2::tu15_ep_input {
using Word24=tu15_sound::Word24;
using Inputs=std::array<std::array<Word24,256>,5>;
class InputWindow final {
public:
    const Inputs& values()const noexcept{return values_;}
    std::uint32_t half_word_offset()const noexcept{return half_;}
private:
    Inputs values_{};
    std::uint32_t half_{};
    InputWindow()=default;
    friend InputWindow copy_input_window(const tu15_gp_output::LogicalCopyReceipt&,std::uint32_t);
};
// Original module5's five command7 acquisitions: the first five GP rings,
// in left/center/right/surround-left/surround-right order, at half0 or256.
// Read the actual current retained resource views and own the copied values.
// The receipt's ordinal is NOT a frame clock or historical value snapshot.
// Caller excludes source changes and supplies its actual selected half. This
// neither chooses/advances phase nor supplies input consumption, DSP status,
// frame completion, DMA byte packing, FIFO delivery or hardware time.
InputWindow copy_input_window(const tu15_gp_output::LogicalCopyReceipt&,
    std::uint32_t half_word_offset);
}
