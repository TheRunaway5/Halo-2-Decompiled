#include "native_tu15_marker_owner_test_support.hpp"
#include "physics/runtime/native_tu15_codec_translation_interpolation_frame.hpp"

int main(int argc,char**argv)try{
    check(argc==2,"Supply unchanged translation interpolation inputs");std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    const auto original=native_raw_x87_detail::capture();struct Restore{native_raw_x87_detail::Image image;~Restore(){native_raw_x87_detail::restore(image);}}restore{original};unsigned comparisons{},packets{};
    for(const auto&row:fixture.at("cases"))for(bool noisy:{false,true})for(bool default_before:{false,true}){
        OwnerMemory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),bytes(bank.at("before")));
        memory.write_sites={0x29631du,0x29632eu,0x29633du,0x29634bu,0x29636cu,0x296375u,0x29637bu,0x29639cu,0x2963bdu};memory.noisy=noisy;if(row.contains("fault"))memory.fault=row.at("fault");
        SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observe;unsigned callbacks{},completed{};bool trace{};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};services.instruction=[&](auto pc){memory.pc=pc;++callbacks;};services.instruction_cursor={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        State state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp");abi.xmm=row.at("xmm");abi.eflags=row.at("eflags");abi.x87_status=row.at("before_fp")[1];abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};abi.x87_depth=row.at("live_x87").size();
        for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}
        if(row.contains("active_mmx")){SourcePhysicsMmxAlias opaque{};opaque.known_mask=row.at("active_mmx").at("known_mask");for(unsigned n=0;n<8;++n){const auto raw=bytes(row.at("active_mmx").at("physical")[n]);std::memcpy(opaque.physical[n].data(),raw.data(),10);}abi.active_mmx=opaque;}
        state.load(abi);memory.capture=[&]{return packet(state,memory.pc);};const auto host=native_raw_x87_detail::capture();
        if(row.value("phase",std::string{})=="after_write")observe=[&](const auto&){if(noisy)hostile();if(completed++==row.at("fault").at("ordinal"))throw Fault{};};
        bool stopped{};try{
            if(default_before)native_codec_translation_interpolation_detail::run_private(state);
            else native_codec_translation_interpolation_detail::run_private(state,[&](auto&,auto pc){memory.pc=pc;});
        }catch(const Fault&){stopped=true;}
        const auto name=row.at("name").get<std::string>();check(stopped==row.contains("phase")&&!callbacks,name+": translation interpolation completion/callback differs");
        check(packet(state,row.at("expected").at("entry"))==row.at("expected"),name+": translation interpolation full ABI differs");
        check(memory.reads==row.at("reads")&&memory.writes==row.at("writes"),name+": translation interpolation ordered IO differs");
        check(memory.packets==row.at("calls"),name+": translation interpolation pre-I/O packets differ");
        for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": translation interpolation bank differs");
        auto expected_host=host;const auto returned=state.save();expected_host.word(0,returned.floating_point->x87_control_word);expected_host.word(4,std::uint16_t((returned.x87_status&~0x3800u)|(host.word(4)&0x3800u)));expected_host.mxcsr=returned.floating_point->mxcsr;
        check(native_raw_x87_detail::equivalent(expected_host,native_raw_x87_detail::capture()),name+": translation interpolation host FP ownership differs");packets+=memory.packets.size();++comparisons;
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"complete_original_pre_io_packets",packets},{"default_and_explicit_before",true},{"native_instruction_callbacks",0},{"standalone_physical_fixture",true}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
