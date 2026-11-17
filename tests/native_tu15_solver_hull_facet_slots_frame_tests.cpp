#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_solver_hull_facet_slots_frame.hpp"
#include <iostream>
#include <set>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;

namespace slots_original {
#include "solver_hull_facet_slots_actual.inc"
}

namespace {
SourcePhysicsWorldRuntimeAbi abi(const J&row){
    SourcePhysicsWorldRuntimeAbi a{};
    a.gp=row.at("gp").get<std::array<std::uint32_t,8>>();
    a.eflags=row.at("eflags");
    a.xmm=row.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();
    a.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
    a.x87_status=row.at("before_fp")[1];a.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<a.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&a.x87[n],value.data(),10);}
    return a;
}
J execute(Memory memory,const SourcePhysicsWorldRuntimeAbi&a,bool hostile){
    memory.hostile=hostile;memory.observer_preimages=true;
    unsigned callbacks{};
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](std::uint32_t pc){memory.pc=pc;++callbacks;};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    auto&cursor=services.instruction_cursor;
    cursor.position=&memory.pc;cursor.callback_type=&services.instruction.target_type();
    cursor.executor=std::this_thread::get_id();cursor.service_sites=sites;cursor.literal_service_sites=true;
    cursor.observation_only_stores=true;
    J packets=J::array(),events=J::array();
    SourcePhysicsContactRecordObserver observe=[&](const auto&e){
        events.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});
        if(hostile)poison();
    };
    physics_step_detail::FpScope environment(*a.floating_point);
    WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(a);
    native_solver_hull_facet_slots_detail::run_private_owner(state,[&](WorldState&s,std::uint32_t pc){
        packets.push_back(packet(s,pc));if(hostile)poison();
    });
    check(callbacks==0u,"Native hull facet slots executed a literal instruction callback");
    check(events==memory.writes,"Native hull facet slots completed observer store differs");
    J banks=J::array();
    for(const auto&[address,data]:memory.banks)banks.push_back(J{{"address",address},{"after",hex(data)}});
    return J{{"abi",packet(state,memory.pc)},{"packets",packets},{"banks",banks},
             {"reads",memory.architectural_reads()},{"writes",memory.writes}};
}

