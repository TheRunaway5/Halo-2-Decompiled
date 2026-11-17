// Native TU1.5 object tick children (BE850/BE610/C0F60/10DD80/C0200/1D5790/
// 1E15D0/C0910) against the unmodified translated bodies, extracted by
// tools/validate_native_object_subtree_tu15.py into subtree_<owner>.inc, plus
// the verification references (subtree_<owner>_reference.inc) the production
// binder generates. Every child (direct, dispatched or reached through the
// required owner) is a genuine stand-in whose effects (stores into the object
// table and data, registers including callee-saved clobbers, stack balance,
// flags, XMM) are a hash of its complete entry ABI and the bytes it can see,
// so any entry difference diverges. Seeded fixtures author complete object
// tables (headers, data blocks, acyclic child/sibling chains, type move
// callback chains, cluster records, model tags, node attachment lists,
// countdown timers, node light rows with contribution/scale lists and
// special floats under every MXCSR mode) with special flag patterns, both direction flags,
// invalid handles and fault injection (one fixture page removed). Each case
// compares the ordered store events (pc, address, preimage, bytes), the child
// call log, final bank, registers, flags, XMM, x87 status and instruction
// cursor; fault cases compare the exception, the cursor and the store prefix.
// Modes: <cases> [census|verify]. Prints a JSON summary with branch coverage.
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_object_subtree.hpp"
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
namespace sub=halo2::native_object_subtree_detail;
namespace {
std::uint32_t sign_extend_value(std::uint32_t value,unsigned count) {
    if(count==1)return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value))));
    if(count==2)return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
    throw std::runtime_error("Unverified sign-extension width");
}
std::uint64_t mix(std::uint64_t h,std::uint64_t v){h^=v+0x9e3779b97f4a7c15ull+(h<<6)+(h>>2);return h*0xff51afd7ed558ccdull;}

constexpr std::uint32_t header=0x10200000u,elements=0x10300000u,data=0x10500000u,game=0x10600000u,
    clusters=0x10700000u,records=0x10800000u,types=0x10900000u,components=0x10a00000u,scratch=0x10b00000u,
    tags=0x10c00000u,tagdata=0x10d00000u,timers=0x10e00000u,lights=0x10f00000u,entries=0x11000000u,stack=0x20010000u;
