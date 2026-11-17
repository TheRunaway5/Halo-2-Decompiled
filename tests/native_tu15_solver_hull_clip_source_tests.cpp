#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_solver_hull_clip_source_frame.hpp"
#include <iostream>
#include <set>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
struct ClipState:WorldState {
    using WorldState::WorldState;
    std::function<void(std::uint32_t)>before;
    void instruction_literal(std::uint32_t pc){WorldState::instruction_literal(pc);before(pc);}
};
#include "solver_hull_clip_source.inc"

namespace {
struct ClipMemory:Memory {
    std::function<void()> after_read;
    void read(std::uint32_t address,std::span<std::byte>data)const override{
        Memory::read(address,data);if(after_read)after_read();
    }
};
struct Result {
    J abi,packets=J::array(),events=J::array();
    std::set<std::uint32_t> sites;
    unsigned feedback{},stopped{};
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
Result run(ClipMemory&memory,const SourcePhysicsWorldRuntimeAbi&a,bool fresh,bool native,
           bool feedback=false,unsigned observer_fault=0u){
    Result result;ClipState*active{};
    SourcePhysicsWorldRuntimeServices services;
    const auto before=[&](std::uint32_t pc){result.packets.push_back(packet(*active,pc));};
    services.instruction=[&](std::uint32_t pc){
        memory.pc=pc;result.sites.insert(pc);
        if(std::binary_search(clip_source::io_sites.begin(),clip_source::io_sites.end(),pc))before(pc);
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
    ClipState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(a);
    state.before=before;active=&state;
    memory.after_read=[&]{
        if(feedback&&memory.pc==0x3147ebu&&!result.feedback){
            state.xmm[1]=_mm_set_ps(1.0f,-2.0f,0.0f,3.0f);++result.feedback;
        }
    };
    try{
        if(native&&fresh){
            const auto spill=[](auto&s){return clip_source::native_single_spill(s.fp.st(0),s.fp.status);};
            native_solver_hull_clip_source_detail::run_private_source<true>(
                state,native_solver_hull_clip_source_detail::Noop{},spill);
        }else if(native)native_solver_hull_clip_source_detail::run_private_source<false>(state);
        else if(fresh)clip_source::fresh(state);
        else clip_source::current(state);
    }catch(const Boundary&){result.stopped=true;}
    result.abi=packet(state,memory.pc);memory.after_read={};return result;
}
void compare(const ClipMemory&a,const ClipMemory&b,const Result&x,const Result&y,const std::string&name){
    if(a.banks==b.banks&&a.operations==b.operations&&a.writes==b.writes&&a.read_count==b.read_count&&
       a.write_count==b.write_count&&x.abi==y.abi&&x.packets==y.packets&&x.events==y.events&&
       x.feedback==y.feedback&&x.stopped==y.stopped)return;
    std::cerr<<name<<" ABI="<<(x.abi==y.abi)<<" I/O="<<(a.operations==b.operations)
             <<" packets="<<(x.packets==y.packets)<<" events="<<(x.events==y.events)<<'\n';
    if(x.packets!=y.packets){
        unsigned i{};while(i<x.packets.size()&&i<y.packets.size()&&x.packets[i]==y.packets[i])++i;
        if(i<x.packets.size()&&i<y.packets.size())std::cerr<<x.packets[i].dump()<<'\n'<<y.packets[i].dump()<<'\n';
    }
    throw std::runtime_error("Complete retained hull clip differs");
}
}
int main()try{
    J rows;std::cin>>rows;std::set<std::uint32_t>sites;
    std::set<std::pair<bool,std::uint32_t>>bound_prefixes;
    unsigned complete{},packets{},prefixes{},feedbacks{};
    for(const auto&row:rows){
        ClipMemory seed;
        for(const auto&bank:row.at("banks"))seed.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
        const auto abi=load_abi(row);const auto name=row.at("name").get<std::string>();
        for(bool fresh:{false,true}){
            auto a=seed,b=seed;
            const auto x=run(a,abi,fresh,false),y=run(b,abi,fresh,true);
            compare(a,b,x,y,name);check(!x.stopped,"Original retained hull clip unexpectedly stopped");
            ++complete;packets+=x.packets.size();sites.insert(x.sites.begin(),x.sites.end());
            if(name=="clip0"){
                auto l=seed,r=seed;
                const auto u=run(l,abi,fresh,false,true),v=run(r,abi,fresh,true,true);
                compare(l,r,u,v,"live source SIMD feedback after lower formal");
                check(u.feedback==1&&u.packets!=x.packets,"Missing genuine retained SIMD feedback");++feedbacks;
            }
            const auto store=std::find_if(x.events.begin(),x.events.end(),[](const auto&e){
                return e.at("instruction")==0x314836u||e.at("instruction")==0x314849u;
            });
            if(name=="fresh-directed-rounding-spill"){
                check(store!=x.events.end()&&store->at("instruction")==0x314849u&&
                      bool(store->at("abi").at("fp")[1].get<unsigned>()&0x200u)==fresh,
                      "Missing actual fresh/legacy rounded-spill C1 witness");
            }
            if(store==x.events.end()||!bound_prefixes.emplace(fresh,store->at("instruction").get<std::uint32_t>()).second)continue;
            const auto stop=[&](int rd,int wr,unsigned ob){
                auto l=seed,r=seed;l.read_fault=r.read_fault=rd;l.write_fault=r.write_fault=wr;
                const auto u=run(l,abi,fresh,false,false,ob),v=run(r,abi,fresh,true,false,ob);
                compare(l,r,u,v,"retained "+name+" stop");
                check(u.stopped&&v.stopped,"Expected retained clip stop");++prefixes;
            };
            for(unsigned i=0;i<a.read_count;++i)stop(int(i),-1,0);
            for(unsigned i=0;i<a.write_count;++i)stop(-1,int(i),0);
            for(unsigned i=1;i<=x.events.size();++i)stop(-1,-1,i);
        }
    }
    check(sites.size()==56u&&bound_prefixes.size()==4u&&feedbacks==2u,"Incomplete retained hull clip coverage");
    std::cout<<J{{"source_cases",complete},{"source_sites",sites.size()},{"pre_io_packets",packets},
                 {"stopped_prefixes",prefixes},{"feedback_witnesses",feedbacks},{"bound_prefix_scopes",bound_prefixes.size()}}.dump()<<'\n';
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
