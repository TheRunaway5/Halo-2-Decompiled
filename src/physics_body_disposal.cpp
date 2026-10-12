#include "halo2/physics_body_disposal.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
using Memory=SourcePhysicsContactMemory;
using Observer=SourcePhysicsContactRecordObserver;
using Services=SourcePhysicsBodyDisposalServices;
std::uint32_t read(const Memory&m,std::uint32_t p,unsigned n=4) {
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v=0;
    for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8);
    return v;
}
std::int32_t sign(std::uint32_t x){return std::bit_cast<std::int32_t>(x);}
void write(Memory&m,const Observer&o,std::uint32_t site,std::uint32_t p,
           unsigned n,std::uint32_t v) {
    SourcePhysicsContactRecordStore s{site,p,{},{}};s.before.resize(n);s.written.resize(n);
    m.read(p,s.before);for(unsigned i=0;i<n;++i)s.written[i]=std::byte((v>>(i*8))&255u);
    m.write(p,s.written);if(o)o(s);
}
[[noreturn]]void unavailable(const char*s){throw std::runtime_error(s);}
void free_block(Memory&m,std::uint32_t p,std::uint32_t bytes,std::uint32_t cls,
    std::uint32_t site,const Services&s,const Observer&o) {
    const auto adapter=read(m,0x487638);
    if(read(m,read(m,adapter)+0x14)!=0x148a70)
        unavailable("Required original disposal allocator virtual owner unavailable");
    if(s.observe_free)s.observe_free({site,adapter,p,bytes,cls});
    if(!p)return;
    const auto backend=read(m,0x480da0);
    if(sign(p)<sign(backend+0x4340u)||sign(p)>sign(backend+0xf0340u))
        unavailable("Required original disposal alternate allocator free unavailable");
    if(read(m,read(m,backend)+0x14)!=0x22eb20)
        unavailable("Required original disposal small-block free owner unavailable");
    source_free_physics_body_small_block_1_1(m,backend,p,bytes,o);
}
void required(Memory&m,const Services&s,std::uint32_t site,std::uint32_t target,
              std::uint32_t p,std::vector<std::uint32_t>args,const Observer&o) {
    if(!s.virtual_owner)unavailable("Required original disposal virtual owner unavailable");
    s.virtual_owner(m,{site,target,p,std::move(args)},o);
}
void base_reference(Memory&m,std::uint32_t p,std::uint32_t first_site,
    std::uint32_t flag_site,std::uint32_t last_site,const Services&s,const Observer&o) {
    const auto header=read(m,p+4,2);
    write(m,o,first_site,p,4,0x459d98);
    if(header&0x8000u) {
        write(m,o,flag_site,p+4,2,header&0x7fffu);
        if(!s.clear_reference_flag)
            unavailable("Required original2DD7F0 reference flag owner unavailable");
        s.clear_reference_flag(m,p,o);
    }
    write(m,o,last_site,p,4,0x459d94);
}
void release_virtual(Memory&,std::uint32_t,std::uint32_t,std::uint32_t,
                     const Services&,const Observer&);
