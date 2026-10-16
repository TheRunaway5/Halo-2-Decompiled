#include "halo2/native_tu15_sound_two_product_accumulator.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace halo2::tu15_sound_two_product_accumulator {
namespace {
std::int64_t signed_word(Word24 w){return w.bits&0x800000u?std::int64_t(w.bits)-0x1000000:std::int64_t(w.bits);}
std::int64_t saturate48(std::int64_t a){return std::clamp(a,-(std::int64_t{1}<<47),(std::int64_t{1}<<47)-1);}
Word24 transfer(std::int64_t a){
    constexpr auto divisor=std::int64_t{1}<<24;
    const auto shifted=a>=0?a/divisor:-((-a+divisor-1)/divisor);
    return Word24(std::uint32_t(std::clamp(shifted,std::int64_t{-0x800000},std::int64_t{0x7fffff}))&0xffffffu);
}
void validate(Word24 w){if(w.bits>0xffffffu)throw std::invalid_argument("Mutated word exceeds 24 bits");}
std::uintptr_t address(const Word24* p){
    auto a=reinterpret_cast<std::uintptr_t>(p);if(a>std::numeric_limits<std::uintptr_t>::max()-32*sizeof(Word24))throw std::invalid_argument("Native span address overflow");return a;
}
bool overlaps(std::uintptr_t a,std::uintptr_t b){return a<b+32*sizeof(Word24)&&b<a+32*sizeof(Word24);}
}
Result process(const Controls& c,const Parameters& p,State& s,std::span<const Word24,32> primary,std::span<const Word24,32> secondary,std::span<Word24,32> output,std::span<ArithmeticTrace,32> arithmetic){
    validate(c.entry_status);validate(p.primary_gain);validate(p.secondary_gain);validate(s.flags);
    // The modules explicitly force SM, then clear it. Entry normal scaling,
    // ordinary 24-bit arithmetic and signed multiplication are the recovered
    // domain; SC changes MOVEC semantics; incoming LF/FV loop context is unowned.
    // These modes and reserved SR bits must be zero. Other entry mode bits remain
    // explicit numeric context and are not a recovered whole-engine producer.
    if(c.entry_status.bits&((3u<<10)|(1u<<17)|(1u<<14)|(1u<<18)|(1u<<12)|(1u<<13)|(1u<<15)|(1u<<16)))throw std::invalid_argument("Unsupported original entry status mode");
    if(c.state_relative!=AddressingMode::linear)throw std::invalid_argument("Unsupported original state-relative addressing mode");
    for(auto mode:{c.primary,c.secondary,c.output})if(mode!=AddressingMode::linear)throw std::invalid_argument("Unsupported original address mode");
    for(auto w:primary)validate(w);
    for(auto w:secondary)validate(w);
    for(auto w:output)validate(w);
    const auto a=address(primary.data()),b=address(secondary.data()),o=address(output.data());
    if(overlaps(a,b)||overlaps(a,o)||overlaps(b,o))throw std::invalid_argument("Native overlap is outside recovered buffer graph");
    auto status=c.entry_status.bits;
    if(!(s.flags.bits&1u)){
        s.flags=Word24(s.flags.bits|1u); // P01EF/01F0 or P0219/021A
        status&=~1u; // Original BSET #0,X0 sets carry to the old clear flag.
    }
    const Word24 captured(status|(1u<<20)); // MOVEC SR,X0; BSET20; MOVEC X0,SR
    for(std::size_t i=0;i<32;i++){
        // BRCLR bit1 skips BOTH dry-load and primary-load. The alternate path
        // reuses the captured status word as X0; it does not read primary[i].
        const bool add_existing=s.flags.bits&2u;
        const auto source=add_existing?primary[i]:captured;
        auto sum=add_existing?signed_word(output[i])*(std::int64_t{1}<<24):0;
        const auto product0=saturate48(2*signed_word(source)*signed_word(p.primary_gain));
        sum=saturate48(sum+product0);
        const auto product1=saturate48(2*signed_word(secondary[i])*signed_word(p.secondary_gain));
        const auto final=saturate48(sum+product1);
        arithmetic[i]={product0,sum,product1,final};
        output[i]=transfer(final);
    }
    return {captured,false}; // Original exit explicitly clears SM.
}
Parameters authored_parameters(){return {Word24(0x7fffff),Word24(0)};}
} // namespace halo2::tu15_sound_two_product_accumulator
