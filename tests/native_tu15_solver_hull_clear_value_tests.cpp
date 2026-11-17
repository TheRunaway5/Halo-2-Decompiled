#include "halo2/native_tu15_solver_hull_polytope_state.hpp"
#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include <iostream>

namespace halo2 {
void legacy_clear_solver_hull_polytope_flags(SourcePhysicsContactMemory&,std::uint32_t,
    const SourcePhysicsContactRecordObserver&,NativeTu15SolverHullPolytopeStateStats*);
}
using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
namespace {
struct RecordingMemory:Memory {
    std::function<void()> after_write;
    void write(std::uint32_t address,std::span<const std::byte>data)override{
        Memory::write(address,data);if(after_write)after_write();
    }
};
enum class ObserverMode {none,present,install,remove};
J execute(RecordingMemory memory,std::uint32_t polytope,bool native,ObserverMode mode,
          unsigned stop_observer=0u){
    J events=J::array();unsigned observed{},rebound{};
    const auto completed=[&](const SourcePhysicsContactRecordStore&e){
        events.push_back(J{{"instruction",e.instruction},{"address",e.address},
                           {"before",hex(e.before)},{"written",hex(e.written)}});
        if(++observed==stop_observer)throw Boundary{};
    };
    SourcePhysicsContactRecordObserver observer;
    if(mode==ObserverMode::present||mode==ObserverMode::remove)observer=completed;
    memory.after_write=[&]{
        if(rebound)return;
        if(mode==ObserverMode::install){observer=completed;++rebound;}
        if(mode==ObserverMode::remove){observer={};++rebound;}
    };
    NativeTu15SolverHullPolytopeStateStats stats{};
    bool stopped{};
    try{
        if(native)native_tu15_clear_solver_hull_polytope_flags(memory,polytope,observer,&stats);
        else legacy_clear_solver_hull_polytope_flags(memory,polytope,observer,&stats);
    }catch(const Boundary&){stopped=true;}
    check(stats.clear_calls==1u&&stats.clear_completed==!stopped&&stats.refusals==stopped&&
          stats.clip_calls==0u&&stats.clip_completed==0u,"Different public hull-clear stats lifecycle");
    J banks=J::array();
    for(const auto&[address,data]:memory.banks)banks.push_back(J{{"address",address},{"after",hex(data)}});
    return J{{"banks",banks},{"operations",memory.operations},{"writes",memory.writes},
             {"events",events},{"stopped",stopped},{"observer_rebounds",rebound},
             {"read_count",memory.read_count},{"write_count",memory.write_count},
             {"stats",{stats.clear_calls,stats.clear_completed,stats.facets_cleared,stats.edges_cleared,stats.refusals}}};
}
}
int main()try{
    J rows;std::cin>>rows;
    unsigned normal{},stops{},rebinds{},events{};
    for(const auto&row:rows){
        RecordingMemory memory;
        for(const auto&bank:row.at("banks"))memory.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
        const auto polytope=row.at("gp")[1].get<std::uint32_t>();
        const auto verify=[&](ObserverMode mode,int read=-1,int write=-1,unsigned observer=0){
            auto seed=memory;seed.read_fault=read;seed.write_fault=write;
            const auto reference=execute(seed,polytope,false,mode,observer),actual=execute(seed,polytope,true,mode,observer);
            check(reference==actual,"Complete public hull clear differs: "+row.at("name").get<std::string>());
            return reference;
        };
        for(auto mode:{ObserverMode::none,ObserverMode::present,ObserverMode::install,ObserverMode::remove}){
            const auto observed=verify(mode);
            check(!observed.at("stopped").get<bool>(),"Complete public hull clear stopped");
            if(mode==ObserverMode::install&&!observed.at("writes").empty())
                check(observed.at("events").size()==observed.at("writes").size()&&
                      observed.at("events").front().at("before")=="",
                      "Old public late observer must see first store with empty preimage");
            rebinds+=observed.at("observer_rebounds").get<unsigned>();events+=observed.at("events").size();++normal;
        }
        if(row.at("name")!="counts-5-6"&&row.at("name")!="facet-count-alias")continue;
        const auto baseline=verify(ObserverMode::present);
        for(unsigned i=0;i<baseline.at("read_count").get<unsigned>();++i){
            check(verify(ObserverMode::present,int(i)).at("stopped").get<bool>(),"Expected public read stop");++stops;
        }
        for(unsigned i=0;i<baseline.at("write_count").get<unsigned>();++i){
            check(verify(ObserverMode::present,-1,int(i)).at("stopped").get<bool>(),"Expected public write stop");++stops;
        }
        for(unsigned i=1;i<=baseline.at("events").size();++i){
            check(verify(ObserverMode::present,-1,-1,i).at("stopped").get<bool>(),"Expected public observer stop");++stops;
        }
    }
    check(normal==180u&&stops>0u&&rebinds>0u,"Incomplete public hull-clear witness");
    std::cout<<J{{"full_public_comparisons",normal},{"stopped_public_prefixes",stops},
                 {"observer_rebinds",rebinds},{"compared_public_events",events}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
