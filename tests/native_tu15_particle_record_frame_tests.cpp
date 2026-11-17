#include "rendering/effects/native_tu15_particle_record_frame.hpp"
#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include <fstream>
#include <iostream>

// The physical original traces one packet at the start of every instruction
// step with a memory operand. A REP MOVS iteration reads and writes in one
// step, and an empty REP still steps, so packets follow the frame's per-step
// position rather than each provider access.
namespace {
using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
struct StepMemory final:Memory {
    WorldState*state{};J packets=J::array();std::size_t step_start{};
    void step(){packets.push_back(packet(*state,pc));step_start=operations.size();}
    // A step stopped before its write never executed, so its own source read
    // (the first half of a MOVS iteration) never happened either.
    void write(std::uint32_t address,std::span<const std::byte>value)override{
        if(int(write_count)==write_fault){
            J kept=J::array();
            for(std::size_t n=0;n<operations.size();++n)
                if(n<step_start||operations[n].at("kind")!="read")kept.push_back(operations[n]);
            operations=kept;
        }
        Memory::write(address,value);
    }
};
} // namespace

int main(int argc,char**argv)try{
    check(argc==2,"Supply genuine 1E4C0 particle record fixture");std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    unsigned comparisons{},packets{},observer_packets{},observer_stops{};
    for(const auto&row:fixture)for(bool hostile:{false,true}){
        StepMemory memory;for(const auto&bank:row.at("banks"))memory.banks[bank.at("address").get<std::uint32_t>()]=bytes(bank.at("before"));
        SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;
        WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp");abi.eflags=row.at("eflags");abi.xmm=row.at("xmm");
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
        abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}
        const physics_step_detail::FpScope scope{*abi.floating_point};state.load(abi);
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
        try{native_particle_record_detail::run_private(state,[&](auto&,auto at){memory.pc=at;memory.step();if(hostile)poison();});}
        catch(const Boundary&){stopped=true;}
        const auto name=row.at("name").get<std::string>()+(hostile?" hostile":" ordinary");
        check(stopped==row.contains("fault"),name+" completion differs");
        auto actual=packet(state,memory.pc);
        if(!stopped) {
            // The driver names a completed case by its highest executed site;
            // the full-pool path returns through the lower shared RET 8.
            check(memory.pc==0x1e599u||memory.pc==0x1e5aeu,name+" did not finish at an original RET 8");
            actual["entry"]=row.at("expected").at("entry");
        }
        check(actual==row.at("expected"),name+" final ABI differs actual="+actual.dump()+" expected="+row.at("expected").dump());
        if(memory.packets!=row.at("calls")){
            const auto&expected=row.at("calls");unsigned first{};
            while(first<memory.packets.size()&&first<expected.size()&&memory.packets[first]==expected[first])++first;
            const auto a=first<memory.packets.size()?memory.packets[first]:J{};
            const auto b=first<expected.size()?expected[first]:J{};
            check(false,name+" pre-I/O differs at ordinal "+std::to_string(first)+" actual="+a.dump()+" expected="+b.dump()+
                " counts="+std::to_string(memory.packets.size())+"/"+std::to_string(expected.size()));
        }
        if(row.value("observer",false)){
            check(observed==row.at("writes"),name+" observer events differ");
            check(observed_states==row.at("observer_states"),name+" retired observer ABI differs");
            observer_packets+=observed.size();if(stopped)++observer_stops;
        }
        check(memory.architectural_reads()==row.at("reads"),name+" read sequence differs");
        check(memory.writes==row.at("writes"),name+" writes differ");
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+" bank differs");
        ++comparisons;packets+=memory.packets.size();
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"full_pre_io_packets",packets},
        {"retired_observer_packets",observer_packets},{"observer_throw_prefixes",observer_stops},{"production_bound",false}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
