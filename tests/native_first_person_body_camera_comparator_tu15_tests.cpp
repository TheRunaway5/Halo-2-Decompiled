#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_presentation.hpp"
#include "halo2/model_node_interpolation.hpp"
#include "halo2/native_vehicle_camera.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/render_interpolation.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/source_authority_delta.hpp"
#include "halo2/source_presented_model_pose_tu15.hpp"
#include "halo2/source_runtime_undo_journal.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <nlohmann/json.hpp>

// Standalone CPU comparator source. Deliberately not registered with CMake:
// the owning coordinator must first finish and pin its coherent native build.
// This invokes the existing complete camera adapter, never a packet substitute.
using namespace halo2;
namespace {
using Json=nlohmann::json;
void need(bool value,const char* message){if(!value)throw FormatError(message);}
Json xyz(Vec3 p){return Json::array({p.x,p.y,p.z});}
std::uint32_t bits(float x){return std::bit_cast<std::uint32_t>(x);}
bool same(Vec3 a,Vec3 b){return bits(a.x)==bits(b.x)&&bits(a.y)==bits(b.y)&&bits(a.z)==bits(b.z);}
bool same(const NativeDeathCameraView& a,const NativeDeathCameraView& b){
    return same(a.position,b.position)&&same(a.forward,b.forward)&&same(a.up,b.up)&&
        bits(a.horizontal_fov)==bits(b.horizontal_fov)&&bits(a.aim_displacement)==bits(b.aim_displacement)&&
        a.camera_mode==b.camera_mode;
}
bool same(std::span<const Transform> a,std::span<const Transform> b){
    if(a.size()!=b.size())return false;
    for(unsigned n=0;n<a.size();++n){
        if(bits(a[n].scale)!=bits(b[n].scale)||!same(a[n].position,b[n].position))return false;
        for(unsigned axis=0;axis<3;++axis)if(!same(a[n].basis[axis],b[n].basis[axis]))return false;
    }
    return true;
}
Json view(const NativeDeathCameraView& c){
    return {{"position",xyz(c.position)},{"forward",xyz(c.forward)},{"up",xyz(c.up)},
        {"horizontal_fov",c.horizontal_fov},{"aim_displacement",c.aim_displacement},{"mode",c.camera_mode}};
}
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p,unsigned count=4){
    const auto b=m.view(p,count);std::uint32_t x{};
    for(unsigned i=0;i<count;++i)x|=std::to_integer<std::uint32_t>(b[i])<<(8*i);
    return x;
}
std::uint64_t transform_hash(std::span<const Transform> values){
    // Hash defined values individually, never C++ padding.
    std::uint64_t h=14695981039346656037ull;
    const auto add=[&](float f){auto w=bits(f);for(unsigned i=0;i<4;++i)h=(h^((w>>(8*i))&255u))*1099511628211ull;};
    for(const auto& t:values){add(t.scale);for(const auto b:t.basis){add(b.x);add(b.y);add(b.z);}add(t.position.x);add(t.position.y);add(t.position.z);}
    return h;
}
class FixedClock final:public PhysicsLiveClockSource {
public:
    // Same admitted external-clock seam as native camera continuation tests.
    // Constants remove unrelated wall/UTC differences between restored trials.
    std::uint32_t milliseconds()const noexcept override{return 1000;}
    std::uint64_t cycles()const noexcept override{return 7000000;}
    std::uint64_t utc_filetime()const noexcept override{return 132000000000000000ull;}
};
NativeDeathCameraView complete(NativeVehicleCamera& camera,const MultiplayerState::Player& p,float seconds){
    std::optional<NativeDeathCameraView> ordinary;
    const auto alternate=camera.update_player(0,p.source_player,p.source_flags,seconds,false,&ordinary);
    need(!alternate&&!camera.death_view(0)&&ordinary&&ordinary->camera_mode==0,
        "Comparator camera did not retain actual ordinary on-foot ownership");
    return *ordinary;
}
struct Endpoint {
    MultiplayerState state;
    Transform root;
    ShadowCasterSphere sphere;
    std::vector<Transform> nodes;
    NativeDeathCameraView eye;
};
const SourceWorldPose& pose(const MultiplayerState& state,std::uint32_t unit){
    const auto found=std::ranges::find(state.source_body_poses,unit,&SourceWorldPose::object);
    need(found!=state.source_body_poses.end()&&!found->nodes.empty(),"Comparator lost its completed full-salt WORLD bank");
    return *found;
}
Json native_head_inputs(MultiplayerObjectRuntime& objects,std::uint32_t unit){
    const auto& memory=objects.memory();const auto body=objects.tu15_world().source_object_body(unit);
    Json words=Json::array();
    for(unsigned offset=0x3c8;offset<0x3d8;offset+=4)words.push_back(word(memory,body+offset));
    return {{"unit_3c8_through_3d4_words",words},
        {"scope","Recorded current native head-state words; the presented-model scope does not interpolate these fields"}};
}
}

