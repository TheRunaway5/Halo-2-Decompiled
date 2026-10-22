#include "halo2/native_tu15_sound_record0_context.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace halo2::tu15_record0_context {
using namespace gain;
using namespace tu15_status_context;
namespace {
void validate(Word24 word) {
    if(word.bits>0xffffffu)throw std::invalid_argument("Mutated data word exceeds 24 bits");
}
std::uintptr_t address(const Word24* p) {
    const auto a=reinterpret_cast<std::uintptr_t>(p);
    if(a>std::numeric_limits<std::uintptr_t>::max()-32*sizeof(Word24))throw std::invalid_argument("Native span address overflow");
    return a;
}
bool overlaps(std::uintptr_t a,std::uintptr_t b) {
    constexpr auto bytes=32*sizeof(Word24);
    return a<b+bytes&&b<a+bytes;
}
}
void process(const Controls& c,const Parameters& p,State& s,Input input,const std::array<Output,8>& routes,Status& status,Trace& trace) {
    status.require_record0_profile();
    if(c.entry_scaling!=ScalingMode::none||c.arithmetic_saturation||c.sixteen_bit_arithmetic||c.double_precision_multiply)
        throw std::invalid_argument("Original arithmetic mode is unsupported");
    if(c.state_relative!=AddressingMode::linear)throw std::invalid_argument("Unsupported original state-relative addressing mode");
    for(auto mode:{c.input,c.destination_table,c.gain_sequence,c.output})
        if(mode!=AddressingMode::linear)throw std::invalid_argument("Original address mode is unsupported");
    validate(s.flags);
    if(s.published_gain_source&&*s.published_gain_source!=GainSource::sequence_begin)
        throw std::invalid_argument("Unknown logical gain source selection");
    for(auto gain:p.gains)validate(gain);
    for(auto word:input)validate(word);
    const auto source=address(input.data());
    std::array<std::uintptr_t,8> outputs;
    for(std::size_t route=0;route<8;route++) {
        outputs[route]=address(routes[route].data());
        if(overlaps(source,outputs[route]))throw std::invalid_argument("Input/output overlap is outside recovered graph");
        for(auto word:routes[route])validate(word);
        for(std::size_t prior=0;prior<route;prior++)
            if(outputs[prior]!=outputs[route]&&overlaps(outputs[prior],outputs[route]))
                throw std::invalid_argument("Partial output overlap is outside recovered graph");
    }
    const bool entry_loop_flag=(status.value()&(1u<<15))!=0;
    trace.count=0;
    if(!(s.flags.bits&1u))Operations::cold_flag_bset(status,trace);
    // P0173..0177: initialization changes only flag bit 0, preserving gains.
    s.flags=Word24(s.flags.bits|1u);
    // P017C..017E: publish the logical beginning of the gain sequence. Native
    // state owns no emulated DSP address and no inferred external consumer.
    s.published_gain_source=GainSource::sequence_begin;
    Operations::loop_flag(status,trace,true,false,0x181,0);
    for(std::size_t route=0;route<8;route++) { // P0181 DOR eight authored routes
        const auto gain=p.gains[route]; // P0186
        Operations::loop_flag(status,trace,true,false,0x187,static_cast<std::uint32_t>(route));
        for(std::size_t sample=0;sample<32;sample++) { // P0187 DOR32
            // P0189..018A fractional signed MPY. This product plus one dry
            // word is at most 2^48, within the original 56-bit accumulator.
            const auto ordinal=static_cast<std::uint32_t>(route*32+sample);
            auto accumulator=Operations::multiply(status,trace,input[sample],gain,ordinal);
            if(s.flags.bits&2u)accumulator=Operations::add_dry(status,trace,accumulator,routes[route][sample],ordinal); // P018B..018F
            routes[route][sample]=Operations::transfer(status,trace,accumulator,ordinal); // P0190 normal limiter
        }
        Operations::loop_flag(status,trace,true,true,0x190,static_cast<std::uint32_t>(route));
    }
    Operations::loop_flag(status,trace,entry_loop_flag,true,0x191,0);
}
}
