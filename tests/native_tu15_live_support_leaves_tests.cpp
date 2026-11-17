// Native TU1.5 live-support leaves (C9550, 1EC970, 1F7F60, C1E90, 110350,
// 243F20, 32CC0C in both caller x87 models, 16E600 and 16E4B0 with their
// strncpy call, 171010, 1211C0 with its 123210 child, 1F7E90, 32A4E0, 16E450,
// the leaves with genuine children through the child boundary of
// native_tu15_leaf_children.hpp (22A790 with 4057F0, 1F8200 with 144090 and
// 1E6980, 138D90 in both caller x87 models with 138510/1385D0/1386E0/138C20,
// 179CD0 with 16EA60/17C370/17C5C0/17A380/17C140; each child a stand-in that
// folds a digest of its entry registers into the bank), and the physics pool
// startup unit's copies of 16E450/16E4B0/16E600/32A4E0 with their canonical
// store PCs (run_with_store_sites and the binder's site maps):
// the systems in src/physics/runtime/native_tu15_live_support_systems.hpp
// through the adapters in native_tu15_live_support_leaves.hpp) against
// the unmodified translated bodies, extracted in their production cursor
// form by tools/validate_native_live_support_leaves_tu15.py into
// live_support_leaves_body.inc. Seeded fixtures author each leaf's complete
// reachable data (object pools, iterators, bit vectors, channel tables,
// interval records, float ramps and the LCG, vertex descriptors in every
// layout, x87 values for every conversion path, data arrays with aliasing
// and both string directions), random caller registers, flags,
// vector lanes, every MXCSR rounding/DAZ/FTZ mode with random sticky flags,
// special floats, aliasing and removed pages (faults). Each case compares the
// ordered store events (pc, address, preimage, bytes), the final bank, GP,
// EFLAGS, XMM, x87 status, MXCSR and control word, and the cursor. A fault
// compares the exception, the cursor and the ordered stores outside the
// leaf's own dead stack frame (some adapters make the register saves at
// entry; the original interleaves them with its first reads). Verify mode
// replays the leaves with children against their reference copies in exact
// read and store order. The reference's
// complete PC trace yields the observed CFG edges (printed for the validator).
// Modes: <cases> [verify|census]. Prints a JSON summary.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_live_support_leaves.hpp"
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace original {
using halo2::physics_world_runtime_detail::WorldState;
#include "live_support_leaves_body.inc"
}
namespace {
namespace leaves=native_tu15_leaves;
struct Memory : SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::shared_ptr<std::array<std::byte,4096>>> pages;
    std::array<std::byte,4096> &page(std::uint32_t at,bool create) {
        auto found=pages.find(at>>12);
        if(found==pages.end()) {
            if(!create){char t[64];std::snprintf(t,sizeof t,"Access escaped the authored fixture at %x",at);throw std::runtime_error(t);}
            found=pages.emplace(at>>12,std::make_shared<std::array<std::byte,4096>>()).first;
        }
        return *found->second;
    }
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        for(std::size_t n=0;n<out.size();++n)out[n]=const_cast<Memory*>(this)->page(at+std::uint32_t(n),false)[(at+n)&4095u];
    }
    void write(std::uint32_t at,std::span<const std::byte> in)override {
        for(std::size_t n=0;n<in.size();++n)page(at+std::uint32_t(n),false)[(at+n)&4095u]=in[n];
    }
    void put(std::uint32_t at,std::uint32_t value,unsigned width=4) {
        for(unsigned n=0;n<width;++n)page(at+n,true)[(at+n)&4095u]=std::byte(value>>(8*n));
    }
    Memory clone()const {
        Memory copy;for(const auto &[k,v]:pages)copy.pages.emplace(k,std::make_shared<std::array<std::byte,4096>>(*v));return copy;
    }
    bool same(const Memory &other)const {
        if(pages.size()!=other.pages.size())return false;
        for(const auto &[k,v]:pages){const auto at=other.pages.find(k);if(at==other.pages.end()||*at->second!=*v)return false;}
        return true;
    }
};
enum Leaf : unsigned {c9550,l1ec970,l1f7f60,c1e90,l110350,l243f20,l32cc0c,l32cc0c_tracked,l16e600,l171010,l1211c0,l1f7e90,l32a4e0,l16e4b0,l16e450,l22a790,l1f8200,l138d90,l138d90_untracked,l179cd0,
    l16e450_pool,l16e4b0_pool,l16e600_pool,l32a4e0_pool,leaf_count};
constexpr const char *names[]{"c9550","1ec970","1f7f60","c1e90","110350","243f20","32cc0c","32cc0c_tracked","16e600","171010","1211c0","1f7e90","32a4e0","16e4b0","16e450","22a790","1f8200","138d90","138d90_untracked","179cd0",
    "16e450_pool","16e4b0_pool","16e600_pool","32a4e0_pool"};
struct Case { Memory memory; SourcePhysicsWorldRuntimeAbi abi{}; std::vector<std::uint32_t> victims; };
constexpr std::uint32_t stack=0x20010000u;
// Leaf verify mode computes on a pure overlay, which never faults on a
// store: fills that run off the authored pages are release-mode cases only.
bool overlay_mode=false;

struct Rng {
    std::mt19937 &g;
    std::uint32_t operator()(){return std::uint32_t(g());}
    std::uint32_t pick(std::uint32_t n){return n?std::uint32_t(g()%n):0u;}
    bool chance(std::uint32_t n){return pick(n)==0;}
    std::uint32_t real(float lo,float hi){return std::bit_cast<std::uint32_t>(lo+(hi-lo)*float(g()%1000003)/1000003.0f);}
    std::uint32_t special() {
        switch(pick(10)) {
        case 0:return 0x7fc00000u|(std::uint32_t(g())&0x3fffffu);
        case 1:return 0x7f800001u|(std::uint32_t(g())&0x3ffffeu);
        case 2:return 0xff800000u;case 3:return 0x7f800000u;
        case 4:return 0x80000000u;case 5:return std::uint32_t(g())&0x007fffffu;
        case 6:return 0x80000000u|(std::uint32_t(g())&0x007fffffu);
        case 7:return 0u;
        case 8:return 0xffc00000u|(std::uint32_t(g())&0x3fffffu);
        default:return std::bit_cast<std::uint32_t>(float(int(g()%2000)-1000)*0.013f);
        }
    }
    std::uint32_t value(float lo,float hi,bool specials){return specials&&chance(3)?special():real(lo,hi);}
};
void common(Case &c,Rng &rng) {
    for(std::uint32_t at=stack-0x80u;at<stack+0x20u;at+=4)c.memory.put(at,rng());
    for(auto &v:c.abi.gp)v=rng();
    c.abi.gp[4]=stack;
    c.abi.eflags=0x202u|(rng()&0x8d5u);
    for(auto &lane:c.abi.xmm)for(auto &v:lane)v=rng();
    c.abi.x87_depth=rng.pick(4);
    for(unsigned i=0;i<c.abi.x87_depth;++i)c.abi.x87[i]=(long double)(int(rng()%2001)-1000)/7.0L;
    const unsigned top=(8u-c.abi.x87_depth+rng.pick(8))&7u;
    c.abi.x87_status=std::uint16_t((top<<11)|(rng()&0x4700u));
    const std::uint16_t control=std::uint16_t(0x3fu|0x40u|(rng.pick(4)<<8)|(rng.pick(4)<<10));
    // Masked SSE exceptions, any rounding, DAZ/FTZ and sticky flags.
    c.abi.floating_point=SourcePhysicsFactoryFloatingPoint{control,0x1f80u|(rng()&0xe03fu)};
}
void object_pool(Case &c,Rng &rng,std::uint32_t handle,std::uint32_t object) {
    constexpr std::uint32_t pool=0x10f00000u,rows=0x10f10000u;
    c.memory.put(0x4ecbd0u,pool);c.memory.put(pool+0x44u,rows);
    for(unsigned i=0;i<2;++i)c.memory.put(rows+(handle&0xffffu)*12u+i*4u,rng());
    c.memory.put(rows+(handle&0xffffu)*12u+8u,object);
    c.victims.insert(c.victims.end(),{0x4ecbd0u,pool,rows});
}

