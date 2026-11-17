#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_simplex_hull_reduction_route_frame.hpp"
#include <iostream>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;

namespace {
constexpr std::array<std::uint32_t,11> destinations{0x30e89bu,0x30e6bfu,0x30e3a7u,0x30e362u,
    0x30e7c7u,0x30e2afu,0x30e25fu,0x30e504u,0x30e27du,0x30e4d3u,0x30e8b5u};
J run(const J& row,bool hostile) {
    Memory memory;memory.observer_preimages=true;memory.hostile=hostile;
    for(const auto& bank:row.at("banks"))memory.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
    memory.banks[0x30e908u]=bytes(row.at("selector_table"));memory.banks[0x30e8dcu]=bytes(row.at("branch_table"));
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=row.at("eflags");
    abi.xmm=row.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
    abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<abi.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],value.data(),10);}
    SourcePhysicsWorldRuntimeServices services;unsigned callbacks{};
    services.instruction=[&](std::uint32_t pc){memory.pc=pc;++callbacks;};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};auto& cursor=services.instruction_cursor;
    cursor.position=&memory.pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();
    cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    J packets=J::array(),events=J::array();
    SourcePhysicsContactRecordObserver observe=[&](const auto& e){
        events.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});
        if(hostile)poison();
    };
    physics_step_detail::FpScope environment(*abi.floating_point);
    WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    const auto before=[&](WorldState& s,std::uint32_t pc){packets.push_back(packet(s,pc));if(hostile)poison();};
    const auto result=row.at("entry")==0x30e5ecu?
        native_simplex_hull_reduction_route_detail::run_private_hull_redispatch(state,before):
        native_simplex_hull_reduction_route_detail::run_private_hull_reduction_route(state,before);
    check(callbacks==0u,"Private simplex hull reduction used a literal instruction callback");
    check(events==memory.writes,"Private simplex hull reduction observer stores differ");
    memory.banks.erase(0x30e908u);memory.banks.erase(0x30e8dcu);
    J banks=J::array();for(const auto&[address,data]:memory.banks)banks.push_back(J{{"address",address},{"after",hex(data)}});
    return J{{"abi",packet(state,destinations.at(static_cast<unsigned>(result)))},{"packets",packets},
             {"banks",banks},{"reads",memory.architectural_reads()},{"writes",events}};
}
}
int main()try{
    J rows,result=J::array();std::cin>>rows;
    for(const auto& row:rows){
        const auto normal=run(row,false),hostile=run(row,true);
        check(normal==hostile,"Hostile provider contaminated simplex hull table selection");result.push_back(normal);
    }
    std::cout<<result.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
