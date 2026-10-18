#include "halo2/multiplayer_audio.hpp"
#include "halo2/original_data.hpp"
#include "halo2/cache.hpp"
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=3 && argc!=4)throw FormatError("Supply TU15_XBE OPENING_MAP [--freeze-emitter]");
    for(int i=1;i<3;++i)if(!std::filesystem::is_regular_file(argv[i]))return 77;
    const bool frozen=argc==4 && std::string(argv[3])=="--freeze-emitter";
    if(argc==4 && !frozen)throw FormatError("Unknown emitter control");
    SDL_setenv("SDL_AUDIODRIVER","dummy",1);
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    const auto cue=std::ranges::find_if(cache.tags(),[](const auto &tag){
        return tag.group=="snd!" && tag.name.ends_with("\\x01_0030_der");
    });
    if(cue==cache.tags().end())throw FormatError("Actual Arbiter Pillar cue unavailable");
    std::uint32_t random=0x12345678u;unsigned packets{};
    MultiplayerAudio audio(cache,random,{SdlSoundPlatformConfiguration::Delivery::Manual,64,{}},
        [&](const auto &){++packets;},&random);
    const Vec3 listener{};audio.listeners(std::span(&listener,1));
    const auto voice=audio.play_impulse_at(cue->datum,{17.5f,0,0},1,MultiplayerAudioKind::Impulse,0xe1230042u);
    if(!voice || packets!=1)throw FormatError("Actual spatial Arbiter PCM unavailable");
    const auto selected_random=random;const auto far=audio.voice_parameters(*voice);
    audio.render(64);const auto before=audio.playback_position(*voice);
    if(!frozen)audio.emitter(*voice,{1,0,0});
    const auto near=audio.voice_parameters(*voice);const auto moved=audio.playback_position(*voice);
    if(near.volume_hundredths_db<=far.volume_hundredths_db ||
       moved.source_frames!=before.source_frames || moved.output_frames!=before.output_frames ||
       moved.first_device_frame!=before.first_device_frame ||
       near.relative_rate_bits!=far.relative_rate_bits || random!=selected_random || packets!=1)
        throw FormatError("Moving speaker did not update distance gain while preserving its selected PCM/RNG/progress");
    const auto reject=[&](auto action){bool failed{};try {action();}catch(const std::exception &){failed=true;}
        if(!failed)throw FormatError("Invalid emitter update was admitted");};
    reject([&]{audio.emitter(*voice,{std::numeric_limits<float>::quiet_NaN(),0,0});});
    if(audio.voice_parameters(*voice).volume_hundredths_db!=near.volume_hundredths_db)
        throw FormatError("Rejected nonfinite point changed a live voice");
    audio.emitter(*voice,{25,0,0});
    if(audio.voice_parameters(*voice).volume_hundredths_db>=far.volume_hundredths_db)
        throw FormatError("Moving speaker outside its original range retained a nearby gain");
    audio.emitter(*voice,{1,0,0});audio.render(64);
    const auto after=audio.playback_position(*voice);
    if(after.output_frames!=before.output_frames+64 || after.source_frames<=before.source_frames ||
       after.first_device_frame!=before.first_device_frame || packets!=1 || random!=selected_random)
        throw FormatError("Emitter refresh restarted, reselected or stopped its PCM");
    const auto centered=audio.play_impulse(cue->datum);
    if(!centered)throw FormatError("Centered control unavailable");
    const auto center=audio.voice_parameters(*centered);const auto center_random=random;
    reject([&]{audio.emitter(*centered,{1,0,0});});
    if(audio.voice_parameters(*centered).volume_hundredths_db!=center.volume_hundredths_db || random!=center_random)
        throw FormatError("Rejected centered update changed playback");
    audio.stop_voice(*voice);reject([&]{audio.emitter(*voice,{1,0,0});});
    audio.stop();
    std::cout<<"PASS actual Arbiter PCM: moving emitter gain "<<far.volume_hundredths_db<<" -> "
        <<near.volume_hundredths_db<<", same voice/PCM/pitch/RNG/first sample; invalid and retired updates rejected\n";
    std::cout<<"LIMIT adapter spatial refresh; original live listener/emitter association, DSP and final cutscene mix remain separate\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
