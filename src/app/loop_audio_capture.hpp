#pragma once
#include "halo2/cache.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/multiplayer_outcome.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/source_loop_audio_location_observation_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdlib>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>

namespace halo2::app {
// Explicit finite CLI diagnostics only. The normal runtime owns all source
// state, input delivery, sounds and rendering; these callbacks observe it.
inline int run_loop_audio_capture(MultiplayerOptions options, std::filesystem::path output) {
    if (options.input_script.empty() || !options.capture_tick || options.capture_tick > 5000 ||
        options.screenshot.empty() || options.frames || options.ticks || options.mute || options.local_players != 1 ||
        options.host_port || !options.join.empty() || options.dedicated || options.replay || options.online_match ||
        !options.gameplay_xbe.empty() || options.capture_root || options.capture_camera ||
        !options.reference_player.empty() || options.reference_auxiliary_seed ||
        !options.reference_tick_cadence || !options.desktop ||
        options.desktop->display_settings.graphics_mode != pstencil::GraphicsMode::Original)
        throw FormatError("Loop-audio report requires an unmuted local Original MCC input-script tick capture");
    const auto *owned = std::getenv("HALO2_TEST_SCRATCH");
    if (!owned || !std::filesystem::path(owned).is_absolute())
        throw FormatError("Loop-audio report requires owned HALO2_TEST_SCRATCH storage");
    const auto scratch = std::filesystem::canonical(owned);
    output = std::filesystem::absolute(output).lexically_normal();
    const auto relative = std::filesystem::weakly_canonical(output).lexically_relative(scratch);
    if (relative.empty() || relative.is_absolute() || *relative.begin() == ".." ||
        output.extension() != ".json" || std::filesystem::exists(output) || std::filesystem::is_symlink(output))
        throw FormatError("Use a fresh loop-audio JSON report inside HALO2_TEST_SCRATCH");
    const auto image=std::filesystem::weakly_canonical(std::filesystem::absolute(options.screenshot));
    const auto image_relative=image.lexically_relative(scratch);
    if (image_relative.empty() || image_relative.is_absolute() || *image_relative.begin()==".." ||
        image.extension()!=".ppm" || std::filesystem::exists(image) || std::filesystem::is_symlink(options.screenshot))
        throw FormatError("Use a fresh loop-audio PPM capture inside HALO2_TEST_SCRATCH");
    std::filesystem::create_directories(output.parent_path());
    options.screenshot=image;
    std::ifstream map_stream(options.map,std::ios::binary);
    std::array<std::uint32_t,2> map_header{};
    map_stream.read(reinterpret_cast<char *>(map_header.data()),sizeof(map_header));
    if (!map_stream || map_header[0]!=0x68656164u || map_header[1]!=13u)
        throw FormatError("Loop-audio report requires an imported MCC version13 cache");
    std::uint32_t smg=0xffffffffu, firing_sound=0xffffffffu, smg_projectile=0xffffffffu;
    std::uint32_t target=0xffffffffu, target_definition=0xffffffffu;
    std::shared_ptr<MultiplayerPreparedWorld> prepared;
    std::set<std::uint32_t> reload_sounds;
    SourceLoopAudioLocationObservationTu15 observer;
    std::uint64_t input_samples{}, frames{}, generate{}, apply{}, projectiles{}, last_projectile{};
    std::uint64_t packets{}, invalid_packets{}, smg_packets{}, reload_packets{};
    struct SmgPacket {
        std::uint32_t sound{},owner{},kind{},view{};
        std::uint64_t voice{},frames{};
        bool has_position{};
        std::array<std::uint32_t,3> position_bits{};
    };
    std::array<SmgPacket,8> smg_packet_samples{};
    unsigned smg_packet_sample_count{};
    std::uint32_t player=0xffffffffu, unit=0xffffffffu, weapon=0xffffffffu;
    std::uint64_t grenade_commands{}; bool smg_started{}, have_previous{}, have_target_body{}, precondition_observed{};
    int previous_loaded{}; std::uint32_t previous_shot{};
    float body_before{}, body_after{};
    bool have_body_before_smg{}, smg_body_damage{};
    float body_before_smg{}, minimum_body_after_smg{};
    std::uint32_t first_smg_command_tick{}, first_smg_damage_tick{};
    std::uint64_t post_smg_vitality_samples{};
    std::map<std::uint32_t,nlohmann::json> grenade_projectiles, grenade_detonations;
    const auto previous_world=options.source_world_observer;
    options.source_world_observer=[&](const auto &memory) {
        if (previous_world) previous_world(memory);
        observer.bind(memory);
    };
    const auto previous_input=options.input_observer;
    options.input_observer=[&](auto step, auto commands, const auto &state) {
        if (previous_input) previous_input(step,commands,state);
        if (commands.size()!=1) throw FormatError("Loop-audio report lost its actual local command");
        const auto &c=commands.front();
        if (c.melee || c.pickup || c.dual_wield || c.switch_weapon || c.fire_secondary)
            throw FormatError("Loop-audio report accepts one ordinary pre-fire frag, SMG fire/reload and movement only");
        const MultiplayerState::Player *local{};
        for (const auto &p:state.players) if (p.connected) {
            if (local) throw FormatError("Loop-audio report requires one actual player");
            local=&p;
        }
        if (!local) throw FormatError("Loop-audio report has no actual player");
        const auto &p=*local; ++input_samples;
        player=p.source_player; unit=p.source_unit; weapon=p.source_weapon;
        if (have_previous && p.shot_tick!=previous_shot && p.loaded<previous_loaded) smg_started=true;
        if (c.grenade && (smg_started || c.fire || ++grenade_commands>1))
            throw FormatError("Loop-audio precondition must be one actual grenade pulse before SMG fire");
        previous_shot=p.shot_tick; previous_loaded=p.loaded; have_previous=true;
        if (target==0xffffffffu) for (const auto &vehicle:state.world_vehicles)
            if (vehicle.placement<prepared->content.objects.size() &&
                prepared->content.objects[vehicle.placement].definition==target_definition) {target=vehicle.handle;break;}
        const bool target_live=target!=0xffffffffu && std::any_of(state.world_vehicles.begin(),state.world_vehicles.end(),
            [&](const auto &vehicle){return vehicle.handle==target;});
        if (target_live) {
            if (prepared->source_objects->object_definition(target)!=target_definition)
                throw FormatError("Loop-audio report lost its full Warthog target generation");
            const auto vitality=prepared->source_objects->object_vitality(target);
            if (!std::isfinite(vitality.body)) throw FormatError("Loop-audio target vitality is nonfinite");
            if (!have_target_body) {body_before=body_after=vitality.body;have_target_body=true;}
            if (c.grenade) body_before=body_after=vitality.body;
            else if (grenade_commands && !smg_started && !precondition_observed) body_after=vitality.body;
            if (!have_body_before_smg && c.fire && precondition_observed) {
                have_body_before_smg=true;body_before_smg=minimum_body_after_smg=vitality.body;
                first_smg_command_tick=state.tick;
            }
            if (have_body_before_smg && smg_started) {
                ++post_smg_vitality_samples;
                minimum_body_after_smg=std::min(minimum_body_after_smg,vitality.body);
                if (!smg_body_damage && vitality.body<body_before_smg) {
                    smg_body_damage=true;first_smg_damage_tick=state.tick;
                }
            }
        }
        if (c.grenade && !target_live) throw FormatError("Loop-audio precondition has no actual Warthog target");
        if (grenade_commands && !smg_started) for (const auto &g:state.grenade_projectiles)
            if (g.type==0 && g.player==player && g.unit==unit) {
                if (g.datum==0xffffffffu || !(g.datum>>16) || g.definition==0xffffffffu || !(g.definition>>16))
                    throw FormatError("Loop-audio precondition lost its full frag generation");
                if (!grenade_projectiles.contains(g.datum)) {
                    if (grenade_projectiles.size()>=1) throw FormatError("Loop-audio precondition admitted more than one frag");
                    grenade_projectiles[g.datum]={{"projectile",g.datum},{"definition",g.definition},{"type",g.type},
                        {"player",g.player},{"unit",g.unit},{"released",false}};
                }
                if (g.parent==0xffffffffu) grenade_projectiles[g.datum]["released"]=true;
            }
        for (const auto &event:state.grenade_detonations) if (event.serial>last_projectile) {
            last_projectile=event.serial;
            if (event.player==player && event.unit==unit && event.definition==smg_projectile) ++projectiles;
            if (!smg_started && grenade_projectiles.contains(event.projectile) && event.player==player && event.unit==unit &&
                grenade_projectiles.at(event.projectile).at("definition")==event.definition)
                grenade_detonations[event.projectile]={{"projectile",event.projectile},{"definition",event.definition},
                    {"player",event.player},{"unit",event.unit},{"serial",event.serial},{"tick",event.tick}};
        }
        if (!precondition_observed && grenade_commands==1 && have_target_body && body_after>0.f && body_after<body_before &&
            grenade_projectiles.size()==1 && grenade_detonations.size()==1 &&
            grenade_projectiles.begin()->second.at("released")==true) precondition_observed=true;
        observer.observe_input({state.tick,smg,player,unit,weapon,p.weapon,p.loaded,p.reserve,p.shot_tick,c.fire,c.reload,
            target_live?target:0xffffffffu,target_live?target_definition:0xffffffffu,precondition_observed&&target_live});
    };
    const auto previous_frame=options.frame_observer;
    options.frame_observer=[&](const auto &frame) {
        if (previous_frame) previous_frame(frame);
        if (!frame.submitted) return;
        ++frames; generate+=frame.renderer.world_native_shadow_generate_prepared_draws;
        apply+=frame.renderer.world_native_shadow_apply_prepared_draws;
    };
    const auto previous_audio=options.audio_observer;
    options.audio_observer=[&](const auto &packet) {
        if (previous_audio) previous_audio(packet);
        if (!packet.frames || !packet.voice) {++invalid_packets;return;}
        ++packets;
        if (packet.sound==firing_sound) {
            if (smg_packet_sample_count<smg_packet_samples.size()) {
                auto &sample=smg_packet_samples[smg_packet_sample_count++];
                sample.sound=packet.sound;sample.owner=packet.owner;sample.kind=unsigned(packet.kind);sample.view=packet.view;
                sample.voice=packet.voice;sample.frames=packet.frames;sample.has_position=packet.position.has_value();
                if(packet.position)sample.position_bits={std::bit_cast<std::uint32_t>(packet.position->x),
                    std::bit_cast<std::uint32_t>(packet.position->y),std::bit_cast<std::uint32_t>(packet.position->z)};
            }
            // Runtime's queued native weapon requests require original bit3
            // and raw+50 equal to this weapon. Location0 is FirstPerson;
            // location1 is Impulse. Both preserve the actual weapon owner.
            bool finite_position=true;
            if(packet.position)for(const auto bits:{std::bit_cast<std::uint32_t>(packet.position->x),
                std::bit_cast<std::uint32_t>(packet.position->y),std::bit_cast<std::uint32_t>(packet.position->z)})
                finite_position&=(bits&0x7f800000u)!=0x7f800000u;
            const bool source_location=(packet.kind==MultiplayerAudioKind::FirstPerson && !packet.position) ||
                (packet.kind==MultiplayerAudioKind::Impulse && packet.position && finite_position);
            if (packet.view==0 && packet.owner==weapon && source_location) ++smg_packets;
        }
        if (packet.owner==weapon && packet.kind==MultiplayerAudioKind::FirstPerson && reload_sounds.contains(packet.sound)) ++reload_packets;
    };
    const auto retain=[&](bool passed,int result,const MultiplayerRuntimeOutcome *outcome,const std::string &error) {
        auto data=observer.report(); data["collector_schema"]=data.at("schema");
        data["schema"]="HALO2_NATIVE_LOOP_AUDIO_LOCATION_V1";
        data.update({{"passed",passed},{"run_result",result},{"error",error},{"map",options.map.string()},
            {"scope","Actual MCC Warthog vitality, native SMG ammo/reload, authored audio packets and passive TU1.5 loop location calls with submitted frames; no audible-speaker or framebuffer equivalence inferred"},
            {"graphics_mode","Original"},{"runtime_cache_source","MCC"},{"mute",false},
            {"requested_capture_tick",options.capture_tick},{"input_samples",input_samples},
            {"final_tick",outcome&&outcome->state?nlohmann::json(outcome->state->tick):nlohmann::json(nullptr)},
            {"captured_tick",outcome&&outcome->reason==MultiplayerExitReason::captured_tick},
            {"submitted_frames",frames},{"native_projectile_publications",projectiles},
            {"shadow_generate_draws",generate},{"shadow_apply_draws",apply},
            {"audio",{{"packets",packets},{"invalid_packets",invalid_packets},
                {"smg_packets",smg_packets},{"reload_packets",reload_packets}}}});
        data["audio"]["authored_smg_sound"]=firing_sound;
        data["audio"]["smg_packet_scope"]="Exact authored local-view SMG sound with the actual pinned weapon owner; native source request location0 selects FirstPerson and location1 selects Impulse. Runtime requires original owner bit3 before queuing.";
        data["audio"]["smg_packet_samples"]=nlohmann::json::array();
        for(unsigned index=0;index<smg_packet_sample_count;++index){const auto &sample=smg_packet_samples[index];
            data["audio"]["smg_packet_samples"].push_back({{"sound",sample.sound},{"owner",sample.owner},{"kind",sample.kind},
                {"view",sample.view},{"voice",sample.voice},{"frames",sample.frames},{"has_position",sample.has_position},
                {"position_bits",sample.position_bits}});}
        data["target"].update({{"body_before_smg",body_before_smg},{"minimum_body_after_smg",minimum_body_after_smg},
            {"smg_damage_observed",smg_body_damage},{"first_smg_command_tick",first_smg_command_tick},
            {"first_smg_damage_tick",first_smg_damage_tick},{"post_smg_vitality_samples",post_smg_vitality_samples},
            {"damage_scope","Actual full-generation Warthog vitality decrease after an observed native SMG ammo/shot transition"}});
        nlohmann::json released=nlohmann::json::array(), detonated=nlohmann::json::array();
        for (const auto &[identity,record]:grenade_projectiles) {(void)identity;released.push_back(record);}
        for (const auto &[identity,record]:grenade_detonations) {(void)identity;detonated.push_back(record);}
        data["precondition"]={{"observed",precondition_observed},{"grenade_commands",grenade_commands},
            {"grenade_projectiles",std::move(released)},{"grenade_detonations",std::move(detonated)},
            {"body_before",body_before},{"body_after",body_after},
            {"scope","Ordinary pre-fire frag, full native release/detonation and surviving Warthog damage; no effect birth causality inferred"}};
        const auto text=data.dump(2)+"\n";
        if (text.size()>262144) throw FormatError("Loop-audio report exceeds its bounded extent");
        std::ofstream stream(output,std::ios::binary|std::ios::trunc);stream<<text;
        if (!stream) throw FormatError("Cannot retain the owned loop-audio report");
    };
    MultiplayerRuntimeOutcome outcome; int result=1;
    try {
        prepared=std::make_shared<MultiplayerPreparedWorld>(options);
        options.prepared_world=prepared;
        if (!prepared->cache->is_mcc() || !prepared->source_objects || !prepared->source_objects->owns_physics())
            throw FormatError("Loop-audio report requires the actual MCC source world");
        smg=prepared->cache->slayer_starting_weapon(5);
        const auto &definition=prepared->content.weapons.at(smg);
        if (!definition.smg || definition.sound==0xffffffffu)
            throw FormatError("Loop-audio report lost its authored SMG firing sound");
        firing_sound=definition.sound;
        smg_projectile=definition.projectile;
        const auto *warthog=prepared->cache->query_tag({.group=TagGroup::from_string("vehi"),.name="objects\\vehicles\\warthog\\warthog"});
        if (!warthog) throw FormatError("Loop-audio report requires the authored Warthog definition");
        target_definition=warthog->datum;
        const auto binding=prepared->cache->load_first_person_binding(smg,2);
        for (const auto &clip:binding.clock_clips) if (clip.activity==0x0b000065u || clip.activity==0x0c000064u)
            for (const auto &event:clip.events) if (event.type==1 && event.tag!=0xffffffffu) reload_sounds.insert(event.tag);
        result=run_multiplayer(options,outcome); observer.unbind();
        const auto evidence=observer.report(); const auto &p=evidence.at("player");
        const bool passed=result==0 && outcome.reason==MultiplayerExitReason::captured_tick &&
            outcome.authority==MultiplayerOutcomeAuthority::local && outcome.state &&
            outcome.state->tick==options.capture_tick && observer.qualified() && frames && generate && apply &&
            projectiles>=20 && smg_packets && reload_packets && !invalid_packets && precondition_observed &&
            have_body_before_smg && smg_body_damage && post_smg_vitality_samples>=2 &&
            p.at("rounds_consumed").get<std::uint64_t>()>60 &&
            p.at("reload_transfers").get<std::uint64_t>()>0;
        retain(passed,result,&outcome,passed?"":"Required native SMG/mode3/frame evidence is incomplete");
        if (!passed) throw FormatError("Required native SMG/mode3/frame evidence is incomplete");
        return result;
    } catch (const std::exception &error) {
        observer.unbind();retain(false,result,&outcome,error.what());throw;
    }
}
} // namespace halo2::app
