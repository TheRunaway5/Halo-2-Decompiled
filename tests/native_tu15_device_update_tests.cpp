// Native TU1.5 device update callbacks (1179C0 machine update, 10B840 device
// update) against the unmodified translated bodies, extracted by
// tools/validate_native_device_update_tu15.py into device_<owner>.inc with
// the unit's own operand resolver, sign extension and single spill
// (device_prelude.inc), plus the verification references the production
// binder generates (device_<owner>_reference.inc). Every child (direct or
// dispatched) is a genuine stand-in whose effects (stores into the device and
// unit data, the unit query's buffer, the marker and forward vectors, the
// channel step's position and velocity, registers including callee-saved
// clobbers, stack balance, flags, XMM, x87 C1 and MXCSR sticky bits) are a
// hash of its complete entry ABI and the bytes it can see, so any entry
// difference diverges. Seeded fixtures author complete object tables
// (headers, device and machine data, acyclic parent chains), device and
// unit definitions, clusters, device groups, the game clock, the structure
// BSP's machine portals, the openness words and portal bits, with special
// floats (NaN payloads, infinities, denormals, signed zeros, exact 0 and 1)
// under every MXCSR mode, and fault injection (one fixture page removed).
// Each case compares the ordered store events (pc, address, preimage, bytes),
// the child call log, final bank, registers, flags, XMM, x87 status and
// instruction cursor; fault cases compare the exception, the cursor and the
// store prefix. Modes: <cases> [census|verify]. Prints a JSON summary with
// branch coverage. HALO2_DEVICE_DEBUG=1 prints the first differing child
// entry ABI.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_device_update.hpp"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <thread>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace dev=halo2::native_device_update_detail;
namespace {
std::uint64_t mix(std::uint64_t h,std::uint64_t v){h^=v+0x9e3779b97f4a7c15ull+(h<<6)+(h>>2);return h*0xff51afd7ed558ccdull;}

constexpr std::uint32_t headers=0x10200000u,elements=0x10300000u,data=0x10500000u,game=0x10600000u,
    clusters=0x10700000u,records=0x10800000u,tags=0x10c00000u,tagdata=0x10d00000u,groups=0x10e00000u,
    groupdata=0x10f00000u,clock=0x11000000u,bsp=0x11100000u,portals=0x11200000u,indices=0x11300000u,
    openness=0x11400000u,bits=0x11500000u,scratch=0x10b00000u,stack=0x20010000u;
constexpr std::uint32_t object_size=0x200u,tag_size=0x200u;
// Per-case stand-in policy: bit0 callee-saved clobbers, bit1 stack
// imbalance, bit2 XMM changes, bit3 flag churn in the device data, bit4 x87
// C1 and MXCSR sticky churn.
std::uint32_t policy{};std::uint32_t objects{};
struct Entry {std::uint32_t target;std::array<std::uint32_t,8> gp;unsigned flags;std::uint32_t other;std::array<std::uint32_t,32> xmm;
              std::uint16_t fp_status;unsigned fp_count;std::uint32_t mxcsr;};
std::vector<std::uint64_t> *call_log{};std::vector<Entry> *entry_log{};
struct Expect {unsigned bytes;std::set<std::uint32_t> returns;};
const std::map<std::uint32_t,Expect> &expectations() {
    static const std::map<std::uint32_t,Expect> value{
        {0x1da750u,{8,{0x117a91u}}},{0x1ce8d0u,{4,{0x117af6u,0x117ba3u,0x10b9e5u,0x10bd01u}}},
        {0x1cebd0u,{16,{0x117b9cu,0x117c0au,0x10bc1cu,0x10bd89u}}},{0xb7e50u,{8,{0x117bd5u}}},
        {0x10c5d0u,{4,{0x117be2u,0x10b937u,0x10b94fu,0x10bbedu,0x10bc79u,0x10bd5du}}},{0xbd080u,{28,{0x117ce4u}}},
        {0xbad60u,{12,{0x117ddfu}}},{0xbbff0u,{0,{0x117e16u}}},{0x10c290u,{8,{0x117eabu,0x11806au}}},
        {0xbf050u,{4,{0x11809bu,0x10bdccu}}},{0x1181f0u,{4,{0x1180efu,0x10bdd7u}}},{0xbda50u,{0,{0x1180f8u}}},
        {0x118350u,{0,{0x11810eu}}},{0x10cd70u,{12,{0x10b8f3u,0x10bc57u}}},{0x10ca30u,{0,{0x10baf1u,0x10bbcau}}},
        {0x121890u,{20,{0x10bb5cu,0x10bd44u}}},{0xb8760u,{0,{0x10bcf5u,0x10bd72u}}},
    };
    return value;
}
bool nan_dense{};
// Floats rich in the values the devices test: exact 0 and 1, NaNs, signed
// zeros, infinities, denormals and small magnitudes near epsilon.
std::uint32_t device_float(std::mt19937_64 &rng) {
    static constexpr std::uint32_t values[]{0x7fc00001u,0xffc00000u,0x7f800000u,0xff800000u,0x00000001u,0x807fffffu,
        0x80000000u,0x00000000u,0x3f800000u,0x7f7fffffu,0x38d1b717u,0x38d1b716u,0xbf800000u,0x7f800001u};
    if(nan_dense && rng()%2)return 0x7fc00000u|std::uint32_t(rng()%0x400000u)|(rng()%2?0x80000000u:0u);
    switch(rng()%8) {
    case 0:return values[rng()%14];
    case 1:return 0x00000000u;
    case 2:return 0x3f800000u;
    default:return std::bit_cast<std::uint32_t>(float(int(rng()%81)-30)/40.0f);
    }
}
std::uint32_t arg(WorldState &s,unsigned n){return r(s.memory,s.gp[4]+4u*n);}
void put_float(WorldState &s,std::uint32_t target,std::uint32_t address,std::mt19937_64 &rng) {
    w(s.memory,s.observe,target,address,4,device_float(rng));
}
// A genuine child stand-in: every effect is a hash of the entry ABI, the
// return address/arguments and the device bytes it can observe.
void stub(WorldState &s,std::uint32_t target) {
    auto found=expectations().find(target);
    if(found==expectations().end())throw std::runtime_error("Unbound child target");
    const auto back=r(s.memory,s.gp[4]);
    if(!found->second.returns.count(back)) {
        char text[96];std::snprintf(text,sizeof text,"Child %x entered with return address %x",target,back);
        throw std::runtime_error(text);
    }
    const unsigned bytes=found->second.bytes;
    std::uint64_t h=mix(0xde71ceu,target);
    for(const auto v:s.gp)h=mix(h,v);
    for(unsigned i=1;i<=7;++i)h=mix(h,arg(s,i));
    h=mix(h,s.flags.bits());h=mix(h,s.other_flags);h=mix(h,s.fp.status);h=mix(h,s.fp.count);h=mix(h,_mm_getcsr());h=mix(h,back);
    for(unsigned i=0;i<8;++i){std::array<std::uint32_t,4> b;std::memcpy(b.data(),&s.xmm[i],16);for(auto v:b)h=mix(h,v);}
    for(std::uint32_t i=0;i<objects && i<8;++i)h=mix(h,r(s.memory,data+i*object_size+0x12cu));
    std::mt19937_64 rng(h);
    if(call_log) {
        if(call_log->size()>=4096)throw std::runtime_error("Child call budget exceeded");
        call_log->push_back(mix(h,target));
    }
    if(entry_log) {
        Entry e{target,s.gp,s.flags.bits(),s.other_flags,{},s.fp.status,s.fp.count,_mm_getcsr()};
        std::memcpy(e.xmm.data(),s.xmm,sizeof e.xmm);entry_log->push_back(e);
    }
    w(s.memory,s.observe,target,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    // Device state churn: flags, channels and header flags of some object.
    if(objects && (policy&8u || rng()%3==0)) {
        const auto index=std::uint32_t(rng()%objects);
        const auto block=data+index*object_size;
        switch(rng()%6) {
        case 0:w(s.memory,s.observe,target+1u,block+0x12cu,4,std::uint32_t(rng())&0x1ffu);break;
        case 1:put_float(s,target+1u,block+0x140u,rng);break;
        case 2:put_float(s,target+1u,block+0x144u,rng);break;
        case 3:put_float(s,target+1u,block+0x134u,rng);break;
        case 4:w(s.memory,s.observe,target+1u,elements+index*12u+2u,1,std::uint32_t(rng()));break;
        default:w(s.memory,s.observe,target+1u,block+0x1ccu,1,std::uint32_t(rng())&0xfu);break;
        }
    }
    std::uint32_t result=std::uint32_t(rng());
    if(rng()%3)result&=0xffffff00u;   // most children report false in AL
    switch(target) {
    // BD080(flags, types, location, center, radius, output, capacity): AX units.
    case 0xbd080u: {
        const auto output=arg(s,6);const int count=int(rng()%19)-2;
        for(int k=0;k<count && k<16;++k)
            w(s.memory,s.observe,target+5u,output+std::uint32_t(k)*4u,4,
              rng()%13==0?std::uint32_t(rng()):(std::uint32_t(rng()%0x10000u)<<16)|std::uint32_t(rng()%(objects+1u)));
        result=(result&0xffff0000u)|(std::uint32_t(count)&0xffffu);
        break;
    }
    // BAD60(handle, marker, 1) with EBX = the marker: AX found; the forward
    // vector at +3C and the position at +60.
    case 0xbad60u: {
        const int found=int(rng()%4)-1;
        for(unsigned k=0;k<3;++k){put_float(s,target+5u,s.gp[3]+0x3cu+4u*k,rng);put_float(s,target+5u,s.gp[3]+0x60u+4u*k,rng);}
        result=(result&0xffff0000u)|(std::uint32_t(found)&0xffffu);
        break;
    }
    case 0xbbff0u:for(unsigned k=0;k<3;++k)put_float(s,target+5u,s.gp[6]+4u*k,rng);break;   // the forward vector at ESI
    // 121890: the channel's new position (EDI) and velocity (ESI); AL arrived.
    case 0x121890u:
        if(rng()%4)put_float(s,target+5u,s.gp[7],rng);
        if(rng()%4)put_float(s,target+5u,s.gp[6],rng);
        break;
    // 10CD70(handle, block, output): the scripted value at the output.
    case 0x10cd70u:if(arg(s,3))put_float(s,target+5u,arg(s,3),rng);break;
    default:break;
    }
    const auto sp=s.gp[4];
    s.gp[0]=result;s.gp[1]=std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(policy&1u && rng()%5==0) {
        static constexpr std::uint32_t values[]{0xfffffffeu,0x7fffffffu,0xffffffffu,0u,1u,0x80000000u};
        static constexpr unsigned saved[]{3,5,6,7}; // EBX/EBP/ESI/EDI
        s.gp[saved[rng()%4]]=values[rng()%6];
    }
    if(policy&4u)for(unsigned i=0;i<8;++i){std::array<std::uint32_t,4> b;for(auto &v:b)v=std::uint32_t(rng());std::memcpy(&s.xmm[i],b.data(),16);}
    if(policy&16u) {
        if(rng()%2)s.fp.status^=0x200u;
        if(rng()%3==0)_mm_setcsr(_mm_getcsr()|std::uint32_t(rng()%64u));
    }
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    std::uint32_t next=sp+4u+bytes;
    if(policy&2u && rng()%9==0)next+=rng()%2?4u:std::uint32_t(-4);
    s.gp[4]=next;
}
} // namespace

#define DEVICE_CHILD(name) static void run_##name(WorldState &s){stub(s,0x##name##u);}
#define DEVICE_DISPATCH static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool){stub(s,target);}
namespace t_1179c0 {
using halo2::physics_world_runtime_detail::WorldState;
#include "device_prelude.inc"
DEVICE_DISPATCH
DEVICE_CHILD(1da750) DEVICE_CHILD(1ce8d0) DEVICE_CHILD(1cebd0) DEVICE_CHILD(10c5d0) DEVICE_CHILD(bd080) DEVICE_CHILD(bad60)
DEVICE_CHILD(10c290) DEVICE_CHILD(1181f0) DEVICE_CHILD(bda50) DEVICE_CHILD(118350)
#include "device_1179c0.inc"
#include "device_1179c0_reference.inc"
}
namespace t_10b840 {
using halo2::physics_world_runtime_detail::WorldState;
#include "device_prelude.inc"
DEVICE_DISPATCH
DEVICE_CHILD(10cd70) DEVICE_CHILD(10c5d0) DEVICE_CHILD(1ce8d0) DEVICE_CHILD(10ca30) DEVICE_CHILD(121890) DEVICE_CHILD(1cebd0)
DEVICE_CHILD(1181f0)
#include "device_10b840.inc"
#include "device_10b840_reference.inc"
}

namespace {
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

enum Owner : unsigned {machine,device,owners};
struct Case {Memory memory;SourcePhysicsWorldRuntimeAbi abi{};Owner owner{};std::uint32_t policy{},objects{},mxcsr{0x1f80u};bool nan_dense{};};

std::uint32_t handle(std::mt19937 &rng,std::uint32_t index){return (std::uint32_t(rng()%0x10000u)<<16)|index;}

// One authored world. `shape` biases the structural edge cases.
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    std::mt19937_64 wide(rng());
    const auto real=[&]{return device_float(wide);};
    c.owner=Owner(n%owners);
    const unsigned shape=(n/owners)%16;
    nan_dense=c.nan_dense=shape==13;
    c.policy=shape==0?0u:pick(32);
    const std::uint32_t count=shape==3?1+pick(3):2+pick(12);
    c.objects=count;
    const std::uint32_t tag_count=8;
    m.put(0x4ecbd0u,headers);m.put(0x4f0414u,tags);m.put(0x52be5cu,clusters);m.put(0x4ecbf8u,groups);
    m.put(0x51df54u,clock);m.put(0x4f3218u,game);m.put(0x4ecc18u,bsp);m.put(0x4f9b58u,openness);m.put(0x505e7cu,bits);
    // The constants (rarely perturbed, to show each is read where the original reads it).
    const bool perturb=shape==14;
    m.put(0x46a0d0u,perturb && pick(3)==0?real():0x3f800000u);m.put(0x46a0e8u,perturb && pick(3)==0?real():0u);
    m.put(0x46a0ecu,perturb && pick(3)==0?real():0x38d1b717u);m.put(0x46a1b4u,perturb && pick(3)==0?real():0x477fff00u);
    m.put(headers+0x44u,elements);m.put(clusters+0x44u,records);m.put(groups+0x44u,groupdata);
    // Headers (identifier, flags, type) and object data.
    static constexpr std::uint32_t types[]{0,1,7,7,7,8,9,1,0,2};
    for(std::uint32_t i=0;i<count+2;++i) {
        const auto e=elements+i*12u;
        m.put(e,pick(0x10000),2);m.put(e+2u,std::uint32_t(rng())&0xffu,1);
        m.put(e+3u,pick(5)==0?pick(256):types[pick(10)],1);
        m.put(e+4u,std::uint32_t(rng()));m.put(e+8u,data+i*object_size);
        const auto d=data+i*object_size;
        for(std::uint32_t at=0;at<object_size;at+=4)m.put(d+at,std::uint32_t(rng()));
        m.put(d,pick(tag_count)|(std::uint32_t(rng())&0xffff0000u));
        // Acyclic parents: an earlier object or NONE.
        m.put(d+0x14u,i==0 || pick(3)?0xffffffffu:handle(rng,pick(i)));
        for(unsigned k=0;k<3;++k)m.put(d+0x30u+4u*k,real());
        m.put(d+0x3cu,real());
        m.put(d+0xb3u,pick(3)==0?0u:pick(256),1);
        m.put(d+0xb4u,pick(4)==0?0xffffffffu:handle(rng,pick(24)));
        m.put(d+0xc0u,(std::uint32_t(rng())&0xebffu)|(pick(2)?0x1000u:0u)|(pick(2)?0x400u:0u),2);
        m.put(d+0xd4u,pick(3)==0?0xffffffffu:std::uint32_t(rng()));
        m.put(d+0x12au,pick(4)==0?0xffffu:pick(3)==0?0x10000u-8u*(1+pick(16)):0x1e0u,2);
        std::uint32_t flags=std::uint32_t(rng())&0x1ffu;
        if(shape<8 && pick(2))flags&=~0x18u;
        m.put(d+0x12cu,flags);
        m.put(d+0x130u,pick(4)==0?0xffffffffu:handle(rng,pick(24)));
        m.put(d+0x13cu,pick(4)==0?0xffffffffu:handle(rng,pick(24)));
        for(const auto at:{0x134u,0x138u,0x140u,0x144u})m.put(d+at,real());
        m.put(d+0x148u,pick(3)==0?0u:pick(4)==0?0xffffu:pick(6),2);
        m.put(d+0x18cu,pick(4)==0?0xffffffffu:std::uint32_t(rng()));m.put(d+0x192u,pick(4)==0?0xffffu:pick(64),2);
        m.put(d+0x1acu,pick(4)==0?0xffffffffu:std::uint32_t(rng()));m.put(d+0x1b2u,pick(4)==0?0xffffu:pick(64),2);
        m.put(d+0x1ccu,shape==6?(pick(2)?4u:8u)|(pick(2)?2u:0u):shape<6?pick(16)&~1u:pick(16));
        if(shape==6 && pick(2))m.put(d+0x140u,0);
        m.put(d+0x1d0u,pick(3)==0?std::uint32_t(rng()):pick(60)-3u);
        m.put(d+0x1dau,pick(6)==0?std::uint32_t(rng())&0xffffu:pick(8),2);
        m.put(d+0x1dcu,pick(7)==0?std::uint32_t(-int(pick(3)))&0xffffu:pick(5),2);
        // Unit fields the door query reads.
        m.put(d+0x10au,std::uint32_t(rng())&(pick(2)?0xfbu:0xffu),1);
        m.put(d+0x13cu,pick(2)?0xffffffffu:handle(rng,pick(4)));
    }
    // Definitions: the device part, the machine part and the unit fields.
    for(std::uint32_t k=0;k<tag_count;++k) {
        m.put(tags+k*16u,std::uint32_t(rng()));m.put(tags+k*16u+4u,std::uint32_t(rng()));
        m.put(tags+k*16u+8u,tagdata+k*tag_size);m.put(tags+k*16u+12u,std::uint32_t(rng()));
        const auto t=tagdata+k*tag_size;
        for(std::uint32_t at=0;at<tag_size;at+=4)m.put(t+at,std::uint32_t(rng()));
        m.put(t+0xbcu,std::uint32_t(rng())&(pick(2)?~0x4000u:~0u));
        for(std::uint32_t at=0xc0u;at<=0xd4u;at+=4)m.put(t+at,real());
        m.put(t+0x10cu,real());m.put(t+0x118u,real());
        m.put(t+0x11cu,shape==5?2u:shape==6?0u:pick(3),2);
        m.put(t+0x120u,real());m.put(t+0x124u,real());m.put(t+0x128u,real());
        m.put(t+0x1ecu,std::uint32_t(rng())&(pick(2)?~0x100000u:~0u));
    }
    for(std::uint32_t k=0;k<24;++k)for(std::uint32_t at=0;at<0xa0u;at+=4)m.put(records+k*0xa0u+at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<24;++k) {
        m.put(groupdata+k*12u,std::uint32_t(rng()));
        // A group's value is often its device's position or power.
        m.put(groupdata+k*12u+4u,pick(3)==0?r(m,data+pick(count)*object_size+(pick(2)?0x140u:0x134u)):real());
        m.put(groupdata+k*12u+8u,real());
    }
    for(std::uint32_t i=0;i<count;++i) {
        const auto d=data+i*object_size;
        if(pick(3)==0)m.put(d+0x138u,0);
        if(pick(3)==0)m.put(d+0x144u,0);
        for(const auto [group,value]:{std::pair{0x130u,0x134u},std::pair{0x13cu,0x140u}})
            if(pick(3)==0 && r(m,d+group)!=0xffffffffu)m.put(groupdata+(r(m,d+group)&0xffffu)*12u+4u,r(m,d+value));
    }
    m.put(clock+4u,pick(9)==0?real():std::bit_cast<std::uint32_t>(1.0f/30.0f));
    m.put(clock+8u,std::uint32_t(rng()));m.put(clock,std::uint32_t(rng()));
    m.put(game+0xcu,pick(4)==0?4u:pick(8),1);
    m.put(bsp+0x220u,portals);m.put(portals+0xcu,indices);
    for(std::uint32_t k=0;k<64;++k)m.put(indices+k*2u,pick(29)==0?std::uint32_t(rng())&0xffffu:pick(256),2);
    for(std::uint32_t at=openness;at<openness+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=bits;at<bits+0x80u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,0);
    for(std::uint32_t at=stack-0x800u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x13ab00u+pick(0x100));
    auto object=pick(17)==0?(std::uint32_t(rng())|0xffffu):handle(rng,pick(count));
    // A NONE argument whose index (FFFF) resolves to an authored header.
    if(pick(13)==0) {
        object=0xffffffffu;
        const auto e=elements+0xffffu*12u,from=elements+pick(count)*12u;
        for(unsigned k=0;k<12;k+=4)m.put(e+k,r(m,from+k));
    }
    m.put(stack+4u,object);
    if(shape==6){m.put(clock+8u,(0u-object)+4u*pick(1000));m.put(game+0xcu,pick(4),1);}
    for(auto &v:c.abi.gp)v=std::uint32_t(rng());
    c.abi.gp[4]=stack;
    c.abi.eflags=0x202u|(std::uint32_t(rng())&0x8d5u);
    for(auto &lane:c.abi.xmm)for(auto &b:lane)b=std::uint32_t(rng());
    c.abi.x87_depth=pick(4);c.abi.x87_status=std::uint16_t(((8u-c.abi.x87_depth)&7u)<<11|(pick(2)?0x200u:0u));
    for(unsigned i=0;i<c.abi.x87_depth;++i)c.abi.x87[i]=(long double)(int(pick(100))-50)/7.0L;
    static constexpr std::uint32_t modes[]{0x1f80u,0x3f80u,0x5f80u,0x7f80u,0x9fc0u,0x1fc0u,0x9f80u,0x1fbfu};
    c.mxcsr=modes[pick(8)];
    // Fault injection: remove one authored page (not the stack).
    if(shape==15) {
        static constexpr std::uint32_t regions[]{elements,data,tagdata,records,groupdata,clock,bsp,portals,indices,openness,bits,
            0x46a000u,game,tags};
        const auto region=regions[pick(14)];
        auto at=m.pages.lower_bound(region>>12);
        if(at!=m.pages.end()){std::advance(at,std::min<std::size_t>(pick(3),std::distance(at,m.pages.end())-1));m.pages.erase(at);}
    }
    return c;
}

struct Result {
    std::vector<SourcePhysicsContactRecordStore> stores;std::vector<std::uint64_t> calls;std::vector<Entry> entries;
    SourcePhysicsWorldRuntimeAbi abi{};std::uint32_t cursor{},mxcsr{};bool admitted{};std::string fault;
};
bool census_mode{},verify{};
template<class V> bool native(Owner owner,V &s) {
    const auto dispatch_machine=[](auto &v,std::uint32_t target,std::uint32_t site){t_1179c0::dispatch(v,target,site,false);};
    const auto dispatch_device=[](auto &v,std::uint32_t target,std::uint32_t site){t_10b840::dispatch(v,target,site,false);};
    switch(owner) {
    case machine:return dev::bridge_1179c0(s,[](auto &v){t_1179c0::native_device_update_reference_1179c0(v);},dispatch_machine,
        [](auto &v){t_1179c0::run_1da750(v);},[](auto &v){t_1179c0::run_1ce8d0(v);},[](auto &v){t_1179c0::run_1cebd0(v);},
        [](auto &v){t_1179c0::run_10c5d0(v);},[](auto &v){t_1179c0::run_bd080(v);},[](auto &v){t_1179c0::run_bad60(v);},
        [](auto &v){t_1179c0::run_10c290(v);},[](auto &v){t_1179c0::run_1181f0(v);},[](auto &v){t_1179c0::run_bda50(v);},
        [](auto &v){t_1179c0::run_118350(v);});
    case device:return dev::bridge_10b840(s,[](auto &v){t_10b840::native_device_update_reference_10b840(v);},dispatch_device,
        [](auto &v){t_10b840::run_10cd70(v);},[](auto &v){t_10b840::run_10c5d0(v);},[](auto &v){t_10b840::run_1ce8d0(v);},
        [](auto &v){t_10b840::run_10ca30(v);},[](auto &v){t_10b840::run_121890(v);},[](auto &v){t_10b840::run_1cebd0(v);},
        [](auto &v){t_10b840::run_1181f0(v);});
    default:return false;
    }
}
void translated(Owner owner,WorldState &s) {
    if(owner==machine)t_1179c0::run_1179c0(s);
    else t_10b840::run_10b840(s);
}
Result execute(Memory &memory,const Case &c,bool is_native,bool debug) {
    Result result;
    SourcePhysicsContactRecordObserver observe=[&](const SourcePhysicsContactRecordStore &e){result.stores.push_back(e);};
    SourcePhysicsWorldRuntimeServices services;
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    if(is_native && census_mode) {
        services.instruction=[](auto){throw std::runtime_error("Native device update delivered a translated instruction");};
    } else if(is_native) {
        services.instruction=[](auto){throw std::runtime_error("Native device update delivered a translated instruction");};
        auto &cursor=services.instruction_cursor;cursor.position=&result.cursor;cursor.service_sites=sites;
        cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
        cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    } else services.instruction=[&](std::uint32_t pc){result.cursor=pc;};
    WorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(c.abi);
    const auto saved=_mm_getcsr();_mm_setcsr(c.mxcsr);
    policy=c.policy;objects=c.objects;call_log=&result.calls;entry_log=debug?&result.entries:nullptr;nan_dense=c.nan_dense;
    try {
        if(is_native)result.admitted=native(c.owner,s);
        else translated(c.owner,s);
    } catch(const std::exception &e){result.fault=e.what();}
    call_log=nullptr;entry_log=nullptr;result.mxcsr=_mm_getcsr();_mm_setcsr(saved);
    result.abi=s.save();return result;
}
bool same_abi(const SourcePhysicsWorldRuntimeAbi &a,const SourcePhysicsWorldRuntimeAbi &b) {
    return a.gp==b.gp && a.eflags==b.eflags && a.xmm==b.xmm && a.x87_status==b.x87_status && a.x87_depth==b.x87_depth;
}
void describe(const Entry &a,const Entry &b) {
    std::fprintf(stderr,"  child %x/%x:",a.target,b.target);
    static constexpr const char *names[]{"eax","ecx","edx","ebx","esp","ebp","esi","edi"};
    for(unsigned i=0;i<8;++i)if(a.gp[i]!=b.gp[i])std::fprintf(stderr," %s %x/%x",names[i],a.gp[i],b.gp[i]);
    if(a.flags!=b.flags)std::fprintf(stderr," flags %x/%x",a.flags,b.flags);
    if(a.other!=b.other)std::fprintf(stderr," other %x/%x",a.other,b.other);
    for(unsigned i=0;i<32;++i)if(a.xmm[i]!=b.xmm[i])std::fprintf(stderr," xmm%u.%u %x/%x",i/4,i%4,a.xmm[i],b.xmm[i]);
    if(a.fp_status!=b.fp_status)std::fprintf(stderr," fsw %x/%x",a.fp_status,b.fp_status);
    if(a.fp_count!=b.fp_count)std::fprintf(stderr," depth %u/%u",a.fp_count,b.fp_count);
    if(a.mxcsr!=b.mxcsr)std::fprintf(stderr," mxcsr %x/%x",a.mxcsr,b.mxcsr);
    std::fprintf(stderr,"\n");
}
void describe_abi(const SourcePhysicsWorldRuntimeAbi &a,const SourcePhysicsWorldRuntimeAbi &b) {
    static constexpr const char *names[]{"eax","ecx","edx","ebx","esp","ebp","esi","edi"};
    std::fprintf(stderr,"  exit:");
    for(unsigned i=0;i<8;++i)if(a.gp[i]!=b.gp[i])std::fprintf(stderr," %s %x/%x",names[i],a.gp[i],b.gp[i]);
    if(a.eflags!=b.eflags)std::fprintf(stderr," eflags %x/%x",a.eflags,b.eflags);
    for(unsigned i=0;i<8;++i)for(unsigned k=0;k<4;++k)if(a.xmm[i][k]!=b.xmm[i][k])std::fprintf(stderr," xmm%u.%u %x/%x",i,k,a.xmm[i][k],b.xmm[i][k]);
    if(a.x87_status!=b.x87_status)std::fprintf(stderr," fsw %x/%x",a.x87_status,b.x87_status);
    std::fprintf(stderr,"\n");
}
} // namespace

