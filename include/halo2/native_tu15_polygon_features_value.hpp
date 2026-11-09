#pragma once
#include "halo2/native_tu15_polygon_features_kernel.hpp"

namespace halo2::native_polygon_features_detail {
// Captured public DATA formals. Memory retains its own vector read/store and
// observer protocol; this view owns no hidden caller frame or retained ABI.
template<class Memory>struct ValueAccess {
    Memory&memory;std::uint32_t shape,ids,requested_count,output;
    std::uint32_t count(const NativeTu15PolygonFeatures&)const{return requested_count;}
    void prepare(NativeTu15PolygonFeaturesPhase phase,NativeTu15PolygonFeatures&d){
        if(phase==NativeTu15PolygonFeaturesPhase::begin_rows){d.shape=shape;d.ids=ids;d.output=output+d.index*16u;}
    }
    std::uint32_t feature(const NativeTu15PolygonFeatures&,std::uint32_t at){return memory.word(at,2u);}
    std::array<std::byte,16>point(const NativeTu15PolygonFeatures&,std::uint32_t at){return memory.vector_bytes(at);}
    void publish(const NativeTu15PolygonFeatures&,std::uint32_t at,const std::array<std::byte,16>&value){memory.vector_bytes(0x1f66beu,at,value);}
};
} // namespace halo2::native_polygon_features_detail