// Per-case stand-in policy: bit0 callee-saved clobbers, bit1 stack
// imbalance, bit2 XMM changes, bit3 flag-byte churn.
std::uint32_t policy{};std::uint32_t objects{};
std::vector<std::uint64_t> *call_log{};
struct Expect {unsigned bytes;std::set<std::uint32_t> returns;};
const std::map<std::uint32_t,Expect> &expectations() {
    static const std::map<std::uint32_t,Expect> value{
        {0xbe610u,{4,{0xbe883u}}},{0xbf0c0u,{4,{0xbe897u}}},{0x1dbbd0u,{12,{0xbe911u}}},{0x10dd80u,{0,{0xbe91bu}}},
        {0xbff20u,{4,{0xbe946u}}},{0xc0f60u,{20,{0xbe95eu,0xc1059u}}},{0xbe850u,{4,{0xbe96cu}}},{0xc0200u,{0,{0xbe991u}}},
        {0x1d9ec0u,{0,{0xbe669u}}},{0x1d9f50u,{0,{0xbe6a8u}}},{0x1dc610u,{4,{0xbe6bau}}},{0xbb200u,{0,{0xbe6e7u}}},
        {0xb9460u,{36,{0xbe706u}}},{0x1d8c80u,{0,{0xbe71du}}},{0x1d8d80u,{0,{0xbe734u}}},{0xb9770u,{4,{0xbe753u}}},
        {0x1e1980u,{0,{0xbe78au}}},{0xbbd10u,{0,{0xc0fb5u}}},{0xc5250u,{0,{0xc1004u}}},{0xc4d30u,{4,{0xc1014u}}},
        {0xc3750u,{12,{0xc1039u}}},{0xbac30u,{4,{0xc025au}}},{0x10e1f0u,{8,{0xc0265u}}},{0x1e15d0u,{0,{0x1d5799u,0x1d57b3u}}},
        {0xbcb70u,{12,{0xc09f1u,0xc0a5cu}}},{0x134730u,{0,{0xc0a14u}}},
    };
    return value;
}
std::uint32_t sp_float_out(WorldState &s){return r(s.memory,s.gp[4]+12u);}
// NaN-dense cases (one C0910 shape) make dual-NaN MULSS operands common, so
// the destination's payload choice is observed.
bool nan_dense{};
std::uint32_t light_float(std::mt19937_64 &rng) {
    static constexpr std::uint32_t values[]{0x7fc00001u,0xffc00000u,0x7f800000u,0xff800000u,0x00000001u,0x807fffffu,
        0x80000000u,0x00000000u,0x3f800000u,0x7f7fffffu};
    if(nan_dense && rng()%2)return 0x7fc00000u|std::uint32_t(rng()%0x400000u)|(rng()%2?0x80000000u:0u);
    if(rng()%7==0)return values[rng()%10];
    return std::bit_cast<std::uint32_t>(float(int(rng()%61)-20)/20.0f);
}
// A genuine child stand-in: every effect is a hash of the entry ABI, the
// return address/arguments and the object flag bytes it can observe.
void stub(WorldState &s,std::uint32_t target) {
    auto found=expectations().find(target);
    unsigned bytes=4;std::uint32_t expected_return=0;
    const auto back=r(s.memory,s.gp[4]);
    if(found!=expectations().end()) {
        bytes=found->second.bytes;
        if(!found->second.returns.count(back)) {
            char text[96];std::snprintf(text,sizeof text,"Child %x entered with return address %x",target,back);
            throw std::runtime_error(text);
        }
        expected_return=back;
    } else if(target>=0x300000u && target<0x300100u) {
        if(back!=0x10ddc3u)throw std::runtime_error("Move callback entered with a wrong return address");
        expected_return=back;
    } else throw std::runtime_error("Unbound child target");
    std::uint64_t h=mix(0x0b1ec7u,target);
    for(const auto v:s.gp)h=mix(h,v);
    for(unsigned i=0;i<3;++i)h=mix(h,r(s.memory,s.gp[4]+i*4u));
    h=mix(h,s.flags.bits());h=mix(h,s.other_flags);h=mix(h,s.fp.status);h=mix(h,expected_return);
    for(unsigned i=0;i<8;++i){std::array<std::uint32_t,4> b;std::memcpy(b.data(),&s.xmm[i],16);for(auto v:b)h=mix(h,v);}
    for(std::uint32_t i=0;i<objects && i<8;++i)h=mix(h,r(s.memory,elements+i*12u+2u,1));
    std::mt19937_64 rng(h);
    if(call_log) {
        // A deterministic budget ends clobber-induced rescans identically.
        if(call_log->size()>=4096)throw std::runtime_error("Child call budget exceeded");
        call_log->push_back(mix(h,target));
    }
    w(s.memory,s.observe,target,scratch+std::uint32_t(rng()%64)*4u,4,std::uint32_t(rng()));
    if(objects && (policy&8u || rng()%3==0)) {
        const auto index=std::uint32_t(rng()%objects);
        w(s.memory,s.observe,target+1u,elements+index*12u+2u,1,std::uint32_t(rng()));
        if(rng()%2) {
            const auto block=data+index*0x200u;
            static constexpr std::uint32_t offsets[]{0xc0u,0xc1u,0xb4u,0x4u};
            const auto at=offsets[rng()%4];
            w(s.memory,s.observe,target+2u,block+at,at==0xb4u?4:1,rng()%5==0?0xffffffffu:std::uint32_t(rng()%objects));
        }
    }
    if(rng()%5==0)w(s.memory,s.observe,target+3u,timers+std::uint32_t(rng()%8),1,std::uint32_t(rng()));
    // BCB70 writes its weight through the third argument; 134730
    // accumulates into the row at EDI.
    if(target==0xbcb70u && rng()%4)w(s.memory,s.observe,target+4u,sp_float_out(s),4,light_float(rng));
    if(target==0x134730u && rng()%2)for(unsigned k=0;k<3;++k)w(s.memory,s.observe,target+4u,s.gp[7]+k*4u,4,light_float(rng));
    const auto sp=s.gp[4];
    // 1E15D0 keeps ECX (its caller rereads the timer through it).
    const bool keeps_ecx=target==0x1e15d0u && rng()%4;
    s.gp[0]=std::uint32_t(rng());s.gp[1]=keeps_ecx?s.gp[1]:std::uint32_t(rng());s.gp[2]=std::uint32_t(rng());
    if(rng()%3)s.gp[0]&=0xffffff00u; // most children report no change in AL
    if(policy&1u && rng()%5==0) {
        static constexpr std::uint32_t values[]{0xfffffffeu,0x7fffffffu,0xffffffffu,0u,1u,0x80000000u};
        static constexpr unsigned saved[]{3,5,6,7}; // EBX/EBP/ESI/EDI
        s.gp[saved[rng()%4]]=values[rng()%6];
    }
    if(policy&4u)for(unsigned i=0;i<8;++i){std::array<std::uint32_t,4> b;for(auto &v:b)v=std::uint32_t(rng());std::memcpy(&s.xmm[i],b.data(),16);}
    s.flags.load(std::uint32_t(rng())&0x8d5u);
    std::uint32_t next=sp+4u+bytes;
    if(policy&2u && rng()%9==0)next+=rng()%2?4u:std::uint32_t(-4);
    s.gp[4]=next;
}
// The required owner (BF0C0/BFF20 from BE850, C4D30 from C0F60's C100F, the
// light callers' boundary): the stand-in on the ABI.
void required_stub(SourcePhysicsContactMemory &m,const SourcePhysicsContactRuntimeCall &call,SourcePhysicsWorldRuntimeAbi &a,
                   const SourcePhysicsContactRecordObserver &observe) {
    const auto expected=call.target==0xbf0c0u?0xbe892u:call.target==0xbff20u?0xbe941u:call.target==0xc4d30u?0xc100fu:0u;
    if(!expected || call.instruction!=expected || call.tail)throw std::runtime_error("Required child at a wrong site");
    SourcePhysicsWorldRuntimeServices none;
    WorldState t{m,observe,none,SourcePhysicsContactImage::TitleUpdate15};t.load(a);
    stub(t,call.target);a=t.save();
}
} // namespace

