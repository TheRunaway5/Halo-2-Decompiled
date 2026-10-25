#include "halo2/source_loop_track_update_tu15.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
using Json=nlohmann::json;
SourceLoopNativeVoiceTu15 native(unsigned value){return value?0x100000000ull+value:0;}
unsigned ordinal(SourceLoopNativeVoiceTu15 value){
    if(!value)return 0;
    if(value<=0x100000000ull || value>0x100000008ull)throw std::runtime_error("Native voice identity narrowed");
    return unsigned(value-0x100000000ull);
}
struct Playback final:SourceLoopTrackPlaybackTu15 {
    const Json& expected;std::size_t cursor{};bool output{};
    std::map<SourceLoopNativeVoiceTu15,std::uint32_t> definitions;
    explicit Playback(const Json& ports,std::uint32_t sound,bool has_output):expected(ports),output(has_output){definitions[native(1)]=sound;}
    unsigned invoke(Json action,bool result=false){
        if(cursor>=expected.size())throw std::runtime_error("Unexpected playback port "+action.dump());
        const auto& row=expected.at(cursor++);unsigned value{};
        if(result){value=row.back().get<unsigned>();action.push_back(value);}
        if(row!=action)throw std::runtime_error("Expected port "+row.dump()+" got "+action.dump());
        return value;
    }
    std::uint32_t definition(SourceLoopNativeVoiceTu15 id)const override{return definitions.at(id);}
    bool has_output(SourceLoopNativeVoiceTu15 id)const override{ordinal(id);return id!=native(1)||output;}
    SourceLoopNativeVoiceTu15 find(std::uint32_t sound,std::optional<unsigned> pitch)override{
        return native(invoke(Json::array({"find",sound,pitch?std::int64_t(*pitch):-1}),true));
    }
    SourceLoopNativeVoiceTu15 create(std::uint32_t sound,std::optional<unsigned> pitch,unsigned state)override{
        const auto id=native(invoke(Json::array({"create",sound,pitch?std::int64_t(*pitch):-1,state}),true));
        if(id)definitions[id]=sound;return id;
    }
    unsigned pitch_range(std::uint32_t sound,float control)override{
        return invoke(Json::array({"pitch_range",sound,std::bit_cast<std::uint32_t>(control)}),true);
    }
    void fade_out(SourceLoopNativeVoiceTu15 id,float time)override{invoke(Json::array({"fade_out",ordinal(id),std::bit_cast<std::uint32_t>(time)}));}
    void fade_out_linear(SourceLoopNativeVoiceTu15 id,float time)override{invoke(Json::array({"fade_out_linear",ordinal(id),std::bit_cast<std::uint32_t>(time)}));}
    void fade_in(SourceLoopNativeVoiceTu15 id,float time)override{invoke(Json::array({"fade_in",ordinal(id),std::bit_cast<std::uint32_t>(time)}));}
    void crossfade(SourceLoopNativeVoiceTu15 next,SourceLoopNativeVoiceTu15 previous,float time)override{
        invoke(Json::array({"crossfade",ordinal(next),ordinal(previous),std::bit_cast<std::uint32_t>(time)}));
    }
    void stopping(SourceLoopNativeVoiceTu15 id)override{invoke(Json::array({"stopping",ordinal(id)}));}
};
}
int main(int argc,char**argv)try{
    if(argc!=2)throw std::runtime_error("Supply complete original track decision fixture");
    std::ifstream file(argv[1]);const auto fixture=Json::parse(file);
    if(fixture.at("original_xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
        throw std::runtime_error("Track decision authority differs");
    unsigned failures{},index{};
    for(const auto& row:fixture.at("cases"))try{
        const auto& i=row.at("input");LoopingSound::Track authored;authored.flags=i.at(1);
        std::array<std::uint32_t*,7> sounds{&authored.start,&authored.loop,&authored.end,
            &authored.alternate_transition_in,&authored.alternate,&authored.alternate_transition_out,&authored.alternate_end};
        for(unsigned n=0;n<7;++n)*sounds[n]=i.at(0).get<unsigned>()&(1u<<n)?0x76540000u+n:0xffffffffu;
        authored.fade_in=1.5f;authored.fade_out=.2f;authored.alternate_crossfade=.75f;authored.alternate_fade_out=.3f;
        SourceLoopTrackStateTu15 state{native(i.at(7).get<bool>()?1:0),i.at(2),i.at(3)};
        const SourceLoopTrackRequestTu15 request{i.at(4),i.at(6),i.at(5),i.at(11),i.at(12),i.at(10),
            std::bit_cast<float>(i.at(13).get<std::uint32_t>()),std::bit_cast<float>(i.at(14).get<std::uint32_t>())};
        Playback playback(row.at("ports"),i.at(8),i.at(9));
        source_loop_track_update_tu15(authored,request,state,playback);
        const auto output=Json::array({ordinal(state.primary),state.phase,state.pending});
        if(playback.cursor!=playback.expected.size() || output!=row.at("output"))
            throw std::runtime_error("Incomplete ports or state: expected "+row.dump()+" got "+output.dump());
        ++index;
    }catch(const std::exception&error){if(failures++<8)std::cerr<<"Track "<<index<<": "<<error.what()<<'\n';++index;}
    if(failures)throw std::runtime_error(std::to_string(failures)+" complete track decisions differ");
    std::cout<<"PASS "<<index<<" complete original track decisions with explicit component playback inputs\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
