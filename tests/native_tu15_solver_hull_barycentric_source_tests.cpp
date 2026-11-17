#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_solver_hull_barycentric_source_frame.hpp"
#include <iostream>
#include <set>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
struct BaryState:WorldState {
    using WorldState::WorldState;
    std::function<void(std::uint32_t)>before;
    void instruction_literal(std::uint32_t pc){WorldState::instruction_literal(pc);before(pc);}
};
#include "solver_hull_barycentric_source.inc"

namespace {
struct Result {
    J abi,packets=J::array(),events=J::array();
    std::set<std::uint32_t> sites;
    bool stopped{};
};
SourcePhysicsWorldRuntimeAbi load_abi(const J&row){
    SourcePhysicsWorldRuntimeAbi a{};
    a.gp=row.at("gp").get<std::array<std::uint32_t,8>>();a.eflags=row.at("eflags");
    a.xmm=row.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();
    a.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
    a.x87_status=row.at("before_fp")[1];a.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<a.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&a.x87[n],value.data(),10);}
    return a;
}
Result run(Memory&memory,const SourcePhysicsWorldRuntimeAbi&a,bool native,unsigned observer_fault=0u){
    Result result;BaryState*active{};
    SourcePhysicsWorldRuntimeServices services;
    const auto before=[&](std::uint32_t pc){result.packets.push_back(packet(*active,pc));};
    services.instruction=[&](std::uint32_t pc){
        memory.pc=pc;result.sites.insert(pc);
        if(std::binary_search(barycentric_source::io_sites.begin(),barycentric_source::io_sites.end(),pc))before(pc);
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
        if(++observed==observer_fault)throw Boundary{};
    };
    physics_step_detail::FpScope environment(*a.floating_point);
    BaryState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(a);
    state.before=before;active=&state;
    try{
        if(native){
            const auto spill=[](auto&s){using namespace physics_step_detail;return u(halo2::physics_step_detail::spill(s.fp.st(0)));};
            native_solver_hull_barycentric_detail::run_private_source<false>(state,[](auto&,std::uint32_t){},spill);
        }else barycentric_source::current(state);
    }catch(const Boundary&){result.stopped=true;}
    result.abi=packet(state,memory.pc);return result;
}
void compare(const Memory&a,const Memory&b,const Result&x,const Result&y,const std::string&name){
    if(a.banks==b.banks&&a.operations==b.operations&&a.writes==b.writes&&a.read_count==b.read_count&&
       a.write_count==b.write_count&&x.abi==y.abi&&x.packets==y.packets&&x.events==y.events&&
       x.stopped==y.stopped)return;
    std::cerr<<name<<" ABI="<<(x.abi==y.abi)<<" I/O="<<(a.operations==b.operations)
             <<" packets="<<(x.packets==y.packets)<<" events="<<(x.events==y.events)<<'\n';
    if(x.packets!=y.packets){
        unsigned i{};while(i<x.packets.size()&&i<y.packets.size()&&x.packets[i]==y.packets[i])++i;
        if(i<x.packets.size()&&i<y.packets.size())std::cerr<<x.packets[i].dump()<<'\n'<<y.packets[i].dump()<<'\n';
    }
    throw std::runtime_error("Complete retained hull barycentric projection differs");
}
}
int main()try{
    J rows;std::cin>>rows;std::set<std::uint32_t>sites,returns;
    unsigned complete{},packets{},prefixes{};
    for(const auto&row:rows){
        Memory seed;
        for(const auto&bank:row.at("banks"))seed.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
        const auto abi=load_abi(row);const auto name=row.at("name").get<std::string>();
        auto a=seed,b=seed;
        const auto x=run(a,abi,false),y=run(b,abi,true);
        compare(a,b,x,y,name);check(!x.stopped,"Original retained hull barycentric projection unexpectedly stopped");
        ++complete;packets+=x.packets.size();sites.insert(x.sites.begin(),x.sites.end());
        if(!returns.insert(x.abi.at("entry").get<std::uint32_t>()).second)continue;
        const auto stop=[&](int rd,int wr,unsigned ob){
            auto l=seed,r=seed;l.read_fault=r.read_fault=rd;l.write_fault=r.write_fault=wr;
            const auto u=run(l,abi,false,ob),v=run(r,abi,true,ob);
            compare(l,r,u,v,"retained "+name+" stop");
            check(u.stopped&&v.stopped,"Expected retained barycentric stop");++prefixes;
        };
        for(unsigned i=0;i<a.read_count;++i)stop(int(i),-1,0);
        for(unsigned i=0;i<a.write_count;++i)stop(-1,int(i),0);
        for(unsigned i=1;i<=x.events.size();++i)stop(-1,-1,i);
    }
    check(sites.size()==242u&&returns.size()==6u,"Incomplete retained barycentric source coverage");
    std::cout<<J{{"source_cases",complete},{"source_sites",sites.size()},{"pre_io_packets",packets},
                 {"stopped_prefixes",prefixes},{"retained_return_paths",returns}}.dump()<<'\n';
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
