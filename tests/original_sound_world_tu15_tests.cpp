#include "helpers/original_sound_preparation_inputs.hpp"
#include "halo2/mcc_script_sound_world_access.hpp"
#include "halo2/mcc_sound_variation_provider.hpp"
#include "halo2/mcc_sound_timing_reference.hpp"
#include "halo2/mcc_runtime_delivery.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/original_data.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_runtime_resource_preparation.hpp"
#include "halo2/source_sound_timing_input_observation_tu15.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <openssl/evp.h>
#include <set>

namespace {
using namespace halo2;using J=nlohmann::json;
void need(bool value,const char* why){if(!value)throw FormatError(why);}
template<class F>void rejects(F&& call,unsigned& controls){bool refused=false;try{call();}catch(const FormatError&){refused=true;}
    need(refused,"Invalid original World ownership was admitted");++controls;}
std::string sha(std::span<const std::byte> data){std::array<unsigned char,32> digest;unsigned bytes{};
    need(EVP_Digest(data.data(),data.size(),digest.data(),&bytes,EVP_sha256(),nullptr)==1&&bytes==32,"World qualification digest failed");
    constexpr char hex[]="0123456789abcdef";std::string out;for(auto v:digest){out+=hex[v>>4];out+=hex[v&15];}return out;}
void encoded_equal(const EncodedSound& a,const EncodedSound& b){
    need(a.data==b.data&&a.codec==b.codec&&a.sample_rate==b.sample_rate&&a.channels==b.channels&&a.samples==b.samples&&a.sound_class==b.sound_class,
        "Actual World provider selected different original encoded bytes or format");}
}
int main(int argc,char** argv){J report{{"passed",false},{"goal_complete",false},{"physical_device_or_xbox_waveform_verified",false},
    {"playing_audio_atomic_restore_verified",false},{"source_voice_allocation_verified",false},{"native_mcc_prefix_dat_backing_verified",false},
    {"full_destruction_or_local_split_lan_acceptance_verified",false},{"retained_large_artifacts",J::array()}};
    if((argc!=7&&argc!=9)||!std::filesystem::is_regular_file(argv[1])||!std::filesystem::is_regular_file(argv[3]))return 77;
    try{
        report["phase"]="catalog";const std::string context=argv[2];
        need(!context.empty()&&std::filesystem::path(context).filename()==context,"Declare the exact original catalog context");
        std::ifstream input(argv[3],std::ios::binary);const std::string raw((std::istreambuf_iterator<char>(input)),{});
        need(sha(std::as_bytes(std::span(raw.data(),raw.size())))==argv[4],"Original catalog differs from its external admission pin");
        const std::filesystem::path map=argv[1];auto cache=std::make_shared<const Cache>(map,map.parent_path(),OriginalData::native_tu15());
        need(cache->is_mcc(),"Original World integration requires actual MCC runtime content");
        const auto cache_digest=sha(cache->source_runtime_cache_bytes());
        const test_original_sound::Inputs original(*cache,J::parse(raw),context);const auto in=original.view();
        const std::string admission=argc==9?argv[7]:"explicit";
        need((admission=="explicit"&&!cache->original_sound_catalog())||
            ((admission=="direct"||admission=="countdown")&&cache->original_sound_catalog()),"World fixture admission mode differs from actual Cache ownership");
        SourceRuntimeResources::Identity ordinary;
        if(admission=="explicit"){SourceRuntimeResources resources(*cache);ordinary=resources.identity();}
        else {
            const std::filesystem::path baseline_map=argv[8];const Cache baseline(baseline_map,baseline_map.parent_path(),OriginalData::native_tu15());
            need(!baseline.original_sound_catalog()&&sha(baseline.source_runtime_cache_bytes())==cache_digest,"Admitted World baseline is not the identical pre-catalog Cache snapshot");
            SourceRuntimeResources resources(baseline);ordinary=resources.identity();
        }
        report["phase"]="prepare";std::unique_ptr<SourceRuntimeResourcePreparation> preparation;
        if(admission=="explicit")preparation=std::make_unique<SourceRuntimeResourcePreparation>(cache,in);
        else if(admission=="countdown")preparation=std::make_unique<SourceRuntimeResourcePreparation>(cache);
        report["phase"]="world_construct";std::filesystem::create_directories(argv[5]);
        auto world=std::make_unique<PhysicsLiveWorldTu15>(*cache,argv[5]);
        report["phase"]="initialize";world->initialize_map();
        world->configure_session(source_game_variant_default(0),1,30,0x6a19d3e1u);
        const auto lineage=world->runtime_resources().original_sound_owner();need(bool(lineage),"Actual World lost its original resource owner");
        const auto identity=world->runtime_resources().identity();
        need(identity.resident==lineage->bind_resource_identity(ordinary.resident)&&identity.shared==ordinary.shared&&identity!=ordinary,
            "Actual World resource identity omitted the original bank");
        auto provider=world->sound_variation_provider(*cache);auto access=world->script_sound_access(*cache);
        TagHandle prefix_sound;EncodedSound prefix_encoded;SoundVariationDefinition prefix_definition;MccSoundOriginalTimingOperands prefix_original;
        {
            std::optional<MccSoundRuntimePlan> prefix_plan;
            const auto delivery=MccRuntimeCacheDelivery::plan(*cache,0x90000000u,0x60000000u,[&](MccSoundRuntimePlan&& p){prefix_plan.emplace(std::move(p));});
            const auto reference=build_mcc_sound_timing_reference(*cache,*prefix_plan,[&](auto at,auto bytes){return delivery.runtime_address(at,bytes);});
            for(const auto& binding:reference.bindings())if(!lineage->contains(binding.sound)&&binding.relative_pitch==0&&binding.relative_permutation==0){
                const auto range=std::span(prefix_plan->bytes).subspan(binding.native_range-prefix_plan->address,12);
                const auto count=Bytes(range).u16(10);if(!count||count>32)continue;
                prefix_sound=binding.sound;prefix_definition=provider.load(*cache,prefix_sound);prefix_original=binding.original;
                prefix_encoded=cache->load_sound(prefix_sound.value,0,0);encoded_equal(provider.encoded(*cache,prefix_sound,0,0),prefix_encoded);break;
            }
            need(!prefix_sound.is_null(),"Actual World fixture lacks an authenticated retained-prefix duration tuple");
        }
        std::uint64_t duration_calls{};std::set<std::uint32_t> duration_sounds;J timing_inputs=J::array();
        unsigned prefix_duration_calls{};
        std::optional<SourceSoundTimingInputObserverBindingTu15> timing_observer;
        timing_observer.emplace(world->memory(),[&](const auto& entry,const auto& operands,const auto& binding){
            const bool original=lineage->contains(binding.sound);
            need(original||(binding.sound==prefix_sound&&binding.relative_pitch==0&&binding.relative_permutation==0),"Duration fixture observed a different resource owner");
            const auto encoded=original?lineage->bank()->encoded(binding.sound,binding.relative_pitch,binding.relative_permutation):prefix_encoded;
            const auto definition=original?lineage->bank()->load(binding.sound,binding.relative_pitch):prefix_definition;
            const MccSoundOriginalTimingOperands expected_original=original?MccSoundOriginalTimingOperands{
                std::uint32_t(encoded.data.size()),std::to_integer<std::uint8_t>(definition.sound[3]),
                std::to_integer<std::uint8_t>(definition.sound[4]),std::to_integer<std::uint8_t>(definition.sound[5])}:prefix_original;
            need(entry.pc==0x21f590u&&entry.return_address==0x21f6f8u&&entry.eax==encoded.data.size()&&
                operands==expected_original,
                "Actual native duration caller differs from its exact original operands");
            if(!original)report["retained_prefix_timing"]=J{{"sound",binding.sound.value},{"path",binding.path},
                {"actual_mcc_encoded_bytes",entry.eax},{"original_timing_encoded_bytes",operands.encoded_bytes},
                {"mcc_codec",{std::to_integer<unsigned>(definition.sound[3]),std::to_integer<unsigned>(definition.sound[4]),std::to_integer<unsigned>(definition.sound[5])}},
                {"original_timing_codec",{operands.sample_rate,operands.encoding,operands.compression}},
                {"independent_original_reference_equal",true}};
            if(original){++duration_calls;duration_sounds.insert(binding.sound.value);}else ++prefix_duration_calls;
            if(timing_inputs.size()<64)timing_inputs.push_back({{"sound",binding.sound.value},{"pitch",binding.relative_pitch},{"permutation",binding.relative_permutation},
                {"entry",entry.pc},{"return_address",entry.return_address},{"encoded_bytes",operands.encoded_bytes},
                {"sample_rate",operands.sample_rate},{"encoding",operands.encoding},{"compression",operands.compression}});
        });
        const SdlSoundPlatformConfiguration manual{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}};
        std::optional<MultiplayerAudioSubmission> packet;unsigned packets{},controls{},voices{},selections{},stored{},duration_queries{};
        MultiplayerAudio audio(cache,0x5821b36du,manual,[&](const auto& value){packet=value;++packets;});
        audio.bind_native_sound_metadata(provider);audio.listeners(std::array{Vec3{}});
        report["phase"]="native_sound_consumers";J sounds=J::array();TagHandle selected{};
        for(const auto& binding:lineage->bindings()){
            const auto expected=std::ranges::find(original.bindings,binding.sound,&OriginalSoundRuntimeBindingTu15::sound);
            need(expected!=original.bindings.end()&&expected->bank->metadata().sound==binding.bank->metadata().sound&&
                std::ranges::equal(expected->bank->sectors(),binding.bank->sectors()),"Actual World adopted a different original bank than the authenticated exporter");
            selected=binding.sound;const auto definition=provider.load(*cache,binding.sound);
            need(definition.sound==lineage->bank()->load(binding.sound).sound,"Actual World changed the original native root");
            encoded_equal(provider.encoded(*cache,binding.sound,0,0),lineage->bank()->encoded(binding.sound,0,0));
            const auto expected_ms=std::bit_cast<std::int32_t>(Bytes(binding.bank->metadata().sound).u32(16));
            need(access.stored_milliseconds(*cache,binding.sound)==expected_ms,"Actual original stored milliseconds changed");++stored;
            SourceScriptSoundClockSelectionTu15 selection;access.select(*cache,binding.sound,selection);++selections;
            const auto seconds=access.selected_seconds(*cache,binding.sound,{0,0});
            need(std::isfinite(std::bit_cast<float>(seconds))&&std::bit_cast<float>(seconds)>=0,"Actual original native duration is invalid");++duration_queries;
            packet.reset();const auto before=packets;const auto voice=audio.play_impulse(binding.sound.value);
            need(voice&&packet&&packets==before+1&&packet->sound==binding.sound.value,"Actual original World provider failed Audio PCM submission");
            const auto pcm=decode_sound(provider.encoded(*cache,binding.sound,packet->pitch_range,packet->permutation));
            need(pcm.frames()&&packet->frames==pcm.frames(),"Actual World Audio packet lost its selected original PCM extent");
            const auto mixed=audio.render(256);need(mixed.size()==512&&std::ranges::all_of(mixed,[](auto value){return std::isfinite(value);}),"Original World Audio mixer returned invalid PCM");
            audio.stop();++voices;
            sounds.push_back({{"sound",binding.sound.value},{"name",binding.bank->metadata().name},{"stored_ms",expected_ms},
                {"native_selector",{selection.pitch_range,selection.permutation}},{"native_seconds_bits",seconds},
                {"audio_pitch",packet->pitch_range},{"audio_permutation",packet->permutation},{"audio_pcm_frames",packet->frames}});
        }
        need(voices==lineage->bindings().size()&&duration_sounds.size()==voices&&duration_calls>=voices,
            "Original World integration did not execute every selected native duration owner");
        const auto prefix_seconds=access.selected_seconds(*cache,prefix_sound,{0,0});
        need(prefix_duration_calls==1&&std::isfinite(std::bit_cast<float>(prefix_seconds)),"Retained prefix duration did not execute its actual native caller");
        report["phase"]="checkpoint";const auto checkpoint=world->authority_checkpoint();
        need(checkpoint.resources==identity,"Actual World checkpoint lost original resource identity");
        {auto invalid=checkpoint;invalid.resources=ordinary;rejects([&]{(void)world->prepare_authority(invalid);},controls);}
        {auto invalid=checkpoint;invalid.resources.resident[0]^=std::byte{1};rejects([&]{(void)world->prepare_authority(invalid);},controls);}
        const auto probe=[&](const auto& current){SourceScriptSoundClockSelectionTu15 value;current.select(*cache,selected,value);
            return std::pair{value,current.selected_seconds(*cache,selected,{0,0})};};
        const auto branch=probe(access);audio.prepare(selected.value);const auto before_packets=packets;const auto frames=audio.presentation_frames();
        timing_observer.reset();
        report["phase"]="restore";{auto restore=world->prepare_authority(checkpoint);need(world->commit_authority(std::move(restore)),"Actual original World checkpoint did not restore");}
        need(world->runtime_resources().identity()==identity&&world->runtime_resources().original_sound_owner()==lineage,"Restore changed the original resource owner");
        rejects([&]{provider.require_current(*cache);},controls);rejects([&]{(void)access.time(*cache);},controls);
        rejects([&]{audio.prepare(selected.value);},controls);rejects([&]{(void)audio.play_impulse(selected.value);},controls);
        rejects([&]{(void)audio.render(256);},controls);
        need(packets==before_packets&&audio.presentation_frames()==frames&&!audio.voices(),"Stale actual World Audio changed backend state");
        auto fresh=world->sound_variation_provider(*cache);auto fresh_access=world->script_sound_access(*cache);
        need(probe(fresh_access)==branch,"Restored original native selection or duration diverged");
        need(fresh_access.selected_seconds(*cache,prefix_sound,{0,0})==prefix_seconds,"Restored retained-prefix duration diverged");
        audio.bind_native_sound_metadata(fresh);need(bool(audio.play_impulse(selected.value)),"Fresh original World binding failed playback after restore");audio.stop();
        report["phase"]="teardown";world.reset();
        rejects([&]{fresh.require_current(*cache);},controls);rejects([&]{(void)fresh_access.time(*cache);},controls);
        rejects([&]{audio.prepare(selected.value);},controls);audio.stop();
        need(sha(cache->source_runtime_cache_bytes())==cache_digest,"Original World execution altered Cache bytes");
        report.update(J{{"passed",true},{"map",context},{"mcc_map",map.filename().string()},{"original_bindings",lineage->bindings().size()},
            {"native_selections",selections},{"stored_time_queries",stored},{"native_duration_queries",duration_queries},{"actual_duration_caller_observations",duration_calls},
            {"audio_original_voices",voices},{"negative_controls",controls},{"resource_identity_retained_through_world_restore",true},
            {"original_world_checkpoint_restored",true},{"selection_duration_replay_equal",true},{"old_epoch_consumers_refused",true},
            {"fresh_binding_playback_after_restore",true},{"raw_cache_unchanged",true},{"timing_inputs",timing_inputs},{"sounds",sounds},
            {"retained_prefix_native_duration_verified",true},{"retained_prefix_duration_replay_equal",true},{"retained_prefix_duration_caller_observations",prefix_duration_calls},
            {"preparation_path",admission},{"normal_cache_catalog_admitted",bool(cache->original_sound_catalog())},
            {"scope","Actual native World, original-bank preparation,213410 selection/213690 duration, manual PCM and native bank restore; no complete gameplay or playing-Audio handoff"}});
        report.erase("phase");
    }catch(const std::exception& error){report["error"]=error.what();}
    std::ofstream output(argv[6]);output<<report.dump(2)<<'\n';if(!output)return 2;std::cout<<report.dump()<<'\n';return report.at("passed").get<bool>()?0:1;
}
