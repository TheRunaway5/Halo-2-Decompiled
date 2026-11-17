#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_solver_hull_clear_source_owner.hpp"
#include <iostream>
#include <set>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
#include "solver_hull_clear_routes.inc"

namespace {
namespace owner=halo2::native_solver_hull_clear_owner_detail;
struct RouteMemory:Memory {
    std::function<void()> acquired;
    void read(std::uint32_t address,std::span<std::byte>data)const override{
        Memory::read(address,data);if(acquired)acquired();
    }
    void write(std::uint32_t address,std::span<const std::byte>data)override{
        Memory::write(address,data);if(acquired)acquired();
    }
};
struct Control{bool callbacks{},trace{},bad_cursor{},raw_install{},foreign{};std::uint32_t revoke{};};
struct Outcome{J packet,events=J::array(),callbacks=J::array();owner::Activity activity{};unsigned changes{};bool foreign{};};
SourcePhysicsWorldRuntimeAbi abi(const J&row){
    SourcePhysicsWorldRuntimeAbi a{};
    a.gp=row.at("gp");a.eflags=row.at("eflags");
    a.xmm=row.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();
    a.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
    a.x87_status=row.at("before_fp")[1];a.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<a.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&a.x87[n],value.data(),10);}
    return a;
}
template<class State>
Outcome run(RouteMemory&memory,const SourcePhysicsWorldRuntimeAbi&a,void(*entry)(State&),Control c){
    Outcome result;SourcePhysicsWorldRuntimeServices services;
    const auto callback=[&](std::uint32_t pc){memory.pc=pc;result.callbacks.push_back(pc);};
    bool trace=c.trace;
    if(c.callbacks)services.instruction=callback;
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    auto&cursor=services.instruction_cursor;
    cursor.position=&memory.pc;cursor.callback_type=&services.instruction.target_type();
    cursor.executor=c.bad_cursor?std::thread::id{}:std::this_thread::get_id();
    cursor.service_sites=sites;cursor.literal_service_sites=true;
    cursor.observation_only_stores=true;cursor.first_trace=&trace;
    const int foreign_token{};
    SourcePhysicsContactRecordObserver observe;
    if(c.callbacks)observe=[&](const auto&e){
        result.events.push_back(J{{"instruction",e.instruction},{"address",e.address},
                                   {"before",hex(e.before)},{"written",hex(e.written)}});
        if(c.foreign)throw owner::Continuation{0xdeadbeefu,&foreign_token};
    };
    memory.acquired=[&]{
        if(result.changes)return;
        if(c.raw_install){services.instruction=callback;++result.changes;}
        if(c.revoke&&memory.pc==c.revoke){trace=true;++result.changes;}
    };
    physics_step_detail::FpScope environment(*a.floating_point);
    State state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(a);
    const auto before=owner::activity();
    try{entry(state);}catch(const owner::Continuation&continuation){
        check(continuation.owner==&foreign_token&&continuation.next==0xdeadbeefu,"Foreign clear exception changed");
        result.foreign=true;
    }
    const auto after=owner::activity();
    result.activity={after.calls-before.calls,after.completed-before.completed,after.continuations-before.continuations};
    result.packet=packet(state,memory.pc);memory.acquired={};return result;
}
template<class State>
Outcome compare(const RouteMemory&seed,const SourcePhysicsWorldRuntimeAbi&a,void(*reference)(State&),
                void(*candidate)(State&),Control c){
    auto left=seed,right=seed;
    const auto expected=run(left,a,reference,c),actual=run(right,a,candidate,c);
    check(expected.packet==actual.packet&&expected.events==actual.events&&expected.callbacks==actual.callbacks&&
          expected.foreign==actual.foreign&&expected.changes==actual.changes&&left.banks==right.banks&&
          left.operations==right.operations&&left.writes==right.writes,"Exact copied hull-clear source route differs");
    return actual;
}
}
int main(int argc,char**argv)try{
    J rows;std::cin>>rows;const bool forced=argc==2&&std::string_view(argv[1])=="reference";
    unsigned paths{},native{},continuations{},refusals{},foreign{};
    std::set<std::uint32_t>frontiers;
    for(const auto&row:rows){
        RouteMemory seed;
        for(const auto&bank:row.at("banks"))seed.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
        const auto input=abi(row);
        const auto run_variant=[&](auto reference,auto candidate,bool prepared){
            const auto one=[&](Control c,unsigned calls,unsigned done,unsigned continued,std::uint32_t first_pc=0u){
                const auto r=compare(seed,input,reference,candidate,c);
                check(r.activity.calls==calls&&r.activity.completed==done&&r.activity.continuations==continued,
                      "Wrong exact hull-clear admission or retained continuation");
                if(first_pc)check(!r.callbacks.empty()&&r.callbacks.front()==first_pc,"Wrong retired hull-clear successor");
                ++paths;native+=r.activity.calls;continuations+=r.activity.continuations;refusals+=!r.activity.calls;foreign+=r.foreign;
            };
            one(Control{.callbacks=prepared},!forced,!forced,0);
            if(forced)return;
            if(!prepared){
                one(Control{.callbacks=true},0,0,0,0x314870u);
                one(Control{.raw_install=true},1,0,1,0x314876u);
                return;
            }
            one(Control{.callbacks=true,.trace=true},0,0,0,0x314870u);
            one(Control{.callbacks=true,.bad_cursor=true},0,0,0,0x314870u);
            if(row.at("name")!="counts-5-6")return;
            for(const auto [pc,next]:clear_routes::frontiers){
                one(Control{.callbacks=true,.revoke=pc},1,0,1,next);frontiers.insert(pc);
            }
            one(Control{.callbacks=true,.foreign=true},1,0,0);
        };
        run_variant(clear_current_raw::retained,clear_current_raw::run_314870,false);
        run_variant(clear_fresh_raw::retained,clear_fresh_raw::run_314870,false);
        run_variant(clear_current_prepared::retained,clear_current_prepared::run_314870,true);
        run_variant(clear_fresh_prepared::retained,clear_fresh_prepared::run_314870,true);
    }
    check(forced?(paths==180u&&native==0u&&refusals==paths):(frontiers.size()==12u&&foreign==2u&&continuations==114u),
          "Incomplete hull-clear source route witnesses");
    std::cout<<J{{"paths",paths},{"native_entries",native},{"retained_continuations",continuations},
                 {"whole_entry_refusals",refusals},{"exact_frontiers",frontiers.size()},{"foreign_exceptions",foreign}}.dump()<<'\n';
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