#define SUB_CHILD(name) static void run_##name(WorldState &s){stub(s,0x##name##u);}
#define SUB_DISPATCH static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t,bool){stub(s,target);}
#define SUB_PRELUDE using halo2::physics_world_runtime_detail::WorldState; \
    std::uint32_t sign_extend(std::uint32_t v,unsigned n){return sign_extend_value(v,n);}
namespace t_be850 {
SUB_PRELUDE
SUB_CHILD(be610) SUB_CHILD(1dbbd0) SUB_CHILD(10dd80) SUB_CHILD(c0f60) SUB_CHILD(c0200)
static void run_be850(WorldState &);
#include "subtree_be850.inc"
#include "subtree_be850_reference.inc"
}
namespace t_be610 {
SUB_PRELUDE
SUB_DISPATCH
SUB_CHILD(1d9f50) SUB_CHILD(1dc610) SUB_CHILD(bb200) SUB_CHILD(b9460) SUB_CHILD(1e1980)
#include "subtree_be610.inc"
#include "subtree_be610_reference.inc"
}
namespace t_c0f60 {
SUB_PRELUDE
SUB_CHILD(bbd10) SUB_CHILD(c5250) SUB_CHILD(c3750)
static void run_c0f60(WorldState &);
#include "subtree_c0f60.inc"
#include "subtree_c0f60_reference.inc"
}
namespace t_10dd80 {
SUB_PRELUDE
SUB_DISPATCH
#include "subtree_10dd80.inc"
#include "subtree_10dd80_reference.inc"
}
namespace t_c0200 {
SUB_PRELUDE
SUB_CHILD(bac30) SUB_CHILD(10e1f0)
#include "subtree_c0200.inc"
#include "subtree_c0200_reference.inc"
}
namespace t_1d5790 {
SUB_PRELUDE
SUB_CHILD(1e15d0)
#include "subtree_1d5790.inc"
#include "subtree_1d5790_reference.inc"
}
namespace t_1e15d0 {
SUB_PRELUDE
#include "subtree_1e15d0.inc"
#include "subtree_1e15d0_reference.inc"
}
namespace t_c0910 {
SUB_PRELUDE
SUB_CHILD(bcb70) SUB_CHILD(134730)
#include "subtree_c0910.inc"
#include "subtree_c0910_reference.inc"
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

enum Owner : unsigned {be850,be610,c0f60,dd80,c0200,d5790,e15d0,c0910,owners};
struct Case {Memory memory;SourcePhysicsWorldRuntimeAbi abi{};Owner owner{};std::uint32_t policy{},objects{},mxcsr{0x1f80u};bool nan_dense{};};

std::uint32_t handle(std::mt19937 &rng,std::uint32_t index){return (std::uint32_t(rng()%0x10000u)<<16)|index;}

// One authored world. `shape` selects structural edge cases.
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    c.owner=Owner(n%owners);
    const unsigned shape=(n/owners)%16;
    nan_dense=c.nan_dense=c.owner==c0910 && shape%4==1;
    c.policy=pick(16);
    std::uint32_t count=1+pick(40);
    if(shape==3)count=1+pick(4);
    c.objects=count;
    m.put(0x4ecbd0u,header);m.put(0x4f3218u,game);m.put(0x52be5cu,clusters);m.put(0x4f0414u,tags);
    m.put(0x474d04u,std::uint32_t(rng()));
    m.put(header+0x44u,elements);m.put(clusters+0x44u,records);
    for(std::uint32_t i=0;i<count+2;++i) {
        const auto e=elements+i*12u;
        m.put(e,pick(0x10000),2);
        std::uint32_t flags=std::uint32_t(rng())&0xffu;
        if(pick(3)==0)flags&=~0x10u;
        m.put(e+2u,flags,1);
        m.put(e+3u,pick(3)==0?7u+32u*pick(8):pick(256),1);
        m.put(e+4u,std::uint32_t(rng()));m.put(e+8u,data+i*0x200u);
    }
    // Object data: acyclic child/sibling chains (later indices only).
    for(std::uint32_t i=0;i<count+2;++i) {
        const auto d=data+i*0x200u;
        for(std::uint32_t at=0;at<0x200u;at+=4)m.put(d+at,std::uint32_t(rng()));
        const auto later=[&]{return i+1<count?handle(rng,i+1+pick(count-i-1)):0xffffffffu;};
        m.put(d+0x10u,pick(3)?0xffffffffu:later());
        m.put(d+0xcu,pick(2)?0xffffffffu:later());
        m.put(d+0x14u,pick(3)==0?0xffffffffu:handle(rng,pick(count)));
        m.put(d,pick(10)==0?pick(0x10000):pick(24),4);
        m.put(d+0xaau,pick(9)==0?std::uint32_t(-1-int(pick(3)))&0xffu:pick(12),1);
        m.put(d+0xb4u,pick(8)==0?0xffffffffu:handle(rng,pick(24)));
        m.put(d+0x114u,pick(7)==0?std::uint32_t(rng()):pick(52*8*32/4),2);
        // Node attachments: count<<3 at +0x11C, list offset at +0x11E.
        const auto nodes=pick(5)==0?std::uint32_t(rng())&0xffffu:pick(6)*8u;
        m.put(d+0x11cu,nodes,2);
        m.put(d+0x11eu,0x140u,2);
        for(std::uint32_t k=0;k<6;++k) {
            m.put(d+0x140u+k*8u,pick(3)==0?pick(256):0u,1);
            m.put(d+0x144u+k*8u,pick(4)==0?0xffffffffu:handle(rng,pick(count)));
        }
    }
    for(std::uint32_t i=0;i<count+2;++i) {
        const auto d=data+i*0x200u;
        const auto child=r(m,d+0x10u),next=r(m,d+0xcu);
        if(child!=0xffffffffu && (child&0xffffu)<=i)m.put(d+0x10u,0xffffffffu);
        if(next!=0xffffffffu && (next&0xffffu)<=i)m.put(d+0xcu,0xffffffffu);
    }
    m.put(game+0xcu,pick(2)==0?4u:pick(8),1);
    // Cluster records (0xA0 bytes): +4 flags, +0x74 count.
    for(std::uint32_t k=0;k<24;++k) {
        const auto rec=records+k*0xa0u;
        for(std::uint32_t at=0;at<0xa0u;at+=4)m.put(rec+at,std::uint32_t(rng()));
        m.put(rec+0x74u,pick(4)==0?std::uint32_t(-int(pick(3))):1+pick(4));
    }
    // Model tags: [4F0414] table of 16-byte rows (+8 data), tag data +0x38
    // model handle, model +4/+0x14 node tags.
    for(std::uint32_t k=0;k<24;++k) {
        m.put(tags+k*16u,std::uint32_t(rng()));m.put(tags+k*16u+4u,std::uint32_t(rng()));
        m.put(tags+k*16u+8u,tagdata+k*0x40u);m.put(tags+k*16u+12u,std::uint32_t(rng()));
        const auto t=tagdata+k*0x40u;
        for(std::uint32_t at=0;at<0x40u;at+=4)m.put(t+at,std::uint32_t(rng()));
        m.put(t+0x38u,pick(4)==0?0xffffffffu:(std::uint32_t(rng())&0xffff0000u)|pick(24));
        m.put(t+4u,pick(4)==0?0xffffffffu:std::uint32_t(rng()));
        m.put(t+0x14u,pick(4)==0?0xffffffffu:std::uint32_t(rng()));
    }
    // Type table (movsx byte index; negative types read below the table),
    // move callbacks at component +0x44.
    for(std::uint32_t at=0x474b90u-0x200u;at<0x474b90u+0x200u;at+=4)
        if(at!=0x474d04u)m.put(at,types+pick(16)*0x100u);
    for(std::uint32_t t=0;t<16;++t) {
        const auto def=types+t*0x100u;
        const auto entries=t%5==0?0u:1+pick(4);
        for(std::uint32_t k=0;k<entries;++k) {
            const auto component=components+(t*8u+k)*0x80u;
            m.put(def+0x84u+k*4u,component);
            m.put(component+0x44u,pick(4)==0?0u:0x300000u+pick(0x100));
        }
        m.put(def+0x84u+entries*4u,0);
    }
    // Node lights (C0910): tag +0x1C flag, +0xB0 light block of 16-byte row
    // headers (+8 count, +0xC 0x28-byte entries: +4/+8/+0x14 accumulate
    // arguments, +0x20 scale and +0x24 contribution handles); object +0x124
    // row bytes (24 per row) and +0x126 the base colour rows, followed by
    // the lit rows; the clamp limit at [46A0D0].
    {
        std::mt19937_64 wide(rng());
        for(std::uint32_t k=0;k<24;++k) {
            const auto t=tagdata+k*0x40u;
            m.put(t+0x1cu,pick(5)==0?0u:1u|(std::uint32_t(rng())&0xfeu),1);
            m.put(t+0xb0u,lights+k*0x100u);
        }
        for(std::uint32_t k=0;k<24*16;++k) {
            const auto row=lights+k*0x10u;
            m.put(row,std::uint32_t(rng()));m.put(row+4u,std::uint32_t(rng()));
            m.put(row+8u,pick(9)==0?std::uint32_t(-int(pick(2))):pick(4));
            m.put(row+0xcu,entries+(k%32)*0x100u);
        }
        for(std::uint32_t at=entries;at<entries+32*0x100u;at+=4)m.put(at,std::uint32_t(rng()));
        for(std::uint32_t k=0;k<32*6;++k) {
            const auto e=entries+k*0x28u;
            m.put(e+0x20u,pick(3)==0?0u:handle(rng,pick(24)));m.put(e+0x24u,pick(3)==0?0u:handle(rng,pick(24)));
        }
        m.put(0x46a0d0u,pick(9)==0?light_float(wide):0x3f800000u);
        for(std::uint32_t i=0;i<count+2;++i) {
            const auto d=data+i*0x200u;
            const auto rows=pick(4);
            m.put(d+0x124u,pick(11)==0?std::uint32_t(rng())&0xffffu:rows*24u+pick(24),2);
            m.put(d+0x126u,0x178u,2);
            for(std::uint32_t at=0x178u;at<0x1f8u;at+=4)m.put(d+at,light_float(wide));
        }
    }
    // Countdown timers (counter, limit, -, flags).
    for(std::uint32_t at=timers;at<timers+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=timers;at<timers+0x100u;at+=4) {
        if(pick(3)==0)m.put(at+1u,0,1);
        if(pick(2))m.put(at+3u,pick(4),1);
        if(pick(3)==0)m.put(at,pick(3),1);
    }
    for(std::uint32_t at=scratch;at<scratch+0x100u;at+=4)m.put(at,0);
    for(std::uint32_t at=stack-0x800u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x13ab00u+pick(0x100));
    const auto object=pick(11)==0?handle(rng,count+pick(3)):handle(rng,pick(count));
    m.put(stack+4u,object);
    for(std::uint32_t k=8;k<0x18u;k+=4)m.put(stack+k,pick(3)==0?(std::uint32_t(rng())&0xffffff00u):pick(2));
    for(auto &v:c.abi.gp)v=std::uint32_t(rng());
    c.abi.gp[4]=stack;
    c.abi.eflags=0x202u|(std::uint32_t(rng())&0x8d5u)|(c.owner==be850 && pick(3)==0?0x400u:0u);
    for(auto &lane:c.abi.xmm)for(auto &b:lane)b=std::uint32_t(rng());
    if(c.owner==dd80)c.abi.gp[7]=object;
    if(c.owner==c0200)c.abi.gp[0]=object;
    if(c.owner==e15d0)c.abi.gp[1]=timers+pick(0xf0u);
    if(c.owner==d5790)c.abi.gp[6]=timers+pick(0x90u)-0x60u;
    if(c.owner==c0910) {
        static constexpr std::uint32_t modes[]{0x1f80u,0x3f80u,0x5f80u,0x7f80u,0x9fc0u,0x1fc0u,0x9f80u,0x1fbfu};
        c.mxcsr=modes[pick(8)];
    }
    // Fault injection: remove one authored page (not the stack).
    if(shape==15) {
        static constexpr std::uint32_t regions[]{elements,data,types,components,0x474000u,game,records,tags,tagdata,timers,
            lights,entries,0x46a000u};
        const auto region=regions[pick(13)];
        auto at=m.pages.lower_bound(region>>12);
        if(at!=m.pages.end()){std::advance(at,std::min<std::size_t>(pick(3),std::distance(at,m.pages.end())-1));m.pages.erase(at);}
    }
    return c;
}