struct SourceMemory:Memory {
    std::function<void()> after_write;
    void write(std::uint32_t address,std::span<const std::byte>value)override{
        Memory::write(address,value);if(after_write)after_write();
    }
};
struct RecordingState:WorldState {
    using WorldState::WorldState;
    std::function<void(std::uint32_t)>before;
    void instruction_literal(std::uint32_t pc){WorldState::instruction_literal(pc);before(pc);}
};
enum class Feedback {none,request,install_observer};
struct SourceResult {
    J abi,packets=J::array(),events=J::array();
    std::set<std::uint32_t> sites;
    bool stopped{};
    unsigned writes{},feedback{};
};
SourceResult source_execute(SourceMemory&memory,const SourcePhysicsWorldRuntimeAbi&a,
                            bool native,Feedback feedback,unsigned observer_fault=0u){
    SourceResult result;
    SourcePhysicsWorldRuntimeServices services;
    RecordingState*active{};
    const auto before=[&](std::uint32_t pc){result.packets.push_back(packet(*active,pc));};
    services.instruction=[&](std::uint32_t pc){
        memory.pc=pc;result.sites.insert(pc);
        if(std::binary_search(slots_original::io_sites.begin(),slots_original::io_sites.end(),pc))before(pc);
    };
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    auto&cursor=services.instruction_cursor;
    cursor.position=&memory.pc;cursor.callback_type=&services.instruction.target_type();
    cursor.executor=std::this_thread::get_id();cursor.service_sites=sites;cursor.literal_service_sites=true;
    cursor.observation_only_stores=true;
    const auto completed=[&](const SourcePhysicsContactRecordStore&e){
        result.events.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});
        if(++result.writes==observer_fault)throw Boundary{};
    };
    SourcePhysicsContactRecordObserver observer;
    if(feedback!=Feedback::install_observer)observer=completed;
    physics_step_detail::FpScope environment(*a.floating_point);
    RecordingState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(a);
    active=&state;state.before=before;
    memory.after_write=[&]{
        if(result.feedback||memory.pc!=0x3149d3u)return;
        if(feedback==Feedback::request){state.gp[6]=1u;++result.feedback;}
        if(feedback==Feedback::install_observer){observer=completed;++result.feedback;}
    };
    try{
        if(native)native_solver_hull_facet_slots_detail::run_private_source(state);
        else slots_original::run_3149b0(state);
    }catch(const Boundary&){result.stopped=true;}
    result.abi=packet(state,memory.pc);memory.after_write={};return result;
}
void same(const SourceMemory&a,const SourceMemory&b,const SourceResult&x,const SourceResult&y){
    check(a.banks==b.banks&&a.operations==b.operations&&a.writes==b.writes&&a.read_count==b.read_count&&
          a.write_count==b.write_count&&x.abi==y.abi&&x.events==y.events&&x.packets==y.packets&&
          x.stopped==y.stopped&&x.feedback==y.feedback,"Complete retained hull facet-slot publication differs");
}
J source_proof(const J&rows){
    std::set<std::uint32_t>sites;
    unsigned complete{},packets{},prefixes{},feedbacks{},legacy_first_store_without_observer{};
    for(const auto&row:rows){
        SourceMemory seed;
        for(const auto&bank:row.at("banks"))seed.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
        const auto frame=abi(row);
        auto a=seed,b=seed;
        auto x=source_execute(a,frame,false,Feedback::none),y=source_execute(b,frame,true,Feedback::none);
        same(a,b,x,y);check(!x.stopped,"Original clear unexpectedly stopped");
        sites.insert(x.sites.begin(),x.sites.end());++complete;packets+=x.packets.size();
        if(row.at("name")!="reuse-2-request-2"&&row.at("name")!="recycled-store-aliases-output-count")continue;
        const auto stop=[&](int read,int write,unsigned observer){
            auto l=seed,r=seed;l.read_fault=r.read_fault=read;l.write_fault=r.write_fault=write;
            const auto u=source_execute(l,frame,false,Feedback::none,observer),v=source_execute(r,frame,true,Feedback::none,observer);
            same(l,r,u,v);check(v.stopped,"Expected retained clear stop not observed");++prefixes;
        };
        for(unsigned n=0;n<a.read_count;++n)stop(int(n),-1,0);
        for(unsigned n=0;n<a.write_count;++n)stop(-1,int(n),0);
        for(unsigned n=1;n<=x.events.size();++n)stop(-1,-1,n);
        for(auto mode:{Feedback::request,Feedback::install_observer}){
            auto l=seed,r=seed;
            const auto u=source_execute(l,frame,false,mode),v=source_execute(r,frame,true,mode);
            same(l,r,u,v);check(!v.stopped&&v.feedback==1,"Expected live retained feedback not observed");++feedbacks;
            if(mode==Feedback::request&&row.at("name")=="reuse-2-request-2")
                check(l.writes.size()+3==a.writes.size(),"Live request must shorten the recycle loop");
            if(mode==Feedback::install_observer){
                check(l.writes.size()==v.events.size()+5,"Observer install must skip four pushes and first recycled store");
                ++legacy_first_store_without_observer;
            }
        }
    }
    check(sites.size()==48u&&feedbacks==4&&legacy_first_store_without_observer==2,"Incomplete retained clear witness");
    return J{{"source_cases",complete},{"source_sites",sites.size()},{"pre_io_packets",packets},
             {"stopped_prefixes",prefixes},{"live_feedback_witnesses",feedbacks},
             {"retained_empty_observer_publication_witnesses",legacy_first_store_without_observer}};
}
}
int main(int argc,char**)try{
    J rows,result=J::array();std::cin>>rows;
    if(argc==2){std::cout<<source_proof(rows).dump()<<'\n';return 0;}
    for(const auto&row:rows){
        Memory memory;
        for(const auto&bank:row.at("banks"))memory.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
        const auto a=abi(row);
        const auto normal=execute(memory,a,false),hostile=execute(memory,a,true);
        check(normal==hostile,"Hostile native hull facet slots provider contaminated ABI or storage");
        result.push_back(normal);
    }
    std::cout<<result.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
