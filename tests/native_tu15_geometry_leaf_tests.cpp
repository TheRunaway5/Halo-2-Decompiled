#include "native_tu15_marker_owner_test_support.hpp"
#include "physics/runtime/native_tu15_geometry_leaf_frame.hpp"
namespace {
struct GeometryMemory:OwnerMemory {
    // Ordinary atomic backing for the physical MOVAPS envelope. The default
    // abstract four-lane provider is a separate source-transport domain.
    void read_vector(std::uint32_t address,std::span<std::byte,16>out)const override{read(address,out);}
};
}

int main(int argc,char**argv)try{
    check(argc==2,"Supply unchanged original geometry-leaf inputs");std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    const auto original=native_raw_x87_detail::capture();
    struct Restore{native_raw_x87_detail::Image image;~Restore(){native_raw_x87_detail::restore(image);}}restore{original};
    unsigned comparisons{},packets{};
    for(const auto&row:fixture.at("cases"))for(bool noisy:{false,true}){
        GeometryMemory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        memory.write_sites={0x2e47b0u,0x2e4690u,0x2e4816u,0x2e46d5u};memory.noisy=noisy;
        if(row.contains("fault"))memory.fault=row.at("fault");
        SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observe;unsigned callbacks{},completed{};bool trace{};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};services.instruction=[&](auto pc){memory.pc=pc;++callbacks;};
        services.instruction_cursor={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        State state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi abi{};
        abi.gp=row.at("gp");abi.xmm=row.at("xmm");abi.eflags=row.at("eflags");abi.x87_status=row.at("before_fp")[1];
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};abi.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}
        state.load(abi);memory.capture=[&]{return packet(state,memory.pc);};const auto host=native_raw_x87_detail::capture();
        if(row.value("phase",std::string{})=="after_write")observe=[&](const auto&){if(noisy)hostile();if(completed++==row.at("fault").at("ordinal"))throw Fault{};};
        bool stopped{},alignment{};std::uint32_t saved_flags{};
        try{native_geometry_leaf_detail::run_private(state,row.at("entry")==0x2e47b0u?native_geometry_leaf_detail::Kind::direction:native_geometry_leaf_detail::Kind::position,
            [&](auto&,auto pc){memory.pc=pc;if(row.value("alignment_fault",false)&&pc==row.at("expected").at("entry"))memory.packets.push_back(memory.capture());});}
        catch(const Fault&){stopped=true;}
        catch(const native_mopp_ray_caller_detail::AlignedLoadFault&fault){stopped=alignment=true;saved_flags=fault.saved_eflags;check(fault.instruction==row.at("expected").at("entry"),"Geometry leaf alignment PC differs");}
        const auto name=row.at("name").get<std::string>();
        check(stopped==row.contains("phase")&&alignment==row.value("alignment_fault",false),name+": geometry leaf stop differs");
        auto returned=packet(state,row.at("expected").at("entry"));if(alignment)returned["eflags"]=saved_flags;
        check(returned==row.at("expected"),name+": geometry leaf full ABI differs actual="+returned.dump()+" expected="+row.at("expected").dump());
        check(memory.reads==row.at("reads")&&memory.writes==row.at("writes"),name+": geometry leaf ordered IO differs reads="+memory.reads.dump()+" expected="+row.at("reads").dump()+" writes="+memory.writes.dump()+" expected="+row.at("writes").dump());
        check(memory.packets.size()==row.at("calls").size(),name+": geometry leaf packet count differs actual="+std::to_string(memory.packets.size())+" expected="+std::to_string(row.at("calls").size()));
        for(unsigned n=0;n<memory.packets.size();++n)check(memory.packets[n]==row.at("calls")[n],name+": geometry leaf pre-IO packet"+std::to_string(n)+" differs actual="+memory.packets[n].dump()+" expected="+row.at("calls")[n].dump());
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": geometry leaf bank differs");
        auto expected_host=host;const auto retained=state.save();expected_host.word(0,retained.floating_point->x87_control_word);
        expected_host.word(4,std::uint16_t((retained.x87_status&~0x3800u)|(host.word(4)&0x3800u)));expected_host.mxcsr=retained.floating_point->mxcsr;
        check(native_raw_x87_detail::equivalent(expected_host,native_raw_x87_detail::capture()),name+": geometry leaf host FP ownership differs");
        check(!callbacks,name+": geometry leaf invoked instruction callback");++comparisons;packets+=memory.packets.size();
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"complete_pre_io_packets",packets},{"native_callbacks",0},
        {"private_unbound",true},{"before_only_non_io_stop_claim",false}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
