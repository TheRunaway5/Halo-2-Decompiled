#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_solver_hull_facet_search_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_clip_source_frame.hpp"
#include <iostream>
#include <set>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
struct FacetState:WorldState {
    using WorldState::WorldState;
    std::function<void(std::uint32_t)>before;
    void instruction_literal(std::uint32_t pc){WorldState::instruction_literal(pc);before(pc);}
};
namespace facet_original {
std::function<void(FacetState&,std::uint32_t,std::uint32_t)> invoke;
void run_3147d0(FacetState&s){invoke(s,0x3147d0u,*s.instruction_cursor->position);}
void run_3148e0(FacetState&s){invoke(s,0x3148e0u,*s.instruction_cursor->position);}
#include "solver_hull_facet_walk_source.inc"
}

namespace {
struct Result{
    J abi,packets=J::array(),events=J::array(),children=J::array();
    std::set<std::uint32_t>visited;
    bool stopped{};
    unsigned visits{},clips{};
};
SourcePhysicsWorldRuntimeAbi input_abi(const J&row){
    SourcePhysicsWorldRuntimeAbi a{};
    a.gp=row.at("gp").get<std::array<std::uint32_t,8>>();a.eflags=row.at("eflags");
    a.xmm=row.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();
    a.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
    a.x87_status=row.at("before_fp")[1];a.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<a.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&a.x87[n],value.data(),10);}
    return a;
}
Result run(Memory&memory,const SourcePhysicsWorldRuntimeAbi&a,bool search,bool native,unsigned stop_observer=0u){
    Result result;FacetState*active{};
    const auto before=[&](std::uint32_t pc){result.packets.push_back(packet(*active,pc));};
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](std::uint32_t pc){
        memory.pc=pc;result.visited.insert(pc);
        if(std::binary_search(facet_original::io_sites.begin(),facet_original::io_sites.end(),pc))before(pc);
    };
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    auto&cursor=services.instruction_cursor;
    cursor.position=&memory.pc;cursor.callback_type=&services.instruction.target_type();
    cursor.executor=std::this_thread::get_id();cursor.service_sites=sites;cursor.literal_service_sites=true;
    cursor.observation_only_stores=true;
    unsigned observed{};
    SourcePhysicsContactRecordObserver observer=[&](const auto&e){
        result.events.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},
                                  {"written",hex(e.written)},{"abi",packet(*active,e.instruction)}});
        if(++observed==stop_observer)throw Boundary{};
    };
    physics_step_detail::FpScope environment(*a.floating_point);
    FacetState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(a);
    state.before=before;active=&state;
    const auto checkpoint=[](FacetState&,std::uint32_t){};
    std::function<void(FacetState&,std::uint32_t,std::uint32_t)>call;
    call=[&](FacetState&s,std::uint32_t target,std::uint32_t site){
        check((target==0x3147d0u&&(site==0x31491cu||site==0x314c77u))||
              (target==0x3148e0u&&(site==0x31493au||site==0x314c8du)),"Wrong genuine facet walk child");
        const auto bank=memory.at(s.gp[4],4u);std::uint32_t returned{};
        std::memcpy(&returned,bank->second.data()+s.gp[4]-bank->first,4u);
        check(returned==site+5u,"Wrong actual facet call return word");
        const auto index=result.children.size();
        result.children.push_back(J{{"target",target},{"site",site},{"entry",packet(s,target)},{"return_word",returned}});
        if(target==0x3147d0u){
            ++result.clips;
            if(native)native_solver_hull_clip_source_detail::run_private_source<false>(s);
            else facet_original::clip(s);
        }else{
            ++result.visits;
            check(result.visits<=20u,"Unexpected repeated retained facet visitation");
            if(native)native_solver_hull_facet_visit_detail::run_private_source(s,call,checkpoint);
            else facet_original::visit(s);
        }
        result.children.at(index)["returned"]=packet(s,returned);
    };
    facet_original::invoke=call;
    try{
        if(search){
            if(native)native_solver_hull_facet_search_detail::run_private_source(state,call,checkpoint);
            else facet_original::search(state);
        }else{
            if(native)native_solver_hull_facet_visit_detail::run_private_source(state,call,checkpoint);
            else facet_original::visit(state);
        }
    }catch(const Boundary&){result.stopped=true;}
    facet_original::invoke={};result.abi=packet(state,memory.pc);return result;
}
void compare(const Memory&a,const Memory&b,const Result&x,const Result&y){
    if(a.banks==b.banks&&a.operations==b.operations&&a.writes==b.writes&&a.read_count==b.read_count&&
       a.write_count==b.write_count&&x.abi==y.abi&&x.packets==y.packets&&x.events==y.events&&
       x.children==y.children&&x.stopped==y.stopped)return;
    std::cerr<<"ABI="<<(x.abi==y.abi)<<" I/O="<<(a.operations==b.operations)<<" packets="<<(x.packets==y.packets)
             <<" events="<<(x.events==y.events)<<" children="<<(x.children==y.children)<<'\n';
    throw std::runtime_error("Complete retained facet walk differs");
}
}
int main()try{
    J rows;std::cin>>rows;
    std::set<std::uint32_t>visit_sites,search_sites;
    unsigned cases{},packets{},stops{},visits{},clips{},prefix_seeds{};
    for(const auto&row:rows){
        Memory seed;
        for(const auto&bank:row.at("banks"))seed.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
        const auto abi=input_abi(row);const bool search=row.at("entry")==0x314c40u;
        const unsigned id=row.at("id");
        auto a=seed,b=seed;
        const auto x=run(a,abi,search,false),y=run(b,abi,search,true);
        compare(a,b,x,y);check(!x.stopped&&(!search||x.clips>=1u),"Wrong genuine facet child activity");
        ++cases;packets+=x.packets.size();visits+=x.visits;clips+=x.clips;
        for(auto site:x.visited){
            if(site>=0x3148e0u&&site<=0x3149a4u)visit_sites.insert(site);
            if(site>=0x314c40u&&site<=0x314ca2u)search_sites.insert(site);
        }
        if(search?(id!=0u&&id!=14u&&id!=38u):(id!=0u&&id!=13u&&id!=37u))continue;
        ++prefix_seeds;
        const auto stop=[&](int rd,int wr,unsigned ob){
            auto l=seed,r=seed;l.read_fault=r.read_fault=rd;l.write_fault=r.write_fault=wr;
            const auto u=run(l,abi,search,false,ob),v=run(r,abi,search,true,ob);compare(l,r,u,v);
            check(u.stopped&&v.stopped,"Missing retained facet walk prefix");++stops;
        };
        for(unsigned i=0;i<a.read_count;++i)stop(int(i),-1,0);
        for(unsigned i=0;i<a.write_count;++i)stop(-1,int(i),0);
        for(unsigned i=1;i<=x.events.size();++i)stop(-1,-1,i);
    }
    check(visit_sites.size()==74u&&search_sites.size()==40u&&prefix_seeds==6u,"Incomplete retained facet source coverage");
    std::cout<<J{{"source_cases",cases},{"visit_sites",visit_sites.size()},{"search_sites",search_sites.size()},
                 {"pre_io_packets",packets},{"stopped_prefixes",stops},{"genuine_visit_entries",visits},
                 {"genuine_clip_entries",clips}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
