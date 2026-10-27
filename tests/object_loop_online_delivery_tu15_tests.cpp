#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/mcc_sound_metadata_publication.hpp"
#include "halo2/source_object_loop_emitter_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>

using namespace halo2;
namespace {
void need(bool value,const char* why){if(!value)throw FormatError(why);}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address) {
    return Bytes(memory.view(address,4)).u32(0);
}
struct Lift {Spawn spawn;std::uint32_t definition;};
Lift authored_lift(const Cache& cache,const LevelContent& content) {
    for(const auto& placed:content.objects) {
        const auto tag=cache.query_tag({.handle=TagHandle{placed.definition}});
        if(!tag || !tag->name.ends_with("\\lockout_lift_tube\\lockout_lift_tube"))continue;
        // Declare a post-birth position input from the authored lower hull.
        // Native allocation/birth and subsequent contact, acceleration,
        // control and sounds retain their complete original owners.
        const auto object=cache.load_source_tag_metadata(placed.definition);
        const Bytes o(object.bytes_at(object.source_address(),object.size()));
        const auto model=cache.load_source_tag_metadata(o.u32(0x38));
        const Bytes m(model.bytes_at(model.source_address(),model.size()));
        const auto physics=cache.load_source_tag_metadata(m.u32(0x24));
        const Bytes p(physics.bytes_at(physics.source_address(),physics.size()));
        need(p.u32(0x70)==2 && p.u32(0x108)==2,"Authored Lockout lift phantom hull changed");
        const Bytes hull(physics.bytes_at(p.u32(0x74),0x100));
        Transform local;local.position={hull.f32(0x40),hull.f32(0x44),hull.f32(0x48)};
        const auto basis=authored_basis(placed.rotation);
        const auto root=source_object_transform(placed.position,basis.forward,basis.up,placed.scale,false);
        return {{compose_transform(root,local).position,0},placed.definition};
    }
    throw FormatError("Prepared map has no authored Lockout lift");
}
std::vector<SourceObjectLoopEmitterTu15> lift_bank(const MultiplayerState& state,std::uint32_t object) {
    need(bool(state.source_object_loop_emitters),"Real UDP snapshot lost its pre-view source emitter publication");
    std::vector<SourceObjectLoopEmitterTu15> bank;
    for(const auto& emitter:*state.source_object_loop_emitters)if(emitter.object==object)bank.push_back(emitter);
    return bank;
}
}
int main(int argc,char** argv)try {
    need(argc==4,"Supply prepared MCC Lockout map, shared maps and a small report path");
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");
    need(scratch && std::filesystem::path(scratch).is_absolute(),"Run UDP audio validation through tools/run_test.py");
    SDL_SetMainReady();
    MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
    options.local_players=0;options.dedicated=true;options.mute=true;options.source_object_world=true;
    options.starting_weapon=5;options.session_random_seed=0x71a5845u;
    options.source_object_savegame_directory=std::filesystem::path(scratch)/"native-loop-host";
    auto client_cache=std::make_shared<Cache>(options.map,options.shared_maps);
    need(client_cache->is_mcc(),"UDP object-loop test requires prepared MCC input");
    const auto content=client_cache->load_content(0,true);
    const auto lift=authored_lift(*client_cache,content);
    MultiplayerPreparedWorld world(options,std::nullopt);
    need(world.source_objects && world.source_objects->owns_physics() && world.local_slots.empty(),
        "Dedicated audio producer acquired a fabricated local view");
    std::uint32_t object=null_datum;
    for(const auto& row:world.source_objects->live_objects())if(row.definition==lift.definition && row.parent==null_datum) {
        need(object==null_datum,"Lockout test lift definition has ambiguous source owners");object=row.object;
    }
    need(object!=null_datum,"Actual source world did not construct the authored lift");
    auto& native=world.source_objects->tu15_world();
    auto& memory=native.memory();
    unsigned audited_publications{},query_private_writes{},query_native_stores{},query_state_writes{};
    const auto audit_publication=[&] {
        // Query the real constructed world twice without stepping it. Every
        // source/native store must stay in the original caller's private ABI
        // bank; in particular this must not advance random/function state.
        const auto random=word(memory,0x4f3cd8u),clock=word(memory,0x51df54u);
        const auto functions=word(memory,0x51daf4u);
        need(random && clock,"Native loop audit lost its actual RNG or clock owner");
        const auto random_before=word(memory,random+4u),tick_before=word(memory,clock+8u);
        std::vector<std::byte> function_before;
        if(functions) {
            const auto bytes=memory.view(functions,0x300u);
            function_before.assign(bytes.begin(),bytes.end());
        }
        std::uint32_t first_state_store{};
        native.observe_physics_stores([&](auto,const auto& store) {
            ++query_native_stores;
            if(store.address<0x60000000u ||
               std::uint64_t(store.address)+store.written.size()>0x60100000ull)
                first_state_store=first_state_store?first_state_store:store.instruction;
        });
        int observer;
        source_runtime_register_write_observer(memory,&observer,[&](auto address,auto bytes) {
            if(address>=0x60000000u && std::uint64_t(address)+bytes<=0x60100000ull)
                ++query_private_writes;
            else ++query_state_writes;
        });
        std::vector<SourceObjectLoopEmitterTu15> first,second;
        try {first=native.object_loop_emitters();second=native.object_loop_emitters();}
        catch(...) {
            source_runtime_unregister_write_observer(memory,&observer);
            native.observe_physics_stores({});throw;
        }
        source_runtime_unregister_write_observer(memory,&observer);
        native.observe_physics_stores({});
        audited_publications+=2;
        need(!first_state_store && !query_state_writes,
            "Pre-view native loop query wrote outside its private caller storage");
        need(word(memory,0x4f3cd8u)==random && word(memory,random+4u)==random_before &&
             word(memory,0x51df54u)==clock && word(memory,clock+8u)==tick_before &&
             word(memory,0x51daf4u)==functions,
            "Pre-view native loop query advanced actual RNG, tick or function ownership");
        if(functions)need(std::ranges::equal(memory.view(functions,0x300u),function_before),
            "Pre-view native loop query changed the actual 32 function override rows");
        const auto target=[&](const auto& rows) {
            std::vector<SourceObjectLoopEmitterTu15> result;
            for(const auto& row:rows)if(row.object==object)result.push_back(row);
            return result;
        };
        need(target(first)==target(second),"Unstepped actual lift query changed its native outputs");
    };
    const auto bsp=client_cache->load_multiplayer_audio_bsp(0);
    const auto atmosphere=client_cache->load_bsp_atmosphere(0);
    const auto metadata=MccSoundMetadataPublication::create(client_cache);
    std::set<std::uint64_t> voices,late_voices;unsigned packets{},activation_packets{},late_activation_packets{},late_packets{};
    double peak{};
    const auto observe=[&](const MultiplayerAudioSubmission& event,bool late) {
        need(event.owner==object && event.voice && event.frames && event.position.has_value(),
            "Received lift packet lost actual spatial source identity or native PCM");
        const auto tag=client_cache->query_tag({.handle=TagHandle{event.sound}});
        need(bool(tag),"Received lift PCM lost its authored sound");
        if(late){++late_packets;late_voices.insert(event.voice);}
        else {++packets;voices.insert(event.voice);}
        if(tag->name.find("\\midship_activate_lift\\")!=std::string::npos) {
            if(late)++late_activation_packets;else ++activation_packets;
        }
    };
    MultiplayerAudio audio(client_cache,0x1258795u,
        {SdlSoundPlatformConfiguration::Delivery::Manual,256,{}},[&](const auto& event){observe(event,false);});
    audio.enroll_continuation();audio.bind_native_loop_metadata(metadata);
    std::unique_ptr<MultiplayerAudio> late_audio;
    const auto fingerprint=multiplayer_fingerprint(options.map,client_cache->shared_directory(),
        0,world.network_starting_weapon_identity());
    LanHost host(world.match,0,fingerprint);
    LanClient client(host.join_address(),fingerprint,1);
    std::unique_ptr<LanClient> late;
    std::array<Controls,multiplayer_capacity> controls{};const std::array<Controls,1> peer{};
    const auto receive=[&] {
        client.send(peer);if(late)late->send(peer);host.receive(controls);
        host.broadcast();client.receive();if(late)late->receive();
        need(client.error().empty() && (!late || late->error().empty()),"Actual UDP rejected the native emitter snapshot");
    };
    for(unsigned attempt=0;attempt<50 && !client.ready();++attempt)receive();
    need(client.ready() && client.slots().size()==1,"Real UDP lift client did not join");
    const auto slot=client.slots()[0];std::optional<float> initial_height;float highest=-1e30f;
    // Native presentation owns this declared stationary camera input, sampled
    // from the actual received player. Physical ascent continues independently.
    std::optional<Vec3> camera_origin,late_camera_origin;
    std::optional<unsigned> placed_tick,late_placed_tick,scenario_start;
    std::uint32_t placed_unit=null_datum,late_placed_unit=null_datum;
    unsigned source_positive{},received_positive{},late_publications{};bool far_retired{},empty_retired{},returned{},independent_view{};
    const auto present=[&](MultiplayerAudio& output,const MultiplayerState& state,unsigned player,
        std::optional<Vec3>& origin,bool far,bool empty) {
        auto bank=lift_bank(state,object);need(bank.size()==2,"Received lift lost an actual attachment generation");
        SourceObjectAudioViewsTu15 views;
        if(!origin) {
            if(state.players[player].source_unit==null_datum || state.players[player].dead)return;
            origin=state.players[player].position+Vec3{0,0,content.player.camera_height};
        }
        auto point=*origin;
        if(far)point=point+Vec3{100000,100000,100000};
        views[0]=SourceObjectAudioViewTu15{point,std::int16_t(atmosphere.cluster_at(point))};
        const std::array listeners{point};output.listeners(listeners);
        if(empty)bank.clear();
        for(const auto& request:native_object_loop_requests_tu15(bank,bsp,views))output.object_loop_request(request);
        output.object_loop_frame(tick_seconds);
        for(const auto sample:output.render(1600))peak=std::max(peak,double(std::abs(sample)));
        output.update();
    };
    for(unsigned tick=0;tick<600 && (!scenario_start || tick<*scenario_start+180);++tick) {
        receive();world.match.step(controls);
        const auto born=world.match.state();
        const auto place=[&](unsigned player,std::optional<unsigned>& at,std::uint32_t& unit) {
            const auto& actual=born.players[player];
            if(at || actual.source_unit==null_datum || actual.dead)return;
            unit=actual.source_unit;
            // Real canonical birth must complete first. Original B95D0 owns
            // the declared diagnostic placement and all physics delivery.
            native.set_object_position(unit,lift.spawn.position);at=tick;
        };
        place(slot,placed_tick,placed_unit);
        if(late && late->ready()) {
            need(late->slots().size()==1,"Late lift client lost its accepted native slot");
            place(late->slots()[0],late_placed_tick,late_placed_unit);
        }
        host.broadcast();client.receive();if(late)late->receive();
        const auto source=world.match.state();const auto& state=client.state();
        const auto source_rows=lift_bank(source,object),received_rows=lift_bank(state,object);
        need(source_rows==received_rows,"Real UDP changed completed pre-view emitter values");
        if(!placed_tick || tick<=*placed_tick || state.players[slot].source_unit!=placed_unit)continue;
        if(!scenario_start)scenario_start=tick;
        const auto frame=tick-*scenario_start;
        if(frame==0 || frame==1 || frame==20 || frame==80)audit_publication();
        const auto positive_activation=[&](const auto& emitter){
            const auto tag=client_cache->query_tag({.handle=TagHandle{emitter.definition}});
            return tag && tag->name.ends_with("\\midship_activate_lift\\midship_activate_lift") &&
                emitter.function_active && Bytes(emitter.location).f32(4)>0;
        };
        for(const auto& emitter:source_rows)if(positive_activation(emitter))++source_positive;
        for(const auto& emitter:received_rows)if(positive_activation(emitter))++received_positive;
        if(state.players[slot].source_unit!=null_datum && !state.players[slot].dead) {
            if(!initial_height)initial_height=state.players[slot].position.z;
            highest=std::max(highest,state.players[slot].position.z);
        }
        const bool far=frame>=75 && frame<85,empty=frame==100;
        present(audio,state,slot,camera_origin,far,empty);
        if(far){need(audio.voices()==0,"Client camera left PAS/distance but native loop voices survived");far_retired=true;}
        if(empty){need(audio.voices()==0,"Available empty emitter publication retained native loop voices");empty_retired=true;}
        if(frame==86 || frame==101){need(audio.voices()>0,"Completed emitter failed to restart on camera return/publication recovery");returned=true;}
        if(frame==40) {
            late=std::make_unique<LanClient>(host.join_address(),fingerprint,1);
            late_audio=std::make_unique<MultiplayerAudio>(client_cache,0x1578486u,
                SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}},
                [&](const auto& event){observe(event,true);});
            late_audio->enroll_continuation();late_audio->bind_native_loop_metadata(metadata);
        }
        if(late && late->ready() && late_placed_tick && tick>*late_placed_tick) {
            need(late->slots().size()==1,"Late lift client lost its local view");
            const auto bank=lift_bank(late->state(),object);
            need(bank.size()==2,"Late UDP join never acquired the persistent native emitter bank");
            ++late_publications;present(*late_audio,late->state(),late->slots()[0],late_camera_origin,false,false);
            if(far) {
                need(late_audio->voices()>0,"One client's distant camera retired another client's nearby lift");
                independent_view=true;
            }
        }
    }
    need(placed_tick && late_placed_tick && scenario_start && initial_height && highest>*initial_height,
        "Actual canonical native birth/placement never ascended the authored lift");
    need(source_positive && received_positive && packets && activation_packets && late_packets && late_publications,
        "Actual native lift control/activation/latejoin never reached client SDL PCM");
    need(far_retired && empty_retired && returned && independent_view,"Client lifecycle route was not exercised");
    need(audited_publications==8 && query_private_writes && query_native_stores && !query_state_writes,
        "Actual native emitter store audit was not exercised");
    nlohmann::json report{{"passed",true},{"scope","Native MCC dedicated authority, actual UDP, local BSP/camera selection and manual SDL PCM"},
        {"source_local_views",world.local_slots.size()},{"source_object",object},{"source_positive_control_samples",source_positive},
        {"received_positive_control_samples",received_positive},{"native_packets",packets},{"activation_packets",activation_packets},
        {"late_native_packets",late_packets},{"late_activation_packets",late_activation_packets},
        {"late_publications",late_publications},{"initial_height",*initial_height},
        {"peak_height",highest},{"pcm_peak",peak},{"far_retired",far_retired},{"empty_retired",empty_retired},
        {"independent_client_view",independent_view},{"audited_publications",audited_publications},
        {"query_private_writes",query_private_writes},{"query_native_stores",query_native_stores},
        {"query_state_writes",query_state_writes},
        {"canonical_birth_then_b95d0",true},{"placed_unit",placed_unit},{"late_placed_unit",late_placed_unit},
        {"placement_tick",*placed_tick},{"late_placement_tick",*late_placed_tick},{"scenario_start_tick",*scenario_start},
        {"camera_input","Stationary native camera sampled from the actual accepted player eye"},
        {"physical_speakers_verified",false},{"full_original_audio_parity",false}};
    std::ofstream(argv[3])<<report.dump(2)<<'\n';
    std::cout<<"PASS native UDP lift activation="<<activation_packets<<" late_packets="<<late_packets<<" lifecycle=retired/restarted\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
