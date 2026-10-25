#include "halo2/source_loop_track_transition_tu15.hpp"
#include <stdexcept>
namespace halo2 {
SourceLoopTrackTransitionTu15 source_loop_track_transition_tu15(
    const std::array<std::uint32_t,7>& sounds,std::uint32_t flags,
    std::uint8_t current,std::uint8_t pending,bool alternate) {
    if(current>=7 || pending>=7)throw std::invalid_argument("Loop track phase escapes the original seven-state domain");
    // Literal original455BA8/455BC4 tables; complete retail/TU1.1/TU1.5
    // correspondence retains these values. Input phase numbering is source.
    constexpr std::array<std::uint8_t,7> normal{1,1,2,5,5,1,6},other{3,3,2,4,4,3,6};
    SourceLoopTrackTransitionTu15 out;out.pending=pending;
    const bool current_alternate=((1u<<current)&0x58u)!=0;
    if(current!=pending && sounds[pending]!=0xffffffffu && alternate==current_alternate)return out;
    out.traversed=true;
    const auto impulses=(flags&1u)|((flags&2u)?0x44u:0u)|((flags&4u)?0x28u:0u);
    const auto &table=alternate?other:normal;
    auto previous=current,next=table[current];
    while(next!=previous) {
        if(sounds[next]!=0xffffffffu) {
            if(!(impulses&(1u<<next))){out.sound=sounds[next];break;}
            out.impulse=sounds[next];
        }
        previous=next;next=table[next];
    }
    out.pending=next;return out;
}
}
