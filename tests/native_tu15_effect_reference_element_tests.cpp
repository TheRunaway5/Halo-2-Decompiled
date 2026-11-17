#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include "rendering/effects/native_tu15_effect_reference_element_frame.hpp"
#include <fstream>
#include <iostream>
#include <set>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
using namespace halo2::mopp_ray_frame_test;
int main(int argc,char**argv)try{
    check(argc==2,"Supply the original effect-reference fixture");std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong effect-reference image");
    const auto sites=fixture.at("io_sites").get<std::set<std::uint32_t>>();unsigned comparisons{},packets{},children{};
    for(const auto&row:fixture.at("cases"))for(bool hostile:{false,true})for(bool observe:{false,true}){
        const auto name=row.at("name").get<std::string>();const auto phase=row.at("phase").get<std::string>();
        if(phase!="normal"&&observe!=(phase=="after_write"))continue;
        Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        memory.hostile=hostile;memory.observer_preimages=observe;
        if(phase=="before_read")memory.read_fault=row.at("native_fault_ordinal");
        if(phase=="before_write")memory.write_fault=row.at("native_fault_ordinal");
        SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;unsigned stores{};
        if(observe)observer=[&](const auto&event){const auto&expected=row.at("writes").at(stores);
            check(event.instruction==expected.at("instruction")&&event.address==expected.at("address")&&
                hex(event.before)==expected.at("before").template get<std::string>()&&hex(event.written)==expected.at("written").template get<std::string>(),name+": store observer differs");
            const auto ordinal=stores++;if(hostile)poison();if(phase=="after_write"&&ordinal==row.at("native_fault_ordinal"))throw Boundary{};
        };
        WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi incoming{};
        incoming.gp=row.at("gp");incoming.eflags=row.at("eflags");incoming.xmm=row.at("xmm");
        incoming.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};incoming.x87_status=row.at("before_fp")[1];incoming.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<incoming.x87_depth;++n){const auto data=bytes(row.at("live_x87")[n]);std::memcpy(&incoming.x87[n],data.data(),10);}state.load(incoming);
        J observed=J::array();std::map<std::uint32_t,unsigned>occurrences;
        auto before=[&](auto&s,std::uint32_t pc){memory.pc=pc;if(sites.contains(pc))observed.push_back(packet(s,pc));
            if(phase=="before_instruction"&&pc==row.at("native_fault_instruction")&&occurrences[pc]++==row.at("native_fault_ordinal"))throw Boundary{};
            if(hostile)poison();};
        std::function<void(WorldState&,std::uint32_t,std::uint32_t)>dispatch;
        dispatch=[&](auto&s,auto target,auto){++children;native_effect_reference_element_detail::run_private(s,target,before,dispatch);};
        bool interrupted{};try{native_effect_reference_element_detail::run_private(state,row.at("entry"),before,dispatch);}catch(const Boundary&){interrupted=true;}
        catch(const std::exception&e){throw std::runtime_error(name+": "+e.what());}
        check(interrupted==(phase!="normal"),name+": completion differs");
        const auto expected=J{{"entry",row.at("expected_pc")},{"gp",row.at("boundary_gp")},{"eflags",row.at("boundary_eflags")},
            {"xmm",row.at("boundary_xmm")},{"fp",row.at("after_fp")},{"live_x87",row.at("boundary_live_x87")}};
        const auto actual=packet(state,row.at("expected_pc"));
        if(observed!=row.at("calls")){
            std::size_t n{};while(n<observed.size()&&n<row.at("calls").size()&&observed[n]==row.at("calls")[n])++n;
            throw std::runtime_error(name+": full packet differs at "+std::to_string(n)+" actual="+(n<observed.size()?observed[n].dump():"missing")+" expected="+(n<row.at("calls").size()?row.at("calls")[n].dump():"missing"));
        }
        check(actual==expected,name+": boundary ABI differs actual="+actual.dump()+" expected="+expected.dump());
        check(memory.architectural_reads()==row.at("reads"),name+": ordered reads differ");check(memory.writes==row.at("writes"),name+": ordered writes differ");
        for(const auto&bank:row.at("banks")){
            const auto&actual=memory.banks.at(bank.at("address"));const auto expected=bytes(bank.at("after"));
            if(actual!=expected){std::size_t n{};while(n<actual.size()&&actual[n]==expected[n])++n;
                throw std::runtime_error(name+": bank differs address="+std::to_string(bank.at("address").get<std::uint32_t>()+n)+
                    " actual="+std::to_string(std::to_integer<unsigned>(actual[n]))+" expected="+std::to_string(std::to_integer<unsigned>(expected[n])));}
        }
        ++comparisons;packets+=observed.size();
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"complete_pre_io_packets",packets},{"genuine_native_child_entries",children},{"production_binding",false}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
