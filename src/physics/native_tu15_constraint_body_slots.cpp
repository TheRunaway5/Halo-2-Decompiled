#include "halo2/native_tu15_constraint_body_slots.hpp"
#include <bit>
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t out{};for(unsigned n=0;n<width;++n)out|=std::to_integer<unsigned>(b[n])<<(n*8u);return out;}
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
std::uint32_t signed_byte(std::uint32_t v){return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(v))));}
std::uint32_t property(const SourcePhysicsContactMemory&m,std::uint32_t body,std::uint32_t count,std::uint32_t key){
    if(!less(0u,count))return 0;const auto bank=word(m,body+0x30u);std::uint32_t index{};
    do{if(word(m,bank+index*8u)==key)return word(m,bank+index*8u+4u);++index;}while(less(index,count));return 0;
}
NativeTu15ConstraintBodySlots model(const SourcePhysicsContactMemory&m,std::uint32_t component,std::uint32_t index){ //DE8C0
    const auto entries=word(m,component+0x7cu),constraint=word(m,entries+index*12u+8u),first=word(m,constraint+0xcu),first_count=word(m,first+0x34u),second=word(m,constraint+0x10u);
    const auto first_slot=property(m,first,first_count,0x2002u),second_count=word(m,second+0x34u);
    if(property(m,second,second_count,0x2001u)==0xffffffffu)return{first_slot,0xffffffffu};
    return{first_slot,property(m,second,second_count,0x2002u)};
}
}
NativeTu15ConstraintBodySlots native_tu15_constraint_body_slots(const SourcePhysicsContactMemory&m,
 std::uint32_t instance,std::uint32_t datum,NativeTu15ConstraintBodySlotStats*stats){
    if(stats)++stats->calls;const auto first=word(m,instance+0x24u),manager=word(m,0x52be5cu),components=word(m,manager+0x44u);
    const auto kind=word(m,instance+0x9cu,2),component=components+(first&0xffffu)*160u;
    if(kind!=0xffffu){
        if(stats)++stats->model_queries;std::uint32_t selected=0xffffffffu;const auto count=word(m,component+0x80u);
        if(less(0u,count)){const auto entries=word(m,component+0x7cu);std::uint32_t index{};
            do{const auto candidate_kind=word(m,entries+index*12u,2),candidate_index=word(m,entries+index*12u+2u,2);
                if(candidate_kind==kind&&candidate_index==word(m,instance+0x9eu,2))selected=index;++index;
            }while(less(index,count));
        }
        return model(m,component,selected);
    }
    if(word(m,instance+0xdu,1)!=0xffu){
        if(stats)++stats->primary_queries;const auto primary=signed_byte(word(m,component+0x18u,1));
        if(less(primary,0u))return{0xffffffffu,0xffffffffu};const auto count=word(m,component+0x74u);
        if(!less(primary,count))return{0xffffffffu,0xffffffffu};return{primary,0xffffffffu};
    }
    NativeTu15ConstraintBodySlots out{0xffffffffu,0xffffffffu};auto count=word(m,component+0x8cu);if(!less(0u,count))return out;
    std::uint32_t index{},offset{};
    do{
        if(stats)++stats->child_rows;const auto entries=word(m,component+0x88u),row=entries+offset,candidate=word(m,row+8u);
        if(candidate==datum){
            if(stats)++stats->matching_children;
            if(out.first==0xffffffffu)out.first=signed_byte(word(m,row+0x45u,1));
            else{const auto slot=signed_byte(word(m,row+0x45u,1));if(less(slot,out.first))out.first=signed_byte(word(m,row+0x45u,1));}
            if(word(m,row+0x14u)!=0xffffffffu){
                if(out.second==0xffffffffu)out.second=signed_byte(word(m,row+0x46u,1));
                else{const auto slot=signed_byte(word(m,row+0x46u,1));if(less(slot,out.second))out.second=signed_byte(word(m,row+0x46u,1));}
            }
        }
        count=word(m,component+0x8cu);++index;offset+=72u;
    }while(less(index,count));return out;
}
} // namespace halo2
