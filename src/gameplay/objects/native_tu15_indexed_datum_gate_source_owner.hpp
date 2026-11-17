#pragma once
#include "gameplay/objects/native_tu15_indexed_datum_gate_source_frame.hpp"
#include <cstdlib>

namespace halo2::native_indexed_datum_gate_owner_detail {
inline bool reference() {
    static const bool enabled=[] {
        const auto* value=std::getenv("HALO2_REFERENCE_INDEXED_DATUM_GATE");
        return value&&*value;
    }();
    return enabled;
}

template<bool Literal,SourcePhysicsContactImage Image,class State> bool bridge(State& state) {
    if(state.image!=Image||reference())return false;
    if constexpr(Image==SourcePhysicsContactImage::TitleUpdate15)
        native_indexed_datum_gate_detail::run_source_frame<Literal,native_indexed_datum_gate_detail::title_update_15>(state);
    else {
        static_assert(Image==SourcePhysicsContactImage::MapPack11&&!Literal);
        native_indexed_datum_gate_detail::run_source_frame<false,native_indexed_datum_gate_detail::map_pack_11>(state);
    }
    return true;
}
} // namespace halo2::native_indexed_datum_gate_owner_detail
