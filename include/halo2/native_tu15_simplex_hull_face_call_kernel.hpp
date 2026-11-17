#pragma once
#include <cstdint>

namespace halo2 {
enum class NativeTu15SimplexHullFaceResume : std::uint32_t { plane=0x30e8b5u,first_triangle=0x30e504u,second_triangle=0x30e3a7u };

template<class View> NativeTu15SimplexHullFaceResume native_tu15_simplex_hull_face_call_kernel(View& view,bool first) {
    using Resume=NativeTu15SimplexHullFaceResume;
    if(!view.select_face(first))return Resume::plane;
    if(first){view.promote_first_face();return Resume::first_triangle;}
    view.promote_second_face();return Resume::second_triangle;
}
} // namespace halo2
