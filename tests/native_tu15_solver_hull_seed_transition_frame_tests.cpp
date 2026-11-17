#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_solver_hull_seed_transition_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_initialize_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_line_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_basis_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_edge_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_support_pair_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_append_frame.hpp"
#include "physics/runtime/native_tu15_polygon_support_frame.hpp"
#include "physics/runtime/native_tu15_zero_shape_support_frame.hpp"
#include "physics/runtime/native_tu15_box_shape_support_frame.hpp"
#include <iostream>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;

namespace {
constexpr bool parent(std::uint32_t pc){return pc>=0x313db2u&&pc<=0x313e89u;}
J run(const J& row) {
    Memory m; m.observer_preimages=true;
    for(const auto& bank:row.at("banks"))m.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=row.at("eflags");
    abi.xmm=row.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
    abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<abi.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],value.data(),10);}
    SourcePhysicsWorldRuntimeServices services;unsigned callbacks{};
    services.instruction=[&](std::uint32_t pc){m.pc=pc;++callbacks;};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};auto& cursor=services.instruction_cursor;
    cursor.position=&m.pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();
    cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    J writes=J::array(),packets=J::array(),children=J::array();
    SourcePhysicsContactRecordObserver observe=[&](const auto& e){
        writes.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});
    };
    physics_step_detail::FpScope environment(*abi.floating_point);
    WorldState state{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    const auto before=[&](WorldState& s,std::uint32_t pc){if(parent(pc))packets.push_back(packet(s,pc));};
    const auto enter=[&](WorldState& s,std::uint32_t target,std::uint32_t pc,unsigned length,unsigned arity){
        const auto sp=s.gp[4];const auto bank=m.at(sp,(arity+1u)*4u);std::array<std::uint32_t,4> words{};
        std::memcpy(words.data(),bank->second.data()+sp-bank->first,(arity+1u)*4u);
        check(words[0]==pc+length,"Genuine hull seed transition return word differs");
        J entry=packet(s,target);entry["arguments"]=J(std::vector(words.begin()+1,words.begin()+1+arity));
        entry["return_word"]=words[0];children.push_back(std::move(entry));
    };
    const auto leaf=[&](WorldState& s,std::uint32_t target,std::uint32_t pc){
        enter(s,target,pc,3u,2u);
        switch(target){
        case 0x1f6610u:native_polygon_support_detail::support_retained<true>(s,before);return;
        case 0x2e8d20u:native_zero_shape_support_detail::run_private_owner(s,before);return;
        case 0x306790u:native_box_shape_support_detail::run_private_owner(s,before);return;
        default:throw std::runtime_error("Unexpected real shape owner");
        }
    };
    const auto pair=[&](WorldState& s,std::uint32_t target,std::uint32_t pc){
        enter(s,target,pc,5u,target==0x313290u?1u:2u);
        switch(target){
        case 0x313390u:native_solver_hull_support_pair_detail::run_private_owner(s,leaf,before);return;
        case 0x313290u:native_solver_hull_append_detail::run_private_owner(s,before);return;
        default:throw std::runtime_error("Unexpected real hull support owner");
        }
    };
    const auto nested=[&](WorldState& s,std::uint32_t target,std::uint32_t pc){
        if(target!=0x313470u){pair(s,target,pc);return;}
        enter(s,target,pc,5u,3u);native_solver_hull_edge_detail::run_private_owner(s,pair,before);
    };
    const auto child=[&](WorldState& s,std::uint32_t target,std::uint32_t pc){
        if(target==0x313470u){nested(s,target,pc);return;}
        enter(s,target,pc,5u,target==0x313560u?2u:0u);
        switch(target){
        case 0x313560u:native_solver_hull_initialize_detail::run_private_owner(s,pair,before);return;
        case 0x3136a0u:native_solver_hull_line_detail::run_private_owner(s,pair,before);return;
        case 0x313790u:native_solver_hull_basis_detail::run_private_owner(s,nested,before);return;
        default:throw std::runtime_error("Unexpected real hull seed child");
        }
    };
    const auto exit=native_solver_hull_seed_transition_detail::run_private_transition(state,
        static_cast<NativeTu15SolverHullSeedExit>(row.at("entry").get<std::uint32_t>()),child,before);
    check(callbacks==0u,"Native hull seed transition executed a literal instruction callback");
    check(writes==m.writes,"Native hull seed transition observer store differs");
    J banks=J::array(),reads=J::array(),parent_writes=J::array();
    for(const auto&[address,data]:m.banks)banks.push_back(J{{"address",address},{"after",hex(data)}});
    for(const auto& event:m.architectural_reads())if(parent(event.at("instruction")))reads.push_back(event);
    for(const auto& event:writes)if(parent(event.at("instruction")))parent_writes.push_back(event);
    return J{{"abi",packet(state,static_cast<std::uint32_t>(exit))},{"packets",packets},{"children",children},
             {"banks",banks},{"reads",reads},{"writes",parent_writes}};
}
}
int main()try{
    J rows,result=J::array();std::cin>>rows;
    for(const auto& row:rows)try{result.push_back(run(row));}catch(const std::exception&e){
        throw std::runtime_error(row.at("name").get<std::string>()+": "+e.what());
    }
    std::cout<<result.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
