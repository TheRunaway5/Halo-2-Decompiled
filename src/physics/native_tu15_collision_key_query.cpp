#include "halo2/native_tu15_collision_key_query.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{
        std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(width));std::uint32_t value{};
        for(unsigned k=0;k<width;++k)value|=std::to_integer<unsigned>(b[k])<<(k*8u);return value;
    }
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t value){
        SourcePhysicsContactRecordStore event{pc,p,{},{}};if(observe_){event.before.resize(4);memory_.read(p,event.before);}
        std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(value>>(k*8u));memory_.write(p,b);
        if(observe_){event.written.assign(b.begin(),b.end());observe_(event);}
    }
};
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
void erase(Memory&m,std::uint32_t array,std::uint32_t index,
    std::uint32_t count_store,std::uint32_t shift_store){
    const auto count=m.word(array+4u)-1u;m.word(count_store,array+4u,count);
    if(!less(index,count))return;
    do{
        const auto data=m.word(array),destination=data+index*4u,value=m.word(destination+4u);
        m.word(shift_store,destination,value);const auto current=m.word(array+4u);++index;
        if(!less(index,current))return;
    }while(true);
}
}
void native_tu15_prune_collision_keys(SourcePhysicsContactMemory&memory,
    std::uint32_t array,const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);auto count=m.word(array+4u);std::uint32_t index{};
    if(!less(0,count))return;
    do{
        const auto data=m.word(array),key=m.word(data+index*4u);
        if(!native_tu15_collision_component_enabled(memory,key)){
            erase(m,array,index,0x1f6fcbu,0x1f6fdau);--index;
        }
        count=m.word(array+4u);++index;
    }while(less(index,count));
}
void native_tu15_query_collision_keys(SourcePhysicsContactMemory&memory,
    std::uint32_t shape,std::uint32_t bounds,std::uint32_t array,
    NativeTu15CollisionKeyQueryScratch scratch,NativeTu15PhysicsFrameFp&fp,
    unsigned depth,const NativeTu15CollisionKeyQueryServices&services,
    const SourcePhysicsContactRecordObserver&observe){
    if(scratch.identity&15u)throw std::runtime_error("Native collision key query requires aligned matrix DATA");
    if(!services.query_shape)throw std::runtime_error("Native collision key query requires2E9670 owner");
    services.query_shape(shape,bounds,array,fp);
    native_tu15_prune_collision_keys(memory,array,observe);
    Memory m(memory,observe);auto count=m.word(array+4u);std::uint32_t index{};
    if(!less(0,count))return;
    do{
        const auto data=m.word(array),key=m.word(data+index*4u),instance_index=key&0xffffu;
        if((key&0xe0000000u)==0x40000000u){
            const auto runtime=m.word(0x4ecc18u),instances=m.word(runtime+0x144u);
            const auto instance=instances+instance_index*0x58u;
            const auto geometry_index=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(m.word(instance+0x34u,2)))));
            const auto geometry=m.word(runtime+0x13cu)+geometry_index*0xc8u;
            if(!less(0x2000u,m.word(geometry+0x98u))){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
                using namespace joint_constraint_numeric;const auto p=scratch.identity;std::uint32_t collision;
                {
                    Environment environment(fp,depth);joint_constraint_numeric::Memory n(memory,observe);
                    auto y=n.word(instance+0x10u),x=n.word(instance+4u);const auto maximum=n.vector(bounds+0x10u);
                    collision=n.word(geometry+0xb4u);
                    n.word(0x1f6db5u,p+0x40u,y);y=n.word(instance+0x14u);n.word(0x1f6dc0u,p+0x44u,y);
                    y=n.word(instance+0x18u);n.word(0x1f6dcbu,p+0x48u,y);
                    y=n.word(instance+0x1cu);n.word(0x1f6dd6u,p+0x50u,y);
                    y=n.word(instance+0x20u);n.word(0x1f6de1u,p+0x54u,y);
                    y=n.word(instance+0x24u);n.word(0x1f6decu,p+0x30u,x);
                    x=n.word(instance+8u);n.word(0x1f6df7u,p+0x58u,y);
                    y=n.word(instance+0x28u);n.word(0x1f6e02u,p+0x20u,y);
                    y=n.word(instance+0x2cu);n.word(0x1f6e0du,p+0x34u,x);
                    x=n.word(instance+0xcu);n.word(0x1f6e18u,p+0x24u,y);
                    y=n.word(instance+0x30u);n.word(0x1f6e23u,p+0x38u,x);
                    n.word(0x1f6e2cu,p+0x28u,y);const auto half=n.word(0x46a0ccu);
                    n.word(0x1f6e3au,p+0x3cu,0);n.word(0x1f6e40u,p+0x4cu,0);
                    n.word(0x1f6e46u,p+0x5cu,0);n.word(0x1f6e4cu,p+0x2cu,0);
                    n.vector(0x1f6e5au,p+0x60u,n.vector(p+0x20u));
                    const auto minimum=n.vector(bounds);auto delta=maximum;
                    asm volatile("subps %1,%0":"+x"(delta):"x"(minimum):"memory");
                    std::array<std::uint32_t,4>half_bits{half,half,half,half};Vector scalar;std::memcpy(&scalar,half_bits.data(),16);
                    const auto extent=multiply(scalar,delta);const auto zero=_mm_setzero_ps();
                    n.vector(0x1f6e76u,p+0x70u,zero);n.vector(0x1f6e7bu,p+0x80u,zero);n.vector(0x1f6e83u,p+0x90u,zero);
                    const auto one=n.word(0x46a0d0u);const auto center=add(minimum,extent);
                    n.vector(0x1f6ea7u,p+0xb0u,extent);
                    n.word(0x1f6eafu,p+0x70u,one);n.word(0x1f6eb5u,p+0x84u,one);n.word(0x1f6ebeu,p+0x98u,one);
                    n.vector(0x1f6ec7u,p+0xa0u,center);
                }
                NativeTu15InverseGeometryMemory inverse(memory,p+0xc0u,p+0x30u,p+0x70u,observe);
                native_tu15_compose_inverse_affine_geometry(inverse,fp);
                erase(m,array,index,0x1f6edcu,0x1f6eecu);
                const auto instance_shape=collision+0x50u,table=m.word(instance_shape),before=m.word(array+4u);
                const auto owner=m.word(table+0x2cu);
                if(!services.query_instance)throw std::runtime_error("Native collision key query requires actual instance virtual2C owner");
                --index;services.query_instance(instance_shape,owner,p+0xc0u,p+0xb0u,0x38d1b717u,array,fp);
                if(less(before,m.word(array+4u))){
                    auto current=before;
                    do{
                        const auto keys=m.word(array),child=m.word(keys+current*4u);
                        const auto encoded=((child|0xffffa000u)<<16u)|instance_index;
                        if(native_tu15_collision_component_enabled(memory,encoded)){
                            const auto output=m.word(array);m.word(0x1f6f52u,output+current*4u,encoded);
                        }else{erase(m,array,current,0x1f6f5fu,0x1f6f6fu);--current;}
                        const auto current_count=m.word(array+4u);++current;
                        if(!less(current,current_count))break;
                    }while(true);
                }
#else
                (void)bounds;(void)depth;
                throw std::runtime_error("Exact native collision instance query backend unavailable");
#endif
            }
        }
        count=m.word(array+4u);++index;
    }while(less(index,count));
}
} // namespace halo2
