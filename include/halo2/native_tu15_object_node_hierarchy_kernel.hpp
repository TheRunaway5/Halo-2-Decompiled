#pragma once
#include <bit>
#include <cstdint>

namespace halo2 {
// BF0C0's hierarchy publication stage, after root/physics/animation delivery.
// Local transforms are converted before composition with the already published
// parent matrix. A physics-owned node is left intact. These are live memory
// fields: neither the model pointer nor its node count is a cached loop bound.
enum class NativeTu15ObjectNodeHierarchyPhase {
    initial_count, matrices, orientations, begin, model, records, mask,
    local_call, parent, matrix_root, product_call, next_model, next_count,
    advance, finish_empty, finish_complete
};
struct NativeTu15ObjectNodeHierarchy {
    std::uint32_t model{}, count{}, index{}, record_offset{}, orientation{},
        matrix{}, record{}, mask_index{}, mask_bit{}, parent{}, matrix_root{};
};
// A real child can restore saved words that alias its output. Adopt the values
// actually resumed by that child, rather than assuming its incidental saved
// integer registers remained equal to the values at entry.
struct NativeTu15ObjectNodeHierarchyResume {
    std::uint32_t index{}, record_offset{}, orientation{}, matrix{}, record{};
};

template<class Access, class Services>
void native_tu15_publish_object_node_hierarchy(Access &memory,
    NativeTu15ObjectNodeHierarchy &s, Services &services) {
    using P=NativeTu15ObjectNodeHierarchyPhase;
    const auto signed_word=[](std::uint32_t x){return std::bit_cast<std::int32_t>(x);};
    const auto read=[&](P phase,std::uint32_t address,unsigned width=4u){return memory.read(phase,s,address,width);};
    const auto frame=[&](P phase,std::uint32_t offset){return read(phase,memory.frame_address(offset));};
    const auto adopt=[&](const NativeTu15ObjectNodeHierarchyResume &child){
        s.index=child.index;s.record_offset=child.record_offset;
        s.orientation=child.orientation;s.matrix=child.matrix;s.record=child.record;
    };
    s.count=read(P::initial_count,s.model+0x48u);
    if(signed_word(s.count)<=1){memory.complete(P::finish_empty,s);return;}
    s.index=1u;s.matrix=frame(P::matrices,0x1cu);
    s.orientation=frame(P::orientations,0x18u);
    s.matrix+=0x34u;s.record_offset=0x60u;s.orientation+=0x20u;
    memory.prepare(P::begin,s);
    do {
        s.model=frame(P::model,0x14u);
        s.record=read(P::records,s.model+0x4cu)+s.record_offset;
        s.mask_index=std::uint32_t(signed_word(s.index)>>5u);
        s.mask_bit=1u<<(s.index&31u);
        const auto mask=read(P::mask,memory.frame_address(0x70u)+s.mask_index*4u);
        if(!(mask&s.mask_bit)) {
            memory.prepare(P::local_call,s);adopt(services.local(s));
            const auto parent=read(P::parent,s.record+4u,2u);
            s.parent=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(parent))));
            s.matrix_root=frame(P::matrix_root,0x1cu);
            memory.prepare(P::product_call,s);adopt(services.product(s.matrix_root+s.parent*0x34u,s));
        }
        s.model=frame(P::next_model,0x14u);
        s.count=read(P::next_count,s.model+0x48u);
        ++s.index;s.record_offset+=0x60u;s.orientation+=0x20u;s.matrix+=0x34u;
        memory.prepare(P::advance,s);
    }while(signed_word(s.index)<signed_word(s.count));
    memory.complete(P::finish_complete,s);
}
} // namespace halo2
