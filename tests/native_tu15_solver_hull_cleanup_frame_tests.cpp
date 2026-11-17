#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_solver_hull_cleanup_frame.hpp"
#include <iostream>
#include <set>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
namespace cleanup_original {
static std::function<void(WorldState&,std::uint32_t,std::uint32_t,bool)> invoke;
static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t pc,bool tail){invoke(s,target,pc,tail);}
#include "solver_hull_cleanup_actual.inc"
}
int main()try{
    J rows;std::cin>>rows;J output=J::array();
    for(const auto&row:rows){
        Memory memory;memory.observer_preimages=true;
        for(const auto&bank:row.at("banks"))memory.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
        SourcePhysicsWorldRuntimeServices services;J packets=J::array(),children=J::array();
        services.instruction=[&](std::uint32_t pc){memory.pc=pc;};
        auto&cursor=services.instruction_cursor;cursor.position=&memory.pc;
        cursor.service_sites=cleanup_original::io_sites;cursor.callback_type=&services.instruction.target_type();
        cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
        SourcePhysicsContactRecordObserver observe=[](const auto&){};
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();
        abi.eflags=row.at("eflags");abi.fs_base=row.at("fs_base");
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f80u};abi.x87_status=0x4100u;
        abi.xmm=row.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();
        physics_step_detail::FpScope environment(*abi.floating_point);
        WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
        const auto before=[&](WorldState&s,std::uint32_t pc){packets.push_back(packet(s,pc));};
        cleanup_original::invoke=[&](WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){
            check(!tail&&target==0x149e00u,"Physical cleanup reached another allocator owner");
            const auto bank=memory.at(s.gp[4],16u);std::array<std::uint32_t,4>words{};
            std::memcpy(words.data(),bank->second.data()+s.gp[4]-bank->first,16u);
            check(words[0]==site+3u&&words[1]==0u,"Physical cleanup allocator received another ABI");
            children.push_back(J{{"target",target},{"site",site},{"return_pc",words[0]},
                                 {"arguments",J::array({words[1],words[2],words[3]})},{"abi",packet(s,target)}});
            cleanup_original::allocator_release(s);
        };
        native_solver_hull_cleanup_detail::run_private_owner(state,cleanup_original::invoke,before);
        J banks=J::array(),reads=J::array();
        for(const auto&[address,data]:memory.banks)banks.push_back(J{{"address",address},{"after",hex(data)}});
        for(const auto&event:memory.architectural_reads()){
            const auto pc=event.at("instruction").get<std::uint32_t>();
            if(pc>=0x3139f0u&&pc<=0x313aa5u)reads.push_back(event);
        }
        output.push_back(J{{"abi",packet(state,memory.pc)},{"banks",banks},{"packets",packets},
                           {"reads",reads},{"children",children},{"writes",memory.writes}});
        cleanup_original::invoke={};
    }
    std::cout<<output.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
