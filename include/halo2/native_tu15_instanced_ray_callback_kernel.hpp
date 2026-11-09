#pragma once
#include <cstdint>

namespace halo2::native_instanced_ray_callback_detail {
struct Forward {
    std::uint32_t collector{},one{},table{},packet{},body{},owner{};
};
// The caller-private packet is storage owned by the view. Resolve the actual
// collector only after the authored packet publication; do not cache its owner
// before a possibly aliased store.
template<class Frame>void forward(Frame&frame,Forward&d){
    d={};frame.begin();d.collector=frame.collector();d.one=frame.one();
    d.table=frame.table(d);d.packet=frame.packet();frame.push_packet(d);
    d.body=frame.body();frame.push_body(d);frame.fraction(d);
    d.owner=frame.owner(d);frame.invoke(d);frame.finish();
}

enum class Read {stamp,result_before,displacement,origin,filter,mask,accepted_mask,result_after,current,candidate,selected};
enum class Write {stamp,fraction,hit};
enum class Return {duplicate,no_hit,any_hit,keep,replace};
struct Collector {
    std::uint32_t body{},instance{},epoch{},collector{},index{},stamp{},result{};
    std::uint32_t displacement{},origin{},filter{},mask{},current{},candidate{},value{};
    bool keep{};
};
// Native instance de-duplication and closest-hit policy. Actual query execution
// and its resumed caller state belong to Frame. No hit/child outcome is supplied
// by the DATA algorithm. The result pointer and selected fraction remain lazy.
template<class Frame>Return collect(Frame&frame,Collector&d){
    d={};d.body=frame.body();d.instance=frame.instance(d);d.epoch=frame.epoch();
    d.collector=frame.begin(d);d.index=std::uint32_t(std::int32_t(std::int16_t(d.instance&0xffffu)));
    frame.index(d);d.stamp=frame.read(Read::stamp,d);frame.compare_stamp(d);
    if(d.stamp==d.epoch)return Return::duplicate;
    d.result=frame.read(Read::result_before,d);frame.write(Write::stamp,d.epoch,d);
    d.displacement=frame.read(Read::displacement,d);d.origin=frame.read(Read::origin,d);
    frame.push_displacement(d);d.filter=frame.read(Read::filter,d);frame.push_origin(d);
    d.mask=frame.read(Read::mask,d);frame.push_filter(d);frame.push_mask(d);
    if(!frame.query(d))return Return::no_hit;
    frame.resumed(d);
    if(frame.read(Read::accepted_mask,d)&0x10000000u){
        frame.zero(d);frame.write(Write::fraction,0u,d);frame.write(Write::hit,1u,d);return Return::any_hit;
    }
    d.result=frame.read(Read::result_after,d);d.current=frame.read(Read::current,d);
    d.candidate=frame.read(Read::candidate,d);d.keep=frame.compare(d);
    d.value=frame.read(Read::selected,d);frame.write(Write::fraction,d.value,d);frame.write(Write::hit,1u,d);
    return d.keep?Return::keep:Return::replace;
}
} // namespace halo2::native_instanced_ray_callback_detail
