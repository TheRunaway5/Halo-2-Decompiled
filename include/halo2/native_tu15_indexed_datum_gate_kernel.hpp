#pragma once
#include <cstdint>

namespace halo2::native_indexed_datum_gate_detail {
struct Globals {std::uint32_t enabled,index,selection,header;};
inline constexpr Globals title_update_15{0x4c6709u,0x4c670cu,0x474c24u,0x4ecc18u};
inline constexpr Globals map_pack_11{0x4c1409u,0x4c140cu,0x46fb64u,0x4e7918u};

template<class View> bool query(View& view) {
    if(!view.valid_datum()||!view.enabled())return view.finish(false);
    view.save_index_register();
    if(view.has_index()&&view.has_selection()&&view.has_rows()&&view.matches_row())view.accept();
    return view.finish(true);
}
} // namespace halo2::native_indexed_datum_gate_detail
