#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "halo2/physics_vehicle_environment_state.hpp"
#include "vehicle_object_lookup_public_proof.hpp"
#include <fstream>
#include <iostream>
#include <set>
using namespace halo2;using namespace halo2::mopp_ray_frame_test;
using halo2::physics_vehicle_motion_math_detail::MathState;
namespace {
unsigned cleanup_calls{};
J host(){std::uint16_t cw{},sw{};asm volatile("fnstcw %0;fnstsw %1":"=m"(cw),"=m"(sw));return J::array({cw,sw,_mm_getcsr()});}
J abi_packet(const SourcePhysicsContactRuntimeAbi&a){
    J extended=J::array();for(const auto&v:a.x87)extended.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&v),10)));
    J fp=nullptr;if(a.floating_point)fp=J::array({a.floating_point->x87_control_word,a.floating_point->mxcsr});
    J mmx=nullptr;if(a.active_mmx){J physical=J::array();for(const auto&v:a.active_mmx->physical)physical.push_back(hex(v));mmx=J{{"known",a.active_mmx->known_mask},{"physical",physical}};}
    return J{{"gp",a.gp},{"xmm",a.xmm},{"eflags",a.eflags},{"fs_base",a.fs_base},{"x87",extended},{"depth",a.x87_depth},{"status",a.x87_status},{"fp",fp},{"mmx",mmx}};
}
struct Memory: mopp_ray_frame_test::Memory {
    MathState*state{};SourcePhysicsContactRuntimeServices*bound{},*original{};
    SourcePhysicsContactRecordObserver*observer{};SourcePhysicsContactRuntimeAbi*published{};
    SourcePhysicsFactoryFloatingPoint*fp{};
    mutable unsigned io_count{},callback_count{},changes{};unsigned selected{},action{};bool select_callback{};
    mutable J packets=J::array(),callbacks=J::array(),events=J::array();
    J current()const{return J{{"live",abi_packet(state->save())},{"published",abi_packet(*published)},{"fp_argument",J::array({fp->x87_control_word,fp->mxcsr})},{"host",host()}};}
    void change()const{
        ++changes;
        switch(action){
        case 1:throw Boundary{};
        case 2:state->gp[6]^=1u;break;
        case 3:state->gp[2]=0x61000100u;break;
        case 4:state->gp[4]+=4u;break;
        case 5:state->flags.load(state->flags.bits()^0x8d5u);break;
        case 6:*observer=[this](const auto&e){observed(e);};break;
        case 7:bound->instruction=[this,different=17u](std::uint32_t at){instruction(at);(void)different;};break;
        case 8:bound->instruction={};break;
        case 9:state->gp[0]^=16u;break;
        case 10:*observer={};break;
        case 11:std::memset(&state->xmm[0],0x31,16);break;
        case 12:_mm_setcsr((_mm_getcsr()^0x6000u)|0x1f80u);break;
        case 13:original->instruction=[this,other=23u](std::uint32_t at){instruction(at);state->gp[0]^=other;};break;
        case 14:published->gp[7]^=0x100u;break;
        case 15:fp->x87_control_word^=0x400u;fp->mxcsr^=0x6000u;break;
        }
    }
    void before(const char*kind)const{
        check(state,"Public provider preceded real MathState load");
        packets.push_back(J{{"kind",kind},{"pc",pc},{"state",current()}});
        if(++io_count==selected&&!select_callback&&action==1u)change();
    }
    void after()const{if(io_count==selected&&!select_callback&&action!=1u)change();}
    void instruction(std::uint32_t at)const{
        const_cast<Memory*>(this)->pc=at;callbacks.push_back(J{{"pc",at},{"state",current()}});
        if(++callback_count==selected&&select_callback)change();
    }
    void observed(const SourcePhysicsContactRecordStore&e)const{
        events.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)},{"state",current()}});
        before("observer");after();
    }
    void read(std::uint32_t address,std::span<std::byte>out)const override{before("read");mopp_ray_frame_test::Memory::read(address,out);after();}
    void write(std::uint32_t address,std::span<const std::byte>value)override{before("write");mopp_ray_frame_test::Memory::write(address,value);after();}
};
struct Result{J abi,fp,host_after;std::string error;bool stopped{},loaded{};unsigned native{},literal{};};
SourcePhysicsContactRuntimeAbi input(const J&row,bool present){
    SourcePhysicsContactRuntimeAbi a{};a.gp=row.at("gp");a.eflags=row.at("eflags");a.xmm=row.at("xmm");a.fs_base=0x12340000u+row.at("id").get<unsigned>();
    if(present)a.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
    a.x87_status=row.at("before_fp")[1];a.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<a.x87_depth;++n){const auto v=bytes(row.at("live_x87")[n]);std::memcpy(&a.x87[n],v.data(),10);}
    switch(row.value("load_guard",0u)){
    case 1:a.x87_depth=9u;break;
    case 2:{SourcePhysicsMmxAlias alias{};alias.known_mask=0xffu;for(unsigned n=0;n<8;++n)for(unsigned k=0;k<10;++k)alias.physical[n][k]=std::byte(n*10u+k);a.active_mmx=alias;a.x87_depth=1u;break;}
    case 3:a.floating_point=SourcePhysicsFactoryFloatingPoint{0x37eu,0x1f80u};break;
    case 4:a.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f00u};break;
    }
    if(row.contains("opaque_mmx")){
        SourcePhysicsMmxAlias alias{};alias.known_mask=row.at("opaque_mmx");for(unsigned n=0;n<8;++n)for(unsigned k=0;k<10;++k)alias.physical[n][k]=std::byte(31u+n*10u+k);
        a.active_mmx=alias;a.x87_depth=0u;a.x87_status&=~0x3800u;
    }
    return a;
}
Result run(Memory&m,const J&row,bool native,bool observed,bool callback,bool fp_present,unsigned selected=0,unsigned action=0,bool callback_phase=false){
    m.selected=selected;m.action=action;m.select_callback=callback_phase;
    SourcePhysicsContactRuntimeServices services;
    services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Unexpected public lookup child owner");};
    SourcePhysicsContactRecordObserver observer;
    if(observed)observer=[&](const auto&e){m.observed(e);};
    if(callback)services.instruction=[&](auto at){m.instruction(at);};
    auto a=input(row,fp_present);SourcePhysicsFactoryFloatingPoint fp{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
    m.original=&services;m.observer=&observer;m.published=&a;m.fp=&fp;
    Result result;const auto before=native_vehicle_object_lookup_owner_detail::activity();const auto literals=vehicle_lookup_public_proof::literals;
    vehicle_lookup_public_proof::entered=[&](MathState&s,const auto&bound,const auto&actual_observer){m.state=&s;m.bound=const_cast<SourcePhysicsContactRuntimeServices*>(&bound);check(&actual_observer==&observer,"Public observer was not the real caller object");result.loaded=true;};
    physics_step_detail::FpScope outer({0x37fu,0x1f80u});const auto incoming=host();
    try{
        if(native)source_run_physics_vehicle_environment_state_runtime(m,0xbcdf0u,a,fp,services,observer);
        else vehicle_lookup_public_proof::source_reference(m,0xbcdf0u,a,fp,services,observer);
    }catch(const Boundary&){result.stopped=true;}catch(const std::exception&e){result.error=e.what();}
    result.abi=abi_packet(a);result.fp=J::array({fp.x87_control_word,fp.mxcsr});result.host_after=host();
    check(result.host_after==incoming,"Public lookup FpScope failed to restore caller environment");
    result.native=native_vehicle_object_lookup_owner_detail::activity()-before;result.literal=vehicle_lookup_public_proof::literals-literals;
    vehicle_lookup_public_proof::entered={};m.state=nullptr;m.bound=nullptr;m.original=nullptr;m.observer=nullptr;m.published=nullptr;m.fp=nullptr;return result;
}
Memory seed(const J&row){Memory m;for(const auto&bank:row.at("banks"))m.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));return m;}
void equal(const Memory&a,const Memory&b,const Result&x,const Result&y,const std::string&name){
    check(x.abi==y.abi&&x.fp==y.fp&&x.host_after==y.host_after&&x.stopped==y.stopped&&x.error==y.error&&x.loaded==y.loaded&&a.banks==b.banks&&a.operations==b.operations&&a.writes==b.writes&&a.packets==b.packets&&a.callbacks==b.callbacks&&a.events==b.events&&a.changes==b.changes,
        "Vehicle public lookup differs "+name+" abi="+std::to_string(x.abi==y.abi)+" fp="+std::to_string(x.fp==y.fp)+" packets="+std::to_string(a.packets==b.packets)+" old="+x.error+" new="+y.error);
}
}
namespace halo2 {
void source_run_physics_vehicle_constraint_cleanup_runtime(SourcePhysicsContactMemory&,std::uint32_t,SourcePhysicsContactRuntimeAbi&,SourcePhysicsFactoryFloatingPoint&,const SourcePhysicsContactRuntimeServices&,const SourcePhysicsContactRecordObserver&){++cleanup_calls;throw std::runtime_error("Unreachable public lookup cleanup service");}
}
int main(int argc,char**argv)try{
    check(argc>=2,"Supply public Vehicle lookup corpus");std::ifstream file(argv[1]);const auto rows=J::parse(file);
    bool forced{},controls_only{};for(int n=2;n<argc;++n){const std::string option=argv[n];if(option=="--reference")forced=true;else if(option=="--controls-only")controls_only=true;else throw std::runtime_error("Unknown Vehicle public proof option");}
    unsigned comparisons{},normal{},packets{},callbacks{},prefixes{},native_entries{},fallbacks{},load_refusals{},opaque_mmx_cases{};std::set<std::uint32_t>sites;
    for(const auto&row:rows){if(controls_only&&row.at("id").get<unsigned>()!=0u)continue;const auto initial=seed(row);
        for(bool present:{false,true})for(bool observed:{false,true})for(bool callback:{false,true}){
            auto a=initial,b=initial;const auto expected=run(a,row,false,observed,callback,present),actual=run(b,row,true,observed,callback,present);equal(a,b,expected,actual,"normal");
            check(!expected.stopped&&expected.error.empty()&&actual.loaded,"Public normal lookup did not return");
            check(actual.native==unsigned(!forced)&&actual.literal==unsigned(forced),"Public lookup did not use its exact candidate/ref route");
            ++comparisons;++normal;native_entries+=actual.native;fallbacks+=actual.literal;packets+=a.packets.size();callbacks+=a.callbacks.size();for(const auto&e:a.callbacks)sites.insert(e.at("pc").get<std::uint32_t>());
            if(row.at("id").get<unsigned>()%32u<4u)for(unsigned at=1;at<=a.io_count;++at)for(unsigned action=1;action<=15u;++action){
                auto c=initial,d=initial;const auto x=run(c,row,false,observed,callback,present,at,action),y=run(d,row,true,observed,callback,present,at,action);equal(c,d,x,y,"provider");check(y.native==unsigned(!forced),"Provider case missed native entry");++comparisons;packets+=c.packets.size();callbacks+=c.callbacks.size();prefixes+=action==1u;
            }
            if(callback&&row.at("id").get<unsigned>()%32u==0u)for(unsigned at=1;at<=a.callback_count;++at)for(unsigned action=1;action<=15u;++action){
                auto c=initial,d=initial;const auto x=run(c,row,false,observed,true,present,at,action,true),y=run(d,row,true,observed,true,present,at,action,true);equal(c,d,x,y,"callback");check(y.native==unsigned(!forced),"Callback case missed native entry");++comparisons;packets+=c.packets.size();callbacks+=c.callbacks.size();prefixes+=action==1u;
            }
        }
    }
    for(unsigned guard=1;guard<=4u;++guard)for(bool observed:{false,true})for(bool callback:{false,true}){
        auto row=rows.front();row["load_guard"]=guard;const auto initial=seed(row);auto a=initial,b=initial;
        const auto expected=run(a,row,false,observed,callback,true),actual=run(b,row,true,observed,callback,true);equal(a,b,expected,actual,"load refusal");
        check(!actual.loaded&&!actual.error.empty()&&!actual.native&&!actual.literal&&a.operations.empty()&&a.callbacks.empty()&&a.events.empty()&&actual.abi==abi_packet(input(row,true)),"Actual public load refusal published or executed owner state "+std::to_string(guard));
        ++load_refusals;++comparisons;
    }
    for(unsigned mask:{0u,0x55u,0xffu})for(bool present:{false,true})for(bool observed:{false,true})for(bool callback:{false,true}){
        auto row=rows.front();row["opaque_mmx"]=mask;const auto initial=seed(row);auto a=initial,b=initial;
        const auto expected=run(a,row,false,observed,callback,present),actual=run(b,row,true,observed,callback,present);equal(a,b,expected,actual,"opaque MMX");
        check(actual.loaded&&actual.error.empty()&&!actual.stopped&&actual.native==unsigned(!forced)&&actual.literal==unsigned(forced)&&actual.abi.at("mmx")==abi_packet(input(row,present)).at("mmx"),"Actual public opaque MMX owner was not preserved");
        ++opaque_mmx_cases;++comparisons;
    }
    check((controls_only||sites.size()==35u)&&prefixes&&cleanup_calls==0u,"Public lookup coverage or childless ownership failed");
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"normal",normal},{"full_provider_packets",packets},{"full_callback_packets",callbacks},{"source_callback_sites",sites.size()},{"failure_prefixes",prefixes},{"normal_native_entries",native_entries},{"normal_literal_fallbacks",fallbacks},{"unreachable_cleanup_calls",cleanup_calls},{"actual_public_API",true},{"actual_preload_refusals",load_refusals},{"opaque_MMX_complete_returns",opaque_mmx_cases},{"controls_only",controls_only}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
