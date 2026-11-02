#include "halo2/native_tu15_collection_filter.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory& memory_;const SourcePhysicsContactRecordObserver& observe_;
public:
    Memory(SourcePhysicsContactMemory& memory,const SourcePhysicsContactRecordObserver& observe):memory_(memory),observe_(observe){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const {
        std::array<std::byte,4> bytes{};memory_.read(p,std::span(bytes).first(width));std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
    }
    void byte(std::uint32_t pc,std::uint32_t p,std::uint32_t value) {
        const std::byte byte{static_cast<unsigned char>(value)};SourcePhysicsContactRecordStore event{pc,p,{},{}};
        if(observe_){event.before.resize(1);memory_.read(p,event.before);event.written={byte};}
        memory_.write(p,std::span(&byte,1));if(observe_)observe_(event);
    }
    std::uint32_t root(std::uint32_t p)const {
        for(;;){const auto parent=word(p+12u);if(!parent)return p;p=parent;}
    }
};
std::uint32_t word(const SourcePhysicsContactMemory& memory,std::uint32_t p) {
    std::array<std::byte,4> bytes{};memory.read(p,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
}
// Actual185020 switch data185044/18504C. Its source consumes only AL.
bool shape_has_user_data(std::uint32_t type) {
    switch(type){case 1:case 4:case 5:case 6:case 7:case 8:case 24:return true;default:return false;}
}
}
std::uint32_t native_tu15_collection_child_count(const SourcePhysicsContactMemory& memory,
    NativeTu15ContactChild collection){return word(memory,collection.identity+16u);}
NativeTu15ContactChild native_tu15_collection_child(const SourcePhysicsContactMemory& memory,
    NativeTu15ContactChild collection,std::uint32_t index){
    const auto children=word(memory,collection.identity+12u);return {word(memory,children+index*8u)};
}
std::uint32_t native_tu15_filter_collection_child(SourcePhysicsContactMemory& memory,
    std::uint32_t receiver,std::uint32_t output,NativeTu15ContactPayload second,
    NativeTu15ContactPayload first,NativeTu15ContactChild collection,std::uint32_t index,
    NativeTu15CollectionFilterScratch scratch,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15CollectionFilterServices& services,const SourcePhysicsContactRecordObserver& observe) {
    Memory m(memory,observe);
    const auto classify=[&](std::uint32_t shape){
        const auto table=m.word(shape);const NativeTu15ContactVirtualOwner owner{m.word(table+20u)};
        if(!services.classify_shape)throw std::runtime_error("Native collection filter requires its real shape class owner");
        return services.classify_shape({shape},owner,fp);
    };
    const auto initial_root=m.root(second.identity),special_body=m.word(0x48b860u);
    if(special_body==m.word(initial_root+32u)) {
        std::uint32_t shape{};
        if(index!=0xffffffffu) {
            const auto table=m.word(collection.identity);const NativeTu15ContactVirtualOwner owner{m.word(table+52u)};
            if(!services.resolve_child)throw std::runtime_error("Native collection filter requires its real child resolver");
            shape=services.resolve_child(collection,owner,index,scratch.child_shape,fp).identity;
        } else shape=m.word(first.identity);
        if(classify(shape)==21u)shape=m.word(shape+12u);
        if(classify(shape)==23u)shape=m.word(shape+48u);
        if(classify(shape)!=24u&&shape_has_user_data(classify(shape))) {
            const auto user=m.word(shape+8u);
            if(user) {
                const auto signed_user=std::bit_cast<std::int32_t>(user);
                const auto clamped=signed_user<1?1u:signed_user>16?16u:user;
                if(clamped!=user&&(m.word(user+31u,1)&1u)) {
                    const auto root=m.root(second.identity);
                    if(m.word(root+24u)==1u) {
                        const auto body=m.word(root+32u);
                        if(body) {
                            const auto motion=m.word(body+60u),table=m.word(motion);
                            const NativeTu15ContactVirtualOwner owner{m.word(table+24u)};
                            if(!services.motion_type)throw std::runtime_error("Native collection filter requires its real motion-type owner");
                            if(services.motion_type(motion,owner,fp)==7u) {
                                m.byte(0x1cbf5bu,output,0);return output;
                            }
                        }
                    }
                }
            }
        }
    }
    const auto first_root=m.root(first.identity);
    //1CBF79 prefetches second.parent BEFORE1CBF7E reads first-root.group;
    // the rest of the second chain is followed only after that group read.
    auto parent=m.word(second.identity+12u);
    const auto column=m.word(first_root+28u);
    auto second_root=second.identity;
    while(parent){second_root=parent;parent=m.word(second_root+12u);}
    const auto row=m.word(second_root+28u);
    // Actual2E98D0 receives (row,column) in that order, after receiver-12.
    if(!((row^column)&0xffff0000u)&&(row&0xffff0000u))m.byte(0x2e98f2u,output,0);
    else {
        const auto mask=m.word((receiver-12u)+(row&31u)*4u+28u);
        m.byte(0x2e9913u,output,(mask&(1u<<(column&31u)))!=0);
    }
    return output;
}
}
