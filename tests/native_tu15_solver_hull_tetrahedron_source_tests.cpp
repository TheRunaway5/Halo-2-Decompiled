#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_solver_hull_tetrahedron_source_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_facet_plane_source_frame.hpp"
#include <iostream>
#include <set>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
struct TetraState:WorldState {
    using WorldState::WorldState;
    std::function<void(std::uint32_t)>before;
    void instruction_literal(std::uint32_t pc){WorldState::instruction_literal(pc);before(pc);}
};
namespace tetra_original {
std::function<void(TetraState&,std::uint32_t,std::uint32_t)> invoke;
void run_314a30(TetraState&s){invoke(s,0x314a30u,*s.instruction_cursor->position);}
void run_314d80(TetraState&s){invoke(s,0x314d80u,*s.instruction_cursor->position);}
#include "solver_hull_tetrahedron_source.inc"
}

namespace {
struct Result{
    J abi,packets=J::array(),events=J::array(),children=J::array(),estimates=J::array();
    std::set<std::uint32_t>visited;
    bool stopped{};
    unsigned seeds{},planes{};
};
SourcePhysicsWorldRuntimeAbi input_abi(const J&row){
    SourcePhysicsWorldRuntimeAbi a{};
    a.gp=row.at("gp").get<std::array<std::uint32_t,8>>();a.eflags=row.at("eflags");a.fs_base=row.value("fs_base",0u);
    a.xmm=row.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();
    a.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
    a.x87_status=row.at("before_fp")[1];a.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<a.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&a.x87[n],value.data(),10);}
    return a;
}
Result run(Memory&memory,const SourcePhysicsWorldRuntimeAbi&a,bool tetra,bool native,unsigned stop_observer=0u){
    Result result;TetraState*active{};
    const auto before=[&](std::uint32_t pc){result.packets.push_back(packet(*active,pc));};
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](std::uint32_t pc){
        memory.pc=pc;result.visited.insert(pc);
        if(std::binary_search(tetra_original::io_sites.begin(),tetra_original::io_sites.end(),pc))before(pc);
    };
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    auto&cursor=services.instruction_cursor;
    cursor.position=&memory.pc;cursor.callback_type=&services.instruction.target_type();
    cursor.executor=std::this_thread::get_id();cursor.service_sites=sites;cursor.literal_service_sites=true;
    cursor.observation_only_stores=true;
    services.reciprocal_sqrt=[&](std::uint32_t operand){
        auto result_lane=_mm_setzero_ps();const auto bits=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(operand)));
        asm volatile("rsqrtss %1,%0":"+x"(result_lane):"x"(bits):"memory");
        const auto result_bits=std::bit_cast<std::uint32_t>(_mm_cvtss_f32(result_lane));
        result.estimates.push_back(J{{"input",operand},{"result",result_bits}});return result_bits;
    };
    unsigned observed{};
    SourcePhysicsContactRecordObserver observer=[&](const auto&e){
        result.events.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},
                                  {"written",hex(e.written)},{"abi",packet(*active,e.instruction)}});
        if(++observed==stop_observer)throw Boundary{};
    };
    physics_step_detail::FpScope environment(*a.floating_point);
    TetraState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(a);
    state.before=before;active=&state;
    const auto checkpoint=[](TetraState&,std::uint32_t){};
    std::function<void(TetraState&,std::uint32_t,std::uint32_t)>call;
    call=[&](TetraState&s,std::uint32_t target,std::uint32_t site){
        check((target==0x314a30u&&(site==0x314e27u||site==0x314eb2u))||
              (target==0x314d80u&&site==0x313b90u),"Wrong genuine tetrahedron child");
        const auto bank=memory.at(s.gp[4],4u);std::uint32_t returned{};
        std::memcpy(&returned,bank->second.data()+s.gp[4]-bank->first,4u);
        check(returned==site+5u,"Wrong actual tetrahedron call return word");
        const auto index=result.children.size();
        result.children.push_back(J{{"target",target},{"site",site},{"entry",packet(s,target)},{"return_word",returned}});
        if(target==0x314a30u){
            ++result.planes;
            if(native)native_solver_hull_facet_plane_detail::run_private_source(s,tetra_original::source_estimate);
            else tetra_original::plane(s);
        }else{
            ++result.seeds;
            if(native)native_solver_hull_tetrahedron_source_detail::run_seed(s,call,checkpoint);
            else tetra_original::seed(s);
        }
        result.children.at(index)["returned"]=packet(s,returned);
    };
    tetra_original::invoke=call;
    try{
        if(tetra){
            if(native)native_solver_hull_tetrahedron_source_detail::run_tetrahedron(state,call,checkpoint);
            else tetra_original::tetrahedron(state);
        }else{
            if(native)native_solver_hull_tetrahedron_source_detail::run_seed(state,call,checkpoint);
            else tetra_original::seed(state);
        }
    }catch(const Boundary&){result.stopped=true;}
    tetra_original::invoke={};result.abi=packet(state,memory.pc);return result;
}
void compare(const Memory&a,const Memory&b,const Result&x,const Result&y){
    if(a.banks==b.banks&&a.operations==b.operations&&a.writes==b.writes&&a.read_count==b.read_count&&
       a.write_count==b.write_count&&x.abi==y.abi&&x.packets==y.packets&&x.events==y.events&&
       x.estimates==y.estimates&&x.children==y.children&&x.stopped==y.stopped)return;
    std::cerr<<"ABI="<<(x.abi==y.abi)<<" I/O="<<(a.operations==b.operations)<<" packets="<<(x.packets==y.packets)
             <<" events="<<(x.events==y.events)<<" children="<<(x.children==y.children)<<'\n';
    for(std::size_t n=0;n<std::min(x.packets.size(),y.packets.size());++n)
        if(x.packets[n]!=y.packets[n]){
            std::cerr<<"first_packet "<<x.packets[n].dump()<<" versus "<<y.packets[n].dump()<<'\n';break;
        }
    throw std::runtime_error("Complete retained tetrahedron differs");
}
}
int main()try{
    J rows;std::cin>>rows;
    std::set<std::uint32_t>seed_sites,tetra_sites;
    unsigned cases{},packets{},stops{},seeds{},planes{},prefix_seeds{};
    for(const auto&row:rows){
        Memory seed_memory;
        for(const auto&bank:row.at("banks"))seed_memory.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
        const auto abi=input_abi(row);const bool tetra=row.at("entry")==0x313ab0u;
        const unsigned id=row.at("id");
        auto a=seed_memory,b=seed_memory;
        try{
            const auto x=run(a,abi,tetra,false),y=run(b,abi,tetra,true);
            compare(a,b,x,y);check(!x.stopped&&x.planes==5u&&x.seeds==unsigned(tetra)&&x.estimates.size()==x.planes,"Wrong genuine tetrahedron child activity");
            ++cases;packets+=x.packets.size();seeds+=x.seeds;planes+=x.planes;
            for(auto site:x.visited){
                if(site>=0x314d80u&&site<=0x314ec3u)seed_sites.insert(site);
                if(site>=0x313ab0u&&site<=0x313ba6u)tetra_sites.insert(site);
            }
            if(id!=0u&&id!=1u&&id!=8u)continue;
            ++prefix_seeds;
            const auto stop=[&](int rd,int wr,unsigned ob){
                auto l=seed_memory,r=seed_memory;l.read_fault=r.read_fault=rd;l.write_fault=r.write_fault=wr;
                const auto u=run(l,abi,tetra,false,ob),v=run(r,abi,tetra,true,ob);compare(l,r,u,v);
                check(u.stopped&&v.stopped,"Missing retained tetrahedron prefix");++stops;
            };
            for(unsigned n=0;n<a.read_count;++n)stop(int(n),-1,0);
            for(unsigned n=0;n<a.write_count;++n)stop(-1,int(n),0);
            for(unsigned n=1;n<=x.events.size();++n)stop(-1,-1,n);
        }catch(...){std::cerr<<"case="<<row.at("name")<<'\n';throw;}
    }
    check(seed_sites.size()==97u&&tetra_sites.size()==58u&&prefix_seeds==6u,"Incomplete retained tetrahedron source coverage");
    std::cout<<J{{"source_cases",cases},{"seed_sites",seed_sites.size()},{"tetrahedron_sites",tetra_sites.size()},
                 {"pre_io_packets",packets},{"stopped_prefixes",stops},{"genuine_seed_entries",seeds},
                 {"genuine_plane_entries",planes}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