struct Result {
    std::vector<SourcePhysicsContactRecordStore> stores;std::vector<std::uint64_t> calls;
    SourcePhysicsWorldRuntimeAbi abi{};std::uint32_t cursor{},mxcsr{};bool admitted{};std::string fault;
};
bool census_mode{},verify{};
void native_be850(WorldState &s);void native_c0f60(WorldState &s);
template<class V> bool native(Owner owner,V &s) {
    const auto dispatch_be610=[](auto &v,std::uint32_t target,std::uint32_t site){t_be610::dispatch(v,target,site,false);};
    const auto dispatch_dd80=[](auto &v,std::uint32_t target,std::uint32_t site){t_10dd80::dispatch(v,target,site,false);};
    switch(owner) {
    case be850:return sub::bridge_be850(s,[](auto &v){t_be850::native_object_subtree_reference_be850(v);},
        [](auto &v,std::uint32_t target,std::uint32_t site){v.required(site,target,false);},
        [](auto &v){t_be850::run_be610(v);},[](auto &v){t_be850::run_1dbbd0(v);},[](auto &v){t_be850::run_10dd80(v);},
        [](auto &v){t_be850::run_c0f60(v);},[](auto &v){native_be850(v);},[](auto &v){t_be850::run_c0200(v);});
    case be610:return sub::bridge_be610(s,[](auto &v){t_be610::native_object_subtree_reference_be610(v);},dispatch_be610,
        [](auto &v){t_be610::run_1d9f50(v);},[](auto &v){t_be610::run_1dc610(v);},[](auto &v){t_be610::run_bb200(v);},
        [](auto &v){t_be610::run_b9460(v);},[](auto &v){t_be610::run_1e1980(v);});
    // C4D30 leaves through the required owner at its original site (the
    // production binder's REQUIRED_CHILDREN route).
    case c0f60:return sub::bridge_c0f60(s,[](auto &v){t_c0f60::native_object_subtree_reference_c0f60(v);},
        [](auto &v){t_c0f60::run_bbd10(v);},[](auto &v){t_c0f60::run_c5250(v);},
        [](auto &v){v.required(r(v.memory,v.gp[4],4)-5u,0xc4d30u,false);},
        [](auto &v){t_c0f60::run_c3750(v);},[](auto &v){native_c0f60(v);});
    case dd80:return sub::bridge_10dd80(s,[](auto &v){t_10dd80::native_object_subtree_reference_10dd80(v);},dispatch_dd80);
    case c0200:return sub::bridge_c0200(s,[](auto &v){t_c0200::native_object_subtree_reference_c0200(v);},
        [](auto &v){t_c0200::run_bac30(v);},[](auto &v){t_c0200::run_10e1f0(v);});
    case d5790:return sub::bridge_1d5790(s,[](auto &v){t_1d5790::native_object_subtree_reference_1d5790(v);},
        [](auto &v){t_1d5790::run_1e15d0(v);});
    case e15d0:return sub::bridge_1e15d0(s,[](auto &v){t_1e15d0::native_object_subtree_reference_1e15d0(v);});
    case c0910:return sub::bridge_c0910(s,[](auto &v){t_c0910::native_object_subtree_reference_c0910(v);},
        [](auto &v){t_c0910::run_bcb70(v);},[](auto &v){t_c0910::run_134730(v);});
    default:return false;
    }
}
// Recursive children run natively at every level.
void native_be850(WorldState &s){if(!native(be850,s))throw std::runtime_error("Recursive BE850 refused");}
void native_c0f60(WorldState &s){if(!native(c0f60,s))throw std::runtime_error("Recursive C0F60 refused");}
void translated(Owner owner,WorldState &s) {
    switch(owner) {
    case be850:t_be850::run_be850(s);return;
    case be610:t_be610::run_be610(s);return;
    case c0f60:t_c0f60::run_c0f60(s);return;
    case dd80:t_10dd80::run_10dd80(s);return;
    case c0200:t_c0200::run_c0200(s);return;
    case d5790:t_1d5790::run_1d5790(s);return;
    case e15d0:t_1e15d0::run_1e15d0(s);return;
    case c0910:t_c0910::run_c0910(s);return;
    default:return;
    }
}
Result execute(Memory &memory,const Case &c,bool is_native) {
    Result result;
    SourcePhysicsContactRecordObserver observe=[&](const SourcePhysicsContactRecordStore &e){result.stores.push_back(e);};
    SourcePhysicsWorldRuntimeServices services;
    services.required_owner=required_stub;
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    if(is_native && census_mode) {
        services.instruction=[](auto){throw std::runtime_error("Native object subtree delivered a translated instruction");};
    } else if(is_native) {
        services.instruction=[](auto){throw std::runtime_error("Native object subtree delivered a translated instruction");};
        auto &cursor=services.instruction_cursor;cursor.position=&result.cursor;cursor.service_sites=sites;
        cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();
        cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    } else services.instruction=[&](std::uint32_t pc){result.cursor=pc;};
    WorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(c.abi);
    const auto saved=_mm_getcsr();_mm_setcsr(c.mxcsr);
    policy=c.policy;objects=c.objects;call_log=&result.calls;nan_dense=c.nan_dense;
    try {
        if(is_native)result.admitted=native(c.owner,s);
        else translated(c.owner,s);
    } catch(const std::exception &e){result.fault=e.what();}
    call_log=nullptr;result.mxcsr=_mm_getcsr();_mm_setcsr(saved);
    result.abi=s.save();return result;
}
bool same_abi(const SourcePhysicsWorldRuntimeAbi &a,const SourcePhysicsWorldRuntimeAbi &b) {
    return a.gp==b.gp && a.eflags==b.eflags && a.xmm==b.xmm && a.x87_status==b.x87_status && a.x87_depth==b.x87_depth;
}
} // namespace

