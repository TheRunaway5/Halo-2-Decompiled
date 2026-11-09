#pragma once
#include <array>
#include <bit>
#include <cstdint>

namespace halo2::native_polygon_support_detail {
using Vector=std::array<std::uint32_t,4>;
struct Progress {
    std::uint32_t count{},index{},selected{},output{};
    Vector direction{},vertex{},products{},xy{},dot{},candidate{},best{},result{};
};
// Native support-point selection over serialized vertex DATA. The view owns
// scratch/output storage and source ABI publication; comparisons and loop
// decisions are geometric operations rather than virtual CPU execution.
template<class Frame>void support(Frame&frame,Progress&d){
    d={};frame.begin();d.count=frame.count();d.best=frame.initial_best();frame.initialize();
    frame.count_test(d);
    if(std::bit_cast<std::int32_t>(d.count)>0){
        d.direction=frame.direction();frame.vertices();
        do{
            d.vertex=frame.vertex(d);d.products=frame.multiply(d.vertex,d.direction);
            d.xy=frame.add_low(frame.shuffle_y(d.products),d.products);
            d.dot=frame.add_low(frame.shuffle_z(d.products),d.xy);frame.store_dot(d);
            d.candidate=frame.load_dot();
            if(frame.greater(d)){
                d.best=d.candidate;d.selected=d.index;frame.improved(d);
            }
            ++d.index;frame.advance(d);
        }while(d.index<d.count);
    }
    d.output=frame.output();frame.select(d);d.result=frame.selected_vertex(d);
    frame.restore_before_store();frame.write_result(d);frame.finish();
}
}
