#pragma once
// Native TU1.5 code class, entered at DF820 (physics live support unit): the
// class of a code, by the original's compare tree.
//
//   code_class::of   DF820(ECX = code) -> AL
//
//   0  0400000C, 0900000E, 0A00000F
//   1  0800001E, 09000015, 09000016, 0A000014, 0A000017, 0D000042
//   2  any other code
//
// The tree compares signed and subtracts (above 0A00000F) as the original
// does; the frame mirrors each step's registers and flags. This is the game
// logic only: there is no memory access.
#include <cstdint>

namespace halo2::native_code_class {
inline constexpr std::uint32_t above_codes[]{0x0a000014u,0x0a000017u,0x0d000042u};   // reached by SUB
inline constexpr std::uint32_t first_low=0x0800001eu,middle_low=0x09000015u,middle_high=0x09000016u;

namespace code_class {
// The step that decided, for the frame's flags and ECX.
enum class Step {top_zero,top_middle,middle_low,middle_high,middle_zero,low_zero,low_one,low_other,above};
struct Result {std::uint32_t value;Step step;};
// Frame: compared(code, constant) CMP ECX,constant; subtracted(constant) SUB ECX,constant.
template<class Frame> Result of(Frame &frame,std::uint32_t code) {
    const auto signed_code=std::int32_t(code);
    frame.compared(code,0x0a00000fu);
    if(signed_code>0x0a00000f) {
        // Above the top: the remaining codes by successive subtractions.
        std::uint32_t rest=code,previous=0;
        for(const auto above:above_codes) {
            rest=frame.subtracted(rest,above-previous);previous=above;
            if(rest==0)return {1u,Step::above};
        }
        return {2u,Step::above};
    }
    if(signed_code==0x0a00000f)return {0u,Step::top_zero};
    frame.compared(code,0x0900000eu);
    if(signed_code>0x0900000e) {
        frame.compared(code,middle_low);
        if(signed_code<std::int32_t(middle_low))return {2u,Step::middle_low};
        frame.compared(code,middle_high);
        return {signed_code<=std::int32_t(middle_high)?1u:2u,Step::middle_high};
    }
    if(signed_code==0x0900000e)return {0u,Step::middle_zero};
    frame.compared(code,0x0400000cu);
    if(signed_code==0x0400000c)return {0u,Step::low_zero};
    frame.compared(code,first_low);
    return {code==first_low?1u:2u,code==first_low?Step::low_one:Step::low_other};
}
} // namespace code_class
} // namespace halo2::native_code_class
