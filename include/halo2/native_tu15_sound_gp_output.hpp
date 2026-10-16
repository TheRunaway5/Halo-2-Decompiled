#pragma once
#include "halo2/native_tu15_sound_word24.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <span>
namespace halo2::tu15_gp_output {
static_assert(sizeof(void*)==8,"The native GP output owner requires64-bit pointers");
using Word24=tu15_sound::Word24;
inline constexpr std::array<unsigned,6> mixbin_order{0,2,1,4,5,3};
inline constexpr std::size_t ring_words=512,aux_words=640,owned_words=6*ring_words+aux_words;
struct RetainedValues {
    std::span<const Word24,owned_words> values;
    std::array<std::uint32_t,6> byte_cursors{};
};
struct RingWrite {
    unsigned mixbin;
    std::uint32_t before_byte_cursor,after_byte_cursor;
    bool split;
};
class OutputOwner;
class LogicalCopyReceipt final {
public:
    LogicalCopyReceipt(const LogicalCopyReceipt&)noexcept=default;
    LogicalCopyReceipt& operator=(const LogicalCopyReceipt&)noexcept=default;
    // Moving a receipt retains both valid resource views. A moved-from receipt
    // is still a completed copy receipt, with the same immutable metadata.
    LogicalCopyReceipt(LogicalCopyReceipt&&)noexcept;
    LogicalCopyReceipt& operator=(LogicalCopyReceipt&&)noexcept;
    const std::array<RingWrite,6>& writes()const noexcept{return writes_;}
    std::uint64_t copy_ordinal()const noexcept{return copy_ordinal_;}
    std::span<const Word24,owned_words> retained_values()const noexcept;
    // These are retained current resources, not immutable per-copy snapshots.
    // The receipt proves completed native value copies, never EP consumption,
    // original DMA byte format, graph execution, installation or hardware time.
private:
    struct Resources {
        std::array<Word24,owned_words> output_values{};
        std::array<std::uint32_t,6> byte_cursors{};
        std::uint64_t copy_ordinal{};
    };
    std::shared_ptr<const Resources> resources_;
    std::array<RingWrite,6> writes_{};
    std::uint64_t copy_ordinal_{};
    LogicalCopyReceipt(std::shared_ptr<const Resources>,std::array<RingWrite,6>,std::uint64_t);
    friend class OutputOwner;
};
class OutputOwner final {
public:
    // Source cold supervisor reconstructs descriptors/cursors but never clears
    // output scratch. Both factories copy actual supplied logical values.
    static std::unique_ptr<OutputOwner> cold_cursor_phase(std::span<const Word24,owned_words>);
    static std::unique_ptr<OutputOwner> retained_phase(const RetainedValues&);
    OutputOwner(const OutputOwner&)=delete;
    OutputOwner& operator=(const OutputOwner&)=delete;
    OutputOwner(OutputOwner&&)=delete;
    OutputOwner& operator=(OutputOwner&&)=delete;
    std::span<const Word24,ring_words> ring(unsigned)const;
    std::span<const Word24,aux_words> auxiliary_values()const noexcept;
    std::span<const Word24,owned_words> retained_values()const noexcept;
    std::array<std::uint32_t,6> byte_cursors()const noexcept;
    // Exactly six 32-word circular copies followed by the640-word linear copy.
    // Caller supplies values after its independently validated graph/service
    // phase. This method neither runs nor certifies that phase.
    LogicalCopyReceipt copy_values(std::span<const Word24,32*32>);
private:
    std::shared_ptr<LogicalCopyReceipt::Resources> resources_;
    explicit OutputOwner(const RetainedValues&);
};
}
