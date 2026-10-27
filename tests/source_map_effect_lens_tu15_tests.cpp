// Read-only source/map presentation observations in an owned diagnostic cohort.
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_map_effects.hpp"
#include "halo2/source_effect_replica_render_tu15.hpp"
#include "halo2/multiplayer_network.hpp"
#include "helpers/multiplayer_snapshot_checks.hpp"
#include <thread>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=3||!std::getenv("HALO2_TEST_SCRATCH"))throw FormatError("Supply prepared MCC MAP REPORT under owned scratch");
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    MultiplayerOptions options;options.map=argv[1];options.shared_maps=options.map.parent_path();
    options.local_players=2;options.mute=true;options.source_object_world=true;
    MultiplayerPreparedWorld world(options);world.bind_source_local_views(world.local_slots);
    Mesh mesh;CollisionWorld collision(world.content.collision);
    NativeWorldEffects effects(*world.cache,mesh,collision,0x1337);
    SourceMapEffects map_effects(*world.cache,world.content.objects,effects);
    nlohmann::json report{{"map",argv[1]},{"local_players",2},{"source_effects",nlohmann::json::array()},
        {"render_requests",0},{"GPU_or_gameplay_pixels_verified",false},
        {"declared_input","Native observer camera placed above an actual authored spark object; no effect/visibility/admission result supplied"}};
    const auto target=world.cache->query_tag({.group=TagGroup::from_string("scen"),
        .name="effects\\scenery\\sparks\\spark_iceblue_large",.require_address=true});
    if(!target)throw FormatError("Actual map has no authored spark scenery definition");
    std::array<Controls,multiplayer_capacity> controls{};unsigned observations{},active{};
    LanHost authority(world.match,0,0x17c710u);
    LanClient replica(authority.join_address(),0x17c710u,2);
    std::array<Controls,2> remote{};
    for(unsigned n=0;n<128&&!replica.ready();++n) {
        replica.send(remote);authority.receive(controls);authority.broadcast();replica.receive();
        if(!replica.ready())std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if(!replica.ready()||replica.slots().size()!=2)throw FormatError("Actual two-local-player UDP peer failed to join");
    unsigned network_snapshots{};
    for(unsigned step=0;step<600;++step) {
        auto state=world.match.state();
        for(const auto &root:state.source_scene_roots)if(root.definition==target->datum)
            for(unsigned view=0;view<2;++view)
                world.source_objects->publish_native_observer_position(view,root.transform.position+Vec3{0,0,1});
        world.match.step(controls);world.source_objects->advance_effect_frame(tick_seconds);state=world.match.state();
        authority.broadcast();replica.receive();
        test::verify_multiplayer_snapshot(state,replica.state());++network_snapshots;
        std::vector<std::vector<std::uint8_t>> permutations;permutations.reserve(world.match.simulation().scene().objects().size());
        std::vector<SourceSceneEffectOwner> owners;
        for(const auto &object:world.match.simulation().scene().objects())if(object.handle!=null_datum&&object.solved_nodes&&!object.solved_nodes->empty()) {
            const auto type=world.content.objects.at(object.placement).type;if(type!=6&&type!=7&&type!=11)continue;
            permutations.push_back(world.source_objects->object_model_regions(object.handle).selection.permutations);
            owners.push_back({object.placement,object.handle,object.created_tick,*object.solved_nodes,permutations.back(),object.position});
        }
        map_effects.synchronize(owners,state.tick,state.source_scene_roots);effects.advance(state.tick,tick_seconds);
        report["render_requests"]=report["render_requests"].get<unsigned>()+effects.replica_lenses().size();
        for(const auto &root:state.source_scene_roots)if(root.definition==target->datum)
            for(const auto &effect:root.attached_effects)if(effect.instance!=null_datum) {
                const auto prefix=source_effect_replica_render_tu15(world.source_objects->tu15_world().memory(),effect.instance,effect.definition);
                const auto live=world.source_objects->live_objects();
                const auto owner=std::find_if(live.begin(),live.end(),[&](const auto &o){return o.object==root.object;});
                if(owner==live.end())throw FormatError("Actual spark effect lost its live object owner");
                ++observations;active+=bool(prefix.flags&1u)&&!(prefix.flags&0x4cu);
                const auto &memory=world.source_objects->tu15_world().memory();
                const auto pool=Bytes(memory.view(0x4f720cu,4)).u32(0);
                const auto body=Bytes(memory.view(pool+0x44u,4)).u32(0)+(effect.instance&65535u)*400u;
                const Bytes clock(memory.view(body+0x5cu,16));
                if(step%100==0||((prefix.flags&1u)&&active<=3))report["source_effects"].push_back({{"tick",state.tick},{"object",root.object},
                    {"object_type",owner->type},{"parent",owner->parent},{"effect",effect.definition},{"identity",effect.instance},
                    {"flags",prefix.flags},{"event",clock.u16(0)},{"elapsed",clock.f32(4)},{"target",clock.f32(8)},
                    {"attached_owner",prefix.attached_owner},{"local_view",prefix.local_view},
                    {"root_owner",prefix.root_owner},{"root_type",prefix.root_type},
                    {"color",{prefix.color.x,prefix.color.y,prefix.color.z}},
                    {"velocity",{prefix.velocity.x,prefix.velocity.y,prefix.velocity.z}},
                    {"locations",effect.locations.size()}});
            }
    }
    const auto stats=map_effects.stats();report["source_observations"]=observations;report["active_source_observations"]=active;
    report["native_spawned"]=stats.spawned;report["native_gaps"]=stats.unavailable;
    report["actual_UDP_snapshots_compared"]=network_snapshots;report["actual_joined_local_players"]=replica.slots().size();
    std::ofstream(argv[2])<<report.dump(2)<<'\n';
    if(!observations||!active)throw FormatError("Actual authored spark did not expose an active source allocation");
    if(report["render_requests"].get<unsigned>()!=active || network_snapshots!=600)
        throw FormatError("Current source spark phases or exact actual UDP snapshots lost their lens producer");
    std::cout<<"Observed actual two-view source spark allocation and native map presentation\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
