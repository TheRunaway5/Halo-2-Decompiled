#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_first_person_draws_tu15.hpp"
#include "halo2/model_skin_submission.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#if defined(__i386__) || defined(__x86_64__)
#include <x86intrin.h>
#endif
using namespace halo2;
namespace {
void need(bool value,const char *message){if(!value)throw FormatError(message);}
std::uint32_t word(const SourceRuntimeMemory &memory,std::uint32_t at){return Bytes(memory.view(at,4)).u32(0);}
struct VisibilityInput {
    SourceRuntimeMemory &memory;std::array<std::uint32_t,2> addresses,original;
    VisibilityInput(SourceRuntimeMemory &m,std::uint32_t view,unsigned hidden):memory(m) {
        for(unsigned hand=0;hand<2;++hand) {
            addresses[hand]=view+0xcu+hand*0x1010u;original[hand]=word(memory,addresses[hand]);
            need((original[hand]&7u)==7u,"Visibility fixture requires genuinely completed visible native hands");
            const auto flags=original[hand]&~((hidden&(1u<<hand))?2u:0u);
            memory.write(addresses[hand],std::as_bytes(std::span(&flags,1)));
        }
    }
    ~VisibilityInput(){for(unsigned h=0;h<2;++h)memory.write(addresses[h],std::as_bytes(std::span(&original[h],1)));}
};
struct Inputs final:SourcePhysicsContactMemory {
    const SourceRuntimeMemory &source;std::vector<std::pair<std::uint32_t,unsigned>> denied;
    Inputs(const SourceRuntimeMemory &m,std::uint32_t view,std::uint32_t locals):source(m) {
        for(unsigned h=0;h<2;++h){denied.emplace_back(view+0xcu+h*0x1010u+0x304u,0xd04u);denied.emplace_back(locals+h*0x1000u,0x800u);}
        denied.emplace_back(view+0x2098u,0x34u);denied.emplace_back(0x60050000u,4u*0xd0cu);
    }
    void read(std::uint32_t at,std::span<std::byte> bytes)const override {
        for(auto [base,count]:denied)if(std::uint64_t(at)<std::uint64_t(base)+count && std::uint64_t(base)<std::uint64_t(at)+bytes.size())
            throw FormatError("Visibility native consumer borrowed a completed FP output");
        source.read(at,bytes);
    }
    void write(std::uint32_t,std::span<const std::byte>)override{throw FormatError("Visibility source inputs are readonly");}
};
float difference(std::span<const Transform> a,std::span<const Transform> b) {
    need(a.size()==b.size(),"Visibility sampler changed model node count");float maximum{};
    for(unsigned n=0;n<a.size();++n) {
        const auto x=std::bit_cast<std::array<float,13>>(a[n]),y=std::bit_cast<std::array<float,13>>(b[n]);
        for(unsigned k=0;k<13;++k){need(std::isfinite(x[k])&&std::isfinite(y[k]),"Visibility matrix is not finite");maximum=std::max(maximum,std::abs(x[k]-y[k]));}
    }return maximum;
}
}
int main(int argc,char **argv)try {
    need(argc==5,"Supply MCC map, shared maps, bounded report, representation2|3");
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Run native visibility through owned scratch");
    const unsigned rep=std::stoul(argv[4]);need(rep==2||rep==3,"Visibility fixture requires human or Elite");
    MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.local_players=1;
    options.starting_weapon=5;options.mute=true;options.source_object_world=true;
    options.variant=UiVariant{"Slayer",0,{{2,0},{10,0},{89,5}}};
    options.source_object_savegame_directory=std::filesystem::path(scratch)/"native-visibility";
    if(rep==3){auto profile=source_default_player_configuration();profile[0x44]=1;options.selected_profiles={profile};}
    MultiplayerPreparedWorld world(options);world.bind_source_local_views(world.local_slots);
    const auto slot=world.local_slots.at(0),weapon=world.cache->slayer_starting_weapon(5);
    std::array<Controls,multiplayer_capacity> controls{};unsigned settled{};
    for(unsigned tick=0;tick<240 && settled<5;++tick) {
        const auto state=world.match.state();const auto &p=state.players[slot];controls[slot]={};
        if(p.source_unit!=null_datum && p.secondary_weapon.datum==null_datum) {
            const auto item=std::find_if(state.world_weapons.begin(),state.world_weapons.end(),[&](const auto &w){return w.definition==weapon;});
            if(item!=state.world_weapons.end()) {
                world.source_objects->tu15_world().set_object_position(p.source_unit,item->position+Vec3{-.2f,0,0});
                controls[slot].dual_wield=true;controls[slot].dual_wield_milliseconds=1000;
            }
        }
        world.match.step(controls);if(world.match.state().players[slot].secondary_weapon.datum!=null_datum)++settled;
    }
    need(settled==5,"Visibility fixture did not acquire its authentic dual pair");
    auto &native=world.source_objects->tu15_world();auto &memory=world.source_objects->memory();
    const auto view=world.source_objects->first_person_view(0);
    const auto definition=world.cache->load_first_person_binding(weapon,rep);
    const auto arms=world.cache->load_model(definition.arms_model,false),gun=world.cache->load_model(definition.weapon_model,false);
    const SourceFirstPersonDrawBindingTu15 binding{view.unit,definition.arms_model,
        {view.hands[0].weapon,view.hands[1].weapon},{definition.weapon_model,definition.weapon_model}};
    SourcePhysicsWorldRuntimeServices cpu;
#if defined(__i386__) || defined(__x86_64__)
    cpu.reciprocal_sqrt=[](auto b){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(std::bit_cast<float>(b)))));};
    cpu.timestamp_counter=[](){return __rdtsc();};
