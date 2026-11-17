#include "gameplay/objects/native_tu15_map_world_property_query_frame.hpp"
#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include <fstream>
#include <iostream>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
namespace {
struct QueryMemory final:mopp_ray_frame_test::Memory {
    WorldState* state{};mutable J packets=J::array();
    void read(std::uint32_t address,std::span<std::byte>out)const override {
        packets.push_back(packet(*state,pc));mopp_ray_frame_test::Memory::read(address,out);
    }
    void write(std::uint32_t address,std::span<const std::byte>data)override {
        packets.push_back(packet(*state,pc));mopp_ray_frame_test::Memory::write(address,data);
    }
};
} // namespace

int main(int argc,char**argv)try {
    check(argc==2,"Supply unchanged-original MapWorld property query cases");
    std::ifstream input(argv[1]);const auto rows=J::parse(input);
    unsigned comparisons{},packets{};
    for(const auto&row:rows)for(bool hostile:{false,true}) {
        QueryMemory memory;
        for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        const SourcePhysicsWorldRuntimeServices services;
        const SourcePhysicsContactRecordObserver observer;
        WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp");abi.eflags=row.at("eflags");abi.xmm=row.at("xmm");
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
        abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}
        const physics_step_detail::FpScope scope{*abi.floating_point};state.load(abi);
        memory.state=&state;memory.hostile=hostile;
        if(row.contains("fault")){
            const auto ordinal=row.at("fault").at("ordinal").get<int>();
            if(row.at("fault").at("phase")=="before_read")memory.read_fault=ordinal;else memory.write_fault=ordinal;
        }
        bool stopped{};
        try{native_map_world_property_query_detail::run_private(state,[&](auto&,auto pc){memory.pc=pc;if(hostile)poison();});}
        catch(const Boundary&){stopped=true;}
        const auto name=row.at("name").get<std::string>()+(hostile?" hostile":" ordinary");
        check(stopped==row.contains("fault"),name+" completion differs");
        const auto final=packet(state,memory.pc);
        check(final==row.at("expected"),name+" final ABI differs actual="+final.dump()+" expected="+row.at("expected").dump());
        if(memory.packets!=row.at("calls")){
            const auto&expected=row.at("calls");std::size_t i{};
            while(i<memory.packets.size()&&i<expected.size()&&memory.packets[i]==expected[i])++i;
            const auto a=i<memory.packets.size()?memory.packets[i].dump():"end";
            const auto b=i<expected.size()?expected[i].dump():"end";
            check(false,name+" pre-I/O differs at "+std::to_string(i)+" actual="+a+" expected="+b);
        }
        check(memory.architectural_reads()==row.at("reads"),name+" ordered reads differ");
        check(memory.writes==row.at("writes"),name+" ordered writes differ");
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+" bank differs");
        ++comparisons;packets+=memory.packets.size();
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"full_pre_io_packets",packets},{"production_bound",false}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
