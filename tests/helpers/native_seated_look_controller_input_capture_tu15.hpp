#pragma once
#include "native_look_controller_input_capture_tu15.hpp"
namespace halo2::test {
// Optional diagnostic selection only. It never changes player/seat/look DATA
// or a controller's admission. The base capture keeps its ordinary typed ABI.
class NativeSeatedLookControllerInputCaptureTu15 {
    NativeLookControllerInputCaptureTu15 capture_;
    std::filesystem::path directory_;bool require_seat_{};
    std::uint64_t skipped_unseated_{};
    static std::uint32_t word(const SourcePhysicsContactMemory&memory,
        std::uint32_t p,unsigned width=4){
        std::array<std::byte,4>b{};memory.read(p,std::span(b).first(width));
        std::uint32_t n{};for(unsigned k=0;k<width;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;
    }
public:
    explicit NativeSeatedLookControllerInputCaptureTu15(std::filesystem::path path,
        bool require_seat=std::getenv("HALO2_TU15_LOOK_CAPTURE_REQUIRE_SEAT")!=nullptr)
        :capture_(path),directory_(std::filesystem::weakly_canonical(path)),require_seat_(require_seat){}
    bool complete()const noexcept{return capture_.complete();}
    template<class Memory>void observe(const NativeTu15LookControllerInput&input,
        const Memory&memory){
        if(capture_.complete())return;
        if(!require_seat_){capture_.observe(input,memory);return;}
        const auto controls=word(memory,0x4f9b54u),record=controls+input.control_index*0x94u+0x14u,unit=word(memory,record);
        if(unit==0xffffffffu){capture_.observe(input,memory);return;}
        const auto objects=word(memory,0x4ecbd0u),rows=word(memory,objects+0x44u),body=word(memory,rows+(unit&0xffffu)*12u+8u),parent=word(memory,body+0x14u),seat=word(memory,body+0x1fcu,2);
        if(parent==0xffffffffu||seat==0xffffu){++skipped_unseated_;return;}
        capture_.observe(input,memory);
        if(capture_.complete()){
            nlohmann::json tuple{{"schema","tu15-seated-look-selection-v1"},{"actual_record_unit",unit},{"actual_body",body},{"actual_parent",parent},{"actual_seat_word",seat},{"skipped_unseated_inputs",skipped_unseated_},{"selection_only",true},{"source_memory_mutated",false},{"physics_result_supplied",false},{"native_controller_admission_added",false}};
            std::ofstream out(directory_/"actual-seated-look-tuple.json");out<<tuple.dump(2)<<'\n';
            if(!out)throw std::runtime_error("Cannot retain actual seated look selection metadata");
        }
    }
};
}
