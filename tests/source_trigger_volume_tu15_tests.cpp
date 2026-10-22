#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_scene_device_constructor_tu15.hpp"
#include "halo2/scene.hpp"
#include <bit>
#include <iostream>
#include <stdexcept>

using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
void original_queries() {
    auto bank=native_tu15_world_memory({0x60212000u,{},"trigger test clock input"});
    constexpr unsigned data=0x90000000u,volume=data+0x200u,names=data+0x400u,
        context=data+0x1000u,objects=data+0x1100u,object=data+0x1200u,
        tags=data+0x2000u,definition=data+0x2400u,model=data+0x2800u,
        nodes=data+0x2c00u,stack=data+0x18000u,point=stack+4u;
    bank.mount(data,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,
        "explicit trigger caller scenario, named object, WORLD node and stack inputs");
    const auto put=[&](unsigned address,unsigned value,unsigned count=4) {
        std::array<std::byte,4> bytes{};
        for(unsigned i=0;i<count;++i)bytes[i]=std::byte(value>>(i*8));
        bank.write(address,std::span(bytes).first(count));
    };
    const auto scalar=[&](unsigned address,float value){put(address,std::bit_cast<unsigned>(value));};
    for(auto [cell,value]:{std::pair{0x4ecc20u,data},std::pair{0x4eaba0u,names},
            std::pair{0x4ecbd0u,context},std::pair{0x4f0414u,tags}}) {
        try{(void)bank.view(cell,4);}catch(const std::out_of_range&){
            bank.mount(cell,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"trigger caller world binding");
        }
        put(cell,value);
    }
    put(data+0x108,1);put(data+0x10c,volume);put(volume+4,0xffff,2);
    scalar(volume+0xc,1);scalar(volume+0x20,1);
    scalar(volume+0x30,2);scalar(volume+0x34,4);scalar(volume+0x38,6);
    const auto query=[&](Vec3 p) {
        put(stack,0x70000000u);scalar(point,p.x);scalar(point+4,p.y);scalar(point+8,p.z);
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp[0]=0;abi.gp[4]=stack;abi.gp[7]=point;abi.eflags=2;
        SourcePhysicsWorldRuntimeServices services{};unsigned instructions=0;
        services.instruction=[&](auto){check(++instructions<2048,"Trigger query escaped its explicit caller inputs");};
        services.required_owner=[&](SourcePhysicsContactMemory &memory,const SourcePhysicsContactRuntimeCall &call,
            SourcePhysicsWorldRuntimeAbi &state,const SourcePhysicsContactRecordObserver &observe) {
            check(call.target==0x144bf0u,"Trigger fixture reached an undeclared original callee");
            source_run_scene_device_constructor_tu15(memory,call.target,state,{0x27f,0x1f80},services,observe);
        };
        source_run_physics_live_support_tu15(bank,0x11eba0u,abi,{0x27f,0x1f80},services);
        check(abi.gp[4]==stack+4,"Original trigger caller stack was not restored");
        return (abi.gp[0]&255u)!=0;
    };
    check(query({1,2,3}),"Actual TU1.5 unattached volume rejected its interior");
    for(auto p:{Vec3{0,2,3},Vec3{2,2,3},Vec3{1,0,3},Vec3{1,4,3},Vec3{1,2,0},Vec3{1,2,6}})
        check(!query(p),"Actual TU1.5 volume admitted a closed face");
    put(volume+4,0,2);put(names,0xffffffffu);
    check(!query({1,2,3}),"Missing named parent acquired an invented transform");
    for(auto name:{0x280u,0xfffeu}){put(volume+4,name,2);check(!query({1,2,3}),"Invalid parent name was admitted");}
    put(volume+4,0,2);put(names,0x12340000u);put(context+0x44,objects);put(objects+8,object);
    put(object,0x23450000u);put(tags+8,definition);put(definition+0x38,0x34560001u);
    put(tags+0x18,model);put(model+0x78,1);put(model+0x7c,nodes);put(nodes,42);put(volume+8,41);
    check(!query({1,2,3}),"Missing authored node acquired an invented root transform");
    put(volume+8,42);put(object+0x116,0x200,2);
    const unsigned world=object+0x200;scalar(world,2);
    scalar(world+8,1);scalar(world+16,-1);scalar(world+36,1);
    scalar(world+40,10);scalar(world+44,20);scalar(world+48,30);
    check(query({6,22,36}),"Original attached volume lost its rotated/scaled current WORLD node");
    check(!query({10,22,36}),"Original attached volume admitted its transformed closed face");
    scalar(world+40,100);
    check(!query({6,22,36})&&query({96,22,36}),"Attached trigger reused an old parent WORLD pose");
    scalar(world,0);check(!query({100,20,30}),"Zero-scale source node was admitted");
}
void scene_delivery() {
    LevelContent content;content.bsp_count=1;content.trigger_volumes.resize(1);
    content.trigger_volumes[0].object_name=0;
    unsigned calls=0;SourceSceneObjectOwner owner;
    owner.contains_trigger=[&](std::size_t row,Vec3 point){++calls;check(row==0,"Scene lost authored trigger row identity");return point.x==7;};
    SceneObjects scene(content,0,{}, {},nullptr,owner);
    check(scene.contains(content.trigger_volumes[0],{7,0,0})&&!scene.contains(content.trigger_volumes[0],{8,0,0})&&calls==2,
        "Scene did not preserve source containment answers");
    bool rejected=false;try{auto foreign=content.trigger_volumes[0];scene.contains(foreign,{7,0,0});}catch(const FormatError&){rejected=true;}
    check(rejected&&calls==2,"Foreign trigger identity reached the source world");
    auto saved=scene.authority_checkpoint();saved.source_trigger_query=false;
    rejected=false;try{scene.prepare_authority(saved);}catch(const FormatError&){rejected=true;}
    check(rejected,"Scene continuation changed its retained trigger owner");
}
}
int main()try{original_queries();scene_delivery();std::cout<<"Original TU1.5 trigger queries and live scene identity pass\n";return 0;}
catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
