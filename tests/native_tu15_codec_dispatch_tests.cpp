#include "native_tu15_marker_owner_test_support.hpp"
#include "physics/runtime/native_tu15_codec_dispatch_frame.hpp"

namespace {
bool tail_site(std::uint32_t pc) {
    namespace selector=halo2::native_codec_dispatch_detail;
    if(std::ranges::find(selector::format_jumps,pc)!=selector::format_jumps.end())return true;
    for(auto entry:selector::field_entries)
        if(pc>=entry&&std::ranges::find(selector::field_jumps,pc-entry)!=selector::field_jumps.end())return true;
    return false;
}
}

int main(int argc,char** argv)try {
    check(argc==2,"Supply original codec dispatch fixture");
    std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    const auto original=native_raw_x87_detail::capture();
    struct Restore {native_raw_x87_detail::Image image;~Restore(){native_raw_x87_detail::restore(image);}} restore{original};
    unsigned comparisons{},packets{},tail_packets{};
    for(const auto& row:fixture.at("cases")) {
        std::map<std::uint32_t,std::vector<std::byte>> seed;
        for(const auto& bank:row.at("banks"))seed.emplace(bank.at("address"),bytes(bank.at("before")));
        const auto stop_at=row.value("jump_fault",0u);
        const auto name=row.at("name").get<std::string>();
        for(bool noisy:{false,true})for(bool use_default:{false,true}) {
            if(use_default&&stop_at)continue;
            Memory memory;memory.banks=seed;memory.noisy=noisy;
            if(row.contains("fault"))memory.fault=row.at("fault");
            SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;unsigned numeric{};bool trace{};
            constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
            services.instruction=[&](auto pc){memory.pc=pc;++numeric;};
            services.instruction_cursor={&memory.pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
            State state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
            SourcePhysicsWorldRuntimeAbi abi{};
            abi.gp=row.at("gp");abi.xmm=row.at("xmm");abi.eflags=row.at("eflags");abi.x87_status=row.at("before_fp")[1];
            abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
            abi.x87_depth=row.at("live_x87").size();
            for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&abi.x87[n],raw.data(),10);}
            if(row.contains("active_mmx")) {
                SourcePhysicsMmxAlias opaque{};opaque.known_mask=row.at("active_mmx").at("known_mask");
                for(unsigned n=0;n<8;++n){const auto raw=bytes(row.at("active_mmx").at("physical")[n]);std::memcpy(opaque.physical[n].data(),raw.data(),10);}
                abi.active_mmx=opaque;
            }
            state.load(abi);memory.capture=[&]{return packet(state,memory.pc);};
            const auto host=native_raw_x87_detail::capture();
            const auto before=[&](auto&,auto pc) {
                check(memory.pc==pc,"Codec dispatch cursor did not publish before Before");
                if(pc==stop_at) {if(noisy)hostile();throw Fault{};}
                if(tail_site(pc)){memory.packets.push_back(memory.capture());++tail_packets;}
                if(noisy)hostile();
            };
            const auto run=[&](std::uint32_t entry) {
                namespace selector=halo2::native_codec_dispatch_detail;
                return use_default?selector::run_private(state,entry):selector::run_private(state,entry,before);
            };
            bool interrupted{};halo2::native_codec_dispatch_detail::Tail outcome{};
            try {
                outcome=run(row.at("entry"));
                if(row.contains("compose")) {
                    check(outcome.target==row.at("compose"),"Codec dispatch first genuine tail target differs");
                    outcome=run(outcome.target);
                }
            } catch(const Fault&) {interrupted=true;}
            catch(const std::exception& error) {throw std::runtime_error(name+": codec dispatch execution failed: "+error.what());}
            check(interrupted==bool(stop_at||row.contains("fault"))&&!numeric,name+": codec dispatch completion/callback differs");
            if(!interrupted)
                check(outcome.instruction==row.at("last_instruction")&&outcome.target==row.at("target"),name+": codec dispatch genuine transfer differs");
            check(packet(state,row.at("expected").at("entry"))==row.at("expected"),name+": codec dispatch full ABI differs");
            check(memory.reads==row.at("reads"),name+": codec dispatch ordered reads differ");
            check(memory.packets==row.at(use_default?"read_packets":"calls"),name+": codec dispatch complete packets differ");
            for(const auto& bank:row.at("banks"))
                check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": codec dispatch bank bytes differ");
            auto expected_host=host;const auto returned=state.save();
            expected_host.word(0,returned.floating_point->x87_control_word);
            expected_host.word(4,std::uint16_t((returned.x87_status&~0x3800u)|(host.word(4)&0x3800u)));
            expected_host.mxcsr=returned.floating_point->mxcsr;
            check(native_raw_x87_detail::equivalent(expected_host,native_raw_x87_detail::capture()),name+": codec dispatch incidental host FP differs");
            packets+=memory.packets.size();++comparisons;
        }
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"complete_original_packets",packets},
        {"pre_tail_packets",tail_packets},{"descendant_outcomes_supplied",false},{"native_numeric_callbacks",0}}.dump()<<'\n';return 0;
}catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
