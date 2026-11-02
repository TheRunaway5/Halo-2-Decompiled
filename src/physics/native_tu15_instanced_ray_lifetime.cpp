#include "halo2/native_tu15_instanced_ray_lifetime.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),o_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{std::array<std::byte,4>b{};m_.read(p,std::span(b).first(width));std::uint32_t n{};for(unsigned k=0;k<width;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;}
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t n,unsigned width=4){std::array<std::byte,4>b{};for(unsigned k=0;k<width;++k)b[k]=std::byte(n>>(k*8u));SourcePhysicsContactRecordStore e{pc,p,{},{}};if(o_){e.before.resize(width);m_.read(p,e.before);e.written.assign(b.begin(),b.begin()+width);}m_.write(p,std::span(b).first(width));if(o_)o_(e);}
};
}
void native_tu15_reference_instanced_mopp_code(SourcePhysicsContactMemory&memory,std::uint32_t code,
    const SourcePhysicsContactRecordObserver&observe){Memory m(memory,observe);m.word(0x30aa40u,code+0x24u,m.word(code+0x24u)+1u);}
void native_tu15_release_instanced_mopp_code(SourcePhysicsContactMemory&memory,std::uint32_t code,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15InstancedRayLifetimeServices&services,const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);const auto count=m.word(code+0x24u)-1u;m.word(0x30aa50u,code+0x24u,count);
    if(count)return;const auto allocator=m.word(0x48c938u),table=m.word(allocator),owner=m.word(table+4u);
    if(!services.release_code)throw std::runtime_error("Native instanced MOPP code requires actual release owner");services.release_code(allocator,owner,code,fp);
}
std::uint32_t native_tu15_initialize_instanced_ray_shape(SourcePhysicsContactMemory&memory,
    std::uint32_t shape,std::uint32_t collection,std::uint32_t code,const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);m.word(0x2e97afu,shape+6u,1u,2);m.word(0x2e97b5u,shape+8u,0u);
    m.word(0x2e97b8u,shape,0x461d68u);m.word(0x2e97beu,shape+12u,collection);
    m.word(0x2e97c1u,collection+6u,m.word(collection+6u,2)+1u,2);
    m.word(0x2e97d1u,shape,0x461e24u);m.word(0x2e97d7u,shape+16u,code);
    native_tu15_reference_instanced_mopp_code(memory,code,observe);return shape;
}
std::uint32_t native_tu15_initialize_instanced_ray_cache(SourcePhysicsContactMemory&memory,
    std::uint32_t cache,const SourcePhysicsContactRecordObserver&observe){Memory m(memory,observe);m.word(0x2e96a2u,cache+0x44u,0u);return cache;}
void native_tu15_destroy_instanced_collection_base(SourcePhysicsContactMemory&memory,
    std::uint32_t shape,NativeTu15PhysicsFrameFp&fp,const NativeTu15InstancedRayLifetimeServices&services,
    const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);const auto collection=m.word(shape+12u);m.word(0x1f3607u,shape,0x461d68u);
    m.word(0x1f360du,collection+6u,m.word(collection+6u,2)-1u,2);const auto count=m.word(collection+6u,2);
    if(!count){
        if(native_tu15_reserved_body_reference(memory,{collection}))m.word(0x1f3624u,collection+6u,1u,2);
        else{
            const auto table=m.word(collection),owner=m.word(table);
            if(!services.destroy_collection)throw std::runtime_error("Native instanced ray requires actual collection destructor");services.destroy_collection(collection,owner,1u,fp);
        }
    }
    const auto flags=m.word(shape+4u,2);m.word(0x1f363cu,shape,0x45eddcu);
    if(flags&0x8000u){m.word(0x1f364du,shape+4u,flags&0x7fffu,2);
        if(!services.allocation_mode)throw std::runtime_error("Native instanced ray requires2E12A0 allocation owner");services.allocation_mode(shape,0u,fp);
    }
    m.word(0x1f3657u,shape,0x45edf0u);
}
void native_tu15_destroy_instanced_ray_shape(SourcePhysicsContactMemory&memory,
    std::uint32_t shape,NativeTu15PhysicsFrameFp&fp,const NativeTu15InstancedRayLifetimeServices&services,
    const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);m.word(0x2e96cdu,shape,0x461e24u);const auto code=m.word(shape+16u);
    native_tu15_release_instanced_mopp_code(memory,code,fp,services,observe);
    native_tu15_destroy_instanced_collection_base(memory,shape,fp,services,observe);
}
} // namespace halo2
