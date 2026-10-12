#pragma once
#include "halo2/physics_vehicle_runtime.hpp"
#include "physics_world_runtime_internal.hpp"

namespace halo2::physics_vehicle_runtime_detail {
using namespace physics_world_runtime_detail;
inline std::uint64_t read_double_word(const M &m,std::uint32_t p) {
    std::array<std::byte,8> bytes{};m.read(p,bytes);std::uint64_t result{};
    for(unsigned n=0;n<8;++n)result|=std::to_integer<std::uint64_t>(bytes[n])<<(n*8u);
    return result;
}
inline long double extended_double(std::uint64_t value) {
    return static_cast<long double>(std::bit_cast<double>(value));
}
inline std::uint64_t double_word(long double value) {
    volatile double stored=static_cast<double>(value);
    return std::bit_cast<std::uint64_t>(double(stored));
}
struct VehicleState : WorldState {
    using WorldState::WorldState;
    void arctangent() {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        const auto x=fp.st(0),y=fp.st(1);long double result;
        asm volatile("fldt %2\n\tfldt %1\n\tfpatan\n\tfstpt %0"
            :"=m"(result):"m"(x),"m"(y):"st","st(1)");
        fp.st(1)=result;fp.pop();
#else
        throw std::runtime_error("Required original vehicle FPATAN backend unavailable");
#endif
    }
};
} // namespace halo2::physics_vehicle_runtime_detail
