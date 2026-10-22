#pragma once
#include <array>
#include <cstddef>
namespace halo2 {
// Full reduction allows vector comparison while retaining float equality:
// signed zeros compare equal and every NaN compares unequal.
template<std::size_t Rows>
bool shader_constants_equal(const std::array<std::array<float,4>,Rows> &a,
                            const std::array<std::array<float,4>,Rows> &b) {
    unsigned different=0;
    for(std::size_t row=0;row<Rows;++row)
        for(unsigned component=0;component<4;++component)
            different|=unsigned(a[row][component]!=b[row][component]);
    return different==0;
}
}
