#include "gameplay/animation/native_tu15_node_progress_query_source_frame.hpp"
#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include <fstream>
#include <iostream>
#include <set>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
namespace reference {
#include "node_progress_query_reference.inc"
}
#ifdef HALO2_NODE_PROGRESS_AI_LOCAL
#include "gameplay/animation/native_tu15_node_progress_query_source_owner.hpp"
#include "node_progress_local_fixture.inc"
#endif
namespace {
struct Provider final:Memory {
    WorldState*state{};
    mutable J trace=J::array();
    int stop{-1};
    void record(std::string_view kind,std::uint32_t site,J operand)const {
        trace.push_back(J{{"kind",kind},{"pc",site},{"cursor",pc},{"state",packet(*state,site)},{"operand",std::move(operand)}});
        if(int(trace.size())-1==stop)throw Boundary{};
    }
    void read(std::uint32_t p,std::span<std::byte>out)const override {
        record("read",pc,J::array({p,out.size()}));Memory::read(p,out);
    }
    void write(std::uint32_t p,std::span<const std::byte>value)override {
        record("write",pc,J::array({p,hex(value)}));Memory::write(p,value);
    }
};
struct Result {
    J trace,abi;
    std::map<std::uint32_t,std::vector<std::byte>>banks;
    bool stopped{};
    friend bool operator==(const Result&,const Result&)=default;
};
std::set<std::uint32_t> parent_sites,child_sites;
Result run(const J&row,bool literal,bool quiet,bool feedback,bool native,int stop=-1) {
    Provider memory;memory.stop=stop;
    for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
    SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp");a.eflags=row.at("eflags");a.xmm=row.at("xmm");
    a.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
    a.x87_status=row.at("before_fp")[1];a.x87_depth=row.at("live_x87").size();
    for(unsigned i=0;i<a.x87_depth;++i){
        const auto value=bytes(row.at("live_x87")[i]);std::memcpy(&a.x87[i],value.data(),10);
    }
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](std::uint32_t pc){
        memory.record("instruction",pc,J{});memory.pc=pc;
        if(!native){
            if(pc>=0x1cf0e0u&&pc<=0x1cf148u)parent_sites.insert(pc);
            else child_sites.insert(pc);
        }
    };
    const bool traced=!quiet;
    if(literal){
        static constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
        services.instruction_cursor={&memory.pc,&traced,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true};
    }
    SourcePhysicsContactRecordObserver observer=[&](const auto&event){
        memory.record("observer",event.instruction,J::array({event.address,hex(event.before),hex(event.written)}));
        if(feedback&&event.instruction==0x1cf0e9u){
            const auto b=memory.at(event.address,4);const std::uint32_t scratch=0x3f000000u;
            auto&data=memory.banks.at(b->first);
            std::memcpy(data.data()+event.address-b->first,&scratch,4);
        }
    };
    WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};memory.state=&state;
    const physics_step_detail::FpScope scope{*a.floating_point};state.load(a);
    bool stopped{};
    try {
#ifdef HALO2_NODE_PROGRESS_AI_LOCAL
        if(literal){
            if(native)candidate_literal::run_1d63c0(state);
            else retained_literal::run_1d63c0(state);
        }else{
            if(native)candidate_raw::run_1d63c0(state);
            else retained_raw::run_1d63c0(state);
        }
#else
        if(literal){
            if(native)native_node_progress_query_detail::run_source_frame<true>(state,reference::literal_child);
            else reference::literal_parent(state);
        }else{
            if(native)native_node_progress_query_detail::run_source_frame<false>(state,reference::raw_child);
            else reference::raw_parent(state);
        }
#endif
    }catch(const Boundary&){stopped=true;}
    return {std::move(memory.trace),packet(state,memory.pc),std::move(memory.banks),stopped};
}
void equal(const Result&actual,const Result&expected,const std::string&name) {
    if(actual==expected)return;
    std::size_t n{};
    while(n<actual.trace.size()&&n<expected.trace.size()&&actual.trace[n]==expected.trace[n])++n;
    throw std::runtime_error("Node progress retained source differs "+name+" event "+std::to_string(n)+
        " actual="+(n<actual.trace.size()?actual.trace[n].dump():"end")+
        " expected="+(n<expected.trace.size()?expected.trace[n].dump():"end"));
}
}
int main(int argc,char**argv)try {
    check(argc==2,"Supply original-backed node progress fixture");
    std::ifstream in(argv[1]);const auto rows=J::parse(in);
    unsigned normal{},prefixes{},packets{};
    for(std::size_t i=0;i<rows.size();++i)for(bool literal:{false,true})for(unsigned mode=0;mode<3;++mode){
        const bool quiet=literal&&mode==1,feedback=mode==2;
        if(!literal&&mode==1)continue;
        const auto name=std::to_string(i)+"/"+std::to_string(literal)+"/"+std::to_string(mode);
        auto reference=run(rows[i],literal,quiet,feedback,false);
        check(!reference.stopped,"Unexpected original node progress stop");
        equal(run(rows[i],literal,quiet,feedback,true),reference,name);++normal;packets+=reference.trace.size();
        if(i<8||i+4>=rows.size())for(std::size_t p=0;p<reference.trace.size();++p){
            auto expected=run(rows[i],literal,quiet,feedback,false,int(p));
            check(expected.stopped,"Missing exact node progress stop");
            equal(run(rows[i],literal,quiet,feedback,true,int(p)),expected,name+"/stop"+std::to_string(p));
            ++prefixes;packets+=expected.trace.size();
        }
    }
    check(parent_sites.size()==34&&child_sites.contains(0x1e3150u),"Parent or genuine child coverage incomplete");
    J result{{"passed",true},{"normal",normal},{"failure_prefixes",prefixes},{"complete_packets",packets},
        {"parent_sites",parent_sites.size()},{"actual_child_sites",child_sites.size()},{"production_bound",false}};
#ifdef HALO2_NODE_PROGRESS_AI_LOCAL
    check(child_sites.contains(0x1d63c0u),"Missing genuine AIFrame local caller");
    result["native_entries"]=native_node_progress_query_owner_detail::activity();
#endif
    std::cout<<result.dump()<<'\n';
    return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
