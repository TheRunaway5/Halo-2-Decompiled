#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_simplex_reduction_continuation_frame.hpp"
#include <iostream>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;

namespace {
constexpr std::array<std::uint32_t,11> destinations{0x30f253u,0x30f066u,0x30eb76u,0x30eb31u,
    0x30f175u,0x30ea8eu,0x30ea3eu,0x30ecd3u,0x30ea5cu,0x30eca2u,0x30edd5u};
J run(const J& row,bool hostile) {
    Memory memory;memory.observer_preimages=true;memory.hostile=hostile;
    for(const auto& bank:row.at("banks"))memory.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
    memory.banks[0x30f70cu]=bytes(row.at("selector_table"));memory.banks[0x30f6e0u]=bytes(row.at("branch_table"));
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
    const auto result=native_simplex_reduction_continuation_detail::run_private_reduction_redispatch(state,before);
    if(result==NativeTu15SimplexReductionKind::degenerate)native_simplex_reduction_continuation_detail::mark_private_simplex_degenerate(state);
    check(callbacks==0u,"Private simplex hull reduction used a literal instruction callback");
    check(events==memory.writes,"Private simplex hull reduction observer stores differ");
    memory.banks.erase(0x30f70cu);memory.banks.erase(0x30f6e0u);
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
