#pragma once
#include "halo2/native_tu15_sound_word24.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
namespace halo2::tu15_status_context {
using Word24=tu15_sound::Word24;
enum class Producer {incoming_known,incoming_unresolved,cold_flag_bset,multiply,dry_add,bus_transfer,loop_setup,loop_restore};
struct Origin {Producer producer;std::uint64_t evidence;std::uint32_t source_pc,ordinal;bool operator==(const Origin&)const=default;};
class Operations;
class Status final {
public:
    // The caller declares its incoming evidence. This label is diagnostic
    // provenance, not a reset/install/frame or other authenticated receipt.
    static Status incoming(std::uint32_t value,std::uint32_t known_mask,std::uint64_t evidence);
    std::uint32_t value()const noexcept{return value_;}
    std::uint32_t known_mask()const noexcept{return known_;}
    const std::array<Origin,24>& origins()const noexcept{return origins_;}
    bool operator==(const Status&)const=default;
    void require_record0_profile()const;
    // Numeric consumers must request established bits, never guess unknowns.
    std::uint32_t numeric_value(std::uint32_t required_mask)const;
private:
    std::uint32_t value_,known_;std::array<Origin,24> origins_;
    Status(std::uint32_t,std::uint32_t,std::uint64_t);
    void write(std::uint32_t mask,std::uint32_t bits,const Origin&);
    void sticky(unsigned bit,bool witness,const Origin&);
    friend class Operations;
};
inline constexpr std::uint32_t record0_clear_modes=(3u<<10)|(1u<<13)|(1u<<14)|(1u<<15)|(1u<<16)|(1u<<17)|(1u<<20);
struct Step {
    Producer operation;std::uint32_t source_pc,ordinal,written_mask;
    std::uint32_t before_value,before_known,after_value,after_known;
    std::int64_t accumulator;std::uint32_t transferred_word;bool limited;
    bool operator==(const Step&)const=default;
};
struct Trace {std::array<Step,800> steps{};std::size_t count{};bool operator==(const Trace&)const=default;};
class Operations final {
public:
    static void cold_flag_bset(Status&,Trace&);
    static void loop_flag(Status&,Trace&,bool value,bool restoring,std::uint32_t pc,std::uint32_t ordinal);
    static std::int64_t multiply(Status&,Trace&,Word24,Word24,std::uint32_t ordinal);
    static std::int64_t add_dry(Status&,Trace&,std::int64_t,Word24,std::uint32_t ordinal);
    static Word24 transfer(Status&,Trace&,std::int64_t,std::uint32_t ordinal);
private:
    static void arithmetic_flags(Status&,std::int64_t,const Origin&,bool writes_carry,bool carry);
    static void record(Trace&,const Status&,std::uint32_t before_value,std::uint32_t before_known,Producer,std::uint32_t pc,std::uint32_t ordinal,std::uint32_t mask,std::int64_t accumulator=0,std::uint32_t transferred=0,bool limited=false);
};
}
