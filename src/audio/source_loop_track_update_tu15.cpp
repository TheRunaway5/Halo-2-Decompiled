#include "halo2/source_loop_track_update_tu15.hpp"
#include "halo2/source_loop_track_transition_tu15.hpp"
#include <stdexcept>

namespace halo2 {
void source_loop_track_update_tu15(const LoopingSound::Track& authored,
    const SourceLoopTrackRequestTu15& request,SourceLoopTrackStateTu15& track,
    SourceLoopTrackPlaybackTu15& playback) {
    constexpr auto none=0xffffffffu;
    if(request.state>2 || track.phase>=7 || track.pending>=7)
        throw std::invalid_argument("Loop request escapes the original phase domain");
    bool alternate=request.alternate;
    if(authored.alternate_transition_in==none && authored.alternate==none &&
        authored.alternate_end==none && authored.alternate_transition_out==none && !(authored.flags&0x14u))alternate=false;
    if(request.created || !track.primary) {
        track.primary=0;track.phase=alternate?3:0;track.pending=track.phase;
    }
    if(request.state==0 && authored.start!=none) {
        const auto old_end=playback.find(authored.end,{});
        if(!(authored.flags&1u))track.primary=playback.create(authored.start,{},1);
        else {
            (void)playback.create(authored.start,{},0);
            if(old_end)playback.fade_out_linear(old_end,authored.fade_in);
            playback.fade_in(track.primary,authored.fade_in);
        }
        track.phase=track.pending=0;
    }
    const auto current_alternate=[&]{return ((1u<<track.phase)&0x58u)!=0;};
    if(request.state!=2 && !request.stopped) {
        const std::array sounds{authored.start,authored.loop,authored.end,
            authored.alternate_transition_in,authored.alternate,authored.alternate_transition_out,authored.alternate_end};
        const auto next=source_loop_track_transition_tu15(sounds,authored.flags,track.phase,track.pending,alternate);
        track.pending=next.pending;
        if(next.traversed && next.impulse!=none)(void)playback.create(next.impulse,{},0);
        if(next.sound==none && alternate==current_alternate())return;
        const bool start_with_fade=request.state==0 && (authored.flags&1u);
        if(track.primary && !start_with_fade) {
            if(playback.definition(track.primary)==next.sound || alternate==current_alternate() || !(authored.flags&4u))return;
            SourceLoopNativeVoiceTu15 replacement{};
            if(next.sound!=none) {
                replacement=playback.find(next.sound,{});
                if(!replacement)replacement=playback.create(next.sound,{},2);
                if(!replacement)return;
            }
            playback.crossfade(replacement,track.primary,authored.alternate_crossfade);
            track.primary=replacement;track.phase=track.pending=alternate?4:1;
        }else if(next.sound!=none) {
            const auto range=playback.pitch_range(next.sound,request.control);
            auto replacement=playback.find(next.sound,range);
            if(!replacement)replacement=playback.create(next.sound,range,2);
            if(replacement) {
                if(request.state!=0)playback.crossfade(replacement,0,.5f);
                else if(authored.flags&1u)playback.crossfade(replacement,0,authored.fade_in);
                track.primary=replacement;track.phase=track.pending;
            }
        }
    }else if(!request.was_stopping) {
        if(request.fade_duration!=0.f) {
            if(track.primary)playback.fade_out(track.primary,request.fade_duration);
            return;
        }
        const bool current=current_alternate();
        const bool alternate_end=current && ((authored.flags&0x10u) || authored.alternate_end!=none);
        const auto end=alternate_end?authored.alternate_end:authored.end;
        const bool impulse=(authored.flags&(alternate_end?0x10u:2u))!=0;
        if(track.primary) {
            if(!impulse && end==none && (request.definition_flags&2u))return;
            if(impulse || end==none)playback.fade_out(track.primary,
                current && (authored.flags&0x10u)?authored.alternate_fade_out:authored.fade_out);
        }
        if(end!=none) {
            if(impulse)(void)playback.create(end,{},0);
            else if(track.primary && playback.has_output(track.primary)) {
                track.pending=alternate_end?6:2;playback.stopping(track.primary);
            }
        }
    }
}
}
