#include "native_tu15_marker_owner_test_support.hpp"
#include "physics/vehicles/native_tu15_vehicle_effect_ratio_frame.hpp"

int main(int argc,char** argv) try {
    check(argc==2,"Supply the unchanged original vehicle effect ratio cases");
    std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    struct Restore {native_raw_x87_detail::Image image;~Restore(){native_raw_x87_detail::restore(image);}} restore{native_raw_x87_detail::capture()};
    unsigned comparisons{},site_packets{},operand_packets{},stops{};
    for(const auto& row:fixture.at("cases")) for(const bool noisy:{false,true}) {
        Memory memory;memory.noisy=noisy;
        for(const auto& bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        if(row.contains("fault"))memory.fault=row.at("fault");
        SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;
        State state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp");abi.xmm=row.at("xmm");abi.eflags=row.at("eflags");
        abi.x87_status=row.at("before_fp")[1];abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
        abi.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}
        asm volatile("fninit;fld1;fldz":::"memory","st","st(1)");
        state.load(abi);memory.capture=[&]{return packet(state,memory.pc);};
        const auto host=native_raw_x87_detail::capture();
        J sites=J::array();bool stopped{};
        try {
            native_vehicle_effect_ratio_detail::run_private(state,[&](State&,std::uint32_t pc){
                memory.pc=pc;sites.push_back(memory.capture());if(noisy)hostile();
            });
        } catch(const Fault&) {stopped=true;++stops;}
        const auto name=row.at("name").get<std::string>();
        check(stopped==row.contains("fault"),name+": effect ratio stop differs");
        const auto returned=packet(state,memory.pc);
        check(returned==row.at("expected"),name+": effect ratio full ABI differs actual="+returned.dump()+" expected="+row.at("expected").dump());
        check(memory.reads==row.at("reads"),name+": effect ratio ordered reads differ actual="+memory.reads.dump()+" expected="+row.at("reads").dump());
        check(memory.packets.size()==row.at("operands").size(),name+": effect ratio operand count differs");
        for(unsigned n=0;n<memory.packets.size();++n)
            check(memory.packets[n]==row.at("operands")[n],name+": effect ratio operand packet differs at "+std::to_string(n)+" actual="+memory.packets[n].dump()+" expected="+row.at("operands")[n].dump());
        check(sites.size()==row.at("sites").size(),name+": effect ratio site count differs");
        for(unsigned n=0;n<sites.size();++n)
            check(sites[n]==row.at("sites")[n],name+": effect ratio site packet differs at "+std::to_string(n)+" actual="+sites[n].dump()+" expected="+row.at("sites")[n].dump());
        for(const auto& bank:row.at("banks"))
            check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": effect ratio bank differs");
        auto expected_host=host;const auto retained=state.save();
        expected_host.word(0,retained.floating_point->x87_control_word);
        expected_host.word(4,std::uint16_t((retained.x87_status&~0x3800u)|(host.word(4)&0x3800u)));
        expected_host.mxcsr=retained.floating_point->mxcsr;
        check(native_raw_x87_detail::equivalent(expected_host,native_raw_x87_detail::capture()),name+": effect ratio host FP differs");
        ++comparisons;site_packets+=sites.size();operand_packets+=memory.packets.size();
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"complete_site_packets",site_packets},
        {"complete_operand_packets",operand_packets},{"provider_stops",stops},{"private_unbound",true}}.dump()<<'\n';
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
