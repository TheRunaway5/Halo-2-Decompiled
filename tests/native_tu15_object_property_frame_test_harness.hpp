#pragma once
#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include <fstream>
#include <iostream>
namespace halo2::object_property_frame_test {
using namespace mopp_ray_frame_test;
struct ProviderMemory final:mopp_ray_frame_test::Memory {
    WorldState*state{};mutable J packets=J::array(),packet_operations=J::array();
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        packets.push_back(packet(*state,pc));
        packet_operations.push_back(J{{"kind","read"},{"instruction",pc},{"address",address},{"bytes",out.size()}});
        mopp_ray_frame_test::Memory::read(address,out);
    }
    void write(std::uint32_t address,std::span<const std::byte>data)override{
        packets.push_back(packet(*state,pc));
        packet_operations.push_back(J{{"kind","write"},{"instruction",pc},{"address",address},{"bytes",data.size()}});
        mopp_ray_frame_test::Memory::write(address,data);
    }
    J architectural_packets()const{
        J result=J::array();
        for(std::size_t n=0;n<packets.size();++n){
            const auto&event=packet_operations[n];
            if(observer_preimages&&event.at("kind")=="read"&&n+1<packets.size()){
                const auto&next=packet_operations[n+1];
                if(next.at("kind")=="write"&&event.at("instruction")==next.at("instruction")&&
                   event.at("address")==next.at("address")&&event.at("bytes")==next.at("bytes"))continue;
            }
            result.push_back(packets[n]);
        }
        return result;
    }
};
template<class Runner> int drive(int argc,char**argv,const char* label,Runner run)try{
    check(argc==2,std::string("Supply genuine ")+label+" fixture");std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    unsigned comparisons{},packets{},observer_packets{},observer_stops{};
    for(const auto&row:fixture)for(bool hostile:{false,true}){
        ProviderMemory memory;for(const auto&bank:row.at("banks"))memory.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
        SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;
        WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp");abi.eflags=row.at("eflags");abi.xmm=row.at("xmm");
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
        abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}const physics_step_detail::FpScope scope{*abi.floating_point};state.load(abi);
        memory.state=&state;memory.hostile=hostile;
        J observed=J::array(),observed_states=J::array();
        if(row.value("observer",false)){
            memory.observer_preimages=true;
            observer=[&](const SourcePhysicsContactRecordStore&event){
                observed.push_back(J{{"instruction",event.instruction},{"address",event.address},
                    {"before",hex(event.before)},{"written",hex(event.written)}});
                observed_states.push_back(packet(state,event.instruction));
                if(hostile)poison();
                if(row.contains("fault")&&row.at("fault").at("phase")=="after_write"&&
                   observed.size()==row.at("fault").at("ordinal").get<unsigned>()+1u)throw Boundary{};
            };
        }
        if(row.contains("fault")){
            const auto ordinal=row.at("fault").at("ordinal").get<int>();
            if(row.at("fault").at("phase")=="before_read")memory.read_fault=ordinal;
            else if(row.at("fault").at("phase")=="before_write")memory.write_fault=ordinal;
        }
        bool stopped{};
        try{run(state,[&](auto&,auto at){memory.pc=at;if(hostile)poison();});}
        catch(const Boundary&){stopped=true;}
        const auto name=row.at("name").get<std::string>()+(hostile?" hostile":" ordinary");
        check(stopped==row.contains("fault"),name+" completion differs");
        const auto actual=packet(state,memory.pc);
        check(actual==row.at("expected"),name+" final ABI differs actual="+actual.dump()+" expected="+row.at("expected").dump());
        const auto actual_packets=memory.architectural_packets();
        if(actual_packets!=row.at("calls")){
            const auto&expected=row.at("calls");unsigned first{};
            while(first<actual_packets.size()&&first<expected.size()&&actual_packets[first]==expected[first])++first;
            const auto a=first<actual_packets.size()?actual_packets[first]:J{};
            const auto b=first<expected.size()?expected[first]:J{};
            check(false,name+" pre-I/O differs at ordinal "+std::to_string(first)+" actual="+a.dump()+" expected="+b.dump()+
                " counts="+std::to_string(actual_packets.size())+"/"+std::to_string(expected.size()));
        }
        if(row.value("observer",false)){
            check(observed==row.at("writes"),name+" observer events differ");
            check(observed_states==row.at("observer_states"),name+" retired observer ABI differs");
            observer_packets+=observed.size();if(stopped)++observer_stops;
        }
        check(memory.architectural_reads()==row.at("reads"),name+" read sequence differs");
        check(memory.writes==row.at("writes"),name+" writes differ");
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+" bank differs");
        ++comparisons;packets+=actual_packets.size();
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"full_pre_io_packets",packets},
        {"retired_observer_packets",observer_packets},{"observer_throw_prefixes",observer_stops},{"production_bound",false}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
} // namespace halo2::object_property_frame_test
