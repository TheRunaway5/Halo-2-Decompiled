#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <sys/wait.h>
#include <unistd.h>
#include <algorithm>
#include <bit>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
using namespace halo2;
using J=nlohmann::json;
namespace {
void check(bool value,const char* why){if(!value)throw FormatError(why);}
std::string sha(std::span<const std::byte> bytes) {
    unsigned char digest[SHA256_DIGEST_LENGTH];SHA256(reinterpret_cast<const unsigned char*>(bytes.data()),bytes.size(),digest);
    std::string out;constexpr char digits[]="0123456789abcdef";
    for(auto b:digest){out+=digits[b>>4];out+=digits[b&15];}return out;
}
J inventory(const SourceRuntimeMemory& m) {
    J out=J::array();m.visit_committed([&](auto at,auto bytes){out.push_back({at,bytes.size(),sha(bytes)});});return out;
}
J state(const SourcePhysicsWorldRuntimeAbi& a) {
    return {{"gp",a.gp},{"fp",{a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr}},
        {"xmm",a.xmm},{"eflags",a.eflags}};
}
J load(const std::filesystem::path& p){std::ifstream f(p);check(bool(f),"Missing child-world receipt");return J::parse(f);}
}
int main(int argc,char** argv)try {
    check(argc==4,"Supply TU15_XBE XBOX_MAP REPORT");
    if(!std::filesystem::is_regular_file(argv[1])||!std::filesystem::is_regular_file(argv[2]))return 77;
    check(std::getenv("HALO2_TEST_SCRATCH"),"Child-world comparison needs owned scratch");
    const std::filesystem::path scratch=std::getenv("HALO2_TEST_SCRATCH");
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    const auto tag=[&](const char* group,const char* fragment) {
        const auto at=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto& t){return t.group==group&&t.name.find(fragment)!=std::string::npos;});
        check(at!=cache.tags().end(),"Authored child-world tag is absent");return at->datum;
    };
    const auto biped=tag("bipd","masterchief"),weapon=tag("weap","smg");
    PhysicsLiveWorldTu15 world(cache,argv[1],scratch/"saves");auto& m=world.memory();
    unsigned premature{};
    {
        SourcePhysicsEntryObserverBinding entries(m,[&](bool before,auto entry,const auto&){if(before&&entry==0x2cbd0u)++premature;});
        world.configure_session(source_game_variant_default(0),1,30,0x12345678u);
        for(auto definition:{biped,weapon})world.preload_render_model(cache.load_initial_model_definition(definition).render_model);
        world.initialize_map();
    }
    const auto position=cache.spawns().at(0).position;
    const auto object=world.create_biped(biped,position,{1,0,0});
    const auto child=world.equip_weapon(object,weapon);check(child!=0xffffffffu,"Original equipment owner refused the child weapon");
    const auto word=[&](auto at){return Bytes(m.view(at,4)).u32(0);};
    const auto body=[&](auto owner){return word(word(word(0x4ecbd0u)+0x44u)+12u*(owner&65535u)+8u);};
    check(word(body(child)+0x14u)==object&&word(body(object)+0x10u)!=0xffffffffu,"Original equipment did not attach a real child");
    {
        SourcePhysicsEntryObserverBinding entries(m,[&](bool before,auto entry,const auto&){if(before&&entry==0x2cbd0u)++premature;});
        world.begin_model_frame();world.begin_model_view();
        (void)world.build_world_model(object,0,1,false,4.f,1.f);
        (void)world.build_world_model(child,0,1,false,4.f,1.f);
        world.begin_model_frame();world.begin_model_view();
    }
    check(!premature,"Reference switch was used before the forked comparison");
    const auto put=[&](std::uint32_t at,const auto& value){world.write_source_call_input(at,std::as_bytes(std::span(&value,1)));};
    constexpr auto context=0x5598f8u;
    const auto queue=word(context+0x14u);check(word(queue)>1,"Original object queue has no usable slot");
    put(queue+4u,std::uint16_t(1));put(word(queue+8u),std::uint16_t(0x3000));put(word(queue+0xcu),object);
    put(word(queue+0x10u),std::uint32_t(0));put(word(queue+0x14u),std::uint16_t(0xffff));
    put(context+0x2a78u,std::uint32_t(1));put(context+0x2a7cu,std::uint32_t(1));put(0x4cd3a0u,context);
    SourceFrustumProjectionInput projection;projection.projection_angle=1.2217304706573486f;
    projection.outer={0,0,480,640};projection.safe=projection.outer;
    const Vec3 eye{position.x-4.f,position.y,position.z},forward{1,0,0};
    const auto camera=source_view_projection({eye,forward,{0,0,1},.015625f,1024.f,{},source_frustum_projection(projection).projection_program});
    put(0x4c6634u,camera);put(0x4c65c0u,eye);put(0x4c65ccu,forward);put(0x4c65f0u,projection.outer);
    for(const auto at:{0x4c65ecu,0x4c6848u})put(at,1.f);
    for(const auto at:{0x4cd398u,0x4cd39cu,0x4cdd48u,0x4ce1ccu,0x4c6710u,0x4c681cu,0x4c6870u})put(at,std::uint32_t(0));
    put(0x4c67acu,0xffffffffu);put(0x4c683cu,std::uint32_t(4));
    for(const auto at:{0x4c6839u,0x4c6824u,0x515ef0u,0x515ef1u})put(at,std::uint8_t(0));
    const auto before=inventory(m);J reports=J::array();
    for(const std::string role:{"reference","native","native-repeat"}) {
        const auto output=scratch/role;std::filesystem::create_directories(output);
        const auto pid=::fork();check(pid>=0,"Cannot fork the identical owned World");
        if(!pid) {
            try {
                ::setenv("HALO2_REFERENCE_MODEL_DRAW_UNITS",role=="reference"?"1":"",1);
                J calls=J::array();unsigned child_calls{},after_model{};std::uint32_t active_models{};
                world.observe_physics_stores_read_only([&](auto pc,const auto& event){
                    check(pc==event.instruction,"Child-world store lost its diagnostic PC");
                    if(pc==0x136a27u){++child_calls;after_model+=active_models!=0;}
                });
                SourcePhysicsEntryObserverBinding entries(m,[&](bool before,auto entry,const auto& a){
                    if(entry==0x2cbd0u&&!before){calls.push_back(state(a));++active_models;}
                });
                auto a=world.prepare_source_call(std::array{1u});const auto sp=a.gp[4];a.gp[0]=context;
                world.run_actual(0x136230u,a);
                check(!calls.empty()&&child_calls&&after_model,"Whole136230 did not build a child after a model append");
                std::ofstream banks(output/"after.bin",std::ios::binary);J regions=J::array();
                m.visit_committed([&](std::uint32_t at,auto bytes){
                    // Bytes below the completed parent SP belong to retired
                    // private callees. Keep all persistent banks and live stack.
                    if(at==0x60000000u){bytes=bytes.subspan(sp-at);at=sp;}
                    const auto n=std::uint32_t(bytes.size());banks.write(reinterpret_cast<const char*>(&at),4);
                    banks.write(reinterpret_cast<const char*>(&n),4);banks.write(reinterpret_cast<const char*>(bytes.data()),n);
                    regions.push_back({at,n,sha(bytes)});
                });
                banks.close();check(bool(banks),"Cannot write owned child-world comparison banks");
                std::ofstream receipt(output/"receipt.json");receipt<<J{{"role",role},{"state",state(a)},{"model_calls",calls},
                    {"child_builds",child_calls},{"child_builds_after_model",after_model},{"regions",regions}}.dump(2)<<'\n';
                receipt.close();check(bool(receipt),"Cannot write child-world receipt");::_exit(0);
            }catch(const std::exception& error){std::cerr<<role<<": "<<error.what()<<'\n';::_exit(1);}
        }
        int status{};check(::waitpid(pid,&status,0)==pid&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"Child-world execution failed");
        check(inventory(m)==before,"Forked collection changed the parent World backing");reports.push_back(load(output/"receipt.json"));
    }
    for(unsigned n=1;n<reports.size();++n) {
        check(reports[n]["regions"]==reports[0]["regions"],"Native child-world persistent banks or live stack differ");
        const auto& a=reports[n]["state"];const auto& b=reports[0]["state"];
        check(a["fp"]==b["fp"],"Native child-world complete FP status differs");
        for(unsigned r=3;r<8;++r)check(a["gp"][r]==b["gp"][r],"Native child-world consumed return GP differs");
        check(reports[n]["child_builds"]==reports[0]["child_builds"]&&reports[n]["model_calls"].size()==reports[0]["model_calls"].size(),"Native child-world original call history differs");
        for(unsigned call=0;call<reports[0]["model_calls"].size();++call) {
            const auto& a=reports[n]["model_calls"][call];const auto& b=reports[0]["model_calls"][call];
            check(a["fp"]==b["fp"]&&a["gp"][0]==b["gp"][0],"Native child-world model return or FP differs");
            for(unsigned r=3;r<8;++r)check(a["gp"][r]==b["gp"][r],"Native child-world model callee-save differs");
        }
        for(unsigned call=0;call<reports[0]["model_calls"].size();++call)for(unsigned r:{0u,1u})for(unsigned lane=1;lane<4;++lane)
            check(reports[n]["model_calls"][call]["xmm"][r][lane]==reports[0]["model_calls"][call]["xmm"][r][lane],"Native child-world MXCSR-live upper lane differs");
    }
    const J report={{"passed",true},{"owner",object},{"weapon_child",child},{"runs",reports},{"forked_parent_unchanged",true},
        {"retained_large_artifacts",J::array()},{"scope","Identical genuinely constructed Worlds; whole136230, real equipped child, native/genuine2CBD0, every persistent bank/live parent stack, complete FP status and MXCSR-live upper lanes. Private callee stack and value-dead XMM residues are excluded."}};
    std::ofstream(argv[3])<<report.dump(2)<<'\n';
    std::cout<<"PASS whole136230 real equipped child, "<<reports[0]["model_calls"].size()<<" model appends, "<<reports[0]["child_builds_after_model"]<<" child builds after model; full banks and FP\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
