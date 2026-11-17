#include "gameplay/animation/native_tu15_node_progress_query_source_frame.hpp"
#include "native_tu15_mopp_ray_frame_test_memory.hpp"
#include <fstream>
#include <iostream>

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
namespace reference {
#include "node_progress_query_reference.inc"
}
namespace {
bool parent_site(std::uint32_t pc) {return pc>=0x1cf0e0u&&pc<=0x1cf148u;}

struct Provider final:Memory {
    WorldState* state{};
    mutable J packets=J::array();
    void read(std::uint32_t p,std::span<std::byte>out)const override {
        if(parent_site(pc))packets.push_back(packet(*state,pc));
        Memory::read(p,out);
    }
    void write(std::uint32_t p,std::span<const std::byte>value)override {
        if(parent_site(pc))packets.push_back(packet(*state,pc));
        Memory::write(p,value);
    }
};

J parent_events(const J&events) {
    J result=J::array();
    for(const auto&event:events)if(parent_site(event.at("instruction")))result.push_back(event);
    return result;
}
}

int main(int argc,char**argv)try {
    check(argc==2,"Supply original hardware node-progress packets");
    std::ifstream input(argv[1]);const auto rows=J::parse(input);
    std::size_t packets{},children{},prefixes{};
    for(const auto&row:rows){
        Provider memory;
        for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        SourcePhysicsWorldRuntimeAbi abi{};
        abi.gp=row.at("gp");abi.eflags=row.at("eflags");abi.xmm=row.at("xmm");
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0].get<std::uint16_t>(),row.at("before_fp")[2]};
        abi.x87_status=row.at("before_fp")[1];abi.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<abi.x87_depth;++n){
            const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);
        }
        SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](std::uint32_t pc){
            memory.pc=pc;
            if(pc==0x1e3150u){memory.packets.push_back(packet(*memory.state,pc));++children;}
        };
        const SourcePhysicsContactRecordObserver observer;
        WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};memory.state=&state;
        const physics_step_detail::FpScope scope{*abi.floating_point};state.load(abi);
        if(row.contains("fault")){
            const auto ordinal=row.at("fault").at("ordinal").get<int>();
            if(row.at("fault").at("phase")=="before_read")memory.read_fault=ordinal;
            else memory.write_fault=ordinal;
        }
        bool stopped{};
        try{native_node_progress_query_detail::run_physical_frame(state,reference::raw_child);}
        catch(const Boundary&){stopped=true;}
        const auto name=row.at("name").get<std::string>();
        check(stopped==row.contains("fault"),name+" completion differs");prefixes+=stopped;
        const auto final=packet(state,memory.pc);
        check(final==row.at("expected"),name+" final packet differs: "+final.dump()+" vs "+row.at("expected").dump());
        if(memory.packets!=row.at("calls")){
            const auto&expected=row.at("calls");std::size_t index{};
            while(index<memory.packets.size()&&index<expected.size()&&memory.packets[index]==expected[index])++index;
            const auto a=index<memory.packets.size()?memory.packets[index].dump():"end";
            const auto b=index<expected.size()?expected[index].dump():"end";
            check(false,name+" packet "+std::to_string(index)+" differs: "+a+" vs "+b);
        }
        check(parent_events(memory.architectural_reads())==parent_events(row.at("reads")),name+" parent reads differ");
        check(parent_events(memory.writes)==parent_events(row.at("writes")),name+" parent writes differ");
        for(const auto&bank:row.at("banks"))
            check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+" final bank differs");
        packets+=memory.packets.size();
    }
    std::cout<<J{{"passed",true},{"comparisons",rows.size()},{"full_packets",packets},
        {"real_child_entries",children},{"stopped_prefixes",prefixes},{"production_bound",false}}.dump()<<'\n';
    return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
