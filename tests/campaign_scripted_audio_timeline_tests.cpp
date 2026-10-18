#include "halo2/multiplayer_audio.hpp"
#include "halo2/original_data.hpp"
#include "halo2/cache.hpp"
#include "halo2/campaign_audio_timeline.hpp"
#include <SDL.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>

using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=4 && argc!=5)throw FormatError("Supply TU15_XBE OPENING_MAP ORIGINAL_TIMING_TSV [--immediate]");
    for(int i=1;i!=4;++i)if(!std::filesystem::is_regular_file(argv[i]))return 77;
    const bool immediate=argc==5 && std::string(argv[4])=="--immediate";
    if(argc==5 && !immediate)throw FormatError("Unknown timing probe mode");
    SDL_setenv("SDL_AUDIODRIVER","dummy",1);
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    std::map<std::uint32_t,std::uint64_t> relative;
    unsigned script,tick;std::uint32_t sound;
    std::ifstream input(argv[3]);
    while(input>>std::dec>>script>>tick>>std::hex>>sound)relative.emplace(sound,tick);
    if(relative.size()!=49)throw FormatError("Incomplete original opening timing fixture");
    constexpr auto council=0xea12089du,truth=0xea13089eu;
    std::uint32_t seed=0x12345678u;
    std::vector<MultiplayerAudioSubmission> packets;
    MultiplayerAudio audio(cache,seed,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}},
        [&](const auto &packet){packets.push_back(packet);},&seed);
    audio.prepare(council);audio.prepare(truth);
    if(seed!=0x12345678u || !packets.empty() || audio.presentation_frames())
        throw FormatError("Preparing dialogue consumed its random owner or presentation clock");
    // The game starts after device creation; neither clock is required to be zero.
    CampaignAudioTimeline timeline;
    bool rejected=false;
    try {timeline.frame(0);}catch(const std::invalid_argument &){rejected=true;}
    if(!rejected)throw FormatError("Unbound campaign epoch accepted a cue");
    audio.render(117);
    constexpr std::uint64_t initial_tick=1900;
    timeline.anchor(initial_tick,audio.presentation_frames());
    std::map<std::uint32_t,std::uint64_t> due;
    for(const auto &[datum,offset]:relative) {
        const auto deadline=timeline.frame(initial_tick+offset);
        if(deadline!=117+offset*1600)throw FormatError("Original cue tick lost its actual output epoch");
        due.emplace(datum,deadline);
    }
    const auto first=immediate?audio.play_impulse(council):audio.schedule_impulse(council,due.at(council));
    const auto second=immediate?audio.play_impulse(truth):audio.schedule_impulse(truth,due.at(truth));
    if(!first || !second || packets.size()!=2 || packets[0].sound!=council || packets[1].sound!=truth)
        throw FormatError("Actual authored Prophet PCM packet missing");
    if(audio.playback_position(*first).first_device_frame || audio.playback_position(*second).first_device_frame)
        throw FormatError("Packet submission was reported as PCM consumption");
    const auto final=due.at(truth)+32;
    bool paused=false;
    while(audio.presentation_frames()<final) {
        const auto count=std::min<std::uint64_t>(733,final-audio.presentation_frames());
        audio.render(count);
        if(!paused && audio.presentation_frames()>due.at(council)+1600) {
            const auto presentation=audio.presentation_frames(),device=audio.mixed_frames();
            const auto source=audio.playback_position(*first).source_frames;
            audio.paused(true);
            const auto silence=audio.render(4800);
            if(std::ranges::any_of(silence,[](float v){return v!=0;}) ||
                audio.presentation_frames()!=presentation || audio.mixed_frames()!=device+4800 ||
                audio.playback_position(*first).source_frames!=source)
                throw FormatError("Native cinematic pause advanced a future cue or active PCM");
            audio.paused(false);paused=true;
        }
    }
    const auto a=audio.playback_position(*first),b=audio.playback_position(*second);
    if(a.first_presentation_frame!=due.at(council) || b.first_presentation_frame!=due.at(truth) ||
        *b.first_presentation_frame-*a.first_presentation_frame!=337600 ||
        *b.first_device_frame-*a.first_device_frame!=342400)
        throw FormatError("Consumed authored Prophet gap differs from the original 211 ticks plus explicit pause");
    if(!a.source_frames || b.source_frames!=32)
        throw FormatError("Actual decoded dialogue did not expose consumed source progress");
    audio.stop();
    if(audio.voices())throw FormatError("Stopped timeline retained a dialogue voice");
    const auto future=audio.schedule_impulse(council,audio.presentation_frames()+7);
    if(!future)throw FormatError("Subsequent dialogue request missing");
    audio.stop_voice(*future);
    const auto silence=audio.render(16);
    if(audio.voice_active(*future) || std::ranges::any_of(silence,[](float v){return v!=0;}))
        throw FormatError("Canceled future authored dialogue reached output");
    // Restoring source state cannot rewind a device. Rebind only at that event.
    timeline.anchor(40,audio.presentation_frames());
    const auto restored_frame=timeline.frame(40);
    const auto late=audio.schedule_impulse(council,timeline.frame(41));
    if(!late)throw FormatError("Restored epoch lost its dialogue");
    audio.render(1601);
    if(audio.playback_position(*late).first_presentation_frame!=restored_frame+1600)
        throw FormatError("Restored source tick did not use its current output epoch");
    audio.stop();
    const auto current=audio.presentation_frames();
    const auto overdue=audio.schedule_impulse(council,timeline.frame(40));
    audio.render(1);
    if(!overdue || audio.playback_position(*overdue).first_presentation_frame!=current ||
        audio.playback_position(*overdue).source_frames!=1 || timeline.frame(40)!=restored_frame)
        throw FormatError("Late dialogue trimmed PCM or rebased the source/device epoch");
    rejected=false;
    try {timeline.frame(39);}catch(const std::invalid_argument &){rejected=true;}
    if(!rejected)throw FormatError("Previous source epoch accepted a cue");
    timeline.clear();
    if(timeline.anchored())throw FormatError("Restart retained an old epoch");
    rejected=false;
    try {timeline.frame(40);}catch(const std::invalid_argument &){rejected=true;}
    if(!rejected)throw FormatError("Cleared source epoch accepted a cue");
    timeline.anchor(0,std::numeric_limits<std::uint64_t>::max()-1599);
    rejected=false;
    try {timeline.frame(1);}catch(const std::overflow_error &){rejected=true;}
    if(!rejected)throw FormatError("Campaign output deadline wrapped");
    std::cout<<"PASS actual Prophet PCM: 49 original cue ticks mapped from a nonzero epoch, exact "
                 "337600-sample gap, pause, restored epoch, late PCM, cancellation and overflow\n";
    std::cout<<"LIMIT adapter deadlines do not establish live request lead or final Xbox mix\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
