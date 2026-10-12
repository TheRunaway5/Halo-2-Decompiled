#include "halo2/physics_world_membership.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
using Memory=SourcePhysicsContactMemory;using Observer=SourcePhysicsContactRecordObserver;
std::uint32_t read(const Memory&m,std::uint32_t p,unsigned n=4) {
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v=0;
    for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8);
    return v;
}
std::int32_t sign(std::uint32_t x){return std::bit_cast<std::int32_t>(x);}
void write(Memory&m,const Observer&o,std::uint32_t site,std::uint32_t p,unsigned n,std::uint32_t v) {
    SourcePhysicsContactRecordStore s{site,p,{},{}};s.before.resize(n);s.written.resize(n);
    m.read(p,s.before);for(unsigned i=0;i<n;++i)s.written[i]=std::byte((v>>(i*8))&255u);
    m.write(p,s.written);if(o)o(s);
}
[[noreturn]]void unavailable(const char*s){throw std::runtime_error(s);}
std::uint32_t allocate_island(Memory&m,const Observer&o) {
    const auto adapter=read(m,0x487638);
    if(read(m,read(m,adapter)+0x10)!=0x148a10||read(m,0x480d98)!=2)
        unavailable("Required original world-island allocator unavailable");
    const auto backend=read(m,0x480da0);
    if(read(m,read(m,backend)+0x10)!=0x22e950)
        unavailable("Required original world-island backend allocator unavailable");
    const auto p=source_allocate_physics_body_small_block_1_1(m,backend,0xb0,o);
    if(!p)unavailable("Required original world-island alternate allocation unavailable");
    return p;
}
}
std::uint32_t source_construct_physics_world_island_1_1(Memory&m,std::uint32_t p,
    std::uint32_t world,const Observer&o) {
    for(const auto [site,offset,value]:std::array<std::array<std::uint32_t,3>,33>{{
        {0x31d738,8,0},{0x31d73b,0xc,0},{0x31d73e,0x10,0},{0x31d741,0x14,0},
        {0x31d744,0x18,0},{0x31d747,0x1c,0x419ebc},{0x31d74e,0x20,world},
        {0x31d751,0,0x41b610},{0x31d757,0x1c,0x41b604},
        {0x31d76c,0x34,0},{0x31d76f,0x38,0},{0x31d772,0x40,0},
        {0x31d775,0x44,0x80000001},{0x31d77f,0x3c,p+0x48},
        {0x31d782,0x4c,0},{0x31d785,0x50,0},{0x31d78d,0x54,0x80000000},
        {0x31d790,0x5c,0},{0x31d793,0x60,0x80000001},{0x31d79e,0x58,p+0x64},
        {0x31d7a1,0x68,0},{0x31d7a4,0x6c,0},{0x31d7a7,0x70,0x80000000},
        {0x31d7aa,0x74,0},{0x31d7ad,0x78,0},{0x31d7b0,0x7c,0x80000000},
        {0x31d7b3,0x80,0},{0x31d7b9,0x84,0},{0x31d7bf,0x88,0x80000000},
        {0x31d7c5,0x8c,0},{0x31d7cb,0x90,0},{0x31d7d1,0x94,0x80000000},
        {0x31d7d7,0xa8,0xffffffff}}}) {
        write(m,o,site,p+offset,4,value);
        if(site==0x31d757) {
            write(m,o,0x31d75e,p+0x28,1,0);write(m,o,0x31d763,p+0x29,1,1);
            write(m,o,0x31d766,p+0x2c,1,0);write(m,o,0x31d769,p+0x30,1,1);
        }
    }
    write(m,o,0x31d7e1,p+0xac,2,0);write(m,o,0x31d7e8,p+0xae,1,0);return p;
}
std::uint32_t source_append_physics_world_island_body_1_1(Memory&m,
    std::uint32_t p,std::uint32_t body,const Observer&o) {
    write(m,o,0x31d25c,body+0x44,4,p);
    if(read(m,p+0x40)==(read(m,p+0x44)&0x7fffffffu))
        source_double_physics_body_bank_1_1(m,p+0x3c,4,o);
    write(m,o,0x31d27e,read(m,p+0x3c)+read(m,p+0x40)*4u,4,body);
    write(m,o,0x31d281,p+0x40,4,read(m,p+0x40)+1u);
    write(m,o,0x31d28b,body+0x58,4,read(m,p+0x40)-1u);
    const auto motion=read(m,body+0x3c);
    if(read(m,read(m,motion)+0x14)!=0x31f8a0)
        unavailable("Required original world-island motion contribution owner unavailable");
    if(read(m,read(m,p)+0x10)!=0x31c5a0)
        unavailable("Required original world-island contribution commit owner unavailable");
    write(m,o,0x31c5a9,p+0x14,4,read(m,p+0x14)+0x80u);return 0x80;
}
std::uint32_t source_commit_physics_world_membership_1_1(Memory&m,std::uint32_t world,
    std::uint32_t body,const Observer&o) {
    write(m,o,0x312df4,body+8,4,world);
    if(read(m,body+0x40,1))return source_append_physics_world_island_body_1_1(m,read(m,world+0x2c),body,o);
    const bool separate=read(m,world+0x23c,1)!=0;
    if(separate||read(m,world+0xc)==0) {
        const auto p=allocate_island(m,o);
        write(m,o,separate?0x312e21:0x312eaa,p+4,2,0xb0);
        source_construct_physics_world_island_1_1(m,p,world,o);
        write(m,o,separate?0x312e40:0x312ecc,p+0x24,4,read(m,world+0xc));
        if(separate)source_append_physics_world_island_body_1_1(m,p,body,o);
        if(read(m,world+0xc)==(read(m,world+0x10)&0x7fffffffu))
            source_double_physics_body_bank_1_1(m,world+8,4,o);
        const auto i=read(m,world+0xc);
        write(m,o,separate?0x312e76:0x312ef7,read(m,world+8)+i*4u,4,p);
        write(m,o,separate?0x312e79:0x312efa,world+0xc,4,i+1u);
        if(separate)return i;
    }
    return source_append_physics_world_island_body_1_1(m,read(m,read(m,world+8)),body,o);
}
void source_notify_physics_world_insertion_1_1(Memory&m,std::uint32_t world,std::uint32_t body,
    const SourcePhysicsWorldInsertionServices&s,const Observer&o) {
    for(auto i=read(m,world+0x4c)-1u;sign(i)>=0;--i) {
        const auto p=read(m,read(m,world+0x48)+i*4u);
        if(p) {
            if(!s.world_listener)unavailable("Required original world insertion listener unavailable");
            s.world_listener(m,read(m,read(m,p)+0xc),p,body,o);
        }
    }
    for(auto i=read(m,world+0x4c)-1u;sign(i)>=0;--i) {
        if(read(m,read(m,world+0x48)+i*4u)!=0)continue;
        write(m,o,0x31e651,world+0x4c,4,read(m,world+0x4c)-1u);
        for(auto j=i;sign(j)<sign(read(m,world+0x4c));++j) {
            const auto at=read(m,world+0x48)+j*4u;
            write(m,o,0x31e66a,at,4,read(m,at+4));
        }
    }
}
std::uint32_t source_insert_physics_world_body_1_1(Memory&m,std::uint32_t world,std::uint32_t body,
    const SourcePhysicsWorldInsertionServices&s,const Observer&o) {
    const auto previous=read(m,world+0x12c,1);write(m,o,0x315830,world+0x12c,1,1);
    const bool static_body=read(m,body+0x40,1)!=0;
    const auto at=world+(static_body?0x134u:0x130u),old=read(m,at);
    write(m,o,static_body?0x315847:0x315858,at,4,static_body?old-1u:old+1u);
    write(m,o,0x315861,body+0x20,4,old);
    source_commit_physics_world_membership_1_1(m,world,body,o);
    if(!s.broadphase)unavailable("Required original3144D0 world broadphase owner unavailable");
    s.broadphase(m,world,body,o);
    write(m,o,0x315872,world+0x12c,1,previous);
    write(m,o,0x315878,body+6,2,read(m,body+6,2)+1u);
    source_notify_physics_world_insertion_1_1(m,world,body,s,o);return body;
}
} // namespace halo2