Case author_c9550(Rng &rng) {
    Case c;common(c,rng);auto &m=c.memory;
    const auto handle=(rng()&0xffff0000u)|rng.pick(6);c.abi.gp[1]=handle;
    const auto object=0x11200000u+rng.pick(4)*0x400u;
    object_pool(c,rng,handle,object);
    const std::int16_t offsets[]{0,0x10,0x100,-0x40,0x7f0};
    const auto offset=std::uint32_t(std::int32_t(offsets[rng.pick(5)]));
    m.put(object+0x12au,offset&0xffffu,2);
    std::uint32_t table[5];for(auto &v:table)v=rng.chance(3)?rng.pick(8):rng();
    if(rng.chance(4))table[rng.pick(5)]=table[rng.pick(5)];
    for(unsigned i=0;i<5;++i)m.put(0x473990u+i*4u,table[i]);
    m.put(0x4739a4u,rng());m.put(0x47398cu,rng());
    const auto key=rng.chance(3)?rng():table[rng.pick(5)];
    m.put(offset+object+0x7cu,key);
    c.victims.insert(c.victims.end(),{object,offset+object+0x7cu,0x473990u});
    return c;
}
Case author_1ec970(Rng &rng) {
    Case c;common(c,rng);auto &m=c.memory;
    constexpr std::uint32_t global=0x11000000u,it=0x11100000u,array=0x11200000u,bits=0x11300000u;
    m.put(0x5020a8u,global);m.put(global,rng());
    m.put(global+1u,rng.chance(8)?0u:1u+rng.pick(255),1);
    c.abi.gp[6]=it;
    std::int32_t count=std::int32_t(rng.pick(48));
    if(rng.chance(10))count=-std::int32_t(1+rng.pick(4));
    const std::uint32_t size=4u*(4u+rng.pick(12));
    std::uint32_t data=0x11400000u;
    // A hit element at address 0 takes the null exit (page 0 authored).
    const bool null_element=rng.chance(12);
    const auto null_index=count>0?rng.pick(std::uint32_t(count)):0u;
    if(null_element){data=0u-size*null_index;m.put(0,rng(),4);}
    m.put(array+0x20u,rng.chance(2)?0xffffffffu:rng());m.put(array+0x24u,size);
    m.put(array+0x38u,std::uint32_t(count));m.put(array+0x44u,data);m.put(array+0x48u,bits);
    const unsigned density=rng.pick(4);
    const auto words=std::uint32_t(std::max(count,0)+63)/32u;
    for(std::uint32_t i=0;i<words;++i) {
        std::uint32_t word=0;
        for(unsigned b=0;b<32;++b)if(density==0?rng.chance(9):density==1?rng.chance(2):density==2?!rng.chance(5):rng.chance(40))word|=1u<<b;
        m.put(bits+i*4u,word);
    }
    for(std::int32_t i=0;i<count;++i) {
        const auto element=data+size*std::uint32_t(i);
        if(!element)continue;
        m.put(element,rng());m.put(element+8u,(rng.chance(3)?0u:1u+rng.pick(255))<<8|rng.pick(256));
    }
    std::uint32_t start;
    switch(rng.pick(6)) {
    case 0:start=0xffffffffu;break;
    case 1:start=0u-2u-rng.pick(3);break;
    case 2:start=std::uint32_t(std::max(count,0))+rng.pick(3);break;
    case 3:start=0x7fffffffu;break;
    default:start=count>0?rng.pick(std::uint32_t(count)):rng.pick(3);break;
    }
    m.put(it,rng());m.put(it+4u,array);m.put(it+8u,rng());m.put(it+0xcu,start);
    m.put(it+0x10u,rng.chance(2)?0u:1u,1);m.put(it+0x11u,rng(),3);m.put(it+0x14u,rng());
    c.victims.insert(c.victims.end(),{0x5020a8u,global,it,array,bits,0x11400000u});
    return c;
}
Case author_1f7f60(Rng &rng) {
    Case c;common(c,rng);auto &m=c.memory;
    constexpr std::uint32_t it=0x11000000u,owner=0x11100000u,groups=0x11200000u;
    c.abi.gp[6]=it;
    std::int32_t group_count=std::int32_t(rng.pick(7));
    if(rng.chance(12))group_count=-1;
    m.put(it,owner);m.put(owner+0x1cu,std::uint32_t(group_count));m.put(owner+0x20u,groups);
    const unsigned allocated=8;
    std::vector<std::uint32_t> sub_counts(allocated);
    for(unsigned g=0;g<allocated;++g) {
        const auto subs=rng.chance(10)?0u:0x11300000u+g*0x400u;
        std::int32_t subs_count=std::int32_t(rng.pick(5));if(rng.chance(8))subs_count=-1;
        m.put(groups+g*12u,rng());m.put(groups+g*12u+4u,std::uint32_t(subs_count));m.put(groups+g*12u+8u,subs);
        sub_counts[g]=std::uint32_t(std::max(subs_count,0));
        if(subs)for(unsigned k=0;k<6;++k) {
            const auto sub=subs+k*20u;
            std::int32_t records=std::int32_t(rng.pick(5));if(rng.chance(6))records=-std::int32_t(rng.pick(2));
            m.put(sub,rng());m.put(sub+4u,std::uint32_t(records));
            const auto data=0x11500000u+g*0x2000u+k*0x200u;
            m.put(sub+8u,data);m.put(sub+12u,rng());m.put(sub+16u,rng());
            for(unsigned r=0;r<5;++r)m.put(data+r*0x44u,rng());
        }
    }
    for(unsigned g=0;g<allocated;++g) {
        std::int32_t pick=std::int32_t(rng.pick(sub_counts[g]+2u))-1;
        if(rng.chance(9))pick=std::int32_t(sub_counts[g]);
        m.put(it+4u+g,std::uint32_t(pick)&0xffu,1);
    }
    // +14 is the packed position; its upper bytes are the entry/sub/group.
    if(rng.chance(3))m.put(it+0x14u,0xffffffffu);
    else {m.put(it+0x14u,rng());m.put(it+0x15u,rng.pick(6),1);m.put(it+0x16u,rng.pick(6),1);m.put(it+0x17u,rng.pick(8),1);}
    m.put(it+0x18u,rng());
    if(rng.chance(9))m.put(groups+rng.pick(8)*12u+8u,0u);
    if(rng.chance(4)) {
        // In-place advance past entry 0x7F (the packed position's entry byte).
        const auto g=rng.pick(8),k=rng.pick(6),e=120u+rng.pick(130);
        m.put(it+0x14u,rng()&0xffu);m.put(it+0x15u,e,1);m.put(it+0x16u,k,1);m.put(it+0x17u,g,1);
        const auto subs=0x11300000u+g*0x400u,data=0x11500000u+g*0x2000u+k*0x200u;
        m.put(groups+g*12u+8u,subs);m.put(subs+k*20u+4u,e+rng.pick(4));m.put(subs+k*20u+8u,data);
        m.put(data+(e+1u)*0x44u,rng());
    }
    c.victims.insert(c.victims.end(),{it,owner,groups,0x11300000u,0x11500000u});
    return c;
}
Case author_c1e90(Rng &rng) {
    Case c;common(c,rng);auto &m=c.memory;
    constexpr std::uint32_t vector=0x11080000u;
    m.put(stack+4u,vector);
    std::uint32_t index=rng.pick(512);
    if(rng.chance(6))index=0u-1u-rng.pick(256);
    if(rng.chance(9))index=0x80000000u|rng.pick(64);
    c.abi.gp[2]=index;
    const auto word=vector+std::uint32_t(std::int32_t(index)>>5)*4u;
    m.put(word,rng.chance(4)?0u:rng.chance(4)?0xffffffffu:rng());
    c.victims.push_back(word);
    return c;
}
Case author_110350(Rng &rng) {
    Case c;common(c,rng);auto &m=c.memory;
    const bool specials=rng.chance(3);
    const auto handle=(rng()&0xffff0000u)|rng.pick(6);c.abi.gp[0]=handle;
    const auto unit=0x11200000u+rng.pick(4)*0x800u;
    object_pool(c,rng,handle,unit);
    constexpr std::uint32_t tags=0x11600000u,definition=0x11700000u,random=0x11800000u;
    const auto tag=rng();m.put(unit,tag);
    const std::int16_t offsets[]{0,0x20,0x400,-0x80};
    const auto offset=std::uint32_t(std::int32_t(offsets[rng.pick(4)]));
    m.put(unit+0x33eu,offset&0xffffu,2);
    m.put(0x4f0414u,tags);m.put((tag&0xffffu)*16u+tags+8u,definition);
    m.put(definition+0x1e8u,rng.chance(3)?0u:rng.chance(4)?0xbf800000u:rng.value(-2.f,2.f,specials));
    const auto object=unit+offset;
    m.put(object+0x38u,rng.chance(2)?0xffffffffu:rng());
    m.put(object+0x36u,rng.chance(3)?0xffffu:rng.pick(0x10000),2);
    m.put(object+0x40u,rng.chance(8)?0u:rng.value(-1.5f,1.5f,specials));
    for(unsigned i=0;i<12;++i)m.put(object+0x50u+i,rng.chance(4)?0u:rng.chance(6)?0x80u|rng.pick(2):rng(),1);
    m.put(object+2u,rng.chance(2)?0u:rng.chance(5)?0xffu:rng(),1);
    m.put(object+0x5cu,rng());
    // The XBE constants (authored, so the special-float paths are reachable).
    const std::uint32_t constants[]{0x46a0ecu,0x46a5d0u,0x44a8a0u,0x46a0d0u,0x44a98cu,0x5564d8u,0x46a194u,0x46a270u};
    const float defaults[]{1.f,-0.033f,0.033f,1.f,0.05f,1.f/127.f,1.f,127.f};
    for(unsigned i=0;i<8;++i)m.put(constants[i],specials&&rng.chance(4)?rng.special():
        rng.chance(2)?std::bit_cast<std::uint32_t>(defaults[i]):rng.real(-2.f,200.f));
    switch(rng.pick(7)) {
    case 0: {
        // Ceiling compare equal: value+[44A8A0] = 0 against [46A0D0] = -0.
        const auto v=rng.real(0.25f,1.5f);
        m.put(object+0x38u,rng()&0x7fffffffu);m.put(object+0x40u,v);m.put(0x44a8a0u,v^0x80000000u);
        m.put(0x46a0d0u,rng.chance(2)?0x80000000u:0u);m.put(0x46a0ecu,rng.chance(2)?0xff800000u:std::bit_cast<std::uint32_t>(0.125f));
        break;
    }
    case 1: {
        // Decay compare equal: value-[44A98C] is a signed zero.
        const auto v=rng.chance(3)?0x80000000u:rng.real(-1.f,1.f);
        m.put(object+0x38u,0xffffffffu);m.put(object+0x40u,v);m.put(0x44a98cu,rng.chance(3)?0u:v);
        break;
    }
    default:break;
    }
    m.put(0x4f3cd8u,random);m.put(random,rng());m.put(random+4u,rng());
    c.victims.insert(c.victims.end(),{unit,tags,definition,random,0x4f0414u,object+0x50u});
    return c;
}
Case author_243f20(Rng &rng) {
    Case c;common(c,rng);auto &m=c.memory;
    constexpr std::uint32_t part=0x11000000u,descriptor=0x11100000u,bounds=0x11400000u,node=0x11500000u;
    // The XBE: the jump table, the default point and the four constants.
    const std::uint32_t table[]{0x243f41u,0x243f62u,0x24403au,0x244070u};
    for(unsigned i=0;i<4;++i)m.put(0x24417cu+i*4u,table[i]);
    m.put(0x474ce8u,0x44abecu);
    const bool specials=rng.chance(3);
    for(unsigned i=0;i<3;++i)m.put(0x44abecu+i*4u,rng.chance(2)?0u:rng.value(-50.f,50.f,specials));
    const std::uint32_t constants[]{0x46a11cu,0x46a0d0u,0x46a2e0u,0x46a0ccu};
    const float defaults[]{2.f,1.f,1.f/131070.f,0.5f};
    for(unsigned i=0;i<4;++i)m.put(constants[i],rng.chance(5)?rng.value(-4.f,4.f,specials):std::bit_cast<std::uint32_t>(defaults[i]));
    const std::uint32_t layouts[]{0u,1u,2u,3u,4u,5u,0xffu,0x80u};
    const auto layout=rng.chance(5)?layouts[rng.pick(8)]:1u+rng.pick(4);
    c.abi.gp[2]=part;m.put(part+0x3cu,descriptor);m.put(part,rng());
    m.put(descriptor,layout|(rng()&0xffffff00u));m.put(descriptor+4u,rng());
    const auto base=0x11200000u+rng.pick(64)*4u,offset=rng.chance(4)?rng.pick(8):rng.pick(16)*4u;
    m.put(descriptor+8u,base);m.put(descriptor+0xcu,offset);
    // Vertices: floats (with specials) or words, and node bytes.
    const auto data=base+offset;
    for(std::uint32_t at=data&~3u;at<data+0x200u;at+=4)m.put(at,rng.chance(2)?rng.value(-100.f,100.f,specials):rng());
    for(unsigned i=0;i<6;++i)m.put(bounds+i*4u,rng.value(-200.f,200.f,specials));
    if(rng.chance(4))m.put(bounds+4u,rng()&0x807fffffu);  // equal or denormal extents
    c.abi.gp[0]=bounds;
    std::uint32_t index=rng.pick(20);
    if(rng.chance(16))index=0x40000000u+rng.pick(4);  // an address that wraps
    m.put(stack+4u,index);
    const auto point=0x11300000u+rng.pick(16)*4u+(rng.chance(6)?rng.pick(4):0u);
    m.put(point,rng());m.put(point+4u,rng());m.put(point+8u,rng());c.abi.gp[1]=point;
    c.abi.gp[7]=rng.chance(3)?0u:node+rng.pick(8)*4u;m.put(node,rng());m.put(node+0x20u,rng());
    if(rng.chance(10))c.abi.gp[1]=bounds+rng.pick(3)*4u;  // the point aliases the bounds
    c.victims.insert(c.victims.end(),{part,descriptor,data,bounds,point,node,0x44abecu,0x46a11cu});
    return c;
}
// A long double from its 64-bit significand and sign/exponent word.
long double extended(std::uint64_t significand,std::uint16_t exponent) {
    long double x{};std::memcpy(&x,&significand,8);std::memcpy(reinterpret_cast<char*>(&x)+8,&exponent,2);return x;
}
long double conversion_value(Rng &rng) {
    switch(rng.pick(14)) {
    case 0:return (long double)(std::int64_t(rng())-0x80000000ll);
    case 1:return (long double)(int(rng.pick(2001))-1000)+0.5L;
    case 2:return (long double)(int(rng.pick(2001))-1000)/7.0L;
    case 3:return extended(0x8000000000000000ull,rng.chance(2)?0x401fu:0xc01fu);  // +-2^32
    case 4:return (long double)((std::int64_t(rng())<<31)|rng())*(rng.chance(2)?1:-1);
    case 5:return extended(0xc000000000000000ull|(std::uint64_t(rng())<<20),0x7fffu|(rng.chance(2)?0x8000u:0u));  // quiet NaN
    case 6:return extended(0x8000000000000000ull,rng.chance(2)?0x7fffu:0xffffu);  // infinity
    case 7:return extended(0x8000000000000000ull|rng(),std::uint16_t(0x403eu+rng.pick(3))|(rng.chance(2)?0x8000u:0u));  // >= 2^63
    case 8:return rng.chance(2)?0.0L:-0.0L;
    case 9:return extended(std::uint64_t(rng())|1u,rng.chance(2)?0u:0x8000u);  // denormal
    case 10:return extended(0x8000000000000000ull|(std::uint64_t(rng())<<32)|rng(),std::uint16_t(0x3ffeu+rng.pick(64))|(rng.chance(2)?0x8000u:0u));
    case 11:return (long double)(std::int64_t(rng.pick(5))-2)*(1.0L-1.0L/(1ll<<40));
    case 12:return extended(0xa000000000000000ull|rng(),0x7fffu);  // signalling NaN
    default:return (long double)(int(rng.pick(41))-20)*0.25L;
    }
}
Case author_32cc0c(Rng &rng) {
    Case c;common(c,rng);
    unsigned depth=1u+rng.pick(7);
    if(rng.chance(40))depth=0u;else if(rng.chance(40))depth=8u;
    c.abi.x87_depth=depth;
    for(unsigned i=0;i<depth;++i)c.abi.x87[i]=i?(long double)(int(rng()%2001)-1000)/7.0L:conversion_value(rng);
    const unsigned top=(8u-depth+rng.pick(8))&7u;
    c.abi.x87_status=std::uint16_t((top<<11)|(rng()&0x4700u));
    return c;
}
Case author_16e600(Rng &rng) {
    Case c;common(c,rng);auto &m=c.memory;
    const auto array=0x11000000u+rng.pick(8)*0x100u+(rng.chance(3)?rng.pick(4):0u);
    c.abi.gp[6]=array;
    for(std::uint32_t at=array&~3u;at<array+0x50u;at+=4)m.put(at,rng());
    // The name: the first character may end it; the second may be zero.
    m.put(array,rng.chance(4)?0u:1u+rng.pick(255),1);m.put(array+1u,rng.chance(4)?0u:rng.pick(256),1);
    m.put(array+0x2au,(rng.chance(2)?8u:0u)|(rng.pick(256)&~8u),1);
    std::int32_t count=std::int32_t(rng.pick(40));
    if(rng.chance(10))count=-std::int32_t(1+rng.pick(31));
    std::uint32_t size=rng.chance(3)?4u*(1u+rng.pick(12)):rng.pick(50);
    if(count<0 && (m.page(array+0x2au,false)[(array+0x2au)&4095u]&std::byte{8})!=std::byte{0})size=0;
    const auto data=0x11200800u+rng.pick(16)*4u+(rng.chance(4)?rng.pick(4):0u);
    const auto bits=0x11300800u+rng.pick(16)*4u+(rng.chance(6)?rng.pick(4):0u);
    m.put(array+0x20u,std::uint32_t(count));m.put(array+0x24u,size);m.put(array+0x44u,data);m.put(array+0x48u,bits);
    for(std::uint32_t at=(data&~3u)-0x800u;at<data+0x800u;at+=4)m.put(at,rng());
    for(std::uint32_t at=(bits&~3u)-0x40u;at<bits+0x40u;at+=4)m.put(at,rng());
    if(rng.chance(10) && count>0 && count<24) {
        // The datums alias the header: the salt loop's re-reads see its stores.
        m.put(array+0x44u,array+rng.pick(4)*4u);m.put(array+0x24u,4u*(1u+rng.pick(3)));
        m.put(array+0x2au,0u,1);
    }
    if(rng.chance(6))c.abi.eflags|=0x400u;  // a set direction flag steps the fills down
    c.victims.insert(c.victims.end(),{array,data,bits});
    return c;
}
Case author_171010(Rng &rng) {
    Case c;common(c,rng);auto &m=c.memory;
    constexpr std::uint32_t table=0x11000000u,cleared=0x11200000u,index_out=0x11300000u,offset_out=0x11400000u;
    std::int32_t count=std::int32_t(rng.pick(12));
    if(rng.chance(10))count=-std::int32_t(1+rng.pick(3));
    const auto records=0x11100000u+rng.pick(16)*4u+(rng.chance(5)?2u:0u);
    m.put(table,std::uint32_t(count));m.put(table+4u,records);
    // Intervals: increasing or overlapping, empty, or reaching past 0xFFFF.
    std::uint32_t first=rng.pick(40);
    for(unsigned i=0;i<14;++i) {
        const auto record=records+i*0x48u;
        for(std::uint32_t at=record&~3u;at<record+0x48u;at+=4)m.put(at,rng());
        const auto length=rng.chance(6)?0u:rng.chance(12)?0xffffu:1u+rng.pick(30);
        m.put(record+6u,first&0xffffu,2);m.put(record+8u,length,2);
        first=rng.chance(5)?rng.pick(0x10000u):first+length+(rng.chance(3)?rng.pick(5):0u);
    }
    std::uint32_t key=rng.pick(400);
    if(rng.chance(8))key=0u-1u-rng.pick(100);
    if(rng.chance(12))key=0x10000u+rng.pick(0x20000u);
    c.abi.gp[1]=key;c.abi.gp[3]=table;
    c.abi.gp[0]=cleared+rng.pick(4)*4u;
    if(rng.chance(12))c.abi.gp[0]=table;          // the clear empties the table
    if(rng.chance(16))c.abi.gp[0]=stack+8u;       // the clear zeroes the offset pointer
    m.put(cleared,rng());m.put(index_out,rng());m.put(offset_out,rng());
    m.put(stack+4u,index_out+rng.pick(4)*4u);m.put(stack+8u,offset_out+rng.pick(4)*4u);
    if(rng.chance(10))m.put(stack+8u,table);  // a match overwrites the count before its re-read
    if(count>0 && rng.chance(8)) {
        // The match at index i stores the offset i over the count: the
        // re-read count then equals the index, which reports no record.
        const auto i=rng.pick(std::uint32_t(count));
        for(unsigned k=0;k<i;++k)m.put(records+k*0x48u+6u,0xfff0u,2);
        const auto start=rng.pick(50);
        m.put(records+i*0x48u+6u,start,2);m.put(records+i*0x48u+8u,i+1u+rng.pick(4),2);
        c.abi.gp[1]=start+i;m.put(stack+8u,table);c.abi.gp[0]=cleared;
    }
    c.victims.insert(c.victims.end(),{table,records,cleared,index_out,offset_out});
    return c;
}
Case author_1211c0(Rng &rng) {
    Case c;common(c,rng);auto &m=c.memory;
    constexpr std::uint32_t point=0x11000000u,v0=0x11000100u,v1=0x11000200u,v2=0x11000300u,u_out=0x11100000u,v_out=0x11100040u;
    const bool specials=rng.chance(6);
    const auto f=[&](float lo,float hi){return std::bit_cast<float>(rng.value(lo,hi,specials));};
    float a[3],e1[3],e2[3],p[3];
    for(unsigned i=0;i<3;++i){a[i]=f(-50.f,50.f);e1[i]=f(-8.f,8.f);e2[i]=f(-8.f,8.f);}
    if(rng.chance(6)) {for(unsigned i=0;i<3;++i)e2[i]=e1[i]*(rng.chance(2)?2.f:-0.5f);}  // degenerate
    if(rng.chance(8))e1[rng.pick(3)]=0.f;
    if(rng.chance(8)){e1[2]=e2[2]=0.f;}  // a flat triangle: the normal is the z axis
    // The point: on the plane at barycentric (s, t), with s, t around [0, 1]
    // and near the edges, or off the plane, or anywhere.
    const float s=rng.chance(5)?0.f:float(int(rng.pick(31))-5)/20.f,t=rng.chance(5)?1.f-s:float(int(rng.pick(31))-5)/20.f;
    const float lift=rng.chance(2)?0.f:rng.chance(2)?1e-5f:float(int(rng.pick(200))-100)/50.f;
    for(unsigned i=0;i<3;++i)p[i]=a[i]+s*e1[i]+t*e2[i]+lift*(i==2?1.f:0.f);
    if(rng.chance(10))for(auto &x:p)x=f(-60.f,60.f);
    for(unsigned i=0;i<3;++i){
        m.put(point+i*4u,std::bit_cast<std::uint32_t>(p[i]));m.put(v0+i*4u,std::bit_cast<std::uint32_t>(a[i]));
        m.put(v1+i*4u,std::bit_cast<std::uint32_t>(a[i]+e1[i]));m.put(v2+i*4u,std::bit_cast<std::uint32_t>(a[i]+e2[i]));
    }
    if(rng.chance(4))m.put(v1+rng.pick(3)*4u,rng.special());
    c.abi.gp[6]=point;c.abi.gp[0]=v0;c.abi.gp[1]=v1;c.abi.gp[2]=v2;
    if(rng.chance(16))c.abi.gp[1]=v0;  // a repeated vertex
    m.put(u_out,rng());m.put(v_out,rng());
    m.put(stack+4u,u_out+rng.pick(4)*4u);m.put(stack+8u,rng.chance(16)?u_out:v_out+rng.pick(4)*4u);
    // XBE constants and the projection table.
    const std::uint32_t constants[]{0x46a0ecu,0x46a0e8u,0x46a30cu,0x46a0d0u};
    const float defaults[]{1e-4f,0.f,-1e-4f,1.f};
    for(unsigned i=0;i<4;++i)m.put(constants[i],rng.chance(12)?rng.value(-1.f,1.f,true):std::bit_cast<std::uint32_t>(defaults[i]));
    const std::int16_t table[]{2,1,0,1,2,0,0,2,1,2,0,1,1,0,2,0,1,2};
    for(unsigned i=0;i<18;++i)m.put(0x44ac8cu+i*2u,std::uint16_t(rng.chance(40)?std::int16_t(rng.pick(3)):table[i]),2);
    c.victims.insert(c.victims.end(),{point,u_out,0x46a0ecu,0x44ac8cu});
    return c;
}
Case author_1f7e90(Rng &rng) {
    Case c;common(c,rng);auto &m=c.memory;
    constexpr std::uint32_t source=0x11100000u,remap=0x11200000u,records=0x11300000u,choices=0x11400000u;
    auto it=0x11000000u+rng.pick(8)*4u+(rng.chance(6)?rng.pick(4):0u);
    // The iterator in the caller's frame: a selection store with a negative
    // group byte reaches the argument slots (the loop's count and iterator).
    const bool on_stack=rng.chance(10);
    if(on_stack)it=stack+0xcu+rng.pick(6)*4u;
    for(std::uint32_t at=stack+0x20u;at<stack+0x40u;at+=4)m.put(at,rng());
    for(std::uint32_t at=it&~3u;at<it+0x20u;at+=4)m.put(at,rng());
    c.abi.gp[0]=rng();
    c.abi.gp[3]=rng.chance(5)?0u:source+rng.pick(4)*4u+(rng.chance(5)?rng.pick(4):0u);
    m.put(stack+4u,it);
    m.put(stack+8u,rng.chance(4)?0u:remap);
    for(std::uint32_t at=source;at<source+0x30u;at+=4) {
        std::uint32_t word=0;
        for(unsigned k=0;k<4;++k)word|=(rng.chance(8)?0x80u+rng.pick(128):rng.chance(6)?0xffu:rng.pick(6))<<(8*k);
        m.put(at,word);
    }
    std::int32_t count=std::int32_t(rng.pick(22));
    if(rng.chance(10))count=-std::int32_t(1+rng.pick(3));
    if(rng.chance(8))count=16+std::int32_t(rng.pick(2));
    m.put(remap+0x70u,std::uint32_t(count));m.put(remap+0x74u,records);m.put(remap,rng());
    for(unsigned i=0;i<24;++i) {
        const auto record=records+i*16u;
        m.put(record,rng());
        std::uint32_t group=rng.pick(16);
        if(rng.chance(6))group=0xffu;
        if(rng.chance(12))group=0x80u+rng.pick(128);  // a negative group byte
        if(on_stack && rng.chance(4))group=std::uint32_t(std::int32_t(stack+4u+rng.pick(2)*4u)-std::int32_t(it+4u))&0xffu;
        m.put(record+4u,group|(rng()&0xffffff00u));
        std::int32_t choice_count=std::int32_t(rng.pick(7));if(rng.chance(9))choice_count=-1;
        m.put(record+8u,std::uint32_t(choice_count));
        const auto list=choices+i*0x40u;m.put(record+12u,list);
        for(unsigned k=0;k<8;++k) {
            m.put(list+k*8u,rng());
            m.put(list+k*8u+4u,(rng()&0xffff00ffu)|((rng.chance(5)?0xffu:rng.pick(6))<<8));
        }
    }
    c.victims.insert(c.victims.end(),{0x11000000u,source,remap,records,choices});
    return c;
}
// A string of a random length (a terminator within limit, or none), with
// bytes that trip the runtime's zero-byte test without being zero.
void author_string(Case &c,Rng &rng,std::uint32_t at,std::uint32_t limit) {
    const auto length=rng.chance(6)?limit+rng.pick(8):rng.pick(limit+1u);
    for(std::uint32_t i=0;i<limit+12u;++i) {
        std::uint32_t byte;
        switch(rng.pick(8)) {
        case 0:byte=0x80u;break;case 1:byte=0x01u;break;case 2:byte=0xffu;break;case 3:byte=0x7fu;break;
        case 4:byte=0x81u;break;default:byte=1u+rng.pick(255);break;
        }
        c.memory.put(at+i,i==length?0u:byte,1);
    }
}
Case author_32a4e0(Rng &rng) {
    Case c;common(c,rng);auto &m=c.memory;
    const auto source=0x11000000u+rng.pick(8)*4u+rng.pick(4);
    auto destination=0x11100000u+rng.pick(8)*4u+rng.pick(4);
    std::uint32_t count=rng.chance(10)?0u:rng.chance(8)?1u+rng.pick(3):rng.pick(72);
    author_string(c,rng,source,count?count+rng.pick(4):8u);
    for(std::uint32_t at=destination&~3u;at<destination+count+8u;at+=4)m.put(at,rng());
    if(rng.chance(12))destination=source+rng.pick(9)-4u;  // overlapping copies keep their order
    if(!overlay_mode && rng.chance(20)){count=0x40000000u+rng.pick(4);}  // runs off the authored pages
    m.put(stack+4u,destination);m.put(stack+8u,source);m.put(stack+12u,count);
    c.victims.insert(c.victims.end(),{source,destination,0x11100000u});
    return c;
}
Case author_16e4b0(Rng &rng) {
    Case c;common(c,rng);auto &m=c.memory;
    const auto array=0x11000000u+rng.pick(8)*0x80u+(rng.chance(4)?rng.pick(4):0u);
    const auto name=0x11100000u+rng.pick(8)*4u+rng.pick(4);
    for(std::uint32_t at=array&~3u;at<array+0x54u;at+=4)m.put(at,rng());
    author_string(c,rng,name,32u);
    c.abi.gp[6]=array;
    for(unsigned i=0;i<6;++i)m.put(stack+4u+i*4u,rng());
    m.put(stack+4u,name);
    if(rng.chance(6))c.abi.eflags|=0x400u;
    if(rng.chance(6))for(std::uint32_t at=array-0x50u;at<array;at+=4)m.put(at,rng());
    c.victims.insert(c.victims.end(),{array,name});
    return c;
}
Case author_16e450(Rng &rng) {
    Case c;common(c,rng);auto &m=c.memory;
    const auto block=0x11000000u+rng.pick(8)*0x40u+(rng.chance(4)?rng.pick(4):0u);
    const auto name=0x11200000u+rng.pick(8)*4u+rng.pick(4);
    author_string(c,rng,name,32u);
    c.abi.gp[1]=(rng()&0xffffff00u)|(rng.chance(4)?rng.pick(64):rng.pick(5));
    std::int32_t count=std::int32_t(rng.pick(96));
    if(!overlay_mode && rng.chance(12))count=-std::int32_t(1+rng.pick(40));  // a bit vector fill off the pages
    c.abi.gp[3]=std::uint32_t(count);c.abi.gp[2]=rng.chance(8)?0u:rng.pick(40);
    for(std::uint32_t at=block&~3u;at<block+0x1800u;at+=4)m.put(at,rng());
    m.put(stack+4u,block);m.put(stack+8u,name);m.put(stack+12u,rng());
    if(rng.chance(6))c.abi.eflags|=0x400u;
    c.victims.insert(c.victims.end(),{block,block+0x1000u,name});
    return c;
}
Case author_22a790(Rng &rng) {
    Case c;common(c,rng);auto &m=c.memory;
    constexpr std::uint32_t owner=0x11000000u,references=0x11100000u,block=0x11200000u,holders=0x11300000u,busy=0x11400000u;
    c.abi.gp[6]=owner;m.put(stack+4u,block);
    std::int32_t count=std::int32_t(rng.pick(9));if(rng.chance(10))count=-std::int32_t(rng.pick(3));
    m.put(owner+0x10u,std::uint32_t(count));m.put(owner+0x14u,references+rng.pick(4)*4u);m.put(owner,rng());
    // Busy words: mostly idle, some busy, some negative.
    for(unsigned i=0;i<16;++i)m.put(busy+i*4u,(rng.chance(3)?1u+rng.pick(4):rng.chance(5)?0x80000000u|rng():0u)|(rng.chance(4)?0x10u|(rng.pick(8)<<8):0u));
    const auto resource=[&]{return rng.chance(4)?0u:busy+rng.pick(16)*4u;};
    for(std::uint32_t at=block;at<block+0x80u;at+=4)m.put(at,rng());
    for(unsigned i=0;i<6;++i){m.put(block+8u+i*8u+4u,holders+i*0x100u);m.put(block+8u+i*8u+0x10u,resource());}
    for(unsigned i=0;i<6;++i)for(unsigned k=0;k<6;++k){m.put(holders+i*0x100u+k*0x20u,rng());m.put(holders+i*0x100u+k*0x20u+0x10u,resource());}
    for(unsigned i=0;i<12;++i) {
        const auto entry=references+rng.pick(4)*4u+i*16u;
        m.put(entry,(rng()&0xffffff00u)|(rng.chance(4)?rng.pick(4):2u));
        m.put(entry+4u,(rng.pick(6)*8u)|((rng.chance(2)?0xffffu:rng.pick(6))<<16));
        m.put(entry+8u,rng());m.put(entry+12u,rng.chance(5)?0xffffffffu:rng());
    }
    c.victims.insert(c.victims.end(),{owner,references,block,holders,busy});
    return c;
}
Case author_1f8200(Rng &rng) {
    Case c;common(c,rng);auto &m=c.memory;
    constexpr std::uint32_t instance=0x11000000u,owner=0x11100000u,groups=0x11200000u,permutations=0x11300000u,bsps=0x11500000u,
        source=0x11700000u,remap=0x11800000u,records=0x11810000u,choices=0x11820000u,ray=0x11900000u,result=0x11a00000u;
    // The node matrices at a varying dword offset (the flags of the ADD that
    // indexes them reach 144090).
    const auto matrices=0x11600000u+rng.pick(64)*4u;
    // The routine's frame and its children's calls reach below the common stack.
    for(std::uint32_t at=stack-0x100u;at<stack-0x80u;at+=4)m.put(at,rng());
    m.put(0x11f00000u,rng());  // the stand-ins' entry-register digest
    const bool specials=rng.chance(3);
    const auto real=[&](float lo,float hi){return rng.value(lo,hi,specials);};
    m.put(stack+4u,instance);m.put(stack+8u,rng());m.put(stack+12u,ray);m.put(stack+16u,ray+0x20u);m.put(stack+20u,result);
    m.put(instance,rng());m.put(instance+4u,rng.chance(3)?remap:0u);m.put(instance+8u,owner);
    m.put(instance+12u,rng.chance(5)?0u:source);m.put(instance+16u,matrices);
    // The model: regions -> permutations -> BSPs (a node byte, then the BSP).
    std::int32_t regions=std::int32_t(rng.pick(5));if(rng.chance(12))regions=-1;
    m.put(owner,rng());m.put(owner+0x1cu,std::uint32_t(regions));m.put(owner+0x20u,groups);
    for(unsigned g=0;g<6;++g) {
        const auto list=permutations+g*0x400u;
        m.put(groups+g*12u,rng());m.put(groups+g*12u+4u,rng.pick(4));m.put(groups+g*12u+8u,rng.chance(12)?0u:list);
        for(unsigned k=0;k<4;++k) {
            const auto permutation=list+k*20u,data=bsps+g*0x1000u+k*0x400u;
            std::int32_t count=std::int32_t(rng.pick(4));if(rng.chance(8))count=-1;
            m.put(permutation,rng());m.put(permutation+4u,std::uint32_t(count));m.put(permutation+8u,data);
            m.put(permutation+12u,rng());m.put(permutation+16u,rng());
            for(unsigned e=0;e<4;++e) {
                // A node past the authored matrices faults inside 144090.
                const auto node=rng.chance(40)?0x40u+rng.pick(0xc0u):rng.pick(12);
                m.put(data+e*0x44u,(rng()&0xffffff00u)|node);m.put(data+e*0x44u+4u,rng());
            }
        }
    }
    for(std::uint32_t at=source;at<source+0x10u;at+=4) {
        std::uint32_t word=0;
        for(unsigned k=0;k<4;++k)word|=(rng.chance(6)?0xffu:rng.pick(4))<<(8*k);
        m.put(at,word);
    }
    std::int32_t count=std::int32_t(rng.pick(8));if(rng.chance(10))count=-1;
    m.put(remap,rng());m.put(remap+0x70u,std::uint32_t(count));m.put(remap+0x74u,records);
    for(unsigned i=0;i<8;++i) {
        const auto record=records+i*16u,list=choices+i*0x40u;
        m.put(record,rng());m.put(record+4u,(rng()&0xffffff00u)|(rng.chance(6)?0xffu:rng.pick(6)));
        m.put(record+8u,rng.pick(5));m.put(record+12u,list);
        for(unsigned k=0;k<8;++k){m.put(list+k*8u,rng());m.put(list+k*8u+4u,(rng()&0xffff00ffu)|((rng.chance(6)?0xffu:rng.pick(4))<<8));}
    }
    // Node matrices: the scale is often exactly 1.0 (the unscaled path).
    for(unsigned n=0;n<16;++n) {
        const auto row=matrices+n*0x34u;
        m.put(row,rng.chance(2)?0x3f800000u:real(-2.0f,2.0f));
        for(unsigned i=1;i<13;++i)m.put(row+i*4u,real(-2.0f,2.0f));
    }
    for(unsigned i=0;i<3;++i){m.put(ray+i*4u,real(-50.0f,50.0f));m.put(ray+0x20u+i*4u,real(-10.0f,10.0f));}
    for(std::uint32_t at=result;at<result+0x40u;at+=4)m.put(at,rng());
    // The constants: the initial fraction (here, through the stand-ins'
    // hits, and in the result) and 1.0, occasionally perturbed.
    constexpr std::uint32_t fractions[]{0x3f800000u,0x7f7fffffu,0x3f000000u,0xbf000000u,0x80000000u,0u,0x7fc00001u,0x3fc00000u};
    m.put(0x45695cu,rng.chance(3)?real(-0.5f,1.5f):fractions[rng.pick(8)]);
    m.put(0x46a0d0u,rng.chance(12)?real(0.5f,1.5f):0x3f800000u);
    c.victims.insert(c.victims.end(),{instance,owner,groups,permutations,bsps,0x11600000u,source,remap,records,ray,result});
    return c;
}
Case author_138d90(Rng &rng) {
    Case c;common(c,rng);auto &m=c.memory;
    const auto bitmap=0x11000000u+rng.pick(16)*4u,coordinate=0x11100000u+rng.pick(16)*4u;
    const bool specials=rng.chance(3);
    for(std::uint32_t at=bitmap;at<bitmap+0x60u;at+=4)m.put(at,rng());
    c.abi.gp[0]=bitmap;m.put(stack+4u,coordinate);
    m.put(0x11f00000u,rng());  // the stand-ins' entry-register digest
    // Dimensions: powers of two, others, 1, 0, negative words, and widths
    // that round up past 0x7FFF.
    const auto dimension=[&]()->std::uint32_t {
        switch(rng.pick(7)) {
        case 0:return 1u<<rng.pick(12);
        case 1:return 1u+rng.pick(700);
        case 2:return rng.pick(2);
        case 3:return 0x8000u|rng.pick(0x8000u);
        case 4:return 0x7ffcu+rng.pick(4);
        case 5:return (1u<<rng.pick(10))*(3u+2u*rng.pick(3));
        default:return rng.pick(0x10000u);
        }
    };
    m.put(bitmap+4u,dimension(),2);m.put(bitmap+6u,dimension(),2);
    constexpr std::uint16_t types[]{0,1,2,3,0xffffu,0x8000u};
    m.put(bitmap+0xau,types[rng.pick(6)],2);
    m.put(bitmap+0xeu,(rng()&0xfffdu)|(rng.chance(2)?2u:0u),2);
    m.put(bitmap+0x14u,rng.chance(5)?(rng.chance(2)?0u:0x8000u|rng.pick(0x8000u)):rng.pick(14),2);
    m.put(bitmap+0x54u,rng.chance(8)?0u:0x11400000u+rng.pick(64)*4u);
    // The level of detail in XMM1 (exactly 1.0 sometimes), and (u, v),
    // with values past the int32 range for the FISTP's indefinite.
    c.abi.xmm[1][0]=rng.chance(6)?0x3f800000u:rng.value(-1.0f,2.0f,specials);
    for(unsigned i=0;i<2;++i)m.put(coordinate+i*4u,rng.chance(12)?(rng.chance(2)?0x4f800000u:0xd0800000u):rng.value(-3.0f,3.0f,specials));
    m.put(coordinate+8u,rng());
    m.put(0x46a0d0u,rng.chance(12)?rng.real(0.5f,1.5f):0x3f800000u);
    m.put(0x46a0ccu,rng.chance(12)?rng.real(0.0f,1.0f):0x3f000000u);
    // A full x87 stack faults at the first FLD.
    if(rng.chance(16)){c.abi.x87_depth=8;for(unsigned i=0;i<8;++i)c.abi.x87[i]=(long double)(int(rng()%2001)-1000)/7.0L;}
    c.victims.insert(c.victims.end(),{bitmap,coordinate});
    return c;
}
Case author_179cd0(Rng &rng) {
    Case c;common(c,rng);auto &m=c.memory;
    constexpr std::uint32_t globals=0x11000000u,first=0x11100000u,second=0x11200000u;
    m.put(0x11f00000u,rng());m.put(0x11f00004u,rng.chance(2)?second:first);
    m.put(0x51df54u,globals);m.put(globals,rng());m.put(globals+4u,rng.value(0.0f,0.1f,rng.chance(4)));m.put(globals+8u,rng());
    m.put(0x4f720cu,first);
    for(const auto array:{first,second}) {
        std::int32_t count=std::int32_t(rng.pick(24));
        if(rng.chance(12))count=-std::int32_t(rng.pick(3));
        const auto size=rng.chance(6)?0x100u+rng.pick(0x100u)*4u:0x190u;
        std::uint32_t data=array+0x10000u;
        // A datum at address 0 (its address test skips the advance).
        const bool null_datum=!overlay_mode && rng.chance(12) && count>0;
        const auto null_index=null_datum?rng.pick(std::uint32_t(count)):0u;
        if(null_datum)data=0u-0x190u*null_index;
        const auto bits=array+0x8000u;
        for(std::uint32_t at=array;at<array+0x50u;at+=4)m.put(at,rng());
        m.put(array+0x24u,size);m.put(array+0x38u,std::uint32_t(count));m.put(array+0x44u,data);m.put(array+0x48u,bits);
        const unsigned density=rng.pick(3);
        for(unsigned i=0;i<2;++i) {
            std::uint32_t word=0;
            for(unsigned k=0;k<32;++k)if(density==0?rng.chance(2):density==1?!rng.chance(6):rng.chance(8))word|=1u<<k;
            m.put(bits+i*4u,word);
        }
        for(std::int32_t i=0;i<std::max(count,0);++i) {
            const auto datum=data+std::uint32_t(i)*0x190u,salt_at=data+std::uint32_t(i)*size;
            for(const auto at:{datum,datum+0x10u,datum+0x14u,datum+0x18u,datum+0xfcu})m.put(at,rng());
            m.put(datum+2u,(rng.chance(3)?0x40u:0u)|(rng()&0xbfu),1);
            if(rng.chance(2))m.put(datum+0xfcu,0xffffffffu);
            m.put(salt_at,rng.chance(8)?0xffffu:rng(),2);
        }
    }
    c.victims.insert(c.victims.end(),{globals,first,second,first+0x10000u,second+0x10000u,first+0x8000u});
    return c;
}
Case author(Leaf leaf,Rng &rng) {
    Case c;
    switch(leaf) {
    case c9550:c=author_c9550(rng);break;
    case l1ec970:c=author_1ec970(rng);break;
    case l1f7f60:c=author_1f7f60(rng);break;
    case c1e90:c=author_c1e90(rng);break;
    case l110350:c=author_110350(rng);break;
    case l243f20:c=author_243f20(rng);break;
    case l32cc0c:case l32cc0c_tracked:c=author_32cc0c(rng);break;
    case l16e600:c=author_16e600(rng);break;
    case l171010:c=author_171010(rng);break;
    case l1f7e90:c=author_1f7e90(rng);break;
    case l32a4e0:c=author_32a4e0(rng);break;
    case l16e4b0:c=author_16e4b0(rng);break;
    case l16e450:c=author_16e450(rng);break;
    case l22a790:c=author_22a790(rng);break;
    case l1f8200:c=author_1f8200(rng);break;
    case l138d90:case l138d90_untracked:c=author_138d90(rng);break;
    case l179cd0:c=author_179cd0(rng);break;
    case l16e450_pool:c=author_16e450(rng);break;
    case l16e4b0_pool:c=author_16e4b0(rng);break;
    case l16e600_pool:c=author_16e600(rng);break;
    case l32a4e0_pool:c=author_32a4e0(rng);break;
    default:c=author_1211c0(rng);break;
    }
    c.memory.put(stack,0x18d9b1u);
    if(rng.chance(16) && !c.victims.empty())c.memory.pages.erase(c.victims[rng.pick(std::uint32_t(c.victims.size()))]>>12);
    return c;
}
template<class Kernel> bool native_kernel(WorldState &s,Kernel kernel,void(*reference)(WorldState&)) {
    return leaves::run(s,kernel,[reference](auto &r){reference(r);},0x0u);
}
bool native_route(Leaf leaf,WorldState &s) {
    switch(leaf) {
    case c9550:return native_kernel(s,[](auto &io,auto &m){leaves::kernel_c9550(io,m);},original::run_c9550);
    case l1ec970:return native_kernel(s,[](auto &io,auto &m){leaves::kernel_1ec970(io,m);},original::run_1ec970);
    case l1f7f60:return native_kernel(s,[](auto &io,auto &m){leaves::kernel_1f7f60(io,m);},original::run_1f7f60);
    case c1e90:return native_kernel(s,[](auto &io,auto &m){leaves::kernel_c1e90(io,m);},original::run_c1e90);
    case l110350:return native_kernel(s,[](auto &io,auto &m){leaves::kernel_110350(io,m);},original::run_110350);
    case l243f20:return native_kernel(s,[](auto &io,auto &m){leaves::kernel_243f20(io,m);},original::run_243f20);
    case l32cc0c:return native_kernel(s,[](auto &io,auto &m){leaves::kernel_32cc0c<false>(io,m);},original::run_32cc0c);
    case l32cc0c_tracked:return native_kernel(s,[](auto &io,auto &m){leaves::kernel_32cc0c<true>(io,m);},original::run_32cc0c_tracked);
    case l16e600:return native_kernel(s,[](auto &io,auto &m){leaves::kernel_16e600(io,m);},original::run_16e600);
    case l171010:return native_kernel(s,[](auto &io,auto &m){leaves::kernel_171010(io,m);},original::run_171010);
    case l1f7e90:return native_kernel(s,[](auto &io,auto &m){leaves::kernel_1f7e90(io,m);},original::run_1f7e90);
    case l32a4e0:return native_kernel(s,[](auto &io,auto &m){leaves::kernel_32a4e0(io,m);},original::run_32a4e0);
    case l16e4b0:return native_kernel(s,[](auto &io,auto &m){leaves::kernel_16e4b0(io,m);},original::run_16e4b0);
    case l16e450:return native_kernel(s,[](auto &io,auto &m){leaves::kernel_16e450(io,m);},original::run_16e450);
    case l22a790:return leaves::run_with_children(s,[](auto &io,auto &m){
            leaves::kernel_22a790(io,m,[](auto &r){original::dispatch(r,0x4057f0u,0x22a7e6u,false);});},
        [](auto &r){original::native_live_support_reference_22a790(r);},0x0u);
    case l1f8200:return leaves::run_with_children(s,[](auto &io,auto &m){
            leaves::kernel_1f8200(io,m,[](auto &r){original::dispatch(r,0x144090u,0x1f8284u,false);},[](auto &r){original::run_1e6980(r);});},
        [](auto &r){original::native_live_support_reference_1f8200(r);},0x0u);
    case l138d90:return leaves::run_with_children(s,[](auto &io,auto &m){
            leaves::kernel_138d90<true>(io,m,[](auto &r){original::run_138510(r);},[](auto &r){original::dispatch(r,0x1385d0u,0x138f17u,false);},
                [](auto &r){original::dispatch(r,0x1386e0u,0x138f0eu,false);},[](auto &r){original::run_138c20(r);});},
        [](auto &r){original::native_live_support_reference_138d90(r);},0x0u);
    case l138d90_untracked:return leaves::run_with_children(s,[](auto &io,auto &m){
            leaves::kernel_138d90<false>(io,m,[](auto &r){original::run_138510(r);},[](auto &r){original::run_1385d0(r);},
                [](auto &r){original::run_1386e0(r);},[](auto &r){original::run_138c20(r);});},
        [](auto &r){original::native_live_support_reference_138d90_untracked(r);},0x0u);
    case l179cd0:return leaves::run_with_children(s,[](auto &io,auto &m){
            leaves::kernel_179cd0(io,m,[](auto &r){original::dispatch(r,0x16ea60u,0x179cedu,false);},[](auto &r){original::run_17c370(r);},
                [](auto &r){original::run_17c5c0(r);},[](auto &r){original::dispatch(r,0x17a380u,0x179d59u,false);},
                [](auto &r){original::run_17c140(r);});},
        [](auto &r){original::native_live_support_reference_179cd0(r);},0x0u);
    // The pool startup copies report canonical store PCs (the binder's site map).
    case l16e450_pool:return leaves::run_with_store_sites(s,[](auto &io,auto &m){leaves::kernel_16e450(io,m);},
        [](auto &r){original::run_16e450_pool(r);},original::pool_sites_16e450);
    case l16e4b0_pool:return leaves::run_with_store_sites(s,[](auto &io,auto &m){leaves::kernel_16e4b0(io,m);},
        [](auto &r){original::run_16e4b0_pool(r);},original::pool_sites_16e4b0);
    case l16e600_pool:return leaves::run_with_store_sites(s,[](auto &io,auto &m){leaves::kernel_16e600(io,m);},
        [](auto &r){original::run_16e600_pool(r);},original::pool_sites_16e600);
    case l32a4e0_pool:return leaves::run_with_store_sites(s,[](auto &io,auto &m){leaves::kernel_32a4e0(io,m);},
        [](auto &r){original::run_32a4e0_pool(r);},original::pool_sites_32a4e0);
    default:return native_kernel(s,[](auto &io,auto &m){leaves::kernel_1211c0(io,m);},original::run_1211c0);
    }
}
void reference_route(Leaf leaf,WorldState &s) {
    switch(leaf) {
    case c9550:original::run_c9550(s);break;
    case l1ec970:original::run_1ec970(s);break;
    case l1f7f60:original::run_1f7f60(s);break;
    case c1e90:original::run_c1e90(s);break;
    case l110350:original::run_110350(s);break;
    case l243f20:original::run_243f20(s);break;
    case l32cc0c:original::run_32cc0c(s);break;
    case l32cc0c_tracked:original::run_32cc0c_tracked(s);break;
    case l16e600:original::run_16e600(s);break;
    case l171010:original::run_171010(s);break;
    case l1f7e90:original::run_1f7e90(s);break;
    case l32a4e0:original::run_32a4e0(s);break;
    case l16e4b0:original::run_16e4b0(s);break;
    case l16e450:original::run_16e450(s);break;
    case l22a790:original::run_22a790(s);break;
    case l1f8200:original::run_1f8200(s);break;
    case l138d90:original::run_138d90(s);break;
    case l138d90_untracked:original::run_138d90_untracked(s);break;
    case l179cd0:original::run_179cd0(s);break;
    case l16e450_pool:original::run_16e450_pool(s);break;
    case l16e4b0_pool:original::run_16e4b0_pool(s);break;
    case l16e600_pool:original::run_16e600_pool(s);break;
    case l32a4e0_pool:original::run_32a4e0_pool(s);break;
    default:original::run_1211c0(s);break;
    }
}
struct Result {
    std::vector<SourcePhysicsContactRecordStore> stores;SourcePhysicsWorldRuntimeAbi abi{};
    std::uint32_t cursor{};bool admitted{};std::string error;std::vector<std::uint32_t> trace;
};
enum class Route{native,reference,census};
Result execute(Memory &memory,const Case &c,Leaf leaf,Route route) {
    Result result;
    SourcePhysicsContactRecordObserver observe=[&](const SourcePhysicsContactRecordStore &e){result.stores.push_back(e);};
    SourcePhysicsWorldRuntimeServices services;
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    if(route==Route::native) {
        services.instruction=[](auto){throw std::runtime_error("Native leaf delivered a translated instruction");};
        auto &cursor=services.instruction_cursor;cursor.position=&result.cursor;cursor.service_sites=sites;
        cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
        cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    } else if(route==Route::census) {
        // Full callbacks without a cursor: admitted only under the census switch.
        services.instruction=[](auto){throw std::runtime_error("Census leaf delivered a translated instruction");};
    } else services.instruction=[&](std::uint32_t pc){result.cursor=pc;result.trace.push_back(pc);};
    const auto host=_mm_getcsr();std::uint16_t host_control;asm volatile("fnstcw %0":"=m"(host_control));
    WorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    try {
        asm volatile("fnclex");
        s.load(c.abi);
        if(route!=Route::reference)result.admitted=native_route(leaf,s);else reference_route(leaf,s);
        result.abi=s.save();
    } catch(const std::exception &e){result.error=e.what();}
    asm volatile("fnclex\n\tfldcw %0"::"m"(host_control));_mm_setcsr(host);
    return result;
}
bool same_abi(const SourcePhysicsWorldRuntimeAbi &a,const SourcePhysicsWorldRuntimeAbi &b) {
    bool same=a.gp==b.gp && a.eflags==b.eflags && a.xmm==b.xmm && a.x87_status==b.x87_status && a.x87_depth==b.x87_depth &&
        a.floating_point.has_value() && b.floating_point.has_value() &&
        a.floating_point->mxcsr==b.floating_point->mxcsr && a.floating_point->x87_control_word==b.floating_point->x87_control_word;
    for(unsigned i=0;i<a.x87_depth && i<8;++i)same=same && !std::memcmp(&a.x87[i],&b.x87[i],10);
    return same;
}
}

