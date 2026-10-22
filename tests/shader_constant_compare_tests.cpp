#include "../src/rendering/renderer/shader_constant_compare.hpp"
#include <bit>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
int main() {
    std::mt19937 random(0x48414c4fu);
    using Bank=std::array<std::array<float,4>,16>;
    unsigned checks{};
    const auto check=[&](const Bank &a,const Bank &b) {
        if(halo2::shader_constants_equal(a,b)!=(a==b))
            throw std::runtime_error("shader constant equality differs from original float comparison");
        ++checks;
    };
    Bank a{},b{};check(a,b);
    constexpr std::array<std::uint32_t,8> edges{0,0x80000000u,0x7f800000u,0xff800000u,
        0x7fc00000u,0x7fa00001u,0x00000001u,0x7f7fffffu};
    for(auto x:edges)for(auto y:edges)for(unsigned at=0;at<64;++at) {
        a={};b={};a[at/4][at%4]=std::bit_cast<float>(x);b[at/4][at%4]=std::bit_cast<float>(y);check(a,b);
    }
    for(unsigned trial=0;trial<10000;++trial) {
        for(auto &row:a)for(auto &value:row)value=std::bit_cast<float>(std::uint32_t(random()));
        b=a;check(a,b);b[random()%16][random()%4]=std::bit_cast<float>(std::uint32_t(random()));check(a,b);
    }
    std::cout<<checks<<" shader equality comparisons passed\n";
}
