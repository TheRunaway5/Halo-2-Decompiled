#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_solver_hull_cleanup_source_frame.hpp"
#include <iostream>
#include <set>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
namespace cleanup_original {
static std::function<void(WorldState&,std::uint32_t,std::uint32_t,bool)> invoke;
static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t pc,bool tail){invoke(s,target,pc,tail);}
#include "solver_hull_cleanup_actual.inc"
}
namespace {
struct ProbeMemory:Memory {
    std::function<void()> after_read;
    void read(std::uint32_t address,std::span<std::byte> bytes)const override{
        Memory::read(address,bytes);if(after_read)after_read();
    }
};
struct Outcome {
    J abi,packets=J::array(),children=J::array(),events=J::array();
    std::set<std::uint32_t> visited;
    bool stopped{};
    unsigned feedback{};
};
Outcome execute(ProbeMemory&memory,const SourcePhysicsWorldRuntimeAbi&abi,bool native,bool alter_capacity,unsigned fail_observer=0u){
    Outcome result;
    SourcePhysicsWorldRuntimeServices services;
    WorldState*state_pointer{};
    services.instruction=[&](std::uint32_t pc){
        memory.pc=pc;
        if(pc>=0x3139f0u&&pc<=0x313aa5u)result.visited.insert(pc);
        if(std::binary_search(cleanup_original::io_sites.begin(),cleanup_original::io_sites.end(),pc))result.packets.push_back(packet(*state_pointer,pc));
    };
    auto&cursor=services.instruction_cursor;
    cursor.position=&memory.pc;cursor.service_sites=cleanup_original::io_sites;
    cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
    cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    unsigned observed{};
    SourcePhysicsContactRecordObserver observe=[&](const auto&e){
        result.events.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});
        if(++observed==fail_observer)throw Boundary{};
    };
    physics_step_detail::FpScope environment(*abi.floating_point);
    WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);state_pointer=&state;
    unsigned release_depth{};
    memory.after_read=[&]{
        if(alter_capacity&&release_depth&&memory.pc==0x149e7eu&&!result.feedback){
            const auto address=state.gp[6]+0x1b8u;
            const auto bank=memory.at(address,4u);
            const std::uint32_t external=0x80000000u;
            std::memcpy(memory.banks.at(bank->first).data()+address-bank->first,&external,4u);++result.feedback;
        }
    };
    cleanup_original::invoke=[&](WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){
        check(!tail&&target==0x149e00u,"Different cleanup allocator target");
        check(site==0x313a3au||site==0x313a69u||site==0x313a92u,"Different cleanup allocator site");
        const auto bank=memory.at(s.gp[4],16u);
        std::array<std::uint32_t,4> words{};
        std::memcpy(words.data(),bank->second.data()+s.gp[4]-bank->first,16u);
        check(words[0]==site+3u&&words[1]==0u&&words[3]==0x12u,"Actual allocator release ABI changed");
        result.children.push_back(J{{"target",target},{"site",site},{"arguments",words},{"entry",packet(s,target)}});
        ++release_depth;
        try{cleanup_original::allocator_release(s);}catch(...){--release_depth;throw;}
        --release_depth;result.children.back()["returned"]=packet(s,words[0]);
    };
    try{
        if(native)native_solver_hull_cleanup_source_detail::run_private_source(state,cleanup_original::invoke);
        else cleanup_original::cleanup(state);
    }catch(const Boundary&){result.stopped=true;}
    result.abi=packet(state,memory.pc);result.abi["fs_base"]=state.fs_base;
    memory.after_read={};cleanup_original::invoke={};return result;
}
void same(const ProbeMemory&left,const ProbeMemory&right,const Outcome&x,const Outcome&y){
    if(left.banks==right.banks&&left.operations==right.operations&&left.writes==right.writes&&
       x.abi==y.abi&&x.events==y.events&&x.children==y.children&&x.packets==y.packets&&
       x.stopped==y.stopped&&x.feedback==y.feedback)return;
    std::cerr<<"ABI="<<(x.abi==y.abi)<<" operations="<<(left.operations==right.operations)
             <<" packets="<<(x.packets==y.packets)<<" child="<<(x.children==y.children)<<"\n";
    throw std::runtime_error("Actual hull cleanup source differs");
}
}
int main()try{
    J rows;std::cin>>rows;std::set<std::uint32_t>visited;
    unsigned calls{},children{},packets{},feedbacks{},stops{};
    for(const auto&row:rows){
        ProbeMemory seed;for(const auto&bank:row.at("banks"))seed.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=row.at("eflags");
        abi.fs_base=row.at("fs_base");abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f80u};
        abi.x87_status=0x4100u;abi.xmm=row.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();
        for(bool feedback:{false,true}){
            auto a=seed,b=seed;auto x=execute(a,abi,false,feedback),y=execute(b,abi,true,feedback);same(a,b,x,y);
            check(!x.stopped,"Original null-data allocator release did not return");
            visited.insert(x.visited.begin(),x.visited.end());++calls;children+=x.children.size();packets+=x.packets.size();feedbacks+=x.feedback;
            if(row.at("id").get<unsigned>()>=8u||feedback)continue;
            for(unsigned n=0;n<a.read_count;++n){auto l=seed,r=seed;l.read_fault=r.read_fault=int(n);auto u=execute(l,abi,false,false),v=execute(r,abi,true,false);same(l,r,u,v);check(v.stopped,"Missing cleanup read stop");++stops;}
            for(unsigned n=0;n<a.write_count;++n){auto l=seed,r=seed;l.write_fault=r.write_fault=int(n);auto u=execute(l,abi,false,false),v=execute(r,abi,true,false);same(l,r,u,v);check(v.stopped,"Missing cleanup write stop");++stops;}
            for(unsigned n=1;n<=x.events.size();++n){auto l=seed,r=seed;auto u=execute(l,abi,false,false,n),v=execute(r,abi,true,false,n);same(l,r,u,v);check(v.stopped,"Missing cleanup observer stop");++stops;}
        }
    }
    check(visited.size()==56u&&feedbacks>0u,"Incomplete genuine cleanup source coverage");
    std::cout<<J{{"source_cases",calls},{"parent_sites",visited.size()},{"genuine_149e00_calls",children},
                 {"parent_pre_io_packets",packets},{"feedback_changes",feedbacks},{"stopped_prefixes",stops}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