int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):5000;
    const std::string mode=argc>2?argv[2]:"";
    census_mode=mode=="census";verify=mode=="verify";
    if(census_mode)setenv("HALO2_CENSUS_NATIVE_BRIDGES","1",1);
    if(verify)setenv("HALO2_VERIFY_OBJECT_SUBTREE","1",1);
    std::mt19937 rng(0xbe850u+(census_mode?1u:0u)+(verify?2u:0u));
    std::uint64_t stores{},compared{},faults{},children{};
    std::array<std::uint64_t,owners> per{};
    for(unsigned n=0;n<cases;++n) {
        auto c=author(rng,n);
        auto left=c.memory.clone(),right=c.memory.clone();
        const auto native_result=execute(right,c,true);
        if(!native_result.admitted && native_result.fault.empty()){std::fprintf(stderr,"case %u refused\n",n);return 1;}
        const auto reference=execute(left,c,false);
        ++compared;++per[c.owner];stores+=reference.stores.size();children+=reference.calls.size();
        if(!reference.fault.empty()) {
            ++faults;
            if(std::getenv("HALO2_SUBTREE_FAULTS"))std::fprintf(stderr,"fault owner %u shape %u: %s\n",c.owner,(n/owners)%16,reference.fault.c_str());
        }
        if(native_result.fault!=reference.fault) {
            std::fprintf(stderr,"case %u (owner %u) fault mismatch: native '%s' translated '%s'\n",n,c.owner,
                native_result.fault.c_str(),reference.fault.c_str());return 1;
        }
        if(native_result.stores!=reference.stores) {
            std::size_t at=0;while(at<native_result.stores.size()&&at<reference.stores.size()&&native_result.stores[at]==reference.stores[at])++at;
            std::fprintf(stderr,"case %u (owner %u) store mismatch at %zu of %zu/%zu",n,c.owner,at,native_result.stores.size(),reference.stores.size());
            if(at<native_result.stores.size()&&at<reference.stores.size())
                std::fprintf(stderr," native pc=%x a=%x original pc=%x a=%x",native_result.stores[at].instruction,native_result.stores[at].address,
                    reference.stores[at].instruction,reference.stores[at].address);
            std::fprintf(stderr,"\n");return 1;
        }
        if(native_result.calls!=reference.calls){std::fprintf(stderr,"case %u (owner %u) child call log mismatch\n",n,c.owner);return 1;}
        if(native_result.mxcsr!=reference.mxcsr) {
            std::fprintf(stderr,"case %u (owner %u) MXCSR %x vs %x\n",n,c.owner,native_result.mxcsr,reference.mxcsr);return 1;
        }
        if(!census_mode && native_result.cursor!=reference.cursor) {
            std::fprintf(stderr,"case %u (owner %u) cursor %x vs %x\n",n,c.owner,native_result.cursor,reference.cursor);return 1;
        }
        if(reference.fault.empty() && (!same_abi(native_result.abi,reference.abi) || !left.same(right))) {
            std::fprintf(stderr,"case %u (owner %u) register/flag/bank mismatch\n",n,c.owner);return 1;
        }
        if(!reference.fault.empty() && !left.same(right)){std::fprintf(stderr,"case %u fault bank mismatch\n",n);return 1;}
    }
    // Admission refusals: other image, full callbacks without a cursor,
    // traces and arbitrary observers.
    if(!census_mode) {
        auto c=author(rng,0);SourcePhysicsContactRecordObserver observe;SourcePhysicsWorldRuntimeServices services;
        WorldState a{c.memory,observe,services,SourcePhysicsContactImage::MapPack11};a.load(c.abi);
        if(native(be850,a))throw std::runtime_error("MapPack11 admitted");
        services.instruction=[](auto){};
        WorldState b{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};b.load(c.abi);
        if(native(be610,b))throw std::runtime_error("Full instruction callbacks admitted");
        bool trace=true;std::uint32_t cursor{};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
        services.instruction=[](auto){};
        services.instruction_cursor.position=&cursor;services.instruction_cursor.service_sites=sites;
        services.instruction_cursor.callback_type=&services.instruction.target_type();
        services.instruction_cursor.executor=std::this_thread::get_id();services.instruction_cursor.literal_service_sites=true;
        services.instruction_cursor.first_trace=&trace;services.instruction_cursor.observation_only_stores=true;
        WorldState d{c.memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};d.load(c.abi);
        if(native(c0f60,d))throw std::runtime_error("Active trace admitted");
        trace=false;services.instruction_cursor.observation_only_stores=false;
        SourcePhysicsContactRecordObserver arbitrary=[](const auto&){};
        WorldState e{c.memory,arbitrary,services,SourcePhysicsContactImage::TitleUpdate15};e.load(c.abi);
        if(native(dd80,e))throw std::runtime_error("Arbitrary observer admitted");
    }
    std::printf("{\"cases\":%u,\"compared\":%llu,\"faults\":%llu,\"ordered_stores\":%llu,\"children\":%llu,"
        "\"per_owner\":[%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu],\"move_coverage\":%llu,\"attach_coverage\":%llu,\"mode\":\"%s\"}\n",
        cases,(unsigned long long)compared,(unsigned long long)faults,(unsigned long long)stores,(unsigned long long)children,
        (unsigned long long)per[0],(unsigned long long)per[1],(unsigned long long)per[2],(unsigned long long)per[3],
        (unsigned long long)per[4],(unsigned long long)per[5],(unsigned long long)per[6],(unsigned long long)per[7],
        (unsigned long long)sub::move_coverage().load(),(unsigned long long)sub::attach_coverage().load(),mode.c_str());
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
