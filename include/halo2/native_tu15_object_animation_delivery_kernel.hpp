#pragma once
#include <bit>
#include <cstdint>

namespace halo2 {
enum class NativeTu15ObjectAnimationPhase {
    begin,selection,graph_tag,animation_index,packed_id,tag_array,graph,
    scan_result,count_test,sample_output,sample_orientation,pose_output,
    pose_orientation,notify_output,post_enabled,post_flags,object_pool,
    object_rows,object_body,post_offset,post_output,resolve_call,sample_call,
    pose_call,notify_call,post_call,finish
};
struct NativeTu15ObjectAnimation {
    std::uint32_t manager{},count{},mask{},object{},selection{},graph_tag{},
        animation_index{},packed_id{},tag_array{},graph{},scan{},output{},
        orientation{},enabled{},flags{},pool{},rows{},body{},offset{};
};
// Selection availability is shared by animation delivery and the public
// manager query. Each acquired field remains live at its actual use boundary.
template<class Access>
bool native_tu15_object_animation_present(Access&memory,NativeTu15ObjectAnimation&d){
    using P=NativeTu15ObjectAnimationPhase;
    d.selection=memory.read(P::selection,d,d.manager+0x68u,4u);
    if(d.selection==0xffffffffu)return false;
    d.graph_tag=memory.read(P::graph_tag,d,d.manager,4u);
    if(d.graph_tag==0xffffffffu)return false;
    d.animation_index=memory.read(P::animation_index,d,d.manager+6u,2u);
    return d.animation_index!=0xffffu;
}
// The two callers acquire the packed ID on opposite sides of the graph
// pointer lookup. Preserve that distinction for descriptor/caller aliases.
template<class Access,class Services>
void native_tu15_resolve_object_animation(Access&memory,NativeTu15ObjectAnimation&d,
    Services&services,bool late_packed_id){
    using P=NativeTu15ObjectAnimationPhase;
    if(!late_packed_id)d.packed_id=memory.read(P::packed_id,d,d.manager+4u,4u);
    d.tag_array=memory.read(P::tag_array,d,0x4f0414u,4u);
    d.graph=memory.read(P::graph,d,d.tag_array+(d.graph_tag&0xffffu)*16u+8u,4u);
    if(late_packed_id)d.packed_id=memory.read(P::packed_id,d,d.manager+4u,4u);
    memory.prepare(P::resolve_call,d);services.resolve(d);
}
template<class Access,class Services>
void native_tu15_deliver_object_animation(Access&memory,NativeTu15ObjectAnimation&d,Services&services){
    using P=NativeTu15ObjectAnimationPhase;
    const auto signed_word=[](std::uint32_t value){return std::bit_cast<std::int32_t>(value);};
    memory.begin(d);
    if(d.mask){
        d.scan=services.scan(d);memory.prepare(P::scan_result,d);
        if(signed_word(d.scan)>=0){const auto used=d.scan+1u;if(signed_word(used)<signed_word(d.count))d.count=used;}
    }
    memory.prepare(P::count_test,d);
    if(d.count){
        if(native_tu15_object_animation_present(memory,d)){
            native_tu15_resolve_object_animation(memory,d,services,false);
            d.output=memory.frame(P::sample_output,d,0x1cu);
            d.orientation=memory.frame(P::sample_orientation,d,0x14u);
            memory.prepare(P::sample_call,d);services.sample(d);
        }
        d.output=memory.frame(P::pose_output,d,0x1cu);
        d.orientation=memory.frame(P::pose_orientation,d,0x14u);
        memory.prepare(P::pose_call,d);services.poses(d);
        d.output=memory.frame(P::notify_output,d,0x1cu);
        memory.prepare(P::notify_call,d);services.notify(d);
        d.enabled=memory.read(P::post_enabled,d,d.manager+0x65u,1u);
        if(d.enabled){
            d.flags=memory.read(P::post_flags,d,d.manager+0x67u,1u);
            if(!(d.flags&2u)){
                d.pool=memory.read(P::object_pool,d,0x4ecbd0u,4u);
                d.rows=memory.read(P::object_rows,d,d.pool+0x44u,4u);
                d.body=memory.read(P::object_body,d,d.rows+(d.object&0xffffu)*12u+8u,4u);
                const auto value=memory.read(P::post_offset,d,d.body+0x10eu,2u);
                d.offset=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
                d.output=memory.frame(P::post_output,d,0x1cu);
                memory.prepare(P::post_call,d);services.postprocess(d);
            }
        }
    }
    memory.finish(d);
}
} // namespace halo2
