#include "halo2/native_tu15_sound_chorus.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace halo2::tu15_sound_chorus {
namespace {
constexpr auto unit=std::int64_t{1}<<24;
std::int64_t signed_word(Word24 w){return w.bits&0x800000u?std::int64_t(w.bits)-0x1000000:std::int64_t(w.bits);}
std::int64_t floor_transfer(std::int64_t value){return value>=0?value/unit:-((-value+unit-1)/unit);}
Word24 transfer(std::int64_t value){return Word24(std::uint32_t(std::clamp(floor_transfer(value),std::int64_t{-0x800000},std::int64_t{0x7fffff}))&0xffffffu);}
void validate(Word24 w){if(w.bits>0xffffffu)throw std::invalid_argument("Mutated word exceeds24bits");}
struct Extent {std::uintptr_t begin,end;};
Extent extent(const Word24*p,std::size_t words){auto a=reinterpret_cast<std::uintptr_t>(p);if(a>std::numeric_limits<std::uintptr_t>::max()-words*sizeof(Word24))throw std::invalid_argument("Native extent overflow");return{a,a+words*sizeof(Word24)};}
bool overlaps(Extent a,Extent b){return a.begin<b.end&&b.begin<a.end;}
}
void process(const Controls& c,const Parameters& p,State& s,Bindings declared,std::span<Word24,401> history,std::span<SampleTrace,32> trace){
    validate(c.entry_status);validate(p.gain);validate(p.depth);validate(s.flags);
    if(c.entry_status.bits&((3u<<10)|(1u<<20)|(1u<<17)|(1u<<14)|(1u<<18)|(1u<<12)|(1u<<13)|(1u<<15)|(1u<<16)))throw std::invalid_argument("Unsupported original entry status");
    for(auto mode:c.addressing)if(mode!=AddressingMode::linear)throw std::invalid_argument("Unsupported original addressing mode");
    if(c.state_relative!=AddressingMode::linear)throw std::invalid_argument("Unsupported original state-relative addressing mode");
    if(c.live_begin_offset||c.live_span!=400)throw std::invalid_argument("Unrecovered live history geometry");
    const bool cold=!(s.flags.bits&1u);
    if(!cold&&!s.active)throw std::invalid_argument("Warm state has no owned cached bindings");
    auto active=cold?ActiveState{declared,0,400}:*s.active;
    if(active.cached_end!=400||active.cursor>400)throw std::invalid_argument("Unrecovered cached history geometry");
    for(auto w:active.bindings.audio)validate(w);
    for(auto w:active.bindings.modulation)validate(w);
    for(auto w:active.bindings.output)validate(w);
    for(auto w:history)validate(w);
    const std::array<Extent,4> ranges={extent(active.bindings.audio.data(),32),extent(active.bindings.modulation.data(),32),extent(active.bindings.output.data(),32),extent(history.data(),401)};
    for(unsigned i=0;i<ranges.size();i++)for(unsigned j=i+1;j<ranges.size();j++)if(overlaps(ranges[i],ranges[j]))throw std::invalid_argument("Unrecovered native buffer overlap");
    // Preflight every address before first mutation. One correction by400 is
    // authentic; repeated modulo/clamping would hide unresolved graph input.
    std::array<std::int64_t,32> before{},after{};std::array<unsigned,32> indices{},cursors{};
    auto cursor=active.cursor;
    for(unsigned i=0;i<32;i++){
        const auto depth=signed_word(p.depth);
        auto position=2*depth*signed_word(active.bindings.modulation[i])-depth*unit+std::int64_t(cursor)*unit; // P0260..0263
        before[i]=position;
        // Original full56-bit CMP precedes MOVE A,R1. Fractional excess past
        // cached end takes the upper correction even if its floor is=end.
        if(position>std::int64_t(active.cached_end)*unit)position-=std::int64_t(c.live_span)*unit; // P0267..026B
        else if(position<0)position+=std::int64_t(c.live_span)*unit; // P026D..0270
        after[i]=position;const auto index=floor_transfer(position);
        if(index<0||index>400)throw std::invalid_argument("Delay read leaves recovered native history extent");
        indices[i]=unsigned(index);cursor++;if(cursor>active.cached_end)cursor=0; // P0277..027A,0282..0288
        cursors[i]=cursor;
    }
    if(cold)s.flags=Word24(s.flags.bits|1u); // P0243..0250 caches, no Y clear or gain overwrite
    const bool dry=s.flags.bits&2u;
    for(unsigned i=0;i<32;i++){
        const auto delayed=history[indices[i]];
        auto sum=2*signed_word(p.gain)*signed_word(delayed); // P0272..0274
        const auto audio=active.bindings.audio[i];sum+=signed_word(audio)*unit; // P0275..0276
        history[active.cursor]=audio; // P0279 writes before output transfer, inclusive end cell
        if(dry)sum+=signed_word(active.bindings.output[i])*unit; // P027C..0280
        active.bindings.output[i]=transfer(sum); // P0281; single final normal limiter
        active.cursor=cursors[i];trace[i]={before[i],after[i],indices[i],delayed,sum,active.cursor};
    }
    s.active=active;
}
Parameters authored_parameters(){return{Word24(0),Word24(30)};}
} // namespace halo2::tu15_sound_chorus
