#include "halo2/native_tu15_sound_gain_fanout.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace halo2::tu15_sound_gain_fanout {
namespace {
std::int64_t signed_word(Word24 word) {
    return word.bits&0x800000u?std::int64_t(word.bits)-0x1000000:std::int64_t(word.bits);
}
std::int64_t floor24(std::int64_t a) {
    constexpr std::int64_t divisor=1<<24;
    return a>=0?a/divisor:-((-a+divisor-1)/divisor);
}
Word24 transfer(std::int64_t a) {
    const auto limited=std::clamp(floor24(a),std::int64_t{-0x800000},std::int64_t{0x7fffff});
    return Word24(std::uint32_t(limited)&0xffffffu);
}
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
void process(const Controls& c,const Parameters& p,State& s,Input input,const std::array<Output,8>& routes) {
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
    // P0173..0177: initialization changes only flag bit 0, preserving gains.
    s.flags=Word24(s.flags.bits|1u);
    // P017C..017E: publish the logical beginning of the gain sequence. Native
    // state owns no emulated DSP address and no inferred external consumer.
    s.published_gain_source=GainSource::sequence_begin;
    for(std::size_t route=0;route<8;route++) { // P0181 DOR eight authored routes
        const auto gain=p.gains[route]; // P0186
        for(std::size_t sample=0;sample<32;sample++) { // P0187 DOR32
            // P0189..018A fractional signed MPY. This product plus one dry
            // word is at most 2^48, within the original 56-bit accumulator.
            auto accumulator=2*signed_word(input[sample])*signed_word(gain);
            if(s.flags.bits&2u)accumulator+=signed_word(routes[route][sample])*(std::int64_t{1}<<24); // P018B..018F
            routes[route][sample]=transfer(accumulator); // P0190 normal limiter
        }
    }
}
Parameters authored_parameters() {
    Parameters p;
    for(auto& gain:p.gains)gain=Word24(0x3ffed0u);
    return p;
}
} // namespace halo2::tu15_sound_gain_fanout