int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):6000;
    const std::string mode=argc>2?argv[2]:"";
    if(mode=="verify"){setenv("HALO2_VERIFY_MATRIX_LEAVES","1",1);overlay_mode=true;}
    if(mode=="census")setenv("HALO2_CENSUS_NATIVE_BRIDGES","1",1);
    std::mt19937 generator(0x110350u+unsigned(mode.size()));Rng rng{generator};
    std::uint64_t stores[leaf_count]{},calls[leaf_count]{},faults[leaf_count]{},refused{};
    std::map<std::uint32_t,std::set<std::uint32_t>> edges;std::map<std::uint32_t,std::uint64_t> exits;
    for(unsigned n=0;n<cases;++n) {
        const auto leaf=Leaf(n%leaf_count);
        auto c=author(leaf,rng);
        auto left=c.memory.clone(),right=c.memory.clone();
        const auto native=execute(right,c,leaf,mode=="census"?Route::census:Route::native);
        if(native.error.empty() && !native.admitted){++refused;continue;}
        const auto reference=execute(left,c,leaf,Route::reference);
        for(std::size_t i=0;i+1<reference.trace.size();++i)edges[reference.trace[i]].insert(reference.trace[i+1]);
        // The shared leaf verify mode computes the kernel on a pure overlay
        // first, so a faulting operand stops before the live prefix stores.
        // The overlay never faults on a store, so a reference that faults on
        // a store is reported by the overlay's next read of that page
        // instead; release mode compares every fault exactly.
        if(mode=="verify" && !reference.error.empty() && !native.error.empty()){++faults[leaf];continue;}
        ++calls[leaf];stores[leaf]+=reference.stores.size();
        // A fault leaves the leaf's frame (below the entry ESP) dead; the
        // adapter's entry saves may precede an early faulting read there.
        const auto outside_frame=[](const std::vector<SourcePhysicsContactRecordStore> &stores) {
            std::vector<SourcePhysicsContactRecordStore> kept;
            for(const auto &store:stores)if(store.address>=stack||store.address+store.written.size()<=stack-0x40u)kept.push_back(store);
            return kept;
        };
        const bool faulted=!reference.error.empty();
        if((faulted?outside_frame(native.stores)!=outside_frame(reference.stores):native.stores!=reference.stores) ||
           native.error!=reference.error) {
            std::fprintf(stderr,"%s case %u store/fault mismatch (%zu/%zu) '%s' '%s'\n",names[leaf],n,native.stores.size(),
                reference.stores.size(),native.error.c_str(),reference.error.c_str());
            for(std::size_t i=0;i<native.stores.size() && i<reference.stores.size();++i)if(!(native.stores[i]==reference.stores[i])) {
                const auto &a=native.stores[i],&b=reference.stores[i];std::uint32_t va{},vb{};
                for(std::size_t k=0;k<a.written.size()&&k<4;++k)va|=std::to_integer<std::uint32_t>(a.written[k])<<(8*k);
                for(std::size_t k=0;k<b.written.size()&&k<4;++k)vb|=std::to_integer<std::uint32_t>(b.written[k])<<(8*k);
                std::fprintf(stderr,"  store %zu pc %x/%x at %x/%x value %x/%x\n",i,a.instruction,b.instruction,a.address,b.address,va,vb);break;
            }
            return 1;
        }
        if(mode.empty() && native.cursor!=reference.cursor) {
            std::fprintf(stderr,"%s case %u cursor %x vs %x (%s)\n",names[leaf],n,native.cursor,reference.cursor,reference.error.c_str());return 1;
        }
        if(!reference.error.empty()){
            if(std::getenv("HALO2_LEAVES_DEBUG"))std::fprintf(stderr,"%s fault %s\n",names[leaf],reference.error.c_str());
            ++faults[leaf];continue;
        }
        if(!same_abi(native.abi,reference.abi)||!left.same(right)) {
            std::fprintf(stderr,"%s case %u register/flag/FP/bank mismatch eflags %x/%x mxcsr %x/%x\n",names[leaf],n,
                native.abi.eflags,reference.abi.eflags,native.abi.floating_point->mxcsr,reference.abi.floating_point->mxcsr);
            for(unsigned i=0;i<8;++i)if(native.abi.gp[i]!=reference.abi.gp[i])std::fprintf(stderr,"  gp%u %x/%x\n",i,native.abi.gp[i],reference.abi.gp[i]);
            for(unsigned i=0;i<8;++i)if(native.abi.xmm[i]!=reference.abi.xmm[i])std::fprintf(stderr,"  xmm%u %x/%x\n",i,native.abi.xmm[i][0],reference.abi.xmm[i][0]);
            return 1;
        }
        ++exits[reference.cursor];
    }
    if(mode.empty()) {
        // Refusals: MapPack11, full callbacks without the census switch, and an
        // active trace; the reference switch is checked by the validator.
        SourcePhysicsContactRecordObserver observe;
        for(unsigned leaf=0;leaf<leaf_count;++leaf) {
            auto c=author(Leaf(leaf),rng);SourcePhysicsWorldRuntimeServices services;
            {WorldState s{c.memory,observe,services,SourcePhysicsContactImage::MapPack11};s.load(c.abi);
                if(native_route(Leaf(leaf),s))throw std::runtime_error(std::string(names[leaf])+" MapPack11 admitted");}
            services.instruction=[](auto){};
            {WorldState s{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(c.abi);
                if(native_route(Leaf(leaf),s))throw std::runtime_error(std::string(names[leaf])+" uncursored callbacks admitted");}
            std::uint32_t position{};bool trace=true;constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
            auto &cursor=services.instruction_cursor;cursor.position=&position;cursor.service_sites=sites;cursor.first_trace=&trace;
            cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;
            {WorldState s{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(c.abi);
                if(native_route(Leaf(leaf),s))throw std::runtime_error(std::string(names[leaf])+" active trace admitted");}
        }
    }
    std::printf("{\"cases\":%u,\"mode\":\"%s\",\"refused\":%llu,\"leaves\":{",cases,mode.c_str(),(unsigned long long)refused);
    for(unsigned i=0;i<leaf_count;++i)std::printf("%s\"%s\":{\"compared\":%llu,\"faults\":%llu,\"ordered_stores\":%llu}",i?",":"",names[i],
        (unsigned long long)calls[i],(unsigned long long)faults[i],(unsigned long long)stores[i]);
    std::printf("},\"exits\":{");bool first=true;
    for(const auto &[pc,count]:exits){std::printf("%s\"%x\":%llu",first?"":",",pc,(unsigned long long)count);first=false;}
    std::printf("},\"edges\":{");first=true;
    for(const auto &[pc,next]:edges){std::printf("%s\"%x\":[",first?"":",",pc);bool f=true;for(auto n:next){std::printf("%s\"%x\"",f?"":",",n);f=false;}std::printf("]");first=false;}
    std::printf("}}\n");
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
