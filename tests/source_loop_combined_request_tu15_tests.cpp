#include "halo2/source_loop_phased_output_tu15.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
using Json=nlohmann::json;
void need(bool value,const char* reason){if(!value)throw FormatError(reason);}
std::array<std::byte,68> decode(const std::string& text){
    need(text.size()==136,"Original location extent differs");std::array<std::byte,68> result{};
    for(unsigned n=0;n<68;++n)result[n]=std::byte(std::stoul(text.substr(2*n,2),nullptr,16));return result;
}
// Deliberately no fake voice or admission. These parent-component cases have
// no tracks; every playback/metadata operation must remain unreachable.
struct ParentInputs final:SourceLoopPhasedOutputTu15 {
    bool available=true;unsigned queries{};std::uint32_t seed=0x17651u,maximum{};
    std::array<std::byte,68> location{};
    std::optional<SourceLoopCombinedListenerInputTu15> listener;
    void require_combined_output()const override{if(!available)SourceLoopPhasedOutputTu15::require_combined_output();}
    std::optional<SourceLoopCombinedListenerInputTu15> combined_listener(std::span<const std::byte,68> value,std::uint32_t range)override{
        need(std::equal(value.begin(),value.end(),location.begin()),"Combined listener received a different source location");
        need(range==maximum,"Combined listener received a different original range");++queries;return listener;
    }
    const SoundVariationDefinition& metadata(std::uint32_t,unsigned)override{throw FormatError("Parent-only test attempted metadata playback");}
    bool single_chunk(std::uint32_t,unsigned,unsigned)override{throw FormatError("Parent-only test attempted a chunk query");}
    bool active(std::uint64_t)const override{throw FormatError("Parent-only test fabricated a native voice");}
    std::uint64_t play(const MultiplayerLoopPacketTu15&)override{throw FormatError("Parent-only test attempted PCM playback");}
    void parameters(std::uint64_t,const MultiplayerLoopPacketParametersTu15&,std::optional<Vec3>)override{throw FormatError("Parent-only test attempted voice parameters");}
    bool accepts_packet(std::uint64_t)const override{throw FormatError("Parent-only test attempted packet admission");}
    bool queue(std::uint64_t,std::uint32_t,unsigned,unsigned)override{throw FormatError("Parent-only test attempted packet delivery");}
    void fade(std::uint64_t,float,bool,std::uint16_t)override{throw FormatError("Parent-only test attempted a voice fade");}
    void retire(std::uint64_t)override{throw FormatError("Parent-only test attempted voice retirement");}
    std::uint32_t fresh_secondary()override{throw FormatError("Combined parent requested synchronization randomness");}
    std::uint32_t& common_secondary()override{return seed;}
    unsigned common_permutation(std::uint32_t,unsigned,std::uint32_t&)override{throw FormatError("Parent-only test attempted permutation selection");}
};
}
int main(int argc,char** argv)try{
    need(argc==2,"Supply the independent original combined-request fixture");
    const auto fixture=Json::parse(std::ifstream(argv[1]));
    need(fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        fixture.at("keys").size()==256 && fixture.at("cases").size()==256 &&
        fixture.at("original_instruction_patches").empty() && fixture.at("substituted_calls").empty(),
        "Shared request reference authority/scope differs");
    for(const auto& row:fixture.at("keys")){
        SourceLoopSoundRequestTu15 request{};request.arguments[0]=row.at("datum");request.arguments[2]=row.at("requested");
        request.location[3]=std::byte(row.at("mode").get<unsigned>());
        LoopingSound definition;definition.flags=row.at("definition_flags");
        const auto key=source_loop_phased_key_tu15(request,definition);
        need(key.first==request.arguments[0] && key.second==row.at("key"),"Original definition-wide key differs");
    }
    unsigned index{},no_listener{},preserved{},resets{};
    for(const auto& row:fixture.at("cases")){
        SourceLoopSoundRequestTu15 request{};
        request.arguments={0x12340000u,row.at("source_flags"),0x33440001u,0xffffffffu,0xffffffffu,1,0,0};
        request.emitter_owner=0x33440001u;request.location=decode(row.at("request_location"));
        LoopingSound definition;definition.flags=0x80;definition.distance={std::bit_cast<float>(row.at("minimum").get<std::uint32_t>()),
            std::bit_cast<float>(row.at("maximum").get<std::uint32_t>())};
        SourceLoopPhasedStateTu15 state;state.frame=row.at("frame");
        SourceLoopPhasedOwnerTu15 owner;owner.definition=definition;owner.emitter=0x77880001u;
        owner.synchronization=0xabc10002u;owner.combined=true;owner.location=decode(row.at("old_location"));
        owner.last_frame=row.at("previous_frame");owner.combined_amplitudes=row.at("prior").get<std::array<std::uint32_t,4>>();
        const auto key=std::pair{request.arguments[0],request.arguments[0]|0x6000u};state.owners.emplace(key,owner);
        ParentInputs inputs;inputs.location=decode(row.at("expected_location"));inputs.maximum=row.at("selected_maximum");
        if(!row.at("listener_input").is_null())inputs.listener=SourceLoopCombinedListenerInputTu15{
            row.at("listener_input").at("point").get<std::array<std::uint32_t,3>>(),row.at("listener_input").at("distance")};
        else ++no_listener;
        source_loop_phased_request_tu15(state,inputs,request,definition);
        const auto& actual=state.owners.at(key);
        if(actual.location!=inputs.location || actual.source_flags!=row.at("expected_source_flags") ||
            actual.last_frame!=row.at("expected_frame") || actual.combined_amplitudes!=row.at("expected_amplitudes").get<std::array<std::uint32_t,4>>())
            throw FormatError("Combined parent differs from original request "+std::to_string(index));
        need(state.owners.size()==1 && actual.emitter==owner.emitter && actual.synchronization==owner.synchronization,
            "Another emitter replaced or duplicated the shared parent");
        need(inputs.queries==1 && inputs.seed==0x17651u,"Parent request changed packet/RNG ownership");
        preserved+=(std::to_integer<unsigned>(request.location[0])&2u)!=0;resets+=owner.last_frame!=state.frame;
        // The same frame can accumulate another emitter's contribution. Its
        // object key/generation does not allocate another definition owner.
        request.arguments[2]^=0x80010000u;request.emitter_owner^=0x80010000u;
        source_loop_phased_request_tu15(state,inputs,request,definition);
        need(state.owners.size()==1,"Second emitter duplicated a combined owner");
        // Frame completion advances the byte counter, including255->0.
        const auto frame=state.frame;source_loop_phased_frame_tu15(state,inputs,1.f/30.f);
        need(state.frame==std::uint8_t(frame+1u),"Combined frame counter failed to wrap");
        ++index;
    }
    // Missing four-bin playback fails before a group, listener query or RNG
    // mutation. Stopping an absent group still needs no playback backend.
    ParentInputs closed;closed.available=false;SourceLoopPhasedStateTu15 state;
    LoopingSound definition;definition.flags=0x80;
    SourceLoopSoundRequestTu15 request{};request.arguments={0x12340000u,3,0x12340001u,0xffffffffu,0xffffffffu,0,0,0};
    request.emitter_owner=0x12340001u;request.location[3]=std::byte{1};
    bool refused=false;try{source_loop_phased_request_tu15(state,closed,request,definition);}catch(const FormatError&){refused=true;}
    need(refused && state.owners.empty() && !closed.queries && closed.seed==0x17651u,"Combined preflight changed a closed native backend");
    request.arguments[5]=2;source_loop_phased_request_tu15(state,closed,request,definition);
    need(state.owners.empty(),"Absent combined stop created a group");
    need(no_listener && preserved && resets,"Combined reference controls are incomplete");
    std::cout<<"PASS 256 original shared keys and "<<index<<" original parent updates; "<<no_listener
        <<" no-listener cases, "<<preserved<<" preserved locations, "<<resets
        <<" frame resets; cross-emitter sharing and closed-backend preflight; no voice/PCM/admission supplied\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
