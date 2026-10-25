#pragma once
#include "halo2/content.hpp"
#include <cstdint>
#include <optional>

namespace halo2 {
// Native voice identity is owned by the actual playback backend. Zero means
// no native voice; an Xbox datum is never narrowed or fabricated from it.
using SourceLoopNativeVoiceTu15=std::uint64_t;
struct SourceLoopTrackStateTu15 {
    SourceLoopNativeVoiceTu15 primary{};
    std::uint8_t phase{},pending{};
};
struct SourceLoopTrackRequestTu15 {
    unsigned state{}; // Original0 start,1 update,2 stop.
    bool created{},alternate{},stopped{},was_stopping{};
    std::uint32_t definition_flags{};
    float fade_duration{},control{};
};
class SourceLoopTrackPlaybackTu15 {
public:
    virtual ~SourceLoopTrackPlaybackTu15()=default;
    virtual std::uint32_t definition(SourceLoopNativeVoiceTu15)const=0;
    virtual bool has_output(SourceLoopNativeVoiceTu15)const=0;
    virtual SourceLoopNativeVoiceTu15 find(std::uint32_t,std::optional<unsigned> pitch)=0;
    // Original222350/222320 state0 impulse,1 start,2 continuing playback.
    virtual SourceLoopNativeVoiceTu15 create(std::uint32_t,std::optional<unsigned> pitch,unsigned state)=0;
    virtual unsigned pitch_range(std::uint32_t,float control)=0;
    virtual void fade_out(SourceLoopNativeVoiceTu15,float seconds)=0;
    // Inline221085..2210E3 fades this existing end voice with curve1.
    //222710 instead obtains its curve from the owning sound class.
    virtual void fade_out_linear(SourceLoopNativeVoiceTu15,float seconds)=0;
    virtual void fade_in(SourceLoopNativeVoiceTu15,float seconds)=0;
    virtual void crossfade(SourceLoopNativeVoiceTu15 next,SourceLoopNativeVoiceTu15 previous,float seconds)=0;
    virtual void stopping(SourceLoopNativeVoiceTu15)=0;
};
// Complete per-track request decisions within220C60. Device availability,
// source aggregation, voice/RNG ownership, packet completion and details are
// separate owners. Backend operations must act on real retained native voices.
void source_loop_track_update_tu15(const LoopingSound::Track&,
    const SourceLoopTrackRequestTu15&,SourceLoopTrackStateTu15&,SourceLoopTrackPlaybackTu15&);
}
