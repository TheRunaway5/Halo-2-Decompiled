#pragma once
#include <cstdint>

namespace halo2 {
enum class NativeTu15SimplexPairedNext : std::uint32_t {
    segments=0x30ea8eu,degenerate=0x30edd0u,direction_count=0x30f268u
};

template<class View> NativeTu15SimplexPairedNext native_tu15_simplex_reduction_paired_kernel(View& view,bool first_two) {
    if(first_two)view.first_two_arguments();else view.second_two_arguments();
    view.call_reducer();return view.classify();
}
} // namespace halo2
