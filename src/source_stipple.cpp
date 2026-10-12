#include "halo2/source_stipple.hpp"

namespace halo2 {
bool source_stipple_pixel(std::span<const std::uint32_t,32> rows,
    std::uint32_t x,std::uint32_t y,SourceStippleRasterPhase phase) {
    const auto column=(x+phase.column_origin)&31u;
    const auto bit=(column&24u)|(7u-(column&7u));
    const auto row=(phase.row_origin-y)&31u;
    return ((rows[row]>>bit)&1u)!=0;
}
}
