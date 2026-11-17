#pragma once
#include <cstdint>

namespace halo2 {
enum class NativeTu15SolverHullClearTable { facets,edges };
struct NativeTu15SolverHullClearLayout {
    std::uint32_t base,count,field,stride,store_pc;
    unsigned width;
};
inline constexpr auto native_tu15_solver_hull_clear_layout(NativeTu15SolverHullClearTable table){
    return table==NativeTu15SolverHullClearTable::facets
        ?NativeTu15SolverHullClearLayout{0x1b0u,0x1b4u,0x44u,0x50u,0x314896u,1u}
        :NativeTu15SolverHullClearLayout{0x2100u,0x2104u,0x10u,0x20u,0x3148c6u,4u};
}

// Each completed store may alias its table's base or count. The view
// reacquires both at the original boundaries instead of caching a range.
template<class View>void native_tu15_solver_hull_clear_kernel(View&view){
    using Table=NativeTu15SolverHullClearTable;
    const auto clear=[&](Table table){
        if(!view.start(table))return;
        do{view.clear(table);}while(view.next(table));
    };
    view.begin();clear(Table::facets);clear(Table::edges);view.finish();
}
} // namespace halo2
