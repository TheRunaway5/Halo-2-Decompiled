#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_solver_hull_search_setup_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_search_iteration_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_tetrahedron_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_seed_facets_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_facet_plane_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_minimum_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_support_pair_frame.hpp"
#include "physics/runtime/native_tu15_zero_shape_support_frame.hpp"
#include "physics/runtime/native_tu15_geometry_leaf_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_facet_search_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_clip_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_edge_insert_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_facet_slots_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_facet_ring_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_clear_frame.hpp"
#include <iostream>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;

namespace {
constexpr bool parent(std::uint32_t pc){return pc>=0x313f63u&&pc<=0x3140d7u;}
J run(const J& row) {
    Memory m;m.observer_preimages=true;
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
    J packets=J::array(),events=J::array(),children=J::array();
    SourcePhysicsContactRecordObserver observe=[&](const auto& e){
        events.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});
    };
    physics_step_detail::FpScope environment(*abi.floating_point);
    WorldState state{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    const auto before=[&](WorldState& s,std::uint32_t pc){if(parent(pc))packets.push_back(packet(s,pc));};
    const auto enter=[&](WorldState& s,std::uint32_t target,std::uint32_t site,unsigned n,unsigned length=5u){
        const auto sp=s.gp[4];const auto bank=m.at(sp,(n+1u)*4u);std::array<std::uint32_t,5> words{};
        std::memcpy(words.data(),bank->second.data()+sp-bank->first,(n+1u)*4u);
        check(words[0]==site+length,"Genuine search iteration child return word differs");
        J entry=packet(s,target);entry["arguments"]=J(std::vector(words.begin()+1,words.begin()+1+n));
        entry["return_word"]=words[0];children.push_back(std::move(entry));
    };
    const auto plane=[&](WorldState& s,std::uint32_t target,std::uint32_t site){
        check(target==0x314a30u,"Wrong actual Plane target");enter(s,target,site,0u);
        native_solver_hull_facet_plane_detail::run_private_owner(s,before);
    };
    const auto seed=[&](WorldState& s,std::uint32_t target,std::uint32_t site){
        check(target==0x314d80u,"Wrong actual Seed target");enter(s,target,site,0u);
        native_solver_hull_seed_facets_detail::run_private_owner(s,plane,before);
    };
    const auto tetra=[&](WorldState& s,std::uint32_t target,std::uint32_t site){
        check(target==0x313ab0u,"Wrong actual Tetrahedron target");enter(s,target,site,4u);
        native_solver_hull_tetrahedron_detail::run_private_owner(s,seed,before);
    };
    const auto zero=[&](WorldState& s,std::uint32_t target,std::uint32_t site){
        check(target==0x2e8d20u,"Wrong actual Zero target");enter(s,target,site,2u,3u);
        native_zero_shape_support_detail::run_private_owner(s,before);
    };
    const auto clip=[&](WorldState& s,std::uint32_t target,std::uint32_t site){
        check(target==0x3147d0u,"Wrong actual Clip target");enter(s,target,site,3u);
        native_solver_hull_clip_detail::run_private_owner(s,before);
    };
    unsigned visits{};
    const auto recurse=[&](auto& self,WorldState& s,std::uint32_t target,std::uint32_t site)->void{
        if(target==0x3147d0u){clip(s,target,site);return;}
        check(target==0x3148e0u&&++visits<=100u,"Wrong actual Visit target/budget");enter(s,target,site,4u);
        const auto call=[&](WorldState& next,std::uint32_t next_target,std::uint32_t next_site){self(self,next,next_target,next_site);};
        native_solver_hull_facet_visit_detail::run_private_owner(s,call,before);
    };
    const auto walk=[&](WorldState& s,std::uint32_t target,std::uint32_t site){recurse(recurse,s,target,site);};
    const auto ring_child=[&](WorldState& s,std::uint32_t target,std::uint32_t site){
        if(target==0x314a30u){plane(s,target,site);return;}
        check(target==0x314870u,"Wrong actual Clear target");enter(s,target,site,0u);
        native_solver_hull_clear_detail::run_private_owner(s,before);
    };
    const auto insert_child=[&](WorldState& s,std::uint32_t target,std::uint32_t site){
        if(target==0x3149b0u){enter(s,target,site,2u);native_solver_hull_facet_slots_detail::run_private_owner(s,before);return;}
        check(target==0x314cb0u,"Dynamic allocator route remains unqualified");enter(s,target,site,4u);
        native_solver_hull_facet_ring_detail::run_private_owner(s,ring_child,before);
    };
    const auto iterate_child=[&](WorldState& s,std::uint32_t target,std::uint32_t site){
        switch(target){
        case 0x3131b0u:enter(s,target,site,0u);native_solver_hull_minimum_detail::run_private_owner(s,before);return;
        case 0x313390u:enter(s,target,site,2u);native_solver_hull_support_pair_detail::run_private_owner(s,zero,before);return;
        case 0x2e47b0u:enter(s,target,site,2u);native_geometry_leaf_detail::run_private(s,native_geometry_leaf_detail::Kind::direction,before);return;
        case 0x314c40u:enter(s,target,site,4u);native_solver_hull_facet_search_detail::run_private_owner(s,walk,before);return;
        case 0x314ed0u:enter(s,target,site,3u);native_solver_hull_edge_insert_detail::run_private_owner(s,insert_child,before);return;
        default:throw std::runtime_error("Unexpected genuine hull iteration child target");
        }
    };
    if(row.at("entry")==0x313e8eu)native_solver_hull_search_setup_detail::run_private_search_setup(state,tetra,before);
    else check(row.at("entry")==0x313f63u,"Unknown actual search iteration fixture start");
    const auto exit=native_solver_hull_search_iteration_detail::run_private_search_iteration(state,iterate_child,before);
    check(callbacks==0u,"Native hull search iteration executed a literal instruction callback");
    check(events==m.writes,"Native hull search iteration observer store differs");
    J banks=J::array(),reads=J::array(),writes=J::array();
    for(const auto&[address,data]:m.banks)banks.push_back(J{{"address",address},{"after",hex(data)}});
    for(const auto& e:m.architectural_reads())if(parent(e.at("instruction")))reads.push_back(e);
    for(const auto& e:events)if(parent(e.at("instruction")))writes.push_back(e);
    return J{{"abi",packet(state,static_cast<std::uint32_t>(exit))},{"packets",packets},{"children",children},
             {"banks",banks},{"reads",reads},{"writes",writes}};
}
}
int main()try{
    J rows,result=J::array();std::cin>>rows;
    for(const auto& row:rows)try{result.push_back(run(row));}catch(const std::exception&e){
        throw std::runtime_error(row.at("name").get<std::string>()+": "+e.what());
    }
    std::cout<<result.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
