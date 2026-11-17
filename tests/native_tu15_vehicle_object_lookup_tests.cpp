#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/vehicles/physics_vehicle_motion_math_internal.hpp"
#include "gameplay/objects/native_tu15_object_body_raw_frame.hpp"
#include <fstream>
#include <iostream>
#include <set>
using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
using halo2::physics_vehicle_motion_math_detail::MathState;
namespace source_reference {
using halo2::physics_vehicle_motion_math_detail::MathState;
#include "vehicle_object_lookup_reference.inc"
}
namespace {
struct Memory final:mopp_ray_frame_test::Memory {
    MathState*state{};SourcePhysicsWorldRuntimeServices*services{};
    SourcePhysicsContactRecordObserver*observer{};
    mutable unsigned io_count{},callback_count{};
    unsigned selected{},action{};bool select_callback{};
    mutable J packets=J::array(),callbacks=J::array();
    void change()const{
        switch(action){
            case 1:throw Boundary{};
            case 2:state->gp[6]^=1u;break;
            case 3:state->gp[2]=0x61000100u;break;
            case 4:state->gp[4]+=4u;break;
            case 5:state->flags.load(state->flags.bits()^0x8d5u);break;
            case 6:*observer=[this](const auto&){before("observer");after();};break;
            case 7:services->instruction=[this,different=17u](std::uint32_t at){instruction(at);(void)different;};break;
            case 8:services->instruction={};break;
            case 9:state->gp[0]^=16u;break;
            case 10:*observer={};break;
            case 11:std::memset(&state->xmm[0],0x31,16);break;
            case 12:{const auto csr=_mm_getcsr();_mm_setcsr((csr^0x6000u)|0x1f80u);break;}
        }
    }
    void before(const char*kind)const{
        packets.push_back(J{{"kind",kind},{"packet",packet(*state,pc)}});
        if(++io_count==selected&&!select_callback&&action==1u)throw Boundary{};
    }
    void after()const{if(io_count==selected&&!select_callback&&action!=1u)change();}
    void instruction(std::uint32_t at)const{
        const_cast<Memory*>(this)->pc=at;
        callbacks.push_back(packet(*state,at));
        if(++callback_count==selected&&select_callback)change();
    }
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        before("read");mopp_ray_frame_test::Memory::read(address,out);after();
    }
    void write(std::uint32_t address,std::span<const std::byte>value)override{
        before("write");mopp_ray_frame_test::Memory::write(address,value);after();
    }
};
struct Result{J abi;std::string error;bool stopped{};};
Result run(Memory&memory,const J&row,bool native,bool observed,bool callback,
           unsigned selected=0,unsigned action=0,bool select_callback=false){
    memory.selected=selected;memory.action=action;memory.select_callback=select_callback;
    SourcePhysicsWorldRuntimeServices services;
    SourcePhysicsContactRecordObserver observer;
    if(observed)observer=[&](const auto&){memory.before("observer");memory.after();};
    if(callback)services.instruction=[&](auto at){memory.instruction(at);};
    MathState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp");abi.eflags=row.at("eflags");abi.xmm=row.at("xmm");
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
    abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<abi.x87_depth;++n){auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}
    state.load(abi);memory.state=&state;memory.services=&services;memory.observer=&observer;
    Result result;
    try{
        if(native)native_object_body_detail::run_raw_frame(state);
        else source_reference::run_bcdf0(state);
    }catch(const Boundary&){result.stopped=true;}
    catch(const std::exception&error){result.error=error.what();}
    result.abi=packet(state,memory.pc);memory.state=nullptr;memory.services=nullptr;memory.observer=nullptr;
    return result;
}
Memory seed(const J&row){Memory memory;for(const auto&bank:row.at("banks"))memory.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));return memory;}
void equal(const Memory&a,const Memory&b,const Result&x,const Result&y,const std::string&name){
    check(x.abi==y.abi&&x.stopped==y.stopped&&x.error==y.error&&a.callbacks==b.callbacks&&
        a.banks==b.banks&&a.operations==b.operations&&a.writes==b.writes&&a.packets==b.packets,
        "Vehicle raw lookup differs "+name+" expected="+x.abi.dump()+" actual="+y.abi.dump()+
        " olderror="+x.error+" newerror="+y.error);
}
}
int main(int argc,char**argv)try{
    check(argc==2,"Supply VehicleEnvironment lookup corpus");std::ifstream input(argv[1]);const auto rows=J::parse(input);
    unsigned comparisons{},normal{},packets{},callbacks{},io_prefixes{},callback_prefixes{};
    std::set<std::uint32_t>sites;
    for(const auto&row:rows){
        const auto initial=seed(row);
        for(bool observed:{false,true})for(bool callback:{false,true}){
            auto a=initial,b=initial;
            const auto old=run(a,row,false,observed,callback),now=run(b,row,true,observed,callback);
            check(!old.stopped&&old.error.empty(),"Vehicle normal original return failed "+old.error);
            equal(a,b,old,now,"normal");++comparisons;++normal;packets+=a.packets.size();callbacks+=a.callbacks.size();
            for(const auto&event:a.callbacks)sites.insert(event.at("entry").get<std::uint32_t>());
            if(row.at("id").get<unsigned>()%32u<8u){
                for(unsigned ordinal=1;ordinal<=a.io_count;++ordinal)for(unsigned action=1;action<=12u;++action){
                    auto c=initial,d=initial;
                    const auto x=run(c,row,false,observed,callback,ordinal,action),y=run(d,row,true,observed,callback,ordinal,action);
                    equal(c,d,x,y,"provider-"+std::to_string(ordinal)+"-"+std::to_string(action));
                    ++comparisons;packets+=c.packets.size();callbacks+=c.callbacks.size();io_prefixes+=action==1u;
                }
            }
            if(callback&&row.at("id").get<unsigned>()%32u<2u){
                for(unsigned ordinal=1;ordinal<=a.callback_count;++ordinal)for(unsigned action=1;action<=12u;++action){
                    auto c=initial,d=initial;
                    const auto x=run(c,row,false,observed,true,ordinal,action,true),y=run(d,row,true,observed,true,ordinal,action,true);
                    equal(c,d,x,y,"callback-"+std::to_string(ordinal)+"-"+std::to_string(action));
                    ++comparisons;packets+=c.packets.size();callbacks+=c.callbacks.size();callback_prefixes+=action==1u;
                }
            }
        }
    }
    check(sites.size()==35u&&io_prefixes&&callback_prefixes,"Vehicle raw lookup coverage absent");
    std::cout<<J{{"passed",true},{"production_bound",false},{"comparisons",comparisons},{"normal_returns",normal},
        {"full_provider_observer_packets",packets},{"full_raw_callback_packets",callbacks},{"callback_sites",sites},
        {"provider_observer_failure_prefixes",io_prefixes},{"raw_callback_failure_prefixes",callback_prefixes}}.dump()<<'\n';
    return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
