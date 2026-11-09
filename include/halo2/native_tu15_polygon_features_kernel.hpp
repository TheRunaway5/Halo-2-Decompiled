#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace halo2 {
enum class NativeTu15PolygonFeaturesPhase {count,begin_rows,feature,point,publish,advance,finish};
struct NativeTu15PolygonFeatures {
    std::uint32_t shape{},ids{},count{},index{},output{},offset{},feature{};
    std::array<std::byte,16>point{};
};
// Reverse feature expansion shared by the semantic shape API and its retained
// ABI owner. All rows are caller-backed; source/output overlap remains streaming.
template<class Access>
void native_tu15_expand_polygon_features(Access&memory,NativeTu15PolygonFeatures&d){
    using P=NativeTu15PolygonFeaturesPhase;
    d.count=memory.count(d);d.index=d.count-1u;
    memory.prepare(P::count,d);
    if(!(d.index&0x80000000u)){
        memory.prepare(P::begin_rows,d);
        do{
            d.feature=memory.feature(d,d.ids+d.index*2u);
            d.offset=(d.feature+2u)<<4u;
            d.point=memory.point(d,d.shape+d.offset);
            memory.publish(d,d.output,d.point);
            --d.index;d.output-=16u;memory.prepare(P::advance,d);
        }while(!(d.index&0x80000000u));
    }
    memory.prepare(P::finish,d);
}
} // namespace halo2
