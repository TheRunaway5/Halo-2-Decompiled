#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_solver_hull_owner_frame.hpp"
#include <iostream>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;

namespace {
J run(const J& row,bool hostile) {
    Memory m;m.observer_preimages=true;m.hostile=hostile;
    for(const auto& bank:row.at("banks"))m.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=row.at("eflags");abi.fs_base=row.at("fs_base");
    abi.xmm=row.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
    abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<abi.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],value.data(),10);}
    SourcePhysicsWorldRuntimeServices services;unsigned callbacks{};
    services.instruction=[&](std::uint32_t pc){m.pc=pc;++callbacks;};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};auto& cursor=services.instruction_cursor;
    cursor.position=&m.pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();
    cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    J packets=J::array(),events=J::array();
    SourcePhysicsContactRecordObserver observe=[&](const auto& e){
        events.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});
        if(hostile)poison();
    };
    physics_step_detail::FpScope environment(*abi.floating_point);
    WorldState state{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    auto before=[&](WorldState& s,std::uint32_t pc){packets.push_back(packet(s,pc));if(hostile)poison();};
    auto call=[](WorldState&,std::uint32_t,std::uint32_t){throw std::runtime_error("Childless private hull exit called a provider owner");};
    native_solver_hull_owner_detail::Frame view{state,call,before};
    std::uint32_t exit=0x3144a0u;
    switch(row.at("entry").get<std::uint32_t>()){
    case 0x3141b5u:view.mark_inline_limit();break;
    case 0x314494u:view.mark_iteration_limit();break;
    case 0x314478u:view.return_degenerate();exit=0x63000000u;break;
    default:throw std::runtime_error("Unknown original hull exit");
    }
    check(callbacks==0u,"Private hull exit used a literal instruction callback");
    check(events==m.writes,"Private hull exit observer store differs");
    J banks=J::array();for(const auto&[address,data]:m.banks)banks.push_back(J{{"address",address},{"after",hex(data)}});
    return J{{"abi",packet(state,exit)},{"packets",packets},{"banks",banks},{"reads",m.architectural_reads()},{"writes",events}};
}
}
int main()try{
    J rows,out=J::array();std::cin>>rows;
    for(const auto& row:rows){
        const auto normal=run(row,false),hostile=run(row,true);
        check(normal==hostile,"Hostile provider contaminated private hull exit");out.push_back(normal);
    }
    std::cout<<out.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
