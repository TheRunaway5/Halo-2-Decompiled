#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "gameplay/objects/native_tu15_object_type_query_source_frame.hpp"
#include <fstream>
#include <iostream>
#include <set>
#include "rendering/effects/source_effect_compiled_symbols_tu15.cpp"
using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
namespace source_reference {
#include "object_type_query_source_reference.inc"
}
namespace {
struct Memory final: mopp_ray_frame_test::Memory {
    WorldState*state{};SourcePhysicsWorldRuntimeServices*services{};bool*trace{};J*callback_log{};SourcePhysicsContactRecordObserver*observer{};
    mutable unsigned io_count{};unsigned selected{},action{};mutable J packets=J::array();
    void before(const char*kind)const{
        packets.push_back(J{{"kind",kind},{"packet",packet(*state,pc)}});
        if(++io_count==selected){if(action==1u)throw Boundary{};if(action==12u)throw native_object_type_query_detail::SourceContinuation{pc,nullptr};}
    }
    void after()const{
        if(io_count!=selected)return;
        switch(action){
            case 2:state->gp[0]^=16u;break;
            case 3:state->gp[3]=0x61003200u;break;
            case 4:state->gp[6]+=4u;break;
            case 5:state->flags.load(state->flags.bits()^0x8d5u);break;
            case 6:*trace=true;break;
            case 7:services->instruction=[this,changed=17u](std::uint32_t at){pc_callback(at);callback_log->push_back(at);(void)changed;};break;
            case 8:services->instruction={};break;
            case 9:state->xmm[0]=_mm_setr_ps(0.25f,-0.5f,2.0f,3.0f);break;
            case 10:*observer=[this](const auto&){before("observer");after();};break;
            case 11:*observer={};break;
            default:break;
        }
    }
    void pc_callback(std::uint32_t at)const{const_cast<Memory*>(this)->pc=at;}
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        before("read");mopp_ray_frame_test::Memory::read(address,out);after();
    }
    void write(std::uint32_t address,std::span<const std::byte>value)override{
        before("write");mopp_ray_frame_test::Memory::write(address,value);after();
    }
};
struct Result{J abi,callbacks;std::string error;bool stopped{};unsigned resume{};};
Result run(Memory&memory,const J&row,bool native,bool observed,unsigned selected=0,unsigned action=0){
    Result out;out.callbacks=J::array();memory.selected=selected;memory.action=action;
    SourcePhysicsWorldRuntimeServices services;bool trace{};
    services.instruction=[&](auto pc){memory.pc=pc;out.callbacks.push_back(pc);};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;
    cursor.position=&memory.pc;cursor.first_trace=&trace;cursor.service_sites=sites;
    cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
    cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    SourcePhysicsContactRecordObserver observer;
    if(observed)observer=[&](const auto&event){memory.before("observer");memory.after();};
    WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp");abi.eflags=row.at("eflags");abi.xmm=row.at("xmm");
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
    abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<abi.x87_depth;++n){auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}
    state.load(abi);memory.state=&state;memory.services=&services;memory.trace=&trace;memory.callback_log=&out.callbacks;memory.observer=&observer;
    try{
        if(native){
            const auto continuation=[&](const auto&current){return current.live_instruction_cursor()==&cursor&&
                cursor.literal_service_sites&&!*cursor.first_trace;};
            out.resume=native_object_type_query_detail::run_source_frame(state,[](auto&v,auto target,auto site,bool tail){source_reference::dispatch(v,target,site,tail);},continuation);
            if(out.resume)source_reference::retained(state,out.resume);
        }else source_reference::retained(state,0u);
    }catch(const Boundary&){out.stopped=true;}
    catch(const native_object_type_query_detail::SourceContinuation&foreign){out.stopped=true;out.error="foreign-continuation-"+std::to_string(foreign.pc);}
    catch(const std::exception&error){out.error=error.what();}
    out.abi=packet(state,memory.pc);memory.state=nullptr;memory.services=nullptr;memory.trace=nullptr;
    return out;
}
Memory seed(const J&row){Memory memory;for(const auto&bank:row.at("banks"))memory.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));return memory;}
void equal(const Memory&a,const Memory&b,const Result&x,const Result&y,const std::string&name){
    check(x.abi==y.abi&&x.stopped==y.stopped&&x.error==y.error&&x.callbacks==y.callbacks&&
        a.banks==b.banks&&a.operations==b.operations&&a.writes==b.writes&&a.packets==b.packets,
        "Type-query source continuation differs "+name+" expected="+x.abi.dump()+" actual="+y.abi.dump()+
        " olderror="+x.error+" newerror="+y.error+" resume="+std::to_string(y.resume));
}
}
int main(int argc,char**argv)try{
    check(argc==2,"Supply type-query source corpus");std::ifstream input(argv[1]);const auto rows=J::parse(input);
    unsigned comparisons{},packets{},resumes{},prefixes{};std::set<std::uint32_t>resume_sites;std::set<std::uint32_t>provider_sites;
    for(const auto&row:rows){
        const auto initial=seed(row);
        for(bool observed:{false,true}){
        auto a=initial,b=initial;const auto x=run(a,row,false,observed),y=run(b,row,true,observed);
        check(!x.stopped&&x.error.empty(),"Normal retained type query did not return: "+x.error);
        equal(a,b,x,y,row.at("name").get<std::string>());++comparisons;packets+=a.packets.size();
        for(const auto&call:a.packets)provider_sites.insert(call.at("packet").at("entry").get<std::uint32_t>());
        if(row.at("id").get<unsigned>()%32u>=8u)continue;
        for(unsigned ordinal=1;ordinal<=a.io_count;++ordinal)for(unsigned action=1;action<=12;++action){
            auto c=initial,d=initial;const auto old=run(c,row,false,observed,ordinal,action),now=run(d,row,true,observed,ordinal,action);
            equal(c,d,old,now,row.at("name").get<std::string>()+"-"+std::to_string(ordinal)+"-"+std::to_string(action));
            ++comparisons;packets+=c.packets.size();resumes+=now.resume!=0u;if(now.resume)resume_sites.insert(now.resume);prefixes+=action==1u;
        }
    }
    }
    check(resumes>0u&&prefixes>0u,"Type-query source feedback controls absent");
    check(resume_sites.size()==24u,"Type-query continuation positive coverage incomplete");
    check(source_reference::coverage.size()==43u,"Type-query retained source coverage incomplete");
    for(const auto pc:{0x10de40u,0x10de46u,0x10de4cu,0x10de4du,0x10de5bu,0x10de5fu,0x10de66u,
        0x10de6fu,0x10de70u,0x10de74u,0x10de80u,0x10de86u,0x10de8du,0x10de91u,0x10de92u,
        0x10de93u,0x10de94u,0x10de95u,0x10de9eu,0x10de9fu,0x10dea0u,0x10deaau,0x10deaeu,
        0x10deb0u,0x10deb1u,0x10deb2u})
        check(provider_sites.contains(pc),"Type-query source provider frontier absent");
    std::cout<<J{{"passed",true},{"source_comparisons",comparisons},{"full_provider_observer_packets",packets},
        {"actual_retained_continuations",resumes},{"positive_continuation_sites",resume_sites},{"provider_observer_failure_prefixes",prefixes},
        {"production_bound",false},{"original_semantic_operations_changed",false},
        {"retained_coverage_sites",source_reference::coverage.size()},{"normal_provider_sites",provider_sites.size()},
        {"positive_normal_returns",rows.size()*2u}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
