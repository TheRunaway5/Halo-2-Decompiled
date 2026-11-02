#include "halo2/multiplayer_audio.hpp"
#include "halo2/mcc_sound_metadata_publication.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string_view>
using namespace halo2;
namespace {
using Json=nlohmann::json;
void need(bool value,const char* message){if(!value)throw FormatError(message);}
void word(std::array<std::byte,68>& bytes,unsigned at,std::uint32_t value){
    for(unsigned n=0;n<4;++n)bytes[at+n]=std::byte(value>>(8*n));
}
}
int main(int argc,char** argv)try{
    if(argc!=4 && argc!=5)throw FormatError("Supply a prepared MCC level, destruction-loop inventory, report path and optional alternates selector");
    const bool alternates_only=argc==5;
    if(alternates_only && std::string_view(argv[4])!="alternates")throw FormatError("Unknown loop qualification selector");
    if(!*argv[1])return 77;
    need(std::getenv("HALO2_TEST_SCRATCH"),"Run destruction loop validation through tools/run_test.py");
    const auto report_path=std::filesystem::weakly_canonical(argv[3]);
    need(report_path.extension()==".json" && report_path!=std::filesystem::weakly_canonical(argv[2]),
        "Use a separate bounded JSON report path");
    const auto inventory=Json::parse(std::ifstream(argv[2]));
    need(inventory.at("passed").get<bool>() && inventory.at("loops").size()==7,"Require the seven authored destruction loops");
    auto cache=std::make_shared<const Cache>(argv[1]);need(cache->is_mcc(),"Loop playback requires MCC runtime content");
    const auto metadata=MccSoundMetadataPublication::create(cache);
    Json report{{"passed",false},{"cases",Json::array()},{"raw_audio_retained",false},
        {"scope","Declared spatial loop requests through the production native controller and manual SDL PCM; original source event timing and physical speaker output remain separate"},
        {"source_voice_or_admission_results_supplied",false},{"selection_seed_is_declared_component_input",true}};
    report["alternates_only"]=alternates_only;unsigned failures{};
    for(const auto& [name,expected]:inventory.at("loops").items()){
        if(alternates_only && expected.at("tracks")[0].at("refs").at("alternate").is_null())continue;
        Json row{{"loop",name},{"passed",false},{"packets",Json::array()}};
        try{
            const auto tag=std::ranges::find_if(cache->tags(),[&](const auto& t){return t.group=="lsnd" && t.name==name;});
            need(tag!=cache->tags().end(),"Prepared level lacks the actual destruction loop identity");
            const auto definition=cache->load_looping_sound(tag->datum);
            need(definition.tracks.size()==1 && expected.at("tracks").size()==1,"Destruction loop track shape changed");
            const auto& track=definition.tracks[0];unsigned normal_packets{},alternate_packets{};double peak{};
            need(definition.flags==expected.at("flags") && track.flags==expected.at("tracks")[0].at("flags"),
                "Destruction loop no longer matches its authenticated controller flags");
            const auto sound_name=[&](std::uint32_t datum)->Json{
                if(datum==0xffffffffu)return nullptr;
                const auto sound=cache->query_tag({.handle=TagHandle{datum}});
                need(sound && sound->group=="snd!","Destruction loop phase lost its resident sound owner");return sound->name;
            };
            need(sound_name(track.loop)==expected.at("tracks")[0].at("refs").at("loop") &&
                sound_name(track.alternate)==expected.at("tracks")[0].at("refs").at("alternate"),
                "Destruction loop no longer references its authenticated phase sounds");
            constexpr std::uint32_t emitter=0x12340001u;
            std::uint64_t mixed_hash=0xcbf29ce484222325ull;
            std::vector<std::array<std::uint32_t,3>> selected_packets;
            MultiplayerAudio audio(cache,0x5821b36du,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}},[&](const auto& packet){
                need(packet.owner==emitter && packet.voice && packet.frames && packet.position,
                    "Destruction loop packet lost its real native voice or declared spatial owner");
                if(packet.sound==track.loop)++normal_packets;
                else if(packet.sound==track.alternate)++alternate_packets;
                else throw FormatError("Destruction loop played a sound outside its authored track");
                selected_packets.push_back({packet.sound,packet.pitch_range,packet.permutation});
                if(row["packets"].size()<128)row["packets"].push_back({{"sound",packet.sound},{"voice",packet.voice},
                    {"pitch_range",packet.pitch_range},{"permutation",packet.permutation},{"frames",packet.frames}});
            });
            audio.enroll_continuation();audio.bind_native_loop_metadata(metadata);
            const std::array listeners{Vec3{0,0,0}};audio.listeners(listeners);
            SourceLoopSoundRequestTu15 request{};
            request.arguments={tag->datum,0,emitter,0xffffffffu,0xffffffffu,0,0,0};
            request.emitter_owner=emitter;request.location[3]=std::byte{1};word(request.location,4,std::bit_cast<std::uint32_t>(1.f));
            const auto tick=[&]{
                audio.object_loop_request(request);
                audio.object_loop_frame(1.f/30.f);
                for(float sample:audio.render(1600)){
                    need(std::isfinite(sample),"Destruction loop produced nonfinite PCM");peak=std::max(peak,std::abs(double(sample)));
                    mixed_hash=(mixed_hash^std::bit_cast<std::uint32_t>(sample))*0x100000001b3ull;
                }
                audio.update();
            };
            tick();request.arguments[5]=1;
            for(unsigned n=0;n<90;++n)tick();
            need(normal_packets && peak>0,"Authored normal destruction loop produced no audible native packet");
            row["normal_packets_before_alternate"]=normal_packets;
            if(track.alternate!=0xffffffffu){
                request.arguments[6]=1;for(unsigned n=0;n<180;++n)tick();
                need(alternate_packets,"Authored destruction alternate was not delivered");
                const auto checkpoint=audio.checkpoint();
                const auto before=normal_packets;request.arguments[6]=0;for(unsigned n=0;n<180;++n)tick();
                need(normal_packets>before,"Destruction loop did not return to its authored normal phase");
                // Rewind the actual native controller, range masks, common
                // random word, fades and queued PCM at an alternate boundary.
                auto replacement=audio.prepare_restore(checkpoint);
                {auto held=audio.hold_publication(replacement);need(audio.begin_commit(replacement,held) && audio.commit(replacement,held),
                    "Destruction alternate checkpoint did not publish");}
                const auto first_packet=selected_packets.size();
                mixed_hash=0xcbf29ce484222325ull;for(unsigned n=0;n<180;++n)tick();
                const auto expected_hash=mixed_hash;
                const std::vector<std::array<std::uint32_t,3>> expected_packets(selected_packets.begin()+first_packet,selected_packets.end());
                replacement=audio.prepare_restore(checkpoint);
                {auto held=audio.hold_publication(replacement);need(audio.begin_commit(replacement,held) && audio.commit(replacement,held),
                    "Repeated destruction alternate checkpoint did not publish");}
                const auto second_packet=selected_packets.size();
                mixed_hash=0xcbf29ce484222325ull;for(unsigned n=0;n<180;++n)tick();
                const std::vector<std::array<std::uint32_t,3>> actual_packets(selected_packets.begin()+second_packet,selected_packets.end());
                need(mixed_hash==expected_hash && actual_packets==expected_packets,
                    "Destruction alternate continuation changed actual PCM or permutation sequence");
                row["restored_suffix_pcm_hash"]=mixed_hash;row["restored_suffix_packet_sequence"]=actual_packets;
            }
            request.arguments[5]=2;for(unsigned n=0;n<180 && audio.voices();++n)tick();
            need(audio.voices()==0,"Stopped destruction loop retained a native PCM voice");
            request.arguments[0]=0xffffffffu;const auto packets=normal_packets+alternate_packets;tick();
            need(audio.voices()==0 && normal_packets+alternate_packets==packets,"Authored loop NONE created output");
            row["normal_packets"]=normal_packets;row["alternate_packets"]=alternate_packets;
            row["peak"]=peak;row["stopped_voices"]=audio.voices();
            if(track.alternate!=0xffffffffu){
                // With these actual zero-skip banks, seed5 proposes the same
                // first candidate for both owners. A shared no-repeat mask
                // must reject the second proposal before any frame refills.
                const auto selection=metadata.load(*cache,TagHandle{track.loop});
                need(selection.permutations.size()>=2 && std::ranges::all_of(selection.permutations,[](const auto& p){return Bytes(p).u16(2)==0;}),
                    "Authored shared-bank no-repeat control inputs changed");
                std::uint32_t common=5;std::vector<unsigned> selected;
                MultiplayerAudio paired(cache,5,{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}},[&](const auto& packet){
                    need(packet.sound==track.loop && packet.voice && packet.owner==emitter+selected.size(),
                        "Paired normal owners lost their authored phase or full emitter identity");selected.push_back(packet.permutation);
                },&common);
                paired.enroll_continuation();paired.bind_native_loop_metadata(metadata);paired.listeners(listeners);
                auto first=request;first.arguments[0]=tag->datum;first.arguments[5]=0;first.arguments[6]=0;
                auto second=first;second.arguments[2]=second.emitter_owner=emitter+1;
                paired.object_loop_request(first);paired.object_loop_request(second);
                need(selected.size()==2 && selected[0]!=selected[1],"Two destruction owners lost the original shared pitch-range no-repeat mask");
                need(common!=5,"Unsynchronized phase creation bypassed the actual common RNG owner");
                // Borrowed Effects RNG must keep its writer's restore gate.
                // Never satisfy it with a caller-assigned scalar postimage.
                const auto borrowed=paired.checkpoint();bool rejected=false;
                try{(void)paired.prepare_restore(borrowed);}catch(const FormatError& error){
                    if(std::string_view(error.what()).find("genuine effects target agreement")==std::string_view::npos)throw;
                    rejected=true;
                }
                need(rejected,"Borrowed Effects RNG restored without its actual writer agreement");
                row["borrowed_rng_restore_guard_verified"]=true;
                row["shared_bank_first_permutations"]=selected;row["shared_bank_common_rng"]=common;paired.stop();
            }
            row["passed"]=true;
        }catch(const std::exception& error){row["error"]=error.what();++failures;}
        report["cases"].push_back(std::move(row));
    }
    need(report["cases"].size()==(alternates_only?2u:7u),"Incomplete authored destruction loop coverage");
    report["passed"]=failures==0;report["failures"]=failures;
    std::ofstream(report_path)<<report.dump(2)<<'\n';
    std::cout<<"Destruction loop profiles="<<report["cases"].size()<<" failures="<<failures<<'\n';return failures?1:0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