int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):5000;
    const std::string mode=argc>2?argv[2]:"";
    census_mode=mode=="census";verify=mode=="verify";
    const bool debug=std::getenv("HALO2_DEVICE_DEBUG")!=nullptr;
    if(census_mode)setenv("HALO2_CENSUS_NATIVE_BRIDGES","1",1);
    if(verify)setenv("HALO2_VERIFY_DEVICE_UPDATE","1",1);
    std::mt19937 rng(0x1179c0u+(census_mode?1u:0u)+(verify?2u:0u));
    std::uint64_t stores{},compared{},faults{},children{};
    std::array<std::uint64_t,owners> per{};
    for(unsigned n=0;n<cases;++n) {
        auto c=author(rng,n);
        auto left=c.memory.clone(),right=c.memory.clone();
        const auto native_result=execute(right,c,true,debug);
        if(!native_result.admitted && native_result.fault.empty()){std::fprintf(stderr,"case %u refused\n",n);return 1;}
        const auto reference=execute(left,c,false,debug);
        ++compared;++per[c.owner];stores+=reference.stores.size();children+=reference.calls.size();
        if(!reference.fault.empty()) {
            ++faults;
            if(std::getenv("HALO2_DEVICE_FAULTS"))std::fprintf(stderr,"fault owner %u shape %u: %s\n",c.owner,(n/owners)%16,reference.fault.c_str());
        }
        const auto fail=[&](const char *what) {
            std::fprintf(stderr,"case %u (owner %u, shape %u) %s\n",n,c.owner,(n/owners)%16,what);
            if(debug) {
                const auto &a=native_result.entries,&b=reference.entries;
                for(std::size_t k=0;k<a.size() && k<b.size();++k)
                    if(std::memcmp(&a[k],&b[k],sizeof(Entry))){std::fprintf(stderr,"  first differing child %zu of %zu/%zu\n",k,a.size(),b.size());describe(a[k],b[k]);break;}
                if(a.size()!=b.size())std::fprintf(stderr,"  children %zu/%zu\n",a.size(),b.size());
                describe_abi(native_result.abi,reference.abi);
                std::fprintf(stderr,"  cursor %x/%x faults '%s'/'%s'\n",native_result.cursor,reference.cursor,native_result.fault.c_str(),reference.fault.c_str());
            }
            return 1;
        };
        if(native_result.fault!=reference.fault) {
            std::fprintf(stderr,"case %u (owner %u) fault mismatch: native '%s' translated '%s'\n",n,c.owner,
                native_result.fault.c_str(),reference.fault.c_str());
            return fail("fault");
        }
        if(native_result.stores!=reference.stores) {
            std::size_t at=0;while(at<native_result.stores.size()&&at<reference.stores.size()&&native_result.stores[at]==reference.stores[at])++at;
            std::fprintf(stderr,"case %u (owner %u) store mismatch at %zu of %zu/%zu",n,c.owner,at,native_result.stores.size(),reference.stores.size());
            if(at<native_result.stores.size()&&at<reference.stores.size())
                std::fprintf(stderr," native pc=%x a=%x original pc=%x a=%x",native_result.stores[at].instruction,native_result.stores[at].address,
                    reference.stores[at].instruction,reference.stores[at].address);
            else if(at<native_result.stores.size())std::fprintf(stderr," native extra pc=%x",native_result.stores[at].instruction);
            else if(at<reference.stores.size())std::fprintf(stderr," original extra pc=%x",reference.stores[at].instruction);
            std::fprintf(stderr,"\n");return fail("stores");
        }
        if(native_result.calls!=reference.calls)return fail("child call log mismatch");
        if(native_result.mxcsr!=reference.mxcsr) {
            std::fprintf(stderr,"case %u (owner %u) MXCSR %x vs %x\n",n,c.owner,native_result.mxcsr,reference.mxcsr);return fail("mxcsr");
        }
        if(!census_mode && native_result.cursor!=reference.cursor) {
            std::fprintf(stderr,"case %u (owner %u) cursor %x vs %x\n",n,c.owner,native_result.cursor,reference.cursor);return fail("cursor");
        }
        if(reference.fault.empty() && (!same_abi(native_result.abi,reference.abi) || !left.same(right)))
            return fail("register/flag/bank mismatch");
        if(!reference.fault.empty() && !left.same(right))return fail("fault bank mismatch");
    }
    // Admission refusals: other image, full callbacks without a cursor,
    // traces, arbitrary observers, a full x87 stack and unmasked exceptions.
    if(!census_mode) {
        auto c=author(rng,0);SourcePhysicsContactRecordObserver observe;SourcePhysicsWorldRuntimeServices services;
        WorldState a{c.memory,observe,services,SourcePhysicsContactImage::MapPack11};a.load(c.abi);
        if(native(machine,a))throw std::runtime_error("MapPack11 admitted");
        services.instruction=[](auto){};
        WorldState b{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};b.load(c.abi);
        if(native(device,b))throw std::runtime_error("Full instruction callbacks admitted");
        bool trace=true;std::uint32_t cursor{};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
        services.instruction=[](auto){};
        services.instruction_cursor.position=&cursor;services.instruction_cursor.service_sites=sites;
        services.instruction_cursor.callback_type=&services.instruction.target_type();
        services.instruction_cursor.executor=std::this_thread::get_id();services.instruction_cursor.literal_service_sites=true;
        services.instruction_cursor.first_trace=&trace;services.instruction_cursor.observation_only_stores=true;
        WorldState d{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};d.load(c.abi);
        if(native(machine,d))throw std::runtime_error("Active trace admitted");
        trace=false;services.instruction_cursor.observation_only_stores=false;
        SourcePhysicsContactRecordObserver arbitrary=[](const auto&){};
        WorldState e{c.memory,arbitrary,services,SourcePhysicsContactImage::TitleUpdate15};e.load(c.abi);
        if(native(device,e))throw std::runtime_error("Arbitrary observer admitted");
        SourcePhysicsWorldRuntimeServices plain;
        auto full=c.abi;full.x87_depth=7;
        WorldState g{c.memory,observe,plain,SourcePhysicsContactImage::TitleUpdate15};g.load(full);
        if(native(machine,g))throw std::runtime_error("A full x87 stack admitted");
        WorldState h{c.memory,observe,plain,SourcePhysicsContactImage::TitleUpdate15};h.load(c.abi);
        const auto saved=_mm_getcsr();_mm_setcsr(0x1f00u);
        const bool unmasked=native(device,h);_mm_setcsr(saved);
        if(unmasked)throw std::runtime_error("Unmasked MXCSR exceptions admitted");
    }
    std::printf("{\"cases\":%u,\"compared\":%llu,\"faults\":%llu,\"ordered_stores\":%llu,\"children\":%llu,"
        "\"per_owner\":[%llu,%llu],\"machine_coverage\":[%llu,%llu],\"device_coverage\":[%llu,%llu],\"mode\":\"%s\"}\n",
        cases,(unsigned long long)compared,(unsigned long long)faults,(unsigned long long)stores,(unsigned long long)children,
        (unsigned long long)per[0],(unsigned long long)per[1],
        (unsigned long long)dev::machine_coverage()[0].load(),(unsigned long long)dev::machine_coverage()[1].load(),
        (unsigned long long)dev::device_coverage()[0].load(),(unsigned long long)dev::device_coverage()[1].load(),mode.c_str());
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
