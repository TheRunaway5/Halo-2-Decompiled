#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_solver_hull_search_setup_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_tetrahedron_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_seed_facets_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_facet_plane_frame.hpp"
#include <iostream>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;

namespace {
constexpr bool parent(std::uint32_t pc){return pc>=0x313e8eu&&pc<=0x313f5fu;}
J execute(Memory memory,const SourcePhysicsWorldRuntimeAbi& abi,bool hostile) {
    memory.hostile=hostile;memory.observer_preimages=true;
    unsigned callbacks{};
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](std::uint32_t pc){memory.pc=pc;++callbacks;};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};auto& cursor=services.instruction_cursor;
    cursor.position=&memory.pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();
    cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    J packets=J::array(),events=J::array(),children=J::array();
    SourcePhysicsContactRecordObserver observe=[&](const auto& e){
        events.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});
        if(hostile)poison();
    };
    physics_step_detail::FpScope environment(*abi.floating_point);
    WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    const auto before=[&](WorldState& s,std::uint32_t pc){if(parent(pc))packets.push_back(packet(s,pc));if(hostile)poison();};
    const auto enter=[&](WorldState& s,std::uint32_t target,std::uint32_t site,unsigned arity){
        const auto sp=s.gp[4];const auto bank=memory.at(sp,(arity+1u)*4u);std::array<std::uint32_t,5> words{};
        std::memcpy(words.data(),bank->second.data()+sp-bank->first,(arity+1u)*4u);
        check(words[0]==site+5u,"Real tetrahedron child return word differs");
        J entry=packet(s,target);entry["arguments"]=J(std::vector(words.begin()+1,words.begin()+1+arity));
        entry["return_word"]=words[0];children.push_back(std::move(entry));
    };
    const auto plane=[&](WorldState& s,std::uint32_t target,std::uint32_t site){
        check(target==0x314a30u&&(site==0x314e27u||site==0x314eb2u),"Wrong actual tetrahedron plane owner");
        enter(s,target,site,0u);native_solver_hull_facet_plane_detail::run_private_owner(s,before);
    };
    const auto seed=[&](WorldState& s,std::uint32_t target,std::uint32_t site){
        check(target==0x314d80u&&site==0x313b90u,"Wrong actual seed facets owner");
        enter(s,target,site,0u);native_solver_hull_seed_facets_detail::run_private_owner(s,plane,before);
    };
    const auto tetra=[&](WorldState& s,std::uint32_t target,std::uint32_t site){
        check(target==0x313ab0u&&site==0x313ea5u,"Wrong actual tetrahedron constructor");
        enter(s,target,site,4u);native_solver_hull_tetrahedron_detail::run_private_owner(s,seed,before);
    };
    native_solver_hull_search_setup_detail::run_private_search_setup(state,tetra,before);
    check(callbacks==0u,"Private hull search setup used a literal instruction callback");
    check(events==memory.writes,"Private hull search setup observer stores differ");
    J banks=J::array(),reads=J::array(),writes=J::array();
    for(const auto&[address,data]:memory.banks)banks.push_back(J{{"address",address},{"after",hex(data)}});
    for(const auto& e:memory.architectural_reads())if(parent(e.at("instruction")))reads.push_back(e);
    for(const auto& e:events)if(parent(e.at("instruction")))writes.push_back(e);
    return J{{"abi",packet(state,0x313f63u)},{"packets",packets},{"children",children},{"banks",banks},{"reads",reads},{"writes",writes}};
}
}
int main()try{
    J rows,out=J::array();std::cin>>rows;
    for(const auto& row:rows){
        Memory memory;for(const auto& bank:row.at("banks"))memory.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
        SourcePhysicsWorldRuntimeAbi abi{};
        abi.gp=row.at("gp").get<std::array<std::uint32_t,8>>();abi.eflags=row.at("eflags");abi.fs_base=row.at("fs_base");
        abi.xmm=row.at("xmm").get<std::array<std::array<std::uint32_t,4>,8>>();
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
        abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<abi.x87_depth;++n){const auto value=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],value.data(),10);}
        try{
            const auto normal=execute(memory,abi,false),hostile=execute(memory,abi,true);
            check(normal==hostile,"Hostile search setup providers contaminated actual child composition");out.push_back(normal);
        }catch(const std::exception& e){throw std::runtime_error(row.at("name").get<std::string>()+": "+e.what());}
    }
    std::cout<<out.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
