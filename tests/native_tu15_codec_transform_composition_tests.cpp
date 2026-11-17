#include "native_tu15_marker_owner_test_support.hpp"
#include "physics/runtime/native_tu15_codec_transform_composition_frame.hpp"

namespace {
struct CompositionMemory:OwnerMemory {
    mutable bool after_repeat_read{};
    static bool repeat(std::uint32_t pc){return pc==0x12058eu||pc==0x1205a3u;}
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        if(repeat(pc)&&after_repeat_read) {
            // A store observer's optional preimage is not a MOVSD operand.
            std::memcpy(out.data(),at(address,out.size()),out.size());
            if(noisy)hostile();return;
        }
        OwnerMemory::read(address,out);
        if(repeat(pc))after_repeat_read=true;
    }
    void write(std::uint32_t address,std::span<const std::byte> in)override {
        OwnerMemory::write(address,in);
        // Single-step has one pre-instruction packet for both MOVSD operands.
        if(repeat(pc)){packets.erase(packets.size()-1);after_repeat_read=false;}
    }
};
}

int main(int argc,char** argv)try {
    check(argc==2,"Supply the original codec transform composition fixture");
    std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    const auto host=native_raw_x87_detail::capture();
    struct Restore {native_raw_x87_detail::Image image;~Restore(){native_raw_x87_detail::restore(image);}} restore{host};
    unsigned comparisons{},packets{},children{};
    for(const auto& row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();
        std::map<std::uint32_t,std::vector<std::byte>> seed;
        for(const auto& bank:row.at("banks"))seed.emplace(bank.at("address"),bytes(bank.at("before")));
        for(bool noisy:{false,true})for(bool use_default:{false,true}) {
            CompositionMemory memory;memory.banks=seed;memory.noisy=noisy;
            for(auto pc:fixture.at("write_sites"))memory.write_sites.insert(pc.get<std::uint32_t>());
            if(row.contains("fault"))memory.fault=row.at("fault");
            SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;
            unsigned numeric{},completed{},returned_product{};bool trace{};
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
            state.load(abi);memory.capture=[&]{if(memory.pc==0x1205b2u)++returned_product;return packet(state,memory.pc);};
            const auto initial=native_raw_x87_detail::capture();
            if(row.value("phase",std::string{})=="after_write")observer=[&](const auto&) {
                if(noisy)hostile();if(completed++==row.at("fault").at("ordinal"))throw Fault{};
            };
            bool interrupted{};
            try {
                namespace compose=halo2::native_codec_transform_composition_detail;
                if(use_default)compose::run_private(state);
                else compose::run_private(state,[&](auto&,auto pc){
                    check(memory.pc==pc,"Composition cursor differs");if(noisy)hostile();
                });
            } catch(const Fault&) {interrupted=true;}
            check(interrupted==row.contains("phase")&&!numeric,name+": composition completion/callback differs");
            const auto actual=packet(state,row.at("expected").at("entry"));
            if(actual!=row.at("expected")) {
                std::cerr<<"expected "<<row.at("expected").dump()<<"\nactual "<<actual.dump()<<'\n';
                throw std::runtime_error(name+": composition complete ABI differs");
            }
            check(memory.reads==row.at("reads")&&memory.writes==row.at("writes"),name+": composition ordered I/O differs");
            if(memory.packets!=row.at("calls")) {
                for(std::size_t n=0;n<std::min(memory.packets.size(),row.at("calls").size());++n)
                    if(memory.packets[n]!=row.at("calls")[n]) {
                        std::cerr<<"packet "<<n<<" expected "<<row.at("calls")[n].dump()<<"\nactual "<<memory.packets[n].dump()<<'\n';break;
                    }
                throw std::runtime_error(name+": composition pre-I/O packets differ");
            }
            for(const auto& bank:row.at("banks"))
                check(memory.banks.at(bank.at("address"))==bytes(bank.at("after")),name+": composition bank bytes differ");
            auto expected_host=initial;const auto returned=state.save();
            expected_host.word(0,returned.floating_point->x87_control_word);
            expected_host.word(4,std::uint16_t((returned.x87_status&~0x3800u)|(initial.word(4)&0x3800u)));
            expected_host.mxcsr=returned.floating_point->mxcsr;
            check(native_raw_x87_detail::equivalent(expected_host,native_raw_x87_detail::capture()),name+": composition incidental host FP differs");
            ++comparisons;packets+=memory.packets.size();children+=returned_product;
        }
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"complete_original_pre_io_packets",packets},
        {"observed_child_returns",children},{"native_numeric_callbacks",0}}.dump()<<'\n';return 0;
}catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