int main(int argc,char** argv){
    if(argc!=4){std::cerr<<"Supply actual MCC Lockout MAP TU1.5_XBE bounded_REPORT_JSON\n";return 2;}
    Json report{{"passed",false},{"scope","CPU-only actual native camera order at sampled WORLD inputs versus completed-eye interpolation; no GPU or retail Vista/MCC parity claim"},
        {"cases",Json::array()},{"retained_large_artifacts",Json::array()}};
    try {
        report["phase"]="canonical native world construction";
        const auto scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Run camera comparator through tools/run_test.py");
        MultiplayerOptions options;options.map=argv[1];options.shared_maps=options.map.parent_path();options.gameplay_xbe=argv[2];
        options.local_players=1;options.starting_weapon=5;options.mute=true;options.source_object_world=true;
        options.variant={"Slayer",0,{{2,0},{10,0},{89,5}}};
        options.source_object_savegame_directory=std::filesystem::path(scratch)/"native-camera-comparator-world";
        auto profile=source_default_player_configuration();profile[0x44]=0;options.selected_profiles={profile};
        auto clock=std::make_shared<FixedClock>();MultiplayerPreparedWorld world(options,std::nullopt,clock);
        world.bind_source_local_views(world.local_slots);
        need(world.match.source_engine_owned()&&options.native_spawn_inputs.empty()&&world.local_slots.size()==1,
            "Comparator lost its canonical native Match/local-view constructor");
        auto& objects=*world.source_objects;SourceMatchAuthority authority(world.match,objects);
        SourceMatchAuthority::AuthorityDigest binding{};binding[0]=std::byte{0x6c};binding[31]=std::byte{0x29};
        NativeVehicleCamera camera(objects,*world.cache->original_data());
        const unsigned slot=world.local_slots.front();std::array<Controls,multiplayer_capacity> controls{};
        controls[slot].yaw=3.14159265358979323846f;
        std::optional<Endpoint> previous;
        const auto advance_camera=[&]{
            auto state=world.match.state();const auto& p=state.players[slot];
            if(!p.connected||p.dead||p.source_unit==null_datum){previous.reset();return;}
            need(p.source_parent==null_datum&&p.source_seat==-1,"Camera warm-up entered an unrelated native seat");
            auto eye=complete(camera,p,tick_seconds);state.players[slot].source_player_camera=eye;
            const auto& completed=pose(state,p.source_unit);
            const auto root=objects.object_transform(p.source_unit);const auto sphere=completed.sphere;const auto nodes=completed.nodes;
            previous=Endpoint{std::move(state),root,sphere,nodes,eye};
        };
        report["phase"]="actual pitch-ramp and camera warm-up";
        for(unsigned n=0;n<105;++n){controls[slot].pitch=-std::min(1.5f,.05f*float(n));world.match.step(controls);advance_camera();}
        need(previous.has_value(),"Camera warm-up did not reach an actual living completed player");
        const auto unit=world.match.state().players[slot].source_unit;
        const auto definition=objects.object_definition(unit);
        const auto render_model=world.cache->load_initial_model_definition(definition).render_model;
        const auto hierarchy=world.cache->load_model_hierarchy(render_model);
        const Vec3 accepted_ledge{-6.188333988189697f,-2.5999999046325684f,-5.989999771118164f};
        objects.tu15_world().set_object_position(unit,accepted_ledge);previous.reset();
        report["route"]={{"reference","analysis/first-person-body-tu15-20261029/lockout-void-route-probe.json"},
            {"camera_owner","Independent actual native-camera-driven route: one full camera update follows each native Match step; Match alone does not publish Runtime's ordinary camera field"},
            {"accepted_ledge",xyz(accepted_ledge)},{"canonical_spawn_inputs_empty",true},{"neutral_settle_ticks",30},
            {"yaw",controls[slot].yaw},{"pitch",controls[slot].pitch},{"crouch",false},
            {"warmup_ticks",105},{"camera_update_seconds",tick_seconds},{"external_clock","Declared constant milliseconds/cycles/UTC test provider"}};
        report["model"]={{"unit",unit},{"definition",definition},{"render_model",render_model},{"nodes",hierarchy.bones.size()}};
        report["trial_policy"]={{"fractions",{0.,.25,.5,.75,1.}},
            {"native_elapsed","fraction * 1/30, from an independent copy of the actual preceding completed camera"},
            {"algorithms",{"current_hierarchy_local_SLERP","per_node_WORLD_SLERP_control"}},
            {"elapsed_only_controls","At current-body fraction0.5, retain the same native World, palette and preceding private camera; compare actual seconds0 and1/30 against the existing1/60 sample"},
            {"limitations","WORLD control retains current quaternion sampler; it does not claim Cartographer NLERP or interpolation of native persistent head/aim/crouch state"},
            {"original_camera_chain","NativeVehicleCamera: whole157FE0/171FF0 packet -> register_adopt -> motion -> observer_world"},
            {"node_input_lease","Actual unit+signed116 bank and signed114 byte count through SourcePresentedModelNodeUseTu15 within SourcePresentedModelPoseScopeTu15"}};
        unsigned trials{},fraction_trials{},elapsed_only_trials{},endpoint_controls{},bank_rollbacks{},coupled_restores{},descending{},fast{};
        float minimum_z=accepted_ledge.z,minimum_velocity{};bool died{};std::set<std::string> selected;
        for(unsigned step=0;step<300;++step){
            report["phase"]="actual native void-route step";report["last_route_step"]=step;
            controls[slot].forward=step<30?0.f:1.f;world.match.step(controls);
            auto state=world.match.state();const auto p=state.players[slot];
            report["last_native_tick"]=state.tick;
            minimum_z=std::min(minimum_z,p.position.z);minimum_velocity=std::min(minimum_velocity,p.velocity.z);
            if(p.dead){died=true;break;}
            need(p.source_unit==unit&&p.source_parent==null_datum&&p.source_seat==-1&&p.crouch==0,
                "Comparator route changed its actual unit, seat or ordinary crouch state");
            if(!p.native_grounded&&p.velocity.z<-.1f)++descending;if(p.velocity.z<-6.f)++fast;
            const auto& completed=pose(state,unit);
            const auto root=objects.object_transform(unit);const auto sphere=completed.sphere;const auto nodes=completed.nodes;
            auto before_camera=camera;
            std::string label;
            if(previous&&previous->state.tick+1==state.tick&&!p.native_grounded&&p.velocity.z<-.1f){
                if(!selected.contains("ledge_descent"))label="ledge_descent";
                else if(p.velocity.z<-6.f&&!selected.contains("fast_fall"))label="fast_fall";
                else if(p.position.z<-20.f&&!selected.contains("deep_fall"))label="deep_fall";
            }
            std::optional<SourceMatchAuthority::Checkpoint> before;
            if(!label.empty()){
                report["active_case"]=label;report["phase"]="complete coupled checkpoint before endpoint camera";
                before=authority.checkpoint(binding);
            }
            report["phase"]="actual completed endpoint camera";
            const auto eye=complete(camera,p,tick_seconds);state.players[slot].source_player_camera=eye;
            if(before){
                report["phase"]="complete coupled checkpoint and named-field images after endpoint camera";
                const auto after=authority.checkpoint(binding);const auto after_image=encode_source_authority_image(after,source_shadow_archive_codec_limits());
                const auto before_image=encode_source_authority_image(*before,source_shadow_archive_codec_limits());
                const auto camera_after=camera.checkpoint(binding);
                const auto restore=[&](const auto& checkpoint,const AuthorityArchiveImage& expected){
                    report["phase"]="complete coupled native preparation";
                    auto prepared=authority.prepare(checkpoint,binding);report["phase"]="complete coupled native commit";
                    need(authority.commit(std::move(prepared)),"Comparator could not restore complete native Match/world/sidecars");
                    report["phase"]="exact coupled named-field restoration verification";
                    const auto actual=encode_source_authority_image(authority.checkpoint(binding),source_shadow_archive_codec_limits());
                    need(std::ranges::equal(actual.raw(),expected.raw()),"Comparator complete coupled restoration changed a named field or native bank");++coupled_restores;
                };
                Json row{{"case",label},{"before_tick",previous->state.tick},{"tick",state.tick},{"step",step},
                    {"root",xyz(p.position)},{"velocity",xyz(p.velocity)},{"previous_eye",view(previous->eye)},{"completed_eye",view(eye)},
                    {"native_head_inputs",native_head_inputs(objects,unit)},{"samples",Json::array()}};
                try {
                    for(unsigned algorithm=0;algorithm<2;++algorithm)for(float fraction:std::array{0.f,.25f,.5f,.75f,1.f})for(unsigned elapsed_control=0;elapsed_control<3;++elapsed_control){
                        if(elapsed_control&&(algorithm!=0||fraction!=.5f))continue;
                        const float camera_seconds=elapsed_control==1?0.f:elapsed_control==2?tick_seconds:fraction*tick_seconds;
                        report["active_algorithm"]=algorithm;report["active_fraction"]=fraction;report["active_elapsed_control"]=elapsed_control;
                        restore(*before,before_image);auto trial=before_camera;
                        auto sampled=algorithm==0?interpolate_model_nodes(hierarchy,previous->nodes,nodes,fraction):std::vector<Transform>{};
                        if(algorithm)for(unsigned n=0;n<nodes.size();++n)sampled.push_back(render_transform(previous->nodes[n],nodes[n],fraction));
                        SourcePresentedModelPoseTu15 input{unit,definition,render_model,render_transform(previous->root,root,fraction),
                            {render_lerp(previous->sphere.center,sphere.center,fraction),std::lerp(previous->sphere.radius,sphere.radius,fraction)},sampled};
                        auto shown=multiplayer_presentation(previous->state,state,fraction);
                        need(shown.players[slot].source_player_camera.has_value(),"Completed-eye comparator lost its retained native endpoints");
                        NativeDeathCameraView actual;
                        {
                            SourceRuntimeUndoJournal undo(objects.memory(),SourceRuntimeUndoJournal::Storage::packed,
                                [](auto,auto){throw FormatError("Camera comparator must not allocate or remount native topology");});
                            SourcePresentedModelPoseScopeTu15 presentation(objects.tu15_world(),std::span(&input,1));
                            const auto body=objects.tu15_world().source_object_body(unit);
                            const auto bytes=std::bit_cast<std::int16_t>(std::uint16_t(word(objects.memory(),body+0x114u,2)));
                            const auto offset=std::bit_cast<std::int16_t>(std::uint16_t(word(objects.memory(),body+0x116u,2)));
                            need(bytes>0&&offset>=0&&std::size_t(bytes)==sampled.size()*sizeof(Transform),
                                "Comparator sampled bank differs from the actual native WORLD allocation");
                            SourcePresentedModelNodeUseTu15 node_input(objects.memory());
                            node_input.publish(unit,render_model,body+std::uint32_t(offset),unsigned(sampled.size()));
                            std::vector<Transform> actual_bank(sampled.size());
                            objects.memory().read(body+std::uint32_t(offset),std::as_writable_bytes(std::span(actual_bank)));
                            need(same(actual_bank,sampled),"Camera comparator failed to publish its exact typed WORLD input");
                            report["phase"]="actual sampled native camera chain";
                            actual=complete(trial,p,camera_seconds);
                        }
                        report["phase"]="complete bank rollback and endpoint controls";
                        need(objects.memory().checkpoint()==before->world.world.bank,"Packed camera trial failed to roll back the complete native bank");++bank_rollbacks;
                        const auto sampled_eye=*shown.players[slot].source_player_camera;
                        if(fraction==1){
                            need(same(actual,eye),"Sampled native camera changed its unaltered fraction1 endpoint");
                            need(NativeVehicleCamera::same_checkpoint(trial.checkpoint(binding),camera_after),
                                "Sampled native camera did not reproduce the complete fraction1 private camera state");++endpoint_controls;
                        }
                        row["samples"].push_back({{"algorithm",algorithm?"per_node_WORLD_SLERP_control":"current_hierarchy_local_SLERP"},
                            {"fraction",fraction},{"elapsed_control",elapsed_control==0?"fractional":elapsed_control==1?"zero":"full_tick"},
                            {"native_seconds",camera_seconds},{"native_eye",view(actual)},
                            {"completed_eye_interpolation",view(sampled_eye)},{"eye_difference",xyz(actual.position-sampled_eye.position)},
                            {"eye_distance",length(actual.position-sampled_eye.position)},
                            {"pelvis_relative_native_eye",xyz(sampled.front().position-actual.position)},
                            {"pelvis_relative_completed_eye",xyz(sampled.front().position-sampled_eye.position)},
                            {"sampled_world_nodes_fnv64",transform_hash(sampled)},{"complete_native_bank_rolled_back",true}});++trials;
                        if(elapsed_control)++elapsed_only_trials;else ++fraction_trials;
                    }
                }catch(...){restore(after,after_image);throw;}
                restore(after,after_image);
                need(NativeVehicleCamera::same_checkpoint(camera.checkpoint(binding),camera_after),"Comparator mutated the baseline private camera owner");
                selected.insert(label);report["cases"].push_back(std::move(row));
            }
            previous=Endpoint{std::move(state),root,sphere,nodes,eye};
        }
        need(died&&descending>=30&&fast>=5&&minimum_z<-20.f&&selected.size()==3,
            "Native camera comparator did not traverse its actual ledge, fast-fall and deep-void route");
        need(trials==36&&fraction_trials==30&&elapsed_only_trials==6&&endpoint_controls==6&&bank_rollbacks==trials&&coupled_restores==39,
            "Native camera comparator omitted endpoint or complete restoration controls");
        report["phase"]="complete";
        report.update({{"passed",true},{"trials",trials},{"exact_fraction1_camera_controls",endpoint_controls},
            {"fraction_trials",fraction_trials},{"elapsed_only_trials",elapsed_only_trials},
            {"complete_native_bank_rollbacks",bank_rollbacks},{"complete_coupled_restores",coupled_restores},
            {"minimum_root_z",minimum_z},{"minimum_vertical_velocity",minimum_velocity},{"descending_ticks",descending},
            {"fast_ticks",fast},{"actual_native_death",died},
            {"interpretation","Differences are comparator measurements. A passed probe qualifies endpoint and restoration controls, not a visible-bob fix or retail parity."}});
    }catch(const std::exception& e){report["error"]=e.what();}
    std::ofstream output(argv[3]);output<<report.dump(2)<<'\n';
    if(!output){std::cerr<<"Could not retain bounded camera comparator report\n";return 1;}
    if(!report["passed"].get<bool>())std::cerr<<report.value("error",std::string("Native camera comparator failed"))<<'\n';
    return report["passed"].get<bool>()?0:1;
}
