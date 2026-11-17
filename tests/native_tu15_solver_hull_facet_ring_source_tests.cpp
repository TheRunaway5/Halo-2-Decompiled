#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_solver_hull_facet_ring_source_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_facet_plane_source_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_clear_frame.hpp"
#include <iostream>
#include <set>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
struct RingState:WorldState {
    using WorldState::WorldState;
    std::function<void(std::uint32_t)>before;
    void instruction_literal(std::uint32_t pc){WorldState::instruction_literal(pc);before(pc);}
};
namespace ring_original {
std::function<void(RingState&,std::uint32_t,std::uint32_t)> invoke;
void run_314a30(RingState&s){invoke(s,0x314a30u,0x314d30u);}
void run_314870(RingState&s){invoke(s,0x314870u,0x314d64u);}
#include "solver_hull_facet_ring_source.inc"
}

namespace {
struct Result{
    J abi,packets=J::array(),events=J::array(),children=J::array(),estimates=J::array();
    std::set<std::uint32_t>visited;
    bool stopped{};
    unsigned planes{},clears{};
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
Result run(Memory&memory,const SourcePhysicsWorldRuntimeAbi&a,bool native,unsigned stop_observer=0u){
    Result result;RingState*active{};
    const auto before=[&](std::uint32_t pc){result.packets.push_back(packet(*active,pc));};
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](std::uint32_t pc){
        memory.pc=pc;result.visited.insert(pc);
        if(std::binary_search(ring_original::io_sites.begin(),ring_original::io_sites.end(),pc))before(pc);
    };
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    auto&cursor=services.instruction_cursor;
    cursor.position=&memory.pc;cursor.callback_type=&services.instruction.target_type();
    cursor.executor=std::this_thread::get_id();cursor.service_sites=sites;cursor.literal_service_sites=true;
    cursor.observation_only_stores=true;
    services.reciprocal_sqrt=[&](std::uint32_t operand){
        const auto value=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(operand)));
        const auto result_bits=std::bit_cast<std::uint32_t>(_mm_cvtss_f32(_mm_rsqrt_ss(value)));
        result.estimates.push_back(J{{"input",operand},{"result",result_bits}});
        return result_bits;
    };
    unsigned observed{};
    SourcePhysicsContactRecordObserver observer=[&](const auto&e){
        result.events.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},
                                  {"written",hex(e.written)},{"abi",packet(*active,e.instruction)}});
        if(++observed==stop_observer)throw Boundary{};
    };
    physics_step_detail::FpScope environment(*a.floating_point);
    RingState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(a);
    state.before=before;active=&state;
    const auto call=[&](RingState&s,std::uint32_t target,std::uint32_t site){
        check((target==0x314a30u&&site==0x314d30u)||(target==0x314870u&&site==0x314d64u),"Wrong genuine ring child");
        const auto bank=memory.at(s.gp[4],4u);std::uint32_t returned{};
        std::memcpy(&returned,bank->second.data()+s.gp[4]-bank->first,4u);
        check(returned==site+5u,"Wrong actual ring call return word");
        result.children.push_back(J{{"target",target},{"site",site},{"entry",packet(s,target)},{"return_word",returned}});
        if(target==0x314a30u){
            ++result.planes;
            if(native)native_solver_hull_facet_plane_detail::run_private_source(s,ring_original::source_estimate);
            else ring_original::plane(s);
        }else{
            ++result.clears;
            if(native)native_solver_hull_clear_detail::run_private_source(s);
            else ring_original::clear(s);
        }
        result.children.back()["returned"]=packet(s,returned);
    };
    ring_original::invoke=call;
    try{
        if(native)native_solver_hull_facet_ring_detail::run_private_source(state,call);
        else ring_original::ring(state);
    }catch(const Boundary&){result.stopped=true;}
    ring_original::invoke={};result.abi=packet(state,memory.pc);return result;
}
void compare(const Memory&a,const Memory&b,const Result&x,const Result&y){
    if(a.banks==b.banks&&a.operations==b.operations&&a.writes==b.writes&&a.read_count==b.read_count&&
       a.write_count==b.write_count&&x.abi==y.abi&&x.packets==y.packets&&x.events==y.events&&
       x.estimates==y.estimates&&x.children==y.children&&x.stopped==y.stopped)return;
    std::cerr<<"ABI="<<(x.abi==y.abi)<<" I/O="<<(a.operations==b.operations)<<" packets="<<(x.packets==y.packets)
             <<" events="<<(x.events==y.events)<<" children="<<(x.children==y.children)<<'\n';
    throw std::runtime_error("Complete retained facet ring differs");
}
}
int main()try{
    J rows;std::cin>>rows;std::set<std::uint32_t>parents;
    unsigned cases{},packets{},stops{},planes{},clears{},estimates{},multi_chain_prefixes{};
    for(const auto&row:rows){
        Memory seed;
        for(const auto&bank:row.at("banks"))seed.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
        const auto abi=input_abi(row);
        auto a=seed,b=seed;
        const auto x=run(a,abi,false),y=run(b,abi,true);
        compare(a,b,x,y);check(!x.stopped&&x.clears==1u&&x.estimates.size()==x.planes,"Wrong genuine ring child activity");
        ++cases;packets+=x.packets.size();planes+=x.planes;clears+=x.clears;estimates+=x.estimates.size();
        for(auto site:x.visited)if(site>=0x314cb0u&&site<=0x314d7bu)parents.insert(site);
        if((x.planes==0u&&row.at("id")!=1)||x.planes==1u||multi_chain_prefixes>=2u)continue;
        if(x.planes>=2u)++multi_chain_prefixes;
        const auto stop=[&](int rd,int wr,unsigned ob){
            auto l=seed,r=seed;l.read_fault=r.read_fault=rd;l.write_fault=r.write_fault=wr;
            const auto u=run(l,abi,false,ob),v=run(r,abi,true,ob);compare(l,r,u,v);
            check(u.stopped&&v.stopped,"Missing retained ring/child prefix");++stops;
        };
        for(unsigned i=0;i<a.read_count;++i)stop(int(i),-1,0);
        for(unsigned i=0;i<a.write_count;++i)stop(-1,int(i),0);
        for(unsigned i=1;i<=x.events.size();++i)stop(-1,-1,i);
    }
    check(parents.size()==72u&&multi_chain_prefixes==2u,"Incomplete retained ring source coverage");
    std::cout<<J{{"source_cases",cases},{"parent_sites",parents.size()},{"pre_io_packets",packets},{"stopped_prefixes",stops},
                 {"genuine_plane_entries",planes},{"genuine_clear_entries",clears},{"actual_estimate_requests",estimates}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
