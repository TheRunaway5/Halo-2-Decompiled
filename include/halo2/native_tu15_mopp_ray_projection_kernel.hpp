#pragma once
#include <array>
#include <cstdint>

namespace halo2::native_mopp_ray_projection_detail {
struct Direction {
    std::array<unsigned,3> axes;
    unsigned count;
    std::array<bool,3> negative;
};
// Authored MOPP plane directions. Order matters for native extended precision,
// NaN selection and the source's observable field-read boundaries.
inline constexpr std::array<Direction,10> directions{{
    {{2,1,0},2,{false,false,false}}, // Z + Y
    {{1,2,0},2,{false,true, false}}, // Y - Z
    {{2,0,0},2,{false,false,false}}, // Z + X
    {{0,2,0},2,{false,true, false}}, // X - Z
    {{1,0,0},2,{false,false,false}}, // Y + X
    {{0,1,0},2,{false,true, false}}, // X - Y
    {{2,1,0},3,{false,false,false}}, // Z + Y + X
    {{1,0,2},3,{false,false,true }}, // Y + X - Z
    {{0,1,2},3,{false,true, false}}, // X - Y + Z
    {{0,1,2},3,{false,true, true }}, // X - Y - Z
}};

// Read owns the actual field view and any semantic I/O progress. Arithmetic
// owns native extended values/status; no CPU register or instruction state is
// part of this numerical interface. Opcode admission belongs to the caller.
template<class Read,class Arithmetic>
void project(std::uint32_t opcode,Read&&read,Arithmetic&&arithmetic){
    const auto&direction=directions[opcode-0x13u];
    arithmetic.begin(read(direction.axes[0],0u));
    for(unsigned n=1;n<direction.count;++n){
        const auto value=read(direction.axes[n],n);
        if(direction.negative[n])arithmetic.subtract(value);
        else arithmetic.add(value);
    }
}
} // namespace halo2::native_mopp_ray_projection_detail
