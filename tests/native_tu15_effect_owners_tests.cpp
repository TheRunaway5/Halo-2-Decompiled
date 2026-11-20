// Native TU1.5 effect owners (20680 lens flare reset; C35D0 / C3300 / C2A80
// per-view light fade; 176A40 / 252100 effect state values) against the
// unmodified translated bodies of the lens flare, effect light view,
// particle frame and death cycle units, extracted by
// tools/validate_native_effect_owners_tu15.py into lens.inc, light.inc,
// state.inc and state_dispatch.inc (4EFA0 dispatched), plus the
// verification references the production binder generates. 4EFA0 is a
// genuine stand-in like 16EA60 (vector stores, XMM/GP/flag clobbers). 16EA60 (first datum of the light array) is a genuine
// stand-in: its EAX result (including -1 and indices whose handle is -1),
// clobbers, flags, stack balance and one store are a hash of its complete
// entry ABI (GP, flags, XMM, x87, MXCSR), so any entry difference diverges.
// Seeded fixtures author complete light arrays (bit vectors, salt words,
// header bits, tag rows, definitions with out-of-table indices, positions,
// radii and outputs with NaN payloads, infinities, denormals and -0), every
// x87 precision/rounding control, x87 stack depths 0..4 (4 keeps the
// translated route), every MXCSR rounding/DAZ/FTZ mode, and the lens flare
// camera/mode/previous-index words around their boundaries, plus fault
// injection (one fixture page removed). Each case compares ordered stores
// (pc, address, preimage, bytes), the child log, bank, registers, flags, XMM,
// x87 lanes/status (C1, TOP and sticky flags), MXCSR and the cursor; faults
// compare the exception, cursor and store prefix.
// Modes: <cases> [census|verify|reference]. Prints a JSON summary with
// branch coverage.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "rendering/effects/native_tu15_effect_owners.hpp"
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <random>
#include <thread>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace eo=halo2::native_effect_owner_detail;
namespace {
std::uint64_t mix(std::uint64_t h,std::uint64_t v){h^=v+0x9e3779b97f4a7c15ull+(h<<6)+(h>>2);return h*0xff51afd7ed558ccdull;}
constexpr std::uint32_t array=0x10000000u,data=0x11000000u,bits=0x12000000u,tags=0x13000000u,defs=0x13800000u,
    scratch=0x14000000u,stack=0x20010000u;
std::vector<std::uint64_t> *call_log{};
struct Child {std::uint32_t policy{},count{},special{};} child;
std::uint32_t special(std::mt19937_64 &rng) {
    static constexpr std::uint32_t values[]{0x7fc00001u,0xffc00000u,0x7f800001u,0xff800005u,0x7f800000u,0xff800000u,
        0x00000001u,0x807fffffu,0x80000000u,0x00000000u,0x3f800000u,0x447a0000u,0x00800000u,0x7f7fffffu};
    return values[rng()%14];
}
// 16EA60 stand-in: ECX array, EDX start index; RET; EAX the first index.
void stub_16ea60(WorldState &s) {
    if(r(s.memory,s.gp[4])!=0xc35e1u)throw std::runtime_error("16EA60 entered without its C35D0 return address");
    std::uint64_t h=0x16ea60u;
    for(const auto v:s.gp)h=mix(h,v);
    h=mix(h,s.flags.bits());h=mix(h,s.other_flags);h=mix(h,_mm_getcsr());h=mix(h,s.fp.count);h=mix(h,s.fp.status);
    for(unsigned i=0;i<8;++i){std::array<std::uint32_t,4> b;std::memcpy(b.data(),&s.xmm[i],16);for(auto v:b)h=mix(h,v);}
    h=mix(h,r(s.memory,s.gp[1]+0x38u));
    std::mt19937_64 rng(h);
    if(call_log) {
        if(call_log->size()>=64)throw std::runtime_error("Child call budget exceeded");
        call_log->push_back(h);
    }
    w(s.memory,s.observe,0x16ea61u,scratch+std::uint32_t(rng()%32)*4u,4,std::uint32_t(rng()));
    const auto roll=rng()%16;
    if(child.special)s.gp[0]=child.special;
    else if(roll==0 || !child.count)s.gp[0]=0xffffffffu;
    else s.gp[0]=std::uint32_t(rng()%child.count);
    s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(child.policy&2u && rng()%23==0){static constexpr unsigned saved[]{3,6,7};s.gp[saved[rng()%3]]=std::uint32_t(rng()%5);}
    if(child.policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    s.gp[4]+=4u;
}
// 4EFA0 stand-in: ECX the 252100 local vector; RET. Its stores (the vector),
// XMM clobbers, GP results and flags are a hash of the entry ABI and the
// vector, so any entry or ordering difference diverges.
void stub_4efa0(WorldState &s) {
    const auto back=r(s.memory,s.gp[4]);
    if(back!=0x25228eu && back!=0x2523b4u)throw std::runtime_error("4EFA0 entered without a 252100 return address");
    std::uint64_t h=0x4efa0u;
    for(const auto v:s.gp)h=mix(h,v);
    h=mix(h,s.flags.bits());h=mix(h,s.other_flags);h=mix(h,_mm_getcsr());
    for(unsigned i=0;i<8;++i){std::array<std::uint32_t,4> b;std::memcpy(b.data(),&s.xmm[i],16);for(auto v:b)h=mix(h,v);}
    for(unsigned k=0;k<3;++k)h=mix(h,r(s.memory,s.gp[1]+k*4u));
    std::mt19937_64 rng(h);
    if(call_log) {
        if(call_log->size()>=256)throw std::runtime_error("Child call budget exceeded");
        call_log->push_back(h);
    }
    for(unsigned k=0;k<3;++k) {
        // Policy 4: NaN-heavy vectors with distinct payloads (dual-NaN operand order).
        const auto bits=(child.policy&4u) && rng()%2?std::uint32_t(0x7fc00000u|(rng()&0x803fffffu)):
            rng()%11==0?special(rng):std::bit_cast<std::uint32_t>(float(int(rng()%201)-100)/100.0f);
        w(s.memory,s.observe,0x4efa1u+k,s.gp[1]+k*4u,4,bits);
    }
    for(unsigned i=0;i<8;++i)if(rng()%3==0){std::array<std::uint32_t,4> lanes{std::uint32_t(rng()),std::uint32_t(rng()),
        std::uint32_t(rng()),std::uint32_t(rng())};std::memcpy(&s.xmm[i],lanes.data(),16);}
    s.gp[0]=std::uint32_t(rng());s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(child.policy&2u && rng()%23==0){static constexpr unsigned saved[]{3,5,6,7};s.gp[saved[rng()%4]]=std::uint32_t(rng()%5);}
    if(child.policy&1u)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64));
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    s.gp[4]+=4u;
}
std::uint32_t canonical_branch(std::uint32_t p,SourcePhysicsContactImage) {
    switch(p) {
    case 0x252119u:case 0x25212au:case 0x25213bu:case 0x252158u:case 0x252175u:case 0x252186u:case 0x252198u:
    case 0x2521a9u:case 0x2521bcu:case 0x2521e2u:case 0x25220fu:case 0x25223cu:case 0x2522f6u:case 0x252415u:return p;
    default:throw std::runtime_error("Unbound TU1.5 contact dispatch target");
    }
}
}
namespace original {
using halo2::physics_world_runtime_detail::WorldState;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
std::uint32_t sign_extend(std::uint32_t value,unsigned count) {
    if(count==1)return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value))));
    if(count==2)return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
    throw std::runtime_error("Unverified sign-extension width");
}
std::uint32_t native_single_spill(long double x,std::uint16_t &status) {
    float result;std::uint16_t actual;
    asm volatile("fldt %2\n\tfstps %0\n\tfnstsw %1":"=m"(result),"=m"(actual):"m"(x):"st");
    status=(status&~0x200u)|(actual&0x200u);
    return std::bit_cast<std::uint32_t>(result);
}
static void run_16ea60(WorldState &s){stub_16ea60(s);}
static void run_c2a80(WorldState &);
static void run_c3300(WorldState &);
static void run_4efa0(WorldState &s){stub_4efa0(s);}
#include "lens.inc"
#include "light.inc"
#include "state.inc"
#include "references.inc"
}
// The death cycle copy: 252100 enters 4EFA0 through the unit's dispatch.
namespace dispatched {
using halo2::physics_world_runtime_detail::WorldState;
using original::selected;using original::sign_extend;
static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t site,bool) {
    if(target!=0x4efa0u || (site!=0x252289u && site!=0x2523afu))throw std::runtime_error("Unbound death cycle dispatch target");
    stub_4efa0(s);
}
#include "state_dispatch.inc"
#include "state_dispatch_references.inc"
}
namespace {
std::uint32_t small_float(std::mt19937 &rng,int range=40,float scale=10.0f) {
    return std::bit_cast<std::uint32_t>(float(int(rng()%unsigned(2*range+1))-range)/scale);
}
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
enum class Owner {lens,light,state,state_dispatch};
struct Case {Memory memory;SourcePhysicsWorldRuntimeAbi abi{};std::uint32_t mxcsr{};Child child{};Owner owner{};};

void registers(Case &c,std::mt19937 &rng,bool x87) {
    for(auto &v:c.abi.gp)v=std::uint32_t(rng());
    c.abi.gp[4]=stack;
    c.abi.eflags=0x202u|(std::uint32_t(rng())&0x8d5u)|(rng()%9==0?0x400u:0u);
    for(auto &lane:c.abi.xmm)for(auto &b:lane)b=std::uint32_t(rng());
    static constexpr std::uint32_t modes[]{0x1f80u,0x3f80u,0x5f80u,0x7f80u,0x9fc0u,0x1fc0u,0x9f80u,0x1f81u,0x1fbfu};
    c.mxcsr=modes[rng()%9];
    // Live x87 stack (depth 0..4; 4 leaves too little room for C2A80),
    // condition bits, TOP and any precision/rounding control.
    c.abi.x87_depth=x87?rng()%5:rng()%3;
    for(unsigned i=0;i<c.abi.x87_depth;++i)c.abi.x87[i]=(long double)(int(rng()%2001)-1000)/7.0L;
    const unsigned top=(8u-c.abi.x87_depth+rng()%8)&7u;
    c.abi.x87_status=std::uint16_t((top<<11)|(std::uint32_t(rng())&0x4700u)|(rng()%5==0?std::uint32_t(rng())&0x3fu:0u));
    const std::uint16_t control=std::uint16_t(0x3fu|0x40u|((rng()%4)<<8)|((rng()%4)<<10));
    c.abi.floating_point=SourcePhysicsFactoryFloatingPoint{control,c.mxcsr};
}

Case author_lens(std::mt19937 &rng,unsigned n) {
    Case c;c.owner=Owner::lens;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    static constexpr std::int32_t edges[]{-5,-1,0,1,3,4,5,6,7,8,9,12,std::numeric_limits<std::int32_t>::min(),std::numeric_limits<std::int32_t>::max()};
    const std::int32_t camera=pick(5)==0?-1-std::int32_t(pick(3)):std::int32_t(pick(6));
    std::int32_t previous;
    switch(pick(6)){case 0:previous=0;break;case 1:previous=camera;break;case 2:previous=camera+1;break;
        case 3:previous=camera-1;break;case 4:previous=-1-std::int32_t(pick(4));break;default:previous=std::int32_t(rng());}
    // The slots and flags first; the camera words are authored over them.
    for(std::uint32_t at=0x52dfd8u;at<0x532098u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(0x4922c0u,std::uint32_t(pick(23)==0?std::numeric_limits<std::int32_t>::min():camera));
    m.put(0x4920b8u,std::uint32_t(edges[pick(14)]));
    m.put(0x532080u,std::uint32_t(previous));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x12b53u);
    registers(c,rng,false);
    if(n%29==7) {
        static constexpr std::uint32_t regions[]{0x52e000u,0x530000u,0x531000u,0x532000u,stack};
        m.pages.erase(regions[pick(5)]>>12);
    }
    return c;
}

Case author_light(std::mt19937 &rng,unsigned n) {
    Case c;c.owner=Owner::light;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    std::mt19937_64 wide(rng());
    const unsigned shape=n%17;
    std::uint32_t count=1+pick(24);
    if(shape==2)count=0;
    std::uint32_t stride=0x110u;
    c.child.count=count;c.child.policy=pick(4);
    if(shape==5){stride=0;c.child.special=0xffffu;}       // first handle -1
    if(shape==6){count=0x10000u;c.child.count=count;c.child.special=0xfffeu;} // next handle -1
    m.put(0x4ecbdcu,array);m.put(0x4f0414u,tags);
    m.put(array+0x24u,stride);m.put(array+0x38u,shape==9?std::uint32_t(-1-int(pick(3))):count);
    m.put(array+0x44u,data);m.put(array+0x48u,bits);
    const unsigned density=1+pick(3);
    const auto words=std::max<std::uint32_t>((count+31)/32,1);
    for(std::uint32_t k=0;k<words+1;++k) {
        std::uint32_t word=0;
        for(unsigned b=0;b<32;++b)if(pick(4)<density)word|=1u<<b;
        if(shape==7 && k*32u>=std::min(count,8u))word=0; // the scan runs off the end
        m.put(bits+k*4u,word);
    }
    const auto value=[&](float scale) {
        if(pick(13)==0)return special(wide);
        return small_float(rng,40,scale);
    };
    const auto record=[&](std::uint32_t index) {
        const auto at=data+index*0x110u;
        m.put(at,pick(31)==0?0xffffu:pick(0x10000u),2);
        m.put(at+2u,pick(10)<7?(pick(256)|2u):(pick(256)&~2u),1);
        m.put(at+3u,pick(256),1);
        m.put(at+4u,(pick(0x10000u)<<16)|pick(8));
        for(std::uint32_t k=8;k<0x110u;k+=4)m.put(at+k,std::uint32_t(rng()));
        for(std::uint32_t k=0x28u;k<0x34u;k+=4)m.put(at+k,value(1.0f));
        m.put(at+0x34u,pick(17)==0?special(wide):small_float(rng,40,4.0f));
    };
    const auto live=std::min(count,64u);
    for(std::uint32_t i=0;i<live;++i)record(i);
    if(shape==5) {
        m.put(data,0xffffu,2);
        for(std::uint32_t k=0;k<0x110u;k+=4)m.put(data+0xffffu*0x110u+k,std::uint32_t(rng()));
    }
    if(shape==6) {
        record(0xfffeu);record(0xffffu);m.put(data+0xffffu*0x110u,0xffffu,2);
        m.put(bits+(0xffffu>>5)*4u,0x80000000u);
    }
    for(std::uint32_t k=0;k<8;++k) {
        m.put(tags+k*16u+8u,defs+k*0x100u);
        for(std::uint32_t o=0;o<0x100u;o+=4)m.put(defs+k*0x100u+o,std::uint32_t(rng()));
        for(std::uint32_t o=0xb8u;o<=0xbcu;o+=2)m.put(defs+k*0x100u+o,std::uint32_t(int(pick(9))-2),2);
    }
    for(std::uint32_t at=0x44a6c0u;at<0x44a760u;at+=4)m.put(at,value(10.0f));
    // The 46A1xx constants: the narrow scale bound (46A178 = 0.1), the
    // three scales and 1.0, occasionally special.
    m.put(0x46a178u,pick(9)==0?special(wide):std::bit_cast<std::uint32_t>(0.1f));
    for(const auto at:{0x46a110u,0x46a100u,0x46a0ccu})m.put(at,pick(9)==0?special(wide):small_float(rng,20,4.0f));
    m.put(0x46a0d0u,pick(13)==0?special(wide):0x3f800000u);
    for(std::uint32_t at=0x4c65c0u;at<0x4c65ccu;at+=4)m.put(at,value(1.0f));
    for(std::uint32_t at=scratch;at<scratch+0x80u;at+=4)m.put(at,0);
    for(std::uint32_t at=stack-0x80u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x2bacbu);
    registers(c,rng,true);
    if(shape==16) {
        static constexpr std::uint32_t regions[]{array,data,bits,tags,defs,0x44a000u,0x46a000u,0x4c6000u,stack-0x80u};
        m.pages.erase(regions[pick(9)]>>12);
    }
    return c;
}
constexpr std::uint32_t particle=0x15000000u,system=0x15100000u,location=0x15200000u,owners=0x15300000u,
    owner_data=0x15400000u,timer=0x15500000u,values=0x15600000u;
Case author_state(std::mt19937 &rng,unsigned n,bool dispatch) {
    Case c;c.owner=dispatch?Owner::state_dispatch:Owner::state;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    std::mt19937_64 wide(rng());
    const unsigned shape=n%13;
    c.child.policy=pick(4);
    const auto value=[&](int range,float scale){return pick(11)==0?special(wide):small_float(rng,range,scale);};
    // The original 17-entry jump table (TU1.5 .text 252430).
    static constexpr std::uint32_t table[17]{0x252119u,0x25212au,0x25213bu,0x25213bu,0x252175u,0x252186u,0x252186u,
        0x252198u,0x2521a9u,0x2521bcu,0x2521e2u,0x25220fu,0x25223cu,0x2522f6u,0x252158u,0x252158u,0x252415u};
    for(unsigned k=0;k<17;++k)m.put(0x252430u+k*4u,table[k]);
    if(shape==7)m.put(0x252430u+pick(17)*4u,0x252111u+pick(3)); // an unbound target
    std::uint32_t mask=pick(1u<<17);
    if(shape==1)mask=0;
    if(shape==2)mask|=(1u<<(17+pick(15)));          // indices beyond the table
    if(shape==3)mask=1u<<pick(17);
    for(std::uint32_t at=particle;at<particle+0x40u;at+=4)m.put(at,value(40,4.0f));
    for(std::uint32_t at=particle+0x10u;at<particle+0x1cu;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(particle+0x38u,pick(7)==0?special(wide):small_float(rng,400,7.0f));
    for(std::uint32_t at=system;at<system+0x40u;at+=4)m.put(at,value(40,4.0f));
    m.put(system+0x18u,pick(4)==0?0xffffffffu:(pick(0x10000u)<<16)|pick(6));
    for(std::uint32_t at=location;at<location+0x40u;at+=4)m.put(at,value(40,4.0f));
    m.put(0x4f720cu,owners);m.put(owners+0x44u,owner_data);
    for(std::uint32_t k=0;k<6;++k)for(std::uint32_t o=0x68u;o<0x78u;o+=4)m.put(owner_data+k*0x190u+o,value(40,4.0f));
    m.put(0x51df54u,timer);m.put(timer+4u,value(40,40.0f));
    m.put(timer+8u,pick(9)==0?std::uint32_t(rng()):pick(100000));
    for(const auto at:{0x46a160u,0x46a0c4u})m.put(at,pick(9)==0?special(wide):small_float(rng,20,40.0f));
    m.put(0x46a0d0u,pick(13)==0?special(wide):0x3f800000u);
    for(std::uint32_t at=0x491e38u;at<0x491e5cu;at+=4)m.put(at,value(40,4.0f));
    if(shape==11) {
        // Distinct NaN payloads in every camera term and the 4EFA0 vector.
        c.child.policy|=4u;
        for(std::uint32_t at=0x491e38u;at<0x491e5cu;at+=4)m.put(at,0x7fc00000u|(std::uint32_t(rng())&0x803fffffu));
    }
    for(std::uint32_t at=values;at<values+0x60u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x100u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x22af04u);m.put(stack+4u,mask);
    m.put(stack+8u,pick(9)==0?0u:system);m.put(stack+0xcu,pick(9)==0?0u:location);
    m.put(stack+0x10u,pick(9)==0?0u:particle);m.put(stack+0x14u,values);
    registers(c,rng,false);
    if(shape==12) {
        static constexpr std::uint32_t regions[]{particle,system,location,owners,owner_data,timer,values,0x491000u,0x252000u};
        m.pages.erase(regions[pick(9)]>>12);
    }
    return c;
}
struct Result {
    std::vector<SourcePhysicsContactRecordStore> stores;std::vector<std::uint64_t> calls;
    SourcePhysicsWorldRuntimeAbi abi{};std::uint32_t cursor{},mxcsr{};bool admitted{};std::string fault;
};
bool census_mode{},trace_sticky{};
template<class V> bool native(V &s,Owner owner) {
    if(owner==Owner::lens)return eo::bridge_20680(s,[](auto &v){original::native_effect_reference_20680(v);});
    if(owner==Owner::state)return eo::bridge_176a40(s,[](auto &v){original::native_effect_reference_176a40(v);},
        [](auto &v,std::uint32_t){original::run_4efa0(v);});
    if(owner==Owner::state_dispatch)return eo::bridge_176a40(s,[](auto &v){dispatched::native_effect_reference_176a40(v);},
        [](auto &v,std::uint32_t site){dispatched::dispatch(v,0x4efa0u,site,false);});
    return eo::bridge_c35d0(s,[](auto &v){original::native_effect_reference_c35d0(v);},[](auto &v){original::run_16ea60(v);});
}
Result execute(Memory &memory,const Case &c,bool is_native) {
    Result result;
    SourcePhysicsContactRecordObserver observe=[&](const SourcePhysicsContactRecordStore &e){
        if(result.stores.size()>=100000)throw std::runtime_error("Store budget exceeded");
        result.stores.push_back(e);
    };
    SourcePhysicsWorldRuntimeServices services;
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    if(is_native) {
        services.instruction=[](auto){throw std::runtime_error("Native effect owner delivered a translated instruction");};
        if(!census_mode) {
            auto &cursor=services.instruction_cursor;cursor.position=&result.cursor;cursor.service_sites=sites;
            cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
            cursor.literal_service_sites=true;cursor.observation_only_stores=true;
        }
    } else if(trace_sticky)services.instruction=[&](std::uint32_t pc) {
        std::uint16_t word;asm volatile("fnstsw %0":"=m"(word));
        static std::uint16_t last;if((word&0x3fu)!=last){std::fprintf(stderr,"  sticky %02x at %x\n",word&0x3fu,pc);last=word&0x3fu;}
        result.cursor=pc;
    };
    else services.instruction=[&](std::uint32_t pc){result.cursor=pc;};
    const auto saved=_mm_getcsr();std::uint16_t control;asm volatile("fnstcw %0":"=m"(control));
    WorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    try {
        s.load(c.abi);_mm_setcsr(c.mxcsr);
        child=c.child;call_log=&result.calls;
        if(is_native)result.admitted=native(s,c.owner);
        else if(c.owner==Owner::lens)original::run_20680(s);
        else if(c.owner==Owner::state)original::run_176a40(s);
        else if(c.owner==Owner::state_dispatch)dispatched::run_176a40(s);
        else original::run_c35d0(s);
    } catch(const std::exception &e){result.fault=e.what();}
    call_log=nullptr;result.mxcsr=_mm_getcsr();
    result.abi=s.save();
    asm volatile("fnclex\n\tfldcw %0"::"m"(control));_mm_setcsr(saved);
    return result;
}
bool same_abi(const SourcePhysicsWorldRuntimeAbi &a,const SourcePhysicsWorldRuntimeAbi &b) {
    bool same=a.gp==b.gp && a.eflags==b.eflags && a.xmm==b.xmm && a.x87_status==b.x87_status && a.x87_depth==b.x87_depth;
    for(unsigned i=0;i<a.x87_depth && i<8;++i)same=same && !std::memcmp(&a.x87[i],&b.x87[i],10);
    if(a.floating_point && b.floating_point)same=same && a.floating_point->x87_control_word==b.floating_point->x87_control_word &&
        a.floating_point->mxcsr==b.floating_point->mxcsr;
    return same;
}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    census_mode=mode=="census";
    if(census_mode)setenv("HALO2_CENSUS_NATIVE_BRIDGES","1",1);
    if(mode=="verify")setenv("HALO2_VERIFY_EFFECT_OWNERS","1",1);
    if(mode=="reference")setenv("HALO2_REFERENCE_EFFECT_OWNERS","c35d0",1);
    std::mt19937 rng(0xe0e0u+(census_mode?1u:0u)+(mode=="verify"?2u:0u)+(mode=="reference"?4u:0u));
    std::uint64_t stores{},compared{},faults{},children{},refused{},lens{},light{},state{};
    for(unsigned n=0;n<cases;++n) {
        auto c=n%6==0?author_lens(rng,n/6):n%6==1?author_state(rng,n/6,false):n%6==2?author_state(rng,n/6,true):author_light(rng,n);
        auto left=c.memory.clone(),right=c.memory.clone();
        const auto native_result=execute(right,c,true);
        if(!native_result.admitted && native_result.fault.empty()) {
            // Too little x87 room, unmasked MXCSR exceptions or the reference
            // switch keep the translated route.
            const bool expected=((c.owner==Owner::light && (c.abi.x87_depth>3 || mode=="reference")) ||
                (c.owner!=Owner::lens && (c.mxcsr&0x1f80u)!=0x1f80u));
            if(expected){++refused;continue;}
            std::fprintf(stderr,"case %u refused\n",n);return 1;
        }
        if(mode=="reference" && c.owner==Owner::light) {
            std::fprintf(stderr,"case %u admitted under the C35D0 reference switch\n",n);return 1;
        }
        const auto reference=execute(left,c,false);
        ++compared;stores+=reference.stores.size();children+=reference.calls.size();
        (c.owner==Owner::lens?lens:c.owner==Owner::light?light:state)++;
        if(!reference.fault.empty())++faults;
        if(native_result.fault!=reference.fault) {
            std::fprintf(stderr,"case %u fault mismatch: native '%s' translated '%s'\n",n,native_result.fault.c_str(),reference.fault.c_str());
            return 1;
        }
        if(native_result.stores!=reference.stores) {
            std::size_t at=0;while(at<native_result.stores.size()&&at<reference.stores.size()&&native_result.stores[at]==reference.stores[at])++at;
            std::fprintf(stderr,"case %u store mismatch at %zu of %zu/%zu",n,at,native_result.stores.size(),reference.stores.size());
            if(at<native_result.stores.size()&&at<reference.stores.size())
                std::fprintf(stderr," native pc=%x a=%x original pc=%x a=%x",native_result.stores[at].instruction,native_result.stores[at].address,
                    reference.stores[at].instruction,reference.stores[at].address);
            std::fprintf(stderr,"\n");return 1;
        }
        if(native_result.calls!=reference.calls){std::fprintf(stderr,"case %u child log mismatch\n",n);return 1;}
        if(!census_mode && native_result.cursor!=reference.cursor){std::fprintf(stderr,"case %u cursor %x vs %x\n",n,native_result.cursor,reference.cursor);return 1;}
        if(native_result.mxcsr!=reference.mxcsr){std::fprintf(stderr,"case %u MXCSR %x vs %x\n",n,native_result.mxcsr,reference.mxcsr);return 1;}
        if(reference.fault.empty() && !same_abi(native_result.abi,reference.abi)) {
            const auto &a=native_result.abi,&b=reference.abi;
            std::fprintf(stderr,"case %u register/flag/XMM/x87 mismatch: eflags %x/%x x87 %x/%x depth %u/%u\n",n,a.eflags,b.eflags,
                a.x87_status,b.x87_status,a.x87_depth,b.x87_depth);
            for(unsigned i=0;i<8;++i)if(a.gp[i]!=b.gp[i])std::fprintf(stderr,"  gp%u %x/%x\n",i,a.gp[i],b.gp[i]);
            for(unsigned i=0;i<8;++i)if(a.xmm[i]!=b.xmm[i])std::fprintf(stderr,"  xmm%u %x/%x\n",i,a.xmm[i][0],b.xmm[i][0]);
            if(std::getenv("HALO2_EFFECT_OWNER_TRACE")){auto again=c.memory.clone();trace_sticky=true;execute(again,c,false);trace_sticky=false;}
            return 1;
        }
        if(!left.same(right)){std::fprintf(stderr,"case %u bank mismatch\n",n);return 1;}
    }
    if(!census_mode && mode!="reference") {
        auto c=author_light(rng,1);SourcePhysicsContactRecordObserver observe;SourcePhysicsWorldRuntimeServices services;
        WorldState a{c.memory,observe,services,SourcePhysicsContactImage::MapPack11};a.load(c.abi);
        if(native(a,Owner::light)||native(a,Owner::lens)||native(a,Owner::state))throw std::runtime_error("MapPack11 admitted");
        WorldState b{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};b.load(c.abi);b.fp.count=0;
        const auto saved=_mm_getcsr();_mm_setcsr(0x1f00u);
        const bool unmasked=native(b,Owner::light)||native(b,Owner::state);_mm_setcsr(saved);
        if(unmasked)throw std::runtime_error("Unmasked MXCSR admitted");
        services.instruction=[](auto){};
        WorldState d{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};d.load(c.abi);d.fp.count=0;
        if(native(d,Owner::light)||native(d,Owner::lens)||native(d,Owner::state))throw std::runtime_error("Full instruction callbacks admitted");
        asm volatile("fnclex");
    }
    std::printf("{\"cases\":%u,\"compared\":%llu,\"lens\":%llu,\"light\":%llu,\"state\":%llu,\"refused\":%llu,\"faults\":%llu,"
        "\"ordered_stores\":%llu,\"children\":%llu,\"coverage\":%llu,\"branches\":%u,\"mode\":\"%s\"}\n",cases,
        (unsigned long long)compared,(unsigned long long)lens,(unsigned long long)light,(unsigned long long)state,(unsigned long long)refused,
        (unsigned long long)faults,(unsigned long long)stores,(unsigned long long)children,
        (unsigned long long)eo::coverage().load(),unsigned(eo::branch_count),mode.c_str());
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
