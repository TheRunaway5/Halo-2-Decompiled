#include "halo2/source_loop_phased_output_tu15.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
int main(int argc,char** argv)try{
    if(argc!=2)throw FormatError("Supply original loop voice-ring fixture");
    const auto fixture=nlohmann::json::parse(std::ifstream(argv[1]));
    if(fixture.at("original_xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
        throw FormatError("Voice-ring source authority differs");
    const auto identity=[](int index)->std::uint64_t{return index<0?0:0x100000001ull+std::uint64_t(index)*17;};
    const auto ordinal=[](std::uint64_t id)->int{return !id?-1:int((id-0x100000001ull)/17);};
    unsigned index=0;
    for(const auto& row:fixture.at("cases")){
        SourceLoopPhasedTrackTu15 track;track.phase.primary=identity(row.at("input_primary"));
        for(const auto& input:row.at("input")){SourceLoopPhasedVoiceTu15 voice;
            voice.id=identity(input.at(0));voice.next=identity(input.at(1));voice.previous=identity(input.at(2));
            track.voices.push_back(voice);}
        const auto selected=identity(row.at("selected"));
        if(row.at("remove"))source_loop_phased_retire_voice_tu15(track,selected);
        else {SourceLoopPhasedVoiceTu15 voice;voice.id=voice.next=voice.previous=selected;
            source_loop_phased_insert_voice_tu15(track,voice);}
        nlohmann::json after=nlohmann::json::array();
        for(const auto& voice:track.voices)after.push_back({ordinal(voice.id),ordinal(voice.next),ordinal(voice.previous)});
        if(after!=row.at("output") || ordinal(track.phase.primary)!=row.at("output_primary") || track.voices.size()!=row.at("output_count"))
            throw FormatError("Original voice ring differs at case "+std::to_string(index));
        ++index;
    }
    std::cout<<"PASS "<<index<<" original insert/retire rings, detached entries, primary replacement and native64 identities\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