void release_reference(Memory&m,std::uint32_t p,std::uint32_t dec_site,
    std::uint32_t resident_site,std::uint32_t call_site,const Services&s,const Observer&o) {
    write(m,o,dec_site,p+6,2,read(m,p+6,2)-1u);
    if(read(m,p+6,2)!=0)return;
    if(source_physics_body_reference_is_resident_1_1(m,p))write(m,o,resident_site,p+6,2,1);
    else release_virtual(m,p,1,call_site,s,o);
}
void listeners(Memory&m,std::uint32_t p,const Services&s,const Observer&o) {
    for(auto i=read(m,p+0x80)-1u;sign(i)>=0;--i) {
        const auto q=read(m,read(m,p+0x7c)+i*4u);
        if(q)required(m,s,0x31c1dd,read(m,read(m,q)+4),q,{p},o);
    }
    for(const auto [base,count,site]:std::array<std::array<std::uint32_t,3>,2>{{
        {0x88,0x8c,0x31c204},{0x94,0x98,0x31c234}}}) {
        for(std::uint32_t i=0;sign(i)<sign(read(m,p+count));++i) {
            const auto q=read(m,read(m,p+base)+i*4u);
            if(q)required(m,s,site,read(m,read(m,q+8)+4),q+8,{p},o);
        }
    }
}
void entity_base(Memory&m,std::uint32_t p,const Services&s,const Observer&o) {
    write(m,o,0x31192e,p,4,0x419dd0);
    const auto shape=read(m,p+0xc);
    if(shape)release_reference(m,shape,0x311943,0x31195b,0x311969,s,o);
    const auto cap=read(m,p+0x38);
    if(sign(cap)>=0)free_block(m,read(m,p+0x30),(cap&0x7fffffffu)*8u,0x12,0x31198e,s,o);
    base_reference(m,p,0xdd97b,0xdd98a,0xdd993,s,o);
}
void entity(Memory&m,std::uint32_t p,const Services&s,const Observer&o) {
    write(m,o,0x3124ce,p,4,0x419ea4);listeners(m,p,s,o);
    const auto q=read(m,p+0x54);
    if(q)release_reference(m,q,0x3124ec,0x312504,0x312512,s,o);
    for(const auto [bank,cap,site]:std::array<std::array<std::uint32_t,3>,5>{{
        {0x94,0x9c,0x31253d},{0x88,0x90,0x312569},{0x7c,0x84,0x312592},
        {0x70,0x78,0x3125b8},{0x60,0x68,0x3125de}}}) {
        const auto n=read(m,p+cap);
        if(sign(n)>=0)free_block(m,read(m,p+bank),(n&0x7fffffffu)*4u,0x12,site,s,o);
    }
    entity_base(m,p,s,o);
}
void rigid_body(Memory&m,std::uint32_t p,const Services&s,const Observer&o) {
    write(m,o,0x31798e,p,4,0x41a950);
    const auto motion=read(m,p+0x3c);
    auto kind=[&] {
        const auto q=read(m,p+0x3c),target=read(m,read(m,q)+0x18);
        if(target==0x31b250)return 7u;
        if(target==0x31b260)return 6u;
        if(target==0x9d900)return 4u;
        if(target==0x9b560)return 5u;
        unavailable("Required original disposal motion-kind owner unavailable");
    };
    if(kind()==6u) {
        const auto q=read(m,motion+0xc0);
        if(q)release_virtual(m,q,1,0x3179bc,s,o);
    }
    if(kind()==7u) {
        const auto q=read(m,motion+0xc0);
        if(q)release_virtual(m,q,1,0x3179dc,s,o);
    }
    if(motion)release_virtual(m,motion,1,0x3179e8,s,o);
    entity(m,p,s,o);
}
void release_virtual(Memory&m,std::uint32_t p,std::uint32_t flags,
    std::uint32_t site,const Services&s,const Observer&o) {
    const auto target=read(m,read(m,p));
    switch(target) {
    case 0x317ad0:
        rigid_body(m,p,s,o);
        if(flags&1u)free_block(m,p,read(m,p+4,2),0x28,0x317aef,s,o);
        break;
    case 0x320d20:
        write(m,o,0x2e00f0,p,4,0x459d94);
        if(flags&1u)free_block(m,p,read(m,p+4,2),0x29,0x320d3f,s,o);
        break;
    case 0x31fa60:
        write(m,o,0x2e00f0,p,4,0x459d94);
        if(flags&1u)free_block(m,p,read(m,p+4,2),0x29,0x31fa7f,s,o);
        break;
    case 0x1c5e00: {
        // The controller's 2E4D30 shape owns its child directly. This
        // decrement deliberately does not use the resident-reference gate.
        write(m,o,0x2e4d4d,p,4,0x45c5a0);
        const auto child=read(m,p+0xc);
        write(m,o,0x2e4d56,child+6,2,read(m,child+6,2)-1u);
        if(read(m,child+6,2)==0)release_virtual(m,child,1,0x2e4d6d,s,o);
        base_reference(m,p,0xdd97b,0xdd98a,0xdd993,s,o);
        if(flags&1u)free_block(m,p,0x50,0x22,0x1c5e1c,s,o);
        break;
    }
    case 0x2e2500:
        write(m,o,0x2e21ce,p,4,0x418930);
        release_reference(m,read(m,p+0x30),0x2e21d7,0x2e21f7,0x2e2205,s,o);
        base_reference(m,p,0xdd97b,0xdd98a,0xdd993,s,o);
        if(flags&1u)free_block(m,p,read(m,p+4,2),0x22,0x2e251f,s,o);
        break;
    case 0x1d53c0:
        base_reference(m,p,0x1d53cb,0x1d53da,0x1d53e8,s,o);
        if(flags&1u)free_block(m,p,read(m,p+4,2),0x26,0x1d5400,s,o);
        break;
    default:required(m,s,site,target,p,{flags},o);
    }
}
}
void source_remove_physics_entity_property_1_1(Memory&m,std::uint32_t p,
    std::uint32_t output,std::uint32_t key,const Observer&o) {
    for(std::uint32_t i=0;sign(i)<sign(read(m,p+0x34));++i) {
        const auto bank=read(m,p+0x30);
        if(read(m,bank+i*8u)!=key)continue;
        const auto value=read(m,bank+i*8u+4),count=read(m,p+0x34)-1u;
        write(m,o,0x318953,p+0x34,4,count);
        for(auto j=i;sign(j)<sign(read(m,p+0x34));++j) {
            const auto at=read(m,p+0x30)+j*8u;
            write(m,o,0x31896a,at,4,read(m,at+8));
            write(m,o,0x31896f,at+4,4,read(m,at+12));
        }
        write(m,o,0x31897e,output,4,value);return;
    }
    write(m,o,0x318933,output,4,0);
}
bool source_physics_body_reference_is_resident_1_1(const Memory&m,std::uint32_t p) {
    constexpr auto start=0x80061000u;
    if(sign(p)<sign(start))return p==start;
    const auto flag=read(m,0x54f248,1),bytes=read(m,0x54f26c);
    const auto end=start+(flag?bytes:0u);
    return sign(p)<=sign(end)||p==end;
}
std::uint32_t source_destroy_physics_rigid_body_1_1(Memory&m,std::uint32_t p,
    std::uint32_t flags,const Services&s,const Observer&o) {
    rigid_body(m,p,s,o);
    if(flags&1u)free_block(m,p,read(m,p+4,2),0x28,0x317aef,s,o);
    return p;
}
std::uint32_t source_dispose_physics_component_bodies_1_1(Memory&m,std::uint32_t p,
    const SourcePhysicsBodyDisposalScratch&scratch,const Services&s,const Observer&o) {
    for(std::uint32_t i=0;sign(i)<sign(read(m,p+0x80));++i) {
        const auto row=read(m,p+0x7c)+i*12u,q=read(m,row+8);
        release_reference(m,q,0x1d3bd9,0x1d3bf1,0x1d3bff,s,o);
        write(m,o,0x1d3c05,row+8,4,0);
    }
    for(std::uint32_t i=0;sign(i)<sign(read(m,p+0x74));++i) {
        const auto slot=read(m,p+0x70)+i*0x60u,body=read(m,slot+0x40);
        auto retire=[&](std::uint32_t key,std::uint32_t output) {
            for(std::uint32_t n=0;sign(n)<sign(read(m,body+0x34));++n)
                if(read(m,read(m,body+0x30)+n*8u)==key) {
                    source_remove_physics_entity_property_1_1(m,body,output,key,o);break;
                }
            source_add_physics_entity_property_1_1(m,body,key,0xffffffffu,o);
        };
        retire(0x2001,scratch.removed_property_2001);
        retire(0x2002,scratch.removed_property_2002);
        release_reference(m,body,0x1d3cbd,0x1d3d03,0x1d3d11,s,o);
        write(m,o,0x1d3d13,slot+0x40,4,0);
        const auto cap=read(m,slot+0x50);
        if(sign(cap)>=0)free_block(m,read(m,slot+0x48),cap&0x7fffffffu,0x12,0x1d3d35,s,o);
    }
    const auto reference98=read(m,p+0x98);
    write(m,o,0x1d3d57,p+4,4,0);write(m,o,0x1d3d5a,p+0x1c,1,0);
    write(m,o,0x1d3d5d,p+0x18,1,0xff);write(m,o,0x1d3d61,p+0x19,1,0xff);
    write(m,o,0x1d3d65,p+0x74,4,0);write(m,o,0x1d3d68,p+0x80,4,0);
    if(reference98) {
        release_reference(m,reference98,0x1d3d70,0x1d3d87,0x1d3d95,s,o);
        write(m,o,0x1d3d9d,p+0x98,4,0);
    }
    const auto reference9c=read(m,p+0x9c);
    if(reference9c) {
        release_reference(m,reference9c,0x1d3dad,0x1d3dc4,0x1d3dd2,s,o);
        write(m,o,0x1d3dd8,p+0x9c,4,0);
    }
    return p;
}
} // namespace halo2
