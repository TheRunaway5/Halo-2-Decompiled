#include "native_tu15_mopp_ray_frame_test_memory.hpp"
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
constexpr bool basis_site(std::uint32_t pc){return pc>=0x313790u&&pc<=0x313981u;}

J replay(const J&row,const Memory&seed,const SourcePhysicsWorldRuntimeAbi&abi,bool hostile){
    auto memory=seed;memory.observer_preimages=true;memory.hostile=hostile;
    SourcePhysicsWorldRuntimeServices services;unsigned callbacks{};
    services.instruction=[&](std::uint32_t){++callbacks;};
    constexpr std::array<std::uint32_t,1>sites{0x33b595u};
    auto&cursor=services.instruction_cursor;cursor.position=&memory.pc;cursor.service_sites=sites;
    cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
    cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    SourcePhysicsContactRecordObserver observe=[&](const auto&){if(hostile)poison();};
    physics_step_detail::FpScope environment(*abi.floating_point);
    WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    J packets=J::array(),children=J::array();
    const auto before=[&](WorldState&s,std::uint32_t pc){
        if(basis_site(pc))packets.push_back(packet(s,pc));
        if(hostile)poison();
    };
    const auto enter=[&](WorldState&s,std::uint32_t target,std::uint32_t pc,unsigned width,unsigned arity){
        const auto bank=memory.at(s.gp[4],(arity+1u)*4u);
        std::array<std::uint32_t,4> words{};
        std::memcpy(words.data(),bank->second.data()+s.gp[4]-bank->first,(arity+1u)*4u);
        check(words[0]==pc+width,"Actual hull basis child return word differs");
        J arguments=J::array();for(unsigned i=0;i<arity;++i)arguments.push_back(words[i+1]);
        children.push_back(J{{"target",target},{"site",pc},{"return_pc",words[0]},
                             {"arguments",arguments},{"abi",packet(s,target)}});
    };
    const auto shape=[&](WorldState&s,std::uint32_t target,std::uint32_t pc){
        enter(s,target,pc,3u,2u);
        if(target==0x1f6610u)native_polygon_support_detail::support_retained<true>(s,before);
        else if(target==0x2e8d20u)native_zero_shape_support_detail::run_private_owner(s,before);
        else if(target==0x306790u)native_box_shape_support_detail::run_private_owner(s,before);
        else throw std::runtime_error("Unqualified hull basis support owner");
    };
    const auto pair=[&](WorldState&s,std::uint32_t target,std::uint32_t pc){
        check(target==0x313390u,"Actual hull edge called a different pair owner");
        enter(s,target,pc,5u,2u);native_solver_hull_support_pair_detail::run_private_owner(s,shape,before);
    };
    const auto child=[&](WorldState&s,std::uint32_t target,std::uint32_t pc){
        enter(s,target,pc,5u,target==0x313470u?3u:1u);
        if(target==0x313470u)native_solver_hull_edge_detail::run_private_owner(s,pair,before);
        else if(target==0x313290u)native_solver_hull_append_detail::run_private_owner(s,before);
        else throw std::runtime_error("Unqualified hull basis child owner");
    };
    native_solver_hull_basis_detail::run_private_owner(state,child,before);
    check(callbacks==0u,"Private hull basis unexpectedly executed a literal callback");
    J banks=J::array(),reads=J::array();
    for(const auto&[address,data]:memory.banks)banks.push_back(J{{"address",address},{"after",hex(data)}});
    for(const auto&event:memory.architectural_reads())if(basis_site(event.at("instruction")))reads.push_back(event);
    return J{{"abi",packet(state,memory.pc)},{"packets",packets},{"children",children},
             {"banks",banks},{"reads",reads},{"writes",memory.writes}};
}

SourcePhysicsWorldRuntimeAbi input(const J&row){
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();
    abi.eflags=row.at("eflags");abi.xmm=row.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
    abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();
    for(unsigned n=0;n<abi.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],value.data(),10);}
    return abi;
}
}

int main(int argc,char**)try{
    J rows;std::cin>>rows;J results=J::array();
    for(const auto&row:rows){
        Memory seed;for(const auto&bank:row.at("banks"))seed.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
        const auto abi=input(row);auto normal=replay(row,seed,abi,false);
        if(argc==1)check(normal==replay(row,seed,abi,true),"Hull basis hostile provider changed caller effects");
        results.push_back(std::move(normal));
    }
    std::cout<<results.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
