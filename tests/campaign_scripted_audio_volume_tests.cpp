#include "halo2/multiplayer_audio.hpp"
#include "halo2/original_data.hpp"
#include "halo2/cache.hpp"
#include <iostream>
#include <SDL.h>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=3)throw FormatError("Supply TU15_XBE OPENING_MAP");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    SDL_setenv("SDL_AUDIODRIVER","dummy",1);
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    unsigned checks{};
    for(const auto &[name,volume]:std::vector<std::pair<std::string,int>>{
        {"x01_0050_pom",-300},{"x01_0200_pcc",-1500},{"x01_0300_pot",-300},
        {"x01_0310_cch",-6400},{"x01_0320_pot",-100},{"x01_0330_pot",0}}) {
        const auto tag=std::ranges::find_if(cache.tags(),[&](const auto &tag){return tag.group=="snd!" && tag.name.ends_with("\\"+name);});
        if(tag==cache.tags().end())throw FormatError("Authored opening volume cue unavailable");
        std::uint32_t shared_random=0x12345678u;unsigned packets{};
        MultiplayerAudio audio(cache,shared_random,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}},
            [&](const auto &){++packets;},&shared_random);
        audio.prepare(tag->datum);audio.prepare(tag->datum);
        if(shared_random!=0x12345678u || packets || audio.voices() || audio.mixed_frames())
            throw FormatError("Authored prediction consumed playback, variation or the manual audio clock");
        const auto voice=audio.play_impulse(tag->datum,1);
        if(volume<=-6400) {
            if(voice || audio.voices())throw FormatError("Authored silent chant was admitted as a full-volume voice");
            const auto silence=audio.render(256);
            if(std::ranges::any_of(silence,[](float v){return v!=0;}))throw FormatError("Authored silent chant reached native output");
        } else {
            if(!voice || audio.voice_parameters(*voice).volume_hundredths_db!=volume)
                throw FormatError("Opening scripted cue lost its authored driver volume: "+name);
            audio.paused(true);
            if(!audio.voice_parameters(*voice).paused)throw FormatError("Scripted voice did not retain pause");
            audio.paused(false);
            if(audio.voice_parameters(*voice).paused)throw FormatError("Scripted voice did not resume");
        }
        std::cout<<name<<" volume_hundredths_db="<<volume<<" admitted="<<bool(voice)<<'\n';++checks;
    }
    if(checks!=6)throw FormatError("Incomplete opening volume coverage");
    std::cout<<"PASS authored Mercy/crowd/Truth playback, class/control and permutation gains through native driver parameters; silent chant produces no PCM\n";
    std::cout<<"LIMIT centered volume inputs; no Xbox DSP/HRTF or complete recorded-mix equivalence\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
