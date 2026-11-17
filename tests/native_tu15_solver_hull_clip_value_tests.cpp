#include "halo2/native_tu15_solver_hull_polytope_state.hpp"
#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include <iostream>
#include <set>

namespace halo2 {
std::uint32_t legacy_clip_solver_hull_facet(SourcePhysicsContactMemory&,std::uint32_t,std::uint32_t,
    std::uint32_t,std::uint32_t,NativeTu15PhysicsFrameFp&,unsigned,
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
J execute(RecordingMemory memory,const J&row,bool native,ObserverMode mode,
          unsigned stop_observer=0u,unsigned depth=0u){
    J events=J::array();unsigned observed{},rebound{};
    const auto completed=[&](const SourcePhysicsContactRecordStore&e){
        const auto host=native_raw_x87_detail::capture();
        events.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},
                           {"written",hex(e.written)},{"host_fp",{host.word(0),host.word(4),host.mxcsr}}});
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
    const auto&before_fp=row.at("before_fp");
    NativeTu15PhysicsFrameFp fp{before_fp[0].get<std::uint16_t>(),before_fp[1].get<std::uint16_t>(),
                                before_fp[2].get<std::uint32_t>()};
    const auto&args=row.at("arguments");
    std::uint32_t result{};bool stopped{};std::string refusal;
    const auto host_before=native_raw_x87_detail::capture();
    try{
        auto operation=native?native_tu15_clip_solver_hull_facet:legacy_clip_solver_hull_facet;
        result=operation(memory,row.at("receiver"),args[0],args[1],args[2],fp,depth,observer,&stats);
    }catch(const Boundary&){stopped=true;}
    catch(const std::runtime_error&e){if(!depth)throw;stopped=true;refusal=e.what();}
    const auto host_after=native_raw_x87_detail::capture();
    check(native_raw_x87_detail::equivalent(host_before,host_after),"Public hull clip changed caller host FP");
    check(stats.clip_calls==1u&&stats.clip_completed==!stopped&&stats.refusals==stopped&&
          stats.clear_calls==0u&&stats.clear_completed==0u,"Different public hull-clip stats lifecycle");
    if(depth)check(stopped&&memory.operations.empty()&&refusal=="Native facet clipping requires qualified empty x87 DATA",
                   "Expected exact untouched public depth refusal");
    J banks=J::array();
    for(const auto&[address,data]:memory.banks)banks.push_back(J{{"address",address},{"after",hex(data)}});
    return J{{"result",result},{"fp",{fp.control_word,fp.status,fp.mxcsr}},{"banks",banks},
             {"operations",memory.operations},{"writes",memory.writes},{"events",events},{"stopped",stopped},
             {"observer_rebounds",rebound},{"read_count",memory.read_count},{"write_count",memory.write_count},
             {"stats",{stats.clip_calls,stats.clip_completed,stats.refusals}},{"refusal",refusal}};
}
}
int main()try{
    J rows;std::cin>>rows;
    unsigned normal{},stops{},refusals{},rebinds{},events{};std::set<std::uint32_t>store_sites;
    for(const auto&row:rows){
        RecordingMemory memory;
        for(const auto&bank:row.at("banks"))memory.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
        const auto verify=[&](ObserverMode mode,int read=-1,int write=-1,unsigned observer=0,unsigned depth=0u){
            auto seed=memory;seed.read_fault=read;seed.write_fault=write;
            const auto reference=execute(seed,row,false,mode,observer,depth),actual=execute(seed,row,true,mode,observer,depth);
            check(reference==actual,"Complete public hull clip differs: "+row.at("name").get<std::string>());
            return reference;
        };
        for(auto mode:{ObserverMode::none,ObserverMode::present,ObserverMode::install,ObserverMode::remove}){
            const auto observed=verify(mode);check(!observed.at("stopped").get<bool>(),"Complete public hull clip stopped");
            if(mode==ObserverMode::install&&!observed.at("writes").empty())
                check(observed.at("events").size()==observed.at("writes").size()&&observed.at("events").front().at("before")=="",
                      "Old public clip late observer must see first store with empty preimage");
            rebinds+=observed.at("observer_rebounds").get<unsigned>();events+=observed.at("events").size();++normal;
        }
        if(refusals<16u){verify(ObserverMode::present,-1,-1,0,1u);++refusals;}
        const auto baseline=verify(ObserverMode::present);
        if(baseline.at("events").empty())continue;
        const auto site=baseline.at("events").front().at("instruction").get<std::uint32_t>();
        if(!store_sites.insert(site).second)continue;
        for(unsigned i=0;i<baseline.at("read_count").get<unsigned>();++i){
            check(verify(ObserverMode::present,int(i)).at("stopped").get<bool>(),"Expected public clip read stop");++stops;
        }
        for(unsigned i=0;i<baseline.at("write_count").get<unsigned>();++i){
            check(verify(ObserverMode::present,-1,int(i)).at("stopped").get<bool>(),"Expected public clip write stop");++stops;
        }
        for(unsigned i=1;i<=baseline.at("events").size();++i){
            check(verify(ObserverMode::present,-1,-1,i).at("stopped").get<bool>(),"Expected public clip observer stop");++stops;
        }
    }
    check(normal==rows.size()*4u&&store_sites==std::set<std::uint32_t>{0x314836u,0x314849u}&&stops>0u&&rebinds>0u,
          "Incomplete public hull-clip witness");
    std::cout<<J{{"full_public_comparisons",normal},{"stopped_public_prefixes",stops},{"depth_refusals",refusals},
                 {"observer_rebinds",rebinds},{"compared_public_events",events},{"both_store_sites",true}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
