#include "halo2/source_unit_speech_tu15.hpp"
#include "halo2/source_campaign_model.hpp"
#include "halo2/source_campaign_cinematic_tu15.hpp"
#include "halo2/source_campaign_camera_tu15.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/original_data.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <unistd.h>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc<3 || argc>5)throw FormatError("Supply TU15_XBE OPENING_MAP [--armorer|--chief] [--no-speech]");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    bool baseline=false,armorer=false,chief=false;
    for(int i=3;i<argc;++i) {
        if(std::string(argv[i])=="--no-speech")baseline=true;
        else if(std::string(argv[i])=="--armorer")armorer=true;
        else if(std::string(argv[i])=="--chief")chief=true;
        else throw FormatError("Unknown speech probe mode");
    }
    if(armorer && chief)throw FormatError("Select one authored speech placement");
    const bool human=armorer||chief;
    const unsigned anchor_name=human?23:22,speaker_name=chief?52:armorer?53:56;
    SDL_setenv("SDL_AUDIODRIVER","dummy",1);
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    const auto placements=[&] {
        const auto c=cache.load_content(0,false);std::map<unsigned,ScenarioObject> out;
        for(auto name:{anchor_name,speaker_name}) {
            const auto &n=c.object_names.at(name);
            const auto p=std::ranges::find_if(c.objects,[&](const auto &p){return p.type==n.type && p.placement==n.placement;});
            if(p==c.objects.end())throw FormatError("Authored speech placement unavailable");out.emplace(name,*p);
        }
        return out;
    }();
    auto path=(std::filesystem::temp_directory_path()/"halo2-source-speech-XXXXXX").string();
    if(!::mkdtemp(path.data()))throw FormatError("Source speech storage unavailable");
    struct Cleanup{std::string path;~Cleanup(){std::error_code e;std::filesystem::remove_all(path,e);}}cleanup{path};
    MultiplayerObjectRuntime objects(argv[1],cache,0,path);auto &w=objects.tu15_world();auto &m=w.memory();
    w.initialize_map();w.configure_campaign_session(1,1,30,0x12345678);
    source_campaign_cinematic_control_tu15(w,true);source_campaign_camera_control_tu15(w,true);
    const auto create=[&](unsigned name) {
        const auto object=objects.create_scenario_object(placements.at(name),SourceScenarioBirth::runtime);
        if(object==null_datum)throw FormatError("Original speech object birth returned NONE");
        source_campaign_cinematic_lod_tu15(w,object,true);return object;
    };
    const auto anchor=create(anchor_name),speaker=create(speaker_name);
    if(!source_campaign_cinematic_animation_tu15(w,speaker,chief?0xea2e08b9u:armorer?0xea2f08bau:0xe9ec0877u,
        chief?0x08001d45u:armorer?0x10001d46u:0x09001cefu,anchor,true,false,false))
        throw FormatError("Original character graph start failed");
    const auto body=objects.source_object_body(speaker);
    const auto record=body+std::bit_cast<std::int16_t>(Bytes(m.view(body+0x33eu,2)).u16(0));
    const auto speech_channel=std::bit_cast<std::int16_t>(Bytes(m.view(record+0x36,2)).u16(0));
    if((speech_channel==-1)!=chief)
        throw FormatError("Original character graph has an unexpected speech channel");
    // Original c01_1040_qtm belongs to master_guns (name53), delay173;
    // c01_1050_mas belongs to chief (name52), delay396. Chief's actual graph
    // has no facial speech channel. Do not borrow the Armorer's graph for it.
    const unsigned delay=chief?396:armorer?173:0;
    for(unsigned tick=0;tick<delay;++tick)w.step_objects();
    const auto pose=[&] {
        w.begin_model_frame();w.begin_model_view();
        const auto collector=objects.build_world_model(speaker,0,1,false,0,0);
        const auto nodes=objects.rendered_world_nodes(speaker);
        if(!nodes)throw FormatError("Actual model builder omitted the character render pose");
        if(human) {
            // The original builder already ran BF640 and the failing facial
            // animation lookup. Keep this human fixture within the unchanged
            // source-test budget by comparing its retained WORLD hierarchy;
            // final vertex expansion is exercised by graphics playback.
            const auto bytes=std::as_bytes(std::span(*nodes));
            return std::vector<std::byte>(bytes.begin(),bytes.end());
        }
        SourceCampaignModel geometry(cache,placements.at(speaker_name));
        const auto vertices=geometry.vertices(speaker,collector,*nodes);
        const auto bytes=std::as_bytes(std::span(vertices));
        return std::vector<std::byte>(bytes.begin(),bytes.end());
    };
    const auto initial=pose();
    MultiplayerAudioSubmission selected;
    MultiplayerAudio audio(cache,0x12345678,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}},
        [&](const auto &packet){selected=packet;});
    const auto voice=audio.play_impulse(chief?0xea2d08b8u:armorer?0xea2c08b7u:0xea12089du,1,MultiplayerAudioKind::Impulse,speaker);
    if(!voice || selected.owner!=speaker)throw FormatError("Actual character PCM did not retain its native speaker");
    bool controlled=false,moved=false;
    for(unsigned tick=0;tick<24 && !moved;++tick) {
        audio.render(1600);
        const auto position=audio.playback_position(*voice);
        const auto seconds=float(position.source_frames/48000.0);
        if(!baseline)source_unit_speech_tu15(w,speaker,selected.sound,selected.pitch_range,selected.permutation,seconds);
        // Whole original unit facial phase, without advancing the body's
        // authored cinematic track. This isolates speech in the render pose.
        auto phase=w.prepare_source_call();phase.gp[0]=speaker;w.run_actual(0x110350u,phase);
        const auto controls=m.view(record+0x50,12);
        controlled|=std::ranges::any_of(controls,[](std::byte value){return value!=std::byte{};});
        const auto current=pose();
        moved=controlled && current!=initial;
    }
    if(chief) {
        if(controlled || std::bit_cast<std::int16_t>(Bytes(m.view(record+0x36,2)).u16(0))!=-1)
            throw FormatError("Chief acquired facial controls absent from his original graph");
    } else if(!controlled || !moved)throw FormatError("Actual character speech controls or rendered mouth remain frozen");
    if(Bytes(m.view(record+0x38,4)).u32(0)!=selected.sound)
        throw FormatError("Original unit speech owner did not retain the actual sound tag");
    source_unit_speech_stop_tu15(w,speaker,selected.sound+1);
    if(Bytes(m.view(record+0x38,4)).u32(0)!=selected.sound)
        throw FormatError("Retiring another sound cleared the current speech tag");
    audio.stop_voice(*voice);source_unit_speech_stop_tu15(w,speaker,selected.sound);
    if(Bytes(m.view(record+0x38,4)).u32(0)!=null_datum)
        throw FormatError("Completed native speech retained its original unit tag");
    for(unsigned tick=0;tick<90;++tick) {
        auto phase=w.prepare_source_call();phase.gp[0]=speaker;w.run_actual(0x110350u,phase);
    }
    if(std::ranges::any_of(m.view(record+0x50,12),[](std::byte value){return value!=std::byte{};}))
        throw FormatError("Original speech controls did not decay after native retirement");
    if(chief)std::cout<<"PASS Chief's actual cue and graph retain the original absent facial channel\n";
    else std::cout<<"PASS actual native character PCM progress drives original facial controls and changes source-selected "
        <<(armorer?"rendered WORLD nodes":"rendered vertices")<<'\n';
    std::cout<<"LIMIT declared speech/unit-frame inputs; full cinematic speaker binding, GPU pixels and audible mix remain separate\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
