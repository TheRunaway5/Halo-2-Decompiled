#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_solver_hull_begin_frame.hpp"
#include "physics/runtime/native_tu15_stack_probe_abi.hpp"
#include <iostream>
#include <utility>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;

namespace {
SourcePhysicsWorldRuntimeAbi abi(const J& row) {
    SourcePhysicsWorldRuntimeAbi a{};
    a.gp=row.at("gp").get<std::array<std::uint32_t,8>>();a.eflags=row.at("eflags");a.fs_base=row.at("fs_base");
    a.xmm=row.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();
    a.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
    a.x87_status=row.at("before_fp")[1];a.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<a.x87_depth;++n){
        const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&a.x87[n],value.data(),10);
    }
    return a;
}
J execute(Memory memory,const SourcePhysicsWorldRuntimeAbi& a,bool hostile) {
    memory.hostile=hostile;memory.observer_preimages=true;
    unsigned callbacks{},probes{};
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](std::uint32_t pc){memory.pc=pc;++callbacks;};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    auto& cursor=services.instruction_cursor;
    cursor.position=&memory.pc;cursor.callback_type=&services.instruction.target_type();
    cursor.executor=std::this_thread::get_id();cursor.service_sites=sites;
    cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    J packets=J::array(),events=J::array();
    SourcePhysicsContactRecordObserver observe=[&](const auto& e){
        events.push_back(J{{"instruction",e.instruction},{"address",e.address},
                           {"before",hex(e.before)},{"written",hex(e.written)}});
        if(memory.hostile)poison();
    };
    physics_step_detail::FpScope environment(*a.floating_point);
    WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(a);
    const auto before=[&](WorldState& s,std::uint32_t pc){packets.push_back(packet(s,pc));if(memory.hostile)poison();};
    const auto child=[&](WorldState& s,std::uint32_t target,std::uint32_t site) {
        check(target==0x32a660u&&site==0x313bd1u,"Wrong original solver reservation");
        ++probes;packets.push_back(packet(s,target));
        // The existing child ABI is qualified with ordinary providers.
        // A separate diagnostic preserves its hostile-host-FP refusal.
        const bool parent_hostile=std::exchange(memory.hostile,false);
        const bool completed=native_stack_probe_detail::run(s,before);
        memory.hostile=parent_hostile;
        check(completed,"Qualified StackProbe child refused");
    };
    const auto path=native_solver_hull_begin_detail::run_private_begin(state,child,before);
    check(probes==1u,"Expected one genuine StackProbe entry");
    memory.banks.erase(0x3146f8u);
    check(callbacks==0u,"Native hull begin executed a literal instruction callback");
    check(events==memory.writes,"Native hull begin observer store differs");
    J banks=J::array();
    for(const auto&[address,data]:memory.banks)banks.push_back(J{{"address",address},{"after",hex(data)}});
    return J{{"abi",packet(state,static_cast<std::uint32_t>(path))},{"packets",packets},{"banks",banks},
             {"reads",memory.architectural_reads()},{"writes",memory.writes}};
}
}
int main()try{
    J rows,result=J::array();std::cin>>rows;
    for(const auto&row:rows) {
        Memory memory;
        for(const auto&bank:row.at("banks"))memory.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
        memory.banks[0x3146f8u]=bytes(row.at("actual_table"));
        const auto a=abi(row);
        const auto normal=execute(memory,a,false),hostile=execute(memory,a,true);
        check(normal==hostile,"Hostile native hull begin provider contaminated ABI or storage");
        result.push_back(normal);
    }
    std::cout<<result.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
