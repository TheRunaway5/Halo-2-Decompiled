#include "native_tu15_marker_owner_test_support.hpp"
#include "physics/runtime/native_tu15_contact_predicate_frame.hpp"

namespace {
struct PredicateMemory : OwnerMemory {
    void read_vector(std::uint32_t address,std::span<std::byte,16> out) const override { read(address,out); }
};
}

int main(int argc,char** argv) try {
    check(argc==2,"Supply unchanged original contact-predicate inputs");
    std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    struct Restore {native_raw_x87_detail::Image image;~Restore(){native_raw_x87_detail::restore(image);}} restore{native_raw_x87_detail::capture()};
    unsigned comparisons{},packets{},alignment_faults{},provider_stops{};
    for(const auto& row:fixture.at("cases")) for(const bool noisy:{false,true}) {
        PredicateMemory memory;
        for(const auto& bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        memory.write_sites={0x303800u,0x30380fu,0x30384fu,0x30388au,0x30389fu,0x3038acu};
        if(row.contains("fault"))memory.fault=row.at("fault");
        memory.noisy=noisy;
        SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;
        unsigned callbacks{},completed{};bool traced{};
        constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};
        services.instruction=[&](auto pc){memory.pc=pc;++callbacks;};
        services.instruction_cursor={&memory.pc,&traced,nullptr,service_sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        State state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp");abi.xmm=row.at("xmm");abi.eflags=row.at("eflags");
        abi.x87_status=row.at("before_fp")[1];abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
        abi.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}
        state.load(abi);memory.capture=[&]{return packet(state,memory.pc);};
        const auto host=native_raw_x87_detail::capture();
        if(row.value("phase",std::string{})=="after_write")observer=[&](const auto&){if(noisy)hostile();if(completed++==row.at("fault").at("ordinal"))throw Fault{};};
        bool stopped{},alignment{};std::uint32_t saved_flags{};
        try {
            auto before=[&](auto&,auto pc){memory.pc=pc;if(row.value("alignment_fault",false)&&pc==row.at("expected").at("entry"))memory.packets.push_back(memory.capture());};
            native_contact_predicate_detail::run_private(state,before);
        } catch(const Fault&) {stopped=true;++provider_stops;}
          catch(const native_mopp_ray_caller_detail::AlignedLoadFault& error) {
            stopped=alignment=true;saved_flags=error.saved_eflags;++alignment_faults;
            check(error.instruction==row.at("expected").at("entry"),"Predicate alignment PC differs");
        }
        const auto name=row.at("name").get<std::string>();
        check(stopped==row.contains("phase") && alignment==row.value("alignment_fault",false),name+": predicate stop differs");
        auto returned=packet(state,row.at("expected").at("entry"));if(alignment)returned["eflags"]=saved_flags;
        check(returned==row.at("expected"),name+": predicate full ABI differs actual="+returned.dump()+" expected="+row.at("expected").dump());
        check(memory.reads==row.at("reads")&&memory.writes==row.at("writes"),name+": predicate ordered IO differs reads="+memory.reads.dump()+" expected="+row.at("reads").dump()+" writes="+memory.writes.dump()+" expected="+row.at("writes").dump());
        check(memory.packets.size()==row.at("calls").size(),name+": predicate packet count differs actual="+std::to_string(memory.packets.size())+" expected="+std::to_string(row.at("calls").size()));
        for(unsigned n=0;n<memory.packets.size();++n)check(memory.packets[n]==row.at("calls")[n],name+": predicate pre-IO packet "+std::to_string(n)+" differs actual="+memory.packets[n].dump()+" expected="+row.at("calls")[n].dump());
        for(const auto& bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": predicate committed bank differs");
        auto expected_host=host;const auto retained=state.save();
        expected_host.word(0,retained.floating_point->x87_control_word);
        expected_host.word(4,std::uint16_t((retained.x87_status&~0x3800u)|(host.word(4)&0x3800u)));expected_host.mxcsr=retained.floating_point->mxcsr;
        check(native_raw_x87_detail::equivalent(expected_host,native_raw_x87_detail::capture()),name+": predicate host FP ownership differs");
        check(!callbacks,name+": predicate invoked retained callback");++comparisons;packets+=memory.packets.size();
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"complete_pre_io_packets",packets},
        {"provider_stops",provider_stops},{"alignment_faults",alignment_faults},{"private_unbound",true}}.dump()<<'\n';
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
