#include "halo2/source_loop_phased_output_tu15.hpp"
#include "halo2/source_loop_track_control_tu15.hpp"
#include "halo2/menu_music.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
namespace halo2 {namespace {
constexpr auto none=0xffffffffu;
std::uint32_t advance(std::uint32_t& seed){return seed=seed*0x19660du+0x3c6ef35fu;}
float base_pitch(const SoundVariationDefinition& definition){
    return float(std::bit_cast<std::int16_t>(Bytes(definition.playback).u16(0x18)));
}
Vec3 position(const SourceLoopPhasedOwnerTu15& owner){const Bytes p(owner.location);
    return {p.f32(0xc),p.f32(0x10),p.f32(0x14)};}
SourceLoopPhasedVoiceTu15* voice(SourceLoopPhasedTrackTu15& track,std::uint64_t id){
    const auto found=std::ranges::find(track.voices,id,&SourceLoopPhasedVoiceTu15::id);
    return found==track.voices.end()?nullptr:&*found;
}
void unlink(SourceLoopPhasedTrackTu15& track,std::uint64_t id){
    const auto current=voice(track,id);if(!current)throw FormatError("Native loop retirement lost its owned voice");
    const auto next=current->next==id?0:current->next,previous=current->previous;
    if(next){auto* n=voice(track,next);auto* p=voice(track,previous);
        if(!n||!p)throw FormatError("Native loop lost its retained voice ring");
        n->previous=previous;p->next=next;}
    if(track.phase.primary==id)track.phase.primary=next;
    std::erase_if(track.voices,[&](const auto& v){return v.id==id;});
}
void prune(SourceLoopPhasedTrackTu15& track,SourceLoopPhasedOutputTu15& output){
    for(;;){const auto it=std::ranges::find_if(track.voices,[&](const auto& v){return !output.active(v.id);});
        if(it==track.voices.end())break;unlink(track,it->id);}
}
std::vector<std::uint64_t> ring(SourceLoopPhasedTrackTu15& track,std::uint64_t first){
    std::vector<std::uint64_t> result;if(!first)return result;auto id=first;
    do{const auto* v=voice(track,id);if(!v || result.size()>=track.voices.size())
        throw FormatError("Native loop voice ring is stale or incomplete");
        result.push_back(id);id=v->next;}while(id!=first);
    return result;
}
MultiplayerLoopPacketParametersTu15 parameters(SourceLoopPhasedOutputTu15& output,
    const SourceLoopPhasedOwnerTu15& owner,const SourceLoopPhasedTrackTu15& track,
    unsigned ordinal,const SourceLoopPhasedVoiceTu15& v){
    const auto& definition=output.metadata(v.sound,v.range);
    const auto pitch=source_sound_control_pitch(definition,v.base_pitch,track.control);
    std::array<std::int16_t,5> interval{};for(unsigned n=0;n<5;++n)
        interval[n]=std::bit_cast<std::int16_t>(Bytes(definition.pitch_ranges.at(v.range).interval).u16(n*2));
    const auto layer=source_loop_pitch_layer_tu15(interval,pitch,(Bytes(definition.sound).u16(0)&0x2000u)!=0);
    return {pitch,owner.definition.tracks.at(ordinal).gain_db+Bytes(owner.location).f32(8)+layer.gain_db,track.control};
}
unsigned select(SourceLoopPhasedOutputTu15& output,std::uint32_t& used,
    std::uint32_t sound,unsigned range,std::uint32_t& seed,bool continuing){
    const auto& definition=output.metadata(sound,range);std::vector<std::uint16_t> chances;
    for(const auto& permutation:definition.permutations)chances.push_back(Bytes(permutation).u16(2));
    if(continuing)used=0;LoopingSoundSelection state{used,-1};
    const auto selected=source_looping_sound_permutation(chances,state,seed);used=state.used;
    if(!output.single_chunk(sound,range,unsigned(selected)))
        throw FormatError("Native phase requires its authored multi-chunk queue owner");
    return unsigned(selected);
}
struct Playback final:SourceLoopTrackPlaybackTu15 {
    SourceLoopPhasedOutputTu15& output;SourceLoopPhasedOwnerTu15& owner;
    SourceLoopPhasedTrackTu15& track;unsigned ordinal;std::uint32_t& seed;
    std::optional<float> initial_fade;
    Playback(SourceLoopPhasedOutputTu15& o,SourceLoopPhasedOwnerTu15& g,
        SourceLoopPhasedTrackTu15& t,unsigned n,std::uint32_t& s):output(o),owner(g),track(t),ordinal(n),seed(s){}
    std::uint32_t definition(std::uint64_t id)const override{
        const auto* v=voice(track,id);if(!v)throw FormatError("Native phase lost its real voice definition");return v->sound;}
    bool has_output(std::uint64_t id)const override{return output.active(id);}
    std::uint64_t find(std::uint32_t sound,std::optional<unsigned> range)override{
        for(const auto id:ring(track,track.phase.primary)){const auto* v=voice(track,id);
            if(v->sound==sound && (!range||v->range==*range))return id;}return 0;
    }
    unsigned pitch_range(std::uint32_t sound,float control)override{
        const auto& d=output.metadata(sound,0);
        const auto selected=source_sound_pitch_range(d.pitch_ranges,-1,source_sound_control_pitch(d,base_pitch(d),control));
        if(selected<0)throw FormatError("Native phase has no authored pitch range");return unsigned(selected);
    }
    std::uint64_t create(std::uint32_t sound,std::optional<unsigned> range,unsigned mode)override{
        const auto& d=output.metadata(sound,0);const auto pitch=base_pitch(d);advance(seed);
        const auto selected=range.value_or(pitch_range(sound,track.control));
        SourceLoopPhasedVoiceTu15 created;created.sound=sound;created.range=selected;created.mode=mode;created.base_pitch=pitch;
        created.seed=track.seed;created.used=track.used;
        const auto permutation=select(output,track.used,sound,selected,seed,mode==2);
        const auto values=parameters(output,owner,track,ordinal,created);
        MultiplayerLoopPacketTu15 packet{sound,owner.emitter,0,selected,permutation,values,position(owner)};
        //220C60/221AF0 complete these controls before the source SDK's
        //later channel pass. Install the equivalent initial native fade
        //before exposing the first PCM packet to the device callback.
        if(mode==2)packet.fade_in_seconds=initial_fade;
        const auto id=output.play(packet);if(!id)return 0;
        created.id=created.next=created.previous=id;
        try{source_loop_phased_insert_voice_tu15(track,created);}
        catch(...){output.retire(id);throw;}
        return id;
    }
    void fade_out(std::uint64_t id,float seconds)override{
        for(const auto current:ring(track,id)){const auto* v=voice(track,current);
            const auto& d=output.metadata(v->sound,v->range);
            output.fade(current,seconds,false,std::uint16_t(!(Bytes(d.sound_class).u16(10)&0x400u)));}
    }
    void fade_out_linear(std::uint64_t id,float seconds)override{output.fade(id,seconds,false,1);}
    void fade_in(std::uint64_t id,float seconds)override{
        for(const auto current:ring(track,id))if(voice(track,current)->mode==2)output.fade(current,seconds,true,1);}
    void crossfade(std::uint64_t next,std::uint64_t previous,float seconds)override{
        if(next)output.fade(next,seconds,true,1);if(previous)output.fade(previous,seconds,false,1);}
    void stopping(std::uint64_t id)override{auto* v=voice(track,id);
        if(!v)throw FormatError("Native phase stop lost its real primary");v->mode=3;}
};
void retire(SourceLoopPhasedOwnerTu15& owner,SourceLoopPhasedOutputTu15& output){
    for(auto& track:owner.tracks)for(const auto& v:track.voices)if(output.active(v.id))output.retire(v.id);
}
void release(SourceLoopPhasedStateTu15& state,std::uint32_t key){
    const auto found=state.synchronization.find(key);
    if(found==state.synchronization.end() || !found->second.references)
        throw FormatError("Native loop lost its actual synchronization reference");
    if(!--found->second.references)state.synchronization.erase(found);
}
}
void source_loop_phased_insert_voice_tu15(SourceLoopPhasedTrackTu15& track,SourceLoopPhasedVoiceTu15 created){
    if(!created.id || voice(track,created.id) || created.next!=created.id || created.previous!=created.id)
        throw FormatError("Native loop insertion requires its new owned self-linked voice");
    // Reserve before modifying the existing ring; allocation failure must
    //leave its live predecessor/successor unchanged.
    track.voices.reserve(track.voices.size()+1);
    if(track.phase.primary){auto* p=voice(track,track.phase.primary);
        if(!p)throw FormatError("Native phase primary disappeared during creation");
        const auto next=p->next;auto* following=voice(track,next);
        if(!following)throw FormatError("Native phase insertion lost its real successor");
        created.previous=p->id;created.next=next;following->previous=created.id;p->next=created.id;}
    track.voices.push_back(created);
}
void source_loop_phased_retire_voice_tu15(SourceLoopPhasedTrackTu15& track,std::uint64_t id){unlink(track,id);}
void source_loop_phased_request_tu15(SourceLoopPhasedStateTu15& state,SourceLoopPhasedOutputTu15& output,
    const SourceLoopSoundRequestTu15& request,const LoopingSound& definition){
    const auto& args=request.arguments;const Bytes location(request.location);
    const auto duration=std::bit_cast<float>(args[7]);
    if(args[5]>2 || !std::isfinite(location.f32(4)) || location.f32(4)<0 ||
        !std::isfinite(location.f32(8)) || !std::isfinite(duration) || duration<0)
        throw FormatError("Native synchronized loop request escapes its caller domain");
    if((definition.flags&~0x300u)!=0x18u || !definition.details.empty() || args[3]==none ||
        request.emitter_owner==none || (std::to_integer<unsigned>(request.location[3])&15u)!=1u)
        throw FormatError("Loop requires its remaining synchronization/profile owner");
    const auto key=std::pair{args[0],args[2]};auto found=state.owners.find(key);const bool created=found==state.owners.end();
    if(created){if(args[5]==2)return;
        for(const auto& track:definition.tracks)if(track.loop==none || (track.flags&~3u) || track.alternate!=none ||
            track.alternate_end!=none || track.alternate_transition_in!=none || track.alternate_transition_out!=none)
            throw FormatError("Native synchronized phase requires its alternate transition owner");
        for(const auto& track:definition.tracks)for(const auto sound:{track.start,track.loop,track.end})if(sound!=none){
            const auto& d=output.metadata(sound,0);
            if(Bytes(d.playback).f32(0x14)!=0.f)
                throw FormatError("Native phase requires its motion-dependent pitch owner");
            if(d.pitch_ranges.empty() || d.pitch_ranges.size()>9)
                throw FormatError("Native phase exceeds the original frame's pitch-layer domain");
            for(unsigned range=0;range<d.pitch_ranges.size();++range){const auto& selected=output.metadata(sound,range);
                for(unsigned p=0;p<selected.permutations.size();++p)if(!output.single_chunk(sound,range,p))
                    throw FormatError("Native phase requires its authored multi-chunk queue owner");}
        }
        auto sync=state.synchronization.find(args[3]);
        if(sync==state.synchronization.end())sync=state.synchronization.emplace(args[3],
            SourceLoopPhasedSynchronizationTu15{output.fresh_secondary(),0}).first;
        if(sync->second.references>=127)throw FormatError("Native synchronization exceeds the original reference domain");
        SourceLoopPhasedOwnerTu15 owner;owner.definition=definition;owner.emitter=request.emitter_owner;
        owner.synchronization=args[3];owner.location=request.location;owner.tracks.resize(definition.tracks.size());
        for(auto& track:owner.tracks){track.seed=sync->second.seed;track.control=location.f32(4);}
        found=state.owners.emplace(key,std::move(owner)).first;++sync->second.references;
    }
    auto& owner=found->second;
    if(owner.synchronization!=args[3] || owner.emitter!=request.emitter_owner)
        throw FormatError("Native synchronized loop changed its source owner generation");
    if(!(std::to_integer<unsigned>(request.location[0])&2u))owner.location=request.location;
    auto command=args[5];if(!created && !command && (args[6]&0x10u))command=1;
    for(unsigned n=0;n<owner.tracks.size();++n){auto& track=owner.tracks[n];prune(track,output);
        Playback playback(output,owner,track,n,track.seed);
        if(command!=0)playback.initial_fade=.5f;
        else if(definition.tracks[n].flags&1u)playback.initial_fade=definition.tracks[n].fade_in;
        source_loop_track_update_tu15(definition.tracks[n],{command,created,(args[6]&1u)!=0,
            false,owner.stopping,definition.flags&~0x300u,duration,track.control},track.phase,playback);
    }
    owner.stopping=command==2;owner.seen=true;owner.retain=(args[6]&2u)!=0;
}
void source_loop_phased_frame_tu15(SourceLoopPhasedStateTu15& state,SourceLoopPhasedOutputTu15& output,float elapsed){
    for(auto& [key,group]:state.synchronization)advance(group.seed); //128733 before128744
    for(auto it=state.owners.begin();it!=state.owners.end();){auto& owner=it->second;
        for(auto& track:owner.tracks)prune(track,output);
        const auto live=[&]{return std::ranges::any_of(owner.tracks,[](const auto& t){return !t.voices.empty();});};
        if((!owner.seen && !(owner.retain&&live())) || (owner.stopping&&!live())){
            retire(owner,output);release(state,owner.synchronization);it=state.owners.erase(it);continue;}
        for(unsigned n=0;n<owner.tracks.size();++n){auto& track=owner.tracks[n];auto* primary=voice(track,track.phase.primary);
            if(primary && !owner.stopping){const auto& d=output.metadata(primary->sound,primary->range);
                track.control=source_loop_track_gain_control_tu15(track.control,Bytes(owner.location).f32(4),Bytes(d.playback).f32(0xc),elapsed);
                const auto pitch=source_sound_control_pitch(d,primary->base_pitch,track.control);
                const auto selected=source_sound_pitch_range(d.pitch_ranges,std::int16_t(primary->range),pitch);
                if(selected<0)throw FormatError("Native synchronized frame lost its authored pitch range");
                auto seed=state.synchronization.at(owner.synchronization).seed;Playback playback(output,owner,track,n,seed);
                if(Bytes(d.sound).u16(0)&0x2000u){
                    const auto current_sound=primary->sound;
                    for(unsigned range=0;range<d.pitch_ranges.size();++range){std::array<std::int16_t,5> interval{};
                        for(unsigned k=0;k<5;++k)interval[k]=std::bit_cast<std::int16_t>(Bytes(d.pitch_ranges[range].interval).u16(k*2));
                        const auto layer=source_loop_pitch_layer_tu15(interval,pitch,true);
                        auto existing=std::uint64_t{};
                        for(const auto id:ring(track,track.phase.primary)){const auto* v=voice(track,id);
                            if(v->sound==current_sound && v->range==range && v->mode==2){existing=id;break;}}
                        if(!layer.active && existing){output.retire(existing);unlink(track,existing);existing=0;}
                        if(layer.active && !existing){existing=playback.create(current_sound,range,2);
                            if(existing && range==unsigned(selected))track.phase.primary=existing;}
                    }
                }else if(primary->range!=unsigned(selected)){
                    playback.initial_fade=.75f;
                    const auto previous=track.phase.primary;auto next=playback.find(primary->sound,unsigned(selected));
                    if(!next)next=playback.create(primary->sound,unsigned(selected),2);
                    if(next){playback.crossfade(next,previous,.75f);track.phase.primary=next;}
                }
            }
            // Source222E10 updates real streams, then queues the selected next
            //packet. Detached entry impulses remain separate voice rings.
            for(auto& v:track.voices){if(!output.active(v.id))continue;
                output.parameters(v.id,parameters(output,owner,track,n,v),position(owner));
                if(v.mode==0 || !output.accepts_packet(v.id))continue;
                auto sound=v.sound;auto range=v.range;
                const auto& authored=owner.definition.tracks[n];
                const std::array phases{authored.start,authored.loop,authored.end,authored.alternate_transition_in,
                    authored.alternate,authored.alternate_transition_out,authored.alternate_end};
                const bool transition=v.id==track.phase.primary && track.phase.phase!=track.phase.pending && phases[track.phase.pending]!=none;
                if(transition){
                    sound=phases[track.phase.pending];
                    const auto& d=output.metadata(sound,0);const auto pitch=source_sound_control_pitch(d,v.base_pitch,track.control);
                    const auto selected=source_sound_pitch_range(d.pitch_ranges,-1,pitch);
                    if(selected<0)throw FormatError("Native phase transition lost its authored range");range=unsigned(selected);
                }else if(v.mode==3)continue;
                auto& seed=v.id==track.phase.primary?track.seed:v.seed;
                auto& used=v.id==track.phase.primary?track.used:v.used;
                const auto permutation=select(output,used,sound,range,seed,transition);
                if(!output.queue(v.id,sound,range,permutation))throw FormatError("Native phase lost its available packet slot");
                if(transition){v.sound=sound;v.range=range;track.phase.phase=track.phase.pending;}
            }
        }
        owner.seen=false;++it;
    }
}
}