#else
    throw FormatError("Visibility proof requires actual native CPU instruction services");
#endif
    nlohmann::json rows=nlohmann::json::array();unsigned previous_admission_rejections{};float maximum{};
    for(unsigned camera=0;camera<2;++camera)for(unsigned hidden=0;hidden<4;++hidden) {
        const Vec3 forward=camera?Vec3{.6f,0,-.8f}:Vec3{1,0,0},up=camera?Vec3{.8f,0,.6f}:Vec3{0,0,1};
        const auto view_at=word(memory,0x4f6498u),locals=word(memory,0x4f6494u);
        VisibilityInput declared_input(memory,view_at,hidden);
        world.source_objects->begin_model_frame();std::vector<SourceFirstPersonModelTu15> original;
        world.source_objects->render_model_view([&](auto &source){original=source.prepare_first_person_models(0,{1,-2,.5f},forward,up);});
        Inputs inputs(memory,view_at,locals);
        const auto reproduced=source_sample_first_person_pose_tu15(*world.cache,inputs,native.prepare_source_call(),0,{1,-2,.5f},forward,up,cpu);
        need(original.size()==reproduced.size(),"Visibility changed native collector cardinality");
        for(unsigned n=0;n<original.size();++n) {
            need(original[n].owner==reproduced[n].owner && original[n].render_model==reproduced[n].render_model && original[n].mode==reproduced[n].mode,
                "Visibility native consumer changed collector ownership");maximum=std::max(maximum,difference(original[n].nodes,reproduced[n].nodes));
        }
        const auto draws=source_first_person_draws_tu15(reproduced,binding);
        need(draws.arms,"Original visibility omitted the shared arms input");
        std::array<std::size_t,2> gun_vertices{};
        for(unsigned h=0;h<2;++h) {
            need(bool(draws.weapons[h])==!(hidden&(1u<<h)),"Production draw plan ignored actual native visibility omission");
            if(draws.weapons[h])gun_vertices[h]=model_skin_world_pose(gun,draws.weapons[h]->nodes,false).vertices.size();
        }
        const auto arm_vertices=model_skin_world_pose(arms,draws.arms->nodes,false).vertices.size();need(arm_vertices>0,"Omitted native guns discarded shared arms geometry");
        const auto foreground=std::count_if(original.begin(),original.end(),[](const auto &m){return m.mode==1;});
        const auto expected=1u+unsigned(!(hidden&1u))+unsigned(!(hidden&2u));need(unsigned(foreground)==expected,"Original168DED flag2 decision differs from source witness");
        if(foreground!=3)++previous_admission_rejections;
        rows.push_back({{"camera",camera},{"hidden_weapon_mask",hidden},{"foreground_models",foreground},{"arms_vertices",arm_vertices},
            {"weapon_vertices",gun_vertices},{"old_bound_hand_admission_would_reject",foreground!=3}});
    }
    need(maximum<=.0001f && previous_admission_rejections==6,"Native visibility decision proof did not cover admissible omissions");
    nlohmann::json report{{"passed",true},{"representation",rep},{"native_kernels",true},{"runtime_emulation",false},
        {"scope","Declared native flag2 render inputs through actual16A4DD/168DED and production draw-plan/skin consumer; no gameplay visibility producer claim"},
        {"maximum_component_difference",maximum},{"prior_admission_red_cases",previous_admission_rejections},{"cases",rows}};
    std::ofstream(argv[3])<<report.dump(2)<<'\n';std::cout<<"PASS native visibility decisions and retained shared arms\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
