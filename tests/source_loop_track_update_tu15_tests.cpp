#include "halo2/source_loop_track_update_tu15.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
using Json=nlohmann::json;
constexpr std::uint64_t native_voice=0x100000001ull;
struct Prefix {};
struct Playback final:SourceLoopTrackPlaybackTu15 {
    const Json& input;
    Json action=Json::array({"none"});
    explicit Playback(const Json&value):input(value){}
    static unsigned voice(SourceLoopNativeVoiceTu15 id) {
        if(id && id!=native_voice)throw std::runtime_error("Planner narrowed or invented a native voice");
        return id?1:0;
    }
    std::uint32_t definition(SourceLoopNativeVoiceTu15 id)const override {voice(id);return input.at("voice_definition");}
    bool has_output(SourceLoopNativeVoiceTu15 id)const override {voice(id);return input.at("has_output");}
    SourceLoopNativeVoiceTu15 find(std::uint32_t sound,std::optional<unsigned> range)override {
        action=Json::array({"find",sound,range?std::int64_t(*range):-1});throw Prefix{};
    }
    SourceLoopNativeVoiceTu15 create(std::uint32_t sound,std::optional<unsigned> range,unsigned state)override {
        action=Json::array({"create",sound,range?std::int64_t(*range):-1,state});throw Prefix{};
    }
    unsigned pitch_range(std::uint32_t sound,float control)override {
        action=Json::array({"pitch_range",sound,std::bit_cast<std::uint32_t>(control)});throw Prefix{};
    }
    void fade_out(SourceLoopNativeVoiceTu15 id,float time)override {
        action=Json::array({"fade_out",voice(id),std::bit_cast<std::uint32_t>(time)});throw Prefix{};
    }
    void fade_out_linear(SourceLoopNativeVoiceTu15 id,float time)override {
        action=Json::array({"fade_out_linear",voice(id),std::bit_cast<std::uint32_t>(time)});throw Prefix{};
    }
    void fade_in(SourceLoopNativeVoiceTu15 id,float time)override {
        action=Json::array({"fade_in",voice(id),std::bit_cast<std::uint32_t>(time)});throw Prefix{};
    }
    void crossfade(SourceLoopNativeVoiceTu15 next,SourceLoopNativeVoiceTu15 previous,float time)override {
        action=Json::array({"crossfade",voice(next),voice(previous),std::bit_cast<std::uint32_t>(time)});throw Prefix{};
    }
    void stopping(SourceLoopNativeVoiceTu15 id)override {action=Json::array({"stopping",voice(id)});throw Prefix{};}
};
}
int main(int argc,char**argv)try {
    if(argc!=2)throw std::runtime_error("Supply the independent original track-decision prefix fixture");
    std::ifstream file(argv[1]);const auto fixture=Json::parse(file);
    if(fixture.at("original_xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
        throw std::runtime_error("Track-update authority differs");
    unsigned failures{},index{};
    for(const auto& row:fixture.at("cases")) {
        LoopingSound::Track authored;authored.flags=row.at("flags");
        std::array<std::uint32_t*,7> sounds{&authored.start,&authored.loop,&authored.end,
            &authored.alternate_transition_in,&authored.alternate,&authored.alternate_transition_out,&authored.alternate_end};
        const auto mask=row.at("mask").get<unsigned>();
        for(unsigned n=0;n<7;++n)*sounds[n]=mask&(1u<<n)?0x76540000u+n:0xffffffffu;
        authored.fade_in=1.5f;authored.fade_out=.2f;authored.alternate_crossfade=.75f;authored.alternate_fade_out=.3f;
        SourceLoopTrackStateTu15 state{row.at("has_primary").get<bool>()?native_voice:0,
            row.at("phase").get<std::uint8_t>(),row.at("pending").get<std::uint8_t>()};
        SourceLoopTrackRequestTu15 request{row.at("state"),row.at("created"),row.at("alternate"),
            row.at("stopped"),row.at("was_stopping"),row.at("definition_flags"),
            std::bit_cast<float>(row.at("fade_bits").get<std::uint32_t>()),
            std::bit_cast<float>(row.at("control_bits").get<std::uint32_t>())};
        Playback playback(row);
        try{source_loop_track_update_tu15(authored,request,state,playback);}catch(const Prefix&){}
        if(playback.action!=row.at("action") || bool(state.primary)!=row.at("out_primary").get<bool>() ||
            state.phase!=row.at("out_phase").get<unsigned>() || state.pending!=row.at("out_pending").get<unsigned>()) {
            if(failures++<8)std::cerr<<"Track prefix "<<index<<" expected "<<row.dump()<<" actual action="
                <<playback.action<<" primary="<<bool(state.primary)<<" phase="<<unsigned(state.phase)<<" pending="<<unsigned(state.pending)<<'\n';
        }
        ++index;
    }
    if(failures)throw std::runtime_error(std::to_string(failures)+" track prefixes differ from original CPU execution");
    std::cout<<"PASS "<<index<<" original pre-playback track-decision prefixes\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
