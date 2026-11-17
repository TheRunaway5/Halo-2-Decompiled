#pragma once
#include <cstdint>
namespace halo2::native_shape_collection_detail {
enum class Field {shape,filter_info};
// Source collection rows own one child pointer and its filter word. Preserve
// the lazy row-base/key acquisitions; ordinary32-bit index arithmetic wraps.
template<class View>std::uint32_t element(View&view){
    const auto rows=view.rows();const auto key=view.key();
    return view.word(rows+key*8u+view.offset());
}
} // namespace halo2::native_shape_collection_detail
