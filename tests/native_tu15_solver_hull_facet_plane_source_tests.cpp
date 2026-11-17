#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_solver_hull_facet_plane_source_frame.hpp"
#include "halo2/source_execution_inputs.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <iostream>
#include <set>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
struct PlaneState:WorldState {
    using WorldState::WorldState;
    std::function<void(std::uint32_t)>before;
    void instruction_literal(std::uint32_t pc){WorldState::instruction_literal(pc);before(pc);}
};
#include "solver_hull_facet_plane_source.inc"

namespace {
std::uint32_t rsqrt(std::uint32_t bits){
    const auto x=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));
    return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(_mm_rsqrt_ss(x)));
}
struct Journal {
    SourceRuntimeMemory memory;
    SourceExecutionInputBinding binding;
    unsigned computed{};
    explicit Journal(SourceExecutionInputs&inputs):binding(memory,inputs){}
    std::uint32_t sample(std::uint32_t operand){
        return static_cast<std::uint32_t>(source_execution_input(memory,SourceExecutionInputKind::ReciprocalSquareRoot,operand,[&]{
            ++computed;return rsqrt(operand);
        }));
    }
};
struct Result{
    J abi,packets=J::array(),events=J::array(),estimates=J::array();
    std::set<std::uint32_t>visited;
    bool stopped{};
    std::string error;
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
Result run(Memory&memory,const SourcePhysicsWorldRuntimeAbi&a,bool native,bool missing_estimate=false,
           unsigned stop_observer=0u,Journal*journal=nullptr){
    Result result;PlaneState*active{};
    const auto before=[&](std::uint32_t pc){result.packets.push_back(packet(*active,pc));};
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](std::uint32_t pc){
        memory.pc=pc;result.visited.insert(pc);
        if(std::binary_search(plane_original::io_sites.begin(),plane_original::io_sites.end(),pc))before(pc);
    };
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    auto&cursor=services.instruction_cursor;
    cursor.position=&memory.pc;cursor.callback_type=&services.instruction.target_type();
    cursor.executor=std::this_thread::get_id();cursor.service_sites=sites;cursor.literal_service_sites=true;
    cursor.observation_only_stores=true;
    if(!missing_estimate)services.reciprocal_sqrt=[&](std::uint32_t operand){
        const auto result_bits=journal?journal->sample(operand):rsqrt(operand);
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
    PlaneState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(a);
    state.before=before;active=&state;
    try{
        if(native)native_solver_hull_facet_plane_detail::run_private_source(state,plane_original::source_estimate);
        else plane_original::retained(state);
    }catch(const Boundary&){result.stopped=true;}
    catch(const std::runtime_error&e){result.stopped=true;result.error=e.what();}
    result.abi=packet(state,memory.pc);return result;
}
void compare(const Memory&a,const Memory&b,const Result&x,const Result&y,const std::string&name){
    if(a.banks==b.banks&&a.operations==b.operations&&a.writes==b.writes&&a.read_count==b.read_count&&
       a.write_count==b.write_count&&x.abi==y.abi&&x.packets==y.packets&&x.events==y.events&&
       x.estimates==y.estimates&&x.stopped==y.stopped&&x.error==y.error)return;
    std::cerr<<name<<" ABI="<<(x.abi==y.abi)<<" I/O="<<(a.operations==b.operations)
             <<" packets="<<(x.packets==y.packets)<<" events="<<(x.events==y.events)
             <<" estimates="<<(x.estimates==y.estimates)<<'\n';
    if(x.packets!=y.packets){
        unsigned i{};while(i<x.packets.size()&&i<y.packets.size()&&x.packets[i]==y.packets[i])++i;
        if(i<x.packets.size()&&i<y.packets.size())std::cerr<<x.packets[i].dump()<<'\n'<<y.packets[i].dump()<<'\n';
    }
    throw std::runtime_error("Complete retained facet plane differs");
}
}
int main()try{
    J rows;std::cin>>rows;std::set<std::uint32_t>visited;
    unsigned cases{},packets{},stops{},journals{},operand_refusals{},missing_refusals{};
    for(const auto&row:rows){
        Memory seed;
        for(const auto&bank:row.at("banks"))seed.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
        const auto abi=input_abi(row);const auto name=row.at("name").get<std::string>();
        auto a=seed,b=seed;
        const auto x=run(a,abi,false),y=run(b,abi,true);
        compare(a,b,x,y,name);check(!x.stopped&&x.estimates.size()==1u,"Retained facet plane must request exactly one estimate");
        ++cases;packets+=x.packets.size();visited.insert(x.visited.begin(),x.visited.end());
        if(row.at("id").get<unsigned>()>=8u)continue;
        const auto stop=[&](int rd,int wr,unsigned ob,bool missing=false){
            auto l=seed,r=seed;l.read_fault=r.read_fault=rd;l.write_fault=r.write_fault=wr;
            const auto u=run(l,abi,false,missing,ob),v=run(r,abi,true,missing,ob);
            compare(l,r,u,v,name+" retained prefix");check(u.stopped&&v.stopped,"Missing stopped facet-plane prefix");++stops;
            if(missing){check(u.error=="Required original contact RSQRTSS owner unavailable"&&u.estimates.empty(),
                              "Wrong missing estimate refusal");++missing_refusals;}
        };
        stop(-1,-1,0,true);
        for(unsigned i=0;i<a.read_count;++i)stop(int(i),-1,0);
        for(unsigned i=0;i<a.write_count;++i)stop(-1,int(i),0);
        for(unsigned i=1;i<=x.events.size();++i)stop(-1,-1,i);
        auto l=seed,r=seed;SourceExecutionInputs capture;Result expected,actual;
        {Journal journal(capture);expected=run(l,abi,false,false,0,&journal);
         check(!expected.stopped&&journal.computed==1u,"Real original estimate was not recorded");}
        const auto entries=capture.finish();check(entries.size()==1&&entries.front().kind==SourceExecutionInputKind::ReciprocalSquareRoot,
                                                 "Wrong genuine estimate journal shape");
        SourceExecutionInputs replay(entries);
        {Journal journal(replay);actual=run(r,abi,true,false,0,&journal);
         check(journal.computed==0u,"Facet-plane replay called the host estimate producer");}
        check(replay.finish().size()==1u,"Facet-plane replay did not consume its genuine estimate");
        compare(l,r,expected,actual,name+" real estimate replay");++journals;
        auto changed=std::vector(entries.begin(),entries.end());changed[0].operand^=1u;
        SourceExecutionInputs left(changed),right(changed);l=seed;r=seed;
        {Journal journal(left);expected=run(l,abi,false,false,0,&journal);check(!journal.computed,"Old rejected operand called host");}
        {Journal journal(right);actual=run(r,abi,true,false,0,&journal);check(!journal.computed,"Native rejected operand called host");}
        compare(l,r,expected,actual,name+" mismatched estimate");
        check(expected.stopped&&expected.estimates.empty()&&expected.abi.at("entry")==0x314a9au,
              "Estimate operand mismatch did not stop at the exact authored site");++operand_refusals;
    }
    check(visited.size()==66u&&journals==8u&&operand_refusals==8u&&missing_refusals==8u,"Incomplete retained facet-plane source domain");
    std::cout<<J{{"source_cases",cases},{"source_sites",visited.size()},{"pre_io_packets",packets},{"stopped_prefixes",stops},
                 {"record_replay_pairs",journals},{"wrong_operand_refusals",operand_refusals},
                 {"missing_estimate_refusals",missing_refusals}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
