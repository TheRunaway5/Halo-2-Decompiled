#include "halo2/multiplayer_audio.hpp"
#include "halo2/mcc_sound_metadata_publication.hpp"
#include "halo2/prepared_original_sound_catalog_tu15.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
using Json=nlohmann::json;
void need(bool yes,const char *why){if(!yes)throw FormatError(why);}
int actual_requests(const char *input_path,const char *output_path) {
    std::ifstream input(input_path);const auto plan=Json::parse(input);
    need(plan.at("passed").get<bool>() && plan.at("mode")=="actual-original-sound-requests" &&
        !plan.at("cases").empty() && plan.at("cases").size()<=32,"Unqualified actual sound-request plan");
    Json report{{"passed",false},{"cases",Json::array()},
        {"scope","Actual captured original caller requests through production native PCM/SDL manual output; coincident listener inputs and auxiliary RNG seed are declared component inputs"},
        {"source_timing_replayed",false},{"source_voice_admission_verified",false},
        {"original_permutation_or_waveform_equivalence_verified",false},
        {"physical_device_or_normal_runtime_delivery_verified",false},{"raw_audio_retained",false}};
    unsigned failed{},original_requests{},retained_requests{};
    for(const auto &entry:plan.at("cases")) {
        Json row{{"case",entry.at("case")},{"passed",false},{"local_listener_cases",Json::array()}};
        try {
            const std::filesystem::path path=entry.at("map").get<std::string>();
            auto cache=std::make_shared<const Cache>(path,path.parent_path());
            need(cache->is_mcc(),"Actual sound output requires its MCC scenario owner");
            const auto metadata=MccSoundMetadataPublication::create(cache);
            const auto catalog=cache->original_sound_catalog();
            const auto original=catalog?catalog->input(*cache):MccOriginalSoundRuntimeInputTu15{};
            row["original_catalog_admitted"]=bool(catalog);
            const auto &requests=entry.at("requests");need(requests.size()<=512,"Actual request plan exceeds its journal bound");
            std::vector<std::uint64_t> baseline_pcm;
            std::vector<std::pair<unsigned,unsigned>> baseline_selection;
            for(const unsigned views:{1u,2u,4u}) {
                unsigned packets{},active_packets{};std::optional<MultiplayerAudioSubmission> submitted;
                const SdlSoundPlatformConfiguration manual{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}};
                MultiplayerAudio audio(cache,0x5821b36du,manual,[&](const auto &packet){
                    ++packets;++active_packets;submitted=packet;
                });
                audio.enroll_continuation();audio.bind_native_loop_metadata(metadata);
                Json outputs=Json::array();unsigned index{},classic{},silent{};
                for(const auto &request:requests) {
                    const auto sound=request.at("sound").get<std::uint32_t>();
                    const auto hex=request.at("request_hex").get<std::string>();
                    need(hex.size()==320,"Actual source request is incomplete");
                    std::array<std::byte,160> bytes;
                    const auto nibble=[](char ch)->unsigned {
                        if(ch>='0'&&ch<='9')return unsigned(ch-'0');
                        if(ch>='a'&&ch<='f')return unsigned(ch-'a'+10);
                        throw FormatError("Actual source request has invalid hex bytes");
                    };
                    for(unsigned n=0;n<bytes.size();++n)bytes[n]=std::byte(nibble(hex[n*2u])*16u+nibble(hex[n*2u+1u]));
                    const Bytes raw(bytes);const auto flags=raw.u32(0),location=std::to_integer<unsigned>(bytes[15])&15u;
                    const auto *tag=sound==0xffffffffu?nullptr:cache->query_tag({.handle=TagHandle{sound}});
                    need(sound==0xffffffffu || (tag && tag->group=="snd!" && tag->name==request.at("name").get<std::string>()),
                        "Actual request lost its full native tag identity");
                    // Literal MCC bit11 is its authored remastered-only guard.
                    // Read it from the same resident tag the production route consumes.
                    const bool admitted=tag && !(Bytes(cache->read_tag_bytes(tag->address,28)).u16(0)&0x800u);
                    const auto owner=flags&8u?raw.u32(0x50u):0xffffffffu;
                    const std::optional<Vec3> position=location==1u?
                        std::optional(Vec3{raw.f32(0x18u),raw.f32(0x1cu),raw.f32(0x20u)}):std::nullopt;
                    audio.listeners(std::vector<Vec3>(views,position.value_or(Vec3{})));
                    active_packets=0;submitted.reset();
                    const auto kind=location==1u?MultiplayerAudioKind::Impulse:MultiplayerAudioKind::FirstPerson;
                    const auto voice=audio.source_impulse(sound,bytes,kind,0);
                    if(!admitted) {
                        need(!voice && !audio.voices() && !active_packets,"Authored silent route fabricated a PCM voice");
                        ++silent;outputs.push_back({{"request",index++},{"packets",0},{"authored_silent_route",true}});continue;
                    }
                    need(voice && active_packets==1 && submitted && submitted->sound==sound &&
                        submitted->owner==owner && submitted->kind==kind && submitted->view==0 &&
                        submitted->voice==*voice && submitted->frames && submitted->position.has_value()==position.has_value(),
                        "Actual caller lost its PCM packet, sound, owner or location");
                    if(position)need(submitted->position->x==position->x && submitted->position->y==position->y &&
                        submitted->position->z==position->z,"Actual caller world position changed at output");
                    const auto binding=std::ranges::find(original.bindings,TagHandle{sound},&OriginalSoundRuntimeBindingTu15::sound);
                    const bool original_payload=binding!=original.bindings.end();
                    if(original_payload){
                        const auto expected=binding->bank->encoded(submitted->pitch_range,submitted->permutation);
                        const auto encoded=metadata.encoded(*cache,{sound},submitted->pitch_range,submitted->permutation);
                        need(encoded.data==expected.data && encoded.codec==expected.codec &&
                            encoded.channels==expected.channels && encoded.sample_rate==expected.sample_rate &&
                            submitted->frames==decode_sound(expected).frames(),
                            "Actual caller did not use its admitted original payload");
                        ++original_requests;
                    }else ++retained_requests;
                    double peak{};std::uint64_t hash=0xcbf29ce484222325ull,frames{};
                    for(unsigned block=0;block<4096 && audio.voices();++block) {
                        const auto pcm=audio.render(4096);frames+=pcm.size()/2;
                        for(const auto sample:pcm){need(std::isfinite(sample),"Actual caller produced nonfinite PCM");
                            peak=std::max(peak,std::abs(double(sample)));hash=(hash^std::bit_cast<std::uint32_t>(sample))*0x100000001b3ull;}
                        audio.update();
                    }
                    need(!audio.voices() && peak>0 && active_packets==1,"Actual caller was silent, duplicated or failed finite completion");
                    const auto selection=std::pair{submitted->pitch_range,submitted->permutation};
                    if(views==1u){baseline_pcm.push_back(hash);baseline_selection.push_back(selection);}
                    else need(hash==baseline_pcm.at(classic) && selection==baseline_selection.at(classic),
                        "Coincident local listeners changed or duplicated actual caller PCM");
                    ++classic;outputs.push_back({{"request",index++},{"packets",active_packets},{"peak",peak},
                        {"mixed_frames",frames},{"pcm_hash",hash},{"selected_pitch_range",selection.first},
                        {"selected_permutation",selection.second},{"original_bank_payload",original_payload}});
                }
                need(packets==classic && classic+silent==requests.size(),"Actual request output accounting differs");
                row["local_listener_cases"].push_back({{"listeners",views},{"classic_requests",classic},
                    {"authored_silent_requests",silent},{"packets",packets},{"outputs",std::move(outputs)}});
            }
            row["passed"]=true;
        }catch(const std::exception &error){row["error"]=error.what();++failed;}
        report["cases"].push_back(std::move(row));
    }
    report["passed"]=failed==0;report["failures"]=failed;
    report["original_bank_pcm_requests"]=original_requests;report["retained_mcc_pcm_requests"]=retained_requests;
    std::ofstream(output_path)<<report.dump(2)<<'\n';
    std::cout<<"Actual destruction sound callers: cases="<<report["cases"].size()<<" failures="<<failed<<'\n';
    return failed?1:0;
}
}
int main(int argc,char **argv)try {
    if(argc==4 && std::string_view(argv[1])=="--actual-requests")return actual_requests(argv[2],argv[3]);
    need(argc==3,"Supply the destruction PCM inventory and bounded report path");
    if(!*argv[1])return 77;
    std::ifstream input(argv[1]);const auto plan=Json::parse(input);
    need(plan.at("passed").get<bool>() && !plan.at("cases").empty() && plan.at("cases").size()<=128,
        "Unqualified destruction sound inventory");
    Json report{{"passed",false},{"cases",Json::array()},
        {"scope","Declared sound requests through production native PCM/SDL manual output; source event timing, physical device output and full original parity remain open"},
        {"full_audio_parity_verified",false},{"raw_audio_retained",false}};
    std::map<std::filesystem::path,std::shared_ptr<const Cache>> caches;
    std::map<std::filesystem::path,MccSoundMetadataPublication> metadata;
    unsigned failed{},decoded_resources{};
    for(const auto &entry:plan.at("cases")) {
        Json row{{"sound",entry.at("sound")},{"map",entry.at("map")},{"passed",false},{"local_listener_cases",Json::array()}};
        try {
            const std::filesystem::path resource_path=entry.at("map").get<std::string>();
            // The decoder can read a standalone shared resource bank. Native
            // metadata publication requires a real scenario and its canonical
            // BSP owner; resolve that bank through the prepared level cache.
            const auto path=resource_path.filename()=="shared.map"?resource_path.parent_path()/"coagulation.map":
                resource_path.filename()=="single_player_shared.map"?resource_path.parent_path()/"03a_oldmombasa.map":resource_path;
            row["runtime_map"]=path.string();
            if(!caches.contains(path)) {
                auto cache=std::make_shared<const Cache>(path,path.parent_path());
                metadata.emplace(path,MccSoundMetadataPublication::create(cache));
                caches.emplace(path,std::move(cache));
            }
            const auto &cache=caches.at(path);need(cache->is_mcc(),"Destruction audio requires MCC runtime content");
            const auto name=entry.at("sound").get<std::string>();
            const auto tag=std::ranges::find_if(cache->tags(),[&](const auto &t){return t.group=="snd!" && t.name==name;});
            need(tag!=cache->tags().end(),"Original destruction sound lost its actual resident native identity");
            row["native_sound_datum"]=tag->datum;
            row["decoded_resources"]=Json::array();
            const auto ranges=cache->sound_pitch_range_count(tag->datum);
            need(ranges && ranges<=1024,"Invalid authored destruction pitch-range extent");
            for(unsigned pitch=0;pitch<ranges;++pitch) {
                const auto permutations=cache->sound_permutation_count(tag->datum,pitch);
                need(permutations && permutations<=65536,"Invalid authored destruction permutation extent");
                for(unsigned permutation=0;permutation<permutations;++permutation) {
                    need(++decoded_resources<=2048,"Destruction resource report exceeds its finite probe budget");
                    const auto pcm=decode_sound(cache->load_sound(tag->datum,pitch,permutation));
                    need(pcm.rate==48000 && pcm.frames() && pcm.stereo.size()%2==0,
                        "Authored destruction permutation produced incomplete native PCM");
                    double peak{};std::uint64_t hash=0xcbf29ce484222325ull;
                    for(float sample:pcm.stereo) {
                        need(std::isfinite(sample),"Authored destruction permutation decoded nonfinite PCM");
                        peak=std::max(peak,std::abs(double(sample)));
                        hash=(hash^std::bit_cast<std::uint32_t>(sample))*0x100000001b3ull;
                    }
                    need(peak>0,"Authored destruction permutation decoded only silence");
                    row["decoded_resources"].push_back({{"pitch_range",pitch},{"permutation",permutation},
                        {"frames",pcm.frames()},{"peak",peak},{"pcm_hash",hash}});
                }
            }
            std::optional<std::uint64_t> single_listener_pcm;
            for(unsigned views:{1u,2u,4u}) {
                constexpr std::uint32_t owner=0x12340001u;
                unsigned packets{};std::uint64_t packet_voice{};unsigned selected_range{},selected_permutation{};
                const SdlSoundPlatformConfiguration manual{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}};
                MultiplayerAudio audio(cache,0x5821b36du,manual,[&](const auto &packet){
                    need(packet.sound==tag->datum && packet.owner==owner && packet.kind==MultiplayerAudioKind::Impulse &&
                        packet.frames>0 && packet.voice && packet.position && packet.position->x==0 &&
                        packet.position->y==0 && packet.position->z==0,"Native destruction packet lost its authored sound/owner/position");
                    ++packets;packet_voice=packet.voice;selected_range=packet.pitch_range;selected_permutation=packet.permutation;
                });
                audio.enroll_continuation();
                audio.bind_native_loop_metadata(metadata.at(path));
                std::vector<Vec3> listeners(views,Vec3{0,0,0});audio.listeners(listeners);
                const auto voice=audio.play_impulse_at(tag->datum,{0,0,0},1,MultiplayerAudioKind::Impulse,owner);
                need(voice && audio.voice_active(*voice),"Native destruction request produced no actual PCM voice");
                double peak{};std::uint64_t frames{},pcm_hash=0xcbf29ce484222325ull;
                for(unsigned block=0;block<4096 && audio.voices();++block) {
                    const auto pcm=audio.render(4096);frames+=pcm.size()/2;
                    for(float sample:pcm){
                        need(std::isfinite(sample),"Native destruction PCM is nonfinite");peak=std::max(peak,std::abs(double(sample)));
                        pcm_hash=(pcm_hash^std::bit_cast<std::uint32_t>(sample))*0x100000001b3ull;
                    }
                    audio.update();
                }
                need(!audio.voices() && peak>0 && packets==1 && packet_voice==*voice,
                    "Native destruction cue was silent, duplicated or failed finite completion");
                if(!single_listener_pcm)single_listener_pcm=pcm_hash;
                need(pcm_hash==*single_listener_pcm,"Additional coincident local listeners duplicated or changed destruction PCM");
                need(!audio.play_impulse_at(0xffffffffu,{0,0,0},1,MultiplayerAudioKind::Impulse,owner) && packets==1,
                    "Authored NONE fabricated a destruction voice");
                row["local_listener_cases"].push_back({{"listeners",views},{"packets",packets},{"mixed_frames",frames},
                    {"peak",peak},{"pcm_hash",pcm_hash},{"selected_pitch_range",selected_range},{"selected_permutation",selected_permutation}});
            }
            row["passed"]=true;row["all_permutation_resources_verified"]=true;
        }catch(const std::exception &error){row["error"]=error.what();++failed;}
        report["cases"].push_back(std::move(row));
    }
    report["passed"]=failed==0;report["failures"]=failed;
    std::ofstream(argv[2])<<report.dump(2)<<'\n';
    std::cout<<"Native destruction audio banks="<<report["cases"].size()<<" failures="<<failed<<'\n';
    return failed?1:0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
