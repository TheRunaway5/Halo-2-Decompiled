#include "halo2/native_tu15_gameplay_policy_destruction.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned n=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v{};
    for(unsigned i=0;i<n;++i)v|=std::to_integer<unsigned>(b[i])<<(8u*i);return v;
}
void write(SourcePhysicsContactMemory&m,std::uint32_t pc,std::uint32_t p,
    std::uint32_t v,unsigned n,const SourcePhysicsContactRecordObserver&o){
    std::array<std::byte,4>b{};for(unsigned i=0;i<n;++i)b[i]=std::byte(v>>(8u*i));
    SourcePhysicsContactRecordStore event{pc,p,{},{}};
    if(o){event.before.resize(n);m.read(p,event.before);event.written.assign(b.begin(),b.begin()+n);}
    m.write(p,std::span(b).first(n));if(o)o(event);
}
}
std::uint32_t native_tu15_destroy_gameplay_body_policy(SourcePhysicsContactMemory&m,
    std::uint32_t policy,std::uint32_t disposing,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15GameplayPolicyDestructionServices&s,const SourcePhysicsContactRecordObserver&o){
    const auto flags=word(m,policy+4u,2);
    write(m,0x1d9c8bu,policy,0x45eddcu,4,o);
    if(flags&0x8000u){
        write(m,0x1d9c9au,policy+4u,flags&0x7fffu,2,o);
        if(!s.release_base_lock)throw std::runtime_error("Native TU1.5 gameplay policy requires actual base-lock release2E12A0");
        s.release_base_lock(policy,0u,fp);
    }
    write(m,0x1d9ca8u,policy,0x45edf0u,4,o);
    if(disposing&1u){
        const auto bytes=word(m,policy+4u,2),allocator=word(m,0x48c938u),table=word(m,allocator),owner=word(m,table+0x14u);
        if(!s.allocation.release)throw std::runtime_error("Native TU1.5 gameplay policy requires actual sized release");
        s.allocation.release({allocator,owner,policy,bytes,0x26u},fp);
    }
    return policy;
}
} // namespace halo2
