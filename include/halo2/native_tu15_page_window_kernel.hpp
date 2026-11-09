#pragma once
#include <algorithm>
#include <bit>
#include <cstdint>

namespace halo2 {
enum class NativeTu15PageWindowPhase {owner_tick,candidate_tick,best_tick,age_range,candidate_count,best_count,early_return,finished};
struct NativeTu15PageWindow {
    std::uint32_t owner{},candidate{},best{},tick{},candidate_tick{},candidate_age{},best_tick{},best_age{},candidate_count{},best_count{};
    bool prefer{};
};
// Shared13FE00 DATA policy: saturate unsigned ages, prefer a sufficiently
// older window, and compare signed page counts inside the age tolerance.
template<class Access>bool native_tu15_prefer_page_window(Access &memory,NativeTu15PageWindow &s){
    using P=NativeTu15PageWindowPhase;
    const auto signed_word=[](auto value){return std::bit_cast<std::int32_t>(value);};
    s.tick=memory.read(P::owner_tick,s,s.owner+0x38u);
    s.candidate_tick=memory.read(P::candidate_tick,s,s.candidate+4u);
    s.candidate_age=std::min(s.tick-s.candidate_tick,300u);
    s.best_tick=memory.read(P::best_tick,s,s.best+4u);s.best_age=std::min(s.tick-s.best_tick,300u);
    if(signed_word(s.candidate_age+150u)<signed_word(s.best_age)){
        s.prefer=false;memory.complete(P::early_return,s);return false;
    }
    memory.prepare(P::age_range,s);
    if(signed_word(s.candidate_age)>signed_word(s.best_age+150u))s.prefer=true;
    else{
        s.candidate_count=memory.read(P::candidate_count,s,s.candidate+12u);
        s.best_count=memory.read(P::best_count,s,s.best+12u);
        s.prefer=signed_word(s.candidate_count)<signed_word(s.best_count);
    }
    memory.complete(P::finished,s);return s.prefer;
}
} // namespace halo2
