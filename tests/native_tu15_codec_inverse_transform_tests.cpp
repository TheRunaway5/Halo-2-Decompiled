#include "native_tu15_marker_owner_test_support.hpp"
#include "physics/runtime/native_tu15_codec_inverse_transform_frame.hpp"

int main(int argc,char** argv)try {
    check(argc==2,"Supply unchanged codec inverse transform inputs");
    std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    const auto original=native_raw_x87_detail::capture();
    struct Restore {native_raw_x87_detail::Image image;~Restore(){native_raw_x87_detail::restore(image);}} restore{original};
    unsigned comparisons{},packets{};
    for(const auto& row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();
        std::map<std::uint32_t,std::vector<std::byte>> seed;
        for(const auto& bank:row.at("banks"))seed.emplace(bank.at("address"),bytes(bank.at("before")));
        for(bool noisy:{false,true})for(bool use_default:{false,true}) {
            OwnerMemory memory;memory.banks=seed;memory.noisy=noisy;
            memory.write_sites={0x120748u,0x120749u,0x120758u,0x12075du,0x120763u,0x120769u,0x120779u,
                0x12081du,0x120840u,0x120862u,0x120894u,0x1208a2u,0x1208b0u,0x1208c6u,0x1208d4u,0x1208e2u,0x1208e7u};
            if(row.contains("fault"))memory.fault=row.at("fault");
            SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;
            unsigned numeric{},completed{};bool trace{};
            constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
            services.instruction=[&](std::uint32_t pc){memory.pc=pc;++numeric;};
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
            if(row.value("phase",std::string{})=="after_write")observer=[&](const auto&) {
                if(noisy)hostile();if(completed++==row.at("fault").at("ordinal"))throw Fault{};
            };
            bool interrupted{};
            try {
                namespace transform=halo2::native_codec_inverse_transform_detail;
                if(use_default)transform::run_private(state);
                else transform::run_private(state,[&](auto&,auto pc){
                    check(memory.pc==pc,"Codec inverse transform cursor differs");if(noisy)hostile();
                });
            } catch(const Fault&) {interrupted=true;}
            catch(const std::exception& error) {throw std::runtime_error(name+": codec inverse transform failed: "+error.what());}
            check(interrupted==row.contains("phase")&&!numeric,name+": codec inverse transform completion/callback differs");
            const auto actual=packet(state,row.at("expected").at("entry"));
            if(actual!=row.at("expected")) {
                std::cerr<<"expected "<<row.at("expected").dump()<<"\nactual "<<actual.dump()<<'\n';
                throw std::runtime_error(name+": codec inverse transform complete ABI differs");
            }
            if(memory.reads!=row.at("reads")||memory.writes!=row.at("writes")) {
                if(memory.reads!=row.at("reads"))
                    std::cerr<<"reads expected "<<row.at("reads").dump()<<"\nactual "<<memory.reads.dump()<<'\n';
                if(memory.writes!=row.at("writes"))
                    std::cerr<<"writes expected "<<row.at("writes").dump()<<"\nactual "<<memory.writes.dump()<<'\n';
                throw std::runtime_error(name+": codec inverse transform ordered I/O differs");
            }
            if(memory.packets!=row.at("calls")) {
                for(std::size_t n=0;n<std::min(memory.packets.size(),row.at("calls").size());++n)
                    if(memory.packets[n]!=row.at("calls")[n]) {
                        std::cerr<<"packet "<<n<<" expected "<<row.at("calls")[n].dump()<<"\nactual "<<memory.packets[n].dump()<<'\n';break;
                    }
                throw std::runtime_error(name+": codec inverse transform complete pre-I/O packets differ");
            }
            for(const auto& bank:row.at("banks"))
                check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": codec inverse transform bank bytes differ");
            auto expected_host=host;const auto returned=state.save();
            expected_host.word(0,returned.floating_point->x87_control_word);
            expected_host.word(4,std::uint16_t((returned.x87_status&~0x3800u)|(host.word(4)&0x3800u)));
            expected_host.mxcsr=returned.floating_point->mxcsr;
            check(native_raw_x87_detail::equivalent(expected_host,native_raw_x87_detail::capture()),name+": codec inverse transform incidental host FP differs");
            ++comparisons;packets+=memory.packets.size();
        }
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"complete_original_pre_io_packets",packets},
        {"native_numeric_callbacks",0},{"standalone_physical_fixture",true}}.dump()<<'\n';return 0;
}catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
