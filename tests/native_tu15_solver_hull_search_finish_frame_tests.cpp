#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_solver_hull_search_finish_frame.hpp"
#include "physics/runtime/native_tu15_solver_hull_cleanup_frame.hpp"
#include <iostream>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;

namespace {
constexpr bool parent(std::uint32_t pc){return pc>=0x3145eeu&&pc<=0x3146f4u;}
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
    J packets=J::array(),events=J::array(),children=J::array();
    SourcePhysicsContactRecordObserver observe=[&](const auto& e){
        events.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});
        if(hostile)poison();
    };
    physics_step_detail::FpScope environment(*abi.floating_point);
    WorldState state{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    const auto before=[&](WorldState& s,std::uint32_t pc){if(parent(pc))packets.push_back(packet(s,pc));if(hostile)poison();};
    struct AllocatorEntry{};
    const auto call=[&](WorldState& s,std::uint32_t target,std::uint32_t site){
        const auto sp=s.gp[4];const unsigned width=target==0x149e00u?3u:5u,n=target==0x149e00u?3u:0u;
        const auto bank=m.at(sp,(n+1u)*4u);std::array<std::uint32_t,4> words{};
        std::memcpy(words.data(),bank->second.data()+sp-bank->first,(n+1u)*4u);
        check(words[0]==site+width,"Genuine finish return word differs");
        J entry=packet(s,target);entry["arguments"]=J(std::vector(words.begin()+1,words.begin()+1+n));
        entry["return_word"]=words[0];children.push_back(std::move(entry));
        if(target==0x149e00u){check(row.value("allocator_prefix",false),"Unexpected real allocator entry");throw AllocatorEntry{};}
        check(target==0x3139f0u&&site==0x3146d7u,"Wrong actual finish Cleanup owner");
        const auto release=[](WorldState&,std::uint32_t,std::uint32_t,bool){throw std::runtime_error("Inline Cleanup left its authenticated scope");};
        native_solver_hull_cleanup_detail::run_private_owner(s,release,before);
    };
    std::uint32_t exit=0x63000000u;
    try{native_solver_hull_search_finish_detail::run_private_search_finish(state,call,before);}
    catch(const AllocatorEntry&){exit=0x149e00u;}
    check(callbacks==0u,"Private hull search finish used literal instruction callback");
    check(events==m.writes,"Private hull search finish observer stores differ");
    J banks=J::array(),reads=J::array(),writes=J::array();
    for(const auto&[address,data]:m.banks)banks.push_back(J{{"address",address},{"after",hex(data)}});
    for(const auto& e:m.architectural_reads())if(parent(e.at("instruction")))reads.push_back(e);
    for(const auto& e:events)if(parent(e.at("instruction")))writes.push_back(e);
    return J{{"abi",packet(state,exit)},{"packets",packets},{"children",children},{"banks",banks},
             {"reads",reads},{"writes",writes}};
}
}
int main()try{
    J rows,result=J::array();std::cin>>rows;
    for(const auto& row:rows)try{
        const auto normal=run(row,false),hostile=run(row,true);
        check(normal==hostile,"Hostile search finish provider contaminated real Cleanup or allocator entry");result.push_back(normal);
    }catch(const std::exception&e){throw std::runtime_error(row.at("name").get<std::string>()+": "+e.what());}
    std::cout<<result.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
