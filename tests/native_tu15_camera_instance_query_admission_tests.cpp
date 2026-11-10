#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_camera_instance_query_frame.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
using J=nlohmann::json;
namespace {
void check(bool value,const char*message){if(!value)throw std::runtime_error(message);}
struct Memory:SourcePhysicsContactMemory {
    mutable unsigned reads{};unsigned writes{};
    void read(std::uint32_t,std::span<std::byte>)const override{++reads;throw std::runtime_error("Refused camera query read a bank");}
    void write(std::uint32_t,std::span<const std::byte>)override{++writes;throw std::runtime_error("Refused camera query wrote a bank");}
};
J packet(const WorldState&s){
    const auto a=s.save();return J{{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},
        {"control",a.floating_point->x87_control_word},{"status",a.x87_status},
        {"mxcsr",a.floating_point->mxcsr},{"depth",a.x87_depth},{"mmx",bool(a.active_mmx)}};
}
void refusal(unsigned policy,bool reference){
    Memory memory;unsigned callbacks{},children{};std::uint32_t position{};bool first{},second{};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u},changed{0x38846eu,0x21f590u,0x16c000u};
    SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){++callbacks;position=pc;};
    auto&cursor=services.instruction_cursor;
    cursor={&position,&first,&second,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
    auto foreign=cursor;
    SourcePhysicsContactRecordObserver observe;
    if(policy==12||policy==21)observe=[](const auto&){};
    if(!reference&&policy==17)cursor.literal_service_sites=false;
    if(!reference&&policy==21){services.instruction={};cursor={};}
    WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    SourcePhysicsWorldRuntimeAbi incoming{};incoming.gp[4]=0x61000800u;incoming.eflags=0x202u;
    incoming.floating_point=SourcePhysicsFactoryFloatingPoint{0x27fu,0x1f80u};state.load(incoming);
    if(!reference)switch(policy){
    case 0:state.image=SourcePhysicsContactImage::MapPack11;break;
    case 1:state.fp.count=7;for(auto&lane:state.fp.lanes)lane=1.L;break;
    case 2:state.other_flags|=0x400u;break;
    case 3:services.instruction={};break;
    case 4:services.instruction=[old=services.instruction](auto pc){old(pc);};break;
    case 5:cursor.position=nullptr;break;
    case 6:cursor.executor={};break;
    case 7:cursor.callback_type=nullptr;break;
    case 8:cursor.callback_type=&typeid(void);break;
    case 9:cursor.service_sites=changed;break;
    case 10:cursor.service_sites={};break;
    case 11:cursor.literal_service_sites=false;break;
    case 12:cursor.observation_only_stores=false;break;
    case 13:first=true;break;
    case 14:second=true;break;
    case 15:state.instruction_cursor=&foreign;break;
    case 16:state.instruction_cursor=nullptr;break;
    case 17:cursor.literal_service_sites=true;break;
    case 18:state.active_mmx=SourcePhysicsMmxAlias{};break;
    case 19:state.fp.status|=0x8080u;break;
    case 20:state.fp.top=8;break;
    case 21:break;
    default:throw std::runtime_error("Unknown camera-query refusal policy");
    }
    const auto before=packet(state);
    auto child=[&](auto&,auto,auto){++children;};
    check(!native_camera_instance_query_detail::bridge(state,child),"Unsupported camera query entered native DATA");
    check(packet(state)==before&&!memory.reads&&!memory.writes&&!callbacks&&!children&&!position,
          "Camera-query refusal changed its caller or performed work");
}
}
int main(int argc,char**argv)try{
    const bool reference=argc==2&&std::string(argv[1])=="--reference";
    check(argc==1||reference,"Unexpected camera-query admission argument");
    if(reference){check(native_camera_instance_query_detail::reference(),"Reference control was not set");refusal(0,true);}
    else for(unsigned policy=0;policy<22;++policy)refusal(policy,false);
    std::cout<<J{{"passed",true},{"refusals",reference?1:22},{"side_effects",0},{"reference_control",reference}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
